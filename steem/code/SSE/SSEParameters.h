////////////////
// Parameters //
////////////////

/*  Some important emulation parameters are concentrated here, which makes 
    experimenting (always fun) easier.
*/

#pragma once
#ifndef SSEPARAMETERS_H
#define SSEPARAMETERS_H


//////////
// ACIA //
//////////

#if defined(SS_ACIA)

#if defined(SS_ACIA_IRQ_DELAY)
#define SS_ACIA_IRQ_DELAY_CYCLES 20 // est. gap between RX and IRQ in CPU cycles
/*  Delay before keys sent by the IKBD are received by the ACIA.
    It was about 20 in Steem 3.2 (>10.000 cycles), I brought it back to
    12 in 3.3 to be closer to 7.200 cycles mentioned in WinsTON/Hatari, but
    experiments with 6301 'true emu' could point to a value > 10.000, 
    vindicating Steem authors! Though it is all guessing.
*/
#if defined(SS_IKBD_6301)
#define SS_6301_TO_ACIA_IN_CYCLES (HD6301EMU_ON?HD6301_CYCLES_TO_SEND_BYTE*8:7200)
#define SS_6301_TO_ACIA_IN_HBL (HD6301EMU_ON?HD6301_CYCLES_TO_SEND_BYTE_IN_HBL:(screen_res==2?24:12))
#else
#define SS_6301_TO_ACIA_IN_CYCLES (7200) // from WinSTon
#define SS_6301_TO_ACIA_IN_HBL (screen_res==2?24:12) // to be <7200
#endif
#endif

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

#if defined(_DEBUG) // change at leisure
#define EXCEPTIONS_REPORTED 25
#else // in file
#define EXCEPTIONS_REPORTED 155
#endif

#endif


///////////
// Hacks //
///////////

#if defined(SS_HACKS)
#define SS_SIGNAL_TOS_PATCH106 1 // checking all ST files we open
#define SS_SIGNAL_SHIFTER_CONFUSED_1 2 // temp hacks for 3.4
#define SS_SIGNAL_SHIFTER_CONFUSED_2 3 // not looking for elegance!
#endif


//////////
// IKBD //
//////////

#if defined(SS_IKBD)

#if defined(SS_IKBD_6301)

#define HD6301_ROM_FILENAME "HD6301V1ST.img" // cooler than 'keyboard.rom'
#define HD6301_ROM_CHECKSUM 451175 // BTW this rom sends $F1 after reset (80,1)
/*  The HD6301 runs at 1MH, the M68000 at +-8MH
    Scanline = 512 M68000 cycles, but only 512/8=64 HD6301 cycles
*/
#define HD6301_CYCLES_PER_SCANLINE 64 // 64
#define IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESS_ALT (HD6301EMU_ON?45:2)
// 2 is very wrong, we know that - todo
// in HBL, for Steem, -1 for precise timing (RX/IRQ delay)
#define HD6301_CYCLES_TO_SEND_BYTE_IN_HBL \
((HD6301_CYCLES_TO_SEND_BYTE*8/(shifter_freq_at_start_of_vbl==50?512: (screen_res==2?160:508)))-1)
#define HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL \
(HD6301_CYCLES_TO_RECEIVE_BYTE*8/ (shifter_freq_at_start_of_vbl==50?512:(screen_res==2?160:508)))
#define HD6301_MAX_DIS_INSTR 2000 
/*  Guessed timings, probably wrong
    1024 is the theoretical minimal value (x8=8192)
    1300;1350 is the minimum for Froggies menu (x8=10400-10800), it also
    makes Unlimited Bobs flicker-free.
    Above that, GEM starts to act up
*/
#define HD6301_CYCLES_TO_SEND_BYTE 1300
#define HD6301_CYCLES_TO_RECEIVE_BYTE 1350
// far from ideal, but maybe we must change method or timings instead
#define HD6301_MOUSE_SPEED_CHUNKS 15
#define HD6301_MOUSE_SPEED_CYCLES_PER_CHUNK 1000

#else

#define IKBD_HBLS_FROM_COMMAND_WRITE_TO_PROCESS_ALT (2) //2 or 3!!!

#endif

#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SS_INTERRUPT)
/*  Timings theory=44 but "at least" 12 cycles more are necessary
    56 was already the Steem value. Changing those values may apparently
    fix some problems, but it's generally a false lead (tried that).
*/
#if defined(SS_INT_MFP)
#define SS_INT_MFP_TIMING 56
#endif
#if defined(SS_INT_HBL)
#define SS_INT_HBL_TIMING 56
#endif
#if defined(SS_INT_VBL)
#define SS_INT_VBL_TIMING 56
#endif

#if defined(SS_MFP_RATIO) 
/*  There was a hack in Steem up to v3.3, where the CPU/MFP frequency ratio
    was set different from what was measured on an STE by Steem authors, that
    allowed Lethal Xcess to run. This hack was hiding a blitter timing problem,
    however (the 64 NOP issue), which explains why the game wasn't sensitive
    to the ratio in STF mode. Now that the blitter bug has been corrected,
    we use the measured value (when SS_MFP_RATIO_STE is defined).
    For STF we use the precise value of the MFP quarts and the real value of
    a typical "PAL" STF, as read on atari-forum (ijor?)
*/

#define  MFP_CLK_LE 2451 // Steem 3.2
#define  MFP_CLK_LE_EXACT 2451134 // Steem 3.2
#define  CPU_STE_TH 8000000 // Steem 3.2 

// Between 2451168 and 2451226 cycles, as measured by Steem authors:
#define  MFP_CLK_STE_EXACT 2451182 // we use this for STE now

#define  CPU_STF_PAL 8021248 // ( 2^8 * 31333 )
#define  MFP_CLK_TH 2457
#define  MFP_CLK_TH_EXACT 2457600 // ( 2^15 * 3 * 5^2 )
#endif

#if defined(SS_INT_VBI_START)
#undef SS_INT_VBI_START
#define SS_INT_VBI_START 68
#endif

#if defined(SS_INT_VBL_STF)
#define HBL_FOR_STE 444
#define HBL_FOR_STF (444+4+4) // strange that it's +8 now...
#endif

#endif


/////////
// OSD //
/////////

#if defined(SS_OSD)
#define RED_LED_DELAY 1500 // Red floppy led for writing, in ms
#define HD_TIMER 100 // Yellow hard disk led (imperfect timing)
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
#define LARGE_BORDER_SIDE 40 // making many hacks necessary (removed)
#endif
#define LARGE_BORDER_SIDE_WIN 40 // max for 800x600 display (fullscreen)
#define VERY_LARGE_BORDER_SIDE 48 // 416 pixels wide for emulation
#define VERY_LARGE_BORDER_SIDE_WIN 46 // trick, 412 pixels wide for rendering
#define ORIGINAL_BORDER_BOTTOM 40 
#define LARGE_BORDER_BOTTOM 45 // & 48 if player wants
#define VERY_LARGE_BORDER_BOTTOM 50  // counts for raster fx
#endif

#if defined(SS_VID_SHIFTER_EVENTS)
#define VID_SHIFTER_EVENTS_FILENAME "shifter_tricks.txt"
#endif

#endif


#endif//#ifndef SSEPARAMETERS_H