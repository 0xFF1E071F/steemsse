// this compiles as an apart object

#include "SSEMMU.h"

#if defined(SS_MMU)
TMMU MMU;

/*
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
*/

BYTE TMMU::WS() {
  BYTE WS_val=WAKE_UP_STATE;
  if(WAKE_UP_STATE==1)
    WS_val=2;
  else if(WAKE_UP_STATE==2 || WAKE_UP_STATE==WU_SHIFTER_PANIC)
    WS_val=4;
  else if(WAKE_UP_STATE==4)
    WS_val=1;
  return WS_val;
}

#endif
