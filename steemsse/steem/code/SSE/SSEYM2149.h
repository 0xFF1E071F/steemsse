#pragma once
#ifndef SSEYM2149_H
#define SSEYM2149_H

#if defined(SSE_YM2149_OBJECT)

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TYM2149 {
  enum {NO_VALID_DRIVE=0xFF};
#if defined(SSE_YM2149_DYNAMIC_TABLE)
  WORD *p_fixed_vol_3voices;
#endif
#if defined(SSE_YM2149A)
  BYTE SelectedDrive; //0/1 (use Drive() to check validity
  BYTE SelectedSide;  //0/1
#endif
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
};

#pragma pack(pop)

#endif

#endif//SSEYM2149_H
