#pragma once
#ifndef SSEDRIVE_H
#define SSEDRIVE_H



#if defined(SSE_DRIVE)

#include "SSEParameters.h"

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
  void Init();
  enum {RPM=DRIVE_RPM,MAX_CYL=DRIVE_MAX_CYL,TRACK_BYTES=DRIVE_BYTES_ROTATION};
  bool Adat(); // accurate disk access times
  WORD BytePosition(); //this has to do with IP and rotation speed
  WORD BytePositionOfFirstId();
  WORD BytesToHbls(int bytes);
#if defined(SSE_DRIVE_RW_SECTOR_TIMING3)
  WORD BytesToID(BYTE &num,WORD &nHbls);// if num=0, next ID
#endif

#ifdef SSE_DISK_STW
  int CyclesPerByte();
  int cycles_per_byte;
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
#if !defined(SSE_DRIVE_STATE)
  BYTE motor_on;
#endif
  DWORD HblOfMotorOn;
#endif

#if defined(SSE_DRIVE_SOUND)
  enum {START,MOTOR,STEP,SEEK,NSOUNDS} ;
  BYTE TrackAtCommand;
#ifdef WIN32
  IDirectSoundBuffer *Sound_Buffer[NSOUNDS]; // fixed array
  void Sound_LoadSamples(IDirectSound *DSObj,DSBUFFERDESC *dsbd,WAVEFORMATEX *wfx);
#endif
  void Sound_ReleaseBuffers();
  void Sound_StopBuffers();
  void Sound_CheckCommand(BYTE cr);
  void Sound_CheckIrq();
  void Sound_CheckMotor();
#if defined(SSE_DRIVE_SOUND_VOLUME)
#if defined(SSE_DRIVE_SOUND_VOLUME_2)
  int Sound_Volume;
#else
  DWORD Sound_Volume;
#endif
  void Sound_ChangeVolume();
#endif

#if defined(SSE_DRIVE_SOUND_SEEK2)
  void Sound_Step();
#endif
#endif//sound

#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM)//debug only?
  WORD SectorChecksum;
#endif

#if defined(SSE_DRIVE_INDEX_PULSE)
  void IndexPulse();
  void Motor(bool state);
#endif


#if defined(SSE_DRIVE_STATE)
  struct  {
    unsigned int motor:1;
    unsigned int single_sided:1;
    unsigned int ghost:1;
    unsigned int empty:1; //TODO
    unsigned int reading:1;
    unsigned int writing:1;
    unsigned int :4;
  }State;
#endif

#if defined(SSE_DRIVE_INDEX_PULSE)
  WORD rpm; // default 300
  int time_of_next_ip;
  int time_of_last_ip;
#endif

#if defined(SSE_DISK_GHOST)
  bool CheckGhostDisk(bool write);
#endif

#if defined(SSE_DISK_STW)
  void Read();
  void Write();
#endif

#if defined(SSE_DRIVE_INDEX_STEP)
  void Step(int direction);
#endif




};

#endif//SSE_DRIVE
#endif//#ifndef SSEDRIVE_H
