#pragma once
#ifndef MFP_DECLA_H
#define MFP_DECLA_H

#include "SSE/SSEOption.h"
#include "SSE/SSEDebug.h"

#define EXT extern
#define INIT(s)

#if defined(SSE_STRUCTURE_SSEDEBUG_OBJ)
#include <binary.h>
#endif

inline int abs_quick(int i) //was in emu.cpp (!)
{
  if (i>=0) return i;
  return -i;
}

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_TIMER_B_NO_WOBBLE)
#define TB_TIME_WOBBLE (0) // no wobble for Timer B 
// there's wobble, confirmed by TIMERB01.TOS; TIMERB03.TOS; could be 2?


//TODO timer b wobble = gpip delay  -> one or the other?
//and no delay timer wobble?
//who knows?

#elif defined(SSE_INT_MFP_TIMER_B_WOBBLE2)
#define TB_TIME_WOBBLE (rand() & 2)
#else
#define TB_TIME_WOBBLE (rand() & 4)
#endif

extern "C" void ASMCALL check_for_interrupts_pending();

EXT BYTE mfp_reg[24]; // 24 directly addressable internal registers, each 8bit


#if defined(SSE_INT_MFP_REFACTOR3)
enum EMfpRegs {
MFPR_GPIP= 0, // ff fa01 MFP General Purpose I/O
MFPR_AER= 1, // ff fa03 MFP Active Edge
MFPR_DDR= 2, // ff fa05 MFP Data Direction
MFPR_IERA= 3, // ff fa07 MFP Interrupt Enable A
MFPR_IERB= 4, // ff fa09 MFP Interrupt Enable B
MFPR_IPRA= 5, // ff fa0b MFP Interrupt Pending A
MFPR_IPRB= 6, // ff fa0d MFP Interrupt Pending B
MFPR_ISRA= 7, // ff fa0f MFP Interrupt In-Service A
MFPR_ISRB= 8, // ff fa11 MFP Interrupt In-Service B
MFPR_IMRA= 9, // ff fa13 MFP Interrupt Mask A
MFPR_IMRB= 10, // ff fa15 MFP Interrupt Mask B
MFPR_VR= 11, // ff fa17 MFP Vector
MFPR_TACR= 12, // ff fa19 MFP Timer A Control
MFPR_TBCR= 13, // ff fa1b MFP Timer B Control
MFPR_TCDCR= 14, // ff fa1d MFP Timers C and D Control
MFPR_TADR= 15, // ff fa1f  MFP Timer A Data
MFPR_TBDR= 16, // ff fa21  MFP Timer B Data
MFPR_TCDR= 17, // ff fa23  MFP Timer C Data
MFPR_TDDR= 18, // ff fa25  MFP Timer D Data

// RS232
MFPR_SCR= 19, // ff fa27 MFP Sync Character
MFPR_UCR= 20, // ff fa29 MFP USART Control
MFPR_RSR= 21, // ff fa2b MFP Receiver Status
MFPR_TSR= 22, // ff fa2d MFP Transmitter Status
MFPR_UDR= 23 // ff fa2f MFP USART Data
};
#else
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

#endif

EXT BYTE mfp_gpip_no_interrupt INIT(0xf7);

#if defined(SSE_INT_MFP_REFACTOR3)
enum EMfpInterrupts{
MFP_INT_MONOCHROME_MONITOR_DETECT =15,
MFP_INT_RS232_RING_INDICATOR= 14,
MFP_INT_TIMER_A= 13,
MFP_INT_RS232_RECEIVE_BUFFER_FULL= 12,
MFP_INT_RS232_RECEIVE_ERROR =11,
MFP_INT_RS232_TRANSMIT_BUFFER_EMPTY= 10,
MFP_INT_RS232_TRANSMIT_ERROR =9,
MFP_INT_TIMER_B =8,
MFP_INT_FDC_AND_DMA =7,
MFP_INT_ACIA =6,  // Vector at $118
MFP_INT_TIMER_C =5,
MFP_INT_TIMER_D =4,
MFP_INT_BLITTER =3,
MFP_INT_RS232_CTS =2,
MFP_INT_RS232_DCD =1,
MFP_INT_CENTRONICS_BUSY= 0
};
#else
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
#endif

#define MFP_GPIP_COLOUR BYTE(0x80)
#define MFP_GPIP_NOT_COLOUR BYTE(~0x80)
#define MFP_GPIP_CTS BYTE(BIT_2)
#define MFP_GPIP_DCD BYTE(BIT_1)
#define MFP_GPIP_RING BYTE(BIT_6)


#if defined(SSE_INT_MFP_REFACTOR3)
enum {
MFP_GPIP_CENTRONICS_BIT =0,
MFP_GPIP_DCD_BIT =1,
MFP_GPIP_CTS_BIT =2,
MFP_GPIP_BLITTER_BIT =3,
MFP_GPIP_ACIA_BIT =4,
MFP_GPIP_FDC_BIT= 5,
MFP_GPIP_RING_BIT =6,
MFP_GPIP_MONO_BIT =7
};
#else
#define MFP_GPIP_CENTRONICS_BIT 0
#define MFP_GPIP_DCD_BIT 1
#define MFP_GPIP_CTS_BIT 2
#define MFP_GPIP_BLITTER_BIT 3
#define MFP_GPIP_ACIA_BIT 4
#define MFP_GPIP_FDC_BIT 5
#define MFP_GPIP_RING_BIT 6
#define MFP_GPIP_MONO_BIT 7
#endif
//#ifdef IN_EMU

#if defined(SSE_INT_MFP_IRQ_DELAY3)
EXT int mfp_time_of_set_pending[16];
#endif

#if defined(SSE_INT_MFP_WRITE_DELAY1)//no (it's in 68901 object)
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

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO)
// it's a variable now! See SSEDecla.h !!!!!!!!!!!!
#else
#define CPU_CYCLES_PER_MFP_CLK (8000000.0/double(MFP_CLK_EXACT))
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_IACK_LATENCY)//no
#define CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED (SSE_HACKS_ON?32:28)
#else
#define CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED 20
#endif

#define MFP_S_BIT (mfp_reg[MFPR_VR] & BIT_3)

//const int mfp_io_port_bit[16]={BIT_0,BIT_1,BIT_2,BIT_3,-1,-1,BIT_4,BIT_5,-1,-1,-1,-1,-1,-1,BIT_6,BIT_7};



#if defined(SSE_INT_MFP_OBJECT) //created v3.7
/*  We create this object for some additions, we don't transfer
    all MFP emu into it. We don't create a SSE file yet.
*/

#if defined(SSE_INT_MFP_UTIL)

struct TMC68901IrqInfo {
  unsigned int IsGpip:1;
  unsigned int IsTimer:1;
  unsigned int Timer:4;
};

#endif

struct TMC68901 {

#if defined(SSE_INT_MFP_IRQ_TIMING)

  TMC68901();
  void Init();
  bool Irq;  // asserted or not, problem, we're not in real-time
  char NextIrq; //-1 reset
  char LastIrq;
  char IrqInService;
  BYTE LastRegisterWritten;
  BYTE LastRegisterFormerValue;
  BYTE LastRegisterWrittenValue;
  /*
  WORD IER;
  WORD IMR;
  WORD ISR;
  */
  BYTE Vector;
  WORD IPR;
  int IrqSetTime;
  int IackTiming;
#if defined(SSE_INT_MFP_REFACTOR2)
  void Reset(bool Cold);
  int UpdateNextIrq(int at_time=-1);
#else
  int UpdateNextIrq(int start_from_irq=15,int at_time=-1);
#endif
  int WriteTiming;
#if defined(SSE_INT_MFP_TIMERS_WOBBLE)
  BYTE Wobble[4];
#endif
#if defined(SSE_INT_MFP_IACK_LATENCY4)//no
  BYTE SkipTimer[4];
#endif
#if defined(SSE_INT_MFP_UTIL)
  TMC68901IrqInfo IrqInfo[16];
#if !defined(SSE_INT_MFP_EVENT_WRITE) //see all the junk that disappears
  BYTE GetReg(int reg_num,int at_time=-1);
  bool Enabled(int irq, int at_time=-1);
  bool InService(int irq, int at_time=-1);
  bool MaskOK(int irq, int at_time=-1);
  bool Pending(int irq, int at_time=-1);
#endif
#if !defined(SSE_INT_MFP_REFACTOR2)
  bool TimerBActive();
#endif
#endif
#if defined(SSE_INT_MFP_RECORD_PENDING_TIMING)
  int PendingTiming[16];
#endif
#if defined(SSE_INT_MFP_SPURIOUS) && !defined(SSE_INT_MFP_REFACTOR2)
  bool CheckSpurious(int irq);
#endif
#if defined(SSE_INT_MFP_TIMER_B_AER2)
  void CalcCyclesFromHblToTimerB(int freq);
#endif
#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS)
  void AdjustTimerB();
#endif
#if defined(SSE_INT_MFP_EVENT_WRITE)
  bool WritePending;
#endif
#endif
};

#if defined(SSE_INT_MFP_TIMER_B_AER2)
#define CALC_CYCLES_FROM_HBL_TO_TIMER_B(freq) MC68901.CalcCyclesFromHblToTimerB(freq)
#endif

extern TMC68901 MC68901; // declaring the singleton

#endif//SSE_INT_MFP_OBJECT


inline BYTE mfp_get_timer_control_register(int);

#if defined(SSE_VAR_RESIZE_370)
EXT const WORD mfp_timer_8mhz_prescale[16];
EXT const BYTE mfp_timer_irq[4];
EXT const BYTE mfp_gpip_irq[8];
#else
EXT const int mfp_timer_8mhz_prescale[16];
EXT const int mfp_timer_irq[4];
EXT const int mfp_gpip_irq[8];
#endif

void calc_time_of_next_timer_b();

EXT int mfp_timer_prescale[16];

EXT int mfp_timer_counter[4];
EXT int mfp_timer_timeout[4];
EXT bool mfp_timer_enabled[4];
EXT int mfp_timer_period[4];
#if defined(SSE_INT_MFP_RATIO_PRECISION)
EXT int mfp_timer_period_fraction[4];
EXT int mfp_timer_period_current_fraction[4];
#endif
EXT bool mfp_timer_period_change[4];
//int mfp_timer_prescale_counter[4]={0,0,0,0};

void mfp_set_timer_reg(int,BYTE,BYTE);
int mfp_calc_timer_counter(int);
void mfp_init_timers();
#if defined(SSE_INT_MFP_REFACTOR1)
bool mfp_set_pending(int,int);
#else
inline bool mfp_set_pending(int,int);
#endif
void mfp_gpip_set_bit(int,bool);

//int mfp_gpip_timeout;

EXT bool mfp_interrupt_enabled[16];
EXT int mfp_time_of_start_of_last_interrupt[16];
#if !defined(SSE_INT_MFP_TIMERS_BASETIME)
EXT int cpu_time_of_first_mfp_tick;
#endif

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

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO_PRECISION)

#define MFP_CALC_TIMER_PERIOD(t)  mfp_timer_period[t]=int(  \
          double(mfp_timer_prescale[mfp_get_timer_control_register(t)]* \
            int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+t])))*CPU_CYCLES_PER_MFP_CLK);\
          mfp_timer_period_fraction[t]=int(  1000*((double(mfp_timer_prescale[mfp_get_timer_control_register(t)]*int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+t]))) * CPU_CYCLES_PER_MFP_CLK)-(double)mfp_timer_period[t])  );\
         mfp_timer_period_current_fraction[t]=0;// TRACE("MFP_CALC_TIMER_PERIOD %d\n",t);
         
#else

#define MFP_CALC_TIMER_PERIOD(t)  mfp_timer_period[t]=int(  \
          double(mfp_timer_prescale[mfp_get_timer_control_register(t)]* \
            int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+t])))*CPU_CYCLES_PER_MFP_CLK);

#endif



#define mfp_interrupt_i_bit(irq) (BYTE(1 << (irq & 7)))
#define mfp_interrupt_i_ab(irq) (1-((irq & 8) >> 3))


#if defined(SSE_INT_MFP_REFACTOR2) // instant IRQ detector (if no irq in service...)
#define MFP_IRQ ( mfp_reg[MFPR_IERA]&mfp_reg[MFPR_IPRA]&mfp_reg[MFPR_IMRA]\
                &(~mfp_reg[MFPR_ISRA]) \
               || mfp_reg[MFPR_IERB]&mfp_reg[MFPR_IPRB]&mfp_reg[MFPR_IMRB]\
               &(~mfp_reg[MFPR_ISRB]))
#endif

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

#if !defined(SSE_INT_MFP_REFACTOR1) // back to mfp.cpp!

inline bool mfp_set_pending(int irq,int when_set) //was in mfp.cpp
{
  ASSERT(mfp_interrupt_enabled[irq]);
#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_IACK_LATENCY)
  if(abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])
    >= (SSE_HACKS_ON
    ?56-8+CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED
    :CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED)){
#else
  if (abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])>=CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED){
#endif
    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq); // Set pending
#if defined(SSE_INT_MFP_IRQ_DELAY3)
    mfp_time_of_set_pending[irq]=when_set; // not ACT
#endif
    return true;
  }
  return (mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)] & mfp_interrupt_i_bit(irq))!=0;
}

#endif

//#endif//in_emu

#undef EXT
#undef INIT
#undef LOGSECTION //SS
#endif//MFP_DECLA_H