#pragma once
#ifndef SSSTF_H
#define SSSTF_H

#if defined(STEVEN_SEAGAL) && defined(SS_STF)

// starting with 0 is easier for GUI; default is STE
enum ESTModels {STE,STF,MEGASTF,SS_STF_ST_MODELS}; 

int SwitchSTType(int new_type); // adapt to new machine

//extern char st_model_name[SS_STF_ST_MODELS][sizeof("Mega ST4")];
extern char* st_model_name[];
#endif

#endif// SSSTF_H