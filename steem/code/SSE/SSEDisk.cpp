#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#include "../pch.h"

#include <fdc.decla.h>

#include "SSEDebug.h"
#include "SSEDisk.h"
#include "SSEDrive.h"
#include "SSEFloppy.h"

#if defined(SSE_DISK)


#if defined(SSE_DISK1)

TDisk::TDisk() {
#ifdef SSE_DEBUG
  Id=0xFF;
  current_byte=0xFFFF;
#endif
  Init();
}

void TDisk::Init() {
  TrackBytes=DRIVE_BYTES_ROTATION; // 6256+20 default for ST/MSA/DIM
}

#endif




#endif//disk

#endif//seagal