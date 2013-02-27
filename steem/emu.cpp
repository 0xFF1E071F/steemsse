/*---------------------------------------------------------------------------
FILE: emu.cpp
MODULE: emu
DESCRIPTION: The hub for the emu module that contains Steem's core emulation
functions. Basically includes all the files that are in the object.
---------------------------------------------------------------------------*/

#include "pch.h"
#pragma hdrstop

#define IN_EMU

inline int abs_quick(int i)
{
  if (i>=0) return i;
  return -i;
}

#include "conditions.h" //SS
#include "SSE/SSE.h" //SS

#include "SSE/SSEDebug.h"
#if defined(STEVEN_SEAGAL)
#include "SSE/SSEDecla.h"
#include "SSE/SSEParameters.h"
#include "SSE/SSEOption.h"
#include "SSE/SSEFloppy.h"
#endif

#if USE_PASTI
#include <pasti/pasti.h>
#endif

#include "easystr.h"
typedef EasyStr Str; // SS who?
#include "mymisc.h"
#include "dirsearch.h"
#include "dynamicarray.h"
#include "portio.h"

#include "translate.h"

#ifdef ONEGAME
#include "onegame.h"
#endif

#include "acc.h"
#include "gui.h"
#include "shortcutbox.h"
#include "stjoy.h"
#include "init_sound.h"
#include "loadsave.h"
#include "macros.h"

#ifdef WIN32
#ifndef NO_ASM_PORTIO
#include <internal_speaker.h>
#else
void internal_speaker_sound(int){}
void internal_speaker_sound_by_period(int){}
#endif

#else

#endif

#include "reset.h"
#include "display.h"
#include "steemh.h"
#include "cpu.h"
#include "run.h"
#ifdef DEBUG_BUILD
#include "debug_emu.h"
#include "iolist.h"
#endif
#include "draw.h"
#include "osd.h"
#include "fdc.h"
#include "floppy_drive.h"
#include "hdimg.h"
#include "psg.h"
#include "mfp.h"
#include "iorw.h"
#include "stports.h"
#include "midi.h"
#include "rs232.h"
#include "blitter.h"
#include "palette.h"
#include "ikbd.h"
#include "stemdos.h"
#include "emulator.h"

#if defined(STEVEN_SEAGAL)
void set_pc(MEM_ADDRESS ad); // forward // still necessary?
#include "SSE/SSE6301.h"
#include "SSE/SSECpu.h"
#include "SSE/SSEInterrupt.h"
#include "SSE/SSESTF.h"
#include "SSE/SSEShifterEvents.h"
#include "SSE/SSEShifter.h"
#include "SSE/SSEVideo.h"
#include "SSE/SSEInline.h"
#include "SSE/SSEDebug.h"
#endif

#include "cpu.cpp"

#ifdef DEBUG_BUILD
#include "debug_emu.cpp"
#endif

#include "draw.cpp"
#include "run.cpp"
#include "ior.cpp"
#include "iow.cpp"
#include "mfp.cpp"
#include "psg.cpp"
#include "blitter.cpp"
#include "fdc.cpp"
#include "stports.cpp"
#include "midi.cpp"
#include "rs232.cpp"
#include "emulator.cpp"

#if defined(STEVEN_SEAGAL)
#include "SSE/SSEInterrupt.cpp"
#include "SSE/SSESTF.cpp"
#include "SSE/SSE6301.cpp" // or in ikbd?


#endif

#include "reset.cpp"
#include "ikbd.cpp"
#include "stemdos.cpp"
#include "loadsave_emu.cpp"


void set_pc(MEM_ADDRESS ad)
{
  SET_PC(ad); 
}

#if !(defined(STEVEN_SEAGAL) && defined(SS_CPU))

void perform_rte()
{
  M68K_PERFORM_RTE(;);
}

#endif

#define LOGSECTION LOGSECTION_CPU
void m68k_process()
{
  m68k_PROCESS
}
#undef LOGSECTION






void m68k_poke_noinline(MEM_ADDRESS ad,BYTE x){ m68k_poke(ad,x); }
void m68k_dpoke_noinline(MEM_ADDRESS ad,WORD x){ m68k_dpoke(ad,x); }
void m68k_lpoke_noinline(MEM_ADDRESS ad,LONG x){ m68k_lpoke(ad,x); }


