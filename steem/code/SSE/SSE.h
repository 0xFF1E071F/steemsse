// Steem Steven Seagal Edition
// SSE.h

/*

This is based on the source code for Steem 3.2 as released by Steem authors,
Ant & Russ Hayward.
Current site for this build: http://ataristeven.host898.net/Steem.htm
SVN code repository is at http://code.google.com/p/steem-engine/ for
v3.3.0 and at http://sourceforge.net/projects/steemsse/  for later versions.

Added some files to the project. 
-In folder 'code\SSE', several files starting with 'SSE', included this one.
-A file div68kCycleAccurate.c in '3rdparty\pasti', to use the correct DIV 
timings found by ijor (also author of Pasti).
-A folder dsp in '3rdparty' for Microwire emulation
-A folder 6301 in '3rdparty' for true emulation of the IKBD
Other mods are in Steem code, inside blocks where STEVEN_SEAGAL is defined.
Some exceptions, like mmu_confused, directly changed, very easy to reverse.
Many other defines are used to segment code. This is heavy but it makes 
debugging a lot easier (real life-savers when something is broken).
To enjoy the new features, you must define STEVEN_SEAGAL!
If not, you should get the last 3.2 build that compiles in VC6 (only
thing changed is no update attempt).

My inane comments outside of defined blocks generally are marked by 'SS:'
They don't mean that I did something cool, only that I comment the source.
		  
VC6 is used as IDE, but also Notepad and the free (and discontinued)
Borland C++ 5.5 compiler, like the original Steem.
Compatibility with those compilers is a requirement of this build.
A Linux build must also be possible (the current version probably doesn't
compile).
The VC6 build should be linked with the C++ library, don't count on system
DLL or it will crash in Windows Vista & 7.
Wich build is better between BCC & VC6 is open to discussion. It seems BCC
refuses to inline more functions but is it better or worse?

SSE.h is supposed to mainly be a collection of compiling switches. It should
include nothing and can be included everywhere.

*/

/*
TODO (future versions, in no definite order)
- Systematic fix for 508/512 cycle counting issue (Omega)
- IPF support (Kryoflux)
- Record video (to AVI)
- Working Linux build
- ACIA/MIDI corrections
- Real timings for 6301 / check SS_IKBD_RUN_IRQ_TO_END: hack?
+ Adjust this with a clock + Captain Blood?
- Event for write to IKBD, but would it be useful?
- TRACE can be commanded by boiler log options ? 
+ redesign system, like all others do: trace(type,msg)? note: no perf loss
- Support for unrar.dll
- Check if TOS1.00 sould boot with 4MB
- Fix FETCH_TIMING/prefetch: not easy
- OSD shifted in 'large' display mode
- 'redraw' in this mode
- better OSD "Steem SSE" logo
- Boiler: real callstack
- Boiler: "run to RTS"
- monochrome demos
- Overdrive/Dragon + a common way for all "unstable" overscan
- Eliminate magic constants in gui
- syndic84
*/


#pragma once // VC guard
#ifndef STEVEN_SEAGAL_H // BCC guard
#define STEVEN_SEAGAL_H


/////////////
// VERSION //
/////////////

#if defined(STEVEN_SEAGAL)
#define SSE_VERSION 340 // check snapshot; rc\resource.rc
#define SSE_VERSION_TXT "SSE 3.4.0" 
#define WINDOW_TITLE "Steem Engine SSE 3.4.0" 
#endif


//////////////////
// BIG SWITCHES //
//////////////////

#if defined(STEVEN_SEAGAL)

#define SS_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SS_BLITTER    // spelled BLiTTER
#define SS_CPU        // M68000 microprocessor
#define SS_FDC        // WD1772 floppy disk controller (just hacks)
#define SS_HACKS      // an option for dubious fixes
#define SS_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#define SS_INTERRUPT  // MC68901 Multi-Function Peripheral Chip, HBL, VBL
#define SS_MMU        // Memory Manager Unit (of the ST, just RAM)
#define SS_OSD        // On Screen Display (drive leds, version)
#define SS_SOUND      // YM2149, STE DMA sound, Microwire
#define SS_STF        // switch STF/STE
#define SS_STRUCTURE  // TODO
#define SS_TOS        // The Operating System
#define SS_UNIX       // TODO
#define SS_VARIOUS    // Mouse capture, keyboard click...
#define SS_VIDEO      // shifter tricks; large borders

#endif


//////////
// TEMP //
//////////

#define SS_TST1 // while adding new features


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

#if defined(SS_CPU_MOVE_W) && defined(SS_VIDEO) // hack: only for move.w
#define SS_CPU_3615GEN4_ULM // changing the scanline being fetched
#endif

//#define SS_CPU_TAS

#if defined(SS_CPU_EXCEPTION)
#define SS_CPU_ASSERT_ILLEGAL // assert before trying to execute
#define SS_CPU_GET_SOURCE // update PC after read - is it a real fix?
#ifndef SS_CPU_GET_SOURCE
#define SS_CPU_PHALEON // opcode based hack for Phaleon (etc.) protection
#endif
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

#if !defined(SS_DEBUG) && (defined(_DEBUG) || defined(DEBUG_BUILD) || defined(SS_BOILER))
#define SS_DEBUG  
#endif

#if defined(SS_DEBUG) && defined(DEBUG_BUILD)
#define SS_DEBUG_CLIPBOARD // right-click on 'dump' to copy then paste
#endif

#if defined(SS_DEBUG)

//#define SS_DEBUG_NO_TRACE
#define SS_DEBUG_OVERRIDE_TRACE // a selection for the released boiler

#if defined(SS_DEBUG_NO_TRACE)

#elif defined(SS_DEBUG_OVERRIDE_TRACE) && !defined(_DEBUG)

#define SS_CPU_EXCEPTION_TRACE
#define SS_DEBUG_START_STOP_INFO
#define SS_FDC_TRACE
#define SS_IKBD_TRACE 
#define SS_IKBD_TRACE_COMMANDS
#define SS_IKBD_TRACE_IO
#define SS_IKBD_6301_DUMP_RAM
#define SS_IKBD_6301_TRACE 
#define SS_IKBD_6301_TRACE_SCI_RX
#define SS_IKBD_6301_TRACE_SCI_TX
#define SS_IKBD_6301_TRACE_KEYS
#define SS_MMU_TRACE
#define SS_RESET_TRACE
#define SS_VID_SHIFTER_EVENTS // recording all shifter events in a frame
#define SS_VID_AUTO_FRAME_REPORT_ON_STOP
#define SS_VID_BORDERS_TRACE
#define SS_VID_REPORT_VBL_TRICKS // a line each VBL
#define SS_VID_TRACE_SUSPICIOUS2 // suspicious +2 & -2
#define SS_VID_END_OF_LINE_CORRECTION_TRACE // their correction

#else // for custom debugging

//#define SS_ACIA_TRACE
//#define SS_ACIA_TRACE_READ_SR // heavy
//#define SS_ACIA_TRACE_IO // heavy when programs poll data register
//#define SS_BLT_TRACE
//#define SS_BLT_TRACE2 // report all blits
//#define SS_CPU_EXCEPTION_TRACE
//#define SS_CPU_PREFETCH_TRACE
//#define SS_DEBUG_REPORT_SKIP_FRAME
//#define SS_DEBUG_START_STOP_INFO
//#define SS_FDC_TRACE // all commands
//#define SS_FDC_TRACE2 // read status register
//#define SS_IKBD_6301_TRACE 
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
//#define SS_IKBD_TRACE 
//#define SS_IKBD_TRACE_COMMANDS // report commands sent to IKBD
//#define SS_IKBD_TRACE_IO // IKBD all reads & writes
//#define SS_IKBD_TRACE_FAKE_CUSTOM // IKBD custom
//#define SS_IKBD_TRACE_6301 // interaction with 6301 emu
//#define SS_IKBD_TRACE_6301_MOUSE // temp
//#define SS_MMU_TRACE
//#define SS_MMU_TRACE2
//#define SS_RESET_TRACE
//#define SS_SOUND_TRACE
//#define SS_INT_TRACE
//#define SS_INT_VBI_START_TRACE//?
//#define SS_STF_TRACE
//#define SS_TOS_TRACE
//#define SS_VID_0BYTE_LINE_TRACE
//#define SS_VID_TRACE_SUSPICIOUS2 // suspicious +2 & -2
//#define SS_VID_END_OF_LINE_CORRECTION_TRACE
//#define SS_VID_LEFT_OFF_COMPARE_STEEM_32
//#define SS_VID_SDP_TRACE 
//#define SS_VID_SDP_TRACE2
//#define SS_VID_SDP_TRACE3 // report differences with Steem v3.2 
//#define SS_VID_VERTICAL_OVERSCAN_TRACE
//#define SS_VID_PALETTE_BYTE_CHANGE_TRACE
#define SS_VID_SHIFTER_EVENTS // recording all shifter events in a frame
#define SS_VID_AUTO_FRAME_REPORT_ON_STOP
//#define SS_VID_REPORT_VBL_TRICKS // a line each VBL
//#define SS_VID_BORDERS_TRACE

#endif//#if defined(SS_DEBUG_NO_TRACE)

#endif


/////////
// FDC //
/////////

#if defined(SS_FDC)

//#define SS_FDC_IDF // IPF (Kryoflux) support...
/*
    for loading/init/etc the DLL, look at pasti (USE_PASTI) and
    duplicate
    apparently it's very different from pasti, that's no WD emulation
    instead we get raw data and density info, the rest is up to us!
    so it could be that we get it working first with trivial cases
    but not protected software
*/

#if defined(SS_HACKS) // all "protected" by the hack option

#define SS_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari
#define SS_FDC_CHANGE_SECTOR_WHILE_BUSY // from Hatari
#define SS_FDC_CHANGE_TRACK_WHILE_BUSY // from Hatari
#define SS_FDC_RESTORE // adding delay (fast+ADAT)
#define SS_FDC_SEEK_VERIFY // adding delay (fast)
#define SS_FDC_STARTING_DELAY // adding delay (fast)
#define SS_FDC_STREAM_SECTORS_SPEED // faster in fast mode, slower with ADAT!

#endif

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
#define SS_IKBD_RUN_IRQ_TO_END // kind of a hack TODO
#if defined(SS_IKBD_RUN_IRQ_TO_END)
#define SS_IKBD_6301_ADJUST_CYCLES
#endif
#define SS_IKBD_6301_SET_TDRE
#define SS_IKBD_6301_TIMER_FIX // not sure there was a problem
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
#define SS_MFP_RATIO_STE // measured for STE
#define SS_MFP_RATIO_STF // theoretical for STF
#endif

#if defined(SS_INT_TIMER_B)
//#define SS_INT_TIMER_B_NO_WOBBLE // does it fix anything???
#define SS_INT_TIMER_B_AER // earlier trigger (from Hatari)
#endif

#if defined(SS_STF) && defined(SS_INT_VBL)
//#define SS_INT_VBI_START // ideal but many problems, eg Calimero
#define SS_INT_VBL_STF // more a hack but easier to do
#endif

#endif


/////////
// MMU //
/////////

#if defined(SS_MMU)

#define SS_MMU_WRITE // programs in RAM may write in the MMU
////#define SS_MMU_NO_CONFUSION // would need to patch TOS then = regression!

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
// note v3.4 no build has been attempted yet
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

#if defined(SS_VARIOUS)

#define SS_VAR_MOUSE_CAPTURE 
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SS_VAR_F12 // F12 starts/stops emulation
#endif
#define SS_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen
//#define SS_VAR_HIDE_OPTIONS_AT_START
#define SS_VAR_KEYBOARD_CLICK // not a sound nor IKBD option
#define SS_VAR_MSA_CONVERTER // don't prompt if found
#define SS_VAR_STEALTH // don't tell we're an emulator (option)
#define SS_VAR_REWRITE // to conform to what compilers expect (warnings...)
#define SS_SSE_OPTION_PAGE // a new page for all our options
#define SS_SSE_OPTION_STRUCT // structure with constructor

#define NO_RAR_SUPPORT // the library is outdated, right?

#endif 


///////////
// VIDEO //
///////////

#if defined(SS_VIDEO)

#define SS_VID_BORDERS // option display size
#define SS_VID_BORDERS_LB_DX // rendering-stage rather than painful hacks
#define SS_VID_STEEM_EXTENDED  // based on Steem system
//#define SS_VID_STEEM_ORIGINAL // only for debugging/separate blocks, careful

//#define SS_VID_RECORDING //TODO (record a video file of rendering)

#if defined(SS_DEBUG) 
//#define SS_VIDEO_DRAW_DBG  // totally bypass CheckSideOverscan() & Render() !
//#define SS_VID_VERT_OVSCN_OLD_STEEM_WAY // only for vertical overscan
//#define SS_VID_SKIP_SCANLINE -29 // fetch but only draw colour 0
//#define SS_VID_VERT_OVSCN_OLD_STEEM_WAY
#else
#define draw_check_border_removal Shifter.CheckSideOverscan
#define draw_scanline_to(cycles_since_hbl) Shifter.Render(cycles_since_hbl)
#endif

// features

#if defined(SS_VID_STEEM_EXTENDED)

#define SS_VID_0BYTE_LINE

#if defined(SS_VID_0BYTE_LINE)
#define SS_VID_0BYTE_LINE_RES_END
#define SS_VID_0BYTE_LINE_RES_HBL
#define SS_VID_0BYTE_LINE_RES_START
#define SS_VID_0BYTE_LINE_SYNC
#endif

#define SS_VID_4BIT_SCROLL
#define SS_VID_60HZ_OVERSCAN
#define SS_VID_AUTOOFF_DECRUNCHING // show borders when decrunching
#define SS_VID_END_OF_LINE_CORRECTION // correct +2, -2 lines (like in Hatari)
#define SS_VID_LINE_PLUS_2_THRESHOLD
#define SS_VID_MED_OVERSCAN
#define SS_VID_NON_STOPPING_LINE // Enchanted Land
#define SS_VID_PALETTE_BYTE_CHANGE 
#define SS_VID_STE_LEFT_OFF // 224 byte scanline
#define SS_VID_STE_MED_HSCROLL
#define SS_VID_STE_HSCROLL_LEFT_OFF
#if defined(SS_STF)
#define SS_STF_VERTICAL_OVERSCAN
#endif
#define SS_VID_STE_VERTICAL_OVERSCAN
#define SS_VID_UNSTABLE_LEFT_OFF // DoLB, Omega, Overdrive/Dragon
#endif

#define SS_VID_SDP // SDP=shifter draw pointer

#if defined(SS_VID_SDP)
#define SS_VID_SDP_READ
#define SS_VID_SDP_WRITE
#define SS_VID_SDP_WRITE_ADD_EXTRA
#define SS_VID_SDP_WRITE_LOWER_BYTE
#endif

#if defined(SS_HACKS)
// most hacks concern SDP, there's room for improvement
#define SS_VID_SDP_WRITE_DE_HSCROLL
#define SS_VID_SDP_WRITE_MIDDLE_BYTE // stable
#define SS_VID_BIG_WOBBLE // Big Wobble shift
#define SS_VID_BPOC // Best Part of the Creation fit display 800
#define SS_VID_DANGEROUS_FANTAISY // Dangerous Fantaisy credits flicker
#if defined(SS_STF_WAKE_UP)
#define SS_VID_DRAGON // confused shifter, temp hack
#endif
#define SS_VID_NO_COOPER_GREETINGS // No Cooper Greetings shift
#define SS_VID_OMEGA  // Omega Full Overscan shift (60hz)
#define SS_VID_PACEMAKER // Pacemaker credits flickering line
#define SS_VID_SCHNUSDIE // Reality is a Lie/Schnusdie overscan logo
#define SS_VID_TCB // Swedish New Year Demo/TCB SDP (60hz)
#endif

#endif// video

#else // if STEVEN_SEAGAL is NOT defined
// a fair warning:
#pragma message("Are you mad? You forgot to #define STEVEN_SEAGAL!") 
#include "SSEDecla.h" // still need to neutralise debug macros

#endif//#ifdef STEVEN_SEAGAL

#endif// #ifndef STEVEN_SEAGAL_H 
