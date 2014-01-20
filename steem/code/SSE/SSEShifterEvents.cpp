#if defined(SS_SHIFTER_EVENTS)

TVideoEvents VideoEvents;  // singleton


TVideoEvents::TVideoEvents() {
  Init();
} 


#if defined(SS_DEBUG_REPORT_SDP_ON_CLICK)
MEM_ADDRESS TVideoEvents::GetSDP(int x,int guessed_scan_y) {
  MEM_ADDRESS sdp=NULL;
  int i,j;
  //TRACE("m_nEvents %d\n",m_nEvents);
  //ASSERT( m_nEvents>0 ); //2: vbi passed!
  for(i=1; i<=MAX_EVENTS ;i++)
  {
    if(m_VideoEvent[i].Scanline==guessed_scan_y)
    {
      if(m_VideoEvent[i].Type=='A')
        sdp=m_VideoEvent[i].Value<<16;
      else if(m_VideoEvent[i].Type=='a')
      {
        sdp|=m_VideoEvent[i].Value; // at start of line
//        TRACE("sdp %X\n",sdp);
        // look for tricks
        int trick=0;
        while( i<=MAX_EVENTS && m_VideoEvent[i].Scanline==guessed_scan_y
          && m_VideoEvent[i].Type!='T')
          i++;
        if(m_VideoEvent[i].Type=='T')
          trick=m_VideoEvent[i].Value;
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
        sdp&=~1; // looks more serious...
        break;
      }

    }
  }//nxt
  return sdp;
}
#endif

void TVideoEvents::Init() {
  m_nEvents=m_nReports=TriggerReport=nVbl=0;
}


int TVideoEvents::Report() {
  TRACE("Saving frame shifter events...\n");
  FILE* fp;
  fp=fopen(SHIFTER_EVENTS_FILENAME,"w"); // unique file name
  ASSERT(fp);
  if(fp)
  {
#if defined(WIN32)
    char sdate[9];
    char stime[9];
    _strdate( sdate );
    _strtime( stime );
    fprintf(fp,"Steem shifter events report - %s -%s \n%s WS%d\n",sdate,stime,
#if defined(SS_STF)
      st_model_name[ST_TYPE]
#else
      "STE"
#endif
#if defined(SS_MMU_WAKE_UP_DL)
      ,MMU.WS[WAKE_UP_STATE]);
#else
    ,0;
#endif
#else
    fprintf(fp,"Steem shifter events report - %s WS%d\n",
      st_model_name[ST_TYPE],MMU.WS[WAKE_UP_STATE]);
#endif
    if(FloppyDrive[0].DiskInDrive())
      fprintf(fp,"Disk A: %s",FloppyDrive[0].DiskName.c_str()); 
    int i,j;
    for(i=1,j=-1;i<=m_nEvents;i++)
    {
      if(m_VideoEvent[i].Scanline!=j)
      {
        j=m_VideoEvent[i].Scanline;
        fprintf(fp,"\n%03d -",j);
      }
      if(m_VideoEvent[i].Type=='F' || m_VideoEvent[i].Type=='C' 
        || m_VideoEvent[i].Type=='#') // decimal
        fprintf(fp," %03d:%c%04d",m_VideoEvent[i].Cycle,m_VideoEvent[i].Type,m_VideoEvent[i].Value);
      else  // hexa
        fprintf(fp," %03d:%c%04X",m_VideoEvent[i].Cycle,m_VideoEvent[i].Type,m_VideoEvent[i].Value);
    }//nxt
    fclose(fp);
  }
  fprintf(fp,"\n--"); // so we know it was OK
  m_nReports++;
  return m_nReports;
}


void TVideoEvents::ReportLine() {
  // current line
  // look back
  int i=m_nEvents;
  while(i>1 && m_VideoEvent[i].Scanline==scan_y)
    i--;
  if(m_VideoEvent[i].Cycle>=508)
    i++; // former line
  TRACE("Y%d C%d ",scan_y,LINECYCLES);
  for(;i<=m_nEvents;i++)
  {
    if(m_VideoEvent[i].Type=='F' || m_VideoEvent[i].Type=='C' 
        || m_VideoEvent[i].Type=='#') // decimal
        TRACE(" %03d:%c%04d",m_VideoEvent[i].Cycle,m_VideoEvent[i].Type,m_VideoEvent[i].Value);
      else  // hexa
        TRACE(" %03d:%c%04X",m_VideoEvent[i].Cycle,m_VideoEvent[i].Type,m_VideoEvent[i].Value);
  }
  TRACE("\n");
}


int TVideoEvents::Vbl() {
#if defined(SS_SHIFTER_REPORT_VBL_TRICKS)
  if(Debug.ShifterTricks)
  {
#define LOGSECTION LOGSECTION_VIDEO
    TRACE_LOG("VBL %d shifter tricks %X\n",nVbl,Debug.ShifterTricks);
//    TRACE_LOG("VBL %d shifter tricks %X xbios2 %X\n",FRAME,Debug.ShifterTricks,xbios2);
    if(TRACE_ENABLED) 
      TRACE_OSD("%X",Debug.ShifterTricks);
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

#endif

