#pragma once
#ifndef SSEHFE_H
#define SSEHFE_H

#include <conditions.h>
#include "SSEDecla.h" // typedefs used by hxc
#include <hfe/libhxcfe.h> //3rdparty
#include <hfe/hfe_format.h> //3rdparty


struct  TImageHFE {
  // interface (the same as for STW disk images)
  bool Open(char *path);
  void Close();
#if defined(SSE_DISK_HFE_DISK_MANAGER)
  bool Create(char *path);
#endif
  bool LoadTrack(BYTE side,BYTE track);
  WORD GetMfmData(WORD position); 
  void SetMfmData(WORD position,WORD mfm_data);
  // other functions
  TImageHFE();
  ~TImageHFE();
  int ComputeIndex();
  void ComputePosition(WORD position);
  inline void IncPosition();
  void Init();
  WORD MirrorMFM(WORD mfm_word);
  // variables
  BYTE Id; //0,1, same as drive
  WORD nBytes; // track data, not image
  picfileformatheader file_header;
  pictrack track_header[84]; // should be enough
  BYTE current_side;
  WORD Position;
private: 
  FILE *fCurrentImage;
  WORD *TrackData;
  BYTE *ImageData;
  int image_size;
};

#endif//#ifndef SSEHFE_H
