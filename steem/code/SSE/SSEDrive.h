#pragma once
#ifndef SSEDRIVE_H
#define SSEDRIVE_H

#if defined(SSE_DRIVE_OBJECT)

#include "SSEParameters.h"
#include "SSEDisk.h"

#pragma pack(push, STRUCTURE_ALIGNMENT)

#if defined(SSE_DISK_MFM0) 
/*  Base class for TImageHFE +...
*/

struct  TImageMfm {
  // interface
  virtual WORD GetMfmData(WORD position)=0; 
  virtual bool LoadTrack(BYTE side,BYTE track,bool reload=false)=0;
  virtual void SetMfmData(WORD position,WORD mfm_data)=0;
  void ComputePosition(WORD position);
  void IncPosition();
  // other functions
  TImageMfm();
  void Init();
  // variables
  FILE *fCurrentImage;
  DWORD Position;
  BYTE Id; //0,1, same as drive
};

#endif//mfm

struct TSF314 {

  // ENUM
  enum {RPM=DRIVE_RPM,MAX_CYL=DRIVE_MAX_CYL};
#if defined(SSE_DRIVE_SOUND)
  enum {START,MOTOR,STEP,SEEK,NSOUNDS} ;
#endif
  // DATA
#if defined(SSE_DISK_MFM0) 
  TImageMfm *MfmManager;
#endif//mfm

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
  BYTE Id; // object has to know if its A: (0) or B: (1)
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
    unsigned int adat:1;
    //unsigned int :1;
  }State;
  // FUNCTIONS
  TSF314();
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
  void UpdateAdat();
#if defined(SSE_WD1772)
  int CyclesPerByte();
  void IndexPulse(bool image_triggered=false);
  void Motor(bool state);
  void Read();
  void Step(int direction);
  void Write();
#endif

#if defined(SSE_DRIVE_SOUND)
  void Sound_LoadSamples(IDirectSound *DSObj,DSBUFFERDESC *dsbd,WAVEFORMATEX *wfx);
  void Sound_ReleaseBuffers();
  void Sound_StopBuffers();
  void Sound_CheckCommand(BYTE cr);
  void Sound_CheckIrq();
  void Sound_CheckMotor();
  void Sound_ChangeVolume();
  void Sound_Step();
#endif//sound

};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#endif//SSE_DRIVE_OBJECT
#endif//#ifndef SSEDRIVE_H
