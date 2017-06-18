/*---------------------------------------------------------------------------
FILE: blitter.cpp
MODULE: emu
DESCRIPTION: Emulation of the STE only blitter chip.
SS: The Mega ST also has a blitter. We emulate this.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: blitter.cpp")
#endif

#if defined(SSE_BUILD)
TBlitter Blit;
#endif

#if defined(SSE_BLT_390)
#define BLITTER_START_WAIT (4)
#define BLITTER_END_WAIT (4)
#define GRAB_BUS_TIMING (4)
#else // Steem 3.2
#define BLITTER_START_WAIT 8//8
#define BLITTER_END_WAIT 0//0
#endif

#define LOGSECTION LOGSECTION_BLITTER
//---------------------------------------------------------------------------
WORD Blitter_DPeek(MEM_ADDRESS ad)
{
  ad&=0xffffff;
  if (ad>=himem){
    if (ad>=MEM_IO_BASE){
      WORD RetVal=0xffff;
#pragma warning(disable: 4611) //390
      TRY_M68K_EXCEPTION
#pragma warning(default: 4611)
        RetVal=io_read_w(ad);
      CATCH_M68K_EXCEPTION
      END_M68K_EXCEPTION
      return RetVal;
    }
    if (ad>=rom_addr){
      if (ad<rom_addr+tos_len) return ROM_DPEEK(ad-rom_addr);
    }
    if (ad>=MEM_EXPANSION_CARTRIDGE){
      if (cart && ad<MEM_EXPANSION_CARTRIDGE + 128*1024) return CART_DPEEK(ad-MEM_EXPANSION_CARTRIDGE);
    }
#if defined(SSE_MMU_MONSTER_ALT_RAM)
    if(abus<MMU.MonSTerHimem)
    {
      return DPEEK(ad);
    }
#endif
  }else{
    return DPEEK(ad);
  }
  return 0xffff;
}
//---------------------------------------------------------------------------
void Blitter_DPoke(MEM_ADDRESS abus,WORD x)
{
  abus&=0xffffff;
  if (abus>=MEM_IO_BASE){
#pragma warning(disable: 4611) //390
    TRY_M68K_EXCEPTION
#pragma warning(default: 4611)
      io_write_w(abus,x); // SS to the palette: Illusion, RGBeast...
    CATCH_M68K_EXCEPTION
    END_M68K_EXCEPTION
  }else if (abus>=MEM_FIRST_WRITEABLE && abus<himem){
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DPEEK(abus)=x;
    DEBUG_CHECK_WRITE_W(abus);
#else
    DEBUG_CHECK_WRITE_W(abus);
    DPEEK(abus)=x;
#endif
  }
#if defined(SSE_MMU_MONSTER_ALT_RAM)
  else if(abus<MMU.MonSTerHimem)
  {
    DEBUG_CHECK_WRITE_W(abus);
    DPEEK(abus)=x;
  }
#endif
}
//---------------------------------------------------------------------------
inline void Blitter_ReadSource(MEM_ADDRESS SrcAdr)
{
/*
  SrcBuffer is a DWORD (32bit). Use:

  Finally, let's look at the case when the source and destination blocks are
  not bit-aligned.  In this case we may  need to  read the  first two source
  words into  the 32-bit source buffer and use the 16 bits that line up with
  the appropriate bits of  the destination,  as specified  by the  SKEW reg-
  ister.  When the next source word is read, the lower 16 bits of the source
  buffer is transferred to the upper 16 bits  and the  lower is  replaced by
  the new  data.   This process  is reversed  when the  source is being read
  from the right to the left (SOURCE X INCREMENT negative).  Since there are
  cases when  it may  be necessary for an extra source read  to be performed
  at the beginning of each line to "prime" the source buffer and  cases when
  it may  not be  necessary due  to the  choice of  end mask, a bit has been
  provided which forces the extra read.   The  FXSR (aka.  pre-fetch) bit in
  the SKEW register indicates, when set, that an extra source read should be
  performed at the beginning of each  line  to  "prime"  the  source buffer.
  Similarly the  NFSR (aka  post-flush) bit, when set, will prevent the last
  source read of the  line.   This read  may not  be necessary  with certain
  combinations of end masks and skews.  If the read is suppressed, the lower
  to upper half buffer transfer still occurs.   Also in  this case,  a read-
  modify-write cycle  is performed  on the destination for the last write of
  each line regardless of the value of the corresponding ENDMASK register.
*/
  if (Blit.SrcXInc>=0){
    Blit.SrcBuffer<<=16; //SS shift former value to the left
    Blit.SrcBuffer|=Blitter_DPeek(SrcAdr); //SS load new value on the right
  }else{
    Blit.SrcBuffer>>=16;
    Blit.SrcBuffer|=Blitter_DPeek(SrcAdr) << 16;
  }
}
//---------------------------------------------------------------------------
void Blitter_Start_Line()
{
  if (Blit.YCounter<=0){ // Blit finished?

#if defined(SSE_BLT_RESTART) && !defined(SSE_BLT_392)
    if(!Blit.Hog && Blit.Restarted)
      INSTRUCTION_TIME(4); 
    Blit.Restarted=false;
#endif

#if defined(SSE_BLT_380) 
//BLTBENCH.TOS, http://www.atari-forum.com/viewtopic.php?f=25&t=28424 (1st version)
    Blit.Hog=Blit.Busy=Blit.HasBus=false; 
#else
    Blit.Busy=false;
#endif
    dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Blitter_Start_Line changing GPIP bit from "+
            bool(mfp_reg[MFPR_GPIP] & MFP_GPIP_BLITTER_BIT)+" to 0");
    mfp_gpip_set_bit(MFP_GPIP_BLITTER_BIT,0);
#if !defined(SSE_BLT_380)
    Blit.HasBus=false;
#endif
#if BLITTER_END_WAIT!=0 
#if defined(SSE_BLT_380)
    INSTRUCTION_TIME(BLITTER_END_WAIT);
#else
    INSTRUCTION_TIME_ROUND(BLITTER_END_WAIT);
#endif
#endif

#if defined(SSE_BLT_390B)
/*  Record # blit cycles during which the CPU could work without
    accessing the bus. More like real emulation, but it has a cost.
*/
    Blit.BlitCycles=ACT-Blit.TimeAtBlit;
    ASSERT(Blit.BlitCycles>=0);
#endif
    dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" ------------- BLITTING DONE --------------");

#ifdef DEBUG_BUILD
    if (stop_on_blitter_flag && runstate==RUNSTATE_RUNNING){
      runstate=RUNSTATE_STOPPING;
      runstate_why_stop="Blitter completed an operation";
    }
#endif

  }else{ //prepare next line

    Blit.Mask=Blit.EndMask[0]; //SS mask for 1st word

    Blit.Last=0;

/*
  FXSR stands  for Force  eXtra Source Read.  When this bit is set one extra
  source read is performed at the  start  of  each  line  to  initialize the
  remainder portion source data latch. The  FXSR (aka.  pre-fetch) bit in
  the SKEW register indicates, when set, that an extra source read should be
  performed at the beginning of each  line  to  "prime"  the  source buffer.
*/ 
#if defined(SSE_BLT_COPY_LOOP) // do the prefetch in the regular copy loop
    Blit.BlittingPhase= (Blit.FXSR
      && ((Blit.Op % 5)!=0 &&(Blit.Hop>1 || (Blit.Hop==1 && Blit.Smudge))))
    ? TBlitter::PRIME : TBlitter::READ_SOURCE;
#else
    if (Blit.FXSR){
      abus=Blit.SrcAdr;
      Blitter_ReadSource(Blit.SrcAdr);
      Blit.SrcAdr+=Blit.SrcXInc;       
    }
#endif
  }
}

//---------------------------------------------------------------------------
void ASMCALL Blitter_Start_Now()
{
 
#if defined(SSE_BLT_390B) && !defined(SSE_BLT_MAIN_LOOP)
  Blit.TimeAtBlit=ACT;
#endif
#if defined(SSE_BLT_392)
  Blit.Request=0;
#endif

  ioaccess=0;
  Blit.Busy=true;
  dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Blitter_Start_Now changing GPIP bit from "+
          bool(mfp_reg[MFPR_GPIP] & MFP_GPIP_BLITTER_BIT)+" to 1");
  mfp_gpip_set_bit(MFP_GPIP_BLITTER_BIT,true);
  Blit.YCounter=Blit.YCount;
  /*Only want to start the line if not in the middle of one.*/
  if (WORD(Blit.XCounter-Blit.XCount)==0) Blitter_Start_Line();

#if defined(SSE_BOILER_BLIT_IN_HISTORY) && !defined(SSE_BLT_MAIN_LOOP)
#if defined(SSE_BOILER_BLIT_IN_HISTORY2)
#if defined(SSE_BOILER_HISTORY_TIMING)
  pc_history_y[pc_history_idx]=scan_y;
  pc_history_c[pc_history_idx]=LINECYCLES;
#endif
#endif
  DEBUG_ONLY( pc_history[pc_history_idx++]=0x98764321; )
  DEBUG_ONLY( if (pc_history_idx>=HISTORY_SIZE) pc_history_idx=0; )
#endif

  Blitter_Draw();
#if !defined(SSE_BLT_MAIN_LOOP)
  // DON'T check interrupts, we're in the middle of an instruction!
  check_for_interrupts_pending();
#endif
}

//---------------------------------------------------------------------------

#if defined(SSE_BLT_COPY_LOOP)
/*  BLITT03I.TOS: In blit mode, the blit can apparently be interrupted before
    the write.
    Data is kept in internal registers, as well as the current phase.
    This function will leave after 'read source', if there are still cycles, 
    the main loop will call it again at once.
    'Read destination' and 'write destination' are still done in one go.
*/

void Blitter_Blit_Word()
{
  ASSERT(Blit.Busy && Blit.HasBus);//there's no stupid assert

  switch(Blit.BlittingPhase)
  {
  case TBlitter::PRIME: // = prefetch = FXSR
    ASSERT( Blit.FXSR && Blit.XCount==Blit.XCounter
      &&((Blit.Op % 5)!=0 &&(Blit.Hop>1 || (Blit.Hop==1 && Blit.Smudge))) );
    abus=Blit.SrcAdr;
    BLT_ABUS_ACCESS_READ;
    Blitter_ReadSource(abus);
    Blit.SrcAdr+=Blit.SrcXInc;     
    Blit.BlittingPhase++;
    break;

  case TBlitter::READ_SOURCE:
    if (Blit.XCounter==1) // last word
    {  
      Blit.Last=true;
      if (Blit.XCount>1) 
        Blit.Mask=Blit.EndMask[2]; 
    }
    if ((Blit.Op % 5)!=0 &&(Blit.Hop>1 || (Blit.Hop==1 && Blit.Smudge))){
      if (Blit.NFSR && Blit.Last){
        if (Blit.SrcXInc>=0){
          Blit.SrcBuffer<<=16;
        }else{
          Blit.SrcBuffer>>=16;
        }
      }else{
        abus=Blit.SrcAdr;
        BLT_ABUS_ACCESS_READ;
        Blitter_ReadSource(Blit.SrcAdr);
      }
      if (Blit.Last){ 
        Blit.SrcAdr+=Blit.SrcYInc;
      }else{ 
        if ((Blit.NFSR && Blit.XCounter==2)==0){
          Blit.SrcAdr+=Blit.SrcXInc;
        }
      }
    }

    switch (Blit.Hop){
      case 0:
        Blit.SrcDat=WORD(0xffff); //SS fill
        break;
      case 1: 
        if (Blit.Smudge){  //SS strange but as documented
          Blit.SrcDat=Blit.HalfToneRAM[WORD(Blit.SrcBuffer >> Blit.Skew) & (BIT_0 | BIT_1 | BIT_2 | BIT_3)];
        }else{
          Blit.SrcDat=Blit.HalfToneRAM[int(Blit.LineNumber)];
        }
        break;
      default:
        ASSERT(Blit.Skew<16);
        Blit.SrcDat=WORD( Blit.SrcBuffer >> Blit.Skew);
        if (Blit.Hop==3){
          if (Blit.Smudge==0){
            Blit.SrcDat&=Blit.HalfToneRAM[int(Blit.LineNumber)];
          }else{
            Blit.SrcDat&=Blit.HalfToneRAM[Blit.SrcDat & (BIT_0 | BIT_1 | BIT_2 | BIT_3)];
          }
        }
    }//sw
    Blit.BlittingPhase++;
    break;
  
  case TBlitter::READ_DEST:
    Blit.DestDat=0;
    if (Blit.NeedDestRead || Blit.Mask!=0xffff){
      abus=Blit.DestAdr;
      BLT_ABUS_ACCESS_READ;
      Blit.DestDat=Blitter_DPeek(Blit.DestAdr);
      Blit.NewDat=Blit.DestDat & WORD(~(Blit.Mask));
    }else{
      Blit.NewDat=0;
    }
    switch (Blit.Op){   // 3 shows the shit in white
      case 0: // 0 0 0 0    - Target will be zeroed out (blind copy)
        Blit.NewDat|=WORD(0) & Blit.Mask; break;
      case 1: // 0 0 0 1    - Source AND Target         (inverse copy)
        Blit.NewDat|=WORD(Blit.SrcDat & Blit.DestDat) & Blit.Mask; break;
      case 2: // 0 0 1 0    - Source AND NOT Target     (mask copy)
        Blit.NewDat|=WORD(Blit.SrcDat & ~Blit.DestDat) & Blit.Mask; break;
      case 3: // 0 0 1 1    - Source only               (replace copy)
        Blit.NewDat|=Blit.SrcDat & Blit.Mask; break;
      case 4: // 0 1 0 0    - NOT Source AND Target     (mask copy)
        Blit.NewDat|=WORD(~Blit.SrcDat & Blit.DestDat) & Blit.Mask; break;
      case 5: // 0 1 0 1    - Target unchanged          (null copy)
        Blit.NewDat|=Blit.DestDat & Blit.Mask; break;
      case 6: // 0 1 1 0    - Source XOR Target         (xor copy)
        Blit.NewDat|=WORD(Blit.SrcDat ^ Blit.DestDat) & Blit.Mask; break;
      case 7: // 0 1 1 1    - Source OR Target          (combine copy)
        Blit.NewDat|=WORD(Blit.SrcDat | Blit.DestDat) & Blit.Mask; break;
      case 8: // 1 0 0 0    - NOT Source AND NOT Target (complex mask copy)
        Blit.NewDat|=WORD(~Blit.SrcDat & ~Blit.DestDat) & Blit.Mask; break;
      case 9: // 1 0 0 1    - NOT Source XOR Target     (complex combine copy)
        Blit.NewDat|=WORD(~Blit.SrcDat ^ Blit.DestDat) & Blit.Mask; break;
      case 10: // 1 0 1 0    - NOT Target                (reverse, no copy)
        Blit.NewDat=Blit.DestDat^Blit.Mask; break;  // ~DestAdr & Blit.Mask
      case 11: // 1 0 1 1    - Source OR NOT Target      (mask copy)
        Blit.NewDat|=WORD(Blit.SrcDat | ~Blit.DestDat) & Blit.Mask; break;
      case 12: // 1 1 0 0    - NOT Source                (reverse direct copy)
        Blit.NewDat|=WORD(~Blit.SrcDat) & Blit.Mask; break;
      case 13: // 1 1 0 1    - NOT Source OR Target      (reverse combine)
        Blit.NewDat|=WORD(~Blit.SrcDat | Blit.DestDat) & Blit.Mask; break;
      case 14: // 1 1 1 0    - NOT Source OR NOT Target  (complex reverse copy)
        Blit.NewDat|=WORD(~Blit.SrcDat | ~Blit.DestDat) & Blit.Mask; break;
      case 15: // 1 1 1 1    - Target is set to "1"      (blind copy)
        Blit.NewDat|=WORD(0xffff) & Blit.Mask; break;
    }//sw
    Blit.BlittingPhase++;
    // no break (eg Lethal Xcess)

  case TBlitter::WRITE_DEST:
    abus=Blit.DestAdr;
    BLT_ABUS_ACCESS_WRITE; 
    Blitter_DPoke(Blit.DestAdr,Blit.NewDat);

    if (Blit.Last){
      Blit.DestAdr+=Blit.DestYInc;
    }else{
      Blit.DestAdr+=Blit.DestXInc;
    }
    Blit.Mask=Blit.EndMask[1];

    if((--Blit.XCounter) <= 0){
      Blit.LineNumber+=char((Blit.DestYInc>=0) ? 1:-1);
      Blit.LineNumber&=15;
      Blit.YCounter--;
      Blit.YCount=(WORD)Blit.YCounter;
      Blit.XCounter=int(Blit.XCount ? Blit.XCount:65536);  //init blitter for line
      Blitter_Start_Line();
    }
    if(Blit.BlittingPhase!=TBlitter::PRIME)
      Blit.BlittingPhase=TBlitter::READ_SOURCE;
    break;

#if defined(SSE_BUGFIX_392)
  default: // recover from bug (old snapshot...)
    Blit.BlittingPhase=Blit.Busy=0;
#endif
  }//sw
}


#else // Steem 3.2


void Blitter_Blit_Word()
{
  if (Blit.Busy==0) return;

  WORD SrcDat,DestDat=0,NewDat;  //internal data registers
  // The modes 0,3,12,15 are source only
  if (Blit.XCounter==1){
    Blit.Last=true;
    if (Blit.XCount>1) Blit.Mask=Blit.EndMask[2];
  }
  //won't read source for 0,5,10,15 or Hop=0,1 (unless 1 and smudge on)
  if ((Blit.Op % 5)!=0 && (Blit.Hop>1 || (Blit.Hop==1 && Blit.Smudge))){
    if (Blit.NFSR && Blit.Last){
      if (Blit.SrcXInc>=0){
        Blit.SrcBuffer<<=16;
      }else{
        Blit.SrcBuffer>>=16;
      }
    }else{
      Blitter_ReadSource(Blit.SrcAdr);
      INSTRUCTION_TIME_ROUND(4);
    }
    if (Blit.Last){
      Blit.SrcAdr+=Blit.SrcYInc;
    }else{
      if ((Blit.NFSR && Blit.XCounter==2)==0){
        Blit.SrcAdr+=Blit.SrcXInc;
      }
    }
  }
  switch (Blit.Hop){
    case 0:
      SrcDat=WORD(0xffff);
      break;
    case 1:
      if (Blit.Smudge){
        SrcDat=Blit.HalfToneRAM[WORD(Blit.SrcBuffer >> Blit.Skew) & (BIT_0 | BIT_1 | BIT_2 | BIT_3)];
      }else{
        SrcDat=Blit.HalfToneRAM[int(Blit.LineNumber)];
      }
      break;
    default:
      SrcDat=WORD(Blit.SrcBuffer >> Blit.Skew);
      if (Blit.Hop==3){
        if (Blit.Smudge==0){
          SrcDat&=Blit.HalfToneRAM[int(Blit.LineNumber)];
        }else{
          SrcDat&=Blit.HalfToneRAM[SrcDat & (BIT_0 | BIT_1 | BIT_2 | BIT_3)];
        }
      }
  }
  if (Blit.NeedDestRead || Blit.Mask!=0xffff){
    DestDat=Blitter_DPeek(Blit.DestAdr);
    NewDat=DestDat & WORD(~(Blit.Mask));
    INSTRUCTION_TIME_ROUND(4);
  }else{
    NewDat=0; //Blit.Mask is FFFF and we're in a source-only mode
  }

  switch (Blit.Op){
    case 0:
      NewDat|=WORD(0) & Blit.Mask; break;
    case 1:
      NewDat|=WORD(SrcDat & DestDat) & Blit.Mask; break;
    case 2:
      NewDat|=WORD(SrcDat & ~DestDat) & Blit.Mask; break;
    case 3:
      NewDat|=SrcDat & Blit.Mask; break;
    case 4:
      NewDat|=WORD(~SrcDat & DestDat) & Blit.Mask; break;
    case 5:
      NewDat|=DestDat & Blit.Mask; break;
    case 6:
      NewDat|=WORD(SrcDat ^ DestDat) & Blit.Mask; break;
    case 7:
      NewDat|=WORD(SrcDat | DestDat) & Blit.Mask; break;
    case 8:
      NewDat|=WORD(~SrcDat & ~DestDat) & Blit.Mask; break;
    case 9:
      NewDat|=WORD(~SrcDat ^ DestDat) & Blit.Mask; break;
    case 10:
      NewDat=DestDat^Blit.Mask; break;  // ~DestAdr & Blit.Mask
    case 11:
      NewDat|=WORD(SrcDat | ~DestDat) & Blit.Mask; break;
    case 12:
      NewDat|=WORD(~SrcDat) & Blit.Mask; break;
    case 13:
      NewDat|=WORD(~SrcDat | DestDat) & Blit.Mask; break;
    case 14:
      NewDat|=WORD(~SrcDat | ~DestDat) & Blit.Mask; break;
    case 15:
      NewDat|=WORD(0xffff) & Blit.Mask; break;
  }
  Blitter_DPoke(Blit.DestAdr,NewDat);
  INSTRUCTION_TIME_ROUND(4);
  if (Blit.Last){
    Blit.DestAdr+=Blit.DestYInc;
  }else{
    Blit.DestAdr+=Blit.DestXInc;
  }
  Blit.Mask=Blit.EndMask[1];

  if((--Blit.XCounter) <= 0){
    Blit.LineNumber+=char((Blit.DestYInc>=0) ? 1:-1);
    Blit.LineNumber&=15;
    Blit.YCounter--;
    Blit.YCount=(WORD)Blit.YCounter;
    Blit.XCounter=int(Blit.XCount ? Blit.XCount:65536);  //init blitter for line
    Blitter_Start_Line();
  }
}


#endif


#if defined(SSE_BLT_MAIN_LOOP)
/*  Blitter_Draw() took care of the blit mode by running its own process
    loop for the CPU cycles.
    In this version, after the blit cycles we leave, and it must be checked
    in run's process loop if it's time to restart the blitter.
*/


void Blitter_Draw() {
  ioaccess&=~IOACCESS_FLAG_DO_BLIT;
#if defined(SSE_BLT_392)
  Blit.Request=0;
  MEM_ADDRESS abus_b4_blit=abus; // temp trick, normally it's in the CPU but we 
                                 // haven't this yet
#endif

  if (Blit.YCount==0){ // NO BLIT/BLIT FINISHED
    Blit.Busy=Blit.Hog=false; 
    dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Blitter_Draw YCount==0 changing GPIP bit from "+
         bool(mfp_reg[MFPR_GPIP] & MFP_GPIP_BLITTER_BIT)+" to 0");
    mfp_gpip_set_bit(MFP_GPIP_BLITTER_BIT,0);
    return;
  }

#if defined(SSE_BLT_392) // TODO
  int blit_bus_cycles=(Blit.Restarted
#if defined(SSE_CPU_392C)
    ||M68000.ThinkingCycles>=4
#endif
    )?64:63;
#endif

#if defined(SSE_BLT_390B) 
  Blit.TimeAtBlit=ACT; // to record #blit cycles
#endif

  INSTRUCTION_TIME(BLITTER_START_WAIT);

  Blit.YCounter=Blit.YCount;
#ifdef DEBUG_BUILD
  dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" ------------- BLITTING NOW --------------");
  dbg_log(EasyStr("SrcAdr=$")+HEXSl(Blit.SrcAdr,6)+", SrcXInc="+Blit.SrcXInc+", SrcYInc="+Blit.SrcYInc);
  dbg_log(EasyStr("DestAdr=$")+HEXSl(Blit.DestAdr,6)+", DestXInc="+Blit.DestXInc+", DestYInc="+Blit.DestYInc);
  dbg_log(EasyStr("XCount=")+int(Blit.XCount ? Blit.XCount:65536)+", YCount="+Blit.YCount);
  dbg_log(EasyStr("Skew=")+Blit.Skew+", NFSR="+(int)Blit.NFSR+", FXSR="+(int)Blit.FXSR);
  dbg_log(EasyStr("Hog=")+Blit.Hog+", Op="+Blit.Op+", Hop="+Blit.Hop);
#endif
  Blit.HasBus=true;

#if !defined(SSE_BLT_392)
  Blit.TimeToSwapBus=ABSOLUTE_CPU_TIME+252;
#if defined(SSE_BLT_RESTART)
  if(Blit.Restarted)
    Blit.TimeToSwapBus+=4; 
#endif
#endif

#if defined(SSE_BOILER_BLIT_IN_HISTORY3) 
  // write the BLiT in history
  pc_history_y[pc_history_idx]=scan_y;
  pc_history_c[pc_history_idx]=LINECYCLES;
  pc_history[pc_history_idx++]=0x98764321; 
  if (pc_history_idx>=HISTORY_SIZE) 
    pc_history_idx=0;
#endif

#if defined(SSE_BLT_392)
  Blit.BusAccessCounter=0;
#endif

#if defined(SSE_BOILER)
  while(Blit.HasBus){

    while(cpu_cycles>0 && Blit.HasBus){
#else
  while(runstate==RUNSTATE_RUNNING && Blit.HasBus){

    while(cpu_cycles>0 && runstate==RUNSTATE_RUNNING && Blit.HasBus){
#endif
      Blitter_Blit_Word();
      if(Blit.Busy)
      {
        // time to stop?
#if defined(SSE_BLT_392)
/*  from Cyprian:
    In Blit mode the BLiTTER counts every memory access used by the CPU.
    And after 64th it takes the control over the bus if I remember correctly
    for 65 bus cycles (1 cycle for bus mastering, 63 for data operations and
    1 for bus mastering), later it releases the bus and again counts every
    memory access used by the CPU, after 64th it takes the control... and so on
    http://www.atari-forum.com/viewtopic.php?f=94&t=30908&p=313416#p313292
    
    => The blitter doesn't count clock cycles, but bus accesses, maybe because
    it was cheaper.
    It explains why we had so much trouble determining CPU/blit cycles.
*/
        if(!Blit.Hog && Blit.BusAccessCounter>=blit_bus_cycles)
#else
        if(!Blit.Hog && ((ABSOLUTE_CPU_TIME-Blit.TimeToSwapBus)>=0))
#endif
        {
          INSTRUCTION_TIME(BLITTER_END_WAIT); //arbitration
          ASSERT(Blit.HasBus);
          Blit.HasBus=false;
#if defined(SSE_BLT_392)
          Blit.Request=1; // blit not finished
#ifdef SSE_BLT_390B
          Blit.BlitCycles=ACT-Blit.TimeAtBlit; 
#endif
          Blit.BusAccessCounter=0; // now we'll count CPU accesses
#else
          Blit.TimeToSwapBus=ACT+258;
#endif
        }
      }
      else // finished
      {
        Blit.HasBus=false;  
        break;
      }
    }

    while (cpu_cycles<=0){
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK2&TRACE_CONTROL_EVENT)
        TRACE_EVENT(screen_event_vector);        
#endif
      // run the events but...
      screen_event_vector();
      prepare_next_event();
      // DON'T check interrupts, we're in the middle of an instruction!
    }
  }
#if defined(SSE_BLT_392)
  abus=abus_b4_blit; // anarchic restart would cause crashes in TOS (internal bug)
#endif

}


#else // Steem 3.2


void Blitter_Draw()
{
//  MEM_ADDRESS SrcAdr=Blit.SrcAdr,DestAdr=Blit.DestAdr;

//  Blit.YCounter=int(Blit.YCount ? Blit.YCount:65536);
  INSTRUCTION_TIME_ROUND(BLITTER_START_WAIT);

  if (Blit.YCount==0){  //see note in Blitter.txt - trying to restart with a ycount of zero results in no restart
/*
     * If the BUSY flag is
     * reset when the Y_Count is zero, the flag will remain clear
     * indicating BLiTTER completion and the BLiTTER won't be restarted.
*/
    Blit.Busy=false;
    log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Blitter_Draw YCount==0 changing GPIP bit from "+
         bool(mfp_reg[MFPR_GPIP] & MFP_GPIP_BLITTER_BIT)+" to 0");
    mfp_gpip_set_bit(MFP_GPIP_BLITTER_BIT,0);
    return;
  }else{
    Blit.YCounter=Blit.YCount;
  }

//  WORD SrcDat,DestDat,Mask,NewDat;
//  bool Last;
//  long Cycles=0;

  log(Str("BLITTER: ")+HEXSl(old_pc,6)+" ------------- BLITTING NOW --------------");
  log(EasyStr("SrcAdr=$")+HEXSl(Blit.SrcAdr,6)+", SrcXInc="+Blit.SrcXInc+", SrcYInc="+Blit.SrcYInc);
  log(EasyStr("DestAdr=$")+HEXSl(Blit.DestAdr,6)+", DestXInc="+Blit.DestXInc+", DestYInc="+Blit.DestYInc);
  log(EasyStr("XCount=")+int(Blit.XCount ? Blit.XCount:65536)+", YCount="+Blit.YCount);
  log(EasyStr("Skew=")+Blit.Skew+", NFSR="+(int)Blit.NFSR+", FXSR="+(int)Blit.FXSR);
  log(EasyStr("Hog=")+Blit.Hog+", Op="+Blit.Op+", Hop="+Blit.Hop);

  // Turn off the "assigned a value that is never used" warning
//#ifdef WIN32
//  #pragma option -w-aus-
//#endif
  Blit.HasBus=true;
  Blit.TimeToSwapBus=ABSOLUTE_CPU_TIME+64;

//  while(Blit.Busy){
//    Blitter_Blit_Word();
//  }

  while (runstate==RUNSTATE_RUNNING){
    while (cpu_cycles>0 && runstate==RUNSTATE_RUNNING){
      if (Blit.HasBus){
        Blitter_Blit_Word();
      }else{
        DEBUG_ONLY( pc_history[pc_history_idx++]=pc; )
        DEBUG_ONLY( if (pc_history_idx>=HISTORY_SIZE) pc_history_idx=0; )

#undef LOGSECTION
#define LOGSECTION LOGSECTION_CPU
        m68k_PROCESS
#undef LOGSECTION
#define LOGSECTION LOGSECTION_BLITTER

        CHECK_BREAKPOINT
      }
      if (Blit.Busy){
        if (Blit.Hog==0){ //not in hog mode, keep switching bus
          if (((ABSOLUTE_CPU_TIME-Blit.TimeToSwapBus)>=0)){
            Blit.HasBus=!(Blit.HasBus);
            if (Blit.HasBus){
              log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Swapping bus to blitter at "+ABSOLUTE_CPU_TIME);
            }else{
              log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Swapping bus to CPU at "+ABSOLUTE_CPU_TIME);
            }
            Blit.TimeToSwapBus+=64;
          }
        }
      }else{
        Blit.HasBus=false;
        break;
      }
    }

    if (cpu_cycles>0) break;
    if (Blit.Busy==0) break; //enough!

    DEBUG_ONLY( if (runstate!=RUNSTATE_RUNNING) break; )
    DEBUG_ONLY( mode=STEM_MODE_INSPECT; )

    while (cpu_cycles<=0){
      screen_event_vector();
      prepare_next_event();
      if (cpu_cycles>0) check_for_interrupts_pending();
    }
    CHECK_BREAKPOINT

    DEBUG_ONLY( mode=STEM_MODE_CPU; )
//---------------------------------------------------------------------------
  } //more CPU!



//#ifdef WIN32
//  #pragma option -w-aus
//#endif

//  if ((Blit.Op % 5)!=0 && Blit.Hop>1) Blit.SrcAdr=SrcAdr;
//  Blit.DestAdr=DestAdr;
//  Blit.YCount=0;
//  if (Blit.Hog) INSTRUCTION_TIME_ROUND(Cycles);
}


#endif //main loop?

//---------------------------------------------------------------------------
BYTE Blitter_IO_ReadB(MEM_ADDRESS Adr)
{
#ifdef DISABLE_BLITTER
  exception(BOMBS_BUS_ERROR,EA_READ,Adr);
  return 0;
#else
	
#if defined(SSE_STF_BLITTER)
/*  When TOS>1.00 resets, it tests for Blitter.
    There was a blitter on (most!) Mega ST, in this emulation you're entitled
    to a blitter without having to place an order with Atari.
    It's the exact same blitter for STF & STE.
*/
 if(ST_TYPE!=STE && ST_TYPE!=MEGASTF)
  {
    exception(BOMBS_BUS_ERROR,EA_READ,Adr);
    return 0;
  }
#endif
	
  MEM_ADDRESS Offset=Adr-0xFF8A00;

  if (Offset<0x20){
    int nWord=(Offset/2);
    if (Offset & 1){  // Low byte
      return LOBYTE(Blit.HalfToneRAM[nWord]);
    }else{
      return HIBYTE(Blit.HalfToneRAM[nWord]);
    }
  }

  switch (Offset){
    case 0x20: return HIBYTE(Blit.SrcXInc);
    case 0x21: return LOBYTE(Blit.SrcXInc);
    case 0x22: return HIBYTE(Blit.SrcYInc);
    case 0x23: return LOBYTE(Blit.SrcYInc);
    case 0x24: return 0;
    case 0x25: return LOBYTE(HIWORD(Blit.SrcAdr));
    case 0x26: return HIBYTE(LOWORD(Blit.SrcAdr));
    case 0x27: return LOBYTE(Blit.SrcAdr);

    case 0x28: return HIBYTE(Blit.EndMask[0]);
    case 0x29: return LOBYTE(Blit.EndMask[0]);
    case 0x2A: return HIBYTE(Blit.EndMask[1]);
    case 0x2B: return LOBYTE(Blit.EndMask[1]);
    case 0x2C: return HIBYTE(Blit.EndMask[2]);
    case 0x2D: return LOBYTE(Blit.EndMask[2]);

    case 0x2E: return HIBYTE(Blit.DestXInc);
    case 0x2F: return LOBYTE(Blit.DestXInc);
    case 0x30: return HIBYTE(Blit.DestYInc);
    case 0x31: return LOBYTE(Blit.DestYInc);
    case 0x32: return 0;
    case 0x33: return LOBYTE(HIWORD(Blit.DestAdr));
    case 0x34: return HIBYTE(LOWORD(Blit.DestAdr));
    case 0x35: return LOBYTE(Blit.DestAdr);

    case 0x36: return HIBYTE(Blit.XCount);
    case 0x37: return LOBYTE(Blit.XCount);
    case 0x38: return HIBYTE(Blit.YCount);
    case 0x39: return LOBYTE(Blit.YCount);

    case 0x3A: return Blit.Hop;
    case 0x3B: return Blit.Op;
/*
          FF 8A3C   |ooo-oooo|
                    ||| |__|_____________ LINE NUMBER
                    |||__________________ SMUDGE (5)
                     ||__________________ HOG (6)
                     |___________________ BUSY (7)

*/
    case 0x3C: return BYTE(Blit.LineNumber | (Blit.Smudge << 5) | (Blit.Hog << 6) | (Blit.Busy<<7));
    case 0x3D: return BYTE(Blit.Skew | (Blit.NFSR << 6) | (Blit.FXSR << 7));
    case 0x3E:case 0x3F:return 0;
  }
  exception(BOMBS_BUS_ERROR,EA_READ,Adr);
  return 0;
#endif
}
//---------------------------------------------------------------------------
void Blitter_IO_WriteB(MEM_ADDRESS Adr,BYTE Val)
{

#ifdef DISABLE_BLITTER
  exception(BOMBS_BUS_ERROR,EA_WRITE,Adr);
  return;
#else

#if defined(SSE_STF_BLITTER)
  if(ST_TYPE!=STE && ST_TYPE!=MEGASTF)
  {
    exception(BOMBS_BUS_ERROR,EA_WRITE,Adr);
    return ;
  }
#endif

  MEM_ADDRESS Offset=Adr-0xFF8A00;
  if (Offset<0x3A && io_word_access==0){
    return;
  }
//  bool old_blit_primed=blit_primed;
//  blit_primed=true;

  if (Offset<0x20){
/*
Halftone RAM:                            

    $FFFF8A00  Halftone RAM, Word 0    (16 Words in total)
    ...
    $FFFF8A1E  Halftone RAM, Word 15

  Halftone RAM is a 32 Byte zero-waitstate Blitter-exclusive RAM that can be 
  used for lightning-quick manipulations of copied data. Its main purpose was 
  to combine monochrome picture data with (16 x 16 pixel) patterns, usually
  to make them a bit darker or brighter (halftone).

*/
    int nWord=(Offset/2);
    if (Offset & 1){  // Low byte
      Blit.HalfToneRAM[nWord]=MAKEWORD(Val,HIBYTE(Blit.HalfToneRAM[nWord]));
    }else{
      Blit.HalfToneRAM[nWord]=MAKEWORD(LOBYTE(Blit.HalfToneRAM[nWord]),Val);
    }
    return;
  }


  switch (Offset){
/*
Source X Increment Register:

    $FFFF8A20  X X X X X X X X X X X X X X X 0

Source Y Increment Register:

    $FFFF8A22  X X X X X X X X X X X X X X X X

 These registers encode how many bytes the Blitter increments the counter 
 after each copied word ($FFFF8A20) or after each line ($FFFF8A22). 
 Source Y Inc has to be even since the Blitter only  works on a Word-basis
 and can not access single Bytes.

*/
    case 0x20: WORD_B_1(&Blit.SrcXInc)=Val;return;
    case 0x21: WORD_B_0(&Blit.SrcXInc)=BYTE(Val & ~1);
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter SrcXInc to "+Blit.SrcXInc);
      return;
    case 0x22: WORD_B_1(&Blit.SrcYInc)=Val;return;
    case 0x23: WORD_B_0(&Blit.SrcYInc)=BYTE(Val & ~1);
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter SrcYInc to "+Blit.SrcYInc);
      return;
/*
Source Address Register:

    $FFFF8A24  XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXX0

  The 32-Bit address of the source, meaning the Blitter will start reading from
  this address. This address has to be even as the Blitter cannot access single
   Bytes. The Blitter actually accepts real 32-Bit addresses, but the MMU 
  filters the upper 10 bytes out.
*/
    case 0x24: return;
    case 0x25: DWORD_B_2(&Blit.SrcAdr)=Val;
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter SrcAdr to "+HEXSl(Blit.SrcAdr,6));
      return;
    case 0x26: DWORD_B_1(&Blit.SrcAdr)=Val;
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter SrcAdr to "+HEXSl(Blit.SrcAdr,6));
      return;
    case 0x27: DWORD_B_0(&Blit.SrcAdr)=BYTE(Val & ~1);
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - set blitter SrcAdr to "+HEXSl(Blit.SrcAdr,6));
      return;
/*
  There are three ENDMASK registers numbered 1 through 3.  ENDMASK 1 is used
  only for the first write of the line.  ENDMASK 3 is used only for the last
  write of the line.  ENDMASK 2 is used for all other writes.

  Endmask Registers

    $FFFF8A28  X X X X X X X X X X X X X X X X  Endmask 1

    $FFFF82AA  X X X X X X X X X X X X X X X X  Endmask 2

    $FFFF82AC  X X X X X X X X X X X X X X X X  Endmask 3

 The Endmask is a Bitmask that can be applied upon the copied data in a
 blockwise way. Endmask 1 is being applied on every first word copied in a row,
 Endmask 2 for all other words in this row except for the last one, which is
 combined with Endmask 3. Clever usage of these registers allow to start copies
 from basically every bit in memory.
*/
    case 0x28: WORD_B_1(Blit.EndMask)=Val;return;
    case 0x29: WORD_B_0(Blit.EndMask)=Val;return;
    case 0x2a: WORD_B_1(Blit.EndMask+1)=Val;return;
    case 0x2b: WORD_B_0(Blit.EndMask+1)=Val;return;
    case 0x2c: WORD_B_1(Blit.EndMask+2)=Val;return;
    case 0x2d: WORD_B_0(Blit.EndMask+2)=Val;return;
/*
Destination X Increment Register:

    $FFFF8A2E  X X X X X X X X X X X X X X X X

Destination Y Increment Register:

    $FFFF8A30  X X X X X X X X X X X X X X X X

Similar to the Source X/Y Increment Register. These two denote how many Bytes 
after each copied word/line the Blitter proceeds.
*/
/*
SOURCE X INCREMENT

This is  a signed  15-bit register,  the least significant bit is ignored,
specifying the offset in bytes to the address of the  next source  word in
the  current  line.    This  value  will be sign-extended and added to the
SOURCE ADDRESS register at the end of a source word fetch, whenever  the X
COUNT register  does not  contain a value of one.  If the X COUNT register
is  loaded  with  a  value  of  one  this  register  is  not  used.   Byte
instructions can not be used to read or write this register.
*/
    case 0x2E: WORD_B_1(&Blit.DestXInc)=Val;return;
    case 0x2F: WORD_B_0(&Blit.DestXInc)=BYTE(Val & ~1);return;
/*
SOURCE Y INCREMENT

This is  a signed  15-bit register,  the least significant bit is ignored,
specifying the offset in bytes to the address of the first source  word in
the next  line.   This value will be sign-extended and added to the SOURCE
ADDRESS register at the end of  the last  source word  fetch of  each line
(when the  X COUNT  register contains  a value  of one).   If  the X COUNT
register is loaded with a value of one this register  is used exclusively.
Byte instructions can not be used to read or write this register. 

*/
    case 0x30: WORD_B_1(&Blit.DestYInc)=Val;return;
    case 0x31: WORD_B_0(&Blit.DestYInc)=BYTE(Val & ~1);return;
/*
  DESTINATION ADDRESS

  This 23-bit register contains the current address of the destination field
  (only word addresses may be specified).  It  may be  accessed using either
  word or long-word instructions.  The value read back is always the address
  of the next word  to be  modified in  the destination  field.   It will be
  updated by  the amounts  specified in  the DESTINATION X INCREMENT and the
  DESTINATION Y INCREMENT registers as the transfer progresses.  

  Destination Address Register:

      $FFFF8A32  XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXX0

  This contains the address where the Blitter copies all the data to that it 
  computes. A real 32 Bit word that has to be even.
*/
    case 0x32: 
      return;
    case 0x33: DWORD_B_2(&Blit.DestAdr)=Val;
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter DestAdr to "+HEXSl(Blit.DestAdr,6));
      return;
    case 0x34: DWORD_B_1(&Blit.DestAdr)=Val;
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter DestAdr to "+HEXSl(Blit.DestAdr,6));
      return;
    case 0x35: DWORD_B_0(&Blit.DestAdr)=BYTE(Val & ~1);
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter DestAdr to "+HEXSl(Blit.DestAdr,6));
      return;
/*
  X Count Register:

      $FFFF8A36  X X X X X X X X X X X X X X X X

  Y Count Register:

      $FFFF8A38  X X X X X X X X X X X X X X X X

  These two registers contain the information about how the 2D bitblock the
  Blitter copies are shaped. The X Count Register contains how many words a 
  line of this rectangular block has, the Y-Count how many lines the bitblock
  has in total. This does not include the skipped words, only those the Blitter
  really copies (hence the name count).
*/
/*
  X COUNT  =  #words

  This 16-bit register  specifies  the  number  of  words  contained  in one
  destination line.   The  minimum number  is one  and the  maximum is 65536
  designated by zero.  Byte instructions can not  be used  to read  or write
  this register.   Reading  this register  returns the number of destination
  words yet to be  written in  the current  line, NOT  necessarily the value
  initially  written  to  the  register.    Each  time a destination word is
  written the value will be decremented until it reaches zero, at which time
  it will be returned to its initial value.
*/
    case 0x36:
      WORD_B_1(&Blit.XCount)=Val;
      Blit.XCounter=Blit.XCount;
      if (Blit.XCounter==0) Blit.XCounter=65536;
      return;
    case 0x37:
      WORD_B_0(&Blit.XCount)=Val;
      Blit.XCounter=Blit.XCount;
      if (Blit.XCounter==0) Blit.XCounter=65536;
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter XCount to "+Blit.XCount);
      return;
/*
  Y COUNT

  This  16-bit  register  specifies  the  number of lines in the destination
  field.  The minimum number is one and the maximum  is 65536  designated by
  zero.   Byte instructions  can not be used to read or write this register.
  Reading this register returns the number  of destination  lines yet  to be
  written,  NOT  necessarily  the  value  initially written to the register.
  Each time a destination line is  completed the  value will  be decremented
  until it reaches zero, at which time the tranfer is complete.
*/
    case 0x38:
      WORD_B_1(&Blit.YCount)=Val;
#if defined(SSE_BLT_YCOUNT)
      Blit.YCount&=0xFFFF; // hack, YCount is now 32bit //TODO, as for XCount?
#endif
      return;
    case 0x39:
      WORD_B_0(&Blit.YCount)=Val;
#if defined(SSE_BLT_YCOUNT)
      if (Blit.YCount==0) 
        Blit.YCount=65536; // hack... (TODO)
#endif
      
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter YCount to "+Blit.YCount);
      return;
/*
  Blit HOP (Halftone OPeration) Register:

      $FFFF8A3A  0 0 0 0 0 0 X X

  How to combine Halftone-Data and copied data is given here. A "00" means all
  copied bits will be set to "1" (blind copy), "01" means ONLY halftone content 
  will be copied, "10" implies that ONLY source content will be copied 
  (1:1 copy). "11" makes the halftone-pattern work as supposed and does a copy 
  "Halftone AND source".
*/
    case 0x3A: 
      Blit.Hop=BYTE(Val & (BIT_0 | BIT_1));
      dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter Hop to "+Blit.Hop);
      return;
/*
  Blit OP (logical OPeration) Register:

    $FFFF8A3B  0 0 0 0 X X X X

  The Blitter can carry out 0-cycles logical operations with source and target.
  The table of possible values follow:

    0 0 0 0    - Target will be zeroed out (blind copy)
    0 0 0 1    - Source AND Target         (inverse copy)
    0 0 1 0    - Source AND NOT Target     (mask copy)
    0 0 1 1    - Source only               (replace copy)
    0 1 0 0    - NOT Source AND Target     (mask copy)
    0 1 0 1    - Target unchanged          (null copy)
    0 1 1 0    - Source XOR Target         (xor copy)
    0 1 1 1    - Source OR Target          (combine copy)
    1 0 0 0    - NOT Source AND NOT Target (complex mask copy)
    1 0 0 1    - NOT Source XOR Target     (complex combine copy)
    1 0 1 0    - NOT Target                (reverse, no copy)
    1 0 1 1    - Source OR NOT Target      (mask copy)
    1 1 0 0    - NOT Source                (reverse direct copy)
    1 1 0 1    - NOT Source OR Target      (reverse combine)
    1 1 1 0    - NOT Source OR NOT Target  (complex reverse copy)
    1 1 1 1    - Target is set to "1"      (blind copy)
*/
    case 0x3B: Blit.Op=BYTE(Val & (BIT_0 | BIT_1 | BIT_2 | BIT_3));
               Blit.NeedDestRead=(Blit.Op && (Blit.Op!=3) && (Blit.Op!=12) && (Blit.Op!=15));
               dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter Op to "+Blit.Op);
               return;
/*
  Blitter Control Register:

    $FFFF8A3C  X X X 0 X X X X

  This register serves multiple purposes. The lowest 4 bit represents the
  number of the line in the Halftone pattern to start with.The upper 3 bits
  feature extended options of the Blitter.

    Bit 5  -  Smudge-mode

              Which line of the halftone pattern to start with is
              read from the first source word when the copy starts

    Bit 6  -  Blit-Mode Register

              Decides wether to copy in BLIT Mode (0) or in 
              HOG Mode (1). In Blit Mode (also known as cooperative),
              CPU and Blitter get 64 clockcycles in turns, in Hog 
              Mode, the Blitter reserves and hogs the bus for as long
              as the copy takes, CPU and DMA get no Bus access.
              [SS: see what a scam it is!]

    Bit 7  -  Busy Bit

              Turns on the Blitter activity and stays "1" until the copy is 
              finished
*/
    case 0x3C:
      Blit.LineNumber=BYTE(Val & (BIT_0 | BIT_1 | BIT_2 | BIT_3)); 
      Blit.Smudge=bool(Val & BIT_5); //SS persistent
      Blit.Hog=bool(Val & BIT_6); //SS volatile
#if defined(SSE_BLT_RESTART)
      Blit.Restarted=false;
#endif
      if (Blit.Busy==0){
        if (Val & BIT_7){ //start new
#if defined(SSE_DEBUG)
          if(!Blit.YCount)
          {
            //TRACE_LOG("No blit\n");
          }
          else
          {
TRACE_LOG("PC %X F%d y%d c%d Blt %X Hop%d Op%X %dx%d from %X (%d, %d) to %X (%d, %d) NF%d FX%d Sk%d Msk %X %X %X\n",
old_pc,TIMING_INFO,Val,Blit.Hop,Blit.Op,Blit.XCount,Blit.YCount,Blit.SrcAdr,Blit.SrcXInc,Blit.SrcYInc,Blit.DestAdr,Blit.DestXInc,Blit.DestYInc,Blit.NFSR,Blit.FXSR,Blit.Skew,Blit.EndMask[0],Blit.EndMask[1],Blit.EndMask[2]);
          }
#if defined(SSE_OSD_CONTROL)
          if(OSD_MASK3 & OSD_CONTROL_STEBLT) 
            TRACE_OSD("BLT %X %dx%d",Val,Blit.XCount,Blit.YCount);
#endif
#endif//dbg
          if (Blit.YCount)
          {
#if defined(SSE_BLT_392)
            Blit.Request=1;
            Blit.TimeToSwapBus=ACT+4; // latching delay?
#endif
            ioaccess|=IOACCESS_FLAG_DO_BLIT;
          }
        }
      }else{ //there's already a blit in progress
/*
     * The BLiTTER is usually operated with the HOG flag cleared.
     * In this mode the BLiTTER and the ST's cpu share the bus equally,
     * each taking 64 bus cycles while the other is halted.  This mode
     * allows interrupts to be fielded by the cpu while an extensive
     * BitBlt is being processed by the BLiTTER.  There is a drawback in
     * that BitBlts in this shared mode may take twice as long as BitBlts
     * executed in hog mode.  Ninety percent of hog mode performance is
     * achieved while retaining robust interrupt handling via a method
     * of prematurely restarting the BLiTTER.  When control is returned
     * to the cpu by the BLiTTER, the cpu immediately resets the BUSY
     * flag, restarting the BLiTTER after just 7 bus cycles rather than
     * after the usual 64 cycles.  Interrupts pending will be serviced
     * before the restart code regains control.  If the BUSY flag is
     * reset when the Y_Count is zero, the flag will remain clear
     * indicating BLiTTER completion and the BLiTTER won't be restarted.
     *
     * (Interrupt service routines may explicitly halt the BLiTTER
     * during execution time critical sections by clearing the BUSY flag.
     * The original BUSY flag state must be restored however, before
     * termination of the interrupt service routine.)

   SS: busy bit was already set, but by setting it again the blitter starts
   blitting at once. 
*/
        if (Val & BIT_7){ // Restart
#if defined(SSE_DEBUG)
TRACE_LOG("PC %X F%d y%d c%d ReBlt %X Hop%d Op%X %dx%d from %X (%d, %d) to %X (%d, %d) NF%d FX%d Sk%d Msk %X %X %X\n",
old_pc,TIMING_INFO,Val,Blit.Hop,Blit.Op,Blit.XCount,Blit.YCount,Blit.SrcAdr,Blit.SrcXInc,Blit.SrcYInc,Blit.DestAdr,Blit.DestXInc,Blit.DestYInc,Blit.NFSR,Blit.FXSR,Blit.Skew,Blit.EndMask[0],Blit.EndMask[1],Blit.EndMask[2]);
#endif
          dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Blitter restarted - swapping bus to Blitter at "+ABSOLUTE_CPU_TIME);

#if defined(SSE_BLT_MAIN_LOOP)
#if defined(SSE_BLT_RESTART)
          Blit.Restarted=true; // trick; changes #blit cycles and arbitration cycles
#endif
#if defined(SSE_BLT_RESTART2)//no...
          if (Blit.YCount)
            ioaccess|=IOACCESS_FLAG_DO_BLIT;
#else
#if defined(SSE_BLT_392) // restart timing
          INSTRUCTION_TIME(4); //BLIT03K TODO, it should be some latency
#endif
          Blitter_Draw();
#endif
#else//!main loop
          Blit.HasBus=true;
//          Blit.TimeToSwapBus=ABSOLUTE_CPU_TIME+64; //SS this was commented out
#if defined(SSE_BLT_BLIT_MODE_CYCLES)
          INSTRUCTION_TIME(BLITTER_START_WAIT);
          Blit.TimeToSwapBus=ABSOLUTE_CPU_TIME+BLITTER_BLIT_MODE_CYCLES;
#else
          Blit.TimeToSwapBus+=64;
#endif
#endif//? main loop
        }else{ // Stop
          Blit.Busy=false;
#if defined(SSE_BLT_392)
          Blit.Request=0;
#endif
          dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Blitter clear busy changing GPIP bit from "+
                  bool(mfp_reg[MFPR_GPIP] & MFP_GPIP_BLITTER_BIT)+" to 0");
          mfp_gpip_set_bit(MFP_GPIP_BLITTER_BIT,0);
        }
      }
      return;
/*
  SKEW

  The least significant four bits  of  the  byte-wide  register  at  FF 8A3D
  specify the  source skew.   This is the amount the data in the source data
  latch is shifted right before being  combined with  the halftone  mask and
  destination data.

      Blitter Skew Register:

      $FFFF8A3D  X X 0 0 X X X X

  The lowest 4 bit of this register allow to shift the data while copying by up
  to 15 bits to the right. 
  The upper 2 bits are
      Bit 6  -  NFSR (No final source read)
      Bit 7  -  FXSR (Force extra Source Read).
 
  NFSR means the last word of source is not being read anymore. This is only 
  sensible with certain Endmask and skew values. 

  FXSR is the opposite and forces the Blitter to read one more word. Also only
  sensible with certain Endmask/Skew combinations.

*/
    case 0x3D: Blit.Skew=BYTE(Val & (BIT_0 | BIT_1 | BIT_2 | BIT_3)); //SS persistent
               Blit.NFSR=bool(Val & BIT_6); //SS persistent
               Blit.FXSR=bool(Val & BIT_7); //SS persistent
               dbg_log(Str("BLITTER: ")+HEXSl(old_pc,6)+" - Set blitter Skew to "+Blit.Skew+", NFSR to "+(int)Blit.NFSR+", FXSR to "+(int)Blit.FXSR);
    case 0x3E:case 0x3F:
      return;
  }
  exception(BOMBS_BUS_ERROR,EA_WRITE,Adr);
#endif
}

#if defined(SSE_BLT_392)

void Blitter_CheckRequest() {
  ASSERT(Blit.Request);
#if defined(SSE_CPU_392B2) 
  if(M68000.ProcessingState==TM68000::EXCEPTION)
    return; //TODO, not sure about that
#endif

  // blit mode, autostart
  if(Blit.Busy) 
  {
    if(Blit.Request==1 && Blit.BusAccessCounter>=64)
    {
      Blit.Request++;
      Blit.TimeToSwapBus=ACT+4; // latency!
    }
    else if(Blit.Request==2 && (ACT-Blit.TimeToSwapBus>=0))
    {
      Blitter_Draw();
    }
  }
  // hog mode + blit mode, start
  else if((ABSOLUTE_CPU_TIME-Blit.TimeToSwapBus)>=0)
    Blitter_Start_Now(); 
}

#endif


//---------------------------------------------------------------------------
#undef LOGSECTION


