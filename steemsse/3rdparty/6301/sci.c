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
*/

sci_in (s, nbytes)
  u_char *s;
int nbytes;
{
#if !defined(SSE_VS2008_WARNING_382) || !defined(SSE_IKBD_6301_380)
  int i;
#endif
#if !defined(SSE_VS2008_WARNING_390) || defined(SSE_DEBUG)
  u_char trcsr=iram[TRCSR];
#endif
  ASSERT(nbytes==1);
  ASSERT(trcsr&RE);
  ASSERT(!(trcsr&RDRF));
  //ASSERT(!(trcsr&WU));
#if defined(SSE_IKBD_6301_380)
  ASSERT(rxinterrupts < BUFSIZE);
  recvbuf[rxinterrupts++] = *s;
#else
  for (i=0; i<nbytes; i++)
    if (rxinterrupts < BUFSIZE)
      recvbuf[rxinterrupts++] = s[i];
    else
      warning ("sci_in:: buffer full\n");
  rec_byte=*s;
  //TRACE("6301 SCI in #%d $%x (TRCSR %X)\n",rxinterrupts,rec_byte,trcsr);
#endif
#ifdef SSE_DEBUG
  if(trcsr&WU) // bug?
    TRACE("6301 in standby mode\n");
#endif
  if(rxinterrupts>1)  
  {
    TRACE("6301 OVR rx %d RDR %X RDRS %X SR %X->%X\n",rxinterrupts,recvbuf[rxinterrupts-2],recvbuf[rxinterrupts-1],iram[TRCSR],iram[TRCSR]|ORFE);
    iram[TRCSR]|=ORFE; // hardware sets overrun bit
    //recvbuf[0]=recvbuf[rxinterrupts-1]; // replace 
    rxinterrupts=1; // there can be only one byte to read
  }
#if defined(SSE_IKBD_6301_380) 
  else
  {
    HD6301.rdr=*s;
    TRACE("6301 RDR %X (PC %X)\n",HD6301.rdr,reg_getpc());
  }
#endif
  return 0;//warning
}

#if !defined(SSE_IKBD_6301_373) 
sci_print ()
{
  printf ("sci recvbuf:\n");
  fprinthex (stdout, recvbuf + rxindex, rxinterrupts);
}
#endif

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
#ifndef SSE_VS2008_WARNING_382
  char c;
#endif
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

  // if a byte is waiting, TDRE is false
  if(!ACIA_IKBD.ByteWaitingRx)
    rv|=TDRE; // "TDR is empty"
  else
    rv&=~TDRE; // "TDR is full"

#if defined(SSE_DEBUG_IKBD_6301_TRACE_STATUS)
  TRACE("6301 PC %X read TRCSR %X txi%d wb%d cycles %d\n",reg_getpc(),rv,txinterrupts,ACIA_IKBD.ByteWaitingRx,cycles_run); //tmp
#endif

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
    TRACE("Set 6301 stand-by\n");
  
#if defined(SSE_IKBD_6301_373) && !defined(SSE_IKBD_6301_380B)
/*  Enable serial int when TIE set, assuming the condition
    for IRQ is true.
    Cases: Cobra Compil 1, Defulloir
    Undef, this version breaks Pothole 2
*/
  value&=0x1F;
  if( (value&TIE) && ! (ireg_getb(TRCSR)&TIE))
  {
    TRACE("6301 set TIE tx %d\n",txinterrupts);
    txinterrupts=1; // force check - hack?
  }
#elif defined(SSE_IKBD_6301_SET_TDRE)
/*  Here we do as if the program could set bit 5 of TRCSR - correct?
    Maybe we're compensating another bug (internal problem with txinterrupts?)
    Cases:
    Cobra Compil 1: if we don't, "keyboard panic" 
    Defulloir
    Pothole 2
*/
  value&=0x3F;
  if(value&TDRE/*0x20*/) 
  {
    if((value & TIE)
#if defined(SSE_IKBD_6301_380B)
      && OPTION_HACKS // don't understand well
#endif
#if !defined(SSE_IKBD_6301_380B) // Defulloir
     &&!ACIA_IKBD.LineRxBusy
#endif
      ) 
    {
      TRACE("6301 program sets TDRE\n");
      txinterrupts = 1; // this will force a check
    }
  }
#else // here, bit 5 of TRCSR can't be set by software
  value&=0x1F;  
#endif
  //TRACE("6301 PC %X program writes TRCSR %X->%X\n",reg_getpc(),ireg_getb(TRCSR),value);
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

      rec_byte=recvbuf[rxindex];
#if defined(SSE_IKBD_6301_380) && defined(SSE_DEBUG)
      ASSERT(rec_byte==HD6301.rdr);
#endif
#if defined(SSE_DEBUG_IKBD_6301_TRACE_SCI_RX)
      //TRACE("6301 SCI read RX $%x (#%d)\n",rec_byte,rxindex);
      TRACE("6301 PC %X read RDR %X\n",reg_getpc(),rec_byte);
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
      TRACE("6301 clear OVR\n");
      iram[TRCSR]&=~ORFE; // clear overrun bit
    }

    ////////rxinterrupts=0;//

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
    Double buffer allows 1 byte waiting in TDR while another is being
    shifted, but not more. 
    TDRE = 0 while a byte is waiting in TDR.
*/
  trcsr = trcsr_getb (TRCSR);
#if 0
  if (trcsr & TIE)
    txinterrupts = 1;
#endif

#if defined(SSE_IKBD_6301_380) 
  HD6301.tdr=value;
#if defined(SSE_DEBUG_IKBD_6301_TRACE_SCI_TX)
  TRACE("6301 TDR %X\n",HD6301.tdr);
#endif
#endif
  if(ACIA_IKBD.LineRxBusy)
  {
#if defined(SSE_DEBUG_IKBD_6301_TRACE_SCI_TX)
    TRACE("6301 %X waits in TDR\n",value);
#endif
    ASSERT( !ACIA_IKBD.ByteWaitingRx ); // if it happened, it's just replaced
    ACIA_IKBD.ByteWaitingRx=1;
  }
  else

  {
    HD6301.tdrs=value;
#if defined(SSE_DEBUG_IKBD_6301_TRACE_SCI_TX)
    TRACE("6301 TDRS %X\n",HD6301.tdrs);
#endif
    keyboard_buffer_write_n_record(value); // call Steem's ikbd function
    txinterrupts=1;
  }
  return 0;//warning
}

