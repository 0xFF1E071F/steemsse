// This file is compiled as a distinct module (resulting in an OBJ file)
#include "6301.h"
#include "SSE/SSE.h"
#include "SSE/SSEDebug.h"
#include "SSE/SSE6301.h"
#include "acia.h"

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
#include "SSE/SSEOption.h"

#ifndef WIN32
unsigned int _rotr(unsigned int Data, unsigned int Bits) {
  return ((Data >> Bits) | (Data << (32-Bits)));
}
unsigned int _rotl(unsigned int Data, unsigned int Bits) {
  return ((Data << Bits) | (Data >> (32-Bits)));
}
#endif
 
// functions & variables from Steem (declared as extern "C") there
extern void keyboard_buffer_write(unsigned char src) ;
extern int cpu_timer,cpu_cycles; // for debug
#define ABSOLUTE_CPU_TIME (cpu_timer-cpu_cycles)
#define act ABSOLUTE_CPU_TIME
extern unsigned char  stick[8]; // joysticks
// variables from Steem we declare here as 'C' linkage (easier than in Steem)
int ST_Key_Down[128]; // not better than what I envisioned but effective!
int mousek;
// our variables that Steem must see
int hd6301_completed_transmission_to_MC6850; // for sync

// additional variables for our module

unsigned char rec_byte;
#if defined(SS_IKBD_6301_RUN_IRQ_TO_END)

#define NOT_EXECUTING_INT 0
#define EXECUTING_INT 1
#define FINISHED_EXECUTING_INT -1

int ExecutingInt=NOT_EXECUTING_INT;
#endif
int Crashed6301=0;

#if defined(SS_IKBD_6301_CHECK_COMMANDS) 
int LoadingMemory=0; // ID command $20
int CustomPrgAddress=0; // parameters 1&2
int BytesToLoad=0; // parameter 3
int CurrentCommand=0;
int CurrentParameter=0;
int TotalParameters=0; // #parameters
#endif

// Debug facilities
// ASSERT
#if defined(_MSC_VER) && defined(_DEBUG)
#undef ASSERT
#define ASSERT(x) {if(!(x)) {TRACE("Assert failed: %s\n",#x);\
                   _asm{int 0x03}}}
#else 
#if !defined(NDEBUG)
#define ASSERT(x) {if(!(x)) TRACE("Assert failed: %s\n",#x);} //just a trace
#else
#define ASSERT(x) // release
#endif
#endif
// TRACE
#if defined(SS_DEBUG) 
// we use this trick because Trace is a struct function
void (*hd6301_trace)(char *fmt, ...);
#define TRACE hd6301_trace
#else
#define TRACE
#endif

// constructing our module (OBJ) the good old Steem way, hem

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IKBD

#define error TRACE //printf // saves headache
#define warning TRACE //printf 
// base
#if !defined(SS_IKBD_6301_DISABLE_CALLSTACK)
#include "callstac.c"
#endif
#include "cpu.c"
#include "fprinthe.c"
#include "memsetl.c"
#include "symtab.c"  
#include "tty.c"
// arch
// m68xx
#include "memory.c"
#include "opfunc.c" // will include alu.c (arithmetic/logic unit)
#include "reg.c"
// h6301
#include "instr.c"
#include "ireg.c"
#include "optab.c"
#include "sci.c"
#include "timer.c"

// Interface with Steem

hd6301_init() {
  return (int)mem_init();
}


hd6301_destroy() {
  TRACE("6301: destroy object\n");
  if(ram) 
    free(ram);
  ram=NULL;
#if !defined(SS_IKBD_6301_DISABLE_BREAKS)
  if(breaks) 
    free(breaks);
  breaks=NULL;
#endif
}


hd6301_reset(int Cold) {
  TRACE("6301 emu cpu reset\n");
  cpu_reset();

  hd6301_completed_transmission_to_MC6850=0;
  Crashed6301=0;
  iram[TRCSR]=0x20;
#if defined(SS_IKBD_6301_RUN_IRQ_TO_END)
  ExecutingInt=NOT_EXECUTING_INT;
#endif
  cpu_start(); // since we don't use the command.c file
}


hd6301_run_cycles(u_int cycles_to_run) {
  // Called by Steem to run some cycles, generally 64/scanline
  int pc;
  int cycles_run=0;
  int i=0;
#if defined(SS_IKBD_6301_ADJUST_CYCLES)
  static int cycles_to_give_back=0;
#endif
  int starting_cycles=cpu.ncycles;
  // make sure our 6301 is running OK
  if(!cpu_isrunning())
  {
    TRACE("6301 starting cpu\n");
    cpu_start();
  }
  if((iram[TRCSR]&1) 
#if defined(SS_ACIA_DOUBLE_BUFFER_TX)
    && !ACIA_IKBD.LineTxBusy
#endif
    )
  {
    TRACE("6301 waking up (PC %X cycles %d ACT %d)\n",reg_getpc(),cpu.ncycles,act);
    iram[TRCSR]&=~1; // ha! that's for Barbarian (& Obliterator, Froggies)
  }
  pc=reg_getpc();
  if(!(pc>=0xF000 && pc<=0xFFFF || pc>=0x80 && pc<=0xFF))
  {
    TRACE("PC out of range, resetting chip\n"); 
    reset(); // hack
  }
#if defined(SS_IKBD_6301_ADJUST_CYCLES)
  if(cycles_to_give_back)
  {
    // we really must go slowly here, better lose some time sync
    // than break custom programs (TB2 etc.)
#ifdef WIN32
    int cycles_given_back=min(20,cycles_to_give_back);
#else
    int cycles_given_back=cycles_to_give_back;
    if(cycles_given_back>20)
      cycles_given_back=20;
#endif
    cycles_to_run-=cycles_given_back;         //cycles_to_give_back;
    //cycles_to_give_back=0;
    cycles_to_give_back-=cycles_given_back;
  }
#endif

  while(!Crashed6301 && (cycles_run<cycles_to_run 
#if defined(SS_IKBD_6301_RUN_IRQ_TO_END)
    || ExecutingInt==EXECUTING_INT
#endif
  ))
  {
    instr_exec (); // execute one instruction
    cycles_run=cpu.ncycles-starting_cycles;
  }
#if defined(SS_IKBD_6301_RUN_IRQ_TO_END)
  if(ExecutingInt)
  {
    ASSERT( ExecutingInt==FINISHED_EXECUTING_INT );
    ExecutingInt=NOT_EXECUTING_INT;
  }
#endif
#if defined(SS_IKBD_6301_ADJUST_CYCLES)
//  ASSERT( !cycles_to_give_back );
  cycles_to_give_back+=cycles_run-cycles_to_run;
#endif
  return (Crashed6301) ? 0 : cycles_run; 
}


hd6301_transmit_byte(u_char byte_in) {
  sci_in(&byte_in,1);
}


hd6301_load_save(int one_if_save,unsigned char *buffer) {
  // A function to help the memory snapshot facility of Steem
  unsigned char *i=buffer; // stack on Steem's side
  ASSERT(buffer);
  ASSERT(ram);
  if(!ram)
    return 0; // run-time anti-crash
  // cpu registers
  if(one_if_save)
    memmove(i,&regs,sizeof(regs));
  else
    memmove(&regs,i,sizeof(regs));
  i+=sizeof(regs);
  // cpu
  if(one_if_save)
    memmove(i,&cpu,sizeof(cpu));
  else
    memmove(&cpu,i,sizeof(cpu));
  i+=sizeof(cpu);
#if !defined(SS_IKBD_6301_DISABLE_CALLSTACK)
  // callstack - it's a debug feature, useless for us, lots of space
  if(one_if_save)
    memmove(i,&callstack,sizeof(callstack));
  else
    memmove(&callstack,i,sizeof(callstack));
  i+=sizeof(callstack);
#endif
  // ram
  if(one_if_save)
  {
    TRACE("Snapshot - save RAM\n");
#if defined(DUMP_RAM)
    dump_ram();
#endif
    memmove(i,&ram[0x80],128);
  }
  else
  {
    TRACE("Snapshot - load RAM\n");
    memmove(&ram[0x80],i,128);
#if defined(DUMP_RAM)
    dump_ram();
#endif
  }
  i+=128;

  // iregs 
  if(one_if_save)
    memmove(i,&iram,sizeof(iram));
  else
    memmove(&iram,i,sizeof(iram));
  i+=sizeof(iram);

  // our variables (TODO?)
  //TRACE("Size of 6301 snapshot: %d bytes\n",i-buffer);
  return i-buffer;
}

// Other functions for ST emulation. Also check ireg.c

load_rom() {
  FILE *fp;
  char romfile[20]=HD6301_ROM_FILENAME;
#if defined(SS_IKBD_6301_TRACE)
  TRACE("6301: Init RAM, load ROM %s\n",romfile);
#endif
  
  //fp=fopen(romfile,"r+b");
  fp=fopen("./HD6301V1ST.img","r+b");
  if(fp)
  {
    int i,checksum=0;
    int n=fread(ram+0xF000,1,4096,fp);
#if defined(SS_IKBD_6301_TRACE)
    ASSERT(n==4096); // this detected the missing +b above
    for(i=0;i<4096;i++)
      checksum+=ram[0xF000+i];
    if(checksum!=HD6301_ROM_CHECKSUM)
      TRACE("checksum of rom=%d expected=%d\n",checksum,HD6301_ROM_CHECKSUM);
#endif
    fclose(fp);
    fprintf(stderr,"6301 OK\n");
  }
  else 
  {
    TRACE("Failed to open file\n");
    fprintf(stderr,"6301 KO\n");
    free(ram);
    ram=NULL;
  } 
  return (int)ram; // pointer as int, 0 if failed to load ROM
}


dump_rom() {
  int i;
  TRACE("************************************************************\n");
  TRACE("* This disassembly of the Atari ST's HD6301V1 ROM was made *\n");
  TRACE("* by Sim6xxx as modified for Steem                         *\n");
  TRACE("************************************************************\n\n\n");
  for(i=0xF000;i<0xFFFF;i++)
  {
    // don't decode data, + indication
    // data bytes
    if(i==0xFF6e || i==0xffed)
      mem_print (i, 1, 1);
    else if(i>=0xFF6F)
    {
      mem_print (i, 2, 2);
      i++;
    } 
    // data groups
    else if(i==0xf2f3)
    {
      TRACE("eg Scancodes are here below\n");
      mem_print (i, 0xF370-0xf2f3+1,14); 
      i+=0xF370-0xf2f3;
    }
    else if(i==0xf679)
    {
      mem_print (i, 8,8); 
      i+=8;
    }
    else if(i==0xfed0)
    {
      mem_print (i, 0xfee1-0xfed0, 0xfee1-0xfed0); 
      i+=0xfee1-0xfed0;
    }
    // instructions
    else
      i+=instr_print (i)-1;
  }//nxt
}


dump_ram() {
  int i;
  TRACE("RAM dump\n    \t00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
  mem_print (0x80,128,16);
  TRACE("RAM disassembly\n");
  for(i=0x80;i<0x80+128;i++)
    i+=instr_print (i)-1;
}

#undef LOGSECTION

#endif