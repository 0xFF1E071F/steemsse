// Steem Steven Seagal Edition
// SSECpu.h

#pragma once
#ifndef SSECPU_H
#define SSECPU_H

#if defined(SS_CPU)

// forward
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


// CPU inlined macros
extern inline void FetchWord(WORD &dest_word); 
//////////////////////extern inline void m68k_Process();
extern inline void m68k_perform_rte();
extern inline void FetchTiming();
extern inline void m68kReadBFromAddr();
extern inline void m68kReadWFromAddr();
extern inline void m68kReadLFromAddr();

#if defined(SS_CPU_POKE)
void m68k_poke_abus2(BYTE);
void m68k_dpoke_abus2(WORD);
void m68k_lpoke_abus2(LONG);
#endif

extern inline void m68kSetDestB(unsigned long addr);
extern inline void m68kSetDestW(unsigned long addr);
extern inline void m68kSetDestL(unsigned long addr);

#if defined(SS_CPU_PREFETCH)
inline void PrefetchIrc();
inline void RefetchIr();
#endif

struct TCpu {
  // functions
  TCpu();
#if defined(SS_CPU_PREFETCH)
  WORD FetchForCall(MEM_ADDRESS ad);
#endif
  // data
#if defined(SS_CPU_PREFETCH)
  BOOL CallPrefetch; 
  WORD *PrefetchAddress; 
  int PrefetchClass;
  WORD PrefetchedOpcode;
#endif
#if defined(SS_DEBUG)
  int nExceptions;
  int nInstr;
  WORD PreviousIr;
  bool NextIrFetched; // fetched next instruction?
#endif
};
extern TCpu Cpu;


inline void FetchTiming() {
#if !defined(DEBUG_BUILD)
  ASSERT(!Cpu.NextIrFetched);
#endif
#if defined(SS_DEBUG)
  Cpu.NextIrFetched=true;
#endif
  INSTRUCTION_TIME(4); 
  cpu_cycles&=-4;
}
#define FETCH_TIMING FetchTiming();


inline void FetchWord(WORD &dest_word) {
  // Replacingmacro FETCH_W(dest_word)
  dest_word=prefetch_buf[0];
  if(prefetched_2)
  {
    if(prefetch_buf[1]!=*lpfetch)
    {
/*  The "prefetch trick" was used by some demos to crash when the CPU
    was in 'trace' mode (nothing to do with our debug TRACE!).
    eg Anomaly, Transbeauce 1, Transbeauce 2
*/
#if defined(SS_CPU_PREFETCH_TRACE)
        TRACE("Prefetched IRC:%X current:%X\n",prefetch_buf[1],*lpfetch);
#endif
//      prefetch_buf[1]=*lpfetch; // enabling this cancels prefetch (don't)
    }
    prefetch_buf[0]=prefetch_buf[1];
    prefetched_2=false;
  }
  else
    prefetch_buf[0]=*lpfetch; // next instr
  lpfetch+=MEM_DIR; // advance the fetch pointer
  if(lpfetch MEM_GE lpfetch_bound) // <=
    exception(BOMBS_BUS_ERROR,EA_FETCH,pc);
}
#define FETCH_W(dest_word) FetchWord(dest_word);


inline void m68k_perform_rte() {
  // replacing macro M68K_PERFORM_RTE(checkints)
  MEM_ADDRESS pushed_return_address=m68k_lpeek(r[15]+2);
  // An Illegal routine could manipulate this value.
  set_pc(pushed_return_address);        
  sr=m68k_dpeek(r[15]);r[15]+=6;      
  sr&=SR_VALID_BITMASK;               
  DETECT_CHANGE_TO_USER_MODE;         
  DETECT_TRACE_BIT;                                           
}


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


inline void m68k_Process() {
  // Replacing macro m68k_PROCESS.
  LOG_CPU  
  old_pc=pc;  
#if defined(SS_DEBUG)
  Cpu.PreviousIr=ir;
  Cpu.nInstr++;
  Cpu.NextIrFetched=false;
  ///if(pc==0x45000)    TRACE("PC %X\n",pc);
#endif
#if defined(SS_CPU_PREFETCH)
  ASSERT(prefetched_2);
#if defined(SS_DEBUG)
  Cpu.PrefetchClass=0; // default - not used
#endif
#endif
  FetchWord(ir); // IR->IRD

//if(debug1) TRACE("%X\n",pc);
//if(pc==0x144c8) debug1=0;
//if(pc==0x1de20) TRACE("A3 %x\n",areg[3]);
///if(pc==0x061A) TRACE("PC %x\n",pc);

#if defined(SS_CPU_EXCEPTION) && defined(SS_CPU_EXCEPTION_TRACE_PC)//heavy!!!
#undef LOGSECTION
#define LOGSECTION LOGSECTION_CRASH
  if(Cpu.nExceptions>1) 
    TRACE_LOG("%X\n",pc);
#undef LOGSECTION
#endif


  pc+=2; 
//  ASSERT(ir!=0xD191); // dbg: break on opcode...
//  ASSERT(!mmu_testing);
#if defined(SS_IPF_CPU) 
  int cycles=cpu_cycles;
#endif
  m68k_high_nibble_jump_table[ir>>12](); // go to instruction...
#if defined(SS_IPF_CPU)
 if(Caps.Active)
  {
    int cpucycles=cycles-cpu_cycles;
    ASSERT( cpucycles>0 );
    CapsFdcEmulate(&WD1772,cpucycles);
  }
#endif
  HANDLE_IOACCESS(m68k_trace();); // keep as macro, wouldn't be inlined
  DEBUG_ONLY( debug_first_instruction=0 );
}


#if defined(SS_CPU_PREFETCH)

#define m68k_interrupt(ad) \
{\
  WORD _sr=sr;\
  if (!SUPERFLAG) change_to_supervisor_mode();\
  Cpu.PrefetchClass=2;\
  m68k_PUSH_L(PC32);\
  m68k_PUSH_W(_sr);\
  SET_PC(ad);\
  SR_CLEAR(SR_TRACE);\
  interrupt_depth++;\
/*  TRACE("%X %d\n",ad,interrupt_depth);*/\
}

#else

#define m68k_interrupt(ad) \
{\
  WORD _sr=sr;\
  if (!SUPERFLAG) change_to_supervisor_mode();\
  m68k_PUSH_L(PC32);\
  m68k_PUSH_W(_sr);\
  SET_PC(ad);\
  SR_CLEAR(SR_TRACE);\
  interrupt_depth++;\
}

#endif


// They're not much called
inline void m68kReadBFromAddr() {
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
    else if(mmu_testing)
      m68k_src_b=mmu_testing_peek(abus,true);                                         
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
#define m68k_READ_B_FROM_ADDR m68kReadBFromAddr();        


inline void m68kReadWFromAddr() {
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
    else if(mmu_testing)
      m68k_src_w=mmu_testing_dpeek(abus,true);                                         
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
#define m68k_READ_W_FROM_ADDR m68kReadWFromAddr();


inline void m68kReadLFromAddr() {
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
    else if (mmu_testing)
      m68k_src_l=mmu_testing_lpeek(abus,true);                                         
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
#define m68k_READ_L_FROM_ADDR m68kReadLFromAddr();                   
/**/

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



//////////////
// Prefetch //
//////////////
/*  We enforce ijor's rules (http://pasti.fxatari.com/68kdocs/68kPrefetch.html)
    while staying in the Steem system to avoid duplication (we use
    prefetched_2 etc.)
    This is directly done in cpu.cpp by adding macros PREFETCH_IRC etc.
    Note that prefetch was good enough in Steem 3.2 to pass known cases 
    (some demos like Anomaly, Transbeauce 2 - which needed other fixes)
    and that this improvement (maybe) fixes nothing AFAIK.

    The FETCH_TIMING macros don't seem well placed in general, but it all 
    holds together.
*/

#if defined(SS_CPU_PREFETCH)

inline void PrefetchIrc() {
#if !(defined(_DEBUG) && defined(DEBUG_BUILD))
  ASSERT(!prefetched_2); // only once per instruction
#endif
//  if(prefetched_2) TRACE("prefetched_2 ir %X\n",ir);
//  if(!prefetched_2)
  {
    prefetch_buf[1]=*lpfetch;
    prefetched_2= TRUE;
  }
}
#define EXTRA_PREFETCH
#define PREFETCH_IRC PrefetchIrc();


inline void RefetchIr() {
  ASSERT(prefetch_buf[0]==*(lpfetch+1)); // detect cases
  prefetch_buf[0]=*(lpfetch-MEM_DIR);
}
#define REFETCH_IR RefetchIr();
#else
#define PREFETCH_IRC
#define REFETCH_IR

#endif

#endif//#if defined(SS_CPU)

#endif //SSECPU_H