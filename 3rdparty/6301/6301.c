// This file is compiled as a distinct module (resulting in an OBJ file)
#include "6301.h"
#include "SSE/SSE.h"
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
#include "SSE/SSEOption.h"
extern int iDummy;
#ifndef WIN32
unsigned int _rotr(unsigned int Data, unsigned int Bits) {
  return ((Data >> Bits) | (Data << (32-Bits)));
}
unsigned int _rotl(unsigned int Data, unsigned int Bits) {
  return ((Data << Bits) | (Data >> (32-Bits)));
}
#endif
 
// functions & variables from Steem (declared as extern "C") there
extern void hd6301_keyboard_buffer_write(unsigned char src) ;
extern int cpu_timer,cpu_cycles; // for debug
#define ABSOLUTE_CPU_TIME (cpu_timer-cpu_cycles)
#define act ABSOLUTE_CPU_TIME
extern unsigned char  stick[8]; // joysticks

// variables from Steem we declare here as 'C' linkage (easier than in Steem)
int ST_Key_Down[128]; // not better than what I envisioned but effective!
int mousek;
// our variables that Steem must see
int hd6301_receiving_from_MC6850=0; // for sync
int hd6301_transmitting_to_MC6850=0; // for sync
int hd6301_completed_transmission_to_MC6850; // for sync
int hd6301_mouse_move_since_last_interrupt_x; // different lifetime
int hd6301_mouse_move_since_last_interrupt_y;

// additional variables for our module

unsigned char rec_byte;
#if defined(SS_IKBD_RUN_IRQ_TO_END)
int ExecutingInt=0;
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

#if defined(SS_IKBD_6301_TRACE)
FILE *trace_file_pointer_6301=0;
#endif

// constructing our module (OBJ) the good old Steem way, hem
#define error printf // saves headache
#define warning printf 
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
  printf("6301: destroy object\n");
  if(ram) 
    free(ram);
  ram=NULL;
#if !defined(SS_IKBD_6301_DISABLE_BREAKS)
  if(breaks) 
    free(breaks);
  breaks=NULL;
#endif
#if defined(SS_IKBD_6301_TRACE)
  if(trace_file_pointer_6301)
    fclose(trace_file_pointer_6301);
#endif  
}


hd6301_reset() {
  printf("6301 emu cpu reset\n");
  cpu_reset();
  hd6301_receiving_from_MC6850=hd6301_transmitting_to_MC6850
    =hd6301_completed_transmission_to_MC6850=0;
  iram[TRCSR]=0x20;
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
    printf("6301 starting cpu\n");
    cpu_start();
  }
  if((iram[TRCSR]&1) && !hd6301_receiving_from_MC6850)  
  {
    printf("6301 waking up (PC %X cycles %d ACT %d)\n",reg_getpc(),cpu.ncycles,act);
    iram[TRCSR]&=~1; // ha! that's for Barbarian (& Froggies)
  }
  pc=reg_getpc();
  if(!(pc>=0xF000 && pc<=0xFFFF || pc>=0x80 && pc<=0xFF))
  {
    printf("PC out of range, resetting chip\n"); 
    reset(); // hack
  }
#if defined(SS_IKBD_6301_ADJUST_CYCLES)
  if(cycles_to_give_back>=cycles_to_run)
  {
    cycles_to_give_back-=cycles_to_run;
    return -1;
  }
  else if(cycles_to_give_back)
  {
    cycles_to_run-=cycles_to_give_back;
    cycles_to_give_back=0;
  }
#endif
  // the ExecutingInt hack seemed necessary at some point, maybe it isn't
  while(cycles_run<cycles_to_run && !Crashed6301 
#if defined(SS_IKBD_RUN_IRQ_TO_END)
    || ExecutingInt==1
#endif
  )
  {
    instr_exec (); // execute one instruction
    cycles_run=cpu.ncycles-starting_cycles;
  }
#if defined(SS_IKBD_RUN_IRQ_TO_END)
  if(ExecutingInt)
  {
    ASSERT( ExecutingInt==-1 );
    ExecutingInt=0;
  }
#endif
#if defined(SS_IKBD_6301_ADJUST_CYCLES)
  ASSERT( !cycles_to_give_back );
  cycles_to_give_back=cycles_run-cycles_to_run;
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
    printf("Snapshot - save RAM\n");
#if defined(DUMP_RAM)
    dump_ram();
#endif
    memmove(i,&ram[0x80],128);
  }
  else
  {
    printf("Snapshot - load RAM\n");
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
  //printf("Size of 6301 snapshot: %d bytes\n",i-buffer);
  return i-buffer;
}

// Other functions for ST emulation. Also check ireg.c

load_rom() {
  FILE *fp;
  char *romfile=HD6301_ROM_FILENAME;
#if defined(SS_IKBD_6301_TRACE)
  fp = freopen("Trace6301.txt", "w", stdout ); // open trace file
  ASSERT(fp);
  if(fp)
    trace_file_pointer_6301=fp; // used to close
  printf("6301: Init RAM, load ROM %s\n",romfile);
#endif
  
  fp=fopen(romfile,"r+b");
  if(fp)
  {
    int i,checksum=0;
    int n=fread(ram+0xF000,1,4096,fp);
#if defined(SS_IKBD_6301_TRACE)
    ASSERT(n==4096); // this detected the missing +b above
    for(i=0;i<4096;i++)
      checksum+=ram[0xF000+i];
    if(checksum!=HD6301_ROM_CHECKSUM)
      printf("checksum of rom=%d expected=%d\n",checksum,HD6301_ROM_CHECKSUM);
#endif
    fclose(fp);
  }
  else 
  {
    printf("Failed to open file\n");
    free(ram);
    ram=NULL;
  } 
  return (int)ram; // pointer as int, 0 if failed to load ROM
}


dump_rom() {
  int i;
  printf("************************************************************\n");
  printf("* This disassembly of the Atari ST's HD6301V1 ROM was made *\n");
  printf("* by Sim6xxx as modified for Steem                         *\n");
  printf("************************************************************\n\n\n");
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
      printf("eg Scancodes are here below\n");
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
  printf("RAM dump\n    \t00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
  mem_print (0x80,128,16);
  printf("RAM disassembly\n");
  for(i=0x80;i<0x80+128;i++)
    i+=instr_print (i)-1;
}

#endif