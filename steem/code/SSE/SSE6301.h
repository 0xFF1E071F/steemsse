#pragma once
#ifndef SSE6301_H
#define SSE6301_H

// This h file will be included in a C and in a C++ module
// so it's gotta be compatible.
#if defined(SS_IKBD_6301)

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
#if defined(SS_DEBUG) || defined(SS_IKBD_MOUSE_OFF_JOYSTICK_EVENT)
  void InterpretCommand(BYTE ByteIn);
#if defined(SS_DEBUG)
  void ReportCommand();
#endif
#endif
#if defined(SS_ACIA_DOUBLE_BUFFER_TX)
  void ReceiveByte(BYTE data);
#endif
  void ResetChip(int Cold);
  void ResetProgram();
  void Init();
#endif
  BYTE Initialised; // we do need a rom
#if defined(SS_IKBD_6301_RUN_CYCLES_AT_IO)
  BYTE RunThisHbl; 
#endif
  BYTE Crashed; // oops
  short CurrentCommand;
  BYTE LastCommand;
  BYTE CurrentParameter; //0-5
  BYTE nParameters; //0-6
  BYTE Parameter[6]; // max 6
  BYTE CustomProgram;
  short MouseVblDeltaX; // must keep separate for true emu
  short MouseVblDeltaY;
};

#ifdef __cplusplus
//extern "C" 
#endif
extern 
#ifdef __cplusplus
"C" 
#endif
struct THD6301 HD6301;

#endif//#if defined(SS_IKBD_6301)

#endif//#ifndef SSE6301_H