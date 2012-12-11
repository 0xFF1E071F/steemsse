#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"

#if defined(SS_DMA)

TDma Dma;

TDma::TDma() {
  Request=FALSE;
#if defined(SS_DMA_DOUBLE_FIFO)
  BufferInUse=0;
#endif
}

#if defined(SS_DMA_IO) // all fixes depend on it

#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC // not IO

/*  DMA/Disk IO table based on Atari doc

    ff 8600           |----------------|   Reserved 
    ff 8602           |----------------|   Reserved

    ff 8604   R/W     |--------xxxxxxxx|   Disk Controller
                                          & Sector Count (Word Access)

    ff 8606   R       |-------------xxx|   DMA Status (Word Access)
                                    |||
                                    || ----   _Error Status (1=OK)
                                    | -----   _Sector Count Zero Status
                                     ------   _Data Request Inactive Status

    ff 8606   W       |-------xxxxxxxx-|   DMA Mode Control (Word Access)
                              ||||||||     0  Reserved (0)
                              ||||||| -----1  A0 lines of FDC/HDC
                              |||||| ------2  A1 ................ 
                              ||||| -------3  HDC (1) / FDC (0) Register Select
                              |||| --------4  Sector Count Register Select
                              |||0         5  Reserved (0)
                              || ----------6  Disable (1) / Enable (0) DMA
                              | -----------7  FDC DRQ (1) / HDC DRQ (0) 
                               ------------8  Write (1) / Read (0)

    ff 8609   R/W             |xxxxxxxx|   DMA Base and Counter High
    ff 860b   R/W             |xxxxxxxx|   DMA Base and Counter Mid
    ff 860d   R/W             |xxxxxxxx|   DMA Base and Counter Low
    
    ff 8610 - ff 86ff                      Reserved
*/

BYTE TDma::IORead(MEM_ADDRESS addr) {

  // test for bus error
  if(addr>0xff860f || addr<0xff8604 || addr<0xff8608 && !io_word_access) 
    exception(BOMBS_BUS_ERROR,EA_READ,addr);

  // trace
#if defined(SS_DEBUG)
  if(TRACE_ENABLED)
    TraceIORead(addr);
#endif

  // pasti handling
#if USE_PASTI 
  if(hPasti && pasti_active)
  {
    if(addr<0xff8608) // word only
      if(addr & 1) return LOBYTE(pasti_store_byte_access);
    struct pastiIOINFO pioi;
    pioi.addr=addr;
    pioi.stPC=pc;
    pioi.cycles=ABSOLUTE_CPU_TIME;
//    log_to(LOGSECTION_PASTI,Str("PASTI: IO read addr=$")
//      +HEXSl(addr,6)+" pc=$"+HEXSl(pc,6)+" cycles="+pioi.cycles);
    pasti->Io(PASTI_IOREAD,&pioi);
    pasti_handle_return(&pioi);
    if(addr<0xff8608) // word only
    {
      pasti_store_byte_access=WORD(pioi.data);
      pioi.data=HIBYTE(pioi.data);
    }
//          log_to(LOGSECTION_PASTI,Str("PASTI: Read returning $")
//              +HEXSl(BYTE(pioi.data),2)+" ("+BYTE(pioi.data)+")");
    return BYTE(pioi.data);
  }
#endif

  int drive=floppy_current_drive();
  switch(addr)
  {

  case 0xff8604:
    ASSERT(!(dma_mode & BIT_4)); 
    // sector counter
    if(dma_mode & BIT_4) 
#if defined(SS_DMA_SECTOR_COUNT)
/*
"The sector count register is write only. Reading this register return
 unpredictable values."
*/
      return (rand()&0xFF);
#else
      return HIBYTE(dma_sector_count); 
#endif
    // HD access
    else if(dma_mode & BIT_3) 
    {
      LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
        " - Reading high byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )
      return 0xff;
    }
    // high byte of FDC
#if defined(SS_FDC_READ_HIGH_BYTE)
    return 0x00;// like in Pasti
#else
/*
"The FDC registers only uses 8 bits when writing and therefore the upper byte 
is ignored and when reading the 8 upper bits consistently reads 1."
*/
    return 0xff;// like in doc
#endif
    BRK(impossible)

  case 0xff8605: 
    // sector counter
    if(dma_mode & BIT_4) 
#if defined(SS_DMA_SECTOR_COUNT)
      return (rand()&0xFF);
#else
      return LOBYTE(dma_sector_count); 
#endif
    // HD access
    else if(dma_mode & BIT_3) 
    {
      LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                  " - Reading low byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )
      return 0xff;
    }
    // Read FDC register
#if defined(SS_DMA_FDC_ACCESS)
/*
Bit 7:FDC/HDC transfers acknowledge; when set the DRQ from the Floppy 
Disk Controller is acknowledged; otherwise, the DRQ from the Hard Disk 
interface is acknowledged. For some reason this bit must also be set 
to generate DMA bus cycle.
*/
    if(!(dma_mode&BIT_7)) // we can't
    {
      TRACE_LOG("Read FDC, DMA mode %x\n",dma_mode);
      return 0xFF;
    }
#endif
    // IPF handling
#if defined(SS_IPF)
    if(Caps.IsIpf(drive))
      return Caps.ReadWD1772();
#endif
    // Steem handling
    switch(dma_mode&(BIT_1+BIT_2)){
    case 0: // STR
      // Update some flags before returning it
      // IP
      if (floppy_track_index_pulse_active())
        fdc_str|=FDC_STR_T1_INDEX_PULSE;
      else
        // If not type 1 command we will get here, it is okay to clear
        // it as this bit is only for the DMA chip for type 2/3.
        fdc_str&=BYTE(~FDC_STR_T1_INDEX_PULSE);
      // WP, SU
      if(floppy_type1_command_active)
      {
        fdc_str&=(~FDC_STR_WRITE_PROTECT);
        if(floppy_mediach[drive])
        {
          if(floppy_mediach[drive]/10!=1) 
            fdc_str|=FDC_STR_WRITE_PROTECT;
          else if (FloppyDrive[drive].ReadOnly)
            fdc_str|=FDC_STR_WRITE_PROTECT;
        }
        if(fdc_spinning_up)
          fdc_str&=BYTE(~FDC_STR_T1_SPINUP_COMPLETE);
        else
          fdc_str|=FDC_STR_T1_SPINUP_COMPLETE;
      } // else it should be set in fdc_execute()
      if ((mfp_reg[MFPR_GPIP] & BIT_5)==0)
      {
        LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
          " - Reading status register as "+Str(itoa(fdc_str,d2_t_buf,2)).LPad(8,'0')+
          " ($"+HEXSl(fdc_str,2)+"), clearing IRQ"); )
        floppy_irq_flag=0;
        mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
      }
      return fdc_str; // STR
    case 2:
      return fdc_tr; // TR
    case 4:
      return fdc_sr; // SR
    case 6:
      return fdc_dr; // DR
    }//sw
    BRK(impossible)

  case 0xff8606:  //high byte of DMA status
    return 0x0; // only bits 0-2 of low byte are used

  case 0xff8607:  //low byte of DMA status
#if defined(SS_DMA_READ_STATUS)
/*
"If the DMA status word is polled during a DMA operation the transfer might
 be disrupted."
Not emulated
*/
    return BYTE(b00000111) & dma_status; // fixes Vindicators IPF
#else
    return BYTE(b11110000) | dma_status;
#endif

/*
"The DMA Address Counter register must be read in a High, Mid, Low order."
TODO?
*/
  case 0xff8609:  // DMA Base and Counter High
    return (BYTE)((dma_address&0xff0000)>>16);

  case 0xff860b:  // DMA Base and Counter Mid
    return (BYTE)((dma_address&0xff00)>>8);

  case 0xff860d:  // DMA Base and Counter Low
    return (BYTE)((dma_address&0xff));

  case 0xff860e: //frequency/density control
  {
    if(FloppyDrive[drive].STT_File) 
      return 0;

    TFloppyImage *floppy=&(FloppyDrive[drive]);
      return BYTE((floppy->BytesPerSector * floppy->SectorsPerTrack)>7000);
  }

  case 0xff860f: //high byte of frequency/density control?
    return 0;
  }//sw
  BRK(impossible)
}


void TDma::IOWrite(MEM_ADDRESS addr,BYTE io_src_b) {

  // test for bus error
  if(addr>0xff860f || addr<0xff8604 || addr<0xff8608 && !io_word_access) 
    exception(BOMBS_BUS_ERROR,EA_WRITE,addr);

  // trace
#if defined(SS_DEBUG)
  if(TRACE_ENABLED)
    TraceIOWrite(addr,io_src_b);
#endif

  // pasti handling
#if USE_PASTI 
  if(hPasti && pasti_active)
  {
    // pasti.dll takes WORDs, so we build it in 2 steps
    WORD data=io_src_b;
    if (addr<0xff8608)
    { // word only
      if(addr & 1)
      {
        data=MAKEWORD(io_src_b,pasti_store_byte_access);
        addr&=~1;
      }
      else
      {
        pasti_store_byte_access=io_src_b;
        return;
      }
    }
    struct pastiIOINFO pioi;
    pioi.addr=addr;
    pioi.data=data;
    pioi.stPC=pc; //SS debug only?
    pioi.cycles=ABSOLUTE_CPU_TIME;
    //          log_to(LOGSECTION_PASTI,Str("PASTI: IO write addr=$")+HEXSl(addr,6)+" data=$"+
    //                            HEXSl(io_src_b,2)+" ("+io_src_b+") pc=$"+HEXSl(pc,6)+" cycles="+pioi.cycles);
    pasti->Io(PASTI_IOWRITE,&pioi); //SS send to DLL
    pasti_handle_return(&pioi);
    return;
  }
#endif

  switch (addr)
  {

  case 0xff8604:
    //write DMA sector counter, 0x190
    if(dma_mode & BIT_4)
    { 
      ASSERT(!io_src_b);
      dma_sector_count&=0xff;
      dma_sector_count|=int(io_src_b) << 8;
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
      break;
    }
    // HD access
    if(dma_mode & BIT_3)
    { 
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Writing $"+HEXSl(io_src_b,2)+"xx to HDC register #"+((dma_mode & BIT_1) ? 1:0));
      break;
    }
/*
"The FDC registers only uses 8 bits when writing and therefore the upper byte 
is ignored and when reading the 8 upper bits consistently reads 1."
*/
    ASSERT(!io_src_b); // Blood Money IPF crashing, Archipelagos IPF
    break;
    
  case 0xff8605:  
    //write FDC sector counter, 0x190
    if (dma_mode & BIT_4)
    { 
      dma_sector_count&=0xff00;
      dma_sector_count|=io_src_b;
      // We need do that only once (word access):
      if (dma_sector_count)
        dma_status|=BIT_1;
      else
        dma_status&=BYTE(~BIT_1); //status register bit for 0 count
      dma_bytes_written_for_sector_count=0;
/*
"It is interesting to note that when the DMA is in write mode, the two internal
 FIFOS are filled immediately after the Count Register is written."
*/
      if(dma_mode&0x100) // RAM to disk
        RequestTransfer(); // we fill one
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
      break;
    }
    // HD access
    if (dma_mode & BIT_3){ 
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Writing $xx"+HEXSl(io_src_b,2)+" to HDC register #"+((dma_mode & BIT_1) ? 1:0));
      break;
    }
    // Write FDC register
#if defined(SS_DMA_FDC_ACCESS)
/*
"Bit 7:FDC/HDC transfers acknowledge; when set the DRQ from the Floppy 
Disk Controller is acknowledged; otherwise, the DRQ from the Hard Disk 
interface is acknowledged. For some reason this bit must also be set 
to generate DMA bus cycle."
*/
    ASSERT( dma_mode&BIT_7 );
    if(!(dma_mode&BIT_7))
      break;
#endif
    // IPF handling
#if defined(SS_IPF)
    if(Caps.IsIpf(floppy_current_drive())) 
    {
      Caps.WriteWD1772(io_src_b);
      break;
    }
#endif
    // Steem handling
    switch (dma_mode&(BIT_1+BIT_2))
    {
    case 0: // CR
      floppy_fdc_command(io_src_b); // in fdc.cpp
      break;
    case 2: // TR
#if defined(SS_FDC_CHANGE_TRACK_WHILE_BUSY)
      if(fdc_str & FDC_STR_BUSY)
        TRACE_LOG("TR change while busy %d -> %d\n",fdc_tr,io_src_b);
      fdc_tr=io_src_b;
#else
      if ((fdc_str & FDC_STR_BUSY)==0){
        log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC track register to "+io_src_b);
        fdc_tr=io_src_b;
      }else{
        log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Can't set FDC track register to "+io_src_b+", FDC is busy");
      }
#endif
      break;
    case 4: // SR
#if defined(SS_FDC_CHANGE_SECTOR_WHILE_BUSY)
      if(fdc_str & FDC_STR_BUSY)
        TRACE_LOG("SR change while busy %d -> %d\n",fdc_sr,io_src_b);
      fdc_sr=io_src_b; // fixes Delirious 4 loader without Pasti
#else
      if ((fdc_str & FDC_STR_BUSY)==0){
        log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC sector register to "+io_src_b);
        fdc_sr=io_src_b;
      }else{
        log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Can't set FDC sector register to "+io_src_b+", FDC is busy");
      }
#endif
      break;
    case 6: // DR
      log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC data register to "+io_src_b);
      fdc_dr=io_src_b;
      break;
    }
    break;

  case 0xff8606:  //high byte of DMA mode
#if defined(SS_DMA_WRITE_CONTROL)
/*
"Toggling the Read/Write transfer direction bit (bit 8) of the mode register
 clears the DMA controller status register, flushes the internal FIFOs, and 
clears the Sector Count Register."
And only those writes that cause a toggle will reset the DMA.
By "flushing the internal FIFOs", we mustn't understand "copy the remainder to
RAM". Note that "flushing to disk" would be impossible.
What was in the buffers will go nowhere, the internal counter is reset.
*/
    // detect toggling of bit 8 (boolean !x ^ !y = logical x ^^ y)
    if( !(dma_mode&0x100) ^ !(io_src_b) ) // fixes Archipelagos IPF
    {
      fdc_read_address_buffer_len=0;
      dma_sector_count=0;
      dma_bytes_written_for_sector_count=0;
      dma_status=1;
      Request=FALSE;
#if defined(SS_DMA_DOUBLE_FIFO) 
      BufferInUse=0;
#endif
    }
    dma_mode&=0x00ff;
    dma_mode|=WORD(WORD(io_src_b) << 8);
#else
    dma_mode&=0x00ff;
    dma_mode|=WORD(WORD(io_src_b) << 8);
    fdc_read_address_buffer_len=0;
    dma_bytes_written_for_sector_count=0;
#endif
    break;
    
  case 0xff8607:  //low byte of DMA mode
    ASSERT(!(io_src_b&1));
    ASSERT(!(io_src_b&BIT_5));
    dma_mode&=0xff00;
    dma_mode|=io_src_b;
#if !defined(SS_DMA_WRITE_CONTROL) // see above
    fdc_read_address_buffer_len=0;
    dma_bytes_written_for_sector_count=0;
#endif
    break;
    
  case 0xff8609:  // DMA Base and Counter High
    dma_address&=0x00ffff;
    dma_address|=((MEM_ADDRESS)io_src_b) << 16;
    log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Set DMA address to "+HEXSl(dma_address,6));
    break;
    
  case 0xff860b:  // DMA Base and Counter Mid
#if defined(SS_DMA_ADDRESS)
/* 
"The DMA Address Counter register must be loaded (written) in a Low, Mid, 
  High order."
*/
    dma_address&=0x0000ff;
    dma_address|=((MEM_ADDRESS)io_src_b) << 8;
    //dma_address|=0xff0000;
#else
    dma_address&=0xff00ff;
    dma_address|=((MEM_ADDRESS)io_src_b) << 8;
#endif
    break;
    
  case 0xff860d:  // DMA Base and Counter Low
#if defined(SS_DMA_ADDRESS)
    //dma_address=0xffff00,dma_address|=io_src_b;
    dma_address=io_src_b;
#else
    dma_address&=0xffff00;
    dma_address|=io_src_b;
#endif       
    break;
    
  case 0xff860e: //high byte of frequency/density control
    break; //ignore
    
  case 0xff860f: //low byte of frequency/density control
    break;
  }//sw
}

#endif//dmaio


#if defined(SS_DEBUG)

void TDma::TraceIORead(MEM_ADDRESS addr) {
  UpdateRegs();
  switch (addr){
  case 0xff8604:  //high byte of FDC access
    break;
  case 0xff8605:  //low byte of FDC access
    if (dma_mode & BIT_4){ //write FDC sector counter, 0x190
      TRACE("R DMA # %d\n",dma_sector_count);
      break;
    }
    if (dma_mode & BIT_3){ // HD access
      TRACE("R DMA HD\n");
      break;
    }
    switch(dma_mode & (BIT_1+BIT_2)){
    case 0:
      TRACE("R STR $%X\n",fdc_str);
      break;
    case 2:
      TRACE("R TR %d\n",fdc_tr);
      break;
    case 4:
      TRACE("R SR %d\n",fdc_sr);
      break;
    case 6:
      TRACE("R DR %d\n",fdc_dr);
      break;
    }
    break;
  case 0xff8606:  //high byte of DMA mode
    break;
  case 0xff8607:  //low byte of DMA mode
    TRACE("R DMA CR $%X\n",dma_mode);
    break;
  case 0xff8609:  //high byte of DMA pointer
    TRACE("R DMA bus %X\n",dma_address);
    break;
  case 0xff860b:  //mid byte of DMA pointer
    break;
  case 0xff860d:  //low byte of DMA pointer
    break;
  case 0xff860e: //high byte of frequency/density control 
    TRACE("R denisity HI\n");
    break;
  case 0xff860f: //low byte of frequency/density control
    TRACE("R denisity LO\n");
    break;
  }
}

void TDma::TraceIOWrite(MEM_ADDRESS addr,BYTE io_src_b) {
  UpdateRegs();
  switch (addr){
  case 0xff8604:  //high byte of FDC access
    break;
  case 0xff8605:  //low byte of FDC access
    if (dma_mode & BIT_4){ //write FDC sector counter, 0x190
      TRACE("W DMA # %d\n",(dma_sector_count&0xff00)|io_src_b);
      break;
    }
    if (dma_mode & BIT_3){ // HD access
      TRACE("DMA HD access %X\n",io_src_b);
      break;
    }
    switch(dma_mode & (BIT_1+BIT_2)){
    case 0:
      TRACE("W CR $%X\n",io_src_b);
      break;
    case 2:
      TRACE("W TR %d\n",io_src_b);
      break;
    case 4:
      TRACE("W SR %d\n",io_src_b);
      break;
    case 6:
      TRACE("W DR %d\n",io_src_b);
      break;
    }
    break;
  case 0xff8606:  //high byte of DMA mode
    break;
  case 0xff8607:  //low byte of DMA mode
#if USE_PASTI
    if(hPasti && pasti_active)
      dma_mode=(dma_mode&0x00FF)|(pasti_store_byte_access<<8);
#endif
    TRACE("W DMA CR $%X\n",(dma_mode&0xff00)|io_src_b);
    break;
  case 0xff8609:  //high byte of DMA pointer
    TRACE("W DMA bus %X\n",(dma_address&0x00ffff)| ((MEM_ADDRESS)io_src_b) << 16);
    break;
  case 0xff860b:  //mid byte of DMA pointer
    break;
  case 0xff860d:  //low byte of DMA pointer
    break;
  case 0xff860e: //high byte of frequency/density control 
    TRACE("W $%X on %X\n",io_src_b,addr); //unexpected
    break;
  case 0xff860f: //low byte of frequency/density control
    TRACE("W $%X on %X\n",io_src_b,addr);//unexpected
    break;
  }
}

#endif

#if defined(SS_DEBUG) || defined(SS_OSD_DRIVE_LED)

void TDma::UpdateRegs(bool trace_them) {
  // this is used for debugging and for pasti drive led
  BYTE psg=psg_reg[PSGR_PORT_A]&7; // 'drive select'
#if USE_PASTI
  if(hPasti && pasti_active)
  {
    pastiPEEKINFO ppi;
    pasti->Peek(&ppi);
    fdc_cr=ppi.commandReg;
    fdc_str=ppi.statusReg;
    fdc_tr=ppi.trackReg;
    fdc_sr=ppi.sectorReg;
    fdc_dr=ppi.dataReg;
    dma_mode=ppi.dmaControl;
    floppy_head_track[0]=ppi.drvaTrack;
    floppy_head_track[1]=ppi.drvbTrack;
    dma_address=ppi.dmaBase;
    dma_status=ppi.dmaStatus;
    dma_sector_count=ppi.dmaCount;
    psg=ppi.drvSelect;
  }else
#endif
#if defined(SS_IPF)
  if(Caps.IsIpf(floppy_current_drive()))
  {
    int ext=0; // TODO
    fdc_cr=CapsFdcGetInfo(cfdciR_Command, &Caps.WD1772,ext);
    fdc_str=CapsFdcGetInfo(cfdciR_ST, &Caps.WD1772,ext);
    fdc_tr=CapsFdcGetInfo(cfdciR_Track, &Caps.WD1772,ext);
    fdc_sr=CapsFdcGetInfo(cfdciR_Sector, &Caps.WD1772,ext);
    fdc_dr=CapsFdcGetInfo(cfdciR_Data, &Caps.WD1772,ext);
    floppy_head_track[0]=Caps.SF314[0].track;
    floppy_head_track[1]=Caps.SF314[1].track;
  } 
#endif
  if(trace_them)
  {
    TRACE("CR %X STR %X ",fdc_cr,fdc_str);
#if defined(SS_FDC_TRACE_STATUS)
    WD1772.TraceStatus();
#endif
    TRACE("TR %d SR %d DR %d ($%X) PSG %X T %d/%d DMA CR %X $%X SR %X #%d PC %X\n",
      fdc_tr,fdc_sr,fdc_dr,fdc_dr,psg,floppy_head_track[0],floppy_head_track[1],
      dma_mode,dma_address,dma_status,dma_sector_count,pc);
  }
}

#endif

void TDma::RequestTransfer() {
  // we make this function to avoid code duplication
  Request=TRUE; 
#if defined(SS_DMA_DOUBLE_FIFO)
  BufferInUse=!Dma.BufferInUse; // toggle 16byte buffer
#endif

#if defined(SS_DMA_DELAY) // set event
  dma_time=ABSOLUTE_CPU_TIME+SS_DMA_ACCESS_DELAY;
  PREPARE_EVENT_CHECK_FOR_DMA
#else
  TransferBytes(); // direct transfer
#endif
}

#if defined(SS_DMA_DELAY) // normally not
// this is a Steem 'event' (static member function)

void TDma::Event() {
#if defined(SS_MFP_RATIO)
  Dma.dma_time=ABSOLUTE_CPU_TIME+CpuNormalHz;
#else
  Dma.dma_time=ABSOLUTE_CPU_TIME+8000000;
#endif
//  if(!Caps.Active)
  //  return;
  if(Dma.Request)
    Dma.TransferBytes();
}
#endif


#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC_BYTES

void TDma::TransferBytes() {
  // execute the DMA transfer (assume floppy)
  ASSERT( dma_mode&0x80 ); // bit 7 or no transfer
  ASSERT(!(dma_mode&BIT_5));
  ASSERT( Request );
  //UpdateRegs();
  //    TRACE_LOG("%d ",ABSOLUTE_CPU_TIME);
  if(!(dma_mode&0x100)) // disk -> RAM
    TRACE_LOG("%d/%d to %X: ",fdc_tr,fdc_sr,dma_address);
  else  // RAM -> disk
    TRACE_LOG("%d/%d from %X: ",fdc_tr,fdc_sr,dma_address);
  
  for(int i=0;i<16;i++) // burst, 16byte packets
  {
    if(!(dma_mode&0x100)&& DMA_ADDRESS_IS_VALID_W) // disk -> RAM
      PEEK(dma_address)=
        fdc_read_address_buffer[
#if defined(SS_DMA_DOUBLE_FIFO)
        16*(!BufferInUse)+
#endif
        i];
    else if((dma_mode&0x100) && DMA_ADDRESS_IS_VALID_R) // RAM -> disk
      fdc_read_address_buffer[
#if defined(SS_DMA_DOUBLE_FIFO)
        16*(!BufferInUse)+
#endif
        i]=PEEK(dma_address);
    else TRACE_LOG("!!!");
    TRACE_LOG("%02X ",fdc_read_address_buffer[
#if defined(SS_DMA_DOUBLE_FIFO)
      16*(!BufferInUse)+
#endif
      i]);
    DMA_INC_ADDRESS; // use Steem's existing routine
  }
  INSTRUCTION_TIME(8); // it doesn't seem to change anything but why not?
  TRACE_LOG("\n");
  Request=FALSE;
}


#endif//dma


#if defined(SS_FDC)

/*
FDC Registers detail

Data Shift Register - This 8-bit register assembles serial data from the Read
 Data input (RD) during Read operations and transfers serial data to the Write
 Data output during Write operations.

Data Register - This 8-bit register is used as a holding register during Disk
 Read and Write operations. In disk Read operations, the assembled data byte 
is transferred in parallel to the Data Register from the Data Shift Register.
 In Disk Write operations, information is transferred in parallel from the 
Data Register to the Data Shift Register.
When executing the Seek Command, the Data Register holds the address of the
 desired Track position. This register is loaded from the Data bus and gated
 onto the Data bus under processor control.

Track Register - This 8-bit register holds the track number of the current
 Read/Write head position. It is incremented by one every time the head is
 stepped in and decremented by one when the head is stepped out (towards 
track 00). The content of the register is compared with the recorded track
 number in the ID field during disk Read, Write, and Verify operations. 
The Track Register can be loaded from or transferred to the Data bus. 
This Register is not loaded when the device is busy.

Sector Register (SR) - This 8-bit register holds the address of the desired
 sector position. The contents of the register are compared with the recorded
 sector number in the ID field during disk Read or Write operations. 
The Sector Register contents can be loaded from or transferred to the Data bus
. This register is not loaded when the device is busy.

Command Register (CR) - This 8-bit register holds the command presently
 being executed. This register is not loaded when the device is busy unless
 the new command is a force interrupt. The Command Register is loaded from 
the Data bus, but not read onto the Data bus.

Status Register (STR) - This 8-bit register holds device Status information.
 The meaning of the Status bits is a function of the type of command
 previously executed. This register is read onto the Data bus, but not loaded 
from the Data bus.
*/

TWD1772 WD1772;

int TWD1772::WritingToDisk() {
  return((fdc_cr&0xF0)==0xF0 || (fdc_cr&0xE0)==0xA0);
}

#if defined(SS_DEBUG)

int TWD1772::CommandType(int command) {
  // return type 1-4 of this FDC command
  int type;
  if(!(command&BIT_7))
    type=1;
  else if(!(command&BIT_6))
    type=2;
  else if((command&0xF0)==0xD) //1101
    type=4;
  else
    type=3;
  return type;
}


void TWD1772::TraceStatus() {
  // this embarrassing part proves my ignorance!
  int type=CommandType(fdc_cr);
  TRACE("( ");
  if(fdc_str&0x80)
    TRACE("MO ");//Motor On
  if(fdc_str&0x40)
    TRACE("WP ");// write protect
  if(fdc_str&0x20)
    if(type==1)
      TRACE("SU "); // Spin-up (meaning up to speed)
    else
      TRACE("RT "); //Record Type (1=deleted data)
  if(fdc_str&0x10)
    if(type==1)
      TRACE("SE ");//Seek Error
    else
      TRACE("RNF ");//Record Not Found
  if(fdc_str&0x08)
    TRACE("CRC "); //CRC Error
  if(fdc_str&0x04)
    if(type==1) 
      TRACE("T0 "); // track zero
    else
      TRACE("LD ");//Lost Data, normally impossible on ST
  if(fdc_str&0x02)
    if(type==1) 
      TRACE("IP "); // index
    else
      TRACE("DRQ "); // data request
  if(fdc_str&0x01)
    TRACE("BSY "); // busy
    TRACE(") "); 
}

#endif//debug


#endif


#if defined(SS_IPF)

#define CYCLES_PRE_IO 100 // those aren't
#define CYCLES_POST_IO 100 // used

TCaps Caps; // singleton

TCaps::TCaps() {
  VERIFY( Init()!=NULL );
}


TCaps::~TCaps() {
  CapsExit();
}


TCaps::Init() {

  Active=FALSE;
  DriveMap=Version=0;
  ContainerID[0]=ContainerID[1]=-1;
  LockedSide[0]=LockedSide[1]=-1;
  LockedTrack[0]=LockedTrack[1]=-1; 
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  CyclesRun=0;
#endif
  CapsInit(SS_IPF_PLUGIN_FILE);

  // drives
  SF314[0].type=SF314[1].type=sizeof(CapsDrive); // must be >=sizeof(CapsDrive)
  SF314[0].rpm=SF314[1].rpm=CAPSDRIVE_35DD_RPM;
  SF314[0].maxtrack=SF314[1].maxtrack=CAPSDRIVE_35DD_HST;

  // controller
  WD1772.type=sizeof(CapsFdc);  // must be >=sizeof(CapsFdc)
  WD1772.model=cfdcmWD1772;
  WD1772.clockfrq=SS_IPF_FREQU; 
  WD1772.drive=SF314; // ain't it cool?
  WD1772.drivecnt=2;
  WD1772.drivemax=0;

  if(CapsFdcInit(&WD1772)!=imgeOk)
    TRACE("CapsFdcInit failed, no IPF support\n");
  else
  {
    CapsVersionInfo versioninfo;
    VERIFY( !CapsGetVersionInfo((void*)&versioninfo,0) );
    ASSERT( !versioninfo.type );
    TRACE("Using CapsImg library V%d.%d\n",versioninfo.release,versioninfo.revision);
    Version=versioninfo.release*10+versioninfo.revision; // keep this for hacks
    ASSERT( Version==42 );
  }

  // the DLL will call them, strange that they're erased at FDC init
  WD1772.cbdrq=CallbackDRQ;
  WD1772.cbirq=CallbackIRQ;
  WD1772.cbtrk=CallbackTRK;
  return Version;
}


void TCaps::Reset() {
    CapsFdcReset(&WD1772);
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IMAGE_INFO

int TCaps::InsertDisk(int drive,char* File,CapsImageInfo *img_info) {
    ASSERT( !drive || drive==1 );
    ASSERT( !IsIpf(drive) );
    ASSERT( img_info );
    ContainerID[drive]=CapsAddImage();
    ASSERT( ContainerID[drive]!=-1 );
    WD1772.drivemax++; 
    ASSERT( WD1772.drivemax>=1 && WD1772.drivemax<=2 );
    DriveMap|=(drive+1); // not <<!
    bool FileIsReadOnly=bool(GetFileAttributes(File) & FILE_ATTRIBUTE_READONLY);
    TRACE_LOG("Adding CAPS drive ID %d for drive %c\n",Caps.ContainerID[drive],drive+'A');
    VERIFY( !CapsLockImage(ContainerID[drive],File) ); // open the IPF file
    VERIFY( !CapsGetImageInfo(img_info,ContainerID[drive]) );
    ASSERT( img_info->type==ciitFDD );
    TRACE_LOG("Disk in %c is CAPS release %d rev %d of %d/%d/%d for ",
      drive+'A',img_info->release,img_info->revision,img_info->crdt.day,
      img_info->crdt.month,img_info->crdt.year);
    bool found=0;
    for(int i=0;i<CAPS_MAXPLATFORM;i++)
    {
      if((img_info->platform[i])!=ciipNA)
        TRACE_LOG("%s ",CapsGetPlatformName(img_info->platform[i]));
      if(img_info->platform[i]==ciipAtariST)
        found=true;
    }
    TRACE_LOG("Sides:%d Tracks:%d-%d\n",img_info->maxhead+1,img_info->mincylinder,
      img_info->maxcylinder);
    ASSERT( found );
    if(!found)    // could be a Spectrum disk etc.
    {
      int Ret=FIMAGE_WRONGFORMAT;
      return Ret;
    }
    Active=TRUE;
    SF314[drive].diskattr|=CAPSDRIVE_DA_IN; // indispensable!
    if(!FileIsReadOnly)
      SF314[drive].diskattr&=~CAPSDRIVE_DA_WP; // Sundog
    CapsFdcInvalidateTrack(&WD1772,drive); // Galaxy Force II
    LockedTrack[drive]=LockedSide[drive]=-1;
  return 0;
}


void TCaps::RemoveDisk(int drive) {
    TRACE_LOG("Drive %c removing image and CAPS drive ID %d\n",drive+'A',Caps.ContainerID[drive]);
    VERIFY( !CapsUnlockImage(Caps.ContainerID[drive]) ); // eject disk
    VERIFY( CapsRemImage(Caps.ContainerID[drive])!=-1 ); // remove drive
    WD1772.drivemax--;
    ASSERT( WD1772.drivemax>=0 && WD1772.drivemax<2 );
    DriveMap&=~(drive+1); // not <<!
    if(!IsIpf(!drive)) // other drive
      Active=FALSE; 
}


#undef LOGSETION

void TCaps::WritePsgA(int data) {
  // drive selection (we don't refactor...)
  if ((psg_reg[PSGR_PORT_A]&BIT_1)==0 /*&& DriveIsIPF[0]*/)
    WD1772.drivenew=0;
  else if ((psg_reg[PSGR_PORT_A]&BIT_2)==0 /*&& DriveIsIPF[1]*/)
    WD1772.drivenew=1;
  else //if(DriveIsIPF[0])
    WD1772.drivenew=0;
  // side selection (Burger Man, Turbo Outrun...)
  ASSERT( !WD1772.drivenew || WD1772.drivenew==1 );
  SF314[WD1772.drivenew].newside=((psg_reg[PSGR_PORT_A]&BIT_0)==0);
}


UDWORD TCaps::ReadWD1772() {

#if defined(SS_DEBUG)
  Dma.UpdateRegs();
#endif

#if defined(SS_IPF_RUN_PRE_IO)
  CapsFdcEmulate(&WD1772,CYCLES_PRE_IO);
  CyclesRun+=CYCLES_PRE_IO;
#endif

  if(!(dma_mode & (BIT_1+BIT_2))) // read status register
  {
    mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
    WD1772.lineout&=~CAPSFDC_LO_INTRQ; // and in the WD1772
  }
  UDWORD data=CapsFdcRead(&WD1772,(dma_mode & (BIT_1+BIT_2))/2); 

#if defined(SS_IPF_RUN_POST_IO)
  CapsFdcEmulate(&WD1772,CYCLES_POST_IO);
  CyclesRun+=CYCLES_POST_IO;
#endif
  ASSERT(!(data&0xFF00));
  return data;//&0xbd ; //& 0xFD
}


int TCaps::WriteWD1772(int data) {

#if defined(SS_DEBUG) || !defined(VC_BUILD)
  Dma.UpdateRegs();
#endif

#if defined(SS_IPF_RUN_PRE_IO)
  CapsFdcEmulate(&WD1772,CYCLES_PRE_IO);
  CyclesRun+=CYCLES_PRE_IO;
#endif  
  int wd_address=(dma_mode&(BIT_1+BIT_2))>>1;
#if defined(SS_IPF_LETHAL_XCESS)
  if(SSE_HACKS_ON && !wd_address && Version==42) 
  { //targeted hack to make up for something missing in capsimg (?)
    if( (::WD1772.OldCr&0xF0)==0xD0 && (data&0xF0)==0xE0 ) 
      WD1772.lineout&=~CAPSFDC_LO_INTIP; // fixes Lethal Xcess
    ::WD1772.OldCr=data;
  }
#endif

  if(!wd_address) // command
  {
    mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Double Dragon II
    if(Version==42 && (data&0xE0)==0xA0)
    {
      TRACE("IPF unimplemented command %X\n",data);
      if(FDCWritingTimer<timer)
        FDCWritingTimer=timer+RED_LED_DELAY;	
#if defined(SS_IPF_WRITE_HACK)
      if(SSE_HACKS_ON && (data&0xE0)==0xA0) // =write sector (not track!)
      {
        TRACE("hack for write to IPF %d sectors\n",dma_sector_count);
        dma_address+=512*dma_sector_count;
        INSTRUCTION_TIME(256*dma_sector_count);
        dma_sector_count=0;
        //dma_status&=~BIT_1;
        dma_status=1;
      }
#endif
    }
    else
      FDCWriting=FALSE;
  }

  CapsFdcWrite(&WD1772,wd_address,data); // send to DLL

#if defined(SS_IPF_RUN_POST_IO)
  CapsFdcEmulate(&WD1772,CYCLES_POST_IO);
  CyclesRun+=CYCLES_POST_IO;
#endif

  return wd_address;
}


void TCaps::Hbl() {
  // we run cycles at each HBL if there's an IPF file in. Performance OK
#if defined(STEVEN_SEAGAL) && defined(SS_VIDEO)
  ASSERT( Shifter.CurrentScanline.Cycles>100)
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  ASSERT( Shifter.CurrentScanline.Cycles-Caps.CyclesRun>0 );
#endif
  CapsFdcEmulate(&WD1772,Shifter.CurrentScanline.Cycles
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
    -CyclesRun
#endif
    );
#else
    CapsFdcEmulate(&WD1772,screen_res==2? 160 : 512);
#endif

#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  CyclesRun=0;
#endif

}

int TCaps::IsIpf(int drive) {
  ASSERT(!drive||drive==1);
  return (DriveMap&(drive+1)); // not <<!
}


#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC_BYTES // to trace all bytes transferred!

/*  Callback functions. Since they're static, they access object data like
    any external function, using 'Caps.'
*/

void TCaps::CallbackDRQ(PCAPSFDC pc, UDWORD setting) {


#if defined(SS_DEBUG) || !defined(VC_BUILD)
  Dma.UpdateRegs();
  ASSERT( dma_mode&BIT_7 ); // DMA enabled
  ASSERT(!(dma_mode&BIT_6)); // DMA enabled
  ASSERT(!(dma_mode&BIT_3)); // Floppy
#endif
  if((dma_mode&BIT_7) && !(dma_mode&BIT_6))
  {
    if(!(dma_mode&BIT_8)) // disk->RAM
    {
      BYTE data_register=CapsFdcGetInfo(cfdciR_Data, &Caps.WD1772,0); 
      fdc_read_address_buffer[
#if defined(SS_DMA_DOUBLE_FIFO)
        16*Dma.BufferInUse+
#endif
        fdc_read_address_buffer_len++]=data_register;
    }
    else // RAM -> disk
      Caps.WD1772.r_data=fdc_read_address_buffer[
#if defined(SS_DMA_DOUBLE_FIFO)
        16*Dma.BufferInUse+
#endif
        fdc_read_address_buffer_len++];
/*
"The FIFOs are not flushed automatically at the end of a transfer, and therefore 
it is only possible to transfer data in multiples of 16 bytes"
*/
    ASSERT( fdc_read_address_buffer_len<=16 );
    if(fdc_read_address_buffer_len==16)
    { 
      Dma.RequestTransfer();
      fdc_read_address_buffer_len=0;
      disk_light_off_time=timeGetTime()+DisableDiskLightAfter;
      if(::WD1772.WritingToDisk()) // it's Steem's one!
        FDCWriting=TRUE;
    }
    Caps.WD1772.r_st1&=~CAPSFDC_SR_IP_DRQ; // The Pawn
    Caps.WD1772.lineout&=~CAPSFDC_LO_DRQ;
  }
}


#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

void TCaps::CallbackIRQ(PCAPSFDC pc, DWORD lineout) {
  //ASSERT( lineout&CAPSFDC_LO_INTRQ );// Manoir de Mortevielle
  //ASSERT( !mfp_interrupt_enabled[7] );  // Cyberball
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
//  TRACE_LOG("%d ",ABSOLUTE_CPU_TIME);
  if(TRACE_ENABLED) Dma.UpdateRegs(true);
#endif
  mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,!(lineout&CAPSFDC_LO_INTRQ));
  disk_light_off_time=timeGetTime()+DisableDiskLightAfter;
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IPF_LOCK_INFO

void TCaps::CallbackTRK(PCAPSFDC pc, UDWORD drive) {
  ASSERT( !drive||drive==1 );
  ASSERT( Caps.IsIpf(drive) );
  ASSERT( drive==floppy_current_drive() );
  CapsTrackInfoT2 track_info;
  track_info.type=2; 
  UDWORD flags=DI_LOCK_DENALT|DI_LOCK_DENVAR|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;
  int side=Caps.SF314[drive].side;
  ASSERT( side==!(psg_reg[PSGR_PORT_A]& BIT_0) );
  int track=Caps.SF314[drive].track;
  if(Caps.LockedTrack[drive]!=-1)
  {
    ASSERT( Caps.LockedSide[drive]!=-1 );
    ASSERT( track!=Caps.LockedTrack[drive] || side!=Caps.LockedSide[drive] );
    VERIFY( !CapsUnlockTrack(Caps.ContainerID[drive],Caps.LockedTrack[drive],
      Caps.LockedSide[drive]) );
    TRACE_LOG("CAPS Unlock %c:S%dT%d\n",drive+'A',Caps.LockedSide[drive],
      Caps.LockedTrack[drive]);
  }
  VERIFY( !CapsLockTrack((PCAPSTRACKINFO)&track_info,Caps.ContainerID[drive],
    track,side,flags) );
  ASSERT( side==track_info.head );
  ASSERT( track==track_info.cylinder );
  ASSERT( !track_info.sectorsize );
  ASSERT( !track_info.weakcnt ); // Dungeon Master?
  ASSERT( !track_info.timebuf ); // asserts with DI_LOCK_DENAUTO lock flag
  TRACE_LOG("CAPS Lock %c:S%dT%d flags %X sectors %d bits %d overlap %d startbit %d timebuf %x\n",
    drive+'A',side,track,flags,track_info.sectorcnt,track_info.tracklen,track_info.overlap,track_info.startbit,track_info.timebuf);
  Caps.SF314[drive].trackbuf=track_info.trackbuf;
  Caps.SF314[drive].timebuf=track_info.timebuf;
  Caps.SF314[drive].tracklen = track_info.tracklen;
  Caps.SF314[drive].overlap = track_info.overlap;
  Caps.LockedSide[drive]=side;
  Caps.LockedTrack[drive]=track;

#if defined(SS_IPF_TRACE_SECTORS)  // debug info
  CapsSectorInfo CSI;
  int sec_num;
  TRACE_LOG("sector info (encoder,cell type,data,gap info)\n");
  for(sec_num=1;sec_num<=track_info.sectorcnt;sec_num++)
  {
    CapsGetInfo(&CSI,Caps.ContainerID[drive],track,side,cgiitSector,sec_num-1);
    TRACE_LOG("#%d|%d|%d|%d %d %d|%d %d %d %d %d %d %d\n",
      sec_num,
      CSI.enctype,      // encoder type
      CSI.celltype,     // bitcell type
      CSI.descdatasize, // data size in bits from IPF descriptor
      CSI.datasize,     // data size in bits from decoder
      CSI.datastart,    // data start position in bits from decoder
      CSI.descgapsize,  // gap size in bits from IPF descriptor
      CSI.gapsize,      // gap size in bits from decoder
      CSI.gapstart,     // gap start position in bits from decoder
      CSI.gapsizews0,   // gap size before write splice
      CSI.gapsizews1,   // gap size after write splice
      CSI.gapws0mode,   // gap size mode before write splice
      CSI.gapws1mode);   // gap size mode after write splice
  }
#endif

}

#undef LOGSECTION

#endif//ipf

#endif