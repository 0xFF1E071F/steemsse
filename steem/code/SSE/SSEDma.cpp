/*  The DMA (Direct Memory Access) chip was developed by Atari.
    It uses two 16-byte FIFO buffers to handle byte transfers between the 
    drive  controller (floppy or hard disk) and memory so that the CPU is
    freed from the task.
    Transfers occur at the end of the scanline, when the MMU/Shifter
    frees the bus.
    We don't count cycles for this, CPU isn't working ;)
*/
#include "SSE.h"

#if defined(SSE_DMA)

#include "../pch.h"
#include <cpu.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <run.decla.h> // for ACT
#include "SSECpu.h"
#if defined(WIN32)
#include <pasti/pasti.h>
#endif
#if !defined(SSE_CPU)
#include <mfp.decla.h>
#endif
#if defined(SSE_TOS_GEMDOS_DRVBIT)
#include <stemdos.decla.h>
#endif

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"
#if defined(SSE_DISK_GHOST)
#include "SSEGhostDisk.h"
#endif

#ifdef SSE_ACSI
#include "SSEAcsi.h"
#include <harddiskman.decla.h>
#endif

#if defined(SSE_DMA_OBJECT)

#if defined(SSE_BOILER_383_LOG2)
#define LOGSECTION LOGSECTION_DMA
#endif


TDma::TDma() {
#if defined(SSE_DMA_FIFO)
  Request=false;
#endif
#if defined(SSE_DMA_DOUBLE_FIFO)
  BufferInUse=0;
#endif
}


#if defined(SSE_DMA_FIFO)
/*  We try to emulate the DMA FIFO transfers in a more accurate way.
    In previous versions the bytes were directly moved between ST RAM
    and disk image. Now there's a real automatic buffer working both
    ways.
*/

void TDma::AddToFifo(BYTE data) {
  // DISK -> FIFO -> RAM = 'read disk' = 'write dma'

  ASSERT( !(MCR&CR_WRITE) ); // disk->RAM (Read media)
  ASSERT( Fifo_idx<16 );
  Fifo
#if defined(SSE_DMA_DOUBLE_FIFO)
    [BufferInUse]
#endif
    [Fifo_idx++]=data;

  if(Fifo_idx==16)
    Dma.RequestTransfer();
}

#endif


#if defined(SSE_DMA_DRQ)//3.7.0 

/*  This just transfers one byte between controller's DR and Fifo at request.
    Direction depends on control register, function checks it itself.
    Note that bits 6 (CR_DISABLE) and 7 (CR_DRQ_FDC_OR_HDC) don't count,
    DMA transfer will happen whatever their value.
    This is observed for example in Kick Off 2 (IPF).
    That's pretty strange when you consider that the DMA chip was designed and
    documented by Atari, but then one needs to consider development speed (aka
    the rush to market).
*/

bool TDma::Drq() {

  SR|=SR_DRQ;
  
  if(MCR&CR_WRITE) // RAM -> disk (writing to disk)
  {
    if(!(MCR&CR_HDC_OR_FDC)) //floppy select
      WD1772.DR=GetFifoByte();
#if defined(SSE_ACSI)
    else if(ACSI_EMU_ON)
      AcsiHdc[acsi_dev].DR=GetFifoByte();
#endif
#if defined(SSE_DMA_DRQ_RND)
    else // hd
      GetFifoByte(); //TODO: put it on HD DR?
#endif
  }
  else // disk->RAM (reading disk)
  {
#if defined(SSE_DMA_FIFO_NATIVE2)
   if(!Counter)
     ; // ignore
   else
#endif
    if(!(MCR&CR_HDC_OR_FDC))
      AddToFifo(WD1772.DR);
#if defined(SSE_ACSI)
    else if(ACSI_EMU_ON)
      AddToFifo(AcsiHdc[acsi_dev].DR);
#endif
#if defined(SSE_DMA_DRQ_RND)
    else
      AddToFifo( (BYTE)rand() ); //TODO take HD DR?
#endif
  }
  SR&=~SR_DRQ;
  return true;
}

#endif


#if defined(SSE_DMA_DELAY) // normally not 
// this is a Steem 'event' (static member function)

void TDma::Event() {
#if defined(SSE_INT_MFP_RATIO)
  Dma.TransferTime=ABSOLUTE_CPU_TIME+CpuNormalHz;
#else
  Dma.TransferTime=ABSOLUTE_CPU_TIME+8000000;
#endif
  if(Dma.Request)
    Dma.TransferBytes();
}

#endif


#if defined(SSE_DMA_FIFO)
/*  For writing to disk, we use the FIFO in the other direction,
    because we want it to fill up when empty.
    On a real ST, the first DMA transfer happens right when the
    sector count has been set.
    This could be important in theory, but know no case, to simplify
    we only request byte when drive is ready.
*/

BYTE TDma::GetFifoByte() {
  // RAM -> FIFO -> DISK = 'write disk' = 'read dma'
  
  ASSERT( (MCR&CR_WRITE) ); // RAM -> disk (Write floppy)
  ASSERT(!(MCR&CR_DISABLE));

  if(!Fifo_idx)
    Dma.RequestTransfer();

  BYTE data=Fifo
#if defined(SSE_DMA_DOUBLE_FIFO)
    [BufferInUse]
#endif
    [--Fifo_idx];

  return data;  

}

#endif


#if defined(SSE_DMA_IO) 
/*  Read/write on disk DMA registers.
*/

#if !defined(SSE_BOILER_383_LOG2)
#define LOGSECTION LOGSECTION_IO
#endif

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
  BYTE drive=DRIVE;//YM2149.SelectedDrive;

//  TRACE_LOG("DMA R %X ",addr);

  // test for bus error
  if(addr>0xff860f || addr<0xff8604 || addr<0xff8608 && !io_word_access) 
    exception(BOMBS_BUS_ERROR,EA_READ,addr);

  switch(addr)
  {

  case 0xff8604:
    // sector counter

    //ASSERT( !(MCR&CR_COUNT_OR_REGS) ); 
    if(MCR&CR_COUNT_OR_REGS) 
    {
#if defined(SSE_DMA_SECTOR_COUNT)
/*
"The sector count register is write only. Reading this register return
 unpredictable values."
*/
#if defined(SSE_DMA_SECTOR_COUNT2)
//      TRACE_FDC("Read DMA sector counter\n");
      ior_byte=0xFF;
#else
      ior_byte=(rand()&0xFF); // or FF?
#endif
#else
      ior_byte=HIBYTE(Counter); 
#endif
      //TRACE("read %x as %x\n",addr,ior_byte);
    }
    // HD access
    else if(MCR&CR_HDC_OR_FDC) 
    {
#ifdef SSE_CPU
      LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
        " - Reading high byte of HDC register #"+((MCR & BIT_1) ? 1:0)); )
#endif
    }
    // high byte of FDC
/*
"The FDC registers only uses 8 bits when writing and therefore the upper byte 
is ignored and when reading the 8 upper bits consistently reads 1."
*/
    break;

  case 0xff8605: 
    // sector counter
    if(MCR&CR_COUNT_OR_REGS) 
    {
//      TRACE_FDC("Read DMA sector counter\n");
#if defined(SSE_DMA_SECTOR_COUNT)
#if defined(SSE_DMA_SECTOR_COUNT2)
      ior_byte=0xFF;
#else
      ior_byte=(rand()&0xFF);
#endif
#else
      ior_byte=LOBYTE(Counter); 
#endif
      //TRACE("read %x as %x\n",addr,ior_byte);
    }
    // HD access
    else if(MCR&CR_HDC_OR_FDC) 
    {
//      TRACE_LOG("HD");
      LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
                  " - Reading low byte of HDC register #"+((dma_mode & BIT_1) ? 1:0)); )

#if defined(SSE_ACSI)
      ASSERT(MCR&CR_DRQ_FDC_OR_HDC);
      if(ACSI_EMU_ON)
#if defined(SSE_VS2008_WARNING_382)
        ior_byte=AcsiHdc[acsi_dev].IORead();
#else
        ior_byte=AcsiHdc[acsi_dev].IORead( (MCR&(CR_A1|CR_A0))/2 );
#endif
#endif


    }
    // Read FDC register
#if defined(SSE_DMA_FDC_ACCESS)
/*
"Bit 7:FDC/HDC transfers acknowledge; when set the DRQ from the Floppy 
Disk Controller is acknowledged; otherwise, the DRQ from the Hard Disk 
interface is acknowledged. For some reason this bit must also be set 
to generate DMA bus cycle."
note: this isn't clear, see Drq()
*/
    else if(!(MCR&CR_DRQ_FDC_OR_HDC)) // we can't
    {
      TRACE_LOG("No FDC access DMA MCR %x\n",MCR);
    }
#endif
    else
#if defined(SSE_WD1772)
      ior_byte=WD1772.IORead( (MCR&(CR_A1|CR_A0))/2 );
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
              //TRACE_FDC("dma MFP_GPIP_FDC_BIT: %d\n",true);
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
#endif//
    //if(!(dma_mode & (BIT_1+BIT_2))) TRACE("read STR pre pasti as %X\n",ior_byte);
    break;

  case 0xff8606:  //high byte of DMA status
    ior_byte=0; // only bits 0-2 of low byte are used
    break;

  case 0xff8607:  //low byte of DMA status
//    TRACE_LOG("SR");
#if defined(SSE_DMA_READ_STATUS)
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
//    TRACE_LOG("BaseAddress");
    ior_byte=(BYTE)((BaseAddress&0xff0000)>>16);
    break;

  case 0xff860b:  // DMA Base and Counter Mid
//    TRACE_LOG("BaseAddress");
    ior_byte=(BYTE)((BaseAddress&0xff00)>>8);
    break;

  case 0xff860d:  // DMA Base and Counter Low
//    TRACE_LOG("BaseAddress");
    ior_byte=(BYTE)((BaseAddress&0xff));
//   TRACE("read %x as %x\n",addr,ior_byte);
//    if(addr==0xff860d) TRACE("base %x ",BaseAddress);
    break;

  case 0xff860e: //frequency/density control
  {
//    TRACE_LOG("Density");
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
#if defined(SSE_DISK_PASTI_ONLY_STX)
    && (!PASTI_JUST_STX || 
    SF314[drive].ImageType.Extension==EXT_STX
#if defined(SSE_DISK_PASTI_ONLY_STX_HD)
    || (MCR&CR_HDC_OR_FDC) // hard disk handling by pasti
#endif
    )
#endif        
#if defined(SSE_DISK_GHOST_SECTOR_STX1)
     &&! (WD1772.Lines.CommandWasIntercepted)
#endif
    )
  {
//    TRACE_LOG(" Pasti");
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

//  TRACE_LOG(" .B=%X\n",ior_byte);
//  if(addr==0xff860d) TRACE("base %x ",BaseAddress);
  //if(addr==0xff860b) TRACE("read %x as %x\n",addr,ior_byte);
  //TRACE("post pasti read %x as %x\n",addr,ior_byte);

#if defined(SSE_DRIVE_MEDIACHANGE)
/*  Media change (changing the floppy disk) on the ST is managed in
    an intricate way, using a timed interrupt to check "write protect"
    status. A change in this status indicates that a disk is being moved
    before the diode in the drive that detects "write protect".
    This is handled in Steem but it must also be forwarded to other
    WD1772 emus.
    Of course we mess with this only if the media has been changed recently.
    eg 4 Wheel Drive/Combo Racer STX
*/
  if( floppy_mediach[drive] && 
    (SF314[drive].ImageType.Manager==MNGR_PASTI
    ||SF314[drive].ImageType.Manager==MNGR_CAPS)
    && addr==0xFF8605 && !(MCR&CR_COUNT_OR_REGS) && !(MCR & (CR_A0|CR_A1)) )
  {
    ior_byte&=~FDC_STR_WRITE_PROTECT;
    if(floppy_mediach[drive]/10!=1) 
      ior_byte|=FDC_STR_WRITE_PROTECT;
  }
#endif


#if defined(SSE_BOILER_TRACE_CONTROL) && !defined(SSE_BOILER_383_LOG2)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCDMA)
    TRACE_LOG("PC %X DMA R %X %X\n",old_pc,addr,ior_byte);
#endif

  return ior_byte;
}


void TDma::IOWrite(MEM_ADDRESS addr,BYTE io_src_b) {

  ASSERT( (addr&0xFFFF00)==0xFF8600 );

#if defined(SSE_BOILER_TRACE_CONTROL) && !defined(SSE_BOILER_383_LOG2)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCDMA)
    TRACE_FDC("PC %X DMA W %X %X\n",old_pc,addr,io_src_b);
#endif

  // test for bus error
  if(addr>0xff860f || addr<0xff8604 || addr<0xff8608 && !io_word_access) 
    exception(BOMBS_BUS_ERROR,EA_WRITE,addr);

  switch (addr)
  {

  case 0xff8604:
    
    if(MCR&CR_COUNT_OR_REGS)
    { 
      //write DMA sector counter, 0x190
      //ASSERT(!io_src_b); // it's an 8bit reg  //int. tennis
      //Counter&=0xff;
      //Counter|=int(io_src_b) << 8;
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
      break;
    }
    // HD access
    if(MCR&CR_HDC_OR_FDC)
    { 
    //  if(io_src_b) TRACE("HDC %x high: W %x\n",(dma_mode & BIT_1) ? 1:0,io_src_b);
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
    if (MCR&CR_COUNT_OR_REGS)
    { 
//      TRACE_LOG("Counter");
      Counter&=0xff00;
      Counter|=io_src_b;
      // We need do that only once (word access):
      if (Counter)
        SR|=SR_COUNT;
      else
        SR&=BYTE(~SR_COUNT); //status register bit for 0 count 
      ByteCount=0;
/*
"It is interesting to note that when the DMA is in write mode, the two internal
 FIFOS are filled immediately after the Count Register is written."
 RAM to disk
 -> to simplify emulation, we fill FIFO only at first DRQ (and each time
    it's empty)
*/
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Set DMA sector count to "+dma_sector_count);
      break;
    }
    // HD access
    if (MCR&CR_HDC_OR_FDC){ 
//      TRACE_LOG("HD");
 //     TRACE("HDC %x: W %x\n",(dma_mode & BIT_1) ? 1:0,io_src_b);
      log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+" - Writing $xx"+HEXSl(io_src_b,2)+" to HDC register #"+((dma_mode & BIT_1) ? 1:0));
#if defined(SSE_ACSI)
/*  According to defines, send byte to unique or "correct" ACSI device
    A1 in ACSI doc is A0 in DMA doc
*/
      ASSERT(MCR&CR_DRQ_FDC_OR_HDC);
      if(ACSI_EMU_ON)
      {
        int device=acsi_dev;
        if(!(MCR&CR_A0) &&  (io_src_b>>5)<TAcsiHdc::MAX_ACSI_DEVICES) 
          device=(io_src_b>>5); // assume new command
        AcsiHdc[device].IOWrite((MCR&CR_A0),io_src_b);
      }
#endif

#if defined(SSE_TOS_GEMDOS_DRVBIT)
/*  This will put the correct value (in $4C2) when TOS tests for hard drives,
    not waiting for a call to BIOS function Drvmap() ($10)
*/
      if(OPTION_HACKS && !HardDiskMan.DisableHardDrives && !(MCR&CR_A0))
        stemdos_update_drvbits(); 
#endif

      break;
    }
    // Write FDC register
#if defined(SSE_DMA_FDC_ACCESS)
/*
"Bit 7:FDC/HDC transfers acknowledge; when set the DRQ from the Floppy 
Disk Controller is acknowledged; otherwise, the DRQ from the Hard Disk 
interface is acknowledged. For some reason this bit must also be set 
to generate DMA bus cycle."
*/
    ASSERT( MCR&CR_DRQ_FDC_OR_HDC );
    if(!(MCR&CR_DRQ_FDC_OR_HDC))
      break;
#endif

#if defined(SSE_WD1772)
    WD1772.IOWrite((MCR&(CR_A1|CR_A0))/2,io_src_b);
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
#if defined(SSE_DMA_WRITE_CONTROL)
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
    if( !(MCR&CR_WRITE) ^ !(io_src_b) ) // fixes Archipelagos IPF
    {
      TRACE_LOG("DMA Reset\n");
#if !defined(SSE_DMA_FIFO_READ_ADDRESS2)
      fdc_read_address_buffer_len=0;// this is only for command III read address
#endif
      Counter=0;
      ByteCount=0;
      SR=SR_NO_ERROR;
      Request=false;
      Fifo_idx=0;
#if defined(SSE_DMA_DOUBLE_FIFO) 
      BufferInUse=0;
#endif
    }
    MCR&=0x00ff;
    MCR|=WORD(WORD(io_src_b) << 8);
#else
    MCR&=0x00ff;
    MCR|=WORD(WORD(io_src_b) << 8);
#if !defined(SSE_DMA_FIFO_READ_ADDRESS2)
    fdc_read_address_buffer_len=0;
#endif
    ByteCount=0;
#endif
    break;
    
  case 0xff8607:  //low byte of DMA mode
//    TRACE_LOG("CR");
    MCR&=0xff00;
    MCR|=io_src_b;
#if !defined(SSE_DMA_WRITE_CONTROL) // see above
#if !defined(SSE_DMA_FIFO_READ_ADDRESS2)
    fdc_read_address_buffer_len=0;
#endif
    ByteCount=0;
#endif
    break;
    
  case 0xff8609:  // DMA Base and Counter High
//    TRACE_LOG("BaseAddress");
    BaseAddress&=0x00ffff;
    BaseAddress|=((MEM_ADDRESS)io_src_b) << 16;
    log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Set DMA address to "+HEXSl(BaseAddress,6)); 
    TRACE_LOG("DMA base address: %X\n",BaseAddress);
    break;
    
  case 0xff860b:  // DMA Base and Counter Mid
//    TRACE_LOG("BaseAddress");


#if defined(SSE_DMA_RIPPLE_CARRY)
/*
DMA pointer has to be initialized in order low, mid, high, why
Short answer. Writing to a lower byte might increase (not clear) the upper one. 
This will happen when bit 7 (uppermost bit) of the written byte is changed from one to zero.
The reason is how exactly the counters are implemented on MMU. They use a ripple carry.
This saves quite some logic at the cost of not being fully synchronous. 
The upper bit of the lowermost byte, inverted, clocks directly the middle byte of 
the counter. Any change from high to low on that bit, anytime, would clock the counter 
on the middle byte. Similarly, the inverted uppermost bit of the middle byte clocks the 
high byte of the counter.

Actually, the ripple carry is not only across bytes, but across nibbles as well. 
But between nibbles doesn't have that collateral effect because both nibbles are written 
at the same time, and the written value overrides the ripple carry.

In case you are wondering, the video counter uses the same ripple mechanism. But there 
is no such effect because they are read only on the ST.
(ijor)
*/
    if(ST_TYPE!=STE && (BaseAddress&0x008000) && !(io_src_b&0x80)) // 1 to 0
    {
      BYTE new_byte=(BYTE)(dma_address>>16)+1;
      IOWrite(addr-2,new_byte); // maybe it works...
    }
#endif
    BaseAddress&=0xff00ff;
    BaseAddress|=((MEM_ADDRESS)io_src_b) << 8;
    break;
    
  case 0xff860d:  // DMA Base and Counter Low
//    TRACE_LOG("BaseAddress L");
    ASSERT( !(io_src_b&1) ); // shouldn't the address be even?
#if defined(SSE_DMA_RIPPLE_CARRY)
    if(ST_TYPE!=STE && (BaseAddress&0x000080) && !(io_src_b&0x80)) // 1 to 0
    {
      BYTE new_byte=(BYTE)(dma_address>>8)+1;
      IOWrite(addr-2,new_byte);
    }
#endif
    BaseAddress&=0xffff00;
    BaseAddress|=io_src_b;
#if defined(SSE_DMA_ADDRESS_EVEN)    
    //ASSERT(!(io_src_b&1));
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
    
#if defined(SSE_DRIVE_OBJECT)&&defined(SSE_DISK_PASTI_ONLY_STX)
    && (!PASTI_JUST_STX 
    || SF314[YM2149.SelectedDrive].ImageType.Extension==EXT_STX
    ||addr!=0xff8605 || (MCR&CR_HDC_OR_FDC))
#endif        
    )
  {

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

#if defined(SSE_DISK_GHOST_SECTOR_STX1)
      if(SSE_GHOST_DISK && WD1772.Lines.CommandWasIntercepted
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

//  TRACE_LOG(" =.B %X\n",io_src_b);

}

#endif//dmaio
#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

#if defined(SSE_VS2008_WARNING_383) && !defined(SSE_DEBUG)
void TDma::UpdateRegs() {
#else
void TDma::UpdateRegs(bool trace_them) {
#endif
/*  This is used for debugging and for pasti drive led
    and may have some other uses.
    We update both DMA and FDC registers (so we use old variable names for the
    latter) - we could have a TWD1772::UpdateRegs() as well.
*/

#if USE_PASTI
  if(hPasti && pasti_active
#if defined(SSE_DISK_PASTI_ONLY_STX) //all or nothing?
    && (!PASTI_JUST_STX || 
    SF314[floppy_current_drive()].ImageType.Extension==EXT_STX
#if defined(SSE_DISK_PASTI_ONLY_STX_HD)
    || (MCR&BIT_3) // hard disk handling by pasti
#endif
    )
#endif//defined(SSE_DISK_PASTI_ONLY_STX)
#if defined(SSE_DISK_GHOST_SECTOR_STX1)
    // regs should be alright since last update (?)
    &&!(SSE_GHOST_DISK&&WD1772.Lines.CommandWasIntercepted)
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
#if defined(SSE_DISK_CAPS)
  if(CAPSIMG_OK && SF314[DRIVE].ImageType.Manager==MNGR_CAPS)
  {
    int ext=0;
    fdc_cr=CAPSFdcGetInfo(cfdciR_Command, &Caps.WD1772,ext);
    fdc_str=CAPSFdcGetInfo(cfdciR_ST, &Caps.WD1772,ext);
#if defined(SSE_DRIVE_MOTOR_ON_IPF)
/*  when disk A is non IPF and disk B is IPF, we may get bogus
    motor on, how to fix that? - rare anyway
    TODO
*/
    if(!(fdc_str&FDC_STR_MOTOR_ON)) // assume this drive!
      ::SF314[DRIVE].State.motor=false;
#endif
    fdc_tr=CAPSFdcGetInfo(cfdciR_Track, &Caps.WD1772,ext);
    fdc_sr=CAPSFdcGetInfo(cfdciR_Sector, &Caps.WD1772,ext);
    fdc_dr=CAPSFdcGetInfo(cfdciR_Data, &Caps.WD1772,ext);
    floppy_head_track[0]=Caps.SF314[0].track;
    floppy_head_track[1]=Caps.SF314[1].track;
  } 
#endif

#if defined(SSE_DEBUG_FDC_TRACE_IRQ)
  if(trace_them)
  {
    //ASSERT(fdc_str);
    if(MCR&CR_HDC_OR_FDC)
      TRACE_HDC("HDC IRQ\n");
    else
    {
      ASSERT(fdc_str);
      TRACE_FDC("%d FDC(%d) IRQ CR %X STR %X ",ACT,SF314[DRIVE].ImageType.Manager,fdc_cr,fdc_str);
#if defined(SSE_DEBUG_FDC_TRACE_STATUS)
      WD1772.TraceStatus();
#endif
#if defined(SSE_BOILER_383_LOG2)
      TRACE_FDC("TR %d (CYL %d) SR %d DR %d\n",fdc_tr,floppy_head_track[DRIVE],fdc_sr,fdc_dr);
#else
      TRACE_LOG("TR %d SR %d DR %d",fdc_tr,fdc_sr,fdc_dr);
#endif
//    }
#if defined(SSE_YM2149A)//?
 //   TRACE_FDC(" %c%d:",'A'+YM2149.SelectedDrive,YM2149.SelectedSide);
#endif
#if !defined(SSE_BOILER_383_LOG2)
    TRACE_LOG(" CYL %d byte %d DMA CR %X $%X #%d\n",
      floppy_head_track[DRIVE],SF314[DRIVE].BytePosition(),MCR,BaseAddress,Counter);
#endif
    }
  }
#endif

#if defined(SSE_OSD_CONTROL) //finally done this
  if(OSD_MASK1 & OSD_CONTROL_FDC)
  {
  if((fdc_str&0x10))
    TRACE_OSD("RNF"); 
  else if((fdc_str&0x08)) // one or the other, we don't combine those traces
    TRACE_OSD("CRC"); 
  }
#endif

}


#undef LOGSECTION


#if defined(SSE_STRUCTURE_DMA_INC_ADDRESS)
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
      SR|=SR_COUNT;  // DMA sector count not 0
      if(!(Counter&0xFF)) 
        SR&=~SR_COUNT;     
    }                                                      
  }
}
#endif


#if defined(SSE_DMA_FIFO)

void TDma::RequestTransfer() {
  // we make this function to avoid code duplication
  Request=true; 
  
  Fifo_idx= (MCR&CR_WRITE) ? 16 : 0;

#if defined(SSE_DMA_DOUBLE_FIFO)
  BufferInUse=!BufferInUse; // toggle 16byte buffer
#endif

#if defined(SSE_DMA_DELAY) // normally not - set event
  // in fact we should taret linecycle "end DE"
  TransferTime=ABSOLUTE_CPU_TIME+SSE_DMA_ACCESS_DELAY; 
  PREPARE_EVENT_CHECK_FOR_DMA
#else
  TransferBytes(); // direct transfer
#endif
}

#endif

#if defined(SSE_DMA_FIFO)

void TDma::TransferBytes() {
  // execute the DMA transfer (assume floppy)
  ASSERT( Request );
 // ASSERT( MCR&CR_DRQ_FDC_OR_HDC ); // bit 7 or no transfer ???->no
  ASSERT(!(MCR&CR_RESERVED)); // reserved bits

#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM)
/*  Computing the checksum if it's bootsector.
    We do it here in DMA because it should work with native, STX, IPF,
    and now ACSI harddisk.
*/

#ifdef SSE_ACSI_BOOTCHECKSUM // floppy + harddrive
  if( !(MCR&CR_HDC_OR_FDC) && fdc_cr==0x80 && !fdc_tr && fdc_sr==1 
    && !(MCR&CR_WRITE) && !CURRENT_SIDE || (MCR&CR_HDC_OR_FDC) && 
    !(MCR&CR_WRITE) && ACSI_EMU_ON && AcsiHdc[acsi_dev].cmd_block[0]==8
     && AcsiHdc[acsi_dev].SectorNum()<3 && AcsiHdc[acsi_dev].cmd_block[4]==1 )
#else
  if(fdc_cr==0x80 && !fdc_tr && fdc_sr==1 && !(MCR&CR_WRITE) && !CURRENT_SIDE)
#endif
  {
    for(int i=0;i<16;i+=2)
    {
      SF314[DRIVE].SectorChecksum+=Fifo
#if defined(SSE_DMA_DOUBLE_FIFO)
        [!BufferInUse]
#endif
        [i]<<8; 
      SF314[DRIVE].SectorChecksum+=Fifo
#if defined(SSE_DMA_DOUBLE_FIFO)
        [!BufferInUse]
#endif
        [i+1]; 
    }
  }

#endif//checksum

#if defined(SSE_BOILER_TRACE_CONTROL__)
#define LOGSECTION LOGSECTION_ALWAYS // for just the bytes 
#else
#define LOGSECTION LOGSECTION_DMA
#endif

#if defined(SSE_DMA_TRACK_TRANSFER)
  Datachunk++; 
#if defined(SSE_BOILER_TRACE_CONTROL)
// this is not relevant for HD, problem when writing too
  if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
#ifdef SSE_ACSI
    if((MCR&CR_HDC_OR_FDC))
      TRACE_LOG("#%03d (%d) %s %06X: ",Datachunk, AcsiHdc[acsi_dev].SectorNum(),(MCR&0x100)?"from":"to",BaseAddress);
    else
#endif
      TRACE_LOG("#%03d (%d-%02d-%02d) %s %06X: ",Datachunk,floppy_current_side(),floppy_head_track[DRIVE],WD1772.CommandType()==2?fdc_sr:0,(MCR&0x100)?"from":"to",BaseAddress);
#endif
#endif
//TODO is PEEK() safe?
  for(int i=0;i<16;i++) // burst, 16byte packets strictly, 8 words
  {
    if(!(MCR&0x100)&& DMA_ADDRESS_IS_VALID_W) // disk -> RAM
      PEEK(BaseAddress)=
        Fifo
#if defined(SSE_DMA_DOUBLE_FIFO)
        [!BufferInUse] // because it's been just toggled
#endif
        [i];
    
    else if((MCR&0x100) && DMA_ADDRESS_IS_VALID_R) // RAM -> disk
      Fifo
#if defined(SSE_DMA_DOUBLE_FIFO)
        [BufferInUse] //bugfix 3.7.2 !  no!!!!!!!!!!! wrong line!
#endif
        [15-i]=PEEK(BaseAddress);
    else 
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
#endif
          TRACE_LOG("!!!");
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
      TRACE_LOG("%02X ",Fifo
#if defined(SSE_DMA_DOUBLE_FIFO)
      [(MCR&CR_WRITE)?BufferInUse:!BufferInUse] //this is correct bugfix 3.7.2
#endif
      [(MCR&CR_WRITE)?15-i:i]);//bugfix 3.6.1 reverse order
#endif
#if USE_PASTI    
    if(hPasti&&pasti_active
#if defined(SSE_DISK_PASTI_ONLY_STX)
    &&(!PASTI_JUST_STX || 
    SF314[DRIVE].ImageType.Extension==EXT_STX)
#if defined(SSE_DISK_PASTI_ONLY_STX_HD)
    || (MCR&CR_HDC_OR_FDC) // hard disk handling by pasti
#endif
#endif        
      )  
      dma_address++;
    else
#endif
      DMA_INC_ADDRESS; // use Steem's existing routine (?)
  }
#if defined(SSE_DMA_COUNT_CYCLES)
  if(ADAT)//3.6.1 condition
    INSTRUCTION_TIME(8); 
#endif
#if defined(SSE_BOILER_TRACE_CONTROL)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
    TRACE_LOG("\n");
#endif
#if defined(SSE_BOILER)
  for(int i=0;i<8;i++) // for Boiler monitor, intercept DMA traffic
  {
    if(!(MCR&0x100)&& DMA_ADDRESS_IS_VALID_W) // disk -> RAM
    {DEBUG_CHECK_WRITE_W(BaseAddress-16+i*2)}
    else if((MCR&0x100) && DMA_ADDRESS_IS_VALID_R) // RAM -> disk
    {DEBUG_CHECK_READ_W(BaseAddress-16+i*2)}
  }
#endif
  Request=FALSE;
}

#undef LOGSECTION

#endif//DMA_FIFO

#endif//object

#endif//dma
