#pragma once
#ifndef SSESHIFTER_H
#define SSESHIFTER_H

#include "SSEDecla.h"

#define LOGSECTION LOGSECTION_VIDEO

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

#pragma pack(push, 1)
struct TShifter {
  //DATA
#if defined(WIN32)
  BYTE *ScanlineBuffer;
#endif
#if defined(SSE_SHIFTER_STE_HI_HSCROLL)
  DWORD Scanline[230/4+2]; // TODO
#endif
#if defined(SSE_DEBUG)
  int nVbl;
#endif
#ifdef SSE_SHIFTER_HIRES_RASTER
  BYTE Scanline2[112+1]; // element 112 holds # raster bytes
#endif
#if defined(SSE_SHIFTER_HSCROLL_380)
  BYTE hscroll0; // what is/was HSCROLL at start of line
#endif
  BYTE HblStartingHscroll; // saving true hscroll in MED RES (no use)
  BYTE m_ShiftMode;
#if defined(SSE_SHIFTER_UNSTABLE)
  BYTE Preload; // #words into Shifter's SR (shifts display)
#endif
  char HblPixelShift; // for 4bit scrolling, other shifts

  //FUNCTIONS
  TShifter(); 
  ~TShifter();
#if defined(WIN32)
  inline void DrawBufferedScanlineToVideo();
#endif
  void DrawScanlineToEnd();
  void IncScanline();
  void Render(int cycles_since_hbl, int dispatcher=DISPATCHER_NONE);
  void Reset(bool Cold);
  inline void RoundCycles(int &cycles_in);
  void SetPal(int n, WORD NewPal);
  void Vbl();

};
#pragma pack(pop)

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