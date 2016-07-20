#include "SSE.h"

#if defined(SSE_DISK)

#include "../pch.h"

#include <fdc.decla.h>

#include "SSEDebug.h"
#include "SSEDisk.h"
#include "SSEDrive.h"
#include "SSEFloppy.h"

#if defined(SSE_DISK_EXT) // available for all SSE versions
char *extension_list[]={ "","ST","MSA","DIM","STT","STX","IPF",
"CTR","STG","STW","PRG","TOS","SCP","HFE",""}; //need last?

char buffer[5]=".XXX";

char *dot_ext(int i) { // is it ridiculous? do we reduce or add overhead?
  strcpy(buffer+1,extension_list[i]);
  return buffer;
}

#endif

#if defined(SSE_DISK1)

TDisk::TDisk() {
#ifdef SSE_DEBUG
  Id=0xFF;
  current_byte=0xFFFF;
#endif
  Init();
}

void TDisk::Init() {
  TrackBytes=DRIVE_BYTES_ROTATION; // 6256+14 default for ST/MSA/DIM
}

#endif

#endif//disk

