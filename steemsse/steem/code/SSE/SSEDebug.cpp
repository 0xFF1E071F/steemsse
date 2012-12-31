#include "SSEDebug.h"

#if defined(SS_DEBUG) 

#include <stdio.h>
#include <stdarg.h>

int debug0=0,debug1=0,debug2=0,debug3=0,debug4=0,
  debug5=0,debug6=0,debug7=0,debug8=0,debug9=0;

TDebug Debug; // singleton

bool logsection_enabled_b[100];

char trace_buffer[1024]; // permanent rather that on the stack

TDebug::TDebug() {

  ZeroMemory(logsection_enabled_b,100*sizeof(bool)); // 100> our need
  // compile time wanted traces (overrides boiler if present)
  logsection_enabled_b[ LOGSECTION_ALWAYS ] = 1;
  logsection_enabled_b[ LOGSECTION_FDC ] = 0;
  logsection_enabled_b[ LOGSECTION_IO ] = 0;
  logsection_enabled_b[ LOGSECTION_MFP_TIMERS ] = 0;
  logsection_enabled_b[ LOGSECTION_INIT ] = 0;
  logsection_enabled_b[ LOGSECTION_CRASH ] = 0;
  logsection_enabled_b[ LOGSECTION_STEMDOS ] = 0;
  logsection_enabled_b[ LOGSECTION_IKBD ] = 0;
  logsection_enabled_b[ LOGSECTION_AGENDA ] = 0;
  logsection_enabled_b[ LOGSECTION_INTERRUPTS ] = 0;
  logsection_enabled_b[ LOGSECTION_TRAP ] = 0;
  logsection_enabled_b[ LOGSECTION_SOUND ] = 0;
  logsection_enabled_b[ LOGSECTION_VIDEO ] = 0;
  logsection_enabled_b[ LOGSECTION_BLITTER ] = 0;
  logsection_enabled_b[ LOGSECTION_MIDI ] = 0;
  logsection_enabled_b[ LOGSECTION_TRACE ] = 0;
  logsection_enabled_b[ LOGSECTION_SHUTDOWN ] = 0;
  logsection_enabled_b[ LOGSECTION_SPEEDLIMIT ] = 0;
  logsection_enabled_b[ LOGSECTION_CPU ] = 0;
  logsection_enabled_b[ LOGSECTION_INIFILE ] = 0;
  logsection_enabled_b[ LOGSECTION_GUI ] = 0;
  logsection_enabled_b[ LOGSECTION_DIV ] = 0;
  logsection_enabled_b[ LOGSECTION_PASTI ] = 0;
  // TODO more options in boiler?
  logsection_enabled_b[ LOGSECTION_FDC_BYTES ] = 0;
  logsection_enabled_b[ LOGSECTION_IPF_LOCK_INFO ] = 0;
  logsection_enabled_b[ LOGSECTION_IMAGE_INFO ] = 1;

#if defined(SS_DEBUG_TRACE_FILE)
  ReportBreakpoints=TRUE; // disabled when clicking cancel in the box
  nTrace=0; // trace counter
  trace_file_pointer=fopen(SS_TRACE_FILE_NAME,"w"); // only one file name...
  ASSERT(trace_file_pointer);
  TRACE("This is a Steem SSE TRACE file\n");
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

#if defined(SS_DEBUG_TRACE_FILE)

void TDebug::TraceToFile(char *fmt, ...){ 
  if(USE_TRACE_FILE && trace_file_pointer)  
  {
    va_list body;	
    va_start(body, fmt);	
    VERIFY( vsprintf(trace_buffer, fmt, body)<1024 );
    va_end(body);	
    fprintf(trace_file_pointer,trace_buffer);
    nTrace++; 
    if(TRACE_FILE_REWIND && nTrace>=TRACE_MAX_WRITES)
    {
      nTrace=0;
      rewind(trace_file_pointer); // it doesn't erase
      TRACE("\n--------------------------\nREWIND TRACE\n--------------------------\n");
    }
  }
}

#endif


#if defined(SS_DEBUG_START_STOP_INFO)
// A series of TRACE giving precious info at the start & end of emulation
// forward
extern int stemdos_current_drive;
#if defined(SS_SOUND_MICROWIRE)
extern int dma_sound_bass,dma_sound_treble;
#endif

int ReportGeneralInfos(int when) {
  
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
      TRACE(" (IPF emu)");
#endif
    }
    if(num_connected_floppies==2 && FloppyDrive[1].DiskInDrive())
    {
      TRACE("; Disk B: %s",FloppyDrive[1].DiskName.c_str()); 
#if defined(SS_IPF)
    if(Caps.IsIpf(1)) 
      TRACE(" (IPF emu)");
#endif
    }
    if(!HardDiskMan.DisableHardDrives && stemdos_current_drive) // check
      TRACE("; HD ON");
    TRACE("; ADAT %d",ADAT);
#if USE_PASTI
    TRACE("; Pasti %d",pasti_active);
#endif
#if defined(SS_HACKS)
    TRACE("\nHacks %d",SSE_HACKS_ON);
#endif
#if defined(SS_IKBD_6301)
    TRACE("; HD6301 true emu %d",HD6301EMU_ON);
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
  return TRUE;
}
#endif


#if defined(_DEBUG) && !defined(SS_DEBUG_TRACE_FILE)
// TRACE for VC IDE
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
// Our TRACE facility has no MFC dependency.
// The same for ASSERT. Could be easier?
void my_trace(char *fmt, ...){
  va_list body;	
  va_start(body, fmt);	
  vsprintf(trace_buffer, fmt, body);	
  va_end(body);	
  OutputDebugString(trace_buffer);
}

#endif

#endif