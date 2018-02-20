#pragma once
#ifndef BLITTER_DECLA_H
#define BLITTER_DECLA_H

#define EXT extern
#define INIT(s)

/*  SS The following structure was named _BLITTER_STRUCT in Steem 3.2.
    We changed the name into TBlitter.
*/

#if defined(SSE_COMPILER_STRUCT_391)

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TBlitter{ 
#if defined(SSE_TIMINGS_CPUTIMER64)
  COUNTER_VAR TimeToSwapBus,TimeAtBlit;
  int BlitCycles;
#endif
  MEM_ADDRESS SrcAdr,DestAdr;
#if defined(SSE_BLT_YCOUNT)
  DWORD YCount; // hack, we need more than 16 bit for the 0=65536 thing //TODO
#endif
  DWORD SrcBuffer;
  int XCounter,YCounter; //internal counters
#if !defined(SSE_TIMINGS_CPUTIMER64)
  int TimeToSwapBus;
#if defined(SSE_BLT_CPU_RUNNING) 
  int TimeAtBlit,BlitCycles;
#endif
#endif
  WORD HalfToneRAM[16];
  WORD EndMask[3];
#if defined(SSE_BLT_YCOUNT)
  WORD XCount;
#else
  WORD XCount,YCount;
#endif
#if defined(SSE_BLT_COPY_LOOP)
  WORD SrcDat,DestDat,NewDat;  //internal data registers - now persistent 
#endif
  WORD Mask;   //internal mask register
  short SrcXInc,SrcYInc,DestXInc,DestYInc;
  BYTE Hop,Op,Skew;
#if defined(SSE_BLT_COPY_LOOP)
  BYTE BlittingPhase; // to follow 'state-machine' phase R-W...
  enum {PRIME,READ_SOURCE,READ_DEST,WRITE_DEST};
#else
  bool InBlitter_Draw; //are we in the routine? //SS not used
#endif
  bool Smudge,Hog,NFSR,FXSR,Busy,Last,HasBus,NeedDestRead;
#if defined(SSE_BLT_RESTART)
  bool Restarted; // flag - cheat
#endif
#if defined(SSE_BLT_BUS_ARBITRATION)
/*  Because there are more checks for blitter start, we use a first flag
    telling whether we should investigate or not.
*/
  BYTE Request; //0 1 2
  BYTE BusAccessCounter; // count accesses by blitter and by CPU in blit mode
#endif
  char LineNumber;

};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#else

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
#if defined(SSE_BLT_CPU_RUNNING)
  int TimeAtBlit,BlitCycles;
#endif
#if defined(SSE_BLT_COPY_LOOP)
  WORD SrcDat,DestDat,NewDat;  //internal data registers - now persistent 
#endif
#if defined(SSE_BLT_RESTART)
  bool Restarted; // flag - cheat
#endif
};

#endif//#if defined(SSE_COMPILER_STRUCT_391)

extern TBlitter Blit;

extern "C" void ASMCALL Blitter_Start_Now();

extern void Blitter_Draw();

BYTE Blitter_IO_ReadB(MEM_ADDRESS);
void Blitter_IO_WriteB(MEM_ADDRESS,BYTE);



#if defined(SSE_BLT_BUS_ARBITRATION)

void Blitter_CheckRequest();
inline void check_blitter_start() {
  if(Blit.Request)
    Blitter_CheckRequest();
}

#define CHECK_BLITTER_START check_blitter_start();
#else
#define CHECK_BLITTER_START
#endif


#undef EXT
#undef INIT

#endif//BLITTER_DECLA_H