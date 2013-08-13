#if defined(SS_SHIFTER)

#if defined(SS_STRUCTURE_INFO)
#pragma message("Included for compilation: SSEShifter.cpp")
#endif

#if defined(SS_MMU)
#include "SSE/SSEMMU.h"
#endif

#if defined(SS_SHIFTER_EVENTS)
#include "SSEShifterEvents.cpp" // debug module, the Steem way...
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
    inflated, for a good cause: now Steem handles a lot more shifter tricks
    than before.
    Much has been inspired by Hatari, but was also discussed on Atari-forum.
    In some cases we go beyond Hatari (at least v1.6.1)
    Also like in Hatari comments briefly explain shifter tricks.
*/

void TShifter::CheckSideOverscan() {
/*  An overbloated version of Steem's draw_check_border_removal()
    This function is called before rendering if there have been shifter events.
    It takes care of all "side overscan" business.
    Some features have been added on the v3.2 basis:
    STE-only left border off (line +20), 0-byte line, 4bit hardscroll, etc.
    Other shifter tricks have been refined, and difference between ST models is
    handled.
    The "old" Steem look-up system is still used, instead of the "Hatari" way
    of acting at the shifter events themselves. 
    The advantages are that we integrate in what exists, that everything 
    is nicely concentrated in one place and that we don't need to take care 
    of the -1 problem when events occur at 'LineCycles+n' (eg 520).
    Disadvantage could be performance, so it's not definitive.
    For STF wake up state 2, all thresholds are increased by 2. This isn't
    rock solid emulation, but it isn't just a few hacks either now.
*/
#if defined(SS_DEBUG) && defined(SS_SHIFTER_DRAW_DBG)
  draw_check_border_removal(); // base function in Steem 3.2
  return;
#endif
 
  int act=ABSOLUTE_CPU_TIME,t,i;
  int CyclesIn=LINECYCLES;
  if(screen_res>=2
    || !FetchingLine() 
    || !act) // if ST is resetting
    return;

#if defined(SS_SHIFTER_TRICKS) 
    int r0cycle,r1cycle,r2cycle,s0cycle,s2cycle;
#endif

  ///////////////////////////
  // 0-BYTE LINE (line-160 //
  ///////////////////////////

/*  Various shift mode or sync mode switches trick the shifter in not fetching
    a scanline. This is a way to implement "hardware" vertical scrolling 
    (-160 bytes) on a computer where it's not foreseen.
    The monitor cathodic ray goes on (of course!) and shows a black line.
*/

#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_0BYTE_LINE)

#if defined(SS_SHIFTER_0BYTE_LINE_RES_END)

/*  A shift mode switch a the end of the line causes the shifter not to fetch
    video RAM and not to count the line in its internal register.
    Game No Buddies Land (500/508), demo D4/NGC (mistake? 496/508).
*/

    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
      && !ShiftModeChangeAtCycle(-4) // 508
      && (ShiftModeChangeAtCycle(-16)==2 || ShiftModeChangeAtCycle(-12)==2))
    {
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
      shifter_last_draw_line++; // fixes lower overscan in D4/NGC
    }
    
#endif

#if defined(SS_SHIFTER_0BYTE_LINE_RES_HBL) && defined(SS_STF)

/*  Beyond/Pax Plax Parallax uses a shift mode switch around the hblank position
    (as in Enchanted Land's hardware test), but DE is off (right border on).
    This causes a 0-byte line (from Hatari).
    It seems to do that only in STF mode; in STE mode the program writes the 
    video RAM address to scroll the screen.
    Maybe the same switch +4 would work on a STE, like for Lemmings?
*/

    if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE) && ST_TYPE!=STE 
      && ShiftModeChangeAtCycle(464-4)==2 && !ShiftModeChangeAtCycle(464+8))
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
#endif

#if defined(SS_SHIFTER_0BYTE_LINE_RES_START)

/*  A shift mode switch after left border removal prevents the shifter from
    fetching the line. The switches must be placed on different cycles 
    according to the ST model.
    Nostalgic-O/Lemmings 28/44 for STF 32/48 for STE
*/

  if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE))
  {
    r2cycle=
#if defined(SS_STF)
      (ST_TYPE!=STE)?28: 
#endif
    32;
    r0cycle=r2cycle+16; // we take the precise value for performance
    if(ShiftModeChangeAtCycle(r2cycle)==2 && !ShiftModeChangeAtCycle(r0cycle))
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;
  }
#endif
  
#if defined(SS_SHIFTER_0BYTE_LINE_SYNC) 

/*  0-byte sync switches are model-dependent and even "wake up state"-
    dependent. They  are used only in a 2006 demo called SHFORSTV.TOS,
    by the dude who also made the Overscan Demos.
    Also: beeshift
    Switche: STF(1) (56/64), STF(2) (58/68), and STE (40/52).
    Haven't found other working combinations yet (hacker!)
*/

  if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE))
  {

#if defined(SS_STF) // determine which switches we're looking for
    if(ST_TYPE!=STE)
    {
#if defined(SS_MMU_WAKE_UP_0_BYTE_LINE)
      if(WAKE_UP_STATE==2) // OK, Forest says it's 2
        s0cycle=58,s2cycle=68+2; // but strange according to compile options
      else
#endif
        s0cycle=56,s2cycle=64; // default STF1 (OK for 'Forest')
    }
    else
#endif // STE:
      s0cycle=40,s2cycle=52; // this also produces a 512 cycles +2 line?

    if(FreqChangeAtCycle(s0cycle)==60 && FreqChangeAtCycle(s2cycle)==50)
      CurrentScanline.Tricks|=TRICK_0BYTE_LINE;

  }
#endif

  // 0-byte line: action
  if(CurrentScanline.Tricks&TRICK_0BYTE_LINE)
  {
    if(!(TrickExecuted&TRICK_0BYTE_LINE))
    { 
      draw_line_off=true; // Steem's original flag
      memset(PCpal,0,sizeof(long)*16); // all colours black
      shifter_draw_pointer-=160; // hack: Steem will draw black and update SDP
      CurrentScanline.Bytes=0;
      TrickExecuted|=TRICK_0BYTE_LINE;
    }
    return; // 0byte line takes no other tricks
  }

#endif

#if defined(SS_SHIFTER_DRAGON1)//temp
    if(SS_signal==SS_SIGNAL_SHIFTER_CONFUSED_2
      && !(CurrentScanline.Tricks&TRICK_CONFUSED_SHIFTER))
    {
      CurrentScanline.Tricks|=TRICK_CONFUSED_SHIFTER;
      ShiftSDP(-2);
      scanline_drawn_so_far=4;//not perfect...
    }
#endif

  /////////////////
  // LEFT BORDER //
  /////////////////

/*  Removing the border can be used with two goals: use a larger display area
    (fullscreen demos are more impressive than borders-on demos), and
    scroll the screen by using "sync lines" (eg Enchanted Land).
*/

  if(left_border
#ifndef SS_SHIFTER_LEFT_OFF_TEST_BEFORE_HBL // fixes Riverside low border
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
    224/8=28,no rest->no shifter confusion.
    Known switches: 504/4 (More or Less Zero); 508/4 (Greets)
    E605 planet 504/8, with stabiliser & STE scrolling: confusing, apparently
    it's not +20, it's +26, but don't we have a -6 shift somewhere instead?
    More cases needed...
*/

#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_LINE_PLUS_20)

#if defined(SS_STF)
    if(ST_TYPE==STE) 
#endif
    {
      if(!ShiftModeChangeAtCycle(4) && (ShiftModeChangeAtCycle(-8)==2
        || ShiftModeChangeAtCycle(-4)==2))
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_20;
    }
#endif

  ///////////////////////////////////
  //  LEFT BORDER OFF (line +26)   //
  ///////////////////////////////////

/*
    Shifter latency could  explain that you may place your switch in the
    first cycles of the line, and the bonus bytes are still 26. My theory!
    The limits are:
    STF R2 8 / R0 28
    STE R2 2 / R0 26
    Programs not working on a STE because they hit at cycle 4: 
    3615Gen4 by Cakeman
    Cuddly Demos (not patched)
    Musical Wonder 1991
    Imagination/Roller Coaster SNYD/TCB
    Omega (cycle 8 is only apparent)  ?
    Overscan Demos original
    SNYD/TCB (F4)
    SNYD2/Sync fullscreen (F5)
    SoWatt/Menu+Sync

    For the moment, emulation works better if we grant +26 for 60Hz as well
    as 50Hz lines, and we consider a line starting at 56 as well as 0 and
    ending at 372 (60hz) as -2. Hatari has the same approach. A demo that
    seems to justify this approach is Forest. In fact fullscreen demos seem
    to expect the same bonus bytes also at the lower overscan limit, with a
    line at 60hz when the shift mode switch happens.
    But maybe it's something else compensating, as the guy knew his business.
*/

#if defined(SS_SHIFTER_TRICKS) 
    int lim_r2=2,lim_r0=26;    
#if defined(SS_STF) 
    if(ST_TYPE!=STE)
      lim_r2+=6,lim_r0+=2; // experimental

#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
    if(MMU.WakeUpState2())
      lim_r2+=2,lim_r0+=2;
#endif

#endif
#endif

    if(!(CurrentScanline.Tricks&TRICK_LINE_PLUS_20))
    {
#if defined(SS_SHIFTER_STEEM_ORIGINAL) // in fact we could keep it? it's simpler
      t=cpu_timer_at_start_of_hbl+2+(ST_TYPE!=STE ? 4 : 0);
      if(act-t>0)
      {
        i=CheckFreq(t);
        if(i>=0 && shifter_freq_change[i]==MONO_HZ)
          CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
      }
#elif defined(SS_SHIFTER_TRICKS)
      r2cycle=NextShiftModeChange(-12,2); // cycle 504 of previous line
      if(r2cycle!=-1 && r2cycle<=lim_r2)
      {
        r0cycle=NextShiftModeChange(r2cycle); 
        // ShiftMode at r0cycle can be 0 (generally) or 1 (ST-CNX, HighResMode)
        if(r0cycle>r2cycle && r0cycle<=lim_r0)
        {
          if(ShiftModeChangeAtCycle(r0cycle)!=2)
            CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
          else // we have R2-R2-R0, like in Ultimate GFA Demo boot screen
          {
            r0cycle=NextShiftModeChange(r0cycle,0); // look for next (R0 only)
            if(r0cycle>-1 && r0cycle<=lim_r0)
              CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
          }
        } 
      }
#if defined(SS_VID_LEFT_OFF_COMPARE_STEEM_32)
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
#endif
    }
    
    // action for line+26 & line+20
    if( (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      || (CurrentScanline.Tricks&TRICK_LINE_PLUS_20))
    {
#if defined(SS_SHIFTER_LINE_PLUS_20)
      if(CurrentScanline.Tricks&TRICK_LINE_PLUS_20)
      {
        CurrentScanline.Bytes+=20;
#if defined(SS_VID_BORDERS)
        if(SideBorderSize==ORIGINAL_BORDER_SIDE)
#endif
          overscan_add_extra+=4; // 16+4=20
#if defined(SS_VID_BORDERS)
        else          
          overscan_add_extra=-4; // 24+4=24; 24-4=20
#endif
        overscan=OVERSCAN_MAX_COUNTDOWN;
#if defined(SS_SHIFTER_STE_HSCROLL_LEFT_OFF)  
        if(shifter_skip_raster_for_hscroll) 
          overscan_add_extra+=6; // fixes MOLZ/Spiral
#endif
        TrickExecuted|=TRICK_LINE_PLUS_20;
#if defined(SS_SHIFTER_LINE_PLUS_20_SHIFT)
        HblPixelShift=-8; // fixes Riverside shift
        if(SideBorderSize!=ORIGINAL_BORDER_SIDE)
          ShiftSDP(-8);
#endif
      }
      else // normal left off
#endif
      {
        if(shifter_freq_at_start_of_vbl==50) 
        {
/*  Cases where we see that this shift is necessary:
    - Oh Crikey What a Scorcher Hidden Screen ULM: obvious shift
    - Overdrive/Dragon
    - Overscan Demos #6 (strangely)
*/
          shifter_pixel+=4;

          overscan_add_extra+=2;  // 8 + 2 + 16 = 26
        }
        CurrentScanline.Bytes+=26;
#if defined(SS_SHIFTER_UNSTABLE)
        // left off eats one preloaded RR, see ST-CNX doc
        if(Preload>0)
          Preload--;
#endif
        if(HSCROLL>=12) // STE shifter bug (Steem authors)
          ShiftSDP(8);
#if defined(SS_VID_BORDERS)
        if(SideBorderSize==ORIGINAL_BORDER_SIDE) // 32
#endif
          shifter_draw_pointer+=8; // 8+16+2=26
//        TRACE_LOG("+26 y %d px %d\n",scan_y,shifter_pixel);
        TrickExecuted|=TRICK_LINE_PLUS_26;

        // additional hacks for left off
        //////////////////////////////////

#if defined(SS_SHIFTER_UNSTABLE_LEFT_OFF) && defined(SS_MMU_WAKE_UP) \
        && defined(SS_STF)
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

#if defined(SS_SHIFTER_BIG_WOBBLE) && defined(SS_SHIFTER_SDP_WRITE)
        if(SSE_HACKS_ON && (PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE)
          && HSCROLL  // so it's STE-only
#if defined(SS_SHIFTER_ARMADA_IS_DEAD)
          && r0cycle==12
#endif
          ) 
          shifter_draw_pointer+=-4; // fixes Big Wobble, see SSEShifter.h
#endif

#if defined(SS_SHIFTER_SCHNUSDIE) && defined(SS_SHIFTER_TRICKS)
        if(SSE_HACKS_ON && !(PreviousScanline.Tricks&TRICK_STABILISER)
          && r2cycle==-4 && r0cycle==20 // 508/20: very long in mode 2
          && shifter_hscroll_extra_fetch && !HSCROLL) // further "targeting"
          overscan_add_extra+=2; // fixes Reality is a Lie/Schnusdie
#endif
      }
      left_border=0;
      CurrentScanline.StartCycle=0;
      overscan=OVERSCAN_MAX_COUNTDOWN;
    }
  }

  ////////////////
  // STABILISER //
  ////////////////

/*  A HI/LO switch after DE "resets" the shifter by emptying its registers.
    All kinds of strange effects may result from absence of stabiliser when
    trying to display lines which have a length in bytes not multiple of 8,
    such as the 230 bytes line (left off, right off).
*/

#if defined(SS_SHIFTER_TRICKS) && defined(TRICK_STABILISER)
  if(!(CurrentScanline.Tricks&TRICK_STABILISER))
  {
    r2cycle=NextShiftModeChange(432/*,2*/); // Forest STF2
    if(r2cycle>-1 && r2cycle<460)
    {
      r0cycle=NextShiftModeChange(r2cycle,0);
      if(r0cycle>-1 && r0cycle<464 ) 
      {
        CurrentScanline.Tricks|=TRICK_STABILISER;
#if defined(SS_SHIFTER_UNSTABLE)
        Preload=0; // "reset" empties RR
#endif
      }
    }
  }
#endif

  /////////////////////
  // DESTABILISATION //
  /////////////////////

/*  Detect MED/LO switches during DE.
    This will load some words into the shifter, the shifter will start displaying
    shifted planes and shifted pixels in low res.
    In Steem's current rendering system, we must use indirect ways, we miss a
    'shift' function.
*/

#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_UNSTABLE)
/*
detect unstable: switch MED/LOW - Beeshift
- 3 (screen shifted by 12 pixels because only 1 word will be read before the 4 are available to draw the bitmap);
- 2 (screen shifted by 8 pixels because only 2 words will be read before the 4 are available to draw the bitmap);
- 1 (screen shifted by 4 pixels because only 3 words will be read before the 4 are available to draw the bitmap);
- 0 (screen shifted by 0 pixels because the 4 words will be read to draw the bitmap);
*/
  if( ST_TYPE==STF && WAKE_UP_STATE // 1 & 2
    && !(CurrentScanline.Tricks&TRICK_UNSTABLE) 
    && (PreviousScanline.Tricks&TRICK_STABILISER) // shifter is stable
    && left_border && LINECYCLES>56 && LINECYCLES<372) //'DE'
  {
    r1cycle=NextShiftModeChange(56,1); // detect switch to medium
    if(r1cycle>-1)
    {
      r0cycle=NextShiftModeChange(r1cycle,0); // detect switch to low
      if(r0cycle>r1cycle)
      {
        int cycles_in_med=r0cycle-r1cycle;
        if(cycles_in_med<=20) 
          Preload=cycles_in_med/4;  // at least it tries to make sense
//        TRACE("y%d R1 %d R0 %d cycles in med %d Preload %d\n",scan_y,r1cycle,r0cycle,cycles_in_med,Preload);
      }
    }
  }
  
#endif

#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_UNSTABLE)
/*  Shift due to unstable shifter, caused by a line+26 (Dragon) without
    stabiliser or a MED/LOW switch, or a line+230 without stabiliser:
    apply effect
    Note this isn't exact science, it's in development.
    DOLB, Omega: 3 words preloaded but it becomes 2 due to left off
    Dragon: 1 word preloaded    
    
    It could be that both determining preload and using preload are wrong!
*/
  
  if( ST_TYPE==STF && WAKE_UP_STATE // 1 & 2
    && Preload && !(CurrentScanline.Tricks&TRICK_UNSTABLE)
    && CyclesIn>40   // wait for left-off check (->Preload--, Omega)
    ) 
  {
    ASSERT( Preload>0 && Preload<6 );

/*
8 -> 2 words -> -8       86-94
12 -> 3 words -> -12     86-98
16 -> 4 words -> 0
20 -> 5 words -> -4

cycles words	MOD	SDP 	pixel
				
8	  2	2	4	8
12	3	3	6	12
16	4	0	0	0
20	5	1	2	4

*/ 

    // 1. planes
    int shift_sdp=-(Preload%4)*2; // unique formula
    if(Preload>1 && shift_sdp) // but...
      shift_sdp+=8; // hack!
    ShiftSDP(shift_sdp); // correct plane shift
    // 2. pixels
    // Beeshift, the full frame shifts in the border
    if(left_border)
    {
      left_border-=(Preload%4)*4;
      right_border+=(Preload%4)*4;
    }
    else  //  hack for DOLB, Omega, centering the pic
    {
      if(SSE_HACKS_ON)
        HblPixelShift=4; 
    }
//    TRACE_LOG("Y%d Preload %d shift SDP %d pixels %d lb %d rb %d\n",scan_y,Preload,shift_sdp,HblPixelShift,left_border,right_border);

    CurrentScanline.Tricks|=TRICK_UNSTABLE;
  }
#endif

  ////////////////
  // BLACK LINE //
  ////////////////

/*  A sync switch at cycle 28 causes the shifter to fetch the line but not
    display it, showing black pixels instead. Overscan #5,#6, Forest.
    028:S0000 036:S0002
*/

  if(!draw_line_off && shifter_freq_at_start_of_vbl==50)
  {

    t=LINECYCLE0+28; //trigger point
#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
    if(MMU.WakeUpState2())
      t+=2;
#endif

    if(act-t>0)
    {
      i=CheckFreq(t);
      if(i>=0 && shifter_freq_change[i]==60 && shifter_freq_change_time[i]==t)
        CurrentScanline.Tricks|=TRICK_BLACK_LINE;
    }
  }

  if(!draw_line_off && (CurrentScanline.Tricks&TRICK_BLACK_LINE))
  {
    //TRACE_LOG("%d BLK\n",scan_y);
    draw_line_off=true;
    memset(PCpal,0,sizeof(long)*16); // all colours black
  }

  ///////////////////////
  // NON-STOPPING LINE //
  ///////////////////////

/*  In the Enchanted Land hardware tests, a HI/LO switch at end of display
    when the right border has been removed causes the shifter (MMU) to continue 
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
    00020c : 000d 0001  have been the patch, 12 bytes)
*/

#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_NON_STOPPING_LINE)
  if(CurrentScanline.Bytes==160+44 && (// left on, right off
#if defined(SS_STF)
    ST_TYPE!=STE && ShiftModeChangeAtCycle(464)==2 
    && !ShiftModeChangeAtCycle(464+8)|| ST_TYPE==STE && 
#endif
    ShiftModeChangeAtCycle(460)==2 && !ShiftModeChangeAtCycle(460+8)))
  {
    TRACE_LOG("F%d y%d c%d Enchanted Land HW test R2 %d R0 %d\n",FRAME,scan_y,CyclesIn,NextShiftModeChange(460-4,2),NextShiftModeChange(460,0));
    CurrentScanline.Bytes+=24; // "double" right off, fixes Enchanted Land
  }
#endif

  //////////////////////
  // MED RES OVERSCAN //
  //////////////////////

/*  Overscan (230byte lines) is possible in medium resolution too. Because the
    shifter may have fetched some bytes when the switch occurs, we need to 
    shift the display according to R1 cycle (No Cooper Greetings), I think. 
    20 for NCG   512R2 12R0 20R1        shift=2
    28 for PYM/BPOC  512R2 12R0 28R1    shift=0
    36 for NCG off lines 183, 200 512R2 12R0 36R1 (strange!) shift=2
    16 for Drag/Reset    shift=?
    12 & 16 for PYM/STCNX left off (which is really doing 4bit hardcsroll)
    D4/Nightmare shift=2
*/

#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_MED_OVERSCAN)

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
#if defined(SS_SHIFTER_MED_OVERSCAN_SHIFT)
        ShiftSDP( (((cycles_in_low_res)/2)%8)/2 );
#endif
#if defined(SS_VID_BPOC)
        if(BORDER_40 && SSE_HACKS_ON && cycles_in_low_res==16) 
        { // fit text of Best Part of the Creation on a 800 display
          ShiftSDP(4);      
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

/*  When the left border is removed, a MED/LO switch causes the shifter to
    shift the line by a number of pixels dependent on the cycles at which
    the switch occurs. It doesn't change the # bytes fetched (230 in known 
    cases). How exactly it happens is worth a study (TODO). 
    It's a pity that it wasn't used more. Was it a French technique?
    PYM/ST-CNX (lines 70-282 (7-219); offset 8, 12, 16, 20) 0R2 12R1 XR0
    D4/NGC (lines 76-231) 0R2 12R0 20R1 XR0
    D4/Nightmare (lines 143-306, cycles 28 32 36 40)
*/
  
#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_4BIT_SCROLL)

  if(!left_border && screen_res==1 && !(TrickExecuted&TRICK_4BIT_SCROLL))
  {
    r1cycle=CycleOfLastChangeToShiftMode(1);
    if(r1cycle>=12 && r1cycle<=40)
    {
      // look for switch to R0 after switch to R1
      r0cycle=NextShiftModeChange(r1cycle);
      if(r0cycle>r1cycle && r0cycle<=40 && !ShiftModeChangeAtCycle(r0cycle))
      {
        CurrentScanline.Tricks|=TRICK_4BIT_SCROLL;
        TrickExecuted|=TRICK_4BIT_SCROLL;
        int cycles_in_med_res=r0cycle-r1cycle;
        // look for previous switch to R0
        int cycles_in_low_res=0; // 0 ST-CNX
        r0cycle=PreviousShiftModeChange(r1cycle);
        if(r0cycle>=0 && !ShiftModeChangeAtCycle(r0cycle))
          cycles_in_low_res=r1cycle-r0cycle; // 8 NGC, Nightmare
        int shift_in_bytes=8-cycles_in_med_res/2+cycles_in_low_res/4;

#if defined(SS_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK)
        // strange corrections now necessary, quick patch. TODO
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
#if defined(SS_DEBUG)
        ASSERT( HblPixelShift==-7||HblPixelShift==-3||HblPixelShift==1||HblPixelShift==5 );
        ASSERT( shift_in_bytes==0||shift_in_bytes==2||shift_in_bytes==4||shift_in_bytes==6 );
      //  if(scan_y==100)  TRACE_LOG("4bit y%d CMR%d CLR%d SH%d PIX%d\n",scan_y,cycles_in_med_res,cycles_in_low_res,shift_in_bytes,HblPixelShift);
#endif
      }
    }
  }
#endif

  /////////////
  // LINE +2 //
  /////////////

/*  A line that starts at cycle 52 because it's at 60hz, but then is switched
    to 50hz gains 2 bytes because it ends 4 cycles later, at 376 instead of
    372.
    The Steem test may give false alerts.
    We keep this test, but we need to check at end of line, and we change the
    threshold.
*/

  // Steem test
  if(shifter_freq_at_start_of_vbl==50 
    && (left_border==BORDER_SIDE
#if defined(SS_SHIFTER_UNSTABLE)
    || (CurrentScanline.Tricks&TRICK_UNSTABLE) 
#endif
    )
    &&!(TrickExecuted&TRICK_LINE_PLUS_2))
  {   
#if defined(SS_SHIFTER_LINE_PLUS_2_THRESHOLD)
    t=LINECYCLE0+52; // fixes Forest

#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
    if(MMU.WakeUpState2())
      t+=2;
#endif

#else
    t=LINECYCLE0+58; 
#endif
    if(act-t>0) 
    {
      i=CheckFreq(t);

      if(i>=0 && shifter_freq_change[i]==60)
      {
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_2;
//        CurrentScanline.Cycles==512;
      }
    }
  }

  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_2)
    && !(TrickExecuted&TRICK_LINE_PLUS_2))
  {
//    ASSERT(left_border==BORDER_SIDE);
    ASSERT(!(CurrentScanline.Tricks&TRICK_LINE_MINUS_2));
//    ASSERT(CurrentScanline.Cycles==512);
    left_border-=4; // 2 bytes -> 4 cycles
    overscan_add_extra+=2;
    CurrentScanline.Bytes+=2;
    TrickExecuted|=TRICK_LINE_PLUS_2;
    overscan=OVERSCAN_MAX_COUNTDOWN; // 25
//    TRACE_LOG("+2 y %d c %d +2 60 %d 50 %d\n",scan_y,LINECYCLES,FreqChangeCycle(i),FreqChangeCycle(i+1));
#if defined(SS_VID_TRACE_LOG_SUSPICIOUS2)
    if(shifter_freq_change_time[i]-LINECYCLE0<0)
      TRACE_LOG("F%d Suspicious +2 y %d tmg sw %d tmg hbl %d diff %d\n",FRAME,scan_y,shifter_freq_change_time[i],cpu_timer_at_start_of_hbl,shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl);
#endif
  }

  if(right_border_changed) // TODO improve this?
    return; 

  ///////////////
  // LINE -106 //
  ///////////////

/*  A shift mode switch to 2 at cycle 160 (end of HIRES line) causes the line 
    to stop there. 106 bytes are not fetched.
    Just Buggin: 152:R0002 172:R0000 (refix v3.5.2); lower?
    ST-CNX: 160:R0002 172:R0000
*/

#if defined(SS_SHIFTER_STEEM_ORIGINAL) || 0
  t=cpu_timer_at_start_of_hbl+172; //trigger point for big right border
  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
     i=CheckFreq(t);
     if(i>=0 && shifter_freq_change[i]==MONO_HZ)
       CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;
  }
#elif defined(SS_SHIFTER_TRICKS) && 1// eg PYM/ST-CNX
  t=LINECYCLE0+164; // look for R2 <= 164

#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
    if(MMU.WakeUpState2())
      t+=2;
#endif

  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
     i=CheckShiftMode(t);
     if(i>=0 && shifter_shift_mode_change[i]==2 && ShiftModeChangeCycle(i)>=152/*140*/)
       CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;

#ifdef SS_DEBUG
     r2cycle=ShiftModeChangeAtCycle(160);     // Superior160 164 (LOL)
     if( (CurrentScanline.Tricks&TRICK_LINE_MINUS_106) && r2cycle!=2)
     {
       TRACE_LOG("Line -106 R2 cycle %d\n",shifter_shift_mode_change_time[i]-LINECYCLE0);
//       CurrentScanline.Tricks&=~TRICK_LINE_MINUS_106;
     }


#endif

  }
#endif

  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_106)
    && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
    overscan_add_extra+=-106;
    TrickExecuted|=TRICK_LINE_MINUS_106;
    CurrentScanline.Bytes+=-106;
    right_border_changed=true;
  }

 
  /////////////
  // LINE -2 //
  /////////////

/*  
    It's a -2 line if it's at 60hz at cycle 372, the switch doesn't
    need to happen at 372, though it makes more sense there. Sooner, there
    could be distortion on a real ST.
*/

  if( shifter_freq_at_start_of_vbl!=50)
    return; 

  // Steem test
  t=LINECYCLE0+372; //trigger point for early right border
/*
#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
    if(MMU.WakeUpState2())
      t+=2;
#endif
*/

  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_2))
  {
    i=CheckFreq(t);
    if(i==-1)
      return;                       // vv looks obvious but...
    if(shifter_freq_change[i]==60 && CurrentScanline.StartCycle!=52)
      CurrentScanline.Tricks|=TRICK_LINE_MINUS_2;
  }

#if defined(SS_VID_TRACE_LOG_SUSPICIOUS2)
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)&&!(shifter_freq_change_time[i]-LINECYCLE0>56))
    TRACE_LOG("F%d Suspicious -2 y %d tmg sw %d tmg hbl %d diff %d\n",FRAME,scan_y,shifter_freq_change_time[i],LINECYCLE0,shifter_freq_change_time[i]-LINECYCLE0);
#endif

  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
    &&!(TrickExecuted&TRICK_LINE_MINUS_2))
  {
    overscan_add_extra+=-2;
    CurrentScanline.Bytes+=-2;
    TrickExecuted|=TRICK_LINE_MINUS_2;
    right_border_changed=true;
//    TRACE_LOG("-2 y %d c %d s %d e %d ea %d\n",scan_y,LINECYCLES,scanline_drawn_so_far,overscan_add_extra,ExtraAdded);
  }


  // TODO right off for 60hz...



  /////////////////////////////////
  // RIGHT BORDER OFF (line +44) // 
  /////////////////////////////////

/* 
    1) The position to remove right border  is cycle 376 (94x4) in our
    reckoning, 374 is OK on many machines.
    2) The number of bonus bytes (44) is quite bigger than for the left
    border (26). 
    3) People talk of 416 wide res (208x2), not 460 (230x2).
    4) Cycle precision is absolutely necessary.
    
    Paulo:
    On the horizontal side we have in fact only 416 pixels displayed in 416 
    CPU clock cycles: 
    - 48 left border pixels 
    - 320 normal pixels 
    - 48 rigth border pixels 
    The remaining 512-416 = 96 CPU clock cycles are used for the Horizontal 
    Blank. 

    4) Fetching ends at 460 (230x2), not 464 (376+2x44).
    5) Steem already handled 71/50 switch to remove right border.
*/
  t=LINECYCLE0+378; //trigger point for right border cut

#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
    if(MMU.WakeUpState2())
      t+=4; // beeshift0 vs omega
#endif

  if(act-t>=0
    && !(CurrentScanline.Tricks&TRICK_LINE_MINUS_2))
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

#if defined(SS_MMU_WAKE_UP_RIGHT_BORDER) && !defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
      int threshold= (MMU.WakeUpState2()) ? 376 : 374;
#elif defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
      int threshold= (WAKE_UP_STATE) ? 376 : 374; // taking care of 'ignore'
#else
      int threshold=374;
#endif

      if(shifter_freq_change[i]!=50 
#if defined(SS_SHIFTER_TRICKS)
        && FreqAtCycle(threshold)==50
#endif
        && shifter_freq_change_time[i]-LINECYCLE0>372 
        )
      {

#if defined(SS_SHIFTER_OMEGA)
/*  Wrong cycle! It really hits at 372 but Steem is confused by the 508 cycles
    lines. Hatari hasn't that problem. It's hard to fix.TODO
*/
        if(SSE_HACKS_ON && CurrentScanline.Cycles==508
          && FreqChangeAtCycle(376)==60)
        {
          TRACE_LOG("Spurious right off\n");
        }
        else
#endif
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_44;
      }
    }
  }

  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_44)
    && !(TrickExecuted&TRICK_LINE_PLUS_44))
  {
    ASSERT(!(CurrentScanline.Tricks&TRICK_LINE_MINUS_2));
    right_border=0;
    overscan_add_extra+=OVERSCAN_ADD_EXTRA_FOR_RIGHT_BORDER_REMOVAL;  // 28 (+16=44)
    TrickExecuted|=TRICK_LINE_PLUS_44;
#if defined(SS_VID_BORDERS)
    if(SideBorderSize==VERY_LARGE_BORDER_SIDE)
      overscan_add_extra-=BORDER_EXTRA/2; // 20 + 24=44
#endif
    CurrentScanline.Bytes+=44;
    CurrentScanline.EndCycle=460; //or 464?
    overscan=OVERSCAN_MAX_COUNTDOWN; // 25
    right_border_changed=true;
#ifdef SS_DEBUG___
    if(scan_y==-29)
    {
      TRACE_LOG("Steem detects right off at %d\n",shifter_freq_change_time[i]-LINECYCLE0);
      VideoEvents.ReportLine();
    }
#endif
  }

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
    504 on a STF. So there's overlap, just checking frequency at cycle c is 
    not enough.
    Those values are not based on measurements but on how programs work or not.
    Still testing.
    STE cases:
    -E605 planet (500:S0)
    -RGBeast (480:S0, sometimes 500:S2)
    STF cases:
    -3615Gen4 Cakeman
    -Auto 168 
    with HBL at 444+4+8!:
    Line -30 - 488:S0000 512:S0002 512:T0100
    Line -30 - 492:S0000 516:S0002 512:T0100

    with HBL at 444+4:
    Line -30 - 504:S0000 512:T0100
    missed
    Line -30 - 508:S0000
    Line -29 - 020:S0002 264:j0006
    No doubt 508 is too high, so yes it's VBI problem

    -European Demos (top 444 60) When it misses there's just no 60 (VBI?)
    -Musical Wonder 90 (416:S0,504:S2)
    -SNYD2/Sync Vectorballs II 424:S0000 504:S0002 (every time)
    "the low border of Sync's vectorballs was never stable on my machine"
    This means the MMU didn't react the same to same value in every frame.

    5) See No Cooper greeting, trying a switch at end of line 182, then end
    of line 199. 199-182 = 17?

    6) I know no cases of programs making the screen smaller. It was small
    enough! TODO PYM big border?
*/

  enum{NO_LIMIT=0,LIMIT_TOP,LIMIT_BOTTOM};

  BYTE on_overscan_limit=NO_LIMIT;
  if(scan_y==-30)
    on_overscan_limit=LIMIT_TOP;
  else if(scan_y==shifter_last_draw_line-1 && scan_y<245)
    on_overscan_limit=LIMIT_BOTTOM;

  int t,i=shifter_freq_change_idx;
  int freq_at_trigger=0; 
  if(screen_res==2) 
    freq_at_trigger=MONO_HZ;
  if(emudetect_overscans_fixed) 
    freq_at_trigger=(on_overscan_limit) ? 60:0;

#if defined(SS_SHIFTER_STEEM_ORIGINAL) || defined(SS_VID_VERT_OVSCN_OLD_STEEM_WAY)
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50 
    && freq_change_this_scanline)
  {
    t=cpu_timer_at_start_of_hbl+CYCLES_FROM_HBL_TO_RIGHT_BORDER_CLOSE+98; // 502
    i=CheckFreq(t); 
    freq_at_trigger=(i>-1) ? shifter_freq_change[i] : 0;
  }
#endif

#if defined(SS_SHIFTER_TRICKS) && !defined(SS_VID_VERT_OVSCN_OLD_STEEM_WAY)
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50)
  {
    t=VERT_OVSCN_LIMIT; //502
#if defined(SS_STF) && defined(SS_STF_VERTICAL_OVERSCAN)
    if(ST_TYPE!=STE)
      t+=STF_VERT_OVSCN_OFFSET; //4
#endif
    
    if(FreqAtCycle(t)==60 
#if defined(SS_SHIFTER_STE_VERTICAL_OVERSCAN) 
      || FreqAtCycle(t-2)==60 && FreqChangeAtCycle(t-2)==50 // fixes RGBeast
#endif
      )
      freq_at_trigger=60;
  }
#if defined(SS_SHIFTER_60HZ_OVERSCAN) 
/*  Removing lower border at 60hz. Simpler test, not many cases.
    Fixes Leavin' Terramis in monitor mode.
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
      + TB_TIME_WOBBLE; //TODO other possible timing for timer B
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

#if defined(SS_SHIFTER_VERTICAL_OVERSCAN_TRACE_LOG)
  if(on_overscan_limit) 
    TRACE_LOG("F%d y%d freq at %d %d at %d %d switch %d to %d, %d to %d, %d to %d overscan %X\n",FRAME,scan_y,t,FreqAtCycle(t),t-2,FreqAtCycle(t-2),PreviousFreqChange(PreviousFreqChange(t)),FreqChangeAtCycle(PreviousFreqChange(PreviousFreqChange(t))),PreviousFreqChange(t),FreqChangeAtCycle(PreviousFreqChange(t)),NextFreqChange(t),FreqChangeAtCycle(NextFreqChange(t)),(bool)(CurrentScanline.Tricks&TRICK_VERTICAL_OVERSCAN));
#endif
}


void TShifter::DrawScanlineToEnd()  { // such a monster wouldn't be inlined
#ifdef SS_SHIFTER_DRAW_DBG
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
#if defined(SS_SHIFTER_STE_MED_HSCROLL) 
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
      nsdp=shifter_draw_pointer+80;
      shifter_pixel=HSCROLL; //start by drawing this pixel
      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
      {
        if(border & 1)
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline((BORDER_SIDE*2)/16, 640/16, (BORDER_SIDE*2)/16,0);
        else
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline(0,640/16,0,0);
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }

      shifter_draw_pointer=nsdp;
    }
    else if(scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border)
    {
      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
      {
        if(border & 1)
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline((BORDER_SIDE*2+640+BORDER_SIDE*2)/16,0,0,0); // rasters!
        else
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline(640/16,0,0,0);
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }
    }
    scanline_drawn_so_far=BORDER_SIDE+320+BORDER_SIDE; //border1+picture+border2;
  }//end Monochrome
}


void TShifter::EndHBL() {

#if defined(SS_SHIFTER_END_OF_LINE_CORRECTION)

/*  Those tests are much like EndHBL in Hatari
    Finish horizontal overscan : correct -2 & +2 effects
    Check shifter stability (preliminary)
*/
  
  if(CurrentScanline.Tricks&TRICK_LINE_PLUS_2 && CurrentScanline.EndCycle==372)     
  {
    CurrentScanline.Tricks&=~TRICK_LINE_PLUS_2;
    shifter_draw_pointer-=2; // eg SNYD/TCB at scan_y -29
    TRACE_LOG("scan_y %d cancel +2\n",scan_y);
  } // no 'else', they're false alerts!

  if(CurrentScanline.Tricks&TRICK_LINE_MINUS_2     
    && (CurrentScanline.StartCycle==52 || CurrentScanline.EndCycle!=372))
  {
    CurrentScanline.Tricks&=~TRICK_LINE_MINUS_2;
    shifter_draw_pointer+=2;
    TRACE_LOG("scan_y %d cancel -2\n",scan_y);
  }
#endif//#if defined(SS_SHIFTER_END_OF_LINE_CORRECTION)

#if defined(SS_SHIFTER_DRAGON1) && defined(SS_STF) && defined(SS_MMU_WAKE_UP)
  // not defined in v3.5.2
  if(SSE_HACKS_ON && ST_TYPE==STF && WAKE_UP_STATE==2 
    && CurrentScanline.Tricks==1) 
    SS_signal=SS_SIGNAL_SHIFTER_CONFUSED_1; // stage 1 of our hack...
  else if(CurrentScanline.Tricks&&CurrentScanline.Tricks!=0x10
    && (CurrentScanline.Tricks!=TRICK_CONFUSED_SHIFTER))
    SS_signal=0;
#endif

#if defined(SS_SHIFTER_UNSTABLE)

/*  3.5.2 This way is more generic. It tries to make sense of #words loaded
    in the shifter after the "unstable" trick.
    Left off: 160+26=186 = (23*8)+2 -> 1 word preloaded
    Left off, right off: 160+26+44=(28*8)+6 -> 3 words preloaded
    See doc by ST-CNX and LJBK's efforts at AF
    3.5.3
    In which WU state it should work isn't clear. TODO
*/

  if( ST_TYPE==STF && WAKE_UP_STATE)//==1)
  {
    // Overdrive/Dragon
    if(CurrentScanline.Tricks==TRICK_LINE_PLUS_26)
      Preload=1;
    // Death of the Left Border, Omega Full Overscan
    else if( (CurrentScanline.Tricks&(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_44))
      ==(TRICK_LINE_PLUS_26|TRICK_LINE_PLUS_44)
      &&!(CurrentScanline.Tricks&TRICK_STABILISER)) 
      Preload=3; // becomes 2 at first left off
  }

  if(CurrentScanline.Cycles==508 && FetchingLine()
    && FreqAtCycle(0)==60 && FreqAtCycle(464)==60)
  {
    Preload=0; // a full 60hz scanline should reset the shifter
  }
  
#endif

}


int TShifter::IncScanline() { // a big extension of 'scan_y++'!

#if defined(SS_SHIFTER_EVENTS) 
  // Record the 'tricks' mask at the end of scanline
  if(CurrentScanline.Tricks)
  {
    VideoEvents.Add(scan_y,CurrentScanline.Cycles,'T',CurrentScanline.Tricks);
//    VideoEvents.Add(scan_y,CurrentScanline.Cycles,'#',CurrentScanline.Bytes);
  }
#endif

#if defined(SS_DEBUG)
  Debug.ShifterTricks|=CurrentScanline.Tricks;
#endif

  scan_y++; 
  
  left_border=BORDER_SIDE;
  if(HSCROLL) 
    left_border+=16;
  if(shifter_hscroll_extra_fetch) 
    left_border-=16;
  right_border=BORDER_SIDE;
#if defined(SS_DEBUG) && !defined(SS_SHIFTER_DRAW_DBG)
  if( scan_y-1>=shifter_first_draw_line && scan_y+1<shifter_last_draw_line
    && (overscan_add_extra || !ExtraAdded) && screen_res<2)
    TRACE_LOG("F%d y%d Extra %d added %d\n",FRAME,scan_y-1,overscan_add_extra,ExtraAdded);
#endif
//        AddExtraToShifterDrawPointerAtEndOfLine(shifter_draw_pointer); //?
  ExtraAdded=overscan_add_extra=0;
#if defined(SS_SHIFTER_SDP_WRITE)
  SDPMiddleByte=999; // an impossible value for a byte
#endif
  PreviousScanline=CurrentScanline; // auto-generated
  CurrentScanline=NextScanline;
  if(scan_y==-29 && (PreviousScanline.Tricks&TRICK_TOP_OVERSCAN)
    || !scan_y && !PreviousScanline.Bytes)
    CurrentScanline.Bytes=160; // needed by ReadSDP - not perfect (TODO)
  else if(FetchingLine()) 
    NextScanline.Bytes=(screen_res==2)?80:160;
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
  }
  TrickExecuted=0;

  CurrentScanline.Tricks=0;

  // In the STE if you make hscroll non-zero in the normal way then the shifter
  // buffers 2 rasters ahead. We don't do this so to make sdp correct at the
  // end of the line we must add a raster.  
  shifter_skip_raster_for_hscroll = HSCROLL!=0;//SS correct place?
  return scan_y; // fancy
}

/*  For simplification, GLUE/MMU/Shifter IO is considered just shifter for what
    concerns the ST video circuits.
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
#if defined(SS_STF)
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
#if defined(SS_SHIFTER_SDP_READ)
      sdp=ReadSDP(LINECYCLES); // a complicated affair
#else
      if(scan_y<shifter_first_draw_line || scan_y>=shifter_last_draw_line){
        sdp=shifter_draw_pointer;
      }else{
        sdp=get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
        LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+
          " - Read shifter draw pointer as $"+HEXSl(sdp,6)+
          " on "+scanline_cycle_log()); )
      }
#endif
      ior_byte=DWORD_B(&sdp,(2-(addr-0xff8205)/2)); // change for big endian !!!!!!!!!
#if defined(SS_SHIFTER_EVENTS) && defined(SS_SHIFTER_EVENTS_READ_SDP)
      VideoEvents.Add(scan_y,LINECYCLES,'r',((addr&0xF)<<8)|ior_byte);
#endif
      }
      break;
      
    case 0xff820a:  //synchronization mode
      ior_byte&=~3;           // this way takes care
      ior_byte|=m_SyncMode;   // of both STF & STE
      break;

    case 0xff820d:  //low byte of screen memory address
      ASSERT(!(xbios2&1));
#if defined(SS_STF)
      ASSERT( ST_TYPE==STE || !(xbios2&0xFF) );
      if(ST_TYPE==STE) 
#endif
        ior_byte=(BYTE)xbios2&0xFF;
      break;
  
    case 0xff820f: // LINEWID
#if defined(SS_STF)
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
#if defined(SS_STF)
      if(ST_TYPE==STE)
#endif
        ior_byte=HSCROLL;
      break;
    }//if
    
  }
#if defined(SS_SHIFTER_IOR_TRACE)
  // made possible by our structure change
  TRACE("Shifter read %X=%X\n",addr,ior_byte);
#endif
  return ior_byte;
}


void TShifter::IOWrite(MEM_ADDRESS addr,BYTE io_src_b) {

  ASSERT( (addr&0xFFFF00)==0xff8200 );

#if defined(SS_SHIFTER_IOW_TRACE)
  TRACE("(%d/%d) Shifter write %X=%X\n",scan_y,LINECYCLES,addr,io_src_b);
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
    
#if defined(SS_SHIFTER_EVENTS) && defined(SS_SHIFTER_EVENTS_PAL)
    VideoEvents.Add(scan_y,LINECYCLES,'p', (n<<12)|io_src_b);  // little p
#endif
    
#if defined(SS_SHIFTER_PALETTE_BYTE_CHANGE) 
    // TESTING maybe Steem was right, Hatari is wrong
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

According to ST-CNX, those registers are in the MMU, not in the shifter.

 STE doc by Paranoid: for compatibility reasons, the low-byte
 of the Video Base Address is ALWAYS set to 0 when the mid- or high-byte of
 the Video Base Address are set. 
 E.g.: Leavin' Terramis

*/
     
    case 0xff8201:  //high byte of screen memory address
#if defined(SS_SHIFTER_EVENTS)
      VideoEvents.Add(scan_y,LINECYCLES,'V',io_src_b); 
#endif
      // asserts on SoWatt, Leavin' Terramis, High Fidelity Dreams
      // ...
//      ASSERT( mem_len>FOUR_MEGS || !(io_src_b&(~b00111111)) ); 
      if (mem_len<=FOUR_MEGS) 
        io_src_b&=b00111111;
      DWORD_B_2(&xbios2)=io_src_b;
#if defined(SS_STF)
      if(ST_TYPE==STE) 
#endif
        DWORD_B_0(&xbios2)=0; 
      log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
      break;
      
    case 0xff8203:  //mid byte of screen memory address
#if defined(SS_SHIFTER_EVENTS)
      VideoEvents.Add(scan_y,LINECYCLES,'M',io_src_b); 
#endif
      DWORD_B_1(&xbios2)=io_src_b;

#if defined(SS_STF)
      if(ST_TYPE==STE) 
#endif
        DWORD_B_0(&xbios2)=0; 
      log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
      break;
      
    case 0xff8205:  //high byte of draw pointer
    case 0xff8207:  //mid byte of draw pointer
    case 0xff8209:  //low byte of draw pointer
      
#if defined(SS_SHIFTER_SDP_WRITE)
      WriteSDP(addr,io_src_b); // very complicated!
      break;
      
#else // Steem 3.2 or SS_SHIFTER_SDP_WRITE not defined
      {
        //          int srp=scanline_raster_position();
        int dst=ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl;
        dst-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
        dst+=16;dst&=-16;
        dst+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
#if defined(SS_SHIFTER) // video defined but not SDP
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
        
        log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+" - Set shifter draw pointer to "+
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

Last bit always cleared.
*/
    case 0xff820d:  //low byte of screen memory address
#if defined(SS_SHIFTER_EVENTS)
      VideoEvents.Add(scan_y,LINECYCLES,'v',io_src_b); 
#endif
#if defined(SS_STF)
      if(ST_TYPE!=STE)
      {
        TRACE_LOG("STF write %X to %X\n",io_src_b,addr);
        break; // fixes Lemmings 40; used by Beyond/Pax Plax Parallax
      }
#endif

      ASSERT(!(io_src_b&1)); //E605
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
#if defined(SS_SHIFTER_EVENTS)
      VideoEvents.Add(scan_y,LINECYCLES,'F',io_src_b); 
#endif
#if defined(SS_STF)
      if(ST_TYPE!=STE)
      {
        TRACE_LOG("STF write %X to %X\n",io_src_b,addr);
        break; // fixes Imagination/Roller Coaster mush
      }
#endif
      
#if defined(SS_SHIFTER) 
      Render(LINECYCLES,DISPATCHER_LINEWIDTH); // eg Beat Demo
#else
      draw_scanline_to(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl); // Update sdp if off right  
#endif
      
#if defined(SS_SHIFTER_SDP_TRACE_LOG)
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
      // to 0 confuses the shifter and causes it to shrink the left border by 16 pixels.
    case 0xff8265:  // Hscroll
#if defined(SS_SHIFTER_EVENTS)
      VideoEvents.Add(scan_y,LINECYCLES,(addr==0xff8264)?'h':'H',io_src_b); 
#endif
#if defined(SS_STF)
      if(ST_TYPE!=STE) 
      {
        TRACE_LOG("STF write %X to %X\n",io_src_b,addr);
        break; // fixes Hyperforce
      }
      else
#endif
      {
        int cycles_in=(int)(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
        
#if defined(SS_SHIFTER_SDP_TRACE_LOG)
        TRACE_LOG("F%d y%d c%d HS %d -> %d\n",FRAME,scan_y,LINECYCLES,HSCROLL,io_src_b);
#endif
        
        HSCROLL=io_src_b & 0xf; // limited to 4bits
        log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set horizontal scroll ("+HEXSl(addr,6)+
          ") to "+(shifter_hscroll)+" at "+scanline_cycle_log());
        if (addr==0xff8265) shifter_hscroll_extra_fetch=(HSCROLL!=0); //OK
        
        
#if defined(SS_VID_BORDERS)
        if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-BORDER_SIDE){
#else
        if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-32){
#endif           // eg Coreflakes hidden screen
          if (left_border>0){ // Don't do this if left border removed!
            shifter_skip_raster_for_hscroll = HSCROLL!=0; //SS computed at end of line anyway
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
#if defined(SS_SHIFTER_DRAW_DBG)
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
#if defined(SS_SHIFTER_DRAGON1)//temp
    || SS_signal==SS_SIGNAL_SHIFTER_CONFUSED_2
#endif
#if defined(SS_SHIFTER_UNSTABLE)
    || Preload // must go apply trick at each scanline
#endif
    )
    CheckSideOverscan(); 

/*  What happens here is very confusing; we render in real time, but not
    quite. As on a real ST, there's a delay of fetch+8 between the 
    'shifter cycles' and the 'palette reading' cycles. 
    We can't render at once because the palette could change. 
    CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN (84) takes care of that delay. 
    This causes all sort of trouble because our SDP is late while rendering,
    and sometimes forward!
*/

#if defined(SHIFTER_PREFETCH_LATENCY)
  // this may look impressive but it's just a bunch of hacks!
  switch(dispatcher) {
  case DISPATCHER_CPU:
    cycles_since_hbl+=16; // 3615 Gen4 by ULM, override normal delay
    break;
  case DISPATCHER_DSTE:
    break;
  case DISPATCHER_SET_PAL:
#if defined(SS_SHIFTER_PALETTE_TIMING)
#if defined(SS_MMU_WAKE_UP_PALETTE_STE)
    if(!(
#if defined(SS_STF)
      ST_TYPE==STE &&
#endif
      WAKE_UP_STATE==1))
#endif
      cycles_since_hbl++; // eg Overscan Demos #6, already in v3.2 TODO why?
#endif
    break;
  case DISPATCHER_SET_SHIFT_MODE:
    RoundCycles(cycles_since_hbl); // eg Drag/Happy Islands, Cool STE
    break; 
/*
  case DISPATCHER_SET_SYNC: //not used
    RoundCycles(cycles_since_hbl); 
    cycles_since_hbl++;
    break;
*/
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
        && scan_y<draw_last_scanline_for_border)
      {
        if(draw_store_dest_ad==NULL 
          && pixels_in<BORDER_SIDE+320+BORDER_SIDE)
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
      if(left_border<0) // shouldn't happen
      {
        BRK( LB<0 leaving render ) ; //test if should remove...
        TRACE_LOG("LB<0, leaving render\n");
        return; // may avoid crash
      }
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
          border1=left_border-scanline_drawn_so_far;
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

      if(!screen_res) // LOW RES
      {
        hscroll=old_shifter_pixel & 15;
        nsdp-=(old_shifter_pixel/16)*8;
        nsdp+=(shifter_pixel/16)*8; // is sdp forward at end of line?
        
#if defined(SS_SHIFTER_TRICKS) && defined(SS_SHIFTER_4BIT_SCROLL)
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
#if defined(SS_SHIFTER_LINE_PLUS_20_SHIFT)
          || (CurrentScanline.Tricks&TRICK_LINE_PLUS_20)
#endif
#if defined(SS_SHIFTER_UNSTABLE)
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

#if defined(SS_SHIFTER_STE_MED_HSCROLL)
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
          && scan_y<draw_last_possible_line)
        {
          // actually draw it
          if(picture_left_edge<0) 
            picture+=picture_left_edge;
          AUTO_BORDER_ADJUST; // hack borders if necessary
          DEBUG_ONLY( shifter_draw_pointer+=debug_screen_shift; );
          if(hscroll>=16) // convert excess hscroll in SDP shift
          {
#if defined(SS_DEBUG)
            if(scan_y==120) TRACE_LOG("y %d hscroll OVL\n",scan_y);
#endif
            shifter_draw_pointer+=(SHIFTER_RASTER_PREFETCH_TIMING/2)
              *(hscroll/16); // ST-CNX large border
            hscroll-=16*(hscroll/16);
          }
          
#if defined(SS_DEBUG)
#ifdef SS_SHIFTER_SKIP_SCANLINE
          if(scan_y==SS_SHIFTER_SKIP_SCANLINE) 
            border1+=picture,picture=0;
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

      // adjust SDP according to shifter tricks
      if(!ExtraAdded // only once - kind of silly variable
        && ( dispatcher==DISPATCHER_DSTE || 
         dispatcher!=DISPATCHER_WRITE_SDP  && 
         pixels_in>=picture_right_edge 
        && scanline_drawn_so_far<picture_right_edge
#if defined(SS_SHIFTER_SDP_WRITE_ADD_EXTRA)
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
      ///////////////// RENDER VIDEO /////////////////        
        draw_scanline(border1,0,0,0); // see SS_VID_ADJUST_DRAWING_ZONE1
      }
    }
    scanline_drawn_so_far=pixels_in;
  }//if(pixels_in>=0)
}


void TShifter::Reset(bool Cold) {
  m_SyncMode=0; 
  m_ShiftMode=screen_res; // we know it's updated first, debug strange screen!
#if defined(SS_DEBUG)
  nVbl=0; // cold or warm
#endif
#if defined(SS_SHIFTER_TRICKS)
  for(int i=0;i<32;i++)
  {
    shifter_freq_change[i]=0; // interference Leavin' Terramis/Flood on PP43
    shifter_shift_mode_change_time[i]=-1;
  }
#endif

#if defined(SS_DEBUG)
  if(Cold)
    Debug.ShifterTricks=0;
#endif

#if defined(SS_SHIFTER_UNSTABLE)
  if(Cold)
    Preload=0;
#endif

}

/*  SetShiftMode() and SetSyncMode() are called when a program writes
    on "shifter" registers FF8260 (shift) or FF820A (sync). 
    Most shifter tricks use those registers. 
    If performance is an issue, the analysis currently in CheckSideOverscan()
    could be moved here, removing the need for look-up functions, 
    but introducing other problems (are we on line, line-1, line+1?)
    According to ST-CNX, those registers are in the GLUE.
    The GLUE is responsible for sync signals, for "DE". Most shifter tricks
    actually are GLUE tricks.
    
    But generally, video registers are considered to be in the shifter by
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

  FF8260 is both in the GLUE and the shifter. It is needed in the GLUE
  because sync signals are different in mode 2 (70hz).
  It is needed in the shifter because it needs to know in how many bit planes
  memory has to be decoded, and where it must send the video signal (RGB, 
  Mono).
    
  In monochrome, frequency is 70hz, a line is transmitted in 28µs.
  There are +- 500 scanlines.
  We see no reason to forbid writing 3 without crash, but how it is
  handled by the shifter is unknown. Awesome 16 should work in STF mode
  anyway.
*/

  int CyclesIn=LINECYCLES;
#if defined(SS_SHIFTER_EVENTS)
  VideoEvents.Add(scan_y,CyclesIn,'R',NewMode); 
  //TRACE_LOG("y%d c%d r%d\n",scan_y,CyclesIn,NewMode);
#endif
  NewMode&=3; // only two lines would physically exist
  m_ShiftMode=NewMode; // update, used by ior now (v3.5.1)

  if(screen_res>=2|| emudetect_falcon_mode!=EMUD_FALC_MODE_OFF)
    return; // if not, bad display in high resolution

#ifndef NO_CRAZY_MONITOR
  if(extended_monitor)
  {
    screen_res=(BYTE)(NewMode & 1);
    return;
  }
#endif

  // From here, we have a colour display:
  ASSERT( mfp_gpip_no_interrupt & MFP_GPIP_COLOUR );
  if(NewMode==3) 
    NewMode=1; // or 2? TESTING

#if defined(SS_SHIFTER_TRICKS) // before rendering
  AddShiftModeChange(NewMode); // add time & mode
#endif

  Render(CyclesIn,DISPATCHER_SET_SHIFT_MODE);

#if defined(SS_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE)
  AddFreqChange( (NewMode==2 ? MONO_HZ : shifter_freq) );
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

#if defined(SS_SHIFTER_EVENTS) 
  VideoEvents.Add(scan_y,CyclesIn,'S',NewSync); 
//  TRACE_LOG("y%d c%d s%d\n",scan_y,CyclesIn,NewSync);
#endif

  int new_freq;  

  m_SyncMode=NewSync&3;

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
    const int limit512=   // TO CHECK
#if defined(SS_STF) && defined (SS_MMU_WAKE_UP)
    (
#if !defined(SS_MMU_WAKE_UP_STE)
      ST_TYPE!=STE && 
#endif
      WAKE_UP_STATE==2) ? 56+2 :
#endif
      54+2;
    if(CyclesIn<=limit512)
      CurrentScanline.Cycles=scanline_time_in_cpu_cycles[shifter_freq_idx];
    if(FetchingLine())
    {
      if(CyclesIn<=52 && left_border)
      {
        CurrentScanline.StartCycle=56;
     ////   CurrentScanline.Cycles=512;
      }
      if(CyclesIn<=372)
        CurrentScanline.EndCycle=376;
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
      {
        CurrentScanline.StartCycle=52; 
      ////  CurrentScanline.Cycles=508;
      }
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
    freq_change_this_scanline=TRUE;  
  shifter_freq=new_freq;
  
#if defined(SS_SHIFTER_TRICKS)
  Shifter.AddFreqChange(new_freq);
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
#if defined(SS_SHIFTER_EVENTS)
  VideoEvents.Vbl(); 
#endif
//  m_nHbls=HBL_PER_FRAME;
#if defined(SS_DEBUG)
  nVbl++; 
#endif
#if defined(SS_SHIFTER_UNSTABLE)
  HblPixelShift=0;
#endif
}

#undef LOGSECTION
#endif//#if defined(SS_SHIFTER)
