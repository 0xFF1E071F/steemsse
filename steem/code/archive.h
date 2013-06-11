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
#include "SSE/SSEOption.h"
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

#define LOGSECTION LOGSECTION_INIT//SS

#ifdef _MSC_VER
#pragma comment(lib,"../../3rdparty/UnRARDLL/unrar.lib")
#if defined(WIN32) && defined(SS_DELAY_LOAD_DLL)
#pragma comment(linker, "/delayload:unrar.dll")
#endif
#endif

#ifdef __BORLANDC__
#pragma comment(lib,"../../3rdparty/UnRARDLL/bcc/unrar.lib")
// Borland: delay load DLL in makefile
#endif


void LoadUnrarDLL() {

  try {
/*  This function is missing in very old versions of UnRAR.dll, so it is safer 
    to use LoadLibrary and GetProcAddress to access it.
*/
    UNRAR_OK=RARGetDllVersion(); // 5 with our unrar.dll
  }
  catch(...) {
    UNRAR_OK=0;
  }
  if (UNRAR_OK)
  {
      TRACE_LOG("UnRAR.DLL loaded, v%d\n",UNRAR_OK);
      // prefill structures for all archives
      ZeroMemory(&zippy.ArchiveData,sizeof(zippy.ArchiveData));  
      ZeroMemory(&zippy.HeaderData,sizeof(zippy.HeaderData));  
      zippy.ArchiveData.OpenMode=RAR_OM_EXTRACT;
  }
  else
    TRACE_LOG("Failed to open UnRAR.DLL\n");
}
#undef LOGSECTION
#endif//ss

