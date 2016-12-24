#include "SSE.h"
#if defined(SSE_BUILD)
#include "../pch.h"
#include <emulator.decla.h>
#include "SSEOption.h"
#include "SSEDebug.h"
#include "SSEInterrupt.h"
#include "SSEVideo.h"

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

#if defined(SSE_VAR_OPT_390)
#ifdef WIN32
  ZeroMemory(this,sizeof(TOption));
#else
  memset(this,0,sizeof(TOption));
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
#else//opt390?
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
  //StealthMode=0;
  EmuDetect=1;
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
#endif//opt390?
}

TConfig SSEConfig;

TConfig::TConfig() {
#ifdef WIN32
  ZeroMemory(this,sizeof(TConfig));
#else
  memset(this,0,sizeof(TConfig));
#endif
}

#if defined(SSE_STF)

int TConfig::SwitchSTType(int new_type) { 

  ASSERT(new_type>=0 && new_type<N_ST_MODELS);
  ST_TYPE=new_type;
  if(ST_TYPE!=STE) // all STF types
  {
//    stfm_borders=4; // Steem 3.2 command-line option STFMBORDER (not used)
#if defined(SSE_INT_VBL_STF)//no
    HblTiming=HBL_FOR_STF; 
#endif
#if defined(SSE_CPU_MFP_RATIO)
#if defined(SSE_CPU_MFP_RATIO_OPTION)
    if(OPTION_CPU_CLOCK)
      CpuMfpRatio=(double)CpuCustomHz/(double)MFP_CLOCK;
    else
#endif
    {
      CpuMfpRatio=(double)CPU_STF_PAL/(double)MFP_CLOCK;
      CpuNormalHz=CPU_STF_PAL;
    }
#endif
  }
  else //STE
  {
//    stfm_borders=0;
#if defined(SSE_INT_VBL_STF)
    HblTiming=HBL_FOR_STE;
#endif
#if defined(SSE_CPU_MFP_RATIO)
#if defined(SSE_CPU_MFP_RATIO_OPTION)
    if(OPTION_CPU_CLOCK)
      CpuMfpRatio=(double)CpuCustomHz/(double)MFP_CLOCK;
    else
#endif
    {
    CpuMfpRatio=(double)CPU_STE_PAL/(double)MFP_CLOCK;
    CpuNormalHz=CPU_STE_PAL; 
    }
#endif

  }
  
#if defined(SSE_CPU_MFP_RATIO)
  TRACE_INIT("CPU~%d hz\n",CpuNormalHz);
#if defined(SSE_CPU_MFP_RATIO_HIGH_SPEED) //fix v3.6.1 (broken v3.5.1)
  if(n_cpu_cycles_per_second<10000000) // avoid interference with ST CPU Speed option
#endif
    n_cpu_cycles_per_second=CpuNormalHz; // no wrong CPU speed icon in OSD (3.5.1)
#endif

#if defined(SSE_INT_VBL_STF)//no
  draw_routines_init(); // to adapt event plans (overkill?)
#endif

#if defined(SSE_GLUE)
  Glue.Update();
#endif

  return ST_TYPE;
}

#endif

#endif//sse