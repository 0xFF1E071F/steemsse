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

/*  We separate disk image type in two:
    who's in charge (manager), and what kind of image we have (extension).
    We start enums at 1 because 0 is used to clear status.
    This info is recorded for each drive (it is a variable in SF314).
    Steem is Steem's original native emulation (fdc.cpp)
    WD1772 is also Steem's, written to run STW disk images (SSEWD1772.cpp).
*/
enum { MNGR_STEEM=1,MNGR_PASTI,MNGR_CAPS,MNGR_WD1772,MNGR_PRG};//TODO put inside

struct TImageType {
  BYTE Manager;
  BYTE Extension;
};


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


struct TDisk {
  enum {TRACK_BYTES=DISK_BYTES_PER_TRACK};
  WORD current_byte;
  WORD TrackBytes;
  BYTE current_side; //of the image
  BYTE current_track; //of the image
  BYTE Id; //0,1, same as drive
  TDisk();
  void Init();

  // gaps
  WORD BytePositionOfFirstId();
  WORD BytesToID(BYTE &num);// if num=0, next ID
  WORD HblsPerSector();
  BYTE nSectors();
  void NextID(BYTE &RecordIdx,WORD &nHbls);
  BYTE PostIndexGap();
  BYTE PreDataGap();
  BYTE PostDataGap();
  WORD PreIndexGap();
  WORD RecordLength();
  BYTE SectorGap();
  WORD TrackGap();

};

#endif//#if defined(SSE_DISK)

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#endif//#ifndef SSEDISK_H