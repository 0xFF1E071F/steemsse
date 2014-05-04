/*---------------------------------------------------------------------------
FILE: associate.cpp
MODULE: Steem
DESCRIPTION: The code to perform the sometimes confusing task of associating
Steem with required file types.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: associate.cpp")
#endif

#ifdef WIN32

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)
/*  v3.5.3
    The proper way to associate in Win32 is to use current user, not root.
    HKEY_CURRENT_USER instead of HKEY_CLASSES_ROOT, so you
    don't need to run as administrator.
*/

#define LOGSECTION LOGSECTION_INIT
char key_location[]="Software\\Classes\\"; // where we put the extensions

/*  Function RegDeleteTree() isn't available in older compiler/Windows versions.
    So we use RegDelnode() and RegDelnodeRecurse() from:
    http://msdn.microsoft.com/en-au/windows/desktop/ms724235(v=vs.85).aspx
    In turn, function StringCchCopy() isn't available, so we use strcpy
    instead.
    TODO move this to '3rd party'.
*/

#include <windows.h>
#include <stdio.h>
//#include <strsafe.h>

HRESULT StringCchCopy(LPTSTR pszDest,size_t cchDest,LPCTSTR pszSrc) {
  strcpy(pszDest,pszSrc);
  return 0;
}

//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful.
//              FALSE if an error occurs.
//
//*************************************************************

BOOL RegDelnodeRecurse (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    LPTSTR lpEnd;
    LONG lResult;
    DWORD dwSize;
    TCHAR szName[MAX_PATH];
    HKEY hKey;
    FILETIME ftWrite;

    // First, see if we can delete the key without having
    // to recurse.

    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) 
        return TRUE;

    lResult = RegOpenKeyEx (hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) 
    {
        if (lResult == ERROR_FILE_NOT_FOUND) {
            printf("Key not found.\n");
            return TRUE;
        } 
        else {
            printf("Error opening key.\n");
            return FALSE;
        }
    }

    // Check for an ending slash and add one if it is missing.

    lpEnd = lpSubKey + lstrlen(lpSubKey);

    if (*(lpEnd - 1) != TEXT('\\')) 
    {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    // Enumerate the keys

    dwSize = MAX_PATH;
    lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                           NULL, NULL, &ftWrite);

    if (lResult == ERROR_SUCCESS) 
    {
        do {

            StringCchCopy (lpEnd, MAX_PATH*2, szName);

            if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
                break;
            }

            dwSize = MAX_PATH;

            lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                                   NULL, NULL, &ftWrite);

        } while (lResult == ERROR_SUCCESS);
    }

    lpEnd--;
    *lpEnd = TEXT('\0');

    RegCloseKey (hKey);

    // Try again to delete the key.

    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) 
        return TRUE;

    return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful.
//              FALSE if an error occurs.
//
//*************************************************************

BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    TCHAR szDelKey[MAX_PATH*2];

    StringCchCopy (szDelKey, MAX_PATH*2, lpSubKey);
    return RegDelnodeRecurse(hKeyRoot, szDelKey);

}

#else // defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)

LONG RegCopyKey(HKEY FromKeyParent,char *FromKeyName,HKEY ToKeyParent,char *ToKeyName)
{
  LONG Ret;
  HKEY FromKey,ToKey;

  if ( (Ret=RegOpenKey(FromKeyParent,FromKeyName,&FromKey))!=ERROR_SUCCESS ) return Ret;
  if ( (Ret=RegCreateKey(ToKeyParent,ToKeyName,&ToKey))!=ERROR_SUCCESS ){
    RegCloseKey(FromKey);
    return Ret;
  }

  int i=0;
  EasyStr Name,Data;
  DWORD NameSize,DataSize,Type;
  Name.SetLength(8192);
  Data.SetLength(8192);
  for (;;){
    NameSize=8192;DataSize=8192;
    if (RegEnumValue(FromKey,i++,Name,&NameSize,NULL,&Type,LPBYTE(Data.Text),&DataSize)!=ERROR_SUCCESS) break;
    RegSetValueEx(ToKey,Name,0,Type,LPBYTE(Data.Text),DataSize);
  }

  i=0;
  EasyStr Class;
  DWORD ClassSize;
  HKEY NewKey;
  FILETIME Time;
  Class.SetLength(8192);
  for (;;){
    NameSize=8192;ClassSize=8192;
    if (RegEnumKeyEx(FromKey,i++,Name,&NameSize,NULL,Class,&ClassSize,&Time)!=ERROR_SUCCESS) break;
    if (RegCreateKeyEx(ToKey,Name,0,Class,0,KEY_ALL_ACCESS,NULL,&NewKey,&Type)==ERROR_SUCCESS){
      RegCloseKey(NewKey);
      RegCopyKey(FromKey,Name,ToKey,Name);
    }
  }

  RegCloseKey(FromKey);
  RegCloseKey(ToKey);

  return Ret;
}
#endif//else of defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)

#endif//WIN32
//---------------------------------------------------------------------------
bool IsSteemAssociated(EasyStr Exts)
{
#ifdef WIN32

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)

  LONG ErrorCode;
  HKEY Key;
  EasyStr KeyName;
  DWORD Size;

  if (Exts[0]!='.') 
    Exts.Insert(".",0); //eg "ST" -> ".ST"
  Exts.Insert(key_location,0);
  ErrorCode=RegOpenKeyEx(HKEY_CURRENT_USER,Exts,0,KEY_ALL_ACCESS,&Key);
  //TRACE_LOG("RegOpenKeyEx %s ErrorCode %d\n",Exts.Text,ErrorCode);

  if(!ErrorCode) // extension recorded, but for us?
  {
    // get value and close key
    Size=400;
    KeyName.SetLength(Size);
    RegQueryValueEx(Key,NULL,NULL,NULL,(BYTE*)KeyName.Text,&Size);
    //TRACE_LOG("RegQueryValueEx %s = %s ErrorCode %d\n",Exts.Text,KeyName.Text,ErrorCode);
    RegCloseKey(Key);
    if(KeyName.Empty()) 
      KeyName=Exts;
    else
      KeyName.Insert("Software\\Classes\\",0);
    // find shell key
    ErrorCode=RegOpenKeyEx(HKEY_CURRENT_USER,KeyName+"\\Shell",0,
      KEY_ALL_ACCESS,&Key);
    //TRACE_LOG("RegOpenKeyEx %s\\Shell ErrorCode %d\n",KeyName.Text,ErrorCode);
    RegCloseKey(Key);
    if(!ErrorCode)
    {
      ErrorCode=RegOpenKeyEx(HKEY_CURRENT_USER,KeyName+"\\Shell\\OpenSteem\\Command",
        0,KEY_READ,&Key); 
      //TRACE_LOG("RegOpenKeyEx %s\\Shell\\OpenSteem\\Command ErrorCode %d\n",KeyName.Text,ErrorCode);

      if(!ErrorCode)
      {
        Size=400;
        EasyStr RegCommand;
        RegCommand.SetLength(Size);
        RegQueryValueEx(Key,NULL,NULL,NULL,(BYTE*)RegCommand.Text,&Size);
        //TRACE_LOG("RegQueryValueEx %s = %s ErrorCode %d\n",Exts.Text,RegCommand.Text,ErrorCode);
        RegCloseKey(Key);

        EasyStr ThisExeName=GetEXEFileName(); // TODO, no PRGID?
        EasyStr Command=char('"');
        Command.SetLength(MAX_PATH+5);
        GetLongPathName(ThisExeName,Command.Text+1,MAX_PATH);
        Command+=EasyStr(char('"'))+" \"%1\"";
        if (IsSameStr_I(Command,RegCommand)) 
          return true; // yes, our EXE is associated
      }
    }
  }

  return 0; // no, our EXE is not associated

#else//#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)

  if (Exts[0]!='.') Exts.Insert(".",0);

  HKEY Key;
  EasyStr KeyName;
  long Size;

  if (RegOpenKey(HKEY_CLASSES_ROOT,Exts,&Key)==ERROR_SUCCESS){
    Size=400;
    KeyName.SetLength(Size);
    RegQueryValue(Key,NULL,KeyName.Text,&Size);
    RegCloseKey(Key);
  }

  if (KeyName.Empty()) KeyName=Exts;

  if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName+"\\Shell",&Key)==ERROR_SUCCESS){
    Size=400;
    EasyStr Default;
    Default.SetLength(Size);
    RegQueryValue(Key,NULL,Default.Text,&Size);
    RegCloseKey(Key);
    if (Default=="OpenSteem"){
      if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName+"\\Shell\\OpenSteem\\Command",&Key)==ERROR_SUCCESS){
        Size=400;
        EasyStr RegCommand;
        RegCommand.SetLength(Size);
        RegQueryValue(Key,NULL,RegCommand.Text,&Size);
        RegCloseKey(Key);

        EasyStr ThisExeName=GetEXEFileName();
        EasyStr Command=char('"');
        Command.SetLength(MAX_PATH+5);
        GetLongPathName(ThisExeName,Command.Text+1,MAX_PATH);
        Command+=EasyStr(char('"'))+" \"%1\"";

        if (IsSameStr_I(Command,RegCommand)) return true;
      }
    }
  }
#endif //else of defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)
#elif defined(UNIX)
#endif
  return 0;
}
//---------------------------------------------------------------------------
void AssociateSteem(EasyStr Exts,EasyStr FileClass,bool Force,char *TypeDisplayName,int IconNum,bool IconOnly)
{
#ifdef WIN32

#if defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)

  LONG ErrorCode;
  HKEY Key;
  EasyStr KeyName,OriginalKeyName;
  unsigned long Size=400;

  // check before Exts will change
  bool WasAlreadyAssociated=IsSteemAssociated(Exts); 

  if (Exts[0]!='.') 
    Exts.Insert(".",0);
  Exts.Insert(key_location,0);

#if defined(SSE_VAR_MAY_REMOVE_ASSOCIATION)
/*  If this version of Steem is already associated, we come here to
    remove the association. We didn't create a separate function to spare
    some code.
*/
  if(WasAlreadyAssociated)
    RegDelnode (HKEY_CURRENT_USER, Exts.c_str() );
  else
#endif
  {
    // create key
    Exts+="\\Shell\\OpenSteem\\Command";
    ErrorCode=RegCreateKeyEx(HKEY_CURRENT_USER,Exts.Text,0,NULL,REG_OPTION_NON_VOLATILE,
      KEY_ALL_ACCESS,NULL,&Key,NULL);
    //TRACE_LOG("RegCreateKeyEx %s ErrorCode %d\n",Exts.Text,ErrorCode);

    // set value
    EasyStr ThisExeName=GetEXEFileName();
    EasyStr Command=EasyStr("\"")+ThisExeName+"\" \"%1\"";
    ErrorCode=RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)Command.Text,Command.Length()+1);
    //TRACE_LOG("RegSetValueEx %s ErrorCode %d\n",(BYTE*)Command.Text,ErrorCode);
    RegCloseKey(Key);
  }
#undef LOGSECTION
#else// defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)

  if (Exts[0]!='.') Exts.Insert(".",0);

  HKEY Key;
  EasyStr KeyName,OriginalKeyName;
  long Size=400;
  bool OriginalForce=Force;

  if (RegOpenKey(HKEY_CLASSES_ROOT,Exts,&Key)==ERROR_SUCCESS){
    Size=400;
    KeyName.SetLength(Size);
    RegQueryValue(Key,NULL,KeyName.Text,&Size);
    RegCloseKey(Key);

    OriginalKeyName=KeyName;
  }else{
    Force=true;
  }

  // If pointing at .st and only Steem on run list move to better FileClass key
  bool IgnoreExts=true;
  if (KeyName.Empty()){
    if (RegOpenKey(HKEY_CLASSES_ROOT,Exts+"\\Shell",&Key)==ERROR_SUCCESS){
      KeyName.SetLength(500);

      int n=0;
      while (RegEnumKey(Key,n++,KeyName.Text,500)==ERROR_SUCCESS){
        if (NotSameStr_I(KeyName,"OpenSteem")){
          IgnoreExts=0;
          KeyName=Exts;
          break;
        }
      }
      RegCloseKey(Key);
    }
    if (IgnoreExts){
      KeyName=FileClass;
      Force=true;
    }
  }

  if (NotSameStr_I(KeyName,FileClass) && NotSameStr_I(KeyName,FileClass+"_"+(Exts.Text+1)) &&
        Force && IgnoreExts){
    // If the file class is not the standard Steem class then it could be used for multiple
    // extensions. To avoid Steem associating itself with all those files make copy of
    // current key and associate in new key.
    EasyStr NewKey=FileClass+"_"+(Exts.Text+1);
    RegCopyKey(HKEY_CLASSES_ROOT,KeyName,HKEY_CLASSES_ROOT,NewKey);
    KeyName=NewKey;
  }

  if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName,&Key)!=ERROR_SUCCESS){
    Force=true; // File class key doesn't exist, force creation.
  }else{
    RegCloseKey(Key);
  }

  // Leave shell integration well alone
  if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName+"\\ShellEx",&Key)==ERROR_SUCCESS){
    RegCloseKey(Key);
    return;
  }
  if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName+"\\CLSID",&Key)==ERROR_SUCCESS){
    RegCloseKey(Key);
    return;
  }

  // Set .st key default value to the type name we are about to make Steem entry for
  // (might be same as it was)
  if (RegCreateKey(HKEY_CLASSES_ROOT,Exts,&Key)!=ERROR_SUCCESS) return;
  RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)KeyName.Text,strlen(KeyName)+1);
  if (NotSameStr_I(OriginalKeyName,KeyName)){
    if (strstr(OriginalKeyName.UpperCase(),FileClass.UpperCase())==NULL){
      // Backup old name, unless it is a Steem class
      RegSetValueEx(Key,"Steem_Backup",0,REG_SZ,(BYTE*)OriginalKeyName.Text,strlen(KeyName)+1);
    }
  }
  RegCloseKey(Key);

  if (Force){
    // Set display name of file type (default value of type class key)
    if (RegCreateKey(HKEY_CLASSES_ROOT,KeyName,&Key)!=ERROR_SUCCESS) return;
    RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)TypeDisplayName,strlen(TypeDisplayName)+1);
    RegCloseKey(Key);
  }

  if (IconOnly==0){
    // Add steem to the run list
    if (RegCreateKey(HKEY_CLASSES_ROOT,KeyName+"\\Shell\\OpenSteem",&Key)!=ERROR_SUCCESS) return;
    RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)CStrT("Open with Steem"),16);
    RegCloseKey(Key);

    // Get default program to run
    if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName+"\\Shell",&Key)!=ERROR_SUCCESS) return;
    Size=400;
    EasyStr Default;
    Default.SetLength(Size);
    RegQueryValue(Key,NULL,Default.Text,&Size);
    if (Default.Empty()) Default="open";

    HKEY Key2;
    if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName+"\\Shell\\"+Default,&Key2)!=ERROR_SUCCESS){
      // No default, make Steem default
      Force=true;
    }else{
      RegCloseKey(Key2);
    }

    if (Force) RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)"OpenSteem",10);
    RegCloseKey(Key);
  }

  // Find out if we should add the Command key with the path of Steem, only
  // if there is no command key, or we are forcing
  bool SetToThisEXE=bool(IconOnly ? 0:true);
  if (Force==0 && IconOnly==0){
    if (RegOpenKey(HKEY_CLASSES_ROOT,KeyName+"\\Shell\\OpenSteem\\Command",&Key)==ERROR_SUCCESS){
      SetToThisEXE=0;
      RegCloseKey(Key);
    }
  }

  EasyStr ThisExeName=GetEXEFileName();
  if (SetToThisEXE){
    // Set command key
    if (RegCreateKey(HKEY_CLASSES_ROOT,KeyName+"\\Shell\\OpenSteem\\Command",&Key)!=ERROR_SUCCESS) return;

    EasyStr Command=EasyStr("\"")+ThisExeName+"\" \"%1\"";
    RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)Command.Text,Command.Length()+1);

    RegCloseKey(Key);
  }

  if (RegCreateKey(HKEY_CLASSES_ROOT,KeyName+"\\DefaultIcon",&Key)!=ERROR_SUCCESS) return;
  EasyStr IconStr=ThisExeName+","+IconNum;
  if (OriginalForce){
    // Only set to Steem icon if the user chooses to associate with Steem
    RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)IconStr.Text,IconStr.Length()+1);
  }else{
    // If icon is from this EXE make sure it is using at the right number
    // If no icon then set to Steem's icon (may as well)
    Size=400;
    EasyStr CurIconStr;
    CurIconStr.SetLength(Size);
    RegQueryValue(Key,NULL,CurIconStr.Text,&Size);
    char *comma=strchr(CurIconStr,',');
    if (comma){
      *comma=0;
      if (IsSameStr_I(CurIconStr,ThisExeName)){
        RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)IconStr.Text,IconStr.Length()+1);
      }
    }else if (CurIconStr.Empty()){
      RegSetValueEx(Key,NULL,0,REG_SZ,(BYTE*)IconStr.Text,IconStr.Length()+1);
    }
  }
  RegCloseKey(Key);

#endif// defined(STEVEN_SEAGAL) && defined(SSE_VAR_ASSOCIATE_CU)

#elif defined(UNIX)
#endif
}
//---------------------------------------------------------------------------
