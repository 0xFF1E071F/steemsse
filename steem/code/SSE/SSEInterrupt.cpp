// note: this is part of emu.cpp module
#if defined(SSE_INTERRUPT)
  
#if defined(SSE_CPU_MFP_RATIO) // need no SSE_STF
DWORD CpuNormalHz=CPU_STF_PAL;
#if defined(SSE_CPU_MFP_RATIO_OPTION)
DWORD CpuCustomHz=CPU_STF_PAL;
#endif
double CpuMfpRatio=(double)CpuNormalHz/(double)MFP_CLOCK;
#endif


#undef LOGSECTION
#define LOGSECTION LOGSECTION_INTERRUPTS


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
          The mention of MK68901 MFP as interrupt autovector 6 is misleading.


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

void ASMCALL check_for_interrupts_pending() {
/*  This function was in mfp.cpp, we moved it here because it checks for
    HBI and VBI as well as MFP interrupts.
*/

#if defined(SSE_INT_MFP)
/*  For MFP interrupts, there are all sorts of tests before triggering the
    interrupt, the problem was that some of those tests were in mfp_interrupt().
    Here we place all the tests in check_for_interrupts_pending() and when
    mfp_interrupt() is called from here, the interrupt is executed for sure.
*/
  if(!STOP_INTS_BECAUSE_INTERCEPT_OS) //internal Steem flags
  {
    if (!(ioaccess & IOACCESS_FLAG_DELAY_MFP) //internal Steem flag
      && ((sr & SR_IPL)<SR_IPL_6)) //MFP can interrupt to begin with
    {
      BYTE iack_latency=MFP_IACK_LATENCY; //28
      MC68901.IackTiming=ACT; // record start of IACK cycle
#if defined(SSE_OSD_CONTROL) && !defined(SSE_DEBUG_MFP_DEACTIVATE_IACK_SUBSTITUTION)
      int detect_iack_subst=0;
#endif
      if(OPTION_C2) // if not, "old" Steem test
      {
#if defined(SSE_INT_MFP_SPURIOUS)
        // make sure we don't start IACK if IRQ should be cleared by a write
        if(MC68901.WritePending&&ACT-MC68901.WriteTiming>=MFP_WRITE_LATENCY)
          event_mfp_write(); // this will update MC68901.Irq
#endif
        if(MC68901.Irq) 
        {
          ASSERT(MFP_IRQ);
//          TRACE_MFP("%d Start IACK for irq %d\n",MC68901.IackTiming,MC68901.NextIrq);
#if defined(SSE_INT_MFP_SPURIOUS) 
          bool no_real_irq=false; // if irq wasn't really set, there can be no spurious
#endif
#if defined(SSE_INT_MFP_IACK)
/*  Start the IACK cycle. We don't do the actual pushing yet.
    But we trigger any event during the cycle. This could change
    the irq, or clear two interrupts at once.
    Cases:
    Anomaly menu
    Extreme Rage guest screen
    Final Conflict
    Froggies/OVR
    Fuzion 77, 78, 84 (STF), 146...
    There are many other cases where it happens but emulation seems
    fine without the feature.
*/
          WORD oldsr=sr;
          sr=WORD((sr & (~SR_IPL)) | SR_IPL_6); // already forbid other interrupts
          while(cpu_cycles&&cpu_cycles<=iack_latency) // can be negative! (Froggies/OVR)
          {
            iack_latency-=cpu_cycles;
            INSTRUCTION_TIME(cpu_cycles); // hop to event

#if defined(SSE_BOILER)
#if defined(SSE_BOILER_TRACE_EVENTS)
            TRACE_MFP(">IACK ");
            if(TRACE_ENABLED(LOGSECTION_MFP))
              TRACE_EVENT(screen_event_vector);
#endif
#if defined(SSE_OSD_CONTROL)
            // not super-smart
            if(screen_event_vector==event_timer_a_timeout
              || screen_event_vector==event_timer_b_timeout
              || screen_event_vector==event_timer_c_timeout
              || screen_event_vector==event_timer_d_timeout
              || screen_event_vector==event_timer_b)
              detect_iack_subst=MC68901.NextIrq;//maybe
#endif
#endif

            screen_event_vector();  // trigger event
#if defined(SSE_INT_MFP_SPURIOUS)
/*  As soon as an event clears IRQ during IACK, we go in panic mode. 
    This should trigger spurious interrupt.
    It could actually depend on when exactly during IACK but in SPURIOUS.TOS,
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
#endif//#if defined(SSE_INT_MFP_IACK)

          // check irq, starting with highest priority
          char irq; //signed
          for (irq=15;irq>=0;irq--) {
            BYTE i_ab=mfp_interrupt_i_ab(irq);
            BYTE i_bit=mfp_interrupt_i_bit(irq);
            if (mfp_reg[MFPR_ISRA+i_ab] & i_bit){ //interrupt in service
              break;  //time to stop looking for pending interrupts
            }
            BYTE relevant_pending_register=mfp_reg[MFPR_IPRA+i_ab];
            if (relevant_pending_register & i_bit){ //is this interrupt pending?
              BYTE relevant_mask_register=mfp_reg[MFPR_IMRA+i_ab];
              if(relevant_mask_register&i_bit) 
              {
                ASSERT(mfp_reg[MFPR_IERA+i_ab] & i_bit);
                ASSERT(!(mfp_reg[MFPR_ISRA+i_ab] & i_bit));
                {
#if defined(SSE_INT_MFP_IRQ_TIMING) && defined(SSE_INT_MFP_GPIP_TO_IRQ_DELAY)
/*  If the GPIP input has just transitioned, IRQ hasn't fired 
    yet. Fixes V8 Music System.
*/
                  if( MC68901.IrqInfo[irq].IsGpip 
                    && MC68901.IackTiming-MC68901.IrqSetTime<0 
                    && MC68901.IackTiming-MC68901.IrqSetTime>=-4)
                  {
                      ASSERT(irq<4||irq==6||irq==7||irq>13);
                      TRACE_MFP("MFP delay GPIP-irq %d %d\n",irq,MC68901.IackTiming-MC68901.IrqSetTime);
                      ioaccess|=IOACCESS_FLAG_DELAY_MFP;
#if defined(SSE_INT_MFP_SPURIOUS)  
                      no_real_irq=true; // don't trigger spurious interrupt!
#endif
                      continue; // examine lower irqs too
                  }
#endif    
#if defined(SSE_OSD_CONTROL) && !defined(SSE_DEBUG_MFP_DEACTIVATE_IACK_SUBSTITUTION)
                  if((OSD_MASK1 & OSD_CONTROL_IACK) && detect_iack_subst)
                    TRACE_OSD("IACK %d %d -> %d",iack_latency,detect_iack_subst,irq);
#endif
                  mfp_interrupt(irq); //then cause interrupt
                  break;        //lower priority interrupts not allowed now.
                }//enabled
              }//mask OK
            }//pending
          }//nxt irq
          //ASSERT(irq>-1);
#if defined(SSE_INT_MFP_SPURIOUS)          
/*  The dangerous spurious test.
    It triggers automatically at the end of IACK if we couldn't find an irq.
    Possible cases: 
    TEST10.TOS and SPURIOUS.TOS (of course, we try to)
    Pacemaker STE! Spurious interrupt has a handler (RTE)
    Zikdisk2        ditto
*/
          if(irq==-1 && !no_real_irq) // couldn't find one and there was no break
          {
            TRACE_OSD("Spurious! %d",iack_latency);
            //TRACE_OSD2("Spurious"); // but maybe it annoys player
            TRACE2("Spurious\n");
            TRACE_MFP("%d PC %X Spurious! %d\n",ACT,old_pc,iack_latency);
            TRACE_MFP("IRQ %d (%d) IERA %X IPRA %X IMRA %X ISRA %X IERB %X IPRB %X IMRB %X ISRB %X\n",MC68901.Irq,MC68901.NextIrq,mfp_reg[MFPR_IERA],mfp_reg[MFPR_IPRA],mfp_reg[MFPR_IMRA],mfp_reg[MFPR_ISRA],mfp_reg[MFPR_IERB],mfp_reg[MFPR_IPRB],mfp_reg[MFPR_IMRB],mfp_reg[MFPR_ISRB]);
            int iack_cycles=ACT-MC68901.IackTiming;
#if defined(SSE_CPU_392B)
#if defined(SSE_CPU_BUS_ERROR_TIMING)
            INSTRUCTION_TIME(70-iack_cycles);
#endif
            M68000.ProcessingState=TM68000::EXCEPTION;
#endif
            //INSTRUCTION_TIME(50-iack_cycles); //?
            m68kInterruptTiming();
            m68k_interrupt(LPEEK(0x60)); // vector for Spurious, NOT Bus Error
            sr=WORD((sr & (~SR_IPL)) | SR_IPL_6); // the CPU does that anyway
          }
#endif//spurious
        }//MC68901.Irq
      }//precise
      // no C2 option = Steem 3.2
      else for (int irq=15;irq>=0;irq--)
      {
        ASSERT(!OPTION_C2)
        BYTE i_bit=BYTE(1 << (irq & 7));
        int i_ab=1-((irq & 8) >> 3);
        if (mfp_reg[MFPR_ISRA+i_ab] & i_bit){ //interrupt in service
          break;  //time to stop looking for pending interrupts
        }
        if (mfp_reg[MFPR_IPRA+i_ab] & i_bit){ //is this interrupt pending?
          if (mfp_reg[MFPR_IMRA+i_ab] & i_bit){ //is it not masked out?
            mfp_interrupt(irq); //then cause interrupt
            break;        //lower priority interrupts not allowed now.
          }
        }
      }//nxt irq
    }//ioaccess delay

#else //Steem 3.2
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
#if defined(SSE_INT_MFP)
            mfp_interrupt(irq); //then cause interrupt
#else
            mfp_interrupt(irq,ABSOLUTE_CPU_TIME); //then cause interrupt
#endif
            break;        //lower priority interrupts not allowed now.
          }
        }
      }
    }
#endif

    if (vbl_pending){ //SS IPL4
      if ((sr & SR_IPL)<SR_IPL_4){
        VBL_INTERRUPT
#if defined(SSE_GLUE)
        if(Glue.Status.hbi_done)
          hbl_pending=false;
#endif
      }
    }

    if (hbl_pending
#if defined(SSE_GLUE)
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
#if defined(SSE_DEBUG) 
        // can happen quite a lot
        else if(LPEEK(0x0068)<0xFC0000) TRACE_LOG("no hbl %X\n",LPEEK(0x0068));
#endif
      }
    }
  }
  prepare_event_again();
}


#if defined(SSE_INT_HBL) 

void HBLInterrupt() {
  ASSERT(hbl_pending);
  hbl_pending=false;
  log_to_section(LOGSECTION_INTERRUPTS,Str("INTERRUPT: HBL at PC=")+HEXSl(pc,6)+" "+scanline_cycle_log());

#ifdef SSE_DEBUG
  Debug.nHbis++; // knowing how many in the frame is interesting
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("HBI");
#endif
#if defined(SSE_BOILER_FRAME_INTERRUPTS)
  Debug.FrameInterrupts|=2;
#endif
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
    FrameEvents.Add(scan_y,LINECYCLES,'I',0x20);
#endif
  TRACE_LOG("%d %d %d (%d) HBI #%d Vec %X\n",TIMING_INFO,ACT,Debug.nHbis,LPEEK(0x0068));

#if defined(SSE_BOILER_IRQ_IN_HISTORY) 
  pc_history_y[pc_history_idx]=scan_y;
  pc_history_c[pc_history_idx]=LINECYCLES;
  pc_history[pc_history_idx++]=0x99000001+(2<<16); 
  if (pc_history_idx>=HISTORY_SIZE) 
    pc_history_idx=0;
#endif


#endif//dbg

  if (cpu_stopped)
    M68K_UNSTOP;

#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::EXCEPTION;
#endif


  // wobble?
#if defined(SSE_INT_E_CLOCK)
  if(!OPTION_C1) // not if mode "E-Clock"
#endif
  {
    INTERRUPT_START_TIME_WOBBLE; //Steem 3.2
  }

#if defined(SSE_INT_HBL_IACK2)
  time_of_last_hbl_interrupt=ABSOLUTE_CPU_TIME; //before e-clock cycles
#endif

  // E-clock?
#if defined(SSE_INT_E_CLOCK)
  if(OPTION_C1)
  {
    COUNTER_VAR current_cycles=ACT;
    INSTRUCTION_TIME(ECLOCK_AUTOVECTOR_CYCLE);
    BYTE e_clock_wait_states=M68000.SyncEClock(TM68000::ECLOCK_HBL);
    INSTRUCTION_TIME(current_cycles-ACT);
    INSTRUCTION_TIME(e_clock_wait_states);
  }
#endif

#if defined(SSE_GLUE)
  Glue.Status.hbi_done=true;
#endif
#if defined(SSE_MMU_ROUNDING_BUS)
  m68kInterruptTiming();
#else
  INSTRUCTION_TIME_ROUND(SSE_INT_HBL_TIMING); 
#endif
  m68k_interrupt(LPEEK(0x0068));       
  // set CPU registers
  sr=(sr & (WORD)(~SR_IPL)) | (WORD)(SR_IPL_2);
  debug_check_break_on_irq(BREAK_IRQ_HBL_IDX); 

#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::NORMAL;
#endif

}

#endif

#if defined(SSE_INT_VBL) 

void VBLInterrupt() {

  ASSERT( vbl_pending );
  vbl_pending=false; 

#ifdef SSE_DEBUG
#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.RecordInterrupt("VBI");
#endif
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
    FrameEvents.Add(scan_y,LINECYCLES,'I',0x40);
#endif
#if defined(SSE_BOILER_FRAME_INTERRUPTS)
  Debug.FrameInterrupts|=1;
#endif
  log_to_section(LOGSECTION_INTERRUPTS,EasyStr("INTERRUPT: VBL at PC=")+HEXSl(pc,6)+" time is "+ABSOLUTE_CPU_TIME+" ("+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)+" cycles into screen)");
  TRACE_LOG("%d %d %d (%d) VBI Vec %X\n",TIMING_INFO,ACT,LPEEK(0x0070));

#if defined(SSE_BOILER_IRQ_IN_HISTORY) 
  pc_history_y[pc_history_idx]=scan_y;
  pc_history_c[pc_history_idx]=LINECYCLES;
  pc_history[pc_history_idx++]=0x99000001+(4<<16); 
  if (pc_history_idx>=HISTORY_SIZE) 
    pc_history_idx=0;
#endif

#endif//dbg

  if (cpu_stopped)
    M68K_UNSTOP;

#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::EXCEPTION;
#endif

  // wobble?
#if defined(SSE_INT_E_CLOCK)
  if(!OPTION_C1) // no jitter no wobble if "E-clock"
#endif
  {
    INTERRUPT_START_TIME_WOBBLE; //wobble for STE
  }

#if defined(SSE_INT_VBL_IACK2)
  time_of_last_vbl_interrupt=ABSOLUTE_CPU_TIME;
#endif

    // E-Clock?
#if defined(SSE_CPU_E_CLOCK)
  if(OPTION_C1)
  { 
    COUNTER_VAR current_cycles=ACT;
    INSTRUCTION_TIME(ECLOCK_AUTOVECTOR_CYCLE);
    BYTE e_clock_wait_states=M68000.SyncEClock(TM68000::ECLOCK_VBL);
    INSTRUCTION_TIME(current_cycles-ACT);
    INSTRUCTION_TIME(e_clock_wait_states);
  }
#endif
#if defined(SSE_MMU_ROUNDING_BUS)
  m68kInterruptTiming();
#else
  INSTRUCTION_TIME_ROUND(SSE_INT_VBL_TIMING);
#endif
  m68k_interrupt(LPEEK(0x0070));

  sr=(sr& (WORD)(~SR_IPL))|(WORD)(SR_IPL_4);

  debug_check_break_on_irq(BREAK_IRQ_VBL_IDX);
  ASSERT(!vbl_pending);

#if defined(SSE_CPU_392B)
  M68000.ProcessingState=TM68000::NORMAL;
#endif


}

#endif

#undef LOGSECTION

#endif//#if defined(SSE_INTERRUPT)