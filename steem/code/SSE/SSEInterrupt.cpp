#if defined(SS_INTERRUPT)

#if defined(SS_INT_JITTER) 
/*  This comes from Hatari but was also discussed on AF.
    Apparently this has been measured and should be correct.
    In v3.5.3 we use Steem's "wobble" instead, as long as it
    works, not because it's more correct, but to save some memory
    bytes, and also because of Krig STE (TODO).
    SS_INT_JITTER not defined
    In v3.5.4, for Japtro we use VBL jitter and not HBL, so
    that Krig is still OK. 
*/
  int HblJitter[] = {8,4,4,0,0}; 
  int HblJitterIndex=0;
  int VblJitter[] = {8,0,4,0,4}; 
  int VblJitterIndex=0;
#endif

#if defined(SS_INT_VBL_STF)
  int HblTiming=HBL_FOR_STE;//HBL_FOR_STF; // default STE
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

