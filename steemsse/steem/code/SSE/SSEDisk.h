#pragma once
#ifndef SSEDISK_H
#define SSEDISK_H

#pragma pack(push, STRUCTURE_ALIGNMENT)

#if defined(SSE_DISK_EXT) // available for all SSE versions
enum { EXT_ST=1,EXT_MSA,EXT_DIM,EXT_STT,EXT_STX,EXT_IPF,EXT_CTR,EXT_STG,EXT_STW,
EXT_PRG,EXT_TOS,EXT_SCP,EXT_HFE};

extern char *extension_list[];

extern char *dot_ext(int i);

#define DISK_EXT_ST extension_list[EXT_ST]
#define DISK_EXT_MSA extension_list[EXT_MSA]
#define DISK_EXT_DIM extension_list[EXT_DIM]
#define DISK_EXT_STT extension_list[EXT_STT]
#define DISK_EXT_STX extension_list[EXT_STX]
#define DISK_EXT_IPF extension_list[EXT_IPF]
#define DISK_EXT_CTR extension_list[EXT_CTR] 
#define DISK_EXT_STG extension_list[EXT_STG]
#define DISK_EXT_STW extension_list[EXT_STW]
#define DISK_EXT_PRG extension_list[EXT_PRG]
#define DISK_EXT_TOS extension_list[EXT_TOS]
#define DISK_EXT_SCP extension_list[EXT_SCP]
#define DISK_EXT_HFE extension_list[EXT_HFE]

#endif//SSE_DISK_EXT

#if defined(SSE_DISK)

#include "SSEWD1772.h"

#if defined(SSE_DISK_IMAGETYPE) 

/*  We separate disk image type in two:
    who's in charge (manager), and what kind of image we have (extension).
    We start enums at 1 because 0 is used to clear status.
    This info is recorded for each drive (it is a variable in SF314).
    Steem is Steem's original native emulation (fdc.cpp)
    WD1772 is also Steem's, written to run STW disk images (SSEWD1772.cpp).
*/
enum { MNGR_STEEM=1,MNGR_PASTI,MNGR_CAPS,MNGR_WD1772};//TODO put inside

struct TImageType {
  BYTE Manager;
  BYTE Extension;
};

#endif


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
  WORD current_byte;
  WORD TrackBytes;
  BYTE Id; //0,1, same as drive
#if defined(SSE_DISK2)
  BYTE current_side; //used in HFE
  BYTE current_track;
#endif
  TDisk();
  void Init();
};

#endif

#endif//#if defined(SSE_DISK)

#pragma pack(pop)

#endif//#ifndef SSEDISK_H