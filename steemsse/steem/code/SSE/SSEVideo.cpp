#if defined(SS_STRUCTURE)
#pragma message("Included for compilation: SSEVideo.cpp")
#endif

#if defined(SS_VIDEO)

TShifter Shifter; // singleton

#if defined(SS_VID_BORDERS)
int SideBorderSize=ORIGINAL_BORDER_SIDE; // 32
int SideBorderSizeWin=ORIGINAL_BORDER_SIDE;
int BottomBorderSize=ORIGINAL_BORDER_BOTTOM; // 40
#endif

#if defined(SS_VID_SHIFTER_EVENTS)
TVideoEvents VideoEvents;  // singleton
#endif


/////////////////////////////
// Video - object TShifter //
/////////////////////////////

#define LOGSECTION LOGSECTION_VIDEO

TShifter::TShifter() {
#if defined(WIN32)
  ScanlineBuffer=NULL;
#endif
#if defined(SS_DEBUG)
  nVbl=0;
#endif
}


TShifter::~TShifter() {
}

/*  Check overscan functions. Those are based on existing Steem code, but much 
    inflated, for a good cause: now Steem handles a lot more shifter tricks
    than before.
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
*/
#if defined(SS_DEBUG) && defined(SS_VIDEO_DRAW_DBG)
  draw_check_border_removal(); // base function in Steem 3.2
  return;
#endif
 
  int act=ABSOLUTE_CPU_TIME,t,i;
  int CyclesIn=LINECYCLES;
  if(screen_res>=2
    || !FetchingLine() 
    || !act) // if ST is resetting
    return;

#if defined(SS_VID_STEEM_EXTENDED) 
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

#if defined(SS_VID_STEEM_EXTENDED) && defined(SS_VID_0BYTE_LINE)

#if defined(SS_VID_0BYTE_LINE_RES_END)

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

#if defined(SS_VID_0BYTE_LINE_RES_HBL) && defined(SS_STF)

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

#if defined(SS_VID_0BYTE_LINE_RES_START)

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
  
#if defined(SS_VID_0BYTE_LINE_SYNC) 

/*  
    ST-CNX

       * 0 Byte line by switching to 60Hz at position 14 (detection of
        the beginning of a line at 50Hz). This method is not recomended,
        because it doesn't work once out of every twenty times the ST is
        powered on. This happens when the timing of the GLUE and the MMU
        are offset in such a manner that the 68000 cannot access the bus
        on the precise cycle required to change the frequency. Recall
        that it is the MMU that decides which of every four cycles will
        be allocated to the 68000, and that at on power on circuits can
        receive parasitic noise on the clock signal that could
        potentially cause this offset.

    SS: 0-byte sync switches are model-dependent and even "wake up state"-
    dependent. They  are used only in a 2006 demo called SHFORSTV.TOS,
    by the dude who also made the Overscan Demos.
    Switche: STF(1) (56/64), STF(2) (58/68), and STE (40/52).
*/

  if(!(CurrentScanline.Tricks&TRICK_0BYTE_LINE))
  {
#if defined(SS_STF) // determine which switches we're looking for
    switch(ST_TYPE)
    {
    case MEGASTF: 
    case STF: 
      s0cycle=56;
      s2cycle=64;
      break;
#if defined(SS_STF_WAKE_UP)  
    case STF2: // (wake up state 2 = Hatari, Steem 3.3)
      s0cycle=58;
      s2cycle=68;
      break;
#endif
    case STE:
#endif
      s0cycle=40;
      s2cycle=52; // this also produces a 512 cycles +2 line?
#if defined(SS_STF)
      break;
    }//sw
#endif
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
#if defined(SS_VID_0BYTE_LINE_TRACE)
      TRACE("%d 0byte line\n",scan_y);
#endif
    }
    return; // 0byte line takes no other tricks
  }

#endif

#if defined(SS_VID_DRAGON)
    if(SS_signal==SS_SIGNAL_SHIFTER_CONFUSED_2
      && !(CurrentScanline.Tricks&TRICK_CONFUSED_SHIFTER))
    {
      CurrentScanline.Tricks|=TRICK_CONFUSED_SHIFTER;
      shifter_draw_pointer+=-2,overscan_add_extra+=2;
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

  if(left_border && CyclesIn<512)
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

#if defined(SS_VID_STEEM_EXTENDED) && defined(SS_VID_STE_LEFT_OFF)

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
    Flix:
    For  a  long time it was considered impossible to open  the  left 
    border,  because the trick with the 60-hertz-switch did not  work 
    in the left border (I was sure about that as well). But than came 
    the  "Death  Of The Left Border"-Demo by  the  TNT-Crew.  Finally 
    there  was a screen in the legendary Union-Demo that  firstly  in 
    ST-history  displayed  no borders at  all:  The  first  so-called 
    "Fullscreen"!  The demo-coders made the impossible possible.  But 
    how  does  this  trick  work?  It's  pretty  simple:  Instead  of 
    switching  to  60  Hz you switch to 71 Hz!  If  you  disable  the 
    interrupt  that  resets  at 71 Hz,  you can switch to  71  Hz  in 
    coulour mode.  You should do this only for short periods of  time 
    (a few clock-cycles).  Being in 71 Hz for a long time can  damage 
    your  monitor (allthough I've never seen such damages).  It  goes 
    without  saying  that neither Maggie nor the "Delta  Force"  take 
    responsibility for damages caused by this technique.  If you  now 
    open the right border additionally,  the ST displays 230  instead 
    of  160  bytes  per line!  

    ST-CNX:
        Thus we have obtained a right overscan, but have no clue as to
        how to remove the left border. It is obvious that the GLUE
        pseudo-code given so far cannot help us in any way. Therefore we
        need to look at the other available register settings. As we
        discouraged any use of external synchronisation, all that is
        left is switching to high resolution mode. The structure of the
        screen is very different in hires than that we have looked at so
        far: each line contains only 80 bytes instead of 160, and each
        line only takes 28us (20us for the useable screen, and 8us for
        the border and Hsync) instead of 64us. Thus each monochrome line
        will have a smaller left border so that the useable line will
        start earlier, and as it only contains 80 bytes it will also
        finish earlier. Thus we need to add the following lines to our
        GLUE pseudo-code:

 IF Position == 0   AND display_freq=70  THEN Activate    signal H
 IF Position == 40  AND display_freq=70  THEN Disactivate signal H
 IF Position == Z   AND display_freq=70  THEN Activate    signal Hsync
 IF Position == Z+1 AND display_freq=70  THEN Disactivate signal Hsync
                                              and start a new line

 (Determining Z is of no value)

        It is therefore sufficient to switch to monochrome to activate H
        and DE, and therefore force the MMU and the SHIFTER to start
        decoding the useable screen. One returns to low or medium
        resolution to actually see the useable screen on the RGB pins.
        Thus one obtains at 50Hz lines of 160+26 = 186 bytes. At 60Hz
        one obtains lines of 184 bytes. The difference of 2 bytes
        corresponds to the difference of 0.5us between the two line
        lengths (63.5us at 60Hz and 64us at 50Hz).
    
  
    SS:
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
    Omega (cycle 8 is only apparent)
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

#if defined(SS_VID_STEEM_EXTENDED) 
    int lim_r2=2,lim_r0=26;    
#if defined(SS_STF) 
    if(ST_TYPE!=STE)
      lim_r2+=6,lim_r0+=2; // experimental
#endif
#endif

    if(!(CurrentScanline.Tricks&TRICK_LINE_PLUS_20))
    {
#if defined(SS_VID_STEEM_ORIGINAL) // in fact we could keep it? it's simpler
      t=cpu_timer_at_start_of_hbl+2+(ST_TYPE!=STE ? 4 : 0);
      if(act-t>0)
      {
        i=CheckFreq(t);
        if(i>=0 && shifter_freq_change[i]==MONO_HZ)
          CurrentScanline.Tricks|=TRICK_LINE_PLUS_26;
      }
#elif defined(SS_VID_STEEM_EXTENDED)
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
          TRACE("F%d y%d left off detected by Steem 3.2 R2 %d R0 %d\n",FRAME,scan_y,r2cycle,r0cycle);
        else if((i<0||shifter_freq_change[i]!=MONO_HZ)
          && CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
          TRACE("F%d y%d left off not detected by Steem 3.2 R2 %d R0 %d\n",FRAME,scan_y,r2cycle,r0cycle);
      }
#endif
#endif
    }
    
    // action for line+26 & line+20
    if( (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      || (CurrentScanline.Tricks&TRICK_LINE_PLUS_20))
    {
#if defined(SS_VID_STE_LEFT_OFF)
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
#if defined(SS_VID_STE_HSCROLL_LEFT_OFF)  
        if(shifter_skip_raster_for_hscroll) 
          overscan_add_extra+=6; // fixes MOLZ/Spiral
#endif
        TrickExecuted|=TRICK_LINE_PLUS_20;
      }
      else // normal left off
#endif
      {
        if(shifter_freq_at_start_of_vbl==50) 
        {
          shifter_pixel+=4;
          overscan_add_extra+=2;  // 8 + 2 + 16 = 26
        }
        CurrentScanline.Bytes+=26;
        if(shifter_hscroll>=12) // STE shifter bug (Steem authors)
        {   
          shifter_draw_pointer+=8; 
          overscan_add_extra-=8; // removed this in 3.3, was a mistake
        }
#if defined(SS_VID_BORDERS)
        if(SideBorderSize==ORIGINAL_BORDER_SIDE) // 32
#endif
          shifter_draw_pointer+=8; // 8+16+2=26
//        TRACE("+26 y %d px %d\n",scan_y,shifter_pixel);
        TrickExecuted|=TRICK_LINE_PLUS_26;

        // additional hacks for left off
        //////////////////////////////////

#if defined(SS_VID_UNSTABLE_LEFT_OFF) && defined(SS_VID_STEEM_EXTENDED) \
&& defined(SS_STF_WAKE_UP)
/*  Hack for Death of the Left Border, Omega Full Overscan (and?)
    no stabiliser, late switch, unstable overscan: shift +4
    'Hack' option not necessary, choosing wake up state 2 is enough!
 */
        if(r0cycle>=16 && ST_TYPE==STF2
          && !(PreviousScanline.Tricks&TRICK_STABILISER))
        {
//          ASSERT(!debug1);
            shifter_draw_pointer+=4,overscan_add_extra-=4;
            shifter_pixel+=-4;
        }
#endif

#if defined(SS_VID_BIG_WOBBLE) && defined(SS_VID_SDP_WRITE)
        if(SSE_HACKS_ON && (PreviousScanline.Tricks&TRICK_WRITE_SDP_POST_DE)
          && HSCROLL) 
          shifter_draw_pointer+=-4; // fixes Big Wobble
#endif

#if defined(SS_VID_SCHNUSDIE) && defined(SS_VID_STEEM_EXTENDED) 
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

/*
    Flix:
    In order to make the  opening  of  the 
    "side-borders"  run  on  all STs,  you've  to  install  so-called 
    "Stabilisation"-switches  at  the  end  of  each  line  (This  is 
    sometimes  called "Closing of the right border" which it  isn't). 
    You  can  switch  to  71 Hz (like in the  source)  or  to  medium 
    resolution (like ULM (Unlimited Matricks) does it).  I prefer the 
    71  Hz  method,  because  I heard that the  other  method  causes 
    problems  on few STs.  The "Stabilisation" is needed because  the 
    shifter  waits for the last word at the end of the  right  border 
    (115  words are displayed in fullscreen) to fill the last  plane. 
    This  word  never  arrives and to avoid total  confusion  of  the 
    shifter the 71-Hz-switch does something like a shifter-reset.

    ST-CNX:
        Stabiliser

        So now we know how to remove the left border, and we know how to
        remove the right border, shouldn't removing both lead to
        overscan? This is where my research was stuck for 3 months...
        The problem is that one obtains an unstable overscan. This was
        predictable as we learnt above that lines in left overscan start
        500ns earlier, and the example of right overscan at 60Hz taught
        us that right overscan is unstable if it starts 500ns earlier.
        The instable right overscan displayed the useable screen 8
        pixels early every second line, which is exactly what one
        observes if one removes both borders! The only difference is
        that the lines have lengths of 226 or 234 bytes, i.e. 230 +/- 4
        bytes. One recognises the word too many that is swallowed by the
        opening of the left border: 226 = 224 + 2, where 224 is a
        multiple of 8; and 234 = 232 + 2 where 232 is a multiple of 8.
        In fact the MMU reads 230 bytes each line, but the SHIFTER
        displays 226 (less the 2 it ignores) or 234 (-2). This shift
        doesn't seem all that bothersome at first glance since one could
        shift the contents of every second line by 8 pixels. However if
        one wants a partial overscan (removing the borders only on some
        lines), the remainder of the normal useable screen will also be
        shifted left if it starts on a shifted line. Furthermore, the
        shift is perpetuated from VBL to VBL, so that at each new VBL
        two words are introduced. Thus the useable screen trembles, each
        line being shifted differently every VBL.

        Some STs have a combination of MMU, GLUE and SHIFTER chips that
        allow them to tollerate such an overscan if an even number of
        lines have their borders removed. Others don't because the DE
        signal is sometimes activated, sometimes disactivated at the end
        of each line, the GLUE and the SHIFTER being out of synch. One
        remedies this situation by using a stabiliser, of which there
        are different kinds. Stabilisers exploit the internal state of
        the SHIFTER to make it believe that it is dealing with a
        multiple of 4 words length line. To do this one changes
        resolution during the useable screen. We saw that changing
        resolution immediately changed the way the SHIFTER decodes
        bitplanes. In fact it also changes the way it manages its IR
        registers. Thus a stabiliser could be implemented by a
        transition to medium resolution. But since each stabiliser
        functions differently, we will only study one: transitioning to
        high resolution at position 108. This is the stablest
        stabiliser. In fact this transition could occur at any position
        that is a multiple of 4 because at these positions the internal
        state of the RR and IR regiters is the same. The position 108 is
        chosen because it is on the extreme right of the picture, hiding
        the effect of the stabiliser. Indeed this stabiliser causes 12
        black pixels followed by 16 pixels with displaced bitplanes to
        be displayed. The 12 pixels correspond to the 12 cycle
        transition to high resolution.

 
        At position 108, a secondary effect occurs: the Blank signal is
        activated 2us later on an STF, causing 16 pixels to be displayed
        further right that is usually possible.

        The stabiliser thus consists of transitioning to high resolution
        for 12 cycles, followed by a return to low or medium resolution.
        The transition occurs just before the RR registers are loaded
        with the contents of the IR registers (Recall that the 68000 and
        the SHIFTER are out of sync so that the MMU can send image data
        to the SHIFTER while the 68000 changes the palette). During the
        first 12 cycles the RR registers act as they would in high
        resolution. The switch back to low or medium resolution occurs
        just before RR1 is loaded with the contents of RR2, RR2 with
        that of RR3 (and so on). Thus RR3 and RR4 contain after these 12
        cycles $0000 or $FFFF according to the value of bit 0 of colour
        0 (BE). RR2 contains what was in RR4, and RR1 contains what what
        was in RR3.

        There are then 4 pixels to show that will be shown in the wrong
        colours: colours 0-3 or 12-15 depending on whether RR3 and RR4
        contain $0000 or $FFFF (RR1 corresponds to the least significant
        bit of the colour, while RR4 corresponds to the most significant
        bit of the colour: see "La Bible ST" chapter 3.4).

        Now consider the IR registers: when the switch to low or medium
        resolution occurs, IR4 has yet to be loaded, but the switch at
        this position has as effect to force Number_of_read_bitplanes to
        4. Since the RR registers are not yet empty, the SHIFTER does
        not reinitialise them, but starts to reload the IR registers.
        There are now still four words to read until the end of the
        line, so the next line will not be shifted. Notice that for 4
        cycles after the return to low or medium resolution, the SHIFTER
        continues to rotate the RR registers as if it were still in high
        resolution. Thus at that time, RR2 to RR4 contain $0000 or
        $FFFF, while only RR1 comes from screen memory: RR1 contains
        what was in RR2 four cycles before, and therefore what should
        have been displayed in RR4. Its colours correspond to the colour
        0-1 or 14-15 of the palette, respectively. A stabiliser
        consisting of a switch to medium instead of high resolution at
        this position also works, but the screen is sometimes shifted by
        a pixel horizontally. This occurs seemingly randomly.

        Resetting the Shifter

        Exiting certain overscans often causes the bitplanes of the
        following normal screen to be displaced. This occurs because the
        last line of the overscan was missing its stabiliser. While in
        overscan, one doesn't observe the displacement of bitplanes
        because the stabiliser of the first line of the next VBL clears
        the effect. Sometimes they hide the effect further by setting
        the colours to zero during that first line. To restore the
        picture at the end of one of these programs. one must reset the
        SHIFTER. This is done by using a sequence of switches of
        resolution at the begining of each VBL during the border, while
        DE is disactivated. It has the effect of clearing out the
        registers of the SHIFTER. I recommend that you use the timings
        indicated in my example programs so that your demos work on
        every ST. Finally do not "optimize" your code by removing the
        stabiliser, which MUST ALSO BE PRECISELY PLACED. This would
        returning to the state of the art 3 years ago!
  
    SS:
        Not really understanding the text above yet.
        For the moment, we consider that on a STF, overscan without stabiliser
        and with late switches would cause a +4 shift (DOLB, Omega) due to
        shifter registers. 
        Stabiliser detection is used for other hacks too.
        Plus for Overdrive/Dragon, a -2 shift... just hacks really
*/

#if defined(SS_VID_STEEM_EXTENDED) && defined(TRICK_STABILISER)
  if(!(CurrentScanline.Tricks&TRICK_STABILISER))
  {
    r2cycle=NextShiftModeChange(432/*,2*/); // Forest STF2
    if(r2cycle>-1 && r2cycle<460)
    {
      r0cycle=NextShiftModeChange(r2cycle,0);
      if(r0cycle>-1 && r0cycle<464 ) 
        CurrentScanline.Tricks|=TRICK_STABILISER;
    }
  }
#endif

  ////////////////
  // BLACK LINE //
  ////////////////

/*  A sync switch at cycle 28 causes the shifter to fetch the line but not
    display it, showing black pixels instead. Overscan #5,#6, Forest.
*/

  if(!draw_line_off && shifter_freq_at_start_of_vbl==50)
  {
    t=LINECYCLE0+28; //trigger point
    if(act-t>0)
    {
      i=CheckFreq(t);
      if(i>=0 && shifter_freq_change[i]==60 && shifter_freq_change_time[i]==t)
        CurrentScanline.Tricks|=TRICK_BLACK_LINE;
    }
  }

  if(!draw_line_off && (CurrentScanline.Tricks&TRICK_BLACK_LINE))
  {
    //TRACE("%d BLK\n",scan_y);
    draw_line_off=true;
    memset(PCpal,0,sizeof(long)*16); // all colours black
  }

  ///////////////////////
  // NON-STOPPING LINE //
  ///////////////////////

/*  In the Enchanted Land hardware tests, a HI/LO switch at end of display
    when the right border has been removed causes the shifter to continue 
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

#if defined(SS_VID_STEEM_EXTENDED) && defined(SS_VID_NON_STOPPING_LINE)
  if(CurrentScanline.Bytes==160+44 && (// left on, right off
#if defined(SS_STF)
    ST_TYPE!=STE && ShiftModeChangeAtCycle(464)==2 
    && !ShiftModeChangeAtCycle(464+8)|| ST_TYPE==STE && 
#endif
    ShiftModeChangeAtCycle(460)==2 && !ShiftModeChangeAtCycle(460+8)))
  {
    TRACE("F%d y%d c%d Enchanted Land HW test R2 %d R0 %d\n",FRAME,scan_y,CyclesIn,NextShiftModeChange(460-4,2),NextShiftModeChange(460,0));
    CurrentScanline.Bytes+=24; // "double" right off, fixes Enchanted Land
  }
#endif

  //////////////////////
  // MED RES OVERSCAN //
  //////////////////////

/*  Overscan (230byte lines) is possible in medium resolution too. Because the
    shifter may have fetched some bytes when the switch occurs, we need to 
    shift the display according to R1 cycle (No Cooper Greetings), I think. 
    20 for NCG   512R2 12R0 20R1
    28 for PYM/BPOC  512R2 12R0 28R1
    36 for NCG off lines 183, 200 512R2 12R0 36R1 (strange!)
    16 for Drag/Reset
    12 & 16 for PYM/STCNX left off (which is really doing 4bit hardcsroll)
*/

#if defined(SS_VID_STEEM_EXTENDED) && defined(SS_VID_MED_OVERSCAN)

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

#if defined(SS_VID_NO_COOPER_GREETINGS)
        if(SSE_HACKS_ON) 
        {
          int shift=(((cycles_in_low_res)/2)%8)/2; // = 2
          shifter_draw_pointer+=-shift; 
          overscan_add_extra+=shift;
        }
#endif
#if defined(SS_VID_BPOC)
        if(BORDER_40 && SSE_HACKS_ON && cycles_in_low_res==16) 
          { // fit text of Best Part of the Creation on a 800 display
              shifter_draw_pointer+=4,overscan_add_extra+=-4;
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
  
#if defined(SS_VID_STEEM_EXTENDED) && defined(SS_VID_4BIT_SCROLL)
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
        shifter_draw_pointer+=shift_in_bytes; // that way we don't need to
        overscan_add_extra-=shift_in_bytes; // correct in Render()
        HblPixelShift=13+8-cycles_in_med_res-8; // -7,-3,1, 5, done in Render()
//        TRACE("4bit y%d CMR%d CLR%d SH%d PIX%d\n",scan_y,cycles_in_med_res,cycles_in_low_res,shift_in_bytes,HblPixelShift);
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
  if(shifter_freq_at_start_of_vbl==50 && left_border==BORDER_SIDE
    &&!(TrickExecuted&TRICK_LINE_PLUS_2) )
  {   
#if defined(SS_VID_LINE_PLUS_2_THRESHOLD)
    t=LINECYCLE0+52; // fixes Forest
#else
    t=LINECYCLE0+58; 
#endif
    if(act-t>0) 
    {
      i=CheckFreq(t);
      if(i>=0 && shifter_freq_change[i]==60)
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_2;
    }
  }

  if((CurrentScanline.Tricks&TRICK_LINE_PLUS_2)
    && !(TrickExecuted&TRICK_LINE_PLUS_2))
  {
    ASSERT(left_border==BORDER_SIDE);
    ASSERT(!(CurrentScanline.Tricks&TRICK_LINE_MINUS_2));
    left_border-=4; // 2 bytes -> 4 cycles
    overscan_add_extra+=2;
    CurrentScanline.Bytes+=2;
    TrickExecuted|=TRICK_LINE_PLUS_2;
    overscan=OVERSCAN_MAX_COUNTDOWN; // 25
   // TRACE("+2 y %d c %d +2 60 %d 50 %d\n",scan_y,LINECYCLES,FreqChangeCycle(i),FreqChangeCycle(i+1));
#if defined(SS_VID_TRACE_SUSPICIOUS2)
    if(shifter_freq_change_time[i]-LINECYCLE0<0)
      TRACE("F%d Suspicious +2 y %d tmg sw %d tmg hbl %d diff %d\n",FRAME,scan_y,shifter_freq_change_time[i],cpu_timer_at_start_of_hbl,shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl);
#endif
  }

  if(right_border_changed) // TODO improve this?
    return; 

  ///////////////
  // LINE -106 //
  ///////////////

/*  A shift mode switch to 2 at cycle 160 (end of HIRES line) causes the line 
    to stop there. 106 bytes are not fetched.

    ST-CNX
           Furthermore if one switches to monochrome at position 40, one
        obtains line lenght of 80 bytes (if the left border has been
        removed) or of 80-26 = 54 bytes (not removing the left border)
        corresponding to the 80 bytes of each high resolution line. Note
        that one has to quickly return to low or medium resolution to
        avoid causing an Hsync and a new line.
*/

#if defined(SS_VID_STEEM_ORIGINAL) || 0
  t=cpu_timer_at_start_of_hbl+172; //trigger point for big right border
  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
     i=CheckFreq(t);
     if(i>=0 && shifter_freq_change[i]==MONO_HZ)
       CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;
  }
#elif defined(SS_VID_STEEM_EXTENDED) && 1// eg PYM/ST-CNX
  t=LINECYCLE0+164; // look for R2 <= 164
  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_106))
  {
     i=CheckShiftMode(t);
     if(i>=0 && shifter_shift_mode_change[i]==2 && ShiftModeChangeCycle(i)>140)
       CurrentScanline.Tricks|=TRICK_LINE_MINUS_106;

#ifdef SS_DEBUG
     r2cycle=ShiftModeChangeAtCycle(160);     // Superior160 164 (LOL)
     if( (CurrentScanline.Tricks&TRICK_LINE_MINUS_106) && r2cycle!=2)
       TRACE("Line -106 R2 cycle %d\n",shifter_shift_mode_change_time[i]-LINECYCLE0);
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
  ST-CNX
       * 158 byte line at 50Hz by switching to 60Hz at position 93,
        enabling the 60Hz end of line detection. The line is shorter by
        a word, corresponding to the difference of 0.5us between the two
        times to display a line (63.5us versus 64us).

    SS: it's a -2 line if it's at 60hz at cycle 372, the switch doesn't
    need to happen at 372, though it makes more sense there. Sooner, there
    could be distortion on a real ST.
*/

  if( shifter_freq_at_start_of_vbl!=50)
    return; 

  // Steem test
  t=LINECYCLE0+372; //trigger point for early right border
  if(act-t>=0 && !(TrickExecuted&TRICK_LINE_MINUS_2))
  {
    i=CheckFreq(t);
    if(i==-1)
      return;                       // vv looks obvious but...
    if(shifter_freq_change[i]==60 && CurrentScanline.StartCycle!=52)
      CurrentScanline.Tricks|=TRICK_LINE_MINUS_2;
  }

#if defined(SS_VID_TRACE_SUSPICIOUS2)
  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)&&!(shifter_freq_change_time[i]-LINECYCLE0>56))
    TRACE("F%d Suspicious -2 y %d tmg sw %d tmg hbl %d diff %d\n",FRAME,scan_y,shifter_freq_change_time[i],LINECYCLE0,shifter_freq_change_time[i]-LINECYCLE0);
#endif

  if((CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
    &&!(TrickExecuted&TRICK_LINE_MINUS_2))
  {
    overscan_add_extra+=-2;
    CurrentScanline.Bytes+=-2;
    TrickExecuted|=TRICK_LINE_MINUS_2;
    right_border_changed=true;
//    TRACE("-2 y %d c %d s %d e %d ea %d\n",scan_y,LINECYCLES,scanline_drawn_so_far,overscan_add_extra,ExtraAdded);
  }

  /////////////////////////////////
  // RIGHT BORDER OFF (line +44) // 
  /////////////////////////////////

/* 
    Flix:
    Soon  the next border was declared to vanish:  The right  border. 
    Unfortunately it is not possible to open this border with Timer B 
    or HBL interrupts.  The ST displays graphics in the right border, 
    if  you  switch the frequency to 60 Hz at a certain  position  in 
    every line,  in which the right border is supposed to be  opened. 
    This  "certain" position requires a completely new  technique  of 
    programming. If you switch colours in an ordinary Timer B, VBL or 
    HBL interrupt, you can see, that these interrupts do not occur at 
    exactly the same position. The colours shake from the left to the 
    right.  In  order to make some commands,  like the colour or  our 
    frequency  switchings,  occur at exactly the same  position,  you 
    have  to  become synchronized with  the  raster-electron-ray.  An 
    ingenious method to achieve this effect is the following one:

    WAIT:     MOVE.B    $FF8209.W,D0   ; Low-Byte
              BEQ.S     WAIT           ; mustn't be 0
              NOT.B     D0             ; negate D0
              LSL.B     D0,D0          ; Synchronisation

    If  you execute this routine every VBL,  all  following  commands 
    will be executed at the same position every VBL,  that means that 
    the colour- or frequency-switches are stable.  But what does this 
    little routine do?  At first,  the low-byte of the screen-address 
    is  loaded  into D0.  This byte exactly determines  the  position 
    within  the line.  It is negated and the LSL-command is  executed 
    (LSR,  ASL  or  ASR  work as well).  As you  can  read  in  every 
    processor-book  the LSL-command takes  8+2*n  clock-cycles.  That 
    means  that  the command needs more clock-cycles the  bigger  the 
    value in D0 is. That is exactly the shifting that we need! I hope 
    that you understood this part,  because all fullscreens and sync-
    scrolling-routines  are based upon this effect.  You should  know 
    that  one  VBL  (50  Hz) consists  of  160000  clock-cycles  (one 
    scanline  consists of 512 clock-cycles).  Now you have to  switch 
    the  frequency  at a certain position and the  border  opens.  Of 
    course  this takes a lot more processor time than the opening  of 
    the  upper  or lower border,  because you've to open  the  border 
    every  line.  Your  ST now displays 204 bytes per  line!  A  line 
    consists  of 25.5 instead of 20 words without the  right  border, 
    but should use only 23 of these 25.5 words,  because the  picture 
    is  distorted on some STs (We made this mistake in  the  "Musical 
    Wonder  - 1991") if you use too much  words.  Anyway,  there  are 
    hardly  any monitors that have such a huge visible right  border. 
    A  demo-screen  without  the  lower  and  the  right  border  was 
    included in the "Amiga-Demo" by TEX.

    ST-CNX:
         * Right Overscan by switching to 60Hz at position 94, overiding
        the 50Hz end-of-useable-line detection, and then returning to
        50Hz. Each line is now 160 + 44 = 204 bytes long. At 50Hz this
        mode is stable on all ST's. Notice that 204 bytes is not a
        multiple of 8, and therefore 2 bitplanes on the extreme right of
        the picture are not displayed.

        It is important to return immediately to 50Hz, otherwise STE's
        react bizzarely and distort the screen.

        At 60Hz, right overscan is also possible by playing the same
        trick at position 93. The length of each line is still 204, but
        the line must be stabilised (the principle of stabilisation is
        described below). Furthermore I have not checked whether this
        kind of line can be stabilised on all STs.
        (...)
        Note that one can obtain remove the right border by switching to
        monochrome at position 94 at 50Hz (overiding the end-of-useable-
        line detection) but that this method causes a black line to be
        displayed between the normal screen and the right overscan (see
        explanation below). A 0 byte line can also be obtained. This
        method is stable but because it occurs at the time Hsync is
        being processed, it can distort the screen.

  
    SS:
    1) The mysterious "certain position" is cycle 376 (94x4) in our reckoning,
    374 is OK on many machines.
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
*/

  t=LINECYCLE0+378; //trigger point for right border cut
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
      if(shifter_freq_change[i]!=50
#if defined(SS_VID_STEEM_EXTENDED)
#if defined(SS_STF_WAKE_UP) // Nostalgia menu doesn't work in wake up state 2
        && (ST_TYPE!=STF2 && FreqAtCycle(374)==50
        || ST_TYPE==STF2 && FreqAtCycle(376)==50)
#else
        && FreqAtCycle(374)==50 
#endif
#endif
        && shifter_freq_change_time[i]-LINECYCLE0>372 
#if defined(SS_VID_OMEGA)
/*  Wrong cycle! It really hits at 372 but Steem is confused by the 508 cycles 
    lines. Hatari hasn't that problem. It's hard to fix.TODO
*/
    &&(!(CurrentScanline.Cycles==508) || !SSE_HACKS_ON)
#endif
        )
        CurrentScanline.Tricks|=TRICK_LINE_PLUS_44;
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
  }

}


void TShifter::CheckSyncTrick() {

#if defined(SS_VID_END_OF_LINE_CORRECTION) //  or should be trashed by compiler
  // Those tests are much like EndHBL in Hatari
  // Finish horizontal overscan : correct -2 & +2 effects
  if(CurrentScanline.Tricks&TRICK_LINE_PLUS_2 && CurrentScanline.EndCycle==372)     
  {
    CurrentScanline.Tricks&=~TRICK_LINE_PLUS_2;
    shifter_draw_pointer-=2; // eg SNYD/TCB at scan_y -29
#if defined(SS_VID_END_OF_LINE_CORRECTION_TRACE)
    TRACE("scan_y %d cancel +2\n",scan_y);
#endif
  } // no 'else', they're false alerts!
  if(CurrentScanline.Tricks&TRICK_LINE_MINUS_2     
    && (CurrentScanline.StartCycle==52 || CurrentScanline.EndCycle!=372))
  {
    CurrentScanline.Tricks&=~TRICK_LINE_MINUS_2;
    shifter_draw_pointer+=2;
#if defined(SS_VID_END_OF_LINE_CORRECTION_TRACE)
    TRACE("scan_y %d cancel -2\n",scan_y);
#endif
  }

#if defined(SS_VID_DRAGON) //temp
  if(ST_TYPE==STF2&&SSE_HACKS_ON&&CurrentScanline.Tricks==1) // left off, no stabiliser
    SS_signal=SS_SIGNAL_SHIFTER_CONFUSED_1; // stage 1 of our hack...
  else if(CurrentScanline.Tricks&&CurrentScanline.Tricks!=0x10
    && (CurrentScanline.Tricks!=TRICK_CONFUSED_SHIFTER))
    SS_signal=0;
#endif

#endif
}


void TShifter::CheckVerticalOverscan() {

/* Vertical overscan

    Flix:
    The  first human ever to display any graphics in a screen  border 
    was  Sven alias "Alyssa" from Mannheim/Germany.  Back in 1987  he 
    wrote  an intro,  that showed graphics in the  lower  border.  As 
    Sven  vanished  very fast from the  ST-scene,  it  is  completely 
    unsettled,  how Sven had the idea to switch the screen  frequency 
    to  60 Hz for a short period of time within the last line of  the 
    screen.  Without this fundamental idea,  no fullscreens or  sync-
    scrolling  would be possible on the ST.  Unfortunately nobody  in 
    the  ST-scene  knows  what "Alyssa"  is  doing  today.  TEX  (The 
    Exceptions)  firstly  used graphics in the lower  border  in  the 
    famous  "B.I.G.  Demo".  To  open the lower  border,  the  screen 
    frequency  has  to be switched to 60 Hz at the end of  the  199th 
    line.  The  ST  then  reads nearly 50 more lines  of  the  screen 
    memory!  The Exceptions revealed this secret in the  B.I.G.  Demo 
    at the end of the longest scrolltext (42 Kybtes of  text).  After 
    The Exceptions took up Alyssa's trick,  the upper border was  the 
    next  one  to  be  removed.  This is  achieved  by  toggling  the 
    frequency  13  or  29 lines above the "usual"  beginning  of  the 
    screen.  You  may wonder,  why the upper border can be 13  or  29 
    lines long.  Unfortunately there are two different  MMU-versions, 
    that have a difference of 16 lines.  Due to the place to open the 
    upper  border  being  above the screen memory  and  the  Timer  B 
    starting  to  count at the beginning of the  screen  memory,  The 
    Exceptions  used a quite complicated method.  They set a Timer  B 
    Interrupt  in  the last line and waited  until  the  electron-ray 
    reached the right position in the upper border.  Doing this, they 
    had  to waste a lot of processor-time.  In the "Musical Wonder  - 
    1991"  I  used a routine that waited a few  scanlines  after  the 
    VBL-Interrupt occured and then toggled the screen-frequency.  But 
    there's  still a better method.  Timer B is counting just  "real" 
    screen-lines  (200),  but there is another  interrupt,  the  HBL, 
    which  counts all scanlines (313).  At the beginning of  the  VBL 
    the  HBL  starts counting until the right position  to  open  the 
    upper border is reached.  If you now open both, the upper and the 
    lower,  border,  you  have  277  lines of  graphics  in  the  low 
    resolution.
    (...)
    If you now open all borders, you've a screen memory that consists 
    of  160+230*276  (=63640) bytes.  The first line  is  needed  for 
    synchronisation, therefore it has 160 bytes. The screen memory is 
    almost twice as big as in the "usual" low resolution!
    (...)
    It  is  possible to open two more lines of the  lower  border  by 
    switching  to 60 Hz for a second time at the end of the  "normal" 
    lower border.  Unfortunately this does not work on the new  Atari 
    colour  monitors 

    ST-CNX:

The vertical counter is  incremented every line, i.e.  at every edge of  the HBL
signal. It will be reset on each edge of the VBL.

At the top of the screen, a border will be displayed, and once a given value  of
the vertical  counter is  reached, an  internal signal  will be  set to show the
useable  screen;  let us  call  this signal  V.  V masks  the  DE signal,  which
specifies the location of the right and  left border. The V signal will also  be
deactivated when the vertical counter reaches a further value, corresponding  to
the end of the useable screen. Finally, once the counter reaches a third  value,
a VBL interrupt and the beginning of a new frame is triggered.

Based on  this, nothing  seems to  allow us  to get  rid of  the upper and lower
border. But we  must recall that  there are only  263 lines for  a 60 Hz picture
whereas there are 313 lines for a  50 Hz picture. If the upper and  lower border
had the same  height at 50Hz,  each one would  be 56 lines  high. If the useable
screen at 60Hz started  at line 56, as  it does at 50Hz,  there would only be  7
lines left for the  lower border: the useable  screen wouldn't be centered  ! On
some monitors, the bottom of the picture wouldn't even be visible...
Therefore the useable screen must start higher at 60 Hz.

At 60 Hz, the picture starts at line 34 and ends at line 234.
At 50Hz, the picture starts at line X and ends at line X+200.

Beware: the value X depends on the genation of the GLUE:
For older STs: X = 63-16 = 47 # (frost: beware also that the diagram says 49
lines).
For "newer" STs (every STs except those of ST Connexion!): X = 63.
In other words, on older STs, the picture is shifted upwards by 16 lines at 50Hz:
this explains why some obtains only 249 lines whereas others get 271 lines for a
simple Low-Border Overscan (thanks to Marlon of ST Connexion for telling me about this).

The following pseudo-code describes the work of the GLUE in a simplified fashion:

Line_Number=0
REPEAT
IF Line_Number ==  34   AND display_freq=60 THEN Activate signal V
IF Line_Number == X     AND display_freq=50 THEN Activate signal V
IF Line_Number == 234   AND display_freq=60 THEN de-activate signal V
IF Line_Number == 200+X AND display_freq=50 THEN de-activate signal V
IF Line_Number == 263   AND display_freq=60 THEN start a new VBL
IF Line_Number == 313   AND display_freq=50 THEN start a new VBL
Line_Number = Line_Number+1
END_REPEAT

Note that the frequency test happens more  than once when a new VBL is  started.
Thus, it  is not  possible to  use the  second example  program (see part #2) to
check the starting line of a 60  Hz VBL: just as well because line  263 triggers
the Low-Border Overscan on newer STs. If you want to check, make the VBL  adjust
the frequency to 50 Hz and the HBL to 60Hz at line 263.

The pseudo-code  is only  presented as  an example  to help  you understand  the
problem: the  GLUE is  not programmed  (that would  be too  slow for a component
which must  react in  500 nanoseconds).  Everything is  implemented as  hardware
logic, which explains the use of  equalities which are easier to implement  than
inequalities.

Since the ST only checks whether the vertical counter is equal to a given  value
to activate or deactivate the signal V,  one can obtain a variety of effects  by
setting the refresh rate at the appropriate line.

Thus, in order to produce a Low  Border Overscan, it is sufficient to switch  to
60 Hz at the right time at the end of line 199 of the useable screen. This  will
prevent the GLUE from  deactivating the V signal:  at the value of  the vertical
counter corresponding to the  199th line of the  useable screen at 50Hz,  V will
only  be deactivated  if the  current frequency  is 50Hz.  Because the  vertical
counter is now  greater than any  of the values  that cause an  event at a  60Hz
refresh rate, there are  no other effects to  worry about. Once the  time of the
test has elapsed (the test occurs at the beginning of the line, even before  the
left border is displayed),  it is necessary to  switch back to 50  Hz, otherwise
the screen will be distorted (refer back to the pseudo-code).

It is also  possible to shrink  the screen by  switching to  60  Hz at line 234:
this  corresponds  to  the point  when the  signal V  is deactivated  at 60 Hz.
Switching back to 50 Hz,  the screen will not be  affected by the subsequent de-
activation of signal V at line X+200.

Finally, it is useful to know that  one can produce a low border Overscan  at 60
Hz: at the end of  line 199 of the useable  screen, we switch to 50  Hz and then
back to  60 Hz,  avoiding the  test  at  line 234.  We can  use the same Timer B
interrupt routine whether the  program is running at  a refresh rate of  50Hz or
60Hzin, by using the following instruction:  eor.b   #2,$ffff820a.w.  The only
commercial program that  I know that  uses this technique  is the game  "Leavin'
Teramis" by Thalion Software: during  level loading, a 22000 colours  picture is
displayed, at either 50  hz or 60 Hz  (to reduce flickering). The  green message
under the picture is displayed in the lower border in both cases.

As  the lower  overscan happens  at the  bottom of  the useable  screen, we  can
synchronize the 68000 code versus the video counter ($ffff8205 to  $ffff8209),
resulting in better stability (see next  the part for example code). It  is even
possible to use the  floppy disk DMA with  a bottom overscan. This  kind of code
trades off the  amount of time  spent at the  alternate refresh rate  versus the
amount of distortion caused on the screen.  Note that there do not appear to  be
any methods other than toggling the refresh rate to remove the lower border.

To remove the  upper border, we  simply switch to  60 Hz at  the end of  line 33
(just before line 34,  where the test happens). As the top border overscan comes
before the useable picture,  we  cannot synchronize our program using  the video
counter. Various solutions have been used:

- A stupid wait loop  from the  beginning  of the VBL  interrupt handler,  which
  wastes cpu cycles but is easy to implement. This method makes two bets:

   * First bet, the VBL interrupt will always happen immediately, which requires
     that the 68000 is not executing an instruction which takes many cycles such
     as  movem  or divu,  since the 68000 will complete its current  instruction
     before servicing the interrupt handler.

   * The Second bet belongs  to the time taken by the  GLUE before treating line
     34. It has to be a constant whichever ST is used. So far this has proven to
     be true.

- The use of a MFP timer: This method allows the more than 17000 cpu cycles
  before line 34 to be used. However this method makes the following additional
  bets:

   * Third bet, the latency to service the MFP interrupt will always be the same.
     This is true for the ST/STF series,  but some STEs that I was able to  test
     (thanks to Laurent) triggered  the interrupt 60 cycles later than on a STF!
     The bottom overscan of Leavin' Teramis doesn't' work on those STEs, even if
     the program  detects the STE:  This  shows that this delay varies according
     to your model of STE.

   * Fourth bet, unless we leave some dead time during line 34, we may encounter
     the same problem for the MFP interrupt as described in the first bet!

- The use of the HBL interrupt: this interrupt is nearly never used on the ST,
  and is even forbidden by the ROM (its interrupt handler, pointed to by vector
  $68 has as only function to mask this interrupt). As this interrupt is
  triggered by the Hsync signal, it occurs on every line (including borders and
  the useable screen), which makes it perfect for counting lines up to the 34th
  line after the VBL. But it has a drawback, which explains why this method is
  rarely used:  The HBL  interrupt occurs at the  beginning of the line, and the
  68000  takes at least  44 cycles  to enter the  interrupt handler, so that the
  interrupt  handling routine  is executed  in the middle of the  left border...
  Which is ugly if one uses it to change the palette!
  To remove  the upper  border,  each  HBL interrupt  will check  whether it has
  reached line 33. If so, it will wait (with a DBF and some NOPs)  until the end
  of the line to toggle the refresh rate. This way, we lose lose less cpu time
  than the first method, and we don't depend on the MFP.

  This method takes 2 bets:

  * First bet, the number of HBL lines to count will not change, whatever the ST
    we use:  This has never happened, and if it ever would,  none of other means
    of  removing  the upper border,  described above,  would work.  However this
    method will survive when the previous methods will not: The time between the
    VBL and line 34 may vary by up to a quarter of a line without  affecting the
    number of HBLs that must be counted...

  * Second bet, the time taken to service  the HBL interrupt will be a constant.
    Again,  I have never  seen this not to be true,  and it is hard  to see why
    Atari  would add  extra  circuitry to  this signal:  Some  chips  were added
    between the MFP and the 68000 on the STE,  but the MFP is fully used by this
    computer. Atari's disdain for the HBL should preserve its reliability.

It is  impossible to trigger a top  overscan in 60 Hz using a 50/60 Hz frequency
switch.

There is another method of removing the upper border: one switches to monochrome
output at the beginning of the VBL. This technique allows us to use most of  the
34 lines  above the  top border  overscan described  above. Unfortunately,  this
method only works on some STs, but not on STEs... This method relies on the fact
that a monochrome screen has an  even shorter top border since the  refresh rate
is 70Hz.  By switching  back to  50 or  60 Hz,  after the  video counter  starts
incrementing, one obtains a top border overscan. However, I've noticed that  one
does not remove  the upper border  if one switches  back to 50/60Hz  immediately
after the video  counter starts. Instead  one is able  to synchronize the  68000
with the  video counter  allowing one  to change  the colour  of the border at a
precise location, like BMT seems to do in the third screen of the Skid Row  demo
(see ST Mag 47, page 14). As  this technique is not reliable, I don't  recommend
it, to prevent  certain people from  complaining... (This is  an allusion to  an
article  previously  published  in  ST  Magazine,  written  by  Belzebub  of  ST
-Connexion, complaining that most overscans do not work on his ancient ST).

  [SS This amazing Skid Row/BMT3 demo doesn't seem to read the SDP, to switch
  sync, all interrupts are disabled, and it runs suspiciously flawlessly in 
  Steem. All timings are good!]

Now that we have learnt how to remove the upper and lower borders, it should  be
easy to  remove both  in the  same screen.  But a  subtlety is often forgotten !
Without top border overscan, we can use Timer B interrupt to trigger the  bottom
overscan, so the border is removed whatever value of X, the number of the  first
line of the useable screen. We  cannot use the same technique when  removing the
upper border: to remove the lower border, we must toggle the refresh rate twice,
with an interval of  16 lines. This will  guarantee compatability with the  most
recent STs as well as antiquities.

    SS:
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
    -Musical Wonder 90 (416:S0,504:S2)
    -SNYD2/Sync Vectorballs II 424:S0000 504:S0002 (every time)
    "the low border of Sync's vectorballs was never stable on my machine"
    This means the MMU didn't react the same to same value in every frame.

    5) See No Cooper greeting, trying a switch at end of line 182, then end
    of line 199. 199-182 = 17?

    6) I know no cases of programs making the screen smaller. It was small
    enough!
*/

  BOOL on_overscan_limit=(scan_y==-30 
    || scan_y==shifter_last_draw_line-1 && scan_y<245); 
  int t,i=shifter_freq_change_idx;
  int freq_at_trigger=0; // =shifter_freq was a stupid bug
  if(screen_res==2) 
    freq_at_trigger=MONO_HZ;
  if(emudetect_overscans_fixed) 
    freq_at_trigger=(on_overscan_limit) ? 60:0;

#if defined(SS_VID_STEEM_ORIGINAL) || defined(SS_VID_VERT_OVSCN_OLD_STEEM_WAY)
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50 
    && freq_change_this_scanline)
  {
    t=cpu_timer_at_start_of_hbl+CYCLES_FROM_HBL_TO_RIGHT_BORDER_CLOSE+98; // 502
    i=CheckFreq(t); 
    freq_at_trigger=(i>-1) ? shifter_freq_change[i] : 0;
  }
#endif

#if defined(SS_VID_STEEM_EXTENDED) && !defined(SS_VID_VERT_OVSCN_OLD_STEEM_WAY)
  else if(on_overscan_limit && shifter_freq_at_start_of_vbl==50)
  {
    t=502;
#if defined(SS_STF) && defined(SS_STF_VERTICAL_OVERSCAN)
    if(ST_TYPE!=STE)
      t+=4; // fixes Auto 168 flicker in Steem
#endif
    
    if(FreqAtCycle(t)==60 
#if defined(SS_VID_STE_VERTICAL_OVERSCAN) 
      || FreqAtCycle(t-2)==60 && FreqChangeAtCycle(t-2)==50 // fixes RGBeast
#endif
      )
      freq_at_trigger=60;
  }
#if defined(SS_VID_60HZ_OVERSCAN) 
  else if(scan_y==199 && shifter_freq_at_start_of_vbl==60 
    && FreqAtCycle(502-4)==50) // simpler test, not many cases
      CurrentScanline.Tricks|=TRICK_VERTICAL_OVERSCAN; // fixes Leavin' Terramis
#endif
#endif

  if(on_overscan_limit && freq_at_trigger==60 // a bit messy, it's because we
    && shifter_freq_at_start_of_vbl==50) // keep the possibility of "old way"
    CurrentScanline.Tricks|=TRICK_VERTICAL_OVERSCAN;

#if !defined(SS_VID_VERT_OVSCN_OLD_STEEM_WAY) \
  && defined(SS_VID_VERTICAL_OVERSCAN_TRACE) && defined(SS_VID_STEEM_EXTENDED)
  if(on_overscan_limit && shifter_freq_at_start_of_vbl==50 
    && freq_change_this_scanline)
  { // debug report differences Steem-SSE
    int t2=cpu_timer_at_start_of_hbl+CYCLES_FROM_HBL_TO_RIGHT_BORDER_CLOSE+98; // 502
    int i2=CheckFreq(t2); 
    int freq_at_trigger2=(i2>-1) ? shifter_freq_change[i2] : 0;
    if(!(CurrentScanline.Tricks&TRICK_VERTICAL_OVERSCAN)
      && freq_at_trigger2==60)
      TRACE("y %d c %d fcts %d\n",scan_y,t,freq_change_this_scanline);
    else if((CurrentScanline.Tricks&TRICK_VERTICAL_OVERSCAN)
      && freq_at_trigger2!=60)
      TRACE("y %d c %d fcts %d FreqAtCycle(t) %d FreqAtCycle(t-2) %d FreqChangeAtCycle(t-2) %d freq_at_trigger %d\n",scan_y,t,freq_change_this_scanline,FreqAtCycle(t),FreqAtCycle(t-2),FreqChangeAtCycle(t-2),freq_at_trigger);

  }
#endif

  if(CurrentScanline.Tricks&TRICK_VERTICAL_OVERSCAN)
  {
    overscan=OVERSCAN_MAX_COUNTDOWN;
    time_of_next_timer_b=time_of_next_event+cpu_cycles_from_hbl_to_timer_b
      + TB_TIME_WOBBLE;
    if(scan_y==-30) // top border off
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

#if defined(SS_VID_VERTICAL_OVERSCAN_TRACE)
  if(on_overscan_limit) 
    TRACE("F%d y%d overscan %X\n",FRAME,scan_y,(bool)(CurrentScanline.Tricks&TRICK_VERTICAL_OVERSCAN));
#endif
}


void TShifter::DrawScanlineToEnd()  { // such a monster wouldn't be inlined
#ifdef SS_VIDEO_DRAW_DBG
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
            draw_scanline(bord,pic,bord,shifter_hscroll);
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
    shifter_pixel=shifter_hscroll; //start by drawing this pixel
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
  
  // Colour.
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
    shifter_pixel=shifter_hscroll; //start by drawing this pixel
#if defined(SS_VID_STE_MED_HSCROLL) 
    HblStartingHscroll=shifter_pixel; // save the real one (not important for Cool STE)
    if(screen_res==1) //it's in cycles, bad name
      shifter_pixel=shifter_hscroll/2; // fixes Cool STE jerky scrollers
#endif
  }
  // Monochrome.
  else 
  {
    if(scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line)
    {
      nsdp=shifter_draw_pointer+80;
      shifter_pixel=shifter_hscroll; //start by drawing this pixel
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


int TShifter::IncScanline() { // a big extension of 'scan_y++'!

#if defined(SS_VID_SHIFTER_EVENTS) 
  // Record the 'tricks' mask at the end of scanline
  if(CurrentScanline.Tricks)
    VideoEvents.Add(scan_y,CurrentScanline.Cycles,'T',CurrentScanline.Tricks);
#endif

#if defined(SS_DEBUG)
  debug7=debug8=debug9=0;
  SSDebug.ShifterTricks|=CurrentScanline.Tricks;
#endif

  scan_y++; 
  
  left_border=BORDER_SIDE;
  if(shifter_hscroll) 
    left_border+=16;
  if(shifter_hscroll_extra_fetch) 
    left_border-=16;
  right_border=BORDER_SIDE;
#if defined(SS_DEBUG) && !defined(SS_VIDEO_DRAW_DBG)
  if( scan_y-1>=shifter_first_draw_line && scan_y+1<shifter_last_draw_line
    && (overscan_add_extra || !ExtraAdded) && screen_res<2)
    TRACE("F%d y%d Extra %d added %d\n",FRAME,scan_y-1,overscan_add_extra,ExtraAdded);
#endif
//        AddExtraToShifterDrawPointerAtEndOfLine(shifter_draw_pointer);
  ExtraAdded=overscan_add_extra=0;
#if defined(SS_VID_SDP_WRITE)
  SDPMiddleByte=999; // an impossible value for a byte
#endif
  PreviousScanline=CurrentScanline; // auto-generated
  CurrentScanline=NextScanline;
  if(scan_y==-29 && (PreviousScanline.Tricks&TRICK_VERTICAL_OVERSCAN)
    || !scan_y && !PreviousScanline.Bytes)
    CurrentScanline.Bytes=160; // needed by ReadSDP - not perfect (TODO)
  else if(FetchingLine()) 
    NextScanline.Bytes=(screen_res==2)?80:160;
  if(shifter_freq==50)
  {
    CurrentScanline.StartCycle=56;
    CurrentScanline.EndCycle=376;
    CurrentScanline.Cycles=512;
  }
  else if(shifter_freq==60)
  {
    CurrentScanline.StartCycle=52;
    CurrentScanline.EndCycle=372;
    CurrentScanline.Cycles=508;
  }
  else if(shifter_freq==72)
  {
    CurrentScanline.StartCycle=0;
    CurrentScanline.EndCycle=160;
    CurrentScanline.Cycles=160;
  }
  TrickExecuted=0;

  // In the STE if you make hscroll non-zero in the normal way then the shifter
  // buffers 2 rasters ahead. We don't do this so to make sdp correct at the
  // end of the line we must add a raster.  
  shifter_skip_raster_for_hscroll = shifter_hscroll!=0;//SS correct place?
  return scan_y; // fancy
}


void TShifter::Render(int cycles_since_hbl,int dispatcher) {
  // this is based on Steem's 'draw_scanline_to'
#if defined(SS_VIDEO_DRAW_DBG)
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
#if defined(SS_VID_DRAGON)
    || SS_signal==SS_SIGNAL_SHIFTER_CONFUSED_2
#endif
    )
    CheckSideOverscan(); 

/*  What happens here is very confusing; we render in real time, but not
    quite. As on a real ST, there's a delay of fetch+12 between the 'shifter
    cycles' and the 'palette reading' cycles. We can't render at once because
    the palette could change. CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN (84) takes
    care of that delay. It's a big hack in Steem I say.
    This causes all sort of trouble because our SDP is late while rendering,
    and sometimes forward!
*/

#if defined(SHIFTER_PREFETCH_LATENCY)
  // this may look impressive but it's just a bunch of hacks!
  switch(dispatcher) {
  case DISPATCHER_CPU:
    cycles_since_hbl+=16; // eg 3615 Gen4 by ULM, override normal delay
    break;
  case DISPATCHER_DSTE:
    break;
  case DISPATCHER_SET_PAL:
    cycles_since_hbl++; // eg Overscan #6, already in v3.2 TODO why?
    break;
  case DISPATCHER_SET_SHIFT_MODE:
    RoundCycles(cycles_since_hbl); // eg Drag/Happy Islands, Cool STE
    break; 
  case DISPATCHER_SET_SYNC:
    RoundCycles(cycles_since_hbl); // eg D4/NGC, D4/Nightmare
    cycles_since_hbl++; // eg the same
    break;
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
        TRACE("LB<0, leaving render\n");
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
        
#if defined(SS_VID_4BIT_SCROLL)
/*  This is where we do the actual shift for those rare programs using the
    4bit hardscroll trick (PYM/ST-CNX,D4/Nightmare,D4/NGC).
    Notice it is quite simple, and also very efficient because it uses 
    the hscroll parameter of the assembly drawing routine (programmed by
    Steem authors, of course).
    hscroll>15 is handled further.
*/
        if(CurrentScanline.Tricks&TRICK_4BIT_SCROLL)
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

#if defined(SS_VID_STE_MED_HSCROLL)
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
            if(scan_y==120) TRACE("y %d hscroll OVL\n",scan_y);
            shifter_draw_pointer+=(SHIFTER_RASTER_PREFETCH_TIMING/2)
              *(hscroll/16); // ST-CNX large border
            hscroll-=16*(hscroll/16);
          }
          
#if defined(SS_DEBUG)
#ifdef SS_VID_SKIP_SCANLINE
          if(scan_y==SS_VID_SKIP_SCANLINE) 
            border1+=picture,picture=0;
#endif
          if(border1<0||picture<0||border2<0||hscroll<0||hscroll>15)
          {
            TRACE("F%d y%d p%d Render error %d %d %d %d\n",
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
#if defined(SS_VID_SDP_WRITE_ADD_EXTRA)
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
      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
        ///////////////// RENDER VIDEO /////////////////
        draw_scanline(border1,0,0,0);
    }
    scanline_drawn_so_far=pixels_in;
  }//if(pixels_in>=0)
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
*/

  int CyclesIn=LINECYCLES;

#if defined(SS_VID_SHIFTER_EVENTS)
  VideoEvents.Add(scan_y,CyclesIn,'R',NewMode); 
  //  TRACE("y%d c%d r%d\n",scan_y,CyclesIn,NewMode);
#endif

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

  NewMode&=3; ASSERT(NewMode!=3); // can only be 0 (low),1 (med),2 (hi) now
#if defined(SS_VID_0BYTE_LINE_TRACE) // try to detect
  if(NewMode!=2 && CyclesIn>448+8 && CyclesIn<512) 
    TRACE("0byte line? y%d c%d r%d\n",scan_y,CyclesIn,NewMode);
#endif
#if defined(SS_VID_STEEM_EXTENDED) // before rendering
  AddShiftModeChange(NewMode); // add time & mode
#endif
  
  Render(CyclesIn,DISPATCHER_SET_SHIFT_MODE);

#if !defined(SS_VID_STEEM_EXTENDED) || defined(SS_DEBUG) // after rendering
  ADD_SHIFTER_FREQ_CHANGE( NewMode==2 ? MONO_HZ : shifter_freq ); // add time & freq
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
    else if(!mixed_output)
      mixed_output=3;
    else if(mixed_output<2)
      mixed_output=2;
    cpu_timer_at_res_change=ABSOLUTE_CPU_TIME;
  }

  freq_change_this_scanline=true; // all switches are interesting
  m_ShiftMode=NewMode; // update

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

#if defined(SS_VID_SHIFTER_EVENTS) 
  VideoEvents.Add(scan_y,CyclesIn,'S',NewSync); 
//  TRACE("y%d c%d s%d\n",scan_y,CyclesIn,NewSync);
#endif

#if defined(SS_VID_BORDERS)
/*  This is very strange. Usually, Steem wouldn't render for sync changes,
    but with a display width of 412, we need to do it or the doubled line
    isn't OK in the left border with the "double size, no stretch" option
    in many programs like Delirious IV menu, Transbeauce 2 menu, etc. 
    There was no such problem with width 416.
    It really is a case of fixing a problem without mastering the rendering
    system. TODo
*/
  if(SideBorderSize==VERY_LARGE_BORDER_SIDE)
    Render(CyclesIn,DISPATCHER_SET_SYNC); // round, +1
#endif
  int new_freq;  

  if(screen_res>=2) // note this part is not extra-reliable yet
  {
    BRK(HIRES new sync); // happens?
    new_freq=MONO_HZ;
    shifter_freq_idx=2;
  }
  else if(NewSync&2) // freq 50hz
  {
    new_freq=50;
    shifter_freq_idx=0;
    const int limit512=
#if defined(SS_STF_WAKE_UP)
      (ST_TYPE==STF2)? 56 :
#endif
      54;
    if(CyclesIn<=56)
      CurrentScanline.Cycles=512;
    if(FetchingLine())
    {
      if(CyclesIn<=52 && left_border)
        CurrentScanline.StartCycle=56;
      if(CyclesIn<=372)
        CurrentScanline.EndCycle=376;
    }
  }
  else // freq 60hz
  {
    new_freq=60;
    shifter_freq_idx=1;
    if(CyclesIn<=52)
      CurrentScanline.Cycles=508;
    if(FetchingLine())
    {
      if(CyclesIn<=52 && left_border)
        CurrentScanline.StartCycle=52; 
      if(CyclesIn<=372 && FetchingLine())
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
  
#if defined(SS_VID_STEEM_EXTENDED)
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
#if defined(SS_VID_SHIFTER_EVENTS)
  VideoEvents.Vbl(); 
#endif
#if defined(SS_DEBUG)
#if defined(SS_VID_REPORT_VBL_TRICKS)
  TRACE("VBL %d shifter tricks %X\n",nVbl,SSDebug.ShifterTricks);
  SSDebug.ShifterTricks=0;
#endif
  nVbl++;
  debug4=debug5=debug6=0;
#endif
}


#if defined(SS_VID_SHIFTER_EVENTS)
#include "SSEShifterEvents.cpp" // debug module, the Steem way...
#endif

#endif//#if defined(SS_VIDEO)