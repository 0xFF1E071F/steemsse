#pragma once
#ifndef SSSTF_H
#define SSSTF_H

#if defined(STEVEN_SEAGAL) && defined(SS_STF)

// starting with 0 is easier for GUI; default is STE
// we list them all even if not defined (TODO?)
enum ESTModels {STE,STF,MEGASTF,STF8MHZ,SS_STF_ST_MODELS}; 

#if defined(SS_TOS_WARNING1)
void CheckSTTypeAndTos();
#endif

int SwitchSTType(int new_type); // adapt to new machine

extern char* st_model_name[];
#endif

#endif// SSSTF_H