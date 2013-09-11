#pragma once
#ifndef SSESDL_H
#define SSESDL_H

#if defined(SS_SDL) //&& !defined(SS_SDL_DEACTIVATE)

#if defined(WIN32) && defined(SS_DELAY_LOAD_DLL)
#include <SDL-WIN/include/SDL.h> 
// DLL is 'delay loaded'
#ifdef _MSC_VER
#pragma comment(lib, "../../3rdparty/SDL-WIN/lib/x86/SDL.lib")
#pragma comment(lib, "../../3rdparty/SDL-WIN/lib/x86/SDLmain.lib")
#ifndef SS_VS2012_DELAYDLL
#pragma comment(linker, "/delayload:SDL.dll")
#endif
#endif
#ifdef __BORLANDC__
#pragma comment(lib, "../../3rdparty/SDL-WIN/bcclib/SDL.lib")
#endif
#endif//win32
#endif //!defined(SS_SDL_DEACTIVATE)

#if defined(STEVEN_SEAGAL) && defined(SS_SDL) && !defined(SS_SDL_DEACTIVATE)

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

#endif//!defined(SS_SDL_DEACTIVATE)

#endif//SSESDL_H