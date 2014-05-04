#pragma once
#ifndef SSEDRIVE_H
#define SSEDRIVE_H

#if defined(SSE_DRIVE)

#if defined(SSE_DISK_IMAGETYPE)
#include "SSEDisk.h"
#endif

/* How a floppy disk is structured (bytes/track, gaps...) is handled
   here.
   TODO review, simplify...
   TODO separate native/pasti/etc... 
   TODO much to move to Disk
*/

struct TSF314 {
  TSF314();
  enum {RPM=DRIVE_RPM,MAX_CYL=DRIVE_MAX_CYL,TRACK_BYTES=DRIVE_BYTES_ROTATION};
  bool Adat(); // accurate disk access times
  WORD BytePosition(); //this has to do with IP and rotation speed
  WORD BytePositionOfFirstId();
  WORD BytesToHbls(int bytes);
#if defined(SSE_DRIVE_RW_SECTOR_TIMING3)
  WORD BytesToID(BYTE &num,WORD &nHbls);// if num=0, next ID
#endif
  DWORD HblsAtIndex();
  WORD HblsNextIndex();
  WORD HblsPerRotation();
  WORD HblsPerSector();
  WORD HblsToBytes(int hbls);
  BYTE nSectors();
  void NextID(BYTE &Id,WORD &nHbls);
  BYTE PostIndexGap();
  BYTE PreDataGap();
  BYTE PostDataGap();
  WORD PreIndexGap();
  WORD RecordLength();
  BYTE SectorGap();
  BYTE Track();
  WORD TrackBytes();
  WORD TrackGap();
  BYTE Id; // object has to know if its A: (0?) or B: (1?)

#if defined(SSE_DISK_IMAGETYPE)
  TImageType ImageType; //WORD size, 3.6.2
#elif defined(SSE_PASTI_ONLY_STX)
  WORD ImageType; //3.6.1, for future extension, snapshot problem?
#endif
#if defined(SSE_DRIVE_MOTOR_ON)
  BYTE MotorOn;
  DWORD HblOfMotorOn;
#endif

#if defined(SSE_DRIVE_SOUND)
  enum {START,MOTOR,STEP,SEEK,NSOUNDS} ;
  BYTE TrackAtCommand;
  IDirectSoundBuffer *Sound_Buffer[NSOUNDS]; // fixed array
  void Sound_LoadSamples(IDirectSound *DSObj,DSBUFFERDESC *dsbd,WAVEFORMATEX *wfx);
  void Sound_ReleaseBuffers();
  void Sound_StopBuffers();
  void Sound_CheckCommand(BYTE cr);
  void Sound_CheckIrq();
  void Sound_CheckMotor();
#if defined(SSE_DRIVE_SOUND_VOLUME)
  DWORD Sound_Volume;
  void Sound_ChangeVolume();
#endif
#endif//sound

#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM)//debug only?
  WORD SectorChecksum;
#endif
//not protected...
  struct  {
    unsigned int MotorOn:1;
    unsigned int SingleSided:1;
    unsigned int Ghost:1;
    unsigned int :5;
  }State;
  WORD Rpm; // default 300
/*
#if defined(SSE_DISK_GHOST)//TODO mask for that, motor, single sided, etc.
  BYTE Ghost; // flag 1 if ghost image is open
#endif
*/

};

#endif//SSE_DRIVE
#endif//#ifndef SSEDRIVE_H
