// This file is compiled as a distinct module (resulting in an OBJ file)

#include "../pch.h"

#include "SSEOption.h"
#include "SSEDebug.h"
#include "SSESTF.h" // for the enum




#if defined(SS_SSE_OPTION_STRUCT)

TOption SSEOption; // singleton

//tmp
#ifdef UNIX
#include <stdio.h>
#include <string.h>
//#define TRUE 1 // don't screw this one up...
//#define FALSE 0
#endif

TOption::TOption() {
  Init(); // a problem...
}


void TOption::Init() {
#if defined(SS_HACKS)
  Hacks=TRUE;
#endif
  HD6301Emu=FALSE;
  STEMicrowire=FALSE;
#if defined(SS_SOUND_FILTER_STF)
  PSGFilter=TRUE;
#endif
  STModel=0;//STE;
  CaptureMouse=TRUE;
  DisplaySize=0; // original Steem 3.2
  StealthMode=FALSE;
#if defined(DEBUG_BUILD)  
  OutputTraceToFile=TRUE; // can be disabled in Boiler
#else
  OutputTraceToFile=FALSE; 
#endif
  TraceFileLimit=FALSE;//TRUE; // stop TRACING to file at +-3+MB
  WakeUpState=0; 
  UseSDL=0;
  OsdDriveInfo=1;
  Dsp=1;
  OsdImageName=0;
  PastiJustSTX=0; // dangerous?
  Interpolate=0;
  StatusBar=1;
  WinVSync=0; // really need correct display (50hz, 100hz) or run at 60hz
  TripleBuffer=0; // CPU intensive
  StatusBarGameName=1;
  DriveSound=0;
}

#endif//#if defined(SS_SSE_OPTION_STRUCT)


#if defined(SS_SSE_CONFIG_STRUCT)

TConfig SSEConfig;

TConfig::TConfig() {
#ifdef WIN32
  ZeroMemory(this,sizeof(TConfig));
#else
  memset(this,0,sizeof(TConfig));
#endif
}

#endif//#if defined(SS_SSE_CONFIG_STRUCT)
