#pragma once
#ifndef SSEFLOPPY_H
#define SSEFLOPPY_H

#if defined(SS_FDC)
#if defined(SS_FDC_IPF)

#include <caps/Comtype.h>
#include <caps/CapsAPI.h>
#include <caps/CapsPlug.h>
#include <caps/CapsFDC.h>

// our interface with CAPSimg.dll
struct TCaps {  
  TCaps();
  ~TCaps();
  Init();
  // using static functions so that there's no 'this'
  static void CallbackDRQ(PCAPSFDC pc, UDWORD setting);
  static void CallbackIRQ(PCAPSFDC pc, UDWORD lineout);
  static void CallbackTRK(PCAPSFDC pc, UDWORD driveact);
  BOOL Initialised; // we do need a correct DLL
  BOOL Active; // if there's an IPF disk in some drive, we must run IPF cycles
  // for drive A & B
  SDWORD DriveIsIPF[2]; 
  SDWORD ContainerID[2]; 
  SDWORD LockedSide[2];
  SDWORD LockedTrack[2]; 
};

extern TCaps Caps;

#endif
#endif

#endif//SSEFLOPPY_H
