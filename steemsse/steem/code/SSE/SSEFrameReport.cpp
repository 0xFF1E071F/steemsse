#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_STRUCTURE_SSEFRAMEREPORT_OBJ)
#include "../pch.h"
#include <conditions.h>
#include <easystr.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <run.decla.h>
#include "SSEFrameReport.h"
#include "SSEMMU.h"
#include "SSEOption.h"
#include "SSEShifter.h"
#include "SSESTF.h"
#endif



#if defined(SSE_DEBUG_FRAME_REPORT)


TFrameEvents FrameEvents;  // singleton


TFrameEvents::TFrameEvents() {
  Init();
} 


#if defined(SSE_DEBUG_REPORT_SDP_ON_CLICK)
MEM_ADDRESS TFrameEvents::GetSDP(int x,int guessed_scan_y) {
  MEM_ADDRESS sdp=NULL;
  int i,j;
#if defined(SSE_DEBUG_FRAME_REPORT_MASK) //skip if not recorded
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SDP_LINES) 
#endif
  for(i=1; i<=MAX_EVENTS ;i++)
  {
    if(m_FrameEvent[i].Scanline==guessed_scan_y)
    {
      if(m_FrameEvent[i].Type=='@' && !sdp)
        sdp=m_FrameEvent[i].Value<<16; // fails if 1st word=0 (Leavin' Terramis)
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

void TFrameEvents::Init() {
  m_nEvents=m_nReports=TriggerReport=nVbl=0;
}


int TFrameEvents::Report() {
  TRACE("Saving frame events...\n");
  FILE* fp;
  fp=fopen(FRAME_REPORT_FILENAME,"w"); // unique file name
  //ASSERT(fp);
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
#if defined(SSE_MMU_WU_DL)
      ,MMU.WS[WAKE_UP_STATE]);
#else
    ,0);
#endif
#else
    fprintf(fp,"Steem frame report - %s WS%d\n",
      st_model_name[ST_TYPE],MMU.WS[WAKE_UP_STATE]);
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
      if(m_FrameEvent[i].Type=='L' // || m_FrameEvent[i].Type=='C' 
        || m_FrameEvent[i].Type=='#') // decimal
        fprintf(fp," %03d:%c%04d",m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
      else  // hexa
        fprintf(fp," %03d:%c%04X",m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
    }//nxt
    fclose(fp);
    fprintf(fp,"\n--"); // so we know it was OK
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
        TRACE(" %03d:%c%04d",m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
      else  // hexa
        TRACE(" %03d:%c%04X",m_FrameEvent[i].Cycle,m_FrameEvent[i].Type,m_FrameEvent[i].Value);
  }
  TRACE("\n");
}


int TFrameEvents::Vbl() {
#if defined(SSE_SHIFTER_REPORT_VBL_TRICKS)
  if(Debug.ShifterTricks)
  {
#undef LOGSECTION
#define LOGSECTION LOGSECTION_VIDEO
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK1 & TRACE_CONTROL_SUMMARY)
#endif
      TRACE_LOG("VBL %d Shifter tricks %X\n",nVbl,Debug.ShifterTricks);
//#undef LOGSECTION

#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK2 & OSD_CONTROL_SHIFTERTRICKS)
#else
    if(TRACE_ENABLED) 
#endif
      TRACE_OSD("T%X",Debug.ShifterTricks);
#undef LOGSECTION//???
    Debug.ShifterTricks=0;
  }
#endif  
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

#endif//#if defined(SSE_DEBUG_FRAME_REPORT)

#endif//#if defined(STEVEN_SEAGAL)
