/* <<<                                  */
/* Copyright (c) 1994-1996 Arne Riiber. */
/* All rights reserved.                 */
/* >>>                                  */
#include <assert.h>
#include <stdio.h>

#include "defs.h"
#include "chip.h"
#include "cpu.h"
#include "ireg.h"
#include "memory.h"
#include "optab.h"
#include "reg.h"
#include "sci.h"
#include "timer.h"

#ifdef USE_PROTOTYPES
#include "instr.h"
#endif


/*
 *  reset - jump to the reset vector
 */
reset () 
{
  reg_setpc (mem_getw (RESVECTOR));
  reg_setiflag (1);
#if defined(SS_IKBD_6301_DISASSEMBLE_ROM) // disassemble ST's 6301 rom
  {//scope
  static int RomWasDisassembled=0;
  if(!RomWasDisassembled) 
  {
    dump_rom();
    RomWasDisassembled++;// only once
  }
  }
#endif
}


/*
 * instr_exec - execute an instruction
 */
instr_exec ()
{

  /*
   * Get opcode from memory,
   * inc program counter to point to first operand,
   * Decode and execute the opcode.
   */
  struct opcode *opptr;
  int interrupted = 0;    /* 1 = HW interrupt occured */

#ifndef M6800
/*  ST
    We kind of sync 6301 to ACIA transmission, allowing the interrupt
    only when the byte has been received.
*/
  if(hd6301_completed_transmission_to_MC6850 
#if defined(SS_IKBD_RUN_IRQ_TO_END)
    && ExecutingInt!=1
#endif
  )
  {
#if defined(SS_IKBD_6301_TRACE_SCI_TX)
    printf("ACT %d Cycles %d TX finished TRCSR=%X\n",act,cpu.ncycles,iram[TRCSR]);
#endif
    ASSERT(hd6301_completed_transmission_to_MC6850==1);
    hd6301_completed_transmission_to_MC6850--;
    ASSERT(hd6301_completed_transmission_to_MC6850>=0);
    ASSERT(!hd6301_completed_transmission_to_MC6850);
    //if(ireg_getb (TRCSR) & TIE)
      txinterrupts=1; // we should trigger IRQ
  }

  if (!reg_getiflag () 
#if defined(SS_IKBD_RUN_IRQ_TO_END)
    && ExecutingInt!=1
#endif
  ) 
  {
    /*
     * Check for interrupts in priority order
     */
    if ((ireg_getb (TCSR) & OCF) && (ireg_getb (TCSR) & EOCI)) {
#if defined(SS_IKBD_6301_TRACE_INT_TIMER)
      printf("ACT %d Timer interrupt\n",act);
#endif
      int_addr (OCFVECTOR);
      interrupted = 1;
    } 
    else if (serial_int ()) {
#if defined(SS_IKBD_6301_TRACE_INT_SCI)
      printf("ACT %d cycles %d SCI interrupt TRCSR=%X RXi=%d TXi=%d\n",act,cpu.ncycles,ireg_getb (TRCSR),rxinterrupts,txinterrupts);
#endif
      txinterrupts=0; //?
      int_addr (SCIVECTOR);
      interrupted = 1;
    }
  }
#if 0
  /*
   * 6301 Address check error: Trap if address is outside RAM/ROM
   * This must be modified to check mode (single-chip/extended) first
   */
  if ((reg_getpc() <= ramstart) || reg_getpc() >  ramend)
  {
    error ("instr_exec: Address error: %04x\n", reg_getpc());
    trap();   /* Highest pri vector after Reset */
  }
#endif /* 0 */
#endif /* M6800 */

  if (interrupted) /* Prepare cycle count for register stacking */
  {
    opptr = &opcodetab[0x3f]; /* SWI */
  }
  else
  {
    int pc=reg_getpc (); 
    
    ASSERT( pc<0xFFFF );
    ASSERT( pc>=0x80 );
    
    if( ! ( pc>=0x80 && pc<0xFFFF) ) // eg bad snapshot
    {
      printf("6301 emu is hopelessly crashed!\n");
      Crashed6301=1;
      return -1;
    }

    opptr = &opcodetab [mem_getb (reg_getpc ())];
    reg_incpc (1);
    (*opptr->op_func) ();
  }

  cpu_setncycles (cpu_getncycles () + opptr->op_n_cycles);
  timer_inc (opptr->op_n_cycles);
  return 0;
}


/*
 * instr_print - print (unassemble) an instruction
 */
instr_print (addr)
  u_int addr;
{
  u_short    pc = addr;
  u_char     op = mem_getb (pc);
  struct opcode *opptr  = &opcodetab[op];
  char    *symname;

  /*
   * In case somebody changes the opcodemap, verify it
   */
  ASSERT (opptr->op_value == op);

  if(pc==mem_getw(0xFFF0)) // SS
    printf("\nSCI interrupt start (rx %d)\n",rxinterrupts);

  if(pc==mem_getw(0xFFF4)) // SS
    printf("\nOCF interrupt start\n");

  printf ("%04x\t", pc);

  if (opptr->op_n_operands == 0)
  {
    printf ("%02x\t\t",  mem_getb (pc));
    printf (opptr->op_mnemonic, mem_getb (pc + 1));
  }
  else if (opptr->op_n_operands == 1)
  {
    printf ("%02x %02x\t\t",  mem_getb (pc), mem_getb (pc+1));
    printf (opptr->op_mnemonic, mem_getb (pc + 1));
    if (symname = sym_find_name (mem_getb (pc + 1)))
      printf ("\t%s", symname);
    // SS: branches, compute handy absolute addresses
    if(opptr->op_value>=0x20 && opptr->op_value<=0x2F
      ||opptr->op_value==0x8d)
    {
      int disp=mem_getb (pc + 1);
      if(disp&0x80) // negative displacement
        disp=-(0xFF-disp+1);
      printf(" (%X)", pc+2+disp);
    }
  }
  else
  {
    printf ("%02x %02x %02x\t", mem_getb (pc),
      mem_getb (pc + 1), mem_getb (pc + 2));
    printf (opptr->op_mnemonic, mem_getw (pc + 1));
    if (symname = sym_find_name (mem_getw (pc + 1)))
      printf ("\t%s", symname);
  }
#if !defined(SS_IKBD_6301_DISASSEMBLE_ROM)
  printf ("\t[%d]\n", cpu_getncycles ());
#endif
  putchar('\n');
  if(opptr->op_value==0x39 || opptr->op_value==0x3b)
    putchar('\n');
  return opptr->op_n_operands + 1;
}
