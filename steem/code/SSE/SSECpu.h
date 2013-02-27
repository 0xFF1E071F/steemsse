
#pragma once
#ifndef SSECPU_H
#define SSECPU_H

#if defined(SS_CPU)

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



#if defined(SS_CPU_POKE)
void m68k_poke_abus2(BYTE);
void m68k_dpoke_abus2(WORD);
void m68k_lpoke_abus2(LONG);
#endif

/*  We now have a nice object representing the Motorola 68000, with some 
    functions, but the instructions stay global C
*/
struct TM68000 {
  TM68000();
  inline void FetchTiming();
  inline void FetchTimingNoRound();
  inline void FetchWord(WORD &dest_word); 
  inline void InstructionTime(int n);
  inline void InstructionTimeRound(int n);
  inline void Interrupt(MEM_ADDRESS ad);
  inline void PerformRte();
  inline void PrefetchSetPC();
  inline void Process();
  inline void m68kReadBFromAddr();
  inline void m68kReadWFromAddr();
  inline void m68kReadLFromAddr();
  inline void SetPC(MEM_ADDRESS ad);
  inline void Unstop();
#if defined(SS_DEBUG)
  int IrAddress; // pc at start
  int nExceptions;
  int nInstr;
  WORD PreviousIr;
  bool NextIrFetched; // fetched next instruction?
#if defined(SS_CPU_ROUNDING_CHECKS)
  int CycleSubstractions; // for rounding (not used...)
#endif
#endif//debug
#if defined(SS_CPU_PREFETCH)
  BOOL CallPrefetch; 
  WORD *PrefetchAddress; 
  int PrefetchClass; // see ijor's article
  WORD PrefetchedOpcode;
  WORD FetchForCall(MEM_ADDRESS ad);
  inline void PrefetchIrc();
  inline void PrefetchIrcNoRound();
  inline void RefetchIr();
#endif
#if defined(SS_CPU_ROUNDING_CHECKS)
  bool Rounding; // timing has been rounded
  bool AlignedCycles; // current counter is multiple of 4
  int CurrentRoundedCycles;
  int CurrentUnroundedCycles;
  inline void CheckExtraRoundedCycles();
#endif
};

extern TM68000 M68000;


// inline functions of TM68000 (instead of macros)


#if defined(SS_CPU_ROUNDING_CHECKS)

/*  

    ADD.L (EA),register
    depends on EA
    
    -(An) : 10 + 6 = 16 (not 20) (Cernit #2)
    first it --, then it reads (4+4) and it's ready

    D8(An,Dn): 14 + 6 rounded to 24 (not 20) (DSOS menu)
    it reads D8 (4), then it must compute address (2)
    then it fetches data (4+4)
    2 cycles are lost between the reads
    Note: we use a more general fix (SS_CPU_ROUNDING_CHECKS undefined)
*/

void TM68000::CheckExtraRoundedCycles() {
  if(CurrentRoundedCycles-CurrentUnroundedCycles==4
        && (ir & BITS_543)==BITS_543_100) // -(An)
  { 
    cpu_cycles+=(4); 
#if defined(SS_DEBUG)
    CycleSubstractions++;
#endif
  }
}
#define CHECK_EXTRA_ROUNDED_CYCLES M68000.CheckExtraRoundedCycles()


#endif//SS_CPU_ROUNDING_CHECKS


inline void TM68000::FetchTiming() {
#if !defined(SS_CPU_PREFETCH_TIMING)
  ASSERT(!NextIrFetched); //strong
#if defined(DEBUG_BUILD)
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
  ASSERT(!NextIrFetched);
#if defined(SS_DEBUG)
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
#define FETCH_W(dest_word) M68000.FetchWord(dest_word);


inline void TM68000::InstructionTime(int t) {
  cpu_cycles-=(t);
#if defined(SS_CPU_ROUNDING_CHECKS)
  CurrentUnroundedCycles+=t; 
  CurrentRoundedCycles+=(t+3)&~3;
#endif
}
#define INSTRUCTION_TIME(t)  M68000.InstructionTime(t)


inline void TM68000::InstructionTimeRound(int t) {
  InstructionTime(t);
  cpu_cycles&=-4;
}
#define INSTRUCTION_TIME_ROUND(t) M68000.InstructionTimeRound(t)


inline void TM68000::Interrupt(MEM_ADDRESS ad) {
  WORD _sr=sr;
  if (!SUPERFLAG) 
    change_to_supervisor_mode();
#if defined(SS_CPU_PREFETCH)
  PrefetchClass=2;
#endif
  m68k_PUSH_L(PC32);
  m68k_PUSH_W(_sr);
  SetPC(ad);//SET_PC(ad);
  SR_CLEAR(SR_TRACE);
  interrupt_depth++;
//  TRACE("%X %d\n",ad,interrupt_depth);
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
#if !(defined(_DEBUG) && defined(DEBUG_BUILD))
  ASSERT(!prefetched_2); // strong, only once per instruction 
#endif
  prefetch_buf[1]=*lpfetch;
  prefetched_2=TRUE;

#if defined(SS_CPU_PREFETCH_TIMING)
  ASSERT(!NextIrFetched); //strong
#if defined(DEBUG_BUILD)
  NextIrFetched=true;
#endif
#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(ir))
    ; else
#endif
  {
    InstructionTimeRound(4);
  }
#endif
}


inline void TM68000::PrefetchIrcNoRound() {
#if !(defined(_DEBUG) && defined(DEBUG_BUILD))
  ASSERT(!prefetched_2); // strong, only once per instruction 
#endif
  prefetch_buf[1]=*lpfetch;
  prefetched_2=TRUE;

#if defined(SS_CPU_PREFETCH_TIMING)
#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(ir))
    ; else
#endif
  {
    InstructionTime(4);
  }
#endif
}


#define EXTRA_PREFETCH //all prefetches actually are "extra"!
#define PREFETCH_IRC  M68000.PrefetchIrc()
#define PREFETCH_IRC_NO_ROUND  M68000.PrefetchIrcNoRound()


#else

#define PREFETCH_IRC
#define PREFETCH_IRC_NO_ROUND

#endif

inline void TM68000::PrefetchSetPC() {
  // called by SetPC; we don't count timing here
  prefetch_buf[0]=*lpfetch;
  prefetch_buf[1]=*(lpfetch+MEM_DIR); 
  prefetched_2=true; // was false in Steem 3.2
#if defined(SS_CPU_PREFETCH)
  if(M68000.CallPrefetch) 
  {
    ASSERT(M68000.PrefetchedOpcode==prefetch_buf[0]);
    prefetch_buf[0]=M68000.PrefetchedOpcode;
    M68000.CallPrefetch=FALSE;
  }
#endif
  lpfetch+=MEM_DIR;
}
#define PREFETCH_SET_PC M68000.PrefetchSetPC();


inline void TM68000::Process() {
// core function, dispatches in cpu emu
  LOG_CPU  
  
#if defined(SS_DEBUG)
  IrAddress=pc;
  PreviousIr=ir;
  nInstr++;
#if defined(SS_CPU_FETCH_TIMING) && defined(DEBUG_BUILD) \
  && defined(SS_DEBUG_TRACE_PREFETCH)
  // 4e72 = STOP, where no fetching occurs
  // 4afc = ILLEGAL
//  ASSERT( NextIrFetched || ir==0x4e72 || ir==0x4afc); // strong
  if(!NextIrFetched && ir!=0x4e72)
  {
    EasyStr instr=disa_d2(old_pc);
    TRACE("%x %x %s no FETCH_TIMING\n",old_pc,PreviousIr,instr.c_str());
  }
#endif
//  NextIrFetched=false; // in FETCH_TIMING or...
#endif//debug

  old_pc=pc;  

#if defined(SS_CPU_PREFETCH)
  ASSERT(prefetched_2);
  PrefetchClass=0; // default, most instructions
#endif

  FetchWord(ir); // IR->IRD
  
//if(debug1) TRACE("%X\n",pc);
//if(pc==0x144c8) debug1=0;
//if(pc==0x1de20) TRACE("A3 %x\n",areg[3]);
///if(pc==0x061A) TRACE("PC %x\n",pc);

#if defined(SS_CPU_EXCEPTION) && defined(SS_CPU_EXCEPTION_TRACE_PC)//heavy!!!
  if(nExceptions>1) 
    TRACE_LOG("%X\n",pc);
#endif

#if defined(SS_CPU_ROUNDING_CHECKS)
  Rounding=false;
  AlignedCycles=true; //? for pairing in the future
  CurrentRoundedCycles=0;
  CurrentUnroundedCycles=0;
#if defined(SS_DEBUG)
  CycleSubstractions=0;
#endif
#endif

  pc+=2; 
#if defined(SS_IPF_CPU) || defined(SS_DEBUG)
  int cycles=cpu_cycles;
#endif
  //ASSERT(ir!=0x91AE); // dbg: break on opcode
  m68k_high_nibble_jump_table[ir>>12](); // go to instruction...
#if defined(SS_IPF_CPU)
 if(Caps.Active)
  {
    int cpucycles=cycles-cpu_cycles;
    ASSERT( cpucycles>0 );
    CapsFdcEmulate(&WD1772,cpucycles);
  }
#endif



#if defined(SS_DEBUG)
 NextIrFetched=false; // in FETCH_TIMING or...// check_interrut
#if defined(SS_CPU_ROUNDING_CHECKS)
  // for the moment, debug only
  ASSERT( CurrentRoundedCycles>= CurrentUnroundedCycles );
#endif
#if defined(DEBUG_BUILD) && defined(SS_DEBUG_TRACE_CPU_ROUNDING) \
  && defined(SS_CPU_ROUNDING_CHECKS)
  if(CurrentRoundedCycles-CurrentUnroundedCycles>=4
    && !CycleSubstractions)
  {
    EasyStr instr=disa_d2(IrAddress);
    TRACE("%x %x %s cycles %d rounded %d\n",IrAddress,ir,instr.c_str(),CurrentUnroundedCycles,CurrentRoundedCycles);
  }
#endif
  if(0
  //||ir==0x48F8 // MovemR->M
  //||ir==0x4cd2 //movem M->R
  //||ir==0xd5e4 // 20 vs 16 //adda.l    -(a4),a2  th: 10 + 6 = 16 STM probably rounded to 12+6=18 rounded = 20
  //||ir==0xd5db //adda.l    (a3)+,a2  16vs 14
  //||ir==0xd093   // add.l (a3),d0  16 vs 14
    )
  {
//    TRACE("ir %x cycles %d\n",ir,cycles-cpu_cycles);
  }
#endif//debug
  HANDLE_IOACCESS(m68k_trace();); // keep as macro, wouldn't be inlined
  DEBUG_ONLY( debug_first_instruction=0 );
}
#define m68k_PROCESS M68000.Process();


#if defined(SS_CPU_PREFETCH)

inline void TM68000::RefetchIr() {
  ASSERT(prefetch_buf[0]==*(lpfetch+1)); // detect cases (none yet!)
  prefetch_buf[0]=*(lpfetch-MEM_DIR);
}
#define REFETCH_IR  M68000.RefetchIr();
#else
#define REFETCH_IR

#endif


inline void TM68000::SetPC(MEM_ADDRESS ad) {
    pc=ad;                               
    pc_high_byte=pc & 0xff000000;     
    pc&=0xffffff;                    
    lpfetch=lpDPEEK(0); //Default to instant bus error when fetch
    lpfetch_bound=lpDPEEK(0);         

    if (pc>=himem){                                                       
      if (pc<MEM_IO_BASE){           
        if (pc>=MEM_EXPANSION_CARTRIDGE){                                
          if (pc>=0xfc0000){                                                   
            if (tos_high && pc<(0xfc0000+192*1024)){         
              lpfetch=lpROM_DPEEK(pc-0xfc0000); 
              lpfetch_bound=lpROM_DPEEK(192*1024);         
            }                                                
          }else if (cart){                    
            lpfetch=lpCART_DPEEK(pc-MEM_EXPANSION_CARTRIDGE);
            lpfetch_bound=lpCART_DPEEK(128*1024);       
          }                   
        }else if(pc>=rom_addr){            
          if (pc<(0xe00000 + 256*1024)){   
            lpfetch=lpROM_DPEEK(pc-0xe00000);
            lpfetch_bound=lpROM_DPEEK(256*1024);   
          }              
        }              
      }else{   
        if (pc>=0xff8240 && pc<0xff8260){      
          lpfetch=lpPAL_DPEEK(pc-0xff8240); 
          lpfetch_bound=lpPAL_DPEEK(64+PAL_EXTRA_BYTES);     
        }                        
      }                                   
    }else{             
      lpfetch=lpDPEEK(pc); 
      lpfetch_bound=lpDPEEK(mem_len+(MEM_EXTRA_BYTES/2));  
    }                            
    PrefetchSetPC();//PREFETCH_SET_PC

}
#define SET_PC(ad) M68000.SetPC(ad);


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



#endif//#if defined(SS_CPU)

#endif //SSECPU_H