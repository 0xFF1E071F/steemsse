#include "SSE.h"

#if defined(SSE_GLUE)

#include "../pch.h"
#include <conditions.h>
#include <display.decla.h>
#include <emulator.decla.h>
#include <cpu.decla.h>
#include <mfp.decla.h>
#include <run.decla.h>

#include "SSEGlue.h"
#include "SSEMMU.h"
#include "SSEDebug.h"
#include "SSEInterrupt.h"
#include "SSEVideo.h"
#include "SSEShifter.h"
#include "SSEParameters.h"
#include "SSEFrameReport.h"
#include "SSECpu.h"

#define LOGSECTION LOGSECTION_VIDEO

#if  defined(SSE_SHIFTER_HSCROLL)
#define HSCROLL0 Shifter.hscroll0
#else
#define HSCROLL0 HSCROLL
#endif


TGlue::TGlue() {
  DE_cycles[2]=160;
  DE_cycles[0]=DE_cycles[1]=DE_cycles[2]<<1;
  Freq[FREQ_50]=50;
  Freq[FREQ_60]=60;
  Freq[FREQ_72]=72;	
  CurrentScanline.Cycles=scanline_time_in_cpu_cycles_8mhz[1]; 
}


void TGlue::AdaptScanlineValues(int CyclesIn) { 
  // on set sync or shift mode
  // on IncScanline (CyclesIn=-1)

  if(FetchingLine())
  {
#if defined(SSE_GLUE_392A)

    if((m_ShiftMode&2)) //currently in HIRES
    {
      if(CyclesIn<=ScanlineTiming[LINE_STOP][FREQ_72])
      {
        CurrentScanline.EndCycle=ScanlineTiming[LINE_STOP][FREQ_72];
        if(CyclesIn<=ScanlineTiming[CHOOSE_FREQ][FREQ_72])
        {
          CurrentScanline.StartCycle=ScanlineTiming[LINE_START][FREQ_72];
          if(shifter_hscroll_extra_fetch)
            CurrentScanline.StartCycle-=4;
        }
      }
    } // not in HIRES
    else if(CyclesIn<=ScanlineTiming[LINE_STOP][FREQ_60] 
      && !(CurrentScanline.Tricks&(TRICK_0BYTE_LINE|TRICK_LINE_MINUS_106
      |TRICK_LINE_PLUS_44|TRICK_LINE_MINUS_2)))
    {
      CurrentScanline.EndCycle= (m_SyncMode&2)
        ? ScanlineTiming[LINE_STOP][FREQ_50]
        : ScanlineTiming[LINE_STOP][FREQ_60];
      if(CyclesIn<=ScanlineTiming[CHOOSE_FREQ][FREQ_60]
        && !(CurrentScanline.Tricks
          &(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_20|TRICK_0BYTE_LINE)))
      {
        CurrentScanline.StartCycle=(m_SyncMode&2)
          ? ScanlineTiming[LINE_START][FREQ_50]
          : ScanlineTiming[LINE_START][FREQ_60];
        if(shifter_hscroll_extra_fetch) 
          //CurrentScanline.StartCycle-=16; // med res too... 
          CurrentScanline.StartCycle-=(m_ShiftMode&1)?8:16; // TODO test
      }
    }

#elif defined(SSE_GLUE_392E)
    // I admit I'm a bit confused by wake-state
    // When reading the video counter in the MMU, WS should have no
    // impact (eg Forest WS2)
    // So starting and ending cycles are not affected as far as the
    // MMU is concerned. We revert SSE_GLUE_392A.

    //currently in HIRES
    if((m_ShiftMode&2)) 
    {
      if(CyclesIn<=ScanlineTiming[LINE_STOP][FREQ_72])
      {
        CurrentScanline.EndCycle=ScanlineTiming[LINE_STOP][FREQ_72];
        if(CyclesIn<=ScanlineTiming[LINE_START][FREQ_72])
        {
          CurrentScanline.StartCycle=GLU_DE_ON_72;
          if(shifter_hscroll_extra_fetch)
            CurrentScanline.StartCycle-=4;
        }
      }
    } 
    // not in HIRES
    else if(CyclesIn<=ScanlineTiming[LINE_STOP][FREQ_60] 
      && !(CurrentScanline.Tricks&(TRICK_0BYTE_LINE|TRICK_LINE_MINUS_106
      |TRICK_LINE_PLUS_44|TRICK_LINE_MINUS_2)))
    {
      CurrentScanline.EndCycle=(m_SyncMode&2) ? GLU_DE_OFF_50 : GLU_DE_OFF_60;
      if(CyclesIn<=ScanlineTiming[LINE_START][FREQ_60]
        && !(CurrentScanline.Tricks
          &(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_20|TRICK_0BYTE_LINE)))
      {
        CurrentScanline.StartCycle=(m_SyncMode&2) ? GLU_DE_ON_50 :GLU_DE_ON_60;

        if(shifter_hscroll_extra_fetch) 
          CurrentScanline.StartCycle-=(m_ShiftMode&1)?8:16; // TODO test
      }
    }

#else

    if((m_ShiftMode&2)&&CyclesIn==-1) // at IncScanline
    {
      CurrentScanline.StartCycle
        =(shifter_hscroll_extra_fetch?2:6);
      CurrentScanline.EndCycle=ScanlineTiming[LINE_STOP][FREQ_72];
    }
    else
    {
      if(CyclesIn<=ScanlineTiming[LINE_START][FREQ_60] 
        && !(CurrentScanline.Tricks
          &(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_20|TRICK_0BYTE_LINE)))
      {
        CurrentScanline.StartCycle=(m_SyncMode&2)?56:52;
        if(shifter_hscroll_extra_fetch) // -16 pixel left border trick, HSCROLL = 0
          CurrentScanline.StartCycle-=16; // only if HSCROLL...
      }

      if(CyclesIn<=372 && !(CurrentScanline.Tricks
        &(TRICK_0BYTE_LINE|TRICK_LINE_MINUS_106|TRICK_LINE_PLUS_44)))
        CurrentScanline.EndCycle=(m_SyncMode&2)?376:372;
    }
#endif//#if defined(SSE_GLUE_392A)

#if defined(SSE_GLUE_392B)
/*  With regular HSCROLL, fetching starts earlier and ends at the same time
    as without scrolling.
    The extra words should be counted at the start of the line, not the end, 
    therefore it should "simply" be part of CurrentScanline.Bytes, but it
    complicates emulation... :)
*/
    if(CyclesIn<=CurrentScanline.StartCycle)
    {
      if(MMU.ExtraBytesForHscroll)
        CurrentScanline.Bytes-=MMU.ExtraBytesForHscroll;
      MMU.ExtraBytesForHscroll=0;
      if(shifter_hscroll_extra_fetch)
      {
        MMU.ExtraBytesForHscroll=(m_ShiftMode&2)?2:8-m_ShiftMode*4;
        CurrentScanline.Bytes+=MMU.ExtraBytesForHscroll;
      } 
    }
#elif defined(SSE_MMU)
/*  The number of words skipped at the end of the scanline is determined 
    when DE is asserted.
*/    
    if(CyclesIn<=CurrentScanline.StartCycle)
      if(shifter_hscroll_extra_fetch) 
        MMU.WordsToSkip=(m_ShiftMode&2)?1:4-m_ShiftMode*2;
      else
        MMU.WordsToSkip=0;
#endif

#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
    // a bit tricky, saves a variable + rewriting
    if(COLOUR_MONITOR && CyclesIn<ScanlineTiming[LINE_STOP][FREQ_72])
    {
      if((m_ShiftMode&2) && !(CurrentScanline.Tricks&TRICK_80BYTE_LINE))
      {
        CurrentScanline.Bytes-=80;
        CurrentScanline.Tricks|=TRICK_80BYTE_LINE;
      }
      else  if(!(m_ShiftMode&2) && (CurrentScanline.Tricks&TRICK_80BYTE_LINE))
      {
        CurrentScanline.Bytes+=80;
        CurrentScanline.Tricks&=~TRICK_80BYTE_LINE;
      }
    }
#endif
  }
#ifdef SSE_INT_MFP_OBJECT
#if defined(SSE_GLUE_392D) && !defined(SSE_INT_MFP_TIMER_B_392A) //MFD?
  if(OPTION_C2)
    MC68901.CalcCyclesFromHblToTimerB();
#endif
#endif

#if defined(SSE_INT_MFP_TIMER_B_392A)
  if(OPTION_C2)
    MC68901.ComputeNextTimerB(CyclesIn);
#endif

  if(CyclesIn<=cycle_of_scanline_length_decision)
  {
#if defined(SSE_TIMING_MULTIPLIER_392B)
    CurrentScanline.Cycles=scanline_time_in_cpu_cycles
#else
    CurrentScanline.Cycles=scanline_time_in_cpu_cycles_8mhz
#endif
      [(m_ShiftMode&2)&&((CyclesIn==-1)||PreviousScanline.Cycles!=224)
        ? 2 : shifter_freq_idx];
    prepare_next_event();
  }
}


void TGlue::CheckSideOverscan() {
/*  Various GLU and Shifter tricks can change the border size and the number 
    of bytes fetched from video RAM. 
    Those tricks can be used with two goals: use a larger display area
    (fullscreen demos are more impressive than borders-on demos), and/or
    scroll the screen by using "sync lines" (eg Enchanted Land, No Buddies Land).
    This function is a big extension of draw_check_border_removal().
*/

  ASSERT(FetchingLine());

#if defined(SSE_VS2008_WARNING_390) && defined(SSE_DEBUG)
  int t=0;
#else
  int t;
#endif

  WORD CyclesIn=LINECYCLES;
  short r0cycle=-1,r1cycle=-1,r2cycle=-1;
#if defined(SSE_VID_BORDERS)
  bool BigBorders=(SideBorderSize==VERY_LARGE_BORDER_SIDE  && border);
#endif

  ASSERT(screen_res<=2);

  /////////////
  // NO SYNC //
  /////////////

#if defined(SSE_GLUE_EXT_SYNC)//TODO other bit?
  if(m_SyncMode&1) // we emulate no genlock -> nothing displayed
    CurrentScanline.Tricks=TRICK_0BYTE_LINE;
#endif

  //////////////////////
  // HIGH RES EFFECTS //
  //////////////////////

#if defined(SSE_GLUE_HIRES_OVERSCAN)
/*  Yes! You may abuse the GLUE when you have a monochrome screen too.
    It's the same idea: changing shift mode when it's about to make 
    decisions.
    R0 parts in black not emulated, and visibly other things, very trashy
    effect for the moment (I like it!)
    I asked a member of AF for pics or vid but got nothing :(

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
#if defined(SSE_GLUE_392A)
      && CyclesIn>=ScanlineTiming[CHOOSE_FREQ][FREQ_72]
      && !(ShiftModeAtCycle(ScanlineTiming[CHOOSE_FREQ][FREQ_72])&2))
#else
      && CyclesIn>=ScanlineTiming[LINE_START][FREQ_72]
      && !(ShiftModeAtCycle(ScanlineTiming[LINE_START][FREQ_72])&2))
#endif
      
    {
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
      fetched_bytes_mod=-80;
      draw_line_off=true;
    }
    else if( !(CurrentScanline.Tricks&0x10) 
      && CyclesIn>=ScanlineTiming[LINE_STOP][FREQ_72]
      && !(ShiftModeAtCycle(ScanlineTiming[LINE_STOP][FREQ_72])&2))
    {
      // note 14x2=28
      // 6-166 DE; we deduce 166+28=194 HSYNC stops DE
      CurrentScanline.Tricks|=0x10; // we have no bit for +14, it's right off
      fetched_bytes_mod=14;
    }
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

  ////////////////////////////////////////
  //  LEFT BORDER OFF (line +26, +20)   //
  ////////////////////////////////////////

/*  To "kill" the left border, the program sets shift mode to 2 so that the 
    GLU thinks that it's a high resolution line starting, and asserts DE
    (Display Enable) 48 (60Hz) or 52 (50Hz) cycles sooner than for other 
    resolutions.
*/

#if defined(SSE_GLUE_LINE_PLUS_26)

  if(!(TrickExecuted&(TRICK_LINE_PLUS_20|TRICK_LINE_PLUS_26|TRICK_0BYTE_LINE)))
  {
#if defined(SSE_GLUE_392A)
#define lim_r2 ScanlineTiming[CHOOSE_FREQ][FREQ_72]
#else
#define lim_r2 Glue.ScanlineTiming[TGlue::LINE_START][TGlue::FREQ_72]
#endif
#define lim_r0 Glue.ScanlineTiming[TGlue::HBLANK_OFF][TGlue::FREQ_50]
    if(!(CurrentScanline.Tricks
      &(TRICK_LINE_PLUS_20|TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_24)))
    {
      r2cycle=PreviousChangeToHi(lim_r2+2);
      if(r2cycle!=-1 && r2cycle>=lim_r2-12 && r2cycle<=lim_r2) //TODO those limits?
      {
        r0cycle=NextChangeToLo(r2cycle);//0 (generally) or 1 (ST-CNX, HighResMode)
        if(r0cycle>=lim_r0-24 && r0cycle<=lim_r0
#if defined(SSE_GLUE_TCB)
/* SNYD/TCB misses left off at -28 in WS2
-32 - 004:I0020 032:E0004 512:#0000
-31 - 000:I0020 028:E0004 512:#0000
-30 - 002:I0020 030:E0004 364:P0000 384:S0000 468:S0000 484:P0000 492:c0900 512:c0900 512:T0100 512:#0000
-29 - 020:c0900 040:c0900 060:c0900 080:c0906 508:#0160
-28 - 000:R0002 008:R0000 372:S0000 380:S0002 440:R0002 452:R0000 508:T2000 508:#0160
-27 - 004:R0002 012:R0000 376:S0000 384:S0002 444:R0002 456:R0000 512:T2011 512:#0230
*/
          && r0cycle>lim_r2 
#endif
          ) 
        {
#if defined(SSE_GLUE_LINE_PLUS_20)
/*  Only on the STE, it is possible to create stable long overscan lines (no 
    left, no right border) without a "Shifter reset" switch (R2/R0  at around
    444/456).
    Those shift mode switches for left border removal produce a 20 bytes bonus
    instead of 26, and the total overscan line is 224 bytes instead of 230. 
    224/8=28,no rest => no Shifter confusion.

    Test cases: MOLZ, Riverside, EPSS...

    Observed on E605 planet, Sommarhack 2010 credits, Circus, no explanation
    yet: When HSCROLL is on, the switch to low-res may happen 4 cycles later.
*/
          if(ST_TYPE==STE && (r0cycle==4 || HSCROLL&&r0cycle==8))
            CurrentScanline.Tricks|=TRICK_LINE_PLUS_20;
          else
#endif
            CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
        }
      }
    }

    // action for line+26 & line+20
    if(CurrentScanline.Tricks&(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_20))
    {
      ASSERT(!(CurrentScanline.Tricks&TRICK_4BIT_SCROLL));
#if defined(SSE_GLUE_LINE_PLUS_20)
      if(CurrentScanline.Tricks&TRICK_LINE_PLUS_20)
      {
        ASSERT(ST_TYPE==STE);
        CurrentScanline.Bytes+=20;

#if defined(SSE_VID_BORDERS)
#if defined(SSE_GLUE_392C)
        if(BigBorders)
#else
        if(SideBorderSize!=ORIGINAL_BORDER_SIDE && border)
#endif
        {
#if defined(SSE_VID_BORDERS_LINE_PLUS_20)
/*  Display border, not video memory as first pixels for the line +20:
    MOLZ in 413x275 display mode.
*/
          left_border=12;
          shifter_draw_pointer+=8;
#endif
        }
        else
#endif
        {
#if defined(SSE_VID_BORDERS_LINE_PLUS_20)
          left_border=0;
#endif
        }

        TrickExecuted|=TRICK_LINE_PLUS_20;
#if defined(SSE_SHIFTER_UNSTABLE_380)
        Shifter.Preload=0; // again because we miss the real reset (We were greets)
#endif

        // correct alignemnt, eg Riverside leave
#if defined(SSE_VID_BORDERS) && defined(SSE_MMU)
        if(BigBorders)
          MMU.ShiftSDP(-8);
        else
#endif
          Shifter.HblPixelShift=-8;
#ifdef SSE_GLUE_392B
        if(shifter_hscroll_extra_fetch) // fetches at 2, 6, 10, 14
        {
          CurrentScanline.StartCycle=2;
          // if we're at cycle 4>2, we must correct here, AdaptScanlineValues 
          // won't do it... eg EPSS
          CurrentScanline.Bytes-=MMU.ExtraBytesForHscroll;//-2
          MMU.ExtraBytesForHscroll=8;
          CurrentScanline.Bytes+=MMU.ExtraBytesForHscroll;//+8
        }
        else
          CurrentScanline.StartCycle=18; // non-fetches at 2, 6, 10, 14
#else
        CurrentScanline.StartCycle=16;
#endif
      }
      else 
#endif//#if defined(SSE_GLUE_LINE_PLUS_20)
      {
/*  A 'left off' grants 26 more bytes, that is 52 pixels (in low res) from 6 to
    58 at 50hz. Confirmed on real STE: Overscan Demos F6
    There's no "shift" hiding the first 4 pixels but the shift is necessary for
    Steem in 384 x 270 display mode: border = 32 pixels instead of 52.
    16 pixels skipped by manipulating video counter, 4 more to skip
 */
        CurrentScanline.Bytes+=26;
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
        if(!BigBorders)
#endif
          shifter_pixel+=4;
        if(HSCROLL0>=12) // 12+4 = 16 -> shift SDP before rendering
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
          if(!BigBorders) 
#endif
            MMU.ShiftSDP(8);
    
#if defined(SSE_VID_BORDERS) 
        if(!BigBorders)
#endif
          shifter_draw_pointer+=8; // 8+16+2=26

        TrickExecuted|=TRICK_LINE_PLUS_26;
        
#if defined(SSE_VID_BORDERS_LINE_PLUS_20)
        left_border=0;
#endif

#if defined(SSE_GLUE_392E)
        CurrentScanline.StartCycle=GLU_DE_ON_72;
        if(shifter_hscroll_extra_fetch)
          CurrentScanline.StartCycle-=4;
#elif defined(SSE_GLUE_392A)
        CurrentScanline.StartCycle=(shifter_hscroll_extra_fetch) 
          ? ScanlineTiming[CHOOSE_FREQ][FREQ_72]
          : ScanlineTiming[LINE_START][FREQ_72];
#else
        CurrentScanline.StartCycle=(HSCROLL0?2:6);
#endif
        // additional hacks for left off
        /////////////////////////////////////////////////////////

#if defined(SSE_SHIFTER_HSCROLL)
/* 
Big Wobble 
-27 - 000:@000B 000:@FD80 404:C0505 428:C079A 452:C0912 476:L0124 500:H000F 512:R0082 516:a0320 512:TC000 512:#0160
-26 - 000:@0005 000:@9A12 012:R00F0 376:S00F0 384:S0082 444:R0082 456:R00F0 512:R0082 516:a0480 512:T2011 512:#0230
-25 - 000:@0005 000:@9BF2 012:R00F0 376:S00F0 384:S0082 444:R0082 456:R00F0 512:R0082 516:a0480 512:T2011 512:#0230

D4/Tekila
-29 - 000:@0006 000:@0000 012:S0082 128:c0920 240:V0006 256:M0000 272:C0900 508:R0082 512:a0168 512:T4000 512:#0160
-28 - 000:@0006 000:@0040 012:R0000 376:S0000 388:S0082 408:C0506 424:C07B8 436:C096E 444:R0082 456:R0000 468:H000A 508:R0082 512:a0238 512:T6011 512:#0230
-27 - 000:@0006 000:@B888 012:R0000 376:S0000 388:S0082 408:C0506 424:C07B9 436:C095E 444:R0082 456:R0000 468:H000A 508:R0082 512:a0240 512:T6011 512:#0230

  When HSROLL is on, there's a left border corresponding to scrolling 
  time in low res.
  Also, timing of switch back to R0 may cause shifted planes.
  HSCROLL + 012:R0000: 2 word shift
*/
        if(OPTION_HACKS && HSCROLL0 && r0cycle==12)
        {
          ASSERT(ST_TYPE==STE);
          MMU.ShiftSDP(4); // 2 words lost in the Shifter? 
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
          if(BigBorders)
#endif
            left_border=16; // border visible on real STE
        }
#endif//SSE_SHIFTER_HSCROLL

#if defined(SSE_SHIFTER_UNSTABLE_380)
        if((r0cycle-r2cycle)==16 && (r0cycle!=12||!OPTION_HACKS)) // 12: Ventura/Naos WS2
        {
          if(OPTION_WS && !(CurrentScanline.Tricks&TRICK_UNSTABLE))
          {
            if(ST_TYPE==STE)
            {
              Shifter.Preload=2;
#if defined(SSE_GLUE_CLOSURE)
// hack Closure STE TODO, we know that's not correct
//008:S0082 016:R0000 376:S0000 512:R0082 516:a0230 512:T0011
              if(OPTION_HACKS && FreqChangeAtCycle(8)==50) 
              {
                if(shifter_pixel<=4)
                  shifter_pixel+=6; // hicolor pics shift
              }
              else
#endif
              {
/*  
On real STE, there's a horrible glitch in the left border, it looks
better in Steem (we don't emulate all trash yet)
Xmas 2004
000 - 000:@000D 000:@0000 016:c0900 040:c0900 064:c0900 088:c090C 424:L0140 444:H0006 492:M0000 496:C050D 500:C0701 504:C0922 512:R0002 516:a0160 512:TC000 512:#0160
001 - 000:@000D 000:@0122 016:R0000 376:S0000 384:S0002 448:R0002 456:R0000 512:R0002 516:a0512 512:T12051 512:#0230
112 - 000:@000D 000:@DF22 016:R0000 376:S0000 384:S0002 448:R0002 456:R0000 512:a0512 512:T12051 512:#0230
113 - 000:@000D 000:@E122 424:L0140 444:H000A 492:M0000 496:C0508 500:C0754 504:C09B6 512:R0002 516:a0448 512:TC000 512:#0160
114 - 000:@0008 000:@54B6 016:R0000 376:S0000 384:S0002 448:R0002 456:R0000 512:R0002 516:a0512 512:T12051 512:#0230
170 - 000:@0008 000:@C4B6 016:R0000 376:S0000 384:S0002 448:R0002 456:R0000 512:R0002 516:a0512 512:T12051 512:#0230
171 - 000:@0008 000:@C6B6 016:R0000 376:S0000 384:S0002 448:R0002 456:R0000 512:a0512 512:T12051 512:#0230
172 - 000:@0008 000:@C8B6 492:M0000 496:C0508 500:C07C4 504:C09B6 512:R0002 516:a0448 512:TC000 512:#0160
173 - 000:@0008 000:@C4B6 016:R0000 376:S0000 384:S0002 448:R0002 456:R0000 492:M0000 496:C0508 500:C07C2 504:C09B6 512:R0002 516:a0512 512:T1E051 512:#0230
174 - 000:@0008 000:@C2B6 016:R0000 376:S0000 384:S0002 448:R0002 456:R0000 492:M0000 496:C0508 500:C07BE 504:C09B6 512:R0002 516:a0512 512:T1E051 512:#0230
Kryos
-30 - 456:S00FC 524:S00FE 512:T0100 512:#0000
-29 - 000:@001F 000:@8000 108:c0916 464:C0506 480:C07F4 496:C092E 512:R0002 516:a0160 512:TC000 512:#0160
-28 - 000:@0006 000:@F42E 016:R0000 376:S0000 392:S0002 512:R0002 516:a0230 512:T10051 512:#0230
-14 - 000:@0007 000:@00C2 016:R0000 376:S0000 392:S0002 444:C050C 460:C0748 476:C095A 512:R0002 516:a0230 512:T1C051 512:#0230
199 - 000:@000D 000:@06D2 016:R0000 376:S0000 392:S0002 496:S0000 512:R0002 516:a0230 512:T10251 512:#0230
200 - 000:@000D 000:@07B8 016:R0000 032:S0002 352:C0900 376:S0000 392:S0002 416:L0131 432:C0506 448:C078E 464:C09A0 480:H0008 512:R0002 516:a0492 512:T14051 512:#0230
201 - 000:@0006 000:@8FAA 016:R0000 376:S0000 392:S0002 512:R0002 516:a0494 512:T10051 512:#0230
*/
                ASSERT(r0cycle-r2cycle==16);
                ASSERT(ST_TYPE==STE);
                ASSERT(Shifter.Preload==2);
                // HSCROLL + 016:R0000: 3 word shift
                MMU.ShiftSDP( HSCROLL0 ? 2 : 8 );
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
                if(BigBorders)
#endif
                  left_border=16;
              }
            }
            else
            {
              ASSERT(ST_TYPE==STF);
/*
Closure STF2
-30 - 000:I0020 472:S0000 512:T0100 512:#0000
-29 - 000:@000D 000:@0000 004:R0082 012:S0082 020:R0000 376:S0000 512:a0230 512:T10051 512:#0230
-28 - 000:@000D 000:@00E6 004:R0082 012:S0082 020:R0000 376:S0000 512:a0230 512:T10051 512:#0230
*/
              Shifter.Preload=1; // Closure STF WS2
              if(r0cycle==16)
                Shifter.Preload++; // DOLB STF
              else if(shifter_pixel<=4)
                shifter_pixel+=4; // hicolor pics shift
            }
            CurrentScanline.Tricks|=TRICK_UNSTABLE;
#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL)
            if((OSD_MASK2 & OSD_CONTROL_PRELOAD) && Shifter.Preload)
              TRACE_OSD("y %d R2 %d R0 %d LOAD %d",scan_y,r2cycle,r0cycle,Shifter.Preload);
#endif
          }
        }//if((r0cycle-r2cycle)==16)
#endif//SSE_SHIFTER_UNSTABLE_380
      }//+20 or +26
#if !defined(SSE_VID_BORDERS_LINE_PLUS_20)
      left_border=0;
#endif
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS) && !defined(SSE_GLUE_392D)
      if(OPTION_C2)
        MC68901.AdjustTimerB(); //isn't it too late? TODO
#endif
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
      overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
    }
#undef lim_r2
#undef lim_r0
  }
#endif//#if defined(SSE_GLUE_LINE_PLUS_26)

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

#if defined(SSE_GLUE_0BYTE_LINE)
/*  Here we test for 0byte line at the start of the scanline, affecting
    current scanline.

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

  if(!(CurrentScanline.Tricks&(TRICK_0BYTE_LINE|TRICK_LINE_PLUS_26
    |TRICK_LINE_PLUS_20
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
    |TRICK_80BYTE_LINE
#endif
    )))
  {
#define r2cycle ScanlineTiming[HBLANK_OFF][FREQ_50]
#if defined(SSE_GLUE_392A)
#define s0cycle ScanlineTiming[CHOOSE_FREQ][FREQ_60]
#define s2cycle ScanlineTiming[CHOOSE_FREQ][FREQ_50]
#else
#define s0cycle ScanlineTiming[LINE_START][FREQ_60]
#define s2cycle ScanlineTiming[LINE_START][FREQ_50]
#endif
    ASSERT(s0cycle>0);
    ASSERT(s2cycle>0);
    if(ST_TYPE!=STE //our new hypothesis
      && CyclesIn>=r2cycle && (ShiftModeAtCycle(r2cycle)&2)) // hblank trick?
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
    else if(CyclesIn>=s2cycle  // DE trick? (mode + sync)
      && FreqAtCycle(s2cycle)!=50 && FreqAtCycle(s0cycle)!=60)
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
    CurrentScanline.Bytes=0;
#if defined(SSE_SHIFTER_UNSTABLE_380)
    Shifter.Preload=0; // for Forest after the line +2... should we?
#endif
    TrickExecuted|=TRICK_0BYTE_LINE;
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
    CurrentScanline.StartCycle=CurrentScanline.EndCycle=-1;//never starts
    if(OPTION_C2)
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

#if defined(SSE_GLUE_BLACK_LINE)

  if(!draw_line_off) // equivalent to "trick executed"
  {
    // test
    if(CyclesIn>=ScanlineTiming[HBLANK_OFF][FREQ_50] 
    && FreqAtCycle(ScanlineTiming[HBLANK_OFF][FREQ_60])==50 
      && FreqAtCycle(ScanlineTiming[HBLANK_OFF][FREQ_50])==60)
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
#endif//#if defined(SSE_GLUE_BLACK_LINE)

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

    v3.8: we've added support for Closure STF WS1,3,4 in this part

    TODO: of course, one day we should unify analysis for Med Red Overscan, 
    4bit hardscroll and Closure-like overscan
*/

#if defined(SSE_SHIFTER_MED_OVERSCAN)
  if(!left_border && !(TrickExecuted&(TRICK_OVERSCAN_MED_RES|TRICK_0BYTE_LINE)))
  {
    // look for switch to R1
    r1cycle=CycleOfLastChangeToShiftMode(1);
#if defined(SSE_SHIFTER_UNSTABLE_380)
    if(r1cycle>=8 && r1cycle<=40) // for Closure, further
#else
    if(r1cycle>16 && r1cycle<=40)
#endif
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
#endif
#if defined(SSE_VID_BORDERS) && defined(SSE_VID_BPOC)
        if(OPTION_HACKS && cycles_in_low_res==16) 
          if(SideBorderSize==VERY_LARGE_BORDER_SIDE && border)
#if defined(SSE_VID_D3D_ONLY) || !defined(SSE_VID_BORDERS_LB_DX)
            shifter_pixel+=1;
#elif defined(SSE_VID_BORDERS_416_NO_SHIFT)
            shifter_pixel+=(BORDER_40?4:1);
#endif
#if defined(SSE_VID_BORDERS_LB_DX) && !defined(SSE_VID_D3D_ONLY) 
          else if(BORDER_40)
          { // fit text of Best Part of the Creation on a 800 display (pure hack)
            MMU.ShiftSDP(4);      
            shifter_pixel+=4; 
          }
#endif
#endif
      }
#if defined(SSE_SHIFTER_UNSTABLE_380)
/*  Demo Closure uses a new technique (2015!) to remove the left border without
    a stabiliser on an STF.
    My understanding is that one shift compensates the other, but at this point,
    we're just adding some hacks to support the demo in STF mode, WS 1,3,4

    008:R0001 016:R0000 024:S0082 376:S0000 512:R0082 512:T10011 512:#0230 
*/
      else if(OPTION_WS  && !(CurrentScanline.Tricks&TRICK_4BIT_SCROLL)) //just avoiding interference
      {
        r0cycle=NextShiftModeChange(r1cycle);
        if(r0cycle!=-1 && r0cycle>r1cycle // Nightmare
          && r0cycle<Glue.ScanlineTiming[TGlue::LINE_STOP][TGlue::FREQ_50])
        {
          Shifter.Preload=1;
          CurrentScanline.Tricks|=TRICK_UNSTABLE;
          // fix hicolor pics shift
          if(shifter_pixel<=4)
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
            shifter_pixel+=(BigBorders)?2:4;
#else
            shifter_pixel+=4;
#endif
#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL)
          if((OSD_MASK2 & OSD_CONTROL_PRELOAD) && Shifter.Preload)
            TRACE_OSD("y %d R1 %d R0 %d LOAD %d",scan_y,r1cycle,r0cycle,Shifter.Preload);
#endif 
        }
      }
    }
  }
#endif//SSE_SHIFTER_UNSTABLE_380
#endif//SSE_SHIFTER_MED_OVERSCAN

  /////////////////////
  // 4BIT HARDSCROLL //
  /////////////////////

/*  When the left border is removed, a MED/LO switch causes the Shifter to
    shift the line by a number of pixels dependent on the cycles at which
    the switch occurs. It doesn't change the # bytes fetched (230 in known 
    cases). How exactly it happens is worth a study (TODO). 
    It's a pity that it wasn't used more. Was it a French technique?
    I'm not sure why it's called 4bit, but it could be because possible pixel
    offsets are 4 pixels distant (-7,-3,1,5).
    PYM/ST-CNX (lines 70-282 (7-219); offset 8, 12, 16, 20) 0R2 12R1 XR0
007 - 012:R0001 032:R0000 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2031 512:#0230
    D4/NGC (lines 76-231) 0R2 12R0 20R1 XR0
015 - 012:R0000 020:R0001 036:R0000 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2071
    D4/Nightmare (lines 143-306, cycles 28 32 36 40)
    Apparently, the ST-CNX technique, R2-R1-R0, works only on STF
    The Delirious technique, R2-R0-R1-R0 should work on STE, but possibly with
    Shifter bands.
*/
#if defined(SSE_SHIFTER_4BIT_SCROLL)

  if(!left_border && !(CurrentScanline.Tricks
    &(TRICK_0BYTE_LINE|TRICK_4BIT_SCROLL)))
  {
    ASSERT(!(TrickExecuted&TRICK_4BIT_SCROLL));
    
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
        ASSERT(!(CurrentScanline.Tricks&TRICK_LINE_MINUS_106));
#if defined(SSE_STF)
        if(ST_TYPE==STF || cycles_in_low_res)
#endif
          CurrentScanline.Tricks|=TRICK_4BIT_SCROLL;
        TrickExecuted|=TRICK_4BIT_SCROLL;

        MMU.ShiftSDP(shift_in_bytes);
        Shifter.HblPixelShift=13+8-cycles_in_med_res-8; // -7,-3,1, 5, done in Render()
#if defined(SSE_SHIFTER_UNSTABLE_380)
        Shifter.Preload=0; // ! it's temporary, avoid conflict with old system
#endif
#if defined(SSE_DEBUG)
        ASSERT( Shifter.HblPixelShift==-7||Shifter.HblPixelShift==-3||Shifter.HblPixelShift==1||Shifter.HblPixelShift==5 );
#endif
      }
    }
  }
#endif

  /////////////////
  // LINE +4, +6 // //TODO
  /////////////////
/*
    (60hz)
    40  IF(RES == HI) PRELOAD will exit after 4 cycles  (372-40)/2 = +6  
    44  IF(RES == HI) PRELOAD will exit after 8 cycles  (372-44)/2 = +4 

    (50hz)
    44  IF(RES == HI) PRELOAD will exit after 4 cycles  (376-44)/2 = +6  
    48  IF(RES == HI) PRELOAD will exit after 8 cycles  (376-48)/2 = +4 

    We need test cases
*/

#if defined(SSE_GLUE_LINE_PLUS_4) || defined(SSE_GLUE_LINE_PLUS_6)
  // this compiles into compact code
  if(ST_TYPE==STE && !shifter_hscroll_extra_fetch && CyclesIn>36 
    && !(TrickExecuted&(TRICK_0BYTE_LINE | TRICK_LINE_PLUS_26
    | TRICK_LINE_PLUS_20 | TRICK_4BIT_SCROLL | TRICK_OVERSCAN_MED_RES
    | TRICK_LINE_PLUS_4 | TRICK_LINE_PLUS_6)))
  {
#if defined(SSE_GLUE_392A)
    t=FreqAtCycle(ScanlineTiming[CHOOSE_FREQ][FREQ_60]==50) ? 44: 40;
#else
    t=FreqAtCycle(ScanlineTiming[LINE_START][FREQ_60]==50) ? 44: 40;
#endif
    if(ShiftModeChangeAtCycle(t)==2)
    {
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_6;
#if defined(SSE_GLUE_LINE_PLUS_6) 
      left_border-=2*6; 
      CurrentScanline.Bytes+=6;
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
      overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
      TrickExecuted|=TRICK_LINE_PLUS_6;
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK_14 & TRACE_CONTROL_LINE_PLUS_2)
        TRACE_OSD("+6 y%d",scan_y);
#endif
#endif
    }
    else if(ShiftModeChangeAtCycle(t+4)==2)
    {
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_4;
#if defined(SSE_GLUE_LINE_PLUS_4)
      left_border-=2*4; 
      CurrentScanline.Bytes+=4;
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
      overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
      TrickExecuted|=TRICK_LINE_PLUS_4;
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK_14 & TRACE_CONTROL_LINE_PLUS_2)
        TRACE_OSD("+4 y%d",scan_y);
#endif
#endif
    }
  }

#endif

  /////////////
  // LINE +2 //
  /////////////

/*  A line that starts as a 60hz line and ends as a 50hz line gains 2 bytes 
    because 60hz lines start and stop 4 cycles before 50hz lines.
    This is used in some demos, but most cases are accidents, especially
    on the STE: the GLU checks frequency earlier because of possible horizontal
    scrolling, and this may interfere with the trick that removes top or bottom
    border.

    Positive cases: Mindbomb/No Shit, Forest, Bees, LoSTE screens, Closure
    Spurious to avoid as STE (emulation problem): DSoTS, Overscan,
    Spurious unavoidable as STE (correct emulation): BIG Demo #1, 
    nordlicht_stniccc2015_partyversion, Decade menu, ...
    Spurious to avoid as STF: Panic, 3615GEN4-CKM
*/

#if defined(SSE_GLUE_LINE_PLUS_2)

  ASSERT(screen_res<2);
  if(!(CurrentScanline.Tricks&
    (TRICK_0BYTE_LINE|TRICK_LINE_PLUS_2|TRICK_LINE_PLUS_4|TRICK_LINE_PLUS_6
      |TRICK_LINE_PLUS_20|TRICK_LINE_PLUS_26)))
  {
#if defined(SSE_GLUE_392A)
    t=ScanlineTiming[CHOOSE_FREQ][FREQ_60];
    if(CyclesIn>=t && FreqAtCycle(t)==60 
      && ((CyclesIn<ScanlineTiming[LINE_STOP][FREQ_60] && shifter_freq==50) 
      || CyclesIn>=ScanlineTiming[LINE_STOP][FREQ_60] 
      && FreqAtCycle(ScanlineTiming[LINE_STOP][FREQ_60])==50))
#else
    t=Glue.ScanlineTiming[TGlue::LINE_START][TGlue::FREQ_60];
    if(CyclesIn>=t && FreqAtCycle(t)==60 
      && ((CyclesIn<Glue.ScanlineTiming[TGlue::LINE_STOP][TGlue::FREQ_60] 
      && shifter_freq==50) 
      || CyclesIn>=Glue.ScanlineTiming[TGlue::LINE_STOP][TGlue::FREQ_60] &&
      FreqAtCycle(Glue.ScanlineTiming[TGlue::LINE_STOP][TGlue::FREQ_60])==50))
#endif

      CurrentScanline.Tricks|=TRICK_LINE_PLUS_2;
  }

  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_2)
    && !(TrickExecuted&TRICK_LINE_PLUS_2))
  {
    CurrentScanline.Bytes+=2;
#if defined(SSE_SHIFTER_UNSTABLE_380)
 /* In NPG_WOM, there's an accidental +2 on STE, but the scroll flicker isn't
    ugly.
    It's because those 2 bytes unbalance the Shifter by one word, and the screen
    gets shifted, also next frame, and the graphic above the scroller is ugly.
    It also fixes nordlicht_stniccc2015_partyversion.
    update: on real STE, flicker depends on some unidentified WS
 */
#if defined(SSE_SHIFTER_UNSTABLE_392) // we add (spurious) conditions
    if(ST_TYPE!=STE&&MMU.WS[OPTION_WS]==1||ST_TYPE==STE&&MMU.WS[OPTION_WS]!=1)
#endif
    if(!(PreviousScanline.Tricks&TRICK_LINE_MINUS_2)) // BIG demo #1 STE
      Shifter.Preload=1; // assume = 0? problem, we don't know where it's reset
#endif
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
    overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
    TrickExecuted|=TRICK_LINE_PLUS_2;
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK_14 & TRACE_CONTROL_LINE_PLUS_2)
    {
      if(TRACE_ENABLED(LOGSECTION_VIDEO))
        REPORT_LINE;
      TRACE_LOG("+2 y%d c%d %d %d\n",scan_y,NextFreqChange(t,50),t,CyclesIn);
      TRACE_OSD("+2 y%d c%d %d %d",scan_y,NextFreqChange(t,50),t,CyclesIn);
    }
#endif
  }
#endif//+2

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
    R2 56 [...] -> 164 [WS1] 166 [WS3,4] 168 [WS2]
*/

#if defined(SSE_GLUE_LINE_MINUS_106)

  // test
  if(!(CurrentScanline.Tricks&(TRICK_LINE_MINUS_106|TRICK_0BYTE_LINE))
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
    && !((CurrentScanline.Tricks&TRICK_80BYTE_LINE)&&(m_ShiftMode&2))
#endif
    && CyclesIn>=ScanlineTiming[LINE_STOP][FREQ_72]
    && (ShiftModeAtCycle(ScanlineTiming[LINE_STOP][FREQ_72])&2))
     CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;

  // action
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_106)
    && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
    ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );
    ASSERT( !(CurrentScanline.Tricks&TRICK_4BIT_SCROLL) );
//    ASSERT( !(CurrentScanline.Tricks&TRICK_80BYTE_LINE) );
    TrickExecuted|=TRICK_LINE_MINUS_106;

#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY) 
    //must anticipate, why?
    if((CurrentScanline.Tricks&TRICK_80BYTE_LINE) && !(m_ShiftMode&2))
    {
      CurrentScanline.Bytes+=80;
      CurrentScanline.Tricks&=~TRICK_80BYTE_LINE;
    }
#endif
    CurrentScanline.Bytes-=106;
#if defined(SSE_SHIFTER_UNSTABLE_380)
    Shifter.Preload=0;  //= registers reset ... chipman in Closure
#endif
#if defined(SSE_SHIFTER_LINE_MINUS_106_BLACK)
/*  The MMU won't fetch anything more, and as long as the ST is in high res,
    the scanline is black, but Steem renders the full scanline in colour.
    It's in fact data of next scanline. We make sure nothing ugly will appear
    (loSTE screens).
    In some cases, maybe we should display border colour after R0?
*/
    draw_line_off=true;
    memset(PCpal,0,sizeof(long)*16); // all colours black
#endif
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
    if(OPTION_C2)
      MC68901.AdjustTimerB(); // Fixes 1st value of TIMERB04.TOS
#endif
  }
#endif//#if defined(SSE_GLUE_LINE_MINUS_106)

  /////////////////////
  // DESTABILISATION //
  /////////////////////

  // In fact, this is a real Shifter trick and ideally would be in SSEShifter.cpp!

#if defined(SSE_SHIFTER_MED_RES_SCROLLING)
/*  Detect MED/LO switches during DE.
    This will load some words into the Shifter, the Shifter will start displaying
    shifted planes and shifted pixels in low res.
    There was very early Steem support for those new techniques, and it's been 
    disappointing to me that it sort of fizzled out.
    Update: technique used again in 2016!
    Note we have 1-4 words in the Shifter, and there's no restabilising at the
    start of next scanline.
*/
/*
ljbk:
detect unstable: switch MED/LOW - Beeshift
- 3 (screen shifted by 12 pixels because only 1 word will be read before the 4 are available to draw the bitmap);
- 2 (screen shifted by 8 pixels because only 2 words will be read before the 4 are available to draw the bitmap);
- 1 (screen shifted by 4 pixels because only 3 words will be read before the 4 are available to draw the bitmap);
- 0 (screen shifted by 0 pixels because the 4 words will be read to draw the bitmap);
*/
  if(OPTION_WS && !(CurrentScanline.Tricks&(TRICK_UNSTABLE|TRICK_0BYTE_LINE
    |TRICK_LINE_PLUS_26|TRICK_LINE_MINUS_106|TRICK_LINE_MINUS_2
    |TRICK_4BIT_SCROLL)) && FetchingLine()) 
  {

    // detect switch to medium or high during DE (more compact code)
    int mode;
    r1cycle=NextShiftModeChange(ScanlineTiming[LINE_START][FREQ_50]); 
    if(r1cycle>ScanlineTiming[LINE_START][FREQ_50] 
    && r1cycle<=ScanlineTiming[LINE_STOP][FREQ_50]
    && (mode=ShiftModeChangeAtCycle(r1cycle))!=0) //!=0 for C4706
    {
      r0cycle=NextShiftModeChange(r1cycle,0); // detect switch to low
      int cycles_in_med_or_high=r0cycle-r1cycle;
      if(r0cycle<=ScanlineTiming[LINE_STOP][FREQ_50]&&cycles_in_med_or_high>0)
      {
        Shifter.Preload=((cycles_in_med_or_high/4)%4);
        if((mode&2)&&Shifter.Preload&1) // if it's 3 or 5
          Shifter.Preload+=(4-Shifter.Preload)*2; // swap value
        CurrentScanline.Tricks|=TRICK_UNSTABLE;//don't apply on this line (!)
#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL)
        if((OSD_MASK2 & OSD_CONTROL_PRELOAD) && Shifter.Preload)
        {
          TRACE_OSD("R%d %d R0 %d LOAD %d",mode,r1cycle,r0cycle,Shifter.Preload);
          TRACE_LOG("y%d R%d %d R0 %d Preload %d Tricks %X\n",scan_y,mode,r1cycle,r0cycle,Shifter.Preload,CurrentScanline.Tricks);
        }
#endif
      }
    }
  }

#endif

#if defined(SSE_SHIFTER_UNSTABLE)
/*  Shift due to unstable Shifter, caused by a line+26 without stabiliser
    (Dragon) or a MED/LOW switch during DE, or a HI/LOW switch during DE, 
    or a line+230 without stabiliser: apply effect
    Note this isn't exact science, it's in development.
    DOLB, Omega: 3 words preloaded but it becomes 2 due to left off
    Dragon: 1 word preloaded    
    It could be that both determining preload and using preload are wrong!
    v3.6.0: it's one word for +26 or 230, because "right off" doesn't load
    44 extra bytes in the Shifter, even if those are fetched. Loading stops
    at HBLANK, before HSYNC.
    ignore when line +2 (TODO)
*/
  
  if(OPTION_WS && Shifter.Preload && 
    !(CurrentScanline.Tricks&(TRICK_0BYTE_LINE|TRICK_LINE_PLUS_2))
    && !(TrickExecuted&TRICK_UNSTABLE))
  {

    // 1. planes
    int shift_sdp=-((Shifter.Preload)%4)*2; // unique formula
    MMU.ShiftSDP(shift_sdp); // correct plane shift

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
#if defined(SSE_SHIFTER_UNSTABLE_390) //interference with Closure STE
    if(OPTION_HACKS && MMU.WS[OPTION_WS]==2&&Shifter.Preload==2 &&ST_TYPE!=STE)
#else
    if(OPTION_HACKS && MMU.WS[OPTION_WS]&&Shifter.Preload==2)
#endif
    {
#if defined(SSE_SHIFTER_DOLB1)
      MMU.ShiftSDP(8); // again...
#endif
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
      if(!BigBorders)
#endif
        Shifter.HblPixelShift=4; 
    }
#endif
    //TRACE_LOG("Y%d Preload %d shift SDP %d pixels %d lb %d rb %d\n",scan_y,Preload,shift_sdp,HblPixelShift,left_border,right_border);
    TrickExecuted|=TRICK_UNSTABLE;
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

#if defined(SSE_GLUE_LINE_MINUS_2)

  // test
  if(!(CurrentScanline.Tricks
    &(TRICK_0BYTE_LINE|TRICK_LINE_MINUS_106|TRICK_LINE_MINUS_2))
#if defined(SSE_INT_MFP_TIMER_B_392D) // check Lethal Xcess beta, TB2/Oxy
    && CyclesIn>ScanlineTiming[LINE_STOP][FREQ_60] // > 372
#else
    && CyclesIn>=ScanlineTiming[LINE_STOP][FREQ_60] // we're at 372 min
#endif
#if defined(SSE_GLUE_392A)
    && FreqAtCycle(ScanlineTiming[CHOOSE_FREQ][FREQ_60])!=60  //50,72?
#else
    && FreqAtCycle(ScanlineTiming[LINE_START][FREQ_60])!=60  //50,72?
#endif
    && FreqAtCycle(ScanlineTiming[LINE_STOP][FREQ_60])==60)
     CurrentScanline.Tricks|=TRICK_LINE_MINUS_2;

  //  action
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
    &&!(TrickExecuted&TRICK_LINE_MINUS_2))
  {
#if defined(SSE_SHIFTER_UNSTABLE_380)
    if(Shifter.Preload)
      Shifter.Preload=(Shifter.Preload%4)-1;
    if(!Shifter.Preload && (CurrentScanline.Tricks&TRICK_UNSTABLE))
      CurrentScanline.Tricks&=~TRICK_UNSTABLE;
#endif
    CurrentScanline.Bytes-=2; 
    TrickExecuted|=TRICK_LINE_MINUS_2;

#ifdef TEST02
    //CurrentScanline.EndCycle-=4;
#endif

//    TRACE_LOG("-2 y %d c %d s %d e %d ea %d\n",scan_y,LINECYCLES,scanline_drawn_so_far,overscan_add_extra,ExtraAdded);
  }

#endif//#if defined(SSE_GLUE_LINE_MINUS_2)

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

    Switch to 60hz  374 - 376 WS1,3
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

TODO Closure doesn't agree with 'Bees' for WS1?
*/

#if defined(SSE_GLUE_LINE_PLUS_44)

  // test
#if defined(SSE_GLUE_RIGHT_OFF_BY_SHIFT_MODE)
  t=ScanlineTiming[LINE_STOP][FREQ_50]
#if defined(SSE_MMU_WU)
  +MMU.ResMod[OPTION_WS]-MMU.FreqMod[OPTION_WS]
#endif
  +2;
#endif

#if defined(SSE_INT_MFP_TIMER_B_392D) // TB2/OXY
  if(CyclesIn<ScanlineTiming[LINE_STOP][FREQ_50] 
#else
  if(CyclesIn<=ScanlineTiming[LINE_STOP][FREQ_50] 
#endif
    ||(CurrentScanline.Tricks&(TRICK_0BYTE_LINE|TRICK_LINE_MINUS_2
    |TRICK_LINE_MINUS_106|TRICK_LINE_PLUS_44))
    || FreqAtCycle(ScanlineTiming[LINE_STOP][FREQ_60])==60)
    ; // no need to test
#if defined(SSE_INT_MFP_TIMER_B_392D)
  else if(CyclesIn==ScanlineTiming[LINE_STOP][FREQ_50] && !(m_SyncMode&2) // now!
    || FreqAtCycle(ScanlineTiming[LINE_STOP][FREQ_50])==60
#else
  else if(FreqAtCycle(ScanlineTiming[LINE_STOP][FREQ_50])==60
#endif
#if defined(SSE_GLUE_RIGHT_OFF_BY_SHIFT_MODE)
/*  Like Alien said, it is also possible to remove the right border by setting
    the shiftmode to 2. This may be done well before cycle 376.
    WS threshold: WS1:376  WS3,4:378  WS2:380 (weird!)

    Note: if a program can test a 2 cycle difference for 72/50, then
    it points to the GLU having this 2 cycle precision, in other
    words write to RES isn't rounded up to 4 as far as the GLU is
    concerned.
*/
    || CyclesIn>=t && (ShiftModeAtCycle(t)&2)
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
    TrickExecuted|=TRICK_LINE_PLUS_44;
    CurrentScanline.Bytes+=44;
#if defined(SSE_GLUE_392E)
    CurrentScanline.EndCycle=GLU_HSYNC_ON_50; //376 + 44*2 = 464
#elif defined(SSE_GLUE_392A) // I thought it would break things... 
    CurrentScanline.EndCycle=ScanlineTiming[HSYNC_ON][FREQ_50];
#else
    CurrentScanline.EndCycle=464;
#endif
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
    if(OPTION_C2)
      MC68901.AdjustTimerB();
#endif
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
    overscan=OVERSCAN_MAX_COUNTDOWN; 
#endif
  }

#endif//#if defined(SSE_GLUE_LINE_PLUS_44)


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
    note: it should be in Shifter object, but we don't split tests further (?)
Phaleon shadow 
119 - 012:R0000 376:S0000 384:S0002 436:R0000 444:R0002 456:R0000 508:R0002 512:T0011 512:#0230
Dragonnels reset
-25 - 012:R0001 376:S0000 392:S0002 448:R0000 508:R0002 512:T10011 512:#0230
*/

#if defined(SSE_SHIFTER_STABILISER)

  if(!(CurrentScanline.Tricks&TRICK_STABILISER) && CyclesIn>432)
  { //TODO those constants...
    r2cycle=NextShiftModeChange(432); // can be 1 or 2
    if(r2cycle>432 && r2cycle<460) 
    {
      if(!ShiftModeChangeAtCycle(r2cycle))
        r2cycle=NextShiftModeChange(r2cycle);
      if(r2cycle>-1 && r2cycle<460 && ShiftModeChangeAtCycle(r2cycle))
      {
        r0cycle=NextShiftModeChange(r2cycle,0);//it's shifter trick, we don't use new glu 'HiRes'?
        if(r0cycle>-1 && r0cycle<464) 
          CurrentScanline.Tricks|=TRICK_STABILISER;
      }
      else if(CyclesIn>440 && ShiftModeAtCycle(440)==1)
      {
        r0cycle=NextShiftModeChange(440,0);
        if(r0cycle>-1 && r0cycle<464) 
          CurrentScanline.Tricks|=TRICK_STABILISER;
      }
    }
  }
#if defined(SSE_SHIFTER_UNSTABLE)
  if(CurrentScanline.Tricks&TRICK_STABILISER)
    Shifter.Preload=0; // "reset" empties RR (that's the goal)
#endif

#endif//#if defined(SSE_SHIFTER_STABILISER)

  /////////////////////////////////////////////////////////
  // 0-BYTE LINE 2 (next scanline) and NON-STOPPING LINE //
  /////////////////////////////////////////////////////////

#if defined(SSE_GLUE_0BYTE_LINE)
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
  if(!(NextScanline.Tricks&TRICK_0BYTE_LINE))
  {
    // test
    r2cycle=(CurrentScanline.Cycles==508)?
      ScanlineTiming[HSYNC_ON][FREQ_60]:ScanlineTiming[HSYNC_ON][FREQ_50];
    if(CyclesIn>=r2cycle && (ShiftModeAtCycle(r2cycle)&2))
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
    else if(CyclesIn>=r2cycle+40 && (ShiftModeAtCycle(r2cycle+40)&2))
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

#if defined(SSE_GLUE_VERT_OVERSCAN) 
/*  Top and bottom border.
    Our hypothesis: #scanlines is determined on a frame basis by the GLU,
    so if the timing is right, using shifter_freq_at_start_of_vbl is
    fine.
*/

  int CyclesIn=LINECYCLES;
  enum{NO_LIMIT=0,LIMIT_TOP,LIMIT_BOTTOM};
  BYTE on_overscan_limit=(scan_y==-30)?LIMIT_TOP:LIMIT_BOTTOM;

#if defined(SSE_VS2008_WARNING_390) && defined(SSE_DEBUG)
  int t=0;
#else
  int t;
#endif

#if defined(SSE_GLUE_VERT_OVERSCAN_390) // feature just couldn't work. TODO test
  if(emudetect_overscans_fixed && on_overscan_limit) 
  {
    CurrentScanline.Tricks|= (on_overscan_limit==LIMIT_TOP) 
        ? TRICK_TOP_OVERSCAN: TRICK_BOTTOM_OVERSCAN;
  }
#else
  int freq_at_trigger=0; 
  if(screen_res==2) 
    freq_at_trigger=MONO_HZ;
  if(emudetect_overscans_fixed) 
    freq_at_trigger=(on_overscan_limit) ? 60:0;
#endif
#if defined(SSE_GLUE_50HZ_OVERSCAN) 
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50)
  {
    t=ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50];
    if(CyclesIn>=t && FreqAtCycle(t)!=50)
      CurrentScanline.Tricks|= (on_overscan_limit==LIMIT_TOP) 
        ? TRICK_TOP_OVERSCAN: TRICK_BOTTOM_OVERSCAN;
  }
#endif
#if defined(SSE_GLUE_60HZ_OVERSCAN) 
/*  Removing lower border at 60hz. 
    Cases: Leavin' Teramis STF, It's a girl 2 STE last screen
*/
  else if(on_overscan_limit==LIMIT_BOTTOM && shifter_freq_at_start_of_vbl==60)
  {
    t=ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_60];
    if(CyclesIn>=t && FreqAtCycle(t)!=60) 
      CurrentScanline.Tricks|=TRICK_BOTTOM_OVERSCAN_60HZ; 
  }
#endif

  if(CurrentScanline.Tricks&
    (TRICK_TOP_OVERSCAN|TRICK_BOTTOM_OVERSCAN|TRICK_BOTTOM_OVERSCAN_60HZ))
  {
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
    overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
#if defined(SSE_INT_MFP_TIMER_B_392A)
    if(!OPTION_C2)
#endif
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
/*  At 60hz, fewer scanlines are displayed in the bottom border than 50hz
    Fixes It's a girl 2 last screen
*/
      shifter_last_draw_line=(CurrentScanline.Tricks&TRICK_BOTTOM_OVERSCAN_60HZ)
        ? 226 : 247;
  }

#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_TRACE_CONTROL)
  if(TRACE_ENABLED(LOGSECTION_VIDEO)&&(TRACE_MASK1 & TRACE_CONTROL_VERTOVSC) && CyclesIn>=t) 
  {
    FrameEvents.ReportLine();
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),CurrentScanline.Tricks);
  //  ASSERT( scan_y!=199|| (CurrentScanline.Tricks&TRICK_BOTTOM_OVERSCAN) );
    //ASSERT( scan_y!=199|| shifter_last_draw_line==247 );
  }
#endif

#endif//#if defined(SSE_GLUE_VERT_OVERSCAN) 

}


void TGlue::EndHBL() {

  ASSERT(FetchingLine());

#if defined(SSE_SHIFTER_END_OF_LINE_CORRECTION)
/*  Finish horizontal overscan : correct -2 & +2 effects
    Those tests are much like EndHBL() in Hatari (v1.6)
*/
  if((CurrentScanline.Tricks&(TRICK_LINE_PLUS_2|TRICK_LINE_PLUS_26))
    && !(CurrentScanline.Tricks&(TRICK_LINE_MINUS_2|TRICK_LINE_MINUS_106))
#if defined(SSE_GLUE_392E)
    && CurrentScanline.EndCycle==GLU_DE_OFF_60)
#elif defined(SSE_GLUE_392A)
    && CurrentScanline.EndCycle==ScanlineTiming[LINE_STOP][FREQ_60])
#else
    && CurrentScanline.EndCycle==372)    
#endif
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
#if defined(SSE_GLUE_392E)
    && (CurrentScanline.StartCycle==GLU_DE_ON_60 
    || CurrentScanline.EndCycle!=GLU_DE_OFF_60))
#elif defined(SSE_GLUE_392A)
    && (CurrentScanline.StartCycle==ScanlineTiming[LINE_START][FREQ_60]
    || CurrentScanline.EndCycle!=ScanlineTiming[LINE_STOP][FREQ_60]))
#else
    && (CurrentScanline.StartCycle==52 || CurrentScanline.EndCycle!=372))
#endif
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
    Nice collection of hacks still necessary (compare Overdrive menu/dragon!!!)
    We just try to add working cases without breaking the rest.
*/

  if(OPTION_WS)
  {
    if(MMU.WU[OPTION_WS]
      && (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      &&!(CurrentScanline.Tricks&(TRICK_STABILISER|TRICK_LINE_MINUS_2))
#if defined(SSE_SHIFTER_UNSTABLE_380)
      &&!(CurrentScanline.Tricks&TRICK_LINE_MINUS_106)
      &&!shifter_hscroll_extra_fetch //schnusdie
#endif
      &&!((PreviousScanline.Tricks&TRICK_STABILISER)
      &&ShiftModeChangeAtCycle(448-512)==2)) //? TODO!!!
    {
#if defined(SSE_SHIFTER_UNSTABLE_380)//closure blue text
      if(Shifter.Preload&&(CurrentScanline.Tricks&TRICK_UNSTABLE))
        Shifter.Preload=0; // preload only for one scanline, I think I'm just simplifying things
      else
#endif
        Shifter.Preload=1;
    }
#ifndef SSE_SHIFTER_UNSTABLE_390___ //TODO
    else if(CurrentScanline.Cycles==508 && FetchingLine()
      && FreqAtCycle(0)==60 && FreqAtCycle(464)==60)
      Shifter.Preload=0; // a full 60hz scanline should reset the shifter 
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
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
  if((FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_SHIFTER_TRICKS) 
    && CurrentScanline.Tricks)
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'T',CurrentScanline.Tricks);
  if((FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_SHIFTER_TRICKS_BYTES))
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'#',CurrentScanline.Bytes);
#if defined(DEBUG_BUILD) && defined(SSE_OSD_CONTROL) && defined(SSE_SHIFTER_UNSTABLE)
  if((OSD_MASK2 & OSD_CONTROL_PRELOAD) && Shifter.Preload)
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'P',Shifter.Preload); // P of Preload, not Palette
#endif
#endif
#if defined(SSE_BOILER_TRACE_CONTROL)
/*  It works but the value has to be validated at least once by clicking
    'Go'. If not it defaults to 0. That way we don't duplicate code.
*/
  if((TRACE_MASK1&TRACE_CONTROL_1LINE) && TRACE_ENABLED(LOGSECTION_VIDEO) 
    && scan_y==debug_run_until_val)
  {
    REPORT_LINE;
    TRACE_OSD("y %d %X %d",scan_y,CurrentScanline.Tricks,CurrentScanline.Bytes);
  }
#endif
#endif//dbg

  ASSERT(VCount);
  if(VCount)//relz build could crash...
    VCount--;

  Shifter.IncScanline();
  PreviousScanline=CurrentScanline; // auto-generated
  CurrentScanline=NextScanline;

  if(CurrentScanline.Tricks)
    ; // don't change #bytes
  else if(FetchingLine() //TODO seems sketchy!
    || scan_y==-29 && (PreviousScanline.Tricks&TRICK_TOP_OVERSCAN)
    || !scan_y && !PreviousScanline.Bytes)
    CurrentScanline.Bytes=(screen_res==2)?80:160;
  else
  {
    ASSERT( !FetchingLine() );
    NextScanline.Bytes=0;
  }
#if defined(SSE_GLUE_392B)
  MMU.ExtraBytesForHscroll=0;
#endif
  AdaptScanlineValues(-1);
  ASSERT(CurrentScanline.Cycles>=224||n_cpu_cycles_per_second>CpuNormalHz);
  TrickExecuted=0;
  NextScanline.Tricks=0; // eg for 0byte lines mess
  shifter_skip_raster_for_hscroll = (HSCROLL!=0); // one more fetch at the end
  Glue.Status.scanline_done=false;

#if defined(SSE_MMU_LINEWID_TIMING)
  LINEWID=shifter_fetch_extra_words;
#endif
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
#if defined(SSE_VC_INTRINSICS_390G)
  BITRESET(shifter_freq_change_idx,5);//stupid, I guess
#else
  shifter_freq_change_idx&=31;
#endif
#if defined(SSE_VAR_OPT_390A1)
  shifter_freq_change_time[shifter_freq_change_idx]=act;
#else
  shifter_freq_change_time[shifter_freq_change_idx]=ABSOLUTE_CPU_TIME;
#endif
  shifter_freq_change[shifter_freq_change_idx]=f;                    
}


void TGlue::AddShiftModeChange(int mode) {
  // called only by SetShiftMode
  shifter_shift_mode_change_idx++;
#if defined(SSE_VC_INTRINSICS_390G)
  BITRESET(shifter_shift_mode_change_idx,5);//stupid, I guess
#else
  shifter_shift_mode_change_idx&=31;
#endif
#if defined(SSE_VAR_OPT_390A1)
  shifter_shift_mode_change_time[shifter_shift_mode_change_idx]=act;
#else
  shifter_shift_mode_change_time[shifter_shift_mode_change_idx]=ABSOLUTE_CPU_TIME;
#endif
  shifter_shift_mode_change[shifter_shift_mode_change_idx]=mode;                    
}


int TGlue::FreqChangeAtCycle(int cycle) {
  // if there was a change at this cycle, return it, otherwise -1
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  // loop while it's later than cycle, with safety
  for(i=shifter_freq_change_idx,j=0;
  j<32 && shifter_freq_change_time[i]-t>0;
  j++,i--,i&=31);
  // here, we're on the right cycle, or before
  int rv=(j<32 && !(shifter_freq_change_time[i]-t))
    ?shifter_freq_change[i]:-1;
  return rv;
}

int TGlue::ShiftModeChangeAtCycle(int cycle) {
  // if there was a change at this cycle, return it, otherwise -1
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  // loop while it's later than cycle, with safety
  for(i=shifter_shift_mode_change_idx,j=0;
  j<32 && shifter_shift_mode_change_time[i]-t>0;
  j++,i--,i&=31);
  // here, we're on the right cycle, or before
  int rv=(j<32 && !(shifter_shift_mode_change_time[i]-t))
    ?shifter_shift_mode_change[i]:-1;
  return rv;

}

int TGlue::FreqAtCycle(int cycle) {
  ASSERT(cycle<=LINECYCLES);
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<=0 && shifter_freq_change[i]>0)
    return shifter_freq_change[i];
  return shifter_freq_at_start_of_vbl;
}


int TGlue::ShiftModeAtCycle(int cycle) {
  // what was the shift mode at this cycle?
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<=0)
    return shifter_shift_mode_change[i];
  return m_ShiftMode; // we don't have at_start_of_vbl
}

#ifdef SSE_BOILER

int TGlue::NextFreqChange(int cycle,int value) {
  // return cycle of next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    //if(value==-1 || shifter_shift_mode_change[i]==value)
    if(value==-1 || shifter_freq_change[i]==value) //v3.8.0
      idx=i;
  if(idx!=-1 && shifter_freq_change_time[idx]-t>0)
    return shifter_freq_change_time[idx]-LINECYCLE0;
  return -1;
}

#endif


int TGlue::NextShiftModeChange(int cycle,int value) {
  // return cycle of next change after this cycle
  // if value=-1, return any change
  // if none is found, return -1
  ASSERT(value>=-1 && value <=2);
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j,rv=-1;
  // we start from now, go back in time
  for(i=shifter_shift_mode_change_idx,j=0; j<32; i--,i&=31,j++)
  {
    int a=shifter_shift_mode_change_time[i]-t;
    if(a>0 && a<1024) // as long as it's valid, it's better...
    {
      if(value==-1 || shifter_shift_mode_change[i]==value)
        rv=shifter_shift_mode_change_time[i]-LINECYCLE0; // in linecycles
    }
    else
      break; // as soon as it's not valid, we're done
  }
  return rv;
}


int TGlue::NextChangeToHi(int cycle) {
  // return cycle of next change after this cycle
  // if none is found, return -1

  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j,rv=-1;
  // we start from now, go back in time
  for(i=shifter_shift_mode_change_idx,j=0; j<32; i--,i&=31,j++)
  {
    int a=shifter_shift_mode_change_time[i]-t;
    if(a>0 && a<1024) // as long as it's valid, it's better...
    {
      if(shifter_shift_mode_change[i]&2) //HI
        rv=shifter_shift_mode_change_time[i]-LINECYCLE0; // in linecycles
    }
    else
      break; // as soon as it's not valid, we're done
  }
  return rv;
}


int TGlue::NextChangeToLo(int cycle) {
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j,rv=-1;
  // we start from now, go back in time
  for(i=shifter_shift_mode_change_idx,j=0; j<32; i--,i&=31,j++)
  {
    int a=shifter_shift_mode_change_time[i]-t;
    if(a>0 && a<1024) // as long as it's valid, it's better...
    {
      if(!(shifter_shift_mode_change[i]&2)) //not HI (also MED)
        rv=shifter_shift_mode_change_time[i]-LINECYCLE0; // in linecycles
    }
    else
      break; // as soon as it's not valid, we're done
  }
  return rv;
}


int TGlue::PreviousChangeToHi(int cycle) {
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_shift_mode_change_idx,j=0
    ; idx==-1 && j<32
    ; i--,i&=31,j++)
    if(shifter_shift_mode_change_time[i]-t<0&&(shifter_shift_mode_change[i]&2))
      idx=i;
  if(idx!=-1)
    idx=shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return idx;
}

int TGlue::PreviousChangeToLo(int cycle) {
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_shift_mode_change_idx,j=0
    ; idx==-1 && j<32
    ; i--,i&=31,j++)
    if(shifter_shift_mode_change_time[i]-t<0&&!(shifter_shift_mode_change[i]&2))
      idx=i;
  if(idx!=-1)
    idx=shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return idx;
}


#if defined(SSE_BOILER_FRAME_REPORT) 
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
#endif

int TGlue::PreviousShiftModeChange(int cycle) {
  // return cycle of previous change before this cycle
  COUNTER_VAR t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}


int TGlue::CycleOfLastChangeToShiftMode(int value) {
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change[i]!=value && j<32
#if defined(SSE_VAR_OPT_390C) //we only need positive values, looking for R1
    // sensible difference in VS2015 diagnostic tools
    && (shifter_shift_mode_change_time[i] - LINECYCLE0)>0 
#endif
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change[i]==value)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}

#endif//look-up functions


void TGlue::GetNextScreenEvent() {
/*  This function is called by run's prepare_next_event() and 
    prepare_event_again(), so it's called a lot.
    It replaces the frame event tables of Steem 3.2 (event_plan[]).
    Conceptually, it is more satisfying, because Steem acts more like a real
    ST, where timings are also computed on the go by the Glue.
*/ 

  // VBI is set pending some cycles into first scanline of frame, 
  // when VSYNC stops.
  // The video counter will be reloaded again.
  if(!Status.vbi_done&&!VCount)
  {
    screen_event_vector=event_trigger_vbi;
    screen_event.time=ScanlineTiming[ENABLE_VBI][FREQ_50]; //60, 72?
    if(!Status.hbi_done)
      hbl_pending=true; 
  }

  // Video counter is reloaded 3 lines before the end of the frame
  // VBLANK is already on since a couple of scanlines. (? TODO)
  // VSYNC will start, which will trigger reloading of the Video Counter
  // by the MMU.
  else if(!Status.sdp_reload_done && VCount==3)
  {
    screen_event_vector=event_start_vbl;
    screen_event.time=ScanlineTiming[RELOAD_SDP][shifter_freq_idx];
  }

  // VSYNC is triggered when the vertical counter is at the last but one
  // line, some cycles into this line
  else if(!Status.vbl_done && VCount==1)
  {
    screen_event_vector=event_vbl_interrupt;
    screen_event.time=CurrentScanline.Cycles;
    Status.vbi_done=false;
  }  

  // default event = scanline
  else
  {
    screen_event_vector=event_scanline;
    screen_event.time=CurrentScanline.Cycles;
  }

#if defined(SSE_DEBUG)
  screen_event.event=screen_event_vector;
#endif

#if defined(SSE_TIMING_MULTIPLIER_392B)
  if(cpu_cycles_multiplier>1 && screen_event_vector!=event_scanline)
    screen_event.time*=cpu_cycles_multiplier;
#elif defined(SSE_TIMING_MULTIPLIER)
  if(cpu_cycles_multiplier>1)
    screen_event.time*=cpu_cycles_multiplier;
#else
#if defined(SSE_CPU_MFP_RATIO)
  if (n_cpu_cycles_per_second>CpuNormalHz){
    ASSERT(n_millions_cycles_per_sec>8);
#else
  if (n_cpu_cycles_per_second>8000000){
#endif
    int factor=n_millions_cycles_per_sec;
    screen_event.time*=factor;
    screen_event.time/=8;
  }
#endif//#if defined(SSE_TIMING_MULTIPLIER)
  time_of_next_event=screen_event.time+cpu_timer_at_start_of_hbl;
}


void TGlue::Reset(bool Cold) {

  m_SyncMode=0; //60hz
#if defined(SSE_TIMING_MULTIPLIER_392B)
  CurrentScanline.Cycles=scanline_time_in_cpu_cycles[shifter_freq_idx];
#else
  CurrentScanline.Cycles=scanline_time_in_cpu_cycles_8mhz[shifter_freq_idx];
  ASSERT(CurrentScanline.Cycles<=512);
#endif
  

  if(Cold) // if warm, Glue keeps on running
  {
    cpu_timer_at_start_of_hbl=0;
    VCount=0;
    Shifter.m_ShiftMode=m_ShiftMode=screen_res; 
  }
  *(BYTE*)(&Status)=0;
}


/*  SetShiftMode() and SetSyncMode() are called when a program writes
    on registers FF8260 (shift) or FF820A (sync). 
*/

void TGlue::SetShiftMode(BYTE NewRes) {
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
  For the GLU, '3' is interpreted as '2'. Case: The World is my Oyster screen #2

  Writes on ShiftMode have a 2 cycle resolution as far as the GLU is
  concerned, 4 for the Shifter. The GLU's timing matters, that's why the write
  is handled by the Glue object.
    
  In monochrome, frequency is 72hz, a line is transmitted in 28µs.
  There are 500 scanlines + vsync = 1 scanline time (more or less).

*/

  int CyclesIn=LINECYCLES;

#if defined(DEBUG_BUILD)
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SHIFTMODE)
    FrameEvents.Add(scan_y,CyclesIn,'R',NewRes); 
#endif
   log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Changed screen res to "+
     NewRes+" at scanline "+scan_y+", cycle "+CyclesIn);
   //TRACE("Set shift mode %d\n",NewRes);
#endif

  int OldRes=m_ShiftMode;

  // Only two lines would physically exist in the Shifter, not a full byte
  NewRes&=3; 
  // Update both Shifter and GLU
  // We don't emulate the possible 2 cycle delay for the Shifter 
  Shifter.m_ShiftMode=m_ShiftMode=NewRes;
  if(screen_res
#if defined(SSE_GLUE_HIRES_OVERSCAN)
    >
#else
    >=
#endif
    2|| emudetect_falcon_mode!=EMUD_FALC_MODE_OFF)
    return; // if not, bad display in high resolution

#ifndef NO_CRAZY_MONITOR
  if(extended_monitor)
  {
    screen_res=(BYTE)(NewRes & 1);
    return;
  }
#endif

#if !defined(SSE_GLUE_HIRES_OVERSCAN)
  ASSERT( COLOUR_MONITOR );
#endif

  if(NewRes==3) 
    NewRes=2; // fixes The World is my Oyster screen #2

#if defined(SSE_SHIFTER_TRICKS)
  if(NewRes!=OldRes)
    AddShiftModeChange(NewRes); // add time & mode
#if defined(SSE_GLUE_RIGHT_OFF_BY_SHIFT_MODE)
  AddFreqChange((NewRes&2) ? MONO_HZ : shifter_freq);
#endif
#endif

  Shifter.Render(CyclesIn,DISPATCHER_SET_SHIFT_MODE);

#if defined(SSE_GLUE_HIRES_OVERSCAN)
  if(screen_res==2 && !COLOUR_MONITOR)
  {
    freq_change_this_scanline=true;
    return;
  }
#endif

  int old_screen_res=screen_res;
  screen_res=(BYTE)(NewRes & 1); // only for 0 or 1 - note could weird things happen?
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
#if defined(SSE_VAR_OPT_390A1)
    if(mixed_output==3 && (act-cpu_timer_at_res_change<30))
#else
    if(mixed_output==3 && (ABSOLUTE_CPU_TIME-cpu_timer_at_res_change<30))
#endif
      mixed_output=0; //cancel!
    else if(scan_y<-30) // not displaying anything: no output to mix...
      ; // eg Pandemonium/Chaos Dister
    else if(!mixed_output)
      mixed_output=3;
    else if(mixed_output<2)
      mixed_output=2;
#if defined(SSE_VAR_OPT_390A1)
    cpu_timer_at_res_change=act;
#else
    cpu_timer_at_res_change=ABSOLUTE_CPU_TIME;
#endif
  }

  freq_change_this_scanline=true; // all switches are interesting
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
  if(OPTION_C2)
  {
    CALC_CYCLES_FROM_HBL_TO_TIMER_B((NewRes&2) ? MONO_HZ : shifter_freq);
    calc_time_of_next_timer_b();
  }
#endif
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
  if(shifter_last_draw_line==400 && !(m_ShiftMode&2) && screen_res<2)
    shifter_last_draw_line>>=1; // simplistic?
#endif

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
    Sync mode is abused to create overscan (3 of the 4 borders)
*/

  int CyclesIn=LINECYCLES;

#ifdef SSE_DEBUG
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SYNCMODE)
    FrameEvents.Add(scan_y,CyclesIn,'S',NewSync); 
#endif
#endif

  m_SyncMode=NewSync&3; // 2bits

#if defined(SSE_SHIFTER_RENDER_SYNC_CHANGES) //no!
  Shifter.Render(CyclesIn,DISPATCHER_SET_SYNC);
#elif defined(SSE_INT_MFP_TIMER_B_392D)
  if(OPTION_C2 && FetchingLine())
    CheckSideOverscan(); // force check to adapt timer B to right off...
#endif

  shifter_freq_idx=(screen_res>=2) ? 2 : ((NewSync&2)?0:1);//TODO
  ASSERT(shifter_freq_idx>=0 && shifter_freq_idx<NFREQS);
  int new_freq=Freq[shifter_freq_idx];
  ASSERT(new_freq==50||new_freq==60||new_freq==72);
  if(shifter_freq!=new_freq)
    freq_change_this_scanline=true;  
  log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Changed frequency to "+new_freq+
    " at "+scanline_cycle_log());
  shifter_freq=new_freq;
#if defined(SSE_SHIFTER_TRICKS)
  AddFreqChange(new_freq);
#endif
  AdaptScanlineValues(CyclesIn);
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
  if(OPTION_C2)
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


void TGlue::Update() {

/*  Update GLU timings according to ST type and wakeup state. We do it when
    player changes options, not at each scanline in CheckSideOverscan().
    As you can see, demo Forest by ljbk (AF) helps us a lot here.
*/

  char WU_res_modifier=MMU.ResMod[ST_TYPE==STE?3:OPTION_WS]; //-2, 0, 2
  char WU_sync_modifier=MMU.FreqMod[ST_TYPE==STE?3:OPTION_WS]; // 0 or 2

  // DE (Display Enable)
  // Before the image is really displayed,there's a delay of 28 cycles (27?):
  // 16 for Shifter prefetch, of course, but also 12 "other" 
  // (8 before prefetch, 4 after).
  // Timer B is also triggered after those 28 cycles, but that's a coincidence,
  // it's due to other delays.
  ScanlineTiming[LINE_START][FREQ_72]=GLU_DE_ON_72+WU_res_modifier; // GLUE tests MODE
  ScanlineTiming[LINE_START][FREQ_60]=GLU_DE_ON_60+WU_sync_modifier; // GLUE tests SYNC
  ScanlineTiming[LINE_START][FREQ_50]=GLU_DE_ON_50+WU_sync_modifier;
  for(int f=0;f<NFREQS;f++) // LINE STOP = LINE START + DE cycles
    ScanlineTiming[LINE_STOP][f]=ScanlineTiming[LINE_START][f]+DE_cycles[f];


#if defined(SSE_GLUE_392A)
/*  On the STE, DE is asserted at the same time as on the STF, except
    if HSCROLL<>0 (this is handled in AdaptScanlineValues()).
    But the process is started at that earlier timing regardless.
    This is why the thresholds for line +2 and line +26 are different.
*/
  ScanlineTiming[CHOOSE_FREQ][FREQ_72]=ScanlineTiming[LINE_START][FREQ_72];
  ScanlineTiming[CHOOSE_FREQ][FREQ_60]=ScanlineTiming[LINE_START][FREQ_60];
  ScanlineTiming[CHOOSE_FREQ][FREQ_50]=ScanlineTiming[LINE_START][FREQ_50];  
  if(ST_TYPE==STE)
  {
    ScanlineTiming[CHOOSE_FREQ][FREQ_72]-=4;
    ScanlineTiming[CHOOSE_FREQ][FREQ_60]-=16;
    ScanlineTiming[CHOOSE_FREQ][FREQ_50]-=16;
  }  
#else
  // On the STE, DE test occurs sooner due to hardscroll possibility
  // but prefetch starts sooner only if HSCROLL <> 0.
  if(ST_TYPE==STE) // adapt for HSCROLL prefetch after DE_OFF has been computed
  {
    ScanlineTiming[LINE_START][FREQ_72]-=4;
    ScanlineTiming[LINE_START][FREQ_60]-=16;
    ScanlineTiming[LINE_START][FREQ_50]-=16;
  }
#endif

  // HBLANK
  // Cases: Overscan demos, Forest
  // There's a -4 difference for 60hz but timings are the same on STE
  // There's no HBLANK in high res
  ScanlineTiming[HBLANK_OFF][FREQ_50]=GLU_HBLANK_OFF_50+WU_sync_modifier;
  ScanlineTiming[HBLANK_OFF][FREQ_60]=ScanlineTiming[HBLANK_OFF][FREQ_50]-4;

  // HSYNC
  // Cases: Enchanted Land, HighResMode, Hackabonds, Forest
  // There's both a -2 difference for STE and a -4 difference for 60hz
  // The STE -2 difference isn't enough to change effects of a "right off" 
  // trick (really?)
  ScanlineTiming[HSYNC_ON][FREQ_50]=GLU_HSYNC_ON_50+WU_res_modifier;
  if(ST_TYPE==STE)
    ScanlineTiming[HSYNC_ON][FREQ_50]-=2; 
  ScanlineTiming[HSYNC_ON][FREQ_60]=ScanlineTiming[HSYNC_ON][FREQ_50]-4;
  ScanlineTiming[HSYNC_OFF][FREQ_50]=ScanlineTiming[HSYNC_ON][FREQ_50]+GLU_HSYNC_DURATION;
  ScanlineTiming[HSYNC_OFF][FREQ_60]=ScanlineTiming[HSYNC_ON][FREQ_60]+GLU_HSYNC_DURATION;  
#ifdef SSE_BETA
 ScanlineTiming[HSYNC_ON][FREQ_72]=194+WU_res_modifier;//not used
 ScanlineTiming[HSYNC_OFF][FREQ_72]=ScanlineTiming[HSYNC_ON][FREQ_72]+24;
#endif

  // Reload video counter
  // Cases: DSOTS, Forest
  // value isn't precise and sync/STE modifier aren't known  TODO
  // 64+2 would break DSOTS STF2 though
  ScanlineTiming[RELOAD_SDP][FREQ_50]= GLU_RELOAD_VIDEO_COUNTER_50+WU_sync_modifier;
//  if(ST_TYPE==STE)
  //  ScanlineTiming[RELOAD_SDP][FREQ_50]-=2;
  ScanlineTiming[RELOAD_SDP][FREQ_60]=ScanlineTiming[RELOAD_SDP][FREQ_50];//?
  ScanlineTiming[RELOAD_SDP][FREQ_72]=ScanlineTiming[RELOAD_SDP][FREQ_50];//?

  // Enable VBI
  // Cases: Forest, 3615GEN4-CKM, Dragonnels/Happy Islands, Auto 168, TCB...
  // but: TEST16.TOS
  ScanlineTiming[ENABLE_VBI][FREQ_50]=GLU_TRIGGER_VBI_50;
  if(ST_TYPE==STE) 
    ScanlineTiming[ENABLE_VBI][FREQ_50]+=4;

/*  A strange aspect of the GLU is that it decides around cycle 54 how
    many cycles the scanline will be, like it had an internal variable for
    this, or various paths.
    What's more, the cycle where it's decided is about the same on the STE
    (2 cycles later actually), despite the fact that GLU DE starts earlier.

    This aspect has been clarified by ijor's GLUE pseudo-code
    http://www.atari-forum.com/viewtopic.php?f=16&t=29578&p=293522#p293492

// Horizontal sync process
HSYNC_COUNTER = 0
DO                                                               ////   FREQ
   IF HS_TICK                                                    ////  0    1    2
      IF HSYNC_COUNTER == 101 && REZ[1] == 0 THEN HSYNC = TRUE   //// 460  464
      IF HSYNC_COUNTER == 111 && REZ[1] == 0 THEN HSYNC = FALSE  //// 500  504
      IF HSYNC_COUNTER == 121 && REZ[1] == 1 THEN HSYNC = TRUE   ////           188?
      IF HSYNC_COUNTER == 127 && REZ[1] == 1 THEN HSYNC = FALSE  ////           212?

      IF HSYNC_COUNTER == 127                                        
         IF REZ[1] == 1 THEN          HSYNC_COUNTER = 72 //// (127+1-72)*4=224 cycles
         ELSEIF FREQ[1] == 0 THEN     HSYNC_COUNTER = 1  //// (127+1-1)*4=508 cycles
         ELSE                         HSYNC_COUNTER = 0  //// (127+1-0)*4=512 cycles
      ELSE
         HSYNC_COUNTER++
      ENDIF
   ENDIF
LOOP
*/
  // Cases: Forest, Closure
  cycle_of_scanline_length_decision=GLU_DECIDE_NCYCLES+WU_sync_modifier;
  if(ST_TYPE==STE)
    cycle_of_scanline_length_decision+=2;

  // Top and bottom border, little trick here, we assume STF is in WU2
  ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50]=GLU_VERTICAL_OVERSCAN_50; //504

  if(ST_TYPE==STE)
    ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50]-=4; //502 //500
#if defined(SSE_MMU_WU)
  else if(MMU.WU[OPTION_WS]==1)
    ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50]-=2; //504 //502
#endif

  ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_60]
    =ScanlineTiming[VERT_OVSCN_LIMIT][FREQ_50];//?
}


void TGlue::Vbl() {
  cpu_timer_at_start_of_hbl=time_of_next_event;
  scan_y=-scanlines_above_screen[shifter_freq_idx];
  Status.hbi_done=Status.sdp_reload_done=false;
  Status.vbl_done=true;
#if defined(SSE_SHIFTER_UNSTABLE)//TODO...
  if(time_of_next_event-cpu_time_of_last_vbl==133604) // according to ljbk ?
    Shifter.Preload=0; 
#endif
#if defined(SSE_BOILER_FAKE_IO) && defined(SSE_OSD_CONTROL)
  if(OSD_MASK2&OSD_CONTROL_MODES)
    TRACE_OSD("R%d S%d P%d",Shifter.m_ShiftMode,m_SyncMode,Shifter.Preload);
#endif
#if defined(SSE_SHIFTER_TRICKS)
  AddFreqChange(shifter_freq); //? doubtful use...
  AddShiftModeChange(m_ShiftMode); //?
#endif
}

#endif//SSE_GLUE
