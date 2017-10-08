#pragma once
#ifndef SSE6301_H
#define SSE6301_H

// This h file will be included in a C and in a C++ module
// so it's gotta be compatible.
#if defined(SSE_IKBD_6301)

#include "SSEDecla.h"

#ifdef __cplusplus // we know this one is already included in 6301.c
extern "C" {
#include "../../3rdparty/6301/6301.h"
}
#endif

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct THD6301 {
 enum  {
   CUSTOM_PROGRAM_NONE,
   CUSTOM_PROGRAM_LOADING,
   CUSTOM_PROGRAM_LOADED,
   CUSTOM_PROGRAM_RUNNING
 }custom_program_tag;

  //DATA 

  COUNTER_VAR ChipCycles,MouseNextTickX,MouseNextTickY;
  int MouseCyclesPerTickX, MouseCyclesPerTickY;
  short MouseVblDeltaX; // must keep separate for true emu
  short MouseVblDeltaY;
#if defined(SSE_DEBUG) 
  short CurrentCommand;
#endif
  BYTE Initialised; // we do need a rom
  BYTE Crashed; // oops
  BYTE click_x,click_y; // current click
  // lower case because uppercase are constants in 6301 emu itself
  BYTE rdr,rdrs,tdr,tdrs; 
#if defined(SSE_DEBUG) 
  BYTE LastCommand;
  BYTE CurrentParameter; //0-5
  BYTE nParameters; //0-6
  BYTE Parameter[6]; // max 6
  BYTE CustomProgram;
#endif

  //FUNCTIONS
#ifdef __cplusplus //isolate member functions, for C it's just POD
  THD6301();
  ~THD6301();
  void ResetChip(int Cold);
  void ResetProgram();
  void Init();
  void Vbl();
#if defined(SSE_DEBUG)
  void InterpretCommand(BYTE ByteIn);
  void ReportCommand();
#endif

#endif//c++?


};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

extern 
#ifdef __cplusplus
"C" 
#endif
struct THD6301 HD6301;

#endif//#if defined(SSE_IKBD_6301)

#endif//#ifndef SSE6301_H