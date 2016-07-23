// for v3.8.3
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
-A folder 'dsp' in '3rdparty' for bad Microwire emulation
-A file 'div68kCycleAccurate.c' in '3rdparty\pasti', to use the correct DIV 
timings found by ijor (also author of Pasti).
-A folder 'SDL-WIN' for future (?) SDL support
-A folder 'unRARDLL' in '3rdparty' for unrar support
-A folder 'various' in '3rdparty'
-A folder 'ArchiveAccess' in '3rdparty' for 7z (and more) support
-A folder 'hfe' in '3rdparty' for HFE support
-Files xxx.decla.h to better separate declaration/implementation

Other mods are in Steem code, inside blocks where STEVEN_SEAGAL is defined.
Many other defines are used to segment code. This is heavy but it makes 
debugging a lot easier (real life-savers when something is broken).

Switches are sorted by version, sometimes you must search a bit.

To enjoy the new features, you must define STEVEN_SEAGAL!
If not, you should get the last 3.2 build that compiles in VC6 (only
thing changed is no update attempt).

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

Since v3.7.3, the VS2008 build uses the DLL C++ library. It is so optimised
that the EXE is smaller than original Steem 3.2 despite all the additions.

Project files are included.
    
SSE.h is supposed to mainly be a collection of compiling switches (defines).
It should include nothing and can be included everywhere.
Normally switches are optional, but they also are useful just to mark
code changes.

SSE_DEBUG, if needed, should be defined in the project/makefile.
It has an effect on both the boiler and the VS debug build.

Some switches have been simplified since release. It's easy enough
to separate them again for bug tracking.

*/


/*

Important (note to self)

Release: not SSE_BETA
All SSE_TEST_ON must go
Version for snapshot (in LoadSave.h) + Windows properties (rc\resource.rc)
Rebuild so that dates are correct

Beta: not SSE_PRIVATE_BUILD

*/

#if defined(VC_BUILD) // so that v3.2+ compiles
#define COMPILER_VC6 // Visual C++ 6.0
#if _MSC_VER >= 1500
#define SSE_VS2008 // Visual C++ 2008
#endif
#endif
#define NO_RAR_SUPPORT // rarlib, SSE build supports unrar.dll

#if defined(STEVEN_SEAGAL) 

#define SSE_BUILD
#define SSE_COMPILER  //  warnings, errors... 
#define SSE_STRUCTURE //  necessary for the SSE build


/////////////
// VERSION //
/////////////

#define SSE_VERSION 383 // versions down to 330 still compile
                        // full v330 not publicly available anymore
                        // for v32+, don't define STEVEN_SEAGAL

#if SSE_VERSION>382 //last release
#define SSE_BETA //title, OSD, plus some testing - new features
#define SSE_BETA_BUGFIX // beta for just bugfixes
#if defined(SSE_BETA) || defined(SSE_BETA_BUGFIX)
//#define SSE_PRIVATE_BUILD // my "beta" option
#endif
#endif


//////////////////
// BIG SWITCHES // 
//////////////////

// Normally you can disable one of these and it should still compile and run

#define SSE_BLITTER    // spelled BLiTTER by those in the known!
#define SSE_CPU        // MC68000 microprocessor
#define SSE_GUI        // Graphic User Interface
#define SSE_HACKS      // an option for dubious fixes
#define SSE_HARDDISK   // GEMDOS improvements + ACSI feature
#define SSE_INTERRUPT  // HBL, VBL, MFP
#define SSE_FLOPPY     // DMA, FDC, Pasti, etc
#define SSE_KEYBOARD   // ACIA, IKBD
#define SSE_MIDI       // Musical Instrument Digital Interface
#define SSE_OSD        // On Screen Display (drive leds, track info, logo)
#define SSE_ROM        // Cartridge, TOS
#define SSE_SOUND      // YM2149, STE DMA sound, Microwire
#define SSE_STF        // switch STF/STE
#define SSE_TIMINGS    // misc. timings
#ifdef UNIX
#define SSE_UNIX       // XSteem SSE
#endif
#define SSE_VARIOUS    // Mouse capture, keyboard click, unrar...
#define SSE_VIDEO      // large borders, screenshot, recording
#define SSE_VIDEO_CHIPSET // Shifter, Glue, Mmu
#define SSE_YM2149     // Sound chipset, also used for drive control

/////////////////////////
// NOT SO BIG SWITCHES //
/////////////////////////

#if defined(SSE_CPU)
#define SSE_CPU_E_CLOCK      // [joke]
#define SSE_CPU_EXCEPTION    // crash like Windows 98
#define SSE_CPU_INSTR        // rewriting some instructions
#define SSE_CPU_POKE         // poke like a C64 
#define SSE_CPU_PREFETCH     // prefetch like a dog
#define SSE_CPU_ROUNDING     // round like a rolling stone
#endif//cpu

#if defined(SSE_FLOPPY)
#define SSE_DISK       // Disk images
#define SSE_DISK_EXT   // Extensions (refactoring), available for all versions
#if defined(WIN32)
#define SSE_DISK_PASTI // Improvements in Pasti support (STX disk images)
#endif
#define SSE_DRIVE      // SF314 floppy disk drives
#define SSE_DMA        // Custom Direct Memory Access chip (disk)
#define SSE_FDC        // "native Steem" (fdc.cpp)
#define SSE_WD1772     // WD1772 floppy disk controller: object
#endif

#if defined(SSE_INTERRUPT)
#define SSE_INT_HBL    // Horizontal Blank interrupt (340)
#define SSE_INT_MFP    // MC68901 Multi-Function Peripheral Chip
#define SSE_INT_VBL    // Vertical Blank interrupt (340)
#endif

#if defined(SSE_KEYBOARD)
#define SSE_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SSE_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#endif

#if defined(SSE_ROM)
#define SSE_CARTRIDGE  // ROM Cartridge slot
#define SSE_TOS        // The Operating System
#endif

#if defined(SSE_VARIOUS)
#define SSE_VAR_DEAD_CODE // just an indication
#endif

#if defined(SSE_VIDEO) 
//#define SSE_VID_SDL        // Simple DirectMedia Layer (TODO?)
#if defined(WIN32)
#define SSE_VID_BORDERS // option display size
#endif
#endif

#if defined(SSE_VIDEO_CHIPSET) // all or nothing, hence the chipset switch
// 
#define SSE_GLUE       // General Logic Unit
#define SSE_MMU        // Memory Manager Unit (of the ST, no paging)
#define SSE_SHIFTER    // Video Shifter
#endif



#if _MSC_VER == 1200 //VC6: DirectDraw build
#define SSE_NO_D3D
#else
#define SSE_NO_DD //v3.8.2
#endif

//////////////
// COMPILER //
//////////////

#if defined(SSE_COMPILER)

#ifdef WIN32

#define SSE_DELAY_LOAD_DLL // can run without DLL//never worked with bcc???
#if _MSC_VER>1200 // higher than VC6
#define SSE_VS2003_DELAYDLL
#endif
#if _MSC_VER >= 1900 //VS2015
#define SSE_VS2015
#endif

#endif//w32

#ifdef _WIN64
#define SSE_X64 //introduced in v3.8.2
#endif
#if defined(SSE_X64)
#define SSE_DRAW_C //introduced in v3.8.2
#define SSE_X64_DEBUG
#define SSE_X64_LIBS
#define SSE_X64_LPTR
#define SSE_X64_MISC
#define SSE_X64_STACK
#endif

#endif//SSE_COMPILER


// #define SSE_LEAN_AND_MEAN //TODO

///////////////
// STRUCTURE //
///////////////


#if defined(SSE_STRUCTURE)

//#define SSE_STRUCTURE_INFO // just telling cpp files included in modules
#define SSE_STRUCTURE_DECLA // necessary for SSE build

#endif//structure


//////////////////
// ALL VERSIONS //
//////////////////

#define SSE_LOAD_SAVE_001


//////////
// v3.3 //
//////////
/*  v3.3 can't really be built as it was because Hatari code for keyboard and
    video, used in v3.3, was removed in v3.4 (so that comments don't appear
    in search engines)
*/

#if SSE_VERSION>=330

#if defined(SSE_CPU_PREFETCH)
#define SSE_CPU_PREFETCH_4PIXEL_RASTERS //see SSECpu.cpp
#endif

#if defined(SSE_CPU_INSTR)
#define SSE_CPU_DIV          // divide like Caesar
#define SSE_CPU_MOVE       // move like a superstar
#endif

#if defined(SSE_GUI)
#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SSE_GUI_F12 // F12 starts/stops emulation
#endif
#define SSE_GUI_MOUSE_CAPTURE_OPTION 
#endif

#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_IACK_LATENCY //was SS_MFP_PENDING in v3.3 undef 3.7.0
#define SSE_INT_MFP_RATIO // change the values of CPU & MFP freq!
#define SSE_INT_MFP_TIMER_B 
#if defined(SSE_INT_MFP_RATIO) 
#define SSE_INT_MFP_RATIO_STF // theoretical for STF
#endif
#endif

#if defined(SSE_MMU)
#define SSE_MMU_WRITE // programs in RAM may write in the MMU
#endif

#if defined(SSE_OSD)
#define SSE_OSD_DRIVE_LED
#endif

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
#define SSE_STF_VIDEO_IOR // default value $FF
#define SSE_STF_VBASELO
#define SSE_STF_VERT_OVSCN
#endif

#if defined(SSE_VID_BORDERS)
#define SSE_VID_BPOC // Best Part of the Creation fit display 800 hack
#endif//vid

#endif//3.3.0



//////////
// v3.4 //
//////////


#if SSE_VERSION>=340

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

#if defined(SSE_CPU_EXCEPTION)
#define SSE_CPU_ASSERT_ILLEGAL // assert before trying to execute (not general)
#define SSE_CPU_POST_INC // no post increment if exception (Beyond)
#endif

#if defined(SSE_CPU_POKE) && defined(SSE_SHIFTER)
#define SSE_CPU_CHECK_VIDEO_RAM // 36.15 GEN4 by ULM
#endif //poke

#if defined(SSE_FDC) 
#define SSE_FDC_ACCURATE // "native Steem", slow disk drive
#endif

#if defined(SSE_FDC_ACCURATE)
#define SSE_FDC_CHANGE_COMMAND_DURING_SPINUP // from Hatari
#define SSE_FDC_CHANGE_SECTOR_WHILE_BUSY // from Hatari or Kryoflux
#define SSE_FDC_CHANGE_TRACK_WHILE_BUSY // from Hatari or Kryoflux
#define SSE_FDC_RESTORE
#endif//adat

#if defined(SSE_GUI)
#define SSE_GUI_MSA_CONVERTER // don't prompt if found
#define SSE_GUI_OPTION_PAGE // a new page for all our options
#endif

#if defined(SSE_IKBD)
#define SSE_IKBD_6301  // HD6301 true emu, my pride!
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
#if defined(SSE_INT_JITTER) && defined(SSE_INT_HBL)
#define SSE_INT_JITTER_HBL
#endif
#if defined(SSE_INT_JITTER) && defined(SSE_INT_VBL) && defined(SSE_STF)
#define SSE_INT_JITTER_VBL
#endif
#endif//int

#if defined(SSE_INT_MFP)
#if defined(SSE_INT_MFP_RATIO) 
#define SSE_INT_MFP_RATIO_STE // measured (by Steem Authors) for STE?
#endif
#if defined(SSE_INT_MFP_TIMER_B)
#define SSE_INT_MFP_TIMER_B_AER // earlier trigger (from Hatari)
#endif
#endif//mfp

#if defined(SSE_VID_SDL)
#define SSE_VID_SDL_DEACTIVATE
#endif

#if defined(SSE_SHIFTER)
#define SSE_SHIFTER_SDP // SDP=Shifter draw pointer
#define SSE_SHIFTER_TRICKS  // based on Steem system, extended
#if defined(SSE_SHIFTER_TRICKS)
#define SSE_SHIFTER_0BYTE_LINE
#if defined(SSE_SHIFTER_0BYTE_LINE) 
#define SSE_SHIFTER_0BYTE_LINE_RES_END //No Buddies Land
#define SSE_SHIFTER_0BYTE_LINE_RES_HBL //Beyond/Pax Plax Parallax
#define SSE_SHIFTER_0BYTE_LINE_RES_START //Nostalgic-O/Lemmings
#define SSE_SHIFTER_0BYTE_LINE_SYNC //Forest
#endif
#define SSE_SHIFTER_4BIT_SCROLL //Let's do the Twist again
#define SSE_SHIFTER_60HZ_OVERSCAN //Leavin' Teramis
#define SSE_SHIFTER_LINE_PLUS_2_THRESHOLD //Forest
#define SSE_SHIFTER_MED_OVERSCAN // BPOC
#define SSE_SHIFTER_NON_STOPPING_LINE // Enchanted Land
#define SSE_SHIFTER_PALETTE_BYTE_CHANGE //undef v3.6.3
#define SSE_SHIFTER_STE_MED_HSCROLL // Cool STE
#define SSE_SHIFTER_UNSTABLE_LEFT_OFF // DoLB, Omega, Overdrive/Dragon old hack
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
#define SSE_SOUND_KEYBOARD_CLICK
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
#define SSE_TOS_PATCH106 // a silly bug, a silly hack //undef 370
#endif
#endif//tos

#if defined(SSE_VARIOUS)
#define SSE_VAR_FULLSCREEN_DONT_START // disable run when going fullscreen - option?
//#define SSE_VAR_HIDE_OPTIONS_AT_START // hack before debugging
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

#if defined(SSE_STF) && defined(SSE_INT_VBL)
#define SSE_INT_VBL_STF // a hack but works
#endif

#endif//int

#endif//341



//////////
// v3.5 //
//////////


#if SSE_VERSION>=350

#define SSE_SSE_CONFIG_STRUCT // structure SSEConfig (350+)

#if defined(SSE_FLOPPY)
#if defined(SSE_DISK) && defined(WIN32)
#define SSE_DISK_CAPS        // CAPS support (IPF, CTR disk images) 
#endif
#if defined(SSE_DRIVE)
#define SSE_DRIVE_OBJECT
#endif
#endif

#if defined(SSE_CPU)

#define SSE_CPU_LINE_F // for interrupt depth counter

#if defined(SSE_CPU_EXCEPTION)
#define SSE_CPU_IGNORE_WRITE_B_0 // for Aladin //undef 3.7.1
#define SSE_CPU_SET_DEST_W_TO_0 // for Aladin //undef 3.7.1
#endif

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

#if defined(SSE_DISK_CAPS)
#define SSE_DISK_CAPS_OSD // for the little box at reset - undef v3.6.1
#endif//ipf

#if defined(SSE_DMA)
#define SSE_DMA_OBJECT
//#define SSE_DMA_ADDRESS // enforcing order for write (no use?)
#define SSE_DMA_COUNT_CYCLES
//#define SSE_DMA_DELAY // works but overkill 3.7.0 -> use generic floppy event?
#define SSE_DMA_FDC_ACCESS
#define SSE_DMA_FIFO // first made for CAPS 
#define SSE_DMA_IO //necessary
#define SSE_DMA_READ_STATUS 
#define SSE_DMA_SECTOR_COUNT
#define SSE_DMA_SECTOR_COUNT2
#define SSE_DMA_WRITE_CONTROL
#endif//dma

#if defined(SSE_FDC_ACCURATE) 
#define SSE_FDC_INDEX_PULSE1 // 4ms
#define SSE_FDC_MOTOR_OFF 
#define SSE_FDC_SPIN_UP_STATUS
// #define SSE_DEBUG_FDC_TRACE_STATUS //spell out status register
#ifdef SSE_DRIVE_OBJECT
#define SSE_FDC_VERIFY_AGENDA 
#endif
#endif//fdc_adat

#if defined(SSE_GUI)
#define SSE_GUI_NOTIFY1 //adding some notify during init
#endif

#if defined(SSE_INT_MFP)
#if defined(SSE_INT_MFP_TIMER_B)
#define SSE_INT_MFP_TIMER_B_NO_WOBBLE // BIG Demo Psych-O screen 2
#endif
#define SSE_INT_MFP_TxDR_RESET // they're not reset according to doc
#endif//mfp

#if defined(SSE_MMU)
#define SSE_MMU_NO_CONFUSION  //undef v3.5.2
#ifdef SSE_STF
#define SSE_MMU_WAKE_UP
#endif//stf
#if defined(SSE_MMU_WAKE_UP)
#define SSE_MMU_WU_0_BYTE_LINE
#define SSE_MMU_WU_IOR_HACK // undef 3.5.4
#define SSE_MMU_WU_IOW_HACK // undef 3.5.4
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

#if defined(SSE_SHIFTER_TRICKS)
#define SSE_SHIFTER_END_OF_LINE_CORRECTION // correct +2, -2 lines 
#define SSE_SHIFTER_PALETTE_TIMING //Overscan Demos #6
#define SSE_SHIFTER_STE_HSCROLL_LEFT_OFF //MOLZ/Spiral
#define SSE_SHIFTER_STE_VERTICAL_OVERSCAN //RGBeast
#endif

#if defined(SSE_TOS)// fixes by other people: 
#define SSE_AVTANDIL_FIX_001 // Russian TOS number
#define SSE_MEGAR_FIX_001 // intercept GEM in extended resolution
#endif

#if defined(SSE_VARIOUS)
#define SSE_VAR_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
#define SSE_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES // at final save
#define SSE_VAR_NO_UPDATE // remove all code in relation to updating
#define SSE_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah
#ifdef WIN32
#define SSE_VAR_UNRAR // using unrar.dll, up to date
#endif
#endif//var

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
#define SSE_ACIA_USE_REGISTERS
#endif

#if defined(SSE_CPU)

#define SSE_CPU_FETCH_IO     // fetch like a dog in outer space
#define SSE_CPU_FETCH_IO2

#if defined(SSE_CPU_EXCEPTION)
#define SSE_CPU_PRE_DEC // no "pre" decrement if exception
#define SSE_CPU_TRUE_PC // based on Motorola microcodes!
#endif

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

#if defined(SSE_DISK_PASTI)
#define SSE_DISK_PASTI_ALWAYS_DISPLAY_STX_DISKS
#define SSE_DISK_PASTI_AUTO_SWITCH
#define SSE_DISK_PASTI_ONLY_STX  // experimental! optional
#define SSE_DISK_PASTI_NO_RESET 
#define SSE_DISK_PASTI_ON_WARNING // mention in disk manager title - undef 370
#endif//stx

#if defined(SSE_DMA)
#define SSE_DMA_FIFO_NATIVE
#define SSE_DMA_FIFO_PASTI
#define SSE_DMA_FIFO_READ_ADDRESS // save some bytes...
#define SSE_DMA_FIFO_READ_ADDRESS2 // save 4 bytes more...
#define SSE_STRUCTURE_DMA_INC_ADDRESS
#endif

#if defined(SSE_DRIVE_OBJECT)
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

#if defined(SSE_YM2149)
#define SSE_YM2149_OBJECT
#endif

#endif//floppy

#if defined(SSE_GUI)
#define SSE_GUI_INFOBOX //changed later
#define SSE_GUI_OPTIONS_REFRESH // 6301, STF... up-to-date with snapshot
#endif

#if defined(SSE_IKBD_6301)
#define SSE_IKBD_6301_MOUSE_ADJUST_SPEED // undef 3.7.0
#endif

#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_RATIO_PRECISION // for short timers
#define SSE_INT_MFP_RATIO_STE_AS_STF // change STF + STE (same for both)
#define SSE_INT_MFP_RS232 //one little anti-hang bugfix
#endif

#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_INLINE 
#if defined(SSE_INT_VBL) && defined(SSE_INT_JITTER)
#define SSE_INT_JITTER_VBL_STE//3.5.1,undef 3.6.0
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

#if defined(SSE_SHIFTER_TRICKS) && defined(SSE_HACKS)
#define SSE_SHIFTER_ARMADA_IS_DEAD // no shift contrary to Big Wobble
#define SSE_SHIFTER_TEKILA // in Delirious 4
#endif

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

#if defined(SSE_TIMINGS)
#define SSE_TIMINGS_MS_TO_HBL
#endif

#if defined(SSE_VARIOUS)
#define SSE_VAR_RESIZE // reduce memory set (int->BYTE etc.)
#ifdef WIN32
#define SSE_VAR_CHECK_SNAPSHOT
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
#endif

#if defined(SSE_BLITTER)
#define SSE_BLT_YCOUNT // 0=65536
#endif

#if defined(SSE_CARTRIDGE)
#define SSE_CARTRIDGE_352
#define SSE_CARTRIDGE_64KB_OK
#define SSE_CARTRIDGE_DIAGNOSTIC
#define SSE_CARTRIDGE_NO_CRASH_ON_WRONG_FILE
#define SSE_CARTRIDGE_NO_EXTRA_BYTES_OK
#endif

#if defined(SSE_FLOPPY)
#if defined(SSE_DRIVE_OBJECT)
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

#if defined(SSE_DRIVE_OBJECT)
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
#ifdef SSE_WD1772
#define SSE_FDC_INDEX_PULSE_COUNTER
#define SSE_WD1772_RESET
#endif
#endif//fdc

#endif//flp

#if defined(SSE_GUI)
#define SSE_GUI_ASSOCIATE
#define SSE_GUI_RESET_BUTTON // invert
#define SSE_GUI_OPTION_SLOW_DISK_SSE // undef 3.7.0
#endif

#ifdef SSE_GUI_ASSOCIATE
#define SSE_GUI_ASSOCIATE_IPF
#define SSE_GUI_ASSOCIATE_CU // current user, not root
#define SSE_GUI_MAY_REMOVE_ASSOCIATION
#define SSE_GUI_NO_ASSOCIATE_STC // cartridge, who uses that?
#define SSE_GUI_NO_AUTO_ASSOCIATE_DISK_STS_STC // disk images + STS STC
#define SSE_GUI_NO_AUTO_ASSOCIATE_MISC // other .PRG, .TOS...
#endif

#if defined(SSE_INT_HBL)
#define SSE_INT_HBL_IACK_FIX // from Hatari - BBC52 (works without?)
#endif

#if defined(SSE_INT_MFP)
#undef SSE_INT_MFP_IRQ_DELAY2
#define SSE_INT_MFP_PATCH_TIMER_D // from Hatari, we keep it for performance
#endif

#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_IACK
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
#if defined(SSE_DRIVE_OBJECT)
#undef SSE_DRIVE_EMPTY_VERIFY_LONG //def 3.5.3
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT //GEM
#endif
#endif//flp

#if defined(SSE_GUI)
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

#if defined(SSE_DISK_PASTI)
#define SSE_DISK_PASTI_ONLY_STX_HD
#endif

#if defined(SSE_DMA)
#define SSE_DMA_ADDRESS_EVEN
#endif

#if defined(SSE_DRIVE_OBJECT)
#define SSE_DRIVE_CREATE_ST_DISK_FIX // from Petari
#define SSE_DRIVE_SINGLE_SIDE
#if defined(SSE_DRIVE_SINGLE_SIDE)
#define SSE_DRIVE_SINGLE_SIDE_NAT1
#define SSE_DRIVE_SINGLE_SIDE_PASTI
#endif
#if defined(WIN32) && defined(SSE_SOUND) //TODO Unix
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

#endif//flp

#if defined(SSE_GUI)
#define SSE_GUI_WINDOW_TITLE
#if defined(SSE_GUI_STATUS_STRING)
#define SSE_GUI_STATUS_STRING_VSYNC // V as VSync
#endif
#endif

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

#if defined(SSE_INT_MFP)
#undef SSE_INT_MFP_IRQ_DELAY
#define SSE_INT_MFP_IRQ_DELAY3 // undef v3.6.1
#define SSE_INT_MFP_TIMERS_BASETIME //
#endif

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
#if defined(SSE_SHIFTER_UNSTABLE)
#define SSE_SHIFTER_UNSTABLE_360
#endif
#endif//sft

#if defined(SSE_SOUND)
#define SSE_SOUND_MICROWIRE_WRITE_LATENCY // as documented
#define SSE_SOUND_VOL_LOGARITHMIC // more intuitive setting
#if ! defined(SSE_YM2149_OBJECT) // if sse_floppy undefined
#define SSE_YM2149_OBJECT
#endif
#endif

#if defined(SSE_TOS)
#define SSE_TOS_FILETIME_FIX //from Petari
#define SSE_TOS_NO_INTERCEPT_ON_RTE1 // undef v3.6.1
#endif

#if defined(SSE_UNIX)
#define SSE_UNIX_OPTIONS_SSE_ICON
#define SSE_UNIX_STATIC_VAR_INIT //odd
#endif

#if defined(SSE_VIDEO)
#if defined(WIN32)
#if defined(SSE_VID_VSYNC_WINDOW)
#define SSE_VID_VSYNC_WINDOW_CRASH_FIX1
#define SSE_VID_VSYNC_WINDOW_CRASH_FIX3 //TODO find something better?
#endif
#endif
#endif//vid

#ifdef SSE_YM2149_OBJECT 
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

#if defined(SSE_CPU_DIV)
#define SSE_CPU_DIVS_OVERFLOW
#define SSE_CPU_DIVU_OVERFLOW
#endif

#if defined(SSE_FLOPPY)

#if defined(SSE_DISK_CAPS)
#undef SSE_DISK_CAPS_OSD
#define SSE_DISK_CAPS_CTRAW//2nd type of file recognised by caps v5.0 (?) //3.6.1
#define SSE_DISK_CAPS_CTRAW_REV //3.6.1 manage different rev data
#define SSE_DISK_CAPS_RESUME
#endif

#if defined(SSE_DISK_PASTI)
#define SSE_DISK_PASTI_ON_WARNING2 //refactoring
#endif

#if defined(SSE_DMA)
#define SSE_DMA_TRACK_TRANSFER // debug + possible later use
#endif

#ifdef SSE_DRIVE_OBJECT
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT2 //motor led still on
#define SSE_DRIVE_IPF1 // know image type (not used yet)
#define SSE_DRIVE_MOTOR_ON_IPF//TODO
#if defined(SSE_DRIVE_SOUND)
#define SSE_DRIVE_SOUND_EMPTY // none
#define SSE_DRIVE_SOUND_IPF // fix 
#endif//snd
#define SSE_DRIVE_WRONG_IMAGE_ALERT
#endif//drv

#endif//floppy

#if defined(SSE_GUI)
#define SSE_GUI_INFOBOX0 // enum
#define SSE_GUI_INFOBOX1 // SSE internet sites
#define SSE_GUI_INFOBOX2 // SSE readme + FAQ
#define SSE_GUI_INFOBOX3 // readme text font
#define SSE_GUI_INFOBOX4 // readme 80 col 
#define SSE_GUI_INFOBOX5 // don't take 64K on the stack!
#define SSE_GUI_INFOBOX6 // no cartridge howto
#define SSE_GUI_INFOBOX7 // specific hints
#if defined(SSE_GUI_STATUS_STRING) && defined(SSE_STF)
#define SSE_GUI_STATUS_STRING_FULL_ST_MODEL //undef v380
#endif
#endif

#if defined(SSE_INTERRUPT)

#if defined(SSE_INT_HBL)
#define SSE_INT_HBL_ONE_FUNCTION // remove event_hbl()
#endif

#if defined(SSE_INT_JITTER) && defined(SSE_INT_VBL) && defined(SSE_STF)
#define SSE_INT_JITTER_RESET //undef v370
#endif

#if defined(SSE_INT_MFP)
#undef SSE_INT_MFP_IRQ_DELAY3
#define SSE_INT_MFP_RATIO_HIGH_SPEED
#define SSE_INT_MFP_WRITE_DELAY1//Audio Artistic//undef v370
#endif

#endif//int

#if defined(SSE_STF)
#define SSE_STF_8MHZ // for Panic study! //undef v370
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
#define SSE_VAR_POWERON1 // undef 3.6.2 ;)
#endif//var

#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_416_NO_SHIFT0
#define SSE_VID_BORDERS_416_NO_SHIFT1 // check border on/off
#define SSE_VID_BORDERS_LB_DX1 // check border on/off
#endif

#endif//361


#if SSE_VERSION>=362

#if defined(SSE_FLOPPY) && defined(SSE_DMA)
#define SSE_DMA_FDC_READ_HIGH_BYTE
#endif

#if defined(SSE_VARIOUS)
#undef SSE_VAR_POWERON1
#define SSE_VAR_POWERON2 // undef 3.6.3 :)
#endif

#endif//362


#if SSE_VERSION>=363

#if defined(SSE_FLOPPY) && defined(SSE_DMA)
#undef SSE_DMA_FDC_READ_HIGH_BYTE //def 3.6.2
#endif

#if defined(SSE_MMU)
#define SSE_MMU_WU_VERTICAL_OVERSCAN1 //defaults to WU1
#endif

#if defined(SSE_SHIFTER_TRICKS)
#undef SSE_SHIFTER_PALETTE_BYTE_CHANGE
#if defined(SSE_SHIFTER_0BYTE_LINE) // former switch approach
#endif//0-byte line
#endif//tricks

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

#if defined(SSE_CPU_E_CLOCK)
#if defined(SSE_DEBUG) || defined(SSE_HACKS)
#define SSE_CPU_E_CLOCK_DISPATCHER // "who" wants to sync?
#endif
#if defined(SSE_ACIA)
#undef SSE_ACIA_BUS_JAM_NO_WOBBLE // def 3.4.0
#define SSE_ACIA_BUS_JAM_PRECISE_WOBBLE // option 6301 on
#endif
#endif//e-clock

#if defined(SSE_CPU_ROUNDING)
#define SSE_CPU_ROUNDING_ADD_L_DN
#define SSE_CPU_ROUNDING_ADDA_L_DN
//#define SSE_CPU_ROUNDING_CMPI_BW
#define SSE_CPU_ROUNDING_CMPI_L
#endif//rounding

#endif//cpu

#if defined(SSE_FLOPPY)

#if defined(SSE_DISK)
#define SSE_DISK_IMAGETYPE
#endif

#if defined(SSE_DISK_CAPS)
#define SSE_DISK_CAPS_CTRAW_1ST_LOCK
#define SSE_DISK_CAPS_CTRAW_NO_UNLOCK
#endif

#if defined(SSE_DMA)
#undef SSE_DMA_COUNT_CYCLES
#endif

#if defined(SSE_DRIVE_OBJECT)
#undef SSE_DRIVE_COMPUTE_BOOT_CHECKSUM //TODO boiler control
#endif

#if defined(SSE_YM2149_OBJECT)
#define SSE_YM2149A // selected drive, side as variables
#define SSE_YM2149B // adapt drive motor status to FDC STR at each change
#endif

#endif//floppy

#if defined(SSE_IKBD_6301)
#define SSE_IKBD_6301_VBL
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
#endif//hbl
#endif//int

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

#if _MSC_VER >= 1500 
#define SSE_VS2008_WARNING_370
#endif

#if defined(SSE_BLITTER) && defined(SSE_HACKS)
#define SSE_BLT_370 // bad hack for Relapse
#endif

#if defined(SSE_COMPILER)
#define SSE_COMPILER_370_INLINE
#endif

#if defined(SSE_CPU)

#define SSE_CPU_DATABUS
#define SSE_CPU_DEST_IS_REGISTER //just new macro
#if defined(SSE_CPU_E_CLOCK)
#define SSE_CPU_E_CLOCK_370
#endif
#define SSE_CPU_FETCH_80000A//setpc
#define SSE_CPU_FETCH_80000B//fetchword
#define SSE_CPU_FETCH_80000C//prefetch IRC
#if defined(SSE_GUI_STATUS_STRING)
#define SSE_CPU_HALT // no reset, just stop
#endif
#define SSE_CPU_TRACE_REFACTOR//debug-only?
#define SSE_CPU_UNSTOP2//not twice

#if defined(SSE_CPU_EXCEPTION)
#undef SSE_CPU_IGNORE_RW_4MB // F-29 hangs on 4MB machines, nothing to fix

#if defined(SSE_CPU_ASSERT_ILLEGAL)
#define SSE_CPU_ASSERT_ILLEGAL_370
#endif
#define SSE_CPU_BUS_ERROR_ADDRESS //high byte
#define SSE_CPU_BUS_ERROR_ADDRESS2 //high byte
#define SSE_CPU_EXCEPTION_FCO
#define SSE_CPU_TRUE_PC2 // JMP, JSR
#endif//exc

#if defined(SSE_CPU_FETCH_IO)
#define SSE_CPU_FETCH_IO3//clarify
#endif

#if defined(SSE_CPU_INSTR)
#define SSE_CPU_ABCD
#define SSE_CPU_DIV_CC //carry clear: always
#define SSE_CPU_MOVEM_BUS_ACCESS_TIMING
#define SSE_CPU_NBCD //TODO?
#define SSE_CPU_SBCD //to test
#endif//instr

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

#define SSE_FLOPPY_EVENT
#define SSE_FLOPPY_EVENT2

#if defined(SSE_DISK)
#define SSE_DISK1//struct
#if defined(SSE_DISK_IMAGETYPE)//3.6.4
#define SSE_DISK_IMAGETYPE1 // replace "TCaps::IsIpf()"
#define SSE_DISK_IMAGETYPE2
#endif
#if defined(SSE_WD1772) //&&...
#define SSE_DISK_STW
#endif
#if defined(SSE_DISK_STW)
#define SSE_DISK_STW_DISK_MANAGER //new context option
#define SSE_DISK_STW_MFM // bytes are MFM encoded on the disk
#endif
#if defined(WIN32) && defined(SSE_WD1772)
#define SSE_DISK_GHOST
#endif
#if defined(SSE_DISK_GHOST)
#define SSE_DISK_GHOST_FAKE_FORMAT
#define SSE_DISK_GHOST_SECTOR // commands $A#/$8#
#define SSE_DISK_GHOST_MULTIPLE_SECTORS // commands $B#/$9#
#define SSE_DISK_GHOST_SECTOR_STX1 // in pasti
#endif
#endif//disk

#if defined(SSE_DISK_PASTI)
#define SSE_DISK_PASTI_ONLY_STX_OPTION3 // move that option SSE -> disk manager
#undef SSE_DISK_PASTI_ON_WARNING//?
#undef SSE_DISK_PASTI_ON_WARNING2//?
#endif

#if defined(SSE_DMA)
#define SSE_DMA_COUNT_CYCLES // again
#define SSE_DMA_DOUBLE_FIFO
#define SSE_DMA_DRQ
//#define SSE_DMA_DRQ_RND 
#define SSE_DMA_FIFO_NATIVE2 //bugfix International Sports Challenge-ICS fast
#define SSE_DMA_FIFO_READ_TRACK //TODO
#endif

#if defined(SSE_DRIVE_OBJECT)
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

#if defined(SSE_FDC_ACCURATE) 
#define SSE_FDC_FORCE_INTERRUPT_D8
#define SSE_FDC_IDFIELD_IGNORE_SIDE
#define SSE_FDC_MULTIPLE_SECTORS
//#define SSE_FDC_RESTORE1//undef to remove bug
#endif

#if defined(SSE_WD1772)
#define SSE_WD1772_CRC 
#define SSE_WD1772_F7_ESCAPE
#define SSE_WD1772_IDFIELD 
#define SSE_WD1772_MFM 
#define SSE_WD1772_PHASE
#define SSE_WD1772_REG2 // DSR, ByteCount
#define SSE_WD1772_REG2_B // StatusType
#define SSE_WD1772_LINES // made switch in v383
#endif

#if defined(SSE_YM2149)
#define SSE_YM2149C // turn on/off motor in drives
#endif

#endif//floppy

#if defined(SSE_GUI)
#define SSE_GUI_CUSTOM_WINDOW_TITLE
#define SSE_GUI_DISK_MANAGER//was missing in v3.7.2
#undef SSE_GUI_OPTION_SLOW_DISK_SSE
#define SSE_GUI_PATCH_SCROLLBAR
#define SSE_GUI_SNAPSHOT_INI
#ifdef WIN32
#define SSE_GUI_CLIPBOARD_TEXT
#endif
#ifndef IN_RC
#if defined(SSE_GUI_CLIPBOARD_TEXT) && defined(SSE_GUI_DISK_MANAGER_LONG_NAMES1)
#define SSE_GUI_DISK_MANAGER_NAME_CLIPBOARD
#endif
#endif
#if defined(SSE_GUI_DISK_MANAGER)
#define SSE_GUI_DISK_MANAGER_DOUBLE_CLK_GO_UP //habit with some file managers
#define SSE_GUI_DISK_MANAGER_GHOST
#define SSE_GUI_DISK_MANAGER_INSERT_DISKB
#define SSE_GUI_DISK_MANAGER_INSERT_DISKB_LS//load/save option
#define SSE_GUI_DISK_MANAGER_LONG_NAMES1
#define SSE_GUI_DISK_MANAGER_NO_DISABLE_B_MENU // click on icon
//#define SSE_GUI_DISK_MANAGER_RGT_CLK_HD//on DM icon 
#define SSE_GUI_DISK_MANAGER_RGT_CLK_HD2//on HD icon
#endif
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
#endif
#endif//SSE_INT_MFP

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
#endif

#if defined(SSE_SHIFTER)
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY//My Socks are Weapons
#ifndef IN_RC
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY_370 //we change the way later
#endif
#endif
#define SSE_SHIFTER_HIRES_OVERSCAN//3.7.0
#define SSE_SHIFTER_PALETTE_NOISE2
#define SSE_SHIFTER_STE_DE_MED_RES // bugfix
#define SSE_SHIFTER_STE_READ_SDP_HSCROLL1 // undef 373
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
#define SSE_TOS_PRG_AUTORUN// Atari PRG + TOS file direct support
#endif
#endif

#if defined(SSE_VARIOUS)
#undef SSE_VAR_FULLSCREEN_DONT_START
#define SSE_VAR_RESET_SAME_DISK//test (with .PRG support)
#define SSE_VAR_RESIZE_370
#define SSE_VAR_STEALTH2 //bugfix 3.7
#endif//various

#if defined(SSE_VIDEO)
#define SSE_VID_EXT_MON
#if defined(WIN32) 
#undef SSE_VID_RECORD_AVI//no, Fraps or other 3rd party programs will do a fantastic job
#endif
#define SSE_VID_BLOCK_WINDOW_SIZE // option can't change size of window
#define SSE_VID_LOCK_ASPET_RATIO // 
#define SSE_VID_UTIL // miscellaneous functions
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_GUARD_EM // don't mix large borders & extended res
#define SSE_VID_BORDERS_GUARD_R2 // don't mix large borders & hi res
//#define SSE_VID_BORDERS_NO_LOAD_SETTING // start at 384 x 270
#endif
#define SSE_VID_CHECK_POINTERS
#if defined(WIN32) 
#if defined(_MSC_VER) && (_MSC_VER >= 1500) //VS2008
#ifndef SSE_NO_D3D //use only DD2
#define SSE_VID_DD7 // DirectDraw7 instead of 2
#define SSE_VID_DIRECT3D 
#endif
#endif//vs2008
#if defined(BCC_BUILD) // Borland C++ 5.5 (after some efforts!)
#ifndef SSE_NO_D3D //use only DD2
#define SSE_VID_DD7      
#define SSE_VID_DIRECT3D  // could be trouble on some machines
#endif
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
#define SSE_VID_D3D_STRETCH_ASPECT_RATIO_OPTION
#endif
#define SSE_VID_D3D_STRETCH_FORCE // only stretch: better for list modes
#endif//d3d
#endif//vid

#ifdef SSE_YM2149_OBJECT
#undef SSE_YM2149_ENV_FIX1 //my mistake
#define SSE_YM2149_DELAY_RENDERING // so that we use table also for envelope
#define SSE_YM2149_QUANTIZE1 //undef 3.8.2
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

#if _MSC_VER >= 1500 
#define SSE_VS2008_WARNING_371
#endif

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

#if defined(SSE_DRIVE_OBJECT)
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
#define SSE_WD1772_WEAK_BITS //undef 3.7.2
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

#if defined(SSE_VARIOUS)
#define SSE_VAR_UNRAR2 //ok with RAR5
#endif
#ifdef SSE_YM2149_OBJECT 
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
#endif

#if defined(SSE_GUI)
#define SSE_GUI_ASSOCIATE_HFE // they're generally not zipped...
#define SSE_GUI_DISK_MANAGER_INSERT_DISKB_REMOVE //works?
#define SSE_GUI_INFOBOX18 //open links in new instance, IE
#ifdef SSE_DISK_EXT
#define SSE_GUI_STATUS_STRING_DISK_TYPE // A:MSA B:STW
#endif
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
#define SSE_TOS_GEMDOS_RESTRICT_TOS2 // undef 3.8.3
#endif

#if defined(SSE_VARIOUS)
#if defined(WIN32) && defined(VC_BUILD) // works with VC6, VS2008 not BCC
#define SSE_VAR_ARCHIVEACCESS // 7z support
#define SSE_VAR_ARCHIVEACCESS2 // bz2 (with modded dll), gz, tar, arj
#ifdef SSE_X64 //no unzip32, no unzipd64.dll...
#define SSE_VAR_ARCHIVEACCESS3 // zip managed by ArchiveAccess.dll by default - no, because of MSA Converter TODO
#define SSE_VAR_ARCHIVEACCESS4 // remove code for unzipd32.dll - what if archiveaccess fails!
#endif
#endif
#define SSE_VAR_MAIN_LOOP1 // protect from crash with try/catch
#define SSE_VAR_NO_UPDATE_372
#define SSE_VAR_RESIZE_372
#endif//var

#if defined(SSE_VID_D3D)
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
#define SSE_BLT_371 // temp fix Drone
#endif

#ifdef SSE_CPU
#define SSE_CPU_ABUS_ACCESS // argh, was forgotten
#undef SSE_CPU_PREFETCH_TIMING_MOVEM_HACK // grr forgot this too!! (Sea of Colour)
#endif

#if defined(SSE_FLOPPY)
#define SSE_DISK_PASTI_AUTO_SWITCH2
#ifdef SSE_DISK
#define SSE_DISK_MSA
#define SSE_DISK_ST
#endif
#ifdef SSE_DISK_SCP
#define SSE_DISK_SCP373 // War Heli
#endif
#ifdef SSE_DRIVE_OBJECT
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
#define SSE_TOS_GEMDOS_RESTRICT_TOS3 // undef 3.8.3
#ifdef SSE_DISK
#define SSE_TOS_PRG_AUTORUN2 //better behaviour
#endif
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

#if defined(SSE_HARDDISK) && defined(SSE_DMA)
#define SSE_ACSI //3.8.0 new feature
#endif

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
#define SSE_ACSI_MODESELECT
#define SSE_ACSI_OPTION
#define SSE_ACSI_REQUEST_SENSE
#define SSE_ACSI_TIMING // ADAT -> slower (risky?)
#ifdef WIN32
#define SSE_ACSI_HDMAN // browser
#define SSE_ACSI_ICON
#endif
#endif//acsi

#if defined(SSE_BLITTER)
#define SSE_BLT_380
#endif//blt

#if defined(SSE_COMPILER)
#define SSE_COMPILER_380
#endif

#ifdef SSE_CPU
#define SSE_CPU_512MHZ
#define SSE_CPU_1GHZ 
#define SSE_CPU_2GHZ//after this I hope they'll leave me alone for a while
//#define SSE_CPU_3GHZ   //note: 2147483647 = max int
//#define SSE_CPU_4GHZ
#if defined(SSE_CPU_ASSERT_ILLEGAL)
#define SSE_CPU_ASSERT_ILLEGAL_380 // mini-refactoring
#endif
#if defined(SSE_CPU_E_CLOCK)
#define SSE_CPU_E_CLOCK_380
#endif
#if defined(SSE_CPU_PREFETCH_CLASS)
#define SSE_CPU_PREFETCH_CLASS_380
#endif
#if defined(SSE_CPU_ROUNDING)
#define SSE_CPU_ROUNDING_ANDI2
#define SSE_CPU_ROUNDING_BUS // big change, much overhead, little effect 
#define SSE_CPU_ROUNDING_BUS2 // blitter problem
#define SSE_CPU_ROUNDING_BUS3 // more shifter regs
#define SSE_CPU_ROUNDING_BUS3B // ...but not for STF
#define SSE_CPU_ROUNDING_CHK
#define SSE_CPU_ROUNDING_DBCC2
#define SSE_CPU_ROUNDING_IMMEDIATE_TO_SR //dubious... future bug reports?
#define SSE_CPU_ROUNDING_MOVEM_380 //final?
#define SSE_CPU_ROUNDING_UNLK
#define SSE_CPU_ROUNDING_LINK
#define SSE_CPU_ROUNDING_RTR
#define SSE_CPU_ROUNDING_RTS
#define SSE_CPU_ROUNDING_TRAP
#define SSE_CPU_ROUNDING_TRAPV
#endif//round
#define SSE_CPU_STOP_380 // little refactoring
#define SSE_CPU_STOP_DELAY // from Hatari
#undef SSE_CPU_UNSTOP2//placement is paramount!
#define SSE_CPU_TIMINGS_REFACTOR_FETCH // moving timings to fetching functions
#define SSE_CPU_TIMINGS_REFACTOR_PUSH // count timing in push macros
#define SSE_CPU_TRACE_TIMING
#define SSE_CPU_TRACE_TIMING_EXT //EXT for "extended" (see SSECpu.h)
#define SSE_CPU_TRACE_LINE_A_F
#endif//cpu

#if defined(SSE_DISK)
#define SSE_DISK_380 // update side
#endif
#if defined(SSE_DMA)
#define SSE_DMA_FIFO_NATIVE3 //bugfix Sabotage (the game)
#endif

#if defined(SSE_GLUE)

#define SSE_GLUE_FRAME_TIMINGS  // big timing change
#define SSE_GLUE_THRESHOLDS     // computing thresholds only when changing option
#if defined(SSE_GLUE_FRAME_TIMINGS) && defined(SSE_GLUE_THRESHOLDS)
#define SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE // I regret this switch now... doubles code!
#endif

#if defined(SSE_GLUE_FRAME_TIMINGS)
#define SSE_GLUE_FRAME_TIMINGS_HBL // routines of last scanline
#define SSE_GLUE_FRAME_TIMINGS_INIT // not indispensable
#define SSE_GLUE_FRAME_TIMINGS_B // eliminate old var
#undef SSE_SHIFTER_FIX_LINE508_CONFUSION // hack unnecessary
#undef SSE_TIMINGS_FRAME_ADJUSTMENT // hack unnecessary
#undef SSE_INT_VBL_STF // hack unnecessary
#endif//SSE_GLUE_FRAME_TIMINGS

#ifndef IN_RC
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE)
#define SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1 // the boring stuff
#define SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE2 //misc
#define SSE_GLUE_60HZ_OVERSCAN2 // # lines in bottom
#undef SSE_SHIFTER_LEFT_OFF_60HZ//forget it
#undef SSE_STF_VERT_OVSCN
#endif//SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE
#endif//rc?

#endif//glue

#ifdef SSE_GUI
#define SSE_GUI_380//tmp name
#define SSE_GUI_CONFIG_FILE // new option to L/S ini files
#define SSE_GUI_RESET_BUTTON2
#define SSE_GUI_SNAPSHOT_INI2 //bugfix
#if defined(SSE_GUI_CONFIG_FILE)
#define SSE_GUI_CONFIG_FILE2 // TOS: relative path
#endif
#if defined(SSE_GUI_DISK_MANAGER)
#define SSE_GUI_DISK_MANAGER_RGT_CLK_HD3 //when changing on/off in hdm
#define SSE_GUI_DISK_MANAGER_HD_SELECTED //stay pushed if on
#endif
#if defined(SSE_GUI_STATUS_STRING) && (defined(SSE_CPU_HALT) || defined(SSE_CPU_TRACE_REFACTOR))
#undef SSE_GUI_STATUS_STRING_FULL_ST_MODEL // or more complicated
#define SSE_GUI_STATUS_STRING_ICONS
#endif
#endif//gui

#ifdef SSE_IKBD_6301
////#undef SSE_IKBD_6301_SET_TDRE //SSE_IKBD_6301_373 was defined anyway
#define SSE_IKBD_6301_380
#define SSE_IKBD_6301_380B
#define SSE_IKBD_6301_EVENT
#endif//6301

#if defined(SSE_INTERRUPT)
#define SSE_INT_ROUNDING
#define SSE_INT_CHECK_BEFORE_PREFETCH
#endif

#if defined(SSE_INT_HBL)
#undef SSE_INT_HBL_E_CLOCK_HACK  // made a patch instead
#define SSE_INT_HBL_380 
#endif

#if defined(SSE_INT_MFP)
#define SSE_INT_MFP_READ_DELAY2 //timer B
#ifdef SSE_CPU //IackCycle is, logically, in CPU
#define SSE_INT_MFP_REFACTOR2 //simpler way for IACK and Spurious
#endif
#if defined(SSE_INT_MFP_REFACTOR2)
#define SSE_INT_MFP_REFACTOR2A//debug...
//#define SSE_INT_MFP_REFACTOR2A1//define for fewer spurious...
#define SSE_INT_MFP_REFACTOR2A2//define for fewer spurious...
#define SSE_INT_MFP_REFACTOR2B//"improve"...
#undef SSE_INT_MFP_IACK_LATENCY4 // adios "skip timer" hack
#define SSE_INT_MFP_EVENT_WRITE 
#if defined(SSE_INT_MFP_EVENT_WRITE)
#define SSE_INT_MFP_EVENT_WRITE_SPURIOUS
#else
#define SSE_INT_MFP_IS_DELAY
#endif
#endif//ref2
#define SSE_INT_MFP_REFACTOR3 //enums 
#undef SSE_INT_MFP_SPURIOUS_372
#undef SSE_INT_MFP_TIMER_B_AER // refactor
#define SSE_INT_MFP_TIMER_B_AER2 // refactor
//#define SSE_INT_MFP_TIMER_B_PULSE //TODO
#ifdef SSE_VIDEO_CHIPSET
#define SSE_INT_MFP_TIMER_B_SHIFTER_TRICKS // timer B should be updated
#endif
#define SSE_INT_MFP_TIMERS_WOBBLE // trade off with timer set delay
#define SSE_INT_MFP_TIMERS_STARTING_DELAY //
#endif

#if defined(SSE_INT_VBL)
#define SSE_INT_VBL_380
#endif

#if defined(SSE_MMU)
#ifndef IN_RC
#define SSE_MMU_RELOAD_SDP_380
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE)
#if defined(SSE_MMU_WU_DL)
#define SSE_MMU_WU_STE_380
#endif
#define SSE_MOVE_SHIFTER_CONCEPTS_TO_MMU1
#endif
#endif//rc?
#endif

#if defined(SSE_OSD) && defined(SSE_DRIVE_OBJECT)
#define SSE_OSD_DRIVE_INFO_EXT // STX, MSA... 
#endif

#ifdef SSE_SHIFTER
#define SSE_SHIFTER_HSCROLL_380
#if defined(SSE_SHIFTER_HSCROLL_380)
#define SSE_MMU_WRITE_SDP_380
#endif
#if defined(SSE_GLUE_FRAME_TIMINGS)
#define SSE_SHIFTER_HIRES_RASTER // edgy stuff in monochrome mode (Time Slices)
#endif
#ifndef IN_RC
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY_380//better test, compatible with GLUE refactoring
#endif
#endif
#if defined(SSE_SHIFTER_UNSTABLE)
#define SSE_SHIFTER_UNSTABLE_380 // demo Closure
#define SSE_SHIFTER_UNSTABLE_380_LINE_PLUS_2 // demo NPG_WOM
#endif
#define SSE_SHIFTER_60HZ_LINE // compensate fix in 'read SDP'
#define SSE_SHIFTER_KRYOS//hack undef 3.8.1
#define SSE_SHIFTER_STE_MED_HSCROLL2 // Desktop Central
#endif//sft

#if defined(SSE_SOUND)
#define SSE_SOUND_DMA_380
#define SSE_SOUND_DMA_380B //hack
#define SSE_SOUND_KEYBOARD_CLICK2 // persistent
#define SSE_SOUND_MICROWIRE_MIXMODE2 //hack
#endif

#ifdef SSE_STF
#define SSE_STE_4MB // new default memory
#endif

#ifdef SSE_TOS
//#define SSE_TOS_GEMDOS_DRVBIT // sooner //really necessary?
#undef SSE_TOS_CHECK_VERSION // MagicOs v 6+
#define SSE_STF_MATCH_TOS3 // + default country
#endif

#ifdef SSE_VARIOUS
#define SSE_VAR_ARG_STFM // start as STF (unless overruled)
#ifdef SSE_TOS
#endif
#define SSE_VAR_RESIZE_380
#define SSE_VAR_REWRITE_380
#endif

#ifdef SSE_VIDEO
#define SSE_VID_380
#define SSE_VID_D3D_380
#define SSE_VID_DISABLE_AUTOBORDER
#define SSE_VID_DISABLE_AUTOBORDER2 // move options around
#define SSE_VID_DISABLE_AUTOBORDER3 // different for monochrome
#define SSE_VID_STRETCH_ASPECT_RATIO 
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_LIMIT_TO_245
#ifdef SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1
#define SSE_VID_BORDERS_LINE_PLUS_20
#endif
#define SSE_VID_BORDERS_416_NO_SHIFT_380
#endif
#endif

#ifdef SSE_YM2149_OBJECT
#define SSE_YM2149_NO_JAM_IF_NOT_RW
#define SSE_YM2149_ENV_DEPHASING //undef v3.8.1
#endif

#endif//380


#if SSE_VERSION>=381

#if defined(SSE_ACSI)
#define SSE_ACSI_ICON2 // more space for the ACSI icon in initial disk manager
#endif

#if defined(SSE_BLITTER)
#define SSE_BLT_381
#endif

#if defined(SSE_CPU)
#define SSE_CPU_CHECK_VIDEO_RAM_381
#define SSE_CPU_E_CLOCK5
#if defined(SSE_CPU_HALT)
#define SSE_CPU_HALT2 // can reset
#define SSE_CPU_HALT2B // warm reset runs emulation
#endif
#if defined(SSE_CPU_ROUNDING_BUS)
#define SSE_CPU_ROUNDING_BUS3C
#define SSE_CPU_ROUNDING_BUS_STOP
#endif
#define SSE_CPU_TPEND // internal bit latch
#if defined(SSE_CPU_TRACE_REFACTOR)
#define SSE_CPU_TPEND2 //use it
#endif
#endif

#if defined(SSE_GLUE)
#define SSE_GLUE_381
#define SSE_GLUE_EXT_SYNC
#endif

#if defined(SSE_MMU)
#if defined(SSE_GLUE_FRAME_TIMINGS)
#define SSE_GLUE_REFACTOR_OVERSCAN_EXTRA // separate shifter_draw_pointer / video counter
#define SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2 // eliminate old code and variables
#endif
#define SSE_MMU_LINEWID_TIMING
#if defined(SSE_MMU_WU_DL)
#define SSE_MMU_WU_STE_381
#endif
#endif//mmu

#if defined(SSE_SHIFTER)
#define SSE_SHIFTER_60HZ_LINE2
#define SSE_SHIFTER_HSCROLL_381
#define SSE_SHIFTER_MED_OVERSCAN_SHIFT_381
#define SSE_SHIFTER_SCHNUSDIE_381
#if defined(SSE_SHIFTER_UNSTABLE)
#define SSE_SHIFTER_UNSTABLE_381
#undef SSE_SHIFTER_KRYOS
#endif
#endif//sft

#if defined(SSE_SOUND)
#define SSE_SOUND_KEYBOARD_CLICK2B
#endif

#ifdef SSE_TOS
#define SSE_TOS_GEMDOS_EM_381A // done the day before release
#define SSE_TOS_GEMDOS_EM_381B // so is it rock-solid?
#endif

#if defined(SSE_VIDEO)
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_413_381
#undef SSE_VID_BORDERS_416_NO_SHIFT4
#define SSE_VID_BORDERS_416_NO_SHIFT_381 // change left_border and right_border
#define SSE_VID_BORDERS_BIGTOP_381
#define SSE_VID_BORDERS_LINE_PLUS_20_381 // left border 12 pixels
#endif//borders
#define SSE_VID_SCANLINES_INTERPOLATED_381
#endif

#if defined(SSE_YM2149_OBJECT)
#undef SSE_YM2149_ENV_DEPHASING // don't understand, it sounds like we shouldn't?
#endif

#endif//381


#if SSE_VERSION>=382

#if _MSC_VER >= 1500 
#define SSE_VS2008_WARNING_382
#endif

#if defined(SSE_COMPILER)
#define SSE_COMPILER_382
#if defined(VC_BUILD) && _MSC_VER>=1500 //VS2008+
#define SSE_VC_INTRINSICS_382 // only some uses
#endif
#endif

#if defined(SSE_ACSI)
#define SSE_ACSI_RELOAD_TOS
#endif

#if defined(SSE_CPU)
#define SSE_CPU_E_CLOCK_382
#define SSE_CPU_TIMINGS_NO_INLINE_382 //undef 383
#define SSE_CPU_TPEND_382
#endif//cpu

#if defined(SSE_GLUE)
#define SSE_GLUE_382
#endif                                                             

#if defined(SSE_GUI)
#define SSE_GUI_MOUSE_VM_FRIENDLY //VM for virtual machine
#endif

#if defined(SSE_INT_HBL) && defined(SSE_STF) && defined(SSE_MMU)
#define SSE_INT_HBL_E_CLOCK_HACK_382 //here we go again
#endif

#if defined(SSE_OSD) 
#undef SSE_OSD_FORCE_REDRAW_AT_STOP//was bugged anyway
#define SSE_OSD_SHOW_TIME // measure time you waste
#endif

#if defined(SSE_SHIFTER)
#define SSE_SHIFTER_382
#ifndef IN_RC
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY_382 // My Socks Are Weapons, other way
#undef SSE_SHIFTER_HIRES_COLOUR_DISPLAY_370
#undef SSE_SHIFTER_HIRES_COLOUR_DISPLAY_380 // incl. in 370
#endif
#endif
#undef SSE_SHIFTER_PANIC//it was cool, but fake emu, it's bugged now
#define SSE_SHIFTER_UNSTABLE_382
#endif//sft

#if defined(SSE_SOUND)
#undef SSE_SOUND_NO_EXTRA_PER_VBL // switch wasn't operating <382 anyway
#endif

#ifdef SSE_TOS
#define SSE_TOS_GEMDOS_EM_382
#define SSE_AVTANDIL_FIX_002 // we have new TOS flags
#endif//tos

#if defined(SSE_TIMINGS) && defined(SSE_GLUE_FRAME_TIMINGS)
#define SSE_TIMINGS_SNAPSHOT_CORRECTION
#endif

#if defined(SSE_VARIOUS)
#define SSE_VAR_OPT_382
#define SSE_VAR_RESIZE_382
#endif

#if defined(SSE_VIDEO)
#define SSE_VID_DD_FS_32BIT
#define SSE_VID_FS_382
#define SSE_VID_FS_GUI_OPTION
#ifdef SSE_VID_DIRECT3D
#define SSE_VID_D3D_382
#define SSE_VID_D3D_3BUFFER
#define SSE_VID_D3D_INTERPOLATED_SCANLINES
#define SSE_VID_D3D_WINDOW
#if defined(SSE_NO_DD)
#define SSE_VID_D3D_ONLY // D3D has smaller footprint than DD
#endif
#define SSE_VID_BORDERS_416_NO_SHIFT_382
#endif
#endif//vid

#if defined(SSE_YM2149_OBJECT)
#undef SSE_YM2149_QUANTIZE1
#undef SSE_YM2149_QUANTIZE2
#define SSE_YM2149_QUANTIZE_382
#endif

#endif//382


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
#define SSE_DEBUG_ASSERT // 3.8.2
#if defined(SSE_BOILER)

#define SSE_BOILER_FRAME_REPORT //3.3, now boiler-only because of mask control
#if defined(SSE_BOILER_FRAME_REPORT)
#define SSE_BOILER_FRAME_REPORT_ON_STOP // each time we stop emulation
#endif
#define SSE_BOILER_REPORT_SCAN_Y_ON_CLICK
#define SSE_BOILER_REPORT_SDP_ON_CLICK // yeah!



#ifdef ACIA
#define SSE_BOILER_ACIA_373 // fix annoying bug 6301 mode
#endif
#define SSE_BOILER_AUTO_FLUSH_TRACE// v3.7.1
#define SSE_BOILER_BLIT_IN_HISTORY // v3.7.1
#define SSE_BOILER_BLIT_WHEN_STOPPED // v3.7.1
#define SSE_BOILER_BLAZING_STEP_OVER 
#ifdef SSE_IKBD_6301
#define SSE_BOILER_BROWSER_6301
#endif
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
#define SSE_BOILER_VIDEO_CONTROL
#endif//fake io
//#define SSE_BOILER_FLUSH_TRACE
#define SSE_BOILER_FRAME_REPORT_MASK // for interactive control in boiler
#define SSE_BOILER_LOGSECTIONS1 //v3.7.1
#define SSE_BOILER_MENTION_READONLY_BROWSERS
#define SSE_BOILER_MOD_REGS // big letters, no =
#define SSE_BOILER_MOD_VBASE
#define SSE_BOILER_MOD_VBASE2 //move it
#ifdef SSE_CPU
#define SSE_BOILER_MONITOR_372 // v3.7.2 to clarify code
#endif
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
#define SSE_OSD_DRIVE_INFO_EXT // v3.7.1
#endif


#define SSE_DEBUG_RESET

#ifdef SSE_SHIFTER
#if !defined(SSE_DEBUG_TRACE_IDE) && !defined(SSE_UNIX)
#define SSE_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif

//#define SSE_DEBUG_SDP_TRACE 

#endif//shifter

//#define SSE_VIDEO_IOR_TRACE // specific, not "log"
//#define SSE_VIDEO_IOW_TRACE // specific, not "log"

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

//v3.8.0
#if defined(SSE_BOILER)
#define SSE_BOILER_BROWSER_EXCEPTION_HANDLERS
#define SSE_BOILER_BROWSER_IRQ_HANDLERS
#define SSE_BOILER_CLOCK
#define SSE_BOILER_HISTORY_VECS
#define SSE_BOILER_HISTORY_TIMING
#define SSE_BOILER_OPTION_SAVE_FRAME_REPORT
#define SSE_BOILER_SHOW_ECLOCK
#define SSE_BOILER_SHOW_PENDING
#if defined(SSE_SHIFTER_UNSTABLE)
#define SSE_BOILER_SHOW_PRELOAD
#endif
#define SSE_BOILER_TRACE_EVENTS
#define SSE_BOILER_XBIOS2
#endif

//v3.8.1
#if defined(SSE_BOILER)
#define SSE_BOILER_BLIT_IN_HISTORY2
#define SSE_BOILER_BLIT_WHEN_TRACING
#define SSE_BOILER_GO_AUTOSAVE_FRAME

#define SSE_BOILER_IOLIST_381
#endif

//3.8.2
#define SSE_DEBUG_382
#define SSE_DEBUG_D3D
//#define SSE_DEBUG_OSD_SCROLLER_EVERYTIME
#ifdef SSE_BOILER
#define SSE_BOILER_BLIT_IN_HISTORY3
#if defined(SSE_BLT_BLIT_MODE_CYCLES) 
#define SSE_BOILER_BLIT_WHEN_TRACING2
#endif
#define SSE_BOILER_HIRES_DONT_REDRAW
//#define SSE_YM2149_QUANTIZE_TRACE
#endif//SSE_BOILER
#define SSE_DEBUG_START_STOP_INFO3
#ifdef _DEBUG
#undef SSE_VAR_MAIN_LOOP1 // let exceptions trigger in VS IDE
#endif

#else//SSE_DEBUG

//3.8.2
#define DEBUG_FACILITIES_IN_RELEASE

#if defined(DEBUG_FACILITIES_IN_RELEASE)
//#define ASSERT_FOR_RELEASE //careful with this (performance)
#define OSD_FOR_RELEASE // time
#define TRACE_FOR_RELEASE // TRACE2: the new breed!
#endif

#if defined(ASSERT_FOR_RELEASE)
#define SSE_DEBUG_ASSERT
#endif

#if defined(OSD_FOR_RELEASE)
#define SSE_OSD_DEBUG_MESSAGE
#define SSE_DEBUG_RESET
#endif

#ifdef TRACE_FOR_RELEASE
#define SSE_DEBUG_TRACE
#define SSE_DEBUG_TRACE_FILE
#define SSE_DEBUG_START_STOP_INFO
#define SSE_DEBUG_START_STOP_INFO2
#define SSE_DEBUG_START_STOP_INFO3
#define SSE_BOILER_AUTO_FLUSH_TRACE
#endif

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

#if defined(SSE_BETA) //next version

#if _MSC_VER >= 1500 
#define SSE_VS2008_383
#define SSE_VS2008_WARNING_383
#endif
#if defined(VC_BUILD) && _MSC_VER>=1500 //VS2008+
#define SSE_VC_INTRINSICS_383
#define SSE_VC_INTRINSICS_383A // some cpu sr checks //useless like 383E?
#define SSE_VC_INTRINSICS_383B // popcount 
#define SSE_VC_INTRINSICS_383C // byteswap
#define SSE_VC_INTRINSICS_383D // instruction time
#define SSE_VC_INTRINSICS_383E // set/clear some sr bits => more object code? => bad idea?
#define SSE_VC_INTRINSICS_383F // avoid shifting mask in MOVEM //i'm sure it's a bad idea too!
#endif

#ifdef SSE_BOILER
#define SSE_BOILER_383
#endif

#if defined(SSE_CPU_INLINE)
#define SSE_CPU_INLINE_SET_DEST_TO_ADDR
#endif

#define SSE_DISK_CAPS_383
#define SSE_GUI_383
#define SSE_VAR_OPT_383
#define SSE_VAR_RESIZE_383

// Exception management...
//#define SSE_M68K_EXCEPTION_TRY_CATCH //works but too slow, especially if _DEBUG
#define SSE_VAR_MAIN_LOOP2 //2KB on optimised exe but will catch and report everything (?)
#if _MSC_VER >= 1500 && !defined(_DEBUG)
#define SSE_VAR_MAIN_LOOP3 //VC only (vs2008)?
#endif
#if defined(SSE_VAR_MAIN_LOOP3)
#undef SSE_VAR_MAIN_LOOP1 //is emu-only
#endif
//#undef SSE_CPU_DIVS_OVERFLOW_PC //temp for tests...

#define SSE_VID_GAMMA
#define SSE_WD1772_383

#endif//beta

#ifdef SSE_BETA // long term
//#define SSE_IKBD_6301_NOT_OPTIONAL
//#define SSE_INT_MFP_RATIO_STE3 // = STF
#endif

#if defined(SSE_BETA_BUGFIX)

#undef SSE_CPU_TIMINGS_NO_INLINE_382 // it's inlined anyway
#define SSE_DISK_GHOST_SECTOR_383
#define SSE_STF_MATCH_TOS_383 // to keep autoselect T104 for HD
#define SSE_TOS_GEMDOS_EM_383 
#undef SSE_TOS_GEMDOS_RESTRICT_TOS2 //HD/TOS check
#undef SSE_TOS_GEMDOS_RESTRICT_TOS3 //INI option
#define SSE_TOS_GEMDOS_RESTRICT_TOS4 // STF T206 OK!
#define SSE_TOS_SNAPSHOT_AUTOSELECT_383 // correct country
#define SSE_SHIFTER_UNSTABLE_383 //bugfixes
#define SSE_VAR_CHECK_SNAPSHOT2

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
