/*---------------------------------------------------------------------------
FILE: cpu.cpp
MODULE: emu
DESCRIPTION: A full cycle-accurate Motorola 68000 emulation core. Trying to
follow the functions in this file is extremely difficult, we recommend
treating it as a black box. The only parts of this file really required by
the rest of the program are the macro m68k_PROCESS that executes the next
instruction and cpu_routines_init in cpuinit.cpp.
---------------------------------------------------------------------------*/

/*  Core of Steem's remarkable MC68000 emulation.
    It's a classic emulation in the sense that each instruction is
    emulated by a function that runs from start to end.
    Timings are handled in a microcode way, they're not added at once.
    As it is, Steem runs many, many programs, but this structure is
    a liability in some rare cases.
    An ideal emulation would have a 1 cycle loop, with each chip changing
    its state cycle by cycle. This could hurt performance. And we're still
    in trouble because of the different clock for the MFP (not a multiple).
    Also check SSECpu.h and SSECpu.cpp (eg MOVE rewritten)
    This part was already frightening, now it's worse with all the #ifdef
*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: cpu.cpp")
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_CPU_H)

#define EXT
#define INIT(s) =s

m68k_exception ExceptionObject;
jmp_buf *pJmpBuf=NULL;
EXT BYTE  m68k_peek(MEM_ADDRESS ad);
EXT WORD  m68k_dpeek(MEM_ADDRESS ad);
EXT LONG  m68k_lpeek(MEM_ADDRESS ad);
#if !defined(SSE_CPU_DIV)
EXT int m68k_divu_cycles INIT(124),m68k_divs_cycles INIT(140); // +4 for overall time
#endif
#ifdef DEBUG_BUILD
#ifndef RELEASE_BUILD
EXT MEM_ADDRESS pc_rel_stop_on_ref;
#endif
#endif

#undef EXT
#undef INIT

#ifdef SSE_UNIX
//temp! Don't understand what's going on
//#include "SSE/SSEDecla.h"
#if !defined(min)
#define min(a,b) (a>b ? b:a)
#define max(a,b) (a>b ? a:b)
#endif

#endif

WORD*lpfetch,*lpfetch_bound;
bool prefetched_2=false;
WORD prefetch_buf[2]; // SS the 2 words prefetch queue

#ifdef ENABLE_LOGFILE
MEM_ADDRESS debug_mem_write_log_address;
int debug_mem_write_log_bytes;
#endif

bool cpu_stopped=0;
bool m68k_do_trace_exception;
signed int compare_buffer;

void sr_check_z_n_l_for_r0()
{
  m68k_dest=&r[0];
  SR_CHECK_Z_AND_N_L;
}

#endif//defined(SSE_STRUCTURE_CPU_H)

#if defined(SSE_BOILER_MONITOR_VALUE2)
///#include "debug_emu.decla.h"
#endif

// SS neutralise macros if needs be
#if !defined(PREFETCH_IRC)
#define PREFETCH_IRC 
#define REFETCH_IR 
#if !defined(SSE_CPU_ROUNDING2)
#define PREFETCH_IRC_NO_ROUND
#endif
#endif

#ifndef CPU_ABUS_ACCESS
#define CPU_ABUS_ACCESS_READ INSTRUCTION_TIME_ROUND(4)
#define CPU_ABUS_ACCESS_WRITE INSTRUCTION_TIME_ROUND(4)
#endif

#if !defined(TRUE_PC)
//int silly_dummy_for_true_pc;
//#define TRUE_PC silly_dummy_for_true_pc //we guard now
//#define CHECK_READ silly_dummy_for_true_pc //we guard now
#endif

void (*m68k_high_nibble_jump_table[16])();
void (*m68k_jump_line_0[64])();
void (*m68k_jump_line_4[64])();
void (*m68k_jump_line_5[8])();
void (*m68k_jump_line_8[8])();
void (*m68k_jump_line_9[8])();
void (*m68k_jump_line_b[8])();
void (*m68k_jump_line_c[8])();
void (*m68k_jump_line_d[8])();
void (*m68k_jump_line_e[64])();
void (*m68k_jump_line_4_stuff[64])();
void (*m68k_jump_get_source_b[8])();
void (*m68k_jump_get_source_w[8])();
void (*m68k_jump_get_source_l[8])();
void (*m68k_jump_get_source_b_not_a[8])();
void (*m68k_jump_get_source_w_not_a[8])();
void (*m68k_jump_get_source_l_not_a[8])();
void (*m68k_jump_get_dest_b[8])();
void (*m68k_jump_get_dest_w[8])();
void (*m68k_jump_get_dest_l[8])();
void (*m68k_jump_get_dest_b_not_a[8])();
void (*m68k_jump_get_dest_w_not_a[8])();
void (*m68k_jump_get_dest_l_not_a[8])();
void (*m68k_jump_get_dest_b_not_a_or_d[8])();
void (*m68k_jump_get_dest_w_not_a_or_d[8])();
void (*m68k_jump_get_dest_l_not_a_or_d[8])();
#if !defined(SSE_CPU_ROUNDING_NO_FASTER_FOR_D)
void (*m68k_jump_get_dest_b_not_a_faster_for_d[8])();
void (*m68k_jump_get_dest_w_not_a_faster_for_d[8])();
void (*m68k_jump_get_dest_l_not_a_faster_for_d[8])();
#endif
bool (*m68k_jump_condition_test[16])();

#if !defined(SSE_CPU_INLINE_READ_FROM_ADDR)

#if !defined(SSE_MMU_NO_CONFUSION)

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

#else//we remove mmu_confused parts...

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

#endif//confusion!
#endif//SSE_CPU_INLINE_READ_FROM_ADDR

#if !defined(SSE_CPU_INLINE_READ_BWL)

#define m68k_READ_B(addr)                              \
  m68k_src_b=m68k_peek(addr);                           \

#define m68k_READ_W(addr)                              \
  m68k_src_w=m68k_dpeek(addr);                           \

#define m68k_READ_L(addr)                              \
  m68k_src_l=m68k_lpeek(addr);                           \

#endif//SSE_CPU_INLINE_READ_BWL

//---------------------------------------------------------------------------
// SS not inlined in VC6

#if !defined(SSE_STRUCTURE_SSECPU_OBJ)
inline void change_to_user_mode()
{
//  if(SUPERFLAG){
  ASSERT(!SUPERFLAG);
  compare_buffer=r[15];r[15]=other_sp;other_sp=compare_buffer;
  SR_CLEAR(SR_SUPER);
//  }
}
//---------------------------------------------------------------------------
inline void change_to_supervisor_mode()
{
//  if(!SUPERFLAG){
  compare_buffer=r[15];r[15]=other_sp;other_sp=compare_buffer;
  SR_SET(SR_SUPER);
//  }
}
#endif


#define LOGSECTION LOGSECTION_CRASH//SS
void m68k_exception::init(int a,exception_action ea,MEM_ADDRESS _abus)
{
  bombs=a;
  _pc=PC32; //old_pc;
  crash_address=old_pc; //SS this is for group 1+2
  address=_abus;
  _sr=::sr;_ir=::ir;
  action=ea;
}

//#define LOGSECTION LOGSECTION_CRASH//ss moved up
//---------------------------------------------------------------------------
void ASMCALL perform_crash_and_burn()
{
#if !defined(SSE_CPU_HALT) || !defined(SSE_GUI_STATUS_STRING_HALT)
  reset_st(RESET_COLD | RESET_NOSTOP | RESET_CHANGESETTINGS | RESET_NOBACKUP);
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
  TRACE("==============\nCRASH AND BURN\n==============\n");
#endif
#if defined(SSE_CPU_HALT) && defined(SSE_GUI_STATUS_STRING_HALT)
  runstate=RUNSTATE_STOPPING;
  M68000.ProcessingState=TM68000::HALTED;
  GUIRefreshStatusBar(); // no OSD, it's mentioned there
#else
  osd_start_scroller(T("CRASH AND BURN - ST RESET"));
#endif
}
//---------------------------------------------------------------------------
#ifdef DEBUG_BUILD
void log_registers()
{
  log_write("        Register dump:");
  log_write(EasyStr("        pc = ")+HEXSl(pc,8));
  log_write(EasyStr("        sr = ")+HEXSl(sr,4));
  for(int n=0;n<16;n++){
    log_write(EasyStr("        ")+("d\0a\0"+(n/8)*2)+(n&7)+" = "+HEXSl(r[n],8));
  }
}

void log_history(int bombs,MEM_ADDRESS crash_address)
{
  if (logsection_enabled[LOGSECTION] && logging_suspended==0){
    log("");
    log("****************************************");
    if (bombs){
      log(EasyStr(bombs)+" bombs");
    }else{
      log("Exception/interrupt");
    }
    log(EasyStr("Crash at ")+HEXSl(crash_address,6));
    int n=pc_history_idx-HIST_MENU_SIZE;
    if (n<0) n=HISTORY_SIZE+n;
    EasyStr Dissasembly;
    do{
      if (pc_history[n]!=0xffffff71){
        Dissasembly=disa_d2(pc_history[n]);
        log(EasyStr(HEXSl(pc_history[n],6))+" - "+Dissasembly);
      }
      n++; if (n>=HISTORY_SIZE) n=0;
    }while (n!=pc_history_idx);
    log("^^ Crash!");
    log("****************************************");
    log("");
  }
}
#endif
//---------------------------------------------------------------------------
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU)) // not inlined in VC6 -> made macro
NOT_DEBUG(inline) void m68k_interrupt(MEM_ADDRESS ad) //not address, bus, illegal instruction or privilege violation interrupt
{
  WORD _sr=sr;
  if (!SUPERFLAG) change_to_supervisor_mode();
  m68k_PUSH_L(PC32);
  m68k_PUSH_W(_sr);
  SET_PC(ad);
//  log(EasyStr("interrupt - increasing interrupt depth from ")+interrupt_depth+" to "+(interrupt_depth+1));
  SR_CLEAR(SR_TRACE);
  interrupt_depth++;
}
#endif
//---------------------------------------------------------------------------
// TODO: Allow exception frames to be written to IO?

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_EXCEPTION))// redone in SSEM68000.cpp
void m68k_exception::crash()
{
  DWORD bytes_to_stack=int((bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR) ? (4+2+2+4+2):(4+2));
  MEM_ADDRESS sp=(MEM_ADDRESS)(SUPERFLAG ? (areg[7] & 0xffffff):(other_sp & 0xffffff));
  if (sp<bytes_to_stack || sp>FOUR_MEGS){
    // Double bus error, CPU halt (we crash and burn)
    // This only has to be done here, m68k_PUSH_ will cause bus error if invalid
    DEBUG_ONLY( log_history(bombs,crash_address) );
    perform_crash_and_burn();
  }else{
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    if (bombs==BOMBS_ILLEGAL_INSTRUCTION || bombs==BOMBS_PRIVILEGE_VIOLATION){
      if (!SUPERFLAG) change_to_supervisor_mode();
      m68k_PUSH_L((crash_address & 0x00ffffff) | pc_high_byte);
      INSTRUCTION_TIME_ROUND(8);
      m68k_PUSH_W(_sr);
      INSTRUCTION_TIME_ROUND(4); //Round first for interrupts
      MEM_ADDRESS ad=LPEEK(bombs*4);
      if (ad & 1){
        bombs=BOMBS_ADDRESS_ERROR;
        address=ad;
        action=EA_FETCH;
      }else{
        SET_PC(ad);
        SR_CLEAR(SR_TRACE);
        INSTRUCTION_TIME_ROUND(22);
        interrupt_depth++;                                        // Is this necessary?
      }
    }
    if (bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR){
      if (!SUPERFLAG) change_to_supervisor_mode();
      // Big page in stack
//      try{
      TRY_M68K_EXCEPTION
        if ((_ir & 0xf000)==(b00100000 << 8)) _pc+=2; // move.l stacks pc+2 (for War Heli) 
        m68k_PUSH_L(_pc);
        m68k_PUSH_W(_sr);
        m68k_PUSH_W(_ir);
        m68k_PUSH_L(address);
        WORD x=WORD(_ir & 0xffe0);
        if (action!=EA_WRITE) x|=B6_010000;
        if (action==EA_FETCH){
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_PROGRAM:FC_USER_PROGRAM);
        }else{
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_DATA:FC_USER_DATA);
        }
        m68k_PUSH_W(x);
//      }catch (m68k_exception&){
      CATCH_M68K_EXCEPTION
        r[15]=0xf000;
      END_M68K_EXCEPTION

      SET_PC(LPEEK(bombs*4));
      SR_CLEAR(SR_TRACE);

      INSTRUCTION_TIME_ROUND(50); //Round for fetch
    }
    DEBUG_ONLY(log_history(bombs,crash_address));
  }
  PeekEvent(); // Stop exception freeze
}
#endif

#undef LOGSECTION

#if !defined(STEVEN_SEAGAL)

NOT_DEBUG(inline) void m68k_poke_abus(BYTE x){
  abus&=0xffffff;
  if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_b(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SSE_MMU_NO_CONFUSION)
    if (mmu_confused){
      mmu_confused_set_dest_to_addr(1,true);
      m68k_DEST_B=x;
    }else
#endif
    if (abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_B(abus);
    if (SUPERFLAG && abus>=MEM_FIRST_WRITEABLE) //SS !defined
      PEEK(abus)=x;
    else if (abus>=MEM_START_OF_USER_AREA)
      PEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}


NOT_DEBUG(inline) void m68k_dpoke_abus(WORD x){
  abus&=0xffffff;
  if(abus&1) exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_w(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SSE_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(2,true);
      m68k_DEST_W=x;
    }else 
#endif
    if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_W(abus);
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)//SS !defined
      DPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      DPEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}


NOT_DEBUG(inline) void m68k_lpoke_abus(LONG x){
  abus&=0xffffff;
  if(abus&1)exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_l(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SSE_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(4,true);
      m68k_DEST_L=x;
    }else 
#endif
    if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_L(abus);
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)//SS !defined
      LPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      LPEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}

#if !defined(SSE_STRUCTURE_CPU_H)
inline void m68k_poke(MEM_ADDRESS ad,BYTE x){
  abus=ad;
  m68k_poke_abus(x);
}

inline void m68k_dpoke(MEM_ADDRESS ad,WORD x){
  abus=ad;
  m68k_dpoke_abus(x);
}

inline void m68k_lpoke(MEM_ADDRESS ad,LONG x){
  abus=ad;
  m68k_lpoke_abus(x);
}
#endif

#endif//SS

BYTE m68k_peek(MEM_ADDRESS ad){
  ad&=0xffffff;
  if (ad>=himem){
    if (ad>=MEM_IO_BASE){
      if(SUPERFLAG)return io_read_b(ad);
      else exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=0xfc0000){
#if defined(SSE_CPU_ROUNDING_BUS)
      if(tos_high && ad<(0xfc0000+192*1024))
      {
        if(M68000.Rounded)
        {
          INSTRUCTION_TIME(-2);
          M68000.Rounded=false;
        }
        return ROM_PEEK(ad-rom_addr);
      }
#else
      if(tos_high && ad<(0xfc0000+192*1024))return ROM_PEEK(ad-rom_addr);
#endif
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=MEM_EXPANSION_CARTRIDGE){
      if (cart) return CART_PEEK(ad-MEM_EXPANSION_CARTRIDGE);
      else return 0xff;
    }else if(ad>=rom_addr){
#if defined(SSE_CPU_ROUNDING_BUS)
      if (ad<(0xe00000+256*1024)) 
      {
        if(M68000.Rounded)
        {
          INSTRUCTION_TIME(-2);
          M68000.Rounded=false;
        }
        return ROM_PEEK(ad-rom_addr);
      }
#else
      if (ad<(0xe00000+256*1024)) return ROM_PEEK(ad-rom_addr);
#endif
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xff;
#if !defined(SSE_MMU_NO_CONFUSION)
    }else if (mmu_confused){
      return mmu_confused_peek(ad,true);
#endif
    }else if (ad>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else{
      return 0xff;
    }
  }else if(ad>=MEM_START_OF_USER_AREA){
    DEBUG_CHECK_READ_B(ad);
    return (BYTE)(PEEK(ad));
  }else if(SUPERFLAG){
    DEBUG_CHECK_READ_B(ad);
    return (BYTE)(PEEK(ad));
  }else exception(BOMBS_BUS_ERROR,EA_READ,ad);
  return 0;
}

WORD m68k_dpeek(MEM_ADDRESS ad){
  ad&=0xffffff;
//  ASSERT( ad!=0x400000);
//  ASSERT( ad!=0x080000);
  if(ad&1)exception(BOMBS_ADDRESS_ERROR,EA_READ,ad);
  else if(ad>=himem){
    if(ad>=MEM_IO_BASE){
      if(SUPERFLAG)return io_read_w(ad);
      else exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=0xfc0000){
#if defined(SSE_CPU_ROUNDING_BUS)
      if(tos_high && ad<(0xfc0000+192*1024))
      {
        if(M68000.Rounded)
        {
          INSTRUCTION_TIME(-2);
          M68000.Rounded=false;
        }
        return ROM_DPEEK(ad-rom_addr);
      }
#else
      if(tos_high && ad<(0xfc0000+192*1024))return ROM_DPEEK(ad-rom_addr);
#endif
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=MEM_EXPANSION_CARTRIDGE){
      if (cart) return CART_DPEEK(ad-MEM_EXPANSION_CARTRIDGE);
      else return 0xffff;
    }else if(ad>=rom_addr){
#if defined(SSE_CPU_ROUNDING_BUS)
      if (ad<(0xe00000+256*1024)) 
      {
        if(M68000.Rounded)
        {
          INSTRUCTION_TIME(-2);
          M68000.Rounded=false;
        }
        return ROM_DPEEK(ad-rom_addr);
      }
#else
      if (ad<(0xe00000+256*1024)) return ROM_DPEEK(ad-rom_addr);
#endif
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xffff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xffff;
#if !defined(SSE_MMU_NO_CONFUSION)
    }else if(mmu_confused){
      return mmu_confused_dpeek(ad,true);
#endif
#if defined(SSE_CPU_IGNORE_RW_4MB)//undef 3.7
      // safe mod for RAM<4MB, fixes F-29 4MB, along with poke
    }else if(ad>FOUR_MEGS || ad==FOUR_MEGS&&mem_len<FOUR_MEGS){
#else
    }else if(ad>=FOUR_MEGS){
#endif
      exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else{
      return 0xffff;
    }
  }else if(ad>=MEM_START_OF_USER_AREA){
    DEBUG_CHECK_READ_W(ad);
    return DPEEK(ad);
  }else if(SUPERFLAG){
    DEBUG_CHECK_READ_W(ad);
    return DPEEK(ad);
  }else exception(BOMBS_BUS_ERROR,EA_READ,ad);
  return 0;
}

LONG m68k_lpeek(MEM_ADDRESS ad){
  ad&=0xffffff;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_EXCEPTION)
  if(ad&1)exception(BOMBS_ADDRESS_ERROR,EA_READ,ad); // bug fix!
#else
  if(ad&1)exception(BOMBS_ADDRESS_ERROR,EA_WRITE,ad); 
#endif
  else if(ad>=himem){
    if(ad>=MEM_IO_BASE){
      if(SUPERFLAG)return io_read_l(ad);
      else exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=0xfc0000){
#if defined(SSE_CPU_ROUNDING_BUS)
      if(tos_high && ad<(0xfc0000+192*1024))
      {
        if(M68000.Rounded)
        {
          INSTRUCTION_TIME(-2);
          M68000.Rounded=false;
        }
        return ROM_LPEEK(ad-rom_addr);
      }
#else
      if(tos_high && ad<(0xfc0000+192*1024-2))return ROM_LPEEK(ad-rom_addr);
#endif
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if (ad>=MEM_EXPANSION_CARTRIDGE){
      if (cart) return CART_LPEEK(ad-MEM_EXPANSION_CARTRIDGE);
      else return 0xffffffff;
    }else if (ad>=rom_addr){
#if defined(SSE_CPU_ROUNDING_BUS)
      if (ad<(0xe00000+256*1024)) 
      {
        if(M68000.Rounded)
        {
          INSTRUCTION_TIME(-2);
          M68000.Rounded=false;
        }
        return ROM_LPEEK(ad-rom_addr);
      }
#else
      if (ad<(0xe00000+256*1024-2)) return ROM_LPEEK(ad-rom_addr);
#endif
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xffffffff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xffffffff;
#if !defined(SSE_MMU_NO_CONFUSION)
    }else if (mmu_confused){
      return mmu_confused_lpeek(ad,true);
#endif
    }else if (ad>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else{
      return 0xffffffff;
    }
  }else if (ad>=MEM_START_OF_USER_AREA){
    DEBUG_CHECK_READ_L(ad);
    return LPEEK(ad);
  }else if (SUPERFLAG){
    DEBUG_CHECK_READ_L(ad);
    return LPEEK(ad);
  }else exception(BOMBS_BUS_ERROR,EA_READ,ad);
  return 0;
}

BYTE m68k_fetchB()
{
  WORD ret;
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
  INSTRUCTION_TIME_ROUND(4);
#endif
  FETCH_W(ret)
  return LOBYTE(ret);
}

WORD m68k_fetchW()
{
  WORD ret;
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
  INSTRUCTION_TIME_ROUND(4);
#endif
  FETCH_W(ret)
  return ret;
}

LONG m68k_fetchL()
{
  LONG ret;
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
  INSTRUCTION_TIME_ROUND(8);
#endif
  FETCH_W(*LPHIWORD(ret));
  FETCH_W(*LPLOWORD(ret));
  return ret;
}

void m68k_unrecognised() //SS trap1 in Motorola doc
{
  exception(BOMBS_ILLEGAL_INSTRUCTION,EA_INST,0);
}

BYTE m68k_read_dest_b(){
  BYTE x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return LOBYTE(r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    INSTRUCTION_TIME_ROUND(4);
    return m68k_peek(areg[PARAM_M]);
  case BITS_543_011:
    INSTRUCTION_TIME_ROUND(4);
    x=m68k_peek(areg[PARAM_M]);areg[PARAM_M]++;
    if(PARAM_M==7)areg[7]++;
    return x;
  case BITS_543_100:
    areg[PARAM_M]--;
    if (PARAM_M==7) areg[7]--;
    INSTRUCTION_TIME_ROUND(6);
    return m68k_peek(areg[PARAM_M]);
  case BITS_543_101:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;
#else
    INSTRUCTION_TIME_ROUND(8);
#endif
    register MEM_ADDRESS ad=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    CPU_ABUS_ACCESS_READ;
#endif
    x=m68k_peek(ad);
    return x;
  }case BITS_543_110:
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);
    CPU_ABUS_ACCESS_READ_FETCH;
#else
    INSTRUCTION_TIME_ROUND(10);
#endif
    m68k_iriwo=m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    CPU_ABUS_ACCESS_READ;
#endif
    if(m68k_iriwo&BIT_b){  //.l
      return m68k_peek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]);
    }else{         //.w
      return m68k_peek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]);
    }
  case BITS_543_111:
    switch(ir&0x7){
    case 0:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME_ROUND(8);
#endif
      register MEM_ADDRESS ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ;
#endif
      x=m68k_peek(ad);
      return x;
    }case 1:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH;
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME_ROUND(12);
#endif
      register MEM_ADDRESS ad=m68k_fetchL()&0xffffff;
      pc+=4;  
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ;
#endif
      x=m68k_peek(ad);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}

WORD m68k_read_dest_w(){
  WORD x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return LOWORD(r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    INSTRUCTION_TIME_ROUND(4);
    return m68k_dpeek(areg[PARAM_M]);
  case BITS_543_011:
    INSTRUCTION_TIME_ROUND(4);
    x=m68k_dpeek(areg[PARAM_M]);areg[PARAM_M]+=2;
    return x;
  case BITS_543_100:
    INSTRUCTION_TIME_ROUND(6);
    areg[PARAM_M]-=2;
    return m68k_dpeek(areg[PARAM_M]);
  case BITS_543_101:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;
#else
    INSTRUCTION_TIME_ROUND(8);
#endif
    register MEM_ADDRESS ad=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    CPU_ABUS_ACCESS_READ;
#endif
    x=m68k_dpeek(ad);
    return x;
  }case BITS_543_110:
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);
    CPU_ABUS_ACCESS_READ_FETCH;
#else
    INSTRUCTION_TIME_ROUND(10);
#endif
    m68k_iriwo=m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    CPU_ABUS_ACCESS_READ;
#endif
    if(m68k_iriwo&BIT_b){  //.l
      return m68k_dpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]);
    }else{         //.w
      return m68k_dpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]);
    }
  case BITS_543_111:
    switch(ir&ir&0x7){
    case 0:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME_ROUND(8);
#endif
      register MEM_ADDRESS ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ;
#endif
      x=m68k_dpeek(ad);
      return x;
    }case 1:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH;
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME_ROUND(12);
#endif
      register MEM_ADDRESS ad=m68k_fetchL()&0xffffff;
      pc+=4;  
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ;
#endif
      x=m68k_dpeek(ad);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}

LONG m68k_read_dest_l(){
  LONG x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return (r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    INSTRUCTION_TIME_ROUND(8);
    return m68k_lpeek(areg[PARAM_M]);
  case BITS_543_011:
    INSTRUCTION_TIME_ROUND(8);
    x=m68k_lpeek(areg[PARAM_M]);areg[PARAM_M]+=4;
    return x;
  case BITS_543_100:
    INSTRUCTION_TIME_ROUND(10);
    areg[PARAM_M]-=4;
    return m68k_lpeek(areg[PARAM_M]);
  case BITS_543_101:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    //  (d16,An)        | 101 | reg |  12(3/0)   |              np nR nr          
    CPU_ABUS_ACCESS_READ_FETCH;
#else
    INSTRUCTION_TIME_ROUND(12);
#endif
    register MEM_ADDRESS ad=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    CPU_ABUS_ACCESS_READ;
    CPU_ABUS_ACCESS_READ;
#endif
    x=m68k_lpeek(ad);
    return x;
  }case BITS_543_110:
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (d8,An,Xn)      | 110 | reg |  14(3/0)   |         n    np nR nr           
    INSTRUCTION_TIME(2);
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_iriwo=m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
    CPU_ABUS_ACCESS_READ;
    CPU_ABUS_ACCESS_READ;
#else
    INSTRUCTION_TIME_ROUND(14);
#endif
    if(m68k_iriwo&BIT_b){  //.l
      return m68k_lpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]);
    }else{         //.w
      return m68k_lpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]);
    }
  case BITS_543_111:
    switch(ir&0x7){
    case 0:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//  (xxx).W         | 111 | 000 |  12(3/0)   |              np nR nr      
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME_ROUND(12);
#endif
      register MEM_ADDRESS ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ;
      CPU_ABUS_ACCESS_READ;
#endif
      x=m68k_lpeek(ad);
      return x;
    }case 1:{
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
//(xxx).L         | 111 | 001 |  16(4/0)   |           np np nR nr           
      CPU_ABUS_ACCESS_READ_FETCH;
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME_ROUND(16);
#endif
      register MEM_ADDRESS ad=m68k_fetchL()&0xffffff;
      pc+=4;  
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ;
      CPU_ABUS_ACCESS_READ;
#endif
      x=m68k_lpeek(ad);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}

#if !defined(SSE_BOILER_MONITOR_372) // defined in SSECpu.h
#ifdef DEBUG_BUILD
#define DEBUG_CHECK_IOACCESS \
  if (ioaccess & IOACCESSE_DEBUG_MEM_WRITE_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Write to address $"+HEXSl(debug_mem_write_log_address,6)+ \
                  ", new value is "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
  }
#else
#define DEBUG_CHECK_IOACCESS
#endif
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU)
// already defined
#else
// keep as macro, wouldn't be inlined
#define HANDLE_IOACCESS(tracefunc) \
  if (ioaccess){                             \
    switch (ioaccess & IOACCESS_NUMBER_MASK){                        \
      case 1: io_write_b(ioad,LOBYTE(iobuffer)); break;    \
      case 2: io_write_w(ioad,LOWORD(iobuffer)); break;    \
      case 4: io_write_l(ioad,iobuffer); break;      \
      case TRACE_BIT_JUST_SET: /*if(!M68000.ProcessingBusError)*/ tracefunc; break;                                        \
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

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU))
#define m68k_PROCESS \
  LOG_CPU  \
  old_pc=pc;  \
  FETCH_W(ir)       \
  pc+=2;               \
  m68k_high_nibble_jump_table[ir>>12]();    \
  HANDLE_IOACCESS(m68k_trace();)           \
  DEBUG_ONLY( debug_first_instruction=0 );
#endif

#define LOGSECTION LOGSECTION_TRACE

#if defined(SSE_CPU_TRACE_REFACTOR)
// See TM68000:Process in SSECpu.h
#else

#if defined(STEVEN_SEAGAL) && (defined(SSE_VAR_REWRITE) || defined(SSE_VS2008))
extern "C" void ASMCALL m68k_trace() //execute instruction with trace bit set
#else
extern "C" ASMCALL void m68k_trace() //execute instruction with trace bit set
#endif
{

#ifdef DEBUG_BUILD
  pc_history[pc_history_idx++]=pc;
  if (pc_history_idx>=HISTORY_SIZE) pc_history_idx=0;
  EasyStr Dissasembly=disa_d2(pc);
  log(EasyStr("TRACE: ")+HEXSl(pc,6)+" - "+Dissasembly);
#endif

  ASSERT( sr&SR_TRACE );

#if defined(SSE_CPU_TRACE_DETECT) && !defined(DEBUG_BUILD)
  TRACE_OSD("TRACE");
#endif
#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK_CPU & OSD_CONTROL_CPUTRACE) 
    TRACE_OSD("TRACE");
#endif

  LOG_CPU

  old_pc=pc;

  DEBUG_ONLY( if (debug_num_bk) breakpoint_check(); )

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU)
  M68000.FetchWord(ir);
#else
  ir=m68k_fetchW();
#endif

  pc+=2;

#if defined(SSE_CPU_TRUE_PC)
  M68000.Pc=pc; // anyway
  M68000.CheckRead=0;
#endif
  
  // Store blitter and interrupt check bits, set trace exception bit, lose everything else
  int store_ioaccess=ioaccess & (IOACCESS_FLAG_DO_BLIT | IOACCESS_FLAG_FOR_CHECK_INTRS |
    IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE);
  ioaccess=0;
  m68k_do_trace_exception=true; //SS this flag won't be reset
#if defined(STEVEN_SEAGAL) 
#if defined(SSE_CPU) && defined(SSE_DEBUG)
  M68000.PreviousIr=ir;
  M68000.nInstr++;
#endif
#if defined(SSE_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=0;
#endif
#endif
 //ASSERT(ir!=0x19F3); // dbg: break on opcode...
  m68k_high_nibble_jump_table[ir>>12](); //SS call in trace

  ///TRACE_LOG("SR = %x\n",sr);

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU) && defined(SSE_DEBUG)
  M68000.NextIrFetched=false;
#endif
  if (m68k_do_trace_exception){
    // This flag is used for exceptions that we don't want to do a proper exception
    // for but should really. i.e Line-A/F, they are just as serious as illegal
    // instruction but are called reguarly, we don't want to slow things down.

#ifdef DEBUG_BUILD
    TRACE_LOG("TRACE PC %X SR %X VEC %X",old_pc,sr,LPEEK(0x24));
    EasyStr instr=disa_d2(old_pc); // take advantage of the disassembler
    //TRACE_LOG("\n");
    TRACE_LOG("IR %X: %s\n",ir,instr.Text);
    //TRACE_LOG("TRACE PC %X VEC %X\n",pc,LPEEK(0x24));
#else
    TRACE_LOG("TRACE PC %X IR %X SR %X $24 %X\n",pc,ir,sr,LPEEK(0x24));
#endif

    // this timing is precisely tested by My Socks are Weapons
#if defined(SSE_CPU_TRACE_TIMING)
    //tests
    INSTRUCTION_TIME_ROUND(36); //same effect
#else
    INSTRUCTION_TIME_ROUND(0); // Round first for interrupts
    INSTRUCTION_TIME_ROUND(34);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("TRACE");
#endif
//    TRACE_LOG("SR=%X\n",sr);
    m68k_interrupt(LPEEK(BOMBS_TRACE_EXCEPTION*4));

  }
  ioaccess|=store_ioaccess;

  // In case of IOACCESS_FLAG_FOR_CHECK_INTRS interrupt must happen after trace
  HANDLE_IOACCESS(;)


}
#endif//#if defined(SSE_CPU_TRACE_REFACTOR)
#undef LOGSECTION
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_ROUDING_EA))
/*  This function is confusing timing reckoning, so now each of the concerned
    instructions (JMP, JSR, LEA, PEA) will directly get EA.
*/
void m68k_get_effective_address()
{
  // Note: The timings in this routine are completely wrong. It is only used for
  //       lea, pea, jsr and jmp, they all have different EA times so they all
  //       correct for these bogus times. Do not use this routine somewhere that
  //       doesn't correct timings!
  switch (ir & BITS_543){
    case BITS_543_010:
      INSTRUCTION_TIME_ROUND(0);
      effective_address=areg[PARAM_M];
      break;
    case BITS_543_101:
      INSTRUCTION_TIME_ROUND(4);
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_543_110:
      INSTRUCTION_TIME_ROUND(8);
      m68k_iriwo=m68k_fetchW();pc+=2; 
      if (m68k_iriwo & BIT_b){  //.l
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_543_111:
      switch (ir & 0x7){
      case 0:
        INSTRUCTION_TIME_ROUND(4);
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
#if defined(SSE_CPU_TRUE_PC2)
/*  For JMP and JSR, different microcodes are used and PC isn't changed before
    it is set, even for fetching operands (AU is used as usual).
    Fixes The Teller (jmp $201.w)
    LEA, PEA: TODO
*/ 
        if(ir&0xFF80!=0x4E80)
#endif
          TRUE_PC+=2;
#endif
        break;
      case 1:
        INSTRUCTION_TIME_ROUND(8);
        effective_address=m68k_fetchL();
        pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
#if defined(SSE_CPU_TRUE_PC2)
        if(ir&0xFF80!=0x4E80) // also for .L according to microcodes
#endif
        TRUE_PC+=4;
#endif
        break;
      case 2:
        INSTRUCTION_TIME_ROUND(4);
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;
      case 3:
        INSTRUCTION_TIME_ROUND(8);
        m68k_iriwo=m68k_fetchW();
        if (m68k_iriwo & BIT_b){  //.l
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]) | pc_high_byte;
        }else{         //.w
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]) | pc_high_byte;
        }
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;       //what do bits 8,9,a  of extra word do?  (not always 0)
      default:
        m68k_unrecognised();
        break;
      }
      break;
    default:
      m68k_unrecognised();
  }
}
#endif

#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU)) // moved to SSECpu.cpp
// forward (temp, structure!)

#if !defined(SSE_STRUCTURE_CPU_H) // in cpu.decla.h
void m68k_get_source_000_b();
void m68k_get_source_000_w();
void m68k_get_source_000_l();
void m68k_get_source_001_b();
void m68k_get_source_001_w();
void m68k_get_source_001_l();
void m68k_get_source_010_b();
void m68k_get_source_010_w();
void m68k_get_source_010_l();
void m68k_get_source_011_b();
void m68k_get_source_011_w();
void m68k_get_source_011_l();
void m68k_get_source_100_b();
void m68k_get_source_100_w();
void m68k_get_source_100_l();
void m68k_get_source_101_b();
void m68k_get_source_101_w();
void m68k_get_source_101_l();
void m68k_get_source_110_b();
void m68k_get_source_110_w();
void m68k_get_source_110_l();
void m68k_get_source_111_b();
void m68k_get_source_111_w();
void m68k_get_source_111_l();
#endif

#else
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//////////////////////////    GET SOURCE     ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void m68k_get_source_000_b(){ m68k_src_b=(BYTE)(r[PARAM_M]); }
void m68k_get_source_000_w(){ m68k_src_w=(WORD)(r[PARAM_M]); }
void m68k_get_source_000_l(){ m68k_src_l=(long)(r[PARAM_M]); }
void m68k_get_source_001_b(){ m68k_src_b=(BYTE)(areg[PARAM_M]); }
void m68k_get_source_001_w(){ m68k_src_w=(WORD)(areg[PARAM_M]); }
void m68k_get_source_001_l(){ m68k_src_l=(long)(areg[PARAM_M]); }
void m68k_get_source_010_b(){ INSTRUCTION_TIME_ROUND(4);m68k_READ_B(areg[PARAM_M]) }
void m68k_get_source_010_w(){ INSTRUCTION_TIME_ROUND(4);m68k_READ_W(areg[PARAM_M]) }
void m68k_get_source_010_l(){ INSTRUCTION_TIME_ROUND(8);m68k_READ_L(areg[PARAM_M]) }
void m68k_get_source_011_b(){ INSTRUCTION_TIME_ROUND(4);m68k_READ_B(areg[PARAM_M]) areg[PARAM_M]++; if(PARAM_M==7)areg[7]++;}
void m68k_get_source_011_w(){ INSTRUCTION_TIME_ROUND(4);m68k_READ_W(areg[PARAM_M]) areg[PARAM_M]+=2; }
void m68k_get_source_011_l(){ INSTRUCTION_TIME_ROUND(8);m68k_READ_L(areg[PARAM_M]) areg[PARAM_M]+=4; }
void m68k_get_source_100_b(){ INSTRUCTION_TIME_ROUND(6);/* 6 */ areg[PARAM_M]--;if(PARAM_M==7)areg[7]--;m68k_READ_B(areg[PARAM_M]) }
void m68k_get_source_100_w(){ INSTRUCTION_TIME_ROUND(6);/* 6 */areg[PARAM_M]-=2;m68k_READ_W(areg[PARAM_M])  }
void m68k_get_source_100_l(){ INSTRUCTION_TIME_ROUND(10);/* 10 */areg[PARAM_M]-=4;m68k_READ_L(areg[PARAM_M])  }
void m68k_get_source_101_b(){
  INSTRUCTION_TIME_ROUND(8);
  register int fw=(signed short)m68k_fetchW(); pc+=2;
  m68k_READ_B(areg[PARAM_M]+fw);

}
void m68k_get_source_101_w(){
  INSTRUCTION_TIME_ROUND(8);
  register int fw=(signed short)m68k_fetchW(); pc+=2;
  m68k_READ_W(areg[PARAM_M]+fw);
}
void m68k_get_source_101_l(){
  INSTRUCTION_TIME_ROUND(12);
  register int fw=(signed short)m68k_fetchW(); pc+=2;
  m68k_READ_L(areg[PARAM_M]+fw);
}
void m68k_get_source_110_b(){
  INSTRUCTION_TIME_ROUND(10);
  WORD w=m68k_fetchW();pc+=2;
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  m68k_READ_B_FROM_ADDR
}
void m68k_get_source_110_w(){
  INSTRUCTION_TIME_ROUND(10);
  WORD w=m68k_fetchW();pc+=2;
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  m68k_READ_W_FROM_ADDR
}
void m68k_get_source_110_l(){
  INSTRUCTION_TIME_ROUND(14);
  WORD w=m68k_fetchW();pc+=2;
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  m68k_READ_L_FROM_ADDR
}
void m68k_get_source_111_b(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2;
    m68k_READ_B(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fl=m68k_fetchL();pc+=4;
    m68k_READ_B(fl)
    break;
  }case 2:{
    INSTRUCTION_TIME_ROUND(8);
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();pc+=2;
    PC_RELATIVE_MONITOR(ad);
    m68k_READ_B(ad)
    break;
  }case 3:{
    INSTRUCTION_TIME_ROUND(10);
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    PC_RELATIVE_MONITOR(abus);
    pc+=2;
    m68k_READ_B_FROM_ADDR
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }case 4:{
    INSTRUCTION_TIME_ROUND(4);
    m68k_src_b=m68k_fetchB();
    pc+=2;
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_source_111_w(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2;
    m68k_READ_W(fw)

    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fl=m68k_fetchL();pc+=4;
    m68k_READ_W(fl)
    break;
  }case 2:{
    INSTRUCTION_TIME_ROUND(8);
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();pc+=2;
    PC_RELATIVE_MONITOR(ad);
    m68k_READ_W(ad)
    break;
  }case 3:{
    INSTRUCTION_TIME_ROUND(10);
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    PC_RELATIVE_MONITOR(abus);
    pc+=2;
    m68k_READ_W_FROM_ADDR
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }case 4:{
    INSTRUCTION_TIME_ROUND(4);
//      ap=m68k_fetchL();pc+=4;
    m68k_src_w=m68k_fetchW();
    pc+=2;
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_source_111_l(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(12);
    register signed int fw=(signed short)m68k_fetchW();pc+=2;
    m68k_READ_L(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fl=m68k_fetchL();pc+=4;
    m68k_READ_L(fl)
    break;
  }case 2:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();pc+=2;
    PC_RELATIVE_MONITOR(ad);
    m68k_READ_L(ad)
    break;
  }case 3:{
    INSTRUCTION_TIME_ROUND(14);
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    PC_RELATIVE_MONITOR(abus);
    pc+=2;
    m68k_READ_L_FROM_ADDR
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }case 4:{
    INSTRUCTION_TIME_ROUND(8);
    m68k_src_l=m68k_fetchL();
    pc+=4;
    break;
  }default:
    ILLEGAL;
  }
}

#endif//!SS-CPU


#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU)) // moved to SSECpu.cpp
// forward (temp, structure!)

void m68k_get_dest_000_b();
void m68k_get_dest_000_w();
void m68k_get_dest_000_l();
void m68k_get_dest_001_b();
void m68k_get_dest_001_w();
void m68k_get_dest_001_l();
void m68k_get_dest_010_b();
void m68k_get_dest_010_w();
void m68k_get_dest_010_l();
void m68k_get_dest_011_b();
void m68k_get_dest_011_w();
void m68k_get_dest_011_l();
void m68k_get_dest_100_b();
void m68k_get_dest_100_w();
void m68k_get_dest_100_l();
void m68k_get_dest_101_b();
void m68k_get_dest_101_w();
void m68k_get_dest_101_l();
void m68k_get_dest_110_b();
void m68k_get_dest_110_w();
void m68k_get_dest_110_l();
void m68k_get_dest_111_b();
void m68k_get_dest_111_w();
void m68k_get_dest_111_l();
#if !defined(SSE_CPU_ROUNDING_NO_FASTER_FOR_D)
void m68k_get_dest_000_b_faster();
void m68k_get_dest_000_w_faster();
void m68k_get_dest_000_l_faster();
void m68k_get_dest_001_b_faster();
void m68k_get_dest_001_w_faster();
void m68k_get_dest_001_l_faster();
void m68k_get_dest_010_b_faster();
void m68k_get_dest_010_w_faster();
void m68k_get_dest_010_l_faster();
void m68k_get_dest_011_b_faster();
void m68k_get_dest_011_w_faster();
void m68k_get_dest_011_l_faster();
void m68k_get_dest_100_b_faster();
void m68k_get_dest_100_w_faster();
void m68k_get_dest_100_l_faster();
void m68k_get_dest_101_b_faster();
void m68k_get_dest_101_w_faster();
void m68k_get_dest_101_l_faster();
void m68k_get_dest_110_b_faster();
void m68k_get_dest_110_w_faster();
void m68k_get_dest_110_l_faster();
void m68k_get_dest_111_b_faster();
void m68k_get_dest_111_w_faster();
void m68k_get_dest_111_l_faster();
#endif

#else
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//////////////////////////    GET DEST       ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void m68k_get_dest_000_b(){ m68k_dest=r+PARAM_M; }
void m68k_get_dest_000_w(){ m68k_dest=r+PARAM_M; }
void m68k_get_dest_000_l(){ m68k_dest=r+PARAM_M; }
void m68k_get_dest_001_b(){ m68k_dest=areg+PARAM_M; }
void m68k_get_dest_001_w(){ m68k_dest=areg+PARAM_M; }
void m68k_get_dest_001_l(){ m68k_dest=areg+PARAM_M; }
void m68k_get_dest_010_b(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_B(areg[PARAM_M]); }
void m68k_get_dest_010_w(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_W(areg[PARAM_M]); }
void m68k_get_dest_010_l(){ INSTRUCTION_TIME_ROUND(8); m68k_SET_DEST_L(areg[PARAM_M]); }
void m68k_get_dest_011_b(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_B(areg[PARAM_M]); areg[PARAM_M]+=1; if(PARAM_M==7)areg[7]++;}
void m68k_get_dest_011_w(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_W(areg[PARAM_M]); areg[PARAM_M]+=2; }
void m68k_get_dest_011_l(){ INSTRUCTION_TIME_ROUND(8); m68k_SET_DEST_L(areg[PARAM_M]); areg[PARAM_M]+=4; }
void m68k_get_dest_100_b(){ INSTRUCTION_TIME_ROUND(6); areg[PARAM_M]-=1; if(PARAM_M==7)areg[7]--; m68k_SET_DEST_B(areg[PARAM_M]); }
void m68k_get_dest_100_w(){ INSTRUCTION_TIME_ROUND(6); areg[PARAM_M]-=2; m68k_SET_DEST_W(areg[PARAM_M]); }
void m68k_get_dest_100_l(){ INSTRUCTION_TIME_ROUND(10); areg[PARAM_M]-=4; m68k_SET_DEST_L(areg[PARAM_M]); }

void m68k_get_dest_101_b(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_B(areg[PARAM_M]+fw);
}
void m68k_get_dest_101_w(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_W(areg[PARAM_M]+fw);
}
void m68k_get_dest_101_l(){
  INSTRUCTION_TIME_ROUND(12);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_L(areg[PARAM_M]+fw);
}
void m68k_get_dest_110_b(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_B_TO_ADDR
}
void m68k_get_dest_110_w(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_W_TO_ADDR
}
void m68k_get_dest_110_l(){
  INSTRUCTION_TIME_ROUND(14);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_L_TO_ADDR
}
void m68k_get_dest_111_b(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    m68k_SET_DEST_B(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    m68k_SET_DEST_B(fw)
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_dest_111_w(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    m68k_SET_DEST_W(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    m68k_SET_DEST_W(fw)
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_dest_111_l(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(12);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    m68k_SET_DEST_L(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    m68k_SET_DEST_L(fw)
    break;
  }default:
    ILLEGAL;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//////////////////////////    GET DEST       ///////////////////////////////////
//////////////////////////  faster for D,    ///////////////////////////////////
//////////////////////////  extra fetch for  ///////////////////////////////////
//////////////////////////  others           ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*  SS: EXTRA_PREFETCH has been deactivated, it's being taken care of for
    all instructions now, by PREFETCH_IRC
    The negative timings correct extra timing added in instructions like ORI,
    it seems it was to spare some code?
*/

void m68k_get_dest_000_b_faster(){ INSTRUCTION_TIME(-4); m68k_dest=r+PARAM_M; }
void m68k_get_dest_000_w_faster(){ INSTRUCTION_TIME(-4); m68k_dest=r+PARAM_M; }
void m68k_get_dest_000_l_faster(){ INSTRUCTION_TIME(-4); m68k_dest=r+PARAM_M; }
void m68k_get_dest_001_b_faster(){ m68k_dest=areg+PARAM_M; EXTRA_PREFETCH}
void m68k_get_dest_001_w_faster(){ m68k_dest=areg+PARAM_M; EXTRA_PREFETCH}
void m68k_get_dest_001_l_faster(){ m68k_dest=areg+PARAM_M; EXTRA_PREFETCH}
void m68k_get_dest_010_b_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_B(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_010_w_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_W(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_010_l_faster(){ INSTRUCTION_TIME_ROUND(8); m68k_SET_DEST_L(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_011_b_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_B(areg[PARAM_M]); areg[PARAM_M]+=1; if(PARAM_M==7)areg[7]++; EXTRA_PREFETCH}
void m68k_get_dest_011_w_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_W(areg[PARAM_M]); areg[PARAM_M]+=2; EXTRA_PREFETCH}
void m68k_get_dest_011_l_faster(){ INSTRUCTION_TIME_ROUND(8); m68k_SET_DEST_L(areg[PARAM_M]); areg[PARAM_M]+=4; EXTRA_PREFETCH}
void m68k_get_dest_100_b_faster(){ INSTRUCTION_TIME_ROUND(6); areg[PARAM_M]-=1; if(PARAM_M==7)areg[7]--; m68k_SET_DEST_B(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_100_w_faster(){ INSTRUCTION_TIME_ROUND(6); areg[PARAM_M]-=2; m68k_SET_DEST_W(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_100_l_faster(){ INSTRUCTION_TIME_ROUND(10); areg[PARAM_M]-=4; m68k_SET_DEST_L(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_101_b_faster(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_B(areg[PARAM_M]+fw);
  EXTRA_PREFETCH
}
void m68k_get_dest_101_w_faster(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_W(areg[PARAM_M]+fw);
  EXTRA_PREFETCH
}
void m68k_get_dest_101_l_faster(){
  INSTRUCTION_TIME_ROUND(12);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_L(areg[PARAM_M]+fw);
  EXTRA_PREFETCH
}
void m68k_get_dest_110_b_faster(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_B_TO_ADDR
  EXTRA_PREFETCH
}
void m68k_get_dest_110_w_faster(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_W_TO_ADDR
  EXTRA_PREFETCH
}
void m68k_get_dest_110_l_faster(){
  INSTRUCTION_TIME_ROUND(14);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_L_TO_ADDR
  EXTRA_PREFETCH
}
void m68k_get_dest_111_b_faster(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    m68k_SET_DEST_B(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    m68k_SET_DEST_B(fw)
    EXTRA_PREFETCH
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_dest_111_w_faster(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    m68k_SET_DEST_W(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    m68k_SET_DEST_W(fw)
    EXTRA_PREFETCH
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_dest_111_l_faster(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(12);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    m68k_SET_DEST_L(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    m68k_SET_DEST_L(fw)
    EXTRA_PREFETCH
    break;
  }default:
    ILLEGAL;
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////        CONDITION TESTS        //////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



bool m68k_condition_test_t(){return true;}
bool m68k_condition_test_f(){return false;}
bool m68k_condition_test_hi(){return ((sr&(SR_C+SR_Z))==0);}
bool m68k_condition_test_ls(){return (bool)((sr&(SR_C+SR_Z)));}
bool m68k_condition_test_cc(){return ((sr&SR_C)==0);}
bool m68k_condition_test_cs(){return (bool)(sr&SR_C);}
bool m68k_condition_test_ne(){return ((sr&SR_Z)==0);}
bool m68k_condition_test_eq(){return (bool)(sr&SR_Z);}
bool m68k_condition_test_vc(){return ((sr&SR_V)==0);}
bool m68k_condition_test_vs(){return (bool)(sr&SR_V);}
bool m68k_condition_test_pl(){return ((sr&SR_N)==0);}
bool m68k_condition_test_mi(){return (bool)(sr&SR_N);}
bool m68k_condition_test_ge(){return ((sr&(SR_N+SR_V))==0 || (sr&(SR_N+SR_V))==(SR_N+SR_V));}
bool m68k_condition_test_lt(){return ((sr&(SR_N+SR_V))==SR_V || (sr&(SR_N+SR_V))==SR_N);}
bool m68k_condition_test_gt(){return (!(sr&SR_Z) && ( ((sr&(SR_N+SR_V))==0) || ((sr&(SR_N+SR_V))==SR_N+SR_V) ));}
bool m68k_condition_test_le(){return ((sr&SR_Z) || ( ((sr&(SR_N+SR_V))==SR_N) || ((sr&(SR_N+SR_V))==SR_V) ));}


//TODO make sure we have other macros than IT() when reading/writing on the bus
// that way it will be easier to refactor (count timing at R/W)


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////  LINE-0 IMMEDIATE ROUTINES    //////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


/*
-------------------------------------------------------------------------------
 EORI, ORI, ANDI, |    Exec Time    |               Data Bus Usage
    SUBI, ADDI    |  INSTR     EA   | 1st Operand |  2nd OP (ea)  |   INSTR
------------------+-----------------+-------------+---------------+------------
#<data>,<ea> :    |                 |             |               |
  .B or .W :      |                 |             |               |
    Dn            |  8(2/0)  0(0/0) |          np |               | np          
    (An)          | 12(2/1)  4(1/0) |          np |            nr | np nw	      
    (An)+         | 12(2/1)  4(1/0) |          np |            nr | np nw	      
    -(An)         | 12(2/1)  6(1/0) |          np | n          nr | np nw	      
    (d16,An)      | 12(2/1)  8(2/0) |          np |      np    nr | np nw	      
    (d8,An,Xn)    | 12(2/1) 10(2/0) |          np | n    np    nr | np nw	      
    (xxx).W       | 12(2/1)  8(2/0) |          np |      np    nr | np nw	      
    (xxx).L       | 12(2/1) 12(3/0) |          np |   np np    nr | np nw	      
  .L :                              |             |               |
    Dn            | 16(3/0)  0(0/0) |       np np |               | np       nn	
    (An)          | 20(3/2)  8(2/0) |       np np |         nR nr | np nw nW    
    (An)+         | 20(3/2)  8(2/0) |       np np |         nR nr | np nw nW    
    -(An)         | 20(3/2) 10(2/0) |       np np | n       nR nr | np nw nW    
    (d16,An)      | 20(3/2) 12(3/0) |       np np |      np nR nr | np nw nW    
    (d8,An,Xn)    | 20(3/2) 14(3/0) |       np np | n    np nR nr | np nw nW    
    (xxx).W       | 20(3/2) 12(3/0) |       np np |      np nR nr | np nw nW    
    (xxx).L       | 20(3/2) 16(4/0) |       np np |   np np nR nr | np nw nW    

-------------------------------------------------------------------------------
 ORI, ANDI, EORI  |    Exec Time    |               Data Bus Usage
  to CCR, to SR   |      INSTR      | 1st Operand |          INSTR
------------------+-----------------+-------------+----------------------------
#<data>,CCR       |                 |             |               
  .B :            | 20(3/0)         |          np |              nn nn np np   
*/

void                              m68k_ori_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
  if ((ir & B6_111111)==B6_111100){  //to sr
#if !defined(SSE_CPU_ROUNDING_ORI)
    INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
    CPU_ABUS_ACCESS_READ;//? respecting Yacht
    INSTRUCTION_TIME(8);
    CPU_ABUS_ACCESS_READ_FETCH; 
#endif
    CCR|=m68k_IMMEDIATE_B;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
    sr&=SR_VALID_BITMASK;
    pc+=2; 
  }else{
#if !defined(SSE_CPU_ROUNDING_ORI)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_GET_IMMEDIATE_B;
#if !defined(SSE_CPU_ROUNDING_ORI)
    m68k_GET_DEST_B_NOT_A_FASTER_FOR_D; // SS this compensates the 8
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
    m68k_GET_DEST_B_NOT_A;
#endif
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
    if(!DEST_IS_DATA_REGISTER)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B|=m68k_src_b;//ss we'll have a new macro operation + timing?
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_B
  }
}
void                              m68k_ori_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

  if ((ir & B6_111111)==B6_111100){  //to sr
    if (SUPERFLAG){
#if !defined(SSE_CPU_ROUNDING_ORI)
      INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
      CPU_ABUS_ACCESS_READ;//? respecting Yacht
      INSTRUCTION_TIME(8);
      CPU_ABUS_ACCESS_READ_FETCH; 
#endif
      sr|=m68k_IMMEDIATE_W;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
      sr&=SR_VALID_BITMASK;
      pc+=2; 
      DETECT_TRACE_BIT;
    }else{
      exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
    }
  }else{
#if !defined(SSE_CPU_ROUNDING_ORI)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_GET_IMMEDIATE_W;
#if !defined(SSE_CPU_ROUNDING_ORI)
    m68k_GET_DEST_W_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
    m68k_GET_DEST_W_NOT_A;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#if defined(SSE_CPU_ROUNDING_ORI)
    if(!DEST_IS_DATA_REGISTER)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W|=m68k_src_w;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_W
  }
}

void                              m68k_ori_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_ORI)
  INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
  CPU_ABUS_ACCESS_READ_FETCH;
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_L;
#if !defined(SSE_CPU_ROUNDING_ORI)
  m68k_GET_DEST_L_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
  m68k_GET_DEST_L_NOT_A;
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ORI)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4);
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_L|=m68k_src_l;
  SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
  SR_CHECK_Z_AND_N_L
}

void                              m68k_andi_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

  if((ir&B6_111111)==B6_111100){  //to sr
#if !defined(SSE_CPU_ROUNDING_ANDI)
    INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    CPU_ABUS_ACCESS_READ;//? respecting Yacht
    INSTRUCTION_TIME(8);
    CPU_ABUS_ACCESS_READ_FETCH; 
#endif
    CCR&=m68k_IMMEDIATE_B;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    pc+=2; 
  }else{
#if !defined(SSE_CPU_ROUNDING_ANDI)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_GET_IMMEDIATE_B;
#if defined(SSE_CPU_DATABUS)
    M68000.dbus|=m68k_src_b;//?
#endif
#if !defined(SSE_CPU_ROUNDING_ANDI)
    m68k_GET_DEST_B_NOT_A_FASTER_FOR_D; // SS this compensates the 8
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    m68k_GET_DEST_B_NOT_A;
#endif
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    if(!DEST_IS_DATA_REGISTER)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B&=m68k_src_b;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_B
  }
}
void                              m68k_andi_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

  if((ir&B6_111111)==B6_111100){  //to sr
    if(SUPERFLAG){
      DEBUG_ONLY( int debug_old_sr=sr; )
      INSTRUCTION_TIME(16);
      sr&=m68k_IMMEDIATE_W;
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      DETECT_CHANGE_TO_USER_MODE;
      pc+=2; 
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
      //SS was commented out:
//    check_for_interrupts_pending(); //in case we've lowered the IPL level

      CHECK_STOP_ON_USER_CHANGE
    }else exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }else{
#if !defined(SSE_CPU_ROUNDING_ANDI)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_GET_IMMEDIATE_W;
#if !defined(SSE_CPU_ROUNDING_ANDI)
    m68k_GET_DEST_W_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    m68k_GET_DEST_W_NOT_A;
#endif
#if defined(SSE_CPU_DATABUS)
    M68000.dbus=m68k_src_w;
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
    if(!DEST_IS_DATA_REGISTER)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W&=m68k_src_w;
#if !defined(SSE_CPU_ROUNDING_ANDI)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#endif
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_W
  }
}
void                              m68k_andi_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

#if !defined(SSE_CPU_ROUNDING_ANDI)
  INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
  CPU_ABUS_ACCESS_READ_FETCH;
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_L;

#if defined(SSE_CPU_DATABUS)
    M68000.dbus=m68k_src_l&0xFFFF;
#endif
#if !defined(SSE_CPU_ROUNDING_ANDI)
  m68k_GET_DEST_L_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
  m68k_GET_DEST_L_NOT_A;
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ANDI)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4);
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_L&=m68k_src_l;
  SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
  SR_CHECK_Z_AND_N_L
}
void                              m68k_subi_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_SUBI)
  INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_B;
#if !defined(SSE_CPU_ROUNDING_SUBI)
  m68k_GET_DEST_B_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  m68k_GET_DEST_B_NOT_A;
#endif
  PREFETCH_IRC;
//  SR_CLEAR(SR_USER_BYTE); //SS was commented out
//  if((unsigned char)m68k_IMMEDIATE_B>(unsigned char)m68k_DEST_B)SR_SET(SR_C+SR_X); // was commented out
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  if(!DEST_IS_DATA_REGISTER)
    CPU_ABUS_ACCESS_WRITE;
#endif
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(SR_X);
  //SS was commented out:
  /*
  int wasnegative=0; // I add
  if(m68k_DEST_B&0x80){
    SR_SET(SR_N);
    wasnegative++;
  }else{
    if(wasnegative){
      SR_SET(SR_V);
    }
    if(m68k_DEST_B==0){
      SR_SET(SR_Z);
    }
  }
  */
}
void                              m68k_subi_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_SUBI)
  INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_W;
#if !defined(SSE_CPU_ROUNDING_SUBI)
  m68k_GET_DEST_W_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  m68k_GET_DEST_W_NOT_A;
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  if(!DEST_IS_DATA_REGISTER)
    CPU_ABUS_ACCESS_WRITE;
#endif
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(SR_X);
}

void                              m68k_subi_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

#if !defined(SSE_CPU_ROUNDING_SUBI)
  INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  CPU_ABUS_ACCESS_READ_FETCH;
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_L;
#if !defined(SSE_CPU_ROUNDING_SUBI)
  m68k_GET_DEST_L_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  m68k_GET_DEST_L_NOT_A;
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_SUBI)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4); 
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(SR_X);
}
void                              m68k_addi_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_ADDI)
  INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_B;
#if !defined(SSE_CPU_ROUNDING_ADDI)
  m68k_GET_DEST_B_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  m68k_GET_DEST_B_NOT_A;
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  if(!DEST_IS_DATA_REGISTER)
    CPU_ABUS_ACCESS_WRITE;
#endif
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B+=m68k_src_b;
  SR_ADD_B;
}
void                              m68k_addi_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_ADDI)
  INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_W;
#if !defined(SSE_CPU_ROUNDING_ADDI)
  m68k_GET_DEST_W_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  m68k_GET_DEST_W_NOT_A;
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  if(!DEST_IS_DATA_REGISTER)
    CPU_ABUS_ACCESS_WRITE;
#endif
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W+=m68k_src_w;
  SR_ADD_W;
}



void                              m68k_addi_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

#if !defined(SSE_CPU_ROUNDING_ADDI)
  INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  CPU_ABUS_ACCESS_READ_FETCH;
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_L;
#if !defined(SSE_CPU_ROUNDING_ADDI)
  m68k_GET_DEST_L_NOT_A_FASTER_FOR_D; //SS this removes 4 cycles
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  m68k_GET_DEST_L_NOT_A;
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDI)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4);
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif

  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L+=m68k_src_l;
  SR_ADD_L;
}



void                              m68k_btst(){
  //SS m68k_jump_line_0[B6_100000]
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_BTST)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_B;
  if ((ir&BITS_543)==BITS_543_000){
/*
#<data>,Dn :      |                 |             |               |
  .L :            | 10(2/0)  0(0/0) |          np |               | np n     
*/
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#if !defined(SSE_CPU_ROUNDING_BTST)
    INSTRUCTION_TIME(6); 
#endif
#if defined(SSE_CPU_ROUNDING_BTST)
    INSTRUCTION_TIME(2);
#endif
    m68k_src_b&=31;
    if((r[PARAM_M]>>m68k_src_b)&1){
      SR_CLEAR(SR_Z);
    }else{
      SR_SET(SR_Z);
    }
  }else{
/*
#<data>,<ea> :    |                 |             |               |             
  .B :            |                 |             |               |
    (An)          |  8(2/0)  4(1/0) |          np |            nr | np          
    (An)+         |  8(2/0)  4(1/0) |          np |            nr | np          
    -(An)         |  8(2/0)  6(1/0) |          np | n          nr | np          
    (d16,An)      |  8(2/0)  8(2/0) |          np |      np    nr | np          
    (d8,An,Xn)    |  8(2/0) 10(2/0) |          np | n    np    nr | np          
    (xxx).W       |  8(2/0)  8(2/0) |          np |      np    nr | np          
    (xxx).L       |  8(2/0) 12(3/0) |          np |   np np    nr | np          
*/
#if !defined(SSE_CPU_ROUNDING_BTST)
    INSTRUCTION_TIME(4);
#endif
    m68k_ap=(short)(m68k_src_b & 7);
//    m68k_GET_DEST_B_NOT_A; // was commented out
    if((ir&(BIT_5+BIT_4+BIT_3+BIT_2+BIT_1+BIT_0))==B6_111100){  //immediate mode is the only one not allowed -
      m68k_unrecognised(); //SS TODO correct timing... (in general)
    }else{
      m68k_GET_SOURCE_B_NOT_A;
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
      if((m68k_src_b>>m68k_ap)&1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
    }
  }
}

void                              m68k_bchg(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

#if defined(SSE_CPU_ROUNDING_BCHG)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_B; //SS this doesn't count any cycles

  if((ir&BITS_543)==BITS_543_000){ //SS on DN
/*
#<data>,Dn :      |                 |             |               |
  .L :            |                 |             |               |
    if data<16    | 10(2/0)  0(0/0) |          np |               | np       n  
    if data>15    | 12(2/0)  0(0/0) |          np |               | np       nn 
*/

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC; 
    m68k_src_b&=31; //SS bit 0-31
#if defined(SSE_CPU_ROUNDING_BCHG)
    if (m68k_src_b>15){
      INSTRUCTION_TIME(4); //MAXIMUM VALUE
    }else{
      INSTRUCTION_TIME(2);
    }
#else
    if (m68k_src_b>=16){
      INSTRUCTION_TIME(8); //MAXIMUM VALUE
    }else{
      INSTRUCTION_TIME(6);
    }
#endif
    m68k_src_l=1<<m68k_src_b;
    if(r[PARAM_M]&m68k_src_l){
      SR_CLEAR(SR_Z);
    }else{
      SR_SET(SR_Z);
    }
    r[PARAM_M]^=m68k_src_l;
  }else{
/*
#<data>,<ea> :    |                 |             |               |
  .B :            |                 |             |               |
    (An)          | 12(2/1)  4(1/0) |          np |            nr | np    nw    
    (An)+         | 12(2/1)  4(1/0) |          np |            nr | np    nw    
    -(An)         | 12(2/1)  6(1/0) |          np | n          nr | np    nw    
    (d16,An)      | 12(2/1)  8(2/0) |          np |      np    nr | np    nw    
    (d8,An,Xn)    | 12(2/1) 10(2/0) |          np | n    np    nr | np    nw    
    (xxx).W       | 12(2/1)  8(2/0) |          np |      np    nr | np    nw    
    (xxx).L       | 12(2/1) 12(3/0) |          np |   np np    nr | np    nw    
*/
#if !defined(SSE_CPU_ROUNDING_BCHG)
    INSTRUCTION_TIME(8);
#endif
    m68k_src_b&=7;
    m68k_GET_DEST_B_NOT_A;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
    m68k_src_b=(BYTE)(1<<m68k_src_b);
    if(m68k_DEST_B&m68k_src_b){
      SR_CLEAR(SR_Z);
    }else{
      SR_SET(SR_Z);
    }
#if defined(SSE_CPU_ROUNDING_BCHG)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B^=(BYTE)m68k_src_b;
  }
}


//#undef SSE_CPU_LINE_0_TIMINGS

void                              m68k_bclr(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_BCLR)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_B;
  if((ir&BITS_543)==BITS_543_000){
/*
#<data>,Dn :      |                 |             |               |
  .L :            |                 |             |               |
    if data<16    | 12(2/0)  0(0/0) |          np |               | np nn       
    if data>15    | 14(2/0)  0(0/0) |          np |               | np nn n     
*/
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    m68k_src_b&=31;
#if defined(SSE_CPU_ROUNDING_BCLR)
    if (m68k_src_b>=16){
      INSTRUCTION_TIME(6);
    }else{
      INSTRUCTION_TIME(4);
    }
#else
    if (m68k_src_b>=16){
      INSTRUCTION_TIME(10); //MAXIMUM VALUE
    }else{
      INSTRUCTION_TIME(8);
    }
#endif
    m68k_src_l=1<<m68k_src_b;
    if(r[PARAM_M]&m68k_src_l){
      SR_CLEAR(SR_Z);
    }else{
      SR_SET(SR_Z);
    }
    r[PARAM_M]&=~m68k_src_l;
  }else{
/*
#<data>,<ea> :    |                 |             |               |
  .B :            |                 |             |               |
    (An)          | 12(2/1)  4(1/0) |          np |            nr | np    nw    
    (An)+         | 12(2/1)  4(1/0) |          np |            nr | np    nw    
    -(An)         | 12(2/1)  6(1/0) |          np | n          nr | np    nw    
    (d16,An)      | 12(2/1)  8(2/0) |          np |      np    nr | np    nw    
    (d8,An,Xn)    | 12(2/1) 10(2/0) |          np | n    np    nr | np    nw    
    (xxx).W       | 12(2/1)  8(2/0) |          np |      np    nr | np    nw    
    (xxx).L       | 12(2/1) 12(3/0) |          np |   np np    nr | np    nw    
*/
#if !defined(SSE_CPU_ROUNDING_BCLR)
    INSTRUCTION_TIME(8); //SS get immediate + write result
#endif
    m68k_src_b&=7;
    m68k_GET_DEST_B_NOT_A;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    m68k_src_b=(BYTE)(1<<m68k_src_b);
    if(m68k_DEST_B&m68k_src_b){
      SR_CLEAR(SR_Z);
    }else{
      SR_SET(SR_Z);
    }
#if defined(SSE_CPU_ROUNDING_BCLR)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B&=(BYTE)(~m68k_src_b);
  }
}

void                              m68k_bset(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_BSET)
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_B;
  if ((ir&BITS_543)==BITS_543_000){ //SS to Dn
/*
#<data>,Dn :      |                 |             |               |
  .L :            |                 |             |               |
    if data<16    | 10(2/0)  0(0/0) |          np |               | np       n  
    if data>15    | 12(2/0)  0(0/0) |          np |               | np       nn 
*/
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
    m68k_src_b&=31;
#if defined(SSE_CPU_ROUNDING_BSET)
    if (m68k_src_b>15){
      INSTRUCTION_TIME(4); 
    }else{
      INSTRUCTION_TIME(2);
    }
#else
    if (m68k_src_b>=16){
      INSTRUCTION_TIME(8); //MAXIMUM VALUE
    }else{
      INSTRUCTION_TIME(6);
    }
#endif
    m68k_src_l=1 << m68k_src_b;
    if (r[PARAM_M] & m68k_src_l){
      SR_CLEAR(SR_Z);
    }else{
      SR_SET(SR_Z);
    }
    r[PARAM_M]|=m68k_src_l;
  }else{
/*
#<data>,<ea> :    |                 |             |               |
  .B :            |                 |             |               |
    (An)          | 12(2/1)  4(1/0) |          np |            nr | np    nw    
    (An)+         | 12(2/1)  4(1/0) |          np |            nr | np    nw    
    -(An)         | 12(2/1)  6(1/0) |          np | n          nr | np    nw    
    (d16,An)      | 12(2/1)  8(2/0) |          np |      np    nr | np    nw    
    (d8,An,Xn)    | 12(2/1) 10(2/0) |          np | n    np    nr | np    nw    
    (xxx).W       | 12(2/1)  8(2/0) |          np |      np    nr | np    nw    
    (xxx).L       | 12(2/1) 12(3/0) |          np |   np np    nr | np    nw    
*/
#if !defined(SSE_CPU_ROUNDING_BSET)
    INSTRUCTION_TIME(8); //SS includes fetch immediate, write result
#endif
    m68k_src_b&=7;
    m68k_GET_DEST_B_NOT_A;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
    m68k_src_b=(BYTE)(1<<m68k_src_b);
    if(m68k_DEST_B&m68k_src_b){
      SR_CLEAR(SR_Z);
    }else{
      SR_SET(SR_Z);
    }
#if defined(SSE_CPU_ROUNDING_BSET)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B|=(BYTE)m68k_src_b;
  }
}

void                              m68k_eori_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
  if((ir&B6_111111)==B6_111100){  //to sr //(ccr)
#if !defined(SSE_CPU_ROUNDING_EORI)
    INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
    CPU_ABUS_ACCESS_READ;//? respecting Yacht
    INSTRUCTION_TIME(8);
    CPU_ABUS_ACCESS_READ_FETCH; 
#endif
    CCR^=m68k_IMMEDIATE_B;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    sr&=SR_VALID_BITMASK;
    pc+=2; 
  }else{
#if !defined(SSE_CPU_ROUNDING_EORI)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_GET_IMMEDIATE_B;
#if !defined(SSE_CPU_ROUNDING_EORI)
    m68k_GET_DEST_B_NOT_A_FASTER_FOR_D; // SS this compensates the 8
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
    m68k_GET_DEST_B_NOT_A;
#endif
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
    if(!DEST_IS_DATA_REGISTER)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B^=m68k_src_b;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_B
  }
}
//#undef SSE_CPU_ROUNDING_EORI
void                              m68k_eori_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
  if((ir&B6_111111)==B6_111100){  //to sr
    if(SUPERFLAG){
      DEBUG_ONLY( int debug_old_sr=sr; )
#if !defined(SSE_CPU_ROUNDING_EORI)
      INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
      CPU_ABUS_ACCESS_READ;//? respecting Yacht
      INSTRUCTION_TIME(8);
      CPU_ABUS_ACCESS_READ_FETCH; 
#endif
      sr^=m68k_IMMEDIATE_W;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
      sr&=SR_VALID_BITMASK;
      pc+=2; 
      DETECT_CHANGE_TO_USER_MODE
      DETECT_TRACE_BIT;
      // Interrupts must come after trace exception
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
//      check_for_interrupts_pending(); // was commented out

      CHECK_STOP_ON_USER_CHANGE;
    }else exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
//#undef SSE_CPU_ROUNDING_EORI
  else{
#if !defined(SSE_CPU_ROUNDING_EORI)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
    CPU_ABUS_ACCESS_READ_FETCH; //oops, bug seen thanks to lowlife60
#endif
    m68k_GET_IMMEDIATE_W;
#if !defined(SSE_CPU_ROUNDING_EORI)
    m68k_GET_DEST_W_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
    m68k_GET_DEST_W_NOT_A;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#if defined(SSE_CPU_ROUNDING_EORI)
    if(!DEST_IS_DATA_REGISTER)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W^=m68k_src_w;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_W;
  }
}
//#undef SSE_CPU_LINE_0_TIMINGS
//#undef SSE_CPU_ROUNDING_EORI
void                              m68k_eori_l(){
/*
  .L :                              |             |               |
    Dn            | 16(3/0)  0(0/0) |       np np |               | np       nn	
    (An)          | 20(3/2)  8(2/0) |       np np |         nR nr | np nw nW    
    (An)+         | 20(3/2)  8(2/0) |       np np |         nR nr | np nw nW    
    -(An)         | 20(3/2) 10(2/0) |       np np | n       nR nr | np nw nW    
    (d16,An)      | 20(3/2) 12(3/0) |       np np |      np nR nr | np nw nW    
    (d8,An,Xn)    | 20(3/2) 14(3/0) |       np np | n    np nR nr | np nw nW    
    (xxx).W       | 20(3/2) 12(3/0) |       np np |      np nR nr | np nw nW    
    (xxx).L       | 20(3/2) 16(4/0) |       np np |   np np nR nr | np nw nW    
*/
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

#if !defined(SSE_CPU_ROUNDING_EORI)
  INSTRUCTION_TIME(16);
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
  CPU_ABUS_ACCESS_READ_FETCH;
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_L;
#if !defined(SSE_CPU_ROUNDING_EORI)
  m68k_GET_DEST_L_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_EORI)
  m68k_GET_DEST_L_NOT_A;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#if defined(SSE_CPU_ROUNDING_EORI)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4);
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_L^=m68k_src_l;
  SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
  SR_CHECK_Z_AND_N_L;
}

/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       CMPI       |  INSTR     EA   | 1st Operand |  2nd OP (ea)  |   INSTR
------------------+-----------------+-------------+-------------+--------------
#<data>,<ea> :    |                 |             |               |
  .B or .W :      |                 |             |               |
    Dn            |  8(2/0)  0(0/0) |          np |               | np          
    (An)          |  8(2/0)  4(1/0) |          np |            nr | np          
    (An)+         |  8(2/0)  4(1/0) |          np |            nr | np          
    -(An)         |  8(2/0)  6(1/0) |          np | n          nr | np          
    (d16,An)      |  8(2/0)  8(2/0) |          np |      np    nr | np          
    (d8,An,Xn)    |  8(2/0) 10(2/0) |          np | n    np    nr | np          
    (xxx).W       |  8(2/0)  8(2/0) |          np |      np    nr | np          
    (xxx).L       |  8(2/0) 12(3/0) |          np |   np np    nr | np          
*/
//#undef SSE_CPU_LINE_0_TIMINGS
void                              m68k_cmpi_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_CMPI)
  CPU_ABUS_ACCESS_READ_FETCH;
#else
  INSTRUCTION_TIME(4);
#endif
  m68k_GET_IMMEDIATE_B;
  m68k_old_dest=m68k_read_dest_b();
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(0);
}

void                              m68k_cmpi_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_CMPI)
  CPU_ABUS_ACCESS_READ_FETCH;
#else
  INSTRUCTION_TIME(4);
#endif
  m68k_GET_IMMEDIATE_W; //SS this doesn't count cycles
  m68k_old_dest=m68k_read_dest_w();
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(0);
}
//#undef SSE_CPU_LINE_0_TIMINGS

void                              m68k_cmpi_l(){
/*
  .L :            |                 |             |               |
    Dn            | 14(3/0)  0(0/0) |       np np |               | np       n  
    (An)          | 12(3/0)  8(2/0) |       np np |         nR nr | np          
    (An)+         | 12(3/0)  8(2/0) |       np np |         nR nr | np          
    -(An)         | 12(3/0) 10(2/0) |       np np | n       nR nr | np          
    (d16,An)      | 12(3/0) 12(3/0) |       np np |      np nR nr | np          
    (d8,An,Xn)    | 12(3/0) 14(3/0) |       np np | n    np nR nr | np          
    (xxx).W       | 12(3/0) 12(3/0) |       np np |      np nR nr | np          
    (xxx).L       | 12(3/0) 16(4/0) |       np np |   np np nR nr | np       
*/
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_CMPI_L) && defined(SSE_CPU_ROUNDING_CMPI_L2)
  CPU_ABUS_ACCESS_READ_FETCH;
  CPU_ABUS_ACCESS_READ_FETCH;
#endif
  m68k_GET_IMMEDIATE_L; //SS this doesn't count cycles
#if !defined(SSE_CPU_ROUNDING_CMPI_L)
  if(DEST_IS_REGISTER){INSTRUCTION_TIME(10);} else {INSTRUCTION_TIME(8);}
#endif
#if defined(SSE_CPU_ROUNDING_CMPI_L) && !defined(SSE_CPU_ROUNDING_CMPI_L2)
  INSTRUCTION_TIME_ROUND(8); // for immediate
#endif
  m68k_old_dest=m68k_read_dest_l(); //SS this counts cycles
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#if defined(SSE_CPU_ROUNDING_CMPI_L)
/* Japtro disk B overscan dot balls, we were rounding 14 to 16 in v3.5
*/
  if(DEST_IS_REGISTER){INSTRUCTION_TIME(2);}
#endif
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
}


//#define SSE_CPU_LINE_0_TIMINGS
void                              m68k_movep_w_to_dN_or_btst(){

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
  if((ir&BITS_543)==BITS_543_001){ //SS MOVEP
    
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
      MOVEP       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
(d16,Ay),Dx :     |                 |
  .W :            | 16(4/0)         |                np    nR    nr np          
  .L :            | 24(6/0)         |                np nR nR nr nr np          
*/

    
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1;
#endif
#if defined(SSE_CPU_ROUNDING_MOVEP_MR_W)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    MEM_ADDRESS addr=areg[PARAM_M]+(signed short)m68k_fetchW();
#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_W)
    INSTRUCTION_TIME(4);
#endif
    pc+=2; 
#if defined(SSE_CPU_ROUNDING_MOVEP_MR_W)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_READ_B(addr);
    DWORD_B_1(&r[PARAM_N])=m68k_src_b; //high byte
#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_W)
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_ROUNDING_MOVEP_MR_W)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_READ_B(addr+2);
    DWORD_B_0(&r[PARAM_N])=m68k_src_b; //low byte
#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_W)
    INSTRUCTION_TIME(4);
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
// was commented out:
//    *( ((BYTE*)(&r[PARAM_N])) +1)=m68k_src_b; //high byte
//    m68k_READ_B(addr+2);
//    *( ((BYTE*)(&r[PARAM_N]))   )=m68k_src_b; //low byte
  }else{

    if ((ir&BITS_543)==BITS_543_000){  //btst to data register
      INSTRUCTION_TIME(2);
        PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif

      if ((r[PARAM_M] >> (31 & r[PARAM_N])) & 1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
    }else{ // btst memory
      m68k_GET_SOURCE_B_NOT_A;   //even immediate mode is allowed!!!!
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
  FETCH_TIMING;
#endif
      if( (m68k_src_b >> (7 & r[PARAM_N])) & 1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
    }
  }
}

void                              m68k_movep_l_to_dN_or_bchg(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

  if((ir&BITS_543)==BITS_543_001){ //SS MOVEP
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1;
#endif

#if defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    MEM_ADDRESS addr=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 

#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    CPU_ABUS_ACCESS_READ;
#endif
    ///// Should handle blitter here, blitter would start 1 word after busy is set
    //SS not here, in "to memory"
    m68k_READ_B(addr)//ss problem with putting timing there, sometimes it shouldn't be there
    DWORD_B_3(&r[PARAM_N])=m68k_src_b;
#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_READ_B(addr+2)
    DWORD_B_2(&r[PARAM_N])=m68k_src_b;
#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_READ_B(addr+4)
    DWORD_B_1(&r[PARAM_N])=m68k_src_b;
#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_READ_B(addr+6)
    DWORD_B_0(&r[PARAM_N])=m68k_src_b;
#if !defined(SSE_CPU_ROUNDING_MOVEP_MR_L)
    INSTRUCTION_TIME(4);
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
  }else{ // bchg
    if((ir&BITS_543)==BITS_543_000){ // register SS:data
/*
Dn,Dm :           |                 |             |               |
  .L :            |                 |             |               |
    if Dn<16      |  6(1/0)  0(0/0) |             |               | np       n  
    if Dn>15      |  8(1/0)  0(0/0) |             |               | np       nn 
*/
      m68k_src_w=BYTE(LOBYTE(r[PARAM_N]) & 31);
///      m68k_src_w=r[PARAM_N]%32;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
      if (m68k_src_w>=16){
        INSTRUCTION_TIME(4); //MAXIMUM VALUE
      }else{
        INSTRUCTION_TIME(2);
      }
      if((r[PARAM_M]>>(m68k_src_w))&1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
      r[PARAM_M]^=(1<<m68k_src_w);
    }else{

/*
Dn,<ea> :         |                 |             |               |
  .B :            |                 |             |               |             
    (An)          |  8(1/1)  4(1/0) |             |            nr | np    nw    
    (An)+         |  8(1/1)  4(1/0) |             |            nr | np    nw    
    -(An)         |  8(1/1)  6(1/0) |             | n          nr | np    nw    
    (d16,An)      |  8(1/1)  8(2/0) |             |      np    nr | np    nw    
    (d8,An,Xn)    |  8(1/1) 10(2/0) |             | n    np    nr | np    nw    
    (xxx).W       |  8(1/1)  8(2/0) |             |      np    nr | np    nw    
    (xxx).L       |  8(1/1) 12(3/0) |             |   np np    nr | np    nw    
*/
#if !defined(SSE_CPU_ROUNDING_BCHG)
      INSTRUCTION_TIME(4);
#endif
      m68k_GET_DEST_B_NOT_A;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
      if((m68k_DEST_B>>(7&r[PARAM_N]))&1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
#if defined(SSE_CPU_ROUNDING_BCHG)
      CPU_ABUS_ACCESS_WRITE;
#endif
      m68k_DEST_B^=(signed char)(1<<(7&r[PARAM_N]));
    }
  }
}

void                              m68k_movep_w_from_dN_or_bclr(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif

  if ((ir & BITS_543)==BITS_543_001){    //SS MOVEP
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1;
#endif
#if defined(SSE_CPU_ROUNDING_MOVEP_RM_W)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    MEM_ADDRESS ad=areg[PARAM_M]+(short)m68k_fetchW();
    pc+=2; 
#if !defined(SSE_CPU_ROUNDING_MOVEP_RM_W)
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_ROUNDING_MOVEP_RM_W)
    CPU_ABUS_ACCESS_WRITE;
#else
    INSTRUCTION_TIME(4);
#endif
    m68k_poke(ad,DWORD_B_1(&r[PARAM_N]));
    ad+=2;
#if defined(SSE_CPU_ROUNDING_MOVEP_RM_W)
    CPU_ABUS_ACCESS_WRITE;
#else
    INSTRUCTION_TIME(4);
#endif
    m68k_poke(ad,DWORD_B_0(&r[PARAM_N]));
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif

    ///INSTRUCTION_TIME(-4);

    PREFETCH_IRC;
  }else{ //bclr
    if((ir&BITS_543)==BITS_543_000){
/*
Dn,Dm :           |                 |             |               |
  .L :            |                 |             |               |
    if Dn<16      |  8(1/0)  0(0/0) |             |               | np nn       
    if Dn>15      | 10(1/0)  0(0/0) |             |               | np nn n     
*/
#if defined(SSE_CPU_ROUNDING_BCLR)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
#endif
      m68k_src_w=BYTE(LOBYTE(r[PARAM_N]) & 31);
      if (m68k_src_w>=16){
        INSTRUCTION_TIME(6); //MAXIMUM VALUE
      }else{
        INSTRUCTION_TIME(4);
      }
      if((r[PARAM_M]>>(m68k_src_w))&1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
#if !defined(SSE_CPU_ROUNDING_BCLR)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
#endif
      r[PARAM_M]&=(long)~((long)(1<<m68k_src_w));

      //length = .l
    }else{

/*
Dn,<ea> :         |                 |             |               |
  .B :            |                 |             |               |
    (An)          |  8(1/1)  4(1/0) |             |            nr | np    nw    
    (An)+         |  8(1/1)  4(1/0) |             |            nr | np    nw    
    -(An)         |  8(1/1)  6(1/0) |             | n          nr | np    nw    
    (d16,An)      |  8(1/1)  8(2/0) |             |      np    nr | np    nw    
    (d8,An,Xn)    |  8(1/1) 10(2/0) |             | n    np    nr | np    nw    
    (xxx).W       |  8(1/1)  8(2/0) |             |      np    nr | np    nw    
    (xxx).L       |  8(1/1) 12(3/0) |             |   np np    nr | np    nw    
*/

#if !defined(SSE_CPU_ROUNDING_BCLR)
      INSTRUCTION_TIME(4);
#endif
      m68k_GET_DEST_B_NOT_A;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
      if((m68k_DEST_B>>(7&r[PARAM_N]))&1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
#if defined(SSE_CPU_ROUNDING_BCLR)
      CPU_ABUS_ACCESS_WRITE;
#endif
      m68k_DEST_B&=(signed char)~(1<<(7&r[PARAM_N]));
    }
  }
}

void                              m68k_movep_l_from_dN_or_bset(){

  //SS there's a TODO for blitter, maybe need a case

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS))
  FETCH_TIMING;
#endif
  if ((ir&BITS_543)==BITS_543_001){  //SS MOVEP
/*
 eg movep.l d0,0(a1)  01C9 0000 
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
      MOVEP       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
Dx,(d16,Ay) :     |                 |
  .L :            | 24(2/4)         |                np nW nW nw nw np          

NOTES :
  .Read and write operations are done from the MSB to the LSB on 2 words if 
  using ".w" (first read/write word at Ay+d16 then word at Ay+d16+2)and on 
  4 words if using ".l" (first read/write word at Ay+d16 then word at Ay+d16+2 
  then word at Ay+d16+4 and finally word at Ay+d16+6).
*/

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1; 
#endif
#if defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    MEM_ADDRESS ad=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 
#if !defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    INSTRUCTION_TIME(4);
#endif

    BYTE *p=(BYTE*)(&r[PARAM_N]);

#if defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    CPU_ABUS_ACCESS_WRITE;
#endif
#if !defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    INSTRUCTION_TIME(4);
#endif
    m68k_poke(ad,DWORD_B_3(p));
    ad+=2;

#if defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    CPU_ABUS_ACCESS_WRITE;
#endif
#if !defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    INSTRUCTION_TIME(4);
#endif
    m68k_poke(ad,DWORD_B_2(p));
    ad+=2;

#if defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    CPU_ABUS_ACCESS_WRITE;
#endif
#if !defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    INSTRUCTION_TIME(4);
#endif
    m68k_poke(ad,DWORD_B_1(p));
    ad+=2;

#if defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    CPU_ABUS_ACCESS_WRITE;
#endif
#if !defined(SSE_CPU_ROUNDING_MOVEP_RM_L)
    INSTRUCTION_TIME(4);
#endif
    m68k_poke(ad,DWORD_B_0(p));

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC; 
  }else{ // SS bset
    if((ir&BITS_543)==BITS_543_000){
/*
Dn,Dm :           |                 |             |               |
  .L :            |                 |             |               |
    if Dn<16      |  6(1/0)  0(0/0) |             |               | np       n  
    if Dn>15      |  8(1/0)  0(0/0) |             |               | np       nn 
*/
#if defined(SSE_CPU_ROUNDING_BSET)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
#endif
      m68k_src_w=BYTE(LOBYTE(r[PARAM_N]) & 31);
      if (m68k_src_w>=16){
        INSTRUCTION_TIME(4); //MAXIMUM VALUE
      }else{
        INSTRUCTION_TIME(2);
      }
      if((r[PARAM_M]>>(m68k_src_w))&1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
#if !defined(SSE_CPU_ROUNDING_BSET)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
#endif
      r[PARAM_M]|=(1<<m68k_src_w);

//SS was so:
    //    m68k_dest=// D2_dM;
      //length = .l
    }else{
/*
Dn,<ea> :         |                 |             |               |
  .B :            |                 |             |               |             
    (An)          |  8(1/1)  4(1/0) |             |            nr | np    nw    
    (An)+         |  8(1/1)  4(1/0) |             |            nr | np    nw    
    -(An)         |  8(1/1)  6(1/0) |             | n          nr | np    nw    
    (d16,An)      |  8(1/1)  8(2/0) |             |      np    nr | np    nw    
    (d8,An,Xn)    |  8(1/1) 10(2/0) |             | n    np    nr | np    nw    
    (xxx).W       |  8(1/1)  8(2/0) |             |      np    nr | np    nw    
    (xxx).L       |  8(1/1) 12(3/0) |             |   np np    nr | np    nw    
*/

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_BSET) \
  || defined(SSE_CPU_ROUNDING_BSET))
      INSTRUCTION_TIME(4);
#endif
      m68k_GET_DEST_B_NOT_A;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_0_TIMINGS)
      FETCH_TIMING;
#endif
      PREFETCH_IRC;
#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_BSET)\
  && !defined(SSE_CPU_ROUNDING_BSET))
      INSTRUCTION_TIME(4);
#endif
      if((m68k_DEST_B>>(7&r[PARAM_N]))&1){
        SR_CLEAR(SR_Z);
      }else{
        SR_SET(SR_Z);
      }
#if defined(SSE_CPU_ROUNDING_BSET)
      CPU_ABUS_ACCESS_WRITE;
#endif
      m68k_DEST_B|=(signed char)(1<<(7&r[PARAM_N]));
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////        LINE-4 ROUTINES        //////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
-------------------------------------------------------------------------------
        CLR,      |    Exec Time    |               Data Bus Usage
  NEGX, NEG, NOT  |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea> :            |                 |               | 
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(2/0) |   np np    nr |               np nw       
  .L :            |                 |               | 
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    

*/


void                              m68k_negx_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true; //this is how we "emulate" the read before write behaviour
#endif
  m68k_GET_DEST_B_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif

  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B=(BYTE)-m68k_DEST_B;
  if(sr&SR_X)m68k_DEST_B--;
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C);
  if(m68k_DEST_B)SR_CLEAR(SR_Z);
  if(m68k_old_dest&m68k_DEST_B&MSB_B)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_B)&MSB_B)SR_SET(SR_C+SR_X);
  if(m68k_DEST_B & MSB_B)SR_SET(SR_N);
}void                             m68k_negx_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
  m68k_GET_DEST_W_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W=(WORD)-m68k_DEST_W;
  if(sr&SR_X)m68k_DEST_W--;
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C);
  if(m68k_DEST_W)SR_CLEAR(SR_Z);
  if(m68k_old_dest&m68k_DEST_W&MSB_W)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_W)&MSB_W)SR_SET(SR_C+SR_X);
  if(m68k_DEST_W & MSB_W)SR_SET(SR_N);
}

void                             m68k_negx_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU)
#if defined(SSE_CPU_ROUNDING_NEGX)
  m68k_GET_DEST_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2);
  else
  {
    CPU_ABUS_ACCESS_WRITE; //there were 8 missing cycles in original Steem
    CPU_ABUS_ACCESS_WRITE;
  }
#else
  m68k_GET_DEST_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2);
#endif
#else
  INSTRUCTION_TIME(2);
  m68k_GET_DEST_L_NOT_A;
#endif
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L=-m68k_DEST_L;
  if(sr&SR_X)m68k_DEST_L-=1;
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C);
  if(m68k_DEST_L)SR_CLEAR(SR_Z);
  if(m68k_old_dest&m68k_DEST_L&MSB_L)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_L)&MSB_L)SR_SET(SR_C+SR_X);
  if(m68k_DEST_L & MSB_L)SR_SET(SR_N);
}

//#undef SSE_CPU_LINE_4_TIMINGS
// TODO: These read the dest if it is memory
//SS: done in crash()
/*
<ea> :            |                 |               | 
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(2/0) |   np np    nr |               np nw       
*/
void                              m68k_clr_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_CLR)
  if (DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true; //it's only important for crashes
#endif
  m68k_GET_DEST_B_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_CLR)
  if(!DEST_IS_DATA_REGISTER)
  {
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_B=0;
#if defined(SSE_BOILER_MONITOR_VALUE3)
  if (DEST_IS_REGISTER==0)
    DEBUG_CHECK_WRITE_B(abus);
#endif
  SR_CLEAR(SR_N+SR_V+SR_C);
  SR_SET(SR_Z);
}

void                             m68k_clr_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_CLR)
  if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
  m68k_GET_DEST_W_NOT_A; //SS this crashes as 'write'
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_CLR)
  if(!DEST_IS_DATA_REGISTER)
  {
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_W=0;
#if defined(SSE_BOILER_MONITOR_VALUE3)
  if (DEST_IS_REGISTER==0)
    DEBUG_CHECK_WRITE_W(abus);
#endif
  SR_CLEAR(SR_N+SR_V+SR_C);
  SR_SET(SR_Z);
}

void                             m68k_clr_l(){

/*
  .L :            |                 |               | 
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    
*/

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_CLR)
  if(DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {INSTRUCTION_TIME(8);}
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
  m68k_GET_DEST_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_DATABUS)
//  M68000.dbus=0;//m68k_DEST_L&0xFFFF;
  M68000.dbus=pc+2;
#endif
#if defined(SSE_CPU_ROUNDING_CLR)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2);
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_L=0;
#if defined(SSE_BOILER_MONITOR_VALUE3)
  if (DEST_IS_REGISTER==0)
    DEBUG_CHECK_WRITE_L(abus);
#endif
  SR_CLEAR(SR_N+SR_V+SR_C);
  SR_SET(SR_Z);
}
//#define SSE_CPU_LINE_4_TIMINGS
//#undef SSE_CPU_LINE_4_TIMINGS

/*
-------------------------------------------------------------------------------
        CLR,      |    Exec Time    |               Data Bus Usage
  NEGX, NEG, NOT  |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea> :            |                 |               | 
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(2/0) |   np np    nr |               np nw       
*/

void                              m68k_neg_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_NEG)
  if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
#endif
  m68k_GET_DEST_B_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_NEG)
  if(DEST_IS_REGISTER==0){CPU_ABUS_ACCESS_WRITE;}
#endif
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B=(BYTE)-m68k_DEST_B;
  SR_CLEAR(SR_USER_BYTE);
  if(m68k_old_dest&m68k_DEST_B&MSB_B)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_B)&MSB_B)SR_SET(SR_C+SR_X);
  SR_CHECK_Z_AND_N_B;
}

void                             m68k_neg_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_NEG)
  if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
#endif
  m68k_GET_DEST_W_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_NEG)
  if(DEST_IS_REGISTER==0){CPU_ABUS_ACCESS_WRITE;}
#endif
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W=(WORD)-m68k_DEST_W;
  SR_CLEAR(SR_USER_BYTE);
  if(m68k_old_dest&m68k_DEST_W&MSB_W)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_W)&MSB_W)SR_SET(SR_C+SR_X);
  SR_CHECK_Z_AND_N_W;
}
/*
-------------------------------------------------------------------------------
        CLR,      |    Exec Time    |               Data Bus Usage
  NEGX, NEG, NOT  |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea> :            |                 |               | 
  .L :            |                 |               | 
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    
*/

void                             m68k_neg_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_NEG)
  if(DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {INSTRUCTION_TIME(8);}
#endif
  m68k_GET_DEST_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif

#if defined(SSE_CPU_ROUNDING_NEG)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2);
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif

  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L=-m68k_DEST_L;
  SR_CLEAR(SR_USER_BYTE);
  if(m68k_old_dest&m68k_DEST_L&MSB_L)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_L)&MSB_L)SR_SET(SR_C+SR_X);
  SR_CHECK_Z_AND_N_L;
}

void                              m68k_not_b(){
/*
  <ea> :            |                 |               | 
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(2/0) |   np np    nr |               np nw       
*/
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_NOT)
  if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);} 
#endif
  m68k_GET_DEST_B_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_NOT)
  if(DEST_IS_REGISTER==0){CPU_ABUS_ACCESS_WRITE;}
#endif
  m68k_DEST_B=(BYTE)~m68k_DEST_B;
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  SR_CHECK_Z_AND_N_B;
}

void                             m68k_not_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_NOT)
  if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
#endif
  m68k_GET_DEST_W_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_NOT)
  if(DEST_IS_REGISTER==0){CPU_ABUS_ACCESS_WRITE;}
#endif
  m68k_DEST_W=(WORD)~m68k_DEST_W;
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  SR_CHECK_Z_AND_N_W;
}

void                             m68k_not_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_NOT)
  if (DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {INSTRUCTION_TIME(8);}
#endif
  m68k_GET_DEST_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_NOT)
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2);
  else
  {
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_L=~m68k_DEST_L;
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  SR_CHECK_Z_AND_N_L;
}

/*
-------------------------------------------------------------------------------
	                |     Exec Time   |               Data Bus Usage             
       TST        |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
<ea> :            |                 |               | 
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  4(1/0)  4(1/0) |            nr |               np          
    (An)+         |  4(1/0)  4(1/0) |            nr |               np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np          
  .L              |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  4(1/0)  8(2/0) |         nR nr |               np          
    (An)+         |  4(1/0)  8(2/0) |         nR nr |               np          
    -(An)         |  4(1/0) 10(2/0) | n       nR nr |               np          
    (d16,An)      |  4(1/0) 12(3/0) |      np nR nr |               np          
    (d8,An,Xn)    |  4(1/0) 14(3/0) | n    np nR nr |               np          
    (xxx).W       |  4(1/0) 12(3/0) |      np nR nr |               np          
    (xxx).L       |  4(1/0) 16(4/0) |   np np nR nr |               np          

*/

void                              m68k_tst_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  BYTE x=m68k_read_dest_b();
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  if(!x)SR_SET(SR_Z);
  if(x&MSB_B)SR_SET(SR_N);
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
}void                             m68k_tst_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  WORD x=m68k_read_dest_w();
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  if(!x)SR_SET(SR_Z);
  if(x&MSB_W)SR_SET(SR_N);
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
}void                             m68k_tst_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  LONG x=m68k_read_dest_l();
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  if(!x)SR_SET(SR_Z);
  if(x&MSB_L)SR_SET(SR_N);
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
}


//#define SSE_CPU_LINE_4_TIMINGS
//#undef SSE_CPU_LINE_4_TIMINGS

/* SS
Instruction   Size   Register   Memory
TAS           Byte    4(1/0)    14(2/1)+

One read for prefetch.
If memory, one read and one write.
TAS is class 0, prefetch occurs before write.

  Read memory (4)
  Test (2)
  Prefetch (4)
  Write memory (4)

Don't see how it could be 10 instead of 14: we shouldn't define
SSE_CPU_TAS. Haven't seen a case that it changes yet.

Update from M68000UM
TAS 	Byte	4(1/0)	14(2/1)+


Update from Yacht:
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
       TAS        |  INSTR     EA   |  1st OP (ea)  |          INSTR            
------------------+-----------------+---------------+--------------------------
<ea> :            |                 |               |
  .B :            |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          | 10(1/1)  4(1/0) |            nr |          n nw np          
    (An)+         | 10(1/1)  4(1/0) |            nr |          n nw np          
    -(An)         | 10(1/1)  6(1/0) | n          nr |          n nw np          
    (d16,An)      | 10(1/1)  8(2/0) |      np    nr |          n nw np          
    (d8,An,Xn)    | 10(1/1) 10(2/0) | n    np    nr |          n nw np          
    (xxx).W       | 10(1/1)  8(1/0) |               |          n nw np          
    (xxx).L       | 10(1/1) 12(2/0) |               |          n nw np          
NOTES :
  .M68000UM is probably wrong with instruction timming when <ea> is different 
   from Dn. It reads "14(3/0)" but according to the same book the read-modify-
   write bus cycle used by this instruction is only 10 clock cycles long which 
   seems coherent with the microwords decomposition in USP4325121. Last, 
   evaluation on real hardware confirms the 10 cycles timing.

Following this, which seems to justify the Steem authors comment, we now define
 SSE_CPU_TAS.

We also move the prefetch timing: it is class 1, not 0.

*/

void                              m68k_tas(){
  if((ir&B6_111111)==B6_111100){
    ASSERT(ir==0x4afc); // it is tas_or_illegal()
    ILLEGAL;
  }else{

#if defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1;
#endif

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
    FETCH_TIMING;
#endif

#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif

    m68k_GET_DEST_B_NOT_A;

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TAS))
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_TAS) // not defined until 3.5.1
    if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(6);} 
#else
    if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(10);} /// Should this be 6?
#endif

    SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
    m68k_DEST_B|=MSB_B;

#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TAS))
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif

  }
}

//#define SSE_CPU_LINE_4_TIMINGS
//#undef SSE_CPU_LINE_4_TIMINGS
/*
-------------------------------------------------------------------------------
       MOVE       |    Exec Time    |               Data Bus Usage
     from SR      |  INSTR     EA   |  2nd Op (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
SR,<ea> :         |                 |               |
  .W :            |                 |               |
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       
*/
// TODO: This should read the memory first 
void                              m68k_move_from_sr(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_MOVE_FROM_SR)
  m68k_GET_DEST_W_NOT_A; // DSOS loader
#endif
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  if (DEST_IS_REGISTER){
    INSTRUCTION_TIME(2);
  }else{
#if defined(SSE_CPU_ROUNDING_MOVE_FROM_SR)
    CPU_ABUS_ACCESS_WRITE;
#else
    INSTRUCTION_TIME(4);
#endif
  }
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_MOVE_FROM_SR))
  m68k_GET_DEST_W_NOT_A;
#endif
  m68k_DEST_W=sr;
}

//#define SSE_CPU_LINE_4_TIMINGS

void                              m68k_move_from_ccr(){
    ILLEGAL;
  
  ////ILLEGAL;  //68010 only!!!
/*
  m68k_GET_DEST_B_NOT_A;
  m68k_DEST_B=LOBYTE(sr);
*/
}


/*
-------------------------------------------------------------------------------
       MOVE       |    Exec Time    |               Data Bus Usage
  to CCR, to SR   |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea>,CCR :        |                 |               |
  .W :            |                 |               |
    Dn            | 12(1/0)  0(0/0) |               |         nn np np          
    (An)          | 12(1/0)  4(1/0) |            nr |         nn np np          
    (An)+         | 12(1/0)  4(1/0) |            nr |         nn np np          
    -(An)         | 12(1/0)  6(1/0) | n          nr |         nn np np          
    (d16,An)      | 12(1/0)  8(2/0) |      np    nr |         nn np np          
    (d8,An,Xn)    | 12(1/0) 10(2/0) | n    np    nr |         nn np np          
    (xxx).W       | 12(1/0)  8(2/0) |      np    nr |         nn np np          
    (xxx).L       | 12(1/0) 12(3/0) |   np np    nr |         nn np np          
    #<data>       | 12(1/0)  4(1/0) |      np       |         nn np np          
*/

void                              m68k_move_to_ccr(){
  if((ir&BITS_543)==BITS_543_001){
    m68k_unrecognised();
  }else{
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif

    m68k_GET_SOURCE_W;
#if defined(SSE_CPU_ROUNDING_MOVE_TO_SR)
    INSTRUCTION_TIME(8);
#endif
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
    CCR=LOBYTE(m68k_src_w);
    sr&=SR_VALID_BITMASK;
#if !defined(SSE_CPU_ROUNDING_MOVE_TO_SR)
    INSTRUCTION_TIME(8);
#endif
  }
}


void                              m68k_move_to_sr(){
  if(SUPERFLAG){
    if((ir&BITS_543)==BITS_543_001){ //address register
      m68k_unrecognised();
    }else{
      DEBUG_ONLY( int debug_old_sr=sr; )
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
      FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_MOVE_TO_SR)
      INSTRUCTION_TIME(8);
#endif
      m68k_GET_SOURCE_W;
#if defined(SSE_CPU_ROUNDING_MOVE_TO_SR)
      INSTRUCTION_TIME(8);
#endif
      sr=m68k_src_w;
      sr&=SR_VALID_BITMASK;
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
      FETCH_TIMING;
#endif
      DETECT_CHANGE_TO_USER_MODE;
      DETECT_TRACE_BIT;
      // Interrupts must come after trace exception
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
//      check_for_interrupts_pending(); // was commented out
      CHECK_STOP_ON_USER_CHANGE;
    }
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}

//#define SSE_CPU_LINE_4_TIMINGS
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       NBCD       |      INSTR      |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea> :            |                 |               |
  .B :            |                 |               |
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       
*/

void                              m68k_nbcd(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_NBCD)
  if (DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {INSTRUCTION_TIME(4);}
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
  m68k_GET_DEST_B_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_NBCD)
  if (DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {CPU_ABUS_ACCESS_WRITE;}
#endif
  int m=m68k_DEST_B,n=0;
  if(m&0xff) n=0xa0;
  if(m&0xf)n=0x9a;
  if(sr&SR_X)n=0x99;
  SR_CLEAR(SR_X+SR_C);
  if(m)SR_SET(SR_X+SR_C); //there will be a carry
  m68k_DEST_B=(BYTE)(n-m);
  if(m68k_DEST_B){SR_CLEAR(SR_Z);}
}
void                              m68k_pea_or_swap(){
  if((ir&BITS_543)==BITS_543_000){ // SWAP

/*
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
       SWAP       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
Dn :              |                 |
  .W :            |  4(1/0)         |                               np          

*/

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
    FETCH_TIMING;
#endif
    r[PARAM_M]=MAKELONG(HIWORD(r[PARAM_M]),LOWORD(r[PARAM_M]));
#if !defined(SSE_CPU_PREFETCH_TIMING_SWAP)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
#endif
    SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
    if(!r[PARAM_M])SR_SET(SR_Z);
    if(r[PARAM_M]&MSB_L)SR_SET(SR_N);
#if defined(SSE_CPU_PREFETCH_TIMING_SWAP)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
#endif
  }else{ //PEA

#if !defined(SSE_CPU_ROUDING_EA)
    // pea instruction times table.
    // ad.mode  time  Steem EA time difference
    // (aN)     12    0             12
    // D(aN)    16    4             12
    // D(aN,dM) 22    8             14
    // xxx.w    16    4             12
    // xxx.l    20    8             12
    // D(pc)    16    4             12
    // D(pc,dM) 22    8             14
    if ((ir & B6_111111)==B6_111011 || (ir & B6_111000)==B6_110000){ INSTRUCTION_TIME(2); } //iriwo
    m68k_get_effective_address();
#endif
#if defined(SSE_CPU_ROUDING_EA)
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        PEA       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
<ea> :            |                 | 
  .L :            |                 |
    (An)          | 12(1/2)         |                               np nS ns    
    (d16,An)      | 16(2/2)         |                          np   np nS ns    
    (d8,An,Xn)    | 20(2/2)         |                        n np n np nS ns    
    (xxx).W       | 16(2/2)         |                               np nS ns np 
    (xxx).L       | 20(3/2)         |                          np   np nS ns np 
*/
  switch (ir & BITS_543){
    case BITS_543_010: //SS (An)
      effective_address=areg[PARAM_M];
      break;
    case BITS_543_101: //SS (d16,An)
      CPU_ABUS_ACCESS_READ_FETCH;
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_543_110: //SS (d8,An,Xn)
      INSTRUCTION_TIME(2);
      CPU_ABUS_ACCESS_READ_FETCH;
      m68k_iriwo=m68k_fetchW();pc+=2; 
      INSTRUCTION_TIME(2);
      if (m68k_iriwo & BIT_b){  //.l
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_543_111:
      switch (ir & 0x7){
      case 0: //SS (xxx).W
        CPU_ABUS_ACCESS_READ_FETCH;
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
        TRUE_PC+=2;
#endif
        break;
      case 1: //SS (xxx).L
        CPU_ABUS_ACCESS_READ_FETCH;
        CPU_ABUS_ACCESS_READ_FETCH;
        effective_address=m68k_fetchL();
        pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
        TRUE_PC+=4;
#endif
        break;
      case 2: //SS (d16,PC) 
        CPU_ABUS_ACCESS_READ_FETCH;
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;
      case 3: //SS (d8,PC,Xn)
        INSTRUCTION_TIME(2);
        CPU_ABUS_ACCESS_READ_FETCH;
        m68k_iriwo=m68k_fetchW();
        INSTRUCTION_TIME(2);
        if (m68k_iriwo & BIT_b){  //.l
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]) | pc_high_byte;
        }else{         //.w
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]) | pc_high_byte;
        }
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;       //what do bits 8,9,a  of extra word do?  (not always 0)
      default:
        m68k_unrecognised();
        break;
      }
      break;
    default:
      m68k_unrecognised();
  }
#endif

#if !defined(SSE_CPU_ROUNDING_PEA)
    INSTRUCTION_TIME_ROUND(8); // Round before writing to memory
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_PEA)
    if((ir & B6_111111)==B6_111000) 
      M68000.PrefetchClass=1; // PEA for absolute short and absolute long addr. modes
    else
    {
      PREFETCH_IRC; 
      FETCH_TIMING;
    }
#endif

#if defined(SSE_CPU_ROUNDING_PEA)
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_PUSH_L(effective_address);

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_PEA)
    if(M68000.PrefetchClass==1)
    {
      PREFETCH_IRC;
      FETCH_TIMING;
    }
#else
    FETCH_TIMING; // <-- Steem authors perceived the 'class 1' behaviour?
#endif
  }
}

//#define SSE_CPU_LINE_4_TIMINGS

void                              m68k_movem_w_from_regs_or_ext_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  if((ir&BITS_543)==BITS_543_000){ //SS EXT.W
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        EXT       |      INSTR      |  1st Operand  |          INSTR
------------------+-----------------+---------------+--------------------------
Dn :              |                 |               |
  .W :            |  4(1/0)         |               |               np          
  .L :            |  4(1/0)         |               |               np          
*/
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
    m68k_dest=&(r[PARAM_M]);
#if !defined(SSE_CPU_PREFETCH_TIMING_EXT)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
#endif
    m68k_DEST_W=(signed short)((signed char)LOBYTE(r[PARAM_M]));
    SR_CHECK_Z_AND_N_W;
#if defined(SSE_CPU_PREFETCH_TIMING_EXT)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
#endif
  }
  //SS MOVEM R->M -(An)
  else if ((ir & BITS_543)==BITS_543_100){ //predecrement 
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1; 
#endif
#if defined(SSE_CPU_ROUNDING_MOVEM)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_src_w=m68k_fetchW();pc+=2; 
#if !defined(SSE_CPU_ROUNDING_MOVEM)
    INSTRUCTION_TIME(4);
#endif
    MEM_ADDRESS ad=areg[PARAM_M];
    DWORD areg_hi=(areg[PARAM_M] & 0xff000000);
#if defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
    short mask=1,BlitterStart=0;
    for (int n=0;n<16;n++){
      if (m68k_src_w & mask){
        ad-=2;
#if defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
        CPU_ABUS_ACCESS_WRITE;
#endif
        m68k_dpoke(ad,LOWORD(r[15-n]));
#if !defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
        INSTRUCTION_TIME(4);
#endif
        if (ioaccess & IOACCESS_FLAG_DO_BLIT){
          // After word that starts blitter must write one more word, then blit
          if ((++BlitterStart)==2){
            Blitter_Start_Now();
            BlitterStart=0;
          }
        }
      }
      mask<<=1;
    }
    // The register written to memory should be the original one, so
    // predecrement afterwards.
    areg[PARAM_M]=ad | areg_hi;
#if !defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
#endif
  }else{ //SS MOVEM other cases
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1; 
#endif
#if defined(SSE_CPU_ASSERT_ILLEGAL4)
    switch (ir & BITS_543){
    case BITS_543_010: // (An)
    case BITS_543_101: // (d16,An)
    case BITS_543_110: // (d8, An, Xn)
      break;
    case BITS_543_111:
      switch(ir&0x7){
      case 0:
        break;
      case 1:
        break;
      default:
        m68k_unrecognised();
      }
      break;
    default:
      m68k_unrecognised();
    }
#endif
#if defined(SSE_CPU_ROUNDING_MOVEM)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    m68k_src_w=m68k_fetchW();pc+=2; 
#if !defined(SSE_CPU_ROUNDING_MOVEM)
    INSTRUCTION_TIME(4);
#endif
    MEM_ADDRESS ad;
    switch (ir & BITS_543){
    case BITS_543_010: // (An)
      ad=areg[PARAM_M];
      break;
    case BITS_543_101: // (d16,An)
#if defined(SSE_CPU_ROUNDING_MOVEM)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(4);
#endif
      ad=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_543_110: // (d8, An, Xn)
#if defined(SSE_CPU_ROUNDING_MOVEM6)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(6);
#endif
      m68k_iriwo=m68k_fetchW();pc+=2; 
      if(m68k_iriwo&BIT_b){  //.l
        ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
#if defined(SSE_CPU_ROUNDING_MOVEM6)
      INSTRUCTION_TIME(2);
#endif
      break;
    case BITS_543_111:
      switch(ir&0x7){
      case 0:
#if defined(SSE_CPU_ROUNDING_MOVEM6)
        CPU_ABUS_ACCESS_READ_FETCH;
#else
        INSTRUCTION_TIME(4);
#endif
        ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case 1:
#if defined(SSE_CPU_ROUNDING_MOVEM6)
        CPU_ABUS_ACCESS_READ_FETCH;
        CPU_ABUS_ACCESS_READ_FETCH;
#else
        INSTRUCTION_TIME(8);
#endif
        ad=0xffffff & m68k_fetchL();
        pc+=4;  
        break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL4)
      default:
        ad=0; // This is to stop an annoying warning
        m68k_unrecognised();
        break;
#endif
      }
      break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL4)
    default:
      ad=0; // This is to stop an annoying warning
      m68k_unrecognised();
#endif
    }
#if defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
    short mask=1,BlitterStart=0;
    for (int n=0;n<16;n++){
      if (m68k_src_w & mask){
#if defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
        CPU_ABUS_ACCESS_WRITE;
#endif
        m68k_dpoke(ad,LOWORD(r[n]));
#if !defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
        INSTRUCTION_TIME(4);
#endif
        ad+=2;
        if (ioaccess & IOACCESS_FLAG_DO_BLIT){
          // After word that starts blitter must write one more word, then blit
          if ((++BlitterStart)==2){
            Blitter_Start_Now();
            BlitterStart=0;
          }
        }
      }
      mask<<=1;
    }
#if !defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
  }
  if (ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_W){  //oh dear, writing multiple words to the PSG
    int s=count_bits_set_in_word(m68k_src_w);
    if (s>4) BUS_JAM_TIME((s-1) & -4);  //we've already had a bus jam of 4, for s=5..8 want extra bus jam of 4
  }
}

//#define SSE_CPU_LINE_4_TIMINGS
//#undef SSE_CPU_LINE_4_TIMINGS
void                              m68k_movem_l_from_regs_or_ext_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  if((ir&BITS_543)==BITS_543_000){  //ext.l
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        EXT       |      INSTR      |  1st Operand  |          INSTR
------------------+-----------------+---------------+--------------------------
Dn :              |                 |               |
  .W :            |  4(1/0)         |               |               np          
  .L :            |  4(1/0)         |               |               np          

*/
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
    m68k_dest=&(r[PARAM_M]);
#if !defined(SSE_CPU_PREFETCH_TIMING_EXT)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
    m68k_DEST_L=(signed long)((signed short)LOWORD(r[PARAM_M]));
    SR_CHECK_Z_AND_N_L;
#if defined(SSE_CPU_PREFETCH_TIMING_EXT)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif

  }else if((ir&BITS_543)==BITS_543_100){ //predecrement //SS MOVEM -(An)
/*

-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
      MOVEM       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
R --> M           |                 | 
  .L              |                 |                             
    -(An)         |  8+8m(2/2m)     |                np (nw nW)*    np      
*/
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS) 
    M68000.PrefetchClass=1; 
#endif
#if defined(SSE_CPU_ROUNDING_MOVEM)
    CPU_ABUS_ACCESS_READ_FETCH; // no action if SSE_CPU_TIMINGS_REFACTOR_FETCH defined
#endif
    m68k_src_w=m68k_fetchW();pc+=2;
#if !defined(SSE_CPU_ROUNDING_MOVEM)
    INSTRUCTION_TIME(4);
#endif
    MEM_ADDRESS ad=areg[PARAM_M];
    DWORD areg_hi=(areg[PARAM_M] & 0xff000000);
#if defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC=pc+2;
#endif
    short mask=1;
    for (int n=0;n<16;n++){
      if (m68k_src_w & mask){
        ad-=4;
#if defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
        CPU_ABUS_ACCESS_WRITE;
        CPU_ABUS_ACCESS_WRITE;
#else
        INSTRUCTION_TIME(4);
#endif
        m68k_lpoke(ad,r[15-n]);
#if !defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
        INSTRUCTION_TIME(4);
#endif
        if (ioaccess & IOACCESS_FLAG_DO_BLIT) Blitter_Start_Now();
      }
      mask<<=1;
    }
    // The register written to memory should be the original one, so
    // predecrement afterwards.
    areg[PARAM_M]=ad | areg_hi;
#if !defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
  }else{ //SS MOVEM other cases

/*

-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
      MOVEM       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
R --> M           |                 | 
  .L              |                 |                             
    (An)          |  8+8m(2/2m)     |                np (nW nw)*    np          
    (d16,An)      | 12+8m(3/2m)     |             np np (nW nw)*    np          
    (d8,An,Xn)    | 14+8m(3/2m)     |        n    np np (nW nw)*    np          
    (xxx).W       | 12+8m(3/2m)     |             np np (nW nw)*    np          
    (xxx).L       | 16+8m(4/2m)     |          np np np (nW nw)*    np      
*/

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS) 
    M68000.PrefetchClass=1; 
#endif
#if defined(SSE_CPU_ASSERT_ILLEGAL5)
    switch (ir & BITS_543){
    case BITS_543_010: // (An)
    case BITS_543_101: // (d16,An)
    case BITS_543_110: // (d8, An, Xn)
      break;
    case BITS_543_111:
      switch(ir&0x7){
      case 0:
      case 1:
        break;
      default:
        m68k_unrecognised();
      }
      break;
    default:
      m68k_unrecognised();
    }
#endif
#if defined(SSE_CPU_ROUNDING_MOVEM) 
    CPU_ABUS_ACCESS_READ_FETCH; // no action if SSE_CPU_TIMINGS_REFACTOR_FETCH defined
#endif
    m68k_src_w=m68k_fetchW();pc+=2; 
#if !defined(SSE_CPU_ROUNDING_MOVEM)
    INSTRUCTION_TIME(4);
#endif
    MEM_ADDRESS ad;
    switch(ir&BITS_543){
    case BITS_543_010:
      ad=areg[PARAM_M];
      break;
    case BITS_543_101:
#if defined(SSE_CPU_ROUNDING_MOVEM)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(4);
#endif
      ad=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_543_110:
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ_FETCH
#endif
      m68k_ap=m68k_fetchW();pc+=2; 
#if !defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
#if defined(SSE_CPU_ROUNDING_MOVEM6) 
      CPU_ABUS_ACCESS_READ;
      ///INSTRUCTION_TIME_ROUND(6);
#else
      INSTRUCTION_TIME(6);
#endif
#endif
      if(m68k_ap&BIT_b){  //.l
        ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_ap)+(int)r[m68k_ap>>12];
      }else{         //.w
        ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_ap)+(signed short)r[m68k_ap>>12];
      }
#if defined(SSE_CPU_ROUNDING_MOVEM6)
      INSTRUCTION_TIME(2);
#endif
      break;
    case BITS_543_111:
      switch(ir&0x7){
      case 0://(xxx).W
#if defined(SSE_CPU_ROUNDING_MOVEM)
        CPU_ABUS_ACCESS_READ_FETCH;
#endif
        ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
#if !defined(SSE_CPU_ROUNDING_MOVEM)
        INSTRUCTION_TIME(4);
#endif
        pc+=2; 
        break;
      case 1://(xxx).L
#if defined(SSE_CPU_ROUNDING_MOVEM)
        CPU_ABUS_ACCESS_READ_FETCH;
        CPU_ABUS_ACCESS_READ_FETCH;
#endif
        ad=0xffffff&m68k_fetchL();
#if !defined(SSE_CPU_ROUNDING_MOVEM)
        INSTRUCTION_TIME(8);
#endif
        pc+=4;  
        break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL5)
      default: //ILLEGAL
        ad=0; // This is to stop an annoying warning
        m68k_unrecognised();
        break;
#endif
      }
      break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL5)
    default:
      ad=0; // This is to stop an annoying warning
      m68k_unrecognised();
#endif
    }
#if defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
/* see below, we must define either SSE_CPU_PREFETCH_TIMING_MOVEM_HACK
   or SSE_CPU_MOVEM_BUS_ACCESS_TIMING for Dragonnel menu correct display on
   line 200.
   SSE_CPU_PREFETCH_TIMING_MOVEM_HACK = hack before 3.7
   SSE_CPU_MOVEM_BUS_ACCESS_TIMING = legit fix in Steem CPU since 3.7
*/
    PREFETCH_IRC; 
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
#endif
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC=pc+2; // Blood Money original
#endif
    short mask=1;
    for (int n=0;n<16;n++){
      if (m68k_src_w&mask){
#if defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
/*  It seems that timing should be counted at once during the long
    move.
    With this switch Dragonnels menu is correct and prefetch is
    correctly placed, at the end of the instruction, at last.
*/
        CPU_ABUS_ACCESS_WRITE;
        CPU_ABUS_ACCESS_WRITE;
#else
        INSTRUCTION_TIME_ROUND(4);
#endif
        m68k_lpoke(ad,r[n]);
        ad+=4;
#if !defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
        INSTRUCTION_TIME(4);
#endif
      }
      mask<<=1;
    }
#if !defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
    PREFETCH_IRC; 
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
#endif
  }
  if (ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_W){  //oh dear, writing multiple longs to the PSG
    int s=count_bits_set_in_word(m68k_src_w)*2; //number of words to write
    if(s>4)BUS_JAM_TIME((s-1)&-4);  //we've already had a bus jam of 4, for s=5..8 want extra bus jam of 4
  }
}

//#define SSE_CPU_LINE_4_TIMINGS

void                              m68k_movem_l_to_regs(){

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ASSERT_ILLEGAL5B)
    switch (ir & BITS_543){
    case BITS_543_010: // (An)
    case BITS_543_011:
    case BITS_543_101: // (d16,An)
    case BITS_543_110: // (d8, An, Xn)
      break;
    case BITS_543_111:
      switch(ir&0x7){
      case 0:
      case 1:
      case 2:
      case 3:
        break;
      default:
        m68k_unrecognised();
      }
      break;
    default:
      m68k_unrecognised();
    }
#endif
  bool postincrement=false;
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
  CPU_ABUS_ACCESS_READ_FETCH;// fixes Hackabonds Demo instructions scroller (STF)
#endif
  m68k_src_w=m68k_fetchW();pc+=2;  // SS: TODO what if m68k_src_w=0?
#if !defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
  INSTRUCTION_TIME(4);
#endif
  MEM_ADDRESS ad;
  switch(ir&BITS_543){
  case BITS_543_010: //ss (A)
    ad=areg[PARAM_M];
    break;
  case BITS_543_011:
    ad=areg[PARAM_M];
    postincrement=true;
    break;
  case BITS_543_101:
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
    CPU_ABUS_ACCESS_READ_FETCH;
#endif
    ad=areg[PARAM_M]+(signed short)m68k_fetchW();
#if !defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
    INSTRUCTION_TIME(4);
#endif
    pc+=2; 
    break;
  case BITS_543_110:
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
    CPU_ABUS_ACCESS_READ_FETCH;
#else
    INSTRUCTION_TIME(6);
#endif
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
    INSTRUCTION_TIME(2);
#endif
    break;
  case BITS_543_111:
    switch(ir&0x7){
    case 0:
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(4);
#endif
      ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      break;
    case 1:
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
      CPU_ABUS_ACCESS_READ_FETCH;
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(8);
#endif
      ad=0xffffff&m68k_fetchL();
      pc+=4;  
      break;
    case 2:
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(4);
#endif
      ad=pc+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case 3:
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(6);
#endif
      m68k_iriwo=m68k_fetchW();
      if(m68k_iriwo&BIT_b){  //.l
        ad=pc+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        ad=pc+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
#if defined(SSE_CPU_ROUNDING_MOVEM_MR_L)
      INSTRUCTION_TIME(2);
#endif
      pc+=2; 
      break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL5B)
    default:
      ad=0; // This is to stop an annoying warning
      m68k_unrecognised();
      break;
#endif
    }
    break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL5B)
  default:
    ad=0; // This is to stop an annoying warning
    m68k_unrecognised();
#endif
  }
#if defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
#if defined(SSE_CPU_TRUE_PC)
  TRUE_PC=pc+2;
#endif
  DWORD areg_hi=(areg[PARAM_M] & 0xff000000);
  short mask=1;
  for (int n=0;n<16;n++){
    if (m68k_src_w & mask){
      
#if defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
      CPU_ABUS_ACCESS_READ;
      CPU_ABUS_ACCESS_READ;
#else
      INSTRUCTION_TIME_ROUND(4);
#endif
      r[n]=m68k_lpeek(ad);
#if !defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
      INSTRUCTION_TIME(4);
#endif
      ad+=4;
    }
    mask<<=1;
  }
  if (postincrement) areg[PARAM_M]=ad | areg_hi;
  m68k_dpeek(ad); //extra word read (discarded)
  INSTRUCTION_TIME_ROUND(4);
#if !defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
  //SS: PSG jam seems to be correctly emulated as changing this (here or there) 
  //will break many programs...
  if (ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_R){  //oh dear, reading multiple longs from the PSG
    int s=count_bits_set_in_word(m68k_src_w)*2+1; //number of words read
    if(s>4)BUS_JAM_TIME((s-1)&-4);  //we've already had a bus jam of 4, for s=5..8 want extra bus jam of 4
  }
}

//#define SSE_CPU_LINE_4_TIMINGS

void                              m68k_movem_w_to_regs(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ASSERT_ILLEGAL4B)
    switch (ir & BITS_543){
    case BITS_543_010: // (An)
    case BITS_543_011:
    case BITS_543_101: // (d16,An)
    case BITS_543_110: // (d8, An, Xn)
      break;
    case BITS_543_111:
      switch(ir&0x7){
      case 0:
      case 1:
      case 2:
      case 3:
        break;
      default:
        m68k_unrecognised();
      }
      break;
    default:
      m68k_unrecognised();
    }
#endif
  bool postincrement=false;
#if defined(SSE_CPU_ROUNDING_MOVEM)
  CPU_ABUS_ACCESS_READ_FETCH;
#else
  INSTRUCTION_TIME(4);
#endif
  m68k_src_w=m68k_fetchW();pc+=2; 

  MEM_ADDRESS ad;
  switch (ir & BITS_543){
  case BITS_543_010:
    ad=areg[PARAM_M];
    break;
  case BITS_543_011:
    ad=areg[PARAM_M];
    postincrement=true;
    break;
  case BITS_543_101:
#if defined(SSE_CPU_ROUNDING_MOVEM)
    CPU_ABUS_ACCESS_READ_FETCH;
#else
    INSTRUCTION_TIME(4);
#endif
    ad=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 
    break;
  case BITS_543_110:
#if defined(SSE_CPU_ROUNDING_MOVEM6)
    CPU_ABUS_ACCESS_READ_FETCH;
    //INSTRUCTION_TIME_ROUND(6);
#else
    INSTRUCTION_TIME(6);
#endif
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      ad=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
#if defined(SSE_CPU_ROUNDING_MOVEM6)
    INSTRUCTION_TIME(2);
#endif
    break;
  case BITS_543_111:
    switch(ir&0x7){
    case 0:
#if defined(SSE_CPU_ROUNDING_MOVEM)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(4);
#endif      
      ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      break;
    case 1:
#if defined(SSE_CPU_ROUNDING_MOVEM)
      CPU_ABUS_ACCESS_READ_FETCH;
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(8);
#endif      
      ad=0xffffff&m68k_fetchL();
      pc+=4;  
      break;
    case 2:
#if defined(SSE_CPU_ROUNDING_MOVEM)
      CPU_ABUS_ACCESS_READ_FETCH;
#else
      INSTRUCTION_TIME(4);
#endif
      ad=pc+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case 3:
#if defined(SSE_CPU_ROUNDING_MOVEM6)
      CPU_ABUS_ACCESS_READ_FETCH;
      //INSTRUCTION_TIME_ROUND(6);
#else
      INSTRUCTION_TIME(6);
#endif
      m68k_iriwo=m68k_fetchW();
      if(m68k_iriwo&BIT_b){  //.l
        ad=pc+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        ad=pc+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
#if defined(SSE_CPU_ROUNDING_MOVEM6)
      INSTRUCTION_TIME(2);
#endif
      pc+=2; 
      // m68k_src_w=// D2_IRIWO_PC;
      break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL4B)
    default:
      ad=0; // This is to stop an annoying warning
      m68k_unrecognised();
      break;
#endif
    }
    break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL4B)
  default:
    ad=0; // This is to stop an annoying warning
    m68k_unrecognised();
#endif
  }
#if defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
  DWORD areg_hi=(areg[PARAM_M] & 0xff000000);
#if defined(SSE_CPU_TRUE_PC)
  TRUE_PC=pc+2;
#endif
  short mask=1;
  for(int n=0;n<16;n++){
    if (m68k_src_w & mask){
#if defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
      CPU_ABUS_ACCESS_READ;
#endif
      r[n]=(signed long)((signed short)m68k_dpeek(ad));
#if !defined(SSE_CPU_MOVEM_BUS_ACCESS_TIMING)
      INSTRUCTION_TIME_ROUND(4);
#endif
      ad+=2;
    }
    mask<<=1;
  }
  if (postincrement) areg[PARAM_M]=ad | areg_hi;
  m68k_dpeek(ad); //extra word read (discarded)
  INSTRUCTION_TIME_ROUND(4);
#if !defined(SSE_CPU_PREFETCH_TIMING_MOVEM_HACK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
  if (ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_R){  //oh dear, reading multiple words from the PSG
    int s=count_bits_set_in_word(m68k_src_w)+1; //number of words read
    if(s>4)BUS_JAM_TIME((s-1)&-4);  //we've already had a bus jam of 4, for s=5..8 want extra bus jam of 4
  }
}

void                              m68k_jsr()
{
#if !defined(SSE_CPU_ROUDING_EA)
  // see jmp instruction times table and +8.
  if((ir&B6_111111)==B6_111001){ INSTRUCTION_TIME(0);}//SS (xxx).L
  else if((ir&BITS_543)==BITS_543_010){ INSTRUCTION_TIME(4);}//SS (An),+round
  else {INSTRUCTION_TIME(2);}//SS  (d16,An) (d8,An,Xn) (xxx).L
  m68k_get_effective_address();
#endif
#if defined(SSE_CPU_ROUDING_EA)
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       JSR        |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------
<ea> :            |                 |                
    (An)          | 16(2/2)         |                      np nS ns np          
    (d16,An)      | 18(2/2)         |                 n    np nS ns np          
    (d8,An,Xn)    | 22(2/2)         |                 n nn np nS ns np          
    (xxx).W       | 18(2/2)         |                 n    np nS ns np          
    (xxx).L       | 20(3/2)         |                   np np nS ns np          
*/
  switch (ir & BITS_543){
    case BITS_543_010: //SS (An)
      // (An)          | 16(2/2)         |                      np nS ns np          
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ_PC; // = prefetch but Steem doesn't work like that yet
#endif
      effective_address=areg[PARAM_M];
      break;
    case BITS_543_101: //SS (d16,An)
      // (d16,An)      | 18(2/2)         |                 n    np nS ns np          
      INSTRUCTION_TIME(2); //immediate was already prefetched...
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ_FETCH; //...so this really fetches next ir (see FetchWord)
#endif
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_543_110: //SS (d8,An,Xn)
      // (d8,An,Xn)    | 22(2/2)         |                 n nn np nS ns np          
      INSTRUCTION_TIME(6);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ_FETCH;
#endif
      m68k_iriwo=m68k_fetchW();pc+=2; 
      if (m68k_iriwo & BIT_b){  //.l
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_543_111:
      switch (ir & 0x7){
      case 0: //SS (xxx).W
        // (xxx).W       | 18(2/2)         |                 n    np nS ns np          
        INSTRUCTION_TIME(2);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
#endif
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
#if !defined(SSE_CPU_TRUE_PC2)
          TRUE_PC+=2;
#endif
#endif
        break;
      case 1: //SS (xxx).L
        // (xxx).L       | 20(3/2)         |                   np np nS ns np     
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
        CPU_ABUS_ACCESS_READ_FETCH;
#else
        CPU_ABUS_ACCESS_READ;
#endif
        effective_address=m68k_fetchL();
        pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
#if !defined(SSE_CPU_TRUE_PC2)
        TRUE_PC+=4;
#endif
#endif
        break;
      case 2: //SS (d16,PC) 
        INSTRUCTION_TIME(2);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
#endif
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;
      case 3: //SS (d8,PC,Xn)
        INSTRUCTION_TIME(6);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH
#endif
        m68k_iriwo=m68k_fetchW();
        if (m68k_iriwo & BIT_b){  //.l
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]) | pc_high_byte;
        }else{         //.w
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]) | pc_high_byte;
        }
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;       //what do bits 8,9,a  of extra word do?  (not always 0)
      default:
        m68k_unrecognised();
        break;
      }
      break;
    default:
      m68k_unrecognised();
  }
#if !defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
  CPU_ABUS_ACCESS_READ; 
#endif
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_ROUNDING_JSR))
  INSTRUCTION_TIME_ROUND(8);
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CALL)
  // read new PC before pushing current PC; fixes nothing AFAIK
  M68000.PrefetchClass=1; 
#if defined(SSE_CPU_ROUNDING_BUS)
  M68000.Rounded=false;
#endif
  m68k_READ_W(effective_address); // Check for bus/address errors
  FETCH_TIMING; // Fetch from new address before setting PC
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC) \
  && !defined(SSE_CPU_ROUNDING_JSR)
  INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
  M68000.FetchForCall(effective_address); // fetch before writing stack

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_ROUNDING_JSR)
  // 8 cycles more than JMP because we push PC
  CPU_ABUS_ACCESS_WRITE;
  CPU_ABUS_ACCESS_WRITE;
#endif
  m68k_PUSH_L(PC32); 
#else
  m68k_PUSH_L(PC32);
  FETCH_TIMING; // Fetch from new address before setting PC
  m68k_READ_W(effective_address); // Check for bus/address errors
#endif
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPush(PC32);
#endif
  SET_PC(effective_address); //SS where prefetch happens
#if defined(SSE_CPU_PREFETCH_TIMING_JSR)
  CPU_ABUS_ACCESS_READ_PC;
#endif
  intercept_os();
}


void                              m68k_jmp()
{
#if !defined(SSE_CPU_ROUDING_EA)
  // jmp instruction times table.
  // ad.mode  time  Steem EA time difference
  // (aN)     8     0             8
  // D(aN)    10    4             6
  // D(aN,dM) 14    8             6
  // xxx.w    10    4             6
  // xxx.l    12    8             4
  // D(pc)    10    4             6
  // D(pc,dM) 14    8             6
  if((ir&B6_111111)==B6_111001){ INSTRUCTION_TIME(0);}  // abs.l
  else if((ir&BITS_543)==BITS_543_010){ INSTRUCTION_TIME(4);} // (an)
  else {INSTRUCTION_TIME(2);}
  m68k_get_effective_address();
#endif
#if defined(SSE_CPU_ROUDING_EA)
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       JMP        |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------
<ea> :            |                 |                          
    (An)          |  8(2/0)         |                      np       np          
    (d16,An)      | 10(2/0)         |                 n    np       np          
    (d8,An,Xn)    | 14(2/0)         |                 n nn np       np          
    (xxx).W       | 10(2/0)         |                 n    np       np          
    (xxx).L       | 12(3/0)         |                   np np       np          
NOTES :
  .M68000UM is probably wrong with bus read accesses for JMP (d8,An,Xn),Dn 
   instruction. It reads "14(3/0)" but, according to USP4325121 and with a 
   little common sense, 2 bus read accesses are far enough.
*/

  switch (ir & BITS_543){
    case BITS_543_010: //SS (An)
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ_PC; // see JSR
#endif
      effective_address=areg[PARAM_M];
      break;
    case BITS_543_101: //SS (d16,An)
      INSTRUCTION_TIME(2);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ_FETCH;
#endif
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_543_110: //SS (d8,An,Xn)
      INSTRUCTION_TIME(6);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      CPU_ABUS_ACCESS_READ_FETCH;
#endif
      m68k_iriwo=m68k_fetchW();pc+=2; 
      if (m68k_iriwo & BIT_b){  //.l
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_543_111:
      switch (ir & 0x7){
      case 0: //SS (xxx).W
        INSTRUCTION_TIME(2);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
#endif
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
#if !defined(SSE_CPU_TRUE_PC2)
        TRUE_PC+=2;
#endif
#endif
        break;
      case 1: //SS (xxx).L
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
        CPU_ABUS_ACCESS_READ_FETCH;
#else
        CPU_ABUS_ACCESS_READ;
#endif
        effective_address=m68k_fetchL();
        pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
#if !defined(SSE_CPU_TRUE_PC2)
        TRUE_PC+=4;
#endif
#endif
        break;
      case 2: //SS (d16,PC) 
        INSTRUCTION_TIME(2);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
#endif
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;
      case 3: //SS (d8,PC,Xn)
        INSTRUCTION_TIME(6);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
#endif
        m68k_iriwo=m68k_fetchW();
        if (m68k_iriwo & BIT_b){  //.l
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]) | pc_high_byte;
        }else{         //.w
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]) | pc_high_byte;
        }
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;       //what do bits 8,9,a  of extra word do?  (not always 0)
      default:
        m68k_unrecognised();
        break;
      }
      break;
    default:
      m68k_unrecognised();
  }
#endif
  FETCH_TIMING; // Fetch from new address before setting PC
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC) \
  && !defined(SSE_CPU_PREFETCH_TIMING_JMP)
  INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#if defined(SSE_CPU_ROUNDING_BUS)
  M68000.Rounded=false;
#endif
  m68k_READ_W(effective_address); // Check for bus/address errors
#if !defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
  CPU_ABUS_ACCESS_READ; // there are 2 prefetches in those instructions
#endif
  SET_PC(effective_address);  
#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_JMP))
  CPU_ABUS_ACCESS_READ_PC;
#endif
  intercept_os();
}
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        CHK       |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
no trap :         |                 |               |
  <ea>,Dn :       |                 |              /  
    .W :          |                 |             /  
      Dn          | 10(1/0)  0(0/0) |            |                  np nn n     
      (An)        | 10(1/0)  4(1/0) |         nr |                  np nn n     
      (An)+       | 10(1/0)  4(1/0) |         nr |                  np nn n     
      -(An)       | 10(1/0)  6(1/0) | n       nr |                  np nn n     
      (d16,An)    | 10(1/0)  8(2/0) |      np nr |                  np nn n     
      (d8,An,Xn)  | 10(1/0) 10(2/0) | n    np nr |                  np nn n     
      (xxx).W     | 10(1/0)  8(2/0) |      np nr |                  np nn n     
      (xxx).L     | 10(1/0) 12(2/0) |   np np nr |                  np nn n     
      #<data>     | 10(1/0)  4(1/0) |      np    |                  np nn n     
trap :            |                 |            |
  <ea>,Dn :       |                 |            |  
    .W :          |                 |            |
      Dn > Src :  |                 |            |
        Dn        | 38(5/3)  0(0/0) |            |np    nn ns nS ns np np np np 
        (An)      | 38(5/3)  4(1/0) |         nr |np    nn ns nS ns np np np np 
        (An)+     | 38(5/3)  4(1/0) |         nr |np    nn ns nS ns np np np np 
        -(An)     | 38(5/3)  6(1/0) | n       nr |np    nn ns nS ns np np np np 
        (d16,An)  | 38(5/3)  8(2/0) |      np nr |np    nn ns nS ns np np np np 
        (d8,An,Xn)| 38(5/3) 10(2/0) | n    np nr |np    nn ns nS ns np np np np 
        (xxx).W   | 38(5/3)  8(2/0) |      np nr |np    nn ns nS ns np np np np 
        (xxx).L   | 38(5/3) 12(2/0) |   np np nr |np    nn ns nS ns np np np np 
        #<data>   | 38(5/3)  4(1/0) |      np    |np    nn ns nS ns np np np np 
      Dn <0 :     |                 |            |
        Dn        | 40(5/3)  0(0/0) |            |np n- nn ns nS ns np np np np 
        (An)      | 40(5/3)  4(1/0) |         nr |np n- nn ns nS ns np np np np 
        (An)+     | 40(5/3)  4(1/0) |         nr |np n- nn ns nS ns np np np np 
        -(An)     | 40(5/3)  6(1/0) | n       nr |np n- nn ns nS ns np np np np 
        (d16,An)  | 40(5/3)  8(2/0) |      np nr |np n- nn ns nS ns np np np np 
        (d8,An,Xn)| 40(5/3) 10(2/0) | n    np nr |np n- nn ns nS ns np np np np 
        (xxx).W   | 40(5/3)  8(2/0) |      np nr |np n- nn ns nS ns np np np np 
        (xxx).L   | 40(5/3) 12(2/0) |   np np nr |np n- nn ns nS ns np np np np 
        #<data>   | 40(5/3)  4(1/0) |      np    |np n- nn ns nS ns np np np np 
NOTES :
  . for more informations about the "trap" lines see exceptions section below.

*/
void                              m68k_chk(){
  #if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  if((ir&BITS_543)==BITS_543_001){
    m68k_unrecognised();
  }else{
    m68k_GET_SOURCE_W;
#if defined(SSE_CPU_PREFETCH_TIMING_CHK)
    PREFETCH_IRC;
    INSTRUCTION_TIME(6);
#else
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("CHK");
#endif
    // ss do later if trap
    if(r[PARAM_N]&0x8000){
      SR_SET(SR_N);
      m68k_interrupt(LPEEK(BOMBS_CHK*4));
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
      INSTRUCTION_TIME(40-4);
      FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
      CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
      INSTRUCTION_TIME_ROUND(40);
#endif
    }else if((signed short)LOWORD(r[PARAM_N])>(signed short)m68k_src_w){
      SR_CLEAR(SR_N);
      m68k_interrupt(LPEEK(BOMBS_CHK*4));
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
#if defined(SSE_CPU_ROUNDING_CHK)
      INSTRUCTION_TIME(28-4); //10 already counted + 2 fewer acording to Yacht
#else
      INSTRUCTION_TIME(40-4);
#endif
      FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
      CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
      INSTRUCTION_TIME_ROUND(40);
#endif
    }
#if !defined(SSE_CPU_PREFETCH_TIMING_CHK)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CHK)
    else
    {
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
      FETCH_TIMING;
#endif
    }
#endif
#endif
  }
}


void                              m68k_lea(){
#if !defined(SSE_CPU_ROUDING_EA)
  // lea instruction times table.
  // ad.mode  time  Steem EA time difference
  // (aN)     4     0             4
  // D(aN)    8     4             4
  // D(aN,dM) 14    8             6
  // xxx.w    8     4             4
  // xxx.l    12    8             4
  // D(pc)    8     4             4
  // D(pc,dM) 14    8             6
  if ((ir & B6_111111)==B6_111011 || (ir & B6_111000)==B6_110000){ INSTRUCTION_TIME(2); }
  m68k_get_effective_address();
#endif
#if defined(SSE_CPU_ROUDING_EA)
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        LEA       |      INSTR      |                  INSTR                   
------------------+-----------------+------------------------------------------
<ea>,An :         |                 |               
  .L :            |                 |               
    (An)          |  4(1/0)         |                               np          
    (d16,An)      |  8(2/0)         |                          np   np          
    (d8,An,Xn)    | 12(2/0)         |                        n np n np          
    (xxx).W       |  8(2/0)         |                          np   np          
    (xxx).L       | 12(3/0)         |                       np np   np          
*/
  switch (ir & BITS_543){
    case BITS_543_010: //SS (An)
      effective_address=areg[PARAM_M];
      break;
    case BITS_543_101: //SS (d16,An)
      CPU_ABUS_ACCESS_READ_FETCH;
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_543_110: //SS (d8,An,Xn)
      INSTRUCTION_TIME(2);
      CPU_ABUS_ACCESS_READ_FETCH;
      m68k_iriwo=m68k_fetchW();pc+=2; 
      INSTRUCTION_TIME(2);
      if (m68k_iriwo & BIT_b){  //.l
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_543_111:
      switch (ir & 0x7){
      case 0: //SS (xxx).W
        CPU_ABUS_ACCESS_READ_FETCH;
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
          TRUE_PC+=2;
#endif
        break;
      case 1: //SS (xxx).L
        CPU_ABUS_ACCESS_READ_FETCH;
        CPU_ABUS_ACCESS_READ_FETCH;
        effective_address=m68k_fetchL();
        pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
        TRUE_PC+=4;
#endif
        break;
      case 2: //SS (d16,PC) 
        CPU_ABUS_ACCESS_READ_FETCH;
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;
      case 3: //SS (d8,PC,Xn)
        INSTRUCTION_TIME(2);
        CPU_ABUS_ACCESS_READ_FETCH;
        m68k_iriwo=m68k_fetchW();
        INSTRUCTION_TIME(2);
        if (m68k_iriwo & BIT_b){  //.l
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]) | pc_high_byte;
        }else{         //.w
          effective_address=(PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]) | pc_high_byte;
        }
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;       //what do bits 8,9,a  of extra word do?  (not always 0)
      default:
        m68k_unrecognised();
        break;
      }
      break;
    default:
      m68k_unrecognised();
  }
#endif
#if !defined(SSE_CPU_PREFETCH_TIMING_LEA)
  PREFETCH_IRC;
#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#endif
  areg[PARAM_N]=effective_address;
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING; /// This seems strange but it is right, it fetches after instruction
#endif
#if defined(SSE_CPU_PREFETCH_TIMING_LEA) // and Steem authors did it right to boot
  PREFETCH_IRC;
#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#endif

}


void                              m68k_line_4_stuff(){
  m68k_jump_line_4_stuff[ir&(BITS_543|0x7)]();
}

#define LOGSECTION LOGSECTION_TRAP
void                              m68k_trap(){
  INSTRUCTION_TIME_ROUND(8); // Time to read address to jump to
  MEM_ADDRESS Vector=LPEEK( 0x80+((ir & 0xf)*4) );
  switch (ir & 0xf){
    case 1: //GEMDOS
      if (os_gemdos_vector==0) if (Vector>=rom_addr) os_gemdos_vector=Vector;
      break;
    case 13: // BIOS
      if (os_bios_vector==0) if (Vector>=rom_addr) os_bios_vector=Vector;
      break;
    case 14: // XBIOS
      if (os_xbios_vector==0) if (Vector>=rom_addr) os_xbios_vector=Vector;
      break;
  }
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("TRAP",(ir&0xF));
#endif
//#ifdef SSE_DEBUG
#ifdef DEBUG_BUILD
  if((ir&0xf)!=1)
    //TRACE_LOG("PC %X TRAP #%d, %X\n",old_pc,(ir&0xf),DPEEK(areg[7]));//unsafe!
    TRACE_LOG("PC %X TRAP #%d, %X\n",old_pc,(ir&0xf),d2_dpeek(areg[7]));
#endif
  m68k_interrupt(Vector);
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  INSTRUCTION_TIME(26-4);
  FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
  CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
  INSTRUCTION_TIME_ROUND(26);
#endif
  intercept_os();
  debug_check_break_on_irq(BREAK_IRQ_TRAP_IDX);
}
#undef LOGSECTION

void                              m68k_link(){
/*
Pushes the contents of the specified address register onto the stack. Then
loads the updated stack pointer into the address register. Finally, adds the
displacement value to the stack pointer.
*/
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=1; 
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif

  INSTRUCTION_TIME(12);
  m68k_GET_IMMEDIATE_W;
  m68k_PUSH_L(areg[PARAM_M]);
  areg[PARAM_M]=r[15];
  r[15]+=(signed short)m68k_src_w;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif

}
void                              m68k_unlk(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif

  INSTRUCTION_TIME(8);
  r[15]=areg[PARAM_M];
  abus=r[15];m68k_READ_L_FROM_ADDR;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif

  r[15]+=4;    //This is contrary to the Programmer's reference manual which says
  areg[PARAM_M]=m68k_src_l; //it does move (a7),An then adds 4 to a7, but it fixed Wrath of Demon
}
void                              m68k_move_to_usp(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  if (SUPERFLAG){
    other_sp=areg[PARAM_M];
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING;
#endif
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}
void                              m68k_move_from_usp(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif

  if (SUPERFLAG){
    areg[PARAM_M]=other_sp;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}

void                              m68k_reset(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
  if (SUPERFLAG){
    reset_peripherals(0);
    INSTRUCTION_TIME(128);
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}

void                              m68k_nop(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_NOP)
  PREFETCH_IRC; // quite the opposite!
  // note rounding necessary, eg Overscan Demos menu
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#else
  prefetched_2=false;
  prefetch_buf[0]=*(lpfetch-MEM_DIR);  //flush prefetch queue
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
  FETCH_TIMING;
#endif
#endif
}


void                              m68k_stop(){

  if (SUPERFLAG){
    if (cpu_stopped==0){
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING) //36.15 OVR
//STOP	4(0/0)	
//stop reads the immediate and doesn't prefetch
#else
      FETCH_TIMING;
#endif
#if defined(SSE_CPU_PREFETCH_TIMING_STOP)
      CPU_ABUS_ACCESS_READ_FETCH;
#endif
      m68k_GET_IMMEDIATE_W;
#if !defined(SSE_CPU_PREFETCH_TIMING_STOP)
      INSTRUCTION_TIME_ROUND(4); // time for immediate fetch
#endif
      DEBUG_ONLY( int debug_old_sr=sr; )

      sr=m68k_src_w; // SS contains IPL
      sr&=SR_VALID_BITMASK;
      DETECT_CHANGE_TO_USER_MODE;
      cpu_stopped=true;
      SET_PC((pc-4) | pc_high_byte); // SS: go back on the stop
      DETECT_TRACE_BIT;
      // Interrupts must come after trace exception
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
//      check_for_interrupts_pending(); // was commented out

      CHECK_STOP_ON_USER_CHANGE;
    }else{ //SS already stopped
      SET_PC((pc-2) | pc_high_byte);
      // If we have got here then there were no interrupts pending when the IPL
      // was changed. Now unless we are in a blitter loop nothing can possibly
      // happen until cpu_cycles<=0.
      if (Blit.Busy){
        INSTRUCTION_TIME_ROUND(4);
      }else if (cpu_cycles>0){
        cpu_cycles=0; // It takes 0 cycles to unstop
      }
    }
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}
void                              m68k_rte(){
#if !defined(SSE_TOS_NO_INTERCEPT_ON_RTE1)
/*  Megamax C would hang while trying to compile the ReDMCSB project.
    http://www.dungeon-master.com/forum/viewtopic.php?f=25&t=29805
    The bug was tracked down to this part. Gemdos command PTerm() would
    be executed twice, unduly terminating another process:
    Apparently Timer C triggers while the TOS trap is executing. That's
    why there's a random aspect in the hanging: depends on when we launch
    the program.
    At the RTE of timer C ($FC431A), Steem peeks the stack and finds the PTerm
    command again.

Trap 1 $40 Fwrite(handle=0009,count=000000C3,buf=0002A244)
Trap 1 $49 Mfree(addr=0002A244)
Trap 1 $3E Fclose(handle=0009)
Close file 9
Trap 1 $49 Mfree(addr=0002A044)
Trap 1 $4C Pterm(retcode=0000)
PC 24FEE terminate process 4
Trap 1 $4C Pterm(retcode=0000)
PC FC07A0 terminate process 3
pexec, close file 6
pexec, close file 7
pexec, close file 8
Trap 1 $3F Fread(handle=0008,count=00000001,buf=0001ED58)
File 8 not open
    The file 8 is BUILDINI.BAT
    When SSE_TOS_NO_INTERCEPT_ON_RTE1 is defined the project will compile 
    to completion, but we don't know what this mod could break. 
    Why intercept at the RTE?
    Disabling this is radical so: TESTING
    If it breaks anything we could try to disable only gemdos 
    intercept on RTE. // v361: done
Also see SSE_TOS_GEMDOS_PEXEC6 for ReDMCSB
*/
  bool dont_intercept_os=false;
#endif
  if (SUPERFLAG){
    DEBUG_ONLY( int debug_old_sr=sr; )
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    INSTRUCTION_TIME_ROUND(20-4); //v3.7 rounds up, changes nothing
    FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
    CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
    INSTRUCTION_TIME_ROUND(20);
#endif
    M68K_PERFORM_RTE(;);

    log_to(LOGSECTION_INTERRUPTS,Str("INTERRUPT: ")+HEXSl(old_pc,6)+" - RTE to "+HEXSl(pc,6)+" sr="+HEXSl(sr,4)+
                                  " at "+ABSOLUTE_CPU_TIME+" idepth="+interrupt_depth);
    if (on_rte){
      if (on_rte_interrupt_depth==interrupt_depth){  //is this the RTE we want?
        switch (on_rte){
#ifdef DEBUG_BUILD
          case ON_RTE_STOP:
            if (runstate==RUNSTATE_RUNNING){
              runstate=RUNSTATE_STOPPING;
              SET_WHY_STOP(HEXSl(old_pc,6)+": RTE");
            }
            on_rte=ON_RTE_RTE;
            break;
#endif
#ifndef DISABLE_STEMDOS
          case ON_RTE_STEMDOS:
            //ASSERT( stemdos_rte_action!=STEMDOS_RTE_MALLOC );
            stemdos_rte();
#if !defined(SSE_TOS_NO_INTERCEPT_ON_RTE1)
            dont_intercept_os=true;
#endif
            break;
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_F)
          case ON_RTE_LINE_F:
            interrupt_depth++; // there's a RTE after all, cancel 'compensate'
            //on_rte=ON_RTE_RTE;
            break;
#endif

#ifndef NO_CRAZY_MONITOR
          case ON_RTE_LINE_A:
            on_rte=ON_RTE_RTE;
            SET_PC(on_rte_return_address);
            extended_monitor_hack();
            break;

          case ON_RTE_DONE_MALLOC_FOR_EM:
//            log_write(HEXSl(pc,6)+EasyStr(": Malloc done - returned $")+HEXSl(r[0],8));
            xbios2=(r[0]+255)&-256;
            LPEEK(SV_v_bas_ad)=xbios2;
            LPEEK(SVscreenpt)=xbios2;
            memcpy(r,save_r,16*4);
            on_rte=ON_RTE_RTE;
//            log_write_stack();
            break;
          case ON_RTE_EMHACK:
            on_rte=ON_RTE_RTE;
            extended_monitor_hack();
            break;
#endif
        }
      }
    }
//    log(EasyStr("RTE - decreasing interrupt depth from ")+interrupt_depth+" to "+(interrupt_depth-1));
    interrupt_depth--;
    ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
//    check_for_interrupts_pending();// was commented out
#if !defined(SSE_TOS_NO_INTERCEPT_ON_RTE1)
#if defined(SSE_TOS_NO_INTERCEPT_ON_RTE2) && !defined(DISABLE_STEMDOS)
/*  This is less heavy-handed than the 1st mod, but we need a new
    variable.
    It's better but not enough for ReDMCSB, that will only complete 50%
    with TOS1.02.
*/
    if(stemdos_command==0x4c)// && stemdos_rte_action!=STEMDOS_RTE_MFREE2)
    {
      //if(stemdos_Pexec_list_ptr==debug1)
      if(stemdos_Pexec_list_ptr==Tos.LastPTermedProcess)
      {
       // TRACE("refuse intercept OS for $%X\n",stemdos_Pexec_list_ptr);
        dont_intercept_os=true;
      }
      else
        Tos.LastPTermedProcess=stemdos_Pexec_list_ptr;
    }
#endif
    if (!dont_intercept_os) intercept_os();
#endif
    CHECK_STOP_ON_USER_CHANGE;
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}
void                              m68k_rtd(){
//  INSTRUCTION_TIME_ROUND(20); // was commented out
//  m68k_GET_IMMEDIATE_W; // was commented out
  m68k_unrecognised();
}
void                              m68k_rts(){
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  INSTRUCTION_TIME(16-4);
  FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
  INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#else
  INSTRUCTION_TIME_ROUND(16);
#endif
  effective_address=m68k_lpeek(r[15]);
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPop();
#endif
  r[15]+=4;
  m68k_READ_W(effective_address); // Check for bus/address errors
  SET_PC(effective_address);
  intercept_os();
#if defined(SSE_BOILER_RUN_TO_RTS)
  if(on_rte==ON_RTS_STOP)
  {
    if (runstate==RUNSTATE_RUNNING){
      runstate=RUNSTATE_STOPPING;
      SET_WHY_STOP(HEXSl(old_pc,6)+": RTS");
    }
    on_rte=ON_RTE_RTE;
  }
#endif

}
void                              m68k_trapv(){
  if (sr & SR_V){
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("TRAPV");
#endif
    m68k_interrupt(LPEEK(BOMBS_TRAPV*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    INSTRUCTION_TIME(34-4);
    FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
    INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#else
    INSTRUCTION_TIME_ROUND(34);
#endif
  }else{
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_4_TIMINGS)
    FETCH_TIMING_NO_ROUND; //?
#else
    INSTRUCTION_TIME(4);
#endif
    PREFETCH_IRC;

  }
}
void                              m68k_rtr(){
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  INSTRUCTION_TIME(20-4);
  FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
    INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#else
  INSTRUCTION_TIME_ROUND(20);
#endif
  CCR=LOBYTE(m68k_dpeek(r[15]));r[15]+=2;
  sr&=SR_VALID_BITMASK;

  effective_address=m68k_lpeek(r[15]);r[15]+=4;
#if defined(SSE_CPU_ROUNDING_BUS)
  M68000.Rounded=false;
#endif
  m68k_READ_W(effective_address); // Check for bus/address errors
  SET_PC(effective_address);
  intercept_os();
}
void                              m68k_movec(){
  m68k_unrecognised();
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/////////////////  LINE 5 - ADDQ, SUBQ, SCC, DBCC     //////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
    ADDQ, SUBQ    |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
#<data>,<ea> :    |                 |               |
  .B or .W :      |                 |               |
    Dn            |  4(1/0)  0(0/0) |               |               np          
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       
  .L :            |                 |               |
    Dn            |  8(1/0)  0(0/0) |               |               np       nn 
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    
NOTES :
  .M68000UM is probably wrong with instruction timing for ADDQ.W #<data>,An 
   instruction. It reads "4(1/0)" but, according to USP4325121, this 
   instruction is based on the same microwords than ADDQ.L #<data>,Dn and 
   ADDQ.L #<data>,An that have a "8(3/0)" timing. That makes sense because of 
   the calculation of a 32 bit address on 16 bits ALU. In addition, evaluation 
   on real hardware confirms the 8 cycles timing and, last, it makes addq and 
   subbq coherent in term of timing.
*/

void                              m68k_addq_b(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
  FETCH_TIMING;
#endif
#if !defined(SSE_CPU_ROUNDING_ADDQ)
  INSTRUCTION_TIME(4);
#endif
  m68k_src_b=(BYTE)PARAM_N;if(m68k_src_b==0)m68k_src_b=8;
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_ADDQ)
  m68k_GET_DEST_B_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
  m68k_GET_DEST_B_NOT_A;
#endif
  m68k_old_dest=m68k_DEST_B;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
  if(!DEST_IS_DATA_REGISTER)
  {
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_B+=m68k_src_b;
  SR_ADD_B;
}

void                             m68k_addq_w(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_src_w=(WORD)PARAM_N;if(m68k_src_w==0)m68k_src_w=8;
  if((ir&BITS_543)==BITS_543_001){ //addq.w to address register
//     An            |  8(1/0)  0(0/0) |               |               np       nn 

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
    PREFETCH_IRC;
    INSTRUCTION_TIME(4);
#else
    INSTRUCTION_TIME(4);
    PREFETCH_IRC;
#endif
    areg[PARAM_M]+=m68k_src_w;
  }else{
/*
    Dn            |  4(1/0)  0(0/0) |               |               np          

    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       

*/

#if !defined(SSE_CPU_ROUNDING_ADDQ)
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
#if defined(SSE_CPU_DATABUS)
  //  M68000.dbus=IRC;
#endif
#if !defined(SSE_CPU_ROUNDING_ADDQ)
    m68k_GET_DEST_W_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
    m68k_GET_DEST_W_NOT_A;
#endif
    m68k_old_dest=m68k_DEST_W;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
    if(!DEST_IS_DATA_REGISTER)
    {
      CPU_ABUS_ACCESS_WRITE;
    }
#endif
    m68k_DEST_W+=m68k_src_w;
#if defined(SSE_CPU_DATABUS)
    M68000.dbus=m68k_old_dest+m68k_src_w;
#endif
    SR_ADD_W;
  }
}

void                             m68k_addq_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_src_l=(LONG)PARAM_N;if(m68k_src_l==0)m68k_src_l=8;
  if((ir&BITS_543)==BITS_543_001){ //addq.l to address register
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
//  An            |  8(1/0)  0(0/0) |               |               np       nn 

    PREFETCH_IRC;
    INSTRUCTION_TIME(4);
    areg[PARAM_M]+=m68k_src_l;
#else
    INSTRUCTION_TIME(4);
    PREFETCH_IRC;
    areg[PARAM_M]+=m68k_src_l;
#endif

  }else{
/*
    Dn            |  8(1/0)  0(0/0) |               |               np       nn 

    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    
*/

#if !defined(SSE_CPU_ROUNDING_ADDQ)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_ADDQ)
    m68k_GET_DEST_L_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
    m68k_GET_DEST_L_NOT_A;
#endif
    m68k_old_dest=m68k_DEST_L;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_ADDQ)
    if(DEST_IS_DATA_REGISTER)
      INSTRUCTION_TIME(4);
    else
    {
      CPU_ABUS_ACCESS_WRITE;
      CPU_ABUS_ACCESS_WRITE;
    }
#endif
    m68k_DEST_L+=m68k_src_l;
    SR_ADD_L;
  }
}

void                              m68k_subq_b(){
/*
    Dn            |  4(1/0)  0(0/0) |               |               np          
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       
*/
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_src_b=(BYTE)PARAM_N;if(m68k_src_b==0)m68k_src_b=8;
#if !defined(SSE_CPU_ROUNDING_SUBQ)
  INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_SUBQ)
  m68k_GET_DEST_B_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_SUBQ)
  m68k_GET_DEST_B_NOT_A;
#endif
  m68k_old_dest=m68k_DEST_B;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_SUBQ)
  if(!DEST_IS_DATA_REGISTER)
  {
    CPU_ABUS_ACCESS_WRITE;
  }
#endif
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(SR_X);
}

void                             m68k_subq_w(){
/*
    Dn            |  4(1/0)  0(0/0) |               |               np          
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       
*/
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_src_w=(WORD)PARAM_N;if(m68k_src_w==0)m68k_src_w=8;
  if((ir&BITS_543)==BITS_543_001){ //subq.w to address register
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_SUBQ)
    PREFETCH_IRC;
    INSTRUCTION_TIME(4);
#else
    INSTRUCTION_TIME(4);
    PREFETCH_IRC;
#endif
    areg[PARAM_M]-=m68k_src_w;
  }else{
#if !defined(SSE_CPU_ROUNDING_SUBQ)
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUNDING_SUBQ)
    m68k_GET_DEST_W_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_SUBQ)
    m68k_GET_DEST_W_NOT_A;
#endif
    m68k_old_dest=m68k_DEST_W;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_SUBQ)
    if(!DEST_IS_DATA_REGISTER)
    {
      CPU_ABUS_ACCESS_WRITE;
    }
#endif
    m68k_DEST_W-=m68k_src_w;
#if defined(SSE_CPU_DATABUS)
//    M68000.dbus=m68k_old_dest-m68k_src_w;
#endif
    SR_SUB_W(SR_X);
  }
}

//#undef SSE_CPU_ROUNDING_SUBQ

void                             m68k_subq_l(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_src_l=(LONG)PARAM_N;if(m68k_src_l==0)m68k_src_l=8;
  if((ir&BITS_543)==BITS_543_001){ //subq.l to address register

#if defined(SSE_CPU_ROUNDING_SUBQ)
    PREFETCH_IRC;
    INSTRUCTION_TIME(4);
    areg[PARAM_M]-=m68k_src_l;
#else
    areg[PARAM_M]-=m68k_src_l;
    INSTRUCTION_TIME(4);
    PREFETCH_IRC;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
  }else{
#if !defined(SSE_CPU_ROUNDING_SUBQ)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
//#undef SSE_CPU_ROUNDING_SUBQ
#if !defined(SSE_CPU_ROUNDING_SUBQ)
    m68k_GET_DEST_L_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUNDING_SUBQ)
    m68k_GET_DEST_L_NOT_A;
#endif
    m68k_old_dest=m68k_DEST_L;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_SUBQ)
    if(DEST_IS_DATA_REGISTER)
      INSTRUCTION_TIME(4);
    else
    {
      CPU_ABUS_ACCESS_WRITE;
      CPU_ABUS_ACCESS_WRITE;
    }
#endif
    m68k_DEST_L-=m68k_src_l;
    SR_SUB_L(SR_X);
  }
}
//#undef SSE_CPU_LINE_5_TIMINGS

void                              m68k_dbCC_or_sCC(){
  if ((ir&BITS_543)==BITS_543_001){ // DBCC
/*
If Condition False 
Then (Dn - 1 -> Dn; If Dn  <> -1 Then PC + dn -> PC) 
*/

/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       DBcc       |      INSTR      |                   INSTR
------------------+-----------------+------------------------------------------
Dn,<label> :      |                 |                               
  branch taken    | 10(2/0)         |                      n np       np        
  branch not taken|                 |                                           
    cc true       | 12(2/0)         |                      n     n np np        
    counter exp   | 14(3/0)         |                      n np    np np        
NOTES :                                                                         
  .DBcc does not exist in USP4325121. Only does a primitive instruction DCNT    
   tht doesn't succeded to reach the production version of M68000 CPU.          
   Nervertheless, looking at DCNT and Bcc micro/nanowords can lead to a         
   realistic idea of how DBcc should work. At least, this re-enginering of DBcc 
   matches the timing written in M68000UM for this instruction.                 
FLOWCHART :                                                                     
                   cc                  n                 /cc                    
                   +-------------------+-------------------+                    
                   n                             z         np       /z          
                   |                             +---------+---------+          
                   +---------------------------->np                  |          
                                                 |                   |          
                                                 np<-----------------+          

*/
//#undef SSE_CPU_ROUNDING_DBCC

#if !defined(SSE_CPU_ROUNDING_DBCC)
    INSTRUCTION_TIME(6);
    m68k_GET_IMMEDIATE_W;
#endif
#if defined(SSE_CPU_ROUNDING_DBCC)
    INSTRUCTION_TIME(2); //do the test
    CPU_ABUS_ACCESS_READ_FETCH; // get displacement (whether we go or not...)
    m68k_GET_IMMEDIATE_W;
#endif

    if (!m68k_CONDITION_TEST){
      (*((WORD*)(&(r[PARAM_M]))))--;
      if( (*( (signed short*)(&(r[PARAM_M]) ))) != (signed short)(-1) ){
        MEM_ADDRESS new_pc=(pc+(signed short)m68k_src_w-2) | pc_high_byte;
#if defined(SSE_CPU_ROUNDING_BUS)
        M68000.Rounded=false;
#endif
        m68k_READ_W(new_pc); // Check for bus/address errors
        SET_PC(new_pc); // SS branch taken (prefetch in SET_PC)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
        FETCH_TIMING;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
#if defined(SSE_CPU_ROUNDING_DBCC)
        CPU_ABUS_ACCESS_READ_PC; // SET_PC doesn't count cycles (TODO?)
#else
        INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#endif
      }else{ //SS end of loop
#if defined(SSE_CPU_ROUNDING_DBCC)
        CPU_ABUS_ACCESS_READ; // but what for?
#else
        INSTRUCTION_TIME(4);
#endif
        PREFETCH_IRC; //branch not taken
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
        FETCH_TIMING;
#endif
    }
    }else{ //SS condition true
      INSTRUCTION_TIME(2);//SS what for?
#if defined(SSE_CPU_ROUNDING_DBCC__)
      //CPU_ABUS_ACCESS_READ; //CPU prefetches once for nothing
      m68k_GET_IMMEDIATE_W; 
#endif
      PREFETCH_IRC; //branch not taken
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
      FETCH_TIMING;
#endif

    }
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
  FETCH_TIMING;
#endif
//#define SSE_CPU_LINE_5_TIMINGS
  }else{ // SCC

/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
        Scc       |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea> :            |                 |               |
  .B :            |                 |               |
    cc false      |                 |               |
      Dn          |  4(1/0)  0(0/0) |               |               np          
      (An)        |  8(1/1)  4(1/0) |            nr |               np    nw    
      (An)+       |  8(1/1)  4(1/0) |            nr |               np    nw    
      -(An)       |  8(1/1)  6(1/0) | n          nr |               np    nw    
      (d16,An)    |  8(1/1)  8(2/0) |      np    nr |               np    nw    
      (d8,An,Xn)  |  8(1/1) 10(2/0) | n    np    nr |               np    nw    
      (xxx).W     |  8(1/1)  8(2/0) |      np    nr |               np    nw    
      (xxx).L     |  8(1/1) 12(3/0) |   np np    nr |               np    nw    
    cc true       |                 |               |
      Dn          |  6(1/0)  0(0/0) |               |               np       n  
      (An)        |  8(1/1)  4(1/0) |            nr |               np    nw    
      (An)+       |  8(1/1)  4(1/0) |            nr |               np    nw    
      -(An)       |  8(1/1)  6(1/0) | n          nr |               np    nw    
      (d16,An)    |  8(1/1)  8(2/0) |      np    nr |               np    nw    
      (d8,An,Xn)  |  8(1/1) 10(2/0) | n    np    nr |               np    nw    
      (xxx).W     |  8(1/1)  8(2/0) |      np    nr |               np    nw    
      (xxx).L     |  8(1/1) 12(3/0) |   np np    nr |               np    nw    
*/

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS))
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_B;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_5_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
    if(m68k_CONDITION_TEST){
#if defined(SSE_CPU_ROUNDING_SCC)
      if(DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {CPU_ABUS_ACCESS_WRITE;}
#endif
#if defined(SSE_VS2008_WARNING_371)
      m68k_DEST_B=(BYTE)0xff; //stupid warning
#else
      m68k_DEST_B=0xff;
#endif
#if !defined(SSE_CPU_ROUNDING_SCC)
      if(DEST_IS_REGISTER){INSTRUCTION_TIME(2);}else {INSTRUCTION_TIME(4);}
#endif
    }else{
#if defined(SSE_CPU_ROUNDING_SCC)
      if(DEST_IS_REGISTER==0){CPU_ABUS_ACCESS_WRITE;}
#endif
      m68k_DEST_B=0;
#if !defined(SSE_CPU_ROUNDING_SCC)
      if(DEST_IS_REGISTER==0){INSTRUCTION_TIME(4);}
#endif
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////  Line 8 - or, div, sbcd   //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//#undef SSE_CPU_LINE_8_TIMINGS

/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
      AND, OR     |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea>,Dn :         |                 |               |
  .B or .W :      |                 |               |
    Dn            |  4(1/0)  0(0/0) |               |               np       
    (An)          |  4(1/0)  4(1/0) |            nr |               np		     
    (An)+         |  4(1/0)  4(1/0) |            nr |               np		     
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np		     
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np		     
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np		     
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np		     
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np		     
    #<data>       |  4(1/0)  4(1/0) |      np       |               np		     
  .L :            |                 |               |
    Dn            |  8(1/0)  0(0/0) |               |               np       nn
    (An)          |  6(1/0)  8(2/0) |         nR nr |               np       n	
    (An)+         |  6(1/0)  8(2/0) |         nR nr |               np       n	
    -(An)         |  6(1/0) 10(2/0) | n       nR nr |               np       n	
    (d16,An)      |  6(1/0) 12(3/0) |      np nR nr |               np       n	
    (d8,An,Xn)    |  6(1/0) 14(3/0) | n    np nR nr |               np       n	
    (xxx).W       |  6(1/0) 12(3/0) |      np nR nr |               np       n	
    (xxx).L       |  6(1/0) 16(4/0) |   np np nR nr |               np       n	
    #<data>       |  8(1/0)  8(2/0) |   np np       |               np       nn
*/


void                              m68k_or_b_to_dN(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_B_NOT_A;
#if !defined(SSE_CPU_PREFETCH_TIMING_OR)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif
#endif
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_B|=m68k_src_b;
#if defined(SSE_CPU_PREFETCH_TIMING_OR)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif
#endif
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_B;
}

void                             m68k_or_w_to_dN(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_W_NOT_A;
#if !defined(SSE_CPU_PREFETCH_TIMING_OR)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif
#endif
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_W|=m68k_src_w;
#if defined(SSE_CPU_PREFETCH_TIMING_OR)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif
#endif
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_W;
}
//#undef SSE_CPU_LINE_8_TIMINGS //sowatt/sync

void                             m68k_or_l_to_dN(){

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif
  if (SOURCE_IS_REGISTER_OR_IMMEDIATE){INSTRUCTION_TIME(4);}
  else {INSTRUCTION_TIME(2);}
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L|=m68k_src_l;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_L;
}
#define SSE_CPU_LINE_8_TIMINGS

#if defined(SSE_CPU_DIV)
void m68k_divu();
#else
void                              m68k_divu(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_BOILER_NODIV))
  DEBUG_ONLY(    log_to(LOGSECTION_DIV,Str("DIV: ")+HEXSl(old_pc,6)+" - "+disa_d2(old_pc));  )
#endif
  FETCH_TIMING;
  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    INSTRUCTION_TIME(m68k_divu_cycles);
    unsigned long q=(((unsigned long)(r[PARAM_N]))/(unsigned long)((unsigned short)m68k_src_w));
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
#endif

//#undef SSE_CPU_LINE_8_TIMINGS
void                              m68k_or_b_from_dN_or_sbcd(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif

  switch(ir&BITS_543){
  case BITS_543_000:case BITS_543_001:{  //sbcd
    if((ir&BITS_543)==BITS_543_000){

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;

      INSTRUCTION_TIME(2);
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;


      INSTRUCTION_TIME_ROUND(14);
      areg[PARAM_M]--;areg[PARAM_N]--;
      if(PARAM_M==7)areg[PARAM_M]--;
      if(PARAM_N==7)areg[PARAM_N]--;
      m68k_src_b=m68k_peek(areg[PARAM_M]);
#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      m68k_SET_DEST_B(areg[PARAM_N]);
    }
#if defined(SSE_CPU_SBCD)
/*  It's more complicated than ABCD if anything.
    eg 22-14 = 8
    2-4 = $FE
    $E-6=8
    one borrow 2-1-1=0
    result: 08

    eg 10-14
    0-4 = $FC
    $C-6=6
    one borrow 1-1-1=$FF
    $F-6=9
    result: 96 with carry ?

*/


#ifdef SSE_DEBUG
    int n=100+
       ( ((m68k_DEST_B&0xf0)>>4)*10+(m68k_DEST_B&0xf) )
      -( ((m68k_src_b&0xf0)>>4)*10+(m68k_src_b&0xf) );
    if(sr&SR_X)n--;
    n%=100;
    n=(BYTE)( (((n/10)%10)<<4)+(n%10) );
#endif

  BYTE src = m68k_src_b;
  BYTE dst = (m68k_DEST_B&0xff);
  BYTE lo_nibble=(dst & 0xF) - (src & 0xF) - ((sr&SR_X)?1:0);
  SR_CLEAR(SR_X+SR_C+SR_N);
  if(lo_nibble&0xF0)
  {
    lo_nibble-=6;
    SR_SET(SR_C);//internal
  }

  WORD hi_nibble=(dst & 0xF0) - (src & 0xF0) - ((sr&SR_C)?0x10:0);
  SR_CLEAR(SR_C);//clear internal bit
  if(hi_nibble&0xF00)
  {
    hi_nibble-=0x60;
    SR_SET(SR_X+SR_C+SR_N);
  }
  m68k_DEST_B=(hi_nibble&0xF0)+(lo_nibble&0xF);
  //ASSERT( m68k_DEST_B==n ); // The Teller, Dark Castle, Jupiter's Masterdrive...
  if(!m68k_DEST_B)
    SR_SET(SR_Z)

#else
    int n=100+
       ( ((m68k_DEST_B&0xf0)>>4)*10+(m68k_DEST_B&0xf) )
      -( ((m68k_src_b&0xf0)>>4)*10+(m68k_src_b&0xf) );
    if(sr&SR_X)n--;
    SR_CLEAR(SR_X+SR_C+SR_N);
    if(n<100)SR_SET(SR_X+SR_C); //if a carry occurs
    n%=100;
    if(n)SR_CLEAR(SR_Z);

/*
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
*/
    m68k_DEST_B=(BYTE)( (((n/10)%10)<<4)+(n%10) );
#endif
    break;
  }default://or.b
/*
Dn,<ea> :         |                 |               | 
  .B or .W :      |                 |               | 
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw	      
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw	      
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw	      
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw	      
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw	      
*/
#if !defined(SSE_CPU_ROUNDING_OR)
    INSTRUCTION_TIME(4);
#endif
    EXTRA_PREFETCH; 
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_B_NOT_A;
    m68k_src_b=LOBYTE(r[PARAM_N]);  
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_OR)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B|=m68k_src_b;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
  }//sw
}
#undef SSE_CPU_LINE_8_TIMINGS
void                             m68k_or_w_from_dN(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif

  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
    m68k_unrecognised();
    break;
  default:
#if !defined(SSE_CPU_ROUNDING_OR)
    INSTRUCTION_TIME(4);
#endif
    EXTRA_PREFETCH; 
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_W_NOT_A;
    m68k_src_w=LOWORD(r[PARAM_N]);
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_OR)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W|=m68k_src_w;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_W;
  }
}
//#undef SSE_CPU_LINE_8_TIMINGS

/*
  .L :            |                 |                              
    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    
*/
void                             m68k_or_l_from_dN(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS))
  FETCH_TIMING;
#endif

  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
    m68k_unrecognised();
    break;
  default:
#if !defined(SSE_CPU_ROUNDING_OR)
    INSTRUCTION_TIME(8);
#endif
    EXTRA_PREFETCH; 
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_L_NOT_A;
    m68k_src_l=r[PARAM_N];
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUNDING_OR)
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_L|=m68k_src_l;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_L;
  }
}

#if defined(SSE_CPU_DIV)
void m68k_divs();
#else
void                              m68k_divs(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_BOILER_NODIV))
  DEBUG_ONLY( log_to(LOGSECTION_DIV,Str("DIV: ")+HEXSl(old_pc,6)+" - "+disa_d2(old_pc)); )
#endif
  FETCH_TIMING;
  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    INSTRUCTION_TIME(m68k_divs_cycles);
    signed long q=(signed long)((signed long)r[PARAM_N])/(signed long)((signed short)m68k_src_w);
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
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////  line 9 - sub            ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
<ea>,Dn :         |                 |              \              |
  .B or .W :      |                 |               |             |
    Dn            |  4(1/0)  0(0/0) |               |             | np          
    An            |  4(1/0)  0(0/0) |               |             | np          
    (An)          |  4(1/0)  4(1/0) |            nr |             | np          
    (An)+         |  4(1/0)  4(1/0) |            nr |             | np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |             | np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |             | np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |             | np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |             | np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |             | np          
    #<data>       |  4(1/0)  4(1/0)        np                     | np          

*/
void                              m68k_sub_b_to_dN(){

#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

  m68k_GET_SOURCE_B_NOT_A;
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_B;

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if !defined(SSE_CPU_ROUNDING_SUB_BW_DN)
  PREFETCH_IRC_NO_ROUND;
#endif

  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(SR_X);

#if defined(SSE_CPU_ROUNDING_SUB_BW_DN)
  PREFETCH_IRC;  // same effect
#else
  INSTRUCTION_TIME_ROUND(0);
#endif

}

void                             m68k_sub_w_to_dN(){

#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

  m68k_GET_SOURCE_W;   //A is allowed
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_W;

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if !defined(SSE_CPU_ROUNDING_SUB_BW_DN)
  PREFETCH_IRC_NO_ROUND;
#endif

  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(SR_X);

#if defined(SSE_CPU_ROUNDING_SUB_BW_DN)
  PREFETCH_IRC;  // same effect
#else
  INSTRUCTION_TIME_ROUND(0);
#endif

}
/*
  .L :            |                 |               |             |
    Dn            |  8(1/0)  0(0/0) |               |             | np       nn 
    An            |  8(1/0)  0(0/0) |               |             | np       nn 
    (An)          |  6(1/0)  8(2/0) |         nR nr |             | np       n  
    (An)+         |  6(1/0)  8(2/0) |         nR nr |             | np       n  
    -(An)         |  6(1/0) 10(2/0) | n       nR nr |             | np       n  
    (d16,An)      |  6(1/0) 12(3/0) |      np nR nr |             | np       n  
    (d8,An,Xn)    |  6(1/0) 14(3/0) | n    np nR nr |             | np       n  
    (xxx).W       |  6(1/0) 12(3/0) |      np nR nr |             | np       n  
    (xxx).L       |  6(1/0) 16(4/0) |   np np nR nr |             | np       n  
    #<data>       |  8(1/0)  8(2/0) |   np np       |               np       nn 
*/
void                             m68k_sub_l_to_dN(){
#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_GET_SOURCE_L;   //A is allowed

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if defined(SSE_CPU_ROUNDING_SUB_L_DN)
  PREFETCH_IRC;
#else
  PREFETCH_IRC_NO_ROUND;
#endif

  if(SOURCE_IS_REGISTER_OR_IMMEDIATE){INSTRUCTION_TIME(4);} //SS looks correct
  else {INSTRUCTION_TIME(2);}

  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(SR_X);

#if !defined(SSE_CPU_ROUNDING_SUB_L_DN)
  INSTRUCTION_TIME_ROUND(0);
#endif

}

/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDA, SUBA    |  INSTR     EA   |  1st OP (ea)  |          INSTR            
------------------+-----------------+---------------+-------------------------- 
<ea>,An :         |                 |               |
  .B or .W :      |                 |               |
    Dn            |  8(1/0)  0(0/0) |               |               np       nn 
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  8(1/0)  4(1/0) |            nr |               np       nn 
    (An)+         |  8(1/0)  4(1/0) |            nr |               np       nn 
    -(An)         |  8(1/0)  6(1/0) | n          nr |               np       nn 
    (d16,An)      |  8(1/0)  8(2/0) |      np    nr |               np       nn 
    (d8,An,Xn)    |  8(1/0) 10(2/0) | n    np    nr |               np       nn 
    (xxx).W       |  8(1/0)  8(2/0) |      np    nr |               np       nn 
    (xxx).L       |  8(1/0) 12(3/0) |   np np    nr |               np       nn 
    #<data>       |  8(1/0)  4(1/0) |      np       |               np       nn 
*/

void                             m68k_suba_w(){

#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

  m68k_GET_SOURCE_W;

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if defined(SSE_CPU_ROUNDING_SUBA_W_DN)
  PREFETCH_IRC;
#else
  PREFETCH_IRC_NO_ROUND;
#endif

  INSTRUCTION_TIME(4);
  m68k_src_l=(signed long)((signed short)m68k_src_w);

  areg[PARAM_N]-=m68k_src_l;

#if !defined(SSE_CPU_ROUNDING_SUBA_W_DN)
  INSTRUCTION_TIME_ROUND(0);
#endif
}

void                              m68k_sub_b_from_dN(){

#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

  switch(ir&BITS_543){
/*
SUBX is done here (bit 8=1, bits543=000 or 001
15 14 13 12     11 10 9    8 7 6  5 4 3       2 1 0
1   0  0  1 REGISTER Dy/Ay 1 SIZE 0 0 R/M     REGISTER Dx/Ax 


------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
Dy,Dx :           |                 |
  .B or .W :      |  4(1/0)         |                               np          

*/

  case BITS_543_000:
  case BITS_543_001:

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if !defined(SSE_CPU_PREFETCH_TIMING_SUBX)
#if defined(SSE_CPU_ROUNDING_SUB_BW_DN2) 
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#endif

    if((ir&BITS_543)==BITS_543_000){
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .B or .W :      | 18(3/1)         |              n nr    nr       np nw       
*/
#if !defined(SSE_CPU_ROUNDING_SUBX)
      INSTRUCTION_TIME_ROUND(14);
#endif
#if defined(SSE_CPU_ROUNDING_SUBX)
      INSTRUCTION_TIME(2);
#endif
      areg[PARAM_M]--;      if(PARAM_M==7)areg[PARAM_M]--;
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_READ;
#endif
      m68k_src_b=m68k_peek(areg[PARAM_M]);
      areg[PARAM_N]--;      if(PARAM_N==7)areg[PARAM_N]--;
#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      m68k_SET_DEST_B(areg[PARAM_N]);
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_READ;
#endif
    }
    m68k_old_dest=m68k_DEST_B;
#if defined(SSE_CPU_PREFETCH_TIMING_SUBX)
    PREFETCH_IRC;
#endif
#if defined(SSE_CPU_ROUNDING_SUBX)
    if((ir&BITS_543)==BITS_543_001)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B-=m68k_src_b;
    if(sr&SR_X)m68k_DEST_B--;
    SR_SUBX_B;
    break;
  default:
/*
Dn,<ea> :         |                 |              /              |
  .B or .W :      |                 |             |               |
    (An)          |  8(1/1)  4(1/0) |             |            nr | np nw       
    (An)+         |  8(1/1)  4(1/0) |             |            nr | np nw       
    -(An)         |  8(1/1)  6(1/0) |             | n          nr | np nw       
    (d16,An)      |  8(1/1)  8(2/0) |             |      np    nr | np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) |             | n    np    nr | np nw       
    (xxx).W       |  8(1/1)  8(2/0) |             |      np    nr | np nw       
    (xxx).L       |  8(1/1) 12(3/0) |             |   np np    nr | np nw       

*/
#if !defined(SSE_CPU_ROUNDING_SUB)
    INSTRUCTION_TIME(4);
#endif
    EXTRA_PREFETCH; 
    m68k_src_b=LOBYTE(r[PARAM_N]);
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_B_NOT_A;

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if defined(SSE_CPU_ROUNDING_SUB_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif

    m68k_old_dest=m68k_DEST_B;
#if defined(SSE_CPU_ROUNDING_SUB)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B-=m68k_src_b;
    SR_SUB_B(SR_X);
  }

#if !defined(SSE_CPU_ROUNDING_SUB_BW_DN2)
  INSTRUCTION_TIME_ROUND(0);
#endif
}

void                              m68k_sub_w_from_dN(){

#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

  switch(ir&BITS_543){
/*
SUBX is done here (bit 8=1, bits543=000 or 001
15 14 13 12     11 10 9    8 7 6  5 4 3       2 1 0
1   0  0  1 REGISTER Dy/Ay 1 SIZE 0 0 R/M     REGISTER Dx/Ax 


------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
Dy,Dx :           |                 |
  .B or .W :      |  4(1/0)         |                               np          

*/
  case BITS_543_000:
  case BITS_543_001:
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif

#if !defined(SSE_CPU_PREFETCH_TIMING_SUBX)
#if defined(SSE_CPU_ROUNDING_SUB_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#endif

    if((ir&BITS_543)==BITS_543_000){
      m68k_src_w=LOWORD(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .B or .W :      | 18(3/1)         |              n nr    nr       np nw       
*/
#if !defined(SSE_CPU_ROUNDING_SUBX)
      INSTRUCTION_TIME_ROUND(14);
#endif
#if defined(SSE_CPU_ROUNDING_SUBX)
      INSTRUCTION_TIME(2);
#endif
      areg[PARAM_M]-=2;
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_READ;
#endif
      m68k_src_w=m68k_dpeek(areg[PARAM_M]);
#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      areg[PARAM_N]-=2;m68k_SET_DEST_W(areg[PARAM_N]);
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_READ;
#endif
    }
    m68k_old_dest=m68k_DEST_W;
#if defined(SSE_CPU_PREFETCH_TIMING_SUBX)
    PREFETCH_IRC;
#endif
#if defined(SSE_CPU_ROUNDING_SUBX)
    if((ir&BITS_543)==BITS_543_001)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W-=m68k_src_w;
    if(sr&SR_X)m68k_DEST_W--;
    SR_SUBX_W;
    break;
  default: //to memory
/*
Dn,<ea> :         |                 |              /              |
  .B or .W :      |                 |             |               |
    (An)          |  8(1/1)  4(1/0) |             |            nr | np nw       
    (An)+         |  8(1/1)  4(1/0) |             |            nr | np nw       
    -(An)         |  8(1/1)  6(1/0) |             | n          nr | np nw       
    (d16,An)      |  8(1/1)  8(2/0) |             |      np    nr | np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) |             | n    np    nr | np nw       
    (xxx).W       |  8(1/1)  8(2/0) |             |      np    nr | np nw       
    (xxx).L       |  8(1/1) 12(3/0) |             |   np np    nr | np nw       

*/


#if !defined(SSE_CPU_ROUNDING_SUB)
    INSTRUCTION_TIME(4);
#endif

    EXTRA_PREFETCH; 
    m68k_src_w=LOWORD(r[PARAM_N]);
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_W_NOT_A;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif

#if defined(SSE_CPU_ROUNDING_SUB_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif

    m68k_old_dest=m68k_DEST_W;
#if defined(SSE_CPU_ROUNDING_SUB)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W-=m68k_src_w;
    SR_SUB_W(SR_X);
  }
#if !defined(SSE_CPU_ROUNDING_SUB_BW_DN2)
  INSTRUCTION_TIME_ROUND(0);
#endif

}


void                              m68k_sub_l_from_dN(){
#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  switch(ir&BITS_543){

/*
SUBX is done here (bit 8=1, bits543=000 or 001
15 14 13 12     11 10 9    8 7 6  5 4 3       2 1 0
1   0  0  1 REGISTER Dy/Ay 1 SIZE 0 0 R/M     REGISTER Dx/Ax 


------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
Dy,Dx :           |                 |
  .L :            |  8(1/0)         |                               np       nn 
*/


  case BITS_543_000:
  case BITS_543_001:
    if((ir&BITS_543)==BITS_543_000){
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
      FETCH_TIMING_NO_ROUND;
#else
      INSTRUCTION_TIME(4);
#endif
#endif

#if defined(SSE_CPU_ROUNDING_SUB_L_DN2)
      PREFETCH_IRC;
#else
      PREFETCH_IRC_NO_ROUND;
#endif

      INSTRUCTION_TIME(4);
      m68k_src_l=r[PARAM_M];
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .L :            | 30(5/2)         |              n nr nR nr nR nw np    nW    
*/
#if !defined(SSE_CPU_ROUNDING_SUBX)
      INSTRUCTION_TIME_ROUND(26);
#endif
#if defined(SSE_CPU_ROUNDING_SUBX)
      INSTRUCTION_TIME(2);
#endif
      areg[PARAM_M]-=4;
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_READ;
      CPU_ABUS_ACCESS_READ;
#endif
      m68k_src_l=m68k_lpeek(areg[PARAM_M]);

#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      areg[PARAM_N]-=4;m68k_SET_DEST_L(areg[PARAM_N]);
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_READ;
      CPU_ABUS_ACCESS_READ;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
      FETCH_TIMING_NO_ROUND;
#else
      INSTRUCTION_TIME(4);
#endif
#endif
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_WRITE;
#endif
#if defined(SSE_CPU_ROUNDING_SUB_L_DN2)
      PREFETCH_IRC;
#else
      PREFETCH_IRC_NO_ROUND;
#endif
#if defined(SSE_CPU_ROUNDING_SUBX)
      CPU_ABUS_ACCESS_WRITE;
#endif
    }
    m68k_old_dest=m68k_DEST_L;
    m68k_DEST_L-=m68k_src_l;
    if(sr&SR_X)m68k_DEST_L--;
    SR_SUBX_L;
    break;
  default:
/*
  .L :            |                 |             |               |
    (An)          | 12(1/2)  8(2/0) |             |            nr | np nw nW    
    (An)+         | 12(1/2)  8(2/0) |             |            nr | np nw nW    
    -(An)         | 12(1/2) 10(2/0) |             | n       nR nr | np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |             |      np nR nr | np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) |             | n    np nR nr | np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |             |      np nR nr | np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |             |   np np nR nr | np nw nW    

*/

#if !defined(SSE_CPU_ROUNDING_SUB)
    INSTRUCTION_TIME(8);
#endif
    EXTRA_PREFETCH; 
    m68k_src_l=r[PARAM_N];
    m68k_GET_DEST_L_NOT_A;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if defined(SSE_CPU_ROUNDING_SUB_L_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif

    m68k_old_dest=m68k_DEST_L;
#if defined(SSE_CPU_ROUNDING_SUB)
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_L-=m68k_src_l;
    SR_SUB_L(SR_X);
  }
#if !defined(SSE_CPU_ROUNDING_SUB_L_DN2)
  INSTRUCTION_TIME_ROUND(0);
#endif
}


/*
  .L :            |                 |               |
    Dn            |  8(1/0)  0(0/0) |               |               np       nn 
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  6(1/0)  8(2/0) |         nR nr |               np       n  
    (An)+         |  6(1/0)  8(2/0) |         nR nr |               np       n  
    -(An)         |  6(1/0) 10(2/0) | n       nR nr |               np       n  
    (d16,An)      |  6(1/0) 12(3/0) |      np nR nr |               np       n  
    (d8,An,Xn)    |  6(1/0) 14(3/0) | n    np nR nr |               np       n  
    (xxx).W       |  6(1/0) 12(3/0) |      np nR nr |               np       n  
    (xxx).L       |  6(1/0) 16(4/0) |   np np nR nr |               np       n  
    #<data>       |  8(1/0)  8(2/0) |   np np       |               np       nn 
*/
void                             m68k_suba_l(){
#if !defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_GET_SOURCE_L;

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_9_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif

#if defined(SSE_CPU_ROUNDING_SUBA_L_DN)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif

  if (SOURCE_IS_REGISTER_OR_IMMEDIATE){INSTRUCTION_TIME(4);}
  else {INSTRUCTION_TIME(2);}
  areg[PARAM_N]-=m68k_src_l;
#if !defined(SSE_CPU_ROUNDING_SUBA_L_DN)
  INSTRUCTION_TIME_ROUND(0);
#endif
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////  line b - cmp, eor       ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
       CMP        |  INSTR     EA   |  1st OP (ea)  |          INSTR            
------------------+-----------------+---------------+-------------------------- 
<ea>,Dn :         |                 |               | 
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    An            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  4(1/0)  4(1/0) |            nr |               np          
    (An)+         |  4(1/0)  4(1/0) |            nr |               np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np          
    #<data>       |  4(1/0)  4(1/0) |      np       |               np          
*/

void                              m68k_cmp_b(){
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif

#if defined(SSE_CPU_ASSERT_ILLEGAL3)
  if((ir&BITS_543)==BITS_543_001) // .B An...
    m68k_unrecognised();
//    ILLEGAL
#endif

  m68k_GET_SOURCE_B;
  m68k_old_dest=LOBYTE(r[PARAM_N]);
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
#if !defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(0);
#if defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
}

void                             m68k_cmp_w(){
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_W;
  m68k_old_dest=LOWORD(r[PARAM_N]);
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
#if !defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(0);
#if defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
#endif
}

/*
  .L :            |                 |               | 
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    An            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          |  6(1/0)  8(1/0) |         nR nr |               np       n  
    (An)+         |  6(1/0)  8(1/0) |         nR nr |               np       n  
    -(An)         |  6(1/0) 10(1/0) | n       nR nr |               np       n  
    (d16,An)      |  6(1/0) 12(2/0) |      np nR nr |               np       n  
    (d8,An,Xn)    |  6(1/0) 14(2/0) | n    np nR nr |               np       n  
    (xxx).W       |  6(1/0) 12(2/0) |      np nR nr |               np       n  
    (xxx).L       |  6(1/0) 16(3/0) |   np np nR nr |               np       n  
    #<data>       |  6(1/0)  8(2/0) |   np np       |               np       n  
*/

void                             m68k_cmp_l(){
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_L;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
  INSTRUCTION_TIME(2);
  m68k_old_dest=r[PARAM_N];
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
}
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage             
       CMPA       |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
<ea>,An :         |                 |               | 
  .B or .W :      |                 |               | 
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    An            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          |  6(1/0)  4(1/0) |            nr |               np       n  
    (An)+         |  6(1/0)  4(1/0) |            nr |               np       n  
    -(An)         |  6(1/0)  6(1/0) | n          nr |               np       n  
    (d16,An)      |  6(1/0)  8(2/0) |      np    nr |               np       n  
    (d8,An,Xn)    |  6(1/0) 10(2/0) | n    np    nr |               np       n  
    (xxx).W       |  6(1/0)  8(2/0) |      np    nr |               np       n  
    (xxx).L       |  6(1/0) 12(3/0) |   np np    nr |               np       n  
    #<data>       |  6(1/0)  4(1/0) |      np       |               np       n  
*/

void                             m68k_cmpa_w(){
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_W;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC;
  INSTRUCTION_TIME(2);
  m68k_src_l=(signed long)((signed short)m68k_src_w);
  m68k_old_dest=areg[PARAM_N];
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
}

void                              m68k_eor_b(){ //SS or cmpm
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  if((ir&BITS_543)==BITS_543_001){  //cmpm
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
       CMPM       |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
(Ay)+,(Ax)+       |                 |
  .B or .W :      | 12(3/0)         |                      nr    nr np          
*/
#if !defined(SSE_CPU_ROUDING_CMP)
    INSTRUCTION_TIME_ROUND(8);
#endif
#if defined(SSE_CPU_ROUDING_CMP)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_src_b=m68k_peek(areg[PARAM_M]);areg[PARAM_M]++; if(PARAM_M==7)areg[PARAM_M]++;
#if defined(SSE_CPU_ROUDING_CMP)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_old_dest=m68k_peek(areg[PARAM_N]);areg[PARAM_N]++; if(PARAM_N==7)areg[PARAM_N]++;
    compare_buffer=m68k_old_dest;
    m68k_dest=&compare_buffer;
#if !defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
    m68k_DEST_B-=m68k_src_b;
    SR_SUB_B(0);
#if defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
  }else{
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage              
        EOR       |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
Dn,<ea> :         |                 |               |
  .B or .W :      |                 |               |
    Dn            |  4(1/0)  0(0/0) |               |               np		     
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw	   
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw	      
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw	      
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw	      
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw	
*/
#if !defined(SSE_CPU_ROUDING_EOR)
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUDING_EOR)
    m68k_GET_DEST_B_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUDING_EOR)
    m68k_GET_DEST_B_NOT_A;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#if defined(SSE_CPU_ROUDING_EOR)
    if(!DEST_IS_DATA_REGISTER)
      CPU_ABUS_ACCESS_WRITE; 
#endif
    m68k_DEST_B^=LOBYTE(r[PARAM_N]);
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
  }
}

void                             m68k_eor_w(){ //SS or CMPM
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif

  if((ir&BITS_543)==BITS_543_001){  //cmpm
#if !defined(SSE_CPU_ROUDING_CMP)
    INSTRUCTION_TIME_ROUND(8);
#endif
#if defined(SSE_CPU_ROUDING_CMP)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_src_w=m68k_dpeek(areg[PARAM_M]);areg[PARAM_M]+=2;
#if defined(SSE_CPU_ROUDING_CMP)
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_old_dest=m68k_dpeek(areg[PARAM_N]);areg[PARAM_N]+=2;
    compare_buffer=m68k_old_dest;
    m68k_dest=&compare_buffer;
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    m68k_DEST_W-=m68k_src_w;
    SR_SUB_W(0);
  }else{
/*
Dn,<ea> :         |                 |               |
  .B or .W :      |                 |               |
    Dn            |  4(1/0)  0(0/0) |               |               np		     
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw	   
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw	      
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw	      
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw	      
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw	  
*/
#if !defined(SSE_CPU_ROUDING_EOR)
    INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUDING_EOR)
    m68k_GET_DEST_W_NOT_A_FASTER_FOR_D; //SS: with 'extra prefetch'
#endif
#if defined(SSE_CPU_ROUDING_EOR)
    m68k_GET_DEST_W_NOT_A;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC; // Operation Clean Streets (Auto 168)
#if defined(SSE_CPU_ROUDING_EOR)
    if(!(DEST_IS_DATA_REGISTER))
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W^=LOWORD(r[PARAM_N]);
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_W;
  }
}

void                             m68k_eor_l(){ //SS or CMPM
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif

  if((ir&BITS_543)==BITS_543_001){  //cmpm
//   .L :            | 20(5/0)         |                   nR nr nR nr np   
#if !defined(SSE_CPU_ROUDING_CMP)
    INSTRUCTION_TIME_ROUND(16);
#endif
#if defined(SSE_CPU_ROUDING_CMP)
    CPU_ABUS_ACCESS_READ;
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_src_l=m68k_lpeek(areg[PARAM_M]);areg[PARAM_M]+=4;
#if defined(SSE_CPU_ROUDING_CMP)
    CPU_ABUS_ACCESS_READ;
    CPU_ABUS_ACCESS_READ;
#endif
    m68k_old_dest=m68k_lpeek(areg[PARAM_N]);areg[PARAM_N]+=4;
    compare_buffer=m68k_old_dest;
    m68k_dest=&compare_buffer;
#if !defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
    m68k_DEST_L-=m68k_src_l;
    SR_SUB_L(0);
#if defined(SSE_CPU_PREFETCH_TIMING_CMP)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#endif
  }else{
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage              
        EOR       |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
Dn,<ea> :         |                 |               |
  .L :            |                 |               |                           
    Dn            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    
*/
#if !defined(SSE_CPU_ROUDING_EOR)
    INSTRUCTION_TIME(8);
#endif
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
#if !defined(SSE_CPU_ROUDING_EOR)
    m68k_GET_DEST_L_NOT_A_FASTER_FOR_D;
#endif
#if defined(SSE_CPU_ROUDING_EOR)
    m68k_GET_DEST_L_NOT_A;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
#if defined(SSE_CPU_ROUDING_EOR)
    if(DEST_IS_DATA_REGISTER)
      INSTRUCTION_TIME(4);
    else
    {
      CPU_ABUS_ACCESS_WRITE;
      CPU_ABUS_ACCESS_WRITE;
    }
#endif
    m68k_DEST_L^=r[PARAM_N];
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_L;
  }
}
//#undef SSE_CPU_LINE_B_TIMINGS
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage             
       CMPA       |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
<ea>,An :         |                 |               | 
  .L :            |                 |               | 
    Dn            |  6(1/0)  0(0/0) |               |               np       n  
    An            |  6(1/0)  0(0/0) |               |               np       n  
    (An)          |  6(1/0)  8(1/0) |         nR nr |               np       n  
    (An)+         |  6(1/0)  8(1/0) |         nR nr |               np       n  
    -(An)         |  6(1/0) 10(1/0) | n       nR nr |               np       n  
    (d16,An)      |  6(1/0) 12(2/0) |      np nR nr |               np       n  
    (d8,An,Xn)    |  6(1/0) 14(2/0) | n    np nR nr |               np       n  
    (xxx).W       |  6(1/0) 12(2/0) |      np nR nr |               np       n  
    (xxx).L       |  6(1/0) 16(3/0) |   np np nR nr |               np       n  
    #<data>       |  6(1/0)  8(2/0) |   np np       |               np       n  
*/

void                             m68k_cmpa_l(){
#if !defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_L;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_B_TIMINGS)
  FETCH_TIMING; //DSOS/scrollers: before the INSTRUCTION_TIME(2)
#endif
  INSTRUCTION_TIME(2);
  m68k_old_dest=areg[PARAM_N];
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);//SS SR_X would break Death of the Clock Cycles
}
//#define SSE_CPU_LINE_B_TIMINGS

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////  Line C - and, abcd, exg, mul   ///////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
<ea>,Dn :         |                 |               |
  .B or .W :      |                 |               |
    Dn            |  4(1/0)  0(0/0) |               |               np       
    (An)          |  4(1/0)  4(1/0) |            nr |               np		     
    (An)+         |  4(1/0)  4(1/0) |            nr |               np		     
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np		     
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np		     
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np		     
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np		     
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np		     
    #<data>       |  4(1/0)  4(1/0) |      np       |               np		     

*/

void                              m68k_and_b_to_dN(){
#if !defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_B_NOT_A;
  m68k_dest=&(r[PARAM_N]);
#if !defined(SSE_CPU_PREFETCH_TIMING_AND)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
#endif
  m68k_DEST_B&=m68k_src_b;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_B;
#if defined(SSE_CPU_PREFETCH_TIMING_AND)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
#endif
}

void                             m68k_and_w_to_dN(){
#if !defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_W_NOT_A;
  m68k_dest=&(r[PARAM_N]);
#if !defined(SSE_CPU_PREFETCH_TIMING_AND)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
#endif  
  m68k_DEST_W&=m68k_src_w;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_W;
#if defined(SSE_CPU_PREFETCH_TIMING_AND)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
#endif
}
/*
  .L :            |                 |               |
    Dn            |  8(1/0)  0(0/0) |               |               np       nn
    (An)          |  6(1/0)  8(2/0) |         nR nr |               np       n	
    (An)+         |  6(1/0)  8(2/0) |         nR nr |               np       n	
    -(An)         |  6(1/0) 10(2/0) | n       nR nr |               np       n	
    (d16,An)      |  6(1/0) 12(3/0) |      np nR nr |               np       n	
    (d8,An,Xn)    |  6(1/0) 14(3/0) | n    np nR nr |               np       n	
    (xxx).W       |  6(1/0) 12(3/0) |      np nR nr |               np       n	
    (xxx).L       |  6(1/0) 16(4/0) |   np np nR nr |               np       n	
    #<data>       |  8(1/0)  8(2/0) |   np np       |               np       nn

*/
void                             m68k_and_l_to_dN(){
#if !defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_L_NOT_A;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
  if(SOURCE_IS_REGISTER_OR_IMMEDIATE){INSTRUCTION_TIME(4);}
  else {INSTRUCTION_TIME(2);}
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L&=m68k_src_l;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_L;
}

//#undef SSE_CPU_LINE_C_TIMINGS

/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
    MULU,MULS     |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea>,Dn :        /                  |               |
  .W :          /                   |               |
    Dn         | 38+2m(1/0)  0(0/0) |               |               np       n* 
    (An)       | 38+2m(1/0)  4(1/0) |            nr |               np       n* 
    (An)+      | 38+2m(1/0)  4(1/0) |            nr |               np       n* 
    -(An)      | 38+2m(1/0)  6(1/0) | n          nr |               np       n* 
    (d16,An)   | 38+2m(1/0)  8(2/0) |      np    nr |               np       n* 
    (d8,An,Xn) | 38+2m(1/0) 10(2/0) | n    np    nr |               np       n* 
    (xxx).W    | 38+2m(1/0)  8(2/0) |      np    nr |               np       n* 
    (xxx).L    | 38+2m(1/0) 12(2/0) |   np np    nr |               np       n* 
    #<data>    | 38+2m(1/0)  8(2/0) |   np np       |               np       n* 
NOTES :
  .for MULU 'm' = the number of ones in the source
    - Best case 38 cycles with $0
    - Worst case : 70 cycles with $FFFF 
  .for MULS 'm' = concatenate the 16-bit pointed by <ea> with a zero as the LSB
   'm' is the resultant number of 10 or 01 patterns in the 17-bit source.
    - Best case : 38 cycles with $0 or $FFFF
    - Worst case : 70 cycles with $5555
  .in both cases : 'n*' should be replaced by 17+m consecutive 'n'
FLOWCHART :                                                                     
                            np                                                  
         LSB=1              |           LSB=0                                   
               +------------+------------+                                      
               |                         |                                      
            +->n------------------------>n<----+                                
            |                            |     | LSB=0 or                       
            |           No more bits     |     | 2LSB=00,11                     
   LSB=1 or +---------------+------------+-----+                                
   2LSB=01,10               |                                                   
                            n                                                   
  .LSB = less significant bit : bit at the far right of the source.             
  .2LSB = 2 less significant bits : 2 last bits at the far right of the source. 

*/

void                              m68k_mulu(){

#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_GET_SOURCE_W_NOT_A;

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING; 
#endif
  PREFETCH_IRC; // prefetch before computing (but after <EA>)
  INSTRUCTION_TIME(34);  

  ///// Hey, this is right apparently
  for (WORD Val=m68k_src_w;Val;Val>>=1){
    if (Val & 1) INSTRUCTION_TIME(2);
  }

  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L=(unsigned long)LOWORD(r[PARAM_N])*(unsigned long)((unsigned short)m68k_src_w);
  SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
  SR_CHECK_Z_AND_N_L;
}
//#define SSE_CPU_LINE_C_TIMINGS

//#undef SSE_CPU_LINE_C_TIMINGS
void                              m68k_and_b_from_dN_or_abcd(){
#if !defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif

  switch (ir & BITS_543){
  case BITS_543_000:case BITS_543_001:{ //SS ABCD
    if((ir&BITS_543)==BITS_543_000){ //SS R+R
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ABCD, SBCD    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
Dy,Dx :           |                 |
  .B :            |  6(1/0)         |                               np       n  
*/

      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
      FETCH_TIMING;
#endif
      INSTRUCTION_TIME(2);
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{ //SS M+M
/*
-(Ay),-(Ax) :     |                 |
  .B :            | 18(3/1)         |                 n    nr    nr np nw       
*/
#if !defined(SSE_CPU_ROUNDING_ABCD)
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
      FETCH_TIMING;
#endif
      INSTRUCTION_TIME_ROUND(14);
#endif
#if defined(SSE_CPU_ROUNDING_ABCD)
      INSTRUCTION_TIME(2);
#endif
      areg[PARAM_M]--;
      if(PARAM_M==7)areg[PARAM_M]--;
      areg[PARAM_N]--;
      if(PARAM_N==7)areg[PARAM_N]--;
#if defined(SSE_CPU_ROUNDING_ABCD)
      CPU_ABUS_ACCESS_READ;
#endif
      m68k_src_b=m68k_peek(areg[PARAM_M]);
#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      m68k_SET_DEST_B(areg[PARAM_N]);
#if defined(SSE_CPU_ROUNDING_ABCD)
      CPU_ABUS_ACCESS_READ;
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
      FETCH_TIMING;
#endif
#endif
    }

#if defined(SSE_CPU_ABCD)
/*
  http://en.wikipedia.org/wiki/Binary-coded_decimal#Addition_with_BCD
  each decimal digit is coded on 4bit

    0-9  0-9   +   0-9  0-9   
    ---- ----      ---- ----  

  The Steem way works for normal operands but when an operand
  is illegal (nibble>9), the result isn't the same as on a MC68000,
  that uses the "+6" trick.
  http://tams-www.informatik.uni-hamburg.de/applets/hades/webdemos/20-arithmetic/10-adders/bcd-adder.html
  In the MC68000, the 'DAA' decimal adjust accumulator is integrated into ABCD.
  It is correct in Hatari, so we use the same way now.
  Fixes Espana 92 -ICS
*/
  BYTE src = m68k_src_b;
  BYTE dst = (m68k_DEST_B&0xff);
  BYTE lo_nibble=(src & 0xF) + (dst & 0xF) + ( (sr&SR_X) ? 1 : 0);
  SR_CLEAR(SR_X+SR_C+SR_N);
  if(lo_nibble>9)
    lo_nibble+=6;
  WORD hi_nibble=(src & 0xF0) + (dst & 0xF0) + (lo_nibble&0xF0);
  if(hi_nibble>0x90)
  {
    hi_nibble+=0x60;
    SR_SET(SR_X+SR_C);
  }
#if defined(SSE_CPU_ROUNDING_ABCD)
  CPU_ABUS_ACCESS_WRITE;
#endif
  m68k_DEST_B=(hi_nibble&0xF0)+(lo_nibble&0xF);
  if(!m68k_DEST_B)
    SR_SET(SR_Z)
  else if(m68k_DEST_B<0)
    SR_SET(SR_N) // not sure of that, it's so in Hatari
  break;

#else  // Steem 3.2
    int n=
       ( ((m68k_DEST_B&0xf0)>>4)*10+(m68k_DEST_B&0xf) )
      +( ((m68k_src_b&0xf0)>>4)*10+(m68k_src_b&0xf) );
    if(sr&SR_X)n++;
    SR_CLEAR(SR_X+SR_C+SR_N);
    if(n>=100)SR_SET(SR_X+SR_C); //if a carry occurs
    n%=100;
    if(n)SR_CLEAR(SR_Z);
    m68k_DEST_B=(BYTE)( (((n/10)%10)<<4)+(n%10) );
    break;
#endif
  }default: //SS and.b from dn
/*
Dn,<ea> :         |                 |               | 
  .B or .W :      |                 |               | 
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw	      
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw	      
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw	      
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw	      
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw	      

*/
#if !defined(SSE_CPU_ROUDING_AND)
    INSTRUCTION_TIME(4);
#endif
    EXTRA_PREFETCH; 
    m68k_GET_DEST_B_NOT_A;
    m68k_src_b=LOBYTE(r[PARAM_N]);
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUDING_AND)
    ASSERT(!DEST_IS_DATA_REGISTER);
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B&=m68k_src_b;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
  }
}
//#define SSE_CPU_LINE_C_TIMINGS
//#undef SSE_CPU_LINE_C_TIMINGS

void                             m68k_and_w_from_dN_or_exg_like(){
#if !defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif

  switch(ir&BITS_543){ 
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
        EXG       |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
  .L :            |                 |
    Dx,Dy         |  6(1/0)         |                               np       n  
    Ax,Ay         |  6(1/0)         |                               np       n  
*/
  case BITS_543_000: //SS EXG D D

    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
    FETCH_TIMING;
#endif

    INSTRUCTION_TIME(2);
    compare_buffer=r[PARAM_N];
    r[PARAM_N]=r[PARAM_M];
    r[PARAM_M]=compare_buffer;
    break;
  case BITS_543_001: //SS EXG A A
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
    FETCH_TIMING;
#endif

    INSTRUCTION_TIME(2);
    compare_buffer=areg[PARAM_N];
    areg[PARAM_N]=areg[PARAM_M];
    areg[PARAM_M]=compare_buffer;
    break;
  default://SS and.b from dn
/*
Dn,<ea> :         |                 |               | 
  .B or .W :      |                 |               | 
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw	      
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw	      
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw	      
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw	      
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw	      
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw	      

*/
#if !defined(SSE_CPU_ROUDING_AND)
    INSTRUCTION_TIME(4);
#endif

    EXTRA_PREFETCH; 
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_W_NOT_A;
    m68k_src_w=LOWORD(r[PARAM_N]);
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUDING_AND)
    ASSERT(!DEST_IS_DATA_REGISTER);
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W&=m68k_src_w;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_W;
  }
}
//#define SSE_CPU_LINE_C_TIMINGS
//#undef SSE_CPU_LINE_C_TIMINGS//**
void                             m68k_and_l_from_dN_or_exg_unlike(){
#if !defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif

  switch(ir&BITS_543){
  case BITS_543_000:
    m68k_unrecognised();
    // m68k_command="exg";
    // m68k_src_l=// D2_aN;
    // m68k_dest=// D2_dM;
    break;
  case BITS_543_001: //SS EXG

/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
        EXG       |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
  .L :            |                 |
    Dx,Ay         |  6(1/0)         |                               np       n  
*/


    //SS TB2/DNT
    // EXG 6(1/0)
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
    FETCH_TIMING;
#endif
    INSTRUCTION_TIME(2);
    compare_buffer=areg[PARAM_M];
    areg[PARAM_M]=r[PARAM_N];
    r[PARAM_N]=compare_buffer;
    break;
  default://SS and.l from dn
/*
Dn,<ea> :         |                 |               | 
  .L :            |                 |                              
    (An)          | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    (An)+         | 12(1/2)  8(2/0) |         nR nr |               np nw nW    
    -(An)         | 12(1/2) 10(2/0) | n       nR nr |               np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) | n    np nR nr |               np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |      np nR nr |               np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |   np np nR nr |               np nw nW    

*/
#if !defined(SSE_CPU_ROUDING_AND)
    INSTRUCTION_TIME(8);
#endif
    EXTRA_PREFETCH; 
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_L_NOT_A;
    m68k_src_l=r[PARAM_N];
    PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
    FETCH_TIMING;
#endif
#if defined(SSE_CPU_ROUDING_AND)
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_L&=m68k_src_l;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_L;
  }
}
//#define SSE_CPU_LINE_C_TIMINGS
//#undef SSE_CPU_LINE_C_TIMINGS

// see mulu for chart

void                              m68k_muls(){

#if !defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif

  m68k_GET_SOURCE_W_NOT_A;

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_C_TIMINGS)
  FETCH_TIMING;
#endif
  PREFETCH_IRC; // prefetch before computing (but after <EA>)
/*
  .for MULS 'm' = concatenate the 16-bit pointed by <ea> with a zero as the LSB
   'm' is the resultant number of 10 or 01 patterns in the 17-bit source.
    - Best case : 38 cycles with $0 or $FFFF
    - Worst case : 70 cycles with $5555

*/

  INSTRUCTION_TIME(34);  

  ///// Hey, this is right apparently
  int LastLow=0;
  int Val=WORD(m68k_src_w);
  for (int n=0;n<16;n++){
    if ((Val & 1)!=LastLow) INSTRUCTION_TIME(2);
    LastLow=(Val & 1);
    Val>>=1;
  }
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L=((signed long)((signed short)LOWORD(r[PARAM_N])))*((signed long)((signed short)m68k_src_w));
  SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
  SR_CHECK_Z_AND_N_L;
}

#define SSE_CPU_LINE_C_TIMINGS


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////  line D - add.b/add.w/add.l ////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
     ADD, SUB     |  INSTR     EA   | 1st Operand |  2nd OP (ea)  |   INSTR     
------------------+-----------------+-------------+---------------+------------ 
<ea>,Dn :         |                 |              \              |
  .B or .W :      |                 |               |             |
    Dn            |  4(1/0)  0(0/0) |               |             | np          
    An            |  4(1/0)  0(0/0) |               |             | np          
    (An)          |  4(1/0)  4(1/0) |            nr |             | np          
    (An)+         |  4(1/0)  4(1/0) |            nr |             | np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |             | np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |             | np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |             | np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |             | np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |             | np          
    #<data>       |  4(1/0)  4(1/0)        np                     | np          
*/

void                              m68k_add_b_to_dN(){

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_GET_SOURCE_B_NOT_A;
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_B;
#if !defined(SSE_CPU_PREFETCH_TIMING_ADD) //prefetches at the end
#if defined(SSE_CPU_ROUNDING_ADD_BW_DN)
  PREFETCH_IRC;
#else
  PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
#endif
  m68k_DEST_B+=m68k_src_b;
  SR_ADD_B;
#if defined(SSE_CPU_PREFETCH_TIMING_ADD)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
  FETCH_TIMING;
#endif
#endif
#if !defined(SSE_CPU_ROUNDING_ADD) && !defined(SSE_CPU_ROUNDING_ADD_BW_DN)
  INSTRUCTION_TIME_ROUND(0);
#endif
}

void                             m68k_add_w_to_dN(){ // add.w ea,dn or adda.w ea,an

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_GET_SOURCE_W;   //A is allowed
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_W;
#if !defined(SSE_CPU_PREFETCH_TIMING_ADD)
#if defined(SSE_CPU_ROUNDING_ADD_BW_DN)
  PREFETCH_IRC;
#else
  PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
#endif
  m68k_DEST_W+=m68k_src_w;
  SR_ADD_W;
#if defined(SSE_CPU_PREFETCH_TIMING_ADD)
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
  FETCH_TIMING;
#endif
#endif
#if !defined(SSE_CPU_ROUNDING_ADD) && !defined(SSE_CPU_ROUNDING_ADD_BW_DN)
  INSTRUCTION_TIME_ROUND(0);
#endif
}

/*
  .L :            |                 |               |             |
    Dn            |  8(1/0)  0(0/0) |               |             | np       nn 
    An            |  8(1/0)  0(0/0) |               |             | np       nn 
    (An)          |  6(1/0)  8(2/0) |         nR nr |             | np       n  
    (An)+         |  6(1/0)  8(2/0) |         nR nr |             | np       n  
    -(An)         |  6(1/0) 10(2/0) | n       nR nr |             | np       n  
    (d16,An)      |  6(1/0) 12(3/0) |      np nR nr |             | np       n  
    (d8,An,Xn)    |  6(1/0) 14(3/0) | n    np nR nr |             | np       n  
    (xxx).W       |  6(1/0) 12(3/0) |      np nR nr |             | np       n  
    (xxx).L       |  6(1/0) 16(4/0) |   np np nR nr |             | np       n  
    #<data>       |  8(1/0)  8(2/0) |   np np       |               np       nn 
*/

void                             m68k_add_l_to_dN(){ // add.l ea,dn or adda.l ea,an

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_GET_SOURCE_L;   //A is allowed
#if defined(SSE_CPU_ROUNDING_ADD_L_DN)
  PREFETCH_IRC;
#endif
  if(SOURCE_IS_REGISTER_OR_IMMEDIATE){INSTRUCTION_TIME(4);}
  else {INSTRUCTION_TIME(2);}
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_L;
#if !defined(SSE_CPU_ROUNDING_ADD_L_DN)
  PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_DEST_L+=m68k_src_l;
  SR_ADD_L;
#if !defined(SSE_CPU_ROUNDING_ADD)
/*  Conform to Yacht now and both programs below work: compensating
    timing issues certainly.
*/
#if defined(SSE_CPU_ROUNDING_ADD_L_DN)
/*  Cernit Trandafir #2 right border
    but DSoTS doesn't like it, hence the senseless test.
    TODO
*/
  if((ir&BITS_543)!=BITS_543_011) // (An)+
#endif
  INSTRUCTION_TIME_ROUND(0); 
#endif
}

/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDA, SUBA    |  INSTR     EA   |  1st OP (ea)  |          INSTR            
------------------+-----------------+---------------+-------------------------- 
<ea>,An :         |                 |               |
  .B or .W :      |                 |               |
    Dn            |  8(1/0)  0(0/0) |               |               np       nn 
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  8(1/0)  4(1/0) |            nr |               np       nn 
    (An)+         |  8(1/0)  4(1/0) |            nr |               np       nn 
    -(An)         |  8(1/0)  6(1/0) | n          nr |               np       nn 
    (d16,An)      |  8(1/0)  8(2/0) |      np    nr |               np       nn 
    (d8,An,Xn)    |  8(1/0) 10(2/0) | n    np    nr |               np       nn 
    (xxx).W       |  8(1/0)  8(2/0) |      np    nr |               np       nn 
    (xxx).L       |  8(1/0) 12(3/0) |   np np    nr |               np       nn 
    #<data>       |  8(1/0)  4(1/0) |      np       |               np       nn 
*/

void                             m68k_adda_w(){

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_GET_SOURCE_W;
#if !defined(SSE_CPU_ROUNDING_ADDA)
  INSTRUCTION_TIME(4);
#endif
  m68k_src_l=(signed long)((signed short)m68k_src_w);
#if defined(SSE_CPU_ROUNDING_ADDA_W_DN)
  PREFETCH_IRC;
#else
  PREFETCH_IRC_NO_ROUND;
#endif
////INSTRUCTION_TIME(4);
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ADDA)
  INSTRUCTION_TIME(4);
#endif
  areg[PARAM_N]+=m68k_src_l;
#if !defined(SSE_CPU_ROUNDING_ADDA_W_DN)// no rounding
  INSTRUCTION_TIME_ROUND(0); 
#endif
}

void                              m68k_add_b_from_dN(){

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  switch(ir&BITS_543){
//SS  ADDX.B
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
Dy,Dx :           |                 |
  .B or .W :      |  4(1/0)         |                               np          
-(Ay),-(Ax) :     |                 |
  .B or .W :      | 18(3/1)         |              n nr    nr       np nw       

*/
  case BITS_543_000:
  case BITS_543_001:
    if((ir&BITS_543)==BITS_543_000){
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
#if !defined(SSE_CPU_ROUNDING_ADDX)
      INSTRUCTION_TIME_ROUND(14);
#endif
#if defined(SSE_CPU_ROUNDING_ADDX)
      INSTRUCTION_TIME(2);
#endif
      areg[PARAM_M]--;
      if(PARAM_M==7)areg[PARAM_M]--;
      m68k_src_b=m68k_peek(areg[PARAM_M]);
      areg[PARAM_N]--;
      if(PARAM_N==7)areg[PARAM_N]--;
#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      m68k_SET_DEST_B(areg[PARAM_N]);
#if defined(SSE_CPU_ROUNDING_ADDX)
      CPU_ABUS_ACCESS_READ;
#endif
    }
    m68k_old_dest=m68k_DEST_B;
#if defined(SSE_CPU_PREFETCH_TIMING_ADDX) || defined(SSE_CPU_ROUNDING_ADD_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ADDX)
    if((ir&BITS_543)==BITS_543_001)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B+=m68k_src_b;
    if(sr&SR_X)m68k_DEST_B++;
    SR_ADDX_B;
    break;
  default:
/*
Dn,<ea> :         |                 |              /              |
  .B or .W :      |                 |             |               |
    (An)          |  8(1/1)  4(1/0) |             |            nr | np nw       
    (An)+         |  8(1/1)  4(1/0) |             |            nr | np nw       
    -(An)         |  8(1/1)  6(1/0) |             | n          nr | np nw       
    (d16,An)      |  8(1/1)  8(2/0) |             |      np    nr | np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) |             | n    np    nr | np nw       
    (xxx).W       |  8(1/1)  8(2/0) |             |      np    nr | np nw       
    (xxx).L       |  8(1/1) 12(3/0) |             |   np np    nr | np nw       
*/
#if !defined(SSE_CPU_ROUNDING_ADD)
    INSTRUCTION_TIME(4);
#endif
    EXTRA_PREFETCH; 
    m68k_src_b=LOBYTE(r[PARAM_N]);
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_B_NOT_A;
    m68k_old_dest=m68k_DEST_B;
#if defined(SSE_CPU_PREFETCH_TIMING_ADD) || defined(SSE_CPU_ROUNDING_ADD_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ADD)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_B+=m68k_src_b;
    SR_ADD_B;
  }
#if !defined(SSE_CPU_ROUNDING_ADD) && !defined(SSE_CPU_ROUNDING_ADD_BW_DN)
  INSTRUCTION_TIME_ROUND(0);
#endif
}

void                              m68k_add_w_from_dN(){

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  switch(ir&BITS_543){
//SS  ADDX.W
  case BITS_543_000:
  case BITS_543_001:
    if((ir&BITS_543)==BITS_543_000){
      m68k_src_w=LOWORD(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
#if !defined(SSE_CPU_ROUNDING_ADDX)
      INSTRUCTION_TIME_ROUND(14);
#endif
#if defined(SSE_CPU_ROUNDING_ADDX)
      INSTRUCTION_TIME(2);
#endif
      areg[PARAM_M]-=2;m68k_src_w=m68k_dpeek(areg[PARAM_M]);
#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      areg[PARAM_N]-=2;
      m68k_SET_DEST_W(areg[PARAM_N]);
#if defined(SSE_CPU_ROUNDING_ADDX)
      CPU_ABUS_ACCESS_READ;
#endif
    }
    m68k_old_dest=m68k_DEST_W;
#if defined(SSE_CPU_PREFETCH_TIMING_ADDX) || defined(SSE_CPU_ROUNDING_ADD_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ADDX)
    if((ir&BITS_543)==BITS_543_001)
      CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W+=m68k_src_w;
    if(sr&SR_X)m68k_DEST_W++;
    SR_ADDX_W;
    break;

  default://SS ADD.W Dn,<EA>
#if !defined(SSE_CPU_ROUNDING_ADDX)
    INSTRUCTION_TIME(4);
#endif
    EXTRA_PREFETCH; 
    m68k_src_w=LOWORD(r[PARAM_N]);
#if defined(SSE_CPU_TRUE_PC)
    CHECK_READ=true;
#endif
    m68k_GET_DEST_W_NOT_A;
    m68k_old_dest=m68k_DEST_W;
#if defined(SSE_CPU_PREFETCH_TIMING_ADD) || defined(SSE_CPU_ROUNDING_ADD_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ADD)
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_W+=m68k_src_w;
    SR_ADD_W;
  }
#if !defined(SSE_CPU_ROUNDING_ADD) && !defined(SSE_CPU_ROUNDING_ADD_BW_DN)
  INSTRUCTION_TIME_ROUND(0);
#endif
}

void                              m68k_add_l_from_dN(){ //SS +ADDX

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  switch (ir&BITS_543){ //SS ADDX.L
  case BITS_543_000: 
  case BITS_543_001: 
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
Dy,Dx :           |                 |
  .L :            |  8(1/0)         |                               np       nn 
-(Ay),-(Ax) :     |                 |
  .L :            | 30(5/2)         |              n nr nR nr nR nw np    nW    
*/

    if((ir&BITS_543)==BITS_543_000){
#if defined(SSE_CPU_PREFETCH_TIMING_ADDX)
      PREFETCH_IRC;
#endif
      INSTRUCTION_TIME(4);
      m68k_src_l=r[PARAM_M];
      m68k_dest=&(r[PARAM_N]);
    }else{ //SS -(An), -(An)
#if !defined(SSE_CPU_ROUNDING_ADDX)
      INSTRUCTION_TIME_ROUND(26);
#endif
#if defined(SSE_CPU_ROUNDING_ADDX)
      INSTRUCTION_TIME(2);
#endif
      areg[PARAM_M]-=4;
#if defined(SSE_CPU_ROUNDING_ADDX)
      CPU_ABUS_ACCESS_READ;
      CPU_ABUS_ACCESS_READ;
#endif      
      m68k_src_l=m68k_lpeek(areg[PARAM_M]);
#if defined(SSE_CPU_TRUE_PC)
      CHECK_READ=true;
#endif
      areg[PARAM_N]-=4;m68k_SET_DEST_L(areg[PARAM_N]);
#if defined(SSE_CPU_ROUNDING_ADDX)
      CPU_ABUS_ACCESS_READ;
      CPU_ABUS_ACCESS_READ;
#endif  
#if defined(SSE_CPU_ROUNDING_ADDX)
      CPU_ABUS_ACCESS_WRITE;
#endif
#if defined(SSE_CPU_PREFETCH_TIMING_ADDX)
      PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ADDX)
      CPU_ABUS_ACCESS_WRITE;
#endif
    }
    m68k_old_dest=m68k_DEST_L;
#if !defined(SSE_CPU_PREFETCH_TIMING_ADDX)
#if defined(SSE_CPU_ROUNDING_ADD_L_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif
#endif
    m68k_DEST_L+=m68k_src_l;
    if(sr&SR_X)m68k_DEST_L++; 
    SR_ADDX_L;
    break;
  default:
/*  ADD Dn,<ea>
  .L :            |                 |             |               |
    (An)          | 12(1/2)  8(2/0) |             |            nr | np nw nW    
    (An)+         | 12(1/2)  8(2/0) |             |            nr | np nw nW    
    -(An)         | 12(1/2) 10(2/0) |             | n       nR nr | np nw nW    
    (d16,An)      | 12(1/2) 12(3/0) |             |      np nR nr | np nw nW    
    (d8,An,Xn)    | 12(1/2) 14(3/0) |             | n    np nR nr | np nw nW    
    (xxx).W       | 12(1/2) 12(3/0) |             |      np nR nr | np nw nW    
    (xxx).L       | 12(1/2) 16(4/0) |             |   np np nR nr | np nw nW    
*/
#if !defined(SSE_CPU_ROUNDING_ADD)
    INSTRUCTION_TIME(8);
#endif
    EXTRA_PREFETCH; 
    m68k_src_l=r[PARAM_N];
    m68k_GET_DEST_L_NOT_A;
    m68k_old_dest=m68k_DEST_L;
#if defined(SSE_CPU_PREFETCH_TIMING_ADD) || defined(SSE_CPU_ROUNDING_ADD_BW_DN2)
    PREFETCH_IRC;
#else
    PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
    FETCH_TIMING_NO_ROUND;
#else
    INSTRUCTION_TIME(4);
#endif
#endif
#if defined(SSE_CPU_ROUNDING_ADD)
    CPU_ABUS_ACCESS_WRITE;
    CPU_ABUS_ACCESS_WRITE;
#endif
    m68k_DEST_L+=m68k_src_l;
    SR_ADD_L;
  }
#if !defined(SSE_CPU_ROUNDING_ADD) && !defined(SSE_CPU_ROUNDING_ADD_L_DN2)
  INSTRUCTION_TIME_ROUND(0); 
#endif
}

/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDA, SUBA    |  INSTR     EA   |  1st OP (ea)  |          INSTR            
------------------+-----------------+---------------+-------------------------- 
<ea>,An :         |                 |               |

  .L :            |                 |               |
    Dn            |  8(1/0)  0(0/0) |               |               np       nn 
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  6(1/0)  8(2/0) |         nR nr |               np       n  
    (An)+         |  6(1/0)  8(2/0) |         nR nr |               np       n  
    -(An)         |  6(1/0) 10(2/0) | n       nR nr |               np       n  
    (d16,An)      |  6(1/0) 12(3/0) |      np nR nr |               np       n  
    (d8,An,Xn)    |  6(1/0) 14(3/0) | n    np nR nr |               np       n  
    (xxx).W       |  6(1/0) 12(3/0) |      np nR nr |               np       n  
    (xxx).L       |  6(1/0) 16(4/0) |   np np nR nr |               np       n  
    #<data>       |  8(1/0)  8(2/0) |   np np       |               np       nn 


SS: does it make sense?
The CPU must do the same once it has the <EA>: prefetch and add.
Why would it take more time with Rn and #?
To check in microcodes: maybe data must be transferred on some
internal bus before computing.
*/

void                             m68k_adda_l(){

#if !defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  m68k_GET_SOURCE_L; 
#if defined(SSE_CPU_PREFETCH_TIMING_ADDA) || defined(SSE_CPU_ROUNDING_ADDA_L_DN)
  PREFETCH_IRC; 
#endif
  if (SOURCE_IS_REGISTER_OR_IMMEDIATE
#if defined(SSE_CPU_ROUNDING_ADDA_L_DN2)
/*  Update v3.7 Timings are conform to Yacht.
    This was no bugfix but a hack.
    Now Pulsion 172 works without this. It certainly was compensating
    something else.
*/
    || (ir& (BITS_543|7))==b00111001 // (xxx).L bugfix Pulsion 172
#endif
    ){
    INSTRUCTION_TIME(4);
  }else{
    //Cernit Trandafir:  (An)+, -(An)
    //ASSERT((ir&BITS_543)==b00011000 || (ir&BITS_543)==b00100000); 
    INSTRUCTION_TIME(2);
  }
#if !defined(SSE_CPU_ROUNDING_ADDA_L_DN)
  PREFETCH_IRC_NO_ROUND;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_D_TIMINGS)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  FETCH_TIMING_NO_ROUND;
#else
  INSTRUCTION_TIME(4);
#endif
#endif
  areg[PARAM_N]+=m68k_src_l;
#if !defined(SSE_CPU_ROUNDING_ADDA) && !defined(SSE_CPU_ROUNDING_ADDA_L_DN)
  INSTRUCTION_TIME_ROUND(0); // no rounding: Cernit Trandafir and Summer Delights
#endif
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////  line e - bit shift      ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
*******************************************************************************
  Line 1110
    ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR
*******************************************************************************
-------------------------------------------------------------------------------
     ASL, ASR,    |    Exec Time    |               Data Bus Usage              
     LSL, LSR,    |                 |
     ROL, ROR,    |                 |
    ROXL, ROXR    |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
Dx,Dy :           |                 |               | 
  .B or .W :      |  6+2m(1/0)      |               |               np    n* n  
  .L :            |  8+2m(1/0)      |               |               np    n* nn 
#<data>,Dy :      |                 |               |
  .B or .W :      |  6+2m(1/0)      |               |               np    n  n* 
  .L :            |  8+2m(1/0)      |               |               np    nn n* 
<ea> :            |                 |               |
  .B or .W :      |                 |               |
    (An)          |  8(1/1)  4(1/0) |            nr |               np    nw    
    (An)+         |  8(1/1)  4(1/0) |            nr |               np    nw    
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np    nw    
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np    nw    
    (d8,An)       |  8(1/1) 10(2/0) | n    np    nr |               np    nw    
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np    nw    
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np    nw    
NOTES :
  .'m' is the shift count.
  .'n*' should be replaced by m consecutive 'n'
*/

void                              m68k_asr_b_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if(m68k_src_w>31)m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if (m68k_src_w){
    if( m68k_DEST_B & (BYTE)( 1 << min(m68k_src_w-1,7) )  ){
      SR_SET(SR_C+SR_X);
    }else{
      SR_CLEAR(SR_C+SR_X);
    }
  }
  *((signed char*)m68k_dest)>>=m68k_src_w;
  SR_CHECK_Z_AND_N_B;
}void                             m68k_lsr_b_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if(m68k_src_w>31)m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if (m68k_src_w){
    if(m68k_src_w>8){
      SR_CLEAR(SR_C+SR_X);
    }else{
      if( m68k_DEST_B&(BYTE)( 1<<(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }else{
        SR_CLEAR(SR_C+SR_X);
      }
    }
  }
  *((unsigned char*)m68k_dest)>>=m68k_src_w;
  SR_CHECK_Z_AND_N_B;
}void                             m68k_roxr_b_to_dM(){ //okay
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif

  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=(sr&SR_X);
    if(m68k_DEST_B&1){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned char*)m68k_dest)>>=1;if(old_x)m68k_DEST_B|=MSB_B;
  }
  SR_CHECK_Z_AND_N_B;
}void                             m68k_ror_b_to_dM(){  //okay!
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_B&1;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned char*)m68k_dest)>>=1;if(old_x)m68k_DEST_B|=MSB_B;
  }
  SR_CHECK_Z_AND_N_B;
}
void                              m68k_asr_w_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if(m68k_src_w>31)m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    if( m68k_DEST_W&(WORD)( 1<<min(m68k_src_w-1,15) )  ){
      SR_SET(SR_C+SR_X);
    }else{
      SR_CLEAR(SR_C+SR_X);
    }
    *((signed short*)m68k_dest)>>=m68k_src_w;
  }
  SR_CHECK_Z_AND_N_W;
}void                             m68k_lsr_w_to_dM(){  //okay!
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if(m68k_src_w>31)m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    if(m68k_src_w>16){
      SR_CLEAR(SR_C+SR_X);
    }else{
      if( m68k_DEST_W & (WORD)( 1<<(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }else{
        SR_CLEAR(SR_C+SR_X);
      }
    }
  }
  *((unsigned short*)m68k_dest)>>=m68k_src_w;
  SR_CHECK_Z_AND_N_W;
}void                             m68k_roxr_w_to_dM(){          //okay
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=(sr&SR_X);
    if(m68k_DEST_W&1){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned short*)m68k_dest)>>=1;if(old_x)m68k_DEST_W|=MSB_W;
  }
  SR_CHECK_Z_AND_N_W;
}void                             m68k_ror_w_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_W&1;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned short*)m68k_dest)>>=1;if(old_x)m68k_DEST_W|=MSB_W;
  }
  SR_CHECK_Z_AND_N_W;
}
void                              m68k_asr_l_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);

  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if (m68k_src_w){
    // If shift by 31 or more then test MSB as this is copied to the whole long
    if ( m68k_DEST_L & (1 << min(m68k_src_w-1,31)) ){
      SR_SET(SR_C+SR_X);
    }else{
      SR_CLEAR(SR_C+SR_X);
    }
    // Because MSB->LSB, MSB has been copied to all other bits so 1 extra shift
    // will make no difference.
    if (m68k_src_w>31) m68k_src_w=31;
    *((signed long*)m68k_dest)>>=m68k_src_w;
  }
  SR_CHECK_Z_AND_N_L;
}
void                             m68k_lsr_l_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    if(m68k_src_w>32){
      SR_CLEAR(SR_C+SR_X);
    }else{
      if( m68k_DEST_L&(DWORD)( 1<<(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }else{
        SR_CLEAR(SR_C+SR_X);
      }
    }
  }
  *((unsigned long*)m68k_dest)>>=m68k_src_w;
  if(m68k_src_w>31)m68k_DEST_L=0;
  SR_CHECK_Z_AND_N_L;
}void                             m68k_roxr_l_to_dM(){   //okay!
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=(sr&SR_X);
    if(m68k_DEST_L&1){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned long*)m68k_dest)>>=1;if(old_x)m68k_DEST_L|=MSB_L;
  }
  SR_CHECK_Z_AND_N_L;
}void                             m68k_ror_l_to_dM(){   //okay!
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_L&1;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned long*)m68k_dest)>>=1;if(old_x)m68k_DEST_L|=MSB_L;
  }
  SR_CHECK_Z_AND_N_L;
}
void                              m68k_asl_b_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if (m68k_src_w>31) m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if (m68k_src_w){
    SR_CLEAR(SR_C+SR_X);
    if (m68k_src_w<=8){
      if ( m68k_DEST_B & (BYTE)( MSB_B >> (m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }
    }
    if(m68k_src_w<=7){
      signed char mask=(signed char)(((signed char)(MSB_B))>>(m68k_src_w));
      // mask:  m  m-1 m-2 m-3 ... m-r m-r-1 ...
      //        1   1   1   1       1   0    0...

      if((mask&(m68k_DEST_B))!=0 && ((mask&(m68k_DEST_B))^mask)!=0){
        SR_SET(SR_V);
      }
    }else if(m68k_DEST_B){
      SR_SET(SR_V);
    }
  }
  *((signed char*)m68k_dest)<<=m68k_src_w;
  SR_CHECK_Z_AND_N_B;
}

void                             m68k_lsl_b_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;

  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if(m68k_src_w>31)m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    SR_CLEAR(SR_X);
    if(m68k_src_w<=8){
      if( m68k_DEST_B&(BYTE)( MSB_B>>(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }
    }
  }
  *((unsigned char*)m68k_dest)<<=m68k_src_w;
  SR_CHECK_Z_AND_N_B;
}

void                             m68k_roxl_b_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif

  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=(sr&SR_X);
    if(m68k_DEST_B&MSB_B){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned char*)m68k_dest)<<=1;if(old_x)m68k_DEST_B|=1;
  }
  SR_CHECK_Z_AND_N_B;
}

void                             m68k_rol_b_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_B&MSB_B;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned char*)m68k_dest)<<=1;if(old_x)m68k_DEST_B|=1;
  }
  SR_CHECK_Z_AND_N_B;
}

void                              m68k_asl_w_to_dM(){       //okay!
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if(m68k_src_w>31)m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    SR_CLEAR(SR_C+SR_X);
    if(m68k_src_w<=16){
      if( m68k_DEST_W&(WORD)( MSB_W>>(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }
    }
    if(m68k_src_w<=15){
      signed short mask=(signed short)(((signed short)(MSB_W))>>(m68k_src_w));
      if((mask&(m68k_DEST_W))!=0 && ((mask&(m68k_DEST_W))^mask)!=0){
        SR_SET(SR_V);
      }
    }else if(m68k_DEST_W){
      SR_SET(SR_V);
    }
  }
  *((signed short*)m68k_dest)<<=m68k_src_w;
  SR_CHECK_Z_AND_N_W;
}

void                             m68k_lsl_w_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  if(m68k_src_w>31)m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    SR_CLEAR(SR_X);
    if(m68k_src_w<=16){
      if( m68k_DEST_W&(WORD)( MSB_W>>(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }
    }
  }
  *((unsigned short*)m68k_dest)<<=m68k_src_w;

  SR_CHECK_Z_AND_N_W;
}

void                             m68k_roxl_w_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=(sr&SR_X);
    if(m68k_DEST_W&MSB_W){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned short*)m68k_dest)<<=1;if(old_x)m68k_DEST_W|=1;
  }
  SR_CHECK_Z_AND_N_W;
}

void                             m68k_rol_w_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(2+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_W&MSB_W;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned short*)m68k_dest)<<=1;if(old_x)m68k_DEST_W|=1;
  }
  SR_CHECK_Z_AND_N_W;
}

void                              m68k_asl_l_to_dM(){    //okay!
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    SR_CLEAR(SR_C+SR_X);
    if(m68k_src_w<=32){
      if( m68k_DEST_L&(LONG)( MSB_L>>(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }
    }
    if(m68k_src_w<=31){
      signed long mask=(((signed long)(MSB_L))>>(m68k_src_w));
      if((mask&(m68k_DEST_L))!=0 && ((mask&(m68k_DEST_L))^mask)!=0){
        SR_SET(SR_V);
      }
    }else if(m68k_DEST_L){
      SR_SET(SR_V);
    }
  }
  *((signed long*)m68k_dest)<<=m68k_src_w;
  if(m68k_src_w>31)m68k_DEST_L=0;
  SR_CHECK_Z_AND_N_L;
}

void                             m68k_lsl_l_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
    SR_CLEAR(SR_X);
    if(m68k_src_w<=32){
      if( m68k_DEST_L&(LONG)( MSB_L>>(m68k_src_w-1) )  ){
        SR_SET(SR_C+SR_X);
      }
    }
  }
  *((unsigned long*)m68k_dest)<<=m68k_src_w;
  if(m68k_src_w>31)m68k_DEST_L=0;
  SR_CHECK_Z_AND_N_L;
}

void                             m68k_roxl_l_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=(sr&SR_X);
    if(m68k_DEST_L&MSB_L){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned long*)m68k_dest)<<=1;if(old_x)m68k_DEST_L|=1;
  }
  SR_CHECK_Z_AND_N_L;
}void                             m68k_rol_l_to_dM(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

  m68k_BIT_SHIFT_TO_dM_GET_SOURCE;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  INSTRUCTION_TIME(4+2*m68k_src_w);
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_L&MSB_L;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned long*)m68k_dest)<<=1;if(old_x)m68k_DEST_L|=1;
  }
  SR_CHECK_Z_AND_N_L;
}
void                              m68k_bit_shift_right_to_mem(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif
#if defined(SSE_CPU_ASSERT_ILLEGAL6)
  switch(ir&BITS_ba9){
  case BITS_ba9_000:
  case BITS_ba9_001:
  case BITS_ba9_010:
  case BITS_ba9_011:
    break;
  default:
    m68k_unrecognised();
  }
#endif
#if !defined(SSE_CPU_ROUNDING_SHIFT_MEM)
  INSTRUCTION_TIME(4); 
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
  m68k_GET_DEST_W_NOT_A_OR_D;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  switch(ir&BITS_ba9){
  case BITS_ba9_000:
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
    if(m68k_DEST_W&1){
      SR_SET(SR_C+SR_X);
    }
    *((signed short*)m68k_dest)>>=1;
    SR_CHECK_Z_AND_N_W;
    break;
  case BITS_ba9_001:
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
    if(m68k_DEST_W&1){
      SR_SET(SR_C+SR_X);

    }
    *((unsigned short*)m68k_dest)>>=1;
    SR_CHECK_Z_AND_N_W;
    break;
  case BITS_ba9_010:{
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
    bool old_x=(sr&SR_X);
    if(m68k_DEST_W&1){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned short*)m68k_dest)>>=1;if(old_x)m68k_DEST_W|=MSB_W;
    SR_CHECK_Z_AND_N_W;
    break;
  }case BITS_ba9_011:{
    SR_CLEAR(SR_N+SR_V+SR_Z);
    bool old_x=m68k_DEST_W&1;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned short*)m68k_dest)>>=1;if(old_x)m68k_DEST_W|=MSB_W;
    SR_CHECK_Z_AND_N_W;
    break;
  }
#if !defined(SSE_CPU_ASSERT_ILLEGAL6)  
  default:
    m68k_unrecognised(); // SS it doesn't pay to crash before
    break;
#endif
  }
#if defined(SSE_CPU_ROUNDING_SHIFT_MEM)
  CPU_ABUS_ACCESS_WRITE;
#endif
}

void                              m68k_bit_shift_left_to_mem(){
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS))
  FETCH_TIMING;
#endif

#if defined(SSE_CPU_ASSERT_ILLEGAL6)
  switch(ir&BITS_ba9){
  case BITS_ba9_000:
  case BITS_ba9_001:
  case BITS_ba9_010:
  case BITS_ba9_011:
    break;
  default:
    m68k_unrecognised();
  }
#endif
#if !defined(SSE_CPU_ROUNDING_SHIFT_MEM)
  INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_TRUE_PC)
  CHECK_READ=true;
#endif
  m68k_GET_DEST_W_NOT_A_OR_D;
  PREFETCH_IRC;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_E_TIMINGS)
  FETCH_TIMING;
#endif
  switch(ir&BITS_ba9){
  case BITS_ba9_000: //asl
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
//    if( m68k_DEST_W&(WORD)( MSB_W>>(m68k_src_w-1) )  ){
    if( m68k_DEST_W&(WORD)( MSB_W )  ){
      SR_SET(SR_C+SR_X);
    }
    if((m68k_DEST_W&0xc000)==0x8000 || (m68k_DEST_W&0xc000)==0x4000){
      SR_SET(SR_V);
    }
    *((signed short*)m68k_dest)<<=1;
    SR_CHECK_Z_AND_N_W;
    break;
  case BITS_ba9_001:
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
    if(m68k_DEST_W&MSB_W){
      SR_SET(SR_C+SR_X);
    }
    *((unsigned short*)m68k_dest)<<=1;
    SR_CHECK_Z_AND_N_W;
    break;
  case BITS_ba9_010:{
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);if(sr&SR_X)SR_SET(SR_C);
    bool old_x=(sr&SR_X);
    if(m68k_DEST_W&MSB_W){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned short*)m68k_dest)<<=1;if(old_x)m68k_DEST_W|=1;
    SR_CHECK_Z_AND_N_W;
    break;
  }case BITS_ba9_011:{
    SR_CLEAR(SR_N+SR_V+SR_Z);
    bool old_x=m68k_DEST_W&MSB_W;
    if(old_x){
      SR_SET(SR_C)
    }else{
      SR_CLEAR(SR_C)
    }
    *((unsigned short*)m68k_dest)<<=1;if(old_x)m68k_DEST_W|=1;
    SR_CHECK_Z_AND_N_W;
    break;
  }
#if !defined(SSE_CPU_ASSERT_ILLEGAL6)    
  default:
    m68k_unrecognised();
    break;
#endif
  }
#if defined(SSE_CPU_ROUNDING_SHIFT_MEM)
  CPU_ABUS_ACCESS_WRITE;
#endif
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////  HIGH NIBBLE ROUTINES    ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


extern "C" void m68k_0000(){ //immediate stuff
  m68k_jump_line_0[(ir&(BITS_876|BITS_ba9))>>6]();
}

// Note the INSTRUCTION_TIME were commented out by Steem authors, and
// the other comments are theirs too. We define our versions of the MOVE
// instructions in SSECpu.cpp.

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_MOVE_B)
void m68k_0001();
#else

void m68k_0001(){  //move.b
  INSTRUCTION_TIME(4); // I don't think this should be here, does move read on cycle 0?
  m68k_GET_SOURCE_B;
  if((ir&BITS_876)==BITS_876_000){
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_B=m68k_src_b;
    SR_CHECK_Z_AND_N_B;
  }else if((ir&BITS_876)==BITS_876_001){
    m68k_unrecognised();
  }else{   //to memory
    bool refetch=0;
    switch(ir&BITS_876){
    case BITS_876_010:
//      INSTRUCTION_TIME(8-4-4);
      abus=areg[PARAM_N];
      break;
    case BITS_876_011:
//      INSTRUCTION_TIME(8-4-4);
      abus=areg[PARAM_N];
      areg[PARAM_N]++;
      if(PARAM_N==7)areg[7]++;
      break;
    case BITS_876_100:
//      INSTRUCTION_TIME(8-4-4);
      areg[PARAM_N]--;
      if(PARAM_N==7)areg[7]--;
      abus=areg[PARAM_N];
      break;
    case BITS_876_101:
      INSTRUCTION_TIME(12-4-4);
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_876_110:
      INSTRUCTION_TIME(14-4-4);
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
        case BITS_ba9_000:
          INSTRUCTION_TIME(12-4-4);
          abus=0xffffff & (unsigned long)((signed long)((signed short)m68k_fetchW()));
          pc+=2;
          break;
        case BITS_ba9_001:
          INSTRUCTION_TIME(16-4-4);
          abus=m68k_fetchL() & 0xffffff;
          pc+=4;
          break;
        default:
          m68k_unrecognised();
      }
    }
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if(!m68k_src_b){
      SR_SET(SR_Z);
    }
    if(m68k_src_b&MSB_B){
      SR_SET(SR_N);
    }

    m68k_poke_abus(m68k_src_b);
    FETCH_TIMING;  // move fetches after instruction
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
  }
}

#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_MOVE_L)
void m68k_0010();
#else

void m68k_0010()  //move.l
{
  INSTRUCTION_TIME(4);
  m68k_GET_SOURCE_L;
  if ((ir & BITS_876)==BITS_876_000){
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_L=m68k_src_l;
    SR_CHECK_Z_AND_N_L;
  }else if ((ir & BITS_876)==BITS_876_001){
//    INSTRUCTION_TIME(4);
    areg[PARAM_N]=m68k_src_l;
  }else{   //to memory
    bool refetch=0;
    switch(ir&BITS_876){
    case BITS_876_010:
      INSTRUCTION_TIME(12-4-4);
      abus=areg[PARAM_N];
      break;
    case BITS_876_011:
      INSTRUCTION_TIME(12-4-4);
      abus=areg[PARAM_N];
      areg[PARAM_N]+=4;
      break;
    case BITS_876_100:
      INSTRUCTION_TIME(12-4-4);
      areg[PARAM_N]-=4;
      abus=areg[PARAM_N];
      break;
    case BITS_876_101:
      INSTRUCTION_TIME(16-4-4);
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2;
      break;
    case BITS_876_110:
      INSTRUCTION_TIME(18-4-4);
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
      case BITS_ba9_000:
        INSTRUCTION_TIME(16-4-4);
        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2;
        break;
      case BITS_ba9_001:
        INSTRUCTION_TIME(20-4-4);
        abus=m68k_fetchL()&0xffffff;
        pc+=4;
        break;
      default:
        m68k_unrecognised();
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
    FETCH_TIMING; // move fetches after instruction
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
  }
}

#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_MOVE_W)
void m68k_0011();
#else

void m68k_0011() //move.w
{
  INSTRUCTION_TIME(4);
  m68k_GET_SOURCE_W;
  if ((ir & BITS_876)==BITS_876_000){
//    INSTRUCTION_TIME(4);
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_W=m68k_src_w;
    SR_CHECK_Z_AND_N_W;
  }else if ((ir & BITS_876)==BITS_876_001){
//    INSTRUCTION_TIME(4);
    areg[PARAM_N]=(signed long)((signed short)m68k_src_w);
  }else{   //to memory
    bool refetch=0;
    switch (ir & BITS_876){
    case BITS_876_010:
//      INSTRUCTION_TIME(8-4);
      abus=areg[PARAM_N];
      break;
    case BITS_876_011:
//      INSTRUCTION_TIME(8-4);
      abus=areg[PARAM_N];
      areg[PARAM_N]+=2;
      break;
    case BITS_876_100:
//      INSTRUCTION_TIME(8-4);
      areg[PARAM_N]-=2;
      abus=areg[PARAM_N];
      break;
    case BITS_876_101:
      INSTRUCTION_TIME(12-4-4);
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2;
      break;
    case BITS_876_110:
      INSTRUCTION_TIME(14-4-4);
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
      case BITS_ba9_000:
        INSTRUCTION_TIME(12-4-4);
        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2;
        break;
      case BITS_ba9_001:
        INSTRUCTION_TIME(16-4-4);
        abus=m68k_fetchL()&0xffffff;
        pc+=4;
        break;
      default:
        m68k_unrecognised();
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
    FETCH_TIMING; // move fetches after instruction
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
  }
}

#endif

extern "C" void m68k_0100(){
  m68k_jump_line_4[(ir&(BITS_ba9|BITS_876))>>6]();
}

extern "C" void m68k_0101(){
  m68k_jump_line_5[(ir&BITS_876)>>6]();
}

//SS line6; this part has become unreadable!

extern "C" void m68k_0110(){  //bCC //SS + BSR
  if (LOBYTE(ir)){ //SS 8-bit displacement
    MEM_ADDRESS new_pc=(pc+(signed long)((signed char)LOBYTE(ir))) | pc_high_byte;
    if ((ir & 0xf00)==0x100){ //BSR
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=2;
#endif
      m68k_PUSH_L(PC32);
#if defined(SSE_BOILER_PSEUDO_STACK)
      Debug.PseudoStackPush(PC32);
#endif
#if defined(SSE_CPU_ROUNDING_BUS)
      M68000.Rounded=false;
#endif
      m68k_READ_W(new_pc); // Check for bus/address errors
#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING))
      INSTRUCTION_TIME(18-4);
#endif
      SET_PC(new_pc);
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
      FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
      CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
      INSTRUCTION_TIME_ROUND(18); // round for fetch
#endif
    }else{ //SS Bcc
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        Bcc       |      INSTR      |                   INSTR
------------------+-----------------+------------------------------------------
<label> :         |                 |
 .B or .S :       |                 |
  branch taken    | 10(2/0)         |                 n          np np          
  branch not taken|  8(1/0)         |                nn             np          
*/

#if defined(SSE_CPU_ROUNDING_BCC)
      INSTRUCTION_TIME(2);
#endif
      if (m68k_CONDITION_TEST){ //SS branch taken
#if defined(SSE_CPU_ROUNDING_BCC) && !defined(SSE_CPU_ROUNDING_BUS)
        CPU_ABUS_ACCESS_READ;
#endif
#if defined(SSE_CPU_ROUNDING_BUS)
        M68000.Rounded=false;
#endif
        m68k_READ_W(new_pc); // Check for bus/address errors
#if defined(SSE_CPU_ROUNDING_BCC) && defined(SSE_CPU_ROUNDING_BUS)
        CPU_ABUS_ACCESS_READ;
#endif
#if !defined(SSE_CPU_ROUNDING_BCC)
#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING))
        INSTRUCTION_TIME(10-4); // DSOS Lots of scrollers
#endif
#endif
        SET_PC(new_pc);
#if !defined(SSE_CPU_ROUNDING_BCC)
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING))
        INSTRUCTION_TIME_ROUND(10); // round for fetch
#endif
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
        FETCH_TIMING;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
        CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
      }else{ //SS branch not taken
#if defined(SSE_CPU_ROUNDING_BCC)
        INSTRUCTION_TIME(2);
#endif
#if !defined(SSE_CPU_ROUNDING_BCC)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
        INSTRUCTION_TIME(8-4);    
        FETCH_TIMING;
#else
        INSTRUCTION_TIME_ROUND(8); // round for fetch
#endif
#endif
        PREFETCH_IRC;
      }
    }
  }else{
//#undef SSE_CPU_LINE_6_TIMINGS
    if ((ir & 0xf00)==0x100){ //bsr.l //SS .W?
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=2;
#endif
      m68k_PUSH_L(PC32+2);
#if defined(SSE_BOILER_PSEUDO_STACK)
      Debug.PseudoStackPush(PC32+2);
#endif
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      INSTRUCTION_TIME(2);
      CPU_ABUS_ACCESS_WRITE;
      CPU_ABUS_ACCESS_WRITE;
      CPU_ABUS_ACCESS_READ_FETCH;
#endif
      MEM_ADDRESS new_pc=(pc+(signed long)((signed short)m68k_fetchW())) | pc_high_byte;
      // stacked pc is always instruction pc+2 due to prefetch (pc doesn't increase before new_pc is read)
#if defined(SSE_CPU_ROUNDING_BUS)
      M68000.Rounded=false;
#endif
      m68k_READ_W(new_pc); // Check for bus/address errors
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING) && !defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
      INSTRUCTION_TIME(18-4);    
#endif
      SET_PC(new_pc);      
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
      FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
      CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
      INSTRUCTION_TIME_ROUND(18); // round for fetch
#endif
    }else{ // Bcc.l //SS .W?
//#undef SSE_CPU_ROUNDING_BCC
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        Bcc       |      INSTR      |                   INSTR
------------------+-----------------+------------------------------------------
<label> :         |                 |
 .W :             |                 |
  branch taken    | 10(2/0)         |                 n          np np          
  branch not taken| 12(2/0)         |                nn          np np          
*/
#if defined(SSE_CPU_ROUNDING_BCC)
      INSTRUCTION_TIME(2);
#endif
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
#if defined(SSE_CPU_ROUNDING_BUS)
      M68000.Rounded=false;//? will be useless in the end
#endif
      //CPU_ABUS_ACCESS_READ_FETCH;
      MEM_ADDRESS new_pc;
#else
      MEM_ADDRESS new_pc=(pc+(signed long)((signed short)m68k_fetchW())) | pc_high_byte;
#endif
      if (m68k_CONDITION_TEST){ //SS bramch taken
        // stacked pc is always instruction pc+2 due to prefetch (pc doesn't increase before new_pc is read)
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        CPU_ABUS_ACCESS_READ_FETCH;
        new_pc=(pc+(signed long)((signed short)m68k_fetchW())) | pc_high_byte;
#elif defined(SSE_CPU_ROUNDING_BCC)
        CPU_ABUS_ACCESS_READ;
#endif
        m68k_READ_W(new_pc); // Check for bus/address errors
#if !defined(SSE_CPU_ROUNDING_BCC) && defined(SSE_CPU_FETCH_TIMING)
        INSTRUCTION_TIME(10-4);
#endif
        SET_PC(new_pc);
#if !defined(SSE_CPU_ROUNDING_BCC)
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING))
        INSTRUCTION_TIME_ROUND(10); // round for fetch
#endif
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
        FETCH_TIMING;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
        CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
      }else{
#if defined(SSE_CPU_ROUNDING_BCC)
        INSTRUCTION_TIME(2);
#if defined(SSE_CPU_TIMINGS_REFACTOR_FETCH)
        // fetched but not used
        // we surmise M68000 has a 'fetch' routine to advance PC, and not
        // another one to advance PC without fetching?
        CPU_ABUS_ACCESS_READ_FETCH;
        new_pc=(pc+(signed long)((signed short)m68k_fetchW())) | pc_high_byte;
#else
        CPU_ABUS_ACCESS_READ;
#endif
#endif
        pc+=2; 
#if !defined(SSE_CPU_ROUNDING_BCC)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
        INSTRUCTION_TIME(12-4);    
        FETCH_TIMING;
#else
        INSTRUCTION_TIME_ROUND(12); // round for fetch
#endif
#endif
        PREFETCH_IRC;
      }
    }
  }
//#define SSE_CPU_LINE_6_TIMINGS
}

/*
*******************************************************************************
  Line 0111
    MOVEQ
*******************************************************************************
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       MOVEQ      |      INSTR      |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
#<data>,Dn :      |                 |               
  .L :            |  4(1/0)         |                               np          

*/

extern "C" void m68k_0111(){  //moveq
  if(ir&BIT_8){
    m68k_unrecognised();
  }else{
#if defined(SSE_CPU_PREFETCH_CLASS)
    M68000.PrefetchClass=1;
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_7_TIMINGS))
    FETCH_TIMING;
#endif
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_L=(signed long)((signed char)LOBYTE(ir));
#if (defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_7_TIMINGS))
    FETCH_TIMING;
#endif
    PREFETCH_IRC;
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    SR_CHECK_Z_AND_N_L;
  }
}

extern "C" void m68k_1000(){ //or, div, sbcd
  m68k_jump_line_8[(ir&BITS_876)>>6]();
}

extern "C" void m68k_1001(){ //sub
/*  SS bits 876 = opmode
Byte Word Long        Operation
 000  001  010      Dn – < ea > 
 100  101  110      < ea > – Dn 

 + SUBA:

  m68k_jump_line_9[0]=m68k_sub_b_to_dN;
  m68k_jump_line_9[1]=m68k_sub_w_to_dN;
  m68k_jump_line_9[2]=m68k_sub_l_to_dN;
  m68k_jump_line_9[3]=m68k_suba_w;
  m68k_jump_line_9[4]=m68k_sub_b_from_dN; + subx.b
  m68k_jump_line_9[5]=m68k_sub_w_from_dN; + subx.w
  m68k_jump_line_9[6]=m68k_sub_l_from_dN; + subx.l
  m68k_jump_line_9[7]=m68k_suba_l;

SUBX:

bits 876 = 1ss with ss: 00 byte 01 word 10 long
bit 3 =  0: SUBX Dx,Dy  1: – (Ax), – (Ay)

*/
  m68k_jump_line_9[(ir&BITS_876)>>6]();
}

extern "C" void m68k_1010() //line-a
{
  pc-=2;  //pc not incremented for illegal instruction
//  log_write("CPU sees line-a instruction");
//  intercept_line_a();
  INSTRUCTION_TIME_ROUND(0);  // Round first for interrupts
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  INSTRUCTION_TIME_ROUND(34-4);
  FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
  CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
  INSTRUCTION_TIME_ROUND(34);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("LINEA");
#endif
  m68k_interrupt(LPEEK(BOMBS_LINE_A*4));
  m68k_do_trace_exception=0;
  debug_check_break_on_irq(BREAK_IRQ_LINEA_IDX);
}

extern "C" void m68k_1011(){ //cmp, eor
  m68k_jump_line_b[(ir&BITS_876)>>6]();
}

extern "C" void m68k_1100(){ // and, abcd, exg, mul
  m68k_jump_line_c[(ir&BITS_876)>>6]();
}

extern "C" void m68k_1101(){   //add
  m68k_jump_line_d[(ir&BITS_876)>>6]();
}

extern "C" void m68k_1110(){  //bit shift
  m68k_jump_line_e[(ir&(BITS_876|BITS_543))>>3]();
}

extern "C" void m68k_1111(){  //line-f emulator
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_LINE_F)
  interrupt_depth--; // compensate
  on_rte=ON_RTE_LINE_F;
#endif
  pc-=2;  //pc not incremented for illegal instruction
#ifdef ONEGAME
  if (ir==0xffff){
    OGIntercept();
    return;
  }
#endif

  INSTRUCTION_TIME_ROUND(0);  // Round first for interrupts
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
  INSTRUCTION_TIME_ROUND(34-4);
  FETCH_TIMING;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
  CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
  INSTRUCTION_TIME_ROUND(34);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
//  Debug.RecordInterrupt("LINEF");
#endif
  m68k_interrupt(LPEEK(BOMBS_LINE_F*4));
  m68k_do_trace_exception=0;

  debug_check_break_on_irq(BREAK_IRQ_LINEF_IDX);
}
//       check PC-relative addressing  // SS: gulp

#include "cpuinit.cpp"

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU) \
  && !defined(SSE_STRUCTURE_SSECPU_OBJ)
#include "SSE/SSECpu.cpp"
#endif
