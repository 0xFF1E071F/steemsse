#pragma once
#ifndef SSECPU_H
#define SSECPU_H

#if defined(SSE_CPU)

//#include <cpu.decla.h>
#include <d2.decla.h>
#ifdef DEBUG_BUILD
#include <debug_emu.decla.h>
#endif
#include <run.decla.h>
#include <blitter.decla.h>
#include <mfp.decla.h>
#include "SSEDebug.h"
#include "SSEVideo.h"

#ifdef SSE_UNIX//?
extern void exception(int,exception_action,MEM_ADDRESS);
#endif

#if defined(DEBUG_BUILD)
void DebugCheckIOAccess();
#define DEBUG_CHECK_IOACCESS DebugCheckIOAccess();
#else
#define DEBUG_CHECK_IOACCESS
#endif

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TM68000 {

  // ENUM
#if defined(SSE_CPU_HALT)
/* v 3.7, see MC68000UM p99 (6-1)
"The processor is always in one of three processing states: normal, exception, or halted.
The normal processing state is associated with instruction execution; the memory
references are to fetch instructions and operands and to store results. A special case of
the normal state is the stopped state, resulting from execution of a STOP instruction. In
this state, no further memory references are made."

Different states are used, but some have NOTHING to do with processor state,
so refactoring due!
*/
  enum {NORMAL,EXCEPTION,HALTED,
    STOPPED,INTEL_CRASH,BLIT_ERROR,BOILER_MESSAGE}; 
#endif
#ifdef SSE_CPU_E_CLOCK
  enum {ECLOCK_VBL,ECLOCK_HBL,ECLOCK_ACIA};
#endif

  // DATA
#if defined(SSE_CPU_E_CLOCK)
  int cycles_for_eclock; // because of integer overflow problems
  int cycles0; // to record ACT
#endif
#if defined(SSE_CPU_TRUE_PC)
  MEM_ADDRESS Pc;
#endif
#if defined(SSE_DEBUG)
  int IrAddress; // pc at start
  int nExceptions;
  int nInstr;
#endif
#if defined(SSE_DEBUG)
  WORD PreviousIr;
#endif
  BYTE PrefetchClass; // see ijor's article
#if defined(SSE_CPU_TRUE_PC)
  bool CheckRead; // most 'write' instructions read before, crash at read...
#endif
#if defined(SSE_CPU_E_CLOCK)
  bool EClock_synced;
  BYTE LastEClockCycles[3];
#endif
  BYTE ProcessingState;
  bool tpend; // actual internal latch set when CPU should trace current instruction

  // FUNCTIONS
  TM68000();
  void Reset(bool Cold);
#if defined(SSE_CPU_E_CLOCK)
  int SyncEClock(int dispatcher);
  void UpdateCyclesForEClock();
#endif
};


#pragma pack(pop)


#define PREFETCH_CLASS(n) M68000.PrefetchClass=(n)
#define PREFETCH_CLASS_0 (M68000.PrefetchClass==0)
#define PREFETCH_CLASS_1 (M68000.PrefetchClass==1)
#define PREFETCH_CLASS_2 (M68000.PrefetchClass==2)

extern TM68000 M68000;

#endif//#if defined(SSE_CPU)

#endif //SSECPU_H
