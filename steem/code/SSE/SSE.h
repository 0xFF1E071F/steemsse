#pragma once // VC guard
#ifndef STEVEN_SEAGAL_H // BCC guard
#define STEVEN_SEAGAL_H

/*
Steem Steven Seagal Edition (SSE)
---------------------------------

This is based on the source code for Steem 3.2 as released by Steem authors,
Ant & Russ Hayward.
Current site for this build: http://ataristeven.eu5.org/Steem.htm
SVN code repository is at http://code.google.com/p/steem-engine/ for
v3.3.0 and at http://sourceforge.net/projects/steemsse/  for later versions.

Added some files to the project. 
-In folder 'code\SSE', several files starting with 'SSE', including this one.
-A folder 6301 in '3rdparty' for true emulation of the IKBD
-A folder avi in '3rdparty' for recording video to AVI support
-A folder caps in '3rdparty' for IPF disk image format support
-A folder dsp in '3rdparty' for Microwire emulation
-A file div68kCycleAccurate.c in '3rdparty\pasti', to use the correct DIV 
timings found by ijor (also author of Pasti).

Other mods are in Steem code, inside blocks where STEVEN_SEAGAL is defined.
Some exceptions, like mmu_confused, directly changed, very easy to reverse.
(Only the name of the variable is changed).
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

*/

/*
DONE
- Working (compiling & running) Linux build of 3.4
- Check fullscreen mode (crashes reported): 3.4.1
- IPF support (Kryoflux) (for V.3.5)
- Record video (to AVI) (1st version, for v3.5)
- Pasti: wrong red led
- TRACE can be commanded by boiler log options (started)
- Disk manager window title mentions 'Pasti On' if active
- Support for unrar.dll (for V.3.5)
- Save snapshot in NEO format (v3.5)
- nuked WinSTon import, it's outdated
- nuked update process
- F12 as shortcut will do nothing
- crash if screen saver
- on rte fix vs save snapshot (still not ideal)
- 6301 emu debug: more accurate clock in Captain Blood
*/

/*
TODO (future versions, in no definite order)
- printing directly on a PC printer?
- Systematic fix for 508/512 cycle counting issue (Omega)
- ACIA/MIDI corrections
- Event for write to IKBD, but would it be useful?
- Check if TOS1.00 sould boot with 4MB (MMU)
- Fix FETCH_TIMING/prefetch: not easy (is there something to fix?)
- OSD shifted in 'large' display mode
- 'redraw' in this mode
- better OSD "Steem SSE" logo
- Boiler: real callstack
- Boiler: "run to RTS"
- monochrome demos
- Overdrive/Dragon + a common way for all "unstable" overscan
- Eliminate magic constants in gui
- unix: SSE options icon
- Boiler: remove DIV logging,we know the timing now
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
*/


/////////////
// VERSION //
/////////////

#if defined(STEVEN_SEAGAL)
#define BETA
#ifdef BETA // beta with all features
#define SSE_VERSION 350 // check snapshot; rc\resource.rc
#define SSE_VERSION_TXT "SSE Beta" 
#define WINDOW_TITLE "Steem SSE beta"//"Steem Engine SSE 3.5.0" 
#else // next planned release
#define SSE_VERSION 350 // check snapshot; rc\resource.rc
#define SSE_VERSION_TXT "3.5.0" 
#define WINDOW_TITLE "Steem Engine SSE 3.5.0" 
#endif
#undef BETA
#endif


//////////////////
// BIG SWITCHES //
//////////////////

#if defined(STEVEN_SEAGAL)

#define SS_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SS_BLITTER    // spelled BLiTTER
#define SS_CPU        // M68000 microprocessor
#define SS_DMA        // Custom Direct Memory Access chip (disk)
#define SS_FDC        // WD1772 floppy disk controller (hacks, IPF)
#define SS_HACKS      // an option for dubious fixes
#define SS_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#define SS_IPF        // CAPS support (IPF disks) 
#define SS_INTERRUPT  // MC68901 Multi-Function Peripheral Chip, HBL, VBL
#define SS_MIDI       // TODO
#define SS_MMU        // Memory Manager Unit (of the ST, just RAM)
#define SS_OSD        // On Screen Display (drive leds, version)
#define SS_SHIFTER    // The legendary custom shifter and all its tricks
#define SS_SOUND      // YM2149, STE DMA sound, Microwire
#define SS_STF        // switch STF/STE
#define SS_STRUCTURE  // TODO
#define SS_TOS        // The Operating System
#define SS_UNIX       // Linux build must be OK too (may lag)
#define SS_VARIOUS    // Mouse capture, keyboard click, unrar...
#define SS_VIDEO      // large borders, screenshot, recording

#endif

#ifdef WIN32
#undef SS_UNIX
#endif
#if !defined(SS_DMA) || !defined(SS_FDC)
#undef SS_IPF
#endif
#if defined(SS_UNIX)
#undef SS_IPF
//#define SS_UNIX_IPF//TODO
#endif


//////////
// TEMP //
//////////

//#define SS_TST1 


///////////////////////
// DETAILED SWITCHES //
///////////////////////


#if defined(STEVEN_SEAGAL)


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
//#define SS_BLT_TRACE2 // report all blits

#endif


/////////
// CPU //
/////////

#if defined(SS_CPU)  

#define SS_CPU_EXCEPTION    // crash like Windows 98
#define SS_CPU_PREFETCH     // fetch like a dog
#define SS_CPU_CLR          // read before writing (changes nothing?)
#define SS_CPU_DIV          // divide like Caesar
#define SS_CPU_MOVE_B       // move like a superstar
#define SS_CPU_MOVE_W       
#define SS_CPU_MOVE_L
#define SS_CPU_POKE         // poke like a C64 (inline in VC6)

//#define SS_CPU_EXCEPTION_TRACE_PC // reporting all PC
//#define SS_CPU_PREFETCH_TRACE

#if defined(SS_CPU_MOVE_W) && defined(SS_VIDEO) // hack: only for move.w
#define SS_CPU_3615GEN4_ULM // changing the scanline being fetched
#endif

//#define SS_CPU_TAS

#if defined(SS_CPU_EXCEPTION)
#define SS_CPU_ASSERT_ILLEGAL // assert before trying to execute
#define SS_CPU_GET_SOURCE // update PC after read - is it a real fix?
#ifndef SS_CPU_GET_SOURCE
//#define SS_CPU_MAY_WRITE_0//tst
#define SS_CPU_PHALEON // opcode based hack for Phaleon (etc.) protection
#endif

#define SS_CPU_LINE_F // for interrupt depth counter

#ifdef SS_CPU_PREFETCH_______
#define SS_CPU_WAR_HELI2 // other way for Wat Heli (trying)  / REDO
#else
#define SS_CPU_WAR_HELI // isolate a hack already in v3.2
#endif
#define SS_CPU_POST_INC // no post increment if exception
#endif

#endif


///////////
// DEBUG //
///////////

// TODO finish conversion to logsection use

#if !defined(SS_DEBUG) && (defined(_DEBUG) || defined(DEBUG_BUILD) || \
defined(SS_BOILER))
#define SS_DEBUG  //use lost?
#endif

#if defined(SS_DEBUG)

//#define SS_DEBUG_TRACE_FILE // if IDE can't follow, use file

#if defined(DEBUG_BUILD) //TODO add other mods here
#define SS_DEBUG_CLIPBOARD // right-click on 'dump' to copy then paste
#endif

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

#endif//#if defined(SS_DEBUG_NO_TRACE)

#else // no SS_DEBUG

#endif


/////////
// DMA //
/////////

#if defined(SS_DMA)

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

#define SS_FDC_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
//& don't remove?
#define SS_FDC_CHANGE_SECTOR_WHILE_BUSY // from Kryoflux
#define SS_FDC_CHANGE_TRACK_WHILE_BUSY // from Kryoflux

//#define SS_FDC_READ_HIGH_BYTE // like pasti, 0

#if defined(SS_HACKS) // all "protected" by the hack option

#define SS_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari

#define SS_FDC_NO_DISK // ignore commands if there's no disk inserted
// no, must sendIRQ (see pasti)
#define SS_FDC_RESTORE // adding delay (fast+ADAT)
#define SS_FDC_SEEK_VERIFY // adding delay (fast)
#define SS_FDC_STARTING_DELAY // adding delay (fast)

#define SS_FDC_STREAM_SECTORS_SPEED // faster in fast mode, slower with ADAT!

#endif//hacks
//#define SS_FDC_TRACE_STATUS //spell out status register

#endif


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
#define SS_IKBD_RUN_IRQ_TO_END // kind of a hack
#define SS_IKBD_6301_ADJUST_CYCLES // stay in sync!
#define SS_IKBD_6301_SET_TDRE
#define SS_IKBD_6301_TIMER_FIX // not sure there was a problem

//#define SS_IKBD_TRACE_COMMANDS // report commands sent to IKBD
//#define SS_IKBD_TRACE_FAKE_CUSTOM // IKBD custom
//#define SS_IKBD_TRACE_6301 // interaction with 6301 emu
//#define SS_IKBD_TRACE_6301_MOUSE // temp
//#define SS_IKBD_6301_TRACE // ambigous!
#if defined(SS_IKBD_6301_TRACE)
//#define SS_IKBD_6301_TRACE_SCI_RX
//#define SS_IKBD_6301_TRACE_SCI_TX
//#define SS_IKBD_6301_TRACE_SCI_TRCSE
//#define SS_IKBD_6301_TRACE_INT_TIMER
//#define SS_IKBD_6301_TRACE_INT_SCI
//#define SS_IKBD_6301_DISASSEMBLE_CUSTOM_PRG 
//#define SS_IKBD_6301_DISASSEMBLE_ROM 
//#define SS_IKBD_6301_DUMP_RAM
//#define SS_IKBD_6301_TRACE_KEYS
#endif

#endif//#if defined(SS_IKBD_6301)

//#define SS_IKBD_TRACE_IO

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
#ifdef BETA
#define SS_IPF_LETHAL_XCESS // hack useful with capsimg v4.2
#define SS_IPF_WRITE_HACK // useless hack for v4.2 AFAIK
#endif
//#define SS_IPF_SAVE_WRITES //TODO?
//#define SS_IPF_TRACE_SECTORS // show sector info

#endif


/////////
// MMU //
/////////

#if defined(SS_MMU)

#define SS_MMU_WRITE // programs in RAM may write in the MMU
////#define SS_MMU_NO_CONFUSION // would need to patch TOS then = regression!
//#define SS_MMU_TRACE2
#endif


/////////
// OSD //
/////////

#if defined(SS_OSD)
///#define SS_OSD_LOGO //TODO
#define SS_OSD_DRIVE_LED
#endif


/////////////
// SHIFTER //
/////////////

#if defined(SS_SHIFTER)

//#define SS_SHIFTER_STEEM_ORIGINAL // only for debugging/separate blocks

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
#define SS_SHIFTER_NON_STOPPING_LINE // Enchanted Land
#define SS_SHIFTER_PALETTE_BYTE_CHANGE 
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

#endif

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
#define SS_SHIFTER_NO_COOPER_GREETINGS // No Cooper Greetings shift
#define SS_SHIFTER_OMEGA  // Omega Full Overscan shift (60hz)
#define SS_SHIFTER_PACEMAKER // Pacemaker credits flickering line
#define SS_SHIFTER_SCHNUSDIE // Reality is a Lie/Schnusdie overscan logo
#define SS_SHIFTER_TCB // Swedish New Year Demo/TCB SDP (60hz)
#endif

#if defined(SS_DEBUG) 
//#define SS_SHIFTER_DRAW_DBG  // totally bypass CheckSideOverscan() & Render()
#define SS_SHIFTER_EVENTS // recording all shifter events in a frame
#define SS_SHIFTER_EVENTS_ON_STOP // each time we stop emulation
//#define SS_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
//#define SS_SHIFTER_VERTICAL_OVERSCAN_TRACE
#else
#define draw_check_border_removal Shifter.CheckSideOverscan
#define draw_scanline_to(cycles_since_hbl) Shifter.Render(cycles_since_hbl)
#endif

#endif


///////////
// SOUND //
///////////

#if defined(SS_SOUND)

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
#define SS_STF_WAKE_UP // wake up states managed in Steem!
// TODO: Falcon, Mega STE, TT... yeah right

#endif


/////////
// TOS //
/////////

#if defined(SS_TOS)

#ifdef SS_HACKS
#define SS_TOS_PATCH106 // a silly bug, a silly hack
#endif

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

#define SS_VAR_NO_UPDATE // remove all code in relation to updating
#define SS_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah


#define SS_VAR_MOUSE_CAPTURE 
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SS_VAR_F12 // F12 starts/stops emulation
#endif
#define SS_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen
//#define SS_VAR_HIDE_OPTIONS_AT_START // hack before debugging
#define SS_VAR_KEYBOARD_CLICK // not a sound nor IKBD option
#define SS_VAR_MSA_CONVERTER // don't prompt if found
#define SS_VAR_STEALTH // don't tell we're an emulator (option)
#define SS_VAR_REWRITE // to conform to what compilers expect (warnings...)
#define SS_SSE_OPTION_PAGE // a new page for all our options
#define SS_SSE_OPTION_STRUCT // structure SSEOption (necessary!)

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
//#define SS_VID_VERT_OVSCN_OLD_STEEM_WAY
#endif

#if defined(WIN32) && SSE_VERSION>=350// && defined(_MSC_VER)
#define SS_VID_RECORD_AVI //avifile
#endif

#if defined(WIN32) && SSE_VERSION>=350
#define SS_VID_SAVE_NEO // screenshots in ST Neochrome format
#endif


#endif// video


/////////////////////////////////////
// if STEVEN_SEAGAL is NOT defined //
/////////////////////////////////////
#else 
// a fair warning:
#pragma message("Are you mad? You forgot to #define STEVEN_SEAGAL!") 
#include "SSEDecla.h" // still need to neutralise debug macros

#endif//#ifdef STEVEN_SEAGAL

#endif// #ifndef STEVEN_SEAGAL_H 
