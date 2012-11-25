#include "SSE.h"

#if defined(STEVEN_SEAGAL) && defined(SS_FDC_IPF)
/* Support for IPF file format using the WD1772 emulator included in 
   CAPSimg.dll (Caps library).
   We also extended "CapsPlug" in the third party folder because it had nothing
   to interface with the WD1772 emulation.
   DMA transfers aren't handled by this emulator, contrary to Pasti, so we 
   use the existing Steem system.
   Part of the difficulty is to know which variables of the DLL we need to 
   update. For example, when we insert a disk image, 'diskattr' of the 
   CapsDrive structure must be ored with CAPSDRIVE_DA_IN, or the emulator will 
   not work at all. It got me stuck for a long time. See floppy_drive.cpp.
*/

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"

TCaps Caps; 
CapsDrive SF314[2]; // the name of the drive, used by IPF emu
CapsFdc WD1772; // the name of the chip, used by IPF emu


TCaps::TCaps() {
  VERIFY( Init()!=NULL );
}


TCaps::~TCaps() {
  CapsExit();
}


TCaps::Init() {

  Active=DriveIsIPF[0]=DriveIsIPF[1]=FALSE;
  ContainerID[0]=ContainerID[1]=-1;
  LockedSide[0]=LockedSide[1]=-1;
  LockedTrack[0]=LockedTrack[1]=-1; 
  CyclesRun=0;
  CapsInit("CAPSImg.dll");

  // drives
  SF314[0].type=SF314[1].type=sizeof(CapsDrive); // must be >=sizeof(CapsDrive)
  SF314[0].rpm=SF314[1].rpm=CAPSDRIVE_35DD_RPM;
  SF314[0].maxtrack=SF314[1].maxtrack=CAPSDRIVE_35DD_HST;

  // controller
  WD1772.type=sizeof(CapsFdc);  // must be >=sizeof(CapsFdc)
  WD1772.model=cfdcmWD1772;
  WD1772.clockfrq=8000000; //? CPU speed?
  WD1772.drive=SF314; // ain't it cool?
  WD1772.drivecnt=2;
  WD1772.drivemax=0;

  Initialised=!CapsFdcInit(&WD1772); // OK->0
  if(!Initialised)
    TRACE("CapsFdcInit failed, no IPF support\n");

  // the DLL will call them, strange that they're erased at FDC init
  WD1772.cbdrq=CallbackDRQ;
  WD1772.cbirq=CallbackIRQ;
  WD1772.cbtrk=CallbackTRK;

  return Initialised;
}


bool TCaps::WritePsgA(int data) {
  // We do this here to imitate Pasti and to unload iow.cpp.
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


/*  Callback functions. Since they're static, they access object data like
    any external function, using 'Caps.'
*/

#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC_BYTES

void TCaps::CallbackDRQ(PCAPSFDC pc, UDWORD setting) {
/*  All data bytes combined by the controller come here, one by one.
    This happens between the disk controller and the DMA circuit.
    On a real ST, there are two 16bytes buffer to avoid overlap, and some delay
    before the transfer starts as the DMA chip must request bus access.
    In this emulation, there's only one 16byte buffer and transfer is
    immediate, but we do count some cycles for the transfer. 
    Only DISK->RAM transfers are handled.
*/
  ASSERT( dma_mode&0x80 ); // the WD1772 emu doesn't handle writing
  if(dma_mode&0x80) // disk->RAM
  {
    BYTE data_register=CapsFdcGetInfo(cfdciR_Data, &WD1772,0); // get byte
    fdc_read_address_buffer[fdc_read_address_buffer_len++]=data_register;
    ASSERT( fdc_read_address_buffer_len<=16 );
    if(fdc_read_address_buffer_len==16)
    {
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
      TRACE_LOG("IPF DRQ ");
      if(TRACE_ENABLED) fdc_report_regs();
      TRACE_LOG("to %X: ",dma_address);
#endif
      for(int i=0;i<16;i++) // burst, 16byte packets
      {
        TRACE_LOG("%02X ",fdc_read_address_buffer[i]);
        if (DMA_ADDRESS_IS_VALID_W)
          PEEK(dma_address)=fdc_read_address_buffer[i]; 
        DMA_INC_ADDRESS; // use Steem's existing routine
      }
      INSTRUCTION_TIME(8); // it doesn't seem to change anything but why not?
      TRACE_LOG("\n");
      fdc_read_address_buffer_len=0;
      disk_light_off_time=timeGetTime()+DisableDiskLightAfter;
    }
    WD1772.r_st1&=~CAPSFDC_SR_IP_DRQ; // The Pawn
    WD1772.lineout&=~CAPSFDC_LO_DRQ;
  }
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

void TCaps::CallbackIRQ(PCAPSFDC pc, DWORD lineout) {
/*  The assigned IRQ (7) is generally disabled on the ST, but the corresponding
    bit (5) in the  MFP GPIP register (IO address $FFFA01) may be polled 
    instead.
*/  
  //ASSERT( lineout&CAPSFDC_LO_INTRQ );// Manoir de Mortevielle
  ASSERT( !mfp_interrupt_enabled[7] ); 
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
  TRACE_LOG("IPF IRQ ");
  if(TRACE_ENABLED) fdc_report_regs();
#endif
  mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,!(lineout&CAPSFDC_LO_INTRQ));
  disk_light_off_time=timeGetTime()+DisableDiskLightAfter;
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_IPF_LOCK_INFO

void TCaps::CallbackTRK(PCAPSFDC pc, UDWORD drive){
/*  Strangely it's our job to change track and update all variables,
    maybe because there are different ways (memory, track by track...)?
*/
  ASSERT( !drive||drive==1 );
  ASSERT( Caps.DriveIsIPF[drive] );
  ASSERT( drive==floppy_current_drive() );
  CapsTrackInfoT2 track_info;
  track_info.type=2; 
  UDWORD flags=DI_LOCK_DENALT|DI_LOCK_DENVAR|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;
  int side=SF314[drive].side;
  ASSERT( side==!(psg_reg[PSGR_PORT_A]& BIT_0) );
  int track=SF314[drive].track;
  if(Caps.LockedTrack[drive]!=-1)
  {
    ASSERT( Caps.LockedSide[drive]!=-1 );
    ASSERT( track!=Caps.LockedTrack[drive] || side!=Caps.LockedSide[drive] );
    VERIFY( !CapsUnlockTrack(Caps.ContainerID[drive],Caps.LockedTrack[drive],
      Caps.LockedSide[drive]) );
    TRACE_LOG("CAPS Unlock track %d-%d\n",side,Caps.LockedSide[drive],Caps.LockedTrack[drive]);
  }
  VERIFY( !CapsLockTrack((PCAPSTRACKINFO)&track_info,Caps.ContainerID[drive],
    track,side,flags) );
  ASSERT( side==track_info.head );
  ASSERT( track==track_info.cylinder );
  ASSERT( !track_info.sectorsize );
  ASSERT( !track_info.weakcnt ); // Dungeon Master?
  ASSERT( !track_info.timebuf ); // asserts with DI_LOCK_DENAUTO lock flag
  TRACE_LOG("CAPS Lock Track %d-%d flags %X sectors %d bits %d overlap %d startbit %d timebuf %x\n",side,track,flags,track_info.sectorcnt,track_info.tracklen,track_info.overlap,track_info.startbit,track_info.timebuf);
  SF314[drive].trackbuf=track_info.trackbuf;
  SF314[drive].timebuf=track_info.timebuf;
  SF314[drive].tracklen = track_info.tracklen;
  SF314[drive].overlap = track_info.overlap;
  Caps.LockedSide[drive]=side;
  Caps.LockedTrack[drive]=track;

#if defined(SS_FDC_TRACE_IPF_SECTORS)  // debug info
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

#endif

