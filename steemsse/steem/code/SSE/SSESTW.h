#pragma once
#ifndef SSE_STW_H
#define SSE_STW_H

#ifdef SSE_UNIX
#include <conditions.h>
#endif

struct TImageSTW {

  // interface
  bool Open(char *path);
  void Close();
  bool Create(char *path);
  bool LoadTrack(BYTE side,BYTE track);
  WORD GetMfmData(WORD position); 
  void SetMfmData(WORD position,WORD mfm_data);
  // other functions
  TImageSTW();
  ~TImageSTW();
  void Init();
  // variables
  WORD Version;
#if !(defined(SSE_VAR_RESIZE_372) && defined(SSE_DISK1))
  WORD nBytes;
#endif
#if !defined(SSE_VAR_RESIZE_372)
  BYTE nSides;
  BYTE nTracks;
#endif
#if defined(SSE_DISK_STW2)
  BYTE Id; //0,1, same as drive
#endif
private: 
#if !defined(SSE_VAR_RESIZE_372) || defined(SSE_VAR_RESIZE_382)
  FILE *fCurrentImage; // use FloppyDrive's
#endif
  WORD *TrackData;
  BYTE *ImageData;
};


#endif//#ifndef SSE_STW_H
