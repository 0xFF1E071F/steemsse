#pragma once
#ifndef SSEGLUE_H
#define SSEGLUE_H

struct TGlue {

  TGlue();
  void Update();

  enum {FREQ_50,FREQ_60,FREQ_72,NFREQS};
#if defined(SSE_SHIFTER_380)
  enum {MMU_DE_ON,HBLANK_OFF,MMU_DE_OFF,HBLANK_ON,HSYNC_ON,HSYNC_OFF,NTIMINGS};
#else //it wasn't used anyway
  enum {NEGATE_HSYNC,NEGATE_HBLANK,START_PREFETCH,STOP_PREFETCH,ASSERT_HBL,ASSERT_HSYNC,NTIMINGS};
#endif
  WORD DE_cycles[NFREQS];
  // cycles can be 0-512, hence words
  WORD ScanlineTiming[NTIMINGS][NFREQS];
  
};

extern TGlue Glue;

#endif//#ifndef SSEGLUE_H
