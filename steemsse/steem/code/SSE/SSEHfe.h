#pragma once
#ifndef SSEHFE_H
#define SSEHFE_H

#include <conditions.h>
#include "SSEDecla.h" // typedefs used by hxc
#include <hfe/libhxcfe.h> //3rdparty
#include <hfe/hfe_format.h> //3rdparty

#pragma pack(push, 1)

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
#if defined(SSE_COMPILER_382)
  void IncPosition();
#else
  inline void IncPosition();
#endif
  void Init();
  WORD MirrorMFM(WORD mfm_word);
  // variables
#if !defined(SSE_VAR_RESIZE_372) || defined(SSE_VAR_RESIZE_382)
  FILE *fCurrentImage; // use FloppyDrive's
#endif
  WORD *TrackData;
  BYTE *ImageData;
#if defined(SSE_DISK_HFE_DYNAMIC_HEADER) //spare some memory
  picfileformatheader *file_header;
  pictrack *track_header;
#else
  picfileformatheader file_header;
  pictrack track_header[84]; // should be enough
#endif
  int image_size;
  WORD Position;
#if !(defined(SSE_VAR_RESIZE_372) && defined(SSE_DISK1))
  WORD nBytes; // track data, not image
#endif
  BYTE Id; //0,1, same as drive
#if !defined(SSE_DISK2)
  BYTE current_side;//in Disk
#endif
};
#pragma pack(pop)

#endif//#ifndef SSEHFE_H
