#pragma once
#ifndef SSEHFE_H
#define SSEHFE_H

#include <conditions.h>
#include "SSEDecla.h" // typedefs used by hxc
#include <hfe/libhxcfe.h> //3rdparty
#include <hfe/hfe_format.h> //3rdparty

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct  TImageHFE {
  // interface (the same as for STW disk images)
  bool Open(char *path);
  void Close();
#if defined(SSE_GUI_DM_HFE)
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
  void IncPosition();
  void Init();
  WORD MirrorMFM(WORD mfm_word);
  // variables
  FILE *fCurrentImage; // use FloppyDrive's
  WORD *TrackData;
  BYTE *ImageData;
  picfileformatheader *file_header;
  pictrack *track_header;
  int image_size;
  WORD Position;
  BYTE Id; //0,1, same as drive
};
#pragma pack(pop)

#endif//#ifndef SSEHFE_H
