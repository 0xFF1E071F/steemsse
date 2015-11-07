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

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#define TRICK_LINE_PLUS_26 0x001
#define TRICK_LINE_PLUS_2 0x02
#define TRICK_LINE_MINUS_106 0x04
#define TRICK_LINE_MINUS_2 0x08
#define TRICK_LINE_PLUS_44 0x10
#define TRICK_4BIT_SCROLL 0x20
#define TRICK_OVERSCAN_MED_RES 0x40
#define TRICK_BLACK_LINE 0x80	
#define TRICK_TOP_OVERSCAN 0x100
#define TRICK_BOTTOM_OVERSCAN 0x200
#define TRICK_BOTTOM_OVERSCAN_60HZ 0x400
#define TRICK_LINE_PLUS_20 0x800	
#define TRICK_0BYTE_LINE 0x1000	
#define TRICK_STABILISER 0x2000
#define TRICK_WRITE_SDP 0x4000
#define TRICK_WRITE_SDP_POST_DE 0x8000
#if defined(SSE_SHIFTER_DRAGON1)
#define TRICK_CONFUSED_SHIFTER 0x10000//tmp hack
#endif
#if defined(SSE_SHIFTER_UNSTABLE)
#define TRICK_UNSTABLE 0x10000 // less specific
#endif
#define TRICK_LINE_PLUS_24 0x20000
#define TRICK_LINE_PLUS_4 0x40000
#define TRICK_LINE_PLUS_6 0x80000
#define TRICK_NEO 0x100000//tests



struct TScanline {
  short StartCycle; // eg 56
  short EndCycle; // eg 376
  BYTE Bytes; // eg 160 - TODO make sure it's always correct
  short Cycles; // eg 512 
  DWORD Tricks; // see mask description above
};
#endif

// Generalized Logic Unit
struct TGlue {
  TGlue();
  void Update();
  enum {FREQ_50,FREQ_60,FREQ_72,NFREQS};
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  enum {MMU_DE_ON,HBLANK_OFF,MMU_DE_OFF,HBLANK_ON,HSYNC_ON,HSYNC_OFF,RELOAD_SDP,
    ENABLE_VBI,VERT_OVSCN_LIMIT,NTIMINGS};
#elif defined(SSE_GLUE_THRESHOLDS)
  enum {MMU_DE_ON,HBLANK_OFF,MMU_DE_OFF,HBLANK_ON,HSYNC_ON,HSYNC_OFF,RELOAD_SDP,
    ENABLE_VBI,NTIMINGS};
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
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  BYTE m_ShiftMode,m_SyncMode;
  BYTE Freq[NFREQS];
  // we need keep info for only 3 scanlines 
  TScanline PreviousScanline, CurrentScanline, NextScanline;
  int TrickExecuted; //make sure that each trick will only be applied once
  BYTE cycle_of_scanline_length_decision; 
  bool ExtraAdded;//rather silly, should do without
  void AdaptScanlineValues(int CyclesIn); // on set sync of shift mode
  void AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra);
  int CheckFreq(int t);
  void CheckSideOverscan(); // left & right border effects
  void CheckVerticalOverscan(); // top & bottom borders
  void EndHBL(); // at end of HBL, check if +2 -2 were correct
  int FetchingLine();
  void IncScanline();
  void SetShiftMode(BYTE NewRes);
  void SetSyncMode(BYTE NewSync);
#if defined(SSE_SHIFTER_TRICKS) // some functions aren't used
  void AddFreqChange(int f);
  void AddShiftModeChange(int r);
  int CheckShiftMode(int t);
  int FreqChange(int idx);
  int ShiftModeChange(int idx=-1);
  int FreqChangeCycle(int idx);
  int ShiftModeChangeCycle(int idx);
  int FreqChangeIdx(int cycle);
  int ShiftModeChangeIdx(int cycle);
  int FreqChangeAtCycle(int cycle);
  int ShiftModeChangeAtCycle(int cycle);
  // value before this cycle (and a possible change)
  int FreqAtCycle(int cycle);
  int ShiftModeAtCycle(int cycle);
  // cycle of next change to whatever value after this cycle
  int NextFreqChange(int cycle,int value=-1);
  int NextShiftModeChange(int cycle,int value=-1);
  // idx of next change to whatever value after this cycle
  int NextFreqChangeIdx(int cycle);
  int NextShiftModeChangeIdx(int cycle);
  // cycle of previous change to whatever value before this cycle
  int PreviousFreqChange(int cycle);
  int PreviousShiftModeChange(int cycle);
  //   int PreviousFreqChange(int cycle,int value=-1);
  //   int PreviousShiftModeChange(int cycle,int value=-1);
  // idx of previous change to whatever value before this cycle
  int PreviousFreqChangeIdx(int cycle);
  int PreviousShiftModeChangeIdx(int cycle);
  int CycleOfLastChangeToFreq(int value);
  int CycleOfLastChangeToShiftMode(int value);
#endif
#endif//SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1
};

extern TGlue Glue;

#endif//#ifndef SSEGLUE_H
