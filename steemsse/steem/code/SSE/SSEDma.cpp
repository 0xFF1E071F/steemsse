#include "SSE.h"

#if defined(STEVEN_SEAGAL)
//some incl. useless?
#if defined(SS_STRUCTURE_SSEFLOPPY_OBJ)
#include "../pch.h"
#include <cpu.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include <run.decla.h>
#include "SSECpu.h"
#include "SSEInterrupt.h"
#include "SSEShifter.h"
#if defined(WIN32)
#include <pasti/pasti.h>
#endif
EasyStr GetEXEDir();//#include <mymisc.h>//missing...

#if defined(SS_DRIVE_IPF1)
#include <stemdialogs.h>//temp...
#include <diskman.decla.h>
#endif

#if !defined(SS_CPU)
#include <mfp.decla.h>
#endif

#endif//#if defined(SS_STRUCTURE_SSEFLOPPY_OBJ)

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"
#if defined(SS_DISK_GHOST)
#include "SSEGhostDisk.h"
#endif


#if defined(SS_DMA)

TDma::TDma() {
#if defined(SS_DMA_FIFO)
  Request=false;
#endif
#if defined(SS_DMA_DOUBLE_FIFO)
  BufferInUse=0;
#endif
}


#if defined(SS_DMA_FIFO)
/*  We try to emulate the DMA FIFO transfers in a more accurate way.
    In previous versions the bytes were directly moved between ST RAM
    and disk image. Now there's a real automatic buffer working both
    ways.
    SS_DMA_DOUBLE_FIFO has been tested but isn't defined, overkill, 
    so we spare some data bytes(17).
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

#endif


#if defined(SS_DMA_DRQ)//3.7.0 

/*  This just transfers between controller's DR and Fifo at request.
    Direction depends on control register
*/
void TDma::Drq() {

  if(MCR&BIT_7) // the DRQ from the FDC is acknowledged
  {
    if(!(Dma.MCR&BIT_8)) // disk->RAM (reading disk)
    {
      AddToFifo(WD1772.DR);
    }
    else // // RAM -> disk (writing to disk)
    {
      WD1772.DR=GetFifoByte();
    }
  }
  else // the DRQ from the HDC is acknowledged
  {
  }
}

#endif


#if defined(SS_DMA_DELAY) // normally not //370 -> replace with generic event?
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


#if defined(SS_DMA_IO) 
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
    {
#if defined(SS_DMA_SECTOR_COUNT)
/*
"The sector count register is write only. Reading this register return
 unpredictable values."
*/
      ior_byte=(rand()&0xFF); // or FF?
#else
      ior_byte=HIBYTE(Counter); 
#endif
      //TRACE("read %x as %x\n",addr,ior_byte);
    }
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
      //TRACE("read %x as %x\n",addr,ior_byte);
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
      //TRACE("read %x as %x\n",addr,ior_byte);
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
    //if(!(dma_mode & (BIT_1+BIT_2))) TRACE("read STR pre pasti as %X\n",ior_byte);
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
    //TRACE("read %x as %x\n",addr,ior_byte);
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
   //TRACE("read %x as %x\n",addr,ior_byte);
//    if(addr==0xff860d) TRACE("base %x ",BaseAddress);
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


//TRACE("pre pasti read %x as %x\n",addr,ior_byte);

////#define LOGSECTION LOGSECTION_FDC//tmp
#if USE_PASTI 
/*  Pasti handles all Dma reads - this cancels the first value
    of ior_byte, but allows to go through TRACE and update our variables..
*/
  if(hPasti && pasti_active
#if defined(SS_PASTI_ONLY_STX)
    && (!PASTI_JUST_STX || 
#if defined(SS_DISK_IMAGETYPE)
// in fact we should refactor this
    SF314[floppy_current_drive()].ImageType.Extension==EXT_STX
#else
    SF314[floppy_current_drive()].ImageType==DISK_PASTI
#endif
#if defined(SS_PASTI_ONLY_STX_HD)
    || (MCR&BIT_3) // hard disk handling by pasti
#endif
    )
#endif        
#if defined(SS_DISK_GHOST_SECTOR_STX1)
     &&! (WD1772.CommandWasIntercepted)
#endif
    )
  {
    TRACE_LOG(" Pasti");
//    TRACE("pasti reading ");
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
//  if(addr==0xff860d) TRACE("base %x ",BaseAddress);
  //if(addr==0xff860b) TRACE("read %x as %x\n",addr,ior_byte);
  //TRACE("post pasti read %x as %x\n",addr,ior_byte);

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
//    ASSERT(!(io_src_b&1)); // Omega, Union
//    ASSERT(!(io_src_b&BIT_5)); // Do Things
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
#if defined(SS_DMA_ADDRESS)
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
#if defined(SS_DMA_ADDRESS)
    //BaseAddress=0xffff00,BaseAddress|=io_src_b;
    BaseAddress=io_src_b;
#else
    BaseAddress&=0xffff00;
    BaseAddress|=io_src_b;
#endif   
#if defined(SS_DMA_ADDRESS_EVEN)    
    BaseAddress&=0xfffffe;//remark by Petari: bit0 ignored
#endif
    //TRACE("W BaseAddress %x\n",BaseAddress);
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
    
#if defined(SS_DRIVE)&&defined(SS_PASTI_ONLY_STX)
    && (!PASTI_JUST_STX 
#if defined(SS_DISK_IMAGETYPE)
    || SF314[floppy_current_drive()].ImageType.Extension==EXT_STX
#else
    || SF314[floppy_current_drive()].ImageType==DISK_PASTI
#endif
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
//      TRACE("Pasti: %X->%X\n",data,addr);

#if defined(SS_DISK_GHOST_SECTOR_STX1)
      if(SSE_GHOST_DISK && WD1772.CommandWasIntercepted
        && addr==0xff8604 && !(MCR&(BIT_1+BIT_2+BIT_3)) // FDC commands
        )
      {
        TRACE_LOG("Pasti doesn't get command %X\n",fdc_cr);
      }
      else
#endif
      {
        pasti->Io(PASTI_IOWRITE,&pioi); //SS send to DLL
        pasti_handle_return(&pioi);
      }
    }
  }
#endif

  TRACE_LOG(" =.B %X\n",io_src_b);

}

#endif//dmaio

#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

void TDma::UpdateRegs(bool trace_them) {
/*  This is used for debugging and for pasti drive led
    and may have some other uses.
    We update both DMA and FDC registers (so we use old variable names for the
    latter) - we could have a TWD1772::UpdateRegs() as well.
*/

#if USE_PASTI
  if(hPasti && pasti_active
#if defined(SS_PASTI_ONLY_STX) //all or nothing?
    && (!PASTI_JUST_STX || 
#if defined(SS_DISK_IMAGETYPE)
// in fact we should refactor this
    SF314[floppy_current_drive()].ImageType.Extension==EXT_STX
#else
    SF314[floppy_current_drive()].ImageType==DISK_PASTI
#endif
#if defined(SS_PASTI_ONLY_STX_HD)
    || (MCR&BIT_3) // hard disk handling by pasti
#endif
    )
#endif//defined(SS_PASTI_ONLY_STX)
#if defined(SS_DISK_GHOST_SECTOR_STX1)
    // regs should be alright since last update (?)
    &&!(SSE_GHOST_DISK&&WD1772.CommandWasIntercepted)
#endif
    )
  {
   // TRACE("pasti regs ");

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
#if defined(SS_DRIVE_MOTOR_ON_IPF)
/*  when disk A is non IPF and disk B is IPF, we may get bogus
    motor on, how to fix that? - rare anyway
    TODO
*/
    if(!(fdc_str&FDC_STR_MOTOR_ON)) // assume this drive!
      ::SF314[DRIVE].MotorOn=false;
#endif
    fdc_tr=CAPSFdcGetInfo(cfdciR_Track, &Caps.WD1772,ext);
    fdc_sr=CAPSFdcGetInfo(cfdciR_Sector, &Caps.WD1772,ext);
    fdc_dr=CAPSFdcGetInfo(cfdciR_Data, &Caps.WD1772,ext);
    floppy_head_track[0]=Caps.SF314[0].track;
    floppy_head_track[1]=Caps.SF314[1].track;
  } 
#endif

#if defined(SS_FDC_TRACE_IRQ)
  if(trace_them)
  {
    if(MCR&BIT_3)
      TRACE_LOG("HDC IRQ HBL %d ",hbl_count);
    else
    {
      TRACE_LOG("FDC IRQ CR %X STR %X ",fdc_cr,fdc_str);
#if defined(SS_FDC_TRACE_STATUS)
      WD1772.TraceStatus();
#endif
      TRACE_LOG("TR %d SR %d DR %d ($%X) HBL %d",fdc_tr,fdc_sr,fdc_dr,fdc_dr,hbl_count);
    }
#if defined(SS_PSG1)//
    TRACE_LOG(" %c%d:",'A'+YM2149.SelectedDrive,YM2149.SelectedSide);
#endif
    TRACE_LOG(" T %d/%d DMA CR %X $%X SR %X #%d PC %X\n",
      floppy_head_track[0],floppy_head_track[1],MCR,BaseAddress,SR,Counter,pc);
  }
#endif

#ifdef SS_DEBUG//no mask yet but important info
  if((fdc_str&0x10) && WD1772.CommandType(fdc_cr)!=1)
    TRACE_OSD("RNF");
#endif

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

#endif


#if defined(SS_DMA_FIFO)

void TDma::RequestTransfer() {
  // we make this function to avoid code duplication
  Request=true; 
  
  Fifo_idx= (MCR&0x100) ? 16 : 0;

#if defined(SS_DMA_DOUBLE_FIFO)
  BufferInUse=!BufferInUse; // toggle 16byte buffer
#endif

#if defined(SS_DMA_DELAY) // normally not - set event
  TransferTime=ABSOLUTE_CPU_TIME+SS_DMA_ACCESS_DELAY;
  PREPARE_EVENT_CHECK_FOR_DMA
#else
  TransferBytes(); // direct transfer
#endif
}

#endif

#if defined(SS_DMA_FIFO)

void TDma::TransferBytes() {
  // execute the DMA transfer (assume floppy)
  ASSERT( MCR&0x80 ); // bit 7 or no transfer
  ASSERT(!(MCR&BIT_5));
  ASSERT( Request );
  ASSERT( MCR==0x80 || MCR==0x90 || MCR==0x180 || MCR==0x190 );

#if defined(SS_DRIVE_COMPUTE_BOOT_CHECKSUM)
/*  Computing the checksum if it's bootsector.
    We do it here in DMA because it should work with naitve, STX, IPF.
*/

#define LOGSECTION LOGSECTION_IMAGE_INFO

  if(fdc_cr==0x80 && !fdc_tr && fdc_sr==1 && !(MCR&0x100)
    &&!floppy_current_side())//3.6.1: side 0
  {
    for(int i=0;i<16;i+=2)
    {
      SF314[floppy_current_drive()].SectorChecksum+=Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
        [!BufferInUse]
#endif
        [i]<<8; 
      SF314[floppy_current_drive()].SectorChecksum+=Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
        [!BufferInUse]
#endif
        [i+1]; 
    }
    //ASSERT( SF314[floppy_current_drive()].SectorChecksum!=0x1234 );
    //TRACE_LOG("Boot sector of %c checksum %X\n",'A'+floppy_current_drive(),SF314[floppy_current_drive()].SectorChecksum);
  }

#endif//checksum

#undef LOGSECTION
#if defined(SS_DEBUG_TRACE_CONTROL)
#define LOGSECTION LOGSECTION_ALWAYS // for just the bytes 
#else
#define LOGSECTION LOGSECTION_FDC_BYTES 
#endif

#if defined(SS_DEBUG_TRACE_CONTROL)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
#endif
  {
#if defined(SS_DMA_TRACK_TRANSFER)
    Datachunk++; 
    if(!(MCR&0x100)) // disk -> RAM
      TRACE_LOG("#%03d (%d-%02d-%02d) to %06X: ",Datachunk,floppy_current_side(),floppy_head_track[DRIVE],fdc_sr,BaseAddress);
    else  // RAM -> disk
      TRACE_LOG("#%03d (%d-%02d-%02d) from %06X: ",Datachunk,floppy_current_side(),floppy_head_track[DRIVE],fdc_sr,BaseAddress);
#else
#endif
  }

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
    else 
#if defined(SS_DEBUG_TRACE_CONTROL)
      if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
#endif
          TRACE_LOG("!!!");
#if defined(SS_DEBUG_TRACE_CONTROL)
    if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
#endif
      TRACE_LOG("%02X ",Fifo
#if defined(SS_DMA_DOUBLE_FIFO)
      [!BufferInUse]
#endif
      [(MCR&0x100)?15-i:i]);//bugfix 3.6.1 reverse order

#if USE_PASTI    
    if(hPasti&&pasti_active
#if defined(SS_PASTI_ONLY_STX)
    &&(!PASTI_JUST_STX || 
#if defined(SS_DISK_IMAGETYPE)
// in fact we should refactor this
    SF314[floppy_current_drive()].ImageType.Extension==EXT_STX)
#else
    SF314[floppy_current_drive()].ImageType==DISK_PASTI)
#endif    
#if defined(SS_PASTI_ONLY_STX_HD)
    || (MCR&BIT_3) // hard disk handling by pasti
#endif
#endif        
      )  
      dma_address++;
    else
#endif
      DMA_INC_ADDRESS; // use Steem's existing routine (?)
  }
#if defined(SS_DMA_COUNT_CYCLES) 
  if(ADAT)//3.6.1 condition
    INSTRUCTION_TIME(8); 
#endif
#if defined(SS_DEBUG_TRACE_CONTROL)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
#endif
    TRACE_LOG("\n");
  Request=FALSE;
}

#undef LOGSECTION

#endif//DMA_FIFO

#endif//dma

#endif//#if defined(STEVEN_SEAGAL)
