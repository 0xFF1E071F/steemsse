#if defined(SSE_SDL) && !defined(SSE_SDL_DEACTIVATE)

#include "SSESDL.h"


TSDL SDL; // singleton


TSDL::TSDL() {
  Available=InUse=false;
  Surface=NULL;
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


bool TSDL::EnterSDLVideoMode() {
  ASSERT( Available );
  ASSERT( !InUse );
  if(FullScreen||SCANLINES_INTERPOLATED) 
    TRACE_LOG("No SDL because F%d SI%d\n",FullScreen,SCANLINES_INTERPOLATED);
  else if(SDL_Init(SDL_INIT_VIDEO)!=-1)
  {
    int w=draw_blit_source_rect.right-draw_blit_source_rect.left;
    int h=draw_blit_source_rect.bottom-draw_blit_source_rect.top;
    if(w==800||w==824)
      w=832; // all directx hacks backlash, this avoids Steem crash (temp)
    Uint32 flags=SDL_SWSURFACE;
    TRACE_LOG("SDL create surface F %X w %d h %d bpp %d\n",flags,w,h,BytesPerPixel);
    Surface = SDL_SetVideoMode(w,h,BytesPerPixel*8,flags);
    ASSERT( Surface );
    if(Surface)
    {
#if !defined(SSE_SDL_KEEP_DDRAW_RUNNING)
      Disp.Release(); // leave current rendering system (GDI or DD)
#endif
      InUse=true;
    }
#if defined(SSE_DEBUG)
    else
      TRACE_LOG("SDL error %s\n",SDL_GetError());
#endif
  }
  return InUse;
}


void TSDL::LeaveSDLVideoMode() {
  ASSERT( Available );
  if(Surface)
  {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    TRACE_LOG("SDL close window %s\n",SDL_GetError());
    Surface=0;
    InUse=false;
#if !defined(SSE_SDL_KEEP_DDRAW_RUNNING)
    Disp.nUseMethod=0;
    Disp.Init();
#endif
  }
}

#undef LOGSECTION //init


void TSDL::Lock() {
  SDL_LockSurface(Surface);
  draw_mem=(BYTE*)Surface->pixels; 
}


void TSDL::Blit(){
  SDL_UpdateRects(SDL.Surface,1,&SDL.Surface->clip_rect);
}


void TSDL::Unlock(){
  SDL_UnlockSurface(Surface);
}



#endif//#if defined(SSE_SDL) && !defined(SSE_SDL_DEACTIVATE)