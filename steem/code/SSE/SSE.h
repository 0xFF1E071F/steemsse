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
-In folder 'code\SSE', several files starting with 'SSE', including this one.
-A folder '6301' in '3rdparty' for true emulation of the IKBD
-A folder 'avi' in '3rdparty' for recording video to AVI support
-A folder 'caps' in '3rdparty' for IPF disk image format support
-A folder 'caps_linux' in '3rdparty' for future IPF disk image format support
-A folder 'dsp' in '3rdparty' for Microwire emulation
-A file 'div68kCycleAccurate.c' in '3rdparty\pasti', to use the correct DIV 
timings found by ijor (also author of Pasti).
-A folder 'SDL-WIN' for future SDL support
-A folder 'unRARDLL' in '3rdparty' for unrar support

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
working build since v3.4.
The VC6 build should be linked with the C++ library, don't count on system
DLL or it will crash in Windows Vista & 7.
The best build should be the M$ one. They're greedy but they know their 
business! 
    
SSE.h is supposed to mainly be a collection of compiling switches (defines).
It should include nothing and can be included everywhere.
Normally switches are optional, but they also are useful just to mark
code changes.

*/

/*
TODO (in no definite order)
- Bad crash on European Demos (3.4.1 onward)
in fact emu going wrong takes Steem in release build in W7
despite the try
- Unicode!
- printing directly on a PC printer?
- Systematic fix for 508/512 cycle counting issue (Omega)
- ACIA/MIDI corrections
- Event for write to IKBD, but would it be useful?
- Check if TOS1.00 sould boot with 4MB (MMU)
- OSD shifted in 'large' display mode
- 'redraw' in this mode
- better OSD "Steem SSE" logo
- Boiler: real callstack
- Boiler: "run to RTS"
- monochrome demos
- Overdrive/Dragon + a common way for all "unstable" overscan
- Eliminate magic constants in gui
- unix: SSE options icon
- use IPF insights to improve fdc emu
- convert ST to IPF on the fly?
- cycles: ROM access isn't shared with shifter
- cycles: CPU vs HBL
- don't crash at corrupt snapshot ('try'?)
- IPF + Pasti Blood Money (CPU?) note bytes are different
- Check if image in zip OK (eg no IPF for pasti), maybe complicated
- automatically switch to Pasti (but linked with above?)
- Move all shifter IO to object shifter
- SLM804 laser printer, yeah right
- IPF support for Linux build
- examine SDL possibilities
- a struct for video? for DLLs?
- sdl option + grey out sdl if unavailable
- bugaboo reset
- PP38: joystick shouldn't work on all STF?
- syndic84 pre-version?
- blitter emu lacks sthg for "smudge"?
- STE wake up states? (Forest)
- Unix build of 3.5, but with fewer features
- Load snaphost -> pasti mention?

*/


/////////////
// VERSION //
/////////////

#if defined(STEVEN_SEAGAL)

// #define SS_BETA

#ifdef SS_BETA // beta with all features
#define SSE_VERSION 350 //was for?
#define SSE_VERSION_TXT "SSE Beta" 
#define WINDOW_TITLE "Steem SSE beta"//"Steem Engine SSE 3.5.0" 
#else // next planned release
#define SSE_VERSION 350 // check snapshot; rc\resource.rc
#define SSE_VERSION_TXT "3.5.0" 
#define WINDOW_TITLE "Steem Engine SSE 3.5.0" 
#endif

#endif


//////////////////
// BIG SWITCHES //
//////////////////

#if defined(STEVEN_SEAGAL)

// todo any combination must be possible

#define SS_ACIA       // MC6850 Asynchronous Communications Interface Adapter
// todo undef SS_ACIA & KBD still works...
#define SS_BLITTER    // spelled BLiTTER
#define SS_CPU        // M68000 microprocessor
#define SS_DMA        // Custom Direct Memory Access chip (disk)
#define SS_FDC        // WD1772 floppy disk controller
#define SS_GLUE       // TODO
#define SS_HACKS      // an option for dubious fixes
#define SS_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#define SS_IPF        // CAPS support (IPF disks) 
#define SS_INTERRUPT  // MC68901 Multi-Function Peripheral Chip, HBL, VBL
#define SS_MIDI       // TODO
#define SS_MMU        // Memory Manager Unit (of the ST, no paging)
#define SS_OSD        // On Screen Display (drive leds, version)
#define SS_SDL        // Simple DirectMedia Layer (TODO)
#define SS_SHIFTER    // The legendary custom shifter and all its tricks
#define SS_SOUND      // YM2149, STE DMA sound, Microwire
#define SS_STF        // switch STF/STE
#define SS_STRUCTURE  // TODO
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

#if SSE_VERSION<360
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
#endif




///////////////////////
// DETAILED SWITCHES //
///////////////////////


#if defined(STEVEN_SEAGAL)


/////////////
// GENERAL //
/////////////

#define SS_SSE_OPTION_PAGE // a new page for all our options
#define SS_SSE_OPTION_STRUCT // structure SSEOption 
#define SS_SSE_CONFIG_STRUCT // structure SSEConfig 
#define SS_DELAY_LOAD_DLL // can run without DLL


//////////
// ACIA //
//////////

#if defined(SS_ACIA)

#define SS_ACIA_BUS_JAM_NO_WOBBLE
#define SS_ACIA_DOUBLE_BUFFER_TX // only to 6301 (not MIDI)
#define SS_ACIA_IRQ_DELAY // only from 6301 (not MIDI)

#endif


/////////////
// BLITTER //
/////////////

#if defined(SS_BLITTER)

#define SS_BLT_BLIT_MODE_CYCLES // #cycles in BLIT Mode in NOPs, not 68K cycles
#define SS_BLT_BLIT_MODE_INTERRUPT // trigger at once (not after blit phase)
#define SS_BLT_HOG_MODE_INTERRUPT // no interrupt in hog mode
//#define SS_BLT_OVERLAP // TODO
//#define SS_BLT_TIMING // based on a table, but Steem does it better

//TODO smudge?
#endif


/////////
// CPU //
/////////

#if defined(SS_CPU)

#define SS_CPU_EXCEPTION    // crash like Windows 98
#define SS_CPU_PREFETCH     // fetch like a dog
#define SS_CPU_ROUNDING     // round like a rolling stone

#if defined(SS_CPU_ROUNDING)

//#define SS_CPU_ROUNDING_CHECKS // 1st approach, a bit on the heavy side
#if defined(SS_CPU_ROUNDING_CHECKS)
#define SS_CPU_ROUNDING_FIX_ADD_L
#define SS_CPU_ROUNDING_FIX_ADDA_L
#define SS_CPU_ROUNDING_FIX_SUB_L 
#define SS_CPU_ROUNDING_FIX_SUBA_L
#endif

// far simpler, but more general (risky) than checking later:
#define SS_CPU_ROUNDING_SOURCE_100 // -(An)
// but this wouldn't be correct:
// you must fetch D8 first, no? Not clear...
//#define SS_CPU_ROUNDING_SOURCE_110 // (d8, An, Xn) // breaks DSOS etc
//#define SS_CPU_ROUNDING_SOURCE_111_011 // (d8, PC, Xn)

#endif//rounding


// Redone instructions:

#define SS_CPU_CLR          // read before writing (changes nothing?)
#define SS_CPU_DIV          // divide like Caesar
#define SS_CPU_MOVE_B       // move like a superstar
#define SS_CPU_MOVE_W       
#define SS_CPU_MOVE_L
#define SS_CPU_POKE         // poke like a C64 (inline in VC6)
//#define SS_CPU_EXCEPTION_TRACE_PC // reporting all PC
//#define SS_CPU_PREFETCH_TRACE

//#define SS_CPU_TAS

#if defined(SS_CPU_EXCEPTION)
#define SS_CPU_ASSERT_ILLEGAL // assert before trying to execute (Titan)
#define SS_CPU_GET_SOURCE // update PC after read - is it a real fix? Phaleon
#ifndef SS_CPU_GET_SOURCE // when PC is ++ (Phaleon)
#define SS_CPU_PHALEON // opcode based hack for Phaleon (etc.) protection
#endif
#define SS_CPU_MAY_WRITE_0 // it's both ROM and RAM!
#define SS_CPU_POST_INC // no post increment if exception (Beyond)
#endif//exc


#if defined(SS_CPU_PREFETCH)
// Change no timing, just the macro used, so that we can identify what timings
// are for prefetch:
#define SS_CPU_FETCH_TIMING  //todo, compile errors if undef

// Move the timing counting from FETCH_TIMING to PREFETCH_IRC, this is a big
// change:
#define SS_CPU_PREFETCH_TIMING //big, big change
#ifdef SS_CPU_PREFETCH_TIMING 
#define SS_CPU_PREFETCH_TIMING_SET_PC // necessary for some SET PC cases
//#define SS_CPU_PREFETCH_TIMING_EXCEPT // to mix unique switch + lines
#endif
#if !defined(SS_CPU_PREFETCH_TIMING) || defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
//#define CORRECTING_PREFETCH_TIMING 
#endif
#ifdef CORRECTING_PREFETCH_TIMING
// powerful prefetch debugging switches
#define SS_CPU_LINE_0_TIMINGS // 0000 Bit Manipulation/MOVEP/Immediate
#define SS_CPU_LINE_1_TIMINGS  // 0001 Move Byte
#define SS_CPU_LINE_1_TIMINGS_2 // ea,Dn
#define SS_CPU_LINE_2_TIMINGS // 0010 Move Long
#define SS_CPU_LINE_2_TIMINGS_2 // ea,Dn/An
#define SS_CPU_LINE_3_TIMINGS // 0011 Move Word
#define SS_CPU_LINE_3_TIMINGS_2 // ea,Dn/An
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

#endif//prefetch

#if defined(SS_CPU_EXCEPTION)
#if defined(SS_CPU_PREFETCH_______)
#define SS_CPU_WAR_HELI2 // other way for Wat Heli (trying)  / REDO
#else
#define SS_CPU_WAR_HELI // isolate a hack already in v3.2
#endif
#endif

#if defined(SS_CPU_MOVE_W) && defined(SS_SHIFTER) // hack: only for move.w
#define SS_CPU_3615GEN4_ULM // changing the scanline being fetched
#endif

#define SS_CPU_LINE_F // for interrupt depth counter

#endif


///////////
// DEBUG //
///////////

#if !defined(SS_DEBUG) && (defined(_DEBUG) || defined(DEBUG_BUILD) || \
defined(SS_BOILER))
#define SS_DEBUG  //use lost?
#endif

#if defined(SS_DEBUG)

#define SS_DEBUG_TRACE

#ifdef SS_DEBUG_TRACE
#ifdef _DEBUG // VC
#define SS_DEBUG_TRACE_IDE
#endif
#if defined(DEBUG_BUILD)
#define SS_DEBUG_TRACE_FILE
#else
//#define SS_DEBUG_TRACE_FILE
#endif
#endif

#if defined(DEBUG_BUILD) //TODO add other mods here
#define SS_DEBUG_CLIPBOARD // right-click on 'dump' to copy then paste
#define SS_DEBUG_DIV // no DIV log necessary
#endif

#define SS_DEBUG_LOG_OPTIONS // mine

#if defined(SS_DEBUG) && !defined(BCC_BUILD) && !defined(_DEBUG)
// supposedly we're building the release boiler, make sure features are in
#define SS_SHIFTER_EVENTS // record all shifter events of the frame
#define SS_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#define SS_SHIFTER_EVENTS_ON_STOP // each time we stop emulation
#define SS_DEBUG_START_STOP_INFO
#define SS_IPF_TRACE_SECTORS // show sector info (IPF)
#define SS_IKBD_TRACE_COMMANDS // only used by 6301 emu now
#define SS_IKBD_6301_DUMP_RAM
#define SS_IKBD_6301_TRACE 
#define SS_IKBD_6301_TRACE_SCI_RX
#define SS_IKBD_6301_TRACE_SCI_TX
#define SS_IKBD_6301_TRACE_KEYS

#else // for custom debugging

#define SS_DEBUG_START_STOP_INFO
//#define SS_DEBUG_TRACE_IO
#define SS_IKBD_TRACE_COMMANDS // only used by 6301 emu now
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

#if defined(SS_DMA)
// this is the DAM as used for disk operation
//#define SS_DMA_ADDRESS // enforcing order for write (no use?)
//#define SS_DMA_DOUBLE_FIFO // works but overkill
//#define SS_DMA_DELAY // works but overkill
#define SS_DMA_FDC_ACCESS
#define SS_DMA_IO // necessary for DMA+FDC fixes
#define SS_DMA_READ_STATUS 
#define SS_DMA_SECTOR_COUNT
#define SS_DMA_WRITE_CONTROL
#endif


/////////
// FDC //
/////////

#if defined(SS_FDC)

#define SS_FDC_ACCURATE // bug fixes and additions for ADAT mode (v3.5)

#ifdef SS_FDC_ACCURATE
#define SS_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari
#define SS_FDC_FORMAT_BYTES // more hbl / sector 
#define SS_FDC_INDEX_PULSE
#define SS_FDC_MOTOR_OFF // post command..
#define SS_FDC_RESTORE_AGENDA
#define SS_FDC_SEEK_UPDATE_TR
#define SS_FDC_SPIN_UP_STATUS
#define SS_FDC_SPIN_UP_TIME
#define SS_FDC_VERIFY_AGENDA
#endif
// those also in fast mode (can't hurt), it's not in fdc.cpp but iow.cpp
#define SS_FDC_CHANGE_SECTOR_WHILE_BUSY // from Hatari or Kryoflux
#define SS_FDC_CHANGE_TRACK_WHILE_BUSY // from Hatari or Kryoflux

//#define SS_FDC_READ_HIGH_BYTE // like pasti, 0
//#define SS_DEBUG_FDC_IO_IN_FDC_LOGSECTION //there's no silly define
//#define SS_FDC_TRACE_STATUS //spell out status register

#endif


//////////
// GLUE //
//////////





//////////
// IKBD //
//////////

#if defined(SS_IKBD)

#define SS_IKBD_6301 // HD6301 true emu

#if defined(SS_IKBD_6301) // all switches for Sim6301 are here too
#define SS_IKBD_6301_CHECK_COMMANDS
#define SS_IKBD_6301_CHECK_IREG_RO // some registers are read-only
#define SS_IKBD_6301_CHECK_IREG_WO // some registers are write-only
#define SS_IKBD_6301_DISABLE_BREAKS // to save 64k
#define SS_IKBD_6301_DISABLE_CALLSTACK // to save 3k on the PC stack
#define SS_IKBD_DOUBLE_BUFFER_6301_TX  // 6301 transmitting, same as for ACIA
#define SS_IKBD_RUN_IRQ_TO_END
#define SS_IKBD_6301_ADJUST_CYCLES // stay in sync!
#define SS_IKBD_6301_SET_TDRE
#define SS_IKBD_6301_TIMER_FIX // not sure there was a problem

#define SS_IKBD_TRACE_COMMANDS // report commands sent to IKBD
//#define SS_IKBD_TRACE_FAKE_CUSTOM // IKBD custom
#define SS_IKBD_TRACE_6301 // interaction with 6301 emu
//#define SS_IKBD_TRACE_6301_MOUSE // temp

#define SS_IKBD_6301_TRACE // ambigous!
#if defined(SS_IKBD_6301_TRACE)
//#define SS_IKBD_6301_TRACE_SCI_RX
//#define SS_IKBD_6301_TRACE_SCI_TX
//#define SS_IKBD_6301_TRACE_SCI_TRCSE
//#define SS_IKBD_6301_TRACE_INT_TIMER
//#define SS_IKBD_6301_TRACE_INT_SCI
//#define SS_IKBD_6301_DISASSEMBLE_CUSTOM_PRG 
//#define SS_IKBD_6301_DISASSEMBLE_ROM 
//#define SS_IKBD_6301_DUMP_RAM
#define SS_IKBD_6301_TRACE_KEYS
#endif

#endif//#if defined(SS_IKBD_6301)

#define SS_IKBD_FAKE_ABS_MOUSE // less is more!
#define SS_IKBD_FAKE_CUSTOM // HD6301 custom programs fake emu (from Hatari)
#define SS_IKBD_FAKE_MOUSE_SCALE // actually use the scale
#define SS_IKBD_MOUSE_OFF_JOYSTICK_EVENT // hardware quirk?

#if defined(SS_HACKS)
#define SS_IKBD_FAKE_CUSTOM_IS_HACK // need option Hacks to make them work
#define SS_IKBD_FAKE_DRAGONNELS // fake custom emu - keyboard selection
#define SS_IKBD_POLL_IN_FRAME // poll once during the frame too 
#endif

#endif


///////////////
// INTERRUPT //
///////////////

#if defined(SS_INTERRUPT)

#define SS_INT_JITTER // there's also the wobble of Steem, which are correct?
#define SS_INT_HBL 
#define SS_INT_MFP
#define SS_INT_TIMER_B 
#define SS_INT_VBL 

#if defined(SS_INT_JITTER) && defined(SS_INT_HBL)
#define SS_INT_JITTER_HBL 
#endif

#if defined(SS_INT_JITTER) && defined(SS_INT_VBL) && defined(SS_STF)
#define SS_INT_JITTER_VBL // only STF
#endif

#if defined(SS_INT_MFP)
///#define SS_MFP_ALL_I_HAVE // temp silly,useless hack! TODO
#define SS_MFP_POST_INT_LATENCY // hardware quirk?
#define SS_MFP_RATIO // change the values of CPU & MFP freq!
#define SS_MFP_RATIO_STE // measured (by Steem Authors) for STE
#define SS_MFP_RATIO_STF // theoretical for STF
#define SS_MFP_TxDR_RESET // they're not reset according to doc
#endif

#if defined(SS_INT_TIMER_B)
#define SS_INT_TIMER_B_NO_WOBBLE // does it fix anything???
#define SS_INT_TIMER_B_AER // earlier trigger (from Hatari)
#endif

#if defined(SS_STF) && defined(SS_INT_VBL)
//#define SS_INT_VBI_START // ideal but many problems, eg Calimero
#define SS_INT_VBL_STF // more a hack but easier to do
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
// MMU //
/////////

#if defined(SS_MMU)

#define SS_MMU_WRITE // programs in RAM may write in the MMU
#define SS_MMU_NO_CONFUSION //  I still don't know what was the use (TESTING)
#define SS_MMU_WAKE_UP
#if defined(SS_MMU_WAKE_UP)
#define SS_MMU_WAKE_UP_DELAY_ACCESS // CPU can't access bus when MMU has it

#define SS_MMU_WAKE_UP_0_BYTE_LINE
#define SS_MMU_WAKE_UP_IOR_HACK 
#define SS_MMU_WAKE_UP_IOW_HACK 
// the following are more experimental 
#define SS_MMU_WAKE_UP_IO_BYTES_R 
#define SS_MMU_WAKE_UP_IO_BYTES_W 
#define SS_MMU_WAKE_UP_IO_BYTES_W_SHIFTER_ONLY

//#define SS_MMU_WAKE_UP_READ_SDP
//#define SS_MMU_WAKE_UP_WRITE_SDP
#define SS_MMU_WAKE_UP_PALETTE_STE // render +1 cycle in state 2

#define SS_MMU_WAKE_UP_RIGHT_BORDER
//#define SS_MMU_WAKE_UP_STE_STATE2 // STE in same state2 as STF (no)
#endif// SS_MMU_WAKE_UP

#endif


/////////
// OSD //
/////////

#if defined(SS_OSD)

///#define SS_OSD_LOGO //TODO
#define SS_OSD_DRIVE_LED

#endif



/////////
// SDL //
/////////

#if defined(SS_SDL)

#endif


/////////////
// SHIFTER //
/////////////

#if defined(SS_SHIFTER)

#define SS_SHIFTER_TRICKS  // based on Steem system, extended

#if defined(SS_SHIFTER_TRICKS)

#define SS_SHIFTER_0BYTE_LINE
#if defined(SS_SHIFTER_0BYTE_LINE)
#define SS_SHIFTER_0BYTE_LINE_RES_END
#define SS_SHIFTER_0BYTE_LINE_RES_HBL
#define SS_SHIFTER_0BYTE_LINE_RES_START
#define SS_SHIFTER_0BYTE_LINE_SYNC
//#define SS_SHIFTER_0BYTE_LINE_TRACE
#endif
#define SS_SHIFTER_4BIT_SCROLL
#define SS_SHIFTER_60HZ_OVERSCAN
#define SS_SHIFTER_END_OF_LINE_CORRECTION // correct +2, -2 lines 
#define SS_SHIFTER_LINE_PLUS_2_THRESHOLD
#define SS_SHIFTER_MED_OVERSCAN
#define SS_SHIFTER_MED_OVERSCAN_SHIFT
#define SS_SHIFTER_NON_STOPPING_LINE // Enchanted Land
#define SS_SHIFTER_PALETTE_BYTE_CHANGE 
#define SS_SHIFTER_PALETTE_TIMING
#define SS_SHIFTER_STE_LEFT_OFF // 224 byte scanline
#define SS_SHIFTER_STE_MED_HSCROLL
#define SS_SHIFTER_STE_HSCROLL_LEFT_OFF

#if defined(SS_STF)
#define SS_STF_VERTICAL_OVERSCAN
#endif
#define SS_SHIFTER_STE_VERTICAL_OVERSCAN
#define SS_SHIFTER_UNSTABLE_LEFT_OFF // DoLB, Omega, Overdrive/Dragon
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
#define SS_SHIFTER_BIG_WOBBLE // Big Wobble shift
#define SS_SHIFTER_DANGEROUS_FANTAISY // Dangerous Fantaisy credits flicker
#define SS_SHIFTER_DRAGON // confused shifter, temp hack
#define SS_SHIFTER_OMEGA  // Omega Full Overscan shift (60hz)
#define SS_SHIFTER_PACEMAKER // Pacemaker credits flickering line
#define SS_SHIFTER_SCHNUSDIE // Reality is a Lie/Schnusdie overscan logo
#define SS_SHIFTER_TCB // Swedish New Year Demo/TCB SDP (60hz)
#endif

#if defined(SS_DEBUG) 
//#define SS_SHIFTER_DRAW_DBG  // totally bypass CheckSideOverscan() & Render()
#define SS_SHIFTER_EVENTS // recording all shifter events in a frame
//#define SS_SHIFTER_EVENTS_PAL // also for palette
#define SS_SHIFTER_EVENTS_ON_STOP // each time we stop emulation
//#define SS_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
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

#ifdef SS_BETA
//#define SS_SOUND_SAMPLES //TODO, can we improve it?
#endif

//#define SS_SOUND_LOW_PASS_FILTER // using DSP, for both STF & STE, disappointing

#define SS_SOUND_FILTER_STF // a very simple filter
#define SS_SOUND_MICROWIRE // volume, balance, bass & treble, primitive DSP
#define SS_SOUND_VOL // -6db for PSG chipsound except samples (using DSP)
#define SS_SOUND_FILTER_STE // same very simple filter as for STF

#endif


/////////////////////////////////
// ST Model (various STF, STE) //
/////////////////////////////////

#if defined(SS_STF)

#define SS_STF_MATCH_TOS // select a compatible TOS for next reset
#define SS_STF_MEGASTF // blitter in STF (could be useful?)
// TODO: Falcon, Mega STE, TT... yeah right

#endif


/////////
// TOS //
/////////

#if defined(SS_TOS)

#ifdef SS_HACKS
#define SS_TOS_PATCH106 // a silly bug, a silly hack
#endif


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
#define SS_UNIX_TRACE // TRACE into the terminal (if it's open?)
#endif

#endif


/////////////
// VARIOUS //
/////////////

#define NO_RAR_SUPPORT // I removed the library, so it's unconditional

#if defined(SS_VARIOUS)

#define SS_VAR_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
#define SS_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES // at final save

#define SS_VAR_NO_AUTO_ASSOCIATE // + may deassociate ... TODO

#define SS_VAR_NO_UPDATE // remove all code in relation to updating
#define SS_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah


#define SS_VAR_MOUSE_CAPTURE 
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SS_VAR_F12 // F12 starts/stops emulation
#endif
#define SS_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen - option?
//#define SS_VAR_HIDE_OPTIONS_AT_START // hack before debugging

#define SS_VAR_INFOBOX // reaction to information button

#define SS_VAR_KEYBOARD_CLICK // not a sound nor IKBD option
#define SS_VAR_MSA_CONVERTER // don't prompt if found
#define SS_VAR_STEALTH // don't tell we're an emulator (option)
#define SS_VAR_REWRITE // to conform to what compilers expect (warnings...)
#define SS_VAR_PASTI_ON_WARNING // mention in disk manager title

#ifdef WIN32
#define SS_VAR_UNRAR // using unrar.dll, up to date
#endif

#define SS_VAR_NOTIFY //adding some notify during init

#endif 


///////////
// VIDEO //
///////////

#if defined(SS_VIDEO)

#define SS_VID_BORDERS // option display size
#define SS_VID_BORDERS_LB_DX // rendering-stage rather than painful hacks
//#define SS_VID_AUTOOFF_DECRUNCHING // show borders when decrunching=confusing
#define SS_VID_BPOC // Best Part of the Creation fit display 800




#if defined(SS_DEBUG) 
//#define SHOW_DRAW_SPEED //was already in Steem
//#define SS_VID_VERT_OVSCN_OLD_STEEM_WAY // only for vertical overscan
#endif

#if defined(WIN32) && SSE_VERSION>=350// && defined(_MSC_VER)
#define SS_VID_RECORD_AVI //avifile
#endif

#if defined(WIN32) && SSE_VERSION>=350
#define SS_VID_SAVE_NEO // screenshots in ST Neochrome format
#endif

#define SS_VID_CHECK_DDFS // is video card/display capable?
#define SS_VID_BLIT_TRY_BLOCK
#define SS_VID_ADJUST_DRAWING_ZONE1 // attempt to avoid crash
#define SS_VID_ADJUST_DRAWING_ZONE2

#endif// video


/////////////////////////////////////
// if STEVEN_SEAGAL is NOT defined //
/////////////////////////////////////
#else 
// a fair warning:
#pragma message("Are you mad? You forgot to #define STEVEN_SEAGAL!") 
#include "SSEDecla.h" // still need to neutralise debug macros


#define NO_RAR_SUPPORT // I removed the library, so it's unconditional

#endif//#ifdef STEVEN_SEAGAL

#endif// #ifndef STEVEN_SEAGAL_H 
