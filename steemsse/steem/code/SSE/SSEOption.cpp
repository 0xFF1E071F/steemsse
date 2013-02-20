// This file is compiled as a distinct module (resulting in an OBJ file)
#include "SSEOption.h"
#include "SSEDebug.h"
#include "SSESTF.h" // for the enum

#if defined(SS_SSE_OPTION_STRUCT)

TOption SSEOption; // singleton

//tmp
#ifdef UNIX
#define TRUE 1 // don't screw this one up...
#define FALSE 0
#endif

TOption::TOption() {
  Init(); // a problem...
}


void TOption::Init() {
  //TRACE("init SSEOption\n");
//  ZeroMemory(this,sizeof(TOption)); 
#if defined(SS_HACKS)
  Hacks=TRUE;
#endif
  HD6301Emu=FALSE;
  STEMicrowire=FALSE;
#if defined(SS_SOUND_FILTER_STF)
  PSGFilter=TRUE;
#endif
  STModel=0;//STE;
  CaptureMouse=TRUE;
  DisplaySize=0; // original Steem 3.2
  StealthMode=FALSE;
  OutputTraceToFile=TRUE; // can be disabled in Boiler
  TraceFileLimit=FALSE;//TRUE; // stop TRACING to file at +-3+MB
  WakeUpState=0; 
}

#endif//#if defined(SS_SSE_OPTION_STRUCT)


#if defined(SS_SSE_CONFIG_STRUCT)

TConfig SSEConfig;

TConfig::TConfig() {
  ZeroMemory(this,sizeof(TConfig));
}

#endif//#if defined(SS_SSE_CONFIG_STRUCT)
