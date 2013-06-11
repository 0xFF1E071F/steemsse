#if defined(SS_INTERRUPT)

#if defined(SS_INT_JITTER) // from Hatari
  int HblJitter[] = {8,4,4,0,0}; 
  int HblJitterIndex=0;
  int VblJitter[] = {8,0,4,0,4}; 
  int VblJitterIndex=0;
#endif

#if defined(SS_INT_VBL_STF)
  int HblTiming=HBL_FOR_STF;//444;
#endif

#endif

#if defined(SS_MFP_RATIO) // need no SS_STF
#if defined(SS_MFP_RATIO_STE)
double CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_STE_EXACT;
#else
double CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_LE_EXACT;
#endif
DWORD CpuNormalHz=CPU_STE_TH; // STE
#endif

