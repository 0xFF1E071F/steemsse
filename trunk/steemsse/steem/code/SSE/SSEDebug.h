#pragma once
#ifndef SSEDEBUG_H
#define SSEDEBUG_H


#if defined(SS_DEBUG)

extern "C" int debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;
// 4,5,6 cleared every VBL, 7,8,9 every HBL (too much?)


struct TDebug {
  TDebug();
  ~TDebug();
#if !defined(_DEBUG) && defined(DEBUG_BUILD)
  void TraceToFile(char *fmt, ...);  
#endif
  FILE *trace_file_pointer; 
  BOOL ReportBreakpoints; // disabled when clicking cancel in the box
  int nTrace;
  int ShifterTricks;
};
extern TDebug SSDebug;

#endif


#if defined(SS_DEBUG)

#define TRACE_MAX_WRITES 200000 // to avoid too big file
 
#if defined(_DEBUG) && defined(VC_BUILD)

#define TRACE my_trace // to be independent of MFC
extern "C" void my_trace(char *fmt, ...);
#define ASSERT(x) {if(!(x) && !FullScreen) _asm{int 0x03}}
#define VERIFY(x) {if(!(x) && !FullScreen) _asm{int 0x03}}
#define BREAKPOINT _asm { int 3 }
#define BRK(x) {TRACE(#x); TRACE("\n");}

#else // for boiler

#define TRACE SSDebug.TraceToFile
#define BREAKPOINT {if(SSDebug.ReportBreakpoints) \
SSDebug.ReportBreakpoints=(Alert("Breakpoint! Click cancel to stop those \
boxes","BRK",MB_OKCANCEL)==IDOK);} 
#define ASSERT(x) {if(SSDebug.ReportBreakpoints&&(!(x))) { \
TRACE("Assert failed: %s\n",#x); \
SSDebug.ReportBreakpoints=(Alert(#x,"ASSERT",MB_OKCANCEL)==IDOK);}}
#define VERIFY ASSERT
#define BRK(x){if(SSDebug.ReportBreakpoints) { \
TRACE("Breakpoint: %s\n",#x); \
SSDebug.ReportBreakpoints=(Alert(#x,"Breakpoint",MB_OKCANCEL)==IDOK);}}

#endif

#else // release versions

#define BREAKPOINT 
#define VERIFY(x) x // I never use VERIFY

#if defined(SS_UNIX_TRACE____) //tmp
#define TRACE my_trace
void my_trace(char *fmt, ...);
#else
#define TRACE
#endif
#define ASSERT(x)
#define BRK(x) 

#endif 

#if defined(SS_DEBUG_START_STOP_INFO)
enum EReportGeneralInfos {START,STOP} ;
int ReportGeneralInfos(int when);
#endif

#endif// SSEDEBUG_H