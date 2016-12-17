#pragma once
#ifndef BLITTER_DECLA_H
#define BLITTER_DECLA_H

#define EXT extern
#define INIT(s)

/*  SS The following structure was named _BLITTER_STRUCT in Steem 3.2.
    We changed the name into TBlitter.
*/

struct TBlitter{ 
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

#if defined(SSE_BLT_YCOUNT)
  WORD XCount;
  DWORD YCount; // hack, we need more than 16 bit for the 0=65536 thing
#else
  WORD XCount,YCount;
#endif
  
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
#if defined(SSE_BLT_390B)
  int TimeAtBlit,BlitCycles;
#endif
};

extern TBlitter Blit;

BYTE Blitter_IO_ReadB(MEM_ADDRESS);
void Blitter_IO_WriteB(MEM_ADDRESS,BYTE);
extern "C" void ASMCALL Blitter_Start_Now();
extern void Blitter_Draw();


#undef EXT
#undef INIT


#endif//BLITTER_DECLA_H