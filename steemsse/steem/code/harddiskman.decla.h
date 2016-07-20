#pragma once
#ifndef HARDDISKMAN_DECLA_H //BCC
#define HARDDISKMAN_DECLA_H

#include <stemdialogs.decla.h>

#define MAX_HARDDRIVES 10

typedef struct{
  char Letter;
  EasyStr Path;
}Hard_Disk_Info;

class THardDiskManager : public TStemDialog
{
private:
#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  void ManageWindowClasses(bool);

  HWND Focus;
#elif defined(UNIX)
	static int WinProc(THardDiskManager*,Window,XEvent*);
	static int button_notify_proc(hxc_button*,int,int*);
	void RemoveLine(int);
		
	hxc_dropdown drive_dd[MAX_HARDDRIVES];
	hxc_edit drive_ed[MAX_HARDDRIVES];
	hxc_button drive_browse_but[MAX_HARDDRIVES],drive_open_but[MAX_HARDDRIVES],drive_remove_but[MAX_HARDDRIVES];
	hxc_button all_off_but,new_but,boot_label,ok_but,cancel_but;
	hxc_dropdown boot_dd;
#endif
public:
  THardDiskManager();
  ~THardDiskManager() { Hide(); }
  void Show(),Hide();
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);

  bool IsMountedDrive(char);
  EasyStr GetMountedDrivePath(char);
  void update_mount();
  bool NewDrive(char *);
  void CreateDriveControls(int);
  void SetWindowHeight();
  void GetDriveInfo();

  bool ApplyChanges;

  Hard_Disk_Info *OldDrive;
  int nOldDrives;
  bool OldDisableHardDrives;

  Hard_Disk_Info Drive[MAX_HARDDRIVES];
  int nDrives;
  bool DisableHardDrives;
#if defined(SSE_TOS_GEMDOS_RESTRICT_TOS2) // warning
  void CheckTos();
#endif
#if defined(SSE_ACSI_HDMAN)
  bool acsi; // for RTTI, booh!
#endif
};


#if defined(SSE_ACSI_HDMAN)
class TAcsiHardDiskManager : public THardDiskManager {
public:
  TAcsiHardDiskManager();
  void CheckTos() {}
  bool LoadData(bool FirstLoad,GoodConfigStoreFile *pCSF,bool *SecDisabled);
  bool SaveData(bool FinalSave,ConfigStoreFile *pCSF);
};
#endif

extern THardDiskManager HardDiskMan; // singleton defined in main.cpp
#if defined(SSE_ACSI_HDMAN)
extern TAcsiHardDiskManager AcsiHardDiskManager;
#endif

#endif//#ifndef HARDDISKMAN_DECLA_H