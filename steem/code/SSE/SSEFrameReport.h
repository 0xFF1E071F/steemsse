#pragma once
#ifndef SSEFRAMEREPORT_H
#define SSEFRAMEREPORT_H

#if defined(SSE_BOILER_FRAME_REPORT)

#include <run.decla.h>

#include "SSEDebug.h"

#if defined(SSE_BOILER_FRAME_REPORT_MASK)
/*  We use the new general fake io control masks, they are 16bit, but 
    only 8 bits should be used for the GUI.
    Higher bits also for the GUI.
*/

//mask 1
#define FRAME_REPORT_MASK1 (Debug.ControlMask[0])
#define FRAME_REPORT_MASK_SYNCMODE                 (1<<15)
#define FRAME_REPORT_MASK_SHIFTMODE                (1<<14)
#define FRAME_REPORT_MASK_PAL                      (1<<13)
#define FRAME_REPORT_MASK_SDP_READ                 (1<<12)
#define FRAME_REPORT_MASK_SDP_WRITE                (1<<11)
#define FRAME_REPORT_MASK_SDP_LINES                (1<<10)
#define FRAME_REPORT_MASK_HSCROLL                  (1<<9)
#define FRAME_REPORT_MASK_VIDEOBASE                (1<<8)

//mask 2
#define FRAME_REPORT_MASK2 (Debug.ControlMask[1])
#define FRAME_REPORT_MASK_INT                      (1<<15) // for hbi, vbi, mfp
#define FRAME_REPORT_MASK_BLITTER                  (1<<14)
#define FRAME_REPORT_MASK_SHIFTER_TRICKS           (1<<13)
#define FRAME_REPORT_MASK_SHIFTER_TRICKS_BYTES     (1<<12)
#define FRAME_REPORT_MASK_TIMER_B                  (1<<11)
#endif

#pragma pack(push, STRUCTURE_ALIGNMENT)


struct SFrameEvent {
  int Scanline; 
  int Cycle;
  int Value;
#if defined(SSE_BOILER_FRAME_REPORT_392)
  int Type;
#else
  char Type;
#endif
  inline void Add(int scanline,int cycle, char type, int value); 
#if defined(SSE_BOILER_FRAME_REPORT_392)
  inline void Add(int scanline,int cycle, char *type, int value); //overload
#endif
};


class TFrameEvents {
private:
  int m_nEvents; // how many video events occurred this vbl?
  int m_nReports;
public:
  enum {MAX_EVENTS=210*32*2*2}; // too high?
  int TriggerReport; // set 2 to ask a full report, then it's set FALSE again
  struct SFrameEvent m_FrameEvent[MAX_EVENTS]; // it's public
  TFrameEvents();
  void Add(int scanline, int cycle, char type, int value);
#if defined(SSE_BOILER_FRAME_REPORT_392) // overload
  void Add(int scanline, int cycle, char *type, int value);
#endif
#if defined(SSE_BOILER_XBIOS2)
  DWORD GetShifterTricks(int y);
#endif
  void Init();
  int Report();
  void ReportLine();
  int Vbl(); 
  int nVbl;
#if defined(SSE_BOILER_REPORT_SDP_ON_CLICK)
  MEM_ADDRESS GetSDP(int guessed_x,int guessed_scan_y);
#endif
};

#pragma pack(pop, STRUCTURE_ALIGNMENT)


extern TFrameEvents FrameEvents; // singleton

#if defined(SSE_BOILER_FRAME_REPORT_392)

inline void SFrameEvent::Add(int scanline,int cycle, char type, int value) {
  Scanline=scanline;
  Cycle=cycle;
  Type=type;
  Value=value;
}

inline void SFrameEvent::Add(int scanline,int cycle, char *type, int value) { // overload
  Scanline=scanline;
  Cycle=cycle;
  Type=(type[0]<<8)+type[1];
  Value=value;
}


#else

inline void SFrameEvent::Add(int scanline,int cycle, char type, int value) {
  Scanline=scanline;
  Cycle=cycle;
  Type=type;
  Value=value;
}

#endif

#endif

#endif//#ifndef SSEFRAMEREPORT_H

