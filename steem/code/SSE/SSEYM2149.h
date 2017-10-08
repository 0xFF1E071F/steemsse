#pragma once
#ifndef SSEYM2149_H
#define SSEYM2149_H

#if defined(SSE_YM2149)

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
#include "dsp/FIR-filter-class/filt.h" // 3rd party
#endif

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TYM2149 {
  enum {NO_VALID_DRIVE=0xFF,ENVELOPE_MASK=31};
  // DATA
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  Filter* AntiAlias;
#endif
#if defined(SSE_YM2149_DYNAMIC_TABLE)
  WORD *p_fixed_vol_3voices;
#endif
#if defined(SSE_YM2149_MAMELIKE)
  enum {NUM_CHANNELS=3};
  COUNTER_VAR m_cycles; // added

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  COUNTER_VAR time_at_vbl_start,time_of_last_sample;
  int frame_samples;
#endif

  int m_count[NUM_CHANNELS];
  int m_count_noise;
  int m_count_env;
  int m_rng;

  DWORD m_env_volume;
//  COUNTER_VAR m_cycles; // added
  BYTE m_output[NUM_CHANNELS];
  BYTE m_env_step_mask;
  char m_env_step;
  BYTE m_hold,m_alternate,m_attack,m_holding;
  BYTE m_prescale_noise;
  BYTE m_vol_enabled[NUM_CHANNELS];
#endif
#if defined(SSE_YM2149A)
  BYTE SelectedDrive; //0/1 (use Drive() to check validity)
  BYTE SelectedSide;  //0/1
#endif
  // FUNCTIONS
  TYM2149();
  ~TYM2149();
  BYTE Drive(); // may return NO_VALID_DRIVE
  BYTE PortA();
#if !defined(SSE_YM2149A)
  BYTE Side();
#endif
#if defined(SSE_DRIVE_FREEBOOT)
  void CheckFreeboot();
#endif
#if defined(SSE_YM2149_DYNAMIC_TABLE)
  void FreeFixedVolTable();
  bool LoadFixedVolTable();
#endif
#if defined(SSE_YM2149_MAMELIKE)
  void Reset();
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  void psg_write_buffer(DWORD to_t, bool vbl=false);
#else
  void psg_write_buffer(DWORD); // mix MAME+Steem's psg_write_buffer()
#endif
#endif
};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#endif

#endif//SSEYM2149_H
