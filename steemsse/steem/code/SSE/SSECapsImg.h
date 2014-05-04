#pragma once
#ifndef SSECAPSIMG_H
#define SSECAPSIMG_H

#if defined(SS_IPF)
/* Support for IPF file format using the WD1772 emulator included in 
   CAPSimg.dll (Caps library).

   All calls to the DLL functions are encapsulated here, but there are some
   direct access to CapsFdc variables.
   DMA transfers aren't handled by this emulator, contrary to Pasti, so we 
   use the existing Steem system, improved.
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
  //TODO: save space
  // for drive A & B
  BYTE DriveMap; // bit0=drive A bit1=drive B
  SDWORD ContainerID[2]; 
  SDWORD LockedSide[2];
  SDWORD LockedTrack[2]; 
/*  Here for the drives and the controllers, we use the names of the actual
    hardware. Note that Steem SSE also uses those variable names, so it's
    getting confusing.
*/
  CapsDrive SF314[2]; // 2 double-sided floppy drives
  CapsFdc WD1772; // 1 cheap controller
};

#endif//ipf

#endif//SSECAPSIMG_H
