#include "../pch.h"
#include "SSE.h"

#if defined(SSE_STF)
#include "SSESTF.h"
//TODO this too belongs in config
#if defined(SSE_STF_LACESCAN)
char* st_model_name[]={"STE","STF","Mega ST4","STF Overscan"};
#else
char* st_model_name[]={"STE","STF","Mega ST4"};
#endif
#endif//#if defined(SSE_STF)
