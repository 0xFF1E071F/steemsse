#pragma once
#ifndef SSEINTERRUPT_H
#define SSEINTERRUPT_H

#include "SSESTF.h"

#if defined(SSE_MFP_RATIO) 
extern double CpuMfpRatio;
extern DWORD CpuNormalHz;
#endif

#if defined(SSE_INTERRUPT)

#if defined(SSE_INT_VBL_STF)
extern int HblTiming;
#endif

#if defined(IN_EMU) // argh! using the ugly Steem trick "IN_EMU"

#if !defined(SSE_CPU)
void m68k_interrupt(MEM_ADDRESS ad);
#endif

#if defined(SSE_INT_JITTER) // not defined in current 3.5.3
extern int HblJitter[],VblJitter[];
extern int HblJitterIndex,VblJitterIndex;
#endif

// We must define multiple times for various defines.
// Those macros wouldn't be inlined by VC6.
// Interesting cases: Auto168, BBC52, Dragonnels/Happy Islands, Xpress/Krig

#if defined(SSE_INT_HBL) 

#if defined(SSE_INT_JITTER_HBL) 
/*  This is from Hatari
    It fixes BBC52 if SSE_INT_HBL_IACK_FIX isn't defined (it should
    be)
    It fixes 3615GEN4 HMD Giga (probably along with other mods)
    When we define it, we mustn't define "jitter" for VBI in STE
    mode. This isn't normal but we try to make programs run.
*/
#define HBL_INTERRUPT {\
  hbl_pending=false;\
  log_to_section(LOGSECTION_INTERRUPTS,Str("INTERRUPT: HBL at PC=")+HEXSl(pc,6)+" "+scanline_cycle_log());\
  M68K_UNSTOP;\
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME;\
  INSTRUCTION_TIME_ROUND(SSE_INT_HBL_TIMING);\
  m68k_interrupt(LPEEK(0x0068));       \
  INSTRUCTION_TIME(HblJitter[HblJitterIndex]);\
  sr=(sr & (WORD)(~SR_IPL)) | (WORD)(SR_IPL_2);\
  debug_check_break_on_irq(BREAK_IRQ_HBL_IDX); \
}

#else // wobble as in Steem 3.2

#define HBL_INTERRUPT {\
  hbl_pending=false;\
  log_to_section(LOGSECTION_INTERRUPTS,Str("INTERRUPT: HBL at PC=")+HEXSl(pc,6)+" "+scanline_cycle_log());\
  M68K_UNSTOP;\
  INTERRUPT_START_TIME_WOBBLE; \
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME;\
  INSTRUCTION_TIME_ROUND(SSE_INT_HBL_TIMING);\
  m68k_interrupt(LPEEK(0x0068));       \
  sr=(sr & (WORD)(~SR_IPL)) | (WORD)(SR_IPL_2);\
  debug_check_break_on_irq(BREAK_IRQ_HBL_IDX); \
}

#endif
#endif

#if defined(SSE_INT_VBL) 

#if defined(SSE_INT_VBL_INLINE)

#undef LOGSECTION
#define LOGSECTION LOGSECTION_INTERRUPTS

inline void VBLInterrupt() {
  ASSERT( vbl_pending );
  vbl_pending=false; 

  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("INTERRUPT: VBL at PC=")+HEXSl(pc,6)+" time is "+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)+" cycles into screen)");

  M68K_UNSTOP;
  //TRACE("VBI %d",LINECYCLES);
#if defined(SSE_INT_JITTER_VBL_STE)
 // no wobble for STF, STE
#else
#if defined(SSE_INT_JITTER_VBL)
  if(ST_TYPE==STE)  // jitter for STF
#endif
  {//3.6.1: Argh! macro must be scoped!
    INTERRUPT_START_TIME_WOBBLE; //wobble for STE
  }
#endif
  //TRACE("->%d",LINECYCLES);//surprise!
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_INT_VBL_IACK)
  time_of_last_vbl_interrupt=ACT;
#endif
  INSTRUCTION_TIME_ROUND(SSE_INT_VBL_TIMING);
  //TRACE("->%d",LINECYCLES);
  //TRACE_LOG("F%d Cycles %d VBI %X #%d\n",FRAME,ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl,LPEEK(0x0070),interrupt_depth);
  TRACE_LOG("F%d Cycles %d VBI\n",FRAME,ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl);
  m68k_interrupt(LPEEK(0x0070));
#if defined(SSE_INT_JITTER_VBL)
/*  In the current build, VBL "jitter" instead of "wobble" is necessary
    for demo Japtro, but this could be due to other timing problems. TODO
*/

#if !defined(SSE_INT_JITTER_VBL_STE)
  if(ST_TYPE!=STE) 
#endif
#if defined(SSE_INT_JITTER_VBL2)//3.6.1
    if(LINECYCLES<132 || !SSE_HACKS_ON) // hack for Demoniak 100%
#endif
      INSTRUCTION_TIME(VblJitter[VblJitterIndex]);
  //TRACE("->%d\n",LINECYCLES);

  sr=(sr& (WORD)(~SR_IPL))|(WORD)(SR_IPL_4);

  debug_check_break_on_irq(BREAK_IRQ_VBL_IDX);
  ASSERT(!vbl_pending);
}

#undef LOGSECTION

#define VBL_INTERRUPT VBLInterrupt();

#else//!inline

#if defined(SSE_INT_JITTER_VBL)

#define VBL_INTERRUPT {\
  vbl_pending=false;    \
  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("INTERRUPT: VBL at PC=")+HEXSl(pc,6)+" time is "+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)+" cycles into screen)");\
  M68K_UNSTOP;\
  if(ST_TYPE==STE) INTERRUPT_START_TIME_WOBBLE;\
  INSTRUCTION_TIME_ROUND(SSE_INT_VBL_TIMING);\
  m68k_interrupt(LPEEK(0x0070));\
  if(ST_TYPE!=STE) INSTRUCTION_TIME(VblJitter[VblJitterIndex]);\
  sr=(sr& (WORD)(~SR_IPL))|(WORD)(SR_IPL_4);\
  debug_check_break_on_irq(BREAK_IRQ_VBL_IDX);\
}


#else // wobble as in Steem 3.2

#define VBL_INTERRUPT {\
  vbl_pending=false;    \
  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("INTERRUPT: VBL at PC=")+HEXSl(pc,6)+" time is "+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)+" cycles into screen)");\
  M68K_UNSTOP;\
  INTERRUPT_START_TIME_WOBBLE; \
  INSTRUCTION_TIME_ROUND(SSE_INT_VBL_TIMING);\
  m68k_interrupt(LPEEK(0x0070));\
  sr=(sr& (WORD)(~SR_IPL))|(WORD)(SR_IPL_4);\
  debug_check_break_on_irq(BREAK_IRQ_VBL_IDX);\
}

#endif
#endif//inline

#endif

#endif//defined(IN_EMU)
#endif//#if defined(SSE_INTERRUPT)
#endif//#ifndef SSEINTERRUPT_H