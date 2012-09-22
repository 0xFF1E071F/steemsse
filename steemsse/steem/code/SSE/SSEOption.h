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
  int BorderSize;// 0=Steem 3.2 1,2 Large 3 Very large
#endif
#if defined(SS_VAR_STEALTH)
  int StealthMode;
#endif
  // we keep those also in the release build so that snapshots are compatible
  BOOL OutputTraceToFile; // can be disabled in Boiler
  BOOL TraceFileLimit; // stop TRACING to file at +-3+MB
#ifdef __cplusplus
  TOption();
  Init();
#endif
};
// C linkage
#ifdef __cplusplus
extern "C" TOption SSEOption;
#else
extern struct TOption SSEOption;
#endif


// shortcuts
#define SSE_HACKS_ON (SSEOption.Hacks)
#define BORDER_40 (SSEOption.BorderSize==1||SSEOption.BorderSize==2)
#define HD6301EMU_ON (SSEOption.HD6301Emu)
#define ST_TYPE (SSEOption.STModel) // was the var name


#endif//#if defined(SS_SSE_OPTION_STRUCT)
#endif//#ifndef SSEOPTION_H

