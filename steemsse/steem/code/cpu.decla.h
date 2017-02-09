#pragma once
#if !defined(CPU_DECLA_H)
#define CPU_DECLA_H

#if defined(SSE_STRUCTURE_DECLA)
#include <binary.h>
#include <setjmp.h>
#include "conditions.h"
#include "acc.decla.h"
#include "emulator.decla.h"
#include "iorw.decla.h"
#include "steemh.decla.h"
#include "SSE/SSEDebug.h"
#include "SSE/SSEDecla.h" //intrinsics
#include "SSE/SSECpu.h"
#endif

//SS It is called jump but they are used as function calls, they return,
// except if there's a ST crash detected, then they jump!
extern void (*m68k_high_nibble_jump_table[16])();
extern void (*m68k_jump_line_0[64])();
extern void (*m68k_jump_line_4[64])();
extern void (*m68k_jump_line_5[8])();
extern void (*m68k_jump_line_8[8])();
extern void (*m68k_jump_line_9[8])();
extern void (*m68k_jump_line_b[8])();
extern void (*m68k_jump_line_c[8])();
extern void (*m68k_jump_line_d[8])();
extern void (*m68k_jump_line_e[64])();
extern void (*m68k_jump_line_4_stuff[64])();
extern void (*m68k_jump_get_source_b[8])();
extern void (*m68k_jump_get_source_w[8])();
extern void (*m68k_jump_get_source_l[8])();
extern void (*m68k_jump_get_source_b_not_a[8])();
extern void (*m68k_jump_get_source_w_not_a[8])();
extern void (*m68k_jump_get_source_l_not_a[8])();
extern void (*m68k_jump_get_dest_b[8])();
extern void (*m68k_jump_get_dest_w[8])();
extern void (*m68k_jump_get_dest_l[8])();
extern void (*m68k_jump_get_dest_b_not_a[8])();
extern void (*m68k_jump_get_dest_w_not_a[8])();
extern void (*m68k_jump_get_dest_l_not_a[8])();
extern void (*m68k_jump_get_dest_b_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_w_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_l_not_a_or_d[8])();
#if !defined(SSE_CPU)
extern void (*m68k_jump_get_dest_b_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_dest_w_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_dest_l_not_a_faster_for_d[8])();
#endif
extern bool (*m68k_jump_condition_test[16])();

void m68k_get_source_000_b();
void m68k_get_source_000_w();
void m68k_get_source_000_l();
void m68k_get_source_001_b();
void m68k_get_source_001_w();
void m68k_get_source_001_l();
void m68k_get_source_010_b();
void m68k_get_source_010_w();
void m68k_get_source_010_l();
void m68k_get_source_011_b();
void m68k_get_source_011_w();
void m68k_get_source_011_l();
void m68k_get_source_100_b();
void m68k_get_source_100_w();
void m68k_get_source_100_l();
void m68k_get_source_101_b();
void m68k_get_source_101_w();
void m68k_get_source_101_l();
void m68k_get_source_110_b();
void m68k_get_source_110_w();
void m68k_get_source_110_l();
void m68k_get_source_111_b();
void m68k_get_source_111_w();
void m68k_get_source_111_l();

void m68k_get_dest_000_b();
void m68k_get_dest_000_w();
void m68k_get_dest_000_l();
void m68k_get_dest_001_b();
void m68k_get_dest_001_w();
void m68k_get_dest_001_l();
void m68k_get_dest_010_b();
void m68k_get_dest_010_w();
void m68k_get_dest_010_l();
void m68k_get_dest_011_b();
void m68k_get_dest_011_w();
void m68k_get_dest_011_l();
void m68k_get_dest_100_b();
void m68k_get_dest_100_w();
void m68k_get_dest_100_l();
void m68k_get_dest_101_b();
void m68k_get_dest_101_w();
void m68k_get_dest_101_l();
void m68k_get_dest_110_b();
void m68k_get_dest_110_w();
void m68k_get_dest_110_l();
void m68k_get_dest_111_b();
void m68k_get_dest_111_w();
void m68k_get_dest_111_l();

#if !defined(SSE_CPU)
void m68k_get_dest_000_b_faster();
void m68k_get_dest_000_w_faster();
void m68k_get_dest_000_l_faster();
void m68k_get_dest_001_b_faster();
void m68k_get_dest_001_w_faster();
void m68k_get_dest_001_l_faster();
void m68k_get_dest_010_b_faster();
void m68k_get_dest_010_w_faster();
void m68k_get_dest_010_l_faster();
void m68k_get_dest_011_b_faster();
void m68k_get_dest_011_w_faster();
void m68k_get_dest_011_l_faster();
void m68k_get_dest_100_b_faster();
void m68k_get_dest_100_w_faster();
void m68k_get_dest_100_l_faster();
void m68k_get_dest_101_b_faster();
void m68k_get_dest_101_w_faster();
void m68k_get_dest_101_l_faster();
void m68k_get_dest_110_b_faster();
void m68k_get_dest_110_w_faster();
void m68k_get_dest_110_l_faster();
void m68k_get_dest_111_b_faster();
void m68k_get_dest_111_w_faster();
void m68k_get_dest_111_l_faster();
#endif

#if defined(SSE_VC_INTRINSICS_390B)
extern int (*count_bits_set_in_word)(unsigned short);
#endif

BYTE m68k_fetchB();
WORD m68k_fetchW();
LONG m68k_fetchL();
void ASMCALL perform_crash_and_burn();
void m68k_unrecognised();

void m68kSetPC(MEM_ADDRESS ad);

#if defined(COMPILER_VC6)
extern "C" void ASMCALL m68k_trace(); //execute instruction with trace bit set
#else
extern "C" ASMCALL void m68k_trace(); //execute instruction with trace bit set
#endif

#if defined(SSE_CPU)
extern "C" unsigned getDivu68kCycles( unsigned long dividend, unsigned short divisor);
extern "C" unsigned getDivs68kCycles( signed long dividend, signed short divisor);
#endif

enum nbombs {
  BOMBS_BUS_ERROR =2,
  BOMBS_ADDRESS_ERROR =3,
  BOMBS_ILLEGAL_INSTRUCTION =4,
  BOMBS_DIVISION_BY_ZERO =5,
  BOMBS_CHK= 6,
  BOMBS_TRAPV= 7,
  BOMBS_PRIVILEGE_VIOLATION= 8,
  BOMBS_TRACE_EXCEPTION= 9, 
  BOMBS_LINE_A= 10,
  BOMBS_LINE_F= 11
};

enum exception_action{EA_READ=0,EA_WRITE,EA_FETCH,EA_INST};

class m68k_exception
{
public:
  int bombs;
  MEM_ADDRESS _pc;
  MEM_ADDRESS crash_address;
  MEM_ADDRESS address;
  WORD _sr,_ir;
  exception_action action;

  m68k_exception() {}
  ~m68k_exception() {}

  void init(int,exception_action,MEM_ADDRESS);
  void crash();
};


extern void exception(int,exception_action,MEM_ADDRESS);

// GCC try/catch fix:
extern m68k_exception ExceptionObject;
extern jmp_buf *pJmpBuf;

// This is exactly the same as the C++ exceptions except you mustn't return or break
// out of the exception block without executing the END_M68K_EXCEPTION. If you have
// to you can just add pJmpBuf=oldpJmpBuf before the return/break and it will work.

// This could be fixed by making a wrapper class for jmp_buf so it will call the
// destructor when it goes out of scope, but GCC seems flakey on that sort of thing.
#if defined(SSE_M68K_EXCEPTION_TRY_CATCH)
/*  Using C++ try/catch works but it's dramatically slower in the debug build.
    Check for example with the BIG Demo.
    There's certainly a performance hit in the release builds too, as try/catch
    does more than setjmp, but we don't need this work, because setjmp is only
    used to go through the calling stack, in other words it's appropriate. 

    No reason to define, but heed warning C4611.
*/
#define TRY_M68K_EXCEPTION try {
#define CATCH_M68K_EXCEPTION } catch (m68k_exception* exception_object) { 
#define END_M68K_EXCEPTION }
#else
#define TRY_M68K_EXCEPTION jmp_buf temp_excep_jump;jmp_buf *oldpJmpBuf=pJmpBuf;pJmpBuf=&temp_excep_jump;if (setjmp(temp_excep_jump)==0){
#define CATCH_M68K_EXCEPTION }else{
#define END_M68K_EXCEPTION }pJmpBuf=oldpJmpBuf;
#endif
#define areg (r+8)

extern BYTE  m68k_peek(MEM_ADDRESS ad);
extern WORD  m68k_dpeek(MEM_ADDRESS ad);
extern LONG  m68k_lpeek(MEM_ADDRESS ad);

extern void cpu_routines_init();

#if !defined(SSE_CPU)
extern int m68k_divu_cycles,m68k_divs_cycles;
#endif

#if defined(SSE_VAR_OPT_390A)
extern int act; // to be updated with ABSOLUTE_CPU_TIME and used as appropriate
#endif

#if defined(SSE_CPU_DATABUS)
extern WORD dbus;
#endif
#if defined(SSE_CPU_RESTORE_ABUS)
extern MEM_ADDRESS dest_addr;
#endif


#define PC32 ( (pc&0xffffff)|(pc_high_byte) )
#define FOUR_MEGS 0x400000
#define FOURTEEN_MEGS 0xE00000

#define MEM_FIRST_WRITEABLE 8

#define SR_IPL   (BIT_a+BIT_9+BIT_8 )
#define SR_IPL_7 (BIT_a+BIT_9+BIT_8 )
#define SR_IPL_6 (BIT_a+BIT_9       )
#define SR_IPL_5 (BIT_a+      BIT_8 )
#define SR_IPL_4 (BIT_a             )
#define SR_IPL_3 (      BIT_9+BIT_8 )
#define SR_IPL_2 (      BIT_9       )
#define SR_IPL_1 (            BIT_8 )
#define SR_IPL_0                    0

#define SR_C 1
#define SR_V 2
#define SR_Z 4
#define SR_N 8
#define SR_X 16
#define SR_SUPER BIT_d
#define SR_USER_BYTE 31 //SS = $1F
#define SR_TRACE (WORD(BIT_f))

#if defined(SSE_VC_INTRINSICS_390)
enum {SR_C_BIT,SR_V_BIT,SR_Z_BIT,SR_N_BIT,SR_X_BIT,SR_SUPER_BIT=0xd,
SR_TRACE_BIT=0xf};
#endif

#if defined(SSE_VC_INTRINSICS_390A)
#define SUPERFLAG (BITTEST(sr,SR_SUPER_BIT))
#elif defined(SSE_VAR_REWRITE)
#define SUPERFLAG ( ((sr&SR_SUPER)!=0) ) // warning C4800
#else
#define SUPERFLAG ((bool)(sr&SR_SUPER))
#endif

#define SSP ((MEM_ADDRESS)((SUPERFLAG) ? r[15]:other_sp))
#define USP ((MEM_ADDRESS)((SUPERFLAG) ? other_sp:r[15]))
#define lpSSP ((MEM_ADDRESS*)((SUPERFLAG) ? &(r[15]):&other_sp))
#define lpUSP ((MEM_ADDRESS*)((SUPERFLAG) ? &other_sp:&(r[15])))

#define IOACCESS_FLAGS_MASK  0xFFFFFFC0
#define IOACCESS_NUMBER_MASK 0x0000003F

#define IOACCESS_FLAG_FOR_CHECK_INTRS BIT_6
#if !defined(SSE_YM2149_BUS_JAM_390) || !defined(SSE_CPU)
#define IOACCESS_FLAG_PSG_BUS_JAM_R BIT_7
#define IOACCESS_FLAG_PSG_BUS_JAM_W BIT_8
#endif
#define IOACCESS_FLAG_DO_BLIT BIT_9
#define IOACCESS_FLAG_FOR_CHECK_INTRS_MFP_CHANGE BIT_10
#define IOACCESS_FLAG_DELAY_MFP BIT_11
#define IOACCESS_INTERCEPT_OS BIT_12
#define IOACCESS_INTERCEPT_OS2 BIT_13

extern WORD*lpfetch,*lpfetch_bound;
extern bool prefetched_2;
extern WORD prefetch_buf[2]; // SS the 2 words prefetch queue

// Finer names!
#define IRC   prefetch_buf[1] // Instruction Register Capture
#define IR    prefetch_buf[0] // Instruction Register
#define IRD   ir              // Instruction Register Decoder

#ifdef SSE_CPU
#define m68k_PROCESS m68kProcess();

#define m68k_DEST_B (*((signed char*)m68k_dest))
#define m68k_DEST_W (*((short*)m68k_dest))
#define m68k_DEST_L (*((long*)m68k_dest))


/////////////////////
// Counting cycles //
/////////////////////

#if defined(SSE_BLT_BUS_ARBITRATION_391)
/*
    http://patpend.net/technical/68000/68000faq.txt

Note that bus arbitration is done on a cycle-by-cycle 
basis, so the bus can be arbitrated away from the MC68000 after the first 
of the two 16-bit writes, and the processor will complete the long-word 
write after it retakes the bus.
*/

inline void InstructionTime(int t) {
#if defined(SSE_BLT_390B)
/*  Only when the CPU doesn't access the main bus, it can run while the blitter
    is also running.
    INSTRUCTION_TIME is used to translate the 'n' timings in Yacht.
    As we can see, this feature costs a lot in overhead, but that's the price
    of "correct" emulation.
*/
  if(Blit.BlitCycles>t && t>0)
    Blit.BlitCycles-=(t);
  else
#endif
  cpu_cycles-=(t);
#if defined(SSE_BLT_BUS_ARBITRATION_391B) //overhead!
  if (ioaccess & IOACCESS_FLAG_DO_BLIT)
#if defined(SSE_BLT_RESTART2)
    if(Blit.Busy)
      Blitter_Draw();
    else
#endif
    Blitter_Start_Now(); 
#endif
}

#define INSTRUCTION_TIME(t)  InstructionTime(t)


/*  INSTRUCTION_TIME_BUS is used for np, nr, nw, etc. in Yacht when the 
    RAM/Shifter bus isn't used (no rounding up to 4, but no running during
    a blit either).
*/

inline void InstructionTimeCpuBus(int t) {
  cpu_cycles-=(t);
#if defined(SSE_BLT_BUS_ARBITRATION_391B)
  if (ioaccess & IOACCESS_FLAG_DO_BLIT)
#if defined(SSE_BLT_RESTART2)
    if(Blit.Busy)
      Blitter_Draw();
    else
#endif
    Blitter_Start_Now(); 
#endif
}

#define INSTRUCTION_TIME_BUS(t)  InstructionTimeCpuBus(t)


inline void InstructionTimeRamBus(int t) { // RAM + Shifter
  cpu_cycles-=(t);
  cpu_cycles&=-4; // MMU adds wait states if necessary
#if defined(SSE_BLT_BUS_ARBITRATION_391B)
  if (ioaccess & IOACCESS_FLAG_DO_BLIT)
#if defined(SSE_BLT_RESTART2)
    if(Blit.Busy)
      Blitter_Draw();
    else
#endif
    Blitter_Start_Now(); 
#endif
}


#define INSTRUCTION_TIME_ROUND(t)  InstructionTimeRamBus(t)

#else

inline void InstructionTime(int t) {
#if defined(SSE_BLT_390B)
  if(Blit.BlitCycles>t && t>0)
    Blit.BlitCycles-=(t);
  else
#endif
  cpu_cycles-=(t);
}

#define INSTRUCTION_TIME(t)  InstructionTime(t)

#endif//391

#if defined(SSE_MMU_ROUNDING_BUS)

inline void FetchTiming();
inline void FetchTimingL();
#define CPU_ABUS_ACCESS_READ_FETCH FetchTiming()
#define CPU_ABUS_ACCESS_READ_FETCH_L FetchTimingL()
inline void ReadBusTiming();
inline void ReadBusTimingL();
#define CPU_ABUS_ACCESS_READ  ReadBusTiming()
#define CPU_ABUS_ACCESS_READ_L  ReadBusTimingL()
#define BLT_ABUS_ACCESS_READ  ReadBusTiming()
inline void StackTiming();
inline void StackTimingL();
#define CPU_ABUS_ACCESS_READ_POP StackTiming()
#define CPU_ABUS_ACCESS_READ_POP_L StackTimingL()
#define CPU_ABUS_ACCESS_WRITE_PUSH StackTiming()
#define CPU_ABUS_ACCESS_WRITE_PUSH_L StackTimingL()
inline void WriteBusTiming();
inline void WriteBusTimingL();
#define CPU_ABUS_ACCESS_WRITE  WriteBusTiming()
#define CPU_ABUS_ACCESS_WRITE_L  WriteBusTimingL()
#define BLT_ABUS_ACCESS_WRITE  WriteBusTiming()

#else

inline void InstructionTimeRound(int t) {
  InstructionTime(t);
  cpu_cycles&=-4;
}

#define INSTRUCTION_TIME_ROUND(t) InstructionTimeRound(t)

#define CPU_ABUS_ACCESS_READ  INSTRUCTION_TIME_ROUND(4)
#define CPU_ABUS_ACCESS_READ_L  INSTRUCTION_TIME_ROUND(8) //for performance
#define CPU_ABUS_ACCESS_READ_FETCH CPU_ABUS_ACCESS_READ
#define CPU_ABUS_ACCESS_READ_FETCH_L CPU_ABUS_ACCESS_READ_L
#define CPU_ABUS_ACCESS_READ_POP CPU_ABUS_ACCESS_READ
#define CPU_ABUS_ACCESS_READ_POP_L CPU_ABUS_ACCESS_READ_L
#define CPU_ABUS_ACCESS_WRITE_PUSH CPU_ABUS_ACCESS_WRITE
#define CPU_ABUS_ACCESS_WRITE_PUSH_L CPU_ABUS_ACCESS_WRITE_L
#define CPU_ABUS_ACCESS_WRITE  INSTRUCTION_TIME_ROUND(4)
#define CPU_ABUS_ACCESS_WRITE_L  INSTRUCTION_TIME_ROUND(8) //for performance

#endif //mmu rounding

#endif


#if defined(SSE_CPU)

#if defined(SSE_BLT_BUS_ARBITRATION_391)
/*  Clear "blit cycles" as soon as there's a bus access.
    If counting for the blitter, no problem, blit cycles are computed after 
    blit.
*/
/*  We do a single comparison here, but if the R/W is to Shifter registers,
    rounding up happens in ior/iow.
*/
inline void FetchTiming() {
#if defined(SSE_BLT_390B)
  Blit.BlitCycles=0; 
#endif
  if(pc<himem) 
  {  INSTRUCTION_TIME_ROUND(4);}
  else
  {  INSTRUCTION_TIME_BUS(4);}
}


inline void FetchTimingL() {
  FetchTiming();
  FetchTiming();
}

inline void ReadBusTiming() {
#if defined(SSE_BLT_390B)
  Blit.BlitCycles=0;
#endif
  if(abus<himem)
   { INSTRUCTION_TIME_ROUND(4);}
  else
   { INSTRUCTION_TIME_BUS(4);}
}

inline void ReadBusTimingL() {
  ReadBusTiming();
  // shouldn't we do abus+=2? or should we transform CPU_ABUS_ACCESS_WRITE_L
  // into 2 CPU_ABUS_ACCESS_WRITE?
  ReadBusTiming();
}

inline void WriteBusTiming() {
#if defined(SSE_BLT_390B)
  Blit.BlitCycles=0;
#endif
  if(abus<himem)
   { INSTRUCTION_TIME_ROUND(4);}
  else
    {INSTRUCTION_TIME_BUS(4);}
}

inline void WriteBusTimingL() {
  WriteBusTiming();
  WriteBusTiming();
}

inline void StackTiming() {
#if defined(SSE_BLT_390B)
  Blit.BlitCycles=0;
#endif
  if((MEM_ADDRESS)r[15]<himem)
  {  INSTRUCTION_TIME_ROUND(4);}
  else
  {  INSTRUCTION_TIME_BUS(4);}
}

inline void StackTimingL() {
  StackTiming();
  StackTiming();
}


#elif defined(SSE_MMU_ROUNDING_BUS)

/*
Note on rounding

You need "round cycles up to 4" only for RAM and Shifter accesses.

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
Though there may be wait states for 8bit peripherals.


ijor:
The scheme is a bit misleading. 
MMU doesn't really sit between the CPU and RAM, those buffers are.

There are two data buses in the ST, the main CPU bus, and the RAM/SHIFTER
(and nothing else) bus. Four TTL chips (two buffers and two latches) 
connect or separate both buses. MMU is connected to the main data bus. 
But it controls the buffers and the RAM address bus.
*/

//pc is of course up-to-date
//we don't round on palette, but it's done in ior (?)


inline void FetchTiming() {
  cpu_cycles-=4;
  if(pc<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif
  }
}


inline void FetchTimingL() {
  cpu_cycles-=8;
  if(pc<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif
  }
}

inline void ReadBusTiming() {
  cpu_cycles-=4;
  if(abus<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
/*  As we can see, this feature costs a lot in overhead, but that's the price
    of "correct" emulation.
*/
    Blit.BlitCycles=0;
#endif
  }
}

inline void ReadBusTimingL() {
  cpu_cycles-=8;
  if(abus<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif
  }
}

inline void WriteBusTiming() {
  cpu_cycles-=4;
  if(abus<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif
  }
}

inline void WriteBusTimingL() {
  cpu_cycles-=8;
  if(abus<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif
  }
}

inline void StackTiming() {
  cpu_cycles-=4;
  if((MEM_ADDRESS)r[15]<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif
  }
}

inline void StackTimingL() {
  cpu_cycles-=8;
  if((MEM_ADDRESS)r[15]<himem)
  {
    cpu_cycles&=-4;
#if defined(SSE_BLT_390B)
    Blit.BlitCycles=0;
#endif
  }
}

#else

inline void FetchTiming() {
  if(pc>=rom_addr && pc<rom_addr+tos_len)
    INSTRUCTION_TIME(4);
  else
    INSTRUCTION_TIME_ROUND(4);
}


inline void FetchTimingL() {
  if(pc>=rom_addr && pc<rom_addr+tos_len)
    INSTRUCTION_TIME(8);
  else
    INSTRUCTION_TIME_ROUND(8);
}

#endif


inline void m68k_poke_abus(BYTE x){
  abus&=0xffffff;
  if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_b(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_MMU_RAM_TEST3)
  }else if(abus>=himem || mmu_confused) {
#else
  }else if(abus>=himem) {
#endif
    if (mmu_confused){
      mmu_confused_set_dest_to_addr(1,true);
      m68k_DEST_B=x;
    }else 
    if (abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
#if !defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_B(abus);
#endif
#if defined(SSE_CPU_CHECK_VIDEO_RAM)
    if(Glue.FetchingLine()
      && abus>=shifter_draw_pointer
      && abus<shifter_draw_pointer_at_start_of_line+LINECYCLES/2)
      Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
    if (abus>=MEM_START_OF_USER_AREA||SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      PEEK(abus)=x;
    else 
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_B(abus);
#endif
  }
}


inline void m68k_dpoke_abus(WORD x){
  abus&=0xffffff;
  if(abus&1) exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_w(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_MMU_RAM_TEST3)
  }else if(abus>=himem || mmu_confused) {
#else
  }else if(abus>=himem) {
#endif
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(2,true);
      m68k_DEST_W=x;
    }else if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
#if !defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_W(abus);
#endif
#if defined(SSE_CPU_CHECK_VIDEO_RAM) 
/*  If we're going to write in video RAM of the current scanline,
    we check whether we need to render before. Some programs write
    just after the memory has been fetched, but Steem renders at
    shift mode changes, and if nothing happens, at the end of the line.
    So if we do nothing it will render wrong memory.
    The test isn't perfect and will cause some "false alerts" but
    we have performance in mind: CPU poke is used a lot, it is rare
    when the address bus is around the current scanline.
    Fixes ULM's 3615GEN4 demo.
*/
    if(Glue.FetchingLine()
      && abus>=shifter_draw_pointer
      && abus<shifter_draw_pointer_at_start_of_line+LINECYCLES/2)
      Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
    if(abus>=MEM_START_OF_USER_AREA||SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      DPEEK(abus)=x;
    else 
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_W(abus);
#endif

  }
}


inline void m68k_lpoke_abus(LONG x){
  abus&=0xffffff;
  if(abus&1)exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_l(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_MMU_RAM_TEST3)
  }else if(abus>=himem || mmu_confused) {
#else
  }else if(abus>=himem) {
#endif
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(4,true);
      m68k_DEST_L=x;
    }else if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
#if !defined(SSE_BOILER_MONITOR_VALUE4) //??
    DEBUG_CHECK_WRITE_L(abus);
#endif
#if defined(SSE_CPU_CHECK_VIDEO_RAM)
    if(Glue.FetchingLine()
      && abus>=shifter_draw_pointer
      && abus<shifter_draw_pointer_at_start_of_line+LINECYCLES/2)
      Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
    if(abus>=MEM_START_OF_USER_AREA||SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      LPEEK(abus)=x;
    else 
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_L(abus);
#endif

  }
}


inline  void m68k_poke(MEM_ADDRESS ad,BYTE x){
  abus=ad;
  m68k_poke_abus(x);
}


inline  void m68k_dpoke(MEM_ADDRESS ad,WORD x){
  abus=ad;
  m68k_dpoke_abus(x);
}


inline  void m68k_lpoke(MEM_ADDRESS ad,LONG x){
  abus=ad;
  m68k_lpoke_abus(x);
}

#else

#ifdef SSE_BUILD
void m68k_poke(MEM_ADDRESS ad,BYTE x);
void m68k_dpoke(MEM_ADDRESS ad,WORD x);
void m68k_lpoke(MEM_ADDRESS ad,LONG x);
void m68k_poke_abus(BYTE x);
void m68k_dpoke_abus(WORD x);
void m68k_lpoke_abus(LONG x);
#else
NOT_DEBUG(inline) void m68k_poke(MEM_ADDRESS ad,BYTE x);
NOT_DEBUG(inline) void m68k_dpoke(MEM_ADDRESS ad,WORD x);
NOT_DEBUG(inline) void m68k_lpoke(MEM_ADDRESS ad,LONG x);
NOT_DEBUG(inline) void m68k_poke_abus(BYTE x);
NOT_DEBUG(inline) void m68k_dpoke_abus(WORD x);
NOT_DEBUG(inline) void m68k_lpoke_abus(LONG x);
#endif

#endif

#if !defined(SSE_CPU)

#define INSTRUCTION_TIME(t) {cpu_cycles-=(t);}
#define INSTRUCTION_TIME_ROUND(t) {INSTRUCTION_TIME(t); cpu_cycles&=-4;}

#endif

#ifdef DEBUG_BUILD
#include "debug_emu.decla.h"
void log_history(int bombs,MEM_ADDRESS crash_address);

/*
This is for the "stop on change to user mode" and "stop on next program run" options.
Whenever an instruction that can change SR is executed we check for SUPER change. If
a change to user mode has happened then stop_on_user_change is set to 2. Then when we
check for interrupts (after the current instruction) we see if an interrupt has happened
and if it hasn't we stop. If it has happened we return stop_on_user_change to 1 and wait
for the next change to user mode (when the interrupt has finished).
*/

#define CHECK_STOP_ON_USER_CHANGE  \
            if (stop_on_user_change){ \
              if ((debug_old_sr & SR_SUPER) && (sr & SR_SUPER)==0) stop_on_user_change=2;  \
            }

#define CHECK_STOP_USER_MODE_NO_INTR \
            if (stop_on_user_change==2){  \
              if ((sr & SR_SUPER)==0){  \
                 if (runstate==RUNSTATE_RUNNING){ \
                  runstate=RUNSTATE_STOPPING; \
                  SET_WHY_STOP( HEXSl(old_pc,6)+": Switch to user mode" ) \
                } \
                if (stop_on_next_program_run==2) stop_new_program_exec(); \
              }else{ \
                stop_on_user_change=1; \
              }  \
            }

#ifndef RELEASE_BUILD

extern MEM_ADDRESS pc_rel_stop_on_ref;

#define PC_RELATIVE_MONITOR(ad) \
  if (pc_rel_stop_on_ref){ \
    if ((ad & 0xffffff)==pc_rel_stop_on_ref){ \
      if (runstate==RUNSTATE_RUNNING){ \
        runstate=RUNSTATE_STOPPING; \
        runstate_why_stop=HEXSl(old_pc,6)+": Referencing "+HEXSl(pc_rel_stop_on_ref,6); \
      } \
    } \
  }

#else
#define PC_RELATIVE_MONITOR(ad)
#endif//RELEASE_BUILD

#else // !DEBUG_BUILD

#define CHECK_STOP_ON_USER_CHANGE
#define CHECK_STOP_USER_MODE_NO_INTR
#define debug_check_break_on_irq(irq)
#define PC_RELATIVE_MONITOR(ad)

#endif//DEBUG_BUILD


#define DEST_IS_REGISTER ((ir&BITS_543)<=BITS_543_001)
#define DEST_IS_MEMORY ((ir&BITS_543)>BITS_543_001)
#define SOURCE_IS_REGISTER_OR_IMMEDIATE ((ir & BITS_543)<=BITS_543_001 || ((ir&b00111111)==b00111100) )

#if defined(SSE_CPU)

#define DEST_IS_DATA_REGISTER ((ir&BITS_543)==BITS_543_000)
#define DEST_IS_ADDRESS_REGISTER ((ir&BITS_543)==BITS_543_001)

#endif


#if !defined(SSE_CPU)

#define FETCH_TIMING {INSTRUCTION_TIME(4); cpu_cycles&=-4;} 

#endif

#if defined(SSE_CPU)

inline void SetDestBToAddr() {
  abus&=0xffffff;                                   
  if(abus>=MEM_IO_BASE){               
    if(SUPERFLAG){                        
      ioaccess&=IOACCESS_FLAGS_MASK; 
      ioaccess|=1;                     
      ioad=abus;                        
      m68k_dest=&iobuffer;               
      DWORD_B_0(&iobuffer)=io_read_b(abus);        
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);             
#if defined(SSE_MMU_RAM_TEST3)
  }else if(abus>=himem || mmu_confused) {
#else
  }else if(abus>=himem) {
#endif                            
    if(mmu_confused){                               
      mmu_confused_set_dest_to_addr(1,true);           
    }else if(abus>=FOUR_MEGS){                                                
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               
    }else{                                                        
      m68k_dest=&iobuffer;                             
    }                                       
  }else{                                            
#if !defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_B(abus); 
#endif
    if (abus>=MEM_START_OF_USER_AREA||SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){                             
      m68k_dest=lpPEEK(abus);           
    }else{                                      
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       
    }                                           
#if defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_B(abus); 
#endif
  }
}

inline void SetDestWToAddr() {
  abus&=0xffffff;                                   
  if(abus&1){                                      
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);    
  }else if(abus>=MEM_IO_BASE){               
    if(SUPERFLAG){                        
      ioaccess&=IOACCESS_FLAGS_MASK; 
      ioaccess|=2;                     
      ioad=abus;                        
      m68k_dest=&iobuffer;               
      *((WORD*)&iobuffer)=io_read_w(abus);        
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                                
#if defined(SSE_MMU_RAM_TEST3)
  }else if(abus>=himem || mmu_confused) {
#else
  }else if(abus>=himem) {
#endif
    if(mmu_confused){                               
      mmu_confused_set_dest_to_addr(2,true);           
    }else if(abus>=FOUR_MEGS){                                                
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               
    }else{                                                        
      m68k_dest=&iobuffer;                             
    }                                       
  }else{                   
#if !defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_W(abus);  
#endif
    if(abus>=MEM_START_OF_USER_AREA||SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){  
      m68k_dest=lpDPEEK(abus);           
    }else{                                      
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       
    }                                           
#if defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_W(abus);  
#endif
  }

}

inline void SetDestLToAddr() {
  abus&=0xffffff;                                   
  if(abus&1){                                      
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);    
  }else if(abus>=MEM_IO_BASE){               
    if(SUPERFLAG){                        
      ioaccess&=IOACCESS_FLAGS_MASK; 
      ioaccess|=4;                     
      ioad=abus;                         
      m68k_dest=&iobuffer;               
      iobuffer=io_read_l(abus);        
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                                 
#if defined(SSE_MMU_RAM_TEST3)
  }else if(abus>=himem || mmu_confused) {
#else
  }else if(abus>=himem) {
#endif 
    if(mmu_confused){                               
      mmu_confused_set_dest_to_addr(4,true);           
    }else if(abus>=FOUR_MEGS){                                                
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               
    }else{                                                        
      m68k_dest=&iobuffer;                             
    }                                       
  }else{                               
#if !defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_L(abus);  
#endif
    if(abus>=MEM_START_OF_USER_AREA||SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){       
      m68k_dest=lpLPEEK(abus);           
    }else{                                      
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       
    }                                           
#if defined(SSE_BOILER_MONITOR_VALUE4)
    DEBUG_CHECK_WRITE_L(abus);  
#endif
  }
}

#define m68k_SET_DEST_B_TO_ADDR SetDestBToAddr();
#define m68k_SET_DEST_W_TO_ADDR SetDestWToAddr();
#define m68k_SET_DEST_L_TO_ADDR SetDestLToAddr();

#else

#define m68k_SET_DEST_B_TO_ADDR        \
  abus&=0xffffff;                                   \
  if(abus>=MEM_IO_BASE){               \
    if(SUPERFLAG){                        \
      ioaccess&=IOACCESS_FLAGS_MASK; \
      ioaccess|=1;                     \
      ioad=abus;                        \
      m68k_dest=&iobuffer;               \
      DWORD_B_0(&iobuffer)=io_read_b(abus);        \
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);             \
  }else if(abus>=himem){                               \
    if(mmu_confused){                               \
      mmu_confused_set_dest_to_addr(1,true);           \
    }else if(abus>=FOUR_MEGS){                                                \
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               \
    }else{                                                        \
      m68k_dest=&iobuffer;                             \
    }                                       \
  }else{                                            \
    DEBUG_CHECK_WRITE_B(abus); \
    if (SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){                             \
      m68k_dest=lpPEEK(abus);           \
    }else if(abus>=MEM_START_OF_USER_AREA){ \
      m68k_dest=lpPEEK(abus);           \
    }else{                                      \
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       \
    }                                           \
  }

#define m68k_SET_DEST_W_TO_ADDR        \
  abus&=0xffffff;                                   \
  if(abus&1){                                      \
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);    \
  }else if(abus>=MEM_IO_BASE){               \
    if(SUPERFLAG){                        \
      ioaccess&=IOACCESS_FLAGS_MASK; \
      ioaccess|=2;                     \
      ioad=abus;                        \
      m68k_dest=&iobuffer;               \
      *((WORD*)&iobuffer)=io_read_w(abus);        \
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                                \
  }else if(abus>=himem){                               \
    if(mmu_confused){                               \
      mmu_confused_set_dest_to_addr(2,true);           \
    }else if(abus>=FOUR_MEGS){                                                \
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               \
    }else{                                                        \
      m68k_dest=&iobuffer;                             \
    }                                       \
  }else{                               \
    DEBUG_CHECK_WRITE_W(abus);  \
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){                       \
      m68k_dest=lpDPEEK(abus);           \
    }else if(abus>=MEM_START_OF_USER_AREA){ \
      m68k_dest=lpDPEEK(abus);           \
    }else{                                      \
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       \
    }                                           \
  }

#define m68k_SET_DEST_L_TO_ADDR        \
  abus&=0xffffff;                                   \
  if(abus&1){                                      \
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);    \
  }else if(abus>=MEM_IO_BASE){               \
    if(SUPERFLAG){                        \
      ioaccess&=IOACCESS_FLAGS_MASK; \
      ioaccess|=4;                     \
      ioad=abus;                         \
      m68k_dest=&iobuffer;               \
      iobuffer=io_read_l(abus);        \
    }else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                                 \
  }else if(abus>=himem){                               \
    if(mmu_confused){                               \
      mmu_confused_set_dest_to_addr(4,true);           \
    }else if(abus>=FOUR_MEGS){                                                \
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);                               \
    }else{                                                        \
      m68k_dest=&iobuffer;                             \
    }                                       \
  }else{                               \
    DEBUG_CHECK_WRITE_L(abus);  \
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE){                       \
      m68k_dest=lpLPEEK(abus);           \
    }else if(abus>=MEM_START_OF_USER_AREA){ \
      m68k_dest=lpLPEEK(abus);           \
    }else{                                      \
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);       \
    }                                           \
  }

#endif

#if defined(SSE_MMU_ROUNDING_BUS) && defined(SSE_MMU_ROUNDING_BUS)

#define m68k_SET_DEST_B(abus) m68k_SET_DEST_B_TO_ADDR;

#define m68k_SET_DEST_W(abus) m68k_SET_DEST_W_TO_ADDR;

#define m68k_SET_DEST_L(abus) m68k_SET_DEST_L_TO_ADDR;

#else

#define m68k_SET_DEST_B(addr)           \
  abus=addr;                            \
  m68k_SET_DEST_B_TO_ADDR;

#define m68k_SET_DEST_W(addr)           \
  abus=addr;                            \
  m68k_SET_DEST_W_TO_ADDR;

#define m68k_SET_DEST_L(addr)           \
  abus=addr;                            \
  m68k_SET_DEST_L_TO_ADDR;

#endif

#if defined(SSE_CPU)

inline void FetchWord(WORD &dest_word) {
  dest_word=IR; //get first word of prefetch queue
  if(prefetched_2)// second word was prefetched
  {
    IR=IRC; // IRC->IR
    prefetched_2=false;
  }
#if defined(SSE_CPU_FETCH_IO)
  // some programs like to set PC on Shifter or PSG registers!
  else if(pc>=MEM_IO_BASE)
#if defined(SSE_CPU_FETCH_IO_391)
/*  In Steem, pc is incremented after fetching, another variable is used.
    White Spirit was broken because of this (wrong stack frame).
    It makes no difference for the Union Demo and Warp that are happy provided
    there's a crash.
*/
    IR=io_read_w(pc+2); 
#else
    IR=io_read_w(pc);
#endif
#endif
#if defined(SSE_CPU_FETCH_80000)
  // no crash if there's no RAM
  else if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
    IR=0xFFFF; // TODO - value not correct
#endif
  else
    IR=*lpfetch; // next instr or imm
#if defined(SSE_CPU_FETCH_IO)
  if(pc<MEM_IO_BASE)
#endif
  {
    lpfetch+=MEM_DIR; // advance the fetch pointer
    if(lpfetch MEM_GE lpfetch_bound) // MEM_GE : <=
      ::exception(BOMBS_BUS_ERROR,EA_FETCH,pc); // :: for gcc "ambiguous" ?
  }
}


#define FETCH_W(dest_word) FetchWord(dest_word);

#else

#define FETCH_W(dest_word)              \
  if(prefetched_2){                     \
    dest_word=prefetch_buf[0];            \
    prefetch_buf[0]=prefetch_buf[1];       \
    prefetched_2=false;                           \
  }else{ /* if(prefetched==1) */             \
    dest_word=prefetch_buf[0];                \
    prefetch_buf[0]=*lpfetch;              \
  }                                            \
  lpfetch+=MEM_DIR;                             \
  if(lpfetch MEM_GE lpfetch_bound)exception(BOMBS_BUS_ERROR,EA_FETCH,pc);
#endif



#if defined(SSE_CPU)

inline void PrefetchIrc() {
  ASSERT(!prefetched_2); // strong, only once per instruction 
  FetchTiming(); // (inline)
#if defined(SSE_CPU_FETCH_IO)
  if(pc>=MEM_IO_BASE)
#if defined(SSE_CPU_FETCH_IO_391)
    IRC=io_read_w(pc+2); // see FetchWord()
#else
    IRC=io_read_w(pc); 
#endif
  else
#endif
#if defined(SSE_CPU_FETCH_80000)
  if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
    IRC=0xFFFF; // TODO - value not correct
  else
#endif
    IRC=*lpfetch;
  prefetched_2=true;
}

#define PREFETCH_IRC PrefetchIrc();

/*  This is no fix, "refetch" was already in Steem 3.2, just rewritten.
    Cases: 
    Synth Dream Sound Demo II
*/

inline void RefetchIr() {
  ASSERT( IR==*(lpfetch+1) ); //detect cases
  IR=*(lpfetch-MEM_DIR);
  // we count fetch timing here (390)
  FetchTiming();
}

#define REFETCH_IR  RefetchIr();

#else

#define EXTRA_PREFETCH                    \
  prefetch_buf[1]=*lpfetch;              \
  prefetched_2=true;

#endif



#ifdef ENABLE_LOGFILE
#define IOACCESS_DEBUG_MEM_WRITE_LOG BIT_14
#if defined(SSE_BOILER_MONITOR_TRACE)
#define IOACCESS_DEBUG_MEM_READ_LOG BIT_15 //no conflict I hope...
#endif
extern MEM_ADDRESS debug_mem_write_log_address;
extern int debug_mem_write_log_bytes;
#endif

#define STOP_INTS_BECAUSE_INTERCEPT_OS bool(ioaccess & (IOACCESS_INTERCEPT_OS | IOACCESS_INTERCEPT_OS2))

#if defined(SSE_CPU)
inline void change_to_user_mode();
inline void change_to_supervisor_mode();
#endif

extern bool cpu_stopped;
extern signed int compare_buffer;

#define PC_RELATIVE_PC pc
//(old_pc+2)
//(old_dpc+2)

#if defined(SSE_CPU)

#define m68k_GET_DEST_B m68kGetDestByte();
#define m68k_GET_DEST_W m68kGetDestWord();
#define m68k_GET_DEST_L m68kGetDestLong();
#define m68k_GET_DEST_B_NOT_A m68kGetDestByteNotA();
#define m68k_GET_DEST_W_NOT_A m68kGetDestWordNotA();
#define m68k_GET_DEST_L_NOT_A m68kGetDestLongNotA();
#define m68k_GET_DEST_B_NOT_A_OR_D m68kGetDestByteNotAOrD();
#define m68k_GET_DEST_W_NOT_A_OR_D m68kGetDestWordNotAOrD();
#define m68k_GET_DEST_L_NOT_A_OR_D m68kGetDestLongNotAOrD();
#define m68k_GET_SOURCE_B m68kGetSourceByte();
#define m68k_GET_SOURCE_W m68kGetSourceWord();
#define m68k_GET_SOURCE_L m68kGetSourceLong();
#define m68k_GET_SOURCE_B_NOT_A m68kGetSourceByteNotA();
#define m68k_GET_SOURCE_W_NOT_A m68kGetSourceWordNotA();
#define m68k_GET_SOURCE_L_NOT_A m68kGetSourceLongNotA();

inline void m68kGetDestByte() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_b[(ir&BITS_543)>>3]();
}


inline void m68kGetDestWord() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_w[(ir&BITS_543)>>3]();
}


inline void m68kGetDestLong() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_l[(ir&BITS_543)>>3]();
}


inline void m68kGetDestByteNotA() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a[(ir&BITS_543)>>3]();
}


inline void m68kGetDestWordNotA() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a[(ir&BITS_543)>>3]();
}


inline void m68kGetDestLongNotA() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a[(ir&BITS_543)>>3]();
}


inline void m68kGetDestByteNotAOrD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_b_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void m68kGetDestWordNotAOrD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_w_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void m68kGetDestLongNotAOrD() {
#if defined(SSE_CPU_TRUE_PC)
  if(!M68000.CheckRead)
    M68000.Pc=pc+2;
#endif
  m68k_jump_get_dest_l_not_a_or_d[(ir&BITS_543)>>3]();
}


inline void m68kGetSourceByte() {
  m68k_jump_get_source_b[(ir&BITS_543)>>3]();
}


inline void m68kGetSourceWord() {
  m68k_jump_get_source_w[(ir&BITS_543)>>3]();
}


inline void m68kGetSourceLong() {
  m68k_jump_get_source_l[(ir&BITS_543)>>3]();
}


inline void m68kGetSourceByteNotA() {
  m68k_jump_get_source_b_not_a[(ir&BITS_543)>>3]();
}


inline void m68kGetSourceWordNotA() {
  m68k_jump_get_source_w_not_a[(ir&BITS_543)>>3]();
}


inline void m68kGetSourceLongNotA() {
  m68k_jump_get_source_l_not_a[(ir&BITS_543)>>3]();
}


#else
#define m68k_GET_SOURCE_B m68k_jump_get_source_b[(ir&BITS_543)>>3]()
#define m68k_GET_SOURCE_W m68k_jump_get_source_w[(ir&BITS_543)>>3]()
#define m68k_GET_SOURCE_L m68k_jump_get_source_l[(ir&BITS_543)>>3]()

#define m68k_GET_SOURCE_B_NOT_A m68k_jump_get_source_b_not_a[(ir&BITS_543)>>3]()
#define m68k_GET_SOURCE_W_NOT_A m68k_jump_get_source_w_not_a[(ir&BITS_543)>>3]()
#define m68k_GET_SOURCE_L_NOT_A m68k_jump_get_source_l_not_a[(ir&BITS_543)>>3]()

#define m68k_GET_DEST_B m68k_jump_get_dest_b[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_W m68k_jump_get_dest_w[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_L m68k_jump_get_dest_l[(ir&BITS_543)>>3]()

#define m68k_GET_DEST_B_NOT_A m68k_jump_get_dest_b_not_a[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_W_NOT_A m68k_jump_get_dest_w_not_a[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_L_NOT_A m68k_jump_get_dest_l_not_a[(ir&BITS_543)>>3]()

#define m68k_GET_DEST_B_NOT_A_OR_D m68k_jump_get_dest_b_not_a_or_d[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_W_NOT_A_OR_D m68k_jump_get_dest_w_not_a_or_d[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_L_NOT_A_OR_D m68k_jump_get_dest_l_not_a_or_d[(ir&BITS_543)>>3]()

#define m68k_GET_DEST_B_NOT_A_FASTER_FOR_D m68k_jump_get_dest_b_not_a_faster_for_d[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_W_NOT_A_FASTER_FOR_D m68k_jump_get_dest_w_not_a_faster_for_d[(ir&BITS_543)>>3]()
#define m68k_GET_DEST_L_NOT_A_FASTER_FOR_D m68k_jump_get_dest_l_not_a_faster_for_d[(ir&BITS_543)>>3]()
#endif//!ss-cpu

#define m68k_CONDITION_TEST m68k_jump_condition_test[(ir&0xf00)>>8]()

#define m68k_PUSH_W(x)                   \
    r[15]-=2;abus=r[15];                 \
    m68k_SET_DEST_W_TO_ADDR;             \
    m68k_DEST_W=x;


#define m68k_PUSH_L(x)                   \
    r[15]-=4;abus=r[15];                 \
    m68k_SET_DEST_L_TO_ADDR;             \
    m68k_DEST_L=x;

#define m68k_BIT_SHIFT_TO_dM_GET_SOURCE         \
  if(ir&BIT_5){                               \
    m68k_src_w=(WORD)(r[PARAM_N]&63);                 \
  }else{                                      \
    m68k_src_w=(WORD)PARAM_N;                       \
    if(!m68k_src_w)m68k_src_w=8;              \
  }

#define CCR WORD_B_0(&sr)

#define SR_CLEAR(f) sr&=(unsigned short)(~((unsigned short)f));
#define SR_SET(f) sr|=(unsigned short)(f);

#define SR_VALID_BITMASK 0xa71f

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_CHECK_Z_AND_N_B                   \
  if(_bittest((LONG*)m68k_dest,7)){          \
    BITSET(sr,SR_N_BIT);                            \
  }else if(m68k_DEST_B==0){                  \
    BITSET(sr,SR_Z_BIT);                            \
  }

#define SR_CHECK_Z_AND_N_W                   \
  if(_bittest((LONG*)m68k_dest,0xf)){        \
    BITSET(sr,SR_N_BIT);                            \
  }else if(m68k_DEST_W==0){                  \
    BITSET(sr,SR_Z_BIT);                            \
  }

#define SR_CHECK_Z_AND_N_L                   \
  if(_bittest((LONG*)m68k_dest,31)){         \
    BITSET(sr,SR_N_BIT);                            \
  }else if(m68k_DEST_L==0){                  \
    BITSET(sr,SR_Z_BIT);                            \
  }

#else

#define SR_CHECK_Z_AND_N_B                   \
  if(m68k_DEST_B&BIT_7){                     \
    SR_SET(SR_N);                            \
  }else if(m68k_DEST_B==0){                  \
    SR_SET(SR_Z);                            \
  }

#define SR_CHECK_Z_AND_N_W                   \
  if(m68k_DEST_W&BIT_f){                     \
    SR_SET(SR_N);                            \
  }else if(m68k_DEST_W==0){                  \
    SR_SET(SR_Z);                            \
  }

#define SR_CHECK_Z_AND_N_L                   \
  if((signed long)m68k_DEST_L<0){            \
    SR_SET(SR_N);                            \
  }else if(m68k_DEST_L==0){                  \
    SR_SET(SR_Z);                            \
  }

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_ADD_B                                                        \
  SR_CLEAR(SR_USER_BYTE)                                                \
  if( ( (( m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|                \
        ((~m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){     \
    BITSET(sr,SR_V_BIT);                                                \
  }                                                                     \
  if( ( (( m68k_src_b)&( m68k_old_dest)) |                              \
        ((~m68k_DEST_B)&( m68k_old_dest))|                              \
        (( m68k_src_b)&(~m68k_DEST_B)) ) & MSB_B){                      \
    SR_SET(SR_C+SR_X);                                                  \
  }                                                                     \
  if(!m68k_DEST_B)BITSET(sr,SR_Z_BIT);                                  \
  if(_bittest((LONG*)m68k_dest,7)) BITSET(sr,SR_N_BIT);               

#else
#define SR_ADD_B                                                        \
  SR_CLEAR(SR_USER_BYTE)                                                \
  if( ( (( m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|                \
        ((~m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){     \
    SR_SET(SR_V);                                                       \
  }                                                                     \
  if( ( (( m68k_src_b)&( m68k_old_dest)) |                              \
        ((~m68k_DEST_B)&( m68k_old_dest))|                              \
        (( m68k_src_b)&(~m68k_DEST_B)) ) & MSB_B){                      \
    SR_SET(SR_C+SR_X);                                                  \
  }                                                                     \
  if(!m68k_DEST_B)SR_SET(SR_Z);                                         \
  if(m68k_DEST_B & MSB_B)SR_SET(SR_N);                                  \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_ADDX_B                                                       \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C)                                         \
  if( ( (( m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|                \
        ((~m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){     \
    BITSET(sr,SR_V_BIT);                                    \
  }                                                                     \
  if( ( (( m68k_src_b)&( m68k_old_dest)) |                              \
        ((~m68k_DEST_B)&( m68k_old_dest))|                              \
        (( m68k_src_b)&(~m68k_DEST_B)) ) & MSB_B){                      \
    SR_SET(SR_C+SR_X);                                                  \
  }                                                                     \
  if(BITTEST(sr,SR_Z_BIT))if(m68k_DEST_B)BITRESET(sr,SR_Z_BIT);               \
  if(_bittest((LONG*)m68k_dest,7))BITSET(sr,SR_N_BIT);     

#else

#define SR_ADDX_B                                                       \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C)                                         \
  if( ( (( m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|                \
        ((~m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){     \
    SR_SET(SR_V);                                                       \
  }                                                                     \
  if( ( (( m68k_src_b)&( m68k_old_dest)) |                              \
        ((~m68k_DEST_B)&( m68k_old_dest))|                              \
        (( m68k_src_b)&(~m68k_DEST_B)) ) & MSB_B){                      \
    SR_SET(SR_C+SR_X);                                                  \
  }                                                                     \
  if(sr&SR_Z)if(m68k_DEST_B)SR_CLEAR(SR_Z);                             \
  if(m68k_DEST_B & MSB_B)SR_SET(SR_N);                                  \

#endif

#if defined(SSE_VC_INTRINSICS_390E)
#define SR_ADD_W                                                        \
  SR_CLEAR(SR_USER_BYTE)                                                \
  if( ( (( m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|                \
        ((~m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){     \
    BITSET(sr,SR_V_BIT);                                                       \
  }                                                                     \
  if( ( (( m68k_src_w)&( m68k_old_dest)) |                              \
        ((~m68k_DEST_W)&( m68k_old_dest))|                              \
        (( m68k_src_w)&(~m68k_DEST_W)) ) & MSB_W){                      \
    SR_SET(SR_C+SR_X);                                                  \
  }                                                                     \
  if(!m68k_DEST_W)BITSET(sr,SR_Z_BIT);                                         \
  if(_bittest((LONG*)m68k_dest,15)) BITSET(sr,SR_N_BIT);               

#else
#define SR_ADD_W                                                        \
  SR_CLEAR(SR_USER_BYTE)                                                \
  if( ( (( m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|                \
        ((~m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){     \
    SR_SET(SR_V);                                                       \
  }                                                                     \
  if( ( (( m68k_src_w)&( m68k_old_dest)) |                              \
        ((~m68k_DEST_W)&( m68k_old_dest))|                              \
        (( m68k_src_w)&(~m68k_DEST_W)) ) & MSB_W){                      \
    SR_SET(SR_C+SR_X);                                                  \
  }                                                                     \
  if(!m68k_DEST_W)SR_SET(SR_Z);                                         \
  if(m68k_DEST_W & MSB_W)SR_SET(SR_N);                                  \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_ADDX_W                  \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( (( m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|  \
        ((~m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_w)&( m68k_old_dest)) |  \
        ((~m68k_DEST_W)&( m68k_old_dest))|  \
        (( m68k_src_w)&(~m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(BITTEST(sr,SR_Z_BIT))if(m68k_DEST_W)BITRESET(sr,SR_Z_BIT);                    \
  if(_bittest((LONG*)m68k_dest,15))BITSET(sr,SR_N_BIT);          


#else

#define SR_ADDX_W                  \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( (( m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|  \
        ((~m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_w)&( m68k_old_dest)) |  \
        ((~m68k_DEST_W)&( m68k_old_dest))|  \
        (( m68k_src_w)&(~m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(sr&SR_Z)if(m68k_DEST_W)SR_CLEAR(SR_Z);                                      \
  if(m68k_DEST_W & MSB_W)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)
#define SR_ADD_L           \
  SR_CLEAR(SR_USER_BYTE) \
  if( ( (( m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        ((~m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    BITSET(sr,SR_V_BIT);                                                       \
  }                                                                     \
  if( ( (( m68k_src_l)&( m68k_old_dest)) |  \
        ((~m68k_DEST_L)&( m68k_old_dest))|  \
        (( m68k_src_l)&(~m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(!m68k_DEST_L)BITSET(sr,SR_Z_BIT);                                         \
  if(_bittest((LONG*)m68k_dest,31)) BITSET(sr,SR_N_BIT);               

#else

#define SR_ADD_L           \
  SR_CLEAR(SR_USER_BYTE) \
  if( ( (( m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        ((~m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_l)&( m68k_old_dest)) |  \
        ((~m68k_DEST_L)&( m68k_old_dest))|  \
        (( m68k_src_l)&(~m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(!m68k_DEST_L)SR_SET(SR_Z);                                      \
  if(m68k_DEST_L & MSB_L)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_ADDX_L           \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( (( m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        ((~m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_l)&( m68k_old_dest)) |  \
        ((~m68k_DEST_L)&( m68k_old_dest))|  \
        (( m68k_src_l)&(~m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(BITTEST(sr,SR_Z_BIT))if(m68k_DEST_L)BITRESET(sr,SR_Z_BIT);                                    \
  if(_bittest((LONG*)m68k_dest,31))BITSET(sr,SR_N_BIT);                               \


#else

#define SR_ADDX_L           \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( (( m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        ((~m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_l)&( m68k_old_dest)) |  \
        ((~m68k_DEST_L)&( m68k_old_dest))|  \
        (( m68k_src_l)&(~m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(sr&SR_Z)if(m68k_DEST_L)SR_CLEAR(SR_Z);                                      \
  if(m68k_DEST_L & MSB_L)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_SUB_B(extend_flag)           \
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C+extend_flag) \
  if( ( ((~m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|  \
        (( m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_b)&(~m68k_old_dest)) |  \
        (( m68k_DEST_B)&(~m68k_old_dest))|  \
        (( m68k_src_b)&( m68k_DEST_B)) ) & MSB_B){  \
    SR_SET(SR_C+extend_flag);                                                 \
  }                                                                     \
  if(!m68k_DEST_B)BITSET(sr,SR_Z_BIT);                                      \
  if(_bittest((LONG*)m68k_dest,7)) BITSET(sr,SR_N_BIT); 

#else

#define SR_SUB_B(extend_flag)           \
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C+extend_flag) \
  if( ( ((~m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|  \
        (( m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_b)&(~m68k_old_dest)) |  \
        (( m68k_DEST_B)&(~m68k_old_dest))|  \
        (( m68k_src_b)&( m68k_DEST_B)) ) & MSB_B){  \
    SR_SET(SR_C+extend_flag);                                                 \
  }                                                                     \
  if(!m68k_DEST_B)SR_SET(SR_Z);                                      \
  if(m68k_DEST_B & MSB_B)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_SUBX_B                                                      \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( ((~m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|  \
        (( m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_b)&(~m68k_old_dest)) |  \
        (( m68k_DEST_B)&(~m68k_old_dest))|  \
        (( m68k_src_b)&( m68k_DEST_B)) ) & MSB_B){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(BITTEST(sr,SR_Z_BIT))if(m68k_DEST_B)BITRESET(sr,SR_Z_BIT);                                  \
  if(_bittest((LONG*)m68k_dest,7))BITSET(sr,SR_N_BIT);                               \


#else

#define SR_SUBX_B                                                      \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( ((~m68k_src_b)&( m68k_old_dest)&(~m68k_DEST_B))|  \
        (( m68k_src_b)&(~m68k_old_dest)&( m68k_DEST_B)) ) & MSB_B){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_b)&(~m68k_old_dest)) |  \
        (( m68k_DEST_B)&(~m68k_old_dest))|  \
        (( m68k_src_b)&( m68k_DEST_B)) ) & MSB_B){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(sr&SR_Z)if(m68k_DEST_B)SR_CLEAR(SR_Z);                                      \
  if(m68k_DEST_B & MSB_B)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_SUB_W(extend_flag)           \
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C+extend_flag)                             \
  if( ( ((~m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|  \
        (( m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_w)&(~m68k_old_dest)) |  \
        (( m68k_DEST_W)&(~m68k_old_dest))|  \
        (( m68k_src_w)&( m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_C+extend_flag);                                                 \
  }                                                                     \
  if(!m68k_DEST_W)BITSET(sr,SR_Z_BIT);                                      \
  if(_bittest((LONG*)m68k_dest,15)) BITSET(sr,SR_N_BIT);

#else

#define SR_SUB_W(extend_flag)           \
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C+extend_flag)                             \
  if( ( ((~m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|  \
        (( m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_w)&(~m68k_old_dest)) |  \
        (( m68k_DEST_W)&(~m68k_old_dest))|  \
        (( m68k_src_w)&( m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_C+extend_flag);                                                 \
  }                                                                     \
  if(!m68k_DEST_W)SR_SET(SR_Z);                                      \
  if(m68k_DEST_W & MSB_W)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_SUBX_W           \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( ((~m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|  \
        (( m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_w)&(~m68k_old_dest)) |  \
        (( m68k_DEST_W)&(~m68k_old_dest))|  \
        (( m68k_src_w)&( m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(BITTEST(sr,SR_Z_BIT))if(m68k_DEST_W)BITRESET(sr,SR_Z_BIT);                                     \
  if(_bittest((LONG*)m68k_dest,15))BITSET(sr,SR_N_BIT);   


#else

#define SR_SUBX_W           \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( ((~m68k_src_w)&( m68k_old_dest)&(~m68k_DEST_W))|  \
        (( m68k_src_w)&(~m68k_old_dest)&( m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_w)&(~m68k_old_dest)) |  \
        (( m68k_DEST_W)&(~m68k_old_dest))|  \
        (( m68k_src_w)&( m68k_DEST_W)) ) & MSB_W){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(sr&SR_Z)if(m68k_DEST_W)SR_CLEAR(SR_Z);                                      \
  if(m68k_DEST_W & MSB_W)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_SUB_L(extend_flag)           \
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C+extend_flag)                             \
  if( ( ((~m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        (( m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_l)&(~m68k_old_dest)) |  \
        (( m68k_DEST_L)&(~m68k_old_dest))|  \
        (( m68k_src_l)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+extend_flag);                                                 \
  }                                                                     \
  if(!m68k_DEST_L)BITSET(sr,SR_Z_BIT);                                      \
  if(_bittest((LONG*)m68k_dest,31)) BITSET(sr,SR_N_BIT);

#else

#define SR_SUB_L(extend_flag)           \
  SR_CLEAR(SR_N+SR_Z+SR_V+SR_C+extend_flag)                             \
  if( ( ((~m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        (( m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_l)&(~m68k_old_dest)) |  \
        (( m68k_DEST_L)&(~m68k_old_dest))|  \
        (( m68k_src_l)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+extend_flag);                                                 \
  }                                                                     \
  if(!m68k_DEST_L)SR_SET(SR_Z);                                      \
  if(m68k_DEST_L & MSB_L)SR_SET(SR_N);                               \

#endif

#if defined(SSE_VC_INTRINSICS_390E)

#define SR_SUBX_L           \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( ((~m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        (( m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    BITSET(sr,SR_V_BIT);                                                     \
  }                                                                 \
  if( ( (( m68k_src_l)&(~m68k_old_dest)) |  \
        (( m68k_DEST_L)&(~m68k_old_dest))|  \
        (( m68k_src_l)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(BITTEST(sr,SR_Z_BIT))if(m68k_DEST_L)BITRESET(sr,SR_Z_BIT);                                      \
  if(BITTEST(m68k_dest,31))BITSET(sr,SR_N_BIT);                               \


#else

#define SR_SUBX_L           \
  SR_CLEAR(SR_X+SR_N+SR_V+SR_C) \
  if( ( ((~m68k_src_l)&( m68k_old_dest)&(~m68k_DEST_L))|  \
        (( m68k_src_l)&(~m68k_old_dest)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_V);                                                     \
  }                                                                 \
  if( ( (( m68k_src_l)&(~m68k_old_dest)) |  \
        (( m68k_DEST_L)&(~m68k_old_dest))|  \
        (( m68k_src_l)&( m68k_DEST_L)) ) & MSB_L){  \
    SR_SET(SR_C+SR_X);                                                 \
  }                                                                     \
  if(sr&SR_Z)if(m68k_DEST_L)SR_CLEAR(SR_Z);                                      \
  if(m68k_DEST_L & MSB_L)SR_SET(SR_N);                               \

#endif

#define m68k_GET_IMMEDIATE_B m68k_src_b=m68k_fetchB();pc+=2; 
#define m68k_GET_IMMEDIATE_W m68k_src_w=m68k_fetchW();pc+=2; 
#define m68k_GET_IMMEDIATE_L m68k_src_l=m68k_fetchL();pc+=4; 

#define m68k_IMMEDIATE_B (signed char)m68k_fetchB()   //ss very few calls
#define m68k_IMMEDIATE_W (short)m68k_fetchW() //ss very few calls
#if !defined(SSE_VAR_REWRITE)
#define m68k_IMMEDIATE_L (long)m68k_fetchL() // SS unused
#endif

//SS SR has just been loaded, if supervisor bit in it isn't set, go user mode:
#define DETECT_CHANGE_TO_USER_MODE if (!SUPERFLAG) change_to_user_mode();

#define ILLEGAL  exception(BOMBS_ILLEGAL_INSTRUCTION,EA_INST,0);

#if defined(SSE_VC_INTRINSICS_390E)
#define DETECT_TRACE_BIT {if (BITTEST(sr,SR_TRACE_BIT)) \
  ioaccess=TRACE_BIT_JUST_SET | (ioaccess & IOACCESS_FLAGS_MASK);}
#else
#define DETECT_TRACE_BIT {if (sr & SR_TRACE) ioaccess=TRACE_BIT_JUST_SET | (ioaccess & IOACCESS_FLAGS_MASK);}
#endif

#define TRACE_BIT_JUST_SET 0x2b

//function codes on the m68k
#define FC_USER_DATA 1
#define FC_USER_PROGRAM 2
#define FC_SUPERVISOR_DATA 5
#define FC_SUPERVISOR_PROGRAM 6
#define FC_INTERRUPT_VERIFICATION 7

#if defined(SSE_CPU)
#define SET_PC(ad) m68kSetPC(ad);
#else
//#define SET_PC(ad) set_pc(ad);// stack overflow!
extern void set_pc(MEM_ADDRESS);
#endif

#if !(defined(SSE_CPU))
extern void perform_rte();
#endif

extern void sr_check_z_n_l_for_r0();
extern void m68k_process();


#if defined(SSE_CPU)

inline void m68kPerformRte() {
  // replacing macro M68K_PERFORM_RTE(checkints)
  MEM_ADDRESS pushed_return_address=m68k_lpeek(r[15]+2);
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPop();
#endif
  // An Illegal routine could manipulate this value.
  SET_PC(pushed_return_address);
  sr=m68k_dpeek(r[15]);r[15]+=6;    
#if defined(SSE_BOILER_68030_STACK_FRAME)
  if(Debug.M68030StackFrame)
    r[15]+=2;   
#endif  
  sr&=SR_VALID_BITMASK;               

  DETECT_CHANGE_TO_USER_MODE;         
  DETECT_TRACE_BIT;     

#if defined(SSE_BOILER_TRACE_CONTROL)
  if(TRACE_MASK2&TRACE_CONTROL_RTE)
    TRACE_INT("%d %d %d pc %X RTE to %X\n",TIMING_INFO,old_pc,pushed_return_address);
#endif

#if defined(SSE_BOILER_SHOW_INTERRUPT)
  Debug.Rte();
#endif
}

#define M68K_PERFORM_RTE(checkints) m68kPerformRte();

#else
// SS I can't find one instance where checkints is passed.
#define M68K_PERFORM_RTE(checkints)             \
            SET_PC(m68k_lpeek(r[15]+2));        \
            sr=m68k_dpeek(r[15]);r[15]+=6;      \
            sr&=SR_VALID_BITMASK;               \
            DETECT_CHANGE_TO_USER_MODE;         \
            DETECT_TRACE_BIT;                   \
            checkints;                          \

#endif

#if defined(SSE_CPU)

inline void m68kPrefetchSetPC() { 
  // called by SetPC; we don't count timing here

  // don't use lpfetch for fetching in IO zone, use io_read: fixes Warp 
  // original STX + Lethal Xcess Beta STX
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    prefetch_buf[0]=io_read_w(pc);
    prefetch_buf[1]=io_read_w(pc+2);
  }
  // don't crash, but the correct value would be 'dbus'
  else if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
  {
    prefetch_buf[0]=prefetch_buf[1]=0xFFFF; // default, incorrect?

  }
  else
  {
    prefetch_buf[0]=*lpfetch;
    prefetch_buf[1]=*(lpfetch+MEM_DIR); 
    lpfetch+=MEM_DIR;
  }
  prefetched_2=true;
}
#define PREFETCH_SET_PC m68kPrefetchSetPC();

#else
  #define PREFETCH_SET_PC                       \
  prefetched_2=false; /*will have prefetched 1 word*/ \
  prefetch_buf[0]=*lpfetch;               \
  lpfetch+=MEM_DIR;  /*let's not cause exceptions here*/

#endif

#if !defined(SSE_CPU)
#define SET_PC(ad)        \
    pc=ad;                               \
    pc_high_byte=pc & 0xff000000;     \
    pc&=0xffffff;                    \
    lpfetch=lpDPEEK(0);          /*Default to instant bus error when fetch*/   \
    lpfetch_bound=lpDPEEK(0);         \
                                        \
    if (pc>=himem){                                                       \
      if (pc<MEM_IO_BASE){           \
        if (pc>=MEM_EXPANSION_CARTRIDGE){                                \
          if (pc>=0xfc0000){                                                   \
            if (tos_high && pc<(0xfc0000+192*1024)){         \
              lpfetch=lpROM_DPEEK(pc-0xfc0000); \
              lpfetch_bound=lpROM_DPEEK(192*1024);         \
            }                                                                          \
          }else if (cart){                                                          \
            lpfetch=lpCART_DPEEK(pc-MEM_EXPANSION_CARTRIDGE); \
            lpfetch_bound=lpCART_DPEEK(128*1024);         \
          }                                                                          \
        }else if(pc>=rom_addr){                                                      \
          if (pc<(0xe00000 + 256*1024)){         \
            lpfetch=lpROM_DPEEK(pc-0xe00000); \
            lpfetch_bound=lpROM_DPEEK(256*1024);         \
          }                                                                          \
        }                            \
      }else{   \
        if (pc>=0xff8240 && pc<0xff8260){         \
          lpfetch=lpPAL_DPEEK(pc-0xff8240); \
          lpfetch_bound=lpPAL_DPEEK(64+PAL_EXTRA_BYTES);         \
        }                              \
      }                                                                           \
    }else{                                                                         \
      lpfetch=lpDPEEK(pc); \
      lpfetch_bound=lpDPEEK(mem_len+(MEM_EXTRA_BYTES/2));         \
    }                                         \
    PREFETCH_SET_PC
#endif

//---------------------------------------------------------------------------
#ifdef SSE_STRUCTURE_DECLA
#if defined(SSE_CPU)
inline void change_to_user_mode()
{
  compare_buffer=r[15];r[15]=other_sp;other_sp=compare_buffer;
#if defined(SSE_VC_INTRINSICS_390E)
  BITRESET(sr,SR_SUPER_BIT);
#else
  SR_CLEAR(SR_SUPER);
#endif
}
inline void change_to_supervisor_mode()
{
  compare_buffer=r[15];r[15]=other_sp;other_sp=compare_buffer;
#if defined(SSE_VC_INTRINSICS_390E)
  BITSET(sr,SR_SUPER_BIT);
#else
  SR_SET(SR_SUPER);
#endif
}
#endif
inline void sr_check_z_n_l_for_r0()
{
  m68k_dest=&r[0];
  SR_CHECK_Z_AND_N_L;
}
#endif
//---------------------------------------------------------------------------
#if defined(SSE_CPU)

#if defined(SSE_MMU_ROUNDING_BUS)

inline void ReadB() {
  m68k_src_b=m68k_peek(abus);
}

inline void ReadW() {
  m68k_src_w=m68k_dpeek(abus);
}

inline void ReadL() {
  m68k_src_l=m68k_lpeek(abus);
}

#else

inline void ReadB(MEM_ADDRESS addr) {
  m68k_src_b=m68k_peek(addr);
}

inline void ReadW(MEM_ADDRESS addr) {
  m68k_src_w=m68k_dpeek(addr);
}

inline void ReadL(MEM_ADDRESS addr) {
  m68k_src_l=m68k_lpeek(addr);
}

#endif

#if defined(SSE_MMU_ROUNDING_BUS)

#define m68k_READ_B(abus) ReadB();
#define m68k_READ_W(abus) ReadW();
#define m68k_READ_L(abus) ReadL();

#else

#define m68k_READ_B(addr) ReadB(addr);
#define m68k_READ_W(addr) ReadW(addr);
#define m68k_READ_L(addr) ReadL(addr);

#endif

void m68k_interrupt(MEM_ADDRESS ad);

inline void m68kUnstop() {
  if(cpu_stopped)
  {
    cpu_stopped=false;     
    m68kSetPC((pc+4) | pc_high_byte); 
  }
}
#define M68K_UNSTOP m68kUnstop();

//TODO should it be inline?
inline void m68kTrapTiming() {
/*
Motorola
Exception Processing Execution Times	

Exception 	Periods
Address Error	50(4/7)
Bus Error	50(4/7)
CHK Instruction	40(4/3)+
Divide by Zero	38(4/3)+
Illegal Instruction	34(4/3)
Interrupt	44(5/3)*
Privilege Violation	34(4/3)
RESET**	40(6/0)
Trace	34(4/3)
TRAP	34(4/3)
TRAPV	34(5/3)


Yacht
******************************************************************************* 
                      EXCEPTION PROCESSING EXECUTION TIMES                  
******************************************************************************* 
------------------------------------------------------------------------------- 
                      | Periods  |               Data Bus Usage              
----------------------+----------+--------------------------------------------- 
                      |          |                                           
Group 0 :             |          |
  Address error       | 50(4/7)  |     nn ns nS ns ns ns nS ns nV nv np np      
  Bus error           | 50(4/7)  |     nn ns nS ns ns ns nS ns nV nv np np      
  /RESET              | 40(6/0)  |             (n-)*5 nn nF nf nV nv np np      
Group 1 :             |          |
  Trace               | 34(4/3)  |              nn    ns nS ns nV nv np np      
  Interrupt           | 44(5/3)  |      n nn ns ni n-  n nS ns nV nv np np      
  Privilege Violation | 34(4/3)  |              nn    ns nS ns nV nv np np      
  Illegal Instruction | 34(4/3)  |              nn    ns nS ns nV nv np np      
Group 2 :             |          |
  CHK Instruction     | 40(4/3)+ |   np (n-)    nn    ns nS ns nV nv np np      
  Divide by Zero      | 38(4/3)+ |           nn nn    ns nS ns nV nv np np      
  TRAP instruction    | 34(4/3)  |              nn    ns nS ns nV nv np np      
  TRAPV instruction   | 34(5/3)  |   np               ns nS ns nV nv np np      
NOTES:       
  .for CHK, Divide by Zero, Illegal, Trap and TRAPV :
    .The numbers of clock periods, for the "trap" lines, include the times for 
     all stacking, the vector fetch and the fetch of the first instruction of 
     THE handler routine.
    .First are pushed on the stack the program counter high and low words then 
     the status register.
  .for CHK and Divide by zero, you must add effective address calculation time
   to the period (according to instruction section above).
  ./RESET line indicates the time from when /RESET and /HALT are first sampled
   as negated to when instruction execution starts. For a subsequent external 
   reset, asserting these signals for 10 clock cycles or longer is mandatory to 
   initiate reset sequence. During these 10 clock cycles the CPU is in halted 
   state (see below). However, an external reset signal that is asserted while 
   the CPU is executing a reset instruction is ignored. Since the processor 
   asserts the /RESET signal for 124 clock cycles during execution of a reset 
   instruction, an external reset should assert /RESET for at least 132 clock 
   periods.
  .For all these exceptions, there is a difference of 2 cycles between Data 
   bus usage as obtained from USP4325121 and periods as written in M68000UM.
   There's no proven theory to explain this gap.

WinUAE
- 4 idle cycles
- write PC low word
- write SR
- write PC high word                       [wrong order...is this credible?]
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch

    So, in the YACHT table, a n would be missing between the last two np,
    which would justify the Motorola totals.
    On the ST, this n would provoke alignment of the last np, so it's like
    it was nn.

    cases
    trace: My Socks Are Weapons (Legacy)
    trap: Bird Mad Girl Show
*/
#if defined(SSE_MMU_ROUNDING_BUS) // in detail
  if(ir==0x4E76) // trapv
    CPU_ABUS_ACCESS_READ_FETCH;
  else
    INSTRUCTION_TIME(4); //  nn 4
  CPU_ABUS_ACCESS_WRITE_PUSH; // ns 8
  CPU_ABUS_ACCESS_WRITE_PUSH_L; // ns nS 16
  abus=0x100;//391 TODO
  CPU_ABUS_ACCESS_READ_L; // nV nv 24
  CPU_ABUS_ACCESS_READ_FETCH; //np 28
  INSTRUCTION_TIME(2); // n 30 // on ST it will provoke alignment of last fetch
  CPU_ABUS_ACCESS_READ_FETCH; //np 34
#else
  INSTRUCTION_TIME_ROUND(36); 
#endif
}

#if defined(SSE_MMU_ROUNDING_BUS)

inline void m68kInterruptTiming() {
/*  
Doc

Motorola UM:
Interrupt 44(5/3)*
* The interrupt acknowledge cycle is assumed to take four clock periods.

Yacht
  Interrupt           | 44(5/3)  |      n nn ns ni n-  n nS ns nV nv np np      

WinUAE
Interrupt:

- 6 idle cycles
- write PC low word
- read exception number byte from (0xfffff1 | (interrupt number << 1)) [amiga specific]
- 4 idle cycles
- write SR
- write PC high word                       [wrong order...is this credible?]
- read exception address high word
- read exception address low word
- prefetch
- 2 idle cycles
- prefetch
total 44 (don't repeat it, the amiga interrupts better...)

ST (speculative)
-IACK = 16 cycles instead of 4
-2 idle cycles between fetches = 4
could be the cycles are used to increment "PC"

Interrupt IACK (MFP)     |  54(5/3)   | n nn ns ni ni ni ni nS ns nV nv np n np
Interrupt auto (HBI,VBI) | 54-62(5/3) | n nn ns E ni ni ni ni nS ns nV nv np n np
(E=E-clock synchronisation 0-8)

 Cases

 MFP  TIMERB01.TOS; TIMERB03.TOS
 HBI  Forest, TCB, 3615GEN4-HMD, TEST16.TOS
 VBI  Auto 168, Dragonnels/Happy Islands, 3615GEN4-CKM

*/

// E-clock has already been added (temp, in fact this function is temp)
  INSTRUCTION_TIME(6); //  n nn
  CPU_ABUS_ACCESS_WRITE_PUSH; // ns 
  INSTRUCTION_TIME(16); // ni ni ni ni
  CPU_ABUS_ACCESS_WRITE_PUSH_L; // nS ns 
  abus=0x100; //391
  CPU_ABUS_ACCESS_READ_L; // nV nv 
  CPU_ABUS_ACCESS_READ_FETCH; //np 
  INSTRUCTION_TIME(2); // n  // on ST it will provoke alignment of last fetch
  CPU_ABUS_ACCESS_READ_FETCH; //np 
}

#endif

#endif//CPU



#endif//!defined(CPU_DECLA_H) 