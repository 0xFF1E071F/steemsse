#pragma once
#ifndef SSEOPTION_H
#define SSEOPTION_H
/*  SSEOption defines structures TOption (player choices) and TConfig 
    (internal use).
*/


#include "SSE.h" 

#ifdef WIN32
#include <windows.h>
#endif

#if defined(SS_SSE_OPTION_STRUCT)

struct TOption {
  // We keep all options in the structure so that snapshots are compatible
  // even if a feature isn't compiled
  int Hacks;
  int HD6301Emu;
  int STEMicrowire;
  int PSGFilter;
  int STModel;
  int CaptureMouse;
  int DisplaySize;// 0=Steem 3.2 1,2 Large 3 Very large
  int StealthMode;
  int OutputTraceToFile; // can be disabled in Boiler // BOOL -> int for Unix...
  int TraceFileLimit; // stop TRACING to file at +-3+MB // BOOL -> int for Unix...
  int WakeUpState; 
#ifdef __cplusplus // visible only to C++ objects
  TOption();
  void Init();
#endif
};

// C linkage to be accessible by 6301 emu (and maybe others)
#ifdef __cplusplus
extern "C" TOption SSEOption;
#else
extern struct TOption SSEOption;
#endif

// All access through macros to allow for struct not defined!
#define SSE_HACKS_ON (SSEOption.Hacks)
#define HD6301EMU_ON (SSEOption.HD6301Emu)
#define MICROWIRE_ON (SSEOption.STEMicrowire)
#define PSG_FILTER_FIX (SSEOption.PSGFilter)
#define ST_TYPE (SSEOption.STModel)
#define CAPTURE_MOUSE (SSEOption.CaptureMouse)
#define BORDER_40 (SSEOption.DisplaySize==1||SSEOption.DisplaySize==2)
#define DISPLAY_SIZE (SSEOption.DisplaySize)
#define STEALTH_MODE SSEOption.StealthMode
#define USE_TRACE_FILE (SSEOption.OutputTraceToFile)
#define TRACE_FILE_REWIND (SSEOption.TraceFileLimit)
#define WAKE_UP_STATE (SSEOption.WakeUpState)

#else//!defined(SS_SSE_OPTION_STRUCT)

#endif//#if defined(SS_SSE_OPTION_STRUCT)


#if defined(SS_SSE_CONFIG_STRUCT)

struct TConfig {
  // files to load at start, were they found?
  // we list them all, use will be progressive
  enum {
    UNRARDLL,
    SDLDLL,
    HD6301V1IMG,
    UNZIP32DLL,
    CAPSIMGDLL,
    PASTIDLL,
    MAX_LIST
  } FileList;
  int Loaded[MAX_LIST];
  int DDFullscreenMask; // mask?
#ifdef __cplusplus // visible only to C++ objects
  TConfig();
//  void Init();
#endif
};

// C linkage to be accessible by 6301 emu (and maybe others)
#ifdef __cplusplus
extern "C" TConfig SSEConfig;
#else
extern struct TConfig SSEConfig;
#endif

#define CAPSIMG_OK (SSEConfig.Loaded[TConfig::CAPSIMGDLL])
#define DD_FULLSCREEN (SSEConfig.DDFullscreenMask)
#define HD6301_OK (SSEConfig.Loaded[TConfig::HD6301V1IMG])
#define SDL_OK (SSEConfig.Loaded[TConfig::SDLDLL])
#define UNRAR_OK (SSEConfig.Loaded[TConfig::UNRARDLL])

#else//#if ! defined(SS_SSE_CONFIG_STRUCT)

#define CAPSIMG_OK (Caps.Version)
#define DD_FULLSCREEN (iDummy)
#define HD6301_OK (HD6301.Initialised)
#define SDL_OK (iDummy)
#define UNRAR_OK (iDummy)

#endif//#if defined(SS_SSE_CONFIG_STRUCT)



#endif//#ifndef SSEOPTION_H

