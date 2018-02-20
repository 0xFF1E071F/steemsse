/*

MMU = Memory Management Unit


Wake-up states (WU): how every 4 cycles are shared

State 1
+-----+
| CPU |
+-----+
| MMU |
+-----+


State 2
+-----+
| MMU |
+-----+
| CPU |
+-----+

This is important for timings relative to GLU "decisions", and is caused by
two 2bit counters, one in GLU, one in MMU that get non deterministic
values at power up on the STF.
The relation between those counters causes a different latency between DE signal
and LOAD signal (=DL latency), one of four, in CPU cycles.

+------------------------------------------------------------+---------------+
| Steem  option    |              Wake-up concepts           |    Cycle      |
|    variable      |                                         |  adjustment   |
+------------------+---------------+------------+------------+-------+-------+
|  OPTION_WS       |   DL Latency  |     WU     |      WS    | SHIFT |  SYNC |
|                  |     (Dio)     |    (ijor)  |    (LJBK)  | (Res) |(Freq) |
+------------------+---------------+------------+------------+-------+-------+
|        1         |      3        |   2 (warm) |      2     |   +2  |   +2  |
|        2         |      4        |     2      |      4     |    -  |   +2  |
|        3         |      5        |   1 (cold) |      3     |    -  |    -  |
|        4         |      6        |     1      |      1     |   -2  |    -  |
+------------------+---------------+------------+------------+-------+-------+

'Cycle adjustment' is applied to apparent GLU timings.

On the STE, DL=6, WS=1.

*/

#ifndef SSEMMU_H
#define SSEMMU_H

#include "SSEOption.h"
#include "SSEParameters.h"
#include "SSESTF.h"
#include "SSEShifter.h"

#ifdef SSE_MMU

// minimal delay between GLUE DE and first LOAD signal emitted by the MMU
#if defined(SSE_CPU)
#define MMU_PREFETCH_LATENCY (8)
#else
#define MMU_PREFETCH_LATENCY (12) 
#endif

/*  We were having a nasty bug in the _DEBUG build where writing VideoCounter 
    would corrupt SideBorderSize, if we moved instantiation of video objects
    to SSEVideo.
    This directive ensures that the same size is considered everywhere.
    We also moved data to be better aligned. 
*/
#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TMMU {

  //DATA
  MEM_ADDRESS VideoCounter; // to separate from rendering
#if defined(SSE_MMU_MONSTER_ALT_RAM)
  MEM_ADDRESS MonSTerHimem;
#endif
#if defined(SSE_MMU_LOW_LEVEL)
  MEM_ADDRESS DecodedAddress;
#endif
#if defined(SSE_MMU_WU)
  BYTE WU[6]; // 0, 1, 2
  BYTE WS[6]; // 0 + 4 + panic
  char ResMod[6],FreqMod[6];
#endif
#if defined(SSE_MMU_LINEWID_TIMING)
  BYTE Linewid0;
#endif
  BYTE ExtraBytesForHscroll;
  // FUNCTIONS
  MEM_ADDRESS ReadVideoCounter(int CyclesIn);
  void ShiftSDP(int shift);  
  void WriteVideoCounter(MEM_ADDRESS addr, BYTE io_src_b);
  void UpdateVideoCounter(int CyclesIn);
#if defined(SSE_MMU_LOW_LEVEL)
  void DecodeAddress();
  void ReadByte(); 
  void WriteByte();
  void ReadWord();
  void WriteWord();
#endif
};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#endif
#endif//#ifndef SSEMMU_H