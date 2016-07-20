#include "SSE.h"
#if defined(SSE_BUILD)
#include "../pch.h"
#include "SSEOption.h"
#include "SSEDebug.h"
#include "SSESTF.h" // for the enum

TOption SSEOption; // singleton

//tmp
#ifdef UNIX
#include <stdio.h>
#include <string.h>
#endif

TOption::TOption() {
  Init(); // a problem...
}


void TOption::Init() {

#if defined(SSE_VAR_OPT_383)
#ifdef WIN32
  ZeroMemory(this,sizeof(TConfig));
#else
  memset(this,0,sizeof(TConfig));
#endif
#if defined(SSE_HACKS)
  Hacks=true;
#endif
#if defined(SSE_IKBD_6301)
  Chipset1=true;
#endif
  Chipset2=true;
#if defined(SSE_SOUND_FILTER_STF)
  PSGFilter=true;
#endif
#if defined(DEBUG_BUILD)  
  OutputTraceToFile=1;
#endif
  OsdDriveInfo=1;
  StatusBar=1;
  DriveSound=1;
#ifdef SSE_VID_DIRECT3D
  Direct3D=1;
#endif
#else//opt383?
#if defined(SSE_HACKS)
  Hacks=TRUE;
#endif
  Chipset1=Chipset2=true;
  Microwire=0;
#if defined(SSE_SOUND_FILTER_STF)
  PSGFilter=1;
#endif
  STModel=0;//STE;
  CaptureMouse=1;
  DisplaySize=0; // original Steem 3.2
  StealthMode=0;
#if defined(DEBUG_BUILD)  
  OutputTraceToFile=1;
#else
  OutputTraceToFile=0; 
#endif
  TraceFileLimit=0;
  WakeUpState=0; 
  UseSDL=0;
  OsdDriveInfo=1;
  Dsp=1;//irrelevant
  OsdImageName=0;
  PastiJustSTX=0; // dangerous? but handy!
  Interpolate=0;
  StatusBar=1;
  WinVSync=0; // really need correct display (50hz, 100hz) or run at 60hz
  TripleBuffer=0; // CPU intensive
  StatusBarGameName=1;
  DriveSound=0;
  SingleSideDriveMap=0;
  GhostDisk=0;
#ifdef SSE_VID_DIRECT3D
  Direct3D=1;
#else
  Direct3D=0; // just in case
#endif
  STAspectRatio=0;
  DriveSoundSeekSample=0;
  TestingNewFeatures=0; //not defined, not loaded, not enabled
  BlockResize=1;
  LockAspectRatio=0;
  FinetuneCPUclock=0;
  PRG_support=1;
  Direct3DCrisp=0;
#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
  VMMouse=0;
#endif
#if defined(SSE_OSD_SHOW_TIME)
  OsdTime=0;
#endif
#endif//opt383?
}


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

#endif//sse