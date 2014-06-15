/*  WD1772 is the object in Steem taking care of various aspects
    of emulation of this disk formatter/controller.
    Because there are many disk image types avaiable, and some
    coming with a WD1772 emulator of their own, and because file
    fdc.cpp already contains Steem's original emulation, which is
    kept, this object can be seen as a dispatcher with integrated
    emulator for STW images and some other emulation variables and
    facilities.
*/

#include "SSE.h"

#include "../pch.h"
#ifdef WIN32
#include <pasti/pasti.h>
#endif
#include <conditions.h>
#include <cpu.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <iorw.decla.h>
#include <mfp.decla.h>
#include <psg.decla.h>
#include <run.decla.h>
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEInterrupt.h"
#include "SSEWD1772.h"


#if defined(SSE_FDC) // other switch?

#define LOGSECTION LOGSECTION_FDC


///////////////////////////////// ID Field ////////////////////////////////////

TWD1772IDField::TWD1772IDField() {
  ZeroMemory(this,sizeof(TWD1772IDField));
}


WORD TWD1772IDField::nBytes() {
  WORD nbytes=1<<( (len&3) +7); // other way: harder
  return nbytes;
}


#ifdef SSE_DEBUG
void TWD1772IDField::Trace() {
  TRACE_LOG("ID track %d side %d num %d len %d CRC %X%X\n",track,side,num,len,CRC[0],CRC[1]);
}
#endif  


/////////////////////////////////// MFM ///////////////////////////////////////

void TWD1772MFM::Decode () {

#if defined(SSE_DISK_STW_MFM) // not optimised
  WORD encoded_shift=encoded;
  data=clock=0; //BYTEs
  for(int i=0;i<8;i++)
  {
    clock|= (encoded_shift&0x8000) != 0;
    if(i<7)
      clock<<=1;
    encoded_shift<<=1;
    data|= (encoded_shift&0x8000) != 0;
    if(i<7)
      data<<=1,encoded_shift<<=1;
  }
#else
  data=(BYTE)encoded&0xFF;
  clock=(BYTE) ((encoded>>8)&0xFF);
  return;//temp no bit mangling
#endif

}


void TWD1772MFM::Encode(int mode) {

  // 1. compute the clock
  clock=0;
  BYTE previous=data_last_bit;
  BYTE current;
  data_last_bit=data&1;
  BYTE data_shift=data;
  for(int i=0;i<8;i++)
  {
    current=data_shift&0x80;
    if(!previous && !current)
      clock|=1;
    if(i<7)
      clock<<=1;
    data_shift<<=1;
    previous=current;
  }
  if(mode==FORMAT_CLOCK)
    if(data==0xA1)
      clock&=~4; // missing bit 2 of clock -> bit 5 of encoded word
    else if(data==0xC2)
      clock&=~2; // missing bit 1 of clock -> bit 4 of encoded word //?


#if defined(SSE_DISK_STW_MFM)
  // 2. mix clock & data to create a word
  data_shift=data;
  BYTE clock_shift=clock;
  encoded=0;
  for(int i=0;i<8;i++)
  {
    encoded|=(clock_shift&0x80) != 0;
    encoded<<=1; clock_shift<<=1;
    encoded|=(data_shift&0x80) != 0;
    if(i<7)
      encoded<<=1; data_shift<<=1;
  }   


#ifdef SSE_DEBUG
  if(mode==FORMAT_CLOCK && (/*data==0xA1 ||*/ data==0xC2)) 
  {
    TRACE_LOG("MFM clock for %X = %X, MFM = %X\n",data,clock,encoded);
    //MFM clock for A1 = A, MFM = 4489
  }
#endif

#else//no mfm:
  encoded=clock;
  encoded<<=8;
  encoded|=data;  
#endif

}


/////////////////////////////////// CRC ///////////////////////////////////////

void TWD1772Crc::Add(BYTE data) {
  //if(data==0xFE) TRACE("CRC %X + %X",crc,data);
  fdc_add_to_crc(crc,data); // we just call Steem's original function for now
 // if(data==0xFE) TRACE("=> %X\n",crc);
#ifdef SSE_DEBUG
  bytes_since_reset++;
#endif
}

bool TWD1772Crc::Check(TWD1772IDField *IDField) {
  bool ok=IDField->CRC[0]==HIBYTE(crc) && IDField->CRC[1]==LOBYTE(crc);
#ifdef SSE_DEBUG
  if(!ok)
    TRACE_LOG("CRC error - computed: %X - read: %X%X\n",crc,IDField->CRC[0],IDField->CRC[1]);
#endif
  return ok;
}


/*  Contrary to what the doc states, the CRC Register isn't preset to ones 
    ($FFFF) prior to data being shifted through the circuit, but to $CDB4.
    This happens for each $A1 address mark (read or written), so the register
    value after $A1 is the same no matter how many address marks.
    When formatting the backup disk, Dragonflight writes a single $F5 (->$A1)
    in its custom track headers and expects value $CDB4.
*/

void TWD1772Crc::Reset() {
  crc=0xCDB4;// not 0xC4DB; 
#ifdef SSE_DEBUG
  bytes_since_reset=0;
#endif
}


/////////////////////////////////// WD1772 ////////////////////////////////////


#if defined(SSE_DISK_GHOST)
/*  Ghost disks are described in doc/sse/stg.txt and implemented in
    SSEGhostDisk.
    Here's the part that does the interception.
*/

bool TWD1772::CheckGhostDisk(BYTE drive, BYTE io_src_b) {

  TWD1772IDField IDField;
  IDField.track=SF314[drive].Track(); // not TR
  IDField.side=YM2149.SelectedSide;
  IDField.num=SR; 
  WORD nbytes;
  
#if defined(SSE_DISK_GHOST_SECTOR)
/*  Simplest case: the game writes sectors using the "single sector"
    way. Super Cycles, Sundog, Great Courts.
    The sector counter could be set for multiple command calls (eg 9
    for all sectors), so we only envision 512/1024.
*/
  if((io_src_b&0xF0)==0xA0 || (io_src_b&0xF0)==0x80)
  {
    switch(Dma.Counter) {
    case 0:
      nbytes=0;
    case 2:
      nbytes=1024;
      IDField.len=3;
    default:
      nbytes=512;
      IDField.len=2;
    }//sw
  }
  
  // WRITE 1 SECTOR
  if(nbytes && (io_src_b&0xF0)==0xA0)
  {
    CR=io_src_b; //update this...
    if(SF314[drive].CheckGhostDisk(true))
    {
      // bytes ST memory -> our buffer
      for(int i=0;i<nbytes;i++)
        *(GhostDisk[drive].SectorData+i)=Dma.GetFifoByte();
      GhostDisk[drive].WriteSector(&IDField);
      STR=FDC_STR_MOTOR_ON;
      WD1772.CommandWasIntercepted=1;
      agenda_fdc_finished(0);
    }
  }
  
  // READ 1 SECTOR
  if(nbytes && (io_src_b&0xF0)==0x80)
  {   
    // sector is in ghost image?
    if(SF314[drive].CheckGhostDisk(false)
      && GhostDisk[drive].ReadSector(&IDField))
    {
      CR=io_src_b; //update this...
      STR=FDC_STR_MOTOR_ON;
      for(int i=0;i<nbytes;i++)
        Dma.AddToFifo(*(GhostDisk[drive].SectorData+i));
      CommandWasIntercepted=1;
      agenda_fdc_finished(0); 
    }
  }
#endif//#if defined(SSE_DISK_GHOST_SECTOR)
  
#if defined(SSE_DISK_GHOST_MULTIPLE_SECTORS)
/*  For multiples sectors:
    - We don't IRQ, the program will use D0 (so we hope).
    - We assume sectors are 512 bytes, which is wrong in
    the only case we know yet: Platoon, but it still works
    because what counts is the #bytes transferred between
    disk and RAM.
    We needed to implement multiple sectors support for Platoon,
    but let's not complain, it's cheap, at least we don't need to
    add Format support even though the game uses command Format (with
    1024byte sectors).
*/
  
  if((io_src_b&0xF0)==0xB0 || (io_src_b&0xF0)==0x90)
  {
    IDField.len=2;
    nbytes=IDField.nBytes(); 
  }
  
  // WRITE MULTIPLE SECTORS
  if(nbytes && (io_src_b&0xF0)==0xB0)
  {
    if(SF314[drive].CheckGhostDisk(true))
    {
      CR=io_src_b; //update this...
      // for all sectors
      const int k=Dma.Counter;
      for(int j=0;j<k;j++)
      {
        // bytes ST memory -> our buffer
        for(int i=0;i<nbytes;i++)
        {
          *(GhostDisk[drive].SectorData+i)=Dma.GetFifoByte();
        }//nxt i
        GhostDisk[drive].WriteSector(&IDField); // write 1 sector
        IDField.num=++SR;
      }//nxt j
      STR=FDC_STR_MOTOR_ON; // but no IRQ
      CommandWasIntercepted=1;
    }
  }//multi-write
 
  // READ MULTIPLE SECTORS
  if(nbytes && (io_src_b&0xF0)==0x90)
  { 
    if(SF314[drive].CheckGhostDisk(false))
    {
      // for all sectors
      const int k=Dma.Counter;
      for(int j=0;j<k;j++)
      {
        // sector is in ghost image?
        if(SF314[drive].State.ghost 
          && GhostDisk[drive].ReadSector(&IDField))
        {
          CR=io_src_b; //update this...
          for(int i=0;i<nbytes;i++)
          {
            Dma.AddToFifo( *(GhostDisk[drive].SectorData+i) );
          } //nxt i
          STR=FDC_STR_MOTOR_ON;
          CommandWasIntercepted=1;
          IDField.num=++SR; // update both SR and ID field's num
        }
      }//nxt j
    }
  }//multi-read
  
#endif//multiple
  
#if defined(SSE_DISK_GHOST_FAKE_FORMAT)
  // FAKE FORMAT
  if((io_src_b&0xF0)==0xF0)
  {
    STR=FDC_STR_MOTOR_ON;
    WD1772.CommandWasIntercepted=1;
    Dma.BaseAddress+=6250;    
    Dma.Counter-=6250/512;//Platoon
    agenda_fdc_finished(0);// "alright"
  }
#endif
  
  
#if defined(SSE_DISK_GHOST_SECTOR_STX1)
/*  Pasti keeps its own variables for all DMA/FDC emulation.
*/
#if USE_PASTI
  if(CommandWasIntercepted && hPasti //&& !WritingToDisk()
    && pasti_active && SF314[drive].ImageType.Manager==MNGR_PASTI)
  {
    ASSERT( !PASTI_JUST_STX || SF314[drive].ImageType.Extension==EXT_STX );
    pasti_update_reg(0xff8609,(Dma.BaseAddress&0xff0000)>>16);
    pasti_update_reg(0xff860b,(Dma.BaseAddress&0xff00)>>8);
    pasti_update_reg(0xff860d,(Dma.BaseAddress&0xff));
  }
#endif//#if USE_PASTI 
#endif//#if defined(SSE_DISK_GHOST_SECTOR_STX1)
  
  return CommandWasIntercepted;
  
}

#endif



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


#if defined(SSE_DISK_STW)

bool TWD1772::Drq(bool state) {
  Lines.drq=state;
  if(state)
    STR|=STR_DRQ;
  else
    STR&=~STR_DRQ;
  if(state) //check old state?
    return Dma.Drq();
  return state;
}

#endif

BYTE TWD1772::IORead(BYTE Line) {
  
  ASSERT( Line>=0 && Line<=3 );
  BYTE ior_byte;
  BYTE drive=floppy_current_drive();
  
  // Steem handling

  switch(Line){

  case 0: // STR
    
#if defined(SSE_DISK_GHOST)
    if(CommandWasIntercepted)
    {
      ior_byte=STR; // just "motor on" $80, but need agenda???
    }
    else
#endif

      // Steem native: Update some flags before returning STR
      
      // IP
      if(floppy_track_index_pulse_active())
        STR|=FDC_STR_T1_INDEX_PULSE;
      else
        STR&=BYTE(~FDC_STR_T1_INDEX_PULSE);
      
      
#if defined(SSE_DISK_STW)
      if(SF314[drive].ImageType.Extension==EXT_STW)
        floppy_type1_command_active= CommandType()==1 ; // TODO
#endif
      
      // WP, SU
      if(floppy_type1_command_active)
      {
        STR&=(~FDC_STR_WRITE_PROTECT);
        if(floppy_mediach[drive])
        {
          if(floppy_mediach[drive]/10!=1) 
            STR|=FDC_STR_WRITE_PROTECT;
          else if (FloppyDrive[drive].ReadOnly)//refix 3.6.1: Aladin!
            STR|=FDC_STR_WRITE_PROTECT;
        }
        //        else if (FloppyDrive[drive].ReadOnly)//refix 3.6.1
        //        STR|=FDC_STR_WRITE_PROTECT;
        
        
#if defined(SSE_DISK_STW)
        if(SF314[drive].ImageType.Extension==EXT_STW)
          ; else
#endif
          if(fdc_spinning_up)
            STR&=BYTE(~FDC_STR_T1_SPINUP_COMPLETE);
          else
            STR|=FDC_STR_T1_SPINUP_COMPLETE;

      } // else it should be set in fdc_execute()
      
      if ((mfp_reg[MFPR_GPIP] & BIT_5)==0)
      {
        LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
          " - Reading status register as "+Str(itoa(STR,d2_t_buf,2)).LPad(8,'0')+
          " ($"+HEXSl(STR,2)+"), clearing IRQ"); )
          floppy_irq_flag=0;
#if defined(SSE_FDC_FORCE_INTERRUPT)
/*
"When using the immediate interrupt condition (i3 = 1) an interrupt
is immediately generated and the current command terminated. 
Reading the status or writing to the Command Register does not
automatically clear the interrupt. The Hex D0 is the only command 
that enables the immediate interrupt (Hex D8) to clear on a subsequent 
load Command Register or Read Status Register operation. 
Follow a Hex D8 with D0 command."
v3.6:
This is wrong apparently, disabled:
Super Monaco Grand Prix (SMGP.MSA on Pirate Gold CD)
*/
//        if(WD1772.InterruptCondition!=8)
#endif
        mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
      }
#if defined(SSE_DISK_STW)
      Lines.irq=0;
#endif
      
      ior_byte=STR;

#if 0 //debug section, original steem
          {
            fdc_str=debug1;
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
            ASSERT(ior_byte==fdc_str);
          }
#endif//debug

#if !defined(SSE_DEBUG_TRACE_IDE)
//      TRACE_LOG("FDC HBL %d STR %X\n",hbl_count,ior_byte);
#endif
#if defined(SSE_DEBUG_TRACE_CONTROL)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCSTR)
    TRACE_LOG("FDC STR %X\n",ior_byte);
#endif

      break;
    case 1:
      ior_byte=TR;
      TRACE_LOG("FDC R TR %d\n",ior_byte);
      break;      
    case 2:
      ior_byte=SR;
      BRK( yoho );
      TRACE_LOG("FDC R SR %d\n",ior_byte);
      break;
    case 3:
      ior_byte=DR;
      TRACE_LOG("FDC R DR %d\n",ior_byte);
      break;
    }//sw

    // IPF handling
#if defined(SSE_IPF)
    if(Caps.IsIpf(drive))
      ior_byte=Caps.ReadWD1772(Line);
#endif

  return ior_byte;
}


void TWD1772::IOWrite(BYTE Line,BYTE io_src_b) {

  ASSERT( Line>=0 && Line<=3 );

  BYTE drive=DRIVE;//YM2149.SelectedDrive;

  switch(Line)
  {
  case 0: // CR - could be blocked, can't record here :(
    {

#if defined(SSE_DISK_GHOST)
      CommandWasIntercepted=0; // reset this at each new whatever command 
#endif

#if defined(SSE_DEBUG) && defined(SSE_DRIVE)
#if defined(SSE_DEBUG_TRACE_CONTROL)
      if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
#endif
      {
        BYTE drive_char= (psg_reg[PSGR_PORT_A]&6)==6? '?' : 'A'+DRIVE;
        TRACE_LOG("FDC CR $%02X drive %c side %d TR %d SR %d DR %d dma$ %X #%d\n",io_src_b,drive_char,floppy_current_side(),fdc_tr,fdc_sr,fdc_dr,Dma.BaseAddress,Dma.Counter);
        //ASSERT(io_src_b!=0xB0);
      }

#endif



#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM)

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IMAGE_INFO
/*  Used this for Auto239.
    Gives the checksum of bootsector.
    $1234 means executable. Use last WORD to adjust.
*/
      if(SF314[drive].SectorChecksum)
        TRACE_LOG("%c: bootsector checksum=$%X\n",'A'+drive,SF314[drive].SectorChecksum);
      SF314[drive].SectorChecksum=0;

#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

#endif//checksum

      bool can_send=true; // are we in Steem's native emu?
#if defined(SSE_IPF)
      can_send=can_send&&!Caps.IsIpf(drive);
#endif

#if USE_PASTI 
      can_send=can_send&&!(hPasti && pasti_active
#if defined(SSE_DRIVE)&&defined(SSE_PASTI_ONLY_STX)
        && (!PASTI_JUST_STX || 
#if defined(SSE_DISK_IMAGETYPE)
// in fact we should refactor this
        SF314[drive].ImageType.Extension==EXT_STX)
#else
        SF314[floppy_current_drive()].ImageType==3)
#endif
#endif            
        );
#endif//pasti

#if defined(SSE_DISK_STW)
      can_send=can_send && SF314[drive].ImageType.Extension!=EXT_STW;
#endif

#ifdef SSE_DEBUG
      if(can_send)
      {
#ifdef SSE_DISK_STW
        ASSERT(!IMAGE_STW);
#endif
//        TRACE_LOG("Can send, drive %d extension %d\n",drive,SF314[drive].ImageType.Extension);
      }
#endif


#if defined(SSE_DISK_GHOST)
      // Check if we should intercept FDC command
      if(!can_send && SSE_GHOST_DISK && !IMAGE_STW) 
      {
        CheckGhostDisk(drive,io_src_b);
      }
#endif//#if defined(SSE_DISK_GHOST)

#if defined(SSE_DRIVE_SOUND)
      if(SSEOption.DriveSound 
#if defined(SSE_DISK_GHOST)
        && !CommandWasIntercepted
#endif
        )
      {
#if defined(SSE_DMA)
        Dma.UpdateRegs();
#endif
        SF314[drive].TrackAtCommand=SF314[drive].Track();
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
        SF314[drive].Sound_CheckCommand(io_src_b);
#else
        SF314[0].Sound_CheckCommand(io_src_b);
#endif
      }
#endif//sound

      if(can_send)
        floppy_fdc_command(io_src_b); // in fdc.cpp
//      else  TRACE_LOG("FDC %X not native, drive %C type %d\n",io_src_b,'A'+YM2149.Drive(),SF314[YM2149.Drive()].ImageType);

#if defined(SSE_DISK_STW)
      if(SF314[drive].ImageType.Extension==EXT_STW)
        WriteCR(io_src_b);
#endif


    }
    break;

  case 1: // TR
#if defined(SSE_DEBUG_TRACE_CONTROL)
    if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
#endif
    TRACE_LOG("FDC TR %d\n",io_src_b);

#if defined(SSE_FDC_CHANGE_TRACK_WHILE_BUSY)
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
#if defined(SSE_DEBUG_TRACE_CONTROL)
    if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
#endif
    TRACE_LOG("FDC SR %d (pc %x)\n",io_src_b,old_pc );

#if defined(SSE_FDC_CHANGE_SECTOR_WHILE_BUSY)
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
#if defined(SSE_DEBUG_TRACE_CONTROL)
    if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
#endif
    TRACE_LOG("FDC DR %d\n",io_src_b);

    log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC data register to "+io_src_b);
    DR=io_src_b;
    break;
  }
  
  // IPF handling
#if defined(SSE_IPF)

  if(Caps.IsIpf(drive)) 
#if defined(SSE_DISK_GHOST)
    if(SSE_GHOST_DISK && WD1772.CommandWasIntercepted)
    {
      TRACE_LOG("Caps doesn't get command %x\n",io_src_b);
      Caps.WD1772.r_command=CR;//update this...
   //   WD1772.CommandWasIntercepted=0;
    }
    else
#endif
    Caps.WriteWD1772(Line,io_src_b);

#endif
  
}


#if defined(SSE_DISK_STW)

void TWD1772::Irq(bool state) {
  if(state&& !Lines.irq && (STR&STR_BUSY) ) // so not on "force interrupt"
  {
    IndexCounter=10;
    //TRACE_LOG("%d IP for motor off\n",IndexCounter);
    prg_phase=WD_MOTOR_OFF;
    STR&=~STR_BUSY;
    if(CommandType()==2 || CommandType()==3)
      STR&=~STR_DRQ;

#if defined(SSE_DRIVE_SOUND)
    if(SSEOption.DriveSound)
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
      SF314[DRIVE].Sound_CheckIrq();
#else
    SF314[0].Sound_CheckIrq();
#endif
#endif
    TRACE_LOG("STW "); Dma.UpdateRegs(true);////// IRQ CR %X STR %X\n",CR,STR);
  }
  Lines.irq=state;
  mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,!state); // MFP GPIP is "active low"

  // reset drive R/W flags
  SF314[DRIVE].State.reading=SF314[DRIVE].State.writing=0;

}

#endif

#if defined(SSE_DISK_STW)

void TWD1772::Motor(bool state) {
  Lines.motor=state;
  if(state)
    STR|=STR_MO;
  else
    STR&=~STR_MO;
  // only on currently selected drive, if any:
  if(YM2149.Drive()!=TYM2149::NO_VALID_DRIVE)
    SF314[YM2149.SelectedDrive].Motor(state); 
  else TRACE_LOG("WD motor: no drive\n");
}

#endif

#if defined(SSE_DISK_STW)

int TWD1772::MsToCycles(int ms) {
  return ms*CpuNormalHz/1000; // *8000
}

#endif

#if defined(SSE_WD1772_PHASE)


/*  Here there's no discussion: WD1772 got a new command and it will
    start executing it.
*/

void TWD1772::NewCommand(BYTE command) {
  ASSERT( IMAGE_STW );//for now
  //TRACE_LOG("STW new command %X\n",command);
  CR=command;

  // reset drive R/W flags
  SF314[DRIVE].State.reading=SF314[DRIVE].State.writing=0;

  BYTE type=CommandType(command);
  switch(type)
  {
  case 1: // I
    // Set Busy, Reset CRC, Seek error, DRQ, INTRQ
    STR|=STR_BUSY;
    STR&=~(STR_CRC|STR_SE);
    Drq(0); // this takes care of status bit
    Irq(0);
    // should we wait for spinup (H=0)?
    if(!(CR&CR_H) && !Lines.motor)
    {
      // Set MO wait 6 index pulses
      Motor(true); // this will create the event at each IP until motor off
      IndexCounter=6;
      TRACE_LOG("%d IP to spin up\n",IndexCounter);
      prg_phase=WD_TYPEI_SPINUP;
    }
    else
    {
//      TRACE("no delay\n");
      //doc doesn't say motor is started if it wasn't spinning yet and h=1
      prg_phase=WD_TYPEI_SPUNUP;
      OnUpdate(); //go direct, no delay
    }
    break;

  case 2: // II
    // Set Busy, Reset CRC, DRQ, LD, RNF, WP, Record Type
    STR|=STR_BUSY;
    STR&=~(STR_LD|STR_RNF|STR_WP|STR_RT);
    Drq(0);
    Irq(0);
    if(!(CR&CR_H) && !Lines.motor)
    {
      Motor(true); 
      IndexCounter=6;
      TRACE_LOG("%d IP to spin up\n",IndexCounter);
      prg_phase=WD_TYPEII_SPINUP;
    }
    else
    {
      prg_phase=WD_TYPEII_SPUNUP;
      OnUpdate();
    }
    break;

  case 3: // III
    STR|=STR_BUSY;
    STR&=~(STR_LD|STR_RNF|STR_WP|STR_RT); // we add WP
    Drq(0);
    Irq(0); // we add this
    // we treat the motor / H business as for type II, not as on flow chart
    if(!(CR&CR_H) && !Lines.motor)
    {
      Motor(true); 
      IndexCounter=6;
      TRACE_LOG("%d IP to spin up\n",IndexCounter);
      prg_phase=WD_TYPEIII_SPINUP;
    }
    else
    {
      prg_phase=WD_TYPEIII_SPUNUP;
      OnUpdate();
    }
    break;

  case 4: // IV
    if(STR&STR_BUSY)
    {
      STR&=~STR_BUSY;
      if(CR&CR_I3) // immediate, D8
      {
        Irq(true); // doc says it stays high but not in the MFP?
        prg_phase=WD_MOTOR_OFF; // must program it ourselves (correct?)
        IndexCounter=10;
        TRACE_LOG("%d IP for motor off\n",IndexCounter);
      }
      else if(CR&CR_I2) // each IP, D4
      {
        prg_phase=WD_TYPEIV_4;
        IndexCounter=1;
      }
      else // D0, just stop motor in 9 rev
      {
        prg_phase=WD_MOTOR_OFF;
        IndexCounter=10;
        TRACE_LOG("%d IP for motor off\n",IndexCounter);
      }
    }
    else 
    {
      floppy_type1_command_active=2; // something like that?
      prg_phase=WD_MOTOR_OFF; // + D0 on D8 (on not busy) ?
      IndexCounter=10;
      TRACE_LOG("%d IP for motor off\n",IndexCounter);
    }
    break;
  }//sw

}

#endif


#if defined(SSE_DRIVE_INDEX_PULSE)
/*  Drive calls this function at IP if it's selected.
    Whether the WD1772 is waiting for it or not.
*/

void TWD1772::OnIndexPulse() {

  ASSERT( prg_phase!=WD_TYPEII_READ_DATA );
  ASSERT( prg_phase!=WD_TYPEII_WRITE_DATA ); //hu ho...

  //time_of_last_ip=ACT; //used?

  IndexCounter--;

  //TRACE_LOG("WD IP %d\n",IndexCounter);
  if(!IndexCounter)
  {
    switch(prg_phase)
    {
    case WD_TYPEI_SPINUP:
    case WD_TYPEII_SPINUP:
    case WD_TYPEIII_SPINUP:
      prg_phase++; // we assume next phase is spunup for this optimisation
      OnUpdate();
      break;

    case WD_TYPEI_FIND_ID:
    case WD_TYPEI_READ_ID:
    case WD_TYPEII_FIND_ID:
    case WD_TYPEII_READ_ID:
    case WD_TYPEIII_READ_ID:
      TRACE_LOG("Find ID timeout\n");
      STR|=STR_SE; // RNF is same bit as SE
      Irq(true);      
      break;

    case WD_TYPEIII_IP_START: 
      IndexCounter=1;
      //TRACE_LOG("%d IP for stop track operation\n",IndexCounter);
      if(CR&CR_TYPEIII_WRITE)
      {
        TRACE_LOG("Start formatting\n");
//        SF314[DRIVE].State.writing=true;
        prg_phase=WD_TYPEIII_WRITE_DATA;
        OnUpdate(); // hop
      }
      else
      {
        TRACE_LOG("Start reading track\n");
        prg_phase=WD_TYPEIII_READ_DATA;
        Read();
      }
      break;

    case WD_TYPEIII_WRITE_DATA:
    case WD_TYPEIII_READ_DATA:
      Irq(true);
      break;

    case WD_TYPEIV_4:
      Irq(true);
      IndexCounter=1;
      //TRACE_LOG("%d IP for $D4 interrupt\n",IndexCounter);
      break;

    case WD_MOTOR_OFF:
      Motor(false);
#if defined(SSE_DISK_GHOST)
      CommandWasIntercepted=0; // reset this at each new command
#endif
      break;

    default: // drive is spinning, WD isn't counting
      break; 
    }//sw
  }//if

}

#endif


#if defined(STEVEN_SEAGAL) && defined(SSE_FLOPPY_EVENT)
/*  This is yet another WD1772 emulation, specially written to handle
    STW disk images. 
    A goal was to be able to use it for another format like SCP as well
    in the future, hence we work with a spinning drive and a flow (of
    bytes).
    We follow Western Digital flow charts, with some additions and (gasp!) 
    corrections. 
    Because we use Dma.Drq() for each byte, the agenda system is too gross.
    Each time an operation takes time, we set up an event that sends here.
    Otherwise we use recursion to hop to next phase, it's better than
    goto.
    So it doesn't work with emulation cycles like CapsImg but with timed
    events like Pasti. Still not sure what's the best approach here.
*/

void TWD1772::OnUpdate() {

  update_time=ACT+n_cpu_cycles_per_second; // we come here anyway

  if(!IMAGE_STW) // only for those images for now
    return; 

  switch(prg_phase)
  {
    
  case WD_TYPEI_SPUNUP:
    // we come here after 6 IP or directly
    if(CR&CR_STEP) // is command a step, step-in, step-out?
    {
      // if step-in or step-out, update DIRC line
      if((CR&CR_STEP)==CR_STEP_IN) 
        Lines.direction=true;
      else if((CR&CR_STEP)==CR_STEP_OUT) 
        Lines.direction=false; 
      // goto B or C according to flag u
      prg_phase=(CR&CR_U)?WD_TYPEI_STEP_UPDATE:WD_TYPEI_STEP;

    }
    else  // else it's seek/restore
    {
      if((CR&CR_SEEK)==CR_RESTORE) // restore?
      {
        TR=0xFF;
        
        Lines.track0=(floppy_head_track[DRIVE]==0);
        if(Lines.track0)
          TR=0;
        DR=0;
      }    
      prg_phase=WD_TYPEI_SEEK; // goto 'A'
    }
    OnUpdate(); // some recursion is always cool
    break;

  case WD_TYPEI_SEEK: // 'A'
    DSR=DR;
    if(TR==DSR)
    {
      prg_phase=WD_TYPEI_CHECK_VERIFY;
      OnUpdate(); // some recursion is always cool
    }
    else
    {
      Lines.direction=(DSR>TR);
      prg_phase=WD_TYPEI_STEP_UPDATE;
      OnUpdate(); // some recursion is always cool
    }
    break;

  case WD_TYPEI_STEP_UPDATE: // 'B'
    if(Lines.direction)
      TR++;
    else
      TR--;
    prg_phase=WD_TYPEI_STEP;
    OnUpdate(); // some recursion is always cool
    break;

  case WD_TYPEI_STEP: // 'C'
            Lines.track0=(floppy_head_track[DRIVE]==0);

    if(Lines.track0 && !Lines.direction)
    {
      TR=0;
      prg_phase=WD_TYPEI_CHECK_VERIFY;
      OnUpdate(); // some recursion is always cool
    }
    else
    {
      StepPulse();
/*
Delay according to r1,r0 field

Command      Bit 7     B6     B5     B4     B3     B2     B1     Bit 0
--------     -----     --     --     --     --     --     --     -----
Restore      0         0      0      0      h      V      r1     r0
Seek         0         0      0      1      h      V      r1     r0
Step         0         0      1      u      h      V      r1     r0
Step in      0         1      0      u      h      V      r1     r0
Step out     0         1      1      u      h      V      r1     r0

r1       r0            1772
--       --            ----
0        0             6000 cycles
0        1             12000 cycles
1        0             2000 cycles
1        1             3000 cycles
*/
      switch(CR&(CR_R1|CR_R0))
      {
      case 0:
        update_time=MsToCycles(6);
        break;
      case 1:
        update_time=MsToCycles(12);
        break;
      case 2:
        update_time=MsToCycles(2);
        break;
      case 3:
        update_time=MsToCycles(3);
        break;
      }//sw
      update_time+=ACT;
      prg_phase=WD_TYPEI_STEP_PULSE;
    }
    break;

  case WD_TYPEI_STEP_PULSE:
    // goto 'D' if command is step, 'A' otherwise
    prg_phase=(CR&CR_STEP)?WD_TYPEI_CHECK_VERIFY:WD_TYPEI_SEEK;
    OnUpdate(); // some recursion is always cool
    break;

  case WD_TYPEI_CHECK_VERIFY: // 'D'
    if(CR&CR_V)
    {
      if(IMAGE_STW)
      {
        ImageSTW[DRIVE].LoadTrack(CURRENT_SIDE,SF314[DRIVE].Track());
      }
      prg_phase=WD_TYPEI_HEAD_SETTLE; 
      update_time=ACT+ MsToCycles(15);
    }
    else
    {
      Irq(true); // this updates status bits
    }
    break;

  case WD_TYPEI_HEAD_SETTLE:
    // flow chart is missing head settling
    prg_phase=WD_TYPEI_FIND_ID;
    n_format_bytes= n00=nFF=0;
    Read(); // drive will send word (clock, byte) and set event
    IndexCounter=6; 
//    TRACE_LOG("%d IP to find ID %d\n",IndexCounter,SR);
    break;

  case WD_TYPEI_FIND_ID:
  case WD_TYPEII_FIND_ID:
  case WD_TYPEIII_FIND_ID:

/*  From doc, sequence is 12x0, 3xA1, FE or FF, it must strictly be respected.
    But Platoon: it may be $FF instead of 00, 11 bytes may be enough, and
    a series of 11 x 0 + 1 x 2 is OK. We try to handle this.
    There are certainly more variations possible.
    When looking for ID, the WD1772 ignores the last bit, so $FE is
    equivalent to $FF.
*/
    CrcChecker.Add(DSR);

    if(DSR==0xA1 && !(Mfm.clock&BIT_5) || DSR==0xC2 && !(Mfm.clock&BIT_4) )
    {
      n_format_bytes++;
      if(DSR==0xA1)
        CrcChecker.Reset(); // only special $A1 resets the CRC logic
    }
    else if(!n_format_bytes) // count zeroes (or ones)
    {
      if(!DSR || n00==11&&DSR==2)
      {
        n00++;
        nFF=0;
      }
      else if(DSR==0xFF)
      {
        n00=0;
        nFF++;
      }
      else
      {
        n00=nFF=0;
      }
    }
    else if((DSR&0xFE)==0xFE && n_format_bytes==3)
    {
      TRACE_LOG("%X at %d\n",DSR,SF314[DRIVE].BytePosition());
      n_format_bytes=n00=nFF=0;//reset
      prg_phase++; // in type I or type II or III
    }
    else if(n_format_bytes)
    {
      n_format_bytes=n00=nFF=0;
    }
    Read(); // this sets up next event
    break;

  case WD_TYPEI_READ_ID:
  case WD_TYPEII_READ_ID:
  case WD_TYPEIII_READ_ID:
    // fill in ID field
    *(((BYTE*)&IDField)+n_format_bytes)=DSR; //no padding!!!!!!
    if(n_format_bytes<4)
      CrcChecker.Add(DSR);
    if(prg_phase==WD_TYPEIII_READ_ID)
    {
      DR=DSR;
      Drq(true); // read address
    }
    n_format_bytes++;
    if(n_format_bytes==sizeof(TWD1772IDField))
    {
      n_format_bytes=0; 
      prg_phase++; // in type I, II, III
      OnUpdate(); // some recursion is always cool
    }
    else
      Read();
    break;

  case WD_TYPEI_TEST_ID:
    //test track and CRC
    if(IDField.track==TR && CrcChecker.Check(&IDField))
    {
      CrcChecker.Reset();
      Irq(true); // verify OK
    }
    else // they should all have correct track, will probably time out
    {
      prg_phase=WD_TYPEI_FIND_ID;
      if(IDField.track==TR)
      {
        STR|=STR_CRC; // set CRC error if track field was OK
      }
      CrcChecker.Add(DSR); //unimportant
      Read(); // this sets up next event
    }
    break;

  case WD_TYPEII_SPUNUP:
  case WD_TYPEIII_SPUNUP:
    prg_phase++;
    if(CR&CR_E) // head settle delay programmed
      update_time=ACT+MsToCycles(15);
    else
      OnUpdate(); // some recursion is always cool
    break;

  case WD_TYPEII_HEAD_SETTLE: // we come directly or after 15ms delay
//    SF314[DRIVE].State.reading=true;
    // check Write Protect for command write sector
    if((CR&CR_TYPEII_WRITE) 
      && (Lines.write_protect|| FloppyDrive[DRIVE].ReadOnly) )
    {
      STR|=STR_WP;
      Irq(true);
    }
    else // read, or write OK
    {
      IndexCounter=5; 
      //TRACE_LOG("%d IP to find ID\n",IndexCounter);
      prg_phase=WD_TYPEII_FIND_ID; // goto '1'
      n_format_bytes=n00=nFF=0;
      Read();
    }
    break;

  case WD_TYPEII_TEST_ID:
    
    ASSERT(!n_format_bytes);
    IDField.Trace();
    if(IDField.track==TR && IDField.num==SR)
    {
      ByteCount=IDField.nBytes();
      if(CrcChecker.Check(&IDField))
      {
        CrcChecker.Reset();
        prg_phase=(CR&CR_TYPEII_WRITE) ? WD_TYPEII_WRITE_DAM :
          WD_TYPEII_FIND_DAM;
      }
      else
      {
        //TRACE("CRC ID error read: %X computed: %X #bytes %d\n",(WORD)(*(WORD*)IDField.CRC),CrcChecker.crc,debug2);
        STR|=STR_CRC;
        CrcChecker.Add(DSR);
        prg_phase=WD_TYPEII_FIND_ID; 
      }
    }
    else // it's no error (yet), the WD1772 must browse the IDs
      prg_phase=WD_TYPEII_FIND_ID;
    Read();
    break;

  case WD_TYPEII_FIND_DAM:
    CrcChecker.Add(DSR);//before eventual reset
    n_format_bytes++;
    if(n_format_bytes==43) //timed out
    {
      TRACE_LOG("No DAM, time out!\n");
      n_format_bytes=0;
      prg_phase=WD_TYPEII_FIND_ID;
    }
    else if(DSR==0xA1 && !(Mfm.clock&BIT_5)) 
      //TODO? We don't really check 3A1 + DAM! 
    {
      //TRACE("%X found at position %d, reset CRC\n",DSR,Disk[DRIVE].current_byte);
      CrcChecker.Reset();
    }
    else if( (DSR&0xFE)==0xF8 ||  (DSR&0xFE)==0xFA ) // DAM found
    {
      TRACE_LOG("TR%d SR%d %X found at position %d\n",TR,SR,DSR,Disk[DRIVE].current_byte);
      n_format_bytes=0; // for CRC later
      prg_phase=WD_TYPEII_READ_DATA;
      if((DSR&0xFE)==0xF8)
        STR|=STR_RT; // "record type" set when "deleted data" DAM
    }
    Read();    
    break;

  case WD_TYPEII_READ_DATA:
    CrcChecker.Add(DSR);
    DR=DSR;
    Drq(true); // DMA never fails to take the byte
    ByteCount--;
    ASSERT(!n_format_bytes);
    if(!ByteCount)
      prg_phase=WD_TYPEII_READ_CRC;
    Read();
    break;

  case WD_TYPEII_READ_CRC:
    IDField.CRC[n_format_bytes]=DSR; // and we don't add to CRC
    if(n_format_bytes) //1
    {
      if(!CrcChecker.Check(&IDField))
      {
//        TRACE_LOG("Read sector CRC error\n");
        STR|=STR_CRC;
        Irq(true);
      }
      else
      {
        prg_phase=WD_TYPEII_CHECK_MULTIPLE;
        OnUpdate(); // some recursion is always cool
      }
    }
    else
    {
      n_format_bytes++;
      Read(); // next CRC byte
    }
    break;

  case WD_TYPEII_CHECK_MULTIPLE:
    if(CR&CR_M)
    {
      SF314[DRIVE].State.writing=0;
      SR++;
      prg_phase=WD_TYPEII_HEAD_SETTLE; // goto '4'
      OnUpdate();
    }
    else
      Irq(true);
    break;

  case WD_TYPEII_WRITE_DAM:
    n_format_bytes++;
    if(n_format_bytes<23-1) //22 or 23?
      Read();    
    else if(n_format_bytes<23-1+12) // write 12 $0
    {
      Lines.write_gate=1; // those lines don't matter for now
      Lines.write=1;
      Mfm.data=0;
      Mfm.Encode(); 
      CrcChecker.Add(Mfm.data); // shouldn't matter
      Write();
    }
    else if(n_format_bytes<23-1+12+3) // write $A1 (missing in flow chart)
    {

      Mfm.data=0xA1;
      Mfm.Encode(TWD1772MFM::FORMAT_CLOCK); 
      //TRACE("write %X at position %d, reset CRC\n",Mfm.data,Disk[DRIVE].current_byte);
      CrcChecker.Add(Mfm.data); // before reset   
      CrcChecker.Reset();
     // CrcChecker.Add(Mfm.data); // after eventual reset (TODO)       
      Write();
    }
    else if(n_format_bytes==23-1+12+3) // write DAM acording to A0 field
    {
      Mfm.data= (CR&CR_A0)? 0xF9 : 0xFB;
      Mfm.Encode(); 
      TRACE_LOG("TR %d SR %d write %X at position %d\n",TR,SR,Mfm.data,Disk[DRIVE].current_byte);
      CrcChecker.Add(Mfm.data); // after eventual reset (TODO)        
      Write();     
    }
    else
    {
      n_format_bytes=0;
      prg_phase=WD_TYPEII_WRITE_DATA;
      OnUpdate(); // some recursion is always cool  
    }

    break;

  case WD_TYPEII_WRITE_DATA:
    Drq(true); // normally first DRQ happened much earlier, we simplify
    DSR=DR;
    CrcChecker.Add(DSR);
    Mfm.data=DSR;
    Mfm.Encode();
    ByteCount--;
    ASSERT(!n_format_bytes);
    if(!ByteCount)
      prg_phase=WD_TYPEII_WRITE_CRC;
    Write();
    break;

  case WD_TYPEII_WRITE_CRC: // CRC + final $FF (?)
    n_format_bytes++;
    if(n_format_bytes==1)
      Mfm.data=CrcChecker.crc>>8;
    else if(n_format_bytes==2)
      Mfm.data=CrcChecker.crc&0xFF;
    else
    {
      n_format_bytes=0;
      prg_phase=WD_TYPEII_CHECK_MULTIPLE;
      Mfm.data=0xFF;
      Lines.write_gate=0; //early
      Lines.write=0; // early
    }
    Mfm.Encode();
    Write();
    break;

  case WD_TYPEIII_HEAD_SETTLE: // we come directly or after 15ms delay
    if((CR&0xF0)==CR_TYPEIII_READ_ADDRESS)
    {
      IndexCounter=5; //not documented, see OnIndexPulse()
//      TRACE_LOG("%d IP for read address\n",IndexCounter);
      prg_phase=WD_TYPEIII_FIND_ID;
      n_format_bytes=n00=nFF=0;
      Read();
    }
    // check Write Protect for command write track
    else if((CR&CR_TYPEIII_WRITE) 
      && (Lines.write_protect|| FloppyDrive[DRIVE].ReadOnly) )
    {
      TRACE_LOG("Can't write on disk\n");
      STR|=STR_WP;
      Irq(true);
    }
    else // for read & write track, we start at next IP
    {
      IndexCounter=1;
  //    TRACE_LOG("%d IP for read or write track\n",IndexCounter);
      prg_phase=WD_TYPEIII_IP_START;
    }
    break;

  case WD_TYPEIII_TEST_ID:
    if(!CrcChecker.Check(&IDField))
      STR|=STR_CRC;
    Irq(true);
    break;

  case WD_TYPEIII_READ_DATA: 

    // "the Address Mark Detector is on for the duration of the command"
    if(DSR==0xA1 && !(Mfm.clock&BIT_5)) //Mfm.clock==0x0A) 
    {
      if(CrcChecker.crc!=0xCDB4)
        DSR=0x14; // 1st AM doesn't read as $A1: Union Demo
      CrcChecker.Reset();
    }
    else
      CrcChecker.Add(DSR); 
    
    DR=DSR;
    Drq(true);
    Read();
    break;
    
  case WD_TYPEIII_WRITE_DATA:  
    // The most interesting part, and novelty in ST emulation!
    Drq(true);
    DSR=DR;
    // analyse byte in for MFM markers
    if(DSR==0xF5) //Write A1 in MFM with missing clock Init CRC
    {
      Mfm.data=DSR=0xA1;
      Mfm.Encode(TWD1772MFM::FORMAT_CLOCK);
      CrcChecker.Reset();
      Write();
    }
    else if(DSR==0xF6) //Write C2 in MFM with missing clock
    {
      Mfm.data=DSR=0xC2;
      Mfm.Encode(TWD1772MFM::FORMAT_CLOCK);
      CrcChecker.Add(Mfm.data);
      Write();
    }
/*  The format code $F7 may be used inside an ID field. The CRC bytes are added
    to the CRC, so that this is correct. This implies that at the receipt of
    $F7, the WD1772 saves the current value of the CRC (at least the lower 
    byte), before it is modified by the upper byte.
    $F7 will trigger output of CRC only if the CRC is non null (Dragonflight
    track headers).
    Notice that the CRC is null if after a reset you output it:
    $F5 -> $A1, CRC = $CDB4
    $F7 -> $CD, $B4, CRC = 0
    $F7 -> $F7
    So the 2nd $F7 is really written $F7 on the disk.
*/
    else if(DSR==0xF7 && CrcChecker.crc) //Write 2 CRC Bytes
    {
//      TRACE("write CRC %X %d %d\n",CrcChecker.crc,HIBYTE(CrcChecker.crc),LOBYTE(CrcChecker.crc));
      Mfm.data=DSR=CrcChecker.crc>>8; // write 1st byte
      DR=CrcChecker.crc&0xFF; // save 2nd byte
      CrcChecker.Add(Mfm.data);
      Mfm.Encode(TWD1772MFM::FORMAT_CLOCK);
      Write();
      prg_phase=WD_TYPEIII_WRITE_DATA2; // for 2nd byte
    }
    else
    {
      Mfm.data=DSR;
      Mfm.Encode(TWD1772MFM::FORMAT_CLOCK);
      CrcChecker.Add(DSR);
      Write();
    }

    break;

  case WD_TYPEIII_WRITE_DATA2:
    // write 2nd byte of CRC
    Mfm.data=DSR=DR;// as saved
    CrcChecker.Add(Mfm.data);
    Mfm.Encode(TWD1772MFM::FORMAT_CLOCK);
    Write(); 
    prg_phase=WD_TYPEIII_WRITE_DATA; // go back
    break;

  }//sw
}

#endif



#if defined(SSE_DISK_STW)

void TWD1772::Read() {
  if(YM2149.Drive()!=TYM2149::NO_VALID_DRIVE)
  {
    SF314[YM2149.SelectedDrive].Read(); // this gets data and creates event
    Mfm.Decode();
    DSR=Mfm.data;
  }
}

#endif

#if defined(SSE_FDC_RESET)
void TWD1772::Reset(bool Cold) {
  STR=2;
#ifdef SSE_FDC_FORCE_INTERRUPT
  InterruptCondition=0;
#endif
#if defined(SSE_FDC_INDEX_PULSE_COUNTER)
  IndexCounter=0;
#endif

#if defined(SSE_DISK_GHOST)
  CommandWasIntercepted=0;
#endif

}
#endif


#if defined(SSE_DISK_STW)

void TWD1772::StepPulse() {
  Lines.step=true; // useless now, normally it lasts some µs
  Lines.step=false;
  if(YM2149.Drive()!=TYM2149::NO_VALID_DRIVE)
    SF314[YM2149.SelectedDrive].Step(Lines.direction);
}

#endif

#if defined(SSE_DEBUG) || defined(SSE_OSD_DRIVE_LED)

int TWD1772::WritingToDisk() { // note, could do this at DMA level
  return((CR&0xF0)==0xF0 || (CR&0xE0)==0xA0 || (CR&0xE0)==0xB0) ;
}

#endif


#if defined(SSE_DEBUG)

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
    if(type==1 || type==4) 
      TRACE_LOG("IP "); // index
    else
      TRACE_LOG("DRQ "); // data request
  if(STR&0x01)
    TRACE_LOG("BSY "); // busy
    TRACE_LOG(") "); 
}

#endif//debug



#if defined(SSE_DISK_STW)

void TWD1772::Write() {
  if(YM2149.Drive()!=TYM2149::NO_VALID_DRIVE)
    SF314[YM2149.SelectedDrive].Write(); // this writes data and creates event  
}

#endif

#if defined(SSE_DISK_STW)

void  TWD1772::WriteCR(BYTE io_src_b) {

  TRACE_LOG("STW new command %X\n",io_src_b);
  if(CommandType(io_src_b)==2 || CommandType(io_src_b)==3)
    ImageSTW[DRIVE].LoadTrack(floppy_current_side(),floppy_head_track[DRIVE]);

/*  CR will accept a new command when busy bit is clear or when command
    is 'Force Interrupt'.
    Not documented: also when the drive is still spinning up 
    TODO only during a type I command? check in caps
    TODO check general structure, not sure it's ideal even if limited
    to STW
*/
  if(!(STR&STR_BUSY) 
    || prg_phase==WD_TYPEI_SPINUP
    || prg_phase==WD_TYPEII_SPINUP
    || prg_phase==WD_TYPEIII_SPINUP
    || (io_src_b&0xF0)==0xD0 )
  {
    mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Turn off IRQ output
#if defined(SSE_DISK_STW)
    Lines.irq=0; // also when previous command = $D8?
#endif
    agenda_delete(agenda_fdc_motor_flag_off); // and others?
    Dma.Datachunk=0;
    NewCommand(io_src_b);
  }
#ifdef SSE_DEBUG
  else
    TRACE_LOG("FDC command %X ignored\n",io_src_b);
#endif
}

#endif


#endif//SSE_FDC
