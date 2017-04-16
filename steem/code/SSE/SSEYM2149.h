#pragma once
#ifndef SSEYM2149_H
#define SSEYM2149_H

#if defined(SSE_YM2149_OBJECT)

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TYM2149 {
  enum {NO_VALID_DRIVE=0xFF,ENVELOPE_MASK=31};
  // DATA
#if defined(SSE_YM2149_DYNAMIC_TABLE)
  WORD *p_fixed_vol_3voices;
#endif
#if defined(SSE_YM2149_MAMELIKE)
  enum {NUM_CHANNELS=3};
  int m_count[NUM_CHANNELS];
  int m_count_noise;
  int m_count_env;
  int m_rng;
  int m_env_volume;
  int m_cycles; // added
  BYTE m_output[NUM_CHANNELS];
  BYTE m_env_step_mask;
  char m_env_step;
  BYTE m_hold,m_alternate,m_attack,m_holding;
  BYTE m_prescale_noise;
  BYTE m_vol_enabled[NUM_CHANNELS];
#if defined(SSE_YM2149_MAMELIKE_AVG_SMP)
  BYTE m_oversampling_count;
#endif
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
#if defined(SSE_YM2149_DYNAMIC_TABLE)
  void FreeFixedVolTable();
  bool LoadFixedVolTable();
#endif
#if defined(SSE_YM2149_MAMELIKE)
  void Reset();
  void psg_write_buffer(DWORD); // mix MAME+Steem's psg_write_buffer()
#endif
};

#pragma pack(pop)

#endif

#endif//SSEYM2149_H
