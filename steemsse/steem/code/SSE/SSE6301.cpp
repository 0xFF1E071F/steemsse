#if defined(SS_IKBD_6301)

#define LOGSECTION LOGSECTION_INIT//SS

#include "SSEOption.h"

// note most useful code is in 3rdparty folder '6301'

THD6301 HD6301; // singleton


THD6301::THD6301() {
}


THD6301::~THD6301() {
  hd6301_destroy(); // calling the 6301 function
}


void THD6301::Init() { // called in 'main'
  Initialised=Crashed=0;
  if(hd6301_init()) // calling the 6301 function
  {
    HD6301_OK=Initialised=1;
    TRACE_LOG("HD6301 emu initialised\n");
  }
  else
  {
    TRACE_LOG("HD6301 emu NOT initialised\n");
    HD6301EMU_ON=0;
    ///Alert("no 6301","Error");
   /// alert.ask(XD,"no 6301",Caption,T("Okay"),default_option,0);
  }
/*
  int a=0;
  int b=7;
  try {
  int c=b/a;
  }
  catch(...)
  {
    TRACE("ho ho\n");
  }
  */
}

#undef LOGSECTION

#endif