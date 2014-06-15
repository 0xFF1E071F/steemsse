#pragma once
#ifndef SSEYM2149_H
#define SSEYM2149_H

#if defined(SSE_YM2149)
/*  In v3.5.1, object PSG is only used for drive management.
    Drive is 0 (A:) or 1 (B:), but if both relevant bits in
    PSG port A are set then no drive is selected ($FF).
*/

struct TYM2149 {
  enum {NO_VALID_DRIVE=0xFF};
  BYTE Drive(); // may return NO_VALID_DRIVE
  BYTE PortA();
#if !defined(SSE_YM2149A)
  BYTE Side();
#endif
#if defined(SSE_YM2149A)
  BYTE SelectedDrive; //0/1 (use Drive() to check validity
  BYTE SelectedSide;  //0/1
#endif
};

#endif//PSG

#endif//SSEYM2149_H
