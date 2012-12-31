#ifdef UNIX
#include <unzip.h>
#endif

#ifdef WIN32
#include "unzip_win32.h"
#endif

#ifndef NO_RAR_SUPPORT
#define RAR_SUPPORT
#endif

#ifdef RAR_SUPPORT
#include <unrarlib/unrarlib/unrarlib.h>	
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_VAR_UNRAR)
#include <UnRARDLL/unrar.h>
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
}zippy; 

#if defined(STEVEN_SEAGAL) && defined(SS_VAR_UNRAR)

#define LOGSECTION LOGSECTION_INIT

// this is heavy but at least it's dynamic (Steem can run without DLL)

HINSTANCE hUnrar=NULL;

HANDLE (PASCAL* RarOpenArchive)(struct RAROpenArchiveData*);
HANDLE (PASCAL* RarOpenArchiveEx)(struct RAROpenArchiveDataEx *ArchiveData);
int    (PASCAL* RarCloseArchive)(HANDLE hArcData);
int    (PASCAL* RarReadHeader)(HANDLE hArcData,struct RARHeaderData *HeaderData);
int    (PASCAL* RarReadHeaderEx)(HANDLE hArcData,struct RARHeaderDataEx *HeaderData);
int    (PASCAL* RarProcessFile)(HANDLE hArcData,int Operation,char *DestPath,char *DestName);
int    (PASCAL* RarProcessFileW)(HANDLE hArcData,int Operation,wchar_t *DestPath,wchar_t *DestName);
void   (PASCAL* RarSetCallback)(HANDLE hArcData,UNRARCALLBACK Callback,LPARAM UserData);
void   (PASCAL* RarSetChangeVolProc)(HANDLE hArcData,CHANGEVOLPROC ChangeVolProc);
void   (PASCAL* RarSetProcessDataProc)(HANDLE hArcData,PROCESSDATAPROC ProcessDataProc);
void   (PASCAL* RarSetPassword)(HANDLE hArcData,char *Password);
int    (PASCAL* RarGetDllVersion)();

void LoadUnrarDLL()
{
  hUnrar=LoadLibrary("unrar.dll");
  enable_rar=(hUnrar!=NULL);
  if (hUnrar){

    RarOpenArchive=(HANDLE(PASCAL*)(struct RAROpenArchiveData *ArchiveData))GetProcAddress(hUnrar,"RAROpenArchive");
    RarOpenArchiveEx=(HANDLE(PASCAL*)(struct RAROpenArchiveDataEx *ArchiveData))GetProcAddress(hUnrar,"RAROpenArchiveEx");
    RarCloseArchive=(int(PASCAL*)(HANDLE hArcData))GetProcAddress(hUnrar,"RARCloseArchive");
    RarReadHeader=(int(PASCAL*)(HANDLE hArcData,struct RARHeaderData *HeaderData))GetProcAddress(hUnrar,"RARReadHeader");
    RarReadHeaderEx=(int(PASCAL*)(HANDLE hArcData,struct RARHeaderDataEx *HeaderData))GetProcAddress(hUnrar,"RARReadHeaderEx");
    RarProcessFile=(int(PASCAL*)(HANDLE hArcData,int Operation,char *DestPath,char *DestName))GetProcAddress(hUnrar,"RARProcessFile");
    RarProcessFileW=(int(PASCAL*)(HANDLE hArcData,int Operation,wchar_t *DestPath,wchar_t *DestName))GetProcAddress(hUnrar,"RARProcessFileW");
    RarSetCallback=(void(PASCAL*)(HANDLE hArcData,UNRARCALLBACK Callback,LPARAM UserData))GetProcAddress(hUnrar,"RARSetCallback");
    RarSetChangeVolProc=(void(PASCAL*)(HANDLE hArcData,CHANGEVOLPROC ChangeVolProc))GetProcAddress(hUnrar,"RARSetChangeVolProc");
    RarSetProcessDataProc=(void(PASCAL*)(HANDLE hArcData, PROCESSDATAPROC ProcessDataProc))GetProcAddress(hUnrar,"RARSetProcessDataProc");
    RarSetPassword=(void(PASCAL*)(HANDLE hArcData,char *Password))GetProcAddress(hUnrar,"RARSetPassword");
    RarGetDllVersion=(int(PASCAL*)())GetProcAddress(hUnrar,"RARGetDllVersion");

    if (!(RarOpenArchive && RarOpenArchiveEx && RarCloseArchive 
      && RarReadHeader && RarReadHeaderEx && RarProcessFile && RarProcessFileW
      && RarSetCallback && RarSetChangeVolProc && RarSetProcessDataProc
      && RarSetPassword && RarGetDllVersion))
    {
      TRACE_LOG("Failed to init UnRAR.DLL\n");
      FreeLibrary(hUnrar);
      hUnrar=NULL;
      enable_rar=false;
    }
    else 
    {
      TRACE_LOG("UnRAR.DLL loaded\n");
      // prefill structures for all archives
      ZeroMemory(&zippy.ArchiveData,sizeof(zippy.ArchiveData));  
      ZeroMemory(&zippy.HeaderData,sizeof(zippy.HeaderData));  
      zippy.ArchiveData.OpenMode=RAR_OM_EXTRACT;
    }
  }
  else
    TRACE_LOG("Failed to open UnRAR.DLL\n");
}
#undef LOGSECTION
#endif//ss

