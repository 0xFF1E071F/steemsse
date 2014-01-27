/*  This file must be included whether STEVEN_SEAGAL is defined or not
    because we need to define macors ASSERT, TRACE etc. anyway, those
    aren't guarded.
    We use '__cplusplus' to make some parts accessible to C objects.
*/

#pragma once
#ifndef SSEDEBUG_H
#define SSEDEBUG_H

#include "SSE.h" // get switches

#if !defined(SS_DEBUG) && defined(UNIX)
#include "../pch.h"
#pragma hdrstop
#endif

#if defined(SS_STRUCTURE_SSEDEBUG_OBJ) && defined(__cplusplus)
#ifdef WIN32
#include <windows.h>
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

#endif//c++

  BYTE logsection_enabled[100]; // we want a double anyway //bool
  int LogSection;


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
 LOGSECTION_FDC_BYTES, // was DIV
 LOGSECTION_IMAGE_INFO, //was Pasti
 LOGSECTION_IPF_LOCK_INFO,
 LOGSECTION_OPTIONS,
 NUM_LOGSECTIONS,
 };
#endif

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
enum {
 LOGSECTION_FDC_BYTES, // was DIV
 LOGSECTION_IMAGE_INFO, //was Pasti
 LOGSECTION_IPF_LOCK_INFO,
 LOGSECTION_OPTIONS,
 NUM_LOGSECTIONS,
 };
#endif


 

#endif//#ifndef SSEDEBUG_H
