#pragma once
#ifndef SSEDISK_H
#define SSEDISK_H

#if defined(SSE_DISK)

#include "SSEWD1772.h"

#if defined(SSE_DISK_IMAGETYPE) 

/*  v3.7.0 We separate disk image type in two:
    who's in charge (manager), and what kind of image we have (extension).
    We start enums at 1 because 0 is used to clear status.
    This info is recorded for each drive (it is a variable in SF314).
    Steem is Steem's original native emulation.
    WD1772 is also Steem's, written to run STW disk images.
*/
enum { MNGR_STEEM=1,MNGR_PASTI,MNGR_CAPS,MNGR_WD1772};
enum { EXT_ST=1,EXT_MSA,EXT_DIM,EXT_STT,EXT_STX,EXT_IPF,EXT_CTR,EXT_STW,
EXT_PRG,EXT_TOS};

  struct TImageType {
    BYTE Manager;
    BYTE Extension;
  };
#endif//defined(SSE_DISK_IMAGETYPE) 


//TODO move gap stuff from drive here


/* v3.7.0 We create a new object 'Disk', as global duet for
   floppy drives inserted in drives.
   Global to avoid long references (drive[0].disk.etc.) and 
   because we prefer interdependent objects to a hierarchy. 
   This could change.
   Disk[0] is the one inserted in drive A:
   Disk[1] is the one inserted in drive B:
   Most functions are for "current track"
   Current track and side are drive's
*/

#if defined(SSE_DISK1)

  struct TDisk {
    BYTE Id; //0,1, same as drive
    WORD current_byte;
    WORD TrackBytes;
    TDisk();
    void Init();
  };

#endif


#endif//#if defined(SSE_DISK)

#endif//#ifndef SSEDISK_H