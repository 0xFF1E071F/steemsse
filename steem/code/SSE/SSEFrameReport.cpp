#include "SSE.h"

#if defined(SSE_BOILER_FRAME_REPORT)

#include "../pch.h"
#include <conditions.h>
#include <easystr.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <run.decla.h>
#include <emulator.decla.h>
#include <cpu.decla.h>

#include "SSEFrameReport.h"
#include "SSEMMU.h"
#include "SSEOption.h"
#include "SSESTF.h"
#include "SSEVideo.h"
#include "SSEGlue.h"
#include "SSEShifter.h"


TFrameEvents FrameEvents;  // singleton


TFrameEvents::TFrameEvents() {
  Init();
} 


void TFrameEvents::Add(int scanline, int cycle, char type, int value) {
  m_nEvents++;  // starting from 0 each VBL, event 0 is dummy 
/*  3.8.0 fix >= not >, no version switch, it's a too bad bug.
    DSOTS was broken only in VC boiler builds, both as STE and STF,with "glue 
    timings" because Glue.ScanlineTiming[TGlue::GLU_DE_ON][TGlue::FREQ_50]
    (and not FREQ_60) was written over: 0 instead of 40/56!
*/
  ASSERT(m_nEvents>0&&m_nEvents<MAX_EVENTS);
#if !defined(SSE_VS2008_WARNING_382)
  int total_cycles= (shifter_freq_at_start_of_vbl==50) ?512:508;// Shifter.CurrentScanline.Cycles;//512;
#endif
  m_FrameEvent[m_nEvents].Add(scanline, cycle, type, value);
}



#if defined(SSE_BOILER_REPORT_SDP_ON_CLICK)
MEM_ADDRESS TFrameEvents::GetSDP(int x,int guessed_scan_y) {
  MEM_ADDRESS sdp=NULL;
  int i;
#if defined(SSE_BOILER_FRAME_REPORT_MASK) //skip if not recorded
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SDP_LINES) 
#endif
  for(i=1; i<=MAX_EVENTS ;i++)
  {
    if(m_FrameEvent[i].Scanline==guessed_scan_y)
    {
      if(m_FrameEvent[i].Type=='@' && !sdp)
        sdp=m_FrameEvent[i].Value<<16; // fails if 1st word=0 (Leavin' Teramis)
      else if(m_FrameEvent[i].Type=='@' && sdp)
      {
        sdp|=m_FrameEvent[i].Value; // at start of line
        // look for tricks
        int trick=0;
        while( i<=MAX_EVENTS && m_FrameEvent[i].Scanline==guessed_scan_y
          && m_FrameEvent[i].Type!='T')
          i++;
        if(m_FrameEvent[i].Type=='T')
          trick=m_FrameEvent[i].Value;
#if defined(SSE_SHIFTER)
        // this could already help the precision but
        // it isn't meant to be complete nor accurate!
        if(trick&TRICK_LINE_PLUS_26)
          x+=52;
        else if(trick&TRICK_LINE_PLUS_24)
          x+=48;
        else if(trick&TRICK_0BYTE_LINE)
          x=-1;
        if(x>0)
          sdp+=x/2;
#endif
        sdp&=~1; // looks more serious...
        break;
      }

    }
  }//nxt
  return sdp;
}
#endif

#if defined(SSE_BOILER_XBIOS2)

DWORD TFrameEvents::GetShifterTricks(int y) {
  DWORD tricks=0;
  for(int i=1; i<=MAX_EVENTS && m_FrameEvent[i].Scanline<=y;i++)
  {
    if(m_FrameEvent[i].Scanline==y)
    {
      if(m_FrameEvent[i].Type=='T')
      {
        tricks=m_FrameEvent[i].Value;
        break;
      }
    }
  }//nxt
  return tricks;
}

#endif


void TFrameEvents::Init() {
  m_nEvents=m_nReports=TriggerReport=nVbl=0;
}


int TFrameEvents::Report() {
  //TRACE("Saving frame events...\n");
  FILE* fp;
  fp=fopen(FRAME_REPORT_FILENAME,"w"); // unique file name
  ASSERT(fp);
  if(fp)
  {
#if defined(WIN32)
    char sdate[9];
    char stime[9];
    _strdate( sdate );
    _strtime( stime );
    fprintf(fp,"Steem frame report - %s -%s \n%s WS%d\n",sdate,stime,
#if defined(SSE_STF)
      st_model_name[ST_TYPE]
#else
      "STE"
#endif
#if defined(SSE_MMU_WU)
      ,MMU.WS[OPTION_WS]);
#else
    ,0);
#endif
#else
    fprintf(fp,"Steem frame report - %s WS%d\n",
      st_model_name[ST_TYPE],MMU.WS[OPTION_WS]);
#endif
    if(FloppyDrive[0].DiskInDrive())
      fprintf(fp,"Disk A: %s",FloppyDrive[0].DiskName.c_str()); 
    int i,j;
    for(i=1,j=-1;i<=m_nEvents;i++)
    {
      if(m_FrameEvent[i].Scanline!=j)
      {
        j=m_FrameEvent[i].Scanline;
        fprintf(fp,"\n%03d -",j);
      }
      if(m_FrameEvent[i].Type=='L' || m_FrameEvent[i].Type=='a'
        || m_FrameEvent[i].Type=='#') // decimal
        fprintf(fp," %03d:%c%04d",m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
      else  // hexa
        fprintf(fp," %03d:%c%04X",m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
    }//nxt
#if 1 //wow, only saw this thx to VS2015! - was debug-only
    fprintf(fp, "\n--"); // so we know it was OK
    fclose(fp);
#else
    fclose(fp);
    fprintf(fp,"\n--"); // so we know it was OK//sure that's OK :)
#endif
  }
  m_nReports++;
  return m_nReports;
}


void TFrameEvents::ReportLine() {
  // current line
  // look back
  int i=m_nEvents;
  while(i>1 && m_FrameEvent[i].Scanline==scan_y)
    i--;
  if(m_FrameEvent[i].Cycle>=508)
    i++; // former line
  TRACE("Y%d C%d ",scan_y,LINECYCLES);
  for(;i<=m_nEvents;i++)
  {
    if(m_FrameEvent[i].Type=='L' //|| m_FrameEvent[i].Type=='C' 
        || m_FrameEvent[i].Type=='#') // decimal
        TRACE(" %d %03d:%c%04d",m_FrameEvent[i].Scanline,m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
      else  // hexa
        TRACE(" %d %03d:%c%04X",m_FrameEvent[i].Scanline,m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
  }
  TRACE("\n");
}


int TFrameEvents::Vbl() {

  if(Debug.ShifterTricks)
  {
#undef LOGSECTION
#define LOGSECTION LOGSECTION_VIDEO
#if defined(SSE_SHIFTER_REPORT_VBL_TRICKS)
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK1 & TRACE_CONTROL_SUMMARY)
      TRACE_LOG("VBL %d Shifter tricks %X\n",nVbl,Debug.ShifterTricks);
#endif
//#undef LOGSECTION
#endif  

#ifdef SSE_BOILER
#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK2 & OSD_CONTROL_SHIFTERTRICKS)
#else
    if(TRACE_ENABLED(LOGSECTION_VIDEO)) 
#endif
      TRACE_OSD("T%X",Debug.ShifterTricks);
#endif
#undef LOGSECTION//???
    Debug.ShifterTricks=0;
  }

  int rv= TriggerReport;
  if(TriggerReport==2 && m_nEvents)
    TriggerReport--;
  else if(TriggerReport==1 && m_nEvents)
  {
    Report();
    TriggerReport=FALSE;
  }
  nVbl++;
  m_nEvents=0;
  return rv;
}

#endif//#if defined(SSE_BOILER_FRAME_REPORT)

