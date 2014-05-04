#include "../pch.h"
#include "SSE.h"
#include "SSEWD1772.h"
#include "SSEDebug.h"


//todo: ambiguous, this is both a dispatcher and budding specialised emu


//temp
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

#include "SSEFloppy.h"//temp!!

#if defined(SS_FDC)

#define LOGSECTION LOGSECTION_FDC

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

#if defined(SS_DISK_GHOST)
      if(CommandWasIntercepted)
      {
        ior_byte=STR; // just "motor on" $80
//        TRACE("will return STR as %x\n",ior_byte);
        break; 
      }
#endif

#if defined(SS_DISK_STW)
      if(SF314[drive].ImageType.Extension==EXT_STW)
      {
        ior_byte=STR;
        goto L1;//temp
      }
#endif


      // Steem native: Update some flags before returning STR
      // IP

      if(floppy_track_index_pulse_active())
        STR|=FDC_STR_T1_INDEX_PULSE;
      else
        STR&=BYTE(~FDC_STR_T1_INDEX_PULSE);
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

        // seems OK, no reset after command:
        if(fdc_spinning_up)
          STR&=BYTE(~FDC_STR_T1_SPINUP_COMPLETE);
        else
          STR|=FDC_STR_T1_SPINUP_COMPLETE;
/* //MFD
        // it's done in another part anyway - agenda_fdc_finished
        // here it will only break programs with useless bloat
        if(ADAT&&fdc_tr  // (ADAT: v3.5.1, No Cooper) //TR or CYL?
          && WD1772.CommandType()==1) // v3.6.0: Super Monaco Grand Prix 
          STR|=FDC_STR_T1_TRACK_0;
*/ 
      } // else it should be set in fdc_execute()
      if ((mfp_reg[MFPR_GPIP] & BIT_5)==0)
      {
        LOG_ONLY( DEBUG_ONLY( if (mode==STEM_MODE_CPU) ) log_to(LOGSECTION_FDC,Str("FDC: ")+HEXSl(old_pc,6)+
          " - Reading status register as "+Str(itoa(STR,d2_t_buf,2)).LPad(8,'0')+
          " ($"+HEXSl(STR,2)+"), clearing IRQ"); )
        floppy_irq_flag=0;
#if defined(SS_FDC_FORCE_INTERRUPT)
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
L1://temp
#if !defined(SS_DEBUG_TRACE_IDE)
//      TRACE_LOG("FDC HBL %d STR %X\n",hbl_count,ior_byte);
#endif
#if defined(SS_DEBUG_TRACE_CONTROL)
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
      TRACE_LOG("FDC R SR %d\n",ior_byte);
      break;
    case 3:
      ior_byte=DR;
      TRACE_LOG("FDC R DR %d\n",ior_byte);
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

#if defined(SS_DISK_GHOST)
      //if(CommandWasIntercepted) TRACE("clear CommandWasIntercepted\n");
      CommandWasIntercepted=0; // reset this at each new command
#endif

#if defined(SS_DEBUG) && defined(SS_DRIVE)
#if defined(SS_DEBUG_TRACE_CONTROL)
      if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
      {
        BYTE drive_char= (psg_reg[PSGR_PORT_A]&6)==6? '?' : 'A'+DRIVE;
        TRACE_LOG("FDC CR $%2X drive %c side %d TR %d SR %d DR %d dma$ %X HBL %d\n",io_src_b,drive_char,floppy_current_side(),fdc_tr,fdc_sr,fdc_dr,Dma.BaseAddress,hbl_count);
      }
#endif
#endif

#if defined(SS_DRIVE_SOUND)
      if(SSE_DRIVE_SOUND)
      {
#if defined(SS_DMA)
        Dma.UpdateRegs();
#endif
        SF314[drive].TrackAtCommand=SF314[drive].Track();
#if defined(SS_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
        SF314[drive].Sound_CheckCommand(io_src_b);
#else
        SF314[0].Sound_CheckCommand(io_src_b);
#endif
      }
#endif//sound

#if defined(SS_DRIVE_COMPUTE_BOOT_CHECKSUM)

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IMAGE_INFO
/*  Used this for Auto239.
    Gives the checksum of bootsector.
    $1234 means executable. Use last WORD to adjust.
*/
      if(SF314[floppy_current_drive()].SectorChecksum)
        TRACE_LOG("%c: bootsector checksum=$%X\n",'A'+floppy_current_drive(),SF314[floppy_current_drive()].SectorChecksum);
      SF314[floppy_current_drive()].SectorChecksum=0;

#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

#endif//checksum

      bool can_send=true; // are we in Steem's native emu?
#if defined(SS_IPF)
      can_send=can_send&&!Caps.IsIpf(drive);
#endif

#if USE_PASTI 
      can_send=can_send&&!(hPasti && pasti_active
#if defined(SS_DRIVE)&&defined(SS_PASTI_ONLY_STX)
        && (!PASTI_JUST_STX || 
#if defined(SS_DISK_IMAGETYPE)
// in fact we should refactor this
        SF314[floppy_current_drive()].ImageType.Extension==EXT_STX)
#else
        SF314[floppy_current_drive()].ImageType==3)
#endif
#endif            
        );
#endif//pasti

#if defined(SS_DISK_STW)
      can_send=can_send && SF314[drive].ImageType.Extension!=EXT_STW;
#endif

      //if(!can_send) TRACE("Not native\n");

#if defined(SS_DISK_GHOST)
      if(!can_send && SSE_GHOST_DISK) //should we intercept?
      {
#if defined(SS_DISK_GHOST_SECTOR)

        TWD1772IDField IDField;
        IDField.track=TR;
        IDField.side=floppy_current_side();
        IDField.num=SR; 

        // WRITE 1 SECTOR

        if((io_src_b&0xF0)==0xA0)
        {
          CR=io_src_b; //update this...
          if(!SF314[DRIVE].State.Ghost) // need to open ghost image?
          {
            EasyStr STGPath=FloppyDrive[DRIVE].GetImageFile();//SS_PASTI_NO_RESET
            STGPath+=".STG"; 
            if(GhostDisk[DRIVE].Open(STGPath.Text))
              SF314[DRIVE].State.Ghost=1; 
          }

          WORD nbytes=Dma.Counter*512; //guess
          
          // we don't fill in len & CRC
          IDField.track=TR;
          IDField.side=floppy_current_side();
          IDField.num=SR; 

          // bytes ST memory -> our buffer
          for(int i=0;i<nbytes;i++)
          {
            *(GhostDisk[DRIVE].SectorData+i)=PEEK(Dma.BaseAddress);

#if defined(SS_DEBUG_TRACE_CONTROL)
            if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
            {
#if defined(SS_DMA_TRACK_TRANSFER)
              if( ! (i%16) )
              {//TODO we know the direction
                Dma.Datachunk++; 
                if(!(Dma.MCR&0x100)) // disk -> RAM
                  TRACE("\n#%03d (%d-%02d-%02d) to %06X: ",Dma.Datachunk,floppy_current_side(),floppy_head_track[DRIVE],fdc_sr,Dma.BaseAddress);
                else  // RAM -> disk
                  TRACE("\n#%03d (%d-%02d-%02d) from %06X: ",Dma.Datachunk,floppy_current_side(),floppy_head_track[DRIVE],fdc_sr,Dma.BaseAddress);
              }
              TRACE("%02x ",*(GhostDisk[DRIVE].SectorData+i));
#endif
            }
#endif

            Dma.IncAddress();
          }
#if defined(SS_DEBUG_TRACE_CONTROL)
          if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
            TRACE("\n");
#endif
          if(nbytes)
            GhostDisk[DRIVE].WriteSector(&IDField);
          STR=FDC_STR_MOTOR_ON;
          WD1772.CommandWasIntercepted=1;
          agenda_fdc_finished(0);
        }

        // READ 1 SECTOR

        if((io_src_b&0xF0)==0x80)
        {          
          // need to open ghost image?
          if(!SF314[DRIVE].State.Ghost) 
          {
            EasyStr STGPath=FloppyDrive[DRIVE].GetImageFile();//SS_PASTI_NO_RESET
            STGPath+=".STG"; 
            if(Exists(STGPath)) // else it would create STG file
            {
              if(GhostDisk[drive].Open(STGPath.Text))
                SF314[drive].State.Ghost=1; 
            }
          }

          // sector is in ghost image?
          if(SF314[drive].State.Ghost 
            && GhostDisk[drive].ReadSector(&IDField))
          {
            CR=io_src_b; //update this...
            STR=FDC_STR_MOTOR_ON;
            for(int i=0;i<Dma.Counter*512;i++)
            {
              PEEK(Dma.BaseAddress)=*(GhostDisk[drive].SectorData+i);

#if defined(SS_DEBUG_TRACE_CONTROL)
              if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
              {
#if defined(SS_DMA_TRACK_TRANSFER)
                if( ! (i%16) )
                {
                  Dma.Datachunk++; // !!!!!!!!! debug,boiler,"bytes" only!!!!
                  ASSERT(!(Dma.MCR&0x100)); // disk -> RAM
                  TRACE("\n#%03d (%d-%02d-%02d) to %06X: ",Dma.Datachunk,floppy_current_side(),floppy_head_track[DRIVE],fdc_sr,Dma.BaseAddress);
                }
                TRACE("%02x ",*(GhostDisk[DRIVE].SectorData+i));
#endif
              }
#endif
              Dma.IncAddress();
            }            
#if defined(SS_DEBUG_TRACE_CONTROL)
            if(TRACE_MASK3 & TRACE_CONTROL_FDCBYTES)
              TRACE("\n");
#endif
            
            WD1772.CommandWasIntercepted=1;
            agenda_fdc_finished(0); 

#if defined(SS_DISK_GHOST_SECTOR_STX1)
#if USE_PASTI
            if(hPasti && pasti_active && SF314[drive].ImageType.Manager==MNGR_PASTI)
            {
              ASSERT( !PASTI_JUST_STX || SF314[drive].ImageType.Extension==EXT_STX );
//              TRACE("write updated dma address in pasti : %x\n",Dma.BaseAddress);
              pasti_update_reg(0xff8609,(Dma.BaseAddress&0xff0000)>>16);
              pasti_update_reg(0xff860b,(Dma.BaseAddress&0xff00)>>8);
              pasti_update_reg(0xff860d,(Dma.BaseAddress&0xff));
              
              //temp, debug
              pastiPEEKINFO ppi;
              pasti->Peek(&ppi);
  //            TRACE("pasti BaseAddress=%x\n",ppi.dmaBase);
            }
#endif//#if USE_PASTI 
#endif//#if defined(SS_DISK_GHOST_SECTOR_STX1)

          }
        }
#endif
      }
#endif//#if defined(SS_DISK_GHOST)

      if(can_send)
        floppy_fdc_command(io_src_b); // in fdc.cpp
//      else  TRACE_LOG("FDC %X not native, drive %C type %d\n",io_src_b,'A'+YM2149.Drive(),SF314[YM2149.Drive()].ImageType);

#if defined(SS_DISK_STW)
      if(SF314[drive].ImageType.Extension==EXT_STW)
        WriteCR(io_src_b);
#endif


    }
    break;

  case 1: // TR
#if defined(SS_DEBUG_TRACE_CONTROL)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
    TRACE_LOG("FDC TR %d\n",io_src_b);
#endif
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
#if defined(SS_DEBUG_TRACE_CONTROL)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
    TRACE_LOG("FDC SR %d (pc %x)\n",io_src_b,old_pc );
#endif
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
#if defined(SS_DEBUG_TRACE_CONTROL)
  if(TRACE_MASK3 & TRACE_CONTROL_FDCREGS)
    TRACE_LOG("FDC DR %d\n",io_src_b);
#endif
    log_to(LOGSECTION_FDC,EasyStr("FDC: ")+HEXSl(old_pc,6)+" - Setting FDC data register to "+io_src_b);
    DR=io_src_b;
    break;
  }
  
  // IPF handling
#if defined(SS_IPF)

  if(Caps.IsIpf(drive)) 
#if defined(SS_DISK_GHOST)
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


#if defined(SS_FDC_RESET)
void TWD1772::Reset(bool Cold) {
  STR=2;
#ifdef SS_FDC_FORCE_INTERRUPT
  InterruptCondition=0;
#endif
#if defined(SS_FDC_INDEX_PULSE_COUNTER)
  IndexCounter=0;
#endif

#if defined(SS_DISK_GHOST)
  CommandWasIntercepted=0;
#endif

}
#endif


#if defined(SS_DEBUG) || defined(SS_OSD_DRIVE_LED)
int TWD1772::WritingToDisk() {
  return((CR&0xF0)==0xF0 || (CR&0xE0)==0xA0);
}
#endif


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
    if(type==1 || type==4) 
      TRACE_LOG("IP "); // index
    else
      TRACE_LOG("DRQ "); // data request
  if(STR&0x01)
    TRACE_LOG("BSY "); // busy
    TRACE_LOG(") "); 
}

#endif//debug

#if defined(SS_DISK_STW)

/*  Managing STW images, a nice challenge!
*/
void  TWD1772::WriteCR(BYTE io_src_b) {
  if(CommandType(io_src_b)==2 || CommandType(io_src_b)==3)
    ImageSTW[DRIVE].LoadTrack(floppy_current_side(),floppy_head_track[DRIVE]);
  floppy_fdc_command(io_src_b); // integrate in native?
}


WORD TWD1772::ByteOfNextID(WORD current_byte) {
  ASSERT( current_byte <= 6300 );
  WORD next_id=0; // 0 means error as it's IP
  BYTE drive=DRIVE;
  if(SF314[drive].ImageType.Extension==EXT_STW && ImageSTW[drive].TrackData)
  {
    ASSERT( ImageSTW[drive].CurrentTrack==floppy_head_track[drive] );
    for(WORD i=current_byte;i<ImageSTW[drive].TrackBytes;i++)
    {
    }
  }
  return next_id;
}

#endif


#endif//SS_FDC
