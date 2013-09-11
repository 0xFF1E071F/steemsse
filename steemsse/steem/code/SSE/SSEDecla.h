#pragma once
#ifndef SSEDECLA_H
#define SSEDECLA_H


/////////////////
// PORTABILITY //
/////////////////

#if defined(BCC_BUILD) // after x warnings, BCC stops compiling!
#pragma warn- 8004 
#pragma warn- 8012
#pragma warn- 8019
#pragma warn- 8027
#pragma warn- 8057
#pragma warn- 8071
#endif

#if !defined(WIN32)
#define TRUE 1
#define FALSE 0
#define BOOL int
typedef unsigned char BYTE;
#endif

#ifdef WIN32
#include <windows.h>
#endif


#if defined(VC_BUILD)
//#pragma warning (1 : 4710) // function '...' not inlined as warning L1
#pragma warning (disable : 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#endif

/////////
// FDC //
/////////




///////////
// Hacks //
///////////

#if ! defined(SS_SSE_CONFIG_STRUCT)
extern
#ifdef __cplusplus
 "C" 
#endif
int iDummy;// can be l-value
#endif



#if defined(SS_HACKS)
extern 
#ifdef __cplusplus
 "C" 
#endif
 //"C" 
int SS_signal; // "handy" global mask (future coding horror case)
#endif


//////////
// IKBD //
//////////

#if defined(SS_IKBD)

#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SS_MFP_RATIO) 
#define CPU_CYCLES_PER_MFP_CLK CpuMfpRatio
#endif


/////////
// IPF //
/////////

#if defined(SS_IPF)

#if defined(WIN32) && defined(SS_DELAY_LOAD_DLL)

#ifdef _MSC_VER
#pragma comment(lib, "../../3rdparty/caps/CAPSImg.lib")
#ifndef SS_VS2012_DELAYDLL
#pragma comment(linker, "/delayload:CAPSImg.dll")
#endif
#endif

#ifdef __BORLANDC__
#pragma comment(lib, "../../3rdparty/caps/bcclib/CAPSImg.lib")
///#pragma comment(linker, "/delayload:CAPSImg.dll")
#endif

#endif

#endif


//////////
// TEMP //
//////////

//before moving to a better location/class




/////////////
// VARIOUS //
/////////////

#if defined(SS_DELAY_LOAD_DLL)
// Delay loading DLLs
// No switch because code depends on it and it's desireable,
// and we can't make a switch for Borland (option directly in makefile)

#include "delayimp.h"

#ifdef _MSC_VER
#pragma comment(lib, "Delayimp.lib")
#ifndef SS_VS2012_DELAYDLL
#pragma comment(linker, "/ignore:4199")
#endif
#endif

#ifdef __BORLANDC__
extern DelayedLoadHook _EXPDATA __pfnDliFailureHook;
FARPROC WINAPI MyLoadFailureHook(dliNotification dliNotify, DelayLoadInfo * pdli);
#endif

#endif


///////////
// VIDEO //
///////////

#define LINECYCLE0 cpu_timer_at_start_of_hbl
#define LINECYCLES (ABSOLUTE_CPU_TIME-LINECYCLE0) 
#define FRAMECYCLES (ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)
//#if defined(SS_DEBUG) || defined(SS_INT_VBI_START) // TODO check
#if defined(SS_SHIFTER) && defined(SS_DEBUG)
#define FRAME (Shifter.nVbl) 
#elif defined(SS_SHIFTER_EVENTS)
#define FRAME (VideoEvents.nVbl)
#else 
#define FRAME (-1)
#endif

#if defined(SS_VIDEO)

#if defined(SS_VID_RECORD_AVI) && defined(__cplusplus)
#define _T(X) X
#define _tcscpy strcpy
#define _tcsncpy strncpy
#include <AVI/AviFile.h>
extern CAviFile *pAviFile;// tmp, useless to have it here
extern BYTE video_recording;
extern char *video_recording_codec;
#endif

#if defined(SS_VID_BORDERS)
extern BYTE SideBorderSize,BottomBorderSize, SideBorderSizeWin;
#define BORDER_SIDE SideBorderSize // avoids much rewriting in Steem!!!!!!!!!!!
#define BORDER_EXTRA (SideBorderSize-ORIGINAL_BORDER_SIDE) // 0 8 16, in pixels
#define BORDER_BOTTOM BottomBorderSize // !!!!!!!!!!!!!!!!!!!!!!!
int ChangeBorderSize(int size); // gui.cpp
#endif

#if defined(SS_VID_BORDERS_BIGTOP) // more hacks...
#define NEW_BORDER_TOP (35) // for The Musical Wonder 1990, 36 tested
#define BORDER_TOP (  DISPLAY_SIZE==3 ? NEW_BORDER_TOP : 30 ) 
#endif


#if defined(SS_VID_SCANLINES_INTERPOLATED)

#if defined(SS_VID_SCANLINES_INTERPOLATED_MED)

#define SCANLINES_INTERPOLATED \
 (draw_win_mode[screen_res]==DWM_STRETCH_SCANLINES&&!mixed_output&&screen_res<2)
#else

// note draw_win_mode[2] doesn't exist!
#define SCANLINES_INTERPOLATED (!screen_res && !mixed_output\
  &&draw_win_mode[screen_res]==DWM_STRETCH_SCANLINES) 
#endif
#endif

#endif

#ifndef FRAME
#define FRAME (-1)
#endif

#ifdef __cplusplus
#if defined(SS_OSD_SCROLLER_CONTROL)
#include "../../../include/easystr.h"

struct TOsdControl {
  enum {NO_SCROLLER,WANT_SCROLLER,SCROLLING};
  long ScrollerColour;
  BYTE ScrollerPhase;
  EasyStr ScrollText;
  TOsdControl() { ScrollerPhase=NO_SCROLLER; };
  void StartScroller(EasyStr text);
};

extern TOsdControl OsdControl;
#endif
#endif

#endif//SSEDECLA_H