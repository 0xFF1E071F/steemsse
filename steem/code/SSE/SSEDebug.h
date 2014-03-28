/*  This file must be included whether STEVEN_SEAGAL is defined or not
    because we need to define macros ASSERT, TRACE etc. anyway, those
    aren't guarded.
    We use '__cplusplus' to make some parts accessible to C objects.
*/

#pragma once
#ifndef SSEDEBUG_H
#define SSEDEBUG_H

#include "SSE.h" // get switches

#if defined(UNIX)
#include "../pch.h"
#pragma hdrstop
#endif

#if defined(SS_STRUCTURE_SSEDEBUG_OBJ) && defined(__cplusplus)
#ifdef WIN32
#include <windows.h>
#endif
#ifdef UNIX
//typedef int KeyCode;
#endif
#include "../conditions.h"

#endif

#include "SSEParameters.h"

#if defined(SS_DEBUG_TRACE_FILE)
#include <stdio.h>
#endif

#ifdef SS_VS2012_WARNINGS
#pragma warning(disable : 4002)		// too many parameter in macro TRACE
#endif

#if defined(SS_DEBUG) // boiler build or ide debug build
// general use debug variables;
extern 
#ifdef __cplusplus
"C" 
#endif//c++
int debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;
#endif

#if defined(SS_DEBUG)

// a structure that may be used by C++ and C objects
struct TDebug {
#ifdef __cplusplus // visible only to C++ objects
  TDebug();
  ~TDebug();
  int ShifterTricks;
#if defined(SS_DEBUG_TRACE)
  enum {MAX_TRACE_CHARS=256}; // the function is safe anyway
  void Trace(char *fmt, ...); // one function for both IDE & file
  void TraceIde(char *fmt, ...); // in IDE, not file, whatever the defines
  // if logsection enabled, static for function pointer, used in '6301':
  static void TraceLog(char *fmt, ...); 
  char trace_buffer[MAX_TRACE_CHARS];
#endif
#endif//c++
#if defined(SS_DEBUG_TRACE_FILE)
  FILE *trace_file_pointer; 
  int nTrace;
#endif
  int IgnoreErrors; 


#ifdef __cplusplus // visible only to C++ objects

#if defined(SS_DEBUG_START_STOP_INFO)
  enum {START,STOP} ;
  void TraceGeneralInfos(int when);
#endif

#if defined(SS_OSD_DEBUG_MESSAGE)
  char m_OsdMessage[OSD_DEBUG_MESSAGE_LENGTH+1]; // +null as usual
  void TraceOsd(char *fmt, ...);
  unsigned long OsdTimer;
#endif

#if defined(SS_DEBUG_SHOW_INTERRUPT)
  enum {MAX_INTERRUPTS=256,DESCRIPTION_LENGTH=8};//
  struct SInterruptTable {
    char description[DESCRIPTION_LENGTH]; // eg MFP VBI TRACE...
    BYTE num; // eg 13 for timer A
  } InterruptTable[MAX_INTERRUPTS];

  short InterruptIdx;
  HWND InterruptReportingZone; // record static control handle in Boiler
  void ClickInterrupt();
  void RecordInterrupt(char *description, BYTE num=0);
  void ReportInterrupt();
  void Rte();
#endif

#if defined(SS_DEBUG_MUTE_PSG_CHANNEL)
  BYTE PsgMask; // bit0-2: mute a-c (if set)
#endif

#if defined(SS_DEBUG_TIMERS_ACTIVE)
  HWND boiler_timer_hwnd[4];//to record WIN handles
#endif

#if defined(SS_DEBUG_MONITOR_VALUE)
  BYTE MonitorValueSpecified; // boiler SSE option
  BYTE MonitorComparison; // as is, none found = 0 means no value to look for
  WORD MonitorValue;
#endif

#if defined(SS_DEBUG_MONITOR_RANGE)
  BYTE MonitorRange; //check from ad1 to ad2
#endif

#if defined(SS_DEBUG_68030_STACK_FRAME)
  BYTE M68030StackFrame;
#endif

#if defined(SS_DEBUG_BROWSER_6301)
  BYTE HD6301RamBuffer[256+8];
#endif

  void Vbl(); //3.6.1

#endif//c++

  BYTE logsection_enabled[100]; // we want a double anyway //bool
  int LogSection;

#if defined(SS_DEBUG_FAKE_IO)
/*  Hack. A free zone in IO is mapped to an array of masks to control 
    a lot of debug options using the Boiler's built-in features.
    Memory browsers display words so we use words even if bytes are
    handier (less GUI clutter).
*/
  WORD ControlMask[FAKE_IO_LENGTH];
#endif

#if defined(SS_DEBUG_FRAME_INTERRUPTS)
  BYTE FrameInterrupts; //bit0 VBI 1 HBI 2 MFP
  WORD FrameMfpIrqs; // for OSD report
#endif

};




extern struct TDebug Debug;

#endif//#if defined(SS_DEBUG)

#ifdef __cplusplus
#if defined(DEBUG_BUILD)
// to place Steem boiler breakpoints in the code
extern void debug_set_bk(unsigned long ad,bool set); //bool set
#define BREAK(ad) debug_set_bk(ad,true)
#else
#define BREAK(ad)
#endif
#endif//#ifdef __cplusplus

#if defined(SS_DEBUG_LOG_OPTIONS)
/* 
   We replace defines (that were in acc.h but also other places!) with an enum
   and we change some of the sections to better suit our needs.
   We use the same sections to control our traces in the boiler, menu log.
   So, Steem has a double log system!
   It came to be because:
   - unfamiliar with the log system
   - familiar with a TRACE system (like in MFC) that outputs in the IDE
   - the log system would choke the computer (each shifter trick written down!)
   This is unfinished business.
*/

#undef LOGSECTION_INIFILE
#undef LOGSECTION_GUI

enum logsection_enum_tag {
 LOGSECTION_ALWAYS,
 LOGSECTION_FDC ,
 LOGSECTION_IO ,
 LOGSECTION_MFP_TIMERS ,
 LOGSECTION_INIT ,
 LOGSECTION_CRASH ,
 LOGSECTION_STEMDOS ,
 LOGSECTION_IKBD ,
 LOGSECTION_AGENDA ,
 LOGSECTION_INTERRUPTS ,
 LOGSECTION_TRAP ,
 LOGSECTION_SOUND ,
 LOGSECTION_VIDEO ,
 LOGSECTION_BLITTER ,
 LOGSECTION_MIDI ,
 LOGSECTION_TRACE ,
 LOGSECTION_SHUTDOWN ,
 LOGSECTION_SPEEDLIMIT ,
 LOGSECTION_CPU ,
 LOGSECTION_INIFILE ,
 LOGSECTION_GUI ,
#if !defined(SS_DEBUG_TRACE_CONTROL)
 LOGSECTION_FDC_BYTES, // was DIV
 LOGSECTION_IPF_LOCK_INFO,
#endif
 LOGSECTION_IMAGE_INFO, //was Pasti
 LOGSECTION_OPTIONS,
 NUM_LOGSECTIONS,
 };
#endif

#if defined(SS_DEBUG_FAKE_IO)

#if defined(SS_OSD_CONTROL)

#define OSD_MASK1 (Debug.ControlMask[2])
#define OSD_CONTROL_CPUTRACE           (1<<15)
#define OSD_CONTROL_CPUPREFETCH                (1<<14)
#define OSD_CONTROL_INTERRUPT               (1<<13)
#define OSD_CONTROL_IKBD                  (1<<12)
#define OSD_CONTROL_60HZ              (1<<11)

#define OSD_MASK2 (Debug.ControlMask[3])
#define OSD_CONTROL_SHIFTERTRICKS           (1<<15)
#define OSD_CONTROL_PRELOAD (1<<14)

#define OSD_MASK3 (Debug.ControlMask[4])
#define OSD_CONTROL_DMASND                  (1<<15)
#define OSD_CONTROL_STEBLT                  (1<<14)
#define OSD_CONTROL_WRITESDP                (1<<13)

#endif//osdcontrol

#if defined(SS_DEBUG_TRACE_CONTROL)
/*  We use this to better control trace output, log section is still
    used.
    For example, log Video for shifter events, and trace control Vert
    for vertical overscan.
*/

#define TRACE_MASK1 (Debug.ControlMask[5]) //shifter
#define TRACE_CONTROL_VERTOVSC (1<<15)
#define TRACE_CONTROL_1LINE (1<<14) // report line of 'go'
#define TRACE_CONTROL_SUMMARY (1<<13) //all tricks of the frame orred
#define TRACE_CONTROL_LINEOFF (1<<12) //don't draw
#define TRACE_CONTROL_ADJUSTMENT (1<<11) //-2, +2 corrections

#define TRACE_MASK2 (Debug.ControlMask[6])
#define TRACE_CONTROL_MFP (1<<15)
#define TRACE_CONTROL_VBI (1<<14)
#define TRACE_CONTROL_HBI   (1<<13)

#define TRACE_MASK3 (Debug.ControlMask[7])
#define TRACE_CONTROL_FDCSTR (1<<15)
#define TRACE_CONTROL_FDCBYTES (1<<14)//no logsection needed
#define TRACE_CONTROL_FDCPSG (1<<13)//sectors
#define TRACE_CONTROL_FDCIPF1 (1<<13)//lock info
#define TRACE_CONTROL_FDCIPF2 (1<<11)//sectors


#endif

#if defined(SS_DEBUG_VIDEO_CONTROL)
#define VIDEO_CONTROL_MASK (Debug.ControlMask[9])
#define VIDEO_CONTROL_LINEOFF (1<<15)

#endif

#endif//#if defined(SS_DEBUG_FAKE_IO)

// debug macros

// ASSERT
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
#if defined(_DEBUG) && defined(VC_BUILD)
// Our ASSERT facility has no MFC dependency.
#define ASSERT(x) {if(!(x) && !FullScreen) _asm{int 0x03}}
#elif defined(SS_UNIX_TRACE)
#define ASSERT(x) if (!(x)) {TRACE("Assert failed: %s\n",#x);} 
#elif defined(DEBUG_BUILD) // for boiler
#ifdef __cplusplus
#define ASSERT(x) {if (!(x)) {TRACE("Assert failed: %s\n",#x); \
  if(!Debug.IgnoreErrors) { \
  debug9=MessageBox(0,#x,"ASSERT",MB_ICONWARNING|MB_ABORTRETRYIGNORE);   \
  if(debug9==IDABORT) exit(EXIT_FAILURE);\
  Debug.IgnoreErrors=(debug9==IDIGNORE);}}}
#endif//c++
#endif//vc
#else //!SS_DEBUG
#define ASSERT(x)
#endif

// BREAKPOINT 
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
#if defined(_DEBUG) && defined(VC_BUILD)
#define BREAKPOINT _asm { int 3 }
#elif defined(SS_UNIX_TRACE)
#define BREAKPOINT TRACE("BREAKPOINT\n"); // extremely silly, I know
#elif defined(DEBUG_BUILD) // for boiler
#ifdef __cplusplus
#define BREAKPOINT {if(!Debug.IgnoreErrors) { \
  TRACE("Breakpoint\n"); \
  Debug.IgnoreErrors=!(MessageBox(0,"no message","Breakpoint",MB_ICONWARNING|MB_OKCANCEL)==IDOK);}}
#endif//c++
#endif
#else //!SS_DEBUG
#define BREAKPOINT {}
#endif

// BRK(x) 
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)

#if defined(DEBUG_BUILD)

#ifdef __cplusplus
#define BRK(x){if(!Debug.IgnoreErrors) { \
  TRACE("Breakpoint: %s\n",#x); \
  Debug.IgnoreErrors=!(MessageBox(0,#x,"Breakpoint",MB_ICONWARNING|MB_OKCANCEL)==IDOK);}}
#endif//c++

#elif defined(SS_UNIX_TRACE)

#define BRK(x) TRACE("BRK %s\n",#x);

#elif defined(VC_BUILD)

#define BRK(x) {TRACE("BRK %s\n",#x); _asm { int 3 } }

#endif

#else //!SS_DEBUG

#define BRK(x)

#endif

// TRACE
#if defined(SS_DEBUG_TRACE)
#ifdef __cplusplus
#define TRACE Debug.Trace
#endif//c++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE(x) // no code left?
#else
#define TRACE // some code left to the compiler
#endif
#endif//#if defined(SS_DEBUG_TRACE)

// TRACE_ENABLED
#if defined(STEVEN_SEAGAL) && defined(DEBUG_BUILD) // boiler
#if defined(_DEBUG) // IDE
#define TRACE_ENABLED (Debug.logsection_enabled[LOGSECTION] || LOGSECTION<NUM_LOGSECTIONS && logsection_enabled[LOGSECTION])
#else // no IDE but boiler
#define TRACE_ENABLED (Debug.logsection_enabled[LOGSECTION] || LOGSECTION<NUM_LOGSECTIONS && logsection_enabled[LOGSECTION])
#endif
#else // no boiler 
#if defined(_DEBUG) // IDE
#define TRACE_ENABLED (Debug.logsection_enabled[LOGSECTION])
#else // 3rd party objects
#define TRACE_ENABLED (0)
#endif
#endif

// TRACE_IDE
#if defined(SS_DEBUG_TRACE)
#ifdef __cplusplus
#define TRACE_IDE Debug.TraceIde
#endif//c++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_IDE(x) // no code left?
#else
#define TRACE_IDE // some code left to the compiler
#endif
#endif//#if defined(SS_DEBUG_TRACE)


// TRACE_LOG
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_LOG Debug.LogSection=LOGSECTION, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_LOG(x) // no code left?
#else
#define TRACE_LOG // some code left to the compiler
#endif
#endif

// TRACE_OSD
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_OSD Debug.TraceOsd
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_OSD(x) // no code left? 
#else
#define TRACE_OSD // some code left to the compiler //?
#endif
#endif

// VERIFY
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
#if defined(_DEBUG) && defined(VC_BUILD)
// Our VERIFY facility has no MFC dependency.
#define VERIFY(x) {if(!(x) && !FullScreen) _asm{int 0x03}}
#elif defined(SS_UNIX_TRACE)
#define VERIFY(x) if (!(x)) {TRACE("Verify failed: %s\n",#x);} 
#elif defined(DEBUG_BUILD) // for boiler
#ifdef __cplusplus
#define VERIFY(x) {if (!(x)) {TRACE("Verify failed: %s\n",#x); \
  if(!Debug.IgnoreErrors) { \
  debug9=MessageBox(0,#x,"VERIFY",MB_ICONWARNING|MB_ABORTRETRYIGNORE);   \
  if(debug9==IDABORT) exit(EXIT_FAILURE);\
  Debug.IgnoreErrors=(debug9==IDIGNORE);}}}
#endif//C++
#endif
#else //!SS_DEBUG
#define VERIFY(x) x
#endif

#if !defined(STEVEN_SEAGAL) || !defined(SS_DEBUG) 
enum { // to pass compilation
 LOGSECTION_FDC_BYTES, // was DIV
 LOGSECTION_IMAGE_INFO, //was Pasti
 LOGSECTION_IPF_LOCK_INFO,
 LOGSECTION_OPTIONS,
 NUM_LOGSECTIONS,
 };
#endif


 

#endif//#ifndef SSEDEBUG_H
