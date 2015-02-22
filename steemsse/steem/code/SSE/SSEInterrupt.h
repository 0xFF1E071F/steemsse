#pragma once
#ifndef SSEINTERRUPT_H
#define SSEINTERRUPT_H

#include "SSESTF.h"

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
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
#if defined(SSE_BOILER_FRAME_REPORT_MASK2)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
#else
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_HBI)
#endif
    FrameEvents.Add(scan_y,LINECYCLES,'I',0x20);
#endif
  TRACE_LOG("%d %d %d (%d) HBI #%d Vec %X\n",TIMING_INFO,ACT,Debug.nHbis,LPEEK(0x0068));
#endif//dbg

#if !defined(SSE_CPU_UNSTOP2)
  if (cpu_stopped)
  M68K_UNSTOP;
#endif

#if defined(SSE_INT_HBL_IACK2) 
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME;
#endif

  // wobble?
#if !defined(SSE_INT_JITTER_HBL)
#if defined(SSE_INT_E_CLOCK)
  if(!HD6301EMU_ON) // not if mode "E-Clock"
#endif
  {
    INTERRUPT_START_TIME_WOBBLE; //Steem 3.2
  }
#endif

  // E-clock?

#if defined(SSE_INT_E_CLOCK)
  if(HD6301EMU_ON)
#if defined(SSE_CPU_E_CLOCK2)
  { 
    int current_cycles=ACT;
    INSTRUCTION_TIME/*_ROUND*/(ECLOCK_AUTOVECTOR_CYCLE);
    BYTE e_clock_wait_states=
#endif
#ifdef SSE_CPU_E_CLOCK_DISPATCHER
    M68000.SyncEClock(TM68000::ECLOCK_HBL);
#else
    M68000.SyncEClock();
#endif
#if defined(SSE_CPU_E_CLOCK2)
    INSTRUCTION_TIME(current_cycles-ACT);
    INSTRUCTION_TIME(e_clock_wait_states);
  }
#endif
#endif


#if !defined(SSE_INT_HBL_IACK2) 
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME;
#endif

  INSTRUCTION_TIME_ROUND(SSE_INT_HBL_TIMING); 

  // jitter?
#if defined(SSE_INT_JITTER_HBL)
#if defined(SSE_INT_E_CLOCK)
  if(!HD6301EMU_ON)
#endif
    INSTRUCTION_TIME(HblJitter[HblJitterIndex]); //Hatari
#endif

  // set CPU registers
  m68k_interrupt(LPEEK(0x0068));       
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
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
#if defined(SSE_BOILER_FRAME_REPORT_MASK2)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
#else
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_VBI)
#endif
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

#if defined(SSE_INT_VBL_IACK2) 
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
    if(!HD6301EMU_ON) // no jitter no wobble if "E-clock"
#endif
    {//3.6.1: Argh! macro must be scoped!
      INTERRUPT_START_TIME_WOBBLE; //wobble for STE
    }
#endif


#if defined(DEBUG_BUILD)
  if(mode==STEM_MODE_CPU) // no cycles when boiler is reading
#endif
    // E-Clock?
#if defined(SSE_CPU_E_CLOCK)
  if(HD6301EMU_ON)
#if defined(SSE_CPU_E_CLOCK2)
  { 
    int current_cycles=ACT;
    INSTRUCTION_TIME_ROUND(ECLOCK_AUTOVECTOR_CYCLE);
    BYTE e_clock_wait_states=
#endif
#ifdef SSE_CPU_E_CLOCK_DISPATCHER
    M68000.SyncEClock(TM68000::ECLOCK_VBL);
#else
    M68000.SyncEClock();
#endif
#if defined(SSE_CPU_E_CLOCK2)
    INSTRUCTION_TIME(current_cycles-ACT);
    INSTRUCTION_TIME(e_clock_wait_states);
  }
#endif
#endif

#if !defined(SSE_INT_VBL_IACK2) 
#if defined(STEVEN_SEAGAL) && defined(SSE_INT_VBL_IACK)
  time_of_last_vbl_interrupt=ACT;
#endif
#endif

  INSTRUCTION_TIME_ROUND(SSE_INT_VBL_TIMING);

  m68k_interrupt(LPEEK(0x0070));

  // jitter?
#if defined(SSE_INT_JITTER_VBL)
#if !defined(SSE_INT_JITTER_VBL_STE)
// see Japtro, it would be fixed on STE, but I don't think it should work
// Steem 3.5.1-3.5.4 used this
  if(ST_TYPE!=STE) 
#endif
#if defined(SSE_INT_E_CLOCK)
    if(!HD6301EMU_ON)
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