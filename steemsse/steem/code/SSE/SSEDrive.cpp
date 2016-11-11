#include "SSE.h"

#if defined(SSE_DRIVE_OBJECT)
#include "../pch.h"
#include <cpu.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include <run.decla.h>
#include <gui.decla.h>
#include "SSECpu.h"
#include "SSEInterrupt.h"
#include "SSEOption.h"
//#include "SSEShifter.h"
#include "SSEVideo.h"

#if defined(WIN32)
#include <pasti/pasti.h>
#endif

#include "SSEDebug.h"
#include "SSEDrive.h"
#include "SSEFloppy.h"

#define LOGSECTION LOGSECTION_FDC

#if defined(SSE_VS2008)
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
  bool is_adat= (
    !floppy_instant_sector_access && ImageType.Manager==MNGR_STEEM
#if USE_PASTI
    || pasti_active && ImageType.Manager==MNGR_PASTI
#endif
#if defined(SSE_DISK_CAPS)
    || ImageType.Manager==MNGR_CAPS
#endif
#if defined(SSE_DISK_STW)
    ||ImageType.Manager==MNGR_WD1772
#if defined(SSE_DISK_STW_FAST) 
/*  To help our MFM disk image format, we finally add a fast mode for 
    STW (and HFE, since we test the image manager).
    It works with "normal" images (so most of them), but fails in cases 
    where floppy disk timing is more important, or if there's a READ TRACK
    or WRITE TRACK command:
    War Heli, MPS Golf, Jupiter's Masterdrive, Union Demo, Fantasia (megademo),
    Demoniak -ELT...
    Part of it is because our system is simplistic.
*/
      &&!floppy_instant_sector_access
#endif
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
    STGPath+=dot_ext(EXT_STG); 
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
  Sound_Volume=5000; 
#endif
#endif//sound
#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM)
  SectorChecksum=0;
#endif
  ImageType.Manager=MNGR_STEEM; //default
}


BYTE TSF314::Track() {
  return floppy_head_track[Id]; //eh eh
}



////////////////////////////////////// ADAT ///////////////////////////////////




WORD TSF314::BytesToHbls(int bytes) {
#if defined(SSE_FDC_383A)
  return HblsPerRotation()*bytes/Disk[Id].TrackBytes;
#else
  return HblsPerRotation()*bytes/TDisk::TRACK_BYTES;
#endif
  
  
}




DWORD TSF314::HblsAtIndex() { // absolute
  return (hbl_count/HblsPerRotation())*HblsPerRotation();
}


WORD TSF314::HblsNextIndex() { // relative
  return HblsPerRotation()-hbl_count%HblsPerRotation();
}


WORD TSF314::HblsPerRotation() {
  return HBL_PER_SECOND/(RPM/60); 
}





WORD TSF314::HblsToBytes(int hbls) {
#if defined(SSE_FDC_383A)
  return Disk[Id].TrackBytes*hbls/HblsPerRotation();
#else
  return TDisk::TRACK_BYTES*hbls/HblsPerRotation();
#endif
  
  
}




#if defined(SSE_WD1772)
/*  The WD1772 written for HFE, SCP, STW images uses events following
    a spinning drive.
    Drive events are index pulse (IP), reading or writing a byte.
*/

int TSF314::CyclesPerByte() { // normally 256
#if defined(SSE_INT_MFP_RATIO) 
  int cycles=CpuNormalHz; // per second  
#else
  int cycles=8000000; // per second  
#endif
  cycles/=RPM/60; // per rotation (300/60 = 5)
  cycles/=Disk[Id].TrackBytes; // per byte
#if defined(SSE_DISK_STW_FAST)
  if(!ADAT)
    cycles=SSE_STW_FAST_CYCLES_PER_BYTE; //hack!
#endif
  cycles_per_byte=cycles; // save
  ASSERT(cycles);
  return cycles;
}

/*  Function called by event manager in run.cpp based on preset timing
    or by the image object when we're  going through the last byte.
    Motor must be on, a floppy disk must be inside.
    The drive must be selected for the pulse to go to the WD1772.
    If conditions are not met, we put timing of next check 1 second away,
    because it seems to be Steem's way. TODO: better way with less checking,
    though 1/sec is OK!
*/

void TSF314::IndexPulse(bool image_triggered) {

  ASSERT(Id==0||Id==1);
  time_of_next_ip=time_of_next_event+n_cpu_cycles_per_second; 
  
  if(ImageType.Manager!=MNGR_WD1772||FloppyDrive[Id].Empty()||!State.motor)
    return; 

  time_of_last_ip=time_of_next_event; // record timing

  // Make sure that we always end track at the same byte when R/W
  // Important for Realm of the Trolls TODO: as for SCP
  if(!State.reading && !State.writing 
#if defined(SSE_DISK_SCP)
    || IMAGE_SCP
#endif
    || Disk[Id].current_byte>=Disk[Id].TrackBytes-1) // 0 - n-1
    Disk[Id].current_byte=0;

  // Program next event, at next IP or in 1 sec (more?)
  ASSERT(State.motor);

  /*  We set up a timing for next IP, but if the drive is reading there are 
  chances the SCP object will trigger IP itself at the end of the track.
  This is a place where we could have bug reports.
  */
  if(IMAGE_SCP // an image is inside
    && ImageSCP[DRIVE].track_header.TDH_TABLESTART[ImageSCP[DRIVE].rev].\
    TDH_DURATION) // a track is loaded... (else it's 0 and it hangs)
  {
    time_of_next_ip=time_of_last_ip 
      + ImageSCP[DRIVE].track_header.TDH_TABLESTART[ImageSCP[DRIVE].rev].\
      TDH_DURATION/5;
    //TRACE_LOG("SCP pos %d/%d next IP in %d cycles\n",ImageSCP[DRIVE].Position,ImageSCP[DRIVE].nBits-1,ImageSCP[DRIVE].track_header.TDH_TABLESTART[ImageSCP[DRIVE].rev].TDH_DURATION/5);
  }
  else
  {
    //TRACE("time_of_next_ip %d time_of_last_ip %d CyclesPerByte() %d Disk[Id].TrackBytes %d\n",time_of_next_ip,time_of_last_ip,CyclesPerByte(),Disk[Id].TrackBytes);
#if defined(SSE_DISK_STW_FAST)
    if(!ADAT)
      // make it longer or it may come before the WD1772 event
      time_of_next_ip=time_of_last_ip + 
      CyclesPerByte() * Disk[Id].TrackBytes *SSE_STW_FAST_IP_MULTIPLIER;
    else
#endif
      time_of_next_ip=time_of_last_ip + CyclesPerByte() * Disk[Id].TrackBytes;
  }

  ASSERT(time_of_next_ip-time_of_last_ip>0);

  //TRACE("%c: IP at %d next at %d (%d cycles, %d ms)\n",Id,time_of_last_ip,time_of_next_ip,time_of_next_ip-time_of_last_ip,(time_of_next_ip-time_of_last_ip)/(n_cpu_cycles_per_second/1000));

  // send pulse to WD1772
  if(DRIVE==Id)
#if defined(SSE_VS2008_WARNING_383) && !defined(SSE_DEBUG)
    WD1772.OnIndexPulse(image_triggered); // transmitting image_triggered
#else
    WD1772.OnIndexPulse(Id,image_triggered); // transmitting image_triggered
#endif
}


void TSF314::Motor(bool state) {
/*  If we're starting the motor, we must program time of next IP.
    We start from last position or from a new random one.
*/
 
#ifdef SSE_DEBUG
  if(state!=(bool)State.motor && FloppyDrive[Id].NotEmpty() )   //State.empty is never updated...
  {
    TRACE_LOG("Drive %c: motor %s\n",'A'+Id,state?"on":"off");
  }
#endif

  if(ImageType.Manager!=MNGR_WD1772)
    ;//TODO
  else if(State.motor && !state) //stopping - record position
    Disk[Id].current_byte=(BytePosition())%Disk[Id].TrackBytes; 
  else if(!State.motor && state) // starting
  {
    WORD bytes_to_next_ip= (Disk[Id].current_byte<Disk[Id].TrackBytes) 
      ? Disk[Id].TrackBytes-Disk[Id].current_byte : rand()%Disk[Id].TrackBytes;
    time_of_next_ip=ACT + bytes_to_next_ip * CyclesPerByte();
  }

#if defined(SSE_DRIVE_IP_HACK)
/*  Check time_of_next_ip, if there's none happening soon, stage one (hack)
    coded for Realm of the Trolls but I think SSE_YM2149C is the real
    fix; we keep it just in case there's another bug.
*/
  if(OPTION_HACKS && state && State.motor && ImageType.Manager==MNGR_WD1772
    && (time_of_next_ip-ACT<0 || time_of_next_ip-ACT> Disk[Id].TrackBytes*CyclesPerByte()))
  {
    int new_time_of_next_ip=ACT+(rand()%Disk[Id].TrackBytes) * CyclesPerByte();
    TRACE_LOG("ACT %d next IP %d diff %d %dms hack next IP %d\n",ACT,time_of_next_ip,time_of_next_ip-ACT,(time_of_next_ip-ACT)/8000,new_time_of_next_ip);
    //TRACE_OSD("IP BUG"); 
    time_of_next_ip=new_time_of_next_ip;
  }
#endif

  State.motor=state;

}


void TSF314::Read() {
/*  For Read() and Write():
    We "sync" on data in the beginning of a sequence.
    After that, we do each successive byte until the command is
    finished. That way, we won't lose any byte, and we add
    2 events/scanline only when reading.
*/
  ASSERT(!State.writing);
  ASSERT(IMAGE_STW || IMAGE_SCP || IMAGE_HFE); // only for those now
  ASSERT(Id==DRIVE);
#if defined(SSE_DISK2)

#if defined(SSE_DISK_380) && defined(SSE_DISK_STW)
  //it works but side could change again in the interval
  if(Disk[Id].current_side!=CURRENT_SIDE && (IMAGE_STW||IMAGE_HFE|IMAGE_SCP))
  {
    if(IMAGE_STW)
      ImageSTW[Id].LoadTrack(CURRENT_SIDE,Track());
#if defined(SSE_DISK_HFE)
    else if(IMAGE_HFE)
      ImageHFE[Id].LoadTrack(CURRENT_SIDE,Track());
#endif
#if defined(SSE_DISK_SCP)
    else if(IMAGE_SCP)
      ImageSCP[Id].LoadTrack(CURRENT_SIDE,Track());
#endif
  }
#endif

  //ASSERT(Disk[Id].current_side==CURRENT_SIDE);
//  ASSERT(Disk[Id].current_track==Track());
#endif

#if defined(SSE_DISK_SCP) || defined(SSE_DISK_HFE)
  bool new_position=!State.reading;
#endif

  if(!State.reading || Disk[Id].current_byte>=Disk[Id].TrackBytes-1)
  {
#if defined(SSE_DISK_SCP)
/*  We should refactor this so that also STW images trigger IP, but my
    first attempt didn't work at all so...
*/
    if(!State.reading || !(IMAGE_SCP))
      Disk[Id].current_byte=BytePosition();
    else
      Disk[Id].current_byte++;
    State.reading=true; 
#else
    State.reading=true; 
    Disk[Id].current_byte=BytePosition();
#endif
//    TRACE_LOG("Start reading at byte %d\n",Disk[Id].current_byte);
  }
  else // get next byte regardless of timing
    Disk[Id].current_byte++;
  if(IMAGE_STW)
    WD1772.Mfm.encoded=ImageSTW[Id].GetMfmData(Disk[Id].current_byte);
#if defined(SSE_DISK_HFE)
  else if(IMAGE_HFE)
    WD1772.Mfm.encoded=ImageHFE[Id].GetMfmData(
      new_position?Disk[Id].current_byte:0xffff);//ImageHFE[Id].GetMfmData(Disk[Id].current_byte);
#endif
#if defined(SSE_DISK_SCP)
  else if(IMAGE_SCP)//position kept in SCP manager?
    WD1772.Mfm.encoded=ImageSCP[Id].GetMfmData(
      new_position?Disk[Id].current_byte:0xffff);
#endif

#if defined(SSE_DRIVE_SINGLE_SIDE_RND)
  ASSERT(!(SSEOption.SingleSideDriveMap&(Id+1) && CURRENT_SIDE==1));
  if(SSEOption.SingleSideDriveMap&(Id+1) && CURRENT_SIDE==1)
    WD1772.Mfm.encoded=rand()%0xFFFF;
#endif

  // set up next byte event
#if defined(SSE_DISK_SCP)
  if(IMAGE_SCP)
    ; // it is done by SCP
  else
#endif
  if(Disk[Id].current_byte<=Disk[Id].TrackBytes)
  {
    WD1772.update_time=time_of_last_ip+cycles_per_byte*(Disk[Id].current_byte+1);
    //ASSERT(WD1772.update_time>ACT);
    if(WD1772.update_time-ACT<0)
      WD1772.update_time=ACT+cycles_per_byte;
  }
}


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
    WD1772.STR|=TWD1772::STR_T00; // doing it here?
  CyclesPerByte();  // compute - should be the same every track but...
  //TRACE_LOG("Drive %d Step d%d new track: %d\n",Id,direction,floppy_head_track[Id]);
}


void TSF314::Write() {
  ASSERT(IMAGE_STW||IMAGE_SCP||IMAGE_HFE); // only for those now
  ASSERT(Id==DRIVE);
#if defined(SSE_DISK2)
  ASSERT(Disk[Id].current_side==CURRENT_SIDE);
  ASSERT(Disk[Id].current_track==Track());
#endif

#if defined(SSE_DISK_SCP) || defined(SSE_DISK_HFE)
  bool new_position=!State.writing;
#endif

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
#if defined(SSE_DISK_HFE)
  else if(IMAGE_HFE)
    ImageHFE[Id].SetMfmData(new_position?Disk[Id].current_byte:0xffff,
      WD1772.Mfm.encoded);//Disk[Id].current_byte,WD1772.Mfm.encoded);
#endif
#if defined(SSE_DISK_SCP)
  else if(IMAGE_SCP)
    ImageSCP[Id].SetMfmData(new_position?Disk[Id].current_byte:0xffff,
      WD1772.Mfm.encoded);
#endif

  // set up next byte event
  if(Disk[Id].current_byte<=Disk[Id].TrackBytes)
    WD1772.update_time=time_of_last_ip+cycles_per_byte*(Disk[Id].current_byte+1);
  if(WD1772.update_time-ACT<0)
    WD1772.update_time=ACT+cycles_per_byte;
}

#endif//WD1772


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

#if defined(SSE_DRIVE_SOUND_EPSON) // already better

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

  Sound_Volume=min(Sound_Volume,10000);
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
      Sound_Buffer[SEEK]->Play(0,0,DSBPLAY_LOOPING);
  }
}


void TSF314::Sound_CheckIrq() {
/*  Called at the end of each FDC command (native, pasti, caps, stw).
    Stop SEEK loop.
    Emit a "STEP" click noise if we were effectively seeking.
    We don't come here if ADAT and SSE_DRIVE_SOUND_SEEK2.
*/
  if(Sound_Buffer[SEEK])
    Sound_Buffer[SEEK]->Stop();

#if defined(SSE_WD1772) 
  if(WD1772.CommandType()==1 && TrackAtCommand!=Track() && Sound_Buffer[STEP]
#if defined(SSE_DRIVE_SOUND_SEEK3)    
    && (!Adat()|| ImageType.Manager!=MNGR_STEEM && ImageType.Manager!=MNGR_WD1772)
#endif
    )
  {
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
#if defined(SSE_DMA_OBJECT)
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
}


void TSF314::Sound_LoadSamples(IDirectSound *DSObj,DSBUFFERDESC *dsbd,WAVEFORMATEX *wfx) {
/*  Called from init_sound.cpp's DSCreateSoundBuf().
    We load each sample in its own secondary buffer, each time, which doesn't 
    seem optimal, but saves memory.
*/

  HRESULT Ret;
  TWavFileFormat WavFileFormat;
  FILE *fp;
  EasyStr path=RunDir+SLASH+DRIVE_SOUND_DIRECTORY+SLASH;
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
          fread(lpvAudioPtr1,1,dwAudioBytes1,fp);
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
  HRESULT Ret1;
  for(int i=0;i<NSOUNDS;i++)
  {
    if(Sound_Buffer[i])
      Ret1=Sound_Buffer[i]->Stop();
  }
}

#endif//sound

#endif//drive


