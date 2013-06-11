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





TSDL::EnterSDLVideoMode() {
/*
  ASSERT( Available );
  TRACE("TSDL::EnterSDLVideoMode()\n");
  Disp.Release(); // leave current rendering system (GDI or DD)
  VERIFY( SDL_Init(SDL_INIT_VIDEO) != -1 );
//  if(!Surface)
  Surface = SDL_SetVideoMode(800, 600, 16, SDL_SWSURFACE);
  ASSERT( Surface );

*/
/*
The framebuffer surface, or NULL if it fails. 
The surface returned is freed by SDL_Quit() and should nt be freed
 by the caller.
...
You should free all surfaces except the display surface before calling
SDL_Quit().  It actually should be safe to free that one too, since SDL
checks internally and doesn't do anything if you try.

*/


}

///extern int nUseMethod;


TSDL::LeaveSDLVideoMode() {
/*
  ASSERT( Available );
  TRACE("TSDL::LeaveSDLVideoMode()\n");
//  ASSERT( Surface );
  if(Surface)
  {
   // TRACE("refcount %d\n",Surface->refcount);  // 1
   // SDL_FreeSurface(Surface); //
   // TRACE("refcount %d\n",Surface->refcount); // 1: no change?
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    Surface=0;
//    TRACE("SDL error %s\n",SDL_GetError());
    Disp.nUseMethod=0;
    Disp.Init();
  }
//  ASSERT( !Surface ); asserts
*/
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

#undef LOGSECTION

#endif