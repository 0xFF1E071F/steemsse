#pragma once
#ifndef SSSTF_H
#define SSSTF_H

#if defined(STEVEN_SEAGAL) && defined(SS_STF)

// starting with 0 is easier for GUI; default is STE
enum ESTModels {STE,STF,STF2,MEGASTF,SS_STF_ST_MODELS}; 

int SwitchSTType(int new_type); // adapt to new machine

#endif

#endif// SSSTF_H