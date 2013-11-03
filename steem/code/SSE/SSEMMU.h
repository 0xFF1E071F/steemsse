/*


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


When we don't use the DL concept (SS_MMU_WAKE_UP_DL not defined), variable 
WAKE_UP_STATE is WU or 0.

+--------------------------------------------+---------------+
| Steem  option    |    Wake-up concepts     |    Cycle      |
|    variable      |                         |  adjustment   |
+------------------+------------+------------+-------+-------+
|  WAKE_UP_STATE   |     WU     |      WS    |  MODE |  SYNC |
|                  |    (ijor)  |    (LJBK)  | (Res) |(Freq) |
+------------------+------------+------------+-------+-------+
|   0 (ignore)     |     -      |      -     |    -  |    -  |
|        1         |  1 (cold)  |     1+3    |    -  |    -  |
|        2         |  2 (warm)  |     2+4    |   +2  |   +2  |
+------------------+------------+------------+-------+-------+


When we use the DL concept (SS_MMU_WAKE_UP_DL defined), WAKE_UP_STATE 
is more confusing, its value is no wake-up state but just an option index.

+------------------------------------------------------------+---------------+
| Steem  option    |              Wake-up concepts           |    Cycle      |
|    variable      |                                         |  adjustment   |
+------------------+---------------+------------+------------+-------+-------+
|  WAKE_UP_STATE   |   DL Latency  |     WU     |      WS    |  MODE |  SYNC |
|                  |     (Dio)     |    (ijor)  |    (LJBK)  | (Res) |(Freq) |
+------------------+---------------+------------+------------+-------+-------+
|   0 (ignore)     |      5        |     -      |      -     |    -  |    -  |
|        1         |      3        |   2 (warm) |      2     |   +2  |   +2  |
|        2         |      4        |     2      |      4     |    -  |   +2  |
|        3         |      5        |   1 (cold) |      3     |    -  |    -  |
|        4         |      6        |     1      |      1     |   -2  |    -  |
+------------------+---------------+------------+------------+-------+-------+

On STE there's no latency, DL=3, but our code doesn't seem ready for that
yet.

*/

#ifndef SSEMMU_H
#define SSEMMU_H

#include "SSEOption.h"
#include "SSEParameters.h"
#include "SSESTF.h"

#ifdef SS_MMU

struct TMMU {
#if defined(SS_MMU_WAKE_UP_DL)
  BYTE WU[6]; // 0, 1, 2
  BYTE WS[6]; // 0 + 4 + panic
  char ResMod[6],FreqMod[6];
#else
  inline bool OnMmuCycles(int CyclesIn);
  inline bool WakeUpState1(); // for STF
  inline bool WakeUpState2(); // for STF 
#endif
};

extern TMMU MMU;

#if !defined(SS_MMU_WAKE_UP_DL)

/*  This function tells if the program is attempting a read or write on the
    bus while the MMU has the bus. In that case, access should be delayed by
    2 cycles. In practice it isn't used to check RAM access but IO access.
    We ended up creating a new MMU object and an inline function because
    of the complicated defines (as usual in this build).
    3.5.4: concept should change
*/
inline bool TMMU::OnMmuCycles(int CyclesIn) {
  return  WakeUpState1()&&(CyclesIn%4) ||  WakeUpState2()&&!(CyclesIn%4) ;
}

/*  A bit confusing. We have option Wake Up State 2 for both STF & STE, but
    apparently the STE isn't in the same wake up state 2 as the STF where
    all shifter tricks may be shifted (lol) by 2 cycles, instead it would just
    be the palette delay (1 cycle).
    3.5.4: give up this STE thing?
*/
inline bool TMMU::WakeUpState1() {
  return (
#if defined(SS_STF) && !defined(SS_MMU_WAKE_UP_STE_STATE2)
  (ST_TYPE!=STE) &&
#endif

#if defined(SS_MMU_WAKE_UP_DL)
  (WAKE_UP_STATE==3||WAKE_UP_STATE==4)
#else
  (WAKE_UP_STATE==1)
#endif
  );
}

inline bool TMMU::WakeUpState2() {
  return (
#if defined(SS_STF) && !defined(SS_MMU_WAKE_UP_STE_STATE2)
  (ST_TYPE!=STE) &&
#endif
#if defined(SS_MMU_WAKE_UP_DL)
  (WAKE_UP_STATE==1||WAKE_UP_STATE==2
#if defined(SS_SHIFTER_PANIC) // used for omega only
  ||WAKE_UP_STATE==WU_SHIFTER_PANIC
#endif
  )
#else
  (WAKE_UP_STATE&2)
#endif
  );
}
#endif

#endif
#endif//#ifndef SSEMMU_H