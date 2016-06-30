/*---------------------------------------------------------------------------
FILE: dataloadsave.cpp
MODULE: Steem
DESCRIPTION: The code to load and save all Steem's options (and there are
a lot of them) to and from the ini file.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: dataloadsave.cpp")
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_DATALOADSAVE_H)
ProfileSectionData ProfileSection[20]=
      {{"Machine and TOS",PSEC_MACHINETOS},{"Ports and MIDI",PSEC_PORTS},
       {"General",PSEC_GENERAL},{"Display, Fullscreen, Brightness and Contrast",PSEC_DISPFULL},
       {"On Screen Display",PSEC_OSD},{"Steem Window Position and Size",PSEC_POSSIZE},
       {"Disk Emulation",PSEC_DISKEMU},{"Disk Manager",PSEC_DISKGUI},
       {"Hard Drives",PSEC_HARDDRIVES},{"Joysticks",PSEC_JOY},
#ifdef UNIX
       {"PC Joysticks",PSEC_PCJOY},
#endif
       {"Sound",PSEC_SOUND},{"Shortcuts",PSEC_CUT},{"Macros",PSEC_MACRO},
       {"Patches",PSEC_PATCH},{"Startup",PSEC_STARTUP},
       {"Auto Update and File Associations",PSEC_AUTOUP},
       {"Memory Snapshots",PSEC_SNAP},{"Paste Delay",PSEC_PASTE},
       {NULL,-1}};

Str ProfileSectionGetStrFromID(int ID)
{
  for (int i=0;;i++){
    if (ProfileSection[i].Name==NULL) break;
    if (ProfileSection[i].ID==ID) return ProfileSection[i].Name;
  }
  return "";
}
#endif

#if defined(STEVEN_SEAGAL) && defined(DEBUG_BUILD)
#include <boiler.decla.h> //sse_menu;
#endif


//---------------------------------------------------------------------------
void LoadAllDialogData(bool FirstLoad,Str INIFile,bool *SecDisabled,GoodConfigStoreFile *pCSF)
{
  bool Temp[200];
  if (SecDisabled==NULL){
    ZeroMemory(Temp,sizeof(Temp));
    SecDisabled=Temp;
  }

  bool DeleteCSF=0;
  if (pCSF==NULL){
    pCSF=new GoodConfigStoreFile(INIFile);
    DeleteCSF=true;
  }

  SEC(PSEC_SNAP){
    LastSnapShot=pCSF->GetStr("Main","LastSnapShot",WriteDir+SLASH+T("memory snapshots")+SLASH);
#if defined(STEVEN_SEAGAL) && defined(SSE_GUI_CONFIG_FILE)
    LastCfgFile=pCSF->GetStr("Main","LastCfgFile",WriteDir+SLASH+T("config")+SLASH);
#endif
    Str Dir=LastSnapShot;
    RemoveFileNameFromPath(Dir,REMOVE_SLASH);
    if (GetFileAttributes(Dir)==0xffffffff){
#ifndef ONEGAME
      LastSnapShot=WriteDir+SLASH+T("memory snapshots");
      CreateDirectory(LastSnapShot,NULL);
      LastSnapShot+=SLASH;
#else
      LastSnapShot=WriteDir+SLASH;
#endif
    }
    for (int n=0;n<10;n++){
      StateHist[n]=pCSF->GetStr("Main",Str("SnapShotHistory")+n,"");
    }
  }

  SEC(PSEC_PASTE){
    PasteSpeed=pCSF->GetInt("Main","PasteSpeed",PasteSpeed);
  }

  DiskMan.LoadData(FirstLoad,pCSF,SecDisabled);
  JoyConfig.LoadData(FirstLoad,pCSF,SecDisabled);
  OptionBox.LoadData(FirstLoad,pCSF,SecDisabled);
#if defined(SSE_VAR_HIDE_OPTIONS_AT_START)
  OptionBox.Hide();
#endif
  InfoBox.LoadData(FirstLoad,pCSF,SecDisabled);
  ShortcutBox.LoadData(FirstLoad,pCSF,SecDisabled);
  PatchesBox.LoadData(FirstLoad,pCSF,SecDisabled);

#ifndef ONEGAME
  SEC(PSEC_POSSIZE){
    bAOT=pCSF->GetInt("Main","AOT",0);
#ifdef WIN32
    CheckMenuItem(StemWin_SysMenu,102,MF_BYCOMMAND | int(bAOT ? MF_CHECKED:MF_UNCHECKED));
    if (FirstLoad==0) SetWindowPos(StemWin,HWND(bAOT ? HWND_TOPMOST:HWND_NOTOPMOST),0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    int Left=pCSF->GetInt("Main","Left",MSW_NOCHANGE);
    if (Left!=MSW_NOCHANGE) Left=max(min(Left,GetScreenWidth()-100),-100);
    int Top=pCSF->GetInt("Main","Top",MSW_NOCHANGE);
    if (Top!=MSW_NOCHANGE) Top=max(min(Top,GetScreenHeight()-100),-100);
    MoveStemWin(Left,Top,pCSF->GetInt("Main","Width",MSW_NOCHANGE),
                pCSF->GetInt("Main","Height",MSW_NOCHANGE));
#else
    Disp.GoToFullscreenOnRun=pCSF->GetInt("Main","GoToFullscreenOnRun",Disp.GoToFullscreenOnRun);
    FullScreenBut.set_check(Disp.GoToFullscreenOnRun);
#endif
  }

  int i=pCSF->GetInt("Display","ScreenShotUseFullName",99);
  if (i==0 || i==1) Disp.ScreenShotUseFullName=bool(i);
  i=pCSF->GetInt("Display","ScreenShotAlwaysAddNum",99);
  if (i==0 || i==1) Disp.ScreenShotAlwaysAddNum=bool(i);

#else
  OGLoadData(pCSF);
#endif

  if (DeleteCSF){
    pCSF->Close();
    if (DeleteCSF) delete pCSF;
  }

  PortsOpenAll();
}
//---------------------------------------------------------------------------
#ifndef ONEGAME
void SaveAllDialogData(bool FinalSave,Str INIFile,ConfigStoreFile *pCSF)
{
  bool DeleteCSF=0;
  if (pCSF==NULL){
    pCSF=new ConfigStoreFile(INIFile);
    DeleteCSF=true;
  }

  if (AutoSnapShotName.Empty()) AutoSnapShotName="auto";

  WINPOSITIONDATA wpd={0,0,0,0,0,0};
  GetWindowPositionData(StemWin,&wpd);
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
  if (border==3 && ResChangeResize){  //Auto border on and border on
    int xmult=WinSize[screen_res][WinSizeForRes[screen_res]].x/WinSize[screen_res][0].x;
    int ymult=WinSize[screen_res][WinSizeForRes[screen_res]].y/WinSize[screen_res][0].y;

    wpd.Left+=BORDER_SIDE*xmult;
    wpd.Width-=BORDER_SIDE*xmult*2;

    wpd.Top+=BORDER_TOP*ymult;
    wpd.Height-=BORDER_BOTTOM*ymult + BORDER_TOP*ymult;
  }
#endif
  pCSF->SetStr("Main","Left",EasyStr(wpd.Left));
  pCSF->SetStr("Main","Top",EasyStr(wpd.Top));
  pCSF->SetStr("Main","Width",EasyStr(wpd.Width));
  pCSF->SetStr("Main","Height",EasyStr(wpd.Height));
  pCSF->SetStr("Main","Maximized",LPSTR(wpd.Maximized ? "1":"0"));
  pCSF->SetStr("Main","AOT",LPSTR(bAOT ? "1":"0"));
  UNIX_ONLY( pCSF->SetInt("Main","GoToFullscreenOnRun",Disp.GoToFullscreenOnRun); )

  pCSF->SetStr("Main","LastSnapShot",LastSnapShot);
  for (int n=0;n<10;n++){
    pCSF->SetStr("Main",Str("SnapShotHistory")+n,StateHist[n]);
  }
#if defined(STEVEN_SEAGAL) && defined(SSE_GUI_CONFIG_FILE)
  pCSF->SetStr("Main","LastCfgFile",LastCfgFile);
#endif
  pCSF->SetInt("Main","PasteSpeed",PasteSpeed);

  int i=pCSF->GetInt("Display","ScreenShotUseFullName",999);
  if (i==999) pCSF->SetInt("Display","ScreenShotUseFullName",99);
  i=pCSF->GetInt("Display","ScreenShotAlwaysAddNum",999);
  if (i==999) pCSF->SetInt("Display","ScreenShotAlwaysAddNum",99);

  DiskMan.SaveData(FinalSave,pCSF);
  JoyConfig.SaveData(FinalSave,pCSF);
  OptionBox.SaveData(FinalSave,pCSF);
  InfoBox.SaveData(FinalSave,pCSF);
  ShortcutBox.SaveData(FinalSave,pCSF);
  PatchesBox.SaveData(FinalSave,pCSF);

  if (DeleteCSF){
    pCSF->Close();
    delete pCSF;
  }
}
#else
void SaveAllDialogData(bool,Str,ConfigStoreFile*)
{
}
#endif
//---------------------------------------------------------------------------
bool TDiskManager::LoadData(bool FirstLoad,GoodConfigStoreFile *pCSF,bool *SecDisabled)
{
#if USE_PASTI
  if (hPasti){
    EasyStringList sl(eslNoSort);
    pCSF->GetWholeSect(&sl,"Pasti",0);
    char Buf[8192],*p=Buf;
    ZeroMemory(Buf,sizeof(Buf));
    for (int i=0;i<sl.NumStrings;i++){
      strcpy(p,sl[i].String); // Key Name
      p+=strlen(p)+1;
      strcpy(p,pCSF->GetStr("Pasti",sl[i].String,"LOAD ERROR")); // Value 
      p+=strlen(p)+1;
    }

    pastiLOADINI pli;
    pli.mode=PASTI_LCSTRINGS;
    pli.name=NULL;
    pli.buffer=Buf;
    pli.bufSize=8192;
    pasti->LoadConfig(&pli,NULL);
  }
#endif

  SEC(PSEC_DISKEMU){
#if USE_PASTI
//    pasti_use_all_possible_disks=pCSF->GetInt("Disks","PastiUseAllDisks",pasti_use_all_possible_disks);
    pasti_active=pCSF->GetInt("Disks","PastiActive",pasti_active);
    if (hPasti==NULL) pasti_active=false;
#endif

    SetNumFloppies(pCSF->GetInt("Disks","NumFloppyDrives",num_connected_floppies));
    floppy_instant_sector_access=pCSF->GetInt("Disks","QuickDiskAccess",floppy_instant_sector_access);
    FloppyArchiveIsReadWrite=bool(pCSF->GetInt("Disks","FloppyArchiveIsReadWrite",FloppyArchiveIsReadWrite));
    if (BootDisk[0].Empty() || FirstLoad==0){
#if defined(STEVEN_SEAGAL) &&  defined(SSE_VAR_DONT_INSERT_NON_EXISTENT_IMAGES)
      if( (pCSF->GetStr("Disks","Disk_A_Name","")).NotEmpty() )
#endif
      InsertDisk(0,pCSF->GetStr("Disks","Disk_A_Name",""),pCSF->GetStr("Disks","Disk_A_Path",""),
                    0,0,pCSF->GetStr("Disks","Disk_A_DiskInZip",""),true);
    }
    if (BootDisk[1].Empty() || FirstLoad==0){
#if defined(STEVEN_SEAGAL) &&  defined(SSE_VAR_DONT_INSERT_NON_EXISTENT_IMAGES)
      if( (pCSF->GetStr("Disks","Disk_B_Name","")).NotEmpty() )
#endif
      InsertDisk(1,pCSF->GetStr("Disks","Disk_B_Name",""),pCSF->GetStr("Disks","Disk_B_Path",""),
                    0,0,pCSF->GetStr("Disks","Disk_B_DiskInZip",""),true);
    }
  }

  SEC(PSEC_DISKGUI){
    Width=pCSF->GetInt("Disks","Width",Width);
    Height=pCSF->GetInt("Disks","Height",Height);
    Maximized=pCSF->GetInt("Disks","Maximized",0);
    FSWidth=pCSF->GetInt("Disks","FSWidth",FSWidth);
    FSHeight=pCSF->GetInt("Disks","FSHeight",FSHeight);
    FSMaximized=pCSF->GetInt("Disks","FSMaximized",0);

    HomeFol=pCSF->GetStr("Disks","HomeFolder",HomeFol);
    NO_SLASH(HomeFol);
    DisksFol=pCSF->GetStr("Disks","CurrentFolder",DisksFol);
    NO_SLASH(DisksFol);

    if (WIN_ONLY(HomeFol.Empty() || ) HomeFol.Lefts(2)=="//"){
      HomeFol=RunDir;
    }else if (HomeFol.NotEmpty()){
      DWORD Attrib=GetFileAttributes(HomeFol);
      if ((Attrib & FILE_ATTRIBUTE_DIRECTORY)==0 || Attrib==0xffffffff) HomeFol=RunDir;
    }
    if (WIN_ONLY(DisksFol.Empty() || ) DisksFol.Lefts(2)=="//"){
      DisksFol=RunDir;
    }else if (DisksFol.NotEmpty()){
      DWORD Attrib=GetFileAttributes(DisksFol);
      if ((Attrib & FILE_ATTRIBUTE_DIRECTORY)==0 || Attrib==0xffffffff) DisksFol=HomeFol;
    }
    for (int n=0;n<10;n++){
      QuickFol[n]=pCSF->GetStr("Disks",EasyStr("QuickFol")+n,QuickFol[n]);
    }
    for (int d=0;d<2;d++){
      for (int n=0;n<10;n++){
        InsertHist[d][n].Name=pCSF->GetStr("Disks",EasyStr("InsertHistoryName")+d+n,InsertHist[d][n].Name);
        InsertHist[d][n].Path=pCSF->GetStr("Disks",EasyStr("InsertHistoryPath")+d+n,InsertHist[d][n].Path);
        InsertHist[d][n].DiskInZip=pCSF->GetStr("Disks",EasyStr("InsertHistoryDiskInZip")+d+n,InsertHist[d][n].DiskInZip);
      }
      if (BootDisk[d].NotEmpty() && FirstLoad) InsertHistoryAdd(d,FloppyDrive[d].DiskName,FloppyDrive[d].GetDisk(),"");
    }

    BytesPerSectorIdx=(WORD)pCSF->GetInt("Disks","BytesPerSectorIdx",BytesPerSectorIdx);
    SecsPerTrackIdx=(WORD)pCSF->GetInt("Disks","SecsPerTrackIdx",SecsPerTrackIdx);
    TracksIdx=(WORD)pCSF->GetInt("Disks","TracksIdx",TracksIdx);
    SidesIdx=(WORD)pCSF->GetInt("Disks","SidesIdx",SidesIdx);

    HideBroken=pCSF->GetInt("Disks","HideBroken",HideBroken);

#ifdef WIN32
    ExplorerFolders=pCSF->GetInt("Disks","ExplorerFolders",ExplorerFolders);
#ifndef SSE_VAR_NO_WINSTON
    WinSTonPath=pCSF->GetStr("Disks","WinSTonPath",WinSTonPath);
    WinSTonDiskPath=pCSF->GetStr("Disks","WinSTonDiskPath",WinSTonDiskPath);
    ImportPath=pCSF->GetStr("Disks","ImportPath",ImportPath);
    ImportOnlyIfExist=(bool)pCSF->GetInt("Disks","ImportOnlyIfExist",ImportOnlyIfExist);
    ImportConflictAction=pCSF->GetInt("Disks","ImportConflictAction",ImportConflictAction);
#endif
    MSAConvPath=pCSF->GetStr("Disks","MSAConvPath",MSAConvPath);

    SmallIcons=pCSF->GetInt("Disks","SmallIcons",SmallIcons);
    IconSpacing=pCSF->GetInt("Disks","IconSpacing",IconSpacing);

#endif
    EjectDisksWhenQuit=pCSF->GetInt("Disks","EjectDisksWhenQuit",EjectDisksWhenQuit);

    DoubleClickAction=pCSF->GetInt("Disks","DoubleClickAction",DoubleClickAction);
    CloseAfterIRR=pCSF->GetInt("Disks","CloseAfterIRR",CloseAfterIRR);

    ContentListsFol=pCSF->GetStr("Disks","ContentListsFol",RunDir+SLASH+"contents");

    UPDATE;
  }

#if defined(SSE_GUI_DISK_MANAGER_INSERT_DISKB_LS)
  AutoInsert2=pCSF->GetInt("Disks","AutoInsert2",AutoInsert2);
#endif

  HardDiskMan.LoadData(FirstLoad,pCSF,SecDisabled);
#if defined(SSE_ACSI_HDMAN)
  AcsiHardDiskMan.LoadData(FirstLoad,pCSF,SecDisabled);
#endif

  return true;
}
//---------------------------------------------------------------------------
bool TDiskManager::SaveData(bool FinalSave,ConfigStoreFile *pCSF)
{
#if USE_PASTI
  if (hPasti){
    pCSF->DeleteSection("Pasti");
    char Buf[8192],*p=Buf;
    ZeroMemory(Buf,sizeof(Buf));
    pastiLOADINI pli;
    pli.mode=PASTI_LCSTRINGS;
    pli.name=NULL;
    pli.buffer=Buf;
    pli.bufSize=8192;
    pasti->SaveConfig(&pli,NULL);
    while (p[0]){
      char *val=p+strlen(p)+1;
      pCSF->SetStr("Pasti",p,val);
      p=val+strlen(val)+1;
    }
  }
#endif

  SavePosition(FinalSave,pCSF);
  pCSF->SetStr("Disks","Width",EasyStr(Width));
  pCSF->SetStr("Disks","Height",EasyStr(Height));
  pCSF->SetStr("Disks","Maximized",LPSTR(Maximized ? "1":"0"));
  pCSF->SetStr("Disks","FSWidth",EasyStr(FSWidth));
  pCSF->SetStr("Disks","FSHeight",EasyStr(FSHeight));
  pCSF->SetStr("Disks","FSMaximized",LPSTR(FSMaximized ? "1":"0"));

  pCSF->SetStr("Disks","CurrentFolder",DisksFol);
  pCSF->SetStr("Disks","HomeFolder",HomeFol);

  EasyStr Path[2];
  Path[0]=FloppyDrive[0].GetDisk();
  Path[1]=FloppyDrive[1].GetDisk();
  if (EjectDisksWhenQuit && FinalSave){Path[0]="";Path[1]="";}

  pCSF->SetStr("Disks","Disk_A_Path",Path[0]);
  pCSF->SetStr("Disks","Disk_A_Name",FloppyDrive[0].DiskName);
  pCSF->SetStr("Disks","Disk_A_DiskInZip",FloppyDrive[0].DiskInZip);

  pCSF->SetStr("Disks","Disk_B_Path",Path[1]);
  pCSF->SetStr("Disks","Disk_B_Name",FloppyDrive[1].DiskName);
  pCSF->SetStr("Disks","Disk_B_DiskInZip",FloppyDrive[1].DiskInZip);

  if (FinalSave){
#if defined(SSE_PASTI_ONLY_STX)
    {//scope
    bool pasti_active_save=pasti_active; //3.6.1, because RemoveDisk clears it
#endif
    FloppyDrive[0].RemoveDisk();
    FloppyDrive[1].RemoveDisk();
#if defined(SSE_PASTI_ONLY_STX)
    pasti_active=pasti_active_save; //we want correct state saved
    }
#endif
  }

  for (int n=0;n<10;n++){
    pCSF->SetStr("Disks",EasyStr("QuickFol")+n,QuickFol[n]);
  }
  for (int d=0;d<2;d++){
    for (int n=0;n<10;n++){
      pCSF->SetStr("Disks",EasyStr("InsertHistoryName")+d+n,InsertHist[d][n].Name);
      pCSF->SetStr("Disks",EasyStr("InsertHistoryPath")+d+n,InsertHist[d][n].Path);
      pCSF->SetStr("Disks",EasyStr("InsertHistoryDiskInZip")+d+n,InsertHist[d][n].DiskInZip);
    }
  }

  pCSF->SetStr("Disks","BytesPerSectorIdx",EasyStr(BytesPerSectorIdx));
  pCSF->SetStr("Disks","SecsPerTrackIdx",EasyStr(SecsPerTrackIdx));
  pCSF->SetStr("Disks","TracksIdx",EasyStr(TracksIdx));
  pCSF->SetStr("Disks","SidesIdx",EasyStr(SidesIdx));

#ifdef WIN32
  pCSF->SetStr("Disks","ExplorerFolders",LPSTR(ExplorerFolders ? "1":"0"));
#ifndef SSE_VAR_NO_WINSTON
  pCSF->SetStr("Disks","WinSTonPath",EasyStr(WinSTonPath));
  pCSF->SetStr("Disks","WinSTonDiskPath",EasyStr(WinSTonDiskPath));
  pCSF->SetStr("Disks","ImportPath",EasyStr(ImportPath));
  pCSF->SetStr("Disks","ImportOnlyIfExist",LPSTR(ImportOnlyIfExist ? "1":"0"));
  pCSF->SetStr("Disks","ImportConflictAction",EasyStr(ImportConflictAction));
#endif
  pCSF->SetStr("Disks","MSAConvPath",MSAConvPath);

  pCSF->SetStr("Disks","SmallIcons",LPSTR(SmallIcons ? "1":"0"));
  pCSF->SetInt("Disks","IconSpacing",IconSpacing);

#endif

#if USE_PASTI
//  pCSF->SetInt("Disks","PastiUseAllDisks",pasti_use_all_possible_disks);
  pCSF->SetInt("Disks","PastiActive",pasti_active);
#endif

  pCSF->SetStr("Disks","HideBroken",LPSTR(HideBroken ? "1":"0"));
  pCSF->SetStr("Disks","EjectDisksWhenQuit",LPSTR(EjectDisksWhenQuit ? "1":"0"));

  pCSF->SetStr("Disks","DoubleClickAction",Str(DoubleClickAction));
  pCSF->SetInt("Disks","CloseAfterIRR",CloseAfterIRR);

  pCSF->SetInt("Disks","NumFloppyDrives",num_connected_floppies);

  pCSF->SetInt("Disks","QuickDiskAccess",floppy_instant_sector_access);

  pCSF->SetInt("Disks","FloppyArchiveIsReadWrite",FloppyArchiveIsReadWrite);

#if defined(SSE_GUI_DISK_MANAGER_INSERT_DISKB_LS)
  pCSF->SetInt("Disks","AutoInsert2",AutoInsert2);
#endif

  HardDiskMan.SaveData(FinalSave,pCSF);
#if defined(SSE_ACSI_HDMAN)
  AcsiHardDiskMan.SaveData(FinalSave,pCSF);
#endif

  return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool THardDiskManager::LoadData(bool FirstLoad,GoodConfigStoreFile *pCSF,bool *SecDisabled)
{
  SEC(PSEC_HARDDRIVES){
    if (nDrives==0 || FirstLoad==0){  // In case of SteemIntro
      EasyStr Str;
      for (nDrives=0;nDrives<MAX_HARDDRIVES;nDrives++){
        Str=pCSF->GetStr("HardDrives",EasyStr("Drive_")+nDrives+"_Path","NOT ASSIGNED");
        if (Str=="NOT ASSIGNED") break;

        NO_SLASH(Str);
        Drive[nDrives].Path=Str;
        Str=pCSF->GetStr("HardDrives",EasyStr("Drive_")+nDrives+"_Letter",EasyStr(char('C'+nDrives)));
        Drive[nDrives].Letter=Str[0];
      }
    }

#ifndef DISABLE_STEMDOS
    stemdos_boot_drive=pCSF->GetInt("HardDrives","BootDrive",stemdos_boot_drive);
#endif
    DisableHardDrives=pCSF->GetInt("HardDrives","DisableHardDrives",DisableHardDrives);
//#ifdef SSE_GUI_DISK_MANAGER_HD_SELECTED
#if defined(SSE_GUI_DISK_MANAGER_HD_SELECTED) && defined(WIN32)//ux382
    SendMessage(GetDlgItem(DiskMan.Handle,10),BM_SETCHECK,!HardDiskMan.DisableHardDrives,0);
#endif
    update_mount();
    UPDATE;
  }

  return true;
}
//---------------------------------------------------------------------------
bool THardDiskManager::SaveData(bool FinalSave,ConfigStoreFile *pCSF)
{
  SavePosition(FinalSave,pCSF);

  for (int n=0;n<MAX_HARDDRIVES;n++){
    if (n<nDrives){
      pCSF->SetStr("HardDrives",EasyStr("Drive_")+n+"_Letter",EasyStr(Drive[n].Letter));
      pCSF->SetStr("HardDrives",EasyStr("Drive_")+n+"_Path",Drive[n].Path);
    }else{
      pCSF->SetStr("HardDrives",EasyStr("Drive_")+n+"_Letter","NOT ASSIGNED");
      pCSF->SetStr("HardDrives",EasyStr("Drive_")+n+"_Path","NOT ASSIGNED");
    }
  }
#ifndef DISABLE_STEMDOS
  pCSF->SetStr("HardDrives","BootDrive",EasyStr(stemdos_boot_drive));
#endif
  pCSF->SetInt("HardDrives","DisableHardDrives",DisableHardDrives);

  return true;
}

#if defined(SSE_ACSI_HDMAN)

bool TAcsiHardDiskManager::LoadData(bool FirstLoad,GoodConfigStoreFile *pCSF,bool *SecDisabled) {

  SEC(PSEC_HARDDRIVES){//what is this?
    if (nDrives==0 || FirstLoad==0){  // In case of SteemIntro
      EasyStr Str;
      for (nDrives=0;nDrives<TAcsiHdc::MAX_ACSI_DEVICES;nDrives++){
        Str=pCSF->GetStr("HardDrives",EasyStr("AcsiDrive_")+nDrives+"_Path","NOT ASSIGNED");
        if (Str=="NOT ASSIGNED") break;

        NO_SLASH(Str);
        Drive[nDrives].Path=Str;
        Str=pCSF->GetStr("HardDrives",EasyStr("AcsiDrive_")+nDrives+"_Letter",EasyStr(char('C'+nDrives)));
        Drive[nDrives].Letter=Str[0];

        if(AcsiHdc[nDrives].Init(nDrives,Drive[nDrives].Path))
        {
          SSEConfig.AcsiImg=true;
        }
      }
    }
    UPDATE;//what is this?
#ifdef SSE_GUI_DISK_MANAGER_HD_SELECTED
    SendMessage(GetDlgItem(DiskMan.Handle,11),BM_SETCHECK,SSEConfig.AcsiImg,0);
#endif
  }

  return true;
}


bool TAcsiHardDiskManager::SaveData(bool FinalSave,ConfigStoreFile *pCSF) {
  SavePosition(FinalSave,pCSF);//what is this?

  for (int n=0;n<TAcsiHdc::MAX_ACSI_DEVICES;n++){
    if (n<nDrives){
      pCSF->SetStr("HardDrives",EasyStr("AcsiDrive_")+n+"_Letter",EasyStr(Drive[n].Letter));
      pCSF->SetStr("HardDrives",EasyStr("AcsiDrive_")+n+"_Path",Drive[n].Path);
    }else{
      pCSF->SetStr("HardDrives",EasyStr("AcsiDrive_")+n+"_Letter","NOT ASSIGNED");
      pCSF->SetStr("HardDrives",EasyStr("AcsiDrive_")+n+"_Path","NOT ASSIGNED");
    }
  }

  return true;
}

#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TJoystickConfig::LoadData(bool FirstLoad,GoodConfigStoreFile *pCSF,bool *SecDisabled)
{
#ifdef UNIX
  SEC(PSEC_PCJOY){
    // Have to load calibration of joysticks
    for (int j=0;j<MAX_PC_JOYS;j++){
      EasyStr Sect=EasyStr("PCJoystick ")+(j+1);

      JoyInfo[j].On=bool(pCSF->GetInt(Sect,"On",JoyInfo[j].On));
      JoyInfo[j].DeviceFile=pCSF->GetStr(Sect,"DeviceFile",JoyInfo[j].DeviceFile);
      JoyInfo[j].NumButtons=pCSF->GetInt(Sect,"NumButtons",JoyInfo[j].NumButtons);
      for (int a=0;a<6;a++){
        JoyInfo[j].AxisMin[a]=pCSF->GetInt(Sect,Str("Axis_")+a+"_Min",JoyInfo[j].AxisMin[a]);
        JoyInfo[j].AxisMax[a]=pCSF->GetInt(Sect,Str("Axis_")+a+"_Max",JoyInfo[j].AxisMax[a]);
        JoyInfo[j].AxisLen[a]=(JoyInfo[j].AxisMax[a]-JoyInfo[j].AxisMin[a]);
        JoyInfo[j].AxisMid[a]=JoyInfo[j].AxisMin[a]+JoyInfo[j].AxisLen[a]/2;
        JoyInfo[j].AxisExists[a]=pCSF->GetInt(Sect,Str("Axis_")+a+"_Exists",JoyInfo[j].AxisExists[a]);
        JoyInfo[j].AxisDZ[a]=pCSF->GetInt(Sect,Str("Axis_")+a+"_DZ",JoyInfo[j].AxisDZ[a]);
        JoyInfo[j].Range=pCSF->GetInt(Sect,Str("Axis_")+a+"_Range",JoyInfo[j].Range);
      }
      JoyPosReset(j); // Sets all to mid
    }
    PCJoyEdit=pCSF->GetInt("Joysticks","PCJoyEdit",PCJoyEdit);
  }
#endif

  SEC(PSEC_JOY){
#ifdef WIN32
    int Method=pCSF->GetInt("Joysticks","JoyReadMethod",JoyReadMethod);
    if (FirstLoad && DisablePCJoysticks) Method=PCJOY_READ_DONT;
    if (FirstLoad || Method!=JoyReadMethod) InitJoysticks(Method);
#else
    if (FirstLoad){
      InitJoysticks(int(DisablePCJoysticks ? PCJOY_READ_DONT:PCJOY_READ_KERNELDRIVER));
    }
#endif

    // Default ST joysticks
    int DefJoy[8]={2,2,2,2,2,2,2,2}; // 2=use cursor keys
#ifndef UNIX
    if (NumJoysticks){
      DefJoy[1]=0; // Use joystick 1 for stick 1
      JoySetup[0][1].ToggleKey=1;
      JoySetup[0][0].ToggleKey=0;
#if defined(ONEGAME)==0
      if (NumJoysticks>1){
        JoySetup[0][0].ToggleKey=1;
        DefJoy[0]=1; // Use joystick 2
      }else{
        JoySetup[0][0].ToggleKey=VK_SCROLL;
      }
#endif
    }else
#endif
    {
      DefJoy[0]=3; // 3=use A, W, S, Z and shift
#if defined(ONEGAME)
      JoySetup[0][1].ToggleKey=1;
      JoySetup[0][0].ToggleKey=0;
#elif defined(WIN32)
      JoySetup[0][1].ToggleKey=VK_SCROLL;
      JoySetup[0][0].ToggleKey=VK_SCROLL;
#else
      JoySetup[0][1].ToggleKey=0;
      JoySetup[0][0].ToggleKey=0;
#endif
    }
    for (int n=0;n<8;n++) SetJoyToDefaults(n,DefJoy[n]);

    UNIX_ONLY( ConfigST=bool(pCSF->GetInt("Joysticks","ConfigST",ConfigST)); )

    for (int Setup=0;Setup<3;Setup++){
      for (int n=0;n<8;n++){
        EasyStr Sect=EasyStr("Joystick ")+(n+1);
        EasyStr Prefix;
        if (Setup) Prefix=Str(Setup)+"_";

        JoySetup[Setup][n].Type=pCSF->GetInt(Sect,Prefix+"Type",JoySetup[Setup][n].Type);

        JoySetup[Setup][n].ToggleKey=pCSF->GetInt(Sect,Prefix+"ToggleKey",JoySetup[Setup][n].ToggleKey);
        JoySetup[Setup][n].AnyFireOnJoy=pCSF->GetInt(Sect,Prefix+"AnyFireOnJoy",JoySetup[Setup][n].AnyFireOnJoy);
        JoySetup[Setup][n].DeadZone=pCSF->GetInt(Sect,Prefix+"DeadZone",JoySetup[Setup][n].DeadZone);
        JoySetup[Setup][n].AutoFireSpeed=pCSF->GetInt(Sect,Prefix+"AutoFireSpeed",JoySetup[Setup][n].AutoFireSpeed);
#if defined(SSE_JOYSTICK_JUMP_BUTTON)
        for (int i=0;i<7;i++){
#else
        for (int i=0;i<6;i++){
#endif
          JoySetup[Setup][n].DirID[i]=pCSF->GetInt(Sect,Prefix+"DirID"+i,JoySetup[Setup][n].DirID[i]);
        }
        if (n==2 || n==4){
          for (int i=0;i<17;i++){
            JoySetup[Setup][n].JagDirID[i]=pCSF->GetInt(Sect,Prefix+"JagDirID"+i,JoySetup[Setup][n].JagDirID[i]);
          }
        }
      }
    }
    nJoySetup=pCSF->GetInt("Joysticks","Setup",nJoySetup);
    for (int n=0;n<8;n++) Joy[n]=JoySetup[nJoySetup][n];

    CreateJoyAnyButtonMasks();

    BasePort=pCSF->GetInt("Joysticks","BasePort",BasePort);

    mouse_speed=pCSF->GetInt("Joysticks","MouseSpeed",mouse_speed);

    UPDATE;
  }

  return true;
}
//---------------------------------------------------------------------------
bool TJoystickConfig::SaveData(bool FinalSave,ConfigStoreFile *pCSF)
{
  SavePosition(FinalSave,pCSF);

  for (int n=0;n<8;n++) JoySetup[nJoySetup][n]=Joy[n];

  pCSF->SetInt("Joysticks","JoyReadMethod",JoyReadMethod);

#ifdef UNIX
  for (int j=0;j<MAX_PC_JOYS;j++){
    EasyStr Sect=EasyStr("PCJoystick ")+(j+1);

    pCSF->SetInt(Sect,"On",JoyInfo[j].On);
    pCSF->SetStr(Sect,"DeviceFile",JoyInfo[j].DeviceFile);
    pCSF->SetInt(Sect,"NumButtons",JoyInfo[j].NumButtons);
    for (int a=0;a<6;a++){
      pCSF->SetInt(Sect,Str("Axis_")+a+"_Min",JoyInfo[j].AxisMin[a]);
      pCSF->SetInt(Sect,Str("Axis_")+a+"_Max",JoyInfo[j].AxisMax[a]);
      pCSF->SetInt(Sect,Str("Axis_")+a+"_Exists",JoyInfo[j].AxisExists[a]);
      pCSF->SetInt(Sect,Str("Axis_")+a+"_DZ",JoyInfo[j].AxisDZ[a]);
      pCSF->SetInt(Sect,Str("Axis_")+a+"_Range",JoyInfo[j].Range);
    }
  }
  pCSF->SetInt("Joysticks","ConfigST",ConfigST);
  pCSF->SetInt("Joysticks","PCJoyEdit",PCJoyEdit);
#endif

  for (int Setup=0;Setup<3;Setup++){
    for (int n=0;n<8;n++){
      EasyStr Sect="Joystick ";
      Sect+=(n+1);
      EasyStr Prefix;
      if (Setup) Prefix=Str(Setup)+"_";

      pCSF->SetStr(Sect,Prefix+"ToggleKey",EasyStr(JoySetup[Setup][n].ToggleKey));
      pCSF->SetStr(Sect,Prefix+"AnyFireOnJoy",EasyStr(JoySetup[Setup][n].AnyFireOnJoy));
      pCSF->SetStr(Sect,Prefix+"DeadZone",EasyStr(JoySetup[Setup][n].DeadZone));
      pCSF->SetStr(Sect,Prefix+"AutoFireSpeed",EasyStr(JoySetup[Setup][n].AutoFireSpeed));
      pCSF->SetStr(Sect,Prefix+"Type",EasyStr(JoySetup[Setup][n].Type));
#if defined(SSE_JOYSTICK_JUMP_BUTTON)
      for (int i=0;i<7;i++){
#else
      for (int i=0;i<6;i++){
#endif
        pCSF->SetStr(Sect,Prefix+"DirID"+i,EasyStr(JoySetup[Setup][n].DirID[i]));
      }
      if (n==2 || n==4){
        for (int i=0;i<17;i++){
          pCSF->SetStr(Sect,Prefix+"JagDirID"+i,EasyStr(JoySetup[Setup][n].JagDirID[i]));
        }
      }
    }
  }
  pCSF->SetInt("Joysticks","Setup",nJoySetup);

  pCSF->SetInt("Joysticks","BasePort",BasePort);

  pCSF->SetStr(Section,"MouseSpeed",EasyStr(mouse_speed));

  return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TOptionBox::LoadData(bool FirstLoad,GoodConfigStoreFile *pCSF,bool *SecDisabled)
{
  SEC(PSEC_MACHINETOS){
#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO)
#if defined(SSE_CPU_4GHZ)
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),4096000000),(int)CpuNormalHz);
#elif defined(SSE_CPU_3GHZ)
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),3072000000),(int)CpuNormalHz);
#elif defined(SSE_CPU_2GHZ)
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),2048000000),(int)CpuNormalHz);
#elif defined(SSE_CPU_1GHZ)
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),1024000000),(int)CpuNormalHz);
#elif defined(SSE_CPU_512MHZ)
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),512000000),(int)CpuNormalHz);
#elif defined(SSE_CPU_256MHZ)
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),256000000),(int)CpuNormalHz);
#else
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),128000000),(int)CpuNormalHz);
#endif
#else
    n_cpu_cycles_per_second=max(min(pCSF->GetInt("Options","CPUBoost",n_cpu_cycles_per_second),128000000),8000000);
#endif
    prepare_cpu_boosted_event_plans();
    ShowTips=(bool)pCSF->GetInt("Options","ShowToolTips",ShowTips);
    TOSBrowseDir=pCSF->GetStr("Machine","ROM_Add_Dir",RunDir);
    Str LastCartFol=pCSF->GetStr("Machine","Cart_Dir","");
    if (LastCartFol.NotEmpty()){
      LastCartFile=LastCartFol+SLASH;
      pCSF->SetStr("Machine","Cart_Dir","");
    }
    LastCartFile=pCSF->GetStr("Machine","LastCartFile",LastCartFile);
    if (LastCartFile.Empty()) LastCartFile=RunDir+SLASH;

    if (FirstLoad){
      bool ColourMonitor=pCSF->GetInt("Machine","Colour_Monitor",1);
      if (ColourMonitor){
        mfp_gpip_no_interrupt|=MFP_GPIP_COLOUR;
      }else{
        mfp_gpip_no_interrupt &= MFP_GPIP_NOT_COLOUR;
      }
      extended_monitor=pCSF->GetInt("Machine","ExMon",extended_monitor);
      em_width=pCSF->GetInt("Machine","ExMonWidth",em_width);
      em_height=pCSF->GetInt("Machine","ExMonHeight",em_height);
      em_planes=pCSF->GetInt("Machine","ExMonPlanes",em_planes);
      if (extended_monitor) 
        Disp.ScreenChange();
    }

    NewMemConf0=int(pCSF->GetInt("Options","NewMemConf0",NewMemConf0));
    NewMemConf1=int(pCSF->GetInt("Options","NewMemConf1",NewMemConf1));
    NewMonitorSel=int(pCSF->GetInt("Options","NewMonitorSel",NewMonitorSel));
    NewROMFile=pCSF->GetStr("Options","NewROMFile",NewROMFile);

    eslTOS_Descend=(bool)pCSF->GetInt("Options","TOSSortDescend",eslTOS_Descend);
    eslTOS_Sort=(ESLSortEnum)pCSF->GetInt("Options","TOSSort",(int)eslTOS_Sort);

    EnableShiftSwitching=pCSF->GetInt("Machine","ShiftSwitching",0);
    if (KeyboardLangID==0){
      WIN_ONLY( KeyboardLangID=GetUserDefaultLangID(); )
      UNIX_ONLY( KeyboardLangID=MAKELONG(LANG_ENGLISH,SUBLANG_ENGLISH_UK); )
    }
    KeyboardLangID=(LANGID)pCSF->GetInt("Machine","KeyboardLanguage",KeyboardLangID);
    InitKeyTable();
	
    if (FirstLoad) CheckResetIcon();

#if defined(STEVEN_SEAGAL) && defined(SSE_STF)
    OptionBox.Hide(); // hack
    ST_TYPE=pCSF->GetInt("Machine","STType",ST_TYPE);
    SwitchSTType(ST_TYPE); // settings for STF or STE
#endif
#if defined(SSE_INT_MFP_RATIO_OPTION2)
    OPTION_CPU_CLOCK=pCSF->GetInt("Option","FinetuneCPUclock",OPTION_CPU_CLOCK);
    CpuCustomHz=pCSF->GetInt("Machine","CpuCustomHz",CpuCustomHz);
#endif

  }

  SEC(PSEC_GENERAL){
    WIN_ONLY( AllowTaskSwitch=(bool)pCSF->GetInt("Options","AllowTaskSwitch",AllowTaskSwitch); )
    PauseWhenInactive=(bool)pCSF->GetInt("Options","PauseWhenInactive",PauseWhenInactive);
    floppy_access_ff=(bool)pCSF->GetInt("Options","DiskAccessFF",floppy_access_ff);
    slow_motion_speed=pCSF->GetInt("Options","SlowMotionSpeed",slow_motion_speed);
    fast_forward_max_speed=pCSF->GetInt("Options","MaxFastForward",fast_forward_max_speed);
    HighPriority=pCSF->GetInt("Options","HighPriority",HighPriority);
    run_speed_ticks_per_second=pCSF->GetInt("Options","RunSpeed",run_speed_ticks_per_second);
    StartEmuOnClick=pCSF->GetInt("Options","StartOnClick",StartEmuOnClick);
  }

  SEC(PSEC_STARTUP){
    AutoLoadSnapShot=(bool)pCSF->GetInt("Options","AutoLoadSnapShot",AutoLoadSnapShot);
    AutoSnapShotName=pCSF->GetStr("Options","AutoSnapShotName",AutoSnapShotName);
  }

  SEC(PSEC_DISPFULL){

#if defined(STEVEN_SEAGAL) // load SSE options
//    TRACE("Retrieve SSE options\n");
#if defined(SSE_HACKS)
    SSE_HACKS_ON=pCSF->GetInt("Options","SpecificHacks",SSE_HACKS_ON);
#endif
#if defined(SSE_GUI_MOUSE_CAPTURE)
    CAPTURE_MOUSE=pCSF->GetInt("Options","CaptureMouse",CAPTURE_MOUSE);
#endif
#if defined(SSE_IKBD_6301)
    HD6301EMU_ON=pCSF->GetInt("Options","HD6301Emu",HD6301EMU_ON);
    if(!HD6301_OK)
      HD6301EMU_ON=0;
#if defined(SSE_IKBD_6301_NOT_OPTIONAL)
    else
      HD6301EMU_ON=1;
#endif
#endif
#if defined(SSE_VAR_STEALTH) 
    STEALTH_MODE=pCSF->GetInt("Options","StealthMode",STEALTH_MODE);
#endif
#if defined(SSE_SDL) && !defined(SSE_SDL_DEACTIVATE)
    USE_SDL=pCSF->GetInt("Options","UseSDL",USE_SDL);
#endif
#if defined(SSE_STF) && SSE_VERSION<370 //double, also in machine
    ST_TYPE=pCSF->GetInt("Options","StType",ST_TYPE);
#endif
#if defined(SSE_MMU_WAKE_UP)
//    WAKE_UP_STATE=pCSF->GetInt("Options","WakeUpState",WAKE_UP_STATE);
#endif
#if defined(SSE_VID_BORDERS) && !defined(SSE_VID_BORDERS_NO_LOAD_SETTING)
    //v3.7.0, we wouldn't keep this setting through sessions
    DISPLAY_SIZE=pCSF->GetInt("Display","BorderSize",DISPLAY_SIZE);
    if(DISPLAY_SIZE<0||DISPLAY_SIZE>BIGGEST_DISPLAY
#if defined(SSE_VID_380)
      || Disp.Method==DISPMETHOD_GDI // Disp has already been initialised
#endif
      )
      DISPLAY_SIZE=0;
    ChangeBorderSize(DISPLAY_SIZE);
#endif
#if defined(SSE_YM2149_FIX_TABLES)
#if SSE_VERSION>=370
    SSEOption.PSGMod=pCSF->GetInt("Sound","PsgMod",SSEOption.PSGMod);
#else
    PSG_FILTER_FIX=SSEOption.PSGMod=pCSF->GetInt("Sound","PsgMod",SSEOption.PSGMod);
#endif
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
    if(SSEOption.PSGMod)
      YM2149.LoadFixedVolTable();
    else
      YM2149.FreeFixedVolTable();
#endif
#endif
#if defined(SSE_YM2149_FIXED_VOL_TABLE) && !defined(SSE_YM2149_NO_SAMPLES_OPTION)
    SSEOption.PSGFixedVolume=pCSF->GetInt("Sound","PsgFixedVolume",SSEOption.PSGFixedVolume);
#endif
#if defined(SSE_SOUND_FILTER_STF)
    PSG_FILTER_FIX=pCSF->GetInt("Sound","PsgFilter",PSG_FILTER_FIX);
#endif
#if defined(SSE_SOUND_MICROWIRE)
    MICROWIRE_ON=pCSF->GetInt("Sound","Microwire",MICROWIRE_ON);
#endif
#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_OPTION_DISABLE_DSP)
    DSP_ENABLED=pCSF->GetInt("Sound","Dsp",DSP_ENABLED);
#endif
#if defined(SSE_OSD_DRIVE_INFO)
    OSD_DRIVE_INFO=pCSF->GetInt("Options","OsdDriveInfo", OSD_DRIVE_INFO);
#endif
#if defined(SSE_OSD_SCROLLER_DISK_IMAGE)
    OSD_IMAGE_NAME=pCSF->GetInt("Options","OsdImageName", OSD_IMAGE_NAME);
#endif
#if defined(SSE_PASTI_ONLY_STX)
#if !defined(SSE_PASTI_ONLY_STX_OPTION1)
    PASTI_JUST_STX=pCSF->GetInt("Options","PastiJustStx",PASTI_JUST_STX);
#else
    PASTI_JUST_STX=true; // we trust it works (and wait for bug reports :))
#endif
#endif
#if defined(SSE_VID_SCANLINES_INTERPOLATED_SSE)
    SSE_INTERPOLATE=pCSF->GetInt("Options","InterpolatedScanlines",SSE_INTERPOLATE);
#endif
#if defined(SSE_GUI_STATUS_STRING)
    SSE_STATUS_BAR=pCSF->GetInt("Options","StatusBar",SSE_STATUS_BAR);
#endif
#if defined(SSE_GUI_STATUS_STRING_DISK_NAME_OPTION)
    SSE_STATUS_BAR_GAME_NAME=pCSF->GetInt("Options","StatusBarGameName",SSE_STATUS_BAR_GAME_NAME);
#endif
#if defined(SSE_VID_VSYNC_WINDOW)
    SSE_WIN_VSYNC=pCSF->GetInt("Options","WinVSync",SSE_WIN_VSYNC);
#endif
#if defined(SSE_VID_3BUFFER)
    SSE_3BUFFER=pCSF->GetInt("Options","TripleBuffer",SSE_3BUFFER);
#endif
#if defined(SSE_DRIVE_SOUND)
    SSEOption.DriveSound=pCSF->GetInt("Options","DriveSound",SSEOption.DriveSound);
#if defined(SSE_DRIVE_SOUND_VOLUME)
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) 
    SF314[1].Sound_Volume=
#endif
    SF314[0].Sound_Volume=pCSF->GetInt("Options","DriveSoundVolume",SF314[0].Sound_Volume);
    SF314[0].Sound_ChangeVolume();
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) 
    SF314[1].Sound_ChangeVolume();
#endif
#endif
#if defined(SSE_DISK_GHOST)
    SSE_GHOST_DISK=pCSF->GetInt("Options","GhostDisk",SSE_GHOST_DISK);
#endif
#if defined(SSE_VID_D3D_OPTION) &&!defined(SSE_VID_D3D_ONLY)
    SSE_OPTION_D3D=pCSF->GetInt("Options","Direct3D",SSE_OPTION_D3D);
#ifndef SSE_VID_D3D_ONLY
    Disp.ScreenChange();
#endif
#endif
#if defined(SSE_VID_D3D_STRETCH_ASPECT_RATIO_OPTION)
    OPTION_ST_ASPECT_RATIO=pCSF->GetInt("Options","STAspectRatio",OPTION_ST_ASPECT_RATIO);
#endif
#if defined(SSE_DRIVE_SOUND_SEEK5)
    DRIVE_SOUND_SEEK_SAMPLE=pCSF->GetInt("Options","DriveSoundSeekSample",DRIVE_SOUND_SEEK_SAMPLE);
#endif
#if defined(SSE_GUI_CUSTOM_WINDOW_TITLE)
/*  Request
    For instance, in the [Options] part of steem.ini you add the line
    WindowTitle=Steem SSE teh beST
    and this will be the title of the window
    limited to WINDOW_TITLE_MAX_CHARS (20)
*/
    {
      EasyStr tmp=pCSF->GetStr("Main","WindowTitle",WINDOW_TITLE);
      strncpy(stem_window_title,tmp.Text,WINDOW_TITLE_MAX_CHARS);
      SetWindowText(StemWin,stem_window_title);
    }
#endif
#if defined(SSE_VAR_SNAPSHOT_INI)
/*  Request, be able to load a specified snapshot using a shortcut.
    We also don't want to bloat Steem, so we use BootStateFile.
    This variable is overwritten if Steem is launched with a state
    file. This seems appropriate behaviour.
    bugfix v3.8.0: statefile as argument was ignored, broke DemoBaseST
*/

#if defined(SSE_VAR_SNAPSHOT_INI2)
    BootStateFile=pCSF->GetStr("Main","DefaultSnapshot",BootStateFile.Text);
#else
    BootStateFile=pCSF->GetStr("Main","DefaultSnapshot","");
#endif
#ifdef SSE_DEBUG
    if(BootStateFile.NotEmpty())
      TRACE_INIT("BootStateFile %s\n",BootStateFile.Text);
#endif
#endif
#if defined(SSE_STF_MATCH_TOS3)
    Tos.DefaultCountry=pCSF->GetInt("Main","TosDefaultCountry",7); // 7=UK
#endif
#if defined(SSE_TOS_GEMDOS_RESTRICT_TOS3)
    Tos.VersionWarning=pCSF->GetInt("Main","TosVersionWarning",1);
#endif
#if defined(SSE_VID_DISABLE_AUTOBORDER3) // monochrome
    SSEOption.MonochromeDisableBorder=pCSF->GetInt("Main","MonochromeDisableBorder",0);
#endif
#if defined(SSE_GUI_OPTION_FOR_TESTS)
    SSE_TEST_ON=pCSF->GetInt("Options","TestingNewFeatures",SSE_TEST_ON);
#endif
#if defined(SSE_VID_BLOCK_WINDOW_SIZE)
    OPTION_BLOCK_RESIZE=pCSF->GetInt("Options","BlockResize",OPTION_BLOCK_RESIZE);
#endif
#if defined(SSE_VID_LOCK_ASPET_RATIO)
    OPTION_LOCK_ASPECT_RATIO=pCSF->GetInt("Options","LockAspectRatio",OPTION_LOCK_ASPECT_RATIO);
#endif
#if defined(SSE_VID_D3D_LIST_MODES)
    Disp.D3DMode=pCSF->GetInt("Display","D3DMode",Disp.D3DMode);
#if defined(SSE_VID_D3D_382)
    Disp.D3DUpdateWH(Disp.D3DMode); // function returns if no pD3D
  //TRACE_LOG("Option D3D mode = %d %dx%d\n",Disp.D3DMode,Disp.D3DFsW,Disp.D3DFsH);
#endif
#endif
#if defined(SSE_INT_MFP_OPTION)
  OPTION_PRECISE_MFP=pCSF->GetInt("Option","MC68901",OPTION_PRECISE_MFP);
#endif
#if defined(SSE_TOS_PRG_AUTORUN)
  OPTION_PRG_SUPPORT=pCSF->GetInt("Option","PRG_support",OPTION_PRG_SUPPORT);
#endif
#if defined(SSE_VID_D3D_CRISP_OPTION)
  OPTION_D3D_CRISP=pCSF->GetInt("Option","Direct3DCrisp",OPTION_D3D_CRISP);
#endif
#if defined(SSE_ACSI_OPTION)
  SSEOption.Acsi=pCSF->GetInt("Option","Acsi",SSEOption.Acsi);
#if defined(SSE_ACSI_ICON)
  SendMessage(GetDlgItem(DiskMan.Handle,11),BM_SETCHECK,SSEOption.Acsi,0);
#endif
#endif//acsi
#if defined(SSE_VAR_KEYBOARD_CLICK2) // option is persistent
  OPTION_KEYBOARD_CLICK=pCSF->GetInt("Option","KeyboardClick",OPTION_KEYBOARD_CLICK);
#endif
#if defined(SSE_VID_FS_GUI_OPTION)
  OPTION_FULLSCREEN_GUI=pCSF->GetInt("Option","FullScreenGUI",OPTION_FULLSCREEN_GUI);
#endif
#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
  SSEOption.VMMouse=pCSF->GetInt("Option","VMMouse",SSEOption.VMMouse);
#endif
#if defined(SSE_OSD_SHOW_TIME)
  SSEOption.OsdTime=pCSF->GetInt("Option","OsdTime",SSEOption.OsdTime);
#endif

#endif//steven_seagal


#if defined(SSE_BOILER_SSE_PERSISTENT)
#if !defined(SSE_BOILER_TRACE_NOT_OPTIONAL)
    USE_TRACE_FILE=pCSF->GetInt("Debug","UseTraceFile",USE_TRACE_FILE);
    CheckMenuItem(sse_menu,1517,MF_BYCOMMAND|
      ((USE_TRACE_FILE)?MF_CHECKED:MF_UNCHECKED));
#endif
    TRACE_FILE_REWIND=pCSF->GetInt("Debug","TraceFileRewind",TRACE_FILE_REWIND);
    CheckMenuItem(sse_menu,1518,MF_BYCOMMAND|
      ((TRACE_FILE_REWIND)?MF_CHECKED:MF_UNCHECKED));
#if defined(SSE_BOILER_MONITOR_VALUE)
    Debug.MonitorValueSpecified=pCSF->GetInt("Debug","MonitorValueSpecified",Debug.MonitorValueSpecified);
    CheckMenuItem(sse_menu,1522,MF_BYCOMMAND|
      ((Debug.MonitorValueSpecified)?MF_CHECKED:MF_UNCHECKED));
#endif
#if defined(SSE_BOILER_MONITOR_RANGE)
    Debug.MonitorRange=pCSF->GetInt("Debug","MonitorRange",Debug.MonitorRange); 
    CheckMenuItem(sse_menu,1523,MF_BYCOMMAND|
      ((Debug.MonitorRange)?MF_CHECKED:MF_UNCHECKED));
#endif
#if defined(SSE_BOILER_68030_STACK_FRAME)
    Debug.M68030StackFrame=pCSF->GetInt("Debug","M68030StackFrame",Debug.M68030StackFrame); 
    CheckMenuItem(sse_menu,1525,MF_BYCOMMAND|
      ((Debug.M68030StackFrame)?MF_CHECKED:MF_UNCHECKED));
#endif
#if defined(SSE_BOILER_STACK_CHOICE)
    Debug.StackDisplayUseOtherSp=pCSF->GetInt("Debug","StackDisplayUseOtherSp",Debug.StackDisplayUseOtherSp); 
    CheckMenuItem(sse_menu,1528,MF_BYCOMMAND|
      ((Debug.StackDisplayUseOtherSp)?MF_CHECKED:MF_UNCHECKED));
#endif
#if defined(SSE_BOILER_FAKE_IO)
    for(int i=0;i<FAKE_IO_LENGTH/2;i++)
    {
      char buffer[15];
      sprintf(buffer,"ControlMask%d",i);
      Debug.ControlMask[i]=(WORD)pCSF->GetInt("Debug",buffer,Debug.ControlMask[i]);
#if defined(SSE_BOILER_NEXT_PRG_RUN)
      if((BOILER_CONTROL_MASK1&BOILER_CONTROL_NEXT_PRG_RUN))
      {
        stop_on_next_program_run=2;
        CheckMenuItem(boiler_op_menu,1513,MF_BYCOMMAND | MF_CHECKED);//update
      }
#endif
    }
#endif

#endif//debug

#endif // SS

    frameskip=pCSF->GetInt("Options","FrameSkip",frameskip);
//    osd_on=(bool)pCSF->GetInt("Options","OSD",osd_on);
#if !defined(SSE_VID_D3D_ONLY)
    draw_fs_blit_mode=pCSF->GetInt("Options","DrawFSMode",draw_fs_blit_mode);
#endif
    FSDoVsync=(bool)pCSF->GetInt("Display","FSDoVsync",FSDoVsync);
#if !defined(SSE_VID_D3D_ONLY)
    prefer_res_640_400=(bool)pCSF->GetInt("Display","Prefer640x400",prefer_res_640_400);
#endif
    ResChangeResize=(bool)pCSF->GetInt("Display","ResChangeResize",ResChangeResize);
#if !defined(SSE_VID_D3D_ONLY)
    draw_fs_fx=pCSF->GetInt("Options","InterlaceMode",draw_fs_fx);
    if (draw_fs_fx==DFSFX_BLUR) draw_fs_fx=DFSFX_NONE;
#endif
    UNIX_ONLY( ResChangeResize=true; )
    WinSizeForRes[0]=pCSF->GetInt("Display","WinSizeLowRes",WinSizeForRes[0]);
    WinSizeForRes[1]=pCSF->GetInt("Display","WinSizeMedRes",WinSizeForRes[1]);
    WinSizeForRes[2]=pCSF->GetInt("Display","WinSizeHighRes",WinSizeForRes[2]);
#ifdef WIN32
    int sl=pCSF->GetInt("Display","DrawWinMode_LowRes",-1);
    int sm=pCSF->GetInt("Display","DrawWinMode_MedRes",-1);
    if (sl<0){
#if defined(SSE_VID_D3D_ONLY)
      sl=sm=DWM_NOSTRETCH;
#else
      sl=sm=int((draw_fs_blit_mode==DFSM_STRETCHBLIT || draw_fs_blit_mode==DFSM_LAPTOP) ? DWM_STRETCH:DWM_NOSTRETCH);
#endif
#if !defined(SSE_VID_D3D_ONLY)
      if (draw_fs_fx==DFSFX_GRILLE && sl==DWM_NOSTRETCH) sl=sm=DWM_GRILLE;
#endif
    }
    draw_win_mode[0]=sl;
    draw_win_mode[1]=sm;
#endif

    // Loading of border is now practically ignored (because it can be set to 0
    // by going to windowed mode). Only used first load of v2.06.
#if defined(SSE_VID_DISABLE_AUTOBORDER)
    //border=min(pCSF->GetInt("Display","Border",border),1);
    border=pCSF->GetInt("Display","Border",border)&1;
    border_last_chosen=pCSF->GetInt("Display","BorderLastChosen",border)&1;
#else
    border=min(pCSF->GetInt("Display","Border",border),2);
    border_last_chosen=min(pCSF->GetInt("Display","BorderLastChosen",border),2);
#endif
    if (!Disp.BorderPossible())
    {
      border=0;
      EnableBorderOptions(false);
    }else
    {
      border=border_last_chosen;
    }
    display_option_8_bit_fs=pCSF->GetInt("Display","EightBitInFS",display_option_8_bit_fs);

    brightness=pCSF->GetInt("Options","Brightness",brightness);
    contrast=pCSF->GetInt("Options","Contrast",contrast);
    make_palette_table(brightness,contrast);
#if !defined(SSE_VID_D3D_ONLY)
    for (int c16=0;c16<2;c16++){
      Str c256=LPSTR(c16 ? "":"_256");
      for (int res=0;res<3;res++){
        prefer_pc_hz[c16][res]=pCSF->GetInt("Options",Str("Hz_")+res+c256,prefer_pc_hz[c16][res]);
        tested_pc_hz[c16][res]=(WORD)pCSF->GetInt("Options",Str("TestedHz_")+res+c256,tested_pc_hz[c16][res]);
      }
    }
#endif//#if !defined(SSE_VID_D3D_ONLY)
    Disp.DoAsyncBlit=pCSF->GetInt("Options","DoAsyncBlit",Disp.DoAsyncBlit);

    ScreenShotFol=pCSF->GetStr("Options","ScreenShotFol",WriteDir+SLASH+"screenshots");
    NO_SLASH(ScreenShotFol);
    if (GetFileAttributes(ScreenShotFol)==0xffffffff){
#ifndef ONEGAME
      ScreenShotFol=WriteDir+SLASH+T("screenshots");
      CreateDirectory(ScreenShotFol,NULL);
#else
      ScreenShotFol=WriteDir;
#endif
    }

    Disp.ScreenShotMinSize=pCSF->GetInt("Options","ScreenShotMinSize",Disp.ScreenShotMinSize);
    Disp.ScreenShotFormat=pCSF->GetInt("Options","ScreenShotFormat",Disp.ScreenShotFormat);
#ifdef WIN32
    Disp.ScreenShotExt=pCSF->GetStr("Options","ScreenShotExt",Disp.ScreenShotExt);
    Disp.ScreenShotFormatOpts=pCSF->GetInt("Options","ScreenShotFormatOpts",Disp.ScreenShotFormatOpts);
    Disp.ScreenShotCheckFreeImageLoad();
#endif

    FSQuitAskFirst=pCSF->GetInt("Options","FSQuitAskFirst",FSQuitAskFirst);
  }

  SEC(PSEC_OSD){
    osd_show_disk_light=pCSF->GetInt("Options","OSDDiskLight",osd_show_disk_light);
    osd_show_plasma=pCSF->GetInt("Options","OSDPlasma",osd_show_plasma);
    osd_show_speed=pCSF->GetInt("Options","OSDSpeed",osd_show_speed);
    osd_show_icons=pCSF->GetInt("Options","OSDIcons",osd_show_icons);
    osd_show_cpu=pCSF->GetInt("Options","OSDCPU",osd_show_cpu);
    osd_show_scrollers=pCSF->GetInt("Options","OSDScroller",osd_show_scrollers);
    osd_disable=pCSF->GetInt("Options","OSDDisable",osd_disable);
    osd_old_pos=pCSF->GetInt("Options","OSDOldPos",osd_old_pos);
  }

  SEC(PSEC_SOUND){
    MaxVolume=pCSF->GetInt("Options","Volume",MaxVolume);
    //TRACE("get MaxVolume %d",MaxVolume);

#if defined(SSE_SOUND_VOL_LOGARITHMIC_3)//v3.7.1
    MaxVolume=min(MaxVolume,10000);
#endif
    sound_mode=pCSF->GetInt("Options","SoundMode",sound_mode);
    sound_last_mode=pCSF->GetInt("Options","LastSoundMode",sound_last_mode);
    int slq=pCSF->GetInt("Options","SoundLowQuality",999);
    if (slq==0 || slq==1){
      sound_chosen_freq=int(slq ? 25033:50066);
      pCSF->SetStr("Options","SoundLowQuality","999");
    }else if (sound_comline_freq==0){
      sound_chosen_freq=pCSF->GetInt("Sound","Freq",sound_chosen_freq);
    }
    UpdateSoundFreq();
    sound_num_bits=(BYTE)pCSF->GetInt("Sound","Bits",sound_num_bits);
    sound_num_channels=(BYTE)pCSF->GetInt("Sound","Channels",sound_num_channels);
    sound_bytes_per_sample=(sound_num_bits/8)*sound_num_channels;
    sound_write_primary=pCSF->GetInt("Sound","WritePrimary",sound_write_primary);
    sound_time_method=pCSF->GetInt("Sound","TimeMethod",sound_time_method);
    psg_write_n_screens_ahead=pCSF->GetInt("Sound","WriteAhead",psg_write_n_screens_ahead);
    WAVOutputFile=pCSF->GetStr("Sound","WAVOutputFile",WAVOutputFile);
    if (WAVOutputFile.Empty()) WAVOutputFile=WriteDir+SLASH "st.wav";
    RecordWarnOverwrite=pCSF->GetInt("Sound","RecordWarnOverwrite",RecordWarnOverwrite);
    WAVOutputDir=pCSF->GetStr("Sound","WAVOutputDir",WAVOutputDir);
    if (WAVOutputDir.Empty()) WAVOutputDir=WriteDir;

    #ifdef UNIX
      x_sound_lib=pCSF->GetInt("Sound","Library",x_sound_lib);
      #ifndef NO_RTAUDIO
        rt_unsigned_8bit=pCSF->GetInt("Sound","RtAudioUnsigned8Bit",rt_unsigned_8bit);
      #endif
      sound_device_name=pCSF->GetStr("Sound","PADevice",Str(sound_device_name));
      if (FirstLoad==0){
        Sound_Stop(true);
        InitSound();
        Sound_Start();
      }
    #endif//UNIX
#if !defined(SOUND_DISABLE_INTERNAL_SPEAKER)
    sound_internal_speaker=pCSF->GetInt("Sound","InternalSpeaker",sound_internal_speaker);
    // Trying to write to ports on WINNT causes the program to be killed!
    WIN_ONLY( if (WinNT) sound_internal_speaker=0; )
#endif
  }

  SEC(PSEC_PORTS){
#ifdef WIN32
    EasyStringList MidiOutList(eslNoSort),MidiInList(eslNoSort);
    Str MidiOutName,MidiInName;
    int c;
    MIDIOUTCAPS moc;
    MIDIINCAPS mic;

    c=midiOutGetNumDevs();
    for (int n=0;n<c;n++){
      midiOutGetDevCaps(n,&moc,sizeof(moc));
      MidiOutList.Add(moc.szPname);
    }
    c=midiInGetNumDevs();
    for (int n=0;n<c;n++){
      midiInGetDevCaps(n,&mic,sizeof(mic));
      MidiInList.Add(mic.szPname);
    }
#endif
    for (int p=0;p<3;p++){
      EasyStr PNam=EasyStr("Port_")+p+"_";
      STPort[p].Type=pCSF->GetInt("MIDI",PNam+"Type",STPort[p].Type);
      
#ifdef WIN32
      int NewDev=-1;
      Str Name=pCSF->GetStr("MIDI",PNam+"MIDIOutName","");
      if (Name.NotEmpty()) NewDev=MidiOutList.FindString_I(Name);
      if (NewDev>=0){
        STPort[p].MIDIOutDevice=NewDev;
      }else{
        STPort[p].MIDIOutDevice=pCSF->GetInt("MIDI",PNam+"MIDIOutDevice",STPort[p].MIDIOutDevice);
      }

      NewDev=-1;
      Name=pCSF->GetStr("MIDI",PNam+"MIDIInName","");
      if (Name.NotEmpty()) NewDev=MidiInList.FindString_I(Name);
      if (NewDev>=0){
        STPort[p].MIDIInDevice=NewDev;
      }else{
        STPort[p].MIDIInDevice=pCSF->GetInt("MIDI",PNam+"MIDIInDevice",STPort[p].MIDIInDevice);
      }

      STPort[p].COMNum=pCSF->GetInt("MIDI",PNam+"COMNum",STPort[p].COMNum);
      STPort[p].LPTNum=pCSF->GetInt("MIDI",PNam+"LPTNum",STPort[p].LPTNum);
#elif defined(UNIX)
      for (int n=0;n<TPORTIO_NUM_TYPES;n++){
        STPort[p].PortDev[n]=pCSF->GetStr("MIDI",PNam+"Dev_"+n,STPort[p].PortDev[n]);
        STPort[p].AllowIO[n][0]=(bool)pCSF->GetInt("MIDI",PNam+"Allow_"+n+"_Out",STPort[p].AllowIO[n][0]);
        STPort[p].AllowIO[n][1]=(bool)pCSF->GetInt("MIDI",PNam+"Allow_"+n+"_In",STPort[p].AllowIO[n][1]);
      }
      STPort[p].LANPipeIn=pCSF->GetStr("MIDI",PNam+"PipeInput",STPort[p].LANPipeIn);
#endif
      // This has to be initialised here because of RunDir
      STPort[p].File=WriteDir;
      switch (p){
        case 0:STPort[p].File+=SLASH "midi.dmp";break;
        case 1:STPort[p].File+=SLASH "printer.dmp";break;
        case 2:STPort[p].File+=SLASH "serial.dmp";break;
      }
      STPort[p].File=pCSF->GetStr("MIDI",PNam+"File",STPort[p].File);
    }

    // Legacy
#ifdef WIN32
    if (pCSF->GetInt("MIDI","OutDevice",1000)<1000){
      MIDIPort.Type=PORTTYPE_MIDI;
      MIDIPort.MIDIOutDevice=pCSF->GetInt("MIDI","OutDevice",MIDIPort.MIDIOutDevice);
      MIDIPort.MIDIInDevice=pCSF->GetInt("MIDI","InDevice",MIDIPort.MIDIInDevice);

      int PrintDevice=pCSF->GetInt("MIDI","PrintDevice",0);
      switch (PrintDevice){
        case 1:case 2:
          ParallelPort.Type=PORTTYPE_PARALLEL;
          ParallelPort.LPTNum=PrintDevice-1;
          break;
        case 3: ParallelPort.Type=PORTTYPE_FILE; break;
        case 4: ParallelPort.Type=PORTTYPE_MIDI; break;
      }
      ParallelPort.File=pCSF->GetStr("MIDI","PrintFilePath",ParallelPort.File);
      ParallelPort.MIDIOutDevice=pCSF->GetInt("MIDI","PrintMIDIDevice",ParallelPort.MIDIOutDevice);

      pCSF->SetStr("MIDI","OutDevice","1000");
    }
#endif

    for (int p=0;p<3;p++){
      if ((STPort[p].Type==PORTTYPE_PARALLEL && AllowLPT==0) ||
          (STPort[p].Type==PORTTYPE_COM && AllowCOM==0)){
        STPort[p].Type=PORTTYPE_NONE;
      }
    }

    if (pCSF->GetInt("MIDI","Port_0_MIDIMessMax",0)==0){
      MIDI_out_volume=(WORD)pCSF->GetInt("MIDI","OutVolume",MIDI_out_volume);
      MIDI_in_sysex_max=pCSF->GetInt("MIDI","InMaxSysEx",MIDI_in_sysex_max);
      MIDI_out_sysex_max=pCSF->GetInt("MIDI","OutMaxSysEx",MIDI_out_sysex_max);
    }else{
      MIDI_out_volume=(WORD)pCSF->GetInt("MIDI","Port_0_MIDIVol",MIDI_out_volume);
      MIDI_in_sysex_max=pCSF->GetInt("MIDI","Port_0_MIDIMessMax",MIDI_in_sysex_max);
      MIDI_out_sysex_max=pCSF->GetInt("MIDI","Port_0_MIDIMessMax",MIDI_out_sysex_max);
      pCSF->SetInt("MIDI","Port_0_MIDIMessMax",0);
    }

    MIDI_out_running_status_flag=pCSF->GetInt("MIDI","OutRunningStatus",MIDI_out_running_status_flag);
    MIDI_in_running_status_flag=pCSF->GetInt("MIDI","InRunningStatus",MIDI_in_running_status_flag);
    MIDI_in_n_sysex=pCSF->GetInt("MIDI","InSysExBufs",MIDI_in_n_sysex);
    MIDI_out_n_sysex=pCSF->GetInt("MIDI","OutSysExBufs",MIDI_out_n_sysex);
    MIDI_in_speed=pCSF->GetInt("MIDI","InSpeed",MIDI_in_speed);
  }

  SEC(PSEC_MACRO){
    MacroDir=pCSF->GetStr(Section,"MacroDir",WriteDir+SLASH "macros");
    NO_SLASH(MacroDir);
    if (GetFileAttributes(MacroDir)==0xffffffff){
#ifndef ONEGAME
      MacroDir=WriteDir+SLASH+T("macros");
      CreateDirectory(MacroDir,NULL);
#else
      MacroDir=WriteDir;
#endif
    }
    MacroSel=pCSF->GetStr(Section,"MacroSel","");
  }

  if (LastIconPath.Empty()) LastIconPath=RunDir+SLASH;
  LastIconPath=pCSF->GetStr("Options","LastIconPath",LastIconPath);
  if (LastIconSchemePath.Empty()) LastIconSchemePath=RunDir+SLASH;
  LastIconSchemePath=pCSF->GetStr("Options","LastIconSchemePath",LastIconSchemePath);

#ifdef UNIX
  for (int i=0;i<NUM_COMLINES;i++){
    Comlines[i]=pCSF->GetStr("Paths",Str("Path")+i,Comlines[i]);
  }
#endif

#ifdef WIN32
#if defined(SSE_VID_DISABLE_AUTOBORDER)
  CheckMenuRadioItem(StemWin_SysMenu,110,112,110+min((int)border,1),MF_BYCOMMAND);
#elif defined(SSE_VAR_RESIZE_370)
  CheckMenuRadioItem(StemWin_SysMenu,110,112,110+min((int)border,2),MF_BYCOMMAND);
#else
  CheckMenuRadioItem(StemWin_SysMenu,110,112,110+min(border,2),MF_BYCOMMAND);
#endif
  CheckMenuItem(StemWin_SysMenu,113,MF_BYCOMMAND | int(osd_disable ? MF_CHECKED:MF_UNCHECKED));
#endif

  if (FirstLoad){
    ProfileDir=pCSF->GetStr(Section,"ProfileDir",WriteDir+SLASH "profiles");
    NO_SLASH(ProfileDir);
    if (GetFileAttributes(ProfileDir)==0xffffffff){
#ifndef ONEGAME
      ProfileDir=WriteDir+SLASH+T("profiles");
      CreateDirectory(ProfileDir,NULL);
#else
      ProfileDir=WriteDir;
#endif
    }
    ProfileSel=pCSF->GetStr(Section,"ProfileSel","");

    Page=pCSF->GetInt("Options","Page",Page);
    UPDATE;
  }
  return true;
}
//---------------------------------------------------------------------------
bool TOptionBox::SaveData(bool FinalSave,ConfigStoreFile *pCSF)
{
  SavePosition(FinalSave,pCSF);

  pCSF->SetStr("Options","CPUBoost",EasyStr(n_cpu_cycles_per_second));

  WIN_ONLY( pCSF->SetStr("Options","AllowTaskSwitch",LPSTR(AllowTaskSwitch ? "1":"0")); )
  pCSF->SetStr("Options","PauseWhenInactive",LPSTR(PauseWhenInactive ? "1":"0"));
  pCSF->SetInt("Options","DiskAccessFF",floppy_access_ff);

  pCSF->SetStr("Options","AutoLoadSnapShot",LPSTR(AutoLoadSnapShot ? "1":"0"));


  pCSF->SetStr("Options","FrameSkip",EasyStr(frameskip));
#if !defined(SSE_VID_D3D_ONLY)
  pCSF->SetStr("Options","DrawFSMode",EasyStr(draw_fs_blit_mode));
#endif
  pCSF->SetStr("Display","FSDoVsync",LPSTR(FSDoVsync ? "1":"0"));
#if !defined(SSE_VID_D3D_ONLY)
  pCSF->SetStr("Display","Prefer640x400",LPSTR(prefer_res_640_400 ? "1":"0"));
#endif
  pCSF->SetStr("Options","ShowToolTips",EasyStr(ShowTips));

#if defined(STEVEN_SEAGAL)
#if defined(SSE_HACKS)
  pCSF->SetStr("Options","SpecificHacks",EasyStr(SSE_HACKS_ON));
#endif
#if defined(SSE_GUI_MOUSE_CAPTURE)
  pCSF->SetStr("Options","CaptureMouse",EasyStr(CAPTURE_MOUSE));
#endif
#if defined(SSE_IKBD_6301)
  pCSF->SetStr("Options","HD6301Emu",EasyStr(HD6301EMU_ON));
#endif
#if defined(SSE_VAR_STEALTH) 
  pCSF->SetStr("Options","StealthMode",EasyStr(STEALTH_MODE));
#endif
#if defined(SSE_SDL) && !defined(SSE_SDL_DEACTIVATE)
  pCSF->SetStr("Options","UseSDL",EasyStr(USE_SDL));  
#endif
#if defined(SSE_STF) && SSE_VERSION<370
  pCSF->SetStr("Options","StType",EasyStr(ST_TYPE));  
#endif
#if defined(SSE_MMU_WAKE_UP)
//  pCSF->SetStr("Options","WakeUpState",EasyStr(WAKE_UP_STATE));  
#endif
#if defined(SSE_VID_BORDERS) && !defined(SSE_VID_BORDERS_NO_LOAD_SETTING)
  pCSF->SetStr("Display","BorderSize",EasyStr(DISPLAY_SIZE));  
#endif
#if defined(SSE_YM2149_FIX_TABLES)
  pCSF->SetStr("Sound","PsgMod",EasyStr(SSEOption.PSGMod));  
#endif
#if defined(SSE_YM2149_FIXED_VOL_TABLE) && !defined(SSE_YM2149_NO_SAMPLES_OPTION)
  pCSF->SetStr("Sound","PsgFixedVolume",EasyStr(SSEOption.PSGFixedVolume));  
#endif
#if defined(SSE_SOUND_FILTER_STF)
  pCSF->SetStr("Sound","PsgFilter",EasyStr(PSG_FILTER_FIX));  
#endif
#if defined(SSE_SOUND_MICROWIRE)
  pCSF->SetStr("Sound","Microwire",EasyStr(MICROWIRE_ON));  
#endif
#if defined(SSE_SOUND_OPTION_DISABLE_DSP)
  pCSF->SetStr("Sound","Dsp",EasyStr(DSP_ENABLED));  
#endif
#if defined(SSE_OSD_DRIVE_INFO)
  pCSF->SetStr("Options","OsdDriveInfo",EasyStr(OSD_DRIVE_INFO));  
#endif
#if defined(SSE_OSD_SCROLLER_DISK_IMAGE)
  pCSF->SetStr("Options","OsdImageName",EasyStr(OSD_IMAGE_NAME));  
#endif
#if !defined(SSE_PASTI_ONLY_STX_OPTION1)
  pCSF->SetStr("Options","PastiJustStx",EasyStr(PASTI_JUST_STX));  
#endif
#if defined(SSE_VID_SCANLINES_INTERPOLATED_SSE)
  pCSF->SetStr("Options","InterpolatedScanlines",EasyStr(SSE_INTERPOLATE));  
#endif
#if defined(SSE_GUI_STATUS_STRING)
  pCSF->SetStr("Options","StatusBar",EasyStr(SSE_STATUS_BAR));
#endif
#if defined(SSE_GUI_STATUS_STRING_DISK_NAME_OPTION)
  pCSF->SetStr("Options","StatusBarGameName",EasyStr(SSE_STATUS_BAR_GAME_NAME));
#endif
#if defined(SSE_VID_VSYNC_WINDOW)
  pCSF->SetStr("Options","WinVSync",EasyStr(SSE_WIN_VSYNC));
#endif
#if defined(SSE_VID_3BUFFER)
  pCSF->SetStr("Options","TripleBuffer",EasyStr(SSE_3BUFFER));
#endif
#if defined(SSE_DRIVE_SOUND)
  pCSF->SetStr("Options","DriveSound",EasyStr(SSEOption.DriveSound));
#if defined(SSE_DRIVE_SOUND_VOLUME) //one for both drives
  pCSF->SetStr("Options","DriveSoundVolume",EasyStr(SF314[0].Sound_Volume)); 
#endif
#endif
#if defined(SSE_DISK_GHOST)
  pCSF->SetStr("Options","GhostDisk",EasyStr(SSE_GHOST_DISK)); 
#endif
#if defined(SSE_VID_D3D_OPTION)
  pCSF->SetStr("Options","Direct3D",EasyStr(SSE_OPTION_D3D)); 
#endif
#if defined(SSE_VID_D3D_STRETCH_ASPECT_RATIO_OPTION)
  pCSF->SetStr("Options","STAspectRatio",EasyStr(OPTION_ST_ASPECT_RATIO));
#endif
#if defined(SSE_DRIVE_SOUND_SEEK5)
  pCSF->SetStr("Options","DriveSoundSeekSample",EasyStr(DRIVE_SOUND_SEEK_SAMPLE));
#endif
#if defined(SSE_GUI_OPTION_FOR_TESTS)
  pCSF->SetStr("Options","TestingNewFeatures",EasyStr(SSE_TEST_ON));
#endif
#if defined(SSE_VID_BLOCK_WINDOW_SIZE)
  pCSF->SetStr("Options","BlockResize",EasyStr(OPTION_BLOCK_RESIZE));
#endif
#if defined(SSE_VID_LOCK_ASPET_RATIO)
  pCSF->SetStr("Options","LockAspectRatio",EasyStr(OPTION_LOCK_ASPECT_RATIO));
#endif
#if defined(SSE_VID_D3D_LIST_MODES)
  pCSF->SetStr("Display","D3DMode",EasyStr(Disp.D3DMode));
#endif
#if defined(SSE_INT_MFP_OPTION)//TODO
  pCSF->SetStr("Option","MC68901",EasyStr(OPTION_PRECISE_MFP));
#endif
#if defined(SSE_TOS_PRG_AUTORUN)
  pCSF->SetStr("Option","PRG_support",EasyStr(OPTION_PRG_SUPPORT));
#endif
#if defined(SSE_VID_D3D_CRISP_OPTION)
  pCSF->SetStr("Option","Direct3DCrisp",EasyStr(OPTION_D3D_CRISP));
#endif
#if defined(SSE_ACSI_OPTION)
  pCSF->SetStr("Option","Acsi",EasyStr(SSEOption.Acsi));
#endif
#if defined(SSE_VAR_KEYBOARD_CLICK2)
  pCSF->SetStr("Option","KeyboardClick",EasyStr(OPTION_KEYBOARD_CLICK));
#endif
#if defined(SSE_VID_FS_GUI_OPTION)
  pCSF->SetStr("Option","FullScreenGUI",EasyStr(OPTION_FULLSCREEN_GUI));
#endif
#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
  pCSF->SetStr("Option","VMMouse",EasyStr(SSEOption.VMMouse));
#endif
#if defined(SSE_OSD_SHOW_TIME)
  pCSF->SetStr("Option","OsdTime",EasyStr(SSEOption.OsdTime));
#endif

//boiler
#if defined(SSE_BOILER_SSE_PERSISTENT)
#if !defined(SSE_BOILER_TRACE_NOT_OPTIONAL)
  pCSF->SetStr("Debug","UseTraceFile",EasyStr(USE_TRACE_FILE)); 
#endif
  pCSF->SetStr("Debug","TraceFileRewind",EasyStr(TRACE_FILE_REWIND)); 
#if defined(SSE_BOILER_MONITOR_VALUE)
  pCSF->SetStr("Debug","MonitorValueSpecified",EasyStr(Debug.MonitorValueSpecified)); 
#endif
#if defined(SSE_BOILER_MONITOR_RANGE)
  pCSF->SetStr("Debug","MonitorRange",EasyStr(Debug.MonitorRange)); 
#endif
#if defined(SSE_BOILER_68030_STACK_FRAME)
  pCSF->SetStr("Debug","M68030StackFrame",EasyStr(Debug.M68030StackFrame)); 
#endif
#if defined(SSE_BOILER_STACK_CHOICE)
  pCSF->SetStr("Debug","StackDisplayUseOtherSp",EasyStr(Debug.StackDisplayUseOtherSp)); 
#endif
#if defined(SSE_BOILER_FAKE_IO)
    for(int i=0;i<FAKE_IO_LENGTH/2;i++)
    {
      char buffer[15];
      sprintf(buffer,"ControlMask%d",i);
      pCSF->SetStr("Debug",buffer,EasyStr(Debug.ControlMask[i]));
    }
#endif
#endif//debug
#endif//SS

  pCSF->SetStr("Display","ResChangeResize",EasyStr(ResChangeResize));
  pCSF->SetStr("Display","WinSizeLowRes",EasyStr(WinSizeForRes[0]));
  pCSF->SetStr("Display","WinSizeMedRes",EasyStr(WinSizeForRes[1]));
  pCSF->SetStr("Display","WinSizeHighRes",EasyStr(WinSizeForRes[2]));
#ifdef WIN32
  pCSF->SetStr("Display","DrawWinMode_LowRes",EasyStr(draw_win_mode[0]));
  pCSF->SetStr("Display","DrawWinMode_MedRes",EasyStr(draw_win_mode[1]));
#endif

  pCSF->SetStr("Display","BorderLastChosen",EasyStr(border_last_chosen));

  pCSF->SetStr("Display","EightBitInFS",EasyStr(display_option_8_bit_fs));

  pCSF->SetStr("Options","Brightness",EasyStr(brightness));

  pCSF->SetStr("Options","Contrast",EasyStr(contrast));

  pCSF->SetStr("Options","SlowMotionSpeed",EasyStr(slow_motion_speed));

  pCSF->SetStr("Options","Page",EasyStr(Page));
#if !defined(SSE_VID_D3D_ONLY)
  for (int c16=0;c16<2;c16++){
    Str c256=LPSTR(c16 ? "":"_256");
    for (int res=0;res<3;res++){
      pCSF->SetInt("Options",Str("Hz_")+res+c256,prefer_pc_hz[c16][res]);
      pCSF->SetInt("Options",Str("TestedHz_")+res+c256,tested_pc_hz[c16][res]);
    }
  }
  pCSF->SetInt("Options","InterlaceMode",draw_fs_fx);
#endif//#if !defined(SSE_VID_D3D_ONLY)
  pCSF->SetInt("Options","DoAsyncBlit",Disp.DoAsyncBlit);

  pCSF->SetStr("Options","Volume",EasyStr(MaxVolume));
  //TRACE("write MaxVolume %d",MaxVolume);
  pCSF->SetStr("Options","SoundMode",EasyStr(sound_mode));
  pCSF->SetStr("Options","LastSoundMode",EasyStr(sound_last_mode));
  pCSF->SetStr("Options","SoundLowQuality","999");
  if (sound_chosen_freq!=sound_comline_freq) pCSF->SetStr("Sound","Freq",Str(sound_chosen_freq));
  pCSF->SetStr("Sound","Bits",Str(sound_num_bits));
  pCSF->SetStr("Sound","Channels",Str(sound_num_channels));
  pCSF->SetStr("Sound","WritePrimary",Str(sound_write_primary));
  pCSF->SetStr("Sound","TimeMethod",Str(sound_time_method));
  pCSF->SetStr("Sound","WriteAhead",Str(psg_write_n_screens_ahead));
  pCSF->SetStr("Sound","WAVOutputFile",WAVOutputFile);
  pCSF->SetStr("Sound","RecordWarnOverwrite",Str(RecordWarnOverwrite));
  pCSF->SetStr("Sound","WAVOutputDir",WAVOutputDir);
  #ifdef UNIX
    pCSF->SetInt("Sound","Library",x_sound_lib);
    #ifndef NO_RTAUDIO
      pCSF->SetInt("Sound","RtAudioUnsigned8Bit",rt_unsigned_8bit);
    #endif
    pCSF->SetStr("Sound","PADevice",Str(sound_device_name));
  #endif
#if !defined(SOUND_DISABLE_INTERNAL_SPEAKER)
  pCSF->SetStr("Sound","InternalSpeaker",Str(sound_internal_speaker));
#endif
  for (int p=0;p<3;p++){
    EasyStr PNam=EasyStr("Port_")+p+"_";
    pCSF->SetStr("MIDI",PNam+"Type",EasyStr(STPort[p].Type));

#ifdef WIN32
    Str MidiOutName,MidiInName;
    if (STPort[p].MIDIOutDevice>=0){
      MIDIOUTCAPS moc;
      midiOutGetDevCaps(STPort[p].MIDIOutDevice,&moc,sizeof(moc));
      MidiOutName=moc.szPname;
    }
    if (STPort[p].MIDIInDevice>=0){
      MIDIINCAPS mic;
      midiInGetDevCaps(STPort[p].MIDIInDevice,&mic,sizeof(mic));
      MidiInName=mic.szPname;
    }
    pCSF->SetStr("MIDI",PNam+"MIDIOutName",MidiOutName);
    pCSF->SetStr("MIDI",PNam+"MIDIOutDevice",EasyStr(STPort[p].MIDIOutDevice));
    pCSF->SetStr("MIDI",PNam+"MIDIInName",MidiInName);
    pCSF->SetStr("MIDI",PNam+"MIDIInDevice",EasyStr(STPort[p].MIDIInDevice));
    pCSF->SetStr("MIDI",PNam+"COMNum",EasyStr(STPort[p].COMNum));
    pCSF->SetStr("MIDI",PNam+"LPTNum",EasyStr(STPort[p].LPTNum));
#elif defined(UNIX)
    for (int n=0;n<TPORTIO_NUM_TYPES;n++){
	    pCSF->SetStr("MIDI",PNam+"Dev_"+n,STPort[p].PortDev[n]);
      pCSF->SetInt("MIDI",PNam+"Allow_"+n+"_Out",STPort[p].AllowIO[n][0]);
      pCSF->SetInt("MIDI",PNam+"Allow_"+n+"_In",STPort[p].AllowIO[n][1]);
    }
    pCSF->SetStr("MIDI",PNam+"PipeInput",STPort[p].LANPipeIn);
#endif

    pCSF->SetStr("MIDI",PNam+"File",EasyStr(STPort[p].File));
  }

  pCSF->SetInt("MIDI","OutRunningStatus",MIDI_out_running_status_flag);
  pCSF->SetInt("MIDI","InRunningStatus",MIDI_in_running_status_flag);
  pCSF->SetInt("MIDI","InSysExBufs",MIDI_in_n_sysex);
  pCSF->SetInt("MIDI","OutSysExBufs",MIDI_out_n_sysex);
  pCSF->SetInt("MIDI","InSpeed",MIDI_in_speed);
  pCSF->SetInt("MIDI","OutVolume",MIDI_out_volume);
  pCSF->SetInt("MIDI","InMaxSysEx",MIDI_in_sysex_max);
  pCSF->SetInt("MIDI","OutMaxSysEx",MIDI_out_sysex_max);

  pCSF->SetInt("Options","MaxFastForward",fast_forward_max_speed);
  pCSF->SetInt("Options","HighPriority",HighPriority);

  pCSF->SetStr("Options","AutoSnapShotName",AutoSnapShotName);

  pCSF->SetInt("Options","RunSpeed",run_speed_ticks_per_second);

  pCSF->SetStr("Options","ScreenShotFol",ScreenShotFol);
  pCSF->SetInt("Options","ScreenShotFormat",Disp.ScreenShotFormat);
#ifdef WIN32
  pCSF->SetStr("Options","ScreenShotExt",Disp.ScreenShotExt);
  pCSF->SetInt("Options","ScreenShotFormatOpts",Disp.ScreenShotFormatOpts);
#endif
  pCSF->SetInt("Options","ScreenShotMinSize",Disp.ScreenShotMinSize);

#if defined(STEVEN_SEAGAL) && defined(SSE_STF)
  pCSF->SetStr("Machine","STType",EasyStr(ST_TYPE));
#endif
#if defined(SSE_INT_MFP_RATIO_OPTION2)
  pCSF->SetStr("Option","FinetuneCPUclock",EasyStr(OPTION_CPU_CLOCK));
  pCSF->SetStr("Machine","CpuCustomHz",EasyStr(CpuCustomHz));
#endif
#if defined(SSE_GUI_CONFIG_FILE2)
/*  v3.8.0 Remove path info if it's TOS browse path.
    We do that to make config files more portable: the full path
    is individual, but TOS files are universal.
    Only for those config files, not steem.ini, so older versions
    of Steem won't be lost.
*/
  EasyStr tmp=ROMFile;
  RemoveFileNameFromPath(tmp,REMOVE_SLASH);
  if(!FinalSave&&tmp==TOSBrowseDir)
  {
      tmp=GetFileNameFromPath(ROMFile.Text); // don't change ROMFile itself
      pCSF->SetStr("Machine","ROM_File",tmp);
  }
  else
#endif
  pCSF->SetStr("Machine","ROM_File",ROMFile);
  //TRACE_INIT("write ROM_Add_Dir = %s\n",TOSBrowseDir.Text);
  pCSF->SetStr("Machine","ROM_Add_Dir",TOSBrowseDir);
  pCSF->SetStr("Machine","Cart_File",CartFile);
  pCSF->SetStr("Machine","LastCartFile",LastCartFile);
  pCSF->SetStr("Machine","Colour_Monitor",LPSTR((mfp_gpip_no_interrupt & MFP_GPIP_COLOUR) ? "1":"0"));
  BYTE MemConf[2]={MEMCONF_512,MEMCONF_512};
  GetCurrentMemConf(MemConf);
  pCSF->SetStr("Machine","Mem_Bank_1",EasyStr(MemConf[0]));
  pCSF->SetStr("Machine","Mem_Bank_2",EasyStr(MemConf[1]));

  pCSF->SetStr("Machine","ShiftSwitching",Str(EnableShiftSwitching ? "1":"0"));
  pCSF->SetInt("Machine","KeyboardLanguage",(int)KeyboardLangID);

  pCSF->SetInt("Machine","ExMon",extended_monitor);
  pCSF->SetInt("Machine","ExMonWidth",em_width);
  pCSF->SetInt("Machine","ExMonHeight",em_height);
  pCSF->SetInt("Machine","ExMonPlanes",em_planes);

  pCSF->SetInt("Options","NewMemConf0",NewMemConf0);
  pCSF->SetInt("Options","NewMemConf1",NewMemConf1);
  pCSF->SetInt("Options","NewMonitorSel",NewMonitorSel);
  pCSF->SetStr("Options","NewROMFile",NewROMFile);

  pCSF->SetInt("Options","TOSSortDescend",eslTOS_Descend);
  pCSF->SetInt("Options","TOSSort",(int)eslTOS_Sort);

  pCSF->SetInt("Options","StartOnClick",StartEmuOnClick);

  pCSF->SetStr("Options","MacroDir",MacroDir);
  pCSF->SetStr("Options","ProfileDir",ProfileDir);
  pCSF->SetStr("Options","MacroSel",MacroSel);
  pCSF->SetStr("Options","ProfileSel",ProfileSel);

  pCSF->SetInt("Options","FSQuitAskFirst",FSQuitAskFirst);

  pCSF->SetStr("Options","LastIconPath",LastIconPath);
  pCSF->SetStr("Options","LastIconSchemePath",LastIconSchemePath);

  pCSF->SetInt("Options","OSDDiskLight",osd_show_disk_light);
  pCSF->SetInt("Options","OSDPlasma",osd_show_plasma);
  pCSF->SetInt("Options","OSDSpeed",osd_show_speed);
  pCSF->SetInt("Options","OSDIcons",osd_show_icons);
  pCSF->SetInt("Options","OSDCPU",osd_show_cpu);
  pCSF->SetInt("Options","OSDScroller",osd_show_scrollers);
  pCSF->SetInt("Options","OSDDisable",osd_disable);
  pCSF->SetInt("Options","OSDOldPos",osd_old_pos);

#ifdef UNIX
  for (int i=0;i<NUM_COMLINES;i++){
    pCSF->SetStr("Paths",Str("Path")+i,Comlines[i]);
  }
#endif

  return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int TShortcutBox::LoadShortcutInfo(DynamicArray<SHORTCUTINFO> &LoadCuts,EasyStringList &StringsESL,
                                      char *File,char *Sect)
{
  EasyStr ValName,MacFile;

  ConfigStoreFile CSF(File);
  int n=0;
  for (;;){
    SHORTCUTINFO si;
    ValName=EasyStr("Shortcut")+n++;
    si.Action=(BYTE)CSF.GetInt(Sect,ValName+"_Action",0xff);
    if (si.Action==0xff) break;
    si.Id[0]=(WORD)CSF.GetInt(Sect,ValName+"_ID1",0xffff);
    si.Id[1]=(WORD)CSF.GetInt(Sect,ValName+"_ID2",0xffff);
    si.Id[2]=(WORD)CSF.GetInt(Sect,ValName+"_ID3",0xffff);
    si.PressKey=(WORD)CSF.GetInt(Sect,ValName+"_Key",0xffff);
    si.PressChar=(DWORD)CSF.GetInt(Sect,ValName+"_Char",0xffff);
    si.MacroFileIdx=-1;
    MacFile=CSF.GetStr(Sect,ValName+"_MacroFile","");
    if (MacFile.NotEmpty()) si.MacroFileIdx=StringsESL.Add(MacFile);
    si.pESL=&StringsESL;
    si.Down=2;si.OldDown=2;
    LoadCuts.Add(si);
  }
  CSF.Close();

  UpdateDisableIfDownLists();
  return n;
}
//---------------------------------------------------------------------------
void TShortcutBox::SaveShortcutInfo(DynamicArray<SHORTCUTINFO> &SaveCuts,char *File)
{
  EasyStr ValName;

  ConfigStoreFile CSF(File);
  for (int n=0;n<SaveCuts.NumItems;n++){
    ValName=EasyStr("Shortcut")+n;
    CSF.SetStr("Shortcuts",ValName+"_ID1",EasyStr(SaveCuts[n].Id[0]));
    CSF.SetStr("Shortcuts",ValName+"_ID2",EasyStr(SaveCuts[n].Id[1]));
    CSF.SetStr("Shortcuts",ValName+"_ID3",EasyStr(SaveCuts[n].Id[2]));
    CSF.SetStr("Shortcuts",ValName+"_Action",EasyStr(SaveCuts[n].Action));
    CSF.SetStr("Shortcuts",ValName+"_Key",EasyStr(SaveCuts[n].PressKey));
    CSF.SetStr("Shortcuts",ValName+"_Char",EasyStr(SaveCuts[n].PressChar));
    if (SaveCuts[n].MacroFileIdx>=0){
      CSF.SetStr("Shortcuts",ValName+"_MacroFile",SaveCuts[n].pESL->Get(SaveCuts[n].MacroFileIdx).String);
    }else{
      CSF.SetStr("Shortcuts",ValName+"_MacroFile","");
    }
  }
  CSF.SetStr("Shortcuts",EasyStr("Shortcut")+SaveCuts.NumItems+"_Action",EasyStr(0xff));
  CSF.Close();
}
//---------------------------------------------------------------------------
void TShortcutBox::LoadAllCuts(bool NOT_ONEGAME(LoadCurrent))
{
#ifndef ONEGAME
  for (int cuts=0;cuts<2;cuts++){
    if (CurrentCutSelType!=2) cuts++;
    SHORTCUTINFO *pCuts=(SHORTCUTINFO*)((cuts==0) ? &(CurrentCuts[0]):&(Cuts[0]));
    int NumItems=int((cuts==0) ? CurrentCuts.NumItems:Cuts.NumItems);
    for (int n=0;n<NumItems;n++){
      if (pCuts[n].Down==1) DoShortcutUp(pCuts[n]);
    }
  }

  Cuts.DeleteAll();
  CutsStrings.DeleteAll();

  for (int i=0;i<CutFiles.NumStrings;i++){
    if (NotSameStr_I(CurrentCutSel,CutFiles[i].String) || IsVisible()==0){
      LoadShortcutInfo(Cuts,CutsStrings,CutFiles[i].String);
    }
  }
  if (LoadCurrent){
    CurrentCuts.DeleteAll();
    CurrentCutsStrings.DeleteAll();
    if (Handle && CurrentCutSelType>0) LoadShortcutInfo(CurrentCuts,CurrentCutsStrings,CurrentCutSel);
  }

  UpdateDisableIfDownLists();
#endif
}
//---------------------------------------------------------------------------
bool TShortcutBox::LoadData(bool NOT_ONEGAME( FirstLoad ),GoodConfigStoreFile *pCSF,bool *SecDisabled)
{
  SEC(PSEC_CUT){
    ScrollPos=pCSF->GetInt(Section,"ScrollPos0",ScrollPos);
    CurrentCutSel=pCSF->GetStr(Section,"CurrentCutSel",CurrentCutSel);
    CurrentCutSelType=pCSF->GetInt(Section,"CurrentCutSelType",CurrentCutSelType);
    CutDir=pCSF->GetStr(Section,"CutDir",WriteDir+SLASH+"shortcuts");
    NO_SLASH(CutDir);
    NOT_ONEGAME( bool NoCutDir=0; )
    if (GetFileAttributes(CutDir)==0xffffffff){
#ifndef ONEGAME
      CutDir=WriteDir+SLASH+T("shortcuts");
      CreateDirectory(CutDir,NULL);
      NoCutDir=true;
#else
      CutDir=WriteDir;
#endif
    }

    CutFiles.DeleteAll();
    int i=0;
    for (;;){
      Str File=pCSF->GetStr(Section,Str("SelectedCutFile")+(i++),"");
      if (File.Empty()) break;
      if (Exists(File)) CutFiles.Add(File);
    }

#ifndef ONEGAME
    if (FirstLoad){
      // Legacy, update to v2.5's new shortcuts system
      if (pCSF->GetInt("Shortcuts","Updated24Shortcuts",0)==0){
        Str OldCutDat=WriteDir+SLASH+"shortcuts.dat";
        if (Exists(OldCutDat)){
          ConfigStoreFile CSF(OldCutDat);
          if (CSF.GetInt("Shortcuts","Done25Update",0)==0){
            DynamicArray<SHORTCUTINFO> TempCuts;
            EasyStringList TempCutsStrings(eslNoSort);
            LoadShortcutInfo(TempCuts,TempCutsStrings,OldCutDat,"__Permanent__");

            Str NewFile=CutDir+SLASH+T("Main Shortcuts")+".stcut";int n=2;
            while (Exists(NewFile)) NewFile=CutDir+SLASH+T("Main Shortcuts")+" ("+(n++)+").stcut";
            SaveShortcutInfo(TempCuts,NewFile);
            CutFiles.Add(NewFile);
            CurrentCutSel=NewFile;
            CurrentCutSelType=2;

            EasyStringList SectList(eslNoSort);
            CSF.GetSectionNameList(&SectList);
            while ((n=SectList.FindString_I("__Permanent__"))>=0) SectList.Delete(n);
            while ((n=SectList.FindString_I("Shortcuts"))>=0) SectList.Delete(n);
            if (SectList.NumStrings){
              Str SetDir=CutDir+SLASH+T("Shortcut Sets");
              CreateDirectory(SetDir,NULL);
              for (int n=0;n<SectList.NumStrings;n++){
                TempCuts.DeleteAll();
                TempCutsStrings.DeleteAll();
                LoadShortcutInfo(TempCuts,TempCutsStrings,OldCutDat,SectList[n].String);

                RemoveIllegalFromName(SectList[n].String);
                NewFile=SetDir+SLASH+SectList[n].String+".stcut";int i=2;
                while (Exists(NewFile)) NewFile=SetDir+SLASH+SectList[n].String+" ("+(i++)+").stcut";
                SaveShortcutInfo(TempCuts,NewFile);
              }
              NewFile=CSF.GetStr("Shortcuts","CurrentGame","");
              RemoveIllegalFromName(NewFile);
              if (NewFile.NotEmpty()) CutFiles.Add(SetDir+SLASH+NewFile+".stcut");
            }
            CSF.SetInt("Shortcuts","Done25Update",1);
            CSF.Close();

            NoCutDir=0;
          }
          pCSF->SetInt("Shortcuts","Updated24Shortcuts",1);
        }
      }

      // Default shortcuts if first run from this dir
      if (NoCutDir){
        DynamicArray<SHORTCUTINFO> TempCuts;
        SHORTCUTINFO si;
#ifdef UNIX
        int VK_PRIOR=XKeysymToKeycode(XD,XK_Page_Up);
        int VK_NEXT=XKeysymToKeycode(XD,XK_Page_Down);
        if (VK_PRIOR && VK_NEXT)
#endif
        {
          ClearSHORTCUTINFO(&si); si.Id[0]=VK_PRIOR, si.Action=CUT_PRESSKEY, si.PressKey=VK_PRIOR; TempCuts.Add(si);
          ClearSHORTCUTINFO(&si); si.Id[0]=VK_NEXT,  si.Action=CUT_PRESSKEY, si.PressKey=VK_NEXT;  TempCuts.Add(si);
        }
        ClearSHORTCUTINFO(&si); si.Id[0]=VK_F11,   si.Action=CUT_PRESSKEY, si.PressKey=VK_F11;   TempCuts.Add(si);
        ClearSHORTCUTINFO(&si); si.Id[0]=VK_F12,   si.Action=CUT_PRESSKEY, si.PressKey=VK_F12;   TempCuts.Add(si);
        ClearSHORTCUTINFO(&si); si.Id[0]=VK_END,   si.Action=CUT_TAKESCREENSHOT,                 TempCuts.Add(si);

        Str NewFile=CutDir+SLASH+T("Default")+".stcut";
        SaveShortcutInfo(TempCuts,NewFile);
        CutFiles.Add(NewFile);
        CurrentCutSel=NewFile;
        CurrentCutSelType=2;
      }
    }
#endif

    LoadAllCuts(true);
    UPDATE;
  }

  return true;
}
//---------------------------------------------------------------------------
bool TShortcutBox::SaveData(bool FinalSave,ConfigStoreFile *pCSF)
{
  SavePosition(FinalSave,pCSF);

#ifdef WIN32
  pCSF->SetStr(Section,"ScrollPos0",EasyStr(ScrollPos));
#endif

  pCSF->SetStr(Section,"CurrentCutSel",CurrentCutSel);
  pCSF->SetInt(Section,"CurrentCutSelType",CurrentCutSelType);
  pCSF->SetStr(Section,"CutDir",CutDir);

  for (int i=0;i<CutFiles.NumStrings;i++) pCSF->SetStr(Section,Str("SelectedCutFile")+i,CutFiles[i].String);
  pCSF->SetStr(Section,Str("SelectedCutFile")+CutFiles.NumStrings,"");

  return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TGeneralInfo::LoadData(bool FirstLoad,GoodConfigStoreFile *pCSF,bool*)
{
  if (FirstLoad){
    LoadPosition(pCSF);

    WIN_ONLY( SearchText=pCSF->GetStr(Section,"SearchText",SearchText);)
    Page=pCSF->GetInt(Section,"Page",Page);

    if (pCSF->GetInt("GeneralInfo","Visible",0)) Show();
  }

  return true;
}
//---------------------------------------------------------------------------
bool TGeneralInfo::SaveData(bool FinalSave,ConfigStoreFile *pCSF)
{
  SavePosition(FinalSave,pCSF);

  WIN_ONLY( pCSF->SetStr(Section,"SearchText",SearchText); )
  pCSF->SetInt(Section,"Page",Page);

  return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TPatchesBox::LoadData(bool,GoodConfigStoreFile *pCSF,bool *SecDisabled)
{
  SEC(PSEC_PATCH){
    SelPatch=pCSF->GetStr(Section,"SelPatch",SelPatch);
    PatchDir=pCSF->GetStr(Section,"PatchDir",RunDir+SLASH "patches");
    NO_SLASH(PatchDir);
    if (GetFileAttributes(PatchDir)==0xffffffff){
      if (GetFileAttributes(RunDir+SLASH+"patches")!=0xffffffff){
        PatchDir=RunDir+SLASH+"patches";
      }
    }
    WIN_ONLY( SetButtonIcon(); )
    UPDATE;
  }

  return true;
}
//---------------------------------------------------------------------------
bool TPatchesBox::SaveData(bool FinalSave,ConfigStoreFile *pCSF)
{
  SavePosition(FinalSave,pCSF);

  pCSF->SetStr(Section,"SelPatch",SelPatch);
  pCSF->SetStr(Section,"PatchDir",PatchDir);

  return true;
}
//---------------------------------------------------------------------------
#undef SEC
#undef UPDATE

