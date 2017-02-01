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
#define LINEWID MMU.Linewid0 //lock it before rendering
#else
#define LINEWID shifter_fetch_extra_words 
#endif

// ID which part of emulation required video rendering (only a few used now)
enum EShifterDispatchers {DISPATCHER_NONE, DISPATCHER_CPU, DISPATCHER_LINEWIDTH,
  DISPATCHER_WRITE_SDP, DISPATCHER_SET_SHIFT_MODE, DISPATCHER_SET_SYNC,
  DISPATCHER_SET_PAL, DISPATCHER_DSTE};



// in all shift modes, all 4 Shifter registers are loaded before picture starts
#define SHIFTER_RASTER_PREFETCH_TIMING 16
#define SHIFTER_RASTER (shifter_freq==72? 2 : (screen_res ? 4 : 8)) 

//    There's a further +4 latency in the Shifter before palette registers are 
//    used. 
//    8+16+4=28 cycles

#pragma pack(push, STRUCTURE_ALIGNMENT)

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
#if defined(SSE_SHIFTER_HSCROLL)
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