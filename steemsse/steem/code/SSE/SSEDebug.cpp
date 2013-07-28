#include "SSEDebug.h"

#if defined(SS_DEBUG) 

int debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;

#if defined(SS_IKBD_6301)
#if defined(SS_UNIX)
extern "C" void (*hd6301_trace)(char *fmt, ...);
#else
extern "C" void (_stdcall *hd6301_trace)(char *fmt, ...);
#endif
#endif


TDebug Debug; // singleton

TDebug::TDebug() {

  ZeroMemory(&debug0,10*sizeof(int));  

#if defined(SS_DEBUG_LOG_OPTIONS)
  //  We must init those variables for the builds without the boiler
  ZeroMemory(logsection_enabled,100*sizeof(bool)); // 100> our need
  logsection_enabled[ LOGSECTION_ALWAYS ] = 1;
#if defined(_DEBUG) && !defined(DEBUG_BUILD) // VC6 IDE debug no boiler
  //TODO move somehow to SSE.H
  logsection_enabled[ LOGSECTION_FDC ] = 0;
  logsection_enabled[ LOGSECTION_IO ] = 0;
  logsection_enabled[ LOGSECTION_MFP_TIMERS ] = 0;
  logsection_enabled[ LOGSECTION_INIT ] =0;
  logsection_enabled[ LOGSECTION_CRASH ] = 0;
  logsection_enabled[ LOGSECTION_STEMDOS ] = 0;
  logsection_enabled[ LOGSECTION_IKBD ] = 1;
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
  // no PASTI, no DIV
// additions
  logsection_enabled[ LOGSECTION_FDC_BYTES ] = 0;
  logsection_enabled[ LOGSECTION_IPF_LOCK_INFO ] = 0; //remove option
  logsection_enabled[ LOGSECTION_IMAGE_INFO ] = 1;
#endif
  logsection_enabled[ LOGSECTION_OPTIONS ] = 1; // no boiler control
#endif

#if defined(SS_DEBUG_TRACE_FILE)
  IgnoreErrors=0; 
  nTrace=0; // trace counter
  trace_file_pointer=freopen(SS_TRACE_FILE_NAME, "w", stdout );
  ASSERT(trace_file_pointer);
  // http://www.ehow.com/how_2190605_use-date-time-c-program.html
  char sdate[9];
  char stime[9];
  _strdate( sdate );
  _strtime( stime );
  printf("Steem SSE TRACE - %s -%s\n",sdate,stime);
#endif

#if defined(SS_IKBD_6301)
  hd6301_trace=&TDebug::TraceLog;
#endif

#if defined(SS_CPU_PREFETCH_DETECT_IRC_TRICK)
  CpuPrefetchDiffDetected=false;
#endif

#if defined(SS_CPU_TRACE_DETECT)
  CpuTraceDetected=false;
#endif
}


TDebug::~TDebug() {
#if defined(SS_DEBUG_TRACE_FILE)
  if(trace_file_pointer)
  {
    TRACE("Closing TRACE file...\n"); 
    fclose(trace_file_pointer);
  }
#endif
}



#if defined(SS_DEBUG_TRACE)
//note: #define TRACE Debug.Trace

#include <stdio.h>
#include <stdarg.h>
#if defined(SS_DEBUG_TRACE_IDE) && defined(WIN32)
#include <windows.h>
#endif

void TDebug::Trace(char *fmt, ...){ 
  // Our TRACE facility has no MFC dependency.
  va_list body;	
  va_start(body, fmt);	
#if defined(SS_UNIX)
  int nchars=vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
  int nchars=_vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif
  va_end(body);	
  if(nchars==-1)
    strcpy(trace_buffer,"TRACE buffer overrun\n");

#if defined(SS_DEBUG_TRACE_IDE) && defined(WIN32)
  OutputDebugString(trace_buffer);
#endif

#if defined(SS_UNIX_TRACE)
  if(!USE_TRACE_FILE)  
    fprintf(stderr,trace_buffer);
#endif 
  
#if defined(SS_DEBUG_TRACE_FILE)
#if defined(DEBUG_BUILD) || defined(SS_UNIX)
  if(USE_TRACE_FILE)
#endif      
    printf(trace_buffer),nTrace++; 
  if(TRACE_FILE_REWIND && nTrace>=TRACE_MAX_WRITES)
  {
    nTrace=0;
    rewind(trace_file_pointer); // it doesn't erase
    TRACE("\n============\nREWIND TRACE\n============\n");
  }
#endif

}

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
#if defined(SS_UNIX)
    int nchars=vsnprintf(Debug.trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
    int nchars=_vsnprintf(Debug.trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif

    va_end(body);	

    if(nchars==-1)
      strcpy(Debug.trace_buffer,"TRACE buffer overrun\n");

#if defined(SS_DEBUG_TRACE_IDE) && defined(WIN32)
    OutputDebugString(Debug.trace_buffer);
#endif

#if defined(SS_UNIX_TRACE)
    if(!USE_TRACE_FILE)
      fprintf(stderr,Debug.trace_buffer);
#endif 
    
#if defined(SS_DEBUG_TRACE_FILE)
#if defined(DEBUG_BUILD) || defined(SS_UNIX)
    if(USE_TRACE_FILE)
#endif      
      printf(Debug.trace_buffer),Debug.nTrace++; 
    if(TRACE_FILE_REWIND && Debug.nTrace>=TRACE_MAX_WRITES)
    {
      Debug.nTrace=0;
      rewind(Debug.trace_file_pointer); // it doesn't erase
      TRACE("\n============\nREWIND TRACE\n============\n");
    }
#endif
  }
}

#endif


#if defined(SS_DEBUG_START_STOP_INFO)
// A series of TRACE giving precious info at the start & end of emulation
// forward
#if !defined(SS_STRUCTURE_BIG_FORWARD)

extern int stemdos_current_drive;
#if defined(SS_SOUND_MICROWIRE)
extern int dma_sound_bass,dma_sound_treble;
#endif
#endif

void TDebug::ReportGeneralInfos(int when) {
  
  if(when==START)
  {
    TRACE(">>> Start Emulation <<<\n");
#if defined(SS_STF)
    TRACE("ST model: %d; ",ST_TYPE);
#endif
    TRACE("TOS %X; ",tos_version);
    TRACE("RAM %dK; ",mem_len/1024);
    TRACE("Display %s", MONO ? "Monochrome" : "Colour");
#if defined(SS_VID_BORDERS)
    if(!MONO)
      TRACE(", Size %d", DISPLAY_SIZE);
#endif
    TRACE("\n");
    TRACE("%d drives",num_connected_floppies);
    if(FloppyDrive[0].DiskInDrive())
    {
      TRACE("; Disk A: %s",FloppyDrive[0].DiskName.c_str()); 
#if defined(SS_IPF)
    if(Caps.IsIpf(0)) 
      TRACE(" (IPF)");
#endif
    }
    if(num_connected_floppies==2 && FloppyDrive[1].DiskInDrive())
    {
      TRACE("; Disk B: %s",FloppyDrive[1].DiskName.c_str()); 
#if defined(SS_IPF)
    if(Caps.IsIpf(1)) 
      TRACE(" (IPF)");
#endif
    }
    if(!HardDiskMan.DisableHardDrives && stemdos_current_drive) // check
      TRACE("; HD ON");
#if defined(SS_FDC)
    TRACE("; ADAT %d",ADAT);
#endif
#if USE_PASTI
    TRACE("; Pasti %d",pasti_active);
#if defined(SS_PASTI_ONLY_STX)
    TRACE(" STX only %d",PASTI_JUST_STX);
#endif
#endif
#if defined(SS_HACKS)
    TRACE("\nHacks %d",SSE_HACKS_ON);
#endif
#if defined(SS_IKBD_6301)
    TRACE("; HD6301 %d",HD6301EMU_ON);
#endif
    TRACE("\n");
  }
  else
  {    // pick the info you need...
    TRACE(">>> Stop Emulation <<<\n");
//    TRACE("debug var 0: %d 1: %d 2: %d 3: %d 4: %d 5: %d 6: %d 7: %d 8: %d 9: %d\n",debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9);
//    TRACE("HblTiming %d\n",HblTiming);
    // Vectors
    TRACE("HBL %X VBL %X\n",LPEEK(0x68),LPEEK(0x70));
    TRACE("Timers A %X B %X C %X D %X\n",LPEEK(0x134),LPEEK(0x120),LPEEK(0x114),LPEEK(0x110));
    TRACE("ACIA IKBD %X\n",LPEEK(0x118));
    // Misc
#if defined(SS_SOUND_MICROWIRE)
    if(dma_sound_bass!=6||dma_sound_treble!=6)
      TRACE("Microwire %d dma bass %X treble %X\n",MICROWIRE_ON,dma_sound_bass,dma_sound_treble);
#endif
  }
}
#endif

#endif