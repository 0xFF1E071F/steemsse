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

#define LOGSECTION LOGSECTION_ACIA

extern "C" {  // necessary for VC6
#endif



#pragma pack(push, STRUCTURE_ALIGNMENT)

struct ACIA_STRUCT{ // removed _ ..

#if defined(SSE_VAR_RESIZE) // problem is memory snapshots, structure: more complicated
#if defined(SSE_TIMINGS_CPUTIMER64)
  COUNTER_VAR last_tx_write_time;
  COUNTER_VAR last_rx_read_time;
#if defined(SSE_ACIA_EVENT)
  COUNTER_VAR time_of_event_incoming, time_of_event_outgoing;
#endif
#else
  int last_tx_write_time;
  int last_rx_read_time;
#if defined(SSE_ACIA_EVENT)
  int time_of_event_incoming, time_of_event_outgoing;
#endif
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
#if !defined(SSE_ACIA_393)
  BYTE ByteWaitingRx; // Byte in 6301's or MIDI's TDR is waiting to be shifted
#endif
/*  When a byte is being shifted in the shift register and being transmitted,
    another byte may already be transmitted in the data register, it will wait
    there until the first transmission is over.
*/
#if !defined(SSE_ACIA_393)
  BYTE ByteWaitingTx;
#endif
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
#if defined(SSE_ACIA_393)
  inline bool CheckIrq() {
    bool irq=IrqForTx() && (SR&BIT_1) // TX
      || (CR&BIT_7) && (SR&(BIT_0|BIT_5)); //RX/OVR
    if(irq) {
      SR|=BIT_7;
      TRACE_LOG("ACIA %d IRQ, SR=%X\n",(CR&1),SR);
    }
    else
      SR&=~BIT_7;
    return irq;
  }
#define ACIA_CHECK_IRQ(i) {acia[i].CheckIrq(); \
  mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!( (ACIA_IKBD.SR&BIT_7) \
      || (ACIA_MIDI.SR&BIT_7))); \
  }
  void TransmitTDR();
  inline int TransmissionTime() { //no more hack for Froggies, can be inlined
/*  
Compute cycles according to bits 0-1 of CR
SS1 SS0                  Speed (bit/s)
 0   0    Normal            500000
 0   1    Div by 16          31250 (ST: MIDI)   CR&1=1
 1   0    Div by 64         7812.5 (ST: IKBD)   CR&1=0
MIDI transmission is 4 times faster than IKBD
*/
    int cycles=(CR&1)?10*16*16:10*16*64; //2560 for MIDI, 10240 for 6301
    return cycles;
  }
#else
  int TransmissionTime();
#endif

#endif

#endif
};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

extern struct ACIA_STRUCT acia[2]; 

#ifdef __cplusplus
}//extern "C"
#endif

enum EAciaOverrun { ACIA_OVERRUN_NO,ACIA_OVERRUN_COMING,ACIA_OVERRUN_YES };
enum EAciaChips { NUM_ACIA_IKBD,NUM_ACIA_MIDI };

#define ACIA_IKBD acia[NUM_ACIA_IKBD]
#define ACIA_MIDI acia[NUM_ACIA_MIDI]
#define ACIA_CYCLES_NEEDED_TO_START_TX 512 //need if !SSE_IKBD

#undef LOGSECTION

#endif //SSE_ACIA

#endif //guard
