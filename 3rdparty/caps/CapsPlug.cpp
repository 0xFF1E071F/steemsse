// Simple caps plug-in support on Win32, should fit most users
//
// You may want to link with the libray instead, if your product fully complies
// with the license.
// If using the library directly without the plug-in, define CAPS_USER, and include CapsLib.h
// functions are the same with the plug-in, but their name start with "CAPS", not "Caps"
// CAPSInit does not have any parameters, CapsInit gets the library name
//
// Comment out stdafx.h if your project does not use the MSVC precompiled headers
//
// www.caps-project.org

/*  SS Compiles as a distinct object.
    The available CapsPlug was outdated, so it has been completed to interface
    Steem SSE with the fine low-level (bits) WD1772 emu present in the DLL.
*/

//#include "stdafx.h" 

#include "../steem/code/SSE/SSE.h"
#include "../steem/code/SSE/SSEDecla.h" // for windows.h
#include "../steem/code/SSE/SSEDebug.h" // for our debug facilities

#include "ComType.h"
#include "CapsAPI.h"
#include "CapsPlug.h"

HMODULE capi=NULL;

CapsProc cpr[]= {
  "CAPSInit", NULL,
  "CAPSExit", NULL,
  "CAPSAddImage", NULL,
  "CAPSRemImage", NULL,
  "CAPSLockImage", NULL,
  "CAPSUnlockImage", NULL,
  "CAPSLoadImage", NULL,
  "CAPSGetImageInfo", NULL,
  "CAPSLockTrack", NULL,
  "CAPSUnlockTrack", NULL,
  "CAPSUnlockAllTracks", NULL,
  "CAPSGetPlatformName", NULL,
  "CAPSLockImageMemory", NULL,
  //SS those were missing:
  "CAPSFdcGetInfo", NULL, //13
  "CAPSFdcInit", NULL,
  "CAPSFdcReset", NULL,
  "CAPSFdcEmulate", NULL,
  "CAPSFdcRead", NULL,
  "CAPSFdcWrite", NULL,
  "CAPSFdcInvalidateTrack", NULL,
  "CAPSGetInfo",NULL,
  NULL, NULL
};

enum {ECapsFdcGetInfo=13,ECapsFdcInit,ECapsFdcReset,ECapsFdcEmulate,
ECapsFdcRead,ECapsFdcWrite,ECapsFdcInvalidateTrack,ECapsGetInfo}; //SS

// start caps image support
SDWORD CapsInit(LPCTSTR lib)
{
  TRACE("Init CAPS library %s\n",lib);
  if (capi)
    return imgeOk;

  capi=LoadLibrary(lib);
  if (!capi)
  {
  //  TRACE("Failed\n");
    return imgeUnsupported; //SS 0 = ERROR
  }
//  TRACE("OK\n");

  for (int prc=0; cpr[prc].name; prc++)
  {
    cpr[prc].proc=GetProcAddress(capi, cpr[prc].name);
//    TRACE("init fct %s %p\n",cpr[prc].name,cpr[prc].proc);
  }
  SDWORD res=cpr[0].proc ? CAPSHOOKN(cpr[0].proc)() : imgeUnsupported;

  return res;
}

// stop caps image support
SDWORD CapsExit()
{
  SDWORD res=cpr[1].proc ? CAPSHOOKN(cpr[1].proc)() : imgeUnsupported;

  if (capi) {
    TRACE("Releasing CAPS library...\n"); // will work only in IDE
    FreeLibrary(capi);
    capi=NULL;
  }

  for (int prc=0; cpr[prc].name; prc++)
    cpr[prc].proc=NULL;

  return res;
}

// add image container
SDWORD CapsAddImage()
{
  SDWORD res=cpr[2].proc ? CAPSHOOKN(cpr[2].proc)() : -1;

  return res;
}

// delete image container
SDWORD CapsRemImage(SDWORD id)
{
  SDWORD res=cpr[3].proc ? CAPSHOOKN(cpr[3].proc)(id) : -1;

  return res;
}

// lock image
SDWORD CapsLockImage(SDWORD id, PCHAR name)
{
  SDWORD res=cpr[4].proc ? CAPSHOOKN(cpr[4].proc)(id, name) : imgeUnsupported;

  return res;
}

// unlock image
SDWORD CapsUnlockImage(SDWORD id)
{
  SDWORD res=cpr[5].proc ? CAPSHOOKN(cpr[5].proc)(id) : imgeUnsupported;

  return res;
}

// load and decode complete image
SDWORD CapsLoadImage(SDWORD id, UDWORD flag)
{
  SDWORD res=cpr[6].proc ? CAPSHOOKN(cpr[6].proc)(id, flag) : imgeUnsupported;

  return res;
}

// get image information
SDWORD CapsGetImageInfo(PCAPSIMAGEINFO pi, SDWORD id)
{
  SDWORD res=cpr[7].proc ? CAPSHOOKN(cpr[7].proc)(pi, id) : imgeUnsupported;

  return res;
}

// load and decode track, or return with the cache
SDWORD CapsLockTrack(PCAPSTRACKINFO pi, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD flag)
{
  SDWORD res=cpr[8].proc ? CAPSHOOKN(cpr[8].proc)(pi, id, cylinder, head, flag) : imgeUnsupported;

  return res;
}

// remove track from cache
SDWORD CapsUnlockTrack(SDWORD id, UDWORD cylinder, UDWORD head)
{
  SDWORD res=cpr[9].proc ? CAPSHOOKN(cpr[9].proc)(id, cylinder, head) : imgeUnsupported;

  return res;
}

// remove all tracks from cache
SDWORD CapsUnlockAllTracks(SDWORD id)
{
  SDWORD res=cpr[10].proc ? CAPSHOOKN(cpr[10].proc)(id) : imgeUnsupported;

  return res;
}

// get platform name
PCHAR CapsGetPlatformName(UDWORD pid)
{
  PCHAR res=cpr[11].proc ? CAPSHOOKS(cpr[11].proc)(pid) : NULL;

  return res;
}

// lock memory mapped image
SDWORD CapsLockImageMemory(SDWORD id, PUBYTE buffer, UDWORD length, UDWORD flag)
{
  SDWORD res=cpr[12].proc ? CAPSHOOKN(cpr[12].proc)(id, buffer, length, flag) : imgeUnsupported;

  return res;
}

//SS additions to wield the WD1772 emu

UDWORD CapsFdcGetInfo(SDWORD iid, PCAPSFDC pc, SDWORD ext) {
  UDWORD res=cpr[ECapsFdcGetInfo].proc ? 
    CAPSHOOKN(cpr[ECapsFdcGetInfo].proc)(iid, pc, ext) : imgeUnsupported;

  return res;
}

SDWORD CapsFdcInit(PCAPSFDC pc) {
  SDWORD res=cpr[ECapsFdcInit].proc ? 
    CAPSHOOKN(cpr[ECapsFdcInit].proc)(pc) : imgeUnsupported;

  return res;
}

void CapsFdcReset(PCAPSFDC pc) {
  if(cpr[ECapsFdcReset].proc)
    CAPSHOOKN(cpr[ECapsFdcReset].proc)(pc);
}

void CapsFdcEmulate(PCAPSFDC pc, UDWORD cyclecnt) {
  if(cpr[ECapsFdcEmulate].proc)
    CAPSHOOKN(cpr[ECapsFdcEmulate].proc)(pc,cyclecnt);
}

UDWORD CapsFdcRead(PCAPSFDC pc, UDWORD address) {
  UDWORD res=cpr[ECapsFdcRead].proc ? 
    CAPSHOOKN(cpr[ECapsFdcRead].proc)(pc,address) : imgeUnsupported;

  return res;
}

void   CapsFdcWrite(PCAPSFDC pc, UDWORD address, UDWORD data) {
  if(cpr[ECapsFdcWrite].proc)
    CAPSHOOKN(cpr[ECapsFdcWrite].proc)(pc,address,data);
}

SDWORD CapsFdcInvalidateTrack(PCAPSFDC pc, SDWORD drive) {
  SDWORD res=cpr[ECapsFdcInvalidateTrack].proc ? 
    CAPSHOOKN(cpr[ECapsFdcInvalidateTrack].proc)(pc,drive) : imgeUnsupported;

  return res;
}

SDWORD CapsGetInfo(PVOID pinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD inftype, UDWORD infid) {
  SDWORD res=cpr[ECapsGetInfo].proc ? 
    CAPSHOOKN(cpr[ECapsGetInfo].proc)(pinfo,id,cylinder,head,inftype,infid) :
    imgeUnsupported;
  
  return res;
}
