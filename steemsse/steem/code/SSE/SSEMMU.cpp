// this compiles as an apart object
#include "../pch.h"
#include "SSEMMU.h"

#if defined(SS_MMU)

#if defined(SS_MMU_WAKE_UP_DL)
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
// yes we can:  WU - WS - MODE - SYNC
TMMU MMU={{0,2,2,1,1,2},{0,2,4,3,1,2},{0,2,0,0,-2,2},{0,2,2,0,0,2}};

#else
TMMU MMU;
#endif

#endif
