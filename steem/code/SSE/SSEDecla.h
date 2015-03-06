#pragma once
#ifndef SSEDECLA_H
#define SSEDECLA_H


/////////////////
// PORTABILITY //
/////////////////

//dinput8.lib dxguid.lib winmm.lib ComCtl32.Lib odbc32.lib odbccp32.lib d3d9.lib

/*
 DEFINE_GUID( IID_IDirectDraw,		0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
 DEFINE_GUID( IID_IDirectDraw2,		0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
*/




#if defined(BCC_BUILD) // after x warnings, BCC stops compiling!
#pragma warn- 8004 
#pragma warn- 8010 // continuation character
#pragma warn- 8012
#pragma warn- 8019
#pragma warn- 8027
#pragma warn- 8057
#pragma warn- 8071
#endif

#if !defined(WIN32) //|| defined(MINGW_BUILD)
//#define TRUE 1
//#define FALSE 0
//#define BOOL int
typedef unsigned char BYTE;
//typedef BYTE BOOL;
//#include <notwindows.h>

#define max(a,b) (a>b ? a:b)
#define min(a,b) (a>b ? b:a)

#endif

#ifdef WIN32
#include <windows.h>
#endif


#if defined(VC_BUILD)
#pragma warning (1 : 4710) // function '...' not inlined as warning L1
#pragma warning (disable : 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#if defined(_MSC_VER) && _MSC_VER <= 1200
#pragma warning (disable : 4127) //conditional expression is constant (for scope trick)
#endif
#pragma warning (disable : 4002) //too many actual parameters for macro 'TRACE_LOG'
#pragma warning (disable : 4244) //conversion from 'int' to 'short', possible loss of data
#endif


/////////
// CPU //
/////////

#define ACT ABSOLUTE_CPU_TIME

#if !defined(SSE_CPU)
#define IRC   prefetch_buf[1] // Instruction Register Capture
#define IR    prefetch_buf[0] // Instruction Register
#define IRD   ir              // Instruction Register Decoder
#endif

/////////
// FDC //
/////////

#if SSE_VERSION<=341
#define DRIVE floppy_current_drive()
#endif

///////////
// Hacks //
///////////

#if ! defined(SSE_SSE_CONFIG_STRUCT)
extern
#ifdef __cplusplus
 "C" 
#endif
int iDummy;// can be l-value
#endif



#if defined(SSE_HACKS)
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

#if defined(SSE_IKBD)

#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SSE_INT_MFP_RATIO) 
#define CPU_CYCLES_PER_MFP_CLK CpuMfpRatio
#endif


/////////
// IPF //
/////////

#if defined(SSE_IPF)

#if defined(WIN32) && defined(SSE_DELAY_LOAD_DLL)

#ifdef _MSC_VER
#pragma comment(lib, "../../3rdparty/caps/CAPSImg.lib")
#ifndef SSE_VS2003_DELAYDLL
#pragma comment(linker, "/delayload:CAPSImg.dll")
#endif
#endif

#ifdef __BORLANDC__
///Lib:DelayImp.lib
//#pragma comment(lib, "DelayImp.lib")
#pragma comment(lib, "../../3rdparty/caps/bcclib/CAPSImg.lib")
///#pragma comment(linker, "/delayload:CAPSImg.dll")
#endif

#endif

#endif


/////////////
// SHIFTER //
/////////////

#if defined(SSE_SHIFTER)

#if !defined(SSE_DEBUG) 
#define draw_check_border_removal Shifter.CheckSideOverscan
#define draw_scanline_to(cycles_since_hbl) Shifter.Render(cycles_since_hbl)
#endif

#endif


//////////
// TEMP //
//////////

//before moving to a better location/class



//////////
// UNIX //
//////////

#ifdef SSE_UNIX
///////#define ZeroMemory(p,sz) memset((p),0,(sz))
#endif



/////////////
// VARIOUS //
/////////////

#if defined(SSE_DELAY_LOAD_DLL)
// Delay loading DLLs
// No switch because code depends on it and it's desireable,
// and we can't make a switch for Borland (option directly in makefile)


// CAPSImg.dll FreeImage.dll unrar.dll
// SDL.dll


#if defined(_MSC_VER) || defined(BCC_BUILD)
#include "delayimp.h"
#endif

#ifdef _MSC_VER
#pragma comment(lib, "Delayimp.lib")
#ifndef SSE_VS2003_DELAYDLL
#pragma comment(linker, "/ignore:4199")
#endif
#endif

//TODO broken! the BCC build needs the DLL anyway 
#ifdef __BORLANDC__
extern DelayedLoadHook _EXPDATA __pfnDliFailureHook;
FARPROC WINAPI MyLoadFailureHook(dliNotification dliNotify, DelayLoadInfo * pdli);
#endif

#endif


#define TIMING_INFO FRAME,scan_y,LINECYCLES


/////////////
// VERSION //
/////////////

#if defined(SSE_VERSION)

#define SSE_VERSION_TXT_LEN 8// "3.7.0" +...
extern BYTE *stem_version_text[SSE_VERSION_TXT_LEN];
#define WINDOW_TITLE stem_window_title
#endif

///////////
// VIDEO //
///////////

#define LINECYCLE0 cpu_timer_at_start_of_hbl
#define LINECYCLES (ABSOLUTE_CPU_TIME-LINECYCLE0) 
#define FRAMECYCLES (ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)

#if defined(SSE_SHIFTER) && defined(SSE_DEBUG)
#define FRAME (Shifter.nVbl) 
#elif defined(SSE_DEBUG_FRAME_REPORT)
#define FRAME (FrameEvents.nVbl)
#else 
#define FRAME (-1)
#endif

#if defined(SSE_VIDEO)

#if defined(SSE_VID_RECORD_AVI) && defined(__cplusplus)
#define _T(X) X
#define _tcscpy strcpy
#define _tcsncpy strncpy
#include <AVI/AviFile.h>
extern CAviFile *pAviFile;// tmp, useless to have it here
extern BYTE video_recording;
extern char *video_recording_codec;
#endif

#if defined(SSE_VID_BORDERS)
extern BYTE SideBorderSize,BottomBorderSize, SideBorderSizeWin;
#define BORDER_SIDE SideBorderSize // avoids much rewriting in Steem!!!!!!!!!!!
#define BORDER_EXTRA (SideBorderSize-ORIGINAL_BORDER_SIDE) // 0 8 16, in pixels
#define BORDER_BOTTOM BottomBorderSize // !!!!!!!!!!!!!!!!!!!!!!!
int ChangeBorderSize(int size); // gui.cpp


#if defined(SSE_VID_BORDERS_LB_DX1) // see SSEDecla.h
#define BORDER_40 (SSEOption.DisplaySize==1&&border) // avoids verbosity
#endif


#endif

#if defined(SSE_VID_BORDERS_BIGTOP) // more hacks...
#undef BORDER_TOP
#define BORDER_TOP (  (DISPLAY_SIZE==BIGGEST_DISPLAY) \
  ? BIG_BORDER_TOP : ORIGINAL_BORDER_TOP )
#endif

#if defined(SSE_VID_FREEIMAGE4)
#ifdef _MSC_VER
#pragma comment(lib,"../../3rdparty/FreeImage/FreeImage.lib")
#ifndef SSE_VS2003_DELAYDLL
#pragma comment(linker, "/delayload:FreeImage.dll")
#endif
#endif
#ifdef __BORLANDC__
#pragma comment(lib,"../../3rdparty/FreeImage/bcclib/FreeImage.lib")
// Borland: delay load DLL in makefile
#endif


#endif

#if defined(SSE_VID_SCANLINES_INTERPOLATED)
#if defined(SSE_VID_SCANLINES_INTERPOLATED_SSE)
#define SCANLINES_INTERPOLATED \
 (SSE_INTERPOLATE/*&&draw_win_mode[screen_res]==DWM_GRILLE*/&&!mixed_output&&screen_res<2)
#elif defined(SSE_VID_SCANLINES_INTERPOLATED_MED)
#define SCANLINES_INTERPOLATED \
 (draw_win_mode[screen_res]==DWM_STRETCH_SCANLINES&&!mixed_output&&screen_res<2)
#else
// note draw_win_mode[2] doesn't exist!
#define SCANLINES_INTERPOLATED (!screen_res && !mixed_output\
  &&draw_win_mode[screen_res]==DWM_STRETCH_SCANLINES) 
#endif
#else
#define SCANLINES_INTERPOLATED (false) //unix
#endif


#endif

#ifndef FRAME
#define FRAME (-1)
#endif

#ifdef __cplusplus
#if defined(SSE_OSD_SCROLLER_CONTROL)
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
