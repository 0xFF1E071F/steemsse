// This file is compiled as a distinct module (resulting in an OBJ file)

// Don't look for anything important here, it's a trash can

#include "SSE.h"
#include "SSEDecla.h"
#include "SSEParameters.h"

#include "SSEDebug.h"

#if defined(STEVEN_SEAGAL)

#if ! defined(SS_SSE_CONFIG_STRUCT)
extern "C" int iDummy=0;
#endif

#if defined(SS_HACKS)
extern "C" int SS_signal=0;
#endif

#if defined(SS_VID_RECORD_AVI) 
#include <AVI/AviFile.h>
CAviFile *pAviFile=0;
BYTE video_recording=0;
char *video_recording_codec=SS_VID_RECORD_AVI_CODEC;
#endif

#if defined(SS_DELAY_LOAD_DLL) && defined(__BORLANDC__)
// http://edn.embarcadero.com/article/28515
FARPROC WINAPI MyLoadFailureHook(dliNotification dliNotify,DelayLoadInfo * pdli)
{
  // dliNotify == dliFailLoadLibrary or dliFailGetProcAddress or ...
  throw 0; // we have a catch(...) (hopefully)
}
#endif

#if defined(SS_OSD_SCROLLER_CONTROL)

TOsdControl OsdControl;

extern bool osd_show_scrollers,osd_shown_scroller;
extern DWORD timer,osd_scroller_finish_time;
extern long col_yellow[2],col_blue,col_red,col_green,col_white;


void TOsdControl::StartScroller(EasyStr text) {
  if(!osd_show_scrollers)
    return;
  ScrollerColour=col_yellow[0]; //TODO
  // add to current scroller
  ScrollText=text;
  ScrollerPhase=WANT_SCROLLER;
  osd_shown_scroller=0;
}

#endif

#endif//#if defined(STEVEN_SEAGAL)