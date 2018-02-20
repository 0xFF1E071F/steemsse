#pragma once
#ifndef SHORTCUTBOX_DECLA_H
#define SHORTCUTBOX_DECLA_H

#define EXT extern
#define INIT(s)

EXT void ShortcutsCheck();
EXT bool CutDisableKey[256];
#define SHORTCUT_VBLS_BETWEEN_CHECKS 3
EXT int shortcut_vbl_count INIT(0),cut_slow_motion_speed INIT(0);
EXT DWORD CutPauseUntilSysEx_Time INIT(0);
EXT int CutModDown INIT(0);

#ifdef IN_MAIN //TODO

UNIX_ONLY( extern "C" BYTE *Get_st_charset_bmp(); )


#if defined(SSE_COMPILER_STRUCT_391)

#pragma pack(push, STRUCTURE_ALIGNMENT)

typedef struct TYPE_SHORTCUTINFO{
  TYPE_SHORTCUTINFO* DisableIfCutDownList[5];
  EasyStringList *pESL;
  DWORD PressChar;
  WORD Id[3],PressKey;
  int MacroFileIdx;
  BYTE OldDown,Down,Action;
}SHORTCUTINFO;

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#else

typedef struct TYPE_SHORTCUTINFO{
  WORD Id[3],PressKey;
  // PressChar=LOWORD(STKeyCode,Modifiers (bit 0=shift,bit 1=alt)),HIWORD(ST ascii code)
  DWORD PressChar;
  BYTE OldDown,Down,Action;
  TYPE_SHORTCUTINFO* DisableIfCutDownList[5];
  int MacroFileIdx;
  EasyStringList *pESL;
}SHORTCUTINFO;

#endif//#if defined(SSE_COMPILER_STRUCT_391)


EXT DynamicArray<SHORTCUTINFO> Cuts,CurrentCuts;
EXT EasyStringList CutsStrings;//(eslNoSort);
EXT EasyStringList CurrentCutsStrings;//(eslNoSort);

void ClearSHORTCUTINFO(SHORTCUTINFO *pSI);

EXT EasyStringList CutFiles;

void DoShortcutDown(SHORTCUTINFO &),DoShortcutUp(SHORTCUTINFO &);

EXT bool CutDisableAxis[MAX_PC_JOYS][20],CutDisablePOV[MAX_PC_JOYS][9];
EXT DWORD CutButtonMask[MAX_PC_JOYS];
EXT int MouseWheelMove;
EXT bool CutButtonDown[2];
//---------------------------------------------------------------------------
#define BPS_NOJOY 1
#define BPS_INSHORTCUT 2

#ifdef UNIX
typedef struct{
	hxc_button *p_sign[3],*p_del,*p_macro;
	hxc_buttonpicker *p_id[3],*p_stkey;
	hxc_dropdown *p_action,*p_stchar;
}PICKERLINE;
#endif


#if defined(SSE_COMPILER_STRUCT_391)

#pragma pack(push, STRUCTURE_ALIGNMENT)

class TShortcutBox : public TStemDialog
{
public: //TODO
  void AddPickerLine(int);
  void UpdateAddButsPosition();
  void TranslateCutNames();
  void ChangeCutFile(Str,int,bool);

  EasyStringList TranslatedCutNamesSL;
  Str CutDir,CurrentCutSel;

#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall ChooseMacroWndProc(HWND,UINT,WPARAM,LPARAM);
  static int DTreeNotifyProc(DirectoryTree*,void*,int,int,int);
  static int ChooseMacroTreeNotifyProc(DirectoryTree*,void*,int,int,int);
  void ManageWindowClasses(bool);
  void RefreshCutFileView();

  Str ShowChooseMacroBox(Str);
  void SetMacroFileButtonText(HWND,int);

  ScrollControlWin Scroller;
  HWND PopupFocus;
  Str ChooseMacroSel;
  int MenuItem;
  int CurrentCutSelType;
  int ScrollPos;
  static bool Picking;
  bool PopupOpen;
#elif defined(UNIX)
  static int WinProc(TShortcutBox*,Window,XEvent*);
	static int picker_notify_proc(hxc_buttonpicker*,int,int);
	static int button_notify_proc(hxc_button*,int,int*);
	static int dd_notify_proc(hxc_dropdown*,int,int);
	static int sa_notify_proc(hxc_scrollarea*,int,int);
	static int dir_lv_notify_proc(hxc_dir_lv*,int,int);
	void ShowHidePressSTKeyPicker(int);
	void LoadCutsAndCreateCutControls();
	void DeleteCut(int);
	PICKERLINE GetLine(int);
  void SetMacroFileButtonText(hxc_button*,int);

  Str ChooseMacro(Str);

  hxc_button sa_border,add_but[2],new_cut_but,change_fol_but;
  hxc_scrollarea sa;
  hxc_dir_lv dir_lv;

  IconGroup st_chars_ig;

	hxc_textdisplay help_td;
#endif
public:
  TShortcutBox();
  ~TShortcutBox() { Hide(); }
  void Show(),Hide();
  bool ToggleVisible(){ IsVisible() ? Hide():Show();return IsVisible(); }
  void UpdateDisableIfDownLists();
  void LoadAllCuts(bool=true);
  int LoadShortcutInfo(DynamicArray<SHORTCUTINFO>&,EasyStringList &,char*,char* ="Shortcuts");
  void SaveShortcutInfo(DynamicArray<SHORTCUTINFO>&,char*);
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);

#ifdef WIN32
  bool HasHandledMessage(MSG *);
  static HWND InfoWin;
  static DirectoryTree *pChooseMacroTree;
  static DirectoryTree DTree;
#elif defined(UNIX)
  int CurrentCutSelType;
  int ScrollPos;
  static bool Picking;
#endif

};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#else

class TShortcutBox : public TStemDialog
{
private:
  void AddPickerLine(int);
  void UpdateAddButsPosition();
  void TranslateCutNames();
  void ChangeCutFile(Str,int,bool);

  EasyStringList TranslatedCutNamesSL;

#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall ChooseMacroWndProc(HWND,UINT,WPARAM,LPARAM);
  static int DTreeNotifyProc(DirectoryTree*,void*,int,int,int);
  static int ChooseMacroTreeNotifyProc(DirectoryTree*,void*,int,int,int);
  void ManageWindowClasses(bool);
  void RefreshCutFileView();

  Str ShowChooseMacroBox(Str);
  void SetMacroFileButtonText(HWND,int);

  ScrollControlWin Scroller;
  bool PopupOpen;
  HWND PopupFocus;
  Str ChooseMacroSel;
  int MenuItem;
#elif defined(UNIX)
  static int WinProc(TShortcutBox*,Window,XEvent*);
	static int picker_notify_proc(hxc_buttonpicker*,int,int);
	static int button_notify_proc(hxc_button*,int,int*);
	static int dd_notify_proc(hxc_dropdown*,int,int);
	static int sa_notify_proc(hxc_scrollarea*,int,int);
	static int dir_lv_notify_proc(hxc_dir_lv*,int,int);
	void ShowHidePressSTKeyPicker(int);
	void LoadCutsAndCreateCutControls();
	void DeleteCut(int);
	PICKERLINE GetLine(int);
  void SetMacroFileButtonText(hxc_button*,int);

  Str ChooseMacro(Str);

  hxc_button sa_border,add_but[2],new_cut_but,change_fol_but;
  hxc_scrollarea sa;
  hxc_dir_lv dir_lv;

  IconGroup st_chars_ig;

	hxc_textdisplay help_td;
#endif
public:
  TShortcutBox();
  ~TShortcutBox() { Hide(); }
  void Show(),Hide();
  bool ToggleVisible(){ IsVisible() ? Hide():Show();return IsVisible(); }
  void UpdateDisableIfDownLists();
  void LoadAllCuts(bool=true);
  int LoadShortcutInfo(DynamicArray<SHORTCUTINFO>&,EasyStringList &,char*,char* ="Shortcuts");
  void SaveShortcutInfo(DynamicArray<SHORTCUTINFO>&,char*);
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);

#ifdef WIN32
  bool HasHandledMessage(MSG *);

  static HWND InfoWin;
  static DirectoryTree *pChooseMacroTree;
  static DirectoryTree DTree;
#elif defined(UNIX)
#endif

  Str CutDir,CurrentCutSel;
  int CurrentCutSelType;
  static bool Picking;
  int ScrollPos;
};

#endif//#if defined(SSE_COMPILER_STRUCT_391)

OldJoystickPosition JoyOldPos[MAX_PC_JOYS];
#endif//main

#undef EXT
#undef INIT

#endif//#ifndef SHORTCUTBOX_DECLA_H
