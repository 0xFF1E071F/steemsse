#pragma once
#ifndef SSECPU_H
#define SSECPU_H

#if defined(SSE_CPU)

#include <cpu.decla.h>
#include <d2.decla.h>
#ifdef DEBUG_BUILD
#include <debug_emu.decla.h>
#endif
#include <run.decla.h>
#include <blitter.decla.h>
#include <mfp.decla.h>
#include "SSEDebug.h"
#include "SSEVideo.h"

#ifdef SSE_UNIX//?
extern void exception(int,exception_action,MEM_ADDRESS);
#endif

#define LOGSECTION LOGSECTION_CPU 

#if defined(SSE_CPU_INLINE_READ_BWL)

#define m68k_READ_B(addr) M68000.ReadB(addr);
#define m68k_READ_W(addr) M68000.ReadW(addr);
#define m68k_READ_L(addr) M68000.ReadL(addr);

#else

#define m68k_READ_B(addr)                              \
  m68k_src_b=m68k_peek(addr);                           \

#define m68k_READ_W(addr)                              \
  m68k_src_w=m68k_dpeek(addr);                           \

#define m68k_READ_L(addr)                              \
  m68k_src_l=m68k_lpeek(addr);                           \

#endif

#if defined(SSE_CPU_INLINE_READ_FROM_ADDR)

#define m68k_READ_B_FROM_ADDR M68000.m68kReadBFromAddr();
#define m68k_READ_W_FROM_ADDR M68000.m68kReadWFromAddr();
#define m68k_READ_L_FROM_ADDR M68000.m68kReadLFromAddr();

#else 

#define m68k_READ_B_FROM_ADDR                         \
  abus&=0xffffff;                                   \
  if(abus>=himem){                                  \
    if(abus>=MEM_IO_BASE){            \
      if(SUPERFLAG)m68k_src_b=io_read_b(abus);           \
      else exception(BOMBS_BUS_ERROR,EA_READ,abus);         \
    }else if(abus>=0xfc0000){                             \
      if(tos_high && abus<(0xfc0000+192*1024))m68k_src_b=ROM_PEEK(abus-rom_addr);   \
      else if (abus<0xfe0000 || abus>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,abus);  \
    }else if(abus>=MEM_EXPANSION_CARTRIDGE){           \
      if(cart){                                             \
        m68k_src_b=CART_PEEK(abus-MEM_EXPANSION_CARTRIDGE);  \
      }else{                                                 \
        m68k_src_b=0xff;                                    \
      }                                                     \
    }else if (abus>=rom_addr){                         \
      if(abus<(0xe00000+256*1024))m68k_src_b=ROM_PEEK(abus-rom_addr);                           \
      else if (abus>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,abus);          \
      else m68k_src_b=0xff;                                          \
    }else if (abus>=0xd00000 && abus<0xd80000){ \
      m68k_src_b=0xff;                                          \
    }else if(mmu_confused){                            \
      m68k_src_b=mmu_confused_peek(abus,true);                                         \
    }else if(abus>=FOUR_MEGS){                   \
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          \
    }else{                                                     \
      m68k_src_b=0xff;                                          \
    }                                                             \
  }else if(abus>=MEM_START_OF_USER_AREA){                                              \
    DEBUG_CHECK_READ_B(abus);  \
    m68k_src_b=(BYTE)(PEEK(abus));                  \
  }else if(SUPERFLAG){     \
    DEBUG_CHECK_READ_B(abus);  \
    m68k_src_b=(BYTE)(PEEK(abus));                  \
  }else exception(BOMBS_BUS_ERROR,EA_READ,abus);


#define m68k_READ_W_FROM_ADDR           \
  abus&=0xffffff;                                   \
  if(abus&1){                                      \
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    \
  }else if(abus>=himem){                                  \
    if(abus>=MEM_IO_BASE){            \
      if(SUPERFLAG)m68k_src_w=io_read_w(abus);           \
      else exception(BOMBS_BUS_ERROR,EA_READ,abus);         \
    }else if(abus>=0xfc0000){                             \
      if (tos_high && abus<(0xfc0000+192*1024)) m68k_src_w=ROM_DPEEK(abus-rom_addr);   \
      else if (abus<0xfe0000 || abus>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,abus);  \
    }else if(abus>=MEM_EXPANSION_CARTRIDGE){           \
      if(cart){                                             \
        m68k_src_w=CART_DPEEK(abus-MEM_EXPANSION_CARTRIDGE);  \
      }else{                                                 \
        m68k_src_w=0xffff;                                    \
      }                                                     \
    }else if(abus>=rom_addr){                         \
      if(abus<(0xe00000+256*1024)) m68k_src_w=ROM_DPEEK(abus-rom_addr);                           \
      else if (abus>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,abus);          \
      else m68k_src_w=0xffff;                                          \
    }else if (abus>=0xd00000 && abus<0xd80000){ \
      m68k_src_w=0xffff;                                          \
    }else if(mmu_confused){                            \
      m68k_src_w=mmu_confused_dpeek(abus,true);                                         \
    }else if(abus>=FOUR_MEGS){                   \
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          \
    }else{                                                     \
      m68k_src_w=0xffff;                                          \
    }                                                             \
  }else if(abus>=MEM_START_OF_USER_AREA){                                              \
    DEBUG_CHECK_READ_W(abus);  \
    m68k_src_w=DPEEK(abus);                  \
  }else if(SUPERFLAG){     \
    DEBUG_CHECK_READ_W(abus);  \
    m68k_src_w=DPEEK(abus);                  \
  }else exception(BOMBS_BUS_ERROR,EA_READ,abus);

#define m68k_READ_L_FROM_ADDR                        \
  abus&=0xffffff;                                   \
  if(abus&1){                                      \
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    \
  }else if(abus>=himem){                                  \
    if(abus>=MEM_IO_BASE){           \
      if(SUPERFLAG)m68k_src_l=io_read_l(abus);          \
      else exception(BOMBS_BUS_ERROR,EA_READ,abus);         \
    }else if(abus>=0xfc0000){                             \
      if(tos_high && abus<(0xfc0000+192*1024-2)) m68k_src_l=ROM_LPEEK(abus-rom_addr);   \
      else if (abus<0xfe0000 || abus>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,abus);  \
    }else if(abus>=MEM_EXPANSION_CARTRIDGE){           \
      if(cart){                                             \
        m68k_src_l=CART_LPEEK(abus-MEM_EXPANSION_CARTRIDGE);  \
      }else{                                                 \
        m68k_src_l=0xffffffff;                                    \
      }                                                     \
    }else if(abus>=rom_addr){                         \
      if(abus<(0xe00000+256*1024-2)) m68k_src_l=ROM_LPEEK(abus-rom_addr);   \
      else if (abus>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,abus);          \
      else m68k_src_l=0xffffffff;                                          \
    }else if (abus>=0xd00000 && abus<0xd80000-2){ \
      m68k_src_l=0xffffffff;                                          \
    }else if (mmu_confused){                            \
      m68k_src_l=mmu_confused_lpeek(abus,true);                                         \
    }else if (abus>=FOUR_MEGS){                   \
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          \
    }else{                                                     \
      m68k_src_l=0xffffffff;                                          \
    }                                                             \
  }else if(abus>=MEM_START_OF_USER_AREA){                                              \
    DEBUG_CHECK_READ_L(abus);  \
    m68k_src_l=LPEEK(abus);                  \
  }else if(SUPERFLAG){     \
    DEBUG_CHECK_READ_L(abus);  \
    m68k_src_l=LPEEK(abus);                  \
  }else exception(BOMBS_BUS_ERROR,EA_READ,abus);

#endif


#if defined(SSE_BOILER_MONITOR_TRACE) && defined(DEBUG_BUILD)
#define DEBUG_CHECK_IOACCESS \
  if (ioaccess & IOACCESS_DEBUG_MEM_WRITE_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Write to address $"+HEXSl(debug_mem_write_log_address,6)+ \
                  ", new value is "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
    val=m68k_peek(debug_mem_write_log_address);\
    int val2=debug_mem_write_log_address&1?0:m68k_dpeek(debug_mem_write_log_address);\
    int val3=debug_mem_write_log_address&1?0:m68k_lpeek(debug_mem_write_log_address);\
    TRACE("PC %X %s write %X|%X|%X to %X\n",old_pc,disa_d2(old_pc).Text,  \
      val,val2,val3,debug_mem_write_log_address);\
    ioaccess|=IOACCESS_DEBUG_MEM_WRITE_LOG;\
  }  else if (ioaccess & IOACCESS_DEBUG_MEM_READ_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Read from address $"+HEXSl(debug_mem_write_log_address,6)+ \
      ", = "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
    val=m68k_peek(debug_mem_write_log_address);\
    int val2=debug_mem_write_log_address&1?0:m68k_dpeek(debug_mem_write_log_address);\
    int val3=debug_mem_write_log_address&1?0:m68k_lpeek(debug_mem_write_log_address);\
    TRACE("PC %X %s read %X|%X|%X from %X\n",old_pc,disa_d2(old_pc).Text,\
      val,val2,val3,debug_mem_write_log_address);\
    ioaccess|=IOACCESS_DEBUG_MEM_READ_LOG;\
  } 

#elif defined(DEBUG_BUILD)
#define DEBUG_CHECK_IOACCESS \
  if (ioaccess & IOACCESS_DEBUG_MEM_WRITE_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Write to address $"+HEXSl(debug_mem_write_log_address,6)+ \
                  ", new value is "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
  }
#else
#define DEBUG_CHECK_IOACCESS
#endif


#ifdef SSE_DEBUG

inline void handle_ioaccess() {
  if (ioaccess/*||(OPTION_C2&&MC68901.Irq)*/){                             \
    switch (ioaccess & IOACCESS_NUMBER_MASK){                        \
      case 1: io_write_b(ioad,LOBYTE(iobuffer)); break;    \
      case 2: io_write_w(ioad,LOWORD(iobuffer)); break;    \
      case 4: io_write_l(ioad,iobuffer); break;      \
      case TRACE_BIT_JUST_SET: break;                                        \
    }                                             \
    if (ioaccess & IOACCESS_FLAG_DELAY_MFP){ \
      ioaccess&=~IOACCESS_FLAG_DELAY_MFP;  \
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; \
    }else if (ioaccess & IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE){ \
      ioaccess|=IOACCESS_FLAG_DELAY_MFP;  \
    } \
    if (ioaccess & IOACCESS_INTERCEPT_OS2){ \
      ioaccess&=~IOACCESS_INTERCEPT_OS2;  \
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; \
    }else if (ioaccess & IOACCESS_INTERCEPT_OS){ \
      ioaccess|=IOACCESS_INTERCEPT_OS2; \
    } \
    if (/*(OPTION_C2&&MC68901.Irq)||*/(ioaccess & IOACCESS_FLAG_FOR_CHECK_INTRS)){   \
      check_for_interrupts_pending();          \
      CHECK_STOP_USER_MODE_NO_INTR \
    }                                             \
    DEBUG_CHECK_IOACCESS; \
    if (ioaccess & IOACCESS_FLAG_DO_BLIT) 
      Blitter_Start_Now(); \
    /* These flags stay until the next instruction to stop interrupts */  \
    ioaccess=ioaccess & (IOACCESS_FLAG_DELAY_MFP | IOACCESS_INTERCEPT_OS2);                                   \
  }
}

#define HANDLE_IOACCESS(tracefunc) handle_ioaccess();

#else

#define HANDLE_IOACCESS(tracefunc) \
  if (ioaccess){                             \
    switch (ioaccess & IOACCESS_NUMBER_MASK){                        \
      case 1: io_write_b(ioad,LOBYTE(iobuffer)); break;    \
      case 2: io_write_w(ioad,LOWORD(iobuffer)); break;    \
      case 4: io_write_l(ioad,iobuffer); break;      \
      case TRACE_BIT_JUST_SET: tracefunc; break;                                        \
    }                                             \
    if (ioaccess & IOACCESS_FLAG_DELAY_MFP){ \
      ioaccess&=~IOACCESS_FLAG_DELAY_MFP;  \
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; \
    }else if (ioaccess & IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE){ \
      ioaccess|=IOACCESS_FLAG_DELAY_MFP;  \
    } \
    if (ioaccess & IOACCESS_INTERCEPT_OS2){ \
      ioaccess&=~IOACCESS_INTERCEPT_OS2;  \
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; \
    }else if (ioaccess & IOACCESS_INTERCEPT_OS){ \
      ioaccess|=IOACCESS_INTERCEPT_OS2; \
    } \
    if (ioaccess & IOACCESS_FLAG_FOR_CHECK_INTRS){   \
      check_for_interrupts_pending();          \
      CHECK_STOP_USER_MODE_NO_INTR \
    }                                             \
    DEBUG_CHECK_IOACCESS; \
    if (ioaccess & IOACCESS_FLAG_DO_BLIT) Blitter_Start_Now(); \
    /* These flags stay until the next instruction to stop interrupts */  \
    ioaccess=ioaccess & (IOACCESS_FLAG_DELAY_MFP | IOACCESS_INTERCEPT_OS2);                                   \
  }


#endif



// Finer names!
#define IRC   prefetch_buf[1] // Instruction Register Capture
#define IR    prefetch_buf[0] // Instruction Register
#define IRD   ir              // Instruction Register Decoder

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TM68000 {

  // ENUM
#if defined(SSE_CPU_HALT) || defined(SSE_CPU_TRACE_REFACTOR)
/* v 3.7, see MC68000UM p99 (6-1)
"The processor is always in one of three processing states: normal, exception, or halted.
The normal processing state is associated with instruction execution; the memory
references are to fetch instructions and operands and to store results. A special case of
the normal state is the stopped state, resulting from execution of a STOP instruction. In
this state, no further memory references are made."
->  For the moment only HALTED is used, for the GUI.
    We add TRACE_MODE to spare a byte, it should correspond to a hidden
    bit in the CPU, so that it knows that the trace bit was set at the
    start of the instruction.
v3.8.0
Different states are used, but some have NOTHING to do with processor state,
so refactoring due!
*/
  enum {NORMAL,EXCEPTION,HALTED,STOPPED,TRACE_MODE,INTEL_CRASH,BLIT_ERROR,BOILER_MESSAGE}; 
#endif
#ifdef SSE_CPU_E_CLOCK
  enum {ECLOCK_VBL,ECLOCK_HBL,ECLOCK_ACIA}; //debug/hacks
#endif

  // DATA
#if defined(SSE_CPU_PREFETCH_CALL)
  WORD *PrefetchAddress; 
#endif
#if defined(SSE_CPU_E_CLOCK)
  int cycles_for_eclock; // because of integer overflow problems
  int cycles0; // to record ACT
#endif
#if defined(SSE_CPU_EXCEPTION) && defined(SSE_CPU_TRUE_PC)
  MEM_ADDRESS Pc;
#endif
#if defined(SSE_DEBUG)
  int IrAddress; // pc at start
  int nExceptions;
  int nInstr;
#endif
#if defined(SSE_CPU_PREFETCH_CALL)
  WORD PrefetchedOpcode;
#endif
#if defined(SSE_CPU_DATABUS)
  WORD dbus; // TODO; we have global abus, which isn't always up-to-date
#endif
#if defined(SSE_DEBUG)
  WORD PreviousIr;
#endif
#if defined(SSE_CPU_PREFETCH_CLASS)
  BYTE PrefetchClass; // see ijor's article
#endif
#if defined(SSE_CPU_EXCEPTION) && defined(SSE_CPU_TRUE_PC)
  bool CheckRead; // most 'write' instructions read before, crash at read...
#endif
#if defined(SSE_CPU_ROUNDING_BUS2)
  bool Unrounded; // for blitter
#endif
#if defined(SSE_CPU_ROUNDING_BUS)
#if defined(SSE_VS2008_WARNING_383)
  char Rounded;
#else
  bool Rounded;
#endif
#endif
#if defined(SSE_CPU_PREFETCH_CALL)
  bool CallPrefetch; 
#endif
#if defined(SSE_CPU_E_CLOCK)
  bool EClock_synced;
  BYTE LastEClockCycles[3];
#endif
#if defined(SSE_CPU_HALT) || defined(SSE_CPU_TRACE_REFACTOR)
  BYTE ProcessingState;
#endif
#if defined(SSE_INT_MFP_SPURIOUS) && !defined(SSE_VAR_OPT_383D)
  bool IackCycle; // flag to avoid starting IACK during IACK (in an emulator, anything goes...)
#endif
#if defined(SSE_CPU_TPEND)  
  bool tpend; // actual internal latch set when CPU should trace current instruction
#endif  
#if defined(SSE_DEBUG)
  bool NextIrFetched; // fetched next instruction?
#endif

  // FUNCTIONS
  TM68000();
#if defined(SSE_CPU_PREFETCH_TIMING) && !defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
#else//for v341-
  inline void FetchTiming();
  inline void FetchTimingNoRound();
#endif
  inline void FetchWord(WORD &dest_word); 
  inline void InstructionTime(int n);
  void InstructionTimeRound(int n);
  void Interrupt(MEM_ADDRESS ad);
  void PerformRte();
  void PrefetchSetPC();
  inline void Process();
#if defined(SSE_CPU_INLINE_READ_BWL)
  static inline void ReadB(MEM_ADDRESS addr);
  static inline void ReadW(MEM_ADDRESS addr);
  static inline void ReadL(MEM_ADDRESS addr);
#endif
#if defined(SSE_CPU_INLINE_READ_FROM_ADDR)
  static void m68kReadBFromAddr();
  static void m68kReadWFromAddr();
  static void m68kReadLFromAddr();
#endif
#if defined(SSE_CPU_INLINE_SET_DEST_TO_ADDR)
  inline void SetDestBToAddr();
  inline void SetDestWToAddr();
  inline void SetDestLToAddr();
#endif
  void SetPC(MEM_ADDRESS ad);
  inline void Unstop();
  void Reset(bool Cold);
  inline void GetDestByte();
  inline void GetDestWord();
  inline void GetDestLong();
  inline void GetDestByteNotA();
  inline void GetDestWordNotA();
  inline void GetDestLongNotA();
  inline void GetDestByteNotAOrD();
  inline void GetDestWordNotAOrD();
  inline void GetDestLongNotAOrD();
  inline void GetDestByteNotAFasterForD();
  inline void GetDestWordNotAFasterForD();
  inline void GetDestLongNotAFasterForD();
  inline void GetSourceByte(); 
  inline void GetSourceWord();
  inline void GetSourceLong();
  inline void GetSourceByteNotA(); 
  inline void GetSourceWordNotA();
  inline void GetSourceLongNotA();
#if defined(SSE_CPU_PREFETCH)
#if defined(SSE_CPU_PREFETCH_CALL)
  WORD FetchForCall(MEM_ADDRESS ad);
#endif
  void PrefetchIrc();
#if !defined(SSE_CPU_ROUNDING2)
  inline void PrefetchIrcNoRound();
#endif
  inline void RefetchIr();
#endif
#if defined(SSE_CPU_E_CLOCK)
  int SyncEClock(int dispatcher);
  void UpdateCyclesForEClock();
#endif
};

#pragma pack(pop)


#define SET_PC(ad) M68000.SetPC(ad);


#if defined(SSE_CPU_TRUE_PC)
#define TRUE_PC M68000.Pc
#define CHECK_READ M68000.CheckRead
#endif


#if defined(SSE_CPU_PREFETCH_TIMING) && !defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)

#ifdef SSE_COMPILER_382
#define FETCH_TIMING {}
#else
#define FETCH_TIMING 
#endif

#else

inline void TM68000::FetchTiming() {
#if !defined(SSE_CPU_PREFETCH_TIMING)

#if defined(SSE_DEBUG)
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
  }
  NextIrFetched=true;
#endif  
  InstructionTimeRound(4); 
#endif
  
#if defined(SSE_CPU_PREFETCH_TIMING) && defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(ir))
    InstructionTimeRound(4); 
#endif

}
#define FETCH_TIMING M68000.FetchTiming()

#endif


#if defined(SSE_CPU_FETCH_TIMING)

#if defined(SSE_CPU_PREFETCH_TIMING) && !defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)

#define FETCH_TIMING_NO_ROUND

#else

inline void TM68000::FetchTimingNoRound() {
#if !defined(SSE_CPU_PREFETCH_TIMING)

#if defined(SSE_DEBUG)
  ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
  }
  NextIrFetched=true;
#endif  
  InstructionTime(4); 
#endif
  
#if defined(SSE_CPU_PREFETCH_TIMING) && defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(ir))
    InstructionTime(4); 
#endif

}
#define FETCH_TIMING_NO_ROUND M68000.FetchTimingNoRound()

#endif

#endif

inline void TM68000::FetchWord(WORD &dest_word) {

// TODO ultimately the timings should be set in those fetching functions

  dest_word=IR;
  if(prefetched_2)// already prefetched
  {
    if(IRC!=*lpfetch)
    {
/*  The "prefetch trick" was used by some programs to crash when the CPU
    was in 'trace' mode 
    eg Anomaly, Transbeauce 1, Transbeauce 2
    Operation Clean Streets (Auto 168)
*/
#if defined(SSE_CPU_PREFETCH_TRACE)
      TRACE_LOG("Prefetched IRC:%X current:%X\n",IRC,*lpfetch);
//      TRACE_OSD("PREFETCH"); // ST-CNX
#endif
#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK_CPU & OSD_CONTROL_CPUPREFETCH) 
    TRACE_OSD("PREFETCH");
#endif

#if defined(SSE_BETA) && defined(SSE_CPU_NO_PREFETCH)
      IRC=*lpfetch; // enabling this cancels prefetch (don't)
#endif
    }
    IR=IRC;
    prefetched_2=false;
  }
  else
#if defined(SSE_CPU_FETCH_IO2)
  // don't use lpfetch for fetching in IO zone, use io_read
  // (palette was already handled by Steem)
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    IR=io_read_w(pc);
    TRACE_LOG("Fetch word %X in IO zone %X\n",IR,pc);
    return;
  }
  else
#endif
#if defined(SSE_CPU_FETCH_80000B)
  if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
  {
    IR=0xFFFF; // TODO
    TRACE_LOG("Fetch IR %X in empty zone %X\n",IR,pc);
    return;
  }
  else
#endif
#if defined(SSE_CPU_ROUNDING_BUS)
  {
    if(Rounded && pc>=rom_addr && pc<rom_addr+tos_len)
      InstructionTime(-2);
    IR=*lpfetch; // next instr or imm
  }
#else
    IR=*lpfetch; // next instr or imm
#endif
  //ASSERT( !(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260)) ); // Warp, Union

#if defined(SSE_CPU_FETCH_IO)
#if defined(SSE_CPU_FETCH_IO3)
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
#else
  if(pc>=MEM_IO_BASE) // normally it took IRC
#endif
    return; //if we test that, we may as well avoid useless inc "PC"
#endif
  lpfetch+=MEM_DIR; // advance the fetch pointer
  if(lpfetch MEM_GE lpfetch_bound) // MEM_GE : <=
    ::exception(BOMBS_BUS_ERROR,EA_FETCH,pc); // :: for gcc "ambiguous" ?
}

#define FETCH_W(dest_word) M68000.FetchWord(dest_word);


#define m68k_GET_DEST_B M68000.GetDestByte();
#define m68k_GET_DEST_W M68000.GetDestWord();
#define m68k_GET_DEST_L M68000.GetDestLong();
#define m68k_GET_DEST_B_NOT_A M68000.GetDestByteNotA();
#define m68k_GET_DEST_W_NOT_A M68000.GetDestWordNotA();
#define m68k_GET_DEST_L_NOT_A M68000.GetDestLongNotA();
#define m68k_GET_DEST_B_NOT_A_OR_D M68000.GetDestByteNotAOrD();
#define m68k_GET_DEST_W_NOT_A_OR_D M68000.GetDestWordNotAOrD();
#define m68k_GET_DEST_L_NOT_A_OR_D M68000.GetDestLongNotAOrD();
#define m68k_GET_DEST_B_NOT_A_FASTER_FOR_D M68000.GetDestByteNotAFasterForD();
#define m68k_GET_DEST_W_NOT_A_FASTER_FOR_D M68000.GetDestWordNotAFasterForD();
#define m68k_GET_DEST_L_NOT_A_FASTER_FOR_D M68000.GetDestLongNotAFasterForD();
#define m68k_GET_SOURCE_B M68000.GetSourceByte();
#define m68k_GET_SOURCE_W M68000.GetSourceWord();
#define m68k_GET_SOURCE_L M68000.GetSourceLong();
#define m68k_GET_SOURCE_B_NOT_A M68000.GetSourceByteNotA();
#define m68k_GET_SOURCE_W_NOT_A M68000.GetSourceWordNotA();
#define m68k_GET_SOURCE_L_NOT_A M68000.GetSourceLongNotA();

inline void TM68000::GetDestByte() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWord() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLong() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestByteNotA() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWordNotA() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLongNotA() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestByteNotAOrD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWordNotAOrD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLongNotAOrD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a_or_d[(ir&BITS_543)>>3]();
}


#if !defined(SSE_CPU_ROUNDING_NO_FASTER_FOR_D)
inline void TM68000::GetDestByteNotAFasterForD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a_faster_for_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWordNotAFasterForD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a_faster_for_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLongNotAFasterForD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a_faster_for_d[(ir&BITS_543)>>3]();
}
#endif

inline void TM68000::GetSourceByte() {
  m68k_jump_get_source_b[(ir&BITS_543)>>3]();
}


inline void TM68000::GetSourceWord() {
  m68k_jump_get_source_w[(ir&BITS_543)>>3]();
}


inline void TM68000::GetSourceLong() {
  m68k_jump_get_source_l[(ir&BITS_543)>>3]();
}


inline void TM68000::GetSourceByteNotA() {
  m68k_jump_get_source_b_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetSourceWordNotA() {
  m68k_jump_get_source_w_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetSourceLongNotA() {
  m68k_jump_get_source_l_not_a[(ir&BITS_543)>>3]();
}


#if defined(SSE_CPU_INLINE_SET_DEST_TO_ADDR)
// but VC9 won't inline it -> exe size  reduction
inline void TM68000::SetDestBToAddr() {
  abus&=0xffffff;                                   
  if(abus>=MEM_IO_BASE){               
    if(SUPERFLAG){                        
      ioaccess&=IOACCESS_FLAGS_MASK; 
      ioaccess|=1;                     
      ioad=abus;                        
      m68k_dest=&iobuffer;               
      DWORD_B_0(&iobuffer)=io_read_b(abus);        
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);             
  }else if(abus>=himem){                               
#if !defined(SSE_MMU_NO_CONFUSION)
    if(mmu_confused){                               
      mmu_confused_set_dest_to_addr(1,true);           
    }else 
#endif
    if(abus>=FOUR_MEGS){                                                
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               
    }else{                                                        
      m68k_dest=&iobuffer;                             
    }                                       
  }else{                                            
#if !defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_B(abus); 
#endif
    if (SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){                             
      m68k_dest=lpPEEK(abus);           
    }else if(abus>=MEM_START_OF_USER_AREA){ 
      m68k_dest=lpPEEK(abus);           
    }else{                                      
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       
    }                                           
#if defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_B(abus); 
#endif
  }
}

inline void TM68000::SetDestWToAddr() {
  abus&=0xffffff;                                   
  if(abus&1){                                      
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);    
  }else if(abus>=MEM_IO_BASE){               
    if(SUPERFLAG){                        
      ioaccess&=IOACCESS_FLAGS_MASK; 
      ioaccess|=2;                     
      ioad=abus;                        
      m68k_dest=&iobuffer;               
      *((WORD*)&iobuffer)=io_read_w(abus);        
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                                
  }else if(abus>=himem){  
#if !defined(SSE_MMU_NO_CONFUSION)
    if(mmu_confused){                               
      mmu_confused_set_dest_to_addr(2,true);           
    }else 
#endif
    if(abus>=FOUR_MEGS){                                                
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               
    }else{                                                        
      m68k_dest=&iobuffer;                             
    }                                       
  }else{                   
#if !defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_W(abus);  
#endif
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){                       
      m68k_dest=lpDPEEK(abus);           
    }else if(abus>=MEM_START_OF_USER_AREA){ 
      m68k_dest=lpDPEEK(abus);           
    }else{                                      
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       
    }                                           
#if defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_W(abus);  
#endif
  }

}

inline void TM68000::SetDestLToAddr() {
  abus&=0xffffff;                                   
  if(abus&1){                                      
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);    
  }else if(abus>=MEM_IO_BASE){               
    if(SUPERFLAG){                        
      ioaccess&=IOACCESS_FLAGS_MASK; 
      ioaccess|=4;                     
      ioad=abus;                         
      m68k_dest=&iobuffer;               
      iobuffer=io_read_l(abus);        
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                                 
  }else if(abus>=himem){  
#if !defined(SSE_MMU_NO_CONFUSION)
    if(mmu_confused){                               
      mmu_confused_set_dest_to_addr(4,true);           
    }else 
#endif
    if(abus>=FOUR_MEGS){                                                
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               
    }else{                                                        
      m68k_dest=&iobuffer;                             
    }                                       
  }else{                               
#if !defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_L(abus);  
#endif
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){                       
      m68k_dest=lpLPEEK(abus);           
    }else if(abus>=MEM_START_OF_USER_AREA){ 
      m68k_dest=lpLPEEK(abus);           
    }else{                                      
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       
    }                                           
#if defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_L(abus);  
#endif
  }
}

#endif


inline void TM68000::InstructionTime(int t) {
  cpu_cycles-=(t);
}

#define INSTRUCTION_TIME(t)  M68000.InstructionTime(t)

#define INSTRUCTION_TIME_ROUND(t) M68000.InstructionTimeRound(t)

#define CPU_ABUS_ACCESS_READ  M68000.InstructionTimeRound(4)

#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
#define CPU_ABUS_ACCESS_READ_FETCH
#else
#define CPU_ABUS_ACCESS_READ_FETCH CPU_ABUS_ACCESS_READ
#endif

#if defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
#define CPU_ABUS_ACCESS_WRITE_PUSH 
#else
#define CPU_ABUS_ACCESS_WRITE_PUSH CPU_ABUS_ACCESS_WRITE
#endif

#if defined(SSE_CPU_ROUNDING_BUS) // TODO: cartridge
#define CPU_ABUS_ACCESS_READ_PC \
    if(pc>=rom_addr && pc<rom_addr+tos_len)\
      INSTRUCTION_TIME(4);\
    else\
      INSTRUCTION_TIME_ROUND(4);
#else
#define CPU_ABUS_ACCESS_READ_PC CPU_ABUS_ACCESS_READ //so we can directly replace macros
#endif

#define CPU_ABUS_ACCESS_WRITE  M68000.InstructionTimeRound(4)

#define m68k_interrupt(ad) M68000.Interrupt(ad)

#define M68K_PERFORM_RTE(checkints) M68000.PerformRte()

#if defined(SSE_CPU_PREFETCH)

#define EXTRA_PREFETCH //all prefetches actually are "extra"!
#define PREFETCH_IRC  M68000.PrefetchIrc()

#else //not defined, macros do nothing

#define EXTRA_PREFETCH
#define PREFETCH_IRC

#endif

// core function, dispatches in cpu emu, inlining m68k_PROCESS

inline void TM68000::Process() {

#if defined(SSE_BOILER)
  LOG_CPU  
/*  Very powerful but demanding traces.
    In SSE boiler, it won't stop if 'suspend log' is checked.
    You may have, beside the disassembly, cycles (absolute, frame, line),
    registers, and values pointed to by address registers as well
    as the stack.
*/
  if(TRACE_ENABLED(LOGSECTION_CPU))
  {
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK4 & TRACE_CONTROL_CPU_CYCLES)
    {
      TRACE_LOG("\nCycles %d %d %d (%d)\n",ACT,FRAMECYCLES,LINECYCLES,scan_y);
    }
    if(TRACE_MASK4 & TRACE_CONTROL_CPU_REGISTERS)
    {
      if(!(TRACE_MASK4 & TRACE_CONTROL_CPU_CYCLES))
        TRACE_LOG("\n");
      for(int i=0;i<8;i++) // D0-D7
        TRACE_LOG("D%d=%X ",i,r[i]);
      TRACE_LOG("\n");
      for(int i=0;i<8;i++) // A0-A7 
      {
        TRACE_LOG("A%d=%X ",i,areg[i]);
        if(TRACE_MASK4 & TRACE_CONTROL_CPU_REGISTERS_VAL) //v3.7.1
          TRACE_LOG("(%X) ",(areg[i]&1)?d2_peek(areg[i]):d2_dpeek(areg[i]));
      }
      TRACE_LOG("SR=%X\n",sr);
    }
    if(TRACE_MASK4 & TRACE_CONTROL_CPU_SP)
    {
      for(int i=0;i<8;i++) // D0-D7
        TRACE_LOG("%X=%X ",areg[7]+i*2,DPEEK(areg[7]+i*2));
      TRACE_LOG("\n");      
    }
#endif
    if(sr&SR_TRACE)
      TRACE_LOG("(T) %X %X %s\n",pc,ir,disa_d2(pc).Text);
    else
      TRACE_LOG("%X %X %s\n",pc,ir,disa_d2(pc).Text);
  }
#endif//boiler

#if defined(SSE_DEBUG)
  IrAddress=pc;
  PreviousIr=IRD;
  nInstr++;
#endif//debug

#if defined(SSE_CPU_CHECK_PC) 
  ASSERT( (PC&0xffffff)==pc+2 );   // strong
#ifdef DEBUG_BUILD
  if((PC&0xffffff)!=pc+2)
  {
    EasyStr instr=disa_d2(old_pc);
    TRACE_LOG("PC %X IR %X %s PC %X TST %X\n",old_pc,PreviousIr,instr.c_str(),pc+2,(PC&0xffffff));
    PC=pc+2;//no error accumulation!
  }
#endif
#endif

#if defined(SSE_CPU_TRACE_REFACTOR)
#if defined(SSE_CPU_TPEND2)  
  if(tpend)
#else  
  if(ProcessingState==TRACE_MODE)
#endif    
  {
#if defined(SSE_CPU_TPEND)
    tpend=false;
#endif    
    ProcessingState=NORMAL;
  }
#if defined(SSE_VC_INTRINSICS_382)
  else if BITTEST(sr,0xf)
#else
  else if(sr&SR_TRACE)
#endif
  {
#if defined(SSE_CPU_TPEND)
    tpend=true; // hardware latch (=flag)
#endif
    ProcessingState=TRACE_MODE; //internal flag: trace after this instruction
#if defined(SSE_CPU_TRACE_DETECT) && !defined(DEBUG_BUILD)
    TRACE_OSD("TRACE");
#endif
#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK_CPU & OSD_CONTROL_CPUTRACE) 
      TRACE_OSD("TRACE");
#endif
    
  }
#endif

  old_pc=pc;  

#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK_CPU & OSD_CONTROL_CPUIO) 
    if(pc>=MEM_IO_BASE)
      TRACE_OSD("PC %X",pc);
#endif

#if defined(SSE_CPU_PREFETCH)
/*  basic prefetch rule:
Just before the start of any instruction two words (no more and no less) are 
already fetched. One word will be in IRD and another one in IRC.
*/
#if defined(SSE_CPU_PREFETCH_ASSERT)
/*  This is a strong assert that verifies that prefetch has been totally
    redesigned in Steem SSE. Of course, our mods must be defined.
*/
#if defined(SSE_CPU_MOVE) && defined(SSE_CPU_DIV)
  ASSERT(prefetched_2); 
#endif
#endif
#if defined(SSE_CPU_PREFETCH_CLASS)
  PrefetchClass=0; // default, most instructions
#endif
#endif

  FetchWord(IRD); // IR->IRD (using reference)
  // TODO should already be there?
  // TODO assert illegal here, but perf?

#if defined(SSE_CPU_EXCEPTION) && defined(SSE_CPU_EXCEPTION_TRACE_PC)//heavy!!!
  if(nExceptions>1) 
    TRACE_LOG("%X\n",pc);
#endif

  pc+=2; // in fact it was already set in previous instruction

#if defined(SSE_CPU_TRUE_PC)
  Pc=pc; // anyway
  CheckRead=0;
#endif

#if defined(SSE_CPU_E_CLOCK)
  EClock_synced=false; // one more bool in Process()!
#endif

  /////////// JUMP TO CPU EMU: ///////////////
  m68k_high_nibble_jump_table[ir>>12](); // go to instruction...

#if defined(SSE_CPU_TRACE_REFACTOR)
#undef LOGSECTION
#define LOGSECTION LOGSECTION_TRACE
/*  Why refactor something that works?
    Because the former way causes a glitch in the Boiler, where
    the instruction being traced always seems to be skipped (can't 
    step, can't set breakpoint). Now this works.
    Because it seems more natural, formalising a flag that must
    be in the CPU.
    Update: it is one-bit latch 'tpend' TODO
    Issue: more tests in Process


Tracing
To aid in program development, the M68000 Family includes a facility to allow tracing
following each instruction. When tracing is enabled, an exception is forced after each
instruction is executed. Thus, a debugging program can monitor the execution of the
program under test.
The trace facility is controlled by the T bit in the supervisor portion of the status register. If
the T bit is cleared (off), tracing is disabled and instruction execution proceeds from
instruction to instruction as normal. If the T bit is set (on) at the beginning of the execution
of an instruction, a trace exception is generated after the instruction is completed. If the
instruction is not executed because an interrupt is taken or because the instruction is
illegal or privileged, the trace exception does not occur. The trace exception also does not
occur if the instruction is aborted by a reset, bus error, or address error exception. If the
instruction is executed and an interrupt is pending on completion, the trace exception is
processed before the interrupt exception. During the execution of the instruction, if an
exception is forced by that instruction, the exception processing for the instruction
exception occurs before that of the trace exception.
As an extreme illustration of these rules, consider the arrival of an interrupt during the
execution of a TRAP instruction while tracing is enabled. First, the trap exception is
processed, then the trace exception, and finally the interrupt exception. Instruction
execution resumes in the interrupt handler routine.
After the execution of the instruction is complete and before the start of the next
instruction, exception processing for a trace begins. A copy is made of the status register.
The transition to supervisor mode is made, and the T bit of the status register is turned off,
disabling further tracing. The vector number is generated to reference the trace exception
vector, and the current program counter and the copy of the status register are saved on
the supervisor stack. On the MC68010, the format/offset word is also saved on the
supervisor stack. The saved value of the program counter is the address of the next
instruction. Instruction execution commences at the address contained in the trace
exception vector.

-> no trace after bus/address/illegal error

*/
#if defined(SSE_CPU_TPEND_382)  //argh, forgot this!
  if(tpend)
#else
  if(ProcessingState==TRACE_MODE)
#endif
  {
#ifdef DEBUG_BUILD
#if defined(SSE_CPU_TPEND) && !defined(SSE_CPU_TPEND2)
    ASSERT(tpend);
#endif
    if(!Debug.logsection_enabled[LOGSECTION_CPU] && !logsection_enabled[LOGSECTION_CPU])
      TRACE_LOG("(T) PC %X SR %X VEC %X IR %04X: %s\n",old_pc,sr,LPEEK(0x24),ir,disa_d2(old_pc).Text);
#else
    TRACE_LOG("TRACE PC %X IR %X SR %X $24 %X\n",pc,ir,sr,LPEEK(0x24));
#endif
#if defined(SSE_CPU_TRACE_TIMING)
/*
Trace timing: 32 34 or 36???
Why "round first for interrupts" ?

Motorola
Trace	34(4/3)

Yacht
Trace               | 34(4/3)  |              nn    ns nS ns nV nv np np   
  .For all these exceptions, there is a difference of 2 cycles between Data 
   bus usage as obtained from USP4325121 and periods as written in M68000UM.
   There's no proven theory to explain this gap.
   SS: addition of timings is 32

WinUAE
- 4 idle cycles
- write PC low word
- write SR
- write PC high word                       [wrong order...is this credible?]
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

On ST, if there are 2 idle cycles between 2 prefetches from RAM,
we must add 2 wait states. -> round(36) instead of 34, and we explain
the timing for LEGACY.
Assume the same for Illegal, Privilege, Line A and Line F 
+TRAPV (SSE_CPU_TRACE_TIMING_EXT)

*/
#if defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
    //INSTRUCTION_TIME_ROUND(CPU_TRACE_TIMING-12); 
    InstructionTimeRound(CPU_TRACE_TIMING-12);
#else
    INSTRUCTION_TIME_ROUND(CPU_TRACE_TIMING); //36, including prefetch
#endif
#else
    INSTRUCTION_TIME_ROUND(0); // Round first for interrupts
    INSTRUCTION_TIME_ROUND(34); // note: tested timing (Legacy.msa)
#endif
    
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("TRACE");
#endif
    //m68k_interrupt(LPEEK(BOMBS_TRACE_EXCEPTION*4));
    Interrupt(LPEEK(BOMBS_TRACE_EXCEPTION*4));
  }
#undef LOGSECTION
#define LOGSECTION LOGSECTION_CPU 
#endif//SSE_CPU_TRACE_REFACTOR

#if defined(SSE_DEBUG)
  NextIrFetched=false;
#endif//debug

#if defined(SSE_CPU_TRACE_REFACTOR)
   // this won't care for trace
   HANDLE_IOACCESS( ; );
#else
  // this will execute the next instruction then trigger trace interrupt
  HANDLE_IOACCESS( m68k_trace(); ); // keep as macro, wouldn't be inlined
#endif
  DEBUG_ONLY( debug_first_instruction=0 );
}
#define m68k_PROCESS M68000.Process();


#if defined(SSE_CPU_PREFETCH)
#ifdef SSE_DEBUG
#pragma warning(disable : 4710) //inlining
#endif
inline void TM68000::RefetchIr() {
//this is no fix, it was already in Steem 3.2
  ASSERT( IR==*(lpfetch+1) ); //detect cases
  //ASSERT( MEM_DIR==-1 );
  IR=*(lpfetch-MEM_DIR); //needed for Synth Dream Sound Demo II!
}
#define REFETCH_IR  M68000.RefetchIr();
#else
#define REFETCH_IR
#ifdef SSE_DEBUG
#pragma warning(default : 4710)
#endif
#endif


#if defined(SSE_CPU_INLINE_READ_BWL)

void TM68000::ReadB(MEM_ADDRESS addr) {
  m68k_src_b=m68k_peek(addr);
}

void TM68000::ReadW(MEM_ADDRESS addr) {
  m68k_src_w=m68k_dpeek(addr);
}

void TM68000::ReadL(MEM_ADDRESS addr) {
  m68k_src_l=m68k_lpeek(addr);
}

#endif


inline void TM68000::Unstop() {
  if(cpu_stopped)
  {
    cpu_stopped=false;     
    SetPC((pc+4) | pc_high_byte); 
  }
}
#define M68K_UNSTOP M68000.Unstop()


////////////////////////
// Poke, inlined part //
////////////////////////


#if defined(SSE_CPU_POKE)

void m68k_poke_abus(BYTE x);
void m68k_dpoke_abus(WORD x);
void m68k_lpoke_abus(LONG x);


// those will be inlined where it counts (cpu.cpp), not where it
// doesn't count (stemdos.cpp)

void m68k_poke(MEM_ADDRESS ad,BYTE x){
  abus=ad&0xffffff;
  BOOL super=SUPERFLAG;
  if(abus>=MEM_IO_BASE && super)
    io_write_b(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
#if defined(SSE_BOILER_MONITOR_VALUE2)
    PEEK(abus)=x;
    DEBUG_CHECK_WRITE_B(abus);
#else
    DEBUG_CHECK_WRITE_B(abus);
    PEEK(abus)=x;
#endif
  }
  else 
    m68k_poke_abus(x);
}


void m68k_dpoke(MEM_ADDRESS ad,WORD x){
  abus=ad&0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1) 
    ::exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE && super)
      io_write_w(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DPEEK(abus)=x;
    DEBUG_CHECK_WRITE_W(abus);
#else
    DEBUG_CHECK_WRITE_W(abus);
    DPEEK(abus)=x;
#endif
  }
  else
    m68k_dpoke_abus(x);
}


void m68k_lpoke(MEM_ADDRESS ad,LONG x){
  abus=ad&0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1)
    ::exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else 
  if(abus>=MEM_IO_BASE && super)
    io_write_l(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
#if defined(SSE_BOILER_MONITOR_VALUE2)//3.6.1, also for long of course (argh!)
    LPEEK(abus)=x;
    DEBUG_CHECK_WRITE_L(abus);
#else
    DEBUG_CHECK_WRITE_L(abus);
    LPEEK(abus)=x;
#endif
  }
  else
    m68k_lpoke_abus(x);
}

#endif//poke

#undef LOGSECTION

extern TM68000 M68000;

#endif//#if defined(SSE_CPU)

#endif //SSECPU_H
