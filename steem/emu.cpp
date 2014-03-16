/*---------------------------------------------------------------------------
FILE: emu.cpp
MODULE: emu
DESCRIPTION: The hub for the emu module that contains Steem's core emulation
functions. Basically includes all the files that are in the object.
---------------------------------------------------------------------------*/

#include "pch.h"
#pragma hdrstop

#define IN_EMU

#include "conditions.h" //SS
#include "SSE/SSE.h" //SS
#if !(defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_MFP_H))
inline int abs_quick(int i) // -> mfp.decla.h
{
  if (i>=0) return i;
  return -i;
}
#endif

#include "SSE/SSEDebug.h"

#if defined(STEVEN_SEAGAL)
#include "SSE/SSEDecla.h"
#include "SSE/SSEParameters.h"
#include "SSE/SSEOption.h"
#if !defined(SS_STRUCTURE_SSEFLOPPY_OBJ)
#include "SSE/SSEFloppy.h"
#endif
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

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_TRANSLATE_H)
#define EXT
#define INIT(s) =s

EXT EasyStr TranslateString,TranslateFileName;
EXT char *TranslateBuf INIT(NULL),*TranslateUpperBuf INIT(NULL);
EXT int TranslateBufLen INIT(0);

EasyStr StripAndT(char *s)
{
  EasyStr Ret=Translation(s);
  for(;;){
    int i=Ret.InStr("&");
    if (i<0) break;
    Ret.Delete(i,1);
  }
  return Ret;
}

#undef EXT
#undef INIT
#endif


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


#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_STEEMH_H)

#ifdef IN_EMU
#define EXT
#define INIT(s) =s
#else
#define EXT extern
#define INIT(s)
#endif

extern "C"
{
EXT WORD ir;                  //0
EXT unsigned short sr;        //2
EXT MEM_ADDRESS pc;           //4
EXT signed int r[16];         //8
EXT MEM_ADDRESS old_pc;       //96  //all wrong from this point onwards
EXT MEM_ADDRESS pc_high_byte; //100
EXT signed int other_sp;      //104
//---------------------     CPU emulation
EXT int cpu_cycles;           //108
EXT int ioaccess;             //112
EXT long iobuffer;            //116
EXT MEM_ADDRESS ioad;         //120
//---------------------    memory
EXT unsigned long himem;      //124
EXT MEM_ADDRESS rom_addr;     //128
EXT unsigned long tos_len;    //132
EXT unsigned long mem_len;    //136
EXT bool tos_high;            //140
#if !defined(SS_MMU_NO_CONFUSION)
EXT bool mmu_confused;        //144 
#endif
EXT unsigned long hbl_count INIT(0);

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
// it's silly but I need to place those 'C' declarations here
EXT int cpu_timer;  
EXT BYTE stick[8];
#endif


// Don't forget to update this in the resource file too!
#if defined(STEVEN_SEAGAL)
EXT const char *stem_version_text INIT(SSE_VERSION_TXT); // OSD + info box
#else
EXT const char *stem_version_text INIT("3.2");
#endif


EXT BYTE *Mem_End,
     *Mem_End_minus_1,
     *Mem_End_minus_2,
     *Mem_End_minus_4,
     *Rom_End,
     *Rom_End_minus_1,
     *Rom_End_minus_2,
     *Rom_End_minus_4,
     *cart INIT(NULL),
     *Cart_End_minus_1,
     *Cart_End_minus_2,
     *Cart_End_minus_4;
}

EXT BYTE palette_exec_mem[64+PAL_EXTRA_BYTES];

EXT long palette_table[4096]; // SS 4K!

#if defined(PEEK_RANGE_TEST) && defined(DEBUG_BUILD)

void RangeError(DWORD &ad,DWORD hi_ad)
{
//  ad/=0;
  ad=hi_ad-1;
}

BYTE& PEEK(DWORD ad){ RANGE_CHECK_MESSAGE(mem_len+MEM_EXTRA_BYTES,0,0);return *LPBYTE(Mem_End_minus_1-ad); }
WORD& DPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(mem_len+MEM_EXTRA_BYTES,1,0);return *LPWORD(Mem_End_minus_2-ad); }
DWORD& LPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(mem_len+MEM_EXTRA_BYTES,3,0);return *LPDWORD(Mem_End_minus_4-ad); }
BYTE* lpPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(mem_len+MEM_EXTRA_BYTES,0,0);return LPBYTE(Mem_End_minus_1-ad); }
WORD* lpDPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(mem_len+MEM_EXTRA_BYTES,1,0);return LPWORD(Mem_End_minus_2-ad); }
DWORD* lpLPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(mem_len+MEM_EXTRA_BYTES,3,0);return LPDWORD(Mem_End_minus_4-ad); }

BYTE& ROM_PEEK(DWORD ad){ RANGE_CHECK_MESSAGE(tos_len,0,0);return *LPBYTE(Rom_End_minus_1-ad); }
WORD& ROM_DPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(tos_len,1,0);return *LPWORD(Rom_End_minus_2-ad); }
DWORD& ROM_LPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(tos_len,3,0);return *LPDWORD(Rom_End_minus_4-ad); }
BYTE* lpROM_PEEK(DWORD ad){ RANGE_CHECK_MESSAGE(tos_len,0,0);return LPBYTE(Rom_End_minus_1-ad); }
WORD* lpROM_DPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(tos_len,1,2);return LPWORD(Rom_End_minus_2-ad); }
DWORD* lpROM_LPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(tos_len,3,0);return LPDWORD(Rom_End_minus_4-ad); }

BYTE& CART_PEEK(DWORD ad){ RANGE_CHECK_MESSAGE(128*1024,0,0);return *LPBYTE(Cart_End_minus_1-ad); }
WORD& CART_DPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(128*1024,1,0);return *LPWORD(Cart_End_minus_2-ad); }
DWORD& CART_LPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(128*1024,3,0);return *LPDWORD(Cart_End_minus_4-ad); }
BYTE* lpCART_PEEK(DWORD ad){ RANGE_CHECK_MESSAGE(128*1024,0,0);return LPBYTE(Cart_End_minus_1-ad); }
WORD* lpCART_DPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(128*1024,1,2);return LPWORD(Cart_End_minus_2-ad); }
DWORD* lpCART_LPEEK(DWORD ad){ RANGE_CHECK_MESSAGE(128*1024,3,0);return LPDWORD(Cart_End_minus_4-ad); }


#else
#endif

EXT char d2_t_buf[200];

EXT void* m68k_dest;
EXT MEM_ADDRESS abus;
EXT long m68k_old_dest;
EXT MEM_ADDRESS effective_address;

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
#else
EXT int cpu_timer;
#endif
EXT WORD m68k_ap,m68k_iriwo;
EXT short m68k_src_w;
EXT long m68k_src_l;
EXT char m68k_src_b;



#undef EXT
#undef INIT

#endif

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
///void set_pc(MEM_ADDRESS ad); // forward // still necessary?
#include "SSE/SSE6301.h"
#include "SSE/SSECpu.h"
//#include "SSE/SSEInterrupt.h"
#include "SSE/SSESTF.h"

#if defined(SS_SHIFTER_EVENTS)
#include "SSE/SSEShifterEvents.h"
#endif

#if defined(SS_DEBUG_FRAME_REPORT) //temp, same place
#include "SSE/SSEFrameReport.h"
#endif

#include "SSE/SSEShifter.h"
#include "SSE/SSEInterrupt.h"
#include "SSE/SSEVideo.h"
#include "SSE/SSEInline.h"
#include "SSE/SSESDL.h"
///#include "SSE/SSEDebug.h"
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

#if !defined(SS_STRUCTURE_SSE6301_OBJ)
#include "SSE/SSE6301.cpp" 
#endif
#include "SSE/SSESDL.cpp" //?
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




#if !(defined(STEVEN_SEAGAL)&&defined(SS_STRUCTURE_CPU_POKE_NOINLINE))
// this seems useless?
void m68k_poke_noinline(MEM_ADDRESS ad,BYTE x){ m68k_poke(ad,x); }
void m68k_dpoke_noinline(MEM_ADDRESS ad,WORD x){ m68k_dpoke(ad,x); }
void m68k_lpoke_noinline(MEM_ADDRESS ad,LONG x){ m68k_lpoke(ad,x); }
#endif

