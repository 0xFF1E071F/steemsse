#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#include "../pch.h"


#if defined(SSE_DRIVE)

#include <cpu.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include <run.decla.h>
#include "SSECpu.h"
#include "SSEInterrupt.h"
#include "SSEShifter.h"
#if defined(WIN32)
#include <pasti/pasti.h>
#endif
EasyStr GetEXEDir();//#include <mymisc.h>//missing...

#include "SSEDebug.h"
#include "SSEDrive.h"
#include "SSEFloppy.h"

#define LOGSECTION LOGSECTION_FDC

/*  Problems concerning the drive itself (RPM...) or the disks (gaps...) are
    treated here.
*/
/*  TODO
    There are still some hacks in drive emulation, we keep them so some
    programs run:
    Microprose Golf
    FDCTNF by Petari
    See SSEParameters.h
*/


/*

from JLG, floppy disk gaps

# bytes 
Name                       9 Sectors  10 Sectors  11 Sectors     Byte
Gap 1 Post Index                  60          60          10      4E

Gap 2 Pre ID                    12+3        12+3         3+3     00+A1
ID Address Mark                    1           1           1      FE
ID                                 6           6           6
Gap 3a Post ID                    22          22          22      4E 
Gap 3b Pre Data                 12+3        12+3        12+3     00+A1
Data Address Mark                  1           1           1      FB
Data                             512         512         512
CRC                                2           2           2
Gap 4 Post Data                   40          40           1      4E

Record bytes                     614         614         566

Gap 5 Pre Index                  664          50          20      4E

Total track                     6250        6250        6256


    Note that the index position can be anywhere when the disk begins
    to spin.
    But, from Hatari:
    "
 * As the FDC waits 6 index pulses during the spin up phase, this means
 * that when motor reaches its desired speed an index pulse was just
 * encountered.
 * So, the position after peak speed is reached is not random, it will always
 * be 0 and we set the index pulse time to "now".
    " 
    In Steem both the random starting position when the disk starts spinning
    and the determined position when it's spun up are emulated by setting
    IP spots absolutely (hbl_couunt%FDC_HBLS_PER_ROTATION=0).

    We do our computing using bytes, then convert the result into HBL, the
    timing unit for drive operations in Steem. Some computation is done in 
    HBL directly.

    Up to now we haven't needed a more precise timing, but who knows, this
    could explain why we must take 6250+20 bytes/track.
   
*/


TSF314::TSF314() {

#if defined(SSE_DRIVE_SOUND)
  //TRACE("null %d sound buffer pointers\n",NSOUNDS);
  for(int i=0;i<NSOUNDS;i++)
    Sound_Buffer[i]=NULL;
#if defined(SSE_DRIVE_SOUND_VOLUME)
  Sound_Volume=0; //changed by option
#endif
#endif//sound

#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM)
  SectorChecksum=0;
#endif

#if defined(SSE_DRIVE_INDEX_PULSE)
  rpm=300; // could be changed in other version

  time_of_next_ip=0;//useful?
  time_of_last_ip=0;//useful?
#endif

}


bool TSF314::Adat() { // accurate disk access times
  return (!floppy_instant_sector_access
#if USE_PASTI
    ||pasti_active 
#endif
#if defined(SSE_IPF)
    ||Caps.IsIpf(Id)
#endif
#if defined(SSE_DISK_STW)
    ||ImageType.Extension==EXT_STW
#endif
    );
}


WORD TSF314::BytePosition() {
/* 
    This assumes constant bytes/track (some protected disks have more)
    This should be 0-6256
    This is independent of #sectors
    This is independent of disk type
    This is based on Index Pulse, that is sent by the drive 
*/
  WORD position=0;
#ifdef SSE_DISK_STW
  if(IMAGE_STW) // called by Read() and Write()
  {
    int nb=time_of_next_ip-time_of_last_ip;
    nb/=cycles_per_byte;//temp!
    position=(ACT-time_of_last_ip)/cycles_per_byte;
  }
  else
#endif
    position=HblsToBytes( hbl_count% HblsPerRotation() );
  return position;

}


WORD TSF314::BytePositionOfFirstId() { // with +7 for reading ID //no!
 // TRACE_IDE("%d %d %d\n",PostIndexGap(),nSectors(),PostIndexGap() + ( (nSectors()<11)?12+3+7-7:3+3+7-7));
  return ( PostIndexGap() + ( (nSectors()<11)?12+3+7-7+1:3+3+7-7+1) );
}


WORD TSF314::BytesToHbls(int bytes) {
  return HblsPerRotation()*bytes/TRACK_BYTES;
}


#if defined(SSE_DRIVE_RW_SECTOR_TIMING3)

WORD TSF314::BytesToID(BYTE &num,WORD &nHbls) {
/*  Compute distance in bytes between current byte and desired ID
    identified by 'num' (sector)
    return 0 if it doesn't exist
    if num=0, assume next ID, num will contain sector index, 1-based
    (so, not num!)
*/

  WORD bytes_to_id=0;
  const WORD current_byte=BytePosition();

  if(FloppyDrive[Id].Empty())
    ;
  else
  {
    //here we assume normal ST disk image, sectors are 1...10
    WORD record_length=RecordLength();
    BYTE n_sectors=nSectors();
    WORD byte_first_id=BytePositionOfFirstId();
    WORD byte_last_id=byte_first_id+(n_sectors-1)*record_length;
    WORD byte_target_id;

    // If we're looking for whatever next num, we compute it first
    if(!num)
    {
      num=(current_byte-byte_first_id)/record_length+1; // current
      num++;
      if(num==n_sectors+1) num=1; //TODO smart way
    }

    byte_target_id=byte_first_id+(num-1)*record_length;

    if(current_byte<=byte_target_id) // this rev
      bytes_to_id=byte_target_id-current_byte;
    else                            // next rev
      bytes_to_id=TRACK_BYTES-current_byte+byte_target_id;
  }
  nHbls=BytesToHbls(bytes_to_id);
  return bytes_to_id;
}

#endif//#if defined(SSE_DRIVE_RW_SECTOR_TIMING3)


#if defined(SSE_DISK_GHOST)

bool TSF314::CheckGhostDisk(bool write) {
  if(!State.ghost) // need to open ghost image?
  {
    EasyStr STGPath=FloppyDrive[Id].GetImageFile();
    STGPath+=".STG"; 
    if(write || Exists(STGPath))
    {
      if(GhostDisk[Id].Open(STGPath.Text))
        State.ghost=1; 
    }
  }
  return State.ghost;
}

#endif


#ifdef SSE_DISK_STW

int TSF314::CyclesPerByte() {
  int cycles=CpuNormalHz; // per second  
  cycles/=rpm/60; // per rotation (300/60 = 5)
  cycles/=Disk[Id].TrackBytes; // per byte
  cycles_per_byte=cycles; // save
  return cycles;
}

#endif



DWORD TSF314::HblsAtIndex() { // absolute
  return (hbl_count/HblsPerRotation())*HblsPerRotation();
}


WORD TSF314::HblsNextIndex() { // relative
  return HblsPerRotation()-hbl_count%HblsPerRotation();
}


WORD TSF314::HblsPerRotation() {
  return HBL_PER_SECOND/(RPM/60); // see SSEParameters
}


WORD TSF314::HblsPerSector() {
  return nSectors()?(HblsPerRotation()-BytesToHbls(TrackGap()))/nSectors() : 0;
}


WORD TSF314::HblsToBytes(int hbls) {
  return TRACK_BYTES*hbls/HblsPerRotation();
}



#if defined(SSE_DRIVE_INDEX_PULSE)

/*  Function called by event manager in run.cpp
    Motor must be on, a floppy disk must be inside.
    For the moment it must be a STW image.
    If conditions are not met, we put timing of next check 1 second away,
    because it seems to be Steem's way. TODO: better way with less checking,
    though 1/sec is OK!
*/

void TSF314::IndexPulse() {
  if(!IMAGE_STW||FloppyDrive[Id].Empty()||!State.motor)
  {
    time_of_next_ip=ACT+n_cpu_cycles_per_second; // put into future
    return; 
  }
  Disk[Id].current_byte=0;
  State.reading=State.writing=0;
  time_of_last_ip=ACT; // record timing

  if(YM2149.Drive()==Id) // WD1772 gets no IP if drive not selected
    WD1772.OnIndexPulse();
  // No more IP if motor isn't on. The motor is turned off by WD1772.
  // (just above)
  if(State.motor)
      time_of_next_ip=time_of_last_ip + CyclesPerByte() * Disk[0].TrackBytes;
}


void TSF314::Motor(bool state) {
/*  If we're starting the motor, we must program time of next IP.
    For the moment, we use a random starting position.
    If we're stopping the motor, just having the variable cleared
    will stop Index Pulse being processed.
*/
  if(!State.motor && state)
    time_of_next_ip=ACT + (rand()%Disk[Id].TrackBytes) * CyclesPerByte();
  State.motor=state;
  TRACE_LOG("Drive %c motor %s\n",'A'+Id,state?"on":"off");
}

#endif


void TSF314::NextID(BYTE &RecordId,WORD &nHbls) {
/*  This routine was written to improve timing of command 'Read Address',
    used by ProCopy. Since it exists, it is also used for 'Verify'.
*/
  RecordId=0;
  nHbls=0;
  if(FloppyDrive[Id].Empty())
    return;

  WORD BytesToRun;
  WORD ByteOfNextId=BytePositionOfFirstId();//default
  WORD BytePositionOfLastId=ByteOfNextId+(nSectors()-1)*RecordLength();
  WORD CurrentByte=BytePosition(); 

  // still on this rev
  if(CurrentByte<ByteOfNextId) // before first ID
    BytesToRun=ByteOfNextId-CurrentByte;
  else if(CurrentByte<BytePositionOfLastId) // before last ID
  {
    while(CurrentByte>=ByteOfNextId)
      ByteOfNextId+=RecordLength();
    BytesToRun=ByteOfNextId-CurrentByte;
    RecordId=(ByteOfNextId-BytePositionOfFirstId())/RecordLength();
    ASSERT( RecordId>=1 && RecordId<nSectors() ); //<= ?
  }
  // next rev
  else
    BytesToRun=TRACK_BYTES-CurrentByte+ByteOfNextId;

  nHbls=BytesToHbls(BytesToRun);
}


BYTE TSF314::nSectors() { 
  // should we add IPF, STX? - here we do native
  BYTE nSects;
#if defined(SSE_YM2149)
  if(FloppyDrive[Id].STT_File)
  {
    FDC_IDField IDList[30]; // much work each time, but STT rare
#if defined(SSE_YM2149A)
    nSects=FloppyDrive[Id].GetIDFields(YM2149.SelectedSide,Track(),IDList);
#else
    nSects=FloppyDrive[Id].GetIDFields(YM2149.Side(),Track(),IDList);
#endif
  }
  else
#endif
    nSects=FloppyDrive[Id].SectorsPerTrack;
  return nSects;
}


BYTE TSF314::PostIndexGap() {
  return (nSectors()<11)? 60 : 10;
}


BYTE TSF314::PreDataGap() {
  int gap=0;
  switch(nSectors())
  {
  case 9:
  case 10: // with ID (7) and DAM (1)
    gap=12+3+7+22+12+3+1; 
    break;
  case 11:
    gap=3+3+7+22+12+3+1;
    break;
  }
  return gap;
}


BYTE TSF314::PostDataGap() {
  return (nSectors()<11)? 40 : 1;
}


WORD TSF314::PreIndexGap() {
  int gap=0;
  switch(nSectors())
  {
  case 9:
    gap=664;
    break;
  case 10:
    gap=50;
    break;
  case 11:
    gap=20;
    break;
  }
  return gap;
}


#if defined(SSE_DISK_STW)


void TSF314::Read() {
/*  We "sync" on data in the beginning of a sequence.
    After that, we do each successive byte until the command is
    finished. That way, we won't lose any byte.
*/
  ASSERT(!State.writing);
  ASSERT(IMAGE_STW); // only for those now

  if(!State.reading)
  {
    State.reading=true; 
    Disk[Id].current_byte=BytePosition();
    TRACE_LOG("Start reading at byte %d\n",Disk[Id].current_byte);
  }
  else // get next byte regardless of timing
    Disk[Id].current_byte++;

  if(IMAGE_STW)
    WD1772.Mfm.encoded=ImageSTW[Id].GetMfmData(Disk[Id].current_byte);

  // set up next byte event
  if(Disk[Id].current_byte<=Disk[Id].TrackBytes)
    WD1772.update_time=time_of_last_ip+cycles_per_byte*(Disk[Id].current_byte+1);

}

#endif


WORD TSF314::RecordLength() {
  return (nSectors()<11)? 614 : 566;
}


BYTE TSF314::SectorGap() {
  return PostDataGap()+PreDataGap();
}


#if defined(SSE_DRIVE_INDEX_STEP)

void TSF314::Step(int direction) {
  if(direction)
  {
    floppy_head_track[Id]++;
  }
  else
  {
    floppy_head_track[Id]--;
  }
  WD1772.Lines.track0=(floppy_head_track[Id]==0);
  if(WD1772.Lines.track0)
    WD1772.STR|=FDC_STR_T1_TRACK_0; // doing it here?
  CyclesPerByte();  // compute - should be the same
  TRACE_LOG("Drive %d Step d%d new track: %d\n",Id,direction,floppy_head_track[Id]);
}

#endif


BYTE TSF314::Track() {
  return floppy_head_track[Id]; //eh eh
}


WORD TSF314::TrackGap() {
  return PostIndexGap()+PreIndexGap();
}


#if defined(SSE_DISK_STW)

void TSF314::Write() {
/*  We "sync" on data in the beginning of a sequence.
    After that, we do each successive byte until the command is
    finished. That way, we won't lose any byte.
*/
  ASSERT(IMAGE_STW); // only for those now
  if(!State.writing)
  {
    if(State.reading)
      Disk[Id].current_byte++;
    else
      Disk[Id].current_byte=BytePosition();
    State.writing=true; 
    State.reading=false;
    TRACE_LOG("Start writing at byte %d\n",Disk[Id].current_byte);
  }
  else
    Disk[Id].current_byte++;

  if(IMAGE_STW)
    ImageSTW[Id].SetMfmData(Disk[Id].current_byte,WD1772.Mfm.encoded);

  // set up next byte event
  if(Disk[Id].current_byte<=Disk[Id].TrackBytes)
    WD1772.update_time=time_of_last_ip+cycles_per_byte*(Disk[Id].current_byte+1);
}

#endif



///////////////////////////////////////////////////////////////////////////////
#if defined(SSE_DRIVE_SOUND)
/*
    This is where we emulate the floppy drive sounds. v3.6
    DirectSound makes it rather easy, you just load your samples
    in secondary buffers and play as needed, one shot or loop,
    the mixing is done by the system.
    Each drive can have its own soundset, but for the moment it
    is shared.
*/

#include "../../../3rdparty/various/sound.h" //struct TWavFileFormat

#if defined(SSE_DRIVE_SOUND_EDIT) // my 1st attempt based on various sources

char* drive_sound_wav_files[]={ "drive_startup_ST_edit.wav",
"drive_spin_ST_edit.wav","drive_click_ST_edit.wav","drive_seek_edit.wav" };

#elif defined(SSE_DRIVE_SOUND_EPSON) // already better

char* drive_sound_wav_files[]={ "drive_startup_Epson.wav",
"drive_spin_Epson.wav","drive_click_Epson.wav","drive_seek_Epson.wav" };

#else // Amiga + my seek

char* drive_sound_wav_files[]={ "drive_startup.wav","drive_spin.wav",
"drive_click.wav","drive_seek.wav" };

#endif

#if defined(SSE_DRIVE_SOUND_VOLUME)

void TSF314::Sound_ChangeVolume() {
/*  Same volume for each buffer
*/
   for(int i=0;i<NSOUNDS;i++)
   {
     if(Sound_Buffer[i])
       Sound_Buffer[i]->SetVolume(Sound_Volume);
   }
}

#endif

void TSF314::Sound_CheckCommand(BYTE cr) {
/*  Called at each WD1772 command.
    If motor wasn't on we play the startup sound.
    We also play the (rattling!) SEEK noise.
*/

  if(!(fdc_str&0x80))
  {
    if(Sound_Buffer[START])
      Sound_Buffer[START]->Play(0,0,0);
  }

  if(
    Adat() &&
    ( (cr&(BIT_7+BIT_6+BIT_5+BIT_4))==0x00 
      && Track()>DRIVE_SOUND_BUZZ_THRESHOLD // RESTORE
    || (cr&(BIT_7+BIT_6+BIT_5+BIT_4))==0x10 
      &&abs(Track()-fdc_dr)>DRIVE_SOUND_BUZZ_THRESHOLD  ) // SEEK
    )
  {
    //TRACE("start seek loop from %d to %d\n",Track(),fdc_dr);

#if DRIVE_SOUND_BUZZ_THRESHOLD <5
    if(FloppyDrive[DRIVE].Empty())
      ; // wouldn't sound right
    else 
#endif
    if(Sound_Buffer[SEEK])
    {
      Sound_Buffer[SEEK]->Play(0,0,DSBPLAY_LOOPING);
      //TRACE("play seek sound\n");
    }
  }
}


void TSF314::Sound_CheckIrq() {
/*  Called at the end of each FDC command (native, pasti, caps, stw).
    Stop SEEK loop.
    Emit a "STEP" click noise if we were effectively seeking.
*/
#if !defined(SSE_DRIVE_SOUND_CHECK_SEEK_VBL)
  if(Sound_Buffer[SEEK])
  {
    Sound_Buffer[SEEK]->Stop();
//    TRACE("stop seek sound\n");
  }
#endif
#if defined(SSE_FDC)
  if(WD1772.CommandType()==1 && TrackAtCommand!=Track() && Sound_Buffer[STEP])
  {
    //TRACE("Step track %d\n",Track());
    DWORD dwStatus ;
    Sound_Buffer[STEP]->GetStatus(&dwStatus);
    if( (dwStatus&DSBSTATUS_PLAYING) )
      Sound_Buffer[STEP]->Stop();
    Sound_Buffer[STEP]->SetCurrentPosition(0);
    Sound_Buffer[STEP]->Play(0,0,0);
  }
#endif
}


void TSF314::Sound_CheckMotor() {
/*  Called at each emu VBL, start or stop playing motor sound loop if needed.
    For the moment we make no difference between empty drive (rare) or not.
*/
  if(!Sound_Buffer[MOTOR])
    return;

  DWORD dwStatus ;
  Sound_Buffer[MOTOR]->GetStatus(&dwStatus);
#if defined(SSE_DMA)
  Dma.UpdateRegs();//overkill
#endif
  bool motor_on= ((fdc_str&0x80)//;//simplification TODO?
#if defined(SSE_DRIVE_SOUND_EMPTY) // but clicks still on
    && !FloppyDrive[floppy_current_drive()].Empty()
#endif
    && floppy_current_drive()==YM2149.Drive() //3.6.4,must be selected
    );
  if(SSEOption.DriveSound && motor_on && !(dwStatus&DSBSTATUS_PLAYING))
    Sound_Buffer[MOTOR]->Play(0,0,DSBPLAY_LOOPING); // start motor loop
  else if((!SSEOption.DriveSound||!motor_on) && (dwStatus&DSBSTATUS_PLAYING))
    Sound_Buffer[MOTOR]->Stop();

#if defined(SSE_DRIVE_SOUND_CHECK_SEEK_VBL)//no, MFD
  // because of some buggy programs? (normally not defined)
  if(Sound_Buffer[SEEK] && !(FRAME%10) && WD1772.CommandType()!=1)
  {
    Sound_Buffer[SEEK]->GetStatus(&dwStatus);
    if((dwStatus&DSBSTATUS_PLAYING))
      Sound_Buffer[SEEK]->Stop();
  }
#endif

}


void TSF314::Sound_LoadSamples(IDirectSound *DSObj,DSBUFFERDESC *dsbd,WAVEFORMATEX *wfx) {
/*  Called from init_sound.cpp's DSCreateSoundBuf().
    We load each sample in its own secondary buffer, each time, which doesn't 
    seem optimal, but saves memory.
*/

  //TRACE("TSF314::Sound_LoadSamples\n");
  HRESULT Ret;
  TWavFileFormat WavFileFormat;
  
  FILE *fp;
  EasyStr path=GetEXEDir();
  path+=DRIVE_SOUND_DIRECTORY;
  path+="\\";
  EasyStr pathplusfile;
  for(int i=0;i<NSOUNDS;i++)
  {
    pathplusfile=path;
    pathplusfile+=drive_sound_wav_files[i];
    fp=fopen(pathplusfile.Text,"rb"); //rb!
    if(fp)
    {
      fread(&WavFileFormat,sizeof(TWavFileFormat),1,fp);
      wfx->nChannels=WavFileFormat.nChannels;
      wfx->nSamplesPerSec=WavFileFormat.nSamplesPerSec;
      wfx->wBitsPerSample=WavFileFormat.wBitsPerSample;
      wfx->nBlockAlign=wfx->nChannels*wfx->wBitsPerSample/8;
      wfx->nAvgBytesPerSec=WavFileFormat.nAvgBytesPerSec;
      dsbd->dwFlags|=DSBCAPS_STATIC ;
      dsbd->dwBufferBytes=WavFileFormat.length;
      Ret=DSObj->CreateSoundBuffer(dsbd,&Sound_Buffer[i],NULL);
      if(Ret==DS_OK)
      {
        LPVOID lpvAudioPtr1;
        DWORD dwAudioBytes1;
        Ret=Sound_Buffer[i]->Lock(0,0,&lpvAudioPtr1,&dwAudioBytes1,NULL,0,DSBLOCK_ENTIREBUFFER );
        if(Ret==DS_OK)
        {
          fread(lpvAudioPtr1,1,dwAudioBytes1,fp);
          //TRACE("load sample %s\n",pathplusfile.Text);
        }
        Ret=Sound_Buffer[i]->Unlock(lpvAudioPtr1,dwAudioBytes1,NULL,0);
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
        SF314[1].Sound_Buffer[i]=Sound_Buffer[i]; // not beautiful C++...
#endif
      }
      fclose(fp);
    }
#ifdef SSE_DEBUG
    else TRACE("DriveSound. Can't load sample file %s\n",pathplusfile.Text);
#endif
  }//nxt
#if defined(SSE_DRIVE_SOUND_VOLUME)
  Sound_ChangeVolume();
#endif
}


void TSF314::Sound_ReleaseBuffers() {
/* Called from init_sound.cpp's  DSReleaseAllBuffers(HRESULT Ret=DS_OK)
*/
  //TRACE("TSF314::Sound_ReleaseBuffers()\n");
  HRESULT Ret1,Ret2;
  for(int i=0;i<NSOUNDS;i++)
  {
    if(Sound_Buffer[i])
    {
      Ret1=Sound_Buffer[i]->Stop();
      Ret2=Sound_Buffer[i]->Release();
      Sound_Buffer[i]=NULL;
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
      SF314[1].Sound_Buffer[i]=NULL; // not beautiful C++...
#endif
      //TRACE("Release Buffer %d: %d %d\n",i,Ret1,Ret2);
    }
  }
}


void TSF314::Sound_StopBuffers() {
  //TRACE("TSF314::Sound_StopBuffers()\n");
  HRESULT Ret1;
  for(int i=0;i<NSOUNDS;i++)
  {
    if(Sound_Buffer[i])
    {
      Ret1=Sound_Buffer[i]->Stop();
      //TRACE("Stop Buffer %d: %d\n",i,Ret1);
    }
  }
}

#endif//sound

#endif//drive

#endif//seagal