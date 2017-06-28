/*---------------------------------------------------------------------------
FILE: diskman.cpp
MODULE: Steem
DESCRIPTION: This file contains the code for Steem's disk manager. Some of
these functions are used in the emulation of Steem for vital processes such
as changing disk images and determining what files are disks.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: diskman.cpp")
#endif

#if defined(SSE_GUI_DM_NAME_CLIPBOARD)
#include <acc.decla.h>
#endif
#if defined(SSE_ACSI_RELOAD_TOS)
#include <loadsave.decla.h>
#endif

#if defined(SSE_BUILD) && defined(WIN32)
void TDiskManager::RefreshDiskView(EasyStr SelPath,bool EditLabel,EasyStr SelLinkPath,int iItem)
{
  SetDir(DisksFol,0,SelPath,EditLabel,SelLinkPath,iItem);
}
#endif


#define LOGSECTION LOGSECTION_IMAGE_INFO//SS

//---------------------------------------------------------------------------
bool ExtensionIsPastiDisk(char *Ext)
{
#if USE_PASTI
  if (Ext==NULL || hPasti==NULL) return false;
  if (*Ext=='.') Ext++;
#if defined(SSE_DISK_PASTI_AUTO_SWITCH4)
/*  v3.9.2. We change again the way pasti.dll works in Steem.
    If the option isn't checked (pasti_active is false), Pasti will run
    STX images only, without us changing pasti_active.
    If the option is checked, Pasti will get all DMA/FDC writes.
*/
  if(!pasti_active)
    return (IsSameStr_I(Ext,DISK_EXT_STX)) ? true : false;
#elif defined(SSE_DISK_PASTI_ONLY_STX)
  if(OPTION_PASTI_JUST_STX)
    return (IsSameStr_I(Ext,DISK_EXT_STX)) ? true : false;
#endif
  char *t=pasti_file_exts;
  while (*t){
    if (IsSameStr_I(Ext,t)) return true;
    t+=strlen(t)+1;
  }
#endif
  return false;
}
//---------------------------------------------------------------------------
#if defined(SSE_DISK_PASTI_AUTO_SWITCH)
int ExtensionIsDisk(char *Ext)
#else
int ExtensionIsDisk(char *Ext,bool returnPastiDisksOnlyWhenPastiOn)
#endif
{
  if (Ext==NULL) return 0;

  if (*Ext=='.') Ext++;
  int ret=0;
#ifdef SSE_DISK_EXT
  if (MatchesAnyString_I(Ext,DISK_EXT_ST,DISK_EXT_STT,DISK_EXT_DIM,DISK_EXT_MSA,
#if defined(SSE_DISK_CAPS)
    DISK_EXT_IPF, DISK_EXT_CTR,
#endif    
#if defined(SSE_DISK_SCP)
    DISK_EXT_SCP,
#endif    
#if defined(SSE_DISK_STW)
    DISK_EXT_STW,
#endif  
#if defined(SSE_DISK_HFE)
    DISK_EXT_HFE,
#endif  
    NULL)){
#else
  if (MatchesAnyString_I(Ext,"ST","STT","DIM","MSA",NULL)){
#endif
    ret=DISK_UNCOMPRESSED;
#if defined(SSE_TOS_PRG_AUTORUN)
  }else if (OPTION_PRG_SUPPORT 
    && MatchesAnyString_I(Ext,DISK_EXT_PRG,DISK_EXT_TOS,NULL)){
    ret=DISK_UNCOMPRESSED;
#endif
  }else if (MatchesAnyString_I(Ext,"STZ","ZIP",NULL)){
    ret=DISK_COMPRESSED;

#ifdef RAR_SUPPORT 
  }else if (MatchesAnyString_I(Ext,"RAR",NULL)){
    ret=DISK_COMPRESSED;
#endif
#if defined(SSE_VAR_UNRAR)
  }else if(UNRAR_OK&&MatchesAnyString_I(Ext,"RAR",NULL)){
    ret=DISK_COMPRESSED;
#endif
#if defined(SSE_VAR_ARCHIVEACCESS)
#if defined(SSE_VAR_ARCHIVEACCESS2)
  }else if(ARCHIVEACCESS_OK
    &&MatchesAnyString_I(Ext,"7Z","BZ2","GZ","TAR","ARJ",NULL)){
#else
  }else if(ARCHIVEACCESS_OK&&MatchesAnyString_I(Ext,"7Z",NULL)){
#endif
    ret=DISK_COMPRESSED;
#endif
  }

#if USE_PASTI
  if (hPasti 
#if !(defined(SSE_DISK_PASTI_ALWAYS_DISPLAY_STX_DISKS))
    && pasti_active
#endif
    ){
    if (ExtensionIsPastiDisk(Ext)){
      return DISK_PASTI;
    }else if (ret==DISK_COMPRESSED){
      return ret;
    }
#if !(defined(SSE_DISK_PASTI_AUTO_SWITCH))    
    else{
      if (returnPastiDisksOnlyWhenPastiOn) return 0;
    }
#endif
  }
#endif

  return ret;
}
//---------------------------------------------------------------------------
EasyStr TDiskManager::CreateDiskName(char *Name,char *DiskInZip)
{
  EasyStr Ret=Name;
  if (DiskInZip[0]) Ret=Ret+" ("+DiskInZip+")";
  return Ret;
}
//---------------------------------------------------------------------------
void TDiskManager::PerformInsertAction(int Action,EasyStr Name,EasyStr Path,EasyStr DiskInZip)
{
  bool InsertSucceeded=true;
  if (Path.NotEmpty()){
    InsertSucceeded=InsertDisk((Action==1) ? 1:0,Name,Path,0,true,DiskInZip,0,true);
  }else{
    EjectDisk((Action==1) ? 1:0);
  }
  if (InsertSucceeded && Action==2){

#ifdef WIN32
    if (CloseAfterIRR && Handle) PostMessage(Handle,WM_CLOSE,0,0);
    if (IsIconic(StemWin)) OpenIcon(StemWin);
    SetForegroundWindow(StemWin);
#else
    if (SetForegroundWindow(StemWin)==0) return;
    if (CloseAfterIRR && Handle){
      XEvent SendEv;
      SendEv.type=ClientMessage;
      SendEv.xclient.window=Handle;
      SendEv.xclient.message_type=hxc::XA_WM_PROTOCOLS;
      SendEv.xclient.format=32;
      SendEv.xclient.data.l[0]=hxc::XA_WM_DELETE_WINDOW;
      XSendEvent(XD,Handle,0,0,&SendEv);
    }
#endif

    reset_st(RESET_COLD | RESET_NOSTOP | RESET_CHANGESETTINGS | RESET_BACKUP);
    if (runstate!=RUNSTATE_RUNNING){
      PostRunMessage();
    }else{
#if defined(SSE_GUI_MOUSE_CAPTURE_OPTION)
      if(OPTION_CAPTURE_MOUSE)
        SetStemMouseMode(STEM_MOUSEMODE_WINDOW);
      else
        SetStemMouseMode(STEM_MOUSEMODE_DISABLED);
#else
      SetStemMouseMode(STEM_MOUSEMODE_WINDOW);
#endif
      osd_init_run(true);
    }
  }
}
//---------------------------------------------------------------------------
void TDiskManager::SetNumFloppies(int NewNum)
{
  ASSERT( num_connected_floppies==1 || num_connected_floppies==2 );
  ASSERT( NewNum==1 || NewNum==2 );
  num_connected_floppies=NewNum;

#if defined(SSE_DISK_CAPS)
  // it's not easy mixing IPF and native!
  if(SF314[1].ImageType.Manager==MNGR_CAPS)
  {
    if(NewNum==1) // deactivated
      Caps.SF314[1].diskattr&=~CAPSDRIVE_DA_IN;
    else // active
      Caps.SF314[1].diskattr|=CAPSDRIVE_DA_IN;
  } // maybe there's more to do for TOS
  Caps.WD1772.drivecnt/*max*/=NewNum; // or drivecnt?
#endif

#if USE_PASTI
  if (hPasti){
    struct pastiCONFIGINFO pci;
    pci.flags=PASTI_CFDRIVES;
    pci.ndrives=NewNum;
    pci.drvFlags=0;
    pasti->Config(&pci);
  }
#endif

#ifdef WIN32
  if (Handle) if (GetDlgItem(Handle,99)) InvalidateRect(GetDlgItem(Handle,99),NULL,0);
#elif defined(UNIX) //SS obviously a bit coded with a unix tool!
	if (Handle){
		if (num_connected_floppies==2){
			drive_icon[1].set_icon(&Ico32,ICO32_DRIVE_B);
		}else{
			drive_icon[1].set_icon(&Ico32,ICO32_DRIVE_B_OFF);
		}
	}
#endif
  CheckResetDisplay();
#if defined(SSE_GUI_STATUS_BAR)
  GUIRefreshStatusBar();
#endif
}
//---------------------------------------------------------------------------
#ifdef WIN32 // SS: big block

//#define LVS_SMALLVIEW LVS_SMALLICON
#define LVS_SMALLVIEW LVS_LIST

#include "diskman_diags.cpp"
#include "diskman_drag.cpp"
//---------------------------------------------------------------------------
TDiskManager::TDiskManager()
{
#ifdef SSE_ACSI_ICON // more space for the ACSI icon in initial disk manager
  Width=403+70+GetSystemMetrics(SM_CXFRAME)*2+GetSystemMetrics(SM_CXVSCROLL);
#else 
  Width=403+GetSystemMetrics(SM_CXFRAME)*2+GetSystemMetrics(SM_CXVSCROLL);
#endif
  Height=331+GetSystemMetrics(SM_CYCAPTION);
  Left=(GetSystemMetrics(SM_CXSCREEN)-Width)/2;
  Top=(GetSystemMetrics(SM_CYSCREEN)-Height)/2;

  FSWidth=Width;FSHeight=Height;
  FSLeft=320 - FSWidth/2;
  FSTop=240 - FSHeight/2;

  Section="Disks";

  il[0]=NULL;il[1]=NULL;

  Dragging=-1;DropTarget=-1;

  HideBroken=0;
#if defined(SSE_GUI_DM_SHOW_EXT)
  HideExtension=true;
#endif

  DiskDiag=NULL;
#if !defined(SSE_VAR_NO_WINSTON)
  LinksDiag=NULL;
  ImportDiag=NULL;
#endif
  PropDiag=NULL;ContentDiag=NULL;DatabaseDiag=NULL;

  BytesPerSectorIdx=2;SecsPerTrackIdx=1;TracksIdx=5;SidesIdx=1;

  SaveScroll=0;

  SmallIcons=0;
  IconSpacing=1;

  DoubleClickAction=2;

  DoExtraShortcutCheck=0;

  EjectDisksWhenQuit=0;

  MSAConvProcess=NULL;
#ifndef SSE_VAR_NO_WINSTON
  HKEY Key;
  WinSTonPath="C:\\Program Files\\WinSTon";
  if (RegOpenKey(HKEY_CURRENT_USER,"Software\\WinSTon",&Key)==ERROR_SUCCESS){
    DWORD Size=500;
    EasyStr Path;
    Path.SetLength(Size);
    if (RegQueryValueEx(Key,"InstalledDirectory",NULL,NULL,(BYTE*)Path.Text,&Size)==ERROR_SUCCESS){
      WinSTonPath=Path;
    }
    RegCloseKey(Key);
  }
  NO_SLASH(WinSTonPath);
  WinSTonDiskPath=WinSTonPath+"\\Discs";
#endif
  MSAConvPath="";
#ifndef SSE_VAR_NO_WINSTON
  ImportOnlyIfExist=true;
  ImportConflictAction=0;

  ContentConflictAction=2;
#endif
}
//---------------------------------------------------------------------------
void TDiskManager::ManageWindowClasses(bool Unreg)
{
  WNDCLASS wc;
  char *ClassName[3]={"Steem Disk Manager","Steem Disk Manager Dialog","Steem Disk Manager Drive Icon"};
  if (Unreg){
    for (int n=0;n<3;n++) UnregisterClass(ClassName[n],Inst);
  }else{
    RegisterMainClass(WndProc,ClassName[0],RC_ICO_DISKMAN);

    wc.style=CS_DBLCLKS;
    wc.lpfnWndProc=Dialog_WndProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=Inst;
    wc.hIcon=hGUIIconSmall[RC_ICO_DRIVE];
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName=NULL;
    wc.lpszClassName=ClassName[1];
    RegisterClass(&wc);

    wc.style=0;
    wc.lpfnWndProc=Drive_Icon_WndProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=Inst;
    wc.hIcon=NULL;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName=NULL;
    wc.lpszClassName=ClassName[2];
    RegisterClass(&wc);
  }
}
//---------------------------------------------------------------------------
void TDiskManager::Show()
{
  if (Handle!=NULL){
    if (IsIconic(Handle)) ShowWindow(Handle,SW_SHOWNORMAL);
    SetForegroundWindow(Handle);
    return;
  }
//  if (FullScreen) Top=max(Top,MENUHEIGHT);

  bool MaximizeIt=bool(FullScreen ? FSMaximized:Maximized);

  ManageWindowClasses(SD_REGISTER);

  Handle=CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_APPWINDOW,"Steem Disk Manager",T("Disk Manager"),
      WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
      Left,Top,Width,Height,ParentWin,NULL,HInstance,NULL);

  if (HandleIsInvalid()){
    ManageWindowClasses(SD_UNREGISTER);
    return;
  }
#if defined(SSE_X64_LPTR)
  SetWindowLongPtr(Handle, GWLP_USERDATA,(LONG_PTR)this);
#else
  SetWindowLong(Handle,GWL_USERDATA,(long)this);
#endif

  MakeParent(HWND(FullScreen ? StemWin:NULL));

  HWND Win;

  int Countdown=10;
  for(;;){
    DiskView=CreateWindowEx(WS_EX_ACCEPTFILES | 512,WC_LISTVIEW,"",WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                      LVS_ICON | LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_EDITLABELS,
                      10,105,480,200,Handle,(HMENU)102,HInstance,NULL);
/*
SS
Another way Vista is a pile!
When you click on an item of the list, the OS emits a stupid beep. 
Not Steem's fault.
Made a "reg" file based on info below.
--

Run Regedit and then delete the default value for
 HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\CCSelect\.current


HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\CCSelect\.current\(Default) has a null value set. Just double click it and empty out anything in it if there is and then click ok. Sound should go away.
To duplicate the effect just insert some bad text in the field.


Okay, I found a solution.  Apparently this IS a Vista-specific bug, and is not related to my code.  I found the answer on a Google Group: http://groups.google.com.au/group/microsoft.public.win32.programmer.gdi/browse_thread/thread/99a2b1f68b47c4d5/681bf4f839f72ee8
 
 
 
The solution is to run regedit and browse to HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\CCSelect\.current\
 
then double-click the (Default) value, clear out any data that's in that key, then click OK.  (Click OK even if it appears blank.)  Then the beep goes away on its own.

  +
http://www.microsoft-questions.com/microsoft/Platform-SDK-Shell/32138755/vista-listview-beep-problem.aspx


*/
    if (DiskView) break;
    Sleep(50);
    if ((--Countdown)<=0){
      DestroyWindow(Handle);Handle=NULL;
      ManageWindowClasses(SD_UNREGISTER);
      return;
    }
  }
  LoadIcons();
  ListView_SetImageList(DiskView,il[0],LVSIL_NORMAL);
  ListView_SetImageList(DiskView,il[1],LVSIL_SMALL);

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_BACK),WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP |
                PBS_RIGHTCLICK,10,80,21,21,Handle,(HMENU)82,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Back"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_FORWARD),WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP |
                PBS_RIGHTCLICK,33,80,21,21,Handle,(HMENU)83,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Forward"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_HOME),WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                PBS_RIGHTCLICK,56,80,21,21,Handle,(HMENU)80,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("To home folder"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_SETHOME),WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                PBS_RIGHTCLICK,79,80,21,21,Handle,(HMENU)81,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Make this folder your home folder"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_DRIVEDROPDOWN),WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                PBS_RIGHTCLICK,102,80,21,21,Handle,(HMENU)84,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Disk Manager options"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_DISKMANTOOLS),WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                PBS_RIGHTCLICK,125,80,21,21,Handle,(HMENU)85,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Disk image management tools"));

  Win=CreateWindow("Combobox","",WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                CBS_HASSTRINGS | CBS_DROPDOWNLIST,
                125+26,80,45,200,Handle,(HMENU)90,HInstance,NULL);
  char Root[4]={0,':','\\',0};
  for (int d=0;d<27;d++){
    Root[0]=char('A'+d);
    if (GetDriveType(Root)>1){
#if defined(SSE_X64_390)
      SendMessage(Win,CB_ADDSTRING,0,(LPARAM)Root);
#else
      SendMessage(Win,CB_ADDSTRING,0,(long)Root);
#endif
    }
  }

  CreateWindowEx(512,"Steem Path Display","",WS_CHILD | WS_VISIBLE,
                125+26+50,80,300,20,Handle,(HMENU)97,HInstance,NULL);


  Win=CreateWindow("Steem Disk Manager Drive Icon","A",WS_CHILD | WS_VISIBLE,
                10,10,64,64,Handle,(HMENU)98,HInstance,NULL);

  int Disabled=(AreNewDisksInHistory(0) ? 0:WS_DISABLED);
  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_SMALLDOWNARROW),WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | Disabled,
                52,52,12,12,Win,(HMENU)100,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Drive A disk history"));

  Win=CreateWindowEx(WS_EX_ACCEPTFILES | 512,WC_LISTVIEW,"",WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP |
                    LVS_ICON | LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_NOSCROLL,
                    75,10,90,64,Handle,(HMENU)100,HInstance,NULL);
  ListView_SetIconSpacing(Win,88,200);
  ListView_SetImageList(Win,il[0],LVSIL_NORMAL);
  SetDriveViewEnable(0,0);
  Win=CreateWindow("Steem Disk Manager Drive Icon","B",WS_CHILD | WS_VISIBLE,
                175,10,64,64,Handle,(HMENU)99,HInstance,NULL);

  Disabled=(AreNewDisksInHistory(1) ? 0:WS_DISABLED);
  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_SMALLDOWNARROW),WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | Disabled,
                52,52,12,12,Win,(HMENU)100,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Drive B disk history"));

  Win=CreateWindowEx(WS_EX_ACCEPTFILES | 512,WC_LISTVIEW,"",WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP |
                    LVS_ICON | LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_NOSCROLL,
                    240,10,90,64,Handle,(HMENU)101,HInstance,NULL);
  ListView_SetIconSpacing(Win,88,200);
  ListView_SetImageList(Win,il[0],LVSIL_NORMAL);
  SetDriveViewEnable(1,0);

  {
    int ico=RC_ICO_HARDDRIVES;
    if (IsSameStr_I(T("File"),"Fichier")) ico=RC_ICO_HARDDRIVES_FR;
#if defined(SSE_GUI_DM_RGT_CLK_HD2)
    Win=CreateWindow("Steem Flat PicButton",Str(ico),
      WS_CHILD | WS_VISIBLE | WS_TABSTOP | PBS_RIGHTCLICK,
                  400,10,60,64,Handle,(HMENU)10,HInstance,NULL);
#else
    Win=CreateWindow("Steem Flat PicButton",Str(ico),WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                  400,10,60,64,Handle,(HMENU)10,HInstance,NULL);
#endif
#if defined(SSE_ACSI_ICON)
    ToolAddWindow(ToolTip,Win,T("GEMDOS Hard Drive Manager - right click to toggle on/off"));
#else
    ToolAddWindow(ToolTip,Win,T("Hard Drive Manager"));
#endif

#ifdef SSE_ACSI_ICON
    ico=RC_ICO_HARDDRIVES_ACSI;
    DWORD style=WS_CHILD | WS_VISIBLE | WS_TABSTOP | PBS_RIGHTCLICK;
#if !defined(SSE_ACSI_HDMAN)
    if(!SSEConfig.AcsiImg)//with no manager...
      style|=WS_DISABLED;
#endif
    Win=CreateWindow("Steem Flat PicButton",Str(ico),style,
                  400,10,60,64,Handle,(HMENU)11,HInstance,NULL);
     SendMessage(Win,BM_SETCHECK,SSEOption.Acsi,0);
#if defined(SSE_ACSI_HDMAN)
    ToolAddWindow(ToolTip,Win,T("ACSI Hard Drive Manager - right click to toggle on/off"));
#else
    ToolAddWindow(ToolTip,Win,T("ACSI disks - right click to toggle on/off"));
#endif
#endif

  }

  SetWindowAndChildrensFont(Handle,Font);
#if defined(SSE_X64_LPTR)
  SetWindowLongPtr(GetDlgItem(Handle, 98), GWLP_USERDATA,(LONG_PTR) this);
  SetWindowLongPtr(GetDlgItem(Handle, 99), GWLP_USERDATA,(LONG_PTR) this);
  Old_ListView_WndProc = (WNDPROC)GetClassLongPtr(GetDlgItem(Handle, 100), GCLP_WNDPROC);
  SetWindowLongPtr(GetDlgItem(Handle, 100), GWLP_USERDATA,(LONG_PTR) this);
  SetWindowLongPtr(GetDlgItem(Handle, 100), GWLP_WNDPROC, (LONG_PTR)DriveView_WndProc);
  SetWindowLongPtr(GetDlgItem(Handle, 101), GWLP_USERDATA,(LONG_PTR) this);
  SetWindowLongPtr(GetDlgItem(Handle, 101), GWLP_WNDPROC,(LONG_PTR) DriveView_WndProc);
  SetWindowLongPtr(GetDlgItem(Handle, 102), GWLP_USERDATA,(LONG_PTR) this);
  SetWindowLongPtr(GetDlgItem(Handle, 102), GWLP_WNDPROC,(LONG_PTR) DiskView_WndProc);
#else
  SetWindowLong(GetDlgItem(Handle,98),GWL_USERDATA,(long)this);
  SetWindowLong(GetDlgItem(Handle,99),GWL_USERDATA,(long)this);

  Old_ListView_WndProc=(WINDOWPROC)GetClassLong(GetDlgItem(Handle,100),GCL_WNDPROC);
  SetWindowLong(GetDlgItem(Handle,100),GWL_USERDATA,(long)this);
  SetWindowLong(GetDlgItem(Handle,100),GWL_WNDPROC,(long)DriveView_WndProc);
  SetWindowLong(GetDlgItem(Handle,101),GWL_USERDATA,(long)this);
  SetWindowLong(GetDlgItem(Handle,101),GWL_WNDPROC,(long)DriveView_WndProc);
  SetWindowLong(GetDlgItem(Handle,102),GWL_USERDATA,(long)this);
  SetWindowLong(GetDlgItem(Handle,102),GWL_WNDPROC,(long)DiskView_WndProc);
#endif
  for (int i=0;i<2;i++){
    if (FloppyDrive[i].DiskInDrive()){
      InsertDisk(i,FloppyDrive[i].DiskName,FloppyDrive[i].GetDisk(),true,0,FloppyDrive[i].DiskInZip);
    }
  }

  ShowWindow(Handle,int(MaximizeIt ? SW_MAXIMIZE:SW_SHOW));
  UpdateWindow(Handle);

  SetDiskViewMode(SmallIcons ? LVS_SMALLVIEW:LVS_ICON);
  RefreshDiskView();
  SetFocus(DiskView);

  if (StemWin) PostMessage(StemWin,WM_USER,1234,0);
}
//---------------------------------------------------------------------------
void TDiskManager::LoadIcons()
{
  if (Handle==NULL) return;

  HIMAGELIST old_il[2]={il[0],il[1]};
  HICON *pIcons=hGUIIcon;
  for (int n=0;n<2;n++){
    il[n]=ImageList_Create(32-n*16,32-n*16,BPPToILC[BytesPerPixel] | ILC_MASK,9,9);
    if (il[n]){
      ImageList_AddIcon(il[n],pIcons[RC_ICO_FOLDER]);
      ImageList_AddIcon(il[n],pIcons[RC_ICO_DRIVE]);
      ImageList_AddIcon(il[n],pIcons[RC_ICO_PARENTDIR]);
      ImageList_AddIcon(il[n],pIcons[RC_ICO_FOLDERLINK]);
      ImageList_AddIcon(il[n],pIcons[RC_ICO_DRIVELINK]);
      ImageList_AddIcon(il[n],pIcons[RC_ICO_DRIVEREADONLY]);
      ImageList_AddIcon(il[n],pIcons[RC_ICO_FOLDERBROKEN]);
      ImageList_AddIcon(il[n],pIcons[RC_ICO_DRIVEBROKEN]);
      if (FloppyArchiveIsReadWrite){
        ImageList_AddIcon(il[n],pIcons[RC_ICO_DRIVEZIPPED_RW]);
      }else{
        ImageList_AddIcon(il[n],pIcons[RC_ICO_DRIVEZIPPED_RO]);
      }
    }
    pIcons=hGUIIconSmall;
  }
#if defined(SSE_X64_LPTR)
  if (VisibleDiag()) SetClassLongPtr(VisibleDiag(), GCLP_HICON, long(hGUIIconSmall[RC_ICO_DRIVE]));
#else
  if (VisibleDiag()) SetClassLong(VisibleDiag(),GCL_HICON,long(hGUIIconSmall[RC_ICO_DRIVE]));
#endif
  if (GetDlgItem(Handle,10)){
    // Update controls
    for (int id=80;id<90;id++){
      if (GetDlgItem(Handle,id)) PostMessage(GetDlgItem(Handle,id),BM_RELOADICON,0,0);
    }
    PostMessage(GetDlgItem(Handle,10),BM_RELOADICON,0,0);
#ifdef SSE_ACSI_ICON 
    PostMessage(GetDlgItem(Handle,11),BM_RELOADICON,0,0);
#endif
    PostMessage(GetDlgItem(GetDlgItem(Handle,98),100),BM_RELOADICON,0,0);
    PostMessage(GetDlgItem(GetDlgItem(Handle,99),100),BM_RELOADICON,0,0);
    InvalidateRect(GetDlgItem(Handle,98),NULL,true);
    InvalidateRect(GetDlgItem(Handle,99),NULL,true);

    ListView_SetImageList(GetDlgItem(Handle,100),il[0],LVSIL_NORMAL);
    ListView_SetImageList(GetDlgItem(Handle,101),il[0],LVSIL_NORMAL);
    ListView_SetImageList(DiskView,il[0],LVSIL_NORMAL);
    ListView_SetImageList(DiskView,il[1],LVSIL_SMALL);
    SendMessage(DiskView,LVM_REDRAWITEMS,0,SendMessage(DiskView,LVM_GETITEMCOUNT,0,0));
  }

  for (int n=0;n<2;n++) if (old_il[n]) ImageList_Destroy(old_il[n]);
}
//---------------------------------------------------------------------------
void TDiskManager::SetDiskViewMode(int Mode)
{
  SetWindowLong(DiskView,GWL_STYLE,(GetWindowLong(DiskView,GWL_STYLE) & ~LVS_SMALLVIEW & ~LVS_ICON) | Mode);
  if (SmallIcons){
    WIDTHHEIGHT widh=GetTextSize(Font,"Width of y Line in small icon view");
    widh.Width/=2;
    if (IconSpacing==1) widh.Width*=2;
    if (IconSpacing==2) widh.Width*=4;
    ListView_SetColumnWidth(DiskView,DWORD(-1),18+widh.Width);
  }else{
    WIDTHHEIGHT widh=GetTextSize(Font,"8");
    ListView_SetIconSpacing(DiskView,32+24+IconSpacing*12,38 + (widh.Height+2)*2);
  }
#ifdef SSE_X64_390
  SendMessage(DiskView,LVM_SORTITEMS,0,(LPARAM)CompareFunc);
#else
  SendMessage(DiskView,LVM_SORTITEMS,0,(long)CompareFunc);
#endif
  if (SmallIcons==0) SendMessage(DiskView,LVM_ARRANGE,LVA_DEFAULT,0);
}
//---------------------------------------------------------------------------
void TDiskManager::Hide()
{
  if (Handle==NULL) return;

  HardDiskMan.Hide();
  if (HardDiskMan.Handle) return;

  ShowWindow(Handle,SW_HIDE);
  if (FullScreen) SetFocus(StemWin);

  ToolsDeleteAllChildren(ToolTip,Handle);
  ToolsDeleteAllChildren(ToolTip,GetDlgItem(Handle,98));
  ToolsDeleteAllChildren(ToolTip,GetDlgItem(Handle,99));

  int c=SendMessage(DiskView,LVM_GETITEMCOUNT,0,0);
  for (int i=0;i<c;i++){
    SendMessage(DiskView,LVM_DELETEITEM,0,0);
  }

  DestroyWindow(Handle);Handle=NULL;DiskView=NULL;

  for (int n=0;n<2;n++){
    ImageList_Destroy(il[n]);il[n]=NULL;
  }

  if (StemWin) PostMessage(StemWin,WM_USER,1234,0);

  ManageWindowClasses(SD_UNREGISTER);
}
//---------------------------------------------------------------------------
void TDiskManager::SetDir(EasyStr NewFol,bool AddToHistory,
                            EasyStr SelPath,bool EditLabel,EasyStr SelLinkPath,int iSelItem)
{
  EasyStr Fol=NewFol;
  if (Fol.RightChar()!='\\' && Fol.RightChar()!='/') Fol+="\\";

  WIN32_FIND_DATA wfd;
  HANDLE Find=FindFirstFile(Fol+"*.*",&wfd);
  if (Find!=INVALID_HANDLE_VALUE){
    SetCursor(LoadCursor(NULL,IDC_WAIT));

    {
      SetWindowText(GetDlgItem(Handle,97),Fol.Lefts(Fol.Length()-1).Text+min(3,Fol.Length()-1));
#if defined(SSE_X64_390)
      int idx=SendMessage(GetDlgItem(Handle,90),CB_FINDSTRING,0xffffffff,(LPARAM)((Fol.Lefts(2)+"\\").Text));
#else
      int idx=SendMessage(GetDlgItem(Handle,90),CB_FINDSTRING,0xffffffff,(long)((Fol.Lefts(2)+"\\").Text));
#endif
      if (idx>-1) SendMessage(GetDlgItem(Handle,90),CB_SETCURSEL,idx,0);
      UpdateWindow(GetDlgItem(Handle,97));
      UpdateWindow(GetDlgItem(Handle,90));
    }

    SendMessage(DiskView,WM_SETREDRAW,0,0);
    int c=SendMessage(DiskView,LVM_GETITEMCOUNT,0,0);
    for (int i=0;i<c;i++){
      SendMessage(DiskView,LVM_DELETEITEM,0,0);
    }

    if (SmallIcons){
      SetDiskViewMode(LVS_ICON);
      SetDiskViewMode(LVS_SMALLVIEW);
    }

    IShellLink *LinkObj=NULL;
    IPersistFile *FileObj=NULL;

    HRESULT hres=CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(void**)&LinkObj);
    if (SUCCEEDED(hres)==0) LinkObj=NULL;

    if (LinkObj){
      hres=LinkObj->QueryInterface(IID_IPersistFile,(void**)&FileObj);
      if (SUCCEEDED(hres)==0) FileObj=NULL;
    }

    DynamicArray<DiskManFileInfo*> Files;
    Files.Resize(512); // Assume for 512 items
    Files.SizeInc=128; // Increase by 128 items at a time

    EasyStr Name,Path,Extension,LinkPath;
    char *exts;
    DiskManFileInfo *Inf;
    bool Link,Broken;
    do{
      bool Add=true;
      if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
        if (wfd.cFileName[0]=='.' && wfd.cFileName[1]==0){
          Add=0;
        }else if (wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM){
          Add=0;
        }
      }
      if (Add){
        Link=0;

        Name=wfd.cFileName;
        Path=Fol+Name;
        LinkPath="";
        Extension="";
        Broken=0;
        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0){
          exts=strrchr(Name,'.');
          if (exts!=NULL){
#if defined(SSE_GUI_DM_SHOW_EXT)
            if(HideExtension)
#endif
              *exts=0; //Strip extension from Name
            Extension=exts+1;
            strupr(Extension);
            if (IsSameStr_I(Extension,"LNK")){
              Link=true;
              LinkPath=Path;

              Path=GetLinkDest(Fol+wfd.cFileName,&wfd,NULL,LinkObj,FileObj);
              NO_SLASH(Path);

              if (Path.NotEmpty() && DoExtraShortcutCheck){
                HANDLE hFind=FindFirstFile(Path,&wfd);
                if (hFind!=INVALID_HANDLE_VALUE){
                  FindClose(hFind);

                  EasyStr DestFilePath=Path;
                  RemoveFileNameFromPath(DestFilePath,WITH_SLASH);
                  Path=DestFilePath+wfd.cFileName;
                }
              }

              if (Path.NotEmpty()){
                UINT HostDriveType=GetDriveType(Path.Lefts(2)+"\\");
                if (HostDriveType==DRIVE_NO_ROOT_DIR){
                  Broken=true;
                }else if (HostDriveType!=DRIVE_REMOVABLE && HostDriveType!=DRIVE_CDROM){
                  if (Path.Length()!=2) Broken=(GetFileAttributes(Path)==0xffffffff);
                }

                exts=strrchr(wfd.cFileName,'.');
                if (exts!=NULL){
                  Extension=exts+1;
                  strupr(Extension);
                }
              }
            }
          }
        }
        if (Path.NotEmpty() && (Broken==0 || HideBroken==0)){
          if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && NOT(Broken && wfd.nFileSizeLow==1)){
            Inf=new DiskManFileInfo;
            Inf->Folder=true;
            Inf->ReadOnly=0;
            Inf->BrokenLink=Broken;
            Inf->Zip=0;
            if (Name==".."){
              Inf->Image=2;
              Inf->Name=T("Parent Directory");
              Str HigherPath=Fol;
              NO_SLASH(HigherPath);
              RemoveFileNameFromPath(HigherPath,REMOVE_SLASH);
              Inf->Path=HigherPath;
              Inf->LinkPath="";
              Inf->UpFolder=true;
            }else{
              Inf->Image=Link*3 + Broken*3;
              Inf->Name=Name;
              Inf->Path=Path;
              Inf->LinkPath=LinkPath;
              Inf->UpFolder=0;
            }
            Files.Add(Inf);
          }else{
            int Type=ExtensionIsDisk(Extension);
            if (Type==DISK_UNCOMPRESSED || Type==DISK_PASTI){
              Inf=new DiskManFileInfo;
              Inf->Name=Name;
              Inf->Path=Path;
              Inf->LinkPath=LinkPath;
              Inf->Folder=0;
              Inf->UpFolder=0;
              if (Broken){
                Inf->ReadOnly=0;
              }else{
                Inf->ReadOnly=(wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
              }
              Inf->BrokenLink=Broken;
              Inf->Zip=0;
              Inf->Image=1 + Link*3 + Broken*3;
              if (Inf->ReadOnly && Link==0) Inf->Image=5;

              Files.Add(Inf);
            }else if (Type==DISK_COMPRESSED && enable_zip){
              Inf=new DiskManFileInfo;
              Inf->Name=Name;
              Inf->Path=Path;
              Inf->LinkPath=LinkPath;
              Inf->Folder=0;
              Inf->UpFolder=0;
              Inf->ReadOnly=true;
              Inf->BrokenLink=Broken;
              Inf->Zip=true;
              Inf->Image=int(Link ? 4 + Broken*3:8);
              Files.Add(Inf);
            }
          }
        }
      }
    }while (FindNextFile(Find,&wfd));
    FindClose(Find);

    if (LinkObj) LinkObj->Release();
    if (FileObj) FileObj->Release();

    SendMessage(DiskView,LVM_SETITEMCOUNT,Files.NumItems+16,0);
    LV_ITEM lvi;
    lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.iItem=0;
    lvi.iSubItem=0;
    lvi.pszText=LPSTR_TEXTCALLBACK;
    for (int n=0;n<Files.NumItems;n++){
#ifdef SSE_X64_390
      lvi.lParam=(LPARAM)(Files[n]);
#else
      lvi.lParam=long(Files[n]);
#endif
      lvi.iImage=Files[n]->Image;
      SendMessage(DiskView,LVM_INSERTITEM,0,LPARAM(&lvi));
    }
    Files.DeleteAll();
    SendMessage(DiskView,LVM_SORTITEMS,0,LPARAM(CompareFunc));
//    SetDiskViewMode(SmallIcons ? LVS_SMALLVIEW:LVS_ICON);

    if (NotSameStr_I(NewFol,DisksFol)){
      if (AddToHistory){
        HistForward[0]="";
        for (int n=9;n>0;n--){
          HistBack[n]=HistBack[n-1];
        }
        HistBack[0]=DisksFol;
        EnableWindow(GetDlgItem(Handle,82),true);
        if (GetFocus()==GetDlgItem(Handle,83)) SetFocus(GetDlgItem(Handle,82));
        EnableWindow(GetDlgItem(Handle,83),0);

      }
      DisksFol=NewFol;
      NO_SLASH(DisksFol);
    }

    if (IsSameStr_I(DisksFol,HomeFol)){
      if (GetFocus()==GetDlgItem(Handle,80) || GetFocus()==GetDlgItem(Handle,81)){
        SetFocus(GetDlgItem(Handle,90));
      }
      AtHome=true;
    }else{
      AtHome=0;
    }

    if (SelPath.NotEmpty() || SelLinkPath.NotEmpty()){
      if (SelectItemWithPath(SelPath,EditLabel,SelLinkPath)==0){
        SelPath="";SelLinkPath="";
      }
    }
    if (SelPath.IsEmpty() && SelLinkPath.IsEmpty()){
      lvi.stateMask=LVIS_SELECTED | LVIS_FOCUSED;
      lvi.state=LVIS_SELECTED | LVIS_FOCUSED;
      iSelItem=bound(iSelItem,0,max(SendMessage(DiskView,LVM_GETITEMCOUNT,0,0)-1,0L));
#if defined(SSE_X64_390)
      SendMessage(DiskView,LVM_SETITEMSTATE,iSelItem,(LPARAM)&lvi);
#else
      SendMessage(DiskView,LVM_SETITEMSTATE,iSelItem,(long)&lvi);
#endif
      SendMessage(DiskView,LVM_ENSUREVISIBLE,iSelItem,1);
    }

    SendMessage(DiskView,WM_SETREDRAW,1,0);
    InvalidateRect(DiskView,NULL,true);
    UpdateWindow(DiskView);

    SetCursor(PCArrow);
  }
}
//---------------------------------------------------------------------------
int CALLBACK TDiskManager::CompareFunc(LPARAM lPar1, LPARAM lPar2, LPARAM)
{
  //SS the place to change how we sort items, but need to edit DiskManFileInfo
  DiskManFileInfo *Inf1=(DiskManFileInfo*)lPar1;
  DiskManFileInfo *Inf2=(DiskManFileInfo*)lPar2;

  if (Inf1->UpFolder){
    return -1;
  }else if (Inf2->UpFolder){
    return 1;
  }else if (Inf1->Folder && Inf2->Folder==0){
    return -1;
  }else if (Inf1->Folder==0 && Inf2->Folder){
    return 1;
  }else{
    return strcmpi(Inf1->Name.Text,Inf2->Name.Text);
  }
}
//---------------------------------------------------------------------------
bool TDiskManager::SelectItemWithPath(char *Path,bool EditLabel,char *LinkPath)
{
  int c=SendMessage(DiskView,LVM_GETITEMCOUNT,0,0);
  bool Match;
  LV_ITEM lvi;
  lvi.mask=LVIF_PARAM;
  lvi.iSubItem=0;
  for (lvi.iItem=0;lvi.iItem<c;lvi.iItem++){
    SendMessage(DiskView,LVM_GETITEM,0,(LPARAM)&lvi);
    Match=true;
    if (Path)     Match&=bool(Path[0] ? IsSameStr_I(((DiskManFileInfo*)lvi.lParam)->Path,Path):true);
    if (LinkPath) Match&=bool(LinkPath[0] ? IsSameStr_I(((DiskManFileInfo*)lvi.lParam)->LinkPath,LinkPath):true);
    if (Match){
      lvi.stateMask=LVIS_SELECTED | LVIS_FOCUSED;
      lvi.state=    LVIS_SELECTED | LVIS_FOCUSED;
      SendMessage(DiskView,LVM_SETITEMSTATE,lvi.iItem,(LPARAM)&lvi);
      SendMessage(DiskView,LVM_ENSUREVISIBLE,lvi.iItem,1);
      if (EditLabel) SendMessage(DiskView,LVM_EDITLABEL,lvi.iItem,0);
      return true;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
int TDiskManager::GetSelectedItem()
{
  int c=SendMessage(DiskView,LVM_GETITEMCOUNT,0,0);
  LV_ITEM lvi;
  lvi.iSubItem=0;
  for (lvi.iItem=0;lvi.iItem<c;lvi.iItem++){
    if (SendMessage(DiskView,LVM_GETITEMSTATE,lvi.iItem,LVIS_SELECTED)){
      return lvi.iItem;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
bool TDiskManager::HasHandledMessage(MSG *mess)
{
  if (Handle!=NULL && Dragging==-1){
#if defined(SSE_VS2008_WARNING_390)
    if (VisibleDiag()){
      return (IsDialogMessage(VisibleDiag(),mess)!=0);
    }else{
      return (IsDialogMessage(Handle,mess)!=0);
    }
#else
    if (VisibleDiag()){
      return IsDialogMessage(VisibleDiag(),mess);
    }else{
      return IsDialogMessage(Handle,mess);
    }
#endif
  }else{
    return 0;
  }
}
//---------------------------------------------------------------------------
void TDiskManager::AddFoldersToMenu(HMENU Pop,int StartID,EasyStr NoAddFol,bool Setting)
{
  int MaxWidth=GetSystemMetrics(SM_CXSCREEN)/2;
  if (NotSameStr_I(HomeFol,NoAddFol)){
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,StartID,
            EasyStr(Setting ? "(":"")+ShortenPath(HomeFol,Font,MaxWidth)+(Setting ? ")":""));
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
  }
  if (Setting){
    StartID+=5;
  }else{
    StartID++;
  }
  for (int n=0;n<10;n++){
    if (Setting){
      HMENU OptionsPop=CreatePopupMenu();

      InsertMenu(OptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,StartID+0,
                  T("Change to Current Folder"));
      InsertMenu(OptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,StartID+1,
                  T("Change to..."));
      InsertMenu(OptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,StartID+2,
                  T("Erase"));

      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,(UINT)OptionsPop,
                  EasyStr(1+n)+": ("+ShortenPath(QuickFol[n],Font,MaxWidth)+")");
      StartID+=5;
    }else if (QuickFol[n].NotEmpty()){
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING |
                  (IsSameStr_I(QuickFol[n],NoAddFol) ? (MF_DISABLED | MF_GRAYED):0),
                  StartID++,EasyStr(1+n)+": "+ShortenPath(QuickFol[n],Font,MaxWidth));
    }else{
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED,StartID++,EasyStr(1+n)+":");
    }
  }
}
//---------------------------------------------------------------------------
Str TDiskManager::GetMSAConverterPath()
{
  if (MSAConvPath.NotEmpty()){
    //if (Exists(MSAConvPath)) return MSAConvPath;
  }

#if defined(SSE_GUI_MSA_CONVERTER)
#if defined(_DEBUG)
  BRK(Auto MSA path wont work in _DEBUG);
#else
	// We don't prompt player if Msa.exe is in Steem's folder.
	// We are in a WIN32 block.
	
	// First check is msa.exe is on the steem path,
	// if yes, no need to ask player

	EasyStr str(RunDir);	// RunDir contains the app folder
	str+="\\msa.exe";	// notice double \    <- this was a stupid bug!///
	if(Exists(str))
	{
		MSAConvPath=str;	// MSAConvParth is an "EasyStr"
		return MSAConvPath;
	}
#endif	// NDBG
#endif	// SS

  int i=Alert(T("Have you installed MSA Converter elsewhere on this computer?"),T("Run MSA Converter"),MB_ICONQUESTION | MB_YESNO);
  if (i==IDYES){
    Str Fol=MSAConvPath;
    if (Fol.NotEmpty()){
      RemoveFileNameFromPath(Fol,REMOVE_SLASH);
    }else{
      Fol="C:\\Program Files";
      ITEMIDLIST *idl;
      if (SHGetSpecialFolderLocation(NULL,CSIDL_PROGRAM_FILES,&idl)==NOERROR){
        IMalloc *Mal;SHGetMalloc(&Mal);
        Fol.SetLength(MAX_PATH);
        SHGetPathFromIDList(idl,Fol);
        Mal->Free(idl);
      }
      NO_SLASH(Fol);
    }
    EnableAllWindows(0,Handle);
    EasyStr NewMSA=FileSelect(HWND(FullScreen ? StemWin:Handle),T("Run MSA Converter"),
                                Fol,FSTypes(1,T("Executables").Text,"*.exe",NULL),1,true,"exe");
    if (NewMSA.NotEmpty()) MSAConvPath=NewMSA;
    SetForegroundWindow(Handle);
    EnableAllWindows(true,Handle);
    return MSAConvPath;
  }else{
    i=Alert(T("MSA Converter is a free Windows program to edit disk images and convert them between different formats.")+" "+
            T("It has great features like converting archives containing files into disk images.")+"\r\n\r\n"+
            T("Would you like to open the MSA Converter website now so you can find out more and download it?"),
            T("Run MSA Converter"),MB_ICONQUESTION | MB_YESNO);
    if (i==IDYES) ShellExecute(NULL,NULL,MSACONV_WEB,"","",SW_SHOWNORMAL);
  }
  return "";
}
//---------------------------------------------------------------------------
void TDiskManager::AddFileOrFolderContextMenu(HMENU Pop,DiskManFileInfo *Inf)
{
  bool AddProperties=0;

  if (Inf->UpFolder==0){
    if (Inf->BrokenLink==0){
      if (Inf->Folder==0){
        int MultiDisk=0;
        HMENU IAPop=NULL,IBPop=NULL,IRRPop=NULL;
        MenuESL.DeleteAll();
        MenuESL.Sort=eslSortByNameI;
        if (Inf->Zip){
          zippy.list_contents(Inf->Path,&MenuESL,true);
          if (MenuESL.NumStrings>1){
            MultiDisk=MF_POPUP;
            IAPop=CreatePopupMenu(),IBPop=CreatePopupMenu(),IRRPop=CreatePopupMenu();
            for (int i=0;i<min(MenuESL.NumStrings,200);i++){
              InsertMenu(IAPop,0xffffffff,MF_BYPOSITION | MF_STRING,9000+i,MenuESL[i].String);
              InsertMenu(IBPop,0xffffffff,MF_BYPOSITION | MF_STRING,9200+i,MenuESL[i].String);
              InsertMenu(IRRPop,0xffffffff,MF_BYPOSITION | MF_STRING,9400+i,MenuESL[i].String);
            }
          }
        }

        AddProperties=true;

        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MultiDisk,
                        (UINT)((MultiDisk==0) ? 1010:int(IAPop)),T("Insert Into Drive &A"));
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MultiDisk,
                        (UINT)((MultiDisk==0) ? 1011:int(IBPop)),T("Insert Into Drive &B"));
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MultiDisk,
                        (UINT)((MultiDisk==0) ? 1012:int(IRRPop)),T("Insert, Reset and &Run"));
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);

        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1015,T("Get &Contents"));
        HMENU ContentsPop=CreatePopupMenu();
        AddFoldersToMenu(ContentsPop,7000,"",0);
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,int(ContentsPop),
                        T("Get Contents and Create Shortcuts In"));
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,0,NULL);

        if (Inf->LinkPath.NotEmpty()){
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1090,T("&Go To Disk"));
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1092,T("Open Disk's Folder in Explorer"));
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
        }
        if (Inf->Zip==0){
          Inf->ReadOnly=(access(Inf->Path,2)!=0);
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(Inf->ReadOnly ? MF_CHECKED:0),1040,T("Read &Only"));
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
        }else{
          if (MenuESL.NumStrings){    // There are disks in archive
            if (MultiDisk){
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1080,T("E&xtract Disks Here"));
            }else{
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1080,T("E&xtract Disk Here"));
            }
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
          }
        }

        HMENU MSAPop=CreatePopupMenu();
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_POPUP,(UINT)MSAPop,"MSA Converter");

        int FileZip=0;
        if (Inf->Zip){
          //SS MSA Converter doesn't work with RAR files
          if (MenuESL.NumStrings==0 && has_extension(Inf->Path,"zip")) FileZip=true;
        }
        if (FileZip){
          InsertMenu(MSAPop,0xffffffff,MF_BYPOSITION | MF_STRING,2034,T("Convert to Disk Image"));
        }else{
          InsertMenu(MSAPop,0xffffffff,MF_BYPOSITION | MF_STRING,2031,T("Open Disk Image"));
          bool AddedSep=0;
          for (int d=2;d<26;d++){
            if (mount_flag[d]){
              if (AddedSep==0){
                InsertMenu(MSAPop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
                AddedSep=true;
              }
              InsertMenu(MSAPop,0xffffffff,MF_BYPOSITION | MF_STRING,2040+d,T("Extract Contents to ST Hard Drive")+" "+char('A'+d)+":");
            }
          }
        }
#if defined(SSE_GUI_STW_CONVERT)
/*  To help our MFM disk image format, we add a right click option to convert
    regular images to STW.
    Works with archives too.
*/
        if(!Inf->Folder && !Inf->UpFolder 
#if USE_PASTI
          && !pasti_active // pasti_active would interfere in SetDisk
#endif
          )
        {
          char *ext=NULL;
          char *dot=strrchr(Inf->Path,'.');
          if (dot) ext=dot+1;
          if(ext && (IsSameStr_I(ext,DISK_EXT_ST)||IsSameStr_I(ext,DISK_EXT_MSA)
            ||IsSameStr_I(ext,DISK_EXT_DIM)||Inf->Zip))
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1041,T("Convert to ST&W"));
         }
#endif
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
      }else{
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1060,T("Open in &Explorer"));
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1061,EasyStr(T("&Find..."))+" \10F3");
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
      }
    }else{
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1070,T("&Fix Shortcut"));
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
    }
    if (Inf->LinkPath.NotEmpty()){
      HMENU MoveLinkPop=CreatePopupMenu();
      AddFoldersToMenu(MoveLinkPop,6060,DisksFol,0);
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,(UINT)MoveLinkPop,T("&Move Shortcut To"));
      HMENU CopyLinkPop=CreatePopupMenu();
      AddFoldersToMenu(CopyLinkPop,6080,DisksFol,0);
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,(UINT)CopyLinkPop,T("&Copy Shortcut To"));
    }
    EasyStr MoveToText=T("&Move Disk To"),CopyToText=T("&Copy Disk To"),
            ShortcutToText=T("Create &Shortcut To Disk In");
    if (Inf->Folder){
      MoveToText=T("&Move Folder To"),CopyToText=T("&Copy Folder To"),
            ShortcutToText=T("Create &Shortcut To Folder In");
    }
    Str FolderContainingDisk=Inf->Path;
    RemoveFileNameFromPath(FolderContainingDisk,REMOVE_SLASH);
    HMENU MovePop=CreatePopupMenu();
    AddFoldersToMenu(MovePop,6000,FolderContainingDisk,0);
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,(UINT)MovePop,MoveToText);
    HMENU CopyPop=CreatePopupMenu();
    AddFoldersToMenu(CopyPop,6020,FolderContainingDisk,0);
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,(UINT)CopyPop,CopyToText);
    if (Inf->LinkPath.Empty()){
      HMENU LinkPop=CreatePopupMenu();
      AddFoldersToMenu(LinkPop,6040,"",0);
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,(UINT)LinkPop,
                  ShortcutToText);
    }
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);

    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1020,EasyStr(T("&Rename"))+" \10F2");
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1030,EasyStr(T("Delete"))+" \10DEL");
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
    if (AddProperties){
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1099,T("Properties"));
#if !defined(SSE_GUI_390)
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
#endif
    }
    InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
  }
}
//---------------------------------------------------------------------------
void TDiskManager::GoToDisk(Str Path,bool Refresh)
{
  EasyStr Fol=Path;
  char *slash=strrchr(Fol,'\\');
  if (slash) *slash=0;
  if (IsSameStr_I(Fol,DisksFol)==0){
    SetDir(Fol,true,Path);
  }else{
    if (Refresh){
      RefreshDiskView(Path);
    }else{
      SelectItemWithPath(Path);
    }
  }
  SetFocus(DiskView);
}
//---------------------------------------------------------------------------
#if defined(SSE_GUI_STW_CONVERT) 
/*  Helper function for case 1041
    What counts here is code size, not performance 
    mode 0 normal
         1 missing clock
         2 crc
*/

void wd1772_write_stw(BYTE data,TWD1772MFM* wd1772mfm,TWD1772Crc* wd1772crc,
                      int& p,int mode) {
  if(!mode)
    wd1772crc->Add(data);
  wd1772mfm->data=data;
  wd1772mfm->Encode( (mode==1) ? (TWD1772MFM::FORMAT_CLOCK) : (TWD1772MFM::NORMAL_CLOCK)); 
  if(mode==1)
    wd1772crc->Reset();
  ImageSTW[1].SetMfmData(p++,wd1772mfm->encoded);
}

#define WD1772_WRITE(d) wd1772_write_stw(d,&wd1772mfm,&wd1772crc,p,0);
#define WD1772_WRITE_A1 wd1772_write_stw(0xA1,&wd1772mfm,&wd1772crc,p,1);
#define WD1772_WRITE_CRC(d) wd1772_write_stw(d,&wd1772mfm,&wd1772crc,p,2);

#endif
//---------------------------------------------------------------------------
#if defined(SSE_X64_LPTR)
#define GET_THIS This=(TDiskManager*)GetWindowLongPtr(Win,GWLP_USERDATA);
#else
#define GET_THIS This=(TDiskManager*)GetWindowLong(Win,GWL_USERDATA);
#endif
#pragma warning (disable: 4701) //lvi in case WM_USER//390
LRESULT __stdcall TDiskManager::WndProc(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar)
{
  LRESULT Ret=DefStemDialogProc(Win,Mess,wPar,lPar);
  if (StemDialog_RetDefVal) return Ret;

  TDiskManager *This;
  switch (Mess){
    case WM_COMMAND:
    {
      GET_THIS;
      switch (LOWORD(wPar)){
        case IDCANCEL: //Esc
        {
          if (This->Dragging>-1) break;

          int SelItem=This->GetSelectedItem();
          if (SelItem>-1){
            DiskManFileInfo *Inf=This->GetItemInf(SelItem);
            if (Inf->LinkPath.NotEmpty()){
              This->RefreshDiskView("",0,Inf->LinkPath);
            }else{
              This->RefreshDiskView(Inf->Path);
            }
          }else{
            This->RefreshDiskView();
          }
          break;
        }
        case 10:  //Hard Drives
#if defined(SSE_GUI_DM_RGT_CLK_HD2)
/*  Right click on HD manager icon toggles HD
*/
          {
            HWND icon_handle=GetDlgItem(Win,10);
            if (SendMessage(icon_handle,BM_GETCLICKBUTTON,0,0)==2)
            {
              HardDiskMan.DisableHardDrives=!HardDiskMan.DisableHardDrives;
              TRACE_INIT("Option GEMDOS HD %d\n",!HardDiskMan.DisableHardDrives);
              HardDiskMan.update_mount();
#if defined(SSE_GUI_STATUS_BAR)
              GUIRefreshStatusBar();
#endif
#ifdef SSE_GUI_DM_HD_SELECTED
              SendMessage(icon_handle,BM_SETCHECK,!HardDiskMan.DisableHardDrives,0);
#endif
            }
            else if (HIWORD(wPar)==BN_CLICKED){
              SendMessage(icon_handle,BM_SETCHECK,1,0);
              HardDiskMan.Show();
            }
          }
#else
          if (HIWORD(wPar)==BN_CLICKED){
            SendMessage(GetDlgItem(Win,10),BM_SETCHECK,1,0);
            HardDiskMan.Show();
          }
#endif
          break;

#ifdef SSE_ACSI_ICON 
        case 11:
          {
            HWND icon_handle=GetDlgItem(Win,LOWORD(wPar));

            if (SendMessage(icon_handle,BM_GETCLICKBUTTON,0,0)==2) //right click
            {
              SSEOption.Acsi=!SSEOption.Acsi;
              TRACE_INIT("Option ACSI %d\n",SSEOption.Acsi);
#if defined(SSE_ACSI_RELOAD_TOS)
              if(SSEOption.Acsi)
                load_TOS(ROMFile);
#endif
#if defined(SSE_GUI_STATUS_BAR)
              GUIRefreshStatusBar();
#endif
              SendMessage(icon_handle,BM_SETCHECK,SSEOption.Acsi,0);
            }
            else if (HIWORD(wPar)==BN_CLICKED){
#if defined(SSE_ACSI_HDMAN)
              SendMessage(icon_handle,BM_SETCHECK,1,0);
              AcsiHardDiskMan.Show();
#endif
            }
          }
          break;
#endif

        case 80:  // Go Home
          if (This->Dragging>-1) break;

          if (HIWORD(wPar)==BN_CLICKED){
          	bool InHome=IsSameStr_I(This->HomeFol,This->DisksFol);
            if (SendMessage(HWND(lPar),BM_GETCLICKBUTTON,0,0)==2 || InHome){
              SendMessage(HWND(lPar),BM_SETCHECK,1,0);
              HMENU Pop=CreatePopupMenu();

              This->AddFoldersToMenu(Pop,5000,This->DisksFol,0);

              RECT rc;
              GetWindowRect(HWND(lPar),&rc);
              TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              rc.left,rc.bottom,0,Win,NULL);

              DestroyMenu(Pop);
              SendMessage(HWND(lPar),BM_SETCHECK,0,0);
            }else{
              This->SetDir(This->HomeFol,true);
            }
          }
          break;
        case 81:  // Set Home
          if (HIWORD(wPar)==BN_CLICKED){
            SendMessage(HWND(lPar),BM_SETCHECK,1,0);
            if (SendMessage(HWND(lPar),BM_GETCLICKBUTTON,0,0)==2 || IsSameStr_I(This->HomeFol,This->DisksFol)){
              HMENU Pop=CreatePopupMenu();

              This->AddFoldersToMenu(Pop,8000,This->DisksFol,true);

              RECT rc;
              GetWindowRect(HWND(lPar),&rc);
              TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              rc.left,rc.bottom,0,Win,NULL);

              DestroyMenu(Pop);
            }else{
              if (Alert(This->DisksFol+"\n\n"+T("Are you sure you want to make this folder your new home folder?"),
                    T("Change Home Folder?"),MB_YESNO | MB_ICONQUESTION)==IDYES){
                This->HomeFol=This->DisksFol;
              }
/*
              if (GetFocus()==HWND(lPar)) SetFocus(GetDlgItem(Win,90));
              EnableWindow(HWND(lPar),0);
              EnableWindow(GetDlgItem(This->Handle,80),0);
*/
            }
            SendMessage(HWND(lPar),BM_SETCHECK,0,0);
          }
          break;
        case 82:  // Back
          if (This->Dragging>-1) break;

          if (HIWORD(wPar)==BN_CLICKED){
            if (SendMessage(HWND(lPar),BM_GETCLICKBUTTON,0,0)==2){
              SendMessage(HWND(lPar),BM_SETCHECK,1,0);
              HMENU Pop=CreatePopupMenu();

              for (int n=0;n<10;n++){
                if (This->HistBack[n].NotEmpty()){
                  EasyStr Name=GetFileNameFromPath(This->HistBack[n]);
                  if (Name.Empty()) Name=This->HistBack[n];
                  InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,5040+n,Name);
                }
              }

              RECT rc;
              GetWindowRect(HWND(lPar),&rc);
              TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              rc.left,rc.bottom,0,Win,NULL);

              DestroyMenu(Pop);
              SendMessage(HWND(lPar),BM_SETCHECK,0,0);
            }else{
              for (int n=9;n>0;n--){
                This->HistForward[n]=This->HistForward[n-1];
              }
              This->HistForward[0]=This->DisksFol;

              This->SetDir(This->HistBack[0],0);
              for (int n=0;n<9;n++){
                This->HistBack[n]=This->HistBack[n+1];
              }
              This->HistBack[9]="";

              EnableWindow(GetDlgItem(Win,83),true);
              if (This->HistBack[0].IsEmpty()){
                if (GetFocus()==HWND(lPar)) SetFocus(GetDlgItem(Win,83));
                EnableWindow(HWND(lPar),0);
              }
            }
          }
          break;
        case 83:  // Forward
          if (This->Dragging>-1) break;

          if (HIWORD(wPar)==BN_CLICKED){
            if (SendMessage(HWND(lPar),BM_GETCLICKBUTTON,0,0)==2){
              SendMessage(HWND(lPar),BM_SETCHECK,1,0);
              HMENU Pop=CreatePopupMenu();

              for (int n=0;n<10;n++){
                if (This->HistForward[n].NotEmpty()){
                  EasyStr Name=GetFileNameFromPath(This->HistForward[n]);
                  if (Name.Empty()) Name=This->HistForward[n];
                  InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,5060+n,Name);
                }
              }

              RECT rc;
              GetWindowRect(HWND(lPar),&rc);
              TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              rc.left,rc.bottom,0,Win,NULL);

              DestroyMenu(Pop);
              SendMessage(HWND(lPar),BM_SETCHECK,0,0);
            }else{
              for (int n=9;n>0;n--){
                This->HistBack[n]=This->HistBack[n-1];
              }
              This->HistBack[0]=This->DisksFol;
              This->SetDir(This->HistForward[0],0);
              for (int n=0;n<9;n++){
                This->HistForward[n]=This->HistForward[n+1];
              }
              This->HistForward[9]="";

              EnableWindow(GetDlgItem(Win,82),true);
              if (This->HistForward[0].IsEmpty()){
                if (GetFocus()==HWND(lPar)) SetFocus(GetDlgItem(Win,82));
                EnableWindow(HWND(lPar),0);
              }
            }
          }
          break;
        case 84:  // Options
          if (This->Dragging>-1) break;

          if (HIWORD(wPar)==BN_CLICKED){
            SendMessage(HWND(lPar),BM_SETCHECK,1,0);

            HMENU Pop=CreatePopupMenu();
#if !defined(SSE_GUI_DM_NO_DISABLE_B_MENU) // click on icon
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(num_connected_floppies==1 ? MF_CHECKED:0),2012,T("Disconnect Drive B"));
#endif
#if !defined(SSE_FLOPPY_ALWAYS_ADAT)
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(floppy_instant_sector_access==0 ? MF_CHECKED:0),2013,T("Accurate Disk Access Times (Slow)"));
#endif
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(FloppyArchiveIsReadWrite ? MF_CHECKED:0),2014,T("Read/Write Archives (Changes Lost On Eject)"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
#if USE_PASTI
            if (hPasti){
#if defined(SSE_DISK_PASTI_AUTO_SWITCH4)
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING 
                | int(pasti_active ? MF_CHECKED:0),2023,
                T("Use Pasti for all floppies and ACSI"));
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2024,
                T("Pasti Configuration"));
#elif defined(SSE_GUI_DM_REGROUP_PASTI)
              HMENU PastiPop=CreatePopupMenu();
              InsertMenu(PastiPop,0xffffffff,MF_BYPOSITION | MF_STRING 
                | int(pasti_active ? MF_CHECKED:0),2023,T("Enable"));
              InsertMenu(PastiPop,0xffffffff,MF_BYPOSITION | MF_STRING,
                2024,T("Configuration"));
#if defined(SSE_GUI_DM_PASTI_ONLY_STX_392) // present it the other way
              InsertMenu(PastiPop,0xffffffff,MF_BYPOSITION | MF_STRING |(int)
                (!OPTION_PASTI_JUST_STX?MF_CHECKED:0),2026,T("Run ST/MSA images"));
#elif defined(SSE_GUI_DM_PASTI_ONLY_STX)
              InsertMenu(PastiPop,0xffffffff,MF_BYPOSITION | MF_STRING |(int)
                (OPTION_PASTI_JUST_STX?MF_CHECKED:0),2026,T("Not for ST/MSA"));
#endif
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,
              (UINT)PastiPop,T("&Pasti"));
#else
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(pasti_active ? MF_CHECKED:0),2023,T("Use Pasti"));
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2024,T("Pasti Configuration"));
//              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(pasti_use_all_possible_disks ? MF_CHECKED:0),
//                                    2024,T("Use Pasti For All Compatible Images"));

#if defined(SSE_GUI_DM_PASTI_ONLY_STX) //option moved from 'SSE' page
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING |(int)
                (OPTION_PASTI_JUST_STX?MF_CHECKED:0),2026,T("Pasti only for STX"));
#endif
#endif
              InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
            }
#endif

#if defined(SSE_DISK_GHOST) && defined(SSE_GUI_DM_GHOST)
#if defined(SSE_VAR_392) // more generic wording
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING |(int)
              (OPTION_GHOST_DISK?MF_CHECKED:0),2027,T("Enable ghost disks for protected disks"));
#elif USE_PASTI 
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING |(int)
              (OPTION_GHOST_DISK?MF_CHECKED:0),2027,T("Enable ghost disks for CTR-IPF-SCP-STX"));
#else //64bit build
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING |(int)
              (OPTION_GHOST_DISK?MF_CHECKED:0),2027,T("Enable ghost disks for CTR-IPF-SCP"));
#endif
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
#endif
#if defined(SSE_TOS_PRG_AUTORUN) && !defined(SSE_TOS_PRG_AUTORUN_NOT_OPTIONAL)
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING |(int)
              (OPTION_PRG_SUPPORT?MF_CHECKED:0),2028,T("Run PRG and TOS files"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
#endif
#if defined(SSE_ACSI_OPTION) && !defined(SSE_ACSI_ICON)
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING 
              | (int)(SSEOption.Acsi?MF_CHECKED:0)
              | (int)(SSEConfig.AcsiImg?0:MF_DISABLED)
              ,2029,T("ACSI hard disk images")); // with a s
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
#endif
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(This->AutoInsert2 ? MF_CHECKED:0),2016,T("Automatically Insert &Second Disk"));
#if !defined(SSE_GUI_DM_REGROUP)
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(This->HideBroken ? MF_CHECKED:0),2002,T("Hide &Broken Shortcuts"));
#endif
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(This->EjectDisksWhenQuit ? MF_CHECKED:0),2011,T("E&ject Disks When Quit"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
#if defined(SSE_GUI_DM_REGROUP)
            HMENU DCActionPop=CreatePopupMenu();
            InsertMenu(DCActionPop,0xffffffff,MF_BYPOSITION | MF_STRING,
              2007,T("&None"));
            InsertMenu(DCActionPop,0xffffffff,MF_BYPOSITION | MF_STRING,
              2008,T("Insert In &Drive A"));
            InsertMenu(DCActionPop,0xffffffff,MF_BYPOSITION | MF_STRING,
              2009,T("Insert, &Reset and Run"));
            CheckMenuRadioItem(DCActionPop,2007,2009,
              2007+This->DoubleClickAction,MF_BYCOMMAND);
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,
              (UINT)DCActionPop,T("Double Click Disk &Action"));
#else
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2007,T("Double Click On Disk Does &Nothing"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2008,T("Double Click On Disk Inserts In &Drive A"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2009,T("Double Click On Disk Inserts, &Resets and Runs"));
            CheckMenuRadioItem(Pop,2007,2009,2007+This->DoubleClickAction,MF_BYCOMMAND);
#endif
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(This->CloseAfterIRR ? MF_CHECKED:0),
                          2015,T("&Close Disk Manager After Insert, Reset and Run"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,1999,NULL);
#if defined(SSE_GUI_DM_REGROUP) // it's a "view" option
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(This->HideBroken ? MF_CHECKED:0),2002,T("Hide &Broken Shortcuts"));
#endif
#if defined(SSE_GUI_DM_SHOW_EXT)
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING |
              int(This->HideExtension ? MF_CHECKED:0),2017,T("Hide E&xtension"));
#endif
#if defined(SSE_GUI_DM_REGROUP)
            HMENU IconSizePop=CreatePopupMenu();
            InsertMenu(IconSizePop,0xffffffff,MF_BYPOSITION | MF_STRING,
              2005,T("&Large"));
            InsertMenu(IconSizePop,0xffffffff,MF_BYPOSITION | MF_STRING,
              2006,T("&Small"));
            CheckMenuRadioItem(IconSizePop,2005,2006,
              2005+This->SmallIcons,MF_BYCOMMAND);
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,
              (UINT)IconSizePop,T("&Icon Size"));
#else
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2005,T("&Large Icons"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2006,T("&Small Icons"));
            CheckMenuRadioItem(Pop,2005,2006,2005+This->SmallIcons,MF_BYCOMMAND);
#endif
            HMENU SpacingPop=CreatePopupMenu();
            InsertMenu(SpacingPop,0xffffffff,MF_BYPOSITION | MF_STRING,2020,T("Thin"));
            InsertMenu(SpacingPop,0xffffffff,MF_BYPOSITION | MF_STRING,2021,T("Normal"));
            InsertMenu(SpacingPop,0xffffffff,MF_BYPOSITION | MF_STRING,2022,T("Wide"));
            CheckMenuRadioItem(SpacingPop,2020,2022,2020+This->IconSpacing,MF_BYCOMMAND);

            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,(UINT)SpacingPop,T("&Icon Spacing"));

            RECT rc;
            GetWindowRect(HWND(lPar),&rc);
            TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                            rc.left,rc.bottom,0,Win,NULL);

            DestroyMenu(Pop);

            SendMessage(HWND(lPar),BM_SETCHECK,0,0);
          }
          break;
        case 85:  // Options
          if (This->Dragging>-1) break;

          if (HIWORD(wPar)==BN_CLICKED){
            SendMessage(HWND(lPar),BM_SETCHECK,1,0);

            HMENU Pop=CreatePopupMenu();

            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2025,T("Search Disk Image Database")+"\10F9");
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,0,NULL);
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2003,T("Open Current Folder In &Explorer")+"\10F4");
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(This->ExplorerFolders ? MF_CHECKED:0),2004,T("&Folders Pane in Explorer"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2010,T("Find In Current Folder"));
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2030,T("Run MSA Converter")+"\10F6");
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,0,NULL);
#ifndef SSE_VAR_NO_WINSTON
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,2001,T("Import &WinSTon Favourites"));
#endif
            RECT rc;
            GetWindowRect(HWND(lPar),&rc);
            TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                            rc.left,rc.bottom,0,Win,NULL);

            DestroyMenu(Pop);

            SendMessage(HWND(lPar),BM_SETCHECK,0,0);
          }
          break;
        case IDOK:  //Return
          if (This->Dragging>-1) break;

          if (GetFocus()==This->DiskView){
            PostMessage(Win,WM_USER,1234,0);
          }
          break;
        case 90:  //Drive Combo
          if (This->Dragging>-1) break;

          if (HIWORD(wPar)==CBN_SELENDOK){
            char Text[8];
#if defined(SSE_X64_390)
            SendMessage((HWND)lPar,CB_GETLBTEXT,SendMessage((HWND)lPar,CB_GETCURSEL,0,0),(LPARAM)Text);
#else
            SendMessage((HWND)lPar,CB_GETLBTEXT,SendMessage((HWND)lPar,CB_GETCURSEL,0,0),(long)Text);
#endif
            if (Text[0]!=This->DisksFol[0]){
              This->SetDir(Text,true);
              if (Text[0]!=This->DisksFol[0]){
#if defined(SSE_X64_390)
                int idx=SendMessage((HWND)lPar,CB_FINDSTRING,0xffffffff,(LPARAM)((This->DisksFol.Lefts(2)+"\\").Text));
#else
                int idx=SendMessage((HWND)lPar,CB_FINDSTRING,0xffffffff,(long)((This->DisksFol.Lefts(2)+"\\").Text));
#endif
                if (idx>-1) SendMessage((HWND)lPar,CB_SETCURSEL,idx,0);
              }
            }
          }
          break;
        case 1000:
        {
          EasyStr FolName=This->DisksFol+"\\"+T("New Folder");
          int n=2;
          while (GetFileAttributes(FolName)<0xFFFFFFFF){
            FolName=This->DisksFol+"\\"+T("New Folder")+" ("+(n++)+")";
          }
          CreateDirectory(FolName,NULL);
          This->RefreshDiskView(FolName,true);
          break;
        }
        case 1001:
        {
          EasyStr STName=This->DisksFol+"\\"+T("Blank Disk")+".st";
          int n=2;
          while (Exists(STName)){
            STName=This->DisksFol+"\\"+T("Blank Disk")+" ("+(n++)+").st";
          }
          if (This->CreateDiskImage(STName,1440,9,2)){
            This->RefreshDiskView(STName,true);
          }else{
            Alert(EasyStr(T("Could not create the disk image "))+STName,T("Error"),MB_ICONEXCLAMATION);
          }


          return 0;
        }
        case 1002:  // Custom disk image
          This->ShowDiskDiag();
          break;
#if defined(SSE_GUI_DM_STW) && defined(SSE_GUI_DM_HFE) //been merged
        case 1003: // New STW Disk Image
        case 1004: // New HFE Disk Image
        {
          char *extension=(wPar==1003)?DISK_EXT_STW:DISK_EXT_HFE;
          EasyStr STName=This->DisksFol+"\\"+extension+" Disk."+extension;
          int n=2;
          while (Exists(STName)){
            STName=This->DisksFol+"\\"+extension+" Disk ("+(n++)+")."+extension;
          }
          if((wPar==1003)&&ImageSTW[0].Create(STName) 
            || (wPar==1004)&&ImageHFE[0].Create(STName)) // ugly C++
            This->RefreshDiskView(STName,true);
          else
            Alert(EasyStr(T("Could not create the disk image "))+STName,T("Error"),MB_ICONEXCLAMATION);
          return 0;
        }
#endif
        case 1005:
          PostMessage(Win,WM_COMMAND,IDCANCEL,0);
          break;
        case 1010:case 1011:case 1012:  //Insert in A/B/RR
        {
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          if (Inf){
            This->PerformInsertAction(LOWORD(wPar)-1010,Inf->Name,Inf->Path,"");
          }
          break;
        }
        case 1020:
          SendMessage(This->DiskView,LVM_EDITLABEL,This->MenuTarget,0);
          break;
        case 1030:
          PostMessage(Win,WM_USER,1234,2);
          break;

#if defined(SSE_DRIVE_SOUND_SEEK_OPTION)
        case 1044:
        case 1045:
          OPTION_DRIVE_SOUND_SEEK_SAMPLE=!OPTION_DRIVE_SOUND_SEEK_SAMPLE;
          break;
#endif

#if defined(SSE_DRIVE_SWITCH_OFF_MOTOR)
        case 1046:
        case 1047:
          SF314[(LOWORD(wPar)-1046)].State.motor=false;
          fdc_str&=~FDC_STR_MOTOR_ON;
          break;
#endif

#if defined(SSE_DRIVE_SINGLE_SIDE)
/*   bit0 A bit 1 B set = single - we toggle bits
Toggling a bit

The XOR operator (^) can be used to toggle a bit.
number ^= 1 << x;

That will toggle bit x.
*/
        case 1048:  
        case 1049:
          SSEOption.SingleSideDriveMap^=1<< (LOWORD(wPar)-1048);
          break;
#endif

          //SS TODO: write-protect that doesn't change file properties

        case 1050:  // Disk in Drive 1
        case 1051:  // Disk in Drive 2
          This->DragLV=GetDlgItem(Win,int(LOWORD(wPar)==1050 ? 100:101));
        case 1040:  // Toggle Read-Only
        {
          bool FromDV=0;
          if (LOWORD(wPar)==1040){
            This->DragLV=This->DiskView;
            FromDV=true;
          }
          LV_ITEM lvi;
          lvi.iItem=int(FromDV ? This->MenuTarget:0);
          lvi.iSubItem=0;
          lvi.mask=LVIF_PARAM;
          SendMessage(This->DragLV,LVM_GETITEM,0,(LPARAM)&lvi);
          DiskManFileInfo *Inf=(DiskManFileInfo*)lvi.lParam;
          EasyStr DiskPath=Inf->Path;

          bool InDrive[2]={0,0};
          EasyStr OldName[2],DiskInZip[2];
          for (int d=0;d<2;d++){
            if (IsSameStr_I(FloppyDrive[d].GetDisk(),DiskPath)){
              InDrive[d]=true;
              OldName[d]=FloppyDrive[d].DiskName;
              DiskInZip[d]=FloppyDrive[d].DiskInZip;
              FloppyDrive[d].RemoveDisk();
            }
          }
          DWORD Attrib=GetFileAttributes(DiskPath);
          if (Inf->ReadOnly){
            SetFileAttributes(DiskPath,Attrib & ~FILE_ATTRIBUTE_READONLY);
          }else{
            SetFileAttributes(DiskPath,Attrib | FILE_ATTRIBUTE_READONLY);
          }
          Inf->ReadOnly=bool(GetFileAttributes(DiskPath) & FILE_ATTRIBUTE_READONLY); // Just in case of failure

          int c=SendMessage(This->DiskView,LVM_GETITEMCOUNT,0,0);
          if (Inf->LinkPath.NotEmpty() || FromDV==0){
            for (lvi.iItem=1;lvi.iItem<c;lvi.iItem++){
              lvi.mask=LVIF_PARAM;
              SendMessage(This->DiskView,LVM_GETITEM,0,(LPARAM)&lvi);
              if (((DiskManFileInfo*)lvi.lParam)->LinkPath.IsEmpty()){
                if (IsSameStr_I(((DiskManFileInfo*)lvi.lParam)->Path,DiskPath)){
                  ((DiskManFileInfo*)lvi.lParam)->ReadOnly=Inf->ReadOnly;
                  break;
                }
              }
            }
          }
          if (lvi.iItem<c){
            lvi.mask=LVIF_IMAGE;
            lvi.iImage=1 + Inf->ReadOnly*4;
            SendMessage(This->DiskView,LVM_SETITEM,0,(LPARAM)&lvi);
          }

          if (InDrive[0]) This->InsertDisk(0,OldName[0],DiskPath,0,0,DiskInZip[0]);
          if (InDrive[1]) This->InsertDisk(1,OldName[1],DiskPath,0,0,DiskInZip[1]);

          SetFocus(This->DragLV);

          break;
        }
#if defined(SSE_GUI_STW_CONVERT) 
/*  Create a new STW disk image, and copy the data from a DIM, MSA or ST
    image into it, using some facilities of our WD1772 emu.
*/
        case 1041:
        {
          TWD1772MFM wd1772mfm;
          TWD1772Crc wd1772crc;
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          if(Inf)
          {
            EasyStr dest=Inf->Path;
            char *dot=strrchr(dest,'.');
            if (dot) 
              strcpy(dot+1,"STW");
            if(dest.Text)
            {
              int Err=FloppyDrive[0].SetDisk(Inf->Path,"");
              if (!Err && SF314[0].ImageType.Manager==MNGR_STEEM //DIM, MSA or ST
                && FloppyDrive[0].SectorsPerTrack<=11) //not "super" disks
              {   
                TRACE_LOG("Creating %s\n",dest.Text);
                ImageSTW[1].Create(dest.Text);
                ImageSTW[1].Open(dest.Text);
                FloppyDrive[1].ReadOnly=false; // make sure we save the STW

                for(int track=0;track<FloppyDrive[0].TracksPerSide;track++)
                {
                  for(int side=0;side<FloppyDrive[0].Sides;side++)
                  {
                    if(!ImageSTW[1].LoadTrack(side,track))
                      continue;
                    int p=0;
                    for(int i=0;i<Disk[0].PostIndexGap();i++) 
                      WD1772_WRITE(0x4E)
                    int sector; //used in trace
                    for(int sector2=1;sector2<=FloppyDrive[0].SectorsPerTrack;sector2++)
                    {
                      //  We must use interleave 6 for 11 sectors, eg Pang -EMP
                      sector= ( FloppyDrive[0].SectorsPerTrack==11 
                        ? ((((sector2-1)*DRIVE_11SEC_INTERLEAVE)%11)+1) 
                        : sector2 );
                      if(FloppyDrive[0].SeekSector(side,track,sector,false))
                        break; // not in source, write nothing more

                      for(int i=0;i<(FloppyDrive[0].SectorsPerTrack==11?3:12);i++)
                        WD1772_WRITE(0)
                      for(int i=0;i<3;i++)
                        WD1772_WRITE_A1

                      // write sector ID
                      WD1772_WRITE(0xFE)
                      WD1772_WRITE(track)
                      WD1772_WRITE(side)
                      WD1772_WRITE(sector)
                      WD1772_WRITE(2) //512 bytes
                      WD1772_WRITE_CRC(wd1772crc.crc>>8)
                      WD1772_WRITE_CRC(wd1772crc.crc&0xFF)

                      for(int i=0;i<22;i++)
                        WD1772_WRITE(0x4E)
                      for(int i=0;i<12;i++) // 11 sectors too?
                        WD1772_WRITE(0)
                      for(int i=0;i<3;i++)
                        WD1772_WRITE_A1

                      // write sector data
                      WD1772_WRITE(0xFB)
                      BYTE b=0;
                      for(int i=0;i<512;i++) 
                      {
                        fread(&b,1,1,FloppyDrive[0].f);
                        WD1772_WRITE(b)                        
                      }
                      WD1772_WRITE_CRC(wd1772crc.crc>>8)
                      WD1772_WRITE_CRC(wd1772crc.crc&0xFF)
                      WD1772_WRITE(0xFF) // so 1 byte gap fewer

                      for(int i=0;i<(FloppyDrive[0].SectorsPerTrack==11?1-1:40-1);i++)
                        WD1772_WRITE(0x4E)

                    }//sector

                    // pre-index gap: the rest
                    ASSERT(Disk[1].TrackBytes>=6256);
                    while(p<Disk[1].TrackBytes) 
                      WD1772_WRITE(0x4E)

                    TRACE_LOG("STW track %d/%d written %d sectors %d bytes\n",side,track,sector-1,p);
        
                  }//side
                }//track
                ImageSTW[1].Close();
                This->RefreshDiskView(dest,true);
              }
            }
          }
          break;
        }
#endif
        case 1060:
        case 1061:
        {
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          if (Inf) ShellExecute(NULL,LPSTR((LOWORD(wPar)==1061) ? "Find":LPSTR(This->ExplorerFolders ? "explore":NULL)),Inf->Path,"","",SW_SHOWNORMAL);
          break;
        }
        case 1070:
        {
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          WIN32_FIND_DATA wfd;
          EasyStr NewPath=GetLinkDest(Inf->LinkPath,&wfd,HWND(FullScreen ? StemWin:This->Handle));
          if (NewPath.NotEmpty()){
            if (GetFileAttributes(NewPath)!=0xffffffff){
              Inf->Path=NewPath;
              Inf->BrokenLink=0;

              LV_ITEM lvi;
              lvi.iItem=This->MenuTarget;
              lvi.iSubItem=0;
              lvi.mask=LVIF_IMAGE;
              lvi.iImage=int(Inf->Folder ? 3:4);
              SendMessage(This->DiskView,LVM_SETITEM,0,(LPARAM)&lvi);
            }
          }
          break;
        }
        case 1080:  //Extract
        {
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          This->ExtractDisks(Inf->Path);
          break;
        }
#if defined(SSE_GUI_DM_NAME_CLIPBOARD)
/*  Put the name of the game in the clipboard when user clicks on it.
*/
        case 1082:
        {
          GET_THIS;
          DiskManFileInfo *Inf=This->GetItemInf( 0, (HWND) GetDlgItem(Win,100+This->MenuTarget) );
          SetClipboardText(Inf->Name.Text); // in acc.cpp
          break;
        }
#endif
        case 1090:  // Go to disk
        case 1091:  // Go to disk in drive
        {
          GET_THIS;
          DiskManFileInfo *Inf=This->GetItemInf(int((LOWORD(wPar)==1090) ? This->MenuTarget:0),
                                HWND((LOWORD(wPar)==1090) ? This->DiskView:GetDlgItem(Win,100+This->MenuTarget)));
          This->GoToDisk(Inf->Path,0);
          break;
        }
        case 1092: // Open Disk's Folder in Explorer
        {
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          if (Inf){
            Str Fol=Inf->Path;
            RemoveFileNameFromPath(Fol,REMOVE_SLASH);
            ShellExecute(NULL,LPSTR(This->ExplorerFolders ? "explore":NULL),Fol,"","",SW_SHOWNORMAL);
          }
          break;
        }
        //SS can make case 1093 for copy-to-clipboard
        case 1099:
        {
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          if (Inf){
            This->PropInf=*Inf;
            This->ShowPropDiag();
          }
          break;
        }
        case 1100: // Swap disks
        {
          This->SwapDisks(This->MenuTarget);
          break;
        }
        case 1101:
          if (SendMessage(GetDlgItem(Win,100 + This->MenuTarget),LVM_GETITEMCOUNT,0,0)){
            This->EjectDisk(This->MenuTarget);
          }
          break;
        case 2002:
          This->HideBroken=!This->HideBroken;
          PostMessage(Win,WM_COMMAND,IDCANCEL,0);
          break;
#if defined(SSE_GUI_DM_SHOW_EXT)
        case 2017:
          This->HideExtension=!This->HideExtension;
          PostMessage(Win,WM_COMMAND,IDCANCEL,0);
          break;
#endif
        case 2003:
          ShellExecute(NULL,LPSTR(This->ExplorerFolders ? "explore":NULL),This->DisksFol,"","",SW_SHOWNORMAL);
          break;
        case 2004:
          This->ExplorerFolders=!This->ExplorerFolders;
          break;
        case 2005:
          if (This->SmallIcons){
            This->SmallIcons=0;
            SendMessage(This->DiskView,WM_SETREDRAW,0,0);
            This->SetDiskViewMode(LVS_ICON);
            This->RefreshDiskView();
          }
          break;
        case 2006:
          if (This->SmallIcons==0){
            This->SmallIcons=true;
            This->RefreshDiskView();
          }
          break;
        case 2007:case 2008:case 2009:
          This->DoubleClickAction=LOWORD(wPar)-2007;
          break;
#ifndef SSE_VAR_NO_WINSTON
        case 2001:  // Import
          This->ShowImportDiag();
          break;
#endif
        case 2010:
          ShellExecute(NULL,"Find",This->DisksFol,"","",SW_SHOWNORMAL);
          break;
        case 2011:
          This->EjectDisksWhenQuit=!This->EjectDisksWhenQuit;
          break;
#if !defined(SSE_GUI_DM_NO_DISABLE_B_MENU)
        case 2012:
          SendDlgItemMessage(Win,99,WM_LBUTTONDOWN,0,0);
          break;
#endif
#if !defined(SSE_FLOPPY_ALWAYS_ADAT)
        case 2013: // SS menu ADAT
          floppy_instant_sector_access=!floppy_instant_sector_access;
          InvalidateRect(GetDlgItem(Win,98),NULL,0);
          InvalidateRect(GetDlgItem(Win,99),NULL,0);
          CheckResetDisplay();
#if defined(SSE_DRIVE)
          SF314[0].UpdateAdat();
          SF314[1].UpdateAdat();
#endif
          break;
#endif
        case 2014:
          FloppyArchiveIsReadWrite=!FloppyArchiveIsReadWrite;
          This->LoadIcons();
          if (FloppyDrive[0].IsZip()) FloppyDrive[0].ReinsertDisk();
          if (FloppyDrive[1].IsZip()) FloppyDrive[1].ReinsertDisk();
          break;
        case 2015:
          This->CloseAfterIRR=!This->CloseAfterIRR;
          break;
        case 2016:
          This->AutoInsert2=!This->AutoInsert2;
          break;
        case 2020:case 2021:case 2022:
          if (This->IconSpacing!=(LOWORD(wPar)-2020)){
            This->IconSpacing=LOWORD(wPar)-2020;
            This->SetDiskViewMode(This->SmallIcons ? LVS_SMALLVIEW:LVS_ICON);
            This->RefreshDiskView();
          }
          break;
#if USE_PASTI
        case 2023:case 2024:
          if (hPasti==NULL) break;
          if (LOWORD(wPar)==2024){
            pasti->DlgConfig(HWND(FullScreen ? StemWin:This->Handle),0,NULL);
          }
          if (LOWORD(wPar)==2023){ //SS option use pasti

#if defined(SSE_DISK_PASTI_NO_RESET)
/*  More complicated than it looked, not foolproof.
    Maybe there's a better way but we created TFloppyDrive::GetImageFile()
    for the occasion.
    No error message if you go native with a STX disk in, or Pasti with a DIM 
    disk in, the disk will just disappear.
*/
            pasti_active=!pasti_active;
            //TRACE_LOG("pasti_active %d\n",pasti_active);
            for(int i=0;i<2;i++)
            {
              if(FloppyDrive[i].NotEmpty())
              {
                EasyStr name=FloppyDrive[i].DiskName;
                EasyStr path=FloppyDrive[i].GetImageFile(); 
                This->EjectDisk(i);
                This->InsertDisk(i,name,path,0,0,"",true);
              }
            }
            This->RefreshDiskView();
            
#else
            bool cancel=false;
            if (pc!=rom_addr){
              int i=Alert(T("Changing this setting necessitates a cold reset.")+"\n"+T("Do you want to reset now?"),
                              T("Reset?"),MB_ICONQUESTION | MB_YESNO);
              if (i==IDYES){
                reset_st(0); //cold reset
              }else{
                cancel=true;
              }
            }
            if(!cancel){
              pasti_active=!pasti_active;
              This->EjectDisk(0);
              This->EjectDisk(1);
              This->RefreshDiskView();
              CheckResetDisplay();
            }
#endif
          }//if (LOWORD(wPar)==2023
          break;

#if defined(SSE_GUI_DM_PASTI_ONLY_STX)
        case 2026:
          OPTION_PASTI_JUST_STX=!OPTION_PASTI_JUST_STX;
          TRACE_LOG("Option Pasti just STX %d\n",OPTION_PASTI_JUST_STX);
          for(int i=0;i<2;i++) // necessary, later refactor (structure) TODO
          {
            if(FloppyDrive[i].NotEmpty())
            {
              EasyStr name=FloppyDrive[i].DiskName;
              EasyStr path=FloppyDrive[i].GetImageFile(); 
              This->EjectDisk(i);
              This->InsertDisk(i,name,path,0,0,"",true);
              This->RefreshDiskView();
            }
          }            

          break;

#endif
#endif//pasti
        case 2025:
          This->ShowDatabaseDiag();
          break;

#if defined(SSE_DISK_GHOST) && defined(SSE_GUI_DM_GHOST)
        case 2027:
          OPTION_GHOST_DISK=!OPTION_GHOST_DISK;
          TRACE_LOG("Option Ghost disk %d\n",OPTION_GHOST_DISK);
          break;
#endif
#if defined(SSE_TOS_PRG_AUTORUN) && !defined(SSE_TOS_PRG_AUTORUN_NOT_OPTIONAL)
        case 2028:
          OPTION_PRG_SUPPORT=!OPTION_PRG_SUPPORT;
          TRACE_LOG("Option PRG support %d\n",OPTION_GHOST_DISK);
          break;
#endif
#if defined(SSE_ACSI_OPTION) && !defined(SSE_ACSI_ICON)
        case 2029:
          SSEOption.Acsi=!SSEOption.Acsi;
          TRACE_LOG("Option ACSI support %d\n",SSEOption.Acsi);
#if defined(SSE_ACSI_RELOAD_TOS)
          if(SSEOption.Acsi)
            load_TOS(ROMFile);
#endif 
          break;
#endif
      }
      if (LOWORD(wPar)>=4000 && LOWORD(wPar)<5000){
        This->MenuTarget=LOWORD(wPar);
      }else if (LOWORD(wPar)>=5000 && LOWORD(wPar)<5080){
        if (LOWORD(wPar)>=5060){
          int nGoForward=(LOWORD(wPar)-5060)+1;
          for (int i=0;i<nGoForward;i++){
            for (int n=9;n>0;n--){
              This->HistBack[n]=This->HistBack[n-1];
            }
          }
          int BackIdx=0,ForIdx=nGoForward-2;
          for (int n=0;n<nGoForward-1;n++) This->HistBack[BackIdx++]=This->HistForward[ForIdx--];
          This->HistBack[BackIdx]=This->DisksFol;
          This->SetDir(This->HistForward[nGoForward-1],0);
          for (int i=0;i<nGoForward;i++){
            for (int n=0;n<9;n++){
              This->HistForward[n]=This->HistForward[n+1];
            }
            This->HistForward[9]="";
          }

          EnableWindow(GetDlgItem(Win,82),true);
          if (This->HistForward[0].IsEmpty()){
            if (GetFocus()==GetDlgItem(Win,83)) SetFocus(GetDlgItem(Win,82));
            EnableWindow(GetDlgItem(Win,83),0);
          }
        }else if (LOWORD(wPar)>=5040){
          int nGoBack=(LOWORD(wPar)-5040)+1;
          for (int i=0;i<nGoBack;i++){
            for (int n=9;n>0;n--){
              This->HistForward[n]=This->HistForward[n-1];
            }
          }
          int ForIdx=0,BackIdx=nGoBack-2;
          for (int n=0;n<nGoBack-1;n++) This->HistForward[ForIdx++]=This->HistBack[BackIdx--];
          This->HistForward[ForIdx]=This->DisksFol;

          This->SetDir(This->HistBack[nGoBack-1],0);
          for (int i=0;i<nGoBack;i++){
            for (int n=0;n<9;n++){
              This->HistBack[n]=This->HistBack[n+1];
            }
            This->HistBack[9]="";
          }

          EnableWindow(GetDlgItem(Win,83),true);
          if (This->HistBack[0].IsEmpty()){
            if (GetFocus()==GetDlgItem(Win,82)) SetFocus(GetDlgItem(Win,83));
            EnableWindow(GetDlgItem(Win,82),0);
          }
        }else{ // Go to quick folder
          if (LOWORD(wPar)==5000){
            This->SetDir(This->HomeFol,true);
          }else{
            if (This->QuickFol[LOWORD(wPar)-5001].NotEmpty()){
              This->SetDir(This->QuickFol[LOWORD(wPar)-5001],true);
            }
          }
        }
      }
      if (LOWORD(wPar)>=8000 && LOWORD(wPar)<8100){  // Change/erase quick folder
        if (LOWORD(wPar)==8000){
          if (Alert(This->DisksFol+"\n\n"+T("Are you sure you want to make this folder your new home folder?"),
                T("Change Home Folder?"),MB_YESNO | MB_ICONQUESTION)==IDYES){
            This->HomeFol=This->DisksFol;
          }
        }else{
          int n=(LOWORD(wPar)-8005)/5;
          int Action=(LOWORD(wPar)-8005) % 5;
          switch (Action){
            case 0:

              This->QuickFol[n]=This->DisksFol;
              break;
            case 1:
            {
              EnableAllWindows(0,Win);
              // SS: this allows player to select a network folder as well
              EasyStr NewFol=ChooseFolder(HWND(FullScreen ? StemWin:Win),T("Pick a Folder"),This->DisksFol);
              if (NewFol.NotEmpty()){
                This->QuickFol[n]=NewFol;
              }

              SetForegroundWindow(Win);
              EnableAllWindows(true,Win);
              break;
            }
            case 2:
              This->QuickFol[n]="";
              break;
          }
        }
      }else if (LOWORD(wPar)>=6000 && LOWORD(wPar)<7000){
        int SelItem=This->GetSelectedItem();
        if (SelItem>-1){
          DiskManFileInfo *Inf=This->GetItemInf(SelItem);
          int Action=(LOWORD(wPar)-6000)/20;

          EasyStr SrcFol,DestFol,To;
          char From[MAX_PATH+2];
          ZeroMemory(From,sizeof(From));
          if ((LOWORD(wPar) % 20)==0){
            DestFol=This->HomeFol;
          }else{
            DestFol=This->QuickFol[(LOWORD(wPar) % 20)-1];
          }
          To=DestFol+"\\"+GetFileNameFromPath((Action<2) ? Inf->Path:Inf->LinkPath);
          bool Moving=0;
          switch (Action){
            case 0: // Move Path
            case 3: // Move LinkPath
              Moving=true;
            case 1: // Copy Path
            case 4: // Copy LinkPath
              strcpy(From,(Action<2) ? Inf->Path:Inf->LinkPath);
              SrcFol=From;
              RemoveFileNameFromPath(SrcFol,REMOVE_SLASH);
              if (NotSameStr_I(SrcFol,DestFol) || Moving==0){
                This->MoveOrCopyFile(Moving,From,To,(Action==0) ? Inf->Path:Str(),IsSameStr_I(SrcFol,DestFol));
                bool Refresh=0;
                if (Action==0 && Inf->LinkPath.NotEmpty()){
                  CreateLink(Inf->LinkPath,To);
                  Inf->Path=To;
                }
                if (IsSameStr_I(SrcFol,This->DisksFol) && Moving) Refresh=true;
                if (IsSameStr_I(DestFol,This->DisksFol)) Refresh=true;
                if (Refresh) This->RefreshDiskView("",0,"",SelItem);
              }
              break;
            case 2:  // Link to Path
            {
              EasyStr LinkName=DestFol+"\\"+Inf->Name+".lnk";
              int n=2;
              while (Exists(LinkName)){
                LinkName=DestFol+"\\"+Inf->Name+" ("+(n++)+").lnk";
              }

              CreateLink(LinkName,Inf->Path);

              if (IsSameStr_I(DestFol,This->DisksFol)) This->RefreshDiskView("",true,LinkName);
              break;
            }
          }
        }
      }else if ((LOWORD(wPar)>=7000 && LOWORD(wPar)<7020) || LOWORD(wPar)==1015){
        // Get contents [and create shortcuts in DestFol]
        Str DestFol;
        if (LOWORD(wPar)!=1015){
          if ((LOWORD(wPar) % 20)==0){
            DestFol=This->HomeFol;
          }else{
            DestFol=This->QuickFol[(LOWORD(wPar) % 20)-1];
          }
        }

        DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);

        This->GetContentsSL(Inf->Path);
        if (This->contents_sl.NumStrings){
          if (DestFol.NotEmpty()){
            // 0 = path, 1 = disk name, 2+ = contents
            Str ShortName=This->GetContentsGetAppendName(This->contents_sl[1].String);
            Str SelLink;
            int start_i=2;
            if (This->contents_sl.NumStrings==1) start_i=1;
            for (int i=start_i;i<This->contents_sl.NumStrings;i++){
              SelLink=GetUniquePath(DestFol,Str(This->contents_sl[i].String)+" ("+ShortName+").lnk");
              CreateLink(SelLink,Inf->Path);
            }
            if (SelLink.NotEmpty() && IsSameStr_I(DestFol,This->DisksFol)){
              This->RefreshDiskView("",0,SelLink);
            }
          }else{
            This->ContentsLinksPath=This->DisksFol;
            This->ShowContentDiag();
          }
        }
      }else if (LOWORD(wPar)>=9000 && LOWORD(wPar)<10000){
        DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
        if (Inf){
          int Action=(LOWORD(wPar)-9000)/200;
          EasyStr DiskInZip=This->MenuESL[(LOWORD(wPar)-9000) % 200].String;
          This->PerformInsertAction(Action,Inf->Name,Inf->Path,DiskInZip);
        }
      }else if (LOWORD(wPar)>=2030 && LOWORD(wPar)<2100){ // MSA Converter
        Str MSA=This->GetMSAConverterPath();
        if (This->MSAConvPath.NotEmpty()){
          Str Comline;
          DiskManFileInfo *Inf=This->GetItemInf(This->MenuTarget);
          Str NewSel;
          switch (LOWORD(wPar)){
            case 2031:  Comline=Str("\"")+Inf->Path+"\"";  break;
            case 2034: // Zip containing files to disk image
            {
              NewSel=Inf->Path;
              *strchr(NewSel,'.')=0;
              NewSel+=".st";
              Comline=Str("\"convert\" \"")+Inf->Path+"\" \"st\" \"exit\"";
              break;
            }
            default:
              if (LOWORD(wPar)>2040 && LOWORD(wPar)<=2040+26){
                NewSel=GetUniquePath(mount_path[LOWORD(wPar)-2040],Inf->Name);
                CreateDirectory(NewSel,NULL);
                Comline=Str("\"diskimg_to_hdisk\" \"")+Inf->Path+"\" \""+NewSel+"\" \"exit\"";
                NewSel=""; // Nothing to select
              }else{
                Comline=Str("\"user_path\" \"")+This->DisksFol+"\"";
              }
          }
          SHELLEXECUTEINFO sei;
          sei.cbSize=sizeof(SHELLEXECUTEINFO);
          sei.fMask=SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
          sei.hwnd=NULL;
          sei.lpVerb=NULL;
          sei.lpFile=MSA;
          sei.lpParameters=Comline;
          sei.lpDirectory=NULL;
          sei.nShow=SW_SHOWNORMAL;
          if (ShellExecuteEx(&sei)){
            if (NewSel.NotEmpty()){
              This->MSAConvProcess=sei.hProcess;
              This->MSAConvSel=NewSel;
              SetTimer(Win,MSACONV_TIMER_ID,1000,NULL);
            }
          }
        }
      }
      break;
    }
    case WM_CONTEXTMENU:
      GET_THIS;
      if (HWND(wPar)==This->DiskView){
        HMENU Pop=CreatePopupMenu();

        int c=SendMessage(This->DiskView,LVM_GETITEMCOUNT,0,0);

        LV_ITEM lvi;
        lvi.mask=LVIF_PARAM | LVIF_STATE;
        lvi.iSubItem=0;
        lvi.stateMask=LVIS_SELECTED;
        for (lvi.iItem=0;lvi.iItem<c;lvi.iItem++){
          SendMessage(This->DiskView,LVM_GETITEM,0,(LPARAM)&lvi);
          if (lvi.state==LVIS_SELECTED){
            This->AddFileOrFolderContextMenu(Pop,(DiskManFileInfo*)lvi.lParam);
            This->MenuTarget=lvi.iItem;
            break;
          }
        }
        HMENU FolOptionsPop=Pop;
        if (GetScreenHeight()<600){
          FolOptionsPop=CreatePopupMenu();
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_POPUP | MF_STRING,(UINT)FolOptionsPop,T("More Options"));
        }
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,1005,Str(T("Refresh"))+" \10ESC");
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,2003,T("Open Current Folder In &Explorer")+"\10F4");
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,2010,T("Find In Current Folder"));
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,2030,T("Run MSA Converter")+"\10F6");
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
#if defined(SSE_GUI_DM_REGROUP2)
            HMENU NewPop=CreatePopupMenu();

            InsertMenu(NewPop,0xffffffff,MF_BYPOSITION | MF_STRING,1000,
              T("&Folder"));
            InsertMenu(NewPop,0xffffffff,MF_BYPOSITION | MF_STRING,1001,
              T("Standard &Disk Image"));
            InsertMenu(NewPop,0xffffffff,MF_BYPOSITION | MF_STRING,1002,
              T("Custom Disk &Image"));
#if defined(SSE_GUI_DM_STW) //new context option
            InsertMenu(NewPop,0xffffffff,MF_BYPOSITION | MF_STRING,1003,
              T("ST&W Disk Image"));
#endif
#if defined(SSE_GUI_DM_HFE) //new context option
            InsertMenu(NewPop,0xffffffff,MF_BYPOSITION | MF_STRING,1004,
              T("&HFE Disk Image"));
#endif
            InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING | MF_POPUP,
              (UINT)NewPop,T("&New..."));
#else
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,1000,T("New &Folder"));
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,1001,T("New Standard &Disk Image"));
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,1002,T("New Custom Disk &Image"));
#if defined(SSE_GUI_DM_STW) //new context option
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,1003,T("New ST&W Disk Image"));
#endif
#if defined(SSE_GUI_DM_HFE) //new context option
        InsertMenu(FolOptionsPop,0xffffffff,MF_BYPOSITION | MF_STRING,1004,T("New &HFE Disk Image"));
#endif
#endif
        POINT pt;
        GetCursorPos(&pt);
        TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                        pt.x,pt.y,0,Win,NULL);

        DestroyMenu(Pop);
      }else if (HWND(wPar)==GetDlgItem(Win,100) || HWND(wPar)==GetDlgItem(Win,101)){
        //SS right click in box on the right of disk icons
        if (SendMessage(HWND(wPar),LVM_GETITEMCOUNT,0,0)>0){
          HMENU Pop=CreatePopupMenu();

          LV_ITEM lvi;
          lvi.iItem=0;
          lvi.iSubItem=0;
          lvi.mask=LVIF_PARAM;
          SendMessage(HWND(wPar),LVM_GETITEM,0,(LPARAM)&lvi);
          DiskManFileInfo *Inf=(DiskManFileInfo*)lvi.lParam;

          if (HWND(wPar)==GetDlgItem(Win,100)){
            This->MenuTarget=0;
          }else{
            This->MenuTarget=1;
          }

          if (Inf->Zip==0){
            InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | int(Inf->ReadOnly ? MF_CHECKED:0),1050+This->MenuTarget,T("Read &Only"));
          }
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1100,T("&Swap"));
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR | MF_STRING,999,"-");

          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1101,T("&Remove Disk From Drive")+" \10DEL");
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_SEPARATOR,999,NULL);
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1091,T("&Go To Disk"));
#if defined(SSE_GUI_DM_LONG_NAMES1)
/*  This is so the player can read the full name of the disk without
    checking at the place of storage.
    If he clicks on it, it is copied in the clipboard, whatever use this
    then may have. (390 switch restored)
*/
#if defined(SSE_GUI_DM_NAME_CLIPBOARD)
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1082,
            Inf->Name.Text);
#else
          //If you click, you go there.
          InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,1091,
            Inf->Name.Text);
#endif
#endif
          POINT pt;
          GetCursorPos(&pt);
          TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                          pt.x,pt.y,0,Win,NULL);

          DestroyMenu(Pop);
        }
      }
      break;
    case WM_NOTIFY:
      if (wPar==102){  //DiskView Only
        switch (((NMHDR*)lPar)->code){
          case LVN_GETDISPINFO:
          {
            LV_DISPINFO *DispInf=(LV_DISPINFO*)lPar;

            DispInf->item.mask=LVIF_TEXT;
            DispInf->item.pszText=((DiskManFileInfo*)(DispInf->item.lParam))->Name.Text;
            break;
          }
          case LVN_BEGINLABELEDIT:
          {
            LV_DISPINFO *DispInf=(LV_DISPINFO*)lPar;

            if (DispInf->item.iItem==0) return true;

            return 0;
          }
          case LVN_ENDLABELEDIT:
          {
            LV_DISPINFO *DispInf=(LV_DISPINFO*)lPar;

            if (DispInf->item.pszText==NULL) return 0;

            GET_THIS;
            DiskManFileInfo *Inf=(DiskManFileInfo*)DispInf->item.lParam;
            bool Link=Inf->LinkPath.NotEmpty();

            bool InDrive[2]={0,0};
            EasyStr NewDiskName[2],OldDiskName[2],OldDiskPath[2],DiskInZip[2];
            EasyStr NewName=DispInf->item.pszText,OldName=Inf->Name,Extension;
            if (Link){
              Extension=".lnk";
            }else{
              if (Inf->Folder==0){
                Extension=strrchr(Inf->Path,'.');
                for (int disk=0;disk<2;disk++){
                  OldDiskName[disk]=FloppyDrive[disk].DiskName;
                  if (OldDiskName[disk]==OldName){
                    NewDiskName[disk]=NewName;
                  }else{
                    NewDiskName[disk]=OldDiskName[disk];
                  }

                  OldDiskPath[disk]=FloppyDrive[disk].GetDisk();
                  DiskInZip[disk]=FloppyDrive[disk].DiskInZip;
                  if (IsSameStr_I(OldDiskPath[disk],This->DisksFol+"\\"+OldName+Extension)){
                    InDrive[disk]=true;
                    FloppyDrive[disk].RemoveDisk();
                  }
                }
              }else{
                // Check for inserted disks being in renamed folder
              }
            }
#if defined(SSE_GUI_DM_SHOW_EXT)
            BOOL success;
            if(This->HideExtension)
              success=MoveFile(This->DisksFol+"\\"+OldName+Extension,This->DisksFol+"\\"+NewName+Extension);
            else
              success=MoveFile(This->DisksFol+"\\"+OldName,This->DisksFol+"\\"+NewName);
            if(success) {
#else
            if (MoveFile(This->DisksFol+"\\"+OldName+Extension,This->DisksFol+"\\"+NewName+Extension)){
#endif
              if (Link){
                Inf->LinkPath=This->DisksFol+"\\"+NewName+Extension;
              }else{
#if defined(SSE_GUI_DM_SHOW_EXT)
                Str NewPath=This->DisksFol+"\\"+NewName;
                if(This->HideExtension)
                  NewPath+=Extension;
#else
                Str NewPath=This->DisksFol+"\\"+NewName+Extension;
#endif
                if (Inf->Folder==0) This->UpdateBPBFiles(Inf->Path,NewPath,true);
                Inf->Path=NewPath;
              }
              Inf->Name=NewName;
              for (int disk=0;disk<2;disk++){
                if (InDrive[disk]){
                  This->InsertHistoryDelete(disk,OldDiskName[disk],This->DisksFol+"\\"+OldName+Extension,DiskInZip[disk]);
                  This->InsertHistoryAdd(disk,NewDiskName[disk],This->DisksFol+"\\"+NewName+Extension,DiskInZip[disk]);
                  FloppyDrive[disk].SetDisk(This->DisksFol+"\\"+NewName+Extension,DiskInZip[disk]);
                  FloppyDrive[disk].DiskName=NewDiskName[disk];

                  HWND LV=GetDlgItem(Win,100+disk);

                  Inf=This->GetItemInf(0,LV);
                  Inf->Path=FloppyDrive[disk].GetDisk();
                  if (Inf->Name==OldDiskName[disk]){
                    Inf->Name=NewDiskName[disk];
                    LV_ITEM lvi;
                    lvi.mask=LVIF_TEXT;
                    lvi.iItem=0;
                    lvi.iSubItem=0;
                    lvi.pszText=NewDiskName[disk].Text;
                    SendMessage(LV,LVM_SETITEM,0,(LPARAM)&lvi);

                    CentreLVItem(LV,0);
                  }
                }
              }
              return true;
            }else{
#if defined(SSE_GUI_DM_SHOW_EXT) // eg STW, system blocks access
              Alert(T("Unable to rename file (read-only? inserted?)"),T("Error"),MB_ICONEXCLAMATION);
#else
              Alert(EasyStr(T("Unable to rename"))+" "+This->DisksFol+"\\"+OldName+Extension+" "+T("to")+" "+This->DisksFol+"\\"+NewName+Extension,T("Error"),MB_ICONEXCLAMATION);
#endif
              for (int disk=0;disk<2;disk++){
                if (InDrive[disk]){
                  FloppyDrive[disk].SetDisk(This->DisksFol+"\\"+OldName+Extension,DiskInZip[disk]);
                  FloppyDrive[disk].DiskName=OldDiskName[disk];
                }

              }
            }
            return 0;
          }
          case LVN_KEYDOWN:
          {
            GET_THIS;
            if (This->Dragging>-1) break;

            LV_KEYDOWN *KeyInf=(LV_KEYDOWN*)lPar;

            switch (KeyInf->wVKey){
              case VK_RETURN:case VK_SPACE:
                PostMessage(Win,WM_USER,1234,0);
                break;
              case VK_BACK:
                PostMessage(Win,WM_USER,1234,1);
                break;
              case VK_DELETE:
                PostMessage(Win,WM_USER,1234,2);
                break;
              case VK_F2:
              {
                GET_THIS;
                int SelItem=This->GetSelectedItem();
                if (SelItem>-1) SendMessage(This->DiskView,LVM_EDITLABEL,SelItem,0);
                break;
              }
              case VK_F3:
              {
                GET_THIS;
                int SelItem=This->GetSelectedItem();
                if (SelItem>-1){
                  DiskManFileInfo *Inf=This->GetItemInf(SelItem);
                  if (Inf->Folder){
                    ShellExecute(NULL,"Find",Inf->Path,"","",SW_SHOWNORMAL);
                  }else{
                    SelItem=-1;
                  }
                }
                if (SelItem==-1){
                  ShellExecute(NULL,"Find",This->DisksFol,"","",SW_SHOWNORMAL);
                }
                break;
              }
              case VK_F4:
                PostMessage(Win,WM_COMMAND,2003,0);
                break;
              case VK_F5:
                PostMessage(Win,WM_COMMAND,IDCANCEL,0);
                break;
              case VK_F6:
                PostMessage(Win,WM_COMMAND,2030,0);
                break;
              case VK_F9:
                PostMessage(Win,WM_COMMAND,2025,0);
                break;
            }
            break;
          }
          case NM_DBLCLK:
            GET_THIS;
            if (This->Dragging>-1) break;

            PostMessage(Win,WM_USER,1234,0);
            break;
        }
      }
      if (wPar>=100 && wPar<=102){ //ListView
        switch (((NMHDR*)lPar)->code){
          case LVN_DELETEITEM:
          {
            LV_ITEM lvi;
            lvi.mask=LVIF_PARAM;
            lvi.iItem=((NM_LISTVIEW*)lPar)->iItem;
            lvi.iSubItem=0;
            SendMessage(GetDlgItem(Win,wPar),LVM_GETITEM,0,(LPARAM)&lvi);
            delete ((DiskManFileInfo*)lvi.lParam);
            if (wPar<102){
              GET_THIS;
              
              This->SetDriveViewEnable(wPar-100,0);
            }
            break;
          }
          case LVN_BEGINDRAG:case LVN_BEGINRDRAG:
          {
            GET_THIS;

            This->BeginDrag(((NM_LISTVIEW*)lPar)->iItem,GetDlgItem(Win,wPar));

            break;
          }
        }
        if (wPar<102){
          switch (((NMHDR*)lPar)->code){
            case LVN_KEYDOWN:
              LV_KEYDOWN *KeyInf=(LV_KEYDOWN*)lPar;

              if (KeyInf->wVKey==VK_DELETE){
                GET_THIS;
                This->EjectDisk(wPar-100);
              }
              break;
          }
        }
      }
      break;
    case WM_MOUSEMOVE:
      GET_THIS;
      if (This->Dragging>-1) This->MoveDrag();

      break;
    case WM_TIMER:
      GET_THIS;

      if (wPar==DISKVIEWSCROLL_TIMER_ID){
        if (This->DragLV==This->DiskView && (This->LastOverID!=80 || This->AtHome)){
          POINT spt;
          GetCursorPos(&spt);
          RECT rc;
          GetWindowRect(This->DiskView,&rc);
          if (spt.x>=rc.left && spt.y<=rc.right){
            int y=0;
            if (spt.y<=rc.top+2 && spt.y>=rc.top-20){
              y=-5;
            }else if (spt.y>=rc.bottom-2 && spt.y<=rc.bottom+10){
              y=5;
            }
            if (y){
              if (This->DragEntered){
                ImageList_DragLeave(Win);
                This->DragEntered=0;
              }
              SendMessage(This->DiskView,LVM_SCROLL,0,y);
              UpdateWindow(This->DiskView);
            }
          }
        }
      }else if (wPar==MSACONV_TIMER_ID){
        bool Kill=true;
        DWORD Code;
        if (GetExitCodeProcess(This->MSAConvProcess,&Code)){
          if (Code==STILL_ACTIVE){
            Kill=0;
          }else{
            This->GoToDisk(This->MSAConvSel,true);
            This->MSAConvSel="";
            This->MSAConvProcess=NULL;
          }
        }
        if (Kill) KillTimer(Win,MSACONV_TIMER_ID);
      }
      break;
    case WM_LBUTTONUP:case WM_RBUTTONUP:
      GET_THIS;

      if (This->Dragging>-1){
        This->EndDrag(LOWORD(lPar),HIWORD(lPar),(Mess==WM_RBUTTONUP));
      }
      break;
    case WM_CAPTURECHANGED:
      GET_THIS;
      if (This->EndingDrag==0){
        if (This->Dragging>-1){
          if (This->DragEntered){
            ImageList_DragLeave(Win);
            This->DragEntered=0;
          }
          ImageList_EndDrag();
          SendMessage(GetDlgItem(Win,80),BM_SETCHECK,0,0);
          This->Dragging=-1;

          InvalidateRect(This->DiskView,NULL,true);
        }
      }
      break;

    case WM_USER:
      GET_THIS;

      if (wPar==1234){
        LV_ITEM lvi;
        int c=SendMessage(This->DiskView,LVM_GETITEMCOUNT,0,0);

        lvi.mask=LVIF_PARAM | LVIF_STATE;
        lvi.iSubItem=0;
        lvi.stateMask=LVIS_SELECTED;
        for (lvi.iItem=0;lvi.iItem<c;lvi.iItem++){
          SendMessage(This->DiskView,LVM_GETITEM,0,(LPARAM)&lvi);
          if (lvi.state==LVIS_SELECTED) break;
        }
        DiskManFileInfo *Inf=(DiskManFileInfo*)lvi.lParam;

        if (lPar==0 && lvi.iItem<c){
          if (Inf->Folder){
            if (Inf->UpFolder){
              lPar=1;
            }else{
              This->SetDir(Inf->Path,true);
            }
          }else{
            DiskManFileInfo *Inf=This->GetItemInf(This->GetSelectedItem());
            if (Inf){
              switch (This->DoubleClickAction){
                case 1: This->PerformInsertAction(0,Inf->Name,Inf->Path,""); break;
                case 2: This->PerformInsertAction(2,Inf->Name,Inf->Path,""); break;
              }
            }
          }
        }

#if defined(SSE_GUI_DM_DOUBLE_CLK_GO_UP) //easy, not optimised
        else if(!lPar)
          lPar++; // go up
#endif

        if (lPar==1){  // Go Up
          EasyStr Fol=This->DisksFol;
          char *LastSlash=strrchr(Fol,'\\');
          if (LastSlash==NULL){
            LastSlash=strrchr(Fol,'/');
            if (LastSlash==NULL){
              LastSlash=strrchr(Fol,':');
            }

          }
          if (LastSlash!=NULL){
            *LastSlash=0;
            if (NotSameStr_I(Fol,This->DisksFol)){
              This->SetDir(Fol,true,This->DisksFol);
            }
          }
        }else if (lPar==2 && Inf->UpFolder==0){  //Delete
          char Fol[MAX_PATH+2];
          ZeroMemory(Fol,MAX_PATH+2);
          if (Inf->LinkPath.IsEmpty()){
            strcpy(Fol,Inf->Path);
          }else{
            strcpy(Fol,Inf->LinkPath);
          }

          EasyStr OldDisk[2],OldName[2],DiskInZip[2];
          for (int disk=0;disk<2;disk++){
            if (IsSameStr_I(FloppyDrive[disk].GetDisk(),Fol)){
              OldDisk[disk]=Fol;
              OldName[disk]=FloppyDrive[disk].DiskName;
              DiskInZip[disk]=FloppyDrive[disk].DiskInZip;
              FloppyDrive[disk].RemoveDisk();
            }
          }

          SHFILEOPSTRUCT fos;
          fos.hwnd=HWND(FullScreen ? StemWin:This->Handle);
          fos.wFunc=FO_DELETE;
          fos.pFrom=Fol;

          fos.pTo="\0\0";
          fos.fFlags=FILEOP_FLAGS(int((GetKeyState(VK_SHIFT)<0) ? 0:FOF_ALLOWUNDO) | int(FullScreen ? FOF_SILENT:0));
          fos.hNameMappings=NULL;
          fos.lpszProgressTitle=StaticT("Deleting...");
          EnableWindow(This->Handle,0);
          SHFileOperation(&fos);
          EnableWindow(This->Handle,true);

          for (int disk=0;disk<2;disk++){
            if (OldDisk[disk].NotEmpty()){
              if (fos.fAnyOperationsAborted){
                FloppyDrive[disk].SetDisk(OldDisk[disk],DiskInZip[disk]);
                FloppyDrive[disk].DiskName=OldName[disk];
              }else{
                SendMessage(GetDlgItem(Win,100+disk),LVM_DELETEITEM,0,0);
              }
            }
          }
          if (fos.fAnyOperationsAborted==0){
            if (Inf->LinkPath.IsEmpty() && Inf->Folder==0){ // Deleting disk
              This->UpdateBPBFiles(Inf->Path,"",0);
            }
            This->RefreshDiskView("",0,"",lvi.iItem);
          }
        }
      }
#ifdef SSE_GUI_DM_HD_SELECTED
     SendMessage(GetDlgItem(Win,10),BM_SETCHECK,
       !HardDiskMan.DisableHardDrives || HardDiskMan.IsVisible(),0);
#else
      SendMessage(GetDlgItem(Win,10),BM_SETCHECK,HardDiskMan.IsVisible(),0);
#endif
#ifdef SSE_ACSI_ICON 
      SendMessage(GetDlgItem(Win,11),BM_SETCHECK,
        SSEOption.Acsi
#if defined(SSE_ACSI_HDMAN)
        || AcsiHardDiskMan.IsVisible()
#endif
        ,0);
#endif

      break;

    case WM_SIZE:
      GET_THIS;

      if (GetDlgItem(Win,10)){
        SetWindowPos(GetDlgItem(Win,10),0,LOWORD(lPar)-70,10,0,0,SWP_NOSIZE | SWP_NOZORDER);
#ifdef SSE_ACSI_ICON 
        SetWindowPos(GetDlgItem(Win,11),0,LOWORD(lPar)-70*2,10,0,0,SWP_NOSIZE | SWP_NOZORDER);
#endif

        SetWindowPos(GetDlgItem(Win,97),0,0,0,LOWORD(lPar)-(125+26+50+10),21,SWP_NOMOVE | SWP_NOZORDER);

        SetWindowPos(GetDlgItem(Win,102),0,0,0,LOWORD(lPar)-20,HIWORD(lPar)-114,SWP_NOMOVE | SWP_NOZORDER);
        if (This->SmallIcons==0) SendMessage(GetDlgItem(Win,102),LVM_ARRANGE,LVA_DEFAULT,0);
      }
      if (FullScreen){
        if (IsZoomed(Win)==0){
          This->FSMaximized=0;

          RECT rc;GetWindowRect(Win,&rc);
          This->FSWidth=rc.right-rc.left;This->FSHeight=rc.bottom-rc.top;
        }else{
          This->FSMaximized=true;
        }
      }else{
        if (IsIconic(Win)==0){
          if (IsZoomed(Win)==0){
            This->Maximized=0;

            RECT rc;GetWindowRect(Win,&rc);
            This->Width=rc.right-rc.left;This->Height=rc.bottom-rc.top;
          }else{
            This->Maximized=true;
          }
        }
      }
      break;
    case (WM_USER+1011):
    {
      GET_THIS;
      ASSERT(This);

      if (This->VisibleDiag()){
        SendMessage(This->VisibleDiag(),WM_COMMAND,IDCANCEL,0);
      }

      HWND NewParent=(HWND)lPar;
      if (NewParent){
//        SetWindowLong(Win,GWL_STYLE,GetWindowLong(Win,GWL_STYLE) & ~WS_MINIMIZEBOX);
        This->CheckFSPosition(NewParent);
        SetWindowPos(Win,NULL,This->FSLeft,This->FSTop,This->FSWidth,This->FSHeight,SWP_NOZORDER);
      }else{
//        SetWindowLong(Win,GWL_STYLE,GetWindowLong(Win,GWL_STYLE) | WS_MINIMIZEBOX);
        SetWindowPos(Win,NULL,This->Left,This->Top,This->Width,This->Height,SWP_NOZORDER);
      }
      This->ChangeParent(NewParent);
      break;
    }
    case WM_GETMINMAXINFO:
    {
      MINMAXINFO *mmi=(MINMAXINFO*)lPar;
#if defined(SSE_ACSI_ICON) //report Dima
      mmi->ptMinTrackSize.x=403+70+GetSystemMetrics(SM_CXFRAME)*2+GetSystemMetrics(SM_CXVSCROLL);
#else
      mmi->ptMinTrackSize.x=403+GetSystemMetrics(SM_CXFRAME)*2+GetSystemMetrics(SM_CXVSCROLL);
#endif
      mmi->ptMinTrackSize.y=186+GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFRAME)*2;
      if (FullScreen){
        mmi->ptMaxSize.x=GetSystemMetrics(SM_CXSCREEN)+GetSystemMetrics(SM_CXFRAME)*2;
        mmi->ptMaxSize.y=GetSystemMetrics(SM_CYSCREEN)+GetSystemMetrics(SM_CYFRAME)-MENUHEIGHT;
        mmi->ptMaxPosition.x=-GetSystemMetrics(SM_CXFRAME);
        mmi->ptMaxPosition.y=MENUHEIGHT;
      }else{
        mmi->ptMaxPosition.x=-GetSystemMetrics(SM_CXFRAME);
        mmi->ptMaxPosition.y=-GetSystemMetrics(SM_CYFRAME);
      }
      break;
    }
    case WM_CLOSE:
      GET_THIS;
      This->Hide();
      return 0;
  }
  return DefWindowProc(Win,Mess,wPar,lPar);
}
#pragma warning (default: 4701)
//---------------------------------------------------------------------------
LRESULT __stdcall TDiskManager::Drive_Icon_WndProc(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar)
{
  TDiskManager *This;

  int disk=GetDlgCtrlID(Win)-98; // SS magic cst...
  switch (Mess){
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      RECT box;
      HBRUSH br;

      BeginPaint(Win,&ps);

      GetClientRect(Win,&box);
      br=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
      FillRect(ps.hdc,&box,br);
      if (disk==1 && num_connected_floppies==1){
        DrawIconEx(ps.hdc,0,0,hGUIIcon[RC_ICO_DRIVEB_DISABLED],64,64,0,NULL,DI_NORMAL);
      }else{
        DrawIconEx(ps.hdc,0,0,hGUIIcon[RC_ICO_DRIVEA+disk],64,64,0,br,DI_NORMAL);
      }
#if !defined(SSE_FLOPPY_ALWAYS_ADAT)
      if (floppy_instant_sector_access==0){ // SS draw snails!
        DrawIconEx(ps.hdc,24,48,hGUIIcon[RC_ICO_ACCURATEFDC],16,16,0,NULL,DI_NORMAL);
      }
#endif
      DeleteObject(br);

      EndPaint(Win,&ps);
      return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
      GET_THIS;
      if (disk==1) This->SetNumFloppies(3-num_connected_floppies);
      return 0;

#if defined(SSE_DRIVE_SINGLE_SIDE) //TODO switch for those context options
    case WM_RBUTTONDOWN: // right click on drive, make context menu
    {
      GET_THIS;
      ASSERT(disk==(disk&1));
      This->MenuTarget=disk;
      HMENU Pop=CreatePopupMenu();
#if defined(SSE_DRIVE_SINGLE_SIDE_NOPASTI)
      if(SF314[disk].ImageType.Manager!=MNGR_PASTI)
#endif
      InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING | 
        (int)(( (SSEOption.SingleSideDriveMap)&(This->MenuTarget+1)) ? 
        MF_CHECKED:0),1048+This->MenuTarget,
        T("SF354 (single side - caution!)"));

#if defined(SSE_DRIVE_SWITCH_OFF_MOTOR)
      if(SF314[This->MenuTarget].State.motor)
      {
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,
          1046+This->MenuTarget,T("Stop motor"));
      }

#if defined(SSE_DRIVE_SOUND_SEEK_OPTION)
      if(OPTION_DRIVE_SOUND)
      {
        InsertMenu(Pop,0xffffffff,MF_BYPOSITION | MF_STRING,
          1044+This->MenuTarget,T("Toggle seek sound"));
      }
#endif

#endif
      POINT pt;
      GetCursorPos(&pt); // menu will appear at the mouse pointer
      TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
        pt.x,pt.y,0,This->Handle,NULL);
      DestroyMenu(Pop);
      return 0; // mimic
    }
#endif

    case WM_COMMAND:
      GET_THIS;
      if (LOWORD(wPar)==100){
        SendMessage(HWND(lPar),BM_SETCHECK,1,0);
        HMENU Pop=CreatePopupMenu();

        EasyStr CurrentDiskName=This->CreateDiskName(FloppyDrive[disk].DiskName,FloppyDrive[disk].DiskInZip);
        for (int n=0;n<10;n++){
          if (This->InsertHist[disk][n].Path.NotEmpty()){
            EasyStr MenuItemText=This->CreateDiskName(This->InsertHist[disk][n].Name,This->InsertHist[disk][n].DiskInZip);
            if (NotSameStr_I(CurrentDiskName,MenuItemText)){
              AppendMenu(Pop,MF_STRING,200+n,MenuItemText);
            }
          }
        }

        RECT rc;
        GetWindowRect(HWND(lPar),&rc);
        TrackPopupMenu(Pop,TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                        rc.left,rc.bottom,0,Win,NULL);

        DestroyMenu(Pop);
        SendMessage(HWND(lPar),BM_SETCHECK,0,0);
      }else if (LOWORD(wPar)>=200 && LOWORD(wPar)<210){
        This->InsertDisk(disk,This->InsertHist[disk][LOWORD(wPar)-200].Name,
                              This->InsertHist[disk][LOWORD(wPar)-200].Path,0,true,
                              This->InsertHist[disk][LOWORD(wPar)-200].DiskInZip,0,true);
      }
      break;
  }
  return DefWindowProc(Win,Mess,wPar,lPar);
}
//---------------------------------------------------------------------------
LRESULT __stdcall TDiskManager::DriveView_WndProc(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar)
{
  TDiskManager *This;

  GET_THIS;

  if (Mess==WM_DROPFILES){
    int nFiles=DragQueryFile((HDROP)wPar,0xffffffff,NULL,0);
    for (int i=0;i<nFiles;i++){
      EasyStr File;
      File.SetLength(MAX_PATH);
      DragQueryFile((HDROP)wPar,i,File,MAX_PATH);
      char *dot=strrchr(GetFileNameFromPath(File),'.');
      if (dot){
        if (IsSameStr_I(dot,".LNK")){
          WIN32_FIND_DATA wfd;
          File=GetLinkDest(File,&wfd);
          dot=strrchr(GetFileNameFromPath(File),'.');
        }
      }
      if (dot){
        if (ExtensionIsDisk(dot)){
          EasyStr Name=GetFileNameFromPath(File);
          *strrchr(Name,'.')=0;

          if (DiskMan.InsertDisk(GetDlgCtrlID(Win)-100,Name,File,0,0,"",0,true)) break;
        }
      }
    }
    DragFinish((HDROP)wPar);
    SetForegroundWindow(This->Handle);
    return 0;
  }else if (Mess==WM_KEYDOWN && This->Dragging>-1){
    return 0;
  }else if (Mess==WM_LBUTTONDOWN || Mess==WM_MBUTTONDOWN || Mess==WM_RBUTTONDOWN ||
            Mess==WM_LBUTTONDBLCLK || Mess==WM_MBUTTONDBLCLK || Mess==WM_RBUTTONDBLCLK){
    if (SendMessage(Win,LVM_GETITEMCOUNT,0,0)==0) return 0;
  }
  return CallWindowProc(This->Old_ListView_WndProc,Win,Mess,wPar,lPar);
}
//---------------------------------------------------------------------------
LRESULT __stdcall TDiskManager::DiskView_WndProc(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar)
{
  TDiskManager *This;

  GET_THIS;

  if (Mess==WM_DROPFILES){
    POINT pt;
    GetCursorPos(&pt);

    This->MenuTarget=0;

    HMENU OpMenu=CreatePopupMenu();
    AppendMenu(OpMenu,MF_STRING,4000,T("&Move Here"));
    AppendMenu(OpMenu,MF_STRING,4001,T("&Copy Here"));
    AppendMenu(OpMenu,MF_STRING,4002,T("Create &Shortcut(s) Here"));
    AppendMenu(OpMenu,MF_SEPARATOR,4099,NULL);
    AppendMenu(OpMenu,MF_STRING,4098,T("Cancel"));

    TrackPopupMenu(OpMenu,TPM_LEFTALIGN | TPM_LEFTBUTTON,pt.x,pt.y,0,This->Handle,NULL);
    DestroyMenu(OpMenu);

    MSG mess;
    while (PeekMessage(&mess,This->Handle,WM_COMMAND,WM_COMMAND,PM_REMOVE)) DispatchMessage(&mess);

    EasyStr SelPath,SelLink;
    if (This->MenuTarget>=4000 && This->MenuTarget<=4002){
      int nFiles=DragQueryFile((HDROP)wPar,0xffffffff,NULL,0);
      if (This->MenuTarget==4002){
        EasyStr File,Name,LinkFile;

        for (int i=0;i<nFiles;i++){
          File.SetLength(MAX_PATH);
          DragQueryFile((HDROP)wPar,i,File,MAX_PATH);

          Name.SetLength(MAX_PATH);
          GetLongPathName(File,Name,MAX_PATH);
          Name=EasyStr(GetFileNameFromPath(Name));
          char *dot=strrchr(Name,'.');
          if (dot){
            if (ExtensionIsDisk(dot) || ExtensionIsPastiDisk(dot)) *dot=0;
          }

          LinkFile=This->DisksFol+"\\"+Name+".lnk";
          int n=2;
          while (Exists(LinkFile)){
            LinkFile=This->DisksFol+"\\"+Name+" ("+(n++)+").lnk";
          }
          CreateLink(LinkFile,File);
          SelLink=LinkFile;
        }
      }else{
        char *From=new char[MAX_PATH*nFiles + 2];
        ZeroMemory(From,MAX_PATH*nFiles + 2);
        char *FromPtr=From;
        for (int i=0;i<nFiles;i++){
          DragQueryFile((HDROP)wPar,i,FromPtr,MAX_PATH);
          // support links
          if (FileIsDisk(FromPtr)) SelPath=This->DisksFol+SLASH+GetFileNameFromPath(FromPtr);
          FromPtr+=strlen(FromPtr)+1;
        }

        SHFILEOPSTRUCT fos;
        fos.hwnd=This->Handle;
        fos.wFunc=int((This->MenuTarget==4000) ? FO_MOVE:FO_COPY);
        fos.pFrom=From;
        fos.pTo=This->DisksFol;
        fos.fFlags=FILEOP_FLAGS(FOF_ALLOWUNDO) | FOF_RENAMEONCOLLISION;
        fos.hNameMappings=NULL;
        fos.lpszProgressTitle=LPSTR((This->MenuTarget==4000) ? StaticT("Moving..."):StaticT("Copying..."));
        EnableWindow(This->Handle,0);
        SHFileOperation(&fos);
        EnableWindow(This->Handle,true);
      }
      This->RefreshDiskView(SelPath,0,SelLink);
      SetForegroundWindow(This->Handle);
    }
    DragFinish((HDROP)wPar);
    return 0;
  }else if (Mess==WM_KEYDOWN && This->Dragging>-1){
    return 0;
  }else if (Mess==WM_VSCROLL && This->Dragging>-1){
    ImageList_DragLeave(This->Handle);

    LRESULT Ret=CallWindowProc(This->Old_ListView_WndProc,Win,Mess,wPar,lPar);
    UpdateWindow(Win);

    POINT mpt;
    GetCursorPos(&mpt);
    ScreenToClient(This->Handle,&mpt);
    ImageList_DragEnter(This->Handle,mpt.x-This->DragWidth,mpt.y-This->DragHeight);
    This->MoveDrag();
    return Ret;
  }
  return CallWindowProc(This->Old_ListView_WndProc,Win,Mess,wPar,lPar);
}
#undef GET_THIS
//---------------------------------------------------------------------------
void TDiskManager::SetDriveViewEnable(int Drive,bool EnableIt)
{
  HWND LV=GetDlgItem(Handle,100+Drive);

  if (GetFocus()==LV) SetFocus(DiskView);
  
  SendMessage(LV,LVM_SETBKCOLOR,0,(LPARAM)GetSysColor(int(EnableIt ? COLOR_WINDOW:COLOR_BTNFACE)));

  if (EnableIt){
    SetWindowLong(LV,GWL_STYLE,GetWindowLong(LV,GWL_STYLE) | WS_TABSTOP);
  }else{
    SetWindowLong(LV,GWL_STYLE,GetWindowLong(LV,GWL_STYLE) & ~WS_TABSTOP);
  }
  InvalidateRect(LV,NULL,true);
}
#endif//WIN32
//---------------------------------------------------------------------------
#ifdef UNIX
#include "x/x_diskman.cpp"
#endif
//---------------------------------------------------------------------------
bool TDiskManager::CreateDiskImage(char *STName,int Sectors,int SecsPerTrack,int Sides)
{
  FILE *f=fopen(STName,"wb");
  if (f){
    {//scope to free stack (3.6.1)
    char zeros[512];
    ZeroMemory(zeros,sizeof(zeros));
    for (int n=0;n<Sectors;n++) fwrite(zeros,1,512,f);
    }//SS scope
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//LITTLE-ENDIAN ONLY             ******************************************************
    int buf;
    fseek(f,0,SEEK_SET);
    fputc(0xeb,f);fputc(0x30,f);
    fseek(f,8,SEEK_SET); // Skip loader
    fputc(BYTE(rand()),f);fputc(BYTE(rand()),f);fputc(BYTE(rand()),f); //Serial number
    buf=512;                          fwrite(&buf,2,1,f); //Bytes Per Sector
    buf=2;                            fwrite(&buf,1,1,f); //Sectors Per Cluster
    buf=1;                            fwrite(&buf,2,1,f); //RES
    buf=2;                            fwrite(&buf,1,1,f); //FATs
    buf=112;                          fwrite(&buf,2,1,f); //Dir Entries
    buf=Sectors;                      fwrite(&buf,2,1,f);
#if defined(SSE_DRIVE_CREATE_ST_DISK_FIX) // from Petari
    buf=249;                          fwrite(&buf,1,1,f); //Unused - MSDOS signo in fact
    
    // 3 sectors per FAT is enough up to 1MB disk capacity
    // 5 sectors per FAT up to 1700 KB - so HD, 20 sectors per track too
    
    if (Sectors<2000)     buf=3;                           
    else      buf=5;   
    fwrite(&buf,2,1,f);    //Sectors Per FAT
#else   
    buf=249;                          fwrite(&buf,1,1,f); //Unused
    buf=5;                            fwrite(&buf,2,1,f); //Sectors Per FAT
#endif
    buf=SecsPerTrack;                 fwrite(&buf,2,1,f);
    buf=Sides;                        fwrite(&buf,2,1,f);
    buf=0;                            fwrite(&buf,2,1,f); //Hidden Sectors
    fseek(f,510,SEEK_SET);
#if defined(SSE_DRIVE_CREATE_ST_DISK_FIX) // from Petari
    fputc(0x97,f);fputc(0xc7,f);  // WTF ?
    fputc(0xf0,f);fputc(0xff,f);fputc(0xff,f);  // First FAT begin
    // Second FAT too ! :
    if (Sectors<2000)    fseek(f,2048,SEEK_SET);
    else  fseek(f,3072,SEEK_SET);
    fputc(0xf0,f);fputc(0xff,f);fputc(0xff,f);  // Second FAT begin
#else
    fputc(0x97,f);fputc(0xc7,f);
    fputc(0xf0,f);fputc(0xff,f);fputc(0xff,f);
#endif
    fclose(f);
    DeleteFile(Str(STName)+".steembpb");
    return true;
  }
  return 0;
}
//---------------------------------------------------------------------------
bool TDiskManager::ToggleVisible()
{
  if (HardDiskMan.IsVisible()){
    HardDiskMan.Show();
  }else{
    IsVisible() ? Hide():Show();
  }
  return IsVisible();
}
//---------------------------------------------------------------------------
void TDiskManager::SwapDisks(int FocusDrive)
{
#ifdef WIN32
  HWND FocusTo=NULL;
  if (GetForegroundWindow()==Handle && Handle){
    if (FocusDrive>-1){
      FocusTo=GetDlgItem(Handle,100+FocusDrive);
    }else{
      FocusTo=GetFocus();
    }
    if (GetFocus()==GetDlgItem(Handle,100) || GetFocus()==GetDlgItem(Handle,101)) SetFocus(NULL);
  }
#endif

  EasyStr Disk[2];
  Disk[0]=FloppyDrive[0].GetDisk();
  Disk[1]=FloppyDrive[1].GetDisk();
  EasyStr Name[2]={FloppyDrive[0].DiskName,FloppyDrive[1].DiskName};
  EasyStr DiskInZip[2]={FloppyDrive[0].DiskInZip,FloppyDrive[1].DiskInZip};
  bool HadDisk[2]={FloppyDrive[0].NotEmpty(),FloppyDrive[1].NotEmpty()};

  FloppyDrive[0].RemoveDisk();
  FloppyDrive[1].RemoveDisk();

#ifdef WIN32
  if (Handle){
    if (HadDisk[0]) SendMessage(GetDlgItem(Handle,100),LVM_DELETEITEM,0,0);
    if (HadDisk[1]) SendMessage(GetDlgItem(Handle,101),LVM_DELETEITEM,0,0);
  }
#endif

  if (HadDisk[1]) InsertDisk(0,Name[1],Disk[1],0,0,DiskInZip[1]);
  if (HadDisk[0]) InsertDisk(1,Name[0],Disk[0],0,0,DiskInZip[0]);

  WIN_ONLY( if (FocusTo) SetFocus(FocusTo); )
	UNIX_ONLY( UpdateDiskNames(0);UpdateDiskNames(1); )
}
//---------------------------------------------------------------------------
bool TDiskManager::AreNewDisksInHistory(int d)
{
  EasyStr CurrentDiskName=CreateDiskName(FloppyDrive[d].DiskName,FloppyDrive[d].DiskInZip);
  for (int n=0;n<10;n++){
    if (InsertHist[d][n].Path.NotEmpty()){

      EasyStr MenuItemText=CreateDiskName(InsertHist[d][n].Name,InsertHist[d][n].DiskInZip);
      if (NotSameStr_I(CurrentDiskName,MenuItemText)){

        return true;
      }
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
void TDiskManager::InsertHistoryAdd(int d,char *Name,char *Path,char *DiskInZip)
{
  InsertHistoryDelete(d,Name,Path,DiskInZip);
  for (int n=9;n>0;n--){
    InsertHist[d][n]=InsertHist[d][n-1];
  }
  InsertHist[d][0].Name=Name;
  InsertHist[d][0].Path=Path;
  InsertHist[d][0].DiskInZip=DiskInZip;

#ifdef WIN32
  if (Handle){
    EnableWindow(GetDlgItem(GetDlgItem(Handle,98+d),100),AreNewDisksInHistory(d));
  }
#endif
}
//---------------------------------------------------------------------------
void TDiskManager::InsertHistoryDelete(int d,char *Name,char *Path,char *DiskInZip)
{
  for (int n=0;n<10;n++){
    if (IsSameStr_I(Name,InsertHist[d][n].Name) &&
        IsSameStr_I(Path,InsertHist[d][n].Path) &&
        IsSameStr_I(DiskInZip,InsertHist[d][n].DiskInZip)){
      InsertHist[d][n].Path="";
    }
  }
  for (int n=0;n<10;n++){
    bool More=0;
    for (int i=n;i<10;i++){
      if (InsertHist[d][i].Path.NotEmpty()){
        More=true;
        break;
      }
    }
    if (More==0) break;
    if (InsertHist[d][n].Path.Empty()){
      for (int i=n;i<9;i++){
        InsertHist[d][i]=InsertHist[d][i+1];
      }
      n--;
    }
  }
}
//---------------------------------------------------------------------------
bool TDiskManager::InsertDisk(int Drive,EasyStr Name,EasyStr Path,bool DontChangeDisk,
                                bool MakeFocus,EasyStr DiskInZip,bool SuppressErr,bool AllowInsert2)
{
  
#ifdef SSE_DEBUG
  if(!DontChangeDisk)
    TRACE_LOG("%c: Inserting disk %s [%s]\n",Drive+'A',Name.c_str(),Path.c_str());
#endif

  if (DontChangeDisk==0){
    if (Path.Empty()) 
      return 0;
    int Err=FloppyDrive[Drive].SetDisk(Path,DiskInZip);
    if (Err){
      TRACE_LOG("Set Disk Error %d\n",Err);
      if (FloppyDrive[Drive].Empty()) EjectDisk(Drive); // Update display
      if (SuppressErr==0){
        switch (Err){
          case FIMAGE_WRONGFORMAT:
#if defined(SSE_GUI_DM_WRONG_IMAGE_ALERT)//3.6.1
            Alert(Path+": "+T("image not recognised!"),T("Disk Image Error"),MB_ICONEXCLAMATION);
#else
            Alert(Path+" "+T("is not in the correct format, it may be corrupt!"),T("Disk Image Error"),MB_ICONEXCLAMATION);
#endif
            break;
          case FIMAGE_CANTOPEN:
            Alert(Path+" "+T("cannot be opened."),T("Disk Image Error"),MB_ICONEXCLAMATION);
            break;
          case FIMAGE_FILEDOESNTEXIST:
            Alert(Path+" "+T("doesn't exist!"),T("Disk Image Error"),MB_ICONEXCLAMATION);
            break;
          case FIMAGE_CORRUPTZIP:
            Alert(Path+" "+T("does not contain any files, it may be corrupt!"),T("Archive Error"),MB_ICONEXCLAMATION);
            break;
          case FIMAGE_NODISKSINZIP:
            ExtractArchiveToSTHardDrive(Path);
            break;
          case FIMAGE_DIMNOMAGIC:
            //SS message is clear enough, people still report "bugs"...
            Alert(Path+" "+T("is not in the correct format, it may be corrupt!")+"\r\n\r\n"+
                    T("This image has the extension DIM, unfortunately many different disk imaging programs use that extension for different disk image formats.")+" "+
                    T("Sometimes DIM images are actually ST images with the incorrect extension.")+" "+
                    T("You may find you can use this image by changing the extension to .st.")+"\r\n"+
                    T("WARNING: Backup the disk image before you change the extension, inserting an image with the wrong extension could corrupt it."),
                    T("Disk Image Error"),MB_ICONEXCLAMATION);
            break;
          case FIMAGE_DIMTYPENOTSUPPORTED:
            Alert(Path+" "+T("is in a version of the DIM format that Steem currently doesn't support.")+" "+
                    T("If you have details for how to read this disk image please let us know and we'll support it in the next version."),
                    T("Disk Image Error"),MB_ICONEXCLAMATION);
            break;
        }
      }
      return 0;
    }
    FloppyDrive[Drive].DiskName=Name;

#if defined(SSE_OSD_SCROLLER_DISK_IMAGE)
    //TODO sometimes wrong name
    if(OSD_IMAGE_NAME && !OPTION_STATUS_BAR_GAME_NAME)
      OsdControl.StartScroller(Name); // display image disk name
#endif
#if defined(SSE_GUI_STATUS_BAR_DISK_NAME)
    GUIRefreshStatusBar();
#endif

    InsertHistoryAdd(Drive,Name,Path,DiskInZip);
#if defined(SSE_GUI_DM_INSERT_DISK_B_REMOVE)
#if defined(SSE_DISK_392B)
    if(AllowInsert2 && !Drive)
#endif
    AutoInsert2&=~2; //TODO def
#endif
    if (AllowInsert2 && Drive==0 && AutoInsert2){
      Err=1;
#if !defined(SSE_GUI_DM_INSERT_DISK_B)
      Str NewName=Name+" (2)";
#endif
      Str NewPath=Path;
      Str NewDiskInZip=DiskInZip;
#if !defined(SSE_GUI_DM_INSERT_DISK_B)
      if (NewDiskInZip.NotEmpty()){
        char *dot=strrchr(NewDiskInZip,'.');
        if (dot){
          dot--;
          if (*dot=='1') *dot='2', Err=0;
          if (*dot=='a') *dot='b', Err=0;
          if (*dot=='A') *dot='B', Err=0;
          if (Err==0) Err=InsertDisk(1,NewName,NewPath,0,0,NewDiskInZip,true,0);
        }
        if (Err) NewDiskInZip="";
      }
#endif
#if !defined(SSE_GUI_DM_INSERT_DISK_B)
      if (NewDiskInZip.Empty())
#endif
      {
        Err=1;
        char *dot=strrchr(NewPath,'.');
        if (dot){
          // The last symbol before the . must be A and B, it will not 
          //work with (A) and (B)
          dot--; 
          if (*dot=='1') *dot='2', Err=0;
          if (*dot=='a') *dot='b', Err=0;
          if (*dot=='A') *dot='B', Err=0;
#if defined(SSE_GUI_DM_INSERT_DISK_B)
          Str NewName=GetFileNameFromPath(NewPath);
          dot=strrchr(NewName,'.');
          NewName=NewName.Lefts(dot-NewName.Text);
#endif
#if defined(SSE_GUI_DM_INSERT_DISK_B_REMOVE)
          if (Err==0) 
          {
            InsertDisk(1,NewName,NewPath,0,0,NewDiskInZip,true,0);
            AutoInsert2|=2; //TODO def
          }
#else
          if (Err==0) InsertDisk(1,NewName,NewPath,0,0,NewDiskInZip,true,0);
#endif
        }
      }

    }
  }

#ifdef WIN32
  if (Handle==NULL) return true;

  HWND LV=GetDlgItem(Handle,100+Drive);

  if (SendMessage(LV,LVM_GETITEMCOUNT,0,0)) SendMessage(LV,LVM_DELETEITEM,0,0);

  SetDriveViewEnable(Drive,true);

  if (GetForegroundWindow()==Handle && MakeFocus) SetFocus(LV);

  Name=CreateDiskName(Name,DiskInZip);

  DiskManFileInfo *Inf=new DiskManFileInfo;
  Inf->Name=Name;
  Inf->Path=Path;
  Inf->Folder=0;
  Inf->UpFolder=0;
  Inf->ReadOnly=FloppyDrive[Drive].ReadOnly;
  Inf->BrokenLink=0;
  Inf->Zip=FloppyDrive[Drive].IsZip();

  LV_ITEM lvi;
  lvi.mask=LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_STATE;
  lvi.iItem=0;
  lvi.iSubItem=0;
  lvi.iImage=int(Inf->Zip ? 8:(1+FloppyDrive[Drive].ReadOnly*4));
  lvi.stateMask=LVIS_SELECTED | LVIS_FOCUSED;
  lvi.state=LVIS_SELECTED | LVIS_FOCUSED;
#if defined(SSE_X64_390)
  lvi.lParam=(LPARAM)Inf;
#else
  lvi.lParam=(long)Inf;
#endif
  lvi.pszText=Inf->Name;
#if defined(SSE_X64_390)
  SendMessage(LV,LVM_INSERTITEM,0,(LPARAM)&lvi);
#else
  SendMessage(LV,LVM_INSERTITEM,0,(long)&lvi);
#endif
  CentreLVItem(LV,0,LVIS_SELECTED | LVIS_FOCUSED);
#elif defined(UNIX)
  UpdateDiskNames(Drive);
#endif
  return true;
}
//---------------------------------------------------------------------------
void TDiskManager::ExtractArchiveToSTHardDrive(Str Path)
{
#if defined(SSE_GUI_DM_WRONG_IMAGE_ALERT)
  if (Alert(Path+": "+T("Steem doesn't recognise any disk images.")+"\n\n"+
#else
  if (Alert(Path+" "+T("doesn't contain any disk images.")+"\n\n"+
#endif
          T("Would you like to extract the contents of this archive to an ST hard drive?"),
          T("Extract Contents?"),MB_ICONQUESTION | MB_YESNO)==IDNO) return;

  Str Name=GetFileNameFromPath(Path);
  char *dot=strrchr(Name,'.');
  if (dot) *dot=0;
  Str ExtractPath;
  int d=2;
  for (;d<26;d++){
    if (mount_flag[d]){
      ExtractPath=mount_path[d];
      break;
    }
  }
  if (ExtractPath.Empty()){
    ExtractPath=WriteDir+SLASH+"st_c";
    CreateDirectory(ExtractPath,NULL);
    if (HardDiskMan.NewDrive(ExtractPath)){
      d=2;
      HardDiskMan.update_mount();
    }else{
      Alert(T("Could not create a new hard drive."),T("Archive Error"),MB_ICONEXCLAMATION);
      return;
    }
  }
  
  Str Fol;
#ifdef WIN32
  Fol=GetUniquePath(ExtractPath,Name);
#else
  {
    if (Name.Length()>8) Name[8]=0;
    strupr(Name);
    struct stat s;
    bool first=true;
    for(;;){
      Fol=ExtractPath+"/"+Name;
      if (stat(Fol,&s)==-1) break;
      if (first){
        if (Name.Length()<7) Name+="_";
        if (Name.Length()<8) Name+="2";
        *Name.Right()='2';
        first=0;
      }else{
        (*Name.Right())++;
        if (Name.RightChar()==char('9'+1)) *Name.Right()='A';
      }
    }
  }
#endif
  CreateDirectory(Fol,NULL);

  EasyStringList sl;
  sl.Sort=eslNoSort;
  zippy.list_contents(Path,&sl,0);

  // If every file is in the same folder then strip it (stops annoying double folder)
  Str Temp=sl[0].String,FirstFol;
  for (int i=0;i<Temp.Length();i++){
    if (Temp[i]=='\\' || Temp[i]=='/'){
      Temp[i+1]=0;
      FirstFol=Temp;
      break;
    }
  }
  if (FirstFol.NotEmpty()){
    for (int s=1;s<sl.NumStrings;s++){
      if (strstr(sl[s].String,FirstFol)!=sl[s].String){
        FirstFol="";
        break;
      }
    }
  }

  // Extract all files, make sure we create all necessary directories
  for (int s=0;s<sl.NumStrings;s++){
    Str Dest=Fol+SLASH+(sl[s].String+FirstFol.Length());
    UNIX_ONLY( while (strchr(Dest,'\\')) (*strchr(Dest,'\\'))='/'; )

    Str ContainingPath=sl[s].String+FirstFol.Length();
    for (int i=0;i<ContainingPath.Length();i++){
      if (ContainingPath[i]=='\\' || ContainingPath[i]=='/'){
        char old=ContainingPath[i];
        ContainingPath[i]=0;
        if (GetFileAttributes(Fol+SLASH+ContainingPath)==0xffffffff){
          CreateDirectory(Fol+SLASH+ContainingPath,NULL);
        }
        ContainingPath[i]=old;
      }
    }

    if (Dest.RightChar()!='/' && Dest.RightChar()!='\\'){

      if (zippy.extract_file(Path,sl[s].Data[0],Dest,0,sl[s].Data[1])==ZIPPY_FAIL){
        Alert(T("Could not extract files, this archive may be corrupt!"),T("Archive Error"),MB_ICONEXCLAMATION);
        return;
      }
    }
  }

  Str STFol=Str(char('A'+d))+":\\";
  DirSearch ds(Fol);
  STFol+=ds.ShortName;
  ds.Close();
  if (Alert(T("Files successfully extracted to:")+"\n\n"+
              T("PC folder")+": "+Fol+"\n"+
              T("ST folder")+": "+STFol+"\n"+
              T("Would you like to run Steem and go to the GEM desktop now?"),
              T("Files Extracted"),MB_ICONQUESTION | MB_YESNO)==IDYES){
    PerformInsertAction(2,"","","");
  }
}
//---------------------------------------------------------------------------
void TDiskManager::EjectDisk(int Drive)
{
  FloppyDrive[Drive].RemoveDisk();
#ifdef WIN32

#if !defined(SSE_TOS_PRG_AUTORUN_392)//eject not the right place

#ifdef SSE_TOS_PRG_AUTORUN2
  if(SF314[Drive].ImageType.Extension==EXT_PRG
    ||SF314[Drive].ImageType.Extension==EXT_TOS)
  {
    HardDiskMan.DisableHardDrives=true; // we suppose it's wanted
#if defined(SSE_TOS_PRG_AUTORUN_390) 
    stemdos_current_drive=0; // A:
#endif
#ifdef SSE_GUI_DM_HD_SELECTED // update GEMDOS HD icon
    HWND icon_handle=GetDlgItem(Handle,10);
    SendMessage(icon_handle,BM_SETCHECK,!HardDiskMan.DisableHardDrives,0);
#endif
  }
  SF314[Drive].ImageType.Extension=0;
#endif

#endif

  if (Handle){
    SendMessage(GetDlgItem(Handle,100+Drive),LVM_DELETEITEM,0,0);
    EnableWindow(GetDlgItem(GetDlgItem(Handle,98+Drive),100),AreNewDisksInHistory(Drive));
  }   
#if defined(SSE_GUI_STATUS_BAR_DISK_NAME)
  GUIRefreshStatusBar();
#endif

#elif defined(UNIX)
  UpdateDiskNames(Drive);
#endif//win32
}

void TDiskManager::ExtractDisks(Str Path)
{
  if (!enable_zip) return;

  EasyStringList sl;
  sl.Sort=eslNoSort;
  zippy.list_contents(Path,&sl,true);
  if (sl.NumStrings==0){
    Alert(EasyStr(T("Cannot find a disk image in the ZIP file"))+" "+Path,T("ZIP Error"),MB_ICONEXCLAMATION);
  }else{
    EasyStr SelPath="";
    for (int s=0;s<sl.NumStrings;s++){
      int Choice=IDYES;
      EasyStr DestPath=DisksFol+SLASH+GetFileNameFromPath(sl[s].String);
      if (Exists(DestPath)){
        Choice=Alert(Str(sl[s].String)+" "+T("already exists, do you want to overwrite it?"),T("Extract Disk?"),MB_ICONQUESTION | MB_YESNO);
      }
      if (Choice==IDYES){
        if (zippy.extract_file(Path,sl[s].Data[0],DestPath,0,sl[s].Data[1])==ZIPPY_FAIL){
          Alert(EasyStr(T("There was an error extracting"))+" "+sl[s].String+" "+T("from")+" "+Path,T("ZIP Error"),MB_ICONEXCLAMATION);
        }else{
          SelPath=DestPath;
        }
      }
    }
    if (SelPath.NotEmpty()) RefreshDiskView(SelPath);
  }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TDiskManager::GCGetCRC(char *Path,DWORD *lpCRC,int nCRCs)
{
  EasyStringList sl;
  zippy.list_contents(Path,&sl,true);
  for (int i=0;i<min(sl.NumStrings,nCRCs);i++){
    *(lpCRC++)=DWORD(sl[i].Data[2]);
  }
}
//---------------------------------------------------------------------------
BYTE* TDiskManager::GCConvertToST(char *Path,int Num,int *pLen)
{
  char file[MAX_PATH];
  char real_name[MAX_PATH];
  bool del=0;
  *pLen=0;
  if (FileIsDisk(Path)==DISK_COMPRESSED){
    EasyStringList sl;
    zippy.list_contents(Path,&sl,true);
    if (Num>=sl.NumStrings){
      *pLen=-1;
      return NULL;
    }

    GetTempFileName(WriteDir,"TMP",0,file);
    zippy.extract_file(Path,sl[Num].Data[0],file,true,0);
    strcpy(real_name,sl[Num].String);
    del=true;
  }else{
    if (Num>0){
      *pLen=-1;
      return NULL;
    }

    strcpy(file,Path);
    strcpy(real_name,Path);
  }
  char *ext=strrchr(real_name,'.');
  if (ext==NULL){
    if (del) DeleteFile(file);
    return NULL;
  }

  BYTE *mem=NULL;
  int len;
#ifdef SSE_DISK_EXT
  if (IsSameStr_I(ext,dot_ext(EXT_MSA))){
#else
  if (IsSameStr_I(ext,".MSA")){
#endif
    FILE *nf=fopen(file,"rb");
    if (nf==NULL){
      if (del) DeleteFile(file);
      return NULL;
    }

    WORD ID,MSA_SecsPerTrack,MSA_Sides,StartTrack,MSA_EndTrack;
    bool Err=0;

    // Read header
    fread(&ID,2,1,nf);               SWAPBYTES(ID);
    fread(&MSA_SecsPerTrack,2,1,nf); SWAPBYTES(MSA_SecsPerTrack);
    fread(&MSA_Sides,2,1,nf);        SWAPBYTES(MSA_Sides);
    fread(&StartTrack,2,1,nf);       SWAPBYTES(StartTrack);
    fread(&MSA_EndTrack,2,1,nf);     SWAPBYTES(MSA_EndTrack);

    if (MSA_SecsPerTrack<1 || MSA_SecsPerTrack>FLOPPY_MAX_SECTOR_NUM ||
        MSA_Sides>1 || StartTrack!=0 ||
        MSA_EndTrack<1 || MSA_EndTrack>FLOPPY_MAX_TRACK_NUM){
      Err=true;
    }

    if (Err==0){
      *pLen=(MSA_SecsPerTrack*512)*(MSA_EndTrack+1)*(MSA_Sides+1);
      mem=(BYTE*)malloc(*pLen+16);

      // Read data
      WORD Len,NumRepeats;
      BYTE *TrackData=new BYTE[(MSA_SecsPerTrack*512)+16];
      BYTE *pDat,*pEndDat,dat;
      BYTE *pSTBuf=mem;
      for (int n=0;n<=MSA_EndTrack;n++){
        for (int s=0;s<=MSA_Sides;s++){
          Len=0;
          fread(&Len,1,2,nf); SWAPBYTES(Len);
          if (Len>MSA_SecsPerTrack*512 || Len==0){
            Err=true;break;
          }
          if (WORD(fread(TrackData,1,Len,nf))<Len){
            Err=true;break;
          }
          if (Len==(MSA_SecsPerTrack*512)){
            memcpy(pSTBuf,TrackData,Len);
            pSTBuf+=Len;
          }else{
            // Convert compressed MSA format track in TrackData to ST format in STBuf
            BYTE *pSTBufEnd=pSTBuf+(MSA_SecsPerTrack*512);
            pDat=TrackData;
            pEndDat=TrackData+Len;
            while (pDat<pEndDat && pSTBuf<pSTBufEnd){
              dat=*(pDat++);
              if (dat==0xE5){
                dat=*(pDat++);
                NumRepeats=*LPWORD(pDat);pDat+=2;
                SWAPBYTES(NumRepeats);
                for (int s=0;s<NumRepeats && pSTBuf<pSTBufEnd;s++) *(pSTBuf++)=dat;
              }else{
                *(pSTBuf++)=dat;
              }
            }
          }
        }
        if (Err) break;
      }
      delete[] TrackData;
    }
    fclose(nf);
    if (Err){
      free(mem);
      mem=NULL;
      *pLen=0;
    }
#ifdef SSE_DISK_EXT
  }else if (IsSameStr_I(ext,dot_ext(EXT_DIM))){
#else
  }else if (IsSameStr_I(ext,".DIM")){
#endif
    FILE *f=fopen(file,"rb");
    if (f){
      len=GetFileLength(f)-32;
      mem=(BYTE*)malloc(len);
      fseek(f,32,SEEK_SET);
      fread(mem,1,len,f);
      fclose(f);
      *pLen=len;
    }
  }
  if (del) DeleteFile(file);
  return mem;
}
//---------------------------------------------------------------------------
void TDiskManager::InitGetContents()
{
  GetContents_GetZipCRCsProc=GCGetCRC;
  GetContents_ConvertToSTProc=GCConvertToST;
  strcpy(GetContents_ListFile,RunDir+SLASH+"disk image list.txt");
}
//---------------------------------------------------------------------------
bool TDiskManager::GetContentsCheckExist()
{
  if (Exists(GetContents_ListFile)) return true;

  int i=Alert(T("Steem cannot find the ST disk image database, would you like to open the disk image database website now?"),
                  T("Cannot Find Database"),MB_ICONQUESTION | MB_YESNO);
  if (i==IDYES){
    WIN_ONLY( ShellExecute(NULL,NULL,DIDATABASE_WEB,"","",SW_SHOWNORMAL); )
    UNIX_ONLY( shell_execute(Comlines[COMLINE_HTTP],Str("[URL]\n")+DIDATABASE_WEB); )
  }
  return 0;
}
//---------------------------------------------------------------------------
void TDiskManager::GetContentsSL(Str Path)
{
  contents_sl.DeleteAll();

  if (GetContentsCheckExist()==0) return;

  char buf[1024];

  int nLinks=GetContentsFromDiskImage(Path,buf,1024,GC_ONAMBIGUITY_GUESS);
  if (nLinks>0){
    contents_sl.Sort=eslNoSort;
    contents_sl.Add(Path);

    char *p=buf;

    for (int i=0;i<nLinks;i++){
      if (p[0]==0) break;
      contents_sl.Add(p);
      p+=strlen(p)+1;
    }
  }else{
    Alert(T("Sorry this disk image was not recognised"),T("Unrecognised Disk Image"),MB_ICONINFORMATION);
  }
}
//---------------------------------------------------------------------------
Str TDiskManager::GetContentsGetAppendName(Str TOSECName)
{
  Str FirstName=TOSECName;
  char *spc=strchr(FirstName,' ');
  if (spc) *spc=0;
  Str TOSEC=TOSECName;
  Str Letter;
  spc=strstr(TOSEC," of ");
  if (spc){
    while (strstr(spc+1," of ")) spc=strstr(spc+1," of "); // find last " of "
    *(spc--)=0;
    for (;spc>TOSEC.Text;spc--){
      if (spc[0]<'0' || spc[0]>'9'){
        *(spc++)=0;
        break;
      }
    }
    Letter=char('a'+atoi(spc)-1);
  }else{
    for (int i=0;i<10;i++){
      if (strstr(TOSEC,Str("Part ")+char('A'+i))==TOSEC.Right()-5) Letter=char('a'+i);
    }
  }
  Str Number;
  for (spc=TOSEC.Text+TOSEC.Length()-1;spc>TOSEC.Text;spc--){
    if (spc[0]>='0' && spc[0]<='9'){
      *(spc+1)=0;
      for (;spc>TOSEC.Text;spc--){
        if (spc[0]<'0' || spc[0]>'9'){
          Number=atoi(spc);
          break;
        }
      }
      break;
    }
  }

  if (FirstName=="Automation") FirstName="Auto";
  if (strstr(TOSEC,"Pompey Pirates")==TOSEC.Text) FirstName="PP";
  if (strstr(TOSEC,"Sewer Doc")==TOSEC.Text) FirstName="Sewer Doc";
  if (strstr(TOSEC,"Flame of Finland")==TOSEC.Text) FirstName="FOF";
  if (strstr(TOSEC,"Persistance of Vision")==TOSEC.Text) FirstName="POV";
  if (strstr(TOSEC,"ST Format")==TOSEC.Text) FirstName="STF";
  if (strstr(TOSEC,"Bad Brew Crew")==TOSEC.Text) FirstName="BBC";
  Str ShortName=FirstName;
  if (Number.NotEmpty()) ShortName+=Str(" ")+Number+Letter;
  return ShortName;
}
//---------------------------------------------------------------------------
#undef LOGSECTION //LOGSECTION_IMAGE_INFO
