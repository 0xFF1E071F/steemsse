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

#ifdef UNIX
//typedef unsigned char BYTE
#endif

struct THD6301 {
 enum  {
   CUSTOM_PROGRAM_NONE,
   CUSTOM_PROGRAM_LOADING,
   CUSTOM_PROGRAM_LOADED,
   CUSTOM_PROGRAM_RUNNING
 }custom_program_tag;

#ifdef __cplusplus //isolate member functions, for C it's just POD
  THD6301();
  ~THD6301();
#if defined(SSE_DEBUG)
  void InterpretCommand(BYTE ByteIn);
#if defined(SSE_DEBUG)
  void ReportCommand();
#endif
#endif
  void ReceiveByte(BYTE data);
  void ResetChip(int Cold);
  void ResetProgram();
  void Init();
#if defined(SSE_IKBD_6301_VBL)
  void Vbl();
#endif

#endif//c++?

  BYTE Initialised; // we do need a rom
  BYTE Crashed; // oops
  short MouseVblDeltaX; // must keep separate for true emu
  short MouseVblDeltaY;
#if defined(SSE_IKBD_6301_MOUSE_ADJUST_SPEED)
  BYTE click_x,click_y; // current click
#endif

#if defined(SSE_IKBD_6301_380) 
  // lower case because uppercase are constants in 6301 emu itself
  BYTE rdr,rdrs,tdr,tdrs; 
#endif

#if defined(SSE_IKBD_6301_EVENT)
  char LineRxFreeTime; // cycles in (0-63)
  char LineTxFreeTime; // cycles in (0-63)
#if !defined(SSE_ACIA_383)
  BYTE EventStatus; // bit0 event1 bit1 event2
#endif
#endif

#if defined(SSE_DEBUG) 
  short CurrentCommand;
  BYTE LastCommand;
  BYTE CurrentParameter; //0-5
  BYTE nParameters; //0-6
  BYTE Parameter[6]; // max 6
  BYTE CustomProgram;
#endif
};

#ifdef __cplusplus
//extern "C" 
#endif
extern 
#ifdef __cplusplus
"C" 
#endif
struct THD6301 HD6301;

#endif//#if defined(SSE_IKBD_6301)

#endif//#ifndef SSE6301_H