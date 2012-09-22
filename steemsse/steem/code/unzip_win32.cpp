#ifdef STEVEN_SEAGAL____
#include "../pch.h"
#include "Seagal.h"
#include "includes2.h"
#include "unzip_win32.h"
HINSTANCE hUnzip=NULL;
//---------------------------------------------------------------------------
void LoadUnzipDLL()
{
	hUnzip=LoadLibrary("unzipd32.dll");
	enable_zip=(hUnzip!=NULL);
	if (hUnzip){
		GetFirstInZip=(int(_stdcall*)(char*,PackStruct*))GetProcAddress(hUnzip,"GetFirstInZip");
		GetNextInZip=(int(_stdcall*)(PackStruct*))GetProcAddress(hUnzip,"GetNextInZip");
		CloseZipFile=(void(_stdcall*)(PackStruct*))GetProcAddress(hUnzip,"CloseZipFile");
		isZip=(BYTE(_stdcall*)(char*))GetProcAddress(hUnzip,"isZip");
		UnzipFile=(int(_stdcall*)(char*,char*,WORD,long,void*,long))GetProcAddress(hUnzip,"unzipfile");

		if (!(GetFirstInZip && GetNextInZip && CloseZipFile && isZip && UnzipFile)){
			FreeLibrary(hUnzip);
      hUnzip=NULL;
      enable_zip=false;
	  }
	}
}
//---------------------------------------------------------------------------
#endif
