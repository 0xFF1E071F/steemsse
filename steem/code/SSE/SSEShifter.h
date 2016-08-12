#pragma once
#ifndef SSESHIFTER_H
#define SSESHIFTER_H

#include "SSEDecla.h"

#define LOGSECTION LOGSECTION_VIDEO

#include <cpu.decla.h>

#if defined(SSE_SHIFTER)
/*  The ST was a barebone machine made up of cheap components hastily patched
    together, including the video Shifter.
    A Shifter was needed to translate the video RAM into the picture.
    Such a system was chosen because RAM space was precious. Chunk modes are
    only possible with byte or more sizes.
*/
// register names in Atari doc / Steem variable
#define HSCROLL shifter_hscroll

#if defined(SSE_MMU_LINEWID_TIMING)
#define LINEWID MMU.Linewid0
#else
#define LINEWID shifter_fetch_extra_words
#endif

// ID which part of emulation required video rendering
enum {DISPATCHER_NONE, DISPATCHER_CPU, DISPATCHER_LINEWIDTH,
  DISPATCHER_WRITE_SDP, DISPATCHER_SET_SHIFT_MODE, DISPATCHER_SET_SYNC,
  DISPATCHER_SET_PAL, DISPATCHER_DSTE};

/*  The Shifter latency could be the time needed by the Shifter to treat
    data it's fed. 
    It is observed that palette changes occuring at some cycle apply at once,
    but on pixels that would be behind those that are currently being fetched
    (video counter).
    If we don't do that, emulation of Spectrum 512 pictures is not correct.
    There is a hack in Steem, delaying rendering by 28 cycles (16 fetching
    +12 treatment),and also a "magic value" (hack) of 7 (x4) used in Hatari 
    to shift lines with the same result.
    We try to rationalise this.
    Update: the delay is reduced after we corrected prefetch timing.
    This latency would be GLUE - MMU, since it impacts the timing of video RAM
    fetches (we renamed the constant MMU_PREFETCH_LATENCY instead of 
    MMU_PREFETCH_LATENCY).
    There's a further +4 latency in the Shifter before palette registers are 
    used. 
    8+16+4=28 cycles
    Current implementation: wake-up states are not directly integrated
    into this latency, nor in scanline start/stop cycles.
*/

#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA) && defined(SSE_CPU_PREFETCH_TIMING)
#define MMU_PREFETCH_LATENCY 8
#else
#define MMU_PREFETCH_LATENCY 12 
#endif

#define SHIFTER_RASTER_PREFETCH_TIMING 16//(shifter_freq==72 ? 4 : 16) 
#define SHIFTER_RASTER (shifter_freq==72? 2 : (screen_res ? 4 : 8)) 


/*  There's something wrong with the cycles when the line is 508 cycles,
    but fixing it will take some care. See the Omega hack
*/
#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
struct TScanline {
  int StartCycle; // eg 56
  int EndCycle; // eg 376
  int Bytes; // eg 160 - TODO make sure it's always correct
  int Cycles; // eg 512 
  int Tricks; // see mask description above
};
#endif

struct TShifter {
/*  As explained by ST-CNX and others, the video picture is produced by the
    MMU, the GLUE and the Shifter. 
    In our emulation, we pretend that the Shifter does most of it by itself:
    fetching memory, generating signals... It's a simplification. Otherwise
    we could imagine new objects: TMMU, TGlue...
*/
  TShifter(); 
  ~TShifter();
#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  inline void AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra);
  inline int CheckFreq(int t);
  void CheckSideOverscan(); // left & right border effects
  void CheckVerticalOverscan(); // top & bottom borders
  void EndHBL(); // at end of HBL, check if +2 -2 were correct
#endif
#if defined(WIN32)
  inline void DrawBufferedScanlineToVideo();
#endif
  void DrawScanlineToEnd();

#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  int FetchingLine();
#endif
  void IncScanline();
#if defined(SSE_SHIFTER_FIX_LINE508_CONFUSION)
  inline bool Line508Confusion();
#endif
  void Render(int cycles_since_hbl, int dispatcher=DISPATCHER_NONE);
  void Reset(bool Cold);
  inline void RoundCycles(int &cycles_in);
  void SetPal(int n, WORD NewPal);
#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  void SetShiftMode(BYTE NewRes);
  void SetSyncMode(BYTE NewSync);
#endif
#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_MMU1)
  inline void ShiftSDP(int shift);
#endif
  void Vbl();

#if defined(SSE_SHIFTER_TRICKS) && !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
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

#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_MMU1)
#if defined(SSE_SHIFTER_SDP)
#ifdef SSE_SHIFTER_SDP_READ
  inline MEM_ADDRESS ReadSDP(int cycles_since_hbl,int dispatcher=DISPATCHER_NONE);
#endif
#ifdef SSE_SHIFTER_SDP_WRITE
  void WriteSDP(MEM_ADDRESS addr, BYTE io_src_b);
  int SDPMiddleByte; // glue it! 
#endif 
#endif
#endif

#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  // we need keep info for only 3 scanlines 
  TScanline PreviousScanline, CurrentScanline, NextScanline;
  int ExtraAdded;//rather silly
#endif
  int HblStartingHscroll; // saving true hscroll in MED RES (no use)
  int HblPixelShift; // for 4bit scrolling, other shifts //BYTE?
#if defined(WIN32)
  BYTE *ScanlineBuffer;
#endif

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  BYTE m_ShiftMode; // Shifter has its copy, Glue could do with only 1 bit
#else
  BYTE m_ShiftMode,m_SyncMode;
#endif

#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  int TrickExecuted; //make sure that each trick will only be applied once
#endif
#if defined(SSE_SHIFTER_UNSTABLE)
  BYTE Preload; // #words into Shifter's RR (shifts display)
#endif
#if defined(SSE_SHIFTER_PANIC) || defined(SSE_SHIFTER_STE_HI_HSCROLL)
  DWORD Scanline[230/4+2]; // the price of fun
#endif

#ifdef SSE_SHIFTER_HIRES_RASTER
  BYTE Scanline2[112+1]; // element 112 holds # raster bytes
#endif

#if defined(SSE_TIMINGS_FRAME_ADJUSTMENT) // v3.6.4->v3.7.2
  // it's a hack, less involved than making a reliable statemachine
  WORD n508lines;
#endif
#if defined(SSE_SHIFTER_HSCROLL_380)
  BYTE hscroll0; // what is/was HSCROLL at start of line
#endif
#if defined(SSE_DEBUG)
  int nVbl;
#endif
};

extern TShifter Shifter; // singleton

// just taking some unimportant code out of Render for clarity

#define   AUTO_BORDER_ADJUST  \
          if(!(border & 1)) { \
            if(scanline_drawn_so_far<BORDER_SIDE) { \
              border1-=(BORDER_SIDE-scanline_drawn_so_far); \
              if(border1<0){ \
                picture+=border1; \
                if(!screen_res) {  \
                  hscroll-=border1;  \
                  shifter_draw_pointer+=(hscroll/16)*8; \
                  hscroll&=15; \
                }else if(screen_res==1) { \
                  hscroll-=border1*2;  \
                  shifter_draw_pointer+=(hscroll/16)*4; \
                  hscroll&=15; \
                } \
                border1=0; \
                if(picture<0) picture=0; \
              } \
            } \
            int ta=(border1+picture+border2)-320; \
            if(ta>0) { \
              border2-=ta; \
              if(border2<0)  { \
                picture+=border2; \
                border2=0; \
                if (picture<0)  picture=0; \
              } \
            } \
            border1=border2=0; \
          }
#endif//#if defined(SSE_SHIFTER)

#undef LOGSECTION 

#endif//define SSESHIFTER_H