#include "SSE.h"

#include "../pch.h"
#include <conditions.h>

#include <emulator.decla.h>
#include <run.decla.h>

#include "SSEGlue.h"
#include "SSEMMU.h"
#include "SSEDebug.h"
#include "SSEInterrupt.h"
#include "SSEShifter.h"

#if defined(SSE_DEBUG_FRAME_REPORT)
#include "SSEFrameReport.h"
#endif

#include "SSECpu.h"//34


#if defined(STEVEN_SEAGAL)

#if defined(SSE_GLUE)

TGlue::TGlue() {
  DE_cycles[0]=DE_cycles[1]=320;
  DE_cycles[2]=160;
}


#if defined(SSE_GLUE_FRAME_TIMINGS_A)
/*  v3.8
    Major refactoring of Steem's original event plans. See draw_routines_init().
    Timings for each scanline, hbl and vbl interrupts were set once and for all
    for the three ST syncs: 50hz, 60hz, 72hz.
    This system was simple, fast and robust.
    It had 2 disadvantages:
    - Some memory use
    - Lack of flexibility. Programs on the ST can and do change sync during
    a frame, so that there's no guarantee of fixed frame timings. Typically,
    a 50hz frame would contain 60hz scanlines (508 cycles instead of 512).
    Steem could still display the frames fine in some cases, but timings during
    and at the end of the frame were wrong.
    In previous versions of Steem SSE hacks were added to deal with more
    situations, at the price of confusing code.
    In v3.8, we compute the next screen event at each event check.
    Advantages and disadvantages are the opposite of the previous system.
    Conceptually, it is more satisfying, because Steem acts more like a real
    ST, where timings are also computed on the go by the GLU.
    Function is called by run's prepare_next_event() and prepare_event_again().
*/

void TGlue::GetNextScreenEvent() {

  ASSERT(Shifter.CurrentScanline.Cycles==224||Shifter.CurrentScanline.Cycles==508||Shifter.CurrentScanline.Cycles==512);//||n_millions_cycles_per_sec>8);

  // default event = scanline
  screen_event.event=event_scanline;
  screen_event.time=Shifter.CurrentScanline.Cycles;

  // VBI is set some cycles into first scanline of frame
  if(!Status.vbi_done&&!scanline)
  {
    screen_event.time=ScanlineTiming[ENABLE_VBI][FREQ_50]; //60, 72?
    screen_event.event=event_trigger_vbi;
    if(!Status.hbi_done)
      hbl_pending=true; 
  }
  // Video counter is reloaded some scanlines before the end of the frame
  else if(!Status.sdp_reload_done &&(scanline==310||scanline==260||scanline==494))
  {
    screen_event.time=ScanlineTiming[RELOAD_SDP][shifter_freq_idx];
    screen_event.event=event_start_vbl;
  }
  // GLU uses counters and sync, shift mode values to trigger VSYNC
  // at cycle 0, mode could be 2, so we use Shifter.CurrentScanline.Cycles
  // instead because we must program event
  else if(!Status.vbl_done && 
    (Shifter.CurrentScanline.Cycles==512&&scanline==312 
    || Shifter.CurrentScanline.Cycles==508&&scanline==262
    || Shifter.CurrentScanline.Cycles==224&&scanline==500))
  {
    screen_event.time=Shifter.CurrentScanline.Cycles;
    screen_event.event=event_vbl_interrupt;
    Status.vbi_done=false;
  }  
  // We consider GLU doesn't need to test shift mode, counter is enough at HIRES
  else if(scan_y>502) // ?
  {
    scan_y=-scanlines_above_screen[shifter_freq_idx]; 
    scanline=0; 
    init_screen();
#if defined(SSE_DEBUG_FRAME_REPORT)
    FrameEvents.Vbl(); //frame events ovf
#endif
  }
  screen_event_vector=screen_event.event;

#if defined(SSE_INT_MFP_RATIO)
  if (n_cpu_cycles_per_second>CpuNormalHz){
#else
  if (n_cpu_cycles_per_second>8000000){
#endif
    ASSERT(n_millions_cycles_per_sec>8);
    int factor=n_millions_cycles_per_sec;
    screen_event.time*=factor;
    screen_event.time/=8;
  }
  time_of_next_event=screen_event.time+cpu_timer_at_start_of_hbl;
}


void TGlue::Reset(bool Cold) {
 if(Cold) // if warm, Glue keeps on running
 {
   scanline=0;
   cpu_timer_at_start_of_hbl=0;
 }
  *(BYTE*)(&Status)=0;
}

#endif

/*  There's already a MMU object where wake-up state is managed.
    We use Glue to compute some "Shifter tricks" thresholds only
    when some options are changed, and not at every check for each
    scanline.
    This should improve performance and make code clearer.
    We don't set values if they won't be used.
    Source note: the idea was in v3.7.1 but that wasn't really done
    before v3.8.0.
*/

void TGlue::Update() {

#if defined(SSE_GLUE_THRESHOLDS)

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
    ScanlineTiming[MMU_DE_OFF][f]=ScanlineTiming[MMU_DE_ON][f]+DE_cycles[f];
  // On the STE, DE test occurs sooner due to hardscroll possibility
  // but prefetch starts sooner only if HSCROLL <> 0.
  // If HSCROLL = 0, The Shifter is fed zeroes instead of video RAM.
  if(ST_TYPE==STE) // adapt for HSCROLL prefetch after DE_OFF has been computed
  {
    ScanlineTiming[MMU_DE_ON][FREQ_72]-=4;
    ScanlineTiming[MMU_DE_ON][FREQ_60]-=16;
    ScanlineTiming[MMU_DE_ON][FREQ_50]-=16;
  }

  // HBLANK
  ScanlineTiming[HBLANK_OFF][FREQ_50]=32+WU_sync_modifier;
  if(ST_TYPE==STE)
    ScanlineTiming[HBLANK_OFF][FREQ_50]-=2; 
  ScanlineTiming[HBLANK_OFF][FREQ_60]=ScanlineTiming[HBLANK_OFF][FREQ_50]-4;

  // HSYNC
  // Cases: Enchanted Land, HighResMode, Hackabonds, Forest
  // There's both a -2 difference for STE and a -4 difference for 60hz
  ScanlineTiming[HSYNC_ON][FREQ_50]=464+WU_res_modifier;
  if(ST_TYPE==STE)
    ScanlineTiming[HSYNC_ON][FREQ_50]-=2; 
  ScanlineTiming[HSYNC_ON][FREQ_60]=ScanlineTiming[HSYNC_ON][FREQ_50]-4;
  ScanlineTiming[HSYNC_OFF][FREQ_50]=ScanlineTiming[HSYNC_ON][FREQ_50]+40;
  //ScanlineTiming[HSYNC_OFF][FREQ_60]=ScanlineTiming[HSYNC_ON][FREQ_60]+40;  

  // Reload video counter
  // Cases: DSOTS, Forest
  ScanlineTiming[RELOAD_SDP][FREQ_50]=(ST_TYPE==STE)?62:64+WU_sync_modifier;
  //ScanlineTiming[RELOAD_SDP][FREQ_60]=ScanlineTiming[RELOAD_SDP][FREQ_50];//?
  //ScanlineTiming[RELOAD_SDP][FREQ_72]=ScanlineTiming[RELOAD_SDP][FREQ_50];//?

  // Enable VBI
  ScanlineTiming[ENABLE_VBI][FREQ_50]=(ST_TYPE==STE)?(68):64; 

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


#if defined(SSE_GLUE_FRAME_TIMINGS_A)

void TGlue::Vbl() {
  cpu_timer_at_start_of_hbl=time_of_next_event;
  scan_y=-scanlines_above_screen[shifter_freq_idx];
  scanline=0;
  Status.sdp_reload_done=false;
  Status.vbl_done=true;
  Status.hbi_done=false;
}

#endif

TGlue Glue; // our one chip

#endif//#if defined(SSE_GLUE)

#endif//#if defined(STEVEN_SEAGAL)
