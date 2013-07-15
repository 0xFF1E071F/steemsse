/*

Wake-up states: how every 4 cycles are shared

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

*/

#ifndef SSEMMU_H
#define SSEMMU_H

#include "SSEOption.h"
#include "SSESTF.h"

#ifdef SS_MMU


struct TMMU {
//  int WakeUpState; // we take the variable in SSEOption
  inline bool OnMmuCycles(int CyclesIn);
  inline bool WakeUpState2(); 
};

extern TMMU MMU;

/*  This function tells if the program is attempting a read or write on the
    bus while the MMU has the bus. In that case, access should be delayed by
    2 cycles. In practice it isn't used to check RAM access but IO access.
    We ended up creating a new MMU object and an inline function because
    of the complicated defines (as usual in this build).
*/
inline bool TMMU::OnMmuCycles(int CyclesIn) {
  bool answer=false;
  if(0);

#if defined(SS_MMU_WAKE_UP_DELAY_ACCESS)
  else if(WAKE_UP_STATE==1 && (CyclesIn%4))
    answer=true;

#if defined(SS_STF) || defined(SS_MMU_WAKE_UP_STE_STATE2)
  else if(WAKE_UP_STATE==2 && !(CyclesIn%4)
#if !defined(SS_MMU_WAKE_UP_STE_STATE2)
    && ST_TYPE!=STE
#endif
    )
    answer=true;
#endif    
#endif

  return answer;
}

/*  A bit confusing. We have option Wake Up State 2 for both STF & STE, but
    apparently the STE isn't in the same wake up state 2 as the STF where
    all shifter tricks may be shifted (lol) by 2 cycles, instead it would just
    be the palette delay (1 cycle).
*/
inline bool TMMU::WakeUpState2() {
  return (
#if defined(SS_STF) && !defined(SS_MMU_WAKE_UP_STE_STATE2)
  (ST_TYPE!=STE) &&
#endif
  WAKE_UP_STATE==2);
}

#endif
#endif//#ifndef SSEMMU_H