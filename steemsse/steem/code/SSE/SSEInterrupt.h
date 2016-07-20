#pragma once
#ifndef SSEINTERRUPT_H
#define SSEINTERRUPT_H

#include "SSESTF.h"
#include "SSEGlue.h"

#if defined(SSE_INT_MFP_RATIO) 
extern double CpuMfpRatio;
extern DWORD CpuNormalHz;
#if defined(SSE_INT_MFP_RATIO_OPTION)
extern DWORD CpuCustomHz;
#endif
#endif

#if defined(SSE_INTERRUPT)

#if defined(SSE_INT_VBL_STF)
extern int HblTiming;
#endif

#if defined(IN_EMU) // argh! using the ugly Steem trick "IN_EMU"

#if !defined(SSE_CPU)
void m68k_interrupt(MEM_ADDRESS ad);
#endif

#if defined(SSE_INT_JITTER) //no
extern int HblJitter[],VblJitter[];
extern int HblJitterIndex,VblJitterIndex;
#endif

#undef LOGSECTION
#define LOGSECTION LOGSECTION_INTERRUPTS

#if defined(SSE_INT_HBL) 
#if defined(SSE_INT_HBL_INLINE)

// no more multiple defines, all switches in inline function
// not sure it's inlined, nor that it should be

inline void HBLInterrupt() {
// BRK(yo);
  ASSERT(hbl_pending);
  hbl_pending=false;
  log_to_section(LOGSECTION_INTERRUPTS,Str("INTERRUPT: HBL at PC=")+HEXSl(pc,6)+" "+scanline_cycle_log());
#ifdef SSE_DEBUG
  Debug.nHbis++; // knowing how many in the frame is interesting
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("HBI");
#endif
#if defined(SSE_BOILER_FRAME_INTERRUPTS)
  Debug.FrameInterrupts|=2;
#endif
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
    FrameEvents.Add(scan_y,LINECYCLES,'I',0x20);
#endif
  TRACE_LOG("%d %d %d (%d) HBI #%d Vec %X\n",TIMING_INFO,ACT,Debug.nHbis,LPEEK(0x0068));
#endif//dbg

#if !defined(SSE_CPU_UNSTOP2) //TODO proper switches
  if (cpu_stopped)
    M68K_UNSTOP;
#endif

#if defined(SSE_INT_HBL_IACK2) && !defined(SSE_INT_HBL_380)
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME; //move this after (undefined) wobble
#endif

  // wobble?
#if !defined(SSE_INT_JITTER_HBL)
//#if defined(SSE_INT_E_CLOCK)
  if(!OPTION_C1) // not if mode "E-Clock"
//#endif
  {
    INTERRUPT_START_TIME_WOBBLE; //Steem 3.2
  }
#endif

#if defined(SSE_INT_HBL_IACK2) && defined(SSE_INT_HBL_380)
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME; //before e-clock cycles
#endif

  // E-clock?
#if defined(SSE_INT_E_CLOCK)
  if(OPTION_C1)
  {

#if defined(SSE_INT_CHECK_BEFORE_PREFETCH)
/*  3.8.0
    On some instructions, interrupt is triggered before prefetch.
    It makes sense as the prefetch is useless.
    Maybe it is indicated by 'dbi' in US patent 4,325,121.
    It explains why some interrupts seem to take too much time in
    emulation.
    Only partial now, TST/HBI (for a fix)
    Update 3.8.1
    It was another of those last-minute insights, can't help it.
    A generalisation of this seems incorrect, but we'll keep it
    until it breaks something or we find a better fix.
*/
    if(OPTION_HACKS && M68000.PrefetchClass==1 && LINECYCLES>=4)
    {
      if((ir&0xFF00)==0x4A00 && (ir&0xFFC0)!=0x4AC0) //TST
      {
        INSTRUCTION_TIME(-4); // fixes Phantom end scroller
        //TRACE_OSD("-4");
      }
    }
#endif//SSE_INT_CHECK_BEFORE_PREFETCH  

#if defined(SSE_CPU_E_CLOCK_370) 
    int current_cycles=ACT;
    INSTRUCTION_TIME(ECLOCK_AUTOVECTOR_CYCLE);
    BYTE e_clock_wait_states=
#endif
#ifdef SSE_CPU_E_CLOCK_DISPATCHER
    M68000.SyncEClock(TM68000::ECLOCK_HBL);
#else
    M68000.SyncEClock();
#endif
#if defined(SSE_CPU_E_CLOCK_370)
    INSTRUCTION_TIME(current_cycles-ACT);
    INSTRUCTION_TIME(e_clock_wait_states);
#endif
  }
#endif

#if !defined(SSE_INT_HBL_IACK2) 
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME;
#endif

#if defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
  INSTRUCTION_TIME_ROUND(SSE_INT_HBL_TIMING-12); 
#else
  INSTRUCTION_TIME_ROUND(SSE_INT_HBL_TIMING); 
#endif

  // jitter?
#if defined(SSE_INT_JITTER_HBL) //no
//#if defined(SSE_INT_E_CLOCK)
  if(!OPTION_C1)
//#endif
    INSTRUCTION_TIME(HblJitter[HblJitterIndex]); //Hatari
#endif

#if defined(SSE_GLUE_FRAME_TIMINGS)
  Glue.Status.hbi_done=true;
#endif

  m68k_interrupt(LPEEK(0x0068));       
  // set CPU registers
  sr=(sr & (WORD)(~SR_IPL)) | (WORD)(SR_IPL_2);
  debug_check_break_on_irq(BREAK_IRQ_HBL_IDX); 
}
#define HBL_INTERRUPT HBLInterrupt(); // called in mpf.cpp
#endif//inline
#endif//hbl


#if defined(SSE_INT_VBL) 
#if defined(SSE_INT_VBL_INLINE)

// no more multiple defines, all switches in inline function

inline void VBLInterrupt() {

  ASSERT( vbl_pending );
  vbl_pending=false; 
#ifdef SSE_DEBUG
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("VBI");
#endif
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
    FrameEvents.Add(scan_y,LINECYCLES,'I',0x40);
#endif
#if defined(SSE_BOILER_FRAME_INTERRUPTS)
  Debug.FrameInterrupts|=1;
#endif
  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("INTERRUPT: VBL at PC=")+HEXSl(pc,6)+" time is "+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)+" cycles into screen)");
  TRACE_LOG("%d %d %d (%d) VBI Vec %X\n",TIMING_INFO,ACT,LPEEK(0x0070));
#endif//dbg

#if !defined(SSE_CPU_UNSTOP2)
  if (cpu_stopped)
  M68K_UNSTOP;
#endif

#if defined(SSE_INT_VBL_IACK2) && !defined(SSE_INT_VBL_380)
  time_of_last_vbl_interrupt=ABSOLUTE_CPU_TIME;
#endif

  // wobble?
#if defined(SSE_INT_JITTER_VBL_STE)
 // no wobble for STE nor STF, but jitter
#else
#if defined(SSE_INT_JITTER_VBL) && !defined(SSE_INT_E_CLOCK)
  if(ST_TYPE==STE)  // jitter for STF, wobble for STE
#endif
#if defined(SSE_INT_E_CLOCK)
    if(!OPTION_C1) // no jitter no wobble if "E-clock"
#endif
    {//3.6.1: Argh! macro must be scoped! TODO proper switches
      INTERRUPT_START_TIME_WOBBLE; //wobble for STE
    }
#endif

#if defined(SSE_INT_VBL_IACK2) && defined(SSE_INT_VBL_380)
  time_of_last_vbl_interrupt=ABSOLUTE_CPU_TIME;
#endif

#if defined(DEBUG_BUILD) && !defined(SSE_INT_VBL_380) //auto168 boiler wu1
  if(mode==STEM_MODE_CPU) // no cycles when boiler is reading //bug?
#endif
    // E-Clock?
#if defined(SSE_CPU_E_CLOCK)
  if(OPTION_C1)
  { 
#if defined(SSE_CPU_E_CLOCK_370)
    int current_cycles=ACT;
#if defined(SSE_INT_VBL_380)
    INSTRUCTION_TIME(ECLOCK_AUTOVECTOR_CYCLE);
#else
    INSTRUCTION_TIME_ROUND(ECLOCK_AUTOVECTOR_CYCLE);
#endif
    BYTE e_clock_wait_states=
#endif
#ifdef SSE_CPU_E_CLOCK_DISPATCHER
    M68000.SyncEClock(TM68000::ECLOCK_VBL);
#else
    M68000.SyncEClock();
#endif
#if defined(SSE_CPU_E_CLOCK_370)
    INSTRUCTION_TIME(current_cycles-ACT);
    INSTRUCTION_TIME(e_clock_wait_states);
#endif
  }

#endif

#if !defined(SSE_INT_VBL_IACK2) 
#if defined(SSE_INT_VBL_IACK)
  time_of_last_vbl_interrupt=ACT;
#endif
#endif

#if defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
  INSTRUCTION_TIME_ROUND(SSE_INT_VBL_TIMING-12); 
#else
  INSTRUCTION_TIME_ROUND(SSE_INT_VBL_TIMING);
#endif

  m68k_interrupt(LPEEK(0x0070));

  // jitter?
#if defined(SSE_INT_JITTER_VBL)
#if !defined(SSE_INT_JITTER_VBL_STE)
// see Japtro, it would be fixed on STE, but I don't think it should work
// Steem 3.5.1-3.5.4 used this
  if(ST_TYPE!=STE) 
#endif
#if defined(SSE_INT_E_CLOCK)
    if(!OPTION_C1)
#endif
      INSTRUCTION_TIME(VblJitter[VblJitterIndex]);
#endif

  sr=(sr& (WORD)(~SR_IPL))|(WORD)(SR_IPL_4);

  debug_check_break_on_irq(BREAK_IRQ_VBL_IDX);
  ASSERT(!vbl_pending);
}
#define VBL_INTERRUPT VBLInterrupt(); // called in mpf.cpp
#endif//inline
#endif//vbl

#undef LOGSECTION

#endif//defined(IN_EMU)
#endif//#if defined(SSE_INTERRUPT)
#endif//#ifndef SSEINTERRUPT_H