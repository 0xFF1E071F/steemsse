#pragma once
#ifndef SSECPU_H
#define SSECPU_H

#if defined(SS_CPU)

#define LOGSECTION LOGSECTION_CRASH // not correct, temp, cpu is trouble


#if defined(SS_STRUCTURE_CPU_H)
#include "../cpu.decla.h" // hey look it already works for this

#elif defined(SS_STRUCTURE_BIG_FORWARD)
// ...
#elif !defined(SS_STRUCTURE_BIG_FORWARD)

// forward (due to shitty structure)
#if defined(SS_VAR_REWRITE)
extern "C" void ASMCALL m68k_trace();
#else
extern "C" ASMCALL void m68k_trace();
#endif
extern void (*m68k_high_nibble_jump_table[16])();
//void HandleIOAccess();
void ASMCALL perform_crash_and_burn();
#if defined(SS_CPU_DIV)
extern "C" unsigned getDivu68kCycles( unsigned long dividend, unsigned short divisor);
extern "C" unsigned getDivs68kCycles( signed long dividend, signed short divisor);
#endif
#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
bool debug_prefetch_timing(WORD ir);
#endif
// forward
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
// but since we changed trace...

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
#define IRC   prefetch_buf[1]
#define IR    prefetch_buf[0]
#define IRD   ir


#include "SSEM68000.h" // there you go...







#if defined(SS_CPU_TRUE_PC)
#define TRUE_PC M68000.Pc
#define CHECK_READ M68000.CheckRead
#endif



// inline functions of TM68000 (instead of macros)

inline void TM68000::FetchTiming() {
#if !defined(SS_CPU_PREFETCH_TIMING)

#if defined(SS_DEBUG)
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
  }
//#if defined(DEBUG_BUILD)
  NextIrFetched=true;
//#endif
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
#if defined(SS_CPU_PREFETCH_DETECT_IRC_TRICK) && defined(SS_DEBUG)
    if(!Debug.CpuPrefetchDiffDetected) {
      ASSERT( IRC==*lpfetch ); 
      if(IRC!=*lpfetch) Debug.CpuPrefetchDiffDetected=true;
    }
#endif
    if(IRC!=*lpfetch)
    {
/*  The "prefetch trick" was used by some programs to crash when the CPU
    was in 'trace' mode 
    eg Anomaly, Transbeauce 1, Transbeauce 2
    Operation Clean Streets (Auto 168)
*/
#if defined(SS_CPU_PREFETCH_TRACE)
      TRACE_LOG("Prefetched IRC:%X current:%X\n",prefetch_buf[1],*lpfetch);
      if(IRC!=*lpfetch && !Debug.CpuPrefetchDiffDetected) 
      {
        OsdControl.StartScroller("CPU possible prefetch trick");
        Debug.CpuPrefetchDiffDetected=true;
      }
#endif
#if defined(SS_CPU_NO_PREFETCH)
      prefetch_buf[1]=*lpfetch; // enabling this cancels prefetch (don't)
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
    TRACE_LOG("Fetch word %X in IO zone %X\n",prefetch_buf[0],pc);
    return;
  }
  else
#endif
    IR=*lpfetch; // next instr
  ASSERT( !(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260)) ); // Warp, Union
#if defined(SS_CPU_FETCH_IO)
  if(pc>=MEM_IO_BASE) // normally it took IRC
    return; //if we test that, we may as well avoid useless inc "PC"
#endif
  lpfetch+=MEM_DIR; // advance the fetch pointer
  if(lpfetch MEM_GE lpfetch_bound) // MEM_GE : <=
    exception(BOMBS_BUS_ERROR,EA_FETCH,pc);
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


inline void TM68000::Interrupt(MEM_ADDRESS ad) {
  WORD _sr=sr;
  if (!SUPERFLAG) 
    change_to_supervisor_mode();
#if defined(SS_CPU_PREFETCH_CLASS)
  PrefetchClass=2;
#endif
  m68k_PUSH_L(PC32);
  m68k_PUSH_W(_sr);
  SetPC(ad);
  SR_CLEAR(SR_TRACE);
  interrupt_depth++;
//  TRACE_LOG("%X %d\n",ad,interrupt_depth);
}
#define m68k_interrupt(ad) M68000.Interrupt(ad)


inline void TM68000::PerformRte() {
  // replacing macro M68K_PERFORM_RTE(checkints)
  MEM_ADDRESS pushed_return_address=m68k_lpeek(r[15]+2);
  // An Illegal routine could manipulate this value.
  SetPC(pushed_return_address);
  sr=m68k_dpeek(r[15]);r[15]+=6;      
  sr&=SR_VALID_BITMASK;               
  DETECT_CHANGE_TO_USER_MODE;         
  DETECT_TRACE_BIT;        
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
  
  IRC=*lpfetch; //prefetch_buf[1]=*lpfetch;
  prefetched_2=TRUE;
#if defined(SS_CPU_PREFETCH_TIMING) // we count fetch timing here
#if defined(SS_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
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
  //prefetch_buf[1]=*lpfetch;
  IRC=*lpfetch;
  prefetched_2=TRUE;
#if defined(SS_CPU_PREFETCH_TIMING) // we count fetch timing here
#if defined(SS_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
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

#if defined(SS_CPU_PREFETCH)
/*  basic prefetch rule:
Just before the start of any instruction two words (no more and no less) are 
already fetched. One word will be in IRD and another one in IRC.
*/
#if defined(SS_CPU_PREFETCH_ASSERT)
  ASSERT(prefetched_2); // strong
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

#if defined(SS_IPF_CPU) || defined(SS_DEBUG)
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
  ASSERT( IR==*(lpfetch+1) ); // detect cases (none yet!)
  ASSERT( MEM_DIR==-1 );
  IR=*(lpfetch-MEM_DIR);
}
#define REFETCH_IR  M68000.RefetchIr();
#else
#define REFETCH_IR

#endif



// They're not much called
inline void TM68000::m68kReadBFromAddr() {
  // Replacing macro m68k_READ_B_FROM_ADDR.
  abus&=0xffffff;
  if(abus>=himem)
  {                                  
    if(abus>=MEM_IO_BASE)
    {            
      if(SUPERFLAG)
        m68k_src_b=io_read_b(abus);           
      else 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);         
    }
    else if(abus>=0xfc0000)
    {                             
      if(tos_high && abus<(0xfc0000+192*1024))
        m68k_src_b=ROM_PEEK(abus-rom_addr);   
      else if (abus<0xfe0000 || abus>=0xfe2000) 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);  
    }
    else if(abus>=MEM_EXPANSION_CARTRIDGE)
    {           
      if(cart)
        m68k_src_b=CART_PEEK(abus-MEM_EXPANSION_CARTRIDGE);  
      else
        m68k_src_b=(BYTE)0xff;
    }
    else if (abus>=rom_addr)
    {                         
      if(abus<(0xe00000+256*1024))
        m68k_src_b=ROM_PEEK(abus-rom_addr);                           
      else if (abus>=0xec0000)
        exception(BOMBS_BUS_ERROR,EA_READ,abus);          
      else
        m68k_src_b=(BYTE)0xff;                                          
    }
    else if (abus>=0xd00000 && abus<0xd80000)
      m68k_src_b=(BYTE)0xff;               
#if !defined(SS_MMU_NO_CONFUSION)      
    else if(mmu_confused)
      m68k_src_b=mmu_confused_peek(abus,true);                                         
#endif
    else if(abus>=FOUR_MEGS)
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          
    else
      m68k_src_b=(BYTE)0xff;                                          
  }
  else if(abus>=MEM_START_OF_USER_AREA)
  {                                              
    DEBUG_CHECK_READ_B(abus);  
    m68k_src_b=(BYTE)(PEEK(abus));                  
  }
  else if(SUPERFLAG)
  {     
    DEBUG_CHECK_READ_B(abus);  
    m68k_src_b=(BYTE)(PEEK(abus));                  
  }
  else
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}
#define m68k_READ_B_FROM_ADDR M68000.m68kReadBFromAddr();        


inline void TM68000::m68kReadWFromAddr() {
  // Replacing macro m68k_READ_W_FROM_ADDR.
  abus&=0xffffff;                                   
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    
  else if(abus>=himem)
  {                                  
    if(abus>=MEM_IO_BASE)
    {            
      if(SUPERFLAG)
        m68k_src_w=io_read_w(abus);           
      else 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);         
    }
    else if(abus>=0xfc0000)
    {                             
      if(tos_high && abus<(0xfc0000+192*1024))
        m68k_src_w=ROM_DPEEK(abus-rom_addr);   
      else if(abus<0xfe0000 || abus>=0xfe2000) 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);  
    }
    else if(abus>=MEM_EXPANSION_CARTRIDGE)
    {           
      if(cart)
        m68k_src_w=CART_DPEEK(abus-MEM_EXPANSION_CARTRIDGE);  
      else
        m68k_src_w=(WORD)0xffff;                                    
    }
    else if(abus>=rom_addr)
    {                         
      if(abus<(0xe00000+256*1024)) 
        m68k_src_w=ROM_DPEEK(abus-rom_addr);                           
      else if (abus>=0xec0000) 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);          
      else 
        m68k_src_w=(WORD)0xffff;                                          
    }
    else if (abus>=0xd00000 && abus<0xd80000)
      m68k_src_w=(WORD)0xffff;                                          
#if !defined(SS_MMU_NO_CONFUSION)
    else if(mmu_confused)
      m68k_src_w=mmu_confused_dpeek(abus,true);                                         
#endif
    else if(abus>=FOUR_MEGS)
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          
    else
      m68k_src_w=(WORD)0xffff;                                          
  }
  else if(abus>=MEM_START_OF_USER_AREA)
  {                                              
    DEBUG_CHECK_READ_W(abus);  
    m68k_src_w=DPEEK(abus);                  
  }
  else if(SUPERFLAG)
  {     
    DEBUG_CHECK_READ_W(abus);  
    m68k_src_w=DPEEK(abus);                  
  }
  else 
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}
#define m68k_READ_W_FROM_ADDR M68000.m68kReadWFromAddr();


inline void TM68000::m68kReadLFromAddr() {
  // Replacing macro m68k_READ_L_FROM_ADDR.
  abus&=0xffffff;                                   
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    
  else if(abus>=himem)
  {                                  
    if(abus>=MEM_IO_BASE)
    {           
      if(SUPERFLAG)
        m68k_src_l=io_read_l(abus);          
      else
        exception(BOMBS_BUS_ERROR,EA_READ,abus);         
    }
    else if(abus>=0xfc0000)
    {                             
      if(tos_high && abus<(0xfc0000+192*1024-2)) 
        m68k_src_l=ROM_LPEEK(abus-rom_addr);   
      else if(abus<0xfe0000 || abus>=0xfe2000)
        exception(BOMBS_BUS_ERROR,EA_READ,abus);  
    }
    else if(abus>=MEM_EXPANSION_CARTRIDGE)
    {           
      if(cart)
        m68k_src_l=CART_LPEEK(abus-MEM_EXPANSION_CARTRIDGE);  
      else
        m68k_src_l=0xffffffff;                                    
    }
    else if(abus>=rom_addr)
    {                         
      if(abus<(0xe00000+256*1024-2)) 
        m68k_src_l=ROM_LPEEK(abus-rom_addr);   
      else if(abus>=0xec0000)
        exception(BOMBS_BUS_ERROR,EA_READ,abus);          
      else
        m68k_src_l=0xffffffff;                                          
    }
    else if(abus>=0xd00000 && abus<0xd80000-2)
      m68k_src_l=0xffffffff;                                          
#if !defined(SS_MMU_NO_CONFUSION)
    else if (mmu_confused)
      m68k_src_l=mmu_confused_lpeek(abus,true);                                         
#endif
    else if(abus>=FOUR_MEGS)
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          
    else
      m68k_src_l=0xffffffff;                                          
  }
  else if(abus>=MEM_START_OF_USER_AREA)
  {                                              
    DEBUG_CHECK_READ_L(abus);  
    m68k_src_l=LPEEK(abus);                  
  }
  else if(SUPERFLAG)
  {     
    DEBUG_CHECK_READ_L(abus);  
    m68k_src_l=LPEEK(abus);                  
  }
  else
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}
#define m68k_READ_L_FROM_ADDR M68000.m68kReadLFromAddr();                   


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

NOT_DEBUG(inline) void m68k_poke_abus(BYTE x){
  abus&=0xffffff; // annoying that we must do this
  BOOL super=SUPERFLAG;
  if(abus>=MEM_IO_BASE && super)
    io_write_b(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
    DEBUG_CHECK_WRITE_B(abus);
    PEEK(abus)=x;
  }
  else 
    m68k_poke_abus2(x);
}
 

NOT_DEBUG(inline) void m68k_dpoke_abus(WORD x){
  abus&=0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1) 
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE && super)
      io_write_w(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
    DEBUG_CHECK_WRITE_W(abus);
    DPEEK(abus)=x;
  }
  else
    m68k_dpoke_abus2(x);
}


NOT_DEBUG(inline) void m68k_lpoke_abus(LONG x){
  ////m68k_lpoke_abus2(x); return; ////dbg    
  abus&=0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else 
  if(abus>=MEM_IO_BASE && super)
    io_write_l(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
    DEBUG_CHECK_WRITE_L(abus);
    LPEEK(abus)=x;
  }
  else
    m68k_lpoke_abus2(x);
}


// it's +- a copy of the above because of VC's inlining rules
// now those ones are not inlined, but in Stemdos (hd emu) so it's a good thing
inline void m68k_poke(MEM_ADDRESS ad,BYTE x){
  ASSERT(ad);
  abus=ad&0xffffff;
  BOOL super=SUPERFLAG;
  if(abus>=MEM_IO_BASE && super)
    io_write_b(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
    DEBUG_CHECK_WRITE_B(abus);
    PEEK(abus)=x;
  }
  else 
    m68k_poke_abus2(x);
}


inline void m68k_dpoke(MEM_ADDRESS ad,WORD x){
  ASSERT(ad);
  abus=ad&0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1) 
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE && super)
      io_write_w(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
    DEBUG_CHECK_WRITE_W(abus);
    DPEEK(abus)=x;
  }
  else
    m68k_dpoke_abus2(x);
}


inline void m68k_lpoke(MEM_ADDRESS ad,LONG x){
  ASSERT(ad);
  abus=ad&0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else 
  if(abus>=MEM_IO_BASE && super)
    io_write_l(abus,x);
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
  {
    DEBUG_CHECK_WRITE_L(abus);
    LPEEK(abus)=x;
  }
  else
    m68k_lpoke_abus2(x);
}

#endif//poke



#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
// this is a silly debug-only function, very useful at some point, we keep
// it just in case...
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