#if defined(SS_IKBD_6301)

#include "SSEOption.h"

// note most of useful code is in 3rdparty folder

THD6301 HD6301; // singleton


THD6301::THD6301() {
}


THD6301::~THD6301() {
  hd6301_destroy(); // calling the 6301 function
}


THD6301::Init() { // called in 'main'
  Initialised=Crashed=0;
  if(hd6301_init()) // calling the 6301 function
  {
    Initialised=1;
    TRACE("HD6301 emu initialised\n");
  }
  else
  {
    TRACE("HD6301 emu NOT initialised\n");
    HD6301EMU_ON=0;
  }
}

#endif