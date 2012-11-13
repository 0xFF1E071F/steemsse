#include "SSE.h"

#if defined(STEVEN_SEAGAL) && defined(SS_FDC_IPF)
/* Support for IPF file format using the WD1772 emulator included in 
   CAPSimg.dll. 
   DMA transfers aren't handled by this emulator, so we use the existing
   Steem system.
*/

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"

TCaps Caps; // our interface with CAPSimg.dll (+CapsPlug in 3rd party)
CapsDrive SF314[2]; // the name of the drive, used by IPF emu
CapsFdc WD1772; // the name of the chip, used by IPF emu

TCaps::TCaps() {
//  VERIFY( Init() );
Init();
//logsection_enabled[0];
}


TCaps::~TCaps() {
  CapsExit();
}


TCaps::Init() {
  Active=DriveIsIPF[0]=DriveIsIPF[1]=FALSE;
  ContainerID[0]=ContainerID[1]=-1;
  LockedSide[0]=LockedSide[1]=-1;
  LockedTrack[0]=LockedTrack[1]=-1; 

  CapsInit("CAPSImg.dll");

  // drives
  SF314[0].type=SF314[1].type=sizeof(CapsDrive); // must be >=sizeof(CapsDrive)
  SF314[0].rpm=SF314[1].rpm=CAPSDRIVE_35DD_RPM;
  SF314[0].maxtrack=SF314[1].maxtrack=CAPSDRIVE_35DD_HST;

  // controller
  WD1772.type=sizeof(CapsFdc);  // must be >=sizeof(CapsFdc)
  WD1772.model=cfdcmWD1772;
  WD1772.clockfrq=8000000;//8021248;//? CPU speed?
  WD1772.drive=SF314;
  WD1772.drivecnt=WD1772.drivemax=2; 

  Initialised=!CapsFdcInit(&WD1772); // OK->0
  if(!Initialised)
    TRACE("CapsFdcInit failed, no CAPS support\n");

  // the DLL will call them, strange that they're erased at FDC init
  WD1772.cbdrq=CallbackDRQ;
  WD1772.cbirq=CallbackIRQ;
  WD1772.cbtrk=CallbackTRK;

  return Initialised;
}


/*  Callback functions. Since they're static, they access object data like
    any external function, using 'Caps.'
*/
#define LOGSECTION LOGSECTION_FDC_BYTES

void TCaps::CallbackDRQ(PCAPSFDC pc, UDWORD setting) {
  // All data bytes combined by the controller come here, one by one
  ASSERT( dma_mode&0x80 ); // the WD1772 emu doesn't handle writing
  if(dma_mode&0x80) // disk->RAM
  {
    BYTE data_register=CapsFdcGetInfo(cfdciR_Data, &WD1772,0); // get byte
    fdc_read_address_buffer[dma_bytes_written_for_sector_count++]=data_register;
    if(dma_bytes_written_for_sector_count==16)
    {
#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG)
      if(TRACE_ENABLED) fdc_report_regs();
#endif
      for(int i=0;i<16;i++) // burst, 16byte packets
      {
        TRACE_LOG("%02X ",fdc_read_address_buffer[i]);
        if (DMA_ADDRESS_IS_VALID_W)
          PEEK(dma_address)=fdc_read_address_buffer[i]; 
        DMA_INC_ADDRESS; // use Steem's existing routine
      }
      TRACE_LOG("\n");
      dma_bytes_written_for_sector_count=0;
      disk_light_off_time=timeGetTime()+DisableDiskLightAfter;
    }
  }
  WD1772.lineout&=~CAPSFDC_LO_DRQ;
  WD1772.r_st1&=~CAPSFDC_SR_IP_DRQ; // The Pawn
}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_FDC

void TCaps::CallbackIRQ(PCAPSFDC pc, DWORD lineout) {
  // IRQ is generally ignored on the ST
  ASSERT( lineout&CAPSFDC_LO_INTRQ );
  ASSERT(!mfp_interrupt_enabled[7]); 
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
  // strangely it's our job to change track and update all variables,
  // maybe because there are different ways
  ASSERT(!drive||drive==1);
  ASSERT(Caps.DriveIsIPF[drive]);
  CapsTrackInfoT2 track_info;
  track_info.type=2; 
  UDWORD flags=DI_LOCK_DENALT|DI_LOCK_DENVAR|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;
  int side=SF314[drive].side;
  int track=SF314[drive].track;
  if(Caps.LockedTrack[drive]!=-1)
  {
    ASSERT( track!=Caps.LockedTrack[drive] );
    VERIFY( !CapsUnlockTrack(Caps.ContainerID[drive],Caps.LockedTrack[drive],
      Caps.LockedSide[drive]) );
    TRACE_LOG("CAPS Unlock track ID %d side %d track %d\n",Caps.ContainerID[drive],Caps.LockedSide[drive],Caps.LockedTrack[drive]);
  }
  VERIFY( !CapsLockTrack((PCAPSTRACKINFO)&track_info,Caps.ContainerID[drive],track,side,flags) );
  ASSERT( side==track_info.head );
  ASSERT( track==track_info.cylinder );
  ASSERT( !track_info.sectorsize );
  ASSERT( !track_info.weakcnt ); // Dungeon Master?
  ASSERT( !track_info.timebuf ); // asserts with DI_LOCK_DENAUTO
  TRACE_LOG("CAPS Lock Track ID %d side %d track %d flags %X\n",Caps.ContainerID[drive],side,track,flags); 
  TRACE_LOG("#sectors %d ",track_info.sectorcnt);
  TRACE_LOG("tracklen %d overlap  %d startbit %d timebuf %x\n",track_info.tracklen,track_info.overlap,track_info.startbit,track_info.timebuf);
  SF314[drive].trackbuf=track_info.trackbuf;
  SF314[drive].timebuf=track_info.timebuf;
  SF314[drive].tracklen = track_info.tracklen;
  SF314[drive].overlap = track_info.overlap;
  Caps.LockedSide[drive]=side;
  Caps.LockedTrack[drive]=track;
#if defined(SS_DEBUG)  
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

