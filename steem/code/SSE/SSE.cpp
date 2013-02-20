// This file is compiled as a distinct module (resulting in an OBJ file)

#include "SSE.h"
#include "SSEdecla.h"
#include "SSEParameters.h"


#if defined(STEVEN_SEAGAL)

extern "C" int iDummy=0;

#if defined(SS_HACKS)
extern "C" int SS_signal=0;
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VID_RECORD_AVI) 
#include <AVI/AviFile.h>
CAviFile *pAviFile=0;
int video_recording=0;
char *video_recording_codec=SS_VID_RECORD_AVI_CODEC;
#endif

#if defined(SS_DELAY_LOAD_DLL)
#ifdef __BORLANDC__
// http://edn.embarcadero.com/article/28515
FARPROC WINAPI MyLoadFailureHook(dliNotification dliNotify,DelayLoadInfo * pdli)
{
  // dliNotify == dliFailLoadLibrary or dliFailGetProcAddress or ...
  throw 0; // we have a catch(...) (hopefully)
}
#endif
#endif

#endif//#if defined(STEVEN_SEAGAL)