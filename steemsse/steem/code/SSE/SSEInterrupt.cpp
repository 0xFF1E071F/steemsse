#if defined(SSE_INTERRUPT)

#if defined(SSE_INT_JITTER) 
/*  This comes from Hatari but was also discussed on AF.
http://www.atari-forum.com/viewtopic.php?f=68&t=9527&p=188067#p188067
"
Just tested the STOP jitter behaviour. 
It is identical with GLUE wake up state 1 and 2: it is cyclic with
 5 positions [8,0,4,0,4],8,0,4,0,4,8, ...
 "
    Apparently this has been measured and should be correct.
    In v3.5.3 we use Steem's "wobble" instead, as long as it
    works, not because it's more correct, but to save some memory
    bytes, and also because of Krig STE.
    SSE_INT_JITTER not defined
    In v3.5.4, for Japtro we use VBL jitter and not HBL, so
    that Krig is still OK. 
    In v3.6.0 we use HBL jitter again, for 3615 GEN4 HMD
    TODO
*/
  int HblJitter[] = {8,4,4,0,0}; 
  int HblJitterIndex;  // 3.6.1 reset at cold reset
  int VblJitter[] = {8,0,4,0,4}; 
  int VblJitterIndex;
#endif

#if defined(SSE_INT_VBL_STF)
  int HblTiming=HBL_FOR_STE;//HBL_FOR_STF; // default STE
#endif

#endif

#if defined(SSE_MFP_RATIO) // need no SSE_STF
#if defined(SSE_MFP_RATIO_STE)
double CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_STE_EXACT;
#else
double CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_LE_EXACT;
#endif
DWORD CpuNormalHz=CPU_STE_TH; // STE
#endif

