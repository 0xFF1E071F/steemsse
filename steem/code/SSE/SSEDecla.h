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
#endif

#ifdef WIN32
#include <windows.h>
#endif


#if defined(VC_BUILD)
//#pragma warning (1 : 4710) // function '...' not inlined as warning L1
#pragma warning (disable : 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#endif


///////////
// Hacks //
///////////

extern
#ifdef __cplusplus
 "C" 
#endif
int iDummy;// can be l-value


#if defined(SS_HACKS)
extern "C" int SS_signal; // "handy" global mask (future coding horror case)
#endif


//////////
// IKBD //
//////////

#if defined(SS_IKBD)

#if defined(SS_IKBD_TRACE_COMMANDS) || defined(SS_IKBD_6301_DISASSEMBLE_CUSTOM_PRG)
#define SS_IKBD_6301_CHECK_COMMANDS // make defines coherent
#endif

#endif





///////////////
// INTERRUPT //
///////////////

#if defined(SS_MFP_RATIO) 
#define CPU_CYCLES_PER_MFP_CLK CpuMfpRatio
#endif


/////////////
// VARIOUS //
/////////////





///////////
// VIDEO //
///////////


#define LINECYCLE0 cpu_timer_at_start_of_hbl
#define LINECYCLES (ABSOLUTE_CPU_TIME-LINECYCLE0) 
#define FRAMECYCLES (ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)
//#if defined(SS_DEBUG) || defined(SS_INT_VBI_START) // TODO check
#if defined(SS_SHIFTER)
#define FRAME (Shifter.nVbl) 
#elif defined(SS_SHIFTER_EVENTS)
#define FRAME (VideoEvents.nVbl)
#else 
#define FRAME (-1)
#endif



#if defined(SS_VIDEO)

#if defined(STEVEN_SEAGAL) && defined(SS_VID_RECORD_AVI) 
#define _T(X) X
#define _tcscpy strcpy
#define _tcsncpy strncpy
#include <AVI/AviFile.h>
extern CAviFile *pAviFile;// tmp, useless to have it here
extern int video_recording;
extern char *video_recording_codec;
#endif

#if defined(SS_VID_BORDERS)
extern int SideBorderSize,BottomBorderSize;
extern int SideBorderSizeWin;
#define SS_VID_BORDERS_HACKS

#define BORDER_SIDE SideBorderSize // avoids much rewriting in Steem!!!!!!!!!!!
#define BORDER_EXTRA (SideBorderSize-ORIGINAL_BORDER_SIDE) // 0 8 16, in pixels
#define BORDER_BOTTOM BottomBorderSize // !!!!!!!!!!!!!!!!!!!!!!!
int ChangeBorderSize(int size); // gui.cpp
#endif

#endif

#ifndef FRAME
#define FRAME (-1)
#endif

#endif//SSEDECLA_H