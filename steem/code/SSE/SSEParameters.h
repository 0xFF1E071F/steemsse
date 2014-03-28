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

#if defined(SS_STRUCTURE_SSE6301_OBJ) && defined(__cplusplus)
#include <conditions.h>
#include <draw.decla.h>
#endif


/////////////
// GENERAL //
/////////////


#if defined(STEVEN_SEAGAL)

#define ACT ABSOLUTE_CPU_TIME

#if defined(SS_SHIFTER_TRICKS) && defined(SS_MFP_RATIO)

#define HBL_PER_SECOND (CpuNormalHz/Shifter.CurrentScanline.Cycles)
//Frequency   50          60            72
//#HBL/sec    15666.5 15789.85827 35809.14286

#else 


#if defined(SS_FDC_PRECISE_HBL)//todo table
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

#if defined(SS_ACIA)

#if defined(SS_ACIA_IRQ_DELAY)// not defined anymore (v3.5.2), see MFP

#define SS_ACIA_IRQ_DELAY_CYCLES 20 // est. gap between RX and IRQ in CPU cycles
/*  Delay before keys sent by the IKBD are received by the ACIA.
    It was about 20 in Steem 3.2 (>10.000 cycles), I brought it back to
    12 in 3.3 to be closer to 7.200 cycles mentioned in WinsTON/Hatari, but
    experiments with 6301 'true emu' could point to a value > 10.000, 
    vindicating Steem authors! Though it is all guessing.
*/

#if !defined(SS_IKBD)
#define HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL \
(1350*8/ \
(shifter_freq_at_start_of_vbl==50?SCANLINE_TIME_IN_CPU_CYCLES_50HZ:\
(screen_res==2?SCANLINE_TIME_IN_CPU_CYCLES_70HZ:\
SCANLINE_TIME_IN_CPU_CYCLES_60HZ)))
#endif

#endif

#if defined(SS_IKBD_6301)

#define SS_6301_TO_ACIA_IN_CYCLES (HD6301_CYCLES_TO_SEND_BYTE*HD6301_CYCLE_DIVISOR)
#define SS_6301_TO_ACIA_IN_HBL (HD6301EMU_ON?HD6301_CYCLES_TO_SEND_BYTE_IN_HBL:(screen_res==2?24:12))

#else

#define SS_6301_TO_ACIA_IN_CYCLES (7200) // from WinSTon
#define SS_6301_TO_ACIA_IN_HBL (screen_res==2?24:12) // to be <7200

#endif

#if defined(SS_ACIA_TDR_COPY_DELAY)
//#define ACIA_TDR_COPY_DELAY ACIA_CYCLES_NEEDED_TO_START_TX //formerly
#define ACIA_TDR_COPY_DELAY (200) // Hades Nebula vs. Nightdawn (???)
#endif

#if defined(SS_ACIA_IRQ_DELAY2)
#define ACIA_RDRF_DELAY (20)
#endif

#else //!ACIA
#define SS_6301_TO_ACIA_IN_HBL (screen_res==2?24:12) // to be <7200
#endif


/////////////
// BLITTER //
/////////////

#if defined(SS_BLITTER)
#if defined(SS_BLT_BLIT_MODE_CYCLES) 
/*  It's not so clear in the doc, but the 64 shared bus cycles are not in CPU
    cycles but in NOP units (4 cycles). 
    It could be deduced from the example program and the mention restarting 
    the blitter takes "7 cycles". The timing table is also in NOP.
    We could choose 65 instead of 64 for some latency? But then are cycles
    lost? TESTING
    Since we prolong the access times, we make sure to check for interrupts
    more often. The smaller the value, the higher the emulation load!
*/
#undef SS_BLT_BLIT_MODE_CYCLES
#define SS_BLT_BLIT_MODE_CYCLES (65*4) // 'NOP' x 4 = cycles
#define SS_BLT_BLIT_MODE_IRQ_CHK (64) // when we check for IRQ, in cycles
#endif
#endif


/////////
// CPU //
/////////

#if defined(SS_CPU)

//todo move clock here
#endif


///////////
// DEBUG //
///////////

#if defined(SS_DEBUG) 
#if defined(SS_UNIX)
#define SS_TRACE_FILE_NAME "./TRACE.txt"
#else
#define SS_TRACE_FILE_NAME "TRACE.txt"
#endif
#define TRACE_MAX_WRITES 200000 // to avoid too big file
#endif

#if defined(SS_OSD_DEBUG_MESSAGE)
#define OSD_DEBUG_MESSAGE_LENGTH 30 // in bytes
#define OSD_DEBUG_MESSAGE_TIME 2 // in seconds
#endif

#if defined(SS_DEBUG_FAKE_IO)
#define FAKE_IO_START 0xfffb00
#define FAKE_IO_LENGTH 64*2 // in bytes
#define FAKE_IO_END (FAKE_IO_START+FAKE_IO_LENGTH-2) // starting address of last one
#define STR_FAKE_IO_CONTROL "Control mask browser"
#endif

#if defined(SS_DEBUG_FRAME_REPORT)
#if defined(SS_UNIX)
#define FRAME_REPORT_FILENAME "./FrameReport.txt" //a fix?
#else
#define FRAME_REPORT_FILENAME "FrameReport.txt"
#endif
#endif


/////////
// DMA //
/////////

#if defined(SS_DMA)
#if defined(SS_DMA_DELAY)
#define SS_DMA_ACCESS_DELAY 20
#endif
#endif


///////////
// DRIVE //
///////////

#if defined(SS_DRIVE)

#define DRIVE_11SEC_INTERLEAVE 6
#define DRIVE_RPM 300
#define DRIVE_MAX_CYL 83

#if defined(SS_DRIVE_BYTES_PER_ROTATION)
#define DRIVE_BYTES_ROTATION (6250+20) // could be true though
#else
#define DRIVE_BYTES_ROTATION (8000) // Steem 3.2
#endif

#if defined(SS_DRIVE_RW_SECTOR_TIMING2)
#define FDC_SECTOR_GAP_BEFORE_IRQ_9_10 (3+(SSE_HACKS_ON?28:0)) //CRC + hack (FDCTNF by Petari)
#define FDC_SECTOR_GAP_BEFORE_IRQ_11 (3)
#endif

#if defined(SS_DRIVE_SOUND)
#define DRIVE_SOUND_DIRECTORY "\\DriveSound"
#define DRIVE_SOUND_BUZZ_THRESHOLD 7 // distance between tracks
#endif

#endif


/////////
// FDC //
/////////

#if defined(SS_FDC)

#if defined(SS_FDC_ACCURATE)
#endif//#if defined(SS_FDC_ACCURATE)

#endif//#if defined(SS_FDC)


///////////
// Hacks //
///////////

#if defined(SS_HACKS)
enum {
SS_SIGNAL_TOS_PATCH106=1, // checking all ST files we open
SS_SIGNAL_SHIFTER_CONFUSED_1,// temp hacks for 3.4
SS_SIGNAL_SHIFTER_CONFUSED_2,// not looking for elegance!
SS_SIGNAL_DRIVE_SPINNING_AT_COMMAND,
SS_SIGNAL_ENUM_EnumDisplayModes, // wait until finished (?)
};
#endif


//////////
// IKBD //
//////////

#if defined(SS_IKBD)

#if defined(SS_IKBD_6301)
/*
#if defined(SS_UNIX)
#define HD6301_ROM_FILENAME "./HD6301V1ST.img" 
#else
#define HD6301_ROM_FILENAME "HD6301V1ST.img"
#endif
*/

#if defined(SS_UNIX)
#define HD6301_ROM_FILENAME "/HD6301V1ST.img" 
#else
#define HD6301_ROM_FILENAME "\\HD6301V1ST.img"
#endif



#define HD6301_ROM_CHECKSUM 451175 // BTW this rom sends $F1 after reset (80,1)
#endif

// Parameters used in true and fake 6301 emu

#define HD6301_CYCLES_PER_SCANLINE 64 // used if SS_SHIFTER not defined
#define HD6301_CYCLE_DIVISOR 8 // the 6301 runs at 1MHz (verified by Stefan jL)

// in HBL, for Steem, -1 for precise timing (RX/IRQ delay)
#define HD6301_CYCLES_TO_SEND_BYTE_IN_HBL \
((HD6301_CYCLES_TO_SEND_BYTE*HD6301_CYCLE_DIVISOR/\
(shifter_freq_at_start_of_vbl==50?SCANLINE_TIME_IN_CPU_CYCLES_50HZ: \
(screen_res==2?SCANLINE_TIME_IN_CPU_CYCLES_70HZ:\
SCANLINE_TIME_IN_CPU_CYCLES_60HZ)))-1)

#define HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL \
(HD6301_CYCLES_TO_RECEIVE_BYTE*HD6301_CYCLE_DIVISOR/ \
(shifter_freq_at_start_of_vbl==50?SCANLINE_TIME_IN_CPU_CYCLES_50HZ:\
(screen_res==2?SCANLINE_TIME_IN_CPU_CYCLES_70HZ:\
SCANLINE_TIME_IN_CPU_CYCLES_60HZ)))
#define HD6301_MAX_DIS_INSTR 2000 


#define HD6301_CYCLES_TO_SEND_BYTE (1350) // see ACIA6850.txt
#define HD6301_CYCLES_TO_RECEIVE_BYTE (1350)

// far from ideal, but maybe we must change method or timings instead
#define HD6301_MOUSE_SPEED_CHUNKS 15
#define HD6301_MOUSE_SPEED_CYCLES_PER_CHUNK 1000


#if defined(SS_IKBD_FAKE_CUSTOM)
/*  This is not some complicated technical aspect of emulation.
    Checksums are used for gross hacks.
*/
#define SS_IKBD_FAKE_CUSTOM_CHECKSUM_DRAGONNELS_LOADER 0x789
#define SS_IKBD_FAKE_CUSTOM_CHECKSUM_DRAGONNELS 0x2643
#define SS_IKBD_FAKE_CUSTOM_CHECKSUM_FROGGIES_LOADER 0x784
#define SS_IKBD_FAKE_CUSTOM_CHECKSUM_FROGGIES 0x66FE
#define SS_IKBD_FAKE_CUSTOM_CHECKSUM_TB2_LOADER 0x5CA
#define SS_IKBD_FAKE_CUSTOM_CHECKSUM_TB2 0x356C
#endif

#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SS_INTERRUPT)
/*  
    IACK (interrupt acknowledge)  16
    Exception processing          40
    Total                         56

Motorola:
Interrupt* 46(5/4)

* The interrupt acknowledge and breakpoint cycles
are assumed to take four clock periods.

*/

#if defined(SS_INT_MFP)
#define SS_INT_MFP_TIMING 56 // many cases
#endif
#if defined(SS_INT_HBL)
#define SS_INT_HBL_TIMING 56 // SNYD/TCB
#endif
#if defined(SS_INT_VBL)
#ifdef SS_BETA//temp test (we don't think it's correct)
#define SS_INT_VBL_TIMING 56
#endif
#endif


#if defined(SS_INT_VBI_START)
#undef SS_INT_VBI_START
#define SS_INT_VBI_START (68) // default = STE
#endif

#if defined(SS_INT_VBL_STF) // modest hack still works
#define HBL_FOR_STE 444
#define HBL_FOR_STF (HBL_FOR_STE+4+(SSE_HACKS_ON?4:0)) //TODO
#endif

#endif


/////////
// IPF //
/////////

#if defined(SS_IPF)
#define SS_IPF_PLUGIN_FILE "CAPSImg.dll" //Windows
#define SS_IPF_FREQU 8000000//? CPU speed? - even for that I wasn't helped!

#ifdef SS_IPF_CTRAW
#undef SS_IPF_CTRAW
#define SS_IPF_CTRAW "CTR" 
#endif
#ifdef SS_IPF_KFSTREAM
#undef SS_IPF_KFSTREAM
#define SS_IPF_KFSTREAM "RAW" 
#endif
#ifdef SS_IPF_DRAFT
#undef SS_IPF_DRAFT
#define SS_IPF_DRAFT "RAW"  //?
#endif



#endif//ipf


/////////
// MFP //
/////////

#if defined(SS_MFP)

#if defined(SS_MFP_RATIO) 
/*  There was a hack in Steem up to v3.3, where the CPU/MFP frequency ratio
    was set different from what was measured on an STE by Steem authors, that
    allowed Lethal Xcess to run. This hack was hiding a blitter timing problem,
    however (the 64 NOP issue), which explains why the game wasn't sensitive
    to the ratio in STF mode. Now that the blitter bug has been corrected,
    we use the measured value (when SS_MFP_RATIO_STE is defined).
    For STF we use the precise value of the MFP quarts and the real value of
    a typical "PAL" STF, as read on atari-forum (ijor?)
    3.5.1: same ratio for STE, it could make DMA sound emu more precise
*/

#define  MFP_CLK_LE 2451 // Steem 3.2
#define  MFP_CLK_LE_EXACT 2451134 // Steem 3.2
 
// Between 2451168 and 2451226 cycles, as measured by Steem authors:

#define  CPU_STE_TH 8000000 // Steem 3.2
#define  MFP_CLK_STE_EXACT 2451182 // not if 'STE as STF' is defined
#define  CPU_STF_PAL (8021248) // ( 2^8 * 31333 )
#define  CPU_STF_ALT (8007100) //ljbk's?
#define  CPU_STE_PAL (CPU_STF_PAL+64) //64 for DMA sound!
#define  MFP_CLK_TH 2457
#define  MFP_CLK_TH_EXACT 2457600 // ( 2^15 * 3 * 5^2 )
#endif

#if defined(SS_MFP_WRITE_DELAY1)
#define MFP_WRITE_LATENCY 8 // 8 = the smallest for Audio Artistic
#endif

#endif//mfp


/////////
// MMMU //
/////////

#if defined(SS_MMU)

#if defined(SS_MMU_WAKE_UP_DL)
#define WU_SHIFTER_PANIC 5
#else
#define WU_SHIFTER_PANIC 3
#endif

#endif


/////////
// OSD //
/////////

#if defined(SS_OSD)
#define RED_LED_DELAY 1500 // Red floppy led for writing, in ms
#define HD_TIMER 100 // Yellow hard disk led (imperfect timing)
#endif


/////////////
// SHIFTER //
/////////////

#ifdef SS_BETA
//#define SS_SHIFTER_SKIP_SCANLINE 5//-29 // fetch but only draw colour 0 (debug)//MFD
#endif
#define VERT_OVSCN_LIMIT (502) //502

#if defined(SS_SHIFTER_EVENTS)
#define SHIFTER_EVENTS_FILENAME "shifter_tricks.txt"
#endif

#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
#define WU2_PLUS_CYCLES 4 // we make cycles +2
#else
#define WU2_PLUS_CYCLES 2 // we don't
#endif



///////////
// SOUND //
///////////

#if defined(SS_SOUND_LOW_PASS_FILTER)
#define LOW_PASS_FILTER_FREQ 6000
#define LOW_PASS_FILTER_GAIN (0)
#endif

#if defined(SS_SOUND_FILTER_STF)
#define SS_SOUND_FILTER_STF_V ((*source_p+dv)/2)
#define SS_SOUND_FILTER_STF_DV (v)//((*source_p+dv)/2)
#endif

#if defined(SS_SOUND_MICROWIRE_WRITE_LATENCY)
// to make XMAS 2004 scroller work, should it be lower?
#define MICROWIRE_LATENCY_CYCLES (128+16)
#endif

/////////
// STF //
/////////


#if defined(SS_STF_VERT_OVSCN)

#define STF_VERT_OVSCN_OFFSET 4

#endif

/////////////
// VARIOUS //
/////////////

#if defined(SS_VARIOUS)

#define README_FONT_NAME "Courier New"
#define README_FONT_HEIGHT 16
#define STEEM_SSE_FAQ_TXT "Steem SSE FAQ.txt"

#endif


///////////
// VIDEO //
///////////

#if defined(SS_VIDEO)

#if defined(SS_VID_BORDERS)
#define ORIGINAL_BORDER_SIDE 32
#if defined(SS_VID_BORDERS_LB_DX)
#define LARGE_BORDER_SIDE 48 // trick, making it 40 at rendering
#else
#define LARGE_BORDER_SIDE 40 // making many hacks necessary 
#endif
#define LARGE_BORDER_SIDE_WIN 40 // max for 800x600 display (fullscreen)

#define VERY_LARGE_BORDER_SIDE 48 // 416 pixels wide for emulation
#define VERY_LARGE_BORDER_SIDE_WIN 46 // trick, 412 pixels wide for rendering
#define ORIGINAL_BORDER_BOTTOM 40 
#define LARGE_BORDER_BOTTOM 48
#define VERY_LARGE_BORDER_BOTTOM 50  // counts for raster fx

#define ORIGINAL_BORDER_TOP 30
#define BIG_BORDER_TOP 36 // for The Musical Wonder 1990

#if defined(SS_VID_BORDERS_416) && defined(SS_VID_BORDERS_412)
#define BIGGEST_DISPLAY 3
#else
#define BIGGEST_DISPLAY 2
#endif

#endif



#if defined(SS_VID_RECORD_AVI)
#define SS_VID_RECORD_AVI_FILENAME "SteemVideo.avi"
#define SS_VID_RECORD_AVI_CODEC "MPG4"
#endif

#endif


#endif//#ifndef SSEPARAMETERS_H