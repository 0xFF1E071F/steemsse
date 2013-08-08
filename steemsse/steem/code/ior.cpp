/*---------------------------------------------------------------------------
FILE: ior.cpp
MODULE: emu
DESCRIPTION: I/O address reads. This file contains crucial core functions
that deal with reads from ST I/O addresses ($ff8000 onwards).
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_INFO)
#pragma message("Included for compilation: ior.cpp")
#endif

#define LOGSECTION LOGSECTION_IO

#if !defined(STEVEN_SEAGAL) || !defined(SS_SHIFTER_SDP_READ) \
  || defined(SS_SHIFTER_DRAW_DBG) || !defined(SS_STRUCTURE)

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

#ifdef ONEGAME
  if (addr>=OG_TEXT_ADDRESS && addr<OG_TEXT_ADDRESS+OG_TEXT_LEN){
    return BYTE(OG_TextMem[addr-OG_TEXT_ADDRESS]);
  }
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_IOR)
/*  We've rewritten this full block because we don't want to directly return
    values, we think it's better style to assign the value to a variable 
    (ior_byte) then return it at the end, with eventual trace.
    Not for blocks that normally aren't compiled.
*/

  ASSERT( (addr&0xFFFFFF)==addr ); // done in 'peek' part

  BYTE ior_byte=0xff; // default value

#if defined(SS_MMU_WAKE_UP_IO_BYTES_R)
  bool adjust_cycles=!io_word_access && MMU.OnMmuCycles(LINECYCLES);
  if(adjust_cycles)
    cpu_cycles+=-2; // = +2 cycles
#endif

  // Main switch: address groups
  switch(addr&0xffff00) 
  {

    ///////////////////////////
    // ACIAs (IKBD and MIDI) //
    ///////////////////////////

#undef LOGSECTION 
#define LOGSECTION LOGSECTION_IKBD

/*  For ACIA emulation, we prefer to explicitly use registers instead of apart
    variables.
*/

    case 0xfffc00:      
/*  
    Bus jam:
    In Hatari, each access causes 8 cycles of wait states.
    In v1.7.0, more complicated
    In Steem, we count 6 cycles, but with rounding.
    We removed extra cycles since v3.4 as it broke Spectrum 512
    If we want to make it more precise, we probably must act on
    different aspects at once.
*/
      if( 
#if defined(DEBUG_BUILD)
        mode==STEM_MODE_CPU && // no cycles when boiler is reading
#endif
        (!io_word_access||!(addr & 1)) ) //Only cause bus jam once per word
      {
        BYTE wait_states=6;
#if !defined(SS_ACIA_BUS_JAM_NO_WOBBLE)
        wait_states+=(8000000-(ABSOLUTE_CPU_TIME-shifter_cycle_base))%10;
#endif

#if defined(SS_SHIFTER_EVENTS)
        VideoEvents.Add(scan_y,LINECYCLES,'j',wait_states);
#endif
        BUS_JAM_TIME(wait_states); // =INSTRUCTION_TIME_ROUND
      }

      switch (addr) // ACIA registers
      {

      // Read ACIA keyboard read status
      case 0xfffc00:

/*  
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

#if !defined(SS_ACIA_USE_REGISTERS) || defined(SS_ACIA_TEST_REGISTERS)

        // Byte is built bit by bit right now at read
        ior_byte=0; // starting from all bits clear

        // BIT 0 - RX full
        if(ACIA_IKBD.overrun || ACIA_IKBD.rx_not_read) // ACIA_IKBD = acia[0]
          ior_byte|=BIT_0;

        // BIT 1 - TX ready (empty)
        if(!ACIA_IKBD.tx_flag)
          ior_byte|=BIT_1;

        // BITS 2 - DCD, 3 - CTS, 4 Framing error: ignored

        // BIT 5 - Overrun
        if (ACIA_IKBD.overrun==ACIA_OVERRUN_YES) 
          ior_byte|=BIT_5; 

        // BIT 6 - parity error: ignored

        // bit 7 - IRQ
        if(ACIA_IKBD.irq) 
        {
          ASSERT( ACIA_IKBD.rx_irq_enabled );
          ior_byte|=BIT_7;
        }

#if defined(SS_DEBUG) && defined(SS_ACIA_TEST_REGISTERS)
        if(ior_byte!=ACIA_IKBD.SR)
          TRACE_LOG("ACIA IKBD SR built %X persistent SR %X\n",ior_byte,ACIA_IKBD.SR);
#endif

#endif

#if defined(SS_ACIA_USE_REGISTERS)
        ior_byte=ACIA_IKBD.SR; 
#endif

#if defined(SS_ACIA_TDR_COPY_DELAY)
/*    
Double buffering means that two bytes may be written very fast one after
the other on TDR. The first byte will be transferred almost at once into
TDRS. Subsequent bytes will have to wait the full serial transfer time.

'Almost' doesn't mean instant, for a few cycles (32?, which would translate
into 512 MC68000 cycles), the TDRE bit in SR will be cleared but TDR will 
take a byte.
This feature was in Steem 3.2 but I made it disappear in some SSE versions!
Refix v3.5.3
*/
        if(ACIA_IKBD.last_tx_write_time
          &&ACT-ACIA_IKBD.last_tx_write_time<ACIA_CYCLES_NEEDED_TO_START_TX)
        {
          TRACE_LOG("ACIA SR TDRE not set yet (%d)\n",ACT-ACIA_IKBD.last_tx_write_time);
          ior_byte&=~BIT_1; // fixes Nightdawn
        }
#endif

        break;

      // ACIA keyboard read data
      case 0xfffc02:
      {
        DEBUG_ONLY( if (mode!=STEM_MODE_CPU) return ACIA_IKBD.data; ) // boiler

        // Update status BIT 0 (RX full)
#if !defined(SS_ACIA_USE_REGISTERS) || defined(SS_ACIA_TEST_REGISTERS)
        ACIA_IKBD.rx_not_read=0; 
        LOG_ONLY( bool old_irq=ACIA_IKBD.irq; )
#endif
#if defined(SS_ACIA_REGISTERS)
//        ACIA_IKBD.SR&=~BIT_0; // ACIA bugfix 3.5.2, bit cleared only if no OVR
#endif
        // Update status BIT 5 (overrun)
        if(ACIA_IKBD.overrun==ACIA_OVERRUN_COMING) // keep this, it's right
        {
          ACIA_IKBD.overrun=ACIA_OVERRUN_YES;
#if defined(SS_ACIA_REGISTERS)
          ACIA_IKBD.SR|=BIT_5; // set overrun (only now, conform to doc)
          if(ACIA_IKBD.CR&BIT_7) // irq enabled
            ACIA_IKBD.SR|=BIT_7; // there's a new IRQ when overrun bit is set
#endif
#if !defined(SS_ACIA_USE_REGISTERS) || defined(SS_ACIA_TEST_REGISTERS)
          if(ACIA_IKBD.rx_irq_enabled) 
            ACIA_IKBD.irq=true;
#endif
          LOG_ONLY( log_to_section(LOGSECTION_IKBD,EasyStr("IKBD: ")+HEXSl(old_pc,6)+
                              " - OVERRUN! Read data ($"+HEXSl(ACIA_IKBD.data,2)+
                              "), changing ACIA IRQ bit from "+old_irq+" to "+ACIA_IKBD.irq); )
        }
        // no overrun, normal
        else
        {
/*
  "The Overrun indication is reset after the reading of data from the 
  Receive Data Register."
*/
          ACIA_IKBD.overrun=ACIA_OVERRUN_NO;

#if defined(SS_ACIA_REGISTERS)
          ACIA_IKBD.SR&=~BIT_0; // ACIA bugfix 3.5.2, bit cleared only if no OVR
          ACIA_IKBD.SR&=~BIT_5;//normally first read SR (TODO)
          if(!( ACIA_IKBD.IrqForTx() && ACIA_IKBD.SR&BIT_1) )
            ACIA_IKBD.SR&=~BIT_7; // clear IRQ bit, normally first read SR
#endif

#if !defined(SS_ACIA_USE_REGISTERS) || defined(SS_ACIA_TEST_REGISTERS)
          // IRQ should be off for receive, but could be set for tx empty interrupt
          ACIA_IKBD.irq=(ACIA_IKBD.tx_irq_enabled && ACIA_IKBD.tx_flag==0);
#endif

          LOG_ONLY( if (ACIA_IKBD.irq!=old_irq) log_to_section(LOGSECTION_IKBD,Str("IKBD: ")+
            HEXSl(old_pc,6)+" - Read data ($"+HEXSl(ACIA_IKBD.data,2)+
            "), changing ACIA IRQ bit from "+old_irq+" to "+ACIA_IKBD.irq); )

        }

#if defined(SS_ACIA_TEST_REGISTERS) && defined(SS_ACIA_DOUBLE_BUFFER_RX)
        ASSERT( ACIA_IKBD.RDR==ACIA_IKBD.data );
//        ASSERT( !( (ACIA_IKBD.SR&BIT_7)||(ACIA_MIDI.SR&BIT_7) )==!(ACIA_IKBD.irq||ACIA_MIDI.irq) );
#endif
/*
"The nondestructive read cycle (...) although the data in the
Receiver Data Register is retained.
*/        
#if defined(SS_ACIA_USE_REGISTERS)
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,
          !( (ACIA_IKBD.SR&BIT_7) || (ACIA_MIDI.SR&BIT_7)) );
        ior_byte=ACIA_IKBD.RDR;
#else
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
        ior_byte=ACIA_IKBD.data;
#endif
      
#if defined(SS_DEBUG) && defined(SS_ACIA_TEST_REGISTERS) \
  && defined(SS_IKBD_TRACE_CPU_READ)
        static BYTE previous_read; // avoid trace overflow when polling
        if(ior_byte!=previous_read)
          TRACE_LOG("CPU reads ACIA IKBD %X\n",ior_byte);
        previous_read=ior_byte;
#elif defined(SS_IKBD_TRACE_CPU_READ2)
          TRACE_LOG("CPU reads ACIA IKBD %X\n",ior_byte);
#endif
        break;
      }


#undef LOGSECTION 
#define LOGSECTION LOGSECTION_MIDI

      // ACIA MIDI read status
      case 0xfffc04:  // status

#if defined(SS_ACIA_USE_REGISTERS)
        ior_byte=ACIA_MIDI.SR; 
#else
        ior_byte=0;
        if (ACIA_MIDI.rx_not_read || ACIA_MIDI.overrun==ACIA_OVERRUN_YES) 
          ior_byte|=BIT_0; //full bit
        if (ACIA_MIDI.tx_flag==0) 
          ior_byte|=BIT_1; //empty bit
        if (ACIA_MIDI.irq) 
          ior_byte|=BIT_7; //irq bit
        if (ACIA_MIDI.overrun==ACIA_OVERRUN_YES) 
          ior_byte|=BIT_5; //overrun

#if defined(SS_ACIA_TEST_REGISTERS)
        if(ior_byte!=ACIA_MIDI.SR || !(ior_byte&2) || !(ACIA_MIDI.SR&2))
          TRACE_LOG("ACIA MIDI SR old way %X new way %X\n",ior_byte,ACIA_MIDI.SR);
#endif

#endif      
        break;

      // ACIA MIDI read data
      case 0xfffc06:
        DEBUG_ONLY(if (mode!=STEM_MODE_CPU) return ACIA_MIDI.data);
#if defined(SS_ACIA_REGISTERS)
#if defined(SS_MIDI_TRACE_BYTES_IN)
        TRACE_LOG("MIDI Read RDR %X\n",ACIA_MIDI.RDR);
#endif
//        ACIA_MIDI.SR&=~BIT_0; // ACIA bugfix 3.5.2, not if overrun
#endif
#if !defined(SS_ACIA_USE_REGISTERS)
        ACIA_MIDI.rx_not_read=0;
#endif
        if (ACIA_MIDI.overrun==ACIA_OVERRUN_COMING){
          ACIA_MIDI.overrun=ACIA_OVERRUN_YES;
#if !defined(SS_ACIA_USE_REGISTERS)
          if (ACIA_MIDI.rx_irq_enabled) 
            ACIA_MIDI.irq=true;
#endif
#if defined(SS_ACIA_REGISTERS)
          ACIA_MIDI.SR|=BIT_5; // set overrun (only now, conform to doc)
          if(ACIA_MIDI.CR&BIT_7) // irq enabled
            ACIA_MIDI.SR|=BIT_7; // set IRQ  
#endif
          TRACE_LOG("ACIA MIDI overrun\n");
        }
        // normal
        else
        {
          ACIA_MIDI.overrun=ACIA_OVERRUN_NO;
#if defined(SS_ACIA_USE_REGISTERS)
          ACIA_MIDI.SR&=~BIT_0; // ACIA bugfix 3.5.2
#endif
          // IRQ should be off for receive, but could be set for tx empty interrupt
#if !defined(SS_ACIA_USE_REGISTERS)
          ACIA_MIDI.irq=(ACIA_MIDI.tx_irq_enabled && ACIA_MIDI.tx_flag==0);
#endif
#if defined(SS_ACIA_REGISTERS)
          ACIA_MIDI.SR&=~BIT_5;// ACIA bugfix 3.5.3
          if(!( ACIA_MIDI.IrqForTx() && (ACIA_MIDI.SR&BIT_1) )) // TDRE
            ACIA_MIDI.SR&=~BIT_7; // clear IRQ bit
#endif
        }

#if defined(SS_ACIA_USE_REGISTERS)
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,
          !( (ACIA_IKBD.SR&BIT_7) || (ACIA_MIDI.SR&BIT_7)) );
        ior_byte=ACIA_MIDI.RDR;
#else
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
        ior_byte=ACIA_MIDI.data;
#endif
        log_to(LOGSECTION_MIDI,Str("MIDI: ")+HEXSl(old_pc,6)+" - Read $"+
                HEXSl(ACIA_MIDI.data,6)+" from MIDI ACIA data register");
        
        break;
      }

      break;

#undef LOGSECTION 
#define LOGSECTION LOGSECTION_IO



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

        // Only cause bus jam once per word (should this be after the read?)
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) if (io_word_access==0 || (addr & 1)==1) BUS_JAM_TIME(4);

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
            ior_byte=BYTE(mfp_reg[MFPR_GPIP] & ~mfp_reg[MFPR_DDR]);
            ior_byte|=BYTE(mfp_gpip_input_buffer & mfp_reg[MFPR_DDR]);
          }
          else if (addr<0xfffa30)
          {
            int n=(addr-0xfffa01) >> 1;
            if (n>=MFPR_TADR && n<=MFPR_TDDR){ //timer data registers
              mfp_calc_timer_counter(n-MFPR_TADR);
              ior_byte=BYTE(mfp_timer_counter[n-MFPR_TADR]/64);
              if (n==MFPR_TBDR){
                if (mfp_get_timer_control_register(1)==8){
                  // Timer B is in event count mode, check if it has counted down since the start of
                  // this instruction. Due to MFP delays this very, very rarely gets changed under 4
                  // cycles from the point of the signal.
                  if ((ABSOLUTE_CPU_TIME-time_of_next_timer_b) > 4){
                    if (!ior_byte)
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
#if defined(SS_STF)
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
#if defined(SS_SHIFTER_EVENTS)
      VideoEvents.Add(scan_y,LINECYCLES,'b',((addr-0xff8a00)<<8)|ior_byte);
#endif
      break;

    ///////////////////
    // STE DMA Sound //
    ///////////////////

#undef  LOGSECTION
#define LOGSECTION LOGSECTION_SOUND

    case 0xff8900:    
      if(addr>0xff893f
#if defined(SS_STF)
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

#if defined(SS_DEBUG)
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

      if(!(ioaccess & IOACCESS_FLAG_PSG_BUS_JAM_R))
      {
        DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) BUS_JAM_TIME(4);
        ioaccess|=IOACCESS_FLAG_PSG_BUS_JAM_R;
      }
      if((addr & 1) && io_word_access)
        ;// odd addresses ignored on word writes
      else if(!(addr & 2))
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

#if defined(SS_DMA) // taken out of here, in SSEFloppy
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

#if defined(SS_SHIFTER_IO)
      ior_byte=Shifter.IORead(addr);
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
#endif//SS-Shifter

    /////////////////
    // MMU for RAM //
    /////////////////

    case 0xff8000:
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
 
    case 0xffc100:
#ifdef DEBUG_BUILD
      if(addr==0xffc123) return (BYTE)runstate;
#endif
      if(emudetect_called){
        if (addr<0xffc120) 
          ior_byte= 0;
        else switch (addr){
          case 0xffc100: ior_byte=BYTE(stem_version_text[0]-'0'); break;
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

          case 0xffc118: ior_byte= HIBYTE( (short) (scan_y)); break;
          case 0xffc119: ior_byte= LOBYTE( (short) (scan_y)); break;
          case 0xffc11a: ior_byte= emudetect_write_logs_to_printer; break;
          case 0xffc11b: ior_byte= emudetect_falcon_mode; break;
          case 0xffc11c: ior_byte= BYTE((emudetect_falcon_mode_size-1) + (emudetect_falcon_extra_height ? 2:0)); break;
          case 0xffc11d: ior_byte= emudetect_overscans_fixed; break;
        }//sw
        break;
    }
    default: //not in allowed area
      exception(BOMBS_BUS_ERROR,EA_READ,addr);
  }//sw

#if defined(SS_DEBUG_TRACE_IO)
  if(!io_word_access
    && (addr&0xFFFF00)!=0xFFFA00 // many MFP reads
    && addr!=0xFFFC02  // if IKBD data polling...
    ) 
    TRACE_LOG("PC %X IOR.B %X = %X\n",pc-2,addr,ior_byte);
#endif

#if defined(SS_MMU_WAKE_UP_IO_BYTES_R)
  if(adjust_cycles)
    cpu_cycles+=2; 
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
#ifdef _DEBUG_BUILD
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

#if defined(STEVEN_SEAGAL) && defined(SS_MMU_WAKE_UP_IOR_HACK)

  WORD return_value;
  int CyclesIn=LINECYCLES;
  if(MMU.OnMmuCycles(CyclesIn))
    cpu_cycles-=2; // - = + !!!!

  if (addr>=0xff8240 && addr<0xff8260){  //palette
    DEBUG_CHECK_READ_IO_W(addr);
    int n=addr-0xff8240;n/=2;
    return_value=STpal[n];
  }else{
    io_word_access=true;
    WORD x=WORD(io_read_b(addr) << 8);
    x|=io_read_b(addr+1);
    io_word_access=0;
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG_TRACE_IO)
    TRACE_LOG("PC %X read word %X at %X\n",old_pc,x,addr);
#endif
    return_value=x;
  }
  if(MMU.OnMmuCycles(CyclesIn))
    cpu_cycles+=2;
  return return_value;

#else

  if (addr>=0xff8240 && addr<0xff8260){  //palette
    DEBUG_CHECK_READ_IO_W(addr);
    int n=addr-0xff8240;n/=2;
    return STpal[n];
  }else{
    io_word_access=true;
    WORD x=WORD(io_read_b(addr) << 8);
    x|=io_read_b(addr+1);
    io_word_access=0;
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG_TRACE_IO)
    TRACE_LOG("PC %X read word %X at %X\n",old_pc,x,addr);
#endif
    return x;
  }
#endif  
}
//---------------------------------------------------------------------------
DWORD ASMCALL io_read_l(MEM_ADDRESS addr)
{
  INSTRUCTION_TIME(-4);
  DWORD x=io_read_w(addr) << 16;
  INSTRUCTION_TIME(4);
  x|=io_read_w(addr+2);
  return x;
}

#undef LOGSECTION
