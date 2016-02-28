////////////////
// Parameters //
////////////////

/*  Some important emulation parameters are concentrated here, which makes 
    experimenting (always fun) easier.
    A lot are still missing.
*/

#pragma once
#ifndef SSEPARAMETERS_H
#define SSEPARAMETERS_H

#if defined(SSE_STRUCTURE_SSE6301_OBJ) && defined(__cplusplus)
#include <conditions.h>
#include <draw.decla.h>
#endif


/////////////
// GENERAL //
/////////////


#if defined(STEVEN_SEAGAL)

//#define ACT ABSOLUTE_CPU_TIME

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_INT_MFP_RATIO)
#define HBL_PER_SECOND (CpuNormalHz/GLU.CurrentScanline.Cycles) //TODO assert
//#endif
//Frequency   50          60            72
//#HBL/sec    15666.5 15789.85827 35809.14286

#else 
#if defined(SSE_FDC_PRECISE_HBL)//todo table
#define HBL_PER_FRAME ( (shifter_freq_at_start_of_vbl==50)?HBLS_50HZ: \
  ( (shifter_freq_at_start_of_vbl==60)? HBLS_60HZ : HBLS_72HZ))
#else
#define HBL_PER_FRAME ( shifter_freq_at_start_of_vbl==50?HBLS_50HZ: \
  (shifter_freq_at_start_of_vbl==60?HBLS_60HZ:(HBLS_72HZ/2)))
#endif

#define HBL_PER_SECOND HBLS_PER_SECOND_AVE//(HBL_PER_FRAME*shifter_freq_at_start_of_vbl)  //still not super accurate
#endif


#endif


//////////
// ACIA //
//////////

#if defined(SSE_ACIA)

#if defined(SSE_ACIA_IRQ_DELAY)// not defined anymore (v3.5.2), see MFP

#define SSE_ACIA_IRQ_DELAY_CYCLES 20 // est. gap between RX and IRQ in CPU cycles
/*  Delay before keys sent by the IKBD are received by the ACIA.
    It was about 20 in Steem 3.2 (>10.000 cycles), I brought it back to
    12 in 3.3 to be closer to 7.200 cycles mentioned in WinsTON/Hatari, but
    experiments with 6301 'true emu' could point to a value > 10.000, 
    vindicating Steem authors! Though it is all guessing.
*/

#if !defined(SSE_IKBD)
#define HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL \
(1350*8/ \
(shifter_freq_at_start_of_vbl==50?SCANLINE_TIME_IN_CPU_CYCLES_50HZ:\
(screen_res==2?SCANLINE_TIME_IN_CPU_CYCLES_70HZ:\
SCANLINE_TIME_IN_CPU_CYCLES_60HZ)))
#endif

#endif

#if defined(SSE_IKBD_6301)

#define HD6301_TO_ACIA_IN_CYCLES (HD6301_CYCLES_TO_SEND_BYTE*HD6301_CYCLE_DIVISOR)
#define ACIA_TO_HD6301_IN_CYCLES (HD6301_CYCLES_TO_RECEIVE_BYTE*HD6301_CYCLE_DIVISOR)
#define HD6301_TO_ACIA_IN_HBL (HD6301EMU_ON?HD6301_CYCLES_TO_SEND_BYTE_IN_HBL:(screen_res==2?24:12))

#else //that was not correct, for older versions (?)

#define HD6301_TO_ACIA_IN_CYCLES (7200) // from WinSTon
#define HD6301_TO_ACIA_IN_HBL (screen_res==2?24:12) // to be <7200

#endif

#if defined(SSE_ACIA_TDR_COPY_DELAY)
//#define ACIA_TDR_COPY_DELAY ACIA_CYCLES_NEEDED_TO_START_TX //formerly
#define ACIA_TDR_COPY_DELAY (200) // Hades Nebula vs. Nightdawn (???)
#endif

#if defined(SSE_ACIA_IRQ_DELAY2)
#define ACIA_RDRF_DELAY (20)
#endif

#if defined(SSE_ACIA_MIDI_SR02_CYCLES)
/*  

SS1 SS0                  Speed (bit/s)
 0   0    Normal            500000
 0   1    Div by 16          31250 (ST: MIDI)
 1   0    Div by 64         7812.5 (ST: IKBD)
MIDI is 4 times faster than IKBD
*/
#define ACIA_MIDI_OUT_CYCLES (HD6301_TO_ACIA_IN_CYCLES/4) //temp
#endif

#if SSE_VERSION<=350
#define IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESSE_ALT (HD6301EMU_ON?45:2)
#endif

#else //!ACIA
#define HD6301_TO_ACIA_IN_HBL (screen_res==2?24:12) // to be <7200
#endif//ACIA


/////////////
// BLITTER //
/////////////

#if defined(SSE_BLITTER)
#if defined(SSE_BLT_BLIT_MODE_CYCLES) 
/*  It's not so clear in the doc, but the 64 shared bus cycles are not in CPU
    cycles but in NOP units (4 cycles). 
    It could be deduced from the example program and the mention restarting 
    the blitter takes "7 cycles". The timing table is also in NOP.
*/
//#undef SSE_BLT_BLIT_MODE_CYCLES
#ifdef SSE_BLT_BLIT_MODE_CYCLES4
//#define BLITTER_BLIT_MODE_CYCLES (SSE_HACKS_ON?(128):(64*4)) // or cycles = 2 clock? //TODO test prg
#define BLITTER_BLIT_MODE_CYCLES (SSE_HACKS_ON?(160):(64*4))//160 hack for AC2011, Down TLN (not good), SmokeTown
#else
#define BLITTER_BLIT_MODE_CYCLES ((65-1)*4) // 'NOP' x 4 = cycles //63?
#endif
#if !defined(SSE_BLT_BLIT_MODE_CYCLES2)
#define SSE_BLT_BLIT_MODE_IRQ_CHK (64) // when we check for IRQ, in cycles
#endif
#endif
#endif


/////////
// CPU //
/////////

#if defined(SSE_CPU)

//todo move clock here?

#if defined(SSE_CPU_TRACE_TIMING)
#define CPU_TRACE_TIMING 36 // doc 34
#endif

#if defined(SSE_CPU_STOP_DELAY)
#define CPU_STOP_DELAY 8
#endif

#endif


///////////
// DEBUG //
///////////

#if defined(SSE_DEBUG) 
#if defined(SSE_UNIX)
#define SSE_TRACE_FILE_NAME "./TRACE.txt"
#else
#define SSE_TRACE_FILE_NAME "TRACE.txt"
#endif
#define TRACE_MAX_WRITES 200000 // to avoid too big file
#endif

#if defined(SSE_OSD_DEBUG_MESSAGE)
#define OSD_DEBUG_MESSAGE_LENGTH 30 // in bytes
#define OSD_DEBUG_MESSAGE_TIME 2 // in seconds
#endif

#if defined(SSE_BOILER_FAKE_IO)
#define FAKE_IO_START 0xfffb00
#define FAKE_IO_LENGTH 64*2 // in bytes
#define FAKE_IO_END (FAKE_IO_START+FAKE_IO_LENGTH-2) // starting address of last one
#define STR_FAKE_IO_CONTROL "Control mask browser"
#endif

#if defined(SSE_DEBUG_FRAME_REPORT)
#if defined(SSE_UNIX)
#define FRAME_REPORT_FILENAME "./FrameReport.txt" //a fix?
#else
#define FRAME_REPORT_FILENAME "FrameReport.txt"
#endif
#endif



#if defined(SSE_DISK_GHOST)
#define AVG_HBLS_SECTOR (200)
#endif

//SF314[DRIVE].HblsPerRotation()

//////////
// DISK //
//////////

#if defined(SSE_DISK)

#ifdef SSE_DISK_ST
#define DISK_EXT_ST "ST"
#endif
#ifdef SSE_DISK_MSA
#define DISK_EXT_MSA "MSA"
#endif
#if defined(SSE_DISK_STW)
#define DISK_EXT_STW "STW"
#endif
#if defined(SSE_DISK_SCP)
#define DISK_EXT_SCP "SCP"
#endif
#if defined(SSE_DISK_HFE)
#define DISK_EXT_HFE "HFE"
#if defined(SSE_UNIX)
#define DISK_HFE_BOOT_FILENAME "/HFE_boot.bin" 
#else
#define DISK_HFE_BOOT_FILENAME "\\HFE_boot.bin"
#endif
#endif
#endif

/////////
// DMA //
/////////

#if defined(SSE_DMA)
#if defined(SSE_DMA_DELAY)
#define SSE_DMA_ACCESS_DELAY 20
#endif
#endif

///////////
// DRIVE //
///////////

#if defined(SSE_DRIVE)

#define DRIVE_11SEC_INTERLEAVE 6
#define DRIVE_RPM 300
#define DRIVE_MAX_CYL 83

#if defined(SSE_DRIVE_BYTES_PER_ROTATION)
/*  #bytes/track
    The value generally seen is 6250.
    The value for 11 sectors is 6256. It's possible if the clock is higher than
    8mhz, which is the case on the ST.
    6256+14 is too much, but this is the value that has some delicate cases
    running, so we just keep it for now, couldn't improve.
    In Steem native emulation precision is HBL, so there are trade-offs.
    For STW images the value is 6256 for same cases.
*/

#define DRIVE_BYTES_ROTATION (6256+14)  //little hack...
#define DRIVE_BYTES_ROTATION_STW (6256)
#else
#define DRIVE_BYTES_ROTATION (8000) // 3.2 (!)
#endif


#if defined(SSE_DRIVE_RW_SECTOR_TIMING2)//no?
#define FDC_SECTOR_GAP_BEFORE_IRQ_9_10 (3+(SSE_HACKS_ON?28:0)) //CRC + hack (FDCTNF by Petari)
#define FDC_SECTOR_GAP_BEFORE_IRQ_11 (3)
#endif

#if defined(SSE_DRIVE_SOUND)
#ifdef SSE_UNIX //TODO...
#define DRIVE_SOUND_DIRECTORY "/DriveSound" 
#else
#define DRIVE_SOUND_DIRECTORY "\\DriveSound"
#endif
#define DRIVE_SOUND_BUZZ_THRESHOLD 7 // distance between tracks
#endif

#endif


/////////
// FDC //
/////////

#if defined(SSE_FDC)

#if defined(SSE_FDC_ACCURATE)
#endif//#if defined(SSE_FDC_ACCURATE)

#endif//#if defined(SSE_FDC)


/////////
// GLU //
/////////


#if defined(SSE_GLUE_THRESHOLDS)

enum {
  GLU_DE_ON_72=6, //+ WU_res_modifier; STE-4
  GLU_DE_ON_60=52, //+ WU_sync_modifier; STE -16
  GLU_DE_ON_50=56, //+ WU_sync_modifier; STE -16
  GLU_HBLANK_OFF_50=28, //+ WU_sync_modifier
  GLU_HSYNC_ON_50=464, //+ WU_res_modifier, STE-2
  GLU_HSYNC_DURATION=40,
  GLU_RELOAD_VIDEO_COUNTER_50=64-2, //+ WU_sync_modifier (STE -2?)
  GLU_TRIGGER_VBI_50=64,	//STE+4
  GLU_DECIDE_NCYCLES=54, //+ WU_sync_modifier, STE +2
  GLU_VERTICAL_OVERSCAN_50=GLU_HSYNC_ON_50+GLU_HSYNC_DURATION // 504 //+ WU_sync_modifier, STE -2
};

#endif


////////
//GUI //
////////

#if defined(SSE_GUI)

#define WINDOW_TITLE_MAX_CHARS 20+5 //argh, 20 wasn't enough

#define README_FONT_NAME "Courier New"
#define README_FONT_HEIGHT 16

#if SSE_VERSION>=370
#define EXT_TXT ".txt" //save bytes?
#define STEEM_SSE_FAQ "Steem SSE FAQ"
#define STEEM_HINTS "Hints"
#else
#define STEEM_SSE_FAQ_TXT "Steem SSE FAQ.txt"
#define STEEM_HINTS_TXT "Hints.txt"
#endif
#if defined(SSE_GUI_INFOBOX9) || defined(SSE_GUI_INFOBOX12)
#define STEEM_MANUAL "Steem manual"
#define STEEM_MANUAL_SSE "Steem SSE manual"
#endif

#if defined(SSE_GUI_CONFIG_FILE)
#define CONFIG_FILE_EXT "ini" // ini, cfg?
#endif

#endif//gui


///////////
// Hacks //
///////////

#if defined(SSE_HACKS)
enum {
SS_SIGNAL_TOS_PATCH106=1, // checking all ST files we open
SS_SIGNAL_SHIFTER_CONFUSED_1,// temp hacks for 3.4
SS_SIGNAL_SHIFTER_CONFUSED_2,// not looking for elegance!
SS_SIGNAL_DRIVE_SPINNING_AT_COMMAND,
SS_SIGNAL_ENUM_EnumDisplayModes, // wait until finished (?)
};
#endif

/////////
// HDC //
/////////

#if defined(SSE_ACSI_NOGUISELECT) // only one file in Steem root can be HD
#if defined(SSE_ACSI_MULTIPLE)
#if defined(SSE_UNIX)
#define ACSI_HD_DIR "/ACSI/"
#else
#define ACSI_HD_DIR "\\ACSI\\"
#endif
#else
#if defined(SSE_UNIX)
#define ACSI_HD_NAME "./ACSI_HD0.img"
#else
#define ACSI_HD_NAME "\\ACSI_HD0.img"
#endif
#endif
#endif

//////////
// IKBD //
//////////

#if defined(SSE_IKBD)

#if defined(SSE_IKBD_6301)

#if defined(SSE_UNIX)
#define HD6301_ROM_FILENAME "/HD6301V1ST.img" 
#else
#define HD6301_ROM_FILENAME "\\HD6301V1ST.img"
#endif
#define HD6301_ROM_CHECKSUM 451175 // BTW this rom sends $F1 after reset (80,1)
#endif

// Parameters used in true and fake 6301 emu

#ifndef SSE_SHIFTER
#define HD6301_CYCLES_PER_SCANLINE 64 // used if SSE_SHIFTER not defined
#endif

#define HD6301_CYCLE_DIVISOR 8 // the 6301 runs at 1MHz (verified by Stefan jL)
#define HD6301_CLOCK (1000000) //used in 6301/ireg.c


// in HBL, for Steem, -1 for precise timing (RX/IRQ delay)
#if defined(SSE_IKBD_6301_373)
#define HD6301_CYCLES_TO_SEND_BYTE_IN_HBL \
  (((HD6301_CYCLES_TO_SEND_BYTE*HD6301_CYCLE_DIVISOR) \
  /scanline_time_in_cpu_cycles_at_start_of_vbl)-1)
#else// those were useless calculations while the result was available as a variable
#define HD6301_CYCLES_TO_SEND_BYTE_IN_HBL \
((HD6301_CYCLES_TO_SEND_BYTE*HD6301_CYCLE_DIVISOR/\
(shifter_freq_at_start_of_vbl==50?SCANLINE_TIME_IN_CPU_CYCLES_50HZ: \
(screen_res==2?SCANLINE_TIME_IN_CPU_CYCLES_70HZ:\
SCANLINE_TIME_IN_CPU_CYCLES_60HZ)))-1)
#endif

#define HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL \
(HD6301_CYCLES_TO_RECEIVE_BYTE*HD6301_CYCLE_DIVISOR/ \
(shifter_freq_at_start_of_vbl==50?SCANLINE_TIME_IN_CPU_CYCLES_50HZ:\
(screen_res==2?SCANLINE_TIME_IN_CPU_CYCLES_70HZ:\
SCANLINE_TIME_IN_CPU_CYCLES_60HZ)))

#ifdef SSE_DEBUG
#define HD6301_MAX_DIS_INSTR 2000 
#endif

#if defined(SSE_IKBD_6301_EVENT)//380
//#define HD6301_CYCLES_TO_SEND_BYTE ((SSE_HACKS_ON&& LPEEK(0x18)==0xFEE74)?1350:1280) // boo!
#define HD6301_CYCLES_TO_SEND_BYTE ((SSE_HACKS_ON&& LPEEK(0x18)==0xFEE74)?1345:1280) // boo!
#define HD6301_CYCLES_TO_RECEIVE_BYTE (HD6301_CYCLES_TO_SEND_BYTE)
#elif defined(SSE_ACIA_OVR_TIMING)
// hack: we count more cycle when overrun is detected, for Froggies
#define HD6301_CYCLES_TO_SEND_BYTE ((SSE_HACKS_ON&&(ACIA_IKBD.overrun==ACIA_OVERRUN_COMING))?1380+30:1300)
#define HD6301_CYCLES_TO_RECEIVE_BYTE HD6301_CYCLES_TO_SEND_BYTE
#else
#define HD6301_CYCLES_TO_SEND_BYTE (1350)
#define HD6301_CYCLES_TO_RECEIVE_BYTE (1350)
#endif

// far from ideal, but maybe we must change method or timings instead //v3.7 done!
#if !defined(SSE_IKBD_6301_MOUSE_ADJUST_SPEED2)
#define HD6301_MOUSE_SPEED_CHUNKS 15
#define HD6301_MOUSE_SPEED_CYCLES_PER_CHUNK 1000
#endif
#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SSE_INTERRUPT)
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

#if defined(SSE_INT_ROUNDING)
#define SSE_INT_MFP_TIMING (54)
#define SSE_INT_HBL_TIMING (54)
#define SSE_INT_VBL_TIMING (54)
#else
#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_TIMING (56) 
#endif
#if defined(SSE_INT_HBL)
#define SSE_INT_HBL_TIMING (56)
#endif
#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_TIMING (56)
#endif
#endif

#define HBL_IACK_LATENCY 28
#define VBL_IACK_LATENCY 28

#if defined(SSE_CPU_E_CLOCK4)
#define ECLOCK_AUTOVECTOR_CYCLE 10 // IACK starts at cycle 10 (?)
#else
#define ECLOCK_AUTOVECTOR_CYCLE 28 //whatever!
#endif

#if defined(SSE_INT_VBI_START)
#undef SSE_INT_VBI_START
#define SSE_INT_VBI_START (68) // default = STE
#endif

#if !defined(SSE_GLUE_THRESHOLDS)
#define THRESHOLD_LINE_PLUS_2_STF (54)
#if defined(SSE_INT_VBL_STF) // modest hack still works
#define HBL_FOR_STE (444)
#define THRESHOLD_LINE_PLUS_2_STE (40) //3.7.0
#if SSE_VERSION<364
//this particular hack doesn't look useful for anything now
#define HBL_FOR_STF (HBL_FOR_STE+4+(SSE_HACKS_ON?4:0))
#else
#define HBL_FOR_STF (HBL_FOR_STE+4)
#endif
#else
#define THRESHOLD_LINE_PLUS_2_STE (THRESHOLD_LINE_PLUS_2_STF-2)
#endif
#endif
#endif//int

/////////
// IPF //
/////////

#if defined(SSE_IPF)
#define SSE_IPF_PLUGIN_FILE "CAPSImg.dll" //Windows
#define SSE_IPF_FREQU 8000000//? CPU speed? - even for that I wasn't helped!

#ifdef SSE_IPF_CTRAW
#undef SSE_IPF_CTRAW
#define SSE_IPF_CTRAW "CTR" 
#endif
#ifdef SSE_IPF_KFSTREAM//no
#undef SSE_IPF_KFSTREAM
#define SSE_IPF_KFSTREAM "RAW" 
#endif
#ifdef SSE_IPF_DRAFT//no
#undef SSE_IPF_DRAFT
#define SSE_IPF_DRAFT "RAW"  //?
#endif

#endif//ipf


/////////
// MFP //
/////////

#if defined(SSE_INT_MFP)

#if defined(SSE_INT_MFP_RATIO) 
/*  There was a hack in Steem up to v3.3, where the CPU/MFP frequency ratio
    was set different from what was measured on an STE by Steem authors, that
    allowed Lethal Xcess to run. This hack was hiding a blitter timing problem,
    however (the 64 NOP issue), which explains why the game wasn't sensitive
    to the ratio in STF mode. Now that the blitter bug has been corrected,
    we use the measured value (when SSE_INT_MFP_RATIO_STE is defined).
    For STF we use the precise value of the MFP quarts and the real value of
    a typical "PAL" STF, as read on atari-forum (ijor?)
    3.5.1: same ratio for STE, it could make DMA sound emu more precise
    3.7.0:  DMA sound uses another clock, we now have a slightly different
    CPU clock and MFP ratio in STE mode. It also "helps" some cases, along
    with other parameters, it's not exact science.

    MFP (no variation) ~ 2457600 hz
    CPU STF ~ 8021247
    CPU STE ~ 8021030, being pragmatic (programs must run - or not - like on
    my STE)
  
*/
/*
The master clock crystal and derived CPU clock table is:


Code: Select all
PAL (all variants)       32.084988   8.021247
NTSC (pre-STE)           32.0424     8.0106
NTSC (STE)               32.215905   8.053976
Peritel (STE) (as PAL)   32.084988   8.021247
Some STFs                32.02480    8.0071
*/

#define  MFP_CLK_LE 2451 // Steem 3.2
#define  MFP_CLK_LE_EXACT 2451134 // Steem 3.2
 
// Between 2451168 and 2451226 cycles, as measured by Steem authors:

#define  CPU_STE_TH 8000000 // Steem 3.2
#define  MFP_CLK_STE_EXACT 2451182 // not if 'STE as STF' is defined

#if defined(SSE_INT_MFP_RATIO_STF2)
#define  CPU_STF_PAL 8021247//(8020736+512+512)// should be 8021247
#else
#define  CPU_STF_PAL (8021248) // ( 2^8 * 31333 )
#endif

#define  CPU_STF_ALT (8007100) //ljbk's? for Panic study!
#if defined(SSE_INT_MFP_RATIO_STE3)
#define  CPU_STE_PAL (CPU_STF_PAL)
#elif defined(SSE_INT_MFP_RATIO_STE2)
#define  CPU_STE_PAL 8021030//8020992//(8021030)//8020736
#else
#define  CPU_STE_PAL (CPU_STF_PAL+64) //64 for DMA sound!
#endif
#define  MFP_CLK_TH 2457
#define  MFP_CLK_TH_EXACT 2457600 // ( 2^15 * 3 * 5^2 )
#endif



#if defined(SSE_INT_MFP_TIMERS_WOBBLE)//yes in v3.8??
#define MFP_WRITE_LATENCY (8)
#define MFP_TIMERS_WOBBLE 4//2//1 //2 // if there's wobble, at least 2 seems likelier
#else
#define MFP_WRITE_LATENCY (5) //5 (=8?) best for TEST10.TOS
#endif

#define MFP_TIMER_DATA_REGISTER_ADVANCE (4)

#if defined(SSE_INT_MFP_TIMERS_STARTING_DELAY)
#if defined(SSE_INT_MFP_TIMERS_WOBBLE) 
#if defined(SSE_INT_MFP_EVENT_WRITE)
#define MFP_TIMER_SET_DELAY (10-2) // 8 OK with wobble 4 for LXS
#elif defined(SSE_GLUE_FRAME_TIMINGS) && !defined(SSE_INT_MFP_RATIO_STE3)
#define MFP_TIMER_SET_DELAY 8//(8) //schnusdie
#elif defined(SSE_GLUE_FRAME_TIMINGS) 
#define MFP_TIMER_SET_DELAY (7)
#elif defined(SSE_INT_MFP_RATIO_STE2)
#define MFP_TIMER_SET_DELAY 8//10 // overscan/schnusdie... [8]
#else
#define MFP_TIMER_SET_DELAY 7 //  Schnusdie vs DSOTS (depends on clock?!)
#endif
#else
// if = 12, better not define, it reduces code
#if defined(SSE_INT_MFP_RATIO_STE3)
#define MFP_TIMER_SET_DELAY 8//9 // DSOS main but then TEST10 gets bad...
#else
// 8-10 break LXS! 
#define MFP_TIMER_SET_DELAY (8)//11 //11: OVSC6 STE (12 breaks) // 8 loSTE STE
#endif
#if (MFP_TIMER_SET_DELAY==12) // (12=Steem 3.2)
#undef SSE_INT_MFP_TIMERS_STARTING_DELAY
#endif
#endif
#endif//starting_delay
// it may seem high but it's not #IACK cycles
// cases Final Conflict, Anomaly menu
#define MFP_IACK_LATENCY (28) 
#define MFP_SPURIOUS_LATENCY MFP_WRITE_LATENCY//?? (MFP_IACK_LATENCY) //?

#endif//mfp


/////////
// MMU //
/////////

#if defined(SSE_MMU)

#if defined(SSE_MMU_WU_DL)
#define WU_SHIFTER_PANIC 5
#else
#define WU_SHIFTER_PANIC 3
#endif

#endif


/////////
// OSD //
/////////

#if defined(SSE_OSD)
#define RED_LED_DELAY 1500 // Red floppy led for writing, in ms
#define HD_TIMER 100 // Yellow hard disk led (imperfect timing)
#endif


/////////////
// SHIFTER //
/////////////


#ifndef SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1
#define VERT_OVSCN_LIMIT (502) //502
#if defined(SSE_MMU_WU_IO_BYTES_W_SHIFTER_ONLY)
#define WU2_PLUS_CYCLES 4 // we make cycles +2
#else
#define WU2_PLUS_CYCLES 2 // we don't
#endif
#endif


///////////
// SOUND //
///////////

#if defined(SSE_SOUND_FILTER_STF)
#define SSE_SOUND_FILTER_STF_V ((*source_p+dv)/2)
#define SSE_SOUND_FILTER_STF_DV (v)//((*source_p+dv)/2)
#endif

#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY)
// to make XMAS 2004 scroller work, should it be lower?
#define MICROWIRE_LATENCY_CYCLES (128+16)
#endif

/////////
// STF //
/////////


#if defined(SSE_STF_VERT_OVSCN) && !defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#define STF_VERT_OVSCN_OFFSET 4
#endif



/////////////
// TIMINGS //
/////////////

// DMA sound has its own clock, it's not CPU's
// We adjust this so that we have 50065 in ljbk's test
//TODO should be 8010613
#if defined(SSE_INT_MFP_RATIO_STE2)

#if CPU_STE_PAL==(8020736) // too slow for Overscan Demos STE...
#define STE_DMA_CLOCK 8021000 // 50065; MOLZ OK
#elif CPU_STE_PAL==(8020736+512+512)
#define STE_DMA_CLOCK 8021350
#elif CPU_STE_PAL==(8021030)
#define STE_DMA_CLOCK 8021500 //50065; MOLZ OK
//#define STE_DMA_CLOCK 8012800
#else
#define STE_DMA_CLOCK 8012800//8021250//8021350 // 50065; MOLZ OK v3.8.0 : 8021250
//#define STE_DMA_CLOCK 8021118 //(8021502-256-128) // before v3.8.0
#endif
#else
#define STE_DMA_CLOCK 8021502 //OK with STE clock=STF?
#endif

//#define STE_DMA_CLOCK 8010613//test
//#define CPU_STE_PAL CPU_STF_PAL//test

/////////
// TOS //
/////////

#if defined(SSE_TOS_PRG_AUTORUN)
#define AUTORUN_HD (2+'Z'-'C')//2=C, Z: is used for PRG support
#endif

#if defined(SSE_STF_MATCH_TOS2)
#if defined(SSE_TOS_GEMDOS_RESTRICT_TOS2)
#define DEFAULT_TOS_STF (HardDiskMan.DisableHardDrives?0x102:0x104) // how caring!
#else
#define DEFAULT_TOS_STF 0x102
#endif
#define DEFAULT_TOS_STE 0x162
#endif


///////////
// VIDEO //
///////////

#if defined(SSE_VIDEO)

#if defined(SSE_VID_BORDERS)
#define ORIGINAL_BORDER_SIDE 32
#if defined(SSE_VID_BORDERS_LB_DX)
#define LARGE_BORDER_SIDE 48 // trick, making it 40 at rendering
#else
#define LARGE_BORDER_SIDE 40 // making many hacks necessary 
#endif
#define LARGE_BORDER_SIDE_WIN 40 // max for 800x600 display (fullscreen)

#define VERY_LARGE_BORDER_SIDE 48 // 416 pixels wide for emulation
#define VERY_LARGE_BORDER_SIDE_WIN 46 // trick, 412 pixels wide for rendering
#define ORIGINAL_BORDER_BOTTOM 40 

#if defined(SSE_VID_BORDERS_LIMIT_TO_245)
/*  v3.8.0
    As seen on Overscan demos on real STE + CRT: there's no trash in the lower
    border because VBLANK is already on.
    This is also in agreement with Atari Monitor Summary Specifications.
    Menu Zuul 86 is buggy: no monitor will display the bottom of the letters.
*/
#define LARGE_BORDER_BOTTOM 45
#define VERY_LARGE_BORDER_BOTTOM 45
#else
#define LARGE_BORDER_BOTTOM 48
#define VERY_LARGE_BORDER_BOTTOM 50  // counts for raster fx
#endif

#define ORIGINAL_BORDER_TOP 30
#define BIG_BORDER_TOP 36 // for The Musical Wonder 1990

#if defined(SSE_VID_BORDERS_416) && defined(SSE_VID_BORDERS_412)
#define BIGGEST_DISPLAY 3
#else
#define BIGGEST_DISPLAY 2
#endif

#endif

#if defined(SSE_VID_D3D_STRETCH_ASPECT_RATIO) || defined(SSE_VID_STRETCH_ASPECT_RATIO)
#if SSE_VERSION>=380
#define ST_ASPECT_RATIO_DISTORTION 1.10 // multiplier for Y axis
#else
#define ST_ASPECT_RATIO_DISTORTION 1.15 // multiplier for Y axis
#endif
#endif

#if defined(SSE_VID_RECORD_AVI)//no, Fraps or other 3rd party programs will do a fantastic job
#define SSE_VID_RECORD_AVI_FILENAME "SteemVideo.avi"
#define SSE_VID_RECORD_AVI_CODEC "MPG4"
#endif

#endif//vid


//YM2149
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
#if defined(SSE_UNIX)
#define YM2149_FIXED_VOL_FILENAME "/ym2149_fixed_vol.bin" 
#else
#define YM2149_FIXED_VOL_FILENAME "\\ym2149_fixed_vol.bin"
#endif
#endif

#endif//#ifndef SSEPARAMETERS_H
