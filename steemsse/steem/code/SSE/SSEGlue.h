#pragma once
#ifndef SSEGLUE_H
#define SSEGLUE_H

struct TGlue {

  TGlue();
  void Update();

  enum {FREQ_50,FREQ_60,FREQ_72,NFREQS};
  enum {NEGATE_HSYNC,NEGATE_HBLANK,START_PREFETCH,STOP_PREFETCH,ASSERT_HBL,ASSERT_HSYNC,NTIMINGS};
  WORD DE_cycles[NFREQS];
  // cycles can be 0-512, hence words
  WORD ScanlineTiming[NTIMINGS][NFREQS];
  
};

extern TGlue Glue;

#endif//#ifndef SSEGLUE_H
