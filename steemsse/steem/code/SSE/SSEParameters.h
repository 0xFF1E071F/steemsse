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

//////////
// ACIA //
//////////

#if defined(SSE_ACIA)

/*
The ACIA master clock is 500kHz.
       |     |Clock divide                                      | ||
       |     |00 - Normal --------------------------------------+-+|
       |     |01 - Div by 16 -----------------------------------+-+|
       |     |10 - Div by 64 -----------------------------------+-+|
       |     |11 - Master reset --------------------------------+-'|

10bits per byte

*/

#define HD6301_TO_ACIA_IN_CYCLES (acia[0].TransmissionTime())
#define ACIA_TO_HD6301_IN_CYCLES (acia[0].TransmissionTime())
//TODO:
#define HD6301_TO_ACIA_IN_HBL (OPTION_C1?HD6301_CYCLES_TO_SEND_BYTE_IN_HBL:(screen_res==2?24:12)) //is a mod??



#if defined(SSE_ACIA_TDR_COPY_DELAY)
//#define ACIA_TDR_COPY_DELAY ACIA_CYCLES_NEEDED_TO_START_TX //formerly
#define ACIA_TDR_COPY_DELAY (200) // Hades Nebula vs. Nightdawn (???)
#endif

#define ACIA_MIDI_OUT_CYCLES (acia[1].TransmissionTime())
#define ACIA_MIDI_IN_CYCLES (acia[1].TransmissionTime())

#else //!ACIA
#define HD6301_TO_ACIA_IN_HBL (screen_res==2?24:12) // to be <7200
#endif//ACIA


/////////////
// BLITTER //
/////////////

#if defined(SSE_BLITTER)


#define BLITTER_LATCH_LATENCY 4 // not sure what it precisely is nor of value

#if defined(SSE_BLT_BLIT_MODE_CYCLES) && !defined(SSE_BLT_MAIN_LOOP)
#define BLITTER_BLIT_MODE_CYCLES (256) //not used in v3.9.1
#endif

#endif


//////////
// CAPS //
//////////

#if defined(SSE_DISK_CAPS)
#if defined(SSE_CPU_MFP_RATIO)
#define SSE_DISK_CAPS_FREQU CpuNormalHz
#else
#define SSE_DISK_CAPS_FREQU 8000000//? CPU speed? - even for that I wasn't helped!
#endif
#endif


/////////
// CPU //
/////////

#if defined(SSE_CPU)

#define CPU_STOP_DELAY 8

#endif

#if defined(SSE_CPU_HISPEED_392) //GHz!=1024MHz
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

#define  CPU_STF_MEGA (8010613) //rounded

#define  CPU_STF_PAL 8021247
#define  CPU_STF_ALT (8007100) //ljbk's? 
#if defined(SSE_CPU_MFP_RATIO_STE)
#define  CPU_STE_PAL (OPTION_HACKS?8021030:CPU_STF_PAL)
#else
#define  CPU_STE_PAL (CPU_STF_PAL) 
#endif
#endif


///////////
// DEBUG //
///////////

//#if defined(SSE_DEBUG) 
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



#if defined(SSE_DISK_GHOST)
#define AVG_HBLS_SECTOR (200)
#endif

//////////
// DISK //
//////////

/*  #bytes/track
    The value generally seen is 6250.
    The value for 11 sectors is 6256. It's possible if the clock is higher than
    8mhz, which is the case on the ST.
*/
#define DISK_BYTES_PER_TRACK (6256)

///////////
// DRIVE //
///////////

#if defined(SSE_DRIVE_OBJECT)

#define DRIVE_11SEC_INTERLEAVE 6
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
#define STEEM_RELEASE_NOTES "Steem SSE 3.9.2" //TODO


/////////
// GLU //
/////////


#if defined(SSE_GLUE)
// extremely important parameters, modified according to ST model and wakestate
enum {
  GLU_DE_ON_72=6, //+ WU_res_modifier; STE-4
  GLU_DE_OFF_72=166,
  GLU_DE_ON_60=52, //+ WU_sync_modifier; STE -16
  GLU_DE_OFF_60=372,
  GLU_DE_ON_50=56, //+ WU_sync_modifier; STE -16
  GLU_DE_OFF_50=376,
  GLU_HBLANK_OFF_50=28, //+ WU_sync_modifier
  GLU_HSYNC_ON_50=464, //+ WU_res_modifier, STE-2
  GLU_HSYNC_DURATION=40,
#if defined(SSE_GLUE_393A)
  GLU_RELOAD_VIDEO_COUNTER_50=50, //problem: 2 cycles precision TODO
  GLU_RELOAD_VIDEO_COUNTER_72=14, // + at last line
#else
  GLU_RELOAD_VIDEO_COUNTER_50=64-2, //+ WU_sync_modifier (STE -2?)
#endif
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

#if defined(SSE_ACIA_EVENT)
// hack for Froggies TODO
//#define HD6301_CYCLES_TO_SEND_BYTE ((OPTION_HACKS&& LPEEK(0x18)==0xFEE74)?1350:1280) // boo!
#define HD6301_CYCLES_TO_SEND_BYTE ((OPTION_HACKS&& LPEEK(0x18)==0xFEE74)?1345:1280) // boo!
#define HD6301_CYCLES_TO_RECEIVE_BYTE (HD6301_CYCLES_TO_SEND_BYTE)
#else
#define HD6301_CYCLES_TO_SEND_BYTE (1350)
#define HD6301_CYCLES_TO_RECEIVE_BYTE (1350)
#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SSE_INTERRUPT)

#if !defined(SSE_MMU_ROUNDING_BUS)
#define SSE_INT_MFP_TIMING (54)
#define SSE_INT_HBL_TIMING (54)
#define SSE_INT_VBL_TIMING (54)
#endif
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

#if defined(SSE_INT_MFP_PRESCALE)
/*  Explanation for the negative value:
    Time necessary to copy the register value to the IO lines. But then it
    is general, not just for data timers...
*/
#define MFP_READ_REGISTER_DELAY (-2)
#endif

#if defined(SSE_INT_MFP_PRESCALE)
// compensates the non-substraction in mfp_set_timer_reg()
// seems likelier this way
#define MFP_TIMER_SET_DELAY (4) 
#else
#define MFP_TIMER_SET_DELAY (8) // see DSOTS
#endif


#if defined(SSE_INT_MFP_PRESCALE)
#define MFP_TIMERS_WOBBLE (4) //<, with STE CPU ~ STF CPU, see DSOTS
#elif defined(SSE_INT_MFP_TIMERS_WOBBLE_390)
#define MFP_TIMERS_WOBBLE (4+1) //<
#else
#define MFP_TIMERS_WOBBLE (4) // &
#endif

#if defined(SSE_INT_MFP_PRESCALE)
#define MFP_WRITE_LATENCY (2)
#else
#define MFP_WRITE_LATENCY (4)//(8) 
#endif

#endif//mfp


/////////
// OSD //
/////////

#if defined(SSE_OSD)
#define RED_LED_DELAY 1500 // Red floppy led for writing, in ms
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
#if defined(SSE_STF_MATCH_TOS_390)
#define DEFAULT_TOS_STF (HardDiskMan.DisableHardDrives?0x102:0x104) // how caring!
#else
#define DEFAULT_TOS_STF 0x102
#endif
#define DEFAULT_TOS_STE 0x162
#endif


/////////////
// VERSION //
/////////////

#define SSE_VERSION 393 // normally will be 400


///////////
// VIDEO //
///////////

#if defined(SSE_VIDEO)

#if defined(SSE_VID_BORDERS)

#define ORIGINAL_BORDER_SIDE 32

#if defined(SSE_VID_BORDERS_GUI_392)
//#define LARGE_BORDER_SIDE 48
//#define LARGE_BORDER_SIDE_WIN 46 //?
#define VERY_LARGE_BORDER_SIDE 48
#define VERY_LARGE_BORDER_SIDE_WIN 46 //?
#define ORIGINAL_BORDER_BOTTOM 40
#define LARGE_BORDER_BOTTOM 45
#define VERY_LARGE_BORDER_BOTTOM 45
#else
#define LARGE_BORDER_SIDE 40 // making many hacks necessary 
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

#endif//#if defined(SSE_VID_BORDERS_GUI_392)

#define ORIGINAL_BORDER_TOP 30

#if defined(SSE_STF_LACESCAN)
#define BIG_BORDER_TOP 38 // 50hz
#else
#define BIG_BORDER_TOP 36 // for The Musical Wonder 1990
#endif

#if defined(SSE_VID_BORDERS_GUI_392) // 0:no border
#if defined(SSE_VID_D3D)
#define BIGGEST_DISPLAY 3 //416
#else
#define BIGGEST_DISPLAY 4 //416
#endif
#elif defined(SSE_VID_D3D)
#define BIGGEST_DISPLAY 2 //no more 400
#elif defined(SSE_VID_BORDERS_416) && defined(SSE_VID_BORDERS_412)
#define BIGGEST_DISPLAY 3
#else
#define BIGGEST_DISPLAY 2
#endif

#endif

#if defined(SSE_VID_ST_ASPECT_RATIO)
#define ST_ASPECT_RATIO_DISTORTION 1.10f // multiplier for Y axis
#endif

#if defined(SSE_VID_RECORD_AVI)//no, Fraps or other 3rd party programs will do a fantastic job
#define SSE_VID_RECORD_AVI_CODEC "MPG4"
#endif

#endif//vid



#endif//sse
#endif//#ifndef SSEPARAMETERS_H
