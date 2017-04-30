#pragma once
#ifndef OPTIONS_DECLA_H
#define OPTIONS_DECLA_H

#define EXT extern
#define INIT(s)

//---------------------------------------------------------------------------

#if defined(SSE_COMPILER_STRUCT_391)

#pragma pack(push, STRUCTURE_ALIGNMENT)

class TOptionBox : public TStemDialog
{
public: //TODO
  
#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall Fullscreen_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall GroupBox_WndProc(HWND,UINT,WPARAM,LPARAM);
  static int DTreeNotifyProc(DirectoryTree*,void*,int,int,int);
  void DestroyCurrentPage();
  void ManageWindowClasses(bool);
  void AssAddToExtensionsLV(char *,char *,int);
  void DrawBrightnessBitmap(HBITMAP);
  void CreateBrightnessBitmap();
  void PortsMakeTypeVisible(int);

  HIMAGELIST il;
  HBITMAP hBrightBmp;
  WNDPROC Old_GroupBox_WndProc;
  HWND BorderOption;
#if !defined(SSE_VAR_REFACTOR_392)
#if defined(SSE_STF) 
  HWND STTypeOption;
#endif
#if defined(SSE_MMU_WU)
  HWND MMUWakeUpOption;
#endif
#endif//ref
#if defined(SSE_VID_BORDERS) && !defined(SSE_VID_BORDERS_GUI_392)
  HWND BorderSizeOption;
#endif
  EasyStr WAVOutputDir;
  Str TOSBrowseDir,LastCartFile;
  Str NewROMFile;
  Str MacroDir,MacroSel;
  Str ProfileDir,ProfileSel;
  Str LastIconPath,LastIconSchemePath;
  int page_l,page_w;
  int Page;
  int NewMemConf0,NewMemConf1,NewMonitorSel;
  ESLSortEnum eslTOS_Sort;
  EasyStringList eslTOS;
  ScrollControlWin Scroller;
  static DirectoryTree DTree;
  bool eslTOS_Descend;

  //WIN32
#elif defined(UNIX)
  static int WinProc(TOptionBox*,Window,XEvent*);
	static int listview_notify_proc(hxc_listview*,int,int);
  static int dd_notify_proc(hxc_dropdown*,int,int);
  static int button_notify_proc(hxc_button*,int,int*);
	static int edit_notify_proc(hxc_edit *,int,int);
	static int scrollbar_notify_proc(hxc_scrollbar*,int,int);
  static int dir_lv_notify_proc(hxc_dir_lv*,int,int);

  void DrawBrightnessBitmap(XImage*),UpdateProfileDisplay(Str="",int=-1);
  void FillSoundDevicesDD();
  Str TOSBrowseDir,LastCartFile;
  int page_p;
  int Page;
  int page_l,page_w;

  hxc_listview page_lv;
  hxc_button control_parent;

  hxc_button cpu_boost_label,pause_inactive_but;
  hxc_dropdown cpu_boost_dd;

	hxc_button memory_label,monitor_label,tos_group;
  hxc_dropdown memory_dd,monitor_dd;
	hxc_button cart_group,cart_display,cart_change_but,cart_remove_but;
	hxc_button keyboard_language_label,keyboard_sc_but;
  hxc_dropdown keyboard_language_dd;
	hxc_button coldreset_but;
  hxc_textdisplay mustreset_td;

  hxc_dropdown tos_sort_dd;
  hxc_listview tos_lv;
	hxc_button tosadd_but,tosrefresh_but;

  hxc_button PortGroup[3],ConnectLabel[3];
  hxc_dropdown ConnectDD[3];
  hxc_button IOGroup[3],IOChooseBut[3],IOAllowIOBut[3][2],IOOpenBut[3];
  hxc_edit IODevEd[3];
  hxc_button LANGroup[3];
  hxc_button FileGroup[3],FileDisplay[3],FileChooseBut[3],FileEmptyBut[3];
  
  hxc_button high_priority_but,start_click_but;
  hxc_button FFMaxSpeedLabel,SMSpeedLabel,RunSpeedLabel;
  hxc_scrollbar FFMaxSpeedSB,SMSpeedSB,RunSpeedSB;
  hxc_button ff_on_fdc_but;

  hxc_button fs_label;hxc_dropdown frameskip_dd;
  hxc_button bo_label;hxc_dropdown border_dd;
  hxc_button size_group,reschangeresize_but;
  hxc_button lowres_doublesize_but,medres_doublesize_but;
  hxc_button screenshots_group,screenshots_fol_display;
  hxc_button screenshots_fol_label,screenshots_fol_but;

  hxc_button sound_group,sound_mode_label,sound_freq_label,sound_format_label;
  hxc_dropdown sound_mode_dd,sound_freq_dd,sound_format_dd;
	hxc_button device_label,record_group,record_but;
	hxc_button wav_output_label,wav_choose_but,overwrite_ask_but;
  hxc_edit device_ed;

  hxc_listview profile_sect_lv;

  IconGroup brightness_ig;
  XImage *brightness_image;
  hxc_button brightness_picture,brightness_picture_label;
  hxc_button brightness_label;
  hxc_scrollbar brightness_sb;
  hxc_button contrast_label;
  hxc_scrollbar contrast_sb;

  hxc_button auto_sts_but;
  hxc_button auto_sts_filename_label;
  hxc_edit auto_sts_filename_edit;
  hxc_button no_shm_but;

  hxc_button osd_disable_but;

  hxc_listview drop_lv;

  static hxc_dir_lv dir_lv;
  int NewMemConf0,NewMemConf1,NewMonitorSel;
  Str NewROMFile;

  ESLSortEnum eslTOS_Sort;
  bool eslTOS_Descend;

  Str MacroDir,MacroSel;
  Str ProfileDir,ProfileSel;

  Str LastIconPath,LastIconSchemePath;


#endif//WIN/UNIX
  bool RecordWarnOverwrite;
  static bool USDateFormat;

  void FullscreenBrightnessBitmap();

  
public:
  TOptionBox();
  ~TOptionBox() { Hide(); }
  void Show(),Hide();
  bool ToggleVisible(){ IsVisible() ? Hide():Show();return IsVisible(); }
  void EnableBorderOptions(bool);
  bool ChangeBorderModeRequest(int);
  void ChangeOSDDisable(bool);
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);
  void CreatePage(int);
  void CreateMachinePage(),CreateTOSPage(),CreateGeneralPage(),CreatePortsPage();
  void CreateSoundPage(),CreateDisplayPage(),CreateBrightnessPage();
  void CreateMacrosPage(),CreateProfilesPage(),CreateStartupPage(),CreateOSDPage();
#if defined(SSE_GUI_OPTION_PAGE)
  void CreateSSEPage();
#endif

#ifdef WIN32
  void CreateFullscreenPage(),CreateMIDIPage();
  void 
#ifndef SSE_VAR_NO_UPDATE
    CreateUpdatePage(),
#endif
    CreateAssocPage();
  void IconsAddToScroller(),CreateIconsPage();
#else
  void CreatePathsPage();
#endif

  void UpdateSoundFreq();
  void ChangeSoundFormat(BYTE,BYTE);
  void UpdateRecordBut();
  void SetRecord(bool);
  void SoundModeChange(int,bool,bool);
  void UpdateMacroRecordAndPlay(Str="",int=0);
  Str CreateMacroFile(bool);
  void LoadProfile(char*);

#if defined(WIN32)
  bool HasHandledMessage(MSG *);
  void LoadIcons();
  void ChangeScreenShotFormat(int,Str);
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  void ChangeScreenShotFormatOpts(int);
#endif
  void ChooseScreenShotFolder(HWND);

  static BOOL CALLBACK EnumDateFormatsProc(char *);
#if !defined(SSE_VID_D3D_ONLY)
  void UpdateHzDisplay();
#endif
  void UpdateWindowSizeAndBorder();
#if defined(SSE_VID_DISABLE_AUTOBORDER) && defined(SSE_VS2008_WARNING_390) \
  && !defined(SSE_VID_BORDERS_GUI_392)
  void SetBorder(bool);
#else
  void SetBorder(int);
#endif
  void UpdateForDSError();
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  void FillScreenShotFormatOptsCombo();
#endif
  void UpdateParallel();

#elif defined(UNIX)
  void UpdatePortDisplay(int);
  hxc_button internal_speaker_but; // changed in Sound_Start
#endif

#if defined(SSE_UNIX)

#if defined(SSE_VID_BORDERS)
  hxc_button border_size_label; 
  hxc_dropdown border_size_dd;
#endif

#if defined(SSE_GUI_MOUSE_CAPTURE_OPTION)
  hxc_button capture_mouse_but;
#endif

#if defined(SSE_HACKS)
  hxc_button specific_hacks_but;
#endif

#if defined(SSE_VAR_EMU_DETECT) 
  hxc_button stealth_mode_but;
#endif

#if defined(SSE_STF)
  hxc_button st_type_label;
  hxc_dropdown st_type_dd;
#endif

#if defined(SSE_MMU_WU)
  hxc_button wake_up_label; 
  hxc_dropdown wake_up_dd;
#endif

#if defined(SSE_IKBD_6301) 
  hxc_button hd6301emu_but;
#endif

#if defined(SSE_INT_MFP_OPTION)
  hxc_button mc68901_but;
#endif

#if defined(SSE_TOS_KEYBOARD_CLICK)
  hxc_button keyboard_click_but; 
#endif

#if defined(SSE_YM2149_FIX_TABLES) 
  hxc_button psg_fixtables_but;
#endif

#if defined(SSE_YM2149_FIXED_VOL_TABLE)
  hxc_button psg_samples_but;
#endif

#if defined(SSE_SOUND_MICROWIRE)
  hxc_button ste_microwire_but;
#endif

#if defined(SSE_VID_SDL) && !defined(SSE_VID_SDL_DEACTIVATE)
  hxc_button use_sdl_but;    
#endif

#endif

  void MachineUpdateIfVisible();
#if defined(SSE_GUI_OPTIONS_REFRESH)
  void SSEUpdateIfVisible();
#endif

  void TOSRefreshBox(EasyStr="");
  bool NeedReset() { return NewMemConf0>=0 || NewMonitorSel>=0 || NewROMFile.NotEmpty(); }
  int GetCurrentMonitorSel();
	int TOSLangToFlagIdx(int);


};//class TOptionBox

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#else

class TOptionBox : public TStemDialog
{
private:
  int page_l,page_w;
#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall Fullscreen_WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall GroupBox_WndProc(HWND,UINT,WPARAM,LPARAM);
  static int DTreeNotifyProc(DirectoryTree*,void*,int,int,int);

  void DestroyCurrentPage();
  void ManageWindowClasses(bool);

  void AssAddToExtensionsLV(char *,char *,int);
  void DrawBrightnessBitmap(HBITMAP);
  void CreateBrightnessBitmap();
  void PortsMakeTypeVisible(int);

  HIMAGELIST il;
  HBITMAP hBrightBmp;
  ScrollControlWin Scroller;
  WNDPROC Old_GroupBox_WndProc;
  static DirectoryTree DTree;
  //WIN32
#elif defined(UNIX)
  static int WinProc(TOptionBox*,Window,XEvent*);
	static int listview_notify_proc(hxc_listview*,int,int);
  static int dd_notify_proc(hxc_dropdown*,int,int);
  static int button_notify_proc(hxc_button*,int,int*);
	static int edit_notify_proc(hxc_edit *,int,int);
	static int scrollbar_notify_proc(hxc_scrollbar*,int,int);
  static int dir_lv_notify_proc(hxc_dir_lv*,int,int);

  void DrawBrightnessBitmap(XImage*),UpdateProfileDisplay(Str="",int=-1);
  void FillSoundDevicesDD();

  int page_p;

  hxc_listview page_lv;
  hxc_button control_parent;

  hxc_button cpu_boost_label,pause_inactive_but;
  hxc_dropdown cpu_boost_dd;

	hxc_button memory_label,monitor_label,tos_group;
  hxc_dropdown memory_dd,monitor_dd;
	hxc_button cart_group,cart_display,cart_change_but,cart_remove_but;
	hxc_button keyboard_language_label,keyboard_sc_but;
  hxc_dropdown keyboard_language_dd;
	hxc_button coldreset_but;
  hxc_textdisplay mustreset_td;

  hxc_dropdown tos_sort_dd;
  hxc_listview tos_lv;
	hxc_button tosadd_but,tosrefresh_but;

  hxc_button PortGroup[3],ConnectLabel[3];
  hxc_dropdown ConnectDD[3];
  hxc_button IOGroup[3],IOChooseBut[3],IOAllowIOBut[3][2],IOOpenBut[3];
  hxc_edit IODevEd[3];
  hxc_button LANGroup[3];
  hxc_button FileGroup[3],FileDisplay[3],FileChooseBut[3],FileEmptyBut[3];
  
  hxc_button high_priority_but,start_click_but;
  hxc_button FFMaxSpeedLabel,SMSpeedLabel,RunSpeedLabel;
  hxc_scrollbar FFMaxSpeedSB,SMSpeedSB,RunSpeedSB;
  hxc_button ff_on_fdc_but;

  hxc_button fs_label;hxc_dropdown frameskip_dd;
  hxc_button bo_label;hxc_dropdown border_dd;
  hxc_button size_group,reschangeresize_but;
  hxc_button lowres_doublesize_but,medres_doublesize_but;
  hxc_button screenshots_group,screenshots_fol_display;
  hxc_button screenshots_fol_label,screenshots_fol_but;

  hxc_button sound_group,sound_mode_label,sound_freq_label,sound_format_label;
  hxc_dropdown sound_mode_dd,sound_freq_dd,sound_format_dd;
	hxc_button device_label,record_group,record_but;
	hxc_button wav_output_label,wav_choose_but,overwrite_ask_but;
  hxc_edit device_ed;

  hxc_listview profile_sect_lv;

  IconGroup brightness_ig;
  XImage *brightness_image;
  hxc_button brightness_picture,brightness_picture_label;
  hxc_button brightness_label;
  hxc_scrollbar brightness_sb;
  hxc_button contrast_label;
  hxc_scrollbar contrast_sb;

  hxc_button auto_sts_but;
  hxc_button auto_sts_filename_label;
  hxc_edit auto_sts_filename_edit;
  hxc_button no_shm_but;

  hxc_button osd_disable_but;

  hxc_listview drop_lv;

  static hxc_dir_lv dir_lv;
#endif//WIN/UNIX
  void FullscreenBrightnessBitmap();

  EasyStr WAVOutputDir;
public:
  TOptionBox();
  ~TOptionBox() { Hide(); }
  void Show(),Hide();
  bool ToggleVisible(){ IsVisible() ? Hide():Show();return IsVisible(); }
  void EnableBorderOptions(bool);
  bool ChangeBorderModeRequest(int);
  void ChangeOSDDisable(bool);
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);

  void CreatePage(int);
  void CreateMachinePage(),CreateTOSPage(),CreateGeneralPage(),CreatePortsPage();
  void CreateSoundPage(),CreateDisplayPage(),CreateBrightnessPage();
  void CreateMacrosPage(),CreateProfilesPage(),CreateStartupPage(),CreateOSDPage();
#if defined(SSE_GUI_OPTION_PAGE)
  void CreateSSEPage();
#endif

#ifdef WIN32
  void CreateFullscreenPage(),CreateMIDIPage();
  void 
#ifndef SSE_VAR_NO_UPDATE
    CreateUpdatePage(),
#endif
    CreateAssocPage();
  void IconsAddToScroller(),CreateIconsPage();
#else
  void CreatePathsPage();
#endif

  void UpdateSoundFreq();
  void ChangeSoundFormat(BYTE,BYTE);
  void UpdateRecordBut();
  void SetRecord(bool);
  void SoundModeChange(int,bool,bool);
  void UpdateMacroRecordAndPlay(Str="",int=0);
  Str CreateMacroFile(bool);
  void LoadProfile(char*);

  int Page;
  bool RecordWarnOverwrite;
  static bool USDateFormat;
  Str TOSBrowseDir,LastCartFile;

#if defined(WIN32)
  bool HasHandledMessage(MSG *);

  void LoadIcons();
  void ChangeScreenShotFormat(int,Str);
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  void ChangeScreenShotFormatOpts(int);
#endif
  void ChooseScreenShotFolder(HWND);

  static BOOL CALLBACK EnumDateFormatsProc(char *);
#if !defined(SSE_VID_D3D_ONLY)
  void UpdateHzDisplay();
#endif
  void UpdateWindowSizeAndBorder();
#if defined(SSE_VID_DISABLE_AUTOBORDER) && defined(SSE_VS2008_WARNING_390)
  void SetBorder(bool);
#else
  void SetBorder(int);
#endif
  void UpdateForDSError();
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  void FillScreenShotFormatOptsCombo();
#endif
  void UpdateParallel();

  HWND BorderOption;
#if defined(SSE_STF)
  HWND STTypeOption;
#endif
#if defined(SSE_MMU_WU)
  HWND MMUWakeUpOption;
#endif
#if defined(SSE_VID_BORDERS)
  HWND BorderSizeOption;
#endif
#elif defined(UNIX)
  void UpdatePortDisplay(int);
  hxc_button internal_speaker_but; // changed in Sound_Start
#endif

#if defined(SSE_UNIX)

#if defined(SSE_VID_BORDERS)
  hxc_button border_size_label; 
  hxc_dropdown border_size_dd;
#endif

#if defined(SSE_GUI_MOUSE_CAPTURE_OPTION)
  hxc_button capture_mouse_but;
#endif

#if defined(SSE_HACKS)
  hxc_button specific_hacks_but;
#endif

#if defined(SSE_VAR_EMU_DETECT) 
  hxc_button stealth_mode_but;
#endif

#if defined(SSE_STF)
  hxc_button st_type_label;
  hxc_dropdown st_type_dd;
#endif

#if defined(SSE_MMU_WU)
  hxc_button wake_up_label; 
  hxc_dropdown wake_up_dd;
#endif

#if defined(SSE_IKBD_6301) 
  hxc_button hd6301emu_but;
#endif

#if defined(SSE_INT_MFP_OPTION)
  hxc_button mc68901_but;
#endif

#if defined(SSE_TOS_KEYBOARD_CLICK)
  hxc_button keyboard_click_but; 
#endif

#if defined(SSE_YM2149_FIX_TABLES) 
  hxc_button psg_fixtables_but;
#endif

#if defined(SSE_YM2149_FIXED_VOL_TABLE)
  hxc_button psg_samples_but;
#endif

#if defined(SSE_SOUND_MICROWIRE)
  hxc_button ste_microwire_but;
#endif

#if defined(SSE_VID_SDL) && !defined(SSE_VID_SDL_DEACTIVATE)
  hxc_button use_sdl_but;    
#endif

#endif

  void MachineUpdateIfVisible();
#if defined(SSE_GUI_OPTIONS_REFRESH)
  void SSEUpdateIfVisible();
#endif

  void TOSRefreshBox(EasyStr="");
  bool NeedReset() { return NewMemConf0>=0 || NewMonitorSel>=0 || NewROMFile.NotEmpty(); }
  int GetCurrentMonitorSel();
	int TOSLangToFlagIdx(int);

  int NewMemConf0,NewMemConf1,NewMonitorSel;
  Str NewROMFile;

  WIN_ONLY( EasyStringList eslTOS; )
  ESLSortEnum eslTOS_Sort;
  bool eslTOS_Descend;

  Str MacroDir,MacroSel;
  Str ProfileDir,ProfileSel;

  Str LastIconPath,LastIconSchemePath;
};//class TOptionBox

#endif//#if defined(SSE_COMPILER_STRUCT_391)



EXT EasyStr WAVOutputFile;
EXT EasyStringList DSDriverModuleList;


#if defined(SSE_VID_EXT_MON_1280X1024) || defined(SSE_VID_EXT_FS2)
#if defined(SSE_VID_EXT_MON_1024X720)
#define EXTMON_RESOLUTIONS (7+2+2)
#else
#define EXTMON_RESOLUTIONS (7+2)
#endif
#else
#define EXTMON_RESOLUTIONS 7
#endif

EXT
#if !defined(SSE_VID_EXT_FS2)
const
#endif
#if defined(SSE_VAR_RESIZE)
WORD
#else
int 
#endif
 extmon_res[EXTMON_RESOLUTIONS][3];

/*
#if defined(SSE_VID_EXT_FS2)
#if defined(SSE_VAR_RESIZE)
EXT WORD
#else
EXT int 
#endif
 extmon_res[EXTMON_RESOLUTIONS][3];
#else
EXT const int extmon_res[EXTMON_RESOLUTIONS][3];
#endif
*/
#undef EXT
#undef INIT

#endif//#ifndef OPTIONS_DECLA_H
