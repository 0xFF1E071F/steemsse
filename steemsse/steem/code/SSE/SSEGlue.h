#pragma once
#ifndef SSEGLUE_H
#define SSEGLUE_H

struct TGlue {

  TGlue();
  void Update();

  enum {FREQ_50,FREQ_60,FREQ_72,NFREQS};
  WORD DE_cycles[NFREQS];
  
};

extern TGlue Glue;

#endif//#ifndef SSEGLUE_H
