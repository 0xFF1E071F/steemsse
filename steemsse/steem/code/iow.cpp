/*---------------------------------------------------------------------------
FILE: iow.cpp
MODULE: emu
DESCRIPTION: I/O address writes. This file contains crucial core functions
that deal with writes to ST I/O addresses ($ff8000 onwards), this is the only
route of communication between programs and the chips in the emulated ST.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE)
#pragma message("Included for compilation: iow.cpp")
#endif

#define LOGSECTION LOGSECTION_IO
/*
  Secret addresses:
    poke byte into FFC123 - stops program running
    poke long into FFC1F0 - logs the string at the specified memory address,
                            which must be null-terminated
*/
//---------------------------------------------------------------------------
void ASMCALL io_write_b(MEM_ADDRESS addr,BYTE io_src_b)
{
/*
  Allowed IO writes (OR 0)

  FF8000 - FF800F MMU
  FF8200 - FF820F SHIFTER
  FF8240 - FF827F pallette, res
  FF8600 - FF860F FDC
  FF8800 - FF88FF sound chip
  FF8900 - FF893F DMA sound, Microwire
  FF8A00 - FF8A3F blitter

  FF9000
  FF9001
  FF9202  paddles
  FF9203  paddles
  FFFA01, odd numbers up to FFFA3F MFP
  FFFC00 - FFFCFF  ACIA, realtime clock
  FFFD00 - FFFDFF
*/

  log_io_write(addr,io_src_b);

#ifdef DEBUG_BUILD
  DEBUG_CHECK_WRITE_IO_B(addr,io_src_b);
#endif

#ifdef ONEGAME
  if (addr>=OG_TEXT_ADDRESS && addr<=OG_TEXT_ADDRESS+OG_TEXT_LEN){
    if (addr==(OG_TEXT_ADDRESS+OG_TEXT_LEN)){
      OGSetRestart();
      return;
    }
    OG_TextMem[addr-OG_TEXT_ADDRESS]=(char)io_src_b;
    return;
  }
#endif

  
  switch (addr & 0xffff00){   //0xfffe00 SS: big switch for all byte writes
    case 0xfffc00:{  //--------------------------------------- ACIAs
/*
          MC6850

          ff fc00                   |xxxxxxxx|   Keyboard ACIA Control
          ff fc02                   |xxxxxxxx|   Keyboard ACIA Data

          ff fc04                   |xxxxxxxx|   MIDI ACIA Control
          ff fc06                   |xxxxxxxx|   MIDI ACIA Data

*/
      // Only cause bus jam once per word
      DEBUG_ONLY( if (mode==STEM_MODE_CPU) )
      {
        if (io_word_access==0 || (addr & 1)==0){
//          if (passed VBL or HBL point){ //SS: those // are not mine
//            BUS_JAM_TIME(4);
//          }else{
//          int waitTable[10]={0,9,8,7,6,5,4,3,2,1};
//          BUS_JAM_TIME(waitTable[ABSOLUTE_CPU_TIME % 10]+6);


#if defined(STEVEN_SEAGAL) && defined(SS_ACIA_BUS_JAM_NO_WOBBLE)
          const int rel_cycle=0; // hoping it will be trashed by compiler
#else // Steem 3.2, 3.3

//          if (passed VBL or HBL point){//SS: those // are not mine
//            BUS_JAM_TIME(4);
//          }else{
          // Jorge Cwik:
          // Access to the ACIA is synchronized to the E signal. Which is a clock with
          // one tenth the frequency of the main CPU clock (800 Khz). So the timing
          // should depend on the phase relationship between both clocks.

          int rel_cycle=ABSOLUTE_CPU_TIME-shifter_cycle_base;
#if defined(STEVEN_SEAGAL) && defined(SS_MFP_RATIO)
          rel_cycle=CpuNormalHz-rel_cycle;
#else
          rel_cycle=8000000-rel_cycle;
#endif
          rel_cycle%=10;
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VID_SHIFTER_EVENTS)
          VideoEvents.Add(scan_y,LINECYCLES,'J',rel_cycle+6); 
#endif
          BUS_JAM_TIME(rel_cycle+6); // just 6

        }
      }

      switch (addr){
    /******************** Keyboard ACIA ************************/

      case 0xfffc00:  //control
        if ((io_src_b & 3)==3){
          log_to(LOGSECTION_IKBD,Str("IKBD: ")+HEXSl(old_pc,6)+" - ACIA reset"); 
#if defined(STEVEN_SEAGAL) && defined(SS_ACIA_TRACE)
          TRACE("IKBD - ACIA IKBD master reset\n");
#endif
          ACIA_Reset(NUM_ACIA_IKBD,0);
        }else{
#if defined(STEVEN_SEAGAL) && defined(SS_ACIA_TRACE)
          TRACE("PC %X ACIA IKBD write CR %X\n",pc,io_src_b); // rare?
#endif
          ACIA_SetControl(NUM_ACIA_IKBD,io_src_b);
        }
        break;

      case 0xfffc02:  //data
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD) 
#if defined(SS_IKBD_TRACE_6301)
        TRACE("%d PC %X IKBD write 0xfffc02 %X hd6301_receiving_from_MC6850 %d\n",ABSOLUTE_CPU_TIME,pc,io_src_b,hd6301_receiving_from_MC6850);
#endif
        {
          bool TXEmptyAgenda=(agenda_get_queue_pos(agenda_acia_tx_delay_IKBD)>=0);
          if(TXEmptyAgenda==0)
          {
            if(ACIA_IKBD.tx_irq_enabled)
            {
              ACIA_IKBD.irq=false;
              mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
            }
#if defined(SS_IKBD_6301)
            // it's taken care of in agenda_ikbd_process() (double buffer)
#if defined(SS_ACIA_DOUBLE_BUFFER_TX)
            if(!HD6301EMU_ON)
#endif
              agenda_add(agenda_acia_tx_delay_IKBD,IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESS_ALT /*ACIAClockToHBLS(ACIA_IKBD.clock_divide)*/,0);
#else
            agenda_add(agenda_acia_tx_delay_IKBD,IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESS_ALT /*ACIAClockToHBLS(ACIA_IKBD.clock_divide)*/,0);
#endif
          }
#if defined(SS_IKBD_6301)
          // If the line is free, the byte in register will be sent very soon 
          // and the bit cleared very soon (double buffer)
#if defined(SS_ACIA_DOUBLE_BUFFER_TX)
          if(hd6301_receiving_from_MC6850||!HD6301EMU_ON)  
#endif
#endif
            ACIA_IKBD.tx_flag=true; //flag for transmitting
          // If send new byte before last one has finished being sent
          if(
#if defined(SS_IKBD_6301) && !defined(SS_ACIA_DOUBLE_BUFFER_TX)
            !HD6301EMU_ON&& // we don't do that TODO otherwise? seems incorrect
#endif
            abs(ABSOLUTE_CPU_TIME-ACIA_IKBD.last_tx_write_time)<ACIA_CYCLES_NEEDED_TO_START_TX)//512
          {
            // replace old byte with new one
            int n=agenda_get_queue_pos(agenda_ikbd_process);
            if(n>=0)
            {
              log_to(LOGSECTION_IKBD,Str("IKBD: ")+HEXSl(old_pc,6)+" - Received new command before old one was sent, replacing "+
                                      HEXSl(agenda[n].param,2)+" with "+HEXSl(io_src_b,2));
              agenda[n].param=io_src_b;
#if defined(SS_IKBD_TRACE)
              TRACE("IKBD replace byte %X with %X\n",agenda[n].param,io_src_b);
#endif
            }
          }

#if defined(SS_IKBD_6301) && defined(SS_ACIA_DOUBLE_BUFFER_TX)
          // we'll place in agenda when the first byte is handled
          else if(hd6301_receiving_from_MC6850)
          {
#if defined(SS_IKBD_TRACE_6301)
            TRACE("%d PC %X IKBD write (shift delayed) %X\n",ACIA_IKBD.last_tx_write_time,pc,io_src_b);
            printf("%d PC %X IKBD write (shift delayed) %X\n",ACIA_IKBD.last_tx_write_time,pc,io_src_b);
#endif
            ACIA_IKBD.data_tdr=io_src_b;
          }
#endif

          else
          {
            // there is a delay before the data gets to the IKBD
            ACIA_IKBD.last_tx_write_time=ABSOLUTE_CPU_TIME;
#if defined(SS_IKBD_TRACE_IO)
            TRACE("iow IKBD write %X (act %d PC %X tx%d)\n",io_src_b,ABSOLUTE_CPU_TIME,pc,ACIA_IKBD.tx_flag);
            if(HD6301EMU_ON)
              printf("iow IKBD write %X (act %d PC %X tx%d)\n",io_src_b,ABSOLUTE_CPU_TIME,pc,ACIA_IKBD.tx_flag);
#endif
            agenda_add(agenda_ikbd_process,
#if defined(SS_IKBD_6301)
              HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL,
#else
             IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESS_ALT,
#endif
             io_src_b);
          }
        }

#if defined(SS_IKBD_6301)
        if(HD6301EMU_ON)
          hd6301_receiving_from_MC6850=1; // line is busy
#endif

        break;
#else // Steem 3.2
      {
        bool TXEmptyAgenda=(agenda_get_queue_pos(agenda_acia_tx_delay_IKBD)>=0);
        if (TXEmptyAgenda==0){
          if (ACIA_IKBD.tx_irq_enabled){
            ACIA_IKBD.irq=false;
            mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
          }
          agenda_add(agenda_acia_tx_delay_IKBD,2 /*ACIAClockToHBLS(ACIA_IKBD.clock_divide)*/,0);
        }
        ACIA_IKBD.tx_flag=true; //flag for transmitting
        // If send new byte before last one has finished being sent
        if (abs(ABSOLUTE_CPU_TIME-ACIA_IKBD.last_tx_write_time)<ACIA_CYCLES_NEEDED_TO_START_TX){
          // replace old byte with new one
          int n=agenda_get_queue_pos(agenda_ikbd_process);
          if (n>=0){
            log_to(LOGSECTION_IKBD,Str("IKBD: ")+HEXSl(old_pc,6)+" - Received new command before old one was sent, replacing "+
                                      HEXSl(agenda[n].param,2)+" with "+HEXSl(io_src_b,2));
            agenda[n].param=io_src_b;
          }
        }else{
          // there is a delay before the data gets to the IKBD
          ACIA_IKBD.last_tx_write_time=ABSOLUTE_CPU_TIME;
          agenda_add(agenda_ikbd_process,IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESS,io_src_b);
        }
        break;
      }
#endif

    /******************** MIDI ACIA *********************************/
/*

               The Musical Instrument Digital Interface (MIDI)  allows
          the  integration of the ST with music synthesizers, sequenc-
          ers, drum boxes, and other devices  possessing  MIDI  inter-
          faces.   High  speed  (31.25  Kbaud) serial communication of
          keyboard and program information is provided by  two  ports,
          MIDI  OUT  and  MIDI IN (MIDI OUT also supports the optional
          MIDI THRU port).

               The MIDI bus permits up to 16 channels in one of  three
          network  addressing modes:  Omni (all units addressed simul-
          taneously, power up  default),  Poly  (each  unit  addressed
          separately),   and   Mono   (each   unit   voice   addressed
          separately).  Information is communicated via five types  of
          data  format  (data  bytes, most significant bit:  status 1,
          data 0) which are prioritized from  highest  to  lowest  as:
          System  Reset  (default  conditions,  should  not be sent on
          power up to avoid deadlock), System Exclusive  (manufacturer
          unique  data:   Sequential  Circuits,  Kawai,  Roland, Korg,
          Yamaha), System Real Time (synchronization),  System  Common
          (broadcast),  and  Channel  (note  selections, program data,
          etc).

               The ST MIDI interface provides current  loop  asynchro-
          nous  serial communication controlled by an MC6850 ACIA sup-
          plied with transmit and receive clock  inputs  of  500  KHz.
          The  data  transfer rate is a constant 31.25 Kbaud which can
          be generated by setting the ACIA Counter  Divide  Select  to
          divide  by 16.  The MIDI specification calls for serial data
          to consist of eight data bits preceded by a  start  bit  and
          followed by one stop bit.


          ----- MIDI Port Pin Assignments ---------------

             MIDI OUT/THRU
             ST           Circular DIN 5S
                          ----                                    ----
          MIDI IN           1 |---- THRU Transmit Data --------->|
                            2 |---- Shield Ground ---------------|
                            3 |<--- THRU Loop Return ------------|
          MIDI ACIA         4 |---- OUT Transmit Data ---------->|
                            5 |<--- OUT Loop Return -------------|
                          ----                                    ----

             MIDI IN
             ST           Circular DIN 5S
                          ----                                    ----
          MIDI ACIA         4 |<--- IN Receive Data -------------|
                            5 |---- IN Loop Return ------------->|
                          ----                                    ----



          Signal Characteristics

                  current loop            5 ma, zero is current on.



*/
      case 0xfffc04:  //control
        if ((io_src_b & 3)==3){ // Reset
          log_to(LOGSECTION_IKBD,Str("MIDI: ")+HEXSl(old_pc,6)+" - ACIA reset");
          ACIA_Reset(NUM_ACIA_MIDI,0);
        }else{
          ACIA_SetControl(NUM_ACIA_MIDI,io_src_b);
        }
        break;
      case 0xfffc06:  //data
      {
        bool TXEmptyAgenda=(agenda_get_queue_pos(agenda_acia_tx_delay_MIDI)>=0);
        if (TXEmptyAgenda==0){
          if (ACIA_MIDI.tx_irq_enabled){
            ACIA_MIDI.irq=false;
            mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
          }
          agenda_add(agenda_acia_tx_delay_MIDI,2 /*ACIAClockToHBLS(ACIA_MIDI.clock_divide)*/,0);
        }
        ACIA_MIDI.tx_flag=true;  //flag for transmitting
        MIDIPort.OutputByte(io_src_b);
        break;
      }
    //-------------------------- unrecognised -------------------------------------------------
      default:
        break;  //all writes allowed
      }
    }
    break;

    case 0xfffa00:  //--------------------------------------- MFP
    {
/*
          MC68xxx

          ff fa01                   |xxxxxxxx|   MFP General Purpose I/O
          ff fa03                   |xxxxxxxx|   MFP Active Edge
          ff fa05                   |xxxxxxxx|   MFP Data Direction
          ff fa07                   |xxxxxxxx|   MFP Interrupt Enable A
          ff fa09                   |xxxxxxxx|   MFP Interrupt Enable B
          ff fa0b                   |xxxxxxxx|   MFP Interrupt Pending A
          ff fa0d                   |xxxxxxxx|   MFP Interrupt Pending B
          ff fa0f                   |xxxxxxxx|   MFP Interrupt In-Service A
          ff fa11                   |xxxxxxxx|   MFP Interrupt In-Service B
          ff fa13                   |xxxxxxxx|   MFP Interrupt Mask A
          ff fa15                   |xxxxxxxx|   MFP Interrupt Mask B
          ff fa17                   |xxxxxxxx|   MFP Vector
          ff fa19                   |xxxxxxxx|   MFP Timer A Control
          ff fa1b                   |xxxxxxxx|   MFP Timer B Control
          ff fa1d                   |xxxxxxxx|   MFP Timers C and D Control
          ff fa1f                   |xxxxxxxx|   MFP Timer A Data
          ff fa21                   |xxxxxxxx|   MFP Timer B Data
          ff fa23                   |xxxxxxxx|   MFP Timer C Data
          ff fa25                   |xxxxxxxx|   MFP Timer D Data
          ff fa27                   |xxxxxxxx|   MFP Sync Character
          ff fa29                   |xxxxxxxx|   MFP USART Control
          ff fa2b                   |xxxxxxxx|   MFP Receiver Status
          ff fa2d                   |xxxxxxxx|   MFP Transmitter Status
          ff fa2f                   |xxxxxxxx|   MFP USART Data
*/
      if (addr<0xfffa40){
        // Only cause bus jam once per word
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) if (io_word_access==0 || (addr & 1)==1) BUS_JAM_TIME(4);

        if (addr & 1){
          if (addr<0xfffa30){
            int old_ioaccess=ioaccess;
            int n=(addr-0xfffa01) >> 1;
            if (n==MFPR_GPIP || n==MFPR_AER || n==MFPR_DDR){
              // The output from the AER is eored with the GPIP/input buffer state
              // and that input goes into a 1-0 transition detector. So if the result
              // used to be 1 and now it is 0 an interrupt will occur (if the
              // interrupt is enabled of course).
              BYTE old_gpip=BYTE(mfp_reg[MFPR_GPIP] & ~mfp_reg[MFPR_DDR]);
              old_gpip|=BYTE(mfp_gpip_input_buffer & mfp_reg[MFPR_DDR]);
              BYTE old_aer=mfp_reg[MFPR_AER];

              if (n==MFPR_GPIP){  // Write to GPIP (can only change bits set to 1 in DDR)
                io_src_b&=mfp_reg[MFPR_DDR];
                // Don't change the bits that are 0 in the DDR
                io_src_b|=BYTE(mfp_gpip_input_buffer & ~mfp_reg[MFPR_DDR]);
                mfp_gpip_input_buffer=io_src_b;
              }else{
                mfp_reg[n]=io_src_b;
              }
              BYTE new_gpip=BYTE(mfp_reg[MFPR_GPIP] & ~mfp_reg[MFPR_DDR]);
              new_gpip|=BYTE(mfp_gpip_input_buffer & mfp_reg[MFPR_DDR]);
              BYTE new_aer=mfp_reg[MFPR_AER];

              for (int bit=0;bit<8;bit++){
                int irq=mfp_gpip_irq[bit];
                if (mfp_interrupt_enabled[irq]){
                  BYTE mask=BYTE(1 << bit);
                  bool old_1_to_0_detector_input=((old_gpip & mask) ^ (old_aer & mask))==mask;
                  bool new_1_to_0_detector_input=((new_gpip & mask) ^ (new_aer & mask))==mask;
                  if (old_1_to_0_detector_input && new_1_to_0_detector_input==0){
                    // Transition the right way! Set pending (interrupts happen later)
                    // Don't need to call set_pending routine here as this can never
                    // happen soon after an interrupt
                    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq);
                  }
                }
              }
            }else if (n>=MFPR_IERA && n<=MFPR_IERB){ //enable
              // See if timers have timed out before write to enabled. This is needed
              // because MFP_CALC_INTERRUPTS_ENABLED religns the timers (so they would
              // not cause a timeout if they are overdue at this point)
              // Update v2.5: I don't think this will happen, when you write to this
              // register the MFP will turn of the interrupt line straight away.
//              mfp_check_for_timer_timeouts();

              ///// Update v2.7, why does calc interrupts enabled need to realign the timers?
              //  It has been removed, so don't need this for anything

              mfp_reg[n]=io_src_b;
              MFP_CALC_INTERRUPTS_ENABLED;
              for (n=0;n<4;n++){
                bool new_enabled=(mfp_interrupt_enabled[mfp_timer_irq[n]] && (mfp_get_timer_control_register(n) & 7));
                if (new_enabled && mfp_timer_enabled[n]==0){
                  // Timer should have been running but isn't, must put into future
                  int stage=(mfp_timer_timeout[n]-ABSOLUTE_CPU_TIME);
                  if (stage<=0){
                    stage+=((-stage/mfp_timer_period[n])+1)*mfp_timer_period[n];
                  }else{
                    stage%=mfp_timer_period[n];
                  }
                  mfp_timer_timeout[n]=ABSOLUTE_CPU_TIME+stage;
                }

                LOG_ONLY( if (new_enabled!=mfp_timer_enabled[n]) log_to(LOGSECTION_MFP_TIMERS,Str("MFP: ")+HEXSl(old_pc,6)+
                                                  " - Timer "+char('A'+n)+" enabled="+new_enabled); )
                mfp_timer_enabled[n]=new_enabled;
              }

              /*
                Disabling an interrupt channel has no
                effect on the corresponding bit in Interrupt In-Service
                Registers (ISRA, ISRB) ; thus, if the In-service
                Registers are used and an interrupt is in service on
                that channel when the channel is disabled, it will remain
                in service until cleared in the normal manner.

                mfp_reg[MFPR_ISRA]&=mfp_reg[MFPR_IERA]; //no in service on disabled registers
                mfp_reg[MFPR_ISRB]&=mfp_reg[MFPR_IERB]; //no in service on disabled registers
              */

              /*
                and any pending interrupt on that channel will be cleared by disabling
                that channel.
              */
              mfp_reg[MFPR_IPRA]&=mfp_reg[MFPR_IERA]; //no pending on disabled registers
              mfp_reg[MFPR_IPRB]&=mfp_reg[MFPR_IERB]; //no pending on disabled registers
            }else if (n>=MFPR_IPRA && n<=MFPR_ISRB){ //can only clear bits in IPR, ISR
              mfp_reg[n]&=io_src_b;
            }else if (n>=MFPR_TADR && n<=MFPR_TDDR){ //have to set counter as well as data register
              mfp_set_timer_reg(n,mfp_reg[n],io_src_b);
              mfp_reg[n]=io_src_b;
            }else if (n==MFPR_TACR || n==MFPR_TBCR){ //wipe low-bit on set
              io_src_b &= BYTE(0xf);
              mfp_set_timer_reg(n,mfp_reg[n],io_src_b);
              mfp_reg[n]=io_src_b;
            }else if (n==MFPR_TCDCR){
              io_src_b&=BYTE(b01110111);
              mfp_set_timer_reg(n,mfp_reg[n],io_src_b);
              mfp_reg[n]=io_src_b;
            }else if (n==MFPR_VR){
              mfp_reg[MFPR_VR]=io_src_b;
              if (!MFP_S_BIT){
                mfp_reg[MFPR_ISRA]=0;
                mfp_reg[MFPR_ISRB]=0;
              }
            }else if (n>=MFPR_SCR){
              RS232_WriteReg(n,io_src_b);
            }else{
              ASSERT(n!=16);
              mfp_reg[n]=io_src_b;
            }
            // The MFP doesn't update for about 8 cycles, so we should execute the next
            // instruction before causing any interrupts
            ioaccess=old_ioaccess;
            if ((ioaccess & (IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE | IOACCESS_FLAG_FOR_CHECK_INTRS |
                                IOACCESS_FLAG_DELAY_MFP))==0){
              ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE;
            }
          }
        }else{ // even
          // Byte access causes bus error
#if defined(STEVEN_SEAGAL)
          if (io_word_access==0) 
          {
#if defined(SS_INT_TRACE)
            TRACE("MFP even byte access\n");
#endif
            exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
          }
#else
          if (io_word_access==0) exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
#endif
        }
      }else{ // beyond allowed range
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }
      break;
    }

    case 0xff9800:        // Falcon 256 colour palette
    case 0xff9900:        // Falcon 256 colour palette
    case 0xff9a00:        // Falcon 256 colour palette
    case 0xff9b00:        // Falcon 256 colour palette
      TRACE("W Falcon 256 colour palette %x\n",addr);
      if (emudetect_called){
        int n=(addr-0xff9800)/4;
        DWORD val=emudetect_falcon_stpal[n];
        DWORD_B(&val,addr & 3)=BYTE(io_src_b & ~1);
        emudetect_falcon_stpal[n]=val;
        emudetect_falcon_palette_convert(n);
        return; // No exception
      }
      exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      break;

    case 0xff8a00:      //----------------------------------- Blitter
#if defined(STEVEN_SEAGAL) && defined(SS_VID_SHIFTER_EVENTS)
      VideoEvents.Add(scan_y,LINECYCLES,'B',((addr-0xff8a00)<<8)|io_src_b);
#endif
      Blitter_IO_WriteB(addr,io_src_b);
      break;

    case 0xff8900:      //----------------------------------- STE DMA Sound

#if defined(STEVEN_SEAGAL) && defined(SS_STF)
      if(ST_TYPE!=STE)
      {
#if defined(SS_STF_TRACE)
        TRACE("STF illegal write to 0xff8900\n");
#endif
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr); // fixes PYM/ST-CNX, SoWatt, etc.
        break;
      }
#endif
      if (addr>0xff893f){ //illegal range
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }else{
        switch (addr){
/*
  DMA-Sound Control Register:

    $FFFF8900  0 0 0 0 0 0 0 0 0 0 0 0 0 0 X X
*/
          case 0xff8900:   //Nowt
            break;
          case 0xff8901:   //DMA control register
            dma_sound_set_control(io_src_b);
            break;
/*
  DMA-Sound Start Address Register:

    $FFFF8902  0 0 X X X X X X X X X X X X X X   Hibyte

    $FFFF8904  X X X X X X X X X X X X X X X X   Midbyte

    $FFFF8906  X X X X X X X X X X X X X X X 0   Lowbyte

These three registers contain the 24-bit address of the sample to play. 
Even though the samples are built on a byte-base, the DMA chip also only
 allows even addresses
*/
          case 0xff8903:   //HiByte of frame start address
          case 0xff8905:   //MidByte of frame start address
          case 0xff8907:   //LoByte of frame start address
            switch (addr & 0xf){
              case 0x3:
/*
 The DMA-Soundsystem expects you to write the high-byte of the Start- and
 Endaddress first. Even though this serves no purpose at all, writing the 
 highbyte clears the others. Hence it must be written first.
 [Isn't it false? Sounds wrong when we do that]
*/
#if defined(STEVEN_SEAGAL) && defined(SS_SOUND__)
                next_dma_sound_start=0;
#else
                next_dma_sound_start&=0x00ffff;
#endif
                next_dma_sound_start|=io_src_b << 16;break;
              case 0x5:next_dma_sound_start&=0xff00ff;next_dma_sound_start|=io_src_b << 8;break;
              case 0x7:
#if defined(STEVEN_SEAGAL) && defined(SS_SOUND)
                //ASSERT(!(io_src_b&1)); // MOLZ
                io_src_b&=0xFE;
#endif
                next_dma_sound_start&=0xffff00;next_dma_sound_start|=io_src_b;break;
            }
            if ((dma_sound_control & BIT_0)==0){
              dma_sound_start=next_dma_sound_start;
              dma_sound_fetch_address=dma_sound_start;
            }
            log_to(LOGSECTION_SOUND,EasyStr("SOUND: ")+HEXSl(old_pc,6)+" - DMA sound start address set to "+HEXSl(next_dma_sound_start,6));
            break;
/*
  DMA-Sound End Register:

    $FFFF890E  0 0 X X X X X X X X X X X X X X   Hibyte

    $FFFF8910  X X X X X X X X X X X X X X X X   Midbyte

    $FFFF8912  X X X X X X X X X X X X X X X 0   Lowbyte 

The address that the sample ends at. When the count registers have reached 
this address, the DMA-sound system will either stop or loop.
*/
          case 0xff890f:   //HiByte of frame end address
          case 0xff8911:   //MidByte of frame end address
          case 0xff8913:   //LoByte of frame end address
            switch (addr & 0xf){
              case 0xf:
#if defined(STEVEN_SEAGAL) && defined(SS_STE_SND__)
                next_dma_sound_end=0; // like 0xff8903, also sounds wrong
#else
                next_dma_sound_end&=0x00ffff;
#endif
                next_dma_sound_end|=io_src_b << 16;break;
              case 0x1:next_dma_sound_end&=0xff00ff;next_dma_sound_end|=io_src_b << 8;break;
              case 0x3:
#if defined(STEVEN_SEAGAL) && defined(SS_SOUND)
                //ASSERT(!(io_src_b&1));// MOLZ
                io_src_b&=0xFE;
#endif
                
                next_dma_sound_end&=0xffff00;next_dma_sound_end|=io_src_b;break;
            }
            if ((dma_sound_control & BIT_0)==0) dma_sound_end=next_dma_sound_end;
            log_to(LOGSECTION_SOUND,EasyStr("SOUND: ")+HEXSl(old_pc,6)+" - DMA sound end address set to "+HEXSl(next_dma_sound_end,6));
            break;
          case 0xff8921:   //Sound mode control
            dma_sound_set_mode(io_src_b);
            break;

/*
  Adress and Data register

    $FFFF8922  x x x x  x x x x  x x x x  x x x x

This address is being used to feed the National LMC both address and data
 bits for a certain setting. The choice which bits are being read are being
 described in the mask register at $FFFF8924. As described above, the first 
 two bits of the 11 bit package need to be a "10" to address the LMC1992. 
*/
          case 0xff8922: // Set high byte of MicroWire_Data
            MicroWire_Data=MAKEWORD(LOBYTE(MicroWire_Data),io_src_b);
            break;
          case 0xff8923: // Set low byte of MicroWire_Data
          {
            MicroWire_Data=MAKEWORD(io_src_b,HIBYTE(MicroWire_Data));
            MicroWire_StartTime=ABSOLUTE_CPU_TIME;
            int dat=MicroWire_Data & MicroWire_Mask;
            int b;
            for (b=15;b>=10;b--){
              if (MicroWire_Mask & (1 << b)){
                if ((dat & (1 << b)) && (dat & (1 << (b-1)))==0){  //DMA Sound Address
                  int dat_b=b-2;
                  for (;dat_b>=8;dat_b--){ // Find start of data
                    if (MicroWire_Mask & (1 << dat_b)) break;
                  }
                  dat >>= dat_b-8; // Move 9 highest bits of data to the start
/*
Then there are 3 more "address" and 6 more data bits. The address bits are 
 3 in total and are being used as follows:

      0 1 1  -  Master Volume (followed by 6 bits of data)      
      1 0 1  -  Left channel volume (followed by 6 bits of data)
      1 0 0  -  Right channel volume (followed by 6 bits of data)
      0 1 0  -  Trebble control (followed by 6 bits of data)
      0 0 1  -  Bass control (followed by 6 bits of data)
      0 0 0  -  Mixer (followed by 6 bits of data).

However, not all bits of the 6 general data bits are being used. It is 
necessary to have a multiple of 6 though since the Microwire is a 3-bit serial
 interface. The explanation of the 6 data bits are (d means necessary data
 bit, x means bit is ignored)

*/
                  int nController=(dat >> 6) & b0111;
                  switch (nController){
                    case b0011: // Master Volume
                    case b0101: // Left Volume
                    case b0100: // Right Volume
                      if (nController==b0011){
/*
0 1 1  -  Master Volume (followed by 6 bits of data)
          Master Volume: d d d  d d d  (all 6 bits used)

                     0 0 0  0 0 0  = -80 db volume
               (20)  0 1 0  1 0 0  = -40 db volume
               (40)  1 0 1  x x x  =   0 db volume (max)

Each increment represents 2 db. If the 3 left bit encode "101", the last 
3 bits are being ignored.
*/
                        // 20 is practically silent!
                        dma_sound_volume=(dat & b00111111);
                        if (dma_sound_volume>47) dma_sound_volume=0; // 47 101111
                        if (dma_sound_volume>40) dma_sound_volume=40;
                      }else{
/*
      Left channel & Right channel
      :  x d d  d d d  (left bit ignored)

                       0 0  0 0 0  = -40 db volume
                 (10)  0 1  0 1 0  = -20 db volume
                 (20)  1 0  1 x x  =   0 db volume (max)

Each increment represents 2 db. If the 3 left bit carry "101", the last 2 
bits are being ignored.
*/
                        int new_val=(dat & b00011111);
                        if (new_val>23) new_val=0;
                        if (new_val>20) new_val=20;
                        if (nController==b0101) dma_sound_l_volume=new_val;
                        if (nController==b0100) dma_sound_r_volume=new_val;
                      }
                      long double lv,rv,mv;
                      lv=dma_sound_l_volume;lv=lv*lv*lv*lv;
                      lv/=(20.0*20.0*20.0*20.0);
                      rv=dma_sound_r_volume;rv=rv*rv*rv*rv;
                      rv/=(20.0*20.0*20.0*20.0);
                      mv=dma_sound_volume;  mv=mv*mv*mv*mv*mv*mv*mv*mv;
                      mv/=(40.0*40.0*40.0*40.0*40.0*40.0*40.0*40.0);
                      // lv rv and mv are numbers between 0 and 1
//SS what is does here is transform the "DB" volume values in a sample
//range??? isn't it gross?
#if defined(STEVEN_SEAGAL) && defined(SS_SOUND_MICROWIRE)
                      dma_sound_l_top_val=128;
                      dma_sound_r_top_val=128;
#else
                      dma_sound_l_top_val=BYTE(128.0*lv*mv);
                      dma_sound_r_top_val=BYTE(128.0*rv*mv);
#endif
#if defined(STEVEN_SEAGAL) && defined(SS_SOUND_MICROWIRE)
#if defined(SS_SOUND_TRACE)
                      TRACE("STE Snd master %X L %X R %X\n",dma_sound_volume,dma_sound_l_volume,dma_sound_r_volume);
                      TRACE("STE Snd vol L %d R %d\n",dma_sound_l_top_val,dma_sound_r_top_val);
#endif
                      //dma_sound_l_top_val=dma_sound_r_top_val=128;
#endif
                      log_to_section(LOGSECTION_SOUND,EasyStr("SOUND: ")+HEXSl(old_pc,6)+" - DMA sound set volume master="+dma_sound_volume+
                                      " l="+dma_sound_l_volume+" r="+dma_sound_r_volume);
                      break;
                    case b0010: // Treble
/*
            Trebble:       x x d  d d d  (left 2 bits are ignored)

                 $0      0  0 0 0  = -12 db (min)
                 $6      0  1 1 0  =   0 db (linear) = no effect
                 $C      1  1 0 0  =  12 db (max)

Each increment represents 2 db, normalized at 15 KHz.
Set at $8D by TOS1.06.
Set at $86 for neutral by Beat Demo, max= min
*/
#if defined(STEVEN_SEAGAL) && defined(SS_SOUND_MICROWIRE)
#if defined(SS_SOUND_TRACE)
                      TRACE("DMA snd Treble $%X\n",io_src_b); 
#endif
                      io_src_b&=0xF;
                      if(io_src_b>0xC)
                        io_src_b=0x6;
                      dma_sound_treble=io_src_b;
                      break;
#endif
                    case b0001: // Base
/*
            Bass:          x x d  d d d  (left 2 bits are ignored)

                 $0      0  0 0 0  = -12 db (min)
                 $6      0  1 1 0  =   0 db (linear) = no effect
                 $C      1  1 0 0  =  12 db (max)

Each increment represents 2 db, normalized at 50 Hz.
Set at D by TOS1.06.
*/
#if defined(STEVEN_SEAGAL) && defined(SS_SOUND_MICROWIRE)
#if defined(SS_SOUND_TRACE)
                      TRACE("DMA snd Bass $%X\n",io_src_b); 
#endif
                      io_src_b&=0xF;
                      if(io_src_b>0xC)
                        io_src_b=0x6;
                      dma_sound_bass=io_src_b;
#endif

                      break;
                    case b0000: // Mixer
/*
            Mixer control: x x x  x d d  (left 4 bits are ignored)

                              0 0  = DMA + (YM2149 - 12 db)
                              0 1  = DMA + YM2149
                              1 0  = DMA only
                              1 1  = reserved

 Setting "00" mixes the output of the YM2149 and the output of the DMA-sound,
 but the YM2149 sound is being downsized by 12 db. "01" mixes DMA and YM2149
 linearly, "10" means DMA sound output only.
*/
#if defined(SS_SOUND_TRACE)
                      TRACE("STE SND mixer %X\n",dat);
#endif
                      ASSERT(dat&3); // Again
                      dma_sound_mixer=dat & b00000011; // 1=PSG too, anything else only DMA
                      log_to_section(LOGSECTION_SOUND,EasyStr("SOUND: ")+HEXSl(old_pc,6)+" - DMA sound mixer is set to "+dma_sound_mixer);
                      break;
                  }
                }
                break;
              }
            }
            break;
          }
/*
       Mask Register

    $FFFF8924  x x x x  x x x x  x x x x  x x x x

This contains in a bitfield which bits of the Address+Data Register are 
explicetely used. Since the Microwire, as it is being used in the STE, requires 
11 bits of data (in general, the Microwire can transport 14 bits), it is
 essential to let the Microwire know WHICH of the 16 bits of this register are
 to be taken into account.As being used in the STE, this register will always 
 feature 11 "1"s and 5 "0"s.
*/
          case 0xff8924:  // Set high byte of MicroWire_Mask
            MicroWire_Mask=MAKEWORD(LOBYTE(MicroWire_Mask),io_src_b);
            break;
          case 0xff8925:  // Set low byte of MicroWire_Mask
            MicroWire_Mask=MAKEWORD(io_src_b,HIBYTE(MicroWire_Mask));
            break;
#if defined(SS_SOUND_TRACE)
          case 0xFF8902:
          case 0xFF890E:
          case 0xFF8920:
            ASSERT(!io_src_b);
            break;
          default:
            TRACE("STE SND %X %X\n",addr,io_src_b);
#endif
        }
      }
      break;
/*        STF
          Sound


               The YM-2149 Programmable Sound Generator produces music
          synthesis,  sound effects, and audio feedback (eg alarms and
          key clicks).  With an applied clock input of 2 MHz, the  PSG
          is  capable  of providing a frequency response range between
          30 Hz (audible) and 125 KHz (post-audible).   The  generator
          places  a  minimal  amount  of processing burden on the main
          system (which acts as the sequencer) and has the ability  to
          perform  using  three independent voice channels.  The three
          sound channel outputs are mixed, along with  Audio  In,  and
          sent  to  an external television or monitor speaker (the PSG
          has built in digital to analog converters).

               The sound generator's internal registers  are  accessed
          via  the  PSG  Register  Select Register (write only, reset:
          registers all zeros).  The tone generator registers  control
          a  basic square wave while the noise generator register con-
          trols a frequency modulated square  wave  of  pseudo  random
          pulse  width.   Tones and noise can be mixed over individual
          channels by using the mixer control register.  The amplitude
          registers allow the specification of a fixed amplitude or of
          a variable amplitude when used with the envelope  generator.
          The  envelope  generator  registers  permit  the  entry of a
          skewed attack-decay-sustain-release envelope in the form  of
          a continue-attack-alternate-hold envelope.


          ff 8800   R               |xxxxxxxx|   PSG Read Data
                                     ||||||||       I/O Port B
                                      --------------   Parallel Interface Data
          ff 8800   W               |xxxxxxxx|   PSG Register Select
                                         ||||
                                          -------   Register Number
                                         0000       Channel A Fine Tune
                                         0001       Channel A Coarse Tune
                                         0010       Channel B Fine Tune
                                         0011       Channel B Coarse Tune
                                         0100       Channel C Fine Tune
                                         0101       Channel C Coarse Tune
                                         0110       Noise Generator Control
                                         0111       Mixer Control - I/O Enable
                                         1000       Channel A Amplitude
                                         1001       Channel B Amplitude
                                         1010       Channel C Amplitude
                                         1011       Envelope Period Fine Tune
                                         1100       Envelope Period Coarse Tune
                                         1101       I/O Port A (Output Only)
                                         1111       I/O Port B

          ff 8802   W               |xxxxxxxx|   PSG Write Data
                                     ||||||||       I/O Port A
                                     ||||||| -------   Floppy Side0/_Side1 Select
                                     |||||| --------   Floppy _Drive0 Select
                                     ||||| ---------   Floppy _Drive1 Select
                                     |||| ----------   RS232 Request To Send
                                     ||| -----------   RS232 Data Terminal Ready
                                     || ------------   Centronics _STROBE
                                     | -------------   General Purpose Output
                                      --------------   Reserved
                                     ||||||||       I/O Port B
                                      --------------   Parallel Interface Data

               The ST parallel interface  supports  Centronics  STROBE
          from the YM-2149 PSG for data synchronization and Centronics
          BUSY to the  MK68901  MFP  (ACKNLG  is  not  supported)  for
          handshaking.   Eight  bits  of  read/write  data are handled
          through I/O Port B on the PSG at  a  typical  data  transfer
          rate of 4000 bytes/second.


          ----- Parallel Port Pin Assignments ---------------

             ST           DB 25S
                          ----                                    ----
          PSG I/O A         1 |---- Centronics STROBE ---------->|
          PSG I/O B         2 |<--- Data 0 --------------------->|
          PSG I/O B         3 |<--- Data 1 --------------------->|
          PSG I/O B         4 |<--- Data 2 --------------------->|
          PSG I/O B         5 |<--- Data 3 --------------------->|
          PSG I/O B         6 |<--- Data 4 --------------------->|
          PSG I/O B         7 |<--- Data 5 --------------------->|
          PSG I/O B         8 |<--- Data 6 --------------------->|
          PSG I/O B         9 |<--- Data 7 --------------------->|
          MFP              11 |<--- Centronics BUSY -------------|
                        18-25 |---- Ground ----------------------|
                          ----                                    ----



          Signal Characteristics

                  pin 1                   TTL levels, active low.
                  pins 2-9                TTL levels.
                  pin 11                  TTL levels, active high,
                                          1 Kohm pullup resistor to +5 VDC.



*/
    case 0xff8800:{  //--------------------------------------- sound chip
      if ((ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_W)==0){
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) BUS_JAM_TIME(4);
        ioaccess|=IOACCESS_FLAG_PSG_BUS_JAM_W;
      }
      if ((addr & 1) && io_word_access) break; //odd addresses ignored on word writes

      if ((addr & 2)==0){  //read data / register select
        psg_reg_select=io_src_b;
        if (psg_reg_select<16){
          psg_reg_data=psg_reg[psg_reg_select];
        }else{
          psg_reg_data=0xff;
        }
      }else{  //write data
        if (psg_reg_select>15) return;
        psg_reg_data=io_src_b;

        BYTE old_val=psg_reg[psg_reg_select];
        psg_set_reg(psg_reg_select,old_val,io_src_b);
        psg_reg[psg_reg_select]=io_src_b;

        if (psg_reg_select==PSGR_PORT_A){
#if USE_PASTI
          if (hPasti && pasti_active) pasti->WritePorta(io_src_b,ABSOLUTE_CPU_TIME);
#endif

          SerialPort.SetDTR(io_src_b & BIT_4);
          SerialPort.SetRTS(io_src_b & BIT_3);
          if ((old_val & (BIT_1+BIT_2))!=(io_src_b & (BIT_1+BIT_2))){
#ifdef ENABLE_LOGFILE
            if ((psg_reg[PSGR_PORT_A] & BIT_1)==0){ //drive 0
              log_to_section(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set current drive to A:");
            }else if ((psg_reg[PSGR_PORT_A] & BIT_2)==0){ //drive 1
              log_to_section(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set current drive to B:");
            }else{                             //who knows?
              log_to_section(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Unset current drive - guess A:");
            }
#endif
            // disk_light_off_time can only get this far in the future when using pasti
            if (int(disk_light_off_time)-int(timer) < 1000*10){
              disk_light_off_time=timer+DisableDiskLightAfter;
            }
          }
        }else if (psg_reg_select==PSGR_PORT_B){
          if (ParallelPort.IsOpen()){
            if (ParallelPort.OutputByte(io_src_b)==0){
              log_write("ARRRGGHH: Lost printer character, printer not responding!!!!");
            }
            UpdateCentronicsBusyBit();
          }
        }else if (psg_reg_select==PSGR_MIXER){
          UpdateCentronicsBusyBit();
        }
      }
      break;
    }
/*

               A direct main memory RAM access channel  is  shared  to
          provide  support  for  both low speed (250 to 500 Kbits/sec)
          and high speed (up to 12 Mbits/sec) 8 bit  device  controll-
          ers.   The  base address for the DMA read or write operation
          is loaded into the DMA Base  Address  and  Counter  Register
          (read/write,  reset:  all  zeros).   Since  only one counter
          register and channel is provided, only one DMA operation can
          be executed at a time.

               The actual DMA operation is performed through a 32 byte
          FIFO  programmed  via  the  DMA  Mode Control Register (word
          access write only, reset: not affected) and DMA Sector Count
          Register  (word  access  write only, reset: all zeros).  The
          progress, success, or failure of a DMA operation is reported
          through  the  DMA  Status  Register  (word access read only,
          reset: one) which is cleared by toggling Write/_Read in  the
          DMA Mode Control Register.

               Bus accesses are granted  to  the  DMA  controller  and
          MC68000  MPU  on  an  egalitarian  first  come, first served
          basis.  The access remains in effect until an  operation  is
          complete or until control is otherwise relinquished.

          DMA/Disk

          ff 8600           |----------------|   Reserved
          ff 8602           |----------------|   Reserved

          ff 8604   R/W     |--------xxxxxxxx|   Disk Controller (Word Access)

          ff 8606   R       |-------------xxx|   DMA Status (Word Access)
                                          |||
                                          || ----   _Error Status
                                          | -----   _Sector Count Zero Status
                                           ------   _Data Request Inactive Status
          ff 8606   W       |-------xxxxxxxx-|   DMA Mode Control (Word Access)
                                    ||||||||
                                    ||||||| -----   A0
                                    |||||| ------   A1
                                    ||||| -------   HDC/_FDC Register Select
                                    |||| --------   Sector Count Register Select
                                    |||0            Reserved
                                    || ----------   Disable/_Enable DMA
                                    | -----------   FDC/_HDC
                                     ------------   Write/_Read

          ff 8609   R/W             |xxxxxxxx|   DMA Base and Counter High
          ff 860b   R/W             |xxxxxxxx|   DMA Base and Counter Mid
          ff 860d   R/W             |xxxxxxxx|   DMA Base and Counter Low
  

               The ST floppy disk drive interface is provided  through
          the  DMA  controller  to an on board WD1772 Floppy Disk Con-
          troller.  A total of two daisy chained  floppy  disk  drives
          (drive  0  or 1) can be supported.  Commands are sent to the
          FDC by first writing to the DMA  Mode  Control  Register  to
          select  the  FDC  internal command register and then writing
          the desired one byte command to the Disk  Controller  Regis-
          ter.   The  entire floppy disk DMA read or write sequence is
          as follows:


          o  select floppy drive 0 or 1 (PSG I/O Port A).
          o  select floppy side 0 or 1 (PSG I/O Port A).
          o  load DMA Base Address and Counter Register.
          o  toggle Write/_Read to clear status (DMA Mode Control Register).
          o  select DMA read or write (DMA Mode Control Register).
          o  select DMA Sector Count Register (DMA Mode Control Register).
          o  load DMA Sector Count Register (DMA operation trigger).
          o  select FDC internal command register (DMA Mode Control Register).
          o  issue FDC read or write command (Disk Controller Register).
          o  DMA active until sector count is zero (DMA Status Register,
             do not poll during DMA active).
          o  issue FDC force interrupt command on multi-sector transfers
             except at track boundaries (Disk Controller Register).
          o  check DMA error status (DMA Status Register, nondestructive).



               The detection of floppy disk removal is  not  supported
          in hardware.

               The ST hard  disk  drive  interface  is  also  provided
          through the DMA controller, however the Atari Hard Disk Con-
          troller is off board and is  sent  commands  using  an  ANSI
          X3T9.2  SCSI-like (Small Computer Systems Interface) command
          descriptor block protocol.  The Atari  Hard  Disk  Interface
          (AHDI)  supports  a minimal subset of SCSI commands (Class 0
          OpCodes), which are dispatched using the following fixed six
          byte  Atari  Computer System Interface (ACSI) command packet
          format:


          ----- ACSI Command Descriptor Block ---------------

                  Byte 0  |xxxxxxxx|
                           ||||||||
                           ||| -------- Operation Code
                            ----------- Controller Number
                  Byte 1  |xxxxxxxx|
                           ||||||||
                           |||--------- Block Address High
                            ----------- Device Number
                  Byte 2  |xxxxxxxx|
                           ||||||||
                            ----------- Block Address Mid
                  Byte 3  |xxxxxxxx|
                           ||||||||
                            ----------- Block Address Low
                  Byte 4  |xxxxxxxx|
                           ||||||||
                            ----------- Block Count
                  Byte 5  |xxxxxxxx|
                           ||||||||
                            ----------- Control Byte



               The  following  is  a  summary  of  available   command
          OpCodes:

          ----- AHDI Command Summary Table ---------------

                   ---------- --------------------
                  | OpCode   | Command            |
                   ---------- --------------------
                  | 0x00     | Test Unit Ready    |
                  | 0x05     | Verify Track       |  *
                  | 0x06     | Format Track       |  *
                  | 0x08     | Read               |  *
                  | 0x0a     | Write              |  *
                  | 0x0b     | Seek               |
                  | 0x0d     | Correction Pattern |
                  | 0x15     | Mode Select        |
                  | 0x1a     | Mode Sense         |
                   ---------- --------------------

                   *  multisector transfer with implied seek

          NOTE:  subject to change.



               Commands are issued to the Atari HDC in a manner  simi-
          lar  to that of the FDC, with the major difference being the
          handshaking of a multi-byte command descriptor  block.   The
          entire hard disk DMA read or write sequence is as follows:


          o  load DMA Base Address and Counter Register.
          o  toggle Write/_Read to clear status (DMA Mode Control Register).
          o  select DMA read or write (DMA Mode Control Register).
          o  select DMA Sector Count Register (DMA Mode Control Register).
          o  load DMA Sector Count Register (DMA operation trigger).
          o  select HDC internal command register (DMA Mode Control Register).
          o  issue controller select byte while clearing A0.
          o  set A0 for remaining command bytes.
          o  after last command byte select controller (DMA Mode Control
             Register).
          o  DMA active until sector count is zero (DMA Status Register,
             do not poll during DMA active).
          o  check DMA error status (DMA Status Register, nondestructive).
          o  check HDC status byte and if necessary perform ECC correction
             following a Verify Track or Read Sector command.

               The format of both floppy and hard  disks  contain  512
          byte data sectors.

*/    
    case 0xff8600:{  //--------------------------------------- DMA / FDC
      if (addr>0xff860f){ //past allowed range
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }else{
/* -- Keep this here!
#ifdef ENABLE_LOGFILE
        EasyStr bin_src_b;bin_src_b.SetLength(8);
        itoa(io_src_b,bin_src_b,2);
        EasyStr a=EasyStr("FDC: Writing byte ")+bin_src_b.LPad(8,'0')+" to IO address "+HEXSl(addr,6);
#ifdef DEBUG_BUILD
        iolist_entry *iol=search_iolist(addr);
        if (iol) a+=EasyStr(" (")+(iol->name)+")";
#endif
        log_to_section(LOGSECTION_FDC,a);
#endif
*/
        if (addr<0xff8604) exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
        if (addr<0xff8608 && io_word_access==0) exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
#if USE_PASTI
        if (hPasti && pasti_active){
          WORD data=io_src_b;
          if (addr<0xff8608){ // word only
            if (addr & 1){
              data=MAKEWORD(io_src_b,pasti_store_byte_access);
              addr&=~1;
            }else{
              pasti_store_byte_access=io_src_b;
              break;
            }
          }
          struct pastiIOINFO pioi;
          pioi.addr=addr;
          pioi.data=data;
          pioi.stPC=pc;
          pioi.cycles=ABSOLUTE_CPU_TIME;
//          log_to(LOGSECTION_PASTI,Str("PASTI: IO write addr=$")+HEXSl(addr,6)+" data=$"+
//                            HEXSl(io_src_b,2)+" ("+io_src_b+") pc=$"+HEXSl(pc,6)+" cycles="+pioi.cycles);
          pasti->Io(PASTI_IOWRITE,&pioi);
          pasti_handle_return(&pioi);
          break;
        }
#endif
        switch (addr){
//          ff 8604   R/W     |--------xxxxxxxx|   Disk Controller (Word Access)
          case 0xff8604:  //high byte of FDC access
            if (dma_mode & BIT_4){ //write DMA sector counter, 0x190
              dma_sector_count&=0xff;
              dma_sector_count|=int(io_src_b) << 8;
              if (dma_sector_count){
                dma_status|=BIT_1;
              }else{
                dma_status&=BYTE(~BIT_1); //status register bit for 0 count
              }
              dma_bytes_written_for_sector_count=0;
              log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
              break;
            }
            if (dma_mode & BIT_3){ // HD access
              log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Writing $"+HEXSl(io_src_b,2)+"xx to HDC register #"+((dma_mode & BIT_1) ? 1:0));
              break;
            }
            break;
          case 0xff8605:  //low byte of FDC access
          {
            if (dma_mode & BIT_4){ //write FDC sector counter, 0x190
              dma_sector_count&=0xff00;
              dma_sector_count|=io_src_b;
              if (dma_sector_count){
                dma_status|=BIT_1;
              }else{
                dma_status&=BYTE(~BIT_1); //status register bit for 0 count
              }
              dma_bytes_written_for_sector_count=0;
              log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
              break;
            }
            if (dma_mode & BIT_3){ // HD access
              log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Writing $xx"+HEXSl(io_src_b,2)+" to HDC register #"+((dma_mode & BIT_1) ? 1:0));
              break;
            }
            switch (dma_mode & (BIT_1+BIT_2)){
              case 0:
                floppy_fdc_command(io_src_b);
                break;
/*
r1 (r/w) - Track Register - The outermost track on the disk is
numbered 0.  During disk reading, writing, and verifying, the 177x
compares the Track Register to the track number in the sector ID
field.  When the 177x is busy, it ignores CPU writes to this register.
[In fact, it doesn't (undocumented), from Hatari]
The highest legal track number is 240.
*/
              case 2:
                if ((fdc_str & FDC_STR_BUSY)==0){
                  log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC track register to "+io_src_b);
                  fdc_tr=io_src_b;
                }else{
#if defined(STEVEN_SEAGAL) && defined(SS_FDC_CHANGE_TRACK_WHILE_BUSY)
#if defined(SS_FDC_TRACE)
                  TRACE("Track register change while busy\n");
#endif
                  if(SSE_HACKS_ON)
                    fdc_tr=io_src_b; 
#endif
                  log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Can't set FDC track register to "+io_src_b+", FDC is busy");
                }
                break;
/*
r2 (r/w) - Sector Register - During disk reading and writing, the 177x
compares the Sector Register to the sector number in the sector ID
field.  When the 177x is busy, it ignores CPU writes to this register.
[In fact, it doesn't (undocumented) from Hatari]
Valid sector numbers range from 1 to 240, inclusive.
*/
              case 4:
                if ((fdc_str & FDC_STR_BUSY)==0){
                  log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC sector register to "+io_src_b);
                  fdc_sr=io_src_b;
                }else{
#if defined(STEVEN_SEAGAL) && defined(SS_FDC_CHANGE_SECTOR_WHILE_BUSY)
#if defined(SS_FDC_TRACE)
                  TRACE("Sector register change while busy %d -> %d\n",fdc_sr,io_src_b);
#endif
                  if(SSE_HACKS_ON)   
                    fdc_sr=io_src_b; // fixes Delirious 4 loader without Pasti
#endif
                  log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Can't set FDC sector register to "+io_src_b+", FDC is busy");
                }
                break;
              case 6:
                log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC data register to "+io_src_b);
                fdc_dr=io_src_b;
                break;
            }
            break;
          }

          // Writes to DMA mode clear the DMA internal buffer
          case 0xff8606:  //high byte of DMA mode
            dma_mode&=0x00ff;
            dma_mode|=WORD(WORD(io_src_b) << 8);

            fdc_read_address_buffer_len=0;
            dma_bytes_written_for_sector_count=0;
            break;
          case 0xff8607:  //low byte of DMA mode
            dma_mode&=0xff00;
            dma_mode|=io_src_b;

            fdc_read_address_buffer_len=0;
            dma_bytes_written_for_sector_count=0;
            break;

          case 0xff8609:  //high byte of DMA pointer
            dma_address&=0x00ffff;
            dma_address|=((MEM_ADDRESS)io_src_b) << 16;
            log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Set DMA address to "+HEXSl(dma_address,6));
            break;
          case 0xff860b:  //mid byte of DMA pointer
            //DMA pointer has to be initialized in order low, mid, high
            dma_address&=0xff00ff;
            dma_address|=((MEM_ADDRESS)io_src_b) << 8;
            break;
          case 0xff860d:  //low byte of DMA pointer
            //DMA pointer has to be initialized in order low, mid, high
            dma_address&=0xffff00;
            dma_address|=io_src_b;
            break;
          case 0xff860e: //high byte of frequency/density control
            break; //ignore
          case 0xff860f: //low byte of frequency/density control
            break;
        }
      }
      break;
    }
#undef LOGSECTION
#define LOGSECTION LOGSECTION_VIDEO

    case 0xff8200:{  //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
                     //----------------------------------------=--------------- shifter
      //----------------------------------------=--------------- shifter
      //----------------------------------------=--------------- shifter
      // SS Shifter emulation is so complicated I took some functions out
      // of here
      if ((addr>=0xff8210 && addr<0xff8240) || addr>=0xff8280){
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }

        /////////////
        // Palette // // SS word (long) writes far more frequent (see below)
        /////////////

      else if (addr>=0xff8240 && addr<0xff8260){  //palette
        int n=(addr-0xff8240) >> 1; 
        
        // Writing byte to palette writes that byte to both the low and high byte!
        WORD new_pal=MAKEWORD(io_src_b,io_src_b & 0xf);

#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO)

#if defined(SS_VID_SHIFTER_EVENTS) && defined(SS_DEBUG)
        VideoEvents.Add(scan_y,LINECYCLES,'p', (n<<12)|io_src_b);  // little p
#endif

#if defined(SS_VID_PALETTE_BYTE_CHANGE) 
        // TESTING maybe Steem was right, Hatari is wrong
        if(addr&1) // the double write happens only on even addresses (?)
        {
          new_pal=(STpal[n]&0xFF00)|io_src_b; // fixes Golden Soundtracker demo
#if defined(SS_VID_PALETTE_BYTE_CHANGE_TRACE)
          TRACE("Single byte  %X write pal %X STPal[%d] %X->%X\n",io_src_b,addr,n,STpal[n],new_pal);
#endif
        }
#endif

        Shifter.SetPal(n,new_pal);
        log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Palette change at scan_y="+scan_y+" cycle "+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl));

#else // Steem 3.2

        if (STpal[n]!=new_pal){
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Palette change at scan_y="+scan_y+" cycle "+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl));
          STpal[n]=new_pal;
          PAL_DPEEK(n*2)=new_pal;  // SS setting pal before drawing???
          if (draw_lock) draw_scanline_to(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl+1);
          if (flashlight_flag==0 && draw_line_off==0 DEBUG_ONLY( && debug_cycle_colours==0) ){
            palette_convert(n);
          }
        }

#endif

      }
      else{
        switch(addr){

/*
    Video Base Address:                           ST     STE

    $FFFF8201    0 0 X X X X X X   High Byte      yes    yes
    $FFFF8203    X X X X X X X X   Mid Byte       yes    yes
    $FFFF820D    X X X X X X X 0   Low Byte       no     yes

    These registers only contain the "reset" value for the Shifter after a 
    whole screen has been drawn. It does not affect the current screen, but 
    the one for the next VBL. To make immediate changes on the screen, use 
    the Video Address Counter [(SDP)].

    According to ST-CNX, those registers are in the MMU, not in the shifter.
*/

        /////////////////
        // Videobase H //
        /////////////////

        case 0xff8201:  //high byte of screen memory address
#if defined(STEVEN_SEAGAL) && defined(SS_VID_SHIFTER_EVENTS)
          VideoEvents.Add(scan_y,LINECYCLES,'V',io_src_b); 
#endif
          if (mem_len<=FOUR_MEGS) io_src_b&=b00111111;
///if(Shifter.FetchingLine())
          DWORD_B_2(&xbios2)=io_src_b;
/*
 For compatibility reasons, the low-byte of the Video Base Address is ALWAYS
 set to 0 when the mid- or high-byte of the Video Base Address are set. 
 This is easy to understand, seeing that the ST did not have this register -
 hence ST software that never sets the low-byte might have problems setting
 the correct Video Base Address. The solution on the STE is simple: Always set
 the Low-Byte last. 
 First set High and Mid-Byte (in no special order), then do the low-byte.
*/
#if defined(STEVEN_SEAGAL) && defined(SS_STF)
          if(ST_TYPE==STE) 
#endif
            DWORD_B_0(&xbios2)=0; 
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;

        /////////////////
        // Videobase M //
        /////////////////

        case 0xff8203:  //mid byte of screen memory address
#if defined(STEVEN_SEAGAL) && defined(SS_VID_SHIFTER_EVENTS)
          VideoEvents.Add(scan_y,LINECYCLES,'V',io_src_b); 
#endif

//ASSERT(Shifter.FetchingLine());
///if(Shifter.FetchingLine())

////////////////////io_src_b&=b11111110;


          DWORD_B_1(&xbios2)=io_src_b;

#if defined(STEVEN_SEAGAL) && defined(SS_STF)
          if(ST_TYPE==STE) 
#endif
            DWORD_B_0(&xbios2)=0;
#ifdef SS_TST1
//          else if(Shifter.FetchingLine()) DWORD_B_0(&xbios2)=6;
#endif 



          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;

        /////////////////////
        // Write SDP (STE) // 
        /////////////////////

        case 0xff8205:  //high byte of draw pointer
        case 0xff8207:  //mid byte of draw pointer
        case 0xff8209:  //low byte of draw pointer

#if defined(STEVEN_SEAGAL) && defined(SS_VID_SDP_WRITE)
          Shifter.WriteSDP(addr,io_src_b); // very complicated!
          break;

#else // Steem 3.2 or SS_VID_SDP_WRITE not defined
        {
//          int srp=scanline_raster_position();
          int dst=ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl;
          dst-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
          dst+=16;dst&=-16;
          dst+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO) // video defined but not SDP
          Shifter.Render(dst);
#else
          draw_scanline_to(dst); // This makes shifter_draw_pointer up to date
#endif
          MEM_ADDRESS nsdp=shifter_draw_pointer;
          if (mem_len<=FOUR_MEGS && addr==0xff8205) io_src_b&=b00111111;
          DWORD_B(&nsdp,(0xff8209-addr)/2)=io_src_b;

/*
          if (shifter_hscroll){
            if (dst>=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-32 && dst<CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320-16){
              log_to(LOGSECTION_VIDEO,Str("ATTANT: addr=")+HEXSl(addr,6));
              if (addr==0xff8209){
                // If you set low byte while on screen with hscroll on then sdp will
                // be an extra raster ahead. Steem's sdp is always 1 raster ahead, so
                // correct for that here.
                nsdp-=8;
              }
            }
          }
*/

//          int off=(get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)&-8)-shifter_draw_pointer;
//          shifter_draw_pointer=nsdp-off;
          shifter_draw_pointer_at_start_of_line-=shifter_draw_pointer;
          shifter_draw_pointer_at_start_of_line+=nsdp;
          shifter_draw_pointer=nsdp;

          log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+" - Set shifter draw pointer to "+
                    HEXSl(shifter_draw_pointer,6)+" at "+scanline_cycle_log()+", aligned to "+dst);
          break;
        }
#endif

        ///////////////////////
        // Videobase L (STE) //
        ///////////////////////
/* 

 VBASELO 
 This register contains the low-order byte of the video display base address. 
 It can be altered at any time and will affect the next display processor data 
 fetch. it is recommended that the video display address be altered only during 
 vertical and horizontal blanking or display garbage may result. 

  The last one (low byte) did not exist on the ST.
  [That's why the low byte is separated from the high & mid bytes.]
  While on the ST this meant that a Video Base address had to be even on 
  a 256-byte basis ( lowbyte was assumed #$00 then ), on the STE it only has
   to be even sincebit 0 is automatically assumed #0.
*/
        case 0xff820d:  //low byte of screen memory address
#if defined(STEVEN_SEAGAL) && defined(SS_VID_SHIFTER_EVENTS)
          VideoEvents.Add(scan_y,LINECYCLES,'v',io_src_b); 
#endif
#if defined(STEVEN_SEAGAL) && defined(SS_STF)
          if(ST_TYPE!=STE)
          {
#if defined(SS_STF_TRACE)
            TRACE("STF ignore write %X to %X VBASELO\n",io_src_b,addr);
#endif
            break; // fixes Lemmings 40; used by Beyond/Pax Plax Parallax
          }
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO)
          ASSERT(!(io_src_b&1));
          io_src_b&=-1; // last bit never set
#endif
          DWORD_B_0(&xbios2)=io_src_b;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;

        //////////
        // Sync //
        //////////
        
        case 0xff820a: //synchronization mode
        
#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO)
          Shifter.SetSyncMode(io_src_b); 
          break;

#else // Steem 3.2
        {
          int new_freq;

          if (io_src_b & 2){
            new_freq=50;
          }else{
            new_freq=60;
          }

          if (screen_res>=2) new_freq=MONO_HZ;

          if (shifter_freq!=new_freq){
            log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Changed frequency to "+new_freq+
                            " at "+scanline_cycle_log());
            shifter_freq=new_freq;
            CALC_SHIFTER_FREQ_IDX;
            if (shifter_freq_change[shifter_freq_change_idx]!=MONO_HZ){
              ADD_SHIFTER_FREQ_CHANGE(shifter_freq);
            }
            freq_change_this_scanline=true;
            if (FullScreen && border==2){
              int off=shifter_first_draw_line-draw_first_possible_line;
              draw_first_possible_line+=off;
              draw_last_possible_line+=off;
            }
          }
          break;
        }
#endif

        /////////////////////
        // LineWidth (STE) //
        /////////////////////
/*
   LINEWID This register indicates the number of extra words of data (beyond
   that required by an ordinary ST at the same resolution) which represent
   a single display line. If it is zero, this is the same as an ordinary ST. 
   If it is nonzero, that many additional words of data will constitute a
   single video line (thus allowing virtual screens wider than the displayed 
   screen). CAUTION In fact, this register contains the word offset which 
   the display processor will add to the video display address to point to 
   the next line. If you are actively scrolling (HSCROLL <> 0), this register
   should contain The additional width of a display line minus one data fetch
   (in low resolution one data fetch would be four words, one word for 
   monochrome, etc.).

  Line-Offset Register

    $FFFF820F  X X X X X X X X X                  no     yes

 This register contains the value how many WORDS (not BYTES!) the Shifter is
 supposed to skip after each Rasterline. This register enables virtual screens
 that are (horizontally) larger than the displayed screen by making the 
 Shifter skip the set number of words when a line has been drawn on screen.    

 The Line Offset Register is very critical. Make sure it contains the correct
 value at any time. If the Pixel Offset Register contains a zero, the Line 
 Offset Register contains the exact number of words to skip after each line. 
 But if you set the Pixel Offset Register to "X", the Shifter will still 
 display 320 (640) pixels a line and therefore has to read "X" pixels from
 the NEXT word which it would have skipped if the Pixel offset Register 
 contained a "0". Hence, for any Pixel Offset Value > 0, please note that 
 the Shifter has to read (a few bits) more each rasterline and these bits 
 must NOT be skipped using the Line Offset Register. 
*/
	    case 0xff820f:   //int shifter_fetch_extra_words;
#if defined(STEVEN_SEAGAL) && defined(SS_VID_SHIFTER_EVENTS)
          VideoEvents.Add(scan_y,LINECYCLES,'F',io_src_b); 
#endif
#if defined(STEVEN_SEAGAL) && defined(SS_STF)
          if(ST_TYPE!=STE)
          {
#if defined(SS_STF_TRACE)
            TRACE("STF ignore write %X to %X LINEWID\n",io_src_b,addr);
#endif
            break; // fixes Imagination/Roller Coaster mush
          }
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO) 
          Shifter.Render(LINECYCLES,DISPATCHER_LINEWIDTH); // eg Beat Demo
#else
          draw_scanline_to(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl); // Update sdp if off right  
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VID_SDP_TRACE)
          TRACE("F%d y%d c%d LW %d -> %d\n",FRAME,scan_y,LINECYCLES,shifter_fetch_extra_words,io_src_b);
#endif
          shifter_fetch_extra_words=(BYTE)io_src_b;

          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set shifter_fetch_extra_words to "+
                    (shifter_fetch_extra_words)+" at "+scanline_cycle_log());
		  break;

        ///////////////////
        // Hscroll (STE) //
        ///////////////////
/*

 HSCROLL 
 This register contains the pixel scroll offset. 
 If it is zero, this is the same as an ordinary ST. 
 If it is nonzero, it indicates which data bits constitute the first pixel 
 from the first word of data. 
 That is, the leftmost displayed pixel is selected from the first data word(s)
 of a given line by this register.


 FF8264 ---- ---- ---- xxxx

[Notice how writes to FF8264 should be ignored. They aren't.]

 Horizontal Bitwise Scroll.
 Delays the start of screen by the specified number of bits. 
  
    Video Base Address Pixel Offset              STF     STE

    $FFFF8265  0 0 0 0 X X X X                   no      yes

This register allows to skip from a single to 15 pixels at the start of each
 rasterline to allow horizontal fine-scrolling. 
*/

        case 0xff8264:   
          // Set hscroll and don't change line length
          // This is an odd register, when you change hscroll below to non-zero each
          // scanline becomes 4 words longer to allow for extra screen data. This register
          // sets hscroll but doesn't do that, instead the left border is increased by
          // 16 pixels. If you have got hscroll extra fetch turned on then setting this
          // to 0 confuses the shifter and causes it to shrink the left border by 16 pixels.
        case 0xff8265:  // Hscroll
#if defined(STEVEN_SEAGAL) && defined(SS_VID_SHIFTER_EVENTS)
          VideoEvents.Add(scan_y,LINECYCLES,(addr==0xff8264)?'h':'H',io_src_b); 
#endif
#if defined(STEVEN_SEAGAL) && defined(SS_STF)
          if(ST_TYPE!=STE) 
          {
#if defined(SS_STF_TRACE)
            TRACE("STF ignore write %X to %X HSCROLL\n",io_src_b,addr);
#endif
            break; // fixes Hyperforce
          }
          else
#endif
          {
            int cycles_in=(int)(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO) // no need to render
#else
            draw_scanline_to(cycles_in); // Update sdp if off right
            shifter_pixel-=shifter_hscroll; //SS not needed/error?
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VID_SDP_TRACE)
            TRACE("F%d y%d c%d HS %d -> %d\n",FRAME,scan_y,LINECYCLES,shifter_hscroll,io_src_b);
#endif

            shifter_hscroll=io_src_b & 0xf;
#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO)
#else
            shifter_pixel+=shifter_hscroll;
#endif
            log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set horizontal scroll ("+HEXSl(addr,6)+
                    ") to "+(shifter_hscroll)+" at "+scanline_cycle_log());
            if (addr==0xff8265) shifter_hscroll_extra_fetch=(shifter_hscroll!=0); //OK


#if defined(STEVEN_SEAGAL) && defined(SS_VID_BORDERS)
            if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-BORDER_SIDE){
#else
            if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-32){
#endif           // eg Coreflakes hidden screen
              if (left_border>0){ // Don't do this if left border removed!
                shifter_skip_raster_for_hscroll = shifter_hscroll!=0; //SS computed at end of line anyway
                left_border=BORDER_SIDE;
                if (shifter_hscroll) left_border+=16;
                if (shifter_hscroll_extra_fetch) left_border-=16;
              }
            }
          } 
          break;

        ////////////////
        // Shift Mode //
        ////////////////

        case 0xff8260: //resolution

#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO) 
////ASSERT(!screen_res);
          Shifter.SetShiftMode(io_src_b);
          break;
        
#else // Steem 3.2
        
          if (screen_res>=2 || emudetect_falcon_mode!=EMUD_FALC_MODE_OFF) return;
#ifndef NO_CRAZY_MONITOR
          if (extended_monitor){
            screen_res=BYTE(io_src_b & 1);
            return;
          }
#endif
          io_src_b&=3;
          int cycles_in=(int)(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
          int dst=cycles_in;
          dst-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
          dst+=16;dst&=-16;
          dst+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;

          draw_scanline_to(dst);
//          cycles_in%=(scanline_time_in_cpu_cycles[shifter_freq_idx]);
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Changed screen res to "+
                              io_src_b+" at scanline "+scan_y+", cycle "+cycles_in);
          if ( mfp_gpip_no_interrupt & MFP_GPIP_COLOUR ){
            if ( io_src_b==2 ){
              // Trying to change to hi res in colour - that's crazy!
              // But wait!  You can remove the left border like that!  Wow!
              ADD_SHIFTER_FREQ_CHANGE(MONO_HZ);
              freq_change_this_scanline=true;
            }else{
              if (shifter_freq_change[shifter_freq_change_idx]==MONO_HZ){
                ADD_SHIFTER_FREQ_CHANGE(shifter_freq);
                freq_change_this_scanline=true;
/*
                if (cycles_in<80){
                  int raster_start=(cycles_in-8)/2;
                  shifter_draw_pointer+=raster_start; //adjust sdp for hi-res bit
                  overscan_add_extra-=raster_start;
                  if (raster_start){
                    log_to_section(LOGSECTION_VIDEO,Str("SHIFTER: Adjusted SDP for late "
                                "change from mono, increased by ")+raster_start+" bytes");
                  }
                }
*/
              }
              int old_screen_res=screen_res;
              screen_res=BYTE(io_src_b & 1);
              if (screen_res!=old_screen_res){
                if (screen_res>0){
                  shifter_x=640;
                }else{
                  shifter_x=320;
                }
                if (draw_lock){
                  if (screen_res==0) draw_scanline=draw_scanline_lowres;
                  if (screen_res==1) draw_scanline=draw_scanline_medres;
#ifdef WIN32
                  if (draw_store_dest_ad){
                    draw_store_draw_scanline=draw_scanline;
                    draw_scanline=draw_scanline_1_line[screen_res];
                  }
#endif
                }
                if (mixed_output==3 && (ABSOLUTE_CPU_TIME-cpu_timer_at_res_change<30)){
                  mixed_output=0; //cancel!
                }else if (mixed_output==0){
                  mixed_output=3;
                }else if (mixed_output<2){
                  mixed_output=2;
                }
                cpu_timer_at_res_change=ABSOLUTE_CPU_TIME;
//              shifter_fetch_extra_words=0; //unspecified
//              draw_set_jumps_and_source();
              }
            }
          }
          break;
#endif

        }//sw
      }//if
    }//scope
    break;//shifter

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO
    case 0xff8000:  //--------------------------------------- memory
    {
      if (addr==0xff8001){ //Memory Configuration
/*
from WinSTon/Hatari:

  "The Atari ST TOS needs to be patched to help with emulation. Eg, it
  references the MMU chip to set memory size. This is patched to the
  sizes we need without the complicated emulation of hardware which
  is not needed (as yet). We also patch DMA devices and Hard Drives."
  
  SS In Steem, this complicated emulation has been done (not by me, by 
  Steem authors)! It would be silly  to discard it. 
  It's only useful  at TOS boot time, apparently, but it  doesn't come 
  in the way of performance, from what we can see.
  Except booting TOS 1.06 takes more time in Steem, probably as on a STE.
  Booting a 4MB machine with TOS 1.00 fails, to check.

  Atari doc:

          ff 8001   R/W             |----xxxx|   Memory Configuration
                                         ||||
                                          -------   Bank0      Bank1 (not used)
                                         0000       128 Kbyte  128 Kbyte
                                         0001       128 Kbyte  512 Kbyte
                                         0010       128 Kbyte    2 Mbyte
                                         0011       Reserved
                                    4    0100       512 Kbyte  128 Kbyte
                                    5    0101       512 Kbyte  512 Kbyte
                                         0110       512 Kbyte    2 Mbyte
                                         0111       Reserved
                                         1000         2 Mbyte  128 Kbyte
                                         1001         2 Mbyte  512 Kbyte
                                         1010         2 Mbyte    2 Mbyte
                                         1011       Reserved
                                         11xx       Reserved



               The configuration of main memory consists  of  five  64
          Kbyte sets of ROM (standard set0 to set2, expansion set3 and
          set4) and one configurable  bank  (standard  bank0)  of  128
          Kbyte, 512 Kbyte, or 2 Mbyte RAM.  The configuration of main
          memory ROM is ascertained through  software  identification.
          The  configuration  of  main  memory RAM is achieved via the
          programming   of   the   Memory    Configuration    Register
          (read/write,  reset:  all zeros).  RAM configuration must be
          asserted during the first steps of the power up sequence and
          can  be  determined by using the following shadow test algo-
          rithm:



          START   o  write 0x000a (2 Mbyte, 2 Mbyte) to the Memory
                     Configuration Register.

          BANK0   o  write Pattern to 0x000000 - 0x0001ff.
                  o  read Pattern from 0x000200 - 0x0003ff.
                  o  if Match, then Bank0 contains 128 Kbyte; goto BANK1.
                  o  read Pattern from 0x000400 - 0x0005ff.
                  o  if Match, then Bank0 contains 512 Kbyte; goto BANK1.
                  o  read Pattern from 0x000000 - 0x0001ff.
                  o  if Match, then Bank0 contains 2 Mbyte; goto BANK1.
                  o  panic:  RAM error in Bank0.

          BANK1   o  write Pattern to 0x200000 - 0x2001ff.
                  o  read Pattern from 0x200200 - 0x2003ff.
                  o  if Match, then Bank1 contains 128 Kbyte; goto FIN.
                  o  read Pattern from 0x200400 - 0x2005ff.
                  o  if Match, then Bank1 contains 512 Kbyte; goto FIN.
                  o  read Pattern from 0x200000 - 0x2001ff.
                  o  if Match, then Bank1 contains 2 Mbyte; goto FIN.
                  o  note:  Bank1 nonexistent.

          FIN     o  write Configuration to the Memory Configuration
                     Register.
                  o  note Total Memory Size (Top of RAM) for future
                     reference.


eg, trace:

TOS102
MMU PC FC00F2 Byte A RAM 1024K Bank 0 512 Bank 1 512 testing 1
MMU PC FC0154 Byte 5 RAM 1024K Bank 0 512 Bank 1 512 testing 0
  
TOS106
MMU PC E000E6 Byte A RAM 1024K Bank 0 512 Bank 1 512 testing 1
MMU PC E0014C Byte 5 RAM 1024K Bank 0 512 Bank 1 512 testing 0

  This is one of the very few changes we made without defines:
  mmu_confused -> mmu_testing everywhere (var & func)

*/

#if defined(STEVEN_SEAGAL) && defined(SS_MMU_WRITE) 
        if(mem_len<=FOUR_MEGS){ // programs actually may write here?
#else
        if (old_pc>=FOURTEEN_MEGS && mem_len<=FOUR_MEGS){
#endif
//          TRACE("PC %X write %X to MMU\n",pc,io_src_b);
          mmu_memory_configuration=io_src_b;
          mmu_bank_length[0]=mmu_bank_length_from_config[(mmu_memory_configuration & b1100) >> 2];
          mmu_bank_length[1]=mmu_bank_length_from_config[(mmu_memory_configuration & b0011)];
#if !defined(SS_MMU_NO_CONFUSION)
          mmu_testing=false;
          if (bank_length[0]) if (mmu_bank_length[0]!=bank_length[0]) mmu_testing=true;
          if (bank_length[1]) if (mmu_bank_length[1]!=bank_length[1]) mmu_testing=true;
#if defined(STEVEN_SEAGAL) && defined(SS_MMU_WRITE)
          if(old_pc<FOURTEEN_MEGS) // the write doesn't "confuse" the MMU
          {
#if defined(SS_MMU_TRACE)
            TRACE("Cancel MMU testing\n");
#endif
            mmu_testing=false; // fixes Super Neo Demo Show (1MB)
          }
#endif
          himem=(MEM_ADDRESS)(mmu_testing ? 0:mem_len);
#else
          himem=(MEM_ADDRESS)mem_len;
          int mmu_testing=0;//dbg
#endif
#if defined(SS_MMU_TRACE)
          TRACE("MMU PC %X Byte %X RAM %dK Bank 0 %d Bank 1 %d testing %d\n",old_pc,io_src_b,mem_len/1024,bank_length[0]/1024,bank_length[1]/1024,mmu_testing);
#endif
        }
      }else if (addr>0xff800f){
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }
      break;
    }

    case 0xffc100: //secret Steem registers!
    {
#ifdef DEBUG_BUILD
      if (addr==0xffc123){ //stop
        if (runstate==RUNSTATE_RUNNING){
          runstate=RUNSTATE_STOPPING;
          SET_WHY_STOP("Software break - write to $FFC123");
        }
        break;
      }else if (addr==0xffc1f4){
        logfile_wipe();
      }
#endif
      if (emudetect_called){
        switch (addr){
          // 100.l = create disk image
          case 0xffc104: emudetect_reset(); break;

          case 0xffc105: new_n_cpu_cycles_per_second=min(max((int)(io_src_b),8),128)*1000000; break;

          case 0xffc108: // Run speed percent hi
          case 0xffc109: // Run speed percent low
          {
            WORD percent=WORD(100000/run_speed_ticks_per_second);
            if (addr==0xffc108) WORD_B_1(&percent)=io_src_b; // High byte
            if (addr==0xffc109) WORD_B_0(&percent)=io_src_b; // Low byte
            run_speed_ticks_per_second=100000 / max((int)(percent),50);
            break;
          }
          case 0xffc107: snapshot_loaded=(bool)(io_src_b); break;
          case 0xffc11a: emudetect_write_logs_to_printer=(bool)(io_src_b); break;
          case 0xffc11b:
            if (extended_monitor==0 && screen_res<2 && BytesPerPixel>1) emudetect_falcon_mode=BYTE(io_src_b);
            break;
          case 0xffc11c:
            emudetect_falcon_mode_size=BYTE((io_src_b & 1)+1);
            emudetect_falcon_extra_height=bool(io_src_b & 2);
            // Make sure we don't mess up video memory. It is possible that the height of
            // scanlines is doubled, if we change to 400 with double height lines then arg!
            draw_set_jumps_and_source();
            break;
          case 0xffc11d: emudetect_overscans_fixed=bool(io_src_b); break;
        }
        if (addr<0xffc120) break; // No exception!
      }
      exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
    }
    case 0xfffd00:{ //?????
      break;
    }case 0xff9000:{ //?????
      if (addr>0xff9001) exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      break;
    }case 0xff9200:{ //paddles
#if defined(STEVEN_SEAGAL) && defined(SS_STF)
       if(ST_TYPE!=STE)
       {
#if defined(SS_STF_TRACE)
         TRACE("STF write to 0xff9200\n");
#endif
         break;  // or bombs?
       }
#endif
      if (addr==0xff9202){ // Doesn't work for high byte
        WORD_B_1(&paddles_ReadMask)=0;
      }else if (addr==0xff9203){
        WORD_B_0(&paddles_ReadMask)=io_src_b;
      }else{
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }
      break;

    }default:{ //unrecognised
      exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
    }
  }
}
//---------------------------------------------------------------------------
void ASMCALL io_write_w(MEM_ADDRESS addr,WORD io_src_w)
{
  if (addr>=0xff8240 && addr<0xff8260){  //palette
    DEBUG_CHECK_WRITE_IO_W(addr,io_src_w);
    int n=(addr-0xff8240) >> 1;

#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO)
  
#if defined(SS_VID_SHIFTER_EVENTS) && defined(SS_DEBUG)
    if(Blit.HasBus)
      VideoEvents.Add(scan_y,LINECYCLES,'Q',(n<<12)|io_src_w); 
    else
      VideoEvents.Add(scan_y,LINECYCLES,'P',(n<<12)|io_src_w); 
#endif
    Shifter.SetPal(n,io_src_w); 
    log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+" - Palette change at scan_y="+scan_y+" cycles so far="+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl));

#else // Steem 3.2

    io_src_w&=0xfff;
    if (STpal[n]!=io_src_w){
      STpal[n]=io_src_w;
      PAL_DPEEK(n*2)=STpal[n];
      log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+" - Palette change at scan_y="+scan_y+" cycles so far="+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl));
      if (draw_lock) draw_scanline_to((ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)+1);
      if (flashlight_flag==0 && draw_line_off==0 DEBUG_ONLY( && debug_cycle_colours==0) ){
        palette_convert(n);
      }
    }

#endif

  }else{
    io_word_access=true;
    io_write_b(addr,HIBYTE(io_src_w));
    io_write_b(addr+1,LOBYTE(io_src_w));
    io_word_access=0;
  }
}
//---------------------------------------------------------------------------
void ASMCALL io_write_l(MEM_ADDRESS addr,LONG io_src_l)
{
  if (emudetect_called){
    if (addr==0xffc100){
      DWORD addr=io_src_l;
      Str Name=read_string_from_memory(addr,500);
      addr+=Name.Length()+1;

      int Param[10]={0,0,0,0,0,0,0,0,0,0};
      Str Num;
      for (int n=0;n<10;n++){
        Num=read_string_from_memory(addr,16);
        if (Num.Length()==0) break;
        addr+=Num.Length()+1;
        Param[n]=atoi(Num);
      }

      int Sides=2,TracksPerSide=80,SectorsPerTrack=9;
      if (Param[0]==1 || Param[0]==2) Sides=Param[0];
      if (Param[1]>=10 && Param[1]<=FLOPPY_MAX_TRACK_NUM+1) TracksPerSide=Param[1];
      if (Param[2]>=1 && Param[2]<=FLOPPY_MAX_SECTOR_NUM) SectorsPerTrack=Param[2];
      GUIEmudetectCreateDisk(Name,Sides,TracksPerSide,SectorsPerTrack);
      return;
    }
  }
  if (addr==0xffc1f0){
#ifdef DEBUG_BUILD
    log_write(Str("ST -- ")+read_string_from_memory(io_src_l,500));
    return;
#else
    if (emudetect_write_logs_to_printer){
      // This can't be turned on unless you call emudetect, so 0xffc1f0 will still work normally
      Str Text=read_string_from_memory(io_src_l,500);
      for (int i=0;i<Text.Length();i++) ParallelPort.OutputByte(Text[i]);
      ParallelPort.OutputByte(13);
      ParallelPort.OutputByte(10);
      return;
    }
#endif
  }
  INSTRUCTION_TIME(-4); // SS cool for all cases?
  io_write_w(addr,HIWORD(io_src_l));
  INSTRUCTION_TIME(4);
  io_write_w(addr+2,LOWORD(io_src_l));
}

#undef LOGSECTION
