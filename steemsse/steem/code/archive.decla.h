#pragma once
#ifndef ARCHIVE_DECLA_H
#define ARCHIVE_DECLA_H

#ifdef UNIX
#include <unzip.h>
#endif

#ifdef WIN32
/*  from "unzip_win32.h"
    unzipd32.dll seems to be an odd library, couldn't find a h file, and when
    looking for DLL functions, very few references, and they seems to be related
    to Steem!
*/
struct PackStruct{
  char InternalUse[12];    // Used internally by the dll
  int Time;                // File time
  int Size;                // File size
  int CompressSize;        // Size in zipfile
  int HeaderOffset;        // File offset in zip
  long Crc;                // CRC, sort of checksum
  char FileName[260];      // File name
  WORD PackMethod;         /* 0=Stored, 1=Shrunk, 2=Reduced 1, 3=Reduced 2, 4=Reduced 3, 5=Reduced 4,
                              6=Imploded,7=Tokenized (format does not exist), 8=Deflated,
                              More than 8=Unknown method.
                              For this DLL this number can only be 0, 8, or more than 8
                              */
  WORD Attr;               // File attributes
  WORD Flags;              // Only used by ARJ unpacker (LOBYTE: arj_flags, HIBYTE: file_type)
};

int (_stdcall *GetFirstInZip)(char*,PackStruct*); //find out what files are in the ZIP file (first file)
int (_stdcall *GetNextInZip)(PackStruct*);        //get next file in ZIP
void (_stdcall *CloseZipFile)(PackStruct*);       //free buffers and close ZIP after GetFirstInZip()
BYTE (_stdcall *isZip)(char*);                    //determine if a file is a ZIP file
int (_stdcall *UnzipFile)(char*,char*,WORD,long,void*,long);        //unzipping

#define UNZIP_Ok           0               // Unpacked ok
#define UNZIP_CRCErr       1               // CRC error
#define UNZIP_WriteErr     2               // Error writing out file: maybe disk full
#define UNZIP_ReadErr      3               // Error reading zip file
#define UNZIP_ZipFileErr   4               // Error in zip structure
#define UNZIP_UserAbort    5               // Aborted by user
#define UNZIP_NotSupported 6               // ZIP Method not supported!
#define UNZIP_Encrypted    7               // Zipfile encrypted
#define UNZIP_InUse        -1              // DLL in use by other program!
#define UNZIP_DLLNotFound  -2              // DLL not loaded!

extern HINSTANCE hUnzip;

#endif//win32

#ifndef NO_RAR_SUPPORT
#define RAR_SUPPORT
#endif

#ifdef RAR_SUPPORT
#include <unrarlib/unrarlib/unrarlib.h>	
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VAR_UNRAR)
#include <UnRARDLL/unrar.h>
void LoadUnrarDLL();
#endif

#define ZIPPY_FAIL true
#define ZIPPY_SUCCEED 0

class zipclass{
private:
public:
#ifdef UNIX
  //use zlib
  unzFile uf;
  unz_global_info gi;
  char filename_inzip[256];
  unz_file_info fi;
#endif
#ifdef WIN32
  PackStruct PackInfo;
#endif
#ifdef RAR_SUPPORT
  ArchiveList_struct *rar_list,*rar_current;
#endif
#if defined(STEVEN_SEAGAL) && defined(SS_VAR_UNRAR)
  // persistent, good idea?
  RAROpenArchiveData ArchiveData;
  RARHeaderData HeaderData; // no Ex?
  HANDLE hArcData;
#endif
  bool is_open;
  int current_file_n;
  int current_file_offset;
  WORD attrib;
  long crc;
  int err;
  char type[12];
  EasyStr last_error;
  zipclass();
  ~zipclass(){ };

  bool first(char*);
  bool next();
  bool close();
  char* filename_in_zip();
  void list_contents(char*,EasyStringList*,bool=false);
  bool extract_file(char*,int,char*,bool=false,DWORD=0);
};

extern zipclass zippy; 

#endif//ARCHIVE_DECLA_H