// This file is compiled as a distinct module (resulting in an OBJ file)
#include "SSEOption.h"

#if defined(SS_SSE_OPTION_STRUCT)

TOption SSEOption; // singleton


TOption::TOption() {
  Init();
}


TOption::Init() {
#if defined(SS_HACKS)
  Hacks=TRUE;
#endif
#if defined(SS_IKBD_6301)
  HD6301Emu=FALSE;
#endif
#if defined(SS_SOUND_MICROWIRE)
  STEMicrowire=FALSE;
#endif
#if defined(SS_SOUND_FILTER_STF)
  PSGFilter=TRUE;
#endif
#if defined(SS_STF)
  STModel=STE;
#endif
#if defined(SS_VAR_MOUSE_CAPTURE)
  CaptureMouse=TRUE;
#endif
#if defined(SS_VID_BORDERS)
  BorderSize=0; // original Steem 3.2
#endif
#if defined(SS_VAR_STEALTH)
  StealthMode=FALSE;
#endif
  OutputTraceToFile=TRUE; // can be disabled in Boiler
  TraceFileLimit=TRUE; // stop TRACING to file at +-3+MB
}

#endif//#if defined(SS_SSE_OPTION_STRUCT)