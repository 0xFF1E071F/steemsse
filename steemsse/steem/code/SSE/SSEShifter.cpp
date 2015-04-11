#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_STRUCTURE_SSESHIFTER_OBJ)
#include "../pch.h"
#include <conditions.h>
#include <display.decla.h>
#include <mfp.decla.h>
#include <palette.decla.h>
#include <run.decla.h>
#include "SSEFrameReport.h"
#include "SSEShifter.h"
#include "SSEParameters.h"
#elif defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: SSEShifter.cpp")
#endif//SSE_STRUCTURE_SSESHIFTER_OBJ

#if defined(SSE_SHIFTER)

#if defined(SSE_MMU)
#include "SSEMMU.h"
#endif
#if defined(SSE_STF)
#include "SSESTF.h"
#endif

#include "SSEGlue.h"


#if defined(SSE_DEBUG_FRAME_REPORT) && !defined(SSE_STRUCTURE_SSEFRAMEREPORT_OBJ)
#include "SSEFrameReport.cpp"
#endif

TShifter Shifter; // singleton


/////////////////////
// object TShifter //
/////////////////////

#define LOGSECTION LOGSECTION_VIDEO

TShifter::TShifter() {
#if defined(WIN32)
  ScanlineBuffer=NULL;
#endif
  CurrentScanline.Cycles=500;
}


TShifter::~TShifter() {
}

/*  Check overscan functions:
    -  CheckSideOverscan()
    -  EndHBL() 
    -  CheckVerticalOverscan()
    Those are based on existing Steem code, but much 
    inflated, for a good cause: now Steem handles a lot more Shifter tricks
    than before.
    Much has been inspired by Hatari, but was also discussed on Atari-forum.
    In some cases we go beyond Hatari (at least v1.6.1)
    Also like in Hatari comments briefly explain Shifter tricks.
*/


void TShifter::CheckSideOverscan() {
/*  An overbloated version of Steem's draw_check_border_removal()
    This function is called before rendering if there have been Shifter events.
    It takes care of all "side overscan" business.
    Some features have been added on the v3.2 basis:
    STE-only left border off (line +20), 0-byte line, 4bit hardscroll, etc.
    Other Shifter tricks have been refined, and difference between ST models is
    handled.
    The "old" Steem look-up system is still used, instead of the "Hatari" way
    of acting at the Shifter events themselves. 
    The advantages are that we integrate in what exists, that everything 
    is nicely concentrated in one place and that we don't need to take care 
    of the -1 problem when events occur at 'LineCycles+n' (eg 520).
    Disadvantage could be performance, so it's not definitive.
*/
#if defined(SSE_DEBUG) && defined(SSE_SHIFTER_DRAW_DBG)
  draw_check_border_removal(); // base function in Steem 3.2
  return;
#endif
  int act=ABSOLUTE_CPU_TIME,t,i;
  WORD CyclesIn=LINECYCLES;

#if defined(SSE_SHIFTER_HIRES_OVERSCAN)//3.7.0
/*  Yes! You may abuse the GLUE when you have a monochrome screen too.
    It's the same idea: changing shift mode when it's about to make 
    decisions.
    R0 parts in black not emulated, and visibly other things, very trashy
    effect for the moment.

    Eg: Monoscreen by Dead Braincells
    004:R0000 012:R0002                       -> 0byte ?
    016:R0000 024:R0002 156:R0000 184:R0002   -> right off + ... ? 
*/

  if(!FetchingLine() || screen_res>2 || !act)
    return;
  else if(screen_res==2) 
  {
    if(!freq_change_this_scanline)
      return;
    int bonus_bytes=0;
    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
      && ShiftModeAtCycle(6)!=2)
    {
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
      bonus_bytes=-80;
      draw_line_off=true;
    }
    else if( !(CurrentScanline.Tricks&2) 
      && ShiftModeAtCycle(164)!=2)
    {
      CurrentScanline.Tricks|=2;
      bonus_bytes=14;
    }
    //else REPORT_LINE;//and there's nothing else in "Monoscreen"
    shifter_draw_pointer+=bonus_bytes;
    CurrentScanline.Bytes+=bonus_bytes;
    return;
  }
#else
  if(screen_res>=2
    || !FetchingLine() 
    || !act) // if ST is resetting
    return;
#endif

#if defined(SSE_SHIFTER_TRICKS) 
    short r0cycle,r1cycle,r2cycle;
#if SSE_VERSION<=353
    short s0cycle,s2cycle; 
#endif
#endif

#if defined(SSE_SHIFTER_STATE_MACHINE)
#if defined(SSE_MMU_WU_DL)
    char WU_res_modifier=MMU.ResMod[WAKE_UP_STATE]; //-2, 0, 2
    char WU_sync_modifier=MMU.FreqMod[WAKE_UP_STATE]; // 0 or 2
#endif
    WORD DEcycle60=36; // DE STE 60hz; used for 0byte (not +2)
#if defined(SSE_STF_MMU_PREFETCH)
    if(ST_TYPE!=STE)
      DEcycle60+=16+WU_sync_modifier; // STF, 60hz: 52
#if !defined(SSE_SHIFTER_STE_DE_MED_RES) 
// DE timing is in fact the same for low and medium resolution 
    else //STE
    {
      if(ShiftModeAtCycle(DEcycle60)==1)
        DEcycle60+=8;
    }
#endif
#endif
#else//!SSE_SHIFTER_STATE_MACHINE
    char WU_res_modifier=0;
    char WU_sync_modifier=0;
#endif


  /////////////////////
  // LEFT BORDER OFF //
  /////////////////////

/*  Removing the border can be used with two goals: use a larger display area
    (fullscreen demos are more impressive than borders-on demos), and
    scroll the screen by using "sync lines" (eg Enchanted Land).
    To do it, the program sets the shift mode to 2 so that the GLUE thinks
    that it's a high resolution line, and starts DE sooner.
*/

  if(left_border
#ifndef SSE_SHIFTER_LEFT_OFF_TEST_BEFORE_HBL // fixes Riverside low border
    && CyclesIn<512
#endif
    )
  {

  //////////////////////////////////////////
  //  STE ONLY LEFT BORDER OFF (line +20) //
  //////////////////////////////////////////

/*  Only on the STE, it is possible to create stable long overscan lines (no 
    left, no right border) without a "Shifter reset" switch (R2/R0  at 
    444/456).
    The shift mode switches for left border removal produce a 20 bytes bonus
    instead of 26, and the total overscan line is 224 bytes instead of 230. 
    224/8=28,no rest->no Shifter confusion.

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

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_LINE_PLUS_20)

#if defined(SSE_STF)
    if(ST_TYPE==STE) 
#endif
    {
      ASSERT( !(CurrentScanline.Tricks&TRICK_LINE_PLUS_20) );
      ASSERT( !(CurrentScanline.Tricks&TRICK_LINE_PLUS_24) );
      ASSERT( !(CurrentScanline.Tricks&TRICK_LINE_PLUS_26) );

#if defined(SSE_SHIFTER_STATE_MACHINE) && defined(SSE_SHIFTER_LINE_PLUS_20B)
/*  The timing of R0 is important, not of R2
*/
      if(!ShiftModeChangeAtCycle(4) && (ShiftModeAtCycle(2)&2))
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_20;
#else
      if(!ShiftModeChangeAtCycle(4) && (ShiftModeChangeAtCycle(-8)==2
        || ShiftModeChangeAtCycle(-4)==2))
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_20;
#endif
    }
#endif

  ////////////////////////////////////////
  //  LEFT BORDER OFF (line +26, +24)   //
  ////////////////////////////////////////

/*
    A switch to shiftmode 2 and back at the start of the line will trigger
    early display and remove the left border.

    The limits are (tables by Paolo)
    STF R2 506 [WU1 504 WU3,4 506 WU2 508] - 6 [WU1 4 WU3,4 6 WU2 8] 
        R0 8 [WU1 6 WU3,4 8 WU2 10] - 32 [WU1 30 WU3,4 32 WU2 34]
    STE R2 2 
        R0 26

    In highres, DE starts at linecycle 6 (+-WU), not 0 like it is generally
    assumed.
    This is why a shift mode switch in the first cycles of the scanline will
    work.
    Bonus bytes are 26 because DE is on from 6 to 58 (start of 50hz line), that
    is 52 cycles. 52/2 = 26.

    On a STE, the line "starts" earlier because of HSCROLL.
    In HIRES, it starts 4 cycles earlier, that's why cycle 2 instead of 6 is
    the limit for R2 on a STE.

    Programs not working on a STE because they hit at cycle 4: 
    3615Gen4 by Cakeman
    Cuddly Demos (not patched)
    Musical Wonder 1991
    Imagination/Roller Coaster 
    Omega 
    Overscan Demos original
    SNYD/TCB (F4)
    SNYD2/Sync fullscreen (F5)
    SoWatt/Menu+Sync

    Hackabonds Demo hits at 504, left off desired, WS1 only?

*/

#if defined(SSE_SHIFTER_TRICKS) 

    int lim_r2=2,lim_r0=26;   
 
#if defined(SSE_STF_LEFT_OFF) 
    if(ST_TYPE!=STE)
      lim_r2+=4,lim_r0+=6;
#if defined(SSE_MMU_WU_SHIFTER_TRICKS)
#if defined(SSE_MMU_WU_DL)
    lim_r2+=WU_res_modifier; 
    lim_r0+=WU_res_modifier;
#else
    if(MMU.WakeUpState1())
      lim_r2+=-2,lim_r0+=-2;
    if(MMU.WakeUpState2())
      lim_r2+=WU2_PLUS_CYCLES,lim_r0+=WU2_PLUS_CYCLES;
#endif
#endif
#endif//leftoff

#endif//tricks

    if(!(CurrentScanline.Tricks&TRICK_LINE_PLUS_20))
    {
      ASSERT( !(CurrentScanline.Tricks&TRICK_LINE_PLUS_24) );
      ASSERT( !(CurrentScanline.Tricks&TRICK_LINE_PLUS_26) );
#if defined(SSE_SHIFTER_STEEM_ORIGINAL) 
      t=cpu_timer_at_start_of_hbl+2+(ST_TYPE!=STE ? 4 : 0);
      if(act-t>0)
      {
        i=CheckFreq(t);
        if(i>=0 && shifter_freq_change[i]==MONO_HZ)
          CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
      }
#elif defined(SSE_SHIFTER_TRICKS)

//#define TESTLINE -28 // for debug

      r2cycle=NextShiftModeChange(lim_r2-14,2);

#ifdef TESTLINE
      if(scan_y==TESTLINE) TRACE_LOG("R2 %d ",r2cycle);
#endif
      if(r2cycle!=-1 && r2cycle>=lim_r2-12 && r2cycle<=lim_r2) //TODO those limits?
      {
/*  ShiftMode at r0cycle can be 0 (generally) or 1 (ST-CNX, HighResMode)
    So we look for any change. 
    In some rare cases two R2 are found, eg Ultimate GFA Demo boot screen:
    004:R0002 012:R0000 376:S0000 388:S0002 436:R0002 448:R0000 508:R0002
    If it happens, we search again.
*/
        r0cycle=NextShiftModeChange(r2cycle); 
        if(ShiftModeChangeAtCycle(r0cycle)==2)
          r0cycle=NextShiftModeChange(r0cycle); 
#ifdef TESTLINE
        if(scan_y==TESTLINE) TRACE_LOG("lims R2 %d-%d R0 %d-%d R2 %d R0 %d",lim_r2-12,lim_r2,lim_r0-24,lim_r0,r2cycle,r0cycle);
#endif        
        if(r0cycle>=lim_r0-24 && r0cycle<=lim_r0)
        {

#if defined(SSE_SHIFTER_LEFT_OFF_60HZ)
/*  In a 60hz line, there are only 24 bonus bytes (6 to 54 = 48, 48/2=24).
    eg HighResMode, a 60hz screen of 184byte lines.
    It's the same as doing +26-2, but conceptually cleaner.
*/
          if(FreqAtCycle(r2cycle)==60 && FreqAtCycle(r0cycle)==72)
            CurrentScanline.Tricks|=TRICK_LINE_PLUS_24;
          else
#endif
            CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
        }
#ifdef TESTLINE
        if(scan_y==TESTLINE) TRACE_LOG(" left off %X\n", CurrentScanline.Tricks);
        if(scan_y==TESTLINE) TRACE_LOG(" freq %d %d %d %d\n",r2cycle,FreqAtCycle(r2cycle),r0cycle,FreqAtCycle(r0cycle));
        if(scan_y==TESTLINE) REPORT_LINE;
#endif
      }
#if defined(SSE_DEBUG) && defined(SSE_VID_LEFT_OFF_COMPARE_STEEM_32)
      // debug compare Steem's original test with ours, report differences
      t=cpu_timer_at_start_of_hbl+2+(ST_TYPE!=STE ? 4 : 0);
      if(act-t>0)
      {
        i=CheckFreq(t);
        if(i>=0 && shifter_freq_change[i]==MONO_HZ
          && ! CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
          TRACE_LOG("F%d y%d left off detected by Steem 3.2 R2 %d R0 %d\n",FRAME,scan_y,r2cycle,r0cycle);
        else if((i<0||shifter_freq_change[i]!=MONO_HZ)
          && CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
          TRACE_LOG("F%d y%d left off not detected by Steem 3.2 R2 %d R0 %d\n",FRAME,scan_y,r2cycle,r0cycle);
      }
#endif
#undef TESTLINE
#endif
    }
    
    // action for line+26, line+24 & line+20
    if( (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
#if defined(SSE_SHIFTER_LEFT_OFF_60HZ)
      || (CurrentScanline.Tricks&TRICK_LINE_PLUS_24)
#endif
      || (CurrentScanline.Tricks&TRICK_LINE_PLUS_20))
    {
      //ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );
#if defined(SSE_SHIFTER_LINE_PLUS_20)
      if(CurrentScanline.Tricks&TRICK_LINE_PLUS_20)
      {
        CurrentScanline.Bytes+=20;
#if defined(SSE_VID_BORDERS)
        if(SideBorderSize==ORIGINAL_BORDER_SIDE)
#endif
          overscan_add_extra+=4; // 16+4=20
#if defined(SSE_VID_BORDERS)
        else          
          overscan_add_extra=-4; // 24+4=24; 24-4=20
#endif
        overscan=OVERSCAN_MAX_COUNTDOWN;
#if defined(SSE_SHIFTER_STE_HSCROLL_LEFT_OFF)  
        if(shifter_skip_raster_for_hscroll) 
          overscan_add_extra+=6; // fixes MOLZ/Spiral
#endif
        TrickExecuted|=TRICK_LINE_PLUS_20;
#if defined(SSE_SHIFTER_LINE_PLUS_20_SHIFT)
        HblPixelShift=-8; // fixes Riverside shift

#if defined(SSE_VID_BORDERS_416_NO_SHIFT0)
/*  bugfix v3.6.0 see below for line +26, it's the same problem
    This is necessary because normal scanlines are pushed 4 pixels to the right
    in this mode (eg Riverside).
    There's trash in the border, that should be black (TODO).
*/
        if(SSE_HACKS_ON && SideBorderSize==VERY_LARGE_BORDER_SIDE
#if defined(SSE_VID_BORDERS_416_NO_SHIFT1)
/*  v3.6.1, not really a bugfix, but a convenience, so that player
    doesn't need to adjust 'Display size' if borders are not always
    displayed.
    'border' is Steem's variable, 0 = no border now
*/
          &&border
#endif
        )
          HblPixelShift+=4;
#endif
#if defined(SSE_VID_BORDERS)//?
        if(SideBorderSize!=ORIGINAL_BORDER_SIDE)
          ShiftSDP(-8);
#endif
#endif
      }
      else // normal left off
#endif
      {

/*  
    A 'left off' grants 26 more bytes but only 24 are visible on
    the screen. 
    Cases where we see that this shift is necessary:
    - Oh Crikey What a Scorcher Hidden Screen ULM: obvious shift
    - Overdrive/Dragon
    - Overscan Demos #6 (strangely)

    Cases with no shift:
    Death of the Left Border
027 - 016:R0000 376:S0000 388:S0002 512:R0002 512:T10011 512:#0230
    It seems to be because shift to R0 happens at cycle 16. Protected by
    'Hacks' for a while.
    Wrong: breaks Forest (funny way) -> SSE_SHIFTER_DOLB_SHIFT2 undef
    Not in 60hz frames? Not sure of this condition
*/


        if(
#if defined(SSE_SHIFTER_DOLB_SHIFT2)
          (r0cycle<16 || !SSE_HACKS_ON)
#else
          (shifter_freq_at_start_of_vbl==50) 
#endif
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
/*  Current theory (v3.5.4):
    This "shift" by 4 pixels is a misconception.
    A "left off" trick grants 26 extra bytes, that is 52 pixels, and not
    48. If you believe the #pixels is 48, then you think you must lose
    4 pixels before the border. If you don't, there's nothing to shift.
*/
          && (!SSE_HACKS_ON||SideBorderSize!=VERY_LARGE_BORDER_SIDE
#if defined(SSE_VID_BORDERS_416_NO_SHIFT1)
          ||!border
#endif
          )
#endif
          )
          shifter_pixel+=4;

#if defined(SSE_SHIFTER_LEFT_OFF_60HZ)
        if((CurrentScanline.Tricks&TRICK_LINE_PLUS_24))
          CurrentScanline.Bytes+=24; // 8 + 16 = 24
        else
#endif
        {
          CurrentScanline.Bytes+=26;
          overscan_add_extra+=2;  // 8 + 2 + 16 = 26
        }


#if defined(SSE_SHIFTER_UNSTABLE___)
/*  Left off eats one preloaded RR, see ST-CNX doc
    We remove 2 in WU2 for Omega, this is certainly not correct TODO
    v3.6.0, see below and EndHBL()
*/
        if(Preload>0)
        {
          Preload--;
#if defined(SSE_MMU_WU_DL)
          if(MMU.WU[WAKE_UP_STATE]==2)
#else
          if(MMU.WakeUpState2())
#endif
            Preload--;
        }
#endif


        if(HSCROLL>=12) // STE Shifter bug (Steem authors)
#if defined(SSE_VID_BORDERS_416_NO_SHIFT) //E605 & Tekila artefacts
          if(!SSE_HACKS_ON||SideBorderSize!=VERY_LARGE_BORDER_SIDE
#if defined(SSE_VID_BORDERS_416_NO_SHIFT1)
          ||!border
#endif
          ) 
#endif
          ShiftSDP(8);

#if defined(SSE_VID_BORDERS)
        if(SideBorderSize==ORIGINAL_BORDER_SIDE) // 32
#endif
          shifter_draw_pointer+=8; // 8+16+2=26
//        TRACE_LOG("+26 y %d px %d\n",scan_y,shifter_pixel);
        TrickExecuted|=TRICK_LINE_PLUS_26;

        // additional hacks for left off
        //////////////////////////////////

#if defined(SSE_SHIFTER_UNSTABLE_LEFT_OFF) && defined(SSE_MMU_WAKE_UP) \
        && defined(SSE_STF_LEFT_OFF)
/*  Hack for Death of the Left Border, Omega Full Overscan (and?)
    no stabiliser, late switch, unstable overscan: shift +4
    'Hack' option not necessary, choosing wake up state 2 is enough!
    But that's a hack, not precise emulation.
    3.5.2: not defined
*/
        if(r0cycle>=16 && ST_TYPE==STF && WAKE_UP_STATE==2
          && !(PreviousScanline.Tricks&TRICK_STABILISER))
        {
            ShiftSDP(4);
            shifter_pixel+=-4;
        }
#endif

/*
Modest hacks to have DOLB, Omega still working without breaking:
Dragon, newly added Ventura/Naos and of course Beeshift
DOLB:
007 - 000:A0007 000:a5204 016:R0000 376:S0000 388:S0002 500:P1000 512:R0002 512:T10011 512:#0230
Omega:
-22 - 000:A0007 000:a0604 008:R00FE 020:R0000 376:S0000 384:S00FE 512:P0707 512:T10011 512:#0230
TODO!
*/

#if defined(SSE_SHIFTER_UNSTABLE_DOLB) //&& defined(SSE_STF)
        if(Preload && SSE_HACKS_ON && r0cycle==16 && !r2cycle) 
          ShiftSDP(-2);
#endif

#if defined(SSE_SHIFTER_BIG_WOBBLE) && defined(SSE_SHIFTER_SDP_WRITE)
        if(SSE_HACKS_ON && (PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE)
          && HSCROLL  // so it's STE-only
#if defined(SSE_SHIFTER_ARMADA_IS_DEAD)
          && r0cycle==12 // just a hack of course
#endif
          ) 
          shifter_draw_pointer+=-4; // fixes Big Wobble, see SSEShifter.h
#endif

#if defined(SSE_SHIFTER_XMAS2004)
/*  Those are just hacks, as ususal for those cases, but they correct
    the last screen of XMAS 2004. TODO: a nice theory...
*/
        if(SSE_HACKS_ON && (PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE)
          && r0cycle==16 && !HSCROLL && LINEWID)
          shifter_draw_pointer+=4;

        if(SSE_HACKS_ON && (PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE)
          && r0cycle==16 && HSCROLL && LINEWID)
          shifter_draw_pointer+=-2;
#endif

#if defined(SSE_SHIFTER_SCHNUSDIE) && defined(SSE_SHIFTER_TRICKS)
        if(SSE_HACKS_ON && !(PreviousScanline.Tricks&TRICK_STABILISER)
          && r2cycle==-4 && r0cycle==20 // 508/20: very long in mode 2
          && shifter_hscroll_extra_fetch && !HSCROLL) // further "targeting"
          overscan_add_extra+=2; // fixes Reality is a Lie/Schnusdie
#endif
      }
      left_border=0;
#if defined(SSE_SHIFTER_TRICKS)
      CurrentScanline.StartCycle=lim_r2;//0; //not really used
#endif
      overscan=OVERSCAN_MAX_COUNTDOWN;
    }
  }


/*
TODO? 14 bytes line
STF2:
025 - 508:R0002
026 - 044:R0000
*/


  /////////////////
  // 0-BYTE LINE //
  /////////////////

/*  Various shift mode or sync mode switches trick the GLUE and MMU in 
    not fetching a scanline while the monitor is still displaying a line.
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

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_0BYTE_LINE)

#if defined(SSE_SHIFTER_STATE_MACHINE)

/*  
    v3.5.4
    We use the values in LJBK's table, taking care not to break
    emulation of other cases..
    Here we test for tricks at the start of the scanline, affecting
    current scanline.

    shift mode (tricking the BLANK signal):
    0byte line   8-32    Nostalgia/Lemmings 28/44 for STF, 32/48 for STE
          
    sync (tricking the DE signal):
    Forest: STF(1) (56/64), STF(2) (58/68), and STE (40/52).
    loSTE: STF(1) (56/68) STF(2) (58/74) STE (40/52)

*/
    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE))
    {

      r2cycle=28; // we start with 60hz lines (no case though)


  //   TRACE("F%d y%d in %d f%d %d f%d %d f%d %d f%d %d f%d %d\n",
    //   FRAME,scan_y,CyclesIn,DEcycle60-2,FreqAtCycle(DEcycle60-2),DEcycle60,FreqAtCycle(DEcycle60),DEcycle60+2,FreqAtCycle(DEcycle60+2),DEcycle60+4,FreqAtCycle(DEcycle60+4),DEcycle60+6,FreqAtCycle(DEcycle60+6));

      if(FreqAtCycle(r2cycle+2)==50)
        r2cycle+=4;

      if(CyclesIn>r2cycle && ShiftModeAtCycle(r2cycle+2+WU_res_modifier)&2)
      {
        //REPORT_LINE;
        TRACE_LOG("detect Lemmings type 0byte y %d\n",scan_y);
        CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
      }
      else if(CyclesIn>=DEcycle60+4 && left_border &&
        FreqAtCycle(DEcycle60)==50 &&  FreqChangeAtCycle(DEcycle60+4)==60)
        // funny, if FreqAtCycle(DEcycle60+2) Enchanted Land broken
      {
        //REPORT_LINE;
        TRACE_LOG("detect Forest type 0byte y %d\n",scan_y);
        CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
      }
    }

#else//state-machine

#if defined(SSE_SHIFTER_0BYTE_LINE_RES_END)//old

/*  A shift mode switch a the end of the line causes the MMU not to fetch
    video RAM of next scanline and not to count the line in its internal 
    register.
    Game No Buddies Land (500/508), demo D4/NGC (mistake? 496/508).
    Explanation: 
    Because shift mode = 2, HSYNC isn't reset at cycle 500 (50hz).
    The counter is also incremented at this time.
*/
    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
      && !ShiftModeChangeAtCycle(-4) // 508
      && (ShiftModeChangeAtCycle(-16)==2 || ShiftModeChangeAtCycle(-12)==2))
    {
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
      shifter_last_draw_line++; // fixes lower overscan in D4/NGC
    }
#endif    

#if defined(SSE_SHIFTER_0BYTE_LINE_RES_HBL) && defined(SSE_STF)//old

/*  Beyond/Pax Plax Parallax uses a shift mode switch around the HSync position
    (as in Enchanted Land's hardware test), but DE is off (right border on).
    This causes a 0-byte line (from Hatari).
    It seems to do that only in STF mode; in STE mode the program writes the 
    video RAM address to scroll the screen.
[    Maybe the same switch +4 would work on a STE, like for Lemmings?]
    TODO, too specific, we could just have
    ShiftModeAtCycle(464)==2 -> 0byte line  (to test with later version)
*/
    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE) && ST_TYPE!=STE 
      && ShiftModeChangeAtCycle(464-4)==2 && !ShiftModeChangeAtCycle(464+8))
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
#endif

#if defined(SSE_SHIFTER_0BYTE_LINE_RES_START)//old

/*  A shift mode switch after left border removal prevents the MMU from
    fetching the line. The switches must be placed on different cycles 
    according to the ST model.
    Nostalgia/Lemmings 28/44 for STF 32/48 for STE

    Explanation:
    HBlank isn't reset, DE doesn't trigger.

   TODO
32     IF(RES == LO) BLANK = FALSE
     
    ShiftModeAtCycle(32)==2 -> 0byte line  (to test with later version)

*/

  if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE))
  {
    r2cycle=
#if defined(SSE_STF)
      (ST_TYPE!=STE)?28: 
#endif
    32;
    r0cycle=r2cycle+16; // we take the precise value for performance
    if(ShiftModeChangeAtCycle(r2cycle)==2 && !ShiftModeChangeAtCycle(r0cycle))
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
  }
#endif
  
#if defined(SSE_SHIFTER_0BYTE_LINE_SYNC) //old

/*  0-byte sync switches are model-dependent and even "wake up state"-
    dependent. They  are used only in a 2006 demo called SHFORSTV.TOS,
    by the dude who also made the Overscan Demos.
    Also: beeshift; beescroll; loSTE screens
    Switches (60/50):
    Forest: STF(1) (56/64), STF(2) (58/68), and STE (40/52).
    loSTE: STF(1) (56/68) STF(2) (58/74) STE (40/52)
*/

  if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE))
  {

#if defined(SSE_STF) // determine which switches we're looking for
    if(ST_TYPE!=STE)
    {
#if defined(SSE_MMU_WU_0_BYTE_LINE)
      if(WAKE_UP_STATE==2) // OK, Forest says it's 2
        s0cycle=58,s2cycle=68+WU2_PLUS_CYCLES//2   // but strange according to compile options
#if defined(SSE_SHIFTER_FIX_LINE508_CONFUSION)
          -2
#endif
        ;
      else
#endif
        s0cycle=56,s2cycle=64; // default STF1 (OK for 'Forest')
    }
    else
#endif // STE:
      s0cycle=40,s2cycle=52; // this also produces a 512 cycles +2 line?

    if(FreqChangeAtCycle(s0cycle)==60 
      && 
      (
      FreqChangeAtCycle(s2cycle)==50
#if defined(SSE_SHIFTER_0BYTE_LINE_SYNC2)
      ||FreqChangeAtCycle(s2cycle+4)==50 // loSTE screens
#endif
      )
      )
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
  }
#endif
#endif//!state-machine

  // 0-byte line: action
  if(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
  {
    if(!(TrickExecuted&TRICK_0BYTE_LINE))
    { 
      TRACE_LOG("F%d apply 0byte y %d\n",FRAME,scan_y);
    //  TRACE_OSD("0 byte");
      draw_line_off=true; // Steem's original flag
      memset(PCpal,0,sizeof(long)*16); // all colours black
      shifter_draw_pointer-=160; // hack: Steem will draw black and update SDP
      CurrentScanline.Bytes=0;
      TrickExecuted|=TRICK_0BYTE_LINE;
    //  TRACE_OSD("0byte");
    }
//    return; // we must still check for stabiliser (Beescroll)
  }

#endif//0byte

#if defined(SSE_SHIFTER_DRAGON1)//temp (undef in 3.5.X)
    if(SS_signal==SS_SIGNAL_SHIFTER_CONFUSED_2
      && !(CurrentScanline.Tricks&TRICK_CONFUSED_SHIFTER))
    {
      CurrentScanline.Tricks|=TRICK_CONFUSED_SHIFTER;
      ShiftSDP(-2);
      scanline_drawn_so_far=4;//not perfect...
    }
#endif


  ////////////////
  // BLACK LINE //
  ////////////////

/*  
    A sync switch at cycle 28 keeps HBLANK asserted for this line.
    Video memory is fetched, but black pixels are displayed.
    This is handy to hide ugly effects of tricks in "sync lines".

    HBLANK OFF 60hz 28 50hz 32

    Paolo table
    switch to 60: 26-28 [WU1,3] 28-30 [WU2,4]
    switch back to 50: 30-...[WU1,3] 32-...[WU2,4]
*/

  if(!draw_line_off && shifter_freq_at_start_of_vbl==50)
  {

    ///t=LINECYCLE0+28; //trigger point (works with 26?)
#if defined(SSE_SHIFTER_STATE_MACHINE)

//    ASSERT( !WU_sync_modifier || ST_TYPE!=STE );
//

#if defined(SSE_SHIFTER_STATE_MACHINE2) // really
    t=28+WU_sync_modifier;
    if(FreqAtCycle(t)==50 && FreqAtCycle(t+4)==60)
#else  // more or less
    t=26+WU_sync_modifier;
    if(FreqChangeAtCycle(t)==60 || FreqChangeAtCycle(t+2)==60)
#endif
      CurrentScanline.Tricks|=TRICK_BLACK_LINE;
#else
    t=LINECYCLE0+28; //trigger point (works with 26?)
#if defined(SSE_MMU_WU_SHIFTER_TRICKS)
    if(MMU.WakeUpState2())
      t+=2;//WU2_PLUS_CYCLES; // it's different for sync
#endif


    if(act-t>0)
    {
      i=CheckFreq(t);
      if(i>=0 && shifter_freq_change[i]==60 && shifter_freq_change_time[i]==t)
        CurrentScanline.Tricks|=TRICK_BLACK_LINE;
    }
#endif
  }

  if(!draw_line_off && (CurrentScanline.Tricks&TRICK_BLACK_LINE))
  {
    //ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );
    //TRACE_LOG("%d BLK\n",scan_y);
    draw_line_off=true;
    memset(PCpal,0,sizeof(long)*16); // all colours black
  }


  //////////////////////
  // MED RES OVERSCAN //
  //////////////////////

/*  Overscan (230byte lines) is possible in medium resolution too. Because the
    MMU may have fetched some bytes when the switch occurs, we need to 
    shift the display according to R1 cycle (No Cooper Greetings),
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

  if(!left_border && !(TrickExecuted&TRICK_OVERSCAN_MED_RES))
  {
    // look for switch to R1
    r1cycle=CycleOfLastChangeToShiftMode(1);
    if(r1cycle>16 && r1cycle<=40)
    {

      // look for switch to R0 before switch to R1
      r0cycle=PreviousShiftModeChange(r1cycle);
      if(r0cycle!=-1 && !ShiftModeChangeAtCycle(r0cycle))
      {
        CurrentScanline.Tricks|=TRICK_OVERSCAN_MED_RES;
        TrickExecuted|=TRICK_OVERSCAN_MED_RES;
        int cycles_in_low_res=r1cycle-r0cycle;
#if defined(SSE_SHIFTER_MED_OVERSCAN_SHIFT)
        ShiftSDP(-(((cycles_in_low_res)/2)%8)/2); //oops - disappeared in 3.5.2
        shifter_pixel+=4; // hmm...
#endif
#if defined(SSE_VID_BPOC)
        if(BORDER_40 && SSE_HACKS_ON && cycles_in_low_res==16) 
        { // fit text of Best Part of the Creation on a 800 display
          ShiftSDP(4);      
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
          shifter_pixel+=4; 
#endif
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
    D4/NGC (lines 76-231) 0R2 12R0 20R1 XR0
015 - 012:R0000 020:R0001 036:R0000 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2071
    D4/Nightmare (lines 143-306, cycles 28 32 36 40)
*/
  
#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_4BIT_SCROLL)

  if(!left_border && screen_res==1 && !(TrickExecuted&TRICK_4BIT_SCROLL))
  {
    r1cycle=CycleOfLastChangeToShiftMode(1);
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

#if defined(SSE_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK____) //?
/*  Strange corrections suddenly necessary at some point, quick patch. 
    when it's necessary or not is a great mystery (undef v3.5.4)
    TODO
*/
        if(SSE_HACKS_ON && DISPLAY_SIZE>0 && cycles_in_low_res==8) 
        {
          switch(shift_in_bytes)
          {
          case 0:
            shift_in_bytes=0;
            break;
          case 2:
            shift_in_bytes=2;
            break;
          case 4:
            shift_in_bytes=-4; //NGC+Nightmare VLB OK:-4
            break;
          case 6:
            shift_in_bytes=-2; //NGC VLB OK:-2
            break;
          }
          shift_in_bytes+=8;
        }
#endif

        ShiftSDP(shift_in_bytes);
        HblPixelShift=13+8-cycles_in_med_res-8; // -7,-3,1, 5, done in Render()
#if defined(SSE_DEBUG)
        ASSERT( HblPixelShift==-7||HblPixelShift==-3||HblPixelShift==1||HblPixelShift==5 );
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
  // this compiles to compact code
  if(ST_TYPE==STE && !shifter_hscroll_extra_fetch && CyclesIn>36 
    && !(TrickExecuted&(TRICK_0BYTE_LINE | TRICK_LINE_PLUS_26
    | TRICK_LINE_PLUS_20 | TRICK_4BIT_SCROLL | TRICK_OVERSCAN_MED_RES
    | TRICK_LINE_PLUS_4 | TRICK_LINE_PLUS_6)))
  {
    t=FreqAtCycle(DEcycle60)==50 ? 44 : 40; 
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

#endif
  /////////////
  // LINE +2 //
  /////////////

/*  
    A line that starts as a 60hz line and ends as a 50hz line gains 2 bytes 
    because 60hz lines start and stop 4 cycles before 50hz lines.
    This is used in some demos, but most cases are accidents, especially
    on the STE.
    On the STE, the GLUE checks frequency earlier because of possible horizontal
    scrolling.
    Notice that the check happens at the same cycle whether HSCROLL is activ
    or not, but display will start early only if it is active.
    This points to the GLUE holding the result of its decision in some internal
    register, or to it feeding the Shifter some 0 instead of video memory if
    HSCROLL=0.
	
    cases: Mindbomb, Forest, LoSTE screens
    spurious to avoid as STE (emulation problem): DSoTS, Overscan, Panic...
    
    [rest of comment nuked, some other timing issues were interfering]
*/

#if defined(SSE_SHIFTER_LINE_PLUS_2_TEST)
  if(!(TrickExecuted&TRICK_LINE_PLUS_2) && screen_res<2
    && left_border && !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) ) 
  {
#if defined(SSE_SHIFTER_LINE_PLUS_2_STE2)
/*  TEST11B: 40 for STE
*/
    if(ST_TYPE==STE)
    {
      t=THRESHOLD_LINE_PLUS_2_STE;
/*
#if defined(SSE_MMU_WU_DL)
      switch(MMU.WU[WAKE_UP_STATE]) 
      {
      case 1:
        t-=4; // Overscan Demos may flicker sometimes
        break;
      case 2:
        t+=8; // Cuddly Demos are generally shifted
        break;
      }//sw
#endif
*/
    }
#if defined(SSE_STF)
    else
      t=THRESHOLD_LINE_PLUS_2_STF+WU_sync_modifier;
#endif
    // old code
#elif defined(SSE_SHIFTER_LINE_PLUS_2_STE)
    t=52-2;
#ifdef SSE_STF
    if(ST_TYPE!=STE)
      t+=WU_sync_modifier +2 ;
#endif
#elif defined(SSE_SHIFTER_LINE_PLUS_2_STE_DSOS)
    t=52-12+2; 
#elif defined(SSE_SHIFTER_STATE_MACHINE)
    t=DEcycle60;
#else
    t=52-16+2;
#endif

#if !defined(SSE_SHIFTER_STATE_MACHINE)
#if defined(SSE_MMU_WU_SHIFTER_TRICKS)
    if(MMU.WakeUpState2())
      t+=2;
#endif
#if !defined(SSE_SHIFTER_LINE_PLUS_2_STE)
#if defined(SSE_STF)
    if(ST_TYPE!=STE)
#if defined(SSE_SHIFTER_LINE_PLUS_2_STE_DSOS)
      t+=12;
#else
      t+=16;
#endif
    else
#endif
    if(ShiftModeAtCycle(t)==1)
      t+=8;
#endif
#endif

  // the test is so-so, but we intend to change the way very soon
  // now just see that cases work

#if defined(SSE_SHIFTER_LINE_PLUS_2_STE2)
    if(CyclesIn>=t && FreqAtCycle(t)==60 
      && ((CyclesIn<372+WU_sync_modifier+2 && shifter_freq==50) 
      || FreqAtCycle(372+WU_sync_modifier+2)==50))
#else
    if(CyclesIn>=t && FreqAtCycle(t-2)==60 
      && ((CyclesIn<372+WU_sync_modifier+2 && shifter_freq==50) 
      || FreqAtCycle(372+WU_sync_modifier+2)==50))
#endif
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_2;
  }

#else  // Steem test
  if(shifter_freq_at_start_of_vbl==50 
    && (left_border==BORDER_SIDE
#if defined(SSE_SHIFTER_UNSTABLE)
    || (CurrentScanline.Tricks&TRICK_UNSTABLE) 
#endif
    )
    &&!(TrickExecuted&TRICK_LINE_PLUS_2))
  {   
#if defined(SSE_SHIFTER_LINE_PLUS_2_THRESHOLD)
    t=LINECYCLE0+52;
#else
    t=LINECYCLE0+58; 
#endif
    if(act-t>0) 
    {
      i=CheckFreq(t);
      if(i>=0 && shifter_freq_change[i]==60)
      {
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_2;
      }
    }
  }
#endif

  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_2)
    && !(TrickExecuted&TRICK_LINE_PLUS_2))
  {
    left_border-=4; // 2 bytes -> 4 cycles
    overscan_add_extra+=2;
    CurrentScanline.Bytes+=2;
    overscan=OVERSCAN_MAX_COUNTDOWN;
    TrickExecuted|=TRICK_LINE_PLUS_2;
    //TRACE_OSD("+2 y%d",scan_y);
  //  TRACE_LOG("+2 y %d c %d +2 60 %d 50 %d\n",scan_y,LINECYCLES,FreqChangeCycle(i),FreqChangeCycle(i+1));
  //  REPORT_LINE;
#if defined(SSE_VID_TRACE_LOG_SUSPICIOUS2)
    if(shifter_freq_change_time[i]-LINECYCLE0<0)
      TRACE_LOG("F%d Suspicious +2 y %d tmg sw %d tmg hbl %d diff %d\n",FRAME,scan_y,shifter_freq_change_time[i],cpu_timer_at_start_of_hbl,shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl);
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
    With two such "sync" lines, you can vertically scroll the screen.
    There were bugs in previous versions because we thought the monitor
    couldn't run too long in HIRES. Alien's text was correct about it but
    not Flix's.
    Just Buggin: 152:R0002 172:R0000
    ST-CNX: 160:R0002 172:R0000
    loSTE screens: 120[!]:R0002  392:R0000 
    
    Paolo table
    R2 56 [...] -> 164 [WU1] 166 [WU3,4] 168 [WU2]
*/

#if defined(SSE_SHIFTER_STATE_MACHINE)
  t=166+WU_res_modifier;
  if(CyclesIn>t && !(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
    && !(CurrentScanline.Tricks&TRICK_LINE_MINUS_106)
    && (ShiftModeAtCycle(t+2)&2))
     CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;
#elif defined(SSE_SHIFTER_STEEM_ORIGINAL) || 0
  t=cpu_timer_at_start_of_hbl+172; //trigger point for big right border
  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
     i=CheckFreq(t);
     if(i>=0 && shifter_freq_change[i]==MONO_HZ)
       CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;
  }
#elif defined(SSE_SHIFTER_TRICKS) && 1
  t=LINECYCLE0+164+2; // look for R2 <= 166
#if defined(SSE_MMU_WU_SHIFTER_TRICKS)
    if(MMU.WakeUpState2())
      t+=WU2_PLUS_CYCLES;
#endif
  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
     i=CheckShiftMode(t);
     if(i>=0 && shifter_shift_mode_change[i]==2 
       && ShiftModeChangeCycle(i)>=56)  
       CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;
  }
#endif

  // action
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_106)
    && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
    ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );
    overscan_add_extra+=-106;
    TrickExecuted|=TRICK_LINE_MINUS_106;
    CurrentScanline.Bytes+=-106;
#if !defined(SSE_VAR_RESIZE_370) 
    right_border_changed=true;
#endif
#if defined(SSE_SHIFTER_LINE_MINUS_106_BLACK)
/*  The MMU won't fetch anything more but Steem renders the full scanline.
    It's in fact data of next scanline. We make sure nothing ugly will appear
    (loSTE screens).
*/
    draw_line_off=true;
    memset(PCpal,0,sizeof(long)*16); // all colours black
#endif
#if defined(SSE_SHIFTER_UNSTABLE)
/*  We examine Line -106 before Destabilisation because otherwise it can
    get horribly complicated with eg ST-CNX, which randomly (?) sets R1 at
    cycle 148 of some sync lines.
001 - 012:R0000 148:R0001 160:R0002 172:R0000 444:R0000 456:R0000 512:R0000 512:T2004 512:#0054
*/
//    ASSERT( !Preload ); //DSOS!
#endif

  }


  /////////////////////
  // DESTABILISATION //
  /////////////////////

/*  Detect MED/LO switches during DE.
    This will load some words into the Shifter, the Shifter will start displaying
    shifted planes and shifted pixels in low res.
    Because there could be extra SDP shifts when rendering pixel shifts, we
    need to offset those here. Those hacks aren't important. (v3.6.0 removed)
*/

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_MED_RES_SCROLLING)
/*
Paulo:
detect unstable: switch MED/LOW - Beeshift
- 3 (screen shifted by 12 pixels because only 1 word will be read before the 4 are available to draw the bitmap);
- 2 (screen shifted by 8 pixels because only 2 words will be read before the 4 are available to draw the bitmap);
- 1 (screen shifted by 4 pixels because only 3 words will be read before the 4 are available to draw the bitmap);
- 0 (screen shifted by 0 pixels because the 4 words will be read to draw the bitmap);
*/

  if( WAKE_UP_STATE && ST_TYPE==STF // condition STF for now
    && !(CurrentScanline.Tricks&TRICK_UNSTABLE) 
    && !Preload // would complicate matters
    && left_border && LINECYCLES>56 && LINECYCLES<372 //'DE'
#if defined(SSE_SHIFTER_HI_RES_SCROLLING)
    && !(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
    && !(CurrentScanline.Tricks&TRICK_LINE_MINUS_106)  // of course
#endif
    ) 
  {
    r1cycle=NextShiftModeChange(56,1); // detect switch to medium
    if(r1cycle>-1)
    {
      r0cycle=NextShiftModeChange(r1cycle,0); // detect switch to low
      int cycles_in_med=r0cycle-r1cycle;
      if(cycles_in_med>0 && cycles_in_med<=20+4) 
      {
        Preload=((cycles_in_med/4)%4)+4-4;  // '+4' to make sure >1 (3.6.0 no)
        CurrentScanline.Tricks|=TRICK_UNSTABLE;//don't apply on this line
//        TRACE_LOG("y%d R1 %d R0 %d cycles in med %d Preload %d\n",scan_y,r1cycle,r0cycle,cycles_in_med,Preload);
      }
    }
#if defined(SSE_SHIFTER_HI_RES_SCROLLING)
/*  
  We had to move this part after the check for 'line -106'. Adding R2/R0 
  switches is always more "dangerous" in emulation.
  Paulo:
  LO/HI/LO works in the same way except -4 and -12 code cases are switched;
 
  Cycles           8      12     16      20
  Preload MED      2       3      4       5
  Preload HI       2       5      4       3 
  Strange but correct: demos beeshift3 and beesclr4 don't glitch when you
  press ESC.
*/
    else 
    {
      r2cycle=NextShiftModeChange(56,2); // detect switch to high
      if(r2cycle>-1)
      {
        r0cycle=NextShiftModeChange(r2cycle,0); // detect switch to low
        int cycles_in_high=r0cycle-r2cycle;
        if(cycles_in_high>0 && cycles_in_high<=20+4)
        {
          Preload=((cycles_in_high/4)%4);
          CurrentScanline.Tricks|=TRICK_UNSTABLE;//don't apply on this line
          if(Preload&1) // if it's 3 or 5
            Preload+=(4-Preload)*2; // swap value
//          Preload+=4; // like for MED, hack for correct SDP shift
//          TRACE_LOG("y%d R1 %d R0 %d cycles in HI %d Preload %d\n",scan_y,r1cycle,r0cycle,cycles_in_high,Preload);
        }
      }
    }
#endif

  }
  
#endif

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_UNSTABLE)
/*  Shift due to unstable Shifter, caused by a line+26 without stabiliser
    (Dragon) or a MED/LOW switch during DE, or a HI/LOW switch during DE, 
    or a line+230 without stabiliser:
    apply effect
    Note this isn't exact science, it's in development.
    DOLB, Omega: 3 words preloaded but it becomes 2 due to left off
    Dragon: 1 word preloaded    
    It could be that both determining preload and using preload are wrong!
    v3.6.0: it's one word for +26 or 230; ignore when line +2 (TODO)
*/
  
  if( 
#if !defined(SSE_SHIFTER_DOLB_STE)
    ST_TYPE==STF &&
#endif
    WAKE_UP_STATE // 1 & 2
    && Preload
    && !(CurrentScanline.Tricks&TRICK_UNSTABLE)
    && !(CurrentScanline.Tricks&TRICK_LINE_PLUS_2) // Beescroll
    && CyclesIn>56)   // wait for eventual +2
  {

    // 1. planes
    int shift_sdp=-(Preload%4)*2; // unique formula
//    if(Preload>1 && shift_sdp) // and...      
  //    shift_sdp+=8; // one hack less!
    ShiftSDP(shift_sdp); // correct plane shift

#if defined(SSE_SHIFTER_PANIC) //fun, bad scanlines (unoptimised code!)
    if(WAKE_UP_STATE==WU_SHIFTER_PANIC)
    {

#if defined(SSE_SHIFTER_UNSTABLE_DOLB) && defined(SSE_STF)
//those hacks are looping...
        if(Preload && SSE_HACKS_ON ) 
          shift_sdp-=2;

#endif

      shift_sdp+=shifter_draw_pointer_at_start_of_line;
#if defined(SSE_SHIFTER_PANIC2) //switch border bands
      for(int i=0+8-8;i<=224-8-8;i+=16)
#else
      for(int i=0+8;i<=224-8;i+=16)
#endif
      {
        Scanline[i/4]=LPEEK(i+shift_sdp); // save 
        Scanline[i/4+1]=LPEEK(i+shift_sdp+4);
        LPEEK(i+shift_sdp)=0;  // interleaved "border" pixels
        LPEEK(i+shift_sdp+4)=0;
      }
      
      Scanline[230/4+1]=shift_sdp;
    }
#endif

    // 2. pixels
    // Beeshift, the full frame shifts in the border
    // Dragon: shifts -4, just like the 230 byte lines below
    if(left_border)
    {
      left_border-=(Preload%4)*4;
      right_border+=(Preload%4)*4;
    }
#if defined(SSE_SHIFTER_DOLB_SHIFT1) 
    else  //  hack for DOLB, Omega, centering the pic
    {
#if defined(SSE_SHIFTER_DOLB1)
      if(SSE_HACKS_ON)
        ShiftSDP(8); // again...
#endif
      if(SSE_HACKS_ON 
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
/*  The fact that the screen of DOLB is good without a special
    adjustment supports our theory of "no shift, 52 pixels".
    With display size 412, it's very close to the screenshot.
*/
        && (SideBorderSize!=VERY_LARGE_BORDER_SIDE
#if defined(SSE_VID_BORDERS_416_NO_SHIFT1)
        ||border
#endif
        )
#endif
        )
        HblPixelShift=4; 
    }
#endif
    //TRACE_LOG("Y%d Preload %d shift SDP %d pixels %d lb %d rb %d\n",scan_y,Preload,shift_sdp,HblPixelShift,left_border,right_border);

    CurrentScanline.Tricks|=TRICK_UNSTABLE;
  }
#endif
 
  /////////////
  // LINE -2 //
  /////////////

/*  
    A line starting at 50hz but switched to 60hz before end of DE for
    60hz will fetch 2 bytes less of video memory.

    It's a -2 line if it's at 60hz at cycle 372, the switch doesn't
    need to happen at 372, though it makes more sense there. Sooner, there
    could be distortion on a real ST.

    Thresholds/WU states (from table by Paolo)

      60hz  58 - 372 WU1,3
            60 - 374 WU2,4

      50hz  374 -... WU1,3
            376 -... WU2,4

*/

//  if( shifter_freq_at_start_of_vbl!=50)
//    return; 

#if defined(SSE_SHIFTER_STATE_MACHINE)
  t=372+WU_sync_modifier;

#if defined(SSE_SHIFTER_LINE_PLUS_2_STE) || defined(SSE_SHIFTER_LINE_PLUS_2_STE3)
  if(CyclesIn>372 && FreqAtCycle(56+2+WU_sync_modifier)!=60 && FreqAtCycle(t+2)==60)
#else
  if(CyclesIn>372 && FreqAtCycle(DEcycle60+2)!=60 && FreqAtCycle(t+2)==60)
#endif
    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
    && !(CurrentScanline.Tricks&TRICK_LINE_MINUS_106)) //eg Pete Mega Four #1
      CurrentScanline.Tricks|=TRICK_LINE_MINUS_2;
#else

  // Steem test
#if defined(SSE_MMU_WU_DL) 
  t=LINECYCLE0+372+WU_sync_modifier;
#else
  t=LINECYCLE0+372; //trigger point for early right border
#if defined(SSE_MMU_WU_SHIFTER_TRICKS)
    if(MMU.WakeUpState2())
      t+=2;//WU2_PLUS_CYCLES;
#endif
#endif


  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_2))
  {
    i=CheckFreq(t);
    if(i==-1)
      return;
    if(shifter_freq_change[i]==60
#if defined(SSE_SHIFTER_LEFT_OFF_60HZ)
/*  An obvious condition for a line-2 to work is that the display
    should be at 50hz when the switch occurs.
    I knew that but left it without check for a long time because it
    made SNYD/TCB seem to work. But it was bogus, the "-2" comes from
    the left border removal granting +24 insted of +26 for a 60hz
    scanline. Finally fixed in v3.5.3.
*/
      && FreqAtCycle(shifter_freq_change_time[i]-LINECYCLE0)!=60
#endif
//      && FreqAtCycle(52)!=60 //stf... and checked later (?)
      )
      CurrentScanline.Tricks|=TRICK_LINE_MINUS_2;
  }
#endif

#if defined(SSE_VID_TRACE_LOG_SUSPICIOUS2)
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)&&!(shifter_freq_change_time[i]-LINECYCLE0>56))
    TRACE_LOG("F%d Suspicious -2 y %d tmg sw %d tmg hbl %d diff %d\n",FRAME,scan_y,shifter_freq_change_time[i],LINECYCLE0,shifter_freq_change_time[i]-LINECYCLE0);
#endif

  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
    &&!(TrickExecuted&TRICK_LINE_MINUS_2))
  {
    //TRACE("-2\n");
    //ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );//false alerts anyway

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_UNSTABLE)//MFD
// wrong, of course, just to have Overdrive menu OK when you come back
//   if(Preload)//==3) //3.6.0: other way now, see EndHBL()
  //    Preload=0;
#endif

    overscan_add_extra+=-2;
    CurrentScanline.Bytes+=-2;
    TrickExecuted|=TRICK_LINE_MINUS_2;
#if !defined(SSE_VAR_RESIZE_370) 
    right_border_changed=true;
#endif
//    TRACE_LOG("-2 y %d c %d s %d e %d ea %d\n",scan_y,LINECYCLES,scanline_drawn_so_far,overscan_add_extra,ExtraAdded);
  }


  // TODO right off for 60hz...

  /////////////////////////////////
  // RIGHT BORDER OFF (line +44) // 
  /////////////////////////////////

/* 
    A sync switch to 0 (60hz) at cycle 376 (end of display for 50hz)
    makes the GLUE fail to stop the line (DE still on).
    DE will stop only at cycle of HSYNC, 464.
    This is 88 cycles later and the reason why the trick grants 44 more
    bytes of video memory for the scanline.
    However, because of a HI/LO stabiliser or HBLANK at cycle 424, only
    about 48 more pixels are visible.
    For a 230byte line DE is on from cycle 4 to 464 (230x2=460).
    Because a 60hz line stops at cycle 372, the sync switch must hit just
    after that and right before the test for end of 50hz line occurs.
    That's why cycle 376 is targeted, but according to wake-up state other
    timings may work.

    WS thresholds (from table by Paolo) 

    Swtich to 60hz  374 - 376 WS1,3
                    376 - 378 WS2,4

    Switch back to 50hz  378 -... WS1,3
                         380 -... WS2,4

    Nostalgia menu: must be bad display in WU2

*/

#if defined(SSE_SHIFTER_STATE_MACHINE)
/*
    We used the following to calibrate (Beeshift3).

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
  
  t=374+WU_sync_modifier;
#if defined(SSE_VAR_RESIZE_370) 
  if(CyclesIn<t || (CurrentScanline.Tricks&(TRICK_0BYTE_LINE
    |TRICK_LINE_MINUS_2|TRICK_LINE_MINUS_106|TRICK_LINE_PLUS_44)))
#else
  if(right_border_changed || CyclesIn<t 
    || (CurrentScanline.Tricks&TRICK_0BYTE_LINE)
    || (CurrentScanline.Tricks&TRICK_LINE_MINUS_106))
#endif
    ;
  // sync
  else if(FreqChangeAtCycle(t+2)==60 || FreqChangeAtCycle(t)==60)
    CurrentScanline.Tricks|=TRICK_LINE_PLUS_44;
#if defined(SSE_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE)
/*  Like Alien said, it is also possible to remove the right border by setting
    the shiftmode to 2. This may be done well before cycle 376 (state-machine
    approach), that's why we use ShiftModeAtCycle() and not 
    ShiftModeChangeAtCycle().

    WS threshold: WS1:376  WS3,4:378  WS2:380

*/
  else
  {  
    // res
    t=378+WU_res_modifier;
    if(ShiftModeAtCycle(t+2)&2) // still this damn offset
      CurrentScanline.Tricks|=TRICK_LINE_PLUS_44;
  }
#endif

#else //old code
  t=LINECYCLE0+376; //trigger point for right border cut
#if defined(SSE_MMU_WU_DL)
  t+=WU_sync_modifier+1;
#endif
#if defined(SSE_MMU_WU_SHIFTER_TRICKS) && !defined(SSE_MMU_WU_DL)
  if(MMU.WakeUpState2())
    t+=2;
#endif
  if(act-t>=0 && !(CurrentScanline.Tricks&TRICK_LINE_MINUS_2))
  {
    i=CheckFreq(t);
    if(i!=-1)
    {// it looks for the NEXT change too!
      if(i!=shifter_freq_change_idx)
      {
        int i1=(i+1) & 31;
        if(shifter_freq_change_time[i1]-t<=0) 
          i=i1;
      }
#if defined(SSE_MMU_WU_RIGHT_BORDER) && !defined(SSE_MMU_WU_IO_BYTES_W_SHIFTER_ONLY)
      int threshold= (MMU.WakeUpState2()) ? 376 : 374;
#elif defined(SSE_MMU_WU_IO_BYTES_W_SHIFTER_ONLY)
      int threshold= (MMU.WakeUpState2()) ? 376 : 374;
#else
      int threshold=374;
#endif
      if(shifter_freq_change[i]!=50 
#if defined(SSE_SHIFTER_TRICKS)
        && FreqAtCycle(threshold)==50
#endif
        && shifter_freq_change_time[i]-LINECYCLE0>372 
        )
      {
#if defined(SSE_SHIFTER_OMEGA)
        if(SSE_HACKS_ON && PreviousScanline.Cycles==508 && CurrentScanline.Cycles==508
          && FreqAtCycle(threshold+4)==60)
        {
          TRACE_LOG("Spurious right off\n");
          shifter_draw_pointer+=-4; // hack (temp :))
          CurrentScanline.Bytes+=-4;    
        }
        else
#endif
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_44;
      }
    }
  }
#endif

  // action
  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_44)
    && !(TrickExecuted&TRICK_LINE_PLUS_44))
  {
    ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) );
    ASSERT(!(CurrentScanline.Tricks&TRICK_LINE_MINUS_2));
    right_border=0;
    overscan_add_extra+=OVERSCAN_ADD_EXTRA_FOR_RIGHT_BORDER_REMOVAL;  // 28 (+16=44)
    TrickExecuted|=TRICK_LINE_PLUS_44;
#if defined(SSE_VID_BORDERS)
    if(SideBorderSize==VERY_LARGE_BORDER_SIDE)
      overscan_add_extra-=BORDER_EXTRA/2; // 20 + 24=44
#endif
    CurrentScanline.Bytes+=44;
    CurrentScanline.EndCycle=464; //or 460?
    overscan=OVERSCAN_MAX_COUNTDOWN; 
#if !defined(SSE_VAR_RESIZE_370) 
    right_border_changed=true;
#endif
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
    This is also handled by this routine, in fact it would even take a R0/R0!
    v3.6.0: we don't take R0/R0 anymore: Ventura/Naos
*/


#if defined(SSE_SHIFTER_TRICKS) && defined(TRICK_STABILISER)
  if(!(CurrentScanline.Tricks&TRICK_STABILISER))
  {
//    ASSERT( !(CurrentScanline.Tricks&TRICK_0BYTE_LINE) ); //asserts
    r2cycle=NextShiftModeChange(432/*,2*/); // Forest STF2
    if(r2cycle>-1 && r2cycle<460 && ShiftModeChangeAtCycle(r2cycle))
    {
      r0cycle=NextShiftModeChange(r2cycle,0);
      if(r0cycle>-1 && r0cycle<464) 
      {
        CurrentScanline.Tricks|=TRICK_STABILISER;
#if defined(SSE_SHIFTER_UNSTABLE)
        Preload=0; // "reset" empties RR
#endif
      }
    }
  }
#endif


  /////////////////////////////////////////////////////////
  // 0-BYTE LINE 2 (next scanline) and NON-STOPPING LINE //
  /////////////////////////////////////////////////////////


#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_0BYTE_LINE)
#if defined(SSE_SHIFTER_STATE_MACHINE)

/*  
    v3.5.4
    We use the values in LJBK's table, taking care not to break
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
    Hackabonds Demo hits at 504, 0-byte undesired: WS1 only?
    Note that if we relax the threshold here, Hackabonds will still
    display wrong because of another WS1 threshold. Curious case. TODO
          
*/
    if(!(NextScanline.Tricks&TRICK_0BYTE_LINE))
    {

      r2cycle=460-2; 
#if defined(SSE_STF_0BYTE)
      if(ST_TYPE!=STE)
        r2cycle+=2; 
#endif

      if(FreqAtCycle(r2cycle+2)==50)
        r2cycle+=4;        
      
      if(CyclesIn>r2cycle && ShiftModeAtCycle(r2cycle+WU_res_modifier+2)&2)
      {
        //REPORT_LINE;
        if(right_border)
        {
          NextScanline.Tricks|=TRICK_0BYTE_LINE; // next, of course
          TRACE_LOG("detect Plax type 0byte y %d\n",scan_y+1);
          shifter_last_draw_line++;//?
        }
/*  In the Enchanted Land hardware tests, a HI/LO switch at end of display
    when the right border has been removed causes the Shifter (MMU) to continue 
    fetching for the rest of the line (24 bytes).
    I don't see that the left border be removed on the next line (in this
    emulation, see Hatari).
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

    The test fails in WS1: correct?

*/
        else if(CurrentScanline.Bytes==160+44) // trick to do it only once
        {
          TRACE_LOG("F%d y%d c%d Enchanted Land HW test R2 %d R0 %d\n",FRAME,scan_y,CyclesIn,NextShiftModeChange(460-4,2),NextShiftModeChange(460,0));
          CurrentScanline.Bytes+=24; // "double" right off, fixes Enchanted Land
        }
      }
      else 
      {
        r2cycle+=38; // -> 502 (STE)

#if defined(SSE_STF_0BYTE) && !defined(SSE_SHIFTER_0BYTE_LINE_RES_END_THRESHOLD)
        if(ST_TYPE!=STE)
          r2cycle+=2; // -> 504 (STF)
#endif
        if(CyclesIn>r2cycle && ShiftModeAtCycle(r2cycle+WU_res_modifier+2)&2)
        {
          NextScanline.Tricks|=TRICK_0BYTE_LINE;
          //REPORT_LINE;
          TRACE_LOG("detect NBL type 0byte y %d\n",scan_y+1);
          shifter_last_draw_line++;
        }
      }
    }

#endif//state-machine
#endif//0byte


}


void TShifter::CheckVerticalOverscan() {

/* Vertical overscan

    1) The two different MMU are not emulated, we emulate the common ST with
    29 bonus top lines.

    2) We don't emulate the 2 bottom extra lines.

    3) 277 lines are available in overscan. Steem's reckoning: -29-0-246
    Generally only 276 lines are useful, and in many programs the last line
    contains trash (because programmers couldn't see it on their monitor)

    4) The thresholds are very tricky.
    The switch to 60hz mustn't happen too late or it doesn't count.
    It must happen before cycle 502 on a STE and before cycle 506 on a STF. 
    But the switch back to 50hz may happen at cycle 500 on a STE (RGBeast),
    504 (WU1), 506 (WU2) on a STF. 
    So there's overlap, just checking frequency at cycle c is not enough.
    Those values are not based on measurements but on how programs work or not.
    Still testing.
    There was variation in some STE & STF and typically some demos wouldn't
    work on some machines because of top/bottom borders.

    STE cases:
    -E605 planet (500:S0)
    -RGBeast (480:S0, sometimes 500:S2)
    
    STF cases:
    -3615Gen4 Cakeman

    -European Demos (top 444 60) When it misses there's just no 60 (VBI?)

    -Musical Wonder 90 (199 416:S0,504:S2) 

    -SNYD2/Sync Vectorballs II (F6) 424:S0000 504:S0002 (every time)
    "the low border of Sync's vectorballs was never stable on my machine"
    This means the MMU didn't react the same to same value in every frame.

    5) See No Cooper greeting, trying a switch at end of line 182, then end
    of line 199. 199-182 = 17?

    6) I know no cases of programs making the screen smaller. It was small
    enough! TODO PYM big border?

    7) The thresholds are wake-up sensitive, ijor's test program uses this
    (and so, this isn't merely a hack).
    This could explain why some demos don't seem to work on a machine,
    according to users.
    TODO: there's got to be a difference between STE as well, but CPU speed
    could be responsible too.

    -Auto 168 (WU1 OK, WU2, STE flicker)
    30 - 488:S0000 512:S0002 512:T0100
    30 - 496:S0000 520:S0002 512:T0100
    30 - 480:S0000 504:S0002 512:T0100

    -Decade title (WU1, STE OK, WU2 flicker)
    Y-30 C524  372:S0000 520:S0002 OK
    Y-30 C520  356:S0000 504:S0002 fails in WU2

    - Dragonnels/Happy Islands: WU1

    -Nostalgia/Lemmings end scroller WS3
    Y-30 C520  448:S0000 504:S0002 sometimes -> breaks screen in WU2

    -Superior 85: WU1
    Y-30 C520  420:S0000 504:S0002 sometimes -> flicker in WU2


*/

  enum{NO_LIMIT=0,LIMIT_TOP,LIMIT_BOTTOM};

  BYTE on_overscan_limit=NO_LIMIT;
  if(scan_y==-30)
    on_overscan_limit=LIMIT_TOP;
  else if(scan_y==shifter_last_draw_line-1 && scan_y<245)
    on_overscan_limit=LIMIT_BOTTOM;
#if defined(SSE_SHIFTER_VERTICAL_OPTIM1)
  else
    return;
#endif

  int t,i=shifter_freq_change_idx;
  int freq_at_trigger=0; 
  if(screen_res==2) 
    freq_at_trigger=MONO_HZ;
  if(emudetect_overscans_fixed) 
    freq_at_trigger=(on_overscan_limit) ? 60:0;

#if defined(SSE_SHIFTER_STEEM_ORIGINAL) || defined(SSE_VID_VERT_OVSCN_OLD_STEEM_WAY)
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50 
    && freq_change_this_scanline)
  {
    t=cpu_timer_at_start_of_hbl+CYCLES_FROM_HBL_TO_RIGHT_BORDER_CLOSE+98; // 502
    i=CheckFreq(t); 
    freq_at_trigger=(i>-1) ? shifter_freq_change[i] : 0;
  }
#endif

#if defined(SSE_SHIFTER_TRICKS) && !defined(SSE_VID_VERT_OVSCN_OLD_STEEM_WAY)
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50)
  {
    t=VERT_OVSCN_LIMIT; //502
#if defined(SSE_STF) && defined(SSE_STF_VERTICAL_OVERSCAN) && defined(SSE_STF_VERT_OVSCN) //!
    if(ST_TYPE!=STE)
      t+=STF_VERT_OVSCN_OFFSET; //4
#endif


#if defined(SSE_MMU_WU_VERTICAL_OVERSCAN)
/*  ijor's wakeup.tos test
WU1:
Y-30 C512  496:S0000 504:S0002 Shifter tricks 100
Y-30 C516  504:S0000 512:S0002 -
WU2:
Y-30 C512  496:S0000 504:S0002 -
Y-30 C516  504:S0000 512:S0002 Shifter tricks 100
Problem: too many cases of WU1, that should be the rarer one
*/

#if defined(SSE_MMU_WU_DL)
    if(MMU.WU[WAKE_UP_STATE]==1
#if defined(SSE_MMU_WU_VERTICAL_OVERSCAN1)
      || !WAKE_UP_STATE //3.6.3 defaults to this for several cases
      && ST_TYPE!=STE//3.6.4
#endif
      )
#else
    if(MMU.WakeUpState1()) // OK WS1, WS3
#endif
        t-=2;
#endif

    if(FreqAtCycle(t)==60 
#if defined(SSE_SHIFTER_STE_VERTICAL_OVERSCAN) 
      || FreqAtCycle(t-2)==60 && FreqChangeAtCycle(t-2)==50 // fixes RGBeast
#if defined(SSE_STF) && defined(SSE_MMU_WU_VERTICAL_OVERSCAN)
      && ST_TYPE==STE
#endif
#endif
      )
      freq_at_trigger=60;
  }
#if defined(SSE_SHIFTER_60HZ_OVERSCAN) 
/*  Removing lower border at 60hz. Simpler test, not many cases.
    Fixes Leavin' Teramis in monitor mode.
*/
  else if(on_overscan_limit==LIMIT_BOTTOM && shifter_freq_at_start_of_vbl==60 
    && FreqAtCycle(502-4)==50) 
      CurrentScanline.Tricks|=TRICK_BOTTOM_OVERSCAN_60HZ; 
#endif
#endif

  if(on_overscan_limit && freq_at_trigger==60 // a bit messy, it's because we
    && shifter_freq_at_start_of_vbl==50) // keep the possibility of "old way"
  {
    CurrentScanline.Tricks|= (on_overscan_limit==LIMIT_TOP) 
      ? TRICK_TOP_OVERSCAN: TRICK_BOTTOM_OVERSCAN;
  }

  if(CurrentScanline.Tricks&
    (TRICK_TOP_OVERSCAN|TRICK_BOTTOM_OVERSCAN|TRICK_BOTTOM_OVERSCAN_60HZ))
  {
    overscan=OVERSCAN_MAX_COUNTDOWN;
    time_of_next_timer_b=time_of_next_event+cpu_cycles_from_hbl_to_timer_b
      + TB_TIME_WOBBLE; 
#if defined(SSE_GLUE) && defined(SSE_INT_MFP_TIMER_B_AER) //v3.7 
    if(mfp_reg[1]&8)
      time_of_next_timer_b-=Glue.DE_cycles[shifter_freq_idx];
#endif
    if(on_overscan_limit==LIMIT_TOP) // top border off
    {
      shifter_first_draw_line=-29;
      if(FullScreen && border==2) // TODO a macro
      {    //hack overscans
        int off=shifter_first_draw_line-draw_first_possible_line;
        draw_first_possible_line+=off;
        draw_last_possible_line+=off;
      }
    }
    else // bottom border off
      shifter_last_draw_line=247; //?
  }

#if defined(SSE_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN)
  if(on_overscan_limit && TRACE_ENABLED) 
  {
    FrameEvents.ReportLine();
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),CurrentScanline.Tricks);
  }
#endif

#if defined(SSE_SHIFTER_VERTICAL_OVERSCAN_TRACE)//MFD
  if(on_overscan_limit) 
  {
   ///////// FrameEvents.ReportLine();
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),CurrentScanline.Tricks);
  //  ASSERT( scan_y!=199|| (CurrentScanline.Tricks&TRICK_BOTTOM_OVERSCAN) );
    //ASSERT( scan_y!=199|| shifter_last_draw_line==247 );
  }
#endif

#if defined(SSE_BOILER_TRACE_CONTROL)
  if(TRACE_ENABLED&&(TRACE_MASK1 & TRACE_CONTROL_VERTOVSC)) 
  {
    FrameEvents.ReportLine();
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),CurrentScanline.Tricks);
  //  ASSERT( scan_y!=199|| (CurrentScanline.Tricks&TRICK_BOTTOM_OVERSCAN) );
    //ASSERT( scan_y!=199|| shifter_last_draw_line==247 );
  }
#endif

/* no!
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
    if(on_overscan_limit
      && Debug.FrameReportMask&FRAME_REPORT_MASK_VERTICAL_OVERSCAN)
  {
    FrameEvents.ReportLine();
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),CurrentScanline.Tricks);
  }

#endif
*/
}


void TShifter::DrawScanlineToEnd()  { // such a monster wouldn't be inlined
#ifdef SSE_SHIFTER_DRAW_DBG
  draw_scanline_to_end();
  return;
#endif
  MEM_ADDRESS nsdp; 
#ifndef NO_CRAZY_MONITOR // SS: we don't care about this yet
  if (emudetect_falcon_mode!=EMUD_FALC_MODE_OFF){
    int pic=320*emudetect_falcon_mode_size,bord=0;
    // We double the size of borders too to keep the aspect ratio of the screen the same
    if (border & 1) bord=BORDER_SIDE*emudetect_falcon_mode_size;
    if (scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line){
      bool in_border=(scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border);
      bool in_pic=(scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line);
      if (in_pic || in_border){
        // We only have 200 scanlines, but if emudetect_falcon_mode_size==2
        // then the picture must be 400 pixels high. So to make it work we
        // draw two different lines at the end of each scanline.
        // To make it more confusing in some drawing modes if
        // emudetect_falcon_mode_size==1 we have to draw two identical
        // lines. That is handled by the draw_scanline routine. 
        for (int n=0;n<emudetect_falcon_mode_size;n++){
          if (in_pic){
            nsdp=shifter_draw_pointer + pic*emudetect_falcon_mode;
            draw_scanline(bord,pic,bord,HSCROLL);
            AddExtraToShifterDrawPointerAtEndOfLine(nsdp);
            shifter_draw_pointer=nsdp;
          }else{
            draw_scanline(bord+pic+bord,0,0,0);
          }
          draw_dest_ad=draw_dest_next_scanline;
          draw_dest_next_scanline+=draw_dest_increase_y;
        }
      }
    }
    shifter_pixel=HSCROLL; //start by drawing this pixel
    scanline_drawn_so_far=BORDER_SIDE+320+BORDER_SIDE;
  }else if (extended_monitor){
    int h=min(em_height,Disp.SurfaceHeight);
    int w=min(em_width,Disp.SurfaceWidth);
    if (extended_monitor==1){	// Borders needed, before hack
      if (em_planes==1){ //mono
        int y=h/2-200, x=(w/2-320)&-16;
        if (scan_y<h){
          if (scan_y<y || scan_y>=y+400){
            draw_scanline(w/16,0,0,0);
          }else{
            draw_scanline(x/16,640/16,w/16-x/16-640/16,0);
            shifter_draw_pointer+=80;
          }
        }
      }else{
        int y=h/2-100, x=(w/2-160)&-16;
        if (scan_y<h){
          if (scan_y<y || scan_y>=y+200){
            draw_scanline(w,0,0,0);
          }else{
            draw_scanline(x,320,w-x-320,0);
            shifter_draw_pointer+=160;
          }
        }
      }
      draw_dest_ad=draw_dest_next_scanline;
      draw_dest_next_scanline+=draw_dest_increase_y;
    }else{
      if (scan_y<h){
        if (em_planes==1) w/=16;
        if (screen_res==1) w/=2; // medium res routine draws two pixels for every one w
        draw_scanline(0,w,0,0);
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }
      int real_planes=em_planes;
      if (screen_res==1) real_planes=2;
      shifter_draw_pointer+=em_width*real_planes/8;
    }
  }else
#endif// #ifndef NO_CRAZY_MONITOR
  
  // Colour

  if(screen_res<2)
  {
    Render(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320+BORDER_SIDE,DISPATCHER_DSTE);
#if defined(WIN32)
    DrawBufferedScanlineToVideo();
#endif
    if(scan_y>=draw_first_possible_line 
      && scan_y<draw_last_possible_line)
    {
      // thsee variables are pointers to PC video memory
      draw_dest_ad=draw_dest_next_scanline;
      draw_dest_next_scanline+=draw_dest_increase_y;
    }
    shifter_pixel=HSCROLL; //start by drawing this pixel
#if defined(SSE_SHIFTER_STE_MED_HSCROLL) 
    HblStartingHscroll=shifter_pixel; // save the real one (not important for Cool STE)
    if(screen_res==1) //it's in cycles, bad name
      shifter_pixel=HSCROLL/2; // fixes Cool STE jerky scrollers
#endif
  }
  // Monochrome
  else 
  {
    if(scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line)
    {


#if defined(SSE_SHIFTER_HIRES_OVERSCAN)
      if(freq_change_this_scanline)
      {
        CheckSideOverscan();
        if(draw_line_off)
        {
          for(int i=0;i<=20;i++)
          {
            Scanline[i]=LPEEK(i*4+shifter_draw_pointer); // save ST memory
            LPEEK(i*4+shifter_draw_pointer)=0;
          }
        }
      }
#endif
      nsdp=shifter_draw_pointer+80;

#if defined(SSE_SHIFTER_STE_HI_HSCROLL) 
/*  shifter_pixel isn't used, there's no support for hscroll in the assembly
    routines drawing the monochrome scanlines.
    Those routines work with a word precision (16 pixels), so they can't be
    used to implement HSCROLL.
    Since I can't code in assembly yet I provide the feature in C.
    At least in monochrome we mustn't deal with bit planes, there's only
    one plane.
    We need some examples to test the feature with a correctly set-up screen.
    SSE_SHIFTER_STE_HI_HSCROLL is defined only in the beta or the boiler.
*/

      if(HSCROLL)
      {
        // save ST memory
        for(int i=0;i<=20;i++)
          Scanline[i]=LPEEK(i*4+shifter_draw_pointer); 
        // shift pixels of full scanline
        for(MEM_ADDRESS i=shifter_draw_pointer; i<nsdp ;i+=2)
        {
          WORD new_value=DPEEK(i)<<HSCROLL;  // shift first bits away
          new_value|=DPEEK(i+2)>>(16-HSCROLL); // add first bits of next word at the end
          DPEEK(i)=new_value;
        }
      }
#else
#if !defined(SSE_VAR_RESIZE) // useless line
      shifter_pixel=HSCROLL; //start by drawing this pixel
#endif
#endif
      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
      {

#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY3)
        if(!COLOUR_MONITOR) 
#endif
        {
        ASSERT(screen_res==2);
        if(border & 1)
          ///////////////// RENDER VIDEO /////////////////
#if defined(SSE_SHIFTER_HIRES_OVERSCAN)
// experimental, right off, left off?
          if((CurrentScanline.Tricks&2))
            draw_scanline((BORDER_SIDE*2)/16-16, 640/16+16*2, (BORDER_SIDE*2)/16-16,0);
            //draw_scanline((BORDER_SIDE*2)/16, 640/16+16*2, (BORDER_SIDE*2)/16-16*2,0);
          else
#endif
          draw_scanline((BORDER_SIDE*2)/16, 640/16, (BORDER_SIDE*2)/16,0);
        else
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline(0,640/16,0,0);
        }
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }
#if defined(SSE_SHIFTER_STE_HI_HSCROLL) 
      if(
#if defined(SSE_SHIFTER_HIRES_OVERSCAN)
        draw_line_off ||
#endif        
        HSCROLL) 
      {
        for(int i=0;i<=20;i++)// restore ST memory
          LPEEK(i*4+shifter_draw_pointer)=Scanline[i]; 
      }
#endif
      shifter_draw_pointer=nsdp;
    }
    else if(scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border)
    {
      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
      {

#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY3)
        if(!COLOUR_MONITOR)
#endif
        {
        ASSERT(screen_res==2);
        if(border & 1)
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline((BORDER_SIDE*2+640+BORDER_SIDE*2)/16,0,0,0); // rasters!
        else
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline(640/16,0,0,0);
        }
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }
    }
    scanline_drawn_so_far=BORDER_SIDE+320+BORDER_SIDE; //border1+picture+border2;
  }//end Monochrome
}


void TShifter::EndHBL() {

#if defined(SSE_SHIFTER_END_OF_LINE_CORRECTION)
/*  Finish horizontal overscan : correct -2 & +2 effects
    Those tests are much like EndHBL in Hatari
    Check Shifter stability (preliminary)
*/

  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_2) && CurrentScanline.EndCycle==372)     
  {
//TRACE_OSD("no +2");
    CurrentScanline.Tricks&=~TRICK_LINE_PLUS_2;
    shifter_draw_pointer-=2; // eg SNYD/TCB at scan_y -29
#if SSE_VERSION>=370
    CurrentScanline.Bytes-=2;
#endif
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK1 & TRACE_CONTROL_ADJUSTMENT)
#endif
      TRACE_LOG("F%d y %d cancel +2\n",FRAME,scan_y);
  } 

  // no 'else', they're false alerts!
  if(CurrentScanline.Tricks&TRICK_LINE_MINUS_2     
    && (CurrentScanline.StartCycle==52 || CurrentScanline.EndCycle!=372))
  {
    CurrentScanline.Tricks&=~TRICK_LINE_MINUS_2;
    shifter_draw_pointer+=2;
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK1 & TRACE_CONTROL_ADJUSTMENT)
#endif
      TRACE_LOG("F%d y %d cancel -2\n",FRAME,scan_y);
  }
#endif//#if defined(SSE_SHIFTER_END_OF_LINE_CORRECTION)

#if defined(SSE_SHIFTER_DRAGON1) && defined(SSE_STF) && defined(SSE_MMU_WAKE_UP)
  // not defined in v3.5.2
  if(SSE_HACKS_ON && ST_TYPE==STF && WAKE_UP_STATE==2 
    && CurrentScanline.Tricks==1) 
    SS_signal=SS_SIGNAL_SHIFTER_CONFUSED_1; // stage 1 of our hack...
  else if(CurrentScanline.Tricks&&CurrentScanline.Tricks!=0x10
    && (CurrentScanline.Tricks!=TRICK_CONFUSED_SHIFTER))
    SS_signal=0;
#endif

#if defined(SSE_SHIFTER_UNSTABLE)
/*  3.5.2 This way is more generic. It tries to make sense of #words loaded
    in the Shifter after the "unstable" trick.
    Left off: 160+26=186 = (23*8)+2 -> 1 word preloaded
    Left off, right off: 160+26+44=(28*8)+6 -> 3 words preloaded
    See doc by ST-CNX and LJBK's efforts at AF
    3.5.3
    In which WU state it should work isn't clear. 
    Unfinished business. What with the last 'left off' in Omega?
    Would there be a definitive impact on SDP?
    And why doesn't the nice theory for Dragon work for the main menu, where
    a 'left off' by itself following a stabilised 230byte line seems to cause
    no shift (incorrect display with WU)? We "hacked" it for now.
    3.6.0
    In current version those effects work better with WU2.
    To support both Overdrive/Dragon and Overdrive/Menu, we must compare
    the cycle when stabiliser is triggered!
    Preload is 1 for both 186 and 230 byte lines, the right off doesn't
    count. This simplifies code.

menu, wrong
003 - 000:A0002 000:a9CE0 004:R0002 012:R0000 376:S0000 384:S0002 448:R0002 460:R0000 512:T2011 512:#0230
004 - 000:A0002 000:a9DC6 004:R0002 012:R0000 512:T0001 512:#0186 512:L0001

dragon, right
234 - 000:A0006 000:aD9F2 004:R0002 012:R0000 376:S0000 384:S0002 444:R0002 456:R0000 512:T2011 512:#0230
235 - 000:A0006 000:aDAD8 004:R0002 012:R0000 512:T0001 512:#0186 512:L0001    
*/

#if SSE_VERSION<354

  if(WAKE_UP_STATE && ST_TYPE==STF)
  {
    // Overdrive/Dragon
    if((CurrentScanline.Tricks&0xFF)==TRICK_LINE_PLUS_26
      &&!(CurrentScanline.Tricks&TRICK_STABILISER)
      &&(PreviousScanline.Tricks&TRICK_STABILISER)
      &&(ShiftModeChangeAtCycle(444-512)==2 ) // hack to target on Dragon
      )
      Preload=1;
    // Death of the Left Border, Omega Full Overscan
    else if( (CurrentScanline.Tricks&0xFF)
      ==(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_44)
      &&!(CurrentScanline.Tricks&TRICK_STABILISER)) 
      Preload=3; // becomes 2 at first left off
    else if(CurrentScanline.Cycles==508 && FetchingLine()
      && FreqAtCycle(0)==60 && FreqAtCycle(464)==60)
      Preload=0; // a full 60hz scanline should reset the shifter

#if defined(SS_SHIFTER_PANIC)
    if(WAKE_UP_STATE==3 && (CurrentScanline.Tricks&TRICK_UNSTABLE))
    {
      // restore ST memory
      int shift_sdp=Scanline[230/4+1]; //as stored
      for(int i=0;i<=224;i+=16)
      {
        LPEEK(i+shift_sdp)=Scanline[i/4];
        LPEEK(i+shift_sdp+4)=Scanline[i/4+1];
      }
    }
#endif

#if defined(SS_DEBUG)
    if(Preload)
      TRACE_OSD("Preload %d\n",Preload);
#endif

  }

#else
  if(WAKE_UP_STATE)/// && ST_TYPE==STF)
  {
    
    if(MMU.WU[WAKE_UP_STATE]==2 
      && (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      &&!(CurrentScanline.Tricks&TRICK_STABILISER)
//      &&!((PreviousScanline.Tricks&TRICK_STABILISER)&&PreviousScanline.Bytes==CurrentScanline.Bytes)
      &&!((PreviousScanline.Tricks&TRICK_STABILISER)&&ShiftModeChangeAtCycle(448-512)==2)
      &&!(CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
      )
      Preload=1;
    else if(CurrentScanline.Cycles==508 && Preload && FetchingLine()
      && FreqAtCycle(0)==60 && FreqAtCycle(464)==60)
      Preload=0; // a full 60hz scanline should reset the Shifter
    
#if defined(SSE_SHIFTER_PANIC)
    if(WAKE_UP_STATE==WU_SHIFTER_PANIC 
      && (CurrentScanline.Tricks&TRICK_UNSTABLE))
    {
      // restore ST memory
      int shift_sdp=Scanline[230/4+1]; //as stored
#if defined(SSE_SHIFTER_PANIC2)
      for(int i=0+8-8;i<=224-8-8;i+=16)
#else
      for(int i=0+8;i<=224-8;i+=16)
#endif
      {
        LPEEK(i+shift_sdp)=Scanline[i/4];
        LPEEK(i+shift_sdp+4)=Scanline[i/4+1];
      }
    }
#endif

#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL)
    if((OSD_MASK2 & OSD_CONTROL_PRELOAD) && Preload)
      TRACE_OSD("Preload %d\n",Preload);
#endif

  }
#endif//ver

#endif

}


void TShifter::IncScanline() { // a big extension of 'scan_y++'!

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

#if defined(SSE_BOILER_FRAME_REPORT_MASK)
  if((FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_SHIFTER_TRICKS) 
    && CurrentScanline.Tricks)
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'T',CurrentScanline.Tricks);
  if((FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_SHIFTER_TRICKS_BYTES)
    && CurrentScanline.Tricks)
    FrameEvents.Add(scan_y,CurrentScanline.Cycles,'#',CurrentScanline.Bytes);
#endif

#endif

#if defined(SSE_BOILER_TRACE_CONTROL)
/*  It works but the value has to be validated at least once by clicking
    'Go'. If not it defaults to 0. That way we don't duplicate code.
*/
  if( (TRACE_MASK1 & TRACE_CONTROL_1LINE) 
    && TRACE_ENABLED
    && scan_y==debug_run_until_val)
  {
    REPORT_LINE;
    TRACE_OSD("y %d %X %d",scan_y,CurrentScanline.Tricks,CurrentScanline.Bytes);
  }
#endif

#if defined(SSE_TIMINGS_FRAME_ADJUSTMENT)
  // this reduces frame cycles, which isn't foreseen in Steem
  if(CurrentScanline.Cycles==508 && shifter_freq_at_start_of_vbl==50)
  {
    event_plan_50hz[314
#if defined(SSE_INT_VBI_START)
      +1 //TODO check if correct... //+ make a define... 
#endif
].time-=4;
    n508lines++; //that or adjust all plan?
    TRACE_LOG("F%d y%d: frame cycles -4 (correction#%d)\n",FRAME,scan_y,n508lines);
    
  }
#endif


  scan_y++; 
  HblPixelShift=0;  
  left_border=BORDER_SIDE;
  if(HSCROLL) 
    left_border+=16;
  if(shifter_hscroll_extra_fetch) 
    left_border-=16;
  right_border=BORDER_SIDE;

#if defined(SSE_DEBUG) && !defined(SSE_SHIFTER_DRAW_DBG)
  if( scan_y-1>=shifter_first_draw_line && scan_y+1<shifter_last_draw_line
    && (overscan_add_extra || !ExtraAdded) && screen_res<2)
    TRACE_LOG("F%d y%d Extra %d added %d\n",FRAME,scan_y-1,overscan_add_extra,ExtraAdded);
#endif
//        AddExtraToShifterDrawPointerAtEndOfLine(shifter_draw_pointer); //?
  ExtraAdded=overscan_add_extra=0;
#if defined(SSE_SHIFTER_SDP_WRITE)
  SDPMiddleByte=999; // an impossible value for a byte
#endif
//  ASSERT(scan_y!=-28);
  PreviousScanline=CurrentScanline; // auto-generated
  CurrentScanline=NextScanline;

  ASSERT( CurrentScanline.Tricks==NextScanline.Tricks );

#if defined(SSE_MMU_SDP2)//refactor, also foresee read SDP in HIRES
  if(FetchingLine()
    || scan_y==-29 && (PreviousScanline.Tricks&TRICK_TOP_OVERSCAN)
    || !scan_y && !PreviousScanline.Bytes)
    CurrentScanline.Bytes=(screen_res==2)?80:160;
#else
  if(scan_y==-29 && (PreviousScanline.Tricks&TRICK_TOP_OVERSCAN)
    || !scan_y && !PreviousScanline.Bytes)
    CurrentScanline.Bytes=160; // needed by ReadSDP - not perfect (TODO)
  else if(FetchingLine()) 
    CurrentScanline.Bytes=(screen_res==2)?80:160;
#endif
  else //if(!FetchingLine()) 
  {
    ASSERT( !FetchingLine() );
    
    NextScanline.Bytes=0;
  }
  if(shifter_freq==50)
  {
    CurrentScanline.StartCycle=56;
    CurrentScanline.EndCycle=376;
    CurrentScanline.Cycles=SCANLINE_TIME_IN_CPU_CYCLES_50HZ;
  }
  else if(shifter_freq==60)
  {
    CurrentScanline.StartCycle=52;
    CurrentScanline.EndCycle=372;
    CurrentScanline.Cycles=SCANLINE_TIME_IN_CPU_CYCLES_60HZ;
  }
  else if(shifter_freq==72 || screen_res==2)
  {
    CurrentScanline.StartCycle=0;
    CurrentScanline.EndCycle=160;
    CurrentScanline.Cycles=SCANLINE_TIME_IN_CPU_CYCLES_70HZ;
#if defined(SSE_SHIFTER_STE_HI_HSCROLL)
    shifter_draw_pointer+=LINEWID*2; // 'AddExtra' wasn't used
#if SSE_VERSION>=370 // 'AddExtra' wasn't used
    if(shifter_skip_raster_for_hscroll) 
      shifter_draw_pointer+=2; // + 1 raster (the last prefetch)
#endif
#endif
  }
  TrickExecuted=0;
  NextScanline.Tricks=0; // eg for 0byte lines mess

  // In the STE if you make hscroll non-zero in the normal way then the Shifter
  // buffers 2 rasters ahead. We don't do this so to make sdp correct at the
  // end of the line we must add a raster.  
  //SS: in fact the last prefetch of the line
  shifter_skip_raster_for_hscroll = (HSCROLL!=0);
}

/*  For simplification, GLUE/MMU/Shifter IO is considered just Shifter for what
    concerns the ST video circuits. TODO
    We moved the parts of ior.cpp and iow.cpp dealing with video circuits here.
*/

BYTE TShifter::IORead(MEM_ADDRESS addr) {

  ASSERT( (addr&0xFFFF00)==0xff8200 );

/*
    This was in Steem 3.2:
    // Below $10 - Odd bytes return value or 0, even bytes return 0xfe/0x7e
    // Above $40 - Unused return 0
    //// Unused bytes between $60 and $80 should return 0!
   For now, we keep that for STE (refactored), return $FF for STF.
   ior_byte is our return value, to which we immediately give a default
   value.
   Cases 
   R FF820D Lemmings40
*/
  BYTE ior_byte= ((addr&1)||addr>0xff8240) ? 0 : 0xFE; 
#if defined(SSE_STF_SHIFTER_IOR)
  if(ST_TYPE!=STE)
    ior_byte=0xFF; 
#endif  

  if (addr>=0xff8240 && addr<0xff8260){  //palette
    int n=(addr-0xff8240)/2; // which palette
    ior_byte= (addr&1) ? (STpal[n]&0xFF) : (STpal[n]>>8);
  }else if (addr>0xff820f && addr<0xff8240){ //forbidden gap
    exception(BOMBS_BUS_ERROR,EA_READ,addr);
  }else if (addr>0xff827f){  //forbidden area after SHIFTER
    exception(BOMBS_BUS_ERROR,EA_READ,addr);
  }else{
    switch(addr){
      
    case 0xff8201:  //high byte of screen memory address
      ior_byte=LOBYTE(HIWORD(xbios2));
      break;
      
    case 0xff8203:  //mid byte of screen memory address
      ior_byte=HIBYTE(LOWORD(xbios2));
      break;
      
    case 0xff8205:  //high byte of screen draw pointer
    case 0xff8207:  //mid byte of screen draw pointer
    case 0xff8209:{  //low byte of screen draw pointer
      MEM_ADDRESS sdp;
#if defined(SSE_SHIFTER_SDP_READ)
      sdp=ReadSDP(LINECYCLES); // a complicated affair
#else
      if(scan_y<shifter_first_draw_line || scan_y>=shifter_last_draw_line){
        sdp=shifter_draw_pointer;
      }else{
        sdp=get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
        LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+
          " - Read Shifter draw pointer as $"+HEXSl(sdp,6)+
          " on "+scanline_cycle_log()); )
      }
#endif
      ior_byte=DWORD_B(&sdp,(2-(addr-0xff8205)/2)); // change for big endian !!!!!!!!!
#if defined(SSE_DEBUG_FRAME_REPORT_READ_SDP) // c for counter now
      FrameEvents.Add(scan_y,LINECYCLES,'c',((addr&0xF)<<8)|ior_byte); 
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SDP_READ)
        FrameEvents.Add(scan_y,LINECYCLES,'c',((addr&0xF)<<8)|ior_byte);
#endif
      }
      break;
      
    case 0xff820a:  //synchronization mode
      ior_byte&=~3;           // this way takes care
      ior_byte|=m_SyncMode;   // of both STF & STE
      break;

    case 0xff820d:  //low byte of screen memory address
      ASSERT(!(xbios2&1));
#if defined(SSE_STF_VBASELO)
      ASSERT( ST_TYPE==STE || !(xbios2&0xFF) );
      if(ST_TYPE==STE) 
#endif
        ior_byte=(BYTE)xbios2&0xFF;
      break;
  
    case 0xff820f: // LINEWID
#if defined(SSE_STF_LINEWID)
      //ASSERT( ST_TYPE==STE ); //No Cooper, Fuzion 77, 78
      if(ST_TYPE==STE) 
#endif
        ior_byte=LINEWID;
      break;
      
    case 0xff8260: //resolution
      ior_byte&=~3;           // this way takes care
      ior_byte|=m_ShiftMode;  // of both STF & STE
      break;

    case 0xff8265:  //HSCROLL
      DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) shifter_hscroll_extra_fetch=(shifter_hscroll!=0);
#if defined(SSE_STF_HSCROLL)
      if(ST_TYPE==STE)
#endif
        ior_byte=HSCROLL;
      break;
    }//if
    
  }
#if defined(SSE_SHIFTER_IOR_TRACE)
  // made possible by our structure change
  TRACE("Shifter read %X=%X\n",addr,ior_byte); // not LOG
#endif
  return ior_byte;
}


void TShifter::IOWrite(MEM_ADDRESS addr,BYTE io_src_b) {

  ASSERT( (addr&0xFFFF00)==0xff8200 );

#if defined(SSE_SHIFTER_IOW_TRACE)  // not LOG
  TRACE("(%d %d/%d) Shifter write %X=%X\n",FRAME,scan_y,LINECYCLES,addr,io_src_b);
#endif  

  if ((addr>=0xff8210 && addr<0xff8240) || addr>=0xff8280){
    exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
  }
  
  /////////////
  // Palette // // SS word (long) writes far more frequent (see below)
  /////////////
  
  else if (addr>=0xff8240 && addr<0xff8260){  //palette
    int n=(addr-0xff8240) >> 1; 
    
    // Writing byte to palette writes that byte to both the low and high byte!
    WORD new_pal=MAKEWORD(io_src_b,io_src_b & 0xf);

#if defined(SSE_DEBUG_FRAME_REPORT_PAL)
      FrameEvents.Add(scan_y,LINECYCLES,'p', (n<<12)|io_src_b);  // little p
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_PAL)
        FrameEvents.Add(scan_y,LINECYCLES,'p', (n<<12)|io_src_b);  // little p
#endif
    
#if defined(SSE_SHIFTER_PALETTE_BYTE_CHANGE) 
    // TESTING maybe Steem was right, Hatari is wrong
    // v3.6.3 apparently Steem was right, undef
    if(addr&1) // the double write happens only on even addresses (?)
    {
      new_pal=(STpal[n]&0xFF00)|io_src_b; // fixes Golden Soundtracker demo
      TRACE_LOG("Single byte  %X write pal %X STPal[%d] %X->%X\n",io_src_b,addr,n,STpal[n],new_pal);
    }
#endif
    
    SetPal(n,new_pal);
    log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Palette change at scan_y="+scan_y+" cycle "+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl));
    
    
  }
  else{
    switch(addr){
      
/*
Video Base Address:                           ST     STE

$FFFF8201    0 0 X X X X X X   High Byte      yes    yes
$FFFF8203    X X X X X X X X   Mid Byte       yes    yes
$FFFF820D    X X X X X X X 0   Low Byte       no     yes

These registers only contain the "reset" value for the Shifter after a 
whole screen has been drawn. It does not affect the current screen, but 
the one for the next VBL. To make immediate changes on the screen, use 
the Video Address Counter [(SDP)].

According to ST-CNX, those registers are in the MMU, not in the Shifter.

 STE doc by Paranoid: for compatibility reasons, the low-byte
 of the Video Base Address is ALWAYS set to 0 when the mid- or high-byte of
 the Video Base Address are set. 
 E.g.: Leavin' Teramis

*/
     
    case 0xff8201:  //high byte of screen memory address
#if defined(SSE_DEBUG_FRAME_REPORT_VIDEOBASE)
      FrameEvents.Add(scan_y,LINECYCLES,'V',io_src_b); 
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
        FrameEvents.Add(scan_y,LINECYCLES,'V',io_src_b); 
#endif

      // asserts on SoWatt, Leavin' Teramis, High Fidelity Dreams
      // ...
      //ASSERT( mem_len>FOUR_MEGS || !(io_src_b&(~b00111111)) ); 
      if (mem_len<=FOUR_MEGS) 
        io_src_b&=b00111111;
      DWORD_B_2(&xbios2)=io_src_b;
#if defined(SSE_STF_VBASELO)
      if(ST_TYPE==STE) 
#endif
        DWORD_B_0(&xbios2)=0; 
      log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
      break;
      
    case 0xff8203:  //mid byte of screen memory address
#if defined(SSE_DEBUG_FRAME_REPORT_VIDEOBASE)
      FrameEvents.Add(scan_y,LINECYCLES,'M',io_src_b); 
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
        FrameEvents.Add(scan_y,LINECYCLES,'M',io_src_b); 
#endif

      DWORD_B_1(&xbios2)=io_src_b;

#if defined(SSE_STF_VBASELO)
      if(ST_TYPE==STE) 
#endif
        DWORD_B_0(&xbios2)=0; 
      log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
      break;
      
    case 0xff8205:  //high byte of draw pointer
    case 0xff8207:  //mid byte of draw pointer
    case 0xff8209:  //low byte of draw pointer
      
#if defined(SSE_SHIFTER_SDP_WRITE)
      WriteSDP(addr,io_src_b); // very complicated!
      break;
      
#else // Steem 3.2 or SSE_SHIFTER_SDP_WRITE not defined
      {
        //          int srp=scanline_raster_position();
        int dst=ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl;
        dst-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
        dst+=16;dst&=-16;
        dst+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
#if defined(SSE_SHIFTER) // video defined but not SDP
        Render(dst);
#else
        draw_scanline_to(dst); // This makes shifter_draw_pointer up to date
#endif
        MEM_ADDRESS nsdp=shifter_draw_pointer;
        if (mem_len<=FOUR_MEGS && addr==0xff8205) io_src_b&=b00111111;
        DWORD_B(&nsdp,(0xff8209-addr)/2)=io_src_b;
        
        /*
        if (shifter_hscroll){
        if (dst>=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-32 && dst<CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320-16){
        log_to(LOGSECTION_VIDEO,Str("ATTANT: addr=")+HEXSl(addr,6));
        if (addr==0xff8209){
        // If you set low byte while on screen with hscroll on then sdp will
        // be an extra raster ahead. Steem's sdp is always 1 raster ahead, so
        // correct for that here.
        nsdp-=8;
        }
        }
        }
        */
        
        //          int off=(get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)&-8)-shifter_draw_pointer;
        //          shifter_draw_pointer=nsdp-off;
        shifter_draw_pointer_at_start_of_line-=shifter_draw_pointer;
        shifter_draw_pointer_at_start_of_line+=nsdp;
        shifter_draw_pointer=nsdp;
        
        log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+" - Set Shifter draw pointer to "+
          HEXSl(shifter_draw_pointer,6)+" at "+scanline_cycle_log()+", aligned to "+dst);
        break;
      }
#endif

    case 0xff820a: //synchronization mode      
      SetSyncMode(io_src_b); 
      break;
      
/* 
VBASELO 
This register contains the low-order byte of the video display base address. 
It can be altered at any time and will affect the next display processor data 
fetch. it is recommended that the video display address be altered only during 
vertical and horizontal blanking or display garbage may result. 

STE only. That's why the low byte is separated from the high & mid bytes.
Writing on it on a STF does nothing.

Last bit always cleared (we must do it).
*/
    case 0xff820d:  //low byte of screen memory address
#if defined(SSE_DEBUG_FRAME_REPORT_VIDEOBASE)
      FrameEvents.Add(scan_y,LINECYCLES,'v',io_src_b); 
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
        FrameEvents.Add(scan_y,LINECYCLES,'v',io_src_b); 
#endif

#if defined(SSE_STF_VBASELO)
      if(ST_TYPE!=STE)
      {
        TRACE_LOG("STF write %X to %X\n",io_src_b,addr);
        break; // fixes Lemmings 40; used by Beyond/Pax Plax Parallax
      }
#endif
      io_src_b&=~1;
      xbios2&=0xFFFF00;
      xbios2|=io_src_b;
      log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
      break;
      
/*
LINEWID This register indicates the number of extra words of data (beyond
that required by an ordinary ST at the same resolution) which represent
a single display line. If it is zero, this is the same as an ordinary ST. 
If it is nonzero, that many additional words of data will constitute a
single video line (thus allowing virtual screens wider than the displayed 
screen). CAUTION In fact, this register contains the word offset which 
the display processor will add to the video display address to point to 
the next line. If you are actively scrolling (HSCROLL <> 0), this register
should contain The additional width of a display line minus one data fetch
(in low resolution one data fetch would be four words, one word for 
monochrome, etc.).

Line-Offset Register
  
FFFF820F  X X X X X X X X X                  no     yes
    
This register contains the value how many WORDS (not BYTES!) the Shifter is
supposed to skip after each Rasterline. This register enables virtual screens
that are (horizontally) larger than the displayed screen by making the 
Shifter skip the set number of words when a line has been drawn on screen.    

The Line Offset Register is very critical. Make sure it contains the correct
value at any time. If the Pixel Offset Register contains a zero, the Line 
Offset Register contains the exact number of words to skip after each line. 
But if you set the Pixel Offset Register to "X", the Shifter will still 
display 320 (640) pixels a line and therefore has to read "X" pixels from
the NEXT word which it would have skipped if the Pixel offset Register 
contained a "0". Hence, for any Pixel Offset Value > 0, please note that 
the Shifter has to read (a few bits) more each rasterline and these bits 
must NOT be skipped using the Line Offset Register. 
*/
    case 0xff820f:   
#if defined(SSE_DEBUG_FRAME_REPORT_HSCROLL)
      FrameEvents.Add(scan_y,LINECYCLES,'L',io_src_b); //we choose L now
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_HSCROLL)
        FrameEvents.Add(scan_y,LINECYCLES,'L',io_src_b); 
#endif

#if defined(SSE_STF_LINEWID)
      if(ST_TYPE!=STE)
      {
        TRACE_LOG("STF write %X to %X\n",io_src_b,addr);
        break; // fixes Imagination/Roller Coaster mush
      }
#endif
      
#if defined(SSE_SHIFTER) 
      Render(LINECYCLES,DISPATCHER_LINEWIDTH); // eg Beat Demo
#else
      draw_scanline_to(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl); // Update sdp if off right  
#endif
      
#if defined(SSE_SHIFTER_SDP_TRACE_LOG)
      TRACE_LOG("F%d y%d c%d LW %d -> %d\n",FRAME,scan_y,LINECYCLES,LINEWID,io_src_b);
#endif
      LINEWID=io_src_b;
      log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set shifter_fetch_extra_words to "+
        (shifter_fetch_extra_words)+" at "+scanline_cycle_log());
      break;

    case 0xff8260: //resolution
      SetShiftMode(io_src_b);
      break;
      
/*
HSCROLL 
This register contains the pixel scroll offset. 
If it is zero, this is the same as an ordinary ST. 
If it is nonzero, it indicates which data bits constitute the first pixel 
from the first word of data. 
That is, the leftmost displayed pixel is selected from the first data word(s)
of a given line by this register.


FF8264 ---- ---- ---- xxxx

[Notice how writes to FF8264 should be ignored. They aren't.]

Horizontal Bitwise Scroll.
Delays the start of screen by the specified number of bits. 

Video Base Address Pixel Offset              STF     STE

$FFFF8265  0 0 0 0 X X X X                   no      yes

This register allows to skip from a single to 15 pixels at the start of each
rasterline to allow horizontal fine-scrolling. 
*/
      
    case 0xff8264:   
      // Set hscroll and don't change line length
      // This is an odd register, when you change hscroll below to non-zero each
      // scanline becomes 4 words longer to allow for extra screen data. This register
      // sets hscroll but doesn't do that, instead the left border is increased by
      // 16 pixels. If you have got hscroll extra fetch turned on then setting this
      // to 0 confuses the Shifter and causes it to shrink the left border by 16 pixels.
    case 0xff8265:  // Hscroll
#if defined(SSE_DEBUG_FRAME_REPORT_HSCROLL)
      FrameEvents.Add(scan_y,LINECYCLES,(addr==0xff8264)?'h':'H',io_src_b); 
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_HSCROLL)
        FrameEvents.Add(scan_y,LINECYCLES,(addr==0xff8264)?'h':'H',io_src_b); 
#endif
#if defined(SSE_STF_HSCROLL)
      if(ST_TYPE!=STE) 
      {
//        TRACE_LOG("STF write %X to %X\n",io_src_b,addr); //ST-CNX
        break; // fixes Hyperforce
      }
      else
#endif
      {
        int cycles_in=(int)(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
        
#if defined(SSE_SHIFTER_SDP_TRACE_LOG)
        TRACE_LOG("F%d y%d c%d HS %d -> %d\n",FRAME,scan_y,LINECYCLES,HSCROLL,io_src_b);
#endif
        
        HSCROLL=io_src_b & 0xf; // limited to 4bits

        log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set horizontal scroll ("+HEXSl(addr,6)+
          ") to "+(shifter_hscroll)+" at "+scanline_cycle_log());
        if (addr==0xff8265) shifter_hscroll_extra_fetch=(HSCROLL!=0);
        
        
#if defined(SSE_VID_BORDERS)
        if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-BORDER_SIDE){
#else
        if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-32){
#endif           // eg Coreflakes hidden screen
          if (left_border>0){ // Don't do this if left border removed!
            shifter_skip_raster_for_hscroll = (HSCROLL!=0); //SS computed at end of line anyway
////            if(HSCROLL) TRACE("%d %d %d write skip\n",TIMING_INFO);
            left_border=BORDER_SIDE;
            if (HSCROLL)
              left_border+=16;
            if(shifter_hscroll_extra_fetch) 
              left_border-=16;
          }
        }
      } 
      break;
        
    }//sw
  }//if     
}


void TShifter::Render(int cycles_since_hbl,int dispatcher) {
  // this is based on Steem's 'draw_scanline_to'
#if defined(SSE_SHIFTER_DRAW_DBG)
  draw_scanline_to(cycles_since_hbl); // base function
  return;
#endif

  if(screen_res>=2) 
    return; 

#ifndef NO_CRAZY_MONITOR
  if(extended_monitor || emudetect_falcon_mode!=EMUD_FALC_MODE_OFF) 
    return;
#endif

  if(freq_change_this_scanline
#if defined(SSE_SHIFTER_DRAGON1)//temp
    || SS_signal==SS_SIGNAL_SHIFTER_CONFUSED_2
#endif
#if defined(SSE_SHIFTER_UNSTABLE)
    || Preload // must go apply trick at each scanline
#endif
    )
    CheckSideOverscan(); 

/*  What happens here is very confusing; we render in real time, but not
    quite. As on a real ST, there's a delay of fetch+8 between the 
    'Shifter cycles' and the 'palette reading' cycles. 
    We can't render at once because the palette could change. 
    CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN (84) takes care of that delay. 
    This causes all sort of trouble because our SDP is late while rendering,
    and sometimes forward!
*/

#if defined(SHIFTER_PREFETCH_LATENCY)
  // this may look impressive but it's just a bunch of hacks!
  switch(dispatcher) {
  case DISPATCHER_CPU:
//    TRACE_OSD("TRACK SDP"); // too many false alerts
    cycles_since_hbl+=16; // 3615 Gen4 by ULM, override normal delay
    break;
/*
  case DISPATCHER_DSTE:
    break;
*/
  case DISPATCHER_SET_PAL:
#if defined(SSE_SHIFTER_PALETTE_TIMING)
#if defined(SSE_MMU_WU_PALETTE_STE)
    if(!(
#if defined(SSE_STF)
      ST_TYPE==STE &&
#endif
      WAKE_UP_STATE==1))
#endif
      cycles_since_hbl++; // eg Overscan Demos #6, already in v3.2 TODO why?
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
/*  We must compensate the "no shifter_pixel+4" of "left off" to get correct
    palette timings. This is a hack but we must manage various sizes.
    OK: Overscan #6, HighResMode STE
    bugfix v3.6.0: for STE line +20 there's nothing to compensate! (pcsv62im)
*/
      if(SSE_HACKS_ON && SideBorderSize==VERY_LARGE_BORDER_SIDE  
#if defined(SSE_VID_BORDERS_416_NO_SHIFT1)
        && border
#endif
        && shifter_freq_at_start_of_vbl==50
        && (CurrentScanline.Tricks&TRICK_LINE_PLUS_26))
        cycles_since_hbl+=4;
#endif
#endif
    break;
  case DISPATCHER_SET_SHIFT_MODE:
    RoundCycles(cycles_since_hbl); // eg Drag/Happy Islands, Cool STE
    break; 
#if defined(SSE_SHIFTER_RENDER_SYNC_CHANGES)//no
  case DISPATCHER_SET_SYNC:
    RoundCycles(cycles_since_hbl); 
    cycles_since_hbl++;
    break;
#endif
  case DISPATCHER_WRITE_SDP:
    RoundCycles(cycles_since_hbl); // eg D4/Tekila
    break;
  }//sw
#endif

  int pixels_in=cycles_since_hbl-(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-BORDER_SIDE);

  if(pixels_in > BORDER_SIDE+320+BORDER_SIDE) 
    pixels_in=BORDER_SIDE+320+BORDER_SIDE;  
  if(pixels_in>=0) // time to render?
  {
#ifdef WIN32 // prepare buffer & ASM routine
    if(draw_buffer_complex_scanlines && draw_lock)
    {
      if(scan_y>=draw_first_scanline_for_border  
#if defined(SSE_VID_BORDERS_BIGTOP) // avoid horrible crash
          && (DISPLAY_SIZE<BIGGEST_DISPLAY || scan_y>=draw_first_scanline_for_border+ 
               (BIG_BORDER_TOP-ORIGINAL_BORDER_TOP))
#endif
        && scan_y<draw_last_scanline_for_border)
      {
        if(draw_store_dest_ad==NULL 
          && pixels_in<BORDER_SIDE+320+BORDER_SIDE
          )
        {
          draw_store_dest_ad=draw_dest_ad;
          ScanlineBuffer=draw_dest_ad=draw_temp_line_buf;
          draw_store_draw_scanline=draw_scanline;
        }
        if(draw_store_dest_ad) 
          draw_scanline=draw_scanline_1_line[screen_res];
      }
    }
#endif
    if(FetchingLine())
    {
      ASSERT( left_border>=0 );
/*
      if(left_border<0) // shouldn't happen
      {
        BRK( LB<0 leaving render ) ; //test if should remove...
        TRACE_LOG("LB<0, leaving render\n");
        return; // may avoid crash
      }
*/
      int border1=0,border2=0,picture=0,hscroll=0; // parameters for ASM routine
      int picture_left_edge=left_border; // 0, BS, BS-4, BS-16 (...)
      //last pixel from extreme left to draw of picture
      int picture_right_edge=BORDER_SIDE+320+BORDER_SIDE-right_border;

      if(pixels_in>picture_left_edge)
      { //might be some picture to draw = fetching RAM
        if(scanline_drawn_so_far>picture_left_edge)
        {
          picture=pixels_in-scanline_drawn_so_far;
          if(picture>picture_right_edge-scanline_drawn_so_far)
            picture=picture_right_edge-scanline_drawn_so_far;
        }
        else
        {
          picture=pixels_in-picture_left_edge;
          if(picture>picture_right_edge-picture_left_edge)
            picture=picture_right_edge-picture_left_edge;
        }
        if(picture<0)
          picture=0;
      }
      if(scanline_drawn_so_far<left_border)
      {

        if(pixels_in>left_border)
        {
          border1=left_border-scanline_drawn_so_far;
#if defined(SSE_VID_BORDERS_416_NO_SHIFT)
/*  In very large display mode, we display 52 pixels when the left
    border is removed. But Steem was built around borders of 32
    pixels, which were extended to 40, 48, not 52. And borders have
    the same size on left and right.
    To make up for larger left border, we shift pixels by 4 to the 
    right when the left border isn't removed!
    We do so to avoid far more intricate changes.
    This is a simple hack that surprisingly works even for:
    Beeshift, Dragonnels menu
*/
          if(SSE_HACKS_ON && SideBorderSize==VERY_LARGE_BORDER_SIDE 
            && left_border
#if defined(SSE_VID_BORDERS_416_NO_SHIFT1)
            && border
#endif
            )
            border1+=4;
#endif
        }
        else
          border1=pixels_in-scanline_drawn_so_far; // we're not yet at end of border

        if(border1<0) 
          border1=0;
      }
      border2=pixels_in-scanline_drawn_so_far-border1-picture;
      if(border2<0) 
        border2=0;

      int old_shifter_pixel=shifter_pixel;
      shifter_pixel+=picture;
      MEM_ADDRESS nsdp=shifter_draw_pointer;

#if defined(SSE_SHIFTER_LINE_MINUS_2_DONT_FETCH)
/*  On lines -2, don't fetch the last 2 bytes as if it was a 160byte line.
    Fixes screen #2 of the venerable B.I.G. Demo.
*/
      if(SSE_HACKS_ON && (CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
        && picture>=16)
      {
        picture-=16,border2+=16; // cancel last raster
        //TRACE_LOG("Line -2 reduce pic to %d\n",picture);
        //overscan_add_extra=0; // no, because shifter_pixel was updated
      }
#endif




      if(!screen_res) // LOW RES
      {
        hscroll=old_shifter_pixel & 15;
        nsdp-=(old_shifter_pixel/16)*8;
        nsdp+=(shifter_pixel/16)*8; // is sdp forward at end of line?
        
#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_SHIFTER_4BIT_SCROLL)
/*  This is where we do the actual shift for those rare programs using the
    4bit hardscroll trick (PYM/ST-CNX,D4/Nightmare,D4/NGC).
    Notice it is quite simple, and also very efficient because it uses 
    the hscroll parameter of the assembly drawing routine (programmed by
    Steem authors, of course).
    hscroll>15 is handled further.
    We use this routine for other shifts as well.
    TODO: be able to shift the line by an arbitrary #pixels left ot right
    (assembly)
*/

        if( (CurrentScanline.Tricks&TRICK_4BIT_SCROLL)
#if defined(SSE_SHIFTER_LINE_PLUS_20_SHIFT)
          || (CurrentScanline.Tricks&TRICK_LINE_PLUS_20)
#endif
#if defined(SSE_SHIFTER_UNSTABLE)
          || (CurrentScanline.Tricks&TRICK_UNSTABLE)
#endif
          )
        {
          hscroll-=HblPixelShift;
          if(hscroll<0)
          {
            if(picture>-hscroll)
            {
              picture+=hscroll,border1-=hscroll;
              hscroll=0;
            }
            else if(!picture) // phew
              hscroll+=HblPixelShift;
          }
        }
#endif

      }
      else if(screen_res==1) // MEDIUM RES
      {
        hscroll=(old_shifter_pixel*2) & 15;

#if defined(SSE_SHIFTER_STE_MED_HSCROLL)
        if(HblStartingHscroll&1) // not useful for anything known yet!
        { 
          hscroll++; // restore precision
          HblStartingHscroll=0; // only once/scanline (?)
        }
#endif

        nsdp-=(old_shifter_pixel/8)*4;
        nsdp+=(shifter_pixel/8)*4;
      }
      if(draw_lock) // draw_lock is set true in draw_begin(), false in draw_end()
      {
        // real lines
        if(scan_y>=draw_first_possible_line
#if defined(SSE_VID_BORDERS_BIGTOP)
          + (DISPLAY_SIZE>BIGGEST_DISPLAY?6:0) 
#endif
          && scan_y<draw_last_possible_line)
        {
          // actually draw it
          if(picture_left_edge<0) 
            picture+=picture_left_edge;
          AUTO_BORDER_ADJUST; // hack borders if necessary
          DEBUG_ONLY( shifter_draw_pointer+=debug_screen_shift; );
          if(hscroll>=16) // convert excess hscroll in SDP shift
          {
#if defined(SSE_DEBUG)
//            if(scan_y==120) TRACE_LOG("y %d hscroll OVL\n",scan_y);
#endif
            shifter_draw_pointer+=(SHIFTER_RASTER_PREFETCH_TIMING/2)
              *(hscroll/16); // ST-CNX large border
            hscroll-=16*(hscroll/16);
          }
          
#if defined(SSE_DEBUG)
#ifdef SSE_SHIFTER_SKIP_SCANLINE
          if(scan_y==SSE_SHIFTER_SKIP_SCANLINE) 
            border1+=picture,picture=0;
#endif
#if defined(SSE_BOILER_VIDEO_CONTROL)
          if((VIDEO_CONTROL_MASK & VIDEO_CONTROL_LINEOFF) 
            && scan_y==debug_run_until_val)
            border1+=picture,picture=0; // just border colour
#endif
          if(border1<0||picture<0||border2<0||hscroll<0||hscroll>15)
          {
            TRACE_LOG("F%d y%d p%d Render error %d %d %d %d\n",
              FRAME,scan_y,pixels_in,border1,picture,border2,hscroll); 
            return;
          }
#endif

          // call to appropriate ASSEMBLER routine!
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline(border1,picture,border2,hscroll); 
        }
      }
      shifter_draw_pointer=nsdp;

      // adjust SDP according to Shifter tricks
      if(!ExtraAdded // only once - kind of silly variable
        && ( dispatcher==DISPATCHER_DSTE || 
         dispatcher!=DISPATCHER_WRITE_SDP  && 
         pixels_in>=picture_right_edge 
        && scanline_drawn_so_far<picture_right_edge
#if defined(SSE_SHIFTER_SDP_WRITE_ADD_EXTRA)
        || dispatcher==DISPATCHER_WRITE_SDP // fixes flicker in Cool STE 
        && cycles_since_hbl>=CurrentScanline.EndCycle+SHIFTER_PREFETCH_LATENCY
#endif
        ) )
        AddExtraToShifterDrawPointerAtEndOfLine(shifter_draw_pointer);
    }
    // overscan lines = a big "left border"
    else if(scan_y>=draw_first_scanline_for_border 
      && scan_y<draw_last_scanline_for_border)
    {
      int border1; // the only var. sent to draw_scanline
      int left_visible_edge,right_visible_edge;
      // Borders on
      if(border & 1)
      {
        left_visible_edge=0;
        right_visible_edge=BORDER_SIDE + 320 + BORDER_SIDE;
      }
      // No, only the part between the borders
      else 
      {
        left_visible_edge=BORDER_SIDE;
        right_visible_edge=320+BORDER_SIDE;
      }
      if(scanline_drawn_so_far<=left_visible_edge)
        border1=pixels_in-left_visible_edge;
      else
        border1=pixels_in-scanline_drawn_so_far;
      if(border1<0)
        border1=0;
      else if(border1> (right_visible_edge - left_visible_edge))
        border1=(right_visible_edge - left_visible_edge);
//      ASSERT( scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line ); // asserts when changing sizes
      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
      {
        //////ASSERT(draw_lock); //!!!!!!!!!!
      ///////////////// RENDER VIDEO /////////////////        
        draw_scanline(border1,0,0,0); // see SSE_VID_ADJUST_DRAWING_ZONE1
      }
    }
    scanline_drawn_so_far=pixels_in;
  }//if(pixels_in>=0)
}


void TShifter::Reset(bool Cold) {
  m_SyncMode=0; 
  m_ShiftMode=screen_res; // we know it's updated first, debug strange screen!
#if defined(SSE_DEBUG)
  nVbl=0; // cold or warm
#endif
#if defined(SSE_SHIFTER_TRICKS)
  for(int i=0;i<32;i++)
  {
    shifter_freq_change[i]=0; // interference Leavin' Teramis/Flood on PP43
    shifter_shift_mode_change_time[i]=-1;
  }
#endif

#if defined(SSE_DEBUG)
  //if(Cold)
    Debug.ShifterTricks=0;
#endif

#if defined(SSE_SHIFTER_UNSTABLE)
//  if(Cold)
    Preload=0;
#endif

}

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


void TShifter::SetShiftMode(BYTE NewMode) {
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

  FF8260 is both in the GLUE and the Shifter. It is needed in the GLUE
  because sync signals are different in mode 2 (70hz).
  It is needed in the Shifter because it needs to know in how many bit planes
  memory has to be decoded, and where it must send the video signal (RGB, 
  Mono).
    
  In monochrome, frequency is 70hz, a line is transmitted in 28�s.
  There are +- 500 scanlines.
  We see no reason to forbid writing 3 without crash, but how it is
  handled by the Shifter is unknown. Awesome 16 should work in STF mode
  anyway.
  Update: we found a case 'Oyster', 3 is treated as 2.
*/

  int CyclesIn=LINECYCLES;

#if defined(SSE_SHIFTER_FIX_LINE508_CONFUSION)
  if( Line508Confusion() )
    CyclesIn-=4; // this is for correct reporting
#endif

#if defined(SSE_DEBUG_FRAME_REPORT_SHIFTMODE)
  FrameEvents.Add(scan_y,CyclesIn,'R',NewMode); 
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SHIFTMODE)
    FrameEvents.Add(scan_y,CyclesIn,'R',NewMode); 
#endif
  NewMode&=3; // only two lines would physically exist
  m_ShiftMode=NewMode; // update, used by ior now (v3.5.1)

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
  // From here, we have a colour display: //not always
  ASSERT( COLOUR_MONITOR );
#endif

  if(NewMode==3) 
#if SSE_VERSION>354
    NewMode=2; // fixes The World is my Oyster screen #2
#else
    NewMode=1; // or 2? TESTING
#endif

#if defined(SSE_SHIFTER_TRICKS) // before rendering
  AddShiftModeChange(NewMode); // add time & mode
#endif

  Render(CyclesIn,DISPATCHER_SET_SHIFT_MODE);

#if defined(SSE_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE) 
  AddFreqChange( (NewMode==2 ? MONO_HZ : shifter_freq) );
#endif

#if defined(SSE_SHIFTER_HIRES_OVERSCAN)

  if(screen_res==2)
  {
    freq_change_this_scanline=true;
    return;
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

}


void TShifter::SetSyncMode(BYTE NewSync) {
/*
    ff 820a   R/W             |------xx|   Sync Mode
                                     ||
                                     | ----   External/_Internal Sync
                                      -----   50 Hz/_60 Hz Field Rate

    Only 1 bit is of interest: bit 1:  1:50 Hz 0:60 Hz
    Normally, 50hz for Europe, 60hz for the USA.
    On the ST, sync mode was abused to create overscan (3 of the 4 borders)
    At 50hz, the ST displays 313 lines every frame, instead of 312.5 like
    in the PAL standard (one frame with 312 lines, one with 313, etc.)    
*/

  int CyclesIn=LINECYCLES;

#if defined(SSE_SHIFTER_FIX_LINE508_CONFUSION)
  if( Line508Confusion() )
    CyclesIn-=4; // this is for correct reporting
#endif

#if defined(SSE_DEBUG_FRAME_REPORT_SYNCMODE)
  FrameEvents.Add(scan_y,CyclesIn,'S',NewSync); 
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SYNCMODE)
    FrameEvents.Add(scan_y,CyclesIn,'S',NewSync); 
#endif
  //TRACE("F%d y%d c%d s%d frame %d\n",TIMING_INFO,NewSync,shifter_freq_at_start_of_vbl);//TEMP
  int new_freq;  

  m_SyncMode=NewSync&3;

#if defined(SSE_SHIFTER_RENDER_SYNC_CHANGES)
  Render(CyclesIn,DISPATCHER_SET_SYNC);
#endif
  if(screen_res>=2) // note this part is not extra-reliable yet
  {
    new_freq=MONO_HZ; //71
    shifter_freq_idx=2;
    CurrentScanline.Cycles=scanline_time_in_cpu_cycles[shifter_freq_idx]; 
  }
  else if(NewSync&2) // freq 50hz
  {
    new_freq=50;
    shifter_freq_idx=0;
/*  This isn't exact science, but the values are compatible with Forest
    (or we could have wrong 508 cycle lines messing the internal timing for
    "blank line" ('28' taken for '32').
*/
    const int limit512=
#if defined(SSE_STF) && defined (SSE_MMU_WAKE_UP)
    (
#if !defined(SSE_MMU_WU_STE)
      ST_TYPE!=STE && 
#endif
#if SSE_VERSION<360
      WAKE_UP_STATE==2) ? 56+2 :
#else
      MMU.WU[WAKE_UP_STATE]==2) ? 56+2 :  // bugfix v3.6.0
#endif
#endif
      54+2 ;

    if(CyclesIn<=limit512)
      CurrentScanline.Cycles=scanline_time_in_cpu_cycles[shifter_freq_idx];

    if(FetchingLine())
    {
      if(CyclesIn<=52 && left_border)
        CurrentScanline.StartCycle=56; 

      if(CyclesIn<=372)
        CurrentScanline.EndCycle=376;

#if defined(SSE_SHIFTER_LEFT_OFF_60HZ)
      if((CurrentScanline.Tricks&TRICK_LINE_PLUS_24) && CyclesIn<372) // &WU?
      {
        CurrentScanline.Tricks&=~TRICK_LINE_PLUS_24;
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
        CurrentScanline.Bytes+=2;
        overscan_add_extra+=2;
      }
#endif
#endif

    }
  }
  else // freq 60hz
  {
    new_freq=60;
    shifter_freq_idx=1;
    if(CyclesIn<=52)
      CurrentScanline.Cycles=scanline_time_in_cpu_cycles[shifter_freq_idx]; 

    if(FetchingLine())
    {


      if(CyclesIn<=52 && left_border) // TO CHECK
        CurrentScanline.StartCycle=52; 

      if(CyclesIn<=372)
      {
        CurrentScanline.EndCycle=372;
        // adjust timer B, not sure it changes anything
        if(time_of_next_event==time_of_next_timer_b) 
          time_of_next_event-=4;
        time_of_next_timer_b-=4; 
      }
    }
  }
  if(shifter_freq!=new_freq)
  {
    freq_change_this_scanline=TRUE;  
#if defined(SSE_INT_MFP_TIMER_B_RECOMPUTE)
    CALC_CYCLES_FROM_HBL_TO_TIMER_B(new_freq); //3.6.1B test
#endif
  }
  shifter_freq=new_freq;
  
#if defined(SSE_SHIFTER_TRICKS)
  AddFreqChange(new_freq);
#else
  if(shifter_freq_change[shifter_freq_change_idx]!=MONO_HZ)
    ADD_SHIFTER_FREQ_CHANGE(shifter_freq);
#endif
  if(FullScreen && border==BORDERS_AUTO_OFF)
  {
    int off=shifter_first_draw_line-draw_first_possible_line;
    draw_first_possible_line+=off;
    draw_last_possible_line+=off;
  }
}


void TShifter::Vbl() {
  // called at the very end of event_vbl_interrupt()
#if defined(SSE_DEBUG_FRAME_REPORT)
  FrameEvents.Vbl(); 
#endif

#if defined(SSE_DEBUG)
  nVbl++; 

#if defined(SSE_OSD_DEBUG_MESSAGE_FREQ) // tell when 60hz
  if(!TRACE_ENABLED  && shifter_freq_at_start_of_vbl==60)
    TRACE_OSD("60HZ");
#endif
#if defined(SSE_OSD_CONTROL)
  if( (OSD_MASK2&OSD_CONTROL_60HZ))// && shifter_freq_at_start_of_vbl==60) 
    TRACE_OSD("R%d S%d",m_ShiftMode,m_SyncMode);
    //TRACE_OSD("60HZ");
#endif

#endif
#if defined(SSE_SHIFTER_UNSTABLE)
  HblPixelShift=0;
#endif

#if defined(SSE_TIMINGS_FRAME_ADJUSTMENT)
  n508lines=0;
#endif
}

#undef LOGSECTION

// part moved from sseshifter.h, vc6 couldn't find the functions
// anymore

#if defined(SSE_STRUCTURE_SSESHIFTER_OBJ)

#define LOGSECTION LOGSECTION_VIDEO

void TShifter::AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra) {
  // What a beautiful name!
  // Replacing macro ADD_EXTRA_TO_SHIFTER_DRAW_POINTER_AT_END_OF_LINE(s)

  if(ExtraAdded)
  {
    ASSERT(!overscan_add_extra);
    return;
  }
  extra+=(LINEWID)*2;     
  if(shifter_skip_raster_for_hscroll) 
#if defined(SSE_SHIFTER_STE_MED_HSCROLL)
    extra+= (left_border) ? 
    (screen_res==1) ? 4 : 8 // handle MED RES, fixes Cool STE unreadable scrollers
    : 2;                    // HI RES is handled in IncScanline()
#else
    extra+= (left_border) ? 8 : 2;
#endif



  extra+=overscan_add_extra;
  overscan_add_extra=0;
  ExtraAdded++;
}


int TShifter::CheckFreq(int t) {
/* This is taken from Steem's draw_check_border_removal()
   v3.4: rewritten with a 'for' instead of a 'while' to avoid a 'break'
   (just C masturbation)
*/
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++);
  if(j==32)
    i=-1; // this ugly thing still necessary anyway
  return i;
}


// TODO DE()


#ifdef WIN32

void TShifter::DrawBufferedScanlineToVideo() {
  // replacing macro DRAW_BUFFERED_SCANLINE_TO_VIDEO

  if(draw_store_dest_ad)
  { 
    // Bytes that will be copied.
    int amount_drawn=(int)(draw_dest_ad-draw_temp_line_buf); 
    // From draw_temp_line_buf to draw_store_dest_ad
    DWORD *src=(DWORD*)draw_temp_line_buf; 
    DWORD *dest=(DWORD*)draw_store_dest_ad;  
    while(src<(DWORD*)draw_dest_ad) 
      *(dest++)=*(src++); 
//    ASSERT(draw_med_low_double_height);//OK, never asserted
//    if(draw_med_low_double_height)
    {
      src=(DWORD*)draw_temp_line_buf;                        
      dest=(DWORD*)(draw_store_dest_ad+draw_line_length);      
      while(src<(DWORD*)draw_dest_ad) 
        *(dest++)=*(src++);       
    }                                                              
    draw_dest_ad=draw_store_dest_ad+amount_drawn;                    
    draw_store_dest_ad=NULL;                                           
    draw_scanline=draw_store_draw_scanline; 
  }
  ScanlineBuffer=NULL;
}

#endif

int TShifter::FetchingLine() {
  // does the current scan_y involve fetching by the Shifter?
  // notice < shifter_last_draw_line, not <=
  return (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line

#if defined(SSE_VID_BORDERS_BIGTOP)
//          && scan_y>=draw_first_possible_line+(DISPLAY_SIZE>=3?6:0) 
      && (DISPLAY_SIZE<BIGGEST_DISPLAY 
      || scan_y>=draw_first_scanline_for_border+
        (BIG_BORDER_TOP-ORIGINAL_BORDER_TOP))
#endif
        );
}


#if defined(SSE_SHIFTER_FIX_LINE508_CONFUSION)
/*  In Steem the timings of all HBLs are prepared for each frame
    using the "event" system.
    In a 50hz frame, when there are 60hz (508 cycles) scanlines, 
    Steem will be off by 4 cycles in its reckoning of the start
    of the HBL (linecycle 0).
    This is hard to understand and also to fix without messing other
    timings (like CPU, MFP...)
    The little function here will just allow to have the
    right linecyles on the scanline after the 60hz scanline.
    Cases where it matters:
    Omega, TCB
*/
bool TShifter::Line508Confusion() {
  bool rv=(PreviousScanline.Cycles==508 && CurrentScanline.Cycles==508 
    && shifter_freq_at_start_of_vbl==50);
//  ASSERT(!rv);

//  if(rv) TRACE("508? %d\n",scan_y);
//  if(rv) REPORT_LINE;

  return rv;
//  return(PreviousScanline.Cycles==508 && CurrentScanline.Cycles==508 
  //  && shifter_freq_at_start_of_vbl==50);
}
#endif


void TShifter::RoundCycles(int& cycles_in) {
  cycles_in-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !HSCROLL) 
    cycles_in+=16;
  cycles_in+=SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in&=-SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !HSCROLL) 
    cycles_in-=16;
}


void TShifter::SetPal(int n, WORD NewPal) {
/*        STF
    A sixteen word color lookup  palette  is
    provided  with  nine  bits  of color per entry.  The sixteen
    Color Palette Registers (read/write,  reset:  not  affected)
    contain  three  bits  of red, green, and blue aligned on low
    nibble boundaries.  Eight intensity  levels  of  red,  eight
    intensity  levels  of  green,  and eight intensity levels of
    blue produce a total of 512 possible colors.
    In 320 x 200 4 plane mode all  sixteen  palette  colors
    can  be  indexed,  while  in 640 x 200 2 plane mode only the
    first four palette entries are applicable.   In  640  x  400
    monochrome mode the color palette is bypassed altogether and
    is instead provided with an inverter for inverse video  con-
    trolled  by  bit 0 of palette color 0 (normal video is black
    0, white 1).  Color palette memory is arranged the  same  as
    main  memory.   Palette  color  0  is  also used to assign a
    border color while in a  multi-plane  mode.   In  monochrome
    mode the border color is always black.

    ff 8240   R/W     |-----xxx-xxx-xxx|   Palette Color 0/0 (Border)
                            ||| ||| |||
                            ||| ||| || ----   Inverted/_Normal Monochrome
                            ||| ||| |||
                            ||| |||  ------   Blue
                            |||  ----------   Green
                             --------------   Red
    ff 8242   R/W     |-----xxx-xxx-xxx|   Palette Color 1/1
    ff 8244   R/W     |-----xxx-xxx-xxx|   Palette Color 2/2
    ff 8246   R/W     |-----xxx-xxx-xxx|   Palette Color 3/3
    ff 8248   R/W     |-----xxx-xxx-xxx|   Palette Color 4
    ff 824a   R/W     |-----xxx-xxx-xxx|   Palette Color 5
    ff 824c   R/W     |-----xxx-xxx-xxx|   Palette Color 6
    ff 824e   R/W     |-----xxx-xxx-xxx|   Palette Color 7
    ff 8250   R/W     |-----xxx-xxx-xxx|   Palette Color 8
    ff 8252   R/W     |-----xxx-xxx-xxx|   Palette Color 9
    ff 8254   R/W     |-----xxx-xxx-xxx|   Palette Color 10
    ff 8256   R/W     |-----xxx-xxx-xxx|   Palette Color 11
    ff 8258   R/W     |-----xxx-xxx-xxx|   Palette Color 12
    ff 825a   R/W     |-----xxx-xxx-xxx|   Palette Color 13
    ff 825c   R/W     |-----xxx-xxx-xxx|   Palette Color 14
    ff 825e   R/W     |-----xxx-xxx-xxx|   Palette Color 15

          STE 4096 Colours oh boy!
FF8240 ---- 0321 0321 0321
through     Red Green Blue
FF825E


  Palette Register:                                 ST     STE

    $FFFF8240  0 0 0 0 x X X X x X X X x X X X       X     x+X

    $FFFF8242  0 0 0 0 x X X X x X X X x X X X ...

    ...

    $FFFF825E  0 0 0 0 x X X X x X X X x X X X    (16 in total)

                       ---_--- ---_--- ---_---

                         red    green    blue

  These 16 registers serve exactly the same purpose as in the ST. 
  The only difference is that each Nibble for Red, Green or Blue 
  consists of 4 bits instead of 3 on the ST.

  For compatibility reasons, each nibble encoding the Red, Green or Blue
   values is not ordered "8 4 2 1", meaning the least significant 
   bit represents the value "1" while the most significant one represents 
   the value "8" - This would make the STE rather incompatible with the ST 
   and only display dark colours. The sequence of Bits is "0 3 2 1" encoding 
   the values "1 8 4 2". Therefore to fade from black to white, the sequence 
   of colours would be $000, $888, $111, $999, $222, $AAA, ...

*/

#if defined(SSE_SHIFTER_PALETTE_STF) //no
  PAL_DPEEK(n*2)=NewPal; // record as is
#endif

#if defined(SSE_STF_PAL)
  if(ST_TYPE!=STE)
    NewPal &= 0x0777; // fixes Forest HW STF test
  else
#endif
    NewPal &= 0x0FFF;
#if defined(SSE_VID_AUTOOFF_DECRUNCHING)//no
  if(!n && NewPal && NewPal!=0x777) // basic test
    overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
    if(STpal[n]!=NewPal)
    {
      int CyclesIn=LINECYCLES;
#if defined(SHIFTER_PREFETCH_LATENCY)
      if(draw_lock && CyclesIn>SHIFTER_PREFETCH_LATENCY+SHIFTER_RASTER_PREFETCH_TIMING)
#else
      if(draw_lock)
#endif
        Shifter.Render(CyclesIn,DISPATCHER_SET_PAL);
      STpal[n]=NewPal;
#if !defined(SSE_SHIFTER_PALETTE_STF)
      PAL_DPEEK(n*2)=STpal[n];
#endif
      if(!flashlight_flag && !draw_line_off DEBUG_ONLY(&&!debug_cycle_colours))
        palette_convert(n);
    }
}

#if defined(SSE_SHIFTER_TRICKS)

/* V.3.3 used Hatari analysis. Now that we do without this hack, we need
   look-up functions to extend the existing Steem system.
   To make it easier we duplicate the 'freq_change' array for shift mode (res)
   changes (every function is duplicated even if not used).
   TODO: debug & optimise (seek only once), this is first draft
   project for v3.7.1 is to replace those functions with a table recording only
   pertinent info
*/

void TShifter::AddFreqChange(int f) {
  // Replacing macro ADD_SHIFTER_FREQ_CHANGE(shifter_freq)
  shifter_freq_change_idx++;
  shifter_freq_change_idx&=31;
  shifter_freq_change_time[shifter_freq_change_idx]=ABSOLUTE_CPU_TIME;
#if defined(SSE_SHIFTER_FIX_LINE508_CONFUSION)
  if( Line508Confusion() )
    shifter_freq_change_time[shifter_freq_change_idx]-=4;
#endif
  shifter_freq_change[shifter_freq_change_idx]=f;                    
}


void TShifter::AddShiftModeChange(int mode) {
  // called only by SetShiftMode
  ASSERT(!mode||mode==1||mode==2);
  shifter_shift_mode_change_idx++;
  shifter_shift_mode_change_idx&=31;
  shifter_shift_mode_change_time[shifter_shift_mode_change_idx]=ABSOLUTE_CPU_TIME;
#if defined(SSE_SHIFTER_FIX_LINE508_CONFUSION)
  if( Line508Confusion() )
    shifter_shift_mode_change_time[shifter_shift_mode_change_idx]-=4;
#endif
  shifter_shift_mode_change[shifter_shift_mode_change_idx]=mode;                    
}


int TShifter::CheckShiftMode(int t) {
  // this is inspired by the original Steem tests in draw_check_border_removal
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++);
  if(j==32)
    i=-1;
  return i;
}

/*
inline int TShifter::FreqChange(int idx) { //not used...
  // return value of this change (50 or 60)
  idx&=31;
  //ASSERT(shifter_freq_change[idx]==50||shifter_freq_change[idx]==60);//OK
  return shifter_freq_change[idx];
}

inline int TShifter::ShiftModeChange(int idx) { //not used...
  // return value of this change (0,1,2)
  if(idx==-1)
    return m_ShiftMode;
  idx&=31;
  //ASSERT(!shifter_shift_mode_change[idx]||shifter_shift_mode_change[idx]==1||shifter_shift_mode_change[idx]==2);
  return shifter_shift_mode_change[idx];
}
*/

int TShifter::FreqChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(shifter_freq_change[idx]==50||shifter_freq_change[idx]==60);
  int rv=shifter_freq_change_time[idx]-LINECYCLE0;
  return rv;
}

int TShifter::ShiftModeChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(!shifter_shift_mode_change[idx]||shifter_shift_mode_change[idx]==1||shifter_shift_mode_change[idx]==2);
  int rv=shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return rv;
}


int TShifter::FreqChangeAtCycle(int cycle) {
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

//if(scan_y==-26 && cycle==28 && rv==-1)
  //TRACE("cycle %d t %d i %d j %d shifter_freq_change_time[i] %d shifter_freq_change[i] %d\n",cycle,t,i,j,shifter_freq_change_time[i],shifter_freq_change[i]);

  return rv;
}


int TShifter::ShiftModeChangeAtCycle(int cycle) {
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


int TShifter::FreqChangeIdx(int cycle) {
  // give the idx of freq change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckFreq(t); // use the <= check
  if(idx!=-1 &&shifter_freq_change_time[idx]==t)
    return idx;
  return -1;
}

int TShifter::ShiftModeChangeIdx(int cycle) {
  // give the idx of shift mode change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckShiftMode(t); // use the <= check
  if(idx!=-1 &&shifter_shift_mode_change_time[idx]==t)
    return idx;
  return -1;
}


int TShifter::FreqAtCycle(int cycle) {
  // what was the frequency at this cycle?

#if 1 //do it (nothing) just once, here, as illustration before refactoring...

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

int TShifter::ShiftModeAtCycle(int cycle) {
  // what was the shift mode at this cycle?
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

// REDO it stinks
int TShifter::NextFreqChange(int cycle,int value) {
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

int TShifter::NextShiftModeChange(int cycle,int value) {
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


int TShifter::NextFreqChangeIdx(int cycle) {
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

int TShifter::NextShiftModeChangeIdx(int cycle) {
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


int TShifter::PreviousFreqChange(int cycle) {
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

int TShifter::PreviousShiftModeChange(int cycle) {
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


int TShifter::PreviousFreqChangeIdx(int cycle) {
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

int TShifter::PreviousShiftModeChangeIdx(int cycle) {
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


int TShifter::CycleOfLastChangeToFreq(int value) {
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change[i]==value)
    return shifter_freq_change_time[i]-LINECYCLE0;
  return -1;
}

int TShifter::CycleOfLastChangeToShiftMode(int value) {
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change[i]==value)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}

#endif//#if defined(SSE_SHIFTER_TRICKS)



//////////////////////////
// Shifter draw pointer //
//////////////////////////

/* 
    Video Address Counter:

    STF - read only, writes ignored
    ff 8205   R               |xxxxxxxx|   Video Address Counter High
    ff 8207   R               |xxxxxxxx|   Video Address Counter Mid
    ff 8209   R               |xxxxxxxx|   Video Address Counter Low

    This counter is used by programs that do side overscan. This is handled
    in ReadSDP, based on Steem 3.2, which worked very well for most cases.
    Cases that didn't work: Forest, SNYD/TCB. Enchanted Land somehow.


    STE - read & write

    FF8204 ---- ---- --xx xxxx (High)
    FF8206 ---- ---- xxxx xxxx
    FF8208 ---- ---- xxxx xxx- (Low)

    Video Address Counter.
    Now read/write. Allows update of the video refresh address during the 
    frame. 
    The effect is immediate, therefore it should be reloaded carefully 
    (or during blanking) to provide reliable results. 
    [LOL!]

    According to ST-CNX, those registers are in the MMU, not in the Shifter.

*/

//#if defined(SSE_SHIFTER_SDP_READ) && defined(IN_EMU) 
#ifdef SSE_UNIX
#define min(a,b) (a>b ? b:a)
#define max(a,b) (a>b ? a:b)
#endif

#if defined(SSE_SHIFTER_SDP_READ)
MEM_ADDRESS TShifter::ReadSDP(int CyclesIn,int dispatcher) {

//ASSERT(old_pc!=0x45336);

  if (bad_drawing){
    // Fake SDP
#if defined(SSE_SHIFTER_SDP_TRACE_LOG)
    TRACE_LOG("fake SDP\n");
#endif
    if (scan_y<0){
      return xbios2;
    }else if (scan_y<shifter_y){
      int line_len=(160/res_vertical_scale);
      return xbios2 + scan_y*line_len + min(CyclesIn/2,line_len) & ~1;
    }else{
      return xbios2+32000;
    }
  }
  CheckSideOverscan();
  MEM_ADDRESS sdp; // return value
  if(FetchingLine())
  {
    int bytes_to_count=CurrentScanline.Bytes; // implicit fixes (Forest)

    if(shifter_skip_raster_for_hscroll)
    {
      //TRACE("%d %d %d read skip\n",TIMING_INFO);
#if defined(SSE_SHIFTER_STE_READ_SDP_SKIP) // bugfix
      bytes_to_count+=SHIFTER_RASTER; // raster size depends on shift mode
#else
      bytes_to_count+=SHIFTER_RASTER_PREFETCH_TIMING/2;
#endif
    }

    // note: same in all shift modes (eg Monoscreen by Dead Braincells)
    ASSERT(SHIFTER_RASTER_PREFETCH_TIMING==16);

/*  TEST11F
    On the STE, prefetch starts 16 cycles earlier only if HSCROLL is non null.
    If it did start earlier whatever the value of HSCROLL, the following
    line wouldn't be correct.
    Apparently the 1st prefetch is also 16 cycles in med res.
*/

#if defined(SSE_SHIFTER_STE_READ_SDP_HSCROLL1) // TEST11D
    int bytes_ahead=(shifter_hscroll_extra_fetch&&shifter_skip_raster_for_hscroll) 
#else
    int bytes_ahead=(shifter_hscroll_extra_fetch) 
#endif
      ?(SHIFTER_RASTER_PREFETCH_TIMING/2)*2:(SHIFTER_RASTER_PREFETCH_TIMING/2);

    int starts_counting=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN/2 - bytes_ahead;

/*  84/2-8 = 34
    In Hatari, they start at 56 + 12 = 68, /2 = 34
    In both cases we use kind of magic values.
    The same results from Hatari because their CPU timing at time of read is 
    wrong like in Steem pre 3.5.

    Hack-approach necessary while we're fixing instruction timings, but
    this is the right ST value.

    TODO: modify CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN
    Before that, we keep this big debug block (normally only 
    SSE_CPU_PREFETCH_TIMING will be defined).
*/

#if defined(SSE_CPU_PREFETCH_TIMING)
  starts_counting-=2;
#else
  if(0);
#if defined(SSE_CPU_LINE_0_TIMINGS)
  else if( (ir&0xF000)==0x0000)
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_1_TIMINGS)
  else if( (ir&0xF000)==0x1000)
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_2_TIMINGS)
  else if( (ir&0xF000)==0x2000) 
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_3_TIMINGS)
  else if( (ir&0xF000)==0x3000) 
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_4_TIMINGS)
  else if( (ir&0xF000)==0x4000) 
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_5_TIMINGS)
  else if( (ir&0xF000)==0x5000) 
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_8_TIMINGS)
  else if( (ir&0xF000)==0x8000) 
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_9_TIMINGS)
  else if( (ir&0xF000)==0x9000)
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_B_TIMINGS)
  else if( (ir&0xF000)==0xB000) // CMP & EOR
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_C_TIMINGS)
  else if( (ir&0xF000)==0xC000) // (+ ABCD, EXG, MUL, line C)
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_D_TIMINGS)
  else if( (ir&0xF000)==0xD000) // (+ ABCD, EXG, MUL, line C)
    starts_counting-=2;
#endif
#if defined(SSE_CPU_LINE_E_TIMINGS)
  else if( (ir&0xF000)==0xE000) 
    starts_counting-=2;
#endif
#if defined(SSE_DEBUG_TRACE_LOG_SDP_READ_IR)
  else 
  {
    if(ir!=debug1)
    {
#if defined(DEBUG_BUILD)
      EasyStr instr=disa_d2(old_pc);
      TRACE_LOG("Read SDP ir %x Disa %s\n",ir,instr.c_str());
#else
      TRACE_LOG("Read SDP ir %4X\n",ir);
#endif    
      debug1=ir;
    }
  }
#endif
#endif

    if(!left_border
#if defined(SSE_SHIFTER_HIRES_OVERSCAN)
      || screen_res==2
#endif      
      )
      starts_counting-=26;

#if defined(SSE_SHIFTER_TCB) && defined(SSE_SHIFTER_TRICKS)
/*  The following is a hack for SNYD/TCB.

Troed:
"I'm still slightly confused over how -29 and -28 are not vertically aligned (WS2) 
nor right-aligned (WS1/3/4) when they both are 508 cycle lines according to this 
theory (which seems to pan out). My visual judgment was that -29 is PAL 160 and 
-28 is NTSC 160, but the code shouldn't be able to produce that due to the failed 
top border switch to 50Hz."

Not working:
-30 - 364:S0000 450:S0000 472:r0900 492:r0900 512:r0900 512:T0100 512:#0000
-29 - 020:r0900 040:r0900 060:r0900 080:r0908 512:R0002

Working:
-30 - 388:S0000 474:S0000 496:r0900 512:T0100 512:#0000
-29 - 004:r0900 024:r0900 044:r0900 064:r0900 084:r0908

The difference between working and not working is that the SDP starts
running 4 cycles earlier in "not working", as is expected in a line starting
 at cycle 52 (60hz line).
But Troed sees that line -29 is a PAL one (50hz).
Theory says it should start at cycle 52, the fact is that it
starts at 56!
It could be the double S0 on the line before, that's what the hack
looks for. It's not understood yet.
Cases to consider: TCB, Mindbomb/No Shit, Omega
*/
    else if(SSE_HACKS_ON && CurrentScanline.Cycles==508
      && PreviousScanline.Cycles==512)
    {
      int c2=PreviousFreqChange(0);
      int c1=PreviousFreqChange(c2);
      if(c2>440-512 && c1>360-512 && FreqChangeAtCycle(c2)==60 
        && FreqChangeAtCycle(c1)==60)
        starts_counting+=2;
    }
#endif

    int c=CyclesIn/2-starts_counting;

    sdp=shifter_draw_pointer_at_start_of_line;

    if (c>=bytes_to_count)
      sdp+=bytes_to_count+(LINEWID*2);
    else if (c>=0){
      c&=-2;
      sdp+=c;
    }

#if defined(SSE_SHIFTER_SDP_TRACE_LOG3) // compare with Steem (can't be 100%)
    if(sdp>shifter_draw_pointer_at_start_of_line)
    {
      MEM_ADDRESS sdpdbg=get_shifter_draw_pointer(CyclesIn);
      if(sdpdbg!=sdp)
      {
        TRACE_LOG("SDP 0 %X %d %X (+%d) Steem 3.2 %X (+%d)\n",shifter_draw_pointer_at_start_of_line,CyclesIn,sdp,sdp-shifter_draw_pointer_at_start_of_line,sdpdbg,sdpdbg-shifter_draw_pointer_at_start_of_line);
        MEM_ADDRESS sdpdbg=get_shifter_draw_pointer(CyclesIn); // rego...
      }
    }
#endif
  }
  else // lines witout fetching (before or after frame)
    sdp=shifter_draw_pointer;

//if(!scan_y)TRACE("Read SDP F%d y%d c%d SDP %X (%d - %d) sdp %X xtr %d\n",FRAME,scan_y,CyclesIn,sdp,sdp-shifter_draw_pointer_at_start_of_line,CurrentScanline.Bytes,shifter_draw_pointer,shifter_hscroll_extra_fetch);
#if defined(SSE_SHIFTER_SDP_TRACE2)
  int nbytes=sdp-shifter_draw_pointer_at_start_of_line;
  if(nbytes && scan_y==-29) TRACE("Read SDP F%d y%d c%d SDP %X (%d - %d) sdp %X\n",FRAME,scan_y,CyclesIn,sdp,sdp-shifter_draw_pointer_at_start_of_line,CurrentScanline.Bytes,shifter_draw_pointer);
#endif
  return sdp;
}
#endif
//#endif

//#ifdef IN_EMU//temp

#ifdef SSE_SHIFTER_SDP_WRITE

void TShifter::WriteSDP(MEM_ADDRESS addr, BYTE io_src_b) {
/*
    This is a difficult side of STE emulation, made difficult too by
    Steem's rendering system, that uses the Shifter draw pointer as 
    video pointer as well as ST IO registers. Other emulators may do 
    otherwise, but there are difficulties and imprecisions anyway.
    So this is mainly a collection of hacks for now, that handle pratical
    cases.
    TODO: examine interaction write SDP/left off in Steem
    Some cases (all should display fine, at least they did when writing this):

    An Cool STE
    Line 000 - 008:r0900 028:r0900 048:r0900 068:r0908 452:w050D 480:w07F1 508:w0990 512:T1000
    Not in DE

    Armada is Dead
    Line 198 - 436:w0501 440:w0747 456:P0334 472:w0966 488:H0003 504:F0092 512:R0082 512:TC000
    Line 199 - 008:R0000 376:S0000 384:S0082 444:R0082 456:R0000 496:S0000 512:R0082 512:T2211
    Line 200 - 008:R0000 016:S0082 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2011
    Line 201 - 008:R0000 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2011
    Line 202 - 008:R0000 376:S0000 384:S0082 444:R0082 456:R0000 512:R0082 512:T2011
    It's similar to Big Wobble, the difference we see is that R0 happens at cycles
    008 instead of 012 in Big Wobble. The compatible hack = no shift when 008.
    See SSEShifter.cpp

    Asylum
    Line 001 - Events:  052:V0000 056:w050B 060:w0746 064:w09EE
    It's in fact not in DE, because SDP starts with delay?

    Beat Demo
    Line 199 - 016:F0096 456:S0000 476:w0502 500:w0711
    Line 200 - 012:w09A2

    Big Wobble
    Line -28 - 392:w050B 416:w07FD 440:w0980 512:TC000
    Line -27 - 404:w0508 428:w0715 452:w091A 476:F0124 500:H0009 512:R0082 512:TC000
    Line -26 - 012:R00F0 376:S00F0 384:S0082 444:R0082 456:R00F0 512:R0082 512:T2011
    Line -25 - 012:R00F0 376:S00F0 384:S0082 444:R0082 456:R00F0 512:R0082 512:T2011
    Line -24 - 012:R00F0 376:S00F0 384:S0082 444:R0082 456:R00F0 512:R0082 512:T2011
    Line -23 - 012:R00F0 376:S00F0 384:S0082 444:R0082 456:R00F0 512:R0082 512:T2011
    Post DE, followed with... change Linewid, change HScroll, switch R2/R0
    Seems to require +4 in our system

    Braindamage 3D maze
    Line 117 - 016:r0900 036:r0900 056:r0900 076:r0904 456:r0776 456:w0775 476:P1002 484:P2002 492:P3002 500:P4002 508:P5002 512:T1000
    ...
    Issue can be: extra should have been added

    Cool STE
    Line -61 - 052:H0013 128:w09C6 162:w0505 166:w07DD 372:F0156
    Line 113 - 344:F0086 368:H0015 396:w0501 400:w0765 424:w093E 
    Line 149 - 156:H0015 384:w0501 388:w07D1 412:w092E
    Line 183 - 336:F0156 356:H0013 380:w0507 384:w0736 404:w09C6
    It's in fact "post-DE" (?), LINEWID mustn't apply or it flickers
    It must have been added before

    Coreflakes 3D titles
    Line -28 - 012:R0000 376:S0000 392:S0002 408:PA707 424:P3707 440:R0001 456:R0000 468:w070D 472:w0998 488:P2707 508:R0002 512:T0011
    Notice the switch at 440/456 is R1/R0
    All lines are like this, there should be no shift at all

    Dangerous Fantaisy credits
    Line -49 - 232:H000C 244:F0076 280:w050A 296:w07A8 312:w0918 472:P0000 476:P1077 496:P2067 500:P3056
    Line 084 - 224:H0000 248:F0000 388:w0900 422:w0766 440:w050B 512:T1000
    Line 179 - 336:H0000 360:F0000 392:w0900 426:w07C5 444:w050B 460:P0222 512:T1000
    The write to lower byte in line 179 may occur at 384 (bytes in: 158, drawn: 152)

    E605 planet
    Line -29 - 024:S0002 036:r0900 056:r0900 076:r0904 480:h0000 480:H000A 496:P00FF 508:P00F7
    Line -28 - ...504:R0002
    Line -27 - 008:R0000 376:S0000 392:S0002 416:V0000 420:w0507 424:w07E0 428:w09D6 440:R0002 456:R0000 504:R0002
    instruction used movep.l

    Molz Green pic
    Line -63 - 432:V0000 436:w0514 440:w0759 444:w0900 not DE
    Line -28 - 040:V0000 044:w051D 048:w0799 052:w0940  not DE
    Line 245 - 212:V0000 216:w0500 220:w0700 224:w0900  in DE but doesn't count

    Pacemaker credits
    ...
    Line 063 - 472:V0000 476:w0507 480:w070F 484:w0940 500:P0000 504:P102E
    ...
    Line 070 - 060:V0000 064:w0507 068:w0717 072:w0980 088:P02AB 092:P102E 472:V0000 476:w0507 480:w0718 484:w09E0 500:P0A34 504:P102E
    ...
    Line 079 - Events:  472:V0000 476:w0507 480:w0725 484:w09F0 500:P0000 504:P10A7
    Only 1 while DE, and there's another write post DE, which cancels the 
    'bytes to add' (which makes it work in Hatari?)

    RGBeast
    Line -29 - 072:r0902 332:w0502 336:w07E6 340:w0903 ... 376:S0192 384:S0130
    DE, right border still to be removed
    instruction used movep.l

    Schnusdie (Reality is a Lie)
    Line 199 - 020:R0000 376:S0000 396:S0002 424:w0503 452:w071D 472:S0000 500:w092C 512:T3111
    Line 200 - 020:S0002 376:S0000 396:S0002 428:P0000 432:P1019 436:P2092 440:P302A 444:P40A3 448:P503B 452:P60B4 456:P704C 460:P80C5 464:P905D 468:PA0D6 472:PB06E 476:PC0E7 480:PD07F 484:PE0FF 488:PF0FF 508:R0002 512:T0010
    Line 201 - 020:R0000 376:S0000 396:S0002 428:P0000 432:P1819 436:P2892 440:P382A 444:P48A3 448:P583B 452:P68B4 456:P784C 460:P88C5 464:P985D 468:PA8D6 472:PB86E 476:PC8E7 480:PD87F 484:PE8FF 488:PF8FF 508:R0002 512:T0011

    ST Magazine STE demo (Stax 065)
    070 - 032:M0000 036:w0507 040:w0709 044:w09FA 

    Sunny
    Line -17 - 400:w09E2 440:w077F 480:w0503 (eg)

    Tekila
    Line -29 - 016:S0130 136:r0922 240:V0006 256:V0000 272:w0900 508:R0002
    Other lines
    note that other lines must be aligned with first visible line
    012:R0000 376:S0000 388:S0130 408:w0506 424:w07D3 436:w0986 ... 468:H0008 508:R0002
    -29 & -28 are black,  write in -29 has no visible effect
    instruction used move.b

*/

  int cycles=LINECYCLES; // cycle in Shifter reckoning

#if defined(SSE_SHIFTER_SDP_TRACE_LOG2)
  TRACE_LOG("F%d y%d c%d Write %X to %X\n",FRAME,scan_y,cycles,io_src_b,addr);
#endif

#ifdef SSE_DEBUG
#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK3 & OSD_CONTROL_WRITESDP) 
#else
  if(TRACE_ENABLED)
#endif
    TRACE_OSD("WRITE SDP");  
#endif

#if defined(SSE_STF_SDP)
  // some STF programs write to those addresses, it just must be ignored.
  if(ST_TYPE!=STE)
  {
#if defined(SSE_SHIFTER_SDP_TRACE_LOG)
    TRACE_LOG("STF ignore write to SDP %x %x\n",addr,io_src_b);
#endif
    return; // fixes Nightdawn, STF-only game
  }
#endif

  // some bits will stay at 0 in the STE whatever you write
  if(mem_len<=FOUR_MEGS && addr==0xFF8205) // already in Steem 3.2 (not in 3.3!)
    io_src_b&=0x3F; // fixes Delirious IV bugged STE detection routine
#if defined(SSE_SHIFTER_SDP_WRITE_LOWER_BYTE)
  else if(addr==0xFF8209)
    io_src_b&=0xFE; // fixes RGBeast mush (yoho!)
#endif

  BOOL fl=FetchingLine();
  BOOL de_started=fl && cycles>=CurrentScanline.StartCycle
    +SHIFTER_PREFETCH_LATENCY; // TESTING
  BOOL de_finished=de_started && cycles>CurrentScanline.EndCycle
    +SHIFTER_PREFETCH_LATENCY; // TESTING
  BOOL de=de_started && !de_finished;
  BOOL middle_byte_corrected=FALSE;

  Render(cycles,DISPATCHER_WRITE_SDP);

#if defined(SSE_SHIFTER_SDP_READ)
  MEM_ADDRESS sdp_real=ReadSDP(cycles,DISPATCHER_WRITE_SDP); 
#else
  MEM_ADDRESS sdp_real=get_shifter_draw_pointer(cycles);
#endif
  ///int sdp_current_byte=( sdp_real>>8*((0xff8209-addr)/2) ) & 0xFF;
  int bytes_in=sdp_real-shifter_draw_pointer_at_start_of_line;
  // the 'draw' pointer can be <, = or > 'real'
  int bytes_drawn=shifter_draw_pointer-shifter_draw_pointer_at_start_of_line;

  if(addr==0xff8209 && SDPMiddleByte!=999) // it has been set?
  {
    int current_sdp_middle_byte=(shifter_draw_pointer&0xFF00)>>8;
    if(current_sdp_middle_byte != SDPMiddleByte) // need to restore?
    {
#if defined(SSE_SHIFTER_SDP_TRACE_LOG)
      TRACE_LOG("F%d y%d c%d SDP %X reset middle byte from %X to %X\n",FRAME,scan_y,cycles,shifter_draw_pointer,current_sdp_middle_byte,SDPMiddleByte);
#endif
      DWORD_B(&shifter_draw_pointer,(0xff8209-0xff8207)/2)=SDPMiddleByte;
      middle_byte_corrected=TRUE;
    }
  }

  MEM_ADDRESS nsdp=shifter_draw_pointer; //sdp_real (should be!)
  DWORD_B(&nsdp,(0xff8209-addr)/2)=io_src_b;

  // Writing low byte while the Shifter supposedly is fetching
  if(addr==0xFF8209 && de)
  {

#if defined(SSE_SHIFTER_PACEMAKER)
    if(SSE_HACKS_ON&&(bytes_drawn==8||bytes_drawn==16)) 
    {
      nsdp+=bytes_drawn; // hack for Pacemaker credits line 70 flicker/shift
      // doesn't work in VLB mode
      overscan_add_extra-=bytes_drawn; 
    }
#endif

#if defined(SSE_SHIFTER_DANGEROUS_FANTAISY)
    if(SSE_HACKS_ON
      && bytes_drawn==CurrentScanline.Bytes-8 && bytes_in>bytes_drawn)
      nsdp+=-8; // hack for Dangerous Fantaisy credits lower overscan flicker
#endif

    // recompute right off bonus bytes - only necessary post-display
    // this part not considered a "hack", but is the computing correct?
    if(!right_border && !ExtraAdded && cycles>416)
    {
      int pxtodraw=320+BORDER_SIDE*2-scanline_drawn_so_far;
      int bytestofetch=pxtodraw/2;
      int bytes_to_run=(CurrentScanline.EndCycle-cycles+SHIFTER_PREFETCH_LATENCY)/2;
//      ASSERT(CurrentScanline.EndCycle==460);  bytes_to_run+=2; // is it 460 or 464?//v3.5.4
      overscan_add_extra+=-44+bytes_to_run-bytestofetch;
    }

    // cancel the Steem 3.2 fix for left off with STE scrolling on
    if(!ExtraAdded && (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      && HSCROLL>=12
#if defined(SSE_VID_BORDERS_416_NO_SHIFT) 
      // don't try to understand this, I don't
      && (!SSE_HACKS_ON||SideBorderSize!=VERY_LARGE_BORDER_SIDE||!border)
#endif
      )
      overscan_add_extra+=8; // fixes bumpy scrolling in E605 Planet

#if defined(SSE_SHIFTER_SDP_WRITE_DE_HSCROLL)
    if(SSE_HACKS_ON
      &&shifter_skip_raster_for_hscroll && !left_border && !ExtraAdded)
    {
      if(PreviousScanline.Tricks&TRICK_STABILISER)
#if defined(SSE_SHIFTER_TEKILA)
        nsdp+=-2; // fixes D4/Tekila shift
#else
        ;
#endif
      else 
#if defined(SSE_CPU_PREFETCH_TIMING) || defined(CORRECTING_PREFETCH_TIMING)
        nsdp+=2; // it's because we come sooner (which is more correct)
#else
        nsdp+=4; // fixes E605 Planet shift
#endif
    }
#endif
  }

  if(de_finished && addr==0xff8209)
    CurrentScanline.Tricks|=TRICK_WRITE_SDP_POST_DE;

  // update shifter_draw_pointer_at_start_of_line or ReadSDP returns garbage
  shifter_draw_pointer_at_start_of_line-=shifter_draw_pointer; //sdp_real
  shifter_draw_pointer_at_start_of_line+=nsdp;
  shifter_draw_pointer=nsdp;

#if defined(SSE_SHIFTER_SDP_WRITE_MIDDLE_BYTE)
  // hack, record middle byte, programmer couldn't intend it to change
  // note for Cool STE it flickers on some real STE
  if(SSE_HACKS_ON && fl && addr==0xff8207)  
    SDPMiddleByte=io_src_b; // fixes Pacemaker credits, Tekila, Cool STE
#endif

  CurrentScanline.Tricks|=TRICK_WRITE_SDP; // could be handy

}

#endif

//#endif//#ifdef IN_EMU

void TShifter::ShiftSDP(int shift) { 
  shifter_draw_pointer+=shift; 
  overscan_add_extra-=shift;
}

#undef LOGSECTION
//#endif//SSE_STRUCTURE_SSESHIFTER_OBJ



#endif//#if defined(SSE_SHIFTER)
#endif//STEVEN_SEAGAL//?
