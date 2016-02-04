/*---------------------------------------------------------------------------
FILE: mfp.cpp
MODULE: emu
DESCRIPTION: The core of Steem's Multi Function Processor emulation
(MFP 68901). This chip handles most of the interrupt and timing functions in
the ST.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: mfp.cpp")
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_MFP_H)

#define EXT
#define INIT(s) =s

EXT BYTE mfp_reg[24]; // 24 directly addressable internal registers, each 8bit
EXT BYTE mfp_gpip_no_interrupt INIT(0xf7);
BYTE mfp_gpip_input_buffer=0;
#if defined(SSE_VAR_RESIZE_370)
const WORD mfp_timer_8mhz_prescale[16]={65535,4,10,16,50,64,100,200,
                                        65535,4,10,16,50,64,100,200};
const BYTE mfp_timer_irq[4]={13,8,5,4};
const BYTE mfp_gpip_irq[8]={0,1,2,3,6,7,14,15};
#else
const int mfp_timer_8mhz_prescale[16]={65535,4,10,16,50,64,100,200,65535,4,10,16,50,64,100,200};
const int mfp_timer_irq[4]={13,8,5,4};
const int mfp_gpip_irq[8]={0,1,2,3,6,7,14,15};
#endif
int mfp_timer_prescale[16]={65535,4,10,16,50,64,100,200,
                            65535,4,10,16,50,64,100,200};
int mfp_timer_counter[4];
int mfp_timer_timeout[4];
bool mfp_timer_enabled[4]={0,0,0,0};
int mfp_timer_period[4]={10000,10000,10000,10000};
#if defined(SSE_INT_MFP_RATIO_PRECISION)
int mfp_timer_period_fraction[4]; //={0,0,0,0}; //no need to init? //v3.7 not init
int mfp_timer_period_current_fraction[4]; //={0,0,0,0}; 
#endif
bool mfp_timer_period_change[4]={0,0,0,0};
//int mfp_timer_prescale_counter[4]={0,0,0,0};
//int mfp_gpip_timeout;
bool mfp_interrupt_enabled[16];
int mfp_time_of_start_of_last_interrupt[16];
#if defined(SSE_INT_MFP_IRQ_DELAY3)
int mfp_time_of_set_pending[16];
#endif
#if defined(SSE_INT_MFP_WRITE_DELAY1)//no
int time_of_last_write_to_mfp_reg=0;
#endif
#if !defined(SSE_INT_MFP_TIMERS_BASETIME)
int cpu_time_of_first_mfp_tick;
#endif
#undef EXT
#undef INIT

#endif

#include "SSE/SSEGlue.h"

#if defined(SSE_INT_MFP_OBJECT)
TMC68901 MC68901; // singleton, the infamous MFP
#endif

#ifdef SSE_DEBUG
#define LOGSECTION LOGSECTION_MFP//SS
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
#ifdef SSE_INT_MFP_UPDATE_IP_ON_GPIP_CHANGE //v3.8.0//mistake, MFD, but should be tested
  //if(!set) //oops :)
  if(OPTION_PRECISE_MFP && set && (mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(mfp_gpip_irq[bit])]&
      BYTE(mfp_interrupt_i_bit(mfp_gpip_irq[bit])) ))
    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(mfp_gpip_irq[bit])]&=
      BYTE(~mfp_interrupt_i_bit(mfp_gpip_irq[bit]));
#endif
  if (cur_val==set_mask) return; //no change
  //SS detects if we're setting an IRQ
  // (but not the reverse: Hackabonds)
  bool old_1_to_0_detector_input=(cur_val ^ (mfp_reg[MFPR_AER] & mask))==mask;
//  ASSERT( old_1_to_0_detector_input || set );
  mfp_reg[MFPR_GPIP]&=BYTE(~mask); //SS zero the target bit, leaving others
  mfp_reg[MFPR_GPIP]|=set_mask; //SS set target bit if needed
  // If the DDR bit is low then the bit from the io line is used,
  // if it is high interrupts then it comes from the input buffer.
  // In that case interrupts are handled in the write to the GPIP.
  if (old_1_to_0_detector_input && (mfp_reg[MFPR_DDR] & mask)==0){
    // Transition the right way! Make the interrupt pend (don't cause an intr
    // straight away in case another more important one has just happened).
    mfp_interrupt_pend(mfp_gpip_irq[bit],ABSOLUTE_CPU_TIME);
#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_IRQ_DELAY) //no
    if(SSE_HACKS_ON)
      ioaccess|=IOACCESS_FLAG_DELAY_MFP; 
    else
#endif
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
  }
}
//---------------------------------------------------------------------------
void calc_time_of_next_timer_b() //SS called only by mfp_set_timer_reg()
{
  int cycles_in=int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
  if (cycles_in<cpu_cycles_from_hbl_to_timer_b){
    if (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line){
#if defined(SSE_INT_MFP_TIMER_B_AER) //refactored
      if(mfp_reg[1]&8)
        time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
      else
#endif
        time_of_next_timer_b=cpu_timer_at_start_of_hbl+cpu_cycles_from_hbl_to_timer_b
#if defined(SSE_INT_MFP_TIMER_B_WOBBLE_HACK)
        /*  We need the hack when emulation is more precise??
        Possible explanation: writing now forces some sync with MFP?
        TODO
        */
        + (OPTION_PRECISE_MFP&&SSE_HACKS_ON?0:TB_TIME_WOBBLE);
#else
        +TB_TIME_WOBBLE;
#endif

#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS) && defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
  if(OPTION_PRECISE_MFP&&(Glue.CurrentScanline.Tricks&TRICK_LINE_MINUS_106)
    && LINECYCLES>166+28+4) // not if Timer B occurred in shift mode 2
    time_of_next_timer_b+=scanline_time_in_cpu_cycles[shifter_freq_idx]; 
#endif 

    }else{
      time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
    }
  }else{
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
  }
}
//---------------------------------------------------------------------------
#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_MFP_H)
// inline functions -> mfp.decla.h
#else
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

#if defined(SSE_INT_MFP_REFACTOR1) 

bool mfp_set_pending(int irq,int when_set) { //SS this function travels...

#if defined(SSE_INT_MFP_IRQ_TIMING) && !defined(SSE_INT_MFP_REFACTOR2) // moved at the end
  if(OPTION_PRECISE_MFP)
  {
#if defined(SSE_INT_MFP_GPIP_TO_IRQ_DELAY)     
    if(MC68901.IrqInfo[irq].IsGpip)
      when_set+=4; // documented delay between input transition and IRQ
#endif
#if defined(SSE_INT_MFP_RECORD_PENDING_TIMING)
    MC68901.PendingTiming[irq]=when_set; // record timing
#endif
    MC68901.UpdateNextIrq(irq,when_set);
    //ASSERT(MC68901.Irq); //argh! before we update register
  }
#endif

#if defined(SSE_INT_MFP_IACK_LATENCY2) || defined(SSE_INT_MFP_IACK_LATENCY4)
    //v3.7 - we check before
#if defined(SSE_INT_MFP_OPTION)
  if(OPTION_PRECISE_MFP
    || (abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])
    >=CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED))
#endif
#else
  if (abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])
    >=CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED)
#endif

  {
#if defined(SSE_DEBUG)
    bool was_already_pending=(mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]&mfp_interrupt_i_bit(irq));
#endif
    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq); // Set pending

#if defined(SSE_INT_MFP_IRQ_DELAY3)//no
    mfp_time_of_set_pending[irq]=when_set; // not always ACT
#endif

#if defined(SSE_INT_MFP_IRQ_TIMING) && defined(SSE_INT_MFP_REFACTOR2)
  if(OPTION_PRECISE_MFP)
  {
#if defined(SSE_INT_MFP_GPIP_TO_IRQ_DELAY)     
    if(MC68901.IrqInfo[irq].IsGpip)
      when_set+=4; // documented delay between input transition and IRQ
#endif
#if defined(SSE_INT_MFP_RECORD_PENDING_TIMING)
    MC68901.PendingTiming[irq]=when_set; // record timing
#endif
#if defined(SSE_DEBUG)
    if(was_already_pending)
      TRACE_MFP("%d MFP irq %d pending again\n",when_set,irq);
    else
      TRACE_MFP("%d MFP irq %d pending\n",when_set,irq);
#endif
#if defined(SSE_INT_MFP_REFACTOR2)
    MC68901.UpdateNextIrq(when_set);
#else
    MC68901.UpdateNextIrq(irq,when_set);
#endif
  }
#endif
    return true;
  }
#if defined(SSE_DEBUG) && ( !defined(SSE_INT_MFP_IACK_LATENCY2)  \
  || defined(SSE_INT_MFP_OPTION))
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
#if defined(SSE_INT_MFP_TIMERS_STARTING_DELAY) //no//yes
/*  Steem authors shift 1st timeout 12 cycles later, we use a parameter
    to test that. In current build 3.7.0, this is still the best
    value.
    This is explained by internal MFP delays, but it also covers a pre-IRQ
    delay (see MFP_TIMER_DATA_REGISTER_ADVANCE).
*/
    INSTRUCTION_TIME(OPTION_PRECISE_MFP?MFP_TIMER_SET_DELAY:12); // SSEParameters.h
#else
    INSTRUCTION_TIME(12); // The MFP doesn't do anything until 12 cycles after the write
#endif
    do{ //SS this do to do D
      if (mfp_get_timer_control_register(timer)!=new_control){
        new_control&=7;
        log( EasyStr("MFP: ")+HEXSl(old_pc,6)+" - Changing timer "+char('A'+timer)+" control; current time="+
              ABSOLUTE_CPU_TIME+"; old timeout="+mfp_timer_timeout[timer]+";"
              "\r\n           ("+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)+
              " cycles into scanline #"+scan_y+")" );

        // This ensures that mfp_timer_counter is set to the correct value just
        // in case the data register is read while timer is stopped or the timer
        // is restarted before a write to the data register.
        // prescale_count is the number of MFP_CLKs there has been since the
        // counter last decreased.
        int prescale_count=mfp_calc_timer_counter(timer);
        TRACE_LOG("Timer %c main %d prescale %d\n",'A'+timer,mfp_timer_counter[timer],prescale_count);

        if (new_control){ // Timer running in delay mode
                          // SS or pulse, but it's very unlikely

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_PATCH_TIMER_D) //no
          if(SSE_HACKS_ON 
            && timer==3 // = Timer D
            && old_pc > rom_addr // for Second Reality 2013 sound
            )
          {
//            TRACE("pc %x timer D %d -> %d",old_pc,mfp_reg[MFPR_TADR+timer],(mfp_reg[MFPR_TADR+timer] & 0xf0) | 0x7);
            mfp_reg[MFPR_TADR+timer]=(mfp_reg[MFPR_TADR+timer] & 0xf0) | 0x7;
          }
#endif

          mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME; //SS as modified

          mfp_timer_timeout[timer]+=int(double(mfp_timer_prescale[new_control]
            *mfp_timer_counter[timer]/64)*CPU_CYCLES_PER_MFP_CLK);
//          mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME+
//              (mfp_timer_prescale[new_control]*(mfp_timer_counter[timer])*125)/MFP_CLK;
              //*8000/MFP_CLK for MFP cycles, /64 for counter resolution
          mfp_timer_enabled[timer]=mfp_interrupt_enabled[mfp_timer_irq[timer]];


#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO_PRECISION_2)
/*  We keep the 'double' result instead of computing it twice like before.
*/
          double precise_cycles= mfp_timer_prescale[new_control]
                                  *(int)(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer]))
                                    *CPU_CYCLES_PER_MFP_CLK;
          mfp_timer_period[timer]=(int)precise_cycles;
#if defined(SSE_INT_MFP_OPTION)
          if(OPTION_PRECISE_MFP)
#endif
          {
            mfp_timer_period_fraction[timer]
              =(precise_cycles-mfp_timer_period[timer])*1000;
            mfp_timer_period_current_fraction[timer]=0;
          }
#else
          // To make this more accurate for short timers, we should store the fractional
          // part as well.  Then every time it times out, increase the fractional part and
          // see if it goes over one.  If it does, make the next time-out a bit later.
          mfp_timer_period[timer]=int( double(mfp_timer_prescale[new_control]
          *int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer]))) 
            * CPU_CYCLES_PER_MFP_CLK);
        //  TRACE("mfp_timer_period[%d]=%d\n",timer,mfp_timer_period[timer]);

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO_PRECISION)
/*  Here we do exactly what Steem authors suggested above, and it does bring the
    timing measurements at the same level as SainT and Hatari, at least in
    HWTST001.PRG by ljbk, so it's definitely an improvement, and it isn't 
    complicated at all!
*/
          mfp_timer_period_fraction[timer]=int( 1000*((double(mfp_timer_prescale[new_control]
          *int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer]))) * CPU_CYCLES_PER_MFP_CLK)-(double)mfp_timer_period[timer])  );
          mfp_timer_period_current_fraction[timer]=0;//added later?
#endif

#endif//#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO_PRECISION_2)

#if defined(SSE_DEBUG) && defined(SSE_INT_MFP_RATIO_PRECISION)
#if defined(SSE_INT_MFP_TIMERS_STARTING_DELAY)
          TRACE_LOG("%d PC %X set timer %c control $%x, data $%x",
            ACT-MFP_TIMER_SET_DELAY,old_pc,'A'+timer,new_control,mfp_reg[MFPR_TADR+timer]);
#else
          TRACE_LOG("%d PC %X set timer %c control $%x, data $%x",
            ACT-12,old_pc,'A'+timer,new_control,mfp_reg[MFPR_TADR+timer]);
#endif
          if(reg==MFPR_TBCR && new_val==8)
            TRACE_LOG(" (%d)\n",mfp_reg[MFPR_TADR+timer]);
          else
            TRACE_LOG(" count %d prescaler %d cycles %d.%d\n",prescale_count,mfp_timer_prescale[new_control],mfp_timer_period[timer],mfp_timer_period_fraction[timer]);
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
#if defined(SSE_INT_MFP_TIMERS_BASETIME)
/*  I don't know what I was thinking, but this should have no impact
    whether we take one or the other variable. 
    OTOH: could do without variable cpu_time_of_first_mfp_tick?
*/
          mfp_timer_timeout[timer]-=cpu_time_of_last_vbl;
#else
          mfp_timer_timeout[timer]-=cpu_time_of_first_mfp_tick;
#endif
         
          // Convert to MFP cycles
          mfp_timer_timeout[timer]*=MFP_CLK; //SS =2451
#if defined(SSE_INT_MFP_TIMERS_RATIO1)
          mfp_timer_timeout[timer]/=8021;
          //mfp_timer_timeout[timer]/=CpuMfpRatio;
#else
          mfp_timer_timeout[timer]/=8000;
#endif
          // Take off number of cycles already counted
          mfp_timer_timeout[timer]-=prescale_count;

          // Convert back to CPU time
#if defined(SSE_INT_MFP_TIMERS_RATIO1)
          mfp_timer_timeout[timer]*=8021;
#else
          mfp_timer_timeout[timer]*=8000;
#endif
          mfp_timer_timeout[timer]/=MFP_CLK;
          //mfp_timer_timeout[timer]*=CpuMfpRatio;

          // Make absolute time again
#if defined(SSE_INT_MFP_TIMERS_BASETIME)
          mfp_timer_timeout[timer]+=cpu_time_of_last_vbl;
#else
          mfp_timer_timeout[timer]+=cpu_time_of_first_mfp_tick;
#endif

#if defined(SSE_INT_MFP_OPTION)
          if(OPTION_PRECISE_MFP) 
#endif
          {

#if defined(SSE_INT_MFP_IACK_LATENCY4)//no
            //MC68901.SkipTimer[timer]=0;//?
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO_PRECISION3)
/*  As soon as there's a fractional part (almost all the time), that means
    that the CPU can't be interrupted before next CPU cycle after IRQ.
*/
            if(mfp_timer_period_fraction[timer])
              mfp_timer_timeout[timer]++; 
#endif

#if defined(SSE_INT_MFP_TIMERS_WOBBLE)
/*  This should be confirmed (or not...), we consider that delay timers
    have some wobble, like timer B, maybe because of CPU/MFP clock sync.
    We add some time to delay, which we'll correct at timeout for
    next timer.
    update: TEST10B doesn't confirm, but unconlusive, undef
*/
            MC68901.Wobble[timer]=(rand()&MFP_TIMERS_WOBBLE);
            mfp_timer_timeout[timer]+=MC68901.Wobble[timer];
#endif
          }
          log(EasyStr("    Set control to ")+new_control+
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

    TODO: eg Anomaly menu asserts on test Irq + extreme rage
*/

#if defined(SSE_INT_MFP_EVENT_WRITE)
          if(OPTION_PRECISE_MFP && !new_val 
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
#else
          if(OPTION_PRECISE_MFP && !new_val 
            && mfp_timer_enabled[timer] // hem... Platoon STX
            && ACT-mfp_timer_timeout[timer]>=0
            && ! MC68901.InService(mfp_timer_irq[timer]) //only use
            && MC68901.Enabled(mfp_timer_irq[timer]) 
            && MC68901.MaskOK(mfp_timer_irq[timer]) )
          {
            TRACE_MFP("MFP timer %c enabled %d pending on stop; timed out %d cycles ago last serviced %d before\n",
             'A'+timer, mfp_timer_enabled[timer],ACT-mfp_timer_timeout[timer],-(mfp_time_of_start_of_last_interrupt[mfp_timer_irq[timer]]-mfp_timer_timeout[timer]));
            mfp_interrupt_pend(mfp_timer_irq[timer],mfp_timer_timeout[timer]);
          }
#endif//event write?
#endif


#ifdef SSE_DEBUG
          if(new_val & BIT_3)
            TRACE_LOG("%d PC %X set Timer %C %d\n",ACT,old_pc,'A'+timer,mfp_timer_counter[timer]/64);
          else
            TRACE_LOG("%d PC %X stop Timer %C prescale %d\n",ACT,old_pc,'A'+timer,prescale_count);
#endif

          mfp_timer_enabled[timer]=false;
          mfp_timer_period_change[timer]=0;
          log(EasyStr("  Set control to ")+new_control+" (reg=$"+HEXSl(new_val,2)+")"+
                "; counter="+mfp_timer_counter[timer]/64+" ;"+
                LPSTR((timer<2 && (new_val & BIT_3)) ? "event count mode.":"stopped.") );
        }
        if (timer==3) RS232_CalculateBaud(bool(mfp_reg[MFPR_UCR] & BIT_7),new_control,0);
      }
      timer++;
      new_control=BYTE(new_val & 7); // Timer D control
    }while (timer==3);

#if defined(SSE_INT_MFP_TIMERS_STARTING_DELAY)
    INSTRUCTION_TIME(OPTION_PRECISE_MFP?-MFP_TIMER_SET_DELAY:-12); 
#else
    INSTRUCTION_TIME(-12);
#endif

    if (reg==MFPR_TBCR && new_val==8) 
      calc_time_of_next_timer_b();
#ifdef ENABLE_LOGGING
    if (reg<=MFPR_TBCR && new_val>8){
      log("MFP: --------------- PULSE EXTENSION MODE!! -----------------");
    }
#endif
#ifdef SSE_DEBUG
    ASSERT(reg>=MFPR_TACR);
    if(reg<=MFPR_TBCR && new_val>8) //Froggies OVR
#if defined(SSE_INT_MFP_TIMER_B_PULSE)
/*  TODO
*/    
      if(OPTION_PRECISE_MFP && reg==MFPR_TBCR)
      {
        TRACE_LOG("MFP Pulse mode %X\n",new_val);
      }
      else        
#endif   
      TRACE_LOG("MFP: --------------- PULSE EXTENSION MODE!! -----------------\n");
#endif
    prepare_event_again();
  }else if (reg>=MFPR_TADR && reg<=MFPR_TDDR){ //data reg change
    timer=reg-MFPR_TADR;
    log(Str("MFP: ")+HEXSl(old_pc,6)+" - Changing timer "+char(('A')+timer)+" data reg to "+new_val+" ($"+HEXSl(new_val,2)+") "+
          " at time="+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)+
          " cycles into scanline #"+scan_y+"); timeout is "+mfp_timer_timeout[timer]);
    new_control=BYTE(mfp_get_timer_control_register(timer));

    if (new_control==0){  // timer stopped
      mfp_timer_counter[timer]=((int)BYTE_00_TO_256(new_val))*64;
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
    log(EasyStr("     Period is ")+mfp_timer_period[timer]+" cpu cycles");
    if (reg==MFPR_TDDR && new_val!=old_val){
      RS232_CalculateBaud(bool(mfp_reg[MFPR_UCR] & BIT_7),new_control,0);
    }
  }
  //TRACE("mfp_timer_period_change[3] %d\n",mfp_timer_period_change[3]);
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

int mfp_calc_timer_counter(int timer)
{
  BYTE cr=mfp_get_timer_control_register(timer);
  if (cr & 7){ // SS delay timer
    int stage=mfp_timer_timeout[timer]-ABSOLUTE_CPU_TIME;
    if (stage<0){ //SS has timed out?
      MFP_CALC_TIMER_PERIOD(timer);
      stage+=((-stage/mfp_timer_period[timer])+1)*mfp_timer_period[timer];
    }
    stage%=mfp_timer_period[timer];
    // so stage is a number from 0 to mfp_timer_period-1
    ASSERT(stage>=0 && stage<mfp_timer_period[timer]);
    int ticks_per_count=mfp_timer_prescale[cr & 7];
    // Convert to number of MFP cycles until timeout
    stage=int(double(stage)/CPU_CYCLES_PER_MFP_CLK);
    mfp_timer_counter[timer]=(stage/ticks_per_count)*64 + 64;
    // return the number of prescale counts done so far
    //TRACE_LOG("Timer %c main %d prescale %d\n",'A'+timer,mfp_timer_counter[timer],prescale_count);
    return ticks_per_count-((stage % ticks_per_count)+1);
  }
  return 0;
}

/*
          ----- MC68000 Interrupt Autovector ---------------

                   --------------- --------------------------------
                  | Level         | Definition                     |
                   --------------- --------------------------------
                  | 7 (HIGHEST)   | NMI (Non Maskable Interrupt)   |
                  | 6             | MK68901 MFP                    |
                  | 5             |                                |
                  | 4             | Vertical Blanking (Sync)       |
                  | 3             |                                |
                  | 2             | Horizontal Blanking (Sync)     |
                  | 1 (LOWEST)    |                                |
                   --------------- --------------------------------

          NOTE:  only interrupt priority level inputs 1 and 2 are used.

          SS An instruction like move.w $2700 SR disables interrupts


          ----- MK68901 Interrupt Control ---------------

                   --------------- --------------------------------
                  | Priority      | Definition                     |
                   --------------- --------------------------------
                  | 15 (HIGHEST)  | Monochrome Monitor Detect    I7|
                  | 14            | RS232 Ring Indicator         I6|
                  | 13            | System Clock / BUSY          TA|
                  | 12            | RS232 Receive Buffer Full      |
                  | 11            | RS232 Receive Error            |
                  | 10            | RS232 Transmit Buffer Empty    |
                  |  9            | RS232 Transmit Error           |
                  |  8            | Horizontal Blanking Counter  TB|
                  |  7            | Disk Drive Controller        I5|
                  |  6            | Keyboard and MIDI            I4|
                  |  5            | Timer C                      TC|
                  |  4            | RS232 Baud Rate Generator    TD|
                  |  3            | GPU Operation Done           I3|
                  |  2            | RS232 Clear To Send          I2|
                  |  1            | RS232 Data Carrier Detect    I1|
                  |  0 (LOWEST)   | Centronics BUSY              I0|
                   --------------- --------------------------------

          NOTE:  the MC6850 ACIA Interrupt Request status bit must be tested
                 to differentiate between keyboard and MIDI interrupts.

*/


void ASMCALL check_for_interrupts_pending()
{
//SS  check_for_interrupts_pending() concerns not just the MFP,
// but also HBL and VBL interrupts

#if defined(SSE_INT_MFP_REFACTOR1)//v3.7.0
/*  For MFP interrupts, there are all sorts of tests before triggering the
    interrupt, the problem was that some of those tests were in mfp_interrupt().
    Here we place all the tests in check_for_interrupts_pending() and when
    mfp_interrupt() is called from here, the interrupt is executed for sure.
*/
  if(!(ioaccess & (IOACCESS_INTERCEPT_OS | IOACCESS_INTERCEPT_OS2))) //internal Steem flags
  {
    if (!(ioaccess & IOACCESS_FLAG_DELAY_MFP) //internal Steem flag
      && ((sr & SR_IPL)<SR_IPL_6)) //MFP can interrupt to begin with
    {
#if defined(SSE_INT_MFP_IACK_LATENCY2) || defined(SSE_INT_MFP_IACK_LATENCY3)
      BYTE iack_latency=MFP_IACK_LATENCY;
#endif
#if defined(SSE_INT_MFP_REFACTOR2) //v3.8.0
/*  Instead of looking up which timers will trigger during IACK, we just
    trigger events themselves, related or not to MFP.
    This is far simpler than the 3.7 way. I needed to further familiarise
    myself with Steem's inner working before I could do it.
    It also adds ACIA events to the test.
*/
      MC68901.IackTiming=ACT; // record start of IACK cycle (also if option C2 not checked)
      if(OPTION_PRECISE_MFP) // if not, "old" Steem test
      {
#if defined(SSE_INT_MFP_EVENT_WRITE_SPURIOUS)
        // make sure we don't start IACK if IRQ should be cleared by a write
        if(MC68901.WritePending&&ACT-MC68901.WriteTiming>=MFP_WRITE_LATENCY)
          event_mfp_write(); // this will update MC68901.Irq
#endif
#if defined(SSE_DEBUG) && defined(SSE_INT_MFP_REFACTOR2)
        ASSERT(MFP_IRQ==false||MFP_IRQ==true); //  is boolean
#if !defined(SSE_INT_MFP_REFACTOR2A)
        ASSERT(MFP_IRQ^(!MC68901.Irq)); // synchronised
#endif
#endif
#if defined(SSE_INT_MFP_REFACTOR2A)
        if(MC68901.Irq) 
#else
        if(MFP_IRQ)
#endif
        {
#if defined(SSE_INT_MFP_REFACTOR2A)
          ASSERT(MFP_IRQ);
#endif
          TRACE_MFP("%d Start IACK for irq %d\n",MC68901.IackTiming,MC68901.NextIrq);
          ASSERT(!M68000.IackCycle);
          M68000.IackCycle=true;
#if defined(SSE_INT_MFP_SPURIOUS) 
          bool no_real_irq=false; // if irq wasn't really set, there can be no spurious
#endif
#if defined(SSE_INT_MFP_WRITE_DELAY2) && !defined(SSE_INT_MFP_EVENT_WRITE_SPURIOUS)
/*  The MFP doesn't update its registers at once after a write.
    There's a delay of estimated 4 CPU cycles.
    This complicates emulation quite a deal.
    We don't need it if we use the MFP write event.
*/
          bool post_write=(MC68901.IackTiming-MC68901.WriteTiming>=0 
            && MC68901.IackTiming-MC68901.WriteTiming<=MFP_WRITE_LATENCY);
#endif

#ifndef SSE_DEBUG_MFP_DEACTIVATE_IACK_SUBSTITUTION
/*  Start the IACK cycle. We don't do the actual pushing yet.
    But we trigger any event during the cycle. This could change
    the irq, or clear two interrupts at once.
    Cases:
    Anomaly menu
    Extreme Rage guest screen
    Final Conflict
    Froggies/OVR
    Fuzion 77, 78, 84, 146...
    There are many other cases where it happens but emulation seems
    fine without the feature.
*/
          WORD oldsr=sr;
#if defined(SSE_DEBUG)
          char dbg_iack_subst=0;
#endif
          sr=WORD((sr & (~SR_IPL)) | SR_IPL_6); // already forbid other interrupts
          while(cpu_cycles&&cpu_cycles<=iack_latency) // can be negative: Froggies/OVR
          {
            iack_latency-=cpu_cycles;
            INSTRUCTION_TIME(cpu_cycles); // hop to event
            screen_event_vector();  // trigger event //big possible bug:twice??
#if defined(SSE_DEBUG) 
            {
              if(screen_event_vector==event_timer_a_timeout)
                dbg_iack_subst='A';
              else if(screen_event_vector==event_timer_b_timeout||screen_event_vector==event_timer_b)
                dbg_iack_subst='B';
              else if(screen_event_vector==event_timer_c_timeout)
                dbg_iack_subst='C';
              else if(screen_event_vector==event_timer_d_timeout)
                dbg_iack_subst='D';
              if(dbg_iack_subst)
                TRACE_MFP(">IACK run timer %c at %d\n",dbg_iack_subst,time_of_next_event);
#if defined(SSE_INT_MFP_EVENT_WRITE)
              else if(screen_event_vector==event_mfp_write)
                TRACE_MFP(">IACK %d wrote %X to MFP reg %d (before: %X)\n",ACT,MC68901.LastRegisterWrittenValue,MC68901.LastRegisterWritten,MC68901.LastRegisterFormerValue);
              else
                TRACE_MFP(">IACK executed vector %X\n",screen_event_vector);
#endif             
            }
#endif//dbg
#if defined(SSE_INT_MFP_WRITE_DELAY2) && defined(SSE_INT_MFP_REFACTOR2) \
  && !defined(SSE_INT_MFP_EVENT_WRITE_SPURIOUS)
            post_write=(ACT-MC68901.WriteTiming>=0 
              && ACT-MC68901.WriteTiming<=MFP_WRITE_LATENCY);
#endif
#if defined(SSE_INT_MFP_REFACTOR2) && defined(SSE_INT_MFP_EVENT_WRITE_SPURIOUS)
/*  As soon as an event clears IRQ during IACK, we go in panic mode. 
    This should trigger spurious interrupt.
    It could actually depend on when exactly during IACK but in TEST10.TOS,
    the timer would trigger again during IACK, asserting IRQ (or maybe there's
    another delay timeout-pend-IRQ, or another bug).
*/
            if(!MFP_IRQ)
            {
              TRACE_MFP("lost IRQ during IACK %d\n",iack_latency);
              break;
            }
#endif
            prepare_next_event();
          }//wend
          sr=oldsr;
#endif//#ifndef SSE_DEBUG_MFP_DEACTIVATE_IACK_SUBSTITUTION

          // browse 15 MFP irq, starting with highest priority
          char irq; //signed
          for (irq=15;irq>=0;irq--) {
            BYTE i_ab=mfp_interrupt_i_ab(irq);
            BYTE i_bit=mfp_interrupt_i_bit(irq);
            if (mfp_reg[MFPR_ISRA+i_ab] & i_bit){ //interrupt in service
              break;  //time to stop looking for pending interrupts
            }
#if defined(SSE_INT_MFP_WRITE_DELAY2) && defined(SSE_INT_MFP_IS_DELAY) && !defined(SSE_INT_MFP_EVENT_WRITE)
            if(post_write && MC68901.LastRegisterWritten==MFPR_ISRA+i_ab
              && !(MC68901.LastRegisterWrittenValue&i_bit) // was clearing
              && (MC68901.LastRegisterFormerValue&i_bit)) // was set
             {
              TRACE_MFP("%d MFP delay write ISR%c %X->%X irq %d %d\n",ACT,'A'+i_ab,MC68901.LastRegisterWrittenValue,MC68901.LastRegisterFormerValue,irq,MC68901.IackTiming-MC68901.WriteTiming);
#if defined(SSE_INT_MFP_REFACTOR2)
              ioaccess|=IOACCESS_FLAG_DELAY_MFP;
#endif
              break; // fixes Super Hang-On C1 + C2 (if event write MFP not defined)
            }
#endif
            BYTE relevant_pending_register=mfp_reg[MFPR_IPRA+i_ab];
#if defined(SSE_INT_MFP_WRITE_DELAY2) && !defined(SSE_INT_MFP_EVENT_WRITE)
            if(post_write && MC68901.LastRegisterWritten==MFPR_IPRA+i_ab)
            {
              TRACE_MFP("%d MFP delay write IPR%c %X->%X irq %d %d\n",ACT,'A'+i_ab,relevant_pending_register,MC68901.LastRegisterFormerValue,irq,MC68901.IackTiming-MC68901.WriteTiming);
              relevant_pending_register=MC68901.LastRegisterFormerValue;
#if defined(SSE_INT_MFP_REFACTOR2)
              ioaccess|=IOACCESS_FLAG_DELAY_MFP;
#else
              ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; // would be cleared
#endif
            }
#endif
            if (relevant_pending_register & i_bit){ //is this interrupt pending?
              BYTE relevant_mask_register=mfp_reg[MFPR_IMRA+i_ab]
#if !defined(SSE_INT_MFP_REFACTOR2)
              & i_bit
#endif
                ;
#if defined(SSE_INT_MFP_WRITE_DELAY2) && !defined(SSE_INT_MFP_EVENT_WRITE)
              // eg Audio Artistic Demo
              if(post_write && MC68901.LastRegisterWritten==MFPR_IMRA+i_ab)
              {
                TRACE_MFP("%d MFP delay write IMR%c %X->%X irq %d %d\n",ACT,'A'+i_ab,relevant_mask_register,MC68901.LastRegisterFormerValue,irq,MC68901.IackTiming-MC68901.WriteTiming);
                relevant_mask_register=MC68901.LastRegisterFormerValue;
#if defined(SSE_INT_MFP_REFACTOR2)
                ioaccess|=IOACCESS_FLAG_DELAY_MFP;
#else
                ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; // would be cleared
#endif
#if defined(SSE_INT_MFP_SPURIOUS)  
                if(!(relevant_mask_register&i_bit))
                  no_real_irq=true;
#endif
              }
#endif
              if(relevant_mask_register&i_bit) 
              {
                ASSERT(mfp_reg[MFPR_IERA+i_ab] & i_bit);
                ASSERT(!(mfp_reg[MFPR_ISRA+i_ab] & i_bit));
                {
#if defined(SSE_INT_MFP_IRQ_TIMING) && defined(SSE_INT_MFP_GPIP_TO_IRQ_DELAY)
/*  If the GPIP input has just transitioned, IRQ hasn't fired 
    yet. Fixes V8 Music System.
    Note: this is maintained also with SSE_INT_MFP_EVENT_WRITE contrary to 
    other tests above.
*/
                  if( MC68901.IrqInfo[irq].IsGpip 
                    && MC68901.IackTiming-MC68901.IrqSetTime<0 
                    && MC68901.IackTiming-MC68901.IrqSetTime>=-4)
                  {
                      ASSERT(irq<4||irq==6||irq==7||irq>13);
                      TRACE_MFP("MFP delay GPIP-irq %d %d\n",irq,MC68901.IackTiming-MC68901.IrqSetTime);
#if defined(SSE_INT_MFP_REFACTOR2)
                      ioaccess|=IOACCESS_FLAG_DELAY_MFP;
#else
                      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS; // would be cleared
#endif
#if defined(SSE_INT_MFP_SPURIOUS)  
                      no_real_irq=true;
#endif
                      continue; // examine lower irqs too
                  }
#endif    
#if defined(SSE_DEBUG) && defined(SSE_OSD_CONTROL) \
  && !defined(SSE_DEBUG_MFP_DEACTIVATE_IACK_SUBSTITUTION)
                  if(dbg_iack_subst)
                    TRACE_OSD("IACK %d %d -> %d",iack_latency,MC68901.NextIrq,irq);
#endif

                  mfp_interrupt(irq,ABSOLUTE_CPU_TIME); //then cause interrupt
                  break;        //lower priority interrupts not allowed now.
                }//enabled
              }//mask OK
            }//pending
          }//nxt irq
          //ASSERT(irq>-1);
#if defined(SSE_INT_MFP_SPURIOUS)          
/*  The dangerous spurious test.
    It triggers automatically at the end of IACK if we couldn't find an irq.
*/
          if(irq==-1 
#if !defined(SSE_INT_MFP_EVENT_WRITE_SPURIOUS)
            && post_write 
#endif
            && !no_real_irq) // couldn't find one and there was no break
          {
            TRACE_OSD("Spurious!");
            TRACE_MFP("%d PC %X Spurious!\n",ACT,old_pc);
            TRACE_MFP("IRQ %d (%d) IERA %X IPRA %X IMRA %X ISRA %X IERB %X IPRB %X IMRB %X ISRB %X\n",MC68901.Irq,MC68901.NextIrq,mfp_reg[MFPR_IERA],mfp_reg[MFPR_IPRA],mfp_reg[MFPR_IMRA],mfp_reg[MFPR_ISRA],mfp_reg[MFPR_IERB],mfp_reg[MFPR_IPRB],mfp_reg[MFPR_IMRB],mfp_reg[MFPR_ISRB]);
#ifdef SSE_BETA //enable for public beta
            BRK(Spurious interrupt); 
#endif
            int iack_cycles=ACT-MC68901.IackTiming;
#if defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
            INSTRUCTION_TIME_ROUND(50-12-iack_cycles); //? unimportant
#else
            INSTRUCTION_TIME(50-iack_cycles); //?
#endif
            m68k_interrupt(LPEEK(0x60)); // vector for Spurious, NOT Bus Error
            sr=WORD((sr & (~SR_IPL)) | SR_IPL_6); // the CPU does that anyway
          }
#endif//spurious-380
          M68000.IackCycle=false;
        }//MC68901.Irq
      }//precise
      // no C2 option = Steem 3.2
      else for (int irq=15;irq>=0;irq--)
      {
        ASSERT(!OPTION_PRECISE_MFP)
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
      }//nxt irq

#else//#if defined(SSE_INT_MFP_REFACTOR2) 
//we didn't debug this part since it's not defined

      // browse possible irq, starting with highest priority
      for (int irq=15;irq>=0;irq--) {
        BYTE i_ab=mfp_interrupt_i_ab(irq);
        BYTE i_bit=mfp_interrupt_i_bit(irq);
        if (mfp_reg[MFPR_ISRA+i_ab] & i_bit){ //interrupt in service
          break;  //time to stop looking for pending interrupts
        }

#if defined(SSE_INT_MFP_SPURIOUS)
        if(OPTION_PRECISE_MFP && MC68901.CheckSpurious(irq))
          break;
#endif

        if (mfp_reg[MFPR_IPRA+i_ab] & i_bit){ //is this interrupt pending?
#if defined(SSE_INT_MFP_WRITE_DELAY2)
/*  The MFP doesn't update its registers at once after a write.
    There's a delay of estimated 4 CPU cycles
    -Audio Artistic Demo (so fix in v3.6 was legit)
    -TEST10
*/
          BYTE relevant_mask_register=mfp_reg[MFPR_IMRA+i_ab] & i_bit;
          BYTE relevant_pending_register=mfp_reg[MFPR_IPRA+i_ab] & i_bit;
          if(OPTION_PRECISE_MFP && ACT-MC68901.WriteTiming<=MFP_WRITE_LATENCY
            && ACT-MC68901.WriteTiming>=0)
          {
            if(MC68901.LastRegisterWritten==MFPR_IMRA+i_ab)
            {
              TRACE_LOG("%d MFP delay write IMR%c irq %d %d\n",ACT,'A'+i_ab,irq,ACT-MC68901.WriteTiming);
              relevant_mask_register=MC68901.LastRegisterFormerValue;
              ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;//always? but it can't hurt
            }
            else if(MC68901.LastRegisterWritten==MFPR_IPRA+i_ab )
            {
              TRACE_LOG("%d MFP delay write IPR%c irq %d %d\n",ACT,'A'+i_ab,irq,ACT-MC68901.WriteTiming);
              relevant_pending_register=MC68901.LastRegisterFormerValue;
              ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
            }
          }
            
          if(relevant_mask_register&i_bit) {
#else
          if (mfp_reg[MFPR_IMRA+i_ab] & i_bit){ //is it not masked out?
#endif
#if defined(SSE_INT_MFP_WRITE_DELAY2)
            if(relevant_pending_register&i_bit) {
#else
            if (mfp_interrupt_enabled[irq]){ // is it enabled
#endif
              ASSERT(mfp_reg[MFPR_IERA+i_ab] & i_bit);
#if defined(SSE_INT_MFP_OPTION)
              if(OPTION_PRECISE_MFP)
#endif
              {
#if defined(SSE_INT_MFP_IACK_LATENCY2)
/*  Check it some same or higher priority delay timer is going to trigger 
    during IACK cycles.
    If yes, clear and execute this irq.
    Our way is a bit heavy now but operational, refactoring due post v3.7.0.
    Cases:
    Final Conflict, timer A (this worked with previous Steem hack), IACK
    As originally explained by ijor:
    http://www.atari-forum.com/viewtopic.php?f=51&t=15885#p195733 
    Froggies/OVR, timer A  (this worked with previous Steem trick), negative
    values must be accepted because instructions are long: MUL, DIV.
*/
                int tn;
                for(tn=0;tn<4;tn++) // browse timers
                {
                  if(mfp_timer_irq[tn]>=irq && mfp_timer_enabled[tn] 
                  && MC68901.MaskOK(mfp_timer_irq[tn])
                    && mfp_timer_timeout[tn]-ACT<=iack_latency)
                  {
#if defined(SSE_OSD_CONTROL)
                    if(OSD_MASK1 & OSD_CONTROL_IACK)
                      TRACE_OSD("IACK %d->%d %d",irq,mfp_timer_irq[tn],mfp_timer_timeout[tn]-ACT);
#endif
                    TRACE_LOG("%d IACK %d->%d %d\n",ACT,irq,mfp_timer_irq[tn],mfp_timer_timeout[tn]-ACT);                  
#if defined(SSE_INT_MFP_IACK_LATENCY4)
                    //if(mfp_timer_timeout[tn]-ACT>=0)
                    {
                      MC68901.NextIrq=irq=mfp_timer_irq[tn]; // execute now
                      MC68901.SkipTimer[tn]++; // skip next 'pending' (run.cpp)
                    }
#endif
                    break;
                  }
                }
#if !defined(SSE_INT_MFP_IACK_LATENCY4)
                if(tn<4)
                  break;//ignore irq: can work but timing trouble
#endif
#endif

#if defined(SSE_INT_MFP_IACK_LATENCY3)
/*  Check if timer B is going to trigger during IACK cycles.
    If yes, clear and execute this irq.
    Cases: 
    Anomaly menu irq 5 "IACKed" by 8. This fixes the flicker, but
    if timers are less precise (option '68901' off), there's no
    flicker anyway.
    That's no bug but 2 previous bugs compensating in Steem 3.2.
    In previous versions of Steem SSE only one side was done (fractional part
    of timers) and this screen was broken.
    Extreme Rage guest screen, 8 IACKing itself (I found a new word)
    This worked with previous Steem hack.
    Fuzion 77 (STF) 5->8
*/
                if(irq<=8 // <= not <
                  &&  MC68901.TimerBActive() // hem! not Audio Artistic then
                  &&  mfp_timer_counter[1]<64+64 // must trigger
                  && time_of_next_timer_b-ACT<=iack_latency
                  && time_of_next_timer_b-ACT>=0)
                {
#if defined(SSE_OSD_CONTROL)
                  if(OSD_MASK1 & OSD_CONTROL_IACK)
                    TRACE_OSD("IACK %d->TB %d y%d %d",irq,time_of_next_timer_b-ACT,scan_y,LINECYCLES);
#endif
                  TRACE_LOG("IACK %d->TB %d y%d %d\n",irq,time_of_next_timer_b-ACT,scan_y,LINECYCLES);                
#if !defined(SSE_INT_MFP_IACK_LATENCY5)
/*  We ignore this irq so timer B will trigger as usual.
    Normally it should trigger some cycles before, since IACK already
    started, and that's what happens when SSE_INT_MFP_IACK_LATENCY5 is
    defined.
    But Fuzion 176 works better if it's not defined. I think it's some
    internal Steem trouble. Should be checked on real STF. TODO
*/
                  break; //ignore irq
#endif
#if defined(SSE_INT_MFP_IACK_LATENCY4) && defined(SSE_INT_MFP_IACK_LATENCY5)//no
                  irq=MC68901.NextIrq=8; // execute now
                  MC68901.SkipTimer[1]++; // skip next 'pending'
#endif
                }
#endif
#if defined(SSE_INT_MFP_IRQ_TIMING) && defined(SSE_INT_MFP_GPIP_TO_IRQ_DELAY)
/*  If the GPIP input has just transitioned, IRQ hasn't fired 
    yet. Fixes V8 Music System.
*/
                if( MC68901.IrqInfo[irq].IsGpip 
                  && ACT-MC68901.IrqSetTime<0 && ACT-MC68901.IrqSetTime>=-4)
                {
                  ASSERT(irq<4||irq==6||irq==7||irq>13);
                  TRACE_LOG("MFP delay GPIP-irq %d %d\n",irq,ACT-MC68901.IrqSetTime);
                  ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;//come back later
                  break;
                }
#endif    
              }//OPTION_PRECISE_MFP
              mfp_interrupt(irq,ABSOLUTE_CPU_TIME); //then cause interrupt
              break;        //lower priority interrupts not allowed now.
            }//enabled
          }//mask OK
        }//pending
      }//nxt irq
#endif//#if defined(SSE_INT_MFP_REFACTOR2)
    }//ioaccess delay

#else //before refactoring:

  if (STOP_INTS_BECAUSE_INTERCEPT_OS==0){
    if ((ioaccess & IOACCESS_FLAG_DELAY_MFP)==0){ 
      for (int irq=15;irq>=0;irq--)
      {

        BYTE i_bit=BYTE(1 << (irq & 7));
        int i_ab=1-((irq & 8) >> 3);

        if (mfp_reg[MFPR_ISRA+i_ab] & i_bit){ //interrupt in service
//          TRACE_LOG("IRQ %d in service\n",irq);
          break;  //time to stop looking for pending interrupts
        }

#if defined(SSE_INT_MFP_IRQ_DELAY3)
        if(irq==6 && SSE_HACKS_ON
          && ACT-mfp_time_of_set_pending[irq]<4 && ACT-mfp_time_of_set_pending[irq]>=0)
        {
          TRACE_LOG("IRQ %d set pending %d cycles ago\n",irq,ACT-mfp_time_of_set_pending[irq]);
          continue;  
        }
#endif
        if (mfp_reg[MFPR_IPRA+i_ab] & i_bit){ //is this interrupt pending?
          if (mfp_reg[MFPR_IMRA+i_ab] & i_bit){ //is it not masked out?
#if defined(STEVEN_SEAGAL) && defined(SSE_BLT_BLIT_MODE_INTERRUPT)
            if(Blit.HasBus) // opt: we assume the test is quicker than clearing
              Blit.HasBus=false; 
#endif
            mfp_interrupt(irq,ABSOLUTE_CPU_TIME); //then cause interrupt
            break;        //lower priority interrupts not allowed now.
          }
        }
      }//nxt irq
    }
#endif//refactor


    if (vbl_pending){ //SS IPL4
      if ((sr & SR_IPL)<SR_IPL_4){
        VBL_INTERRUPT
#if defined(SSE_GLUE_FRAME_TIMINGS_A)
        if(Glue.Status.hbi_done)
          hbl_pending=false;
#endif
      }
    }
#if defined(SSE_INT_MFP_REFACTOR1)
    else
#endif
    if (hbl_pending
#if defined(SSE_GLUE_FRAME_TIMINGS_A)
      && !Glue.Status.hbi_done
#endif      
      ){ 
      if ((sr & SR_IPL)<SR_IPL_2){ //SS rare
        // Make sure this HBL can't occur when another HBL has already happened
        // but the event hasn't fired yet.
        //SS scanline_time_in_cpu_cycles_at_start_of_vbl is 512, 508 or 224
        //ASSERT( int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)<scanline_time_in_cpu_cycles_at_start_of_vbl );
        if (int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)<scanline_time_in_cpu_cycles_at_start_of_vbl){
          HBL_INTERRUPT;
        }
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG) 
        // can happen quite a lot
        else if(LPEEK(0x0068)<0xFC0000) TRACE_LOG("no hbl %X\n",LPEEK(0x0068));
#endif
      }
    }

  }
  prepare_event_again();
}
//---------------------------------------------------------------------------


#if defined(SSE_INT_MFP_REFACTOR1)

void mfp_interrupt(int irq,int when_fired) {

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
#if !defined(SSE_INT_MFP_AUTO_NO_IS_CLEAR__)//recheck...
    // doc doesn't say it's cleared?
    mfp_reg[MFPR_ISRA+mfp_interrupt_i_ab(irq)]&=BYTE(~mfp_interrupt_i_bit(irq));
#endif

  }
#if defined(STEVEN_SEAGAL) && defined(SSE_IPF) 
  // should be rare, most programs, including TOS, poll
  if(irq==7 && Caps.Active && (Caps.WD1772.lineout&CAPSFDC_LO_INTRQ))
  {
    TRACE_FDC("execute WD1772 irq\n");
    Caps.WD1772.lineout&=~CAPSFDC_LO_INTRQ; 
  }
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_BLT_BLIT_MODE_INTERRUPT)
/*  Stop blitter to start interrupt. This is a bugfix and necessary for Lethal
    Xcess if we use the correct BLIT mode cycles (64x4).
    v3.7 This mod is better placed here, when there's an actual interrupt.
*/
  if(Blit.HasBus) // opt: we assume the test is quicker than clearing
    Blit.HasBus=false; 
#endif

  MEM_ADDRESS vector;
  vector=    (mfp_reg[MFPR_VR] & 0xf0)  +(irq);
  vector*=4;
  //SS timing is recorded before counting any IACK/fetching cycle
  mfp_time_of_start_of_last_interrupt[irq]=ABSOLUTE_CPU_TIME; 

#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)
#if defined(SSE_DEBUG_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
#if defined(SSE_BOILER_FRAME_REPORT_MASK2)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
#else
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_MFP)
#endif
    FrameEvents.Add(scan_y,LINECYCLES,'I',0x60+irq);
#endif
  //TRACE_INT("%d %d %d (%d) PC %X IRQ %d VEC %X ",TIMING_INFO,ACT,old_pc,irq,LPEEK(vector));
  TRACE_INT("%d PC %X IRQ %d VEC %X ",ACT,old_pc,irq,LPEEK(vector));
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

//TRACE("mfp_timer_period_change[3] %d\n",mfp_timer_period_change[3]);


#if defined(SSE_BOILER_FRAME_INTERRUPTS)
  Debug.FrameInterrupts|=4;
  Debug.FrameMfpIrqs|= 1<<irq;
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("MFP",irq);
#endif
#endif//dbg

#if defined(SSE_INT_MFP_IRQ_TIMING)
  if(OPTION_PRECISE_MFP)
  {
#if !defined(SSE_INT_MFP_REFACTOR2)
    MC68901.IackTiming=ACT;//was never used as such
#endif
    MC68901.LastIrq=irq;
    MC68901.UpdateNextIrq();
  }
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP) 
#if defined(SSE_CPU_FETCH_TIMING)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
#if defined(SSE_INT_MFP_REFACTOR2)
  int iack_cycles=ACT-MC68901.IackTiming;
#if defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
  INSTRUCTION_TIME(SSE_INT_MFP_TIMING-12-4-iack_cycles);
#else
  INSTRUCTION_TIME(SSE_INT_MFP_TIMING-4-iack_cycles);
#endif
#elif defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
  INSTRUCTION_TIME_ROUND(SSE_INT_MFP_TIMING-12-4);
#else
  INSTRUCTION_TIME_ROUND(SSE_INT_MFP_TIMING-4);
#endif
  FETCH_TIMING;
#if defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
  CPU_ABUS_ACCESS_READ_PC; // because FETCH_TIMING does nothing
#endif
#else
  INSTRUCTION_TIME_ROUND(34);
#endif
#else
  INSTRUCTION_TIME_ROUND(SSE_INT_MFP_TIMING); // same
#endif//SSE_CPU_FETCH_TIMING
#else
  INSTRUCTION_TIME_ROUND(56);
#endif

  m68k_interrupt(LPEEK(vector));
  sr=WORD((sr & (~SR_IPL)) | SR_IPL_6);
  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("  IRQ fired - vector=")+HEXSl(LPEEK(vector),6));
  debug_check_break_on_irq(irq);

}

#else //before refactoring
void mfp_interrupt(int irq,int when_fired)
{
#if defined(SSE_INT_MFP_WRITE_DELAY1)
  if(SSE_HACKS_ON &&
    abs(when_fired-time_of_last_write_to_mfp_reg)<MFP_WRITE_LATENCY)
  {
    TRACE_LOG("mfp irq %d rejected because of write delay %d\n",irq,abs(when_fired-time_of_last_write_to_mfp_reg));
    return; 
  }
#endif
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
#if defined(SSE_ACIA_IRQ_DELAY2) 
      }else if( SSE_HACKS_ON &&
        irq==6 && (abs(ACT-ACIA_IKBD.last_rx_read_time)<ACIA_RDRF_DELAY)){
        TRACE_LOG("MFP delay irq %d %d\n",irq,ACT-ACIA_IKBD.last_rx_read_time);
#endif
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
#if defined(STEVEN_SEAGAL) && defined(SSE_IPF) 
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
            mfp_time_of_start_of_last_interrupt[irq]=ABSOLUTE_CPU_TIME; 
#ifdef SSE_DEBUG // trace, osd, frame report
#if defined(SSE_BOILER_FRAME_REPORT_MASK)
#if defined(SSE_BOILER_FRAME_REPORT_MASK2)
          if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
#else
          if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_MFP)
#endif
            FrameEvents.Add(scan_y,LINECYCLES,'I',0x70+irq);
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_DEBUG)//tmp
          TRACE_LOG("%d %d %d (%d) PC %X IRQ %d VEC %X ",TIMING_INFO,ACT,old_pc,irq,LPEEK(vector));
          switch(irq)
          {
          case 0:TRACE_LOG("Centronics busy\n");break;
          case 1:TRACE_LOG("RS-232 DCD\n");  break;                                         
          case 2:TRACE_LOG("RS-232 CTS\n");        break;                                   
          case 3:TRACE_LOG("Blitter done\n");            break;                             
          case 4:TRACE_LOG("Timer D\n");         break;                       
          case 5:TRACE_LOG("Timer C\n");    break;                            
          case 6:TRACE_LOG("ACIA\n");   break;                              
          case 7:TRACE_LOG("FDC/HDC\n");   break;                                           
          case 8:TRACE_LOG("Timer B\n");   break;                                     
          case 9:TRACE_LOG("Send Error\n");            break;                               
          case 10:TRACE_LOG("Send buffer empty\n");          break;                         
          case 11:TRACE_LOG("Receive error\n");                    break;                   
          case 12:TRACE_LOG("Receive buffer full\n");        break;                         
          case 13:TRACE_LOG("Timer A\n");              break;                   
          case 14:TRACE_LOG("RS-232 Ring detect\n");                     break;             
          case 15:TRACE_LOG("Monochrome Detect\n");       break;                     
          }//sw
#endif
#if defined(SSE_BOILER_FRAME_INTERRUPTS)
            Debug.FrameInterrupts|=4;
            Debug.FrameMfpIrqs|= 1<<irq;
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
            Debug.RecordInterrupt("MFP",irq);
#endif
#endif//dbg

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP) 
#if defined(SSE_CPU_FETCH_TIMING)
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_FETCH_TIMING)
            INSTRUCTION_TIME_ROUND(SSE_INT_MFP_TIMING-4);
            FETCH_TIMING;
#if defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
            INSTRUCTION_TIME_ROUND(4); 
#endif
#else
            INSTRUCTION_TIME_ROUND(34);
#endif
#else
            INSTRUCTION_TIME_ROUND(SSE_INT_MFP_TIMING);
#endif//SSE_CPU_FETCH_TIMING
#else
            INSTRUCTION_TIME_ROUND(56);
#endif

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
#endif//refactor


#if defined(SSE_INT_MFP_OBJECT) //created v3.7, in old mfp.cpp


TMC68901::TMC68901() {

#if defined(SSE_INT_MFP_UTIL)
  Init();
#endif
}

void TMC68901::Init() {
#if defined(SSE_INT_MFP_UTIL)
  // init IrqInfo structure
  ZeroMemory(&IrqInfo,sizeof(TMC68901IrqInfo)*16);
  IrqInfo[0].IsGpip=IrqInfo[1].IsGpip=IrqInfo[2].IsGpip=IrqInfo[3].IsGpip
    =IrqInfo[6].IsGpip=IrqInfo[7].IsGpip=IrqInfo[14].IsGpip=IrqInfo[15].IsGpip
    =true;
  IrqInfo[4].IsTimer=IrqInfo[5].IsTimer=IrqInfo[8].IsTimer=IrqInfo[13].IsTimer
    =true;
  IrqInfo[4].Timer=3;  // timer D
  IrqInfo[5].Timer=2;  // timer C
  IrqInfo[8].Timer=1;  // timer B
  IrqInfo[13].Timer=0;  // timer A
#endif
}

#if defined(SSE_INT_MFP_UTIL) 
/* we already may use from Steem 3.2:
#define mfp_interrupt_i_bit(irq) (BYTE(1 << (irq & 7)))
#define mfp_interrupt_i_ab(irq) (1-((irq & 8) >> 3))
mfp_interrupt_enabled[irq]
mfp_timer_enabled[tn] 
mfp_timer_irq[tn]

we add functions, some are not used yet...
problem: we're anticipating on a next version but use some functions
already...
*/

#if !defined(SSE_INT_MFP_EVENT_WRITE)
/*  Adding an event is always a load on emulation, but here it does
    away with some little check functions that could be a drain as well.
*/

BYTE TMC68901::GetReg(int reg_num,int at_time) {
  if(at_time==-1)
    at_time=ACT;
  BYTE reg= (reg_num==LastRegisterWritten 
    && at_time-WriteTiming>=0 
    && at_time-WriteTiming<=MFP_SPURIOUS_LATENCY) //should be write_latency
    ? LastRegisterFormerValue : mfp_reg[reg_num];
  //and update if necessary in setpending
  return reg;
}


bool TMC68901::Enabled(int irq,int at_time) {
  
  int reg_num=MFPR_IERA+mfp_interrupt_i_ab(irq);
#if defined(SSE_INT_MFP_UTIL2)
  BYTE reg=GetReg(reg_num,at_time);
#else
  BYTE reg=GetReg(reg_num);
#endif
  bool is_enabled=(bool)(reg & mfp_interrupt_i_bit(irq));
  return is_enabled;

}


bool TMC68901::InService(int irq,int at_time){
  int reg_num=MFPR_ISRA+mfp_interrupt_i_ab(irq);
#if defined(SSE_INT_MFP_UTIL2)
  BYTE reg=GetReg(reg_num,at_time);
#else
  BYTE reg=GetReg(reg_num);
#endif
  bool in_service=(reg & mfp_interrupt_i_bit(irq)); 
  return in_service;
}


bool TMC68901::MaskOK(int irq,int at_time) { //used
//  ASSERT(at_time==-1);
  int reg_num=MFPR_IMRA+mfp_interrupt_i_ab(irq);
#if defined(SSE_INT_MFP_UTIL2)
  BYTE reg=GetReg(reg_num,at_time);
#else
  BYTE reg=GetReg(reg_num);
#endif
  bool mask_ok=(reg & mfp_interrupt_i_bit(irq)); 
  return mask_ok;
}


bool TMC68901::Pending(int irq,int at_time) {
#if defined(SSE_INT_MFP_REFACTOR2)
  int reg_num=MFPR_IPRA+mfp_interrupt_i_ab(irq);
  BYTE reg=GetReg(reg_num,at_time);
#else
  int reg_num=MFPR_IMRA+mfp_interrupt_i_ab(irq); // argh! mask
  BYTE reg=GetReg(reg_num); //argh! timing
#endif
  // if it times out just before the effective write to reg
  // we're checking later anyway and it's rightfully cleared
  bool is_pending= 
#ifdef SSE_INT_MFP_RECORD_PENDING_TIMING
    (at_time-PendingTiming[irq]>=0) && 
#endif
    ((reg & mfp_interrupt_i_bit(irq)));
  return is_pending;
}
#endif

#if !defined(SSE_INT_MFP_REFACTOR2)
bool TMC68901::TimerBActive() {
  bool timer_b_active=mfp_reg[MFPR_TBCR]==8 && mfp_interrupt_enabled[8] 
    && MaskOK(8);
  return timer_b_active;
}
#endif

#endif

#if defined(SSE_INT_MFP_SPURIOUS) && !defined(SSE_INT_MFP_REFACTOR2)
/*  TEST10.TOS, TEST10B.TOS, TEST10C.TOS, TEST10D.TOS
    Fun in Steem!!
    This is quite a lot of tests we add, and it doesn't help any
    known case, but this could be useful for programmers who code 
    against Steem. Optional.
    Idea: when you write on a MFP register, a timer can trigger
    during the instruction. In that case IRQ is asserted by the MFP,
    but negated right after, yet the CPU started a IACK cycle.
    On the ST that should produce 24 bombs.
    Tests not perfect, work with test programs (without breaking everything).
    In v3.8, those complicated tests have been replaced with something
    more simple.
*/

bool TMC68901::CheckSpurious(int irq) {
  ASSERT((sr&SR_IPL_6)<SR_IPL_6);
  ASSERT( OPTION_PRECISE_MFP ); // you need more CPU power
  bool spurious_triggered=false;

#if defined(SSE_INT_MFP_SPURIOUS_372)
/* v3.7.2 bugfix Return STE -HMD
    dangerous code (interrupt is pending):
	move #$2700,sr                                   ; 012DA2: 46FC 2700 
	move.l #$12dcc,$68.w                             ; 012DA6: 21FC 0001 2DCC 0068 
	move.b $fa13.W,$12e2d                            ; 012DAE: 13F8 FA13 0001 2E2D 
	move.b $fa15.W,$12e33                            ; 012DB6: 13F8 FA15 0001 2E33 
	clr.b $fa13.W                                    ; 012DBE: 4238 FA13 
	clr.b $fa15.W                                    ; 012DC2: 4238 FA15 
	stop #$2100                                      ; 012DC6: 4E72 2100 
	bra.s -6 {$012DC6}                               ; 012DCA: 60FA 

This should trigger spurious but it doesn't on real STE.
Fix is pragmatic (TEST10 still working).
This whole part isn't compiled now anayway (3.8)
*/
  BYTE mfp_write_latency=MFP_WRITE_LATENCY;
  if(LastRegisterWritten==MFPR_IMRA||LastRegisterWritten==MFPR_IMRB) //mask
    mfp_write_latency-=1; // it's all done after the STOP (no IRQ)
#endif

  if(IrqInfo[irq].IsTimer && ACT-WriteTiming>=0 
#if defined(SSE_INT_MFP_SPURIOUS_372)
    && ACT-WriteTiming<=mfp_write_latency)
#else
    && ACT-WriteTiming<=MFP_SPURIOUS_LATENCY)
#endif
  {

    BYTE i_ab=mfp_interrupt_i_ab(irq);
    BYTE i_bit=mfp_interrupt_i_bit(irq);

    int next_timeout=(irq==8 && mfp_reg[MFPR_TBCR]==8)
      ? time_of_next_timer_b : mfp_timer_timeout[IrqInfo[irq].Timer];

    BYTE reg_enabled_before=(LastRegisterWritten==MFPR_IERA+i_ab)
      ? LastRegisterFormerValue : mfp_reg[MFPR_IERA+i_ab];

    BYTE reg_pending_before=(LastRegisterWritten==MFPR_IPRA+i_ab)
      ? LastRegisterFormerValue : mfp_reg[MFPR_IPRA+i_ab];

    BYTE reg_pending_after=mfp_reg[MFPR_IPRA+i_ab];


    // Value of register is set only after 4 cycles, possibly
    // voiding an interrupt.
#if defined(SSE_INT_MFP_SPURIOUS_372)
    if(IrqSetTime-WriteTiming<=mfp_write_latency)
#else
    if(IrqSetTime-WriteTiming<=MFP_WRITE_LATENCY)
#endif
    {
      if(LastRegisterWritten==MFPR_IPRA+i_ab)
      {
        reg_pending_after=LastRegisterWrittenValue;     
        reg_pending_before=mfp_reg[MFPR_IPRA+i_ab];

        //TRACE("%d IPR%c before %X after %X next timeout %d last write %d\n",
        //  ACT,'A'+i_ab,reg_pending_before,reg_pending_after,next_timeout,MC68901.WriteTiming);

        // maybe an irq was voided but it will timeout again during IACK!
        if( (reg_pending_before&i_bit) && !(reg_pending_after&i_bit) 
          && (next_timeout-ACT>0 
          && next_timeout-ACT<=MFP_IACK_LATENCY-4))
        {
          TRACE_OSD("cancel spurious");
          TRACE_LOG("%d next_timeout-ACT=%d next_timeout-WriteTiming=%d\n",ACT,next_timeout-ACT,next_timeout-WriteTiming);
          reg_pending_after|=i_bit;
        }
      }
    }

    BYTE reg_masked_before=(LastRegisterWritten==MFPR_IMRA+i_ab)
      ? LastRegisterFormerValue : mfp_reg[MFPR_IMRA+i_ab];

#if defined(SSE_INT_MFP_SPURIOUS_372)
    BYTE reg_masked_after=(LastRegisterWritten==MFPR_IMRA+i_ab 
      && ACT-WriteTiming<mfp_write_latency)//MFP_WRITE_LATENCY+2)
      ? LastRegisterFormerValue : mfp_reg[MFPR_IMRA+i_ab];
#endif

    //  TRACE_LOG("%d ?Spurious irq %d timer %d skip %d reg_enabled_before %X  reg_pending_before %X reg_masked_before %X timeout %d last reg %d former value %X\n",
  // ACT,irq,IrqInfo[irq].Timer,MC68901.SkipTimer[IrqInfo[irq].Timer],reg_enabled_before,reg_pending_before,reg_masked_before,next_timeout,MC68901.LastRegisterWritten,MC68901.LastRegisterFormerValue);

    if( (reg_enabled_before&i_bit) && (reg_masked_before&i_bit)
      // was pending or will pend very soon
      && ( (reg_pending_before&i_bit) || 
      LastRegisterWritten==MFPR_IPRA+i_ab 
      && next_timeout-ACT>=0 //fuz105, LTC2
      && next_timeout-WriteTiming<=MFP_WRITE_LATENCY)
      && ( !(mfp_reg[MFPR_IERA+i_ab] & i_bit) ||!(reg_pending_after & i_bit)
#if defined(SSE_INT_MFP_SPURIOUS_372)
      ||!(reg_masked_after & i_bit)))
#else
      ||!(mfp_reg[MFPR_IMRA+i_ab] & i_bit)))
#endif
    {
      spurious_triggered=true;
    }
    // expect reports of bad spurious now! (hence TRACE, not _LOG)
    if(spurious_triggered) 
    {
      TRACE_OSD("Spurious!");
      TRACE("%d SPURIOUS irq %d timer %d timeout %d before IE%d IM%d IP%d now IE%d IM%d IP%d last reg %d former value %X at %d diff %d\n",
        ACT,irq,IrqInfo[irq].Timer,next_timeout,
        !!(reg_enabled_before&i_bit),!!(reg_masked_before&i_bit),!!(reg_pending_before&i_bit),
        !!(mfp_reg[MFPR_IERA+i_ab] & i_bit),!!(mfp_reg[MFPR_IMRA+i_ab] & i_bit),!!(reg_pending_after & i_bit),
        LastRegisterWritten,LastRegisterFormerValue,WriteTiming,next_timeout-WriteTiming);
#if defined(SSE_CPU_TIMINGS_REFACTOR_PUSH)
      INSTRUCTION_TIME_ROUND(50-12); //?
#else
      INSTRUCTION_TIME(50); //?
#endif
      m68k_interrupt(LPEEK(0x60)); // vector for Spurious, NOT Bus Error
      sr=WORD((sr & (~SR_IPL)) | SR_IPL_6); // the CPU does that anyway

    }
  }
  return spurious_triggered;
}
          
#endif

#if defined(SSE_INT_MFP_IRQ_TIMING)
/*  This logic is instant on the MFP.
    The IRQ line depends only on MFP registers, not on IPL in the CPU.
    It may be asserted then later negated without the CPU handling it.
    In v3.7, it was useful for some cases.
    In v3.8 it has become vital for precise MFP emu.
*/

#if defined(SSE_INT_MFP_REFACTOR2)
int TMC68901::UpdateNextIrq(int at_time) {
#else
int TMC68901::UpdateNextIrq(int start_from_irq,int at_time) {
#endif
  ASSERT(OPTION_PRECISE_MFP);
  NextIrq=-1; //default: none in sight
  Irq=false;
  if(at_time==-1)//default
    at_time=ACT;
#if defined(SSE_INT_MFP_REFACTOR2)
  if(MFP_IRQ) // global test before we check each irq
    for (int irq=15;irq>=0;irq--) { //need to check all
#else
  for (int irq=start_from_irq;irq>=0;irq--) {
#endif
    BYTE i_ab=mfp_interrupt_i_ab(irq);
    BYTE i_bit=mfp_interrupt_i_bit(irq);
#if !defined(SSE_INT_MFP_REFACTOR2)
    if(InService(irq,at_time))
      break;
#endif
#if defined(SSE_INT_MFP_EVENT_WRITE)
    if((i_bit // instant test thanks to event system
    & mfp_reg[MFPR_IERA+i_ab]&mfp_reg[MFPR_IPRA+i_ab]&mfp_reg[MFPR_IMRA+i_ab])
#if defined(SSE_INT_MFP_REFACTOR2A)
    && !(i_bit&mfp_reg[MFPR_ISRA+i_ab])
#endif
    )
#else
    if(Enabled(irq,at_time) && Pending(irq,at_time) && MaskOK(irq,at_time))
#endif
    {
      Irq=true; // line is asserted by the MFP
      NextIrq=irq;
      IrqSetTime=at_time;
      TRACE_MFP("%d MFP IRQ (%d)\n",ACT,NextIrq);
      break;
    }
#if defined(SSE_INT_MFP_REFACTOR2A)
    else if((i_bit&mfp_reg[MFPR_ISRA+i_ab]))
      break; // no IRQ possible then (was bug)
#endif
  }//nxt irq
#if defined(SSE_INT_MFP_REFACTOR2) && !defined(SSE_INT_MFP_REFACTOR2A) && defined(SSE_DEBUG)
  if(!Irq) 
   ASSERT(!MFP_IRQ);
#endif
  return NextIrq;
}
#endif

#endif


#if defined(SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS) 
/*  Shifter tricks can change timing of timer B. This wasn't handled yet
    in Steem. Don't think any game/demo depends on this, I added it for a
    test program.
    Refactoring due together with shifter tricks. [?]
*/

void TMC68901::AdjustTimerB() {
  ASSERT(OPTION_PRECISE_MFP); // another waste of cycles...
  if(mfp_reg[MFPR_TBCR]!=8)
    return;
  int CyclesIn=LINECYCLES;
  int linecycle_of_end_de=(mfp_reg[MFPR_AER]&8)
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
    ?Glue.CurrentScanline.StartCycle:Glue.CurrentScanline.EndCycle;
#else
    ?Shifter.CurrentScanline.StartCycle:Shifter.CurrentScanline.EndCycle;
#endif
  if(linecycle_of_end_de==-1) //0byte -> no timer B?
    linecycle_of_end_de+=scanline_time_in_cpu_cycles_8mhz[shifter_freq_idx];
  
  if(CyclesIn<=linecycle_of_end_de 
    && linecycle_of_end_de-(time_of_next_timer_b-28-LINECYCLE0) >2)
  {
    int tmp=time_of_next_timer_b-LINECYCLE0;
    bool adapt_time_of_next_event=(time_of_next_event==time_of_next_timer_b);
    time_of_next_timer_b=LINECYCLE0+linecycle_of_end_de+28+TB_TIME_WOBBLE;
    TRACE_LOG("F%d y%d c%d timer b type %d %d -> %d adapt %d\n",TIMING_INFO,tmp,(mfp_reg[MFPR_AER]&8),time_of_next_timer_b-LINECYCLE0,adapt_time_of_next_event);
    if(adapt_time_of_next_event)
      time_of_next_event=time_of_next_timer_b;
  }
}

#endif

#if defined(SSE_INT_MFP_TIMER_B_AER2) // refactoring

void TMC68901::CalcCyclesFromHblToTimerB(int freq) {
  switch (freq){ //this part was the macro
    case MONO_HZ: cpu_cycles_from_hbl_to_timer_b=192;break; 
    case 60: cpu_cycles_from_hbl_to_timer_b=(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320-4);break; 
    default: cpu_cycles_from_hbl_to_timer_b=(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320); 
  }
  if(OPTION_PRECISE_MFP && (mfp_reg[MFPR_AER]&8)) 
    // from Hatari, fixes Seven Gates of Jambala; Trex Warrior
    cpu_cycles_from_hbl_to_timer_b-=Glue.DE_cycles[shifter_freq_idx];
  ASSERT(cpu_cycles_from_hbl_to_timer_b<512); // can be short
}

#endif

#if defined(SSE_INT_MFP_REFACTOR2)
void TMC68901::Reset(bool Cold) {
  Irq=false;
  IrqSetTime=ACT;
}
#endif

//---------------------------------------------------------------------------
#undef LOGSECTION


#if !defined(STEVEN_SEAGAL) // this function isn't used
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
