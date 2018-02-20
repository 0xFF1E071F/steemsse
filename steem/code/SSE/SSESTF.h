#pragma once
#ifndef SSSTF_H
#define SSSTF_H

// starting with 0 is easier for GUI; default is STE
#if defined(SSE_STF_HW_OVERSCAN)
enum ESTModels {STE,STF,
#if defined(SSE_STF_MEGASTF)
MEGASTF,
#endif
#if defined(SSE_STF_LACESCAN)
STF_LACESCAN,
#endif
#if defined(SSE_STF_AUTOSWITCH)
STF_AUTOSWITCH,
#endif
N_ST_MODELS}; 
#else
enum ESTModels {STE,STF,MEGASTF,N_ST_MODELS}; 
#endif

#if defined(SSE_STF)

extern char* st_model_name[];

#endif

#endif// SSSTF_H