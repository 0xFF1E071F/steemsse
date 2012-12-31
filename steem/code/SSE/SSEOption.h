#pragma once
#ifndef SSEOPTION_H
#define SSEOPTION_H

#include "SSE.h" // to get the switches
#include "SSESTF.h" // for the enum
#ifdef WIN32
#include <windows.h>
#endif

#if defined(SS_SSE_OPTION_STRUCT)

struct TOption {

#if defined(SS_HACKS)
  int Hacks;
#endif
#if defined(SS_IKBD_6301)
  int HD6301Emu;
#endif
#if defined(SS_SOUND_MICROWIRE)
  int STEMicrowire;
#endif
#if defined(SS_SOUND_FILTER_STF)
  int PSGFilter;
#endif
#if defined(SS_STF)
  int STModel;
#endif
#if defined(SS_VAR_MOUSE_CAPTURE)
  int CaptureMouse;
#endif
#if defined(SS_VID_BORDERS)
  int DisplaySize;// 0=Steem 3.2 1,2 Large 3 Very large
#endif
#if defined(SS_VAR_STEALTH)
  int StealthMode;
#endif
  // we keep those also in the release build so that snapshots are compatible
  int OutputTraceToFile; // can be disabled in Boiler // BOOL -> int for Unix...
  int TraceFileLimit; // stop TRACING to file at +-3+MB // BOOL -> int for Unix...
#ifdef __cplusplus
  TOption();
  void Init();
#endif
};
// C linkage
#ifdef __cplusplus
extern "C" TOption SSEOption;
#else
extern struct TOption SSEOption;
#endif

// All access through macros to allow for struct or option not defined!


#define USE_TRACE_FILE (SSEOption.OutputTraceToFile)
#define TRACE_FILE_REWIND (SSEOption.TraceFileLimit)

#if defined(SS_HACKS)
#define SSE_HACKS_ON (SSEOption.Hacks)
#else
#define SSE_HACKS_ON (iDummy) 
#endif
#if defined(SS_IKBD_6301)
#define HD6301EMU_ON (SSEOption.HD6301Emu)
#else
#define HD6301EMU_ON (iDummy)
#endif
#if defined(SS_SOUND_MICROWIRE)
#define MICROWIRE_ON (SSEOption.STEMicrowire)
#else
#define MICROWIRE_ON (iDummy)
#endif
#if defined(SS_SOUND_FILTER_STF)
#define PSG_FILTER_FIX (SSEOption.PSGFilter)
#else
#define PSG_FILTER_FIX (iDummy)
#endif
#if defined(SS_STF)
#define ST_TYPE (SSEOption.STModel) // was the var name
#else
#define ST_TYPE (iDummy) 
#endif
#if defined(SS_VAR_MOUSE_CAPTURE)
#define CAPTURE_MOUSE (SSEOption.CaptureMouse)
#else
#define CAPTURE_MOUSE (iDummy)
#endif
#if defined(SS_VID_BORDERS)
#define BORDER_40 (SSEOption.DisplaySize==1||SSEOption.DisplaySize==2)
#define DISPLAY_SIZE (SSEOption.DisplaySize)
#else
#define BORDER_40 (iDummy)
#define DISPLAY_SIZE (iDummy)
#endif
#if defined(SS_VAR_STEALTH)
#define STEALTH_MODE SSEOption.StealthMode
#else
#endif

#else//!defined(SS_SSE_OPTION_STRUCT)

#define SSE_HACKS_ON (iDummy) 
#define BORDER_40 (iDummy)
#define HD6301EMU_ON (iDummy)
#define ST_TYPE (iDummy) 
#define MICROWIRE_ON (iDummy)
#define PSG_FILTER_FIX (iDummy)
#define STEALTH_MODE (iDummy)
#define USE_TRACE_FILE (iDummy)
#define TRACE_FILE_REWIND (iDummy)
#define CAPTURE_MOUSE (iDummy)
#define DISPLAY_SIZE (iDummy)

#endif//#if defined(SS_SSE_OPTION_STRUCT)

#endif//#ifndef SSEOPTION_H

