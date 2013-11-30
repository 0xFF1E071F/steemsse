#if defined(SS_SHIFTER_EVENTS)

TVideoEvents VideoEvents;  // singleton


TVideoEvents::TVideoEvents() {
  Init();
} 


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
    fprintf(fp,"Steem shifter events report - %s -%s\nFrame freq %d res %d %s WS%d\n",
      sdate,stime,shifter_freq_at_start_of_vbl,screen_res,st_model_name[ST_TYPE],MMU.WS[WAKE_UP_STATE]);
#else
    fprintf(fp,"Steem shifter events report - Frame freq %d res %d %s WS%d\n",
      shifter_freq_at_start_of_vbl,screen_res,st_model_name[ST_TYPE],MMU.WS[WAKE_UP_STATE]);
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
//    TRACE_LOG("VBL %d shifter tricks %X\n",nVbl,Debug.ShifterTricks);
    TRACE_LOG("VBL %d shifter tricks %X\n",FRAME,Debug.ShifterTricks);
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

