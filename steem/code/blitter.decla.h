#pragma once
#ifndef BLITTER_DECLA_H
#define BLITTER_DECLA_H

#define EXT extern
#define INIT(s)


struct BLITTER_STRUCT{ // SS removed _
  WORD HalfToneRAM[16];
/*
All the address-related auxilary registers such as X-Count/Y-Count,
 X/Y-Increments etc. are signed values. 
 In other words, the Blitter can go backwards in memory as well as forward.
*/
  short SrcXInc,SrcYInc;
  MEM_ADDRESS SrcAdr;

  WORD EndMask[3];

  short DestXInc,DestYInc;
  MEM_ADDRESS DestAdr;

  WORD XCount,YCount;
  BYTE Hop,Op;

  char LineNumber;
  bool Smudge,Hog;//,Busy;

  BYTE Skew;
  bool NFSR,FXSR;

  DWORD SrcBuffer;

  bool Busy;

  int XCounter,YCounter; //internal counters
  WORD Mask;   //internal mask register
  bool Last;   //last flag

  bool HasBus;
  int TimeToSwapBus;

  bool NeedDestRead; //from Op


  bool InBlitter_Draw; //are we in the routine?
#if defined(STEVEN_SEAGAL) && defined(SS_BLT_BLIT_MODE_CYCLES)
  // we check every 64 cycles when we run for 256 cycles
  int TimeToCheckIrq; // same name style!
#endif

};

extern BLITTER_STRUCT Blit;

BYTE Blitter_IO_ReadB(MEM_ADDRESS);
void Blitter_IO_WriteB(MEM_ADDRESS,BYTE);
extern "C" void ASMCALL Blitter_Start_Now();
extern void Blitter_Draw();


#undef EXT
#undef INIT


#endif//BLITTER_DECLA_H