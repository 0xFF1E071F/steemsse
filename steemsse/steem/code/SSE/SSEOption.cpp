#include "SSE.h"
#if defined(SSE_BUILD)
#include "../pch.h"

#if defined(SSE_GUI_ADVANCED)
#include <easystr.h>
#define Str EasyStr
#include <conditions.h>
#include <display.decla.h>
#include <fdc.decla.h>
#include <gui.decla.h>
#include <loadsave.decla.h>
#include <options.decla.h> 
#include <stports.decla.h>
#include <psg.decla.h>
extern TOptionBox OptionBox;
#endif

#include <emulator.decla.h>

#include "SSEOption.h" 
#include "SSEDebug.h"
#include "SSEInterrupt.h"
#include "SSEVideo.h"
#include "SSEFloppy.h" //for ym2149


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
#if defined(DEBUG_BUILD)  
  OutputTraceToFile=1;
#endif
  OsdDriveInfo=1;
  StatusBar=1;
  DriveSound=1;
#ifdef SSE_VID_D3D
  Direct3D=1;
#endif
  SampledYM=true;
//393
  Microwire=1;
  EmuDetect=1;
  PastiJustSTX=1;
  FakeFullScreen=1; // safer than 32x200!
  KeyboardClick=1;
}


#if defined(SSE_GUI_ADVANCED)
/*  Restore advanced settings.
    All when player pressed the 'Reset Advanced Settings' button.
    Not all when player unchecked the 'Advanced Settings' option.
*/

void TOption::Restore(bool all) {
  Chipset1=TRUE; // need file
  Chipset2=true;

  run_speed_ticks_per_second=1000;
  floppy_access_ff=false;

  FinetuneCPUclock=false;
  SSEConfig.OverscanOn=false;
  if(STModel>STF)
    STModel=STF;
  SSEConfig.SwitchSTType(ST_TYPE); // updates cpu/mfp ratio
  n_cpu_cycles_per_second=CpuNormalHz;
  FakeFullScreen=true;
  extended_monitor=0;
  // display size
  WinSizeForRes[0]=1; // double low
  WinSizeForRes[1]=1; // double height med
  WinSizeForRes[2]=0; // normal high
#if defined(SSE_GUI_CRISP_IN_DISPLAY)
  draw_win_mode[0]=draw_win_mode[1]=Direct3DCrisp; 
#else
  draw_win_mode[0]=draw_win_mode[1]=1;
#endif
  StemWinResize();
#if USE_PASTI
  pasti_active=false;
#endif
  SampledYM=true; // need file
  YM2149.LoadFixedVolTable();
#if defined(SSE_YM2149_MAMELIKE_394)
  YmLowLevel=true;
#endif
  Microwire=true;
  sound_freq=44100;
  sound_write_primary=false;
  sound_time_method=1; //def?
  OptionBox.ChangeSoundFormat(16,2); // needs lots of declarations but proper
  
  if(all) // player pressed 'Reset Advanced Settings'
  {
    VMMouse=false;
    floppy_instant_sector_access=false;
    EmuDetect=false;
    Hacks=true;
    Str NewCart="";
    LoadSnapShotChangeCart(NewCart); // remove cartridge
    DONGLE_ID=0;
    HighPriority=false;
    //AutoLoadSnapShot=false;
    Disp.DrawToVidMem=true;
    Disp.BlitHideMouse=false;
    WriteCSFStr("Options","NoDirectDraw","0",INIFile);
    WriteCSFStr("Options","NoDirectSound","0",INIFile);
    NoDsp=false;
  }
}

#endif



TConfig SSEConfig;

#if defined(SSE_GUI_FONT_FIX)
HFONT my_gui_font=NULL; // use global, not member, because of constructor order
#endif

TConfig::TConfig() {
#ifdef WIN32
  ZeroMemory(this,sizeof(TConfig));
#else
  memset(this,0,sizeof(TConfig));
#endif
}

TConfig::~TConfig() {
#if defined(SSE_GUI_FONT_FIX)
  if(my_gui_font)
  {
    DeleteObject(my_gui_font); // free Windows resource
#ifndef LEAN_AND_MEAN
    my_gui_font=NULL;
#endif
  }
#endif
}

#if defined(SSE_GUI_FONT_FIX)
/*  Steem used DEFAULT_GUI_FONT at several places to get the GUI font.
    On some systems it is different, and it can seriously mess the GUI.
    With this mod, we create a logical font with the parameters of
    DEFAULT_GUI_FONT on a normal system.
    Only if it fails do we use DEFAULT_GUI_FONT.
    Notice that this function is called as soon as needed, which is
    in a constructor. That means we can't reliably use TRACE() here.
update: turns out it was a problem of Windows size setting instead, so
this isn't enabled after all
*/

HFONT TConfig::GuiFont() {
  if(my_gui_font==NULL)
  {
    my_gui_font=CreateFont(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 
      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
      "MS Shell Dlg");
    ASSERT(my_gui_font);
    if(!my_gui_font) // fall back
      my_gui_font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
#ifdef SSE_DEBUG__
    LOGFONT fi;
    GetObject(my_gui_font,sizeof(fi),&fi);
#endif
  }
  return my_gui_font;
}

#endif


#if defined(SSE_VID_BPP_CHOICE) && !defined(SSE_VID_BPP_NO_CHOICE) 

int TConfig::GetBitsPerPixel() {
  int bpp=(SSEConfig.VideoCard32bit)?32:16; // default
  if(display_option_fs_bpp==1)
  {
    if(SSEConfig.VideoCard8bit)
      bpp=16;
  }
  else if (display_option_fs_bpp==0)
  {
    if(SSEConfig.VideoCard8bit)
      bpp=8;
    else if(SSEConfig.VideoCard16bit)
      bpp=16;
  }
  return bpp;
}

#endif

#if defined(SSE_STF)

int TConfig::SwitchSTType(int new_type) { 

  ASSERT(new_type>=0 && new_type<N_ST_MODELS);
  ST_TYPE=new_type;
  if(ST_TYPE!=STE) // all STF types
  {
#if defined(SSE_CPU_MFP_RATIO)
#if defined(SSE_CPU_MFP_RATIO_OPTION)
    if(OPTION_CPU_CLOCK)
      CpuMfpRatio=(double)CpuCustomHz/(double)MFP_CLOCK;
    else
#endif
    {
#if defined(SSE_STF_MEGASTF_CLOCK)
      if(ST_TYPE==MEGASTF)
      {
        CpuMfpRatio=((double)CPU_STF_MEGA-0.5)/(double)MFP_CLOCK; //it's rounded up in hz
        CpuNormalHz=CPU_STF_MEGA;
      }
      else
      {
        CpuMfpRatio=(double)CPU_STF_PAL/(double)MFP_CLOCK;
        CpuNormalHz=CPU_STF_PAL;
      }
#else
      CpuMfpRatio=(double)CPU_STF_PAL/(double)MFP_CLOCK;
      CpuNormalHz=CPU_STF_PAL;
#endif
    }
#endif
  }
  else //STE
  {
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
#if defined(SSE_CPU_MFP_RATIO_HIGH_SPEED) && !defined(SSE_GUI_NO_CPU_SPEED)
  if(n_cpu_cycles_per_second<10000000) // avoid interference with ST CPU Speed option
#endif
    n_cpu_cycles_per_second=CpuNormalHz; // no wrong CPU speed icon in OSD (3.5.1)
#endif
#if defined(SSE_GLUE)
  Glue.Update();
#endif

  return ST_TYPE;
}

#endif

#endif//sse