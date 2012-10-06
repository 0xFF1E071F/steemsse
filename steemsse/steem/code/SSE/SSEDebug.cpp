#include "SSEDebug.h"

#if defined(SS_DEBUG) 

#include <stdio.h>
#include <stdarg.h>

int debug0=0,debug1=0,debug2=0,debug3=0,debug4=0,
  debug5=0,debug6=0,debug7=0,debug8=0,debug9=0;

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
      TRACE(", Size %d", SSEOption.BorderSize);
#endif
    TRACE("\n");
    TRACE("%d drives",num_connected_floppies);
    if(FloppyDrive[0].DiskInDrive())
      TRACE("; Disk A: %s",FloppyDrive[0].DiskName.c_str()); 
    if(num_connected_floppies==2 && FloppyDrive[1].DiskInDrive())
      TRACE("; Disk B: %s",FloppyDrive[1].DiskName.c_str()); 
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
#if defined(SS_SOUND_MICROWIRE) && defined(SS_SOUND_TRACE)
    TRACE("Microwire %d dma bass %X treble %X\n",SSEOption.STEMicrowire,dma_sound_bass,dma_sound_treble);
#endif
  }
  return TRUE;
}
#endif


#if defined(_DEBUG)
// TRACE for VC IDE
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
// Our TRACE facility has no MFC dependency.
// The same for ASSERT. Could be easier?
void my_trace(char *fmt, ...){
  char out[1024];	
  va_list body;	
  va_start(body, fmt);	
  vsprintf(out, fmt, body);	
  va_end(body);	
  OutputDebugString(out);
}
#endif


TDebug::TDebug() {
  trace_file_pointer=NULL; // classic C file handling
  ReportBreakpoints=TRUE; // disabled when clicking cancel in the box
  nTrace=0; // trace counter
#if !defined(_DEBUG) && defined(DEBUG_BUILD)
  trace_file_pointer=fopen("TRACE.txt","w"); // only one file name...
  TRACE("This is a Steem SSE TRACE file\n");
  ASSERT(trace_file_pointer);
#endif
}


TDebug::~TDebug() {
  if(trace_file_pointer)
  {
    TRACE("Closing TRACE file...\n");
    fclose(trace_file_pointer);
  }
}

#if !defined(_DEBUG) && (defined(DEBUG_BUILD) || defined(SS_UNIX_TRACE))
void TDebug::TraceToFile(char *fmt, ...){ 
  if(SSEOption.OutputTraceToFile
    && (!SSEOption.TraceFileLimit||nTrace <TRACE_MAX_WRITES)
    && trace_file_pointer)  
  {
    char out[1024];	
    va_list body;	
    va_start(body, fmt);	
    vsprintf(out, fmt, body);
    va_end(body);	
    fprintf(trace_file_pointer,out);
    nTrace++; 
  }
}
#endif

TDebug SSDebug; // singleton

#endif