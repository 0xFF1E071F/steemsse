#pragma once
#ifndef FLOPPYDRIVE_DECLA_H
#define FLOPPYDRIVE_DECLA_H

//SS this is ambiguous in Steem, class is 'image' but object is 'drive'

#include <stdio.h>
#include <easystr.h>
#include "fdc.decla.h"//FLOPPY_MAX_TRACK_NUM?

#define FIMAGE_OK                  0
#define FIMAGE_WRONGFORMAT         1
#define FIMAGE_CANTOPEN            2
#define FIMAGE_FILEDOESNTEXIST     3
#define FIMAGE_NODISKSINZIP        4
#define FIMAGE_CORRUPTZIP          5
#define FIMAGE_DIMNOMAGIC          6
#define FIMAGE_DIMTYPENOTSUPPORTED 7


#if defined(SSE_COMPILER_STRUCT_391)

#pragma pack(push, STRUCTURE_ALIGNMENT)

typedef struct{
    BYTE Track,Side,SectorNum,SectorLen,CRC1,CRC2;
}FDC_IDField;

typedef struct{
  int BytesPerSector,Sectors,SectorsPerTrack,Sides;
}BPBINFO;

class TFloppyImage
{
  //DATA
private:
  EasyStr ImageFile,MSATempFile,ZipTempFile,FormatTempFile;
public:
  EasyStr DiskName,DiskInZip;
  FILE *f,*Format_f;
  BYTE *PastiBuf;
  DWORD DiskFileLen;
  DWORD STT_TrackStart[2][FLOPPY_MAX_TRACK_NUM+1];
  int FormatMostSectors,FormatLargestSector;
  int PastiBufLen;
  WORD STT_TrackLen[2][FLOPPY_MAX_TRACK_NUM+1];  
  short BytesPerSector,Sides,SectorsPerTrack,TracksPerSide;
  bool STT_File,PastiDisk;
#if defined(SSE_DISK_392)
   BYTE Id;
   bool m_DiskInDrive;
#else
#if defined(SSE_DISK_CAPS)
  bool IPFDisk,CTRDisk;
#endif
#if defined(SSE_DISK_SCP)
  bool SCPDisk;
#endif
#if defined(SSE_DISK_STW)
  bool STWDisk;
#endif
#if defined(SSE_DISK_HFE)
  bool HFEDisk;
#endif
#endif
  bool ReadOnly;
  bool DIM_File,ValidBPB;
  bool TrackIsFormatted[2][FLOPPY_MAX_TRACK_NUM+1];
  bool WrittenTo;
  //FUNCTIONS
  TFloppyImage()             { f=NULL;Format_f=NULL;PastiDisk=
#if defined(SSE_DISK_392)
    m_DiskInDrive=
#else
#if defined(SSE_DISK_CAPS)    
    IPFDisk=CTRDisk=
#endif
#if defined(SSE_DISK_SCP)
    SCPDisk=
#endif 
#if defined(SSE_DISK_STW)
    STWDisk=
#endif 
#if defined(SSE_DISK_HFE)
    HFEDisk=
#endif 
#endif
    0;PastiBuf=NULL;RemoveDisk();}

  ~TFloppyImage()            { RemoveDisk(); }

#if defined(SSE_DISK_PASTI_NO_RESET) || defined(SSE_DISK_GHOST)
  EasyStr GetImageFile() {return ImageFile;}
#endif
  int SetDisk(EasyStr,EasyStr="",BPBINFO* = NULL,BPBINFO* = NULL);
  EasyStr GetDisk()  { return ImageFile; }
  bool ReinsertDisk();
  void RemoveDisk(bool LoseChanges=0);
  bool DiskInDrive() { 
#if defined(SSE_DISK_392) // SF314 isn't known here
    return m_DiskInDrive;  
#else
    return f!=NULL || PastiDisk 
#if defined(SSE_DISK_CAPS)
    || IPFDisk || CTRDisk
#endif    
#if defined(SSE_DISK_SCP)
    || SCPDisk
#endif   
#if defined(SSE_DISK_STW)
    || STWDisk
#endif     
#if defined(SSE_DISK_HFE)
    || HFEDisk
#endif
#endif
    ; }
  bool NotEmpty() { return DiskInDrive(); }
  bool Empty()       { return DiskInDrive()==0; }
  bool IsMSA()       { return MSATempFile.NotEmpty(); }
  bool IsZip()       { return ZipTempFile.NotEmpty(); }
  bool BeenFormatted() { return Format_f!=NULL; }
  bool NotBeenFormatted() { return Format_f==NULL; }
  bool SeekSector(int,int,int,bool=0);
  long GetLogicalSector(int,int,int,bool=0);
  int GetIDFields(int,int,FDC_IDField*);
  int GetRawTrackData(int,int);
  bool OpenFormatFile();
  bool ReopenFormatFile();
};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#else

typedef struct{
    BYTE Track,Side,SectorNum,SectorLen,CRC1,CRC2;
}FDC_IDField;

typedef struct{
  int BytesPerSector,Sectors,SectorsPerTrack,Sides;
}BPBINFO;

class TFloppyImage
{
private:
  EasyStr ImageFile,MSATempFile,ZipTempFile,FormatTempFile;
public:
  TFloppyImage()             { f=NULL;Format_f=NULL;PastiDisk=
#if defined(SSE_DISK_CAPS)    
    IPFDisk=CTRDisk=
#endif
#if defined(SSE_DISK_SCP)
    SCPDisk=
#endif 
#if defined(SSE_DISK_STW)
    STWDisk=
#endif 
#if defined(SSE_DISK_HFE)
    HFEDisk=
#endif 
    0;PastiBuf=NULL;RemoveDisk();}

  ~TFloppyImage()            { RemoveDisk(); }


#if defined(SSE_DISK_PASTI_NO_RESET)
  EasyStr GetImageFile() {return ImageFile;}
#endif

  int SetDisk(EasyStr,EasyStr="",BPBINFO* = NULL,BPBINFO* = NULL);
  EasyStr GetDisk()  { return ImageFile; }
  bool ReinsertDisk();
  void RemoveDisk(bool LoseChanges=0);
  bool DiskInDrive() { return f!=NULL || PastiDisk 
#if defined(SSE_DISK_CAPS)
    || IPFDisk || CTRDisk
#endif    
#if defined(SSE_DISK_SCP)
    || SCPDisk
#endif   
#if defined(SSE_DISK_STW)
    || STWDisk
#endif     
#if defined(SSE_DISK_HFE)
    || HFEDisk
#endif
    ; }
  bool NotEmpty() { return DiskInDrive(); }
  bool Empty()       { return DiskInDrive()==0; }
  bool IsMSA()       { return MSATempFile.NotEmpty(); }
  bool IsZip()       { return ZipTempFile.NotEmpty(); }
  bool BeenFormatted() { return Format_f!=NULL; }
  bool NotBeenFormatted() { return Format_f==NULL; }
  bool SeekSector(int,int,int,bool=0);
  long GetLogicalSector(int,int,int,bool=0);
  int GetIDFields(int,int,FDC_IDField*);
  int GetRawTrackData(int,int);
  bool OpenFormatFile();
  bool ReopenFormatFile();

  FILE *f,*Format_f;
  bool ReadOnly;
  short BytesPerSector,Sides,SectorsPerTrack,TracksPerSide;
  EasyStr DiskName,DiskInZip;
  DWORD DiskFileLen;

  BYTE *PastiBuf; // SS same for IPF?
  int PastiBufLen;
  bool STT_File,PastiDisk;
#if defined(SSE_DISK_CAPS)
  bool IPFDisk,CTRDisk;
#endif
#if defined(SSE_DISK_SCP)
  bool SCPDisk;
#endif
#if defined(SSE_DISK_STW)
  bool STWDisk;
#endif
#if defined(SSE_DISK_HFE)
  bool HFEDisk;
#endif
  DWORD STT_TrackStart[2][FLOPPY_MAX_TRACK_NUM+1];
  WORD STT_TrackLen[2][FLOPPY_MAX_TRACK_NUM+1];

  bool DIM_File,ValidBPB;

  bool TrackIsFormatted[2][FLOPPY_MAX_TRACK_NUM+1];
  int FormatMostSectors,FormatLargestSector;

  bool WrittenTo;	//SS: WrittenTo just means 'dirty'
};


#endif//#if defined(SSE_COMPILER_STRUCT_391)


extern TFloppyImage FloppyDrive[2];
extern bool FloppyArchiveIsReadWrite;

#endif//#define FLOPPYDRIVE_DECLA_H