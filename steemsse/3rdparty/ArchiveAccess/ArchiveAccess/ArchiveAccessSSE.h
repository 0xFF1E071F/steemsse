#pragma once
#ifndef ARCHIVEACCESSSSE_H
#define ARCHIVEACCESSSSE_H

#include "../ArchiveAccess/bgstr.h"
#include "../ArchiveAccess/ArchiveAccessBase.h"
//#include "../ArchiveAccess/ArchiveAccessDynamic.h"
// check that file for some C++ horror :(

#define FileInArchiveInfoStringSize 1024
struct FileInArchiveInfo {
	int ArchiveHandle; // handle for Archive/class pointer
    //int FileIndex;
	UINT64 CompressedFileSize;
	UINT64 UncompressedFileSize;
	UINT32 attributes;
	bool IsDir, IsEncrypted;
	_FILETIME LastWriteTime, CreationTime, LastAccessTime;
	unsigned short path[FileInArchiveInfoStringSize];
};
typedef void* aaHandle;

extern FileInArchiveInfo FileInfo;
extern BYTE current_archived_file;
extern aaHandle ArchiveHandle;

bool LoadArchiveAccessDll (const TCHAR* LibName);
void UnloadArchiveAccessDll();
bool ArchiveAccess_Open(TCHAR* name) ; // returns true = success
void ArchiveAccess_Close();
bool ArchiveAccess_Extract(char *dest_dir);
bool ArchiveAccess_Select(int n);

#endif//#ifndef ARCHIVEACCESSSSE_H