/* <<<                                  */
/* Copyright (c) 1994-1996 Arne Riiber. */
/* All rights reserved.                 */
/* >>>                                  */
#include <stdio.h>

#include "defs.h"
#include "chip.h"
#include "cpu.h"
#include "ireg.h"
#include "sci.h"

/*
This part of the emu deals with serial I/O between the 6301 and its CPU, in
our case (ST), via the ACIA. 

Registers:
------------------------------------------------------------------------------
  * Serial Communication Interface
------------------------------------------------------------------------------
$0010    | B | RMCR  | Rate & Mode Control Register                   | RW
$0011    | B | TRCSR | Transmit/Receive Control & Status Register     | RW
$0012    | B | RDR   | Receive Data Register                          | R0
$0013    | B | TDR   | Transmit Data Register                         | W0
------------------------------------------------------------------------------
*/

/*
 * Pseudo-received data buffer used by rdr_getb() routines
 */
static u_char recvbuf[BUFSIZE];
static int  rxindex      = 0; /* Index of first byte in recvbuf */
       int  rxinterrupts = 0; /* Number of outstanding rx interrupts */
       int  txinterrupts = 0; /* Number of outstanding tx interrupts */

/*
 * sci_in - input to SCI
 *
 * increment number of outstanding rx interrupts
 */

/*  This function is called by Steem when a byte sent to the 6301 is supposed
    to have been received, decoded in the shift register and transferred in RDR.
    The transmission delay is taken care of in Steem.
    The function allows accumulation of input, but in real hardware, there can
    be only one byte in the RDR.
*/

sci_in (s, nbytes)
  u_char *s;
int nbytes;
{
  int i;
  u_char trcsr=iram[TRCSR];
#if defined(SS_IKBD_6301_CHECK_COMMANDS)
  int pc=reg_getpc(); // word
#endif

  ASSERT(nbytes==1);
  ASSERT(trcsr&RE);
  //ASSERT(!(trcsr&WU));

  for (i=0; i<nbytes; i++)
    if (rxinterrupts < BUFSIZE)
      recvbuf[rxinterrupts++] = s[i];
    else
      warning ("sci_in:: buffer full\n");
  rec_byte=*s;

#if defined(SS_IKBD_6301_TRACE_SCI_RX)
  TRACE("SCI RX #%d $%x (TRCSR %X PC %X cycles %d ACT %d)\n",rxinterrupts,rec_byte,trcsr,pc,cpu.ncycles,ABSOLUTE_CPU_TIME);
#endif
  if(trcsr&WU) // bug?
    TRACE("6301 in standby mode\n");

  if(rxinterrupts>1)  
  {
    TRACE("RX interrupt overload %d\n",rxinterrupts); 
    iram[TRCSR]|=ORFE; // hardware sets overrun bit
    rxinterrupts=1; // there can be only one byte to read
  }

#if defined(SS_IKBD_6301_CHECK_COMMANDS)
  if(pc<0xF000) // PC is in the RAM=>custom program running
  {
#if defined(SS_IKBD_6301_DISASSEMBLE_CUSTOM_PRG)
    BytesToLoad--; // we need a counter, we recycle this one
    if(BytesToLoad==-256) // Drag: Frog: TB2: 
      dump_ram();
#endif
  }
  else if(BytesToLoad>0)
  {
    int our_address=CustomPrgAddress-3+CurrentParameter;
    CurrentParameter++;
    BytesToLoad--;
#if defined(SS_IKBD_TRACE_COMMANDS)
    TRACE("Loading byte %X in %X, %d to go $F0:%d\n",rec_byte,our_address,BytesToLoad,ram[0xF0]);
#endif
    if(!BytesToLoad)
      CurrentCommand=CurrentParameter=0;
  }
  else if(CurrentCommand && TotalParameters)
  {
    CurrentParameter++;
#if defined(SS_IKBD_TRACE_COMMANDS)
    TRACE("Parameter %d of command %x = %X\n",CurrentParameter,CurrentCommand,rec_byte);
#endif
    if(CurrentCommand==0x20) // MEMORY LOAD
    {
      if(CurrentParameter==1)
        CustomPrgAddress=rec_byte<<8;
      if(CurrentParameter==2)
        CustomPrgAddress|=rec_byte;
      if(CurrentParameter==3)
      {
#if defined(SS_IKBD_TRACE_COMMANDS)
        TRACE("%d bytes to load in RAM %X\n",rec_byte,CustomPrgAddress);
#endif
        BytesToLoad=rec_byte;
      }
    }
    else if(CurrentParameter==TotalParameters)
    {
      if(CurrentCommand==0x22) // CONTROLLER EXECUTE
      {
#if defined(SS_IKBD_6301_DISASSEMBLE_CUSTOM_PRG)
        TRACE("Boot\n");
        dump_ram();
#endif
      }
      CurrentCommand=0;
    }
  }
  else if(rec_byte) // new command
  {
    CurrentParameter=0;
    switch(rec_byte)
    {
    case 0x80: // RESET
    case 0x07: // SET MOUSE BUTTON ACTION
    case 0x17: // SET JOYSTICK MONITORING
      TotalParameters=1;
      break;
    case 0x09: // SET ABSOLUTE MOUSE POSITIONING
      TotalParameters=4;
      break;
    case 0x0A: // SET MOUSE KEYCODE MOUSE
    case 0x0B: // SET MOUSE THRESHOLD
    case 0x0C: // SET MOUSE SCALE
      TotalParameters=2;
      break;
    case 0x0E: // LOAD MOUSE POSITION
      TotalParameters=5;
      break;
    case 0x12: // 12 DISABLE MOUSE
      TotalParameters=0;
#if defined(SS_IKBD_MOUSE_OFF_JOYSTICK_EVENT)
      TRACE("Disable mouse hardware quirk\n");
      if(SSE_HACKS_ON)
        mouse_x_counter=mouse_y_counter=~MOUSE_MASK; // fixes Jumping Jackson
#endif
      break;
    case 0x19: // SET JOYSTICK KEYCODE MODE
    case 0x1B: // TIME-OF-DAY CLOCK SET
      TotalParameters=6;
      break;
    case 0x20: // MEMORY LOAD
      TotalParameters=3;
      break;
    case 0x21: // MEMORY READ
    case 0x22: // CONTROLLER EXECUTE
//      mem_print (CustomPrgAddress, 0xE, 1); //bytes to load = 0
      TotalParameters=2;
      break;
    default:
      // 8 SET RELATIVE MOUSE POSITION REPORTING
      // D INTERROGATE MOUSE POSITION
      // F SET Y=0 AT BOTTOM
      // 10 SET Y=0 AT TOP
      // 11 RESUME
      // 13 PAUSE OUTPUT
      // 14 SET JOYSTICK EVENT REPORTING
      // 15 SET JOYSTICK INTERROGATION MODE
      // 16 JOYSTICK INTERROGATE
      // 18 SET FIRE BUTTON MONITORING
      // 1A DISABLE JOYSTICKS
      // 1C INTERROGATE TIME-OF-DAT CLOCK
      // 87... STATUS INQUIRIES
      TotalParameters=0;
    }
#if defined(SS_IKBD_TRACE_COMMANDS)
    TRACE("IKBD command in $%X, %d parameters\n",rec_byte,TotalParameters);
#endif
    CurrentCommand=rec_byte;
  }
  else ; // could be junk?
#endif
  return 0;//warning
}


sci_print ()
{
  printf ("sci recvbuf:\n");
  fprinthex (stdout, recvbuf + rxindex, rxinterrupts);
}


/*
TRCSR

This register controls the communications.
-Bit 0 [$1] (RW) : Wake Up, when set to 1, wait until ten 1 appear on the line,
              then it switches to zero.
-Bit 1 [$2] (RW) : Transmit Enable
-Bit 2 [$4] (RW) : Transmit Interrupt Enable
-Bit 3 [$8] (RW) : Receive Enable
-Bit 4 [$10] (RW) : Receive Interrupt Enable
-Bit 5 [$20] (RO?) : Transmit Data Register Empty, is set to 1 when a byte has been
              sent, and is set to 0 when TRCSR is read then TDR is written.
-Bit 6 [$40] (RO) : Overrun or Framing Error, is set to 1 when a byte is received if
              the previous byte was not read, and is set to 0 when TRCSR then
              RDR are read.
-Bit 7 [$80] (RO) : Receive Data Register Full, is set to 1 when a byte have been
              received, and is set to zero when TRSCR then RDR are read.

The read-only bits are set and cleared by the hardware.

Normally bit 5 is read-only, but the ROM tries to set it (writing $2A,$3A,$3E)

The following 2 functions trcsr_getb and trcsr_putb are used when the program
reads or writes the status/control register $11.

*/



/*
 * trcsr_getb - always return Transmit Data Reg. Empty = 1
 */

trcsr_getb (offs)
  u_int offs;
{
  char c;
  unsigned char rv;
#if 0
  /*
   * Check if user has typed a key, prevent simulator overrun
   * if data in recvbuf, return RDRF
   */
  if (!rxinterrupts && (c = tty_getkey (0))) /* Typed a key */
    sci_in (&c, 1);
#endif

  rv=ireg_getb (TRCSR);

/*  ST
    Update RDRF (Receive Data Register Full), as it was
    Update TDRE (Transmit Data Register Empty), not always 1
*/

  if (rxinterrupts)
    rv|=RDRF; 
  else
    rv&= ~RDRF;

#if defined(SS_IKBD_DOUBLE_BUFFER_6301_TX)
  // this could work with high values (buffers) but is meant for 1
  if(hd6301_transmitting_to_MC6850<=hd6301_completed_transmission_to_MC6850) 
#else
  if(!hd6301_transmitting_to_MC6850)
#endif
    rv|=TDRE; // is empty
  else
    rv&=~TDRE; // is full
    
  return rv;
}


/*
 *  trcsr_putb - enable/disable tx/rx interrupt
 *
 *  Sets global interrupt flag if tx interrupt is enabled
 *  so main loop can execute interrupt vector
 */
trcsr_putb (offs, value)
  u_int  offs;
  u_char value;
{
//  u_char trcsr; //ST
  ASSERT(value&RE); // Receive is never disabled by program
  if(value&1)
    TRACE("Stand by mode PC %X\n cycles %d",reg_getpc(),cpu.ncycles);
  
#if defined(SS_IKBD_6301_SET_TDRE)
  //  Here we do as if the program could set bit 5 of TRCSR
  value&=0x3F; 
  if(value&0x20) 
  {
#if defined(SS_IKBD_6301_TRACE_SCI_TRCSE)
    TRACE("set TRCSR %X (PC %X Cycles %d ACT %d)\n",value,reg_getpc(),cpu.ncycles,act);
#endif
    txinterrupts = 1; // this will force a check; fixes Cobra Compil 1
  }
#else // here, bit 5 of TRCSR can't be set by software
  value&=0x1F; 
#endif
  ireg_putb (TRCSR, value);
#if 0 // ST: We do nothing of the sort, of course
  trcsr = trcsr_getb (TRCSR);
  /*
   * trcsr & TDRE is always non-zero, thus we can
   * start generating tx int. request immediately
   */
  if (trcsr & TIE)
    txinterrupts = 1;
  else
    txinterrupts = 0;
#endif
  return 0;//warning
}

/*
 * rdr_getb - return the byte in the SCI receive data register
 *
 * If cpu is running (not memory dump), eat the "received" byte,
 * decrement number of outstanding rx interrupts
 * Assume RIE is enabled.
 */
rdr_getb (offs)
  u_int  offs;
{
  if (cpu_isrunning ()) {
    /*
     * If recvbuf is not empty, eat a byte from it
     * into RDR
     */
    if (rxinterrupts) {
#if defined(SS_IKBD_6301_TRACE)
      rec_byte=recvbuf[rxindex];
#if defined(SS_IKBD_6301_TRACE_SCI_RX)
      TRACE("SCI read RX $%x (PC %X cycles %d ACT %d)\n",rec_byte,reg_getpc(),cpu.ncycles,ABSOLUTE_CPU_TIME);
#endif
#endif
      ireg_putb (RDR, recvbuf[rxindex++]);

      rxinterrupts--;
    }
    /*
     * If the cpu has read all bytes in recvbuf[]
     * make recvbuf[] ready for more user sci data input
     */
    if (rxinterrupts == 0)
      rxindex = 0;

    //  ST
    if(iram[TRCSR]&ORFE)
    {
#if defined(SS_IKBD_6301_TRACE_SCI_RX)
      TRACE("Read RDR, clearing overrun bit\n");
#endif
      iram[TRCSR]&=~ORFE; // clear overrun bit
    }

    rxinterrupts=0;

  }
  return ireg_getb (RDR);
}

/*
 * tdr_putb - called to output a character
 *
 * Sets global interrupt flag if Tx interrupt is enabled
 * to signalize main loop to execute sci interrupt vector
 */

tdr_putb (offs, value)
  u_int  offs;
  u_char value;
{ 
  u_char trcsr;

  ireg_putb (TDR, value);
//  io_putb (value); // ST: this would pollute our trace (wondered what it was)
  /*
   * trcsr & TDRE is always non-zero, thus we can
   * start generating tx int. request immediately
   */
/*  ST
    In fact for Steem, we will generate the TX interrupt later when the byte
    has been transmitted, except if the line was free (double buffering), then
    it can be triggered at once.
*/
  trcsr = trcsr_getb (TRCSR);
//  if (trcsr & TIE)
//    txinterrupts = 1;
  
#if defined(SS_IKBD_DOUBLE_BUFFER_6301_TX)
  if((trcsr & TIE)&&!hd6301_transmitting_to_MC6850) 
    txinterrupts = 1; // double buffer in 6301: TX-shift registers
  ASSERT(hd6301_transmitting_to_MC6850<2);
#else
  txinterrupts=0; 
  ASSERT(!hd6301_transmitting_to_MC6850); 
#endif

  // this puts the byte on an agenda, the ST will read later
#if defined(SS_IKBD_DOUBLE_BUFFER_6301_TX)
  if(hd6301_transmitting_to_MC6850<2)
#else
  if(!hd6301_transmitting_to_MC6850  && (trcsr & TE))
#endif
    hd6301_keyboard_buffer_write(value); // call Steem's ikbd function
  else
    TRACE("Serial line not ready, sending %X fails!\n",value);

#if defined(SS_IKBD_DOUBLE_BUFFER_6301_TX)
  hd6301_transmitting_to_MC6850++; // can be >1 !!!
  ASSERT(hd6301_transmitting_to_MC6850<3); // max 2 bytes, 1 on the line, one in the register
#else
  hd6301_transmitting_to_MC6850=1; 
#endif
#if defined(SS_IKBD_6301_TRACE_SCI_TX)
  TRACE("SCI TX #%d $%x (PC %X Cycles %d ACT %d)\n",hd6301_transmitting_to_MC6850,value,reg_getpc (),cpu.ncycles,ABSOLUTE_CPU_TIME);
#endif
  return 0;//warning
}

