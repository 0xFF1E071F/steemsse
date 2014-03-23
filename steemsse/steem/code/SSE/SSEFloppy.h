#pragma once
#ifndef SSEFLOPPY_H
#define SSEFLOPPY_H

/*
"The Atari ST uses a WD1772 FDC controller to interface to the Floppy Drive.
 In the Atari system architecture the FDC data and address busses are 
connected to a private bus of the Atari Custom DMA controller and therefore
 the DMA sits between the processor bus and the FDC/HDC.
 This enables the DMA controller to perform automatic DMA transfer between
 the FDC, or an external Hard Disk Controller, and the memory. 
But this also implies that all accesses to the FDC/HDC registers have to be
 done through the DMA controller. The floppy drives interface is also
 connected to three bits of the output Port A of the PSG (YM2149). 
These outputs allow controlling the selection of the drives and the side. 
The interrupt request of the FDC is connected to an input of the MFP (68901)
 general purpose I/O port (GPIO). This allows checking when an FDC/HDC 
command is terminated by either polling this input or by triggering an 
interrupt."
*/

#if defined(SS_DRIVE_SOUND) && defined(SS_STRUCTURE_SSEDEBUG_OBJ)
#ifdef WIN32
#include <dsound.h>
#endif
#endif

#if defined(SS_FLOPPY)
#ifdef SS_DRIVE
#define ADAT (SF314[floppy_current_drive()].Adat())
#else
#define ADAT (!floppy_instant_sector_access)
#endif
#endif

#if defined(SS_DMA)
/*  We use object TDma to refine emulation of the DMA in Steem.
    Steem original variables are defined as the new ones.
*/

#define dma_sector_count Dma.Counter
#define dma_address Dma.BaseAddress
#define dma_bytes_written_for_sector_count Dma.ByteCount
#define dma_mode Dma.MCR
#define dma_status Dma.SR

struct TDma {

  MEM_ADDRESS BaseAddress; // Base Address Register
  WORD MCR; // mode control register
  BYTE SR; // status register
  WORD Counter; // Counter register
  WORD ByteCount; // 1-512 for sectors

  TDma();
  BYTE IORead(MEM_ADDRESS addr);
  void IOWrite(MEM_ADDRESS addr,BYTE io_src_b);
/*  Because Steem runs CAPS and Pasti plugins, DMA/FDC regs are not
    always up-to-date. This function copies the values from CAPS or
    Pasti so that they are. Useful for debug, OSD.
*/
  void UpdateRegs(bool trace_them=false);

#if defined(SS_STRUCTURE_DMA_INC_ADDRESS)
  void IncAddress();
#define DMA_INC_ADDRESS Dma.IncAddress();
#endif

#if defined(SS_DMA_FIFO)

  void AddToFifo(BYTE data);
  BYTE GetFifoByte();

/*  To be able to emulate the DMA precisely if necessary, DMA transfers are
    emulated in two steps. First, they're requested, then they're executed.
    That way we can emulate a little delay before the transfer occurs & the
    toggling of two FIFO buffers at each transfer.
    It proved overkill however, so calling RequestTransfer() will result in
    the immediate call of TransferBytes() (as #defined in SSE.h).
    And only one buffer is used.
*/
  void RequestTransfer();
  void TransferBytes();
  bool Request;
  BYTE Fifo_idx;
#if defined(SS_DMA_DOUBLE_FIFO)
/*
"Internally the DMA has two 16 bytes FIFOs that are used alternatively. 
This feature allows the DMA to continue to receive bytes from the FDC/HDC
 controller while waiting for the processor to read the other FIFO. 
When a FIFO is full a bus request is made to the 68000 and when granted,
 the FIFO is transferred to the memory. This continues until all bytes 
have been transferred."
This feature is emulated in Steem if SS_DMA_DOUBLE_FIFO is defined, but it
hasn't proved necessary yet.
*/
  bool BufferInUse; 
  BYTE Fifo[2][16
#if defined(SS_DMA_FIFO_READ_ADDRESS) && !defined(SS_DMA_FIFO_READ_ADDRESS2)
    +4 // see fdc.h, replaces fdc_read_address_buffer[20]
#endif
    ];
#else
  BYTE Fifo[16
#if defined(SS_DMA_FIFO_READ_ADDRESS) && !defined(SS_DMA_FIFO_READ_ADDRESS2)
    +4 // see fdc.h, replaces fdc_read_address_buffer[20]
#endif
    ];
#endif
#endif//FIFO
#if defined(SS_DMA_DELAY)
/*
"In read mode, when one of the FIFO is full (i.e. when 16 bytes have been
 transferred from the FDC or HDC) the DMA chip performs a bus request to
 the 68000 and in return the processor will grant the control of the bus to
  the DMA, the transfer to the memory is then done with 8 cycles then the 
bus is released to the system. As the processor takes time to grant 
the bus and as transferring data from FIFO to memory also takes time,
 the other FIFO is used for continuing the data transfer with the FD/HD
 controllers."
When SS_DMA_DELAY is defined (normally not), an arbitrary delay 
(SSEParameters.h) is applied before the transfer takes place. 
SS_DMA_DOUBLE_FIFO should also be defined (or the single buffer would be
overwritten).
This feature uses Steem's event system, which is cycle accurate but also 
heavy. It was a test.
*/
  static void Event(); 
  int TransferTime;
#endif

};

extern TDma Dma;

#endif//dma


#if defined(SS_DRIVE)
/* How a floppy disk is structured (bytes/track, gaps...) is handled
   here.
   TODO review, simplify...
   TODO separate native/pasti/etc... 
*/

struct TSF314 {
  TSF314();
  enum {RPM=DRIVE_RPM,MAX_CYL=DRIVE_MAX_CYL,TRACK_BYTES=DRIVE_BYTES_ROTATION};
  bool Adat(); // accurate disk access times
  WORD BytePosition();
  WORD BytePositionOfFirstId();
  WORD BytesToHbls(int bytes);
  DWORD HblsAtIndex();
  WORD HblsNextIndex();
  WORD HblsPerRotation();
  WORD HblsPerSector();
  WORD HblsToBytes(int hbls);
  BYTE nSectors();
  void NextID(BYTE &Id,WORD &nHbls);
  BYTE PostIndexGap();
  BYTE PreDataGap();
  BYTE PostDataGap();
  WORD PreIndexGap();
  WORD RecordLength();
  BYTE SectorGap();
  BYTE Track();
  WORD TrackBytes();
  WORD TrackGap();
  BYTE Id; // object has to know if its A: or B:
#if defined(SS_PASTI_ONLY_STX)
  BYTE ImageType; //TODO use this to make it more general (0 none + nativeST + pasti + ipf...)
//maybe "handler" (steem, pasti...) + type (st,msa,stx...)
#endif
#if defined(SS_DRIVE_MOTOR_ON)
  BYTE MotorOn;
  DWORD HblOfMotorOn;
#endif

#if defined(SS_DRIVE_SOUND)
  enum {START,MOTOR,STEP,SEEK,NSOUNDS} ;
  BYTE TrackAtCommand;
  IDirectSoundBuffer *Sound_Buffer[NSOUNDS]; // fixed array
  void Sound_LoadSamples(IDirectSound *DSObj,DSBUFFERDESC *dsbd,WAVEFORMATEX *wfx);
  void Sound_ReleaseBuffers();
  void Sound_StopBuffers();
  void Sound_CheckCommand(BYTE cr);
  void Sound_CheckIrq();
  void Sound_CheckMotor();
#if defined(SS_DRIVE_SOUND_VOLUME)
  DWORD Sound_Volume;
  void Sound_ChangeVolume();
#endif
#endif//sound

#if defined(SS_DRIVE_COMPUTE_BOOT_CHECKSUM)
  WORD SectorChecksum;
#endif

};

extern TSF314 SF314[2]; // 2 double-sided drives, wow!

#endif//SS_DRIVE


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

struct TWD1772 {
  // Official registers
  BYTE CR;  // command
  BYTE STR; // status
  BYTE TR;  // track
  BYTE SR;  // sector
  BYTE DR;  // data
  // Internal registers
#if defined(SS_FDC_FORCE_INTERRUPT)
  BYTE InterruptCondition;
#endif
#if defined(SS_FDC_INDEX_PULSE_COUNTER)
  BYTE IndexCounter;
#endif
  BYTE IORead(BYTE Line);
  void IOWrite(BYTE Line,BYTE io_src_b);
  BYTE CommandType(int command=-1); // I->IV
#if defined(SS_FDC_RESET)
  void Reset(bool Cold);
#endif

#if defined(SS_DEBUG) || defined(SS_OSD_DRIVE_LED)
/*  This is useful for OSD: if we're writing then we need to display a red
    light (green when reading). This is used by pasti & IPF.
*/
  int WritingToDisk();
#endif
#if defined(SS_DEBUG)
  void TraceStatus();
#endif
};
extern TWD1772 WD1772;

#define fdc_cr WD1772.CR     // problem:
#define fdc_str WD1772.STR   // not identified in debugger
#define fdc_tr WD1772.TR
#define fdc_sr WD1772.SR
#define fdc_dr WD1772.DR
#endif //FDC


#if defined(SS_PSG)
/*  In v3.5.1, object PSG is only used for drive management.
    Drive is 0 (A:) or 1 (B:), but if both relevant bits in
    PSG port A are set then no drive is selected ($FF).
*/

struct TYM2149 {
  enum {NO_VALID_DRIVE=0xFF};
  BYTE Drive(); // may return NO_VALID_DRIVE
  BYTE PortA();
  BYTE Side();
};

extern TYM2149 YM2149;

#endif//PSG


#if defined(SS_IPF)
/* Support for IPF file format using the WD1772 emulator included in 
   CAPSimg.dll (Caps library).

   All calls to the DLL functions are encapsulated here, but there are some
   direct access to CapsFdc variables.
   DMA transfers aren't handled by this emulator, contrary to Pasti, so we 
   use the existing Steem system, improved.
   Part of the difficulty is to know which variables of the DLL we need to 
   update. For example, when we insert a disk image, 'diskattr' of the 
   CapsDrive structure must be ored with CAPSDRIVE_DA_IN, or the emulator will 
   not work at all. It got me stuck for a long time.
   In version 3.5.0, most IPF images should run.
   Known cases: Sundog needs write support, 
   Blood Money v3.5.1 (CPU fix)
   It's perfectly possible to give write support in Steem, but it would be 
   better emulated in the plugin itself. We should do it ourselve only if the
   CAPS WD1772 emu is never completed and if it would bring much to players
   (many disk images available).
   In the current state, only Sundog IPF would benefit, and we have Sundog 
   Pasti, not to mention the disk isn't protected. Not worth the code.
*/

#include <caps/Comtype.h>
#include <caps/CapsAPI.h>
#include <caps/CapsFDC.h>
#include<caps/CapsLib.h>

// our interface with CAPSimg.dll, C++ style (RAII)
struct TCaps {  
  TCaps();
  ~TCaps();
  int Init();
  void Reset();
  void Hbl(); // run the emulator for the HBL (512 cycles at 50hz)
  int InsertDisk(int drive, char *File,CapsImageInfo *img_info);
  int IsIpf(int drive);
  void RemoveDisk(int drive);
  UDWORD ReadWD1772(BYTE Line);
  void WritePsgA(int data);
  void WriteWD1772(BYTE Line,int data);

/*  Using static functions so that there's no 'this' for those
    callback functions.

    1) DRQ (data request)
    If we're reading from the disk, all data bytes combined by the controller
   come here, one by one.
    If we're writing to the disk, everytime the controller is ready to
    translate a byte, this function is called to put a byte in the data
    register.
    The caps library doesn't emulate writes yet, but Steem 3.5 should be
    ready for it.
*/
  static void CallbackDRQ(PCAPSFDC pc, UDWORD setting);


/*  2) IRQ (interrupt request)
    The assigned IRQ (7) is generally disabled on the ST, but the corresponding
    bit (5) in the  MFP GPIP register (IO address $FFFA01) may be polled 
    instead.
*/  
  static void CallbackIRQ(PCAPSFDC pc, UDWORD lineout);


/*  3) Track
    Strangely it's our job to change track and update all variables,
    maybe because there are different ways (memory, track by track...)?
*/
  static void CallbackTRK(PCAPSFDC pc, UDWORD driveact);


  int Version; // 0: failed; else release revision eg 42
  BOOL Active; // if there's an IPF disk in some drive, we must run IPF cycles
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  int CyclesRun; // must be the same for each line
#endif
  // for drive A & B
  BYTE DriveMap; // bit0=drive A bit1=drive B
  SDWORD ContainerID[2]; 
  SDWORD LockedSide[2];
  SDWORD LockedTrack[2]; 
/*  Here for the drives and the controllers, we use the names of the actual
    hardware. Note that Steem SSE also uses those variable names.
*/
  CapsDrive SF314[2]; // 2 double-sided floppy drives
  CapsFdc WD1772; // 1 cheap controller
};

extern TCaps Caps;

#endif//ipf

#if defined(SS_SCP)//TODO
/* SCP images are big, we must decide if we load in memory or not

*/
struct TScpImageInfo {
  BYTE DiskIn;
  EasyStr File; // name of the actual file (may be tmp)
};

struct TScp {

  TScpImageInfo ImageInfo[2];

  int InsertDisk(int drive,char* File);

};

extern TScp Scp;

#endif//scp

#endif//SSEFLOPPY_H
