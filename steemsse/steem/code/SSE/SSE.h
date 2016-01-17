// for v3.8.0
#pragma once // VC guard
#ifndef STEVEN_SEAGAL_H // BCC guard
#define STEVEN_SEAGAL_H

/*

Steem Steven Seagal Edition (SSE)
---------------------------------

This is based on the source code for Steem R63 as released by Steem authors,
Ant & Russ Hayward.
SVN code repository is at:
 http://sourceforge.net/projects/steemsse/
Homepage:
 http://ataristeven.exxoshost.co.uk/

Added some files to the project. 
- acia.decla.h, key_table.cpp in 'steem\code'.
-In folder 'steem\code\SSE', several files starting with 'SSE', including 
this one.
-In folder 'steem\doc\SSE', some doc files (mainly done by me, but sometimes
only by composing other docs)
-A folder '6301' in '3rdparty' for true emulation of the IKBD
-A folder 'avi' in '3rdparty' for recording video to AVI support
-A folder 'caps' in '3rdparty' for IPF/CTR disk image format support
-A folder 'caps_linux' in '3rdparty' for future (?) IPF disk image format 
support
-A folder 'd3d' in '3rdparty' to compile the D3D9 parts with BCC
-A folder 'doc' in '3rdparty' for some doc files (done by others)
-A folder 'dsp' in '3rdparty' for Microwire emulation
-A file 'div68kCycleAccurate.c' in '3rdparty\pasti', to use the correct DIV 
timings found by ijor (also author of Pasti).
-A folder 'SDL-WIN' for future (?) SDL support
-A folder 'unRARDLL' in '3rdparty' for unrar support
-A folder 'various' in '3rdparty'
-A folder 'ArchiveAccess' in '3rdparty' for 7z support
-A folder 'hfe' in '3rdparty' for HFE support
-Files xxx.decla.h to better separate declaration/implementation
TODO: restore previous h files

Other mods are in Steem code, inside blocks where STEVEN_SEAGAL is defined.
Many other defines are used to segment code. This is heavy but it makes 
debugging a lot easier (real life-savers when something is broken).

Switches are sorted by version, sometimes you must search a bit.

To enjoy the new features, you must define STEVEN_SEAGAL!
If not, you should get the last 3.2 build that compiles in VC6 (only
thing changed is no update attempt).
TODO: make a 320/1 version instead

My inane comments outside of defined blocks generally are marked by 'SS:'
They don't mean that I did something cool, only that I comment the source.

I removed nothing from the original source code or comments.

Since v3.7, the main build is the VS2008 one (before, it was VC6).
The BCC build is much used for development. Borland compiler is much faster
than the Microsoft ones.
The MinGW build is experimental.
A Unix (gcc) version is also more or less maintained, with fewer features
than the Windows versions.

The VC6 build should be linked with the C++ library, don't count on system
DLL or it will crash in Windows Vista & 7. 

Since v3.7.3, the VS2008 build uses the DLL C++ library.

Project files are included.
    
SSE.h is supposed to mainly be a collection of compiling switches (defines).
It should include nothing and can be included everywhere.
Normally switches are optional, but they also are useful just to mark
code changes.

SSE_DEBUG, if needed, should be defined in the project/makefile.
It has an effect on both the boiler and the VS debug build.

LEGACY_BUILD is now needed to build Steem using the 3 big modules
Steem Emu and Helper.
Recent SSE features won't be built.
Those configurations are marked with '_modules'.
When it's not defined, more separate units are compiled.


Each author may define his name.

eg:

#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH)
 //... (awesome mod)
#endif

#if defined(JEAN_CLAUDE_VAN_DAMME) && defined(SSE_CPU_PREFETCH)
 //... (awesome mod)
#endif


So if later Jean Claude Van Damme disrespects me again, all we have to do is:

#undef JEAN_CLAUDE_VAN_DAMME

and all his silly mods are gone!

*/


/*

Important (note to self)

Release: not SSE_BETA
All SSE_TEST_ON must go
Version for snapshot (in LoadSave.h) + Windows properties (rc\resource.rc)

Beta: not SSE_PRIVATE_BUILD

*/

#if defined(STEVEN_SEAGAL)


/////////////
// VERSION //
/////////////

#define SSE_VERSION 380 // versions down to 340 still compile
                        // full v330 not publicly available anymore

#if SSE_VERSION>373 //last release
#define SSE_BETA //title, OSD, plus some testing - new features
#define SSE_BETA_BUGFIX // beta for just bugfixes
#if defined(SSE_BETA) || defined(SSE_BETA_BUGFIX)
///#define SSE_PRIVATE_BUILD // my "beta" option
#endif
#endif


//////////////////
// BIG SWITCHES //
//////////////////

#define SSE_BLITTER    // spelled BLiTTER by those in the known!
#define SSE_COMPILER  //  warnings, errors... 
#define SSE_CPU        // MC68000 microprocessor
#define SSE_GLUE       // General Logic Unit
#define SSE_GUI        // Graphic User Interface
#define SSE_HACKS      // an option for dubious fixes
#define SSE_HARDDISK
#define SSE_INTERRUPT  // HBL, VBL, MFP
#define SSE_FLOPPY     // DMA, FDC, Pasti, etc
#define SSE_KEYBOARD   // ACIA, IKBD
#define SSE_MIDI       // Musical Instrument Digital Interface
#define SSE_MMU        // Memory Manager Unit (of the ST, no paging)
#define SSE_OSD        // On Screen Display (drive leds, track info, logo)
#define SSE_ROM        // Cartridge, TOS
//#define SSE_SDL        // Simple DirectMedia Layer (TODO)
#define SSE_SHIFTER    // The legendary custom Shifter and all its tricks
#define SSE_SOUND      // YM2149, STE DMA sound, Microwire
#define SSE_STF        // switch STF/STE
#define SSE_STRUCTURE //  now necessary for the STEVEN_SEAGAL build anyway TODO
#define SSE_TIMINGS    // misc. timings
#ifdef UNIX
#define SSE_UNIX       // Unix build of Steem SSE
#endif
#define SSE_VARIOUS    // Mouse capture, keyboard click, unrar...
#define SSE_VIDEO      // large borders, screenshot, recording


//////////////
// COMPILER //
//////////////

#define NO_RAR_SUPPORT

#if defined(SSE_COMPILER)

#ifdef WIN32

#if _MSC_VER>1200 // higher than VC6
#define SSE_VS2003_DELAYDLL
#endif

#if _MSC_VER >= 1500 
#define SSE_VS2008 // so that the VS2008 build compiles
//#define SSE_INLINE_370
#define SSE_VS2008_WARNING_370
#define SSE_VS2008_WARNING_371
#endif

#define SSE_DELAY_LOAD_DLL // can run without DLL//never worked with bcc???

#endif//w32

// #define SSE_LEAN_AND_MEAN //TODO

#endif//SSE_COMPILER


///////////////
// STRUCTURE //
///////////////


#if defined(SSE_STRUCTURE)
/* Those were dev switches. 
   TODO Steem "not SSE" builds the ancient way, Steem "SSE" builds using
   decla.h files exclusively
*/

#if defined(LEGACY_BUILD)
#define SSE_STRUCTURE_DECLA // for all features
#endif

#if !defined(LEGACY_BUILD)
#define SSE_STRUCTURE_DECLA // necessary
#define SSE_STRUCTURE_SSE_OBJ // try to have all separate SSE objects
#endif

//3 switches appeared in v3.5.0...
#define SSE_SSE_OPTION_PAGE // a new page for all our options
#define SSE_SSE_OPTION_STRUCT // structure SSEOption 
#define SSE_SSE_CONFIG_STRUCT // structure SSEConfig 

#if defined(SSE_STRUCTURE_DECLA)
// those were dev switches 
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
#define SSE_STRUCTURE_HARDDISKMAN_H //3.7.2
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
#endif//SSE_STRUCTURE_DECLA

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

#if defined(SSE_STRUCTURE)
#define SSE_STRUCTURE_CPU_POKE_NOINLINE //little detail 3.6.0
#define SSE_STRUCTURE_IOR//3.5.1
#endif

#endif//structure


//////////
// v3.3 //
//////////
/*  v3.3 can't really be built as it was because Hatari code for keyboard and
    video, used in v3.3, was removed in v3.4 (so that comments don't appear
    in search engines)
*/

#if SSE_VERSION>=330

#define SS_HATARI // won't work...

#if defined(SSE_CPU)
#define SSE_CPU_EXCEPTION    // crash like Windows 98
#define SSE_CPU_INSTR        // rewriting some instructions
#define SSE_CPU_PREFETCH     // prefetch like a dog
#if defined(SSE_CPU_PREFETCH)
#define SSE_CPU_PREFETCH_4PIXEL_RASTERS //see SSECpu.cpp
#endif
#if defined(SSE_CPU_INSTR)
#define SSE_CPU_DIV          // divide like Caesar
#define SSE_CPU_MOVE_B       // move like a superstar
#define SSE_CPU_MOVE_W       
#define SSE_CPU_MOVE_L
#endif//instr
#endif//cpu
#if defined(SSE_INTERRUPT)
#define SSE_INT_MFP        // MC68901 Multi-Function Peripheral Chip
#endif
#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_IACK_LATENCY //was SS_MFP_PENDING in v3.3 undef 3.7.0
#define SSE_INT_MFP_RATIO // change the values of CPU & MFP freq!
#define SSE_INT_MFP_TIMER_B 
#if defined(SSE_INT_MFP_RATIO) 
#define SSE_INT_MFP_RATIO_STF // theoretical for STF
#endif
#endif//mfp
#if defined(SSE_MMU)
#define SSE_MMU_WRITE // programs in RAM may write in the MMU
#endif
#if defined(SSE_OSD)
#define SSE_OSD_DRIVE_LED
#endif
#if defined(SSE_SHIFTER)
#define SSE_SHIFTER_IO // move blocks from ior, iow, necessary
#define SSE_SHIFTER_SDP // SDP=Shifter draw pointer
#if defined(SSE_SHIFTER_TRICKS) // see old source
#endif
#endif//sft
#if defined(SSE_STF)
#define SSE_STF_0BYTE
#define SSE_STF_BLITTER
#define SSE_STF_DMA
#define SSE_STF_HSCROLL
#define SSE_STF_LEFT_OFF 
#define SSE_STF_LINEWID
#define SSE_STF_MMU_PREFETCH
#define SSE_STF_PADDLES
#define SSE_STF_PAL
#define SSE_STF_SDP
#define SSE_STF_SHIFTER_IOR
#define SSE_STF_VBASELO
#define SSE_STF_VERT_OVSCN
#endif
#if defined(SSE_VARIOUS)
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SSE_GUI_F12 // F12 starts/stops emulation
#endif
#define SSE_GUI_MOUSE_CAPTURE 
#define SS_VAR_PROG_ID //X Program ID
#endif//var
#if defined(SSE_VIDEO) && defined(WIN32)
#define SSE_VID_BORDERS // option display size
#define SSE_VID_BPOC // Best Part of the Creation fit display 800 hack
#endif//vid

#endif//3.3.0



//////////
// v3.4 //
//////////


#if SSE_VERSION>=340

#undef SS_HATARI

#if defined(SSE_KEYBOARD)
#define SSE_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SSE_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#endif
#if defined(SSE_ROM)
#define SSE_TOS        // The Operating System
#endif
#if defined(SSE_FLOPPY)
#define SSE_FDC        // WD1772 floppy disk controller
#endif
#if defined(SSE_ACIA)
#define SSE_ACIA_BUS_JAM_NO_WOBBLE // simple "fix" //undef 3.6.4
#define SSE_ACIA_DOUBLE_BUFFER_TX // only to 6301 (not MIDI)
#define SSE_ACIA_IRQ_DELAY // only from 6301 (not MIDI) // undef 3.5.2
#endif
#if defined(SSE_BLITTER)
#define SSE_BLT_BLIT_MODE_CYCLES // #cycles in BLIT Mode in NOPs, not M68K cycles
#define SSE_BLT_BLIT_MODE_INTERRUPT // trigger at once (not after blit phase)
#define SSE_BLT_HOG_MODE_INTERRUPT // no interrupt in hog mode
#endif
#if defined(SSE_CPU)
#define SSE_CPU_POKE         // poke like a C64 
#if defined(SSE_CPU_EXCEPTION)
#define SSE_CPU_ASSERT_ILLEGAL // assert before trying to execute (not general)
#define SSE_CPU_POST_INC // no post increment if exception (Beyond)
#endif//exc
#if defined(SSE_CPU_POKE) && defined(SSE_SHIFTER)
#define SSE_CPU_CHECK_VIDEO_RAM_B
#define SSE_CPU_CHECK_VIDEO_RAM_L
#define SSE_CPU_CHECK_VIDEO_RAM_W // including: 36.15 GEN4 by ULM
#endif //poke
#endif//cpu
#if defined(SSE_FDC) 
#define SSE_FDC_ACCURATE
#endif
#if defined(SSE_FDC_ACCURATE)
#define SSE_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari
#define SSE_FDC_CHANGE_SECTOR_WHILE_BUSY // from Hatari or Kryoflux
#define SSE_FDC_CHANGE_TRACK_WHILE_BUSY // from Hatari or Kryoflux
// #define SS_FDC_KRYOFLUX //TODO
#define SSE_FDC_RESTORE
// #define SS_FDC_SEEK_VERIFY // adding delay (fast)
// #define SS_FDC_STARTING_DELAY // adding delay (fast)
// #define SS_FDC_STREAM_SECTORS_SPEED // faster in fast mode, slower with ADAT!
#endif//adat
#if defined(SSE_IKBD)
#define SSE_IKBD_6301 // HD6301 true emu, my pride!
#if defined(SSE_IKBD_6301)
#define SSE_IKBD_6301_ADJUST_CYCLES // stay in sync (check with clock)
#define SSE_IKBD_6301_CHECK_IREG_RO // some registers are read-only
#define SSE_IKBD_6301_CHECK_IREG_WO // some registers are write-only
#define SSE_IKBD_6301_DISABLE_BREAKS // to save 64k RAM (we still consume 64k)
#define SSE_IKBD_6301_DISABLE_CALLSTACK // to save 3k on the PC stack
#define SSE_IKBD_6301_RUN_IRQ_TO_END // hack around Sim6xxx's working
#define SSE_IKBD_6301_SET_TDRE
#define SSE_IKBD_6301_TIMER_FIX // not sure there was a problem
#endif//6301
#endif//ikbd
#if defined(SSE_INTERRUPT)
#define SSE_INT_JITTER // from Hatari (option 6301/Acia unchecked)
#define SSE_INT_HBL
#define SSE_INT_VBL  
#if defined(SSE_INT_JITTER) && defined(SSE_INT_HBL)
#define SSE_INT_JITTER_HBL
#endif
#if defined(SSE_INT_VBL)
//#define SSE_INT_VBI_START //MFD, was never defined in any release
#endif
#if defined(SSE_INT_JITTER) && defined(SSE_INT_VBL) && defined(SSE_STF)
#define SSE_INT_JITTER_VBL
#endif
#endif//int
#if defined(SSE_INT_MFP)
#if defined(SSE_INT_MFP_RATIO) 
// #define SS_MFP_ALL_I_HAVE // temp silly,useless hack! TODO
#define SSE_INT_MFP_POST_INT_LATENCY // hardware quirk? //code?
#define SSE_INT_MFP_RATIO_STE // measured (by Steem Authors) for STE?
#endif
#if defined(SSE_INT_MFP_TIMER_B)
#define SSE_INT_MFP_TIMER_B_AER // earlier trigger (from Hatari)
///#define SSE_INT_MFP_TIMER_B_NO_WOBBLE
#endif
#endif//mfp
#if defined(SSE_SDL)
#define SSE_SDL_DEACTIVATE
#endif
#if defined(SSE_SHIFTER)
#define SSE_SHIFTER_TRICKS  // based on Steem system, extended
#if defined(SSE_SHIFTER_TRICKS)
#define SSE_SHIFTER_0BYTE_LINE
#if defined(SSE_SHIFTER_0BYTE_LINE) 
//#define SSE_SHIFTER_0BYTE_LINE_RES_END //No Buddies Land
//#define SSE_SHIFTER_0BYTE_LINE_RES_HBL //Beyond/Pax Plax Parallax
//#define SSE_SHIFTER_0BYTE_LINE_RES_START //Nostalgic-O/Lemmings
//#define SSE_SHIFTER_0BYTE_LINE_SYNC //Forest
#endif
#define SSE_SHIFTER_4BIT_SCROLL //Let's do the Twist again
#define SSE_SHIFTER_60HZ_OVERSCAN //Leavin' Teramis
#define SSE_SHIFTER_LINE_PLUS_2_THRESHOLD //Forest
#define SSE_SHIFTER_MED_OVERSCAN // BPOC
#define SSE_SHIFTER_NON_STOPPING_LINE // Enchanted Land
#define SSE_SHIFTER_PALETTE_BYTE_CHANGE //undef v3.6.3
//#define SSE_SHIFTER_UNSTABLE_LEFT_OFF // DoLB, Omega, Overdrive/Dragon old hack
#endif
#if defined(SSE_SHIFTER_SDP)
#define SSE_SHIFTER_SDP_READ
#define SSE_SHIFTER_SDP_WRITE
#define SSE_SHIFTER_SDP_WRITE_ADD_EXTRA
#define SSE_SHIFTER_SDP_WRITE_LOWER_BYTE
#endif
#if defined(SSE_HACKS)
// most hacks concern SDP, there's room for improvement
#define SSE_SHIFTER_BIG_WOBBLE // Big Wobble shift
#define SSE_SHIFTER_DANGEROUS_FANTAISY // Dangerous Fantaisy credits flicker
#define SSE_SHIFTER_MED_OVERSCAN_SHIFT // No Cooper/greetings
#define SSE_SHIFTER_PACEMAKER // Pacemaker credits flickering line
#define SSE_SHIFTER_SCHNUSDIE // Reality is a Lie/Schnusdie overscan logo
#define SSE_SHIFTER_SDP_WRITE_DE_HSCROLL
#define SSE_SHIFTER_SDP_WRITE_MIDDLE_BYTE // stable
#define SSE_SHIFTER_TCB // Swedish New Year Demo/TCB SDP (60hz)
#endif
#if defined(SSE_STF)
#define SSE_STF_VERTICAL_OVERSCAN
#endif
#endif//shifter
#if defined(SSE_SOUND)
#define SSE_SOUND_FILTER_STF // a very simple filter
#define SSE_SOUND_MICROWIRE // volume, balance, bass & treble, primitive DSP
#define SSE_SOUND_VOL // -6db for PSG chipsound (using DSP) //undef v3.7.0
#define SSE_SOUND_FILTER_STE // same very simple filter as for STF//undef 3.6.3
#endif
#if defined(SSE_STF)
#ifdef WIN32
#define SSE_STF_MATCH_TOS // select a compatible TOS for next reset
#endif
#define SSE_STF_MEGASTF // blitter in STF (could be useful?) + 4MB!
#endif//stf
#if defined(SSE_TOS)
#ifdef SSE_HACKS
#define SSE_TOS_PATCH106 // a silly bug, a silly hack
#endif
#endif//tos
#if defined(SSE_VARIOUS)
#define SSE_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen - option?
//#define SSE_VAR_HIDE_OPTIONS_AT_START // hack before debugging
#define SSE_VAR_KEYBOARD_CLICK // not a sound nor IKBD option
#define SSE_GUI_MSA_CONVERTER // don't prompt if found
#define SSE_VAR_STEALTH // don't tell we're an emulator (option)
#define SSE_VAR_REWRITE // to conform to what compilers expect (warnings...)
#endif
#if defined(SSE_VIDEO)
#define SSE_VID_AUTOOFF_DECRUNCHING //undef v3.5.0
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_LB_DX // rendering-stage trick rather than painful hacks
#endif
#endif//video

#endif//340


#if SSE_VERSION>=341

#if defined(SSE_INTERRUPT)
#if defined(SSE_INT_VBL)
//#define SSE_INT_VBI_START // ideal but many problems, eg Calimero
#if defined(SSE_STF) && defined(SSE_INT_VBL) && !defined(SSE_INT_VBI_START)
#define SSE_INT_VBL_STF // more a hack but works
#endif
#endif
#if defined(SSE_INT_MFP)
#endif//mfp
#endif//int

#endif//341



//////////
// v3.5 //
//////////


#if SSE_VERSION>=350

#if defined(SSE_FLOPPY)
#define SSE_DMA        // Custom Direct Memory Access chip (disk)
#define SSE_DRIVE      // SF314 floppy disk drive
#ifdef WIN32
#define SSE_IPF        // CAPS support (IPF, CTR disk images) 
#endif
#endif
//#define SSE_SDL        // Simple DirectMedia Layer (TODO)
/*
#define SSE_SSE_OPTION_PAGE // a new page for all our options
#define SSE_SSE_OPTION_STRUCT // structure SSEOption 
#define SSE_SSE_CONFIG_STRUCT // structure SSEConfig 
*/
#ifdef WIN32
#define SSE_DELAY_LOAD_DLL // can run without DLL//never worked with bcc???
#if _MSC_VER>1200 // higher than VC6
#define SSE_VS2003_DELAYDLL
#endif
#endif
#if defined(SSE_CPU)
#define SSE_CPU_LINE_F // for interrupt depth counter
#define SSE_CPU_ROUNDING     // round like a rolling stone
#if defined(SSE_CPU_EXCEPTION)
#define SSE_CPU_IGNORE_WRITE_B_0 // for Aladin, may write on 1st byte
#define SSE_CPU_SET_DEST_W_TO_0 // for Aladin
#endif//exc
#if defined(SSE_CPU_PREFETCH)
// Change no timing, just the macro used, so that we can identify what timings
// are for prefetch:
#define SSE_CPU_FETCH_TIMING  // TODO can't isolate
// Move the timing counting from FETCH_TIMING to PREFETCH_IRC:
#define SSE_CPU_PREFETCH_TIMING //big, big change
#if defined(SSE_CPU_PREFETCH_TIMING)
#define SSE_CPU_PREFETCH_TIMING_MOVEM_HACK // undef v3.7.0 
#define SSE_CPU_PREFETCH_TIMING_SET_PC // necessary for some SET PC cases
#endif
//#define SSE_CPU_PREFETCH_TIMING_EXCEPT // to mix unique switch + lines
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
#endif//CORRECTING_PREFETCH_TIMING
#endif//prefetch
#endif//cpu
#if defined(SSE_DMA)
//#define SSE_DMA_ADDRESS // enforcing order for write (no use?)
#define SSE_DMA_COUNT_CYCLES
//#define SSE_DMA_DELAY // works but overkill 3.7.0 -> use generic floppy event?
#define SSE_DMA_FDC_ACCESS
//#define SSE_DMA_DOUBLE_FIFO //prepared, not defined until 3.7
#define SSE_DMA_FIFO // first made for CAPS 
#define SSE_DMA_IO //necessary
#define SSE_DMA_READ_STATUS 
#define SSE_DMA_SECTOR_COUNT
#define SSE_DMA_SECTOR_COUNT2
#define SSE_DMA_WRITE_CONTROL
#endif//dma
#if defined(SSE_FDC_ACCURATE) 
// #define SS_FDC_FORMAT_BYTES // more hbl / sector 
#define SSE_FDC_INDEX_PULSE1 // 4ms
#define SSE_FDC_MOTOR_OFF 
// #define SS_FDC_SEEK_UPDATE_TR
#define SSE_FDC_SPIN_UP_STATUS
// #define SS_FDC_SPIN_UP_TIME
// #define SSE_DEBUG_FDC_TRACE_STATUS //spell out status register
#ifdef SSE_DRIVE
#define SSE_FDC_VERIFY_AGENDA 
#endif
#endif//fdc_adat
#if defined(SSE_IPF)
// those switches were used for development, normally they ain't necessary
//#define SSE_IPF_CPU // total sync, works but overkill
//#define SSE_IPF_RUN_PRE_IO 
//#define SSE_IPF_RUN_POST_IO
#define SSE_IPF_OSD // for the little box at reset - undef v3.6.1
//#define SSE_IPF_SAVE_WRITES //TODO?
#define SSE_DEBUG_IPF_TRACE_SECTORS // show sector info
#endif//ipf
#if defined(SSE_INT_MFP)
#if defined(SSE_INT_MFP_TIMER_B)
#define SSE_INT_MFP_TIMER_B_NO_WOBBLE // BIG Demo Psych-O screen 2
#endif
#define SSE_INT_MFP_TxDR_RESET // they're not reset according to doc
#endif//mfp
#if defined(SSE_MMU)
#define SSE_MMU_NO_CONFUSION  //undef v3.5.2
//#define SS_MMU_WAKE_UP_DELAY_ACCESS // CPU can't access bus when MMU has it
#ifdef SSE_STF
#define SSE_MMU_WAKE_UP
#endif//stf
#if defined(SSE_MMU_WAKE_UP)
#define SSE_MMU_WU_0_BYTE_LINE
#define SSE_MMU_WU_IOR_HACK 
#define SSE_MMU_WU_IOW_HACK 
// the following 3 switches were more experimental 
#define SSE_MMU_WU_IO_BYTES_R  // undef 3.5.3
#define SSE_MMU_WU_IO_BYTES_W // undef 3.5.3
#define SSE_MMU_WU_IO_BYTES_W_SHIFTER_ONLY // undef v3.5.3
#define SSE_MMU_WU_PALETTE_STE // render +1 cycle (pixel) in state 2
//#define SSE_MMU_WU_READ_SDP
#define SSE_MMU_WU_RIGHT_BORDER
//#define SSE_MMU_WU_STE_STATE2 // STE in same state2 as STF (no)
//#define SSE_MMU_WU_WRITE_SDP
#endif//wu
#endif//mmu
#if defined(SSE_SHIFTER)
#if defined(SSE_SHIFTER_TRICKS)
#define SSE_SHIFTER_END_OF_LINE_CORRECTION // correct +2, -2 lines 
#define SSE_SHIFTER_PALETTE_TIMING //Overscan Demos #6
#define SSE_SHIFTER_STE_MED_HSCROLL // Cool STE
#define SSE_SHIFTER_STE_HSCROLL_LEFT_OFF //MOLZ/Spiral
#define SSE_SHIFTER_STE_VERTICAL_OVERSCAN //RGBeast
#endif
#endif//shifter
#if defined(SSE_TOS)
// fixes by other people: 
#define SSE_AVTANDIL_FIX_001 // Russin TOS number
#define SSE_MEGAR_FIX_001 // intercept GEM in extended resolution
#endif
#if defined(SSE_UNIX)
#endif
#if defined(SSE_VARIOUS)
#define SSE_VAR_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
#define SSE_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES // at final save
#define SSE_GUI_INFOBOX //changed later
#define SSE_VAR_NO_ASSOCIATE //changed later
#define SSE_VAR_NO_UPDATE // remove all code in relation to updating
#define SSE_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah
#define SSE_GUI_NOTIFY1 //adding some notify during init
#ifdef WIN32
#define SSE_VAR_UNRAR // using unrar.dll, up to date
#endif
#endif
#if defined(SSE_VIDEO)
#undef SSE_VID_AUTOOFF_DECRUNCHING
#define SSE_VID_ADJUST_DRAWING_ZONE1 // attempt to avoid crash
#define SSE_VID_ADJUST_DRAWING_ZONE2
#define SSE_VID_BLIT_TRY_BLOCK //useless?
#define SSE_VID_CHECK_DDFS // is video card/display capable?
#if defined(WIN32) //TODO Unix
#define SSE_VID_SAVE_NEO // screenshots in ST Neochrome format
#define SSE_VID_RECORD_AVI //avifile undef v3.7.0
#endif
#endif//video

#endif//350


#if SSE_VERSION>=351

#if defined(SSE_ACIA)
#define SSE_ACIA_DOUBLE_BUFFER_RX // only from 6301 (not MIDI) 
#define SSE_ACIA_REGISTERS // formalising the registers
#define SSE_ACIA_NO_RESET_PIN // don't reset on warm reset
// #define SS_ACIA_OVERRUN // only 6301
// #define SS_ACIA_REMOVE_OLD_VARIABLES
#define SSE_ACIA_USE_REGISTERS
#endif
#if defined(SSE_CPU)
#if defined(SSE_CPU_EXCEPTION)
#define SSE_CPU_PRE_DEC // no "pre" decrement if exception
#define SSE_CPU_TRUE_PC // based on Motorola microcodes!
#endif
#define SSE_CPU_FETCH_IO     // fetch like a dog in outer space
#define SSE_CPU_FETCH_IO2
#if defined(SSE_CPU_INSTR)
#define SSE_CPU_TAS          // 4 cycles fewer if memory
#endif
#if defined(SSE_CPU_PREFETCH)
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
#if defined(SSE_CPU_PREFETCH_TIMING) || defined(CORRECTING_PREFETCH_TIMING)
#define SSE_CPU_PREFETCH_MOVE_MEM 
#define SSE_CPU_PREFETCH_TAS 
#endif
#endif//prefetch
#endif//cpu
#if defined(SSE_FLOPPY)
#define SSE_YM2149 // was _PSG, but object YM2149 was defined
#if defined(WIN32)
#define SSE_PASTI      // Improvements in Pasti support (STX disk images)
#endif
#if defined(SSE_DMA)
//#define SSE_DMA_FDC_READ_HIGH_BYTE // like pasti, 0  
#define SSE_DMA_FIFO_NATIVE
#define SSE_DMA_FIFO_PASTI
#define SSE_DMA_FIFO_READ_ADDRESS // save some bytes...
#define SSE_DMA_FIFO_READ_ADDRESS2 // save 4 bytes more...
#endif//dma
#if defined(SSE_DRIVE)
#define SSE_DRIVE_BYTES_PER_ROTATION
#define SSE_DRIVE_MULTIPLE_SECTORS
#define SSE_DRIVE_READ_ADDRESS_TIMING
#define SSE_DRIVE_RW_SECTOR_TIMING // start of sector
#define SSE_DRIVE_RW_SECTOR_TIMING2 // end of sector (hack) //undef 3.7.0
#endif//drv
#if defined(SSE_FDC_ACCURATE) 
#define SSE_FDC_HEAD_SETTLE
#define SSE_FDC_PRECISE_HBL
#define SSE_FDC_READ_ADDRESS_UPDATE_SR
#define SSE_FDC_SEEK
#define SSE_FDC_SPIN_UP_AGENDA
#define SSE_FDC_STEP
#define SSE_FDC_IGNORE_WHEN_NO_DRIVE_SELECTED // from Hatari
#define SSE_FDC_MOTOR_OFF_COUNT_IP
#endif//fdc
#if defined(SSE_PASTI)
#define SSE_PASTI_ALWAYS_DISPLAY_STX_DISKS
#define SSE_PASTI_AUTO_SWITCH
#define SSE_PASTI_ONLY_STX  // experimental! optional
#define SSE_PASTI_NO_RESET 
#define SSE_PASTI_ON_WARNING // mention in disk manager title - undef 370
#endif//stx
#endif//floppy
#if defined(SSE_IKBD_6301)
#define SSE_IKBD_6301_MOUSE_ADJUST_SPEED // undef 3.7.0
//#define SSE_IKBD_6301_RUN_CYCLES_AT_IO // overkill //MFD
#endif
#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_INLINE 
#if defined(SSE_INT_VBL) && defined(SSE_INT_JITTER)
#define SSE_INT_JITTER_VBL_STE//3.5.1,undef 3.6.0
#endif
#endif
#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_RATIO_PRECISION // for short timers
#define SSE_INT_MFP_RATIO_STE_AS_STF // change STF + STE (same for both)
#define SSE_INT_MFP_RS232 //one little anti-hang bugfix
#endif
#if defined(SSE_MMU)
#if defined(SSE_CPU)
//#define SSE_MMU_WAIT_STATES // extreme, replaces rounding to 4, TODO
#endif
#endif
#if defined(SSE_OSD)
#if defined(SSE_FLOPPY)
#define SSE_OSD_DRIVE_INFO // cool!
#endif
#if defined(SSE_OSD_DRIVE_LED) && defined(SSE_FLOPPY)
#define SSE_OSD_DRIVE_LED2 // simpler approach
#define SSE_OSD_DRIVE_LED3 // remove useless variable
#endif
#define SSE_OSD_LOGO1 // suppress former logo (=3.2 in grx) //undef v3.5.3
#define SSE_OSD_LOGO2 //hack (temp) //undef v3.5.3
#define SSE_OSD_SCROLLER_CONTROL
#endif
#if defined(SSE_SDL)
#define SSE_SDL_DEACTIVATE
#endif
#if defined(SSE_SHIFTER)
#if defined(SSE_SHIFTER_TRICKS)
#if defined(SSE_HACKS)
#define SSE_SHIFTER_ARMADA_IS_DEAD // no shift contrary to Big Wobble
#define SSE_SHIFTER_TEKILA // in Delirious 4
#endif//hacks
#endif//tricks
#endif//sft
#if defined(SSE_SOUND)
#define SSE_SOUND_INLINE // macro->inline, easier for my tests, but hard to do
#define SSE_SOUND_CHANGE_TIME_METHOD_DELAY //detail
#define SSE_SOUND_NO_EXTRA_PER_VBL //compensating hack? changes what?
#define SSE_SOUND_OPTIMISE //big word for what it is
#define SSE_SOUND_RECOMMEND_OPTIONS
#ifdef WIN32
//#define SSE_SOUND_SKIP_DSOUND_TEST
#endif
#ifdef UNIX
#define SS_RTAUDIO_LARGER_BUFFER //simplistic attempt
#endif
#endif
#if defined(SSE_STF)
#define SSE_STE_2MB // auto make RAM 2MB, more compatible
#define SSE_STF_1MB // auto make RAM 1MB, more compatible
#endif
#if defined(SSE_STRUCTURE)
#if defined(SSE_DMA)
#define SSE_STRUCTURE_DMA_INC_ADDRESS
#endif
#if defined(LEGACY_BUILD)
//#define SSE_STRUCTURE_INFO // just telling files included in modules
#endif
//#define SSE_STRUCTURE_IOR
#endif
#define SSE_TIMINGS
#if defined(SSE_TIMINGS)
#define SSE_TIMINGS_MS_TO_HBL
#endif
#if defined(SSE_TOS)
#endif
#if defined(SSE_UNIX)
#endif
#if defined(SSE_VARIOUS)
#define SSE_VAR_RESIZE // reduce memory set (int->BYTE etc.)
#ifdef WIN32
#define SSE_VAR_CHECK_SNAPSHOT
#define SSE_GUI_OPTIONS_REFRESH // 6301, STF... up-to-date with snapshot
#endif
#endif//var
#if defined(SSE_VIDEO)
#if defined(WIN32)
#define SSE_VID_SCANLINES_INTERPOLATED // using stretch mode!
#define SSE_VID_SCANLINES_INTERPOLATED_MED
#endif
#endif

#endif//351


#if SSE_VERSION>=352

#if defined(SSE_ACIA)
#undef SSE_ACIA_IRQ_DELAY //code missing now
// #define SSE_ACIA_DONT_CLEAR_DR //?
// #define SS_ACIA_IRQ_ASSERT_READ_SR //TODO
// #define SS_ACIA_BUS_JAM_PRECISE_WOBBLE //TODO
// #define SS_ACIA_OVERRUN_REPLACE_BYTE // normally no
// #define SS_ACIA_TDR_COPY_DELAY // apparently not necessary
#endif
#if defined(SSE_BLITTER)
#define SSE_BLT_YCOUNT // 0=65536
#endif

#ifdef SSE_ROM
#define SSE_CARTRIDGE  // ROM Cartridge slot
#endif
#if defined(SSE_CARTRIDGE)
#define SSE_CARTRIDGE_64KB_OK
#define SSE_CARTRIDGE_DIAGNOSTIC
#define SSE_CARTRIDGE_NO_CRASH_ON_WRONG_FILE
#define SSE_CARTRIDGE_NO_EXTRA_BYTES_OK
#endif
#if defined(SSE_FLOPPY)
#if defined(SSE_DRIVE)
#define SSE_DRIVE_MOTOR_ON
#define SSE_DRIVE_SPIN_UP_TIME
#define SSE_DRIVE_SPIN_UP_TIME2 // more precise
#endif
#endif//flp
#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_IRQ_DELAY // undef v3.6.0
#define SSE_INT_MFP_IRQ_DELAY2 // undef v3.5.3
#endif
#if defined(SSE_MMU)
#undef SSE_MMU_NO_CONFUSION //Diagnostic cartridge: don't define
#endif
#if defined(SSE_OSD)
#if defined(SSE_OSD_DRIVE_INFO)
// #define SSE_OSD_DRIVE_INFO2 // no SR when fast
#endif
#endif
#if defined(SSE_SHIFTER)
#if defined(SSE_SHIFTER_TRICKS)
#define SSE_SHIFTER_4BIT_SCROLL_LARGE_BORDER_HACK
#define SSE_SHIFTER_LEFT_OFF_TEST_BEFORE_HBL // for Riverside
#define SSE_SHIFTER_LINE_PLUS_20 // 224 byte scanline STE only
#define SSE_SHIFTER_LINE_PLUS_20_SHIFT // for Riverside
#if defined(SSE_HACKS)
#define SSE_SHIFTER_DRAGON1 // undef v3.5.4
#endif
#endif//trck
#if defined(SSE_MMU_WAKE_UP)
#define SSE_SHIFTER_UNSTABLE // DoLB, Omega, Overdrive/Dragon, Beeshift
#endif
#if defined(SSE_SHIFTER_UNSTABLE)
#define SSE_SHIFTER_UNSTABLE_DOLB
#define SSE_SHIFTER_UNSTABLE_OMEGA
#define SSE_SHIFTER_HI_RES_SCROLLING // Beeshift2
#define SSE_SHIFTER_MED_RES_SCROLLING // Beeshift
#if defined(SSE_MMU_WAKE_UP)
#define SSE_SHIFTER_PANIC // funny effect, interleaved border bands
#endif
#endif//unstable
#define SSE_SHIFTER_REMOVE_USELESS_VAR //3.6.1
#define SSE_SHIFTER_VERTICAL_OPTIM1 //avoid useless tests
#endif//sft
#if defined(SSE_SOUND)
#define SSE_SOUND_OPTION_DISABLE_DSP //undef v3.5.3
#endif
#if defined(SSE_VIDEO)
#define SSE_VID_SCREENSHOTS_NUMBER
#endif

#endif//352


#if SSE_VERSION>=353

#if defined(SSE_ACIA)
#define SSE_ACIA_TDR_COPY_DELAY // effect on SR
#endif
#if defined(SSE_FLOPPY)
#if defined(SSE_DRIVE)
#define SSE_DRIVE_EMPTY_VERIFY_LONG // undef 3.5.4
#define SSE_DRIVE_11_SECTORS
#define SSE_DRIVE_READ_TRACK_11
#define SSE_DRIVE_READ_TRACK_11B //Gap 4: 1
#define SSE_DRIVE_READ_TRACK_11C //Gap 5
#endif
#if defined(SSE_FDC_ACCURATE) 
#define SSE_FDC_FORCE_INTERRUPT // Panzer rotation using $D4
#define SSE_FDC_FORCE_INTERRUPT_D4
#define SSE_FDC_INDEX_PULSE2 // read STR
//#define SSE_WD1772_RESET
#define SSE_FDC_INDEX_PULSE_COUNTER
#endif//fdc
#define SSE_WD1772_RESET
#endif//flp
#if defined(SSE_INT_HBL)
#define SSE_INT_HBL_IACK_FIX // from Hatari - BBC52 (works without?)
#endif
#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_IACK
#endif
#if defined(SSE_INT_MFP)
#undef SSE_INT_MFP_IRQ_DELAY2
#define SSE_INT_MFP_PATCH_TIMER_D // from Hatari, we keep it for performance
#endif
#if defined(SSE_MMU)
#undef SSE_MMU_WU_IO_BYTES_R  // breaks too much (read SDP) TODO
#undef SSE_MMU_WU_IO_BYTES_W // no too radical
#undef SSE_MMU_WU_IO_BYTES_W_SHIFTER_ONLY // adapt cycles for Shifter write
#define SSE_MMU_WU_SHIFTER_TRICKS // Adapt limit values based on Paolo's table
#define SSE_MMU_WU_VERTICAL_OVERSCAN // ijor's wakeup.tos test
#endif
#if defined(SSE_OSD)
#undef SSE_OSD_LOGO1
#undef SSE_OSD_LOGO2
#define SSE_OSD_LOGO3 // nicer
#endif
#if defined(SSE_SHIFTER)
///#define SSE_SHIFTER_RENDER_SYNC_CHANGES//don't until debug
#if defined(SSE_SHIFTER_TRICKS)
#define SSE_SHIFTER_FIX_LINE508_CONFUSION // hack at recording Shifter event
#define SSE_SHIFTER_LINE_PLUS_2_STE_DSOS // limit 42 instead of 38? //undef 3.5.4
#define SSE_SHIFTER_LINE_PLUS_2_TEST // loSTE screens
#define SSE_SHIFTER_LINE_MINUS_106_BLACK // loSTE screens
#define SSE_SHIFTER_LEFT_OFF_60HZ //24 bytes!
#define SSE_SHIFTER_RIGHT_OFF_BY_SHIFT_MODE //beeshift0
#if defined(SSE_SHIFTER_0BYTE_LINE) // former switch approach
#define SSE_SHIFTER_0BYTE_LINE_SYNC2 // loSTE screens
#endif//0byte
#endif//tricks
#endif//sft
#if defined(SSE_SOUND)
#undef SSE_SOUND_OPTION_DISABLE_DSP
#endif
#if defined(SSE_TOS)
#ifdef SSE_HACKS
#define SSE_TOS_STE_FAST_BOOT //from hatari
#endif
#endif
#if defined(SSE_VARIOUS)
#define SSE_GUI_ASSOCIATE
#define SSE_GUI_RESET_BUTTON // invert
#endif
#ifdef SSE_GUI_ASSOCIATE
#define SSE_GUI_ASSOCIATE_IPF // extension may be associated with Steem //RENAME
#define SSE_GUI_ASSOCIATE_CU // current user, not root
#define SSE_GUI_MAY_REMOVE_ASSOCIATION
#define SSE_GUI_NO_ASSOCIATE_STC // cartridge, who uses that?
#define SSE_GUI_NO_AUTO_ASSOCIATE_DISK_STS_STC // disk images + STS STC
#define SSE_GUI_NO_AUTO_ASSOCIATE_MISC // other .PRG, .TOS...
#endif
#define SSE_GUI_OPTION_SLOW_DISK_SSE // because many people miss it in disk manager
#if defined(SSE_VIDEO)
#define SSE_VID_CHECK_DDFS2 //list all modes
#if defined(SSE_VID_SCANLINES_INTERPOLATED)
#define SSE_VID_SCANLINES_INTERPOLATED_SSE // put option in SSE page
#endif
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_BIGTOP // more lines for palette effects
#endif
#endif//vid

#endif//353


#if SSE_VERSION>=354

#if defined(SSE_FLOPPY)
#if defined(SSE_DRIVE)
#undef SSE_DRIVE_EMPTY_VERIFY_LONG //def 3.5.3
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT //GEM
#endif
#endif//flp
#if defined(SSE_INTERRUPT)
#if defined(SSE_INT_JITTER) && defined(SSE_INT_HBL)
#undef SSE_INT_JITTER_HBL
#endif
#endif//int
#if defined(SSE_MMU)
#undef SSE_MMU_WU_0_BYTE_LINE
#undef SSE_MMU_WU_IOR_HACK 
#undef SSE_MMU_WU_IOW_HACK 
#if defined(SSE_STF)
#define SSE_MMU_WU_DL // Dio's DE-LOAD delay
#define SSE_MMU_WU_RESET_ON_SWITCH_ST
#endif
#endif//mmu
#if defined(SSE_OSD)
//#define SSE_OSD_DRIVE_INFO_OSD_PAGE
#define SSE_OSD_DRIVE_INFO_SSE_PAGE
#define SSE_OSD_SCROLLER_DISK_IMAGE //TODO sometimes wrong name
#endif
#if defined(SSE_SHIFTER)
#if defined(SSE_SHIFTER_TRICKS)
#undef SSE_SHIFTER_LINE_PLUS_2_STE_DSOS
#define SSE_SHIFTER_LINE_PLUS_2_STE // hack?
#define SSE_SHIFTER_STATE_MACHINE //simpler approach and WS-aware
#define SSE_SHIFTER_STE_HI_HSCROLL
#if defined(SSE_SHIFTER_STATE_MACHINE)
#undef SSE_SHIFTER_0BYTE_LINE_SYNC2 // loSTE screens
#endif
#if defined(SSE_HACKS)
#undef SSE_SHIFTER_DRAGON1
#define SSE_SHIFTER_DOLB_SHIFT1 // based on "unstable overscan"
//#define SSE_SHIFTER_DOLB_SHIFT2 // based on cycle of R0
#endif//hacks
#endif//tricks
#endif//sft
#if defined(SSE_VARIOUS)
#define SSE_GUI_OPTIONS_SSE_ICON_VERSION
#ifdef WIN32
#define SSE_GUI_STATUS_STRING // "status bar" in the icon bar
#endif
#if defined(SSE_GUI_STATUS_STRING)
#define SSE_GUI_STATUS_STRING_6301
#define SSE_GUI_STATUS_STRING_ADAT
#define SSE_GUI_STATUS_STRING_DISK_NAME
#define SSE_GUI_STATUS_STRING_DISK_NAME_OPTION
#define SSE_GUI_STATUS_STRING_HACKS
#define SSE_GUI_STATUS_STRING_IPF // undef v3.7.0
#define SSE_GUI_STATUS_STRING_PASTI // undef v3.7.0
#endif
#endif
#if defined(SSE_VIDEO)
#if defined(WIN32)
#define SSE_VID_3BUFFER // Triple Buffer to remove tearing
#define SSE_VID_VSYNC_WINDOW // no tearing and smooth scrolling also in window
#endif
#if defined(SSE_VID_3BUFFER)
#define SSE_VID_3BUFFER_FS // fullscreen: stretch mode only
//#define SSE_VID_3BUFFER_NO_VSYNC // for tests: "VSync" is necessary
#define SSE_VID_3BUFFER_WIN //windowed mode (necessary for FS)
#endif
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_412 // 
#define SSE_VID_BORDERS_413 // best fit for overscan?
#define SSE_VID_BORDERS_416 
#define SSE_VID_BORDERS_416_NO_SHIFT
#endif
#endif//vid

#endif//354


//////////
// v3.6 //
//////////


#if SSE_VERSION>=360

#if defined(SSE_ACIA)
#define SSE_ACIA_MIDI_TIMING1 //check
#define SSE_ACIA_TDR_COPY_DELAY2 // effect on byte flow
#endif
#if defined(SSE_CPU)
#if defined(SSE_CPU_EXCEPTION)
#define SSE_CPU_IGNORE_RW_4MB // for F-29, works but... 
#endif
#define SSE_CPU_INLINE       // supposes TM68000 exists!
#if defined(SSE_CPU_INLINE)
#define SSE_CPU_INLINE_PREFETCH_SET_PC
#define SSE_CPU_INLINE_READ_BWL
#define SSE_CPU_INLINE_READ_FROM_ADDR
#endif
#if defined(SSE_CPU_ROUNDING)
#define SSE_CPU_ROUNDING_MOVEM_MR_L //Hackabonds instructions scroller
#endif
#define SSE_CPU_UNSTOP
#endif//cpu
#if defined(SSE_FLOPPY)
#if defined(SSE_DMA)
#define SSE_DMA_ADDRESS_EVEN
#endif
#if defined(SSE_DRIVE)
#define SSE_DRIVE_CREATE_ST_DISK_FIX // from Petari
#define SSE_DRIVE_SINGLE_SIDE
#if defined(SSE_DRIVE_SINGLE_SIDE)
#define SSE_DRIVE_SINGLE_SIDE_NAT1
#define SSE_DRIVE_SINGLE_SIDE_PASTI
#endif
#if defined(WIN32) //TODO Unix
#define SSE_DRIVE_SOUND // heavily requested, delivered!//3.6.0
#if defined(SSE_DRIVE_SOUND)
#define SSE_DRIVE_SOUND_SINGLE_SET // drive B uses sounds of A
//#define SSE_DRIVE_SOUND_EDIT // 1st beta soundset
#define SSE_DRIVE_SOUND_EPSON // current samples=Epson
#define SSE_DRIVE_SOUND_VOLUME // logarithmic 
#endif//drive sound
#endif//win32
#define SSE_DRIVE_SWITCH_OFF_MOTOR //hack
#endif//drv
#if defined(SSE_PASTI)
#define SSE_PASTI_ONLY_STX_HD
#endif
#endif//flp
#if defined(SSE_IKBD_6301)
#define SSE_IKBD_6301_MOUSE_MASK // Jumping Jackson auto
#endif
#if defined(SSE_INTERRUPT)
#if defined(SSE_INT_JITTER) && defined(SSE_INT_HBL)
#define SSE_INT_JITTER_HBL //defined again for 3615HMD
#endif
#if defined(SSE_INT_JITTER) && defined(SSE_INT_VBL)
#undef SSE_INT_JITTER_VBL_STE//3.5.1,undef 3.6.0
#endif
#endif
#if defined(SSE_INT_MFP)
#undef SSE_INT_MFP_IRQ_DELAY
#define SSE_INT_MFP_IRQ_DELAY3 // undef v3.6.1
#define SSE_INT_MFP_TIMERS_BASETIME //
#endif
#if defined(SSE_MMU)
#if defined(WIN32) //TODO Unix
#define SSE_MMU_256K // Atari 260 ST
#define SSE_MMU_2560K // some STE with 2MB memory upgrade
#endif
#endif
#if defined(SSE_SHIFTER)
#if defined(SSE_SHIFTER_TRICKS)
#define SSE_SHIFTER_MED_RES_SCROLLING_360//switch was missing in original source revision
#define SSE_SHIFTER_PALETTE_NOISE //UMD8730 STF
#if defined(SSE_HACKS)
#define SSE_SHIFTER_XMAS2004 // XMas 2004 by Paradox shift
#endif//hck
#endif//tricks
#endif//sft
#if defined(SSE_SOUND)
#define SSE_SOUND_MICROWIRE_WRITE_LATENCY // as documented
#define SSE_SOUND_VOL_LOGARITHMIC // more intuitive setting
#define SSE_YM2149
#endif
#if defined(SSE_TOS)
#define SSE_TOS_FILETIME_FIX //from Petari
#define SSE_TOS_NO_INTERCEPT_ON_RTE1 // undef v3.6.1
#endif
#if defined(SSE_UNIX)
#define SSE_UNIX_OPTIONS_SSE_ICON
#define SSE_UNIX_STATIC_VAR_INIT //odd
#endif
#if defined(SSE_VARIOUS)
#define SSE_GUI_WINDOW_TITLE
#if defined(SSE_GUI_STATUS_STRING)
#define SSE_GUI_STATUS_STRING_VSYNC // V as VSync
#endif
#endif//var
#if defined(SSE_VIDEO)
#if defined(WIN32)
#if defined(SSE_VID_VSYNC_WINDOW)
#define SSE_VID_VSYNC_WINDOW_CRASH_FIX1
//#define SSE_VID_VSYNC_WINDOW_CRASH_FIX2 //safer but annoying
#define SSE_VID_VSYNC_WINDOW_CRASH_FIX3 //TODO find something better?
#endif
#endif
#endif//vid
#ifdef SSE_YM2149 
#define SSE_YM2149_FIX_TABLES // option P.S.G.
#define SSE_YM2149_FIXED_VOL_TABLE // was SSE_YM2149_FIXED_VOL_FIX2 in v3.6.4
#define SSE_YM2149_OPT1
#if defined(SSE_YM2149_FIX_TABLES)
#define SSE_YM2149_FIXED_VOL_FIX1 //undef v3.7.0
#define SSE_YM2149_ENV_FIX1 //undef v3.7.0
#endif
#endif//ym2149

#endif//360


#if SSE_VERSION>=361

#if defined(SSE_ACIA)
#define SSE_ACIA_IRQ_DELAY2// back to this approach (hack)
#endif

#if defined(SSE_CPU)
#define SSE_CPU_ALT_REG_NAMES  // convenience
#if defined(SSE_CPU_DIV)
#define SSE_CPU_DIVS_OVERFLOW
#define SSE_CPU_DIVU_OVERFLOW
#endif
#endif//cpu
#if defined(SSE_FLOPPY)
#if defined(SSE_DMA)
#define SSE_DMA_TRACK_TRANSFER // debug + possible later use
#endif
#ifdef SSE_DRIVE
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT2 //motor led still on
#define SSE_DRIVE_IPF1 // know image type (not used yet)
#define SSE_DRIVE_MOTOR_ON_IPF//TODO
#if defined(SSE_DRIVE_SOUND)
#define SSE_DRIVE_SOUND_EMPTY // none
#define SSE_DRIVE_SOUND_IPF // fix 
#endif//snd
#define SSE_DRIVE_WRONG_IMAGE_ALERT
#endif//drv
#if defined(SSE_IPF)
#undef SSE_IPF_OSD
#define SSE_IPF_CTRAW//2nd type of file recognised by caps v5.0 (?) //3.6.1
#define SSE_IPF_CTRAW_REV //3.6.1 manage different rev data
#define SSE_IPF_RESUME//TODO
//#define SSE_IPF_KFSTREAM//3rd type of file recognised by caps v5.1 (?) //3.6.1
//#define SSE_IPF_DRAFT//4th type of file recognised by caps v5.1 (?) //3.6.1
#endif//ipf
#if defined(SSE_PASTI)
#define SSE_PASTI_ON_WARNING2 //refactoring
#endif
#endif//floppy
#if defined(SSE_INTERRUPT)
#if defined(SSE_INT_HBL)
#define SSE_INT_HBL_ONE_FUNCTION // remove event_hbl()
#endif
#if defined(SSE_INT_JITTER) && defined(SSE_INT_VBL) && defined(SSE_STF)
#define SSE_INT_JITTER_RESET
#endif
#endif//int
#if defined(SSE_INT_MFP)
#undef SSE_INT_MFP_IRQ_DELAY3
#define SSE_INT_MFP_RATIO_HIGH_SPEED
#define SSE_INT_MFP_WRITE_DELAY1//Audio Artistic
#endif
#if defined(SSE_STF)
#define SSE_STF_8MHZ // for Panic study!
#endif
#if defined(SSE_TOS)
#undef SSE_TOS_NO_INTERCEPT_ON_RTE1
#define SSE_TOS_GEMDOS_NOINLINE
#define SSE_TOS_GEMDOS_PEXEC6 //ReDMCSB 100% in TOS104
#define SSE_TOS_STRUCT
#define SSE_TOS_GEMDOS_VAR1 //various unimportant fixes 
#if defined(SSE_TOS_STRUCT)
#define SSE_TOS_NO_INTERCEPT_ON_RTE2 // ReDMCSB 50% in TOS102
#ifdef WIN32
#define SSE_TOS_SNAPSHOT_AUTOSELECT
#define SSE_TOS_SNAPSHOT_AUTOSELECT2//with refactoring
#define SSE_TOS_SNAPSHOT_AUTOSELECT3//options.cpp uses refactoring
#endif//win32
#endif
#ifdef SSE_STF
#define SSE_TOS_WARNING1 // version/ST type
#endif
#endif//tos
#if defined(SSE_VARIOUS)
#define SSE_GUI_INFOBOX0 // enum
#define SSE_GUI_INFOBOX1 // SSE internet sites
#define SSE_GUI_INFOBOX2 // SSE readme + FAQ
#define SSE_GUI_INFOBOX3 // readme text font
#define SSE_GUI_INFOBOX4 // readme 80 col 
#define SSE_GUI_INFOBOX5 // don't take 64K on the stack!
#define SSE_GUI_INFOBOX6 // no cartridge howto
#define SSE_GUI_INFOBOX7 // specific hints
#define SSE_VAR_POWERON1 // undef 3.6.2 ;)
#if defined(SSE_GUI_STATUS_STRING) && defined(SSE_STF)
#define SSE_GUI_STATUS_STRING_FULL_ST_MODEL
#endif
#endif//var
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_416_NO_SHIFT0
#define SSE_VID_BORDERS_416_NO_SHIFT1 // check border on/off
#define SSE_VID_BORDERS_LB_DX1 // check border on/off
#endif

#endif//361


#if SSE_VERSION>=362

#if defined(SSE_FLOPPY)
#if defined(SSE_DMA)
#define SSE_DMA_FDC_READ_HIGH_BYTE
#endif
#endif
#if defined(SSE_VARIOUS)
#undef SSE_VAR_POWERON1
#define SSE_VAR_POWERON2 // undef 3.6.3 :)
#endif
#endif//362


#if SSE_VERSION>=363

#if defined(SSE_FLOPPY)
#define SSE_WD1772     // WD1772 floppy disk controller (IO, STW...)
#if defined(SSE_DMA)
#undef SSE_DMA_FDC_READ_HIGH_BYTE //def 3.6.2
#endif
#endif
#if defined(SSE_MMU)
#define SSE_MMU_WU_VERTICAL_OVERSCAN1 //defaults to WU1
#endif
#if defined(SSE_SHIFTER)
#if defined(SSE_SHIFTER_TRICKS)
#undef SSE_SHIFTER_PALETTE_BYTE_CHANGE
#if defined(SSE_SHIFTER_0BYTE_LINE) // former switch approach
//#define SSE_SHIFTER_0BYTE_LINE_RES_END_THRESHOLD
#endif//0-byte line
#endif//tricks
#endif//sft
#if defined(SSE_SOUND)
#undef SSE_SOUND_FILTER_STE
#define SSE_SOUND_MICROWIRE_MIXMODE
#endif
#if defined(SSE_VARIOUS)
#undef SSE_VAR_POWERON2
#endif
#if defined(SSE_VIDEO)
#define SSE_VID_FREEIMAGE1 //init library
#define SSE_VID_FREEIMAGE2 //always convert pixels
#define SSE_VID_FREEIMAGE3 //flip pic
#define SSE_VID_FREEIMAGE3 //remove WBMP
#endif//vid

#endif//363


#if SSE_VERSION>=364

#if defined(SSE_CPU)
#define SSE_CPU_E_CLOCK // other place?
#if defined(SSE_CPU_E_CLOCK)
#if defined(SSE_DEBUG) || defined(SSE_HACKS)
#define SSE_CPU_E_CLOCK_DISPATCHER // "who" wants to sync?
#endif
#if defined(SSE_ACIA)
#undef SSE_ACIA_BUS_JAM_NO_WOBBLE // def 3.4.0
#define SSE_ACIA_BUS_JAM_PRECISE_WOBBLE // option 6301 on
#endif
#endif
#if defined(SSE_CPU_ROUNDING)
#define SSE_CPU_ROUNDING_ADD_L_DN
#define SSE_CPU_ROUNDING_ADDA_L_DN
//#define SSE_CPU_ROUNDING_CMPI_BW
#define SSE_CPU_ROUNDING_CMPI_L
#endif//rounding
#endif//cpu
#if defined(SSE_FLOPPY)
#if defined(SSE_DMA)
#undef SSE_DMA_COUNT_CYCLES
#endif
#define SSE_DISK       // Disk images
#if defined(SSE_DISK)
#define SSE_DISK_IMAGETYPE
#endif
#if defined(SSE_DRIVE)
#undef SSE_DRIVE_COMPUTE_BOOT_CHECKSUM //TODO boiler control
#endif
#if defined(SSE_IPF)
#define SSE_IPF_CTRAW_1ST_LOCK
#define SSE_IPF_CTRAW_NO_UNLOCK
#endif
#if !defined(SSE_YM2149) // normally defined for sound
#define SSE_YM2149
#endif
#if defined(SSE_YM2149)
#define SSE_YM2149A // selected drive, side as variables
#define SSE_YM2149B // adapt drive motor status to FDC STR at each change
#endif
#endif//floppy
#if defined(SSE_IKBD)
#if defined(SSE_IKBD_6301)
#define SSE_IKBD_6301_VBL
#endif
#endif
#if defined(SSE_INTERRUPT)
#ifdef  SSE_CPU_E_CLOCK
#define SSE_INT_E_CLOCK //was lost (beta bug)
#endif
#if defined(SSE_INT_HBL)
#if defined(SSE_HACKS)
#define SSE_INT_HBL_E_CLOCK_HACK //3615GEN4 HMD #1
#endif
#define SSE_INT_HBL_INLINE
///#undef SSE_INT_JITTER_HBL//SSE_INT_JITTER_HBL defined in 3.6.4
#endif//hbl
#endif//int
#if defined(SSE_INT_MFP)
#define SSE_INT_MFP//temp, was missing
#endif
#if defined(SSE_TIMINGS)
#define SSE_TIMINGS_FRAME_ADJUSTMENT // due to Shifter tricks
#endif
#if defined(SSE_VARIOUS)
#define SSE_VAR_WRONG_IMAGE_ALERT1
#endif
#if defined(SSE_VIDEO)
#define SSE_VID_MEMORY_LOST // no message box
#if defined(SSE_DELAY_LOAD_DLL) && !defined(MINGW_BUILD)
#define SSE_VID_FREEIMAGE4 // use official header
#endif
#endif//vid

#endif//364


//////////
// v3.7 //
//////////

 
#if SSE_VERSION>=370
#if defined(SSE_BLITTER) && defined(SSE_HACKS)
#define SSE_BLITTER_RELAPSE//hack
#endif
#if defined(SSE_CPU)
#define SSE_CPU_DATABUS
#define SSE_CPU_DEST_IS_REGISTER //just new macro
#define SSE_CPU_E_CLOCK2
#define SSE_CPU_E_CLOCK3
#define SSE_CPU_FETCH_80000A//setpc
#define SSE_CPU_FETCH_80000B//fetchword
#define SSE_CPU_FETCH_80000C//prefetch IRC
#if defined(SSE_GUI_STATUS_STRING)
#define SSE_CPU_HALT // no reset, just stop
#endif
#define SSE_CPU_TRACE_REFACTOR//debug-only?
//#define SSE_CPU_TRACE_TIMING//tests
#define SSE_CPU_UNSTOP2//not twice
#if defined(SSE_CPU_EXCEPTION)
#undef SSE_CPU_IGNORE_RW_4MB // F-29 hangs on 4MB machines, nothing to fix
#define SSE_CPU_ASSERT_ILLEGAL2 // move.b EA=an
#define SSE_CPU_ASSERT_ILLEGAL3 // cmp.b EA=an
#define SSE_CPU_ASSERT_ILLEGAL4 // movem .w R to M
#define SSE_CPU_ASSERT_ILLEGAL4B // movem .w M to R
#define SSE_CPU_ASSERT_ILLEGAL5 // movem .l R to M
#define SSE_CPU_ASSERT_ILLEGAL5B // movem .l M to R
#define SSE_CPU_ASSERT_ILLEGAL6 // shl, shr
#define SSE_CPU_BUS_ERROR_ADDRESS //high byte
#define SSE_CPU_BUS_ERROR_ADDRESS2 //high byte
#define SSE_CPU_EXCEPTION_FCO
#define SSE_CPU_TRUE_PC2 // JMP, JSR
#endif//exc
#if defined(SSE_CPU_INSTR)
#define SSE_CPU_ABCD
#define SSE_CPU_DIV_CC //carry clear: always
#define SSE_CPU_MOVEM_BUS_ACCESS_TIMING
#define SSE_CPU_NBCD //TODO?
#define SSE_CPU_SBCD //to test
#endif//instr
#if defined(SSE_CPU_FETCH_IO)
#define SSE_CPU_FETCH_IO3//clarify
#endif
#if defined(SSE_CPU_PREFETCH_TIMING)
#undef SSE_CPU_PREFETCH_TIMING_MOVEM_HACK
#define SSE_CPU_PREFETCH_TIMING_ADD
#define SSE_CPU_PREFETCH_TIMING_ADDA
#define SSE_CPU_PREFETCH_TIMING_ADDX
#define SSE_CPU_PREFETCH_TIMING_AND
#define SSE_CPU_PREFETCH_TIMING_CHK
#define SSE_CPU_PREFETCH_TIMING_CMP
#define SSE_CPU_PREFETCH_TIMING_EXT
#define SSE_CPU_PREFETCH_TIMING_JMP
#define SSE_CPU_PREFETCH_TIMING_JSR
#define SSE_CPU_PREFETCH_TIMING_LEA
#define SSE_CPU_PREFETCH_TIMING_OR
#define SSE_CPU_PREFETCH_TIMING_STOP
#define SSE_CPU_PREFETCH_TIMING_SUBX
#define SSE_CPU_PREFETCH_TIMING_SWAP
#endif
#if defined(SSE_CPU_ROUNDING)
#define SSE_CPU_ROUNDING2 // no more "NO_ROUND" //when debugged!
#define SSE_CPU_ROUNDING_ADD_BW_DN // to DN
#define SSE_CPU_ROUNDING_ADD_BW_DN2 // from DN
#define SSE_CPU_ROUNDING_ADD_L_DN2
#define SSE_CPU_ROUNDING_ADDA_W_DN
#define SSE_CPU_ROUNDING_ADDA_L_DN2
#define SSE_CPU_ROUNDING_CMPI_L2
#define SSE_CPU_ROUNDING_MOVE
#define SSE_CPU_ROUNDING_MOVEM
#define SSE_CPU_ROUNDING_MOVEM_MR_L // renamed
#define SSE_CPU_ROUNDING_MOVEM6
#define SSE_CPU_ROUNDING_MOVEP_MR_L
#define SSE_CPU_ROUNDING_MOVEP_MR_W
#define SSE_CPU_ROUNDING_MOVEP_RM_L
#define SSE_CPU_ROUNDING_MOVEP_RM_W
#define SSE_CPU_ROUNDING_SUB_BW_DN
#define SSE_CPU_ROUNDING_SUB_BW_DN2
#define SSE_CPU_ROUNDING_SUB_L_DN
#define SSE_CPU_ROUNDING_SUB_L_DN2
#define SSE_CPU_ROUNDING_SUBA_W_DN
#define SSE_CPU_ROUNDING_SUBA_L_DN
#define SSE_CPU_ROUNDING_ABCD
#undef SSE_CPU_ROUNDING_ADD_BW_DN //simplify
#undef SSE_CPU_ROUNDING_ADD_BW_DN2 //simplify
#define SSE_CPU_ROUNDING_ADD
#undef SSE_CPU_ROUNDING_ADDA_L_DN2 //correct
#define SSE_CPU_ROUNDING_ADDA
#define SSE_CPU_ROUNDING_ADDQ
#define SSE_CPU_ROUNDING_ADDX
#define SSE_CPU_ROUNDING_ADDI
#define SSE_CPU_ROUDING_AND
#define SSE_CPU_ROUNDING_ANDI
#define SSE_CPU_ROUNDING_BCC
#define SSE_CPU_ROUNDING_BCHG
#define SSE_CPU_ROUNDING_BCLR
#define SSE_CPU_ROUNDING_BSET
#define SSE_CPU_ROUNDING_BTST
#define SSE_CPU_ROUNDING_CLR
#define SSE_CPU_ROUDING_CMP
#define SSE_CPU_ROUNDING_CMPI
#define SSE_CPU_ROUNDING_DBCC
#define SSE_CPU_ROUDING_EA // no more m68k_get_effective_address(), more code
#define SSE_CPU_ROUDING_EOR
#define SSE_CPU_ROUNDING_EORI//
#define SSE_CPU_ROUNDING_JSR
#define SSE_CPU_ROUNDING_MOVE_FROM_SR
#define SSE_CPU_ROUNDING_MOVE_TO_SR
#define SSE_CPU_ROUNDING_NBCD
#define SSE_CPU_ROUNDING_NEG
#define SSE_CPU_ROUNDING_NEGX
#define SSE_CPU_ROUNDING_NO_FASTER_FOR_D // eliminate confusing hack, less code
#define SSE_CPU_ROUNDING_NOT
#define SSE_CPU_ROUNDING_OR
#define SSE_CPU_ROUNDING_ORI
#define SSE_CPU_ROUNDING_PEA
#define SSE_CPU_ROUNDING_SCC
#define SSE_CPU_ROUNDING_SHIFT_MEM
#define SSE_CPU_ROUNDING_SUB
#define SSE_CPU_ROUNDING_SUBI
#define SSE_CPU_ROUNDING_SUBQ
#define SSE_CPU_ROUNDING_SUBX
#endif
#endif//cpu
#if defined(SSE_FLOPPY)
#define SSE_PASTI_ONLY_STX_OPTION3 // move that option SSE -> disk manager
#define SSE_FLOPPY_EVENT
#define SSE_FLOPPY_EVENT2
#if defined(SSE_DISK)
#define SSE_DISK1//struct
#if defined(SSE_DISK_IMAGETYPE)//3.6.4
#define SSE_DISK_IMAGETYPE1 // replace "TCaps::IsIpf()"
#define SSE_DISK_IMAGETYPE2
#endif
#define SSE_DISK_STW
#if defined(SSE_DISK_STW)
#define SSE_DISK_STW_DISK_MANAGER //new context option
#define SSE_DISK_STW_MFM // bytes are MFM encoded on the disk
#endif
#ifdef WIN32
#define SSE_DISK_GHOST
#endif
#if defined(SSE_DISK_GHOST)
#define SSE_DISK_GHOST_FAKE_FORMAT
#define SSE_DISK_GHOST_SECTOR // commands $A#/$8#
#define SSE_DISK_GHOST_MULTIPLE_SECTORS // commands $B#/$9#
#define SSE_DISK_GHOST_SECTOR_STX1 // in pasti
#endif
#endif//disk
#if defined(SSE_DMA)
#define SSE_DMA_COUNT_CYCLES // again
#endif//dma
#if defined(SSE_DRIVE)
#define SSE_DRIVE_INDEX_PULSE
#define SSE_DRIVE_INDEX_STEP
#define SSE_DRIVE_IP_HACK
#define SSE_DRIVE_MEDIACHANGE // for CAPSImg, Pasti
#define SSE_DRIVE_REM_HACKS 
#if defined(SSE_DRIVE_REM_HACKS)
#define SSE_DRIVE_REM_HACKS2//gaps
#undef SSE_DRIVE_RW_SECTOR_TIMING2
#define SSE_DRIVE_RW_SECTOR_TIMING3 //test v3.7.0 use ID
#define SSE_DRIVE_RW_SECTOR_TIMING4 //remove more hacks
#endif
#if defined(SSE_DRIVE_SINGLE_SIDE)
#undef SSE_DRIVE_SINGLE_SIDE_NAT1
#undef SSE_DRIVE_SINGLE_SIDE_PASTI
#define SSE_DRIVE_SINGLE_SIDE_CAPS //using the new feature
#define SSE_DRIVE_SINGLE_SIDE_NOPASTI // too involved
#define SSE_DRIVE_SINGLE_SIDE_RND//random data instead
#endif
#if defined(SSE_DRIVE_SOUND)
#define SSE_DRIVE_SOUND_SEEK2 // = steps
//#define SSE_DRIVE_SOUND_SEEK3 // buzz sample choice for seek
#define SSE_DRIVE_SOUND_SEEK4
#define SSE_DRIVE_SOUND_SEEK5// option buzz/steps for seek
#define SSE_DRIVE_SOUND_STW
#define SSE_DRIVE_SOUND_VOLUME_2 //bugfix on resume
#endif//snd
#define SSE_DRIVE_STATE //mask
#endif//drive
#if defined(SSE_DMA)
#define SSE_DMA_DOUBLE_FIFO
#define SSE_DMA_DRQ
//#define SSE_DMA_DRQ_RND 
#define SSE_DMA_FIFO_NATIVE2 //bugfix International Sports Challenge-ICS fast
#define SSE_DMA_FIFO_READ_TRACK //TODO
#endif
#if defined(SSE_FDC_ACCURATE) 
#define SSE_FDC_FORCE_INTERRUPT_D8
#define SSE_FDC_IDFIELD_IGNORE_SIDE
#define SSE_FDC_MULTIPLE_SECTORS
//#define SSE_FDC_RESTORE1//undef to remove bug
#endif
#if defined(SSE_PASTI)
#undef SSE_PASTI_ON_WARNING//?
#undef SSE_PASTI_ON_WARNING2//?
#if defined(SSE_PASTI_ONLY_STX)
//#define SSE_PASTI_ONLY_STX_OPTION1 //remove SSE option
//#define SSE_PASTI_ONLY_STX_OPTION2 //remove disk manager option
#endif
#endif
//#define SSE_WD1772
#if defined(SSE_WD1772)
#define SSE_WD1772_CRC 
#define SSE_WD1772_F7_ESCAPE
#define SSE_WD1772_IDFIELD 
#define SSE_WD1772_MFM 
#define SSE_WD1772_PHASE
#define SSE_WD1772_REG2 // DSR, ByteCount
#define SSE_WD1772_REG2_B // StatusType
#endif
#define SSE_YM2149C // turn on/off motor in drives
#endif//floppy
#if defined(SSE_GUI)
#define SSE_GUI_CUSTOM_WINDOW_TITLE
#define SSE_GUI_DISK_MANAGER//was missing in v3.7.2
#define SSE_GUI_DISK_MANAGER_DOUBLE_CLK_GO_UP //habit with some file managers
#define SSE_GUI_DISK_MANAGER_GHOST
#define SSE_GUI_DISK_MANAGER_INSERT_DISKB
#define SSE_GUI_DISK_MANAGER_INSERT_DISKB_LS//load/save option
#define SSE_GUI_DISK_MANAGER_LONG_NAMES1
#define SSE_GUI_DISK_MANAGER_NO_DISABLE_B_MENU // click on icon
#ifdef WIN32
#define SSE_GUI_CLIPBOARD_TEXT
#endif
#if defined(SSE_GUI_CLIPBOARD_TEXT) && defined(SSE_GUI_DISK_MANAGER_LONG_NAMES1)
#define SSE_GUI_DISK_MANAGER_NAME_CLIPBOARD
#endif
//#define SSE_GUI_DISK_MANAGER_RGT_CLK_HD//on DM icon 
#define SSE_GUI_DISK_MANAGER_RGT_CLK_HD2//on HD icon
#undef SSE_GUI_OPTION_SLOW_DISK_SSE
#if defined(SSE_GUI_INFOBOX)//need that for ver??
#undef SSE_GUI_INFOBOX2
//#define SSE_GUI_INFOBOX9 // +2 manuals
#define SSE_GUI_INFOBOX10 // note about thanks
#define SSE_GUI_INFOBOX11 // compiler
#define SSE_GUI_INFOBOX12 // Steem SSE manual replaces readme
#define SSE_GUI_INFOBOX13 // Steem SSE faq replaces faq
#define SSE_GUI_INFOBOX14 // order 'about' 'hints'
#define SSE_GUI_INFOBOX15 // release notes (not sse faq)
#define SSE_GUI_INFOBOX16 // no crash on big files
#endif
#if defined(SSE_GUI_STATUS_STRING)
#undef SSE_GUI_STATUS_STRING_IPF // what if mixed?
#undef SSE_GUI_STATUS_STRING_PASTI // what if mixed?
#define SSE_GUI_STATUS_STRING_68901
#define SSE_GUI_STATUS_STRING_HD
#define SSE_GUI_STATUS_STRING_HALT
#define SSE_GUI_STATUS_STRING_THRESHOLD //not if window too small
#if defined(SSE_DISK_STW)
//#define SSE_GUI_STATUS_STRING_STW // what if mixed? (not def)
#endif
#endif//string
#define SSE_GUI_OPTIONS_DISABLE_DISPLAY_SIZE_IF_NO_BORDER
#define SSE_GUI_OPTIONS_DONT_MENTION_WINDOW_COMPOSITING
#define SSE_GUI_OPTION_DISPLAY_CHANGE_TEXT // normal is small + scanlines
#define SSE_GUI_OPTIONS_DISPLAY_SIZE_IN_DISPLAY
#define SSE_GUI_OPTIONS_STF_IN_MACHINE
#define SSE_GUI_OPTIONS_WU_IN_MACHINE
#define SSE_GUI_OPTIONS_SOUND1 // make some room free
#define SSE_GUI_OPTIONS_SOUND2 // drive sound on sound page
#define SSE_GUI_OPTIONS_SOUND3 // options for PSG and Microwire on sound page
#define SSE_GUI_OPTIONS_SOUND4 // option keyboard click on sound page
#endif
#if defined(SSE_IKBD)
#if defined(SSE_IKBD_6301)
#undef SSE_IKBD_6301_MOUSE_ADJUST_SPEED //def 3.5.1
#define SSE_IKBD_6301_MOUSE_ADJUST_SPEED2 //better
//#define SSE_IKBD_6301_MOUSE_MASK2//hack but game is bugged
#define SSE_IKBD_6301_MOUSE_MASK3 // reset
#define SSE_IKBD_6301_SEND1 // bugfix
#define SSE_IKBD_6301_STUCK_KEYS // reset
#ifdef SSE_BETA
//#define SSE_IKBD_6301_NOT_OPTIONAL
#endif
#define SSE_IKBD_6301_ROM_KEYTABLE //spare an array
#endif//6301
#define SSE_JOYSTICK_JUMP_BUTTON
#endif//ikbd
#if defined(SSE_INTERRUPT)
#undef SSE_INT_HBL_IACK_FIX
#define SSE_INT_HBL_IACK2
#define SSE_INT_VBL_IACK2
#undef SSE_INT_JITTER 
#undef SSE_INT_JITTER_HBL
#undef SSE_INT_JITTER_VBL
#undef SSE_INT_JITTER_RESET
#endif
#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_AUTO_NO_IS_CLEAR//test
#define SSE_INT_MFP_OBJECT // new MC68901 chip in out ST!
#if defined(SSE_INT_MFP_OBJECT)
#define SSE_INT_MFP_IRQ_TIMING //tracking it more precisely
#define SSE_INT_MFP_GPIP_TO_IRQ_DELAY // only for GPIP interrupts
#undef SSE_ACIA_IRQ_DELAY2
#undef SSE_INT_MFP_IACK_LATENCY //same irq
#define SSE_INT_MFP_IACK_LATENCY2 //delay timers
#define SSE_INT_MFP_IACK_LATENCY3 //timer B
#define SSE_INT_MFP_IACK_LATENCY4 //delay timers 
//#define SSE_INT_MFP_IACK_LATENCY5 //timer B 
#define SSE_INT_MFP_TIMERS_NO_BOOST_LIMIT
#define SSE_INT_MFP_OPTION //performance/precision
#define SSE_INT_MFP_UTIL
#define SSE_INT_MFP_SPURIOUS//cool crashes
#define SSE_INT_MFP_CHECKTIMEOUT_ON_STOP
#undef SSE_INT_MFP_PATCH_TIMER_D//Audio Artistic
#define SSE_INT_MFP_READ_DELAY1 
#undef SSE_INT_MFP_WRITE_DELAY1 //Audio Artistic
#define SSE_INT_MFP_WRITE_DELAY2
#if defined(SSE_INT_MFP_RATIO)
#define SSE_INT_MFP_RATIO_OPTION // user can fine tune CPU clock
#define SSE_INT_MFP_RATIO_OPTION2 // L/S
#define SSE_INT_MFP_RATIO_PRECISION_2 // 1 cycle precision
#define SSE_INT_MFP_RATIO_PRECISION3 // 100%
#endif
#define SSE_INT_MFP_RATIO_STE2
//#define SSE_INT_MFP_RATIO_STE3 // = STF
#define SSE_INT_MFP_RATIO_STF2 
#define SSE_INT_MFP_REFACTOR1
#undef SSE_INT_MFP_TIMER_B_NO_WOBBLE //there is wobble
#define SSE_INT_MFP_TIMER_B_WOBBLE2 // 2 instead of 4
#define SSE_INT_MFP_TIMER_B_WOBBLE_HACK //for Sunny
#define SSE_INT_MFP_TIMERS_RATIO1 //unimportant
#define SSE_INT_MFP_TIMERS_INLINE
//#define SSE_INT_MFP_TIMERS_RUN_IF_DISABLED //load!
//#define SSE_INT_MFP_TIMERS_STARTING_DELAY //12->?
//#define SSE_INT_MFP_TIMERS_WOBBLE //as for timer B
#endif//SSE_INT_MFP
#endif
#if defined(SSE_MIDI) 
//#define SSE_MIDI_CHECK1//VERIFY is broken?!
#if defined(SSE_ACIA)
#define SSE_ACIA_MIDI_SR02_CYCLES // cycles instead of agenda
#endif
#endif//midi
#if defined(SSE_MMU)
#define SSE_MMU_SDP1
#define SSE_MMU_SDP2
#endif
#if defined(SSE_OSD)
#undef SSE_OSD_DRIVE_INFO_SSE_PAGE // moving the option...
#define SSE_OSD_DRIVE_INFO_OSD_PAGE // ...here
#define SSE_OSD_FORCE_REDRAW_AT_STOP // works or not...
//#define SSE_OSD_TRACE_ALL_BUILDS -> no, depends on TDebug
#endif
#if defined(SSE_SHIFTER)
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY//My Socks are Weapons
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY2//black display
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY3//don't crash 1
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY4//don't crash 2
#define SSE_SHIFTER_HIRES_OVERSCAN//3.7.0
//#define UNDEF_SSE_SHIFTER_PALETTE_NOISE
#define SSE_SHIFTER_PALETTE_NOISE2
//#define SSE_SHIFTER_PALETTE_STF
#define SSE_SHIFTER_STE_DE_MED_RES // bugfix
#define SSE_SHIFTER_STE_READ_SDP_HSCROLL1 // bugfix  //breaks 20year STE intro
#define SSE_SHIFTER_STE_READ_SDP_SKIP // bugfix
#define SSE_SHIFTER_LINE_MINUS_2_DONT_FETCH //BIG Demo #2 bad raster finally!
#undef SSE_SHIFTER_LINE_PLUS_2_STE
#define SSE_SHIFTER_LINE_PLUS_2_STE2 //tested on hardware
#define SSE_SHIFTER_LINE_PLUS_20B // general
#define SSE_SHIFTER_LINE_PLUS_4
#define SSE_SHIFTER_LINE_PLUS_6
#define SSE_SHIFTER_DOLB_STE
#define SSE_SHIFTER_DOLB1 //again a hack... TODO
#define SSE_SHIFTER_PANIC2 //band order
#define SSE_SHIFTER_STATE_MACHINE2
#endif//shifter
#define SSE_SOUND_DMA_CLOCK //not CPU, apart clock
#if defined(SSE_SOUND_FILTER_STF)
#define SSE_SOUND_FILTER_HATARI
//#define SSE_SOUND_FILTER_STF2 // for samples, original is better
#define SSE_SOUND_FILTER_STF3 // better detect samples
#define SSE_SOUND_FILTER_STF4 // disable option if no chip sound
#define SSE_SOUND_FILTER_STF5 // option in sound
#define SSE_SOUND_INLINE2 
#if defined(SSE_SOUND_INLINE2) //there was a bug to find...
#define SSE_SOUND_INLINE2A
#define SSE_SOUND_INLINE2B
#define SSE_SOUND_INLINE2C
#define SSE_SOUND_INLINE2D
#define SSE_SOUND_INLINE2E
#define SSE_SOUND_INLINE2F
#endif
#define SSE_SOUND_MICROWIRE_MASK1 //bugfix
#define SSE_SOUND_MICROWIRE_MASK2 //incorrect doc (?)
#define SSE_SOUND_MICROWIRE_READ1
#define SSE_SOUND_MICROWIRE_WRITE_LATENCY_B //Antiques 
#undef  SSE_SOUND_VOL //Antiques sounds better without this precaution
#define SSE_SOUND_VOL_LOGARITHMIC_2 // bugfix on resume
#endif
#if defined(SSE_STF)
#undef SSE_STF_8MHZ // we have better option now
#define SSE_STF_MATCH_TOS2 // default = 1.62 instead of 1.06
#endif
#if defined(SSE_TOS)
#undef SSE_TOS_PATCH106 // TOS 1.62 recommended
#define SSE_TOS_BOOTER1//accept TOS boot of the 260ST
#if defined(SSE_DISK_IMAGETYPE) && defined(WIN32) //see note in floppy_drive.cpp
#define SSE_TOS_PRG_AUTORUN// Atari PRG file direct support
#define SSE_TOS_TOS_AUTORUN// Atari TOS file direct support
#endif
#endif
#if defined(SSE_VARIOUS)
#undef SSE_VAR_FULLSCREEN_DONT_START
#define SSE_GUI_PATCH_SCROLLBAR
#define SSE_VAR_RESET_SAME_DISK//test (with .PRG support)
#define SSE_VAR_RESIZE_370
#define SSE_INLINE_370
#define SSE_VAR_SNAPSHOT_INI
#define SSE_VAR_STEALTH2 //bugfix 3.7
#endif//various
#if defined(SSE_VIDEO)
#define SSE_VID_EXT_MON
#if defined(WIN32) 
#undef SSE_VID_RECORD_AVI
#endif
#define SSE_VID_BLOCK_WINDOW_SIZE // option can't change size of window
#define SSE_VID_LOCK_ASPET_RATIO // 
#define SSE_VID_UTIL // miscellaneous functions
//#define SSE_VID_RECORD_MODES // TODO eg record 800 x 600 16bit 60hz... waste?
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_GUARD_EM // don't mix large borders & extended res
#define SSE_VID_BORDERS_GUARD_R2 // don't mix large borders & hi res
//#define SSE_VID_BORDERS_NO_LOAD_SETTING // start at 384 x 270
#endif
#define SSE_VID_CHECK_POINTERS
#if defined(WIN32) 
#if defined(_MSC_VER) && (_MSC_VER >= 1500) //VS2008
#define SSE_VID_DD7 // DirectDraw7 instead of 2
#define SSE_VID_DIRECT3D 
#endif//vs2008
#if defined(BCC_BUILD) // Borland C++ 5.5 (after some efforts!)
#define SSE_VID_DD7      
#define SSE_VID_DIRECT3D  // could be trouble on some machines
#endif
#if _MSC_VER == 1200 //VC6 //it works but we make a 'No D3D' build
//#define SSE_VID_DD7
//#define SSE_VID_DIRECT3D
#endif
#endif//win32
#if defined(SSE_VID_EXT_MON)
#define SSE_VID_EXT_MON_1024X720
#define SSE_VID_EXT_MON_1280X1024
#endif
#if defined(SSE_VID_DIRECT3D)
#undef SSE_VID_CHECK_DDFS
#undef SSE_VID_CHECK_DDFS2
#define SSE_VID_D3D //main
#define SSE_VID_D3D1 //adaptation
#define SSE_VID_D3D2 //adaptation
#define SSE_VID_D3D_LIST_MODES // player can choose
#define SSE_VID_D3D_NO_GUI
#define SSE_VID_D3D_OPTION
//#define SSE_VID_D3D_OPTION2//change tip on fullscreen page
//#define SSE_VID_D3D_OPTION3//disable on fullscreen page
#define SSE_VID_D3D_OPTION4//option on fullscreen page -> disable 2,3
#define SSE_VID_D3D_OPTION5//enable/disable DD options
#define SSE_VID_D3D_STRETCH
#define SSE_VID_D3D_STRETCH_ASPECT_RATIO // like SainT, higher pixels
#if defined(SSE_VID_D3D_STRETCH_ASPECT_RATIO)
#define SSE_VID_D3D_STRETCH_ASPECT_RATIO_OPTION // like SainT, higher pixels
#endif
#define SSE_VID_D3D_STRETCH_FORCE // only stretch: better for list modes
#endif//d3d
#endif//vid
#ifdef SSE_YM2149
#undef SSE_YM2149_ENV_FIX1 //my mistake
#define SSE_YM2149_DELAY_RENDERING // so that we use table also for envelope
#define SSE_YM2149_QUANTIZE1
#if defined(SSE_YM2149_DELAY_RENDERING)  
#undef SSE_YM2149_FIXED_VOL_FIX1 // former 'P.S.G.' option replaced with that
#define SSE_YM2149_DELAY_RENDERING1 // use that table, interpolate...
#define SSE_YM2149_NO_SAMPLES_OPTION // remove 'Samples' option
#endif
#if defined(SSE_YM2149_FIXED_VOL_TABLE)
//#define SSE_YM2149_DYNAMIC_TABLE0 //temp, to build file
#define SSE_YM2149_DYNAMIC_TABLE //using file
#endif
#endif//ym

#endif//v3.7.0


#if SSE_VERSION>=371

#if defined(SSE_CPU_EXCEPTION)
#undef SSE_CPU_IGNORE_WRITE_B_0 // Aladin remove hack
#undef SSE_CPU_SET_DEST_W_TO_0 // Aladin remove hack
#define SSE_CPU_TRUE_PC3 // Aladin real fix
#endif
#if defined(SSE_FLOPPY)
#define SSE_DISK_SCP // Supercard Pro disk image format support
//#define SSE_DISK_SCP_TO_MFM_PREVIEW // keep it, could be useful
#define SSE_DISK_STW_READONLY //3.7.1 last minute
#if defined(SSE_DMA)
#undef SSE_DMA_COUNT_CYCLES //again... ;)
#define SSE_DMA_TRACK_TRANSFER2 // also for CAPS images
#endif
#if defined(SSE_DRIVE)
#define SSE_DRIVE_INDEX_PULSE2 // improvements for SCP
#define SSE_DRIVE_READ_TRACK_11C2 //ProCopy 1.50 Analyze
#if defined(SSE_DRIVE_SOUND)
#define SSE_DRIVE_SOUND_VOLUME_3
#endif
#endif
#if defined(SSE_WD1772)
#define SSE_WD1772_371 // SPC support
#define SSE_WD1772_AM_LOGIC // code inspired by SPS (CapsFDCEmulator.cpp)
#define SSE_WD1772_DPLL // code inspired by MAME/MESS (wd_fdc.c)
//#define SSE_WD1772_MFM_PRODUCE_TABLE // one-shot switch...
#define SSE_WD1772_WEAK_BITS //hack
#endif//wd1772
#endif//flp
#if defined(SSE_MMU)
#define SSE_MMU_SDP1B
#endif
#if defined(SSE_OSD)
#undef SSE_OSD_SCROLLER_DISK_IMAGE // doubles with status bar
#endif
#if defined(SSE_SOUND)
#if defined(SSE_SOUND_MICROWIRE)
#undef SSE_SOUND_MICROWIRE_READ1 //Sleepwalker STE
#endif
#if defined(SSE_SOUND_VOL_LOGARITHMIC_2)
#define SSE_SOUND_VOL_LOGARITHMIC_3
#endif
#endif//snd
//#define SSE_TOS_GEMDOS_FDUP // for EmuTOS - it's fixed already, not def
#if defined(SSE_VARIOUS)
#define SSE_VAR_UNRAR2 //ok with RAR5
#endif
#ifdef SSE_YM2149 
#define SSE_YM2149_DELAY_RENDERING2 //bug in v3.7.0
#define SSE_YM2149_QUANTIZE2 //bug in v3.7.0
#endif
#if defined(SSE_GUI_INFOBOX)
#define SSE_GUI_INFOBOX17 //mousewheel on links
#endif

#endif//371


#if SSE_VERSION>=372

#if defined(SSE_CPU)
#define SSE_CPU_256MHZ
//#define SSE_CPU_512MHZ // ready if someone requests it!
#define SSE_CPU_DIVS_OVERFLOW_PC //long-standing bug that could crash Steem
#endif
#if defined(SSE_DISK)
#define SSE_DISK2 //add info
#define SSE_DISK_HFE // HxC floppy emulator HFE image support
#define SSE_DISK_HFE_DISK_MANAGER // creating HFE images in Steem
#define SSE_DISK_HFE_TRIGGER_IP // changes nothing?
//#define SSE_DISK_HFE_TO_STW // using Steem to convert current disk
#define SSE_DISK_STW2 
#define SSE_DISK_HFE_DYNAMIC_HEADER //spare some memory
#define SSE_DISK_REMOVE_DISK_ON_SET_DISK //hmm, could explain some strange bugs
#define SSE_DISK_SCP2A //id
#define SSE_DISK_SCP2B //all IP 1->2 (..->1 during rev) - perf?
#define SSE_DISK_SCP_DRIVE_WOBBLE
//#define SSE_DISK_STW_TRIGGER_IP //TODO
#endif
#if defined(SSE_GUI)
#define SSE_GUI_ASSOCIATE_HFE // they're generally not zipped...
#define SSE_GUI_DISK_MANAGER_INSERT_DISKB_REMOVE //works?
#define SSE_GUI_INFOBOX18 //open links in new instance, IE
#define SSE_GUI_STATUS_STRING_DISK_TYPE // A:MSA B:STW
#define SSE_GUI_STATUS_STRING_HISPEED
#define SSE_GUI_STATUS_STRING_SINGLE_SIDE
#endif//gui
#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_SPURIOUS_372 // bugfix Return -HMD "spurious spurious"//see 380
#endif
#if defined(SSE_SOUND)
#define SOUND_DISABLE_INTERNAL_SPEAKER //of course, about time
#endif
#ifdef SSE_TOS
#define SSE_TOS_CHECK_VERSION // to avoid ID something else as TOS
#define SSE_TOS_STE_FAST_BOOT2 // check each cold reset
//#define SSE_TOS_GEMDOS_RESTRICT_TOS // enforce T1.04 or T1.62 only for GEMDOS
#define SSE_TOS_GEMDOS_RESTRICT_TOS2 // warning 
#endif
#if defined(SSE_VARIOUS)
#if defined(WIN32) && defined(VC_BUILD) // works with VC6, VS2008 not BCC
#define SSE_VAR_ARCHIVEACCESS // 7z support
#define SSE_VAR_ARCHIVEACCESS2 // bz2 (with modded dll), gz, tar, arj
//#define SSE_VAR_ARCHIVEACCESS3 // zip managed by ArchiveAccess.dll by default - no, because of MSA Converter TODO
//#define SSE_VAR_ARCHIVEACCESS4 // remove code for unzipd32.dll - what if archiveaccess fails!
#endif
#define SSE_VAR_MAIN_LOOP1 // protect from crash with try/catch
#define SSE_VAR_NO_UPDATE_372
#define SSE_VAR_RESIZE_372
#endif//var
#if defined(SSE_VIDEO)
#define SSE_VID_D3D_CRISP //D3D can do that
#define SSE_VID_D3D_CRISP_OPTION //the harder part!
#endif//vid
#if defined(SSE_WD1772)
#undef SSE_WD1772_WEAK_BITS
#define SSE_WD1772_372 // bugfix write MFM word with missing clock bit on data!
#endif

#endif//372


#if SSE_VERSION>=373

#ifdef SSE_ACIA
#define SSE_ACIA_OVR_TIMING // Snork/Defulloir
#endif

#ifdef SSE_BLITTER
#define SSE_BLITTER_RELAPSE2 // quick fix before v3.8
#endif

#ifdef SSE_CPU
#define SSE_CPU_ABUS_ACCESS // argh, was forgotten
#undef SSE_CPU_PREFETCH_TIMING_MOVEM_HACK // grr forgot this too!! (Sea of Colour)
#endif

#if defined(SSE_FLOPPY)
#define SSE_PASTI_AUTO_SWITCH2
#ifdef SSE_DISK
#define SSE_DISK_MSA
#define SSE_DISK_ST
#endif
#ifdef SSE_DISK_SCP
#define SSE_DISK_SCP373 // War Heli
#endif
#ifdef SSE_DRIVE
#define SSE_DRIVE_INIT_373 //bugfix, don't play with 300 rpm (STW)
#endif
#endif//floppy

#ifdef SSE_GUI
#define SSE_GUI_SHORTCUT_SELECT_DISK//request
#endif

#ifdef SSE_IKBD_6301
#define SSE_IKBD_6301_373 
#define SSE_IKBD_6301_MINIRAM // save close to 60k, at last
#endif

#ifdef SSE_INT_MFP
#define SSE_INT_MFP_UTIL2 // bugfix parameter not used
#endif

#ifdef SSE_SHIFTER
#undef SSE_SHIFTER_STE_READ_SDP_HSCROLL1 //breaks 20year STE intro
#endif

#ifdef SSE_TOS
#define SSE_TOS_GEMDOS_RESTRICT_TOS3 // EmuTOS + PRG/TOS
#define SSE_TOS_PRG_AUTORUN2 //better behaviour
#endif

#ifdef SSE_VIDEO
#define SSE_VID_D3D_373 // avoid crash
#define SSE_VID_EXT_FS1
#define SSE_VID_EXT_FS2 // max screen
#endif

#endif//373



//////////
// v3.8 //
//////////

 
#if SSE_VERSION>=380

#endif//380


///////////
// DEBUG //
///////////

// SSE_DEBUG is defined or not by the environment, just like STEVEN_SEAGAL
// debug switches are grouped here

#if defined(SSE_DEBUG) // Boiler + debug build

#ifdef SSE_ACIA
// #define SSE_ACIA_TEST_REGISTERS//????
#endif
#if defined(SSE_DEBUG) && defined(DEBUG_BUILD)
#define SSE_BOILER
#endif
#if defined(SSE_BOILER)
#define SSE_BOILER_ACIA_373 // fix annoying bug 6301 mode
#define SSE_BOILER_AUTO_FLUSH_TRACE// v3.7.1
#define SSE_BOILER_BLIT_IN_HISTORY // v3.7.1
#define SSE_BOILER_BLIT_WHEN_STOPPED // v3.7.1
#define SSE_BOILER_BLAZING_STEP_OVER 
#define SSE_BOILER_BROWSER_6301
#define SSE_BOILER_BROWSER_ACIA
#define SSE_BOILER_BROWSER_BLITTER
#define SSE_BOILER_BROWSER_DMASOUND
#define SSE_BOILER_BROWSER_INSTRUCTIONS //window name
#define SSE_BOILER_BROWSER_PSEUDO_IO_SCROLL // for the bigger 6301 browser
#define SSE_BOILER_BROWSER_SHIFTER
#define SSE_BOILER_BROWSER_VECTORS
#define SSE_BOILER_BROWSERS_VECS // in 'reg' columns, eg TB for timer B
#define SSE_BOILER_CLIPBOARD // v3.4 right-click on 'dump' to copy then paste
//#define SSE_BOILER_CPU_LOG_NO_STOP // never stop
#define SSE_BOILER_CPU_TRACE_NO_STOP // depends on 'suspend logging'
#define SSE_BOILER_DECRYPT_TIMERS
#define SSE_BOILER_DISPLACEMENT_DECIMAL
#ifdef SSE_IKBD_6301
#define SSE_BOILER_DUMP_6301_RAM
#define SSE_BOILER_DUMP_6301_RAM2 //disa 3.7.3
#endif
#define SSE_BOILER_EXCEPTION_NOT_TOS
#define SSE_BOILER_EXTRA_IOLIST
#define SSE_BOILER_FAKE_IO //to control some debug options
#if defined(SSE_BOILER_FAKE_IO)
#define SSE_DEBUG_TRACE_IO//
#define SSE_BOILER_MUTE_SOUNDCHANNELS
#define SSE_BOILER_MUTE_SOUNDCHANNELS_ENV
#define SSE_BOILER_MUTE_SOUNDCHANNELS_NOISE
#ifdef SSE_OSD
#define SSE_OSD_CONTROL
#define SSE_BOILER_FRAME_INTERRUPTS//OSD, handy
#define SSE_BOILER_FRAME_INTERRUPTS2
#endif
#define SSE_BOILER_MUTE_SOUNDCHANNELS //fake io
#define SSE_BOILER_TRACE_CONTROL //beyond log options
#define SSE_BOILER_VBL_HBL 
#define SSE_BOILER_VIDEO_CONTROL
#endif//fake io
//#define SSE_BOILER_FLUSH_TRACE
#define SSE_BOILER_FRAME_REPORT_MASK // for interactive control in boiler
#define SSE_BOILER_LOGSECTIONS1 //v3.7.1
#define SSE_BOILER_MENTION_READONLY_BROWSERS
#define SSE_BOILER_MOD_REGS // big letters, no =
#define SSE_BOILER_MOD_VBASE
#define SSE_BOILER_MOD_VBASE2 //move it
#define SSE_BOILER_MONITOR_372 // v3.7.2 to clarify code
#define SSE_BOILER_MONITOR_IO_FIX1 // ? word check, not 2x byte on word access
#define SSE_BOILER_MONITOR_RANGE // will stop for every address between 2 stops
#define SSE_BOILER_MONITOR_TRACE // v3.7.2 mode log also in TRACE (duh!)
#define SSE_BOILER_MONITOR_VALUE // specify value (RW) that triggers stop
#define SSE_BOILER_MONITOR_VALUE2 // write before check
#define SSE_BOILER_MONITOR_VALUE3 // add checks for CLR
#define SSE_BOILER_MONITOR_VALUE4 // corrections (W on .L)
#define SSE_BOILER_MOUSE_WHEEL // yeah!
#define SSE_BOILER_MOVE_OTHER_SP
#define SSE_BOILER_MOVE_OTHER_SP2//SSP+USP
#define SSE_BOILER_NEXT_PRG_RUN//request
#define SSE_BOILER_NODIV // no DIV log necessary anymore
#define SSE_BOILER_NO_SET_BRK_PC_MENU
#define SSE_BOILER_NO_SOUND_DAMPING //PSG filter control 'd' and 'a'
#define SSE_BOILER_PSEUDO_STACK
#define SSE_BOILER_PSEUDO_STACK2 //v3.7.1
#define SSE_BOILER_RUN_TO_RTS
#define SSE_BOILER_SHOW_ACT
#define SSE_BOILER_SHOW_FRAME
#define SSE_BOILER_SHOW_FREQ // sync mode
#define SSE_BOILER_SHOW_INTERRUPT // yeah!
#define SSE_BOILER_SHOW_RES // shift mode
#define SSE_BOILER_SHOW_SDP // the draw pointer
#define SSE_BOILER_SHOW_SR // in HEX on the left of bit flags
#define SSE_BOILER_SHOW_TRICKS //line
#define SSE_BOILER_SHOW_TRICKS2 //frame
#define SSE_BOILER_SSE_PERSISTENT // L/S options 
//#define SSE_BOILER_68030_STACK_FRAME
#define SSE_BOILER_STACK_CHOICE
#define SSE_BOILER_TIMER_B // instead of 0
#define SSE_BOILER_TIMERS_ACTIVE // (in reverse video) yeah!
#define SSE_BOILER_TRACE_MFP1 // one log option for MFP, one for interrupt
//#define SSE_BOILER_TRACE_NOT_OPTIONAL
#define SSE_BOILER_WIPE_TRACE // as logfile
#define SSE_BOILER_WIPE_TRACE2//need date
#endif//boiler

#ifdef SSE_CPU
//#define SSE_CPU_EXCEPTION_TRACE_PC // reporting all PC (!)
#if defined(SSE_BETA)
//#define SSE_CPU_DETECT_STACK_PC_USE // push garbage!!
#endif
#if defined(SSE_CPU_PREFETCH) 
//#define SSE_CPU_PREFETCH_TRACE 
#endif
//#define SSE_CPU_TRACE_DETECT
#endif//cpu

#ifdef SSE_FLOPPY
#define SSE_DEBUG_FDC_TRACE_STATUS //spell out status register
#define SSE_DEBUG_IPF_TRACE_SECTORS // show sector info (IPF)
#define SSE_DRIVE_COMPUTE_BOOT_CHECKSUM //TODO boiler control
//#define SSE_DEBUG_FDC_TRACE_STR // trace read STR (careful)
#define SSE_DEBUG_WRITE_TRACK_TRACE_IDS
//#define SSE_YM2149_REPORT_DRIVE_CHANGE // as FDC trace
#define SSE_DEBUG_FDC_TRACE_IRQ //for some reason was commented out in v3.7.2
#endif//floppy

#define SSE_DEBUG_FRAME_REPORT //it was already there in v3.3!
#if defined(SSE_DEBUG_FRAME_REPORT)
#ifdef SSE_BOILER
#define SSE_BOILER_FRAME_REPORT_MASK2
#endif
#define SSE_DEBUG_FRAME_REPORT_ON_STOP // each time we stop emulation
// Normally those are controlled with the Control mask browser
//#define SSE_DEBUG_FRAME_REPORT_ACIA
//#define SSE_DEBUG_FRAME_REPORT_BLITTER
//#define SSE_DEBUG_FRAME_REPORT_PAL
//#define SSE_DEBUG_FRAME_REPORT_HSCROLL // & linewid
//#define SSE_DEBUG_FRAME_REPORT_SDP_LINES
//#define SSE_DEBUG_FRAME_REPORT_SDP_READ//?
//#define SSE_DEBUG_FRAME_REPORT_SDP_WRITE//?
//#define SSE_DEBUG_FRAME_REPORT_SHIFTER_TRICKS
//#define SSE_DEBUG_FRAME_REPORT_SHIFTER_TRICKS_BYTES
//#define SSE_DEBUG_FRAME_REPORT_SHIFTMODE
//#define SSE_DEBUG_FRAME_REPORT_SYNCMODE
//#define SSE_DEBUG_FRAME_REPORT_VIDEOBASE
///#define SSE_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN//no, sthg else!
#endif//frame report

#ifdef SSE_IKBD_6301
//#define SSE_IKBD_6301_DISASSEMBLE_ROM 
#define SSE_IKBD_6301_IKBDI // command interpreter
#define SSE_DEBUG_IKBD_6301_TRACE
#if defined(SSE_DEBUG_IKBD_6301_TRACE)
#define SSE_DEBUG_IKBD_6301_TRACE_SCI_RX
#define SSE_DEBUG_IKBD_6301_TRACE_SCI_TX
//#define SSE_DEBUG_IKBD_6301_TRACE_KEYS
//#define SSE_DEBUG_IKBD_6301_TRACE_INT_TIMER
//#define SSE_DEBUG_IKBD_6301_TRACE_INT_SCI
//#define SSE_DEBUG_IKBD_6301_TRACE_STATUS
//#define SSE_DEBUG_IKBD_6301_TRACE_WRITES
#endif
#endif//6301

#define SSE_DEBUG_LOG_OPTIONS // mine, boiler or _DEBUG

#ifdef SSE_INT_MFP
//#define SSE_DEBUG_MFP_DEACTIVATE_IACK_SUBSTITUTION // to check cases
#endif

#if defined(SSE_MIDI)
//#define SSE_MIDI_TRACE_BYTES_IN
//#define SSE_MIDI_TRACE_BYTES_OUT
//#define SSE_MIDI_TRACE_BYTES_OUT_OVR
//#define SSE_MIDI_TRACE_READ_STATUS
#endif//midi

#ifdef SSE_OSD
#define SSE_OSD_DEBUG_MESSAGE
//#define SSE_OSD_DEBUG_MESSAGE_FREQ // tell when 60hz (?)
#define SSE_OSD_DRIVE_INFO_EXT // v3.7.1
#endif

#define SSE_DEBUG_REPORT_SCAN_Y_ON_CLICK
#define SSE_DEBUG_REPORT_SDP // tracking sdp at start of each scanline
#define SSE_DEBUG_REPORT_SDP_ON_CLICK // yeah!
#define SSE_DEBUG_RESET

#ifdef SSE_SHIFTER
#if !defined(SSE_BOILER_TRACE_CONTROL)
//#define SSE_DEBUG_FRAME_REPORT_VERTICAL_OVERSCAN
#endif
//#define SSE_SHIFTER_DRAW_DBG  // totally bypass CheckSideOverscan() & Render()
//#define SSE_SHIFTER_IOR_TRACE // specific, not "log"
//#define SSE_SHIFTER_IOW_TRACE // specific, not "log"
#if !defined(SSE_DEBUG_TRACE_IDE) && !defined(SSE_UNIX)
#define SSE_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif
#if defined(SSE_SHIFTER_SDP)
//#define SSE_SHIFTER_SDP_TRACE 
//#define SSE_SHIFTER_SDP_TRACE2//todo control mask?
//#define SSE_SHIFTER_SDP_TRACE3 // report differences with Steem v3.2 
#endif
//#define SSE_SHIFTER_STEEM_ORIGINAL // only for debugging/separate blocks
//#define SSE_VID_VERT_OVSCN_OLD_STEEM_WAY // only for vertical overscan
#endif//shifter

#define SSE_DEBUG_START_STOP_INFO
#define SSE_DEBUG_START_STOP_INFO2

#define SSE_DEBUG_TRACE
#ifdef SSE_DEBUG_TRACE
#ifdef _DEBUG // VC
#define SSE_DEBUG_TRACE_IDE
#endif
#if defined(DEBUG_BUILD) 
#define SSE_DEBUG_TRACE_FILE
#elif defined(UNIX)
#define SSE_DEBUG_TRACE_FILE
#else//VC
//#define SSE_DEBUG_TRACE_FILE
#endif
#endif//trace

#ifdef SSE_TOS
#define SSE_TOS_DONT_TRACE_3F//read file
#define SSE_TOS_DONT_TRACE_40//write file
#define SSE_TOS_DONT_TRACE_42//seek file
#define SSE_TOS_TRACE_CONOUT
#endif

#if defined(SSE_UNIX)
#define SSE_UNIX_TRACE // TRACE into the terminal (if it's open?)
#endif

#ifdef SSE_VARIOUS
#define SSE_VAR_NO_INTRO
#endif

//#define SHOW_DRAW_SPEED //was already in Steem

#endif//SSE_DEBUG


///////////////
// DEV BUILD //
///////////////

#if defined(SSE_BETA) || defined(SSE_BETA_BUGFIX)
#define TEST01//quick switch
#define TEST02//track bug
//#define TEST03
//#define TEST04
//#define TEST05
//#define TEST06 
//#define TEST07
//#define TEST08
//#define TEST09
#endif

#if defined(SSE_PRIVATE_BUILD)
//#define ENABLE_VARIABLE_SOUND_DAMPING
//#define ENABLE_VARIABLE_SOUND_DAMPING2
#define SSE_GUI_OPTION_FOR_TESTS 
#endif

#if defined(SSE_BETA)
// those switches are later moved to both features and version zones!
// TODO only 1 zone (again)

//#define SSE_DRIVE_WRITE_TRACK_11
//#define SSE_GUI_FULLSCREEN_NO_VSYNC_OPTION //but all the rest?
//#define SSE_SOUND_APART_BUFFERS //TODO, one for PSG one for DMA, but Microwire?

#define SSE_ACSI //3.8.0 new feature

#if defined(SSE_ACIA)
#define SSE_ACIA_380
#endif

#if defined(SSE_ACSI)
#define SSE_ACSI_FORMAT 
#ifdef SSE_DEBUG
#define SSE_ACSI_BOOTCHECKSUM
#endif
#define SSE_ACSI_DISABLE_HDIMG // former Steem medium-level stub
#define SSE_ACSI_INQUIRY // could even do without but it's too cool
#define SSE_ACSI_INQUIRY2 // take name of file without extension
#define SSE_ACSI_LED // cool too
#define SSE_ACSI_LS
#define SSE_ACSI_MEGASTF
#define SSE_ACSI_MULTIPLE
#define SSE_ACSI_MULTIPLE2
#define SSE_ACSI_MODESELECT
#define SSE_ACSI_NOGUISELECT // ignored if SSE_ACSI_HDMAN defined
#define SSE_ACSI_OPTION
#define SSE_ACSI_OPTION_INDEPENDENT // of 'HardDiskMan.DisableHardDrives'
#define SSE_ACSI_REQUEST_SENSE
#define SSE_ACSI_TIMING // ADAT -> slower (risky?)
#ifdef WIN32
#define SSE_ACSI_HDMAN // browser
#define SSE_ACSI_ICON
#endif
#endif//acsi

#if defined(SSE_BLITTER)
#undef SSE_BLITTER_RELAPSE
#define SSE_BLT_CLEAR_HOG
#define SSE_BLT_TIMING_CPU_NO_BUS
#define SSE_BLT_TIMING_FXSR
//#define SSE_BLT_TIMING_NFSR//mistake, MFD
#define SSE_BLT_TIMING_START_BLITTER
#endif//blt

#if defined(SSE_BOILER)
//#define SSE_BOILER_TRACE_NOT_OPTIONAL
#endif//boiler

#ifdef SSE_CPU
#define SSE_CPU_512MHZ
#define SSE_CPU_1GHZ 
#define SSE_CPU_2GHZ
//#define SSE_CPU_3GHZ
//#define SSE_CPU_4GHZ//after this I hope they'll leave me alone for a while
#define SSE_CPU_E_CLOCK4
#define SSE_CPU_ROUNDING_BUS // big change, much overhead, little effect 
#define SSE_CPU_ROUNDING_BUS2 // blitter problem
#define SSE_CPU_TIMINGS_REFACTOR_FETCH // moving timings to fetching functions
#define SSE_CPU_TIMINGS_REFACTOR_PUSH // count timing in push macros
#endif

#ifdef SSE_GUI
#define SSE_GUI_CONFIG_FILE // new option to L/S ini files
#if defined(SSE_GUI_CONFIG_FILE)
#define SSE_GUI_CONFIG_FILE2 // TOS: relative path
#endif
#define SSE_GUI_380//tmp name
#define SSE_GUI_RESET_BUTTON2
#if defined(SSE_GUI_DISK_MANAGER)
#define SSE_GUI_DISK_MANAGER_RGT_CLK_HD3 //when changing on/off in hdm
#define SSE_GUI_DISK_MANAGER_HD_SELECTED //stay pushed if on
#endif

#if defined(SSE_GUI_STATUS_STRING)
#undef SSE_GUI_STATUS_STRING_FULL_ST_MODEL // or more complicated
#define SSE_GUI_STATUS_STRING_380 //will be just this switch for rlz
#define SSE_GUI_STATUS_STRING_ADAT_ICON 
#define SSE_GUI_STATUS_STRING_CHIPSET_ICON 
#define SSE_GUI_STATUS_STRING_HACKS_ICON
#define SSE_GUI_STATUS_STRING_HD_ICON 
#define SSE_GUI_STATUS_STRING_TOSFLAG
#endif
#endif//gui

#if defined(SSE_GLUE)
#define SSE_GLUE_FRAME_TIMINGS
#if defined(SSE_GLUE_FRAME_TIMINGS)
#undef SSE_SHIFTER_FIX_LINE508_CONFUSION
#undef SSE_TIMINGS_FRAME_ADJUSTMENT
#undef SSE_INT_VBL_STF
#define SSE_GLUE_FRAME_TIMINGS_A
#define SSE_GLUE_FRAME_TIMINGS_B // remove old var and functions
#define SSE_GLUE_FRAME_TIMINGS_C // extended resolutions
#define SSE_GLUE_THRESHOLDS // computing thresholds only when changing option
#define SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#undef SSE_SHIFTER_LEFT_OFF_60HZ//forget it
//#define SSE_GLUE_001 //to show all the Tekila oddities
#define SSE_GLUE_002//no safety net
#endif//SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1
#endif//SSE_GLUE_FRAME_TIMINGS
#endif//glue

#ifdef SSE_IKBD_6301
#undef SSE_IKBD_6301_SET_TDRE //SSE_IKBD_6301_373 was defined anyway
#define SSE_IKBD_6301_380
#define SSE_IKBD_6301_EVENT
#endif//6301

#if defined(SSE_INTERRUPT)
#define SSE_INT_ROUNDING
#endif

#if defined(SSE_INT_HBL)
#undef SSE_INT_HBL_E_CLOCK_HACK // made a patch instead
#define SSE_INT_HBL_380 
#endif

#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_READ_DELAY2 //timer B
//#define SSE_INT_MFP_RECORD_PENDING_TIMING
#define SSE_INT_MFP_REFACTOR2 //simpler way for IACK and Spurious
#if defined(SSE_INT_MFP_REFACTOR2)
#define SSE_INT_MFP_IS_DELAY
#undef SSE_INT_MFP_IACK_LATENCY4 // adios "skip timer" hack
#endif//ref2
#define SSE_INT_MFP_REFACTOR3 //enums 
#undef SSE_INT_MFP_SPURIOUS_372
#undef SSE_INT_MFP_TIMER_B_AER // refactor
#define SSE_INT_MFP_TIMER_B_AER2 // refactor
//#define SSE_INT_MFP_TIMER_B_PULSE //TODO
#define SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS // timer B should be updated
#define SSE_INT_MFP_TIMERS_WOBBLE //(v3.8.0: Overscan #6B; Spurious)
//#define SSE_INT_MFP_RATIO_STE3 // same ratio as STF?
#define SSE_INT_MFP_TIMERS_STARTING_DELAY //
//#define SSE_INT_MFP_UPDATE_IP_ON_GPIP_CHANGE //mistake, MFD (do test prg b4?)
#endif

#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_380
#endif

#if defined(SSE_MMU)
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#define SSE_MOVE_SHIFTER_CONCEPTS_TO_MMU1
#endif
#endif

#if !defined(SSE_DEBUG)
#define SSE_OSD_DRIVE_INFO_EXT // STX, MSA... 
#endif

#ifdef SSE_SHIFTER
#define SSE_SHIFTER_380 // some modest refactoring, debugging
#define SSE_SHIFTER_HSCROLL_380_A // better test, should new HSCROLL apply on current line
#define SSE_SHIFTER_HSCROLL_380_A1
#define SSE_SHIFTER_HSCROLL_380_B // new variable to hold HSCROLL at start of line
#ifdef SSE_SHIFTER_HSCROLL_380_B
#define SSE_SHIFTER_HSCROLL_380_B1 // use this instead of HSCROLL for some tests
#endif
#define SSE_SHIFTER_HSCROLL_380_C // update shifter_pixel for new HSCROLL
#define SSE_SHIFTER_HIRES_RASTER // edgy stuff in monochrome mode
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY5//better test, compatible with GLUE refactoring
#endif
//#undef SSE_SHIFTER_MED_RES_SCROLLING_360
#define SSE_SHIFTER_MED_RES_SCROLLING_380
#endif//sft

#ifdef SSE_STF
#define SSE_STE_4MB
#define SSE_STF_MATCH_TOS3 // + default country
#endif

#ifdef SSE_TOS
//#define DISABLE_STEMDOS
//#define DISABLE_STEMDOS2//TODO
#define SSE_TOS_GEMDOS_DRVBIT // sooner
#define SSE_TOS_GEMDOS_RESTRICT_TOS3 // EmuTOS + PRG/TOS
#define SSE_TOS_PRG_AUTORUN2
#endif

#ifdef SSE_VARIOUS
#define SSE_VAR_KEYBOARD_CLICK2 // persistent
#define SSE_VAR_SNAPSHOT_INI2
#endif

#ifdef SSE_VIDEO
#define SSE_VID_380
#define SSE_VID_D3D_380
#define SSE_VID_DISABLE_AUTOBORDER
#define SSE_VID_DISABLE_AUTOBORDER2 // move options around
#define SSE_VID_STRETCH_ASPECT_RATIO 
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_LIMIT_TO_245
#define SSE_VID_BORDERS_LINE_PLUS_20
#define SSE_VID_BORDERS_416_NO_SHIFT2 //remove hack condition
#define SSE_VID_BORDERS_416_NO_SHIFT3 //?
#endif
#endif

#endif//beta

#if defined(SSE_BETA_BUGFIX)
#endif//bugfix

#else//!SS
/////////////////////////////////////
// if STEVEN_SEAGAL is NOT defined //
/////////////////////////////////////
// a fair warning (it actually helps detect building errors)
#pragma message("Are you mad? You forgot to #define STEVEN_SEAGAL!") 
#include "SSEDecla.h" // still need to neutralise debug macros

#endif//?SS

#endif// #ifndef STEVEN_SEAGAL_H 
