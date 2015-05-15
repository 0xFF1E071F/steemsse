///////////////////////////////////////////////////////////////////////////////
// ArchiveAccessSSE.cpp for Steem SSE
// based on:
//
// ArchiveAccessTest.cpp 
// Copyright 2004 X-Ways Technology AG
// Author: Björn Ganster
// Licensed under the LGPL
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "../ArchiveAccess/bgstr.h"
#include "../ArchiveAccess/ArchiveAccessBase.h"

#if defined(STEVEN_SEAGAL)
#include <SSE/SSE.h> // get switches
#if defined(SSE_VAR_ARCHIVEACCESS)
#include <SSE/SSEDebug.h>
#define LOGSECTION LOGSECTION_IMAGE_INFO
#if defined(BCC_BUILD)
#define max(a,b) ( (a)>(b)?(a):(b) ) //when 1st compiling as is
#endif


///////////////////////////////////////////////////////////////////////////////
// Settings

//#define UseStaticLinking
//#define DumpData
//#define DumpHex
#define UseStreams

///////////////////////////////////////////////////////////////////////////////
// Static library

#ifdef UseStaticLinking

#include "../ArchiveAccess/ArchiveAccess.h"

bool LoadArchiveAccessDll (const char*)
{
    return true;
}

#endif

///////////////////////////////////////////////////////////////////////////////
// Dynamically loaded library

#ifndef UseStaticLinking

#include "../ArchiveAccess/ArchiveAccessDynamic.h"

HINSTANCE hinstLib;

bool LoadArchiveAccessDll (const TCHAR* LibName)
{
    hinstLib = LoadLibrary(LibName); 
    if (hinstLib != NULL) {
        aaDetermineArchiveType = (paaDetermineArchiveType) GetProcAddress(hinstLib, "aaDetermineArchiveType");
//		aaInit = (paaInit) GetProcAddress(hinstLib, "aaInit");
        aaOpenArchiveFile = (paaOpenArchiveFile) GetProcAddress(hinstLib, "aaOpenArchiveFile");
        aaOpenArchive = (paaOpenArchive) GetProcAddress(hinstLib, "aaOpenArchive"); 
        aaGetFileCount = (paaGetFileCount) GetProcAddress(hinstLib, "aaGetFileCount");
        aaGetFileInfo = (paaGetFileInfo) GetProcAddress(hinstLib, "aaGetFileInfo"); 
        aaExtract = (paaExtract) GetProcAddress(hinstLib, "aaExtract");
        aaCloseArchive = (paaCloseArchive) GetProcAddress(hinstLib, "aaCloseArchive");

		bool t1 = (aaDetermineArchiveType != NULL);
		bool t2 = (aaOpenArchiveFile != NULL);
		bool t3 = (aaOpenArchive != NULL);
		bool t4 = (aaGetFileCount != NULL);
		bool t5 = (aaGetFileInfo != NULL);
		bool t6 = (aaExtract != NULL);
		bool t7 = (aaCloseArchive != NULL);
		return (t1 && t2 && t3 && t4 && t5 && t6 && t7);
    } else 
        return false;
}

void UnloadArchiveAccessDll/*Lib*/ ()
{
    FreeLibrary(hinstLib); 
}

#endif


///////////////////////////////////////////////////////////////////////////////
// Callback function for reading data

#ifdef UseStreams
HRESULT __stdcall readCallback (int StreamID, INT64 offset, UINT32 count, 
                                void* buf, UINT32* processedSize)
{
   #ifdef DumpData
      cout << "Reading " << count << " bytes from offset "
           << (unsigned long) offset << endl;
   #endif

   unsigned long ReadBytes;
   HANDLE handle = reinterpret_cast <HANDLE> (StreamID);
   long offsHi = (long) (offset >> 32);
   ASSERT(!offsHi);
   long  offsLo = (long) (offset & ((1 << 32) -1));

   SetFilePointer (handle, offsLo, &offsHi, FILE_BEGIN);
   int result = ReadFile(handle, buf, count, &ReadBytes, NULL);

   if (processedSize != NULL)
       (*processedSize) = ReadBytes;
   //ASSERT(count == ReadBytes); //it asserts on non 7z formats but seems to work
   //if (count != ReadBytes)  TRACE_LOG("ArchiveAccess requested %d read %d\n",count,ReadBytes);

#ifdef DumpHex
   DumpBuf ((char*) (buf), min (count, 32));
#endif
   //TRACE_LOG("StreamID %d buf %x count %d ReadBytes %d result %d\n",StreamID,buf,count,ReadBytes,result);
   if (result != 0)
     return S_OK;
   else
     return S_FALSE;
}

#endif

///////////////////////////////////////////////////////////////////////////////
// Callback function for writing data

HRESULT __stdcall writeCallback (int StreamID, INT64 offset, UINT32 count, 
                                 const void* buf, UINT32* processedSize)
{
#ifdef DumpData
  cout << "Writing " << count << " bytes" << endl;
#endif

  int result = 1;
  if (count != 0) {
    unsigned long procSize;
    HANDLE outFileHandle = reinterpret_cast <HANDLE> (StreamID);

    long offsHi = (long) (offset >> 32);
    long offsLo = (long) (offset & ((1 << 32) -1));

    SetFilePointer (outFileHandle, offsLo, &offsHi, FILE_BEGIN);
    result = WriteFile (outFileHandle, buf, count, &procSize, NULL);

    if (processedSize != NULL)
      (*processedSize) = procSize;

    //if (count != procSize)  TRACE_LOG("ArchiveAccess requested %d processed %d\n",count,procSize);
  }

  if (result != 0)
    return S_OK;
  else
    return S_FALSE;
}

/*  We create here our little interface to the DLL.
    Its functions will be called by "zippy" in archive.cpp.
    It's simpler so than doing the work in zippy, in part because of
    dubious code structure (spitting in the soup :))
*/

FileInArchiveInfo FileInfo;
BYTE current_archived_file;
aaHandle ArchiveHandle=0;

void ArchiveAccess_Close() {
  if(ArchiveHandle) // or big crash...
    aaCloseArchive (ArchiveHandle);
  ArchiveHandle=0;
}


bool ArchiveAccess_Extract(char *dest_dir) {
  HANDLE outFileHandle = 
    CreateFile (dest_dir, FILE_WRITE_DATA, FILE_SHARE_READ, 
    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 
    NULL);
  UINT64 ExtractedFileSize;
  int oStreamID = reinterpret_cast <int> (outFileHandle);
  aaExtract (ArchiveHandle, current_archived_file, oStreamID, writeCallback, 
    &ExtractedFileSize);
  CloseHandle (outFileHandle);
  return true; //of course
}


bool ArchiveAccess_Open(TCHAR* ArchiveFileName) {
  // ArchiveFileName is the full path
  wchar_t password[] = L"xxx";
  int OpenArchiveError = 0;
  HANDLE FileHandle = CreateFile (ArchiveFileName, FILE_READ_DATA, 
                                       FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, NULL);
  if(FileHandle!=(void*)-1) 
  {
    // Open archive
    int iStreamID = reinterpret_cast <int> (FileHandle);
    unsigned long FileSizeHi;
    UINT32 FileSizeLo = GetFileSize (FileHandle, &FileSizeHi);
    INT64 FileSize = (FileSizeHi << 32) + FileSizeLo;
    int flags;
    int ArchiveType = 
      aaDetermineArchiveType (readCallback, iStreamID, FileSize, &flags);
    TRACE_LOG("ArchiveAccess %s open FileHandle %d ArchiveType %d %X flags %X\n",
      ArchiveFileName,FileHandle,ArchiveType,ArchiveType,flags);
    ArchiveHandle = aaOpenArchive (readCallback, iStreamID, FileSize, 
      ArchiveType, &OpenArchiveError, password);
    if (OpenArchiveError == 0) { //OK
      current_archived_file=0; // first file
      aaGetFileInfo (ArchiveHandle,current_archived_file, &FileInfo);
      return true;
    }
  }
  return false;    
}


bool ArchiveAccess_Select(int n) {
  if(ArchiveHandle)
  {
    int FileCount = aaGetFileCount (ArchiveHandle);
    if(n<FileCount)
    {
      aaGetFileInfo (ArchiveHandle,n,&FileInfo);
      return true;
    }
  }
  return false;
}

#endif//archiveaccess
#endif//SS