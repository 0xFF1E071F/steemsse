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
    Source note: the idea was in v3.7.1 but that wasn't really done
    before v3.8.0.
*/

void TGlue::Update() {


#if defined(SSE_SHIFTER_380)

#if defined(SSE_MMU_WU_DL)
  char WU_res_modifier=MMU.ResMod[WAKE_UP_STATE]; //-2, 0, 2
  char WU_sync_modifier=MMU.FreqMod[WAKE_UP_STATE]; // 0 or 2
#else
  const char WU_res_modifier=0;
  const char WU_sync_modifier=0;
#endif

  // DE
  ScanlineTiming[MMU_DE_ON][FREQ_72]=6+WU_res_modifier; // GLUE tests MODE
  ScanlineTiming[MMU_DE_ON][FREQ_60]=52+WU_sync_modifier; // GLUE tests SYNC
  ScanlineTiming[MMU_DE_ON][FREQ_50]=56+WU_sync_modifier;
  for(int f=0;f<NFREQS;f++) // MMU DE OFF = MMU DE ON + DE cycles
  {
    ScanlineTiming[MMU_DE_OFF][f]=ScanlineTiming[MMU_DE_ON][f]+DE_cycles[f];
  }
  if(ST_TYPE==STE) // adapt for HSCROLL prefetch after DE_OFF has been computed
  {
    ScanlineTiming[MMU_DE_ON][FREQ_72]-=4;
    ScanlineTiming[MMU_DE_ON][FREQ_60]-=16;
    ScanlineTiming[MMU_DE_ON][FREQ_50]-=16;
  }

  // HBLANK demo Forest implies there's a 60/50hz difference, but not STF/STE!
  ScanlineTiming[HBLANK_OFF][FREQ_50]=32+WU_sync_modifier;
  ScanlineTiming[HBLANK_OFF][FREQ_60]=ScanlineTiming[HBLANK_OFF][FREQ_50]-4;

  //HSYNC
  ScanlineTiming[HSYNC_ON][FREQ_50]=464+WU_res_modifier;
  if(ST_TYPE==STE)
    ScanlineTiming[HSYNC_ON][FREQ_50]-=4; //?
  ScanlineTiming[HSYNC_ON][FREQ_60]=ScanlineTiming[HSYNC_ON][FREQ_50];  //?
  ScanlineTiming[HSYNC_OFF][FREQ_50]=ScanlineTiming[HSYNC_ON][FREQ_50]+40;
  ScanlineTiming[HSYNC_OFF][FREQ_60]=ScanlineTiming[HSYNC_OFF][FREQ_50];  //?

#else // this wasn't used anyway
  char STE_modifier=-16; // not correct!
#if defined(SSE_MMU_WU_DL)
  char WU_res_modifier=MMU.ResMod[WAKE_UP_STATE]; //-2, 0, 2
  char WU_sync_modifier=MMU.FreqMod[WAKE_UP_STATE]; // 0 or 2
#endif
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
#endif
}

TGlue Glue;

#endif//#if defined(SSE_GLUE)

#endif//#if defined(STEVEN_SEAGAL)