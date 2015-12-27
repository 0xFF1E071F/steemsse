#pragma once
#ifndef SSECPU_H
#define SSECPU_H

#if defined(SSE_CPU)

#if defined(SSE_STRUCTURE_CPU_H)

#include <cpu.decla.h>
#include <d2.decla.h> //V3.7.1

#ifdef SSE_UNIX
void exception(int,exception_action,MEM_ADDRESS);
#endif

#if defined(SSE_STRUCTURE_SSECPU_OBJ)
#ifdef DEBUG_BUILD
#include <debug_emu.decla.h>
#endif
#include <run.decla.h>
#include <blitter.decla.h>
#include <mfp.decla.h>
#include "SSEDebug.h"
#include "SSEShifter.h" //TIMING_INFO
#define LOGSECTION LOGSECTION_CPU 
#endif//#if defined(SSE_STRUCTURE_SSECPU_OBJ)

#else//#if defined(SSE_STRUCTURE_CPU_H)

// forward (due to shitty structure)
#if defined(SSE_VAR_REWRITE)
extern "C" void ASMCALL m68k_trace();
#else
extern "C" ASMCALL void m68k_trace();
#endif
extern void (*m68k_high_nibble_jump_table[16])();
void ASMCALL perform_crash_and_burn();
#if defined(SSE_CPU_DIV)
extern "C" unsigned getDivu68kCycles( unsigned long dividend, unsigned short divisor);
extern "C" unsigned getDivs68kCycles( signed long dividend, signed short divisor);
#endif
#if defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
bool debug_prefetch_timing(WORD ir);
#endif
extern void (*m68k_jump_get_dest_b[8])();
extern void (*m68k_jump_get_dest_w[8])();
extern void (*m68k_jump_get_dest_l[8])();
extern void (*m68k_jump_get_dest_b_not_a[8])();
extern void (*m68k_jump_get_dest_w_not_a[8])();
extern void (*m68k_jump_get_dest_l_not_a[8])();
extern void (*m68k_jump_get_dest_b_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_w_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_l_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_b_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_dest_w_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_dest_l_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_source_b[8])();
extern void (*m68k_jump_get_source_w[8])();
extern void (*m68k_jump_get_source_l[8])();
extern void (*m68k_jump_get_source_b_not_a[8])();
extern void (*m68k_jump_get_source_w_not_a[8])();
extern void (*m68k_jump_get_source_l_not_a[8])();

#endif//!defined(SSE_STRUCTURE_CPU_H)

#if !defined(SSE_SHIFTER)
#include "SSEFrameReport.h" //for some trace
#endif

#if defined(SSE_CPU_INLINE_READ_BWL)
#define m68k_READ_B(addr) M68000.ReadB(addr);
#define m68k_READ_W(addr) M68000.ReadW(addr);
#define m68k_READ_L(addr) M68000.ReadL(addr);
#else //for versions <360

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
#else //for versions <360 (big block)

#if !defined(SSE_CPU_INLINE_READ_FROM_ADDR)

#if defined(SSE_MMU_NO_CONFUSION) //<352

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



#else //352+

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
#endif
#endif//?SSE_CPU_INLINE_READ_FROM_ADDR



#if defined(SSE_BOILER_MONITOR_TRACE)
#define DEBUG_CHECK_IOACCESS \
  if (ioaccess & IOACCESSE_DEBUG_MEM_WRITE_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Write to address $"+HEXSl(debug_mem_write_log_address,6)+ \
                  ", new value is "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
    val=m68k_peek(debug_mem_write_log_address);\
    int val2=debug_mem_write_log_address&1?0:m68k_dpeek(debug_mem_write_log_address);\
    int val3=debug_mem_write_log_address&1?0:m68k_lpeek(debug_mem_write_log_address);\
    TRACE("PC %X %s write %X|%X|%X to %X\n",old_pc,disa_d2(old_pc).Text,  \
      val,val2,val3,debug_mem_write_log_address);\
    ioaccess|=IOACCESSE_DEBUG_MEM_WRITE_LOG;\
  }  else if (ioaccess & IOACCESSE_DEBUG_MEM_READ_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Read from address $"+HEXSl(debug_mem_write_log_address,6)+ \
      ", = "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
    val=m68k_peek(debug_mem_write_log_address);\
    int val2=debug_mem_write_log_address&1?0:m68k_dpeek(debug_mem_write_log_address);\
    int val3=debug_mem_write_log_address&1?0:m68k_lpeek(debug_mem_write_log_address);\
    TRACE("PC %X %s read %X|%X|%X from %X\n",old_pc,disa_d2(old_pc).Text,\
      val,val2,val3,debug_mem_write_log_address);\
    ioaccess|=IOACCESSE_DEBUG_MEM_READ_LOG;\
  } 

#elif defined(DEBUG_BUILD)
#define DEBUG_CHECK_IOACCESS \
  if (ioaccess & IOACCESSE_DEBUG_MEM_WRITE_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Write to address $"+HEXSl(debug_mem_write_log_address,6)+ \
                  ", new value is "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
  }
#else
#define DEBUG_CHECK_IOACCESS
#endif


// keep as macro, wouldn't be inlined in VC6
// but since we changed trace... (change was reverted...)

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

#if defined(SSE_CPU_POKE)  && !defined(SSE_INLINE_370)
void m68k_poke_abus2(BYTE);
void m68k_dpoke_abus2(WORD);
void m68k_lpoke_abus2(LONG);
#endif

// Finer names!
#define IRC   prefetch_buf[1] // Instruction Register Capture
#define IR    prefetch_buf[0] // Instruction Register
#define IRD   ir              // Instruction Register Decoder

struct TM68000 {
  TM68000();
#if defined(SSE_CPU_PREFETCH_TIMING) && !defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
#else//for v341-
  inline void FetchTiming();
  inline void FetchTimingNoRound();
#endif
  inline void FetchWord(WORD &dest_word); 
  inline void InstructionTime(int n);
  inline void InstructionTimeRound(int n);
  void Interrupt(MEM_ADDRESS ad);
  inline void PerformRte();
#if SSE_VERSION<370
  inline //not inlined anyway
#endif
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
  void SetPC(MEM_ADDRESS ad);
  inline void Unstop();
  void Reset(bool cold);
#if defined(SSE_DEBUG)
  int IrAddress; // pc at start
  int nExceptions;
  int nInstr;
  WORD PreviousIr;
  bool NextIrFetched; // fetched next instruction?
#endif//debug
#if defined(SSE_CPU_EXCEPTION)
//  BYTE GetSize(WORD ir);
#if defined(SSE_CPU_TRUE_PC)
  MEM_ADDRESS Pc;
  bool CheckRead; // most 'write' instructions read before, crash at read...
#endif
#endif//excp
#if defined(SSE_CPU_ROUNDING_BUS)
  bool Rounded;
#endif
#if defined(SSE_CPU_ROUNDING_BUS2)
  bool Unrounded; // for blitter
#endif
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

#if defined(SSE_CPU_PREFETCH_CLASS)
  int PrefetchClass; // see ijor's article
#endif

#if defined(SSE_CPU_PREFETCH_CALL)
  BOOL CallPrefetch; 
  WORD FetchForCall(MEM_ADDRESS ad);
  WORD *PrefetchAddress; 
  WORD PrefetchedOpcode;
#endif
#if !defined(SSE_INLINE_370)
  inline 
#endif
    void PrefetchIrc();
#if !defined(SSE_CPU_ROUNDING2)
  inline void PrefetchIrcNoRound();
#endif
  inline void RefetchIr();

#endif

#if defined(SSE_CPU_E_CLOCK)
  bool EClock_synced;

#ifdef SSE_CPU_E_CLOCK_DISPATCHER
  enum {ECLOCK_ACIA,ECLOCK_HBL,ECLOCK_VBL}; //debug/hacks
#endif
#if defined(SSE_CPU_E_CLOCK2)
  int
#else
  void
#endif
  SyncEClock(
#ifdef SSE_CPU_E_CLOCK_DISPATCHER
  int dispatcher
#endif
  );
#if defined(SSE_CPU_E_CLOCK2) && defined(SSE_CPU_E_CLOCK_DISPATCHER)
  BYTE LastEClockCycles[3];
#endif
#endif

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
*/
  enum {NORMAL,EXCEPTION,HALTED,STOPPED,TRACE_MODE,INTEL_CRASH,BLIT_ERROR,BOILER_MESSAGE}; 
  BYTE ProcessingState;
#endif

#if defined(SSE_CPU_DATABUS)
  WORD dbus; // TODO; we have global abus, which isn't always up-to-date
#endif

#if defined(SSE_INT_MFP_REFACTOR2)
  bool Iack;
#endif

};

extern TM68000 M68000;

#if !defined(SSE_INLINE_370)

void TM68000::PrefetchSetPC() { 
  // called by SetPC; we don't count timing here

#if defined(SSE_CPU_FETCH_IO)
  // don't use lpfetch for fetching in IO zone, use io_read: fixes Warp 
  // original STX + Lethal Xcess Beta STX
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    prefetch_buf[0]=io_read_w(pc);
    prefetch_buf[1]=io_read_w(pc+2);
    TRACE("Set PC in IO zone %X\n",pc);
    prefetched_2=true;
    return;
  }
#endif
#if defined(SSE_CPU_FETCH_80000A)
  // don't crash, but the correct value would be 'dbus'
  if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
  {
    prefetch_buf[0]=prefetch_buf[1]=0xFFFF; // default, incorrect?
    prefetched_2=true;
    TRACE("Set PC in empty zone %X\n",pc);
    return;
  }
#endif
  prefetch_buf[0]=*lpfetch;
  prefetch_buf[1]=*(lpfetch+MEM_DIR); 
  prefetched_2=true; // was false in Steem 3.2
#if defined(SSE_CPU_PREFETCH_CALL)
  if(CallPrefetch) 
  {
    ASSERT(PrefetchedOpcode==prefetch_buf[0]);
    prefetch_buf[0]=PrefetchedOpcode;
    CallPrefetch=FALSE;
  }
#endif
  lpfetch+=MEM_DIR;

}

#endif

#define SET_PC(ad) M68000.SetPC(ad);


#if defined(SSE_CPU_TRUE_PC)
#define TRUE_PC M68000.Pc
#define CHECK_READ M68000.CheckRead
#endif


#if defined(SSE_CPU_PREFETCH_TIMING) && !defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)

#define FETCH_TIMING 

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
    TRACE("Fetch IR %X in empty zone %X\n",IR,pc);
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


inline void TM68000::InstructionTime(int t) {
  cpu_cycles-=(t);
}

#define INSTRUCTION_TIME(t)  M68000.InstructionTime(t)


inline void TM68000::InstructionTimeRound(int t) {
  InstructionTime(t);
#if ! defined(SSE_MMU_WAIT_STATES)
#if defined(SSE_CPU_ROUNDING_BUS)
  Rounded=(cpu_cycles&2); //horrible because it's inlined each time
#endif
  cpu_cycles&=-4;
#endif
}

#define INSTRUCTION_TIME_ROUND(t) M68000.InstructionTimeRound(t)

///#ifdef SSE_CPU_ABUS_ACCESS //just new macros
#define CPU_ABUS_ACCESS_READ  M68000.InstructionTimeRound(4)

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

inline void TM68000::PerformRte() {
  // replacing macro M68K_PERFORM_RTE(checkints)
  MEM_ADDRESS pushed_return_address=m68k_lpeek(r[15]+2);
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPop();
#endif
  // An Illegal routine could manipulate this value.
  SetPC(pushed_return_address);
  sr=m68k_dpeek(r[15]);r[15]+=6;    
#if defined(SSE_BOILER_68030_STACK_FRAME)
  if(Debug.M68030StackFrame)
    r[15]+=2;   
#endif  
  sr&=SR_VALID_BITMASK;               

  DETECT_CHANGE_TO_USER_MODE;         
  DETECT_TRACE_BIT;     

#if defined(SSE_INT_MFP_IRQ_DELAY2) //no
/*  v3.5.3: hack for Audio Artistic Demo removed (SSE_INT_MFP_IRQ_DELAY2 not 
    defined)
    Doesn't make sense and breaks ST Magazine STE Demo (Stax 65)!
    See more robust hacks in mfp.cpp, SSE_INT_MFP_PATCH_TIMER_D and 
    SSE_INT_MFP_WRITE_DELAY2.
*/
  if(SSE_HACKS_ON && (ioaccess&IOACCESS_FLAG_DELAY_MFP))
  {
    ioaccess&=~IOACCESS_FLAG_DELAY_MFP;
    ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE; 
  }
#endif

#if defined(SSE_BOILER_TRACE_CONTROL)
  if(TRACE_MASK2&TRACE_CONTROL_RTE)
    TRACE_INT("%d %d %d pc %X RTE to %X\n",TIMING_INFO,old_pc,pushed_return_address);
#endif

#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.Rte();
#endif



}

#define M68K_PERFORM_RTE(checkints) M68000.PerformRte()


#if defined(SSE_CPU_PREFETCH)

#if !defined(SSE_INLINE_370)
// InstructionTimeRound(4); ended up not being inlined
inline void TM68000::PrefetchIrc() {

#if defined(SSE_DEBUG) && defined(SSE_CPU_PREFETCH_ASSERT)
  ASSERT(!prefetched_2); // strong, only once per instruction 
#endif

#if !defined(SSE_CPU_FETCH_IO2)
  ASSERT( !(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260)) );
#else 
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    IRC=io_read_w(pc); 
    TRACE_LOG("IRC %X in IO zone %X\n",IRC,pc);
  }
  else
#endif
#if defined(SSE_CPU_FETCH_80000C)
  if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
  {
    IRC=0xFFFF;
    TRACE("Fetch IRC %X in empty zone %X\n",IRC,pc);
  }
  else
#endif
    IRC=*lpfetch;
  prefetched_2=true;
#if defined(SSE_CPU_PREFETCH_TIMING) // we count fetch timing here
#if defined(SSE_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK_CPU & OSD_CONTROL_CPUPREFETCH) 
#endif
      TRACE_OSD("IRC 2X FETCH"); // generally false alert at start-up
  }
  NextIrFetched=true;
#endif
#if defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(IRD))
    ; else
#endif
    InstructionTimeRound(4);
#endif//SSE_CPU_PREFETCH_TIMING

}

#endif

#if !defined(SSE_CPU_ROUNDING2) //no more needed

inline void TM68000::PrefetchIrcNoRound() { // the same except no rounding

#if defined(SSE_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  ASSERT(!prefetched_2); // strong, only once per instruction 
#endif
#if !defined(SSE_CPU_FETCH_IO2)
  ASSERT( !(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260)) );
#else 
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    IRC=io_read_w(pc); 
    TRACE_LOG("IRC %X in IO zone %X\n",IRC,pc);
  }
  else
#endif
  IRC=*lpfetch;
  prefetched_2=true;
#if defined(SSE_CPU_PREFETCH_TIMING) // we count fetch timing here
#if defined(SSE_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK_CPU & OSD_CONTROL_CPUPREFETCH) 
#endif
    TRACE_OSD("IRC 2X FETCH"); // generally false alert at start-up
  }
  NextIrFetched=true;
#endif
#if defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(IRD))
    ; else
#endif
    InstructionTime(4);
#endif//SSE_CPU_PREFETCH_TIMING

}
#endif

#define EXTRA_PREFETCH //all prefetches actually are "extra"!
#define PREFETCH_IRC  M68000.PrefetchIrc()
#if !defined(SSE_CPU_ROUNDING2)
#define PREFETCH_IRC_NO_ROUND  M68000.PrefetchIrcNoRound()
#endif
#else //not defined, macros do nothing

#define EXTRA_PREFETCH
#define PREFETCH_IRC
#if !defined(SSE_CPU_ROUNDING2)
#define PREFETCH_IRC_NO_ROUND
#endif
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
  if(TRACE_ENABLED)
  {
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK4 & TRACE_CONTROL_CPU_CYCLES)
    {
      TRACE_LOG("\nCycles %d %d %d (%d)\n",ACT,FRAMECYCLES,LINECYCLES,scan_y);
    }

//#if defined(SSE_DEBUG_TRACE_IO) 

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
  if(ProcessingState==TRACE_MODE)
  {
    ProcessingState=NORMAL;
  }
  else if(sr&SR_TRACE)
  {
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
#if defined(SSE_CPU_MOVE_B) && defined(SSE_CPU_MOVE_W) && defined(SSE_CPU_MOVE_L)\
  && defined(SSE_CPU_DIV)
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

//not defined
#if defined(SSE_IPF_CPU) //|| defined(SSE_DEBUG)
  int cycles=cpu_cycles;
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
  if(ProcessingState==TRACE_MODE)
  {
#ifdef DEBUG_BUILD
    if(!Debug.logsection_enabled[LOGSECTION_CPU] && !logsection_enabled[LOGSECTION_CPU])
      TRACE_LOG("(T) PC %X SR %X VEC %X IR %04X: %s\n",old_pc,sr,LPEEK(0x24),ir,disa_d2(old_pc).Text);
#else
    TRACE_LOG("TRACE PC %X IR %X SR %X $24 %X\n",pc,ir,sr,LPEEK(0x24));
#endif
    INSTRUCTION_TIME_ROUND(0); // Round first for interrupts
    INSTRUCTION_TIME_ROUND(34); // note: tested timing (Legacy.msa)
    //INSTRUCTION_TIME(36);
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("TRACE");
#endif
    m68k_interrupt(LPEEK(BOMBS_TRACE_EXCEPTION*4));
  }
#undef LOGSECTION
#define LOGSECTION LOGSECTION_CPU 
#endif//SSE_CPU_TRACE_REFACTOR

#if defined(SSE_IPF_CPU) // no
 if(Caps.Active)
  {
    int cpucycles=cycles-cpu_cycles;
    ASSERT( cpucycles>0 );
    CapsFdcEmulate(&WD1772,cpucycles);
  }
#endif
#if defined(SSE_DEBUG)
  NextIrFetched=false; // in FETCH_TIMING or...// check_interrut
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
  ASSERT( MEM_DIR==-1 );
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

void TM68000::ReadB(MEM_ADDRESS addr)
{
  //m68k_READ_B(addr)
  m68k_src_b=m68k_peek(addr);
}

void TM68000::ReadW(MEM_ADDRESS addr)
{
  //m68k_READ_W(addr)
  m68k_src_w=m68k_dpeek(addr);
}

void TM68000::ReadL(MEM_ADDRESS addr)
{
  //m68k_READ_L(addr)
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

#if defined(SSE_INLINE_370) || defined(DEBUG_BUILD)

void m68k_poke_abus(BYTE x);
void m68k_dpoke_abus(WORD x);
void m68k_lpoke_abus(LONG x);

#else

NOT_DEBUG(inline) void m68k_poke_abus(BYTE x){
  abus&=0xffffff; // annoying that we must do this
  BOOL super=SUPERFLAG;
  if(abus>=MEM_IO_BASE && super)
    io_write_b(abus,x);
#if defined(SSE_CPU_CHECK_VIDEO_RAM_B)
/*  To save some performance, we do just one basic Shifter test in the inline
    part. More precise test is in m68k_poke_abus2().
*/
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
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
    m68k_poke_abus2(x);
}
 

NOT_DEBUG(inline) void m68k_dpoke_abus(WORD x){
  abus&=0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1) 
    ::exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE && super)
      io_write_w(abus,x);
#if defined(SSE_CPU_CHECK_VIDEO_RAM_W) // 3615 GEN4 100
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
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
    m68k_dpoke_abus2(x);
}


NOT_DEBUG(inline) void m68k_lpoke_abus(LONG x){
  abus&=0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1)
    ::exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else 
  if(abus>=MEM_IO_BASE && super)
    io_write_l(abus,x);
#if defined(SSE_CPU_CHECK_VIDEO_RAM_L)
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
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
    m68k_lpoke_abus2(x);
}


#endif//!debug


// those will be inlined where it counts (cpu.cpp), not where it
// doesn't count (stemdos.cpp)
void m68k_poke(MEM_ADDRESS ad,BYTE x){
  ASSERT(ad);
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
#if defined(SSE_INLINE_370) 
    m68k_poke_abus(x);
#else
    m68k_poke_abus2(x);
#endif
}


void m68k_dpoke(MEM_ADDRESS ad,WORD x){
  ASSERT(ad);
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
#if defined(SSE_INLINE_370) 
    m68k_dpoke_abus(x);
#else
    m68k_dpoke_abus2(x);
#endif
}


void m68k_lpoke(MEM_ADDRESS ad,LONG x){
  ASSERT(ad);
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
#if defined(SSE_INLINE_370)
    m68k_lpoke_abus(x);
#else
    m68k_lpoke_abus2(x);
#endif
}

#endif//poke





#if defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
// this is a silly debug-only function, very useful at some point, we keep
// it just in case...
// pb when we undef/def only parts of instructions... not that super
// important because it's just "read SDP"

bool debug_prefetch_timing(WORD ir) {
  bool answer=false;
#ifdef SSE_CPU_PREFETCH_TIMING_EXCEPT
#ifdef SSE_CPU_LINE_0_TIMINGS 
  if( (ir&0xF000)==0x0000)
    answer=true;
#endif
#ifdef SSE_CPU_MOVE_B_TIMINGS 
  if( (ir&0xF000)==0x1000)
    answer=true;
#endif
#ifdef SSE_CPU_MOVE_L_TIMINGS
  if( (ir&0xF000)==0x2000)
    answer=true;
#endif
#ifdef SSE_CPU_MOVE_W_TIMINGS
  if( (ir&0xF000)==0x3000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_4_TIMINGS 
  if( (ir&0xF000)==0x4000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_5_TIMINGS 
  if( (ir&0xF000)==0x5000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_6_TIMINGS 
  if( (ir&0xF000)==0x6000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_7_TIMINGS 
  if( (ir&0xF000)==0x7000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_8_TIMINGS
  if( (ir&0xF000)==0x8000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_9_TIMINGS
  if( (ir&0xF000)==0x9000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_B_TIMINGS
  if( (ir&0xF000)==0xB000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_C_TIMINGS
  if( (ir&0xF000)==0xC000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_D_TIMINGS
  if( (ir&0xF000)==0xD000)
    answer=true;
#endif
#ifdef SSE_CPU_LINE_E_TIMINGS
  if( (ir&0xF000)==0xE000)
    answer=true;
#endif
#endif
  return answer;
}
#endif

#undef LOGSECTION

#endif//#if defined(SSE_CPU)

#endif //SSECPU_H
