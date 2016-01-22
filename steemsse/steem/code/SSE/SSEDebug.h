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

#if defined(SSE_STRUCTURE_SSEDEBUG_OBJ) && defined(__cplusplus)
#ifdef WIN32
#include <windows.h>
#endif
#ifdef UNIX
//typedef int KeyCode;
#endif
#include "../conditions.h"

#endif

#include "SSEParameters.h"

#if defined(SSE_DEBUG_TRACE_FILE)
#include <stdio.h>
#endif

#if defined(SSE_DEBUG) // boiler build or ide debug build
// general use debug variables;
extern 
#ifdef __cplusplus
"C" 
#endif//c++
int debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;
#endif

#if defined(SSE_DEBUG)

// a structure that may be used by C++ and C objects

//todo, all C above

struct TDebug {
#if defined(SSE_DEBUG_TRACE_FILE)
  FILE *trace_file_pointer; 
  int nTrace;
#endif
  int IgnoreErrors; 
  BYTE logsection_enabled[100]; // we want a double anyway //bool
  int LogSection;


#ifdef __cplusplus // visible only to C++ objects
  TDebug();
  ~TDebug();
  int ShifterTricks;

#if defined(SSE_DEBUG_TRACE)
  enum {MAX_TRACE_CHARS=256}; // the function is safe anyway
  void Trace(char *fmt, ...); // one function for both IDE & file
  void TraceIde(char *fmt, ...); // in IDE, not file, whatever the defines
  // if logsection enabled, static for function pointer, used in '6301':
  static void TraceLog(char *fmt, ...); 
  char trace_buffer[MAX_TRACE_CHARS];
#endif
#endif//c++

#ifdef __cplusplus // visible only to C++ objects

#if defined(SSE_DEBUG_START_STOP_INFO)
  enum {START,STOP,INIT} ;
  void TraceGeneralInfos(int when);
#endif

#if defined(SSE_OSD_DEBUG_MESSAGE)
  char m_OsdMessage[OSD_DEBUG_MESSAGE_LENGTH+1]; // +null as usual
  void TraceOsd(char *fmt, ...);
  unsigned long OsdTimer;
#endif

#if defined(SSE_BOILER_SHOW_INTERRUPT)
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

#if defined(SSE_BOILER_TIMERS_ACTIVE)
  HWND boiler_timer_hwnd[4];//to record WIN handles
#endif

#if defined(SSE_BOILER_MONITOR_VALUE)
  BYTE MonitorValueSpecified; // boiler SSE option
  BYTE MonitorComparison; // as is, none found = 0 means no value to look for
  WORD MonitorValue;
#endif

#if defined(SSE_BOILER_MONITOR_RANGE)
  BYTE MonitorRange; //check from ad1 to ad2
#endif

#if defined(SSE_BOILER_68030_STACK_FRAME)
  BYTE M68030StackFrame;//flag
#endif

#if defined(SSE_BOILER_BROWSER_6301)
  BYTE HD6301RamBuffer[256+8];
#endif

#if defined(SSE_BOILER_STACK_CHOICE)
  BYTE StackDisplayUseOtherSp;//flag
#endif

  void Vbl(); //3.6.1
#if defined(SSE_DEBUG_RESET)
  void Reset(bool Cold);
#endif

#endif//c++

#if defined(SSE_BOILER_FAKE_IO)
/*  Hack. A free zone in IO is mapped to an array of masks to control 
    a lot of debug options using the Boiler's built-in features.
    Memory browsers display words so we use words even if bytes are
    handier (less GUI clutter).
*/
  WORD ControlMask[FAKE_IO_LENGTH];
#endif

#if defined(SSE_BOILER_FRAME_INTERRUPTS)
  BYTE FrameInterrupts; //bit0 VBI 1 HBI 2 MFP
  WORD FrameMfpIrqs; // for OSD report
#endif

  WORD nHbis; // counter for each frame

#if defined(SSE_BOILER_PSEUDO_STACK)
#define PSEUDO_STACK_ELEMENTS 64
  DWORD PseudoStack[PSEUDO_STACK_ELEMENTS]; // rotating stack
#ifdef __cplusplus
  void PseudoStackCheck(DWORD return_address);
  DWORD PseudoStackPop();
  void PseudoStackPush(DWORD return_address);
#endif
#endif

};


extern 
#ifdef __cplusplus
"C" 
#endif
struct TDebug Debug;

#endif//#if defined(SSE_DEBUG)

#ifdef __cplusplus
#if defined(DEBUG_BUILD)
// to place Steem boiler breakpoints in the code
extern void debug_set_bk(unsigned long ad,bool set); //bool set
#define BREAK(ad) debug_set_bk(ad,true)
#else
#define BREAK(ad)
#endif
#endif//#ifdef __cplusplus

#if defined(SSE_DEBUG_LOG_OPTIONS)
/* 
   We replace defines (that were in acc.h but also other places!) with an enum
   and we change some of the sections to better suit our needs.
   We use the same sections to control our traces in the boiler, menu log.
   So, Steem has a double log system!
   It came to be because:
   - unfamiliar with the log system
   - familiar with a TRACE system (like in MFC) that outputs in the IDE
   - the log system would choke the computer (each Shifter trick written down!)
   This is unfinished business.
*/

#undef LOGSECTION_INIFILE
#undef LOGSECTION_GUI

enum logsection_enum_tag {
 LOGSECTION_ALWAYS,
 LOGSECTION_FDC ,
 LOGSECTION_IO ,
 LOGSECTION_MFP_TIMERS ,
//#if defined(SSE_BOILER_TRACE_MFP1) //also in vc ide
 LOGSECTION_MFP=LOGSECTION_MFP_TIMERS,
//#endif
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
#if !defined(SSE_BOILER_NODIV)
 LOGSECTION_DIV,
#endif
#if !defined(SSE_BOILER_TRACE_CONTROL)
 LOGSECTION_FDC_BYTES, // was DIV
// LOGSECTION_IPF_LOCK_INFO,
#endif
 LOGSECTION_IMAGE_INFO, //was Pasti
 LOGSECTION_OPTIONS, // now free (we use INIT for options)
 NUM_LOGSECTIONS,
 };
#endif

#if defined(SSE_BOILER_FAKE_IO)

#if defined(SSE_OSD_CONTROL)

#define OSD_MASK1 (Debug.ControlMask[2])
#define OSD_CONTROL_INTERRUPT               (1<<15)
#define OSD_CONTROL_IKBD                  (1<<14)
#define OSD_CONTROL_IACK              (1<<13)
#define OSD_CONTROL_FDC              (1<<12)

#define OSD_MASK_CPU (Debug.ControlMask[3])
#define OSD_CONTROL_CPUTRACE           (1<<15)
#define OSD_CONTROL_CPUBOMBS  (1<<14)
#define OSD_CONTROL_CPUIO  (1<<13)
#define OSD_CONTROL_CPURESET            (1<<12)
#define OSD_CONTROL_CPUPREFETCH   (1<<11)    
#define OSD_CONTROL_CPUROUNDING   (1<<10)

#define OSD_MASK2 (Debug.ControlMask[4])
#define OSD_CONTROL_SHIFTERTRICKS           (1<<15)
#define OSD_CONTROL_PRELOAD (1<<14)
#define OSD_CONTROL_60HZ              (1<<13)

#define OSD_MASK3 (Debug.ControlMask[5])
#define OSD_CONTROL_DMASND                  (1<<15)
#define OSD_CONTROL_STEBLT                  (1<<14)
#define OSD_CONTROL_WRITESDP                (1<<13)

#endif//osdcontrol

#if defined(SSE_BOILER_TRACE_CONTROL)
/*  We use this to better control trace output, log section is still
    used.
    For example, log Video for Shifter events, and trace control Vert
    for vertical overscan.
*/

#define TRACE_MASK1 (Debug.ControlMask[6]) //Shifter
#define TRACE_CONTROL_VERTOVSC (1<<15)
#define TRACE_CONTROL_1LINE (1<<14) // report line of 'go'
#define TRACE_CONTROL_SUMMARY (1<<13) //all tricks of the frame orred
#define TRACE_CONTROL_LINEOFF (1<<12) //don't draw
#define TRACE_CONTROL_ADJUSTMENT (1<<11) //-2, +2 corrections
#define TRACE_CONTROL_0BYTE (1<<10) //


#define TRACE_MASK2 (Debug.ControlMask[7])
#define TRACE_CONTROL_ECLOCK (1<<15)
#define TRACE_CONTROL_RTE (1<<14)
//#define TRACE_CONTROL_HBI   (1<<13)


#define TRACE_MASK3 (Debug.ControlMask[8])
#define TRACE_CONTROL_FDCSTR (1<<15)
#define TRACE_CONTROL_FDCBYTES (1<<14)//no logsection needed
#define TRACE_CONTROL_FDCPSG (1<<13)//drive/side
#define TRACE_CONTROL_FDCREGS (1<<12)// writes to registers CR,TR,SR,DR

#if SSE_VERSION>370
#define TRACE_CONTROL_FDCMFM (1<<11)
#define TRACE_CONTROL_FDCDMA (1<<10)
#else
#define TRACE_CONTROL_FDCIPF1 (1<<11)//lock info
#define TRACE_CONTROL_FDCIPF2 (1<<10)//sectors
#endif

#define TRACE_MASK4 (Debug.ControlMask[13]) //cpu
#define TRACE_CONTROL_CPU_REGISTERS (1<<15) 
#define TRACE_CONTROL_CPU_REGISTERS_VAL (1<<14)  // values of (Ai)
#define TRACE_CONTROL_CPU_SP (1<<13) 
#define TRACE_CONTROL_CPU_CYCLES (1<<12) 

#if SSE_VERSION>=380

#define TRACE_MASK_14 (Debug.ControlMask[14]) //Shifter 2
#define TRACE_CONTROL_LINE_PLUS_2 (1<<15) 

#endif

#endif

#if defined(SSE_BOILER_VIDEO_CONTROL)
#define VIDEO_CONTROL_MASK (Debug.ControlMask[9])
#define VIDEO_CONTROL_LINEOFF (1<<15)
#define VIDEO_CONTROL_RES (1<<14)
#endif

#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS)
#define SOUND_CONTROL_MASK (Debug.ControlMask[10])
#define SOUND_CONTROL_OSD (1<<9)//first entries other variables
#endif

#if defined(SSE_BOILER_NEXT_PRG_RUN)
#define BOILER_CONTROL_MASK1 (Debug.ControlMask[11])
#define BOILER_CONTROL_NEXT_PRG_RUN (1<<15)
#if defined(SSE_BOILER_VBL_HBL)
#define BOILER_CONTROL_VBL_PENDING (1<<14)
#define BOILER_CONTROL_HBL_PENDING (1<<13)
#endif
#endif

#endif//#if defined(SSE_BOILER_FAKE_IO)

// debug macros

// ASSERT
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#if defined(_DEBUG) && defined(VC_BUILD)
// Our ASSERT facility has no MFC dependency.
#define ASSERT(x) {if(!(x) && !FullScreen) _asm{int 0x03}}
#elif defined(SSE_UNIX_TRACE)
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
#else //!SSE_DEBUG
#define ASSERT(x)
#endif

// BREAKPOINT 
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#if defined(_DEBUG) && defined(VC_BUILD)
#define BREAKPOINT _asm { int 3 }
#elif defined(SSE_UNIX_TRACE)
#define BREAKPOINT TRACE("BREAKPOINT\n"); // extremely silly, I know
#elif defined(DEBUG_BUILD) // for boiler
#ifdef __cplusplus
#define BREAKPOINT {if(!Debug.IgnoreErrors) { \
  TRACE("Breakpoint\n"); \
  Debug.IgnoreErrors=!(MessageBox(0,"no message","Breakpoint",MB_ICONWARNING|MB_OKCANCEL)==IDOK);}}
#endif//c++
#endif
#else //!SSE_DEBUG
#define BREAKPOINT {}
#endif

// BRK(x) 
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)

#if defined(DEBUG_BUILD)

#ifdef __cplusplus
#define BRK(x){if(!Debug.IgnoreErrors) { \
  TRACE("Breakpoint: %s\n",#x); \
  Debug.IgnoreErrors=!(MessageBox(0,#x,"Breakpoint",MB_ICONWARNING|MB_OKCANCEL)==IDOK);}}
#endif//c++

#elif defined(SSE_UNIX_TRACE)

#define BRK(x) TRACE("BRK %s\n",#x);

#elif defined(VC_BUILD)

#define BRK(x) {TRACE("BRK %s\n",#x); _asm { int 3 } }

#endif

#else //!SSE_DEBUG

#define BRK(x)

#endif

// TRACE
#if defined(SSE_DEBUG_TRACE)
#ifdef __cplusplus
#define TRACE Debug.Trace
#endif//c++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE(x) // no code left?
#else
#define TRACE // some code left to the compiler
#endif
#endif//#if defined(SSE_DEBUG_TRACE)

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
#if defined(SSE_DEBUG_TRACE)
#ifdef __cplusplus
#define TRACE_IDE Debug.TraceIde
#endif//c++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_IDE(x) // no code left?
#else
#define TRACE_IDE // some code left to the compiler
#endif
#endif//#if defined(SSE_DEBUG_TRACE)


// TRACE_LOG
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_LOG Debug.LogSection=LOGSECTION, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_LOG(x) // no code left?
#pragma warning(disable : 4002)
#else
#define TRACE_LOG // some code left to the compiler
#endif
#endif

// v3.6.3 introducing more traces,  verbose here, short in code
// TRACE_FDC
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_FDC Debug.LogSection=LOGSECTION_FDC, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_FDC(x) // no code left?
#else
#define TRACE_FDC // some code left to the compiler
#endif
#endif

#define TRACE_HDC TRACE_FDC //3.7.2

// TRACE_INIT 3.7.0
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_INIT Debug.LogSection=LOGSECTION_INIT, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_INIT(x) // no code left?
#else
#define TRACE_INIT // some code left to the compiler
#endif
#endif

// TRACE_INT 3.7.0
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_INT Debug.LogSection=LOGSECTION_INTERRUPTS, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_INT(x) // no code left?
#else
#define TRACE_INT // some code left to the compiler
#endif
#endif

// TRACE_MFP 3.7.0
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_MFP Debug.LogSection=LOGSECTION_MFP, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_MFP(x) // no code left?
#else
#define TRACE_MFP // some code left to the compiler
#endif
#endif

// TRACE_MFM 3.7.1
#if defined(STEVEN_SEAGAL) && defined(SSE_BOILER_TRACE_CONTROL) && SSE_VERSION>=371
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_MFM if(TRACE_MASK3&TRACE_CONTROL_FDCMFM) Debug.LogSection=LOGSECTION_FDC, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_MFM(x) // no code left?
#else
#define TRACE_MFM // some code left to the compiler
#endif
#endif

// TRACE_TOS 3.7.1
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_TOS Debug.LogSection=LOGSECTION_STEMDOS, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_TOS(x) // no code left?
#else
#define TRACE_TOS // some code left to the compiler
#endif
#endif

// TRACE_VID 3.7.3
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#ifdef __cplusplus // visible only to C++ objects
#define TRACE_VID Debug.LogSection=LOGSECTION_VIDEO, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) // OK for Unix?
#define TRACE_VID(x) // no code left?
#else
#define TRACE_VID // some code left to the compiler
#endif
#endif

// TRACE_OSD
#if defined(STEVEN_SEAGAL) && defined(SSE_OSD_DEBUG_MESSAGE)
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
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#if defined(_DEBUG) && defined(VC_BUILD)
// Our VERIFY facility has no MFC dependency.
#define VERIFY(x) {if(!(x) && !FullScreen) _asm{int 0x03}}
#elif defined(SSE_UNIX_TRACE)
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
#else //!SSE_DEBUG
#define VERIFY(x) x
#pragma warning(disable : 4552)
#endif

#if !defined(STEVEN_SEAGAL) || !defined(SSE_DEBUG) 
enum { // to pass compilation
 LOGSECTION_FDC_BYTES, // was DIV
 LOGSECTION_IMAGE_INFO, //was Pasti
// LOGSECTION_IPF_LOCK_INFO,
 LOGSECTION_OPTIONS,
 NUM_LOGSECTIONS,
 };
#endif

#if defined(SSE_DEBUG_FRAME_REPORT)
#define REPORT_LINE FrameEvents.ReportLine()
#else
#define REPORT_LINE
#endif
 

#endif//#ifndef SSEDEBUG_H
