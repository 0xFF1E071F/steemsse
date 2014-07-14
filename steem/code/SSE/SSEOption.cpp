// This file is compiled as a distinct module (resulting in an OBJ file)

#include "../pch.h"

#include "SSEOption.h"
#include "SSEDebug.h"
#include "SSESTF.h" // for the enum




#if defined(SSE_SSE_OPTION_STRUCT)

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
#if defined(SSE_HACKS)
  Hacks=TRUE;
#endif
  HD6301Emu=0;
  STEMicrowire=0;
#if defined(SSE_SOUND_FILTER_STF)
  PSGFilter=1;
#endif
  STModel=0;//STE;
  CaptureMouse=1;
  DisplaySize=0; // original Steem 3.2
  StealthMode=0;
#if defined(DEBUG_BUILD)  
  OutputTraceToFile=1; // can be disabled in Boiler
#else
  OutputTraceToFile=0; 
#endif
  TraceFileLimit=0;
  WakeUpState=0; 
  UseSDL=0;
  OsdDriveInfo=1;
  Dsp=1;//irrelevant
  OsdImageName=0;
#if defined(SSE_PASTI_ONLY_STX_OPTION1)
  PastiJustSTX=1; // go for it! (needed for autoload)
#else
  PastiJustSTX=0; // dangerous? but handy!
#endif
  Interpolate=0;
  StatusBar=1;
  WinVSync=0; // really need correct display (50hz, 100hz) or run at 60hz
  TripleBuffer=0; // CPU intensive
  StatusBarGameName=1;
  DriveSound=0;
  SingleSideDriveMap=0;
  GhostDisk=0;
}

#endif//#if defined(SSE_SSE_OPTION_STRUCT)


#if defined(SSE_SSE_CONFIG_STRUCT)

TConfig SSEConfig;

TConfig::TConfig() {
#ifdef WIN32
  ZeroMemory(this,sizeof(TConfig));
#else
  memset(this,0,sizeof(TConfig));
#endif
}

#endif//#if defined(SSE_SSE_CONFIG_STRUCT)
