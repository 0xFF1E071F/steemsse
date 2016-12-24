#pragma once
#ifndef SSSTF_H
#define SSSTF_H

// starting with 0 is easier for GUI; default is STE

enum ESTModels {STE,STF,MEGASTF,N_ST_MODELS}; 

#if defined(SSE_STF)

extern char* st_model_name[];

#endif

#endif// SSSTF_H