#include "SSEDebug.h"
#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#include "SSEMMU.h"
#endif
#if defined(SSE_DEBUG) 

#if defined(UNIX) || defined(BCC_BUILD)
#include "../pch.h" 
#pragma hdrstop 
#else
//#include <windows.h>
#include <ddraw.h>
#endif

#if defined(SSE_STRUCTURE_SSEDEBUG_OBJ)
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
#include "SSEFloppy.h"
#include <display.decla.h>
#endif
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#include "SSEMMU.h"
#endif

int debug0,debug1=0,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;

#if defined(SSE_IKBD_6301)
#if defined(SSE_UNIX)
extern "C" void (*hd6301_trace)(char *fmt, ...);
#else
extern "C" void (_stdcall *hd6301_trace)(char *fmt, ...);
#endif
#endif

#ifdef __cplusplus
//extern "C" 
#endif
TDebug Debug; // singleton

TDebug::TDebug() {

  ZeroMemory(&debug0,10*sizeof(int));  

#if defined(SSE_DEBUG_LOG_OPTIONS)
  //  We must init those variables for the builds without the boiler
  ZeroMemory(logsection_enabled,100*sizeof(bool)); // 100> our need
  logsection_enabled[ LOGSECTION_ALWAYS ] = 1;
#if defined(_DEBUG) && !defined(DEBUG_BUILD) // VC6 IDE debug no boiler
  logsection_enabled[ LOGSECTION_FDC ] = 0;
  logsection_enabled[ LOGSECTION_IO ] = 0;
  logsection_enabled[ LOGSECTION_MFP_TIMERS ] = 0;
  logsection_enabled[ LOGSECTION_INIT ] =0;
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
  // no PASTI, no DIV
// additions
#if !defined(SSE_BOILER_TRACE_CONTROL)
  logsection_enabled[ LOGSECTION_FDC_BYTES ] = 0;
  //logsection_enabled[ LOGSECTION_IPF_LOCK_INFO ] = 0; //remove option
#endif
  logsection_enabled[ LOGSECTION_IMAGE_INFO ] = 0;
#endif
  logsection_enabled[ LOGSECTION_OPTIONS ] = 1; // no boiler control
#endif

#if defined(SSE_DEBUG_TRACE_FILE)
  IgnoreErrors=0; 
  nTrace=0; // trace counter
  SetCurrentDirectory(GetEXEDir().Text);
  trace_file_pointer=freopen(SSE_TRACE_FILE_NAME, "w", stdout );
  if(!trace_file_pointer)
    Alert("Couldn't open TRACE file",GetEXEDir().Text,0);
  //if(!trace_file_pointer) TRACE_IDE("%s\n",WriteDir.Text);
#if !defined(SSE_BOILER_WIPE_TRACE2)
#ifdef WIN32 // at each start now because of wiping
  // http://www.ehow.com/how_2190605_use-date-time-c-program.html
  char sdate[9];
  char stime[9];
  _strdate( sdate );
  _strtime( stime );
  if(trace_file_pointer)printf("Steem SSE TRACE - %s - %s\n",sdate,stime);
#endif
#endif
#endif

#if defined(SSE_IKBD_6301)
  hd6301_trace=&TDebug::TraceLog;
#endif

#if defined(SSE_OSD_DEBUG_MESSAGE)
  TraceOsd("Debug Build"); // implies clean init
#endif

#if defined(SSE_BOILER_SHOW_INTERRUPT)
  ZeroMemory(&InterruptTable,sizeof(SInterruptTable));
#endif

#if defined(SSE_BOILER_STACK_68030_FRAME)
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
    TRACE("Closing TRACE file...\n"); // silly but it shows it's OK
    fclose(trace_file_pointer);
  }
#endif
}


void TDebug::Vbl(){ 
#if defined(SSE_BOILER_FRAME_INTERRUPTS)
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
   nHbis=0;
}



#if defined(SSE_DEBUG_TRACE)
//note: #define TRACE Debug.Trace

#include <stdio.h>
#include <stdarg.h>
#if defined(SSE_DEBUG_TRACE_IDE) && defined(WIN32)
#include <windows.h>
#endif

void TDebug::Trace(char *fmt, ...){ 
  // Our TRACE facility has no MFC dependency.
  va_list body;	
  va_start(body, fmt);
  ASSERT(trace_buffer);
#if defined(SSE_UNIX)
  int nchars=vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
  int nchars=_vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif
  va_end(body);	
  if(nchars==-1)
    strcpy(trace_buffer,"TRACE buffer overrun\n");

#if defined(SSE_DEBUG_TRACE_IDE) && defined(WIN32)
  OutputDebugString(trace_buffer);
#endif

#if defined(SSE_UNIX_TRACE)
  if(!USE_TRACE_FILE)  
    fprintf(stderr,trace_buffer);
#endif 
  
#if defined(SSE_DEBUG_TRACE_FILE)
#if defined(DEBUG_BUILD) || defined(SSE_UNIX)
  if(USE_TRACE_FILE && trace_file_pointer && trace_buffer)
#endif      
    printf(trace_buffer),nTrace++; 
  if(TRACE_FILE_REWIND && nTrace>=TRACE_MAX_WRITES && trace_file_pointer)
  {
    nTrace=0;
    rewind(trace_file_pointer); // it doesn't erase
    TRACE("\n============\nREWIND TRACE\n============\n");
  }
#endif

}


#if defined(SSE_DEBUG_START_STOP_INFO)
// A series of TRACE giving precious info at the start & end of emulation
// forward
#ifndef DISABLE_STEMDOS
extern int stemdos_current_drive;
#endif
#if defined(SSE_SOUND_MICROWIRE)
extern int dma_sound_bass,dma_sound_treble;
#endif


void TDebug::TraceGeneralInfos(int when) {
  
  if(when==START)
  {

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

    TRACE(">>> Start Emulation <<<\n");
#if defined(SSE_STF)
    TRACE("%s; ",st_model_name[ST_TYPE]);
#endif
    TRACE("T%X; ",tos_version);
    TRACE("%dK; ",mem_len/1024);
    if(MONO)
      TRACE("Monochrome");  
#if defined(SSE_VID_BORDERS)
    else if(DISPLAY_SIZE)
      TRACE("Size %d", DISPLAY_SIZE);
#endif
    TRACE("\n");
    TRACE("%d drives",num_connected_floppies);
    if(FloppyDrive[0].DiskInDrive())
      TRACE("; Disk A: %s",FloppyDrive[0].DiskName.c_str()); 
    if(num_connected_floppies==2 && FloppyDrive[1].DiskInDrive())
      TRACE("; Disk B: %s",FloppyDrive[1].DiskName.c_str()); 
#ifndef DISABLE_STEMDOS
    if(!HardDiskMan.DisableHardDrives && stemdos_current_drive) // check
      TRACE("; HD ON");
#endif
#if defined(SSE_FDC)
    if(ADAT)
      TRACE("; ADAT");
#endif
#if USE_PASTI
    if(pasti_active)
    {
      TRACE("; Pasti %d",pasti_active);
#if defined(SSE_PASTI_ONLY_STX)
      if(PASTI_JUST_STX)
        TRACE(" STX only");
#endif
    }
#endif
#if defined(SSE_HACKS)
    if(SSE_HACKS_ON)
      TRACE("\nHacks");
#endif
#if defined(SSE_IKBD_6301)
    if(HD6301EMU_ON)
      TRACE("; HD6301");
#endif

#if defined(SSE_MMU_WU_DL)
    if(WAKE_UP_STATE)
      TRACE("; WS%d",MMU.WS[WAKE_UP_STATE]);
#elif defined(SSE_MMU_WAKE_UP)
    TRACE("; WU%d",WAKE_UP_STATE);
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
//    TRACE("ACIA IKBD %X\n",LPEEK(0x118));
    // Misc
#if defined(SSE_SOUND_MICROWIRE___)
    if(dma_sound_bass!=6||dma_sound_treble!=6)
      TRACE("Microwire %d dma bass %X treble %X\n",MICROWIRE_ON,dma_sound_bass,dma_sound_treble);
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
    if(!USE_TRACE_FILE)
      fprintf(stderr,Debug.trace_buffer);
#endif 
    
#if defined(SSE_DEBUG_TRACE_FILE)
#if defined(DEBUG_BUILD) || defined(SSE_UNIX)
    if(USE_TRACE_FILE && Debug.trace_file_pointer && Debug.trace_buffer)
#endif      
      printf(Debug.trace_buffer),Debug.nTrace++; 
    if(TRACE_FILE_REWIND && Debug.nTrace>=TRACE_MAX_WRITES && Debug.trace_file_pointer)
    {
      Debug.nTrace=0;
      rewind(Debug.trace_file_pointer); // it doesn't erase
      TRACE("\n============\nREWIND TRACE\n============\n");
    }
#endif
  }
}

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

#endif//#if defined(SSE_DEBUG) 
