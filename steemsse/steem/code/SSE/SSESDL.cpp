#if defined(SS_SDL) && !defined(SS_SDL_DEACTIVATE)

#include "SSESDL.h"


TSDL SDL; // singleton


TSDL::TSDL() {
  Available=InUse=false;
  Surface=0;
}


TSDL::~TSDL() {
  if(InUse)
    SDL_Quit();
}


#define LOGSECTION LOGSECTION_INIT


bool TSDL::Init() {
  Available=true;
  try{
    if(SDL_Init(SDL_INIT_EVERYTHING)==-1)
      Available=false;
    else
    {
      SDL_Quit();
    }
  }
  catch(...)
  {
    TRACE_LOG("no SDL.DLL\n");
    Available=false;
  }
  return Available;
}


void TSDL::EnterSDLVideoMode() {
  ASSERT( Available );
  if(SDL_Init(SDL_INIT_VIDEO)!=-1)
  {
    Surface = SDL_SetVideoMode(800, 600, 16, SDL_SWSURFACE); //TODO parameters
    if(Surface)
    {
#if !defined(SS_SDL_KEEP_DDRAW_RUNNING)
      Disp.Release(); // leave current rendering system (GDI or DD)
#endif
      InUse=true;
    }
  }
  TRACE_LOG("SDL Init and create surface %s\n",SDL_GetError());
}


void TSDL::LeaveSDLVideoMode() {
  ASSERT( Available );
  if(Surface)
  {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    TRACE_LOG("SDL close window %s\n",SDL_GetError());
    Surface=0;
    InUse=false;
#if !defined(SS_SDL_KEEP_DDRAW_RUNNING)
    Disp.nUseMethod=0;
    Disp.Init();
#endif
  }
}

#undef LOGSECTION //init


  void TSDL::Lock();
  void TSDL::Blit();
  void TSDL::Unlock();



#endif//#if defined(SS_SDL) && !defined(SS_SDL_DEACTIVATE)