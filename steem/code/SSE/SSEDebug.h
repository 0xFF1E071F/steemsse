#pragma once
#ifndef SSEDEBUG_H
#define SSEDEBUG_H
/*  This file must be included whether STEVEN_SEAGAL is defined or not
    because we need to define macors ASSERT, TRACE etc. anyway
*/
#if defined(SS_DEBUG) 

extern "C" int debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,
  debug9; // 4,5,6 cleared every VBL, 7,8,9 every HBL (too much?)

#if (!defined(_DEBUG)) && !defined(SS_DEBUG_TRACE_FILE) \
  && (defined(DEBUG_BUILD) || defined(SS_UNIX_TRACE))
#define SS_DEBUG_TRACE_FILE
#endif

#if defined(SS_DEBUG_TRACE_FILE)
#include <stdio.h>
#endif


#if defined(DEBUG_BUILD)
void debug_set_bk(unsigned long ad,bool set);
#define BREAK(ad) debug_set_bk(ad,true)
#else
#define BREAK(ad)
#endif

extern char trace_buffer[1024]; 

struct TDebug {
  TDebug();
  ~TDebug();
  
#if defined(SS_DEBUG_TRACE_FILE)
  void TraceToFile(char *fmt, ...); 
  FILE *trace_file_pointer; 
  BOOL ReportBreakpoints; // disabled when clicking cancel in the box
  int nTrace;
#endif
  int ShifterTricks;
};
extern TDebug Debug;


#if defined(SS_DEBUG_START_STOP_INFO)
enum EReportGeneralInfos {START,STOP} ;
int ReportGeneralInfos(int when);
#endif


#if defined(DEBUG_BUILD) // boiler
#if defined(_DEBUG) // IDE
#define TRACE_ENABLED (logsection_enabled_b[LOGSECTION] || LOGSECTION<NUM_LOGSECTIONS && logsection_enabled[LOGSECTION])
#else // no IDE
#define TRACE_ENABLED (logsection_enabled_b[LOGSECTION] || LOGSECTION<NUM_LOGSECTIONS && logsection_enabled[LOGSECTION])
//#define TRACE_ENABLED (LOGSECTION<NUM_LOGSECTIONS && logsection_enabled[LOGSECTION])
#endif
#else // no boiler 
#if defined(_DEBUG) // IDE
#define TRACE_ENABLED (logsection_enabled_b[LOGSECTION])
#else
impossible!
#endif
#endif

#define TRACE_LOG if(TRACE_ENABLED) TRACE  //very dangerous though, use {}

extern bool logsection_enabled_b[100];
#if !defined(DEBUG_BUILD)
  #define LOGSECTION_ALWAYS 0
  #define LOGSECTION_FDC 1
  #define LOGSECTION_IO 2
  #define LOGSECTION_MFP_TIMERS 3
  #define LOGSECTION_INIT 4
  #define LOGSECTION_CRASH 5
  #define LOGSECTION_STEMDOS 6
  #define LOGSECTION_IKBD 7
  #define LOGSECTION_AGENDA 8
  #define LOGSECTION_INTERRUPTS 9
  #define LOGSECTION_TRAP 10
  #define LOGSECTION_SOUND 11
  #define LOGSECTION_VIDEO 12
  #define LOGSECTION_BLITTER 13
  #define LOGSECTION_MIDI 14
  #define LOGSECTION_TRACE 15
  #define LOGSECTION_SHUTDOWN 16
  #define LOGSECTION_SPEEDLIMIT 17
  #define LOGSECTION_CPU 18
  #define LOGSECTION_INIFILE 19
  #define LOGSECTION_GUI 20
  #define LOGSECTION_DIV 21
  #define LOGSECTION_PASTI 22
  #define NUM_LOGSECTIONS 23
#endif

#if defined(_DEBUG) && defined(VC_BUILD) && !defined(SS_DEBUG_TRACE_FILE)

#define TRACE my_trace // to be independent of MFC

extern "C" void my_trace(char *fmt, ...);
#define ASSERT(x) {if(!(x) && !FullScreen) _asm{int 0x03}}
#define VERIFY(x) {if(!(x) && !FullScreen) _asm{int 0x03}} 
#define BREAKPOINT _asm { int 3 }
#define BRK(x) {TRACE(#x); TRACE("\n");}

#else // for boiler

#define TRACE Debug.TraceToFile
#define BREAKPOINT {if(Debug.ReportBreakpoints) \
Debug.ReportBreakpoints=(MessageBox(0,"Breakpoint! Click cancel to stop those \
boxes","BRK",MB_OKCANCEL)==IDOK);} 
#define ASSERT(x) {if (!(x)) {TRACE("Assert failed: %s\n",#x); \
  if(Debug.ReportBreakpoints) { \
  Debug.ReportBreakpoints=(MessageBox(0,#x,"ASSERT",MB_OKCANCEL)==IDOK);}}}
#define VERIFY(X) ASSERT(X)
#define BRK(x){if(Debug.ReportBreakpoints) { \
TRACE("Breakpoint: %s\n",#x); \
Debug.ReportBreakpoints=(MessageBox(0,#x,"Breakpoint",MB_OKCANCEL)==IDOK);}}

#endif

#else // release versions

#define BREAKPOINT 
#define VERIFY(x) x 

#if defined(SS_UNIX_TRACE____) //tmp
#define TRACE my_trace
void my_trace(char *fmt, ...);
#else

#if defined(VC_BUILD) // OK for Unix?
#define TRACE(x) 
#define TRACE_LOG(x) 
#else
#define TRACE
#define TRACE_LOG
#endif

#endif

#define ASSERT(x)
#define BRK(x) 

#endif 


// our additions to logsections, needed in all builds
enum {
  LOGSECTION_FDC_BYTES=23,
  LOGSECTION_IPF_LOCK_INFO,
  LOGSECTION_IMAGE_INFO, 
 };


#endif// SSEDEBUG_H