#include "SSE.h"
#if defined(STEVEN_SEAGAL) && defined(SSE_ACSI)
/*  This is based on "Atari ACSI/DMA Integration guide", June 28, 1991
    (no copy/paste of code this time, it's for some fun).
    Emulation is straightforward: just fetch sector #n, all seem to
    be 512bytes. 
    It was more difficult to adapt the GUI (hard disk manager...).
*/

#include "../pch.h"

#include <mfp.decla.h>
#include <run.decla.h>
#include <mymisc.h> //GetFileLength()
#if defined(SSE_ACSI_LED) && defined(SSE_OSD_DRIVE_LED)
#include <osd.decla.h>
#include <run.decla.h>
#endif
#if defined(SSE_ACSI_TIMING)
#include <steemh.decla.h> //cpu_timer,cpu_cycles
#include <fdc.decla.h>
#endif
#include "SSEAcsi.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"

#if defined(SSE_ACSI_MULTIPLE)
TAcsiHdc AcsiHdc[TAcsiHdc::MAX_ACSI_DEVICES]; // several, going beyond pasti
#else
TAcsiHdc AcsiHdc; // only one
#endif

#include <cpu.decla.h>

#define BLOCK_SIZE 512 // fortunately it seems constant

#if defined(SSE_ACSI_MULTIPLE)
BYTE acsi_dev=0; // active device, ugly global, private to this file
#endif

TAcsiHdc::TAcsiHdc() {
  hard_disk_image=NULL;
  Active=false;
}


TAcsiHdc::~TAcsiHdc() {
  CloseImageFile();
}


void TAcsiHdc::CloseImageFile() {
  if(hard_disk_image)
    fclose(hard_disk_image);
  hard_disk_image=NULL;
  Active=false;
}

#if defined(SSE_ACSI_FORMAT)

void TAcsiHdc::Format() { 
/*  For fun. Fill full image with $6C.
    We do this sector by sector because otherwise it can be really slow
    and Steem looks hanged ("not responding") for a while, and because 
    we dont want to add agendas on the other hand.
*/
  BYTE sector[BLOCK_SIZE];
  memset(sector,0x6c,BLOCK_SIZE);
  ASSERT(hard_disk_image);
  fseek(hard_disk_image,0,SEEK_SET); //restore
  ASSERT(nSectors>0);
  for(int i=0;i<nSectors;i++)
    fwrite(sector,BLOCK_SIZE,1,hard_disk_image); //fill sectors
}

#endif

bool TAcsiHdc::Init(int num, char *path) {
  ASSERT(num<MAX_ACSI_DEVICES);
  CloseImageFile();
#if defined(SSE_ACSI_INQUIRY2)
  ASSERT(inquiry_string);
  memset(inquiry_string,0,32);
#endif
  hard_disk_image=fopen(path,"rb+");
  Active=(hard_disk_image!=NULL); // file is there or not
  if(Active) // note it could be anything, even HD6301V1ST.img ot T102.img
  {
    int l=GetFileLength(hard_disk_image); //in bytes - int is enough
    nSectors=l/BLOCK_SIZE;
    ASSERT(!(l%BLOCK_SIZE) && nSectors>=20480 && device_num>=0 && device_num<MAX_ACSI_DEVICES); // but we take it?
    device_num=num&7;
#if defined(SSE_ACSI_INQUIRY2)
    char *filename=GetFileNameFromPath(path);
    char *dot=strrchr(filename,'.');
    int nchars=dot?(dot-filename):23;
    ASSERT(nchars>0);
    ASSERT(inquiry_string);
    strncpy(inquiry_string+8,filename,nchars);
    TRACE_HDC("ACSI %d init %s %d sectors %d MB\n",device_num,inquiry_string+8,nSectors,nSectors/(2*1024));
#endif
#if defined(SSE_ACSI_MULTIPLE)
    acsi_dev=device_num;
#endif
  }
  //TRACE_INIT("ACSI %d open %s %d sectors %d MB\n",device_num,path,nSectors,nSectors/(2*1024));
  return (bool)Active;
}


BYTE TAcsiHdc::IORead(BYTE Line) {
  BYTE ior_byte=0;
  if((Dma.MCR&0xFF)==0x8a) // "read status"
    ior_byte=STR;
  //TRACE_HD("ACSI PC %X read %X %X = %X\n",old_pc,Line,Dma.MCR,ior_byte);
  Irq(false);
#if defined(SSE_ACSI_TIMING) 
  Active=1;
#endif
  return ior_byte;
}


void TAcsiHdc::IOWrite(BYTE Line,BYTE io_src_b) {
  if(!hard_disk_image)
    return;
  //TRACE_HD("ACSI PC %X write %X = %X\n",old_pc,Line,io_src_b);
  bool do_irq=false;
  // take new command only if A1 is low, it's our ID and we're ready
  // A1 in ACSI doc is A0 in DMA doc
  if(!Line && (io_src_b>>5)==device_num && cmd_ctr==7)
  {
    cmd_ctr=0;
    io_src_b&=0x1f;
#if defined(SSE_ACSI_MULTIPLE)
    acsi_dev=device_num; // we have the bus
    ASSERT(acsi_dev<MAX_ACSI_DEVICES);
#endif
  }
  if(cmd_ctr<6) // getting command
  {
    cmd_block[cmd_ctr]=io_src_b;
    ASSERT( Line || !cmd_ctr); //asserts on buggy drivers, but it should work if there's only 1 device
    cmd_ctr++;
    do_irq=true;
  }
  if(cmd_ctr==6) // command in
  {
    TRACE_HDC("ACSI %d command §%X (%X %X %X %X %X)\n",device_num,cmd_block[0],cmd_block[1],cmd_block[2],cmd_block[3],cmd_block[4],cmd_block[5]);
#if defined(SSE_DMA_TRACK_TRANSFER2)
    Dma.Datachunk=0;
#endif
    STR=0; // all fine
    switch(*cmd_block)
    {
      case 0x00: //ready
        break;

      case 0x03: //request sense 
#if defined(SSE_ACSI_REQUEST_SENSE)
        DR=error_code;
        Dma.Drq();
        DR=0;
        Dma.Drq();
        Dma.Drq();
        Dma.Drq();
#endif
        break;

      case 0x04: //format
#if defined(SSE_ACSI_FORMAT)
        Format();
#endif
        break;

      case 0x08: //read
#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM) && defined(SSE_ACSI_BOOTCHECKSUM)
        SF314[DRIVE].SectorChecksum=0;
#endif
        ReadWrite(false,cmd_block[4]);
#if defined(SSE_DRIVE_COMPUTE_BOOT_CHECKSUM) && defined(SSE_ACSI_BOOTCHECKSUM)
        if(SF314[DRIVE].SectorChecksum)
          TRACE_HDC("Sector %d checksum $%X\n",SectorNum(),SF314[DRIVE].SectorChecksum);
        SF314[DRIVE].SectorChecksum=0;
#endif
        break;

      case 0x0a: //write
        ReadWrite(true,cmd_block[4]);
        break;

      case 0x0b: //seek
        Seek();
        break;

      case 0x12: //inquiry
#if defined(SSE_ACSI_INQUIRY) 
        Inquiry();
#else
        STR=2;  // "not supported"
#endif
        break;

#if defined(SSE_ACSI_MODESELECT) 
      case 0x15: //SCSI mode select
        {
          TRACE_HDC("Mode select (%d) %d %x\n",cmd_block[4],Dma.Counter,Dma.BaseAddress);
          for(int i=0;i<cmd_block[4];i++)
            Dma.Drq(); //do nothing with it?
        }
        break;
#endif

      default: //other commands
        STR=2;
        error_code=0x20; //invalid opcode
    }//sw
#ifdef SSE_DEBUG
    if(STR&2)
      TRACE_HDC("ACSI error STR %X error code %X\n",STR,error_code);
#endif
    cmd_ctr++;
    ASSERT(cmd_ctr<8);
#if defined(SSE_ACSI_LED) && defined(SSE_OSD_DRIVE_LED)
    HDDisplayTimer=timer+HD_TIMER;
#endif
#if defined(SSE_ACSI_TIMING) // some delay... 1MB/s 512bytes/ 0.5ms
    if(!floppy_instant_sector_access&&(*cmd_block==8 || *cmd_block==0xa)&&!STR)
    {
      time_of_irq=ACT+cmd_block[4]*4000;
      Active=2; // signal for ior
#if defined(SSE_ACSI_LED) && defined(SSE_OSD_DRIVE_LED)
      HDDisplayTimer+=cmd_block[4]/2;
#endif
    }
    else
      Active=1;
#endif
  }
  if(do_irq)
    Irq(true);
}


void TAcsiHdc::Inquiry() {//drivers display this so we have a cool name
#if !defined(SSE_ACSI_INQUIRY2)
  const char inquiry_string[]="STEEM_ACSI";
  TRACE_HDC("Inquiry: %s\n",inquiry_string);
  DR=0; //?
#else
  TRACE_HDC("Inquiry: %s\n",inquiry_string+8); //strange...
#endif
  for(int i=0;i<32;i++)
  {
#if !defined(SSE_ACSI_INQUIRY2)
    if(i>7 && i<=7+11) // 6 strange...
      DR=inquiry_string[i-8];
#else
    DR=inquiry_string[i];
#endif
    Dma.Drq();
  }
}


void TAcsiHdc::Irq(bool state) {
  mfp_gpip_set_bit(MFP_GPIP_FDC_BIT,!state); // MFP GPIP is "active low"
}


void TAcsiHdc::ReadWrite(bool write,BYTE block_count) {
  ASSERT(block_count);
  TRACE_HDC("%s sectors %d-%d (%d)\n",write?"Write":"Read",SectorNum(),SectorNum()+block_count-1,block_count);
  int ok=Seek(); // read/write implies seek
  for(int i=0;ok&&i<block_count;i++)
  {
    for(int j=0;ok&&j<BLOCK_SIZE;j++)
    {
      ASSERT(hard_disk_image);
      if(write)
      {
        Dma.Drq(); // get byte write from DMA
        ok=fwrite(&DR,1,1,hard_disk_image);
      }
      else if(ok=fread(&DR,1,1,hard_disk_image)) // fails when driver tests size
        Dma.Drq(); // put byte on DMA - if there is one
    }//j
  }//i
  if(!ok)
    STR=2;
}


void TAcsiHdc::Reset(bool Cold) {
  cmd_ctr=7; // "ready"; we don't restore
}


int TAcsiHdc::SectorNum() {
  int block_number=(cmd_block[1]<<16) + (cmd_block[2]<<8) + cmd_block[3];
  //block_number&=0x1FFFFF; //limit is?
  return block_number;
}


bool TAcsiHdc::Seek() {
 int block_number=SectorNum();
 if(fseek(hard_disk_image,block_number*BLOCK_SIZE,SEEK_SET))
   STR=2;
 return (STR!=2); // that would mean "OK"
}

#endif//#if defined(STEVEN_SEAGAL) && defined(SSE_ACSI)
