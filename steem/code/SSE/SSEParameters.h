////////////////
// Parameters //
////////////////

#pragma once
#ifndef SSEPARAMETERS_H
#define SSEPARAMETERS_H
#if defined(SSE_BUILD)

#if defined(__cplusplus)
#include <conditions.h>
#include <draw.decla.h>
#endif

#if defined(STRUCTURE_ALIGNMENT)
#error STRUCTURE_ALIGNMENT defined!
#endif
#if defined(SSE_X64)
#define STRUCTURE_ALIGNMENT 16 //default in Win64
#else
#define STRUCTURE_ALIGNMENT 8 //default in Win32
#endif


/////////////
// BLITTER //
/////////////

#if defined(SSE_BLITTER)

#define BLITTER_LATCH_LATENCY 4 // not sure what it precisely is nor of value

#endif


//////////
// CAPS //
//////////

#if defined(SSE_DISK_CAPS)
#if defined(SSE_CPU_MFP_RATIO)
#define SSE_DISK_CAPS_FREQU CpuNormalHz
#else
#define SSE_DISK_CAPS_FREQU 8000000
#endif
#endif


/////////
// CPU //
/////////

#ifdef SSE_CPU_394A1
#define CPU_STOP_DELAY 4 //?
#else 
#define CPU_STOP_DELAY 8
#endif

#if defined(SSE_CPU_IPL_DELAY)
#define CPU_IPL_CHECK_TIME 2 // cycles before end of instruction when IPL is scanned
#endif

#if defined(SSE_CPU_HISPEED_392)
#if defined(SSE_CPU_4GHZ)
#define CPU_MAX_HERTZ (4000000000)
#elif defined(SSE_CPU_3GHZ)
#define CPU_MAX_HERTZ (3000000000)
#elif defined(SSE_CPU_2GHZ)
#define CPU_MAX_HERTZ (2000000000) //v392
#elif defined(SSE_CPU_1GHZ)
#define CPU_MAX_HERTZ (1000000000)
#elif defined(SSE_CPU_512MHZ)
#define CPU_MAX_HERTZ (512000000)
#elif defined(SSE_CPU_256MHZ)
#define CPU_MAX_HERTZ (256000000)
#else
#define CPU_MAX_HERTZ (128000000) //Steem 3.2
#endif
#endif

#if defined(SSE_CPU_MFP_RATIO) 
/*
The master clock crystal and derived CPU clock table is:
PAL (all variants)       32.084988   8.021247
NTSC (pre-STE)           32.0424     8.0106
Mega ST                  32.04245    8.0106125
NTSC (STE)               32.215905   8.053976
Peritel (STE) (as PAL)   32.084988   8.021247
Some STFs                32.02480    8.0071
*/

#define  CPU_CLOCK_STF_PAL 8021247
#define  CPU_CLOCK_STF_MEGA (8010613) //rounded
#define  CPU_CLOCK_STE_PAL (CPU_CLOCK_STF_PAL) 

#endif


///////////
// DEBUG //
///////////

#if defined(SSE_DEBUG_TRACE_FILE)//3.8.2
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

#if defined(SSE_BOILER_FRAME_REPORT)
#if defined(SSE_UNIX)
#define FRAME_REPORT_FILENAME "./FrameReport.txt" //a fix?
#else
#define FRAME_REPORT_FILENAME "FrameReport.txt"
#endif
#endif


//////////
// DISK //
//////////

#define DISK_BYTES_PER_TRACK (6256)
/*  #bytes/track
    The value generally seen is 6250.
    The value for 11 sectors is 6256. It's possible if the clock is higher than
    8mhz, which is the case on the ST.
*/

#define DISK_11SEC_INTERLEAVE 6


///////////
// DRIVE //
///////////

#if defined(SSE_DRIVE)

#define DRIVE_RPM 300
#define DRIVE_MAX_CYL 83

#if defined(SSE_DISK_STW_FAST) // hacky by definition
#define SSE_STW_FAST_CYCLES_PER_BYTE 4
#define SSE_STW_FAST_IP_MULTIPLIER 8
#endif

#define DRIVE_SOUND_BUZZ_THRESHOLD 7 // distance between tracks

#endif


///////////
// FILES //
///////////

#define ACSI_HD_DIR "ACSI"
#define SSE_VID_RECORD_AVI_FILENAME "SteemVideo.avi"
#define DISK_HFE_BOOT_FILENAME "HFE_boot.bin"
#define HD6301_ROM_FILENAME "HD6301V1ST.img"
#define DRIVE_SOUND_DIRECTORY "DriveSound"
#define YM2149_FIXED_VOL_FILENAME "ym2149_fixed_vol.bin"
#define PASTI_DLL "pasti.dll"
#define SSE_DISK_CAPS_PLUGIN_FILE "CAPSImg.dll"
#define ARCHIVEACCESS_DLL "ArchiveAccess.dll"
#define UNRAR_DLL "unrar.dll" 
#define UNZIP_DLL "unzipd32.dll" 
#define STEEM_SSE_FAQ "Steem SSE FAQ"
#define STEEM_HINTS "Hints"
#define STEEM_MANUAL_SSE "Steem SSE manual"
#define STEEM_RELEASE_NOTES "Steem SSE 3.9.4" //TODO


/////////
// GLU //
/////////

#if defined(SSE_GLUE)
// extremely important parameters, modified according to ST model and wakestate

enum EGlueTimings {
  GLU_DE_ON_72=6, //+ WU_res_modifier; STE-4
  GLU_DE_OFF_72=166,
  GLU_DE_ON_60=52, //+ WU_sync_modifier; STE -16
  GLU_DE_OFF_60=372,
  GLU_DE_ON_50=56, //+ WU_sync_modifier; STE -16
  GLU_DE_OFF_50=376,
  GLU_HBLANK_OFF_50=28, //+ WU_sync_modifier
  GLU_HSYNC_ON_50=464, //+ WU_res_modifier, STE-2
  GLU_HSYNC_DURATION=40,
  GLU_RELOAD_VIDEO_COUNTER_50=50, //problem: 2 cycles precision TODO
  GLU_RELOAD_VIDEO_COUNTER_72=14, // + at last line
  GLU_TRIGGER_VBI_50=64,	//STE+4
  GLU_DECIDE_NCYCLES=54, //+ WU_sync_modifier, STE +2
  GLU_VERTICAL_OVERSCAN_50=GLU_HSYNC_ON_50+GLU_HSYNC_DURATION // 504 //+ WU_sync_modifier, STE -2
};

#endif


////////
//GUI //
////////

#define WINDOW_TITLE_MAX_CHARS 20+5 //argh, 20 wasn't enough
#define README_FONT_NAME "Courier New"
#define README_FONT_HEIGHT 16
#define EXT_TXT ".txt" //save bytes?
#define CONFIG_FILE_EXT "ini" // ini, cfg?


//////////
// IKBD //
//////////

#if defined(SSE_IKBD_6301)
#define HD6301_CYCLE_DIVISOR 8 // the 6301 runs at 1MHz (verified by Stefan jL)
#define HD6301_CLOCK (1000000) //used in 6301/ireg.c for mouse speed
#define HD6301_ROM_CHECKSUM 451175 // BTW this rom sends $F1 after reset (80,1)
#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SSE_INTERRUPT)

#define HBL_IACK_LATENCY 28 // 10-28 ?
#define VBL_IACK_LATENCY 28

#if defined(SSE_CPU_E_CLOCK)
#define ECLOCK_AUTOVECTOR_CYCLE 10 // IACK starts at cycle 10 (?)
#endif

#endif//int


/////////
// MFP //
/////////

#if defined(SSE_INT_MFP)
/*  
    MFP clock (no variation) ~ 2457600 hz

    MFP_IACK_LATENCY: it may seem high but it's not #IACK cycles, it's when 
    IACK ends.
    Final Conflict, Super Hang-On, Anomaly menu, Froggies/OVR

    MFP_TIMER_SET_DELAY includes latency before the timer starts +
    time to IRQ.
    DSOTS, Extreme rage, Schnusdie, Lethal Xcess, Panic

    MFP_WRITE_LATENCY supposes that data is copied with some delay into
    registers, which may have unintended effects.
    Audio Artistic (timer is stopped before IRQ so it triggers only once)
    Spurious.tos (mask out just after IRQ triggers spurious)
*/

#define MFP_CLOCK 2457600
#define MFP_IACK_LATENCY (28) 

#if defined(SSE_INT_MFP_READ_DELAY_392)
/*  Explanation for the negative value:
    Time necessary to copy the register value to the IO lines. But then it
    is general, not just for data timers...
    (smells like a hack...)
*/
#define MFP_READ_REGISTER_DELAY (-2)
#endif

#if defined(SSE_INT_MFP_394B)
#define MFP_TIMER_SET_DELAY (5)
#else
#define MFP_TIMER_SET_DELAY (4) 
#endif

#if defined(SSE_INT_MFP_TIMERS_WOBBLE)
#if defined(SSE_INT_MFP_394B)
#define MFP_TIMERS_WOBBLE (2) 
#else
#define MFP_TIMERS_WOBBLE (4)
#endif
#endif // wobble

#if defined(SSE_INT_MFP_394C)
#define MFP_WRITE_LATENCY (4)
#else
#define MFP_WRITE_LATENCY (2)
#endif

#endif//mfp


/////////
// OSD //
/////////

#if defined(SSE_OSD)
#define HD_TIMER 100 // Yellow hard disk led (imperfect timing)
#endif


///////////
// SOUND //
///////////

#if defined(SSE_SOUND_FILTERS)
#define SSE_SOUND_FILTER_MONITOR_V ((*source_p+dv)/2)
#define SSE_SOUND_FILTER_MONITOR_DV (v)//((*source_p+dv)/2)
#endif

#if defined(SSE_SOUND_MICROWIRE_WRITE_LATENCY)
// to make XMAS 2004 scroller work, should it be lower?
#define MICROWIRE_LATENCY_CYCLES (128+16)
#endif

#if defined(SSE_SOUND_16BIT_CENTRED)
#define DMA_SOUND_MULTIPLIER (32) // <<5 ?
#endif

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
#define YM_LOW_PASS_FREQ (10.5) //in khz
#endif


/////////////
// TIMINGS //
/////////////

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_CPU_MFP_RATIO)
#define HBL_PER_SECOND (CpuNormalHz/Glue.CurrentScanline.Cycles) //TODO assert
//#endif
//Frequency   50          60            72
//#HBL/sec    15666.5 15789.85827 35809.14286

#else 
#if defined(SSE_FDC_ACCURATE_TIMING)//todo table
#define HBL_PER_FRAME ( (shifter_freq_at_start_of_vbl==50)?HBLS_50HZ: \
  ( (shifter_freq_at_start_of_vbl==60)? HBLS_60HZ : HBLS_72HZ))
#else
#define HBL_PER_FRAME ( shifter_freq_at_start_of_vbl==50?HBLS_50HZ: \
  (shifter_freq_at_start_of_vbl==60?HBLS_60HZ:(HBLS_72HZ/2)))
#endif

#define HBL_PER_SECOND HBLS_PER_SECOND_AVE//(HBL_PER_FRAME*shifter_freq_at_start_of_vbl)  //still not super accurate
#endif


/////////
// TOS //
/////////

#if defined(SSE_TOS_PRG_AUTORUN)
#define AUTORUN_HD (2+'Z'-'C')//2=C, Z: is used for PRG support
#endif

#if defined(SSE_STF_MATCH_TOS)
#define DEFAULT_TOS_STF (HardDiskMan.DisableHardDrives?0x102:0x104) // how caring!
#define DEFAULT_TOS_STE 0x162
#endif


///////////
// VIDEO //
///////////

#if defined(SSE_VIDEO)

#if defined(SSE_VID_BORDERS)

#define ORIGINAL_BORDER_SIDE 32
#define VERY_LARGE_BORDER_SIDE 48
#define VERY_LARGE_BORDER_SIDE_WIN 46 //?
#define ORIGINAL_BORDER_BOTTOM 40
#define LARGE_BORDER_BOTTOM 45
#define VERY_LARGE_BORDER_BOTTOM 45
#define ORIGINAL_BORDER_TOP 30
#define BIG_BORDER_TOP 38 // 50hz
#define BIGGEST_DISPLAY 3 //416

#endif

#if defined(SSE_VID_ST_ASPECT_RATIO)
#define ST_ASPECT_RATIO_DISTORTION 1.10f // multiplier for Y axis
#endif

#if defined(SSE_VID_RECORD_AVI) //DD-only, poor result
#define SSE_VID_RECORD_AVI_CODEC "MPG4"
#endif

#endif//vid


#endif//sse
#endif//#ifndef SSEPARAMETERS_H
