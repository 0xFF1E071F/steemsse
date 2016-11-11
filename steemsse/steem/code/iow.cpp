/*---------------------------------------------------------------------------
FILE: iow.cpp
MODULE: emu
DESCRIPTION: I/O address writes. This file contains crucial core functions
that deal with writes to ST I/O addresses ($ff8000 onwards), this is the only
route of communication between programs and the chips in the emulated ST.
---------------------------------------------------------------------------*/

#if defined(SSE_STRUCTURE_INFO)
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
#if defined(SSE_DEBUG_TRACE_IO)
  if(!io_word_access
#if defined(SSE_BOILER_TRACE_CONTROL)
    && (TRACE_MASK_IO & TRACE_CONTROL_IO_W)
//    && (((1<<15)&d2_dpeek(FAKE_IO_START+24))) 
    // we add conditions address range - logsection enabled
      //&& (old_pc<rom_addr)
//      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_INTERRUPTS] ) //mfp
      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_MFP] ) //mfp
      && ( (addr&0xffff00)!=0xfffc00 || logsection_enabled[LOGSECTION_IKBD] ) //acia
#if defined(SSE_BOILER_383_LOG2)
      && ( (addr&0xffff00)!=0xff8600 || logsection_enabled[LOGSECTION_DMA] ) //dma
#else
      && ( (addr&0xffff00)!=0xff8600 || logsection_enabled[LOGSECTION_FDC] ) //dma
#endif
      && ( (addr&0xffff00)!=0xff8800 || logsection_enabled[LOGSECTION_SOUND] )//psg
      && ( (addr&0xffff00)!=0xff8900 || logsection_enabled[LOGSECTION_SOUND] )//dma
      && ( (addr&0xffff00)!=0xff8a00 || logsection_enabled[LOGSECTION_BLITTER] )
      && ( (addr&0xffff00)!=0xff8200 || logsection_enabled[LOGSECTION_VIDEO] )//shifter
      && ( (addr&0xffff00)!=0xff8000 || (((1<<13)&d2_dpeek(FAKE_IO_START+24)) ))//MMU

#endif
    ) 
    TRACE_LOG("%d PC %X write byte %X to %X\n",ACT,old_pc,io_src_b,addr);
#endif

#if defined(SSE_MMU_ROUNDING_BUS0A)
/*  Should round up only for RAM and Shifter (palette), not peripherals
    that sit on the CPU bus.
*/
    if(MMU.Rounded && !(addr>=0xff8240 
     && addr<(MEM_ADDRESS)(ST_TYPE==STE?0xff8266:0xff8260))
#if defined(DEBUG_BUILD)
     && mode==STEM_MODE_CPU // no cycles when boiler is reading
#endif      
     )
  {
    INSTRUCTION_TIME(-2);
    MMU.Rounded=false;
    //TRACE("PC %X addr %X\n",old_pc,addr);
#if defined(SSE_OSD_CONTROL)
    if((OSD_MASK_CPU&OSD_CONTROL_CPUROUNDING))
      TRACE_OSD("W %X -2",addr);
#endif
   }
#endif

#ifdef DEBUG_BUILD
#if defined(SSE_BOILER_MONITOR_IO_FIX1)
  if(!io_word_access)
#endif
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

#if defined(SSE_ACIA) && defined(SSE_ACIA_383) //more compact code

#undef LOGSECTION
#define LOGSECTION LOGSECTION_ACIA

    case 0xfffc00:{  //--------------------------------------- ACIAs
/*
          MC6850

          ff fc00                   |xxxxxxxx|   Keyboard ACIA Control
          ff fc02                   |xxxxxxxx|   Keyboard ACIA Data

          ff fc04                   |xxxxxxxx|   MIDI ACIA Control
          ff fc06                   |xxxxxxxx|   MIDI ACIA Data

*/
      acia[0].BusJam(addr);
      int acia_num;
      switch (addr){
      case 0xfffc00: // Keyboard ACIA Control
      case 0xfffc04: // MIDI ACIA Control
        acia_num=(addr&0xf)>>2;
        ASSERT(acia_num==0 || acia_num==1);
        acia[acia_num].CR=io_src_b; // no option test
        if ((io_src_b & 3)==3)  // 'Master reset'
        {
          log_to(acia_num?LOGSECTION_MIDI:LOGSECTION_IKBD,Str("ACIA ")+Str(acia_num)+": "+HEXSl(old_pc,6)+" - ACIA reset "); 
          ACIA_Reset(acia_num,0);
        }
        else
          ACIA_SetControl(acia_num,io_src_b); // TOS: 95 for MIDI, 96 for IKBD
        break;

      case 0xfffc02:  // Keyboard ACIA Data
      case 0xfffc06:  // MIDI ACIA Data
        acia_num=((addr&0xf)-2)>>2;
        ASSERT(acia_num==0 || acia_num==1);
        acia[acia_num].TDR=io_src_b; // no option test
        TRACE_LOG("%d PC %X ACIA %d TDR %X\n",ACT,old_pc,acia_num,io_src_b);
        if(OPTION_C1)
        {
/*  Effect of write on status register.
    The 'Tx data register empty' (TDRE) bit is cleared: register isn't empty.
    Writing on ACIA TDR clears the IRQ bit if IRQ for transmission
    is enabled. Eg Hades Nebula
*/
          acia[acia_num].SR&=~BIT_1; // clear TDRE bit
          if(acia[acia_num].IrqForTx())
            acia[acia_num].SR&=~BIT_7; // clear IRQ bit
          
          //update in MFP (if needs be)
          mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,
            !((ACIA_IKBD.SR&BIT_7) || (ACIA_MIDI.SR&BIT_7)));

/*  If the line is free, the byte in TDR is copied almost at once into the 
    shifting register, and TDR is free again.
    When TDR is free, status bit TDRE is set, and Steem's tx_flag is false!
    (hard to follow)
    If the ACIA is shifting and already has a byte in TDR, the byte in TDR
    can be changed (High Fidelity Dreams).
    We record the timing of 'tx' only if the line is free: Pandemonium Demos.
    Grumbler by Electricity: key repeat in 6301 true emu mode
*/
          if(!acia[acia_num].LineTxBusy
            || ACT-acia[acia_num].last_tx_write_time<ACIA_TDR_COPY_DELAY)
          {
            if(!acia[acia_num].LineTxBusy)
              acia[acia_num].last_tx_write_time=ABSOLUTE_CPU_TIME;

            if(acia_num==NUM_ACIA_IKBD)
              HD6301.ReceiveByte(io_src_b);
            else
            {
              acia[acia_num].TDRS=acia[acia_num].TDR;
              acia[acia_num].time_of_event_outgoing=ACT+ACIA_MIDI_OUT_CYCLES;
              acia[acia_num].LineTxBusy=true;
            }
            acia[acia_num].SR|=BIT_1; // TDRE free (see ior: can be "cancelled")

            // rare case when IRQ is enabled for transmit
            if(acia[acia_num].IrqForTx())
            {
              TRACE_LOG("ACIA %d TX IRQ\n",acia_num);            
              acia[acia_num].SR|=BIT_7; 
              mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,0); //trigger
            }
          }
          else
          {
#if defined(SSE_DEBUG)
            ASSERT(acia[acia_num].LineTxBusy);
            if(!acia[acia_num].ByteWaitingTx) // TDR was free 
              TRACE_LOG("ACIA %d byte waiting $%X\n",acia_num,io_src_b);
            else
              TRACE_LOG("ACIA %d new byte waiting $%X (instead of $%X)\n",acia_num,io_src_b,ACIA_IKBD.TDR);
#endif
            if(ACT-acia[acia_num].last_tx_write_time<ACIA_TDR_COPY_DELAY)
              acia[acia_num].TDRS=acia[acia_num].TDR; // replaces
            else
              acia[acia_num].ByteWaitingTx=true;
          }

          break;
        }//option C1
        {
          bool TXEmptyAgenda=(agenda_get_queue_pos(acia_num==NUM_ACIA_IKBD?
            agenda_acia_tx_delay_IKBD:agenda_acia_tx_delay_MIDI)>=0);
          if (TXEmptyAgenda==0){
            if (acia[acia_num].tx_irq_enabled){
              acia[acia_num].irq=false;
              mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
            }
            agenda_add(acia_num==NUM_ACIA_IKBD?agenda_acia_tx_delay_IKBD
              :agenda_acia_tx_delay_MIDI,
              ACIAClockToHBLS(acia[acia_num].clock_divide),0);
          }
          acia[acia_num].tx_flag=true; //flag for transmitting

          //Steem 3.2, not C1, different paths for IKBD and MIDI:
          if(acia_num==NUM_ACIA_IKBD)
          {
            // If send new byte before last one has finished being sent
            if (abs(ABSOLUTE_CPU_TIME-acia[acia_num].last_tx_write_time)
              <ACIA_CYCLES_NEEDED_TO_START_TX){
                // replace old byte with new one
                int n=agenda_get_queue_pos(agenda_ikbd_process);
                if (n>=0){
                  log_to(LOGSECTION_IKBD,Str("IKBD: ")+HEXSl(old_pc,6)+" - Received new command before old one was sent, replacing "+
                    HEXSl(agenda[n].param,2)+" with "+HEXSl(io_src_b,2));
                  agenda[n].param=io_src_b;
                }
            }else{
              // there is a delay before the data gets to the IKBD
              acia[acia_num].last_tx_write_time=ABSOLUTE_CPU_TIME;
              agenda_add(agenda_ikbd_process,IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESS,io_src_b);
            }
          }
          else
          {
            ASSERT(acia_num==NUM_ACIA_MIDI);
            MIDIPort.OutputByte(io_src_b);
          }
        break;
      }
    //-------------------------- unrecognised -------------------------------------------------
      default:
        break;  //all writes allowed
      }
    }
    break;

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO

#else//defined(SSE_ACIA_383) ?
    case 0xfffc00:{  //--------------------------------------- ACIAs
/*
          MC6850

          ff fc00                   |xxxxxxxx|   Keyboard ACIA Control
          ff fc02                   |xxxxxxxx|   Keyboard ACIA Data

          ff fc04                   |xxxxxxxx|   MIDI ACIA Control
          ff fc06                   |xxxxxxxx|   MIDI ACIA Data

*/
      // Bus "jam" when accessing ACIA
      // Only cause bus jam once per word
      DEBUG_ONLY( if (mode==STEM_MODE_CPU) )
      {
        if (io_word_access==0 || (addr & 1)==0)
        {
#if defined(SSE_ACIA)
          BYTE wait_states=6;
#if defined(SSE_CPU_E_CLOCK)
          if(OPTION_C1)
          {
            INSTRUCTION_TIME(wait_states);
            wait_states=M68000.SyncEClock(TM68000::ECLOCK_ACIA);
            INSTRUCTION_TIME(wait_states);
          }
          else
#endif
          {
/*  This is a part from Steem 3.2, but in ior.cpp.
    With this instead of the original part iow.cpp below,
    Spectrum 512 is fixed also without option 6301/ACIA.
*/
            wait_states+=(8000000-(ACT-shifter_cycle_base))%10;
            BUS_JAM_TIME(wait_states);
          }

#else //Steem 3.2
//          if (passed VBL or HBL point){
//            BUS_JAM_TIME(4);
//          }else{
//          int waitTable[10]={0,9,8,7,6,5,4,3,2,1};
//          BUS_JAM_TIME(waitTable[ABSOLUTE_CPU_TIME % 10]+6);
          int rel_cycle=ABSOLUTE_CPU_TIME-shifter_cycle_base;
          rel_cycle=8000000-rel_cycle;
          rel_cycle%=10;
          BUS_JAM_TIME(rel_cycle+6);
//          BUS_JAM_TIME(8);
#endif

        }
      }

      switch (addr){
    /******************** Keyboard ACIA ************************/


/*

CR  Control (write $FFFC00 on the ST)

$FFFC00|byte |Keyboard ACIA control             BIT 7 6 5 4 3 2 1 0|W
       |     |Rx Int enable (1 - enable) -----------' | | | | | | ||
       |     |Tx Interrupts                           | | | | | | ||
       |     |00 - RTS low, Tx int disable -----------+-+ | | | | ||
       |     |01 - RTS low, Tx int enable ------------+-+ | | | | ||
       |     |10 - RTS high, Tx int disable ----------+-+ | | | | ||
       |     |11 - RTS low, Tx int disable,           | | | | | | ||
       |     |     Tx a break onto data out ----------+-' | | | | ||
       |     |Settings                                    | | | | ||
       |     |000 - 7 bit, even, 2 stop bit --------------+-+-+ | ||
       |     |001 - 7 bit, odd, 2 stop bit ---------------+-+-+ | ||
       |     |010 - 7 bit, even, 1 stop bit --------------+-+-+ | ||
       |     |011 - 7 bit, odd, 1 stop bit ---------------+-+-+ | ||
       |     |100 - 8 bit, 2 stop bit --------------------+-+-+ | ||
       |     |101 - 8 bit, 1 stop bit --------------------+-+-+ | ||
       |     |110 - 8 bit, even, 1 stop bit --------------+-+-+ | ||
       |     |111 - 8 bit, odd, 1 stop bit ---------------+-+-' | ||
       |     |Clock divide                                      | ||
       |     |00 - Normal --------------------------------------+-+|
       |     |01 - Div by 16 -----------------------------------+-+|
       |     |10 - Div by 64 -----------------------------------+-+|
       |     |11 - Master reset --------------------------------+-'|


$FFFC00|byte |Keyboard ACIA status              BIT 7 6 5 4 3 2 1 0|R
       |     |Interrupt request --------------------' | | | | | | ||
       |     |Parity error ---------------------------' | | | | | ||
       |     |Rx overrun -------------------------------' | | | | ||
       |     |Framing error ------------------------------' | | | ||
       |     |CTS ------------------------------------------' | | ||
       |     |DCD --------------------------------------------' | ||
       |     |Tx data register empty ---------------------------' ||
       |     |Rx data register full ------------------------------'|

*/

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IKBD//SS

      case 0xfffc00:  //control //SS writing ACIA IKBD control register

#if defined(SSE_ACIA_REGISTERS)
        if(OPTION_C1)
          ACIA_IKBD.CR=io_src_b; // assign before we send to other functions
#endif //... and we run the same functions in 6301 mode too:
        if ((io_src_b & 3)==3){
          log_to(LOGSECTION_IKBD,Str("IKBD: ")+HEXSl(old_pc,6)+" - ACIA reset"); 
          ACIA_Reset(NUM_ACIA_IKBD,0); // SS = 'Master reset'
        }else{
          // TOS sends 96: irq rx, 8bit1stop, div64
          ACIA_SetControl(NUM_ACIA_IKBD,io_src_b);
        }
        break;

      case 0xfffc02:  // data //SS sending data to HD6301

#if defined(SSE_IKBD_6301)
#if defined(SSE_ACIA_380)
        ACIA_IKBD.TDR=io_src_b;
#endif
        if(OPTION_C1)
        {
          //TRACE_LOG("%d %d %d PC %X CPU $%X -> ACIA IKDB TDR (SR $%X)\n",TIMING_INFO,old_pc,io_src_b,ACIA_IKBD.SR);
          TRACE_LOG("%d %d %d PC %x ACIA TDR %X\n",TIMING_INFO,old_pc,io_src_b);

/*  Effect of write on status register.
    The 'Tx data register empty' (TDRE) bit is cleared: register isn't empty.
    Writing on ACIA TDR clears the IRQ bit if IRQ for transmission
    is enabled. Eg Hades Nebula
*/
          ACIA_IKBD.SR&=~BIT_1; // clear TDRE bit
          if(ACIA_IKBD.IrqForTx())
            ACIA_IKBD.SR&=~BIT_7; // clear IRQ bit
          
          //update in MFP (if needs be)
          mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,
            !((ACIA_IKBD.SR&BIT_7) || (ACIA_MIDI.SR&BIT_7)));


/*  If the line is free, the byte in TDR is copied almost at once into the 
    shifting register, and TDR is free again.
    When TDR is free, status bit TDRE is set, and Steem's tx_flag is false!
    (hard to follow)
    v3.5.2:
    If the ACIA is shifting and already has a byte in TDR, the byte in TDR
    can be changed (High Fidelity Dreams).
    We record the timing of 'tx' only if the line is free: Pandemonium Demos.

    Brattacas "should" "work" in 6301 true emu mode.

    v3.6 Grumbler by Electricity: key repeat in 6301 true emu mode

*/

          if(!ACIA_IKBD.LineTxBusy
#if defined(SSE_ACIA_TDR_COPY_DELAY) // for Grumbler
            || ACT-ACIA_IKBD.last_tx_write_time<ACIA_TDR_COPY_DELAY
#endif
            )
          {
            if(ACIA_IKBD.LineTxBusy)//?
              agenda_delete(agenda_ikbd_process);//cancel previous one...

#if defined(SSE_ACIA_TDR_COPY_DELAY)
            else  // Pandemonium!
              ACIA_IKBD.last_tx_write_time=ABSOLUTE_CPU_TIME;
#endif
            HD6301.ReceiveByte(io_src_b);
            ACIA_IKBD.SR|=BIT_1; // TDRE free (see ior: can be "cancelled")
            // rare case when IRQ is enabled for transmit
            if( (ACIA_IKBD.CR&BIT_5)&&!(ACIA_IKBD.CR&BIT_6))
            {
              TRACE_LOG("ACIA IKBD TX IRQ\n");            
              ACIA_IKBD.SR|=BIT_7; 
              mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,0); //trigger
            }
          }
          else
          {
#if defined(SSE_DEBUG)
            if(!ACIA_IKBD.ByteWaitingTx) // TDR was free 
              TRACE_LOG("ACIA IKBD byte waiting $%X\n",io_src_b);
            else
              TRACE_LOG("ACIA IKBD new byte waiting $%X (instead of $%X)\n",io_src_b,ACIA_IKBD.TDR);
#endif
#if defined(SSE_ACIA_380)
#if defined(SSE_ACIA_TDR_COPY_DELAY) // for Grumbler
            if(ACT-ACIA_IKBD.last_tx_write_time<ACIA_TDR_COPY_DELAY)
              ACIA_IKBD.TDRS=ACIA_IKBD.TDR; // replaces
            else
#endif
#else
            ACIA_IKBD.TDR=io_src_b; // replaces
#endif
            ACIA_IKBD.ByteWaitingTx=true;
          }

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO//SS

          break;
        }
#endif // Steem 3.2
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
        }
        break;

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





MIDI beat clock defines the following real time messages:
 clock (decimal 248, hex 0xF8)
 start (decimal 250, hex 0xFA)
 continue (decimal 251, hex 0xFB)
 stop (decimal 252, hex 0xFC)

system exclusive start and end messages (F0 and F7).


*/

#undef LOGSECTION
#define LOGSECTION LOGSECTION_MIDI//SS

      case 0xfffc04:  //control

#if defined(SSE_ACIA_REGISTERS)
        ACIA_MIDI.CR=io_src_b; 
#endif
        if ((io_src_b & 3)==3){ // Reset
          log_to(LOGSECTION_IKBD,Str("MIDI: ")+HEXSl(old_pc,6)+" - ACIA reset");
          ACIA_Reset(NUM_ACIA_MIDI,0);
        }else{
          TRACE_LOG("%X -> ACIA MIDI CR\n",io_src_b); 
          // TOS sends 95: irq rx, 8bit1stop, div16
          ACIA_SetControl(NUM_ACIA_MIDI,io_src_b); 
        }
        break;

      case 0xfffc06:  //data
      {
#if defined(SSE_ACIA_REGISTERS)
        if(OPTION_C1)
        {
          if( (ACIA_MIDI.CR&BIT_5)&&!(ACIA_MIDI.CR&BIT_6) )// IRQ transmit enabled
            ACIA_MIDI.SR&=~BIT_7; // clear IRQ bit
          ASSERT( ACIA_MIDI.SR&2 ); //asserts?
          ACIA_MIDI.SR&=~BIT_1; // clear TDRE bit

        //update in MFP (if needs be)
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,
          !((ACIA_IKBD.SR&BIT_7) || (ACIA_MIDI.SR&BIT_7)));
        }
#endif
        bool TXEmptyAgenda=(agenda_get_queue_pos(agenda_acia_tx_delay_MIDI)
#if defined(SSE_MIDI)
          <0
#else
          >=0
#endif
          );

#if defined(SSE_ACIA_MIDI_SR02_CYCLES)
        if(OPTION_C1)
        TXEmptyAgenda=(abs(ACT-ACIA_MIDI.last_tx_write_time)
          >=ACIA_MIDI_OUT_CYCLES);
#endif

#if defined(SSE_DEBUG)
        if(TXEmptyAgenda)
        {
#if defined(SSE_MIDI_TRACE_BYTES_OUT)
          TRACE_LOG("%X -> ACIA MIDI TDR\n",io_src_b);
#endif
        }
        else
        {
#if defined(SSE_MIDI_TRACE_BYTES_OUT_OVR)
          TRACE_LOG("ACIA MIDI TX OVR, can't send %X?\n",io_src_b);
#endif
        }
#endif

        if (TXEmptyAgenda
#if !defined(SSE_MIDI)
          ==0
#endif
          ){
#if defined(SSE_IKBD_6301) //what was it?
          if(!OPTION_C1)
#endif
          if (ACIA_MIDI.tx_irq_enabled){
            ACIA_MIDI.irq=false;
            mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
          }

#if defined(SSE_MIDI)
#if defined(SSE_ACIA_MIDI_SR02_CYCLES)
          ACIA_MIDI.last_tx_write_time=ACT; // record timing
          if(!OPTION_C1) //beta bugfix, was if(OPTION_C1)...
#endif
          agenda_add(agenda_acia_tx_delay_MIDI,ACIAClockToHBLS(ACIA_MIDI.clock_divide),0);
          MIDIPort.OutputByte(io_src_b); //SS timing?
#else
          agenda_add(agenda_acia_tx_delay_MIDI,2 /*ACIAClockToHBLS(ACIA_MIDI.clock_divide)*/,0);
#endif

        }
        ACIA_MIDI.tx_flag=true;  //flag for transmitting

#if !defined(SSE_MIDI)
        MIDIPort.OutputByte(io_src_b); //SS timing?
#endif
        break;
      }

    //-------------------------- unrecognised -------------------------------------------------
      default:
        break;  //all writes allowed
      }
    }
    break;

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO//SS

#endif//defined(SSE_ACIA_383) 

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

#if defined(SSE_INT_MFP)
/*    If we come here while a write is pending, it should mean that we're at
      least 4 cycles later, so there's been some delay.
      We have no choice anyway, registers must be correct at the end of the
      day and we keep no buffer/agenda for multiple writes.
*/
            if(OPTION_C2)
            {
              if(MC68901.WritePending)
              {
                TRACE_MFP("IOW Flush MFP event ");
                event_mfp_write(); //flush
              }
              MC68901.LastRegisterFormerValue=mfp_reg[n];
              MC68901.LastRegisterWrittenValue=io_src_b;
            }
#endif
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
#if defined(SSE_INT_MFP_TIMER_B_AER) // maybe Timer B's AER bit changed?
                ASSERT(n==MFPR_AER || n==MFPR_DDR);
                if(OPTION_C2)
                  CALC_CYCLES_FROM_HBL_TO_TIMER_B(shifter_freq); //update
#endif
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
#if defined(SSE_INT_MFP)
                    if(OPTION_C2) 
                      mfp_set_pending(irq,ACT); // update timing, Irq...
                    else
#endif                      
                    // Don't need to call set_pending routine here as this can never
                    // happen soon after an interrupt
                    mfp_reg[MFPR_IPRA+mfp_interrupt_i_ab(irq)]|=mfp_interrupt_i_bit(irq);
                  }
                }
              }
            }else if (n>=MFPR_IERA && n<=MFPR_IERB){ //enable

              
//              TRACE_MFP("F%d y%d c%d PC %X MFP IER%c %X\n",TIMING_INFO,old_pc,'A'+n-MFPR_IERA,io_src_b);

              TRACE_MFP("%d PC %X MFP IER%c %X -> %X\n",ABSOLUTE_CPU_TIME,old_pc,'A'+n-MFPR_IERA,mfp_reg[n],io_src_b);

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
#if defined(SSE_VAR_REWRITE_380)
              for (int n=0;n<4;n++){ // we need to save n now
#else
              for (n=0;n<4;n++){
#endif
                bool new_enabled=(mfp_interrupt_enabled[mfp_timer_irq[n]] && (mfp_get_timer_control_register(n) & 7));
                if (new_enabled && mfp_timer_enabled[n]==0){
                  // Timer should have been running but isn't, must put into future
                  int stage=(mfp_timer_timeout[n]-ABSOLUTE_CPU_TIME);
                  //TRACE_MFP("F%d y%d c%d PC %X enable Timer %C stage %d",TIMING_INFO,old_pc,'A'+n,stage);
                  if (stage<=0){
                    stage+=((-stage/mfp_timer_period[n])+1)*mfp_timer_period[n];
                  }else{
                    stage%=mfp_timer_period[n];
                  }
                  mfp_timer_timeout[n]=ABSOLUTE_CPU_TIME+stage;
                  //TRACE_MFP(":%d\n",stage);
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
#ifdef SSE_DEBUG
            if (n>=MFPR_IPRA && n<=MFPR_IPRB)
              TRACE_MFP("%d PC %X MFP IPR%c %X -> %X\n",ACT,old_pc,'A'+n-MFPR_IPRA,mfp_reg[n],mfp_reg[n]&io_src_b);
            else
              TRACE_MFP("%d PC %X MFP ISR%c %X -> %X\n",ACT,old_pc,'A'+n-MFPR_ISRA,mfp_reg[n],mfp_reg[n]&io_src_b);
#endif
              mfp_reg[n]&=io_src_b;
            }else if (n>=MFPR_TADR && n<=MFPR_TDDR){ //have to set counter as well as data register
//              TRACE_MFP("F%d y%d c%d PC %X MFP T%cDR %X\n",TIMING_INFO,old_pc,'A'+n-MFPR_TADR,io_src_b);
              TRACE_MFP("%d PC %X MFP T%cDR %X -> %X\n",ABSOLUTE_CPU_TIME,old_pc,'A'+n-MFPR_TADR,mfp_reg[n],io_src_b);
              mfp_set_timer_reg(n,mfp_reg[n],io_src_b);
              mfp_reg[n]=io_src_b;
            }else if (n==MFPR_TACR || n==MFPR_TBCR){ //wipe low-bit on set
//              TRACE_MFP("F%d y%d c%d PC %X MFP T%cCR %X\n",TIMING_INFO,old_pc,'A'+n-MFPR_TACR,mfp_reg[n],io_src_b);
              TRACE_MFP("%d PC %X MFP T%cCR %X -> %X\n",ABSOLUTE_CPU_TIME,old_pc,'A'+n-MFPR_TACR,mfp_reg[n],io_src_b);
              io_src_b &= BYTE(0xf);
              mfp_set_timer_reg(n,mfp_reg[n],io_src_b);
              mfp_reg[n]=io_src_b;
            }else if (n==MFPR_TCDCR){
//              TRACE_MFP("F%d y%d c%d PC %X MFP TCDCR %X\n",TIMING_INFO,old_pc,io_src_b);
              TRACE_MFP("%d PC %X MFP TCDCR %X -> %X\n",ABSOLUTE_CPU_TIME,old_pc,mfp_reg[n],io_src_b);
              io_src_b&=BYTE(b01110111);
              mfp_set_timer_reg(n,mfp_reg[n],io_src_b);
              mfp_reg[n]=io_src_b;
            }else if (n==MFPR_VR){
              TRACE_MFP("%d PC %X MFP VR %X -> %X\n",ABSOLUTE_CPU_TIME,old_pc,mfp_reg[n],io_src_b);
#if defined(SSE_VAR_OPT_380)
              mfp_reg[n]=io_src_b;
#else
              mfp_reg[MFPR_VR]=io_src_b;
#endif
              if (!MFP_S_BIT){ //SS clearing this bit clears In Service registers
                mfp_reg[MFPR_ISRA]=0;
                mfp_reg[MFPR_ISRB]=0;
              }
            }else if (n>=MFPR_SCR){
              RS232_WriteReg(n,io_src_b);
            }else{
              ASSERT(n!=16);
#ifdef SSE_DEBUG
              if (n>=MFPR_IMRA && n<=MFPR_IMRB)
                TRACE_MFP("%d PC %X MFP IMR%c %X -> %X (IER%c %X)\n",ACT,old_pc,'A'+n-MFPR_IMRA, mfp_reg[n],io_src_b,'A'+n-MFPR_IMRA,mfp_reg[MFPR_IERA+n-MFPR_IMRA]);
#endif
              mfp_reg[n]=io_src_b;
            }

#if defined(SSE_INT_MFP)
/*  We execute the write now, but we'll update the register value after a
    while. We could do otherwise (delay everything), it was just faster that
    way.
*/            if(OPTION_C2)
            {
              MC68901.WriteTiming=ACT;
              MC68901.LastRegisterWritten=n;
              MC68901.LastRegisterWrittenValue=mfp_reg[n]; // in case there was a mask
              mfp_reg[n]=MC68901.LastRegisterFormerValue; // revert write for a while
              ASSERT(!MC68901.WritePending);
              bool prepare_event=(time_of_next_event==time_of_event_mfp_write);
              time_of_event_mfp_write=MC68901.WriteTiming+MFP_WRITE_LATENCY;
              if(prepare_event)
                prepare_next_event(); // for SPURIOUS.TOS
              MC68901.WritePending=true;
//              TRACE_MFP("plan event to write %X on reg %d at %d\n",MC68901.LastRegisterWrittenValue,n,time_of_event_mfp_write);
              TRACE_MFP("plan event to write %X on %s at %d\n",MC68901.LastRegisterWrittenValue,mfp_reg_name[n],time_of_event_mfp_write);
            }
#endif
            // The MFP doesn't update for about 8 cycles, so we should execute the next
            // instruction before causing any interrupts
            //SS this seems suspicious but it is actually needed: Super Hang-On TODO
            ioaccess=old_ioaccess;
#if defined(SSE_INT_MFP)
            if(OPTION_C2)
                ioaccess|=IOACCESS_FLAG_DELAY_MFP; // we manage delay another way
            else
#endif
            if ((ioaccess & (IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE | IOACCESS_FLAG_FOR_CHECK_INTRS |
                                IOACCESS_FLAG_DELAY_MFP))==0){
              ioaccess|=IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE;
            }
          }
        }else{ // even
          // Byte access causes bus error
          if (io_word_access==0) exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
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
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_BLITTER)
        FrameEvents.Add(scan_y,LINECYCLES,'B',((addr-0xff8a00)<<8)|io_src_b);
#endif
      Blitter_IO_WriteB(addr,io_src_b);
      break;

    case 0xff8900:      //----------------------------------- STE DMA Sound

#if defined(SSE_STF_DMA)
      if(ST_TYPE!=STE)
      {
        TRACE_LOG("STF write %X to DMA %X\n",io_src_b,addr);
       // crashing fixes PYM/ST-CNX, SoWatt, etc., which use this as test
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr); 
        break;
      }
#endif

#undef LOGSECTION
#define LOGSECTION LOGSECTION_SOUND

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



 The DMA Sound chip's mode 3 (repeat mode) ensures seamless linkage of frames, 
 because the start and end registers are actually double-buffered. When you 
 write to these registers, what you write really goes into a "holding area". 
 The contents of the holding area go into the true registers at the end of the
 current frame. (Actually, they go in when the chip is idle, which means right 
 away if the chip was idle to begin with.) 
 -> think it's OK in Steem ("next" dma sound)

*/
          case 0xff8903:   //HiByte of frame start address
          case 0xff8905:   //MidByte of frame start address
          case 0xff8907:   //LoByte of frame start address
            switch (addr & 0xf){
              case 0x3:
                next_dma_sound_start&=0x00ffff;
                next_dma_sound_start|=io_src_b << 16;
                //next_dma_sound_start=(io_src_b << 16);
                break;
              case 0x5:
                next_dma_sound_start&=0xff00ff;
                next_dma_sound_start|=io_src_b << 8;
                break;
              case 0x7:
#if defined(SSE_SOUND)
                io_src_b&=0xFE;
#endif
                next_dma_sound_start&=0xffff00;
                next_dma_sound_start|=io_src_b;
                break;
            }
            TRACE_LOG("DMA frame start %X\n",next_dma_sound_start);
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
                next_dma_sound_end&=0x00ffff;
                next_dma_sound_end|=io_src_b << 16;break;
              case 0x1:next_dma_sound_end&=0xff00ff;next_dma_sound_end|=io_src_b << 8;break;
              case 0x3:
#if defined(SSE_SOUND)
                io_src_b&=0xFE;
#endif
                next_dma_sound_end&=0xffff00;next_dma_sound_end|=io_src_b;break;
            }
            //TRACE_LOG("DMA frame end %X\n",next_dma_sound_start);//imbecile
            TRACE_LOG("PC %X DMA frame end %X\n",old_pc,next_dma_sound_end);
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
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY)
/*

 "The mask register must be written before the data register. Sending commences 
 when the data register is written and takes approximately 16 psec. Subsequent 
 writes to the data and mask registers are blocked until sending is complete. "
 Just a hack for now.
 Bugfix v3.7, negative values if some minutes into emulation. eg Antiques.
*/
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY_B)
            if(OPTION_HACKS 
              && abs(ACT-MicroWire_StartTime) <MICROWIRE_LATENCY_CYCLES) 
#else
            if(ACT-MicroWire_StartTime<MICROWIRE_LATENCY_CYCLES) 
#endif

            {
              TRACE_LOG("Microwire write %X at %X denied, %d cycles after write\n",io_src_b,addr,ACT-MicroWire_StartTime);
              break; // fixes XMas 2004 scroller
            }
#endif
            MicroWire_Data=MAKEWORD(LOBYTE(MicroWire_Data),io_src_b);
            break;
          case 0xff8923: // Set low byte of MicroWire_Data
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY)
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY_B)
            if(OPTION_HACKS 
              && abs(ACT-MicroWire_StartTime) <MICROWIRE_LATENCY_CYCLES) 
#else
            if(ACT-MicroWire_StartTime<MICROWIRE_LATENCY_CYCLES) 
#endif
            {
              TRACE_LOG("Microwire write %X at %X denied, %d cycles after write\n",io_src_b,addr,ACT-MicroWire_StartTime);
              break;
            }
#endif
          {

            MicroWire_Data=MAKEWORD(io_src_b,HIBYTE(MicroWire_Data));
            MicroWire_StartTime=ABSOLUTE_CPU_TIME;
            int dat=MicroWire_Data & MicroWire_Mask;
            int b;
            for (b=15;b>=10;b--){
              if (MicroWire_Mask & (1 << b)
#if defined(SSE_SOUND_MICROWIRE_MASK1)                 
                && (MicroWire_Mask & (1 << (b-1))) // both mask bits must be set //383 added ()
#endif
                ){
                if ((dat & (1 << b)) && (dat & (1 << (b-1)))==0
                  ){  //DMA Sound Address
                  int dat_b=b-2;

#if defined(SSE_SOUND_MICROWIRE_MASK2)
/*  Atari Doc says:
 The MICROWIRE™ bus is a three wire serial connection and protocol designed
 to allow multiple devices to be individually addressed by the controller. 
 The length of the serial data stream depends on the destination device. 
 In general, the stream consists of N bits of address, followed by zero or 
 more don't care hits, followed by M bits of data. The hardware interface 
 provided consists of two 16 bit read/write registers: one data register 
 which contains the actual bit stream to be shifted out, and one mask register
 which indicates which bits are valid. Let's consider a mythical device which 
 requires two address bits and one data bit. For this device the total bit 
 stream is three bits (minimum). Any three bits of the register pair may be 
 used. However, since the most significant bit is shifted first, the command
 will be received by the device soonest if the three most significant bits 
 are used. Let's assume: 01 is the device's address, D is the data to be 
 written, and X's are don't cares. Then all of the following register
 combinations will provide the same information to the device. 
1110 0000 0000 0000 Mask
01DX XXXX XXXX XXXX Data

0000 0000 0000 0111 Mask
XXXX XXXX XXXX X01D Data

0000 0001 1100 0000 Mask
XXXX XXX0 1DXX XXXX Data

0000 1100 0001 0000 Mask
XXXX 01XX XXXD XXXX Data

1100 0000 0000 0001 Mask
01XX XXXX XXXX XXXD Data

 As you can see, the address bits must be contiguous, and so must the 
 data bits, but they don't have to be contiguous with each other. 

->
This description is conform to the LMC datasheet.
But demo Pacemaker has some zeroes between 'address' and 'data',
and apparently this makes the command invalid, so all bits must be
contiguous.
If option 'Microwire' isn't checked, the Steem 3.2 way of skipping
don't cares to get to data is used.
*/
                  if(OPTION_MICROWIRE && (MicroWire_Mask & (1 << dat_b)) == 0)
                  {
                    TRACE_LOG("Microwire reject mask %X\n",MicroWire_Mask);
                    return;
                  } else
#endif
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
#if defined(SSE_SOUND_MICROWIRE) && !defined(SSE_SOUND_DMA_383A)
                      dma_sound_l_top_val=128;
                      dma_sound_r_top_val=128;
#else // we keep Steem 3.2 code, but will filter further if necessary
                      long double lv,rv,mv;
                      lv=dma_sound_l_volume;lv=lv*lv*lv*lv;
                      lv/=(20.0*20.0*20.0*20.0);
                      rv=dma_sound_r_volume;rv=rv*rv*rv*rv;
                      rv/=(20.0*20.0*20.0*20.0);
                      mv=dma_sound_volume;  mv=mv*mv*mv*mv*mv*mv*mv*mv;
                      mv/=(40.0*40.0*40.0*40.0*40.0*40.0*40.0*40.0);
                      // lv rv and mv are numbers between 0 and 1
                      dma_sound_l_top_val=BYTE(128.0*lv*mv);
                      dma_sound_r_top_val=BYTE(128.0*rv*mv);
#endif
                      TRACE_LOG("Microwire volume:  master %d L %d R %d\n",dma_sound_volume,dma_sound_l_volume,dma_sound_r_volume);
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
Sound not correctly emulated
*/
#if defined(SSE_SOUND_MICROWIRE)
                      TRACE_LOG("DMA snd Treble $%X\n",io_src_b); 
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
#if defined(SSE_SOUND_MICROWIRE)
                      TRACE_LOG("DMA snd Bass $%X\n",io_src_b); 
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
                      TRACE_LOG("STE SND mixer %X\n",dat);
//                      ASSERT(dat&3); // Again, Pacemaker
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
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY)
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY_B)
            if(OPTION_HACKS 
              && abs(ACT-MicroWire_StartTime) <MICROWIRE_LATENCY_CYCLES) 
#else
            if(ACT-MicroWire_StartTime<MICROWIRE_LATENCY_CYCLES) 
#endif
            {
              TRACE_LOG("Microwire write %X at %X denied, %d cycles after write\n",io_src_b,addr,ACT-MicroWire_StartTime);
              break;
            }
#endif
            MicroWire_Mask=MAKEWORD(LOBYTE(MicroWire_Mask),io_src_b);
//            TRACE_LOG("MicroWire Mask has become %x\n",MicroWire_Mask); //yeah, how useful
            break;
          case 0xff8925:  // Set low byte of MicroWire_Mask
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY)
#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY_B)
            if(OPTION_HACKS 
              && abs(ACT-MicroWire_StartTime) <MICROWIRE_LATENCY_CYCLES) 
#else
            if(ACT-MicroWire_StartTime<MICROWIRE_LATENCY_CYCLES) 
#endif
            {
              TRACE_LOG("Microwire write %X at %X denied, %d cycles after write\n",io_src_b,addr,ACT-MicroWire_StartTime);
              break;// fixes XMas 2004 scroller
            }
#endif
            MicroWire_Mask=MAKEWORD(io_src_b,HIBYTE(MicroWire_Mask));
            break;
          case 0xFF8902:
          case 0xFF890E:
          case 0xFF8920:
            //ASSERT(!io_src_b);//Antiques
            break;
          default:
            TRACE_LOG("STE SND %X %X\n",addr,io_src_b);
        }
      }
      break;



#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO


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
#if defined(SSE_YM2149_NO_JAM_IF_NOT_RW)
#if defined(SSE_YM2149_BUS_JAM_383B)
      if ((addr & 1) && io_word_access) 
#else
      if (OPTION_HACKS && (addr & 1) && io_word_access)
#endif
        break; //odd addresses ignored on word writes, don't jam
#endif

#if defined(SSE_YM2149_BUS_JAM_383)
/*  ijor: 

Essentially, the main point is that the 68000 doesn't constrain the wait states
to be an even number of cycles.

Instructions take always an even number of cycles, but this is only the minimum.
Every microcode block that completes a bus cycle can be extended, for any number 
of cycles while wait states are generated by the system. 

So if the system inserts, say 3 wait states, a bus cycles that normally would 
take 4 cycles would take 7. And after that bus cycle, the cpu will continue 
executing from odd cycles,at least until more wait cycles change the alignment
once again.

In the case of the ST, GLUE inserts just a single wait state when accessing the
PSG audio chip. This would result in bus cycles accessing the PSG to take 5 
clock cycles. And hence the next bus cycle will start at an odd cycle. 

Of course, if the next cycle happens to be at main RAM (or Shifter), such 
as when performing a prefetch, then MMU would align the CPU back to a boundary 
of 4 cycles.

But any bus cycle(s) performed in between, after a PSG access, and before any 
RAM access (including prefetch), will be executed at an odd cycle!

http://www.atari-forum.com/viewtopic.php?f=16&t=30575
*/
      DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) INSTRUCTION_TIME(1); // much simpler
#else
      if ((ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_W)==0){
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) BUS_JAM_TIME(4);
//        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) BUS_JAM_TIME(2);//
//        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) INSTRUCTION_TIME(2);//
        ioaccess|=IOACCESS_FLAG_PSG_BUS_JAM_W;
      }
#endif
#if !defined(SSE_YM2149_BUS_JAM_383B)
      if ((addr & 1) && io_word_access) break; //odd addresses ignored on word writes
#endif
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
/*
    The PSG ports are used for other purposes than chip sound.

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
    
    Floppy drive & side is selected if the bit is cleared, not set!
    Bit 0: side
    Bit 1: drive A
    Bit 2: drive B
    If both bits 1&2 are cleared, drive A is selected.
*/

#if USE_PASTI // SS Pasti manages this as well
/*
  if ((psg_reg[PSGR_PORT_A] & BIT_1)==0){ // Drive A
    return 0;
  }else if ((psg_reg[PSGR_PORT_A] & BIT_2)==0){ // Drive B
    return 1;
  }
*/
          if (hPasti && pasti_active) 
            pasti->WritePorta(io_src_b,ABSOLUTE_CPU_TIME);
#endif//pasti
#if defined(SSE_FDC_INDEX_PULSE_COUNTER) && defined(SSE_DEBUG) && defined(SSE_YM2149_OBJECT)
/* Symic Demo TODO right or wrong?
*/
          if(WD1772.IndexCounter && YM2149.Drive()==TYM2149::NO_VALID_DRIVE)
            TRACE_FDC("drive deselect while IP counter %d\n",WD1772.IndexCounter);
#endif
#if defined(SSE_DISK_CAPS)
          if(Caps.Active) // like the above (we imitate!)
            Caps.WritePsgA(io_src_b);
#endif
          SerialPort.SetDTR(io_src_b & BIT_4);
          SerialPort.SetRTS(io_src_b & BIT_3);
          if ((old_val & (BIT_1+BIT_2))!=(io_src_b & (BIT_1+BIT_2))){

#if defined(SSE_YM2149A)
            // TODO not reliable
            YM2149.SelectedDrive=floppy_current_drive();
            YM2149.SelectedSide=floppy_current_side();
#endif


#if defined(SSE_YM2149C)
/*  turn on/off motor in drives
    Fixes Delirious 3 STW and Realm of the Trolls STX init STW disc.
*/
            SF314[0].Motor( YM2149.SelectedDrive==0 && (fdc_str&0x80));
            SF314[1].Motor( YM2149.SelectedDrive==1 && (fdc_str&0x80));
#elif defined(SSE_YM2149B)
            if(YM2149.Drive()!=TYM2149::NO_VALID_DRIVE)
              SF314[YM2149.SelectedDrive].State.motor=!!(fdc_str&0x80);
#endif


#if !defined(SSE_DEBUG_TRACE_IDE) && defined(SSE_YM2149A)
#if defined(SSE_YM2149_REPORT_DRIVE_CHANGE)||defined(SSE_BOILER_TRACE_CONTROL)
#if defined(SSE_BOILER_TRACE_CONTROL) // controlled by boiler now (3.6.1)
            if(TRACE_MASK3 & TRACE_CONTROL_FDCPSG)
#endif
              TRACE_FDC("PC %X PSG %X -> %c%d:\n",old_pc,io_src_b,'A'+YM2149.SelectedDrive,YM2149.SelectedSide);
#endif
#endif//#ifndef SSE_DEBUG_TRACE_IDE
#ifdef ENABLE_LOGFILE
            if ((psg_reg[PSGR_PORT_A] & BIT_1)==0){ //drive 0
              log_to_section(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set current drive to A:");
            }else if ((psg_reg[PSGR_PORT_A] & BIT_2)==0){ //drive 1
              log_to_section(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set current drive to B:");
            }else{                             //who knows?
              log_to_section(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Unset current drive - guess A:");
            }
#endif
#if !defined(SSE_OSD_DRIVE_LED3)
            // disk_light_off_time can only get this far in the future when using pasti
            if (int(disk_light_off_time)-int(timer) < 1000*10){
              disk_light_off_time=timer+DisableDiskLightAfter;
            }
#endif
          }
        }else if (psg_reg_select==PSGR_PORT_B){
#if defined(SSE_DONGLE_PROSOUND)
/*  Wings of Death, Lethal Xcess could use the Pro Sound Centronics adapter
    to play 8bit samples on the STF.
*/
          if(DONGLE_ID==TDongle::PROSOUND)
            dma_mv16_fetch(io_src_b<<3); // <<3 again in that function
          else
#endif
          if (ParallelPort.IsOpen()){
            if (ParallelPort.OutputByte(io_src_b)==0){
              log_write("ARRRGGHH: Lost printer character, printer not responding!!!!");
              BRK( printer char lost );
              //TRACE("ARRRGGHH: Lost printer character, printer not responding!!!!");
            }
            UpdateCentronicsBusyBit();
          }
        }else if (psg_reg_select==PSGR_MIXER){
          UpdateCentronicsBusyBit();
        }
      }
      break;
    }
    case 0xff8600:{  //--------------------------------------- DMA / FDC

#if defined(SSE_DMA_OBJECT)
      Dma.IOWrite(addr,io_src_b);
      break;
#else // Steem 3.2
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
              case 2:
                if ((fdc_str & FDC_STR_BUSY)==0){
                  log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC track register to "+io_src_b);
                  fdc_tr=io_src_b;
                }else{
                  log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Can't set FDC track register to "+io_src_b+", FDC is busy");
                }
                break;
              case 4:
                if ((fdc_str & FDC_STR_BUSY)==0){
                  log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC sector register to "+io_src_b);
                  fdc_sr=io_src_b;
                }else{
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
#endif//!dma
    }

#if defined(SSE_VIDEO_CHIPSET)

    //////////////////////
    // Shifter-MMU-GLUE //
    //////////////////////

    case 0xff8200: {

#if defined(SSE_VAR_OPT_383A1)
      act=ACT;
#endif

      ASSERT( (addr&0xFFFF00)==0xff8200 );

#if defined(SSE_VIDEO_IOW_TRACE)  // not LOG
      TRACE("(%d %d/%d) Shifter write %X=%X\n",FRAME,scan_y,LINECYCLES,addr,io_src_b);
#endif  

      if ((addr>=0xff8210 && addr<0xff8240) || addr>=0xff8280){
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }
  
  /////////////
  // Palette // // SS word (long) writes far more frequent (see below)
  /////////////
  
      else if (addr>=0xff8240 && addr<0xff8260){  //palette

#if defined(SSE_MMU_ROUNDING_BUS2_IO)
        cpu_cycles&=-4;
#endif
#if defined(SSE_BLT_383B)
        Blit.BlitCycles=0;
#endif
        int n=(addr-0xff8240) >> 1; 

        // Writing byte to palette writes that byte to both the low and high byte!
        WORD new_pal=MAKEWORD(io_src_b,io_src_b & 0xf);

#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
        if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_PAL)
          FrameEvents.Add(scan_y,LINECYCLES,'p', (n<<12)|io_src_b);  // little p
#endif
        Shifter.SetPal(n,new_pal);
        log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Palette change at scan_y="+scan_y+" cycle "+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl));

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

According to ST-CNX, those registers are in the MMU, not in the Shifter.

 STE doc by Paranoid: for compatibility reasons, the low-byte
 of the Video Base Address is ALWAYS set to 0 when the mid- or high-byte of
 the Video Base Address are set. 
 E.g.: Leavin' Teramis

*/
     
        case 0xff8201:  //high byte of screen memory address
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
          if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
            FrameEvents.Add(scan_y,LINECYCLES,'V',io_src_b); 
#endif
/* "Later 3rd-party upgrade kits allow a maximum of 14MB w/Magnum-ST,
 bypassing the stock MMU with a replacement unit and the additional
 chips on a separate board fitting over it."
*/
          if (mem_len<=FOUR_MEGS
#if defined(SSE_TOS_GEMDOS_EM_382) && !defined(SSE_TOS_GEMDOS_EM_383)//oops
            && !extended_monitor
#endif
            ) 
            io_src_b&=b00111111;
          DWORD_B_2(&xbios2)=io_src_b;
#if defined(SSE_STF_VBASELO)
          if(ST_TYPE==STE) 
#endif
#if defined(SSE_TOS_GEMDOS_EM_383)
            if(!extended_monitor)
#endif
              DWORD_B_0(&xbios2)=0; 
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;

        case 0xff8203:  //mid byte of screen memory address
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
          if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
            FrameEvents.Add(scan_y,LINECYCLES,'M',io_src_b); 
#endif
#if defined(SSE_TOS_GEMDOS_EM_382)
          if(!extended_monitor)
#endif
            DWORD_B_1(&xbios2)=io_src_b;

#if defined(SSE_STF_VBASELO)
          if(ST_TYPE==STE) 
#endif
            DWORD_B_0(&xbios2)=0; 
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;

        case 0xff8205:  //high byte of draw pointer
        case 0xff8207:  //mid byte of draw pointer
        case 0xff8209:  //low byte of draw pointer

#if defined(SSE_VIDEO_CHIPSET)
          MMU.WriteVideoCounter(addr,io_src_b);
#ifdef SSE_GLUE_REFACTOR_OVERSCAN_EXTRA
          log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+" - Set Shifter draw pointer to "+
            HEXSl(MMU.VideoCounter,6)+" at "+scanline_cycle_log());
#endif
          break;
#else 
          {
            //          int srp=scanline_raster_position();
            int dst=ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl;
            dst-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
            dst+=16;dst&=-16;
            dst+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
#if defined(SSE_SHIFTER) // video defined but not SDP
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

            log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+" - Set Shifter draw pointer to "+
              HEXSl(shifter_draw_pointer,6)+" at "+scanline_cycle_log()+", aligned to "+dst);
            break;
          }
#endif

        case 0xff820a: //synchronization mode      
          Glue.SetSyncMode(io_src_b); 
          break;

/* 
VBASELO 
This register contains the low-order byte of the video display base address. 
It can be altered at any time and will affect the next display processor data 
fetch. it is recommended that the video display address be altered only during 
vertical and horizontal blanking or display garbage may result. 

STE only. That's why the low byte is separated from the high & mid bytes.
Writing on it on a STF does nothing.

Last bit always cleared (we must do it).
*/
        case 0xff820d:  //low byte of screen memory address
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
          if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
            FrameEvents.Add(scan_y,LINECYCLES,'v',io_src_b); 
#endif

#if defined(SSE_STF_VBASELO)
          if(ST_TYPE!=STE)
          {
            TRACE_VID("STF write %X to %X\n",io_src_b,addr);
            break; // fixes Lemmings 40; used by Beyond/Pax Plax Parallax
          }
#endif
          io_src_b&=~1;
          xbios2&=0xFFFF00;
          xbios2|=io_src_b;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;

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
  
FFFF820F  X X X X X X X X X                  no     yes
    
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
        case 0xff820f:   
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
          if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_HSCROLL)
            FrameEvents.Add(scan_y,LINECYCLES,'L',io_src_b); 
#endif

#if defined(SSE_STF_LINEWID)
          if(ST_TYPE!=STE)
          {
            TRACE_VID("STF write %X to %X\n",io_src_b,addr);
            break; // fixes Imagination/Roller Coaster mush
          }
#endif

#if defined(SSE_SHIFTER) 
          Shifter.Render(LINECYCLES,DISPATCHER_LINEWIDTH); // eg Beat Demo
#else
          draw_scanline_to(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl); // Update sdp if off right  
#endif

#if defined(SSE_MMU_LINEWID_TIMING)
          shifter_fetch_extra_words=io_src_b;
          if(LINECYCLES<Glue.CurrentScanline.EndCycle+MMU_PREFETCH_LATENCY)
            LINEWID=shifter_fetch_extra_words;
#else
          LINEWID=io_src_b;
#endif
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set shifter_fetch_extra_words to "+
            (shifter_fetch_extra_words)+" at "+scanline_cycle_log());
          break;

        case 0xff8260: //resolution
          Glue.SetShiftMode(io_src_b);
          break;

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
          // to 0 confuses the Shifter and causes it to shrink the left border by 16 pixels.
        case 0xff8265:  // Hscroll
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
          if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_HSCROLL)
            FrameEvents.Add(scan_y,LINECYCLES,(addr==0xff8264)?'h':'H',io_src_b); 
#endif
#if defined(SSE_STF_HSCROLL)
          if(ST_TYPE!=STE) 
          {
            TRACE_VID("STF write %X to %X\n",io_src_b,addr); //ST-CNX
            break; // fixes Hyperforce
          }
          else
#endif
          {
            int cycles_in=(int)(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
#if defined(SSE_SHIFTER_HSCROLL_380) && !defined(SSE_VS2008_WARNING_382)
            BYTE former_hscroll=HSCROLL;
#endif
            HSCROLL=io_src_b & 0xf; // limited to 4bit

            log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set horizontal scroll ("+HEXSl(addr,6)+
              ") to "+(shifter_hscroll)+" at "+scanline_cycle_log());
            if (addr==0xff8265) 
              shifter_hscroll_extra_fetch=(HSCROLL!=0);

#if defined(SSE_SHIFTER_HSCROLL_380)
/*  Better test, should new HSCROLL apply on current line
    TODO: what is exact threshold ?
*/
            if(cycles_in<=Glue.CurrentScanline.StartCycle+24) {
#elif defined(SSE_VID_BORDERS)
            if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-BORDER_SIDE){
#else
            if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-32){
#endif           // eg Coreflakes hidden screen //SS:which one?...
#if defined(SSE_SHIFTER_HSCROLL_380)
              Shifter.hscroll0=HSCROLL;
#endif
#if defined(SSE_SHIFTER_HSCROLL_380)
              {
#else
              if (left_border>0){ // Don't do this if left border removed!
#endif
                shifter_skip_raster_for_hscroll = (HSCROLL!=0);

#if defined(SSE_SHIFTER_HSCROLL_381) //argh!
#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
                Glue.AdaptScanlineValues(cycles_in); // ST Magazin
#endif
#ifndef SSE_VID_BORDERS
                if (left_border>=BORDER_SIDE) {//ux382
#else
                if (left_border>=SideBorderSize){ // Don't do this if left border removed!
#endif
#elif defined(SSE_SHIFTER_HSCROLL_380)
                if (left_border>0){ // Don't do this if left border removed!
#endif
                  left_border=BORDER_SIDE;
                  if (HSCROLL)
                    left_border+=16;
                  if(shifter_hscroll_extra_fetch) 
                    left_border-=16;
#if defined(SSE_SHIFTER_HSCROLL_380)
                }
#endif
#if defined(SSE_SHIFTER_HSCROLL_380) // update shifter_pixel for new HSCROLL
                shifter_pixel=HSCROLL; //fixes We Were STE distorter (party version)
#endif
              }
            }
          } 
          break;
        }//sw
      }//if     

      break;

#else // Steem 3.2

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
      if ((addr>=0xff8210 && addr<0xff8240) || addr>=0xff8280){
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }else if (addr>=0xff8240 && addr<0xff8260){  //palette
        int n=(addr-0xff8240) >> 1;
        // Writing byte to palette writes that byte to both the low and high byte!
        WORD new_pal=MAKEWORD(io_src_b,io_src_b & 0xf);
        if (STpal[n]!=new_pal){
          STpal[n]=new_pal;
          PAL_DPEEK(n*2)=new_pal;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Palette change at scan_y="+scan_y+" cycle "+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl));
          if (draw_lock) draw_scanline_to((ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl)+1);
          if (flashlight_flag==0 && draw_line_off==0 DEBUG_ONLY( && debug_cycle_colours==0) ){
            palette_convert(n);
          }
        }
      }else{
        switch(addr){
        case 0xff8201:  //high byte of screen memory address
          if (mem_len<=FOUR_MEGS) io_src_b&=b00111111;
          DWORD_B_2(&xbios2)=io_src_b;
          DWORD_B_0(&xbios2)=0;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;
        case 0xff8203:  //mid byte of screen memory address
          DWORD_B_1(&xbios2)=io_src_b;
          DWORD_B_0(&xbios2)=0;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;
        case 0xff8205:  //high byte of draw pointer
        case 0xff8207:  //mid byte of draw pointer
        case 0xff8209:  //low byte of draw pointer
        {
//          int srp=scanline_raster_position();
          int dst=ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl;
          dst-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
          dst+=16;dst&=-16;
          dst+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;

          draw_scanline_to(dst); // This makes shifter_draw_pointer up to date
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
        case 0xff820d:  //low byte of screen memory address
          DWORD_B_0(&xbios2)=io_src_b;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set screen base to "+HEXSl(xbios2,6));
          break;
        case 0xff820a:  //synchronization mode
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
        case 0xff820f:   //int shifter_fetch_extra_words;
          draw_scanline_to(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl); // Update sdp if off right
          shifter_fetch_extra_words=(BYTE)io_src_b;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set shifter_fetch_extra_words to "+
                    (shifter_fetch_extra_words)+" at "+scanline_cycle_log());
          break;
        case 0xff8264:  // Set hscroll and don't change line length
          // This is an odd register, when you change hscroll below to non-zero each
          // scanline becomes 4 words longer to allow for extra screen data. This register
          // sets hscroll but doesn't do that, instead the left border is increased by
          // 16 pixels. If you have got hscroll extra fetch turned on then setting this
          // to 0 confuses the shifter and causes it to shrink the left border by 16 pixels.
        case 0xff8265:  // Hscroll
        {
          int cycles_in=int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
/*
          int dst=cycles_in;
          dst-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
          dst+=15;dst&=-16;
          dst+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
*/
//          log_write(Str("draw_scanline_to(")+Str(dst)+")");

          draw_scanline_to(cycles_in); // Update sdp if off right
          shifter_pixel-=shifter_hscroll;
          shifter_hscroll=io_src_b & 0xf;
          shifter_pixel+=shifter_hscroll;

          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: ")+HEXSl(old_pc,6)+" - Set horizontal scroll ("+HEXSl(addr,6)+
                    ") to "+(shifter_hscroll)+" at "+scanline_cycle_log());
          if (addr==0xff8265) shifter_hscroll_extra_fetch=(shifter_hscroll!=0);

          if (cycles_in<=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-32){
            if (left_border>0){ // Don't do this if left border removed!
              shifter_skip_raster_for_hscroll = shifter_hscroll!=0;
              left_border=BORDER_SIDE;
              if (shifter_hscroll) left_border+=16;
              if (shifter_hscroll_extra_fetch) left_border-=16;
            }
          }


          break;
        }
        case 0xff8260: //resolution
          if (screen_res>=2 || emudetect_falcon_mode!=EMUD_FALC_MODE_OFF) return;
#ifndef NO_CRAZY_MONITOR
          if (extended_monitor){
            screen_res=BYTE(io_src_b & 1);
            return;
          }
#endif
          io_src_b&=3;
          int cycles_in=int(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
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
        }
      }
      break;

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO
#endif
    }

    /////////
    // MMU //
    /////////
#undef LOGSECTION
#define LOGSECTION LOGSECTION_MMU
    case 0xff8000:  //--------------------------------------- memory
    {
      if (addr==0xff8001){ //Memory Configuration

#if defined(SSE_MMU_WRITE_MEM_CONF) 
        if(mem_len<=FOUR_MEGS){ // programs actually may write here
#else
        if (old_pc>=FOURTEEN_MEGS && mem_len<=FOUR_MEGS){
#endif
          mmu_memory_configuration=io_src_b;
          mmu_bank_length[0]=mmu_bank_length_from_config[(mmu_memory_configuration & b1100) >> 2];
          mmu_bank_length[1]=mmu_bank_length_from_config[(mmu_memory_configuration & b0011)];
//#if defined(SSE_BOILER_TRACE_CONTROL)
  //        if(TRACE_MASK_IO & TRACE_CONTROL_IO_MMU)
    //        TRACE_LOG("PC %X write %X to MMU (bank 0: %d bank 1: %d)\n",pc,io_src_b,
          TRACE_LOG("PC %X write %X to MMU (bank 0: %d bank 1: %d)\n",pc,io_src_b,
              mmu_bank_length[0]/1024, mmu_bank_length[1]/1024);
//#endif
#if !defined(SSE_MMU_NO_CONFUSION)
          mmu_confused=false;
          if (bank_length[0]) if (mmu_bank_length[0]!=bank_length[0]) mmu_confused=true;
          if (bank_length[1]) if (mmu_bank_length[1]!=bank_length[1]) mmu_confused=true;
#if defined(SSE_MMU_WRITE_MEM_CONF) && !defined(SSE_MMU_RAM_TEST2)
          if(old_pc<FOURTEEN_MEGS) // the write doesn't "confuse" the MMU
          {
//#if defined(SSE_BOILER_TRACE_CONTROL)
  //          if(TRACE_MASK_IO & TRACE_CONTROL_IO_MMU)
              TRACE_LOG("Cancel MMU testing\n");
//#endif
            mmu_confused=false; // fixes Super Neo Demo Show (1MB) (hack)
          }
#endif
#if !defined(SSE_MMU_RAM_TEST3)
          himem=(MEM_ADDRESS)(mmu_confused ? 0:mem_len);
#else
          himem=(MEM_ADDRESS)mem_len;
#endif
#else//?SSE_MMU_NO_CONFUSION
          himem=(MEM_ADDRESS)mem_len;
          int mmu_confused=0;//dbg
#endif
//#if defined(SSE_BOILER_TRACE_CONTROL)
  //        if(TRACE_MASK_IO & TRACE_CONTROL_IO_MMU)
    //        TRACE_LOG("MMU PC %X Byte %X RAM %dK Bank 0 %d Bank 1 %d confused %d\n",old_pc,io_src_b,mem_len/1024,bank_length[0]/1024,bank_length[1]/1024,mmu_confused);
          TRACE_LOG("MMU PC %X Byte %X RAM %dK Bank 0 %d Bank 1 %d confused %d\n",old_pc,io_src_b,mem_len/1024,bank_length[0]/1024,bank_length[1]/1024,mmu_confused);
//#endif
        }
      }else if (addr>0xff800f){
        exception(BOMBS_BUS_ERROR,EA_WRITE,addr);
      }
      break;
    }
#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO
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
#if defined(SSE_STF_PADDLES)
       if(ST_TYPE!=STE)
       {
         TRACE_LOG("STF write %X to %X\n",io_src_b,addr);
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
#if defined(SSE_DEBUG_TRACE_IO)
#if defined(SSE_BOILER_TRACE_CONTROL)
  if (((1<<15)&d2_dpeek(FAKE_IO_START+24))
  // we add conditions address range - logsection enabled

//      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_INTERRUPTS] ) //mfp
      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_MFP] ) //mfp
      && ( (addr&0xffff00)!=0xfffc00 || logsection_enabled[LOGSECTION_IKBD] ) //acia
      && ( (addr&0xffff00)!=0xff8600 || logsection_enabled[LOGSECTION_FDC] ) //dma
      && ( (addr&0xffff00)!=0xff8800 || logsection_enabled[LOGSECTION_SOUND] )//psg
      && ( (addr&0xffff00)!=0xff8900 || logsection_enabled[LOGSECTION_SOUND] )//dma
      && ( (addr&0xffff00)!=0xff8a00 || logsection_enabled[LOGSECTION_BLITTER] )
      && ( (addr&0xffff00)!=0xff8200 || logsection_enabled[LOGSECTION_VIDEO] )//shifter
      && ( (addr&0xffff00)!=0xff8000 || (((1<<13)&d2_dpeek(FAKE_IO_START+24)) ))//MMU

    ) 
#endif
  if (addr>=0xff8240 && addr<0xff8260)
    TRACE_LOG("PC %X write PAL %X to %X\n",pc-2,io_src_w,addr);
  else
    TRACE_LOG("PC %X write word %X to %X\n",pc-2,io_src_w,addr);
#endif

#if defined(SSE_CPU_DATABUS)
  dbus=io_src_w;
#endif

  if (addr>=0xff8240 && addr<0xff8260){  //palette

#if defined(SSE_MMU_ROUNDING_BUS2_IO)
    cpu_cycles&=-4;
#endif
#if defined(SSE_BLT_383B)
    Blit.BlitCycles=0;
#endif

    DEBUG_CHECK_WRITE_IO_W(addr,io_src_w);
    int n=(addr-0xff8240) >> 1;

//    TRACE("PC %X W PAL %X %X\n",old_pc,n,io_src_w);

#if defined(SSE_SHIFTER)

#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1&FRAME_REPORT_MASK_PAL) 
  {
    if(Blit.HasBus)
      FrameEvents.Add(scan_y,LINECYCLES,'Q',(n<<12)|io_src_w); 
    else
      FrameEvents.Add(scan_y,LINECYCLES,'P',(n<<12)|io_src_w); 
  }
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
//SS this is the way chosen by Steem authors, word accesses are treated byte 
//by byte
    io_word_access=true;
#if defined(DEBUG_BUILD) && defined(SSE_BOILER_MONITOR_IO_FIX1)
    DEBUG_CHECK_WRITE_IO_W(addr,io_src_w);
#endif
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
/*  SS same way for long accesses, so that a .L write will resolve in 4 .B writes.
    Notice the timing trick. At CPU emu level, the write is counted for eg 8 
    cycles, before coming here, the adjustment is here where it counts.
*/
  INSTRUCTION_TIME(-4);
  io_write_w(addr,HIWORD(io_src_l));
  INSTRUCTION_TIME(4);
  io_write_w(addr+2,LOWORD(io_src_l));
}

#undef LOGSECTION
