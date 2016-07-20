#pragma once //SS
#ifndef INCLUDE_H //SS
#define INCLUDE_H //SS

#ifdef UNIX
bool enable_zip=true;
#else
bool enable_zip=false;
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
//---------------------------------------------------------------------------
#ifdef WIN32
HINSTANCE Inst;	// this global variable is declared in this header SS
#endif
#define HInstance Inst
//---------------------------------------------------------------------------
#include <easystr.h>
#include <mymisc.h>
#include <easycompress.h>
#include <easystringlist.h>
#include <portio.h>
#include <dynamicarray.h>
#include <dirsearch.h>

#if USE_PASTI
#include <pasti/pasti.h>
#endif

#include <configstorefile.h>
#define GoodConfigStoreFile ConfigStoreFile

#ifndef NO_PORTAUDIO
#ifdef WIN32
#if defined(USE_PORTAUDIO_ON_WIN32) && defined(IN_MAIN)
#include <portaudio/portaudio_dll.cpp>
#endif
#else
//#include <portaudio.h>//SS temp
#endif
#endif

//#define NO_RTAUDIO

#ifndef NO_RTAUDIO
#ifdef UNIX
#include <RtAudio.h>
using namespace std;
#endif
#endif

typedef EasyStr Str;
#if defined(SSE_STRUCTURE_DECLA)
#include "translate.decla.h"
#else
#include "translate.h"
#endif
#ifdef WIN32

#include <choosefolder.h>
#include <scrollingcontrolswin.h>
#include <directory_tree.h>
#include <input_prompt.h>

#elif defined(UNIX)

#include <x/hxc.h>
#include <x/hxc_alert.h>
#include <x/hxc_prompt.h>
#include <x/hxc_fileselect.h>
#include <x/hxc_popup.h>
#include <x/hxc_popuphints.h>
#include <x/hxc_dir_lv.h>
#include "x/x_controls.h"

//#include <X11/Xcms.h>

#ifdef LINUX
#include </usr/include/linux/kd.h>  // linux internal speaker header
#endif

#endif
//---------------------------------------------------------------------------
#if defined(SSE_STRUCTURE_DECLA)

#ifdef WIN32
#include "resnum.decla.h"
#endif

#include "steemh.decla.h"
#include "notifyinit.decla.h"
#include "stports.decla.h"
#include "dir_id.decla.h"
#include "fdc.decla.h"
#include "floppy_drive.decla.h"
#include "hdimg.decla.h"
#include "init_sound.decla.h"
#include "psg.decla.h"
#include "loadsave.decla.h"


#else

#ifdef WIN32
#include "resnum.h"
#endif

#include "steemh.h"
#include "notifyinit.h"
#include "stports.h"
#include "dir_id.h"
#include "fdc.h"
#include "floppy_drive.h"
#include "hdimg.h"
#include "init_sound.h"
#include "psg.h"
#include "loadsave.h"

#endif

#ifndef NO_GETCONTENTS
#include "di_get_contents.h"
#endif

#if defined(SSE_STRUCTURE_DECLA)

#include "stemdialogs.decla.h"
#include "harddiskman.decla.h"
#include "diskman.decla.h"
#include "stjoy.decla.h"
#include "infobox.decla.h"
#include "options.decla.h"
#include "shortcutbox.decla.h"
#include "patchesbox.decla.h"
#include "display.decla.h"
#include "draw.decla.h"
#include "osd.decla.h"
#include "palette.decla.h"
#include "acc.decla.h"
#include "ikbd.decla.h"
#include "key_table.decla.h"
#include "mfp.decla.h"
#include "blitter.decla.h"
#include "run.decla.h"
#include "reset.decla.h"
#include "stemdos.decla.h"
#include "iorw.decla.h"
#include "cpu.decla.h"
#include "midi.decla.h"
#include "rs232.decla.h"
#ifdef DEBUG_BUILD
  #include "historylist.decla.h"
  #include "d2.decla.h"
  #include "dwin_edit.decla.h"
  #include "iolist.decla.h"
  #include "mem_browser.decla.h"
  #include "mr_static.decla.h"
  #include "debug_emu.decla.h"
  #include "boiler.decla.h"
  #include "trace.decla.h"
#endif

#include "archive.decla.h"
#include "gui.decla.h"

#include "macros.decla.h"
#include "dataloadsave.decla.h"
#include "emulator.decla.h"


#else

#include "stemdialogs.h"
#include "harddiskman.h"
#include "diskman.h"
#include "stjoy.h"
#include "infobox.h"
#include "options.h"
#include "shortcutbox.h"
#include "patchesbox.h"
#include "display.h"
#include "draw.h"
#include "osd.h"
#include "palette.h"
#include "acc.h"
#include "ikbd.h"
#include "key_table.h"
#include "mfp.h"
#include "blitter.h"
#include "run.h"
#include "reset.h"
#include "stemdos.h"
#include "iorw.h"
#include "cpu.h"
#include "midi.h"
#include "rs232.h"
#ifdef DEBUG_BUILD
  #include "historylist.h"
  #include "d2.h"
  #include "dwin_edit.h"
  #include "iolist.h"
  #include "mem_browser.h"
  #include "mr_static.h"
  #include "debug_emu.h"
  #include "boiler.h"
  #include "trace.h"
#endif

#include "archive.h"
#include "gui.h"

#include "macros.h"
#include "dataloadsave.h"
#include "emulator.h"

#endif

#include <wordwrapper.h>

#if defined(SSE_STRUCTURE_DECLA)
#include "screen_saver.decla.h"
#else
#include "screen_saver.h"
#endif

#ifdef WIN32
#include "dderr_meaning.h"
#endif

#ifdef ONEGAME
#include "onegame.h"
#endif

#endif//#ifndef INCLUDE_H //SS