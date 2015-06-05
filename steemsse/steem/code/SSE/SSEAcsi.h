#pragma once
#ifndef SSEACSI_H
#define SSEACSI_H

#include <conditions.h>

struct TAcsiHdc {
  TAcsiHdc();
  ~TAcsiHdc();
  // interface
  bool Init(int num,char *path);
  BYTE IORead(BYTE Line);
  void IOWrite(BYTE Line,BYTE io_src_b);
  void Irq(bool state);
  void Reset(bool Cold);
  // other functions
  void CloseImageFile();
  void ReadWrite(bool write,BYTE block_count);
  int SectorNum();
  bool Seek();
#if defined(SSE_ACSI_FORMAT)
  void Format();
#endif
#if defined(SSE_ACSI_INQUIRY) 
  void Inquiry();
#endif
  // member variables
  FILE *hard_disk_image;
  BYTE device_num; //0-7
  BYTE cmd_block[6];
  BYTE cmd_ctr;
  BYTE STR,DR; //STR 0=OK
  BYTE error_code;
  BYTE Active;
  int nSectors; //total
#if defined(SSE_ACSI_TIMING)
  int time_of_irq;
#endif
};

extern TAcsiHdc AcsiHdc;

#endif//#ifndef SSEACSI_H