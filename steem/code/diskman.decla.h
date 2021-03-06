#pragma once
#ifndef DISKMAN_DECLA_H
#define DISKMAN_DECLA_H

#define DISK_UNCOMPRESSED 1
#define DISK_COMPRESSED 2
#define DISK_PASTI 3



#define FileIsDisk(s) ExtensionIsDisk(strrchr(s,'.'))

#ifdef SSE_UNIX
#include <x/hxc_dir_lv.h>
#endif
#if defined(SSE_DISK_PASTI_AUTO_SWITCH)
int ExtensionIsDisk(char*);
#else
int ExtensionIsDisk(char*,bool returnPastiDisksOnlyWhenPastiOn=true);
#endif


#if defined(SSE_COMPILER_STRUCT_391)

#pragma pack(push, STRUCTURE_ALIGNMENT)

typedef struct{
  EasyStr Name,Path,LinkPath;  
  int Image;
  bool UpFolder,Folder,ReadOnly,BrokenLink,Zip;

// Could implement the next few, in a million years!
//
//  EasyStr IconPath;int IconIdx;
//  EasyStr Description
}DiskManFileInfo;

#define DISKVIEWSCROLL_TIMER_ID 1
#define MSACONV_TIMER_ID 2

class TDiskManager : public TStemDialog
{
  //DATA
public: //TODO
  EasyStr HistBack[10],HistForward[10];
  EasyStr SaveSelPath;
  EasyStr ContentsLinksPath;
  EasyStr DisksFol,HomeFol,ContentListsFol;
  EasyStr QuickFol[10];
  struct _DM_INSERT_STRUCT{
    EasyStr Name,Path,DiskInZip;
  }InsertHist[2][10];
  DiskManFileInfo PropInf;
#ifdef WIN32
  EasyStr MultipleLinksPath,LinksTargetPath;
  WINDOWPROC Old_ListView_WndProc;
  HIMAGELIST il[2];
  HWND DragLV;
  HIMAGELIST DragIL;
  HWND DiskView;
  HICON DriveIcon[2],AccurateFDCIcon,DisableDiskIcon;
  HWND DatabaseDiag,ContentDiag,DiskDiag,LinksDiag,
#if !defined(SSE_VAR_NO_WINSTON)
    ImportDiag,
#endif
    PropDiag,DiagFocus;
  HANDLE MSAConvProcess;
  BPBINFO bpbi,file_bpbi,final_bpbi;
  int Dragging,DragWidth,DragHeight,DropTarget;
  int LastOverID;
  int MenuTarget;  
  int SaveScroll;
  int ContentConflictAction;
  int Width,Height,FSWidth,FSHeight;
  int IconSpacing;
  int DoubleClickAction;
  Str MSAConvPath;
  Str MSAConvSel;
  EasyStringList MenuESL;
  EasyStringList contents_sl;
  WORD BytesPerSectorIdx,SecsPerTrackIdx,TracksIdx,SidesIdx;
  bool DragEntered,EndingDrag;
#if !defined(SSE_VAR_NO_WINSTON)
  bool Importing;
#endif
#elif defined(UNIX)
  BPBINFO bpbi,file_bpbi,final_bpbi;
  EasyStringList contents_sl;
	int HistBackLength,HistForwardLength;
  int ArchiveTypeIdx;
  int SaveScroll;
  int ContentConflictAction;
  int Width,Height,FSWidth,FSHeight;
  int IconSpacing;
  int DoubleClickAction;
  bool TempEject_InDrive[2];
  Str TempEject_Name,TempEject_DiskInZip[2];
  hxc_dir_lv dir_lv;
  hxc_button UpBut,BackBut,ForwardBut,eject_but[2];
  hxc_button DirOutput,disk_name[2],drive_icon[2];
  hxc_button HomeBut,SetHomeBut,MenuBut;
  WORD BytesPerSectorIdx,SecsPerTrackIdx,TracksIdx,SidesIdx;
#endif//win32?
	bool HideBroken,CloseAfterIRR;
#if defined(SSE_GUI_DM_SHOW_EXT)
  bool HideExtension;
#endif
  bool Maximized,FSMaximized;
#if defined(SSE_GUI_DM_INSERT_DISK_B_REMOVE)//in fact warning
  BYTE SmallIcons,AutoInsert2;
#else
  bool SmallIcons,AutoInsert2;
#endif
  bool EjectDisksWhenQuit;
#ifdef WIN32
  bool AtHome;
  bool ExplorerFolders;
#ifndef SSE_VAR_NO_WINSTON
  EasyStr WinSTonPath,WinSTonDiskPath,ImportPath;
  bool ImportOnlyIfExist;
  int ImportConflictAction;
#endif
  bool DoExtraShortcutCheck;
#elif defined(UNIX)
	hxc_button HardBut;
#endif//win32?
  //FUNCTION
private:
  void PerformInsertAction(int,EasyStr,EasyStr,EasyStr);
  void ExtractArchiveToSTHardDrive(Str);
  static void GCGetCRC(char*,DWORD*,int);
  static BYTE* GCConvertToST(char *,int,int *);
  void GetContentsSL(Str);
  bool GetContentsCheckExist();
  Str GetContentsGetAppendName(Str);
#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall Drive_Icon_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall DiskView_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall DriveView_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall Dialog_WndProc(HWND,UINT,WPARAM,LPARAM);
  static int CALLBACK CompareFunc(LPARAM,LPARAM,LPARAM);
  void BeginDrag(int,HWND),MoveDrag(),EndDrag(int,int,bool);
#ifndef SSE_VAR_NO_WINSTON
  HRESULT CreateLinkCheckForOverwrite(char *,char *,IShellLink *,IPersistFile *);
  bool ImportDiskExists(char *,EasyStr &);
#endif
  bool DoCreateMultiLinks()
#ifndef SSE_VAR_NO_WINSTON    
    ,DoImport()
#endif
  ;
  void AddFoldersToMenu(HMENU,int,EasyStr,bool);
  bool MoveOrCopyFile(bool,char*,char*,char*,bool);
  void PropShowFileInfo(int);
  void AddFileOrFolderContextMenu(HMENU,DiskManFileInfo *);
  void UpdateBPBFiles(Str,Str,bool);
  void ManageWindowClasses(bool);
  Str GetMSAConverterPath();
  void GoToDisk(Str,bool);
#elif defined(UNIX)
  static int WinProc(TDiskManager*,Window,XEvent*);
  void set_path(EasyStr,bool=true,bool=true);
  void UpdateDiskNames(int);
  void ToggleReadOnly(int);
  Str GetCustomDiskImage(int*,int*,int*);
  void set_home(Str);
  static int dir_lv_notify_handler(hxc_dir_lv*,int,int);
  static int button_notify_handler(hxc_button*,int,int*);
	static int menu_popup_notifyproc(hxc_popup*,int,int);
  static int diag_lv_np(hxc_listview *,int,int);
  static int diag_but_np(hxc_button *,int,int*);
  static int diag_ed_np(hxc_edit *,int,int);
#endif
public:
  TDiskManager();
  ~TDiskManager() { Hide(); }
  void Show(),Hide();
  bool ToggleVisible();
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);
  void SwapDisks(int);
  bool InsertDisk(int,EasyStr,EasyStr,bool=0,bool=true,EasyStr="",bool=0,bool=0);
  void EjectDisk(int);
  bool AreNewDisksInHistory(int);
  void InsertHistoryAdd(int,char *,char *,char* = "");
  void InsertHistoryDelete(int,char *,char *,char* = "");
  bool CreateDiskImage(char *,int,int,int);
  EasyStr CreateDiskName(char *,char *);
  void SetNumFloppies(int);
  void ExtractDisks(Str);
  void InitGetContents();
  void ShowDatabaseDiag(),ShowContentDiag();

#ifdef WIN32
  bool HasHandledMessage(MSG*);
  void SetDir(EasyStr,bool,EasyStr="",bool=0,EasyStr="",int=0);
  bool SelectItemWithPath(char *,bool=0,char* = NULL);
  bool SelectItemWithLinkPath(char *LinkPath,bool EditLabel=0)
  {
    return SelectItemWithPath(NULL,EditLabel,LinkPath);
  }
  void RefreshDiskView(EasyStr SelPath="",bool EditLabel=0,EasyStr SelLinkPath="",int iItem=0);
  int GetSelectedItem();
  DiskManFileInfo *GetItemInf(int iItem,HWND LV=NULL)
  {
    LV_ITEM lvi;
    lvi.iItem=iItem;
    lvi.iSubItem=0;
    lvi.mask=LVIF_PARAM;
    SendMessage(HWND(LV ? LV:DiskView),LVM_GETITEM,0,(LPARAM)&lvi);
    return (DiskManFileInfo*)lvi.lParam;
  }
  void ShowLinksDiag(),
#if !defined(SSE_VAR_NO_WINSTON)
    ShowImportDiag(),
#endif
    ShowPropDiag(),ShowDiskDiag();
  int GetDiskSelectionSize();
  void SetDiskViewMode(int);
  void LoadIcons();

  void SetDriveViewEnable(int,bool);

#ifdef SSE_X64_LPTR
  HWND VisibleDiag() { return HWND(int64_t(DiskDiag) | int64_t(LinksDiag) 
#if !defined(SSE_VAR_NO_WINSTON)
    | int64_t(ImportDiag) 
#endif
    | int64_t(PropDiag) | int64_t(ContentDiag) | int64_t(DatabaseDiag)); }
#else
  HWND VisibleDiag() { return HWND(long(DiskDiag) | long(LinksDiag)
#if !defined(SSE_VAR_NO_WINSTON)    
    | long(ImportDiag) 
#endif
    | long(PropDiag) | long(ContentDiag) | long(DatabaseDiag)); }  
#endif
#elif defined(UNIX)
	void RefreshDiskView(Str="");
#endif
};


#pragma pack(pop, STRUCTURE_ALIGNMENT)

#else

typedef struct{
  EasyStr Name,Path,LinkPath;
  bool UpFolder,Folder,ReadOnly,BrokenLink,Zip;
  int Image;

// Could implement the next few, in a million years!
//
//  EasyStr IconPath;int IconIdx;
//  EasyStr Description
}DiskManFileInfo;

#define DISKVIEWSCROLL_TIMER_ID 1
#define MSACONV_TIMER_ID 2

class TDiskManager : public TStemDialog
{
private:
  EasyStr HistBack[10],HistForward[10];
  void PerformInsertAction(int,EasyStr,EasyStr,EasyStr);
  void ExtractArchiveToSTHardDrive(Str);
  static void GCGetCRC(char*,DWORD*,int);
  static BYTE* GCConvertToST(char *,int,int *);
  void GetContentsSL(Str);
  bool GetContentsCheckExist();
  Str GetContentsGetAppendName(Str);

#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall Drive_Icon_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall DiskView_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall DriveView_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall Dialog_WndProc(HWND,UINT,WPARAM,LPARAM);
  static int CALLBACK CompareFunc(LPARAM,LPARAM,LPARAM);
  void BeginDrag(int,HWND),MoveDrag(),EndDrag(int,int,bool);
#ifndef SSE_VAR_NO_WINSTON
  HRESULT CreateLinkCheckForOverwrite(char *,char *,IShellLink *,IPersistFile *);
  bool ImportDiskExists(char *,EasyStr &);
#endif
  bool DoCreateMultiLinks()
#ifndef SSE_VAR_NO_WINSTON    
    ,DoImport()
#endif
  ;
  void AddFoldersToMenu(HMENU,int,EasyStr,bool);
  bool MoveOrCopyFile(bool,char*,char*,char*,bool);
  void PropShowFileInfo(int);
  void AddFileOrFolderContextMenu(HMENU,DiskManFileInfo *);
  void UpdateBPBFiles(Str,Str,bool);
  void ManageWindowClasses(bool);
  Str GetMSAConverterPath();
  void GoToDisk(Str,bool);

  WINDOWPROC Old_ListView_WndProc;
  HIMAGELIST il[2];

  int Dragging,DragWidth,DragHeight,DropTarget;
  HWND DragLV;
  bool DragEntered,EndingDrag;
  HIMAGELIST DragIL;
  int LastOverID;
  int MenuTarget;
  EasyStringList MenuESL;

  Str MSAConvPath;
  HANDLE MSAConvProcess;
  Str MSAConvSel;
#if !defined(SSE_VAR_NO_WINSTON)
  bool Importing;
#endif
#elif defined(UNIX)
	int HistBackLength,HistForwardLength;

  static int WinProc(TDiskManager*,Window,XEvent*);

  void set_path(EasyStr,bool=true,bool=true);
  void UpdateDiskNames(int);
  void ToggleReadOnly(int);
  Str GetCustomDiskImage(int*,int*,int*);
  void set_home(Str);

  static int dir_lv_notify_handler(hxc_dir_lv*,int,int);
  static int button_notify_handler(hxc_button*,int,int*);
	static int menu_popup_notifyproc(hxc_popup*,int,int);
  static int diag_lv_np(hxc_listview *,int,int);
  static int diag_but_np(hxc_button *,int,int*);
  static int diag_ed_np(hxc_edit *,int,int);

  int ArchiveTypeIdx;
  bool TempEject_InDrive[2];
  Str TempEject_Name,TempEject_DiskInZip[2];

  hxc_dir_lv dir_lv;
  hxc_button UpBut,BackBut,ForwardBut,eject_but[2];
  hxc_button DirOutput,disk_name[2],drive_icon[2];
  hxc_button HomeBut,SetHomeBut,MenuBut;
#endif
public:
  TDiskManager();
  ~TDiskManager() { Hide(); }
  void Show(),Hide();
  bool ToggleVisible();
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);
  void SwapDisks(int);
  bool InsertDisk(int,EasyStr,EasyStr,bool=0,bool=true,EasyStr="",bool=0,bool=0);
  void EjectDisk(int);
  bool AreNewDisksInHistory(int);
  void InsertHistoryAdd(int,char *,char *,char* = "");
  void InsertHistoryDelete(int,char *,char *,char* = "");
  bool CreateDiskImage(char *,int,int,int);
  EasyStr CreateDiskName(char *,char *);
  void SetNumFloppies(int);
  void ExtractDisks(Str);
  void InitGetContents();
  void ShowDatabaseDiag(),ShowContentDiag();

#ifdef WIN32
  bool HasHandledMessage(MSG*);
  void SetDir(EasyStr,bool,EasyStr="",bool=0,EasyStr="",int=0);
  bool SelectItemWithPath(char *,bool=0,char* = NULL);
  bool SelectItemWithLinkPath(char *LinkPath,bool EditLabel=0)
  {
    return SelectItemWithPath(NULL,EditLabel,LinkPath);
  }
  void RefreshDiskView(EasyStr SelPath="",bool EditLabel=0,EasyStr SelLinkPath="",int iItem=0);
  int GetSelectedItem();
  DiskManFileInfo *GetItemInf(int iItem,HWND LV=NULL)
  {
    LV_ITEM lvi;
    lvi.iItem=iItem;
    lvi.iSubItem=0;
    lvi.mask=LVIF_PARAM;
    SendMessage(HWND(LV ? LV:DiskView),LVM_GETITEM,0,(LPARAM)&lvi);
    return (DiskManFileInfo*)lvi.lParam;
  }
  void ShowLinksDiag(),
#if !defined(SSE_VAR_NO_WINSTON)
    ShowImportDiag(),
#endif
    ShowPropDiag(),ShowDiskDiag();
  int GetDiskSelectionSize();
  void SetDiskViewMode(int);
  void LoadIcons();

  void SetDriveViewEnable(int,bool);

  HWND DiskView;
  HICON DriveIcon[2],AccurateFDCIcon,DisableDiskIcon;
  HWND DatabaseDiag,ContentDiag,DiskDiag,LinksDiag,
#if !defined(SSE_VAR_NO_WINSTON)
    ImportDiag,
#endif
    PropDiag,DiagFocus;
#ifdef SSE_X64_LPTR
  HWND VisibleDiag() { return HWND(int64_t(DiskDiag) | int64_t(LinksDiag) 
#if !defined(SSE_VAR_NO_WINSTON)
    | int64_t(ImportDiag) 
#endif
    | int64_t(PropDiag) | int64_t(ContentDiag) | int64_t(DatabaseDiag)); }
#else
  HWND VisibleDiag() { return HWND(long(DiskDiag) | long(LinksDiag)
#if !defined(SSE_VAR_NO_WINSTON)    
    | long(ImportDiag) 
#endif
    | long(PropDiag) | long(ContentDiag) | long(DatabaseDiag)); }  
#endif
  bool AtHome;
  bool ExplorerFolders;

#ifndef SSE_VAR_NO_WINSTON
  EasyStr WinSTonPath,WinSTonDiskPath,ImportPath;
  bool ImportOnlyIfExist;
  int ImportConflictAction;
#endif
  EasyStr MultipleLinksPath,LinksTargetPath;

  bool DoExtraShortcutCheck;
#elif defined(UNIX)
	hxc_button HardBut;
	void RefreshDiskView(Str="");
#endif
	bool HideBroken,CloseAfterIRR;
#if defined(SSE_GUI_DM_SHOW_EXT)
  bool HideExtension;
#endif
  int SaveScroll;
  EasyStr SaveSelPath;

  Str ContentsLinksPath;
  int ContentConflictAction;

  int Width,Height,FSWidth,FSHeight;
  bool Maximized,FSMaximized;

  EasyStr DisksFol,HomeFol,ContentListsFol;
  EasyStr QuickFol[10];
  struct _DM_INSERT_STRUCT{
    EasyStr Name,Path,DiskInZip;
  }InsertHist[2][10];

  DiskManFileInfo PropInf;
  BPBINFO bpbi,file_bpbi,final_bpbi;
#if defined(SSE_GUI_DM_INSERT_DISK_B_REMOVE)
  BYTE SmallIcons,AutoInsert2;
#else
  bool SmallIcons,AutoInsert2;
#endif
  int IconSpacing;
  bool EjectDisksWhenQuit;
  WORD BytesPerSectorIdx,SecsPerTrackIdx,TracksIdx,SidesIdx;

  EasyStringList contents_sl;

  int DoubleClickAction;
};


#endif//#if defined(SSE_COMPILER_STRUCT_391)



#ifdef WIN32
//void TDiskManager::RefreshDiskView(EasyStr SelPath,bool EditLabel,EasyStr SelLinkPath,int iItem);
#endif

#endif//DISKMAN_DECLA_H
