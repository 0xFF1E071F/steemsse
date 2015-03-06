#pragma once
#ifndef SSESCP_H
#define SSESCP_H

#include <easystr.h>
#include <conditions.h>
#include "SSEParameters.h"

#if defined(SSE_DRIVE_SOUND) && defined(SSE_STRUCTURE_SSEDEBUG_OBJ)
#ifdef WIN32
#include <dsound.h>
#endif
#endif

#include "SSEDma.h"
#include "SSEDrive.h"
#include "SSEPsg.h"
#include "SSEWD1772.h"

#if defined(SSE_DISK_GHOST)
#include "SSEGhostDisk.h"
#endif

#if defined(SSE_DISK_STW)
#include "SSESTW.h"
#endif

#if defined(SSE_SCP)//TODO
/* SCP images are big, we must decide if we load in memory or not

*/
struct TScpImageInfo {
  BYTE DiskIn;
  EasyStr File; // name of the actual file (may be tmp)
};

struct TScp {

  TScpImageInfo ImageInfo[2];
  int InsertDisk(int drive,char* File);

};

extern TScp Scp;

#endif//scp

#endif//SSESCP_H
