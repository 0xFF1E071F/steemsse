#include "SSE.h"

#if defined(STEVEN_SEAGAL) && defined(SSE_GLUE)

#include "../pch.h"
#include <conditions.h>

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#include <display.decla.h>
#endif

#include <emulator.decla.h>
#include <run.decla.h>

#include "SSEGlue.h"
#include "SSEMMU.h"
#include "SSEDebug.h"
#include "SSEInterrupt.h"
#include "SSEShifter.h"
#include "SSEParameters.h"

#if defined(SSE_DEBUG_FRAME_REPORT)
#include "SSEFrameReport.h"
#endif

#include "SSECpu.h"//34


//#if defined(STEVEN_SEAGAL) && defined(SSE_GLUE)

TGlue::TGlue() {
#if !defined(SSE_VAR_RESIZE_380)
  DE_cycles[0]=DE_cycles[1]=320;
#endif
  DE_cycles[2]=160;
#if defined(SSE_VAR_RESIZE_380)
  ASSERT(DE_cycles[2]<<1==320);
  DE_cycles[0]=DE_cycles[1]=DE_cycles[2]<<1; // do we reduce footprint?
#endif
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  Freq[FREQ_50]=50;
  Freq[FREQ_60]=60;
  Freq[FREQ_72]=72;	
  CurrentScanline.Cycles=scanline_time_in_cpu_cycles_8mhz[1]; 
#endif
}


#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
/*  Because Shifter tricks generally involve the GLU chip more than the
    Shifter, we move some functions and variables from Shifter object
    to Glue object.
    Advantages:
    - no need to qualify GLU thresholds with TGlue::
    - can remove some previous compile choices (#ifdef) while still
      being able to compile previous versions, making code easier to read
    Disadvantages:
    - increased code base
    - can be confusing if you don't know where to look
    - debugging using previous switches will be harder
    But we need to go on...
    We assume: 
    - version >= 380
    - SSE_GLUE_FRAME_TIMINGS, SSE_GLUE_THRESHOLDS, SSE_SHIFTER_380,
    SSE_SHIFTER_TRICKS, SSE_SHIFTER_STATE_MACHINE, SSE_SHIFTER_STE_MED_HSCROLL,
    SSE_SHIFTER_LEFT_OFF_TEST_BEFORE_HBL,SSE_SHIFTER_LINE_PLUS_2_TEST,
    SSE_VID_BORDERS_416_NO_SHIFT (all) defined
    - SSE_SHIFTER_STEEM_ORIGINAL, SSE_SHIFTER_FIX_LINE508_CONFUSION,
    SSE_SHIFTER_LEFT_OFF_60HZ, SSE_VID_VERT_OVSCN_OLD_STEEM_WAY undefined

*/

#define LOGSECTION LOGSECTION_VIDEO


void TGlue::AdaptScanlineValues(int CyclesIn) { 
  // on set sync or shift mode
  // on IncScanline (CyclesIn=-1)

  if(FetchingLine())
  {
    if((m_ShiftMode&2)&&CyclesIn==-1) // at IncScanline
    {
      CurrentScanline.StartCycle=ScanlineTiming[GLU_DE_ON][FREQ_72];
      CurrentScanline.EndCycle=ScanlineTiming[GLU_DE_OFF][FREQ_72];
    }
    else
    {
      if(CyclesIn<=52 && left_border)
        CurrentScanline.StartCycle= (m_SyncMode&2)?56:52;
      if(CyclesIn<=372)
        CurrentScanline.EndCycle=(m_SyncMode&2)?376:372;
    }
  }
  if(CyclesIn<=cycle_of_scanline_length_decision)
  {
    CurrentScanline.Cycles=scanline_time_in_cpu_cycles_8mhz
      [(m_ShiftMode&2)?2:shifter_freq_idx];
    prepare_next_event();
  }
}


void TGlue::AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra) {
  // What a beautiful name!
  // Replacing macro ADD_EXTRA_TO_SHIFTER_DRAW_POINTER_AT_END_OF_LINE(s)
  ASSERT(!ExtraAdded);
  extra+=(LINEWID)*2;  // the timing of this should still be investigated   
  // One raster is different according to shift mode, eg Cool STE
  if(shifter_skip_raster_for_hscroll) 
    extra+=(left_border&&screen_res<2) ? ((screen_res==1) ? 4 : 8) : 2; 
  extra+=overscan_add_extra;
  overscan_add_extra=0;
  ExtraAdded=true;
}


int TGlue::CheckFreq(int t) {
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++);
  if(j==32)
    i=-1; // this ugly thing still necessary anyway
  return i;
}


void TGlue::CheckSideOverscan() {
/*  Various "Shifter tricks" (in fact all chips responsible for video
    are involved) can change the border size and the number of bytes fetched
    from video RAM. 
    Those tricks can be used with two goals: use a larger display area
    (fullscreen demos are more impressive than borders-on demos), and/or
    scroll the screen by using "sync lines" (eg Enchanted Land).
*/
#if defined(SSE_DEBUG) && defined(SSE_SHIFTER_DRAW_DBG)
  draw_check_border_removal(); // base function in Steem 3.2
  return;
#endif

  ASSERT(FetchingLine());
  ASSERT(screen_res<=2)

  int act=ABSOLUTE_CPU_TIME,t;
  WORD CyclesIn=LINECYCLES;

  //////////////////////
  // HIGH RES EFFECTS //
  //////////////////////

#if defined(SSE_SHIFTER_HIRES_OVERSCAN)//3.7.0
/*  Yes! You may abuse the GLUE when you have a monochrome screen too.
    It's the same idea: changing shift mode when it's about to make 
    decisions.
    R0 parts in black not emulated, and visibly other things, very trashy
    effect for the moment (I like it!)

    Eg: Monoscreen by Dead Braincells
    004:R0000 012:R0002                       -> 0byte ?
    016:R0000 024:R0002 156:R0000 184:R0002   -> right off + ... ? 
*/

  if(screen_res==2) 
  {
    if(!freq_change_this_scanline)
      return;
    int fetched_bytes_mod=0;

    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
      && CyclesIn>ScanlineTiming[GLU_DE_ON][FREQ_72]
      && !(ShiftModeAtCycle(ScanlineTiming[GLU_DE_ON][FREQ_72]+2)&2))
    {
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
      fetched_bytes_mod=-80;
      draw_line_off=true;
    }
    else if( !(CurrentScanline.Tricks&2) 
      && CyclesIn>ScanlineTiming[GLU_DE_OFF][FREQ_72]
      && !(ShiftModeAtCycle(ScanlineTiming[GLU_DE_OFF][FREQ_72]+2)&2))
    {
      // we have no bit for +14, it's right off, next left off
      // note 14x2=28 = 6 + 22
      // 6-166 DE; we deduce 166+22=188 HSYNC
      CurrentScanline.Tricks|=2; 
      fetched_bytes_mod=14;
    }
#ifdef SSE_DEBUG
    //else { TRACE("F %d ",FRAME); REPORT_LINE; }
#endif
    shifter_draw_pointer+=fetched_bytes_mod;
    CurrentScanline.Bytes+=fetched_bytes_mod;
    return;
  }
#else
  if(screen_res>=2
    || !FetchingLine() 
    || !act) // if ST is resetting
    return;
#endif

  short r0cycle=-1,r1cycle=-1,r2cycle=-1;
#if defined(SSE_MMU_WU_DL) && !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  char WU_res_modifier=MMU.ResMod[WAKE_UP_STATE]; //-2, 0, 2
  char WU_sync_modifier=MMU.FreqMod[WAKE_UP_STATE]; // 0 or 2
#endif

  /////////////////////
  // LEFT BORDER OFF //
  /////////////////////

/*  To "kill" the left border, the program sets shift mode to 2 so that the 
    GLU thinks that it's a high resolution line, and enables DE (Display 
    Enable) sooner.
*/
#if defined(SSE_VID_BORDERS_LINE_PLUS_20) // there could be some border left
  if(!(CurrentScanline.Tricks
    &(TRICK_LINE_PLUS_20|TRICK_LINE_PLUS_26|TRICK_0BYTE_LINE)))
#else
  if(left_border)
#endif
  {

  //////////////////////////////////////////
  //  STE ONLY LEFT BORDER OFF (line +20) //
  //////////////////////////////////////////

#if defined(SSE_SHIFTER_LINE_PLUS_20)
/*  Only on the STE, it is possible to create stable long overscan lines (no 
    left, no right border) without a "Shifter reset" switch (R2/R0  at around
    444/456).
    Those shift mode switches for left border removal produce a 20 bytes bonus
    instead of 26, and the total overscan line is 224 bytes instead of 230. 
    224/8=28,no rest => no Shifter confusion.

    Test cases: MOLZ, Riverside, (...)

    Current explanation:

    4  IF(RES == LO) PRELOAD will run until cycle 16  (56-16)/2 = +20 

    WORDS_READ=0
    WHILE(PRELOAD == TRUE) {
      LOAD
      WORDS_READ++
      IF(RES == HIGH AND WORDS_READ=>1) PRELOAD = FALSE
      IF(RES == LO AND WORDS_READ=>4) PRELOAD = FALSE
    }
    H = TRUE

    During preload, no video memory is fetched if HSCROLL=0.
    Instead the MMU feeds the Shifter with 0.

cycle       0               4               8               12               12

+26        00              VV              VV               VV               VV
                           Preload stops, real fetching begins
+20        00              00              00               00               VV
                           Preload goes on until cycle 16
*/

#if defined(SSE_STF)
    if(ST_TYPE==STE) 
#endif
    {
      ASSERT(!(CurrentScanline.Tricks&(TRICK_LINE_PLUS_20|TRICK_LINE_PLUS_26)));
      if( CyclesIn>ScanlineTiming[GLU_DE_ON][FREQ_72]+4
        && !(ShiftModeAtCycle(ScanlineTiming[GLU_DE_ON][FREQ_72]+4+2)&2)  // is R0 at 6
        && (ShiftModeAtCycle(ScanlineTiming[GLU_DE_ON][FREQ_72]+2)&2))  // is R2 at 2
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_20;
    }
#endif

  ///////////////////////////////////
  //  LEFT BORDER OFF (line +26)   //
  ///////////////////////////////////

/*  The "+26" trick works on both the STF and the STE, but working timings
    are not the same.
    The limits are (tables by Paolo)
    STF R2 506 [WU1 504 WU3,4 506 WU2 508] - 6 [WU1 4 WU3,4 6 WU2 8] 
        R0 8 [WU1 6 WU3,4 8 WU2 10] - 32 [WU1 30 WU3,4 32 WU2 34]
    STE R2 2 
        R0 26

    In highres, MMU DE starts at linecycle 6 on the STF (+-WU), not 0 like
    it is generally assumed.
    This is why a shift mode switch in the first cycles of the scanline will
    work.
    Bonus bytes are 26 because DE is on from 6 to 58 (start of 50hz line), that
    is 58-6=52 cycles. 52/2 = 26.

    On a STE, the DE decision is made earlier than on the STF because of 
    HSCROLL.
    In HIRES, it starts 4 cycles earlier, that's why cycle 2 instead of 6 is
    the limit for R2 on a STE.

    Programs not working on a STE because they hit at cycle 4: 
    3615Gen4 by Cakeman
    Cuddly Demos (not patched)
    Hackabonds Demo
    Musical Wonder 1991
    Imagination/Roller Coaster 
    Overdrive/Dragon
    Overscan Demos original
    SNYD/TCB (F4)
    SNYD2/Sync fullscreen (F5)
    SoWatt/Menu+Sync
    Union/Fullscreen by Level 16
    ...

    Omega hits at 8! only works on STF in WS2
    008:R00FE 020:R0000 376:S0000 384:S00FE 512:T10011 512:#0230

    Shift mode must be switched back for HBLANK OFF (28+WU), or nothing will 
    be seen.
    By this time, the pixels of left border have gone through the Shifter:
    6 + 28 (prefetch + latency) = 34. This means that every bit that enters the
    Shifter is displayed.

    We "give" 26 bytes even when frequency is 60hz like in HighResMode:
    008:R0001 428:R0002 440:R0001 500:R0002 508:#0184
    This is combined with a "-2 effect" at EndHBL, it's simpler so
    emulation-wise (in v3.7 we had a separate trick).
*/

    if(CyclesIn>ScanlineTiming[GLU_DE_ON][FREQ_72]
      && !(CurrentScanline.Tricks&TRICK_LINE_PLUS_20)
      && (ShiftModeAtCycle(ScanlineTiming[GLU_DE_ON][FREQ_72]+2)&2))
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_26; // mark "left-off"

    // action for line+26 & line+20

    if(CurrentScanline.Tricks&(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_20))
    {
#if defined(SSE_SHIFTER_LINE_PLUS_20)
      if(CurrentScanline.Tricks&TRICK_LINE_PLUS_20)
      {
        ASSERT(ST_TYPE==STE);
        CurrentScanline.Bytes+=20;
#if defined(SSE_VID_BORDERS)
        if(SideBorderSize!=ORIGINAL_BORDER_SIDE && border)
        {
          overscan_add_extra=-4; // 24-4=20
#if defined(SSE_VID_BORDERS_LINE_PLUS_20)
/*  Display border, not video memory as first pixels for the line +20:
    MOLZ in 413x275 display mode.
*/
          left_border=(LINEWID||!SSE_HACKS_ON)?16:14; //cool hack...
          shifter_draw_pointer+=8;
#endif
        }
        else
#endif
        {
#if defined(SSE_VID_BORDERS_LINE_PLUS_20)
          left_border=0;
#endif
          overscan_add_extra+=4; // 16+4=20
        }

#if defined(SSE_SHIFTER_STE_HSCROLL_LEFT_OFF)  
        if(shifter_skip_raster_for_hscroll
#if defined(SSE_VID_BORDERS_LINE_PLUS_20)
          && (SideBorderSize==ORIGINAL_BORDER_SIDE||!border)
#endif
          ) 
          overscan_add_extra+=6; // fixes MOLZ/Spiral
#endif

#if defined(SSE_SHIFTER_LINE_PLUS_20_SHIFT)
        //cases: Riverside
#if defined(SSE_VID_BORDERS)
        if(SideBorderSize==VERY_LARGE_BORDER_SIDE && border)
        {
          Shifter.HblPixelShift=-4;
          MMU.ShiftSDP(-8);
        }
        else
#endif
          Shifter.HblPixelShift=-8;
#endif
        TrickExecuted|=TRICK_LINE_PLUS_20;
      }
      else // normal left off
#endif
      {
/*  A 'left off' grants 26 more bytes, that is 52 pixels from 6 to 58 at 50hz.
    Confirmed on real STE: Overscan Demos F6
    There's no "shift" hiding the first 4 pixels but the shift is necessary for
    Steem in 384 x 270 display mode: border = 32 pixels instead of 52.
    16 pixels skipped by manipulating video counter, 4 more to skip
 */
#if defined(SSE_VID_BORDERS) 
        if(SideBorderSize!=VERY_LARGE_BORDER_SIDE || !border)
#endif
        {
          shifter_pixel+=4;
          shifter_draw_pointer+=8; // 8+16+2=26
        }
        CurrentScanline.Bytes+=26;
/*  We add 2 to video counter at the end of the line whatever the display size,
    for different reasons: 4 pixels skipped in 384 x 270 mode, 4 pixels won't
    be displayed on the right in other modes.
*/
        overscan_add_extra+=2;

#if !defined(SSE_GLUE_001)
#if defined(SSE_SHIFTER_HSCROLL_380_B1)
        if(Shifter.hscroll0>=12)
#else
        if(HSCROLL>=12) // STE Shifter bug (Steem authors) //TODO
#endif
#if defined(SSE_VID_BORDERS) //[E605 &] Tekila artefacts
          if(SideBorderSize!=VERY_LARGE_BORDER_SIDE || !border) 
#endif
            MMU.ShiftSDP(8);
#endif//001 
        TrickExecuted|=TRICK_LINE_PLUS_26;
#if defined(SSE_VID_BORDERS_LINE_PLUS_20)
        left_border=0;
#endif

        // additional hacks for left off
        //////////////////////////////////

        if(SSE_HACKS_ON)
        {
/*  Look for r2cycle and r0cycle, for our hacks :)
    ShiftMode at r0cycle can be 0 (generally) or 1 (ST-CNX, HighResMode).

    In some rare cases two R2 are found, eg Ultimate GFA Demo boot screen
    STF
    004:R0002 012:R0000 376:S0000 388:S0002 436:R0002 448:R0000 508:R0002
    STE:
    008:R0000 372:S0000 384:S0002 432:R0002 444:R0000 504:R0002 512:R0002
    (left border removed, right border misses)

*/
      r2cycle=PreviousShiftModeChange(ScanlineTiming[GLU_DE_ON][FREQ_72]+2);
      if(CyclesIn>ScanlineTiming[GLU_DE_ON][FREQ_72])
        r0cycle=NextShiftModeChange(ScanlineTiming[GLU_DE_ON][FREQ_72]);  

#if defined(SSE_SHIFTER_UNSTABLE_DOLB)
/*  The famous Death of the Left Border
    016:R0000 376:S0000 388:S0002 512:R0002 512:T10011 512:#0230
    We guess that because it's set in HIRES during 16 cycles, this affects
    the preloaded Shifter registers (due to no stabiliser) somehow. 
*/
          if(Shifter.Preload && (r0cycle-r2cycle)==16)
            MMU.ShiftSDP(-2);
#endif
#if defined(SSE_SHIFTER_BIG_WOBBLE)
/*
-29 - 016:S0002 048:c0900 072:c0904 388:C0916 412:C0745 436:C050A 512:TC000 512:#0160
-28 - 392:C050B 416:C07FD 440:C0980 512:TC000 512:#0160
-27 - 404:C0507 428:C074B 452:C0932 512:R0082 512:TC000 512:#0160
-26 - 012:R00F0 376:S00F0 384:S0082 444:R0082 456:R00F0 512:R0082 512:T2011 512:#0230
*/
#if defined(SSE_SHIFTER_HSCROLL_380_B1)
          if((PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE) && Shifter.hscroll0  // so it's STE-only
#else
          if((PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE) && HSCROLL  // so it's STE-only
#endif
#if defined(SSE_SHIFTER_ARMADA_IS_DEAD)
/*
198 - 436:C0501 440:C0747 472:C094E 512:R0082 512:TC000 512:#0160
199 - 008:R0000 376:S0000 384:S0082 444:R0082 456:R0000 496:S0000 512:R0082 512:T2211 512:#0230
200 - 008:R0000 016:S0082 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2011 512:#0230
*/
            && (r0cycle-r2cycle)==12 // still a hack
#endif
            && scan_y==-26
            ) 
            shifter_draw_pointer+=-4; // "fixes" Big Wobble shift
          //TODO: black left border
#endif
#if defined(SSE_SHIFTER_XMAS2004)
/*  Those are just hacks, as ususal for those cases, but they correct
    the last screen of XMAS 2004
    Strange that it doesn't break things (it will come!)
*/
          if((PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE)
            && (r0cycle-r2cycle)==16 && LINEWID)
#if defined(SSE_SHIFTER_HSCROLL_380_B1)
            if(!Shifter.hscroll0)
#else
            if(!HSCROLL) // we find back this 6bytes difference, this could be simplified
#endif
              shifter_draw_pointer+=4;
            else
              shifter_draw_pointer+=-2;
#endif
#if defined(SSE_SHIFTER_SCHNUSDIE)
          if(!(PreviousScanline.Tricks&TRICK_STABILISER)
            && (r0cycle-r2cycle)==24 // 508/20: very long in mode 2
            && shifter_hscroll_extra_fetch 
#if defined(SSE_SHIFTER_HSCROLL_380_B1)
            && !Shifter.hscroll0)
#else
            && !HSCROLL) // further "targeting"
#endif
            overscan_add_extra+=2; // fixes Reality is a Lie/Schnusdie bottom border
#endif
        }//hacks
      }//+26
#if !defined(SSE_VID_BORDERS_LINE_PLUS_20)
      left_border=0;
#endif
      //CurrentScanline.StartCycle=ScanlineTiming[GLU_DE_ON][FREQ_72]; //not really used
      CurrentScanline.StartCycle=6; //used by ReadSDP now... TODO
      //CurrentScanline.StartCycle=ST_TYPE==STE?2:6;
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
      if(OPTION_PRECISE_MFP)
        MC68901.AdjustTimerB(); //isn't it too late? TODO
#endif
      overscan=OVERSCAN_MAX_COUNTDOWN;
    }
  }

  /////////////////
  // 0-BYTE LINE //
  /////////////////

/*  Various shift mode or sync mode switches trick the GLUE into passing a 
    scanline in video RAM while the monitor is still displaying a line.
    This is a way to implement "hardware" downward vertical scrolling 
    on a computer where it's not foreseen (No Buddies Land - see end of
    this function).
    0-byte lines can also be combined with other sync lines (Forest, etc.).

    Normal lines:

    Video RAM
    [][][][][][][][][][][][][][][][] (1)
    [][][][][][][][][][][][][][][][] (2)
    [][][][][][][][][][][][][][][][] (3)
    [][][][][][][][][][][][][][][][] (4)


    Screen
    [][][][][][][][][][][][][][][][] (1)
    [][][][][][][][][][][][][][][][] (2)
    [][][][][][][][][][][][][][][][] (3)
    [][][][][][][][][][][][][][][][] (4)


    0-byte line:

    Video RAM
    [][][][][][][][][][][][][][][][] (1)
    [][][][][][][][][][][][][][][][] (2)
    [][][][][][][][][][][][][][][][] (3)
    [][][][][][][][][][][][][][][][] (4)


    Screen
    [][][][][][][][][][][][][][][][] (1)
    -------------------------------- (0-byte line)
    [][][][][][][][][][][][][][][][] (2)
    [][][][][][][][][][][][][][][][] (3)
    [][][][][][][][][][][][][][][][] (4)


*/

#if defined(SSE_SHIFTER_0BYTE_LINE)
/*  Here we test for 0byte line at the start of the scanline, affecting
    current scanline.
    v3.8.0, test is more generic.

    Two timings may be targeted.

    HBLANK OFF: if it doesn't happen, the line doesn't start.
    (ljbk) 0byte line   8-32 on STF
    DE ON: if mode/frequency isn't right at the timing, the line won't start.
    This can be done by shift or sync mode switches.

    Cases
    shift:
    Nostalgia/Lemmings 28/44 for STF
    Nostalgia/Lemmings 32/48 for STE
    sync:
    Forest: STF(1) (56/64), STF(2) (58/68), and STE (40/52)
    loSTE: STF(1) (56/68) STF(2) (58/74) STE (40/52)

    In fact, HBLANK OFF timing appears to be the same on the STE as on the STF,
    as used in demo Forest's "black lines".
    It could be that the GLUE is different on the STE, and wouldn't
    produce a 0-byte line when mode is 2 at HBLANK OFF for LORES timings.
    Instead, it would be a 0-byte line because mode is 2 at DE for LORES
    timings, that's why demo author had to delay the switch by 4 cycles.
    So Lemmings STE will still work, but now it reports "Forest type
    0byte line". 
*/

  if(!(CurrentScanline.Tricks
    &(TRICK_0BYTE_LINE|TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_20)))
  {
#define r2cycle ScanlineTiming[HBLANK_OFF][FREQ_50]
#define s0cycle ScanlineTiming[GLU_DE_ON][FREQ_60]
#define s2cycle ScanlineTiming[GLU_DE_ON][FREQ_50]
    ASSERT(s0cycle>0);
    ASSERT(s2cycle>0);
    if(ST_TYPE!=STE //our new hypothesis
      && CyclesIn>r2cycle && ShiftModeAtCycle(r2cycle+2)&2) // hblank trick?
    {
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK1 & TRACE_CONTROL_0BYTE)
      {
        REPORT_LINE;
        TRACE_LOG("detect Lemmings STF type 0byte y %d\n",scan_y);
      }
#endif
    }
    else if(CyclesIn>=s2cycle+2  // DE trick? (mode + sync)
      && FreqAtCycle(s2cycle+2)!=50 && FreqAtCycle(s0cycle+2)!=60)
    {
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK1 & TRACE_CONTROL_0BYTE)
      {
        REPORT_LINE;
        TRACE_LOG("detect Forest type 0byte y %d\n",scan_y);
        //   TRACE_LOG("%d %d\n",FreqAtCycle(s2cycle+2),FreqAtCycle(s0cycle+2));
      }
#endif
    }
#undef r2cycle
#undef s0cycle
#undef s2cycle
  }

  // 0-byte line: action (for 0byte detected just now or at end of line before)
  if( (CurrentScanline.Tricks&TRICK_0BYTE_LINE) 
    && !(TrickExecuted&TRICK_0BYTE_LINE))
  { 
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK1 & TRACE_CONTROL_0BYTE)
    {
      TRACE_LOG("F%d apply 0byte y %d\n",FRAME,scan_y);
      TRACE_OSD("0 byte %d",scan_y);
    }
#endif
    draw_line_off=true; // Steem's original flag for black line
    memset(PCpal,0,sizeof(long)*16); // all colours black
    shifter_draw_pointer-=160; // hack: Steem will draw black from video RAM and update SDP
    CurrentScanline.Bytes=0;
    TrickExecuted|=TRICK_0BYTE_LINE;
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
    CurrentScanline.StartCycle=CurrentScanline.EndCycle=-1;//never starts
    if(OPTION_PRECISE_MFP)
      MC68901.AdjustTimerB();
#endif
  }

#endif//0byte

  ////////////////
  // BLACK LINE //
  ////////////////

/*  A sync switch at cycle 26 keeps HBLANK asserted for this line.
    Video memory is fetched, but black pixels are displayed.
    This is handy to hide ugly effects of tricks in "sync lines".

    HBLANK OFF 60hz 28 50hz 32

    Paolo table
    switch to 60: 26-28 [WU1,3] 28-30 [WU2,4]
    switch back to 50: 30-...[WU1,3] 32-...[WU2,4]

    Test cases: Forest, Overscan #5, #6
*/

  if(!draw_line_off) // equivalent to "trick executed"
  {
    // test
    if(CyclesIn>ScanlineTiming[HBLANK_OFF][FREQ_50] 
    && FreqAtCycle(ScanlineTiming[HBLANK_OFF][FREQ_60]+2)==50 
      && FreqAtCycle(ScanlineTiming[HBLANK_OFF][FREQ_50]+2)==60)
      CurrentScanline.Tricks|=TRICK_BLACK_LINE;

    // action
    if(CurrentScanline.Tricks&TRICK_BLACK_LINE)
    {
      ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );
      //TRACE_LOG("%d BLK\n",scan_y);
      draw_line_off=true;
      memset(PCpal,0,sizeof(long)*16); // all colours black
    }
  }

  //////////////////////
  // MED RES OVERSCAN //
  //////////////////////

/*  Overscan (230byte lines) is possible in medium resolution too. Because the
    MMU may have fetched some bytes when the switch occurs, we need to 
    shift the display according to R1 cycle (No Cooper Greetings).
    We haven't analysed it at low level, the formula below is pragmatic, just
    like for 4bit scrolling.
    20 for NCG   512R2 12R0 20R1        shift=2
    28 for PYM/BPOC  512R2 12R0 28R1    shift=0
    36 for NCG off lines 183, 200 512R2 12R0 36R1 (strange!) shift=2
    16 for Drag/Reset    shift=?
    12 & 16 for PYM/STCNX left off (which is really doing 4bit hardcsroll)
    D4/Nightmare shift=2:
082 - 012:R0000 020:R0001 028:R0000 376:S0000 388:S0082 444:R0082 456:R0000 508:R0082 512:T2071
*/

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_MED_OVERSCAN)

  if(!left_border && !(TrickExecuted&(TRICK_OVERSCAN_MED_RES|TRICK_0BYTE_LINE)))
  {
    // look for switch to R1
    r1cycle=CycleOfLastChangeToShiftMode(1);
    if(r1cycle>16 && r1cycle<=40)
    //if(r1cycle>=12 && r1cycle<=40)
    {
      // look for switch to R0 before switch to R1
      r0cycle=PreviousShiftModeChange(r1cycle);
      if(r0cycle!=-1 && !ShiftModeChangeAtCycle(r0cycle)) //so we have R2-R0-R1
      {
        CurrentScanline.Tricks|=TRICK_OVERSCAN_MED_RES;
        TrickExecuted|=TRICK_OVERSCAN_MED_RES;
        int cycles_in_low_res=r1cycle-r0cycle;
#if defined(SSE_SHIFTER_MED_OVERSCAN_SHIFT)
        MMU.ShiftSDP(-(((cycles_in_low_res)/2)%8)/2); //impressive formula!
        shifter_pixel+=4; // hmm...
#endif
#if defined(SSE_VID_BORDERS) && defined(SSE_VID_BPOC)
        if(BORDER_40 && SSE_HACKS_ON && cycles_in_low_res==16) 
        { // fit text of Best Part of the Creation on a 800 display (pure hack)
          MMU.ShiftSDP(4);      
          shifter_pixel+=4; 
        }
#endif
      }
    }
  }
#endif

  /////////////////////
  // 4BIT HARDSCROLL //
  /////////////////////

/*  When the left border is removed, a MED/LO switch causes the Shifter to
    shift the line by a number of pixels dependent on the cycles at which
    the switch occurs. It doesn't change the # bytes fetched (230 in known 
    cases). How exactly it happens is worth a study (TODO). 
    It's a pity that it wasn't used more. Was it a French technique?
    PYM/ST-CNX (lines 70-282 (7-219); offset 8, 12, 16, 20) 0R2 12R1 XR0
007 - 012:R0001 032:R0000 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2031 512:#0230
    D4/NGC (lines 76-231) 0R2 12R0 20R1 XR0
015 - 012:R0000 020:R0001 036:R0000 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2071
    D4/Nightmare (lines 143-306, cycles 28 32 36 40)
    Apparently, the ST-CNX technique, R2-R1-R0, works only on STF
    The Delirious technique, R2-R0-R1-R0 should work on STE, but possibly with
    Shifter bands.
*/
  
#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_4BIT_SCROLL)

  if(!left_border // && screen_res==1 // depends on when we come here
    && !(CurrentScanline.Tricks&(TRICK_0BYTE_LINE|TRICK_4BIT_SCROLL)))
  {
    r1cycle=CycleOfLastChangeToShiftMode(1);//could be more generic
    if(r1cycle>=12 && r1cycle<=40)
    {
      // look for switch to R0 after switch to R1
      r0cycle=NextShiftModeChange(r1cycle);
      if(r0cycle>r1cycle && r0cycle<=40 && !ShiftModeChangeAtCycle(r0cycle))
      {
        int cycles_in_med_res=r0cycle-r1cycle;
        // look for previous switch to R0
        int cycles_in_low_res=0; // 0 ST-CNX
        r0cycle=PreviousShiftModeChange(r1cycle);
        if(r0cycle>=0 && !ShiftModeChangeAtCycle(r0cycle))
          cycles_in_low_res=r1cycle-r0cycle; // 8 NGC, Nightmare
        int shift_in_bytes=8-cycles_in_med_res/2+cycles_in_low_res/4;
#if defined(SSE_STF)
        if(ST_TYPE==STF || cycles_in_low_res)
#endif
          CurrentScanline.Tricks|=TRICK_4BIT_SCROLL;
        TrickExecuted|=TRICK_4BIT_SCROLL;

        MMU.ShiftSDP(shift_in_bytes);
        Shifter.HblPixelShift=13+8-cycles_in_med_res-8; // -7,-3,1, 5, done in Render()
#if defined(SSE_DEBUG)
        ASSERT( Shifter.HblPixelShift==-7||Shifter.HblPixelShift==-3||Shifter.HblPixelShift==1||Shifter.HblPixelShift==5 );
//        ASSERT( shift_in_bytes==0||shift_in_bytes==2||shift_in_bytes==4||shift_in_bytes==6 );
      //  if(scan_y==100)  TRACE_LOG("4bit y%d CMR%d CLR%d SH%d PIX%d\n",scan_y,cycles_in_med_res,cycles_in_low_res,shift_in_bytes,HblPixelShift);
#endif
      }
    }
  }
#endif

  /////////////////
  // LINE +4, +6 //
  /////////////////
#ifdef SSE_BETA
/*
    (60hz)
    40  IF(RES == HI) PRELOAD will exit after 4 cycles  (372-40)/2 = +6  
    44  IF(RES == HI) PRELOAD will exit after 8 cycles  (372-44)/2 = +4 

    (50hz)
    44  IF(RES == HI) PRELOAD will exit after 4 cycles  (376-44)/2 = +6  
    48  IF(RES == HI) PRELOAD will exit after 8 cycles  (376-48)/2 = +4 

    We need test cases
*/

#if defined(SSE_SHIFTER_LINE_PLUS_4) || defined(SSE_SHIFTER_LINE_PLUS_6)
  // this compiles into compact code
  if(ST_TYPE==STE && !shifter_hscroll_extra_fetch && CyclesIn>36 
    && !(TrickExecuted&(TRICK_0BYTE_LINE | TRICK_LINE_PLUS_26
    | TRICK_LINE_PLUS_20 | TRICK_4BIT_SCROLL | TRICK_OVERSCAN_MED_RES
    | TRICK_LINE_PLUS_4 | TRICK_LINE_PLUS_6)))
  {
    t=FreqAtCycle(ScanlineTiming[GLU_DE_ON][FREQ_60]+2==50) ? 44: 40;
    if(ShiftModeChangeAtCycle(t)==2)
    {
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_6;
#if defined(SSE_SHIFTER_LINE_PLUS_6) 
      left_border-=2*6; 
      overscan_add_extra+=6;
      CurrentScanline.Bytes+=6;
      overscan=OVERSCAN_MAX_COUNTDOWN;
      TrickExecuted|=TRICK_LINE_PLUS_6;
      TRACE_OSD("+6 y%d",scan_y);
#endif
    }
    else if(ShiftModeChangeAtCycle(t+4)==2)
    {
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_4;
#if defined(SSE_SHIFTER_LINE_PLUS_4)
      left_border-=2*4; 
      overscan_add_extra+=4;
      CurrentScanline.Bytes+=4;
      overscan=OVERSCAN_MAX_COUNTDOWN;
      TrickExecuted|=TRICK_LINE_PLUS_4;
      TRACE_OSD("+4 y%d",scan_y);
#endif
    }
  }

#endif

#endif//beta

  /////////////
  // LINE +2 //
  /////////////

/*  A line that starts as a 60hz line and ends as a 50hz line gains 2 bytes 
    because 60hz lines start and stop 4 cycles before 50hz lines.
    This is used in some demos, but most cases are accidents, especially
    on the STE: the GLU checks frequency earlier because of possible horizontal
    scrolling, and this may interfere with the trick that removes top or bottom
    border.
    A real headache in Steem, because some apparent "video" issues were in
    fact MFP or CPU timing issues.
    Positive cases: Mindbomb/No Shit, Forest, LoSTE screens
    Spurious to avoid as STE (emulation problem): DSoTS, Overscan
    Spurious unavoidable as STE (correct emulation): BIG Demo #1, ... [many]
    Spurious to avoid as STF: Panic, 3615GEN4-CKM
*/

  // test
  if(!(CurrentScanline.Tricks&
    (TRICK_0BYTE_LINE|TRICK_LINE_PLUS_2|TRICK_LINE_PLUS_4|TRICK_LINE_PLUS_6|
    TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_20)))  
  {
    ASSERT(screen_res<2);//was?

    // With this test, "line +2" isn't detected before MMU DE OFF, it's no
    // problem for ReadVideoCounter(), which doesn't check tricks
    if(CyclesIn>ScanlineTiming[GLU_DE_OFF][FREQ_60]
      && FreqAtCycle(ScanlineTiming[GLU_DE_ON][FREQ_60]+2)==60
      && FreqAtCycle(ScanlineTiming[GLU_DE_OFF][FREQ_60]+2)!=60)
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_2;
  }

  // action
  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_2)
    && !(TrickExecuted&TRICK_LINE_PLUS_2))
  {
    left_border-=4; // 2 bytes -> 4 cycles
    overscan_add_extra+=2;
    CurrentScanline.Bytes+=2;
    overscan=OVERSCAN_MAX_COUNTDOWN;
    TrickExecuted|=TRICK_LINE_PLUS_2;

#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK_14 & TRACE_CONTROL_SHIFTER_PLUS_2)
    {
      TRACE_OSD("+2 y%d",scan_y);
      REPORT_LINE;
    }
#endif
  }

  ///////////////
  // LINE -106 //
  ///////////////

/*  A shift mode switch to 2 before cycle 168 (end of HIRES line) causes 
    the line to stop there. 106 bytes are not fetched.
    This limit is another indication that in HIRES DE starts at 6 and stops
    at 166.
    When combined with a left off, this makes +26-106 = -80, or a line 80 
    (160-80). By itself it's a 54 byte line.
    It can also be combined with +2 for a 56 byte line (Hackabonds menu).
    With two such "sync" lines, you can vertically scroll the screen.
    There were bugs in previous versions because we thought the monitor
    couldn't run too long in HIRES. Alien's text was correct about it but
    not Flix's.
    Still, the Shifter displays on monochrome output, colour output is
    disabled.
    Just Buggin: 152:R0002 172:R0000
    ST-CNX: 160:R0002 172:R0000
    loSTE screens: 120:R0002  392:R0000 
    
    Paolo table
    R2 56 [...] -> 164 [WU1] 166 [WU3,4] 168 [WU2]
*/

  // test

  if(!(CurrentScanline.Tricks&(TRICK_LINE_MINUS_106|TRICK_0BYTE_LINE))
    && CyclesIn>ScanlineTiming[GLU_DE_OFF][FREQ_72]
    && (ShiftModeAtCycle(ScanlineTiming[GLU_DE_OFF][FREQ_72]+2)&2))
     CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;

  // action
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_106)
    && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
    ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );
    overscan_add_extra-=106;
    TrickExecuted|=TRICK_LINE_MINUS_106;
    ASSERT(CurrentScanline.Bytes>=106);
    CurrentScanline.Bytes-=106;
#if defined(SSE_SHIFTER_LINE_MINUS_106_BLACK)
/*  The MMU won't fetch anything more, and as long as the ST is in high res,
    the scanline is black, but Steem renders the full scanline in colour.
    It's in fact data of next scanline. We make sure nothing ugly will appear
    (loSTE screens).
*/
    draw_line_off=true;
    memset(PCpal,0,sizeof(long)*16); // all colours black
#endif
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
    if(OPTION_PRECISE_MFP)
      MC68901.AdjustTimerB(); // Fixes 1st value of TIMERB04.TOS
#endif
  }

  /////////////////////
  // DESTABILISATION //
  /////////////////////

#if defined(SSE_STF) && defined(SSE_SHIFTER_MED_RES_SCROLLING)
/*  Detect MED/LO switches during DE.
    This will load some words into the Shifter, the Shifter will start displaying
    shifted planes and shifted pixels in low res.
    There was very early Steem support for those new techniques, and it's been 
    disappointing to me that it sort of fizzled out.
*/
/*
ljbk:
detect unstable: switch MED/LOW - Beeshift
- 3 (screen shifted by 12 pixels because only 1 word will be read before the 4 are available to draw the bitmap);
- 2 (screen shifted by 8 pixels because only 2 words will be read before the 4 are available to draw the bitmap);
- 1 (screen shifted by 4 pixels because only 3 words will be read before the 4 are available to draw the bitmap);
- 0 (screen shifted by 0 pixels because the 4 words will be read to draw the bitmap);
*/
 
  if(ST_TYPE!=STE && WAKE_UP_STATE 
#if defined(SSE_SHIFTER_MED_RES_SCROLLING_360)
    && !Shifter.Preload // would complicate matters
#else
    && (PreviousScanline.Tricks&TRICK_STABILISER) // shifter is stable
#endif    
#if defined(SSE_SHIFTER_MED_RES_SCROLLING_380)
    && !(CurrentScanline.Tricks
    &(TRICK_UNSTABLE|TRICK_0BYTE_LINE|TRICK_LINE_PLUS_26|TRICK_LINE_MINUS_106))
    && FetchingLine()
#else
    && left_border && LINECYCLES>56 && LINECYCLES<372 //'DE'
#if defined(SSE_SHIFTER_HI_RES_SCROLLING)
    && !(CurrentScanline.Tricks&TRICK_UNSTABLE) 
    && !(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
    && !(CurrentScanline.Tricks&TRICK_LINE_MINUS_106)  // of course
#endif
#endif
    ) 
  {
#if defined(SSE_SHIFTER_MED_RES_SCROLLING_380)
    // detect switch to medium or high during DE (more compact code)
    int mode;
    r1cycle=NextShiftModeChange(ScanlineTiming[GLU_DE_ON][FREQ_50]); 
    if(r1cycle>ScanlineTiming[GLU_DE_ON][FREQ_50] 
    && r1cycle<=ScanlineTiming[GLU_DE_OFF][FREQ_50]
    && (mode=ShiftModeChangeAtCycle(r1cycle))
    )
    {
      r0cycle=NextShiftModeChange(r1cycle,0); // detect switch to low
      int cycles_in_med_or_high=r0cycle-r1cycle;
      if(r0cycle<=ScanlineTiming[GLU_DE_OFF][FREQ_50]&&cycles_in_med_or_high>0)
      {
        Shifter.Preload=((cycles_in_med_or_high/4)%4);
        if((mode&2)&&Shifter.Preload&1) // if it's 3 or 5
          Shifter.Preload+=(4-Shifter.Preload)*2; // swap value
#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL)
        if((OSD_MASK2 & OSD_CONTROL_PRELOAD) && Shifter.Preload)
          TRACE_OSD("R%d %d R0 %d LOAD %d",mode,r1cycle,r0cycle,Shifter.Preload);
#endif
        CurrentScanline.Tricks|=TRICK_UNSTABLE;//don't apply on this line
//        TRACE_LOG("y%d R1 %d R0 %d cycles in med %d Preload %d\n",scan_y,r1cycle,r0cycle,cycles_in_med,Shifter.Preload);
      }
    }
#else
    r1cycle=NextShiftModeChange(56,1); // detect switch to medium
    if(r1cycle>-1)
    {
      r0cycle=NextShiftModeChange(r1cycle,0); // detect switch to low
      int cycles_in_med=r0cycle-r1cycle;
      if(cycles_in_med>0 && cycles_in_med<=20+4) 
      {
        Shifter.Preload=((cycles_in_med/4)%4)+4-4;  // '+4' to make sure >1 (3.6.0 no)
        CurrentScanline.Tricks|=TRICK_UNSTABLE;//don't apply on this line
//        TRACE_LOG("y%d R1 %d R0 %d cycles in med %d Preload %d\n",scan_y,r1cycle,r0cycle,cycles_in_med,Shifter.Preload);
      }
    }
//#endif

#if defined(SSE_SHIFTER_HI_RES_SCROLLING)
/*  We had to move this part after the check for 'line -106'. Adding R2/R0 
    switches is always more "dangerous" in emulation.
  ljbk:
  LO/HI/LO works in the same way except -4 and -12 code cases are switched;
 
    Cycles           8      12     16      20
    Preload MED      2       3      4       5
    Preload HI       2       5      4       3 
    Strange but correct: demos beeshift3 and beesclr4 don't glitch when you
    press ESC.
*/
    else 
    {
      // there was no switch to MED, detect switch to high during DE
      r2cycle=NextShiftModeChange(56,2); // detect switch to high
      if(r2cycle>-1)
      {
        r0cycle=NextShiftModeChange(r2cycle,0); // detect switch to low
        int cycles_in_high=r0cycle-r2cycle;
        if(cycles_in_high>0 && cycles_in_high<=20+4)
        {
          Shifter.Preload=((cycles_in_high/4)%4);
          CurrentScanline.Tricks|=TRICK_UNSTABLE;//don't apply on this line
          if(Shifter.Preload&1) // if it's 3 or 5
            Shifter.Preload+=(4-Shifter.Preload)*2; // swap value
//          TRACE_LOG("y%d R1 %d R0 %d cycles in HI %d Preload %d\n",scan_y,r1cycle,r0cycle,cycles_in_high,Shifter.Preload);
        }
      }
    }
#endif  
#endif
  }

#endif

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_UNSTABLE)
/*  Shift due to unstable Shifter, caused by a line+26 without stabiliser
    (Dragon) or a MED/LOW switch during DE, or a HI/LOW switch during DE, 
    or a line+230 without stabiliser: apply effect
    Note this isn't exact science, it's in development.
    DOLB, Omega: 3 words preloaded but it becomes 2 due to left off
    Dragon: 1 word preloaded    
    It could be that both determining preload and using preload are wrong!
    v3.6.0: it's one word for +26 or 230; ignore when line +2 (TODO)
*/
  
  if(WAKE_UP_STATE && Shifter.Preload && 
#if !defined(SSE_SHIFTER_DOLB_STE)
   ST_TYPE==STF &&
#endif
    !(CurrentScanline.Tricks&(TRICK_UNSTABLE|TRICK_0BYTE_LINE|TRICK_LINE_PLUS_2))
#if defined(SSE_SHIFTER_MED_RES_SCROLLING_380)
    && !(TrickExecuted&TRICK_UNSTABLE))
#else
    && CyclesIn>ScanlineTiming[GLU_DE_ON][FREQ_50])   // wait for eventual +2
#endif

  {

    // 1. planes
    int shift_sdp=-(Shifter.Preload%4)*2; // unique formula
    MMU.ShiftSDP(shift_sdp); // correct plane shift

#if defined(SSE_SHIFTER_PANIC) //fun, bad scanlines (unoptimised code!)
    if(WAKE_UP_STATE==WU_SHIFTER_PANIC)
    {
#if defined(SSE_SHIFTER_UNSTABLE_DOLB) && defined(SSE_STF)
      ASSERT(Shifter.Preload==1);
      if(SSE_HACKS_ON && ShiftModeChangeAtCycle(8)==-1)   //TODO: Omega
        shift_sdp-=2; // align planes on correct bands
#endif
      shift_sdp+=shifter_draw_pointer_at_start_of_line;
      for(int i=0+8-8;i<=224-8-8;i+=16)
      {
        Shifter.Scanline[i/4]=LPEEK(i+shift_sdp); // save 
        Shifter.Scanline[i/4+1]=LPEEK(i+shift_sdp+4);
        LPEEK(i+shift_sdp)=0;  // interleaved "border" pixels
        LPEEK(i+shift_sdp+4)=0;
      }
      Shifter.Scanline[230/4+1]=shift_sdp;
    }
#endif
    // 2. pixels
    // Beeshift, the full frame shifts in the border
    // Dragon: shifts -4, just like the 230 byte lines below
    // notice the shift is different and dirtier on real STF
    if(left_border)
    {
      left_border-=(Shifter.Preload%4)*4;
      right_border+=(Shifter.Preload%4)*4;
    }
#if defined(SSE_SHIFTER_DOLB_SHIFT1) 
    else  //  hack for DOLB, Omega, centering the pic
    {
#if defined(SSE_SHIFTER_DOLB1)
      if(SSE_HACKS_ON)
        MMU.ShiftSDP(8); // again...
#endif
      if(SSE_HACKS_ON 
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
/*  The fact that the screen of DOLB is good without a special
    adjustment supports our theory of "no shift, 52 pixels".
*/
        && (SideBorderSize!=VERY_LARGE_BORDER_SIDE
#if !defined(SSE_VID_BORDERS_416_NO_SHIFT3)
        ||border  //?mistake?
#endif
        )
#endif
        )
        Shifter.HblPixelShift=4; 
    }
#endif
    //TRACE_LOG("Y%d Preload %d shift SDP %d pixels %d lb %d rb %d\n",scan_y,Preload,shift_sdp,HblPixelShift,left_border,right_border);
    CurrentScanline.Tricks|=TRICK_UNSTABLE;
  }
#endif

  /////////////
  // LINE -2 //
  /////////////

/*  A line starting at 50hz but switched to 60hz before end of DE for
    60hz will fetch 2 bytes less of video memory.

    Thresholds/WU states (from table by Paolo)

      60hz  58 - 372 WU1,3
            60 - 374 WU2,4

      50hz  374 -... WU1,3
            376 -... WU2,4
*/

  // test
  if(!(CurrentScanline.Tricks
    &(TRICK_0BYTE_LINE|TRICK_LINE_MINUS_106|TRICK_LINE_MINUS_2))
    && CyclesIn>ScanlineTiming[GLU_DE_OFF][FREQ_60]
    && FreqAtCycle(ScanlineTiming[GLU_DE_ON][FREQ_60]+2)!=60  //50,72?
    && FreqAtCycle(ScanlineTiming[GLU_DE_OFF][FREQ_60]+2)==60)
     CurrentScanline.Tricks|=TRICK_LINE_MINUS_2;

  //  action
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
    &&!(TrickExecuted&TRICK_LINE_MINUS_2))
  {
    overscan_add_extra-=2;
    CurrentScanline.Bytes-=2;
    TrickExecuted|=TRICK_LINE_MINUS_2;
//    TRACE_LOG("-2 y %d c %d s %d e %d ea %d\n",scan_y,LINECYCLES,scanline_drawn_so_far,overscan_add_extra,ExtraAdded);
  }

  /////////////////////////////////
  // RIGHT BORDER OFF (line +44) // 
  /////////////////////////////////

/*  A sync switch to 0 (60hz) at cycle 376 (end of display for 50hz)
    makes the GLUE fail to stop the line (DE still on).
    DE will stop only at cycle of HSYNC, 464.
    This is 88 cycles later and the reason why the trick grants 44 more
    bytes of video memory for the scanline.
    However, display stops at HBLANK (cycle 424, 48 cycles after 376),
    and we presume Shifter doesn't LOAD anything anymore, otherwise it would
    be loaded with 2 words at the end of the line (44 bytes is no multiple of
    8).
    HBLANK hides the display of the 4 last pixels at once.
    Because a 60hz line stops at cycle 372, the sync switch must hit just
    after that and right before the test for end of 50hz line occurs.
    That's why cycle 376 is targeted, but according to wake-up state other
    timings may work.
    Obviously, the need to hit the GLU/Shifter registers at precise cycles
    on every useful scanline was impractical.

    WS thresholds (from table by Paolo) 

    Swtich to 60hz  374 - 376 WS1,3
                    376 - 378 WS2,4

    Switch back to 50hz  378 -... WS1,3
                         380 -... WS2,4

    Nostalgia menu: must be bad display in WU2 (emulated in Steem)

*/

/*  We used the following to calibrate (Beeshift3).

LJBK:
So going back to the tests, the switchs are done at the places indicated:
-71/50 at 295/305;
-71/50 at 297/305;
-60/50 at 295/305;
and the MMU counter at $FFFF8209.w is read at the end of that line.
It can be either $CC: 204 or $A0: 160.
The combination of the three results is then tested:
WS4: CC A0 CC
WS3: CC A0 A0
WS2: CC CC CC Right Border is always open
WS1: A0 A0 A0 no case where Right Border was open

Emulators_cycle = (cycle_Paulo + 83) mod 512

WS                           1         2         3         4
 
-71/50 at 378/388            N         Y         Y         Y
-71/50 at 380/388            N         Y         N         N
-60/50 at 378/388            N         Y         N         Y

Tests are arranged to be efficient.
*/

  // test
#if defined(SSE_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE)
  t=ScanlineTiming[GLU_DE_OFF][FREQ_50]+MMU.ResMod[WAKE_UP_STATE]
    -MMU.FreqMod[WAKE_UP_STATE]+2;
#endif
  if(CyclesIn<=ScanlineTiming[GLU_DE_OFF][FREQ_50] 
    ||(CurrentScanline.Tricks&(TRICK_0BYTE_LINE|TRICK_LINE_MINUS_2
    |TRICK_LINE_MINUS_106|TRICK_LINE_PLUS_44))
    || FreqAtCycle(ScanlineTiming[GLU_DE_OFF][FREQ_60])==60)
    ; // no need to test
  else if(FreqAtCycle(ScanlineTiming[GLU_DE_OFF][FREQ_50]+2)==60
#if defined(SSE_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE)
/*  Like Alien said, it is also possible to remove the right border by setting
    the shiftmode to 2. This may be done well before cycle 376.
    WS threshold: WS1:376  WS3,4:378  WS2:380 (weird!)
*/
    || CyclesIn>t && (ShiftModeAtCycle(t+2)&2)
#endif
    )
    CurrentScanline.Tricks|=TRICK_LINE_PLUS_44;

  // action
  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_44)
    && !(TrickExecuted&TRICK_LINE_PLUS_44))
  {
    ASSERT(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE));
    ASSERT(!(CurrentScanline.Tricks&TRICK_LINE_MINUS_2));
    right_border=0;
    overscan_add_extra+=OVERSCAN_ADD_EXTRA_FOR_RIGHT_BORDER_REMOVAL;  // 28 (+16=44)
    TrickExecuted|=TRICK_LINE_PLUS_44;
#if defined(SSE_VID_BORDERS)
    if(SideBorderSize==VERY_LARGE_BORDER_SIDE)
      overscan_add_extra-=BORDER_EXTRA/2; // 20 + 24=44
#endif
    CurrentScanline.Bytes+=44;
    CurrentScanline.EndCycle=464; // should adapt to WS? 
    //CurrentScanline.EndCycle=ScanlineTiming[HSYNC_ON][FREQ_50];
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
    if(OPTION_PRECISE_MFP)
      MC68901.AdjustTimerB();
#endif
    overscan=OVERSCAN_MAX_COUNTDOWN; 
  }

  ////////////////
  // STABILISER //
  ////////////////

/*  A HI/LO switch after DE "resets" the Shifter by emptying its registers.
    All kinds of strange effects may result from absence of stabiliser when
    trying to display lines which have a length in bytes not multiple of 8,
    such as the 230 bytes line (left off, right off).
    Another stabiliser is MED/LO, it's called 'ULM' because it was used by
    this group.
    TODO: limits are arbitrary
*/

#if defined(SSE_SHIFTER_TRICKS) && defined(TRICK_STABILISER)
  if(!(CurrentScanline.Tricks&TRICK_STABILISER))
  {
    r2cycle=NextShiftModeChange(432); // can be 1 or 2
    if(r2cycle>-1 && r2cycle<460 && ShiftModeChangeAtCycle(r2cycle))
    {
      r0cycle=NextShiftModeChange(r2cycle,0);
      if(r0cycle>-1 && r0cycle<464) 
      {
        CurrentScanline.Tricks|=TRICK_STABILISER;
#if defined(SSE_SHIFTER_UNSTABLE)
        Shifter.Preload=0; // "reset" empties RR
#endif
      }
    }
  }
#endif

  /////////////////////////////////////////////////////////
  // 0-BYTE LINE 2 (next scanline) and NON-STOPPING LINE //
  /////////////////////////////////////////////////////////

/*  We use the values in LJBK's table, taking care not to break
    emulation of other cases.
    Here we test for tricks at the end of the scanline, affecting
    next scanline.

    shift mode:
    No line 1 452-464    Beyond/Pax Plax Parallax STF (460/472)
          note: mustn't break Enchanted Land STF 464/472 STE 460/468
          no 0byte when DE on (no right border)
          note D4/Tekila 199: 444:S0000 452:R0082 460:R0000
    No line 2 466-504    No Buddies Land (500/508); D4/NGC (496/508)
          note: STE line +20 504/4 (MOLZ) or 504/8 -> 504 is STF-only
*/

#if defined(SSE_SHIFTER_0BYTE_LINE)
  if(!(NextScanline.Tricks&TRICK_0BYTE_LINE))
  {
    // test
    r2cycle=(CurrentScanline.Cycles==508)?
      ScanlineTiming[HSYNC_ON][FREQ_60]:
    ScanlineTiming[HSYNC_ON][FREQ_50];
    if(CyclesIn>r2cycle && ShiftModeAtCycle(r2cycle+2)&2)
    {
      if(right_border)
      {
/*  When DE has been negated and the GLU misses HSYNC due to some trick,
    the GLU will fail to trigger next line, until next HSYNC.
    Does the monitor miss one HSYNC too?
*/
        NextScanline.Tricks|=TRICK_0BYTE_LINE; // next, of course
        NextScanline.Bytes=0;
#if defined(SSE_BOILER_TRACE_CONTROL)
        if(TRACE_MASK1 & TRACE_CONTROL_0BYTE)
        {
          TRACE_LOG("detect Plax type 0byte y %d\n",scan_y+1);
          REPORT_LINE;
        }
#endif
        shifter_last_draw_line++;//?
      }
/*  In the Enchanted Land hardware tests, a HI/LO switch at end of display
    when the right border has been removed causes the GLUE to miss HSYNC,
    and DE stays asserted for the rest of the line (24 bytes), then the next 
    scanline, not stopping until "MMU DE OFF" of that line.
    The result of the test is written to $204-$20D and the line +26 trick 
    timing during the game depends on it. This STF/STE distinction, which 
    was the point of the test, is emulated in Steem (3.4). It doesn't change
    the game itself.

      STF 464/472 => R2 4 (wouldn't work on the STE)
    000204 : 00bd 0003                :
    000208 : 0059 000d (this was the Steem patch - 4 bytes)
    00020c : 000d 0000                : 

      STE 460/468 => R2 0
    000204 : 00bd 0002  those values may be used for a STE patch
    000208 : 005a 000c  that also works in Steem 3.2 (this should
    00020c : 000d 0001  have been the patch, 12 bytes, no need for STFMBORDER)

    For performance, this is integrated in 0-byte test.
*/
      else if(!(CurrentScanline.Tricks&TRICK_LINE_PLUS_24)) 
      {
        TRACE_LOG("Enchanted Land HW test F%d Y%d R2 %d R0 %d\n",FRAME,scan_y,PreviousShiftModeChange(466),NextShiftModeChange(466,0));
        CurrentScanline.Bytes+=24; // "double" right off, fixes Enchanted Land
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_24; // recycle left off 60hz bit!
        NextScanline.Tricks|=TRICK_LINE_PLUS_26|TRICK_STABILISER; // not important for the game
        NextScanline.Bytes=160+(ST_TYPE==STF?2:0); // ? not important for the game
        time_of_next_timer_b+=512; // no timer B for this line (too late?)
      }
    }
    else if(CyclesIn>r2cycle+42 && ShiftModeAtCycle(r2cycle+42)&2)
    {
      NextScanline.Tricks|=TRICK_0BYTE_LINE;
      NextScanline.Bytes=0;
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK1 & TRACE_CONTROL_0BYTE)
      {        
        TRACE_LOG("detect NBL type 0byte y %d\n",scan_y+1); // No Buddies Land
        REPORT_LINE;
      }
#endif
      shifter_last_draw_line++;
    }
  }
#endif//0byte
}


void TGlue::CheckVerticalOverscan() {

  if(CurrentScanline.Cycles==224) // no vertical overscan check for HIRES lines
    return; // some trace routines will trigger assert (TB2)

  int CyclesIn=LINECYCLES;
  enum{NO_LIMIT=0,LIMIT_TOP,LIMIT_BOTTOM};

//  ASSERT(scan_y==-30||scan_y==199||scan_y==200);
  BYTE on_overscan_limit=(scan_y==-30)?LIMIT_TOP:LIMIT_BOTTOM;

  int t,i=shifter_freq_change_idx;
  int freq_at_trigger=0; 
  if(screen_res==2) 
    freq_at_trigger=MONO_HZ;
  if(emudetect_overscans_fixed) 
    freq_at_trigger=(on_overscan_limit) ? 60:0;
  // It is still decided at the start of the frame if it's a 50hz
  // or a 60hz one, rendering issue (TODO)
  // At least we could remove the hack for RGBeast
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50)
  {
    t=ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50];
    if(CyclesIn>=t && FreqAtCycle(t)!=50)
      CurrentScanline.Tricks|= (on_overscan_limit==LIMIT_TOP) 
        ? TRICK_TOP_OVERSCAN: TRICK_BOTTOM_OVERSCAN;
  }
#if defined(SSE_SHIFTER_60HZ_OVERSCAN) 
/*  Removing lower border at 60hz. 
    Cases: Leavin' Teramis STF
*/
  else if(on_overscan_limit==LIMIT_BOTTOM && shifter_freq_at_start_of_vbl==60)
  {
    t=ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_60];
    if(CyclesIn>t && FreqAtCycle(t)!=60) 
      CurrentScanline.Tricks|=TRICK_BOTTOM_OVERSCAN_60HZ; 
  }
#endif
  if(CurrentScanline.Tricks&
    (TRICK_TOP_OVERSCAN|TRICK_BOTTOM_OVERSCAN|TRICK_BOTTOM_OVERSCAN_60HZ))
  {
    overscan=OVERSCAN_MAX_COUNTDOWN;
    ASSERT(screen_event.event==event_scanline);
    time_of_next_timer_b=time_of_next_event+cpu_cycles_from_hbl_to_timer_b
      + TB_TIME_WOBBLE; 
    if(on_overscan_limit==LIMIT_TOP) // top border off
    {
      shifter_first_draw_line=-29;
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
      if(FullScreen && border==2) // TODO a macro
      {    //hack overscans
        int off=shifter_first_draw_line-draw_first_possible_line;
        draw_first_possible_line+=off;
        draw_last_possible_line+=off;
      }
#endif
    }
    else // bottom border off
      // Timer B will fire for the last time when scan_y is 246	    
      shifter_last_draw_line=247;   
  }

#if defined(SSE_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN)
  if(on_overscan_limit && TRACE_ENABLED) 
  {
    FrameEvents.ReportLine();
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),CurrentScanline.Tricks);
  }
#endif
#if defined(SSE_DEBUG_FRAME_REPORT) && defined(SSE_BOILER_TRACE_CONTROL)
  if(TRACE_ENABLED&&(TRACE_MASK1 & TRACE_CONTROL_VERTOVSC) && CyclesIn>=t) 
  {
    FrameEvents.ReportLine();
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),CurrentScanline.Tricks);
  //  ASSERT( scan_y!=199|| (CurrentScanline.Tricks&TRICK_BOTTOM_OVERSCAN) );
    //ASSERT( scan_y!=199|| shifter_last_draw_line==247 );
  }
#endif
}


void TGlue::EndHBL() {

#if defined(SSE_SHIFTER_END_OF_LINE_CORRECTION)
/*  Finish horizontal overscan : correct -2 & +2 effects
    Those tests are much like EndHBL in Hatari
    + Check Shifter stability (preliminary)
*/
  if((CurrentScanline.Tricks&(TRICK_LINE_PLUS_2|TRICK_LINE_PLUS_26))
    && !(CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
    && CurrentScanline.EndCycle==372)     
  {
    CurrentScanline.Tricks&=~TRICK_LINE_PLUS_2;
    shifter_draw_pointer-=2; // eg SNYD/TCB at scan_y -29
    CurrentScanline.Bytes-=2;
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK1 & TRACE_CONTROL_ADJUSTMENT)
      TRACE_LOG("F%d y %d cancel +2\n",FRAME,scan_y);
#endif
  } 
  // no 'else', they're false alerts!
  if(CurrentScanline.Tricks&TRICK_LINE_MINUS_2     
    && (CurrentScanline.StartCycle==52 || CurrentScanline.EndCycle!=372))
  {
    CurrentScanline.Tricks&=~TRICK_LINE_MINUS_2;
    shifter_draw_pointer+=2;
    CurrentScanline.Bytes+=2;
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK1 & TRACE_CONTROL_ADJUSTMENT)
      TRACE_LOG("F%d y %d cancel -2\n",FRAME,scan_y);
#endif
  }
#endif//#if defined(SSE_SHIFTER_END_OF_LINE_CORRECTION)


#if defined(SSE_SHIFTER_UNSTABLE)
/*  Shifter unstable (preloaded registers) due to overscan tricks without
    stabiliser.
*/

  if(WAKE_UP_STATE)
  {
    // nice collection of hacks still necessary (compare Overdrive menu/dragon!!!)
    if(MMU.WU[WAKE_UP_STATE]//==2 
      && (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      &&!(CurrentScanline.Tricks&TRICK_STABILISER)
//      &&!((PreviousScanline.Tricks&TRICK_STABILISER)&&PreviousScanline.Bytes==CurrentScanline.Bytes)
      &&!((PreviousScanline.Tricks&TRICK_STABILISER)&&ShiftModeChangeAtCycle(448-512)==2) //?
      &&!(CurrentScanline.Tricks&TRICK_LINE_MINUS_2))
      Shifter.Preload=1;
    else if(CurrentScanline.Cycles==508 && FetchingLine()
      && FreqAtCycle(0)==60 && FreqAtCycle(464)==60)
      Shifter.Preload=0; // a full 60hz scanline should reset the shifter


#if defined(SSE_SHIFTER_PANIC)
    if(WAKE_UP_STATE==WU_SHIFTER_PANIC 
      && (CurrentScanline.Tricks&TRICK_UNSTABLE))
    {
      // restore ST memory
      int shift_sdp=Shifter.Scanline[230/4+1]; //as stored
      for(int i=0+8-8;i<=224-8-8;i+=16)
      {
        LPEEK(i+shift_sdp)=Shifter.Scanline[i/4];
        LPEEK(i+shift_sdp+4)=Shifter.Scanline[i/4+1];
      }
    }
#endif
#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL)
    if((OSD_MASK2 & OSD_CONTROL_PRELOAD) && Shifter.Preload)
      TRACE_OSD("Preload %d\n",Shifter.Preload);
#endif
  }
#endif
}


int TGlue::FetchingLine() {
  // does the current scan_y involve fetching by the MMU?
  // notice < shifter_last_draw_line, not <=

  return (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line

#if defined(SSE_VID_BORDERS_BIGTOP) //?? what is this, TODO
//          && scan_y>=draw_first_possible_line+(DISPLAY_SIZE>=3?6:0) 
      && (DISPLAY_SIZE<BIGGEST_DISPLAY 
      || scan_y>=draw_first_scanline_for_border+
        (BIG_BORDER_TOP-ORIGINAL_BORDER_TOP))
#endif
        );
}


void TGlue::IncScanline() {

#if defined(SSE_DEBUG)
  Debug.ShifterTricks|=CurrentScanline.Tricks; // for frame
#if defined(SSE_DEBUG_FRAME_REPORT_SHIFTER_TRICKS)
  // Record the 'tricks' mask at the end of scanline
  if(CurrentScanline.Tricks)
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'T',CurrentScanline.Tricks);
#endif
#if defined(SSE_DEBUG_FRAME_REPORT_SHIFTER_TRICKS_BYTES)
  if(CurrentScanline.Tricks)
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'#',CurrentScanline.Bytes);
#endif
#if defined(SSE_DEBUG_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if((FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_SHIFTER_TRICKS) 
    && CurrentScanline.Tricks)
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'T',CurrentScanline.Tricks);
  if((FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_SHIFTER_TRICKS_BYTES)
    //&& CurrentScanline.Tricks
    )
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'#',CurrentScanline.Bytes);
#endif
#if defined(SSE_BOILER_TRACE_CONTROL)
/*  It works but the value has to be validated at least once by clicking
    'Go'. If not it defaults to 0. That way we don't duplicate code.
*/
  if((TRACE_MASK1&TRACE_CONTROL_1LINE) && TRACE_ENABLED 
    && scan_y==debug_run_until_val)
  {
    REPORT_LINE;
    TRACE_OSD("y %d %X %d",scan_y,CurrentScanline.Tricks,CurrentScanline.Bytes);
  }
#endif
#endif//dbg

  scanline++; // this is GLU's variable, scan_y is Shifter's

  if(FetchingLine()) //TODO, it's because we don't display line 246 in lores
  {
    if(scan_y==246 && screen_res<2)
      shifter_draw_pointer+=CurrentScanline.Bytes;
    if(!ExtraAdded&&(screen_res>=2||scan_y==246))
      AddExtraToShifterDrawPointerAtEndOfLine(shifter_draw_pointer);
  }
  ASSERT(ExtraAdded||!FetchingLine());

  ExtraAdded=false;
  overscan_add_extra=0;

  Shifter.IncScanline();

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_MMU1) //assume
  MMU.SDPMiddleByte=999; // an impossible value for a byte
#endif

  PreviousScanline=CurrentScanline; // auto-generated
  CurrentScanline=NextScanline;
  ASSERT( CurrentScanline.Tricks==NextScanline.Tricks );

  if(CurrentScanline.Tricks)
    ; // don't change #bytes
  else
#if defined(SSE_MMU_SDP2)//refactor, also foresee read SDP in HIRES
  if(FetchingLine()
    || scan_y==-29 && (PreviousScanline.Tricks&TRICK_TOP_OVERSCAN)
    || !scan_y && !PreviousScanline.Bytes)
    CurrentScanline.Bytes=(screen_res==2)?80:160;
#else
  if(scan_y==-29 && (PreviousScanline.Tricks&TRICK_TOP_OVERSCAN)
    || !scan_y && !PreviousScanline.Bytes)
    CurrentScanline.Bytes=160; // needed by ReadVideoCounter - not perfect (TODO)
  else if(FetchingLine()) 
    CurrentScanline.Bytes=(screen_res==2)?80:160;
#endif
  else //if(!FetchingLine()) 
  {
    ASSERT( !FetchingLine() );
    NextScanline.Bytes=0;
  }
  AdaptScanlineValues(-1);
  ASSERT(CurrentScanline.Cycles>=224);
  TrickExecuted=0;
  NextScanline.Tricks=0; // eg for 0byte lines mess
  shifter_skip_raster_for_hscroll = (HSCROLL!=0); // one more fetch at the end
  Glue.Status.scanline_done=false;
}

#if defined(SSE_SHIFTER_TRICKS)
/*  Argh! those horrible functions still there, I thought refactoring would 
    remove them, instead, all the rest has changed.
    An attempt at replacing them with a table proved less efficient anyway
    (see R419), so we should try to optimise them instead... TODO
*/

void TGlue::AddFreqChange(int f) {
  // Replacing macro ADD_SHIFTER_FREQ_CHANGE(shifter_freq)
  shifter_freq_change_idx++;
  shifter_freq_change_idx&=31;
  shifter_freq_change_time[shifter_freq_change_idx]=ABSOLUTE_CPU_TIME;
  shifter_freq_change[shifter_freq_change_idx]=f;                    
}


void TGlue::AddShiftModeChange(int mode) {
  // called only by SetShiftMode
  ASSERT((mode&3)==mode); // Oyster sets it to 3
  shifter_shift_mode_change_idx++;
  shifter_shift_mode_change_idx&=31;
  shifter_shift_mode_change_time[shifter_shift_mode_change_idx]=ABSOLUTE_CPU_TIME;
  shifter_shift_mode_change[shifter_shift_mode_change_idx]=mode;                    
}


int TGlue::CheckShiftMode(int t) {
  // this is inspired by the original Steem tests in draw_check_border_removal
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++);
  if(j==32)
    i=-1;
  return i;
}


int TGlue::FreqChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(shifter_freq_change[idx]==50||shifter_freq_change[idx]==60);
  int rv=shifter_freq_change_time[idx]-LINECYCLE0;
  return rv;
}

int TGlue::ShiftModeChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(!shifter_shift_mode_change[idx]||shifter_shift_mode_change[idx]==1||shifter_shift_mode_change[idx]==2);
  int rv=shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return rv;
}


int TGlue::FreqChangeAtCycle(int cycle) {
  // if there was a change at this cycle, return it, otherwise -1
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  // loop while it's bigger than cycle, with safety
  for(i=shifter_freq_change_idx,j=0;
  j<32 && shifter_freq_change_time[i]-t>0;
  j++,i--,i&=31);
  // here, we're on the right cycle, or smaller
  int rv=(j<32 && !(shifter_freq_change_time[i]-t))
    ?shifter_freq_change[i]:-1;
  return rv;
}

int TGlue::ShiftModeChangeAtCycle(int cycle) {
  // if there was a change at this cycle, return it, otherwise -1
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  // loop while it's bigger than cycle, with safety
  for(i=shifter_shift_mode_change_idx,j=0;
  j<32 && shifter_shift_mode_change_time[i]-t>0;
  j++,i--,i&=31);
  // here, we're on the right cycle, or smaller
  int rv=(j<32 && !(shifter_shift_mode_change_time[i]-t))
    ?shifter_shift_mode_change[i]:-1;
  return rv;

}


int TGlue::FreqChangeIdx(int cycle) {
  // give the idx of freq change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckFreq(t); // use the <= check
  if(idx!=-1 &&shifter_freq_change_time[idx]==t)
    return idx;
  return -1;
}

int TGlue::ShiftModeChangeIdx(int cycle) {
  // give the idx of shift mode change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckShiftMode(t); // use the <= check
  if(idx!=-1 &&shifter_shift_mode_change_time[idx]==t)
    return idx;
  return -1;
}


// FreqAtCycle and ShiftModeAtCycle return the value BEFORE any
// change, hence all the +2

int TGlue::FreqAtCycle(int cycle) {
  // what was the frequency at this cycle?

#if 1
#if defined(SSE_SHIFTER_380)
  ASSERT(cycle<=LINECYCLES); //TODO, this is not acceptable
#endif
  if(cycle>LINECYCLES)
    return 0;
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
//  ASSERT(shifter_freq_change[i]>0);
  if(shifter_freq_change_time[i]-t<0
#if defined(SSE_SHIFTER_380)
    && shifter_freq_change[i]>0 // internal bug? Japtro
#endif
    )
    return shifter_freq_change[i];
  return shifter_freq_at_start_of_vbl;


#else
  if(cycle>LINECYCLES)
    return 0;
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<0)
    return shifter_freq_change[i];
  return shifter_freq_at_start_of_vbl;
#endif


}

int TGlue::ShiftModeAtCycle(int cycle) {
  // what was the shift mode at this cycle?
#if defined(SSE_SHIFTER_380)
  ASSERT(cycle<=LINECYCLES);
#endif

  if(cycle>LINECYCLES) // it's a problem
  //  return 8; // binary 1000, not 0, not 1, not 2
    return m_ShiftMode;
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return shifter_shift_mode_change[i];
  return m_ShiftMode; //  return 0;
}

#ifdef SSE_BOILER
// REDO it stinks
int TGlue::NextFreqChange(int cycle,int value) {
  // return cycle of next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    if(value==-1 || shifter_shift_mode_change[i]==value)
      idx=i;
  if(idx!=-1 && shifter_freq_change_time[idx]-t>0)
    return shifter_freq_change_time[idx]-LINECYCLE0;
  return -1;
}
#endif

int TGlue::NextShiftModeChange(int cycle,int value) {
  // return cycle of next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    if(value==-1 || shifter_shift_mode_change[i]==value)
      idx=i;
  if(idx!=-1 && shifter_shift_mode_change_time[idx]-t>0)
    return shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return -1;
}


int TGlue::NextFreqChangeIdx(int cycle) {
  // return idx next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    idx=i;
  if(shifter_freq_change_time[idx]-t>0)
    return idx;
  return -1;
}

int TGlue::NextShiftModeChangeIdx(int cycle) {
  // return idx next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    idx=i;
  if(shifter_shift_mode_change_time[idx]-t>0)
    return idx;
  return -1;
}


int TGlue::PreviousFreqChange(int cycle) {
  // return cycle of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<0)
    return shifter_freq_change_time[i]-LINECYCLE0;
  return -1;
}

int TGlue::PreviousShiftModeChange(int cycle) {
  // return cycle of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}


int TGlue::PreviousFreqChangeIdx(int cycle) {
  // return idx of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<0)
    return i; 
  return -1;
}

int TGlue::PreviousShiftModeChangeIdx(int cycle) {
  // return idx of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return i;
  return -1;
}


int TGlue::CycleOfLastChangeToFreq(int value) {
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change[i]==value)
    return shifter_freq_change_time[i]-LINECYCLE0;
  return -1;
}

int TGlue::CycleOfLastChangeToShiftMode(int value) {
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change[i]==value)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}

#endif//#if defined(SSE_SHIFTER_TRICKS)

#endif//#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)

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

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  ASSERT(CurrentScanline.Cycles==224||CurrentScanline.Cycles==508||CurrentScanline.Cycles==512);//||n_millions_cycles_per_sec>8);
#endif

  // default event = scanline
  screen_event.event=event_scanline;
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  screen_event.time=CurrentScanline.Cycles;
#else
  screen_event.time=Shifter.CurrentScanline.Cycles;
#endif

  // VBI is set pending some cycles into first scanline of frame, 
  // when VSYNC stops.
  if(!Status.vbi_done&&!scanline)
  {
    screen_event.time=ScanlineTiming[ENABLE_VBI][FREQ_50]; //60, 72?
    screen_event.event=event_trigger_vbi;
    if(!Status.hbi_done)
      hbl_pending=true; 
  }
  // Video counter is reloaded some scanlines before the end of the frame.
  // VBLANK is already on since a couple of scanlines.
  // VSYNC starts.
  else if(!Status.sdp_reload_done &&(scanline==310||scanline==260||scanline==494))
  {
    screen_event.time=ScanlineTiming[RELOAD_SDP][shifter_freq_idx];
    screen_event.event=event_start_vbl;
  }
  // The GLU uses counters and sync, shift mode values to trigger VSYNC.
  // At cycle 0, mode could be 2, so we use Shifter.CurrentScanline.Cycles
  // instead because we must program event.
  else if(!Status.vbl_done && 
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
    (CurrentScanline.Cycles==512&&scanline==312&&screen_res!=2
    || CurrentScanline.Cycles==508&&scanline==262&&screen_res!=2)
    || scanline==500) // scanline 500: unconditional to catch oddities...
#else
    (Shifter.CurrentScanline.Cycles==512&&scanline==312 
    || Shifter.CurrentScanline.Cycles==508&&scanline==262
    || Shifter.CurrentScanline.Cycles==224&&scanline==500))
#endif
  {
//    ASSERT(scanline!=500);
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
    screen_event.time=CurrentScanline.Cycles;
#else
    screen_event.time=Shifter.CurrentScanline.Cycles;
#endif
    screen_event.event=event_vbl_interrupt;
    Status.vbi_done=false;
     //TRACE("prg VBL at %d\n",scanline);
  }  
#if !defined(SSE_GLUE_002) // done at scanline=500, but no init_screen()
  else if(scan_y>502) // no Status.vbl_done, Overscan Demos
  {
    scan_y=-scanlines_above_screen[shifter_freq_idx]; 
    scanline=0; 
    init_screen();
#if defined(SSE_DEBUG_FRAME_REPORT)
    FrameEvents.Vbl(); //frame events ovf
#endif
  }
#endif
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
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  m_SyncMode=0; //60hz
  CurrentScanline.Cycles=scanline_time_in_cpu_cycles_8mhz[shifter_freq_idx];
  ASSERT(CurrentScanline.Cycles<=512);
#endif
  if(Cold) // if warm, Glue keeps on running
  {
    scanline=0;
    cpu_timer_at_start_of_hbl=0;
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)	  
    Shifter.m_ShiftMode=m_ShiftMode=screen_res; //vital for HIRES emu	  
#endif
  }
  *(BYTE*)(&Status)=0;
}
#endif//frame_tmg

/*  SetShiftMode() and SetSyncMode() are called when a program writes
    on "Shifter" registers FF8260 (shift) or FF820A (sync). 
    Most Shifter tricks use those registers. 
    If performance is an issue, the analysis currently in CheckSideOverscan()
    could be moved here, removing the need for look-up functions, 
    but introducing other problems (are we on line, line-1, line+1?)
    According to ST-CNX, those registers are in the GLUE.
    The GLUE is responsible for sync signals, for "DE". Most Shifter tricks
    actually are GLUE tricks.
    
    But generally, video registers are considered to be in the Shifter by
    coders and doc.
*/

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)

void TGlue::SetShiftMode(BYTE NewMode) {
/*
  The ST possesses three modes  of  video  configuration:
  320  x  200  resolution  with 4 planes, 640 x 200 resolution
  with 2 planes, and 640 x 400 resolution with 1  plane.   The
  modes  are  set through the Shift Mode Register (read/write,
  reset: all zeros).

  ff 8260   R/W             |------xx|   Shift Mode
                                   ||
                                   00       320 x 200, 4 Plane
                                   01       640 x 200, 2 Plane
                                   10       640 x 400, 1 Plane
                                   11       Reserved

  FF8260 is both in the GLU and the Shifter. It is needed in the GLU
  because sync signals are different in mode 2 (72hz).
  It is needed in the Shifter because it needs to know in how many bit planes
  memory has to be decoded, and where it must send the video signal (RGB, 
  Mono).
    
  In monochrome, frequency is 72hz, a line is transmitted in 28µs.
  There are 500 scanlines + vsync = 1 scanline time (more or less).

  For the GLU, '3' is interpreted as '2' because only bit1 is tested
  for 'HIRES'.
  Cases: The World is my Oyster screen #2
  In v3.8, we can keep the 3 everywhere because all tests are in the form '&2'.
*/

  int CyclesIn=LINECYCLES;

#if defined(SSE_DEBUG_FRAME_REPORT_SHIFTMODE)
  FrameEvents.Add(scan_y,CyclesIn,'R',NewMode); 
#endif
#if defined(SSE_DEBUG_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SHIFTMODE)
    FrameEvents.Add(scan_y,CyclesIn,'R',NewMode); 
#endif
  
  NewMode&=3; // only two lines would physically exist
  // Update both Shifter and GLU - on the STE it should be one chip, one register
  Shifter.m_ShiftMode=m_ShiftMode=NewMode; 

  if(screen_res
#if defined(SSE_SHIFTER_HIRES_OVERSCAN)
    >
#else
    >=
#endif
    2|| emudetect_falcon_mode!=EMUD_FALC_MODE_OFF)
    return; // if not, bad display in high resolution

#ifndef NO_CRAZY_MONITOR
  if(extended_monitor)
  {
    screen_res=(BYTE)(NewMode & 1);
    return;
  }
#endif

#if !defined(SSE_SHIFTER_HIRES_OVERSCAN)
  ASSERT( COLOUR_MONITOR );
#endif

#if defined(SSE_SHIFTER_TRICKS) // before rendering
  AddShiftModeChange(NewMode); // add time & mode
#endif
#if defined(SSE_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE) 
  AddFreqChange((NewMode&2) ? MONO_HZ : shifter_freq);
#endif

  Shifter.Render(CyclesIn,DISPATCHER_SET_SHIFT_MODE);

#if defined(SSE_SHIFTER_HIRES_OVERSCAN)
  if(screen_res==2 && !COLOUR_MONITOR)
  {
    freq_change_this_scanline=true;
    return;
  //  goto L1;
  }
#endif

  int old_screen_res=screen_res;
  screen_res=(BYTE)(NewMode & 1); // only for 0 or 1 - note could weird things happen?
  if(screen_res!=old_screen_res)
  {
    shifter_x=(screen_res>0) ? 640 : 320;
    if(draw_lock)
    {
      if(screen_res==0) 
        draw_scanline=draw_scanline_lowres; // the ASM function
      if(screen_res==1) 
        draw_scanline=draw_scanline_medres;
#ifdef WIN32
      if(draw_store_dest_ad)
      {
        draw_store_draw_scanline=draw_scanline;
        draw_scanline=draw_scanline_1_line[screen_res];
      }
#endif
    }
    if(mixed_output==3 && (ABSOLUTE_CPU_TIME-cpu_timer_at_res_change<30))
      mixed_output=0; //cancel!
    else if(scan_y<-30) // not displaying anything: no output to mix...
      ; // eg Pandemonium/Chaos Dister
    else if(!mixed_output)
      mixed_output=3;
    else if(mixed_output<2)
      mixed_output=2;
    cpu_timer_at_res_change=ABSOLUTE_CPU_TIME;
  }

  freq_change_this_scanline=true; // all switches are interesting
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
  if(OPTION_PRECISE_MFP)
  {
    CALC_CYCLES_FROM_HBL_TO_TIMER_B((NewMode&2) ? MONO_HZ : shifter_freq);
    calc_time_of_next_timer_b();
  }
#endif

  if(old_screen_res==2 && !(m_ShiftMode&2))//LEGACY back from HIRES
  {
    ASSERT( COLOUR_MONITOR );
    shifter_freq_idx= (m_SyncMode&2) ? 0 : 1;
    shifter_freq=Freq[shifter_freq_idx];
    screen_res=m_ShiftMode&3;
    init_screen();// we may be at any scanline though... but Steem must render
    scan_y=-scanlines_above_screen[shifter_freq_idx]; //reset or no more VBI!!
  }
  AdaptScanlineValues(CyclesIn);
}


void TGlue::SetSyncMode(BYTE NewSync) {
/*
    ff 820a   R/W             |------xx|   Sync Mode
                                     ||
                                     | ----   External/_Internal Sync
                                      -----   50 Hz/_60 Hz Field Rate

    Only bit 1 is of interest:  1:50 Hz 0:60 Hz.
    Normally, 50hz for Europe, 60hz for the USA.
    At 50hz, the ST displays 313 lines every frame, instead of 312.5 like
    in the PAL standard (one frame with 312 lines, one with 313, etc.) 
    On the ST, sync mode was abused to create overscan (3 of the 4 borders)
*/

  int CyclesIn=LINECYCLES;

#ifdef SSE_DEBUG
#if defined(SSE_DEBUG_FRAME_REPORT_SYNCMODE)
  FrameEvents.Add(scan_y,CyclesIn,'S',NewSync); 
#endif
#if defined(SSE_DEBUG_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SYNCMODE)
    FrameEvents.Add(scan_y,CyclesIn,'S',NewSync); 
#endif
  //TRACE("F%d y%d c%d s%d frame %d\n",TIMING_INFO,NewSync,shifter_freq_at_start_of_vbl);//TEMP
#endif

  m_SyncMode=NewSync&3; // 2bits

#if defined(SSE_SHIFTER_RENDER_SYNC_CHANGES) //no!
  Render(CyclesIn,DISPATCHER_SET_SYNC);
#endif

  shifter_freq_idx=(screen_res>=2) ? 2 : ((NewSync&2)?0:1);
  ASSERT(shifter_freq_idx>=0 && shifter_freq_idx<NFREQS);
  int new_freq=Freq[shifter_freq_idx];
  ASSERT(new_freq==50||new_freq==60||new_freq==72);
  if(shifter_freq!=new_freq)
    freq_change_this_scanline=true;  
  shifter_freq=new_freq;
  AddFreqChange(new_freq);
  AdaptScanlineValues(CyclesIn);
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
  if(OPTION_PRECISE_MFP)
    MC68901.AdjustTimerB(); 
#endif
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
  if(FullScreen && border==BORDERS_AUTO_OFF)
  {
    int off=shifter_first_draw_line-draw_first_possible_line;
    draw_first_possible_line+=off;
    draw_last_possible_line+=off;
  }
#endif
}

#endif//#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)

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
/*  Update GLU timings according to ST type and wakeup state. We do it when
    player changes options, not at each scanline like before.
    As you can see, demo Forest by ljbk (AF) helps us a lot here.
*/
//#define ST_TYPE 0
#if defined(SSE_MMU_WU_DL)
  char WU_res_modifier=(ST_TYPE==STE)?0:MMU.ResMod[WAKE_UP_STATE]; //-2, 0, 2
  char WU_sync_modifier=(ST_TYPE==STE)?0:MMU.FreqMod[WAKE_UP_STATE]; // 0 or 2
#else
  const char WU_res_modifier=0;
  const char WU_sync_modifier=0;
#endif

  // DE (Display Enable)
  // Before the image is really displayed,there's a delay of 28 cycles:
  // 16 for Shifter prefetch, of course, but also 12 "other" 
  // (8 before prefetch, 4 after).
  // Timer B is triggered after those 28 cycles, on "Shifter DE".
  ScanlineTiming[GLU_DE_ON][FREQ_72]=GLU_DE_ON_72+WU_res_modifier; // GLUE tests MODE
  ScanlineTiming[GLU_DE_ON][FREQ_60]=GLU_DE_ON_60+WU_sync_modifier; // GLUE tests SYNC
  ScanlineTiming[GLU_DE_ON][FREQ_50]=GLU_DE_ON_50+WU_sync_modifier;
  for(int f=0;f<NFREQS;f++) // MMU DE OFF = MMU DE ON + DE cycles
    ScanlineTiming[GLU_DE_OFF][f]=ScanlineTiming[GLU_DE_ON][f]+DE_cycles[f];
  
  // On the STE, DE test occurs sooner due to hardscroll possibility
  // but prefetch starts sooner only if HSCROLL <> 0.
  // If HSCROLL = 0, The Shifter is fed zeroes instead of video RAM.
  // (Current theory)
  if(ST_TYPE==STE) // adapt for HSCROLL prefetch after DE_OFF has been computed
  {
    ScanlineTiming[GLU_DE_ON][FREQ_72]-=4;
    ScanlineTiming[GLU_DE_ON][FREQ_60]-=16;
    ScanlineTiming[GLU_DE_ON][FREQ_50]-=16;
  }

  // HBLANK
  // Cases: Overscan demos, Forest
  // There's a -4 difference for 60hz but timings are the same on STE
  // There's no HBLANK in high res
  ScanlineTiming[HBLANK_OFF][FREQ_50]=GLU_HBLANK_OFF_50+WU_sync_modifier;
  ScanlineTiming[HBLANK_OFF][FREQ_60]=ScanlineTiming[HBLANK_OFF][FREQ_50]-4;

  // HSYNC
  // Cases: Enchanted Land, HighResMode, Hackabonds, Forest
  // There's both a -2 difference for STE and a -4 difference for 60hz
  // The STE -2 difference isn't enough to change effects of a "right off" trick
  ScanlineTiming[HSYNC_ON][FREQ_50]=GLU_HSYNC_ON_50+WU_res_modifier;
  if(ST_TYPE==STE)
    ScanlineTiming[HSYNC_ON][FREQ_50]-=2; 
  ScanlineTiming[HSYNC_ON][FREQ_60]=ScanlineTiming[HSYNC_ON][FREQ_50]-4;
  ScanlineTiming[HSYNC_OFF][FREQ_50]=ScanlineTiming[HSYNC_ON][FREQ_50]+GLU_HSYNC_DURATION;
  ScanlineTiming[HSYNC_OFF][FREQ_60]=ScanlineTiming[HSYNC_ON][FREQ_60]+GLU_HSYNC_DURATION;  
  // ScanlineTiming[HSYNC_ON][FREQ_72]=188+WU_res_modifier;//not used
  // ScanlineTiming[HSYNC_OFF][FREQ_72]=ScanlineTiming[HSYNC_ON][FREQ_72]+36;

  // Reload video counter
  // Cases: DSOTS, Forest
  ScanlineTiming[RELOAD_SDP][FREQ_50]=GLU_RELOAD_VIDEO_COUNTER_50
    +WU_sync_modifier;
//  if(ST_TYPE==STE)
  //  ScanlineTiming[RELOAD_SDP][FREQ_50]-=2;
  ScanlineTiming[RELOAD_SDP][FREQ_60]=ScanlineTiming[RELOAD_SDP][FREQ_50];//?
  ScanlineTiming[RELOAD_SDP][FREQ_72]=ScanlineTiming[RELOAD_SDP][FREQ_50];//?

  // Enable VBI
  // Cases: Forest, 3615GEN4-CKM, Dragonnels/Happy Islands, Auto 168, TCB...
  ScanlineTiming[ENABLE_VBI][FREQ_50]=GLU_TRIGGER_VBI_50;
  if(ST_TYPE==STE)
    ScanlineTiming[ENABLE_VBI][FREQ_50]+=4; //68


#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
/*  A strange aspect of the GLU is that it decides around cycle 54 how
    many cycles the scanline will be, like it had an internal variable for
    this, or various paths.
    What's more, the cycle where it's decided is about the same on the STE, 
    despite the fact that GLU DE starts earlier.
*/
  // Cases: Forest
  cycle_of_scanline_length_decision=GLU_DECIDE_NCYCLES+WU_sync_modifier;
  if(ST_TYPE==STE)
    cycle_of_scanline_length_decision+=2;
#endif

  // Top and bottom border, little trick here, we assume STF is in WU2
  ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50]=GLU_VERTICAL_OVERSCAN_50+2;
  if(ST_TYPE==STE||MMU.WU[WAKE_UP_STATE]==1)
    ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50]-=2;

  ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_60]
    =ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50];//?

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
  Status.hbi_done=Status.sdp_reload_done=false;
  Status.vbl_done=true;

  if(time_of_next_event-cpu_time_of_last_vbl==133604)
    Shifter.Preload=0; // according to ljbk

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK2&OSD_CONTROL_60HZ)
    TRACE_OSD("R%d S%d",m_ShiftMode,m_SyncMode);
#endif
#endif
}

#endif

TGlue Glue; // our one chip

#endif//SSE_GLUE && STEVEN_SEAGAL
