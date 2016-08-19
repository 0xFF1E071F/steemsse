#pragma once
#ifndef SSEGLUE_H
#define SSEGLUE_H

#include <run.decla.h>

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TScanline {
  DWORD Tricks; // see mask description in SSEVideo.h
  short StartCycle; // eg 56
  short EndCycle; // eg 376
  short Cycles; // eg 512 
  BYTE Bytes; // eg 160
};

#if defined(SSE_GLUE_FRAME_TIMINGS)

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

  // ENUM
  enum {FREQ_50,FREQ_60,FREQ_72,NFREQS};

  enum {GLU_DE_ON,HBLANK_OFF,GLU_DE_OFF,HBLANK_ON,HSYNC_ON,HSYNC_OFF,RELOAD_SDP,
    ENABLE_VBI,VERT_OVSCN_LIMIT,NTIMINGS};

  // DATA
  int TrickExecuted; //make sure that each trick will only be applied once
#if defined(SSE_GLUE_FRAME_TIMINGS)
  screen_event_struct screen_event;
  WORD scanline; 
#endif
  WORD DE_cycles[NFREQS];
  WORD ScanlineTiming[NTIMINGS][NFREQS];
#if defined(SSE_GLUE_383)
  bool HiRes; // one single line/bit/latch/whatever
  BYTE m_SyncMode;
#else
  BYTE m_ShiftMode,m_SyncMode;
#endif
  BYTE Freq[NFREQS];
  BYTE cycle_of_scanline_length_decision; 
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
  bool ExtraAdded;
#endif
  // we need keep info for only 3 scanlines:
  TScanline PreviousScanline, CurrentScanline, NextScanline;
#if defined(SSE_GLUE_FRAME_TIMINGS)
#ifdef UNIX
#undef Status // ?? ux382
#endif
  TGlueStatusBYTE Status;
#endif

  // FUNCTIONS
  TGlue();
  void AdaptScanlineValues(int CyclesIn); // on set sync of shift mode
  void AddFreqChange(int f);
  void AddShiftModeChange(int r);
  void CheckSideOverscan(); // left & right border effects
  void CheckVerticalOverscan(); // top & bottom borders
  void EndHBL();
  int FetchingLine();
  void IncScanline();
  void SetShiftMode(BYTE NewRes);
  void SetSyncMode(BYTE NewSync);
  void Update();
#if defined(SSE_GLUE_FRAME_TIMINGS)
  void GetNextScreenEvent();
  void Reset(bool Cold);
  void Vbl();
#endif
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
  void AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra);
#endif
  int FreqChangeAtCycle(int cycle);
  int FreqAtCycle(int cycle);
  int ShiftModeAtCycle(int cycle);
  int ShiftModeChangeAtCycle(int cycle);
#ifdef SSE_BOILER
  int NextFreqChange(int cycle,int value=-1);
  int PreviousFreqChange(int cycle);
#endif
  int NextShiftModeChange(int cycle,int value=-1); //move to shifter...
#if defined(SSE_GLUE_383B)
  int NextChangeToHi(int cycle);
  int NextChangeToLo(int cycle); // Lo = not HI for GLU
  int PreviousChangeToHi(int cycle);
  int PreviousChangeToLo(int cycle); // Lo = not HI for GLU
#endif
  int NextShiftModeChangeIdx(int cycle);
  int PreviousShiftModeChange(int cycle);
  int CycleOfLastChangeToShiftMode(int value);
};

#pragma pack(pop)

#endif//#ifndef SSEGLUE_H
