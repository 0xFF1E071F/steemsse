#pragma once
#ifndef SSEFLOPPY_H
#define SSEFLOPPY_H

/*

DMA = Direct Memory Access, this is a custom Atari chip
FDC = Floppy Disk Controller, this is the WD1772

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

#if defined(SS_DMA)
/*  We use object TDma to refine emulation of the DMA in Steem, but we keep
    using other Steem variables (dma_... and fdc_...).
*/

struct TDma {
  TDma();
#if defined(SS_DMA_IO)
/*  All DMA and FDC fixes of Steem SSE need this to be defined. This part
    takes over and expends the DMA/FDC segments in ior.cpp & iow.cpp, which
    makes those files more readable (now we handle Steem "native", pasti and
    IPF).
*/
  BYTE IORead(MEM_ADDRESS addr);
  void IOWrite(MEM_ADDRESS addr,BYTE io_src_b);
#endif
#if defined(SS_DEBUG)
  void TraceIORead(MEM_ADDRESS addr);
  void TraceIOWrite(MEM_ADDRESS addr,BYTE io_src_b);
#endif
#if defined(SS_DEBUG) || defined(SS_OSD_DRIVE_LED)
/*  When pasti is in charge, Steem dma & fdc variables aren't updated. This
    function updates those variables with pasti values.
    Useful for TRACE and for the red led of drives.
*/
  void UpdateRegs(bool trace_them=false);
#endif  
/*  To be able to emulate the DMA precisely if necessary, DMA transfers are
    emulated in two steps. First, they're requested, then they're executed.
    That way we can emulate a little delay before the transfer occurs & the
    toggling of two FIFO buffers at each transfer.
    It proved overkill however, so calling RequestTransfer() will result in
    the immediate call of TransferBytes() (as defined in SSE.h).
*/
  void RequestTransfer();
  void TransferBytes();
  int Request;
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
  int BufferInUse; 
#endif
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
  int dma_time;
#endif

};

extern TDma Dma;

#endif


#if defined(SS_FDC)

struct TWD1772 {
#if defined(SS_DEBUG) || defined(SS_OSD_DRIVE_LED)
/*  This is useful for OSD: if we're writing then we need to display a red
    light (green when reading). This is used by pasti & IPF.
*/
  int WritingToDisk();
#endif
#if defined(SS_DEBUG)
  int CommandType(int command);
  void TraceStatus();
#endif
  int OldCr;
};
extern TWD1772 WD1772;

#endif

#if defined(SS_IPF)
/* Support for IPF file format using the WD1772 emulator included in 
   CAPSimg.dll (Caps library).
   We also extended "CapsPlug" in the third party folder because it had nothing
   to interface with the WD1772 emulation.
   DMA transfers aren't handled by this emulator, contrary to Pasti, so we 
   use the existing Steem system, improved.
   Part of the difficulty is to know which variables of the DLL we need to 
   update. For example, when we insert a disk image, 'diskattr' of the 
   CapsDrive structure must be ored with CAPSDRIVE_DA_IN, or the emulator will 
   not work at all. It got me stuck for a long time.
   All calls to the DLL are encapsulated in CapsPlug.cpp, all calls to CapsPlug
   functions are encapsulated here, but there are some direct access to CapsFdc
   variables.
   In the current version (3.5.0), most IPF images should run.
   Known cases: Sundog needs write support, Blood Money not sure if it's a disk
   problem.
   It's perfectly possible to give write support in Steem, but it would be 
   better emulated in the plugin itself. We should do it ourselve only if the
   CAPS WD1772 emu is never completed and if it would bring much to players. 
   In the current state, only Sundog IPF would benefit, and we have Sundog 
   Pasti. Not worth the code.
*/

#include <caps/Comtype.h>
#include <caps/CapsAPI.h>
#include <caps/CapsPlug.h>
#include <caps/CapsFDC.h>

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
  UDWORD ReadWD1772();
  void WritePsgA(int data);
  int WriteWD1772(int data);

/*  Using static functions so that there's no 'this'.
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

  static void CallbackIRQ(PCAPSFDC pc, UDWORD lineout);
/*  2) IRQ (interrupt request)
    The assigned IRQ (7) is generally disabled on the ST, but the corresponding
    bit (5) in the  MFP GPIP register (IO address $FFFA01) may be polled 
    instead.
*/  

  static void CallbackTRK(PCAPSFDC pc, UDWORD driveact);
/*  3) Track
    Strangely it's our job to change track and update all variables,
    maybe because there are different ways (memory, track by track...)?
*/

  int Version; // 0: failed; else release revision eg 42
  BOOL Active; // if there's an IPF disk in some drive, we must run IPF cycles
#if defined(SS_IPF_RUN_PRE_IO) || defined(SS_IPF_RUN_POST_IO)
  int CyclesRun; // must be the same for each line
#endif
  // for drive A & B
  int DriveMap; // bit0=drive A bit1=drive B
  SDWORD ContainerID[2]; 
  SDWORD LockedSide[2];
  SDWORD LockedTrack[2]; 
/*  Here for the drives and the controllers, we use the names of the actual
    hardware. Note that Steem SSE also uses its own WD1772 variable.
*/
  CapsDrive SF314[2]; 
  CapsFdc WD1772; 
};

extern TCaps Caps;

#endif

#endif//SSEFLOPPY_H
