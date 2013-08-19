#pragma once
#ifndef ACIA_H
#define ACIA_H
/*  This file has been added to clarify and improve ACIA emulation.
*/

#if defined(STEVEN_SEAGAL) && defined(SS_ACIA)


#ifdef __cplusplus
extern "C" {  // necessary for VC6
#endif


struct ACIA_STRUCT{ // removed _ ..
  int clock_divide; //OLD_VARIABLES?
#if !defined(SS_ACIA_REMOVE_OLD_VARIABLES)
  int rx_delay__unused; //SS see this?
  BYTE rx_irq_enabled;
  BYTE rx_not_read;
#endif
  int overrun;

  int tx_flag;
#if !defined(SS_ACIA_REMOVE_OLD_VARIABLES)
  BYTE tx_irq_enabled;
#endif
  BYTE data;
#if !defined(SS_ACIA_REMOVE_OLD_VARIABLES)
  BYTE irq;
#endif
  int last_tx_write_time;
  int last_rx_read_time;

#if defined(SS_ACIA_IRQ_DELAY)// not defined anymore (v3.5.2), see MFP
/*  This is to help implement the short delay between RX set and IRQ set,
    as mentioned in Hatari.
    1: RX will be set at right time 2: IRQ will be (some cycles later)
*/
  int rx_stage; 
#endif
#if defined(SS_ACIA_DOUBLE_BUFFER_RX)
  BYTE LineRxBusy; // receiveing from 6301 or MIDI
  BYTE ByteWaitingRx; // Byte in 6301's TDR is waiting to be shifted
#endif
#if defined(SS_ACIA_DOUBLE_BUFFER_TX)
/*  When a byte is being shifted in the shift register and being transmitted,
    another byte may already be transmitted in the data register, it will wait
    there until the first transmission is over.
*/
  BYTE ByteWaitingTx;
  BYTE LineTxBusy; // transmitting to 6301 or MIDI
#if !defined(SS_ACIA_REGISTERS)
  BYTE WaitingByte; // would be TDR before going to TDRS
#endif
#endif

#if defined(SS_ACIA_REGISTERS)
  // this should make things clearer
  BYTE CR,  // control 
    SR,     // status
    RDR,    // receive data 
    TDR,    // transmit data
    RDRS,   // receive data shift
    TDRS;   // transmit data shift
#ifdef __cplusplus
  inline bool IrqForTx() { return ((CR&BIT_5)&&!(CR&BIT_6)); }
#endif
#endif
};

extern struct ACIA_STRUCT acia[2]; 

#ifdef __cplusplus
}//extern "C"
#endif

enum { ACIA_OVERRUN_NO,ACIA_OVERRUN_COMING,ACIA_OVERRUN_YES };
enum { NUM_ACIA_IKBD,NUM_ACIA_MIDI };

#define ACIA_IKBD acia[NUM_ACIA_IKBD]
#define ACIA_MIDI acia[NUM_ACIA_MIDI]

#define ACIA_CYCLES_NEEDED_TO_START_TX 512 //need if !SS_IKBD

#endif

#endif