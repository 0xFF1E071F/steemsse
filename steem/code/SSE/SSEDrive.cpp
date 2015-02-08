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
#include "SSEOption.h"
#include "SSEShifter.h"
#if defined(WIN32)
#include <pasti/pasti.h>
#endif
EasyStr GetEXEDir();//#include <mymisc.h>//missing...

#include "SSEDebug.h"
#include "SSEDrive.h"
#include "SSEFloppy.h"

#define LOGSECTION LOGSECTION_FDC

#if defined(STEVEN_SEAGAL) && defined(SSE_VS2008)
#pragma warning(disable : 4710)
#endif

TSF314::TSF314() {
  Init();
}


bool TSF314::Adat() {
/*  ADAT=accurate disk access times
    This is defined so: Steem slow (original ADAT) or Pasti or Caps 
    or Steem WD1772.
*/
  bool is_adat= (!floppy_instant_sector_access
#if defined(SSE_DISK_IMAGETYPE)
    && ImageType.Manager==MNGR_STEEM
#endif

#if USE_PASTI
    ||pasti_active
#endif
#if defined(SSE_DISK_IMAGETYPE)
    && ImageType.Manager==MNGR_PASTI
#endif

#if defined(SSE_IPF)
#if defined(SSE_DISK_IMAGETYPE1)
    || ImageType.Manager==MNGR_CAPS
#else
    ||Caps.IsIpf(Id)
#endif
#endif

#if defined(SSE_DISK_STW)
    ||ImageType.Manager==MNGR_WD1772
#endif
    );
  return is_adat;
}


WORD TSF314::BytePosition() {
  WORD position=0;
#ifdef SSE_DISK_STW
/*  This assumes constant bytes/track (some protected disks have more)
    This should be 0-6255
    This is independent of #sectors
    This is independent of disk type
    This is based on Index Pulse, that is sent by the drive 
*/
  if(ImageType.Manager==MNGR_WD1772 && CyclesPerByte())
  {
    position=(ACT-time_of_last_ip)/cycles_per_byte;
    if(position>=Disk[Id].TrackBytes)
    {
      // argh! IP didn't occur yet (Overdrive demo)
      position=Disk[Id].TrackBytes-(time_of_next_ip-ACT)/cycles_per_byte;
      time_of_last_ip=ACT;
      if(position>=Disk[Id].TrackBytes)
        position=0; //some safety
    }
  }
  else
#endif
    position=HblsToBytes( hbl_count% HblsPerRotation() );
  return position;

}

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


void TSF314::Init() {

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

#if defined(SSE_DISK_IMAGETYPE) 
  ImageType.Manager=MNGR_STEEM; //default
#endif

}


BYTE TSF314::Track() {
  return floppy_head_track[Id]; //eh eh
}



////////////////////////////////////// ADAT ///////////////////////////////////

WORD TSF314::BytePositionOfFirstId() { // with +7 for reading ID //no!
  return ( PostIndexGap() + ( (nSectors()<11)?12+3+1:3+3+1) );
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
    {
      bytes_to_id=TRACK_BYTES-current_byte+byte_target_id;
      TRACE_FDC("%d next rev current %d target %d diff %d to id %d\n",num,current_byte,byte_target_id,current_byte-byte_target_id,bytes_to_id);
    }
  }
  nHbls=BytesToHbls(bytes_to_id);
  return bytes_to_id;
}

#endif//#if defined(SSE_DRIVE_RW_SECTOR_TIMING3)


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


void TSF314::NextID(BYTE &RecordId,WORD &nHbls) {
/*  This routine was written to improve timing of command 'Read Address',
    used by ProCopy. Since it exists, it is also used for 'Verify'.
    In 'native' mode both.
    TODO rationalise between BytesToID() and NextID()
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
  // should we add IPF, STX? - here we do native - in IPF there's no nSectors,
  // in STX, it's indicative
  BYTE nSects;

  if(FloppyDrive[Id].STT_File)
  {
    FDC_IDField IDList[30]; // much work each time, but STT rare
    nSects=FloppyDrive[Id].GetIDFields(CURRENT_SIDE,Track(),IDList);
  }
  else
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
#if defined(SSE_DRIVE_REM_HACKS2)
    gap=664+6; // 6256 vs 6250
#else
    gap=664;
#endif
    break;
  case 10:
#if defined(SSE_DRIVE_REM_HACKS2)
    gap=50+6;
#else
    gap=50;
#endif
    break;
  case 11:
    gap=20;
    break;
  }
  return gap;
}


WORD TSF314::RecordLength() {
  return (nSectors()<11)? 614 : 566;
}


BYTE TSF314::SectorGap() {
  return PostDataGap()+PreDataGap();
}


WORD TSF314::TrackGap() {
  return PostIndexGap()+PreIndexGap();
}


///////////////////////////////////// STW /////////////////////////////////////

/*  Written to help handle STW disk images.
*/

#if defined(SSE_DISK_STW)


#ifdef SSE_DISK_STW

int TSF314::CyclesPerByte() {
//? TODO
#if defined(SSE_INT_MFP_RATIO) 
  int cycles=CpuNormalHz; // per second  
#else
  int cycles=8000000; // per second  
#endif


  cycles/=rpm/60; // per rotation (300/60 = 5)
  cycles/=Disk[Id].TrackBytes; // per byte
  cycles_per_byte=cycles; // save
  ASSERT( cycles==256 || shifter_freq!=50 );
  return cycles;
}

#endif


#if defined(SSE_DRIVE_INDEX_PULSE)

/*  Function called by event manager in run.cpp
    Motor must be on, a floppy disk must be inside.
    The drive must be selected for the pulse to go to the WD1772.
    If conditions are not met, we put timing of next check 1 second away,
    because it seems to be Steem's way. TODO: better way with less checking,
    though 1/sec is OK!
*/

void TSF314::IndexPulse() {

  //TRACE("Drive IP mngr %d drive %d empty: %d motor %d\n",ImageType.Manager,Id,FloppyDrive[Id].Empty(),State.motor);

  ASSERT(Id==0||Id==1);
  
  if(ImageType.Manager!=MNGR_WD1772||FloppyDrive[Id].Empty()||!State.motor)
  {
    time_of_next_ip=ACT+n_cpu_cycles_per_second; // put into future
    return; 
  }
#if defined(SSE_FLOPPY_EVENT2)
  time_of_last_ip=time_of_next_event; // record timing
#else
  time_of_last_ip=ACT; // record timing
#endif
  // Make sure that we always end track at the same byte when R/W
  // Important for Realm of the Trolls
  if(!State.reading && !State.writing 
    || Disk[Id].current_byte>=Disk[Id].TrackBytes-1) // 0 - n-1
    Disk[Id].current_byte=0;

  // Program next event, at next IP or in 1 sec (more?)
  if(State.motor)
    // try to maintain speed constant (ACT may be off by some cycles)
    //time_of_next_ip=time_of_next_ip + CyclesPerByte() * Disk[Id].TrackBytes;
    // accept some cycles off
    time_of_next_ip=time_of_last_ip + CyclesPerByte() * Disk[Id].TrackBytes;
  else //as a safety, or it could hang //TODO
    time_of_next_ip=ACT+n_cpu_cycles_per_second; // put into future

  // send pulse to WD1772
  if(DRIVE==Id)
    WD1772.OnIndexPulse(Id);

}


void TSF314::Motor(bool state) {
/*  If we're starting the motor, we must program time of next IP.
    For the moment, we use a random starting position.
    If we're stopping the motor, just having the variable cleared
    will stop Index Pulse being processed.
*/
 
#ifdef SSE_DEBUG
  if(state!=State.motor)
  {
    TRACE_LOG("Drive %c: motor %s\n",'A'+Id,state?"on":"off");
  }
#endif
  if(!State.motor && state)
    //time_of_next_ip=ACT + (rand()%Disk[Id].TrackBytes) * CyclesPerByte();
    time_of_next_ip=ACT + (Disk[Id].TrackBytes-(Disk[Id].current_byte%Disk[Id].TrackBytes)) * CyclesPerByte();
  State.motor=state;

}

#endif


#if defined(SSE_DRIVE_INDEX_STEP)


void TSF314::Step(int direction) {
  
#if defined(SSE_DRIVE_SOUND_SEEK2)
  if(SSEOption.DriveSound
#if defined(SSE_DRIVE_SOUND_SEEK5)
    && !DRIVE_SOUND_SEEK_SAMPLE
#endif
    )
    Sound_Step();
#endif

  if(direction && floppy_head_track[Id]<MAX_CYL)
  {
    floppy_head_track[Id]++;
  }
  else if(floppy_head_track[Id])
  {
    floppy_head_track[Id]--;
  }
  WD1772.Lines.track0=(floppy_head_track[Id]==0);
  if(WD1772.Lines.track0)
    WD1772.STR|=TWD1772::STR_T0; // doing it here?
  CyclesPerByte();  // compute - should be the same every track but...
  //TRACE_LOG("Drive %d Step d%d new track: %d\n",Id,direction,floppy_head_track[Id]);
}


#endif


void TSF314::Read() {
/*  For Read() and Write():
    We "sync" on data in the beginning of a sequence.
    After that, we do each successive byte until the command is
    finished. That way, we won't lose any byte, and we add
    2 events/scanline only when reading.

*/
  ASSERT(!State.writing);
  ASSERT(IMAGE_STW); // only for those now

  if(!State.reading || Disk[Id].current_byte>=Disk[Id].TrackBytes-1)
  {
    State.reading=true; 
    Disk[Id].current_byte=BytePosition();
//    TRACE_LOG("Start reading at byte %d\n",Disk[Id].current_byte);
  }
  else // get next byte regardless of timing
    Disk[Id].current_byte++;

  if(IMAGE_STW)
    WD1772.Mfm.encoded=ImageSTW[Id].GetMfmData(Disk[Id].current_byte);

#if defined(SSE_DRIVE_SINGLE_SIDE_RND)
  if(SSEOption.SingleSideDriveMap&(Id+1) && CURRENT_SIDE==1)
    WD1772.Mfm.encoded=rand()%0xFFFF;
#endif

  // set up next byte event
  if(Disk[Id].current_byte<=Disk[Id].TrackBytes)
  {
    WD1772.update_time=time_of_last_ip+cycles_per_byte*(Disk[Id].current_byte+1);
    //ASSERT(WD1772.update_time>ACT);
    if(WD1772.update_time-ACT<0)
      WD1772.update_time=ACT+cycles_per_byte;
  }
}


void TSF314::Write() {
  ASSERT(IMAGE_STW); // only for those now
  if(!State.writing || Disk[Id].current_byte>=Disk[Id].TrackBytes-1)
  {
    if(State.reading && Disk[Id].current_byte<Disk[Id].TrackBytes-1)
      Disk[Id].current_byte++;
    else
      Disk[Id].current_byte=BytePosition();
    State.writing=true; 
    State.reading=false;
//    TRACE_LOG("Start writing at byte %d\n",Disk[Id].current_byte);
  }
  else
    Disk[Id].current_byte++;

#if defined(SSE_DRIVE_SINGLE_SIDE_RND)
  if(SSEOption.SingleSideDriveMap&(Id+1) && CURRENT_SIDE==1)
    ; 
  else
#endif

  if(IMAGE_STW)
    ImageSTW[Id].SetMfmData(Disk[Id].current_byte,WD1772.Mfm.encoded);

  // set up next byte event
  if(Disk[Id].current_byte<=Disk[Id].TrackBytes)
    WD1772.update_time=time_of_last_ip+cycles_per_byte*(Disk[Id].current_byte+1);
  if(WD1772.update_time-ACT<0)
    WD1772.update_time=ACT+cycles_per_byte;
}


#endif


///////////////////////////////////// SOUND ///////////////////////////////////

#if defined(SSE_DRIVE_SOUND)
/*
    This is where we emulate the floppy drive sounds. v3.6
    Could be a separate cpp file.
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
"drive_spin_Epson.wav","drive_click_Epson.wav",
#if defined(SSE_DRIVE_SOUND_SEEK3)
"drive_seek_edit.wav"
#else
"drive_seek_Epson.wav"
#endif
 };

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
       Sound_Buffer[i]->SetVolume(
#if defined(SSE_DRIVE_SOUND_SEEK3)
       i==SEEK?Sound_Volume/2:
#endif
     Sound_Volume);
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

  if( Adat() &&

#if defined(SSE_DRIVE_SOUND_SEEK2) && !defined(SSE_DRIVE_SOUND_SEEK3) 
/*  Because we have no 'Step' callback, we use a loop to emulate
    the Seek noise with Pasti or Caps in charge.
*/
    (ImageType.Manager==MNGR_PASTI||ImageType.Manager==MNGR_CAPS
#if defined(SSE_DRIVE_SOUND_SEEK5)
      || DRIVE_SOUND_SEEK_SAMPLE
#endif
    )&&
#endif

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
      //Sound_Buffer[SEEK]
      //TRACE("play seek sound\n");
    }
  }
}


void TSF314::Sound_CheckIrq() {
/*  Called at the end of each FDC command (native, pasti, caps, stw).
    Stop SEEK loop.
    Emit a "STEP" click noise if we were effectively seeking.
    We don't come here if ADAT and SSE_DRIVE_SOUND_SEEK2.
*/
#if !defined(SSE_DRIVE_SOUND_CHECK_SEEK_VBL)
  if(Sound_Buffer[SEEK])
  {
    Sound_Buffer[SEEK]->Stop();
//    TRACE("stop seek sound\n");
  }
#endif
#if defined(SSE_FDC) 
  if(WD1772.CommandType()==1 && TrackAtCommand!=Track() && Sound_Buffer[STEP]
#if defined(SSE_DRIVE_SOUND_SEEK3)    
    && (!Adat()|| ImageType.Manager!=MNGR_STEEM && ImageType.Manager!=MNGR_WD1772)
#endif
    )
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


#if defined(SSE_DRIVE_SOUND_SEEK2)
/*  The idea is that Seek uses the same sample as Step.
    Maybe some motor noise is missing.
    It's possible to do this only when we are informed of each step.
    For the moment, only native and WD1772 emulations.
*/

void TSF314::Sound_Step() {
  if(!Sound_Buffer[STEP])
    return;

#if !defined(SSE_DRIVE_SOUND_SEEK4) //don't bother stopping
  DWORD dwStatus ;
  Sound_Buffer[STEP]->GetStatus(&dwStatus);
  if( (dwStatus&DSBSTATUS_PLAYING) )
    Sound_Buffer[STEP]->Stop();
#endif

  Sound_Buffer[STEP]->SetCurrentPosition(0);
  Sound_Buffer[STEP]->Play(0,0,0);
}

#endif


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
