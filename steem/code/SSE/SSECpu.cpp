#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE)
#pragma message("Included for compilation: SSECpu.cpp")
#endif

#if defined(SS_CPU)

TM68000 M68000; // singleton

TM68000::TM68000() {
#if defined(SS_DEBUG)    
  nExceptions=nInstr=0;
  NextIrFetched=true;
#endif
#if defined(SS_CPU_PREFETCH_CALL)
  CallPrefetch=FALSE;
#endif
#if defined(SS_CPU_ROUNDING_CHECKS)
  Rounding=false; 
#endif
}


////////////////
// Exceptions //
////////////////

#if defined(SS_CPU_EXCEPTION)

#define LOGSECTION LOGSECTION_CRASH//SS

void m68k_exception::crash()
{
  DWORD bytes_to_stack=int((bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR) ? (4+2+2+4+2):(4+2));
  MEM_ADDRESS sp=(MEM_ADDRESS)(SUPERFLAG ? (areg[7] & 0xffffff):(other_sp & 0xffffff));

#if defined(SS_DEBUG)   
  if(M68000.nExceptions!=-1)
  {
    M68000.nExceptions++;  
    TRACE_LOG("\nException #%d, %d bombs (",M68000.nExceptions,bombs);
    switch(bombs)
    {  
    case 2:
      TRACE_LOG("BOMBS_BUS_ERROR"); 
      break;
    case 3:
      TRACE_LOG("BOMBS_ADDRESS_ERROR"); 
      break;
    case 4:
      TRACE_LOG("BOMBS_ILLEGAL_INSTRUCTION"); 
      break;
    case 5:
      TRACE_LOG("BOMBS_DIVISION_BY_ZERO"); 
      break;
    case 6:
      TRACE_LOG("BOMBS_CHK"); 
      break;
    case 7:
      TRACE_LOG("BOMBS_TRAPV"); 
      break;
    case 8:
      TRACE_LOG("BOMBS_PRIVILEGE_VIOLATION"); 
      break;
    case 9:
      TRACE_LOG("BOMBS_TRACE_EXCEPTION"); 
      break;
    case 10:
      TRACE_LOG("BOMBS_LINE_A"); 
      break;
    case 11:
      TRACE_LOG("BOMBS_LINE_F"); 
      break;
    }//sw
    TRACE_LOG(") at %d\n",ABSOLUTE_CPU_TIME);

#ifdef DEBUG_BUILD 
    EasyStr instr=disa_d2(old_pc); // take advantage of the disassembler
    TRACE_LOG("PC=%X-IR=%X-Ins: %s -SR=%X-Bus=%X",old_pc,ir,instr.Text,sr,abus);
#else
    TRACE_LOG("PC=%X-IR=%X-SR=%X-Bus=%X",old_pc,ir,sr,abus);
#endif
    TRACE_LOG("-Vector $%X=%X\n",bombs*4,LPEEK(bombs*4));
    // dump registers
    for(int i=0;i<8;i++) // D0-D7
      TRACE_LOG("D%d=%X ",i,r[i]);
    TRACE_LOG("\n");
    for(int i=0;i<8;i++) // A0-A7 (A7 when the exception occured)
      TRACE_LOG("A%d=%X ",i,areg[i]);
    TRACE_LOG("\n");
  }
#endif

  if (sp<bytes_to_stack || sp>FOUR_MEGS)
  {
    // Double bus error, CPU halt (we crash and burn)
    // This only has to be done here, m68k_PUSH_ will cause bus error if invalid
    DEBUG_ONLY( log_history(bombs,crash_address) );
    TRACE_LOG("Double bus error SP:%d < bytes to stack %d\n",sp,bytes_to_stack);
    perform_crash_and_burn();
#if defined(SS_DEBUG)
    M68000.nExceptions=-1;
#endif
  }
  else
  {
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    if(bombs==BOMBS_ILLEGAL_INSTRUCTION || bombs==BOMBS_PRIVILEGE_VIOLATION)
    {
      if(!SUPERFLAG) 
        change_to_supervisor_mode();
      TRACE_LOG("Push crash address %X on %X\n",(crash_address & 0x00ffffff) | pc_high_byte,r[15]-4);
      m68k_PUSH_L((crash_address & 0x00ffffff) | pc_high_byte); 
      INSTRUCTION_TIME_ROUND(8);
      TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
      m68k_PUSH_W(_sr); // Status register 
      INSTRUCTION_TIME_ROUND(4); //Round first for interrupts
      MEM_ADDRESS ad=LPEEK(bombs*4); // Get the vector
      if(ad & 1) // bad vector!
      {
        // Very rare, generally indicates emulation bug
        bombs=BOMBS_ADDRESS_ERROR;
#if defined(SS_DEBUG)
        BRK(odd exception vector);
        M68000.nExceptions++;
        TRACE_LOG("->%d bombs\n",bombs);
#endif
        address=ad;
        action=EA_FETCH;
      }
      else
      {
        TRACE_LOG("PC = %X\n\n",ad);
        set_pc(ad);
        SR_CLEAR(SR_TRACE);
        INSTRUCTION_TIME_ROUND(22); 
        interrupt_depth++; // Is this necessary?
      }
    }
    if(bombs==BOMBS_BUS_ERROR||bombs==BOMBS_ADDRESS_ERROR)
    {
      if(!SUPERFLAG) 
        change_to_supervisor_mode();
      TRY_M68K_EXCEPTION
      {

#if defined(SS_CPU_WAR_HELI2)
/*  New fix for War Heli, based on when exactly in the MOVE process the crash
    occured.
    Apparently when cpu crashes while writing, the last prefetch occurs right
    before the exception is handled, or that's what we suppose for this hack.
    If it's true, it's weird.
    In fact it seems wrong TODO
*/        
#if defined(SS_DEBUG)
        if(((_ir & 0xF000)>>12) <3 // MOVE opcode 00, 01, 02
          && M68000.PrefetchClass>0
          && !prefetched_2
          && action==EA_WRITE
          && (ir&BITS_876)!=BITS_876_101 // (d16, An) Syntax Terror, move.b
          && (ir&BITS_876)!=BITS_876_110 // (d8, An, Xn) pc shouldn't have +2?
          )
#else
        if(_ir==0x2285) // hack of 3.3
#endif
        {
//          ASSERT((_ir & 0xf000)==(b00100000 << 8)  ); //MOVE.L //tmp disable-froggies
          ASSERT(M68000.PrefetchClass==1);
          PREFETCH_IRC;
          _pc+=2;
          TRACE_LOG("Crash during write PC%X IR%X +2 for prefetch\n",_pc,_ir);
        }

#endif

        // MOVE.L ad hoc hacks
        if((_ir & 0xf000)==(b00100000 << 8))
        {
          int offset=0;
          switch(_ir) // opcode
          {
#if defined(SS_CPU_WAR_HELI)
          case 0x2285: 
            TRACE_LOG("War Heli? (use ADAT)\n");
            offset=2; // fixes War Heli, already in Steem 3.2.
            break;
#endif
#if defined(SS_CPU_PHALEON)
          case 0x21F8:
            TRACE_LOG("Protected demo (European, Phaleon, Transbeauce 2?\n");
            offset=-2; // fixes those protected demos
            break;
#endif
          case 0x2ABB: // Lethal Xcess (STF)
            break;
          case 0x2235: 
            TRACE_LOG("Super Neo Show demo?\n"); // strange beast
            break;
          case 0x20BC:
          case 0x2089:
            TRACE_LOG("Cuddly demo?\n");
            break;
          default: // generally an emu fault
            TRACE_LOG("MOVE.L crash - Opcode %X\n",_ir);
            break;
          }//sw
          if(offset)
            TRACE_LOG("Adjusting stacked PC %d: %X\n",offset,_pc+offset);
          _pc+=offset;
        }

        TRACE_LOG("Push PC %X on %X\n",_pc,r[15]-4);
        m68k_PUSH_L(_pc);
        TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
        m68k_PUSH_W(_sr);
        TRACE_LOG("Push IR %X on %X\n",_ir,r[15]-2);
        m68k_PUSH_W(_ir);
        TRACE_LOG("Push address %X on %X\n",address,r[15]-4);
        m68k_PUSH_L(address);
        // status
        WORD x=WORD(_ir & 0xffe0); 
        if(action!=EA_WRITE) x|=B6_010000;
        if(action==EA_FETCH)
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_PROGRAM:FC_USER_PROGRAM);
        else
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_DATA:FC_USER_DATA);
        TRACE_LOG("Push status %X on %X\n",x,r[15]-2);
        m68k_PUSH_W(x);
      }
      CATCH_M68K_EXCEPTION
      {
        TRACE_LOG("Exception during exception...\n");
        r[15]=0xf000; // R15=A7
      }
      END_M68K_EXCEPTION
      TRACE_LOG("PC = %X\n\n",LPEEK(bombs*4));
      SET_PC(LPEEK(bombs*4)); // includes final prefetch
      SR_CLEAR(SR_TRACE);
      INSTRUCTION_TIME_ROUND(50); //Round for fetch
#if defined(SS_CPU_PREFETCH) && defined(SS_DEBUG)
      M68000.PrefetchClass=2;
#endif
    }
///    DEBUG_ONLY(log_history(bombs,crash_address));
  }
  PeekEvent(); // Stop exception freeze
}
#undef LOGSECTION
#endif//#if defined(SS_CPU_EXCEPTION)


//////////////
// Prefetch //
//////////////

#if defined(SS_CPU_PREFETCH)

WORD TM68000::FetchForCall(MEM_ADDRESS ad) {
  // fetch PC before pushing return address in JSR, fixes nothing
  ad&=0xffffff; 
  PrefetchAddress=lpDPEEK(0); // default
  if(ad>=himem)
  {
    if(ad<MEM_IO_BASE)
    {           
      if(ad>=MEM_EXPANSION_CARTRIDGE)
      {                                
        if(ad>=0xfc0000)
        {                                                   
          if(tos_high && ad<(0xfc0000+192*1024)) 
            PrefetchAddress=lpROM_DPEEK(ad-0xfc0000); 
        }
        else if(cart)
          PrefetchAddress=lpCART_DPEEK(ad-MEM_EXPANSION_CARTRIDGE); 
      }
      else if(ad>=rom_addr)
      {                                                      
        if(ad<(0xe00000 + 256*1024))
          PrefetchAddress=lpROM_DPEEK(ad-0xe00000); 
      }                            
    }
    else
    {   
      if(ad>=0xff8240 && ad<0xff8260)      
      {
        BRK(set PC to palette!);// impossible?
        PrefetchAddress=lpPAL_DPEEK(ad-0xff8240);
      }
    }                                                                           
  }
  else //  in normal ram
    PrefetchAddress=lpDPEEK(ad);
  prefetched_2=false; // will have prefetched 1 word / cancels eventual prefetch
  PrefetchedOpcode=*(PrefetchAddress); // fetch instruction!
  CallPrefetch=TRUE;
  return PrefetchedOpcode;
}

#endif


/////////////////////////////////////////////
// Poke, non inlined part (full functions) //
/////////////////////////////////////////////
// TODO: only parts not treated already, but it's dangerous
#if defined(SS_CPU_POKE)

void m68k_poke_abus2(BYTE x){
  abus&=0xffffff;
  if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_b(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SS_MMU_NO_CONFUSION)
    if (mmu_confused){
      mmu_confused_set_dest_to_addr(1,true);
      m68k_DEST_B=x;
#if defined(SS_MMU_TRACE2)
      TRACE("MMU %X: poke %X=%X\n",old_pc,m68k_DEST_B,x);
#endif
    }else 
#endif      
    if (abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_B(abus);
    if (SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      PEEK(abus)=x;
    else if (abus>=MEM_START_OF_USER_AREA)
      PEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}


void m68k_dpoke_abus2(WORD x){
  abus&=0xffffff;
  if(abus&1) exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_w(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SS_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(2,true);
      m68k_DEST_W=x;
#if defined(SS_MMU_TRACE2)
      TRACE("MMU %X: dpoke %X=%X\n",old_pc,m68k_DEST_W,x);
#endif
    }else 
#endif
    if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_W(abus);
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      DPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      DPEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}


void m68k_lpoke_abus2(LONG x){
  abus&=0xffffff;
  if(abus&1)exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_l(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SS_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(4,true);
      m68k_DEST_L=x;
#if defined(SS_MMU_TRACE2)
      TRACE("MMU %X: lpoke %X=%X\n",old_pc,m68k_DEST_L,x);
#endif
    }else 
#endif
    if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_L(abus);
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      LPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      LPEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}

#endif


/////////////////////////
// Redone instructions //
/////////////////////////

#if defined(SS_CPU_CLR)

//#undef SS_CPU_LINE_4_TIMINGS


// CLR reads before writing but we leave timings unchanged (Decade Menu)
// It fixes nothing that I know, it was a 'TODO' from Steem authors

void                              m68k_clr_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif

  if (DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
  m68k_GET_DEST_B_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  int save=cpu_cycles;
  if (DEST_IS_REGISTER==0) 
    M68000.m68kReadBFromAddr();
  cpu_cycles=save;
  m68k_DEST_B=0;
  SR_CLEAR(SR_N+SR_V+SR_C);
  SR_SET(SR_Z);
}void                             m68k_clr_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif

  if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
  m68k_GET_DEST_W_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  int save=cpu_cycles;
  if (DEST_IS_REGISTER==0) 
    M68000.m68kReadWFromAddr();
  cpu_cycles=save;
  m68k_DEST_W=0;
  SR_CLEAR(SR_N+SR_V+SR_C);
  SR_SET(SR_Z);
}void                             m68k_clr_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif

  if(DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {INSTRUCTION_TIME(8);}
  m68k_GET_DEST_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  int save=cpu_cycles;
  if (DEST_IS_REGISTER==0) 
    M68000.m68kReadLFromAddr();
  cpu_cycles=save;
  m68k_DEST_L=0;
  SR_CLEAR(SR_N+SR_V+SR_C);
  SR_SET(SR_Z);
}

//#define SS_CPU_LINE_4_TIMINGS
#endif

#if defined(SS_CPU_DIV) 

// using ijor's timings (in 3rdparty\pasti), what they fix proves them correct
//#undef SS_CPU_LINE_8_TIMINGS
void                              m68k_divu(){
#if !(defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){ // div by 0
    // Clear V flag when dividing by zero. Fixes...?
    SR_CLEAR(SR_V);
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    unsigned long q;
    unsigned long dividend = (unsigned long) (r[PARAM_N]);
    unsigned short divisor = (unsigned short) m68k_src_w;
    int cycles_for_instr=getDivu68kCycles(dividend,divisor) -4; // -prefetch
    INSTRUCTION_TIME(cycles_for_instr); // fixes Pandemonium loader
    q=(unsigned long)((unsigned long)dividend)/(unsigned long)((unsigned short)divisor);
    if(q&0xffff0000){
      SR_SET(SR_V);
    }else{
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
      r[PARAM_N]=((((unsigned long)r[PARAM_N])%((unsigned short)m68k_src_w))<<16)+q;
    }
  }
}


void                              m68k_divs(){
#if !(defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){
    /* Clear V flag when dividing by zero - Alcatraz Odyssey demo depends
     * on this (actually, it's doing a DIVS).  */
    SR_CLEAR(SR_V);
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SS_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    signed long q;
    signed long dividend = (signed long) (r[PARAM_N]);
    signed short divisor = (signed short) m68k_src_w;
    int cycles_for_instr=getDivs68kCycles(dividend,divisor)-4; // -prefetch
    INSTRUCTION_TIME(cycles_for_instr);   // fixes Dragonnels loader
    q=(signed long)((signed long)dividend)/(signed long)((signed short)divisor);
    if(q<-32768 || q>32767){
      SR_SET(SR_V);
    }else{
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
      r[PARAM_N]=((((signed long)r[PARAM_N])%((signed short)m68k_src_w))<<16)|((long)LOWORD(q));
    }
  }
}
//#define SS_CPU_LINE_8_TIMINGS
#endif

/*  Move have each its switch (useful when debugging); various fixes.
    There's no FETCH_TIMING when moving to register, which isn't correct!
    But all Steem timings hold together.
    If we 'move' (lol) the first INSTRUCTION_TIME(4) after 'get_source',
    lots of programs are broken.
    We need to compensate it at the 'read SDP' function, with a filter on
    opcode. In most instructions, FETCH_TIMING comes at once, which isn't 
    correct. The general cycle is 'read', 'fetch', 'write if necessary'.
    If we fix Steem timings, we should do all at once, or use filters 
    (supposing we know where to apply them, like in 'read SDP').
    The -4 in INSTRUCTION_TIME() below are explained so: we start from
    official timing, we take 4 for the first INSTRUCTION_TIME(4), we
    take 4 for instruction prefetch.
*/

/*
MOVE instructions. Most variants, except the ones noted below

 

1)      Perform as many prefetch cycles as extension words are in the source 
operand (optional).

2)      Read source operand (optional if source is register).

3)      Perform as many prefetch cycles as extension words are in the 
destination operand (optional).

4)      Writes memory operand.

5)      Perform last prefetch cycle.

 

Net effect is that MOVE instructions are of class 1.

 

 

MOVE <ea>,-(An)

 

When the destination addressing mode is pre-decrement, steps 4 and 5 above are 
inverted. So it behaves like a read modify instruction and it is a class 0 
instruction.

 

Note: The behavior is the same disregarding transfer size (byte, word or long), 
and disregarding the source addressing mode.

 

 

MOVE memory,(xxx).L

 

When the destination addressing mode is long absolute and the source operand is 
any memory addr.mode, step 4 is interleaved in the middle of step 3. Step 3 
only performs a single prefetch in this case. The other prefetch cycle that is 
normally performed at that step is deferred after the write cycles.

 

So, two prefetch cycles are performed after the write ones. It is a class 2 
instruction.

 

Note: The behavior is the same disregarding transfer size (byte, word or long). 
But if the source operand is a data or address register, or immediate, then the 
behavior is the same as other MOVE variants (class 1 instruction).

*/

//todo from Hatari:
	/* [NP] genamode counts 2 cycles if dest is -(An), this is wrong. */
	/* For move dest (An), (An)+ and -(An) take the same time */
	/* (for other instr, dest -(An) really takes 2 cycles more) */
//-> already OK in Steem

#if defined(SS_CPU_MOVE_B)

void m68k_0001() {  // move.b

#if !defined(SS_CPU_LINE_1_TIMINGS)

#if defined(SS_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000)
    FETCH_TIMING_NO_ROUND; // it's the same, but recorded as fetch timing
  else
#endif
    INSTRUCTION_TIME(4);
#endif//!defined(SS_CPU_LINE_1_TIMINGS)

#if defined(SS_CPU_PREFETCH)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SS_CPU_POST_INC)
  BOOL PostIncrement=FALSE;
#endif

#if defined(SS_CPU_ASSERT_ILLEGAL)
  // We consider ILLEGAL before bus/address error
  // fixes Transbeauce 2 loader, Titan
  if( (ir&BITS_876)==BITS_876_001
    || (ir&BITS_876)==BITS_876_111
    && (ir&BITS_ba9)!=BITS_ba9_000
    && (ir&BITS_ba9)!=BITS_ba9_001 )
    m68k_unrecognised(); 
#endif

  // Source
  m68k_GET_SOURCE_B;

  // Destination
  if((ir&BITS_876)==BITS_876_000)
  { // Dn
#if defined(SS_CPU_LINE_1_TIMINGS) && !defined(SS_CPU_LINE_1_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_B=m68k_src_b; // move completed
    SR_CHECK_Z_AND_N_B; // update flags
#if defined(SS_CPU_LINE_1_TIMINGS) && defined(SS_CPU_LINE_1_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif
  }
#if !defined(SS_CPU_ASSERT_ILLEGAL)
  else if((ir&BITS_876)==BITS_876_001)
    m68k_unrecognised();
#endif
  else
  {   //to memory
    bool refetch=false;
    switch(ir&BITS_876)
    {
    case BITS_876_010: // (An)
#if defined(SS_CPU_LINE_1_TIMINGS)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011: // (An)+
#if defined(SS_CPU_LINE_1_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N];
#if defined(SS_CPU_POST_INC)
      PostIncrement=TRUE;
#else
      areg[PARAM_N]++; 
      if(PARAM_N==7)
        areg[7]++;
#endif
      break;
    case BITS_876_100: // -(An)
#if defined(SS_CPU_PREFETCH)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC;
      FETCH_TIMING;
#endif  
#if defined(SS_CPU_LINE_1_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      areg[PARAM_N]--;
      if(PARAM_N==7)
        areg[7]--;
      abus=areg[PARAM_N];
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(12-4-4);
#if defined(SS_CPU_LINE_1_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2;
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(14-4-4);
#if defined(SS_CPU_LINE_1_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      m68k_iriwo=m68k_fetchW();pc+=2;
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_876_111:
      if (SOURCE_IS_REGISTER_OR_IMMEDIATE==0) refetch=true;
      switch (ir & BITS_ba9){
      case BITS_ba9_000: // (xxx).W
        INSTRUCTION_TIME(12-4-4);
#if defined(SS_CPU_LINE_1_TIMINGS)
        INSTRUCTION_TIME(4);
#endif

        abus=0xffffff & (unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2;
        break;
      case BITS_ba9_001: // (xxx).L
#if defined(SS_CPU_PREFETCH)
        if(refetch)
          M68000.PrefetchClass=2; // only .L
#endif
        INSTRUCTION_TIME(16-4-4);
#if defined(SS_CPU_LINE_1_TIMINGS)
        INSTRUCTION_TIME(4);
#endif

        abus=m68k_fetchL() & 0xffffff;
        pc+=4;
        break;
#if !defined(SS_CPU_ASSERT_ILLEGAL)
      default:
        m68k_unrecognised();
#endif
      }
    }
    // Set flags
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if(!m68k_src_b){
      SR_SET(SR_Z);
    }
    if(m68k_src_b&MSB_B){
      SR_SET(SR_N);
    }
    m68k_poke_abus(m68k_src_b); // write; could crash

#if defined(SS_CPU_POST_INC)
    if(PostIncrement)
    {
      areg[PARAM_N]++; 
      if(PARAM_N==7)
        areg[7]++;
    }
#endif

#if defined(SS_CPU_PREFETCH)
    if(M68000.PrefetchClass==2)
    {
      REFETCH_IR;
      PREFETCH_IRC;
    }
    if(M68000.PrefetchClass) // -(An), already fetched
      FETCH_TIMING;
#else
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
    FETCH_TIMING;  
#endif  
  }// to memory
#if defined(SS_CPU_PREFETCH)
  if(M68000.PrefetchClass==1) // classes 0 & 2 already handled
  {
    if((ir&BITS_876)==BITS_876_000)
      PREFETCH_IRC_NO_ROUND;
    else
      PREFETCH_IRC;
  }
#endif
}

#endif

#if defined(SS_CPU_MOVE_L)

void m68k_0010()  //move.l
{

#if !defined(SS_CPU_LINE_2_TIMINGS)
#if defined(SS_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000
    ||(ir & BITS_876)==BITS_876_001)
    FETCH_TIMING_NO_ROUND;
  else
#endif
  INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SS_CPU_POST_INC)
  BOOL PostIncrement=FALSE;
#endif

#if defined(SS_CPU_ASSERT_ILLEGAL)
  if( (ir&BITS_876)==BITS_876_111
    && (ir&BITS_ba9)!=BITS_ba9_000
    && (ir&BITS_ba9)!=BITS_ba9_001 )
      m68k_unrecognised();
#endif

  m68k_GET_SOURCE_L; // where tb2 etc crash
  // Destination
  if ((ir & BITS_876)==BITS_876_000) // Dn
  { 
#if defined(SS_CPU_LINE_2_TIMINGS) && !defined(SS_CPU_LINE_2_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif

    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_L=m68k_src_l;
    SR_CHECK_Z_AND_N_L;
#if defined(SS_CPU_LINE_2_TIMINGS) && defined(SS_CPU_LINE_2_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif

  }
  else if ((ir & BITS_876)==BITS_876_001) // An
  {
#if defined(SS_CPU_LINE_2_TIMINGS) && !defined(SS_CPU_LINE_2_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif

    areg[PARAM_N]=m68k_src_l; // MOVEA
#if defined(SS_CPU_LINE_2_TIMINGS) && defined(SS_CPU_LINE_2_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif

  }
  else
  {   //to memory
    bool refetch=0;
    switch(ir&BITS_876)
    {
    case BITS_876_010: // (An)
      INSTRUCTION_TIME(12-4-4);
#if defined(SS_CPU_LINE_2_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N];
      break;
    case BITS_876_011: // (An)+
      INSTRUCTION_TIME(12-4-4);
#if defined(SS_CPU_LINE_2_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N];
#if defined(SS_CPU_POST_INC)
      PostIncrement=TRUE;
#else
      areg[PARAM_N]+=4;
#endif
      break;
    case BITS_876_100: // -(An)
      INSTRUCTION_TIME(12-4-4);
#if defined(SS_CPU_LINE_2_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC;
      FETCH_TIMING;
#endif  
      areg[PARAM_N]-=4;
      abus=areg[PARAM_N];
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(16-4-4);
#if defined(SS_CPU_LINE_2_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2;
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(18-4-4);
#if defined(SS_CPU_LINE_2_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      m68k_iriwo=m68k_fetchW();pc+=2;
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_876_111:
      if (SOURCE_IS_REGISTER_OR_IMMEDIATE==0) refetch=true;
      switch(ir&BITS_ba9){
      case BITS_ba9_000: // (xxx).W
        INSTRUCTION_TIME(16-4-4);
#if defined(SS_CPU_LINE_2_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2;
        break;
      case BITS_ba9_001: // (xxx).L
        INSTRUCTION_TIME(20-4-4);
#if defined(SS_CPU_LINE_2_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH)
        if(refetch) 
          M68000.PrefetchClass=2; 
#endif 
        abus=m68k_fetchL()&0xffffff;
        pc+=4;
        break;
#if !defined(SS_CPU_ASSERT_ILLEGAL)
      default:
        m68k_unrecognised();
#endif
      }
    }
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if(!m68k_src_l){
      SR_SET(SR_Z);
    }
    if(m68k_src_l&MSB_L){
      SR_SET(SR_N);
    }
    m68k_lpoke_abus(m68k_src_l);

#if defined(SS_CPU_POST_INC)
    if(PostIncrement)
      areg[PARAM_N]+=4;
#endif

#if defined(SS_CPU_PREFETCH)
    if(M68000.PrefetchClass==2)
    {
      REFETCH_IR;
      PREFETCH_IRC;
    }
    if(M68000.PrefetchClass) // -(An), already fetched
      FETCH_TIMING;
#else
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
    FETCH_TIMING;  // move fetches after instruction
#endif  

  }// to memory
#if defined(SS_CPU_PREFETCH)
  if(M68000.PrefetchClass==1)
  {
    if((ir&BITS_876)==BITS_876_000 || (ir & BITS_876)==BITS_876_001)
      PREFETCH_IRC_NO_ROUND;
    else
      PREFETCH_IRC;
  }

#endif
}

#endif

#if defined(SS_CPU_MOVE_W)

void m68k_0011() //move.w
{
#if !defined(SS_CPU_LINE_3_TIMINGS)
#if defined(SS_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000
    ||(ir & BITS_876)==BITS_876_001)
    FETCH_TIMING_NO_ROUND;
  else
#endif
  INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SS_CPU_POST_INC)
  BOOL PostIncrement=FALSE;
#endif

#if defined(SS_CPU_ASSERT_ILLEGAL)
    if( (ir&BITS_876)==BITS_876_111
      && (ir&BITS_ba9)!=BITS_ba9_000
      && (ir&BITS_ba9)!=BITS_ba9_001 )
      m68k_unrecognised();
#endif

  m68k_GET_SOURCE_W;
  // Destination
  if ((ir & BITS_876)==BITS_876_000) // Dn
  {
#if defined(SS_CPU_LINE_3_TIMINGS) && !defined(SS_CPU_LINE_3_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_W=m68k_src_w;
    SR_CHECK_Z_AND_N_W;
#if defined(SS_CPU_LINE_3_TIMINGS) && defined(SS_CPU_LINE_3_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif
  }
  else if ((ir & BITS_876)==BITS_876_001) // An
  {
#if defined(SS_CPU_LINE_3_TIMINGS) && !defined(SS_CPU_LINE_3_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif
    areg[PARAM_N]=(signed long)((signed short)m68k_src_w); // movea
#if defined(SS_CPU_LINE_3_TIMINGS) && defined(SS_CPU_LINE_3_TIMINGS_2)
    FETCH_TIMING_NO_ROUND;
#endif
  }
  else
  {   //to memory
    bool refetch=0;
    switch (ir & BITS_876)
    {
    case BITS_876_010: // (An)
#if defined(SS_CPU_LINE_3_TIMINGS)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011:  // (An)+
#if defined(SS_CPU_LINE_3_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N];
#if defined(SS_CPU_POST_INC)
      PostIncrement=TRUE; // fixes Beyond loader
#else
      areg[PARAM_N]+=2;
#endif
      break;
    case BITS_876_100: // -(An)
#if defined(SS_CPU_LINE_3_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC;
      FETCH_TIMING;
#endif  
      areg[PARAM_N]-=2;
      abus=areg[PARAM_N];
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(12-4-4);
#if defined(SS_CPU_LINE_3_TIMINGS)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
#if defined(SS_CPU_3615GEN4_ULM) && defined(SS_SHIFTER)
      // Writing in video RAM just after it's been fetched into the shifter
      // fixes 3615Gen4 by ULM, but maybe not 100% (glitch in left border?)
      if(abus>=shifter_draw_pointer && abus<=shifter_draw_pointer+32 
        && Shifter.FetchingLine())
        Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
      pc+=2;
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(14-4-4);
#if defined(SS_CPU_LINE_3_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

      m68k_iriwo=m68k_fetchW();pc+=2;
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_876_111:
      if (SOURCE_IS_REGISTER_OR_IMMEDIATE==0) refetch=true;
      switch (ir & BITS_ba9){
      case BITS_ba9_000: // (xxx).W
        INSTRUCTION_TIME(12-4-4);
#if defined(SS_CPU_LINE_3_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2;
        break;
      case BITS_ba9_001: // (xxx).L
        INSTRUCTION_TIME(16-4-4);
#if defined(SS_CPU_LINE_3_TIMINGS)
      INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH)
        if(refetch) 
          M68000.PrefetchClass=2; 
#endif  
        abus=m68k_fetchL()&0xffffff;
        pc+=4;
        break;
#if defined(SS_CPU_ASSERT_ILLEGAL)
      default:
        m68k_unrecognised();
#endif
      }
    }
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if (!m68k_src_w){
      SR_SET(SR_Z);
    }
    if (m68k_src_w & MSB_W){
      SR_SET(SR_N);
    }

    m68k_dpoke_abus(m68k_src_w);

#if defined(SS_CPU_POST_INC)
    if(PostIncrement)
      areg[PARAM_N]+=2;
#endif

#if defined(SS_CPU_PREFETCH)
    if(M68000.PrefetchClass==2)
    {
      REFETCH_IR;
      PREFETCH_IRC;
    }
    if(M68000.PrefetchClass) // -(An), already fetched
      FETCH_TIMING;
#else
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
    FETCH_TIMING;  // move fetches after instruction
#endif  
  }// to memory
#if defined(SS_CPU_PREFETCH)
  if(M68000.PrefetchClass==1)
  {
    if((ir&BITS_876)==BITS_876_000 || (ir & BITS_876)==BITS_876_001)
      PREFETCH_IRC_NO_ROUND;
    else
      PREFETCH_IRC;
  }

#endif
}

#endif

#endif//#if defined(SS_CPU)
