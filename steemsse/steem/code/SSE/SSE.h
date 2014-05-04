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

SS_DEBUG, if needed, should be defined in the project/makefile.

LEGACY_BUILD is now needed to build Steem using the 3 big modules
Steem Emu and Helper.
Those configurations are marked with '_modules'.
When it's not defined, more separate units are compiled (work in progress).

TODO: All switches SS_ -> SSE_ to mark the branch rather than the author

Then each author may define his name (STEVEN_SEAGAL)

eg:

#if defined(STEVEN_SEAGAL) && defined(SS_CPU_PREFETCH)
 //... (awesome mod)
#endif

#if defined(JEAN_CLAUDE_VAN_DAMME) && defined(SS_CPU_PREFETCH)
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

//#define SS_BETA //title, OSD, plus some testing - new features
//#define SS_BETA_BUGFIX // beta for just bugfixes

#if defined(SS_BETA) //TODO check before release what will stay beta...

#define SSE_VERSION 370 // big features coming up
#define SSE_VERSION_TXT "3.7.0" 
#ifdef DEBUG_BUILD
#define WINDOW_TITLE "Steem Boiler 3.7.0B"
#else
#define WINDOW_TITLE "Steem Beta 3.7.0"
#endif

#elif defined(SS_BETA_BUGFIX) // bugfixes (point release)

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

#define SS_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SS_BLITTER    // spelled BLiTTER by those in the known!
#define SS_CPU        // MC68000 microprocessor
#define SS_CARTRIDGE  // ROM Cartridge slot
#define SS_FLOPPY     // Group switch for disk drive (DMA, FDC, Pasti, etc)
#define SS_GLUE       // TODO
#define SS_HACKS      // an option for dubious fixes
#define SS_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#define SS_INTERRUPT  // HBL, VBL  
#define SS_MFP        // MC68901 Multi-Function Peripheral Chip
#define SS_MIDI       // 
#define SS_MMU        // Memory Manager Unit (of the ST, no paging)
#define SS_OSD        // On Screen Display (drive leds, track info, logo)
#define SS_SDL        // Simple DirectMedia Layer
#define SS_SHIFTER    // The legendary custom shifter and all its tricks
#define SS_SOUND      // YM2149, STE DMA sound, Microwire
#define SS_STF        // switch STF/STE
#define SS_STRUCTURE  // conditions other switches (that or duplicate all)
#define SS_TIMINGS    // TODO (only HBL now)
#define SS_TOS        // The Operating System
#define SS_UNIX       // Linux build must be OK too (may lag)
#define SS_VARIOUS    // Mouse capture, keyboard click, unrar...
#define SS_VIDEO      // large borders, screenshot, recording

#endif

// Adapt some switches (also see by the end of this file)
#ifdef WIN32
#undef SS_UNIX
#endif

#if SSE_VERSION<380 && !defined(SS_BETA)
#undef SS_SDL 
#endif

#ifdef BCC_BUILD
#define SS_SDL // it's in the makefile now (TODO?)
#endif

//////////
// TEMP //
//////////

#if defined(SS_BETA) || defined(SS_BETA_BUGFIX)
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
#define SS_SSE_OPTION_PAGE // a new page for all our options
#define SS_SSE_OPTION_STRUCT // structure SSEOption 
#define SS_SSE_CONFIG_STRUCT // structure SSEConfig 
#ifdef WIN32
#define SS_DELAY_LOAD_DLL // can run without DLL
#endif


//////////
// ACIA //
//////////

#if defined(SS_ACIA)

#define SS_ACIA_BUS_JAM_NO_WOBBLE // simple "fix"
//#define SS_ACIA_BUS_JAM_PRECISE_WOBBLE //TODO
#define SS_ACIA_DOUBLE_BUFFER_RX // only from 6301 (not MIDI) 
#define SS_ACIA_DOUBLE_BUFFER_TX // only to 6301 (not MIDI)
//#define SS_ACIA_IRQ_ASSERT_READ_SR //TODO
#define SS_ACIA_IRQ_DELAY2//3.6.1 back to this approach
#define SS_ACIA_MIDI_TIMING1 //check
#define SS_ACIA_REGISTERS // formalising the registers
//#define SS_ACIA_DONT_CLEAR_DR //?
#define SS_ACIA_NO_RESET_PIN // don't reset on warm reset
#define SS_ACIA_TDR_COPY_DELAY // effect on SR
#define SS_ACIA_TDR_COPY_DELAY2 // effect on byte flow

#endif


/////////////
// BLITTER //
/////////////

#if defined(SS_BLITTER)

#define SS_BLT_BLIT_MODE_CYCLES // #cycles in BLIT Mode in NOPs, not M68K cycles
#define SS_BLT_BLIT_MODE_INTERRUPT // trigger at once (not after blit phase)
#define SS_BLT_HOG_MODE_INTERRUPT // no interrupt in hog mode

//#define SS_BLT_OVERLAP // TODO ?
//#define SS_BLT_TIMING // based on a table, but Steem does it better
#define SS_BLT_YCOUNT // 0=65536

#endif


///////////////
// CARTRIDGE //
///////////////

#if defined(SS_CARTRIDGE)

#define SS_CARTRIDGE_64KB_OK
#define SS_CARTRIDGE_DIAGNOSTIC
#define SS_CARTRIDGE_NO_CRASH_ON_WRONG_FILE
#define SS_CARTRIDGE_NO_EXTRA_BYTES_OK

#endif


/////////
// CPU //
/////////

#if defined(SS_CPU)

#define SS_CPU_ALT_REG_NAMES  // convenience
#define SS_CPU_DIV          // divide like Caesar
#define SS_CPU_EXCEPTION    // crash like Windows 98
#define SS_CPU_FETCH_IO     // fetch like a dog in outer space
#define SS_CPU_INLINE       // supposes TM68000 exists!
#define SS_CPU_MOVE_B       // move like a superstar
#define SS_CPU_MOVE_W       
#define SS_CPU_MOVE_L
#define SS_CPU_MOVEM_MR_L//v3.6.0
#define SS_CPU_POKE         // poke like a C64 
#define SS_CPU_PREFETCH     // prefetch like a dog
#define SS_CPU_ROUNDING     // round like a rolling stone
#define SS_CPU_TAS          // 4 cycles fewer if memory
#define SS_CPU_UNSTOP//v3.6.0


#if defined(SS_CPU_DIV)
#define SS_CPU_DIVS_OVERFLOW//v3.6.1
#define SS_CPU_DIVU_OVERFLOW//v3.6.1
#endif

#if defined(SS_CPU_EXCEPTION)

#define SS_CPU_ASSERT_ILLEGAL // assert before trying to execute (not general)
//#define SS_CPU_EXCEPTION_TRACE_PC // reporting all PC (!)
#if defined(SS_BETA) && defined(SS_DEBUG)
//#define SS_CPU_DETECT_STACK_PC_USE // push garbage!!
#endif

#if defined(SS_HACKS) // hacks on: we don't use 'true PC', but this is undef further
#define SS_CPU_HACK_BLOOD_MONEY
//#define SS_CPU_HACK_PHALEON // TODO: why does it work with or without?
#define SS_CPU_HACK_WAR_HELI
#endif

//#define SS_CPU_HIMEM_BONUS_BYTES // for F-29: bad idea
#define SS_CPU_IGNORE_RW_4MB // for F-29, works but... //v3.6.0
#define SS_CPU_IGNORE_WRITE_0 // for Aladin, may write on 1st byte
#define SS_CPU_POST_INC // no post increment if exception 
#define SS_CPU_PRE_DEC // no "pre" decrement if exception!
#define SS_CPU_SET_DEST_TO_0 // for Aladin

// PC as pushed in case of bus error, based on microcodes
#define SS_CPU_TRUE_PC 
#define SS_CPU_TRUE_PC_AND_NO_HACKS // option 'Hacks' or not: true PC

#endif//exception

#if defined(SS_CPU_FETCH_IO)
#define SS_CPU_FETCH_IO_FULL // need all or nothing for: Union Demo!
#endif

#if defined(SS_CPU_INLINE) //todo
#define SS_CPU_INLINE_PREFETCH_SET_PC
#define SS_CPU_INLINE_READ_BWL
#define SS_CPU_INLINE_READ_FROM_ADDR
#endif//SS_CPU_INLINE



#if defined(SS_CPU_POKE)
//#define SS_CPU_3615GEN4_ULM //targeted for 36.15 GEN4 by ULM
#define SS_CPU_CHECK_VIDEO_RAM_B
#define SS_CPU_CHECK_VIDEO_RAM_L
#define SS_CPU_CHECK_VIDEO_RAM_W // including: 36.15 GEN4 by ULM
#endif //poke

#if defined(SS_CPU_PREFETCH)

#if defined(SS_BETA) && defined(SS_DEBUG)
//#define SS_CPU_NO_PREFETCH // fall for all prefetch tricks (debug)
#endif


// Change no timing, just the macro used, so that we can identify what timings
// are for prefetch:
#define SS_CPU_FETCH_TIMING  // TODO can't isolate

// Move the timing counting from FETCH_TIMING to PREFETCH_IRC:
#define SS_CPU_PREFETCH_TIMING //big, big change
#ifdef SS_CPU_PREFETCH_TIMING 
//#define SS_CPU_PREFETCH_TIMING_CMPI_B // move the instruction timing place-no
//#define SS_CPU_PREFETCH_TIMING_CMPI_W // move the instruction timing place
#define SS_CPU_PREFETCH_TIMING_CMPI_L // move the instruction timing place
#define SS_CPU_PREFETCH_TIMING_MOVEM // at wrong place, probably compensates bug
#define SS_CPU_PREFETCH_TIMING_SET_PC // necessary for some SET PC cases
//#define SS_CPU_PREFETCH_TIMING_EXCEPT // to mix unique switch + lines
#endif
#if !defined(SS_CPU_PREFETCH_TIMING) || defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
#define CORRECTING_PREFETCH_TIMING 
#endif
#ifdef CORRECTING_PREFETCH_TIMING
// powerful prefetch debugging switches

#define SS_CPU_LINE_0_TIMINGS // 0000 Bit Manipulation/MOVEP/Immediate
#define SS_CPU_LINE_1_TIMINGS // 0001 Move Byte
#define SS_CPU_LINE_2_TIMINGS // 0010 Move Long
#define SS_CPU_LINE_3_TIMINGS // 0011 Move Word
#define SS_CPU_LINE_4_TIMINGS // 0100 Miscellaneous
#define SS_CPU_LINE_5_TIMINGS // 0101 ADDQ/SUBQ/Scc/DBcc/TRAPcc
#define SS_CPU_LINE_6_TIMINGS // 0110 Bcc/BSR/BRA
#define SS_CPU_LINE_7_TIMINGS // 0111 MOVEQ
#define SS_CPU_LINE_8_TIMINGS // 1000 OR/DIV/SBCD
#define SS_CPU_LINE_9_TIMINGS // 1001 SUB/SUBX
#define SS_CPU_LINE_A_TIMINGS // 1010 (Unassigned, Reserved)
#define SS_CPU_LINE_B_TIMINGS // 1011 CMP/EOR
#define SS_CPU_LINE_C_TIMINGS // 1100 AND/MUL/ABCD/EXG
#define SS_CPU_LINE_D_TIMINGS // 1101 ADD/ADDX
#define SS_CPU_LINE_E_TIMINGS // 1110 Shift/Rotate/Bit Field
#define SS_CPU_LINE_F_TIMINGS // 1111 Coprocessor Interface/MC68040 and CPU32 Extensions

#endif

// all those switches because we had some nasty bug... 

#define SS_CPU_PREFETCH_ASSERT
#define SS_CPU_PREFETCH_CALL
#define SS_CPU_PREFETCH_CLASS
#define SS_CPU_PREFETCH_BSET
#define SS_CPU_PREFETCH_CHK
#define SS_CPU_PREFETCH_JSR
#define SS_CPU_PREFETCH_MOVE_FROM_SR
//#define SS_CPU_PREFETCH_MULS //not used (TODO?)
//#define SS_CPU_PREFETCH_MULU
#define SS_CPU_PREFETCH_NOP
#define SS_CPU_PREFETCH_PEA

#define SS_CPU_PREFETCH_4PIXEL_RASTERS //no hack, just disabling switch

#if defined(SS_CPU_PREFETCH_TIMING) || defined(CORRECTING_PREFETCH_TIMING)
#define SS_CPU_PREFETCH_MOVE_MEM // this was forgotten in v3.5.0! limited impact
// Additional fixes based on Yacht, each one protected (3.5.0 seems stable)
// if it can change anything (TODO)
#define SS_CPU_YACHT_TAS // confirming what Steem authors suspected
#endif

#if !defined(SS_OSD_CONTROL)
#define SS_CPU_PREFETCH_TRACE 
#define SS_CPU_TRACE_DETECT //was in prefetch define zone BTW
#endif

#endif//prefetch

#if defined(SS_CPU_ROUNDING)

#define SS_CPU_ROUNDING_ADD_L // EA = -(An) //v3.6.0
#define SS_CPU_ROUNDING_ADDA_L // EA = -(An) //v3.6.0

//#define SS_CPU_ROUNDING_SOURCE_100_B // -(An)
//#define SS_CPU_ROUNDING_SOURCE_100_W // -(An): no
//#define SS_CPU_ROUNDING_SOURCE_100_L // -(An): no!

#endif//rounding

#define SS_CPU_LINE_F // for interrupt depth counter
#endif


///////////
// DEBUG //
///////////

#if defined(SS_DEBUG)

#define SS_DEBUG_TRACE

#ifdef SS_DEBUG_TRACE
#ifdef _DEBUG // VC
#define SS_DEBUG_TRACE_IDE
#endif
#if defined(DEBUG_BUILD) 
#define SS_DEBUG_TRACE_FILE
#else//VC
//#define SS_DEBUG_TRACE_FILE
#endif
#endif


// boiler + IDE

#define SS_DEBUG_FRAME_REPORT

#if defined(SS_DEBUG_FRAME_REPORT)

#define SS_DEBUG_FRAME_REPORT_ON_STOP // each time we stop emulation

#if defined(DEBUG_BUILD)
#define SS_DEBUG_FRAME_REPORT_MASK // for interactive control in boiler//3.6.1
#endif

#if !defined(SS_DEBUG_FRAME_REPORT_MASK) // control by define
#define SS_DEBUG_FRAME_REPORT_ACIA
#define SS_DEBUG_FRAME_REPORT_BLITTER
#define SS_DEBUG_FRAME_REPORT_PAL
#define SS_DEBUG_FRAME_REPORT_HSCROLL // & linewid
#define SS_DEBUG_FRAME_REPORT_SDP_LINES
#define SS_DEBUG_FRAME_REPORT_SDP_READ
#define SS_DEBUG_FRAME_REPORT_SDP_WRITE
#define SS_DEBUG_FRAME_REPORT_SHIFTER_TRICKS
#define SS_DEBUG_FRAME_REPORT_SHIFTER_TRICKS_BYTES
#define SS_DEBUG_FRAME_REPORT_SHIFTMODE
#define SS_DEBUG_FRAME_REPORT_SYNCMODE
#define SS_DEBUG_FRAME_REPORT_VIDEOBASE

///#define SS_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN//no, sthg else!
#endif

#endif//frame report

#define SS_DEBUG_REPORT_SCAN_Y_ON_CLICK
#define SS_DEBUG_REPORT_SDP // tracking sdp at start of each scanline
#define SS_DEBUG_REPORT_SDP_ON_CLICK // yeah!

// boiler
#if defined(DEBUG_BUILD) //TODO add other mods here
#define SS_DEBUG_BLAZING_STEP_OVER 
#define SS_DEBUG_BROWSER_6301
#define SS_DEBUG_BROWSER_ACIA
#define SS_DEBUG_BROWSER_BLITTER
#define SS_DEBUG_BROWSER_DMASOUND
#define SS_DEBUG_BROWSER_INSTRUCTIONS //window name
#define SS_DEBUG_BROWSER_PSEUDO_IO_SCROLL // for the bigger 6301 browser
#define SS_DEBUG_BROWSER_SHIFTER
#define SS_DEBUG_BROWSER_VECTORS
#define SS_DEBUG_CLIPBOARD // right-click on 'dump' to copy then paste
//#define SS_DEBUG_CPU_LOG_NO_STOP // never stop
#define SS_DEBUG_CPU_TRACE_NO_STOP // depends on 'suspend logging'
#define SS_DEBUG_DUMP_6301_RAM
#define SS_DEBUG_FAKE_IO //3.6.1 to control some debug options
#if defined(SS_DEBUG_FAKE_IO)
#define SS_DEBUG_FRAME_INTERRUPTS//3.6.1 OSD, handy
#define SS_DEBUG_TRACE_CONTROL //3.6.1 beyond log options
#endif
#define SS_DEBUG_MOD_REGS // big letters, no =
#define SS_DEBUG_MONITOR_IO_FIX1 // ? word check, not 2x byte on word access
#define SS_DEBUG_MONITOR_RANGE // will stop for every address between 2 stops
#define SS_DEBUG_MONITOR_VALUE // specify value (RW) that triggers stop
#define SS_DEBUG_MONITOR_VALUE2 // write before check
#define SS_DEBUG_MONITOR_VALUE3 // add checks for CLR
#define SS_DEBUG_MONITOR_VALUE4 // 3.6.1 corrections (W on .L)
#define SS_DEBUG_MOUSE_WHEEL // yeah!
#define SS_DEBUG_MOVE_OTHER_SP
#define SS_DEBUG_MOVE_OTHER_SP2//3.6.1, SSP+USP
#if defined(SS_DEBUG_FAKE_IO)
#define SS_DEBUG_MUTE_SOUNDCHANNELS //fake io, 3.6.1
#endif
#define SS_DEBUG_NODIV // no DIV log necessary anymore
#define SS_DEBUG_NO_SOUND_DAMPING //PSG filter control 'd' and 'a'
#define SS_DEBUG_RUN_TO_RTS
//#define SS_DEBUG_SHOW_ACT //useful?
//#define SS_DEBUG_SHOW_FRAME //useful?
#define SS_DEBUG_SHOW_FREQ // sync mode
#define SS_DEBUG_SHOW_INTERRUPT // yeah!
#define SS_DEBUG_SHOW_RES // shift mode
#define SS_DEBUG_SHOW_SDP // the draw pointer
#define SS_DEBUG_SHOW_SR // in HEX on the left of bit flags
#define SS_DEBUG_SSE_PERSISTENT // L/S options 
#define SS_DEBUG_STACK_68030_FRAME //request, to check compatibility
#define SS_DEBUG_STACK_CHOICE//3.6.1
#define SS_DEBUG_TIMER_B // instead of 0
#define SS_DEBUG_TIMERS_ACTIVE // (in reverse video) yeah!
#if defined(SS_DEBUG_FAKE_IO)
#define SS_DEBUG_VIDEO_CONTROL //3.6.1
#endif
#define SS_DEBUG_WIPE_TRACE // as logfile
#endif

#define SS_DEBUG_LOG_OPTIONS // mine

#if defined(SS_DEBUG) && !defined(BCC_BUILD) && !defined(_DEBUG) 
// supposedly we're building the release boiler, make sure features are in

#define SS_DEBUG_TRACE_IO

#if !defined(SS_UNIX)
#define SS_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif

#define SS_DEBUG_START_STOP_INFO

#define SS_IPF_TRACE_SECTORS // show sector info (IPF)

#define SS_IKBD_6301_DUMP_RAM
//#define SS_IKBD_6301_DUMP_RAM_ON_LS//no
#define SS_IKBD_6301_TRACE 
#define SS_IKBD_6301_TRACE_SCI_RX
#define SS_IKBD_6301_TRACE_SCI_TX
#define SS_IKBD_6301_TRACE_KEYS

#else // for custom debugging

#define SS_DEBUG_START_STOP_INFO
#define SS_DEBUG_TRACE_IO

#define SS_IKBD_6301_DUMP_RAM
#define SS_IKBD_6301_TRACE 
#define SS_IKBD_6301_TRACE_SCI_RX
#define SS_IKBD_6301_TRACE_SCI_TX
#define SS_IKBD_6301_TRACE_KEYS

//#define SS_DEBUG_TRACE_CPU_ROUNDING
//#define SS_DEBUG_TRACE_PREFETCH
#define SS_DEBUG_TRACE_SDP_READ_IR

#endif//#if defined(SS_DEBUG_NO_TRACE)

#else // no SS_DEBUG

#endif


////////////
// FLOPPY //
////////////

#if defined(SS_FLOPPY)

#define SS_DMA        // Custom Direct Memory Access chip (disk)
#define SS_DISK       // Disk images 3.6.1
#define SS_DRIVE      // SF314 floppy disk drive
#define SS_FDC        // WD1772 floppy disk controller
#if SSE_VERSION>=370
#define SS_FLOPPY_EVENT//TODO
#endif
#if defined(WIN32)
#define SS_IPF        // CAPS support (IPF disk images) 
#define SS_PASTI      // Improvements in Pasti support (STX disk images)
#ifdef SS_BETA
#define SS_SCP //TODO
#endif
#endif
#define SS_PSG        // YM2149 - for portA first


//////////////////
// FLOPPY: DISK //
//////////////////

//TODO move much SS_DRIVE code here

#if defined(SS_DISK)

#if SSE_VERSION>=370

//imagetype needed for ghost & stw
#define SS_DISK_GHOST //3.7.0
#define SS_DISK_IMAGETYPE //3.7.0
#define SS_DISK_STW // 3.7.0

#if defined(SS_DISK_GHOST)
#define SS_DISK_GHOST_SECTOR // intercept write 1 sector
#define SS_DISK_GHOST_SECTOR_STX1 // in pasti
#endif

#if defined(SS_DISK_STW)
#define SS_DISK_STW_DISK_MANAGER //new context option
#define SS_DISK_STW_DISK_MANAGER1 // no custom
#endif

#endif//v3.7.0 

#endif


/////////////////
// FLOPPY: DMA //
/////////////////

#if defined(SS_DMA) // this is the DMA as used for disk operation

//#define SS_DMA_ADDRESS // enforcing order for write (no use?)
#define SS_DMA_ADDRESS_EVEN

#if SSE_VERSION>=370
#define SS_DMA_DOUBLE_FIFO // works but overkill  //3.7.0 define then remove def?
#endif

#if SSE_VERSION>=370
#define SS_DMA_DRQ
#endif

//#define SS_DMA_DELAY // works but overkill 3.7.0 -> use generic floppy event?
#define SS_DMA_COUNT_CYCLES
#define SS_DMA_FDC_ACCESS
//#define SS_DMA_FDC_READ_HIGH_BYTE // like pasti, 0  
#define SS_DMA_FIFO // first made for CAPS 
#define SS_DMA_FIFO_NATIVE // then extended for Steem native (not R/W tracks)
#define SS_DMA_FIFO_PASTI // and Pasti
#define SS_DMA_FIFO_READ_ADDRESS // save some bytes...
#define SS_DMA_FIFO_READ_ADDRESS2 // save 4 bytes more...
#define SS_DMA_IO
#define SS_DMA_READ_STATUS 
#define SS_DMA_SECTOR_COUNT
#define SS_DMA_WRITE_CONTROL
///#ifdef SS_DEBUG //will be useful for release version too...
#define SS_DMA_TRACK_TRANSFER //add one var...
#///endif
#endif


///////////////////
// FLOPPY: DRIVE //
///////////////////


#if defined(SS_DRIVE)

#define SS_DRIVE_BYTES_PER_ROTATION
#ifdef SS_DEBUG
#define SS_DRIVE_COMPUTE_BOOT_CHECKSUM // to mod a boot sector//v3.6.0
#endif
#define SS_DRIVE_CREATE_ST_DISK_FIX // from Petari//v3.6.0
// one or the other:
//#define SS_DRIVE_EMPTY_VERIFY_LONG // GEM
#define SS_DRIVE_EMPTY_VERIFY_TIME_OUT //GEM
#define SS_DRIVE_EMPTY_VERIFY_TIME_OUT2 //3.6.1 motor led still on
#define SS_DRIVE_MOTOR_ON
#define SS_DRIVE_MOTOR_ON_IPF//3.6.1 TODO
#define SS_DRIVE_MULTIPLE_SECTORS
#define SS_DRIVE_READ_ADDRESS_TIMING
#define SS_DRIVE_11_SECTORS //TODO acopy13A?
#define SS_DRIVE_READ_TRACK_11
#define SS_DRIVE_READ_TRACK_11B //Gap 4: 1
#define SS_DRIVE_READ_TRACK_11C //Gap 5
#define SS_DRIVE_RW_SECTOR_TIMING // start of sector
#define SS_DRIVE_RW_SECTOR_TIMING2 // end of sector (hack)

#if SSE_VERSION>=370
#define SS_DRIVE_RW_SECTOR_TIMING3 //test v3.7.0 use ID... (?)
#endif

#define SS_DRIVE_SINGLE_SIDE //3.6.0 
#if SSE_VERSION>=370
#define SS_DRIVE_SINGLE_SIDE_RND//3.7.0//TODO random data instead
#else
#define SS_DRIVE_SINGLE_SIDE_IPF //3.6.0
#define SS_DRIVE_SINGLE_SIDE_NAT1 //3.6.0
#define SS_DRIVE_SINGLE_SIDE_PASTI //3.6.0
#endif
#if defined(WIN32) //TODO Unix
#define SS_DRIVE_SOUND // heavily requested, delivered!//3.6.0
//#define SS_DRIVE_SOUND_CHECK_SEEK_VBL
#if defined(SS_DRIVE_SOUND)
#define SS_DRIVE_SOUND_SINGLE_SET // drive B uses sounds of A
//#define SS_DRIVE_SOUND_EDIT // 1st beta soundset
#define SS_DRIVE_SOUND_EPSON // current samples=Epson
#define SS_DRIVE_SOUND_EMPTY // different 3.6.1 (none)
#define SS_DRIVE_SOUND_IPF // fix 3.6.1
#define SS_DRIVE_SOUND_VOLUME // logarithmic 
#endif//drive sound
#endif//win32
#define SS_DRIVE_SPIN_UP_TIME
#define SS_DRIVE_SPIN_UP_TIME2 // more precise
#define SS_DRIVE_SWITCH_OFF_MOTOR //hack//3.6.0
//#define SS_DRIVE_WRITE_TRACK_11//TODO
#define SS_DRIVE_WRONG_IMAGE_ALERT//3.6.1
#endif


/////////////////
// FLOPPY: FDC //
/////////////////

#if defined(SS_FDC)

#define SS_FDC_ACCURATE // bug fixes and additions for ADAT mode (v3.5)

#if defined(SS_FDC_ACCURATE) 

#define SS_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari
#define SS_FDC_CHANGE_SECTOR_WHILE_BUSY // from Hatari or Kryoflux
#define SS_FDC_CHANGE_TRACK_WHILE_BUSY // from Hatari or Kryoflux
#define SS_FDC_FORCE_INTERRUPT // Panzer rotation using $D4
#define SS_FDC_FORCE_INTERRUPT_RESET_D4 // not clear
#define SS_FDC_HEAD_SETTLE
#if SSE_VERSION>=370
#define SS_FDC_IDFIELD_IGNORE_SIDE 
#endif
#define SS_FDC_INDEX_PULSE1 // 4ms
#define SS_FDC_INDEX_PULSE2 // read STR
#define SS_FDC_MOTOR_OFF 
#define SS_FDC_PRECISE_HBL
#define SS_FDC_READ_ADDRESS_UPDATE_SR
#define SS_FDC_RESET
#define SS_FDC_RESTORE
#define SS_FDC_SEEK
#define SS_FDC_SPIN_UP_AGENDA
#define SS_FDC_SPIN_UP_STATUS
#define SS_FDC_STEP

#ifdef SS_PSG
#define SS_FDC_IGNORE_WHEN_NO_DRIVE_SELETED // from Hatari
#define SS_FDC_INDEX_PULSE_COUNTER
#ifdef SS_FDC_MOTOR_OFF
#define SS_FDC_MOTOR_OFF_COUNT_IP
#endif
#endif

#ifdef SS_DRIVE
#define SS_FDC_VERIFY_AGENDA 
#endif

#endif//SS_FDC_ACCURATE

#ifdef SS_DEBUG
#define SS_FDC_TRACE_IRQ
#define SS_FDC_TRACE_STATUS //spell out status register
#endif

#endif


/////////////////
// FLOPPY: IPF //
/////////////////

#if defined(SS_IPF)
#ifdef SS_DRIVE
#define SS_DRIVE_IPF1 // 3.6.1 know image type (not used yet TODO)
#endif

// those switches were used for development, normally they ain't necessary
//#define SS_IPF_CPU // total sync, works but overkill
//#define SS_IPF_RUN_PRE_IO 
//#define SS_IPF_RUN_POST_IO
#define SS_IPF_ASSOCIATE // extension may be associated with Steem
#define SS_IPF_CTRAW//2nd type of file recognised by caps v5.0 (?) //3.6.1
#define SS_IPF_CTRAW_REV //3.6.1 manage different rev data
//#define SS_IPF_KFSTREAM//3rd type of file recognised by caps v5.1 (?) //3.6.1
//#define SS_IPF_DRAFT//4th type of file recognised by caps v5.1 (?) //3.6.1
//#define SS_IPF_OSD // for the little box at reset - silly?
#define SS_IPF_RESUME//3.6.1 TODO
//#define SS_IPF_SAVE_WRITES //TODO?
#define SS_IPF_TRACE_SECTORS // show sector info

#endif


///////////////////
// FLOPPY: PASTI //
///////////////////

#if defined(SS_PASTI) && defined(WIN32)

#define SS_PASTI_ALWAYS_DISPLAY_STX_DISKS
#define SS_PASTI_AUTO_SWITCH
#define SS_PASTI_ONLY_STX  // experimental! optional
#define SS_PASTI_ONLY_STX_HD //v3.6.0
#define SS_PASTI_NO_RESET 
#define SS_PASTI_ON_WARNING // mention in disk manager title
#define SS_PASTI_ON_WARNING2 // v3.6.1 refactoring

#endif


/////////////////
// FLOPPY: PSG //
/////////////////

#if defined(SS_PSG)

#if SSE_VERSION>=370
#define SS_PSG1 // selected drive, side as variables
#define SS_PSG2 // adpat drive motor status to FDC STR at each change
#endif

#ifdef SS_DEBUG
#define SS_PSG_REPORT_DRIVE_CHANGE // as FDC trace
#endif

#endif


/////////////////
// FLOPPY: SCP //
/////////////////

#if defined(SS_SCP)

#endif


#endif//SS_FLOPPY


//////////
// GLUE //
////////// 





//////////
// IKBD //
//////////

#if defined(SS_IKBD)

#define SS_IKBD_6301 // HD6301 true emu, my pride!

#define SS_IKBD_TRACE_CPU_READ

#define SS_IKBD_TRACE_CPU_READ2 //beware polling


#if defined(SS_IKBD_6301) 
// Switches for Sim6301 (modded 3rd party) are here too
#define SS_IKBD_6301_ADJUST_CYCLES // stay in sync! (check with clock)
#define SS_IKBD_6301_CHECK_COMMANDS
#define SS_IKBD_6301_CHECK_IREG_RO // some registers are read-only
#define SS_IKBD_6301_CHECK_IREG_WO // some registers are write-only
#define SS_IKBD_6301_DISABLE_BREAKS // to save 64k (we still consume 64k)
#define SS_IKBD_6301_DISABLE_CALLSTACK // to save 3k on the PC stack
//#define SS_IKBD_6301_RUN_CYCLES_AT_IO // overkill
#define SS_IKBD_6301_MOUSE_ADJUST_SPEED //poor attempt
#define SS_IKBD_6301_MOUSE_MASK // Jumping Jackson auto//v3.6.0
//#define SS_IKBD_MOUSE_OFF_JOYSTICK_EVENT // hardware quirk? (hack)
#define SS_IKBD_6301_RUN_IRQ_TO_END // hack around Sim6xxx's working
#define SS_IKBD_6301_SET_TDRE
#define SS_IKBD_6301_TIMER_FIX // not sure there was a problem
//#define SS_IKBD_6301_TRACE // defined in SS_DEBUG
#if defined(SS_IKBD_6301_TRACE)
#ifdef SS_BETA
//#define SS_IKBD_6301_DISASSEMBLE_ROM 
//#define SS_IKBD_6301_DUMP_RAM
#endif
//#define SS_IKBD_6301_TRACE_SCI_RX
//#define SS_IKBD_6301_TRACE_SCI_TX
//#define SS_IKBD_6301_TRACE_INT_TIMER
//#define SS_IKBD_6301_TRACE_INT_SCI
//#define SS_IKBD_6301_TRACE_STATUS
//#define SS_IKBD_6301_TRACE_KEYS
//#define SS_IKBD_6301_TRACE_WRITES
#endif
#endif//#if defined(SS_IKBD_6301)

#endif


///////////////
// INTERRUPT //
///////////////
// temp there ought to be a better structure
#if defined(SS_INTERRUPT)

#define SS_INT_HBL 
#define SS_INT_JITTER // from Hatari 
#define SS_INT_VBL 


#if defined(SS_INT_HBL)

#if !defined(SS_DEBUG_TRACE_CONTROL)
#define SS_INT_OSD_REPORT_HBI
#endif

#define SS_INT_HBL_IACK_FIX // from Hatari - BBC52
#define SS_INT_HBL_ONE_FUNCTION // 3.6.1 remove event_hbl()
#endif

#if defined(SS_INT_JITTER) && defined(SS_INT_HBL)
#define SS_INT_JITTER_HBL //3.6.0 defined again for 3615HMD
#endif

#if defined(SS_INT_VBL)
#define SS_INT_VBL_IACK
#define SS_INT_VBL_INLINE 
#ifdef SS_BETA
//#define SS_INT_VBI_START // generally working now
#endif
#endif

#if defined(SS_INT_JITTER) && defined(SS_INT_VBL) && defined(SS_STF)
#define SS_INT_JITTER_RESET//3.6.1
#define SS_INT_JITTER_VBL // STF
#define SS_INT_JITTER_VBL2 //3.6.1
//#define SS_INT_JITTER_VBL_STE // STF + STE 
#endif

#if defined(SS_STF) && defined(SS_INT_VBL) && !defined(SS_INT_VBI_START)
#define SS_INT_VBL_STF // more a hack but works
#endif

#endif


/////////
// MFP //
/////////

#if defined(SS_MFP)

#define SS_MFP_IACK_LATENCY
//#define SS_MFP_IRQ_DELAY // V8MS but breaks Sinfull Sinuses
//#define SS_MFP_IRQ_DELAY2 // Audio Artistic Demo (no!)
//#define SS_MFP_IRQ_DELAY3 // V8MS from Hatari 3.6.1:undef
#define SS_MFP_PATCH_TIMER_D // from Hatari, we keep it for performance
#define SS_MFP_RATIO // change the values of CPU & MFP freq!
#define SS_MFP_RATIO_HIGH_SPEED //fix v3.6.1
#define SS_MFP_RATIO_PRECISION // for short timers
#define SS_MFP_RATIO_STE // measured (by Steem Authors) for STE?
#define SS_MFP_RATIO_STF // theoretical for STF
#define SS_MFP_RATIO_STE_AS_STF // change STF + STE (same for both)
#define SS_MFP_RS232 //one little anti-hang bugfix
#define SS_MFP_TIMER_B 
#if defined(SS_MFP_TIMER_B)
#define SS_MFP_TIMER_B_AER // earlier trigger (from Hatari)
#define SS_MFP_TIMER_B_NO_WOBBLE // does it fix anything???
#define SS_MFP_TIMER_B_RECOMPUTE //3.6.1
#endif
#define SS_MFP_TIMERS_BASETIME // "vbl" instead of "mfptick"
#define SS_MFP_TxDR_RESET // they're not reset according to doc
#define SS_MFP_WRITE_DELAY1//3.6.1 (Audio Artistic)

#endif


//////////
// MIDI //
//////////

#if defined(SS_MIDI)

//#define SS_MIDI_TRACE_BYTES_IN
//#define SS_MIDI_TRACE_BYTES_OUT

#endif


/////////
// MMU //
/////////

#if defined(SS_MMU)

//TODO, properly separate MMU, GLUE, Shifter functions, first proper C++ structure

#if defined(WIN32) //TODO Unix
#define SS_MMU_256K // Atari 260 ST//v3.6.0
#define SS_MMU_2560K // some STE with 2MB memory upgrade//v3.6.0
#endif

//#define SS_MMU_NO_CONFUSION // Diagnostic cartridge: don't define (v3.5.2)
#ifdef SS_STF
#define SS_MMU_WAKE_UP
#endif
#define SS_MMU_WRITE // programs in RAM may write in the MMU

#if defined(SS_MMU_WAKE_UP)
#if defined(SS_CPU)
//#define SS_MMU_WAIT_STATES // extreme, replaces rounding to 4, TODO
#endif
#define SS_MMU_WAKE_UP_DL // Dio's DE-LOAD delay
#if !defined(SS_MMU_WAKE_UP_DL)
#define SS_MMU_WAKE_UP_0_BYTE_LINE
//#define SS_MMU_WAKE_UP_IO_BYTES_R  // breaks too much (read SDP) TODO
//#define SS_MMU_WAKE_UP_IO_BYTES_W // no too radical
//#define SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY // adapt cycles for shifter write
#define SS_MMU_WAKE_UP_IOR_HACK 
#define SS_MMU_WAKE_UP_IOW_HACK 
#endif

#define SS_MMU_WAKE_UP_PALETTE_STE // render +1 cycle (pixel) in state 2
//#define SS_MMU_WAKE_UP_READ_SDP
#define SS_MMU_WAKE_UP_RESET_ON_SWITCH_ST
#define SS_MMU_WAKE_UP_RIGHT_BORDER
#define SS_MMU_WAKE_UP_SHIFTER_TRICKS // Adapt limit values based on Paolo's table
//#define SS_MMU_WAKE_UP_STE_STATE2 // STE in same state2 as STF (no)
#define SS_MMU_WAKE_UP_VERTICAL_OVERSCAN // ijor's wakeup.tos test
#define SS_MMU_WAKE_UP_VERTICAL_OVERSCAN1 //3.6.3 defaults to WU1
//#define SS_MMU_WAKE_UP_WRITE_SDP
#endif// SS_MMU_WAKE_UP


#endif


/////////
// OSD //
/////////

#if defined(SS_OSD)

#ifdef SS_DEBUG
#define SS_OSD_DEBUG_MESSAGE // pretty handy

#if defined(SS_DEBUG_FAKE_IO)
#define SS_OSD_CONTROL //3.6.1
#else
#define SS_OSD_DEBUG_MESSAGE_FREQ // tell when 60hz (?)
#endif//fakeio
#endif//debug
#define SS_OSD_DRIVE_INFO // cool! (v3.5.1)
#define SS_OSD_DRIVE_INFO2 // no SR when fast
//#define SS_OSD_DRIVE_INFO_OSD_PAGE
#define SS_OSD_DRIVE_INFO_SSE_PAGE
#define SS_OSD_DRIVE_LED
#define SS_OSD_DRIVE_LED2 // simpler approach
#define SS_OSD_DRIVE_LED3 // remove useless variable
//#define SS_OSD_LOGO1 // suppress former logo (=3.2 in grx)
//#define SS_OSD_LOGO2 //hack (temp)
#define SS_OSD_LOGO3 // nicer
#define SS_OSD_SCROLLER_CONTROL
#define SS_OSD_SCROLLER_DISK_IMAGE //TODO sometimes wrong name

#endif


/////////
// SDL //
/////////

#if defined(SS_SDL)
// tests - but shouldn't it be a pure SDL build? 
//#if SSE_VERSION<360
#define SS_SDL_DEACTIVATE // support planned in v3.6 //nope...
//#endif
//#define SS_SDL_KEEP_DDRAW_RUNNING // normally not!

#endif


/////////////
// SHIFTER //
/////////////

#if defined(SS_SHIFTER)

#define SS_SHIFTER_IO // move blocks from ior, iow

///#define SS_SHIFTER_RENDER_SYNC_CHANGES//don't until debug

#define SS_SHIFTER_SDP // SDP=shifter draw pointer
#define SS_SHIFTER_TRICKS  // based on Steem system, extended

#if defined(SS_SHIFTER_TRICKS)

#define SS_SHIFTER_0BYTE_LINE
#if defined(SS_SHIFTER_0BYTE_LINE) // former switch approach
//#define SS_SHIFTER_0BYTE_LINE_RES_END //No Buddies Land
//#define SS_SHIFTER_0BYTE_LINE_RES_HBL //Beyond/Pax Plax Parallax
//#define SS_SHIFTER_0BYTE_LINE_RES_START //Nostalgic-O/Lemmings
//#define SS_SHIFTER_0BYTE_LINE_SYNC //Forest
//#define SS_SHIFTER_0BYTE_LINE_SYNC2 // loSTE screens

//#define SS_SHIFTER_0BYTE_LINE_RES_END_THRESHOLD//Hackabonds Demo not WS1

#endif//0-byte line

#define SS_SHIFTER_4BIT_SCROLL //Let's do the Twist again
#define SS_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK
#define SS_SHIFTER_60HZ_OVERSCAN //Leavin' Terramis
#define SS_SHIFTER_END_OF_LINE_CORRECTION // correct +2, -2 lines 
#define SS_SHIFTER_FIX_LINE508_CONFUSION // hack at recording shifter event

//#define SS_SHIFTER_LEFT_OFF_THRESHOLD//Hackabonds Demo not WS1

#define SS_SHIFTER_LEFT_OFF_60HZ // 24 bytes!
#define SS_SHIFTER_LEFT_OFF_TEST_BEFORE_HBL // for Riverside
#define SS_SHIFTER_LINE_MINUS_106_BLACK // loSTE screens
#define SS_SHIFTER_LINE_PLUS_2_STE // hack?
#define SS_SHIFTER_LINE_PLUS_2_TEST // loSTE screens
#define SS_SHIFTER_LINE_PLUS_2_THRESHOLD //Forest
//#define SS_SHIFTER_LINE_PLUS_2_POST_TOP_OFF // Panic
#define SS_SHIFTER_LINE_PLUS_20 // 224 byte scanline STE only
#define SS_SHIFTER_LINE_PLUS_20_SHIFT // for Riverside
#define SS_SHIFTER_MED_OVERSCAN // BPOC
#define SS_SHIFTER_MED_OVERSCAN_SHIFT // No Cooper/greetings
#define SS_SHIFTER_NON_STOPPING_LINE // Enchanted Land
//#define SS_SHIFTER_PALETTE_BYTE_CHANGE //Golden Soundtracker
#define SS_SHIFTER_PALETTE_NOISE //UMD8730 STF
#define SS_SHIFTER_PALETTE_TIMING //Overscan Demos #6
#define SS_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE //beeshift0
#define SS_SHIFTER_STATE_MACHINE //v3.5.4, simpler approach and WS-aware
#define SS_SHIFTER_STE_MED_HSCROLL // Cool STE
#define SS_SHIFTER_STE_HI_HSCROLL
#define SS_SHIFTER_STE_HSCROLL_LEFT_OFF //MOLZ/Spiral
#define SS_SHIFTER_STE_VERTICAL_OVERSCAN //RGBeast
#define SS_SHIFTER_UNSTABLE // DoLB, Omega, Overdrive/Dragon, Beeshift
#ifdef SS_SHIFTER_UNSTABLE
//TODO swtiches for Dragon, etc.
#if SSE_VERSION>353
//#define SS_SHIFTER_LINE_PLUS_2_ON_PRELOAD3 // DSOS STE //MFD?
#endif
#define SS_SHIFTER_UNSTABLE_DOLB
#define SS_SHIFTER_UNSTABLE_OMEGA
#define SS_SHIFTER_HI_RES_SCROLLING // Beeshift2
#define SS_SHIFTER_MED_RES_SCROLLING // Beeshift
#define SS_SHIFTER_PANIC // funny effect, interleaved border bands
#define SS_SHIFTER_REMOVE_USELESS_VAR //6.3.1
#define SS_SHIFTER_VERTICAL_OPTIM1 //avoid useless tests
#endif
//#define SS_SHIFTER_UNSTABLE_LEFT_OFF // DoLB, Omega, Overdrive/Dragon old hack

#if defined(SS_STF)
#define SS_STF_VERTICAL_OVERSCAN
#endif

#endif//#if defined(SS_SHIFTER_TRICKS)

#if defined(SS_SHIFTER_SDP)
#define SS_SHIFTER_SDP_READ
#define SS_SHIFTER_SDP_WRITE
#define SS_SHIFTER_SDP_WRITE_ADD_EXTRA
#define SS_SHIFTER_SDP_WRITE_LOWER_BYTE
//#define SS_SHIFTER_SDP_TRACE 
//#define SS_SHIFTER_SDP_TRACE2
//#define SS_SHIFTER_SDP_TRACE3 // report differences with Steem v3.2 
#endif

#if defined(SS_HACKS)
// most hacks concern SDP, there's room for improvement
#define SS_SHIFTER_SDP_WRITE_DE_HSCROLL
#define SS_SHIFTER_SDP_WRITE_MIDDLE_BYTE // stable
#define SS_SHIFTER_ARMADA_IS_DEAD // no shift contrary to Big Wobble
#define SS_SHIFTER_BIG_WOBBLE // Big Wobble shift
#define SS_SHIFTER_DANGEROUS_FANTAISY // Dangerous Fantaisy credits flicker
#define SS_SHIFTER_DOLB_SHIFT1 // based on "unstable overscan"
//#define SS_SHIFTER_DOLB_SHIFT2 // based on cycle of R0
//#define SS_SHIFTER_DRAGON1 // confused shifter, temp hack
#define SS_SHIFTER_PACEMAKER // Pacemaker credits flickering line
#define SS_SHIFTER_SCHNUSDIE // Reality is a Lie/Schnusdie overscan logo
#define SS_SHIFTER_TCB // Swedish New Year Demo/TCB SDP (60hz)
#define SS_SHIFTER_TEKILA // Delirious 4/Tekila
#define SS_SHIFTER_XMAS2004 // XMas 2004 by Paradox shift
#endif

#if defined(SS_DEBUG) 
#ifdef SS_BETA
//#define SS_SHIFTER_DRAW_DBG  // totally bypass CheckSideOverscan() & Render()
#endif


//#define SS_SHIFTER_IOR_TRACE // specific, not "log"
//#define SS_SHIFTER_IOW_TRACE // specific, not "log"
#if !defined(SS_DEBUG_TRACE_IDE)
#define SS_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif

#if !defined(SS_DEBUG_TRACE_CONTROL)
//#define SS_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN
//#define SS_SHIFTER_VERTICAL_OVERSCAN_TRACE
#endif

//#define SS_SHIFTER_STEEM_ORIGINAL // only for debugging/separate blocks

#else
#define draw_check_border_removal Shifter.CheckSideOverscan
#define draw_scanline_to(cycles_since_hbl) Shifter.Render(cycles_since_hbl)
#endif

#endif


///////////
// SOUND //
///////////

#if defined(SS_SOUND)

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

#define SS_SOUND_APART_BUFFERS //TODO, one for PSG one for DMA, but Microwire?

#define SS_SOUND_CHANGE_TIME_METHOD_DELAY //detail

#define SS_SOUND_FILTER_STF // a very simple filter

#define SS_SOUND_INLINE // macro->inline, easier for my tests, but hard to do

#define SS_SOUND_MICROWIRE // volume, balance, bass & treble, primitive DSP
#define SS_SOUND_MICROWIRE_MIXMODE//3.6.3

#define SS_SOUND_MICROWIRE_WRITE_LATENCY // as documented (3.XX?)
#define SS_SOUND_NO_EXTRA_PER_VBL //compensating hack? changes what?

#define SS_SOUND_OPTIMISE
//#define SS_SOUND_OPTION_DISABLE_DSP // not needed if no anti-aliasing
#define SS_SOUND_RECOMMEND_OPTIONS

#ifdef WIN32
//#define SS_SOUND_SKIP_DSOUND_TEST
#endif

#define SS_SOUND_VOL // -6db for PSG chipsound (using DSP)
#define SS_SOUND_VOL_LOGARITHMIC // more intuitive setting
#define SS_SOUND_FILTER_STE // same very simple filter as for STF

#ifdef UNIX
#define SS_RTAUDIO_LARGER_BUFFER //simplistic attempt
#endif


#endif


/////////////////////////////////
// ST Model (various STF, STE) //
/////////////////////////////////

#if defined(SS_STF)
//note all those switches have been useful for SS_SHIFTER_PALETTE_NOISE
#define SS_STF_0BYTE
#define SS_STE_2MB // auto make RAM 2MB, more compatible
#define SS_STF_1MB // auto make RAM 1MB, more compatible
#define SS_STF_8MHZ // 3.6.1 - for Panic study!
#define SS_STF_BLITTER
#define SS_STF_DMA
#define SS_STF_HSCROLL
#define SS_STF_LEFT_OFF 
#define SS_STF_LINEWID
#ifdef WIN32
#define SS_STF_MATCH_TOS // select a compatible TOS for next reset
#endif
#define SS_STF_MEGASTF // blitter in STF (could be useful?) + 4MB!
#define SS_STF_MMU_PREFETCH
#define SS_STF_PADDLES
#define SS_STF_PAL
#define SS_STF_SDP
#define SS_STF_SHIFTER_IOR
#define SS_STF_VBASELO
#define SS_STF_VERT_OVSCN

#endif


///////////////
// STRUCTURE //
///////////////

#if defined(SS_STRUCTURE)

#define SS_STRUCTURE_CPU_POKE_NOINLINE //little detail

#define SS_STRUCTURE_DECLA // decla.h files

#if defined(LEGACY_BUILD)
#define SS_STRUCTURE_NEW_H_FILES // for all features
#endif

#if !defined(LEGACY_BUILD)
#define SS_STRUCTURE_NEW_H_FILES // necessary
#define SS_STRUCTURE_SSE_OBJ // try to have all separate SSE objects
#endif

#define SS_STRUCTURE_DMA_INC_ADDRESS
#if defined(LEGACY_BUILD)
//#define SS_STRUCTURE_INFO // just telling files included in modules
#endif
#define SS_STRUCTURE_IOR

#if defined(SS_STRUCTURE_NEW_H_FILES)
// those were dev switches (TODO remove, just use SS_STRUCTURE_DECLA)
#define SS_STRUCTURE_ACC_H
#define SS_STRUCTURE_ARCHIVE_H
#define SS_STRUCTURE_BLITTER_H
#define SS_STRUCTURE_BOILER_H
#define SS_STRUCTURE_CPU_H
#define SS_STRUCTURE_D2_H
#define SS_STRUCTURE_DATALOADSAVE_H
#define SS_STRUCTURE_DEBUGEMU_H
#define SS_STRUCTURE_DIRID_H
#define SS_STRUCTURE_DISKMAN_H
#define SS_STRUCTURE_DISPLAY_H
#define SS_STRUCTURE_DRAW_H
#define SS_STRUCTURE_DWINEDIT_H
#define SS_STRUCTURE_EMULATOR_H
#define SS_STRUCTURE_FDC_H
#define SS_STRUCTURE_FLOPPYDRIVE_H
#define SS_STRUCTURE_GUI_H
//#define SS_STRUCTURE_HARDDISKMAN_H
#define SS_STRUCTURE_HDIMG_H
//#define SS_STRUCTURE_HISTORYLIST_H
#define SS_STRUCTURE_IKBD_H
#define SS_STRUCTURE_INFOBOX_H //systematic use of .decla.h
#define SS_STRUCTURE_INITSOUND_H
#define SS_STRUCTURE_IOLIST_H
#define SS_STRUCTURE_IORW_H
#define SS_STRUCTURE_KEYTABLE_H
#define SS_STRUCTURE_LOADSAVE_H
#define SS_STRUCTURE_MACROS_H
#define SS_STRUCTURE_MEMBROWSER_H
#define SS_STRUCTURE_MFP_H
#define SS_STRUCTURE_MIDI_H
#define SS_STRUCTURE_MRSTATIC_H
#define SS_STRUCTURE_NOTIFYINIT_H
#define SS_STRUCTURE_ONEGAME_H
#define SS_STRUCTURE_OPTIONS_H
#define SS_STRUCTURE_OSD_H
#define SS_STRUCTURE_PALETTE_H
//#define SS_STRUCTURE_PATCHESBOX_H// nothing to do?
#define SS_STRUCTURE_PSG_H
//#define SS_STRUCTURE_RESET_H// nothing to do?
#define SS_STRUCTURE_RS232_H
#define SS_STRUCTURE_RUN_H
//#define SS_STRUCTURE_SCREENSAVER_H// nothing to do?
#define SS_STRUCTURE_SHORTCUTBOX_H
#define SS_STRUCTURE_STEEMH_H
#define SS_STRUCTURE_STEMDIALOGS_H
#define SS_STRUCTURE_STEMDOS_H
#define SS_STRUCTURE_STJOY_H
#define SS_STRUCTURE_STPORTS_H
#define SS_STRUCTURE_TRACE_H
#define SS_STRUCTURE_TRANSLATE_H
#endif//#if defined(SS_STRUCTURE_NEW_H_FILES)


#if defined(SS_STRUCTURE_SSE_OBJ)
// We begin with our SSE additions. Notice that it already slows down compiling
#define SS_STRUCTURE_SSE6301_OBJ//3.6.0
#define SS_STRUCTURE_SSECPU_OBJ//3.6.0
#define SS_STRUCTURE_SSEDEBUG_OBJ//3.6.0
#define SS_STRUCTURE_SSEFLOPPY_OBJ//3.6.1
#define SS_STRUCTURE_SSEFRAMEREPORT_OBJ//3.6.1
//#define SS_STRUCTURE_SSE_INTERRUPT_OBJ//skip for now...
#define SS_STRUCTURE_SSESHIFTER_OBJ//3.6.1
#define SS_STRUCTURE_SSESTF_OBJ//3.6.1
#define SS_STRUCTURE_SSESTF_VIDEO//3.6.1
#endif//#if defined(SS_STRUCTURE_SSE_OBJ)



#endif


/////////////
// TIMINGS //
/////////////

#if defined(SS_TIMINGS)

#define SS_TIMINGS_MS_TO_HBL

#endif


/////////
// TOS //
/////////
#if defined(SS_TOS)

#ifdef SS_HACKS
#define SS_TOS_PATCH106 // a silly bug, a silly hack
#define SS_TOS_STE_FAST_BOOT //from hatari
#endif

#define SS_TOS_FILETIME_FIX //from Petari//3.6.0
#define SS_TOS_GEMDOS_NOINLINE//3.6.1
#define SS_TOS_GEMDOS_PEXEC6 //3.6.1 ReDMCSB 100% in TOS104

#define SS_TOS_STRUCT//3.6.1

#define SS_TOS_GEMDOS_VAR1 //various unimportant fixes 3.6.1

//#define SS_TOS_NO_INTERCEPT_ON_RTE1 // fix (not) Megamax C on ReDMCSB//3.6.0
#if defined(SS_TOS_STRUCT)
#define SS_TOS_NO_INTERCEPT_ON_RTE2 //3.6.1//try to be less radical... ReDMCSB 50% in TOS102
#ifdef WIN32
#define SS_TOS_SNAPSHOT_AUTOSELECT//3.6.1
#define SS_TOS_SNAPSHOT_AUTOSELECT2//3.6.1//with refactoring
#define SS_TOS_SNAPSHOT_AUTOSELECT3//3.6.1//options.cpp uses refactoring
#endif//win32
#endif
#ifdef SS_DEBUG
#define SS_TOS_DONT_TRACE_3F//read file//3.6.1
#define SS_TOS_DONT_TRACE_40//write file//3.6.1
#define SS_TOS_DONT_TRACE_42//seek file//3.6.1
#define SS_TOS_TRACE_CONOUT//3.6.1
#endif

#define SS_TOS_WARNING1 // version/ST type//3.6.1

// fixes by other people: //TODO, another big category?
#define SSE_AVTANDIL_FIX_001 // Russin TOS number
#define SSE_MEGAR_FIX_001 // intercept GEM in extended resolution

#endif


////////////////
// UNIX/LINUX // 
////////////////

#if defined(SS_UNIX)

#if !defined(UNIX)
#undef SS_UNIX
#else

#define SS_UNIX_OPTIONS_SSE_ICON
#define SS_UNIX_STATIC_VAR_INIT //odd

#if defined(SS_DEBUG)
#define SS_UNIX_TRACE // TRACE into the terminal (if it's open?)
#define SS_DEBUG_TRACE_FILE
#endif
#endif

#endif


/////////////
// VARIOUS //
/////////////

#define NO_RAR_SUPPORT // I removed the library, so it's unconditional

#if defined(SS_VARIOUS)
#define SS_VAR_ASSOCIATE
#ifdef SS_VAR_ASSOCIATE
#define SS_VAR_ASSOCIATE_CU // current user, not root
#define SS_VAR_MAY_REMOVE_ASSOCIATION
#define SS_VAR_NO_ASSOCIATE_STC // cartridge, who uses that?
#define SS_VAR_NO_AUTO_ASSOCIATE_DISK_STS_STC // disk images + STS STC
#define SS_VAR_NO_AUTO_ASSOCIATE_MISC // other .PRG, .TOS...
#endif
#ifdef WIN32
#define SS_VAR_CHECK_SNAPSHOT
#endif
#define SS_VAR_DISK_MANAGER_LONG_NAMES1
#define SS_VAR_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
#define SS_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES // at final save
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SS_VAR_F12 // F12 starts/stops emulation
#endif
#define SS_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen - option?
//#define SS_VAR_HIDE_OPTIONS_AT_START // hack before debugging
#define SS_VAR_INFOBOX0 // enum 3.6.1
#define SS_VAR_INFOBOX1 // SSE internet sites
#define SS_VAR_INFOBOX2 // SSE readme + FAQ
#define SS_VAR_INFOBOX3 // readme text font
#define SS_VAR_INFOBOX4 // readme 80 col 
#define SS_VAR_INFOBOX5 // don't take 64K on the stack!
#define SS_VAR_INFOBOX6 // no cartridge howto 3.6.1
#define SS_VAR_INFOBOX7 // specific hints 3.6.1
//TODO also in unix
#define SS_VAR_KEYBOARD_CLICK // not a sound nor IKBD option
#define SS_VAR_MOUSE_CAPTURE 
#define SS_VAR_MSA_CONVERTER // don't prompt if found
#define SS_VAR_NOTIFY //adding some notify during init
#define SS_VAR_NO_UPDATE // remove all code in relation to updating
#define SS_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah
#define SS_VAR_OPTION_SLOW_DISK // because many people miss it in disk manager
#define SS_VAR_OPTIONS_ICON_VERSION
#define SS_VAR_OPTIONS_REFRESH // 6301, STF... up-to-date with snapshot
//#define SS_VAR_POWERON2 //3.6.1 try other, safer way ;) //3.6.2: not safe either :)
#define SS_VAR_RESET_BUTTON // invert
#define SS_VAR_RESIZE // reduce memory set (int->BYTE etc.)
#define SS_VAR_REWRITE // to conform to what compilers expect (warnings...)
#ifdef WIN32
#define SS_VAR_STATUS_STRING // "status bar" in the icon bar
#define SS_VAR_STATUS_STRING_6301
#define SS_VAR_STATUS_STRING_ADAT
#define SS_VAR_STATUS_STRING_DISK_NAME
#define SS_VAR_STATUS_STRING_DISK_NAME_OPTION
#define SS_VAR_STATUS_STRING_FULL_ST_MODEL//3.6.1
#define SS_VAR_STATUS_STRING_HACKS
#define SS_VAR_STATUS_STRING_IPF
#define SS_VAR_STATUS_STRING_PASTI
#define SS_VAR_STATUS_STRING_VSYNC//3.6.0
#endif
#define SS_VAR_STEALTH // don't tell we're an emulator (option)
#ifdef WIN32
#define SS_VAR_UNRAR // using unrar.dll, up to date
#endif
#define SS_VAR_WINDOW_TITLE


#endif 


///////////
// VIDEO //
///////////

#if defined(SS_VIDEO)

#define SS_VID_ADJUST_DRAWING_ZONE1 // attempt to avoid crash
#define SS_VID_ADJUST_DRAWING_ZONE2
//#define SS_VID_AUTOOFF_DECRUNCHING // bad idea (complaints)
#define SS_VID_BLIT_TRY_BLOCK //?

#define SS_VID_BORDERS // option display size
#if defined(SS_VID_BORDERS)
//#ifdef WIN32 // Unix?
#define SS_VID_BORDERS_412 // 
#define SS_VID_BORDERS_413 // best fit for overscan?
#define SS_VID_BORDERS_416 
#define SS_VID_BORDERS_416_NO_SHIFT
#define SS_VID_BORDERS_416_NO_SHIFT1 //3.6.1: check border on/off
#define SS_VID_BORDERS_BIGTOP // more lines for palette effects
#define SS_VID_BORDERS_LB_DX // rendering-stage trick rather than painful hacks
#define SS_VID_BORDERS_LB_DX1 //3.6.1: check border on/off
//#endif
#endif

#define SS_VID_BPOC // Best Part of the Creation fit display 800 hack
#define SS_VID_CHECK_DDFS // is video card/display capable?
//#define SS_VID_CHECK_DDFS2 //list all modes
#if defined(WIN32) 
#define SS_VID_DIRECT3D // TODO, like in WinUAE there's an option
#define SS_VID_RECORD_AVI //avifile
#endif

#if defined(WIN32) //TODO Unix
#define SS_VID_SAVE_NEO // screenshots in ST Neochrome format
#endif

#if defined(WIN32)
#define SS_VID_3BUFFER // Triple Buffer to remove tearing
#if defined(SS_VID_3BUFFER)
#define SS_VID_3BUFFER_FS // fullscreen: stretch mode only
//#define SS_VID_3BUFFER_NO_VSYNC // for tests: "VSync" is necessary
#define SS_VID_3BUFFER_WIN //windowed mode (necessary for FS)
#endif//SS_VID_3BUFFER
#ifdef SS_BETA 
#define SS_VID_D3D // TODO for v3.7
#endif
#endif

#define SS_VID_FREEIMAGE1 //3.6.3 init library
#define SS_VID_FREEIMAGE2 //3.6.3 always convert pixels
#define SS_VID_FREEIMAGE3 //3.6.3 flip pic
#define SS_VID_FREEIMAGE3 //3.6.3 remove WBMP
#if defined(SS_DELAY_LOAD_DLL)
#define SS_VID_FREEIMAGE4 //3.6.4 use official header
#endif
#define SS_VID_MEMORY_LOST // no message box

#if defined(WIN32)
#define SS_VID_SCANLINES_INTERPOLATED // using stretch mode!
#define SS_VID_SCANLINES_INTERPOLATED_MED
#define SS_VID_SCANLINES_INTERPOLATED_SSE // put option in SSE page
#endif

#define SS_VID_SCREENSHOTS_NUMBER

#if defined(WIN32)
#define SS_VID_VSYNC_WINDOW // no tearing and smooth scrolling also in window
#define SS_VID_VSYNC_WINDOW_CRASH_FIX1//3.6.0
//#define SS_VID_VSYNC_WINDOW_CRASH_FIX2 //safer but annoying
#define SS_VID_VSYNC_WINDOW_CRASH_FIX3 //TODO find something better?
#endif

#if defined(SS_DEBUG) 
//#define SHOW_DRAW_SPEED //was already in Steem
//#define SS_VID_VERT_OVSCN_OLD_STEEM_WAY // only for vertical overscan
#endif

#endif


/////////////////
// ADAPTATIONS //
/////////////////

// not all switches are compatible with each other:

#if !defined(SS_ACIA)
#undef SS_IKBD_6301
#endif

#if !defined(SS_ACIA_REGISTERS)
#undef SS_ACIA_DOUBLE_BUFFER_TX
#endif

#if !defined(SS_ACIA_DOUBLE_BUFFER_RX) || !defined(SS_ACIA_DOUBLE_BUFFER_TX)
#undef SS_ACIA_TDR_COPY_DELAY
#endif

#if !defined(SS_DEBUG) || !defined(SS_OSD_DEBUG_MESSAGE)
#undef SS_CPU_PREFETCH_TRACE 
#undef SS_CPU_TRACE_DETECT 
#endif

#if !defined(SS_DMA)
#undef SS_STRUCTURE_DMA_INC_ADDRESS
#endif

#if !defined(SS_DMA_FIFO)
#undef SS_IPF
#undef SS_IPF_OSD
#endif

#if !defined(SS_FDC)
#undef SS_OSD_DRIVE_INFO2
#endif

#if !defined(SS_DMA) || !defined(SS_FDC)
#undef SS_OSD_DRIVE_LED2
#undef SS_OSD_DRIVE_LED3
#endif

#if !defined(SS_FLOPPY)
#undef SS_VAR_STATUS_STRING_IPF
#undef SS_VAR_STATUS_STRING_ADAT
#endif

#if !defined(SS_IKBD)
#undef SS_ACIA_DOUBLE_BUFFER_RX
#undef SS_ACIA_DOUBLE_BUFFER_TX

#endif

#if !defined(SS_MFP)
#undef SS_FDC_PRECISE_HBL
#endif

#if defined(SS_OSD_LOGO3)
#undef SS_OSD_LOGO
#undef SS_OSD_LOGO2
#endif

#if defined(SS_OSD_LOGO2)
#undef SS_OSD_LOGO3
#endif


#if !defined(SS_DRIVE)
#undef SS_PASTI_ONLY_STX 
#endif

#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
#undef SS_MMU_WAKE_UP_IO_BYTES_W
#endif

#if !defined(SS_OSD_SCROLLER_CONTROL)
#undef SS_VAR_SCROLLER_DISK_IMAGE
#endif

#if !defined(SS_SHIFTER)
#undef SS_CPU_3615GEN4_ULM
#undef SS_CPU_CHECK_VIDEO_RAM_B
#undef SS_CPU_CHECK_VIDEO_RAM_L
#undef SS_CPU_CHECK_VIDEO_RAM_W
#undef SS_MMU_WAKE_UP_IO_BYTES_R
#undef SS_MMU_WAKE_UP_IOR_HACK
#undef SS_MMU_WAKE_UP_IOW_HACK
#undef SS_DEBUG_FRAME_REPORT//same
#endif

#if defined(SS_SHIFTER_UNSTABLE)
#undef SS_SHIFTER_DRAGON1
#endif

#if !defined(SS_STF)
#undef SS_SHIFTER_MED_RES_SCROLLING
#undef SS_SHIFTER_UNSTABLE
#endif

#if !defined(SS_STRUCTURE_NEW_H_FILES)
// mods after we created "decla" files are not duplicated
#undef SS_INT_VBL_IACK
#undef SS_VAR_STATUS_STRING
#undef SS_VAR_STATUS_STRING_ADAT
#undef SS_VAR_STATUS_STRING_DISK_NAME
#undef SS_VID_3BUFFER
#undef SS_VID_3BUFFER_FS
#undef SS_VID_3BUFFER_WIN
#undef SS_VID_BORDERS_BIGTOP
#endif


#if !USE_PASTI
#undef SS_DMA_FIFO_PASTI
#endif

#if defined(SS_UNIX) //temp!
#define SS_SDL_DEACTIVATE
#endif

#if !defined(SS_VID_BORDERS) || !defined(SS_HACKS)
#undef SS_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK
#endif

#if defined(SS_DISK_IMAGETYPE) && !defined(SS_PASTI_ON_WARNING2)//3.6.2
#error("Incompatible switches") // or remove some old code after tested OK
#endif

#if defined(SS_SSE_LEAN_AND_MEAN) //TODO
//6301 not optional...
#undef SS_VID_CHECK_DDFS
#undef SS_VID_RECORD_AVI
#undef SS_VID_SAVE_NEO
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
#define SS_VS2012_INIT		// correct uninitialized variables
#define SS_VS2012_POW		// First arg of pow function should be a real

// Following flags are for successfull compilation with VS 2012
// They only make sense if you are using VS2012 or above
#if _MSC_VER > 1600
#ifndef SS_VS2012_INIT		// required
#define SS_VS2012_INIT
#endif
#ifndef SS_VS2012_POW		// required
#define SS_VS2012_POW
#endif
#define SS_VS2012_WARNINGS	// remove many VS2012 warnings
#define SS_VS2012_DELAYDLL	// remove some directives associated with delayed 
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
