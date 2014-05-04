/*  
*/

#pragma once
#ifndef WD1772_H
#define WD1772_H

#include <conditions.h>

/*
This is from the WD1772 doc:
Byte #     Meaning                |     Sector length code     Sector length
------     ------------------     |     ------------------     -------------
1          Track                  |     0                      128
2          Side                   |     1                      256
3          Sector                 |     2                      512
4          Sector length code     |     3                      1024
5          CRC byte 1             |
6          CRC byte 2             |

*/
struct TWD1772IDField {
  BYTE track;
  BYTE side;
  BYTE num;
  BYTE len;
  WORD CRC;
};

struct TWD1772 {
  // Official registers
  BYTE CR;  // command
  BYTE STR; // status
  BYTE TR;  // track
  BYTE SR;  // sector
  BYTE DR;  // data
  // Internal registers
#if defined(SSE_FDC_FORCE_INTERRUPT)
  BYTE InterruptCondition;
#endif
#if defined(SSE_FDC_INDEX_PULSE_COUNTER)
  BYTE IndexCounter;
#endif
  BYTE IORead(BYTE Line);
  void IOWrite(BYTE Line,BYTE io_src_b);
  BYTE CommandType(int command=-1); // I->IV

#if defined(SSE_DISK_GHOST)
  BYTE CommandWasIntercepted;
#endif

#if defined(SSE_FDC_RESET)
  void Reset(bool Cold);
#endif

#if defined(SSE_DISK_STW)

  WORD ByteOfNextID(WORD current_byte);

  void  WriteCR(BYTE io_src_b); // a temporary solution...
#endif

#if defined(SSE_DEBUG) || defined(SSE_OSD_DRIVE_LED)
/*  This is useful for OSD: if we're writing then we need to display a red
    light (green when reading). This is used by pasti & IPF.
*/
  int WritingToDisk();
#endif
#if defined(SSE_DEBUG)
  void TraceStatus();
#endif
};

#endif//#ifndef WD1772_H