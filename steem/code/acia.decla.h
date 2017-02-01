#pragma once
#ifndef ACIA_H
#define ACIA_H
/*  This file has been added to clarify and improve ACIA emulation.
    TODO: create acia.cpp, move code around
*/

#if defined(SSE_ACIA)

#include <binary.h>

#ifdef __cplusplus
#include "emulator.decla.h"
#include "steemh.decla.h"
#include "SSE/SSEDebug.h"
#include "SSE/SSEOption.h"
extern "C" {  // necessary for VC6
#endif



#pragma pack(push, STRUCTURE_ALIGNMENT)

struct ACIA_STRUCT{ // removed _ ..
#if defined(SSE_VAR_RESIZE) // problem is memory snapshots, structure: more complicated
  int last_tx_write_time;
  int last_rx_read_time;
#if defined(SSE_ACIA_EVENT)
  int time_of_event_incoming, time_of_event_outgoing;
#endif
  BYTE clock_divide;
  BYTE rx_irq_enabled;
  BYTE rx_not_read;
  BYTE overrun;
  BYTE tx_flag;
  BYTE tx_irq_enabled;
  BYTE data;
  BYTE irq;
#else
  int clock_divide;
  int rx_delay__unused;
  BYTE rx_irq_enabled;
  BYTE rx_not_read;
  int overrun;
  int tx_flag;
  BYTE tx_irq_enabled;
  BYTE data;
  BYTE irq;
  int last_tx_write_time;
  int last_rx_read_time;
#endif
  BYTE LineRxBusy; // receiveing from 6301 or MIDI
  BYTE ByteWaitingRx; // Byte in 6301's or MIDI's TDR is waiting to be shifted

/*  When a byte is being shifted in the shift register and being transmitted,
    another byte may already be transmitted in the data register, it will wait
    there until the first transmission is over.
*/
  BYTE ByteWaitingTx;
  BYTE LineTxBusy; // transmitting to 6301 or MIDI


#if defined(SSE_ACIA)
  BYTE CR,  // control 
    SR,     // status
    RDR,    // receive data 
    TDR,    // transmit data
    RDRS,   // receive data shift
    TDRS;   // transmit data shift
#ifdef __cplusplus
  // FUNCTIONS
  inline bool IrqForTx() { return ((CR&BIT_5)&&!(CR&BIT_6)); }
  void BusJam(unsigned long addr);
  int TransmissionTime();
#endif
#endif
};

#pragma pack(pop)

extern struct ACIA_STRUCT acia[2]; 

#ifdef __cplusplus
}//extern "C"
#endif

enum EAciaOverrun { ACIA_OVERRUN_NO,ACIA_OVERRUN_COMING,ACIA_OVERRUN_YES };
enum EAciaChips { NUM_ACIA_IKBD,NUM_ACIA_MIDI };

#define ACIA_IKBD acia[NUM_ACIA_IKBD]
#define ACIA_MIDI acia[NUM_ACIA_MIDI]
#define ACIA_CYCLES_NEEDED_TO_START_TX 512 //need if !SSE_IKBD

#endif

#endif