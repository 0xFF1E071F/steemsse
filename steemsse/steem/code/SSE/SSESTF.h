#pragma once
#ifndef SSSTF_H
#define SSSTF_H

// starting with 0 is easier for GUI; default is STE
// we list them all even if not defined (TODO?)
enum ESTModels {STE,STF,MEGASTF,SSE_STF_ST_MODELS}; 

#if defined(SSE_STF)

#if defined(SSE_TOS_WARNING1) && !defined(SSE_TOS_WARNING1A)
void CheckSTTypeAndTos();
#endif
#if !defined(SSE_STF_383)
int SwitchSTType(int new_type); // adapt to new machine
#endif
extern char* st_model_name[];
#endif

#endif// SSSTF_H