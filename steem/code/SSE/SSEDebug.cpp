#include "SSE.h"
#include "SSEDebug.h"

#if defined(SSE_DEBUG) || defined(DEBUG_FACILITIES_IN_RELEASE)

#if defined(UNIX) || defined(BCC_BUILD)
#include "../pch.h" 
#pragma hdrstop 
#else
#include <ddraw.h>
#endif

#ifdef WIN32
#include <time.h>
#endif
#include <emulator.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <gui.decla.h>//WriteDir//alert
#include <harddiskman.decla.h>
#include <mfp.decla.h>
#include <mymisc.h>//getexe
#include <run.decla.h>
#include <steemh.decla.h>
#include <cpu.decla.h>


#include <stports.decla.h>

#include "SSEFloppy.h"
#include <display.decla.h>
#include <init_sound.decla.h>
//#include "SSEMMU.h"
#include "SSEVideo.h"
#include "SSEInterrupt.h"
#include "SSECpu.h"
#include <stdio.h>
#include <stdarg.h>


#if defined(SSE_DEBUG)
int debug0,debug1=0,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;
#endif

#if defined(SSE_DEBUG)
#if defined(SSE_IKBD_6301)
#if defined(SSE_UNIX)
extern "C" void (*hd6301_trace)(char *fmt, ...);
#else
extern "C" void (_stdcall *hd6301_trace)(char *fmt, ...);
#endif
#endif
#endif

TDebug Debug; // singleton, now present in all builds

TDebug::TDebug() {
#if defined(SSE_DEBUG)
  ZeroMemory(&debug0,10*sizeof(int));  
#endif
#if defined(SSE_DEBUG_LOG_OPTIONS)
  //  We must init those variables for the builds without the boiler
  ZeroMemory(logsection_enabled,100*sizeof(bool)); // 100> our need
  logsection_enabled[ LOGSECTION_ALWAYS ] = 1;
#if defined(_DEBUG) && !defined(DEBUG_BUILD) // VC6 IDE debug no boiler
  logsection_enabled[ LOGSECTION_FDC ] = 0;
  logsection_enabled[ LOGSECTION_IO ] = 0;
  logsection_enabled[ LOGSECTION_MFP_TIMERS ] = 0;
  logsection_enabled[ LOGSECTION_INIT ] =1; //0; by default
  logsection_enabled[ LOGSECTION_CRASH ] = 0;
  logsection_enabled[ LOGSECTION_STEMDOS ] = 0;
  logsection_enabled[ LOGSECTION_IKBD ] = 0;
  logsection_enabled[ LOGSECTION_AGENDA ] = 0;
  logsection_enabled[ LOGSECTION_INTERRUPTS ] = 0;
  logsection_enabled[ LOGSECTION_TRAP ] = 0;
  logsection_enabled[ LOGSECTION_SOUND ] = 0;
  logsection_enabled[ LOGSECTION_VIDEO ] = 0;
  logsection_enabled[ LOGSECTION_BLITTER ] = 0;
  logsection_enabled[ LOGSECTION_MIDI ] = 0;
  logsection_enabled[ LOGSECTION_TRACE ] = 0;
  logsection_enabled[ LOGSECTION_SHUTDOWN ] = 0;
  logsection_enabled[ LOGSECTION_SPEEDLIMIT ] = 0;
  logsection_enabled[ LOGSECTION_CPU ] = 0;
  logsection_enabled[ LOGSECTION_INIFILE ] = 0;
  logsection_enabled[ LOGSECTION_GUI ] = 0;
  logsection_enabled[ LOGSECTION_VIDEO_RENDERING ] = 1;
  logsection_enabled[ LOGSECTION_OPTIONS ] = 1;
  // no PASTI, no DIV
// additions
#if !defined(SSE_BOILER_TRACE_CONTROL)
  logsection_enabled[ LOGSECTION_FDC_BYTES ] = 0;
#endif
  logsection_enabled[ LOGSECTION_IMAGE_INFO ] = 0;
#endif
#endif

#if defined(SSE_DEBUG_TRACE_FILE)
#if defined(SSE_DEBUG_ASSERT)
  IgnoreErrors=0; 
#endif
#if defined(SSE_DEBUG)
  nTrace=0; // trace counter
#endif
#if defined(SSE_VAR_OPT_382)
  SetCurrentDirectory(RunDir.Text);
#else
  SetCurrentDirectory(GetEXEDir().Text);
#endif
#if defined(SSE_TRACE_FOR_RELEASE_390)
  trace_file_pointer=NULL;
#else
  trace_file_pointer=freopen(SSE_TRACE_FILE_NAME, "w", stdout );
#ifdef SSE_DEBUG
  if(!trace_file_pointer)
//    Alert("Couldn't open TRACE file",GetEXEDir().Text,0);
    Alert("Couldn't open TRACE file",RunDir.Text,0);
#endif
 
#if !defined(SSE_BOILER_WIPE_TRACE2)
#ifdef WIN32 // at each start now because of wiping

#ifdef SSE_TRACE_FOR_RELEASE //date is enough, time on start/stop
  char sdate[9];
  _strdate( sdate );
  if(trace_file_pointer)printf("Steem SSE TRACE - %s\n",sdate);
#else
  // http://www.ehow.com/how_2190605_use-date-time-c-program.html
  char sdate[9];
  char stime[9];
  _strdate( sdate );
  _strtime( stime );
  if(trace_file_pointer)printf("Steem SSE TRACE - %s - %s\n",sdate,stime);
#endif
#endif
#endif
#endif
#endif

#if defined(SSE_DEBUG)
#if defined(SSE_IKBD_6301)
  hd6301_trace=&TDebug::TraceLog;
#endif
#if defined(SSE_OSD_DEBUG_MESSAGE)
  TraceOsd("Debug Build"); // implies clean init
#endif
#endif//#if defined(SSE_DEBUG)

#if defined(SSE_BOILER_SHOW_INTERRUPT)
  ZeroMemory(&InterruptTable,sizeof(SInterruptTable));
#endif

#if defined(SSE_BOILER_68030_STACK_FRAME)
  M68030StackFrame=0;
#endif
#if defined(SSE_BOILER_STACK_CHOICE)
  StackDisplayUseOtherSp=0;
#endif
}


TDebug::~TDebug() {
#if defined(SSE_DEBUG_TRACE_FILE)
  if(trace_file_pointer)
  {
    //TRACE("Closing TRACE file...\n"); // silly but it shows it's OK
    TRACE("End\n");
    fclose(trace_file_pointer);
  }
#endif
}

#if defined(SSE_DEBUG) || defined(SSE_OSD_SHOW_TIME)

void TDebug::Vbl(){ 
#if defined(SSE_OSD_CONTROL) && defined(SSE_BOILER_FRAME_INTERRUPTS)
/*  This system so that we only report these once per frame, giving
    convenient info about VBI, HBI, and MFP IRQ.
*/
  if((OSD_MASK1 & OSD_CONTROL_INTERRUPT) && FrameInterrupts)
  {
    char buf1[40]="",buf2[4];
    if(FrameInterrupts&1)
      strcat(buf1,"V");
    if(FrameInterrupts&2)
      strcat(buf1,"H");
    if(FrameMfpIrqs)
    {
      strcat(buf1," MFP ");
      for(int i=15;i>=0;i--)
      {
        if( FrameMfpIrqs&(1<<i) )
        {
          sprintf(buf2,"%d ",i);
          strcat(buf1,buf2);
        }
      }
    } 
    TRACE_OSD(buf1);
  }

#if defined(SSE_BOILER_VIDEO_CONTROL)
  if(VIDEO_CONTROL_MASK & VIDEO_CONTROL_RES)
  {  // no more room on stretched modes
    TRACE_OSD("%dx%d %db",Disp.SurfaceWidth,Disp.SurfaceHeight,BytesPerPixel*8);
  }
#endif

  FrameInterrupts=0;
  FrameMfpIrqs=0;
#endif  

#if defined(SSE_OSD_SHOW_TIME)
  if(OPTION_OSD_TIME)
  {
    DWORD ms=timer-StartingTime;
    DWORD s=ms/1000;
    DWORD h=s/(60*60);
    s=s%(60*60);
    DWORD m=s/60;
    s=s%60;
    TRACE_OSD2("%02d:%02d:%02d",h,m,s);
  }
#endif
#ifdef SSE_DEBUG
  nHbis=0;
#endif
}

#endif//#if defined(SSE_DEBUG)


#if defined(SSE_TRACE_FOR_RELEASE_390)

void TDebug::TraceInit() {
  // make sure we run in Steem dir... (RunDir is set in WinMain)
  EasyStr trace_file=RunDir+SLASH+SSE_TRACE_FILE_NAME; 
  trace_file_pointer=freopen(trace_file, "w", stdout );
  //trace_file_pointer=freopen(SSE_TRACE_FILE_NAME, "w", stdout );
#ifdef SSE_DEBUG
  if(!trace_file_pointer)
//    Alert("Couldn't open TRACE file",GetEXEDir().Text,0);
    Alert("Couldn't open TRACE file",RunDir.Text,0);
#endif
 
#if !defined(SSE_BOILER_WIPE_TRACE2)
#ifdef WIN32 // at each start now because of wiping

#ifdef SSE_TRACE_FOR_RELEASE //date is enough, time on start/stop
  char sdate[9];
  _strdate( sdate );
  if(trace_file_pointer)printf("Steem SSE TRACE - %s\n",sdate);
#else
  // http://www.ehow.com/how_2190605_use-date-time-c-program.html
  char sdate[9];
  char stime[9];
  _strdate( sdate );
  _strtime( stime );
  if(trace_file_pointer)printf("Steem SSE TRACE - %s - %s\n",sdate,stime);
#endif
#endif
#endif
   //TraceGeneralInfos(INIT);
}

#endif


#if defined(SSE_DEBUG_RESET) 

void TDebug::Reset(bool Cold) {
  TRACE_INIT("%s reset\n",(Cold?"Cold":"Warm"));
  //TRACE("%s reset\n",(Cold?"Cold":"Warm"));
  if(Cold)
  {
#if defined(SSE_DEBUG_ASSERT)
    IgnoreErrors=0;
#endif
#if defined(SSE_OSD_SHOW_TIME)
    StartingTime=timeGetTime();
    StoppingTime=0;
#endif
  }
#if defined(SSE_OSD_CONTROL)
  else if((OSD_MASK_CPU&OSD_CONTROL_CPURESET)) 
    TRACE_OSD("RESET");
#endif
}

#endif


#if defined(SSE_DEBUG_TRACE)


void TDebug::Trace(char *fmt, ...){ 
  // Our TRACE facility has no MFC dependency.
  va_list body;	
  va_start(body, fmt);
  ASSERT(trace_buffer);
#if defined(SSE_UNIX)
  int nchars=vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
#if defined(SSE_VS2008_WARNING_390) && !defined(SSE_DEBUG)
  _vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
  int nchars=_vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif
#endif
  va_end(body);	
#ifdef SSE_DEBUG
  if(nchars==-1)
    strcpy(trace_buffer,"TRACE buffer overrun\n");
#endif
#if defined(SSE_DEBUG_TRACE_IDE) && defined(WIN32)
  OutputDebugString(trace_buffer);
#endif

#if defined(SSE_UNIX_TRACE)
  if(!OPTION_TRACE_FILE)  
    fprintf(stderr,trace_buffer);
#endif 
  
#if defined(SSE_DEBUG_TRACE_FILE)
#if defined(DEBUG_BUILD) || defined(SSE_UNIX)
  if(OPTION_TRACE_FILE && trace_file_pointer && trace_buffer)
#endif      
  {
    printf(trace_buffer);
#if defined(SSE_DEBUG)
    nTrace++; 
//#endif

#else
    if(Debug.trace_file_pointer)
      fflush(Debug.trace_file_pointer);
#endif

  }
#if defined(SSE_DEBUG)
  if(TRACE_FILE_REWIND && nTrace>=TRACE_MAX_WRITES && trace_file_pointer)
  {
    nTrace=0;
    rewind(trace_file_pointer); // it doesn't erase
    TRACE("\n============\nREWIND TRACE\n============\n");
  }
#endif
#endif

#if defined(SSE_BOILER_AUTO_FLUSH_TRACE_390)
  if(trace_file_pointer)
    fflush(trace_file_pointer);
#endif


}


#if defined(SSE_DEBUG_START_STOP_INFO)
// A series of TRACE giving precious info at the start & end of emulation
// forward
#ifndef DISABLE_STEMDOS
//extern int stemdos_current_drive;
#include <stemdos.decla.h>
#endif
#if defined(SSE_SOUND_MICROWIRE)
//extern int dma_sound_bass,dma_sound_treble;
#include <psg.decla.h>
#endif


void TDebug::TraceGeneralInfos(int when) {

#if defined(SSE_DEBUG_START_STOP_INFO2)
  if(when==INIT)
  {
#if defined(SSE_BOILER_WIPE_TRACE2) && defined(SSE_DEBUG_TRACE_FILE)
#ifdef WIN32 // at each start now because of wiping
    // http://www.ehow.com/how_2190605_use-date-time-c-program.html
    char sdate[9];
#if !defined(SSE_DEBUG_START_STOP_INFO3)
    char stime[9];
#endif
    _strdate( sdate );
#if defined(SSE_DEBUG_START_STOP_INFO3)
    TRACE("Steem SSE TRACE - %s\n",sdate);
#else
    _strtime( stime );
    TRACE("Steem SSE TRACE - %s -%s\n",sdate,stime);
#endif
#endif
#endif
    TRACE("Build: ");
#if defined(SSE_LE)
    TRACE("LE ");
#endif
#ifdef SSE_BETA
    TRACE("Beta ");
#endif
#ifdef BCC_BUILD
    TRACE("BCC ");
#endif
#ifdef VC_BUILD
    TRACE("VC %d ",_MSC_VER);
#endif
#ifdef SSE_X64_MISC
    TRACE("x64 ");
#endif
#ifdef SSE_BOILER
    TRACE("Boiler ");
#endif
#ifndef SSE_NO_D3D
    TRACE("D3D ");
#endif
#ifndef SSE_NO_DD //v3.8.2
    TRACE("DD ");
#endif
    TRACE("v%d %s %s\n",SSE_VERSION,__DATE__,__TIME__);
    TRACE("%s %d %s %d %s %d %s %d %s %d %s %d\n",UNRAR_DLL,SSEConfig.UnrarDll,
      UNZIP_DLL,SSEConfig.unzipd32Dll,SSE_DISK_CAPS_PLUGIN_FILE,SSEConfig.CapsImgDll,
      PASTI_DLL,SSEConfig.PastiDll,ARCHIVEACCESS_DLL,SSEConfig.ArchiveAccess,
      HD6301_ROM_FILENAME,SSEConfig.Hd6301v1Img);
#ifdef SSE_VID_D3D
    TRACE("Video DX %d D3D %d HWM %d BHM %d BPP%d%d%d\n",TryDD,SSEConfig.Direct3d9,Disp.DrawToVidMem,Disp.BlitHideMouse,SSEConfig.VideoCard8bit,SSEConfig.VideoCard16bit,SSEConfig.VideoCard32bit);
#else
    TRACE("Video DX %d HWM %d BHM %d BPP%d%d%d\n",TryDD,Disp.DrawToVidMem,Disp.BlitHideMouse,SSEConfig.VideoCard8bit,SSEConfig.VideoCard16bit,SSEConfig.VideoCard32bit);
#endif
    TRACE("HP %d ATS %d PWI %d FAFF %d SEOC %d ALSS %d\n",HighPriority,AllowTaskSwitch,PauseWhenInactive,floppy_access_ff,StartEmuOnClick,AutoLoadSnapShot);
  }
  else
#endif
  if(when==START)
  {
#if defined(SSE_OSD_SHOW_TIME)
    if(StoppingTime)
      StartingTime+=timeGetTime()-StoppingTime;
#endif
#if !defined(SSE_DEBUG_START_STOP_INFO2)
#if defined(SSE_BOILER_WIPE_TRACE2) && defined(SSE_DEBUG_TRACE_FILE)
#ifdef WIN32 // at each start now because of wiping
    // http://www.ehow.com/how_2190605_use-date-time-c-program.html
    char sdate[9];
    char stime[9];
    _strdate( sdate );
    _strtime( stime );
    TRACE("Steem SSE TRACE - %s -%s\n",sdate,stime);
#endif
#endif
#endif
#if defined(SSE_DEBUG_START_STOP_INFO3)
    char stime[9];
    _strtime( stime );
    //TRACE(">>> Start Emulation %s <<<\n",stime);
    TRACE("%s Run\n",stime);
#else
    TRACE(">>> Start Emulation <<<\n");
#endif
    //options
    if(cart)
      TRACE("Cart %X ",CART_LPEEK(0));
#if defined(SSE_CPU_MFP_RATIO) && !defined(SSE_GUI_NO_CPU_SPEED)
    if (n_cpu_cycles_per_second>CpuNormalHz)
      TRACE("Speed %d Mhz ",n_cpu_cycles_per_second/1000000);
#endif
#if defined(SSE_STF) && defined(SSE_MMU_WU)
    TRACE("%s%d; ",st_model_name[ST_TYPE],MMU.WS[OPTION_WS]);
#endif
    TRACE("T%X-%d; ",tos_version,ROM_PEEK(0x1D)); //+country
    TRACE("%dK",mem_len/1024);

#if defined(SSE_HACKS)
    if(OPTION_HACKS)
      //TRACE("\nHacks");
      TRACE("; #");
#endif
#if defined(SSE_IKBD_6301) && !defined(SSE_IKBD_6301_NOT_OPTIONAL)
    if(OPTION_C1)
      TRACE("; C1");
#endif
#if !defined(SSE_C2_NOT_OPTIONAL)
    if(OPTION_C2)
      TRACE("; C2");
#endif
#if defined(SSE_CPU_MFP_RATIO) && !defined(SSE_GUI_NO_CPU_SPEED)
    if(n_cpu_cycles_per_second>CpuNormalHz)
      //TRACE("; Speed %d",n_cpu_cycles_per_second);
      TRACE("; ~%d",n_cpu_cycles_per_second);
#if defined(SSE_CPU_MFP_RATIO_OPTION)
    if(OPTION_C2 && OPTION_CPU_CLOCK)
      //TRACE("; Clock %d",CpuCustomHz);
      TRACE("; ~%d",CpuCustomHz); // not ambiguous, different order
#endif
#endif
#if !defined(SSE_YM2149_TABLE_NOT_OPTIONAL)
    if(OPTION_SAMPLED_YM)
      TRACE("; YM");
#endif
#if defined(SSE_DONGLE_PORT) // if bug report = mouse drift...
    if(STPort[3].Type)
      TRACE("; Dongle %d", STPort[3].Type);  
#endif
    if(MONO)
      //TRACE("Monochrome\n");  
      TRACE("; HI");  
#if defined(SSE_VID_BORDERS_GUI_392)
    TRACE("; Border %d",border);
#elif defined(SSE_VID_BORDERS)
    else if(DISPLAY_SIZE)
      //TRACE("Dispay size %d\n", DISPLAY_SIZE);
      TRACE("; Size %d", DISPLAY_SIZE);
#endif
    if(extended_monitor)
      TRACE("; EXT %dx%d",em_width,em_height);
#if defined(SSE_VID_3BUFFER_392)
    if(FullScreen)
      TRACE("; FS 3B%d VS%d",OPTION_3BUFFER_FS,FSDoVsync);
    else
#ifdef SSE_VID_DD
      TRACE("; WM 3B%d VS%d %d-%d,%d-%d,%d",OPTION_3BUFFER_WIN,OPTION_WIN_VSYNC,
        WinSizeForRes[0],draw_win_mode[0],WinSizeForRes[1],draw_win_mode[1],WinSizeForRes[2]); 
#else
      TRACE("; WM VS%d %d-%d,%d-%d,%d",OPTION_WIN_VSYNC,
        WinSizeForRes[0],draw_win_mode[0],WinSizeForRes[1],draw_win_mode[1],WinSizeForRes[2]); 
#endif
#else
    if(FullScreen)
      TRACE("; FS");
    else
      TRACE("; WM %d-%d,%d-%d,%d",WinSizeForRes[0],draw_win_mode[0],WinSizeForRes[1],draw_win_mode[1],WinSizeForRes[2]); 
#endif
    TRACE("\n");
    //disk
    //TRACE("%d drives",num_connected_floppies);
    if(FloppyDrive[0].DiskInDrive())
      //TRACE("; Disk A: %s",FloppyDrive[0].DiskName.c_str()); 
      TRACE("A: %s",FloppyDrive[0].DiskName.c_str()); 
    if(num_connected_floppies==2 && FloppyDrive[1].DiskInDrive())
      //TRACE("; Disk B: %s",FloppyDrive[1].DiskName.c_str()); 
      TRACE("; B: %s",FloppyDrive[1].DiskName.c_str()); 
#if defined(SSE_DRIVE_OBJECT) && !defined(SSE_FLOPPY_ALWAYS_ADAT)
    if(ADAT)
      TRACE("; ADAT");
#endif
#ifndef DISABLE_STEMDOS
    if(!HardDiskMan.DisableHardDrives && stemdos_current_drive) // check
      //TRACE("; HD ON");
      TRACE("; HD");
#endif
#if defined(SSE_ACSI_OPTION)
    if(SSEOption.Acsi)
      //TRACE("; ACSI ON");
      TRACE("; ACSI");
#endif
#if USE_PASTI
    if(pasti_active)
      //TRACE("; Pasti active %s",OPTION_PASTI_JUST_STX?"STX only":"");
      TRACE("; Pasti");
#endif
    TRACE("\n");
//    TRACE(">>> Start Emulation <<<\n");
  }
  else
  {
#if defined(SSE_OSD_SHOW_TIME)
    StoppingTime=timeGetTime();
#endif
    // pick the info you need...
#if defined(SSE_DEBUG_START_STOP_INFO3)
    char stime[9];
    _strtime( stime );
    //TRACE(">>> Stop Emulation %s <<<\n",stime);
    TRACE("%s Stop\n",stime);
#else
    TRACE(">>> Stop Emulation <<<\n");
#endif
//    TRACE("debug var 0: %d 1: %d 2: %d 3: %d 4: %d 5: %d 6: %d 7: %d 8: %d 9: %d\n",debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9);
//    TRACE("HblTiming %d\n",HblTiming);
    // Vectors
    //TRACE("HBL %X VBL %X\n",LPEEK(0x68),LPEEK(0x70));
    //TRACE("Timers A %X B %X C %X D %X\n",LPEEK(0x134),LPEEK(0x120),LPEEK(0x114),LPEEK(0x110));
    //TRACE("ACIA %X FDC %X\n",LPEEK(0x118),LPEEK(0x11C));
    //TRACE("BUS %X ADDR %X ILLEG %X TRACE %X\n",LPEEK(0x8),LPEEK(0xC),LPEEK(0x10),LPEEK(0x24));
    // Misc
#if defined(SSE_SOUND_MICROWIRE___)
    if(dma_sound_bass!=6||dma_sound_treble!=6)
      TRACE("Microwire %d dma bass %X treble %X\n",OPTION_MICROWIRE,dma_sound_bass,dma_sound_treble);
#endif
#if defined(SSE_BOILER_PSEUDO_STACK___)
    for(int i=0;i<PSEUDO_STACK_ELEMENTS;i++)
      TRACE("%X\n",PseudoStack[i]);
#endif
  }
}
#endif


void TDebug::TraceIde(char *fmt, ...){ 
  va_list body;	
  va_start(body, fmt);	
#if defined(SSE_UNIX)
  int nchars=vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
  int nchars=_vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif
  va_end(body);	
  if(nchars==-1)
    strcpy(trace_buffer,"TRACE buffer overrun\n");
#if defined(WIN32)
  OutputDebugString(trace_buffer);
#elif defined(SSE_UNIX)
  printf("%s\n",trace_buffer);  // TODO
#endif
}

#if defined(SSE_DEBUG)

void TDebug::TraceLog(char *fmt, ...) { // static
  
  ASSERT(Debug.LogSection>=0 && Debug.LogSection<100);
  if(Debug.LogSection<NUM_LOGSECTIONS && Debug.logsection_enabled[Debug.LogSection]
#if defined(DEBUG_BUILD)
    ||::logsection_enabled[Debug.LogSection] 
#endif
    )
  {
    // we must replicate the Trace body because passing ... arguments isn't
    // trivial (TODO)
    va_list body;	
    va_start(body, fmt);
    ASSERT(Debug.trace_buffer);
#if defined(SSE_UNIX)
    int nchars=vsnprintf(Debug.trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
    int nchars=_vsnprintf(Debug.trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif

    va_end(body);	

    if(nchars==-1)
      strcpy(Debug.trace_buffer,"TRACE buffer overrun\n");

#if defined(SSE_DEBUG_TRACE_IDE) && defined(WIN32)
    OutputDebugString(Debug.trace_buffer);
#endif

#if defined(SSE_UNIX_TRACE)
    if(!OPTION_TRACE_FILE)
      fprintf(stderr,Debug.trace_buffer);
#endif 
    
#if defined(SSE_DEBUG_TRACE_FILE)
#if defined(DEBUG_BUILD) || defined(SSE_UNIX)
    if(OPTION_TRACE_FILE && Debug.trace_file_pointer && Debug.trace_buffer)
#endif      
      printf(Debug.trace_buffer),Debug.nTrace++; 
    if(TRACE_FILE_REWIND && Debug.nTrace>=TRACE_MAX_WRITES && Debug.trace_file_pointer)
    {
      Debug.nTrace=0;
      rewind(Debug.trace_file_pointer); // it doesn't erase
      TRACE("\n============\nREWIND TRACE\n============\n");
      Debug.TraceGeneralInfos(INIT);
    }
#endif
  }
}

#endif//#if defined(SSE_DEBUG)


#endif


#if defined(SSE_OSD_DEBUG_MESSAGE)
/*  This is a little something that will help a lot.
    Current systems are powerful enough to accomodate a lot of those
    messages, eg when the CPU is in trace mode (Transbeauce 2).
*/  
void TDebug::TraceOsd(char *fmt, ...) {
  va_list body;	
  va_start(body, fmt);	
#if defined(SSE_UNIX)
  int nchars=vsnprintf(m_OsdMessage,OSD_DEBUG_MESSAGE_LENGTH,fmt,body); // check for overrun 
#else
  int nchars=_vsnprintf(m_OsdMessage,OSD_DEBUG_MESSAGE_LENGTH,fmt,body); // check for overrun 
#endif
  va_end(body);	
#if defined(SSE_COMPILER_382)
  if(nchars==-1)
    strcpy(m_OsdMessage,"OSD OVR");
#endif
  strupr(m_OsdMessage); // OSD font is upper-only
  OsdTimer=timer+OSD_DEBUG_MESSAGE_TIME*1000;
}
#endif

#if defined(SSE_BOILER_SHOW_INTERRUPT)
/*  
    It tries to figure out which interrupt if any is being
    executed when the Boiler is stopped.
    MAIN means the CPU isn't executing an interrupt 
    OVF means 'Overflow', interrupts keep coming without corresponding
    RTE. This happens with a lot of programs that directly manipulate the 
    stack. Even the TOS does it on bus error.
    That's why Steem's original interrupt_depth variables quickly goes
    all the way.
    For this reason, a click on the control will reset it if in
    overflow. If not, it will show previous interrupt. So much power!
    TODO: there are ways to check those fake RTS/RTE
*/

void TDebug::ClickInterrupt() {  // user clicked on control
  if(InterruptIdx>=MAX_INTERRUPTS)
    InterruptIdx=0; // user reset
  else if(InterruptIdx>0)
    InterruptIdx--; // show previous "interrupted interrupt"
  ASSERT( InterruptIdx>=0 );
  ReportInterrupt(); // update display
}


#include <limits.h>

void TDebug::RecordInterrupt(char *description, BYTE num) {
  if(InterruptIdx<SHRT_MAX) // bad crash else, eg Nitzer Ebb
    InterruptIdx++;
  ASSERT( InterruptIdx>0 );
  if(InterruptIdx<MAX_INTERRUPTS 
    && (num!=InterruptTable[InterruptIdx-1].num // basic test
    || strcmp(InterruptTable[InterruptIdx-1].description,description)))
  {
    strncpy(InterruptTable[InterruptIdx].description,description,DESCRIPTION_LENGTH);
    InterruptTable[InterruptIdx].num=num;
  }
}


void TDebug::ReportInterrupt() {
  char tmp[20];
  if(!InterruptIdx)
    strcpy(tmp,"MAIN");
  else if(InterruptIdx<MAX_INTERRUPTS)
  {
    ASSERT( InterruptIdx>0 );
    if(InterruptTable[InterruptIdx].num)
      sprintf(tmp,"%d:%s %d",InterruptIdx,
        InterruptTable[InterruptIdx].description,
        InterruptTable[InterruptIdx].num);
    else
      sprintf(tmp,"%d:%s",InterruptIdx,
        InterruptTable[InterruptIdx].description);
  }
  else//very common
  {
    //first check vectors to guess
    if(pc==LPEEK(0x70))
      strcpy(tmp,"VBI?");
    else  if(pc==LPEEK(0x114))
      strcpy(tmp,"Timer C?");
    else  if(pc==LPEEK(0x134))
      strcpy(tmp,"Timer A?");
    else  if(pc==LPEEK(0x118))
      strcpy(tmp,"ACIA?");
// etc.
    else // admit we're lost
      strcpy(tmp,"OVF");
//    for(int i=0;i<MAX_INTERRUPTS;i++)
  //    TRACE("%d:%s %d\n",i,InterruptTable[i].description,InterruptTable[i].num);
  }
  SetWindowText(InterruptReportingZone,tmp);
}


void TDebug::Rte() {
  if(InterruptIdx>0)
    InterruptIdx--;
}

#endif

#if defined(SSE_BOILER_PSEUDO_STACK)

void TDebug::PseudoStackCheck(DWORD return_address) {
  for(int i=0;i<PSEUDO_STACK_ELEMENTS;i++)
    if(PseudoStack[i]==return_address)
      for(int j=i;j<PSEUDO_STACK_ELEMENTS-1;j++)
        PseudoStack[j]=PseudoStack[j+1];
}


DWORD TDebug::PseudoStackPop() {
  DWORD return_address=PseudoStack[0];
  for(int i=0;i<PSEUDO_STACK_ELEMENTS-1;i++)
    PseudoStack[i]=PseudoStack[i+1];
#if defined(SSE_DEBUG_TRACE_IO__) 
    if(TRACE_MASK4 & TRACE_CONTROL_CPU_SP)
      TRACE("pop %X\n",return_address);
#endif
  return return_address;
}


void TDebug::PseudoStackPush(DWORD return_address) {
  for(int i=PSEUDO_STACK_ELEMENTS-1;i>0;i--)
    PseudoStack[i]=PseudoStack[i-1];
  PseudoStack[0]=return_address;
#if defined(SSE_DEBUG_TRACE_IO__) 
    if(TRACE_MASK4 & TRACE_CONTROL_CPU_SP)
      TRACE("push %X\n",return_address);
#endif
}

#endif


#if defined(SSE_BOILER_TRACE_EVENTS)
  // not very smart...
void TDebug::TraceEvent(void* pointer) {
  TRACE("%d ",ACT);
  if(pointer==event_timer_a_timeout)
    TRACE("event_timer_a_timeout");
  else if(pointer==event_timer_b_timeout)
    TRACE("event_timer_b_timeout");
  else if(pointer==event_timer_c_timeout)
    TRACE("event_timer_c_timeout");
  else if(pointer==event_timer_d_timeout)
    TRACE("event_timer_d_timeout");
  else if(pointer==event_timer_b)
    TRACE("event_timer_b");
  else if(pointer==event_scanline)
    TRACE("event_scanline");
  else if(pointer==event_start_vbl)
    TRACE("event_start_vbl");
  else if(pointer==event_vbl_interrupt)
    TRACE("event_vbl_interrupt");
#if defined(SSE_GLUE)
  else if(pointer==event_trigger_vbi)
    TRACE("event_trigger_vbi");
#endif
#if defined(SSE_WD1772_EMU)
  else if(pointer==event_wd1772)
    TRACE("event_wd1772");
  else if(pointer==event_driveA_ip)
    TRACE("event_driveA_ip");
  else if(pointer==event_driveB_ip)
    TRACE("event_driveB_ip");
#endif
#if defined(SSE_ACIA_EVENT)
  else if(pointer==event_acia)
    TRACE("event_acia");
#endif
#if defined(SSE_INT_MFP_LATCH_DELAY)
  else if(pointer==event_mfp_write)
    TRACE("event_mfp_write");
#endif
#if defined(SSE_DISK_PASTI)
  else if(pointer==event_pasti_update)
    TRACE("event_pasti_update");
#endif
    TRACE(" (%d)\n",ACT-time_of_next_event);
}
#endif

#endif//#if defined(SSE_DEBUG) 
