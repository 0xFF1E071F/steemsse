
#include "../pch.h"
#include "SSEMMU.h"
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#include "SSEGlue.h"
#include "SSEFrameReport.h"
#endif

#if defined(SSE_MMU)

#if defined(SSE_MMU_WU_DL)
/*
+------------------------------------------------------------+---------------+
| Steem  option    |              Wake-up concepts           |    Cycle      |
|    variable      |                                         |  adjustment   |
+------------------+---------------+------------+------------+-------+-------+
|  WAKE_UP_STATE   |   DL Latency  |     WU     |      WS    |  MODE |  SYNC |
|                  |     (Dio)     |    (ijor)  |    (LJBK)  | (Res) |(Freq) |
+------------------+---------------+------------+------------+-------+-------+
|   0 (ignore)     |      5        |     -      |      -     |    -  |    -  |
|        1         |      3        |   2 (warm) |      2     |   +2  |   +2  |
|        2         |      4        |     2      |      4     |    -  |   +2  |
|        3         |      5        |   1 (cold) |      3     |    -  |    -  |
|        4         |      6        |     1      |      1     |   -2  |    -  |
+------------------+---------------+------------+------------+-------+-------+
*/
// yes we can:  WU - WS - MODE - SYNC
TMMU MMU={{0,2,2,1,1,2},{0,2,4,3,1,2},{0,2,0,0,-2,2},{0,2,2,0,0,2}};

#else
TMMU MMU;
#endif

#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_MMU1)
/*  The video counter registers are inside the MMU.
    The MMU is tasked with fetching video memory and putting
    it in the Shifter.
    Because the counter is used by Steem to render, it is called 
    shifter_draw_pointer (SDP). The dual nature of the variable may
    be trouble.
    As for GLU/Shifter tricks, this is the opportunity to simplify code.
    We assume: 
    - SSE_SHIFTER_SDP_READ, SSE_SHIFTER_SDP_WRITE, SSE_CPU_PREFETCH_TIMING,
    SSE_SHIFTER_STE_READ_SDP_SKIP, SSE_GLUE_FRAME_TIMINGS defined
    - SSE_SHIFTER_STE_READ_SDP_HSCROLL1 undefined
    - version >= 380
*/
#define LOGSECTION LOGSECTION_VIDEO

//////////////////////////
// Shifter draw pointer //
//////////////////////////

/* 
    Video Address Counter:

    STF - read only, writes ignored
    ff 8205   R               |xxxxxxxx|   Video Address Counter High
    ff 8207   R               |xxxxxxxx|   Video Address Counter Mid
    ff 8209   R               |xxxxxxxx|   Video Address Counter Low

    STE - read & write

    FF8204 ---- ---- --xx xxxx (High)
    FF8206 ---- ---- xxxx xxxx
    FF8208 ---- ---- xxxx xxx- (Low)

    Video Address Counter.
    Now read/write. Allows update of the video refresh address during the 
    frame. 
    The effect is immediate, therefore it should be reloaded carefully 
    (or during blanking) to provide reliable results. 
    Of course, some programs modify the counter during display, which
    is difficult and funny to emulate.
*/

#ifdef SSE_UNIX
#define min(a,b) (a>b ? b:a)
#define max(a,b) (a>b ? a:b)
#endif

MEM_ADDRESS TMMU::ReadSDP(int CyclesIn,int dispatcher) {
  // the function has been greatly streamlined in v3.8.0,
  // relying on info already in "CurrentScanline" (no WS modifiers)
 
  MEM_ADDRESS sdp; // return value
  if (bad_drawing){  // Fake SDP, eg extended monitor
    if (scan_y<0)
      sdp=xbios2;
    else if (scan_y<shifter_y){
      int line_len=(160/res_vertical_scale);
      sdp=xbios2 + scan_y*line_len + min(CyclesIn/2,line_len) & ~1;
    }else
      sdp=xbios2+32000;
  }
  else if(Glue.FetchingLine()) // when counter actually moves
  {
    Glue.CheckSideOverscan(); // updates Bytes and StartCycle
    int bytes_to_count=Glue.CurrentScanline.Bytes;
    if(bytes_to_count && shifter_skip_raster_for_hscroll)
    {
      ASSERT(ST_TYPE==STE);
      bytes_to_count+=SHIFTER_RASTER; // raster size depends on shift mode
    }
#if defined(SSE_SHIFTER_HIRES_OVERSCAN)
    bool hires=!left_border||screen_res==2;
#else
    bool hires=!left_border;
#endif
    // 8 cycles latency before MMU starts prefetching, 4 more after prefetch
    int starts_counting=(Glue.CurrentScanline.StartCycle+8)/2;

    // can't be odd though (hires)
    if(starts_counting&1)
      starts_counting-=1;

    // starts earlier if HSCROLL
    if(shifter_hscroll_extra_fetch)
      starts_counting-=(hires?2:8);

    int c=CyclesIn/2-starts_counting;
    sdp=shifter_draw_pointer_at_start_of_line;
    if (c>=bytes_to_count)
      sdp+=bytes_to_count+(LINEWID*2);
    else if (c>=0){
      c&=-2;
      sdp+=c;
    }
  }
  else // lines witout fetching (before or after frame)
    sdp=shifter_draw_pointer;

  return sdp;
}


void TMMU::ShiftSDP(int shift) { 
  shifter_draw_pointer+=shift; 
  overscan_add_extra-=shift;
}


void TMMU::WriteSDP(MEM_ADDRESS addr, BYTE io_src_b) {
/*
    This is a difficult side of STE emulation, made difficult too by
    Steem's rendering system, that uses the Shifter draw pointer as 
    video pointer as well as ST IO registers. Other emulators may do 
    otherwise, but there are difficulties and imprecisions anyway.
    So this is mainly a collection of hacks for now, that handle pratical
    cases.
    TODO: examine interaction write SDP/left off in Steem

    rechecked those cases for v3.8.0

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
    Probably sthg else

    Braindamage 3D maze
    Line 117 - 016:r0900 036:r0900 056:r0900 076:r0904 456:r0776 456:w0775 476:P1002 484:P2002 492:P3002 500:P4002 508:P5002 512:T1000
    ...

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
    [for some reason, hack no more necessary]

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
-29 - 008:S0082 124:c091E 272:C0900 508:R0082 512:T4000 512:#0160
-28 - 012:R0000 376:S0000 388:S0082 408:C0506 424:C07A2 436:C090E 444:R0082 456:R0000 468:H000E 508:R0082 512:T6011 512:#0230
-27 - 012:R0000 376:S0000 388:S0082 408:C0506 424:C07A2 436:C09FE 444:R0082 456:R0000 468:H000E 508:R0082 512:T6011 512:#0230

    note that other lines must be aligned with -28: the shift is
    different because of HSCROLL=0 when line -28 starts

    instruction used move.b

*/

  int cycles=LINECYCLES; // cycle in Shifter reckoning

#ifdef SSE_DEBUG
#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK3 & OSD_CONTROL_WRITESDP) 
#else
  if(TRACE_ENABLED)
#endif
    TRACE_OSD("WRITE SDP");  
#if defined(SSE_DEBUG_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SDP_WRITE)
    FrameEvents.Add(scan_y,LINECYCLES,'C',((addr&0xF)<<8)|io_src_b);
#endif
#endif//dbg

  Glue.CurrentScanline.Tricks|=TRICK_WRITE_SDP;

#if defined(SSE_STF_SDP)
  // some STF programs write to those addresses, it just must be ignored.
  if(ST_TYPE!=STE)
    return; // fixes Nightdawn
#endif

  // some bits will stay at 0 in the STE whatever you write
  if(mem_len<=FOUR_MEGS && addr==0xFF8205) 
    io_src_b&=0x3F; // fixes Delirious IV bugged STE detection routine (was OK in Steem 3.2)
#if defined(SSE_SHIFTER_SDP_WRITE_LOWER_BYTE)
  else if(addr==0xFF8209)
    io_src_b&=0xFE; // fixes RGBeast mush (yoho!)
#endif

  bool fl=Glue.FetchingLine();
  bool de_started=fl && cycles>=Glue.CurrentScanline.StartCycle;
  bool de_finished=de_started && cycles>Glue.CurrentScanline.EndCycle;
  bool de=de_started && !de_finished;

  if(de_finished && addr==0xff8209)
    Glue.CurrentScanline.Tricks|=TRICK_WRITE_SDP_POST_DE; // for our hacks!

  Shifter.Render(cycles,DISPATCHER_WRITE_SDP); //rounding trouble

  MEM_ADDRESS sdp_real=ReadSDP(cycles,DISPATCHER_WRITE_SDP); 
  int bytes_in=sdp_real-shifter_draw_pointer_at_start_of_line;
  ASSERT(bytes_in>=0);
  // the 'draw' pointer can be <, = or > 'real'
  int bytes_drawn=shifter_draw_pointer-shifter_draw_pointer_at_start_of_line;
  ASSERT(bytes_drawn>=0);

  if(addr==0xff8209 && SDPMiddleByte!=999) // it has been set?
  {
    ASSERT(SSE_HACKS_ON);
    int current_sdp_middle_byte=(shifter_draw_pointer&0xFF00)>>8;
    if(current_sdp_middle_byte != SDPMiddleByte) // need to restore?
    {
#if defined(SSE_SHIFTER_SDP_TRACE_LOG)
      TRACE_LOG("F%d y%d c%d SDP %X reset middle byte from %X to %X\n",FRAME,scan_y,cycles,shifter_draw_pointer,current_sdp_middle_byte,SDPMiddleByte);
#endif
      DWORD_B(&shifter_draw_pointer,(0xff8209-0xff8207)/2)=SDPMiddleByte;
    }
  }

  MEM_ADDRESS nsdp=shifter_draw_pointer; //sdp_real (should be!)
#if defined(SSE_GLUE_001)
  nsdp=sdp_real;//test: problem only with Tekila?
#endif
  DWORD_B(&nsdp,(0xff8209-addr)/2)=io_src_b;

  // Writing low byte while the MMU is fetching
  if(addr==0xFF8209 && de)
  {

    // recompute right off bonus bytes (E605 Planet, D4/Tekila)
    // that's no hack by itself, but the method could be...
    if(!right_border && !Glue.ExtraAdded)
    {
      int pxtodraw=320+BORDER_SIDE*2-scanline_drawn_so_far;
      int bytestofetch=pxtodraw/2;
      //int bytes_to_run=(Glue.CurrentScanline.EndCycle-cycles)/2;
      int bytes_to_run=(464-cycles)/2; // this or round up cycles
      ASSERT(!(bytes_to_run&1));//!
#if defined(SSE_GLUE_001)
      overscan_add_extra=bytes_to_run-bytestofetch;
#else
      // we can't set extra because of bumpy scrolling
      overscan_add_extra+=-28+bytes_to_run-bytestofetch;
#endif
      if(SSE_HACKS_ON&&!shifter_skip_raster_for_hscroll)
        overscan_add_extra+=6; // Tekila line -28
    }
#if !defined(SSE_GLUE_001)
    // cancel the Steem 3.2 fix for left off with STE scrolling on
    if(!Glue.ExtraAdded && (Glue.CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      && HSCROLL>=12
#if defined(SSE_VID_BORDERS_416_NO_SHIFT) 
      // don't try to understand this, I don't
#if defined(SSE_VID_BORDERS_416_NO_SHIFT2)
      && (SideBorderSize!=VERY_LARGE_BORDER_SIDE||!border)
#else
      && (!SSE_HACKS_ON||SideBorderSize!=VERY_LARGE_BORDER_SIDE||!border)
#endif
#endif
      )
      overscan_add_extra+=8; // fixes bumpy scrolling in E605 Planet
#endif//001
#if defined(SSE_SHIFTER_SDP_WRITE_DE_HSCROLL) && defined(SSE_SHIFTER_TEKILA)
    // TODO, those conditions are certainly not correct
    if(SSE_HACKS_ON && shifter_skip_raster_for_hscroll && !left_border 
      && !right_border && !Glue.ExtraAdded 
      && (Glue.PreviousScanline.Tricks&TRICK_STABILISER)) //especially this
      nsdp+=4; // Tekila other lines
#endif
  }

#if defined(SSE_SHIFTER_DANGEROUS_FANTAISY)
  if(SSE_HACKS_ON
    && bytes_drawn==Glue.CurrentScanline.Bytes-8 && bytes_in>bytes_drawn)
    nsdp+=-8; // hack for Dangerous Fantaisy credits lower overscan flicker
#endif

  // update shifter_draw_pointer_at_start_of_line or ReadSDP returns garbage
  shifter_draw_pointer_at_start_of_line-=shifter_draw_pointer; //sdp_real
  shifter_draw_pointer_at_start_of_line+=nsdp;
  shifter_draw_pointer=nsdp;

#if defined(SSE_SHIFTER_SDP_WRITE_MIDDLE_BYTE)
  // hack, record middle byte, programmer couldn't intend it to change
  // note for Cool STE it flickers on some real STE
  if(SSE_HACKS_ON && fl && addr==0xff8207)  
    SDPMiddleByte=io_src_b; // fixes Tekila large display, Cool STE
#endif

}

#endif//#if !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_MMU1)

#endif//#if defined(SSE_MMU)
