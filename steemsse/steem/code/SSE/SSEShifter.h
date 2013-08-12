#pragma once
#ifndef SSESHIFTER_H
#define SSESHIFTER_H


#if defined(SS_SHIFTER)
/*  The ST was a barebone machine made up of cheap components hastily patched
    together, including the video shifter.
    A shifter was needed to translate the video RAM into the picture.
    Such a system was chosen because RAM space was precious. Chunk modes are
    only possible with byte or more sizes.
    All the strange things you can do with the shifter were found by hackers,
    they weren't devised by Atari. 
    This is both difficult and very interesting to emulate.
    I inserted some good documentation on all those tricks.
    Parts marked Flix are extracts of a text written by this demo maker
    Parts marked ST-CNX come from Alien of ST-Connexion,
    another demo maker, as translated by frost of Sector One
    (more to add)
*/


/*  The shifter trick mask is an imitation of Hatari's BorderMask.
    Each trick has a dedicated bit, to set it we '|' it, to check it
    we '&' it. Each value is the previous one x2.
    Note max 32 bit = $80 000 000
*/

#define TRICK_LINE_PLUS_26 0x001
#define TRICK_LINE_PLUS_2 0x02
#define TRICK_LINE_MINUS_106 0x04
#define TRICK_LINE_MINUS_2 0x08
#define TRICK_LINE_PLUS_44 0x10
#define TRICK_4BIT_SCROLL 0x20
#define TRICK_OVERSCAN_MED_RES 0x40
#define TRICK_BLACK_LINE 0x80	
#define TRICK_TOP_OVERSCAN 0x100
#define TRICK_BOTTOM_OVERSCAN 0x200
#define TRICK_BOTTOM_OVERSCAN_60HZ 0x400
#define TRICK_LINE_PLUS_20 0x800	
#define TRICK_0BYTE_LINE 0x1000	
#define TRICK_STABILISER 0x2000
#define TRICK_WRITE_SDP 0x4000
#define TRICK_WRITE_SDP_POST_DE 0x8000
#if defined(SS_SHIFTER_DRAGON1)
#define TRICK_CONFUSED_SHIFTER 0x10000//tmp hack
#endif
#if defined(SS_SHIFTER_UNSTABLE)
#define TRICK_UNSTABLE 0x10000 // less specific
#endif

// register names in Atari doc / Steem variable
#define HSCROLL shifter_hscroll
#define LINEWID shifter_fetch_extra_words

// ID which part of emulation required video rendering
enum {DISPATCHER_NONE, DISPATCHER_CPU, DISPATCHER_LINEWIDTH,
  DISPATCHER_WRITE_SDP, DISPATCHER_SET_SHIFT_MODE, DISPATCHER_SET_SYNC,
  DISPATCHER_SET_PAL, DISPATCHER_DSTE};

enum {BORDERS_NONE, BORDERS_ON, BORDERS_AUTO_OFF, BORDERS_AUTO_ON};


/*  The shifter latency could be the time needed by the shifter to treat
    data it's fed. 
    It is observed that palette changes occuring at some cycle apply at once,
    but on pixels that would be behind those that are currently being fetched
    (video counter).
    If we don't do that, emulation of Spectrum 512 pictures is not correct.
    There is a hack in Steem, delaying rendering by 28 cycles (16 fetching
    +12 treatment),and also a "magic value" (hack) of 7 (x4) used in Hatari 
    to shift lines with the same result.
    We try to rationalise this.
    Update: the delay is reduced after we corrected prefetch timing.
    TODO, but all reversible
*/
#define SHIFTER_PREFETCH_LATENCY 12 
#define SHIFTER_RASTER_PREFETCH_TIMING (shifter_freq==72 ? 4 : 16) 
#define SHIFTER_RASTER (shifter_freq==72? 2 : (screen_res ? 4 : 8)) 


/*  There's something wrong with the cycles when the line is 508 cycles,
    but fixing it will take some care. See the Omega hack
*/

struct TScanline {
  int StartCycle; // eg 56
  int EndCycle; // eg 376
  int Bytes; // eg 160 - TODO make sure it's always correct
  int Cycles; // eg 512 
  int Tricks; // see mask description above
};


struct TShifter {
/*  As explained by ST-CNX and others, the video picture is produced by the
    MMU, the GLUE and the shifter. 
    In our emulation, we pretend that the shifter does most of it by itself:
    fetching memory, generating signals... It's a simplification. Otherwise
    we could imagine new objects: TMMU, TGlue...
*/
  TShifter(); 
  ~TShifter();
  inline void AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra);
  inline int CheckFreq(int t);
  void CheckSideOverscan(); // left & right border effects
  void CheckVerticalOverscan(); // top & bottom borders
  void EndHBL(); // at end of HBL, check if +2 -2 were correct
#if defined(WIN32)
  inline void DrawBufferedScanlineToVideo();
#endif
  void DrawScanlineToEnd();
  inline int FetchingLine();
  int IncScanline();
  BYTE IORead(MEM_ADDRESS addr);
  void IOWrite(MEM_ADDRESS addr,BYTE io_src_b);
  void Render(int cycles_since_hbl, int dispatcher=DISPATCHER_NONE);
  void Reset(bool Cold);
  inline void RoundCycles(int &cycles_in);
  inline void SetPal(int n, WORD NewPal);
  void SetShiftMode(BYTE NewRes);
  void SetSyncMode(BYTE NewSync);
  inline void ShiftSDP(int shift);
  void Vbl();

#if defined(SS_SHIFTER_TRICKS) // some functions aren't used
  inline void AddFreqChange(int f);
  inline void AddShiftModeChange(int r);
  inline int CheckShiftMode(int t);
  inline int FreqChange(int idx);
  inline int ShiftModeChange(int idx=-1);
  inline int FreqChangeCycle(int idx);
  inline int ShiftModeChangeCycle(int idx);
  inline int FreqChangeIdx(int cycle);
  inline int ShiftModeChangeIdx(int cycle);
  inline int FreqChangeAtCycle(int cycle);
  inline int ShiftModeChangeAtCycle(int cycle);
  // value before this cycle (and a possible change)
  inline int FreqAtCycle(int cycle);
  inline int ShiftModeAtCycle(int cycle);
  // cycle of next change to whatever value after this cycle
  inline int NextFreqChange(int cycle,int value=-1);
  inline int NextShiftModeChange(int cycle,int value=-1);
  // idx of next change to whatever value after this cycle
  inline int NextFreqChangeIdx(int cycle);
  inline int NextShiftModeChangeIdx(int cycle);
  // cycle of previous change to whatever value before this cycle
  inline int PreviousFreqChange(int cycle,int value=-1);
  inline int PreviousShiftModeChange(int cycle,int value=-1);
  // idx of previous change to whatever value before this cycle
  inline int PreviousFreqChangeIdx(int cycle);
  inline int PreviousShiftModeChangeIdx(int cycle);
  inline int CycleOfLastChangeToFreq(int value);
  inline int CycleOfLastChangeToShiftMode(int value);
#endif

#if defined(SS_SHIFTER_SDP)
#ifdef SS_SHIFTER_SDP_READ
  inline MEM_ADDRESS ReadSDP(int cycles_since_hbl,int dispatcher=DISPATCHER_NONE);
#endif
#ifdef SS_SHIFTER_SDP_WRITE
//#if defined(_DEBUG) // VC IDE
//  int WriteSDP(MEM_ADDRESS addr, BYTE io_src_b);
//#else
  inline void WriteSDP(MEM_ADDRESS addr, BYTE io_src_b);
//#endif
  int SDPMiddleByte; // glue it! 
#endif 
#endif

  // we need keep info for only 3 scanlines 
  TScanline PreviousScanline, CurrentScanline, NextScanline;

  int ExtraAdded;//rather silly
  int HblStartingHscroll; // saving true hscroll in MED RES (no use)
  int HblPixelShift; // for 4bit scrolling, other shifts //BYTE?
#if defined(WIN32)
  BYTE *ScanlineBuffer;
#endif
  int m_ShiftMode; // contrary to screen_res, it's updated at each change
  int m_SyncMode;//
//  int m_nHbls; //313,263,501 //not used yet
  int TrickExecuted; //make sure that each trick will only be applied once
#if defined(SS_DEBUG)
  int nVbl;
#endif
#if defined(SS_SHIFTER_UNSTABLE)
  BYTE Preload; // #words into shifter's RR (shifts display)
#endif

};

extern TShifter Shifter; // singleton


////////////////////////////////
// TShifter: inline functions //
////////////////////////////////


#if defined(IN_EMU) //temp...

inline void TShifter::AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra) {
  // What a beautiful name!
  // Replacing macro ADD_EXTRA_TO_SHIFTER_DRAW_POINTER_AT_END_OF_LINE(s)

  if(ExtraAdded)
  {
    ASSERT(!overscan_add_extra);
    return;
  }
  extra+=(LINEWID)*2;     
/*
Left border off + STE scrolling
For STE scrolling, the shifter must load the extra raster that will be used
to skip h pixels. But when the left border has been removed in the STF way (+
26 bytes), the shifter already has 6 bytes inside and will load only 2 more.
This doesn't happen with the STE-only left off (+20 bytes), hence we undo then
what we do here. Maybe.
Also, when must this be applied? Should be moved?
*/
  if(shifter_skip_raster_for_hscroll) 
#if defined(SS_SHIFTER_STE_MED_HSCROLL)
    extra+= (left_border) ? 
    (screen_res==1) ? 4 : 8 // handle MED RES, fixes Cool STE unreadable scrollers
    : 2; 
#else
    extra+= (left_border) ? 8 : 2;
#endif
  extra+=overscan_add_extra;
  overscan_add_extra=0;
  ExtraAdded++;
}


inline int TShifter::CheckFreq(int t) {
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

inline void TShifter::DrawBufferedScanlineToVideo() {
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


inline int TShifter::FetchingLine() {
  // does the current scan_y involve fetching by the shifter?
  // notice < shifter_last_draw_line, not <=
  return (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line);
}


inline void TShifter::RoundCycles(int& cycles_in) {
  cycles_in-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !HSCROLL) 
    cycles_in+=16;
  cycles_in+=SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in&=-SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !HSCROLL) 
    cycles_in-=16;
}


inline void TShifter::SetPal(int n, WORD NewPal) {
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
#if defined(SS_STF)
  if(ST_TYPE!=STE)
    NewPal &= 0x0777; // fixes Forest HW STF test
  else
#endif
    NewPal &= 0x0FFF;
#if defined(SS_VID_AUTOOFF_DECRUNCHING)
  if(!n && NewPal && NewPal!=0x777) // basic test, but who uses autoborder?
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
      PAL_DPEEK(n*2)=STpal[n];
      if(!flashlight_flag && !draw_line_off DEBUG_ONLY(&&!debug_cycle_colours))
        palette_convert(n);
    }
}


#if defined(SS_SHIFTER_TRICKS)

/* V.3.3 used Hatari analysis. Now that we do without this hack, we need
   look-up functions to extend the existing Steem system.
   To make it easier we duplicate the 'freq_change' array for shift mode (res)
   changes (every function is duplicated even if not used).
   TODO: debug & optimise (seek only once), this is first draft
*/

inline void TShifter::AddFreqChange(int f) {
  // Replacing macro ADD_SHIFTER_FREQ_CHANGE(shifter_freq)
  shifter_freq_change_idx++;
  shifter_freq_change_idx&=31;
  shifter_freq_change_time[shifter_freq_change_idx]=ABSOLUTE_CPU_TIME;
  shifter_freq_change[shifter_freq_change_idx]=f;                    
}

inline void TShifter::AddShiftModeChange(int mode) {
  // called only by SetShiftMode
  ASSERT(!mode||mode==1||mode==2);
  shifter_shift_mode_change_idx++;
  shifter_shift_mode_change_idx&=31;
  shifter_shift_mode_change_time[shifter_shift_mode_change_idx]=ABSOLUTE_CPU_TIME;
  shifter_shift_mode_change[shifter_shift_mode_change_idx]=mode;                    
}


inline int TShifter::CheckShiftMode(int t) {
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

inline int TShifter::FreqChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(shifter_freq_change[idx]==50||shifter_freq_change[idx]==60);
  int rv=shifter_freq_change_time[idx]-LINECYCLE0;
  return rv;
}

inline int TShifter::ShiftModeChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(!shifter_shift_mode_change[idx]||shifter_shift_mode_change[idx]==1||shifter_shift_mode_change[idx]==2);
  int rv=shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return rv;
}


inline int TShifter::FreqChangeAtCycle(int cycle) {
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


inline int TShifter::ShiftModeChangeAtCycle(int cycle) {
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


inline int TShifter::FreqChangeIdx(int cycle) {
  // give the idx of freq change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckFreq(t); // use the <= check
  if(idx!=-1 &&shifter_freq_change_time[idx]==t)
    return idx;
  return -1;
}

inline int TShifter::ShiftModeChangeIdx(int cycle) {
  // give the idx of shift mode change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckShiftMode(t); // use the <= check
  if(idx!=-1 &&shifter_shift_mode_change_time[idx]==t)
    return idx;
  return -1;
}


inline int TShifter::FreqAtCycle(int cycle) {
  // what was the frequency just before we reach this cycle?
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<0)
    return shifter_freq_change[i];
  return shifter_freq_at_start_of_vbl;
}

inline int TShifter::ShiftModeAtCycle(int cycle) {
  // what was the shift mode just before we reach this cycle?
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return shifter_shift_mode_change[i];
  return 0;
}

// REDO it stinks
inline int TShifter::NextFreqChange(int cycle,int value) {
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

inline int TShifter::NextShiftModeChange(int cycle,int value) {
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


inline int TShifter::NextFreqChangeIdx(int cycle) {
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

inline int TShifter::NextShiftModeChangeIdx(int cycle) {
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


inline int TShifter::PreviousFreqChange(int cycle,int value) {
  // return cycle of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) 
    if(value==-1 || shifter_freq_change[i]==value)
      idx=i;
  if(idx!=-1 && shifter_freq_change_time[idx]-t<0)
    return shifter_freq_change_time[idx]-LINECYCLE0;
  return -1;
}

inline int TShifter::PreviousShiftModeChange(int cycle,int value) {
  // return cycle of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) 
    if(value==-1 || shifter_shift_mode_change[i]==value)
      idx=i;
  if(idx!=-1 && shifter_shift_mode_change_time[idx]-t<0)
    return shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return -1;
}


inline int TShifter::PreviousFreqChangeIdx(int cycle) {
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

inline int TShifter::PreviousShiftModeChangeIdx(int cycle) {
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


inline int TShifter::CycleOfLastChangeToFreq(int value) {
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change[i]==value)
    return shifter_freq_change_time[i]-LINECYCLE0;
  return -1;
}

inline int TShifter::CycleOfLastChangeToShiftMode(int value) {
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change[i]==value)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}

#endif//#if defined(SS_SHIFTER_TRICKS)



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

    According to ST-CNX, those registers are in the MMU, not in the shifter.

*/

#if defined(SS_SHIFTER_SDP_READ)

inline MEM_ADDRESS TShifter::ReadSDP(int CyclesIn,int dispatcher) {
  if (bad_drawing){
    // Fake SDP
#if defined(SS_SHIFTER_SDP_TRACE_LOG)
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
      bytes_to_count+=SHIFTER_RASTER_PREFETCH_TIMING/2;
    int bytes_ahead=(shifter_hscroll_extra_fetch) 
      ?(SHIFTER_RASTER_PREFETCH_TIMING/2)*2:(SHIFTER_RASTER_PREFETCH_TIMING/2);
    int starts_counting=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN/2 - bytes_ahead;
/*
    84/2-8 = 34
    In Hatari, they start at 56 + 12 = 68, /2 = 34
    In both cases we use kind of magic values.
    The same results from Hatari because their CPU timing at time of read is 
    wrong like in Steem pre 3.5.

    Hack-approach necessary while we're fixing instruction timings, but
    this is the right ST value.

    TODO: modify CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN
    Before that, we keep this big debug block (normally only 
    SS_CPU_PREFETCH_TIMING will be defined).
*/

#if defined(SS_CPU_PREFETCH_TIMING)
  starts_counting-=2;
#else
  if(0);
#if defined(SS_CPU_LINE_0_TIMINGS)
  else if( (ir&0xF000)==0x0000)
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_1_TIMINGS)
  else if( (ir&0xF000)==0x1000)
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_2_TIMINGS)
  else if( (ir&0xF000)==0x2000) 
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_3_TIMINGS)
  else if( (ir&0xF000)==0x3000) 
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_4_TIMINGS)
  else if( (ir&0xF000)==0x4000) 
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_5_TIMINGS)
  else if( (ir&0xF000)==0x5000) 
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_8_TIMINGS)
  else if( (ir&0xF000)==0x8000) 
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_9_TIMINGS)
  else if( (ir&0xF000)==0x9000)
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_B_TIMINGS)
  else if( (ir&0xF000)==0xB000) // CMP & EOR
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_C_TIMINGS)
  else if( (ir&0xF000)==0xC000) // (+ ABCD, EXG, MUL, line C)
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_D_TIMINGS)
  else if( (ir&0xF000)==0xD000) // (+ ABCD, EXG, MUL, line C)
    starts_counting-=2;
#endif
#if defined(SS_CPU_LINE_E_TIMINGS)
  else if( (ir&0xF000)==0xE000) 
    starts_counting-=2;
#endif
#if defined(SS_DEBUG_TRACE_LOG_SDP_READ_IR)
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

    if(!left_border)
      starts_counting-=26;

#if defined(SS_SHIFTER_TCB) && defined(SS_SHIFTER_TRICKS)
/*  There seems to be much confusion caused by 50/60hz switches and when
    the video counter starts running.
    This is just a hack so that SNYD/TCB works without breaking, for example,
    Mindbomb/No Shit.
    Or maybe there's yet another issue in TCB.

TCB:
-broken:
Line -30 - 372:S0000 456:S0000 480:r0900 500:r0900 512:T0100
Line -29 - 008:r0900 028:r0900 048:r0900 068:r0902 512:R0002
Line -28 - 008:R0000 372:S0000 380:S0002 440:R0002 452:R0000 512:R0002 508:T2009
-OK:
Line -30 - 372:S0000 456:S0000 480:r0900 500:r0900 512:T0100
Line -29 - 008:r0900 028:r0900 048:r0900 068:r0900 088:r090A
Line -28 - 004:R0002 012:R0000 376:S0000 384:S0002 444:R0002 456:R0000 508:T2009
NoShit:
- broken:
Line -30 - 416:S0000 512:T0100
Line -29 - 236:S0002 244:r0958 260:r0960 508:T0002
Line -28 - 008:R0002 016:R0000 380:S0000 388:S0002 448:R0002 460:R0000 512:T2001
-OK:
Line -30 - 432:S0000 512:T0100
Line -29 - 252:S0002 260:r0962 276:r096A 508:T0002
Line -28 - 004:R0002 012:R0000 376:S0000 384:S0002 444:R0002 456:R0000 512:T2011

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
    
#if defined(SS_SHIFTER_SDP_TRACE_LOG3) // compare with Steem (can't be 100%)
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

#if defined(SS_SHIFTER_SDP_TRACE_LOG2)
  if(scan_y==-29) TRACE_LOG("Read SDP F%d y%d c%d SDP %X (%d - %d) sdp %X\n",FRAME,scan_y,CyclesIn,sdp,sdp-shifter_draw_pointer_at_start_of_line,CurrentScanline.Bytes,shifter_draw_pointer);
#endif
  int nbytes=sdp-shifter_draw_pointer_at_start_of_line;
  return sdp;
}

#endif

#ifdef IN_EMU//temp

#ifdef SS_SHIFTER_SDP_WRITE

void TShifter::WriteSDP(MEM_ADDRESS addr, BYTE io_src_b) {
/*
    This is a difficult side of STE emulation, made difficult too by
    Steem's rendering system, that uses the shifter draw pointer as 
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

  int cycles=LINECYCLES; // cycle in shifter reckoning

#if defined(SS_SHIFTER_SDP_TRACE_LOG2)
  TRACE_LOG("F%d y%d c%d Write %X to %X\n",FRAME,scan_y,cycles,io_src_b,addr);
#endif
#if defined(SS_SHIFTER_EVENTS)
  VideoEvents.Add(scan_y,cycles,'w',((addr&0xF)<<8)|io_src_b);
#endif

#if defined(SS_STF)
  // some STF programs write to those addresses, it just must be ignored.
  if(ST_TYPE!=STE)
  {
#if defined(SS_SHIFTER_SDP_TRACE_LOG)
    TRACE_LOG("STF ignore write to SDP %x %x\n",addr,io_src_b);
#endif
//    return FALSE; // fixes Nightdawn
    return; // fixes Nightdawn, STF-only game
  }
#endif

  // some bits will stay at 0 in the STE whatever you write
  if(mem_len<=FOUR_MEGS && addr==0xFF8205) // already in Steem 3.2 (not in 3.3!)
    io_src_b&=0x3F; // fixes Delirious IV bugged STE detection routine
#if defined(SS_SHIFTER_SDP_WRITE_LOWER_BYTE)
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

#if defined(SS_SHIFTER_SDP_READ)
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
#if defined(SS_SHIFTER_SDP_TRACE_LOG)
      TRACE_LOG("F%d y%d c%d SDP %X reset middle byte from %X to %X\n",FRAME,scan_y,cycles,shifter_draw_pointer,current_sdp_middle_byte,SDPMiddleByte);
#endif
      DWORD_B(&shifter_draw_pointer,(0xff8209-0xff8207)/2)=SDPMiddleByte;
      middle_byte_corrected=TRUE;
    }
  }

  MEM_ADDRESS nsdp=shifter_draw_pointer; //sdp_real (should be!)
  DWORD_B(&nsdp,(0xff8209-addr)/2)=io_src_b;

  // Writing low byte while the shifter supposedly is fetching
  if(addr==0xFF8209 && de)
  {

#if defined(SS_SHIFTER_PACEMAKER)
    if(SSE_HACKS_ON&&(bytes_drawn==8||bytes_drawn==16)) 
    {
      nsdp+=bytes_drawn; // hack for Pacemaker credits line 70 flicker/shift
      overscan_add_extra-=bytes_drawn; 
    }
#endif

#if defined(SS_SHIFTER_DANGEROUS_FANTAISY)
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
      ASSERT(CurrentScanline.EndCycle==460); bytes_to_run+=2; // is it 460 or 464?
      overscan_add_extra+=-44+bytes_to_run-bytestofetch;
    }

    // cancel the Steem 3.2 fix for left off with STE scrolling on
    if(!ExtraAdded && (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      && HSCROLL>=12)
      overscan_add_extra+=8; // fixes bumpy scrolling in E605 Planet

#if defined(SS_SHIFTER_SDP_WRITE_DE_HSCROLL)
    if(SSE_HACKS_ON
      &&shifter_skip_raster_for_hscroll && !left_border && !ExtraAdded)
    {
      if(PreviousScanline.Tricks&TRICK_STABILISER)
#if defined(SS_SHIFTER_TEKILA)
        nsdp+=-2; // fixes D4/Tekila shift
#else
        ;
#endif
      else
#if defined(SS_CPU_PREFETCH_TIMING) || defined(CORRECTING_PREFETCH_TIMING)
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

#if defined(SS_SHIFTER_SDP_WRITE_MIDDLE_BYTE)
  // hack, record middle byte, programmer couldn't intend it to change
  if(SSE_HACKS_ON && fl && addr==0xff8207)  
    SDPMiddleByte=io_src_b; // fixes Pacemaker credits, Tekila
#endif

  CurrentScanline.Tricks|=TRICK_WRITE_SDP; // could be handy

  //return shifter_draw_pointer; // fancy
}

#endif

#endif//#ifdef IN_EMU

void TShifter::ShiftSDP(int shift) {
  shifter_draw_pointer+=shift; 
  overscan_add_extra-=shift;
}


#endif// defined(IN_EMU)


// just taking some unimportant code out of Render for clarity

#define   AUTO_BORDER_ADJUST  \
          if(!(border & 1)) { \
            if(scanline_drawn_so_far<BORDER_SIDE) { \
              border1-=(BORDER_SIDE-scanline_drawn_so_far); \
              if(border1<0){ \
                picture+=border1; \
                if(!screen_res) {  \
                  hscroll-=border1;  \
                  shifter_draw_pointer+=(hscroll/16)*8; \
                  hscroll&=15; \
                }else if(screen_res==1) { \
                  hscroll-=border1*2;  \
                  shifter_draw_pointer+=(hscroll/16)*4; \
                  hscroll&=15; \
                } \
                border1=0; \
                if(picture<0) picture=0; \
              } \
            } \
            int ta=(border1+picture+border2)-320; \
            if(ta>0) { \
              border2-=ta; \
              if(border2<0)  { \
                picture+=border2; \
                border2=0; \
                if (picture<0)  picture=0; \
              } \
            } \
            border1=border2=0; \
          }


#endif//#if defined(SS_SHIFTER)

#endif//define SSESHIFTER_H