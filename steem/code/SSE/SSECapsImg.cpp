#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_STRUCTURE_SSEFLOPPY_OBJ)
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

#if defined(SSE_DRIVE_IPF1)
#include <stemdialogs.h>//temp...
#include <diskman.decla.h>
#endif

#if !defined(SSE_CPU)
#include <mfp.decla.h>
#endif

#endif//#if defined(SSE_STRUCTURE_SSEFLOPPY_OBJ)

#include "SSECapsImg.h"
#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"
#if defined(SSE_DISK_GHOST)
#include "SSEGhostDisk.h"
#endif


#if defined(SSE_IPF) // Implementation of CAPS support in Steem

#define CYCLES_PRE_IO 100 // those aren't
#define CYCLES_POST_IO 100 // used


TCaps::TCaps() {
  CAPSIMG_OK=0; //3.6.3
  // we init in main to keep control of timing
}


TCaps::~TCaps() {
  if(CAPSIMG_OK)
    CAPSExit();
  CAPSIMG_OK=0;//3.6.3
}


void SetNotifyInitText(char*);//forward

#undef LOGSECTION
#define LOGSECTION LOGSECTION_INIT


int TCaps::Init() {

  Active=FALSE;
  DriveMap=Version=0;
  ContainerID[0]=ContainerID[1]=-1;
  LockedSide[0]=LockedSide[1]=-1;
  LockedTrack[0]=LockedTrack[1]=-1; 
#if defined(SSE_IPF_RUN_PRE_IO) || defined(SSE_IPF_RUN_POST_IO)
  CyclesRun=0;
#endif
#if defined(SSE_VAR_NOTIFY)
  SetNotifyInitText(SSE_IPF_PLUGIN_FILE);
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

  CAPSIMG_OK= (Version>0);
  // controller init
  WD1772.type=sizeof(CapsFdc);  // must be >=sizeof(CapsFdc)
  WD1772.model=cfdcmWD1772;
  WD1772.clockfrq=SSE_IPF_FREQU; 
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
    if(img_info->platform[i]==ciipAtariST 
#if defined(SSE_IPF_CTRAW) 
#if defined(SSE_DISK_IMAGETYPE)
      || ::SF314[drive].ImageType.Extension!=EXT_IPF
#else
      || ::SF314[drive].ImageType!=DISK_IPF // the other SF314 (confusing)
#endif
#endif
      || SSE_HACKS_ON) //MPS GOlf 'test'
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

void TCaps::WritePsgA(int data) {//TODO use data
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

#if defined(SSE_DEBUG)
  Dma.UpdateRegs();
#endif

#if defined(SSE_IPF_RUN_PRE_IO)
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

#if defined(SSE_IPF_RUN_POST_IO)
  CAPSFdcEmulate(&WD1772,CYCLES_POST_IO);
  CyclesRun+=CYCLES_POST_IO;
#endif
  ASSERT(!(data&0xFF00));
  return data;
}


void TCaps::WriteWD1772(BYTE Line,int data) {

  Dma.UpdateRegs();

#if defined(SSE_IPF_RUN_PRE_IO)
  CAPSFdcEmulate(&WD1772,CYCLES_PRE_IO);
  CyclesRun+=CYCLES_PRE_IO;
#endif  

  if(!Line) // command
  {
    // TODO drive selection problem
    mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,true); // Double Dragon II
#ifdef SSE_DEBUG
    if( (Version==42||Version==50||Version==51) && (data&0xF0)==0xA0)
    {
      ASSERT(!SSE_GHOST_DISK);
      TRACE_LOG("IPF unimplemented command %x\n",data);
    }
#endif
#if defined(SSE_DRIVE_MOTOR_ON_IPF)
    // record time of motor start in hbl (TODO use)
    if(!(::WD1772.STR&FDC_STR_MOTOR_ON)) // assume no cycle run!
    {
      //TRACE_LOG("record hbl %d\n");
      ::SF314[DRIVE].motor_on=true;
      ::SF314[DRIVE].HblOfMotorOn=hbl_count;
    }
#endif

  }

  CAPSFdcWrite(&WD1772,Line,data); // send to DLL

#if defined(SSE_IPF_RUN_POST_IO)
  CAPSFdcEmulate(&WD1772,CYCLES_POST_IO);
  CyclesRun+=CYCLES_POST_IO;
#endif
}


void TCaps::Hbl() {

  // we run cycles at each HBL if there's an IPF file in. Performance OK
#if defined(SSE_SHIFTER)
  ASSERT( Shifter.CurrentScanline.Cycles>100)
#if defined(SSE_IPF_RUN_PRE_IO) || defined(SSE_IPF_RUN_POST_IO)
  ASSERT( Shifter.CurrentScanline.Cycles-Caps.CyclesRun>0 );
#endif
  CAPSFdcEmulate(&WD1772,Shifter.CurrentScanline.Cycles
#if defined(SSE_IPF_RUN_PRE_IO) || defined(SSE_IPF_RUN_POST_IO)
    -CyclesRun
#endif
    );
#else
  CAPSFdcEmulate(&WD1772,screen_res==2? 224 : 512);
#endif

#if defined(SSE_IPF_RUN_PRE_IO) || defined(SSE_IPF_RUN_POST_IO)
  CyclesRun=0;
#endif

}

int TCaps::IsIpf(BYTE drive) {
  return (DriveMap&((drive&1)+1)); // not <<!
}


/*  Callback functions. Since they're static, they access object data like
    any external function, using 'Caps.'
*/

void TCaps::CallbackDRQ(PCAPSFDC pc, UDWORD setting) {
#if defined(SSE_DEBUG) || !defined(VC_BUILD)
  Dma.UpdateRegs();
  ASSERT( Dma.MCR&BIT_7 ); // DMA enabled
  ASSERT(!(Dma.MCR&BIT_6)); // DMA enabled
  ASSERT(!(Dma.MCR&BIT_3)); // Floppy
#endif
  if((Dma.MCR&BIT_7) //&& !(Dma.MCR&BIT_6) //3.7.0, 
)
  {
#if defined(SSE_DMA_DRQ)
    ::WD1772.DR=Caps.WD1772.r_data;
    Dma.Drq();
    Caps.WD1772.r_data=::WD1772.DR;
#else
    // transfer one byte
    if(!(Dma.MCR&BIT_8)) // disk->RAM
      Dma.AddToFifo( CAPSFdcGetInfo(cfdciR_Data,&Caps.WD1772,0) );
    else // RAM -> disk
      Caps.WD1772.r_data=Dma.GetFifoByte();  
#endif

    Caps.WD1772.r_st1&=~CAPSFDC_SR_IP_DRQ; // The Pawn
    Caps.WD1772.lineout&=~CAPSFDC_LO_DRQ;
  }
}


void TCaps::CallbackIRQ(PCAPSFDC pc, DWORD lineout) {
  ASSERT(pc==&Caps.WD1772);

#if defined(SSE_DEBUG)
  if(TRACE_ENABLED) 
  {
    TRACE("caps ");
    Dma.UpdateRegs(true);

  }
    else
#endif
    Dma.UpdateRegs(); // why it only worked in boiler, log on...
  
#if defined(SSE_DRIVE_SOUND)
  if(SSEOption.DriveSound)
  {
#if defined(SSE_DRIVE_SOUND_IPF)
    Dma.UpdateRegs(); // why it only worked in boiler, log on...
#endif
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
    ::SF314[DRIVE].Sound_CheckIrq();
#else
    ::SF314[0].Sound_CheckIrq();
#endif
  }
#endif
  mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,!(lineout&CAPSFDC_LO_INTRQ));
#if !defined(SSE_OSD_DRIVE_LED3)
  disk_light_off_time=timeGetTime()+DisableDiskLightAfter;
#endif
}

#if !defined(SSE_DEBUG_TRACE_CONTROL)
#undef LOGSECTION
//#define LOGSECTION LOGSECTION_IPF_LOCK_INFO
#define LOGSECTION LOGSECTION_IMAGE_INFO
#endif

void TCaps::CallbackTRK(PCAPSFDC pc, UDWORD drive) {
  ASSERT( !drive||drive==1 );
  ASSERT( Caps.IsIpf(drive) );
  ASSERT( drive==floppy_current_drive() );

  int side=Caps.SF314[drive].side;
  ASSERT( side==!(psg_reg[PSGR_PORT_A]& BIT_0) );
  int track=Caps.SF314[drive].track;
  CapsTrackInfoT2 track_info; // apparently we must use type 2...
  track_info.type=1;
  UDWORD flags=DI_LOCK_DENALT|DI_LOCK_DENVAR|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;

#if defined(SSE_DRIVE_SINGLE_SIDE_IPF) // wait for IPF feature, not defined
  if( SSEOption.SingleSideDriveMap&(floppy_current_drive()+1) )
    side=0;
#endif
  
  CapsRevolutionInfo CRI;


#if !defined(SSE_IPF_CTRAW_NO_UNLOCK) 
/*  This part is undefined, since unlocking messes internal variables that are
    important for games using "weak bits" protections, such as Outrun, Vroom.
*/

  if(Caps.LockedTrack[drive]!=-1)
  {
    ASSERT( Caps.LockedSide[drive]!=-1 );
//    ASSERT( track!=Caps.LockedTrack[drive] || side!=Caps.LockedSide[drive] );

    VERIFY( !CAPSUnlockTrack(Caps.ContainerID[drive],Caps.LockedTrack[drive],
      Caps.LockedSide[drive]) );
#if defined(SSE_DEBUG_TRACE_CONTROL)
    if(TRACE_MASK3 & TRACE_CONTROL_FDCIPF1)
#endif
    TRACE_LOG("CAPS Unlock %c:S%dT%d\n",drive+'A',Caps.LockedSide[drive],Caps.LockedTrack[drive]);


#if defined(SSE_IPF_CTRAW_REV)
#if defined(SSE_DISK_IMAGETYPE)
    if(::SF314[drive].ImageType.Extension!=EXT_IPF)
#else
    if(::SF314[drive].ImageType!=DISK_IPF)
#endif
    {
      if(Caps.LockedSide[drive]==side && Caps.LockedTrack[drive]==track)
      { // not tested!
        CAPSGetInfo(&CRI,Caps.ContainerID[drive],track,side,cgiitRevolution,0);
        TRACE_LOG("Same track, keep rev %d\n",CRI.next);
      }
      else
      {
        TRACE_LOG("New track, reset rev\n");
        CAPSSetRevolution(Caps.ContainerID[drive],0);
      }
    }
#endif//SSE_IPF_CTRAW_NO_UNLOCK

  }
#endif

#if defined(SSE_IPF_CTRAW_1ST_LOCK)
/*  We've changed track, we reset # revs as recommended by caps authors.
    Up to now we haven't seen the difference (eg Turrican works with or
    without) so we "protect" this with option Hacks.
*/
  if(Caps.LockedSide[drive]!=side || Caps.LockedTrack[drive]!=track)
  {
    if(SSE_HACKS_ON)
      CAPSSetRevolution(Caps.ContainerID[drive],0);
  }
#endif

  VERIFY( !CAPSLockTrack((PCAPSTRACKINFO)&track_info,Caps.ContainerID[drive],
    track,side,flags) );

  CAPSGetInfo(&CRI,Caps.ContainerID[drive],track,side,cgiitRevolution,0);
  TRACE_LOG("max rev %d real %d next %d\n",CRI.max,CRI.real,CRI.next);
  ASSERT( track==track_info.cylinder );
  ASSERT( !track_info.sectorsize );
  TRACE_LOG("CAPS Lock %c:S%dT%d flags %X sectors %d bits %d overlap %d startbit %d timebuf %x\n",
    drive+'A',side,track,flags,track_info.sectorcnt,track_info.tracklen,track_info.overlap,track_info.startbit,track_info.timebuf);
  Caps.SF314[drive].trackbuf=track_info.trackbuf;
  Caps.SF314[drive].timebuf=track_info.timebuf;
  Caps.SF314[drive].tracklen = track_info.tracklen;
  Caps.SF314[drive].overlap = track_info.overlap;
  Caps.SF314[drive].ttype=track_info.type;//?
  Caps.LockedSide[drive]=side;
  Caps.LockedTrack[drive]=track;

#if defined(SSE_IPF_TRACE_SECTORS) &&defined(SSE_DISK_IMAGETYPE) // debug info
  if(::SF314[drive].ImageType.Extension==EXT_IPF)
  {
#if defined(SSE_DEBUG_TRACE_CONTROL) // controlled by boiler now (3.6.1)
    if(TRACE_MASK3&TRACE_CONTROL_FDCIPF2) 
#endif
    {
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
    }
  }
#endif

}

#undef LOGSECTION

#endif//ipf

#endif//#if defined(STEVEN_SEAGAL)
