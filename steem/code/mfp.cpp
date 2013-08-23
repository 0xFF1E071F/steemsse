/*---------------------------------------------------------------------------
FILE: mfp.cpp
MODULE: emu
DESCRIPTION: The core of Steem's Multi Function Processor emulation
(MFP 68901). This chip handles most of the interrupt and timing functions in
the ST.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_INFO)
#pragma message("Included for compilation: mfp.cpp")
#endif

#define LOGSECTION LOGSECTION_INTERRUPTS//SS

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
  //SS detects if we're setting an IRQ
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
#if defined(STEVEN_SEAGAL) && defined(SS_MFP_IRQ_DELAY)
/*  Both SS_MFP_IRQ_DELAY and SS_MFP_IRQ_DELAY2 organise a
    "dirty quick fix" approach to imitate a feature that's more structurally
    implemented in Hatari.
    Because of this, the mods are protected by option 'Hacks'.
    TODO: more reliable

2013/03/01 [NP] When MFP_IRQ goes from 0 to 1, the resulting signal is visible
to the CPU only 4 cycles later
*/
    if(SSE_HACKS_ON)
      ioaccess|=IOACCESS_FLAG_DELAY_MFP; // fixes V8 Music System
    else
      ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS;
#endif
  }
}
//---------------------------------------------------------------------------
void calc_time_of_next_timer_b()
{
/*
                                      The  Horizontal  Blanking (down)
          Counter MFP Timer B has an active high input signal and pro-
          duces  an  interrupt when the counter times out (Event Count
          Mode).   The  horizontal  blanking  counter  actually   uses
          display  enable, the first of which occurs at the end of the
          first display line.
*/
/* Only fetching lines are counted.
   This function is only called by mfp_set_timer_reg().
   When a new timer B is set, we check if it still could trigger this
   scanline. TODO simplify?
*/

  int cycles_in=int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
  if (cycles_in<cpu_cycles_from_hbl_to_timer_b){
    if (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line){
#if defined(SS_MFP_TIMER_B)
      if(mfp_reg[1]&8)
        time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
      else
#endif
      time_of_next_timer_b=cpu_timer_at_start_of_hbl+cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
    }else{
      time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
    }
  }else{
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
  }
}
//---------------------------------------------------------------------------
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
*/
  if(abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])
    >= (SSE_HACKS_ON
    ?56-8+CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED
    :CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED)){
#else
  if (abs_quick(when_set-mfp_time_of_start_of_last_interrupt[irq])>=CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED){
#endif
    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq); // Set pending
    return true;
  }
  return (mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)] & mfp_interrupt_i_bit(irq))!=0;
}

#undef LOGSECTION//ss
#define LOGSECTION LOGSECTION_MFP_TIMERS
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
    INSTRUCTION_TIME(12); // The MFP doesn't do anything until 12 cycles after the write
    do{
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

        if (new_control){ // Timer running in delay mode
                          // SS or pulse, but it's very unlikely
          mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME;
          mfp_timer_timeout[timer]+=int(double(mfp_timer_prescale[new_control]*mfp_timer_counter[timer]/64)*CPU_CYCLES_PER_MFP_CLK);
//          mfp_timer_timeout[timer]=ABSOLUTE_CPU_TIME+
//              (mfp_timer_prescale[new_control]*(mfp_timer_counter[timer])*125)/MFP_CLK;
              //*8000/MFP_CLK for MFP cycles, /64 for counter resolution
          mfp_timer_enabled[timer]=mfp_interrupt_enabled[mfp_timer_irq[timer]];

          // To make this more accurate for short timers, we should store the fractional
          // part as well.  Then every time it times out, increase the fractional part and
          // see if it goes over one.  If it does, make the next time-out a bit later.
          mfp_timer_period[timer]=int( double(mfp_timer_prescale[new_control]*int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer]))) * CPU_CYCLES_PER_MFP_CLK);

#if defined(STEVEN_SEAGAL) && defined(SS_MFP_RATIO_PRECISION)
/*  Here we do exactly what Steem authors suggested above, and it does bring the
    timing measurements at the same level as SainT and Hatari (HWTST001.PRG),
    so it's definitely an improvement, and it isn't complicated at all!
*/
          mfp_timer_period_fraction[timer]=int(  1000*((double(mfp_timer_prescale[new_control]*int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+timer]))) * CPU_CYCLES_PER_MFP_CLK)-(double)mfp_timer_period[timer])  );
          mfp_timer_period_current_fraction[timer]=0;
          TRACE_LOG("PC %X MFP set timer %C control %x period %d.%d\n",old_pc,'A'+timer,new_control,mfp_timer_period[timer],mfp_timer_period_fraction[timer]);
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
          mfp_timer_timeout[timer]-=cpu_time_of_first_mfp_tick;
          
          // Convert to MFP cycles
          mfp_timer_timeout[timer]*=MFP_CLK;
          mfp_timer_timeout[timer]/=8000;

          // Take off number of cycles already counted
          mfp_timer_timeout[timer]-=prescale_count;

          // Convert back to CPU time
          mfp_timer_timeout[timer]*=8000;
          mfp_timer_timeout[timer]/=MFP_CLK;

          // Make absolute time again
          mfp_timer_timeout[timer]+=cpu_time_of_first_mfp_tick;

          log(EasyStr("    Set control to ")+new_control+
                " (reg=$"+HEXSl(new_val,2)+"); data="+mfp_reg[MFPR_TADR+timer]+
                "; counter="+mfp_timer_counter[timer]/64+
                "; period="+mfp_timer_period[timer]+
                "; new timeout="+mfp_timer_timeout[timer]);
        }else{  //timer stopped, or in event count mode
          // This checks all timers to see if they have timed out, if they have then
          // it will set the pend bit. This is dangerous, messes up LXS!
          // mfp_check_for_timer_timeouts(); // ss not implemented
          TRACE_LOG("MFP Timer %C stopped\n",'A'+timer);
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
    INSTRUCTION_TIME(-12);

    if (reg==MFPR_TBCR && new_val==8)  // SS event count (HBL++)
      calc_time_of_next_timer_b();// SS it is the general use of this timer

#ifdef ENABLE_LOGGING
    if (reg<=MFPR_TBCR && new_val>8){
      log("MFP: --------------- PULSE EXTENSION MODE!! -----------------");
    }
#endif
    ASSERT(!(reg<=MFPR_TBCR && new_val>8)); 
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
  if (cr & 7){
    int stage=mfp_timer_timeout[timer]-ABSOLUTE_CPU_TIME;
    if (stage<0){
      MFP_CALC_TIMER_PERIOD(timer);
      stage+=((-stage/mfp_timer_period[timer])+1)*mfp_timer_period[timer];
    }
    stage%=mfp_timer_period[timer];
    // so stage is a number from 0 to mfp_timer_period-1

    int ticks_per_count=mfp_timer_prescale[cr & 7];
    // Convert to number of MFP cycles until timeout
    stage=int(double(stage)/CPU_CYCLES_PER_MFP_CLK);
    mfp_timer_counter[timer]=(stage/ticks_per_count)*64 + 64;
    // return the number of prescale counts done so far
    return ticks_per_count-((stage % ticks_per_count)+1);
  }
  return 0;
}

#undef LOGSECTION

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

#define LOGSECTION LOGSECTION_INTERRUPTS//SS
//SS  check_for_interrupts_pending() concerns not just the MFP...


/*  TODO change for higher timer during IACK, but are there cases?
*/

void ASMCALL check_for_interrupts_pending()
{

  if (STOP_INTS_BECAUSE_INTERCEPT_OS==0){
    if ((ioaccess & IOACCESS_FLAG_DELAY_MFP)==0){ //SS MFP=IPL6
      //SS check IRQ from highest (15) to lowest (0)
      for (int irq=15;irq>=0;irq--)
      {

        BYTE i_bit=BYTE(1 << (irq & 7));
        int i_ab=1-((irq & 8) >> 3);

        if (mfp_reg[MFPR_ISRA+i_ab] & i_bit){ //interrupt in service
          TRACE_LOG("IRQ %d in service\n",irq);
          break;  //time to stop looking for pending interrupts
        }

        if (mfp_reg[MFPR_IPRA+i_ab] & i_bit){ //is this interrupt pending?
          if (mfp_reg[MFPR_IMRA+i_ab] & i_bit){ //is it not masked out?

#if defined(STEVEN_SEAGAL) && defined(SS_BLT_BLIT_MODE_INTERRUPT)
/*  Stop blitter to start interrupt. This is a bugfix and necessary for Lethal
    Xcess if we use the correct BLIT mode cycles (64x4). (I think)
*/
            if(Blit.HasBus) // opt: we assume the test is quicker than clearing
              Blit.HasBus=false; 
#endif
            mfp_interrupt(irq,ABSOLUTE_CPU_TIME); //then cause interrupt
            break;        //lower priority interrupts not allowed now.
          }
        }
      }//nxt irq
    }
#if defined(STEVEN_SEAGAL) && defined(SS_INT_VBI_START) // normally, no
/*
We wouldn't do it here
    if (
#ifdef SS_SHIFTER
      Shifter.nVbl && // hack for resuming emu (auto.sts)
#endif
      vbl_pending && ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl>64+4){
      //TRACE_LOG("delayed vbl %d y %d SR %X\n",FRAME,scan_y,sr);
      if ((sr & SR_IPL)<SR_IPL_4){
        ASSERT(!Blit.HasBus);
        VBL_INTERRUPT
//        if(LPEEK(0x70)!=0xFC06DE) TRACE_LOG("VBL %d vector %X\n",Shifter.nVbl,LPEEK(0x70));
      }
    }
*/
#else

/*  TODO check change during IACK but is it useful??
*/


     if (vbl_pending){ //SS IPL4
      if ((sr & SR_IPL)<SR_IPL_4){
        VBL_INTERRUPT
  ////      if(LPEEK(0x70)!=0xFC06DE) TRACE_LOG("F%d VBI %X\n",FRAME,LPEEK(0x70));
      }
    }

#endif

    if (hbl_pending){ //SS IPL2 - rare
      if ((sr & SR_IPL)<SR_IPL_2){
        // Make sure this HBL can't occur when another HBL has already happened
        // but the event hasn't fired yet.
        if (int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)<scanline_time_in_cpu_cycles_at_start_of_vbl){
          ASSERT(!Blit.HasBus);
          HBL_INTERRUPT;
        }
#if defined(STEVEN_SEAGAL) && defined(SS_INT_HBL) // can happen quite a lot
        else if(LPEEK(0x0068)<0xFC0000) TRACE_LOG("no hbl %X\n",LPEEK(0x0068));
#endif
      }
    }
  }
  prepare_event_again();
}
//---------------------------------------------------------------------------
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
#if defined(STEVEN_SEAGAL) && defined(SS_IPF) 
            // should be rare, most programs, including TOS, poll
            if(irq==7 && Caps.Active && (Caps.WD1772.lineout&CAPSFDC_LO_INTRQ))
            {
              TRACE_LOG("execute WD1772 irq\n");
              Caps.WD1772.lineout&=~CAPSFDC_LO_INTRQ; 
            }
#endif
            MEM_ADDRESS vector;
            vector=    (mfp_reg[MFPR_VR] & 0xf0)  +(irq);
            vector*=4;
            mfp_time_of_start_of_last_interrupt[irq]=ABSOLUTE_CPU_TIME;
#if defined(STEVEN_SEAGAL) && defined(SS_INT_MFP)
#if defined(SS_CPU_FETCH_TIMING)
#if defined(STEVEN_SEAGAL) && defined(SS_CPU_FETCH_TIMING)
            INSTRUCTION_TIME_ROUND(SS_INT_MFP_TIMING-4);
            FETCH_TIMING;
#if defined(SS_CPU_PREFETCH_TIMING_SET_PC)
    INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#else
            INSTRUCTION_TIME_ROUND(34);
#endif
#else
            INSTRUCTION_TIME_ROUND(SS_INT_MFP_TIMING); // same
#endif
#else
            INSTRUCTION_TIME_ROUND(56);
#endif


#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)//tmp
//            TRACE_LOG("MFP Execute IRQ %d Vector %X Address %X\n",irq,vector,LPEEK(vector));
            TRACE_LOG("MFP %d IRQ %d ",interrupt_depth,irq);
            switch(irq)
            {
            case 0:TRACE_LOG("Centronics busy\n");break;
            case 1:TRACE_LOG("RS-232 DCD\n");  break;                                         
            case 2:            TRACE_LOG("RS-232 CTS\n");        break;                                   
            case 3:     TRACE_LOG("Blitter done\n");            break;                             
            case 4:            TRACE_LOG("Timer D\n");         break;                       
            case 5:     TRACE_LOG("Timer C\n");    break;                            
            case 6:            TRACE_LOG("ACIA\n");   break;                              
            case 7:     TRACE_LOG("FDC/HDC\n");   break;                                           
            case 8:            TRACE_LOG("Timer B\n");   break;                                     
            case 9:     TRACE_LOG("Send Error\n");            break;                               
            case 10:            TRACE_LOG("Send buffer empty\n");          break;                         
            case 11:     TRACE_LOG("Receive error\n");                    break;                   
            case 12:            TRACE_LOG("Receive buffer full\n");        break;                         
            case 13:     TRACE_LOG("Timer A\n");              break;                   
            case 14:            TRACE_LOG("RS-232 Ring detect\n");                     break;             
            case 15:     TRACE_LOG("Monochrome Detect\n");       break;                     

            }//sw
//            ASSERT( interrupt_depth<2 );
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
