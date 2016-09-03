#pragma once
#ifndef GUI_DECLA_H
#define GUI_DECLA_H

#define EXT extern
#define INIT(s)

/*

#include <mymisc.h>
#include <easycompress.h>
#include <easystringlist.h>
#include <portio.h>
#include <dynamicarray.h>
#include <dirsearch.h>

#if USE_PASTI
#include <pasti/pasti.h>
#endif
*/
#include <easystr.h>//SSESTF.obj
typedef EasyStr Str;//SSESTF.obj
#include <configstorefile.h>
#include "resnum.decla.h"




#ifndef ONEGAME
#define MENUHEIGHT 20
#else
#define MENUHEIGHT 0
#endif


#include "SSE/SSEDecla.h"
#include "SSE/SSEParameters.h"

#define PEEKED_MESSAGE 0
#define PEEKED_QUIT 1
#define PEEKED_NOTHING 2
#define PEEKED_RUN 3

#if defined(VC_BUILD)
extern "C" int ASMCALL  PeekEvent();
#else
extern "C" ASMCALL int PeekEvent();
#endif

#ifdef SSE_UNIX//temp
UNIX_ONLY( extern void PostRunMessage(); )
#else
UNIX_ONLY( EXT void PostRunMessage(); )
#endif

#if defined(SSE_GUI_STATUS_STRING)
#if defined(SSE_GUI_STATUS_STRING_ICONS)
  void GUIRefreshStatusBar(bool invalidate=true);
#else
  void GUIRefreshStatusBar();
#endif
#endif
#define EXT extern//SSESTF.obj-tmp
#define INIT(s)//SSESTF.obj-tmp
EXT void GUIRunStart(),GUIRunEnd();
EXT int Alert(char *,char *,UINT);
EXT void QuitSteem();
EXT void fast_forward_change(bool,bool);
EXT bool GUIPauseWhenInactive();
EXT void PasteVBL(),StemWinResize(int DEFVAL(0),int DEFVAL(0));
EXT void GUIDiskErrorEject(int);
EXT bool GUICanGetKeys();
EXT void GUIColdResetChangeSettings();
EXT void GUISaveResetBackup();
EXT void CheckResetIcon(),CheckResetDisplay(bool=0);
EXT void UpdateSTKeys();
EXT void GUIEmudetectCreateDisk(Str,int,int,int);

#define STPASTE_TOGGLE 0
#define STPASTE_START 1
#define STPASTE_STOP 2
EXT void PasteIntoSTAction(int);

EXT int DoSaveScreenShot INIT(0);
EXT bool ResChangeResize INIT(true),CanUse_400 INIT(0);
EXT bool bAppActive INIT(true),bAppMinimized INIT(0);
EXT DWORD DisableDiskLightAfter INIT(3000);

#define IGNORE_EXTEND 2
#define NO_SHIFT_SWITCH 8
#if defined(SSE_VS2008_WARNING_383)
EXT void HandleKeyPress(UINT,DWORD,int DEFVAL(IGNORE_EXTEND));
#else
EXT void HandleKeyPress(UINT,bool,int DEFVAL(IGNORE_EXTEND));
#endif
EXT LANGID KeyboardLangID INIT(0);

#define STEM_MOUSEMODE_DISABLED 0
#define STEM_MOUSEMODE_WINDOW 1
#define STEM_MOUSEMODE_BREAKPOINT 3141

EXT int stem_mousemode INIT(STEM_MOUSEMODE_DISABLED);
EXT int window_mouse_centre_x,window_mouse_centre_y;
EXT bool TaskSwitchDisabled INIT(0);
EXT Str ROMFile,CartFile;

EXT bool FSQuitAskFirst INIT(true),Quitting INIT(0);
EXT bool FSDoVsync INIT(0);

#define LVI_SI_CHECKED (1 << 13)
#define LVI_SI_UNCHECKED (1 << 12)
#define PROFILESECT_ON LVI_SI_CHECKED
#define PROFILESECT_OFF LVI_SI_UNCHECKED

EXT int ExternalModDown INIT(0);
EXT bool comline_allow_LPT_input INIT(0);

#define RC_FLAG_WIDTH 16
#define RC_FLAG_HEIGHT 12

#if defined(UNIX)

EXT void XGUIUpdatePortDisplay();
EXT short GetKeyState(int);
#define GetAsyncKeyState GetKeyState
EXT void GUIUpdateInternalSpeakerBut();
EXT void GetCursorPos(POINT *);
EXT void SetCursorPos(int,int);

typedef struct{
  bool LShift,RShift;
  bool LCtrl,RCtrl;
  bool LAlt,RAlt;
}MODIFIERSTATESTRUCT;

EXT MODIFIERSTATESTRUCT GetLRModifierStates();

#define MB_OK 0x00000000L
#define MB_OKCANCEL 0x00000001L
#define MB_ABORTRETRYIGNORE 0x00000002L
#define MB_YESNOCANCEL 0x00000003L
#define MB_YESNO 0x00000004L
#define MB_RETRYCANCEL 0x00000005L

#define MB_ICONHAND 0x00000010L
#define MB_ICONQUESTION 0x00000020L
#define MB_ICONEXCLAMATION 0x00000030L
#define MB_ICONASTERISK 0x00000040L
#define MB_USERICON 0x00000080L
#define MB_ICONWARNING MB_ICONEXCLAMATION
#define MB_ICONERROR MB_ICONHAND
#define MB_ICONINFORMATION MB_ICONASTERISK
#define MB_ICONSTOP MB_ICONHAND

#define MB_DEFBUTTON1 0x00000000L
#define MB_DEFBUTTON2 0x00000100L
#define MB_DEFBUTTON3 0x00000200L
#define MB_DEFBUTTON4 0x00000300L

#define MB_APPLMODAL 0x00000000L
#define MB_SYSTEMMODAL 0x00001000L
#define MB_TASKMODAL 0x00002000L

#define MB_HELP 0x00004000L
#define MB_NOFOCUS 0x00008000L
#define MB_SETFOREGROUND 0x00010000L
#define MB_DEFAULT_DESKTOP_ONLY 0x00020000L
#define MB_TOPMOST 0x00040000L
#define MB_RIGHT 0x00080000L

#define MB_TYPEMASK 0x0000000FL
#define MB_ICONMASK 0x000000F0L
#define MB_DEFMASK 0x00000F00L
#define MB_MODEMASK 0x00003000L
#define MB_MISCMASK 0xFFFFC000L

#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7

#endif
//---------------------------------------------------------------------------
//#ifdef IN_MAIN

#define SHORTCUTS_TIMER_ID 2000
#define DISPLAYCHANGE_TIMER_ID 2100
#define MIDISYSEX_TIMER_ID 2200

void LoadAllIcons(ConfigStoreFile *,bool=0);

int GetComLineArgType(char *,EasyStr &);

// Flags
#define ARG_UNKNOWN 0
#define ARG_GDI 1
#define ARG_NODS 2
#define ARG_WINDOW 3
#define ARG_NOLPT 4
#define ARG_NOCOM 5
#define ARG_NOSHM 6
#define ARG_QUITQUICKLY 7
#define ARG_SOUNDCLICK 8
#define ARG_HELP 9
#define ARG_FULLSCREEN 10
#define ARG_DOUBLECHECKSHORTCUTS 11
#define ARG_DONTLIMITSPEED 12
#define ARG_EXACTSPEEDLIMITNONE 13
#define ARG_EXACTSPEEDLIMITTICK 14
#define ARG_EXACTSPEEDLIMITHP 15
#define ARG_EXACTSPEEDLIMITSLEEPTIME 16
#define ARG_EXACTSPEEDLIMITSLEEPHP 17
#define ARG_ACCURATEFDC 18
#define ARG_NOPCJOYSTICKS 19
#define ARG_OLDPORTIO 20
#define ARG_ALLOWREADOPEN 21
#define ARG_NOINTS 22
#define ARG_STFMBORDER 23
#define ARG_SCREENSHOTUSEFULLNAME 24
#define ARG_ALLOWLPTINPUT 25
#define ARG_NONOTIFYINIT 26
#define ARG_SCREENSHOTALWAYSADDNUM 27
#define ARG_PSGCAPTURE 28
#define ARG_CROSSMOUSE 29
#define ARG_RUN 30
#define ARG_GDIFSBORDER 31
#define ARG_PASTI 32
#define ARG_NOAUTOSNAPSHOT 33
#define ARG_NOPASTI 34

// Settings
#define ARG_SETSOF 100
#define ARG_SETINIFILE 101
#define ARG_SETTRANSFILE 102
#define ARG_SETFONT 103
#define ARG_SETCUTSFILE 104
#if !defined(SSE_CPU)
#define ARG_SETDIVUTIME 105
#define ARG_SETDIVSTIME 106
#endif
#define ARG_TAKESHOT 107
#define ARG_SETPABUFSIZE 108
#define ARG_RTBUFSIZE 109
#define ARG_RTBUFNUM 110

// Files
#define ARG_DISKIMAGEFILE 201
#define ARG_SNAPSHOTFILE 202
#define ARG_CARTFILE 203
#define ARG_STPROGRAMFILE 204
#define ARG_STPROGRAMTPFILE 205
#define ARG_LINKFILE 206
#define ARG_TOSIMAGEFILE 207
#define ARG_PASTIDISKIMAGEFILE 208

#define ARG_NONEWINSTANCE 250
#define ARG_ALWAYSNEWINSTANCE 251

#if defined(SSE_UNIX_TRACE)
#define ARG_LOGSECTION 252
#define ARG_TRACEFILE 253
#endif

void ParseCommandLine(int,char*[],int=0);

bool MakeGUI();

void SetStemWinSize(int,int,int=0,int=0);
void SetStemMouseMode(int);

#define MSW_NOCHANGE int(0x7fff)
void MoveStemWin(int,int,int,int);

int GetScreenWidth(),GetScreenHeight();

void ShowAllDialogs(bool);
void slow_motion_change(bool);

EXT bool RunMessagePosted;

EXT BYTE KeyDownModifierState[256];

void ShiftSwitchChangeModifiers(bool,bool,int[]);
void ShiftSwitchRestoreModifiers(int[]);
#if defined(SSE_VS2008_WARNING_383)
void HandleShiftSwitching(UINT,DWORD,BYTE&,int[]);
#else
void HandleShiftSwitching(UINT,bool,BYTE&,int[]);
#endif
Str SnapShotGetLastBackupPath();
void SnapShotGetOptions(EasyStringList*);

EXT int PasteVBLCount,PasteSpeed;
EXT Str PasteText;
EXT bool StartEmuOnClick;
//---------------------------------------------------------------------------
#ifdef WIN32
LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT __stdcall FSClipWndProc(HWND,UINT,WPARAM,LPARAM);
LRESULT __stdcall FSQuitWndProc(HWND,UINT,WPARAM,LPARAM);

EXT HWND FSQuitBut;

EXT HICON hGUIIcon[RC_NUM_ICONS],hGUIIconSmall[RC_NUM_ICONS];

inline bool HandleMessage(MSG*);
void EnableAllWindows(bool,HWND);

void fast_forward_change(bool,bool);
void HandleButtonMessage(UINT,HWND);
void DisableTaskSwitch();
void EnableTaskSwitch();

bool IsSteemAssociated(EasyStr);
#if defined(SSE_VS2008_WARNING_383) && defined(SSE_GUI_ASSOCIATE_CU)
void AssociateSteem(EasyStr,EasyStr);
#else
void AssociateSteem(EasyStr,EasyStr,bool,char *,int,bool);
#endif
void UpdatePasteButton();

EXT HWND StemWin,ParentWin,ToolTip,DisableFocusWin,UpdateWin;
EXT HMENU StemWin_SysMenu;
EXT HFONT fnt;
EXT HCURSOR PCArrow;
EXT COLORREF MidGUIRGB,DkMidGUIRGB;
EXT HANDLE SteemRunningMutex;

EXT bool WinNT;
EXT bool AllowTaskSwitch;
EXT HHOOK hNTTaskSwitchHook;
EXT HWND NextClipboardViewerWin;

#define PostRunMessage()  if (RunMessagePosted==0){ \
                            SendDlgItemMessage(StemWin,101,BM_SETCLICKBUTTON,1,0); \
                            PostMessage(StemWin,WM_COMMAND,MAKEWPARAM(101,BN_CLICKED),(LPARAM)GetDlgItem(StemWin,101)); \
                            RunMessagePosted=true; \
                          }
//---------------------------------------------------------------------------
#elif defined(UNIX)
//---------------------------------------------------------------------------

EXT XErrorEvent XError;
int HandleXError(Display*,XErrorEvent*);

int StemWinProc(void*,Window,XEvent*);
int timerproc(void*,Window,int);

#include "../../include/x/hxc.h"
#include "../../include/x/hxc_popup.h"
#include "../../include/x/hxc_popuphints.h"
#include "../../include/x/hxc_alert.h"
#include "../../include/x/hxc_fileselect.h"


int hyperlink_np(hxc_button*,int,int*);

void steem_hxc_modal_notify(bool);

EXT hxc_popup pop;
EXT hxc_popuphints hints;

int ProcessEvent(XEvent *);
void InitColoursAndIcons();

void steem_hxc_alloc_colours(Display*);
void steem_hxc_free_colours(Display*);

EXT Window StemWin;
EXT GC DispGC;
EXT Cursor EmptyCursor;
EXT Atom RunSteemAtom,LoadSnapShotAtom;
EXT XID SteemWindowGroup;
EXT DWORD BlackCol,WhiteCol,BkCol,BorderLightCol,BorderDarkCol;
EXT hxc_alert alert;
//XFontStruct *GUIFont=NULL,*SmallFont=NULL;

void PrintHelpToStdout();
#define GetLongPathName(from,to,len) if (to!=from){strncpy(to,from,len);to[len-1]=0;}
bool SetForegroundWindow(Window,Time=CurrentTime);
Window GetForegroundWindow();
void CentreWindow(Window,bool);
bool GetWindowPositionData(Window,WINPOSITIONDATA *);

EXT short KeyState[256];
void SetKeyState(int,bool,bool=false);
short GetKeyStateSym(KeySym);

int MessageBox(WINDOWTYPE,char *,char *,UINT);

typedef int (BUTNOTIFYPROC)(Window,int,int,int*);
typedef BUTNOTIFYPROC* LPBUTNOTIFYPROC;
int StemWinButtonNotifyProc(Window,int,int,int *);

void SnapShotProcess(int);
//SS check icon16.bmp, there they are!
#define ICO16_FOLDER 0
#define ICO16_PARENTDIRECTORY 1
#define ICO16_EXCLAMATION 2
#define ICO16_QUESTION 3
#define ICO16_STOP 4
#define ICO16_INFO 5
#define ICO16_FF 6
#define ICO16_FORWARD 7
#define ICO16_HARDDRIVE 8
#define ICO16_HOMEFOLDER 9
#define ICO16_GENERALINFO 10
#define ICO16_JOY 11
#define ICO16_PORTS 12
#define ICO16_OSD 13
#define ICO16_STEEM 14
#define ICO16_INSERTDISK 15
#define ICO16_RESET 16
#define ICO16_RUN 17
#define ICO16_SETHOMEFOLDER 18
#define ICO16_CUT 19
#define ICO16_SNAPSHOTS 20
#define ICO16_SNAPSHOT 20
#define ICO16_EJECTDISK 21
#define ICO16_OPTIONS 22
#define ICO16_BACK 23
#define ICO16_STCONFIG 24
#define ICO16_CHIP 24
#define ICO16_CART 24
#define ICO16_SOUND 25
#define ICO16_DISKMAN 26
#define ICO16_DISK 26
#define ICO16_JOYDIR 27
#define ICO16_ZIP_RO 28
#define ICO16_PATCHES 29
#define ICO16_TOOLS ICO16_OPTIONS
#define ICO16_DISPLAY 32
#define ICO16_BRIGHTCON 31
#define ICO16_LINKS 33
#define ICO16_README 34
#define ICO16_FAQ 35
#define ICO16_DRAWSPEED 36
#define ICO16_PASTE 37
#define ICO16_FUJI16 38
#define ICO16_ACCURATEFDC 39
#define ICO16_MIDI 40
#define ICO16_CUTON 41
#define ICO16_CUTOFF 42
#define ICO16_PATCHESNEW 43
#define ICO16_RESETGLOW 44
#define ICO16_PROFILE 45
#define ICO16_ST 46
#define ICO16_MACROS 47
#define ICO16_TICKED 48
#define ICO16_RADIOMARK 49
#define ICO16_DISKMANMENU 50
#define ICO16_FOLDERLINK 51
#define ICO16_FOLDERLINKBROKEN 52
#define ICO16_DISKLINK 53
#define ICO16_DISKLINKBROKEN 54
#define ICO16_CUTONLINK 55
#define ICO16_CUTOFFLINK 56
#define ICO16_ZIP_RW 57
#define ICO16_DISK_RO 58
#define ICO16_PROFILELINK 59
#define ICO16_MACROLINK 60
#define ICO16_UNTICKED 61
#define ICO16_FULLSCREEN 62
#define ICO16_TAKESCREENSHOTBUT 63
#define ICO16_UNRADIOMARKED 64

#if defined(SSE_UNIX_OPTIONS_SSE_ICON)
#define ICO16_SSE_OPTION 65 // must be in icon16.bmp
#endif

#define ICO32_JOYDIR 0
#define ICO32_RECORD 1
#define ICO32_DRIVE_A 2
#define ICO32_DRIVE_B 3
#define ICO32_LINKCUR 4
#define ICO32_PLAY 5
#define ICO32_DRIVE_B_OFF 6
#define ICO32_EXCLAM 7
#define ICO32_QUESTION 8
#define ICO32_STOP 9
#define ICO32_INFO 10

#define ICO64_STEEM 0
#define ICO64_HARDDRIVES 1
#define ICO64_HARDDRIVES_FR 2

extern "C" LPBYTE Get_icon16_bmp(),Get_icon32_bmp(),Get_icon64_bmp(),Get_tos_flags_bmp();
EXT IconGroup Ico16,Ico32,Ico64,IcoTOSFlags;
EXT Pixmap StemWinIconPixmap,StemWinIconMaskPixmap;

EXT hxc_button RunBut,FastBut,ResetBut,SnapShotBut,ScreenShotBut,PasteBut,FullScreenBut;
EXT hxc_button InfBut,PatBut,CutBut,OptBut,JoyBut,DiskBut;

#define FF_DOUBLECLICK_MS 200

EXT DWORD ff_doubleclick_time;

int StemWinButtonNotifyProc(hxc_button*,int,int*);

inline bool SetProp(Window Win,XContext Prop,DWORD Val)
{
  return SetProp(XD,Win,Prop,Val);
}

inline DWORD GetProp(Window Win,XContext Prop)
{
  return GetProp(XD,Win,Prop);
}

inline DWORD RemoveProp(Window Win,XContext Prop)
{
  return RemoveProp(XD,Win,Prop);
}

int romfile_parse_routine(char*fn,struct stat*s);

int diskfile_parse_routine(char *fn,struct stat *s);

int wavfile_parse_routine(char *fn,struct stat *s);

int folder_parse_routine(char *fn,struct stat *s);

int cartfile_parse_routine(char *fn,struct stat*s);

EXT hxc_fileselect fileselect;

#define COMLINE_HTTP 0
#define COMLINE_FTP 1
#define COMLINE_MAILTO 2
#define COMLINE_FM 3
#define COMLINE_FIND 4

#define NUM_COMLINES 5

EXT char* Comlines_Default[NUM_COMLINES][8];

EXT Str Comlines[NUM_COMLINES];

#endif //win32,unix

bool load_cart(char*);
void CleanUpSteem();
EXT bool StepByStepInit;
EXT EasyStr RunDir,WriteDir,INIFile,ScreenShotFol;
EXT EasyStr LastSnapShot,BootStateFile,StateHist[10],AutoSnapShotName;
#if defined(SSE_GUI_CONFIG_FILE)
EXT EasyStr LastCfgFile;
#endif
EXT Str BootDisk[2];

#define BOOT_PASTI_DEFAULT 0
#define BOOT_PASTI_ON 1
#define BOOT_PASTI_OFF 2

EXT int BootPasti;
EXT bool PauseWhenInactive,BootTOSImage;
EXT bool bAOT,bAppMaximized;
#ifndef ONEGAME
EXT bool AutoLoadSnapShot,ShowTips;
#else
EXT bool AutoLoadSnapShot,ShowTips;
#endif
EXT bool AllowLPT,AllowCOM;
EXT bool HighPriority;

#define BOOT_MODE_DEFAULT 0
#define BOOT_MODE_FULLSCREEN 1
#define BOOT_MODE_WINDOW 2
#define BOOT_MODE_FLAGS_MASK 0xff
#define BOOT_MODE_RUN 0x100
EXT int BootInMode;

char *FSTypes(int,...);

EXT bool NoINI;

EXT const POINT WinSize[4][5];

#if defined(SSE_VID_BORDERS)

EXT  POINT WinSizeBorderOriginal[4][5];
#if !defined(SSE_VID_D3D_ONLY)
EXT  POINT WinSizeBorderLarge[4][5];
#endif
//EXT  POINT WinSizeBorderLarge2[4][5];

EXT  POINT WinSizeBorderVeryLarge[4][5];

#if defined(SSE_VID_BORDERS_416)
EXT  POINT WinSizeBorderVeryLarge2[4][5];
#endif

EXT  POINT WinSizeBorder[4][5];

#else

EXT const POINT WinSizeBorder[4][5];

#endif

EXT int WinSizeForRes[4]; // SS: for resolutions 0,1,2 & 3("crazy")

EXT RECT rcPreFS;

void flashlight(bool);

EXT BYTE PCCharToSTChar[128];

EXT BYTE STCharToPCChar[128];


//#endif

#undef EXT
#undef INIT

#endif//GUI_DECLA_H
