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


When we don't use the DL concept (SSE_MMU_WU not defined), variable 
OPTION_WS is WU or 0.

+--------------------------------------------+---------------+
| Steem  option    |    Wake-up concepts     |    Cycle      |
|    variable      |                         |  adjustment   |
+------------------+------------+------------+-------+-------+
|  OPTION_WS   |     WU     |      WS    | SHIFT |  SYNC |
|                  |    (ijor)  |    (LJBK)  | (Res) |(Freq) |
+------------------+------------+------------+-------+-------+
|   0 (ignore)     |     -      |      -     |    -  |    -  |
|        1         |  1 (cold)  |     1+3    |    -  |    -  |
|        2         |  2 (warm)  |     2+4    |   +2  |   +2  |
+------------------+------------+------------+-------+-------+


When we use the DL concept (SSE_MMU_WU defined), OPTION_WS 
is more confusing, its value is no wake-up state but just an option index.

+------------------------------------------------------------+---------------+
| Steem  option    |              Wake-up concepts           |    Cycle      |
|    variable      |                                         |  adjustment   |
+------------------+---------------+------------+------------+-------+-------+
|  OPTION_WS       |   DL Latency  |     WU     |      WS    | SHIFT |  SYNC |
|                  |     (Dio)     |    (ijor)  |    (LJBK)  | (Res) |(Freq) |
+------------------+---------------+------------+------------+-------+-------+
|   0 (ignore)     |      5        |     -      |      -     |    -  |    -  |
|        1         |      3        |   2 (warm) |      2     |   +2  |   +2  |
|        2         |      4        |     2      |      4     |    -  |   +2  |
|        3         |      5        |   1 (cold) |      3     |    -  |    -  |
|        4         |      6        |     1      |      1     |   -2  |    -  |
+------------------+---------------+------------+------------+-------+-------+

On STE there's no latency, DL=3, WS=1.

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
#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
  MEM_ADDRESS VideoCounter; // to separate from rendering
#endif
#if defined(SSE_MMU_WU)
  BYTE WU[6]; // 0, 1, 2
  BYTE WS[6]; // 0 + 4 + panic
  char ResMod[6],FreqMod[6];
#endif
#if defined(SSE_MMU_LINEWID_TIMING)
  BYTE Linewid0;
#endif
#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
  BYTE WordsToSkip; // for HSCROLL
#endif
#if defined(SSE_MMU_ROUNDING_BUS0A)
/*
Note for SSE_MMU_ROUNDING_BUS (v3.8.0)

This removes a long standing hack in Steem.
The INSTRUCTION_TIME() macros in MOVE when writing to memory weren't correct,
it should have been INSTRUCTION_TIME_ROUND().

On the other hand, you need "round cycles up to 4" only for RAM and Shifter
accesses.

I suspect it was so to have more programs running, hecause timing is super-
important for GLU sync and shift mode registers. Writes to the GLU aren't
subject to wait states.

Funny but this was already in the Engineering Hardware Specification of 1986:

    ---------------
    | MC68000 MPU   |<--
    |               |   |
     ---------------    |
                        |                           ----------
                        |<------------------------>|192 Kbyte |<--->EXPAN
               ---------|------------------------->| ROM      |
              |         |                           ----------
              |         |                           ----------
              |         |                          |512K or 1M|  
              |         |                       -->| byte RAM |<--
      ----------        |        ----------    |    ----------    |
     | Control  |<----->|<----->| Memory   |<--                   |
     | Logic    |-------|------>|Controller|<--                   |
      ----------        |        ----------    |    ----------    |
       |||||            |        ----------     -->| Video    |<--  RF MOD
       |||||            |<----->| Buffers  |<----->| Shifter  |---->RGB
       |||||            |       |          |        ----------      MONO
       |||||            |        ----------
       |||||            |        ----------         ----------
       |||||            |<----->| MC6850   |<----->| Keyboard |<--->IKBD
       |||| ------------|------>| ACIA     |       | Port     |
       ||||             |        ----------         ----------
       ||||             |        ----------         ----------
       ||||             |<----->| MC6850   |<----->| MIDI     |---->OUT/THRU
       ||| -------------|------>| ACIA     |       | Ports    |<----IN
       |||              |        ----------         ----------
       |||              |        ----------         ----------
       |||              |<----->| MK68901  |<----->| RS232    |<--->MODEM
       || --------------|------>| MFP      |<--    | Port     |
       ||               |        ----------    |    ----------
       ||               |                      |    ----------
       ||               |                       ---| Parallel |<--->PRINTER
       ||               |                       -->| Port     |
       ||               |        ----------    |    ----------
       ||               |<----->| YM-2149  |<--     ----------
       | ---------------|------>| PSG      |------>| Sound    |---->AUDIO
       |                |       |          |---    | Channels |
       |                |        ----------    |    ----------
       |                |                      |    ----------
       |                |        ----------     -->| Floppy   |<--->FLOPPY
       |                |<------| WD1772   |<----->|Disk Port |     DRIVE
       |                |    -->| FDC      |        ----------
       |                |   |    ----------
       |                |   |    ----------         ----------
       |                |    -->| DMA      |<----->|Hard Disk |<--->HARD
       |                |<----->|Controller|       | Port     |     DRIVE
        ----------------------->|          |        ----------
                                 ----------

The CPU accesses RAM and the Shifter through the MMU, which forces it to share
cycles with the video system. All the rest is directly available on the bus.
Though there are wait states too for all 8bit peripherals.

It is not such a breaking change, nor does it fix anything, but it does 
add overhead.

TODO: we added on the current system, but we could refactor into
something simpler, now that we better understand rounding rules.

*/

/*  The MMU is responsible for inserting wait states when the CPU needs
    to access memory while the MMU is feeding the Shifter with video ram
    (shared memory).
    'Rounded' means that such wait states were inserted.
    'Unrounded' means that those wait states were removed because something
    directly accessible on the bus was accessed, not ram. It's a flag that
    prevents double correction. It's a bit hacky.
*/
  bool Unrounded;
  char Rounded;
#endif

  // FUNCTIONS
  MEM_ADDRESS ReadVideoCounter(int CyclesIn);
  void ShiftSDP(int shift);  
  void WriteVideoCounter(MEM_ADDRESS addr, BYTE io_src_b);
#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
  void UpdateVideoCounter(int CyclesIn);
#endif
#if defined(SSE_MMU_LOW_LEVEL)
  WORD ReadRAM(); // abus as parameter?
  void WriteRAM(); // abus, dbus as parameters?
#endif
};

#pragma pack(pop)

#endif
#endif//#ifndef SSEMMU_H