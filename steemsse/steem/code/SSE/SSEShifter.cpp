#include "SSE.h"

#if defined(SSE_SHIFTER)

#include "../pch.h"
#include <conditions.h>
#include <display.decla.h>
#include <mfp.decla.h>
#include <palette.decla.h>
#include <run.decla.h>
#include <emulator.decla.h>
#include <cpu.decla.h>
#include "SSEFrameReport.h"
#include "SSEParameters.h"
#include "SSEVideo.h"
#include "SSEGlue.h"
#include "SSEMMU.h"
#include "SSEShifter.h"

#if  defined(SSE_SHIFTER_HSCROLL)
#define HSCROLL0 hscroll0
#else
#define HSCROLL0 HSCROLL
#endif


#define LOGSECTION LOGSECTION_VIDEO


TShifter::TShifter() {
#if defined(WIN32)
  ScanlineBuffer=NULL;
#endif
#ifdef SSE_SHIFTER_HIRES_RASTER
  Scanline2[112]=0;
#endif
}


TShifter::~TShifter() {
}


void TShifter::DrawScanlineToEnd()  {
  MEM_ADDRESS nsdp; 
#ifndef NO_CRAZY_MONITOR
  if (emudetect_falcon_mode!=EMUD_FALC_MODE_OFF){
    int pic=320*emudetect_falcon_mode_size,bord=0;
    // We double the size of borders too to keep the aspect ratio of the screen the same
#if defined(SSE_VID_BORDERS_GUI_392)
    if (border) bord=BORDER_SIDE*emudetect_falcon_mode_size;
#else
    if (border & 1) bord=BORDER_SIDE*emudetect_falcon_mode_size;
#endif
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
        ASSERT(shifter_draw_pointer<mem_len);
        ASSERT(shifter_draw_pointer+em_width*em_planes/8<mem_len);
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
    if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
    {
      // thsee variables are pointers to PC video memory
      draw_dest_ad=draw_dest_next_scanline;
      draw_dest_next_scanline+=draw_dest_increase_y;
    }
    shifter_pixel=HSCROLL; //start by drawing this pixel (note: for next line)
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
#if defined(SSE_GLUE_HIRES_OVERSCAN)
      if(freq_change_this_scanline)
      {
        Glue.CheckSideOverscan();
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
    Case: Time Slices, it works!
*/

#if defined(SSE_SHIFTER_HIRES_RASTER)
#if defined(SSE_STF_HW_OVERSCAN)
      if(SSEConfig.OverscanOn)
        Scanline2[112]=0; //quick fix
#endif
      if(HSCROLL0||Scanline2[112])
#else
      if(HSCROLL)
#endif
      {
        // save ST memory
        for(int i=0;i<=20;i++)
          Scanline[i]=LPEEK(i*4+shifter_draw_pointer); 
#if defined(SSE_SHIFTER_HIRES_RASTER)
        if(HSCROLL0)
#endif
        // shift pixels of full scanline
        for(MEM_ADDRESS i=shifter_draw_pointer; i<nsdp ;i+=2)
        {
          WORD new_value=DPEEK(i)<<HSCROLL0;  // shift first bits away
          new_value|=DPEEK(i+2)>>(16-HSCROLL0); // add first bits of next word at the end
          DPEEK(i)=new_value;
        }
      }
#endif

#ifdef SSE_SHIFTER_HIRES_RASTER
      if(Scanline2[112]) // apply raster effect!
      {
        for(int byte=0;byte<Scanline2[112];byte++)
          PEEK(shifter_draw_pointer+byte)
            =PEEK(shifter_draw_pointer+byte)^Scanline2[byte]; // using XOR
      }
#endif

      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
      {


        ASSERT(screen_res==2);
#if defined(SSE_SHIFTER_390)
        if(nsdp<=mem_len)
#elif defined(SSE_SHIFTER_382)
        if(nsdp<mem_len) // potential crash
#endif
#if defined(SSE_VID_BORDERS_GUI_392)
        if (border)
#else
        if(border & 1)
#endif
          ///////////////// RENDER VIDEO /////////////////
#if defined(SSE_GLUE_HIRES_OVERSCAN)
/*  This is used for one demo that removes borders in high res, and for
    hardware overscan.
*/
          if(Glue.CurrentScanline.Tricks&2)
#if defined(SSE_BUGFIX_393) //ugly bug was somehow caught by assembly routine, but not draw c
            draw_scanline((BORDER_SIDE*2)/16- BORDER_SIDE/8, 
            640/16+ BORDER_SIDE/4, (BORDER_SIDE*2)/16- BORDER_SIDE/8,0);
#elif defined(SSE_SHIFTER_382)
            draw_scanline((BORDER_SIDE*2)/16-1, 640/16+3, 0,0);
#else
            draw_scanline((BORDER_SIDE*2)/16-16, 640/16+16*2, (BORDER_SIDE*2)/16-16,0);
#endif
            //draw_scanline((BORDER_SIDE*2)/16, 640/16+16*2, (BORDER_SIDE*2)/16-16*2,0);
          else
#endif
          draw_scanline((BORDER_SIDE*2)/16, 640/16, (BORDER_SIDE*2)/16,0);
        else
          ///////////////// RENDER VIDEO /////////////////
          draw_scanline(0,640/16,0,0);
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }
#if defined(SSE_SHIFTER_STE_HI_HSCROLL) 
      if(
#if defined(SSE_GLUE_HIRES_OVERSCAN)
        draw_line_off ||
#endif        
#ifdef SSE_SHIFTER_HIRES_RASTER
        Scanline2[112] ||
#endif
        HSCROLL0) 
      {
        for(int i=0;i<=20;i++)// restore ST memory
          LPEEK(i*4+shifter_draw_pointer)=Scanline[i]; 
#ifdef SSE_SHIFTER_HIRES_RASTER
        if(Scanline2[112])
#ifdef UNIX
          memset(Scanline2,0,113);//ux382
#else
          ZeroMemory(Scanline2,113);
#endif
#endif
      }
#endif
      shifter_draw_pointer=nsdp;
    }
    else if(scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border)
    {
      if(scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line)
      {
        ASSERT(screen_res==2); //>?
#if defined(SSE_VID_BORDERS_GUI_392)
        if (border)
#else
        if(border & 1)
#endif
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


void TShifter::IncScanline() {
  scan_y++;
  HblPixelShift=0;  
  left_border=right_border=BORDER_SIDE;

/*  Don't shift scanlines by 4 pixcels when the left border is removed. 
    We must shift other scanlines in compensation, which is
    correct emulation. On the ST, the left border is larger than the right
    border. By directly changing left_border and right_border, we can get
    the right timing for palette effects.
    Cases: Backlash -TEX, Appendix 4pix plasma, Gobliins II -ICS...
*/
#if defined(SSE_VID_BORDERS)
  if(SideBorderSize==VERY_LARGE_BORDER_SIDE && border)
    left_border+=4,right_border-=4;
#endif
  if(HSCROLL) 
    left_border+=16;
#if defined(SSE_SHIFTER_HSCROLL)
  hscroll0=HSCROLL; // save
#endif
  if(shifter_hscroll_extra_fetch) 
    left_border-=16;
}


void TShifter::Render(int cycles_since_hbl,int dispatcher) {
  // this is based on Steem's 'draw_scanline_to'
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
  if(screen_res>=2 || scan_y>247) // bugfix stf1pix_512k
#else
  if(screen_res>=2) 
#endif
    return; 

#ifndef NO_CRAZY_MONITOR
  if(extended_monitor || emudetect_falcon_mode!=EMUD_FALC_MODE_OFF) 
    return;
#endif

  if(Glue.FetchingLine()&&(freq_change_this_scanline
#if defined(SSE_SHIFTER_UNSTABLE)
    ||Preload
#endif
    ))
    Glue.CheckSideOverscan();

/*  What happens here is very confusing; we render in real time, but not
    quite. As on a real ST, there's a delay of fetch+8 between the 
    'Shifter cycles' and the 'palette reading' cycles. 
    We can't render at once because the palette could change. 
    CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN (84) takes care of that delay. 
    This causes all sort of trouble because our SDP is late while rendering,
    and sometimes forward!
*/
#if defined(MMU_PREFETCH_LATENCY)
  // this may look impressive but it's just a bunch of hacks!
  switch(dispatcher) {
  case DISPATCHER_CPU:
    cycles_since_hbl+=MMU_PREFETCH_LATENCY;
    break;
  case DISPATCHER_SET_PAL:
#if defined(SSE_SHIFTER_PALETTE_TIMING)
#if defined(SSE_MMU_WU_PALETTE_STE)
#if defined(SSE_SHIFTER_393) // difficult choice!!!
    if((ST_TYPE==STF || MMU.WU[OPTION_WS]==2)) // dots in Overscan demos frequent on STE
#else
    if(!(ST_TYPE==STE && MMU.WU[OPTION_WS]==2)) // it really is another WU, to check Spectrum 512 pics
#endif
#endif
      cycles_since_hbl++;
#endif
    //60hz line with border, HSCROLL or not
#if defined(SSE_GLUE_392E)
    if(Glue.CurrentScanline.StartCycle==GLU_DE_ON_60
      || Glue.CurrentScanline.StartCycle==GLU_DE_ON_60-16) //TODO
#elif defined(SSE_GLUE_392A)
    // or we would need another variable to record 'choose' timing
    if(Glue.CurrentScanline.StartCycle== ((shifter_hscroll_extra_fetch) // or HSCROLL? TODO
      ? Glue.ScanlineTiming[TGlue::CHOOSE_FREQ][TGlue::FREQ_60]
      : Glue.ScanlineTiming[TGlue::LINE_START][TGlue::FREQ_60]))
#else
    if(Glue.CurrentScanline.StartCycle==52 || Glue.CurrentScanline.StartCycle==36)
#endif
      cycles_since_hbl+=4; // it's a girl 2 bear //TODO better way?
    break;
  case DISPATCHER_SET_SHIFT_MODE:
    RoundCycles(cycles_since_hbl); // eg Drag/Happy Islands, Cool STE, Bees (...)
    break; 
#if defined(SSE_SHIFTER_RENDER_SYNC_CHANGES)//no
  case DISPATCHER_SET_SYNC:
    RoundCycles(cycles_since_hbl); 
    cycles_since_hbl++;
    break;
#endif
  case DISPATCHER_WRITE_SDP:
    RoundCycles(cycles_since_hbl); // eg Riverside rainbow
    break;
  }//sw
#endif

  int pixels_in=cycles_since_hbl-(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-BORDER_SIDE);
      
  if(pixels_in > BORDER_SIDE+320+BORDER_SIDE) 
    pixels_in=BORDER_SIDE+320+BORDER_SIDE; 

  // this is the most tricky part of our hack for large border/no shift on left off...
  int pixels_in0=pixels_in;
#if defined(SSE_VID_BORDERS)
  if(SideBorderSize==VERY_LARGE_BORDER_SIDE && border && pixels_in>0)
    pixels_in+=4;
#endif
  if(pixels_in>=0) // time to render?
  {
#ifdef WIN32 // prepare buffer & ASM routine
    if(pixels_in>416)
      pixels_in=pixels_in0;

    if(draw_buffer_complex_scanlines && draw_lock)
    {
      if(scan_y>=draw_first_scanline_for_border  
#if defined(SSE_VID_BORDERS_BIGTOP) && !defined(SSE_VID_BORDERS_BIGTOP_381)
        // avoid horrible crash //381:?? 
          && (DISPLAY_SIZE<BIGGEST_DISPLAY || scan_y>=draw_first_scanline_for_border+ 
               (BIG_BORDER_TOP-ORIGINAL_BORDER_TOP))
#endif
        && scan_y<draw_last_scanline_for_border)
      {
        if(draw_store_dest_ad==NULL && pixels_in0<=BORDER_SIDE+320+BORDER_SIDE)
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
    if(Glue.FetchingLine())
    {
      if(left_border<0) // shouldn't happen but sometimes it does
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

#if defined(SSE_SHIFTER_LINE_MINUS_2_DONT_FETCH)
/*  On lines -2, don't fetch the last 2 bytes as if it was a 160byte line.
    Fixes screen #2 of the venerable B.I.G. Demo.
*/
      if((Glue.CurrentScanline.Tricks&TRICK_LINE_MINUS_2)
        && picture>=SHIFTER_RASTER*2)
      {
        picture-=SHIFTER_RASTER*2,border2+=SHIFTER_RASTER*2; // cancel last raster
      }
#endif
      if(!screen_res) // LOW RES
      {
        hscroll=old_shifter_pixel & 15;
        nsdp-=(old_shifter_pixel/16)*8;
        nsdp+=(shifter_pixel/16)*8; // is sdp forward at end of line?
        
#if defined(SSE_SHIFTER_4BIT_SCROLL)
/*  This is where we do the actual shift for those rare programs using the
    overscan 4bit hardscroll trick (PYM/ST-CNX,D4/Nightmare,D4/NGC).
    Notice it is quite simple, and also very efficient because it uses 
    the hscroll parameter of the assembly drawing routine (programmed by
    Steem authors, of course).
    hscroll>15 is handled further.
    TODO: be able to shift the line by an arbitrary #pixels left ot right
    (assembly)
*/
        if(Glue.CurrentScanline.Tricks
#if defined(SSE_GLUE_393)
          &(TRICK_4BIT_SCROLL))
#else
          &(TRICK_4BIT_SCROLL|TRICK_LINE_PLUS_20|TRICK_UNSTABLE)) // as refactored in 380
#endif
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
          + (DISPLAY_SIZE>BIGGEST_DISPLAY?6:0)  //TODO: never...
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
          if(nsdp<=mem_len) // safety
            draw_scanline(border1,picture,border2,hscroll); 
        }
      }
      shifter_draw_pointer=nsdp;
    }
    // overscan lines = a big "left border"
    else if(scan_y>=draw_first_scanline_for_border 
      && scan_y<draw_last_scanline_for_border)
    {
      int border1; // the only var. sent to draw_scanline
      int left_visible_edge,right_visible_edge;
      // Borders on
#if defined(SSE_VID_BORDERS_GUI_392)
      if (border)
#else
      if(border & 1)
#endif
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
      {
      ///////////////// RENDER VIDEO /////////////////        
        draw_scanline(border1,0,0,0);
      }
    }
    scanline_drawn_so_far=pixels_in;
  }//if(pixels_in>=0)
}

#pragma warning(disable:4100)//'Cold' : unreferenced formal parameter

void TShifter::Reset(bool Cold) {
  m_ShiftMode=screen_res;// we know it's updated first, debug strange screen!

#if defined(SSE_SHIFTER_TRICKS)
  for(int i=0;i<32;i++)
  {
    shifter_freq_change[i]=0; // interference Leavin' Teramis/Flood on PP43
    shifter_shift_mode_change_time[i]=-1;
  }
#endif

#if defined(SSE_DEBUG)
  Debug.ShifterTricks=0;
  nVbl=0; // cold or warm
#endif

#if defined(SSE_SHIFTER_UNSTABLE)
  Preload=0;
#endif
}

#pragma warning(default:4100)


void TShifter::Vbl() {
  // called at the very end of event_vbl_interrupt()

#if defined(SSE_DEBUG)
  nVbl++; 
#if defined(SSE_BOILER_FRAME_REPORT)
  FrameEvents.Vbl(); 
#endif
#endif//dbg

#if defined(SSE_SHIFTER_UNSTABLE)
  HblPixelShift=0;
#endif

}

#undef LOGSECTION

#define LOGSECTION LOGSECTION_VIDEO_RENDERING

#ifdef WIN32

void TShifter::DrawBufferedScanlineToVideo() {
  // replacing macro DRAW_BUFFERED_SCANLINE_TO_VIDEO
  if(draw_store_dest_ad)
  { 
    // Bytes that will be copied.
    int amount_drawn=(int)(draw_dest_ad-draw_temp_line_buf); 

#if defined(SSE_VID_ANTICRASH_392)
    // Don't access video memory beyond the surface, it causes a crash
    if(draw_store_dest_ad+amount_drawn>draw_mem+Disp.VideoMemorySize)
    {
      TRACE_LOG("Video memory overflow\n");
#ifdef SSE_BUGFIX_393
      /////draw_end();
#endif
      return;
    }
#endif

    // From draw_temp_line_buf to draw_store_dest_ad
    DWORD *src=(DWORD*)draw_temp_line_buf; 
    DWORD *dest=(DWORD*)draw_store_dest_ad;  
    while(src<(DWORD*)draw_dest_ad)
      *(dest++)=*(src++); 
//    ASSERT(draw_med_low_double_height);//OK, never asserted
    if(draw_med_low_double_height) //390 on my system... you never know
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


void TShifter::RoundCycles(int& cycles_in) { //TODO
  cycles_in-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !HSCROLL) 
    cycles_in+=16;
  cycles_in+=SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in&=-SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !HSCROLL) 
    cycles_in-=16;
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_VIDEO


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

#if defined(SSE_STF_PAL)
  if(ST_TYPE!=STE)
    NewPal &= 0x0777; // fixes Forest HW STF test
  else
#endif
    NewPal &= 0x0FFF;
    if(STpal[n]!=NewPal)
    {
      int CyclesIn=LINECYCLES;
#if defined(MMU_PREFETCH_LATENCY)
      if(draw_lock && CyclesIn>MMU_PREFETCH_LATENCY+SHIFTER_RASTER_PREFETCH_TIMING)
#else
      if(draw_lock)
#endif
        Shifter.Render(CyclesIn,DISPATCHER_SET_PAL);

#if defined(SSE_SHIFTER_HIRES_RASTER) && defined(SSE_GLUE)
/*  v3.8.0 
    Record when palette is changed during display, so that we apply effect
    at rendering time (there's no real-time rendering for HIRES).
    Limitations: 
    -Our trick uses video RAM, so this limits the field to real picture.
    -Resolution is byte, not bit/cycle. It seems OK for Time Slice.
*/
      if(screen_res==2 && Glue.FetchingLine())
      {
        int time_to_first_pixel
          =Glue.ScanlineTiming[TGlue::LINE_START][TGlue::FREQ_72]+28-6;
        int cycle=CyclesIn-time_to_first_pixel;
        if(cycle>=0 && cycle<160) // only during Shifter DE
        {
          int byte=cycle/2; // HIRES: 4 pixels/cycle
          for(int i=Scanline2[112];i<=byte;i++) // from previous pixel chunk to now
            Scanline2[i]=(NewPal&1)?0xFF:0x00; // and not the reverse...
          Scanline2[112]=byte; //record where we are
        }
      }
#endif
      STpal[n]=NewPal;
      PAL_DPEEK(n*2)=STpal[n];
      if(!flashlight_flag && !draw_line_off DEBUG_ONLY(&&!debug_cycle_colours))
        palette_convert(n);
    }
}

#undef LOGSECTION

#endif//#if defined(SSE_SHIFTER)
