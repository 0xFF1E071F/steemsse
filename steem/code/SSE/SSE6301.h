#pragma once
#ifndef SSE6301_H
#define SSE6301_H

#if defined(SS_IKBD_6301)

extern "C" {
#include "..\..\3rdparty\6301\6301.h"
}

struct THD6301 {
  THD6301();
  ~THD6301();
  Init();
  BOOL Initialised; // we do need a rom
  BOOL RunThisHbl; 
  BOOL Crashed; // oops
};

extern THD6301 HD6301;

#endif//#if defined(SS_IKBD_6301)

#endif//#ifndef SSE6301_H