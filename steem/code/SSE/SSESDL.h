#pragma once
#ifndef SSESDL_H
#define SSESDL_H

#if defined(SSE_SDL) //&& !defined(SSE_SDL_DEACTIVATE)

#include <SDL-WIN/include/SDL.h> 

#if defined(WIN32) && defined(SSE_DELAY_LOAD_DLL)

// DLL is 'delay loaded'
#ifdef _MSC_VER
#pragma comment(lib, "../../3rdparty/SDL-WIN/lib/x86/SDL.lib")
#pragma comment(lib, "../../3rdparty/SDL-WIN/lib/x86/SDLmain.lib")
#ifndef SSE_VS2012_DELAYDLL
#pragma comment(linker, "/delayload:SDL.dll")
#endif
#endif
#ifdef __BORLANDC__
#pragma comment(lib, "../../3rdparty/SDL-WIN/bcclib/SDL.lib")
#endif
#endif//win32
#endif //!defined(SSE_SDL_DEACTIVATE)

#if defined(STEVEN_SEAGAL) && defined(SSE_SDL) && !defined(SSE_SDL_DEACTIVATE)

struct TSDL {
  bool Available;
  bool InUse;
  SDL_Surface *Surface;
  TSDL();
  ~TSDL();
  bool Init();

  bool EnterSDLVideoMode();
  void LeaveSDLVideoMode();

  void Lock();
  void Blit();
  void Unlock();


};

extern TSDL SDL;

#endif//!defined(SSE_SDL_DEACTIVATE)

#endif//SSESDL_H