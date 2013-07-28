#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"


#if defined(SS_DMA)

TDma Dma;

TDma::TDma() {
  Request=false;
#if defined(SS_DMA_DOUBLE_FIFO)
  BufferInUse=0;
#endif
}

#if defined(SS_DMA) 
/*  Read/write on disk DMA registers.
*/

#define LOGSECTION LOGSECTION_IO // DMA+FDC

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

  ASSERT( (addr&0xFFFF00)==0xFF8600 );
  BYTE ior_byte=0xFF;

  TRACE_LOG("DMA R %X ",addr);

  // test for bus error
  if(addr>0xff860f || addr<0xff8604 || addr<0xff8608 && !io_word_access) 
    exception(BOMBS_BUS_ERROR,EA_READ,addr);

  switch(addr)
  {

  case 0xff8604:
    // sector counter
    ASSERT(!(MCR & BIT_4)); 
    if(MCR & BIT_4) 
#if defined(SS_DMA_SECTOR_COUNT)
/*
"The sector count register is write only. Reading this register return
 unpredictable values."
*/
      ior_byte=(rand()&0xFF); // or FF?
#else
      ior_byte=HIBYTE(Counter); 
#endif
    // HD access
    else if(MCR & BIT_3) 
    {
      LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
        " - Reading high byte of HDC register #"+((MCR & BIT_1) ? 1:0)); )
    }
    // high byte of FDC
    else
#if defined(SS_DMA_FDC_READ_HIGH_BYTE)
      ior_byte=0x00;// like in Pasti
#else
/*
"The FDC registers only uses 8 bits when writing and therefore the upper byte 
is ignored and when reading the 8 upper bits consistently reads 1."
*/
      ;
#endif
    break;

  case 0xff8605: 
    // sector counter
    if(MCR & BIT_4) 
    {
      TRACE_LOG("Counter");
#if defined(SS_DMA_SECTOR_COUNT)
      ior_byte=(rand()&0xFF);
#else
      ior_byte=LOBYTE(Counter); 
#endif
    }
    // HD access
    else if(MCR & BIT_3) 
    {
      TRACE_LOG("HD");
      LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                  " - Reading low byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )
    }
    // Read FDC register
#if defined(SS_DMA_FDC_ACCESS)
/*
Bit 7:FDC/HDC transfers acknowledge; when set the DRQ from the Floppy 
Disk Controller is acknowledged; otherwise, the DRQ from the Hard Disk 
interface is acknowledged. For some reason this bit must also be set 
to generate DMA bus cycle.
*/
    else if(!(MCR&BIT_7)) // we can't
    {
      TRACE_LOG("No FDC access DMA MCR %x",MCR);
    }
#endif
    else
#if defined(SS_FDC)
      ior_byte=WD1772.IORead( (MCR&(BIT_1+BIT_2))/2 );
#else // old ior block... ugly but flexible - normally not compiled
    {
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
    }
#endif

    break;

  case 0xff8606:  //high byte of DMA status
    ior_byte=0; // only bits 0-2 of low byte are used
    break;

  case 0xff8607:  //low byte of DMA status
    TRACE_LOG("SR");
#if defined(SS_DMA_READ_STATUS)
/*
"If the DMA status word is polled during a DMA operation the transfer might
 be disrupted."
Not emulated
*/
    ior_byte=BYTE(b00000111) & SR; // fixes Vindicators IPF
#else
    ior_byte=BYTE(b11110000) | SR;
#endif
    break;
/*
"The DMA Address Counter register must be read in a High, Mid, Low order."
TODO?
*/
  case 0xff8609:  // DMA Base and Counter High
    TRACE_LOG("BaseAddress");
    ior_byte=(BYTE)((BaseAddress&0xff0000)>>16);
    break;

  case 0xff860b:  // DMA Base and Counter Mid
    TRACE_LOG("BaseAddress");
    ior_byte=(BYTE)((BaseAddress&0xff00)>>8);
    break;

  case 0xff860d:  // DMA Base and Counter Low
    TRACE_LOG("BaseAddress");
    ior_byte=(BYTE)((BaseAddress&0xff));
    break;

  case 0xff860e: //frequency/density control
  {
    TRACE_LOG("Density");
    BYTE drive=floppy_current_drive();
    if(FloppyDrive[drive].STT_File) 
      ior_byte=0;
    else
    {
      TFloppyImage *floppy=&(FloppyDrive[drive]);
      ior_byte=BYTE((floppy->BytesPerSector * floppy->SectorsPerTrack)>7000);
    }
    break;
  }

  case 0xff860f: //high byte of frequency/density control?
    ior_byte=0;
    break;
  }//sw

#if USE_PASTI 
/*  Pasti handles all Dma reads - this cancels the first value
    of ior_byte, but allows to go through TRACE and update our variables..
*/
  if(hPasti && pasti_active
#if defined(STEVEN_SEAGAL) && defined(SS_PASTI_ONLY_STX)
    && (!PASTI_JUST_STX || SF314[floppy_current_drive()].ImageType==3
   // ||addr!=0xff8605||MCR&BIT_3||MCR&BIT_4
    )
#endif        
    )
  {
    TRACE_LOG(" Pasti");
    if(addr<0xff8608 && (addr & 1))
    {
      ior_byte=LOBYTE(pasti_store_byte_access);
    }
    else
    {
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
      ior_byte=BYTE(pioi.data);
    }
  }
#endif

  TRACE_LOG(" .B=%X\n",ior_byte);

  return ior_byte;
}


void TDma::IOWrite(MEM_ADDRESS addr,BYTE io_src_b) {

  ASSERT( (addr&0xFFFF00)==0xFF8600 );

  TRACE_LOG("DMA W %X ",addr);

  // test for bus error
  if(addr>0xff860f || addr<0xff8604 || addr<0xff8608 && !io_word_access) 
    exception(BOMBS_BUS_ERROR,EA_WRITE,addr);

  switch (addr)
  {

  case 0xff8604:
    //write DMA sector counter, 0x190
    if(MCR & BIT_4)
    { 
      ASSERT(!io_src_b);
      Counter&=0xff;
      Counter|=int(io_src_b) << 8;
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
      break;
    }
    // HD access
    if(MCR & BIT_3)
    { 
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Writing $"+HEXSl(io_src_b,2)+"xx to HDC register #"+((dma_mode & BIT_1) ? 1:0));
      break;
    }
/*
"The FDC registers only use 8 bits when writing and therefore the upper byte 
is ignored and when reading the 8 upper bits consistently reads 1."
*/

    break;
    
  case 0xff8605:  
    //write FDC sector counter, 0x190
    if (MCR & BIT_4)
    { 
      TRACE_LOG("Counter");
      Counter&=0xff00;
      Counter|=io_src_b;
      // We need do that only once (word access):
      if (Counter)
        SR|=BIT_1;
      else
        SR&=BYTE(~BIT_1); //status register bit for 0 count
      ByteCount=0;
/*
"It is interesting to note that when the DMA is in write mode, the two internal
 FIFOS are filled immediately after the Count Register is written."
 RAM to disk
 -> to simplify emulation, we fill FIFO only at first DRQ (and each time
    it' empty)
*/
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
      break;
    }
    // HD access
    if (MCR & BIT_3){ 
      TRACE_LOG("HD");
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
    ASSERT( MCR&BIT_7 );
    if(!(MCR&BIT_7))
      break;
#endif

#if defined(SS_FDC)
    WD1772.IOWrite((MCR&(BIT_1+BIT_2))/2,io_src_b);
#else // old iow block... ugly but flexible - normally not compiled
    {
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

    }
#endif
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
    if( !(MCR&0x100) ^ !(io_src_b) ) // fixes Archipelagos IPF
    {
      TRACE_LOG("Reset");
#if !defined(SS_DMA_FIFO_READ_ADDRESS2)
      fdc_read_address_buffer_len=0;// this is only for command III read address
#endif
      Counter=0;
      ByteCount=0;
      SR=1;
      Request=false;
      Fifo_idx=0;
#if defined(SS_DMA_DOUBLE_FIFO) 
      BufferInUse=0;
#endif
    }
    MCR&=0x00ff;
    MCR|=WORD(WORD(io_src_b) << 8);
#else
    MCR&=0x00ff;
    MCR|=WORD(WORD(io_src_b) << 8);
#if !defined(SS_DMA_FIFO_READ_ADDRESS2)
    fdc_read_address_buffer_len=0;
#endif
    ByteCount=0;
#endif
    break;
    
  case 0xff8607:  //low byte of DMA mode
    TRACE_LOG("CR");
    ASSERT(!(io_src_b&1)); // Omega, Union
    ASSERT(!(io_src_b&BIT_5));
    MCR&=0xff00;
    MCR|=io_src_b;
#if !defined(SS_DMA_WRITE_CONTROL) // see above
#if !defined(SS_DMA_FIFO_READ_ADDRESS2)
    fdc_read_address_buffer_len=0;
#endif
    ByteCount=0;
#endif
    break;
    
  case 0xff8609:  // DMA Base and Counter High
    TRACE_LOG("BaseAddress");
    BaseAddress&=0x00ffff;
    BaseAddress|=((MEM_ADDRESS)io_src_b) << 16;
    log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Set DMA address to "+HEXSl(BaseAddress,6));
    break;
    
  case 0xff860b:  // DMA Base and Counter Mid
    TRACE_LOG("BaseAddress");
#if defined(SS_BaseAddress)
/* 
"The DMA Address Counter register must be loaded (written) in a Low, Mid, 
  High order."
*/
    BaseAddress&=0x0000ff;
    BaseAddress|=((MEM_ADDRESS)io_src_b) << 8;
    //BaseAddress|=0xff0000;
#else
    BaseAddress&=0xff00ff;
    BaseAddress|=((MEM_ADDRESS)io_src_b) << 8;
#endif
    break;
    
  case 0xff860d:  // DMA Base and Counter Low
    TRACE_LOG("BaseAddress L");
    ASSERT( !(io_src_b&1) ); // shouldn't the address be even?
#if defined(SS_BaseAddress)
    //BaseAddress=0xffff00,BaseAddress|=io_src_b;
    BaseAddress=io_src_b;
#else
    BaseAddress&=0xffff00;
    BaseAddress|=io_src_b;
#endif       
    break;
    
  case 0xff860e: //high byte of frequency/density control
    break; //ignore
    
  case 0xff860f: //low byte of frequency/density control
    break;
  }//sw

  
#if USE_PASTI 
/*  Pasti handles all DMA writes, still we want to update our variables
    and go through TRACE.
*/
  if(hPasti && pasti_active
    
#if defined(STEVEN_SEAGAL)&&defined(SS_DRIVE)&&defined(SS_PASTI_ONLY_STX)
    && (!PASTI_JUST_STX || SF314[floppy_current_drive()].ImageType==3
    ||addr!=0xff8605||MCR&BIT_3||MCR&BIT_4
    )
#endif        
    )
  {
    TRACE_LOG(" Pasti");
    WORD data=io_src_b;

    if(addr<0xff8608 && !(addr&1))
      pasti_store_byte_access=io_src_b;
    else
    {
      if(addr<0xff8608)
      {
        ASSERT( addr&1 );
        data=MAKEWORD(io_src_b,pasti_store_byte_access);
        addr&=~1;
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
    }
  }
#endif

  TRACE_LOG(" =.B %X\n",io_src_b);

}

#endif


void TDma::UpdateRegs(bool trace_them) {
/*  This is used for debugging and for pasti drive led
    and may have some other uses.
    We update both DMA and FDC registers (so we use old variable names for the
    latter) - we could have a TWD1772::UpdateRegs() as well.
*/

#if USE_PASTI
  if(hPasti && pasti_active
#if defined(STEVEN_SEAGAL) && defined(SS_PASTI_ONLY_STX) //all or nothing?
    && (!PASTI_JUST_STX || SF314[floppy_current_drive()].ImageType==3)
#endif      
    )
  {
    pastiPEEKINFO ppi;
    pasti->Peek(&ppi);
    fdc_cr=ppi.commandReg;
    fdc_str=ppi.statusReg;
    fdc_tr=ppi.trackReg;
    fdc_sr=ppi.sectorReg;
    fdc_dr=ppi.dataReg;
    floppy_head_track[0]=ppi.drvaTrack;
    floppy_head_track[1]=ppi.drvbTrack;
    Counter=ppi.dmaCount;
    BaseAddress=ppi.dmaBase;
    MCR=ppi.dmaControl;
    SR=ppi.dmaStatus;
  }
#endif
#if defined(SS_IPF)
  if(CAPSIMG_OK && Caps.IsIpf(floppy_current_drive()))
  {
    int ext=0;
    fdc_cr=CAPSFdcGetInfo(cfdciR_Command, &Caps.WD1772,ext);
    fdc_str=CAPSFdcGetInfo(cfdciR_ST, &Caps.WD1772,ext);
    fdc_tr=CAPSFdcGetInfo(cfdciR_Track, &Caps.WD1772,ext);
    fdc_sr=CAPSFdcGetInfo(cfdciR_Sector, &Caps.WD1772,ext);
    fdc_dr=CAPSFdcGetInfo(cfdciR_Data, &Caps.WD1772,ext);
    floppy_head_track[0]=Caps.SF314[0].track;
    floppy_head_track[1]=Caps.SF314[1].track;
  } 
#endif
  if(trace_them)
  {
    TRACE_LOG("FDC IRQ CR %X STR %X ",fdc_cr,fdc_str);
#if defined(SS_FDC_TRACE_STATUS)
    WD1772.TraceStatus();
#endif
    TRACE_LOG("TR %d SR %d DR %d ($%X)",fdc_tr,fdc_sr,fdc_dr,fdc_dr);
#if defined(SS_PSG)
    BYTE drive=YM2149.Drive();
    BYTE side=YM2149.Side();
    TRACE_LOG(" %c%d:",'A'+drive,side);
#endif
    TRACE_LOG(" T %d/%d DMA CR %X $%X SR %X #%d PC %X\n",
      floppy_head_track[0],floppy_head_track[1],MCR,BaseAddress,SR,Counter,pc);
  }
}


#undef LOGSECTION


#if defined(SS_STRUCTURE_DMA_INC_ADDRESS)
/*  We see no need for an inline function/macro for disk operation.
    This is the same as the macro in fdc.cpp but with our new names.
    "The count should indicate the number of 512 bytes chunks that 
    the DMA will have to transfer
    "This register is decrement by one each time 512 bytes has been
    transferred. When the sector count register reaches zero the DMA
    will stop to transfer data. Only the lower 8 bits are used."
    -> it's always 512 bytes, regardless of sector size
*/

void TDma::IncAddress() {
  if (Counter&0xFF){                                   
    BaseAddress++;                                         
    ByteCount++;                  
    if (ByteCount>=512) 
    {        
      ByteCount=0;              
      Counter--;                                  
      SR|=BIT_1;  // DMA sector count not 0
      if(!(Counter&0xFF)) 
        SR&=~BIT_1;     
    }                                                      
  }
}
#endif


#if defined(SS_DMA_FIFO)
/*  We try to emulate the DMA FIFO transfers in a more accurate way.
    In previous versions the bytes were directly moved between ST RAM
    and disk image. Now there's a real automatic buffer working both
    ways.
    SS_DMA_DOUBLE_FIFO has been tested but isn't defined, overkill, 
    so we spare some data bytes(17); strangely EXE size stays the same.
*/

void TDma::AddToFifo(BYTE data) {
  // DISK -> FIFO -> RAM = 'read disk' = 'write dma'

  ASSERT( (MCR&BIT_7) && !(MCR&BIT_6) );
  ASSERT( !(MCR&BIT_8) ); // disk->RAM (Read floppy)
  ASSERT( Fifo_idx<16 );

  Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
    [BufferInUse]
#endif
    [Fifo_idx++]=data;

  if(Fifo_idx==16)
    Dma.RequestTransfer();
}


/*  For writing to disk, we use the FIFO in the other direction,
    because we want it to fill up when empty.
    On a real ST, the first DMA transfer happens right when the
    sector count has been set.
    This could be important in theory, but know no case.
*/

BYTE TDma::GetFifoByte() {
  // RAM -> FIFO -> DISK = 'write disk' = 'read dma'

  ASSERT( (MCR&BIT_7) && !(MCR&BIT_6) );
  ASSERT( (MCR&BIT_8) ); // RAM -> disk (Write floppy)

  if(!Fifo_idx)
    Dma.RequestTransfer();

  BYTE data=Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
    [BufferInUse]
#endif
    [--Fifo_idx];

  return data;  

}


void TDma::RequestTransfer() {
  // we make this function to avoid code duplication
  Request=true; 
  
  Fifo_idx= (MCR&0x100) ? 16 : 0;

#if defined(SS_DMA_DOUBLE_FIFO) // normally not
  BufferInUse=!BufferInUse; // toggle 16byte buffer
#endif

#if defined(SS_DMA_DELAY) // normally not - set event
  TransferTime=ABSOLUTE_CPU_TIME+SS_DMA_ACCESS_DELAY;
  PREPARE_EVENT_CHECK_FOR_DMA
#else
  TransferBytes(); // direct transfer
#endif
}


#if defined(SS_DMA_DELAY) // normally not
// this is a Steem 'event' (static member function)

void TDma::Event() {
#if defined(SS_MFP_RATIO)
  Dma.TransferTime=ABSOLUTE_CPU_TIME+CpuNormalHz;
#else
  Dma.TransferTime=ABSOLUTE_CPU_TIME+8000000;
#endif
  if(Dma.Request)
    Dma.TransferBytes();
}

#endif


#define LOGSECTION LOGSECTION_FDC_BYTES


void TDma::TransferBytes() {
  // execute the DMA transfer (assume floppy)
  ASSERT( MCR&0x80 ); // bit 7 or no transfer
  ASSERT(!(MCR&BIT_5));
  ASSERT( Request );
  ASSERT( MCR==0x80 || MCR==0x90 || MCR==0x180 || MCR==0x190 );

  if(!(MCR&0x100)) // disk -> RAM
    TRACE_LOG("%2d/%2d/%3d to %X: ",fdc_tr,fdc_sr,ByteCount,BaseAddress);
  else  // RAM -> disk
    TRACE_LOG("%2d/%2d/%3d from %X: ",fdc_tr,fdc_sr,ByteCount,BaseAddress);
  
  for(int i=0;i<16;i++) // burst, 16byte packets strictly, 8 words
  {
    if(!(MCR&0x100)&& DMA_ADDRESS_IS_VALID_W) // disk -> RAM
      PEEK(BaseAddress)=
        Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
        [!BufferInUse] // because it's been just toggled
#endif
        [i];
    
    else if((MCR&0x100) && DMA_ADDRESS_IS_VALID_R) // RAM -> disk
      Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
        [BufferInUse] // because we fill the new buffer
#endif
        [15-i]=PEEK(BaseAddress);
    else TRACE_LOG("!!!");
    TRACE_LOG("%02X ",Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
      [!BufferInUse]
#endif
      [i]);

    if(hPasti&&pasti_active
#if defined(STEVEN_SEAGAL) && defined(SS_PASTI_ONLY_STX)
    &&(!PASTI_JUST_STX || SF314[floppy_current_drive()].ImageType==3)
#endif        
      )  
      dma_address++;
    else
      DMA_INC_ADDRESS; // use Steem's existing routine
  }
#if defined(SS_DMA_COUNT_CYCLES) 
  INSTRUCTION_TIME(8); 
#endif
  TRACE_LOG("\n");
  Request=FALSE;
}

#undef LOGSECTION

#endif//DMA_FIFO

#endif//dma



#if defined(SS_DRIVE)
/*  Problems concerning the drive itself (RPM...) or the disks (gaps...) are
    treated here.
*/
/*  TODO
    There are still some hacks in drive emulation, we keep them so some
    programs run:
    Microprose Golf
    FDCTNF by Petari
    See SSEParameters.h
*/

TSF314 SF314[2]; // cool!


/*

from JLG, floppy disk gaps

# bytes 
Name                       9 Sectors  10 Sectors  11 Sectors     Byte
Gap 1 Post Index                  60          60          10      4E

Gap 2 Pre ID                    12+3        12+3         3+3     00+A1
ID Address Mark                    1           1           1      FE
ID                                 6           6           6
Gap 3a Post ID                    22          22          22      4E 
Gap 3b Pre Data                 12+3        12+3        12+3     00+A1
Data Address Mark                  1           1           1      FB
Data                             512         512         512
CRC                                2           2           2
Gap 4 Post Data                   40          40           1      4E

Record bytes                     614         614         566

Gap 5 Pre Index                  664          50          20      4E

Total track                     6250        6250        6256


    Note that the index position can be anywhere when the disk begins
    to spin.
    But, from Hatari:
    "
 * As the FDC waits 6 index pulses during the spin up phase, this means
 * that when motor reaches its desired speed an index pulse was just
 * encountered.
 * So, the position after peak speed is reached is not random, it will always
 * be 0 and we set the index pulse time to "now".
    " - TODO
    We do our computing using bytes, then convert the result into HBL, the
    timing unit for drive operations in Steem.
   
*/

WORD TSF314::BytePosition() {
  return HblsToBytes( hbl_count % HblsPerRotation() );
}


WORD TSF314::BytePositionOfFirstId() { // with +7 for reading ID
  return ( PostIndexGap() + ( (nSectors()<11)?12+3+7:3+3+7) );
}


WORD TSF314::BytesToHbls(int bytes) {
  return HblsPerRotation()*bytes/TRACK_BYTES;
}


unsigned long TSF314::HblsAtIndex() {
  return (hbl_count/HblsPerRotation())*HblsPerRotation();
}


WORD TSF314::HblsPerRotation() {
  return HBL_PER_SECOND/(RPM/60); // see SSEParameters
}


WORD TSF314::HblsPerSector() {
  return nSectors()?(HblsPerRotation()-BytesToHbls(TrackGap()))/nSectors() : 0;
}


WORD TSF314::HblsToBytes(int hbls) {
  return TRACK_BYTES*hbls/HblsPerRotation();
}


void TSF314::NextID(BYTE &Id,WORD &nHbls) {
/*  This routine was written to improve timing of command 'Read Address',
    used by ProCopy. Since it exists, it is also used for 'Verify'.
*/
  Id=0;
  nHbls=0;
  if(FloppyDrive[floppy_current_drive()].Empty()) //3.5.2
  //if(FloppyDrive[Id].Empty())
    return;
  WORD BytesToRun;
  WORD ByteOfNextId=BytePositionOfFirstId();//default
  WORD BytePositionOfLastId=ByteOfNextId+(nSectors()-1)*RecordLength();
  WORD CurrentByte=BytePosition(); 
  // still on this rev
  if(CurrentByte<ByteOfNextId) // before first ID
    BytesToRun=ByteOfNextId-CurrentByte;
  else if(CurrentByte<BytePositionOfLastId) // before last ID
  {
    while(CurrentByte>=ByteOfNextId)
      ByteOfNextId+=RecordLength();
    BytesToRun=ByteOfNextId-CurrentByte;
    Id=(ByteOfNextId-BytePositionOfFirstId())/RecordLength();
    ASSERT( Id>=1 && Id<nSectors() );
  }
  // next rev
  else
    BytesToRun=TRACK_BYTES-CurrentByte+ByteOfNextId;
  nHbls=BytesToHbls(BytesToRun);
}


BYTE TSF314::nSectors() { 
  // should we add IPF, STX? - here we do native
  BYTE nSects;
#if defined(SS_PSG)
  if(FloppyDrive[Id].STT_File)
  {
    FDC_IDField IDList[30]; // much work each time, but STT rare
    nSects=FloppyDrive[Id].GetIDFields(YM2149.Side(),Track(),IDList);
  }
  else
#endif
    nSects=FloppyDrive[Id].SectorsPerTrack;
  return nSects;
}


BYTE TSF314::PostIndexGap() {
  return (nSectors()<11)? 60 : 10;
}


BYTE TSF314::PreDataGap() {
  int gap=0;
  switch(nSectors())
  {
  case 9:
  case 10: // with ID (7) and DAM (1)
    gap=12+3+7+22+12+3+1; 
    break;
  case 11:
    gap=3+3+7+22+12+3+1;
    break;
  }
  return gap;
}


BYTE TSF314::PostDataGap() {
  return (nSectors()<11)? 40 : 1;
}


WORD TSF314::PreIndexGap() {
  int gap=0;
  switch(nSectors())
  {
  case 9:
    gap=664;
    break;
  case 10:
    gap=50;
    break;
  case 11:
    gap=20;
    break;
  }
  return gap;
}


WORD TSF314::RecordLength() {
  return (nSectors()<11)? 614 : 566;
}


BYTE TSF314::SectorGap() {
  return PostDataGap()+PreDataGap();
}


BYTE TSF314::Track() {
  return floppy_head_track[Id]; //eh eh
}


WORD TSF314::TrackGap() {
  return PostIndexGap()+PreIndexGap();
}

#endif//drive



#if defined(SS_FDC)

#define LOGSECTION LOGSECTION_FDC

TWD1772 WD1772;


BYTE TWD1772::CommandType(int command) {
  // return type 1-4 of this FDC command
  if(command==-1) //default: current command
    command=CR;
  BYTE type;
  if(!(command&BIT_7))
    type=1;
  else if(!(command&BIT_6))
    type=2;
  else if((command&0xF0)==0xD0) //1101
    type=4;
  else
    type=3;
  return type;
}


BYTE TWD1772::IORead(BYTE Line) {

  ASSERT( Line>=0 && Line<=3 );
  BYTE ior_byte;
  BYTE drive=floppy_current_drive();

    // Steem handling
    switch(Line){
    case 0: // STR
      // Update some flags before returning STR
      // IP
      if(floppy_track_index_pulse_active())
        STR|=FDC_STR_T1_INDEX_PULSE;
      else
        // If not type 1 command we will get here, it is okay to clear
        // it as this bit is only for the DMA chip for type 2/3.
        STR&=BYTE(~FDC_STR_T1_INDEX_PULSE);
      // WP, SU
      if(floppy_type1_command_active)
      {
        STR&=(~FDC_STR_WRITE_PROTECT);
        if(floppy_mediach[drive])
        {
          if(floppy_mediach[drive]/10!=1) 
            STR|=FDC_STR_WRITE_PROTECT;
          else if (FloppyDrive[drive].ReadOnly)
            STR|=FDC_STR_WRITE_PROTECT;
        }
        // seems OK, no reset after command:
        if(fdc_spinning_up)
          STR&=BYTE(~FDC_STR_T1_SPINUP_COMPLETE);
        else
          STR|=FDC_STR_T1_SPINUP_COMPLETE;
        if(ADAT&&fdc_tr) { // (ADAT: v3.5.1, No Cooper) //TR or CYL?
          STR|=FDC_STR_T1_TRACK_0;
        }
      } // else it should be set in fdc_execute()
      if ((mfp_reg[MFPR_GPIP] & BIT_5)==0)
      {
        LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
          " - Reading status register as "+Str(itoa(STR,d2_t_buf,2)).LPad(8,'0')+
          " ($"+HEXSl(STR,2)+"), clearing IRQ"); )
        floppy_irq_flag=0;
        mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
      }
      ior_byte=STR;
#if !defined(SS_DEBUG_TRACE_IDE)
      TRACE_LOG("FDC STR %X\n",ior_byte);
#endif
      break;
    case 1:
      TRACE_LOG("FDC R TR %d\n",ior_byte);
      ior_byte=TR;
      break;      
    case 2:
      TRACE_LOG("FDC R SR %d\n",ior_byte);
      ior_byte=SR;
      break;
    case 3:
      TRACE_LOG("FDC R DR %d\n",ior_byte);
      ior_byte=DR;
      break;
    }//sw

    // IPF handling
#if defined(SS_IPF)
    if(Caps.IsIpf(drive))
      ior_byte=Caps.ReadWD1772(Line);
#endif
  return ior_byte;
}


void TWD1772::IOWrite(BYTE Line,BYTE io_src_b) {

  ASSERT( Line>=0 && Line<=3 );
  BYTE drive=floppy_current_drive();

  switch(Line)
  {
  case 0: // CR - could be blocked, can't record here :(
    {
#if defined(SS_DEBUG)
      BYTE drive_char= (psg_reg[PSGR_PORT_A]&6)==6? '?' : 'A'+DRIVE;
      TRACE_LOG("FDC CR $%2X drive %c side %d TR %d SR %d DR %d\n",io_src_b,drive_char,floppy_current_side(),fdc_tr,fdc_sr,fdc_dr);
#endif
      bool can_send=true; // are we in Steem's native emu?
#if defined(SS_IPF)
      can_send=can_send&&!Caps.IsIpf(drive);
#endif
#if USE_PASTI 
      can_send=can_send&&!(hPasti && pasti_active
#if defined(STEVEN_SEAGAL)&&defined(SS_DRIVE)&&defined(SS_PASTI_ONLY_STX)
        && (!PASTI_JUST_STX || SF314[floppy_current_drive()].ImageType==3)
#endif            
        );
#endif
      if(can_send)
        floppy_fdc_command(io_src_b); // in fdc.cpp
 //     else
 //       TRACE_LOG("FDC %X not native, drive %C type %d\n",io_src_b,'A'+YM2149.Drive(),SF314[YM2149.Drive()].ImageType);
    }
    break;
  case 1: // TR
    TRACE_LOG("FDC TR %d\n",io_src_b);
#if defined(SS_FDC_CHANGE_TRACK_WHILE_BUSY)
    if(!(STR&FDC_STR_BUSY)||ADAT)
      fdc_tr=io_src_b;
#else
    if (!(STR&FDC_STR_BUSY)){
      log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC track register to "+io_src_b);
      TR=io_src_b;
    }else{
      log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Can't set FDC track register to "+io_src_b+", FDC is busy");
    }
#endif
    break;
  case 2: // SR
    TRACE_LOG("FDC SR %d\n",io_src_b);
#if defined(SS_FDC_CHANGE_SECTOR_WHILE_BUSY)
    if(!(STR&FDC_STR_BUSY)||ADAT)
      SR=io_src_b; // fixes Delirious 4 loader without Pasti
    
#else
    if (!(STR & FDC_STR_BUSY)){
      log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC sector register to "+io_src_b);
      SR=io_src_b;
    }else{
      log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Can't set FDC sector register to "+io_src_b+", FDC is busy");
    }
#endif
    break;
  case 3: // DR
    TRACE_LOG("FDC DR %d\n",io_src_b);
    log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC data register to "+io_src_b);
    DR=io_src_b;
    break;
  }
  
  // IPF handling
#if defined(SS_IPF)
  if(Caps.IsIpf(drive)) 
    Caps.WriteWD1772(Line,io_src_b);
#endif
  
}


int TWD1772::WritingToDisk() {
  return((CR&0xF0)==0xF0 || (CR&0xE0)==0xA0);
}


#if defined(SS_DEBUG)

/*
Bits:
Bit 7 - Motor On.  This bit is high when the drive motor is on, and
low when the motor is off.

Bit 6 - Write Protect.  This bit is not used during reads.  During
writes, this bit is high when the disk is write protected.

Bit 5 - Spin-up / Record Type.  For Type I commands, this bit is low
during the 6-revolution motor spin-up time.  This bit is high after
spin-up.  For Type II and Type III commands, Bit 5 low indicates a
normal data mark.  Bit 5 high indicates a deleted data mark.

Bit 4 - Record Not Found.  This bit is set if the 177x cannot find the
track, sector, or side which the CPU requested.  Otherwise, this bit
is clear.

Bit 3 - CRC Error.  This bit is high if a sector CRC on disk does not
match the CRC which the 177x computed from the data.  The CRC
polynomial is x^16+x^12+x^5+1.  If the stored CRC matches the newly
calculated CRC, the CRC Error bit is low.  If this bit and the Record
Not Found bit are set, the error was in an ID field.  If this bit is
set but Record Not Found is clear, the error was in a data field.

Bit 2 - Track Zero / Lost Data.  After Type I commands, this bit is 0
if the mechanism is at track zero.  This bit is 1 if the head is not
at track zero.  After Type II or III commands, this bit is 1 if the
CPU did not respond to Data Request (Status bit 1) in time for the
177x to maintain a continuous data flow.  This bit is 0 if the CPU
responded promptly to Data Request.

Bit 1 - Index / Data Request.  On Type I commands, this bit is high
during the index pulse that occurs once per disk rotation.  This bit
is low at all times other than the index pulse.  For Type II and III
commands, Bit 1 high signals the CPU to handle the data register in
order to maintain a continuous flow of data.  Bit 1 is high when the
data register is full during a read or when the data register is empty
during a write.  "Worst case service time" for Data Request is 23.5
cycles.

Bit 0 - Busy.  This bit is 1 when the 177x is busy.  This bit is 0
when the 177x is free for CPU commands.
*/


void TWD1772::TraceStatus() {
  // this embarrassing part proves my ignorance!
  int type=CommandType(fdc_cr);
  TRACE_LOG("( ");
  if(STR&0x80)
    TRACE_LOG("MO ");//Motor On
  if(STR&0x40)
    TRACE_LOG("WP ");// write protect
  if(STR&0x20)
    if(type==1)
      TRACE_LOG("SU "); // Spin-up (meaning up to speed)
    else
      TRACE_LOG("RT "); //Record Type (1=deleted data)
  if(STR&0x10)
    if(type==1)
      TRACE_LOG("SE ");//Seek Error
    else
      TRACE_LOG("RNF ");//Record Not Found
  if(STR&0x08)
    TRACE_LOG("CRC "); //CRC Error
  if(STR&0x04)
    if(type==1) 
      TRACE_LOG("T0 "); // track zero
    else
      TRACE_LOG("LD ");//Lost Data, normally impossible on ST
  if(STR&0x02)
    if(type==1) 
      TRACE_LOG("IP "); // index
    else
      TRACE_LOG("DRQ "); // data request
  if(STR&0x01)
    TRACE_LOG("BSY "); // busy
    TRACE_LOG(") "); 
}

#endif//debug


#endif//FDC



#if defined(SS_PSG)
/*  In v3.5.1, object YM2149 is only used for drive management.
*/


TYM2149 YM2149;


BYTE TYM2149::Drive(){
  BYTE drive=NO_VALID_DRIVE; // different from floppy_current_drive()
  if(!(PortA()&BIT_1))
    drive=0; //A:
  else if(!(PortA()&BIT_2))
    drive=1; //B:
  return drive;
}


BYTE TYM2149::PortA(){
  return psg_reg[PSGR_PORT_A];
}


BYTE TYM2149::Side(){
  return (PortA()&BIT_0)==0;  //0: side 1;  1: side 0 !
}

#endif//PSG


#if defined(SS_IPF) // Implementation of CAPS support in Steem

#define CYCLES_PRE_IO 100 // those aren't
#define CYCLES_POST_IO 100 // used

TCaps Caps; // singleton

TCaps::TCaps() {
  // we init in main to keep control of timing
}


TCaps::~TCaps() {
  if(CAPSIMG_OK)
    CAPSExit();
}

#if !defined(SS_STRUCTURE_BIG_FORWARD)
void SetNotifyInitText(char*);//forward
#endif
#undef LOGSECTION
#define LOGSECTION LOGSECTION_INIT//SS


int TCaps::Init() {

  Active=FALSE;
  DriveMap=Version=0;
  ContainerID[0]=ContainerID[1]=-1;
  LockedSide[0]=LockedSide[1]=-1;
  LockedTrack[0]=LockedTrack[1]=-1; 
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  CyclesRun=0;
#endif
#if defined(SS_VAR_NOTIFY)
  SetNotifyInitText(SS_IPF_PLUGIN_FILE);
#endif

  CapsVersionInfo versioninfo;
  try {
    CAPSInit();
  }
  catch(...) {
    TRACE_LOG("CapsImg.DLL can't be loaded\n");
    return 0;
  }
  VERIFY( !CAPSGetVersionInfo((void*)&versioninfo,0) );
  ASSERT( !versioninfo.type );
  TRACE_LOG("Using CapsImg library V%d.%d\n",versioninfo.release,versioninfo.revision);
  Version=versioninfo.release*10+versioninfo.revision; 
  ASSERT( Version==42 );
  CAPSIMG_OK= (Version>0);
  // controller init
  WD1772.type=sizeof(CapsFdc);  // must be >=sizeof(CapsFdc)
  WD1772.model=cfdcmWD1772;
  WD1772.clockfrq=SS_IPF_FREQU; 
  WD1772.drive=SF314; // ain't it cool?
  WD1772.drivecnt=2;
  WD1772.drivemax=0;
  // drives
  SF314[0].type=SF314[1].type=sizeof(CapsDrive); // must be >=sizeof(CapsDrive)
  SF314[0].rpm=SF314[1].rpm=CAPSDRIVE_35DD_RPM;
  SF314[0].maxtrack=SF314[1].maxtrack=CAPSDRIVE_35DD_HST;

  int ec=CAPSFdcInit(&WD1772);
  if(ec!=imgeOk)
  {
    TRACE_LOG("CAPSFdcInit failure %d\n",ec);
    Version=0;
    return 0;
  }

  // the DLL will call them, strange that they're erased at FDC init:
  WD1772.cbdrq=CallbackDRQ;
  WD1772.cbirq=CallbackIRQ;
  WD1772.cbtrk=CallbackTRK;

  // we already create our 2 IPF drives, instead of waiting for an image:
  ContainerID[0]=CAPSAddImage();
  ContainerID[1]=CAPSAddImage();
  ASSERT( ContainerID[0]!=-1 && ContainerID[1]!=-1 );
  WD1772.drivemax=2;
  WD1772.drivecnt=2;

  return Version;
}


void TCaps::Reset() {
    CAPSFdcReset(&WD1772);
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IMAGE_INFO//SS

int TCaps::InsertDisk(int drive,char* File,CapsImageInfo *img_info) {

  ASSERT( CAPSIMG_OK );
  if(!CAPSIMG_OK)
    return -1;

  ASSERT( !drive || drive==1 );
  ASSERT( img_info );
  ASSERT( ContainerID[drive]!=-1 );
  ASSERT( ContainerID[drive]!=-1 );
  DriveMap|=(drive+1); // not <<!
  bool FileIsReadOnly=bool(GetFileAttributes(File) & FILE_ATTRIBUTE_READONLY);
  VERIFY( !CAPSLockImage(ContainerID[drive],File) ); // open the IPF file
  VERIFY( !CAPSGetImageInfo(img_info,ContainerID[drive]) );
  ASSERT( img_info->type==ciitFDD );
  TRACE_LOG("Disk in %c is CAPS release %d rev %d of %d/%d/%d for ",
    drive+'A',img_info->release,img_info->revision,img_info->crdt.day,
    img_info->crdt.month,img_info->crdt.year);
  bool found=0;
  for(int i=0;i<CAPS_MAXPLATFORM;i++)
  {
    if((img_info->platform[i])!=ciipNA)
      TRACE_LOG("%s ",CAPSGetPlatformName(img_info->platform[i]));
    if(img_info->platform[i]==ciipAtariST || SSE_HACKS_ON) //MPS GOlf 'test'
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
  CAPSFdcInvalidateTrack(&WD1772,drive); // Galaxy Force II
  LockedTrack[drive]=LockedSide[drive]=-1;
  return 0;
}


void TCaps::RemoveDisk(int drive) {
  ASSERT( CAPSIMG_OK );
  if(!CAPSIMG_OK)
    return;
  TRACE_LOG("Drive %c removing image\n",drive+'A');
  VERIFY( !CAPSUnlockImage(Caps.ContainerID[drive]) ); // eject disk
  SF314[drive].diskattr&=~CAPSDRIVE_DA_IN;
  DriveMap&=~(drive+1); // not <<!
  if(!IsIpf(!drive)) // other drive
    Active=FALSE; 
}


#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

void TCaps::WritePsgA(int data) {
  // drive selection 
  if ((psg_reg[PSGR_PORT_A]&BIT_1)==0 /*&& DriveIsIPF[0]*/)
    WD1772.drivenew=0;
  else if ((psg_reg[PSGR_PORT_A]&BIT_2)==0 /*&& DriveIsIPF[1]*/)
    WD1772.drivenew=1;
//TODO: no drive...
  else //if(DriveIsIPF[0])
    WD1772.drivenew=-2; //0;  //?  //TESTING
  // side selection (Burger Man, Turbo Outrun...)
//  ASSERT( !WD1772.drivenew || WD1772.drivenew==1 );
  if(!WD1772.drivenew || WD1772.drivenew==1)
    SF314[WD1772.drivenew].newside=((psg_reg[PSGR_PORT_A]&BIT_0)==0);
}


UDWORD TCaps::ReadWD1772(BYTE Line) {

#if defined(SS_DEBUG)
  Dma.UpdateRegs();
#endif

#if defined(SS_IPF_RUN_PRE_IO)
  CAPSFdcEmulate(&WD1772,CYCLES_PRE_IO);
  CyclesRun+=CYCLES_PRE_IO;
#endif

  UDWORD data=CAPSFdcRead(&WD1772,Line); 
  if(!Line) // read status register
  {
    mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
    WD1772.lineout&=~CAPSFDC_LO_INTRQ; // and in the WD1772
    int drive=floppy_current_drive(); //TODO code duplication
    if(floppy_mediach[drive])
    {
      if(floppy_mediach[drive]/10!=1) 
        data|=FDC_STR_WRITE_PROTECT;
      else
        data&=~FDC_STR_WRITE_PROTECT;
      TRACE_LOG("FDC SR mediach %d WP %x\n",floppy_mediach[drive],data&FDC_STR_WRITE_PROTECT);
    }
  }

#if defined(SS_IPF_RUN_POST_IO)
  CAPSFdcEmulate(&WD1772,CYCLES_POST_IO);
  CyclesRun+=CYCLES_POST_IO;
#endif
  ASSERT(!(data&0xFF00));
  return data;
}


void TCaps::WriteWD1772(BYTE Line,int data) {

  Dma.UpdateRegs();

#if defined(SS_IPF_RUN_PRE_IO)
  CAPSFdcEmulate(&WD1772,CYCLES_PRE_IO);
  CyclesRun+=CYCLES_PRE_IO;
#endif  

#if defined(SS_FDC) && defined(SS_IPF_LETHAL_XCESS)
  if(SSE_HACKS_ON && !Line && Version==42) 
  { //targeted hack to make up for something missing in capsimg (?)
    if( (::WD1772.CR&0xF0)==0xD0 && (data&0xF0)==0xE0 ) 
    {
      TRACE_LOG("hack Lethal Xcess\n");
      WD1772.lineout&=~CAPSFDC_LO_INTIP; // fixes Lethal Xcess
      // in current beta, this doesn't work anymore?
    }
  }
#endif

  if(!Line) // command
  {
    // TODO drive selection problem
    mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Double Dragon II
    if(Version==42 && (data&0xE0)==0xA0)
    {
      TRACE_LOG("IPF unimplemented command",data);

#if defined(SS_IPF_WRITE_HACK)
      if(SSE_HACKS_ON && (data&0xE0)==0xA0) // =write sector (not track!)
      {
        TRACE_LOG("Hack for write to IPF %d sectors\n",Dma.Counter);
        Dma.BaseAddress+=512*Dma.Counter;
#if defined(SS_DMA_COUNT_CYCLES)
        INSTRUCTION_TIME(256*Dma.Counter);
#endif
        Dma.Counter=0;
        Dma.SR=1;
      }
#endif
    }
  }

  CAPSFdcWrite(&WD1772,Line,data); // send to DLL

#if defined(SS_IPF_RUN_POST_IO)
  CAPSFdcEmulate(&WD1772,CYCLES_POST_IO);
  CyclesRun+=CYCLES_POST_IO;
#endif
}


void TCaps::Hbl() {

  // we run cycles at each HBL if there's an IPF file in. Performance OK
#if defined(STEVEN_SEAGAL) && defined(SS_SHIFTER)
  ASSERT( Shifter.CurrentScanline.Cycles>100)
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  ASSERT( Shifter.CurrentScanline.Cycles-Caps.CyclesRun>0 );
#endif
  CAPSFdcEmulate(&WD1772,Shifter.CurrentScanline.Cycles
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
    -CyclesRun
#endif
    );
#else
  CAPSFdcEmulate(&WD1772,screen_res==2? 224 : 512);
#endif

#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  CyclesRun=0;
#endif

}

int TCaps::IsIpf(int drive) {
  ASSERT(!drive||drive==1);
  return (DriveMap&(drive+1)); // not <<!
}


/*  Callback functions. Since they're static, they access object data like
    any external function, using 'Caps.'
*/

void TCaps::CallbackDRQ(PCAPSFDC pc, UDWORD setting) {
#if defined(SS_DEBUG) || !defined(VC_BUILD)
  Dma.UpdateRegs();
  ASSERT( Dma.MCR&BIT_7 ); // DMA enabled
  ASSERT(!(Dma.MCR&BIT_6)); // DMA enabled
  ASSERT(!(Dma.MCR&BIT_3)); // Floppy
#endif
  if((Dma.MCR&BIT_7) && !(Dma.MCR&BIT_6))
  {
    // transfer one byte
    if(!(Dma.MCR&BIT_8)) // disk->RAM
      Dma.AddToFifo( CAPSFdcGetInfo(cfdciR_Data,&Caps.WD1772,0) );
    else // RAM -> disk
      Caps.WD1772.r_data=Dma.GetFifoByte();  

    Caps.WD1772.r_st1&=~CAPSFDC_SR_IP_DRQ; // The Pawn
    Caps.WD1772.lineout&=~CAPSFDC_LO_DRQ;
  }
}


void TCaps::CallbackIRQ(PCAPSFDC pc, DWORD lineout) {
  ASSERT(pc==&Caps.WD1772);
#if defined(SS_DEBUG)
  if(TRACE_ENABLED) Dma.UpdateRegs(true);
#endif
  mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,!(lineout&CAPSFDC_LO_INTRQ));
#if !defined(SS_OSD_DRIVE_LED3)
  disk_light_off_time=timeGetTime()+DisableDiskLightAfter;
#endif
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IPF_LOCK_INFO

void TCaps::CallbackTRK(PCAPSFDC pc, UDWORD drive) {
  ASSERT( !drive||drive==1 );
  ASSERT( Caps.IsIpf(drive) );
  ASSERT( drive==floppy_current_drive() );
  CapsTrackInfoT2 track_info; // apparently we must use type 2...
  track_info.type=2; 
  UDWORD flags=DI_LOCK_DENALT|DI_LOCK_DENVAR|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;
  int side=Caps.SF314[drive].side;
  ASSERT( side==!(psg_reg[PSGR_PORT_A]& BIT_0) );
  int track=Caps.SF314[drive].track;
  if(Caps.LockedTrack[drive]!=-1)
  {
    ASSERT( Caps.LockedSide[drive]!=-1 );
    ASSERT( track!=Caps.LockedTrack[drive] || side!=Caps.LockedSide[drive] );
    VERIFY( !CAPSUnlockTrack(Caps.ContainerID[drive],Caps.LockedTrack[drive],
      Caps.LockedSide[drive]) );
    TRACE_LOG("CAPS Unlock %c:S%dT%d\n",drive+'A',Caps.LockedSide[drive],
      Caps.LockedTrack[drive]);
  }
  VERIFY( !CAPSLockTrack((PCAPSTRACKINFO)&track_info,Caps.ContainerID[drive],
    track,side,flags) );
  ASSERT( side==track_info.head );
  ASSERT( track==track_info.cylinder );
  ASSERT( !track_info.sectorsize );
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
    CAPSGetInfo(&CSI,Caps.ContainerID[drive],track,side,cgiitSector,sec_num-1);
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

#endif//ipf

#undef LOGSECTION

#endif//#if defined(STEVEN_SEAGAL)