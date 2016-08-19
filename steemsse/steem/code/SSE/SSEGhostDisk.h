#pragma once
#ifndef SSE_GHOSTDISK_H
#define SSE_GHOSTDISK_H

/*
    Version 1.0
    Version 1.0 of GhostDisk was written to manage STG files in v1.0 format.
    This feature should be available in Steem 3.7.

Interface:

    The object reads or writes one sector in one go.
 
    The memory buffer is handled by the object, with direct access: 
    BYTE* SectorData.

    The object can manage max. one image at a time. 

    Open(char *path) 

    Opens the file specified in path.
    If the file didn't exist, it creates it.
    Returns false on failure (not a STW file).

    ReadSector(TWD1772IDField *IDField)

    If the image doesn't hold the sector, returns 0.
    If the image holds the sector, copies it in data buffer (SectorData),
    and returns #bytes read (should be 512).

    WriteSector(TWD1772IDField *IDField) 

    Adds a sector to the image or rewrite an existing sector.
    Structures to fill in are defined below.
    The data to write is in SectorData.

    Close()

    Closes current file. 
    This function is called by Open() and at object destruction, 
    just in case.   
    Allocated memory is freed.
*/

#if defined(SSE_DISK_GHOST)

#include "SSEWD1772.h"

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TGhostDisk {
  //DATA
  BYTE *SectorData; // memory allocation is handled by object
  FILE *fCurrentImage;
  WORD Version;  // version of the STG file (we write $100)
  WORD nRecords;
  WORD SectorBytes;
  TWD1772IDField CurrentIDField;
  // interface
  bool Open(char *path);
  void Close();
  WORD ReadSector(TWD1772IDField *IDField);
  void WriteSector(TWD1772IDField *IDField); 
  // other functions
  TGhostDisk();
  ~TGhostDisk();
  void Init();
  void Reset();
  bool FindIDField(TWD1772IDField *IDField);
};

#pragma pack(pop)

#endif//#if defined(SSE_DISK_GHOST)

#endif//#ifndef SSE_GHOSTDISK_H
