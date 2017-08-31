/*---------------------------------------------------------------------------
FILE: cpu_sse.cpp
MODULE: emu
DESCRIPTION: This file is compiled instead of cpu.cpp in the SSE build if
SSE_CPU is defined. We use this way here because this is a large file.

This emulation now strictly follows the YACHT (Yet Another Cycle Hunting Table)
table of Motorola 68000 timings, except for... exceptions.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: cpu_sse.cpp")
#endif

#define EXT
#define INIT(s) =s

extern const char*exception_action_name[4];//390

m68k_exception ExceptionObject;
jmp_buf *pJmpBuf=NULL;
EXT BYTE  m68k_peek(MEM_ADDRESS ad);
EXT WORD  m68k_dpeek(MEM_ADDRESS ad);
EXT LONG  m68k_lpeek(MEM_ADDRESS ad);
#ifdef DEBUG_BUILD
#ifndef RELEASE_BUILD
EXT MEM_ADDRESS pc_rel_stop_on_ref;
#endif
#endif

#undef EXT
#undef INIT

#ifdef SSE_UNIX //TODO
#if !defined(min)
#define min(a,b) (a>b ? b:a)
#define max(a,b) (a>b ? a:b)
#endif
#endif

WORD *lpfetch,*lpfetch_bound;
bool prefetched_2=false;
WORD prefetch_buf[2]; //2 words prefetch queue

#ifdef ENABLE_LOGFILE
MEM_ADDRESS debug_mem_write_log_address;
int debug_mem_write_log_bytes;
#endif

bool cpu_stopped=0;
#if !defined(SSE_CPU_TRACE_LINE_A_F)
bool m68k_do_trace_exception;
#endif
signed int compare_buffer;

#if defined(SSE_VAR_OPT_390A) //TODO
COUNTER_VAR act; 
#endif

#if defined(SSE_CPU_RESTORE_ABUS)
MEM_ADDRESS dest_addr;
#endif

#if defined(SSE_CPU_TRUE_PC)
#define TRUE_PC M68000.Pc
#define CHECK_READ M68000.CheckRead
#else
int dummy_for_true_pc; 
#define TRUE_PC dummy_for_true_pc
#define CHECK_READ dummy_for_true_pc
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
bool (*m68k_jump_condition_test[16])();

// big functions not much used, not inlined
#define m68k_READ_B_FROM_ADDR m68kReadBFromAddr();
#define m68k_READ_W_FROM_ADDR m68kReadWFromAddr();
#if !defined(SSE_CPU_SPLIT_RL)
#define m68k_READ_L_FROM_ADDR m68kReadLFromAddr();
#endif
//---------------------------------------------------------------------------
void m68k_interrupt(MEM_ADDRESS ad) {

#if defined(SSE_CPU_392B)
  ASSERT(M68000.ProcessingState==TM68000::EXCEPTION);
#endif
  // we don't count cycles here, they are variable (trace, irq...)
  M68K_UNSTOP; // eg Hackabounds Demo intro (STOP in trace)
  WORD _sr=sr;
  if (!SUPERFLAG) 
    change_to_supervisor_mode();
  PREFETCH_CLASS(2);
#if defined(SSE_BOILER_68030_STACK_FRAME)
  if(Debug.M68030StackFrame)
  {//macro must be scoped!
    m68k_PUSH_W(0); // format + offset, both may be 0
  }
#endif
  m68k_PUSH_L(PC32);
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPush(PC32);
#endif
  m68k_PUSH_W(_sr);
  SET_PC(ad);
#if defined(SSE_VC_INTRINSICS_390E)
  BITRESET(sr,SR_TRACE_BIT);
#else
  SR_CLEAR(SR_TRACE);
#endif
  interrupt_depth++;
//  TRACE_LOG("%X %d\n",ad,interrupt_depth);
}
//---------------------------------------------------------------------------
#define LOGSECTION LOGSECTION_CRASH
void m68k_exception::init(int a,exception_action ea,MEM_ADDRESS _abus)
{
  bombs=a;
  _pc=PC32; //old_pc;
  crash_address=old_pc; //SS this is for group 1+2
  address=_abus;
  _sr=::sr;_ir=::ir;
  action=ea;
}
//---------------------------------------------------------------------------
void ASMCALL perform_crash_and_burn()
{
#if defined(SSE_CPU_HALT)
  TRACE("HALT\n");
  runstate=RUNSTATE_STOPPING;
  M68000.ProcessingState=TM68000::HALTED;
#if defined(SSE_GUI_STATUS_BAR)
  GUIRefreshStatusBar();
#endif
#else
  reset_st(RESET_COLD | RESET_NOSTOP | RESET_CHANGESETTINGS | RESET_NOBACKUP);
  osd_start_scroller(T("CRASH AND BURN - ST RESET"));
  TRACE("CRASH AND BURN - ST RESET  \n");
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
    dbg_log("");
    dbg_log("****************************************");
    if (bombs){
      dbg_log(EasyStr(bombs)+" bombs");
    }else{
      dbg_log("Exception/interrupt");
    }
    dbg_log(EasyStr("Crash at ")+HEXSl(crash_address,6));
    int n=pc_history_idx-HIST_MENU_SIZE;
    if (n<0) n=HISTORY_SIZE+n;
    EasyStr Dissasembly;
    do{
      if (pc_history[n]!=0xffffff71){
        Dissasembly=disa_d2(pc_history[n]);
        dbg_log(EasyStr(HEXSl(pc_history[n],6))+" - "+Dissasembly);
      }
      n++; if (n>=HISTORY_SIZE) n=0;
    }while (n!=pc_history_idx);
    dbg_log("^^ Crash!");
    dbg_log("****************************************");
    dbg_log("");
  }
}
#endif
//---------------------------------------------------------------------------
// TODO: Allow exception frames to be written to IO?


void m68k_exception::crash() {
  DWORD bytes_to_stack=int((bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR)
    ? (4+2+2+4+2):(4+2));
  MEM_ADDRESS sp=(MEM_ADDRESS)(SUPERFLAG 
    ? (areg[7] & 0xffffff):(other_sp & 0xffffff));
  PREFETCH_CLASS(2);

/*
Table 6-1. Reference Classification
Function Code Output
FC2 FC1 FC0 Address Space
0 0 0 (Undefined, Reserved)*
0 0 1 User Data
0 1 0 User Program
0 1 1 (Undefined, Reserved)*
1 0 0 (Undefined, Reserved)*
1 0 1 Supervisor Data
1 1 0 Supervisor Program
1 1 1 CPU Space
*Address space 3 is reserved for user definition, while 0 and
4 are reserved for future use by Motorola.

  For those cases, we generally get "data" at this point, but "program"
  may be more appropriate.
  TODO, it's preliminary  
*/
  if(bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR)
  {
    switch(_ir)
    {
    case 0x4e73://rte
    case 0x4e75://rts - Fixes Blood Money -SUP 1MB
      action=EA_FETCH;
    }
    if((_ir&0xF000)==0x6000 //bra etc
      || (_ir&0xFFC0)==0x4EC0) // jmp (?)
    {
      action=EA_FETCH;
    }
  }

#if defined(SSE_DEBUG)   
  if(M68000.nExceptions!=-1)
  {
    M68000.nExceptions++;  
    TRACE_LOG("\nException #%d, %d bombs (",M68000.nExceptions,bombs);

#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK_CPU & OSD_CONTROL_CPUBOMBS) 
#endif
    TRACE_OSD("%d-%d BOMBS",M68000.nExceptions,bombs);
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
#if defined(SSE_CPU_TRUE_PC)
#if defined(DEBUG_BUILD)
    TRACE_LOG(") during \"%s\"\n",exception_action_name[action]);
#else
    TRACE_LOG(") during %s\n",(action==EA_READ||CHECK_READ)?"Read":"Write");
#endif
#else
    TRACE_LOG(") during %s\n",(action==EA_READ)?"Read":"Write (maybe!)");
#endif
#ifdef DEBUG_BUILD 
    EasyStr instr=disa_d2(old_pc); // take advantage of the disassembler
    //TRACE_LOG("PC=%X-IR=%X-Ins: %s -SR=%X-Bus=%X",old_pc,ir,instr.Text,sr,abus);
    TRACE_LOG("PC=%X-IR=%X-Ins: %s -SR=%X-Bus=%X",old_pc,_ir,instr.Text,sr,abus);
#else
    TRACE_LOG("PC=%X-IR=%X-SR=%X-Bus=%X",old_pc,ir,sr,abus);
#endif
    TRACE_LOG("-Vector $%X=%X\n",bombs*4,LPEEK(bombs*4));
    // dump registers
    for(int i=0;i<8;i++) // D0-D7
      TRACE_LOG("D%d=%X ",i,r[i]);
    TRACE_LOG("\n");
    for(int i=0;i<8;i++) // A0-A7 (A7 when the exception occurred)
      TRACE_LOG("A%d=%X ",i,areg[i]);
    TRACE_LOG("\n");
  }

#endif
#if defined(SSE_MMU_392)
  if (sp<bytes_to_stack || sp>mem_len)
#else
  if (sp<bytes_to_stack || sp>FOUR_MEGS)
#endif
  {
    // Double bus error, CPU halt (we crash and burn)
    // This only has to be done here, m68k_PUSH_ will cause bus error if invalid
    // TODO: there should be other conditions, see WinUAE
    DEBUG_ONLY( log_history(bombs,crash_address) );
    TRACE_LOG("Double bus error SP:%d < bytes to stack %d\n",sp,bytes_to_stack);
    perform_crash_and_burn();
  }
  else
  {
#if defined(SSE_CPU_HALT)
    M68000.ProcessingState=TM68000::EXCEPTION;
#endif
    M68000.tpend=false;
    if(bombs==BOMBS_ILLEGAL_INSTRUCTION || bombs==BOMBS_PRIVILEGE_VIOLATION)
    {
/*
YACHT
Illegal Instruction | 34(4/3)  |              nn    ns nS ns nV nv np np 
WinUAE:             | 34(4/3)  |              nn    ns nS ns nV nv np n  np 
ST:                 | 36(4/3)  |              nn    ns nS ns nV nv np n+ np    
*/
      INSTRUCTION_TIME(4); //nn
      if(!SUPERFLAG) 
        change_to_supervisor_mode();
      TRACE_LOG("Push crash address %X->%X on %X\n",crash_address,(crash_address & 0x00ffffff) | pc_high_byte,r[15]-4);
      CPU_ABUS_ACCESS_WRITE_PUSH_L; // ns nS
      m68k_PUSH_L(( (crash_address) & 0x00ffffff) | pc_high_byte);  // crash address = old_pc
      TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
      CPU_ABUS_ACCESS_WRITE_PUSH; // ns
      m68k_PUSH_W(_sr); // Status register 
      MEM_ADDRESS ad=LPEEK(bombs*4); // Get the vector
      abus=ad;//390
      CPU_ABUS_ACCESS_READ_L; //nV nv
      if(ad & 1) // bad vector!
      {
        // Very rare, generally indicates emulation/snapshot bug, but there are cases
        bombs=BOMBS_ADDRESS_ERROR;
#if defined(SSE_DEBUG)
//        BRK(odd exception vector); // GEN4-OVR
        M68000.nExceptions++;
        TRACE_LOG("->%d bombs\n",bombs);
#endif
        address=ad;
        action=EA_FETCH;
      }
      else
      {
        pc=ad;//390
        TRACE_LOG("PC = %X\n\n",ad);
        CPU_ABUS_ACCESS_READ_FETCH; // np
        INSTRUCTION_TIME(2); //n (-> nn)
        CPU_ABUS_ACCESS_READ_FETCH; //np
        SET_PC(ad);
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_TRACE_BIT);
#else
        SR_CLEAR(SR_TRACE);
#endif
      /////////!  INSTRUCTION_TIME_ROUND(22);  // 8+4+22=36: OK
        interrupt_depth++; // Is this necessary?
      }
    }
    if(bombs==BOMBS_BUS_ERROR||bombs==BOMBS_ADDRESS_ERROR)
    {
/*
YACHT
Address error       | 50(4/7)  |     nn ns nS ns ns ns nS ns nV nv np np      
Bus error           | 50(4/7)  |     nn ns nS ns ns ns nS ns nV nv np np   
WinUAE:             | 50(4/7)  |     nn ns nS ns ns ns nS ns nV nv np n  np
ST:                 | 52(4/7)  |     nn ns nS ns ns ns nS ns nV nv np n+ np 
*/
#if defined(SSE_CPU_BUS_ERROR_TIMING)
/*  BUSERRT1.TOS
    The GLU keeps trying to access the address for a while
    before asserting bus error.
*/
      if(bombs==BOMBS_BUS_ERROR)
        INSTRUCTION_TIME(70); // timing on STE
#endif
      INSTRUCTION_TIME(4); //nn
      if(!SUPERFLAG) 
        change_to_supervisor_mode();
#pragma warning(disable: 4611) //390
      TRY_M68K_EXCEPTION
#pragma warning(default: 4611)
      {

#if defined(SSE_CPU_DETECT_STACK_PC_USE)
        _pc=0x123456; // push garbage!
#elif defined(SSE_CPU_TRUE_PC) 
/*  Legit fix for Aladin
    In bus errors, CLR.W usually crashes on 'read' but not in that
    special ST memory zone where only writing crashes, so it's a logic
    'exception' in our 'true pc' system for bus error stack frame.
    We test here, not before, for performance.
*/
        if(M68000.CheckRead && abus<MEM_FIRST_WRITEABLE) 
          M68000.Pc=pc+2;
        if(_pc!=M68000.Pc)
        {
          TRACE_LOG("pc %X true PC %X\n",_pc,M68000.Pc);
          _pc=M68000.Pc; // guaranteed exact...
        }
#endif
        TRACE_LOG("Push PC %X on %X\n",_pc | pc_high_byte,r[15]-4);
        CPU_ABUS_ACCESS_WRITE_PUSH_L; // ns nS
        m68k_PUSH_L(_pc| pc_high_byte);
        TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
        CPU_ABUS_ACCESS_WRITE_PUSH; // ns
        m68k_PUSH_W(_sr);
        TRACE_LOG("Push IR %X on %X\n",_ir,r[15]-2);
        CPU_ABUS_ACCESS_WRITE_PUSH; // ns
        m68k_PUSH_W(_ir);
        TRACE_LOG("Push crash address %X->%X on %X\n",address,(address & 0x00ffffff) | pc_high_byte,r[15]-4);
        CPU_ABUS_ACCESS_WRITE_PUSH_L; // ns nS
        m68k_PUSH_L((address & 0x00ffffff) | pc_high_byte); 
        // status
        // strangely, status word is based on the opcode
        WORD x=WORD(_ir & 0xffe0); 
        if(action!=EA_WRITE) x|=B6_010000;
        if(action==EA_FETCH)
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_PROGRAM:FC_USER_PROGRAM);
        else
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_DATA:FC_USER_DATA);
        TRACE_LOG("Push status %X on %X\n",x,r[15]-2);
        CPU_ABUS_ACCESS_WRITE_PUSH; // ns
        m68k_PUSH_W(x);
      }
      CATCH_M68K_EXCEPTION
      {
        TRACE_LOG("Exception during exception...\n");
#if defined(SSE_CPU_HALT)
        perform_crash_and_burn(); //wait for cases...
#else//390
        r[15]=0xf000; // R15=A7  // should be halt?
#endif
      }
      END_M68K_EXCEPTION
      abus=LPEEK(bombs*4);//390
      TRACE_LOG("PC = %X\n\n",abus);
#if defined(SSE_BOILER_SHOW_INTERRUPT)
      Debug.RecordInterrupt("BOMBS",bombs);
#endif

      CPU_ABUS_ACCESS_READ_L; //nV nv
      CPU_ABUS_ACCESS_READ_FETCH; // np
      INSTRUCTION_TIME(2); //n (-> nn)
      SET_PC(abus);
      CPU_ABUS_ACCESS_READ_FETCH; //np
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_TRACE_BIT);
#else
      SR_CLEAR(SR_TRACE);
#endif
    }
    DEBUG_ONLY(log_history(bombs,crash_address));
  }
#if defined(SSE_CPU_HALT)
  if(M68000.ProcessingState!=TM68000::HALTED
    &&M68000.ProcessingState!=TM68000::INTEL_CRASH
    &&M68000.ProcessingState!=TM68000::BOILER_MESSAGE )
    M68000.ProcessingState=TM68000::NORMAL;
#endif

  PeekEvent(); // Stop exception freeze
}

#undef LOGSECTION


BYTE m68k_fetchB()
{
  WORD ret;
  FETCH_W(ret)
  return LOBYTE(ret);
}

WORD m68k_fetchW()
{
  WORD ret;
  FETCH_W(ret)
  return ret;
}

LONG m68k_fetchL()
{
  LONG ret;
  FETCH_W(*LPHIWORD(ret));
  FETCH_W(*LPLOWORD(ret));
  return ret;
}

void m68k_unrecognised() //SS trap1 in Motorola doc
{
  exception(BOMBS_ILLEGAL_INSTRUCTION,EA_INST,0);
}

#if defined(DEBUG_BUILD)

void DebugCheckIOAccess() {
#if defined(SSE_BOILER_390)
  if (ioaccess & IOACCESS_DEBUG_MEM_WRITE_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(d2_peek(debug_mem_write_log_address)):int(d2_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Write to address $"+HEXSl(debug_mem_write_log_address,6)+ \
                  ", new value is "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
    val=d2_peek(debug_mem_write_log_address);\
    int val2=debug_mem_write_log_address&1?0:d2_dpeek(debug_mem_write_log_address);\
    int val3=debug_mem_write_log_address&1?0:d2_lpeek(debug_mem_write_log_address);\
    TRACE("PC %X %s write %X|%X|%X to %X\n",old_pc,disa_d2(old_pc).Text,  \
      val,val2,val3,debug_mem_write_log_address);\
    ioaccess|=IOACCESS_DEBUG_MEM_WRITE_LOG;\
  }  else if (ioaccess & IOACCESS_DEBUG_MEM_READ_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(d2_peek(debug_mem_write_log_address)):int(d2_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Read from address $"+HEXSl(debug_mem_write_log_address,6)+ \
      ", = "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
    val=d2_peek(debug_mem_write_log_address);\
    int val2=debug_mem_write_log_address&1?0:d2_dpeek(debug_mem_write_log_address);\
    int val3=debug_mem_write_log_address&1?0:d2_lpeek(debug_mem_write_log_address);\
    TRACE("PC %X %s read %X|%X|%X from %X\n",old_pc,disa_d2(old_pc).Text,\
      val,val2,val3,debug_mem_write_log_address);\
    ioaccess|=IOACCESS_DEBUG_MEM_READ_LOG;\
  } 

#elif defined(SSE_BOILER_MONITOR_TRACE)
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
#else
  if (ioaccess & IOACCESS_DEBUG_MEM_WRITE_LOG){ \
    int val=int((debug_mem_write_log_bytes==1) ? int(m68k_peek(debug_mem_write_log_address)):int(m68k_dpeek(debug_mem_write_log_address))); \
    log_write(HEXSl(old_pc,6)+": Write to address $"+HEXSl(debug_mem_write_log_address,6)+ \
                  ", new value is "+val+" ($"+HEXSl(val,debug_mem_write_log_bytes*2)+")"); \
  }
#endif
}
#endif


#if defined(SSE_TIMING_NO_IO_W_DELAY)
/*  This is intermediate refactoring.
    Steem sets the destination of a CPU operation using a pointer.
    If the destination is in the io zone, the pointer will be updated
    in good time, but the write to io itself is delayed until the end
    of the instruction. This is not really correct.
    Not all instructions use this system. Notably not MOVE and the like.
    We eliminate the delay at the price of more code inside CPU instructions,
    using macros.
*/

inline void check_io_write_b() {
  if(ioaccess&1)
    io_write_b(ioad,LOBYTE(iobuffer)); 
}

inline void check_io_write_w() {
  if(ioaccess&2)
    io_write_w(ioad,LOWORD(iobuffer)); 
}

inline void check_io_write_l() {
  if(ioaccess&4)
    io_write_l(ioad,iobuffer);
}

#define CHECK_IOW_B check_io_write_b()
#define CHECK_IOW_W check_io_write_w()
#define CHECK_IOW_L check_io_write_l()
#else
#define CHECK_IOW_B
#define CHECK_IOW_W
#define CHECK_IOW_L
#endif

inline void handle_ioaccess() {
  if (ioaccess){                             
#if defined(SSE_TIMING_NO_IO_W_DELAY)
    ASSERT(!(ioaccess&7) || (ioaccess&0xfffffff8));
#endif
    switch (ioaccess & IOACCESS_NUMBER_MASK){    
#if !defined(SSE_TIMING_NO_IO_W_DELAY___)
      case 1: io_write_b(ioad,LOBYTE(iobuffer)); 
        break;    
      case 2: io_write_w(ioad,LOWORD(iobuffer)); 
        break;    
      case 4: io_write_l(ioad,iobuffer); 
        break;      
#endif
      case TRACE_BIT_JUST_SET: 
#if !defined(SSE_CPU_TRACE_REFACTOR)
        m68k_trace();
#endif
        break;                                        
    }                                             
    if (ioaccess & IOACCESS_FLAG_DELAY_MFP){ 
      ioaccess&=~IOACCESS_FLAG_DELAY_MFP;  
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; 
    }else if (ioaccess & IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE){ 
      ioaccess|=IOACCESS_FLAG_DELAY_MFP;  
    } 
    if (ioaccess & IOACCESS_INTERCEPT_OS2){ 
      ioaccess&=~IOACCESS_INTERCEPT_OS2;  
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; 
    }else if (ioaccess & IOACCESS_INTERCEPT_OS){ 
      ioaccess|=IOACCESS_INTERCEPT_OS2; 
    } 
    if ((ioaccess & IOACCESS_FLAG_FOR_CHECK_INTRS)){   
      check_for_interrupts_pending();          
      CHECK_STOP_USER_MODE_NO_INTR 
    }                                             
    DEBUG_CHECK_IOACCESS; 
#if !defined(SSE_BLT_BUS_ARBITRATION)
    if (ioaccess & IOACCESS_FLAG_DO_BLIT) 
      Blitter_Start_Now(); 
#endif
    // These flags stay until the next instruction to stop interrupts
    ioaccess=ioaccess & (IOACCESS_FLAG_DELAY_MFP | IOACCESS_INTERCEPT_OS2
#if defined(SSE_BLT_BUS_ARBITRATION)
      |IOACCESS_FLAG_DO_BLIT // keep it
#endif
    );                                   
  }
}

#define HANDLE_IOACCESS(tracefunc) handle_ioaccess();



#define LOGSECTION LOGSECTION_CPU

// a big function now
void m68kProcess() {

#if defined(SSE_DEBUG)
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
  M68000.IrAddress=pc;
  M68000.PreviousIr=IRD;
  M68000.nInstr++;
#endif//debug

#if defined(SSE_CPU_TRACE_REFACTOR)
  if(M68000.tpend)
    M68000.tpend=false;
#if defined(SSE_VC_INTRINSICS_382) && defined(SSE_VC_INTRINSICS_390A)
  else if BITTEST(sr,0xf)
#else
  else if(sr&SR_TRACE)
#endif
  {
    M68000.tpend=true; // hardware latch (=flag)
#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL)
    if(OSD_MASK_CPU & OSD_CONTROL_CPUTRACE) 
      TRACE_OSD("TRACE %X",pc);
#endif
  }
#endif

  old_pc=pc;  
//  ASSERT(old_pc!=0xc374);

#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK_CPU & OSD_CONTROL_CPUIO) 
    if(pc>=MEM_IO_BASE)
      TRACE_OSD("PC %X",pc);
#endif

/*  basic prefetch rule:
Just before the start of any instruction two words (no more and no less) are 
already fetched. One word will be in IRD and another one in IRC.
*/
  ASSERT(prefetched_2); 

  PREFETCH_CLASS(0);// default, most instructions

  FetchWord(IRD); // IR->IRD (using reference)
  // TODO should already be there?
  // TODO assert illegal here, but perf?

#if defined(SSE_CPU_EXCEPTION_TRACE_PC)//heavy!!!
  if(nExceptions>1) 
    TRACE_LOG("%X\n",pc);
#endif

  pc+=2; // in fact it was already set in previous instruction

#if defined(SSE_CPU_TRUE_PC)
  M68000.Pc=pc; // anyway
  M68000.CheckRead=0;
#endif

#if defined(SSE_CPU_E_CLOCK)
  M68000.EClock_synced=false; // one more bool in Process()!
#endif

  /////////// CALL CPU EMU FUNCTION ///////////////
//  ASSERT(ir!=0x8178);
//  ASSERT(ir!=0xe1d0);//bitshift
  //ASSERT(ir!=0xd750);//add
//  ASSERT(ir!=0x46fc);
//  ASSERT(ir!=0x4af9);
  //ASSERT(ir!=0x4e90);
  //ASSERT(!(ioaccess&7));
  m68k_high_nibble_jump_table[ir>>12]();

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

  if(M68000.tpend)
  {
#ifdef DEBUG_BUILD
    if(!Debug.logsection_enabled[LOGSECTION_CPU] && !logsection_enabled[LOGSECTION_CPU])
      TRACE_LOG("(T) PC %X SR %X VEC %X IR %04X: %s\n",old_pc,sr,LPEEK(0x24),ir,disa_d2(old_pc).Text);
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("TRACE");
#endif
#else
    TRACE_LOG("TRACE PC %X IR %X SR %X $24 %X\n",pc,ir,sr,LPEEK(0x24));
#endif

#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::EXCEPTION;
#endif

    m68kTrapTiming();
    m68k_interrupt(LPEEK(BOMBS_TRACE_EXCEPTION*4));

#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::NORMAL;
#endif

  }
#undef LOGSECTION
#define LOGSECTION LOGSECTION_CPU 
#endif//SSE_CPU_TRACE_REFACTOR

#if defined(SSE_CPU_TRACE_REFACTOR)
  // this won't care for trace
  HANDLE_IOACCESS( ; );
#else
  // this will execute the next instruction then trigger trace interrupt
  HANDLE_IOACCESS( m68k_trace(); ); // keep as macro, wouldn't be inlined
#endif
  DEBUG_ONLY( debug_first_instruction=0 );
}


void m68kSetPC(MEM_ADDRESS ad) {
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  if(ad==rom_addr)
    Debug.InterruptIdx=0;
#endif
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
#if defined(SSE_CPU_FETCH_80000)
#ifdef SSE_BUGFIX_392
      else //392
#endif
      if(pc>=0x80000 && pc<0x3FFFFF)
      {
        lpfetch=lpDPEEK(xbios2); 
        lpfetch_bound=lpDPEEK(mem_len+(MEM_EXTRA_BYTES/2)); 
      }
#endif
#if defined(SSE_MMU_MONSTER_ALT_RAM)
      else if(ad<MMU.MonSTerHimem)
      {
        lpfetch=lpDPEEK(pc); 
        lpfetch_bound=lpDPEEK(mem_len+(MEM_EXTRA_BYTES/2));
      }
#endif
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
#if defined(SSE_CPU_CHECK_PC)
  PC=ad;
#endif    
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackCheck(ad) ;
#endif
  m68kPrefetchSetPC();
}


#undef LOGSECTION
#define LOGSECTION LOGSECTION_TRACE

#if !defined(SSE_CPU_TRACE_REFACTOR)

#ifdef COMPILER_VC6
extern "C" void ASMCALL m68k_trace() //execute instruction with trace bit set
#else
extern "C" ASMCALL void m68k_trace() //execute instruction with trace bit set
#endif
{
  ASSERT( sr&SR_TRACE );
#ifdef DEBUG_BUILD
  pc_history[pc_history_idx++]=pc;
  if (pc_history_idx>=HISTORY_SIZE) pc_history_idx=0;
  EasyStr Dissasembly=disa_d2(pc);
  dbg_log(EasyStr("TRACE: ")+HEXSl(pc,6)+" - "+Dissasembly);
#endif
#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK_CPU & OSD_CONTROL_CPUTRACE) 
    TRACE_OSD("TRACE");
#endif

  LOG_CPU

  old_pc=pc;

  DEBUG_ONLY( if (debug_num_bk) breakpoint_check(); )

#if defined(SSE_CPU)
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
#if !defined(SSE_CPU_TRACE_LINE_A_F)
  m68k_do_trace_exception=true;
#endif
#if defined(SSE_CPU) && defined(SSE_DEBUG)
  M68000.PreviousIr=ir;
  M68000.nInstr++;
#endif
  PREFETCH_CLASS(0);

  m68k_high_nibble_jump_table[ir>>12](); //SS call in trace

#if !defined(SSE_CPU_TRACE_LINE_A_F) 
  if (m68k_do_trace_exception)
#endif
  {
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
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("TRACE");
#endif
//    TRACE_LOG("SR=%X\n",sr);
    m68kTrapTiming();
    m68k_interrupt(LPEEK(BOMBS_TRACE_EXCEPTION*4));
  }
  ioaccess|=store_ioaccess;
  // In case of IOACCESS_FLAG_FOR_CHECK_INTRS interrupt must happen after trace
  HANDLE_IOACCESS(;)
}
#endif//#if defined(SSE_CPU_TRACE_REFACTOR)
#undef LOGSECTION

#include "cpu_ea.cpp"

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


#if defined(SSE_VC_INTRINSICS_390A) //apparently it performs worse?

inline bool m68k_condition_test_t(){return true;}
inline bool m68k_condition_test_f(){return false;}
inline bool m68k_condition_test_hi(){return ((sr&(SR_C+SR_Z))==0);}
inline bool m68k_condition_test_ls(){return (bool)((sr&(SR_C+SR_Z)));}
inline bool m68k_condition_test_cc(){return (BITTEST(sr,SR_C_BIT)==0);}
inline bool m68k_condition_test_cs(){return (BITTEST(sr,SR_C_BIT));}
inline bool m68k_condition_test_ne(){return (BITTEST(sr,SR_Z_BIT)==0);}
inline bool m68k_condition_test_eq(){return (BITTEST(sr,SR_Z_BIT));}
inline bool m68k_condition_test_vc(){return (BITTEST(sr,SR_V_BIT)==0);}
inline bool m68k_condition_test_vs(){return (BITTEST(sr,SR_V_BIT));}
inline bool m68k_condition_test_pl(){return (BITTEST(sr,SR_N_BIT)==0);}
inline bool m68k_condition_test_mi(){return (BITTEST(sr,SR_N_BIT));}
inline bool m68k_condition_test_ge(){return ((sr&(SR_N+SR_V))==0 || (sr&(SR_N+SR_V))==(SR_N+SR_V));}
inline bool m68k_condition_test_lt(){return ((sr&(SR_N+SR_V))==SR_V || (sr&(SR_N+SR_V))==SR_N);}
inline bool m68k_condition_test_gt(){return ((BITTEST(sr,SR_Z_BIT)==0) && ( ((sr&(SR_N+SR_V))==0) || ((sr&(SR_N+SR_V))==SR_N+SR_V) ));}
inline bool m68k_condition_test_le(){return ((BITTEST(sr,SR_Z_BIT)) || ( ((sr&(SR_N+SR_V))==SR_N) || ((sr&(SR_N+SR_V))==SR_V) ));}

#else

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

#endif


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
-------------------------------------------------------------------------------
 ORI, ANDI, EORI  |    Exec Time    |               Data Bus Usage
  to CCR, to SR   |      INSTR      | 1st Operand |          INSTR
------------------+-----------------+-------------+----------------------------
#<data>,CCR       |                 |             |               
  .B :            | 20(3/0)         |          np |              nn nn np np   

ijor:
Writing to SR is, internally, quite different than writing to a regular register.
The most important difference is that the prefetch must be flushed and filled again.
This is because the instruction might have changed mode from SUPER to USER 
(other way around is not possible, it would trigger a privilege violation). 
And some systems use a different memory map for each mode. 
Then the prefetch must be filled again, now with the new mode. Yes, just in case.

Not only that, it can't refill the prefetch until the write to SR and the possible
mode change has been completed internally. So they can't overlap.

The first np fetched the opcode for the next instruction or whatever was at that 
location, which in this case is completely wasted because it will be flushed. 
And the wastefull bus cycle is because the instruction just reuses the same microcode 
as every other immediate instruction for that part.
*/

void                              m68k_ori_b(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  if ((ir & B6_111111)==B6_111100){  //to CCR
    BYTE immediate=m68k_IMMEDIATE_B;
    INSTRUCTION_TIME(8); // nn nn
    CCR|=immediate;
    REFETCH_IR; //np
    PREFETCH_IRC; //np
    sr&=SR_VALID_BITMASK;
    pc+=2; 
  }else{
    m68k_GET_IMMEDIATE_B;
    m68k_GET_DEST_B_NOT_A; // EA
    PREFETCH_IRC; // np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1) //for when PREFETCH_IRC modifies abus (not the case now)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; // nw
    }
    m68k_DEST_B|=m68k_src_b;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_B
    CHECK_IOW_B;
  }
}


void                              m68k_ori_w(){
  if ((ir & B6_111111)==B6_111100){  //to sr
    if (SUPERFLAG){
      WORD immediate=m68k_IMMEDIATE_W;
      CPU_ABUS_ACCESS_READ_FETCH; //np
      INSTRUCTION_TIME(8);//nn nn 
      sr|=immediate;
      REFETCH_IR; //np
      PREFETCH_IRC; //np
      sr&=SR_VALID_BITMASK;
      pc+=2; 
      DETECT_TRACE_BIT;
    }else{
      exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
    }
  }else{
    CPU_ABUS_ACCESS_READ_FETCH; //np
    m68k_GET_IMMEDIATE_W;
    m68k_GET_DEST_W_NOT_A; // EA
    PREFETCH_IRC; //np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_W|=m68k_src_w;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_W
    CHECK_IOW_W;
  }
}

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

void                              m68k_ori_l(){
  CPU_ABUS_ACCESS_READ_FETCH_L; // np np
  m68k_GET_IMMEDIATE_L;
  m68k_GET_DEST_L_NOT_A; // EA
  PREFETCH_IRC; // np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4); // nn
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nw nW
  }
  m68k_DEST_L|=m68k_src_l;
  SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
  SR_CHECK_Z_AND_N_L
  CHECK_IOW_L;
}


void                              m68k_andi_b(){
  CPU_ABUS_ACCESS_READ_FETCH; // np
  if((ir&B6_111111)==B6_111100){  //to CCR
    BYTE immediate=m68k_IMMEDIATE_B;
    INSTRUCTION_TIME(8); // nn nn
    CCR&=immediate;
    REFETCH_IR; //np
    PREFETCH_IRC; //np
    pc+=2; 
  }else{
    m68k_GET_IMMEDIATE_B;
#if defined(SSE_CPU_DATABUS)
    dbus|=m68k_src_b;//?
#endif
    m68k_GET_DEST_B_NOT_A; //EA
    PREFETCH_IRC;//np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_B&=m68k_src_b;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_B
    CHECK_IOW_B;
  }
}


void                              m68k_andi_w(){
  if((ir&B6_111111)==B6_111100){  //to sr
    if(SUPERFLAG){
      DEBUG_ONLY( int debug_old_sr=sr; )
      CPU_ABUS_ACCESS_READ_FETCH; //np
      WORD immediate=m68k_IMMEDIATE_W;
      INSTRUCTION_TIME(8); //nn nn
      sr&=immediate;
      REFETCH_IR;//np
      PREFETCH_IRC;//np
      DETECT_CHANGE_TO_USER_MODE;
      pc+=2; 
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
      CHECK_STOP_ON_USER_CHANGE
    }
    else 
      exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }else{
    CPU_ABUS_ACCESS_READ_FETCH; //np
    m68k_GET_IMMEDIATE_W; 
    m68k_GET_DEST_W_NOT_A;//EA
#if defined(SSE_CPU_DATABUS)
    dbus=m68k_src_w;
#endif
    PREFETCH_IRC;//np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE;//nw
    }
    m68k_DEST_W&=m68k_src_w;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_W
    CHECK_IOW_W;
  }
}


void                              m68k_andi_l(){
  CPU_ABUS_ACCESS_READ_FETCH_L; // np np
  m68k_GET_IMMEDIATE_L;
#if defined(SSE_CPU_DATABUS)
  dbus=m68k_src_l&0xFFFF;
#endif
  m68k_GET_DEST_L_NOT_A; //EA
  PREFETCH_IRC; //np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4); //nn nn
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; //nw nW
  }
  m68k_DEST_L&=m68k_src_l;
  SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
  SR_CHECK_Z_AND_N_L
  CHECK_IOW_L;
}


void                              m68k_subi_b(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_B;
  m68k_GET_DEST_B_NOT_A;//EA
  PREFETCH_IRC;//np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE;//nw
  }
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(SR_X);
  CHECK_IOW_B;
}


void                              m68k_subi_w(){
  CPU_ABUS_ACCESS_READ_FETCH;//np
  m68k_GET_IMMEDIATE_W;
  m68k_GET_DEST_W_NOT_A; // EA
  PREFETCH_IRC; //np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
  }
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(SR_X);
  CHECK_IOW_W;
}


void                              m68k_subi_l(){
  CPU_ABUS_ACCESS_READ_FETCH_L; //np np
  m68k_GET_IMMEDIATE_L;
  m68k_GET_DEST_L_NOT_A; // EA
  PREFETCH_IRC;//np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4);  //nn
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nw nW
  }
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(SR_X);
  CHECK_IOW_L;
}


void                              m68k_addi_b(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_B;
  m68k_GET_DEST_B_NOT_A; // EA
  PREFETCH_IRC;//np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
  }
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B+=m68k_src_b;
  SR_ADD_B;
  CHECK_IOW_B;
}


void                              m68k_addi_w(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_W;
  m68k_GET_DEST_W_NOT_A; //EA
  PREFETCH_IRC; // np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
  }
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W+=m68k_src_w;
  SR_ADD_W;
  CHECK_IOW_W;
}


void                              m68k_addi_l(){
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
  CPU_ABUS_ACCESS_READ_FETCH_L; // np np
  m68k_GET_IMMEDIATE_L;
  m68k_GET_DEST_L_NOT_A; //EA
  PREFETCH_IRC; //np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4); //nn
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; //nw nW
  }
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L+=m68k_src_l;
  SR_ADD_L;
  CHECK_IOW_L;
}


void                              m68k_btst(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_B;
  if ((ir&BITS_543)==BITS_543_000){
/*
#<data>,Dn :      |                 |             |               |
  .L :            | 10(2/0)  0(0/0) |          np |               | np n     
*/
    PREFETCH_IRC; //np
    INSTRUCTION_TIME(2); //n
    m68k_src_b&=31;
    if((r[PARAM_M]>>m68k_src_b)&1){
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_Z_BIT);
#else
      SR_CLEAR(SR_Z);
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
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
    m68k_ap=(short)(m68k_src_b & 7);
    if((ir&(BIT_5+BIT_4+BIT_3+BIT_2+BIT_1+BIT_0))==B6_111100){  //immediate mode is the only one not allowed -
      m68k_unrecognised();
    }else{
      m68k_GET_SOURCE_B_NOT_A; //EA
      PREFETCH_IRC; //np
      if((m68k_src_b>>m68k_ap)&1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
    }
  }
}


void                              m68k_bchg(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_B;

  if((ir&BITS_543)==BITS_543_000){ //SS on DN
/*
#<data>,Dn :      |                 |             |               |
  .L :            |                 |             |               |
    if data<16    | 10(2/0)  0(0/0) |          np |               | np       n  
    if data>15    | 12(2/0)  0(0/0) |          np |               | np       nn 
*/
    PREFETCH_IRC;  //np
    m68k_src_b&=31;
    if (m68k_src_b>15)
      INSTRUCTION_TIME(4); //nn
    else
      INSTRUCTION_TIME(2); //n
    m68k_src_l=1<<m68k_src_b;
    if(r[PARAM_M]&m68k_src_l){
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_Z_BIT);
#else
      SR_CLEAR(SR_Z);
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
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
    m68k_src_b&=7;
    m68k_GET_DEST_B_NOT_A; // EA
    PREFETCH_IRC; // np
    m68k_src_b=(BYTE)(1<<m68k_src_b);
    if(m68k_DEST_B&m68k_src_b){
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_Z_BIT);
#else
      SR_CLEAR(SR_Z);
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
    }
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_B^=(BYTE)m68k_src_b;
    CHECK_IOW_B;
  }
}


void                              m68k_bclr(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_B;
  if((ir&BITS_543)==BITS_543_000){
/*
#<data>,Dn :      |                 |             |               |
  .L :            |                 |             |               |
    if data<16    | 12(2/0)  0(0/0) |          np |               | np nn       
    if data>15    | 14(2/0)  0(0/0) |          np |               | np nn n     
*/
    PREFETCH_IRC; //np
    m68k_src_b&=31;
    if (m68k_src_b>15)
      INSTRUCTION_TIME(6); //nn n
    else
      INSTRUCTION_TIME(4); //nn
    m68k_src_l=1<<m68k_src_b;
    if(r[PARAM_M]&m68k_src_l){
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_Z_BIT);
#else
      SR_CLEAR(SR_Z);
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
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
    m68k_src_b&=7;
    m68k_GET_DEST_B_NOT_A; // EA
    PREFETCH_IRC; //np
    m68k_src_b=(BYTE)(1<<m68k_src_b);
    if(m68k_DEST_B&m68k_src_b){
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_Z_BIT);
#else
      SR_CLEAR(SR_Z);
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
    }
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_B&=(BYTE)(~m68k_src_b);
    CHECK_IOW_B;
  }
}


void                              m68k_bset(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_B;
  if ((ir&BITS_543)==BITS_543_000){ //SS to Dn
/*
#<data>,Dn :      |                 |             |               |
  .L :            |                 |             |               |
    if data<16    | 10(2/0)  0(0/0) |          np |               | np       n  
    if data>15    | 12(2/0)  0(0/0) |          np |               | np       nn 
*/
    PREFETCH_IRC; //np
    m68k_src_b&=31;
    if (m68k_src_b>15)
      INSTRUCTION_TIME(4); // nn
    else
      INSTRUCTION_TIME(2); // n
    m68k_src_l=1 << m68k_src_b;
    if (r[PARAM_M] & m68k_src_l){
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_Z_BIT);
#else
      SR_CLEAR(SR_Z);
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
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
    m68k_src_b&=7;
    m68k_GET_DEST_B_NOT_A; // EA
    PREFETCH_IRC; //np
    m68k_src_b=(BYTE)(1<<m68k_src_b);
    if(m68k_DEST_B&m68k_src_b){
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_Z_BIT);
#else
      SR_CLEAR(SR_Z);
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
    }
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_B|=(BYTE)m68k_src_b;
    CHECK_IOW_B;
  }
}


void                              m68k_eori_b(){
/*
-------------------------------------------------------------------------------
 ORI, ANDI, EORI  |    Exec Time    |               Data Bus Usage
  to CCR, to SR   |      INSTR      | 1st Operand |          INSTR
------------------+-----------------+-------------+----------------------------
#<data>,CCR       |                 |             |               
  .B :            | 20(3/0)         |          np |              nn nn np np    
*/
  CPU_ABUS_ACCESS_READ_FETCH; //np
  if((ir&B6_111111)==B6_111100){  //to CCR
    BYTE immediate=m68k_IMMEDIATE_B; 
    INSTRUCTION_TIME(8); //nn nn
    CCR^=immediate;
    sr&=SR_VALID_BITMASK;
    REFETCH_IR; //np
    PREFETCH_IRC; //np
    pc+=2; 
  }else{
/*
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
*/
    m68k_GET_IMMEDIATE_B;
    m68k_GET_DEST_B_NOT_A; //EA
    PREFETCH_IRC; //np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_B^=m68k_src_b;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_B
    CHECK_IOW_B;
  }
}


void                              m68k_eori_w(){
  if((ir&B6_111111)==B6_111100){  //to sr
    if(SUPERFLAG){
      DEBUG_ONLY( int debug_old_sr=sr; )
      CPU_ABUS_ACCESS_READ_FETCH; // np
      WORD immediate=m68k_IMMEDIATE_W; //timing counted there
      INSTRUCTION_TIME(8); // nn nn
      sr^=immediate;
      REFETCH_IR; // np
      PREFETCH_IRC; // np
      sr&=SR_VALID_BITMASK;
      pc+=2; 
      DETECT_CHANGE_TO_USER_MODE
      DETECT_TRACE_BIT;
      // Interrupts must come after trace exception
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
      CHECK_STOP_ON_USER_CHANGE;
    }
    else 
      exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
  else{
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
*/
    CPU_ABUS_ACCESS_READ_FETCH; //np
    m68k_GET_IMMEDIATE_W;
    m68k_GET_DEST_W_NOT_A; //EA
    PREFETCH_IRC; //np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_W^=m68k_src_w;
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    SR_CHECK_Z_AND_N_W;
    CHECK_IOW_W;
  }
}


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
  CPU_ABUS_ACCESS_READ_FETCH_L; //np np
  m68k_GET_IMMEDIATE_L;
  m68k_GET_DEST_L_NOT_A; //EA
  PREFETCH_IRC; //np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(4); //nn
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; //nw nW
  }
  m68k_DEST_L^=m68k_src_l;
  SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
  SR_CHECK_Z_AND_N_L;
  CHECK_IOW_L;
}

#if defined(SSE_CPU_SIMPLIFY_READ_DEST_CMPI)

void                              m68k_cmpi_b(){
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
  CPU_ABUS_ACCESS_READ_FETCH; // np
  m68k_GET_IMMEDIATE_B;
  m68k_GET_DEST_B_NOT_A;
  m68k_old_dest=m68k_DEST_B;
  //m68k_old_dest=m68k_read_dest_b(); //EA
  PREFETCH_IRC; // np
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(0);
}


void                              m68k_cmpi_w(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_W; 
  m68k_GET_DEST_W_NOT_A;
  m68k_old_dest=m68k_DEST_W;
  //m68k_old_dest=m68k_read_dest_w(); //EA
  PREFETCH_IRC; //np
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(0);
}


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
  CPU_ABUS_ACCESS_READ_FETCH_L; // np np
  m68k_GET_IMMEDIATE_L;
  m68k_GET_DEST_L_NOT_A;
  m68k_old_dest=m68k_DEST_L;
  //m68k_old_dest=m68k_read_dest_l(); //EA
  PREFETCH_IRC; // np
  if(DEST_IS_REGISTER)
    INSTRUCTION_TIME(2); // n
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
}

#else

void                              m68k_cmpi_b(){
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
  CPU_ABUS_ACCESS_READ_FETCH; // np
  m68k_GET_IMMEDIATE_B;
  m68k_old_dest=m68k_read_dest_b(); //EA
  PREFETCH_IRC; // np
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(0);
  CHECK_IOW_B;
}


void                              m68k_cmpi_w(){
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_W; 
  m68k_old_dest=m68k_read_dest_w(); //EA
  PREFETCH_IRC; //np
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(0);
  CHECK_IOW_W;
}


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
  CPU_ABUS_ACCESS_READ_FETCH_L; // np np
  m68k_GET_IMMEDIATE_L;
  m68k_old_dest=m68k_read_dest_l(); //EA
  PREFETCH_IRC; // np
  if(DEST_IS_REGISTER)
    INSTRUCTION_TIME(2); // n
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
  CHECK_IOW_L;
}

#endif//#if defined(SSE_CPU_SIMPLIFY_READ_DEST_CMPI)

void                              m68k_movep_w_to_dN_or_btst(){
  if((ir&BITS_543)==BITS_543_001){ //MOVEP.W 
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
      MOVEP       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
(d16,Ay),Dx :     |                 |
  .W :            | 16(4/0)         |                np    nR    nr np          
*/
    CPU_ABUS_ACCESS_READ_FETCH; //np
    MEM_ADDRESS addr=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=addr;
#endif
    CPU_ABUS_ACCESS_READ; //nR
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_B(abus);
#else
    m68k_READ_B(addr);
#endif
    DWORD_B_1(&r[PARAM_N])=m68k_src_b; //high byte
#if defined(SSE_MMU_ROUNDING_BUS)
    abus+=2;
#elif defined(SSE_MMU_ROUNDING_BUS)
    abus=addr+2;
#endif
    CPU_ABUS_ACCESS_READ; //nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_B(abus);
#else
    m68k_READ_B(addr+2);
#endif
    DWORD_B_0(&r[PARAM_N])=m68k_src_b; //low byte
    PREFETCH_IRC; //np
  }else{ // BTST
    if ((ir&BITS_543)==BITS_543_000){  //btst to data register
/*
Dn,Dm :           |                 |             |               |
  .L :            |  6(1/0)  0(0/0) |             |               | np n    
*/
      PREFETCH_IRC; //np
      INSTRUCTION_TIME(2); //n //390
      if ((r[PARAM_M] >> (31 & r[PARAM_N])) & 1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
    }else{ // btst memory
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
      CPU_ABUS_ACCESS_READ_FETCH; //np //390
      m68k_GET_SOURCE_B_NOT_A; //EA  //even immediate mode is allowed!!!!
      PREFETCH_IRC; //np
      if( (m68k_src_b >> (7 & r[PARAM_N])) & 1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
    }
  }
}


void                              m68k_movep_l_to_dN_or_bchg(){
  if((ir&BITS_543)==BITS_543_001){ //MOVEP.L
//  .L :            | 24(6/0)         |                np nR nR nr nr np          
    PREFETCH_CLASS(1);
    CPU_ABUS_ACCESS_READ_FETCH; // np
    MEM_ADDRESS addr=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=addr;
#endif
    CPU_ABUS_ACCESS_READ; // nR
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_B(abus);
#else
    m68k_READ_B(addr)//ss problem with putting timing there, sometimes it shouldn't be there
#endif
    DWORD_B_3(&r[PARAM_N])=m68k_src_b;
#if defined(SSE_MMU_ROUNDING_BUS)
    abus+=2;
#endif
    CPU_ABUS_ACCESS_READ; // nR
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_B(abus);
#else
    m68k_READ_B(addr+2)
#endif
    DWORD_B_2(&r[PARAM_N])=m68k_src_b;
#if defined(SSE_MMU_ROUNDING_BUS)
    abus+=2;
#endif
    CPU_ABUS_ACCESS_READ; // nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_B(abus);
#else
    m68k_READ_B(addr+4)
#endif
    DWORD_B_1(&r[PARAM_N])=m68k_src_b;
#if defined(SSE_MMU_ROUNDING_BUS)
    abus+=2;
#endif
    CPU_ABUS_ACCESS_READ; // nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_B(abus);
#else
    m68k_READ_B(addr+6)
#endif
    DWORD_B_0(&r[PARAM_N])=m68k_src_b;
    PREFETCH_IRC; //np
  }else{ // bchg
    if((ir&BITS_543)==BITS_543_000){ // register SS:data
/*
Dn,Dm :           |                 |             |               |
  .L :            |                 |             |               |
    if Dn<16      |  6(1/0)  0(0/0) |             |               | np       n  
    if Dn>15      |  8(1/0)  0(0/0) |             |               | np       nn 
*/
      m68k_src_w=BYTE(LOBYTE(r[PARAM_N]) & 31);
      PREFETCH_IRC; //np
      if (m68k_src_w>15)
        INSTRUCTION_TIME(4); //nn
      else
        INSTRUCTION_TIME(2); //n
      if((r[PARAM_M]>>(m68k_src_w))&1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
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
      m68k_GET_DEST_B_NOT_A; // EA
      PREFETCH_IRC; //np
      if((m68k_DEST_B>>(7&r[PARAM_N]))&1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
      m68k_DEST_B^=(signed char)(1<<(7&r[PARAM_N]));
      CHECK_IOW_B;
    }
  }
}


void                              m68k_movep_w_from_dN_or_bclr(){
  if ((ir & BITS_543)==BITS_543_001){    // MOVEP.W
/*
Dx,(d16,Ay) :     |                 |
  .W :            | 16(2/2)         |                np    nW    nw np          
*/
    PREFETCH_CLASS(1);
    CPU_ABUS_ACCESS_READ_FETCH; //np
    abus=areg[PARAM_M]+(short)m68k_fetchW();
    pc+=2; 
    CPU_ABUS_ACCESS_WRITE; // nW
    m68k_poke(abus,DWORD_B_1(&r[PARAM_N]));
    abus+=2;
    CPU_ABUS_ACCESS_WRITE; // nw
    m68k_poke(abus,DWORD_B_0(&r[PARAM_N]));
    PREFETCH_IRC; // np
  }else{ //bclr
    if((ir&BITS_543)==BITS_543_000){
/*
Dn,Dm :           |                 |             |               |
  .L :            |                 |             |               |
    if Dn<16      |  8(1/0)  0(0/0) |             |               | np nn       
    if Dn>15      | 10(1/0)  0(0/0) |             |               | np nn n     
*/
      PREFETCH_IRC; //np
      m68k_src_w=BYTE(LOBYTE(r[PARAM_N]) & 31);
      if (m68k_src_w>=16)
        INSTRUCTION_TIME(6); //nn n
      else
        INSTRUCTION_TIME(4); //n
      if((r[PARAM_M]>>(m68k_src_w))&1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
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
      m68k_GET_DEST_B_NOT_A; // EA
      PREFETCH_IRC; //np
      if((m68k_DEST_B>>(7&r[PARAM_N]))&1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
      m68k_DEST_B&=(signed char)~(1<<(7&r[PARAM_N]));
      CHECK_IOW_B;
    }
  }
}


void                              m68k_movep_l_from_dN_or_bset(){
  //SS there's a TODO for blitter, maybe need a case
  if ((ir&BITS_543)==BITS_543_001){  // MOVEP.L
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
    PREFETCH_CLASS(1);
    CPU_ABUS_ACCESS_READ_FETCH; //np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 
    BYTE *p=(BYTE*)(&r[PARAM_N]);
    CPU_ABUS_ACCESS_WRITE; // nW
    m68k_poke(abus,DWORD_B_3(p));
    abus+=2;
    CPU_ABUS_ACCESS_WRITE; // nW
    m68k_poke(abus,DWORD_B_2(p));
    abus+=2;
    CPU_ABUS_ACCESS_WRITE; // nw
    m68k_poke(abus,DWORD_B_1(p));
    abus+=2;
    CPU_ABUS_ACCESS_WRITE; // nw
    m68k_poke(abus,DWORD_B_0(p));
    PREFETCH_IRC;  //np
  }else{ // BSET
    if((ir&BITS_543)==BITS_543_000){
/*
Dn,Dm :           |                 |             |               |
  .L :            |                 |             |               |
    if Dn<16      |  6(1/0)  0(0/0) |             |               | np       n  
    if Dn>15      |  8(1/0)  0(0/0) |             |               | np       nn 
*/
      PREFETCH_IRC; //np
      m68k_src_w=BYTE(LOBYTE(r[PARAM_N]) & 31);
      if (m68k_src_w>15)
        INSTRUCTION_TIME(4); //nn
      else
        INSTRUCTION_TIME(2); //n
      if((r[PARAM_M]>>(m68k_src_w))&1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
      r[PARAM_M]|=(1<<m68k_src_w);
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
      m68k_GET_DEST_B_NOT_A; // EA
      PREFETCH_IRC; //np
      if((m68k_DEST_B>>(7&r[PARAM_N]))&1){
#if defined(SSE_VC_INTRINSICS_390E)
        BITRESET(sr,SR_Z_BIT);
#else
        SR_CLEAR(SR_Z);
#endif
      }else{
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_Z_BIT);
#else
        SR_SET(SR_Z);
#endif
      }
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
      m68k_DEST_B|=(signed char)(1<<(7&r[PARAM_N]));
      CHECK_IOW_B;
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
*/

void                              m68k_negx_b(){
/*
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
  CHECK_READ=true; 
  m68k_GET_DEST_B_NOT_A; //EA
  PREFETCH_IRC; //np
  if(!DEST_IS_DATA_REGISTER) 
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw //390
  }
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B=(BYTE)-m68k_DEST_B;
  if(sr&SR_X)m68k_DEST_B--;
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(m68k_DEST_B)
    BITRESET(sr,SR_Z_BIT);
  if(m68k_old_dest&m68k_DEST_B&MSB_B)
    BITSET(sr,SR_V_BIT); 
  if((m68k_old_dest|m68k_DEST_B)&MSB_B)
    SR_SET(SR_C+SR_X);
  if(m68k_DEST_B & MSB_B)
    BITSET(sr,SR_N_BIT); 
#else
  if(m68k_DEST_B)SR_CLEAR(SR_Z);
  if(m68k_old_dest&m68k_DEST_B&MSB_B)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_B)&MSB_B)SR_SET(SR_C+SR_X);
  if(m68k_DEST_B & MSB_B)SR_SET(SR_N);
#endif
  CHECK_IOW_B;
}


void                             m68k_negx_w(){
/*
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
  CHECK_READ=true;
  m68k_GET_DEST_W_NOT_A; //EA
  PREFETCH_IRC; //np
  if(!DEST_IS_DATA_REGISTER) 
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw 390
  }
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W=(WORD)-m68k_DEST_W;
  if(sr&SR_X)m68k_DEST_W--;
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(m68k_DEST_W)
    BITRESET(sr,SR_Z_BIT);
  if(m68k_old_dest&m68k_DEST_W&MSB_W)
    BITSET(sr,SR_V_BIT); 
  if((m68k_old_dest|m68k_DEST_W)&MSB_W)
    SR_SET(SR_C+SR_X);
  if(m68k_DEST_W & MSB_W)
    BITSET(sr,SR_N_BIT); 
#else
  if(m68k_DEST_W)SR_CLEAR(SR_Z);
  if(m68k_old_dest&m68k_DEST_W&MSB_W)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_W)&MSB_W)SR_SET(SR_C+SR_X);
  if(m68k_DEST_W & MSB_W)SR_SET(SR_N);
#endif
  CHECK_IOW_W;
}

void                             m68k_negx_l(){
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
  CHECK_READ=true;
  m68k_GET_DEST_L_NOT_A; //EA
  PREFETCH_IRC; //np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2); //n
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; //nw nW
  }
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L=-m68k_DEST_L;
  if(sr&SR_X)m68k_DEST_L-=1;
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(m68k_DEST_L)
    BITRESET(sr,SR_Z_BIT);
  if(m68k_old_dest&m68k_DEST_L&MSB_L)
    BITSET(sr,SR_V_BIT);
  if((m68k_old_dest|m68k_DEST_L)&MSB_L)SR_SET(SR_C+SR_X);
  if(m68k_DEST_L & MSB_L)
    BITSET(sr,SR_N_BIT);
#else
  if(m68k_DEST_L)SR_CLEAR(SR_Z);
  if(m68k_old_dest&m68k_DEST_L&MSB_L)SR_SET(SR_V);
  if((m68k_old_dest|m68k_DEST_L)&MSB_L)SR_SET(SR_C+SR_X);
  if(m68k_DEST_L & MSB_L)SR_SET(SR_N);
#endif
  CHECK_IOW_L;
}


void                              m68k_clr_b(){
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
  CHECK_READ=true; 
  m68k_GET_DEST_B_NOT_A; // EA
  PREFETCH_IRC; // np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_DEST_B=0;
#if defined(SSE_BOILER_MONITOR_VALUE3)
  if (DEST_IS_REGISTER==0)
    DEBUG_CHECK_WRITE_B(abus);
#endif
  SR_CLEAR(SR_N+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  BITSET(sr,SR_Z_BIT);
#else
  SR_SET(SR_Z);
#endif
  CHECK_IOW_B;
}


void                             m68k_clr_w(){
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
  CHECK_READ=true;
  m68k_GET_DEST_W_NOT_A; //EA
  PREFETCH_IRC; // np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_DEST_W=0;
#if defined(SSE_BOILER_MONITOR_VALUE3)
  if (DEST_IS_REGISTER==0)
    DEBUG_CHECK_WRITE_W(abus);
#endif
  SR_CLEAR(SR_N+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  BITSET(sr,SR_Z_BIT);
#else
  SR_SET(SR_Z);
#endif
  CHECK_IOW_W;
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
  CHECK_READ=true;
  m68k_GET_DEST_L_NOT_A; // EA
  PREFETCH_IRC; // np
#if defined(SSE_CPU_DATABUS)
  dbus=pc+2;
#endif
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2); // n
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nw nW
  }
  m68k_DEST_L=0;
#if defined(SSE_BOILER_MONITOR_VALUE3)
  if (DEST_IS_REGISTER==0)
    DEBUG_CHECK_WRITE_L(abus);
#endif
  SR_CLEAR(SR_N+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  BITSET(sr,SR_Z_BIT);
#else
  SR_SET(SR_Z);
#endif
  CHECK_IOW_L;
}


void                              m68k_neg_b(){
/*
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
  CHECK_READ=true;
  m68k_GET_DEST_B_NOT_A; // EA
  PREFETCH_IRC; // np
  if(DEST_IS_REGISTER==0)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B=(BYTE)-m68k_DEST_B;
  SR_CLEAR(SR_USER_BYTE);
#if defined(SSE_VC_INTRINSICS_390E)
  if(m68k_old_dest&m68k_DEST_B&MSB_B)
    BITSET(sr,SR_V_BIT);
#else
  if(m68k_old_dest&m68k_DEST_B&MSB_B)SR_SET(SR_V);
#endif
  if((m68k_old_dest|m68k_DEST_B)&MSB_B)SR_SET(SR_C+SR_X);
  SR_CHECK_Z_AND_N_B;
  CHECK_IOW_B;
}


void                             m68k_neg_w(){
/*
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
  CHECK_READ=true;
  m68k_GET_DEST_W_NOT_A; // EA
  PREFETCH_IRC; // np
  if(DEST_IS_REGISTER==0)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W=(WORD)-m68k_DEST_W;
  SR_CLEAR(SR_USER_BYTE);
#if defined(SSE_VC_INTRINSICS_390E)
  if(m68k_old_dest&m68k_DEST_W&MSB_W)
    BITSET(sr,SR_V_BIT);
#else
  if(m68k_old_dest&m68k_DEST_W&MSB_W)SR_SET(SR_V);
#endif
  if((m68k_old_dest|m68k_DEST_W)&MSB_W)SR_SET(SR_C+SR_X);
  SR_CHECK_Z_AND_N_W;
  CHECK_IOW_W;
}


void                             m68k_neg_l(){
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
  CHECK_READ=true;
  m68k_GET_DEST_L_NOT_A; // EA
  PREFETCH_IRC; // np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2); // n
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nw nW
  }
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L=-m68k_DEST_L;
  SR_CLEAR(SR_USER_BYTE);
#if defined(SSE_VC_INTRINSICS_390E)
  if(m68k_old_dest&m68k_DEST_L&MSB_L)
    BITSET(sr,SR_V_BIT);
#else
  if(m68k_old_dest&m68k_DEST_L&MSB_L)SR_SET(SR_V);
#endif
  if((m68k_old_dest|m68k_DEST_L)&MSB_L)SR_SET(SR_C+SR_X);
  SR_CHECK_Z_AND_N_L;
  CHECK_IOW_L;
}

void                              m68k_not_b(){
/*
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
  CHECK_READ=true;
  m68k_GET_DEST_B_NOT_A; // EA
  PREFETCH_IRC; // np
  if(DEST_IS_REGISTER==0)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_DEST_B=(BYTE)~m68k_DEST_B;
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  SR_CHECK_Z_AND_N_B;
  CHECK_IOW_B;
}


void                             m68k_not_w(){
/*
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
  CHECK_READ=true;
  m68k_GET_DEST_W_NOT_A; // EA
  PREFETCH_IRC; // np
  if(DEST_IS_REGISTER==0)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_DEST_W=(WORD)~m68k_DEST_W;
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  SR_CHECK_Z_AND_N_W;
  CHECK_IOW_W;
}


void                             m68k_not_l(){
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
  CHECK_READ=true;
  m68k_GET_DEST_L_NOT_A; // EA
  PREFETCH_IRC; // np
  if(DEST_IS_DATA_REGISTER)
    INSTRUCTION_TIME(2); // n
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nw nW
  }
  m68k_DEST_L=~m68k_DEST_L;
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
  SR_CHECK_Z_AND_N_L;
  CHECK_IOW_L;
}

#if defined(SSE_CPU_SIMPLIFY_READ_DEST_TST)

/*
-------------------------------------------------------------------------------
	                |     Exec Time   |               Data Bus Usage             
       TST        |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
*/

void                              m68k_tst_b(){
/*
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  4(1/0)  4(1/0) |            nr |               np          
    (An)+         |  4(1/0)  4(1/0) |            nr |               np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np          
*/
  //MEM_ADDRESS save_pc=pc;
  
 // BYTE x=m68k_read_dest_b(); // EA
  //MEM_ADDRESS save_abus=abus;
  //pc=save_pc;
  //m68k_GET_DEST_B_NOT_A;
  //BYTE y=m68k_DEST_B;
  BYTE x = 0;
  switch(ir&BITS_543) {
    //case BITS_543_000:
    //case BITS_543_001:
    case BITS_543_010:
    //case BITS_543_011:
      x=m68k_read_dest_b();
      break;
    default:
      m68k_GET_DEST_B_NOT_A;
      x=m68k_DEST_B;
      break;
  }

  /*if(x!=y) {
    TRACE("abus %X peek %X\n",abus,m68k_peek(abus));
    TRACE("save_abus %X peek %X\n",save_abus,m68k_peek(save_abus));
  }*/

  //ASSERT(x==y);

  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(!x)
    BITSET(sr,SR_Z_BIT);
  if(x&MSB_B)
    BITSET(sr,SR_N_BIT);
#else
  if(!x)SR_SET(SR_Z);
  if(x&MSB_B)SR_SET(SR_N);
#endif
  PREFETCH_IRC; // np
}


void                             m68k_tst_w(){
/*
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  4(1/0)  4(1/0) |            nr |               np          
    (An)+         |  4(1/0)  4(1/0) |            nr |               np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np          
*/
  //WORD x=m68k_read_dest_w(); // EA
  m68k_GET_DEST_W_NOT_A;
  WORD x=m68k_DEST_W;

  //WORD y=m68k_read_dest_w(); // EA
  //ASSERT(x==y); //if fetching...

  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(!x)
    BITSET(sr,SR_Z_BIT);
  if(x&MSB_W)
    BITSET(sr,SR_N_BIT);
#else
  if(!x)SR_SET(SR_Z);
  if(x&MSB_W)SR_SET(SR_N);
#endif
  PREFETCH_IRC; // np
}


void                             m68k_tst_l(){
/*
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
  //LONG x=m68k_read_dest_l(); // EA
  m68k_GET_DEST_L_NOT_A;
  LONG x=m68k_DEST_L;
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(!x)
    BITSET(sr,SR_Z_BIT);
  if(x&MSB_L)
    BITSET(sr,SR_N_BIT);
#else
  if(!x)SR_SET(SR_Z);
  if(x&MSB_L)SR_SET(SR_N);
#endif
  PREFETCH_IRC; // np
}


#else

/*
-------------------------------------------------------------------------------
	                |     Exec Time   |               Data Bus Usage             
       TST        |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
*/

void                              m68k_tst_b(){
/*
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  4(1/0)  4(1/0) |            nr |               np          
    (An)+         |  4(1/0)  4(1/0) |            nr |               np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np          
*/
  BYTE x=m68k_read_dest_b(); // EA
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(!x)
    BITSET(sr,SR_Z_BIT);
  if(x&MSB_B)
    BITSET(sr,SR_N_BIT);
#else
  if(!x)SR_SET(SR_Z);
  if(x&MSB_B)SR_SET(SR_N);
#endif
  PREFETCH_IRC; // np
}


void                             m68k_tst_w(){
/*
  .B or .W :      |                 |               | 
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  4(1/0)  4(1/0) |            nr |               np          
    (An)+         |  4(1/0)  4(1/0) |            nr |               np          
    -(An)         |  4(1/0)  6(1/0) | n          nr |               np          
    (d16,An)      |  4(1/0)  8(2/0) |      np    nr |               np          
    (d8,An,Xn)    |  4(1/0) 10(2/0) | n    np    nr |               np          
    (xxx).W       |  4(1/0)  8(2/0) |      np    nr |               np          
    (xxx).L       |  4(1/0) 12(3/0) |   np np    nr |               np          
*/
  WORD x=m68k_read_dest_w(); // EA
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(!x)
    BITSET(sr,SR_Z_BIT);
  if(x&MSB_W)
    BITSET(sr,SR_N_BIT);
#else
  if(!x)SR_SET(SR_Z);
  if(x&MSB_W)SR_SET(SR_N);
#endif
  PREFETCH_IRC; // np
}


void                             m68k_tst_l(){
/*
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
  LONG x=m68k_read_dest_l(); // EA
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(!x)
    BITSET(sr,SR_Z_BIT);
  if(x&MSB_L)
    BITSET(sr,SR_N_BIT);
#else
  if(!x)SR_SET(SR_Z);
  if(x&MSB_L)SR_SET(SR_N);
#endif
  PREFETCH_IRC; // np
}

#endif//#if defined(SSE_CPU_SIMPLIFY_READ_DEST_TST)

void                              m68k_tas(){
  if((ir&B6_111111)==B6_111100){
    ASSERT(ir==0x4afc); // it is tas_or_illegal()
    ILLEGAL;
  }else{
/*
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
*/
    CHECK_READ=true;
#if defined(SSE_BLT_392) // no blitter during read-modify-write bus cycle
    BYTE save=Blit.Request;
    Blit.Request=0;
#endif
    m68k_GET_DEST_B_NOT_A; // EA
    if(DEST_IS_REGISTER==0) {
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_WRITE; // nw
    } 
    SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
    m68k_DEST_B|=MSB_B;
    CHECK_IOW_B;//Good example where it could be important
#if defined(SSE_BLT_392)
    Blit.Request=save;
#endif
    PREFETCH_IRC; // np
  }
}


void                              m68k_move_from_sr(){
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
  CHECK_READ=true;
  m68k_GET_DEST_W_NOT_A; //EA
  PREFETCH_IRC; // np
  if (DEST_IS_REGISTER)
    INSTRUCTION_TIME(2); // n
  else
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_DEST_W=sr;
  CHECK_IOW_W; // move SR to io zone? why not!
}


void                              m68k_move_from_ccr(){
  ILLEGAL; // Stupid Motorola!
}


/*
-------------------------------------------------------------------------------
       MOVE       |    Exec Time    |               Data Bus Usage
  to CCR, to SR   |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
*/

void                              m68k_move_to_ccr(){
  if((ir&BITS_543)==BITS_543_001){
    m68k_unrecognised();
  }else{
/*
  .B :            |                 |               |
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
    m68k_GET_SOURCE_W; // EA
    INSTRUCTION_TIME(4); // nn
    REFETCH_IR; // np
    PREFETCH_IRC; // np
    CCR=LOBYTE(m68k_src_w);
    sr&=SR_VALID_BITMASK;
  }
}


void                              m68k_move_to_sr(){
  if(SUPERFLAG){
    if((ir&BITS_543)==BITS_543_001){ //address register
      m68k_unrecognised();
    }else{
/*
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
      DEBUG_ONLY( int debug_old_sr=sr; )
      //ASSERT(!(ioaccess&7));
      m68k_GET_SOURCE_W; //EA
      INSTRUCTION_TIME(4); // nn
      sr=m68k_src_w;
      sr&=SR_VALID_BITMASK;
      REFETCH_IR; // np
      PREFETCH_IRC; // np
      //ASSERT(!(ioaccess&7));
      DETECT_CHANGE_TO_USER_MODE;
      //ASSERT(!(ioaccess&7));
      DETECT_TRACE_BIT;
      //ASSERT(!(ioaccess&7));
      // Interrupts must come after trace exception
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
      CHECK_STOP_ON_USER_CHANGE;
    }
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}


void                              m68k_nbcd(){
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
  CHECK_READ=true;
  m68k_GET_DEST_B_NOT_A; // EA
  PREFETCH_IRC; // np
  if (DEST_IS_REGISTER)
    INSTRUCTION_TIME(2); // n
  else 
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  int m=m68k_DEST_B,n=0;
  if(m&0xff) n=0xa0;
  if(m&0xf)n=0x9a;
  if(sr&SR_X)n=0x99;
  SR_CLEAR(SR_X+SR_C);
  if(m)SR_SET(SR_X+SR_C); //there will be a carry
  m68k_DEST_B=(BYTE)(n-m);
#if defined(SSE_VC_INTRINSICS_390E)
  if(m68k_DEST_B) 
    BITRESET(sr,SR_Z_BIT);
#else
  if(m68k_DEST_B){SR_CLEAR(SR_Z);}
#endif
  CHECK_IOW_B;
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
    r[PARAM_M]=MAKELONG(HIWORD(r[PARAM_M]),LOWORD(r[PARAM_M]));
    SR_CLEAR(SR_N+SR_Z+SR_V+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
    if(!r[PARAM_M])
      BITSET(sr,SR_Z_BIT);
    if(r[PARAM_M]&MSB_L)
      BITSET(sr,SR_N_BIT);
#else
    if(!r[PARAM_M])SR_SET(SR_Z);
    if(r[PARAM_M]&MSB_L)SR_SET(SR_N);
#endif
    PREFETCH_IRC; // np
  }else{ //PEA
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
    case BITS_543_010: // (An)
      effective_address=areg[PARAM_M];
      break;

    case BITS_543_101: // (d16,An)
      CPU_ABUS_ACCESS_READ_FETCH; // np
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_543_110: // (d8,An,Xn)
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_READ_FETCH; // np
      m68k_iriwo=m68k_fetchW();pc+=2; 
      INSTRUCTION_TIME(2); //n
      if (m68k_iriwo & BIT_b){  //.l
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;

    case BITS_543_111:
      switch (ir & 0x7){

      case 0: // (xxx).W
        PREFETCH_CLASS(1);
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
        TRUE_PC+=2;
        break;

      case 1: // (xxx).L
        PREFETCH_CLASS(1);
        CPU_ABUS_ACCESS_READ_FETCH; // np
        effective_address=m68k_fetchL();
        pc+=4;  
        TRUE_PC+=4;
        break;

      case 2: // (d16,PC) 
        CPU_ABUS_ACCESS_READ_FETCH; // np
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;

      case 3: // (d8,PC,Xn)
        INSTRUCTION_TIME(2); // n
        CPU_ABUS_ACCESS_READ_FETCH; // np
        m68k_iriwo=m68k_fetchW();
        INSTRUCTION_TIME(2); // n
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
    }//sw
//390
    if(PREFETCH_CLASS_1) //(xxx).W (xxx).L   np nS ns np 
      CPU_ABUS_ACCESS_READ_FETCH;  //np
    else //np nS ns
      PREFETCH_IRC; //np
    CPU_ABUS_ACCESS_WRITE_PUSH_L; // nS ns
    m68k_PUSH_L(effective_address); 
    if(PREFETCH_CLASS_1)
      PREFETCH_IRC; //np
  }//pea
}

void                              m68k_movem_w_from_regs_or_ext_w(){
  if((ir&BITS_543)==BITS_543_000){ 
    // EXT.W
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        EXT       |      INSTR      |  1st Operand  |          INSTR
------------------+-----------------+---------------+--------------------------
Dn :              |                 |               |
  .W :            |  4(1/0)         |               |               np          
*/
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
    m68k_dest=&(r[PARAM_M]);
    m68k_DEST_W=(signed short)((signed char)LOBYTE(r[PARAM_M]));
    SR_CHECK_Z_AND_N_W;
    PREFETCH_IRC; // np
  }
  else if ((ir & BITS_543)==BITS_543_100){ //predecrement 
  // MOVEM R->M -(An)
  //-(An)         |  8+4m(2/m)      |                np (nw)*       np    
    PREFETCH_CLASS(1);
    CPU_ABUS_ACCESS_READ_FETCH; // np
    m68k_src_w=m68k_fetchW();pc+=2; 
    abus=areg[PARAM_M];
    DWORD areg_hi=(abus & 0xff000000);
#if defined(SSE_BLT_BUS_ARBITRATION)
#if defined(SSE_VC_INTRINSICS_390F)
    short bit=0;
#else
    short mask=1;
#endif
#else
#if defined(SSE_VC_INTRINSICS_390F)
    short bit=0,BlitterStart=0;
#else
    short mask=1,BlitterStart=0;
#endif
#endif
    for (int n=0;n<16;n++){
#if defined(SSE_VC_INTRINSICS_390F)
      if(BITTEST(m68k_src_w,bit)) {
#else
      if (m68k_src_w & mask){
#endif
        abus-=2;
        CPU_ABUS_ACCESS_WRITE; // (nw)*
        m68k_dpoke(abus,LOWORD(r[15-n]));
#if !defined(SSE_BLT_BUS_ARBITRATION)
/*  Notice that this was potentially buggy in blit mode, though I know
    no case: we execute other instructions then we come back here?
*/
        if (ioaccess & IOACCESS_FLAG_DO_BLIT){
          // After word that starts blitter must write one more word, then blit
          if ((++BlitterStart)==2){
            Blitter_Start_Now();
            BlitterStart=0;
          }
        }
#endif
      }
#if defined(SSE_VC_INTRINSICS_390F)
      bit++;
#else
      mask<<=1;
#endif
    }
    // The register written to memory should be the original one, so
    // predecrement afterwards.
    areg[PARAM_M]=abus | areg_hi;
    PREFETCH_IRC; // np
  }else{ //SS MOVEM other cases
/*
  .W              |                 | 
    (An)          |  8+4m(2/m)      |                np (nw)*       np          
    (d16,An)      | 12+4m(3/m)      |             np np (nw)*       np          
    (d8,An,Xn)    | 14+4m(3/m)      |          np n  np (nw)*       np          
    (xxx).W       | 12+4m(3/m)      |             np np (nw)*       np          
    (xxx).L       | 16+4m(4/m)      |          np np np (nw)*       np        
*/
    PREFETCH_CLASS(1);

    // check illegal before EA
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
    CPU_ABUS_ACCESS_READ_FETCH; //np
    m68k_src_w=m68k_fetchW();pc+=2;  // registers

    switch (ir & BITS_543){

    case BITS_543_010: // (An)
      abus=areg[PARAM_M];
      break;

    case BITS_543_101: // (d16,An)
      CPU_ABUS_ACCESS_READ_FETCH; // np
      abus=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_543_110: 
      // (d8,An,Xn)    | 14+4m(3/m)      |          np n  np (nw)*       np
      m68k_iriwo=m68k_fetchW();pc+=2; 
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_READ_FETCH; // np
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;

    case BITS_543_111:
      switch(ir&0x7){
      case 0:
        CPU_ABUS_ACCESS_READ_FETCH; // np
        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case 1:
        CPU_ABUS_ACCESS_READ_FETCH_L; // np np
        abus=0xffffff & m68k_fetchL();
        pc+=4;  
        break;
#if defined(SSE_DEBUG)
      default:
        ASSERT(0);
#endif
      }
      break;
#if defined(SSE_DEBUG)
    default:
      ASSERT(0);
#endif
    }
#if defined(SSE_BLT_BUS_ARBITRATION)
#if defined(SSE_VC_INTRINSICS_390F)
    short bit=0;
#else
    short mask=1;
#endif
#else
#if defined(SSE_VC_INTRINSICS_390F)
    short bit=0,BlitterStart=0;
#else
    short mask=1,BlitterStart=0;
#endif
#endif
    for (int n=0;n<16;n++){
#if defined(SSE_VC_INTRINSICS_390F)
      if(BITTEST(m68k_src_w,bit)) {
#else
      if (m68k_src_w & mask){
#endif
        CPU_ABUS_ACCESS_WRITE; // (nw)*
        m68k_dpoke(abus,LOWORD(r[n]));
        abus+=2;
#if !defined(SSE_BLT_BUS_ARBITRATION)
        if (ioaccess & IOACCESS_FLAG_DO_BLIT){
          // After word that starts blitter must write one more word, then blit
          if ((++BlitterStart)==2){
            Blitter_Start_Now();
            BlitterStart=0;
          }
        }
#endif
      }
#if defined(SSE_VC_INTRINSICS_390F)
      bit++;
#else
      mask<<=1;
#endif
    }
    PREFETCH_IRC; // np
  }
}


void                              m68k_movem_l_from_regs_or_ext_l(){
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
    m68k_DEST_L=(signed long)((signed short)LOWORD(r[PARAM_M]));
    SR_CHECK_Z_AND_N_L;
    PREFETCH_IRC; // np
  }
  else if((ir&BITS_543)==BITS_543_100) // MOVEM -(An)
  {
    //SS this is used to save registers on the stack
/*

-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
      MOVEM       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
R --> M           |                 | 
  .L              |                 |                             
    -(An)         |  8+8m(2/2m)     |                np (nw nW)*    np   
*/
    PREFETCH_CLASS(1);
    CPU_ABUS_ACCESS_READ_FETCH;
    m68k_src_w=m68k_fetchW();pc+=2;
    abus=areg[PARAM_M];
    DWORD areg_hi=(areg[PARAM_M] & 0xff000000);
    TRUE_PC=pc+2;
#if defined(SSE_VC_INTRINSICS_390F)
    short bit=0;
#else
    short mask=1;
#endif
    for (int n=0;n<16;n++){
#if defined(SSE_VC_INTRINSICS_390F)
      if(BITTEST(m68k_src_w,bit)) {
#else
      if (m68k_src_w & mask){
#endif
        abus-=4;
        CPU_ABUS_ACCESS_WRITE_L; // (nw nW)*
        m68k_lpoke(abus,r[15-n]);
#if !defined(SSE_BLT_BUS_ARBITRATION)
        if (ioaccess & IOACCESS_FLAG_DO_BLIT) 
          Blitter_Start_Now();
#endif
      }
#if defined(SSE_VC_INTRINSICS_390F)
      bit++;
#else
      mask<<=1;
#endif
    }
    // The register written to memory should be the original one, so
    // predecrement afterwards.
    areg[PARAM_M]=abus | areg_hi;
    PREFETCH_IRC;//np
  }// movem .l -(an)
  else{ //SS MOVEM other cases
/*
R --> M           |                 | 
  .L              |                 |                             
    (An)          |  8+8m(2/2m)     |                np (nW nw)*    np          
    (d16,An)      | 12+8m(3/2m)     |             np np (nW nw)*    np          
    (d8,An,Xn)    | 14+8m(3/2m)     |        n    np np (nW nw)*    np          
    (xxx).W       | 12+8m(3/2m)     |             np np (nW nw)*    np          
    (xxx).L       | 16+8m(4/2m)     |          np np np (nW nw)*    np      
*/
    PREFETCH_CLASS(1);
    // check illegal
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
    //note because of (d8,An,Xn) we can't count prefetch timing here
    m68k_src_w=m68k_fetchW();pc+=2; // register mask

    switch(ir&BITS_543){
    case BITS_543_010: 
      // (An)          |  8+8m(2/2m)     |                np (nW nw)*    np
      CPU_ABUS_ACCESS_READ_FETCH; //np
      abus=areg[PARAM_M];
      break;

    case BITS_543_101: 
      // (d16,An)      | 12+8m(3/2m)     |             np np (nW nw)*    np     
      CPU_ABUS_ACCESS_READ_FETCH_L; // np np
      abus=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_543_110: 
      // (d8,An,Xn)    | 14+8m(3/2m)     |        n    np np (nW nw)*    np 
      INSTRUCTION_TIME(2); // n 
      CPU_ABUS_ACCESS_READ_FETCH_L; // np np
      m68k_ap=m68k_fetchW();pc+=2; 
      if(m68k_ap&BIT_b){  //.l
        abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_ap)+(int)r[m68k_ap>>12];
      }else{         //.w
        abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_ap)+(signed short)r[m68k_ap>>12];
      }
      break;

    case BITS_543_111:
      switch(ir&0x7){

      case 0:
        //(xxx).W       | 12+8m(3/2m)     |             np np (nW nw)*    np
        CPU_ABUS_ACCESS_READ_FETCH_L; // np np
        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;

      case 1:
        //(xxx).L       | 16+8m(4/2m)     |          np np np (nW nw)*    np
        CPU_ABUS_ACCESS_READ_FETCH; // np
        CPU_ABUS_ACCESS_READ_FETCH_L; // np np
        abus=0xffffff&m68k_fetchL();
        pc+=4;  
        break;

      default: //ILLEGAL
        ASSERT(0);
      }
      break;

    default:
      ASSERT(0);
    }
    TRUE_PC=pc+2; // Blood Money original
#if defined(SSE_VC_INTRINSICS_390F)
    short bit=0;
#else
    short mask=1;
#endif
    for (int n=0;n<16;n++){
#if defined(SSE_VC_INTRINSICS_390F)
      if(BITTEST(m68k_src_w,bit)) {
#else
      if (m68k_src_w&mask){
#endif
        CPU_ABUS_ACCESS_WRITE_L; //(nW nw)*
        m68k_lpoke(abus,r[n]);
        abus+=4;
      }
#if defined(SSE_VC_INTRINSICS_390F)
      bit++;
#else
      mask<<=1;
#endif
    }//nxt
    PREFETCH_IRC; // np
  }//movem regs->mem
}


void                              m68k_movem_l_to_regs(){
  // check illegal
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
/*
  .L              |                 | 
    (An)          | 12+8m(3+2m/0)   |                np nR (nr nR)* np          
    (An)+         | 12+8m(3+2m/0)   |                np nR (nr nR)* np          
    (d16,An)      | 16+8m(4+2m/0)   |             np np nR (nr nR)* np          
    (d8,An,Xn)    | 18+8m(4+2m/0)   |          np n  np nR (nr nR)* np          
    (xxx).W       | 16+8m(4+2m/0)   |             np np nR (nr nR)* np          
    (xxx).L       | 20+8m(5+2m/0)   |          np np np nR (nr nR)* np          
*/
  bool postincrement=false;
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_src_w=m68k_fetchW();pc+=2; 

  switch(ir&BITS_543){
  case BITS_543_011: //(An)+  
    postincrement=true;
    //no break
  case BITS_543_010: //(An)
    abus=areg[PARAM_M];
    break;

  case BITS_543_101://(d16,An)
    CPU_ABUS_ACCESS_READ_FETCH; //np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 
    break;

  case BITS_543_110:
    //(d8,An,Xn)    | 18+8m(4+2m/0)   |          np n  np nR (nr nR)* np
    INSTRUCTION_TIME(2); //n //390
    CPU_ABUS_ACCESS_READ_FETCH; // np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    break;

  case BITS_543_111:
    switch(ir&0x7){
    case 0:
      //(xxx).W       | 16+8m(4+2m/0)   |             np np nR (nr nR)* np          
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      break;

    case 1:
      //(xxx).L       | 20+8m(5+2m/0)   |          np np np nR (nr nR)* np    
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      abus=0xffffff&m68k_fetchL();
      pc+=4;  
      break;

    case 2:
      //(d16,An)      | 16+8m(4+2m/0)   |             np np nR (nr nR)* np          
      CPU_ABUS_ACCESS_READ_FETCH; //np
      abus=pc+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case 3:
      //(d8,An,Xn)    | 18+8m(4+2m/0)   |          np n  np nR (nr nR)* np         
      INSTRUCTION_TIME(2); //n //390
      CPU_ABUS_ACCESS_READ_FETCH;//np
      m68k_iriwo=m68k_fetchW();
      if(m68k_iriwo&BIT_b){  //.l
        abus=pc+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=pc+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      pc+=2; 
      break;

#if defined(SSE_DEBUG)
    default:
      ASSERT(0);
#endif
    }
    break;
#if defined(SSE_DEBUG)
  default:
    ASSERT(0);
#endif
  }
  CPU_ABUS_ACCESS_READ; //nR 390
  m68k_dpeek(abus); //extra word read (discarded) 390
  TRUE_PC=pc+2;
  DWORD areg_hi=(areg[PARAM_M] & 0xff000000);
#if defined(SSE_VC_INTRINSICS_390F)
  short bit=0;
#else
  short mask=1;
#endif
  for (int n=0;n<16;n++){
#if defined(SSE_VC_INTRINSICS_390F)
    if(BITTEST(m68k_src_w,bit)) {
#else
    if (m68k_src_w & mask){
#endif  
      CPU_ABUS_ACCESS_READ_L; //(nr nR)* //SS isn't nR nr more logical?
      r[n]=m68k_lpeek(abus);
      abus+=4;
    }
#if defined(SSE_VC_INTRINSICS_390F)
    bit++;
#else
    mask<<=1;
#endif
  }
  if (postincrement) 
    areg[PARAM_M]=abus | areg_hi;
  PREFETCH_IRC;
}


void                              m68k_movem_w_to_regs(){
    // check illegal
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
/*
  .W              |                 | 
    (An)          | 12+4m(3+m/0)    |                np (nr)*    nr np          
    (An)+         | 12+4m(3+m/0)    |                np (nr)*    nr np          
    (d16,An)      | 16+4m(4+m/0)    |             np np (nr)*    nr np          
    (d8,An,Xn)    | 18+4m(4+m/0)    |          np n  np (nr)*    nr np          
    (xxx).W       | 16+4m(4+m/0)    |             np np (nr)*    nr np          
    (xxx).L       | 20+4m(5+m/0)    |          np np np (nr)*    nr np  
*/
  bool postincrement=false;
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_src_w=m68k_fetchW();pc+=2; 

  switch (ir & BITS_543){

  case BITS_543_011://(An)+         | 12+4m(3+m/0)    |                np (nr)*    nr np
    postincrement=true;
    //no break
  case BITS_543_010://(An)          | 12+4m(3+m/0)    |                np (nr)*    nr np
    abus=areg[PARAM_M];
    break;

  case BITS_543_101://(d16,An)      | 16+4m(4+m/0)    |             np np (nr)*    nr np
    CPU_ABUS_ACCESS_READ_FETCH; //np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();
    pc+=2; 
    break;

  case BITS_543_110://(d8,An,Xn)    | 18+4m(4+m/0)    |          np n  np (nr)*    nr np 
    INSTRUCTION_TIME(2);//n //390
    CPU_ABUS_ACCESS_READ_FETCH; //np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    break;

  case BITS_543_111:
    switch(ir&0x7){
    case 0://(xxx).W       | 16+4m(4+m/0)    |             np np (nr)*    nr np
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      break;

    case 1://(xxx).L       | 20+4m(5+m/0)    |          np np np (nr)*    nr np  
      CPU_ABUS_ACCESS_READ_FETCH_L; // np np
      abus=0xffffff&m68k_fetchL();
      pc+=4;  
      break;

    case 2:////(d16,An)      | 16+4m(4+m/0)    |             np np (nr)*    nr np
      CPU_ABUS_ACCESS_READ_FETCH; //np
      abus=pc+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case 3:
      INSTRUCTION_TIME(2);//n //390
      CPU_ABUS_ACCESS_READ_FETCH; //np
      m68k_iriwo=m68k_fetchW();
      if(m68k_iriwo&BIT_b){  //.l
        abus=pc+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=pc+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      pc+=2; 
      break;

#if defined(SSE_DEBUG)
    default:
      ASSERT(0);
#endif
    }
    break;
#if defined(SSE_DEBUG)
  default:
    ASSERT(0);
#endif
  }

  DWORD areg_hi=(areg[PARAM_M] & 0xff000000);
  TRUE_PC=pc+2;
#if defined(SSE_VC_INTRINSICS_390F)
  short bit=0;
#else
  short mask=1;
#endif
  for(int n=0;n<16;n++){
#if defined(SSE_VC_INTRINSICS_390F)
    if(BITTEST(m68k_src_w,bit)) {
#else
    if (m68k_src_w & mask){
#endif
      CPU_ABUS_ACCESS_READ;//(nr)*
      r[n]=(signed long)((signed short)m68k_dpeek(abus));
      abus+=2;
    }
#if defined(SSE_VC_INTRINSICS_390F)
    bit++;
#else
    mask<<=1;
#endif
  }
  if (postincrement) 
    areg[PARAM_M]=abus | areg_hi;
  CPU_ABUS_ACCESS_READ;//nr
  m68k_dpeek(abus); //extra word read (discarded)
  PREFETCH_IRC;//np
}

void                              m68k_jsr()
{
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
    case BITS_543_010: 
      // (An)          | 16(2/2)         |                      np nS ns np          
      CPU_ABUS_ACCESS_READ_FETCH;//np
      effective_address=areg[PARAM_M];
      break;

    case BITS_543_101: 
      // (d16,An)      | 18(2/2)         |                 n    np nS ns np          
      INSTRUCTION_TIME(2); //n
      CPU_ABUS_ACCESS_READ_FETCH;//np
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_543_110: 
      // (d8,An,Xn)    | 22(2/2)         |                 n nn np nS ns np          
      INSTRUCTION_TIME(6); //n nn
      CPU_ABUS_ACCESS_READ_FETCH;//np
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
        // (xxx).W       | 18(2/2)         |                 n    np nS ns np          
        INSTRUCTION_TIME(2); //n
        CPU_ABUS_ACCESS_READ_FETCH; //np
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
        break;

      case 1:
        // (xxx).L       | 20(3/2)         |                   np np nS ns np     
        CPU_ABUS_ACCESS_READ_FETCH_L;// np np
        effective_address=m68k_fetchL();
        pc+=4;  
        break;

      case 2:
        // (d16,An)      | 18(2/2)         |                 n    np nS ns np
        INSTRUCTION_TIME(2); //n
        CPU_ABUS_ACCESS_READ_FETCH; //np
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;

      case 3: 
        // (d8,An,Xn)    | 22(2/2)         |                 n nn np nS ns np      
        INSTRUCTION_TIME(6); //n nn
        CPU_ABUS_ACCESS_READ_FETCH; //np
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
  PREFETCH_CLASS(1); 
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=effective_address;
  // The sequence is 'np nS ns np'
  // First np could cause bus/address error, before stacking - Blood Money STX
  // It's different for BSR
  m68k_READ_W(abus); // Check for bus/address errors
#else
  m68k_READ_W(effective_address);
#endif
  m68k_PUSH_L(PC32);
  // 8 cycles more than JMP because we push PC
  CPU_ABUS_ACCESS_WRITE_PUSH_L; //nS ns
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPush(PC32);
#endif
  SET_PC(effective_address);
  CPU_ABUS_ACCESS_READ_FETCH; //np
  intercept_os();
}


void                              m68k_jmp()
{
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
    case BITS_543_010:
      //(An)          |  8(2/0)         |                      np       np  
      CPU_ABUS_ACCESS_READ_FETCH; //np
      effective_address=areg[PARAM_M];
      break;

    case BITS_543_101:
      //(d16,An)      | 10(2/0)         |                 n    np       np 
      INSTRUCTION_TIME(2); //n
      CPU_ABUS_ACCESS_READ_FETCH;  //np
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_543_110:
      //(d8,An,Xn)    | 14(2/0)         |                 n nn np       np
      INSTRUCTION_TIME(6); //n nn
      CPU_ABUS_ACCESS_READ_FETCH; //np
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
        //(xxx).W       | 10(2/0)         |                 n    np       np          
        INSTRUCTION_TIME(2);//n
        CPU_ABUS_ACCESS_READ_FETCH; //np
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
        break;

      case 1:
        //(xxx).L       | 12(3/0)         |                   np np       np          
        CPU_ABUS_ACCESS_READ_FETCH_L; //np np
        effective_address=m68k_fetchL();
        pc+=4;  
        break;

      case 2: 
        //(d16,An)      | 10(2/0)         |                 n    np       np 
        INSTRUCTION_TIME(2);//n
        CPU_ABUS_ACCESS_READ_FETCH;//np
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;

      case 3:
        //(d8,An,Xn)    | 14(2/0)         |                 n nn np       np
        INSTRUCTION_TIME(6);//n nn
        CPU_ABUS_ACCESS_READ_FETCH; //np
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
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=effective_address;
  m68k_READ_W(abus);
#else
  m68k_READ_W(effective_address); // Check for bus/address errors
#endif
  SET_PC(effective_address);  
  CPU_ABUS_ACCESS_READ_FETCH;//np
  intercept_os();
}


void                              m68k_chk(){
  if((ir&BITS_543)==BITS_543_001){
    m68k_unrecognised();
  }else{
/*
Yacht:
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        CHK       |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------

trap :            |                 |            |
  <ea>,Dn :       |                 |            |  
    .W :          |                 |            |
...
NOTES :
  . for more informations about the "trap" lines see exceptions section below.

WinUAE
CHK:

- 8 idle cycles
- write PC low word
- write SR
- write PC high word
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

Not exactly the same... cases?
*/
    m68k_GET_SOURCE_W; //EA
    PREFETCH_IRC; // np 
    //INSTRUCTION_TIME(6);//390
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("CHK");
#endif

    if(r[PARAM_N]&0x8000){
/*
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

        //np n- nn ns nS ns np np np n np
        //   2  4  4  4  4  4  4  4  4  4  -> np + 38
*/
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::EXCEPTION;
#endif
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_N_BIT);
#else
      SR_SET(SR_N);
#endif
#if defined(SSE_MMU_ROUNDING_BUS)
      INSTRUCTION_TIME(2);
      m68kTrapTiming();
#else
      INSTRUCTION_TIME_ROUND(38); //TODO
#endif
      m68k_interrupt(LPEEK(BOMBS_CHK*4));
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::NORMAL;
#endif
    }else if((signed short)LOWORD(r[PARAM_N])>(signed short)m68k_src_w){
/*
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

        //np    nn ns nS ns np np np n np
        //      4  4  4  4  4  4  4  4  4 -> np + 36
*/
#if defined(SSE_CPU_392B)
      M68000.ProcessingState=TM68000::EXCEPTION;
#endif
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_N_BIT);
#else
      SR_CLEAR(SR_N);
#endif
#if defined(SSE_MMU_ROUNDING_BUS)
      m68kTrapTiming();
#else
      INSTRUCTION_TIME_ROUND(36); //TODO
#endif
      m68k_interrupt(LPEEK(BOMBS_CHK*4));
#if defined(SSE_CPU_392B)
      M68000.ProcessingState=TM68000::NORMAL;
#endif
    }
    else // no trap
    {
/*
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
*/
      INSTRUCTION_TIME(6); // nn n
    }
  }
}


void                              m68k_lea(){
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
    case BITS_543_010: //(An)
      effective_address=areg[PARAM_M];
      break;

    case BITS_543_101: //(d16,An)
      CPU_ABUS_ACCESS_READ_FETCH; //np
      effective_address=areg[PARAM_M]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_543_110: //(d8,An,Xn)
      INSTRUCTION_TIME(2); //n
      CPU_ABUS_ACCESS_READ_FETCH; //np
      m68k_iriwo=m68k_fetchW();pc+=2; 
      INSTRUCTION_TIME(2); // n
      if (m68k_iriwo & BIT_b){  //.l
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        effective_address=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;

    case BITS_543_111:
      switch (ir & 0x7){
      case 0: //(xxx).W
        CPU_ABUS_ACCESS_READ_FETCH; //np
        effective_address=(signed long)(signed short)m68k_fetchW();
        pc+=2; 
        TRUE_PC+=2;
        break;

      case 1: //(xxx).L
        CPU_ABUS_ACCESS_READ_FETCH_L; //np np
        effective_address=m68k_fetchL();
        pc+=4;  
        TRUE_PC+=4;
        break;

      case 2: //(d16,PC) 
        CPU_ABUS_ACCESS_READ_FETCH; //np
        effective_address=(PC_RELATIVE_PC+(signed short)m68k_fetchW()) | pc_high_byte;
        PC_RELATIVE_MONITOR(effective_address);
        pc+=2; 
        break;

      case 3: //(d8,PC,Xn)
        INSTRUCTION_TIME(2); //n
        CPU_ABUS_ACCESS_READ_FETCH; //np
        m68k_iriwo=m68k_fetchW();
        INSTRUCTION_TIME(2); //n
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
  areg[PARAM_N]=effective_address;
  PREFETCH_IRC; //np
}


void                              m68k_line_4_stuff(){
  m68k_jump_line_4_stuff[ir&(BITS_543|0x7)]();
}


//#define LOGSECTION LOGSECTION_TRAP
void                              m68k_trap(){
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
#if defined(DEBUG_BUILD) && defined(SSE_CPU) && defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("TRAP",(ir&0xF));
#endif
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::EXCEPTION;
#endif
  m68kTrapTiming();
  m68k_interrupt(Vector);
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::NORMAL;
#endif
  intercept_os();
  debug_check_break_on_irq(BREAK_IRQ_TRAP_IDX);
}
//#undef LOGSECTION


void                              m68k_link(){
/*
Pushes the contents of the specified address register onto the stack. Then
loads the updated stack pointer into the address register. Finally, adds the
displacement value to the stack pointer.
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       LINK       |      INSTR      |  2nd Operand  |   INSTR
------------------+-----------------+---------------+--------------------------
An,#<data> :      |                 |
  .W :            | 16(2/2)         |                      np nS ns np          
*/
  PREFETCH_CLASS(1);
  CPU_ABUS_ACCESS_READ_FETCH; //np
  m68k_GET_IMMEDIATE_W;
  CPU_ABUS_ACCESS_WRITE_PUSH_L; // nS ns
  m68k_PUSH_L(areg[PARAM_M]);
  areg[PARAM_M]=r[15];
  r[15]+=(signed short)m68k_src_w;
  PREFETCH_IRC; //np
}


void                              m68k_unlk(){
/*
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
      UNLNK       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
An :              | 12(3/0)         |                         nU nu np          
*/
#if defined(SSE_CPU_SPLIT_RL)
  abus=r[15]=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ; //nR
  m68k_READ_W(abus)
  m68k_src_l=m68k_src_w<<16;
  abus+=2;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(abus)
  m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?  
#else
  CPU_ABUS_ACCESS_READ_POP_L; // nU nu
  r[15]=areg[PARAM_M];
  abus=r[15];m68k_READ_L_FROM_ADDR;
#endif
  PREFETCH_IRC; //np
  r[15]+=4;    //This is contrary to the Programmer's reference manual which says
  areg[PARAM_M]=m68k_src_l; //it does move (a7),An then adds 4 to a7, but it fixed Wrath of Demon
}


void                              m68k_move_to_usp(){
  if (SUPERFLAG){
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
     MOVE USP     |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------
 An,USP :         |  4(1/0)         |                               np          
*/
    other_sp=areg[PARAM_M];
    PREFETCH_IRC; //np
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}


void                              m68k_move_from_usp(){
  if (SUPERFLAG){
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
     MOVE USP     |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------
 USP,An :         |  4(1/0)         |                               np          
*/
    areg[PARAM_M]=other_sp;
    PREFETCH_IRC; //np
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}


void                              m68k_reset(){
  if (SUPERFLAG){
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       RESET      |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------
                  | 132(1/0)        |                     nn (??-)* np         
NOTES :
  .(??)* is for 124 cycles when the CPU will asserts the /RSTO signal in order 
   to reset all external devices. Normally, the CPU won't use the bus during 
   these cycles.
  .RESET instruction have nothing to do with /RESET exception. 
*/
#ifdef SSE_BUGFIX_393
    ioaccess=0;
#endif
    reset_peripherals(0);
    INSTRUCTION_TIME(124);//(??-)*
    PREFETCH_IRC;//np
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}


void                              m68k_nop(){
/*
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
       NOP        |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
                  |  4(1/0)         |                               np          
*/
  PREFETCH_IRC; //np
}


void                              m68k_stop(){
  if (SUPERFLAG){
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       STOP       |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------
                  |  4(0/0)         |                               n          
NOTES :
  .At this point, there is no real clues of how the second microcyle of this *
   instruction is spent. 
*/
    if (cpu_stopped==0){
/*http://www.atari-forum.com/viewtopic.php?f=68&t=13264&p=292437#p292437
"Yes, STOP can (kinda) pair with the previous instruction. It's not a pairing
in the strict sense. STOP would always take a multiple of 4 cycles. 
But STOP is transparent (because it doesn't perform any bus access) in terms 
of pairing to the previous instruction.

So EXG+STOP+INTR does pair, because EXG and INTR pair."

--> STOP must read the immediate, but we know it's already fetched, so 
    it just uses it without refetching at the same time.
*/
      m68k_GET_IMMEDIATE_W;
      DEBUG_ONLY( int debug_old_sr=sr; )
      sr=m68k_src_w; // IPL
      sr&=SR_VALID_BITMASK;
      DETECT_CHANGE_TO_USER_MODE;
      cpu_stopped=true;

      DETECT_TRACE_BIT;
      // Interrupts must come after trace exception
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
      CHECK_STOP_ON_USER_CHANGE;

/*  from Hatari:
    The CPU can't be restarted at once after a STOP, there's a
    delay of 8 cycles, even if an interrupt is pending.
    option C2: this fixes the spurious interrupt in Return -HMD (STE)
*/
      if(!M68000.tpend)//390
        INSTRUCTION_TIME(CPU_STOP_DELAY); 

    }else{ // already stopped
      // If we have got here then there were no interrupts pending when the IPL
      // was changed. Now unless we are in a blitter loop nothing can possibly
      // happen until cpu_cycles<=0.
      INSTRUCTION_TIME(4); // n*
    }
    SET_PC(old_pc); // note it refills prefetch (TODO)
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}


void                              m68k_rte(){
  //TRACE("rte\n");
  bool dont_intercept_os=false;
  if (SUPERFLAG){
    DEBUG_ONLY( int debug_old_sr=sr; )
/*
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
     RTE, RTR     |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
                  | 20(5/0)         |                      nU nu nu np np       
*/
    CPU_ABUS_ACCESS_READ_POP_L; //nU nu
    CPU_ABUS_ACCESS_READ_POP; //nu
    CPU_ABUS_ACCESS_READ_FETCH_L; // np np
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
            stemdos_rte();
            dont_intercept_os=true;
            break;
#endif
#if defined(SSE_CPU_LINE_F)
          case ON_RTE_LINE_F:
            interrupt_depth++; // there's a RTE after all, cancel 'compensate'
            break;
#endif
#ifndef NO_CRAZY_MONITOR
          case ON_RTE_LINE_A:
            on_rte=ON_RTE_RTE;
            SET_PC(on_rte_return_address);
            extended_monitor_hack();
            break;
#ifndef SSE_TOS_GEMDOS_EM_381B
          case ON_RTE_DONE_MALLOC_FOR_EM:
//            log_write(HEXSl(pc,6)+EasyStr(": Malloc done - returned $")+HEXSl(r[0],8));
            xbios2=(r[0]+255)&-256;
            //TRACE("EM xbios 2 %X\n",xbios2);
            LPEEK(SV_v_bas_ad)=xbios2;
            LPEEK(SVscreenpt)=xbios2;
            memcpy(r,save_r,16*4);
            on_rte=ON_RTE_RTE;
//            log_write_stack();
            break;
#endif
          case ON_RTE_EMHACK:
            on_rte=ON_RTE_RTE;
            extended_monitor_hack();
            break;
#endif
        }
      }
    }
//    dbg_log(EasyStr("RTE - decreasing interrupt depth from ")+interrupt_depth+" to "+(interrupt_depth-1));
    interrupt_depth--;
    ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
#if defined(SSE_TOS_NO_INTERCEPT_ON_RTE2) && !defined(DISABLE_STEMDOS)
/*  Hack for ReDMCSB, that will only complete 50%
    with TOS1.02.
*/
    if(stemdos_command==0x4c)
    {
      if(stemdos_Pexec_list_ptr==Tos.LastPTermedProcess)
        dont_intercept_os=true;
      else
        Tos.LastPTermedProcess=stemdos_Pexec_list_ptr;
    }
#endif
    if (!dont_intercept_os) 
      intercept_os();

    CHECK_STOP_ON_USER_CHANGE;
  }else{
    exception(BOMBS_PRIVILEGE_VIOLATION,EA_INST,0);
  }
}


void                              m68k_rtd(){
  m68k_unrecognised();
}

void                              m68k_rts(){
/*
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
       RTS        |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
                  | 16(4/0)         |                   nU nu    np np          
*/
  CPU_ABUS_ACCESS_READ_POP_L; //nU nu
  CPU_ABUS_ACCESS_READ_FETCH_L; // np np
  effective_address=m68k_lpeek(r[15]);
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPop();
#endif
  r[15]+=4;
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=effective_address;
  m68k_READ_W(abus);
#else
  m68k_READ_W(effective_address); // Check for bus/address errors
#endif
  SET_PC(effective_address);
  intercept_os();
#if defined(SSE_BOILER_RUN_TO_RTS) //rather useless?
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
/*
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
      TRAPV       |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
no trap           |  4(1/0)         |                               np          
   trap           | 34(5/3)         |          np ns nS ns np np np np          
NOTES :
  . for more informations about the "trap" line see exceptions section below.
*/
  if (sr & SR_V){
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("TRAPV");
#endif
#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::EXCEPTION;
#endif
    m68kTrapTiming();
    m68k_interrupt(LPEEK(BOMBS_TRAPV*4));
#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::NORMAL;
#endif
  }else{
    PREFETCH_IRC; //np
  }
}


void                              m68k_rtr(){
/*
-------------------------------------------------------------------------------
	                |    Exec Time    |               Data Bus Usage
     RTE, RTR     |      INSTR      |                  INSTR
------------------+-----------------+------------------------------------------
                  | 20(5/0)         |                      nU nu nu np np       
NOTES :
  .M68000UM is probably wrong with bus read accesses for RTR instruction. 
   It reads "20(2/0)" but, according to USP4325121 and with a little common 
   sense, 2 bus read accesses aren't enough for reading status register and 
   program counter values from the stack and prefetching.
*/
  CPU_ABUS_ACCESS_READ_POP_L; //nU nu
  CPU_ABUS_ACCESS_READ_POP; //nu
  CPU_ABUS_ACCESS_READ_FETCH_L; // np np
  CCR=LOBYTE(m68k_dpeek(r[15]));r[15]+=2;
  sr&=SR_VALID_BITMASK;
  effective_address=m68k_lpeek(r[15]);r[15]+=4;
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=effective_address;
  m68k_READ_W(abus);
#else
  m68k_READ_W(effective_address); // Check for bus/address errors
#endif
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
*/

void                              m68k_addq_b(){
/*
  .B              |                 |               |
    Dn            |  4(1/0)  0(0/0) |               |               np          
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       
*/
  m68k_src_b=(BYTE)PARAM_N;
  if(m68k_src_b==0)
    m68k_src_b=8;
  CHECK_READ=true;
  m68k_GET_DEST_B_NOT_A; //EA
  m68k_old_dest=m68k_DEST_B;
  PREFETCH_IRC; //np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
  }
  m68k_DEST_B+=m68k_src_b;
  SR_ADD_B;
  CHECK_IOW_B;
}

void                             m68k_addq_w(){
/*
  .W :            |                 |               |
    Dn            |  4(1/0)  0(0/0) |               |               np          
    An            |  8(1/0)  0(0/0) |               |               np       nn 
    (An)          |  8(1/1)  4(1/0) |            nr |               np nw       
    (An)+         |  8(1/1)  4(1/0) |            nr |               np nw       
    -(An)         |  8(1/1)  6(1/0) | n          nr |               np nw       
    (d16,An)      |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (d8,An,Xn)    |  8(1/1) 10(2/0) | n    np    nr |               np nw       
    (xxx).W       |  8(1/1)  8(2/0) |      np    nr |               np nw       
    (xxx).L       |  8(1/1) 12(3/0) |   np np    nr |               np nw       
NOTES :
  .M68000UM is probably wrong with instruction timing for ADDQ.W #<data>,An 
   instruction. It reads "4(1/0)" but, according to USP4325121, this 
   instruction is based on the same microwords than ADDQ.L #<data>,Dn and 
   ADDQ.L #<data>,An that have a "8(3/0)" timing. That makes sense because of 
   the calculation of a 32 bit address on 16 bits ALU. In addition, evaluation 
   on real hardware confirms the 8 cycles timing and, last, it makes addq and 
   subbq coherent in term of timing.
*/
  m68k_src_w=(WORD)PARAM_N;
  if(m68k_src_w==0)
    m68k_src_w=8;
  if((ir&BITS_543)==BITS_543_001){ //addq.w to address register
    PREFETCH_IRC; //np
    INSTRUCTION_TIME(4); //nn
    areg[PARAM_M]+=m68k_src_w;
  }else{
    CHECK_READ=true;
    m68k_GET_DEST_W_NOT_A; //EA
    m68k_old_dest=m68k_DEST_W;
    PREFETCH_IRC; // np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; // nw
    }
    m68k_DEST_W+=m68k_src_w;
#if defined(SSE_CPU_DATABUS)
    dbus=m68k_DEST_W;
#endif
    SR_ADD_W;
    CHECK_IOW_W;
  }
}


void                             m68k_addq_l(){
/*
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
*/
  m68k_src_l=(LONG)PARAM_N;if(m68k_src_l==0)m68k_src_l=8;
  if((ir&BITS_543)==BITS_543_001){ //addq.l to address register
    PREFETCH_IRC; // np
    INSTRUCTION_TIME(4); // nn
    areg[PARAM_M]+=m68k_src_l;
  }else{
    CHECK_READ=true;
    m68k_GET_DEST_L_NOT_A; // EA
    m68k_old_dest=m68k_DEST_L;
    PREFETCH_IRC; // np
    if(DEST_IS_DATA_REGISTER)
      INSTRUCTION_TIME(4); // nn
    else
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE_L; // nw nW
    }
    m68k_DEST_L+=m68k_src_l;
    SR_ADD_L;
    CHECK_IOW_L;
  }
}


void                              m68k_subq_b(){
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
  m68k_src_b=(BYTE)PARAM_N;
  if(m68k_src_b==0)
    m68k_src_b=8;
  CHECK_READ=true;
  m68k_GET_DEST_B_NOT_A; // EA
  m68k_old_dest=m68k_DEST_B;
  PREFETCH_IRC; //np
  if(!DEST_IS_DATA_REGISTER)
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
  }
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(SR_X);
  CHECK_IOW_B;
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
  m68k_src_w=(WORD)PARAM_N;
  if(m68k_src_w==0)
    m68k_src_w=8;
  if((ir&BITS_543)==BITS_543_001){ //subq.w to address register
    PREFETCH_IRC; // np
    INSTRUCTION_TIME(4); //nn
    areg[PARAM_M]-=m68k_src_w;
  }else{
    CHECK_READ=true;
    m68k_GET_DEST_W_NOT_A; // EA
    m68k_old_dest=m68k_DEST_W;
    PREFETCH_IRC; //np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_W-=m68k_src_w;
    SR_SUB_W(SR_X);
    CHECK_IOW_W;
  }
}


void                             m68k_subq_l(){
/*
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
*/
  m68k_src_l=(LONG)PARAM_N;
  if(m68k_src_l==0)
    m68k_src_l=8;
  if((ir&BITS_543)==BITS_543_001){ //subq.l to address register
    PREFETCH_IRC; //np
    INSTRUCTION_TIME(4); //nn
    areg[PARAM_M]-=m68k_src_l;
  }else{
    CHECK_READ=true;
    m68k_GET_DEST_L_NOT_A; //EA
    m68k_old_dest=m68k_DEST_L;
    PREFETCH_IRC; //np
    if(DEST_IS_DATA_REGISTER)
      INSTRUCTION_TIME(4); //nn
    else
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE_L; // nw nW
    }
    m68k_DEST_L-=m68k_src_l;
    SR_SUB_L(SR_X);
    CHECK_IOW_L;
  }
}

void                              m68k_dbCC_or_sCC(){
  if ((ir&BITS_543)==BITS_543_001){ // DBCC
/* Motorola
If Condition False 
Then (Dn - 1 -> Dn; If Dn  <> -1 Then PC + dn -> PC) 
*/

/* Yacht
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
    INSTRUCTION_TIME(2); // n 
    if (!m68k_CONDITION_TEST){ 
      //  /cc
      (*((WORD*)(&(r[PARAM_M]))))--;

      CPU_ABUS_ACCESS_READ_FETCH; //np
      m68k_GET_IMMEDIATE_W; 

      if( (*( (signed short*)(&(r[PARAM_M]) ))) != (signed short)(-1) ){
        // counter not expired, branch taken
        // | 10(2/0)         |                      n np       np        
        MEM_ADDRESS new_pc=(pc+(signed short)m68k_src_w-2) | pc_high_byte;
#if defined(SSE_MMU_ROUNDING_BUS)
        abus=new_pc;
        m68k_READ_W(abus);
#else
        m68k_READ_W(new_pc); // Check for bus/address errors
#endif
        SET_PC(new_pc); 
        CPU_ABUS_ACCESS_READ_FETCH; // np
      }else{ 
        // counter expired, branch not taken
        // | 14(3/0)         |                      n np    np np     
        CPU_ABUS_ACCESS_READ_FETCH; //np
        PREFETCH_IRC; //np
      }
    }else{ 
      //  cc
      //  | 12(2/0)         |                      n     n np np  
      INSTRUCTION_TIME(2); //n
      CPU_ABUS_ACCESS_READ_FETCH; //np
      m68k_GET_IMMEDIATE_W;
      PREFETCH_IRC; //np
    }
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
    CHECK_READ=true;
    m68k_GET_DEST_B; // EA
    PREFETCH_IRC; // np
    if(m68k_CONDITION_TEST) //cc true
    {
      if(DEST_IS_REGISTER)
        INSTRUCTION_TIME(2); // n
      else 
      {
#if defined(SSE_CPU_RESTORE_ABUS1)
        abus=dest_addr; 
#endif
        CPU_ABUS_ACCESS_WRITE; //nw
      }
      m68k_DEST_B=(BYTE)0xff; 
    }
    else // cc false
    {
      if(!DEST_IS_REGISTER)
      {
#if defined(SSE_CPU_RESTORE_ABUS1)
        abus=dest_addr; 
#endif
        CPU_ABUS_ACCESS_WRITE; // nw
      }
      m68k_DEST_B=0;
    }
    CHECK_IOW_B;
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


/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
      AND, OR     |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
*/

void                              m68k_or_b_to_dN(){
/*
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
  m68k_GET_SOURCE_B_NOT_A; //EA
  PREFETCH_IRC; //np
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_B|=m68k_src_b;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_B;
  CHECK_IOW_B;
}


void                             m68k_or_w_to_dN(){
/*
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
  m68k_GET_SOURCE_W_NOT_A; //EA
  PREFETCH_IRC; //np
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_W|=m68k_src_w;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_W;
}


void                             m68k_or_l_to_dN(){
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
  m68k_GET_SOURCE_L_NOT_A; // EA
  PREFETCH_IRC; //np
  if (SOURCE_IS_REGISTER_OR_IMMEDIATE)
    INSTRUCTION_TIME(4);//nn
  else 
    INSTRUCTION_TIME(2);//n
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L|=m68k_src_l;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_L;
}


void                              m68k_divu(){
  m68k_GET_SOURCE_W_NOT_A; // EA
  if (m68k_src_w==0){ // div by 0
#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::EXCEPTION;
#endif
    // Clear V flag when dividing by zero. (from WinUAE)
    // also clear CC
#if defined(SSE_VC_INTRINSICS_390E)
    BITRESET(sr,SR_V_BIT);
    BITRESET(sr,SR_C_BIT);
#else
    SR_CLEAR(SR_V);
    SR_CLEAR(SR_C);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("DIV");
#endif
//Divide by Zero      | 38(4/3)+ |           nn nn    ns nS ns nV nv np np (36 accounted)
//Divide by Zero      | 38(4/3)+ |           nn nn    ns nS ns nV nv np n np ("corrected")
#if defined(SSE_MMU_ROUNDING_BUS)
      INSTRUCTION_TIME(4);
      m68kTrapTiming();
#else
    INSTRUCTION_TIME_ROUND(38); //TODO
#endif
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::NORMAL;
#endif
  }
  else
  {
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       DIVU       |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea>,Dn :        /                  |               |
  .W :          |                   |               |
    Dn          | 76+?(1/0)  0(0/0) |               | nn n [seq.] n nn np       
    (An)        | 76+?(1/0)  4(1/0) |            nr | nn n [seq.] n nn np       
    (An)+       | 76+?(1/0)  4(1/0) |            nr | nn n [seq.] n nn np       
    -(An)       | 76+?(1/0)  6(1/0) | n          nr | nn n [seq.] n nn np       
    (d16,An)    | 76+?(1/0)  8(2/0) |      np    nr | nn n [seq.] n nn np       
    (d8,An,Xn)  | 76+?(1/0) 10(2/0) | n    np    nr | nn n [seq.] n nn np       
    (xxx).W     | 76+?(1/0)  8(2/0) |      np    nr | nn n [seq.] n nn np       
    (xxx).L     | 76+?(1/0) 12(2/0) |   np np    nr | nn n [seq.] n nn np       
    #<data>     | 76+?(1/0)  8(2/0) |   np np       | nn n [seq.] n nn np       
NOTES :
  .Overflow always cost 10 cycles (n nn np).
  .For more informations about division by zero see exceptions section below.
  .[seq.] refers to 15 consecutive blocks to be chosen in the following 3 :
   (nn), (nnn-) or (nnn-n). 
   (see following flowchart for details).
  .Best case : 76 cycles (nn n [nn]*15 n nn np)
  .Worst case : 136 (140 ?) cycles.
FLOWCHART :
                                       n                                        
                                       n             Divide by zero             
                  +--------------------+--------------------+                   
  Overflow        n                                         n                   
      +-----------+----------+                              n                   
     np                      n<---------------------+-+-+   nw                  
                             n                      | | |   nw                  
               No more bits  |  pMSB=0       pMSB=1 | | |   nw                  
                       +-----+-----+-----------+    | | |   np                  
                       n    MSB=0  n- MSB=1    |    | | |   np                  
                       np      +---+---+       +----+ | |   np                  
                               |       n              | |   np                  
                               |       +--------------+ |                       
                               +------------------------+                       
  .for each iteration of the loop : shift dividend to the left by 1 bit then    
   substract divisor to the MSW of new dividend, discard after test if result   
   is negative keep it if positive.                                             
  .MSB = most significant bit : bit at the far left of the dividend             
  .pMSB = previous MSB : MSB used in the previous iteration of the loop                                                                                              
*/
    unsigned long q;
    unsigned long dividend = (unsigned long) (r[PARAM_N]);
    unsigned short divisor = (unsigned short) m68k_src_w;
    // using ijor's timings (in 3rdparty\pasti)
    int cycles_for_instr=getDivu68kCycles(dividend,divisor) -4; // -prefetch
#if defined(SSE_BLT_392)
    INSTRUCTION_TIME(4);
    INSTRUCTION_TIME(cycles_for_instr-4); 
#else
    INSTRUCTION_TIME(cycles_for_instr); // fixes Pandemonium loader
#endif
    q=(unsigned long)((unsigned long)dividend)/(unsigned long)((unsigned short)divisor);
    if(q&0xffff0000){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_V_BIT);
#else
      SR_SET(SR_V);
#endif
/*  
"N : Set if the quotient is negative; cleared otherwise; undefined if overflow 
  or divide by zero occurs."
    Setting it if overflow fixes Sadeness and Spectrum Analyzer by Axxept.
    (hex) 55667788/55=349B+101 strange, neither the dividend nor the quotient
    are negative.
    The value of SR is used by the trace protection decoder of those demos.
    But Z must be cleared.
    v3.7: 
    C — Always cleared, official doc (Spacker II)
*/
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_N_BIT);
      BITRESET(sr,SR_C_BIT);
#else
      SR_SET(SR_N);
      SR_CLEAR(SR_C);
#endif
    }else{
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
#if defined(SSE_VC_INTRINSICS_390E)
      if(q&MSB_W)
        BITSET(sr,SR_N_BIT);
      if(q==0)
        BITSET(sr,SR_Z_BIT);
#else
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
#endif
      r[PARAM_N]=((((unsigned long)r[PARAM_N])%((unsigned short)m68k_src_w))<<16)+q;
    }
    PREFETCH_IRC; //np
  }
}


void                              m68k_or_b_from_dN_or_sbcd(){
  switch(ir&BITS_543){
  case BITS_543_000:case BITS_543_001:{  //sbcd
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ABCD, SBCD    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
*/
    if((ir&BITS_543)==BITS_543_000){
/*
Dy,Dx :           |                 |
  .B :            |  6(1/0)         |                               np       n  
*/
      PREFETCH_IRC; //np
      INSTRUCTION_TIME(2); //n
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .B :            | 18(3/1)         |                 n    nr    nr np nw       
*/
      INSTRUCTION_TIME(2); //n
      areg[PARAM_M]--;
      if(PARAM_M==7)
        areg[PARAM_M]--;
      areg[PARAM_N]--;
      if(PARAM_N==7)
        areg[PARAM_N]--;
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ; //nr
      m68k_src_b=m68k_peek(abus);
      CHECK_READ=true;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ; //nr
      m68k_SET_DEST_B(abus);
      PREFETCH_IRC; //np
    }
    // computing of result lifted from WinUAE
    BYTE src = m68k_src_b;
    BYTE dst = (m68k_DEST_B&0xff);
    BYTE lo_nibble=(dst & 0xF) - (src & 0xF) - ((sr&SR_X)?1:0);
    SR_CLEAR(SR_X+SR_C+SR_N);
    if(lo_nibble&0xF0)
    {
      lo_nibble-=6;
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C);//internal
#endif
    }
    WORD hi_nibble=(dst & 0xF0) - (src & 0xF0) - ((sr&SR_C)?0x10:0);
#if defined(SSE_VC_INTRINSICS_390E)
    BITRESET(sr,SR_C_BIT);
#else
    SR_CLEAR(SR_C);//clear internal bit
#endif
    if(hi_nibble&0xF00)
    {
      hi_nibble-=0x60;
      SR_SET(SR_X+SR_C+SR_N);
    }
    if((ir&BITS_543)==BITS_543_001) //390
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_B=(hi_nibble&0xF0)+(lo_nibble&0xF);
    if(!m68k_DEST_B)
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z)
#endif
    CHECK_IOW_B;
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
    CHECK_READ=true;
    m68k_GET_DEST_B_NOT_A; // EA
    m68k_src_b=LOBYTE(r[PARAM_N]);  
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_B|=m68k_src_b;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
    CHECK_IOW_B;
  }//sw
}


void                             m68k_or_w_from_dN(){
  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
    m68k_unrecognised();
    break;
  default:
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
    CHECK_READ=true;
    m68k_GET_DEST_W_NOT_A; //EA
    m68k_src_w=LOWORD(r[PARAM_N]);
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_W|=m68k_src_w;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_W;
#if defined(SSE_BUGFIX_393B)
    CHECK_IOW_W; //(D-Bug 117)
#endif
  }
}


void                             m68k_or_l_from_dN(){
  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
    m68k_unrecognised();
    break;
  default:
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
    CHECK_READ=true;
    m68k_GET_DEST_L_NOT_A; //EA
    m68k_src_l=r[PARAM_N];
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; //nw nW
    m68k_DEST_L|=m68k_src_l;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_L;
    CHECK_IOW_L;
  }
}


void                              m68k_divs(){
  m68k_GET_SOURCE_W_NOT_A; //EA
  if (m68k_src_w==0){
#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::EXCEPTION;
#endif
#if defined(SSE_VC_INTRINSICS_390E)
    BITRESET(sr,SR_V_BIT);
    BITRESET(sr,SR_C_BIT);
#else
    SR_CLEAR(SR_V);
    SR_CLEAR(SR_C);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("DIV");
#endif
//Divide by Zero      | 38(4/3)+ |           nn nn    ns nS ns nV nv np np (36 accounted)
//Divide by Zero      | 38(4/3)+ |           nn nn    ns nS ns nV nv np n np ("corrected")
#if defined(SSE_MMU_ROUNDING_BUS)
      INSTRUCTION_TIME(4);
      m68kTrapTiming();
#else
    INSTRUCTION_TIME_ROUND(38); //TODO
#endif
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
#if defined(SSE_CPU_392B)
    M68000.ProcessingState=TM68000::NORMAL;
#endif
  }else{
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       DIVS       |  INSTR     EA   |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea>,Dn :       /                  /            /
 .W :         /                  /          /
  Dn        |120+?(1/0)  0(0/0)|          | nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  (An)      |120+?(1/0)  4(1/0)|        nr| nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  (An)+     |120+?(1/0)  4(1/0)|        nr| nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  -(An)     |120+?(1/0)  6(1/0)|n       nr| nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  (d16,An)  |120+?(1/0)  8(2/0)|     np nr| nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  (d8,An,Xn)|120+?(1/0) 10(2/0)|n    np nr| nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  (xxx).W   |120+?(1/0)  8(2/0)|     np nr| nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  (xxx).L   |120+?(1/0) 12(2/0)|  np np nr| nn nnn (n)n [seq] nnn-nnn n(n(n))np 
  #<data>   |120+?(1/0)  8(2/0)|  np np   | nn nnn (n)n [seq] nnn-nnn n(n(n))np 
NOTES :
  .Overflow cost 16 or 18 cycles depending on dividend sign (n nn nn (n)n np).
  .For more informations about division by zero see exceptions section below.
  .[seq.] refers to 15 consecutive blocks to be chosen in the following 2 :
   (nnn-n) or (nn-n).
   (see following flowchart for details).
  .Best case : 120-122 cycles depending on dividend sign.
  .Worst case : 156 (158 ?) cycles.
FLOWCHART :                                   
                                         n                                      
                                         n                       Divide by zero 
                                +--------+--------------------------------+     
                                n                                         n     
                                n                                         n     
                  Dividend<0    n    Dividend>0                           nw    
                          +-----+-----+                                   nw    
                          n           |                                   nw    
                          +---------->n                                   np    
                                      |    Overflow                       np    
        +-----------------+      +----+----+                              np    
        | +----------+    +----->n         np                             np    
        | |          +---------->n                                              
        | |                      n-                                             
        | | MSB=1      MSB=0     n   No more bits                               
        | | +-----------+--------+--------+                                     
        | +-+           |                 n                                     
        +---------------+   divisor<0     n        divisor>0                    
                               +----------+-----------+                         
                               n         dividend<0   n   dividend>0            
                               n           +----------+----+                    
                               np          n               np                   
                                           n                                    
                                           np                                   
  .for each iteration of the loop : shift quotient to the left by 1 bit.             
  .MSB = most significant bit : bit at the far left of the quotient.                  
*/
    signed long q;
    signed long dividend = (signed long) (r[PARAM_N]);
    signed short divisor = (signed short) m68k_src_w;
    // using ijor's timings (in 3rdparty\pasti)
    int cycles_for_instr=getDivs68kCycles(dividend,divisor)-4; // -prefetch
#if defined(SSE_BLT_392) //other places?
    INSTRUCTION_TIME(4);
    INSTRUCTION_TIME(cycles_for_instr-4); 
#else
    INSTRUCTION_TIME(cycles_for_instr);   // fixes Dragonnels loader
#endif
#if defined(SSE_CPU_DIVS_OVERFLOW_PC)
    ASSERT(divisor);
#if defined(BCC_BUILD) || (defined(VC_BUILD) && _MSC_VER < 1500) 
    ASSERT(dividend!=(-2147483647 - 1)); // X86 crashes on div overflow
    if(dividend==(-2147483647 - 1))
#else
    ASSERT(dividend!=INT_MIN); // X86 crashes on div overflow
    if(dividend==INT_MIN)
#endif
      q=dividend;//-32768-1; 
    else
#endif
    q=(signed long)((signed long)dividend)/(signed long)((signed short)divisor);
    if(q<-32768 || q>32767){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_V_BIT);
      BITSET(sr,SR_N_BIT); // TODO test prg
      BITRESET(sr,SR_C_BIT);
      SR_CLEAR(SR_C);
#else
      SR_SET(SR_V);
      SR_SET(SR_N); // maybe
#endif
    }
    else
    {
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
#if defined(SSE_VC_INTRINSICS_390E)
      if(q&MSB_W)
        BITSET(sr,SR_N_BIT);
      if(q==0)
        BITSET(sr,SR_Z_BIT);
#else
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
#endif
      r[PARAM_N]=((((signed long)r[PARAM_N])%((signed short)m68k_src_w))<<16)|((long)LOWORD(q));
    }
    PREFETCH_IRC; //np
  }
}



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


void                              m68k_sub_b_to_dN(){
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
  m68k_GET_SOURCE_B_NOT_A; // EA
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(SR_X);
  PREFETCH_IRC; //np
}


void                             m68k_sub_w_to_dN(){
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
  m68k_GET_SOURCE_W;   //EA - A is allowed
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(SR_X);
  PREFETCH_IRC; //np
}


void                             m68k_sub_l_to_dN(){
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
  m68k_GET_SOURCE_L; //EA - A is allowed
  PREFETCH_IRC;
  if(SOURCE_IS_REGISTER_OR_IMMEDIATE)
    INSTRUCTION_TIME(4); // nn
  else 
    INSTRUCTION_TIME(2); // n
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(SR_X);
}


void                             m68k_suba_w(){
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
  m68k_GET_SOURCE_W; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(4); //nn
  m68k_src_l=(signed long)((signed short)m68k_src_w);
  areg[PARAM_N]-=m68k_src_l;
}


void                              m68k_sub_b_from_dN(){
  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
*/
    if((ir&BITS_543)==BITS_543_000){
/*
Dy,Dx :           |                 |
  .B or .W :      |  4(1/0)         |                               np          
*/
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }
    else
    {
/*
-(Ay),-(Ax) :     |                 |
  .B or .W :      | 18(3/1)         |              n nr    nr       np nw       
*/
      INSTRUCTION_TIME(2); // n
      areg[PARAM_M]--;      
      if(PARAM_M==7)
        areg[PARAM_M]--;
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ; // nr
      m68k_src_b=m68k_peek(areg[PARAM_M]);
      areg[PARAM_N]--;      
      if(PARAM_N==7)
        areg[PARAM_N]--;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ; // nr
      CHECK_READ=true;
      m68k_SET_DEST_B(abus); // can crash
    }
    m68k_old_dest=m68k_DEST_B;
    PREFETCH_IRC; // np
    if((ir&BITS_543)==BITS_543_001) // PREFETCH_IRC costs more than this test
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_B-=m68k_src_b;
    if(sr&SR_X)
      m68k_DEST_B--;
    SR_SUBX_B;
    CHECK_IOW_B;
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
    m68k_src_b=LOBYTE(r[PARAM_N]);
    CHECK_READ=true;
    m68k_GET_DEST_B_NOT_A; // EA
    PREFETCH_IRC; // np
    m68k_old_dest=m68k_DEST_B;
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_B-=m68k_src_b;
    SR_SUB_B(SR_X);
    CHECK_IOW_B;
  }
}


void                              m68k_sub_w_from_dN(){
  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
*/
    if((ir&BITS_543)==BITS_543_000){
/*
Dy,Dx :           |                 |
  .B or .W :      |  4(1/0)         |                               np          
*/
      m68k_src_w=LOWORD(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .B or .W :      | 18(3/1)         |              n nr    nr       np nw       
*/
      INSTRUCTION_TIME(2); //n
      areg[PARAM_M]-=2;
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ; //nr
      m68k_src_w=m68k_dpeek(abus);
      CHECK_READ=true;
      areg[PARAM_N]-=2;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ; //nr
      m68k_SET_DEST_W(abus);
    }
    m68k_old_dest=m68k_DEST_W;
    PREFETCH_IRC; //np
    if((ir&BITS_543)==BITS_543_001)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_W-=m68k_src_w;
    if(sr&SR_X)
      m68k_DEST_W--;
    SR_SUBX_W;
    CHECK_IOW_W;
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
    m68k_src_w=LOWORD(r[PARAM_N]);
    CHECK_READ=true;
    m68k_GET_DEST_W_NOT_A; //EA
    PREFETCH_IRC; //np
    m68k_old_dest=m68k_DEST_W;
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_W-=m68k_src_w;
    SR_SUB_W(SR_X);
    CHECK_IOW_W;
  }
}


void                              m68k_sub_l_from_dN(){
  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
*/
    if((ir&BITS_543)==BITS_543_000){
/*
Dy,Dx :           |                 |
  .L :            |  8(1/0)         |                               np       nn 
*/
      PREFETCH_IRC; //np
      INSTRUCTION_TIME(4); //nn
      m68k_src_l=r[PARAM_M];
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .L :            | 30(5/2)         |              n nr nR nr nR nw np    nW    
*/
      PREFETCH_CLASS(1);
      INSTRUCTION_TIME(2); //n 
      areg[PARAM_M]-=4;
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ_L; // nr nR
      m68k_src_l=m68k_lpeek(abus);
      CHECK_READ=true;
      areg[PARAM_N]-=4;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ_L; // nr nR
      m68k_SET_DEST_L(abus);
      CPU_ABUS_ACCESS_WRITE; //nw
      PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nW
    }
    m68k_old_dest=m68k_DEST_L;
    m68k_DEST_L-=m68k_src_l;
    if(sr&SR_X)m68k_DEST_L--;
    SR_SUBX_L;
    CHECK_IOW_L;
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
    m68k_src_l=r[PARAM_N];
    m68k_GET_DEST_L_NOT_A; // EA
    PREFETCH_IRC; //np
    m68k_old_dest=m68k_DEST_L;
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nw nW
    m68k_DEST_L-=m68k_src_l;
    SR_SUB_L(SR_X);
    CHECK_IOW_L;
  }
}


void                             m68k_suba_l(){
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
  m68k_GET_SOURCE_L; // EA
  PREFETCH_IRC; //np
  if (SOURCE_IS_REGISTER_OR_IMMEDIATE)
    INSTRUCTION_TIME(4); // nn
  else 
    INSTRUCTION_TIME(2); //n
  areg[PARAM_N]-=m68k_src_l;
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
*/

void                              m68k_cmp_b(){
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
  m68k_GET_SOURCE_B_NOT_A; //EA
  PREFETCH_IRC; //np
  m68k_old_dest=LOBYTE(r[PARAM_N]);
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_B-=m68k_src_b;
  SR_SUB_B(0);
}


void                             m68k_cmp_w(){
/*
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
  m68k_GET_SOURCE_W;//EA
  PREFETCH_IRC;//np
  m68k_old_dest=LOWORD(r[PARAM_N]);
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_W-=m68k_src_w;
  SR_SUB_W(0);
}


void                             m68k_cmp_l(){
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
  m68k_GET_SOURCE_L; //EA
  PREFETCH_IRC;//np
  INSTRUCTION_TIME(2);//n
  m68k_old_dest=r[PARAM_N];
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
}


void                             m68k_cmpa_w(){
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage             
       CMPA       |  INSTR     EA   |  1st OP (ea)  |          INSTR           
------------------+-----------------+---------------+--------------------------
<ea>,An :         |                 |               | 
   .W :           |                 |               | 
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
  m68k_GET_SOURCE_W; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2); //n
  m68k_src_l=(signed long)((signed short)m68k_src_w);
  m68k_old_dest=areg[PARAM_N];
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
}


void                              m68k_eor_b(){ //or CMPM.B
  if((ir&BITS_543)==BITS_543_001){  //cmpm
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
       CMPM       |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
(Ay)+,(Ax)+       |                 |
  .B or .W :      | 12(3/0)         |                      nr    nr np          
*/
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    m68k_src_b=m68k_peek(abus);
    areg[PARAM_M]++; 
    if(PARAM_M==7)
      areg[PARAM_M]++;
    abus=areg[PARAM_N];
    CPU_ABUS_ACCESS_READ; //nr
    m68k_old_dest=m68k_peek(abus);
    areg[PARAM_N]++; 
    if(PARAM_N==7)
      areg[PARAM_N]++;
    compare_buffer=m68k_old_dest;
    m68k_dest=&compare_buffer;
    PREFETCH_IRC; //np
    m68k_DEST_B-=m68k_src_b;
    SR_SUB_B(0);
    CHECK_IOW_B;
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
    CHECK_READ=true;
    m68k_GET_DEST_B_NOT_A; //EA
    PREFETCH_IRC; //np
    if(!DEST_IS_DATA_REGISTER)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE;  //nw
    }
    m68k_DEST_B^=LOBYTE(r[PARAM_N]);
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
    CHECK_IOW_B;
  }
}


void                             m68k_eor_w(){ //or CMPM.W
  if((ir&BITS_543)==BITS_543_001){  //cmpm
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
       CMPM       |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
(Ay)+,(Ax)+       |                 |
  .B or .W :      | 12(3/0)         |                      nr    nr np          
*/
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    m68k_src_w=m68k_dpeek(abus);
    areg[PARAM_M]+=2;
    abus=areg[PARAM_N];
    CPU_ABUS_ACCESS_READ; //nr
    m68k_old_dest=m68k_dpeek(abus);
    areg[PARAM_N]+=2;
    compare_buffer=m68k_old_dest;
    m68k_dest=&compare_buffer;
    PREFETCH_IRC; //np
    m68k_DEST_W-=m68k_src_w;
    SR_SUB_W(0);
    CHECK_IOW_W;
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
    CHECK_READ=true;
    m68k_GET_DEST_W_NOT_A;//EA
    PREFETCH_IRC; //np
    if(!(DEST_IS_DATA_REGISTER))
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_W^=LOWORD(r[PARAM_N]);
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_W;
    CHECK_IOW_W;
  }
}


void                             m68k_eor_l(){ //or CMPM.L
  if((ir&BITS_543)==BITS_543_001){  //cmpm
//   .L :            | 20(5/0)         |                   nR nr nR nr np   
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ_L; //nR nr
    m68k_src_l=m68k_lpeek(abus);
    areg[PARAM_M]+=4;
    abus=areg[PARAM_N];
    CPU_ABUS_ACCESS_READ_L; //nR nr
    m68k_old_dest=m68k_lpeek(abus);
    areg[PARAM_N]+=4;
    compare_buffer=m68k_old_dest;
    m68k_dest=&compare_buffer;
    PREFETCH_IRC; //np
    m68k_DEST_L-=m68k_src_l;
    SR_SUB_L(0);
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
    CHECK_READ=true;
    m68k_GET_DEST_L_NOT_A; //EA
    PREFETCH_IRC; //np
    if(DEST_IS_DATA_REGISTER)
      INSTRUCTION_TIME(4);//nn
    else
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE_L; //nw nW
    }
    m68k_DEST_L^=r[PARAM_N];
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_L;
    CHECK_IOW_L;
  }
}


void                             m68k_cmpa_l(){
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
  m68k_GET_SOURCE_L; //EA
  PREFETCH_IRC;//np
  INSTRUCTION_TIME(2);//n
  m68k_old_dest=areg[PARAM_N];
  compare_buffer=m68k_old_dest;
  m68k_dest=&compare_buffer;
  m68k_DEST_L-=m68k_src_l;
  SR_SUB_L(0);
  CHECK_IOW_L;
}


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

void                              m68k_and_b_to_dN(){
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
  m68k_GET_SOURCE_B_NOT_A; // EA
  PREFETCH_IRC; //np
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_B&=m68k_src_b;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_B;
  CHECK_IOW_B;
}


void                             m68k_and_w_to_dN(){
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
  m68k_GET_SOURCE_W_NOT_A; //EA
  PREFETCH_IRC; //np
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_W&=m68k_src_w;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_W;
}


void                             m68k_and_l_to_dN(){
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
  m68k_GET_SOURCE_L_NOT_A; //EA
  PREFETCH_IRC; //np
  if(SOURCE_IS_REGISTER_OR_IMMEDIATE)
    INSTRUCTION_TIME(4); //nn
  else 
    INSTRUCTION_TIME(2); //n
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L&=m68k_src_l;
  SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
  SR_CHECK_Z_AND_N_L;
}


void                              m68k_mulu(){
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
  m68k_GET_SOURCE_W_NOT_A; // EA
  PREFETCH_IRC; // np 
  INSTRUCTION_TIME(34);
#if defined(SSE_VC_INTRINSICS_390B)
  int m=count_bits_set_in_word(m68k_src_w);
  INSTRUCTION_TIME(2*m); //n*
#else
  for (WORD Val=m68k_src_w;Val;Val>>=1){ 
    if (Val & 1) INSTRUCTION_TIME(2);
  }
#endif
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L=(unsigned long)LOWORD(r[PARAM_N])*(unsigned long)((unsigned short)m68k_src_w);
  SR_CLEAR(SR_Z+SR_N+SR_C+SR_V); //SS 16bit x 16bit < 32bit, no overflow
  SR_CHECK_Z_AND_N_L;
}


void                              m68k_and_b_from_dN_or_abcd(){
  switch (ir & BITS_543){
  case BITS_543_000:case BITS_543_001:{ //SS ABCD
/*
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ABCD, SBCD    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
*/
    if((ir&BITS_543)==BITS_543_000){ //SS R+R
/*
Dy,Dx :           |                 |
  .B :            |  6(1/0)         |                               np       n  
*/
      PREFETCH_IRC; //np
      INSTRUCTION_TIME(2); //n
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{ //SS M+M
/*
-(Ay),-(Ax) :     |                 |
  .B :            | 18(3/1)         |                 n    nr    nr np nw       
*/
      INSTRUCTION_TIME(2); //n
      areg[PARAM_M]--;
      if(PARAM_M==7)
        areg[PARAM_M]--;
      areg[PARAM_N]--;
      if(PARAM_N==7)
        areg[PARAM_N]--;
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ; //nr
      m68k_src_b=m68k_peek(abus);
      CHECK_READ=true;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ; // nr
      m68k_SET_DEST_B(abus);
      PREFETCH_IRC; //np
    }
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
  It is correct in WinUAE, so we use the same way now.
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
  if((ir&BITS_543)==BITS_543_001)//390
  {
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
  }
  m68k_DEST_B=(hi_nibble&0xF0)+(lo_nibble&0xF);
#if defined(SSE_VC_INTRINSICS_390E)
  if(!m68k_DEST_B)
    BITSET(sr,SR_Z_BIT);
  else if(m68k_DEST_B<0)
    BITSET(sr,SR_N_BIT);
#else
  if(!m68k_DEST_B)
    SR_SET(SR_Z)
  else if(m68k_DEST_B<0)
    SR_SET(SR_N) // not sure of that, it's so in WinUAE
#endif
  CHECK_IOW_B;
  break;
  }
  default: // and.b from dn
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
    m68k_GET_DEST_B_NOT_A; // EA
    m68k_src_b=LOBYTE(r[PARAM_N]);
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_B&=m68k_src_b;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_B;
    CHECK_IOW_B;
  }
}


void                             m68k_and_w_from_dN_or_exg_like(){
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
  case BITS_543_000: //EXG D D
    PREFETCH_IRC; //np
    INSTRUCTION_TIME(2); //n
    compare_buffer=r[PARAM_N];
    r[PARAM_N]=r[PARAM_M];
    r[PARAM_M]=compare_buffer;
    break;
  case BITS_543_001: //EXG A A
    PREFETCH_IRC; //np
    INSTRUCTION_TIME(2); //n
    compare_buffer=areg[PARAM_N];
    areg[PARAM_N]=areg[PARAM_M];
    areg[PARAM_M]=compare_buffer;
    break;
  default://and.w from dn
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
    CHECK_READ=true;
    m68k_GET_DEST_W_NOT_A; //EA
    m68k_src_w=LOWORD(r[PARAM_N]);
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; // nw
    m68k_DEST_W&=m68k_src_w;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_W;
    CHECK_IOW_W;
  }
}


void                             m68k_and_l_from_dN_or_exg_unlike(){
  switch(ir&BITS_543){
  case BITS_543_000:
    m68k_unrecognised();
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
    PREFETCH_IRC; //np
    INSTRUCTION_TIME(2); //n
    compare_buffer=areg[PARAM_M];
    areg[PARAM_M]=r[PARAM_N];
    r[PARAM_N]=compare_buffer;
    break;
  default:// and.l from dn
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
    CHECK_READ=true;
    m68k_GET_DEST_L_NOT_A; //EA
    m68k_src_l=r[PARAM_N];
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; //nw nW
    m68k_DEST_L&=m68k_src_l;
    SR_CLEAR(SR_Z+SR_N+SR_V+SR_C);
    SR_CHECK_Z_AND_N_L;
    CHECK_IOW_L;
  }
}


void                              m68k_muls(){
  // see mulu for YACHT chart
  m68k_GET_SOURCE_W_NOT_A; // EA
  PREFETCH_IRC; // np 
/*
  .for MULS 'm' = concatenate the 16-bit pointed by <ea> with a zero as the LSB
   'm' is the resultant number of 10 or 01 patterns in the 17-bit source.
    - Best case : 38 cycles with $0 or $FFFF
    - Worst case : 70 cycles with $5555
*/

  INSTRUCTION_TIME(34); //TODO blitter? 
  int LastLow=0;
  int Val=WORD(m68k_src_w);
  for (int n=0;n<16;n++){
    if ((Val & 1)!=LastLow) INSTRUCTION_TIME(2);//n*
    LastLow=(Val & 1);
    Val>>=1;
  }
  m68k_dest=&(r[PARAM_N]);
  m68k_DEST_L=((signed long)((signed short)LOWORD(r[PARAM_N])))*((signed long)((signed short)m68k_src_w));
  SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
  SR_CHECK_Z_AND_N_L;
}


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


void                              m68k_add_b_to_dN(){
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
  m68k_GET_SOURCE_B_NOT_A; //EA
  PREFETCH_IRC; //np
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_B;
  m68k_DEST_B+=m68k_src_b;
  SR_ADD_B;
}


void                             m68k_add_w_to_dN(){ // add.w ea,dn or adda.w ea,an
/*
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
  m68k_GET_SOURCE_W; //EA  //A is allowed
  PREFETCH_IRC; // np
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_W;
  m68k_DEST_W+=m68k_src_w;
  SR_ADD_W;
}


void                             m68k_add_l_to_dN(){ // add.l ea,dn or adda.l ea,an
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
  m68k_GET_SOURCE_L; //EA  //A is allowed
  PREFETCH_IRC;//np
  if(SOURCE_IS_REGISTER_OR_IMMEDIATE)
    INSTRUCTION_TIME(4); //nn
  else 
    INSTRUCTION_TIME(2); //n
  m68k_dest=&r[PARAM_N];
  m68k_old_dest=m68k_DEST_L;
  m68k_DEST_L+=m68k_src_l;
  SR_ADD_L;
}


void                             m68k_adda_w(){
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
  m68k_GET_SOURCE_W; // EA
  m68k_src_l=(signed long)((signed short)m68k_src_w);
  PREFETCH_IRC; // np
  INSTRUCTION_TIME(4); // nn
  areg[PARAM_N]+=m68k_src_l;
}


void                              m68k_add_b_from_dN(){
  switch(ir&BITS_543){
  case BITS_543_000:
  case BITS_543_001:
//  ADDX.B
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
*/
    if((ir&BITS_543)==BITS_543_000){
/*
Dy,Dx :           |                 |
  .B or .W :      |  4(1/0)         |                               np       
*/
      m68k_src_b=LOBYTE(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .B or .W :      | 18(3/1)         |              n nr    nr       np nw       
*/
      INSTRUCTION_TIME(2); // n
      areg[PARAM_M]--;
      if(PARAM_M==7)
        areg[PARAM_M]--;
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ; //nr
      m68k_src_b=m68k_peek(abus);
      areg[PARAM_N]--;
      if(PARAM_N==7)
        areg[PARAM_N]--;
      CHECK_READ=true;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ; //nr
      m68k_SET_DEST_B(abus);
    }
    m68k_old_dest=m68k_DEST_B;
    PREFETCH_IRC; //np
    if((ir&BITS_543)==BITS_543_001)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_B+=m68k_src_b;
    if(sr&SR_X)m68k_DEST_B++;
    SR_ADDX_B;
    CHECK_IOW_B;
    break;

  default:
/* ADD.B
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
    m68k_src_b=LOBYTE(r[PARAM_N]);
    CHECK_READ=true;
    m68k_GET_DEST_B_NOT_A; // EA
    m68k_old_dest=m68k_DEST_B;
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_B+=m68k_src_b;
    SR_ADD_B;
    CHECK_IOW_B;
  }
}


void                              m68k_add_w_from_dN(){
  switch(ir&BITS_543){
//  ADDX.W
  case BITS_543_000:
  case BITS_543_001:
    if((ir&BITS_543)==BITS_543_000){
/*
Dy,Dx :           |                 |
  .B or .W :      |  4(1/0)         |                               np       
*/
      m68k_src_w=LOWORD(r[PARAM_M]);
      m68k_dest=&(r[PARAM_N]);
    }else{
/*
-(Ay),-(Ax) :     |                 |
  .B or .W :      | 18(3/1)         |              n nr    nr       np nw       
*/
      INSTRUCTION_TIME(2); // n
      areg[PARAM_M]-=2; // or after peek? TODO
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ; // nr
      m68k_src_w=m68k_dpeek(abus);
      CHECK_READ=true;
      areg[PARAM_N]-=2;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ; // nr
      m68k_SET_DEST_W(abus);
    }
    m68k_old_dest=m68k_DEST_W;
    PREFETCH_IRC; //np
    if((ir&BITS_543)==BITS_543_001)
    {
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nw
    }
    m68k_DEST_W+=m68k_src_w;
    if(sr&SR_X)m68k_DEST_W++;
    SR_ADDX_W;
    CHECK_IOW_W;
    break;

  default:// ADD.W Dn,<EA>
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
    m68k_src_w=LOWORD(r[PARAM_N]);
    CHECK_READ=true;
    m68k_GET_DEST_W_NOT_A; // EA
    m68k_old_dest=m68k_DEST_W;
    PREFETCH_IRC; //np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE; //nw
    m68k_DEST_W+=m68k_src_w;
    SR_ADD_W;
    CHECK_IOW_W;
  }
}


void                              m68k_add_l_from_dN(){ //SS +ADDX
  switch (ir&BITS_543){ //ADDX.L
  case BITS_543_000: 
  case BITS_543_001: 
/*
------------------------------------------------------------------------------- 
                  |    Exec Time    |               Data Bus Usage              
    ADDX, SUBX    |      INSTR      |                  INSTR                    
------------------+-----------------+------------------------------------------ 
*/
    if((ir&BITS_543)==BITS_543_000){
/*
Dy,Dx :           |                 |
  .L :            |  8(1/0)         |                               np       nn 
*/
      PREFETCH_IRC; // np
      INSTRUCTION_TIME(4); //nn
      m68k_src_l=r[PARAM_M];
      m68k_dest=&(r[PARAM_N]);
    }else{ //SS -(An), -(An)
/*
-(Ay),-(Ax) :     |                 |
  .L :            | 30(5/2)         |              n nr nR nr nR nw np    nW    
*/
      PREFETCH_CLASS(1);
      INSTRUCTION_TIME(2); //n
      areg[PARAM_M]-=4;
      abus=areg[PARAM_M];
      CPU_ABUS_ACCESS_READ_L; // nr nR
      m68k_src_l=m68k_lpeek(abus);
      CHECK_READ=true;
      areg[PARAM_N]-=4;
      abus=areg[PARAM_N];
      CPU_ABUS_ACCESS_READ_L; // nr nR
      m68k_SET_DEST_L(abus);
      CPU_ABUS_ACCESS_WRITE; //nw
      PREFETCH_IRC; //np 
#if defined(SSE_CPU_RESTORE_ABUS1)
      abus=dest_addr; 
#endif
      CPU_ABUS_ACCESS_WRITE; //nW
    }
    m68k_old_dest=m68k_DEST_L;
    m68k_DEST_L+=m68k_src_l;
    if(sr&SR_X)
      m68k_DEST_L++; 
    SR_ADDX_L;
    CHECK_IOW_L;
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
    m68k_src_l=r[PARAM_N];
    m68k_GET_DEST_L_NOT_A; //EA
    m68k_old_dest=m68k_DEST_L;
    PREFETCH_IRC; // np
#if defined(SSE_CPU_RESTORE_ABUS1)
    abus=dest_addr; 
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nw nW
    m68k_DEST_L+=m68k_src_l;
    SR_ADD_L;
    CHECK_IOW_L;
  }
}


void                             m68k_adda_l(){
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
*/
  m68k_GET_SOURCE_L;  // EA
  PREFETCH_IRC;  //np
  if (SOURCE_IS_REGISTER_OR_IMMEDIATE)
    INSTRUCTION_TIME(4); //nn
  else
    INSTRUCTION_TIME(2); //n
  areg[PARAM_N]+=m68k_src_l;
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
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; // EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); //n* n
  //limit to 32bit but don't change timing, the CPU isn't optimised for that!
  if(m68k_src_w>31)
    m68k_src_w=31; 
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
}


void                             m68k_lsr_b_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; // EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
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
}

void                             m68k_roxr_b_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(BITTEST(sr,SR_X_BIT))
    BITSET(sr,SR_C_BIT);
#else
  if(sr&SR_X)SR_SET(SR_C);
#endif
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
}


void                             m68k_ror_b_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_B&1;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned char*)m68k_dest)>>=1;if(old_x)m68k_DEST_B|=MSB_B;
  }
  SR_CHECK_Z_AND_N_B;
}


void                              m68k_asr_w_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  if(m68k_src_w>31)
    m68k_src_w=31;
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
}


void                             m68k_lsr_w_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  if(m68k_src_w>31)
    m68k_src_w=31;
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
}


void                             m68k_roxr_w_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(BITTEST(sr,SR_X_BIT))
    BITSET(sr,SR_C_BIT);  
#else
  if(sr&SR_X)SR_SET(SR_C);
#endif
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
}


void                             m68k_ror_w_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_W&1;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned short*)m68k_dest)>>=1;if(old_x)m68k_DEST_W|=MSB_W;
  }
  SR_CHECK_Z_AND_N_W;
}


void                              m68k_asr_l_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
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
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
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
}


void                             m68k_roxr_l_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(BITTEST(sr,SR_X_BIT))
    BITSET(sr,SR_C_BIT);
#else
  if(sr&SR_X)SR_SET(SR_C);
#endif
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
}


void                             m68k_ror_l_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_L&1;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned long*)m68k_dest)>>=1;if(old_x)m68k_DEST_L|=MSB_L;
  }
  SR_CHECK_Z_AND_N_L;
}


void                              m68k_asl_b_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  if (m68k_src_w>31) 
    m68k_src_w=31;
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
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_V_BIT);
#else
        SR_SET(SR_V);
#endif
      }
    }else if(m68k_DEST_B){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_V_BIT);
#else
      SR_SET(SR_V);
#endif
    }
  }
  *((signed char*)m68k_dest)<<=m68k_src_w;
  SR_CHECK_Z_AND_N_B;
}


void                             m68k_lsl_b_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  if(m68k_src_w>31)
    m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
#if defined(SSE_VC_INTRINSICS_390E)
    BITRESET(sr,SR_X_BIT);
#else
    SR_CLEAR(SR_X);
#endif
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
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(BITTEST(sr,SR_X_BIT))
    BITSET(sr,SR_C_BIT);
#else
  if(sr&SR_X)SR_SET(SR_C);
#endif
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
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_B&MSB_B;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned char*)m68k_dest)<<=1;if(old_x)m68k_DEST_B|=1;
  }
  SR_CHECK_Z_AND_N_B;
}


void                              m68k_asl_w_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  if(m68k_src_w>31)
    m68k_src_w=31;
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
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_V_BIT);
#else
        SR_SET(SR_V);
#endif
      }
    }else if(m68k_DEST_W){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_V_BIT);
#else
      SR_SET(SR_V);
#endif
    }
  }
  *((signed short*)m68k_dest)<<=m68k_src_w;
  SR_CHECK_Z_AND_N_W;
}


void                             m68k_lsl_w_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  if(m68k_src_w>31)
    m68k_src_w=31;
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
#if defined(SSE_VC_INTRINSICS_390E)
    BITRESET(sr,SR_X_BIT);
#else
    SR_CLEAR(SR_X);
#endif
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
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(BITTEST(sr,SR_X_BIT))
    BITSET(sr,SR_C_BIT);
#else
  if(sr&SR_X)SR_SET(SR_C);
#endif
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
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+2); // n* n
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_W&MSB_W;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned short*)m68k_dest)<<=1;if(old_x)m68k_DEST_W|=1;
  }
  SR_CHECK_Z_AND_N_W;
}


void                              m68k_asl_l_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
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
#if defined(SSE_VC_INTRINSICS_390E)
        BITSET(sr,SR_V_BIT);
#else
        SR_SET(SR_V);
#endif
      }
    }else if(m68k_DEST_L){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_V_BIT);
#else
      SR_SET(SR_V);
#endif
    }
  }
  *((signed long*)m68k_dest)<<=m68k_src_w;
  if(m68k_src_w>31)m68k_DEST_L=0;
  SR_CHECK_Z_AND_N_L;
}


void                             m68k_lsl_l_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  if(m68k_src_w){
#if defined(SSE_VC_INTRINSICS_390E)
    BITRESET(sr,SR_X_BIT);
#else
    SR_CLEAR(SR_X);
#endif
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
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(BITTEST(sr,SR_X_BIT))
    BITSET(sr,SR_C_BIT);
#else
  if(sr&SR_X)SR_SET(SR_C);
#endif
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
}


void                             m68k_rol_l_to_dM(){
  m68k_BIT_SHIFT_TO_dM_GET_SOURCE; //EA
  PREFETCH_IRC; //np
  INSTRUCTION_TIME(2*m68k_src_w+4); // n* nn
  m68k_dest=&(r[PARAM_M]);
  SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
  for(int n=0;n<m68k_src_w;n++){
    bool old_x=m68k_DEST_L&MSB_L;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned long*)m68k_dest)<<=1;if(old_x)m68k_DEST_L|=1;
  }
  SR_CHECK_Z_AND_N_L;
}


void                              m68k_bit_shift_right_to_mem(){
  // check illegal before get EA
  switch(ir&BITS_ba9){
  case BITS_ba9_000:
  case BITS_ba9_001:
  case BITS_ba9_010:
  case BITS_ba9_011:
    break;
  default:
    m68k_unrecognised();
  }
  CHECK_READ=true;
  m68k_GET_DEST_W_NOT_A_OR_D; // EA
  PREFETCH_IRC; //np
  switch(ir&BITS_ba9){
  case BITS_ba9_000:
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
    if(m68k_DEST_W&1){
      SR_SET(SR_C+SR_X);
    }
    *((signed short*)m68k_dest)>>=1;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  case BITS_ba9_001:
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
    if(m68k_DEST_W&1){
      SR_SET(SR_C+SR_X);
    }
    *((unsigned short*)m68k_dest)>>=1;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  case BITS_ba9_010:{
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
  if(BITTEST(sr,SR_X_BIT))
    BITSET(sr,SR_C_BIT);
#else
    if(sr&SR_X)SR_SET(SR_C);
#endif
    bool old_x=(sr&SR_X);
    if(m68k_DEST_W&1){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned short*)m68k_dest)>>=1;if(old_x)m68k_DEST_W|=MSB_W;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  }case BITS_ba9_011:{
    SR_CLEAR(SR_N+SR_V+SR_Z);
    bool old_x=m68k_DEST_W&1;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned short*)m68k_dest)>>=1;if(old_x)m68k_DEST_W|=MSB_W;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  }
#if defined(SSE_DEBUG)  
  default:
    ASSERT(0);
    break;
#endif
  }
#if defined(SSE_CPU_RESTORE_ABUS1)
  abus=dest_addr; 
#endif
  CPU_ABUS_ACCESS_WRITE; //nw
#if defined(SSE_BUGFIX_393C)
  CHECK_IOW_W; //after we count timing :)
#endif
}


void                              m68k_bit_shift_left_to_mem(){
  // check illegal before get EA
  switch(ir&BITS_ba9){
  case BITS_ba9_000:
  case BITS_ba9_001:
  case BITS_ba9_010:
  case BITS_ba9_011:
    break;
  default:
    m68k_unrecognised();
  }
  CHECK_READ=true;
  m68k_GET_DEST_W_NOT_A_OR_D; // EA
  PREFETCH_IRC; //np
  switch(ir&BITS_ba9){
  case BITS_ba9_000: //asl
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
//    if( m68k_DEST_W&(WORD)( MSB_W>>(m68k_src_w-1) )  ){
    if( m68k_DEST_W&(WORD)( MSB_W )  ){
      SR_SET(SR_C+SR_X);
    }
    if((m68k_DEST_W&0xc000)==0x8000 || (m68k_DEST_W&0xc000)==0x4000){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_V_BIT);
#else
      SR_SET(SR_V);
#endif
    }
    *((signed short*)m68k_dest)<<=1;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  case BITS_ba9_001:
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C+SR_X);
    if(m68k_DEST_W&MSB_W){
      SR_SET(SR_C+SR_X);
    }
    *((unsigned short*)m68k_dest)<<=1;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  case BITS_ba9_010:{
    SR_CLEAR(SR_N+SR_V+SR_Z+SR_C);
#if defined(SSE_VC_INTRINSICS_390E)
    if(BITTEST(sr,SR_X_BIT))
      BITSET(sr,SR_C_BIT);
#else
    if(sr&SR_X)SR_SET(SR_C);
#endif
    bool old_x=(sr&SR_X);
    if(m68k_DEST_W&MSB_W){
      SR_SET(SR_X+SR_C)
    }else{
      SR_CLEAR(SR_X+SR_C)
    }
    *((unsigned short*)m68k_dest)<<=1;if(old_x)m68k_DEST_W|=1;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  }case BITS_ba9_011:{
    SR_CLEAR(SR_N+SR_V+SR_Z);
    bool old_x=m68k_DEST_W&MSB_W;
    if(old_x){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_C_BIT);
#else
      SR_SET(SR_C)
#endif
    }else{
#if defined(SSE_VC_INTRINSICS_390E)
      BITRESET(sr,SR_C_BIT);
#else
      SR_CLEAR(SR_C)
#endif
    }
    *((unsigned short*)m68k_dest)<<=1;if(old_x)m68k_DEST_W|=1;
    SR_CHECK_Z_AND_N_W;
#if !defined(SSE_BUGFIX_393C)
    CHECK_IOW_W;
#endif
    break;
  }
#if defined(SSE_DEBUG)    
  default:
    ASSERT(0);
    break;
#endif
  }
#if defined(SSE_CPU_RESTORE_ABUS1)
  abus=dest_addr; 
#endif
  CPU_ABUS_ACCESS_WRITE; //nw
#if defined(SSE_BUGFIX_393C)
  CHECK_IOW_W;
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

/*
Prefetch (ijor)
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


/*
Timings (YACHT)

*******************************************************************************
  Line 0001         &          Line 0010          &          Line 0011
    MOVE.B                       MOVE.L, MOVEA.L               MOVE.W, MOVEA.W
*******************************************************************************

-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
       MOVE       |      INSTR      |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea>,Dn :         |                 |               |
  .B or .W :      |                 |               |
    Dn            |  4(1/0)         |               |               np		   
    An            |  4(1/0)         |               |               np		   
    (An)          |  8(2/0)         |            nr |               np		   
    (An)+         |  8(2/0)         |            nr |               np		   
    -(An)         | 10(2/0)         | n          nr |               np		   
    (d16,An)      | 12(3/0)         |      np    nr |               np		   
    (d8,An,Xn)    | 14(3/0)         | n    np    nr |               np		   
    (xxx).W       | 12(3/0)         |      np    nr |               np		   
    (xxx).L       | 16(4/0)         |   np np    nr |               np		 
    #<data>       |  8(2/0)         |      np       |               np		 
  .L :            |                 |               |            
    Dn            |  4(1/0)         |               |               np		     
    An            |  4(1/0)         |               |               np		    
    (An)          | 12(3/0)         |         nR nr |               np		     
    (An)+         | 12(3/0)         |         nR nr |               np		      
    -(An)         | 14(3/0)         | n       nR nr |               np		      
    (d16,An)      | 16(4/0)         |      np nR nr |               np		      
    (d8,An,Xn)    | 18(4/0)         | n    np nR nr |               np		      
    (xxx).W       | 16(4/0)         |      np nR nr |               np		      
    (xxx).L       | 20(5/0)         |   np np nR nr |               np		      
    #<data>       | 12(3/0)         |   np np       |               np		      
<ea>,(An) :       |                 |               |
  .B or .W :      |                 |               |
    Dn            |  8(1/1)         |               |            nw np          
    An            |  8(1/1)         |               |            nw np          
    (An)          | 12(2/1)         |            nr |            nw np          
    (An)+         | 12(2/1)         |            nr |            nw np          
    -(An)         | 14(2/1)         | n          nr |            nw np          
    (d16,An)      | 16(3/1)         |      np    nr |            nw np          
    (d8,An,Xn)    | 18(3/1)         | n    np    nr |            nw np          
    (xxx).W       | 16(3/1)         |      np    nr |            nw np          
    (xxx).L       | 20(4/1)         |   np np    nr |            nw np          
    #<data>       | 12(2/1)         |      np       |            nw np          
  .L :            |                 |               |
    Dn            | 12(1/2)         |               |         nW nw np		 
    An            | 12(1/2)         |               |         nW nw np		 
    (An)          | 20(3/2)         |         nR nr |         nW nw np		 
    (An)+         | 20(3/2)         |         nR nr |         nW nw np		 
    -(An)         | 22(3/2)         | n       nR nr |         nW nw np		 
    (d16,An)      | 24(4/2)         |      np nR nr |         nW nw np		 
    (d8,An,Xn)    | 26(4/2)         | n    np nR nr |         nW nw np		 
    (xxx).W       | 24(4/2)         |      np nR nr |         nW nw np		 
    (xxx).L       | 28(5/2)         |   np np nR nr |         nW nw np		 
    #<data>       | 20(3/2)         |   np np       |         nW nw np		 
<ea>,(An)+ :      |                 |               |
  .B or .W :      |                 |               |
    Dn            |  8(1/1)         |               |            nw np          
    An            |  8(1/1)         |               |            nw np          
    (An)          | 12(2/1)         |            nr |            nw np          
    (An)+         | 12(2/1)         |            nr |            nw np          
    -(An)         | 14(2/1)         | n          nr |            nw np          
    (d16,An)      | 16(3/1)         |      np    nr |            nw np          
    (d8,An,Xn)    | 18(3/1)         | n    np    nr |            nw np          
    (xxx).W       | 16(3/1)         |      np    nr |            nw np          
    (xxx).L       | 20(4/1)         |   np np    nr |            nw np          
    #<data>       | 12(2/1)         |      np       |            nw np          
  .L :            |                 |               |                           
    Dn            | 12(1/2)         |               |         nW nw np          
    An            | 12(1/2)         |               |         nW nw np          
    (An)          | 20(3/2)         |         nR nr |         nW nw np          
    (An)+         | 20(3/2)         |         nR nr |         nW nw np          
    -(An)         | 22(3/2)         | n       nR nr |         nW nw np          
    (d16,An)      | 24(4/2)         |      np nR nr |         nW nw np          
    (d8,An,Xn)    | 26(4/2)         | n    np nR nr |         nW nw np          
    (xxx).W       | 24(4/2)         |      np nR nr |         nW nw np          
    (xxx).L       | 28(5/2)         |   np np nR nr |         nW nw np          
    #<data>       | 20(3/2)         |   np np       |         nW nw np          
<ea>,-(An) :      |                 |               |
  .B or .W :      |                 |               |
    Dn            |  8(1/1)         |               |                  np nw    
    An            |  8(1/1)         |               |                  np nw    
    (An)          | 12(2/1)         |            nr |                  np nw    
    (An)+         | 12(2/1)         |            nr |                  np nw    
    -(An)         | 14(2/1)         | n          nr |                  np nw    
    (d16,An)      | 16(3/1)         |      np    nr |                  np nw    
    (d8,An,Xn)    | 18(3/1)         | n    np    nr |                  np nw    
    (xxx).W       | 16(3/1)         |      np    nr |                  np nw    
    (xxx).L       | 20(4/1)         |   np np    nr |                  np nw    
    #<data>       | 12(2/1)         |      np       |                  np nw    
  .L :            |                 |               |                           
    Dn            | 12(1/2)         |               |                  np nw nW 
    An            | 12(1/2)         |               |                  np nw nW 
    (An)          | 20(3/2)         |         nR nr |                  np nw nW 
    (An)+         | 20(3/2)         |         nR nr |                  np nw nW 
    -(An)         | 22(3/2)         | n       nR nr |                  np nw nW 
    (d16,An)      | 24(4/2)         |      np nR nr |                  np nw nW 
    (d8,An,Xn)    | 26(4/2)         | n    np nR nr |                  np nw nW 
    (xxx).W       | 24(4/2)         |      np nR nr |                  np nw nW 
    (xxx).L       | 28(5/2)         |   np np nR nr |                  np nw nW 
    #<data>       | 20(3/2)         |   np np       |                  np nw nW 
<ea>,(d16,An) :   |                 |               |
  .B or .W :      |                 |               |
    Dn            | 12(2/1)         |               |      np    nw np        
    An            | 12(2/1)         |               |      np    nw np		      
    (An)          | 16(3/1)         |            nr |      np    nw np		      
    (An)+         | 16(3/1)         |            nr |      np    nw np		      
    -(An)         | 18(3/1)         | n          nr |      np    nw np		      
    (d16,An)      | 20(4/1)         |      np    nr |      np    nw np		      
    (d8,An,Xn)    | 22(4/1)         | n    np    nr |      np    nw np		      
    (xxx).W       | 20(4/1)         |      np    nr |      np    nw np		      
    (xxx).L       | 24(5/1)         |   np np    nr |      np    nw np		      
    #<data>       | 16(3/1)         |      np       |      np    nw np		      
  .L :            |                 |               |
    Dn            | 16(2/2)         |               |      np nW nw np		      
    An            | 16(2/2)         |               |      np nW nw np		      
    (An)          | 24(4/2)         |         nR nr |      np nW nw np          
    (An)+         | 24(4/2)         |         nR nr |      np nW nw np          
    -(An)         | 26(4/2)         | n       nR nr |      np nW nw np          
    (d16,An)      | 28(5/2)         |      np nR nr |      np nW nw np          
    (d8,An,Xn)    | 30(5/2)         | n    np nR nr |      np nW nw np          
    (xxx).W       | 28(5/2)         |      np nR nr |      np nW nw np          
    (xxx).L       | 32(6/2)         |   np np nR nr |      np nW nw np     
    #<data>       | 24(4/2)         |   np np       |      np nW nw np		   
<ea>,(d8,An,Xn) : |                 |               |
  .B or .W :      |                 |               |
    Dn            | 14(2/1)         |               | n    np    nw np		   
    An            | 14(2/1)         |               | n    np    nw np		   
    (An)          | 18(3/1)         |            nr | n    np    nw np		   
    (An)+         | 18(3/1)         |            nr | n    np    nw np		   
    -(An)         | 20(3/1)         | n          nr | n    np    nw np		   
    (d16,An)      | 22(4/1)         |      np    nr | n    np    nw np		   
    (d8,An,Xn)    | 24(4/1)         | n    np    nr | n    np    nw np		   
    (xxx).W       | 22(4/1)         |      np    nr | n    np    nw np		   
    (xxx).L       | 26(5/1)         |   np np    nr | n    np    nw np		   
    #<data>       | 18(3/1)         |      np       | n    np    nw np		   
  .L :            |                 |               |
    Dn            | 18(2/2)         |               | n    np nW nw np		   
    An            | 18(2/2)         |               | n    np nW nw np		   
    (An)          | 26(4/2)         |         nR nr | n    np nW nw np          
    (An)+         | 26(4/2)         |         nR nr | n    np nW nw np          
    -(An)         | 28(4/2)         | n       nR nr | n    np nW nw np          
    (d16,An)      | 30(5/2)         |      np nR nr | n    np nW nw np          
    (d8,An,Xn)    | 32(5/2)         | n    np nR nr | n    np nW nw np          
    (xxx).W       | 30(5/2)         |      np nR nr | n    np nW nw np          
    (xxx).L       | 34(6/2)         |   np np nR nr | n    np nW nw np      
    #<data>       | 26(4/2)         |   np np       | n    np nW nw np		    
<ea>,(xxx).W :    |                 |               |
  .B or .W :      |                 |               |
    Dn            | 12(2/1)         |               |      np    nw np		    
    An            | 12(2/1)         |               |      np    nw np		    
    (An)          | 16(3/1)         |            nr |      np    nw np		    
    (An)+         | 16(3/1)         |            nr |      np    nw np		    
    -(An)         | 18(3/1)         | n          nr |      np    nw np		    
    (d16,An)      | 20(4/1)         |      np    nr |      np    nw np		    
    (d8,An,Xn)    | 22(4/1)         | n    np    nr |      np    nw np		    
    (xxx).W       | 20(4/1)         |      np    nr |      np    nw np		    
    (xxx).L       | 24(5/1)         |   np np    nr |      np    nw np		    
    #<data>       | 16(3/1)         |      np       |      np    nw np		    
  .L :            |                 |               |
    Dn            | 16(2/2)         |               |      np nW nw np		    
    An            | 16(2/2)         |               |      np nW nw np		    
    (An)          | 24(4/2)         |         nR nr |      np nW nw np      
    (An)+         | 24(4/2)         |         nR nr |      np nW nw np          
    -(An)         | 26(4/2)         | n       nR nr |      np nW nw np          
    (d16,An)      | 28(5/2)         |      np nR nr |      np nW nw np          
    (d8,An,Xn)    | 30(5/2)         | n    np nR nr |      np nW nw np          
    (xxx).W       | 28(5/2)         |      np nR nr |      np nW nw np          
    (xxx).L       | 32(6/2)         |   np np nR nr |      np nW nw np      
    #<data>       | 24(4/2)         |   np np       |      np nW nw np		    
<ea>,(xxx).L :    |                 |               |
  .B or .W :      |                 |               |
    Dn            | 16(3/1)         |               |   np np    nw np		    
    An            | 16(3/1)         |               |   np np    nw np		    
    (An)          | 20(4/1)         |            nr |      np    nw np np	  
    (An)+         | 20(4/1)         |            nr |      np    nw np np	  
    -(An)         | 22(4/1)         | n          nr |      np    nw np np	  
    (d16,An)      | 24(5/1)         |      np    nr |      np    nw np np	  
    (d8,An,Xn)    | 26(5/1)         | n    np    nr |      np    nw np np	  
    (xxx).W       | 24(5/1)         |      np    nr |      np    nw np np	  
    (xxx).L       | 28(6/1)         |   np np    nr |      np    nw np np	  
    #<data>       | 20(4/1)         |      np       |   np np    nw np		    
  .L :            |                 |               |
    Dn            | 20(3/2)         |               |   np np nW nw np		    
    An            | 20(3/2)         |               |   np np nW nw np		    
    (An)          | 28(5/2)         |         nR nr |      np nW nw np np   
    (An)+         | 28(5/2)         |         nR nr |      np nW nw np np   
    -(An)         | 30(5/2)         | n       nR nr |      np nW nw np np       
    (d16,An)      | 32(6/2)         |      np nR nr |      np nW nw np np       
    (d8,An,Xn)    | 34(6/2)         | n    np nR nr |      np nW nw np np       
    (xxx).W       | 32(6/2)         |      np nR nr |      np nW nw np np       
    (xxx).L       | 36(7/2)         |   np np nR nr |      np nW nw np np       
    #<data>       | 28(5/2)         |   np np       |   np np nW nw np          

-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
      MOVEA       |      INSTR      |  1st OP (ea)  |          INSTR
------------------+-----------------+---------------+--------------------------
<ea>,An :         |                 |               |
  .W :            |                 |               |
    Dn            |  4(1/0)         |               |               np		      
    An            |  4(1/0)         |               |               np		      
    (An)          |  8(2/0)         |            nr |               np        
    (An)+         |  8(2/0)         |            nr |               np        
    -(An)         | 10(2/0)         | n          nr |               np        
    (d16,An)      | 12(3/0)         |      np    nr |               np        
    (d8,An,Xn)    | 14(3/0)         | n    np    nr |               np        
    (xxx).W       | 12(3/0)         |      np    nr |               np        
    (xxx).L       | 16(4/0)         |   np np    nr |               np        
    #<data>       |  8(2/0)         |      np       |               np		      
  .L :            |                 |               |
    Dn            |  4(1/0)         |               |               np		      
    An            |  4(1/0)         |               |               np		      
    (An)          | 12(3/0)         |         nR nr |               np        
    (An)+         | 12(3/0)         |         nR nr |               np        
    -(An)         | 14(3/0)         | n       nR nr |               np          
    (d16,An)      | 16(4/0)         |      np nR nr |               np          
    (d8,An,Xn)    | 18(4/0)         | n    np nR nr |               np          
    (xxx).W       | 16(4/0)         |      np nR nr |               np        
    (xxx).L       | 20(5/0)         |   np np nR nr |               np        
    #<data>       | 12(3/0)         |   np np       |               np		      
*/

void m68k_0001() {  // move.b

  PREFETCH_CLASS(1); // default

  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis

  // Illegal instructions trigger trap1 during decoding (partial in Steem)
  if( (ir&BITS_876)==BITS_876_001 || (ir&BITS_876)==BITS_876_111
    && (ir&BITS_ba9)!=BITS_ba9_000 && (ir&BITS_ba9)!=BITS_ba9_001)
    m68k_unrecognised();

  // Source
  m68k_GET_SOURCE_B_NOT_A; // EA

  // Destination
  TRUE_PC=pc+2;
  if((ir&BITS_876)==BITS_876_000) // to Dn
  { 
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_B=m68k_src_b; // move completed
    SR_CHECK_Z_AND_N_B; // update flags
  }
  else //to memory
  {   
    switch(ir&BITS_876)
    {

    case BITS_876_010: // (An)
      abus=areg[PARAM_N];
      break;

    case BITS_876_011: // (An)+
      abus=areg[PARAM_N];
      UpdateAn=(PARAM_N==7)?2:1;
      break;

    case BITS_876_100: // -(An) np nw
      PREFETCH_CLASS(0); 
      PREFETCH_IRC; // np
      UpdateAn=(PARAM_N==7)?-2:-1;
      abus=areg[PARAM_N]+UpdateAn;
      break;

    case BITS_876_101: // (d16, An) // np    nw np
      CPU_ABUS_ACCESS_READ_FETCH; // np
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_876_110: // (d8, An, Xn) // n    np    nw np	
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_READ_FETCH; // np
      m68k_iriwo=m68k_fetchW();pc+=2; 
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;

    case BITS_876_111:

      switch (ir & BITS_ba9){

      case BITS_ba9_000: // (xxx).W // np    nw np
        CPU_ABUS_ACCESS_READ_FETCH; // np
        abus=0xffffff & (unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;

      case BITS_ba9_001: // (xxx).L 
        // register or immediate: np np    nw np
        // other:                 np    nw np np 
        CPU_ABUS_ACCESS_READ_FETCH; // np
        if(SOURCE_IS_REGISTER_OR_IMMEDIATE)
        {
          CPU_ABUS_ACCESS_READ_FETCH; // np
          TRUE_PC+=2; // move.b #0,0
        }
        else
          PREFETCH_CLASS(2);
        abus=m68k_fetchL() & 0xffffff;
        pc+=4;  
        break;

#if defined(SSE_DEBUG)
      default:
        ASSERT(0);
#endif
      }
    }
    // Set flags
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if(!m68k_src_b){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
    }
    if(m68k_src_b&MSB_B){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_N_BIT);
#else
      SR_SET(SR_N);
#endif
    }

#if defined(SSE_CPU_DATABUS)
    dbus|=m68k_src_b; // | or = ?
#endif

    CPU_ABUS_ACCESS_WRITE; // nw
    m68k_poke_abus(m68k_src_b); // could crash
    areg[PARAM_N]+=UpdateAn; // can be 0,-1,-2,+1,+2

    if(PREFETCH_CLASS_2) // some (xxx).L
    {
      REFETCH_IR; // np
      PREFETCH_IRC; // np
    }
  }// to memory

  if(PREFETCH_CLASS_1) // default; classes 0 & 2 already handled
    PREFETCH_IRC; // np
}


void m68k_0010() { //move.l

  PREFETCH_CLASS(1); // default

  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis

  // Illegal instructions trigger trap1 during decoding (partial in Steem)
  if( (ir&BITS_876)==BITS_876_111 && (ir&BITS_ba9)!=BITS_ba9_000
    && (ir&BITS_ba9)!=BITS_ba9_001 )
      m68k_unrecognised();

  // Source
  m68k_GET_SOURCE_L; // EA

  // Destination
  TRUE_PC=pc+2; // eg. War Heli move.l d5,(a1)
  if ((ir & BITS_876)==BITS_876_000) // Dn
  { 
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_L=m68k_src_l;
    SR_CHECK_Z_AND_N_L;
  }
  else if ((ir & BITS_876)==BITS_876_001) // An
    areg[PARAM_N]=m68k_src_l; // MOVEA
  else //to memory
  {   
    switch(ir&BITS_876)
    {

    case BITS_876_010: // (An)
      abus=areg[PARAM_N];
      break;

    case BITS_876_011: // (An)+
      abus=areg[PARAM_N];
      UpdateAn=4;
      break;

    case BITS_876_100: // -(An) // np nw nW
      PREFETCH_CLASS(0);
      PREFETCH_IRC; // np
      UpdateAn=-4;
      abus=areg[PARAM_N]+UpdateAn;
      break;

    case BITS_876_101: // (d16, An) //np nW nw np
      CPU_ABUS_ACCESS_READ_FETCH; // np
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_876_110: // (d8, An, Xn) // n    np nW nw np
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_READ_FETCH; // np
      m68k_iriwo=m68k_fetchW();pc+=2; 
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;

    case BITS_876_111:
      switch(ir&BITS_ba9){

      case BITS_ba9_000: // (xxx).W // np nW nw np
        CPU_ABUS_ACCESS_READ_FETCH; // np
        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;

      case BITS_ba9_001: // (xxx).L
        // register or immediate:  np np nW nw np
        // other:                  np nW nw np np (refetch IR)
        CPU_ABUS_ACCESS_READ_FETCH; // np
        if (SOURCE_IS_REGISTER_OR_IMMEDIATE) 
        {
          CPU_ABUS_ACCESS_READ_FETCH; // np
          TRUE_PC+=2; // move.l #0,0
        }
        else
          PREFETCH_CLASS(2);
        abus=m68k_fetchL()&0xffffff;
        pc+=4;  
        break;

#if defined(SSE_DEBUG)
      default:
        ASSERT(0);
#endif
      }
    }
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if(!m68k_src_l){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
    }
    if(m68k_src_l&MSB_L){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_N_BIT);
#else
      SR_SET(SR_N);
#endif
    }
#if defined(SSE_CPU_DATABUS)
    dbus=m68k_src_l&0xFFFF; // 2nd part?
#endif
    CPU_ABUS_ACCESS_WRITE_L; // nW nw
    m68k_lpoke_abus(m68k_src_l);
    areg[PARAM_N]+=UpdateAn; // can be 0,-4,+4
    if(PREFETCH_CLASS_2)
    {
      REFETCH_IR; // np
      PREFETCH_IRC; // np
    }
  }// to memory
  if(PREFETCH_CLASS_1) // default
      PREFETCH_IRC; // np
}


void m68k_0011() { //move.w

  PREFETCH_CLASS(1); // default

  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis

  // Illegal instructions trigger trap1 during decoding (partial in Steem)
  if( (ir&BITS_876)==BITS_876_111 && (ir&BITS_ba9)!=BITS_ba9_000
      && (ir&BITS_ba9)!=BITS_ba9_001)
      m68k_unrecognised();

  // Source
  m68k_GET_SOURCE_W; // EA

  // Destination
  TRUE_PC=pc+2;
  if ((ir & BITS_876)==BITS_876_000) // Dn
  {
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_W=m68k_src_w;
    SR_CHECK_Z_AND_N_W;
  }
  else if ((ir & BITS_876)==BITS_876_001) // An
    areg[PARAM_N]=(signed long)((signed short)m68k_src_w); // movea
  else //to memory
  {   
    switch (ir & BITS_876)
    {

    case BITS_876_010: // (An)
      abus=areg[PARAM_N];
      break;

    case BITS_876_011:  // (An)+
      abus=areg[PARAM_N];
      UpdateAn=2; // Beyond loader
      break;

    case BITS_876_100: // -(An)
      PREFETCH_CLASS(0);
      PREFETCH_IRC; // np
      UpdateAn=-2;
      abus=areg[PARAM_N]+UpdateAn;
      break;

    case BITS_876_101: // (d16, An) //np    nw np
      CPU_ABUS_ACCESS_READ_FETCH; //np
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;

    case BITS_876_110: // (d8, An, Xn) //n    np    nw np
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_READ_FETCH; // np
      m68k_iriwo=m68k_fetchW();pc+=2; 
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;

    case BITS_876_111:
      switch (ir & BITS_ba9){

      case BITS_ba9_000: // (xxx).W //np    nw np
        CPU_ABUS_ACCESS_READ_FETCH; // np
        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;

      case BITS_ba9_001: // (xxx).L
        // register or immediate: np np    nw np
        // other:                 np    nw np np (refetch IR)
        CPU_ABUS_ACCESS_READ_FETCH;
        if (SOURCE_IS_REGISTER_OR_IMMEDIATE) 
        {
          CPU_ABUS_ACCESS_READ_FETCH;
          TRUE_PC+=2; // move.w #0,0
        }
        else
          PREFETCH_CLASS(2);
        abus=m68k_fetchL()&0xffffff;
        pc+=4;  
        break;

#if defined(SSE_DEBUG)
      default:
        ASSERT(0);
#endif
      }
    }
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if (!m68k_src_w){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_Z_BIT);
#else
      SR_SET(SR_Z);
#endif
    }
    if (m68k_src_w & MSB_W){
#if defined(SSE_VC_INTRINSICS_390E)
      BITSET(sr,SR_N_BIT);
#else
      SR_SET(SR_N);
#endif
    }

    CPU_ABUS_ACCESS_WRITE; // nw
    m68k_dpoke_abus(m68k_src_w);

    areg[PARAM_N]+=UpdateAn; // can be 0,-2,+2

    if(PREFETCH_CLASS_2)
    {
      REFETCH_IR;
      PREFETCH_IRC;
    }
  }// to memory
  if(PREFETCH_CLASS_1)
      PREFETCH_IRC;
}


extern "C" void m68k_0100(){
  m68k_jump_line_4[(ir&(BITS_ba9|BITS_876))>>6]();
}


extern "C" void m68k_0101(){
  m68k_jump_line_5[(ir&BITS_876)>>6]();
}


extern "C" void m68k_0110(){  //bCC + BSR
  if (LOBYTE(ir)){ //8-bit displacement
    MEM_ADDRESS new_pc=(pc+(signed long)((signed char)LOBYTE(ir))) | pc_high_byte;
    if ((ir & 0xf00)==0x100){ //BSR
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        BSR       |      INSTR      |                   INSTR
------------------+-----------------+------------------------------------------
<label> :         |                 |
  .B .S or .W :   | 18(2/2)         |                 n    nS ns np np          
*/
      PREFETCH_CLASS(2);
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_WRITE_PUSH_L; // nS ns
      m68k_PUSH_L(PC32);
#if defined(SSE_BOILER_PSEUDO_STACK)
      Debug.PseudoStackPush(PC32);
#endif
#if defined(SSE_MMU_ROUNDING_BUS)
      abus=new_pc;
      
      m68k_READ_W(abus);
#else
      m68k_READ_W(new_pc); // Check for bus/address errors
#endif
      CPU_ABUS_ACCESS_READ_FETCH_L; // np np
      SET_PC(new_pc);
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
      if (m68k_CONDITION_TEST){ // branch taken
        INSTRUCTION_TIME(2); // n
#if defined(SSE_MMU_ROUNDING_BUS)
        abus=new_pc;
        m68k_READ_W(abus);
#else
        m68k_READ_W(new_pc); // Check for bus/address errors
#endif
        CPU_ABUS_ACCESS_READ_FETCH_L; // np np
        SET_PC(new_pc);
      }else{ //SS branch not taken
        INSTRUCTION_TIME(4); // nn
        PREFETCH_IRC; // np
      }
    }
  }else{

    if ((ir & 0xf00)==0x100){ //bsr.l //SS .W?
/*
-------------------------------------------------------------------------------
                  |    Exec Time    |               Data Bus Usage
        BSR       |      INSTR      |                   INSTR
------------------+-----------------+------------------------------------------
<label> :         |                 |
  .B .S or .W :   | 18(2/2)         |                 n    nS ns np np          
*/
      PREFETCH_CLASS(2);
      INSTRUCTION_TIME(2); // n
      CPU_ABUS_ACCESS_WRITE_PUSH_L; // nS ns
      m68k_PUSH_L(PC32+2);
#if defined(SSE_BOILER_PSEUDO_STACK)
      Debug.PseudoStackPush(PC32+2);
#endif
      MEM_ADDRESS new_pc=(pc+(signed long)((signed short)m68k_fetchW())) | pc_high_byte;
      // stacked pc is always instruction pc+2 due to prefetch (pc doesn't increase before new_pc is read)
#if defined(SSE_MMU_ROUNDING_BUS)
      abus=new_pc;
      m68k_READ_W(abus);
#else
      m68k_READ_W(new_pc); // Check for bus/address errors
#endif
      CPU_ABUS_ACCESS_READ_FETCH_L; // np np
      SET_PC(new_pc);      
    }else{ // Bcc.l //SS .W?
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
      // the word must be fetched anyway
      MEM_ADDRESS new_pc=(pc+(signed long)((signed short)m68k_fetchW())) | pc_high_byte;
      if (m68k_CONDITION_TEST){ // branch taken
        INSTRUCTION_TIME(2); //n
        // stacked pc is always instruction pc+2 due to prefetch (pc doesn't increase before new_pc is read)
#if defined(SSE_MMU_ROUNDING_BUS)
        abus=new_pc;
        m68k_READ_W(abus);
#else
        m68k_READ_W(new_pc); // Check for bus/address errors
#endif
        CPU_ABUS_ACCESS_READ_FETCH_L; // np np
        SET_PC(new_pc);
      }
      else // branch not taken
      {
        INSTRUCTION_TIME(4); // nn
        CPU_ABUS_ACCESS_READ_FETCH; // np
        pc+=2; 
        PREFETCH_IRC; // np
      }
    }
  }
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
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_L=(signed long)((signed char)LOBYTE(ir));
    PREFETCH_IRC; //np
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    SR_CHECK_Z_AND_N_L;
  }
}


extern "C" void m68k_1000(){ //or, div, sbcd
  m68k_jump_line_8[(ir&BITS_876)>>6]();
}


extern "C" void m68k_1001(){ //sub
  m68k_jump_line_9[(ir&BITS_876)>>6]();
}


extern "C" void m68k_1010() //line-a
{
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::EXCEPTION;
#endif
  pc-=2;  //pc not incremented for illegal instruction
//  log_write("CPU sees line-a instruction");
//  intercept_line_a();//SS doesn't exist

#ifdef SSE_DEBUG
#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK3 & OSD_CONTROL_STEBLT) // by default
    TRACE_OSD("LINE A %X",(IRD&0xF));
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("LINEA");
#endif
/*
B_WD            equ     +00     ; width of block in pixels                          
B_HT            equ     +02     ; height of block in pixels                         

PLANE_CT        equ     +04     ; number of consecutive planes to blt       {D}     

FG_COL          equ     +06     ; foreground color (logic op index:hi bit)  {D}     
BG_COL          equ     +08     ; background color (logic op index:lo bit)  {D}     
OP_TAB          equ     +10     ; logic ops for all fore and background combos      
S_XMIN          equ     +14     ; minimum X: source                                 
S_YMIN          equ     +16     ; minimum Y: source                                 
S_FORM          equ     +18     ; source form base address                          
S_NXWD          equ     +22     ; offset to next word in line  (in bytes)           
S_NXLN          equ     +24     ; offset to next line in plane (in bytes)           
S_NXPL          equ     +26     ; offset to next plane from start of current plane  

D_XMIN          equ     +28     ; minimum X: destination                            
D_YMIN          equ     +30     ; minimum Y: destination                            
D_FORM          equ     +32     ; destination form base address                     
D_NXWD          equ     +36     ; offset to next word in line  (in bytes)           
D_NXLN          equ     +38     ; offset to next line in plane (in bytes)           
D_NXPL          equ     +40     ; offset to next plane from start of current plane  

P_ADDR          equ     +42     ; address of pattern buffer   (0:no pattern) {D}
P_NXLN          equ     +46     ; offset to next line in pattern  (in bytes)    
P_NXPL          equ     +48     ; offset to next plane in pattern (in bytes)    
P_MASK          equ     +50     ; pattern index mask                            

*/
#define LOGSECTION LOGSECTION_BLITTER // by default
/*
  if((IRD&0xF)==7) //bitblt
  {
    MEM_ADDRESS p=areg[6];
  ///  TRACE_LOG("Line A biblt %dx%d %d planes fg %d bg %d op %X src %X x %d y %d p %d dst %X x %d y %d p %d pattern %X\n",DPEEK(p),DPEEK(p+2),DPEEK(p+4),DPEEK(p+6),DPEEK(p+8),LPEEK(p+10),LPEEK(p+18),DPEEK(p+22),DPEEK(p+24),DPEEK(p+26),LPEEK(p+32),DPEEK(p+36),DPEEK(p+38),DPEEK(p+40),LPEEK(p+42));
    // TODO, as is, can hang Steem
  }
  */
#undef LOGSECTION
#endif//dbg
  m68kTrapTiming();
  m68k_interrupt(LPEEK(BOMBS_LINE_A*4));
#if !defined(SSE_CPU_TRACE_LINE_A_F)
  m68k_do_trace_exception=0;
#endif
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::NORMAL;
#endif
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
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::EXCEPTION;
#endif
#if defined(SSE_CPU_LINE_F)
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
  m68kTrapTiming();
  m68k_interrupt(LPEEK(BOMBS_LINE_F*4));
#if !defined(SSE_CPU_TRACE_LINE_A_F)
  m68k_do_trace_exception=0;
#endif
#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::NORMAL;
#endif
  debug_check_break_on_irq(BREAK_IRQ_LINEF_IDX);
}

#include "cpuinit.cpp"
