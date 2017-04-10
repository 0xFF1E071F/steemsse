#pragma once
#ifndef SSE_STW_H
#define SSE_STW_H
#include "SSE.H"
#include "SSEDrive.h"
#include "SSEParameters.h"
#ifdef SSE_UNIX
#include <conditions.h>
#endif

#pragma pack(push, STRUCTURE_ALIGNMENT)

#if defined(SSE_DISK_MFM0)

struct TImageSTW:public TImageMfm {

  // interface
  bool Open(char *path);
  void Close();
#if defined(SSE_GUI_DM_STW)
  bool Create(char *path);
#endif
  bool LoadTrack(BYTE side,BYTE track,bool reload=false);
  WORD GetMfmData(WORD position); 
  void SetMfmData(WORD position,WORD mfm_data);
  // other functions
  TImageSTW();
  ~TImageSTW();
  void Init();
  // variables
  WORD *TrackData;
  BYTE *ImageData;
  WORD Version;
private: 

};

#else

struct TImageSTW {

  // interface
  bool Open(char *path);
  void Close();
#if defined(SSE_GUI_DM_STW)
  bool Create(char *path);
#endif
  bool LoadTrack(BYTE side,BYTE track);
  WORD GetMfmData(WORD position); 
  void SetMfmData(WORD position,WORD mfm_data);
  // other functions
  TImageSTW();
  ~TImageSTW();
  void Init();
  // variables
  FILE *fCurrentImage; 
  WORD *TrackData;
  BYTE *ImageData;
  WORD Version;
  BYTE Id; //0,1, same as drive
private: 

};

#endif

#pragma pack(pop)

#endif//#ifndef SSE_STW_H
