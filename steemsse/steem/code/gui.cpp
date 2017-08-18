/*---------------------------------------------------------------------------
FILE: gui.cpp
MODULE: Steem
DESCRIPTION: This is a core file that has lots and lots of miscellaneous
GUI functions. It creates the main window in MakeGUI, handles translations
and (for some reason) command-line options.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: gui.cpp")
#endif

#if defined(SSE_BUILD)

#define EXT
#define INIT(s) =s

EXT int DoSaveScreenShot INIT(0);
#if defined(SSE_VID_AUTO_RESIZE)
const EXT bool ResChangeResize INIT(true);
EXT bool CanUse_400 INIT(0);
#else
EXT bool ResChangeResize INIT(true),CanUse_400 INIT(0);
#endif
EXT bool bAppActive INIT(true),bAppMinimized INIT(0);
EXT DWORD DisableDiskLightAfter INIT(3000);
EXT LANGID KeyboardLangID INIT(0);
EXT int stem_mousemode INIT(STEM_MOUSEMODE_DISABLED);
EXT int window_mouse_centre_x,window_mouse_centre_y;
EXT bool TaskSwitchDisabled INIT(0);
EXT Str ROMFile,CartFile;

EXT bool FSQuitAskFirst INIT(true),Quitting INIT(0);
EXT bool FSDoVsync INIT(0);
EXT int ExternalModDown INIT(0);
EXT bool comline_allow_LPT_input INIT(0);

bool RunMessagePosted=0;

BYTE KeyDownModifierState[256];
int PasteVBLCount=0,PasteSpeed=2;
#if defined(SSE_IKBD_6301_PASTE)
bool bPastingText=false;
#endif
Str PasteText;
bool StartEmuOnClick=0;

#ifdef WIN32
#if !defined(SSE_VID_FS_PROPER_QUIT_BUTTON)
EXT HWND FSQuitBut=NULL;
#endif
EXT HICON hGUIIcon[RC_NUM_ICONS],hGUIIconSmall[RC_NUM_ICONS];

HWND StemWin=NULL,ParentWin=NULL,ToolTip=NULL,DisableFocusWin=NULL,UpdateWin=NULL;
HMENU StemWin_SysMenu=NULL;
HFONT fnt;
HCURSOR PCArrow;
COLORREF MidGUIRGB,DkMidGUIRGB;
HANDLE SteemRunningMutex=NULL;

bool WinNT=0;
bool AllowTaskSwitch = NOT_ONEGAME(true) ONEGAME_ONLY(0);
HHOOK hNTTaskSwitchHook=NULL;
HWND NextClipboardViewerWin=NULL;

#elif defined(UNIX)

XErrorEvent XError;

hxc_popup pop;
hxc_popuphints hints;


Window StemWin=0;
GC DispGC=0;
Cursor EmptyCursor=0;
Atom RunSteemAtom,LoadSnapShotAtom;
XID SteemWindowGroup = 0;
DWORD BlackCol=0,WhiteCol=0,BkCol=0,BorderLightCol,BorderDarkCol;
hxc_alert alert;
//XFontStruct *GUIFont=NULL,*SmallFont=NULL;

  // SS: look how shitty C++ can be
short KeyState[256]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


extern "C" LPBYTE Get_icon16_bmp(),Get_icon32_bmp(),Get_icon64_bmp(),Get_tos_flags_bmp();
IconGroup Ico16,Ico32,Ico64,IcoTOSFlags;
Pixmap StemWinIconPixmap=0,StemWinIconMaskPixmap=0;

hxc_button RunBut,FastBut,ResetBut,SnapShotBut,ScreenShotBut,PasteBut,FullScreenBut;
hxc_button InfBut,PatBut,CutBut,OptBut,JoyBut,DiskBut;

DWORD ff_doubleclick_time=0;

int romfile_parse_routine(char*fn,struct stat*s)
{
  if (S_ISDIR(s->st_mode)) return FS_FTYPE_FOLDER;
  if (has_extension_list(fn,".IMG",".ROM",NULL)){
    return FS_FTYPE_FILE_ICON+ICO16_STCONFIG;
  }
  return FS_FTYPE_REJECT;
}

int diskfile_parse_routine(char *fn,struct stat *s)
{
  if (S_ISDIR(s->st_mode)) return FS_FTYPE_FOLDER;
  if (FileIsDisk(fn)){
	  return FS_FTYPE_FILE_ICON+ICO16_DISKMAN;	
  }
  return FS_FTYPE_REJECT;
}

int wavfile_parse_routine(char *fn,struct stat *s)
{
  if (S_ISDIR(s->st_mode)) return FS_FTYPE_FOLDER;
  if (has_extension(fn,".WAV")){
	  return FS_FTYPE_FILE_ICON+ICO16_SOUND;	
  }
  return FS_FTYPE_REJECT;
}

int folder_parse_routine(char *fn,struct stat *s)
{
  if (S_ISDIR(s->st_mode)) return FS_FTYPE_FOLDER;
  return FS_FTYPE_REJECT;
}

int cartfile_parse_routine(char *fn,struct stat*s)
{
  if (S_ISDIR(s->st_mode)) return FS_FTYPE_FOLDER;
  if (has_extension(fn,".STC")){
    if ((s->st_size)==128*1024+4){
      return FS_FTYPE_FILE_ICON+ICO16_CART;
    }
  }
  return FS_FTYPE_REJECT;
}

hxc_fileselect fileselect;

char* Comlines_Default[NUM_COMLINES][8]={
        {"netscape \"[URL]\"","konqueror \"[URL]\"","galeon \"[URL]\"","opera \"[URL]\"","firefox \"[URL]\"","mozilla \"[URL]\"",NULL},
        {"netscape \"[URL]\"","konqueror \"[URL]\"","galeon \"[URL]\"","opera \"[URL]\"","firefox \"[URL]\"","mozilla \"[URL]\"",NULL},
        {"netscape \"mailto:[ADDRESS]\"","mozilla \"mailto:[ADDRESS]\"","kmail \"[ADDRESS]\"","galeon \"mailto:[ADDRESS]\"",NULL},
        {"konqueror \"[PATH]\"","nautilus \"[PATH]\"","xfm \"[PATH]\"",NULL},
        {"kfind \"[PATH]\"","gnome-search-tool \"[PATH]\"",NULL}
        };

Str Comlines[NUM_COMLINES]={Comlines_Default[0][0],Comlines_Default[1][0],Comlines_Default[2][0],Comlines_Default[3][0],Comlines_Default[4][0]};

#endif //win32,unix

bool StepByStepInit=0;
EasyStr RunDir,WriteDir,INIFile,ScreenShotFol;
EasyStr LastSnapShot,BootStateFile,StateHist[10],AutoSnapShotName="auto";
#if defined(SSE_GUI_SNAPSHOT_INI)
EasyStr DefaultSnapshotFile;
#endif
#if defined(SSE_GUI_CONFIG_FILE)
EasyStr LastCfgFile;
#endif
Str BootDisk[2];

int BootPasti=BOOT_PASTI_DEFAULT;
bool PauseWhenInactive=0,BootTOSImage=0;
bool bAOT=0,bAppMaximized=0;
#ifndef ONEGAME
#if defined(SSE_GUI_SHOW_TIPS)
bool AutoLoadSnapShot=true;
const bool ShowTips=true;
#else
bool AutoLoadSnapShot=true,ShowTips=true;
#endif
#else
bool AutoLoadSnapShot=0,ShowTips=0;
#endif
bool AllowLPT=true,AllowCOM=true;
#if defined(SSE_NO_HIGH_PRIORITY)
const bool HighPriority=0;
#else
bool HighPriority=0;
#endif
int BootInMode=BOOT_MODE_DEFAULT;

bool NoINI;

const POINT WinSize[4][5]={ {{320,200},{640,400},{960, 600},{1280,800},{-1,-1}},
                            {{640,200},{640,400},{1280,400},{1280,800},{-1,-1}},
                            {{640,400},{1280,800},{-1,-1}},
                            {{800,600},{-1,-1}}};

#if defined(SSE_VID_BORDERS)

#if defined(SSE_VID_BORDERS_BIGTOP) && !defined(SSE_VID_BORDERS_416)
#error BIGTOP needs 416!
#endif

POINT WinSizeBorderOriginal[4][5]={ 
{{320+ORIGINAL_BORDER_SIDE*2,200+(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{640+(ORIGINAL_BORDER_SIDE*2)*2,400+2*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{960+(ORIGINAL_BORDER_SIDE*3)*2, 600+3*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{1280+(ORIGINAL_BORDER_SIDE*4)*2,800+4*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{-1,-1}},
{{640+(ORIGINAL_BORDER_SIDE*2)*2,200+(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{640+(ORIGINAL_BORDER_SIDE*2)*2,400+2*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{1280+(ORIGINAL_BORDER_SIDE*4)*2,400+2*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{1280+(ORIGINAL_BORDER_SIDE*4)*2,800+4*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{-1,-1}},
{{640+(ORIGINAL_BORDER_SIDE*2)*2,400+2*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{1280+(ORIGINAL_BORDER_SIDE*4)*2,800+4*(ORIGINAL_BORDER_TOP+ORIGINAL_BORDER_BOTTOM)},
{-1,-1}},
{{800,600},
{-1,-1}}
};


#if  defined(SSE_VID_BORDERS_413) //1 more horizontal pixel in low and med res
POINT WinSizeBorderLarge[4][5]={ 
{{320+VERY_LARGE_BORDER_SIDE_WIN*2+1,200+(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2+2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)}, 
{960+(VERY_LARGE_BORDER_SIDE_WIN*3)*2+3, 600+3*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2+4,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2+1,200+(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2+2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2+4,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2+4,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{800,600},
{-1,-1}}
};
#elif defined(SSE_VID_BORDERS_412) 
POINT WinSizeBorderLarge[4][5]={ 
{{320+VERY_LARGE_BORDER_SIDE_WIN*2,200+(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)}, 
{960+(VERY_LARGE_BORDER_SIDE_WIN*3)*2, 600+3*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2,200+(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE_WIN*2)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE_WIN*4)*2,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{800,600},
{-1,-1}}
};
#endif


#if defined(SSE_VID_BORDERS_416)

#if defined(SSE_VID_BORDERS_BIGTOP) // 416*286
POINT WinSizeBorderMax[4][5]={ 
{{320+VERY_LARGE_BORDER_SIDE*2,200+(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE*2)*2,400+2*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)}, 
{960+(VERY_LARGE_BORDER_SIDE*3)*2, 600+3*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,800+4*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE*2)*2,200+(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE*2)*2,400+2*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,400+2*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,800+4*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE*2)*2,400+2*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,800+4*(BIG_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{800,600},
{-1,-1}}
};
#else // 416*280
POINT WinSizeBorderMax[4][5]={ 
{{320+VERY_LARGE_BORDER_SIDE*2,200+(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE*2)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)}, 
{960+(VERY_LARGE_BORDER_SIDE*3)*2, 600+3*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE*2)*2,200+(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{640+(VERY_LARGE_BORDER_SIDE*2)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{640+(VERY_LARGE_BORDER_SIDE*2)*2,400+2*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{1280+(VERY_LARGE_BORDER_SIDE*4)*2,800+4*(ORIGINAL_BORDER_TOP+VERY_LARGE_BORDER_BOTTOM)},
{-1,-1}},
{{800,600},
{-1,-1}}
};
#endif//bigtop
#endif//416


POINT WinSizeBorder[4][5];

#else

const POINT WinSizeBorder[4][5]={ {{320+BORDER_SIDE*2,200+(BORDER_TOP+BORDER_BOTTOM)},
                                   {640+(BORDER_SIDE*2)*2,400+2*(BORDER_TOP+BORDER_BOTTOM)},
                                   {960+(BORDER_SIDE*3)*2, 600+3*(BORDER_TOP+BORDER_BOTTOM)},
                                   {1280+(BORDER_SIDE*4)*2,800+4*(BORDER_TOP+BORDER_BOTTOM)},
                                   {-1,-1}},
                                  {{640+(BORDER_SIDE*2)*2,200+(BORDER_TOP+BORDER_BOTTOM)},
                                   {640+(BORDER_SIDE*2)*2,400+2*(BORDER_TOP+BORDER_BOTTOM)},
                                   {1280+(BORDER_SIDE*4)*2,400+2*(BORDER_TOP+BORDER_BOTTOM)},
                                   {1280+(BORDER_SIDE*4)*2,800+4*(BORDER_TOP+BORDER_BOTTOM)},
                                   {-1,-1}},
                                  {{640+(BORDER_SIDE*2)*2,400+2*(BORDER_TOP+BORDER_BOTTOM)},
                                   {1280+(BORDER_SIDE*4)*2,800+4*(BORDER_TOP+BORDER_BOTTOM)},
                                   {-1,-1}},
                                   {{800,600},
                                   {-1,-1}}
                                   };
#endif

#if defined(SSE_GUI_393)
int WinSizeForRes[4]={1,1,0,0}; // first run: double size (finally)
#else
int WinSizeForRes[4]={0,0,0,0}; // SS: for resolutions 0,1,2 & 3("crazy")
#endif

RECT rcPreFS;

BYTE PCCharToSTChar[128]={  0,  0,  0,159,  0,  0,187,  0,  0,  0,  0,  0,181,  0,  0,  0,
                            0,  0,154,  0,  0,  0,  0,255,  0,191,  0,  0,182,  0,  0,  0,
                            0,173,  0,156,  0,157,  0,221,185,189,  0,174,170,  0,190,  0,
                          248,241,253,254,  0,230,188,  0,  0,199,  0,175,172,171,  0,168,
                          182,  0,  0,183,142,143,147,128,  0,144,  0,  0,  0,  0,  0,  0,
                            0,165,  0,  0,  0,184,153,194,178,  0,  0,  0,154,  0,  0,158,
                          133,160,131,176,132,134,145,135,138,130,136,137,141,161,140,139,
                            0,164,149,162,147,177,148,246,179,151,163,150,154,  0,  0,152};

BYTE STCharToPCChar[128]={199,  0,233,226,228,224,229,231,234,235,232,239,238,236,196,197,
                          201,230,  0,244,246,242,251,249,255,214,252,  0,163,165,223,131,
                          225,237,243,250,241,209,  0,  0,191,  0,172,189,188,161,171,187,
                          227,245,216,248,  0,140,156,195,213,168,  0,134,182,169,174,153,
                            0,  0,215,  0,  0,  0,  0,185,  0,  0,  0,  0,  0,  0,  0,  0,
                            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,167,  0,  0,
                            0,  0,  0,  0,  0,  0,181,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                            0,177,  0,  0,  0,  0,247,  0,176,  0,  0,  0,  0,178,179,151};


#undef EXT
#undef INIT

#endif

#if defined(SSE_BUILD)
#include "key_table.cpp" //temp!
#endif

#include "stemwin.cpp"
//#define LOGSECTION LOGSECTION_INIT
#if 1 || defined(SSE_BOILER_390_LOG2)
#define LOGSECTION LOGSECTION_VIDEO_RENDERING
#else
#define LOGSECTION LOGSECTION_OPTIONS
#endif


#if defined(SSE_VID_BORDERS)

extern int draw_last_scanline_for_border,res_vertical_scale; // forward

void ChangeBorderSize(int size) {

  if(size<0||size>BIGGEST_DISPLAY)
    size=0;

  switch(size)
  {
    case 0: //no border
    case 1:
      SideBorderSize=ORIGINAL_BORDER_SIDE;
      SideBorderSizeWin=ORIGINAL_BORDER_SIDE;
      BottomBorderSize=ORIGINAL_BORDER_BOTTOM;
      break;
    case 2:
      SideBorderSize=VERY_LARGE_BORDER_SIDE; // render 416
      SideBorderSizeWin=VERY_LARGE_BORDER_SIDE_WIN; // show 412
      BottomBorderSize=VERY_LARGE_BORDER_BOTTOM;
      break;
    case 3:
      SideBorderSize=VERY_LARGE_BORDER_SIDE; // render 416
      SideBorderSizeWin=VERY_LARGE_BORDER_SIDE; // show 416
      BottomBorderSize=VERY_LARGE_BORDER_BOTTOM;
      break;
  }//sw
  int i,j;
  for(i=0;i<4;i++) 
  {
    for(j=0;j<5;j++) 
    {
      switch(size)
      {
      case 0:
      case 1:
        WinSizeBorder[i][j]=WinSizeBorderOriginal[i][j];
        break;
      case 2:
        WinSizeBorder[i][j]=WinSizeBorderLarge[i][j];
        break;
      case 3:
        WinSizeBorder[i][j]=WinSizeBorderMax[i][j];
        break;
      }//sw
    }//nxt j
  }//nxt i
  draw_last_scanline_for_border=shifter_y+res_vertical_scale*(BORDER_BOTTOM);

  TRACE_LOG("ChangeBorderSize(%d) side %d side win %d bottom %d\n",size,SideBorderSize,SideBorderSizeWin,BottomBorderSize);

  StemWinResize();

  Disp.ScreenChange();

#if defined(SSE_VID_ANTICRASH_392)
  draw_begin(); // Lock() will update draw_line_length
  draw_end(); // but we must Unlock() or trouble when changing display mode
#endif
}

#endif//#if defined(SSE_VID_BORDERS)

#if defined(SSE_GUI_STATUS_BAR)
#if defined(SSE_GUI_STATUS_BAR_ICONS)
void GUIRefreshStatusBar(bool invalidate) {
#else
void GUIRefreshStatusBar() {
#endif

  HWND status_bar_win=GetDlgItem(StemWin,120); // get handle

  // should we show or hide that "status bar"?
  bool should_we_show=(OPTION_STATUS_BAR||OPTION_STATUS_BAR_GAME_NAME); 

#if defined(SSE_GUI_STATUS_BAR_392)
  if(HD6301.Crashed)
    M68000.ProcessingState=TM68000::HD6301_CRASH;
#endif

  // build text of "status bar", only if we're to show it
#if defined(SSE_GUI_STATUS_BAR_ALERT)
  // and it's no special string
  if(should_we_show && M68000.ProcessingState!=TM68000::INTEL_CRASH
    && M68000.ProcessingState!=TM68000::HALTED
    && M68000.ProcessingState!=TM68000::BOILER_MESSAGE
#if defined(SSE_GUI_STATUS_BAR_392)
    && M68000.ProcessingState!=TM68000::HD6301_CRASH
#endif
    )
#else
  if(should_we_show)
#endif
  {
    char *status_bar=ansi_string;
#if defined(SSE_GUI_STATUS_BAR_ICONS)
    status_bar[0]='\0';
#endif
    if(OPTION_STATUS_BAR)
    {
      // basic ST/TOS/RAM
      char 
        sb_st_model[5],
        sb_tos[5],sb_ram[7];
#if defined(SSE_MMU_WU) && defined(SSE_GUI_OPTIONS_WU)
      sprintf(sb_st_model,"%s%d",(ST_TYPE)? "STF":"STE",MMU.WS[OPTION_WS]);
      if(!OPTION_WS)
        sb_st_model[3]='\0';
#else
      sprintf(sb_st_model,"%s",(ST_TYPE)? "STF":"STE");
#endif
      ASSERT(tos_version<0x1000);
      sprintf(sb_tos,"T%x",tos_version);
      sprintf(sb_ram,"%dK",mem_len/1024);
#if defined(SSE_GUI_STATUS_BAR_ICONS) // make room for flag after TXXX
      sprintf(status_bar,"%s %s       %s",sb_st_model,sb_tos,sb_ram);
#else
      sprintf(status_bar,"%s %s %s",sb_st_model,sb_tos,sb_ram);
#endif
#if defined(SSE_GUI_STATUS_BAR_HISPEED) && defined(SSE_CPU_MFP_RATIO)
      if(n_cpu_cycles_per_second>CpuNormalHz)
      {
        char sb_clock[10];
        sprintf(sb_clock," %dMHZ",n_cpu_cycles_per_second/1000000);
        strcat(status_bar,sb_clock);
      }
#endif
#if !defined(SSE_GUI_STATUS_BAR_ICONS)
      // some options
#if defined(SSE_IKBD_6301) && defined(SSE_GUI_STATUS_BAR_6301)
      if(OPTION_C1)
        strcat(status_bar," C1"); //saves som space
#if defined(SSE_GUI_STATUS_BAR_68901) && !defined(SSE_GUI_STATUS_BAR_ICONS)
      else
        strcat(status_bar," X");
#endif
#endif
#if defined(SSE_GUI_STATUS_BAR_68901)
      if(OPTION_C2)
        strcat(status_bar," C2");
#if !defined(SSE_GUI_STATUS_BAR_ICONS)
      else
        strcat(status_bar," X");
#endif
#endif//!icon
      if(OPTION_CPU_CLOCK 
#if defined(SSE_GUI_STATUS_BAR_SINGLE_SIDE)
        || (SSEOption.SingleSideDriveMap&3)
#endif
        )
        strcat(status_bar,"!");
#if defined(SSE_GUI_STATUS_BAR_HD)
      if(!HardDiskMan.DisableHardDrives || ACSI_EMU_ON)
        strcat(status_bar," HD");
#endif
#if defined(SSE_GUI_STATUS_BAR_ADAT)
      if(!floppy_instant_sector_access) // the option only 
        strcat(status_bar," ADAT");
#endif
#if defined(SSE_GUI_STATUS_BAR_HACKS)
      if(OPTION_HACKS)
        strcat(status_bar," #"); // which symbol?
#endif
#endif//#if !defined(SSE_GUI_STATUS_BAR_ICONS)
#if defined(SSE_PRIVATE_BUILD)
      if(SSEOption.TestingNewFeatures)
        strcat(status_bar," ##");
#endif
#if defined(SSE_GUI_STATUS_BAR_VSYNC)
      if(OPTION_WIN_VSYNC)
        strcat(status_bar," V"); // V for VSync!
#endif
    }

#if defined(SSE_GUI_STATUS_BAR_DISK_NAME)
/*  We try to take advantage of all space.
    Font is proportional so we need a margin.
    TODO: precise computing
*/
    if(OPTION_STATUS_BAR_GAME_NAME 
      && (FloppyDrive[floppy_current_drive()].NotEmpty() 
#if defined(SSE_TOS_PRG_AUTORUN) && !defined(SSE_TOS_PRG_AUTORUN_392)
      || SF314[0].ImageType.Extension==EXT_PRG 
      || SF314[0].ImageType.Extension==EXT_TOS
#endif
      ))
    {
#define MAX_TEXT_LENGTH_BORDER_ON (30+62+10) 
#define MAX_TEXT_LENGTH_BORDER_OFF (30+42+10) 
#if defined(SSE_VID_BORDERS_GUI_392)
      size_t max_text_length=(border!=0)?MAX_TEXT_LENGTH_BORDER_ON:
        MAX_TEXT_LENGTH_BORDER_OFF;
#elif defined(SSE_VS2008_WARNING_382)
      size_t max_text_length=(border&1)?MAX_TEXT_LENGTH_BORDER_ON:MAX_TEXT_LENGTH_BORDER_OFF;
#else
      int max_text_length=(border&1)?MAX_TEXT_LENGTH_BORDER_ON:MAX_TEXT_LENGTH_BORDER_OFF;
#endif
      if(OPTION_STATUS_BAR)
        max_text_length-=30;
#if defined(SSE_VID_BORDERS)
      if(SideBorderSizeWin<VERY_LARGE_BORDER_SIDE)
        max_text_length-=5;
      if(SideBorderSizeWin==ORIGINAL_BORDER_SIDE)
        max_text_length-=5;
#endif
#if defined(SSE_GUI_STATUS_BAR_ICONS)
      char tmp[MAX_TEXT_LENGTH_BORDER_ON+2+1]=" ";
#else
      char tmp[MAX_TEXT_LENGTH_BORDER_ON+2+1]=" \"";
#endif
      if( strlen(FloppyDrive[floppy_current_drive()].DiskName.Text)<=max_text_length)
      {
        strncpy(tmp
#if defined(SSE_GUI_STATUS_BAR_ICONS)
          +1
#else
          +2
#endif
          ,FloppyDrive[floppy_current_drive()].DiskName.Text,max_text_length);
#if defined(SSE_TOS_PRG_AUTORUN)
        if(SF314[0].ImageType.Extension==EXT_PRG)
          strcat(tmp,dot_ext(EXT_PRG)); // TODO
        else if(SF314[0].ImageType.Extension==EXT_TOS)
          strcat(tmp,dot_ext(EXT_TOS));
#endif
      }
      else
      {
        strncpy(tmp
#if defined(SSE_GUI_STATUS_BAR_ICONS)
          +1
#else
          +2
#endif
          ,FloppyDrive[floppy_current_drive()].DiskName.Text,max_text_length-3);
        strcat(tmp,"...");
      }
      strcat(status_bar,tmp);
#if !defined(SSE_GUI_STATUS_BAR_ICONS)
      strcat(status_bar,"\"");
#endif
    }
#undef MAX_TEXT_LENGTH_BORDER_ON
#undef MAX_TEXT_LENGTH_BORDER_OFF
#else
    if(0);
#endif
#if defined(SSE_DISK_EXT) && defined(SSE_DISK)
/*  If the game in A: isn't displayed on status bar, then we
    show what kind of file is in A: and B:. v3.7.2
*/
    else
    {
      char disk_type[13]; // " A:MSA B:STW"
#if defined(SSE_GUI_STATUS_BAR_ICONS)
      if(num_connected_floppies==1)
        sprintf(disk_type," A:%s",extension_list[SF314[0].ImageType.Extension]);
      else
#endif
      sprintf(disk_type," A:%s B:%s",extension_list[SF314[0].ImageType.Extension]
      ,extension_list[SF314[1].ImageType.Extension]);

      strcat(status_bar,disk_type);
    }
#endif
#if defined(SSE_GUI_STATUS_BAR_392)
    if(Disp.Method==DISPMETHOD_GDI)
      strcat(status_bar," GDI");
#endif
#if !defined(SSE_GUI_STATUS_BAR_ICONS)
    // change text
#if defined(SSE_GUI_STATUS_BAR_HALT) && defined(SSE_CPU_HALT)
    if(M68000.ProcessingState==TM68000::HALTED)
      //strcpy(status_bar,T("HALT (ST crashed)"));
      strcpy(status_bar,T("HALT"));
#endif
    SendMessage(status_bar_win,WM_SETTEXT,0,(LPARAM)(LPCTSTR)status_bar);
#endif
  }

#if defined(SSE_GUI_STATUS_BAR_ICONS)
#if defined(SSE_GUI_STATUS_BAR_HALT) && defined(SSE_CPU_HALT)
    if(M68000.ProcessingState==TM68000::HALTED)
      //strcpy(status_bar,T("HALT (ST crashed)"));
      strcpy(ansi_string,T("HALT"));
    else if(M68000.ProcessingState==TM68000::BLIT_ERROR)
      strcpy(ansi_string,T("BLIT ERROR"));
#if defined(SSE_GUI_STATUS_BAR_392)
    else if(M68000.ProcessingState==TM68000::HD6301_CRASH)
      strcpy(ansi_string,T("HD6301 CRASHED"));
#endif
#endif
#endif

  if(should_we_show)
  {
    // compute free width
#if defined(SSE_GUI_STATUS_BAR_392)
    RECT window_rect1,window_rect2;
#else
    RECT window_rect1,window_rect2,window_rect3;
#endif
    // last icon on the left is...
#if defined(SSE_GUI_CONFIG_FILE)
    HWND previous_icon=GetDlgItem(StemWin,121); // config
#else
    HWND previous_icon=GetDlgItem(StemWin,114); // paste
#endif
    GetWindowRect(previous_icon,&window_rect1); //absolute
    // first icon on the right
    HWND next_icon=GetDlgItem(StemWin,105); // info
    GetWindowRect(next_icon,&window_rect2); //absolute
    //TRACE_RECT(window_rect1); TRACE_RECT(window_rect2);
#if defined(SSE_GUI_STATUS_BAR_392)
    int w=window_rect2.left-window_rect1.right;
#else
    GetWindowRect(status_bar_win,&window_rect3);
    int w=window_rect2.left-window_rect1.right-10;
#endif
#if defined(SSE_GUI_STATUS_BAR_THRESHOLD)
    if(w<200)
      should_we_show=false;
    else
#endif    
    // resize status bar without trashing other icons
#if defined(SSE_GUI_STATUS_BAR_392)
    ;
    POINT mypoint;
    mypoint.x=window_rect1.right;
    mypoint.y=window_rect1.top;
    ScreenToClient(StemWin,&mypoint);

    //TRACE("move satus bar %d %d %d %d\n",mypoint.x,0,w,window_rect1.bottom-window_rect1.top);
    MoveWindow(status_bar_win,mypoint.x,0,w,window_rect1.bottom-window_rect1.top,FALSE);
#elif defined(SSE_GUI_CONFIG_FILE) // TODO, more "pro"
    MoveWindow(status_bar_win,23*7,0,w,window_rect3.bottom-window_rect3.top,FALSE);
#else
    MoveWindow(status_bar_win,23*6,0,w,window_rect3.bottom-window_rect3.top,FALSE);
#endif
  }

  // show or hide
  ShowWindow(status_bar_win, (should_we_show) ? SW_SHOW : SW_HIDE);

#if defined(SSE_GUI_STATUS_BAR_ICONS)
  if(invalidate)
    InvalidateRect(status_bar_win,NULL,FALSE); //to get message WM_DRAWITEM
#endif
}

#endif

//---------------------------------------------------------------------------
void GUIRunStart()
{
#ifdef WIN32
  KillTimer(StemWin,SHORTCUTS_TIMER_ID);
  if (AllowTaskSwitch==0) DisableTaskSwitch();
  if (HighPriority) SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
#elif defined(PRIO_PROCESS) && defined(PRIO_MAX)
  hxc::kill_timer(StemWin,SHORTCUTS_TIMER_ID);
	if (HighPriority) setpriority(PRIO_PROCESS,0,PRIO_MAX);
#endif
  CheckResetDisplay(true);
  WIN_ONLY( if (FullScreen) TScreenSaver::killTimer(); )
}
//---------------------------------------------------------------------------
bool GUIPauseWhenInactive()
{
  if ((PauseWhenInactive && bAppActive==0) || timer<CutPauseUntilSysEx_Time){
    bool MouseWasCaptured=(stem_mousemode==STEM_MOUSEMODE_WINDOW);
    if (FullScreen==0) SetStemMouseMode(STEM_MOUSEMODE_DISABLED);
#ifdef SSE_SOUND_16BIT_CENTRED
    Sound_Stop();
#else
    Sound_Stop(0);
#endif
#ifdef WIN32
    SetWindowText(StemWin,Str("Steem - ")+T("Suspended"));

    MSG mess;
    SetTimer(StemWin,MIDISYSEX_TIMER_ID,100,NULL);
    while (GetMessage(&mess,NULL,0,0)){
      if (HandleMessage(&mess)){
        TranslateMessage(&mess);
        DispatchMessage(&mess);
      }
      if (timeGetTime()>CutPauseUntilSysEx_Time && (PauseWhenInactive==0 || bAppActive)) break;
      if (runstate!=RUNSTATE_RUNNING) break;
    }
    if (mess.message==WM_QUIT) QuitSteem();
    KillTimer(StemWin,MIDISYSEX_TIMER_ID);

    SetWindowText(StemWin,stem_window_title);
#elif defined(UNIX)
    XEvent Ev;
    do{
      if (hxc::wait_for_event(XD,&Ev,200)) ProcessEvent(&Ev);
      
      if (timeGetTime()>CutPauseUntilSysEx_Time && (PauseWhenInactive==0 || bAppActive)) break;
    }while (runstate==RUNSTATE_RUNNING && Quitting==0);
#endif

    if (FullScreen==0 && MouseWasCaptured && GetForegroundWindow()==StemWin){
      SetStemMouseMode(STEM_MOUSEMODE_WINDOW);
    }

    Sound_Start();
    return true;
  }
  return 0;
}
//---------------------------------------------------------------------------
void GUIRunEnd()
{
  slow_motion=0;

  if (fast_forward){
    fast_forward=0;
    flashlight(false);

#ifdef WIN32
    SendMessage(GetDlgItem(StemWin,109),BM_SETCHECK,0,1);
#elif defined(UNIX)
    FastBut.set_check(0);
#endif
  }
  WIN_ONLY( if (HighPriority) SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS); )
#ifdef PRIO_PROCESS
	UNIX_ONLY( setpriority(PRIO_PROCESS,0,0); )
#endif
  SetStemMouseMode(STEM_MOUSEMODE_DISABLED);
  WIN_ONLY( EnableTaskSwitch(); )
  WIN_ONLY( SetTimer(StemWin,SHORTCUTS_TIMER_ID,50,NULL); )
  UNIX_ONLY( hxc::set_timer(StemWin,SHORTCUTS_TIMER_ID,50,timerproc,NULL); )
  WIN_ONLY( if (FullScreen) TScreenSaver::prepareTimer(); )
}
//---------------------------------------------------------------------------
void GUIColdResetChangeSettings()
{
  if (OptionBox.NewMemConf0==-1){
    if ((bank_length[0]+bank_length[1])==(KB512+KB128)){  // Old 512Kb
      OptionBox.NewMemConf0=MEMCONF_512;
      OptionBox.NewMemConf1=MEMCONF_512K_BANK1_CONF;
#if !(defined(SSE_MMU_2560K) && defined(SSE_BUGFIX_392))
    }else if ((bank_length[0]+bank_length[1])==(MB2+KB512)){  // Old 2Mb
      OptionBox.NewMemConf0=MEMCONF_2MB;
      OptionBox.NewMemConf1=MEMCONF_2MB_BANK1_CONF;
#endif
    }
  }
  if (OptionBox.NewMemConf0>=0){
    delete[] Mem;Mem=NULL;
    make_Mem(BYTE(OptionBox.NewMemConf0),BYTE(OptionBox.NewMemConf1));
    OptionBox.NewMemConf0=-1;
  }
  if (OptionBox.NewMonitorSel>=0){
#ifndef NO_CRAZY_MONITOR
#if defined(SSE_VS2008_WARNING_390)
    BYTE old_em=extended_monitor; //would need type_of() 
#else
    bool old_em=extended_monitor;
#endif
    extended_monitor=0;
#endif
    if (OptionBox.NewMonitorSel==1){
      mfp_gpip_no_interrupt &= MFP_GPIP_NOT_COLOUR;
#if defined(SSE_VID_BORDERS) && defined(SSE_VID_BORDERS_GUARD_R2)
      if(DISPLAY_SIZE)
        ChangeBorderSize(0);
#endif
    }else if (OptionBox.NewMonitorSel==0){
      mfp_gpip_no_interrupt |= MFP_GPIP_COLOUR;
    }else{ //crazy monitor
#ifndef NO_CRAZY_MONITOR
      int m=OptionBox.NewMonitorSel-2;
      if (extmon_res[m][2]==1){
        mfp_gpip_no_interrupt &= MFP_GPIP_NOT_COLOUR;
        screen_res=2;
      }else{
        mfp_gpip_no_interrupt |= MFP_GPIP_COLOUR;
        screen_res=0;
      }
      extended_monitor=1;
      em_width=extmon_res[m][0];em_height=extmon_res[m][1];em_planes=extmon_res[m][2];
#if defined(SSE_VID_BORDERS_GUARD_EM)
      DISPLAY_SIZE=0;
#endif
#endif
    }
#ifndef NO_CRAZY_MONITOR
#if defined(SSE_VS2008_WARNING_390)
    if (extended_monitor!=old_em || extended_monitor){
#else
    if (bool(extended_monitor)!=old_em || extended_monitor){
#endif
      if (FullScreen){
        change_fullscreen_display_mode(true);
      }else{
        Disp.ScreenChange(); // For extended monitor
      }
    }
#endif
    OptionBox.NewMonitorSel=-1;
  }
#if defined(SSE_TOS_STE_FAST_BOOT) //force recheck
  if(OPTION_HACKS && (tos_version==0x106||tos_version==0x162)
#if USE_PASTI
    && !pasti_active
#endif
#if defined(SSE_ACSI)
    && !(ACSI_EMU_ON) 
#endif
    && OptionBox.NewROMFile.IsEmpty()
    )
    OptionBox.NewROMFile=ROMFile;
#endif
  if (OptionBox.NewROMFile.NotEmpty()){
    if (load_TOS(OptionBox.NewROMFile)){
      Alert(T("The selected TOS file")+" "+OptionBox.NewROMFile+" "+T("is not in the correct format or may be corrupt."),T("Cannot Load TOS"),MB_ICONEXCLAMATION);
    }else{
      ROMFile=OptionBox.NewROMFile;
    }
    OptionBox.NewROMFile="";
  }
}
//---------------------------------------------------------------------------
void GUISaveResetBackup()
{
  DeleteFile(WriteDir+SLASH+"auto_loadsnapshot_backup.sts");
  SaveSnapShot(WriteDir+SLASH+"auto_reset_backup.sts",-1,0); // Don't add to history
}
//---------------------------------------------------------------------------
void GUIDiskErrorEject(int f)
{
  DiskMan.EjectDisk(f);
}
//---------------------------------------------------------------------------
void GUIEmudetectCreateDisk(Str Name,int Sides,int TracksPerSide,int SectorsPerTrack)
{
  Str DiskPath=DiskMan.HomeFol+SLASH+Name+".st";
  DiskMan.CreateDiskImage(Name,TracksPerSide*Sides*SectorsPerTrack,SectorsPerTrack,Sides);
  DiskMan.InsertDisk(0,Name,DiskPath);
}
//---------------------------------------------------------------------------
bool GUICanGetKeys()
{
  return (GetForegroundWindow()==StemWin);
}
//---------------------------------------------------------------------------
#ifdef WIN32
//---------------------------------------------------------------------------
int GetScreenWidth()
{
  return GetSystemMetrics(SM_CXSCREEN);
}
//---------------------------------------------------------------------------
int GetScreenHeight()
{
  return GetSystemMetrics(SM_CYSCREEN);
}
//---------------------------------------------------------------------------
void LoadAllIcons(ConfigStoreFile *NOT_ONEGAME( pCSF ),bool NOT_ONEGAME( FirstCall ))
{
#ifdef ONEGAME
  for (int n=1;n<RC_NUM_ICONS;n++) hGUIIcon[n]=NULL;
#else
  //ASSERT(RC_NUM_ICONS==80);
  HICON hOld[RC_NUM_ICONS],hOldSmall[RC_NUM_ICONS];
  for (int n=1;n<RC_NUM_ICONS;n++){
    hOld[n]=hGUIIcon[n];
    hOldSmall[n]=hGUIIconSmall[n];
  }

  bool UseDefault=0;
  HDC dc=GetDC(NULL);
  if (GetDeviceCaps(dc,BITSPIXEL)<=8){
#if defined(SSE_VS2008_WARNING_390)
    UseDefault=(pCSF->GetInt("Icons","UseDefaultIn256",0)!=0);
#else
    UseDefault=bool(pCSF->GetInt("Icons","UseDefaultIn256",0));
#endif
  }
  ReleaseDC(NULL,dc);

  Str File;
  for (int n=1;n<RC_NUM_ICONS;n++){
//    ASSERT(n!=79);
    int size=RCGetSizeOfIcon(n);
    bool load16too=size & 1;
    size&=~1;

    hGUIIcon[n]=NULL;
    hGUIIconSmall[n]=NULL;

    
    if (size){
      if (UseDefault==0) File=pCSF->GetStr("Icons",Str("Icon")+n,"");
      if (File.NotEmpty()) hGUIIcon[n]=(HICON)LoadImage(Inst,File,IMAGE_ICON,size,size,LR_LOADFROMFILE);
      if (hGUIIcon[n]==NULL) hGUIIcon[n]=(HICON)LoadImage(Inst,RCNUM(n),IMAGE_ICON,size,size,0);
     // TRACE("%d %d %d\n",n,size,hGUIIcon[n]);
      if (load16too){
        if (File.NotEmpty()) hGUIIconSmall[n]=(HICON)LoadImage(Inst,File,IMAGE_ICON,16,16,LR_LOADFROMFILE);
        if (hGUIIconSmall[n]==NULL) hGUIIconSmall[n]=(HICON)LoadImage(Inst,RCNUM(n),IMAGE_ICON,16,16,0);
      }
    }
  }
  if (FirstCall==0){
    // Update all window classes, buttons and other icon thingies
#if defined(SSE_X64_LPTR)
    SetClassLongPtr(StemWin, GCLP_HICON, long(hGUIIcon[RC_ICO_APP]));
#else
    SetClassLong(StemWin,GCL_HICON,long(hGUIIcon[RC_ICO_APP]));
#endif
#ifdef DEBUG_BUILD
#if defined(SSE_X64_LPTR)
    SetClassLongPtr(DWin,GCLP_HICON,long(hGUIIcon[RC_ICO_TRASH]));
    SetClassLongPtr(trace_window_handle,GCLP_HICON,long(hGUIIcon[RC_ICO_STCLOSE]));
#else
    SetClassLong(DWin,GCL_HICON,long(hGUIIcon[RC_ICO_TRASH]));
    SetClassLong(trace_window_handle,GCL_HICON,long(hGUIIcon[RC_ICO_STCLOSE]));
#endif
    for (int n=0;n<MAX_MEMORY_BROWSERS;n++){
      if (m_b[n]) m_b[n]->update_icon();
    }
#endif
    for (int n=0;n<nStemDialogs;n++) DialogList[n]->UpdateMainWindowIcon();
#if defined(SSE_GUI_CONFIG_FILE)
    for (int id=100;id<=120+1;id++){
#else
    for (int id=100;id<=120;id++){
#endif
      if (GetDlgItem(StemWin,id)) PostMessage(GetDlgItem(StemWin,id),BM_RELOADICON,0,0);
    }
    DiskMan.LoadIcons();
    OptionBox.LoadIcons();
    InfoBox.LoadIcons();
    ShortcutBox.UpdateDirectoryTreeIcons(&(ShortcutBox.DTree));
    if (ShortcutBox.pChooseMacroTree) ShortcutBox.UpdateDirectoryTreeIcons(ShortcutBox.pChooseMacroTree);

    for (int n=1;n<RC_NUM_ICONS;n++){
      if (hOld[n]) DestroyIcon(hOld[n]);
      if (hOldSmall[n]) DestroyIcon(hOldSmall[n]);
    }
  }
#endif
}
//---------------------------------------------------------------------------

//#define LOGSECTION LOGSECTION_INIT //SS

bool MakeGUI()
{
#if defined(SSE_GUI_FONT_FIX)
  fnt=SSEConfig.GuiFont();
#else
	fnt=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
#endif
  MidGUIRGB=GetMidColour(GetSysColor(COLOR_3DFACE),GetSysColor(COLOR_WINDOW));
  DkMidGUIRGB=GetMidColour(GetSysColor(COLOR_3DFACE),MidGUIRGB);

  PCArrow=LoadCursor(NULL,IDC_ARROW);

  ParentWin=GetDesktopWindow();
  WNDCLASS wc={0,WndProc,0,0,Inst,hGUIIcon[RC_ICO_APP],
                PCArrow,NULL,NULL,"Steem Window"};
  RegisterClass(&wc);

  wc.lpfnWndProc=FSClipWndProc;
  wc.hIcon=NULL;
  wc.hCursor=NULL;
  wc.lpszClassName="Steem Fullscreen Clip Window";
  RegisterClass(&wc);

  wc.lpfnWndProc=FSQuitWndProc;
  wc.lpszClassName="Steem Fullscreen Quit Button";
  RegisterClass(&wc);

  wc.lpfnWndProc=ResetInfoWndProc;
  wc.lpszClassName="Steem Reset Info Window";
  RegisterClass(&wc);

  RegisterSteemControls();
  RegisterButtonPicker();

  StemWin=CreateWindowEx(WS_EX_ACCEPTFILES,"Steem Window",stem_window_title,WS_CAPTION | WS_SYSMENU |
                          WS_MINIMIZEBOX | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS,
                          180,180,324 + GetSystemMetrics(SM_CXFRAME)*2,
                          204+MENUHEIGHT + GetSystemMetrics(SM_CYFRAME)*2 + GetSystemMetrics(SM_CYCAPTION),
                          ParentWin,NULL,Inst,NULL);

  if (StemWin==NULL) return 0;
  if (IsWindow(StemWin)==0){
    StemWin=NULL;
    return 0;
  }

  StemWin_SysMenu=GetSystemMenu(StemWin,0);
  int pos=GetMenuItemCount(StemWin_SysMenu)-2;
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,105,T("Smaller Window"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,104,T("Bigger Window"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_SEPARATOR,0,NULL);
#if defined(SSE_VID_DISABLE_AUTOBORDER) //leave it so?
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,110,T("Borders Off"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,111,T("Borders On"));
#else
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,112,T("Auto Borders"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,111,T("Always Show Borders"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,110,T("Never Show Borders"));
#endif
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_SEPARATOR,0,NULL);
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,113,T("Disable On Screen Display"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_SEPARATOR,0,NULL);
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,102,T("Always On Top"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,103,T("Restore Aspect Ratio"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_STRING,101,T("Normal Size"));
  InsertMenu(StemWin_SysMenu,pos,MF_BYPOSITION | MF_SEPARATOR,0,NULL);

  ToolTip=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,TTS_ALWAYSTIP | TTS_NOPREFIX,
                          0,0,100,100,NULL,NULL,Inst,NULL);
#ifdef DEBUG_BUILD
  SendMessage(ToolTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,20000); // 20 seconds before disappear
  SendMessage(ToolTip,TTM_SETDELAYTIME,TTDT_INITIAL,100);   // 0.1 of a second before appear
  SendMessage(ToolTip,TTM_SETDELAYTIME,TTDT_RESHOW,10);     // No time when moving from one tool to next
  SendMessage(ToolTip,TTM_SETMAXTIPWIDTH,0,150);
#else
  SendMessage(ToolTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,20000); // 20 seconds before disappear
  SendMessage(ToolTip,TTM_SETDELAYTIME,TTDT_INITIAL,400);   // 0.4 second before appear
  SendMessage(ToolTip,TTM_SETDELAYTIME,TTDT_RESHOW,200);     // 0.2 moving from one tool to next
  SendMessage(ToolTip,TTM_SETMAXTIPWIDTH,0,400);
#endif

#ifndef ONEGAME
  int x=0;
  HWND Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_PLAY),WS_CHILDWINDOW | WS_VISIBLE |
                          PBS_RIGHTCLICK,x,0,20,20,StemWin,(HMENU)101,Inst,NULL);
#if defined(SSE_GUI_380)
  ToolAddWindow(ToolTip,Win,T("Run (Left Click = Run/Stop, Right Click = Slow Motion)"));
#else
  ToolAddWindow(ToolTip,Win,T("Run (Right Click = Slow Motion)"));
#endif
  x+=23;

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_FF),WS_CHILDWINDOW | WS_VISIBLE |
                          PBS_RIGHTCLICK | PBS_DBLCLK,x,0,20,20,StemWin,(HMENU)109,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Fast Forward (Right Click = Searchlight, Double Click = Sticky)"));
  x+=23;

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_RESET),WS_CHILDWINDOW | WS_VISIBLE |
                          PBS_RIGHTCLICK,x,0,20,20,StemWin,(HMENU)102,Inst,NULL);

#if defined(SSE_GUI_RESET_BUTTON2)
  ToolAddWindow(ToolTip,Win,T("Reset (Left Click = Warm, Right Click = Cold)"));
#elif defined(SSE_GUI_RESET_BUTTON)
  ToolAddWindow(ToolTip,Win,T("Reset (Left Click) - Switch off (Right Click)"));
#else
  ToolAddWindow(ToolTip,Win,T("Reset (Left Click = Cold, Right Click = Warm)"));
#endif

  x+=23;

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_SNAPSHOTBUT),WS_CHILDWINDOW | WS_VISIBLE,
                          x,0,20,20,StemWin,(HMENU)108,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Memory Snapshot Menu"));
  x+=23;

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_TAKESCREENSHOTBUT),WS_CHILDWINDOW | WS_VISIBLE |
                          PBS_RIGHTCLICK,x,0,20,20,StemWin,(HMENU)115,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Take Screenshot")+" ("+T("Right Click = Options")+")");
  x+=23;
#if !defined(SSE_GUI_NO_PASTE)
  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_PASTE),WS_CHILD | WS_VISIBLE | PBS_RIGHTCLICK,
                          x,0,20,20,StemWin,(HMENU)114,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Paste Text Into ST (Right Click = Options)"));
  x+=23;
#endif
#ifdef RELEASE_BUILD
  // This causes freeze up if tracing in debugger, so only do it in final build
  NextClipboardViewerWin=SetClipboardViewer(StemWin);
#endif
#if !defined(SSE_GUI_NO_PASTE)
  UpdatePasteButton();
#endif
#if !(defined(SSE_VAR_NO_UPDATE))
  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_UPDATE),WS_CHILD,
                          x,0,20,20,StemWin,(HMENU)120,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Steem Update Available! Click Here For Details!"));
#endif

#if defined(SSE_GUI_CONFIG_FILE)
  // new 'wrench' icon for config files, popup menu when left click
  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_CFG),WS_CHILDWINDOW
    | WS_VISIBLE,x,0,20,20,StemWin,(HMENU)121,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Load/save configuration file"));
  x+=23;
#endif

#if defined(SSE_GUI_STATUS_BAR)  
/*  Create a static control as status bar. We take the undef update icon's
    number.
    WINDOW_TITLE is dummy, the field will be updated later, its size too.
*/

  Win=CreateWindowEx(0,"Static",WINDOW_TITLE,WS_CHILD | WS_VISIBLE
#if defined(SSE_GUI_STATUS_BAR_ICONS)
    |SS_OWNERDRAW
#else
    |SS_CENTER // horizontally
    |SS_CENTERIMAGE // vertically
#endif
    ,x,0,50,20,StemWin,(HMENU)120,Inst,NULL);
#endif

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_INFO),WS_CHILD | WS_VISIBLE,
                            100,0,20,20,StemWin,(HMENU)105,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("General Info"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_PATCHES),WS_CHILD | WS_VISIBLE,
                            100,0,20,20,StemWin,(HMENU)113,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Patches"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_SHORTCUT),WS_CHILD | WS_VISIBLE,
                            100,0,20,20,StemWin,(HMENU)112,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Shortcuts"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_OPTIONS),WS_CHILD | WS_VISIBLE,
                          100,0,20,20,StemWin,(HMENU)107,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Options"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_JOY),WS_CHILDWINDOW | WS_VISIBLE,
                          100,0,20,20,StemWin,(HMENU)103,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Joystick Configuration"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_DISKMAN),WS_CHILDWINDOW | WS_VISIBLE
    ,100,0,20,20,StemWin,(HMENU)100,Inst,NULL);

  ToolAddWindow(ToolTip,Win,T("Disk Manager"));

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_TOWINDOW),WS_CHILD,
                          120,0,20,20,StemWin,(HMENU)106,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Windowed Mode"));

#if defined(SSE_VID_FS_PROPER_QUIT_BUTTON)
  Win=CreateWindow("Steem Fullscreen Quit Button","",WS_CHILD,
      120,0,20,20,StemWin,(HMENU)116,Inst,NULL);
  ToolAddWindow(ToolTip,Win,T("Quit Steem"));
#endif

  SetWindowAndChildrensFont(StemWin,fnt);
#endif

#ifndef ONEGAME
  CentreWindow(StemWin,0);
#else
  MoveWindow(StemWin,GetScreenWidth(),0,100,100,0);
#endif

#ifdef DEBUG_BUILD
  dbg_log("STARTUP: DWin_init Called");
  DWin_init();
#endif

  return true;
}
//---------------------------------------------------------------------------
void CheckResetIcon()
{
  if (StemWin==NULL) return;

  HWND ResetBut=GetDlgItem(StemWin,102);
  if (ResetBut==NULL) return;

  int new_icon=int(OptionBox.NeedReset() ? RC_ICO_RESETGLOW:RC_ICO_RESET);
  Str CurNumText;
  CurNumText.SetLength(20);
  GetWindowText(ResetBut,CurNumText,20);
  if (atoi(CurNumText)!=new_icon) SetWindowText(ResetBut,Str(new_icon));
}
//---------------------------------------------------------------------------
void CheckResetDisplay(bool NOT_ONEGAME(AlwaysHide))
{
#ifndef ONEGAME
  if (pc==rom_addr && StemWin && runstate==RUNSTATE_STOPPED && AlwaysHide==0){
    if (ResetInfoWin==NULL){
      if (FullScreen==0) SetWindowLong(StemWin,GWL_STYLE,GetWindowLong(StemWin,GWL_STYLE) | WS_CLIPCHILDREN);
#if defined(SSE_VID_D3D) || defined(SSE_VID_DD_NO_FS_CLIPPER)
      ResetInfoWin=CreateWindow("Steem Reset Info Window","",WS_CHILD,
                            0,0,0,0,HWND(StemWin),(HMENU)9876,Inst,NULL);
#else
      ResetInfoWin=CreateWindow("Steem Reset Info Window","",WS_CHILD,
                            0,0,0,0,HWND(FullScreen ? ClipWin:StemWin),(HMENU)9876,Inst,NULL);
#endif
      SendMessage(ResetInfoWin,WM_USER,1789,0);
      ShowWindow(ResetInfoWin,SW_SHOWNA);
    }else{
      SendMessage(ResetInfoWin,WM_USER,1789,0);
      InvalidateRect(ResetInfoWin,NULL,0);
    }
  }else if (ResetInfoWin){
    HWND Win=ResetInfoWin;
    ResetInfoWin=NULL;
    DestroyWindow(Win);
    SetWindowLong(StemWin,GWL_STYLE,GetWindowLong(StemWin,GWL_STYLE) & ~WS_CLIPCHILDREN);
  }
#endif
}
//---------------------------------------------------------------------------

#undef LOGSECTION
#define LOGSECTION LOGSECTION_SHUTDOWN

void CleanupGUI()
{
  WNDCLASS wc;
#ifdef DEBUG_BUILD
  DWin_edit_is_being_temporarily_defocussed=true;
  dbg_log("SHUTDOWN: Destroying debug-build menus");
  if (insp_menu) DestroyMenu(insp_menu);

  if (trace_window_handle) DestroyWindow(trace_window_handle);
  trace_window_handle=NULL;

  dbg_log("SHUTDOWN: Destroying Boiler Room Mr Statics");
  if (DWin){
    mr_static_delete_children_of(DWin);
    mr_static_delete_children_of(DWin_timings_scroller.GetControlPage());
  }
  
  dbg_log("SHUTDOWN: Destroying debug-build Boiler Room window");
  if (DWin) DestroyWindow(DWin);

  dbg_log("SHUTDOWN: Destroying debug-build memory browsers");
  for (int n=0;n<MAX_MEMORY_BROWSERS;n++){
    if (m_b[n]!=NULL){
      if (m_b[n]->owner!=NULL){
        if (IsWindow(m_b[n]->owner)){
          DestroyWindow(m_b[n]->owner);
          n--;
        }
      }
    }

  }

  debug_plugin_free();

  if (HiddenParent) DestroyWindow(HiddenParent);

  if (GetClassInfo(Inst,"Steem Debug Window",&wc)){
    UnregisterClass("Steem Debug Window",Inst);
  }
  if (GetClassInfo(Inst,"Steem Trace Window",&wc)){
    UnregisterClass("Steem Mem Browser Window",Inst);
    UnregisterClass("Steem Trace Window",Inst);
  }
  if (mem_browser::icons_bmp){
    DeleteDC(mem_browser::icons_dc);
    DeleteObject(mem_browser::icons_bmp);
  }
#endif

  dbg_log("SHUTDOWN: Destroying StemWin");
  if (StemWin){
    CheckResetDisplay(true);
    DestroyWindow(StemWin);
    StemWin=NULL;
  }
  dbg_log("SHUTDOWN: Destroying ToolTip");
  if (ToolTip) DestroyWindow(ToolTip);

  if (GetClassInfo(Inst,"Steem Window",&wc)){
    UnregisterSteemControls();
    UnregisterButtonPicker();
    UnregisterClass("Steem Window",Inst);
    UnregisterClass("Steem Fullscreen Clip Window",Inst);
  }

  dbg_log("SHUTDOWN: Calling CoUninitialize()");
  CoUninitialize();

  for (int n=1;n<RC_NUM_ICONS;n++) if (hGUIIcon[n]) DestroyIcon(hGUIIcon[n]);
}


#undef LOGSECTION
#define LOGSECTION LOGSECTION_INIT

#endif

bool ComLineArgCompare(char*Arg,char*s,bool truncate=false)
{
  if (*Arg=='/' || *Arg=='-') Arg++;
  if (*Arg=='-') Arg++;
  if (truncate) return(IsSameStr_I(EasyStr(Arg).Lefts(strlen(s)),s));
  return IsSameStr_I(Arg,s);
}

int GetComLineArgType(char *Arg,EasyStr &Path)
{
  if (ComLineArgCompare(Arg,"NODD") || ComLineArgCompare(Arg,"GDI")){
    return ARG_GDI;
  }else if (ComLineArgCompare(Arg,"NODS") || ComLineArgCompare(Arg,"NOSOUND")){
    return ARG_NODS;
  }else if (ComLineArgCompare(Arg,"WINDOW")){
    return ARG_WINDOW;
  }else if (ComLineArgCompare(Arg,"FULLSCREEN")){
    return ARG_FULLSCREEN;
  }else if (ComLineArgCompare(Arg,"NONEW")){
    return ARG_NONEWINSTANCE;
  }else if (ComLineArgCompare(Arg,"OPENNEW")){
    return ARG_ALWAYSNEWINSTANCE;
  }else if (ComLineArgCompare(Arg,"NOLPT")){
    return ARG_NOLPT;
  }else if (ComLineArgCompare(Arg,"NOCOM")){
    return ARG_NOCOM;
  }else if (ComLineArgCompare(Arg,"NOSHM")){
    return ARG_NOSHM;
  }else if (ComLineArgCompare(Arg,"SCLICK")){
    return ARG_SOUNDCLICK;
  }else if (ComLineArgCompare(Arg,"HELP") || ComLineArgCompare(Arg,"H")){
    return ARG_HELP;
  }else if (ComLineArgCompare(Arg,"QUITQUICKLY")){
    return ARG_QUITQUICKLY;
  }else if (ComLineArgCompare(Arg,"DOUBLECHECKSHORTCUTS")){
    return ARG_DOUBLECHECKSHORTCUTS;
  }else if (ComLineArgCompare(Arg,"DONTLIMITSPEED")){
    return ARG_DONTLIMITSPEED;
  }else if (ComLineArgCompare(Arg,"ACCURATEFDC")){
    return ARG_ACCURATEFDC;
  }else if (ComLineArgCompare(Arg,"NOPCJOYSTICKS")){
    return ARG_NOPCJOYSTICKS;
  }else if (ComLineArgCompare(Arg,"OLDPORTIO")){
    return ARG_OLDPORTIO;
  }else if (ComLineArgCompare(Arg,"ALLOWREADOPEN")){
    return ARG_ALLOWREADOPEN;
  }else if (ComLineArgCompare(Arg,"NOINTS")){
    return ARG_NOINTS;
  }else if (ComLineArgCompare(Arg,"STFMBORDER")){
    return ARG_STFMBORDER;
  }else if (ComLineArgCompare(Arg,"SCREENSHOTUSEFULLNAME")){
    return ARG_SCREENSHOTUSEFULLNAME;
  }else if (ComLineArgCompare(Arg,"SCREENSHOTALWAYSADDNUM")){
    return ARG_SCREENSHOTALWAYSADDNUM;
  }else if (ComLineArgCompare(Arg,"ALLOWLPTINPUT")){
    return ARG_ALLOWLPTINPUT;
  }else if (ComLineArgCompare(Arg,"NONOTIFYINIT")){
    return ARG_NONOTIFYINIT;
  }else if (ComLineArgCompare(Arg,"PSGCAPTURE")){
    return ARG_PSGCAPTURE;
  }else if (ComLineArgCompare(Arg,"CROSSMOUSE")){
    return ARG_CROSSMOUSE;
  }else if (ComLineArgCompare(Arg,"RUN")){
    return ARG_RUN;
  }else if (ComLineArgCompare(Arg,"GDIFSBORDER")){
    return ARG_GDIFSBORDER;
  }else if (ComLineArgCompare(Arg,"PASTI")){
    return ARG_PASTI;
  }else if (ComLineArgCompare(Arg,"NOPASTI")){
    return ARG_NOPASTI;
  }else if (ComLineArgCompare(Arg,"NOAUTOSNAPSHOT")){
    return ARG_NOAUTOSNAPSHOT;

  }else if (ComLineArgCompare(Arg,"SOF=",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_SETSOF;
  }else if (ComLineArgCompare(Arg,"FONT=",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_SETFONT;
  }else if (ComLineArgCompare(Arg,"SCREENSHOT=",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_TAKESHOT;
  }else if (ComLineArgCompare(Arg,"SCREENSHOT",true)){
    Path="";
    return ARG_TAKESHOT;
#if !defined(SSE_CPU)
  }else if (ComLineArgCompare(Arg,"DIVUTIME=",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_SETDIVUTIME;
  }else if (ComLineArgCompare(Arg,"DIVSTIME=",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_SETDIVSTIME;
#endif
  }else if (ComLineArgCompare(Arg,"PABUFSIZE=",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_SETPABUFSIZE;
  }else if (ComLineArgCompare(Arg,"RTBUFSIZE",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_RTBUFSIZE;
  }else if (ComLineArgCompare(Arg,"RTBUFNUM",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_RTBUFNUM;
  }
#if defined(SSE_TRACE_FOR_RELEASE_390)
  else if (ComLineArgCompare(Arg,"NOTRACE",true)){
    return ARG_NOTRACE;
  }
#endif
#if defined(SSE_UNIX_TRACE)
  else if (ComLineArgCompare(Arg,"TRACEFILE=",true)){ //Y,N
    Path=strchr(Arg,'=')+1;
    return ARG_TRACEFILE;
  }
  else if (ComLineArgCompare(Arg,"LOGSECTION=",true)){
    Path=strchr(Arg,'=')+1;
    return ARG_LOGSECTION;
  }
#endif
  else{
    int Type=ARG_UNKNOWN;
    char *pArg=Arg;
    if (ComLineArgCompare(Arg,"INI=",true)){
      pArg=strchr(Arg,'=')+1;
      Type=ARG_SETINIFILE;
    }else if (ComLineArgCompare(Arg,"TRANS=",true)){
      pArg=strchr(Arg,'=')+1;
      Type=ARG_SETTRANSFILE;
    }else if (ComLineArgCompare(Arg,"CUTS=",true)){
      pArg=strchr(Arg,'=')+1;
      Type=ARG_SETCUTSFILE;
    }
    
    Path.SetLength(MAX_PATH);
    GetLongPathName(pArg,Path,MAX_PATH);

    if (Type!=ARG_UNKNOWN) return Type;

    char *dot=strrchr(GetFileNameFromPath(Path),'.');
    if (dot){
#if defined(SSE_DISK_PASTI_AUTO_SWITCH)
      if (ExtensionIsDisk(dot)){
#else
      if (ExtensionIsDisk(dot,false)){
#endif
        return ARG_DISKIMAGEFILE;
      }else if (ExtensionIsPastiDisk(dot)){
        return ARG_PASTIDISKIMAGEFILE;
      }else if (IsSameStr_I(dot,".STS")){
        return ARG_SNAPSHOTFILE;
      }else if (IsSameStr_I(dot,".STC")){
        return ARG_CARTFILE;
      }else if (IsSameStr_I(dot,".PRG") || IsSameStr_I(dot,".APP") || IsSameStr_I(dot,".TOS")){
        return ARG_STPROGRAMFILE;
      }else if (IsSameStr_I(dot,".GTP") || IsSameStr_I(dot,".TTP")){
        return ARG_STPROGRAMTPFILE;
      }else if (IsSameStr_I(dot,".LNK")){
        return ARG_LINKFILE;
      }else if (IsSameStr_I(dot,".IMG") || IsSameStr_I(dot,".ROM")){
        return ARG_TOSIMAGEFILE;
      }
    }
    return ARG_UNKNOWN;
  }
}

void ParseCommandLine(int NumArgs,char *Arg[],int Level)
{
  dbg_log("STARTUP: Command line arguments:");
  for (int n=0;n<NumArgs;n++){
    dbg_log(Str("     ")+Arg[n]);
    EasyStr Path;
    int Type=GetComLineArgType(Arg[n],Path);
    //TRACE("ARG %d %s %d\n",n,Path.Text,Type);
    switch (Type){
      case ARG_GDI:    WIN_ONLY( TryDD=0; ) break;
      case ARG_NODS:   TrySound=0; break;
      case ARG_NOSHM:  UNIX_ONLY( TrySHM=0; ) break;
      case ARG_NOLPT:  AllowLPT=0; break;
      case ARG_NOCOM:  AllowCOM=0; break;
      case ARG_WINDOW: BootInMode&=~BOOT_MODE_FLAGS_MASK;BootInMode=BOOT_MODE_WINDOW; break;
      case ARG_FULLSCREEN: BootInMode&=~BOOT_MODE_FLAGS_MASK;BootInMode=BOOT_MODE_FULLSCREEN; break;
      case ARG_SETSOF: sound_comline_freq=atoi(Path);sound_chosen_freq=sound_comline_freq;break;
      case ARG_SOUNDCLICK: sound_click_at_start=true; break;
      case ARG_DOUBLECHECKSHORTCUTS: WIN_ONLY( DiskMan.DoExtraShortcutCheck=true; ) break;
      case ARG_DONTLIMITSPEED: disable_speed_limiting=true; break;
#if !defined(SSE_FLOPPY_ALWAYS_ADAT)
      case ARG_ACCURATEFDC: floppy_instant_sector_access=0; break;
#endif
      case ARG_NOPCJOYSTICKS: DisablePCJoysticks=true; break;
      case ARG_OLDPORTIO: WIN_ONLY( TPortIO::AlwaysUseNTMethod=0; ) break;
#if !defined(SSE_CPU)
      case ARG_SETDIVUTIME: m68k_divu_cycles=atoi(Path)-4; break;
      case ARG_SETDIVSTIME: m68k_divs_cycles=atoi(Path)-4; break;
#endif
      case ARG_TAKESHOT:
        Disp.ScreenShotNextFile=Path;
        if (runstate==RUNSTATE_RUNNING){
          DoSaveScreenShot|=1;
        }else{
          Disp.SaveScreenShot();
        }
        break;
      case ARG_SETPABUFSIZE:  UNIX_ONLY( pa_output_buffer_size=atoi(Path); ) break;
      case ARG_ALLOWREADOPEN: stemdos_comline_read_is_rb=true; break;
      case ARG_NOINTS:        no_ints=true; break; // SS removed _
#if !(defined(SSE_SHIFTER_REMOVE_USELESS_VAR))
      case ARG_STFMBORDER:    stfm_borders=4; break;
#elif defined(SSE_VAR_ARG_STFM)
      case ARG_STFMBORDER:
        ST_TYPE=STF; // to help DemobaseST just in case
        break;
#endif
      case ARG_SCREENSHOTUSEFULLNAME: Disp.ScreenShotUseFullName=true; break;
      case ARG_SCREENSHOTALWAYSADDNUM: Disp.ScreenShotAlwaysAddNum=true; break;
      case ARG_ALLOWLPTINPUT: comline_allow_LPT_input=true; break;
#if !defined(SSE_YM2149_DISABLE_CAPTURE_FILE)
      case ARG_PSGCAPTURE: psg_always_capture_on_start=true; break;
#endif
      case ARG_CROSSMOUSE: no_set_cursor_pos=true; break;
#if defined(UNIX) && !defined(NO_RTAUDIO)
      case ARG_RTBUFSIZE: rt_buffer_size=atoi(Path); break;
      case ARG_RTBUFNUM: rt_buffer_num=atoi(Path); break;
#endif
      case ARG_RUN: BootInMode|=BOOT_MODE_RUN; break;
WIN_ONLY( case ARG_GDIFSBORDER:   Disp.DrawLetterboxWithGDI=true; break; )
      case ARG_PASTI: BootPasti=BOOT_PASTI_ON; break;
      case ARG_NOPASTI: BootPasti=BOOT_PASTI_OFF; break;
      case ARG_NOAUTOSNAPSHOT:
        BootDisk[0]=".";
        BootDisk[1]=".";
        break;

      case ARG_DISKIMAGEFILE:
        //TRACE("ARG_DISKIMAGEFILE %s A free %d B free %d\n",Path.Text,BootDisk[0].Empty(),BootDisk[1].Empty());
#if defined(SSE_VAR_ARG_SNAPSHOT_PLUS_DISK)
        if (BootDisk[1].Empty()||BootDisk[1]==".")
          BootDisk[int( (BootDisk[0].Empty()||BootDisk[0]==".") ? 0:1)]=Path;
#else
        if (BootDisk[1].Empty()) BootDisk[int(BootDisk[0].Empty() ? 0:1)]=Path;
#endif
        //TRACE("Boot disks %s %s\n",BootDisk[0].Text,BootDisk[1].Text);
        break;
      case ARG_PASTIDISKIMAGEFILE:
        BootPasti=BOOT_PASTI_ON;
        if (BootDisk[1].Empty()) BootDisk[int(BootDisk[0].Empty() ? 0:1)]=Path;
        break;
      case ARG_SNAPSHOTFILE:
        //TRACE("ARG_SNAPSHOTFILE %s\n",Path.Text);
//#if !defined(SSE_VAR_ARG_SNAPSHOT_PLUS_DISK)
        BootDisk[0]=".";
        BootDisk[1]=".";
//#endif
        BootStateFile=Path;
        TRACE_INIT("BootStateFile %s given as argument\n",BootStateFile.Text);
        break;
      case ARG_CARTFILE:
        if (load_cart(Path)==0){
          CartFile=Path;
          OptionBox.MachineUpdateIfVisible();
        }
        break;
      case ARG_STPROGRAMFILE:
        // Mount folder as Z: (disable all normal hard drives)
        // Copy autorun program into auto folder
        break;
      case ARG_LINKFILE:
#ifdef WIN32
        if (Level<10){
          WIN32_FIND_DATA wfd;
          Path=GetLinkDest(Path,&wfd);
          if (Path.NotEmpty()){
            ParseCommandLine(1,&(Path.Text),Level+1);
          }
        }
#endif
        break;
      case ARG_TOSIMAGEFILE:
        if (BootTOSImage==0){
          if (load_TOS(Path)==0){
            ROMFile=Path;
            BootTOSImage=true;
          }
        }
        break;
    }
  }
}
//---------------------------------------------------------------------------
bool OpenComLineFilesInCurrent(bool AlwaysSendToCurrent)
{
  EasyStringList esl;
  esl.Sort=eslNoSort;
  for (int n=0;n<_argc-1;n++){
    EasyStr Path;
    int Type=GetComLineArgType(_argv[1+n],Path);
#ifdef WIN32
    if (Type==ARG_LINKFILE){
      WIN32_FIND_DATA wfd;
      int Level=0;
      EasyStr DestPath=Path;
      do{

        DestPath=GetLinkDest(DestPath,&wfd);
        if (DestPath.NotEmpty()) break;
        Type=GetComLineArgType(DestPath,Path);
      }while (Type==ARG_LINKFILE && (++Level)<10);
    }
#endif
    switch (Type){
      case ARG_DISKIMAGEFILE:case ARG_PASTIDISKIMAGEFILE:case ARG_SNAPSHOTFILE:
      case ARG_CARTFILE:case ARG_TOSIMAGEFILE:
        esl.Add(Path,0);
      case ARG_TAKESHOT:
        esl.Add(_argv[1+n],0);
        break;
      case ARG_RUN:
        esl.Add(_argv[1+n],1);
        break;
    }
  }
  if (esl.NumStrings){
    bool RunOnly=true;
    for (int i=0;i<esl.NumStrings;i++){
      if (esl[i].Data[0]==0){
        RunOnly=0;
        break;
      }
    }
    // If you only pass the RUN command and haven't specified to open in current
    // then we shouldn't do anything, RUN is handled later.
    if (RunOnly && AlwaysSendToCurrent==0) return 0;
    // Send strings to running Steem
#ifdef WIN32
    HWND CurSteemWin=FindWindow("Steem Window",NULL);
    if (CurSteemWin){
      bool Success=0;
      COPYDATASTRUCT cds;
      cds.dwData=MAKECHARCONST('S','C','O','M');
      for (int n=0;n<esl.NumStrings;n++){
        cds.cbData=strlen(esl[n].String)+1;
        cds.lpData=esl[n].String;
        if (SendMessage(CurSteemWin,WM_COPYDATA,0,LPARAM(&cds))==MAKECHARCONST('Y','A','Y','S')) Success=true;
      }
      if (Success) return true;
    }
#elif defined(UNIX)
#endif
  }
  return 0;
}
#undef LOGSECTION
//---------------------------------------------------------------------------
void InitTranslations()
{
  if (Exists(TranslateFileName)==0){
    TranslateFileName="";
    DirSearch ds;
    if (ds.Find(RunDir+SLASH "Translate_*.txt")) TranslateFileName=RunDir+SLASH+ds.Name;
  }
  if (TranslateFileName.Empty()) return;

  FILE *f=fopen(TranslateFileName,"rb");
  if (f==NULL) return;

  int FileLen=GetFileLength(f);
  TranslateBufLen=FileLen+2000;
  TranslateBuf=new char[TranslateBufLen+8];
  fread(TranslateBuf,FileLen,1,f);
  TranslateBuf[FileLen]=0;
  fclose(f);

  TranslateUpperBuf=new char[strlen(TranslateBuf)+1];
  strcpy(TranslateUpperBuf,TranslateBuf);
  strupr(TranslateUpperBuf);
}
//---------------------------------------------------------------------------
#pragma warning (disable: 4701) //EStart==0 if break;//390
EasyStr Translation(char *s)
{
#ifdef TRANSLATION_TEST
  if (TranslateBuf==NULL) return Str(s).UpperCase();
#else
  if (TranslateBuf==NULL) return s;
#endif
  if (s[0]==0) return s;

  EasyStr UpperS=s;
  strupr(UpperS);

  char *EStart=TranslateUpperBuf-1,*EEnd,*TStart,*TEnd;

  bool Match;
  do{
    EStart=strstr(EStart+1,UpperS);
    if (EStart==NULL) break;
    Match=true;
    if (EStart>TranslateUpperBuf){
      if (*(EStart-1)!='\n') Match=0;
    }
    EEnd=EStart+strlen(s);
    if (EEnd>=TranslateUpperBuf+TranslateBufLen){
      EStart=NULL;break;
    }
    if (*EEnd=='\r'){
      EEnd+=2;
    }else if (*EEnd=='\n'){
      EEnd++;
    }else{
      Match=0;
    }
    if (EEnd>=TranslateUpperBuf+TranslateBufLen){
      EStart=NULL;break;
    }
    if (*EEnd=='\r' || *EEnd=='\n') Match=0;
  }while (Match==0);

  if (EStart==NULL){
    FILE *f=fopen(TranslateFileName,"ab");
    if (f){
      fprintf(f,"\r\n\r\n%s\r\n%s",s,s);
      fclose(f);

      if (strlen(TranslateBuf)+6+strlen(s)*2 >= size_t(TranslateBufLen-8)){
        TranslateBufLen=strlen(TranslateBuf)+6+strlen(s)*2+2000;
        char *Temp=new char[TranslateBufLen+8];
        strcpy(Temp,TranslateBuf);

        delete[] TranslateBuf;
        TranslateBuf=Temp;
      }
      strcat(TranslateBuf,EasyStr("\r\n\r\n")+s+"\r\n"+s);

      delete[] TranslateUpperBuf;
      TranslateUpperBuf=new char[strlen(TranslateBuf)+1];
      strcpy(TranslateUpperBuf,TranslateBuf);
      strupr(TranslateUpperBuf);
    }
#ifdef TRANSLATION_TEST
    return EasyStr(s).UpperCase();
#else
    return s;
#endif
  }
  EEnd-=DWORD(TranslateUpperBuf);
  EEnd+=DWORD(TranslateBuf);

  TStart=EEnd;

  EasyStr Ret;
  
  TEnd=strchr(TStart,'\n');
  if (TEnd){
    if (*(TEnd-1)=='\r') TEnd--;
    char OldEndChar=*TEnd;
    *TEnd=0;
    Ret=TStart;
    *TEnd=OldEndChar;
  }else{
    Ret=TStart;
  }

#ifdef TRANSLATION_TEST
  return Ret.UpperCase();
#else
  return Ret;
#endif
}

//---------------------------------------------------------------------------
#if defined(SSE_VAR_RESIZE)
#define FileTypes ansi_string //no problem as it's used for modal fileselect
#else
char FileTypes[512];
#endif

char *FSTypes(int Type,...)
{
  char *tp=FileTypes;
#if defined(SSE_VAR_RESIZE)
  ZeroMemory(FileTypes,256);
#else
  ZeroMemory(FileTypes,512);
#endif
  if (Type==2){
    strcpy(tp,T("Disk Images"));tp+=strlen(tp)+1;
    strcpy(tp,"*.st;*.stt;*.msa;*.dim;*.zip;*.stz");tp+=strlen(tp);
#ifdef RAR_SUPPORT
    strcpy(tp,";*.rar");tp+=strlen(tp);
#endif
#if defined(SSE_VAR_UNRAR)
    if(UNRAR_OK)
      strcpy(tp,";*.rar");tp+=strlen(tp);
#endif
#if defined(SSE_VAR_ARCHIVEACCESS)
    if(ARCHIVEACCESS_OK)
#if defined(SSE_VAR_ARCHIVEACCESS2)
      strcpy(tp,";*.7z;*.bz2;*.gz;*.tar;*.arj");tp+=strlen(tp);
#else
      strcpy(tp,";*.7z");tp+=strlen(tp);
#endif
#endif

#if USE_PASTI
    if (hPasti){
      tp[0]=';';tp++;
      pasti->GetFileExtensions(tp,160,TRUE); // will add "*.st;*.stx"
      tp+=strlen(tp);
    }
#endif
    tp++;
  }else if (Type==3){
    strcpy(tp,T("TOS Images"));tp+=strlen(tp)+1;
    strcpy(tp,"*.img;*.rom");tp+=strlen(tp)+1;
  }else{
    char **pStr=((char**)&Type)+1;
    while (*pStr){
      strcpy(tp,*pStr);
      tp+=strlen(*pStr)+1;
      pStr++;
      strcpy(tp,*pStr);
      tp+=strlen(*pStr)+1;
      pStr++;
    }
  }
  if (Type){
    strcpy(tp,T("All Files"));tp+=strlen(tp)+1;
    strcpy(tp,"*.*");
  }
  ASSERT(strlen(FileTypes)<256); //512 was overkill (maybe...)
  return FileTypes;
#if defined(SSE_VAR_RESIZE)
#undef FileTypes
#endif
}
//---------------------------------------------------------------------------
bool CheckForSteemRunning()
{
#ifdef WIN32
  SteemRunningMutex=CreateMutex(NULL,0,"Steem_Running");
  return GetLastError()==ERROR_ALREADY_EXISTS;
#elif defined(UNIX)
  return 0;
#endif
}
//---------------------------------------------------------------------------
bool CleanupTempFiles()
{
  bool SteemHasCrashed=0;

  for (int n=0;n<4;n++){
    char *prefix="MSA";
    switch (n){
      case 1: prefix="ZIP"; break;
      case 2: prefix="FMT"; break;
      case 3: prefix="CRA"; break;
    }
    DirSearch ds;
    if (ds.Find(WriteDir+SLASH+prefix+"*.TMP")){
      EasyStringList FileESL;
      do{
        FileESL.Add(WriteDir+SLASH+ds.Name);
      }while (ds.Next());
      ds.Close();

      for (int i=0;i<FileESL.NumStrings;i++) DeleteFile(FileESL[i].String);

      if (n==3) SteemHasCrashed=true;
    }
  }
  return SteemHasCrashed;
}
//---------------------------------------------------------------------------

#ifdef WIN32
//---------------------------------------------------------------------------
void EnableWindow2(HWND Win,bool Enable,HWND NoDisable)
{
  if (Win!=NoDisable){
    SetWindowLong(Win,GWL_STYLE,(GetWindowLong(Win,GWL_STYLE) & int(Enable ? ~WS_DISABLED:0xffffffff)) | int(Enable==0 ? WS_DISABLED:0));
  }
}
//---------------------------------------------------------------------------
void EnableAllWindows(bool Enable,HWND NoDisable)
{
  if (Enable){
    DisableFocusWin=NULL;
  }else{
    DisableFocusWin=NoDisable;
  }

  DEBUG_ONLY( EnableWindow2(DWin,Enable,NoDisable) );
  DEBUG_ONLY( if (trace_window_handle) EnableWindow2(trace_window_handle,Enable,NoDisable) );
  EnableWindow2(StemWin,Enable,NoDisable);
  if (DiskMan.Handle){
    if (HardDiskMan.Handle){
      EnableWindow2(HardDiskMan.Handle,Enable,NoDisable);
    }else if (DiskMan.VisibleDiag()==NULL){
      EnableWindow2(DiskMan.Handle,Enable,NoDisable);
    }else{
      EnableWindow2(DiskMan.VisibleDiag(),Enable,NoDisable);
    }
  }
  for (int n=0;n<nStemDialogs;n++){
    if (DialogList[n]!=&DiskMan){
      if (DialogList[n]->Handle) EnableWindow2(DialogList[n]->Handle,Enable,NoDisable);
    }
  }
}
//---------------------------------------------------------------------------
void ShowAllDialogs(bool Show)
{
  if (FullScreen==0) return;

  static bool DiskManWasMaximized=0;
  int PosChange=int(Show ? -3000:3000);
  if (DiskMan.Handle){
    if (DiskMan.FSMaximized && Show==0) DiskManWasMaximized=true;
    if (DiskManWasMaximized && Show){
#if defined(SSE_VID_GUI_392)
      SetWindowPos(DiskMan.Handle,NULL,-GetSystemMetrics(SM_CXFRAME),MENUHEIGHT,
        Disp.SurfaceWidth+GetSystemMetrics(SM_CXFRAME)*2,
        Disp.SurfaceHeight+GetSystemMetrics(SM_CYFRAME)-MENUHEIGHT,
        SWP_NOZORDER | SWP_NOACTIVATE);
#else
      SetWindowPos(DiskMan.Handle,NULL,-GetSystemMetrics(SM_CXFRAME),MENUHEIGHT,
                    int((border & 1) ? 800:640)+GetSystemMetrics(SM_CXFRAME)*2,
                    int((border & 1) ? 600:480)+GetSystemMetrics(SM_CYFRAME)-MENUHEIGHT,
                    SWP_NOZORDER | SWP_NOACTIVATE);
#endif
      DiskManWasMaximized=0;
    }else{
      DiskMan.FSLeft+=PosChange;
      //TRACE("PosChange %d FS %d->%d %d\n",PosChange,DiskMan.FSLeft-PosChange,DiskMan.FSLeft,DiskMan.FSTop);
      //TRACE_RECT(Disp.rcMonitor);
      SetWindowPos(DiskMan.Handle,NULL,DiskMan.FSLeft,DiskMan.FSTop,0,0,SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }
  }
  for (int n=0;n<nStemDialogs;n++){
    if (DialogList[n]!=&DiskMan){
      if (DialogList[n]->Handle){
        DialogList[n]->FSLeft+=PosChange;
        SetWindowPos(DialogList[n]->Handle,NULL,DialogList[n]->FSLeft,DialogList[n]->FSTop,0,0,SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
      }
    }
  }
#if !defined(SSE_VID_FS_PROPER_QUIT_BUTTON)
  if (FSQuitBut) ShowWindow(FSQuitBut,int(Show ? SW_SHOWNA:SW_HIDE));
#endif
}
//---------------------------------------------------------------------------
#if defined(SSE_VS2008_WARNING_390)
void HandleKeyPress(UINT VKCode,DWORD Up,int Extended)
#else
void HandleKeyPress(UINT VKCode,bool Up,int Extended)
#endif
{
  if (disable_input_vbl_count) return;
#if defined(SSE_IKBD_6301_393)
  if (!OPTION_C1 && ikbd_keys_disabled()) 
    return; //in duration mode
#else
  if (ikbd_keys_disabled()) return; //in duration mode
#endif
#if !defined(SSE_GUI_NO_MACROS)
  if (macro_play_has_keys) return;
#endif
  BYTE STCode=0;

  if ((Extended & 3)==1){
    switch (LOBYTE(VKCode)){
      case VK_RETURN: STCode=STKEY_PAD_ENTER; break;
      case VK_DIVIDE: STCode=STKEY_PAD_DIVIDE; break;
    }
  }

  bool DidShiftSwitching=0;
  int ModifierRestoreArray[3];
  if (EnableShiftSwitching && shift_key_table[0] && (Extended & NO_SHIFT_SWITCH)==0){
    if (STCode==0){
      HandleShiftSwitching(VKCode,Up,STCode,ModifierRestoreArray);
      if (STCode) DidShiftSwitching=true;
    }
  }
  
  if (STCode==0) STCode=key_table[BYTE(VKCode)]; //SS: +- ASCII -> ST scancode
  if (STCode
#if defined(SSE_GUI_F12)
    && VKCode!=VK_F12
#endif
    ){
    ST_Key_Down[STCode]=!Up; // this is used by ikbd.cpp & ireg.c


#if defined(SSE_DEBUG) //&& defined(SSE_DEBUG_IKBD_6301_TRACE_KEYS)
#define LOGSECTION LOGSECTION_IKBD
    TRACE_LOG("Key PC $%X ST $%X ",VKCode,STCode);
    TRACE_LOG( (Up) ? "-\n" : "+\n");
#undef LOGSECTION
#endif

#if defined(SSE_IKBD_6301_MACRO) && !defined(SSE_GUI_NO_MACROS)
    if (Up)
      STCode|=MSB_B; // MSB_B = $80
    if(OPTION_C1)
    {
      //We don't write in a buffer, 6301 emu will do it after having scanned
      //ST_Key_Down.
      if(macro_record)
        macro_record_key(STCode);
    }
    else
      keyboard_buffer_write_n_record(STCode);
#elif defined(SSE_IKBD_6301)
    if(!OPTION_C1)
    {
      if (Up) STCode|=MSB_B; // MSB_B = $80
      keyboard_buffer_write_n_record(STCode);
    }
#else  // Steem 3.2
// The break code for each key is obtained by ORing 0x80 with the make code:
    if (Up) STCode|=MSB_B; // MSB_B = $80
    keyboard_buffer_write_n_record(STCode);
#endif
    
#ifndef DISABLE_STEMDOS
    if (VKCode=='C'){ //control-C
      if (ST_Key_Down[key_table[VK_CONTROL]]){ //control-C
        stemdos_control_c();
      }
    }
#endif
  }//SS: if (STCode): if not, it's a PC key that doesn't translate to ST
  if (DidShiftSwitching) ShiftSwitchRestoreModifiers(ModifierRestoreArray);
}
//---------------------------------------------------------------------------
void SetStemMouseMode(int NewMM)
{
  static POINT OldMousePos={-1,0};
//  if (NewMM==stem_mousemode) return;

  if (stem_mousemode!=STEM_MOUSEMODE_WINDOW && NewMM==STEM_MOUSEMODE_WINDOW) GetCursorPos(&OldMousePos);
  stem_mousemode=NewMM;
  if (NewMM==STEM_MOUSEMODE_WINDOW){
#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
    if (no_set_cursor_pos || (SSEOption.VMMouse)){
      SetCursor( (no_set_cursor_pos)? LoadCursor(NULL,RCNUM(IDC_CROSS)) : NULL);
#else
    if (no_set_cursor_pos){
      SetCursor(LoadCursor(NULL,RCNUM(IDC_CROSS)));
#endif
      POINT pt;
      GetCursorPos(&pt);
      window_mouse_centre_x=pt.x;
      window_mouse_centre_y=pt.y;
    }else{
      SetCursor(NULL);
      RECT rc;
      GetWindowRect(StemWin,&rc);
      window_mouse_centre_x=rc.left+164+GetSystemMetrics(SM_CXFRAME);
      window_mouse_centre_y=rc.top+104+MENUHEIGHT+GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYCAPTION);
      SetCursorPos(window_mouse_centre_x,window_mouse_centre_y);
    }

#ifndef DEBUG_BUILD
#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
    if(!SSEOption.VMMouse)// we don't clip, mouse can exit window
#endif
      if (FullScreen){
        ClipCursor(NULL);
      }else{
        RECT rc;
        POINT pt={0,0};
        GetClientRect(StemWin,&rc);
        rc.right-=6;
        rc.bottom-=6+MENUHEIGHT;
        ClientToScreen(StemWin,&pt);
        OffsetRect(&rc,pt.x+3,pt.y+3+MENUHEIGHT);
        ClipCursor(&rc);
      }
#endif
  }else{
    SetCursor(PCArrow);
    if (FullScreen && runstate==RUNSTATE_RUNNING) runstate=RUNSTATE_STOPPING;
#ifndef DEBUG_BUILD
    ClipCursor(NULL);
#endif
#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
    if(!SSEOption.VMMouse)
#endif
      if (OldMousePos.x>=0 && no_set_cursor_pos==0){
        SetCursorPos(OldMousePos.x,OldMousePos.y);
        OldMousePos.x=-1;
      }
  }
  mouse_move_since_last_interrupt_x=0;
  mouse_move_since_last_interrupt_y=0;
  mouse_change_since_last_interrupt=false;
}
//---------------------------------------------------------------------------
#define WH_KEYBOARD_LL 13
#if (_WIN32_WINNT < 0x0400)
#define LLKHF_ALTDOWN 0x00000020
#endif
#if !defined(MINGW_BUILD) && (_WIN32_WINNT < 0x0400) // well well
typedef struct{
  DWORD vkCode;
  DWORD scanCode;
  DWORD flags;
  DWORD time;
  DWORD dwExtraInfo;
}KBDLLHOOKSTRUCT, *LPKBDLLHOOKSTRUCT;
#endif

LRESULT CALLBACK NTKeyboardProc(INT nCode,WPARAM wParam,LPARAM lParam)
{
  KBDLLHOOKSTRUCT *pkbhs=LPKBDLLHOOKSTRUCT(lParam);

  if (nCode==HC_ACTION){
#if defined(SSE_VS2008_WARNING_390)
    bool ControlDown=(GetAsyncKeyState(VK_CONTROL) < 0),AltDown=(pkbhs->flags & LLKHF_ALTDOWN)!=0;
#else
    bool ControlDown=(GetAsyncKeyState(VK_CONTROL) < 0),AltDown=(pkbhs->flags & LLKHF_ALTDOWN);
#endif
    bool ShiftDown=(GetAsyncKeyState(VK_SHIFT) < 0);

    if (pkbhs->vkCode==VK_TAB && AltDown) return 1;

    if (pkbhs->vkCode==VK_ESCAPE && (AltDown || ShiftDown || ControlDown)) return 1;

    if (pkbhs->vkCode==VK_DELETE && AltDown && ControlDown) return 1;

#ifdef ONEGAME
    if (pkbhs->vkCode==VK_LWIN || pkbhs->vkCode==VK_RWIN) return 1;
#endif
  }
  return CallNextHookEx(hNTTaskSwitchHook,nCode,wParam,lParam);
}
//---------------------------------------------------------------------------
void DisableTaskSwitch()
{
  if (TaskSwitchDisabled) return;

  if (WinNT){
    hNTTaskSwitchHook=SetWindowsHookEx(WH_KEYBOARD_LL,HOOKPROC(NTKeyboardProc),NULL,GetCurrentThreadId());
    if (hNTTaskSwitchHook==NULL){
      int Base=1400;
      RegisterHotKey(StemWin,Base++,MOD_ALT,VK_TAB);
      RegisterHotKey(StemWin,Base++,MOD_ALT | MOD_SHIFT,VK_TAB);


      RegisterHotKey(StemWin,Base++,MOD_ALT,VK_ESCAPE);
      RegisterHotKey(StemWin,Base++,MOD_ALT | MOD_SHIFT,VK_ESCAPE);

      RegisterHotKey(StemWin,Base++,MOD_CONTROL,VK_ESCAPE);
      RegisterHotKey(StemWin,Base++,MOD_CONTROL | MOD_ALT,VK_ESCAPE);
      RegisterHotKey(StemWin,Base++,MOD_CONTROL | MOD_SHIFT,VK_ESCAPE);
      RegisterHotKey(StemWin,Base++,MOD_CONTROL | MOD_ALT | MOD_SHIFT,VK_ESCAPE);

      RegisterHotKey(StemWin,Base++,MOD_CONTROL | MOD_ALT,VK_DELETE);
      RegisterHotKey(StemWin,Base++,MOD_CONTROL | MOD_ALT | MOD_SHIFT,VK_DELETE);

      RegisterHotKey(StemWin,Base++,MOD_SHIFT,VK_ESCAPE);

#ifdef ONEGAME
      RegisterHotKey(StemWin,Base++,0,VK_LWIN);
      RegisterHotKey(StemWin,Base++,0,VK_RWIN);
#endif
    }
  }else{
    UINT PrevSS;
    SystemParametersInfo(SPI_SETSCREENSAVERRUNNING,TRUE,&PrevSS,0);
  }
  TaskSwitchDisabled=true;
}
//---------------------------------------------------------------------------
void EnableTaskSwitch()
{
  if (TaskSwitchDisabled==0) return;

  if (WinNT){
    if (hNTTaskSwitchHook==NULL){
      for (int n=1400;n<1411 ONEGAME_ONLY(+2);n++) UnregisterHotKey(StemWin,n);
    }
  }else{
    UINT PrevSS;
    SystemParametersInfo(SPI_SETSCREENSAVERRUNNING,FALSE,&PrevSS,0);
  }

  if (hNTTaskSwitchHook){
    UnhookWindowsHookEx(hNTTaskSwitchHook);
    hNTTaskSwitchHook=NULL;
  }
  TaskSwitchDisabled=0;
}
//---------------------------------------------------------------------------
inline bool HandleMessage(MSG *mess)
{
  if (DiskMan.HasHandledMessage(mess))     return 0;
  if (HardDiskMan.HasHandledMessage(mess)) return 0;
  if (JoyConfig.HasHandledMessage(mess))   return 0;
  if (InfoBox.HasHandledMessage(mess))     return 0;
  if (OptionBox.HasHandledMessage(mess))   return 0;
  if (ShortcutBox.HasHandledMessage(mess)) return 0;
  if (PatchesBox.HasHandledMessage(mess)) return 0;
  return true;
}
//---------------------------------------------------------------------------
int ASMCALL PeekEvent()
{
  static MSG mess;
  if (PeekMessage(&mess,NULL,0,0,PM_REMOVE)==0) return PEEKED_NOTHING;

  if (mess.message==WM_QUIT){
    QuitSteem();
    return PEEKED_QUIT;
  }

  if (HandleMessage(&mess)){
    TranslateMessage(&mess);
    DispatchMessage(&mess);
  }
  return PEEKED_MESSAGE;
}
//---------------------------------------------------------------------------
#if !defined(SSE_GUI_NO_PASTE)
void UpdatePasteButton()
{
#ifdef RELEASE_BUILD
  // Only have automatic updating of paste button if final build
  if (PasteText.Empty()) EnableWindow(GetDlgItem(StemWin,114),IsClipboardFormatAvailable(CF_TEXT));
#else
  EnableWindow(GetDlgItem(StemWin,114),true);
#endif
}
#endif
//---------------------------------------------------------------------------
#if !defined(RELEASE_BUILD) && defined(DEBUG_BUILD)
bool HWNDNotValid(HWND Win,char *File,int Line)
{
  bool Err=0;
  if (Win==NULL){
    Err=true;
  }else if (IsWindow(Win)==0){
    Err=true;
  }
  if (Err){
    Alert(Str("WINDOWS: Arrghh, using ")+long(Win)+" as HWND in file "+File+" at line "+Line,"Window Handle Error",MB_ICONEXCLAMATION);
    return true;
  }
  return 0;
}

LRESULT SendMessage_checkforbugs(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar,char *File,int Line)
{
  if (HWNDNotValid(Win,File,Line)) return 0;
  return SendMessageA(Win,Mess,wPar,lPar);
}

BOOL PostMessage_checkforbugs(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar,char *File,int Line)
{
  if (HWNDNotValid(Win,File,Line)) return 0;
  return PostMessageA(Win,Mess,wPar,lPar);
}

#endif

#endif //WIN32

//---------------------------------------------------------------------------
int Alert(char *Mess,char *Title,UINT Flags)
{
  WIN_ONLY( HWND Win=GetActiveWindow(); )
  Disp.FlipToDialogsScreen();
  log_write(EasyStr(Title)+": "+Mess);
  int Ret=MessageBox(WINDOWTYPE(FullScreen ? StemWin:0),Mess,Title,Flags | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND);
  WIN_ONLY( SetActiveWindow(Win); )
  return Ret;
}
//---------------------------------------------------------------------------
// Shift and alt switching, use 4 tables of 256 WORDs:
//  table 0 = no shift, no alt
//  table 1 = shift, no alt
//  table 2 = no shift, alt
//  table 3 = shift, alt
// Look up our key (VKCode) in the table, if it isn't 0 then shift/alt switching
// must be performed. The LOBYTE contains the ST key code, the high byte has
// bit 0 set if shift should be down when the key is sent and bit 1 for alt to
// be down. Before the key is pressed we must change the ST shift and alt states
// to what we need by sending releasing/press IKBD messages for those keys.
// After the key has been sent we then send more messages to restore them to
// their former glory. Surprisingly it works great, except if you try to do any
// key repeat.

void ShiftSwitchChangeModifiers(bool ShiftShouldBePressed,bool AltShouldBePressed,int ModifierRestoreArray[3])
{
  // Get current states
#if defined(SSE_VS2008_WARNING_390)
  BYTE STLShiftDown=(ST_Key_Down[key_table[VK_LSHIFT]]);
  BYTE STRShiftDown=(ST_Key_Down[key_table[VK_RSHIFT]]);
#else
  bool STLShiftDown=(ST_Key_Down[key_table[VK_LSHIFT]]);
  bool STRShiftDown=(ST_Key_Down[key_table[VK_RSHIFT]]);
#endif
  int STAltDown=(ST_Key_Down[key_table[VK_MENU]] ? BIT_1:0);

  if ((STLShiftDown || STRShiftDown) && ShiftShouldBePressed==0){
    // Send Shift Up Messages
    if (STLShiftDown){
      keyboard_buffer_write_n_record(key_table[VK_LSHIFT] | MSB_B);
      ModifierRestoreArray[0]=1; //Lshift down
    }
    if (STRShiftDown){
      keyboard_buffer_write_n_record(key_table[VK_RSHIFT] | MSB_B);
      ModifierRestoreArray[1]=1; //Rshift down
    }
  }else if ((STLShiftDown || STRShiftDown)==0 && ShiftShouldBePressed){
    // Send Shift Down Message
    keyboard_buffer_write_n_record(key_table[VK_LSHIFT]);
    ModifierRestoreArray[0]=2; //Lshift up
  }
  if (STAltDown && AltShouldBePressed==0){
    // Send Alt Up Messages
    keyboard_buffer_write_n_record(key_table[VK_MENU] | MSB_B);
    ModifierRestoreArray[2]=1; //Alt down
  }else if (STAltDown==0 && AltShouldBePressed){
    // Send Alt Down Message
    keyboard_buffer_write_n_record(key_table[VK_MENU]);
    ModifierRestoreArray[2]=2; //Alt up
  }
}
//---------------------------------------------------------------------------
void ShiftSwitchRestoreModifiers(int ModifierRestoreArray[3])
{
  if (ModifierRestoreArray[0]==1) keyboard_buffer_write_n_record(key_table[VK_LSHIFT]);
  if (ModifierRestoreArray[0]==2) keyboard_buffer_write_n_record(key_table[VK_LSHIFT] | MSB_B);
  if (ModifierRestoreArray[1]==1) keyboard_buffer_write_n_record(key_table[VK_RSHIFT]);
  if (ModifierRestoreArray[2]==1) keyboard_buffer_write_n_record(key_table[VK_MENU]);
  if (ModifierRestoreArray[2]==2) keyboard_buffer_write_n_record(key_table[VK_MENU] | MSB_B);
}
//---------------------------------------------------------------------------
#if defined(SSE_VS2008_WARNING_390)
void HandleShiftSwitching(UINT VKCode,DWORD Up,BYTE &STCode,int ModifierRestoreArray[3])
#else
void HandleShiftSwitching(UINT VKCode,bool Up,BYTE &STCode,int ModifierRestoreArray[3])
#endif
{
  // These are set to tell the HandleKeyPress routine what to do after it has
  // sent the key.
  ModifierRestoreArray[0]=0;  // LShift
  ModifierRestoreArray[1]=0;  // RShift
  ModifierRestoreArray[2]=0;  // Alt (only one on ST)

  // Don't need to do anything when you release the key
  if (shift_key_table[0]==NULL) return;

  // Get ST code and required modifier states
  int Shift,Alt;
  if (Up==0){ // Pressing key
    // Get current state of modifiers
    Shift=int((ST_Key_Down[key_table[VK_LSHIFT]] || ST_Key_Down[key_table[VK_RSHIFT]]) ? BIT_0:0);
    Alt=int(ST_Key_Down[key_table[VK_MENU]] ? BIT_1:0);

  }else{ // Releasing key
    // Get state of modifiers when key was pressed
    Shift=(KeyDownModifierState[BYTE(VKCode)] & BIT_0);
    Alt=(KeyDownModifierState[BYTE(VKCode)] & BIT_1);
  }
  WORD KeyEntry=shift_key_table[Shift | Alt][BYTE(VKCode)];
  STCode=LOBYTE(KeyEntry);
  KeyDownModifierState[BYTE(VKCode)]=BYTE(Shift | Alt);
  if (STCode && Up==0){
#if defined(SSE_VS2008_WARNING_390)
    bool ShiftShouldBePressed=(HIBYTE(KeyEntry) & BIT_0)!=0;
    bool AltShouldBePressed=(HIBYTE(KeyEntry) & BIT_1)!=0;
#else
    bool ShiftShouldBePressed=(HIBYTE(KeyEntry) & BIT_0);
    bool AltShouldBePressed=(HIBYTE(KeyEntry) & BIT_1);
#endif
    ShiftSwitchChangeModifiers(ShiftShouldBePressed,AltShouldBePressed,ModifierRestoreArray);
  }
}
//---------------------------------------------------------------------------
#if !defined(SSE_GUI_NO_PASTE)
void PasteIntoSTAction(int Action)
{
  if (Action==STPASTE_STOP || Action==STPASTE_TOGGLE){
    if (PasteText.NotEmpty()){
      PasteText="";
      PasteVBLCount=0;
      WIN_ONLY( SendDlgItemMessage(StemWin,114,BM_SETCHECK,0,0); )
      UNIX_ONLY( PasteBut.set_check(0); )
      return;
    }else{
      if (Action==STPASTE_STOP) return;
    }
  }

#if defined(WIN32)
  if (IsClipboardFormatAvailable(CF_TEXT)==0) return;
  if (OpenClipboard(StemWin)==0) return;

  HGLOBAL hGbl=GetClipboardData(CF_TEXT);
  if (hGbl){
    PasteText=(char*)GlobalLock(hGbl);
    PasteVBLCount=PasteSpeed;
#if defined(SSE_IKBD_6301_PASTE)
    bPastingText=true;
#endif
    SendDlgItemMessage(StemWin,114,BM_SETCHECK,1,0);
    GlobalUnlock(hGbl);
  }
  CloseClipboard();
#elif defined(UNIX)
  Window SelectionOwner=XGetSelectionOwner(XD,XA_PRIMARY);
  if (SelectionOwner!=None){
    XEvent SendEv;
    SendEv.type=SelectionRequest;
    SendEv.xselectionrequest.requestor=StemWin;
    SendEv.xselectionrequest.owner=SelectionOwner;
    SendEv.xselectionrequest.selection=XA_PRIMARY;
    SendEv.xselectionrequest.target=XA_STRING;
    SendEv.xselectionrequest.property=XA_CUT_BUFFER0;
    SendEv.xselectionrequest.time=CurrentTime;
    XSendEvent(XD,SelectionOwner,0,0,&SendEv);
    // PasteText,PasteVBLCount and PasteBut are set up
    // in SelectionNotify event handler
	}
#endif
}
//---------------------------------------------------------------------------
void PasteVBL()
{
  if (PasteText.NotEmpty()){
    if ((--PasteVBLCount)<=0){
      // Convert to ST Ascii
      BYTE c=BYTE(PasteText[0]);
      if (c>127){
        c=STCharToPCChar[c-128];
        if (c) PasteText[0]=char(c);
      }

      // Go through every character TOS can produce to find it
      switch (c){
        case '\r': break; // Only need line feeds
        case '\n':
          keyboard_buffer_write_n_record(0x1c);
          keyboard_buffer_write_n_record(BYTE(0x1c | BIT_7));
          break;
        case '\t':
          keyboard_buffer_write_n_record(0x0f);
          keyboard_buffer_write_n_record(BYTE(0x0f | BIT_7));
          break;
        case ' ':
          keyboard_buffer_write_n_record(0x39);
          keyboard_buffer_write_n_record(BYTE(0x39 | BIT_7));
          break;
        default:
          DynamicArray<DWORD> Chars;
          GetAvailablePressChars(&Chars);
          for (int n=0;n<Chars.NumItems;n++){
            if (HIWORD(Chars[n])==c){
              // Now fix shift/alt and press the key
              int ModifierRestoreArray[3]={0,0,0};
              BYTE STCode=LOBYTE(LOWORD(Chars[n]));
              BYTE Modifiers=HIBYTE(LOWORD(Chars[n]));
#if defined(SSE_VS2008_WARNING_390)
              ShiftSwitchChangeModifiers((Modifiers & BIT_0)!=0,
                (Modifiers & BIT_1)!=0,ModifierRestoreArray);
#else
              ShiftSwitchChangeModifiers(Modifiers & BIT_0,Modifiers & BIT_1,ModifierRestoreArray);
#endif
              keyboard_buffer_write_n_record(STCode);
              keyboard_buffer_write_n_record(BYTE(STCode | BIT_7));
              ShiftSwitchRestoreModifiers(ModifierRestoreArray);
              break;
            }
          }
      }
      PasteText.Delete(0,1);
      if (PasteText.NotEmpty()){
        PasteVBLCount=PasteSpeed;
      }else{
        PasteText=""; // Release some memory
        WIN_ONLY( SendDlgItemMessage(StemWin,114,BM_SETCHECK,0,0); )
        UNIX_ONLY( PasteBut.set_check(0); )
      }
    }
  }
}
#endif
//---------------------------------------------------------------------------
void UpdateSTKeys()
{
	for (int n=0;n<128;n++){
    if (ST_Key_Down[n]){
#if defined(SSE_IKBD_6301)
      if(!OPTION_C1)
#endif
      keyboard_buffer_write(BYTE(n | BIT_7));

      ST_Key_Down[n]=0;
    }
  }
}
//---------------------------------------------------------------------------

#ifdef UNIX
#include "x/x_gui.cpp"
#endif

