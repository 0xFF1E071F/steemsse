#pragma once
#ifndef MFP_DECLA_H
#define MFP_DECLA_H

#include "SSE/SSEOption.h"
#define EXT extern
#define INIT(s)

#if defined(SS_STRUCTURE_SSEDEBUG_OBJ)
#include <binary.h>
#endif

inline int abs_quick(int i) //was in emu.cpp (!)
{
  if (i>=0) return i;
  return -i;
}

#if defined(STEVEN_SEAGAL) && defined(SS_MFP_TIMER_B_NO_WOBBLE)
#define TB_TIME_WOBBLE (0) // no wobble for Timer B
#else
#define TB_TIME_WOBBLE (rand() & 4)
#endif

extern "C" void ASMCALL check_for_interrupts_pending();

EXT BYTE mfp_reg[24]; // 24 directly addressable internal registers, each 8bit

#define MFPR_GPIP 0 // ff fa01 MFP General Purpose I/O
#define MFPR_AER 1 // ff fa03 MFP Active Edge
#define MFPR_DDR 2 // ff fa05 MFP Data Direction
#define MFPR_IERA 3 // ff fa07 MFP Interrupt Enable A
#define MFPR_IERB 4 // ff fa09 MFP Interrupt Enable B
#define MFPR_IPRA 5 // ff fa0b MFP Interrupt Pending A
#define MFPR_IPRB 6 // ff fa0d MFP Interrupt Pending B
#define MFPR_ISRA 7 // ff fa0f MFP Interrupt In-Service A
#define MFPR_ISRB 8 // ff fa11 MFP Interrupt In-Service B
#define MFPR_IMRA 9 // ff fa13 MFP Interrupt Mask A
#define MFPR_IMRB 10 // ff fa15 MFP Interrupt Mask B
#define MFPR_VR 11 // ff fa17 MFP Vector
#define MFPR_TACR 12 // ff fa19 MFP Timer A Control
#define MFPR_TBCR 13 // ff fa1b MFP Timer B Control
#define MFPR_TCDCR 14 // ff fa1d MFP Timers C and D Control
#define MFPR_TADR 15 // ff fa1f  MFP Timer A Data
#define MFPR_TBDR 16 // ff fa21  MFP Timer B Data
#define MFPR_TCDR 17 // ff fa23  MFP Timer C Data
#define MFPR_TDDR 18 // ff fa25  MFP Timer D Data

// RS232
#define MFPR_SCR 19 // ff fa27 MFP Sync Character
#define MFPR_UCR 20 // ff fa29 MFP USART Control
#define MFPR_RSR 21 // ff fa2b MFP Receiver Status
#define MFPR_TSR 22 // ff fa2d MFP Transmitter Status
#define MFPR_UDR 23 // ff fa2f MFP USART Data

EXT BYTE mfp_gpip_no_interrupt INIT(0xf7);

#define MFP_INT_MONOCHROME_MONITOR_DETECT 15
#define MFP_INT_RS232_RING_INDICATOR 14
#define MFP_INT_TIMER_A 13
#define MFP_INT_RS232_RECEIVE_BUFFER_FULL 12
#define MFP_INT_RS232_RECEIVE_ERROR 11
#define MFP_INT_RS232_TRANSMIT_BUFFER_EMPTY 10
#define MFP_INT_RS232_TRANSMIT_ERROR 9
#define MFP_INT_TIMER_B 8
#define MFP_INT_FDC_AND_DMA 7
#define MFP_INT_ACIA 6  // Vector at $118
#define MFP_INT_TIMER_C 5
#define MFP_INT_TIMER_D 4
#define MFP_INT_BLITTER 3
#define MFP_INT_RS232_CTS 2
#define MFP_INT_RS232_DCD 1
#define MFP_INT_CENTRONICS_BUSY 0

#define MFP_GPIP_COLOUR BYTE(0x80)
#define MFP_GPIP_NOT_COLOUR BYTE(~0x80)
#define MFP_GPIP_CTS BYTE(BIT_2)
#define MFP_GPIP_DCD BYTE(BIT_1)
#define MFP_GPIP_RING BYTE(BIT_6)

#define MFP_GPIP_CENTRONICS_BIT 0
#define MFP_GPIP_DCD_BIT 1
#define MFP_GPIP_CTS_BIT 2
#define MFP_GPIP_BLITTER_BIT 3
#define MFP_GPIP_ACIA_BIT 4
#define MFP_GPIP_FDC_BIT 5
#define MFP_GPIP_RING_BIT 6
#define MFP_GPIP_MONO_BIT 7

//#ifdef IN_EMU

#if defined(SS_MFP_IRQ_DELAY3)
EXT int mfp_time_of_set_pending[16];
#endif

#if defined(SS_MFP_WRITE_DELAY1)
EXT int time_of_last_write_to_mfp_reg;
#endif

EXT BYTE mfp_gpip_input_buffer;

#define MFP_CLK 2451

#define MFP_CLK_EXACT 2451134 // Between 2451168 and 2451226 cycles

// Number of MFP clock ticks per 8000000 CPU cycles, very accurately tested!
// This is the most accurate number but we use the one above because Lethal Xcess
// won't work with this one.
//#define MFP_CLK_EXACT 2451182 // Between 2451168 and 2451226 cycles

//#define MFP_CLK_EXACT 2451034 // Between 2450992 and 2451050 (erring high) old version before 12 cycle delay

//#define MFP_CLK_EXACT 2450780  old version, checked inaccurately

#if defined(STEVEN_SEAGAL) && defined(SS_MFP_RATIO)
// it's a variable now! See SSE.h !!!!!!!!!!!!
#else
#define CPU_CYCLES_PER_MFP_CLK (8000000.0/double(MFP_CLK_EXACT))
#endif

#define CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED 20
//#define CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED 20
#define MFP_S_BIT (mfp_reg[MFPR_VR] & BIT_3)

//const int mfp_io_port_bit[16]={BIT_0,BIT_1,BIT_2,BIT_3,-1,-1,BIT_4,BIT_5,-1,-1,-1,-1,-1,-1,BIT_6,BIT_7};




inline BYTE mfp_get_timer_control_register(int);
EXT const int mfp_timer_8mhz_prescale[16];

EXT const int mfp_timer_irq[4];

void calc_time_of_next_timer_b();

EXT int mfp_timer_prescale[16];

EXT int mfp_timer_counter[4];
EXT int mfp_timer_timeout[4];
EXT bool mfp_timer_enabled[4];
EXT int mfp_timer_period[4];
#if defined(SS_MFP_RATIO_PRECISION)
EXT int mfp_timer_period_fraction[4];
EXT int mfp_timer_period_current_fraction[4];
#endif
EXT bool mfp_timer_period_change[4];
//int mfp_timer_prescale_counter[4]={0,0,0,0};

void mfp_set_timer_reg(int,BYTE,BYTE);
int mfp_calc_timer_counter(int);
void mfp_init_timers();
inline bool mfp_set_pending(int,int);

void mfp_gpip_set_bit(int,bool);

EXT const int mfp_gpip_irq[8];

//int mfp_gpip_timeout;

EXT bool mfp_interrupt_enabled[16];
EXT int mfp_time_of_start_of_last_interrupt[16];

EXT int cpu_time_of_first_mfp_tick;


#define MFP_CALC_INTERRUPTS_ENABLED	\
{	\
  int mask=1;	\
  for (int n=0;n<8;n++){	\
    mfp_interrupt_enabled[n]=mfp_reg[MFPR_IERB] & mask; mask<<=1;	\
  }	\
  mask=1;	\
  for (int n=0;n<8;n++){	\
    mfp_interrupt_enabled[n+8]=mfp_reg[MFPR_IERA] & mask; mask<<=1;	\
  }	\
}

#define MFP_CALC_TIMERS_ENABLED	\
	for (int n=0;n<4;n++){	\
		mfp_timer_enabled[n]=mfp_interrupt_enabled[mfp_timer_irq[n]] && (mfp_get_timer_control_register(n) & 7);	\
	}

//int mfp_timer_tick_countdown[4];
void mfp_interrupt(int,int);
//bool mfp_interrupt_enabled(int irq);
#if !defined(STEVEN_SEAGAL) // this function isn't used
void mfp_gpip_transition(int,bool);
void mfp_check_for_timer_timeouts(); // SS not implemented
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_MFP_RATIO_PRECISION)

#define MFP_CALC_TIMER_PERIOD(t)  mfp_timer_period[t]=int(  \
          double(mfp_timer_prescale[mfp_get_timer_control_register(t)]* \
            int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+t])))*CPU_CYCLES_PER_MFP_CLK);\
          mfp_timer_period_fraction[t]=int(  1000*((double(mfp_timer_prescale[mfp_get_timer_control_register(t)]*int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+t]))) * CPU_CYCLES_PER_MFP_CLK)-(double)mfp_timer_period[t])  );\
         ;// TRACE("MFP_CALC_TIMER_PERIOD %d\n",t);
         
#else

#define MFP_CALC_TIMER_PERIOD(t)  mfp_timer_period[t]=int(  \
          double(mfp_timer_prescale[mfp_get_timer_control_register(t)]* \
            int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+t])))*CPU_CYCLES_PER_MFP_CLK);

#endif



#define mfp_interrupt_i_bit(irq) (BYTE(1 << (irq & 7)))
#define mfp_interrupt_i_ab(irq) (1-((irq & 8) >> 3))

//TODO check the condition
#define mfp_interrupt_pend(irq,when_fired)                                       \
  if (mfp_interrupt_enabled[irq]){                             \
    LOG_ONLY( bool done= ) mfp_set_pending(irq,when_fired);   \
    LOG_ONLY( if (done==0) log_to(LOGSECTION_MFP_TIMERS,EasyStr("INTERRUPT: MFP IRQ #")+irq+" ("+    \
                                (char*)name_of_mfp_interrupt[irq]+") - can't set pending as MFP cleared "  \
                                "pending after timeout"); )             \
  } 


inline BYTE mfp_get_timer_control_register(int n) //was in mfp.cpp
{
  if (n==0){
    return mfp_reg[MFPR_TACR];
  }else if (n==1){
    return mfp_reg[MFPR_TBCR];
  }else if (n==2){
    return BYTE((mfp_reg[MFPR_TCDCR] & b01110000) >> 4);
  }else{
    return BYTE(mfp_reg[MFPR_TCDCR] & b00000111);
  }
}

inline bool mfp_set_pending(int irq,int when_set) //was in mfp.cpp
{
#if defined(STEVEN_SEAGAL) && defined(SS_MFP_IACK_LATENCY)
/*
Final Conflict

http://www.atari-forum.com/viewtopic.php?f=51&t=15885#p195733 (ijor):

The CPU takes several cycles between the moment it decides to take an interrupt
 (and which one), until it actually acks the interrupt to the MFP. 
And only at the latter time the MFP clears its pending bit (and interrupt 
state). The former happens (in this case) during execution of the RTE 
instruction. 
The latter on the Interrupt ACK bus cycle, part of the interrupt exception 
sequence. Let's call those two critical moments, "t1" and "t2".

Let's consider a timer interrupt happening just before "t2", the Int ACK 
cycle (interrupt is being processed as a consequence of the previous timer
 interrupt). The MFP would clear this new interrupt, even when the CPU
 meant to ack the previous one. In other words, MFP doesn't signal any
 interrupt any more, at least not until the next timer period. But if the
 timing is right, the next timer interrupt would happen just after "t1".
 So by the time the next interrupt happens, the CPU already dediced to 
either run the VBL interrupt, or the main code.
  =>
  Timer A is pending...      check interrupts... start IACK (12 cycles)
  During those 12 cycles, Timer A triggers again! On a real ST, the MFP
  clears the interrupt for both occurrences.
  In Steem:
  Without the fix, 56 cycles will be counted for first interrupt, then
  timers will be checked and Timer A will be set pending again.
  With the fix, being too close to former one, Timer A will be ignored.
  So we don't need to split the interrupt timings.
  Because of the way we implement it, the fix is considered a hack, but
  I've seen nothing broken by it yet (since v3.3).
  CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED = 20
  But the same concept for HBL = 28
  We change those values to 12? 16?

  TODO: This only takes care of 'double pending' case, but there's also
  the 'higher int during IACK' possibility (cases?)

*/
  if(abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])
    >= (SSE_HACKS_ON
    ?56-8+CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED
    :CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED)){
#else
  if (abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])>=CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED){
#endif
    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq); // Set pending
#if defined(SS_MFP_IRQ_DELAY3)
    mfp_time_of_set_pending[irq]=when_set; // not ACT
#endif
    return true;
  }
  return (mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)] & mfp_interrupt_i_bit(irq))!=0;
}


//#endif//in_emu

#undef EXT
#undef INIT

#endif//MFP_DECLA_H