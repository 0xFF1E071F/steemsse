#pragma once
#ifndef BLITTER_DECLA_H
#define BLITTER_DECLA_H

#define EXT extern
#define INIT(s)

/*  SS The following structure was named _BLITTER_STRUCT in Steem 3.2.
    We changed the name into TBlitter.
*/
// TODO padding... why only for "SSE" objects? but then trouble again with old snapshots 

//#pragma pack(push, STRUCTURE_ALIGNMENT)

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

#if defined(SSE_BLT_COPY_LOOP)
  BYTE BlittingPhase; // to follow 'state-machine' phase R-W...
  enum {PRIME,READ_SOURCE,READ_DEST,WRITE_DEST};
#else
  bool InBlitter_Draw; //are we in the routine? //SS not used
#endif
#if defined(SSE_BLT_390B)
  int TimeAtBlit,BlitCycles;
#endif
#if defined(SSE_BLT_COPY_LOOP)
  WORD SrcDat,DestDat,NewDat;  //internal data registers - now persistent 
#endif
#if defined(SSE_BLT_RESTART)
  bool Restarted; // flag - cheat
#endif
};

//#pragma pack(pop)

extern TBlitter Blit;

extern "C" void ASMCALL Blitter_Start_Now();

extern void Blitter_Draw();

BYTE Blitter_IO_ReadB(MEM_ADDRESS);
void Blitter_IO_WriteB(MEM_ADDRESS,BYTE);

#undef EXT
#undef INIT


#endif//BLITTER_DECLA_H