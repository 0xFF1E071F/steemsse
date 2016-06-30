/*---------------------------------------------------------------------------
FILE: archive.cpp
MODULE: Steem
DESCRIPTION: This file contains the code for zipclass, Steem's abstraction
of the various unarchiving libraries it can use.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: archive.cpp")
#endif

#ifdef SSE_UNIX
#include <conditions.h>
#endif

#if defined(STEVEN_SEAGAL) && (defined(SSE_VAR_ARCHIVEACCESS) \
  ||defined(SSE_ACSI_MULTIPLE2) || defined(SSE_GUI_STATUS_STRING_380))
char ansi_name[MAX_PATH]; // for conversion from unicode
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_ARCHIVE_H) 
zipclass zippy; 

#if defined(WIN32)


#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
HINSTANCE hUnzip=NULL;
void LoadUnzipDLL()
{
  //hUnzip=LoadLibrary("unzipd32.dll");
  hUnzip=LoadLibrary(UNZIP_DLL);
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
#ifdef SSE_DEBUG
    if(hUnzip)
      TRACE_INIT("%s loaded\n",UNZIP_DLL);
    else
      TRACE_INIT("%s not available",UNZIP_DLL);
#elif defined(SSE_SSE_CONFIG_STRUCT)//3.8.2
    else SSEConfig.unzipd32Dll=true;
#endif

  }//hunzip
}
#endif
#endif //defined(WIN32)

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_UNRAR)

#define LOGSECTION LOGSECTION_INIT

#ifdef _MSC_VER

#if defined(SSE_X64_LIBS)
#pragma comment(lib,"../../3rdparty/UnRARDLL/x64/unrar64.lib")
#else
#pragma comment(lib,"../../3rdparty/UnRARDLL/unrar.lib")
#endif
#if defined(WIN32) && defined(SSE_DELAY_LOAD_DLL)
#ifndef SSE_VS2003_DELAYDLL
#pragma comment(linker, "/delayload:unrar.dll")
#endif
#endif
#endif
#ifdef __BORLANDC__
#pragma comment(lib,"../../3rdparty/UnRARDLL/bcclib/unrar.lib")
// Borland: delay load DLL in makefile
#endif


void LoadUnrarDLL() {

  try {
/*  This function is missing in very old versions of UnRAR.dll, so it is safer 
    to use LoadLibrary and GetProcAddress to access it.
*/
#if defined(SSE_VAR_UNRAR2)
    UNRAR_OK=(RARGetDllVersion()>0); //bugfix v3.7.1, works with v6 too :)
#else
    UNRAR_OK=RARGetDllVersion(); // 5 with our unrar.dll
#endif
  }
  catch(...) {
    UNRAR_OK=0;
  }
  if (UNRAR_OK)
  {
//      TRACE_LOG("UnRAR.DLL loaded, v%d\n",UNRAR_OK);
      TRACE_INIT("unrar.dll loaded, v%d\n",RARGetDllVersion());
      // prefill structures for all archives
      ZeroMemory(&zippy.ArchiveData,sizeof(zippy.ArchiveData));  
      ZeroMemory(&zippy.HeaderData,sizeof(zippy.HeaderData));  
      zippy.ArchiveData.OpenMode=RAR_OM_EXTRACT;
  }
#ifdef SSE_DEBUG
  else
    TRACE_INIT("Unrar.dll not available\n");
#endif
}
#undef LOGSECTION
#endif//ss


#endif

#define LOGSECTION LOGSECTION_IMAGE_INFO//SS

//---------------------------------------------------------------------------
zipclass::zipclass()
{
  type[0]=0;
  is_open=false;
  current_file_n=0;
  err=0;
#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_UNRAR)
  hArcData=0;
#endif
}
//---------------------------------------------------------------------------
bool zipclass::first(char *name)
{
  if (enable_zip==0) return ZIPPY_FAIL;

  if (is_open) close();

  type[0]=0;
  char *dot=strrchr(name,'.');
  if (dot){
    if (strlen(dot+1)<11) strcpy(type,dot+1);
  }
  if (type[0]==0) strcpy(type,"ZIP");
  strupr(type);
#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS)
#if defined(SSE_VAR_ARCHIVEACCESS3)
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"ZIP","7Z","BZ2","GZ","TAR","ARJ",NULL)){
#elif defined(SSE_VAR_ARCHIVEACCESS2)
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"7Z","BZ2","GZ","TAR","ARJ",NULL)){
#else
  if (strcmp(type,"7Z")==0 && ARCHIVEACCESS_OK) {
#endif
    if(ArchiveAccess_Open(name))
    {
      is_open=true;
      current_file_n=0;
      current_file_offset=0;
      attrib=WORD(FileInfo.attributes);
      crc=0;//?
      return ZIPPY_SUCCEED;
    }
    else
      return ZIPPY_FAIL;
} else
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  if (strcmp(type,"ZIP")==0){
#ifdef UNIX
    uf=unzOpen(name);
    if (uf==NULL){
      last_error="Couldn't open ";
      last_error+=name;
      return ZIPPY_FAIL;
    }else{
      is_open=true;
      err=unzGetGlobalInfo(uf,&gi);
      current_file_n=0;
      current_file_offset=0;
      err=unzGetCurrentFileInfo(uf,&fi,filename_inzip,
            sizeof(filename_inzip),NULL,0,NULL,0);
      if (err){
        close();
        return ZIPPY_FAIL;
      }
      crc=fi.crc;
    }
#endif
#ifdef WIN32
    ZeroMemory(&PackInfo,sizeof(PackInfo));

    if (GetFirstInZip(name,&PackInfo)!=UNZIP_Ok){
      return ZIPPY_FAIL;
    }
    is_open=true;
    current_file_n=0;
    current_file_offset=PackInfo.HeaderOffset;
    attrib=PackInfo.Attr;
    crc=PackInfo.Crc;
#endif
    return ZIPPY_SUCCEED;
#endif
#ifdef RAR_SUPPORT
  }else if (strcmp(type,"RAR")==0){
    if (urarlib_list(name,(ArchiveList_struct*)&rar_list)<=0) return ZIPPY_FAIL;

    // There are some files in this RAR
    while (rar_list){
      if ((rar_list->item.FileAttr & 0x10)==0) break; // Not directory
      rar_list=rar_list->next;
    }
    if (rar_list==NULL) return ZIPPY_FAIL;

    is_open=true;
    rar_current=rar_list;
    current_file_n=0;
    current_file_offset=0;
    attrib=WORD(rar_current->item.FileAttr);
    crc=rar_current->item.FileCRC;

    return ZIPPY_SUCCEED;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_UNRAR)
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  } else 
#endif
  if (strcmp(type,"RAR")==0 && UNRAR_OK) {
    ArchiveData.ArcName=name;
    hArcData=RAROpenArchive(&ArchiveData);
    ASSERT(hArcData);
    ASSERT(!ArchiveData.OpenResult);
    if(!hArcData || ArchiveData.OpenResult)
      return ZIPPY_FAIL;
//    TRACE_LOG("UnRAR: %s now open\n",name);
    int ec=RARReadHeader(hArcData,&HeaderData);
    if(!ec)
    {
//      ASSERT( !(HeaderData.FileAttr&0x10) ); // directory!
//      TRACE_LOG("UnRAR 1st file %s v%d\n",HeaderData.FileName,HeaderData.UnpVer);
      is_open=true;
      current_file_n=0;
      current_file_offset=0;
      attrib=WORD(HeaderData.FileAttr);
      crc=HeaderData.FileCRC;
      return ZIPPY_SUCCEED;
    }
#endif
  }
  return ZIPPY_FAIL;
}
//---------------------------------------------------------------------------
bool zipclass::next()
{
  if (enable_zip==0) return ZIPPY_FAIL;

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS)
#if defined(SSE_VAR_ARCHIVEACCESS3)
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"ZIP","7Z","BZ2","GZ","TAR","ARJ",NULL)){
#elif defined(SSE_VAR_ARCHIVEACCESS2)
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"7Z","BZ2","GZ","TAR","ARJ",NULL)){
#else
  if (strcmp(type,"7Z")==0 && ARCHIVEACCESS_OK) {
#endif
    if (is_open==0 || !ArchiveHandle) 
      return ZIPPY_FAIL;
    current_file_n++;
    current_archived_file++;
    ASSERT(current_archived_file==current_file_n);
    if(ArchiveAccess_Select(current_file_n))
    {
      attrib=WORD(FileInfo.attributes);
      current_file_offset=current_file_n;
      return ZIPPY_SUCCEED;
    }
  } else
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  if (strcmp(type,"ZIP")==0){
#ifdef UNIX
    if (is_open==0) return ZIPPY_FAIL;
  	if (current_file_n>=(int)(gi.number_entry-1)) return ZIPPY_FAIL;
    err=unzGoToNextFile(uf);
    if (err) return ZIPPY_FAIL;
    err=unzGetCurrentFileInfo(uf,&fi,filename_inzip,
          sizeof(filename_inzip),NULL,0,NULL,0);
    if (err) return ZIPPY_FAIL;
    current_file_n++;
    current_file_offset=current_file_n;
    crc=fi.crc;
#endif
#ifdef WIN32
    err=GetNextInZip(&PackInfo);
    if (err!=UNZIP_Ok) return ZIPPY_FAIL;
    attrib=PackInfo.Attr;
    crc=PackInfo.Crc;
    current_file_n++;
    current_file_offset=PackInfo.HeaderOffset;
#endif

    return ZIPPY_SUCCEED;
#endif
#ifdef RAR_SUPPORT
  }else if (strcmp(type,"RAR")==0){
    if (is_open==0 || rar_current==NULL) return ZIPPY_FAIL;

    do{
      rar_current=rar_current->next;
      current_file_n++;
      if (rar_current==NULL) return ZIPPY_FAIL;
    }while (rar_current->item.FileAttr & 0x10); // Skip if directory

    current_file_offset=current_file_n;
    attrib=WORD(rar_current->item.FileAttr);
    crc=rar_current->item.FileCRC;

    return ZIPPY_SUCCEED;
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_UNRAR)
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  }else 
#endif
  if (strcmp(type,"RAR")==0){

    if (is_open==0 || !UNRAR_OK || !hArcData) 
      return ZIPPY_FAIL;

    int ec;
    // we must 'process' the current one or reading will fail
    ec=RARProcessFile(hArcData,RAR_TEST,NULL,NULL);
    ASSERT(!ec);
    ec=RARReadHeader(hArcData,&HeaderData);
    current_file_n++;
    if(!ec)
    {
//      ASSERT( !(HeaderData.FileAttr&0x10) ); // directory!
//      TRACE_LOG("UnRAR next file %s\n",HeaderData.FileName);
      current_file_offset=current_file_n;
      attrib=WORD(HeaderData.FileAttr);
      crc=HeaderData.FileCRC;
      return ZIPPY_SUCCEED;
    }
//    else
//      TRACE_LOG("UnRAR next %d ec %d\n",current_file_n,ec);
#endif
  }
  return ZIPPY_FAIL;
}

char* zipclass::filename_in_zip()
{
  if (enable_zip==0) return "";
#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS)
#if defined(SSE_VAR_ARCHIVEACCESS3)
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"ZIP","7Z","BZ2","GZ","TAR","ARJ",NULL)){
#elif defined(SSE_VAR_ARCHIVEACCESS2)
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"7Z","BZ2","GZ","TAR","ARJ",NULL)){
#else
  if (strcmp(type,"7Z")==0 && ARCHIVEACCESS_OK) {
#endif
    if(ArchiveHandle) // need to convert name from unicode
    {
      WideCharToMultiByte ( CP_ACP,                // ANSI code page
        WC_COMPOSITECHECK,     // Check for accented characters
        (LPCWSTR)FileInfo.path,         // Source Unicode string
        -1,                    // -1 means string is zero-terminated
        (char*)ansi_name, //SS a global
        MAX_PATH,  // Size of buffer
        NULL,                  // No default character
        NULL );                // Don't care about this flag
      return (char*)ansi_name;
    }
  } else
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  if (strcmp(type,"ZIP")==0){
#ifdef UNIX
    return filename_inzip;
#endif
#ifdef WIN32
    return PackInfo.FileName;
#endif
#endif//aa4
#ifdef RAR_SUPPORT
  }else if (strcmp(type,"RAR")==0){
    if (rar_current) return rar_current->item.Name;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_UNRAR)
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  }else 
#endif
  if (strcmp(type,"RAR")==0){
    if(UNRAR_OK&&hArcData) 
      return HeaderData.FileName;
#endif
  }
  return "";
}
//---------------------------------------------------------------------------
bool zipclass::close()
{
  if (enable_zip==0) return ZIPPY_FAIL;

  if (is_open){

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS)
#if defined(SSE_VAR_ARCHIVEACCESS3)
    if(ARCHIVEACCESS_OK
      && MatchesAnyString_I(type,"ZIP","7Z","BZ2","GZ","TAR","ARJ",NULL)){
#elif defined(SSE_VAR_ARCHIVEACCESS2)
    if(ARCHIVEACCESS_OK
      && MatchesAnyString_I(type,"7Z","BZ2","GZ","TAR","ARJ",NULL)){
#else
    if (strcmp(type,"7Z")==0 && ARCHIVEACCESS_OK) {
#endif
      ArchiveAccess_Close();
      is_open=false;
      return ZIPPY_SUCCEED;
    }
    else
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
    if (strcmp(type,"ZIP")==0){
      UNIX_ONLY( unzClose(uf); )
      WIN_ONLY( CloseZipFile(&PackInfo); )
      is_open=false;
      return ZIPPY_SUCCEED;
#endif//aa4
#ifdef RAR_SUPPORT
    }else if (strcmp(type,"RAR")==0){
      is_open=false;
      return ZIPPY_SUCCEED;
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_UNRAR)
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
    }else 
#endif
    if (strcmp(type,"RAR")==0 && UNRAR_OK){
//      TRACE_LOG("closing RAR archive\n");
      VERIFY( !RARCloseArchive(hArcData) );
      is_open=false;
      return ZIPPY_SUCCEED;
#endif
    }
  }
  return ZIPPY_FAIL;
}
//---------------------------------------------------------------------------
void zipclass::list_contents(char *name,EasyStringList *eslp,bool st_disks_only)
{
  if (enable_zip==0) return;

  eslp->DeleteAll();

  if (first(name)==0){
    do{
      EasyStr a=filename_in_zip();
      bool addflag=true;
      if (st_disks_only){
        if (FileIsDisk(a)==DISK_UNCOMPRESSED
#if defined(SSE_TOS_PRG_AUTORUN)
          // no interference with context menu, MSA Converter
          && !MatchesAnyString_I(strrchr(a.Text,'.')+1,"PRG","TOS",NULL)
#endif
          || FileIsDisk(a)==DISK_PASTI){
          addflag=true;
        }else{
          addflag=0;
        }
      }
      if (addflag) eslp->Add(a,current_file_offset,attrib,crc);
    }while (next()==0);
  }
  close();
}
//---------------------------------------------------------------------------
bool zipclass::extract_file(char *fn,int offset,char *dest_dir,bool hide,DWORD attrib)
{
//  TRACE_LOG("zippy extract %s (#%d) to %s\n",fn,offset,dest_dir);
  if (enable_zip==0) return ZIPPY_FAIL;

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS)
#if defined(SSE_VAR_ARCHIVEACCESS3)
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"ZIP","7Z","BZ2","GZ","TAR","ARJ",NULL)){
#elif defined(SSE_VAR_ARCHIVEACCESS2) 
  if(ARCHIVEACCESS_OK
    && MatchesAnyString_I(type,"7Z","BZ2","GZ","TAR","ARJ",NULL)){
#else
  if (strcmp(type,"7Z")==0 && ARCHIVEACCESS_OK) {
#endif
    if (is_open) 
      close();
    if (first(fn)==0){
      while (offset > 0){
        if (next()){ // Failed
          close();
          return ZIPPY_FAIL;
        }
        offset--;
      }
    }
    ArchiveAccess_Extract(dest_dir);
    close();
#ifdef WIN32
    if (hide){
      SetFileAttributes(dest_dir,FILE_ATTRIBUTE_HIDDEN);
    }else{
      SetFileAttributes(dest_dir,attrib);
    }
#endif
    return ZIPPY_SUCCEED;
  } else
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  if (strcmp(type,"ZIP")==0){
    if (is_open) close();

#ifdef UNIX
    uf=unzOpen(fn);
    if (uf==NULL){
      last_error=Str("Couldn't open ")+fn;
      return ZIPPY_FAIL;
    }
    is_open=true;
    err=unzGetGlobalInfo(uf,&gi);
    if (err){
      close();
      return ZIPPY_FAIL;
    }

    unz_global_info gi;
    int err=unzGetGlobalInfo(uf,&gi);
    if (err!=UNZ_OK){
      close();
      last_error="couldn't get global info";
      return ZIPPY_FAIL;
    }
    if (offset>=(int)gi.number_entry){
      close();
      last_error="too few files in zip";
      return ZIPPY_FAIL;
    }
  //  unzGoToFirstFile(uf);
    for (int i=0;i<offset;i++) unzGoToNextFile(uf);

#define UNZIP_BUF_SIZE 8192
    BYTE buf[UNZIP_BUF_SIZE];
    err=unzGetCurrentFileInfo(uf,&fi,filename_inzip,
          sizeof(filename_inzip),NULL,0,NULL,0);
    if (err) return ZIPPY_FAIL;

    EasyStr dest_file=dest_dir;
    if (dest_dir[0]==0 || dest_dir[strlen(dest_dir)-1]=='/'){
      char *t=strrchr(filename_inzip,'/');
      if (t){
        t++;
      }else{
        t=strrchr(filename_inzip,'\\');
        if(t){
          t++;
        }else{
          t=filename_inzip;
        }
      }
      dest_file+=t;
    }

    err=unzOpenCurrentFile(uf);
    if (err){
      close();
      last_error="error opening file in ZIP";
      printf("%s\n",last_error.Text);
      return ZIPPY_FAIL;
    }

    FILE *fout=fopen(dest_file,"wb");
    if (fout==NULL){
      close();unzCloseCurrentFile(uf);
      last_error="error opening file ";last_error+=dest_file.Text;
      printf("%s\n",last_error.Text);
      return ZIPPY_FAIL;
    }

    do{
      err=unzReadCurrentFile(uf,buf,UNZIP_BUF_SIZE);
      if (err<0){
        last_error=EasyStr("error #")+err+" with zipfile in unzReadCurrentFile";
        printf("%s\n",last_error.Text);
        break;
      }else if(err>0){
        fwrite(buf,err,1,fout);
      }
    }while (err>0);
    fclose(fout);

    err=unzCloseCurrentFile(uf);
    close();
    if (err) return ZIPPY_FAIL;

#elif defined(WIN32)
    EasyStr dest_file=dest_dir;
    err=UnzipFile(fn,dest_file.Text,(WORD)(hide ? 2:attrib),offset,NULL,0);
    if (err!=UNZIP_Ok) return ZIPPY_FAIL;
#endif

    return ZIPPY_SUCCEED;
#endif//aa4
#ifdef RAR_SUPPORT
  }else if (strcmp(type,"RAR")==0){
    if (is_open) close();

    if (first(fn)==0){
      while (offset > 0){
        if (next()){ // Failed
          close();
          return ZIPPY_FAIL;
        }
        offset--;
      }
    }

    char *data_ptr;
    DWORD data_size;
    if (urarlib_get(&data_ptr,&data_size,rar_current->item.Name,fn,"")==0) return ZIPPY_FAIL;
    close();

    EasyStr dest_file=dest_dir;
    FILE *f=fopen(dest_file,"wb");
    if (f==NULL) return ZIPPY_FAIL;
    fwrite(data_ptr,1,data_size,f);
    fclose(f);

#ifdef WIN32
    if (hide){
      SetFileAttributes(dest_file,FILE_ATTRIBUTE_HIDDEN);
    }else{
      SetFileAttributes(dest_file,attrib);
    }
#endif

    return ZIPPY_SUCCEED;

#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_UNRAR)
#if !(defined(STEVEN_SEAGAL) && defined(SSE_VAR_ARCHIVEACCESS4))
  }else 
#endif
  if (strcmp(type,"RAR")==0 && UNRAR_OK){
    if (is_open) 
      close();

    if (first(fn)==0){
      while (offset > 0){
        if (next()){ // Failed
          close();
          return ZIPPY_FAIL;
        }
        offset--;
      }
    }
#ifndef SSE_VS2008_WARNING_382
    char *data_ptr;
    DWORD data_size;
#endif
    ASSERT(hArcData);
    int ec=RARProcessFile(hArcData,RAR_EXTRACT,NULL,dest_dir); 
//    TRACE_LOG("%s -> %s : %d\n",HeaderData.FileName,dest_dir,ec);
    ASSERT(!ec);
    if(ec)
      return ZIPPY_FAIL;    
    close();
#ifdef WIN32
    if (hide){
      SetFileAttributes(dest_dir,FILE_ATTRIBUTE_HIDDEN);
    }else{
      SetFileAttributes(dest_dir,attrib);
    }
#endif
    return ZIPPY_SUCCEED;
#endif

  }
  return ZIPPY_FAIL;
}
//---------------------------------------------------------------------------

#undef LOGSECTION
