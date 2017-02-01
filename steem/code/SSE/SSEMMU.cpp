#include "SSE.h"

#if defined(SSE_MMU)

#include "../pch.h"
#include <cpu.decla.h>

#include "SSEVideo.h"
#include "SSEMMU.h"
#include "SSEGlue.h"
#include "SSEFrameReport.h"

#if  defined(SSE_SHIFTER_HSCROLL)
#define HSCROLL0 Shifter.hscroll0
#else
#define HSCROLL0 HSCROLL
#endif


#define LOGSECTION LOGSECTION_VIDEO

////////////////////////
// MMU: Video counter //
////////////////////////

/*  The video counter registers are inside the MMU.
    The MMU is tasked with fetching video memory and putting
    it in the Shifter.

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
*/

#ifdef SSE_UNIX
#define min(a,b) (a>b ? b:a)
#define max(a,b) (a>b ? a:b)
#endif

/*  "ReadVideoCounter" has been refactored in case we would want
    to use it to recompute the video counter at each scanline (but we don't).
    Now we have a member variable VideoCounter and a function that
    updates it.
    CurrentScanline.StartCycle is mostly correct now, so it simplifies
    the function further.
    This is an extremely important function for Atari ST emulation because a lot
    of programs (demos essentially) synchronise on the video counter.
*/

void TMMU::UpdateVideoCounter(int CyclesIn) {

  MEM_ADDRESS vc;
  if (bad_drawing){  // Fake SDP, eg extended monitor
    if (scan_y<0)
      vc=xbios2;
    else if (scan_y<shifter_y){
      int line_len=(160/res_vertical_scale);
      vc=xbios2 + scan_y*line_len + min(CyclesIn/2,line_len) & ~1;
    }else
      vc=xbios2+32000;
  }
  else if(Glue.FetchingLine()) // lines where the counter actually moves
  {
    Glue.CheckSideOverscan(); // this updates Bytes and StartCycle
    int bytes_to_count=Glue.CurrentScanline.Bytes;

    // 8 cycles latency before MMU starts prefetching
    int starts_counting=(Glue.CurrentScanline.StartCycle+MMU_PREFETCH_LATENCY)/2;

    // can't be odd though (hires)
    starts_counting&=-2;

    // compute vc
    int c=CyclesIn/2-starts_counting;
    vc=shifter_draw_pointer_at_start_of_line;
    if(!bytes_to_count)
      ; // 0-byte lines
    else if(c>=bytes_to_count)
    {
      vc+=bytes_to_count;
      // The timing of this is a strange thing on a real STE - TODO
      if(ST_TYPE==STE
        && CyclesIn>=Glue.CurrentScanline.EndCycle + (HSCROLL0?WordsToSkip*2:4))
        vc+=(LINEWID+WordsToSkip)*2;
    }
    else if (c>=0){
      c&=-2;
      vc+=c;
    }
  }
  else // lines witout fetching (before or after frame)
    vc=shifter_draw_pointer_at_start_of_line;

  VideoCounter=vc; // update member variable
}


MEM_ADDRESS TMMU::ReadVideoCounter(int CyclesIn) {
  UpdateVideoCounter(CyclesIn);
  return VideoCounter;
}


void TMMU::ShiftSDP(int shift) { // we count on the compiler to optimise this...
  shifter_draw_pointer+=shift; 
}

/*  Now that video counter reckoning (MMU.VideoCounter) is separated from 
    rendering (shifter_draw_pointer), a lot of cases that seemed complicated
    are simplified. Most hacks could be removed.

    Also, a simple test program allowed us to demystify writes to the video
    counter. It is actually straightforward, here's the rule:
    The byte in the MMU register is replaced with the byte on the bus, that's
    it, even if the counter is running at the time (Display Enable), and
    whatever words are in the Shifter.
    It's logical after all. The video counter resides in the MMU and the
    Shifter never sees it.
    TODO: upper bytes could be impacted? But we had apparent cases before, not now
*/

void TMMU::WriteVideoCounter(MEM_ADDRESS addr, BYTE io_src_b) {

  int CyclesIn=LINECYCLES;

#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SDP_WRITE)
    FrameEvents.Add(scan_y,LINECYCLES,'C',((addr&0xF)<<8)|io_src_b);
#endif

#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK3 & OSD_CONTROL_WRITESDP) 
    TRACE_OSD("VC%d y%d",addr-0xff8200,scan_y);
#endif

#if defined(SSE_STF)
  // some STF programs write to those addresses, it just must be ignored.
  if(ST_TYPE!=STE)
    return; // eg Nightdawn
#endif

  // some bits will stay at 0 in the STE whatever you write
  if(mem_len<=FOUR_MEGS && addr==0xFF8205) 
    io_src_b&=0x3F; // eg Delirious IV
  else if(addr==0xFF8209)
    io_src_b&=0xFE; // eg RGBeast

  bool fl=Glue.FetchingLine();

  if(fl)
    Shifter.Render(CyclesIn,DISPATCHER_WRITE_SDP);

  UpdateVideoCounter(CyclesIn);
  const MEM_ADDRESS former_video_counter=VideoCounter;

  // change appropriate byte
  DWORD_B(&VideoCounter,(0xff8209-addr)/2)=io_src_b;

#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK_14 & TRACE_CONTROL_VIDEO_COUNTER)
      TRACE_VID("y%d c%d %X=%02X %06X -> %06X (%06X -> %06X)\n",
      scan_y,CyclesIn,addr,io_src_b,former_video_counter,VideoCounter,shifter_draw_pointer_at_start_of_line,shifter_draw_pointer_at_start_of_line+VideoCounter-former_video_counter);
#endif

  // update shifter_draw_pointer_at_start_of_line
  shifter_draw_pointer_at_start_of_line-=former_video_counter; 
  shifter_draw_pointer_at_start_of_line+=VideoCounter; 

  // updating the video counter while video memory is still being fetched
  // could cause display artefacts because of shifter latency (eg Tekila)
  bool fetching=fl 
    && CyclesIn>=Glue.CurrentScanline.StartCycle +MMU_PREFETCH_LATENCY
    && CyclesIn<Glue.CurrentScanline.EndCycle+MMU_PREFETCH_LATENCY;
 if(!fetching)
    shifter_draw_pointer=VideoCounter;
}

#endif//#if defined(SSE_MMU)
