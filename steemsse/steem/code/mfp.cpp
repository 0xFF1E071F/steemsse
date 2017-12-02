/*---------------------------------------------------------------------------
FILE: mfp.cpp
MODULE: emu
DESCRIPTION: The core of Steem's Multi Function Processor emulation
(MFP 68901). This chip handles most of the interrupt and timing functions in
the ST.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: mfp.cpp")
#endif

#if defined(SSE_BUILD)

#define EXT
#define INIT(s) =s

EXT BYTE mfp_reg[24]; // 24 directly addressable internal registers, each 8bit
EXT BYTE mfp_gpip_no_interrupt INIT(0xf7);
BYTE mfp_gpip_input_buffer=0;

#if defined(SSE_VAR_RESIZE)
const BYTE mfp_timer_irq[4]={13,8,5,4};
const BYTE mfp_gpip_irq[8]={0,1,2,3,6,7,14,15};
#else
const int mfp_timer_irq[4]={13,8,5,4};
const int mfp_gpip_irq[8]={0,1,2,3,6,7,14,15};
#endif

#if defined(SSE_TIMING_MULTIPLIER_392)
const int mfp_timer_prescale[16]={65535,4,10,16,50,64,100,200,
                            65535,4,10,16,50,64,100,200};
#else
int mfp_timer_prescale[16]={65535,4,10,16,50,64,100,200,
                            65535,4,10,16,50,64,100,200};
const int mfp_timer_8mhz_prescale[16]={65535,4,10,16,50,64,100,200,65535,4,10,16,50,64,100,200};
#endif

int mfp_timer_counter[4];

#if defined(SSE_TIMINGS_CPUTIMER64)
COUNTER_VAR mfp_timer_timeout[4];
COUNTER_VAR mfp_time_of_start_of_last_interrupt[16];
COUNTER_VAR cpu_time_of_first_mfp_tick;
#else
int mfp_timer_timeout[4];
int mfp_time_of_start_of_last_interrupt[16];
int cpu_time_of_first_mfp_tick;
#endif

bool mfp_timer_enabled[4]={0,0,0,0};
int mfp_timer_period[4]={10000,10000,10000,10000};

#if defined(SSE_INT_MFP_RATIO_PRECISION)
int mfp_timer_period_fraction[4];
int mfp_timer_period_current_fraction[4];
#endif

bool mfp_timer_period_change[4]={0,0,0,0};
bool mfp_interrupt_enabled[16];

#undef EXT
#undef INIT

#include "SSE/SSEGlue.h"
#endif

#if defined(SSE_INT_MFP)
TMC68901 MC68901; // singleton, the infamous MFP
#endif

#ifdef SSE_DEBUG
#define LOGSECTION LOGSECTION_MFP
char* mfp_reg_name[]={"GPIP","AER","DDR","IERA","IERB","IPRA","IPRB","ISRA",
  "ISRB","IMRA","IMRB","VR","TACR","TBCR","TCDCR","TADR","TBDR","TCDR","TDDR",
  "SCR","UCR","RSR","TSR","UDR"};
#endif

//---------------------------------------------------------------------------

void mfp_gpip_set_bit(int bit,bool set)
{
/*
The GPIP has three associated registers. One allows the programmer to specify
the Active Edge for each bit that will trigger an interrupt. Another register
specifies the Data Direction (input or output) associated with each bit. The
third register is the actual data I/O register used to input or output data
to the port. These three registers are illustrated below.

General Purpose I/O Registers

                    Active Edge Register
Port 1 (AER)  GPIP GPIP GPIP GPIP GPIP GPIP GPIP GPIP   1=Rising
               7    6    5    4    3    2    1    0     0=Falling

                    Data Direction Register
Port 2 (DDR)  GPIP GPIP GPIP GPIP GPIP GPIP GPIP GPIP   1=Output
               7    6    5    4    3    2    1    0     0=Input

-                    General Purpose I/O Register
Port 3 (GPIP) GPIP GPIP GPIP GPIP GPIP GPIP GPIP GPIP
               7    6    5    4    3    2    1    0

Practically on the ST, the request is placed by clearing the bit in the GPIP.

*/ 
  BYTE mask=BYTE(1 << bit); //SS get the bit in hexa //is cast useless?
  BYTE set_mask=BYTE(set ? mask:0); //SS same or 0 if we clear
  BYTE cur_val=(mfp_reg[MFPR_GPIP] & mask); //SS state of that GPIP bit
  if (cur_val==set_mask) return; //no change
  //SS detects if we're setting an IRQ (but not the reverse: Hackabonds)
  bool old_1_to_0_detector_input=(cur_val ^ (mfp_reg[MFPR_AER] & mask))==mask;
  mfp_reg[MFPR_GPIP]&=BYTE(~mask); //SS zero the target bit, leaving others
  mfp_reg[MFPR_GPIP]|=set_mask; //SS set target bit if needed
  // If the DDR bit is low then the bit from the io line is used,
  // if it is high interrupts then it comes from the input buffer.
  // In that case interrupts are handled in the write to the GPIP.
  if (old_1_to_0_detector_input && (mfp_reg[MFPR_DDR] & mask)==0){
    // Transition the right way! Make the interrupt pend (don't cause an intr
    // straight away in case another more important one has just happened).
    mfp_interrupt_pend(mfp_gpip_irq[bit],ABSOLUTE_CPU_TIME);
    ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
  }
}
//---------------------------------------------------------------------------
void calc_time_of_next_timer_b() //SS called only by mfp_set_timer_reg()
{
#if defined(SSE_INT_MFP_TIMER_B_392)
  if(OPTION_C2)
  {
    // use our new function instead
    MC68901.ComputeNextTimerB(TMC68901::SettingTimer); 
    return;
  }
#endif

  int cycles_in=int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
  if (cycles_in<cpu_cycles_from_hbl_to_timer_b){
    if (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line){
        time_of_next_timer_b=cpu_timer_at_start_of_hbl+cpu_cycles_from_hbl_to_timer_b
        +TB_TIME_WOBBLE;
    }else{
      time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
    }
  }else{
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
  }
}
//---------------------------------------------------------------------------
#if defined(SSE_BUILD)
// inline functions -> mfp.decla.h
#else // Steem 3.2 build, using mfp.h
inline BYTE mfp_get_timer_control_register(int n)
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

inline bool mfp_set_pending(int irq,int when_set)
{
  if (abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])>=CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED){
    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq); // Set pending
    return true;
  }
  return (mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)] & mfp_interrupt_i_bit(irq))!=0;
}
#endif

#if defined(SSE_INT_MFP_INLINE) 

bool mfp_set_pending(int irq,COUNTER_VAR when_set) {
  if(
#if defined(SSE_INT_MFP_IACK)
    OPTION_C2 || 
#endif
    (abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])
    >=CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED))
  {
#if defined(SSE_DEBUG)
    bool was_already_pending=(mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]&mfp_interrupt_i_bit(irq));
#endif
    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq); // Set pending

#if defined(SSE_BLT_MAIN_LOOP)
/*  It's a blitter switch because with our loop change, we want to make
    sure that MFP timer IRQ is checked after the instruction where the
    blit occurred. 
    In previous versions of Steem, this flag was set for GPIP interrupts
    but not timers, because interrupts were checked at all events anyway.
*/
    ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; 
#endif

    if(OPTION_C2)
    {
#if defined(SSE_INT_MFP_GPIP_TO_IRQ_DELAY)     
      if(MC68901.IrqInfo[irq].IsGpip)
        when_set+=4; // documented delay between input transition and IRQ
#endif
#if defined(SSE_DEBUG)
      if(was_already_pending)
        TRACE_MFP("%d MFP irq %d pending again\n",when_set,irq);
      else
        TRACE_MFP("%d MFP irq %d pending\n",when_set,irq);
#endif
#if defined(SSE_INT_MFP) 
      MC68901.UpdateNextIrq(when_set);
#endif
    }
    return true;
  }
#if defined(SSE_DEBUG)
  else
  {
    TRACE_LOG("MFP irq %d pending included in former because delay=%d\n",irq,abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq]));
    TRACE_OSD("IACK %d  %d",irq,abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq]));
  }
#endif
  return (mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)] & mfp_interrupt_i_bit(irq))!=0;
}

#endif

/*
  The four timers are programmed via three Timer Control Registers and four
  Timer Data Registers. Timers A and B are controlled bu the control registers
  TACR and TBCR, respectively, and by the data registers TADR and TBDR. Timers
  C and D are controlled by the control register TCDCR and two data registers
  TCDR and TDDR. Bits in the control registers allow the selection of
  operational mode, prescale, and control, while data registers are used to
  read the timer or write into the time constant register. Timer A and B input
  pins, TAI and TBI, are used for the event and pulse width modes for timers A
  and B.
*/
#if !defined(SSE_DEBUG)
#define LOGSECTION LOGSECTION_MFP_TIMERS
#endif

void mfp_set_timer_reg(int reg,BYTE old_val,BYTE new_val)
{
  ASSERT(reg>=MFPR_TACR && reg<=MFPR_TCDCR || reg>=MFPR_TADR && reg<=MFPR_TDDR); // data too!
  int timer=0; // SS 0=Timer A 1=Timer B 2=Timer C 3=Timer D
  BYTE new_control;

  if (reg>=MFPR_TACR && reg<=MFPR_TCDCR){ //control reg change
    new_control=BYTE(new_val & 15);
    switch (reg){
      case MFPR_TACR: timer=0; break;
      case MFPR_TBCR: timer=1; break;
      case MFPR_TCDCR: //TCDCR
        timer=2; //we'll do D too
        new_control=BYTE((new_val >> 4) & 7);
        break;
    }
    ASSERT(timer<4);
#if defined(SSE_INT_MFP_TIMERS_STARTING_DELAY)
/*  Steem authors shift 1st timeout 12 cycles later, we use a parameter
    to test that.
    This is explained by internal MFP delays, but it also covers a pre-IRQ
    delay.
*/
    INSTRUCTION_TIME(OPTION_C2?MFP_TIMER_SET_DELAY:12); // SSEParameters.h 8
#else
    INSTRUCTION_TIME(12); // The MFP doesn't do anything until 12 cycles after the write
#endif
    do{ //SS this do to do D
      if (mfp_get_timer_control_register(timer)!=new_control){
        new_control&=7;
        dbg_log( EasyStr("MFP: ")+HEXSl(old_pc,6)+" - Changing timer "+char('A'+timer)+" control; current time="+
              ABSOLUTE_CPU_TIME+"; old timeout="+mfp_timer_timeout[timer]+";"
              "\r\n           ("+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)+
              " cycles into scanline #"+scan_y+")" );
/*  MC68901 doc:
"When the timer is stopped, counting is inhibited. The contents of the timer's
main counter are not affected although any residual count in the prescaler is lost."
-> When starting a timer, the MFP doesn't reload the main counter with the data
register. This is of course correct in Steem but hey, I didn't even know that.
*/
        // This ensures that mfp_timer_counter is set to the correct value just
        // in case the data register is read while timer is stopped or the timer
        // is restarted before a write to the data register.
        // prescale_count is the number of MFP_CLKs there has been since the
        // counter last decreased.
        int prescale_count=mfp_calc_timer_counter(timer);
        //TRACE_LOG("Timer %c main %d prescale %d\n",'A'+timer,mfp_timer_counter[timer],prescale_count);

        if (new_control){ // Timer running in delay mode
                          // SS or pulse, but it's very unlikely (not emulated)

          mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME; //SS as modified
          //SS compute next timeout, which depends on current counter + prescale

#if defined(SSE_INT_MFP_RATIO_PRECISION)
          double precise_cycles0=mfp_timer_prescale[new_control]
            *mfp_timer_counter[timer]/64;
#if defined(SSE_INT_MFP_PRESCALE)
          if(OPTION_C2)
            precise_cycles0-=prescale_count; // we count it here now
#endif
          precise_cycles0*=CPU_CYCLES_PER_MFP_CLK;
#if defined(SSE_TIMING_MULTIPLIER_392)
          precise_cycles0*=cpu_cycles_multiplier;
#endif
          mfp_timer_timeout[timer]+=(int)precise_cycles0;
          if(OPTION_C2 && (precise_cycles0-(int)precise_cycles0)) //generally
            mfp_timer_timeout[timer]++; 
#else
          mfp_timer_timeout[timer]+=int(double(mfp_timer_prescale[new_control]
            *mfp_timer_counter[timer]/64)*CPU_CYCLES_PER_MFP_CLK);
//          mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME+
//              (mfp_timer_prescale[new_control]*(mfp_timer_counter[timer])*125)/MFP_CLK;
              //*8000/MFP_CLK for MFP cycles, /64 for counter resolution
#endif
          mfp_timer_enabled[timer]=mfp_interrupt_enabled[mfp_timer_irq[timer]];

#if defined(SSE_INT_MFP_RATIO_PRECISION)
/*  Here we do exactly what Steem authors suggest just below, and it does bring
    the timing measurements at the same level as SainT and Hatari, at least in
    HWTST001.PRG by ljbk, so it's definitely an improvement, and it isn't 
    complicated at all!
*/
          double precise_cycles= mfp_timer_prescale[new_control]
            *(int)(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer]))
#if defined(SSE_TIMING_MULTIPLIER_392) //let's not forget this...
            *cpu_cycles_multiplier
#endif
            *CPU_CYCLES_PER_MFP_CLK;
          mfp_timer_period[timer]=(int)precise_cycles;
          if(OPTION_C2)
          {
            mfp_timer_period_fraction[timer]
              =(precise_cycles-mfp_timer_period[timer])*1000;
            mfp_timer_period_current_fraction[timer]=0;
#if defined(SSE_INT_MFP_TIMER_CHECK)
            MC68901.Period[timer]=precise_cycles;
#endif
          }

#if defined(SSE_DEBUG)
#if defined(SSE_INT_MFP_TIMERS_STARTING_DELAY)
          TRACE_LOG("%d PC %X set timer %c control %d data %d counter %d prescale %d",
            ACT-MFP_TIMER_SET_DELAY,old_pc,'A'+timer,new_control,mfp_reg[MFPR_TADR+timer],
            mfp_timer_counter[timer]/64,prescale_count);
#else
          TRACE_LOG("%d PC %X set timer %c control $%x, data $%x",
            ACT-12,old_pc,'A'+timer,new_control,mfp_reg[MFPR_TADR+timer]);
#endif
          if(reg==MFPR_TBCR && new_val==8)
            TRACE_LOG(" (%d)\n",mfp_reg[MFPR_TADR+timer]);
          else
          {
            TRACE_LOG(" prescaler %d MFP cycles %d CPU cycles %d.%d next timeout %d\n",
            mfp_timer_prescale[new_control],
            mfp_timer_prescale[new_control]*int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer])),
            mfp_timer_period[timer],mfp_timer_period_fraction[timer],mfp_timer_timeout[timer]);
          }
#endif

#else
          // To make this more accurate for short timers, we should store the fractional
          // part as well.  Then every time it times out, increase the fractional part and
          // see if it goes over one.  If it does, make the next time-out a bit later.
          mfp_timer_period[timer]=int( double(mfp_timer_prescale[new_control]
          *int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer]))) 
            * CPU_CYCLES_PER_MFP_CLK);
#endif

#if defined(SSE_INT_MFP_PRESCALE)
/*
Changing the prescale value with the timer running
can cause the first Time Out pulse to occur at an indeterminate
time, (no less than one nor more than
200 timer clock cycles times the number in the time
constant register), but subsequent Time Out pulses
will then occur at the correct interval.
->
It seems the MFP will decrement current prescale whatever the new prescaler
but the doc doesn't explicitly say it either.
The MFP probably doesn't compute a min like here.

We also remove the div, multiply, which had a net effect of substracting some
cycles from the first timeout. This is compensated by a shorter MFP_TIMER_SET_DELAY.
TODO
*/
          if(!OPTION_C2) {
#endif
          // Here mfp_timer_timeout assumes that the next MFP_CLK tick happens
          // at exactly 3.24 cycles from when the timer is started, but that isn't
          // what really happens. Below we adjust for the fixed boundary of the clock.
          // We also handle prescale (changing between different divides when running)

          // This makes sure that we don't go back in time more than one count
          // It may be that the MFP checks prescale_count==mfp_timer_prescale
          // when it decides when to count, in that case we need a rethink
          // (the timer would fire much later).
          prescale_count=min(prescale_count,mfp_timer_prescale[new_control]);

          // Make manageable time (cpu_time_of_first_mfp_tick is updated every VBL)
//          TRACE_LOG("timer %d: %d - cpu_time_of_first_mfp_tick %d = %d\n",timer,mfp_timer_timeout[timer],cpu_time_of_first_mfp_tick,mfp_timer_timeout[timer]-cpu_time_of_first_mfp_tick);
          mfp_timer_timeout[timer]-=cpu_time_of_first_mfp_tick;
         
          // Convert to MFP cycles
          mfp_timer_timeout[timer]*=MFP_CLK; //SS =2451
          mfp_timer_timeout[timer]/=8000;

          // Take off number of cycles already counted
          mfp_timer_timeout[timer]-=prescale_count;

          // Convert back to CPU time
          mfp_timer_timeout[timer]*=8000;
          mfp_timer_timeout[timer]/=MFP_CLK;

          // Make absolute time again
          mfp_timer_timeout[timer]+=cpu_time_of_first_mfp_tick;

#if defined(SSE_INT_MFP_PRESCALE)
          } //if(!OPTION_C2)
#endif

#if defined(SSE_INT_MFP_TIMERS)
          if(OPTION_C2) 
          {
#if defined(SSE_INT_MFP_TIMERS_WOBBLE)
/*  This should be confirmed (or not...), we consider that delay timers
    have some wobble, like timer B, maybe because of CPU/MFP clock sync.
    We add some time to delay, which we'll correct at timeout for
    next timer.
    TEST10B doesn't confirm, but unconlusive
    MFPTA001 could indicate timer wobble
    (non-working image) Audio Sculpture suggests there's no such wobble.
    Suppose the real interaction between CPU and MFP clocks should be emulated.
*/
#if defined(SSE_INT_MFP_TIMERS_WOBBLE_390)
            MC68901.Wobble[timer]=(rand() % MFP_TIMERS_WOBBLE); //1-4
#else
            MC68901.Wobble[timer]=(rand()&MFP_TIMERS_WOBBLE);
#endif
            mfp_timer_timeout[timer]+=MC68901.Wobble[timer];
#endif
          }
#endif
          dbg_log(EasyStr("    Set control to ")+new_control+
                " (reg=$"+HEXSl(new_val,2)+"); data="+mfp_reg[MFPR_TADR+timer]+
                "; counter="+mfp_timer_counter[timer]/64+
                "; period="+mfp_timer_period[timer]+
                "; new timeout="+mfp_timer_timeout[timer]);
        }else{  //timer stopped, or in event count mode
          // This checks all timers to see if they have timed out, if they have then
          // it will set the pend bit. This is dangerous, messes up LXS!
          // mfp_check_for_timer_timeouts(); // ss not implemented

#if defined(SSE_INT_MFP_CHECKTIMEOUT_ON_STOP) 
/*  Idea, see just above, this version OK with LXS
*/
          if(OPTION_C2 && !new_val && old_val!=8 
            && mfp_timer_enabled[timer] && ACT-mfp_timer_timeout[timer]>=0)
          {
            BYTE i_ab=mfp_interrupt_i_ab(mfp_timer_irq[timer]);
            BYTE i_bit=mfp_interrupt_i_bit(mfp_timer_irq[timer]);
            if(!(mfp_reg[MFPR_ISRA+i_ab]&i_bit) &&
              (mfp_reg[MFPR_IERA+i_ab]&mfp_reg[MFPR_IMRA+i_ab]&i_bit))
            {
              TRACE_MFP("%d timer %c pending on stop\n",ACT,'A'+timer);
              mfp_interrupt_pend(mfp_timer_irq[timer],mfp_timer_timeout[timer]);
            }
          }
#endif

#ifdef SSE_DEBUG
          if(new_val & BIT_3)
            TRACE_LOG("%d PC %X set Timer %c\n",ACT,old_pc,'A'+timer);
          else
            TRACE_LOG("%d PC %X stop Timer %c counter %d prescale %d\n",
            ACT,old_pc,'A'+timer,mfp_timer_counter[timer]/64,prescale_count);
#endif

          mfp_timer_enabled[timer]=false;
          mfp_timer_period_change[timer]=0;
          dbg_log(EasyStr("  Set control to ")+new_control+" (reg=$"+HEXSl(new_val,2)+")"+
                "; counter="+mfp_timer_counter[timer]/64+" ;"+
                LPSTR((timer<2 && (new_val & BIT_3)) ? "event count mode.":"stopped.") );
        }
        if (timer==3) RS232_CalculateBaud(bool(mfp_reg[MFPR_UCR] & BIT_7),new_control,0);
      }
      timer++;
      new_control=BYTE(new_val & 7); // Timer D control
    }while (timer==3);

#if defined(SSE_INT_MFP_TIMERS_STARTING_DELAY)
    INSTRUCTION_TIME(OPTION_C2?-MFP_TIMER_SET_DELAY:-12); 
#else
    INSTRUCTION_TIME(-12);
#endif

    if (reg==MFPR_TBCR && new_val==8) 
      calc_time_of_next_timer_b();
#ifdef ENABLE_LOGGING
    if (reg<=MFPR_TBCR && new_val>8){
      dbg_log("MFP: --------------- PULSE EXTENSION MODE!! -----------------");
    }
#endif
#ifdef SSE_DEBUG
    ASSERT(reg>=MFPR_TACR);
    if(reg<=MFPR_TBCR && new_val>8) //Froggies OVR
#if defined(SSE_INT_MFP_TIMER_B_PULSE)
/*  TODO
*/    
      if(OPTION_C2 && reg==MFPR_TBCR)
      {
        TRACE_LOG("MFP Pulse mode %X\n",new_val);
      }
      else        
#endif   
      TRACE_LOG("MFP: --------------- PULSE EXTENSION MODE!! -----------------\n");
#endif
    prepare_event_again();
  }
  else if (reg>=MFPR_TADR && reg<=MFPR_TDDR){ //data reg change
    timer=reg-MFPR_TADR;
    dbg_log(Str("MFP: ")+HEXSl(old_pc,6)+" - Changing timer "+char(('A')+timer)+" data reg to "+new_val+" ($"+HEXSl(new_val,2)+") "+
          " at time="+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)+
          " cycles into scanline #"+scan_y+"); timeout is "+mfp_timer_timeout[timer]);

    new_control=BYTE(mfp_get_timer_control_register(timer));

    if (new_control==0){  // timer stopped
      mfp_timer_counter[timer]=((int)BYTE_00_TO_256(new_val))*64;
#if defined(SSE_INT_MFP_PRESCALE)
      MC68901.Counter[timer]=new_val;
      MC68901.Prescale[timer]=0;
#endif
      mfp_timer_period[timer]=int(double(mfp_timer_prescale[new_control]*int(BYTE_00_TO_256(new_val)))*CPU_CYCLES_PER_MFP_CLK);
    }else if (new_control & 7){
/*
If the Timer Data Register is written while the timer is running, the new word
is not loaded into the timer until it counts through H01.      
*/      
      // Need to calculate the period next time the timer times out
      mfp_timer_period_change[timer]=true;
      if (mfp_timer_enabled[timer]==0){
        // If it is disabled it could be in the past, causing instant
        // event_timer_?_timeout, so realign it
        int stage=mfp_timer_timeout[timer]-ABSOLUTE_CPU_TIME;
        if (stage<0) stage+=((-stage/mfp_timer_period[timer])+1)*mfp_timer_period[timer];
        stage%=mfp_timer_period[timer];
        mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME+stage; //realign
      }
    }
    dbg_log(EasyStr("     Period is ")+mfp_timer_period[timer]+" cpu cycles");
    if (reg==MFPR_TDDR && new_val!=old_val){
      RS232_CalculateBaud(bool(mfp_reg[MFPR_UCR] & BIT_7),new_control,0);
    }
  }
}

void mfp_init_timers() // For load state and CPU speed change
{
  MFP_CALC_INTERRUPTS_ENABLED;
  MFP_CALC_TIMERS_ENABLED;
  for (int timer=0;timer<4;timer++){
    BYTE cr=mfp_get_timer_control_register(timer);
    if (cr & 7){ // Not stopped or in event count mode
      // This must allow for counter not being a multiple of 64
      mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME+int((double(mfp_timer_prescale[cr])*
                      double(mfp_timer_counter[timer])/64.0)*CPU_CYCLES_PER_MFP_CLK);
      mfp_timer_period_change[timer]=true;
    }
  }
  RS232_CalculateBaud(bool(mfp_reg[MFPR_UCR] & BIT_7),mfp_get_timer_control_register(3),true);
}

//SS This is called whether a timer is being started or stopped
// reference is timeout
// this is called by ior too

int mfp_calc_timer_counter(int timer)
{
  BYTE cr=mfp_get_timer_control_register(timer); //SS register before write
  if (cr & 7){ // SS delay timer
#if defined(SSE_TIMINGS_CPUTIMER64)
    COUNTER_VAR stage=mfp_timer_timeout[timer]-ABSOLUTE_CPU_TIME;
#else
    int stage=mfp_timer_timeout[timer]-ABSOLUTE_CPU_TIME;
#endif
    if (stage<0){ //SS has timed out? - no high precision here...
      MFP_CALC_TIMER_PERIOD(timer);
#ifdef SSE_INT_MFP_394
      if(OPTION_C2) 
      {
        // If the counter wrapped not long ago, its visible value
        // is 0, not TXDR - fixes Froggies Over The Fence back to menu after disk 2
        if(stage>=-4)
        {
          mfp_timer_counter[timer]=0;
          MC68901.Counter[timer]=0;
          MC68901.Prescale[timer]=0;
          return MC68901.Prescale[timer];
        }
        // guess it's the intention, don't like that +1 (probably against /0)
        while(stage<0 && mfp_timer_period[timer]>0)
          stage+=mfp_timer_period[timer];
      }
      else
#endif
      stage+=((-stage/mfp_timer_period[timer])+1)*mfp_timer_period[timer];
    }
    stage%=mfp_timer_period[timer];
    // so stage is a number from 0 to mfp_timer_period-1
    ASSERT(stage>=0 && stage<mfp_timer_period[timer]);
    int ticks_per_count=mfp_timer_prescale[cr & 7];
    // Convert to number of MFP cycles until timeout
#if defined(SSE_TIMINGS_CPUTIMER64)
    stage=(COUNTER_VAR)(double(stage)/CPU_CYCLES_PER_MFP_CLK);
#else
    stage=int(double(stage)/CPU_CYCLES_PER_MFP_CLK);
#endif
    
    mfp_timer_counter[timer]=(stage/ticks_per_count)*64 + 64;
#if defined(SSE_INT_MFP_PRESCALE)
    MC68901.Counter[timer]=mfp_timer_counter[timer]/64;
    MC68901.Prescale[timer]=ticks_per_count-((stage % ticks_per_count)+1);
#else
    // return the number of prescale counts done so far
    //TRACE_LOG("Timer %c main %d prescale %d\n",'A'+timer,mfp_timer_counter[timer],prescale_count);
    return ticks_per_count-((stage % ticks_per_count)+1);
#endif
  }
#if defined(SSE_INT_MFP_PRESCALE)
  //TRACE_LOG("calc timer %c cr %d counter %d prescale %d\n",'A'+timer,cr,MC68901.Counter[timer],MC68901.Prescale[timer]);
  return MC68901.Prescale[timer];
#else
  return 0;
#endif
}

#ifndef SSE_BUILD // moved to SSEInterrupt
void ASMCALL check_for_interrupts_pending()
{
  if (STOP_INTS_BECAUSE_INTERCEPT_OS==0){
    if ((ioaccess & IOACCESS_FLAG_DELAY_MFP)==0){
      for (int irq=15;irq>=0;irq--){
        BYTE i_bit=BYTE(1 << (irq & 7));
        int i_ab=1-((irq & 8) >> 3);
        if (mfp_reg[MFPR_ISRA+i_ab] & i_bit){ //interrupt in service
          break;  //time to stop looking for pending interrupts
        }
        if (mfp_reg[MFPR_IPRA+i_ab] & i_bit){ //is this interrupt pending?
          if (mfp_reg[MFPR_IMRA+i_ab] & i_bit){ //is it not masked out?
            mfp_interrupt(irq,ABSOLUTE_CPU_TIME); //then cause interrupt
            break;        //lower priority interrupts not allowed now.
          }
        }
      }
    }
    if (vbl_pending){
      if ((sr & SR_IPL)<SR_IPL_4){
        VBL_INTERRUPT
      }
    }
    if (hbl_pending){
      if ((sr & SR_IPL)<SR_IPL_2){
        // Make sure this HBL can't occur when another HBL has already happened
        // but the event hasn't fired yet.
        if (int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)<scanline_time_in_cpu_cycles_at_start_of_vbl){
          HBL_INTERRUPT;
        }
      }
    }
  }
  prepare_event_again();
}
#endif

//---------------------------------------------------------------------------
#if defined(SSE_INT_MFP)

void mfp_interrupt(int irq) {
  // we redo some tests for the RS232 part that comes directly here, and also
  // when option C2 isn't checked!
  if(!(mfp_interrupt_enabled[irq]) || (sr & SR_IPL) >= SR_IPL_6
    || (ioaccess & (IOACCESS_FLAG_DELAY_MFP | IOACCESS_INTERCEPT_OS | IOACCESS_INTERCEPT_OS2))
    || ((mfp_reg[MFPR_IMRA+mfp_interrupt_i_ab(irq)] & mfp_interrupt_i_bit(irq))==0)
    || ((mfp_reg[MFPR_ISRA+mfp_interrupt_i_ab(irq)] & (-mfp_interrupt_i_bit(irq))) || (mfp_interrupt_i_ab(irq) && mfp_reg[MFPR_ISRA]))
    )
  {
    TRACE_LOG("MFP irq denied\n"); // funny message
    return;
  }

  M68K_UNSTOP;

#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::EXCEPTION;
#endif

  mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]&=BYTE(~mfp_interrupt_i_bit(irq));
  /*
  There are two end-of-interrupt modes: the automatic end-of-interrupt mode and
  the software end-of-interrupt mode. The mode is selected by writing a one or
  a zero to the S bit of the Vector Register (VR). If the S bit of the VR is a
  one, all channels operate in the software end-of-interrupt mode. If the S bit
  is a zero, all channels operate in the automatic end-of-interrupt mode, and a
  reset is held on all in-service bits. In the automatic end-of-interrupt mode,
  the pending bit is cleared when that channel passes its vector. At that
  point, no further history of that interrupt remains in the MK68901 MFP. In
  the software end-of-interrupt mode, the in-service bit is set and the pending
  bit is cleared when the channel passes its vector. With the in-service bit
  set, no lower priority channel is allowed to request an interrupt or to pass
  its vector during an acknowledge sequence, however, a lower priority channel
  may still receive and interrupt and latch it into the pending bit. A higher
  priority channel may still request an interrupt and be acknowledged.
  The in-service bit of a particular
  channel may be cleared by writing a zero to the corresponding bit in ISRA or
  ISRB. Typically, this will be done at the conclusion of the interrupt routine
  just before the return. Thus no lower priority channel will be allowed to
  request service until the higer priority channel is complete, while channels
  of still higher priority will be allowed to request service. While the
  in-service bit is set, a second interrupt on that channel may be received and
  latched into the pending bit, though no service request will be made in
  response to the second interrupt until the in-service bit is cleared. ISRA
  and ISRB may be read at any time. Only a zero may be written into any bit of
  ISRA and ISRB; thus the in-service may be cleared in software but cannot be
  set in software. This allows any one bit to be cleared, without altering any
  other bits, simply by writing all ones except for the bit position to be
  cleared to ISRA or ISRB, as with IPRA and IPRB.
  */
  if (MFP_S_BIT){ // SS software mode, set when vector is passed, 
    // cleared by program
    mfp_reg[MFPR_ISRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq);
  }else{ // SS automatic
    mfp_reg[MFPR_ISRA+mfp_interrupt_i_ab(irq)]&=BYTE(~mfp_interrupt_i_bit(irq));
  }
#if defined(SSE_DISK_CAPS) 
  // should be rare, most programs, including TOS, poll
  if(irq==7 && Caps.Active && (Caps.WD1772.lineout&CAPSFDC_LO_INTRQ))
  {
    TRACE_FDC("execute WD1772 irq\n");
    Caps.WD1772.lineout&=~CAPSFDC_LO_INTRQ; 
  }
#endif

  MEM_ADDRESS vector;
  vector=    (mfp_reg[MFPR_VR] & 0xf0)  +(irq);
  vector*=4;
  //SS timing is recorded before counting any IACK/fetching cycle
  mfp_time_of_start_of_last_interrupt[irq]=ABSOLUTE_CPU_TIME; 

#if defined(SSE_DEBUG)
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
    FrameEvents.Add(scan_y,LINECYCLES,'I',0x60+irq);
#endif
  TRACE_INT("%d %d %d %d PC %X IRQ %d VEC %X ",ACT,TIMING_INFO,old_pc,irq,LPEEK(vector));
  switch(irq)
  {
  case 0:TRACE_INT("Centronics busy\n");          break;
  case 1:TRACE_INT("RS-232 DCD\n");               break;                                         
  case 2:TRACE_INT("RS-232 CTS\n");               break;                                   
  case 3:TRACE_INT("Blitter done\n");             break;                             
  case 4:TRACE_INT("Timer D\n");                  break;                       
  case 5:TRACE_INT("Timer C\n");                  break;                            
  case 6:TRACE_INT("ACIA\n");                     break;                              
  case 7:TRACE_INT("FDC/HDC\n");                  break;                                           
  case 8:TRACE_INT("Timer B\n");                  break;                                     
  case 9:TRACE_INT("Send Error\n");               break;                               
  case 10:TRACE_INT("Send buffer empty\n");       break;                         
  case 11:TRACE_INT("Receive error\n");           break;                   
  case 12:TRACE_INT("Receive buffer full\n");     break;                         
  case 13:TRACE_INT("Timer A\n");                 break;                   
  case 14:TRACE_INT("RS-232 Ring detect\n");      break;             
  case 15:TRACE_INT("Monochrome Detect\n");       break;                     
  }//sw

#if defined(SSE_BOILER_FRAME_INTERRUPTS)
  Debug.FrameInterrupts|=4;
  Debug.FrameMfpIrqs|= 1<<irq;
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("MFP",irq);
#endif
#if defined(SSE_BOILER_IRQ_IN_HISTORY) 
  pc_history_y[pc_history_idx]=scan_y;
  pc_history_c[pc_history_idx]=LINECYCLES;
  pc_history[pc_history_idx++]=0x99000001+(6<<16)+(irq<<8); 
  if (pc_history_idx>=HISTORY_SIZE) 
    pc_history_idx=0;
#endif

#endif//dbg

#if defined(SSE_INT_MFP_IRQ_TIMING)
  if(OPTION_C2)
  {
    MC68901.LastIrq=irq;//unused
    MC68901.UpdateNextIrq();
  }
  int iack_cycles=ACT-MC68901.IackTiming;
#endif
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_INT_MFP_IRQ_TIMING) 
  INSTRUCTION_TIME(-iack_cycles);//temp
#endif
  m68kInterruptTiming();
#else
  INSTRUCTION_TIME_ROUND(SSE_INT_MFP_TIMING-iack_cycles);
#endif


  m68k_interrupt(LPEEK(vector));
  sr=WORD((sr & (~SR_IPL)) | SR_IPL_6);

#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::NORMAL;
#endif

  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  IRQ fired - vector=")+HEXSl(LPEEK(vector),6));
  debug_check_break_on_irq(irq);

}

#else //Steem 3.2
void mfp_interrupt(int irq,int when_fired)
{
  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("INTERRUPT: MFP IRQ #")+irq+" ("+(char*)name_of_mfp_interrupt[irq]+
                                        ") at PC="+HEXSl(pc,6)+" at time "+ABSOLUTE_CPU_TIME);
  if (mfp_interrupt_enabled[irq]){
    log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  enabled"));
    if (mfp_set_pending(irq,when_fired)==0){
      log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  but ignored due to MFP clearing pending after it was set"));
      TRACE_LOG("MFP irq ignored due to MFP clearing pending after it was set\n"); // cases?
    }else{
      if ((mfp_reg[MFPR_IMRA+mfp_interrupt_i_ab(irq)] & mfp_interrupt_i_bit(irq))==0){
        log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  but masked"));
      }else if ((mfp_reg[MFPR_ISRA+mfp_interrupt_i_ab(irq)] & (-mfp_interrupt_i_bit(irq))) || (mfp_interrupt_i_ab(irq) && mfp_reg[MFPR_ISRA])){
        log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  but outprioritized - ISR = ")+HEXSl(mfp_reg[MFPR_ISRA],2)+HEXSl(mfp_reg[MFPR_ISRB],2));
      }else{
        if ((sr & SR_IPL) < SR_IPL_6){
          if ((ioaccess & (IOACCESS_FLAG_DELAY_MFP | IOACCESS_INTERCEPT_OS | IOACCESS_INTERCEPT_OS2))==0){
            M68K_UNSTOP;
            mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]&=BYTE(~mfp_interrupt_i_bit(irq));
            if (MFP_S_BIT){
              mfp_reg[MFPR_ISRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq);
            }else{
              mfp_reg[MFPR_ISRA+mfp_interrupt_i_ab(irq)]&=BYTE(~mfp_interrupt_i_bit(irq));
            }
            MEM_ADDRESS vector;
            vector=    (mfp_reg[MFPR_VR] & 0xf0)  +(irq);
            vector*=4;
            mfp_time_of_start_of_last_interrupt[irq]=ABSOLUTE_CPU_TIME; 
            INSTRUCTION_TIME_ROUND(56);
            m68k_interrupt(LPEEK(vector));
            sr=WORD((sr & (~SR_IPL)) | SR_IPL_6);
            log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  IRQ fired - vector=")+HEXSl(LPEEK(vector),6));
            debug_check_break_on_irq(irq);
          }else{
            log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  MFP is too busy to request interrupt"));
          }
        }else{
          log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  masked by CPU (IPL too high)"));
        }
      }
    }
  }
  LOG_ONLY( else{log_to_section(LOGSECTION_INTERRUPTS,"  disabled");}  )
}
#endif

#if defined(SSE_INT_MFP)

TMC68901::TMC68901() {
  Init();
}

void TMC68901::Init() {
  ZeroMemory(this,sizeof(TMC68901));
  // init IrqInfo structure
  IrqInfo[0].IsGpip=IrqInfo[1].IsGpip=IrqInfo[2].IsGpip=IrqInfo[3].IsGpip
    =IrqInfo[6].IsGpip=IrqInfo[7].IsGpip=IrqInfo[14].IsGpip=IrqInfo[15].IsGpip
    =true;
  IrqInfo[4].IsTimer=IrqInfo[5].IsTimer=IrqInfo[8].IsTimer=IrqInfo[13].IsTimer
    =true;
  IrqInfo[4].Timer=3;  // timer D
  IrqInfo[5].Timer=2;  // timer C
  IrqInfo[8].Timer=1;  // timer B
  IrqInfo[13].Timer=0;  // timer A
}


int TMC68901::UpdateNextIrq(COUNTER_VAR at_time) {
/*  This logic is instant (?) on the MFP.
    The IRQ line depends only on MFP registers, not on IPL in the CPU.
    It may be asserted then later negated without the CPU handling it.
    In v3.7, it was useful for some cases.
    In v3.8 it has become vital for precise MFP emu.
*/
  ASSERT(OPTION_C2);
#if defined(SSE_CPU_IPL_DELAY)
  bool old_irq=Irq;
#endif
  NextIrq=-1; //default: none in sight
  Irq=false;
  if(at_time==-1)//default
    at_time=ACT;
  if(MFP_IRQ) // global test before we check each irq
  {
    for (int irq=15;irq>=0;irq--) { //need to check all
      BYTE i_ab=mfp_interrupt_i_ab(irq);
      BYTE i_bit=mfp_interrupt_i_bit(irq);
      if((i_bit // instant test thanks to event system
        & mfp_reg[MFPR_IERA+i_ab]&mfp_reg[MFPR_IPRA+i_ab]&mfp_reg[MFPR_IMRA+i_ab])
        && !(i_bit&mfp_reg[MFPR_ISRA+i_ab]))
      {
        Irq=true; // line is asserted by the MFP
        NextIrq=irq;
        IrqSetTime=at_time;
        TRACE_MFP("%d MFP IRQ (%d)\n",ACT,NextIrq);
        break;
      }
      else if((i_bit&mfp_reg[MFPR_ISRA+i_ab]))
        break; // no IRQ possible then (was bug)
    }//nxt irq
  }
#if defined(SSE_CPU_IPL_DELAY)
  if(Irq && !old_irq)
    M68000.LastIplChange=at_time;
#endif
  return NextIrq;
}

#if defined(SSE_INT_MFP_TIMER_B_392)
/*  Unique function (option C2) that sets up the timing of next
    "timer B event", as far as the MFP input is concerned.
    It should be correct also when timing of DE is changed by
    a "Shifter trick".
*/

void TMC68901::ComputeNextTimerB(int info) {
  ASSERT(OPTION_C2);
  COUNTER_VAR tontb=0;


#if defined(SSE_STF_HW_OVERSCAN)
  // mfp receives the normal DE signal, not the tricked overscan DE
  // in this emulation, it's a bit off compared with HW
  if((ST_TYPE==STF_LACESCAN||ST_TYPE==STF_AUTOSWITCH) && SSEConfig.OverscanOn 
    &&( (COLOUR_MONITOR && (scan_y<0 || scan_y>=200))
    || (!COLOUR_MONITOR && (scan_y<0 || scan_y>=400))))
    ; // skip - notice TOS expects groups of lines with no timer B tick at reset
  else
#endif
#ifdef SSE_GLUE_HIRES_394
/*  Audio Sculpture in hires switches to shift mode 0 and uses the timer B 
    so it needs to be more precise.
    FetchingLine() is based on scan_y and actual screen (mono or colour)
*/
  if(Glue.de_v_on && !(Glue.CurrentScanline.Tricks&TRICK_0BYTE_LINE)
#else
  if(Glue.FetchingLine() && !(Glue.CurrentScanline.Tricks&TRICK_0BYTE_LINE)
#endif    
    )
  {
    // time of DE transition this scanline
    cpu_cycles_from_hbl_to_timer_b = 
#if defined(SSE_INT_MFP_TIMER_B_AER)
      // from Hatari, fixes Seven Gates of Jambala; Trex Warrior
      (mfp_reg[MFPR_AER]&8) ? Glue.CurrentScanline.StartCycle :
#endif
      Glue.CurrentScanline.EndCycle;

    // Don't trigger a spurious Timer B tick. The exact timing could be later.
    // There's a line with 2x TB ticks in 'Jambala', but it has no visible impact.
    if(info==ChangingAer && LINECYCLES>cpu_cycles_from_hbl_to_timer_b)
    {
      TRACE_LOG("MFP AER change at %d, no timer B at %d\n",LINECYCLES,cpu_cycles_from_hbl_to_timer_b);
    }
    else
    {
      // add MFP delays (read + irq) (TIMERB07.TOS)
#if defined(SSE_CPU_IPL_DELAY)
      cpu_cycles_from_hbl_to_timer_b+=28-CPU_IPL_CHECK_TIME; 
#else
      cpu_cycles_from_hbl_to_timer_b+=28;
#endif
      // absolute
      tontb=cpu_timer_at_start_of_hbl+cpu_cycles_from_hbl_to_timer_b;
      // add jitter
      tontb+=TB_TIME_WOBBLE; 
    }
  }
  // In 'Jambala', the scanline event is much delayed on MOVEM.
  // There are compromises in Steem due to the fact that events are checked
  // between instructions. A bit of a hack. TODO
  if(!tontb || info!=NewScanline && tontb-ACT<=0 
    && time_of_next_event!=time_of_next_timer_b)
  {
    tontb=cpu_timer_at_start_of_hbl+160000;  //put into future
  }
  bool recheck_events=(time_of_next_timer_b!=tontb);
  time_of_next_timer_b=tontb;
  if(recheck_events)
    prepare_next_event();
}

#endif


void TMC68901::Reset() {
  Irq=false;
  IrqSetTime=ACT;

  for(int t=0;t<4;t++)
  {
    Counter[t]=0;
    Prescale[t]=0;
#if defined(SSE_INT_MFP_TIMER_CHECK)
    Period[t]=0.0;
#endif
  }
#ifdef SSE_DEBUG
  //test for corruption
  ASSERT(IrqInfo[8].Timer==1 && IrqInfo[7].IsGpip && IrqInfo[13].IsTimer);
#endif
}

#endif

//---------------------------------------------------------------------------
#undef LOGSECTION


#if !defined(SSE_VAR_DEAD_CODE)
// Use this for single-bit transitions in the GPIP
void mfp_gpip_transition(int bitnum,bool is_0_1_transition)
{
  // Zero for falling edge, 1 for rising edge
  bool edge=mfp_reg[MFPR_AER] & (1<<bitnum); 
  if(!(edge^is_0_1_transition)){
    int irq=mfp_gpip_irq[bitnum]; // Store for faster macro
    mfp_interrupt(irq,ABSOLUTE_CPU_TIME);
  }
}
#endif
