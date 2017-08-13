// for v3.9.3
#pragma once // VC guard
#ifndef SSE_H // BCC guard
#define SSE_H

/*
Steem Steven Seagal Edition (SSE)
---------------------------------

This is based on the source code for Steem R63 as released by Steem authors,
Ant & Russ Hayward.
SVN code repository is at:
 http://sourceforge.net/projects/steemsse/
Homepage:
 http://ataristeven.exxoshost.co.uk/

To enjoy the new features, you must define STEVEN_SEAGAL in the 
project/makefile.

SSE_DEBUG, if needed, should also be defined in the project/makefile.
It has an effect on both the boiler and the VS debug build.

SSE_DD for the DirectDraw build.

SSE.h is supposed to mainly be a collection of compiling switches (defines).
It should include nothing and can be included everywhere.

My inane comments outside of defined blocks generally are marked by 'SS:'
They don't mean that I did something cool, only that I comment the source.

I removed nothing from the original source code or comments.
*/

/*
Important (note to self)

Release: not SSE_BETA
All SSE_TEST_ON must go
Version for snapshot (in LoadSave.h) + Windows properties (rc\resource.rc)
Rebuild so that dates are correct

Beta: not SSE_PRIVATE_BUILD
*/

#if defined(VC_BUILD) // so that v3.2+ compiles in Visual C++ (no SSE switch)
#define COMPILER_VC6 // Visual Studio 6
#if _MSC_VER >= 1500
#define SSE_VS2008 // Visual Studio 2008
#endif
#endif
#define NO_RAR_SUPPORT // don't use rarlib (SSE build supports unrar.dll)

#if defined(STEVEN_SEAGAL) 

#define SSE_BUILD
#define SSE_COMPILER  //  warnings, errors... 

#define SSE_BETA //title, OSD, plus some testing - new features
#define SSE_BETA_BUGFIX // beta for just bugfixes

#if defined(SSE_BETA) || defined(SSE_BETA_BUGFIX)
//#define SSE_PRIVATE_BUILD // my "beta" option
#endif

//#define SSE_LEAN_AND_MEAN //TODO


//////////////
// COMPILER //
//////////////


#if defined(SSE_COMPILER)

//#define SSE_COMPILER_INCLUDED_CPP // just telling cpp files included in modules


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
#define SSE_X64_390 // bad casts
#define SSE_X64_390B // ellipses (,...)
#define SSE_X64_392
#endif

#if _MSC_VER >= 1500 
#define SSE_VS2008_WARNING
#define SSE_VS2008_WARNING_370
#define SSE_VS2008_WARNING_371
#define SSE_VS2008_390
#define SSE_VS2008_WARNING_390
#define SSE_VS2008_WARNING_390B
#define SSE_VS2008_WARNING_392
#endif

#define SSE_COMPILER_380

#if _MSC_VER >= 1500 
#define SSE_VS2008_WARNING_382
#endif

#if defined(SSE_COMPILER)
#define SSE_COMPILER_WARNING // all compilers?
#define SSE_COMPILER_382
#if defined(VC_BUILD) && _MSC_VER>=1500 //VS2008+
#define SSE_VC_INTRINSICS_382 // only some uses
#endif
#endif

#define SSE_COMPILER_STRUCT_391

#endif//SSE_COMPILER

#define SSE_DISK_EXT   // Extensions (refactoring), available for all versions


//////////////////
// BIG SWITCHES // 
//////////////////

// Normally you can disable one of these and it should still compile and run
// (this is regularly broken)

#define SSE_BLITTER    // spelled BLiTTER by those in the known!
#define SSE_CPU        // MC68000 microprocessor
#define SSE_DONGLE     // special adapters (including protection dongles)
#define SSE_FLOPPY     // DMA, FDC, Pasti, etc
#define SSE_GUI        // Graphic User Interface
#define SSE_HACKS      // an option for dubious fixes
#define SSE_HARDDISK   // GEMDOS improvements + ACSI feature
#define SSE_INTERRUPT  // HBL, VBL, MFP
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

#if defined(SSE_FLOPPY)
#define SSE_DISK       // Disk images
#if defined(WIN32) && !defined(SSE_X64)
#define SSE_DISK_PASTI // Improvements in Pasti support (STX disk images)
#endif
#define SSE_DRIVE      // SF314 floppy disk drives
#define SSE_DMA        // Custom Direct Memory Access chip (disk)
#define SSE_FDC        // "native Steem" (fdc.cpp)
#define SSE_WD1772     // WD1772 floppy disk controller: object
#if defined(WIN32) && defined(SSE_SOUND) //TODO Unix
#define SSE_DRIVE_SOUND // heavily requested, delivered!//3.6.0
#endif
#endif

#if defined(SSE_GUI)
#define SSE_GUI_WINDOW_TITLE
#define SSE_GUI_DISK_MANAGER
#ifdef WIN32
#define SSE_GUI_ASSOCIATE
#define SSE_GUI_CONFIG_FILE // new option to L/S ini files
#define SSE_GUI_STATUS_BAR // "status bar" in the icon bar
#endif
#endif

#if defined(SSE_INTERRUPT)
#define SSE_INT_HBL    // Horizontal Blank interrupt (IPL 2)
#define SSE_INT_MFP    // MC68901 Multi-Function Peripheral Chip (IPL 6)
#define SSE_INT_VBL    // Vertical Blank interrupt (IPL 4)
#endif

#if defined(SSE_KEYBOARD)
#define SSE_ACIA       // MC6850 Asynchronous Communications Interface Adapter
#define SSE_IKBD       // HD6301V1 IKBD (keyboard, mouse, joystick controller)
#define SSE_JOYSTICK
#endif

#if defined(SSE_ROM)
#define SSE_CARTRIDGE  // ROM Cartridge slot
#define SSE_TOS        // The Operating System
#endif

#if defined(SSE_VIDEO_CHIPSET) 
// all or nothing, hence the chipset switch; in STE GLUE and MMU are one chip
#define SSE_GLUE       // General Logic Unit
#define SSE_MMU        // Memory Manager Unit (of the ST, no paging)
#define SSE_SHIFTER    // Video Shifter
#define SSE_SHIFTER_TRICKS  // Largely speaking
#endif


////////////////
// INDIVIDUAL //
////////////////

#if defined(SSE_ACIA)

#define SSE_ACIA_NO_RESET_PIN // don't reset on warm reset
#define SSE_ACIA_TDR_COPY_DELAY // effect on SR
#define SSE_ACIA_MIDI_TIMING1 //check
#define SSE_ACIA_TDR_COPY_DELAY2 // effect on byte flow
#define SSE_ACIA_380
#define SSE_ACIA_EVENT

#endif//acia


#if defined(SSE_BLITTER)

#define SSE_BLT_BLIT_MODE_CYCLES // #cycles in BLIT Mode in NOPs, not M68K cycles
#define SSE_BLT_BLIT_MODE_INTERRUPT //?
#define SSE_BLT_YCOUNT // 0=65536
#define SSE_BLT_380
#define SSE_BLT_381
#define SSE_BLT_390 //bugfix
#define SSE_BLT_390B //ambitious + overhead: CPU can work when blitter has bus
#define SSE_BLT_392
#define SSE_BLT_BUS_ARBITRATION
#define SSE_BLT_COPY_LOOP
#define SSE_BLT_RESTART //trick
//#define SSE_BLT_RESTART2 // too heavy 
#define SSE_BLT_MAIN_LOOP

#endif//blt


#if defined(SSE_CARTRIDGE)

#define SSE_CARTRIDGE_64KB_OK
#define SSE_CARTRIDGE_DIAGNOSTIC
#define SSE_CARTRIDGE_NO_CRASH_ON_WRONG_FILE
#define SSE_CARTRIDGE_NO_EXTRA_BYTES_OK
#define SSE_CARTRIDGE_BAT //fun!
#define SSE_CARTRIDGE_BAT2
#define SSE_CARTRIDGE_FREEZE
#define SSE_CARTRIDGE_REPLAY16
#define SSE_CARTRIDGE_TRANSPARENT //check for bugs

#endif//cartridge


#if defined(SSE_CPU)

#define SSE_CPU_256MHZ
#define SSE_CPU_512MHZ
#define SSE_CPU_1GHZ 
#define SSE_CPU_2GHZ//after this I hope they'll leave me alone for a while
//#define SSE_CPU_3GHZ   //note: 2147483647 = max int
//#define SSE_CPU_4GHZ
#define SSE_CPU_BUS_ERROR_TIMING
#define SSE_CPU_DATABUS//preliminary
#define SSE_CPU_DIVS_OVERFLOW_PC //long-standing bug that could crash Steem
#define SSE_CPU_E_CLOCK
#define SSE_CPU_FETCH_80000  // no crash when RAM is missing
#define SSE_CPU_FETCH_IO     // fetch like a dog in outer space
#define SSE_CPU_HALT
#define SSE_CPU_HISPEED_392
#define SSE_CPU_LINE_F // for interrupt depth counter
#define SSE_CPU_MFP_RATIO // change the values of CPU & MFP freq!
#define SSE_CPU_SPLIT_RL
#define SSE_CPU_TRACE_LINE_A_F
#define SSE_CPU_TRACE_REFACTOR
#define SSE_CPU_TRUE_PC
#define SSE_CPU_392 // refactoring
#define SSE_CPU_392B // add exception states
#define SSE_CPU_392B2// no blit if exception state
#define SSE_CPU_392C // "thinking" cycles...
#define SSE_CPU_392D // bus access always 4 cycles

#endif//cpu


#if defined(SSE_CPU_MFP_RATIO) 

#define SSE_CPU_MFP_RATIO_HIGH_SPEED
#define SSE_CPU_MFP_RATIO_OPTION // user can fine tune CPU clock
//#define SSE_CPU_MFP_RATIO_STE // different for STE, hack

#endif


#if defined(SSE_DISK)

#define SSE_DISK_11_SECTORS
#define SSE_DISK_READ_ADDRESS_TIMING
#define SSE_DISK_RW_SECTOR_TIMING // start of sector
#define SSE_DISK_380 // update side
#define SSE_DISK_392 // refactoring
#define SSE_DISK_392B // load disk options before disks...
#define SSE_DISK_MFM0 // C++ style

#endif//disk


#if defined(SSE_DISK_PASTI)

#define SSE_DISK_PASTI_AUTO_SWITCH

#endif//stx


#if defined(SSE_DMA)

#define SSE_DMA_OBJECT
#if defined(SSE_DMA_OBJECT)
#define SSE_DMA_FDC_ACCESS
#define SSE_DMA_FIFO
#define SSE_DMA_IO //necessary
#define SSE_DMA_READ_STATUS 
#define SSE_DMA_SECTOR_COUNT
#define SSE_DMA_SECTOR_COUNT2
#define SSE_DMA_WRITE_CONTROL
#define SSE_DMA_FIFO_FDC
#define SSE_DMA_FIFO_PASTI
#define SSE_DMA_FIFO_FDC_READ_ADDRESS // save some bytes...
#define SSE_DMA_ADDRESS_EVEN
#define SSE_DMA_TRACK_TRANSFER
#define SSE_DMA_DOUBLE_FIFO
#define SSE_DMA_DRQ
//#define SSE_DMA_DRQ_RND 
#define SSE_DMA_FIFO_READ_TRACK //TODO
#endif

#endif//dma


#if defined(SSE_DONGLE)

#define SSE_DONGLE_PORT // all dongles grouped in "virtual" port
#define SSE_DONGLE_BAT2
#define SSE_DONGLE_CRICKET
#define SSE_DONGLE_PROSOUND // Wings of Death, Lethal Xcess  STF
#define SSE_DONGLE_LEADERBOARD
#define SSE_DONGLE_MULTIFACE
#define SSE_DONGLE_MUSIC_MASTER
#define SSE_DONGLE_URC

#endif//dongle


#if defined(SSE_DRIVE)

#define SSE_DRIVE_OBJECT
#if defined(SSE_DRIVE_OBJECT)
#define SSE_DRIVE_CREATE_ST_DISK_FIX // from Petari
#define SSE_DRIVE_SINGLE_SIDE


#define SSE_DRIVE_SWITCH_OFF_MOTOR //hack
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT2 //motor led still on
#define SSE_DRIVE_MOTOR_ON_IPF
#define SSE_DRIVE_EMPTY_VERIFY_TIME_OUT //GEM
#define SSE_DRIVE_INDEX_PULSE
#define SSE_DRIVE_INDEX_STEP
#define SSE_DRIVE_IP_HACK
#define SSE_DRIVE_MEDIACHANGE // for CAPSImg, Pasti
#define SSE_DRIVE_REM_HACKS 
#if defined(SSE_DRIVE_REM_HACKS)
#define SSE_DRIVE_REM_HACKS2//gaps
#define SSE_DISK_RW_SECTOR_TIMING3 //test v3.7.0 use ID
#define SSE_DISK_RW_SECTOR_TIMING4 //remove more hacks
#endif
#if defined(SSE_DRIVE_SINGLE_SIDE)
#define SSE_DRIVE_SINGLE_SIDE_CAPS //using the new feature
#define SSE_DRIVE_SINGLE_SIDE_NOPASTI // too involved
#define SSE_DRIVE_SINGLE_SIDE_RND//random data instead
#endif
#define SSE_DRIVE_INIT_373 //bugfix, don't play with 300 rpm (STW)
#endif

#endif//drive


#if defined(SSE_DRIVE_SOUND)

#define SSE_GUI_OPTIONS_DRIVE_SOUND
#define SSE_DRIVE_SOUND_EPSON // current samples=Epson
#define SSE_DRIVE_SOUND_EMPTY // none
#define SSE_DRIVE_SOUND_SEEK_OPTION// option buzz/steps for seek
#define SSE_DRIVE_SOUND_391

#endif//drive sound


#if defined(SSE_FDC) 

#define SSE_FDC_ACCURATE // "native Steem", slow disk drive

#endif//fdc


#if defined(SSE_GLUE)

#define SSE_GLUE_EXT_SYNC
#define SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2 // eliminate old code and variables
#if defined(SSE_TIMINGS) 
#define SSE_TIMINGS_SNAPSHOT_CORRECTION
#endif
#if defined(SSE_HACKS)
#define SSE_GLUE_CLOSURE
#endif
//#define SSE_GLUE_392A //correct start & end cycles //wrong?
#define SSE_GLUE_392B //refactor hscroll extra fetch
#define SSE_GLUE_392C //sundry
#define SSE_GLUE_392E 

#endif//glue


#if defined(SSE_GUI)

//#if !(defined(_DEBUG) && defined(VC_BUILD)) // it's Windows 'break' key
#define SSE_GUI_F12 // F12 starts/stops emulation
//#endif
#define SSE_GUI_MOUSE_CAPTURE_OPTION 
#define SSE_GUI_MSA_CONVERTER // don't prompt if found
#define SSE_GUI_OPTION_PAGE // a new page for all our options
#define SSE_GUI_NOTIFY1 //adding some notify during init
#define SSE_GUI_INFOBOX 
#define SSE_GUI_OPTIONS_REFRESH // 6301, STF... up-to-date with snapshot
#define SSE_GUI_RESET_BUTTON // invert
#define SSE_GUI_OPTIONS_SSE_ICON_VERSION
#define SSE_GUI_CUSTOM_WINDOW_TITLE
#define SSE_GUI_DISK_MANAGER//was missing in v3.7.2
#define SSE_GUI_PATCH_SCROLLBAR
#define SSE_GUI_SNAPSHOT_INI
#ifdef WIN32
#define SSE_GUI_CLIPBOARD_TEXT
#endif
#define SSE_GUI_OPTIONS_DISABLE_DISPLAY_SIZE_IF_NO_BORDER
#define SSE_GUI_OPTION_DISPLAY_CHANGE_TEXT // normal is small + scanlines
#define SSE_GUI_OPTIONS_DISPLAY_SIZE
#define SSE_GUI_OPTIONS_STF
#define SSE_GUI_OPTIONS_WU
#define SSE_GUI_OPTIONS_SOUND
#define SSE_GUI_380//tmp name
#define SSE_GUI_RESET_BUTTON2
#define SSE_GUI_MOUSE_VM_FRIENDLY //VM for virtual machine
#define SSE_GUI_390
#define SSE_GUI_390B
#define SSE_GUI_392

#endif//gui


#if defined(SSE_GUI_CONFIG_FILE)

#define SSE_GUI_CONFIG_FILE2 // TOS: relative path

#endif//cfg


#if defined(SSE_GUI_ASSOCIATE)

#define SSE_GUI_ASSOCIATE_CU // current user, not root
#define SSE_GUI_MAY_REMOVE_ASSOCIATION
#define SSE_GUI_NO_ASSOCIATE_STC // cartridge, who uses that?
#define SSE_GUI_NO_AUTO_ASSOCIATE_DISK_STS_STC // disk images + STS STC
#define SSE_GUI_NO_AUTO_ASSOCIATE_MISC // other .PRG, .TOS...

#endif//associate


#if defined(SSE_GUI_DISK_MANAGER)

#define SSE_GUI_DM_WRONG_IMAGE_ALERT
#define SSE_GUI_DM_DOUBLE_CLK_GO_UP //habit with some file managers
#define SSE_GUI_DM_DRIVE_OPTIONS
#define SSE_GUI_DM_GHOST
#define SSE_GUI_DM_INSERT_DISK_B
#define SSE_GUI_DM_INSERT_DISK_B_LS//load/save option
#define SSE_GUI_DM_LONG_NAMES1
#ifndef IN_RC
#if defined(SSE_GUI_CLIPBOARD_TEXT) && defined(SSE_GUI_DM_LONG_NAMES1)
#define SSE_GUI_DM_NAME_CLIPBOARD
#endif
#endif
#define SSE_GUI_DM_NO_DISABLE_B_MENU // click on icon
#define SSE_GUI_DM_RGT_CLK_HD2//on HD icon
#define SSE_GUI_DM_INSERT_DISK_B_REMOVE //works?
#define SSE_GUI_DM_RGT_CLK_HD3 //when changing on/off in hdm
#define SSE_GUI_DM_HD_SELECTED //stay pushed if on
#define SSE_GUI_DM_REGROUP // it's getting cluttered
#define SSE_GUI_DM_REGROUP2 // context
#define SSE_GUI_DM_SHOW_EXT 

#endif


#if defined(SSE_GUI_INFOBOX)

#define SSE_GUI_INFOBOX_80COL
#define SSE_GUI_INFOBOX_GREETINGS
#define SSE_GUI_INFOBOX_LINKS
#define SSE_GUI_INFOBOX_MALLOC
#define SSE_GUI_INFOBOX_NO_CART // no cartridge howto
#define SSE_GUI_INFOBOX_NO_DISK // no disk howto
#define SSE_GUI_INFOBOX_NO_THANKS
#define SSE_GUI_INFOBOX_CLIPBOARD

#endif


#if defined(SSE_GUI_STATUS_BAR)

#define SSE_GUI_STATUS_BAR_6301
#define SSE_GUI_STATUS_BAR_ADAT
#define SSE_GUI_STATUS_BAR_DISK_NAME
#define SSE_GUI_STATUS_BAR_DISK_NAME_OPTION
#define SSE_GUI_STATUS_BAR_HACKS
#define SSE_GUI_STATUS_BAR_VSYNC // V as VSync
#define SSE_GUI_STATUS_BAR_68901
#define SSE_GUI_STATUS_BAR_HD
#define SSE_GUI_STATUS_BAR_HALT
#define SSE_GUI_STATUS_BAR_THRESHOLD //not if window too small
#define SSE_GUI_STATUS_BAR_DISK_TYPE // A:MSA B:STW
#define SSE_GUI_STATUS_BAR_HISPEED
#define SSE_GUI_STATUS_BAR_SINGLE_SIDE
#ifdef SSE_CPU//unfortunate dependency
//win32
#define SSE_GUI_STATUS_BAR_ICONS
#endif
#define SSE_GUI_STATUS_BAR_392
#endif//status bar


#if defined(SSE_IKBD)

#define SSE_IKBD_6301  // HD6301 true emu, my pride!

#endif//ikbd


#if defined(SSE_IKBD_6301)
//TODO too many switches
#define SSE_IKBD_6301_ADJUST_CYCLES // stay in sync (check with clock)
#define SSE_IKBD_6301_CHECK_IREG_RO // some registers are read-only
#define SSE_IKBD_6301_CHECK_IREG_WO // some registers are write-only
#define SSE_IKBD_6301_DISABLE_BREAKS // to save 64k RAM (we still consume 64k)
#define SSE_IKBD_6301_DISABLE_CALLSTACK // to save 3k on the PC stack
#define SSE_IKBD_6301_RUN_IRQ_TO_END // hack around Sim6xxx's working
#define SSE_IKBD_6301_SET_TDRE
#define SSE_IKBD_6301_TIMER_FIX // not sure there was a problem
#define SSE_IKBD_6301_MOUSE_MASK // Jumping Jackson auto
#define SSE_IKBD_6301_VBL
#define SSE_IKBD_6301_MOUSE_ADJUST_SPEED
//#define SSE_IKBD_6301_MOUSE_MASK2//hack but game is bugged
#define SSE_IKBD_6301_MOUSE_MASK3 // reset
#define SSE_IKBD_6301_STUCK_KEYS // reset
//#define SSE_IKBD_6301_ROM_KEYTABLE //spare an array
#define SSE_IKBD_6301_373 
#define SSE_IKBD_6301_MINIRAM // save close to 60k, at last
#define SSE_IKBD_6301_380
#define SSE_IKBD_6301_380B
#define SSE_IKBD_6301_390

#endif//6301


#if defined(SSE_INT_HBL)

#define SSE_INT_HBL_INLINE

#endif


#if defined(SSE_INT_MFP)

#define SSE_INT_MFP_INLINE //necessary
#define SSE_INT_MFP_JAM_AFTER_READ // bus jam after read
#define SSE_INT_MFP_OPTION //performance/precision
#define SSE_INT_MFP_OBJECT // struct
#define SSE_INT_MFP_READ_DELAY_392
#define SSE_INT_MFP_TIMERS
#define SSE_INT_MFP_TIMER_B
#define SSE_INT_MFP_TxDR_RESET // they're not reset according to doc
#define SSE_INT_MFP_RS232 //one little anti-hang bugfix
#define SSE_INT_MFP_392

#endif//mfp

#if defined(SSE_INT_MFP_OBJECT)

#define SSE_INT_MFP_GPIP_TO_IRQ_DELAY // only for GPIP interrupts
#define SSE_INT_MFP_IACK
#define SSE_INT_MFP_IRQ_TIMING //tracking it more precisely
#define SSE_INT_MFP_LATCH_DELAY
#define SSE_INT_MFP_PRESCALE
#define SSE_INT_MFP_SPURIOUS
#define SSE_INT_MFP_TIMER_CHECK

#endif

#if defined(SSE_INT_MFP_TIMERS)

#define SSE_INT_MFP_CHECKTIMEOUT_ON_STOP
#define SSE_INT_MFP_RATIO_PRECISION // for short timers
#define SSE_INT_MFP_TIMERS_BASETIME//MFD
#define SSE_INT_MFP_TIMERS_RATIO1 //unimportant//MFD
#define SSE_INT_MFP_TIMERS_STARTING_DELAY
//#define SSE_INT_MFP_TIMERS_RUN_IF_DISABLED //load!
#if defined(SSE_INT_MFP_OBJECT)
#define SSE_INT_MFP_TIMERS_WOBBLE // trade off with timer set delay
#define SSE_INT_MFP_TIMERS_WOBBLE_390
#endif
#endif

#if defined(SSE_INT_MFP_TIMER_B)

#define SSE_INT_MFP_TIMER_B_SUNNY
#define SSE_INT_MFP_TIMER_B_WOBBLE2 // 2 cycles instead of 4
#if defined(SSE_INT_MFP_OBJECT)
#define SSE_INT_MFP_TIMER_B_392 // refactoring
#define SSE_INT_MFP_TIMER_B_AER // from Hatari
#endif

#endif


#if defined(SSE_INT_VBL)

#define SSE_INT_VBL_INLINE 
#define SSE_INT_VBL_IACK

#endif


#if defined(SSE_JOYSTICK)

#define SSE_JOYSTICK_DELETE_KEY
#define SSE_JOYSTICK_JUMP_BUTTON
#define SSE_JOYSTICK_JOYPAD

#endif


#if defined(SSE_MMU)

#if defined(WIN32)
#define SSE_MMU_256K // Atari 260 ST
#define SSE_MMU_2560K // some STE with 2MB memory upgrade?
#define SSE_MMU_RELOAD_SDP_380
#endif
#define SSE_MMU_LINEWID_TIMING
#define SSE_MMU_RAM_TEST
#define SSE_MMU_RAM_WRITE_CONF // programs in RAM may write in the MMU
#define SSE_MMU_WU // wake-up states
#define SSE_MMU_WU_PALETTE_STE // render +1 cycle (pixel) in state 2
#define SSE_MMU_392

#endif//mmu


#if defined(SSE_MMU_RAM_TEST)

#define SSE_MMU_RAM_TEST1 // change test emulation
#define SSE_MMU_RAM_TEST3 // remove himem=0 hack

#endif

#if defined(SSE_MMU_392) && !defined(SSE_LE)
#define SSE_MMU_MONSTER_ALT_RAM // HW hack for ST
#endif

#if defined(SSE_MMU_MONSTER_ALT_RAM)
#define SSE_MMU_MONSTER_ALT_RAM_IO // $FFFE00
#define SSE_MMU_MONSTER_ALT_RAM_IO2 // supervisor test
#endif



#if defined(SSE_OSD)

#if defined(SSE_FLOPPY)
#define SSE_OSD_DRIVE_LED
#define SSE_OSD_DRIVE_INFO // cool!
#define SSE_GUI_OPTIONS_DRIVE_INFO // ...here
#endif
#define SSE_OSD_LOGO // "Steem SSE" instead of "Steem 3.2"
#define SSE_OSD_SCROLLER_CONTROL
//#define SSE_OSD_SCROLLER_DISK_IMAGE // doubles with status bar
#define SSE_OSD_SHOW_TIME // measure time you waste

#endif//osd


#if defined(SSE_OSD_DRIVE_LED)

#define SSE_OSD_DRIVE_LED3 // remove useless variable
#define SSE_OSD_DRIVE_LED_HD

#endif


#if defined(SSE_SHIFTER)

#define SSE_SHIFTER_HIRES_COLOUR_DISPLAY//My Socks are Weapons
#define SSE_SHIFTER_LINE_MINUS_2_DONT_FETCH //BIG Demo #2
#define SSE_SHIFTER_REMOVE_USELESS_VAR //3.6.1
///#define SSE_SHIFTER_RENDER_SYNC_CHANGES//don't until debug
#define SSE_SHIFTER_PALETTE_TIMING //Overscan Demos #6
#define SSE_SHIFTER_STE_MED_HSCROLL // Cool STE
#define SSE_SHIFTER_HSCROLL
#define SSE_SHIFTER_390 //bugfixes

#endif//shifter


#if defined(SSE_SHIFTER_TRICKS) // subset of VIDEO_CHIPSET

#define SSE_GLUE_VERT_OVERSCAN
#define SSE_GLUE_50HZ_OVERSCAN // vertical
#define SSE_GLUE_60HZ_OVERSCAN //Leavin' Teramis
#define SSE_GLUE_VERT_OVERSCAN_390
#define SSE_GLUE_LINE_PLUS_26
#define SSE_GLUE_LINE_PLUS_20 // 224 byte scanline STE only
#define SSE_GLUE_LINE_PLUS_2
#define SSE_GLUE_LINE_PLUS_4
#define SSE_GLUE_LINE_PLUS_6
#define SSE_GLUE_0BYTE_LINE
#define SSE_GLUE_BLACK_LINE
#define SSE_GLUE_LINE_MINUS_106
#define SSE_SHIFTER_4BIT_SCROLL //Let's do the Twist again
#define SSE_GLUE_LINE_MINUS_2
#define SSE_GLUE_LINE_PLUS_44
#define SSE_SHIFTER_STABILISER
#define SSE_SHIFTER_MED_OVERSCAN // BPOC
#define SSE_GLUE_NON_STOPPING_LINE // Enchanted Land
#define SSE_SHIFTER_LINE_MINUS_106_BLACK // loSTE screens
#define SSE_GLUE_RIGHT_OFF_BY_SHIFT_MODE //beeshift0
#define SSE_SHIFTER_STE_HI_HSCROLL
#define SSE_GLUE_HIRES_OVERSCAN
#define SSE_SHIFTER_382
#define SSE_SHIFTER_END_OF_LINE_CORRECTION // correct +2, -2 lines 
#define SSE_SHIFTER_STE_HSCROLL_LEFT_OFF // (not used)
#define SSE_SHIFTER_HIRES_RASTER // edgy stuff in monochrome mode (Time Slices)
#define SSE_GLUE_CHECK_OVERSCAN_AT_SYNC_CHANGE
#if defined(SSE_HACKS)
#define SSE_SHIFTER_DOLB1 //again a hack... TODO
#define SSE_SHIFTER_DOLB_SHIFT1 // based on "unstable overscan"
#define SSE_SHIFTER_MED_OVERSCAN_SHIFT // No Cooper/greetings
#endif//hacks

#endif//tricks


#if defined(SSE_SOUND)

#define SSE_SOUND_CAN_CHANGE_DRIVER 
#define SSE_SOUND_CHANGE_TIME_METHOD_DELAY //detail
#define SOUND_DISABLE_INTERNAL_SPEAKER
#define SSE_SOUND_DMA
#define SSE_SOUND_DMA_390E
#define SSE_SOUND_FILTERS
#define SSE_SOUND_INLINE // macro->inline, easier for my tests, but hard to do
#define SSE_SOUND_INLINE2 
#define SSE_SOUND_MICROWIRE // volume, balance, bass & treble, primitive DSP
#define SSE_SOUND_MICROWIRE_392 //use sound_freq, not dma_sound_freq
#define SSE_SOUND_MOVE_ZERO
#undef SSE_SOUND_MOVE_ZERO // it only made it louder vs DMA...
#define SSE_SOUND_OPT1 
#define SSE_SOUND_RECOMMEND_OPTIONS
#define SSE_SOUND_RECORD_391B // don't change source_p!//refactor?
#define SSE_SOUND_VOL_LOGARITHMIC // more intuitive setting
#ifdef UNIX
#define SSE_RTAUDIO_LARGER_BUFFER //simplistic attempt
#endif
#if ! defined(SSE_YM2149_OBJECT) // if sse_floppy undefined
#define SSE_YM2149_OBJECT
#endif

#endif//snd


#if defined(SSE_SOUND_DMA)

#define SSE_SOUND_DMA_INSANE //fix
#define SSE_SOUND_DMA_LIGHT //hack

#endif


#if defined(SSE_SOUND_FILTERS)

#define SSE_SOUND_FILTER_HATARI
#define SSE_SOUND_FEWER_FILTERS

#endif


#if defined(SSE_SOUND_MICROWIRE)

#define SSE_GUI_OPTIONS_MICROWIRE
#define SSE_SOUND_MICROWIRE_MASK1 //bugfix
#define SSE_SOUND_MICROWIRE_MASK2 //incorrect doc (?)
#define SSE_SOUND_MICROWIRE_MIXMODE
#define SSE_SOUND_MICROWIRE_MIXMODE2 //hack
#define SSE_SOUND_MICROWIRE_TREBLE // trouble?
#define SSE_SOUND_MICROWIRE_WRITE_LATENCY
//#define SSE_SOUND_MICROWIRE_VOLUME  // trouble
#define SSE_SOUND_MICROWIRE_VOLUME_SLOW

#endif


#if defined(SSE_STF)

#define SSE_STF_STE_AUTORAM
#define SSE_STF_BLITTER
#define SSE_STF_DMA
#define SSE_STF_MMU
#define SSE_STF_PADDLES
#define SSE_STF_PAL
#define SSE_STF_VIDEO_IOR // default value $FF
#define SSE_STF_MEGASTF // blitter in STF (could be useful?) + 4MB!
#define SSE_STF_MATCH_TOS_390 // to keep autoselect T104 for HD

#endif//stf

#if defined(SSE_STF_MEGASTF)
#define SSE_STF_MEGASTF_CLOCK
#endif


#if defined(SSE_TIMINGS)

#define SSE_TIMINGS_MS_TO_HBL
#define SSE_TIMING_MULTIPLIER
#define SSE_TIMING_NO_IO_W_DELAY // refactoring, see note in cpu_sse.cpp
#if defined(SSE_TIMING_MULTIPLIER)
#define SSE_TIMING_MULTIPLIER_392 // refactor MFP prescale boost
#define SSE_TIMING_MULTIPLIER_392B
#endif
#if defined(SSE_X64)
#define SSE_TIMINGS_CPUTIMER64
#endif
#if defined(SSE_TIMINGS_CPUTIMER64)
#define SSE_TIMINGS_CPUTIMER64_B // (temp?) consequences for emudetect
#define SSE_TIMINGS_CPUTIMER64_C // consequences for e-clock
#define SSE_VAR_SNAPSHOTS_INCOMPATIBLE
#endif

#endif//timing-misc


#if defined(SSE_TOS)

#define SSE_AVTANDIL_FIX_001 // Russian TOS number
#define SSE_MEGAR_FIX_001 // intercept GEM in extended resolution
#define SSE_TOS_FILETIME_FIX //from Petari
#define SSE_TOS_GEMDOS_NOINLINE
#define SSE_TOS_GEMDOS_PEXEC6 //ReDMCSB 100% in TOS104
#define SSE_TOS_KEYBOARD_CLICK // click click... click click click...
#define SSE_TOS_GEMDOS_VAR1 //various unimportant fixes 
#define SSE_TOS_NO_INTERCEPT_ON_RTE2 // ReDMCSB 50% in TOS102
#ifdef WIN32
#if defined(SSE_DISK)//see note in floppy_drive.cpp
#define SSE_TOS_PRG_AUTORUN// Atari PRG + TOS file direct support
#define SSE_TOS_PRG_AUTORUN2 //better behaviour
#define SSE_TOS_PRG_AUTORUN_390 // HD not really off after ejection!
#define SSE_TOS_PRG_AUTORUN_392
#endif
#define SSE_TOS_SNAPSHOT_AUTOSELECT
#endif//win32
#define SSE_TOS_WARNING // version/ST type
#ifdef SSE_HACKS
#define SSE_TOS_STE_FAST_BOOT //from hatari
#endif
#define SSE_TOS_BOOTER1//accept TOS boot of the 260ST
//#define SSE_TOS_GEMDOS_DRVBIT // sooner //really necessary?
#define SSE_TOS_GEMDOS_EM_381A // done the day before release
#define SSE_TOS_GEMDOS_EM_381B // so is it rock-solid?
#define SSE_TOS_GEMDOS_EM_382
#define SSE_AVTANDIL_FIX_002 // we have new TOS flags
#define SSE_TOS_CHECKSUM
#define SSE_TOS_GEMDOS_EM_390 

#endif


#if defined(SSE_TOS_KEYBOARD_CLICK)

#define SSE_GUI_OPTIONS_KEYBOARD_CLICK

#endif


#if defined(SSE_UNIX)

#define SSE_UNIX_OPTIONS_SSE_ICON
#define SSE_UNIX_STATIC_VAR_INIT //odd

#endif


#if defined(SSE_VARIOUS)

#define SSE_VAR_DEAD_CODE // just an indication
#define SSE_VAR_EMU_DETECT // option to disable emu detect
#define SSE_VAR_DONT_INSERT_NON_EXISTENT_IMAGES // at startup
#define SSE_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES // at final save
#define SSE_VAR_NO_UPDATE // remove all code in relation to updating
#define SSE_VAR_NO_WINSTON // nuke WinSTon import, saves 16K in VC6 release yeah
#define SSE_VAR_REFACTOR_392
#define SSE_VAR_RESIZE // reduce memory set (int->BYTE etc.)
#define SSE_VAR_RESIZE_392
#define SSE_VAR_REWRITE
#ifdef WIN32
#define SSE_VAR_UNRAR // using unrar.dll, up to date
#define SSE_VAR_CHECK_SNAPSHOT
#endif
#define SSE_VAR_RESET_SAME_DISK//test (with .PRG support)
#if defined(WIN32) && defined(VC_BUILD) // works with VC6, VS2008 not BCC
#define SSE_VAR_ARCHIVEACCESS // 7z support
#define SSE_VAR_ARCHIVEACCESS2 // bz2 (with modded dll), gz, tar, arj
#ifdef SSE_X64 //no unzip32, no unzipd64.dll...
#define SSE_VAR_ARCHIVEACCESS3 // zip managed by ArchiveAccess.dll by default - no, because of MSA Converter TODO
#define SSE_VAR_ARCHIVEACCESS4 // remove code for unzipd32.dll - what if archiveaccess fails!
#endif
#endif
#define SSE_VAR_NO_UPDATE_372
#define SSE_VAR_ARG_STFM // start as STF (unless overruled)
#define SSE_VAR_392
#define SSE_BUGFIX_392
#define SSE_VAR_OPT_380 //switch created in v3.9.0
#define SSE_VAR_OPT_382
#define SSE_VAR_OPT_390
#define SSE_VAR_OPT_391
#define SSE_VAR_OPT_392
#define SSE_VAR_WINVER
#if defined(SSE_VAR_OPT_390) && defined(VC_BUILD) && _MSC_VER>=1500 //VS2008+
#define SSE_VC_INTRINSICS_390
#if defined(SSE_VC_INTRINSICS_390)
//#define SSE_VC_INTRINSICS_390A // some cpu sr checks //useless like 390E?
#define SSE_VC_INTRINSICS_390B // popcount 
#define SSE_VC_INTRINSICS_390C // byteswap
#define SSE_VC_INTRINSICS_390D // instruction time => much less object code :)
//#define SSE_VC_INTRINSICS_390E // set/clear some sr bits => more object code? => bad idea?
//#define SSE_VC_INTRINSICS_390F // avoid shifting mask in MOVEM //i'm sure it's a bad idea too!
#define SSE_VC_INTRINSICS_390G // GLU functions
#endif
#endif
#if defined(SSE_VAR_OPT_390)
#ifdef SSE_CPU
#define SSE_VAR_OPT_390A // variable to hold ABSOLUTE_CPU_TIME for a while (TODO)
#define SSE_VAR_OPT_390A1 // Video chipset writes
#endif
#define SSE_VAR_OPT_390C // CycleOfLastChangeToShiftMode()
#define SSE_VAR_OPT_390E // blitter
#endif
#if defined(SSE_VAR_OPT_390) && defined(SSE_DRAW_C)
#define SSE_DRAW_C_390
#if defined(SSE_DRAW_C_390)
#define SSE_DRAW_C_390A //loops (better performance in VS2015)
#define SSE_DRAW_C_390A1 // replace if ladder with shifts (better performance in VS2015)
#define SSE_DRAW_C_390B //border (better performance in VS2015)
#define SSE_DRAW_C_390C //
#if defined(SSE_VC_INTRINSICS_390)
#define SSE_DRAW_C_390_INTRINSICS // (better performance in VS2015)
//#define SSE_DRAW_C_390_INTRINSICSB //BT worse performance in VS2015
//#define SSE_DRAW_C_390_INTRINSICSC //STOSD 2 items: worse performance in VS2015
#endif
#endif
#endif
// Exception management...
//#define SSE_M68K_EXCEPTION_TRY_CATCH //works but too slow, especially if _DEBUG
#ifndef _DEBUG
#define SSE_VAR_MAIN_LOOP2 //2KB on optimised exe but will catch and report everything (?)
#endif
#if _MSC_VER >= 1500 && !defined(_DEBUG)
#define SSE_VAR_MAIN_LOOP3 //VC only (vs2008)?
#endif
//#undef SSE_CPU_DIVS_OVERFLOW_PC //temp for tests...
#define SSE_VAR_CHECK_SNAPSHOT2
#define SSE_VAR_NO_UPDATE_390
#define SSE_VAR_ARCHIVEACCESS_390 // file handle leak

#endif//various

///////////
// VIDEO //
///////////

#if defined(SSE_VIDEO)

#if defined(WIN32)
// Windows build is either DirectDraw (DD) or Direct3D (D3D)
// SSE_DD as compile directive (config, makefile) commands DirectDraw build
// D3D has smaller footprint and more abilities than DD
#if defined(SSE_DD)
#define SSE_VID_DD
//#define SSE_VID_DD7 // if not defined, DD2
#else
#define SSE_VID_D3D
#endif
#endif//win32

//#define SSE_VID_SDL        // Simple DirectMedia Layer (TODO?)

// no crash...
#define SSE_VID_ADJUST_DRAWING_ZONE1 // attempt to avoid crash DD
#define SSE_VID_ADJUST_DRAWING_ZONE2 // DD+D3D
#define SSE_VID_ANTICRASH_392
#define SSE_VID_CHECK_POINTERS
//
#define SSE_VID_BPP_CHOICE // 8bit, 16bit or 32bit at choice
#define SSE_VID_DISABLE_AUTOBORDER
#define SSE_VID_DISABLE_AUTOBORDER_HIRES // different for monochrome
#define SSE_VID_EXT_MON
#define SSE_VID_FS_382
#define SSE_VID_FS_GUI_OPTION
#define SSE_VID_FS_GUI_392 // dialog boxes weren't erased when moved (DD+D3D)
#define SSE_VID_FS_GUI_392B // changing res triggered status bar refresh (DD+D3D)
#define SSE_VID_GAMMA
#define SSE_VID_GUI_392
#define SSE_VID_SCREEN_CHANGE_TIMING // useful if VSYNC is used
#define SSE_VID_SCREENSHOTS_NUMBER
#define SSE_VID_UTIL // miscellaneous functions

#if defined(WIN32)
#define SSE_VID_2SCREENS //392
#define SSE_VID_3BUFFER // Triple Buffering
#if defined(SSE_VID_3BUFFER)
#define SSE_VID_3BUFFER_FS // fullscreen Triple Buffering // DD + D3D
#endif
#define SSE_VID_BLOCK_WINDOW_SIZE // option can't change size of window
#define SSE_VID_BORDERS // optional larger borders
#define SSE_VID_FREEIMAGE // mods in use of this plugin
#define SSE_VID_FS_PROPER_QUIT_BUTTON // top right + made bigger icon
#define SSE_VID_LOCK_ASPET_RATIO // 
#define SSE_VID_MEMORY_LOST // no message box
#define SSE_VID_SAVE_NEO // screenshots in ST Neochrome format
#define SSE_VID_SCANLINES_INTERPOLATED
#define SSE_VID_SCANLINES_INTERPOLATED_392
#define SSE_VID_SCANLINES_INTERPOLATED_392B // mixed output
#define SSE_VID_ST_ASPECT_RATIO // like SainT, higher pixels
#if defined(SSE_VID_ST_ASPECT_RATIO)
#define SSE_VID_ST_ASPECT_RATIO_WIN
#endif
#define SSE_VID_VSYNC_WINDOW // yeah!
#endif//WIN32

#endif//video

#if defined(SSE_VID_BORDERS)

#define SSE_VID_BPOC // Best Part of the Creation
#define SSE_VID_BORDERS_BIGTOP // more lines for palette effects
#define SSE_VID_BORDERS_412 // 
#define SSE_VID_BORDERS_413 // best fit for overscan?
#define SSE_VID_BORDERS_416 
#define SSE_VID_BORDERS_LIMIT_TO_245
#define SSE_VID_BORDERS_LINE_PLUS_20
#define SSE_VID_BORDERS_BIGTOP_381
#define SSE_VID_BORDERS_GUARD_EM // don't mix large borders & extended res
#define SSE_VID_BORDERS_GUARD_R2 // don't mix large borders & hi res
//#define SSE_VID_BORDERS_NO_LOAD_SETTING // start at 384 x 270
#if defined(SSE_VID_BORDERS)
#define SSE_VID_BORDERS_GUI_392 // two options -> one option for on/off + size
#endif

#endif//borders

#if defined(SSE_VID_D3D)

#if defined(SSE_VID_2SCREENS) && !defined(BCC_BUILD) // (Win 2000 min)
#define SSE_VID_D3D_2SCREENS // DD does it itself
#endif
#define SSE_VID_D3D_FAKE_FULLSCREEN
#define SSE_VID_D3D_LIST_MODES // player can choose (TODO switch broken)
#define SSE_VID_D3D_CRISP //D3D can do that
#define SSE_GUI_D3D_CRISP_OPTION //the harder part!
#define SSE_VID_D3D_382
#if defined(SSE_VID_3BUFFER_FS)
#define SSE_VID_D3D_3BUFFER
#endif
#define SSE_VID_D3D_INTERPOLATED_SCANLINES
#define SSE_VID_D3D_FS_392A // changing fullscreen size caused trash in borders
#define SSE_VID_D3D_FS_392C // double creation at "activate"?
#define SSE_VID_D3D_FS_392D // memorise d3dpp
#define SSE_VID_D3D_FS_392D1 // delete/create texture with sprite
#define SSE_VID_D3D_FS_DEFAULT_HZ
#define SSE_VID_D3D_NO_FREEIMAGE //saves some KB
#define SSE_VID_D3D_PIC_AT_START
#define SSE_VID_D3D_SCREENSHOT
#if defined(SSE_VID_ST_ASPECT_RATIO)
#define SSE_VID_D3D_ST_ASPECT_RATIO 
#endif

#endif//d3d

#ifdef SSE_VID_DD // DirectDraw

#if defined(SSE_VID_3BUFFER)
#define SSE_VID_3BUFFER_WIN // window Triple Buffering (DD)
#define SSE_VID_DD_3BUFFER_WIN
#endif
#define SSE_VID_DD_BLIT_TRY_BLOCK //useless?
#define SSE_VID_DD_FS_32BIT
#define SSE_VID_DD_FS_IS_392
#if defined(SSE_VID_BORDERS)
#define SSE_VID_DD_FS_MAXRES // using the display's natural resolution 
#endif
#define SSE_VID_DD_NO_FS_CLIPPER // clipper makes the fullscreen GUI unusable in Windows 10
#define SSE_VID_RECORD_AVI //avifile not so good 
#define SSE_VID_DD_SCREENSHOT_391
#if defined(SSE_VID_ST_ASPECT_RATIO)
#define SSE_VID_DD_ST_ASPECT_RATIO 
#endif

#endif//dd


#if defined(SSE_VID_EXT_MON)

#define SSE_VID_EXT_MON_1024X720
#define SSE_VID_EXT_MON_1280X1024
#define SSE_VID_EXT_FS1
#define SSE_VID_EXT_FS2 // max screen

#endif


#if defined(SSE_VID_FREEIMAGE)

#define SSE_VID_FREEIMAGE1 //init library
#define SSE_VID_FREEIMAGE2 //always convert pixels
#define SSE_VID_FREEIMAGE3 //flip pic + remove WBMP
#if defined(SSE_DELAY_LOAD_DLL) && !defined(MINGW_BUILD) //TODO check this again
#define SSE_VID_FREEIMAGE4 // use official header
#endif

#endif//freeimage


#if defined(SSE_WD1772)

#define SSE_WD1772_REGS_FOR_FDC
#define SSE_WD1772_CRC 
#define SSE_WD1772_IDFIELD 
#define SSE_WD1772_MFM 

#endif//wd


#if defined(SSE_YM2149)

#define SSE_YM2149_OBJECT

#ifdef SSE_DRIVE
#define SSE_YM2149_DRIVE
#endif
#ifdef SSE_SOUND
#define SSE_YM2149_SOUND
#endif

#endif//ym2149


#if defined(SSE_YM2149_DRIVE)

#define SSE_YM2149A // selected drive, side as variables
#define SSE_YM2149B // adapt drive motor status to FDC STR at each change
#define SSE_YM2149C // turn on/off motor in drives
#define SSE_YM2149_DRIVE_392

#endif


#if defined(SSE_YM2149_SOUND)

#define SSE_YM2149_BUS_JAM_390 // 1 cycle each access
#define SSE_YM2149_BUS_JAM_390B // not more (word access)
#define SSE_YM2149_BUS_NO_JAM_IF_NOT_RW
#define SSE_YM2149_DELAY_RENDERING // so that we use table also for envelope
#define SSE_YM2149_FIX_TABLES // option P.S.G.
#define SSE_YM2149_FIXED_VOL_TABLE // was SSE_YM2149_FIXED_VOL_FIX2 in v3.6.4
#define SSE_SOUND_250K
#define SSE_SOUND_DYNAMICBUFFERS //psg
#define SSE_SOUND_DYNAMICBUFFERS2 //dma
#if ! defined(SSE_LEAN_AND_MEAN)
#define SSE_SOUND_DYNAMICBUFFERS3  //use factor, maybe? (larger, safer)
#endif
#define SSE_SOUND_MORE_SAMPLE_RATES
#define SSE_SOUND_16BIT_CENTRED
#define SSE_YM2149_392

#endif


#if defined(SSE_YM2149_FIXED_VOL_TABLE)

//#define SSE_YM2149_DYNAMIC_TABLE0 //temp, to build file
//#define SSE_YM2149_DYNAMIC_TABLE1 //temp, to build quieter file
#define SSE_YM2149_DYNAMIC_TABLE //using file
#define SSE_GUI_OPTIONS_SAMPLED_YM
#define SSE_YM2149_QUANTIZE_382

#endif

#if defined(SSE_YM2149_392)
#define SSE_YM2149_DISABLE_CAPTURE_FILE // never noticed this before...
#define SSE_YM2149_FIX_ENV_TABLE // interpolated from fixed volume values
#define SSE_YM2149_MAMELIKE
//#define SSE_YM2149_MAMELIKE2 //undef
#define SSE_YM2149_MAMELIKE3 //def
#define SSE_YM2149_MAMELIKE_AVG_SMP // oversampling by artihmetic averaging
#endif


////////////////////////
// MULTIPLE DEPENDENT //
////////////////////////

#if defined(SSE_CPU_E_CLOCK) && defined(SSE_INTERRUPT)

#define SSE_INT_E_CLOCK
#define SSE_INT_HBL_IACK2
#define SSE_INT_VBL_IACK2

#endif


#if defined(SSE_CPU) && defined(SSE_MMU)

#define SSE_MMU_ROUNDING_BUS

#endif


#if defined(SSE_CPU) && defined(SSE_VIDEO_CHIPSET)

#define SSE_CPU_CHECK_VIDEO_RAM

#endif


#if defined(SSE_DISK) && defined(WIN32) && defined(SSE_WD1772) \
  && defined(SSE_WD1772_IDFIELD)

#define SSE_DISK_GHOST

#endif


#if defined(SSE_DMA) && defined(SSE_STF)

#define SSE_DMA_RIPPLE_CARRY

#endif


#if defined(SSE_FDC_ACCURATE) && defined(SSE_DISK) && defined(SSE_DMA) \
  && defined(SSE_WD1772) && defined(SSE_YM2149_DRIVE)

#define SSE_FDC_ACCURATE_BEHAVIOUR
#define SSE_FDC_ACCURATE_TIMING

#endif


#if defined(SSE_WD1772) && defined(SSE_DISK) && defined(SSE_DRIVE)\
  && defined(SSE_WD1772_CRC) && defined(SSE_WD1772_IDFIELD)

#define SSE_WD1772_EMU

#endif


#if defined(SSE_WD1772) && defined(SSE_DISK) && defined(WIN32)

#define SSE_DISK_CAPS        // CAPS support (IPF, CTR disk images) 

#endif


#if defined(SSE_WD1772_EMU) && defined(SSE_DISK) && defined(WIN32)

#define SSE_DISK_MFM

#endif


#if defined(SSE_GUI_STATUS_BAR) && defined(SSE_CPU_HALT)

#define SSE_GUI_STATUS_BAR_ALERT // blit error, halt...

#endif


#if defined(SSE_MMU_WU) && defined(SSE_SHIFTER_TRICKS)

#define SSE_GLUE_TCB // Swedish New Year Demo/TCB
#define SSE_SHIFTER_UNSTABLE // DoLB, Omega, Overdrive/Dragon, Beeshift

#endif


#if defined(SSE_STF) && defined(SSE_SHIFTER)

#define SSE_SHIFTER_PALETTE_NOISE //UMD8730 STF

#endif


#if defined(SSE_STF) && defined(SSE_TOS) && defined(WIN32)

#define SSE_STF_MATCH_TOS // select a compatible TOS for next reset
#define SSE_STF_MATCH_TOS3 // + default country

#endif


#if defined(SSE_HARDDISK) && defined(SSE_DMA) && defined(SSE_WD1772_EMU)

#define SSE_ACSI //3.8.0 new feature

#endif


#if defined(SSE_INT_HBL) && defined(SSE_STF) && defined(SSE_MMU_WU)

#define SSE_INT_HBL_E_CLOCK_HACK_382 //here we go again

#endif


/////////////////////////////////////
// DETAILS FOR  MULTIPLE DEPENDENT //
/////////////////////////////////////


#if defined(SSE_ACSI)

#define SSE_ACSI_FORMAT 
#ifdef SSE_DEBUG
#define SSE_ACSI_BOOTCHECKSUM
#endif
#define SSE_ACSI_DISABLE_HDIMG // former Steem medium-level stub
#define SSE_ACSI_INQUIRY
#if defined(SSE_OSD_DRIVE_LED_HD)
#define SSE_ACSI_LED
#endif
#define SSE_ACSI_LS
#define SSE_ACSI_MODESELECT
#define SSE_ACSI_OPTION
#define SSE_ACSI_REQUEST_SENSE
#define SSE_ACSI_TIMING // ADAT -> slower (risky?)
#ifdef WIN32
#define SSE_ACSI_HDMAN // browser
#define SSE_ACSI_ICON
#endif
#define SSE_ACSI_RELOAD_TOS

#endif//acsi


#if defined(SSE_DISK_GHOST)

#define SSE_DISK_GHOST_FAKE_FORMAT
#define SSE_DISK_GHOST_SECTOR // commands $A#/$8#
#define SSE_DISK_GHOST_MULTIPLE_SECTORS // commands $B#/$9#
#define SSE_DISK_GHOST_SECTOR_STX1 // in pasti

#endif


#if defined(SSE_DISK_CAPS)

#define SSE_DISK_CAPS_RESUME
#define SSE_DISK_CAPS_CTRAW_1ST_LOCK
#define SSE_DISK_CAPS_390C

#endif


#if defined(SSE_DISK_MFM)

#define SSE_DISK_STW
#if defined(SSE_DISK_STW)
#define SSE_DISK_STW_FAST // +HFE
#endif
#define SSE_DISK_SCP // Supercard Pro disk image format support
#define SSE_DISK_HFE // HxC floppy emulator HFE image support
#if defined(SSE_GUI_DISK_MANAGER)
#define SSE_GUI_DM_STW //new context option
#define SSE_GUI_STW_CONVERT // Convert to STW
#endif

#endif


#if defined(SSE_DISK_SCP)

//#define SSE_DISK_SCP_TO_MFM_PREVIEW // keep it, could be useful
#define SSE_DISK_SCP_DRIVE_WOBBLE // for weak bits
#define SSE_DISK_SCP_RANDOMISE // War Heli

#endif


#if defined(SSE_DISK_HFE)

#if defined(SSE_GUI_DISK_MANAGER)
#define SSE_GUI_DM_HFE // creating HFE images in Steem
#endif
#if defined(SSE_GUI)
#define SSE_GUI_ASSOCIATE_HFE // they're generally not zipped...
#endif
#define SSE_DISK_HFE_TRIGGER_IP // changes nothing?
//#define SSE_DISK_HFE_TO_STW // using Steem to convert current disk

#endif


#if defined(SSE_FDC_ACCURATE_BEHAVIOUR)

#define SSE_FDC_FORCE_INTERRUPT

#endif


#if defined(SSE_FDC_ACCURATE_TIMING)

#define SSE_FDC_SPIN_UP // both timing and behaviour
#define SSE_FDC_VERIFY_AGENDA 

#endif


#if defined(SSE_WD1772_EMU) 

#define SSE_WD1772_F7_ESCAPE
#define SSE_WD1772_BIT_LEVEL // necessary for SCP support
#define SSE_WD1772_390
#define SSE_WD1772_390C

#endif


#if defined(SSE_SHIFTER_UNSTABLE)

#define SSE_SHIFTER_HI_RES_SCROLLING // Beeshift2
#define SSE_SHIFTER_MED_RES_SCROLLING // Beeshift
#define SSE_SHIFTER_MED_RES_SCROLLING_360//switch was missing in original source revision
#define SSE_SHIFTER_UNSTABLE_380
#define SSE_SHIFTER_UNSTABLE_390 //bugfixes
#define SSE_SHIFTER_UNSTABLE_392

#endif


///////////
// DEBUG //
///////////

// SSE_DEBUG is defined or not by the environment, just like STEVEN_SEAGAL
// debug switches are grouped here

#if defined(SSE_DEBUG) // Boiler + debug build
#define SSE_DEBUG_ASSERT // 3.8.2
#if defined(SSE_DEBUG) && defined(DEBUG_BUILD)
#define SSE_BOILER
#endif

#if defined(SSE_BOILER)
#define SSE_BOILER_FRAME_REPORT //3.3, now boiler-only because of mask control
#if defined(SSE_BOILER_FRAME_REPORT)
#define SSE_BOILER_FRAME_REPORT_ON_STOP // each time we stop emulation
#endif
#define SSE_BOILER_REPORT_SCAN_Y_ON_CLICK
#define SSE_BOILER_REPORT_SDP_ON_CLICK // yeah!
#define SSE_BOILER_AUTO_FLUSH_TRACE// v3.7.1
#define SSE_BOILER_AUTO_FLUSH_TRACE_390
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
#ifdef SSE_CPU
#define SSE_BOILER_NODIV // no DIV log necessary anymore
#endif
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
#define SSE_BOILER_68030_STACK_FRAME 
#define SSE_BOILER_STACK_CHOICE
#define SSE_BOILER_TIMER_B // instead of 0
#define SSE_BOILER_TIMERS_ACTIVE // (in reverse video) yeah!
#define SSE_BOILER_TRACE_MFP1 // one log option for MFP, one for interrupt
//#define SSE_BOILER_TRACE_NOT_OPTIONAL
#define SSE_BOILER_TRAP
#define SSE_BOILER_WIPE_TRACE // as logfile
#define SSE_BOILER_WIPE_TRACE2//need date
#endif//boiler

#ifdef SSE_CPU
//#define SSE_CPU_EXCEPTION_TRACE_PC // reporting all PC (!)
#if defined(SSE_BETA)
//#define SSE_CPU_DETECT_STACK_PC_USE // push garbage!!
#endif

#endif//cpu

#ifdef SSE_FLOPPY
#define SSE_DEBUG_FDC_TRACE_STATUS //spell out status register
#define SSE_DEBUG_IPF_TRACE_SECTORS // show sector info (IPF)
#define SSE_DRIVE_COMPUTE_BOOT_CHECKSUM //TODO boiler control
//#define SSE_DEBUG_FDC_TRACE_STR // trace read STR (careful)
#define SSE_DEBUG_WRITE_TRACK_TRACE_IDS
//#define SSE_YM2149_REPORT_DRIVE_CHANGE // as FDC trace
#define SSE_DEBUG_FDC_TRACE_IRQ
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

#if defined(SSE_MIDI)
//#define SSE_MIDI_TRACE_BYTES_IN
//#define SSE_MIDI_TRACE_BYTES_OUT
//#define SSE_MIDI_TRACE_BYTES_OUT_OVR
//#define SSE_MIDI_TRACE_READ_STATUS
#endif//midi

#ifdef SSE_OSD
#define SSE_OSD_DEBUG_MESSAGE
#endif


#define SSE_DEBUG_RESET

#ifdef SSE_SHIFTER
#if !defined(SSE_DEBUG_TRACE_IDE) && !defined(SSE_UNIX)
#define SSE_SHIFTER_REPORT_VBL_TRICKS // a line each VBL
#endif

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
#ifdef SSE_CPU_E_CLOCK
#define SSE_BOILER_SHOW_ECLOCK
#endif
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
#if defined(SSE_VID_D3D)
#define SSE_DEBUG_D3D
#endif
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

//390
#ifdef SSE_BOILER
#define SSE_BOILER_390
#define SSE_BOILER_390B
#define SSE_BOILER_390_LOG2 // new title for more options
#define SSE_BOILER_390_IRQ_BREAK //request
#endif


#else//SSE_DEBUG

//3.8.2
#define DEBUG_FACILITIES_IN_RELEASE

#if defined(DEBUG_FACILITIES_IN_RELEASE)
//#define ASSERT_FOR_RELEASE //careful with this (performance)
#define OSD_FOR_RELEASE // time
#define SSE_TRACE_FOR_RELEASE // TRACE2: the new breed!
#endif

#if defined(ASSERT_FOR_RELEASE)
#define SSE_DEBUG_ASSERT
#endif

#if defined(OSD_FOR_RELEASE)
#define SSE_OSD_DEBUG_MESSAGE
#define SSE_DEBUG_RESET
#endif

#ifdef SSE_TRACE_FOR_RELEASE
#define SSE_DEBUG_TRACE
#define SSE_DEBUG_TRACE_FILE
#define SSE_DEBUG_START_STOP_INFO
#define SSE_DEBUG_START_STOP_INFO2
#define SSE_DEBUG_START_STOP_INFO3
#define SSE_BOILER_AUTO_FLUSH_TRACE
#define SSE_BOILER_AUTO_FLUSH_TRACE_390
#define SSE_TRACE_FOR_RELEASE_390
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


#if defined(SSE_LE)

#define SSE_VAR_MAIN_LOOP4
#define SSE_IKBD_6301_NOT_OPTIONAL 
#define SSE_C2_NOT_OPTIONAL
#undef SSE_CPU_MFP_RATIO_OPTION
#undef SSE_CPU_MFP_RATIO_OPTION2
#define SSE_FLOPPY_ALWAYS_ADAT
// pasti corrections are for SSE too
#define SSE_SOUND_NO_8BIT
#define SSE_SOUND_ENFORCE_RECOM_OPT
#define SSE_SOUND_MICROWIRE_NOT_OPTIONAL
#undef SSE_GUI_OPTIONS_MICROWIRE
#define SSE_YM2149_TABLE_NOT_OPTIONAL
//#define SSE_SOUND_FEWER_FILTERS //all builds
//#define SSE_SOUND_CAN_CHANGE_DRIVER //all builds
#define SSE_SOUND_NO_NOSOUND_OPTION
#define SSE_TOS_PRG_AUTORUN_NOT_OPTIONAL
#define SSE_VID_ENFORCE_AUTOFRAMESKIP
#define SSE_VID_BPP_NO_CHOICE
#if defined(SSE_DD)
#define SSE_VID_DD_SIMPLIFY_VSYNC
#define SSE_GUI_DD_FULLSCREEN_LE
#endif
#define SSE_GUI_STATUS_BAR_NOT_OPTIONAL
#define SSE_GUI_NO_PROFILES
#define SSE_MMU_WU_LE
#undef SSE_GUI_OPTIONS_WU
#define SSE_GUI_NO_CPU_SPEED
#undef SSE_GUI_STATUS_BAR_HISPEED
#undef SSE_GUI_STATUS_BAR_DISK_NAME
#define SSE_GUI_NO14MB
#undef SSE_MMU_256K
#undef SSE_MMU_2560K
#define NO_CRAZY_MONITOR //+ a little fix in emulator.cpp
#define SSE_GUI_NO_MACROS //don't work with C1
#define SSE_GUI_NO_PASTE
#define SSE_GUI_NO_ICONCHOICE

#endif//LE

#ifdef SSE_BOILER
#define SSE_BOILER_FRAME_REPORT_392
#endif

#if defined(SSE_BETA) //next version

//#define SSE_BLT_RESTART_393 //wrong, see down_tln
#define SSE_BOILER_393
#define SSE_DRIVE_FREEBOOT //393
#define SSE_GLUE_393
#define SSE_JOYSTICK_PADDLES //393
#define SSE_SOUND_OPTION_DISABLE_DSP // retake deleted code... 
#define SSE_SOUND_MICROWIRE_MIXMODE_393
#undef SSE_SOUND_MICROWIRE_MIXMODE
#undef  SSE_SOUND_MICROWIRE_MIXMODE2
#define SSE_STF_HW_OVERSCAN //393 - considered as STF models
#define SSE_VAR_393
#define SSE_VAR_STEEMINTRO_393 // finally update this
#define SSE_VAR_UPDATE_LINK 
#define SSE_VID_HIRES_BORDER_FIX //set screen higher: not beautiful but realistic and less hacky
#define SSE_VID_HIRES_BORDER_BLACK // border is black
#define SSE_VID_ST_MONITOR_393 // 2 options again
#define SSE_YM2149_MAMELIKE_393
#define SSE_YM2149_RECORD // record to YM file

#if defined(SSE_GLUE_393)
#define SSE_GLUE_393A // param SDP reload
#define SSE_GLUE_393B // VCount counts up (again :)), makes more sense for mono
#define SSE_GLUE_393C // not necessary, to test!
#endif

#if defined(SSE_STF_HW_OVERSCAN)
#define SSE_STF_AUTOSWITCH
#define SSE_STF_LACESCAN
#endif

#if defined(SSE_YM2149_MAMELIKE_393)
#define SSE_YM2149_MAMELIKE_ANTIALIAS
#undef SSE_YM2149_MAMELIKE_AVG_SMP // poor man's filter
#endif

#endif//beta

#ifdef SSE_BETA // long term, tests
//#define SSE_CPU_RESTORE_ABUS
//#define SSE_CPU_RESTORE_ABUS1
//#define SSE_CPU_SIMPLIFY_READ_DEST //no good, TODO?
//#define SSE_CPU_SIMPLIFY_READ_DEST_CMPI
//#define SSE_CPU_SIMPLIFY_READ_DEST_TST
//#define SSE_INT_MFP_TIMER_B_PULSE //TODO
//#define SSE_MMU_LOW_LEVEL //?
#define SSE_VAR_ARG_SNAPSHOT_PLUS_DISK
#define SSE_VAR_SNAPSHOT_ADAPT_ST_TYPE
//#define TEST_STEEM_INTRO
#endif

#if defined(SSE_BETA_BUGFIX)

//#define SSE_BUGFIX_MORE_RLZ_TRACES // for interactive debugging with unfortunate player

#define SSE_BUGFIX_393
#define SSE_BUGFIX_393A //blitter don't wipe all ioaccess
#define SSE_BUGFIX_393B //forgotten check io macro OR.W
#define SSE_BUGFIX_393B1 //badly placed check io macros bitshift
#define SSE_BLT_BUS_ARBITRATION_393A // blitter start check should be pre read, post write
//#define SSE_GUI_FONT_FIX // not DEFAULT_GUI_FONT if possible //problem was different apparently...
#define SSE_VID_D3D_393 // update m_DisplayFormat
#define SSE_VID_D3D_2SCREENS_393 // negative coordinates
#define SSE_VS2008_WARNING_393
#define SSE_WD1772_393 // wrong status after interrupt command
#define SSE_WD1772_393B // other fixes based on Suska (test)
#define SSE_WD1772_393C //timeout (use?)
#define SSE_JOYSTICK_NO_MM //circle around unsolved bug

#endif//bugfix

#else//!SS
/////////////////////////////////////
// if STEVEN_SEAGAL is NOT defined //
/////////////////////////////////////
// a fair warning (it actually helps detect building errors)
#pragma message("Are you mad? You forgot to #define STEVEN_SEAGAL!") 
#include "SSEDecla.h" // still need to neutralise debug macros
#endif//?SS

#endif// #ifndef SSE_H 
