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

#include "SSEDecla.h" //BYTE

#if defined(SS_SSE_OPTION_STRUCT)

struct TOption {
  // We keep all options in the structure so that snapshots are compatible
  // even if a feature isn't compiled
  BYTE STModel;
  BYTE DisplaySize;
  BYTE WakeUpState; 
  unsigned int Hacks:1;
  unsigned int HD6301Emu:1;
  unsigned int STEMicrowire:1;
  unsigned int PSGFilter:1;
  unsigned int CaptureMouse:1;
  unsigned int StealthMode:1;
  unsigned int OutputTraceToFile:1; // can be disabled in Boiler // BOOL -> int for Unix...
  unsigned int TraceFileLimit:1; // stop TRACING to file at +-3+MB // BOOL -> int for Unix...
  unsigned int UseSDL:1;
  unsigned int OsdDriveInfo:1;
  unsigned int Dsp:1; // giving the ability to disable buggy DSP
  unsigned int OsdImageName:1;
  unsigned int PastiJustSTX:1;
  unsigned int Interpolate:1;
  unsigned int StatusBar:1;
  unsigned int WinVSync:1;
  unsigned int TripleBuffer:1;
  unsigned int StatusBarGameName:1;
  unsigned int DriveSound:1;
  unsigned int SingleSideDriveMap:2;
  unsigned int PSGMod:1;
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

// these macros were considered useful at some point
#define SSE_HACKS_ON (SSEOption.Hacks)
#define HD6301EMU_ON (SSEOption.HD6301Emu)
#define MICROWIRE_ON (SSEOption.STEMicrowire)
#define PSG_FILTER_FIX (SSEOption.PSGFilter)
#define ST_TYPE (SSEOption.STModel)
#define CAPTURE_MOUSE (SSEOption.CaptureMouse)
#define BORDER_40 (SSEOption.DisplaySize==1)//||SSEOption.DisplaySize==2)
#define DISPLAY_SIZE (SSEOption.DisplaySize)
#define STEALTH_MODE SSEOption.StealthMode
#define USE_TRACE_FILE (SSEOption.OutputTraceToFile)
#define TRACE_FILE_REWIND (SSEOption.TraceFileLimit)
#define WAKE_UP_STATE (SSEOption.WakeUpState)
#define USE_SDL (SSEOption.UseSDL)
#define OSD_DRIVE_INFO (SSEOption.OsdDriveInfo)
#define DSP_ENABLED (SSEOption.Dsp)
#define OSD_IMAGE_NAME (SSEOption.OsdImageName)
#define PASTI_JUST_STX (SSEOption.PastiJustSTX)
#define SSE_INTERPOLATE (SSEOption.Interpolate)
#define SSE_STATUS_BAR (SSEOption.StatusBar)
#define SSE_STATUS_BAR_GAME_NAME (SSEOption.StatusBarGameName)
#define SSE_WIN_VSYNC (SSEOption.WinVSync)
#define SSE_3BUFFER (SSEOption.TripleBuffer)
#define SSE_DRIVE_SOUND (SSEOption.DriveSound)
#else//!defined(SS_SSE_OPTION_STRUCT)

#endif//#if defined(SS_SSE_OPTION_STRUCT)


#if defined(SS_SSE_CONFIG_STRUCT)

struct TConfig {

  int DDFullscreenMask; // mask?

  // files to load at start, were they found?
  unsigned int UnrarDll:1;
  unsigned int SdlDll:1;
  unsigned int Hd6301v1Img:1;
  unsigned int Unzip32Dll:1;
  unsigned int CapsImgDll:1;
  unsigned int PastiDll:1;

#ifdef __cplusplus // visible only to C++ objects
  TConfig();
#endif
};

// C linkage to be accessible by 6301 emu (and maybe others)
#ifdef __cplusplus
extern "C" TConfig SSEConfig;
#else
extern struct TConfig SSEConfig;
#endif

#define CAPSIMG_OK (SSEConfig.CapsImgDll)
#define DD_FULLSCREEN (SSEConfig.DDFullscreenMask)
#define HD6301_OK (SSEConfig.Hd6301v1Img)
#define SDL_OK (SSEConfig.SdlDll)
#define UNRAR_OK (SSEConfig.UnrarDll)



#else//#if ! defined(SS_SSE_CONFIG_STRUCT)

#define CAPSIMG_OK (Caps.Version)
#define DD_FULLSCREEN (iDummy)
#define HD6301_OK (HD6301.Initialised)
#define SDL_OK (iDummy)
#define UNRAR_OK (iDummy)

#endif//#if defined(SS_SSE_CONFIG_STRUCT)

#endif//#ifndef SSEOPTION_H

