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


struct TGlueStatusBYTE { 
  // necessary because GetNextScreenEvent() can be called an arbitrary
  // number of times at the same timing
  unsigned int scanline_done:1;
  unsigned int sdp_reload_done:1;
  unsigned int vbl_done:1;
  unsigned int vbi_done:1;
  unsigned int hbi_done:1;
  unsigned int :11;
};


// Generalized Logic Unit

struct TGlue {

  // ENUM
  enum {FREQ_50,FREQ_60,FREQ_72,NFREQS};

  enum {GLU_DE_ON,HBLANK_OFF,GLU_DE_OFF,HBLANK_ON,HSYNC_ON,HSYNC_OFF,RELOAD_SDP,
    ENABLE_VBI,VERT_OVSCN_LIMIT,NTIMINGS};

  // DATA
  int TrickExecuted; //make sure that each trick will only be applied once
  screen_event_struct screen_event;
  TScanline PreviousScanline, CurrentScanline, NextScanline;
  short VCount;
  WORD DE_cycles[NFREQS];
  WORD ScanlineTiming[NTIMINGS][NFREQS];
  BYTE m_ShiftMode,m_SyncMode; // both bits of shift mode are shadowed in GLU (ijor)
  BYTE Freq[NFREQS];
  BYTE cycle_of_scanline_length_decision; 
  // we need keep info for only 3 scanlines:

#ifdef UNIX
#undef Status // ?? ux382
#endif
  TGlueStatusBYTE Status;

  // FUNCTIONS
  TGlue();
  void AdaptScanlineValues(int CyclesIn); // on set sync of shift mode

  void CheckSideOverscan(); // left & right border effects
  void CheckVerticalOverscan(); // top & bottom borders
  void EndHBL();
  int FetchingLine();
  void IncScanline();
  void SetShiftMode(BYTE NewRes);
  void SetSyncMode(BYTE NewSync);
  void Update();
  void GetNextScreenEvent();
  void Reset(bool Cold);
  void Vbl();

#if defined(SSE_SHIFTER_TRICKS)
  void AddFreqChange(int f);
  void AddShiftModeChange(int r);  
  int FreqChangeAtCycle(int cycle);
  int FreqAtCycle(int cycle);
  int ShiftModeAtCycle(int cycle);
  int ShiftModeChangeAtCycle(int cycle);
#ifdef SSE_BOILER
  int NextFreqChange(int cycle,int value=-1);
  int PreviousFreqChange(int cycle);
#endif
  int NextShiftModeChange(int cycle,int value=-1); //move to shifter...
  int NextChangeToHi(int cycle);
  int NextChangeToLo(int cycle); // Lo = not HI for GLU
  int PreviousChangeToHi(int cycle);
  int PreviousChangeToLo(int cycle); // Lo = not HI for GLU
  int NextShiftModeChangeIdx(int cycle);
  int PreviousShiftModeChange(int cycle);
  int CycleOfLastChangeToShiftMode(int value);
#endif

};

#pragma pack(pop)

#endif//#ifndef SSEGLUE_H
