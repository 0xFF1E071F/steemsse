#if defined(SSE_INTERRUPT)

#if defined(SSE_INT_JITTER) //no
/*  This comes from Hatari but was also discussed on AF.
http://www.atari-forum.com/viewtopic.php?f=68&t=9527&p=188067#p188067
"
Just tested the STOP jitter behaviour. 
It is identical with GLUE wake up state 1 and 2: it is cyclic with
 5 positions [8,0,4,0,4],8,0,4,0,4,8, ...
 "
 Since v3.6.4, we prefer the E-clock approach (when 6301/ACIA selected)
 v3.7: undef because we fall back on Steem 3.2's wobble without the option
*/
  int HblJitter[] = {8,4,4,0,0}; 
  int HblJitterIndex;  // 3.6.1 reset at cold reset
  int VblJitter[] = {8,0,4,0,4}; 
  int VblJitterIndex;
#endif

#if defined(SSE_INT_VBL_STF)
  int HblTiming=HBL_FOR_STE; // default STE
#endif

#endif

#if defined(SSE_INT_MFP_RATIO) // need no SSE_STF
DWORD CpuNormalHz=CPU_STF_PAL;
#if defined(SSE_INT_MFP_RATIO_OPTION)
DWORD CpuCustomHz=CPU_STF_PAL;
#endif
double CpuMfpRatio=(double)CpuNormalHz/(double)MFP_CLK_TH_EXACT;
#endif

