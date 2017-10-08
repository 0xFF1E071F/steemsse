/*  6301.c
    This file is compiled as one distinct module (resulting in an OBJ file)
    We don't compile each C file
 */
#include "6301.h"
#include "SSE/SSE.h"
#include <SSE/SSEDebug.h>
#include <SSE/SSE6301.h>
#include <acia.decla.h>

#if defined(SSE_IKBD_6301)
#include <SSE/SSEOption.h>

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
extern unsigned char  stick[8]; // joysticks
// variables from Steem we declare here as 'C' linkage (easier than in Steem)
BYTE ST_Key_Down[128];
int mousek;
// our variables that Steem must see
int cycles_run=0; 

// additional variables for our module
unsigned char rec_byte;
unsigned int mouse_x_counter;
unsigned int mouse_y_counter;
#if defined(SSE_BUGFIX_394)
COUNTER_VAR hd6301_vbl_cycles;
#else
int hd6301_vbl_cycles;
#endif
COUNTER_VAR time_of_tdr_to_tdrs=0;
COUNTER_VAR current_m68_cycle=0; // will lag or be forward

// Debug facilities
// ASSERT
#if defined(_MSC_VER) && defined(_DEBUG)
#undef ASSERT

#if defined(SSE_X64_DEBUG)
#define ASSERT(x) {if(!(x)) {TRACE("Assert failed: %s\n",#x);\
                  DebugBreak();}}
#else
#define ASSERT(x) {if(!(x)) {TRACE("Assert failed: %s\n",#x);\
                   _asm{int 0x03}}}
#endif
#else 
#if !defined(NDEBUG) ||(defined(VC_BUILD) && defined(DEBUG_BUILD))
#define ASSERT(x) {if(!(x)) TRACE("Assert failed: %s\n",#x);} //just a trace
#else
#define ASSERT(x) // release
#endif
#endif
// TRACE
#undef TRACE
#if defined(SSE_DEBUG) 
// we use this trick because Trace is a struct function
void (*hd6301_trace)(char *fmt, ...);
#undef LOGSECTION
#define TRACE Debug.LogSection=LOGSECTION_IKBD,hd6301_trace
#else
#define TRACE
#endif

// constructing our module (OBJ) the good old Steem way, hem
#ifdef VC_BUILD //on vs2008, v3.8.2, v3.9.0
// we don't want to edit old working 3rd party C code
#pragma warning( disable : 4013 4100 4127 4131 4431 4245 4706)
#endif

#ifdef MINGW_BUILD
//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define __max(a,b) (a>b ? a:b)
#define __min(a,b) (a>b ? b:a)

#endif

#define error printf // saves headache
#define warning TRACE //printf 
// base
#if !defined(SSE_IKBD_6301_DISABLE_CALLSTACK)
#include "callstac.c"
#endif
#include "cpu.c"
#include "fprinthe.c"
#include "memsetl.c"
#include "symtab.c"  
#ifndef SSE_COMPILER_382
#include "tty.c"
#endif
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

BYTE* hd6301_init() {
  return mem_init();
}


hd6301_destroy() {
  TRACE("6301: destroy object\n");
  if(ram) 
    free(ram);
  ram=NULL;
#if !defined(SSE_IKBD_6301_DISABLE_BREAKS)
  if(breaks) 
    free(breaks);
  breaks=NULL;
#endif
}


hd6301_reset(int Cold) {
  TRACE("6301 emu cpu reset (cold %d)\n",Cold);
  cpu_reset();
  if(Cold)
    memset (ram, 0, 256);
  iram[TRCSR]=0x20;
  mem_putw (OCR, 0xFFFF);
  if(Cold)
  {
    mouse_x_counter=MOUSE_MASK;
    mouse_y_counter=MOUSE_MASK;
  }
  current_m68_cycle=0;
}

extern double cpu_cycles_multiplier;
  
hd6301_run_cycles(COUNTER_VAR to_m68_cycle) {
  // Called by Steem to run some cycles (per scanline or to update before to IO)
  COUNTER_VAR cycles_to_run=(to_m68_cycle-current_m68_cycle)/HD6301_CYCLE_DIVISOR;
  int pc;
  COUNTER_VAR starting_cycles=cpu.ncycles;
#if defined(SSE_BUGFIX_394) && defined(SSE_TIMING_MULTIPLIER_392) //forgot that...
  if(cpu_cycles_multiplier>1.0)
    cycles_to_run/=cpu_cycles_multiplier;
#endif
  if(cycles_to_run<-256 || cycles_to_run>255)
  {
    current_m68_cycle=to_m68_cycle; //safety
    return 0;
  }

  // make sure our 6301 is running OK
  if(!cpu_isrunning())
  {
    TRACE("6301 starting cpu\n");
    cpu_start();
  }
  if((iram[TRCSR]&1) && !ACIA_IKBD.LineTxBusy)
  {
    TRACE("6301 waking up (PC %X cycles %d)\n",reg_getpc(),cpu.ncycles);
    iram[TRCSR]&=~1; // ha! that's for Barbarian (& Obliterator, Froggies)
  }
  pc=reg_getpc();
  if(OPTION_HACKS && !(pc>=0xF000 && pc<=0xFFFF || pc>=0x80 && pc<=0xFF))
  {
    TRACE("PC out of range, resetting chip\n"); 
    reset(); // hack
  }
  while(!HD6301.Crashed && (cycles_run<cycles_to_run))
  {
    instr_exec (); // execute one instruction
    cycles_run=cpu.ncycles-starting_cycles;
  }
  
  hd6301_vbl_cycles+=cycles_run;
  HD6301.ChipCycles=cpu.ncycles; // HD6301 object doesn't know cpu.ncycles
  current_m68_cycle+=cycles_run*HD6301_CYCLE_DIVISOR
#if defined(SSE_BUGFIX_394) && defined(SSE_TIMING_MULTIPLIER_392)
    *(cpu_cycles_multiplier>1.0 ? cpu_cycles_multiplier : 1)
#endif
    ;
  cycles_run=0;
  return cycles_run; //unused
}


hd6301_receive_byte(u_char byte_in) {
  ASSERT(byte_in==HD6301.rdrs);
  return sci_in(&byte_in,1);
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
#if !defined(SSE_IKBD_6301_DISABLE_CALLSTACK)
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
    TRACE("6301 Snapshot - save RAM\n");
#if defined(SSE_BOILER_DUMP_6301_RAM_ON_LS)
    hd6301_dump_ram();
#endif
    memmove(i,&ram[0x80],128);
  }
  else
  {
    TRACE("Snapshot - load RAM\n");
    memmove(&ram[0x80],i,128);
#if defined(SSE_BOILER_DUMP_6301_RAM_ON_LS)
    hd6301_dump_ram();
#endif
  }
  i+=128;

  // iregs 
  if(one_if_save)
    memmove(i,&iram,sizeof(iram));
  else
    memmove(&iram,i,sizeof(iram));
  i+=sizeof(iram);

  if(!one_if_save)
    current_m68_cycle=0;

  return i-buffer;
}

// Other functions for ST emulation. Also check ireg.c

#if defined(SSE_IKBD_6301_DISASSEMBLE_ROM)
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
    else if(i==0xf679)//arrows +...
    {
      mem_print (i, 7,2); 
      i+=7;
    }
    else if(i==0xf87a)//arrows
    {
      mem_print (i, 4,2); 
      i+=4-1;
    }
    else if(i==0xf930)
    {
      mem_print (i, 0xf990-0xf930,2); 
      i+=0xf990-0xf930-1;
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
#endif

#if defined(SSE_BOILER_DUMP_6301_RAM)

hd6301_dump_ram() { // commanded by Boiler
#if defined(SSE_BOILER_DUMP_6301_RAM2)
  int i;
#endif
  printf("6301 RAM dump\n    \t00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
  mem_print (0,256,16);
#if defined(SSE_BOILER_DUMP_6301_RAM2)
  // disassembly in case
  for(i=0x89;i<256;i++)
  {
      i+=instr_print (i)-1;
  }//nxt
#endif


}

#endif

#if defined(SSE_BOILER_BROWSER_6301)
/*  We copy the memory instead of having direct access.
    TODO: use proper separation methods?
*/
hd6301_copy_ram(unsigned char *ptr) {
  int i;
  if(!ram)//during init
    return -1;
  for(i=0;i<0x16;i++)
    ptr[i]=iram[i];
  for(i=0x16;i<256;i++)
    ptr[i]=mem_getb (i);
  return 0;
}

#endif



#undef LOGSECTION

#ifdef MINGW_BUILD
//#pragma GCC diagnostic pop
#endif

#endif