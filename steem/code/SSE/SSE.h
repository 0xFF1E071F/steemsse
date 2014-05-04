#pragma once // VC guard
#ifndef STEVEN_SEAGAL_H // BCC guard
#define STEVEN_SEAGAL_H

/*

Steem Steven Seagal Edition (SSE)
---------------------------------

This is based on the source code for Steem R63 as released by Steem authors,
Ant & Russ Hayward.
Current site for this build: 
http://ataristeven.t15.org/Steem.htm
SVN code repository is at http://code.google.com/p/steem-engine/ for
v3.3.0 and at http://sourceforge.net/projects/steemsse/  for later versions.

Added some files to the project. 
- acia.h, key_table.cpp in 'steem\code'.
-In folder 'steem\code\SSE', several files starting with 'SSE', including 
this one.
-In folder 'steem\doc\SSE', some doc files (mainly done by me)
-A folder '6301' in '3rdparty' for true emulation of the IKBD
-A folder 'avi' in '3rdparty' for recording video to AVI support
-A folder 'caps' in '3rdparty' for IPF disk image format support
-A folder 'caps_linux' in '3rdparty' for future IPF disk image format support
-A folder 'doc' in '3rdparty' for some doc files (done by others)
-A folder 'dsp' in '3rdparty' for Microwire emulation
-A file 'div68kCycleAccurate.c' in '3rdparty\pasti', to use the correct DIV 
timings found by ijor (also author of Pasti).
-A folder 'SDL-WIN' for future SDL support
-A folder 'unRARDLL' in '3rdparty' for unrar support
-A folder 'various' in '3rdparty'
-Files xxx.decla.h to better separate declaration/implementation

Other mods are in Steem code, inside blocks where STEVEN_SEAGAL is defined.
Many other defines are used to segment code. This is heavy but it makes 
debugging a lot easier (real life-savers when something is broken).
To enjoy the new features, you must define STEVEN_SEAGAL!
If not, you should get the last 3.2 build that compiles in VC6 (only
thing changed is no update attempt).

My inane comments outside of defined blocks generally are marked by 'SS:'
They don't mean that I did something cool, only that I comment the source.

I removed nothing from the original source code or comments.
		  
VC6 is used as IDE, but also Notepad and the free (and discontinued)
Borland C++ 5.5 compiler, like the original Steem.
Compatibility with those compilers is a requirement of this build.
It must also compile in Linux (gcc) for the Unix build XSteem (there's a 
working build since v3.4).
The VC6 build should be linked with the C++ library, don't count on system
DLL or it will crash in Windows Vista & 7.
The best build should be the M$ one. They're greedy but they know their 
business! 
    
SSE.h is supposed to mainly be a collection of compiling switches (defines).
It should include nothing and can be included everywhere.
Normally switches are optional, but they also are useful just to mark
code changes.

SSE_DEBUG, if needed, should be defined in the project/makefile.

LEGACY_BUILD is now needed to build Steem using the 3 big modules
Steem Emu and Helper.
Those configurations are marked with '_modules'.
When it's not defined, more separate units are compiled (work in progress).

TODO: All switches SS_ -> SSE_ to mark the branch rather than the author

Then each author may define his name (STEVEN_SEAGAL)

eg:

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH)
 //... (awesome mod)
#endif

#if defined(JEAN_CLAUDE_VAN_DAMME) && defined(SSE_CPU_PREFETCH)
 //... (awesome mod)
#endif


So if later Jean Claude Van Damme disrespects me again, all we have to do is:

//#define JEAN_CLAUDE_VAN_DAMME

and all his silly mods are gone!

*/


/////////////
// VERSION //
/////////////

#if defined(STEVEN_SEAGAL)

#define SSE_BETA //title, OSD, plus some testing - new features
//#define SSE_BETA_BUGFIX // beta for just bugfixes

#if defined(SSE_BETA) //TODO check before release what will stay beta...

#define SSE_VERSION 370 // big features coming up
#define SSE_VERSION_TXT "3.7.0" 
#ifdef DEBUG_BUILD
#define WINDOW_TITLE "Steem Boiler 3.7.0B"
#else
#define WINDOW_TITLE "Steem Beta 3.7.0"
#endif

#elif defined(SSE_BETA_BUGFIX) // bugfixes (point release)

#define SSE_VERSION 363
#define SSE_VERSION_TXT "3.6.3" 
#ifdef DEBUG_BUILD
#define WINDOW_TITLE "Steem Boiler 3.6.3B"
#else
#define WINDOW_TITLE "Steem Beta 3.6.3"
#endif

#else // release

#define SSE_VERSION 363
// check snapshot Version (in LoadSave.h); rc\resource.rc
#define SSE_VERSION_TXT "3.6.3" 
#ifdef DEBUG_BUILD
#define WINDOW_TITLE "Steem Boiler 3.6.3"
#else
#define WINDOW_TITLE "Steem SSE 3.6.3"
#endif

#endif

#endif


//////////////////
// BIG SWITCHES //
//////////////////

#if defined(STEVEN_SEAGAL)

#define SSE_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SSE_BLITTER    // spelled BLiTTER by those in the known!
#define SSE_CPU        // MC68000 microprocessor
#define SSE_CARTRIDGE  // ROM Cartridge slot
#define SSE_FLOPPY     // Group switch for disk drive (DMA, FDC, Pasti, etc)
#define SSE_GLUE       // TODO
#define SSE_HACKS      // an option for dubious fixes
#define SSE_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#define SSE_INTERRUPT  // HBL, VBL  
#define SSE_MFP        // MC68901 Multi-Function Peripheral Chip
#define SSE_MIDI       // 
#define SSE_MMU        // Memory Manager Unit (of the ST, no paging)
#define SSE_OSD        // On Screen Display (drive leds, track info, logo)
#define SSE_SDL        // Simple DirectMedia Layer
#define SSE_SHIFTER    // The legendary custom shifter and all its tricks
#define SSE_SOUND      // YM2149, STE DMA sound, Microwire
#define SSE_STF        // switch STF/STE
#define SSE_STRUCTURE  // conditions other switches (that or duplicate all)
#define SSE_TIMINGS    // TODO (only HBL now)
#define SSE_TOS        // The Operating System
#define SSE_UNIX       // Linux build must be OK too (may lag)
#define SSE_VARIOUS    // Mouse capture, keyboard click, unrar...
#define SSE_VIDEO      // large borders, screenshot, recording

#endif

// Adapt some switches (also see by the end of this file)
#ifdef WIN32
#undef SSE_UNIX
#endif

#if SSE_VERSION<380 && !defined(SSE_BETA)
#undef SSE_SDL 
#endif

#ifdef BCC_BUILD
#define SSE_SDL // it's in the makefile now (TODO?)
#endif

//////////
// TEMP //
//////////

#if defined(SSE_BETA) || defined(SSE_BETA_BUGFIX)
//#define TEST01
//#define TEST02
//#define TEST03
//#define TEST04
//#define TEST05
//#define TEST06
//#define TEST07
//#define TEST08
//#define TEST09
#endif


///////////////////////
// DETAILED SWITCHES //
///////////////////////


#if defined(STEVEN_SEAGAL)


/////////////
// GENERAL //
/////////////

// #define SS_SSE_LEAN_AND_MEAN //TODO
#define SSE_SSE_OPTION_PAGE // a new page for all our options
#define SSE_SSE_OPTION_STRUCT // structure SSEOption 
#define SSE_SSE_CONFIG_STRUCT // structure SSEConfig 
#ifdef WIN32
#define SSE_DELAY_LOAD_DLL // can run without DLL
#endif


//////////
// ACIA //
//////////

#if defined(SSE_ACIA)

#define SSE_ACIA_BUS_JAM_NO_WOBBLE // simple "fix"
//#define SSE_ACIA_BUS_JAM_PRECISE_WOBBLE //TODO
#define SSE_ACIA_DOUBLE_BUFFER_RX // only from 6301 (not MIDI) 
#define SSE_ACIA_DOUBLE_BUFFER_TX // only to 6301 (not MIDI)
//#define SSE_ACIA_IRQ_ASSERT_READ_SR //TODO
#define SSE_ACIA_IRQ_DELAY2//3.6.1 back to this approach
#define SSE_ACIA_MIDI_TIMING1 //check
#define SSE_ACIA_REGISTERS // formalising the registers
//#define SSE_ACIA_DONT_CLEAR_DR //?
#define SSE_ACIA_NO_RESET_PIN // don't reset on warm reset
#define SSE_ACIA_TDR_COPY_DELAY // effect on SR
#define SSE_ACIA_TDR_COPY_DELAY2 // effect on byte flow

#endif


/////////////
// BLITTER //
/////////////

#if defined(SSE_BLITTER)

#define SSE_BLT_BLIT_MODE_CYCLES // #cycles in BLIT Mode in NOPs, not M68K cycles
#define SSE_BLT_BLIT_MODE_INTERRUPT // trigger at once (not after blit phase)
#define SSE_BLT_HOG_MODE_INTERRUPT // no interrupt in hog mode

//#define SSE_BLT_OVERLAP // TODO ?
//#define SSE_BLT_TIMING // based on a table, but Steem does it better
#define SSE_BLT_YCOUNT // 0=65536

#endif


///////////////
// CARTRIDGE //
///////////////

#if defined(SSE_CARTRIDGE)

#define SSE_CARTRIDGE_64KB_OK
#define SSE_CARTRIDGE_DIAGNOSTIC
#define SSE_CARTRIDGE_NO_CRASH_ON_WRONG_FILE
#define SSE_CARTRIDGE_NO_EXTRA_BYTES_OK

#endif


/////////
// CPU //
/////////

#if defined(SSE_CPU)

#define SSE_CPU_ALT_REG_NAMES  // convenience
#define SSE_CPU_DIV          // divide like Caesar
#define SSE_CPU_EXCEPTION    // crash like Windows 98
#define SSE_CPU_FETCH_IO     // fetch like a dog in outer space
#define SSE_CPU_INLINE       // supposes TM68000 exists!
#define SSE_CPU_MOVE_B       // move like a superstar
#define SSE_CPU_MOVE_W       
#define SSE_CPU_MOVE_L
#define SSE_CPU_MOVEM_MR_L//v3.6.0
#define SSE_CPU_POKE         // poke like a C64 
#define SSE_CPU_PREFETCH     // prefetch like a dog
#define SSE_CPU_ROUNDING     // round like a rolling stone
#define SSE_CPU_TAS          // 4 cycles fewer if memory
#define SSE_CPU_UNSTOP//v3.6.0


#if defined(SSE_CPU_DIV)
#define SSE_CPU_DIVS_OVERFLOW//v3.6.1
#define SSE_CPU_DIVU_OVERFLOW//v3.6.1
#endif

#if defined(SSE_CPU_EXCEPTION)

#define SSE_CPU_ASSERT_ILLEGAL // assert before trying to execute (not general)
//#define SSE_CPU_EXCEPTION_TRACE_PC // reporting all PC (!)
#if defined(SSE_BETA) && defined(SSE_DEBUG)
//#define SSE_CPU_DETECT_STACK_PC_USE // push garbage!!
#endif

#if defined(SSE_HACKS) // hacks on: we don't use 'true PC', but this is undef further
#define SSE_CPU_HACK_BLOOD_MONEY
//#define SSE_CPU_HACK_PHALEON // TODO: why does it work with or without?
#define SSE_CPU_HACK_WAR_HELI
#endif

//#define SSE_CPU_HIMEM_BONUS_BYTES // for F-29: bad idea
#define SSE_CPU_IGNORE_RW_4MB // for F-29, works but... //v3.6.0
#define SSE_CPU_IGNORE_WRITE_0 // for Aladin, may write on 1st byte
#define SSE_CPU_POST_INC // no post increment if exception 
#define SSE_CPU_PRE_DEC // no "pre" decrement if exception!
#define SSE_CPU_SET_DEST_TO_0 // for Aladin

// PC as pushed in case of bus error, based on microcodes
#define SSE_CPU_TRUE_PC 
#define SSE_CPU_TRUE_PC_AND_NO_HACKS // option 'Hacks' or not: true PC

#endif//exception

#if defined(SSE_CPU_FETCH_IO)
#define SSE_CPU_FETCH_IO_FULL // need all or nothing for: Union Demo!
#endif

#if defined(SSE_CPU_INLINE) //todo
#define SSE_CPU_INLINE_PREFETCH_SET_PC
#define SSE_CPU_INLINE_READ_BWL
#define SSE_CPU_INLINE_READ_FROM_ADDR
#endif//SSE_CPU_INLINE



#if defined(SSE_CPU_POKE)
//#define SSE_CPU_3615GEN4_ULM //targeted for 36.15 GEN4 by ULM
#define SSE_CPU_CHECK_VIDEO_RAM_B
#define SSE_CPU_CHECK_VIDEO_RAM_L
#define SSE_CPU_CHECK_VIDEO_RAM_W // including: 36.15 GEN4 by ULM
#endif //poke

#if defined(SSE_CPU_PREFETCH)

#if defined(SSE_BETA) && defined(SSE_DEBUG)
//#define SSE_CPU_NO_PREFETCH // fall for all prefetch tricks (debug)
#endif


// Change no timing, just the macro used, so that we can identify what timings
// are for prefetch:
#define SSE_CPU_FETCH_TIMING  // TODO can't isolate

// Move the timing counting from FETCH_TIMING to PREFETCH_IRC:
#define SSE_CPU_PREFETCH_TIMING //big, big change
#ifdef SSE_CPU_PREFETCH_TIMING 
//#define SSE_CPU_PREFETCH_TIMING_CMPI_B // move the instruction timing place-no
//#define SSE_CPU_PREFETCH_TIMING_CMPI_W // move the instruction timing place
#define SSE_CPU_PREFETCH_TIMING_CMPI_L // move the instruction timing place
#define SSE_CPU_PREFETCH_TIMING_MOVEM // at wrong place, probably compensates bug
#define SSE_CPU_PREFETCH_TIMING_SET_PC // necessary for some SET PC cases
//#define SSE_CPU_PREFETCH_TIMING_EXCEPT // to mix unique switch + lines
#endif
#if !defined(SSE_CPU_PREFETCH_TIMING) || defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
#define CORRECTING_PREFETCH_TIMING 
#endif
#ifdef CORRECTING_PREFETCH_TIMING
// powerful prefetch debugging switches

#define SSE_CPU_LINE_0_TIMINGS // 0000 Bit Manipulation/MOVEP/Immediate
#define SSE_CPU_LINE_1_TIMINGS // 0001 Move Byte
#define SSE_CPU_LINE_2_TIMINGS // 0010 Move Long
#define SSE_CPU_LINE_3_TIMINGS // 0011 Move Word
#define SSE_CPU_LINE_4_TIMINGS // 0100 Miscellaneous
#define SSE_CPU_LINE_5_TIMINGS // 0101 ADDQ/SUBQ/Scc/DBcc/TRAPcc
#define SSE_CPU_LINE_6_TIMINGS // 0110 Bcc/BSR/BRA
#define SSE_CPU_LINE_7_TIMINGS // 0111 MOVEQ
#define SSE_CPU_LINE_8_TIMINGS // 1000 OR/DIV/SBCD
#define SSE_CPU_LINE_9_TIMINGS // 1001 SUB/SUBX
#define SSE_CPU_LINE_A_TIMINGS // 1010 (Unassigned, Reserved)
#define SSE_CPU_LINE_B_TIMINGS // 1011 CMP/EOR
#define SSE_CPU_LINE_C_TIMINGS // 1100 AND/MUL/ABCD/EXG
#define SSE_CPU_LINE_D_TIMINGS // 1101 ADD/ADDX
#define SSE_CPU_LINE_E_TIMINGS // 1110 Shift/Rotate/Bit Field
#define SSE_CPU_LINE_F_TIMINGS // 1111 Coprocessor Interface/MC68040 and CPU32 Extensions

#endif

// all those switches because we had some nasty bug... 

#define SSE_CPU_PREFETCH_ASSERT
#define SSE_CPU_PREFETCH_CALL
#define SSE_CPU_PREFETCH_CLASS
#define SSE_CPU_PREFETCH_BSET
#define SSE_CPU_PREFETCH_CHK
#define SSE_CPU_PREFETCH_JSR
#define SSE_CPU_PREFETCH_MOVE_FROM_SR
//#define SSE_CPU_PREFETCH_MULS //not used (TODO?)
//#define SSE_CPU_PREFETCH_MULU
#define SSE_CPU_PREFETCH_NOP
#define SSE_CPU_PREFETCH_PEA

#define SSE_CPU_PREFETCH_4PIXEL_RASTERS //no hack, just disabling switch

#if defined(SSE_CPU_PREFETCH_TIMING) || defined(CORRECTING_PREFETCH_TIMING)
#define SSE_CPU_PREFETCH_MOVE_MEM // this was forgotten in v3.5.0! limited impact
// Additional fixes based on Yacht, each one protected (3.5.0 seems stable)
// if it can change anything (TODO)
#define SSE_CPU_YACHT_TAS // confirming what Steem authors suspected
#endif

#if !defined(SSE_OSD_CONTROL)
#define SSE_CPU_PREFETCH_TRACE 
#define SSE_CPU_TRACE_DETECT //was in prefetch define zone BTW
#endif

#endif//prefetch

#if defined(SSE_CPU_ROUNDING)

#define SSE_CPU_ROUNDING_ADD_L // EA = -(An) //v3.6.0
#define SSE_CPU_ROUNDING_ADDA_L // EA = -(An) //v3.6.0

//#define SSE_CPU_ROUNDING_SOURCE_100_B // -(An)
//#define SSE_CPU_ROUNDING_SOURCE_100_W // -(An): no
//#define SSE_CPU_ROUNDING_SOURCE_100_L // -(An): no!

#endif//rounding

#define SSE_CPU_LINE_F // for interrupt depth counter
#endif


///////////
// DEBUG //
///////////

#if defined(SSE_DEBUG)

#define SSE_DEBUG_TRACE

#ifdef SSE_DEBUG_TRACE
#ifdef _DEBUG // VC
#define SSE_DEBUG_TRACE_IDE
#endif
#if defined(DEBUG_BUILD) 
#define SSE_DEBUG_TRACE_FILE
#else//VC
//#define SSE_DEBUG_TRACE_FILE
#endif
#endif


// boiler + IDE

#define SSE_DEBUG_FRAME_REPORT

#if defined(SSE_DEBUG_FRAME_REPORT)

#define SSE_DEBUG_FRAME_REPORT_ON_STOP // each time we stop emulation

#if defined(DEBUG_BUILD)
#define SSE_DEBUG_FRAME_REPORT_MASK // for interactive control in boiler//3.6.1
#endif

#if !defined(SSE_DEBUG_FRAME_REPORT_MASK) // control by define
#define SSE_DEBUG_FRAME_REPORT_ACIA
#define SSE_DEBUG_FRAME_REPORT_BLITTER
#define SSE_DEBUG_FRAME_REPORT_PAL
#define SSE_DEBUG_FRAME_REPORT_HSCROLL // & linewid
#define SSE_DEBUG_FRAME_REPORT_SDP_LINES
#define SSE_DEBUG_FRAME_REPORT_SDP_READ
#define SSE_DEBUG_FRAME_REPORT_SDP_WRITE
#define SSE_DEBUG_FRAME_REPORT_SHIFTER_TRICKS
#define SSE_DEBUG_FRAME_REPORT_SHIFTER_TRICKS_BYTES
#define SSE_DEBUG_FRAME_REPORT_SHIFTMODE
#define SSE_DEBUG_FRAME_REPORT_SYNCMODE
#define SSE_DEBUG_FRAME_REPORT_VIDEOBASE

///#define SSE_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN//no, sthg else!
#endif

#endif//frame report

#define SSE_DEBUG_REPORT_SCAN_Y_ON_CLICK
#define SSE_DEBUG_REPORT_SDP // tracking sdp at start of each scanline
#define SSE_DEBUG_REPORT_SDP_ON_CLICK // yeah!

// boiler
#if defined(DEBUG_BUILD) //TODO add other mods here
#define SSE_DEBUG_BLAZING_STEP_OVER 
#define SSE_DEBUG_BROWSER_6301
#define SSE_DEBUG_BROWSER_ACIA
#define SSE_DEBUG_BROWSER_BLITTER
#define SSE_DEBUG_BROWSER_DMASOUND
#define SSE_DEBUG_BROWSER_INSTRUCTIONS //window name
#define SSE_DEBUG_BROWSER_PSEUDO_IO_SCROLL // for the bigger 6301 browser
#define SSE_DEBUG_BROWSER_SHIFTER
#define SSE_DEBUG_BROWSER_VECTORS
#define SSE_DEBUG_CLIPBOARD // right-click on 'dump' to copy then paste
//#define SSE_DEBUG_CPU_LOG_NO_STOP // never stop
#define SSE_DEBUG_CPU_TRACE_NO_STOP // depends on 'suspend logging'
#define SSE_DEBUG_DUMP_6301_RAM
#define SSE_DEBUG_FAKE_IO //3.6.1 to control some debug options
#if defined(SSE_DEBUG_FAKE_IO)
#define SSE_DEBUG_FRAME_INTERRUPTS//3.6.1 OSD, handy
#define SSE_DEBUG_TRACE_CONTROL //3.6.1 beyond log options
#endif
#define SSE_DEBUG_MOD_REGS // big letters, no =
#define SSE_DEBUG_MONITOR_IO_FIX1 // ? word check, not 2x byte on word access
#define SSE_DEBUG_MONITOR_RANGE // will stop for every address between 2 stops
#define SSE_DEBUG_MONITOR_VALUE // specify value (RW) that triggers stop
#define SSE_DEBUG_MONITOR_VALUE2 // write before check
#define SSE_DEBUG_MONITOR_VALUE3 // add checks for CLR
#define SSE_DEBUG_MONITOR_VALUE4 // 3.6.1 corrections (W on .L)
#define SSE_DEBUG_MOUSE_WHEEL // yeah!
#define SSE_DEBUG_MOVE_OTHER_SP
#define SSE_DEBUG_MOVE_OTHER_SP2//3.6.1, SSP+USP
#if defined(SSE_DEBUG_FAKE_IO)
#define SSE_DEBUG_MUTE_SOUNDCHANNELS //fake io, 3.6.1
#endif
#define SSE_DEBUG_NODIV // no DIV log necessary anymore
#define SSE_DEBUG_NO_SOUND_DAMPING //PSG filter control 'd' and 'a'
#define SSE_DEBUG_RUN_TO_RTS
//#define SSE_DEBUG_SHOW_ACT //useful?
//#define SSE_DEBUG_SHOW_FRAME //useful?
#define SSE_DEBUG_SHOW_FREQ // sync mode
#define SSE_DEBUG_SHOW_INTERRUPT // yeah!
#define SSE_DEBUG_SHOW_RES // shift mode
#define SSE_DEBUG_SHOW_SDP // the draw pointer
#define SSE_DEBUG_SHOW_SR // in HEX on the left of bit flags
#define SSE_DEBUG_SSE_PERSISTENT // L/S options 
#define SSE_DEBUG_STACK_68030_FRAME //request, to check compatibility
#define SSE_DEBUG_STACK_CHOICE//3.6.1
#define SSE_DEBUG_TIMER_B // instead of 0
#define SSE_DEBUG_TIMERS_ACTIVE // (in reverse video) yeah!
#if defined(SSE_DEBUG_FAKE_IO)
#define SSE_DEBUG_VIDEO_CONTROL //3.6.1
#endif
#define SSE_DEBUG_WIPE_TRACE // as logfile
#endif

#define SSE_DEBUG_LOG_OPTIONS // mine

#if defined(SSE_DEBUG) && !defined(BCC_BUILD) && !defined(_DEBUG) 
// supposedly we're building the release boiler, make sure features are in

#define SSE_DEBUG_TRACE_IO

#if !defined(SSE_UNIX)
#define SSE_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif

#define SSE_DEBUG_START_STOP_INFO

#define SSE_IPF_TRACE_SECTORS // show sector info (IPF)

#define SSE_IKBD_6301_DUMP_RAM
//#define SSE_IKBD_6301_DUMP_RAM_ON_LS//no
#define SSE_IKBD_6301_TRACE 
#define SSE_IKBD_6301_TRACE_SCI_RX
#define SSE_IKBD_6301_TRACE_SCI_TX
#define SSE_IKBD_6301_TRACE_KEYS

#else // for custom debugging

#define SSE_DEBUG_START_STOP_INFO
#define SSE_DEBUG_TRACE_IO

#define SSE_IKBD_6301_DUMP_RAM
#define SSE_IKBD_6301_TRACE 
#define SSE_IKBD_6301_TRACE_SCI_RX
#define SSE_IKBD_6301_TRACE_SCI_TX
#define SSE_IKBD_6301_TRACE_KEYS

//#define SSE_DEBUG_TRACE_CPU_ROUNDING
//#define SSE_DEBUG_TRACE_PREFETCH
#define SSE_DEBUG_TRACE_SDP_READ_IR

#endif//#if defined(SSE_DEBUG_NO_TRACE)

#else // no SSE_DEBUG

#endif


////////////
// FLOPPY //
////////////

#if defined(SSE_FLOPPY)

#define SSE_DMA        // Custom Direct Memory Access chip (disk)
#define SSE_DISK       // Disk images 3.6.1
#define SSE_DRIVE      // SF314 floppy disk drive
#define SSE_FDC        // WD1772 floppy disk controller
#if SSE_VERSION>=370
#define SSE_FLOPPY_EVENT//TODO
#endif
#if defined(WIN32)
#define SSE_IPF        // CAPS support (IPF disk images) 
#define SSE_PASTI      // Improvements in Pasti support (STX disk images)
#ifdef SSE_BETA
#define SSE_SCP //TODO
#endif
#endif
#define SS_PSG        // YM2149 - for portA first


//////////////////
// FLOPPY: DISK //
//////////////////

//TODO move much SSE_DRIVE code here

#if defined(SSE_DISK)

#if SSE_VERSION>=370

//imagetype needed for ghost & stw
#define SSE_DISK_GHOST //3.7.0
#define SSE_DISK_IMAGETYPE //3.7.0
#define SSE_DISK_STW // 3.7.0

#if defined(SSE_DISK_GHOST)
#define SSE_DISK_GHOST_SECTOR // intercept write 1 sector
#define SSE_DISK_GHOST_SECTOR_STX1 // in pasti
#endif

#if defined(SSE_DISK_STW)
#define SSE_DISK_STW_DISK_MANAGER //new context option
#define SSE_DISK_STW_DISK_MANAGER1 // no custom
#endif

#endif//v3.7.0 

#endif


/////////////////
// FLOPPY: DMA //
/////////////////

#if defined(SSE_DMA) // this is the DMA as used for disk operation

//#define SSE_DMA_ADDRESS // enforcing order for write (no use?)
#define SSE_DMA_ADDRESS_EVEN

#if SSE_VERSION>=370
#define SSE_DMA_DOUBLE_FIFO // works but overkill  //3.7.0 define then remove def?
#endif

#if SSE_VERSION>=370
#define SSE_DMA_DRQ
#endif

//#define SSE_DMA_DELAY // works but overkill 3.7.0 -> use generic floppy event?
#define SSE_DMA_COUNT_CYCLES
#define SSE_DMA_FDC_ACCESS
//#define SSE_DMA_FDC_READ_HIGH_BYTE // like pasti, 0  
#define SSE_DMA_FIFO // first made for CAPS 
#define SSE_DMA_FIFO_NATIVE // then extended for Steem native (not R/W tracks)
#define SSE_DMA_FIFO_PASTI // and Pasti
#define SSE_DMA_FIFO_READ_ADDRESS // save some bytes...
#define SSE_DMA_FIFO_READ_ADDRESS2 // save 4 bytes more...
#define SSE_DMA_IO
#define SSE_DMA_READ_STATUS 
#define SSE_DMA_SECTOR_COUNT
#define SSE_DMA_WRITE_CONTROL
///#ifdef SSE_DEBUG //will be useful for release version too...
#define SSE_DMA_TRACK_TRANSFER //add one var...
#///endif
#endif


///////////////////
// FLOPPY: DRIVE //
///////////////////


#if defined(SSE_DRIVE)

#define SSE_DRIVE_BYTES_PER_ROTATION
#ifdef SSE_DEBUG
#define SSE_DRIVE_COMPUTE_BOOT_CHECKSUM // to mod a boot sector//v3.6.0
#endif
#define SSE_DRIVE_CREATE_ST_DISK_FIX // from Petari//v3.6.0
// one or the other:
//#define SSE_DRIVE_EMPTY_VERIFY_LONG // GEM
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT //GEM
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT2 //3.6.1 motor led still on
#define SSE_DRIVE_MOTOR_ON
#define SSE_DRIVE_MOTOR_ON_IPF//3.6.1 TODO
#define SSE_DRIVE_MULTIPLE_SECTORS
#define SSE_DRIVE_READ_ADDRESS_TIMING
#define SSE_DRIVE_11_SECTORS //TODO acopy13A?
#define SSE_DRIVE_READ_TRACK_11
#define SSE_DRIVE_READ_TRACK_11B //Gap 4: 1
#define SSE_DRIVE_READ_TRACK_11C //Gap 5
#define SSE_DRIVE_RW_SECTOR_TIMING // start of sector
#define SSE_DRIVE_RW_SECTOR_TIMING2 // end of sector (hack)

#if SSE_VERSION>=370
#define SSE_DRIVE_RW_SECTOR_TIMING3 //test v3.7.0 use ID... (?)
#endif

#define SSE_DRIVE_SINGLE_SIDE //3.6.0 
#if SSE_VERSION>=370
#define SSE_DRIVE_SINGLE_SIDE_RND//3.7.0//TODO random data instead
#else
#define SSE_DRIVE_SINGLE_SIDE_IPF //3.6.0
#define SSE_DRIVE_SINGLE_SIDE_NAT1 //3.6.0
#define SSE_DRIVE_SINGLE_SIDE_PASTI //3.6.0
#endif
#if defined(WIN32) //TODO Unix
#define SSE_DRIVE_SOUND // heavily requested, delivered!//3.6.0
//#define SSE_DRIVE_SOUND_CHECK_SEEK_VBL
#if defined(SSE_DRIVE_SOUND)
#define SSE_DRIVE_SOUND_SINGLE_SET // drive B uses sounds of A
//#define SSE_DRIVE_SOUND_EDIT // 1st beta soundset
#define SSE_DRIVE_SOUND_EPSON // current samples=Epson
#define SSE_DRIVE_SOUND_EMPTY // different 3.6.1 (none)
#define SSE_DRIVE_SOUND_IPF // fix 3.6.1
#define SSE_DRIVE_SOUND_VOLUME // logarithmic 
#endif//drive sound
#endif//win32
#define SSE_DRIVE_SPIN_UP_TIME
#define SSE_DRIVE_SPIN_UP_TIME2 // more precise
#define SSE_DRIVE_SWITCH_OFF_MOTOR //hack//3.6.0
//#define SSE_DRIVE_WRITE_TRACK_11//TODO
#define SSE_DRIVE_WRONG_IMAGE_ALERT//3.6.1
#endif


/////////////////
// FLOPPY: FDC //
/////////////////

#if defined(SSE_FDC)

#define SSE_FDC_ACCURATE // bug fixes and additions for ADAT mode (v3.5)

#if defined(SSE_FDC_ACCURATE) 

#define SSE_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari
#define SSE_FDC_CHANGE_SECTOR_WHILE_BUSY // from Hatari or Kryoflux
#define SSE_FDC_CHANGE_TRACK_WHILE_BUSY // from Hatari or Kryoflux
#define SSE_FDC_FORCE_INTERRUPT // Panzer rotation using $D4
#define SSE_FDC_FORCE_INTERRUPT_RESET_D4 // not clear
#define SSE_FDC_HEAD_SETTLE
#if SSE_VERSION>=370
#define SSE_FDC_IDFIELD_IGNORE_SIDE 
#endif
#define SSE_FDC_INDEX_PULSE1 // 4ms
#define SSE_FDC_INDEX_PULSE2 // read STR
#define SSE_FDC_MOTOR_OFF 
#define SSE_FDC_PRECISE_HBL
#define SSE_FDC_READ_ADDRESS_UPDATE_SR
#define SSE_FDC_RESET
#define SSE_FDC_RESTORE
#define SSE_FDC_SEEK
#define SSE_FDC_SPIN_UP_AGENDA
#define SSE_FDC_SPIN_UP_STATUS
#define SSE_FDC_STEP

#ifdef SS_PSG
#define SSE_FDC_IGNORE_WHEN_NO_DRIVE_SELETED // from Hatari
#define SSE_FDC_INDEX_PULSE_COUNTER
#ifdef SSE_FDC_MOTOR_OFF
#define SSE_FDC_MOTOR_OFF_COUNT_IP
#endif
#endif

#ifdef SSE_DRIVE
#define SSE_FDC_VERIFY_AGENDA 
#endif

#endif//SSE_FDC_ACCURATE

#ifdef SSE_DEBUG
#define SSE_FDC_TRACE_IRQ
#define SSE_FDC_TRACE_STATUS //spell out status register
#endif

#endif


/////////////////
// FLOPPY: IPF //
/////////////////

#if defined(SSE_IPF)
#ifdef SSE_DRIVE
#define SSE_DRIVE_IPF1 // 3.6.1 know image type (not used yet TODO)
#endif

// those switches were used for development, normally they ain't necessary
//#define SSE_IPF_CPU // total sync, works but overkill
//#define SSE_IPF_RUN_PRE_IO 
//#define SSE_IPF_RUN_POST_IO
#define SSE_IPF_ASSOCIATE // extension may be associated with Steem
#define SSE_IPF_CTRAW//2nd type of file recognised by caps v5.0 (?) //3.6.1
#define SSE_IPF_CTRAW_REV //3.6.1 manage different rev data
//#define SSE_IPF_KFSTREAM//3rd type of file recognised by caps v5.1 (?) //3.6.1
//#define SSE_IPF_DRAFT//4th type of file recognised by caps v5.1 (?) //3.6.1
//#define SSE_IPF_OSD // for the little box at reset - silly?
#define SSE_IPF_RESUME//3.6.1 TODO
//#define SSE_IPF_SAVE_WRITES //TODO?
#define SSE_IPF_TRACE_SECTORS // show sector info

#endif


///////////////////
// FLOPPY: PASTI //
///////////////////

#if defined(SSE_PASTI) && defined(WIN32)

#define SSE_PASTI_ALWAYS_DISPLAY_STX_DISKS
#define SSE_PASTI_AUTO_SWITCH
#define SSE_PASTI_ONLY_STX  // experimental! optional
#define SSE_PASTI_ONLY_STX_HD //v3.6.0
#define SSE_PASTI_NO_RESET 
#define SSE_PASTI_ON_WARNING // mention in disk manager title
#define SSE_PASTI_ON_WARNING2 // v3.6.1 refactoring

#endif


/////////////////
// FLOPPY: PSG //
/////////////////

#if defined(SS_PSG)

#if SSE_VERSION>=370
#define SS_PSG1 // selected drive, side as variables
#define SS_PSG2 // adpat drive motor status to FDC STR at each change
#endif

#ifdef SSE_DEBUG
#define SS_PSG_REPORT_DRIVE_CHANGE // as FDC trace
#endif

#endif


/////////////////
// FLOPPY: SCP //
/////////////////

#if defined(SSE_SCP)

#endif


#endif//SSE_FLOPPY


//////////
// GLUE //
////////// 





//////////
// IKBD //
//////////

#if defined(SSE_IKBD)

#define SSE_IKBD_6301 // HD6301 true emu, my pride!

#define SSE_IKBD_TRACE_CPU_READ

#define SSE_IKBD_TRACE_CPU_READ2 //beware polling


#if defined(SSE_IKBD_6301) 
// Switches for Sim6301 (modded 3rd party) are here too
#define SSE_IKBD_6301_ADJUST_CYCLES // stay in sync! (check with clock)
#define SSE_IKBD_6301_CHECK_COMMANDS
#define SSE_IKBD_6301_CHECK_IREG_RO // some registers are read-only
#define SSE_IKBD_6301_CHECK_IREG_WO // some registers are write-only
#define SSE_IKBD_6301_DISABLE_BREAKS // to save 64k (we still consume 64k)
#define SSE_IKBD_6301_DISABLE_CALLSTACK // to save 3k on the PC stack
//#define SSE_IKBD_6301_RUN_CYCLES_AT_IO // overkill
#define SSE_IKBD_6301_MOUSE_ADJUST_SPEED //poor attempt
#define SSE_IKBD_6301_MOUSE_MASK // Jumping Jackson auto//v3.6.0
//#define SSE_IKBD_MOUSE_OFF_JOYSTICK_EVENT // hardware quirk? (hack)
#define SSE_IKBD_6301_RUN_IRQ_TO_END // hack around Sim6xxx's working
#define SSE_IKBD_6301_SET_TDRE
#define SSE_IKBD_6301_TIMER_FIX // not sure there was a problem
//#define SSE_IKBD_6301_TRACE // defined in SSE_DEBUG
#if defined(SSE_IKBD_6301_TRACE)
#ifdef SSE_BETA
//#define SSE_IKBD_6301_DISASSEMBLE_ROM 
//#define SSE_IKBD_6301_DUMP_RAM
#endif
//#define SSE_IKBD_6301_TRACE_SCI_RX
//#define SSE_IKBD_6301_TRACE_SCI_TX
//#define SSE_IKBD_6301_TRACE_INT_TIMER
//#define SSE_IKBD_6301_TRACE_INT_SCI
//#define SSE_IKBD_6301_TRACE_STATUS
//#define SSE_IKBD_6301_TRACE_KEYS
//#define SSE_IKBD_6301_TRACE_WRITES
#endif
#endif//#if defined(SSE_IKBD_6301)

#endif


///////////////
// INTERRUPT //
///////////////
// temp there ought to be a better structure
#if defined(SSE_INTERRUPT)

#define SSE_INT_HBL 
#define SSE_INT_JITTER // from Hatari 
#define SSE_INT_VBL 


#if defined(SSE_INT_HBL)

#if !defined(SSE_DEBUG_TRACE_CONTROL)
#define SSE_INT_OSD_REPORT_HBI
#endif

#define SSE_INT_HBL_IACK_FIX // from Hatari - BBC52
#define SSE_INT_HBL_ONE_FUNCTION // 3.6.1 remove event_hbl()
#endif

#if defined(SSE_INT_JITTER) && defined(SSE_INT_HBL)
#define SSE_INT_JITTER_HBL //3.6.0 defined again for 3615HMD
#endif

#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_IACK
#define SSE_INT_VBL_INLINE 
#ifdef SSE_BETA
//#define SSE_INT_VBI_START // generally working now
#endif
#endif

#if defined(SSE_INT_JITTER) && defined(SSE_INT_VBL) && defined(SSE_STF)
#define SSE_INT_JITTER_RESET//3.6.1
#define SSE_INT_JITTER_VBL // STF
#define SSE_INT_JITTER_VBL2 //3.6.1
//#define SSE_INT_JITTER_VBL_STE // STF + STE 
#endif

#if defined(SSE_STF) && defined(SSE_INT_VBL) && !defined(SSE_INT_VBI_START)
#define SSE_INT_VBL_STF // more a hack but works
#endif

#endif


/////////
// MFP //
/////////

#if defined(SSE_MFP)

#define SSE_MFP_IACK_LATENCY
//#define SSE_MFP_IRQ_DELAY // V8MS but breaks Sinfull Sinuses
//#define SSE_MFP_IRQ_DELAY2 // Audio Artistic Demo (no!)
//#define SSE_MFP_IRQ_DELAY3 // V8MS from Hatari 3.6.1:undef
#define SSE_MFP_PATCH_TIMER_D // from Hatari, we keep it for performance
#define SSE_MFP_RATIO // change the values of CPU & MFP freq!
#define SSE_MFP_RATIO_HIGH_SPEED //fix v3.6.1
#define SSE_MFP_RATIO_PRECISION // for short timers
#define SSE_MFP_RATIO_STE // measured (by Steem Authors) for STE?
#define SSE_MFP_RATIO_STF // theoretical for STF
#define SSE_MFP_RATIO_STE_AS_STF // change STF + STE (same for both)
#define SSE_MFP_RS232 //one little anti-hang bugfix
#define SSE_MFP_TIMER_B 
#if defined(SSE_MFP_TIMER_B)
#define SSE_MFP_TIMER_B_AER // earlier trigger (from Hatari)
#define SSE_MFP_TIMER_B_NO_WOBBLE // does it fix anything???
#define SSE_MFP_TIMER_B_RECOMPUTE //3.6.1
#endif
#define SSE_MFP_TIMERS_BASETIME // "vbl" instead of "mfptick"
#define SSE_MFP_TxDR_RESET // they're not reset according to doc
#define SSE_MFP_WRITE_DELAY1//3.6.1 (Audio Artistic)

#endif


//////////
// MIDI //
//////////

#if defined(SSE_MIDI)

//#define SSE_MIDI_TRACE_BYTES_IN
//#define SSE_MIDI_TRACE_BYTES_OUT

#endif


/////////
// MMU //
/////////

#if defined(SSE_MMU)

//TODO, properly separate MMU, GLUE, Shifter functions, first proper C++ structure

#if defined(WIN32) //TODO Unix
#define SSE_MMU_256K // Atari 260 ST//v3.6.0
#define SSE_MMU_2560K // some STE with 2MB memory upgrade//v3.6.0
#endif

//#define SSE_MMU_NO_CONFUSION // Diagnostic cartridge: don't define (v3.5.2)
#ifdef SSE_STF
#define SSE_MMU_WAKE_UP
#endif
#define SSE_MMU_WRITE // programs in RAM may write in the MMU

#if defined(SSE_MMU_WAKE_UP)
#if defined(SSE_CPU)
//#define SSE_MMU_WAIT_STATES // extreme, replaces rounding to 4, TODO
#endif
#define SSE_MMU_WAKE_UP_DL // Dio's DE-LOAD delay
#if !defined(SSE_MMU_WAKE_UP_DL)
#define SSE_MMU_WAKE_UP_0_BYTE_LINE
//#define SSE_MMU_WAKE_UP_IO_BYTES_R  // breaks too much (read SDP) TODO
//#define SSE_MMU_WAKE_UP_IO_BYTES_W // no too radical
//#define SSE_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY // adapt cycles for shifter write
#define SSE_MMU_WAKE_UP_IOR_HACK 
#define SSE_MMU_WAKE_UP_IOW_HACK 
#endif

#define SSE_MMU_WAKE_UP_PALETTE_STE // render +1 cycle (pixel) in state 2
//#define SSE_MMU_WAKE_UP_READ_SDP
#define SSE_MMU_WAKE_UP_RESET_ON_SWITCH_ST
#define SSE_MMU_WAKE_UP_RIGHT_BORDER
#define SSE_MMU_WAKE_UP_SHIFTER_TRICKS // Adapt limit values based on Paolo's table
//#define SSE_MMU_WAKE_UP_STE_STATE2 // STE in same state2 as STF (no)
#define SSE_MMU_WAKE_UP_VERTICAL_OVERSCAN // ijor's wakeup.tos test
#define SSE_MMU_WAKE_UP_VERTICAL_OVERSCAN1 //3.6.3 defaults to WU1
//#define SSE_MMU_WAKE_UP_WRITE_SDP
#endif// SSE_MMU_WAKE_UP


#endif


/////////
// OSD //
/////////

#if defined(SSE_OSD)

#ifdef SSE_DEBUG
#define SSE_OSD_DEBUG_MESSAGE // pretty handy

#if defined(SSE_DEBUG_FAKE_IO)
#define SSE_OSD_CONTROL //3.6.1
#else
#define SSE_OSD_DEBUG_MESSAGE_FREQ // tell when 60hz (?)
#endif//fakeio
#endif//debug
#define SSE_OSD_DRIVE_INFO // cool! (v3.5.1)
#define SSE_OSD_DRIVE_INFO2 // no SR when fast
//#define SSE_OSD_DRIVE_INFO_OSD_PAGE
#define SSE_OSD_DRIVE_INFO_SSE_PAGE
#define SSE_OSD_DRIVE_LED
#define SSE_OSD_DRIVE_LED2 // simpler approach
#define SSE_OSD_DRIVE_LED3 // remove useless variable
//#define SSE_OSD_LOGO1 // suppress former logo (=3.2 in grx)
//#define SSE_OSD_LOGO2 //hack (temp)
#define SSE_OSD_LOGO3 // nicer
#define SSE_OSD_SCROLLER_CONTROL
#define SSE_OSD_SCROLLER_DISK_IMAGE //TODO sometimes wrong name

#endif


/////////
// SDL //
/////////

#if defined(SSE_SDL)
// tests - but shouldn't it be a pure SDL build? 
//#if SSE_VERSION<360
#define SSE_SDL_DEACTIVATE // support planned in v3.6 //nope...
//#endif
//#define SSE_SDL_KEEP_DDRAW_RUNNING // normally not!

#endif


/////////////
// SHIFTER //
/////////////

#if defined(SSE_SHIFTER)

#define SSE_SHIFTER_IO // move blocks from ior, iow

///#define SSE_SHIFTER_RENDER_SYNC_CHANGES//don't until debug

#define SSE_SHIFTER_SDP // SDP=shifter draw pointer
#define SSE_SHIFTER_TRICKS  // based on Steem system, extended

#if defined(SSE_SHIFTER_TRICKS)

#define SSE_SHIFTER_0BYTE_LINE
#if defined(SSE_SHIFTER_0BYTE_LINE) // former switch approach
//#define SSE_SHIFTER_0BYTE_LINE_RES_END //No Buddies Land
//#define SSE_SHIFTER_0BYTE_LINE_RES_HBL //Beyond/Pax Plax Parallax
//#define SSE_SHIFTER_0BYTE_LINE_RES_START //Nostalgic-O/Lemmings
//#define SSE_SHIFTER_0BYTE_LINE_SYNC //Forest
//#define SSE_SHIFTER_0BYTE_LINE_SYNC2 // loSTE screens

//#define SSE_SHIFTER_0BYTE_LINE_RES_END_THRESHOLD//Hackabonds Demo not WS1

#endif//0-byte line

#define SSE_SHIFTER_4BIT_SCROLL //Let's do the Twist again
#define SSE_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK
#define SSE_SHIFTER_60HZ_OVERSCAN //Leavin' Terramis
#define SSE_SHIFTER_END_OF_LINE_CORRECTION // correct +2, -2 lines 
#define SSE_SHIFTER_FIX_LINE508_CONFUSION // hack at recording shifter event

//#define SSE_SHIFTER_LEFT_OFF_THRESHOLD//Hackabonds Demo not WS1

#define SSE_SHIFTER_LEFT_OFF_60HZ // 24 bytes!
#define SSE_SHIFTER_LEFT_OFF_TEST_BEFORE_HBL // for Riverside
#define SSE_SHIFTER_LINE_MINUS_106_BLACK // loSTE screens
#define SSE_SHIFTER_LINE_PLUS_2_STE // hack?
#define SSE_SHIFTER_LINE_PLUS_2_TEST // loSTE screens
#define SSE_SHIFTER_LINE_PLUS_2_THRESHOLD //Forest
//#define SSE_SHIFTER_LINE_PLUS_2_POST_TOP_OFF // Panic
#define SSE_SHIFTER_LINE_PLUS_20 // 224 byte scanline STE only
#define SSE_SHIFTER_LINE_PLUS_20_SHIFT // for Riverside
#define SSE_SHIFTER_MED_OVERSCAN // BPOC
#define SSE_SHIFTER_MED_OVERSCAN_SHIFT // No Cooper/greetings
#define SSE_SHIFTER_NON_STOPPING_LINE // Enchanted Land
//#define SSE_SHIFTER_PALETTE_BYTE_CHANGE //Golden Soundtracker
#define SSE_SHIFTER_PALETTE_NOISE //UMD8730 STF
#define SSE_SHIFTER_PALETTE_TIMING //Overscan Demos #6
#define SSE_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE //beeshift0
#define SSE_SHIFTER_STATE_MACHINE //v3.5.4, simpler approach and WS-aware
#define SSE_SHIFTER_STE_MED_HSCROLL // Cool STE
#define SSE_SHIFTER_STE_HI_HSCROLL
#define SSE_SHIFTER_STE_HSCROLL_LEFT_OFF //MOLZ/Spiral
#define SSE_SHIFTER_STE_VERTICAL_OVERSCAN //RGBeast
#define SSE_SHIFTER_UNSTABLE // DoLB, Omega, Overdrive/Dragon, Beeshift
#ifdef SSE_SHIFTER_UNSTABLE
//TODO swtiches for Dragon, etc.
#if SSE_VERSION>353
//#define SSE_SHIFTER_LINE_PLUS_2_ON_PRELOAD3 // DSOS STE //MFD?
#endif
#define SSE_SHIFTER_UNSTABLE_DOLB
#define SSE_SHIFTER_UNSTABLE_OMEGA
#define SSE_SHIFTER_HI_RES_SCROLLING // Beeshift2
#define SSE_SHIFTER_MED_RES_SCROLLING // Beeshift
#define SSE_SHIFTER_PANIC // funny effect, interleaved border bands
#define SSE_SHIFTER_REMOVE_USELESS_VAR //6.3.1
#define SSE_SHIFTER_VERTICAL_OPTIM1 //avoid useless tests
#endif
//#define SSE_SHIFTER_UNSTABLE_LEFT_OFF // DoLB, Omega, Overdrive/Dragon old hack

#if defined(SSE_STF)
#define SSE_STF_VERTICAL_OVERSCAN
#endif

#endif//#if defined(SSE_SHIFTER_TRICKS)

#if defined(SSE_SHIFTER_SDP)
#define SSE_SHIFTER_SDP_READ
#define SSE_SHIFTER_SDP_WRITE
#define SSE_SHIFTER_SDP_WRITE_ADD_EXTRA
#define SSE_SHIFTER_SDP_WRITE_LOWER_BYTE
//#define SSE_SHIFTER_SDP_TRACE 
//#define SSE_SHIFTER_SDP_TRACE2
//#define SSE_SHIFTER_SDP_TRACE3 // report differences with Steem v3.2 
#endif

#if defined(SSE_HACKS)
// most hacks concern SDP, there's room for improvement
#define SSE_SHIFTER_SDP_WRITE_DE_HSCROLL
#define SSE_SHIFTER_SDP_WRITE_MIDDLE_BYTE // stable
#define SSE_SHIFTER_ARMADA_IS_DEAD // no shift contrary to Big Wobble
#define SSE_SHIFTER_BIG_WOBBLE // Big Wobble shift
#define SSE_SHIFTER_DANGEROUS_FANTAISY // Dangerous Fantaisy credits flicker
#define SSE_SHIFTER_DOLB_SHIFT1 // based on "unstable overscan"
//#define SSE_SHIFTER_DOLB_SHIFT2 // based on cycle of R0
//#define SSE_SHIFTER_DRAGON1 // confused shifter, temp hack
#define SSE_SHIFTER_PACEMAKER // Pacemaker credits flickering line
#define SSE_SHIFTER_SCHNUSDIE // Reality is a Lie/Schnusdie overscan logo
#define SSE_SHIFTER_TCB // Swedish New Year Demo/TCB SDP (60hz)
#define SSE_SHIFTER_TEKILA // Delirious 4/Tekila
#define SSE_SHIFTER_XMAS2004 // XMas 2004 by Paradox shift
#endif

#if defined(SSE_DEBUG) 
#ifdef SSE_BETA
//#define SSE_SHIFTER_DRAW_DBG  // totally bypass CheckSideOverscan() & Render()
#endif


//#define SSE_SHIFTER_IOR_TRACE // specific, not "log"
//#define SSE_SHIFTER_IOW_TRACE // specific, not "log"
#if !defined(SSE_DEBUG_TRACE_IDE)
#define SSE_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif

#if !defined(SSE_DEBUG_TRACE_CONTROL)
//#define SSE_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN
//#define SSE_SHIFTER_VERTICAL_OVERSCAN_TRACE
#endif

//#define SSE_SHIFTER_STEEM_ORIGINAL // only for debugging/separate blocks

#else
#define draw_check_border_removal Shifter.CheckSideOverscan
#define draw_scanline_to(cycles_since_hbl) Shifter.Render(cycles_since_hbl)
#endif

#endif


///////////
// SOUND //
///////////

#if defined(SSE_SOUND)

#ifndef SS_PSG // also used for drive
#define SS_PSG
#endif

#ifdef SS_PSG

#define SS_PSG_FIX_TABLES // based on Yamaha doc//v3.6.0
#if defined(SS_PSG_FIX_TABLES)
#define SS_PSG_FIXED_VOL_FIX1
#define SS_PSG_ENV_FIX1
#endif

#define SS_PSG_FIXED_VOL_FIX2 // from ljbk, measured output//v3.6.0

//#define SS_PSG_WRITE_SAME_VALUE //test
#define SS_PSG_OPT1//v3.6.0
#endif

#define SSE_SOUND_APART_BUFFERS //TODO, one for PSG one for DMA, but Microwire?

#define SSE_SOUND_CHANGE_TIME_METHOD_DELAY //detail

#define SSE_SOUND_FILTER_STF // a very simple filter

#define SSE_SOUND_INLINE // macro->inline, easier for my tests, but hard to do

#define SSE_SOUND_MICROWIRE // volume, balance, bass & treble, primitive DSP
#define SSE_SOUND_MICROWIRE_MIXMODE//3.6.3

#define SSE_SOUND_MICROWIRE_WRITE_LATENCY // as documented (3.XX?)
#define SSE_SOUND_NO_EXTRA_PER_VBL //compensating hack? changes what?

#define SSE_SOUND_OPTIMISE
//#define SSE_SOUND_OPTION_DISABLE_DSP // not needed if no anti-aliasing
#define SSE_SOUND_RECOMMEND_OPTIONS

#ifdef WIN32
//#define SSE_SOUND_SKIP_DSOUND_TEST
#endif

#define SSE_SOUND_VOL // -6db for PSG chipsound (using DSP)
#define SSE_SOUND_VOL_LOGARITHMIC // more intuitive setting
#define SSE_SOUND_FILTER_STE // same very simple filter as for STF

#ifdef UNIX
#define SS_RTAUDIO_LARGER_BUFFER //simplistic attempt
#endif


#endif


/////////////////////////////////
// ST Model (various STF, STE) //
/////////////////////////////////

#if defined(SSE_STF)
//note all those switches have been useful for SSE_SHIFTER_PALETTE_NOISE
#define SSE_STF_0BYTE
#define SS_STE_2MB // auto make RAM 2MB, more compatible
#define SSE_STF_1MB // auto make RAM 1MB, more compatible
#define SSE_STF_8MHZ // 3.6.1 - for Panic study!
#define SSE_STF_BLITTER
#define SSE_STF_DMA
#define SSE_STF_HSCROLL
#define SSE_STF_LEFT_OFF 
#define SSE_STF_LINEWID
#ifdef WIN32
#define SSE_STF_MATCH_TOS // select a compatible TOS for next reset
#endif
#define SSE_STF_MEGASTF // blitter in STF (could be useful?) + 4MB!
#define SSE_STF_MMU_PREFETCH
#define SSE_STF_PADDLES
#define SSE_STF_PAL
#define SSE_STF_SDP
#define SSE_STF_SHIFTER_IOR
#define SSE_STF_VBASELO
#define SSE_STF_VERT_OVSCN

#endif


///////////////
// STRUCTURE //
///////////////

#if defined(SSE_STRUCTURE)

#define SSE_STRUCTURE_CPU_POKE_NOINLINE //little detail

#define SSE_STRUCTURE_DECLA // decla.h files

#if defined(LEGACY_BUILD)
#define SSE_STRUCTURE_NEW_H_FILES // for all features
#endif

#if !defined(LEGACY_BUILD)
#define SSE_STRUCTURE_NEW_H_FILES // necessary
#define SSE_STRUCTURE_SSE_OBJ // try to have all separate SSE objects
#endif

#define SSE_STRUCTURE_DMA_INC_ADDRESS
#if defined(LEGACY_BUILD)
//#define SSE_STRUCTURE_INFO // just telling files included in modules
#endif
#define SSE_STRUCTURE_IOR

#if defined(SSE_STRUCTURE_NEW_H_FILES)
// those were dev switches (TODO remove, just use SSE_STRUCTURE_DECLA)
#define SSE_STRUCTURE_ACC_H
#define SSE_STRUCTURE_ARCHIVE_H
#define SSE_STRUCTURE_BLITTER_H
#define SSE_STRUCTURE_BOILER_H
#define SSE_STRUCTURE_CPU_H
#define SSE_STRUCTURE_D2_H
#define SSE_STRUCTURE_DATALOADSAVE_H
#define SSE_STRUCTURE_DEBUGEMU_H
#define SSE_STRUCTURE_DIRID_H
#define SSE_STRUCTURE_DISKMAN_H
#define SSE_STRUCTURE_DISPLAY_H
#define SSE_STRUCTURE_DRAW_H
#define SSE_STRUCTURE_DWINEDIT_H
#define SSE_STRUCTURE_EMULATOR_H
#define SSE_STRUCTURE_FDC_H
#define SSE_STRUCTURE_FLOPPYDRIVE_H
#define SSE_STRUCTURE_GUI_H
//#define SSE_STRUCTURE_HARDDISKMAN_H
#define SSE_STRUCTURE_HDIMG_H
//#define SSE_STRUCTURE_HISTORYLIST_H
#define SSE_STRUCTURE_IKBD_H
#define SSE_STRUCTURE_INFOBOX_H //systematic use of .decla.h
#define SSE_STRUCTURE_INITSOUND_H
#define SSE_STRUCTURE_IOLIST_H
#define SSE_STRUCTURE_IORW_H
#define SSE_STRUCTURE_KEYTABLE_H
#define SSE_STRUCTURE_LOADSAVE_H
#define SSE_STRUCTURE_MACROS_H
#define SSE_STRUCTURE_MEMBROWSER_H
#define SSE_STRUCTURE_MFP_H
#define SSE_STRUCTURE_MIDI_H
#define SSE_STRUCTURE_MRSTATIC_H
#define SSE_STRUCTURE_NOTIFYINIT_H
#define SSE_STRUCTURE_ONEGAME_H
#define SSE_STRUCTURE_OPTIONS_H
#define SSE_STRUCTURE_OSD_H
#define SSE_STRUCTURE_PALETTE_H
//#define SSE_STRUCTURE_PATCHESBOX_H// nothing to do?
#define SSE_STRUCTURE_PSG_H
//#define SSE_STRUCTURE_RESET_H// nothing to do?
#define SSE_STRUCTURE_RS232_H
#define SSE_STRUCTURE_RUN_H
//#define SSE_STRUCTURE_SCREENSAVER_H// nothing to do?
#define SSE_STRUCTURE_SHORTCUTBOX_H
#define SSE_STRUCTURE_STEEMH_H
#define SSE_STRUCTURE_STEMDIALOGS_H
#define SSE_STRUCTURE_STEMDOS_H
#define SSE_STRUCTURE_STJOY_H
#define SSE_STRUCTURE_STPORTS_H
#define SSE_STRUCTURE_TRACE_H
#define SSE_STRUCTURE_TRANSLATE_H
#endif//#if defined(SSE_STRUCTURE_NEW_H_FILES)


#if defined(SSE_STRUCTURE_SSE_OBJ)
// We begin with our SSE additions. Notice that it already slows down compiling
#define SSE_STRUCTURE_SSE6301_OBJ//3.6.0
#define SSE_STRUCTURE_SSECPU_OBJ//3.6.0
#define SSE_STRUCTURE_SSEDEBUG_OBJ//3.6.0
#define SSE_STRUCTURE_SSEFLOPPY_OBJ//3.6.1
#define SSE_STRUCTURE_SSEFRAMEREPORT_OBJ//3.6.1
//#define SSE_STRUCTURE_SSE_INTERRUPT_OBJ//skip for now...
#define SSE_STRUCTURE_SSESHIFTER_OBJ//3.6.1
#define SSE_STRUCTURE_SSESTF_OBJ//3.6.1
#define SSE_STRUCTURE_SSESTF_VIDEO//3.6.1
#endif//#if defined(SSE_STRUCTURE_SSE_OBJ)



#endif


/////////////
// TIMINGS //
/////////////

#if defined(SSE_TIMINGS)

#define SSE_TIMINGS_MS_TO_HBL

#endif


/////////
// TOS //
/////////
#if defined(SSE_TOS)

#ifdef SSE_HACKS
#define SSE_TOS_PATCH106 // a silly bug, a silly hack
#define SSE_TOS_STE_FAST_BOOT //from hatari
#endif

#define SSE_TOS_FILETIME_FIX //from Petari//3.6.0
#define SSE_TOS_GEMDOS_NOINLINE//3.6.1
#define SSE_TOS_GEMDOS_PEXEC6 //3.6.1 ReDMCSB 100% in TOS104

#define SSE_TOS_STRUCT//3.6.1

#define SSE_TOS_GEMDOS_VAR1 //various unimportant fixes 3.6.1

//#define SSE_TOS_NO_INTERCEPT_ON_RTE1 // fix (not) Megamax C on ReDMCSB//3.6.0
#if defined(SSE_TOS_STRUCT)
#define SSE_TOS_NO_INTERCEPT_ON_RTE2 //3.6.1//try to be less radical... ReDMCSB 50% in TOS102
#ifdef WIN32
#define SSE_TOS_SNAPSHOT_AUTOSELECT//3.6.1
#define SSE_TOS_SNAPSHOT_AUTOSELECT2//3.6.1//with refactoring
#define SSE_TOS_SNAPSHOT_AUTOSELECT3//3.6.1//options.cpp uses refactoring
#endif//win32
#endif
#ifdef SSE_DEBUG
#define SSE_TOS_DONT_TRACE_3F//read file//3.6.1
#define SSE_TOS_DONT_TRACE_40//write file//3.6.1
#define SSE_TOS_DONT_TRACE_42//seek file//3.6.1
#define SSE_TOS_TRACE_CONOUT//3.6.1
#endif

#define SSE_TOS_WARNING1 // version/ST type//3.6.1

// fixes by other people: //TODO, another big category?
#define SSE_AVTANDIL_FIX_001 // Russin TOS number
#define SSE_MEGAR_FIX_001 // intercept GEM in extended resolution

#endif


////////////////
// UNIX/LINUX // 
////////////////

#if defined(SSE_UNIX)

#if !defined(UNIX)
#undef SSE_UNIX
#else

#define SSE_UNIX_OPTIONS_SSE_ICON
#define SSE_UNIX_STATIC_VAR_INIT //odd

#if defined(SSE_DEBUG)
#define SSE_UNIX_TRACE // TRACE into the terminal (if it's open?)
#define SSE_DEBUG_TRACE_FILE
#endif
#endif

#endif


/////////////
// VARIOUS //
/////////////

#define NO_RAR_SUPPORT // I removed the library, so it's unconditional

#if defined(SSE_VARIOUS)
#define SSE_VAR_ASSOCIATE
#ifdef SSE_VAR_ASSOCIATE
#define SSE_VAR_ASSOCIATE_CU // current user, not root
#define SSE_VAR_MAY_REMOVE_ASSOCIATION
#define SSE_VAR_NO_ASSOCIATE_STC // cartridge, who uses that?
#define SSE_VAR_NO_AUTO_ASSOCIATE_DISK_STS_STC // disk images + STS STC
#define SSE_VAR_NO_AUTO_ASSOCIATE_MISC // other .PRG, .TOS...
#endif
#ifdef WIN32
#define SSE_VAR_CHECK_SNAPSHOT
#endif
#define SSE_VAR_DISK_MANAGER_LONG_NAMES1
#define SSE_VAR_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
#define SSE_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES // at final save
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SSE_VAR_F12 // F12 starts/stops emulation
#endif
#define SSE_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen - option?
//#define SSE_VAR_HIDE_OPTIONS_AT_START // hack before debugging
#define SSE_VAR_INFOBOX0 // enum 3.6.1
#define SSE_VAR_INFOBOX1 // SSE internet sites
#define SSE_VAR_INFOBOX2 // SSE readme + FAQ
#define SSE_VAR_INFOBOX3 // readme text font
#define SSE_VAR_INFOBOX4 // readme 80 col 
#define SSE_VAR_INFOBOX5 // don't take 64K on the stack!
#define SSE_VAR_INFOBOX6 // no cartridge howto 3.6.1
#define SSE_VAR_INFOBOX7 // specific hints 3.6.1
//TODO also in unix
#define SSE_VAR_KEYBOARD_CLICK // not a sound nor IKBD option
#define SSE_VAR_MOUSE_CAPTURE 
#define SSE_VAR_MSA_CONVERTER // don't prompt if found
#define SSE_VAR_NOTIFY //adding some notify during init
#define SSE_VAR_NO_UPDATE // remove all code in relation to updating
#define SSE_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah
#define SSE_VAR_OPTION_SLOW_DISK // because many people miss it in disk manager
#define SSE_VAR_OPTIONS_ICON_VERSION
#define SSE_VAR_OPTIONS_REFRESH // 6301, STF... up-to-date with snapshot
//#define SSE_VAR_POWERON2 //3.6.1 try other, safer way ;) //3.6.2: not safe either :)
#define SSE_VAR_RESET_BUTTON // invert
#define SSE_VAR_RESIZE // reduce memory set (int->BYTE etc.)
#define SSE_VAR_REWRITE // to conform to what compilers expect (warnings...)
#ifdef WIN32
#define SSE_VAR_STATUS_STRING // "status bar" in the icon bar
#define SSE_VAR_STATUS_STRING_6301
#define SSE_VAR_STATUS_STRING_ADAT
#define SSE_VAR_STATUS_STRING_DISK_NAME
#define SSE_VAR_STATUS_STRING_DISK_NAME_OPTION
#define SSE_VAR_STATUS_STRING_FULL_ST_MODEL//3.6.1
#define SSE_VAR_STATUS_STRING_HACKS
#define SSE_VAR_STATUS_STRING_IPF
#define SSE_VAR_STATUS_STRING_PASTI
#define SSE_VAR_STATUS_STRING_VSYNC//3.6.0
#endif
#define SSE_VAR_STEALTH // don't tell we're an emulator (option)
#ifdef WIN32
#define SSE_VAR_UNRAR // using unrar.dll, up to date
#endif
#define SSE_VAR_WINDOW_TITLE


#endif 


///////////
// VIDEO //
///////////

#if defined(SSE_VIDEO)

#define SSE_VID_ADJUST_DRAWING_ZONE1 // attempt to avoid crash
#define SSE_VID_ADJUST_DRAWING_ZONE2
//#define SSE_VID_AUTOOFF_DECRUNCHING // bad idea (complaints)
#define SSE_VID_BLIT_TRY_BLOCK //?

#define SSE_VID_BORDERS // option display size
#if defined(SSE_VID_BORDERS)
//#ifdef WIN32 // Unix?
#define SSE_VID_BORDERS_412 // 
#define SSE_VID_BORDERS_413 // best fit for overscan?
#define SSE_VID_BORDERS_416 
#define SSE_VID_BORDERS_416_NO_SHIFT
#define SSE_VID_BORDERS_416_NO_SHIFT1 //3.6.1: check border on/off
#define SSE_VID_BORDERS_BIGTOP // more lines for palette effects
#define SSE_VID_BORDERS_LB_DX // rendering-stage trick rather than painful hacks
#define SSE_VID_BORDERS_LB_DX1 //3.6.1: check border on/off
//#endif
#endif

#define SSE_VID_BPOC // Best Part of the Creation fit display 800 hack
#define SSE_VID_CHECK_DDFS // is video card/display capable?
//#define SSE_VID_CHECK_DDFS2 //list all modes
#if defined(WIN32) 
#define SSE_VID_DIRECT3D // TODO, like in WinUAE there's an option
#define SSE_VID_RECORD_AVI //avifile
#endif

#if defined(WIN32) //TODO Unix
#define SSE_VID_SAVE_NEO // screenshots in ST Neochrome format
#endif

#if defined(WIN32)
#define SSE_VID_3BUFFER // Triple Buffer to remove tearing
#if defined(SSE_VID_3BUFFER)
#define SSE_VID_3BUFFER_FS // fullscreen: stretch mode only
//#define SSE_VID_3BUFFER_NO_VSYNC // for tests: "VSync" is necessary
#define SSE_VID_3BUFFER_WIN //windowed mode (necessary for FS)
#endif//SSE_VID_3BUFFER
#ifdef SSE_BETA 
#define SSE_VID_D3D // TODO for v3.7
#endif
#endif

#define SSE_VID_FREEIMAGE1 //3.6.3 init library
#define SSE_VID_FREEIMAGE2 //3.6.3 always convert pixels
#define SSE_VID_FREEIMAGE3 //3.6.3 flip pic
#define SSE_VID_FREEIMAGE3 //3.6.3 remove WBMP
#if defined(SSE_DELAY_LOAD_DLL)
#define SSE_VID_FREEIMAGE4 //3.6.4 use official header
#endif
#define SSE_VID_MEMORY_LOST // no message box

#if defined(WIN32)
#define SSE_VID_SCANLINES_INTERPOLATED // using stretch mode!
#define SSE_VID_SCANLINES_INTERPOLATED_MED
#define SSE_VID_SCANLINES_INTERPOLATED_SSE // put option in SSE page
#endif

#define SSE_VID_SCREENSHOTS_NUMBER

#if defined(WIN32)
#define SSE_VID_VSYNC_WINDOW // no tearing and smooth scrolling also in window
#define SSE_VID_VSYNC_WINDOW_CRASH_FIX1//3.6.0
//#define SSE_VID_VSYNC_WINDOW_CRASH_FIX2 //safer but annoying
#define SSE_VID_VSYNC_WINDOW_CRASH_FIX3 //TODO find something better?
#endif

#if defined(SSE_DEBUG) 
//#define SHOW_DRAW_SPEED //was already in Steem
//#define SSE_VID_VERT_OVSCN_OLD_STEEM_WAY // only for vertical overscan
#endif

#endif


/////////////////
// ADAPTATIONS //
/////////////////

// not all switches are compatible with each other:

#if !defined(SSE_ACIA)
#undef SSE_IKBD_6301
#endif

#if !defined(SSE_ACIA_REGISTERS)
#undef SSE_ACIA_DOUBLE_BUFFER_TX
#endif

#if !defined(SSE_ACIA_DOUBLE_BUFFER_RX) || !defined(SSE_ACIA_DOUBLE_BUFFER_TX)
#undef SSE_ACIA_TDR_COPY_DELAY
#endif

#if !defined(SSE_DEBUG) || !defined(SSE_OSD_DEBUG_MESSAGE)
#undef SSE_CPU_PREFETCH_TRACE 
#undef SSE_CPU_TRACE_DETECT 
#endif

#if !defined(SSE_DMA)
#undef SSE_STRUCTURE_DMA_INC_ADDRESS
#endif

#if !defined(SSE_DMA_FIFO)
#undef SSE_IPF
#undef SSE_IPF_OSD
#endif

#if !defined(SSE_FDC)
#undef SSE_OSD_DRIVE_INFO2
#endif

#if !defined(SSE_DMA) || !defined(SSE_FDC)
#undef SSE_OSD_DRIVE_LED2
#undef SSE_OSD_DRIVE_LED3
#endif

#if !defined(SSE_FLOPPY)
#undef SSE_VAR_STATUS_STRING_IPF
#undef SSE_VAR_STATUS_STRING_ADAT
#endif

#if !defined(SSE_IKBD)
#undef SSE_ACIA_DOUBLE_BUFFER_RX
#undef SSE_ACIA_DOUBLE_BUFFER_TX

#endif

#if !defined(SSE_MFP)
#undef SSE_FDC_PRECISE_HBL
#endif

#if defined(SSE_OSD_LOGO3)
#undef SSE_OSD_LOGO
#undef SSE_OSD_LOGO2
#endif

#if defined(SSE_OSD_LOGO2)
#undef SSE_OSD_LOGO3
#endif


#if !defined(SSE_DRIVE)
#undef SSE_PASTI_ONLY_STX 
#endif

#if defined(SSE_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
#undef SSE_MMU_WAKE_UP_IO_BYTES_W
#endif

#if !defined(SSE_OSD_SCROLLER_CONTROL)
#undef SSE_VAR_SCROLLER_DISK_IMAGE
#endif

#if !defined(SSE_SHIFTER)
#undef SSE_CPU_3615GEN4_ULM
#undef SSE_CPU_CHECK_VIDEO_RAM_B
#undef SSE_CPU_CHECK_VIDEO_RAM_L
#undef SSE_CPU_CHECK_VIDEO_RAM_W
#undef SSE_MMU_WAKE_UP_IO_BYTES_R
#undef SSE_MMU_WAKE_UP_IOR_HACK
#undef SSE_MMU_WAKE_UP_IOW_HACK
#undef SSE_DEBUG_FRAME_REPORT//same
#endif

#if defined(SSE_SHIFTER_UNSTABLE)
#undef SSE_SHIFTER_DRAGON1
#endif

#if !defined(SSE_STF)
#undef SSE_SHIFTER_MED_RES_SCROLLING
#undef SSE_SHIFTER_UNSTABLE
#endif

#if !defined(SSE_STRUCTURE_NEW_H_FILES)
// mods after we created "decla" files are not duplicated
#undef SSE_INT_VBL_IACK
#undef SSE_VAR_STATUS_STRING
#undef SSE_VAR_STATUS_STRING_ADAT
#undef SSE_VAR_STATUS_STRING_DISK_NAME
#undef SSE_VID_3BUFFER
#undef SSE_VID_3BUFFER_FS
#undef SSE_VID_3BUFFER_WIN
#undef SSE_VID_BORDERS_BIGTOP
#endif


#if !USE_PASTI
#undef SSE_DMA_FIFO_PASTI
#endif

#if defined(SSE_UNIX) //temp!
#define SSE_SDL_DEACTIVATE
#endif

#if !defined(SSE_VID_BORDERS) || !defined(SSE_HACKS)
#undef SSE_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK
#endif

#if defined(SSE_DISK_IMAGETYPE) && !defined(SSE_PASTI_ON_WARNING2)//3.6.2
#error("Incompatible switches") // or remove some old code after tested OK
#endif

#if defined(SS_SSE_LEAN_AND_MEAN) //TODO
//6301 not optional...
#undef SSE_VID_CHECK_DDFS
#undef SSE_VID_RECORD_AVI
#undef SSE_VID_SAVE_NEO
#endif

/////////////////////////////////
// DrCoolZic Conditional Flags //
/////////////////////////////////
//
// These flags protects all modifications done by DrCoolZic
// DrCoolZic is only defined in STEVEN_SEAGAL 
// This implies that you should define STEVEN_SEAGAL to het my modif
//
#define DR_COOL_ZIC
#ifdef DR_COOL_ZIC

// Following are flags for generic code modifications that should work 
// regardless of the compiler
#define SSE_VS2012_INIT		// correct uninitialized variables
#define SSE_VS2012_POW		// First arg of pow function should be a real

// Following flags are for successfull compilation with VS 2012
// They only make sense if you are using VS2012 or above
#if _MSC_VER > 1600
#ifndef SSE_VS2012_INIT		// required
#define SSE_VS2012_INIT
#endif
#ifndef SSE_VS2012_POW		// required
#define SSE_VS2012_POW
#endif
#define SSE_VS2012_WARNINGS	// remove many VS2012 warnings
#define SSE_VS2012_DELAYDLL	// remove some directives associated with delayed 
                            //DLL not supported by VS2012
#endif	// we are compiling for MS VS2012 or above

#endif	// DR_COOL_ZIC

/////////////////////////////////////
// if STEVEN_SEAGAL is NOT defined //
/////////////////////////////////////
#else 
// a fair warning (it actually helps detect building errors)
#pragma message("Are you mad? You forgot to #define STEVEN_SEAGAL!") 
#include "SSEDecla.h" // still need to neutralise debug macros


#define NO_RAR_SUPPORT // I removed the library, so it's unconditional

#endif//#ifdef STEVEN_SEAGAL

#endif// #ifndef STEVEN_SEAGAL_H 
