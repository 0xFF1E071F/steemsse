#pragma once
#ifndef SSECPU_H
#define SSECPU_H

#if defined(SS_CPU)



#if defined(SS_STRUCTURE_CPU_H)

#include <cpu.decla.h>

#ifdef SS_UNIX
void exception(int,exception_action,MEM_ADDRESS);
#endif

#if defined(SS_STRUCTURE_SSECPU_OBJ)
#ifdef DEBUG_BUILD
#include <debug_emu.decla.h>
#endif
#include <run.decla.h>
#include <blitter.decla.h>
#include <mfp.decla.h>
#include "SSEDebug.h"

#include "SSEShifter.h" //TIMING_INFO
#define LOGSECTION LOGSECTION_CPU 

#endif//#if defined(SS_STRUCTURE_SSECPU_OBJ)

#else

///#undef IN_EMU
///#include <cpu.h>

// forward (due to shitty structure)
#if defined(SS_VAR_REWRITE)
extern "C" void ASMCALL m68k_trace();
#else
extern "C" ASMCALL void m68k_trace();
#endif
extern void (*m68k_high_nibble_jump_table[16])();
void ASMCALL perform_crash_and_burn();
#if defined(SS_CPU_DIV)
extern "C" unsigned getDivu68kCycles( unsigned long dividend, unsigned short divisor);
extern "C" unsigned getDivs68kCycles( signed long dividend, signed short divisor);
#endif
#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
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

#endif//!defined(SS_STRUCTURE_CPU_H)




#if defined(SS_CPU_INLINE_READ_BWL)
#define m68k_READ_B(addr) M68000.ReadB(addr);
#define m68k_READ_W(addr) M68000.ReadW(addr);
#define m68k_READ_L(addr) M68000.ReadL(addr);
#endif

#if defined(SS_CPU_INLINE_READ_FROM_ADDR)
#define m68k_READ_B_FROM_ADDR M68000.m68kReadBFromAddr();
#define m68k_READ_W_FROM_ADDR M68000.m68kReadWFromAddr();
#define m68k_READ_L_FROM_ADDR M68000.m68kReadLFromAddr();
#endif










#ifdef DEBUG_BUILD
#define DEBUG_CHECK_IOACCESS \
  if (ioaccess & IOACCESS_DEBUG_MEM_WRITE_LOG){ \
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


#if defined(SS_CPU_POKE)
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
  inline void FetchTiming();
  inline void FetchTimingNoRound();
  inline void FetchWord(WORD &dest_word); 
  inline void InstructionTime(int n);
  inline void InstructionTimeRound(int n);
  void Interrupt(MEM_ADDRESS ad);
  inline void PerformRte();
  inline void PrefetchSetPC();
  inline void Process();
#if defined(SS_CPU_INLINE_READ_BWL)
  static inline void ReadB(MEM_ADDRESS addr);
  static inline void ReadW(MEM_ADDRESS addr);
  static inline void ReadL(MEM_ADDRESS addr);
#endif
#if defined(SS_CPU_INLINE_READ_FROM_ADDR)
  static void m68kReadBFromAddr();
  static void m68kReadWFromAddr();
  static void m68kReadLFromAddr();
#endif
  void SetPC(MEM_ADDRESS ad);
  inline void Unstop();
  void Reset(bool cold);
#if defined(SS_DEBUG)
  int IrAddress; // pc at start
  int nExceptions;
  int nInstr;
  WORD PreviousIr;
  bool NextIrFetched; // fetched next instruction?
#endif//debug
#if defined(SS_CPU_EXCEPTION)
//  BYTE GetSize(WORD ir);
#if defined(SS_CPU_TRUE_PC)
  MEM_ADDRESS Pc;
  bool CheckRead; // most 'write' instructions read before, crash at read...
#endif
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


#if defined(SS_CPU_PREFETCH)

#if defined(SS_CPU_PREFETCH_CLASS)
  int PrefetchClass; // see ijor's article
#endif

#if defined(SS_CPU_PREFETCH_CALL)
  BOOL CallPrefetch; 
  WORD FetchForCall(MEM_ADDRESS ad);
  WORD *PrefetchAddress; 
  WORD PrefetchedOpcode;
#endif

  inline void PrefetchIrc();
  inline void PrefetchIrcNoRound();
  inline void RefetchIr();

#endif
};

extern TM68000 M68000;


void TM68000::PrefetchSetPC() { 
  // called by SetPC; we don't count timing here

#if defined(SS_CPU_FETCH_IO)
  // don't use lpfetch for fetching in IO zone, use io_read: fixes Warp original
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    prefetch_buf[0]=io_read_w(pc);
    prefetch_buf[1]=io_read_w(pc+2);
    TRACE("Set PC in IO zone %X\n",pc);
    prefetched_2=true;
    return;
  }
#endif
  prefetch_buf[0]=*lpfetch;
  prefetch_buf[1]=*(lpfetch+MEM_DIR); 
  prefetched_2=true; // was false in Steem 3.2
#if defined(SS_CPU_PREFETCH_CALL)
  if(M68000.CallPrefetch) 
  {
    ASSERT(M68000.PrefetchedOpcode==prefetch_buf[0]);
    prefetch_buf[0]=M68000.PrefetchedOpcode;
    M68000.CallPrefetch=FALSE;
  }
#endif
  lpfetch+=MEM_DIR;

}


#define SET_PC(ad) M68000.SetPC(ad);


#if defined(SS_CPU_TRUE_PC)
#define TRUE_PC M68000.Pc
#define CHECK_READ M68000.CheckRead
#endif


inline void TM68000::FetchTiming() {
#if !defined(SS_CPU_PREFETCH_TIMING)

#if defined(SS_DEBUG)
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
  }
  NextIrFetched=true;
#endif  
  InstructionTimeRound(4); 
#endif
  
#if defined(SS_CPU_PREFETCH_TIMING) && defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(ir))
    InstructionTimeRound(4); 
#endif

}
#define FETCH_TIMING M68000.FetchTiming()


#if defined(SS_CPU_FETCH_TIMING)

inline void TM68000::FetchTimingNoRound() {
#if !defined(SS_CPU_PREFETCH_TIMING)

#if defined(SS_DEBUG)
  ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
  }
  NextIrFetched=true;
#endif  
  InstructionTime(4); 
#endif
  
#if defined(SS_CPU_PREFETCH_TIMING) && defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(ir))
    InstructionTime(4); 
#endif

}
#define FETCH_TIMING_NO_ROUND M68000.FetchTimingNoRound()

#endif


inline void TM68000::FetchWord(WORD &dest_word) {
  dest_word=IR;
  if(prefetched_2)// already fetched
  {
    if(IRC!=*lpfetch)
    {
/*  The "prefetch trick" was used by some programs to crash when the CPU
    was in 'trace' mode 
    eg Anomaly, Transbeauce 1, Transbeauce 2
    Operation Clean Streets (Auto 168)
*/
#if defined(SS_CPU_PREFETCH_TRACE)
      TRACE_LOG("Prefetched IRC:%X current:%X\n",IRC,*lpfetch);
//      TRACE_OSD("PREFETCH"); // ST-CNX
#endif
#if defined(SS_OSD_CONTROL)
  if(OSD_MASK1 & OSD_CONTROL_CPUPREFETCH) 
    TRACE_OSD("PREFETCH");
#endif

#if defined(SS_BETA) && defined(SS_CPU_NO_PREFETCH)
      IRC=*lpfetch; // enabling this cancels prefetch (don't)
#endif
    }
    IR=IRC;
    prefetched_2=false;
  }
  else
#if defined(SS_CPU_FETCH_IO_FULL)
  // don't use lpfetch for fetching in IO zone, use io_read
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    IR=io_read_w(pc);
    TRACE_LOG("Fetch word %X in IO zone %X\n",IR,pc);
    return;
  }
  else
#endif
    IR=*lpfetch; // next instr
  //ASSERT( !(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260)) ); // Warp, Union
#if defined(SS_CPU_FETCH_IO)
  if(pc>=MEM_IO_BASE) // normally it took IRC
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
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWord() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLong() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestByteNotA() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWordNotA() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLongNotA() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestByteNotAOrD() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWordNotAOrD() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLongNotAOrD() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestByteNotAFasterForD() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a_faster_for_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestWordNotAFasterForD() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a_faster_for_d[(ir&BITS_543)>>3]();
}


inline void TM68000::GetDestLongNotAFasterForD() {
#if defined(SS_CPU_TRUE_PC)
  if(!CheckRead)
    Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a_faster_for_d[(ir&BITS_543)>>3]();
}


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
#if ! defined(SS_MMU_WAIT_STATES)
  cpu_cycles&=-4;
#endif
}
#define INSTRUCTION_TIME_ROUND(t) M68000.InstructionTimeRound(t)


#define m68k_interrupt(ad) M68000.Interrupt(ad)


inline void TM68000::PerformRte() {
  // replacing macro M68K_PERFORM_RTE(checkints)
  MEM_ADDRESS pushed_return_address=m68k_lpeek(r[15]+2);
  // An Illegal routine could manipulate this value.
  SetPC(pushed_return_address);
  sr=m68k_dpeek(r[15]);r[15]+=6;    
#if defined(SS_DEBUG_STACK_68030_FRAME)
  if(Debug.M68030StackFrame)
    r[15]+=2;   
#endif  
  sr&=SR_VALID_BITMASK;               

  DETECT_CHANGE_TO_USER_MODE;         
  DETECT_TRACE_BIT;       
#if defined(SS_MFP_IRQ_DELAY2)
/*  v3.5.3: hack for Audio Artistic Demo removed (SS_MFP_IRQ_DELAY2 not 
    defined)
    Doesn't make sense and breaks ST Magazine STE Demo (Stax 65)!
    See a more robust hack in mfp.cpp, SS_MFP_PATCH_TIMER_D
*/
  if(SSE_HACKS_ON && (ioaccess&IOACCESS_FLAG_DELAY_MFP))
  {
    ioaccess&=~IOACCESS_FLAG_DELAY_MFP;
    ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE; 
  }
#endif

#if defined(SS_DEBUG_SHOW_INTERRUPT)
  Debug.Rte();
#endif

}
#define M68K_PERFORM_RTE(checkints) M68000.PerformRte()


#if defined(SS_CPU_PREFETCH)

inline void TM68000::PrefetchIrc() {

#if defined(SS_DEBUG) && defined(SS_CPU_PREFETCH_ASSERT)
  ASSERT(!prefetched_2); // strong, only once per instruction 
#endif

#if !defined(SS_CPU_FETCH_IO_FULL)
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
#if defined(SS_CPU_PREFETCH_TIMING) // we count fetch timing here
#if defined(SS_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
    TRACE_OSD("IRC 2X FETCH"); // generally false alert at start-up
  }
  NextIrFetched=true;
#endif
#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(IRD))
    ; else
#endif
    InstructionTimeRound(4);
#endif//SS_CPU_PREFETCH_TIMING

}


inline void TM68000::PrefetchIrcNoRound() { // the same except no rounding

#if defined(SS_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  ASSERT(!prefetched_2); // strong, only once per instruction 
#endif
#if !defined(SS_CPU_FETCH_IO_FULL)
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
#if defined(SS_CPU_PREFETCH_TIMING) // we count fetch timing here
#if defined(SS_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
    TRACE_OSD("IRC 2X FETCH"); // generally false alert at start-up
  }
  NextIrFetched=true;
#endif
#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(IRD))
    ; else
#endif
    InstructionTime(4);
#endif//SS_CPU_PREFETCH_TIMING

}

#define EXTRA_PREFETCH //all prefetches actually are "extra"!
#define PREFETCH_IRC  M68000.PrefetchIrc()
#define PREFETCH_IRC_NO_ROUND  M68000.PrefetchIrcNoRound()

#else //not defined, macros do nothing

#define EXTRA_PREFETCH
#define PREFETCH_IRC
#define PREFETCH_IRC_NO_ROUND

#endif

// core function, dispatches in cpu emu, inlining m68k_PROCESS

inline void TM68000::Process() {
  LOG_CPU  
#if defined(SS_DEBUG)
  IrAddress=pc;
  PreviousIr=IRD;
  nInstr++;
#if defined(SS_CPU_FETCH_TIMING) && defined(DEBUG_BUILD) \
  && defined(SS_DEBUG_TRACE_PREFETCH)
  // 4e72 = STOP, where no fetching occurs
  // 4afc = ILLEGAL
//  ASSERT( NextIrFetched || ir==0x4e72 || ir==0x4afc); // strong
  if(!NextIrFetched && ir!=0x4e72)
  {
    EasyStr instr=disa_d2(old_pc);
    TRACE_LOG("%x %x %s no FETCH_TIMING\n",old_pc,PreviousIr,instr.c_str());
  }
#endif
//  NextIrFetched=false; // in FETCH_TIMING or...
#endif//debug

#if defined(SS_CPU_CHECK_PC) 
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

  old_pc=pc;  

  //temp...MFD
//  if(pc==0x036AEE) 
  //  TRACE("PC %X reached at %d-%d-%d\n",pc,TIMING_INFO);

//ASSERT(old_pc!=0x006164);
#if defined(SS_CPU_PREFETCH)
/*  basic prefetch rule:
Just before the start of any instruction two words (no more and no less) are 
already fetched. One word will be in IRD and another one in IRC.
*/
#if defined(SS_CPU_PREFETCH_ASSERT)

#if defined(SS_CPU_MOVE_B) && defined(SS_CPU_MOVE_W) && defined(SS_CPU_MOVE_L)\
  && defined(SS_CPU_DIV)
  ASSERT(prefetched_2); // strong
#endif
#endif
#if defined(SS_CPU_PREFETCH_CLASS)
  PrefetchClass=0; // default, most instructions
#endif
#endif

  FetchWord(IRD); // IR->IRD (using reference)
  // TODO should already be there?
  // TODO assert illegal here, but perf?

#if defined(SS_CPU_EXCEPTION) && defined(SS_CPU_EXCEPTION_TRACE_PC)//heavy!!!
  if(nExceptions>1) 
    TRACE_LOG("%X\n",pc);
#endif

  pc+=2; // in fact it was already set in previous instruction

#if defined(SS_CPU_TRUE_PC)
  Pc=pc; // anyway
  CheckRead=0;
#endif

#if defined(SS_IPF_CPU) //|| defined(SS_DEBUG)
  int cycles=cpu_cycles;
#endif

  //ASSERT(ir!=0x91AE); // dbg: break on opcode
  /////////// JUMP TO CPU EMU: ///////////////
  m68k_high_nibble_jump_table[ir>>12](); // go to instruction...
#if defined(SS_IPF_CPU) // no
 if(Caps.Active)
  {
    int cpucycles=cycles-cpu_cycles;
    ASSERT( cpucycles>0 );
    CapsFdcEmulate(&WD1772,cpucycles);
  }
#endif
#if defined(SS_DEBUG)
  NextIrFetched=false; // in FETCH_TIMING or...// check_interrut
#endif//debug

  HANDLE_IOACCESS( m68k_trace(); ); // keep as macro, wouldn't be inlined

  DEBUG_ONLY( debug_first_instruction=0 );
}
#define m68k_PROCESS M68000.Process();


#if defined(SS_CPU_PREFETCH)

inline void TM68000::RefetchIr() {
//this is no fix, it was already in Steem 3.2
  ASSERT( IR==*(lpfetch+1) ); // detect cases Synth Dream Sound Demo II
//  ASSERT( MEM_DIR==-1 );
  IR=*(lpfetch-MEM_DIR); //needed for Synth Dream Sound Demo II!
}
#define REFETCH_IR  M68000.RefetchIr();
#else
#define REFETCH_IR

#endif


#if defined(SS_CPU_INLINE_READ_BWL)

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
  if (cpu_stopped){
                   
    cpu_stopped=false;     
    SetPC((pc+4) | pc_high_byte);          
  }
}
#define M68K_UNSTOP M68000.Unstop()


////////////////////////
// Poke, inlined part //
////////////////////////


#if defined(SS_CPU_POKE)

#if defined(DEBUG_BUILD)

void m68k_poke_abus(BYTE x);
void m68k_dpoke_abus(WORD x);
void m68k_lpoke_abus(LONG x);

#else

NOT_DEBUG(inline) void m68k_poke_abus(BYTE x){
  abus&=0xffffff; // annoying that we must do this
  BOOL super=SUPERFLAG;
  if(abus>=MEM_IO_BASE && super)
    io_write_b(abus,x);
#if defined(SS_CPU_CHECK_VIDEO_RAM_B)
/*  To save some performance, we do just one basic shifter test in the inline
    part. More precise test is in m68k_poke_abus2().
*/
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
  {
#if defined(SS_DEBUG_MONITOR_VALUE2)
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
#if defined(SS_CPU_CHECK_VIDEO_RAM_W) // 3615 GEN4 100
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
  {
#if defined(SS_DEBUG_MONITOR_VALUE2)
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
#if defined(SS_CPU_CHECK_VIDEO_RAM_L)
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
  {
#if defined(SS_DEBUG_MONITOR_VALUE2)//3.6.1, also for long of course (argh!)
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
#if defined(SS_DEBUG_MONITOR_VALUE2)
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
#if defined(SS_DEBUG_MONITOR_VALUE2)
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
#if defined(SS_DEBUG_MONITOR_VALUE2)//3.6.1, also for long of course (argh!)
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


#endif//poke





#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
// this is a silly debug-only function, very useful at some point, we keep
// it just in case...
// pb when we undef/def only parts of instructions... not that super
// important because it's just "read SDP"

bool debug_prefetch_timing(WORD ir) {
  bool answer=false;
#ifdef SS_CPU_PREFETCH_TIMING_EXCEPT
#ifdef SS_CPU_LINE_0_TIMINGS 
  if( (ir&0xF000)==0x0000)
    answer=true;
#endif
#ifdef SS_CPU_MOVE_B_TIMINGS 
  if( (ir&0xF000)==0x1000)
    answer=true;
#endif
#ifdef SS_CPU_MOVE_L_TIMINGS
  if( (ir&0xF000)==0x2000)
    answer=true;
#endif
#ifdef SS_CPU_MOVE_W_TIMINGS
  if( (ir&0xF000)==0x3000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_4_TIMINGS 
  if( (ir&0xF000)==0x4000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_5_TIMINGS 
  if( (ir&0xF000)==0x5000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_6_TIMINGS 
  if( (ir&0xF000)==0x6000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_7_TIMINGS 
  if( (ir&0xF000)==0x7000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_8_TIMINGS
  if( (ir&0xF000)==0x8000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_9_TIMINGS
  if( (ir&0xF000)==0x9000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_B_TIMINGS
  if( (ir&0xF000)==0xB000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_C_TIMINGS
  if( (ir&0xF000)==0xC000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_D_TIMINGS
  if( (ir&0xF000)==0xD000)
    answer=true;
#endif
#ifdef SS_CPU_LINE_E_TIMINGS
  if( (ir&0xF000)==0xE000)
    answer=true;
#endif
#endif
  return answer;
}
#endif

#undef LOGSECTION

#endif//#if defined(SS_CPU)

#endif //SSECPU_H
