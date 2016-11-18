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


#include <SSE/SSE.h> // get switches

#if defined(SSE_VAR_ARCHIVEACCESS)

#include <SSE/SSEDebug.h>
#include <windows.h>
#include "../ArchiveAccess/bgstr.h"
#include "../ArchiveAccess/ArchiveAccessBase.h"
#define LOGSECTION LOGSECTION_IMAGE_INFO
#if defined(BCC_BUILD)
#define max(a,b) ( (a)>(b)?(a):(b) ) //when 1st compiling as is
#endif

#include "../ArchiveAccess/ArchiveAccessDynamic.h"


#include <archive.decla.h>

///////////////////////////////////////////////////////////////////////////////
// Dynamically loaded library

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
		TRACE_INIT("%s loaded, t1 %d t2 %d t3 %d t4 %d t5 %d t6 %d t7 %d\n",LibName,t1,t2,t3,t4,t5,t6,t7);
		return (t1 && t2 && t3 && t4 && t5 && t6 && t7);
    } else 
    {
        TRACE_INIT("%s not available\n",LibName);
        return false;
    }
}

void UnloadArchiveAccessDll ()
{
    FreeLibrary(hinstLib); 
}

#pragma warning(disable: 4293)//'<<' : shift count negative or too big, undefined behavior//382

///////////////////////////////////////////////////////////////////////////////
// Callback function for reading data

HRESULT __stdcall readCallback (int StreamID, INT64 offset, UINT32 count, 
                                void* buf, UINT32* processedSize)
{
   unsigned long ReadBytes;
   HANDLE handle = reinterpret_cast <HANDLE> (StreamID);
   long offsHi = (long) (offset >> 32);
   long  offsLo = (long) (offset & ((1 << 32) -1));
   SetFilePointer (handle, offsLo, &offsHi, FILE_BEGIN);
   int result = ReadFile(handle, buf, count, &ReadBytes, NULL);
   if (processedSize != NULL)
       (*processedSize) = ReadBytes;
   if (result != 0)
     return S_OK;
   else
     return S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// Callback function for writing data

HRESULT __stdcall writeCallback (int StreamID, INT64 offset, UINT32 count, 
                                 const void* buf, UINT32* processedSize)
{
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
  {
    aaCloseArchive (ArchiveHandle);
#if defined(SSE_VAR_ARCHIVEACCESS_383)
    CloseHandle(zippy.hArcData); // fixes handle leak + impossible to move file
#endif
  }
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
  bool ok=false;
  wchar_t password[] = L"xxx";
  int OpenArchiveError;
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
    ArchiveHandle = aaOpenArchive(readCallback, iStreamID, FileSize,
      ArchiveType, &OpenArchiveError, password);
#if defined(SSE_VAR_ARCHIVEACCESS_383)
    zippy.hArcData=FileHandle; // memorise this
#endif
#ifdef SSE_DEBUG
    TRACE_LOG("ArchiveAccess open %s Type %c%c%c%c flags %X: ERR %d handle:%d\n",
      ArchiveFileName,(ArchiveType>>(3*8))&0xFF,(ArchiveType>>(2*8))&0xFF,
      (ArchiveType>>8)&0xFF,ArchiveType&0xFF,flags,OpenArchiveError,iStreamID);
#endif
    if (OpenArchiveError == 0) { //OK
      current_archived_file=0; // first file
      aaGetFileInfo (ArchiveHandle,current_archived_file, &FileInfo);
      ok=true;
    }
  }
  return ok;
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

