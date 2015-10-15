#pragma once
#ifndef SSEGLUE_H
#define SSEGLUE_H

#include <run.decla.h>

#if defined(SSE_GLUE_FRAME_TIMINGS_A)

struct TGlueStatusBYTE { 
  // used to avoid doing tasks twice on repeat calls, a little cheat maybe
  unsigned int scanline_done:1;
  unsigned int sdp_reload_done:1;
  unsigned int vbl_done:1;
  unsigned int vbi_done:1;
  unsigned int hbi_done:1;
};

#endif


// Generalized Logic Unit
struct TGlue {

  TGlue();
  void Update();

  enum {FREQ_50,FREQ_60,FREQ_72,NFREQS};
#if defined(SSE_GLUE_THRESHOLDS)
  enum {MMU_DE_ON,HBLANK_OFF,MMU_DE_OFF,HBLANK_ON,HSYNC_ON,HSYNC_OFF,RELOAD_SDP,ENABLE_VBI,NTIMINGS};
#else //it wasn't used anyway
  enum {NEGATE_HSYNC,NEGATE_HBLANK,START_PREFETCH,STOP_PREFETCH,ASSERT_HBL,ASSERT_HSYNC,NTIMINGS};
#endif
  WORD DE_cycles[NFREQS];
  // cycles can be 0-512, hence words
  WORD ScanlineTiming[NTIMINGS][NFREQS];

#if defined(SSE_GLUE_FRAME_TIMINGS_A)
  TGlueStatusBYTE Status;
  screen_event_struct screen_event; // there's only one now
  WORD scanline; 
  void GetNextScreenEvent();
  void Reset(bool Cold);
  void Vbl();
#endif
  
};

extern TGlue Glue;

#endif//#ifndef SSEGLUE_H
