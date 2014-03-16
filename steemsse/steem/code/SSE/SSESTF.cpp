#if defined(SS_STF)

char* st_model_name[]={"STE","STF","Mega ST4"};

int SwitchSTType(int new_type) {

  ASSERT( sizeof("123")==4 );
  ASSERT(new_type==STE||new_type==STF||new_type==MEGASTF);
  ST_TYPE=new_type;
  if(ST_TYPE!=STE) // all STF types
  {
//    stfm_borders=4; // Steem 3.2 command-line option STFMBORDER (not used)
#if defined(SS_INT_VBL_STF) // instead of SS_INT_VBI_START
    HblTiming=HBL_FOR_STF; 
#endif
#if defined(SS_MFP_RATIO)
#if defined(SS_MFP_RATIO_STF)
    CpuMfpRatio=(double)CPU_STF_PAL/(double)MFP_CLK_TH_EXACT;
    CpuNormalHz=CPU_STF_PAL;
#else
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_LE_EXACT;
    CpuNormalHz=CPU_STE_TH;
#endif
#endif
  }
  else //STE
  {
//    stfm_borders=0;
#if defined(SS_INT_VBL_STF)
    HblTiming=HBL_FOR_STE;
#endif

#if defined(SS_MFP_RATIO)
#if defined(SS_MFP_RATIO_STE_AS_STF)
    CpuMfpRatio=(double)CPU_STF_PAL/(double)MFP_CLK_TH_EXACT;
    CpuNormalHz=CPU_STE_PAL;
#elif defined(SS_MFP_RATIO_STE)
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_STE_EXACT;
    CpuNormalHz=CPU_STE_TH;
#else
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_LE_EXACT;
    CpuNormalHz=CPU_STE_TH;
#endif
#endif

  }

#if defined(SS_MFP_RATIO)
#if defined(SS_MFP_RATIO_HIGH_SPEED) //fix v3.6.1 (broken v3.5.1)
  if(n_cpu_cycles_per_second<10000000) // avoid interference with ST CPU Speed option
#endif
    n_cpu_cycles_per_second=CpuNormalHz; // no wrong CPU speed icon in OSD (3.5.1)
#endif

#if defined(SS_INT_VBI_START) || defined(SS_INT_VBL_STF)
  draw_routines_init(); // to adapt event plans (overkill?)
#endif
  return ST_TYPE;
}

#endif//#if defined(SS_STF)