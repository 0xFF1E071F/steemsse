#pragma once
#ifndef SSEINTERRUPT_H
#define SSEINTERRUPT_H
#if defined(SS_INTERRUPT)

#if defined(SS_MFP_RATIO) 
extern double CpuMfpRatio;
extern DWORD CpuNormalHz;
#endif

#if defined(SS_INT_VBL_STF)
extern int HblTiming;
#endif

#if defined(IN_EMU) // argh! using the ugly Steem trick "IN_EMU"

#if defined(SS_INT_JITTER)
extern int HblJitter[],VblJitter[];
extern int HblJitterIndex,VblJitterIndex;
#endif

// We must define multiple times for various defines.
// Those macros wouldn't be inlined by VC6.
// Interesting cases: Auto168, BBC52, Dragonnels/Happy Islands, Xpress/Krig

#if defined(SS_INT_HBL) 
#if defined(SS_INT_JITTER_HBL) 

#define HBL_INTERRUPT {\
  hbl_pending=false;\
  log_to_section(LOGSECTION_INTERRUPTS,Str("INTERRUPT: HBL at PC=")+HEXSl(pc,6)+" "+scanline_cycle_log());\
  M68K_UNSTOP;\
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME;\
  INSTRUCTION_TIME_ROUND(SS_INT_HBL_TIMING);\
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
  INSTRUCTION_TIME_ROUND(SS_INT_HBL_TIMING);\
  m68k_interrupt(LPEEK(0x0068));       \
  sr=(sr & (WORD)(~SR_IPL)) | (WORD)(SR_IPL_2);\
  debug_check_break_on_irq(BREAK_IRQ_HBL_IDX); \
}

#endif
#endif

#if defined(SS_INT_VBL) 

#if defined(SS_INT_JITTER_VBL)
// we use the jitter only for STF, on the STE it breaks Krieg

#define VBL_INTERRUPT {\
  vbl_pending=false;    \
  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("INTERRUPT: VBL at PC=")+HEXSl(pc,6)+" time is "+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)+" cycles into screen)");\
  M68K_UNSTOP;\
  if(ST_TYPE==STE) INTERRUPT_START_TIME_WOBBLE;\
  INSTRUCTION_TIME_ROUND(SS_INT_VBL_TIMING);\
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
  INSTRUCTION_TIME_ROUND(SS_INT_VBL_TIMING);\
  m68k_interrupt(LPEEK(0x0070));\
  sr=(sr& (WORD)(~SR_IPL))|(WORD)(SR_IPL_4);\
  debug_check_break_on_irq(BREAK_IRQ_VBL_IDX);\
}

#endif
#endif

#endif//defined(IN_EMU)
#endif//#if defined(SS_INTERRUPT)
#endif//#ifndef SSEINTERRUPT_H