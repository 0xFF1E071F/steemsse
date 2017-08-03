//#include "../pch.h"
#include "SSE.h"
#include "SSESTF.h"

#if defined(SSE_STF)
//TODO this too belongs in config

char* st_model_name[]={"STE","STF"
#if defined(SSE_STF_MEGASTF)
,"Mega ST4"
#endif
#if defined(SSE_STF_LACESCAN)
,"STF LaceScan"
#endif
#if defined(SSE_STF_AUTOSWITCH)
,"STF AutoSwitch"
#endif
};

#endif//#if defined(SSE_STF)
