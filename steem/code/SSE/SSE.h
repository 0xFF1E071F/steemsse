#pragma once // VC guard
#ifndef STEVEN_SEAGAL_H // BCC guard
#define STEVEN_SEAGAL_H

/*

Steem Steven Seagal Edition (SSE)
---------------------------------

This is based on the source code for Steem 3.2 as released by Steem authors,
Ant & Russ Hayward.
Current site for this build: 
http://ataristeven.t15.org/Steem.htm
SVN code repository is at http://code.google.com/p/steem-engine/ for
v3.3.0 and at http://sourceforge.net/projects/steemsse/  for later versions.

Added some files to the project. 
- acia.h in 'steem\code'.
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
-Files xxx.decla.h: work in progress

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

*/


/////////////
// VERSION //
/////////////

#if defined(STEVEN_SEAGAL)

#define SS_BETA //title, OSD, plus some testing

#ifdef SS_BETA 
#define SSE_VERSION 353
#define SSE_VERSION_TXT "3.5.3" 
#define WINDOW_TITLE "Steem Beta 3.5.3"
#else // next planned release
#define SSE_VERSION 353 
// check snapshot Version (in LoadSave.h); rc\resource.rc
#define SSE_VERSION_TXT "3.5.3" 
#define WINDOW_TITLE "Steem SSE 3.5.3" //not 'Engine', too long
#endif

#endif


//////////////////
// BIG SWITCHES //
//////////////////

// Any switch may be disabled and it should still compile & run

//TODO: dma, broken

#if defined(STEVEN_SEAGAL)

#define SS_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SS_BLITTER    // spelled BLiTTER by those in the known!
#define SS_CPU        // MC68000 microprocessor
#define SS_CARTRIDGE  // ROM Cartridge slot
#define SS_DMA        // Custom Direct Memory Access chip (disk)
#define SS_DRIVE      // SF314 floppy disk drive
#define SS_FDC        // WD1772 floppy disk controller
#define SS_GLUE       // TODO
#define SS_HACKS      // an option for dubious fixes
#define SS_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#define SS_INTERRUPT  // HBL, VBL  
#define SS_IPF        // CAPS support (IPF disks) 
#define SS_MFP        // MC68901 Multi-Function Peripheral Chip
#define SS_MIDI       // 
#define SS_MMU        // Memory Manager Unit (of the ST, no paging)
#define SS_OSD        // On Screen Display (drive leds, version)
#define SS_PASTI      // Improvements in Pasti support
#define SS_PSG        // YM2149 - for portA first
#define SS_SDL        // Simple DirectMedia Layer (TODO)
#define SS_SHIFTER    // The legendary custom shifter and all its tricks//TODO can't disable
#define SS_SOUND      // YM2149, STE DMA sound, Microwire
#define SS_STF        // switch STF/STE
#define SS_STRUCTURE
#define SS_TIMINGS    
#define SS_TOS        // The Operating System
#define SS_UNIX       // Linux build must be OK too (may lag)
#define SS_VARIOUS    // Mouse capture, keyboard click, unrar...
#define SS_VIDEO      // large borders, screenshot, recording

#endif

// Adapt switches
#ifdef WIN32
#undef SS_UNIX
#endif
#if !defined(SS_DMA) || !defined(SS_FDC)
#undef SS_IPF
#endif
#if defined(SS_UNIX)
#undef SS_IPF//TODO
//#define SS_UNIX_IPF//TODO
#endif

#if SSE_VERSION<360 && !defined(SS_BETA)
#undef SS_SDL 
#endif

#ifdef BCC_BUILD
#define SS_SDL // it's in the makefile now (TODO?)
#endif

//////////
// TEMP //
//////////

#if defined(SS_BETA)
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


//#define SS_ACIA_DOUBLE_BUFFER_RX // only from 6301 (not MIDI) //broken, can't disable
#define SS_ACIA_DOUBLE_BUFFER_TX // only to 6301 (not MIDI)

#ifdef SS_BETA
//#define SS_ACIA_IKBD_SHORTER_TIMINGS //tests
#endif

//#define SS_ACIA_IRQ_DELAY // not defined anymore (v3.5.2), see MFP
//#define SS_ACIA_IRQ_ASSERT_READ_SR //TODO
//#define SS_ACIA_OVERRUN_REPLACE_BYTE // normally no
#define SS_ACIA_REGISTERS // formalising the registers
//#define SS_ACIA_REMOVE_OLD_VARIABLES // TODO
//#define SS_ACIA_DONT_CLEAR_DR //?
#define SS_ACIA_NO_RESET_PIN // don't reset on warm reset
///#define SS_ACIA_TDR_COPY_DELAY // apparently not necessary but see Nightdawn
#if defined(SS_DEBUG)
#define SS_ACIA_TEST_REGISTERS
#endif
#define SS_ACIA_USE_REGISTERS // instead of Steem variables TESTING

#endif


/////////////
// BLITTER //
/////////////

#if defined(SS_BLITTER)

#define SS_BLT_BLIT_MODE_CYCLES // #cycles in BLIT Mode in NOPs, not M68K cycles
#define SS_BLT_BLIT_MODE_INTERRUPT // trigger at once (not after blit phase)
#define SS_BLT_HOG_MODE_INTERRUPT // no interrupt in hog mode
//#define SS_BLT_OVERLAP // TODO
//#define SS_BLT_TIMING // based on a table, but Steem does it better
#define SS_BLT_YCOUNT // 0=65536
//TODO smudge?

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

#define SS_CPU_DIV          // divide like Caesar
#define SS_CPU_EXCEPTION    // crash like Windows 98
#define SS_CPU_FETCH_IO     // fetch like a dog in outer space
#define SS_CPU_3615GEN4_ULM // be from a new generation
#define SS_CPU_MOVE_B       // move like a superstar
#define SS_CPU_MOVE_W       
#define SS_CPU_MOVE_L
#define SS_CPU_POKE         // poke like a C64 (inline in VC6)
#define SS_CPU_PREFETCH     // prefetch like a dog
#define SS_CPU_ROUNDING     // round like a rolling stone
#define SS_CPU_TAS          // 4 cycles fewer if memory

#if defined(SS_CPU_EXCEPTION)

#define SS_CPU_ASSERT_ILLEGAL // assert before trying to execute (not general)
//#define SS_CPU_EXCEPTION_TRACE_PC // reporting all PC (!)
#if defined(SS_BETA) && defined(SS_DEBUG)
//#define SS_CPU_DETECT_STACK_PC_USE // push garbage!!
#endif

#if defined(SS_HACKS) // hacks on: we don't use 'true PC'
#define SS_CPU_HACK_BLOOD_MONEY
//#define SS_CPU_HACK_PHALEON // TODO: why does it work with or without?
#define SS_CPU_HACK_WAR_HELI
#endif

#define SS_CPU_IGNORE_WRITE_0 // for Aladin, may write on 1st word
#define SS_CPU_POST_INC // no post increment if exception 
#define SS_CPU_PRE_DEC // no "pre" decrement if exception!
#define SS_CPU_SET_DEST_TO_0 // for Aladin

// PC as pushed in case of bus error, based on microcodes, cost 4KB:
#define SS_CPU_TRUE_PC 
#define SS_CPU_TRUE_PC_AND_NO_HACKS // option 'Hacks' or not: true PC

#endif//exception

#if defined(SS_CPU_FETCH_IO)
#define SS_CPU_FETCH_IO_FULL // need all or nothing for: Union Demo!
#endif

#if defined(SS_CPU_PREFETCH)

#if defined(SS_BETA) && defined(SS_DEBUG)
//#define SS_CPU_NO_PREFETCH // fall for all prefetch tricks (debug)
#endif


// Change no timing, just the macro used, so that we can identify what timings
// are for prefetch:
#define SS_CPU_FETCH_TIMING  

// Move the timing counting from FETCH_TIMING to PREFETCH_IRC:
#define SS_CPU_PREFETCH_TIMING //big, big change
#ifdef SS_CPU_PREFETCH_TIMING 
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
#define SS_CPU_PREFETCH_MULS
#define SS_CPU_PREFETCH_MULU
#define SS_CPU_PREFETCH_NOP
#define SS_CPU_PREFETCH_PEA


#if defined(SS_CPU_PREFETCH_TIMING) || defined(CORRECTING_PREFETCH_TIMING)
#define SS_CPU_PREFETCH_MOVE_MEM // this was forgotten in v3.5.0! limited impact
// Additional fixes based on Yacht, each one protected (3.5.0 seems stable)
// if it can change anything (TODO)
#define SS_CPU_YACHT_TAS // confirming what Steem authors suspected
#endif

#if defined(SS_DEBUG)
//#define SS_CPU_PREFETCH_DETECT_IRC_TRICK // asserts 
#define SS_CPU_PREFETCH_TRACE 
#define SS_CPU_TRACE_DETECT 
#endif

#endif//prefetch

#if defined(SS_CPU_ROUNDING)
#define SS_CPU_ROUNDING_SOURCE_100 // -(An)
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
//#define SS_DEBUG_TRACE_IDE
#endif
#if defined(DEBUG_BUILD) 
#define SS_DEBUG_TRACE_FILE
#else//VC
#define SS_DEBUG_TRACE_FILE
#endif
#endif

#if defined(DEBUG_BUILD) //TODO add other mods here
#define SS_DEBUG_CLIPBOARD // right-click on 'dump' to copy then paste
#define SS_DEBUG_DIV // no DIV log necessary
#endif

#define SS_DEBUG_LOG_OPTIONS // mine

#if defined(SS_DEBUG) && !defined(BCC_BUILD) && !defined(_DEBUG) 
// supposedly we're building the release boiler, make sure features are in

#define SS_DEBUG_TRACE_IO

#define SS_SHIFTER_EVENTS // record all shifter events of the frame
#if !defined(SS_UNIX)
#define SS_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif
#define SS_SHIFTER_EVENTS_ON_STOP // each time we stop emulation
#define SS_DEBUG_START_STOP_INFO
#define SS_IPF_TRACE_SECTORS // show sector info (IPF)

#define SS_IKBD_6301_DUMP_RAM
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


/////////
// DMA //
/////////

#if defined(SS_DMA) // this is the DMA as used for disk operation

//#define SS_DMA_ADDRESS // enforcing order for write (no use?)
//#define SS_DMA_DOUBLE_FIFO // works but overkill  
//#define SS_DMA_DELAY // works but overkill
#define SS_DMA_COUNT_CYCLES
#define SS_DMA_FDC_ACCESS
//#define SS_DMA_FDC_READ_HIGH_BYTE // like pasti, 0
#define SS_DMA_FIFO // first made for CAPS 
#define SS_DMA_FIFO_NATIVE // then extended for Steem native (not R/W tracks)
#define SS_DMA_FIFO_PASTI // and Pasti
#define SS_DMA_FIFO_READ_ADDRESS // save some bytes...
#define SS_DMA_FIFO_READ_ADDRESS2 // save 4 bytes more...
#define SS_DMA_READ_STATUS 
#define SS_DMA_SECTOR_COUNT
#define SS_DMA_WRITE_CONTROL

#endif


///////////
// DRIVE //
///////////

#if defined(SS_DRIVE)

#define SS_DRIVE_BYTES_PER_ROTATION
#define SS_DRIVE_EMPTY
#define SS_DRIVE_MOTOR_ON
#define SS_DRIVE_MULTIPLE_SECTORS
#define SS_DRIVE_READ_ADDRESS_TIMING
#define SS_DRIVE_READ_TRACK_TIMING
#define SS_DRIVE_RW_SECTOR_TIMING // start of sector
#define SS_DRIVE_RW_SECTOR_TIMING2 // end of sector (hack)
#define SS_DRIVE_SPIN_UP_TIME
#define SS_DRIVE_SPIN_UP_TIME2 // more precise
#define SS_DRIVE_WRITE_TRACK_TIMING

#endif


/////////
// FDC //
/////////

#if defined(SS_FDC)

#define SS_FDC_ACCURATE // bug fixes and additions for ADAT mode (v3.5)

#if defined(SS_FDC_ACCURATE) 

#define SS_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari
#define SS_FDC_CHANGE_SECTOR_WHILE_BUSY // from Hatari or Kryoflux
#define SS_FDC_CHANGE_TRACK_WHILE_BUSY // from Hatari or Kryoflux
#define SS_FDC_HEAD_SETTLE
#define SS_FDC_INDEX_PULSE
#define SS_FDC_MOTOR_OFF 
#define SS_FDC_PRECISE_HBL
#define SS_FDC_READ_ADDRESS_UPDATE_SR
#define SS_FDC_RESTORE_AGENDA
#define SS_FDC_SEEK
#define SS_FDC_SPIN_UP_AGENDA
#define SS_FDC_SPIN_UP_STATUS
#define SS_FDC_STEP


#ifdef SS_PSG
#define SS_FDC_IGNORE_WHEN_NO_DRIVE_SELETED // from Hatari
#define SS_FDC_MOTOR_OFF_COUNT_IP
#endif

#ifdef SS_DRIVE
#define SS_FDC_VERIFY_AGENDA //TODO can't really disable
#endif

#endif//SS_FDC_ACCURATE

//#define SS_FDC_TRACE_STATUS //spell out status register

#endif


//////////
// GLUE //
//////////





//////////
// IKBD //
//////////

#if defined(SS_IKBD)

#define SS_IKBD_6301 // HD6301 true emu, my pride!
#define SS_IKBD_FAKE_ABS_MOUSE // less is more!
// 6301 custom programs fake emu from Hatari (4K), disabled in v3.5.1:
//#define SS_IKBD_FAKE_CUSTOM 
//#define SS_IKBD_FAKE_CUSTOM_TRACE // trace checksums
#define SS_IKBD_FAKE_MOUSE_SCALE // actually use the scale
#define SS_IKBD_MANAGE_ACIA_TX
#define SS_IKBD_MOUSE_OFF_JOYSTICK_EVENT // hardware quirk?
#define SS_IKBD_TRACE_CPU_READ
//#define SS_IKBD_TRACE_CPU_READ2

#if defined(SS_HACKS)
//#define SS_IKBD_FAKE_CUSTOM_IS_HACK // need option Hacks to make them work
//#define SS_IKBD_FAKE_CUSTOM_DRAGONNELS // keyboard selection
#define SS_IKBD_POLL_IN_FRAME // poll once during the frame too
#define SS_IKBD_OVERDRIVE // reset keys?
#endif

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
#define SS_IKBD_6301_RUN_IRQ_TO_END // hack around Sim6xxx's working
#define SS_IKBD_6301_SET_TDRE
#define SS_IKBD_6301_TIMER_FIX // not sure there was a problem
//#define SS_IKBD_6301_TRACE // defined in SS_DEBUG
#if defined(SS_IKBD_6301_TRACE)
//#define SS_IKBD_6301_DISASSEMBLE_ROM 
//#define SS_IKBD_6301_TRACE_SCI_RX
//#define SS_IKBD_6301_TRACE_SCI_TX
//#define SS_IKBD_6301_TRACE_INT_TIMER
//#define SS_IKBD_6301_TRACE_INT_SCI
//#define SS_IKBD_6301_TRACE_STATUS
//#define SS_IKBD_6301_DUMP_RAM
//#define SS_IKBD_6301_TRACE_KEYS
#endif
#endif//#if defined(SS_IKBD_6301)

#endif


///////////////
// INTERRUPT //
///////////////
// temp there ought to be a better structure
#if defined(SS_INTERRUPT)

#define SS_INT_JITTER // there's also the wobble of Steem, which are correct?
#define SS_INT_HBL 
#define SS_INT_VBL 
#define SS_INT_VBL_INLINE

#if defined(SS_INT_JITTER) && defined(SS_INT_HBL)
#define SS_INT_JITTER_HBL 
#endif

#if defined(SS_INT_JITTER) && defined(SS_INT_VBL) && defined(SS_STF)
#define SS_INT_JITTER_VBL // only STF
#endif

#if defined(SS_STF) && defined(SS_INT_VBL)
//#define SS_INT_VBI_START // using event system / broken
#define SS_INT_VBL_STF // more a hack 
#endif

#endif


/////////
// IPF //
/////////

#if defined(SS_IPF)

// those switches were used for development, normally they ain't necessary
//#define SS_IPF_CPU // total sync, works but overkill
//#define SS_IPF_RUN_PRE_IO 
//#define SS_IPF_RUN_POST_IO
#define SS_IPF_ASSOCIATE // extension associated with Steem
#define SS_IPF_OSD // for the little box at reset
#ifdef SS_BETA
#define SS_IPF_LETHAL_XCESS // hack useful with capsimg v4.2
#define SS_IPF_WRITE_HACK // useless hack for v4.2 AFAIK
#endif
//#define SS_IPF_SAVE_WRITES //TODO?
#define SS_IPF_TRACE_SECTORS // show sector info

#endif


/////////
// MFP //
/////////

#if defined(SS_MFP)

#define SS_MFP_IACK_LATENCY
#define SS_MFP_IRQ_DELAY // from Hatari 
#define SS_MFP_IRQ_DELAY2
#define SS_MFP_RATIO // change the values of CPU & MFP freq!
#define SS_MFP_RATIO_PRECISION // for short timers
#define SS_MFP_RATIO_STE // measured (by Steem Authors) for STE?
#define SS_MFP_RATIO_STF // theoretical for STF
#define SS_MFP_RATIO_STE_AS_STF // change STF + STE (same for both)
#define SS_MFP_RS232 //one little anti-hang bugfix
#define SS_MFP_TIMER_B 
#if defined(SS_MFP_TIMER_B)
#define SS_MFP_TIMER_B_AER // earlier trigger (from Hatari)
#define SS_MFP_TIMER_B_NO_WOBBLE // does it fix anything???
#endif
#define SS_MFP_TxDR_RESET // they're not reset according to doc
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

//#define SS_MMU_NO_CONFUSION // Diagnostic cartridge: don't define (v3.5.2)
#define SS_MMU_WAKE_UP
#define SS_MMU_WRITE // programs in RAM may write in the MMU

#if defined(SS_MMU_WAKE_UP)
#if defined(SS_CPU)
//#define SS_MMU_WAIT_STATES // extreme, replaces rounding to 4, TODO
#endif
#define SS_MMU_WAKE_UP_0_BYTE_LINE
#define SS_MMU_WAKE_UP_DELAY_ACCESS // CPU can't access bus when MMU has it
#define SS_MMU_WAKE_UP_IO_BYTES_R 
#define SS_MMU_WAKE_UP_IO_BYTES_W 
#define SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY
#define SS_MMU_WAKE_UP_IOR_HACK 
#define SS_MMU_WAKE_UP_IOW_HACK 
#define SS_MMU_WAKE_UP_PALETTE_STE // render +1 cycle in state 2
//#define SS_MMU_WAKE_UP_READ_SDP
#define SS_MMU_WAKE_UP_RIGHT_BORDER
//#define SS_MMU_WAKE_UP_STE_STATE2 // STE in same state2 as STF (no)
//#define SS_MMU_WAKE_UP_WRITE_SDP
#endif// SS_MMU_WAKE_UP


#endif


/////////
// OSD //
/////////

#if defined(SS_OSD)

#define SS_OSD_DRIVE_INFO // cool! (v3.5.1)
#define SS_OSD_DRIVE_INFO2 // no SR when fast
#define SS_OSD_DRIVE_LED
#define SS_OSD_DRIVE_LED2 // simpler approach
#define SS_OSD_DRIVE_LED3 // remove useless variable
#define SS_OSD_LOGO //hack (temp)
#define SS_OSD_SCROLLER_CONTROL

#endif


///////////
// PASTI //
///////////

#if defined(SS_PASTI) && defined(WIN32)

#define SS_PASTI_ALWAYS_DISPLAY_STX_DISKS
#define SS_PASTI_AUTO_SWITCH
#define SS_PASTI_ONLY_STX  // experimental! optional
#define SS_PASTI_NO_RESET 
#define SS_PASTI_ON_WARNING // mention in disk manager title

#endif


/////////
// PSG //
/////////

#if defined(SS_PSG)

#ifdef SS_DEBUG
#define SS_PSG_REPORT_DRIVE_CHANGE // as FDC trace
#endif

#endif

/////////
// SDL //
/////////

#if defined(SS_SDL)
#if SSE_VERSION<360
#define SS_SDL_DEACTIVATE // support planned in v3.6
#endif
#endif


/////////////
// SHIFTER //
/////////////

#if defined(SS_SHIFTER)

#define SS_SHIFTER_IO // move blocks from ior, iow
//#define SS_SHIFTER_IOR_TRACE // specific, not "log"
//#define SS_SHIFTER_IOW_TRACE // specific, not "log"

#define SS_SHIFTER_TRICKS  // based on Steem system, extended

#if defined(SS_SHIFTER_TRICKS)

#define SS_SHIFTER_0BYTE_LINE
#if defined(SS_SHIFTER_0BYTE_LINE)
#define SS_SHIFTER_0BYTE_LINE_RES_END //No Buddies Land
#define SS_SHIFTER_0BYTE_LINE_RES_HBL //Beyond/Pax Plax Parallax
#define SS_SHIFTER_0BYTE_LINE_RES_START //Nostalgic-O/Lemmings
#define SS_SHIFTER_0BYTE_LINE_SYNC //Forest
#endif
#define SS_SHIFTER_4BIT_SCROLL //Let's do the Twist again
#define SS_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK
#define SS_SHIFTER_60HZ_OVERSCAN //Leavin' Terramis
#define SS_SHIFTER_END_OF_LINE_CORRECTION // correct +2, -2 lines 
#define SS_SHIFTER_LEFT_OFF_TEST_BEFORE_HBL // for Riverside
#define SS_SHIFTER_LINE_PLUS_2_THRESHOLD //Forest
#define SS_SHIFTER_LINE_PLUS_20 // 224 byte scanline STE only
#define SS_SHIFTER_LINE_PLUS_20_SHIFT // for Riverside
#define SS_SHIFTER_MED_OVERSCAN // BPOC
#define SS_SHIFTER_MED_OVERSCAN_SHIFT // No Cooper/greetings
#define SS_SHIFTER_NON_STOPPING_LINE // Enchanted Land
#define SS_SHIFTER_PALETTE_BYTE_CHANGE //Golden Soundtracker
#define SS_SHIFTER_PALETTE_TIMING //Overscan Demos #6
#define SS_SHIFTER_STE_MED_HSCROLL // Cool STE
#define SS_SHIFTER_STE_HSCROLL_LEFT_OFF //MOLZ/Spiral
#define SS_SHIFTER_STE_VERTICAL_OVERSCAN //RGBeast
#define SS_SHIFTER_UNSTABLE // DoLB, Omega, Overdrive/Dragon, Beeshift
//#define SS_SHIFTER_UNSTABLE_LEFT_OFF // DoLB, Omega, Overdrive/Dragon

#if defined(SS_STF)
#define SS_STF_VERTICAL_OVERSCAN
#endif

//#define SS_VID_LEFT_OFF_COMPARE_STEEM_32
//#define SS_VID_TRACE_SUSPICIOUS2 // suspicious +2 & -2
#endif//#if defined(SS_SHIFTER_TRICKS)

#define SS_SHIFTER_SDP // SDP=shifter draw pointer

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
#define SS_SHIFTER_DRAGON1 // confused shifter, temp hack
#define SS_SHIFTER_OMEGA  // Omega Full Overscan shift (60hz)
#define SS_SHIFTER_PACEMAKER // Pacemaker credits flickering line
#define SS_SHIFTER_SCHNUSDIE // Reality is a Lie/Schnusdie overscan logo
#define SS_SHIFTER_TCB // Swedish New Year Demo/TCB SDP (60hz)
#define SS_SHIFTER_TEKILA // Delirious 4/Tekila
#endif

#if defined(SS_DEBUG) 
//#define SS_SHIFTER_DRAW_DBG  // totally bypass CheckSideOverscan() & Render()
#define SS_SHIFTER_EVENTS // recording all shifter events in a frame
#define SS_SHIFTER_EVENTS_PAL // also for palette
#define SS_SHIFTER_EVENTS_READ_SDP // also for read SDP
#define SS_SHIFTER_EVENTS_ON_STOP // each time we stop emulation
#if !defined(SS_DEBUG_TRACE_IDE)
#define SS_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif
//#define SS_SHIFTER_VERTICAL_OVERSCAN_TRACE
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

#define SS_SOUND_CHANGE_TIME_METHOD_DELAY //detail
//#define SS_SOUND_DETECT_SAMPLE_RATE//?
#define SS_SOUND_FILTER_STF // a very simple filter

#define SS_SOUND_INLINE // macro->inline, easier for my tests, but hard to do
//#define SS_SOUND_LOW_PASS_FILTER  // Float exceptions, no thanks

#define SS_SOUND_MICROWIRE // volume, balance, bass & treble, primitive DSP
#define SS_SOUND_NO_EXTRA_PER_VBL //compensating hack? changes what?

#define SS_SOUND_OPTIMISE
//#define SS_SOUND_OPTION_DISABLE_DSP // not needed if no anti-aliasing
#define SS_SOUND_RECOMMEND_OPTIONS

#ifdef WIN32
//#define SS_SOUND_SKIP_DSOUND_TEST
#endif

#define SS_SOUND_VOL // -6db for PSG chipsound except samples (using DSP)
#define SS_SOUND_FILTER_STE // same very simple filter as for STF

#ifdef UNIX
#define SS_RTAUDIO_LARGER_BUFFER //simplistic attempt
#endif


#endif


/////////////////////////////////
// ST Model (various STF, STE) //
/////////////////////////////////

#if defined(SS_STF)

#define SS_STE_2MB // auto make RAM 2MB, more compatible
#define SS_STF_1MB // auto make RAM 1MB, more compatible
#ifdef WIN32
#define SS_STF_MATCH_TOS // select a compatible TOS for next reset
#endif
#define SS_STF_MEGASTF // blitter in STF (could be useful?) + 4MB!

#endif


///////////////
// STRUCTURE //
///////////////

#if defined(SS_STRUCTURE)

/*
step by step
we copy all Steem declarations in one file, big_forward.h
then we may delete all previous forward declarations
later it will be between big forward and full separation
still must ve totally reversible, and depends on compiler
err
problem, multiple struct/class definition not allowed?
*/

#define SS_STRUCTURE_BIG_FORWARD // temp

#if !defined(SS_UNIX)
#define SS_STRUCTURE_NEW_H_FILES
#endif

#define SS_STRUCTURE_DMA_INC_ADDRESS
//#define SS_STRUCTURE_INFO // just telling files included in modules
#define SS_STRUCTURE_IOR

#if defined(SS_STRUCTURE_NEW_H_FILES)
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
//...
#define SS_STRUCTURE_RUN_H
#endif

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
#endif

// fixes by other people:
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

#ifdef WIN32
#define SS_VAR_CHECK_SNAPSHOT
#endif
#define SS_VAR_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
#define SS_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES // at final save
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SS_VAR_F12 // F12 starts/stops emulation
#endif
#define SS_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen - option?
//#define SS_VAR_HIDE_OPTIONS_AT_START // hack before debugging
#define SS_VAR_INFOBOX1 // SSE internet sites
#define SS_VAR_INFOBOX2 // SSE readme + FAQ
#define SS_VAR_INFOBOX3 // readme text font
#define SS_VAR_INFOBOX4 // readme 80 col 
#define SS_VAR_INFOBOX5 // don't take 64K on the stack!
#define SS_VAR_KEYBOARD_CLICK // not a sound nor IKBD option
#define SS_VAR_MOUSE_CAPTURE 
#define SS_VAR_MSA_CONVERTER // don't prompt if found
#define SS_VAR_NO_AUTO_ASSOCIATE // + may deassociate ... TODO
#define SS_VAR_NOTIFY //adding some notify during init
#define SS_VAR_NO_UPDATE // remove all code in relation to updating
#define SS_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah
#define SS_VAR_OPTIONS_REFRESH
#define SS_VAR_RESIZE // reduce memory set (int->BYTE etc.)
#define SS_VAR_REWRITE // to conform to what compilers expect (warnings...)
#define SS_VAR_REWRITE_VC_2012 // from DrCoolZic
#define SS_VAR_SCROLLER_DISK_IMAGE
#define SS_VAR_STEALTH // don't tell we're an emulator (option)
#ifdef WIN32
#define SS_VAR_UNRAR // using unrar.dll, up to date
#endif


#endif 


///////////
// VIDEO //
///////////

#if defined(SS_VIDEO)

#define SS_VID_ADJUST_DRAWING_ZONE1 // attempt to avoid crash
#define SS_VID_ADJUST_DRAWING_ZONE2
//#define SS_VID_AUTOOFF_DECRUNCHING // bad idea (complaints)
#define SS_VID_BLIT_TRY_BLOCK //?

#define SS_VID_BORDERS // option display size (normal-big-bigger-biggest)
#define SS_VID_BORDERS_LB_DX // rendering-stage trick rather than painful hacks

#define SS_VID_BPOC // Best Part of the Creation fit display 800 hack
#define SS_VID_CHECK_DDFS // is video card/display capable?

#if defined(WIN32) 
#define SS_VID_RECORD_AVI //avifile
#endif

#if defined(WIN32) //TODO Unix
#define SS_VID_SAVE_NEO // screenshots in ST Neochrome format
#endif

#if defined(WIN32)
#define SS_VID_SCANLINES_INTERPOLATED // using stretch mode!
#define SS_VID_SCANLINES_INTERPOLATED_MED
#endif

#define SS_VID_SCREENSHOTS_NUMBER

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
#undef SS_IKBD_MANAGE_ACIA_TX
#endif

#if !defined(SS_ACIA_REGISTERS)
#undef SS_ACIA_DOUBLE_BUFFER_TX
#undef SS_ACIA_TEST_REGISTERS
#undef SS_ACIA_USE_REGISTERS
#endif

#if !defined(SS_ACIA_DOUBLE_BUFFER_RX) || !defined(SS_ACIA_DOUBLE_BUFFER_TX)
//#undef SS_ACIA_TEST_REGISTERS
#undef SS_ACIA_USE_REGISTERS
#endif

#if !defined(SS_DMA)
#undef SS_STRUCTURE_DMA_INC_ADDRESS
#endif

#if !defined(SS_DMA_FIFO)
#undef SS_IPF
#endif

#if !defined(SS_FDC)
#undef SS_OSD_DRIVE_INFO2
#endif

#if !defined(SS_SHIFTER)
#undef SS_CPU_3615GEN4_ULM
#undef SS_MMU_WAKE_UP_IO_BYTES_R
#undef SS_MMU_WAKE_UP_IOR_HACK
#undef SS_MMU_WAKE_UP_IOW_HACK
#undef SS_SHIFTER_EVENTS
#endif

#if defined(SS_SHIFTER_UNSTABLE)
#undef SS_SHIFTER_DRAGON1
#endif

#if defined(SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY)
#undef SS_MMU_WAKE_UP_IO_BYTES_W
#endif

#if !defined(SS_OSD_SCROLLER_CONTROL)
#undef SS_VAR_SCROLLER_DISK_IMAGE
#endif


#if !defined(SS_SOUND_LOW_PASS_FILTER)
#undef SS_SOUND_DETECT_SAMPLE_RATE
#endif

#if !USE_PASTI
#undef SS_DMA_FIFO_PASTI
#endif

#if !defined(SS_DMA) || !defined(SS_FDC)
#undef SS_OSD_DRIVE_LED2
#undef SS_OSD_DRIVE_LED3
#endif

#if !defined(SS_DRIVE)
#undef SS_PASTI_ONLY_STX 
#endif

#if defined(SS_SSE_LEAN_AND_MEAN) //TODO
//6301 not optional...
#undef SS_VID_CHECK_DDFS
#undef SS_VID_RECORD_AVI
#undef SS_VID_SAVE_NEO
#endif

#if !defined(SS_VID_BORDERS) || !defined(SS_HACKS)
#undef SS_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK
#endif

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
