/*---------------------------------------------------------------------------
FILE: ior.cpp
MODULE: emu
DESCRIPTION: I/O address reads. This file contains crucial core functions
that deal with reads from ST I/O addresses ($ff8000 onwards).
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: ior.cpp")
#endif

#if defined(SSE_BUILD)
bool io_word_access=false;
#endif

#if defined(SSE_ACSI)
#include "harddiskman.decla.h"
#if defined(SSE_ACSI_TIMING) 
#include "SSE/SSEAcsi.h"
#endif
#endif

#define LOGSECTION LOGSECTION_IO

#if !defined(SSE_VIDEO_CHIPSET)

MEM_ADDRESS get_shifter_draw_pointer(int cycles_since_hbl)
{ 
  if (bad_drawing){
    // Fake SDP
    if (scan_y<0){
      return xbios2;
    }else if (scan_y<shifter_y){
      int line_len=(160/res_vertical_scale);
      return xbios2 + scan_y*line_len + min(cycles_since_hbl/2,line_len) & ~1;
    }else{
      return xbios2+32000;
    }
  }
  draw_check_border_removal();

  if (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line){
    int bytes_ahead=8;
    if (shifter_hscroll_extra_fetch) bytes_ahead=16;
    int starts_counting=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN/2 - bytes_ahead;
    int bytes_to_count=160; //160 bytes later

    if (left_border==0){ starts_counting-=26;bytes_to_count+=28; }

    if (right_border==0){
      bytes_to_count+=50; //44
    }else if (shifter_skip_raster_for_hscroll){
      bytes_to_count+=8;
    }
    if (overscan_add_extra<-60) bytes_to_count-=106; //big right border

    int c=cycles_since_hbl/2-starts_counting;
    MEM_ADDRESS sdp=shifter_draw_pointer_at_start_of_line;
    if (c>=bytes_to_count){
      sdp+=bytes_to_count+(shifter_fetch_extra_words*2);
    }else if (c>=0){
      c&=-2;
      sdp+=c;
    }
    return sdp;
  }else{
    return shifter_draw_pointer;
  }
}

#endif
//---------------------------------------------------------------------------

BYTE ASMCALL io_read_b(MEM_ADDRESS addr)
{
/*
  Allowed addresses

  000000 - 3FFFFF   RAM

  D00000 - D7FFFF
  E00000 - E3FFFF   TOS
  E40000 - EBFFFF

  FA0000 - FBFFFF   Cart

  FE0000 - FE1FFF

  FF8000 - FF800F   MMU
  FF8200 - FF820F   SHIFTER
  FF8240 - FF827F   pallette, res
  FF8608 - FF860F   FDC
  FF8800 - FF88FF   sound chip
  FF8900 - FF893F   DMA sound, microwire
  FF8A00 - FF8A3F   blitter
  FF9000 - FF9001   
  FF9201, FF9202, FF9203           paddles
  FF9211, FF9213, FF9215, FF9217   paddles
  FFFA01, odd addresses up to FFFA3F   MFP
  FFFC00 - FFFDFF   ACIA, realtime clock

  Word differences:

  FF8604 - FF860F   FDC
  FF9200, FF9202                   paddles
  FF9210, FF9212, FF9214, FF9216   paddles
  FF9220, FF9222                   paddles
  FFFA00 - FFFA3F   MFP
*/
  DEBUG_CHECK_READ_IO_B(addr);

#ifdef SSE_MMU_MONSTER_ALT_RAM_IO2
/*  Now we test the supervisor flag inside io_read and io_write functions
    because the MonSTer expansion doesn't protect address $FFFE00.W.
*/
    if(!SUPERFLAG && (addr&0xfffffe)!=0xfffe00)
      exception(BOMBS_BUS_ERROR,EA_READ,addr);
#endif

#ifdef ONEGAME
  if (addr>=OG_TEXT_ADDRESS && addr<OG_TEXT_ADDRESS+OG_TEXT_LEN){
    return BYTE(OG_TextMem[addr-OG_TEXT_ADDRESS]);
  }
#endif

#if defined(SSE_BUILD)

  ASSERT( (addr&0xFFFFFF)==addr ); // done in 'peek' part

  BYTE ior_byte=0xff; // default value

#if defined(SSE_VAR_OPT_390A)
  act=ACT;
#endif

  // Main switch: address groups
  switch(addr&0xffff00) 
  {

    ///////////////////////////
    // ACIAs (IKBD and MIDI) //
    ///////////////////////////

#if defined(SSE_ACIA)

#undef LOGSECTION
#define LOGSECTION LOGSECTION_ACIA

    case 0xfffc00:
    {
/*

$FFFC00|byte |Keyboard ACIA status              BIT 7 6 5 4 3 2 1 0|R
$FFFC04|byte |MIDI ACIA status                  BIT 7 6 5 4 3 2 1 0|R
       |     |Interrupt request --------------------' | | | | | | ||
       |     |Parity error ---------------------------' | | | | | ||
       |     |Rx overrun -------------------------------' | | | | ||
       |     |Framing error ------------------------------' | | | ||
       |     |CTS ------------------------------------------' | | ||
       |     |DCD --------------------------------------------' | ||
       |     |Tx data register empty ---------------------------' ||
       |     |Rx data register full ------------------------------'|

$FFFC02|byte |Keyboard ACIA data                                   |R/W
$FFFC06|byte |MIDI ACIA data                                       |R/W

*/
      acia[0].BusJam(addr);
      int acia_num;
      switch (addr) // ACIA registers
      {
      case 0xfffc00: // Keyboard ACIA Control
      case 0xfffc04: // MIDI ACIA Control
        acia_num=(addr&0xf)>>2;
        ASSERT(acia_num==0 || acia_num==1);
#if defined(SSE_IKBD_6301_PASTE)
        if(OPTION_C1 && !bPastingText)
#else
        if(OPTION_C1)
#endif
        {
          ior_byte=acia[acia_num].SR;
#if !defined(SSE_ACIA_393) // SR should be up-to-date now
#if defined(SSE_TIMINGS_CPUTIMER64)
          if(abs((int)(ACT-acia[acia_num].last_tx_write_time))
            <ACIA_TDR_COPY_DELAY)
#else
          if(abs(ACT-acia[acia_num].last_tx_write_time)<ACIA_TDR_COPY_DELAY)
#endif
          {
            TRACE_LOG("ACIA %d SR TDRE not set yet (%d)\n",acia_num,ACT-acia[acia_num].last_tx_write_time);
            ior_byte&=~BIT_1; // eg Nightdawn STF
          }
#endif
          break;
        }//C1
        ior_byte=0;
        if (acia[acia_num].rx_not_read || acia[acia_num].overrun==ACIA_OVERRUN_YES) ior_byte|=BIT_0; //full bit
        if (acia[acia_num].tx_flag==0) ior_byte|=BIT_1; //empty bit
        if (acia[acia_num].irq) ior_byte|=BIT_7; //irq bit
        if (acia[acia_num].overrun==ACIA_OVERRUN_YES) ior_byte|=BIT_5; //overrun
        break;

      case 0xfffc02:  // Keyboard ACIA Data
      case 0xfffc06:  // MIDI ACIA Data
        acia_num=((addr&0xf)-2)>>2;
        ASSERT(acia_num==0 || acia_num==1);
#ifdef DEBUG_BUILD
        if(mode!=STEM_MODE_CPU) 
        {
          ior_byte=(OPTION_C1)?ACIA_IKBD.RDR:ACIA_IKBD.data; 
          break;
        }
#endif
#if defined(SSE_IKBD_6301_PASTE)
        if(OPTION_C1 && !bPastingText)
#else
        if(OPTION_C1)
#endif
        {
          // Update status BIT 5 (overrun)
          if(acia[acia_num].overrun==ACIA_OVERRUN_COMING) // keep this, it's right
          {
            acia[acia_num].overrun=ACIA_OVERRUN_YES;
            acia[acia_num].SR|=BIT_5; // set overrun (only now, conform to doc)
#if !defined(SSE_ACIA_393)              
            if(acia[acia_num].CR&BIT_7) // irq enabled
              acia[acia_num].SR|=BIT_7; // there's a new IRQ when overrun bit is set
#endif            
            TRACE_LOG("%d %d %d PC %X reads ACIA %d RDR %X, OVR\n",TIMING_INFO,old_pc,acia_num,acia[acia_num].RDR);
          }
          // no overrun, normal
          else
          {
/*
"The Overrun indication is reset after the reading of data from the 
Receive Data Register."
ACIA02.TOS: reading ACIA RDR once after overrun bit is set is enough
to clear overrun
*/
            acia[acia_num].overrun=ACIA_OVERRUN_NO;
            acia[acia_num].SR&=~BIT_0;
            acia[acia_num].SR&=~BIT_5;
#if !defined(SSE_ACIA_393)  
            if(!( acia[acia_num].IrqForTx() && acia[acia_num].SR&BIT_1))
              acia[acia_num].SR&=~BIT_7;
#endif
            TRACE_LOG("%d %d %d PC %X CPU reads ACIA %d RDR %X\n",TIMING_INFO,old_pc,acia_num,acia[acia_num].RDR);
          }
#if defined(SSE_ACIA_393)      
          ACIA_CHECK_IRQ(acia_num);
#else
          mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,
            !( (ACIA_IKBD.SR&BIT_7) || (ACIA_MIDI.SR&BIT_7)) );
#endif

/*
"The nondestructive read cycle (...) although the data in the
Receiver Data Register is retained.
*/
          ior_byte=acia[acia_num].RDR;
          break;
        }//C1
        {//scope
          acia[acia_num].rx_not_read=0;
          LOG_ONLY( bool old_irq=acia[acia_num].irq; )
          if (acia[acia_num].overrun==ACIA_OVERRUN_COMING){
            acia[acia_num].overrun=ACIA_OVERRUN_YES;
            if (acia[acia_num].rx_irq_enabled) acia[acia_num].irq=true;
            LOG_ONLY( log_to_section(acia_num?LOGSECTION_MIDI:LOGSECTION_IKBD,
              EasyStr("ACIA ")+Str(acia_num)+": "+HEXSl(old_pc,6)+
              " - OVERRUN! Read data ($"+HEXSl(acia[acia_num].data,2)+
              "), changing ACIA IRQ bit from "+old_irq+" to "+acia[acia_num].irq); )
          }else{
            acia[acia_num].overrun=ACIA_OVERRUN_NO;
            // IRQ should be off for receive, but could be set for tx empty interrupt
            acia[acia_num].irq=(acia[acia_num].tx_irq_enabled && acia[acia_num].tx_flag==0);
#ifdef SSE_VS2008_WARNING_370
#pragma warning(disable : 4805) 
#endif
            LOG_ONLY( if (acia[acia_num].irq!=old_irq) log_to_section(acia_num?LOGSECTION_MIDI:LOGSECTION_IKBD,
              Str("ACIA ")+Str(acia_num)+": "+HEXSl(old_pc,6)+" - Read data ($"+HEXSl(acia[acia_num].data,2)+
              "), changing ACIA IRQ bit from "+old_irq+" to "+acia[acia_num].irq); )
#ifdef SSE_VS2008_WARNING_370
#pragma warning(default : 4805) 
#endif
          }
          mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
          ior_byte=acia[acia_num].data;
#if defined(SSE_IKBD_6301_PASTE)
          if(!keyboard_buffer_length && PasteText.Empty())
            bPastingText=false; // pasting finished
#endif
          break;
        }
        break;//390 in extremis though it should be one of the four
      }
    }//fffc00-6

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IO

#else // Steem 3.2

    case 0xfffc00:      //----------------------------------- ACIAs
    {
      // Only cause bus jam once per word
      DEBUG_ONLY( if (mode==STEM_MODE_CPU) )
      {
        if (io_word_access==0 || (addr & 1)==0){
//          if (passed VBL or HBL point){
//            BUS_JAM_TIME(4);
//          }else{
          // Jorge Cwik:
          // Access to the ACIA is synchronized to the E signal. Which is a clock with
          // one tenth the frequency of the main CPU clock (800 Khz). So the timing
          // should depend on the phase relationship between both clocks.

          int rel_cycle=ABSOLUTE_CPU_TIME-shifter_cycle_base;
          rel_cycle=8000000-rel_cycle;
          rel_cycle%=10;
          BUS_JAM_TIME(rel_cycle+6);
//          BUS_JAM_TIME(8);
        }
      }

      switch (addr){
/******************** Keyboard ACIA ************************/

      case 0xfffc00:  //status
      {
        BYTE x=0;
        if (ACIA_IKBD.rx_not_read || ACIA_IKBD.overrun==ACIA_OVERRUN_YES) x|=BIT_0; //full bit
        if (ACIA_IKBD.tx_flag==0) x|=BIT_1; //empty bit
//        if (acia[ACIA_IKBD].rx_not_read && acia[ACIA_IKBD].rx_irq_enabled) x|=BIT_7; //irq bit
        if (ACIA_IKBD.irq) x|=BIT_7; //irq bit
        if (ACIA_IKBD.overrun==ACIA_OVERRUN_YES) x|=BIT_5; //overrun
        return x;
      }
      case 0xfffc02:  //data
      {
        DEBUG_ONLY( if (mode!=STEM_MODE_CPU) return ACIA_IKBD.data; )
//        if (acia[ACIA_IKBD].rx_not_read) keyboard_buffer_length--;
        ACIA_IKBD.rx_not_read=0;
        LOG_ONLY( bool old_irq=ACIA_IKBD.irq; )
        if (ACIA_IKBD.overrun==ACIA_OVERRUN_COMING){
          ACIA_IKBD.overrun=ACIA_OVERRUN_YES;
          if (ACIA_IKBD.rx_irq_enabled) ACIA_IKBD.irq=true;
          LOG_ONLY( log_to_section(LOGSECTION_IKBD,EasyStr("IKBD: ")+HEXSl(old_pc,6)+
                              " - OVERRUN! Read data ($"+HEXSl(ACIA_IKBD.data,2)+
                              "), changing ACIA IRQ bit from "+old_irq+" to "+ACIA_IKBD.irq); )
        }else{
          ACIA_IKBD.overrun=ACIA_OVERRUN_NO;
          // IRQ should be off for receive, but could be set for tx empty interrupt
          ACIA_IKBD.irq=(ACIA_IKBD.tx_irq_enabled && ACIA_IKBD.tx_flag==0);
          LOG_ONLY( if (ACIA_IKBD.irq!=old_irq) log_to_section(LOGSECTION_IKBD,Str("IKBD: ")+
                            HEXSl(old_pc,6)+" - Read data ($"+HEXSl(ACIA_IKBD.data,2)+
                            "), changing ACIA IRQ bit from "+old_irq+" to "+ACIA_IKBD.irq); )
        }
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
        return ACIA_IKBD.data;
      }

  /******************** MIDI ACIA ************************/

      case 0xfffc04:  // status
      {
        BYTE x=0;
        if (ACIA_MIDI.rx_not_read || ACIA_MIDI.overrun==ACIA_OVERRUN_YES) x|=BIT_0; //full bit
        if (ACIA_MIDI.tx_flag==0) x|=BIT_1; //empty bit
        if (ACIA_MIDI.irq) x|=BIT_7; //irq bit
        if (ACIA_MIDI.overrun==ACIA_OVERRUN_YES) x|=BIT_5; //overrun
        return x;
      }
      case 0xfffc06:  // data
        DEBUG_ONLY(if (mode!=STEM_MODE_CPU) return ACIA_MIDI.data);
        ACIA_MIDI.rx_not_read=0;
        if (ACIA_MIDI.overrun==ACIA_OVERRUN_COMING){
          ACIA_MIDI.overrun=ACIA_OVERRUN_YES;
          if (ACIA_MIDI.rx_irq_enabled) ACIA_MIDI.irq=true;
        }else{
          ACIA_MIDI.overrun=ACIA_OVERRUN_NO;
          // IRQ should be off for receive, but could be set for tx empty interrupt
          ACIA_MIDI.irq=(ACIA_MIDI.tx_irq_enabled && ACIA_MIDI.tx_flag==0);
        }
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
        log_to(LOGSECTION_MIDI,Str("MIDI: ")+HEXSl(old_pc,6)+" - Read $"+
                HEXSl(ACIA_MIDI.data,6)+" from MIDI ACIA data register");
        return ACIA_MIDI.data;
      }

      break;

#endif//SSE_ACIA

    /////////
    //  ?  //
    /////////

    case 0xfffd00:      
      break;

    /////////
    // MFP //
    /////////


    case 0xfffa00:{   
      if (addr<0xfffa40){
        //  Hatari also counts 4 cycles of wait states

        
#if defined(SSE_INT_MFP_JAM_AFTER_READ) //see below, we put it after the read
        if(!OPTION_C2)
        {
          DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) 
            if (io_word_access==0 || (addr & 1)==1) 
              BUS_JAM_TIME(4);
        }
#else
        // Only cause bus jam once per word (should this be after the read?)
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) if (io_word_access==0 || (addr & 1)==1) BUS_JAM_TIME(4);
#endif
        if (addr & 1){

#if defined(SSE_INT_MFP_READ_DELAY_392)
          if(OPTION_C2)
            INSTRUCTION_TIME(MFP_READ_REGISTER_DELAY);
#endif

          if (addr==0xfffa01){
            // read GPIP
            // BIT
            //  0   Centronics busy
            //  1   RS-232 data carrier detect - input
            //  2   RS-232 clear to send - input
            //  3   Reserved
            //  4   Keyboard and MIDI
            //  5   FDC/HDC
            //  6   RS-232 ring indicator
            //  7   Monochrome monitor detect
            ior_byte=BYTE(mfp_reg[MFPR_GPIP] & ~mfp_reg[MFPR_DDR]);
            ior_byte|=BYTE(mfp_gpip_input_buffer & mfp_reg[MFPR_DDR]);

#if defined(SSE_ACSI_TIMING)  
/*  Keep the HDC bit set as long as we deem a real hard disk would
    take to trigger IRQ. We do it this way to avoid adding overhead
    (events, block DMA transfers...), knowing that generally GPIP
    is polled by programs. It is a bit risky.
*/
            if(ADAT && ACSI_EMU_ON && AcsiHdc[acsi_dev].Active==2 && !(ior_byte&32)
              && AcsiHdc[acsi_dev].time_of_irq-ACT>0)
              ior_byte|=32; // active-low: inactive
#endif

#if defined(SSE_DONGLE)
/*  Some dongles modify the GPIP register.
    The dongle for BAT2 on the ST is simplistic. It just permanently changes
    a bit (by grounding the line?).
    The dongle for Music Master looks more like the dongle for BAT2 on the
    Amiga. The program changes a line and checks the effect on another line
    at different times.
*/
            switch(DONGLE_ID) {
#if defined(SSE_DONGLE_BAT2)
            case TDongle::BAT2:
              ior_byte&=~BIT_2; //CTS
              break;
#endif
#if defined(SSE_DONGLE_MUSIC_MASTER)
            case TDongle::MUSIC_MASTER:
              { //inspired by WinUAE
                int bit=(ACT-Dongle.Timing>200)?(Dongle.Value&1):(Dongle.Value&2);
                if(bit)
                  ior_byte|=BIT_1; //DCD
                else
                  ior_byte&=~BIT_1;
              }
              break;
#endif
            }//sw
#endif
          }
          else if (addr<0xfffa30)
          {
            int n=(addr-0xfffa01) >> 1;

#if defined(SSE_INT_MFP_LATCH_DELAY)
/*  If a write on the same register is pending, maybe it's time to
    execute it so that the register we read is the correct one.
*/
            if(OPTION_C2 && MC68901.WritePending 
              && n==MC68901.LastRegisterWritten)
            {
              TRACE_MFP("IOR Flush MFP event ");
              event_mfp_write(); //flush
            }
#endif
            if (n>=MFPR_TADR && n<=MFPR_TDDR){ //timer data registers
              mfp_calc_timer_counter(n-MFPR_TADR);
              ior_byte=BYTE(mfp_timer_counter[n-MFPR_TADR]/64);
              if (n==MFPR_TBDR){
                //TRACE("read TBDR y %d lc %d tontb %d ior_byte %x mfp_reg[MFPR_TBDR] %x\n",scan_y,LINECYCLES,time_of_next_timer_b-cpu_timer_at_start_of_hbl,ior_byte,mfp_reg[MFPR_TBDR]);
                if (mfp_get_timer_control_register(1)==8){
                  // Timer B is in event count mode, check if it has counted down since the start of
                  // this instruction. Due to MFP delays this very, very rarely gets changed under 4
                  // cycles from the point of the signal.
#if defined(SSE_INT_MFP_TIMER_B_392)
                  if ((ABSOLUTE_CPU_TIME-time_of_next_timer_b) 
                    >= (OPTION_C2?0:5)){ // >=5 = >4
#else
                  if ((ABSOLUTE_CPU_TIME-time_of_next_timer_b) > 4){
#endif
                    if (!ior_byte
#if defined(SSE_INT_MFP_TIMER_B_SUNNY)
/*  Finally a legit fix for Sunny STE.

	clr.b $fffa1b                                    ; 013A82: 4239 00FF FA1B 
	move.b #$0,$fffa21                               ; 013A88: 13FC 0000 00FF FA21 
	move.b #$8,$fffa1b                               ; 013A90: 13FC 0008 00FF FA1B 
	cmpi.b #$ff,$fffa21                              ; 013A98: 0C39 00FF 00FF FA21 
	bne .l -10 {$013A98}                             ; 013AA0: 6600 FFF6 
	nop                                              ; 013AA4: 4E71 
	nop                                              ; 013AA6: 4E71 
  ...
	jmp 2(pc,D0.W)                                   ; 013CA4: 4EFB 0002 
	nop                                              ; 013CA8: 4E71 
	nop                                              ; 013CAA: 4E71 
	nop                                              ; 013CAC: 4E71 
  ...
	nop                                              ; 013CD2: 4E71 
	nop                                              ; 013CD4: 4E71 
	nop                                              ; 013CD6: 4E71 
	nop                                              ; 013CD8: 4E71 
	move.w #$1b,d0                                   ; 013CDA: 303C 001B 
	move.w (a2)+,(a1)                                ; 013CDE: 329A 
	move.w (a2)+,(a1)                                ; 013CE0: 329A 
  ...

    The program is currently synchronised on the video counter.
    It sets up a one line timer B interrupt and expects no jitter trouble when
    reading the counter. (a1) is palette 0, it is used to draw the scroller.
    There is jitter but it will not cause one more test, the counter changes
    during the read.
    When it was loaded with 0 it counts down to $FF, not 0, that was the real
    problem here
*/
                      && (mfp_reg[MFPR_TBDR]||!OPTION_C2)
#endif
                      )
                      ior_byte=mfp_reg[MFPR_TBDR];
                    else
                      ior_byte--;
                  }
                }
              }
              LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_MFP_TIMERS,Str("MFP: ")+HEXSl(old_pc,6)+
                      " - Read timer "+char('A'+(n-MFPR_TADR))+" counter as "+ior_byte); )
            }else if (n>=MFPR_SCR){
              ior_byte=RS232_ReadReg(n);
            }else{
              ior_byte=mfp_reg[n];
            }
          }
#if defined(SSE_INT_MFP_READ_DELAY_392)
          if(OPTION_C2)
            INSTRUCTION_TIME(-MFP_READ_REGISTER_DELAY);
#endif
#if defined(SSE_INT_MFP_JAM_AFTER_READ)
/*  Counting the bus jam after the read makes sense in the way we can
    remove the 'if ((ABSOLUTE_CPU_TIME-time_of_next_timer_b) > 4)'
    This seems to fix Down TLN.
*/
          if(OPTION_C2)
          {
            DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) 
              if (io_word_access==0 || (addr & 1)==1) 
                BUS_JAM_TIME(4);
          }
#endif
        } 
        else
        { // Even address byte access causes bus error
          DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) 
            if (!io_word_access) 
              exception(BOMBS_BUS_ERROR,EA_READ,addr);
        }
        break;
      }
      else // max allowed address in range is 0xfffa3f
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      break;
    }

    ///////////////////////////////
    // Falcon 256 colour palette //
    ///////////////////////////////

    case 0xff9800:        
    case 0xff9900:        
    case 0xff9a00:        
    case 0xff9b00:        
      if(emudetect_called)
      {
        int n=(addr-0xff9800)/4;
        DWORD val=emudetect_falcon_stpal[n];
        ior_byte=DWORD_B(&val,addr & 3);
      }
      else
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      break;

    /////////////////
    // STE paddles //
    /////////////////

    case 0xff9200:
    {
      bool Illegal=false;
      ior_byte=JoyReadSTEAddress(addr,&Illegal);
      if (Illegal
#if defined(SSE_STF_PADDLES)
        || ST_TYPE!=STE //  Ultimate Arena, thx Petari
#endif
        )
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      break;
    }


    /////////
    //  ?  //
    /////////

    case 0xff9000:
      if(addr>0xff9001)
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      break;


    /////////////
    // Blitter //
    /////////////

    case 0xff8a00:
      ior_byte=Blitter_IO_ReadB(addr); // STF crash there
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
      if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_BLITTER)
        FrameEvents.Add(scan_y,LINECYCLES,'b',((addr-0xff8a00)<<8)|ior_byte);
#endif
      break;

    ///////////////////
    // STE DMA Sound //
    ///////////////////

#undef  LOGSECTION
#define LOGSECTION LOGSECTION_SOUND

    case 0xff8900:    
      if(addr>0xff893f
#if defined(SSE_STF_DMA)
        ||(ST_TYPE!=STE)
#endif
        )
        exception(BOMBS_BUS_ERROR,EA_READ,addr);

      switch (addr){
        case 0xff8901:   //DMA control register
          LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_SOUND,Str("SOUND: ")+HEXSl(old_pc,6)+
                        " - Read DMA sound control as $"+HEXSl(dma_sound_control,2)); )
          ior_byte=dma_sound_control;
          break;
        case 0xff8903:   //HiByte of frame start address
        case 0xff8905:   //MidByte of frame start address
        case 0xff8907:   //LoByte of frame start address
          ior_byte=DWORD_B(&next_dma_sound_start,(0xff8907-addr)/2);
          break;
/*
  DMA-Sound Count Register:

    $FFFF8908  0 0 X X X X X X X X X X X X X X   Hibyte  (ro)
    $FFFF890A  X X X X X X X X X X X X X X X X   Midbyte (ro)
    $FFFF890C  X X X X X X X X X X X X X X X 0   Lowbyte (ro)

  The frame address counter register is read-only, and holds the address of 
  the next sample word to be fetched. 

  Used internally for the DMA-soundchip to count from start- to end-address. 
  No write access.
*/
        case 0xff8909:   //HiByte of frame address counter
        case 0xff890B:   //MidByte of frame address counter
        case 0xff890D:   //LoByte of frame address counter

#if defined(SSE_DEBUG)
          if(addr==0xff890d) 
            TRACE_LOG("F%d Y%d PC%X C%d Read DMA counter %X (%X->%X)\n",FRAME,scan_y,old_pc,LINECYCLES,dma_sound_fetch_address,dma_sound_start,dma_sound_end);
#endif

          if (addr==0xff8909) 
            ior_byte=DWORD_B_2(&dma_sound_fetch_address);
          else if (addr==0xff890B) 
            ior_byte=DWORD_B_1(&dma_sound_fetch_address);
          else
            ior_byte=DWORD_B_0(&dma_sound_fetch_address);
          break;

        case 0xff890F:   //HiByte of frame end address
          ior_byte=LOBYTE(HIWORD(next_dma_sound_end));
          break;

        case 0xff8911:   //MidByte of frame end address
          ior_byte=HIBYTE(LOWORD(next_dma_sound_end));
          break;

        case 0xff8913:   //LoByte of frame end address
          ior_byte=LOBYTE(next_dma_sound_end);
          break;

        case 0xff8921:   //Sound mode control
          ior_byte=dma_sound_mode;
          break;

        case 0xff8922:          // MicroWire data hi
        case 0xff8923:          // MicroWire data lo
        case 0xff8924:          // MicroWire Mask hi
        case 0xff8925:          // MicroWire Mask lo
        {
          WORD dat=0;
          WORD mask=MicroWire_Mask;
          if (MicroWire_StartTime){
            int nShifts=DWORD(ABSOLUTE_CPU_TIME-MicroWire_StartTime)/CPU_CYCLES_PER_MW_SHIFT;
            if (nShifts>15){
              MicroWire_StartTime=0;
            }else{
              dat=WORD(MicroWire_Data << nShifts);
              while (nShifts--){
                bool lobit=(mask & BIT_15)!=0;// warning
                mask<<=1;
                mask|=(int)lobit;
              }
            }
          }
          if(addr==0xff8922)
            ior_byte=HIBYTE(dat);
          else if(addr==0xff8923)
            ior_byte=LOBYTE(dat);
          else if(addr==0xff8924)
            ior_byte=HIBYTE(mask);
          else if (addr==0xff8925) 
            ior_byte=LOBYTE(mask);
          else 
            ior_byte=0;
          break;
        }
      }
      break;

    /////////
    // PSG //
    /////////

    case 0xff8800:
#if defined(SSE_YM2149_BUS_NO_JAM_IF_NOT_RW)
#if defined(SSE_YM2149_BUS_JAM_390B)
      if ((addr & 1) && io_word_access) 
#else
      if (OPTION_HACKS && (addr & 1) && io_word_access) 
#endif
        break; //odd addresses ignored on word read, don't jam
#endif
#if defined(SSE_YM2149_BUS_JAM_390) //see note in iow.cpp
      DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) INSTRUCTION_TIME(1);
#else
      if(!(ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_R))
      {
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) BUS_JAM_TIME(4);
        ioaccess|=IOACCESS_FLAG_PSG_BUS_JAM_R;
      }
#endif
#if defined(SSE_YM2149_BUS_JAM_390B)
      ASSERT(!((addr & 1) && io_word_access));
#else
      if((addr & 1) && io_word_access)
        ;// odd addresses ignored on word writes
      else 
#endif
      if(!(addr & 2))
      { //read data / register select, mirrored at 4,8,12,...
        if(psg_reg_select==PSGR_PORT_A)
        {
          // Drive A, drive B, side, RTS, DTR, strobe and monitor GPO
          // are normally set by ST
          ior_byte=psg_reg[PSGR_PORT_A];
          // Parallel port 0 joystick fire (strobe)
          if (stick[N_JOY_PARALLEL_0] & BIT_4){
            if (stick[N_JOY_PARALLEL_0] & BIT_7)
              ior_byte&=~BIT_5;
            else
              ior_byte|=BIT_5;
          }
        }
        else if(psg_reg_select==PSGR_PORT_B)
        {
          if(!(stick[N_JOY_PARALLEL_0] & BIT_4)
            && !(stick[N_JOY_PARALLEL_1] & BIT_4)
            && ParallelPort.IsOpen())
          {
            ParallelPort.NextByte();
            UpdateCentronicsBusyBit();
            ior_byte=ParallelPort.ReadByte();
          }
          else
            ior_byte=BYTE(0xff & ~( (stick[N_JOY_PARALLEL_0] & b1111) | ((stick[N_JOY_PARALLEL_1] & b1111) << 4) ));
        }
        else
          ior_byte=psg_reg_data;
      }
      break;

#undef  LOGSECTION
#define LOGSECTION LOGSECTION_IO

    ///////////////////
    // Disk DMA +FDC //
    ///////////////////

    case 0xff8600:   

#if defined(SSE_DMA_OBJECT)
      ior_byte=Dma.IORead(addr);
      break;
#else 
    {  
      if (addr>0xff860f) exception(BOMBS_BUS_ERROR,EA_READ,addr);
      if (addr<0xff8604) exception(BOMBS_BUS_ERROR,EA_READ,addr);
      if (addr<0xff8608 && io_word_access==0) exception(BOMBS_BUS_ERROR,EA_READ,addr);
#if USE_PASTI
      if (hPasti && pasti_active){
        if (addr<0xff8608){ // word only
          if (addr & 1) return LOBYTE(pasti_store_byte_access);
        }
        struct pastiIOINFO pioi;
        pioi.addr=addr;
        pioi.stPC=pc;
        pioi.cycles=ABSOLUTE_CPU_TIME;
//          log_to(LOGSECTION_PASTI,Str("PASTI: IO read addr=$")+HEXSl(addr,6)+" pc=$"+HEXSl(pc,6)+" cycles="+pioi.cycles);
        pasti->Io(PASTI_IOREAD,&pioi);
        pasti_handle_return(&pioi);
        if (addr<0xff8608){ // word only
          pasti_store_byte_access=WORD(pioi.data);
          pioi.data=HIBYTE(pioi.data);
        }
//          log_to(LOGSECTION_PASTI,Str("PASTI: Read returning $")+HEXSl(BYTE(pioi.data),2)+" ("+BYTE(pioi.data)+")");
        return BYTE(pioi.data);
      }
#endif
      switch(addr){
      case 0xff8604:  //high byte of FDC access
        //should check bit 8 = 0 (read)
        if (dma_mode & BIT_4){ //read sector counter (maintained by the DMA chip)
          return HIBYTE(dma_sector_count);
        }
        if (dma_mode & BIT_3){ // HD access
          LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                " - Reading high byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )
          return 0xff;
        }
        return 0xff;
      case 0xff8605:  //low byte of FDC access
        //should check bit 8 = 0, read
        if (dma_mode & BIT_4){ //read sector counter (maintained by the DMA chip)
          return LOBYTE(dma_sector_count);
        }

        if (dma_mode & BIT_3){ // HD access
          LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                  " - Reading low byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )
          return 0xff;
        }

        // Read FDC register
        switch (dma_mode & (BIT_1+BIT_2)){
          case 0:
          {
            int fn=floppy_current_drive();
            if (floppy_track_index_pulse_active()){
              fdc_str|=FDC_STR_T1_INDEX_PULSE;
            }else{
              // If not type 1 command we will get here, it is okay to clear
              // it as this bit is only for the DMA chip for type 2/3.
              fdc_str&=BYTE(~FDC_STR_T1_INDEX_PULSE);
            }
            if (floppy_type1_command_active){
              /* From Jorge Cwik
                The FDC has two different
                type of status. There is a "Type I" status after any Type I command,
                and there is a different "status" after types II & III commands. The
                meaning of some of the status bits is different (this probably you
                already know),  but the updating of these bits is different too.

                In a Type II-III status, the write protect bit is updated from the write
                protect signal only when trying to write to the disk (write sector
                or format track), otherwise is clear. This bit is static, once it was
                updated or cleared, it will never change until a new command is
                issued to the FDC.
              */
              fdc_str&=(~FDC_STR_WRITE_PROTECT);
              if (floppy_mediach[fn]){
                if (floppy_mediach[fn]/10!=1) fdc_str|=FDC_STR_WRITE_PROTECT;
              }else if (FloppyDrive[fn].ReadOnly){
                fdc_str|=FDC_STR_WRITE_PROTECT;
              }
              if (fdc_spinning_up){
                fdc_str&=BYTE(~FDC_STR_T1_SPINUP_COMPLETE);
              }else{
                fdc_str|=FDC_STR_T1_SPINUP_COMPLETE;
              }
            } // else it should be set in fdc_execute()
            if ((mfp_reg[MFPR_GPIP] & BIT_5)==0){
              LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                          " - Reading status register as "+Str(itoa(fdc_str,d2_t_buf,2)).LPad(8,'0')+
                          " ($"+HEXSl(fdc_str,2)+"), clearing IRQ"); )
              floppy_irq_flag=0;
              mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
            }
//            log_DELETE_SOON(Str("FDC: ")+HEXSl(old_pc,6)+" - reading FDC status register as $"+HEXSl(fdc_str,2));
/*
            LOG_ONLY( if (mode==STEM_MODE_CPU) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                            " - Read status register as $"+HEXSl(fdc_str,2)); )
*/
            return fdc_str;
          }
          case 2:
            return fdc_tr; //track register
          case 4:
            return fdc_sr; //sector register
          case 6:
            return fdc_dr; //data register
        }
        break;
      case 0xff8606:  //high byte of DMA status
        return 0x0;
      case 0xff8607:  //low byte of DMA status
        return BYTE(b11110000) | dma_status;
      case 0xff8609:  //high byte of DMA pointer
        return (BYTE)((dma_address&0xff0000)>>16);
      case 0xff860b:  //mid byte of DMA pointer
        return (BYTE)((dma_address&0xff00)>>8);
      case 0xff860d:  //low byte of DMA pointer
        return (BYTE)((dma_address&0xff));
      case 0xff860e: //frequency/density control
      {
        if (FloppyDrive[floppy_current_drive()].STT_File) return 0;

        TFloppyImage *floppy=&(FloppyDrive[floppy_current_drive()]);
        return BYTE((floppy->BytesPerSector * floppy->SectorsPerTrack)>7000);
      }
      case 0xff860f: //high byte of frequency/density control?
        return 0;
      }
      break;
    }
#endif//!dmaio

    //////////////////////
    // Shifter-MMU-GLUE //
    //////////////////////

    case 0xff8200:

#if defined(SSE_VIDEO_CHIPSET)

      ASSERT( (addr&0xFFFF00)==0xff8200 );
/*
    This was in Steem 3.2:
    // Below $10 - Odd bytes return value or 0, even bytes return 0xfe/0x7e
    // Above $40 - Unused return 0
    //// Unused bytes between $60 and $80 should return 0!
   For now, we keep that for STE (refactored), return $FF for STF.
   ior_byte is our return value, to which we immediately give a default
   value.
   Cases 
   R FF820D Lemmings40
*/
      ior_byte= ((addr&1)||addr>0xff8240) ? 0 : 0xFE; 
#if defined(SSE_STF_VIDEO_IOR)
      if(ST_TYPE!=STE)
        ior_byte=0xFF; 
#endif  

      if (addr>=0xff8240 && addr<0xff8260){  //palette
#if defined(SSE_MMU_ROUNDING_BUS)
        cpu_cycles&=-4; // Shifter access -> wait states possible
#endif
#if defined(SSE_BLT_390B)
        Blit.BlitCycles=0;
#endif
        int n=(addr-0xff8240)/2; // which palette
        ior_byte= (addr&1) ? (STpal[n]&0xFF) : (STpal[n]>>8);
      }else if (addr>0xff820f && addr<0xff8240){ //forbidden gap
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }else if (addr>0xff827f){  //forbidden area after SHIFTER
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }else{
        switch(addr){

        case 0xff8201:  //high byte of screen memory address
          ior_byte=LOBYTE(HIWORD(xbios2));
          break;

        case 0xff8203:  //mid byte of screen memory address
          ior_byte=HIBYTE(LOWORD(xbios2));
          break;

        case 0xff8205:  //high byte of screen draw pointer
        case 0xff8207:  //mid byte of screen draw pointer
        case 0xff8209:{  //low byte of screen draw pointer
          MEM_ADDRESS sdp;
#ifdef SSE_VIDEO_CHIPSET
          sdp=MMU.ReadVideoCounter(LINECYCLES); // a complicated affair
#else
          if(scan_y<shifter_first_draw_line || scan_y>=shifter_last_draw_line){
            sdp=shifter_draw_pointer;
          }else{
            sdp=get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
            LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+
              " - Read Shifter draw pointer as $"+HEXSl(sdp,6)+
              " on "+scanline_cycle_log()); )
          }
#endif

          ior_byte=DWORD_B(&sdp,(2-(addr-0xff8205)/2)); // change for big endian !!!!!!!!!
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
          if(mode!=STEM_MODE_INSPECT &&(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SDP_READ))
            FrameEvents.Add(scan_y,LINECYCLES,'c',((addr&0xF)<<8)|ior_byte);
#endif
          }
          break;

        case 0xff820a:  //synchronization mode
          ior_byte&=~3;           // this way takes care...
          ior_byte|=Glue.m_SyncMode;   // ...of both STF & STE
          break;

        case 0xff820d:  //low byte of screen memory address
#if defined(SSE_STF)
          if(ST_TYPE==STE) 
#endif
            ior_byte=(BYTE)xbios2&0xFF;
          break;

        case 0xff820f: // LINEWID
#if defined(SSE_STF)
          if(ST_TYPE==STE) 
#endif
#if defined(SSE_MMU_LINEWID_TIMING)
            ior_byte=shifter_fetch_extra_words;
#else
            ior_byte=LINEWID;
#endif
          break;

        case 0xff8260: //resolution
#if defined(SSE_MMU_ROUNDING_BUS)
          cpu_cycles&=-4; // Shifter access -> wait states possible
#endif
#if defined(SSE_BLT_390B)
          Blit.BlitCycles=0;
#endif
          ior_byte&=~3;           // this way takes care
          ior_byte|=Shifter.m_ShiftMode;  // of both STF & STE
          break;

        case 0xff8265:  //HSCROLL
#if defined(SSE_MMU_ROUNDING_BUS)
          cpu_cycles&=-4; // Shifter access -> wait states possible
#endif
#if defined(SSE_BLT_390B)
          Blit.BlitCycles=0;
#endif
          DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) 
            shifter_hscroll_extra_fetch=(shifter_hscroll!=0); //case Kultur Melk
#if defined(SSE_STF)
          if(ST_TYPE==STE)
#endif
            ior_byte=HSCROLL;
          break;
        }//if
      }
#if defined(SSE_VIDEO_IOR_TRACE)
      // made possible by our structure change
      TRACE("Video read %X=%X\n",addr,ior_byte); // not LOG
#endif
      break;

#else // Steem 3.2
    {
/*
allowed addresses

FF8200 - FF820F   SHIFTER
FF8240 - FF827F   palette, res
*/
      if (addr>=0xff8240 && addr<0xff8260){  //palette
        int n=addr-0xff8240;n/=2;
        if (addr&1) return LOBYTE(STpal[n]);
        else return HIBYTE(STpal[n]);
      }else if (addr>0xff820f && addr<0xff8240){ //forbidden gap
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }else if (addr>0xff827f){  //forbidden area after SHIFTER
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }else{
        switch(addr){
        case 0xff8201:  //high byte of screen memory address
          return LOBYTE(HIWORD(xbios2));
        case 0xff8203:  //mid byte of screen memory address
          return HIBYTE(LOWORD(xbios2));
        case 0xff820d:  //low byte of screen memory address
          return LOBYTE(xbios2);
        case 0xff8205:  //high byte of screen draw pointer
        case 0xff8207:  //mid byte of screen draw pointer
        case 0xff8209:{  //low byte of screen draw pointer
          MEM_ADDRESS sdp;
          if (scan_y<shifter_first_draw_line || scan_y>=shifter_last_draw_line){
            sdp=shifter_draw_pointer;
          }else{
            sdp=get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
            LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+
                        " - Read shifter draw pointer as $"+HEXSl(sdp,6)+
                        " on "+scanline_cycle_log()); )
          }
          return DWORD_B(&sdp, (2-(addr-0xff8205)/2) );    // change for big endian
        }
        case 0xff820a:  //synchronization mode
          if (shifter_freq==50) return b11111110;
          return b11111100;
        case 0xff820f:
          return (BYTE)shifter_fetch_extra_words;

        //// Unused bytes between $60 and $80 should return 0!
        case 0xff8260: //resolution
          return (BYTE)screen_res;
        case 0xff8264:  //hscroll no increase screen width
          return (BYTE)0;
        case 0xff8265:  //hscroll
          DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) shifter_hscroll_extra_fetch=(shifter_hscroll!=0);
          return (BYTE)shifter_hscroll;
        }
        // Below $10 - Odd bytes return value or 0, even bytes return 0xfe/0x7e
        // Above $40 - Unused return 0
        if (addr<=0xff820f && (addr & 1)==0) return 0xfe;
        return 0;
      }
      break;
    }
#endif

    /////////
    // MMU //
    /////////

    case 0xff8000:
//SS note  mmu_memory_configuration=BYTE((conf0 << 2) | conf1);
      if(addr==0xff8001)
        ior_byte=(mem_len>FOUR_MEGS) ? (BYTE)(MEMCONF_2MB|(MEMCONF_2MB<<2))
                                     : mmu_memory_configuration;
      else if(addr>0xff800f) //forbidden range
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      else if(addr & 1)
        ior_byte=0;
      break;

    /////////////////////////
    // Secret Emu Register //
    /////////////////////////

#if !defined(SSE_VAR_NO_EMU_DETECT)
    case 0xffc100:
#ifdef DEBUG_BUILD
      if(addr==0xffc123) return (BYTE)runstate;
#endif
      if(emudetect_called){
        switch (addr){
#ifdef SSE_UNIX
        case 0xffc100: ior_byte=(BYTE)(stem_version_text[0]-'0'); break;
#else
        case 0xffc100: ior_byte=BYTE(stem_version_text[0]-'0'); break;
#endif
        case 0xffc101:
          {
#if defined(SSE_BUILD)  //BCC
            Str minor_ver=(char*)stem_version_text+2;
#else
            Str minor_ver=stem_version_text+2;
#endif
            for (int i=0;i<minor_ver.Length();i++){
              if (minor_ver[i]<'0' || minor_ver[i]>'9'){
                minor_ver.SetLength(i);
                break;
              }
            }
            int ver=atoi(minor_ver.RPad(2,'0'));
            ior_byte=BYTE(((ver/10) << 4) | (ver % 10));
            break;
          }
        case 0xffc102: ior_byte=BYTE(slow_motion); break;
        case 0xffc103: ior_byte=BYTE(slow_motion_speed/10); break;
        case 0xffc104: ior_byte= BYTE(fast_forward); break;
        case 0xffc105: ior_byte= BYTE(n_cpu_cycles_per_second/1000000); break;
        case 0xffc106: ior_byte= BYTE(0 DEBUG_ONLY(+1)); break;
        case 0xffc107: ior_byte= snapshot_loaded; break;
        case 0xffc108: ior_byte= BYTE((100000/run_speed_ticks_per_second) >> 8); break;
        case 0xffc109: ior_byte= BYTE((100000/run_speed_ticks_per_second) & 0xff); break;
        case 0xffc10a:
          if(avg_frame_time) 
            ior_byte= BYTE((((12000/avg_frame_time)*100)/shifter_freq) >> 8);
          ior_byte= 0;
          break;
        case 0xffc10b:
          if(avg_frame_time)
            ior_byte= BYTE((((12000/avg_frame_time)*100)/shifter_freq) & 0xff);
          ior_byte= 0;
          break;
#if !defined(SSE_TIMINGS_CPUTIMER64_B)
        case 0xffc10c: ior_byte= HIBYTE(HIWORD(ABSOLUTE_CPU_TIME)); break;
        case 0xffc10d: ior_byte= LOBYTE(HIWORD(ABSOLUTE_CPU_TIME)); break;
        case 0xffc10e: ior_byte= HIBYTE(LOWORD(ABSOLUTE_CPU_TIME)); break;
        case 0xffc10f: ior_byte= LOBYTE(LOWORD(ABSOLUTE_CPU_TIME)); break;

        case 0xffc110: ior_byte= HIBYTE(HIWORD(cpu_time_of_last_vbl)); break;
        case 0xffc111: ior_byte= LOBYTE(HIWORD(cpu_time_of_last_vbl)); break;
        case 0xffc112: ior_byte= HIBYTE(LOWORD(cpu_time_of_last_vbl)); break;
        case 0xffc113: ior_byte= LOBYTE(LOWORD(cpu_time_of_last_vbl)); break;

        case 0xffc114: ior_byte= HIBYTE(HIWORD(cpu_timer_at_start_of_hbl)); break;
        case 0xffc115: ior_byte= LOBYTE(HIWORD(cpu_timer_at_start_of_hbl)); break;
        case 0xffc116: ior_byte= HIBYTE(LOWORD(cpu_timer_at_start_of_hbl)); break;
        case 0xffc117: ior_byte= LOBYTE(LOWORD(cpu_timer_at_start_of_hbl)); break;
#endif
        case 0xffc118: ior_byte= HIBYTE( (short) (scan_y)); break;
        case 0xffc119: ior_byte= LOBYTE( (short) (scan_y)); break;
        case 0xffc11a: ior_byte= emudetect_write_logs_to_printer; break;
        case 0xffc11b: ior_byte= emudetect_falcon_mode; break;
        case 0xffc11c: ior_byte= BYTE((emudetect_falcon_mode_size-1) + (emudetect_falcon_extra_height ? 2:0)); break;
        case 0xffc11d: ior_byte= emudetect_overscans_fixed; break;
#if defined(SSE_VAR_EMU_DETECT) 
        default: ior_byte= 0;
#endif
        }//sw
        break;
    }
#endif//!defined(SSE_VAR_NO_EMU_DETECT)

    /////////////
    // Alt-RAM //
    /////////////

#if defined(SSE_MMU_MONSTER_ALT_RAM_IO)
    case 0xfffe00: {
      int offset=addr&0xFF;
      if(mem_len!=0xC00000 || !io_word_access || offset>8 
        || (offset==8 &&!SUPERFLAG))
        exception(BOMBS_BUS_ERROR,EA_READ,addr);

      else switch(offset) {
        case 1:
          ior_byte=(MMU.MonSTerHimem)?(MMU.MonSTerHimem/0x100000-4):0;
          break;
        case 8:
          ior_byte=1; // "firmware version"
          break;
        default:
          ior_byte=0;
      }
      break;
    }
#endif

    default: //not in allowed area
      exception(BOMBS_BUS_ERROR,EA_READ,addr);
  }//sw

#if defined(SSE_DEBUG_TRACE_IO)
  if(!io_word_access
#if defined(SSE_BOILER_TRACE_CONTROL)
 //   && (((1<<14)&d2_dpeek(FAKE_IO_START+24))
      && ((TRACE_MASK_IO & TRACE_CONTROL_IO_R)
// we add conditions address range - logsection enabled

      && (old_pc<rom_addr)
//      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_INTERRUPTS] ) //mfp
      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_MFP] ) //mfp
      && ( (addr&0xffff00)!=0xfffc00 || logsection_enabled[LOGSECTION_IKBD] ) //acia
#if defined(SSE_BOILER_390_LOG2)
      && ( (addr&0xffff00)!=0xff8600 || logsection_enabled[LOGSECTION_DMA] ) //dma
#else
      && ( (addr&0xffff00)!=0xff8600 || logsection_enabled[LOGSECTION_FDC] ) //dma
#endif
      && ( (addr&0xffff00)!=0xff8800 || logsection_enabled[LOGSECTION_SOUND] )//psg
      && ( (addr&0xffff00)!=0xff8900 || logsection_enabled[LOGSECTION_SOUND] )//dma
      && ( (addr&0xffff00)!=0xff8a00 || logsection_enabled[LOGSECTION_BLITTER] )
      && ( (addr&0xffff00)!=0xff8200 || logsection_enabled[LOGSECTION_VIDEO] )//shifter
      && ( (addr&0xffff00)!=0xff8000 || (((1<<13)&d2_dpeek(FAKE_IO_START+24)) ))//MMU

      ) 
#else
    && (addr&0xFFFF00)!=0xFFFA00 // many MFP reads
    && addr!=0xFFFC02  // if IKBD data polling...
#endif
    )
    TRACE_LOG("%d PC %X IOR.B %X : %X\n",ACT,old_pc,addr,ior_byte);
#endif

  return ior_byte;

#else // Steem 3.2

  switch (addr & 0xffff00){  //fffe00
    case 0xfffc00:      //----------------------------------- ACIAs
    {
      // Only cause bus jam once per word
      DEBUG_ONLY( if (mode==STEM_MODE_CPU) )
      {
        if (io_word_access==0 || (addr & 1)==0){
//          if (passed VBL or HBL point){
//            BUS_JAM_TIME(4);
//          }else{
          // Jorge Cwik:
          // Access to the ACIA is synchronized to the E signal. Which is a clock with
          // one tenth the frequency of the main CPU clock (800 Khz). So the timing
          // should depend on the phase relationship between both clocks.

          int rel_cycle=ABSOLUTE_CPU_TIME-shifter_cycle_base;
          rel_cycle=8000000-rel_cycle;
          rel_cycle%=10;
          BUS_JAM_TIME(rel_cycle+6);
//          BUS_JAM_TIME(8);
        }
      }

      switch (addr){
/******************** Keyboard ACIA ************************/

      case 0xfffc00:  //status
      {
        BYTE x=0;
        if (ACIA_IKBD.rx_not_read || ACIA_IKBD.overrun==ACIA_OVERRUN_YES) x|=BIT_0; //full bit
        if (ACIA_IKBD.tx_flag==0) x|=BIT_1; //empty bit
//        if (acia[ACIA_IKBD].rx_not_read && acia[ACIA_IKBD].rx_irq_enabled) x|=BIT_7; //irq bit
        if (ACIA_IKBD.irq) x|=BIT_7; //irq bit
        if (ACIA_IKBD.overrun==ACIA_OVERRUN_YES) x|=BIT_5; //overrun
        return x;
      }
      case 0xfffc02:  //data
      {
        DEBUG_ONLY( if (mode!=STEM_MODE_CPU) return ACIA_IKBD.data; )
//        if (acia[ACIA_IKBD].rx_not_read) keyboard_buffer_length--;
        ACIA_IKBD.rx_not_read=0;
        LOG_ONLY( bool old_irq=ACIA_IKBD.irq; )
        if (ACIA_IKBD.overrun==ACIA_OVERRUN_COMING){
          ACIA_IKBD.overrun=ACIA_OVERRUN_YES;
          if (ACIA_IKBD.rx_irq_enabled) ACIA_IKBD.irq=true;
          LOG_ONLY( log_to_section(LOGSECTION_IKBD,EasyStr("IKBD: ")+HEXSl(old_pc,6)+
                              " - OVERRUN! Read data ($"+HEXSl(ACIA_IKBD.data,2)+
                              "), changing ACIA IRQ bit from "+old_irq+" to "+ACIA_IKBD.irq); )
        }else{
          ACIA_IKBD.overrun=ACIA_OVERRUN_NO;
          // IRQ should be off for receive, but could be set for tx empty interrupt
          ACIA_IKBD.irq=(ACIA_IKBD.tx_irq_enabled && ACIA_IKBD.tx_flag==0);
          LOG_ONLY( if (ACIA_IKBD.irq!=old_irq) log_to_section(LOGSECTION_IKBD,Str("IKBD: ")+
                            HEXSl(old_pc,6)+" - Read data ($"+HEXSl(ACIA_IKBD.data,2)+
                            "), changing ACIA IRQ bit from "+old_irq+" to "+ACIA_IKBD.irq); )
        }
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
        return ACIA_IKBD.data;
      }

  /******************** MIDI ACIA ************************/

      case 0xfffc04:  // status
      {
        BYTE x=0;
        if (ACIA_MIDI.rx_not_read || ACIA_MIDI.overrun==ACIA_OVERRUN_YES) x|=BIT_0; //full bit
        if (ACIA_MIDI.tx_flag==0) x|=BIT_1; //empty bit
        if (ACIA_MIDI.irq) x|=BIT_7; //irq bit
        if (ACIA_MIDI.overrun==ACIA_OVERRUN_YES) x|=BIT_5; //overrun
        return x;
      }
      case 0xfffc06:  // data
        DEBUG_ONLY(if (mode!=STEM_MODE_CPU) return ACIA_MIDI.data);
        ACIA_MIDI.rx_not_read=0;
        if (ACIA_MIDI.overrun==ACIA_OVERRUN_COMING){
          ACIA_MIDI.overrun=ACIA_OVERRUN_YES;
          if (ACIA_MIDI.rx_irq_enabled) ACIA_MIDI.irq=true;
        }else{
          ACIA_MIDI.overrun=ACIA_OVERRUN_NO;
          // IRQ should be off for receive, but could be set for tx empty interrupt
          ACIA_MIDI.irq=(ACIA_MIDI.tx_irq_enabled && ACIA_MIDI.tx_flag==0);
        }
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
        log_to(LOGSECTION_MIDI,Str("MIDI: ")+HEXSl(old_pc,6)+" - Read $"+
                HEXSl(ACIA_MIDI.data,6)+" from MIDI ACIA data register");
        return ACIA_MIDI.data;
      }

      break;
    }case 0xfffd00:{      //----------------------------------- ?
      return 0xff;
    }case 0xfffa00:{      //----------------------------------- MFP
      if (addr<0xfffa40){
        // Only cause bus jam once per word (should this be after the read?)
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) if (io_word_access==0 || (addr & 1)==1) BUS_JAM_TIME(4);

        BYTE x=0xff;
        if (addr & 1){
          if (addr==0xfffa01){
            // read GPIP
            // BIT
            //  0   Centronics busy
            //  1   RS-232 data carrier detect - input
            //  2   RS-232 clear to send - input
            //  3   Reserved
            //  4   Keyboard and MIDI
            //  5   FDC/HDC
            //  6   RS-232 ring indicator
            //  7   Monochrome monitor detect
            x=BYTE(mfp_reg[MFPR_GPIP] & ~mfp_reg[MFPR_DDR]);
            x|=BYTE(mfp_gpip_input_buffer & mfp_reg[MFPR_DDR]);
          }else if (addr<0xfffa30){
            int n=(addr-0xfffa01) >> 1;
            if (n>=MFPR_TADR && n<=MFPR_TDDR){ //timer data registers
              mfp_calc_timer_counter(n-MFPR_TADR);
              x=BYTE(mfp_timer_counter[n-MFPR_TADR]/64);
              if (n==MFPR_TBDR){
                if (mfp_get_timer_control_register(1)==8){
                  // Timer B is in event count mode, check if it has counted down since the start of
                  // this instruction. Due to MFP delays this very, very rarely gets changed under 4
                  // cycles from the point of the signal.
                  if ((ABSOLUTE_CPU_TIME-time_of_next_timer_b) > 4){
                    if (x==0){
                      x=mfp_reg[MFPR_TBDR];
                    }else{
                      x--;
                    }
                  }
                }
              }
              LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_MFP_TIMERS,Str("MFP: ")+HEXSl(old_pc,6)+
                      " - Read timer "+char('A'+(n-MFPR_TADR))+" counter as "+x); )
            }else if (n>=MFPR_SCR){
              x=RS232_ReadReg(n);
            }else{
              x=mfp_reg[n];
            }
          }
        }else{ // Even address
          // Byte access causes bus error
          DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) if (io_word_access==0) exception(BOMBS_BUS_ERROR,EA_READ,addr);
        }
        return x;
      }else{ // max allowed address in range is 0xfffa3f
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
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
        return DWORD_B(&val,addr & 3);
      }
      exception(BOMBS_BUS_ERROR,EA_READ,addr);
      break;
    case 0xff9200:{      //----------------------------------- paddles
      bool Illegal=0;
      BYTE ret=JoyReadSTEAddress(addr,&Illegal);
      if (Illegal) exception(BOMBS_BUS_ERROR,EA_READ,addr);
      return ret;
    }case 0xff9000:{      //----------------------------------- ?
      if(addr>0xff9001){
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }break;
    }case 0xff8a00:{      //----------------------------------- Blitter
      return Blitter_IO_ReadB(addr);
    }case 0xff8900:{      //----------------------------------- STE DMA Sound
      switch (addr){
        case 0xff8901:   //DMA control register
          LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_SOUND,Str("SOUND: ")+HEXSl(old_pc,6)+
                        " - Read DMA sound control as $"+HEXSl(dma_sound_control,2)); )
          return dma_sound_control;
        case 0xff8903:   //HiByte of frame start address
        case 0xff8905:   //MidByte of frame start address
        case 0xff8907:   //LoByte of frame start address
          return DWORD_B(&next_dma_sound_start,(0xff8907-addr)/2);
        case 0xff8909:   //HiByte of frame address counter
        case 0xff890B:   //MidByte of frame address counter
        case 0xff890D:   //LoByte of frame address counter
        {
          if (addr==0xff8909) return DWORD_B_2(&dma_sound_fetch_address);
          if (addr==0xff890B) return DWORD_B_1(&dma_sound_fetch_address);
          return DWORD_B_0(&dma_sound_fetch_address);
        }
        case 0xff890F:   //HiByte of frame end address
          return LOBYTE(HIWORD(next_dma_sound_end));
        case 0xff8911:   //MidByte of frame end address
          return HIBYTE(LOWORD(next_dma_sound_end));
        case 0xff8913:   //LoByte of frame end address
          return LOBYTE(next_dma_sound_end);
        case 0xff8921:   //Sound mode control
          return dma_sound_mode;

        case 0xff8922:          // MicroWire data hi
        case 0xff8923:          // MicroWire data lo
        case 0xff8924:          // MicroWire Mask hi
        case 0xff8925:          // MicroWire Mask lo
        {
          WORD dat=0;
          WORD mask=MicroWire_Mask;
          if (MicroWire_StartTime){
            int nShifts=DWORD(ABSOLUTE_CPU_TIME-MicroWire_StartTime)/CPU_CYCLES_PER_MW_SHIFT;
            if (nShifts>15){
              MicroWire_StartTime=0;
            }else{
              dat=WORD(MicroWire_Data << nShifts);
              while (nShifts--){
                bool lobit=(mask & BIT_15)!=0;
                mask<<=1;
                mask|=lobit;
              }
            }
          }
          if (addr==0xff8922) return HIBYTE(dat);
          if (addr==0xff8923) return LOBYTE(dat);
          if (addr==0xff8924) return HIBYTE(mask);
          if (addr==0xff8925) return LOBYTE(mask);
          return 0;
        }
      }
      if (addr>0xff893f){ // FF8900 - FF893F   DMA sound, microwire
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }
      break;
    }case 0xff8800:{      //----------------------------------- sound chip
      if ((ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_R)==0){
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) BUS_JAM_TIME(4);
        ioaccess|=IOACCESS_FLAG_PSG_BUS_JAM_R;
      }
      if ((addr & 1) && io_word_access) return 0xff; //odd addresses ignored on word writes

      if ((addr & 2)==0){ //read data / register select, mirrored at 4,8,12,...
        if (psg_reg_select==PSGR_PORT_A){
          // Drive A, drive B, side, RTS, DTR, strobe and monitor GPO
          // are normally set by ST
          BYTE Ret=psg_reg[PSGR_PORT_A];

          // Parallel port 0 joystick fire (strobe)
          if (stick[N_JOY_PARALLEL_0] & BIT_4){
            if (stick[N_JOY_PARALLEL_0] & BIT_7){
              Ret&=~BIT_5;
            }else{
              Ret|=BIT_5;
            }
          }
          return Ret;
        }else if (psg_reg_select==PSGR_PORT_B){
          if ((stick[N_JOY_PARALLEL_0] & BIT_4)==0 && (stick[N_JOY_PARALLEL_1] & BIT_4)==0){
            if (ParallelPort.IsOpen()){
              ParallelPort.NextByte();
              UpdateCentronicsBusyBit();
              return ParallelPort.ReadByte();
            }else{
              return 0xff;
            }
          }else{
            return BYTE(0xff & ~( (stick[N_JOY_PARALLEL_0] & b1111) | ((stick[N_JOY_PARALLEL_1] & b1111) << 4) ));
          }
        }else{
          return psg_reg_data;
        }
      }
      return 0xff;
    }case 0xff8600:{      //----------------------------------- DMA/FDC
      if (addr>0xff860f) exception(BOMBS_BUS_ERROR,EA_READ,addr);
      if (addr<0xff8604) exception(BOMBS_BUS_ERROR,EA_READ,addr);
      if (addr<0xff8608 && io_word_access==0) exception(BOMBS_BUS_ERROR,EA_READ,addr);
#if USE_PASTI
      if (hPasti && pasti_active){
        if (addr<0xff8608){ // word only
          if (addr & 1) return LOBYTE(pasti_store_byte_access);
        }
        struct pastiIOINFO pioi;
        pioi.addr=addr;
        pioi.stPC=pc;
        pioi.cycles=ABSOLUTE_CPU_TIME;
//          log_to(LOGSECTION_PASTI,Str("PASTI: IO read addr=$")+HEXSl(addr,6)+" pc=$"+HEXSl(pc,6)+" cycles="+pioi.cycles);
        pasti->Io(PASTI_IOREAD,&pioi);
        pasti_handle_return(&pioi);
        if (addr<0xff8608){ // word only
          pasti_store_byte_access=WORD(pioi.data);
          pioi.data=HIBYTE(pioi.data);
        }
//          log_to(LOGSECTION_PASTI,Str("PASTI: Read returning $")+HEXSl(BYTE(pioi.data),2)+" ("+BYTE(pioi.data)+")");
        return BYTE(pioi.data);
      }
#endif
      switch(addr){
      case 0xff8604:  //high byte of FDC access
        //should check bit 8 = 0 (read)
        if (dma_mode & BIT_4){ //read sector counter (maintained by the DMA chip)
          return HIBYTE(dma_sector_count);
        }
        if (dma_mode & BIT_3){ // HD access
          LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                " - Reading high byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )
          return 0xff;
        }
        return 0xff;
      case 0xff8605:  //low byte of FDC access
        //should check bit 8 = 0, read
        if (dma_mode & BIT_4){ //read sector counter (maintained by the DMA chip)
          return LOBYTE(dma_sector_count);
        }

        if (dma_mode & BIT_3){ // HD access
          LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                  " - Reading low byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )
          return 0xff;
        }

        // Read FDC register
        switch (dma_mode & (BIT_1+BIT_2)){
          case 0:
          {
            int fn=floppy_current_drive();
            if (floppy_track_index_pulse_active()){
              fdc_str|=FDC_STR_T1_INDEX_PULSE;
            }else{
              // If not type 1 command we will get here, it is okay to clear
              // it as this bit is only for the DMA chip for type 2/3.
              fdc_str&=BYTE(~FDC_STR_T1_INDEX_PULSE);
            }
            if (floppy_type1_command_active){
              /* From Jorge Cwik
                The FDC has two different
                type of status. There is a "Type I" status after any Type I command,
                and there is a different "status" after types II & III commands. The
                meaning of some of the status bits is different (this probably you
                already know),  but the updating of these bits is different too.

                In a Type II-III status, the write protect bit is updated from the write
                protect signal only when trying to write to the disk (write sector
                or format track), otherwise is clear. This bit is static, once it was
                updated or cleared, it will never change until a new command is
                issued to the FDC.
              */
              fdc_str&=(~FDC_STR_WRITE_PROTECT);
              if (floppy_mediach[fn]){
                if (floppy_mediach[fn]/10!=1) fdc_str|=FDC_STR_WRITE_PROTECT;
              }else if (FloppyDrive[fn].ReadOnly){
                fdc_str|=FDC_STR_WRITE_PROTECT;
              }
              if (fdc_spinning_up){
                fdc_str&=BYTE(~FDC_STR_T1_SPINUP_COMPLETE);
              }else{
                fdc_str|=FDC_STR_T1_SPINUP_COMPLETE;
              }
            } // else it should be set in fdc_execute()
            if ((mfp_reg[MFPR_GPIP] & BIT_5)==0){
              LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                          " - Reading status register as "+Str(itoa(fdc_str,d2_t_buf,2)).LPad(8,'0')+
                          " ($"+HEXSl(fdc_str,2)+"), clearing IRQ"); )
              floppy_irq_flag=0;
              mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
            }
//            log_DELETE_SOON(Str("FDC: ")+HEXSl(old_pc,6)+" - reading FDC status register as $"+HEXSl(fdc_str,2));
/*
            LOG_ONLY( if (mode==STEM_MODE_CPU) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                            " - Read status register as $"+HEXSl(fdc_str,2)); )
*/
            return fdc_str;
          }
          case 2:
            return fdc_tr; //track register
          case 4:
            return fdc_sr; //sector register
          case 6:
            return fdc_dr; //data register
        }
        break;
      case 0xff8606:  //high byte of DMA status
        return 0x0;
      case 0xff8607:  //low byte of DMA status
        return BYTE(b11110000) | dma_status;
      case 0xff8609:  //high byte of DMA pointer
        return (BYTE)((dma_address&0xff0000)>>16);
      case 0xff860b:  //mid byte of DMA pointer
        return (BYTE)((dma_address&0xff00)>>8);
      case 0xff860d:  //low byte of DMA pointer
        return (BYTE)((dma_address&0xff));
      case 0xff860e: //frequency/density control
      {
        if (FloppyDrive[floppy_current_drive()].STT_File) return 0;

        TFloppyImage *floppy=&(FloppyDrive[floppy_current_drive()]);
        return BYTE((floppy->BytesPerSector * floppy->SectorsPerTrack)>7000);
      }
      case 0xff860f: //high byte of frequency/density control?
        return 0;
      }
      break;
    }case 0xff8200:{      //----------------------------------- shifter
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
/*
allowed addresses

FF8200 - FF820F   SHIFTER
FF8240 - FF827F   palette, res
*/
      if (addr>=0xff8240 && addr<0xff8260){  //palette
        int n=addr-0xff8240;n/=2;
        if (addr&1) return LOBYTE(STpal[n]);
        else return HIBYTE(STpal[n]);
      }else if (addr>0xff820f && addr<0xff8240){ //forbidden gap
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }else if (addr>0xff827f){  //forbidden area after SHIFTER
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }else{
        switch(addr){
        case 0xff8201:  //high byte of screen memory address
          return LOBYTE(HIWORD(xbios2));
        case 0xff8203:  //mid byte of screen memory address
          return HIBYTE(LOWORD(xbios2));
        case 0xff820d:  //low byte of screen memory address
          return LOBYTE(xbios2);
        case 0xff8205:  //high byte of screen draw pointer
        case 0xff8207:  //mid byte of screen draw pointer
        case 0xff8209:{  //low byte of screen draw pointer
          MEM_ADDRESS sdp;
          if (scan_y<shifter_first_draw_line || scan_y>=shifter_last_draw_line){
            sdp=shifter_draw_pointer;
          }else{
            sdp=get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
            LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_VIDEO,Str("VIDEO: ")+HEXSl(old_pc,6)+
                        " - Read shifter draw pointer as $"+HEXSl(sdp,6)+
                        " on "+scanline_cycle_log()); )
          }
          return DWORD_B(&sdp, (2-(addr-0xff8205)/2) );    // change for big endian
        }
        case 0xff820a:  //synchronization mode
          if (shifter_freq==50) return b11111110;
          return b11111100;
        case 0xff820f:
          return (BYTE)shifter_fetch_extra_words;

        //// Unused bytes between $60 and $80 should return 0!
        case 0xff8260: //resolution
          return (BYTE)screen_res;
        case 0xff8264:  //hscroll no increase screen width
          return (BYTE)0;
        case 0xff8265:  //hscroll
          DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) shifter_hscroll_extra_fetch=(shifter_hscroll!=0);
          return (BYTE)shifter_hscroll;
        }
        // Below $10 - Odd bytes return value or 0, even bytes return 0xfe/0x7e
        // Above $40 - Unused return 0
        if (addr<=0xff820f && (addr & 1)==0) return 0xfe;
        return 0;
      }
      break;
    }case 0xff8000:{      //----------------------------------- MMU
      if (addr==0xff8001){
        if (mem_len>FOUR_MEGS) return MEMCONF_2MB | (MEMCONF_2MB << 2);
        return mmu_memory_configuration;
      }else if (addr>0xff800f){ //forbidden range
        exception(BOMBS_BUS_ERROR,EA_READ,addr);
      }else if (addr & 1){
        return 0;
      }
      return 0xff;
    case 0xffc100:
#ifdef DEBUG_BUILD
      if (addr==0xffc123) return (BYTE)runstate;
#endif
      if (emudetect_called){
        switch (addr){
          case 0xffc100: return BYTE(stem_version_text[0]-'0');
          case 0xffc101:
          {
            Str minor_ver=stem_version_text+2;
            for (int i=0;i<minor_ver.Length();i++){
              if (minor_ver[i]<'0' || minor_ver[i]>'9'){
                minor_ver.SetLength(i);
                break;
              }
            }
            int ver=atoi(minor_ver.RPad(2,'0'));
            return BYTE(((ver/10) << 4) | (ver % 10));
          }
          case 0xffc102: return BYTE(slow_motion);
          case 0xffc103: return BYTE(slow_motion_speed/10);
          case 0xffc104: return BYTE(fast_forward);
          case 0xffc105: return BYTE(n_cpu_cycles_per_second/1000000);
          case 0xffc106: return BYTE(0 DEBUG_ONLY(+1));
          case 0xffc107: return snapshot_loaded;
          case 0xffc108: return BYTE((100000/run_speed_ticks_per_second) >> 8);
          case 0xffc109: return BYTE((100000/run_speed_ticks_per_second) & 0xff);
          case 0xffc10a:
            if (avg_frame_time) return BYTE((((12000/avg_frame_time)*100)/shifter_freq) >> 8);
            return 0;
          case 0xffc10b:
            if (avg_frame_time) return BYTE((((12000/avg_frame_time)*100)/shifter_freq) & 0xff);
            return 0;
          case 0xffc10c: return HIBYTE(HIWORD(ABSOLUTE_CPU_TIME));
          case 0xffc10d: return LOBYTE(HIWORD(ABSOLUTE_CPU_TIME));
          case 0xffc10e: return HIBYTE(LOWORD(ABSOLUTE_CPU_TIME));
          case 0xffc10f: return LOBYTE(LOWORD(ABSOLUTE_CPU_TIME));

          case 0xffc110: return HIBYTE(HIWORD(cpu_time_of_last_vbl));
          case 0xffc111: return LOBYTE(HIWORD(cpu_time_of_last_vbl));
          case 0xffc112: return HIBYTE(LOWORD(cpu_time_of_last_vbl));
          case 0xffc113: return LOBYTE(LOWORD(cpu_time_of_last_vbl));

          case 0xffc114: return HIBYTE(HIWORD(cpu_timer_at_start_of_hbl));
          case 0xffc115: return LOBYTE(HIWORD(cpu_timer_at_start_of_hbl));
          case 0xffc116: return HIBYTE(LOWORD(cpu_timer_at_start_of_hbl));
          case 0xffc117: return LOBYTE(LOWORD(cpu_timer_at_start_of_hbl));

          case 0xffc118: return HIBYTE(short(scan_y));
          case 0xffc119: return LOBYTE(short(scan_y));
          case 0xffc11a: return emudetect_write_logs_to_printer;
          case 0xffc11b: return emudetect_falcon_mode;
          case 0xffc11c: return BYTE((emudetect_falcon_mode_size-1) + (emudetect_falcon_extra_height ? 2:0));
          case 0xffc11d: return emudetect_overscans_fixed;
        }
        if (addr<0xffc120) return 0;
      }
    }default:{ //not in allowed area
      exception(BOMBS_BUS_ERROR,EA_READ,addr);
    }       //end case
  }       //end switch
  return 0xff;


#endif


}

//---------------------------------------------------------------------------
WORD ASMCALL io_read_w(MEM_ADDRESS addr)
{
  if (addr>=0xff8240 && addr<0xff8260){  //palette
    DEBUG_CHECK_READ_IO_W(addr);
    int n=addr-0xff8240;n/=2;

#if defined(SSE_MMU_ROUNDING_BUS)
    cpu_cycles&=-4; // Shifter access -> wait states possible
#endif
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif

#if defined(SSE_SHIFTER_PALETTE_NOISE)
/*  When one reads the palette on a STF, the high bit of each nibble
    isn't always 0, nor always 1.
    The value could have something to do with the last values on the
    data bus.
    v3.7
    Cases:
    1) Random noise when PC is set on the palette would cause
    a crash of Union demo (illegal instead of bus error).
    Steem uses the direct "lpfetch" pointers however, so we have no
    problem with this for now.
    2) Random noise worsens Awesome 4
	nop                                              ; 00CEDE: 4E71 
	nop                                              ; 00CEE0: 4E71 
	lea $ff8240,a0                                   ; 00CEE2: 41F9 00FF 8240 
	move.w #$e4,d2                                   ; 00CEE8: 343C 00E4 
	move.w (a3)+,(a0)                                ; 00CEEC: 309B 
data bus: ?
	addq.w #1,$ff8240                                ; 00CEEE: 5279 00FF 8240 
data bus: that new value?
	addq.w #1,$ff8240                                ; 00CEF4: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CEFA: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF00: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF06: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF0C: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF12: 5279 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF18: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF1E: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF24: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF2A: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF30: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF36: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF3C: 5379 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF42: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF48: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF4E: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF54: 5279 00FF 8240 
	addq.w #1,$ff8240                                ; 00CF5A: 5279 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF60: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF66: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF6C: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF72: 5379 00FF 8240 
	subq.w #1,$ff8240                                ; 00CF78: 5379 00FF 8240 
      Emulation of this intro wasn't satisfying without that noise either, we
      see that noise is necessary, only not random.
    3) Random noise seems to fix UMD8730
	move.l d7,-(a7)                                  ; 033D2C: 2F07 
	lea $340d2,a2                                    ; 033D2E: 45F9 0003 40D2 
	lea $340e2,a3                                    ; 033D34: 47F9 0003 40E2 
	clr.l d0                                         ; 033D3A: 4280 
	clr.l d1                                         ; 033D3C: 4281 
	clr.l d3                                         ; 033D3E: 4283 
	clr.l d4                                         ; 033D40: 4284 
	clr.l d5                                         ; 033D42: 4285 
	clr.l d6                                         ; 033D44: 4286 
	move.w #$f,d7                                    ; 033D46: 3E3C 000F 
Loop palettes
	clr.l d2                                         ; 033D4A: 4282 
	movea.w (a0),a4                                  ; 033D4C: 3850 
Reading palette
	move.w a4,d6                                     ; 033D4E: 3C0C 
	andi.w #$f,d6                                    ; 033D50: 0246 000F 
	move.b 0(a2,D6.W),d0                             ; 033D54: 1032 6000 
	move.w a4,d6                                     ; 033D58: 3C0C 
	lsr.b #4,d6                                      ; 033D5A: E80E 
	andi.w #$f,d6                                    ; 033D5C: 0246 000F 
	move.b 0(a2,D6.W),d1                             ; 033D60: 1232 6000 
	move.w a4,d6                                     ; 033D64: 3C0C 
	lsr.w #8,d6                                      ; 033D66: E04E 
	andi.w #$f,d6                                    ; 033D68: 0246 000F 
	move.b 0(a2,D6.W),d2                             ; 033D6C: 1432 6000 
	movea.w (a1)+,a4                                 ; 033D70: 3859 
	move.w a4,d6                                     ; 033D72: 3C0C 
	andi.w #$f,d6                                    ; 033D74: 0246 000F 
	move.b 0(a2,D6.W),d3                             ; 033D78: 1632 6000 
	move.w a4,d6                                     ; 033D7C: 3C0C 
	lsr.b #4,d6                                      ; 033D7E: E80E 
	andi.w #$f,d6                                    ; 033D80: 0246 000F 
	move.b 0(a2,D6.W),d4                             ; 033D84: 1832 6000 
	move.w a4,d6                                     ; 033D88: 3C0C 
	lsr.w #8,d6                                      ; 033D8A: E04E 
	andi.w #$f,d6                                    ; 033D8C: 0246 000F 
	move.b 0(a2,D6.W),d5                             ; 033D90: 1A32 6000 
	cmp.b d3,d0                                      ; 033D94: B003 
	beq.s +12 {$033DA4}                              ; 033D96: 670C 
	bgt.s +6 {$033DA0}                               ; 033D98: 6E06 
	addi.b #$1,d0                                    ; 033D9A: 0600 0001 
	bra.s +4 {$033DA4}                               ; 033D9E: 6004 
	subi.b #$1,d0                                    ; 033DA0: 0400 0001 
	cmp.b d4,d1                                      ; 033DA4: B204 
	beq.s +12 {$033DB4}                              ; 033DA6: 670C 
	bgt.s +6 {$033DB0}                               ; 033DA8: 6E06 
	addi.b #$1,d1                                    ; 033DAA: 0601 0001 
	bra.s +4 {$033DB4}                               ; 033DAE: 6004 
	subi.b #$1,d1                                    ; 033DB0: 0401 0001 
	cmp.b d5,d2                                      ; 033DB4: B405 
	beq.s +12 {$033DC4}                              ; 033DB6: 670C 
	bgt.s +6 {$033DC0}                               ; 033DB8: 6E06 
	addi.b #$1,d2                                    ; 033DBA: 0602 0001 
	bra.s +4 {$033DC4}                               ; 033DBE: 6004 
	subi.b #$1,d2                                    ; 033DC0: 0402 0001 
	move.b 0(a3,D2.W),d2                             ; 033DC4: 1433 2000 
	lsl.b #4,d2                                      ; 033DC8: E90A 
	or.b 0(a3,D1.W),d2                               ; 033DCA: 8433 1000 
	lsl.w #4,d2                                      ; 033DCE: E94A 
	or.b 0(a3,D0.W),d2                               ; 033DD0: 8433 0000 
	move.w d2,(a0)+                                  ; 033DD4: 30C2 
Writing palette, ++
	dbra d7,-142 {$033D4A}                           ; 033DD6: 51CF FF72 
Loop on
	move.l (a7)+,d7                                  ; 033DDA: 2E1F 
	rts                                              ; 033DDC: 4E75 
Done one cycle of all palettes
    4) Forest HW test has a double palette test, noise seems to be expected
       for $555.
	move.b #$0,$8260.w                               ; 014BD6: 11FC 0000 8260 
	move.b #$2,$820a.w                               ; 014BDC: 11FC 0002 820A 
	move.w #$555,d0                                  ; 014BE2: 303C 0555 
	move.w #$aaa,d1                                  ; 014BE6: 323C 0AAA 
	move.w d0,$825e.w                                ; 014BEA: 31C0 825E 
	move.w $825e.W,d2                                ; 014BEE: 3438 825E 
	andi.w #$fff,d2                                  ; 014BF2: 0242 0FFF 
	move.w d1,$825e.w                                ; 014BF6: 31C1 825E 
	move.w $825e.W,d3                                ; 014BFA: 3638 825E 
	andi.w #$fff,d3                                  ; 014BFE: 0243 0FFF 
	cmp.w d0,d2                                      ; 014C02: B440 
	bne.s +8 {$014C0E}                               ; 014C04: 6608 
	cmp.w d1,d3                                      ; 014C06: B641 
	bne.s +4 {$014C0E}                               ; 014C08: 6604 
	moveq #1,d4                                      ; 014C0A: 7801 
"STE"
	bra.s +2 {$014C10}                               ; 014C0C: 6002 
	moveq #0,d4                                      ; 014C0E: 7800 

   v3.7
   We can't emulate this correctly yet, so we will hack it instead:
   - Do the random thing only for UMD8730
   - Try to update "data bus" for Awesome 04
   It's not bad to target ir, as data bus would depend on those. 
   
*/
    WORD palette=STpal[n];
    if(ST_TYPE!=STE && OPTION_HACKS)
    {
      if(ir==0x3850)
        palette|=(0xF888)&(rand()); // UMD8730
#if defined(SSE_CPU_DATABUS)
      else if(ir==0x5279 || ir==0x5379)
        palette|=0xF888&dbus; // Awesome 04
#endif
    }
#if defined(SSE_BOILER_TRACE_CONTROL) // double condition, IO + Video
    if ( ((1<<14)&d2_dpeek(FAKE_IO_START+24))
      && (logsection_enabled[LOGSECTION_VIDEO]) )
      TRACE_LOG("PC %X read PAL %X %X\n",old_pc,n,palette);
#endif
    return palette;
#else
    return STpal[n];
#endif
  }else{
    io_word_access=true;
    WORD x=WORD(io_read_b(addr) << 8);
    x|=io_read_b(addr+1);
    io_word_access=0;
#if defined(SSE_DEBUG_TRACE_IO)
#if defined(SSE_BOILER_TRACE_CONTROL)
    if (((1<<14)&d2_dpeek(FAKE_IO_START+24))
    // we add conditions address range - logsection enabled

//      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_INTERRUPTS] ) //mfp
      && ( (addr&0xffff00)!=0xFFFA00 || logsection_enabled[LOGSECTION_MFP] ) //mfp
      && ( (addr&0xffff00)!=0xfffc00 || logsection_enabled[LOGSECTION_IKBD] ) //acia
      && ( (addr&0xffff00)!=0xff8600 || logsection_enabled[LOGSECTION_FDC] ) //dma
      && ( (addr&0xffff00)!=0xff8800 || logsection_enabled[LOGSECTION_SOUND] )//psg
      && ( (addr&0xffff00)!=0xff8900 || logsection_enabled[LOGSECTION_SOUND] )//dma
      && ( (addr&0xffff00)!=0xff8a00 || logsection_enabled[LOGSECTION_BLITTER] )
      && ( (addr&0xffff00)!=0xff8200 || logsection_enabled[LOGSECTION_VIDEO] )//shifter
      && ( (addr&0xffff00)!=0xff8000 || ( ((1<<13)&d2_dpeek(FAKE_IO_START+24)) ))//MMU

      ) 
#endif
    TRACE_LOG("PC %X read word %X at %X\n",old_pc,x,addr);
#endif
    return x;
  }

}
//---------------------------------------------------------------------------
DWORD ASMCALL io_read_l(MEM_ADDRESS addr)
{
/*  SS same way for long accesses, so that a .L read will resolve in 4 .B reads.
    Notice the timing trick. At CPU emu level, the read is counted for 8 
    cycles right before coming here, the adjustment is here where it counts.
*/
  INSTRUCTION_TIME(-4);
  DWORD x=io_read_w(addr) << 16;
  INSTRUCTION_TIME(4);
  x|=io_read_w(addr+2);
  return x;
}

#undef LOGSECTION
