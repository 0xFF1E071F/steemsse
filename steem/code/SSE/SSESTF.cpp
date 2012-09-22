#if defined(SS_STF)

SwitchSTType(int new_type) {
  ASSERT(new_type==STE||new_type==STF||new_type==STF2||new_type==MEGASTF);
  ST_TYPE=new_type;
  if(ST_TYPE!=STE) // all STF types
  {
    stfm_borders=4; // Steem 3.2 command-line option STFMBORDER (not used)
#if defined(SS_INT_VBL_STF) // instead of SS_INT_VBI_START
    HblTiming=HBL_FOR_STF; // fixes 36.15 Gen4 demo by Cakeman 444+4+4
                          // but it's +8 now, and it may fail
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
  else 
  {
    stfm_borders=0;
#if defined(SS_INT_VBL_STF)
    HblTiming=HBL_FOR_STE;//444
#endif
#if defined(SS_MFP_RATIO)
#if defined(SS_MFP_RATIO_STE)
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_STE_EXACT;
#else
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_LE_EXACT;
#endif
    CpuNormalHz=CPU_STE_TH;
#endif
  }

#if defined(SS_INT_VBI_START) || defined(SS_INT_VBL_STF)
  draw_routines_init(); // to adapt event plans (overkill?)
#endif
  return ST_TYPE;
}

#endif//#if defined(SS_STF)