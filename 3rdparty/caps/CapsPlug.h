#ifndef CAPSPLUG_H
#define CAPSPLUG_H

#include "CapsFDC.h"

#ifdef __cplusplus

// library function pointers
struct CapsProc {
  LPCSTR name;
  FARPROC proc;
};

typedef SDWORD (__cdecl *CAPSHOOKN)(...);
typedef PCHAR  (__cdecl *CAPSHOOKS)(...);

extern "C" {
#endif

SDWORD CapsInit(LPCTSTR lib);
SDWORD CapsExit();
SDWORD CapsAddImage();
SDWORD CapsRemImage(SDWORD id);
SDWORD CapsLockImage(SDWORD id, PCHAR name);
SDWORD CapsLockImageMemory(SDWORD id, PUBYTE buffer, UDWORD length, UDWORD flag);
SDWORD CapsUnlockImage(SDWORD id);
SDWORD CapsLoadImage(SDWORD id, UDWORD flag);
SDWORD CapsGetImageInfo(PCAPSIMAGEINFO pi, SDWORD id);
SDWORD CapsLockTrack(PCAPSTRACKINFO pi, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD flag);
SDWORD CapsUnlockTrack(SDWORD id, UDWORD cylinder, UDWORD head);
SDWORD CapsUnlockAllTracks(SDWORD id);
PCHAR  CapsGetPlatformName(UDWORD pid);
//SS those were missing:
UDWORD CapsFdcGetInfo(SDWORD iid, PCAPSFDC pc, SDWORD ext);
SDWORD CapsFdcInit(PCAPSFDC pc);
void CapsFdcReset(PCAPSFDC pc);
void CapsFdcEmulate(PCAPSFDC pc, UDWORD cyclecnt);
UDWORD CapsFdcRead(PCAPSFDC pc, UDWORD address);
void   CapsFdcWrite(PCAPSFDC pc, UDWORD address, UDWORD data);
SDWORD CapsFdcInvalidateTrack(PCAPSFDC pc, SDWORD drive);
SDWORD CapsGetInfo(PVOID pinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD inftype, UDWORD infid);
#ifdef __cplusplus
}
#endif

#endif
