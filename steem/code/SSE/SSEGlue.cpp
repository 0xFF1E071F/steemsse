#include "SSE.h"

#include "../pch.h"
#include <conditions.h>

#include "SSEGlue.h"
#include "SSEMMU.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_GLUE)

TGlue::TGlue() {
  DE_cycles[0]=DE_cycles[1]=320;
  DE_cycles[2]=160;
}



/*  There's already a MMU object where wake-up state is managed.
    We use Glue to compute some "Shifter tricks" thresholds only
    when some options are changed, and not at every check for each
    scanline.
    This should improve performance and make code clearer.
*/

void TGlue::Update() {

  char STE_modifier=-16; // not correct!
#if defined(SSE_MMU_WU_DL)
  char WU_res_modifier=MMU.ResMod[WAKE_UP_STATE]; //-2, 0, 2
  char WU_sync_modifier=MMU.FreqMod[WAKE_UP_STATE]; // 0 or 2
#endif
/*
  ScanlineTiming[NEGATE_HSYNC][FREQ_50]=
  ScanlineTiming[NEGATE_HBLANK][FREQ_50]=
*/
  ScanlineTiming[START_PREFETCH][FREQ_50]=56;
  ScanlineTiming[STOP_PREFETCH][FREQ_50]=56+320;
/*
  ScanlineTiming[ASSERT_HBL][FREQ_50]=
  ScanlineTiming[ASSERT_HSYNC][FREQ_50]=
*/


  for(int f=0;f<NFREQS;f++)
  {
    ScanlineTiming[STOP_PREFETCH][f]
      =ScanlineTiming[START_PREFETCH][f]+DE_cycles[f];
    for(int t=0;t<NTIMINGS;t++)
    {
      ScanlineTiming[t][f]+=STE_modifier; // all timings!
      if(f==FREQ_60)
        ScanlineTiming[t][f]-=4;
    }
  }

}

TGlue Glue;

#endif//#if defined(SSE_SHIFTER)

#endif//#if defined(STEVEN_SEAGAL)