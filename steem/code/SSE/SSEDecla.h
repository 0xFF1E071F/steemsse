#pragma once
#ifndef SSEDECLA_H
#define SSEDECLA_H

#include "SSE.h"
#if defined(SSE_BUILD)

///////////////
// COMPILERS //
///////////////

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
typedef unsigned char BYTE;
#define max(a,b) (a>b ? a:b)
#define min(a,b) (a>b ? b:a)
#endif

#if defined(SSE_UNIX) || defined(MINGW_BUILD)
#include <stdint.h>
#else // for CAPS, HFE
typedef signed __int8		int8_t;
typedef unsigned __int8		uint8_t;
typedef signed __int16		int16_t;
typedef unsigned __int16	uint16_t;
typedef signed __int32		int32_t;
typedef unsigned __int32	uint32_t;
typedef signed __int64		int64_t;
typedef unsigned __int64	uint64_t;
#endif

#ifdef WIN32
#include <windows.h>
#endif

#if defined(SSE_VS2008_WARNING_383) && !defined(NODEFAULT) //can be defined in caps/CommomTypes.h
#ifdef _DEBUG
#define NODEFAULT   assert(0)
#else
#define NODEFAULT   __assume(0)
#endif
#endif

#if defined(VC_BUILD)
#pragma warning (1 : 4710) // function '...' not inlined as warning L1
#pragma warning (disable : 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
// C4800 isn't a silly warning but to avoid it we should write !=0 ourselves,
// which is what the compiler does on its own, and warns us about
#if (MSC_VER <= 1200)//VC6
#pragma warning (disable : 4127) //conditional expression is constant (for scope trick)
#endif
#if defined(SSE_BOILER)
#pragma warning (disable : 4125) //decimal digit terminates octal escape sequence
#pragma warning (disable : 4127) //conditional expression is constant (for logsection)
#else
#pragma warning (disable : 4002) //too many actual parameters for macro 'TRACE_LOG'
#endif
#pragma warning (disable : 4244) //conversion from 'int' to 'short', possible loss of data
#pragma warning (disable : 4996)// This function or variable may be unsafe.
#endif

#if defined(SSE_VC_INTRINSICS_382)
#include <intrin.h>
#define BITTEST(var,bit) (_bittest((long*)&var,bit)/*!=0*/)
#if defined(SSE_VC_INTRINSICS_383)
#define BITRESET(var,bit) (_bittestandreset((long*)&var,bit))
#define BITSET(var,bit) (_bittestandset((long*)&var,bit))
#endif
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


///////////////
// INTERRUPT //
///////////////

#if defined(SSE_INT_MFP_RATIO) 
#define CPU_CYCLES_PER_MFP_CLK CpuMfpRatio
#endif


/////////
// IPF //
/////////

#if defined(SSE_DISK_CAPS)

#if defined(WIN32) && defined(SSE_DELAY_LOAD_DLL)

#ifdef _MSC_VER

#if defined(SSE_X64_LIBS)
#pragma comment(lib, "../../3rdparty/caps/x64/CAPSImg.lib")
#else
#pragma comment(lib, "../../3rdparty/caps/CAPSImg.lib")
#endif

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


#ifdef SSE_DISK_SCP
#define CAPSFDC_AI_AMDETENABLE BIT_0
#define CAPSFDC_AI_CRCENABLE   BIT_1
#define CAPSFDC_AI_CRCACTIVE   BIT_2
#define CAPSFDC_AI_AMACTIVE    BIT_3
#define CAPSFDC_AI_MA1ACTIVE   BIT_4
#define CAPSFDC_AI_AMFOUND     BIT_5
#define CAPSFDC_AI_MARKA1      BIT_6
#define CAPSFDC_AI_MARKC2      BIT_7
#define CAPSFDC_AI_DSRREADY    BIT_8
#define CAPSFDC_AI_DSRAM       BIT_9
#define CAPSFDC_AI_DSRMA1      BIT_10
#endif


#endif



/////////////
// VARIOUS //
/////////////

#if defined(SSE_VAR_ARCHIVEACCESS)||defined(SSE_ACSI)\
  ||defined(SSE_GUI_STATUS_STRING_ICONS)
#define SSE_ANSI_STRING
extern char ansi_name[MAX_PATH];
#endif

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
#if defined(SSE_VS2008_383)
#include <assert.h> //NODEFAULT
#endif

/////////////
// VERSION //
/////////////

#if defined(SSE_BUILD)

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
#elif defined(SSE_BOILER_FRAME_REPORT)
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
#if !defined(SSE_VID_D3D_ONLY)
#define BORDER_40 (SSEOption.DisplaySize==1&&border) // avoids verbosity
#endif
#endif


#endif

#if defined(SSE_VID_BORDERS_BIGTOP) // more hacks...
#undef BORDER_TOP
#define BORDER_TOP (  (DISPLAY_SIZE==BIGGEST_DISPLAY) \
  ? BIG_BORDER_TOP : ORIGINAL_BORDER_TOP )
#endif

#if defined(SSE_VID_FREEIMAGE4)
#ifdef _MSC_VER
#if defined(SSE_X64_LIBS)
#pragma comment(lib,"../../3rdparty/FreeImage/x64/FreeImage.lib")
#else
#pragma comment(lib,"../../3rdparty/FreeImage/FreeImage.lib")
#endif
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

#endif//ss