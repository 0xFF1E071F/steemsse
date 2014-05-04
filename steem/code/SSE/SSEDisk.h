#pragma once
#ifndef SSEDISK_H
#define SSEDISK_H

#if defined(SS_DISK)

#if defined(SS_DISK_IMAGETYPE) 

/*  v3.7.0 We separate disk image type in two:
    who's in charge (manager), and what kind of image we have (extension).
    We start enums at 1 because 0 is used to clear status.
    This info is recorded for each drive (it is a variable in SF314).
*/
  enum { MNGR_STEEM=1,MNGR_PASTI,MNGR_CAPS,NMANAGERS };
  enum { EXT_ST=1,EXT_MSA,EXT_DIM,EXT_STT,EXT_STX,EXT_IPF,EXT_CTR,EXT_STW,NEXTENSIONS };

  struct TImageType {
    BYTE Manager;
    BYTE Extension;
  };
#endif//defined(SS_DISK_IMAGETYPE) 


//TODO move gap stuff from drive here

#endif//#if defined(SS_DISK)

#endif//#ifndef SSEDISK_H