#include "SSE.h"

#if defined(SSE_STF)

#include "../pch.h"
#include <conditions.h>
#include <emulator.decla.h>
#include <gui.decla.h>
#if defined(SSE_TOS_GEMDOS_RESTRICT_TOS3)
#include <stemdos.decla.h>
#endif
#include "SSEDebug.h"
#include "SSEInterrupt.h"
#include "SSESTF.h"
#include "SSEOption.h"
#include "SSEGlue.h"

// note this is global here, not in classes. TODO?
// and it is also in the release build (but now used in TRACE)

char* st_model_name[]={"STE","STF","Mega ST4"
#if defined(SSE_STF_8MHZ)//no
  ,"STF 8.00 MHZ"
#endif
};

#if defined(SSE_TOS_WARNING1) //TODO refactor, functions in TTos
void CheckSTTypeAndTos() {
#if defined(SSE_TOS_GEMDOS_RESTRICT_TOS3)
  if(Tos.VersionWarning)
#endif
  if(tos_version<0x106 && ST_TYPE==STE || tos_version>=0x106 && ST_TYPE!=STE
#if defined(SSE_TOS_GEMDOS_RESTRICT_TOS4)    
    && tos_version!=0x206
#endif
    )
    Alert("TOS and ST type normally not compatible","Warning",MB_OK|MB_ICONWARNING);
}
#endif

int SwitchSTType(int new_type) { // it was one of the first added functions, no object

  ASSERT(new_type>=0 && new_type<SSE_STF_ST_MODELS);
  ST_TYPE=new_type;
  if(ST_TYPE!=STE) // all STF types
  {
//    stfm_borders=4; // Steem 3.2 command-line option STFMBORDER (not used)
#if defined(SSE_INT_VBL_STF)//no
    HblTiming=HBL_FOR_STF; 
#endif
#if defined(SSE_INT_MFP_RATIO)

#if defined(SSE_INT_MFP_RATIO_STF) 

#if defined(SSE_STF_8MHZ)//no
    if(ST_TYPE==STF8MHZ) // this removes artefacts in panic.tos, is it normal?
    {
      CpuMfpRatio=(double)CPU_STF_ALT/(double)MFP_CLK_TH_EXACT;
      CpuNormalHz=CPU_STF_ALT;
    }
    else
#elif defined(SSE_INT_MFP_RATIO_OPTION)
    if(OPTION_CPU_CLOCK)
      CpuMfpRatio=(double)CpuCustomHz/(double)MFP_CLK_TH_EXACT;
    else
#endif
    {
      CpuMfpRatio=(double)CPU_STF_PAL/(double)MFP_CLK_TH_EXACT;
      CpuNormalHz=CPU_STF_PAL;
    }

#else
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_LE_EXACT;
    CpuNormalHz=CPU_STE_TH;
#endif
#endif

  }
  else //STE
  {
//    stfm_borders=0;
#if defined(SSE_INT_VBL_STF)
    HblTiming=HBL_FOR_STE;
#endif

#if defined(SSE_INT_MFP_RATIO)
#if defined(SSE_INT_MFP_RATIO_OPTION)
    if(OPTION_CPU_CLOCK)
      CpuMfpRatio=(double)CpuCustomHz/(double)MFP_CLK_TH_EXACT;
    else
#endif
    {
#if defined(SSE_INT_MFP_RATIO_STE_AS_STF)
#if defined(SSE_INT_MFP_RATIO_STE2)
    CpuMfpRatio=(double)CPU_STE_PAL/(double)MFP_CLK_TH_EXACT;
#else
    CpuMfpRatio=(double)CPU_STF_PAL/(double)MFP_CLK_TH_EXACT;
#endif
    CpuNormalHz=CPU_STE_PAL; 
#elif defined(SSE_INT_MFP_RATIO_STE)
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_STE_EXACT;
    CpuNormalHz=CPU_STE_TH;
#else
    CpuMfpRatio=(double)CPU_STE_TH/(double)MFP_CLK_LE_EXACT;
    CpuNormalHz=CPU_STE_TH;
#endif
    }
    TRACE_INIT("CPU~%d hz\n",CpuNormalHz);
#endif

  }
  
#if defined(SSE_INT_MFP_RATIO)
#if defined(SSE_INT_MFP_RATIO_HIGH_SPEED) //fix v3.6.1 (broken v3.5.1)
  if(n_cpu_cycles_per_second<10000000) // avoid interference with ST CPU Speed option
#endif
    n_cpu_cycles_per_second=CpuNormalHz; // no wrong CPU speed icon in OSD (3.5.1)
#endif

#if defined(SSE_INT_VBL_STF)//no
  draw_routines_init(); // to adapt event plans (overkill?)
#endif

#if defined(SSE_GLUE)
  Glue.Update();
#endif

  return ST_TYPE;
}


#endif//#if defined(SSE_STF)
