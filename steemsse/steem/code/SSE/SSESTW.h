#pragma once
#ifndef SSE_STW_H
#define SSE_STW_H

struct TImageSTW {
  // interface
  bool Open(char *path);
  void Close();
  bool Create(char *path);
  bool LoadTrack(BYTE side,BYTE track);
  WORD GetMfmData(WORD position); 
  void SetMfmData(WORD position, WORD mfm_data);
  // other functions
  TImageSTW();
  ~TImageSTW();
  void Init();
  // variables
  WORD Version;
  BYTE nSides;
  BYTE nTracks;
  WORD nBytes;
private: 
  FILE *fCurrentImage;
  WORD *TrackData;
  BYTE *ImageData;
};


#endif//#ifndef SSE_STW_H
