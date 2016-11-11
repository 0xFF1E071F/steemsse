#pragma once
#ifndef SSEDRIVE_H
#define SSEDRIVE_H

#if defined(SSE_DRIVE_OBJECT)

#include "SSEParameters.h"
#include "SSEDisk.h"

/* How a floppy disk is structured (bytes/track, gaps...) is handled
   here.
   TODO review, simplify...
   TODO separate native/pasti/etc... 
   TODO much to move to Disk
*/

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TSF314 {

  // ENUM
  enum {RPM=DRIVE_RPM,MAX_CYL=DRIVE_MAX_CYL};
#if defined(SSE_DRIVE_SOUND)
  enum {START,MOTOR,STEP,SEEK,NSOUNDS} ;
#endif
  // DATA
#if defined(SSE_DRIVE_SOUND)
#ifdef WIN32
  IDirectSoundBuffer *Sound_Buffer[NSOUNDS]; // fixed array
#endif
  int Sound_Volume;
#endif
  int cycles_per_byte;
#if defined(SSE_WD1772)
  int time_of_next_ip;
  int time_of_last_ip;
#endif
  TImageType ImageType; //WORD size, 3.6.2
#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM)//debug only
  WORD SectorChecksum;
#endif
  BYTE Id; // object has to know if its A: (0?) or B: (1?)
#if defined(SSE_DRIVE_SOUND)
  BYTE TrackAtCommand;
#endif
  struct  {
    unsigned int motor:1;
    unsigned int single_sided:1; // so it becomes a SF354!
    unsigned int ghost:1;
    unsigned int empty:1; //TODO
    unsigned int reading:1;
    unsigned int writing:1;
    unsigned int :2;
  }State;
  // FUNCTIONS
  TSF314();
  bool Adat(); // accurate disk access times
  WORD BytePosition(); //this has to do with IP and rotation speed

  WORD BytesToHbls(int bytes);

#if defined(SSE_DISK_GHOST)
  bool CheckGhostDisk(bool write);
#endif
  DWORD HblsAtIndex();
  WORD HblsNextIndex();
  WORD HblsPerRotation();

  WORD HblsToBytes(int hbls);
  void Init();

  BYTE Track();

#if defined(SSE_WD1772)
  int CyclesPerByte();
  void IndexPulse(bool image_triggered=false);
  void Motor(bool state);
  void Read();
  void Step(int direction);
  void Write();
#endif

#if defined(SSE_DRIVE_SOUND)
#ifdef WIN32
  void Sound_LoadSamples(IDirectSound *DSObj,DSBUFFERDESC *dsbd,WAVEFORMATEX *wfx);
#endif
  void Sound_ReleaseBuffers();
  void Sound_StopBuffers();
  void Sound_CheckCommand(BYTE cr);
  void Sound_CheckIrq();
  void Sound_CheckMotor();
#if defined(SSE_DRIVE_SOUND_VOLUME)
  void Sound_ChangeVolume();
#endif
#if defined(SSE_DRIVE_SOUND_SEEK2)
  void Sound_Step();
#endif
#endif//sound

};

#pragma pack(pop)

#endif//SSE_DRIVE_OBJECT
#endif//#ifndef SSEDRIVE_H
