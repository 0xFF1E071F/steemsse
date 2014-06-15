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
  TrackBytes=6256; // we insist :)
}


//TWD1772MfmDecoded 
WORD TDisk::GetByte(WORD position) {

// on STW this gets both clock and data byte

  WORD data;

  if(position==0xFFFF) // default parameter
    position=current_byte;
  ASSERT(IMAGE_STW);
  if(IMAGE_STW)
  {
    data=ImageSTW[DRIVE].GetMfmData(position);
  }
  return data;
}

#endif




#endif//disk

#endif//seagal