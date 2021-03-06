/*---------------------------------------------------------------------------
FILE: options_create.cpp
MODULE: Steem
DESCRIPTION: Functions to create the pages of the options dialog box.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: options_create.cpp")
#endif
#if defined(SSE_BUILD)
#include "SSE\SSESTF.h"
#include "SSE\SSE6301.h"
#include "SSE\SSEOption.h"
#include "SSE\SSEInterrupt.h"
#include "display.decla.h"
#endif

//---------------------------------------------------------------------------
void TOptionBox::CreatePage(int n)
{
  switch (n){
    case 9:CreateMachinePage();break;
    case 10:CreateTOSPage();break;
#if !defined(SSE_GUI_NO_MACROS)
    case 13:CreateMacrosPage();break;
#endif
    case 12:CreatePortsPage();break;
#if !defined(SSE_GUI_NO_MIDIOPTION)
    case 4:CreateMIDIPage();break;
#endif
    case 0:CreateGeneralPage();break;
    case 5:CreateSoundPage();break;
    case 1:CreateDisplayPage();break;
    case 15:CreateOSDPage();break;
    case 3:CreateFullscreenPage();break;
    case 2:CreateBrightnessPage();break;
#if !defined(SSE_GUI_NO_PROFILES)
    case 11:CreateProfilesPage();break;
#endif
    case 6:CreateStartupPage();break;
#if !defined(SSE_GUI_NO_ICONCHOICE)
    case 14:CreateIconsPage();break;
#endif
    case 8:CreateAssocPage();break;
#if !(defined(SSE_VAR_NO_UPDATE))
    case 7:CreateUpdatePage();break;
#endif
#if defined(SSE_GUI_OPTION_PAGE)
    case 16:CreateSSEPage();break;
#endif

  }
}
//---------------------------------------------------------------------------
void TOptionBox::CreateMachinePage()
{
  HWND Win;
  long Wid;
  int y=10;

#if defined(SSE_STF) && defined(SSE_GUI_OPTIONS_STF)
  Wid=get_text_width(T("ST model"));
  CreateWindow("Static",T("ST model"),WS_CHILD,
    page_l,y+4,Wid,21,Handle,(HMENU)209,HInstance,NULL);
#if defined(SSE_GUI_393)
  Win=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
    page_l+5+Wid,y,80+20,200,Handle,(HMENU)211,HInstance,NULL);
#else
  Win=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
    page_l+5+Wid,y,80,200,Handle,(HMENU)211,HInstance,NULL);
#endif
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT(st_model_name[STE]));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT(st_model_name[STF]));
  ADVANCED_BEGIN
#if defined(SSE_STF_MEGASTF)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT(st_model_name[MEGASTF]));
#endif
#if defined(SSE_STF_LACESCAN)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT(st_model_name[STF_LACESCAN]));
#endif
#if defined(SSE_STF_AUTOSWITCH)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT(st_model_name[STF_AUTOSWITCH]));
#endif
  ADVANCED_END
  SendMessage(Win,CB_SETCURSEL,min((int)ST_TYPE,N_ST_MODELS-1),0);
  Wid+=73;//by hand...
#if defined(SSE_GUI_390)
#if defined(SSE_STF_HW_OVERSCAN)
  ToolAddWindow(ToolTip,Win,
    T("The STE was more elaborated than the older STF but some programs are \
compatible only with the STF - see the manual for other options"));
#else //LE
  ToolAddWindow(ToolTip,Win,
    T("The STE was more elaborated than the older STF but some programs are \
compatible only with the STF"));
#endif
#else
  ToolAddWindow(ToolTip,Win,
    T("Some programs will run only with STF or STE. Changing ST model will preselect a TOS for next cold reset."));
#endif
#endif

ADVANCED_BEGIN
#if defined(SSE_GUI_OPTIONS_WU) && defined(SSE_MMU_WU)
  long Offset=Wid+40-20;
#if defined(SSE_GUI_393) //make shorter...
  Offset+=26;
  Wid=get_text_width(T("Wake-up"));
  CreateWindow("Static",T("Wake-up"),WS_CHILD,
    page_l+Offset,y+4,Wid,21,Handle,(HMENU)209,HInstance,NULL);
#else
  Wid=get_text_width(T("Wake-up state"));
  CreateWindow("Static",T("Wake-up state"),WS_CHILD,
    page_l+Offset,y+4,Wid,21,Handle,(HMENU)209,HInstance,NULL);
#endif
  Win=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
    page_l+5+Wid+Offset,y,85+20,200,Handle,(HMENU)212,HInstance,NULL);

  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Ignore"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("DL3 WU2 WS2"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("DL4 WU2 WS4"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("DL5 WU1 WS3"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("DL6 WU1 WS1"));
  SendMessage(Win,CB_SETCURSEL,OPTION_WS,0);

#if defined(SSE_GUI_390)
  ToolAddWindow(ToolTip,Win,
    T("Check Hints for cases"));
#else
  ToolAddWindow(ToolTip,Win,
    T("Some rare demos will display correctly only in one of those states."));
#endif
#endif
ADVANCED_END
  y+=30;
ADVANCED_BEGIN
#if !defined(SSE_GUI_NO_CPU_SPEED)
  Wid=get_text_width(T("ST CPU speed"));
  CreateWindow("Static",T("ST CPU speed"),WS_CHILD,page_l,y+4,Wid,23,Handle,(HMENU)403,HInstance,NULL);
  
  Win=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
	  page_l+5+Wid,y,page_w-(5+Wid),400,Handle,(HMENU)404,HInstance,NULL);
#if defined(SSE_CPU_HISPEED_392)
  EasyStr Mhz=T("Mhz");
#if defined(SSE_CPU_MFP_RATIO)
  CBAddString(Win,EasyStr("8 ")+Mhz+" ("+T("ST standard")+")",CpuNormalHz);
#else
  CBAddString(Win,EasyStr("8 ")+Mhz+" ("+T("ST standard")+")",8000000);
#endif
#else
  EasyStr Mhz=T("Megahertz");
  CBAddString(Win,EasyStr("8 ")+Mhz+" ("+T("ST standard")+")",8000000);
#endif
  CBAddString(Win,EasyStr("9 ")+Mhz,9000000);
  CBAddString(Win,EasyStr("10 ")+Mhz,10000000);
  CBAddString(Win,EasyStr("11 ")+Mhz,11000000);
  CBAddString(Win,EasyStr("12 ")+Mhz,12000000);
  CBAddString(Win,EasyStr("14 ")+Mhz,14000000);
  CBAddString(Win,EasyStr("16 ")+Mhz,16000000);
  CBAddString(Win,EasyStr("20 ")+Mhz,20000000);
  CBAddString(Win,EasyStr("24 ")+Mhz,24000000);
  CBAddString(Win,EasyStr("28 ")+Mhz,28000000);
  CBAddString(Win,EasyStr("32 ")+Mhz,32000000);
  CBAddString(Win,EasyStr("36 ")+Mhz,36000000);
  CBAddString(Win,EasyStr("40 ")+Mhz,40000000);
  CBAddString(Win,EasyStr("44 ")+Mhz,44000000);
  CBAddString(Win,EasyStr("48 ")+Mhz,48000000);
  CBAddString(Win,EasyStr("56 ")+Mhz,56000000);
  CBAddString(Win,EasyStr("64 ")+Mhz,64000000);
  CBAddString(Win,EasyStr("80 ")+Mhz,80000000);
  CBAddString(Win,EasyStr("96 ")+Mhz,96000000);
  CBAddString(Win,EasyStr("128 ")+Mhz,128000000);
#if defined(SSE_CPU_256MHZ)
  CBAddString(Win,EasyStr("256 ")+Mhz,256000000);
#endif
#if defined(SSE_CPU_512MHZ)
  CBAddString(Win,EasyStr("512 ")+Mhz,512000000);
#endif
#if defined(SSE_CPU_HISPEED_392)
#if defined(SSE_CPU_1GHZ)
  CBAddString(Win,EasyStr("1 Ghz"),1000000000);
#endif
#if defined(SSE_CPU_2GHZ)
  CBAddString(Win,EasyStr("2 Ghz"),2000000000);
#endif
#if defined(SSE_CPU_3GHZ) //no!
  CBAddString(Win,EasyStr("3 Ghz"),3000000000);
#endif
#if defined(SSE_CPU_4GHZ) //no!
  CBAddString(Win,EasyStr("4 Ghz"),4000000000);
#endif
#endif
  if (CBSelectItemWithData(Win,n_cpu_cycles_per_second)<0){
    EasyStr Cycles=n_cpu_cycles_per_second;
    Cycles=Cycles.Lefts(Cycles.Length()-6);

    SendMessage(Win,CB_SETCURSEL,CBAddString(Win,Cycles+" "+T("Megahertz"),n_cpu_cycles_per_second),0);
  }
  y+=30;
#endif

ADVANCED_END

  Wid=GetTextSize(Font,T("Memory size")).Width;
  CreateWindow("Static",T("Memory size"),WS_CHILD,page_l,y+4,Wid,20,Handle,HMENU(8090),HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+5+Wid,y,page_w-(5+Wid),200,Handle,(HMENU)8100,HInstance,NULL);

#if defined(SSE_MMU_256K)
ADVANCED_BEGIN
  CBAddString(Win,"256Kb",MAKELONG(MEMCONF_128,MEMCONF_128));
ADVANCED_END
#endif

  CBAddString(Win,"512Kb",MAKELONG(MEMCONF_512,MEMCONF_0));
  CBAddString(Win,"1 MB",MAKELONG(MEMCONF_512,MEMCONF_512));
  CBAddString(Win,"2 MB",MAKELONG(MEMCONF_2MB,MEMCONF_0));
#if defined(SSE_MMU_2560K)
ADVANCED_BEGIN
  CBAddString(Win,"2.5 MB",MAKELONG(MEMCONF_512,MEMCONF_2MB));
ADVANCED_END
#endif
  CBAddString(Win,"4 MB",MAKELONG(MEMCONF_2MB,MEMCONF_2MB));
ADVANCED_BEGIN
#if defined(SSE_MMU_MONSTER_ALT_RAM)
  CBAddString(Win,"12 MB (MonSTer alt-RAM)",MAKELONG(MEMCONF_6MB,MEMCONF_6MB));
#endif
#if !defined(SSE_GUI_NO14MB)
#ifdef  SSE_BUILD
  CBAddString(Win,"14 MB (hack)",MAKELONG(MEMCONF_7MB,MEMCONF_7MB));
#else  
  CBAddString(Win,"14 MB",MAKELONG(MEMCONF_7MB,MEMCONF_7MB));
#endif
#endif
ADVANCED_END
  y+=30;

  Wid=GetTextSize(Font,T("Monitor")).Width;
  CreateWindow("Static",T("Monitor"),WS_CHILD,page_l,y+4,Wid,20,Handle,HMENU(8091),HInstance,NULL);
#ifdef TEST02
  Win=CreateWindow("Combobox","",WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,
            page_l+5+Wid,y,page_w-(5+Wid),200,Handle,(HMENU)8200,HInstance,NULL);
#else
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
         page_l+5+Wid,y,page_w-(5+Wid),200,Handle,(HMENU)8200,HInstance,NULL);
#endif
  CBAddString(Win,T("Colour")+" ("+T("Low/Med Resolution")+")");
  CBAddString(Win,T("Monochrome")+" ("+T("High Resolution")+")");

#ifndef NO_CRAZY_MONITOR
ADVANCED_BEGIN
  for (int n=0;n<EXTMON_RESOLUTIONS;n++){
    CBAddString(Win,T("Extended Monitor At")+" "+extmon_res[n][0]+"x"+extmon_res[n][1]+"x"+extmon_res[n][2]);
  }
ADVANCED_END
#endif
  y+=30;

  CreateWindow("Button",T("Keyboard"),WS_CHILD | BS_GROUPBOX,
                  page_l,y,page_w,80,Handle,(HMENU)8093,HInstance,NULL);
  y+=20;

  Wid=GetTextSize(Font,T("Language")).Width;
  CreateWindow("Static",T("Language"),WS_CHILD,
                  page_l+10,y+4,Wid,25,Handle,(HMENU)8400,HInstance,NULL);

  HWND Combo=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+15+Wid,y,(page_w-20)-(5+Wid),200,Handle,(HMENU)8401,HInstance,NULL);
  CBAddString(Combo,T("United States"),MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US));
  CBAddString(Combo,T("United Kingdom"),MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_UK));
  CBAddString(Combo,T("Australia (UK TOS)"),MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_AUS));
  CBAddString(Combo,T("German"),MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN));
  CBAddString(Combo,T("French"),MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH));
  CBAddString(Combo,T("Spanish"),MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH));
  CBAddString(Combo,T("Italian"),MAKELANGID(LANG_ITALIAN,SUBLANG_ITALIAN));
  CBAddString(Combo,T("Swedish"),MAKELANGID(LANG_SWEDISH,SUBLANG_SWEDISH));
  CBAddString(Combo,T("Norwegian"),MAKELANGID(LANG_NORWEGIAN,SUBLANG_NEUTRAL));
  CBAddString(Combo,T("Belgian"),MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_BELGIAN));
  if (CBSelectItemWithData(Combo,KeyboardLangID)<0){
    SendMessage(Combo,CB_SETCURSEL,0,0);
  }
  y+=30;

  Wid=GetCheckBoxSize(Font,T("Shift and alternate correction")).Width;
  Win=CreateWindow("Button",T("Shift and alternate correction"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                          page_l+10,y,Wid,25,Handle,(HMENU)8402,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,EnableShiftSwitching,0);
  EnableWindow(Win,ShiftSwitchingAvailable);
  ToolAddWindow(ToolTip,Win,T("When checked this allows Steem to emulate all keys correctly, it does this by changing the shift and alternate state of the ST when you press them.")+" "+
                              T("This could interfere with games and other programs, only use it if you are doing lots of typing.")+" "+
                              T("Please note that instead of pressing Alt-Gr or Control to access characters on the right-hand side of a key, you have to press Alt or Alt+Shift (this is how it was done on an ST)."));
  y+=40;

#if !defined(SSE_NO_CARTRIDGE)
ADVANCED_BEGIN
  CreateWindow("Button",T("Cartridge"),WS_CHILD | BS_GROUPBOX,
                  page_l,y,page_w,80,Handle,(HMENU)8093,HInstance,NULL);
  y+=20;

  CreateWindowEx(512,"Steem Path Display","",WS_CHILD,
                  page_l+10,y,page_w-20,22,Handle,(HMENU)8500,HInstance,NULL);
  y+=30;
  
#if defined(SSE_CARTRIDGE_FREEZE) || defined(SSE_CARTRIDGE_TRANSPARENT)
  CreateWindow("Button",T("Choose"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10,y,(page_w-20)/4-5,23,Handle,(HMENU)8501,HInstance,NULL);

  CreateWindow("Button",T("Remove"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10+(page_w-20)/4,y,(page_w-20)/4-5,23,Handle,(HMENU)8502,HInstance,NULL);
#if defined(SSE_CARTRIDGE_TRANSPARENT)
  Win=CreateWindow("Button","",WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10+2*(page_w-20)/4,y,(page_w-20)/4-5,23,Handle,(HMENU)8504,HInstance,NULL);
#endif
#if defined(SSE_CARTRIDGE_FREEZE)
  CreateWindow("Button",T("Freeze"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10+3*(page_w-20)/4,y,(page_w-20)/4-5,23,Handle,(HMENU)8503,HInstance,NULL);
#endif
#else
  CreateWindow("Button",T("Choose"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10,y,(page_w-20)/2-5,23,Handle,(HMENU)8501,HInstance,NULL);

  CreateWindow("Button",T("Remove"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10+(page_w-20)/2+5,y,(page_w-20)/2-5,23,Handle,(HMENU)8502,HInstance,NULL);
#endif
  y+=40;
ADVANCED_END
#endif//#if !defined(SSE_NO_CARTRIDGE)

  WIDTHHEIGHT wh=GetTextSize(Font,T("Memory and monitor changes don't take effect until the next cold reset of the ST"));
  if (wh.Width>=page_w) wh.Height=(wh.Height+1)*2;
  CreateWindow("Static",T("Memory and monitor changes don't take effect until the next cold reset of the ST"),
        WS_CHILD,page_l,y,page_w,wh.Height,Handle,HMENU(8600),HInstance,NULL);

  y+=wh.Height+5;

  CreateWindow("Button",T("Perform cold reset now"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l,y,page_w,23,Handle,(HMENU)8601,HInstance,NULL);
//  y+=30;

  MachineUpdateIfVisible();
  if (Focus==NULL) Focus=GetDlgItem(Handle,404);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
void TOptionBox::MachineUpdateIfVisible()
{
  TOSRefreshBox("");

  if (Handle==NULL) return;

#if defined(SSE_VID_ST_MONITOR_393)
  if(GetDlgItem(Handle,1042)!=NULL) // display options are visible
  {
    DestroyCurrentPage();
    CreatePage(Page);
  }
#endif

  if (GetDlgItem(Handle,8100)==NULL) return;

#if defined(SSE_GUI_OPTIONS_STF)
  HWND Win=GetDlgItem(Handle,211); //ST Model
  if(Win!=NULL) 
    SendMessage(Win,CB_SETCURSEL,min((int)ST_TYPE,N_ST_MODELS-1),0);
#endif
#if defined(SSE_GUI_OPTIONS_WU)
ADVANCED_BEGIN
  Win=GetDlgItem(Handle,212); //WU
  if(Win!=NULL) 
    SendMessage(Win,CB_SETCURSEL,OPTION_WS,0);
ADVANCED_END
#endif


#if defined(SSE_MMU_393)
  // should work with different builds (w/wo 256KB, 2.5MB...)
  HWND cb_mem=GetDlgItem(Handle,8100);
  BYTE MemConf[2]={MEMCONF_512,MEMCONF_512};
  if(NewMemConf0==-1) // no new memory config selected
    GetCurrentMemConf(MemConf);
  else
  {
    MemConf[0]=NewMemConf0;
    MemConf[1]=NewMemConf1;
  }
  DWORD dwMemConf=MAKELONG(MemConf[0],MemConf[1]);
  // by Steem authors - could be used more?
  int curs_index=CBFindItemWithData(cb_mem,dwMemConf); 
  SendMessage(cb_mem,CB_SETCURSEL,curs_index,0);
#endif

  int monitor_sel=NewMonitorSel;
  if (monitor_sel<0) monitor_sel=GetCurrentMonitorSel();
  SendMessage(GetDlgItem(Handle,8200),CB_SETCURSEL,monitor_sel,0);

  SetWindowText(GetDlgItem(Handle,8500),CartFile);
  EnableWindow(GetDlgItem(Handle,8502),CartFile.NotEmpty());
#if defined(SSE_CARTRIDGE_FREEZE)
  EnableWindow(GetDlgItem(Handle,8503),CartFile.NotEmpty());
#endif
#if defined(SSE_CARTRIDGE_TRANSPARENT)
  SendMessage(GetDlgItem(Handle,8504),WM_SETTEXT,0,SSEOption.CartidgeOff
    ?(LPARAM)"Switch on":(LPARAM)"Switch off");
  EnableWindow(GetDlgItem(Handle,8504),CartFile.NotEmpty());
#endif
}
//---------------------------------------------------------------------------
void TOptionBox::CreateTOSPage()
{
  int y=10,Wid;
  HWND Win;

#if !defined(SSE_GUI_TOS_NOSORTCHOICE)
ADVANCED_BEGIN
  Wid=GetTextSize(Font,T("Sort by")).Width;
  CreateWindow("Static",T("Sort by"),WS_CHILD,page_l,y+4,Wid,25,
                  Handle,HMENU(8310),HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP,page_l+Wid+5,y,page_w-(Wid+5),200,
                  Handle,HMENU(8311),HInstance,NULL);
  CBAddString(Win,T("Version (Ascending)"),MAKELONG((WORD)eslSortByData0,0));
  CBAddString(Win,T("Version (Descending)"),MAKELONG((WORD)eslSortByData0,1));
  CBAddString(Win,T("Language"),MAKELONG((WORD)eslSortByData1,0));
  CBAddString(Win,T("Date (Ascending)"),MAKELONG((WORD)eslSortByData2,0));
  CBAddString(Win,T("Date (Descending)"),MAKELONG((WORD)eslSortByData2,1));
  CBAddString(Win,T("Name (Ascending)"),MAKELONG((WORD)(signed short)eslSortByNameI,0));
  CBAddString(Win,T("Name (Descending)"),MAKELONG((WORD)(signed short)eslSortByNameI,1));
  if (CBSelectItemWithData(Win,MAKELONG(eslTOS_Sort,eslTOS_Descend))<0){
    SendMessage(Win,CB_SETCURSEL,0,0);
    eslTOS_Sort=eslSortByData0;
    eslTOS_Descend=0;
  }
  y+=30;
ADVANCED_END
#endif //#if !defined(SSE_GUI_TOS_NOSORTCHOICE)
#ifdef SSE_GUI_393
  WIDTHHEIGHT wh=GetTextSize(Font,T("TOS changes don't take effect until the next cold reset of the ST. \
Be advised that STF and STE need different TOS. e.g. STF: 1.02 STE: 1.62"));
#else
  WIDTHHEIGHT wh=GetTextSize(Font,T("TOS changes don't take effect until the next cold reset of the ST"));
#endif
  if (wh.Width>=page_w) wh.Height=(wh.Height+1)*2;

  int TOSBoxHeight=(OPTIONS_HEIGHT-20)-(10+30+30+wh.Height+5+23+10);

  Win=CreateWindowEx(512,"ListBox","",WS_CHILD | WS_VSCROLL |
                  WS_TABSTOP | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_NOTIFY | LBS_SORT,
                  page_l,y,page_w,TOSBoxHeight,Handle,(HMENU)8300,HInstance,NULL);
  SendMessage(Win,LB_SETITEMHEIGHT,0,max((int)GetTextSize(Font,"HyITljq").Height+4,RC_FLAG_HEIGHT+4));
  y+=TOSBoxHeight+10;

  CreateWindow("Button",T("Add"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l,y,page_w/2-5,23,Handle,(HMENU)8301,HInstance,NULL);

  CreateWindow("Button",T("Remove"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+page_w/2+5,y,page_w/2-5,23,Handle,(HMENU)8302,HInstance,NULL);
  y+=30;
#ifdef SSE_GUI_393
  CreateWindow("Static",T("TOS changes don't take effect until the next cold reset of the ST. \
Be advised that STF and STE need different TOS."),
        WS_CHILD,page_l,y,page_w,40,Handle,HMENU(8600),HInstance,NULL);
#else
  CreateWindow("Static",T("TOS changes don't take effect until the next cold reset of the ST"),
        WS_CHILD,page_l,y,page_w,40,Handle,HMENU(8600),HInstance,NULL);
#endif
  y+=wh.Height+5;

  CreateWindow("Button",T("Perform cold reset now"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l,y,page_w,23,Handle,(HMENU)8601,HInstance,NULL);

  MachineUpdateIfVisible();
  if (Focus==NULL) Focus=GetDlgItem(Handle,8300);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
BOOL CALLBACK TOptionBox::EnumDateFormatsProc(char *DateFormat)
{
  USDateFormat=(strchr(DateFormat,'m')<strchr(DateFormat,'d'));
  return 0;
}
//---------------------------------------------------------------------------
void TOptionBox::CreateGeneralPage()
{
  HWND Win;
  long Wid;

  int y=10;
#if !defined(SSE_NO_RUN_SPEED)
ADVANCED_BEGIN
  CreateWindow("Static","",WS_CHILD | SS_CENTER,page_l,y,page_w,20,Handle,(HMENU)1040,HInstance,NULL);
  y+=20;

  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,28,Handle,(HMENU)1041,HInstance,NULL);
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,190)); // Each tick worth 5
  SendMessage(Win,TBM_SETPOS,1,((100000/run_speed_ticks_per_second)-50)/5);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  for (int n=0;n<190;n+=10) SendMessage(Win,TBM_SETTIC,0,n);
  SendMessage(Win,TBM_SETPAGESIZE,0,5);

  SendMessage(Handle,WM_HSCROLL,0,LPARAM(Win));
  y+=35;
ADVANCED_END
#endif
  CreateWindow("Static",T("Slow motion speed")+": "+(slow_motion_speed/10)+"%",WS_CHILD | SS_CENTER,
                          page_l,y,page_w,20,Handle,(HMENU)1000,HInstance,NULL);
  y+=20;

  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,28,Handle,(HMENU)1001,HInstance,NULL);
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,79));
  SendMessage(Win,TBM_SETPOS,1,(slow_motion_speed-10)/10);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  for (int n=4;n<79;n+=5) SendMessage(Win,TBM_SETTIC,0,n);
  SendMessage(Win,TBM_SETPAGESIZE,0,10);
  y+=35;

  CreateWindow("Static","",WS_CHILD | SS_CENTER,page_l,y,page_w,20,Handle,(HMENU)1010,HInstance,NULL);
  y+=20;

  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS,
                    page_l,y,page_w,28,Handle,(HMENU)1011,HInstance,NULL);
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,18));
  SendMessage(Win,TBM_SETPOS,1,(1000/max(fast_forward_max_speed,50))-2);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  SendMessage(Win,TBM_SETTICFREQ,1,0);
  SendMessage(Win,TBM_SETPAGESIZE,0,3);

  SendMessage(Handle,WM_HSCROLL,0,LPARAM(Win));
  y+=35;
#if !defined(SSE_GUI_SHOW_TIPS)
  Wid=GetCheckBoxSize(Font,T("Show pop-up hints")).Width;
  Win=CreateWindow("Button",T("Show pop-up hints"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,25,Handle,(HMENU)400,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,ShowTips,0);
  y+=30;
#endif
#if !defined(SSE_NO_HIGH_PRIORITY)
ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Make Steem high priority")).Width;
  Win=CreateWindow("Button",T("Make Steem high priority"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,25,Handle,(HMENU)1030,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,HighPriority,0);
  ToolAddWindow(ToolTip,Win,T("When this option is ticked Steem will get first use of the CPU ahead of other applications, this means Steem will still run smoothly even if you start doing something else at the same time, but everything else will run slower."));
  y+=30;
ADVANCED_END
#endif
  Wid=GetCheckBoxSize(Font,T("Pause emulation when inactive")).Width;
  Win=CreateWindow("Button",T("Pause emulation when inactive"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,25,Handle,(HMENU)800,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,PauseWhenInactive,0);
  y+=30;

#if defined(SSE_SOUND_MUTE_WHEN_INACTIVE)
  Wid=GetCheckBoxSize(Font,T("Mute sound when inactive")).Width;
  Win=CreateWindow("Button",T("Mute sound when inactive"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,25,Handle,(HMENU)801,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,MuteWhenInactive,0);
  y+=30;
#endif

  Wid=GetCheckBoxSize(Font,T("Disable system keys when running")).Width;
  Win=CreateWindow("Button",T("Disable system keys when running"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,25,Handle,(HMENU)700,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,(AllowTaskSwitch==0),0);
  ToolAddWindow(ToolTip,Win,T("When this option is ticked Steem will disable the Alt-Tab, Alt-Esc and Ctrl-Esc key combinations when it is running, this allows the ST to receive those keys. This option doesn't work in fullscreen mode."));
  y+=30;

#if !defined(SSE_FDC_NOFF)
ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Automatic fast forward on disk access")).Width;
  Win=CreateWindow("Button",T("Automatic fast forward on disk access"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,25,Handle,(HMENU)900,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,floppy_access_ff,0);
  y+=30;
ADVANCED_END
#endif

  Wid=GetCheckBoxSize(Font,T("Start emulation on mouse click")).Width;
  Win=CreateWindow("Button",T("Start emulation on mouse click"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,25,Handle,(HMENU)901,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,StartEmuOnClick,0);
  ToolAddWindow(ToolTip,Win,T("When this option is ticked clicking a mouse button on Steem's main window will start emulation."));
  SetPageControlsFont();
  if (Focus==NULL) Focus=GetDlgItem(Handle,1041);
  ShowPageControls();
}
//---------------------------------------------------------------------------
void TOptionBox::CreatePortsPage()
{
  HWND Win;
  int y=10,Wid;
  
#if defined(SSE_DONGLE_PORT)
  int GroupHeight=(OPTIONS_HEIGHT-10)/3-10-15;
#else
  int GroupHeight=(OPTIONS_HEIGHT-10)/3-10;
#endif
  int GroupMiddle=20+30+(GroupHeight-20-30)/2;
#if defined(SSE_DONGLE_PORT)
  for (int p=0;p<(OPTION_ADVANCED?4:3);p++){
    if(p==3)
      GroupHeight-=45;
#else
  for (int p=0;p<3;p++){
#endif
    HWND CtrlParent;
    EasyStr PortName;
    int base=9000+p*100;
    switch (p){
      case 0:PortName=T("MIDI Ports");break;
      case 1:PortName=T("Parallel Port");break;
      case 2:PortName=T("Serial Port");break;
#if defined(SSE_DONGLE_PORT)
      case 3:PortName=T("Special Adapters");break;
#endif
    }
    CtrlParent=CreateWindow("Button",PortName,WS_CHILD | BS_GROUPBOX,
                                page_l,y,page_w,GroupHeight,Handle,HMENU(base),HInstance,NULL);
#if defined(SSE_X64_LPTR)
    SetWindowLongPtr(CtrlParent, GWLP_USERDATA,(LONG_PTR)this);
    Old_GroupBox_WndProc = (WNDPROC)SetWindowLongPtr(CtrlParent, GWLP_WNDPROC, (LONG_PTR)GroupBox_WndProc);
#else
    SetWindowLong(CtrlParent,GWL_USERDATA,(long)this);
    Old_GroupBox_WndProc = (WNDPROC)SetWindowLong(CtrlParent, GWL_WNDPROC, (long)GroupBox_WndProc);
#endif

    y+=GroupHeight;
#if !defined(SSE_DONGLE_PORT)
    y+=10;
#endif
    Wid=get_text_width(T("Connect to"));
    CreateWindow("Static",T("Connect to"),WS_CHILD | WS_VISIBLE,
                          10,24,Wid,23,CtrlParent,HMENU(base+1),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                            15+Wid,20,page_w-10-(15+Wid),200,CtrlParent,HMENU(base+2),HInstance,NULL);
    CBAddString(Win,T("None"),PORTTYPE_NONE);

#if defined(SSE_DONGLE_PORT)
    if(p==3)
    {
ADVANCED_BEGIN
#if defined(SSE_DONGLE_LEADERBOARD)
      CBAddString(Win,T("10th Frame dongle"),TDongle::TENTHFRAME);
#endif
#if defined(SSE_DONGLE_BAT2)
      CBAddString(Win,T("B.A.T II dongle"),TDongle::BAT2);
#endif
#if defined(SSE_DONGLE_CRICKET)
      CBAddString(Win,T("Cricket Captain dongle"),TDongle::CRICKET);
#endif
#if defined(SSE_DONGLE_LEADERBOARD)
      CBAddString(Win,T("Leader Board dongle"),TDongle::LEADERBOARD);
#endif
#if defined(SSE_DONGLE_JEANNEDARC)
      CBAddString(Win,T("Jeanne d'Arc dongle"),TDongle::JEANNEDARC);
#endif
#if defined(SSE_DONGLE_CRICKET)
      CBAddString(Win,T("Rugby Coach dongle"),TDongle::RUGBY);
#endif
#if defined(SSE_DONGLE_CRICKET)
      CBAddString(Win,T("Multi Player Soccer Manager dongle"),TDongle::SOCCER);
#endif
#if defined(SSE_DONGLE_MUSIC_MASTER)
      CBAddString(Win,T("Music Master dongle"),TDongle::MUSIC_MASTER);
#endif
#if defined(SSE_DONGLE_PROSOUND)
      CBAddString(Win,T("Centronics Soundcard (WOD/LXS)"),TDongle::PROSOUND);
      //CBAddString(Win,T("Pro Sound Designer"),TDongle::PROSOUND);
#endif
#if defined(SSE_DONGLE_MULTIFACE)
      CBAddString(Win,T("Multiface Cartridge switch"),TDongle::MULTIFACE);
#endif
#if defined(SSE_DONGLE_URC)
      CBAddString(Win,T("Ultimate Ripper Cartridge switch"),TDongle::URC);
#endif

ADVANCED_END
    }
    else
    {
#endif
    CBAddString(Win,T("MIDI Device"),PORTTYPE_MIDI);
    if (AllowLPT) CBAddString(Win,T("Parallel Port (LPT)"),PORTTYPE_PARALLEL);
    if (AllowCOM) CBAddString(Win,T("COM Port"),PORTTYPE_COM);
    CBAddString(Win,T("File"),PORTTYPE_FILE);
    CBAddString(Win,T("Loopback (Output->Input)"),PORTTYPE_LOOP);
#if defined(SSE_DONGLE_PORT)
    }
#endif

    if (CBSelectItemWithData(Win,STPort[p].Type)<0){
      SendMessage(Win,CB_SETCURSEL,0,0);
    }

    // MIDI
    Wid=get_text_width(T("Output device"));
#if defined(SSE_DONGLE_PORT)
    CreateWindow("Static",T("Output device"),WS_CHILD,
                  10,54-5,Wid,23,CtrlParent,HMENU(base+10),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                      15+Wid,50-5,page_w-10-(15+Wid),200,CtrlParent,HMENU(base+11),HInstance,NULL);
#else
    CreateWindow("Static",T("Output device"),WS_CHILD,
                  10,54,Wid,23,CtrlParent,HMENU(base+10),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                      15+Wid,50,page_w-10-(15+Wid),200,CtrlParent,HMENU(base+11),HInstance,NULL);
#endif
#if defined(SSE_X64_390)
    SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("None"));
#else
    SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("None"));
#endif
    int c=midiOutGetNumDevs();
    MIDIOUTCAPS moc;
    for (int n=-1;n<c;n++){
      midiOutGetDevCaps(n,&moc,sizeof(moc));
#if defined(SSE_X64_390)
      SendMessage(Win,CB_ADDSTRING,0,(LPARAM)moc.szPname);
#else
      SendMessage(Win,CB_ADDSTRING,0,(long)moc.szPname);
#endif
    }
    SendMessage(Win,CB_SETCURSEL,STPort[p].MIDIOutDevice+2,0);

    Wid=get_text_width(T("Input device"));
#if defined(SSE_DONGLE_PORT)
    CreateWindow("Static",T("Input device"),WS_CHILD,
                            10,80+4-10,Wid,23,CtrlParent,HMENU(base+12),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                            15+Wid,80-10,page_w-10-(15+Wid),200,CtrlParent,HMENU(base+13),HInstance,NULL);
#else
    CreateWindow("Static",T("Input device"),WS_CHILD,
                            10,80+4,Wid,23,CtrlParent,HMENU(base+12),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                            15+Wid,80,page_w-10-(15+Wid),200,CtrlParent,HMENU(base+13),HInstance,NULL);
#endif
#if defined(SSE_X64_390)
    SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("None"));
#else
    SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("None"));
#endif
    c=midiInGetNumDevs();
    MIDIINCAPS mic;
    for (int n=0;n<c;n++){
      midiInGetDevCaps(n,&mic,sizeof(mic));
#if defined(SSE_X64_390)
      SendMessage(Win,CB_ADDSTRING,0,(LPARAM)mic.szPname);
#else
      SendMessage(Win,CB_ADDSTRING,0,(long)mic.szPname);
#endif
    }
    SendMessage(Win,CB_SETCURSEL,STPort[p].MIDIInDevice+1,0);

    //Parallel
    Wid=get_text_width(T("Select port"));
#if defined(SSE_DONGLE_PORT)
    CreateWindow("Static",T("Select port"),WS_CHILD,
                            page_w/2-(Wid+105)/2,GroupMiddle-15+4-5,Wid,23,CtrlParent,HMENU(base+20),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                            page_w/2-(Wid+105)/2+Wid+5,GroupMiddle-15-5,100,200,CtrlParent,HMENU(base+21),HInstance,NULL);
#else
    CreateWindow("Static",T("Select port"),WS_CHILD,
                            page_w/2-(Wid+105)/2,GroupMiddle-15+4,Wid,23,CtrlParent,HMENU(base+20),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                            page_w/2-(Wid+105)/2+Wid+5,GroupMiddle-15,100,200,CtrlParent,HMENU(base+21),HInstance,NULL);
#endif
    for (int n=1;n<10;n++) SendMessage(Win,CB_ADDSTRING,0,long((EasyStr("LPT")+n).Text));
    SendMessage(Win,CB_SETCURSEL,STPort[p].LPTNum,0);

    //COM
    Wid=get_text_width(T("Select port"));
#if defined(SSE_DONGLE_PORT)
    CreateWindow("Static",T("Select port"),WS_CHILD,
                            page_w/2-(Wid+105)/2,GroupMiddle-15+4-5,Wid,23,CtrlParent,HMENU(base+30),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                            page_w/2-(Wid+105)/2+Wid+5,GroupMiddle-15-5,100,200,CtrlParent,HMENU(base+31),HInstance,NULL);
#else
    CreateWindow("Static",T("Select port"),WS_CHILD,
                            page_w/2-(Wid+105)/2,GroupMiddle-15+4,Wid,23,CtrlParent,HMENU(base+30),HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                            page_w/2-(Wid+105)/2+Wid+5,GroupMiddle-15,100,200,CtrlParent,HMENU(base+31),HInstance,NULL);
#endif
    for (int n=1;n<10;n++) SendMessage(Win,CB_ADDSTRING,0,long((EasyStr("COM")+n).Text));
    SendMessage(Win,CB_SETCURSEL,STPort[p].COMNum,0);

    //File
#if defined(SSE_DONGLE_PORT)
    CreateWindowEx(512,"Steem Path Display",STPort[p].File,WS_CHILD,
                    10,GroupMiddle-30-2,page_w-20,22,CtrlParent,HMENU(base+40),HInstance,NULL);

    CreateWindow("Button",T("Change File"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  10,GroupMiddle-5-2,page_w/2-15,23,CtrlParent,HMENU(base+41),HInstance,NULL);

    CreateWindow("Button",T("Reset Current File"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_w/2+5,GroupMiddle-5-2,page_w/2-15,23,CtrlParent,HMENU(base+42),HInstance,NULL);
#else
    CreateWindowEx(512,"Steem Path Display",STPort[p].File,WS_CHILD,
                    10,GroupMiddle-30,page_w-20,22,CtrlParent,HMENU(base+40),HInstance,NULL);

    CreateWindow("Button",T("Change File"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  10,GroupMiddle,page_w/2-15,23,CtrlParent,HMENU(base+41),HInstance,NULL);

    CreateWindow("Button",T("Reset Current File"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_w/2+5,GroupMiddle,page_w/2-15,23,CtrlParent,HMENU(base+42),HInstance,NULL);
#endif
    // Disabled (parallel only)
    if (p==1){
#if defined(SSE_DONGLE_PORT)
      CreateWindow("Steem Path Display",T("Disabled due to parallel joystick"),
                    WS_CHILD | PDS_VCENTRESTATIC,
                    10,20,page_w-20,GroupHeight-30-5,CtrlParent,HMENU(99),HInstance,NULL);
#else
      CreateWindow("Steem Path Display",T("Disabled due to parallel joystick"),
                    WS_CHILD | PDS_VCENTRESTATIC,
                    10,20,page_w-20,GroupHeight-30,CtrlParent,HMENU(99),HInstance,NULL);
#endif
    }
    SetWindowAndChildrensFont(CtrlParent,Font);
  }

  SetPageControlsFont();
  if (Focus==NULL) Focus=GetDlgItem(GetDlgItem(Handle,9000),9002);
  ShowPageControls();
  for (int p=0;p<3;p++) PortsMakeTypeVisible(p);
}
//---------------------------------------------------------------------------
void TOptionBox::PortsMakeTypeVisible(int p)
{
  int base=9000+p*100;
  HWND CtrlParent=GetDlgItem(Handle,base);
  if (CtrlParent==NULL) return;

  bool Disabled=(p==1 && (Joy[N_JOY_PARALLEL_0].ToggleKey || Joy[N_JOY_PARALLEL_1].ToggleKey));

  for (int n=base+10;n<base+100;n++){
    if (GetDlgItem(CtrlParent,n)) ShowWindow(GetDlgItem(CtrlParent,n),SW_HIDE);
  }
  if (Disabled==0){
    for (int n=base+STPort[p].Type*10;n<base+STPort[p].Type*10+10;n++){
      if (GetDlgItem(CtrlParent,n)) ShowWindow(GetDlgItem(CtrlParent,n),SW_SHOW);
    }
  }
  if (p==1){
    if (Disabled){
      ShowWindow(GetDlgItem(CtrlParent,base+1),SW_HIDE);
      ShowWindow(GetDlgItem(CtrlParent,base+2),SW_HIDE);
      ShowWindow(GetDlgItem(CtrlParent,99),SW_SHOW);
    }else{
      ShowWindow(GetDlgItem(CtrlParent,99),SW_HIDE);
      ShowWindow(GetDlgItem(CtrlParent,base+1),SW_SHOW);
      ShowWindow(GetDlgItem(CtrlParent,base+2),SW_SHOW);
    }
  }

  // Redraw the groupbox
  RECT rc;
  GetWindowRect(CtrlParent,&rc);
#if defined(SSE_DONGLE_PORT)
  rc.left+=8;rc.right-=8;rc.top+=20+25-5;rc.bottom-=5;
#else
  rc.left+=8;rc.right-=8;rc.top+=20+25;rc.bottom-=5;
#endif
  POINT pt={0,0};
  ClientToScreen(Handle,&pt);
  OffsetRect(&rc,-pt.x,-pt.y);
  InvalidateRect(Handle,&rc,true);
}
//---------------------------------------------------------------------------
void TOptionBox::UpdateParallel()
{
  if (Handle) PortsMakeTypeVisible(1);
}
//---------------------------------------------------------------------------
#if defined(SSE_X64_LPTR)
#define GET_THIS This=(TOptionBox*)GetWindowLongPtr(Win,GWLP_USERDATA);
#else
#define GET_THIS This=(TOptionBox*)GetWindowLong(Win,GWL_USERDATA);
#endif
LRESULT __stdcall TOptionBox::GroupBox_WndProc(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar)
{
  TOptionBox *This;

  GET_THIS;

  switch (Mess){
    case WM_COMMAND:
    case WM_HSCROLL:
      return SendMessage(This->Handle,Mess,wPar,lPar);
  }
  return CallWindowProc(This->Old_GroupBox_WndProc,Win,Mess,wPar,lPar);
}
#undef GET_THIS
//---------------------------------------------------------------------------
void TOptionBox::CreateBrightnessBitmap()
{
  if (Handle==NULL) return;
  if (GetDlgItem(Handle,2010)==NULL) return;

  if (hBrightBmp) DeleteObject(hBrightBmp);
  HDC ScrDC=GetDC(NULL);
  hBrightBmp=CreateCompatibleBitmap(ScrDC,136+136,160);
  ReleaseDC(NULL,ScrDC);

  make_palette_table(brightness,contrast);
  DrawBrightnessBitmap(hBrightBmp);
  SendMessage(GetDlgItem(Handle,2010),STM_SETIMAGE,IMAGE_BITMAP,LPARAM(hBrightBmp));
}
//---------------------------------------------------------------------------
void TOptionBox::DrawBrightnessBitmap(HBITMAP hBmp)
{
  if (hBmp==NULL) return;

  BITMAP bi;
  GetObject(hBmp,sizeof(BITMAP),&bi);
  int w=bi.bmWidth,h=bi.bmHeight,bpp=bi.bmBitsPixel;

  int text_h=h/8;
  int band_w=w/16;
  int col_h=(h-text_h)/4;
  int BytesPP=(bpp+7)/8;
  BYTE *PicMem=new BYTE[w*h*BytesPP + 16];
  ZeroMemory(PicMem,w*h*BytesPP);
  BYTE *pMem=PicMem;

  int pc_pal_start_idx=10+118+(118-65); // End of the second half of the palette
  PALETTEENTRY *pbuf=(PALETTEENTRY*)&logpal[pc_pal_start_idx];
  for (int y=0;y<h-text_h;y++){
    for (int i=0;i<w;i++){
      int r=((i/band_w) >> 1)+(((i/band_w) & 1) << 3),g=r,b=r;
      int pal_offset=0;
      if (y>col_h*3){
        g=0,b=0;
        pal_offset=48;
      }else if (y>col_h*2){
        r=0,b=0;
        pal_offset=32;
      }else if (y>col_h){
        r=0,g=0;
        pal_offset=16;
      }
      long Col=palette_table[r | (g << 4) | (b << 8)];
      switch (BytesPP){
        case 1:
        {
          int ncol=pal_offset+(i/band_w);
          pbuf[ncol].peFlags=PC_RESERVED;
          pbuf[ncol].peRed=  BYTE((Col & 0xff0000) >> 16);
          pbuf[ncol].peGreen=BYTE((Col & 0x00ff00) >> 8);
          pbuf[ncol].peBlue= BYTE((Col & 0x0000ff));
          *pMem=BYTE(1+pc_pal_start_idx+ncol);
          break;
        }
        case 2:
          *LPWORD(pMem)=WORD(Col);
          break;
        case 3:case 4:
          *LPDWORD(pMem)=DWORD(Col);
          break;
      }
      pMem+=BytesPP;
    }
  }
  SetBitmapBits(hBmp,w*h*BytesPP,PicMem);
  delete[] PicMem;
  if (BytesPP==1) AnimatePalette(winpal,pc_pal_start_idx,64,pbuf);

  int gap_w=band_w/4,gap_h=text_h/8;
  HFONT f=MakeFont("Arial",-(text_h-gap_h),band_w/2 - gap_w);
  HDC ScrDC=GetDC(NULL);
  HDC BmpDC=CreateCompatibleDC(ScrDC);
  ReleaseDC(NULL,ScrDC);

  SelectObject(BmpDC,hBmp);
  SelectObject(BmpDC,f);
  SetTextColor(BmpDC,RGB(224,224,224));
  SetBkMode(BmpDC,TRANSPARENT);
  for (int i=0;i<16;i++){
    TextOut(BmpDC,i*band_w + (band_w-GetTextSize(f,EasyStr(i+1)).Width)/2,h-text_h-1+gap_h/2,EasyStr(i+1),EasyStr(i+1).Length());
  }
  DeleteDC(BmpDC);
  DeleteObject(f);
}
//---------------------------------------------------------------------------
LRESULT __stdcall TOptionBox::Fullscreen_WndProc(HWND Win,UINT Mess,WPARAM wPar,LPARAM lPar)
{
  if (Mess==WM_PAINT || Mess==WM_NCPAINT){
    HDC WinDC=GetWindowDC(Win);
    HDC BmpDC=CreateCompatibleDC(WinDC);
    SelectObject(BmpDC,GetProp(Win,"Bitmap"));
    BitBlt(WinDC,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),BmpDC,0,0,SRCCOPY);
    DeleteDC(BmpDC);
    ReleaseDC(Win,WinDC);

    ValidateRect(Win,NULL);
    return 0;
  }
  return DefWindowProc(Win,Mess,wPar,lPar);
}
//---------------------------------------------------------------------------
#if !defined(SSE_VID_D3D_ONLY_)
void TOptionBox::FullscreenBrightnessBitmap()
{
  int w=GetSystemMetrics(SM_CXSCREEN),h=GetSystemMetrics(SM_CYSCREEN);

  WNDCLASS wc;
  wc.style=CS_DBLCLKS;
  wc.lpfnWndProc=Fullscreen_WndProc;
  wc.cbClsExtra=0;
  wc.cbWndExtra=0;
  wc.hInstance=HInstance;
  wc.hIcon=NULL;
  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground=NULL;
  wc.lpszMenuName=NULL;
  wc.lpszClassName="Steem Temp Fullscreen Window";
  RegisterClass(&wc);

  HWND Win=CreateWindow("Steem Temp Fullscreen Window","",0,
                            0,0,w,h,Handle,NULL,HInstance,NULL);
  SetWindowLong(Win,GWL_STYLE,0);
  HDC ScrDC=GetDC(NULL);
  HBITMAP hBmp=CreateCompatibleBitmap(ScrDC,w,h);
  ReleaseDC(NULL,ScrDC);

  DrawBrightnessBitmap(hBmp);
  SetProp(Win,"Bitmap",hBmp);

  ShowWindow(Win,SW_SHOW);
  SetWindowPos(Win,HWND_TOPMOST,0,0,w,h,0);
  UpdateWindow(Win);

  bool DoneMouseUp=0;
  MSG mess;
  for (;;){
    PeekMessage(&mess,Win,0,0,PM_REMOVE);
    DispatchMessage(&mess);

    short MouseBut=(GetKeyState(VK_LBUTTON) | GetKeyState(VK_RBUTTON) | GetKeyState(VK_MBUTTON));
    if (MouseBut>=0) DoneMouseUp=true;
    if (MouseBut<0 && DoneMouseUp) break;
  }

  RemoveProp(Win,"Bitmap");
  DestroyWindow(Win);
  DeleteObject(hBmp);

  UnregisterClass("Steem Temp Fullscreen Window",HInstance);
}
#endif
//---------------------------------------------------------------------------
void TOptionBox::CreateBrightnessPage()
{
  int mid=page_l + page_w/2;
  RECT rc={mid-136,12,mid+136,12+160};
  AdjustWindowRectEx(&rc,WS_CHILD | SS_BITMAP,0,512);
  HWND Win=CreateWindowEx(512,"Static","",WS_CHILD | SS_BITMAP | SS_NOTIFY,
                            rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
                            Handle,(HMENU)2010,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Click to view fullscreen"));

  CreateBrightnessBitmap();

  GetWindowRect(Win,&rc);
  POINT pt={0,0};
  ClientToScreen(Handle,&pt);
  int y=(rc.bottom-pt.y)+5;
#if defined(SSE_VID_GAMMA)
/*  We make sliders smaller so that we can fit 3 more on the same page.
*/
  char tmp[30];
  CreateWindow("Static",T("There should be 16 vertical strips (one black)"),WS_CHILD | SS_CENTER,
                          page_l,y,page_w,40-20,Handle,(HMENU)2011,HInstance,NULL);
  y+=40-20;
#else
  CreateWindow("Static",T("There should be 16 vertical strips (one black)"),WS_CHILD | SS_CENTER,
                          page_l,y,page_w,40,Handle,(HMENU)2011,HInstance,NULL);
  y+=40;
#endif

#if defined(SSE_VID_GAMMA)
  sprintf(tmp,"Brightness:%d",brightness);
  CreateWindow("Static",tmp,WS_CHILD | SS_CENTER,
                          page_l,y,page_w,25-10,Handle,(HMENU)2000,HInstance,NULL);
  y+=25-10;
  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,28-10,Handle,(HMENU)2001,HInstance,NULL);
#else
  CreateWindow("Static",T("Brightness")+": "+brightness,WS_CHILD | SS_CENTER,
                          page_l,y,page_w,25,Handle,(HMENU)2000,HInstance,NULL);
  y+=25;
  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,28,Handle,(HMENU)2001,HInstance,NULL);
#endif
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,256));
  SendMessage(Win,TBM_SETPOS,1,brightness+128);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  SendMessage(Win,TBM_SETPAGESIZE,0,10);
  SendMessage(Win,TBM_SETTIC,0,128);

#if defined(SSE_VID_GAMMA)
  y+=40-20;
  sprintf(tmp,"Contrast:%d",contrast);
  CreateWindow("Static",tmp,WS_CHILD | SS_CENTER,
                          page_l,y,page_w,25-10,Handle,(HMENU)2002,HInstance,NULL);
  y+=25-10;
  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,28-10,Handle,(HMENU)2003,HInstance,NULL);
#else
  y+=40;

  CreateWindow("Static",T("Contrast")+": "+contrast,WS_CHILD | SS_CENTER,
                          page_l,y,page_w,25,Handle,(HMENU)2002,HInstance,NULL);
  y+=25;
  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,28,Handle,(HMENU)2003,HInstance,NULL);
#endif
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,256));
  SendMessage(Win,TBM_SETPOS,1,contrast+128);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  SendMessage(Win,TBM_SETPAGESIZE,0,10);
  SendMessage(Win,TBM_SETTIC,0,128);

#if defined(SSE_VID_GAMMA)
  for(int i=0;i<3;i++)
  {
    y+=40-20;
    sprintf(tmp,"Gamma %s:%d",rgb_txt[i],gamma[i]); // red, green, blue
    CreateWindow("Static",tmp,WS_CHILD | SS_CENTER,
                          page_l,y,page_w,25-10,Handle,(HMENU)(2004+i*2),HInstance,NULL);

    y+=25-10;
    Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,28-10,Handle,(HMENU)(2005+i*2),HInstance,NULL);

    SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(1,256));
    SendMessage(Win,TBM_SETPOS,1,gamma[i]+128);
    SendMessage(Win,TBM_SETLINESIZE,0,1);
    SendMessage(Win,TBM_SETPAGESIZE,0,10);
    SendMessage(Win,TBM_SETTIC,0,128);
  }
#endif

#if defined(SSE_GUI_COLOUR_CTRL_RESET)
  CreateWindow("Button",T("Reset"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                          page_l,y+20,50,20,Handle,(HMENU)1025,HInstance,NULL);
#endif

  if (Focus==NULL) Focus=GetDlgItem(Handle,2001);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
void TOptionBox::CreateDisplayPage()
{
  HWND Win;
  long Wid;
  int y=10;

#if defined(SSE_BUILD) // Border + Frameskip
  int mask;
  long Offset=0;
  const int LineHeight=30;

  // One option only for border on/off and size.
  int x=page_l;
  Wid=get_text_width(T("Borders"));
  CreateWindow("Static",T("Borders"),WS_CHILD,
                          x,y+4,Wid,21,Handle,(HMENU)209,HInstance,NULL);

  BorderOption=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
                          x+5+Wid,y,Wid+50,200,Handle,(HMENU)207,HInstance,NULL);
  SendMessage(BorderOption,CB_ADDSTRING,0,(LPARAM)CStrT("Off"));
  SendMessage(BorderOption,CB_ADDSTRING,0,(LPARAM)CStrT("Normal"));
  if(screen_res<2) //doesn't work on startup...
  {
    SendMessage(BorderOption,CB_ADDSTRING,0,(LPARAM)CStrT("Large"));
    SendMessage(BorderOption,CB_ADDSTRING,0,(LPARAM)CStrT("Max"));
  }

  EnableBorderOptions(true);
#if !defined(SSE_VID_ENFORCE_AUTOFRAMESKIP)

  x=page_l+150;
  int w=get_text_width(T("Frameskip"));
ADVANCED_BEGIN
  CreateWindow("Static",T("Frameskip"),WS_CHILD,
                    x,y+4,w,20,Handle,(HMENU)200,HInstance,NULL);
  x+=w+5;
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                    x,y,110,200,Handle,(HMENU)201,HInstance,NULL);
  // must make shorter
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("None"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Draw 1/2"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Draw 1/3"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Draw 1/4"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Auto"));
  SendMessage(Win,CB_SETCURSEL,min(frameskip-1,4),0);
ADVANCED_END
#endif
  y+=LineHeight;
  x=page_l;
  mask=WS_CHILD|WS_TABSTOP|BS_CHECKBOX;

#else // Steem 3.2

  int x=page_l;
  int w=get_text_width(T("Frameskip"));
  CreateWindow("Static",T("Frameskip"),WS_CHILD,
                    x,y+4,w,20,Handle,(HMENU)200,HInstance,NULL);
  x+=w+5;
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                    x,y,page_w-(w+5),200,Handle,(HMENU)201,HInstance,NULL);
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Draw Every Frame"));
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Draw Every Second Frame"));
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Draw Every Third Frame"));
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Draw Every Fourth Frame"));
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Auto Frameskip"));
  SendMessage(Win,CB_SETCURSEL,min(frameskip-1,4),0);
  y+=30;

  Wid=get_text_width(T("Borders"));
  CreateWindow("Static",T("Borders"),WS_CHILD,
                          page_l,y+4,Wid,21,Handle,(HMENU)209,HInstance,NULL);
  BorderOption=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+5+Wid,y,page_w-(5+Wid),200,Handle,(HMENU)207,HInstance,NULL);
  SendMessage(BorderOption,CB_ADDSTRING,0,(long)CStrT("Never Show Borders"));
  SendMessage(BorderOption,CB_ADDSTRING,1,(long)CStrT("Always Show Borders"));
  SendMessage(BorderOption,CB_ADDSTRING,2,(long)CStrT("Auto Borders"));
  if (Disp.BorderPossible()==0 && FullScreen==0) EnableBorderOptions(0);
  y+=30;

#endif//SSE

#if defined(SSE_VID_VSYNC_WINDOW) && defined(SSE_VID_GUI_392)
ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("VSync")).Width;
  Win=CreateWindow("Button",T("VSync"), mask,
               x,y,Wid,25,Handle,(HMENU)1033,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_WIN_VSYNC,0);
  ToolAddWindow(ToolTip,Win,T("For the window. This can change emulation speed"));
  x+=Wid+5;
ADVANCED_END
  y+=LineHeight;
#endif

#if defined(SSE_VID_DD_3BUFFER_WIN) // DirectDraw-only
  y-=LineHeight;
ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Triple Buffering")).Width;
  Win=CreateWindow("Button",T("Triple Buffering"),mask,
    x,y,Wid,25,Handle,(HMENU)1034,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_3BUFFER_WIN,0);
  ToolAddWindow(ToolTip,Win,
    T("For the window. Only useful on older systems, otherwise it interferes with Windows."));
  x+=Wid+5;
ADVANCED_END
  y+=LineHeight;
#endif

#if defined(SSE_VID_ST_MONITOR_393)
#if (defined(SSE_VID_VSYNC_WINDOW) && defined(SSE_VID_GUI_392)) || defined(SSE_VID_DD_3BUFFER_WIN)
  y-=LineHeight;
#endif
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
  if(screen_res==2)
    mask|=WS_DISABLED;
  Wid=GetCheckBoxSize(Font,T("ST Aspect Ratio")).Width;
  Win=CreateWindow("Button",T("ST Aspect Ratio"),mask,
                          x,y,Wid,25,Handle,(HMENU)1042,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_ST_ASPECT_RATIO,0);
  //ToolAddWindow(ToolTip,Win,T("Tries to emulate aspect ratio of your typical monitor."));
  ToolAddWindow(ToolTip,Win,
    T("Reproduces the familiar vertical distortion on standard colour screens"));

  x+=Wid+5;

  //  The new option covers both former draw mode and interpolated
  Wid=GetCheckBoxSize(Font,T("Scanlines")).Width;
  Win=CreateWindow("Button",T("Scanlines"),mask,
    x,y,Wid,25,Handle,(HMENU)1032,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_SCANLINES,0);
 ToolAddWindow(ToolTip,Win,T("Reproduces scanlines of colour screens"));

#if defined(SSE_GUI_CRISP_IN_DISPLAY)
NOT_ADVANCED_BEGIN
  x+=Wid+5;
  Wid=GetCheckBoxSize(Font,T("Crisp")).Width;
  Win=CreateWindow("Button",T("Crisp"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    x,y,Wid,25,Handle,(HMENU)7324,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_D3D_CRISP,0);
  ToolAddWindow(ToolTip,Win,T("Toggles blurry/crisp rendering (also for\
 fullscreen), but it also depends on the video card, some render the same"));
ADVANCED_END
#endif
  y+=LineHeight;
#endif

  
#if defined(SSE_VID_BLOCK_WINDOW_SIZE) // absolute placement TODO?
  Offset=10;
  y+=10;
ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Lock window size")).Width;
  ASSERT(Wid>0);
  Win=CreateWindow("Button",T("Lock window size"),WS_CHILD  | WS_TABSTOP | BS_CHECKBOX,
                          page_l+Offset,y,Wid,23,Handle,(HMENU)7317,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_BLOCK_RESIZE,0);
ADVANCED_END
#endif

#if defined(SSE_VID_LOCK_ASPET_RATIO)
ADVANCED_BEGIN
  Offset+=Wid+10;
  Wid=GetCheckBoxSize(Font,T("Lock aspect ratio")).Width;
  ASSERT(Wid>0);
  mask=WS_CHILD  | WS_TABSTOP | BS_CHECKBOX;
  if(OPTION_BLOCK_RESIZE)
    mask|=WS_DISABLED;
  Win=CreateWindow("Button",T("Lock aspect ratio"),mask,
                          page_l+Offset,y,Wid,23,Handle,(HMENU)7318,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_LOCK_ASPECT_RATIO,0);
ADVANCED_END
  y+=10;
#endif

ADVANCED_BEGIN

#if defined(SSE_BUILD) && !defined(SSE_VID_BLOCK_WINDOW_SIZE) \
  && !defined(SSE_VID_LOCK_ASPET_RATIO) //LE
  y+=30;
  CreateWindow("Button",T("Window Size"),WS_CHILD | BS_GROUPBOX,
                  page_l,y-25,page_w,30+30+30+2+25,Handle,(HMENU)99,HInstance,NULL);
#elif defined(SSE_BUILD)
ADVANCED_BEGIN
  CreateWindow("Button",T("Window Size"),WS_CHILD | BS_GROUPBOX,
                  page_l,y-25,page_w,20+30+30+30+30+2+25,Handle,(HMENU)99,HInstance,NULL);
ADVANCED_ELSE
  CreateWindow("Button",T("Window Size"),WS_CHILD | BS_GROUPBOX,
                  page_l,y-25,page_w,20+30+30+10+2+25,Handle,(HMENU)99,HInstance,NULL);
ADVANCED_END
#else
  CreateWindow("Button",T("Window Size"),WS_CHILD | BS_GROUPBOX,
                  page_l,y,page_w,20+30+30+30+30+2,Handle,(HMENU)99,HInstance,NULL);
#endif

#if !defined(SSE_VID_BLOCK_WINDOW_SIZE) && !defined(SSE_VID_LOCK_ASPET_RATIO) //LE
#else
  y+=20;
#endif
#if !defined(SSE_VID_AUTO_RESIZE)
ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Automatic resize on resolution change")).Width;
  Win=CreateWindow("Button",T("Automatic resize on resolution change"),WS_CHILD  | WS_TABSTOP | BS_CHECKBOX,
                          page_l+10,y,Wid,23,Handle,(HMENU)300,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,ResChangeResize,0);
  y+=30;
ADVANCED_ELSE
  y-=20;
ADVANCED_END
#endif
  
#if defined(SSE_VID_ENFORCE_AUTOFRAMESKIP)
  int
#endif
  w=get_text_width(T("Low resolution"));
  CreateWindow("Static",T("Low resolution"),WS_CHILD ,
                          page_l+10,y+4,w,23,Handle,(HMENU)301,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+15+w,y,page_w-10-(15+w),200,Handle,(HMENU)302,HInstance,NULL);
#if defined(SSE_GUI_OPTION_DISPLAY_CHANGE_TEXT)
  CBAddString(Win,T("Normal Size (small)"),0);
#else
  CBAddString(Win,T("Normal Size"),0);
#endif
  CBAddString(Win,T("Double Size")+" - "+T("Stretch"),1);
  CBAddString(Win,T("Double Size")+" - "+T("No Stretch"),MAKELONG(1,DWM_NOSTRETCH));

#if !defined(SSE_VID_ST_MONITOR_393)
  CBAddString(Win,T("Double Size")+" - "+T("Grille"),MAKELONG(1,DWM_GRILLE));
#endif

  CBAddString(Win,T("Treble Size"),2);
  CBAddString(Win,T("Quadruple Size"),3);
  y+=30;

  w=get_text_width(T("Medium resolution"));
  CreateWindow("Static",T("Medium resolution"),WS_CHILD ,
                          page_l+10,y+4,w,23,Handle,(HMENU)303,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+15+w,y,page_w-10-(15+w),200,Handle,(HMENU)304,HInstance,NULL);
  CBAddString(Win,T("Normal Size"),0);
  CBAddString(Win,T("Double Height")+" - "+T("Stretch"),1);
  CBAddString(Win,T("Double Height")+" - "+T("No Stretch"),MAKELONG(1,DWM_NOSTRETCH));
#if !defined(SSE_VID_ST_MONITOR_393)
  CBAddString(Win,T("Double Height")+" - "+T("Grille"),MAKELONG(1,DWM_GRILLE));
#endif

  CBAddString(Win,T("Double Size"),2);
  CBAddString(Win,T("Quadruple Height"),3);
  y+=30;

  w=get_text_width(T("High resolution"));
  CreateWindow("Static",T("High resolution"),WS_CHILD ,
                          page_l+10,y+4,w,23,Handle,(HMENU)305,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD  | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+15+w,y,page_w-10-(15+w),200,Handle,(HMENU)306,HInstance,NULL);
  CBAddString(Win,T("Normal Size"),0);
  CBAddString(Win,T("Double Size"),1);
  y+=30;

  y+=10;

ADVANCED_END

#if !defined(SSE_GUI_NO2SCREENSHOT_SETTINGS) // useful only in DD build

  EasyStringList format_sl;
  Disp.ScreenShotGetFormats(&format_sl);
  bool FIAvailable=format_sl.NumStrings>2;//SS wrong ... since NEO was added?

  int h=20+30+30+30+25+3;
  CreateWindow("Button",T("Screenshots"),WS_CHILD | BS_GROUPBOX,
                  page_l,y,page_w,h,Handle,(HMENU)99,HInstance,NULL);
  y+=20;

  Wid=get_text_width(T("Folder"));
  CreateWindow("Static",T("Folder"),WS_CHILD,page_l+10,y+4,Wid,23,Handle,(HMENU)1020,HInstance,NULL);

  CreateWindowEx(512,"Steem Path Display",ScreenShotFol,WS_CHILD,
                  page_l+15+Wid,y,page_w-10-(15+Wid),25,Handle,(HMENU)1021,HInstance,NULL);
  y+=30;

  CreateWindow("Button",T("Choose"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                          page_l+10,y,(page_w-20)/2-5,23,Handle,(HMENU)1022,HInstance,NULL);

  CreateWindow("Button",T("Open"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                          page_l+10+(page_w-20)/2+5,y,(page_w-20)/2-5,23,Handle,(HMENU)1023,HInstance,NULL);
  y+=30;

  w=get_text_width(T("Format"));
  CreateWindow("Static",T("Format"),WS_CHILD,page_l+10,y+4,w,23,Handle,(HMENU)1050,HInstance,NULL);

  int l=page_l+10+w+5;
  if (FIAvailable){
    w=(page_w-10-(10+w+5))/2-5;
  }else{
    w=page_w-10-(10+w+5);
  }

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                          l,y,w,300,Handle,(HMENU)1051,HInstance,NULL);
  for (int i=0;i<format_sl.NumStrings;i++){
    CBAddString(Win,format_sl[i].String,format_sl[i].Data[0]);
  }

  int n,c=SendMessage(Win,CB_GETCOUNT,0,0);
  for (n=0;n<c;n++){
    if (SendMessage(Win,CB_GETITEMDATA,n,0)==Disp.ScreenShotFormat) break;
  }
  if (n>=c){
    Disp.ScreenShotFormat=FIF_BMP;
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
    Disp.ScreenShotFormatOpts=0;
#endif
    n=1;    
  }
  SendMessage(Win,CB_SETCURSEL,n,0);
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  if (FIAvailable){
    CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                            l+w+5,y,w,200,Handle,(HMENU)1052,HInstance,NULL);
    FillScreenShotFormatOptsCombo();
  }
#endif
  y+=30;

  Wid=GetCheckBoxSize(Font,T("Minimum size screenshots")).Width;
  Win=CreateWindow("Button",T("Minimum size screenshots"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l+10,y,Wid,23,Handle,(HMENU)1024,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,Disp.ScreenShotMinSize,0);
  ToolAddWindow(ToolTip,Win,T("This option, when checked, ensures all screenshots will be taken at the smallest size possible for the resolution.")+" "+
                            T("WARNING: Some video cards may cause the screenshots to look terrible in certain drawing modes."));
#endif //#if !defined(SSE_GUI_NO2SCREENSHOT_SETTINGS)

  UpdateWindowSizeAndBorder();

  if (Focus==NULL) Focus=GetDlgItem(Handle,201);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
void TOptionBox::FillScreenShotFormatOptsCombo()
{
  HWND Win=GetDlgItem(Handle,1052);
  if (Win==NULL) return;

  EasyStringList sl;
  sl.Sort=eslNoSort;
  Disp.ScreenShotGetFormatOpts(&sl);
  SendMessage(Win,CB_RESETCONTENT,0,0);
  if (sl.NumStrings){
    EnableWindow(Win,true);
    for (int i=0;i<sl.NumStrings;i++) CBAddString(Win,sl[i].String,sl[i].Data[0]);
  }else{
    EnableWindow(Win,0);
    CBAddString(Win,T("Normal"),0);
  }
  if (CBSelectItemWithData(Win,Disp.ScreenShotFormatOpts)<0){
    SendMessage(Win,CB_SETCURSEL,0,0);
  }
}
#endif
//---------------------------------------------------------------------------
void TOptionBox::UpdateWindowSizeAndBorder()
{
  if (BorderOption==NULL) return;
#if defined(SSE_VID_BORDERS_GUI_392)
  SendMessage(BorderOption,CB_SETCURSEL,min((int)border,BIGGEST_DISPLAY),0);
#elif defined(SSE_VID_DISABLE_AUTOBORDER)
  SendMessage(BorderOption,CB_SETCURSEL,min((int)border,1),0);
#else
  SendMessage(BorderOption,CB_SETCURSEL,min(border,2),0);
#endif
  for (int r=0;r<3;r++){
    DWORD dat=WinSizeForRes[r];
    if (r<2) dat=MAKELONG(dat,draw_win_mode[r]);
    CBSelectItemWithData(GetDlgItem(Handle,302+r*2),dat);
  }
}
//---------------------------------------------------------------------------
void TOptionBox::CreateOSDPage()
{
  HWND Win;
  long Wid;
  int y=10;

#if defined(SSE_OSD_DRIVE_LED)
  Wid=GetCheckBoxSize(Font,T("Disk access light")).Width;
  Win=CreateWindow("Button",T("Disk access light"),WS_CHILD  | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)12000,HInstance,NULL);
#else
  Wid=GetCheckBoxSize(Font,T("Floppy disk access light")).Width;
  Win=CreateWindow("Button",T("Floppy disk access light"),WS_CHILD  | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)12000,HInstance,NULL);
#endif

  SendMessage(Win,BM_SETCHECK,osd_show_disk_light,0);

#if defined(SSE_OSD_DRIVE_INFO_GUI)
  const int HorizontalSeparation=10;
  long Wid2=Wid+HorizontalSeparation;
  Wid=GetCheckBoxSize(Font,T("Disk drive track info")).Width;
  Win=CreateWindow("Button",T("Disk drive track info"),WS_CHILD  | WS_TABSTOP | BS_CHECKBOX,
                          page_l + Wid2,y,Wid,23,Handle,(HMENU)12001,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_DRIVE_INFO,0);
  ToolAddWindow(ToolTip,Win,
    T("See what the floppy drive is doing with this option"));
#endif

  y+=30;

  int *p_element[4]={&osd_show_plasma,&osd_show_speed,&osd_show_icons,&osd_show_cpu};
  Str osd_name[4];
  osd_name[0]=T("Logo");
  osd_name[1]=T("Speed bar");
  osd_name[2]=T("State icons");
  osd_name[3]=T("CPU speed indicator");
  for (int i=0;i<4;i++){
    Wid=GetTextSize(Font,osd_name[i]).Width;
    CreateWindow("Static",osd_name[i],WS_CHILD | WS_TABSTOP,
                          page_l,y+4,Wid,23,Handle,(HMENU)0,HInstance,NULL);

    Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                            page_l+Wid+5,y,page_w-(Wid+5),200,Handle,HMENU(12010+i),HInstance,NULL);
    CBAddString(Win,T("Off"),0);
    CBAddString(Win,Str("2 ")+T("Seconds"),2);
    CBAddString(Win,Str("3 ")+T("Seconds"),3);
    CBAddString(Win,Str("4 ")+T("Seconds"),4);
    CBAddString(Win,Str("5 ")+T("Seconds"),5);
    CBAddString(Win,Str("6 ")+T("Seconds"),6);
    CBAddString(Win,Str("8 ")+T("Seconds"),8);
    CBAddString(Win,Str("10 ")+T("Seconds"),10);
    CBAddString(Win,Str("12 ")+T("Seconds"),12);
    CBAddString(Win,Str("15 ")+T("Seconds"),15);
    CBAddString(Win,Str("20 ")+T("Seconds"),20);
    CBAddString(Win,Str("30 ")+T("Seconds"),30);
    CBAddString(Win,T("Always Shown"),OSD_SHOW_ALWAYS);
    if (CBSelectItemWithData(Win,*(p_element[i]))<0){
      SendMessage(Win,CB_SETCURSEL,0,0);
    }
    y+=30;
  }

/*
  Wid=GetCheckBoxSize(Font,T("Old positions")).Width;
  Win=CreateWindow("Button",T("Old positions"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)12050,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,osd_old_pos,0);
  y+=30;
*/

  Wid=GetCheckBoxSize(Font,T("Scrolling messages")).Width;
  Win=CreateWindow("Button",T("Scrolling messages"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)12020,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,osd_show_scrollers,0);
#if defined(SSE_OSD_SCROLLER_DISK_IMAGE)
#if !defined(SSE_OSD_DRIVE_INFO_GUI)  
  const int HorizontalSeparation=10;
  long 
#endif
  Wid2=Wid+HorizontalSeparation;
  Wid=GetCheckBoxSize(Font,T("Disk image names")).Width;
  Win=CreateWindow("Button",T("Disk image names"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l+Wid2,y,Wid,23,Handle,(HMENU)12002,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OSD_IMAGE_NAME,0);
#endif

#if defined(SSE_OSD_SHOW_TIME)
  y+=30;
  Wid=GetCheckBoxSize(Font,T("Time")).Width;
  Win=CreateWindow("Button",T("Time"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)1036,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_OSD_TIME,0);
#endif

  y+=30;
  Wid=GetCheckBoxSize(Font,T("Disable on screen display")).Width;
  Win=CreateWindow("Button",T("Disable on screen display"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)12030,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,osd_disable,0);

  if (Focus==NULL) Focus=GetDlgItem(Handle,201);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
void TOptionBox::CreateFullscreenPage()
{
  HWND Win;
  long w;
  int y=10;

#if defined(SSE_BUILD)
  const int LineHeight=30;
  int mask=WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX;
#endif

#if defined(SSE_BUGFIX_394) && defined(SSE_VID_D3D)
  int disable=(OPTION_FAKE_FULLSCREEN)?WS_DISABLED:0;
#else
  int disable=false;
#endif


#if defined(SSE_VID_D3D)

  long Offset=0;
  long Wid=0;

#if defined(SSE_VID_D3D_LIST_MODES)

ADVANCED_BEGIN

/*  We do some D3D here, listing all modes only when necessary to save memory.
    Or we could have another function in display, but then code bloat...
*/

#if defined(SSE_VID_D3D_2SCREENS)
  UINT Adapter=Disp.m_Adapter;
#else
  UINT Adapter=D3DADAPTER_DEFAULT;
#endif

#if defined(SSE_VID_BPP_CHOICE) && !defined(SSE_VID_BPP_NO_CHOICE)
  D3DFORMAT DisplayFormat;
  switch(SSEConfig.GetBitsPerPixel()) {
    case 32:
      DisplayFormat=D3DFMT_X8R8G8B8;
      break;
    case 16:
      DisplayFormat=D3DFMT_R5G6B5;//D3DFMT_X1R5G5B5;
      break;
    default:
      DisplayFormat=D3DFMT_P8;
      break;
  }
#else
  D3DFORMAT DisplayFormat=D3DFMT_X8R8G8B8; //32bit; D3DFMT_R5G6B5=16bit
#endif

#if defined(SSE_VID_CHECK_POINTERS)
  UINT nD3Dmodes=(Disp.pD3D)
    ? Disp.pD3D->GetAdapterModeCount(Adapter,DisplayFormat) : 0;
#else
  UINT nD3Dmodes=Disp.pD3D->GetAdapterModeCount(Adapter,DisplayFormat);
#endif

  w=get_text_width(T("Mode"));
  CreateWindow("Static",T("Mode"),WS_CHILD ,
                          page_l+Offset,y+4,w,23,Handle,(HMENU)205,HInstance,NULL);
  Wid=110; // manual...
#ifdef SSE_BUGFIX_394
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST
   | WS_VSCROLL | disable,page_l+5+w+Offset,y,Wid,200,Handle,(HMENU)7319,
   HInstance,NULL);
#else
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                          page_l+5+w+Offset,y,Wid,200,Handle,(HMENU)7319,HInstance,NULL);
#endif
  ToolAddWindow(ToolTip,Win,T("With Direct3D option you have the choice between all the 32bit modes your video card can handle."));
  D3DDISPLAYMODE Mode;
  for(UINT i=0;i<nD3Dmodes;i++)
  {
    Disp.pD3D->EnumAdapterModes(Adapter,DisplayFormat,i,&Mode);
    char tmp[20];
    sprintf(tmp,"%dx%d %dhz",Mode.Width,Mode.Height,Mode.RefreshRate);
    SendMessage(Win,CB_ADDSTRING,0,(LPARAM)tmp);
  }
  SendMessage(Win,CB_SETCURSEL,Disp.D3DMode,0);
  y+=LineHeight;
  Wid+=w;

ADVANCED_END

#endif//#if defined(SSE_VID_D3D_LIST_MODES)
  
#if defined(SSE_GUI_D3D_CRISP_OPTION) //can't say code is getting cleaner
#if defined(SSE_GUI_CRISP_IN_DISPLAY)
ADVANCED_BEGIN
#else
  {
#endif
  Wid=GetCheckBoxSize(Font,T("Crisp Rendering")).Width;
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
  Win=CreateWindow("Button",T("Crisp Rendering"),mask,
    page_l,y-1,Wid,25,Handle,(HMENU)7324,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_D3D_CRISP,0);
  ToolAddWindow(ToolTip,Win,T("You like those big pixels? That's the option for you"));
  y+=LineHeight;
ADVANCED_END
#endif

#endif//#if defined(SSE_VID_D3D)

#if defined(SSE_GUI_DD_FULLSCREEN_LE)
  y+=LineHeight;
#endif

#if defined(SSE_VID_FS_GUI_OPTION)
/*  This is handy if it works. Blame Microsoft and video card drivers 
    if it doesn't.
*/
  w=GetCheckBoxSize(Font,T("Fullscreen GUI")).Width;
  Win=CreateWindow("Button",T("Fullscreen GUI"),mask,
    page_l,y,w,23,Handle,(HMENU)7325,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_FULLSCREEN_GUI,0);
  ToolAddWindow(ToolTip,Win,T("Depends on system, leaving this unchecked is safer\
 but if it works it's quite handy"));
#if defined(SSE_GUI_392) //more logical to have this option close to 'Fullscreen GUI'
  if(!OPTION_FULLSCREEN_GUI)
    mask|=WS_DISABLED;
  w=GetCheckBoxSize(Font,T("Confirm Before Quit")).Width;
  Win=CreateWindow("Button",T("Confirm Before Quit"),mask,page_l+130,y,w,23,
    Handle,(HMENU)226,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,FSQuitAskFirst,0);
#endif//#if defined(SSE_GUI_392)
  y+=LineHeight;
#endif//#if defined(SSE_VID_FS_GUI_OPTION)

#if defined(SSE_GUI_DD_FULLSCREEN_LE)
  y-=LineHeight*2;
#endif

#if !defined(SSE_VID_D3D)
  w=get_text_width(T("Drawing mode"));
  CreateWindow("Static",T("Drawing mode"),WS_CHILD ,
                          page_l,y+4,w,23,Handle,(HMENU)205,HInstance,NULL);
#if defined(SSE_VID_BPP_CHOICE)
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+5+w,y,100,200,Handle,(HMENU)204,HInstance,NULL);
#else
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+5+w,y,page_w-(5+w),200,Handle,(HMENU)204,HInstance,NULL);
#endif
#if defined(SSE_VID_DD_FS_MAXRES)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Screen Flip"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Straight Blit"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Stretch Blit"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Max Resolution"));
#elif defined(SSE_X64_390)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Screen Flip"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Straight Blit"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Stretch Blit"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Laptop"));
#else
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Screen Flip"));
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Straight Blit"));
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Stretch Blit"));
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Laptop"));
#endif
  SendMessage(Win,CB_SETCURSEL,draw_fs_blit_mode,0);
#if defined(SSE_VID_BORDERS) 
#if SSE_VERSION>=370
#if defined(SSE_VID_DD_FS_MAXRES)
    ToolAddWindow(ToolTip,Win,
T("Flip recommended. Only 'Max Resolution' will work with large borders"));
#else
    ToolAddWindow(ToolTip,Win,T("DirectDraw: Screen Flip only works with 384 x 270. Straight blit OK with 400 x 278. For higher than 400 first switch to laptop mode.\
                                 \rDirect3D: Screen Flip or Straight Blit for small crisp screen. Stretch Blit or Laptop for big screen."));
#endif
#else
    ToolAddWindow(ToolTip,Win,T("SSE note: Screen Flip only works with 384 x 270. Straight blit OK with 400 x 278. For higher than 400 first switch to laptop mode."));
#endif
#endif
#else
  int old_y=y;
  y=10; 
#endif

#if defined(SSE_VID_BPP_CHOICE) && !defined(SSE_VID_BPP_NO_CHOICE)
  ADVANCED_BEGIN
  w=get_text_width(T("Bits per pixel"));
  int offset=190;
  CreateWindow("Static",T("Bits per pixel"),WS_CHILD ,
                          page_l+offset,y+4,w,23,Handle,(HMENU)205,HInstance,NULL);
#ifdef SSE_BUGFIX_394
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | disable,
                          page_l+5+w+offset,y,40,208,Handle,(HMENU)208,HInstance,NULL);
#else
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                          page_l+5+w+offset,y,40,208,Handle,(HMENU)208,HInstance,NULL);
#endif
  int nconfig=0;
  if(SSEConfig.VideoCard8bit)
    SendMessage(Win,CB_ADDSTRING,nconfig++,(LPARAM)CStrT("8"));
  if(SSEConfig.VideoCard16bit)
    SendMessage(Win,CB_ADDSTRING,nconfig++,(LPARAM)CStrT("16"));
  if(SSEConfig.VideoCard32bit)
    SendMessage(Win,CB_ADDSTRING,nconfig++,(LPARAM)CStrT("32"));
  SendMessage(Win,CB_SETCURSEL,min((int)display_option_fs_bpp,nconfig-1),0);
  ADVANCED_END
#endif

#if defined(SSE_VID_D3D)
  y=old_y;
#else
#if defined(SSE_GUI_DD_FULLSCREEN_LE)
  y+=LineHeight;
#endif
  y+=30;

  int disabledflag=0;

  if (draw_fs_blit_mode==DFSM_STRETCHBLIT || draw_fs_blit_mode==DFSM_LAPTOP) disabledflag=WS_DISABLED;
  w=GetCheckBoxSize(Font,T("Scanline Grille")).Width;
#if defined(SSE_GUI_DD_FULLSCREEN_LE)
  Win=CreateWindow("Button",T("Scanline Grille"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | disabledflag,
                          page_l,y,w,23,Handle,(HMENU)280,HInstance,NULL);

#elif defined(SSE_VID_BPP_CHOICE) || defined(SSE_VID_BPP_NO_CHOICE)
  Win=CreateWindow("Button",T("Scanline Grille"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | disabledflag,
                          page_l+230,y,w,23,Handle,(HMENU)280,HInstance,NULL);
#elif defined(SSE_VID_FS_GUI_OPTION)
  Win=CreateWindow("Button",T("Scanline Grille"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | disabledflag,
                          page_l+130,y,w,23,Handle,(HMENU)280,HInstance,NULL);
#else
  Win=CreateWindow("Button",T("Scanline Grille"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | disabledflag,
                          page_l,y,w,23,Handle,(HMENU)280,HInstance,NULL);
#endif
  SendMessage(Win,BM_SETCHECK,(draw_fs_fx==DFSFX_GRILLE ? BST_CHECKED:BST_UNCHECKED),0);
#if !defined(SSE_VID_FS_GUI_OPTION)
  y+=30;
#endif
  
#if !defined(SSE_VID_BPP_CHOICE) &&!defined(SSE_VID_BPP_NO_CHOICE)
  w=GetCheckBoxSize(Font,T("Use 256 colour mode")).Width;
#if defined(SSE_VID_DD_FS_32BIT) //disable if unavailable
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX | disabledflag;
  if(!SSEConfig.VideoCard8bit||OPTION_D3D&&D3D9_OK)
    mask|=WS_DISABLED;
  Win=CreateWindow("Button",T("Use 256 colour mode"),
    mask,
    page_l,y,w,23,Handle,(HMENU)208,HInstance,NULL);
#else
  Win=CreateWindow("Button",T("Use 256 colour mode"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l,y,w,23,Handle,(HMENU)208,HInstance,NULL);
#endif    
  SendMessage(Win,BM_SETCHECK,display_option_8_bit_fs,0);
  ToolAddWindow(ToolTip,Win,T("When this option is ticked Steem will use 256 colour mode in fullscreen, this is much faster but some screen effects involving many colours will not work"));
  y+=30;
#endif
#if defined(SSE_GUI_DD_FULLSCREEN_LE)
  y+=LineHeight;
#endif
#if defined(SSE_VID_GUI_392)
  w=GetCheckBoxSize(Font,T("Use 640x400 (no borders only)")).Width;
  
  Win=CreateWindow("Button",T("Use 640x400 (no borders only)"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l,y,w,23,Handle,(HMENU)210,HInstance,NULL);    
#else
  w=GetCheckBoxSize(Font,T("Use 640x400 (never show borders only)")).Width;
  
  Win=CreateWindow("Button",T("Use 640x400 (never show borders only)"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l,y,w,23,Handle,(HMENU)210,HInstance,NULL);    
#endif
  ToolAddWindow(ToolTip,Win,T("When this option is ticked Steem will use the 600x400 PC screen resolution in fullscreen if it can"));
  if (draw_fs_blit_mode==DFSM_LAPTOP){
    EnableWindow(Win,0);
  }else{
    EnableWindow(Win,border==0);
  }
  SendMessage(Win,BM_SETCHECK,prefer_res_640_400,0);
  y+=30;
#if !defined(SSE_VID_DD_SIMPLIFY_VSYNC)
  CreateWindow("Button",T("Synchronisation"),
    WS_CHILD | BS_GROUPBOX,
    page_l,y,page_w,170,Handle,(HMENU)99,HInstance,NULL);
  y+=20;
#endif
#endif//#if !defined(SSE_VID_D3D)

ADVANCED_BEGIN

#if defined(SSE_VID_GUI_392) // DD + D3D
  w=GetCheckBoxSize(Font,T("VSync")).Width;
#if defined(SSE_VID_DD) //in a group rectangle
  Win=CreateWindow("Button",T("VSync"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l+10,y,w,23,Handle,(HMENU)206,HInstance,NULL);
#else
#ifdef SSE_BUGFIX_394
  Win=CreateWindow("Button",T("VSync"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX |disable,
    page_l,y,w,23,Handle,(HMENU)206,HInstance,NULL);
#else
  Win=CreateWindow("Button",T("VSync"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l,y,w,23,Handle,(HMENU)206,HInstance,NULL);
#endif
#endif
#else // Steem 3.2
  w=GetCheckBoxSize(Font,T("Vsync to PC display")).Width;
  Win=CreateWindow("Button",T("Vsync to PC display"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l+10,y,w,23,Handle,(HMENU)206,HInstance,NULL);
#endif  
  SendMessage(Win,BM_SETCHECK,FSDoVsync,0);
  ToolAddWindow(ToolTip,Win,T("When this option is ticked Steem will synchronise the PC monitor with the ST in fullscreen mode, this makes some things look a lot smoother but can be very slow.")+
                              " "+T("The ST used 50Hz (PAL), 60Hz (NTSC) and 70Hz (Mono), for good synchronisation you should set the PC refresh rate to the same or double the ST refresh rate."));

#if defined(SSE_VID_3BUFFER_FS) // DD + D3D
  w=GetCheckBoxSize(Font,T("Triple Buffering")).Width;
#ifdef SSE_BUGFIX_394
  Win=CreateWindow("Button",T("Triple Buffering"), WS_CHILD | WS_TABSTOP | BS_CHECKBOX |disable,
               page_l+130,y,w,25,Handle,(HMENU)1037,HInstance,NULL);
#else
  Win=CreateWindow("Button",T("Triple Buffering"), WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
               page_l+130,y,w,25,Handle,(HMENU)1037,HInstance,NULL);
#endif
  SendMessage(Win,BM_SETCHECK,OPTION_3BUFFER_FS,0);
  ToolAddWindow(ToolTip,Win,T("Yes, we add a buffer :) You decide if it's better or not."));
#endif

#if defined(SSE_VID_D3D_FS_DEFAULT_HZ)
  y+=LineHeight;
  w=GetCheckBoxSize(Font,T("Use Desktop Refresh Rate")).Width;
#ifdef SSE_BUGFIX_394
  Win=CreateWindow("Button",T("Use Desktop Refresh Rate"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX |disable,
    page_l+10-10,y,w,23,Handle,(HMENU)209,HInstance,NULL);
#else
  Win=CreateWindow("Button",T("Use Desktop Refresh Rate"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l+10-10,y,w,23,Handle,(HMENU)209,HInstance,NULL);
#endif
  ToolAddWindow(ToolTip,Win,T("This will bypass the hz setting in Mode, useful for some NVIDIA cards"));
  SendMessage(Win,BM_SETCHECK,OPTION_FULLSCREEN_DEFAULT_HZ,0);
#endif

#if defined(SSE_VID_D3D_FAKE_FULLSCREEN)
  y+=LineHeight;
  w=GetCheckBoxSize(Font,T("Windowed Borderless Mode")).Width;
  Win=CreateWindow("Button",T("Windowed Borderless Mode"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l,y,w,23,Handle,(HMENU)210,HInstance,NULL);
#ifdef SSE_BUGFIX_394
  ToolAddWindow(ToolTip,Win,T("Safer fullscreen mode, like a big window without frame"));
#else
  ToolAddWindow(ToolTip,Win,T("This 'fake' fullscreen mode bypasses most everything else on this page!"));
#endif
  SendMessage(Win,BM_SETCHECK,OPTION_FAKE_FULLSCREEN,0);
#endif

ADVANCED_END

#if !defined(SSE_VID_D3D) && !defined(SSE_VID_DD_SIMPLIFY_VSYNC)
  y+=30;

  CreateWindow("Static",T("Preferred PC refresh rates:"),WS_CHILD,
                          page_l+10,y,page_w-20,25,Handle,(HMENU)99,HInstance,NULL);
  y+=25;

  w=get_text_width("640x400");
  CreateWindow("Static","640x400",WS_CHILD,
                          page_l+10,y+4,w,25,Handle,(HMENU)99,HInstance,NULL);

  Win=CreateWindow("Combobox","",
    WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
    page_l+15+w,y,page_w-10-(105+w),200,Handle,(HMENU)220,HInstance,NULL);
#if defined(SSE_X64_390)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Default"));
#else
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Default"));
#endif
  for (int n=1;n<NUM_HZ;n++){
    SendMessage(Win,CB_ADDSTRING,0,LPARAM((Str(HzIdxToHz[n])+"Hz").Text));
  }

  CreateWindowEx(512,"Steem Path Display","",
    WS_CHILD | PDS_VCENTRESTATIC,
    page_l+page_w-90,y,80,23,Handle,(HMENU)221,HInstance,NULL);
  y+=30;

  w=get_text_width("640x480");
  CreateWindow("Static","640x480",WS_CHILD,
    page_l+10,y+4,w,25,Handle,(HMENU)99,HInstance,NULL);

  Win=CreateWindow("Combobox","",
    WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
    page_l+15+w,y,page_w-10-(105+w),200,Handle,(HMENU)222,HInstance,NULL);
#if defined(SSE_X64_390)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Default"));
#else
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Default"));
#endif
  for (int n=1;n<NUM_HZ;n++){
    SendMessage(Win,CB_ADDSTRING,0,LPARAM((Str(HzIdxToHz[n])+"Hz").Text));
  }
 
  CreateWindowEx(512,"Steem Path Display","",
    WS_CHILD | PDS_VCENTRESTATIC,
    page_l+page_w-90,y,80,23,Handle,(HMENU)223,HInstance,NULL);

  y+=30;
  w=get_text_width("800x600");
  CreateWindow("Static","800x600",
    WS_CHILD,page_l+10,y+4,w,25,Handle,(HMENU)99,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
    page_l+15+w,y,page_w-10-(105+w),200,Handle,(HMENU)224,HInstance,NULL);
#if defined(SSE_X64_390)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Default"));
#else
  SendMessage(Win,CB_ADDSTRING,0,(long)CStrT("Default"));
#endif
  for (int n=1;n<NUM_HZ;n++){
    SendMessage(Win,CB_ADDSTRING,0,LPARAM((Str(HzIdxToHz[n])+"Hz").Text));
  }

  CreateWindowEx(512,"Steem Path Display","",
    WS_CHILD | PDS_VCENTRESTATIC,
    page_l+page_w-90,y,80,23,Handle,(HMENU)225,HInstance,NULL);

  y+=40;
#endif//#if !defined(SSE_VID_D3D)

#if !defined(SSE_GUI_392) //moved up
  w=GetCheckBoxSize(Font,T("Confirm before quit")).Width;
  Win=CreateWindow("Button",T("Confirm before quit"),
    WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
    page_l,y,w,23,Handle,(HMENU)226,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,FSQuitAskFirst,0);
#endif//#if !defined(SSE_GUI_392)

#if !defined(SSE_VID_D3D)
  UpdateHzDisplay();
  if (Focus==NULL) Focus=GetDlgItem(Handle,204);
#endif

  SetPageControlsFont();
  ShowPageControls();
}
#if !defined(SSE_VID_D3D)
void TOptionBox::UpdateHzDisplay()
{
  if (Handle==NULL) return;
  if (GetDlgItem(Handle,220)==NULL) return;
#if defined(SSE_VID_BPP_CHOICE) && !defined(SSE_VID_BPP_NO_CHOICE)
  int c256=int((display_option_fs_bpp==0) ? 0:1);
#else
  int c256=int(display_option_8_bit_fs ? 0:1);
#endif
  for (int i=0;i<3;i++){
    for (int n=0;n<NUM_HZ;n++){
      if (HzIdxToHz[n]==prefer_pc_hz[c256][i]){
        SendDlgItemMessage(Handle,220+i*2,CB_SETCURSEL,n,0);
        break;
      }
    }

    EasyStr Text=T("UNTESTED");
    if (prefer_pc_hz[c256][i]){
      if (LOBYTE(tested_pc_hz[c256][i])==prefer_pc_hz[c256][i]){
        if (HIBYTE(tested_pc_hz[c256][i])){
          Text=T("OK");
        }else{
          Text=T("FAILED");
        }
      }
    }else{
      Text=T("OK");
    }
    SendDlgItemMessage(Handle,221+i*2,WM_SETTEXT,0,LPARAM(Text.Text));
  }
}
#endif
//---------------------------------------------------------------------------
void TOptionBox::CreateMIDIPage()
{
  HWND Win;
  int w,y=10,Wid,Wid2,x;

  Wid=GetTextSize(Font,T("Volume")+": "+T("Min")).Width;
  CreateWindow("Static",T("Volume")+": "+T("Min"),WS_CHILD,
                page_l,y+4,Wid,23,Handle,HMENU(6000),HInstance,NULL);

  Wid2=GetTextSize(Font,T("Max")).Width;

  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l+Wid+5,y,page_w-(Wid2+5)-(Wid+5),27,Handle,HMENU(6001),HInstance,NULL);
  SendMessage(Win,TBM_SETRANGEMAX,0,0xffff);
  SendMessage(Win,TBM_SETPOS,true,MIDI_out_volume);
  SendMessage(Win,TBM_SETLINESIZE,0,0xff);
  SendMessage(Win,TBM_SETPAGESIZE,0,0xfff);

  CreateWindow("Static",T("Max"),WS_CHILD,
                page_l+page_w-Wid2,y+4,Wid2,23,Handle,HMENU(6002),HInstance,NULL);
  y+=35;

  w=GetCheckBoxSize(Font,T("Allow running status for output")).Width;
  Win=CreateWindow("Button",T("Allow running status for output"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,w,23,Handle,(HMENU)6010,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,int((MIDI_out_running_status_flag==MIDI_ALLOW_RUNNING_STATUS) ? BST_CHECKED:BST_UNCHECKED),0);
  y+=30;

  w=GetCheckBoxSize(Font,T("Allow running status for input")).Width;
  Win=CreateWindow("Button",T("Allow running status for input"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,w,23,Handle,(HMENU)6011,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,int((MIDI_in_running_status_flag==MIDI_ALLOW_RUNNING_STATUS) ? BST_CHECKED:BST_UNCHECKED),0);
  y+=30;

  CreateWindow("Button",T("System Exclusive Buffers"),WS_CHILD | BS_GROUPBOX,
                  page_l,y,page_w,85,Handle,(HMENU)99,HInstance,NULL);
  y+=20;

  x=page_l+10;
  Wid=GetTextSize(Font,T("Available for output")).Width;
  CreateWindow("Static",T("Available for output"),WS_CHILD,
                x,y+4,Wid,20,Handle,HMENU(6020),HInstance,NULL);
  x+=Wid+5;

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                          x,y,40,200,Handle,HMENU(6021),HInstance,NULL);
  for (int n=2;n<MAX_SYSEX_BUFS;n++){
#if defined(SSE_X64_390)
    SendMessage(Win,CB_ADDSTRING,0,(LPARAM)Str(n).Text);
#else
    SendMessage(Win,CB_ADDSTRING,0,(long)Str(n).Text);
#endif
  }
  SendMessage(Win,CB_SETCURSEL,MIDI_out_n_sysex-2,0);
  x+=45;

  Wid=GetTextSize(Font,T("size")).Width;
  CreateWindow("Static",T("size"),WS_CHILD,
                x,y+4,Wid,20,Handle,HMENU(6022),HInstance,NULL);
  x+=Wid+5;

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                          x,y,page_w-10-(x-page_l),200,Handle,HMENU(6023),HInstance,NULL);
#if defined(SSE_X64_390)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"16Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"32Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"64Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"128Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"256Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"512Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"1Mb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"2Mb");
#else
  SendMessage(Win,CB_ADDSTRING,0,(long)"16Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"32Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"64Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"128Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"256Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"512Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"1Mb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"2Mb");
#endif
  SendMessage(Win,CB_SETCURSEL,log_to_base_2(MIDI_out_sysex_max/1024)-4,0);
  y+=30;

  x=page_l+10;
  Wid=GetTextSize(Font,T("Available for input")).Width;
  CreateWindow("Static",T("Available for input"),WS_CHILD,
                x,y+4,Wid,20,Handle,HMENU(6030),HInstance,NULL);

  x+=Wid+5;

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                          x,y,40,200,Handle,HMENU(6031),HInstance,NULL);
  for (int n=2;n<MAX_SYSEX_BUFS;n++){
#if defined(SSE_X64_390)
    SendMessage(Win,CB_ADDSTRING,0,(LPARAM)Str(n).Text);
#else
    SendMessage(Win,CB_ADDSTRING,0,(long)Str(n).Text);
#endif
  }
  SendMessage(Win,CB_SETCURSEL,MIDI_in_n_sysex-2,0);
  x+=45;

  Wid=GetTextSize(Font,T("size")).Width;
  CreateWindow("Static",T("size"),WS_CHILD,
                x,y+4,Wid,20,Handle,HMENU(6032),HInstance,NULL);
  x+=Wid+5;


  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                          x,y,page_w-10-(x-page_l),200,Handle,HMENU(6033),HInstance,NULL);
#if defined(SSE_X64_390)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"16Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"32Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"64Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"128Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"256Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"512Kb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"1Mb");
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)"2Mb");
#else
  SendMessage(Win,CB_ADDSTRING,0,(long)"16Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"32Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"64Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"128Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"256Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"512Kb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"1Mb");
  SendMessage(Win,CB_ADDSTRING,0,(long)"2Mb");
#endif
  SendMessage(Win,CB_SETCURSEL,log_to_base_2(MIDI_in_sysex_max/1024) - 4,0);
  y+=30;

  y+=13;

  CreateWindow("Static",T("Input speed")+": "+Str(MIDI_in_speed)+"%",WS_CHILD | SS_CENTER,
                page_l,y,page_w,20,Handle,HMENU(6040),HInstance,NULL);
  y+=20;

  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,27,Handle,HMENU(6041),HInstance,NULL);
  SendMessage(Win,TBM_SETRANGEMAX,0,99);
  SendMessage(Win,TBM_SETPOS,true,MIDI_in_speed-1);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  SendMessage(Win,TBM_SETPAGESIZE,0,5);
  for (int n=4;n<99;n+=5) SendMessage(Win,TBM_SETTIC,0,n);
  y+=40;

  Win=CreateWindowEx(512,"Edit",T("The Steem MIDI interface is only suitable for programs that communicate using MIDI messages.")+"\r\n\r\n"+
                        T("Any program that attempts to send raw data over the MIDI ports (for example a MIDI network game) will not work."),
                        WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                        page_l,y,page_w,OPTIONS_HEIGHT-y-10,Handle,(HMENU)0,HInstance,NULL);
  MakeEditNoCaret(Win);


  if (Focus==NULL) Focus=GetDlgItem(Handle,6001);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
void TOptionBox::CreateSoundPage()
{
  HWND Win;
  long Wid;
#if defined(SSE_GUI_OPTIONS_SOUND) 
  long Offset=0;
  const int LineHeight=30;
#if defined(SSE_YM2149_MAMELIKE_394)
  const int HorizontalSeparation=5;
#else
  const int HorizontalSeparation=10;
#endif
  DWORD mask;
#endif
  int y=10;

#if defined(SSE_SOUND_CAN_CHANGE_DRIVER)
  ConfigStoreFile CSF(INIFile);
  Wid=get_text_width(T("Sound driver"));
  CreateWindow("Static",T("Sound driver"),WS_CHILD,
                          page_l,y+4,Wid,20,Handle,(HMENU)3000,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                    page_l+5+Wid,y,page_w-(5+Wid),200,Handle,(HMENU)3001,HInstance,NULL);
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Default"));
  for (int i=0;i<DSDriverModuleList.NumStrings;i++){
    SendMessage(Win,CB_ADDSTRING,0,(LPARAM)DSDriverModuleList[i].String);
  }
  SendMessage(Win,CB_SETCURSEL,0,0);
  EasyStr DSDriverModName=CSF.GetStr("Options","DSDriverName","");
  if (DSDriverModName.NotEmpty()){
    for (int i=0;i<DSDriverModuleList.NumStrings;i++){
      if (IsSameStr_I(DSDriverModuleList[i].String,DSDriverModName)){
        SendMessage(Win,CB_SETCURSEL,1+i,0);
        break;
      }
    }
  }
  CSF.Close();
  y+=LineHeight;
#endif//#if defined(SSE_SOUND_CAN_CHANGE_DRIVER)

  DWORD DisableIfMute=DWORD(((sound_mode==SOUND_MODE_MUTE) || UseSound==0) ? WS_DISABLED:0);
  DWORD DisableIfNoSound=DWORD((UseSound==0) ? WS_DISABLED:0);
#if !(defined(SSE_VS2008_WARNING_382) && defined(SOUND_DISABLE_INTERNAL_SPEAKER))
  DWORD DisableIfNT=DWORD(WinNT ? WS_DISABLED:0);
#endif
  Wid=GetTextSize(Font,T("Output type")).Width;
  CreateWindow("Static",T("Output type"),WS_CHILD | DisableIfNoSound,
                  page_l,y+4,Wid,23,Handle,(HMENU)7049,HInstance,NULL);
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP |
                  DisableIfNoSound | CBS_DROPDOWNLIST,
                  page_l+5+Wid,y,page_w-(5+Wid),200,Handle,(HMENU)7099,HInstance,NULL),
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("None (Mute)"));

#if defined(SSE_SOUND_FILTERS)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("No filter"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Filter 'coaxial' (Steem original)"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Filter 'SCART'"));
#if !defined(SSE_SOUND_FEWER_FILTERS)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Filter 'coaxial' tunes only"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Filter 'coaxial' samples only"));
#endif
#if defined(SSE_SOUND_FILTER_HATARI)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Filter 'Hatari'"));
#endif
  y+=LineHeight;
#else // Steem 3.2
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("None (Mute)"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Simulated ST Speaker"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Direct"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Sharp STFM Samples"));
  SendMessage(Win,CB_SETCURSEL,sound_mode,0);
#endif
  SendMessage(Win,CB_SETCURSEL,sound_mode,0);

#if defined(SSE_GUI_OPTIONS_SAMPLED_YM) && !defined(SSE_YM2149_TABLE_NOT_OPTIONAL)
ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Sampled YM-2149")).Width;
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
  if(!SSEConfig.ym2149_fixed_vol && !YM2149.LoadFixedVolTable())
    mask|=WS_DISABLED;
#endif
  Win=CreateWindow("Button",T("Sampled YM2149"),mask,
    page_l+HorizontalSeparation,y,Wid,25,Handle,(HMENU)7311,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_SAMPLED_YM,0);
  ToolAddWindow(ToolTip,Win,
    T("Punchier P.S.G. (YM2149) sound using a table by ljbk, thx dude!"));
  y+=LineHeight;
ADVANCED_END
#endif

#if defined(SSE_YM2149_MAMELIKE) && defined(SSE_YM2149_MAMELIKE_394)
ADVANCED_BEGIN
  y-=LineHeight;
  Offset+=Wid+HorizontalSeparation*2;
  Wid=GetCheckBoxSize(Font,T("Alt YM2149 emu")).Width;
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
  Win=CreateWindow("Button",T("Alt YM2149 emu"),mask,
    page_l+Offset,y,Wid,25,Handle,(HMENU)7312,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_MAME_YM,0);
  ToolAddWindow(ToolTip,Win,T("Inspired by MAME, thx Couriersud!"));
  y+=LineHeight;
ADVANCED_END
#endif

#if defined(SSE_GUI_OPTIONS_MICROWIRE)
ADVANCED_BEGIN
  Offset+=Wid+HorizontalSeparation*2;
#if defined(SSE_GUI_OPTIONS_SAMPLED_YM)
  y-=LineHeight; // maybe it will be optimised away!
#endif
  Wid=GetCheckBoxSize(Font,T("Microwire")).Width;
#if !defined(SSE_YM2149_MAMELIKE_394)
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
#endif
#if defined(SSE_SOUND_OPTION_DISABLE_DSP)
  if(DSP_DISABLED)
    mask|=WS_DISABLED;
#endif

  Win=CreateWindow("Button",T("Microwire"),mask,
    page_l+Offset,y,Wid,25,Handle,(HMENU)7302,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,SSEOption.Microwire,0);
  ToolAddWindow(ToolTip,Win,
    T("Microwire (for STE sound), incomplete emulation"));
  y+=LineHeight;
ADVANCED_END
#endif


#if defined(SSE_GUI_OPTIONS_SOUND)
#if defined(SSE_SOUND_CAN_CHANGE_DRIVER)
  Str DrivStr="Settings"; //TODO
#else
  Str DrivStr=T("Device ");
  EasyStr DSDriverModName=GetCSFStr("Options","DSDriverName","",INIFile);
  if (DSDriverModName.Empty()) DSDriverModName=T("Default");
  DrivStr+=DSDriverModName;
#endif
#if defined(SSE_SOUND_ENFORCE_RECOM_OPT)
  CreateWindow("Button",DrivStr.Text,WS_CHILD | BS_GROUPBOX | DisableIfMute,
                  page_l,y,page_w,230-60-30-30,Handle,(HMENU)7105,HInstance,NULL);
#else
  CreateWindow("Button",DrivStr.Text,WS_CHILD | BS_GROUPBOX | DisableIfMute,
                  page_l,y,page_w,230-30-30,Handle,(HMENU)7105,HInstance,NULL);
#endif
  y+=20;
#else
  y+=30;
  CreateWindow("Button",T("Configuration"),WS_CHILD | BS_GROUPBOX | DisableIfMute,
                  page_l,y,page_w,230,Handle,(HMENU)7105,HInstance,NULL);
  y+=20;
  Str DrivStr=T("Current driver")+": ";
  EasyStr DSDriverModName=GetCSFStr("Options","DSDriverName","",INIFile);
  if (DSDriverModName.Empty()) DSDriverModName=T("Default");
  DrivStr+=DSDriverModName;
  CreateWindow("Static",DrivStr,WS_CHILD,
                  page_l+10,y,page_w-20,23,Handle,(HMENU)7010,HInstance,NULL);
  y+=25;
#endif

  Wid=GetTextSize(Font,T("Volume")+": "+T("Min")).Width;
  CreateWindow("Static",T("Volume")+": "+T("Min"),WS_CHILD | DisableIfMute,
                  page_l+10,y+4,Wid,23,Handle,(HMENU)7050,HInstance,NULL);

  int Wid2=GetTextSize(Font,T("Max")).Width;
  CreateWindow("Static",T("Max"),WS_CHILD | DisableIfMute,
                  page_l+page_w-10-Wid2,y+4,Wid2,23,Handle,(HMENU)7051,HInstance,NULL);
#if defined(SSE_GUI_OPTIONS_SOUND)
  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP |
                    DisableIfMute | TBS_HORZ,
                    page_l+15+Wid,y,(page_w-10-(Wid2+5))-(Wid+15),20,Handle,(HMENU)7100,HInstance,NULL);
#else
  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP |
                    DisableIfMute | TBS_HORZ,
                    page_l+15+Wid,y,(page_w-10-(Wid2+5))-(Wid+15),27,Handle,(HMENU)7100,HInstance,NULL);
#endif
#if defined(SSE_SOUND_VOL_LOGARITHMIC) // more intuitive setting
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,100));
  int db=MaxVolume;
#if defined(SSE_VS2008)
  int position= pow(10, log10((float)101)*(db + 10000)/10000 )-1 ;
#else
  int position= pow(10, log10(101)*(db + 10000)/10000 )-1 ;
#endif
  SendMessage(Win,TBM_SETPOS,1,position);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  SendMessage(Win,TBM_SETPAGESIZE,0,10);
#else
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,9000));
  SendMessage(Win,TBM_SETPOS,1,MaxVolume+9000);
  SendMessage(Win,TBM_SETLINESIZE,0,100);
  SendMessage(Win,TBM_SETPAGESIZE,0,1000);
#endif

#if defined(SSE_GUI_OPTIONS_SOUND)
  y+=LineHeight;
#else
  y+=35;
#endif

  Wid=GetTextSize(Font,T("Frequency")).Width;
  CreateWindow("Static",T("Frequency"),WS_CHILD | DisableIfMute,
                  page_l+10,y+4,Wid,23,Handle,(HMENU)7052,HInstance,NULL);
#if defined(SSE_GUI_OPTIONS_SOUND)
  Offset=70;
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP |
                    DisableIfMute | CBS_DROPDOWNLIST,
                    page_l+15+Wid,y,Offset,200,Handle,(HMENU)7101,HInstance,NULL);
  Offset+=Wid;
#else
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP |
                    DisableIfMute | CBS_DROPDOWNLIST,
                    page_l+15+Wid,y,page_w-10-(15+Wid),200,Handle,(HMENU)7101,HInstance,NULL);
#endif
  if (sound_comline_freq){
    CBAddString(Win,Str(sound_comline_freq)+"Hz",sound_comline_freq);
  }
#if defined(SSE_SOUND_MORE_SAMPLE_RATES)
  CBAddString(Win,"384Khz",384000);
#if defined(SSE_SOUND_250K)
  CBAddString(Win,"250Khz",250000); //lol!
#endif
  CBAddString(Win,"192Khz",192000);
  CBAddString(Win,"96Khz",96000);
  CBAddString(Win,"50Khz",50066);
  CBAddString(Win,"48Khz",48000);
  CBAddString(Win,"44.1 Khz",44100);
  CBAddString(Win,"25Khz",25033);
  CBAddString(Win,"22Khz",22050);
#else
  CBAddString(Win,"50066Hz",50066);
  CBAddString(Win,"44100Hz",44100);
  CBAddString(Win,"25033Hz",25033);
  CBAddString(Win,"22050Hz",22050);
#endif
  if (CBSelectItemWithData(Win,sound_chosen_freq)==-1){
    SendMessage(Win,CB_SETCURSEL,CBAddString(Win,Str(sound_chosen_freq)+"Hz",sound_chosen_freq),0);
  }
#if !defined(SSE_GUI_OPTIONS_SOUND) 
  y+=30;
#endif

  Wid=GetTextSize(Font,T("Format")).Width;
#if defined(SSE_GUI_OPTIONS_SOUND) 
  Offset+=25;
  CreateWindow("Static",T("Format"),WS_CHILD | DisableIfMute,
                  page_l+Offset,y+4,Wid,23,Handle,(HMENU)7060,HInstance,NULL);
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP |
                    DisableIfMute | CBS_DROPDOWNLIST,
                    page_l+Offset+Wid,y,95,200,Handle,(HMENU)7061,HInstance,NULL);
#else
  CreateWindow("Static",T("Format"),WS_CHILD | DisableIfMute,
                  page_l+10,y+4,Wid,23,Handle,(HMENU)7060,HInstance,NULL);
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP |
                    DisableIfMute | CBS_DROPDOWNLIST,
                    page_l+15+Wid,y,page_w-10-(15+Wid),200,Handle,(HMENU)7061,HInstance,NULL);
#endif
#if !defined(SSE_SOUND_NO_8BIT)
ADVANCED_BEGIN // way to make it sound bad!
  CBAddString(Win,T("8-Bit Mono"),MAKEWORD(8,1));
  CBAddString(Win,T("8-Bit Stereo"),MAKEWORD(8,2));
ADVANCED_END
#endif
  CBAddString(Win,T("16-Bit Mono"),MAKEWORD(16,1));
  CBAddString(Win,T("16-Bit Stereo"),MAKEWORD(16,2));
#if defined(SSE_SOUND_NO_8BIT)
  SendMessage(Win,CB_SETCURSEL,(sound_num_channels-1),0);
#else  
ADVANCED_BEGIN
  SendMessage(Win,CB_SETCURSEL,(sound_num_bits-8)/4 + (sound_num_channels-1),0);
ADVANCED_ELSE
  SendMessage(Win,CB_SETCURSEL,(sound_num_channels-1),0);
ADVANCED_END
#endif
  y+=30;

#if !defined(SSE_SOUND_ENFORCE_RECOM_OPT)
ADVANCED_BEGIN
#if defined(SSE_SOUND_RECOMMEND_OPTIONS)
  Wid=GetCheckBoxSize(Font,T("Write to primary buffer (not recommended)")).Width;
  Win=CreateWindow("Button",T("Write to primary buffer (not recommended)"),WS_CHILD | WS_TABSTOP |
                          BS_CHECKBOX | DisableIfMute,
                          page_l+10,y,Wid,23,Handle,(HMENU)7102,HInstance,NULL);
#else
  Wid=GetCheckBoxSize(Font,T("Write to primary buffer")).Width;
  Win=CreateWindow("Button",T("Write to primary buffer"),WS_CHILD | WS_TABSTOP |
                          BS_CHECKBOX | DisableIfMute,
                          page_l+10,y,Wid,23,Handle,(HMENU)7102,HInstance,NULL);
#endif
  SendMessage(Win,BM_SETCHECK,sound_write_primary,0);
  ToolAddWindow(ToolTip,Win,T("Steem tries to output sound in a way that is friendly to other programs.")+" "+
#if defined(SSE_SOUND_RECOMMEND_OPTIONS)
    T("Check this option ONLY if you have problems with your soundcard."));
#else
  T("However some sound cards do not like that, if you are having problems check this option to make Steem take full control."));
#endif
  y+=30;

  Wid=GetTextSize(Font,T("Timing method")).Width;
  CreateWindow("Static",T("Timing method"),WS_CHILD | DisableIfMute,
                  page_l+10,y+4,Wid,23,Handle,(HMENU)7053,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP |
                    DisableIfMute | CBS_DROPDOWNLIST,
                    page_l+15+Wid,y,page_w-10-(15+Wid),200,Handle,(HMENU)7103,HInstance,NULL),

  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Play Cursor"));
#if defined(SSE_SOUND_RECOMMEND_OPTIONS)
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Write Cursor (recommended)"));
#else
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Write Cursor"));
#endif
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Milliseconds"));
  SendMessage(Win,CB_SETCURSEL,sound_time_method,0);
  y+=30;
ADVANCED_END
#endif//#if !defined(SSE_SOUND_ENFORCE_RECOM_OPT)

  Wid=GetTextSize(Font,T("Delay")).Width;
  CreateWindow("Static",T("Delay"),WS_CHILD | DisableIfMute,
                  page_l+10,y+4,Wid,23,Handle,(HMENU)7054,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | DisableIfMute | WS_VSCROLL | CBS_DROPDOWNLIST,
                    page_l+15+Wid,y,page_w-10-(15+Wid),300,Handle,(HMENU)7104,HInstance,NULL);
  EasyStr Ms=T("Milliseconds");
  for (int i=0;i<=300;i+=20) CBAddString(Win,Str(i)+" "+Ms);
  SendMessage(Win,CB_SETCURSEL,psg_write_n_screens_ahead,0);
  y+=30;
  y+=5;

#if defined(SSE_GUI_OPTIONS_DRIVE_SOUND)
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX  ;
  EasyStr path=RunDir+SLASH+DRIVE_SOUND_DIRECTORY; // we suppose the sounds are in it!
  if(!Exists(path.Text))
  {
    OPTION_DRIVE_SOUND=0;
    mask|=WS_DISABLED;
  }
  Wid=GetCheckBoxSize(Font,T("Drive sound")).Width;
  Win=CreateWindow("Button",T("Drive sound"),mask,
    page_l+HorizontalSeparation,y,Wid,25,Handle,(HMENU)7310,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_DRIVE_SOUND,0);
  ToolAddWindow(ToolTip,Win,
    T("Epson SMD-480L sound sampled by Stefan jL, thx dude!"));
  mask&=~BS_CHECKBOX;
  Win=CreateWindow(TRACKBAR_CLASS,"",mask | TBS_HORZ,
     page_l+15+Wid,y,150-50,20,Handle,(HMENU)7311,HInstance,NULL);
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(0,100));
  db=SF314[0].Sound_Volume;
  position= pow(10, log10((float)101)*(db + 10000)/10000 )-1 ;
  SendMessage(Win,TBM_SETPOS,1,position);
  SendMessage(Win,TBM_SETLINESIZE,0,1);
  SendMessage(Win,TBM_SETPAGESIZE,0,10);
  y+=LineHeight;
#endif

#if defined(SSE_GUI_OPTIONS_KEYBOARD_CLICK) && defined(SSE_SOUND_CAN_CHANGE_DRIVER)
  y-=LineHeight; // maybe it will be optimised away!
  Offset+=Wid;
  Wid=GetCheckBoxSize(Font,T("Keyboard click")).Width;
  Win=CreateWindow("Button",T("Keyboard click"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l+Offset,y,Wid,25,Handle,(HMENU)7301,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_KEYBOARD_CLICK,0);
  y+=LineHeight;
#endif  

NOT_ADVANCED_BEGIN
  y+=30;
ADVANCED_END

  CreateWindow("Button",T("Record"),WS_CHILD | BS_GROUPBOX | DisableIfMute,
                  page_l,y,page_w,80,Handle,(HMENU)7200,HInstance,NULL);
  y+=20;

  Win=CreateWindow("Steem Flat PicButton",Str(RC_ICO_RECORD),WS_CHILD | DisableIfMute,
                page_l+10,y,25,25,Handle,(HMENU)7201,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,sound_record,0);

  if (WAVOutputFile.Empty()) WAVOutputFile=WriteDir+"\\ST.wav";
  CreateWindowEx(512,"Steem Path Display",WAVOutputFile,
                  WS_CHILD | DisableIfMute,
                  page_l+40,y,page_w-10-75-40,25,Handle,(HMENU)7202,HInstance,NULL);

  CreateWindow("Button",T("Choose"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX |
                          BS_PUSHLIKE | DisableIfMute,
                          page_l+page_w-10-70,y,70,23,Handle,(HMENU)7203,HInstance,NULL);
  y+=30;

#if defined(SSE_YM2149_RECORD_YM)
/*  Add record to YM functionality using the same GUI elements as for WAV.
    We add a combobox to select format rather than radio buttons, this way
    we can add more formats.
*/
  Wid=GetTextSize(Font,T("Format")).Width;
  CreateWindow("Static",T("Format"),WS_CHILD,page_l+10,y+4,Wid,23,Handle,
    (HMENU)7053,HInstance,NULL);
  Offset=page_l+15+Wid;
  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
    Offset,y,60,200,Handle,(HMENU)7105,HInstance,NULL);
  Offset+=80;
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Wav"));
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("YM"));
  SendMessage(Win,CB_SETCURSEL,OPTION_SOUND_RECORD_FORMAT,0);
#endif

  Wid=GetCheckBoxSize(Font,T("Warn before overwrite")).Width;
#if defined(SSE_YM2149_RECORD_YM)
  Win=CreateWindow("Button",T("Warn before overwrite"),WS_CHILD | WS_TABSTOP |
                          BS_CHECKBOX | DisableIfMute,
                          Offset,y,Wid,25,Handle,(HMENU)7204,HInstance,NULL);
#else
  Win=CreateWindow("Button",T("Warn before overwrite"),WS_CHILD | WS_TABSTOP |
                          BS_CHECKBOX | DisableIfMute,
                          page_l+10,y,Wid,25,Handle,(HMENU)7204,HInstance,NULL);
#endif
  SendMessage(Win,BM_SETCHECK,RecordWarnOverwrite,0);
  y+=30;
  y+=5;
#if !defined(SOUND_DISABLE_INTERNAL_SPEAKER)
  Wid=GetCheckBoxSize(Font,T("Internal speaker sound")).Width;
  Win=CreateWindow("Button",T("Internal speaker sound"),WS_CHILD | WS_TABSTOP |
                          BS_CHECKBOX | DisableIfNT,
                          page_l,y,Wid,25,Handle,(HMENU)7300,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,sound_internal_speaker,0);
#endif
  if (Focus==NULL) Focus=GetDlgItem(Handle,7099);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
void TOptionBox::CreateStartupPage()
{
  ConfigStoreFile CSF(INIFile);
#if defined(SSE_VS2008_WARNING_390) 
  bool NoDD=(CSF.GetInt("Options","NoDirectDraw",0)!=0);
#else
  bool NoDD=(bool)CSF.GetInt("Options","NoDirectDraw",0);
#endif

  int y=10,Wid;
  HWND Win;
//ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Restore previous state")).Width;
  Win=CreateWindow("Button",T("Restore previous state"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)3303,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,AutoLoadSnapShot,0);
#ifdef SSE_GUI_393
  ToolAddWindow(ToolTip,Win,
    T("When this is checked, Steem saves the state when leaving and loads it\
 when starting. Without a hiccup."));

#endif
  y+=30;

  Wid=get_text_width(T("Filename"));
  CreateWindow("Static",T("Filename"),WS_CHILD,
                            page_l,y+4,Wid,25,Handle,(HMENU)3310,HInstance,NULL);

  Win=CreateWindowEx(512,"Edit",AutoSnapShotName,WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                      page_l+Wid+5,y,page_w-(Wid+5),23,Handle,(HMENU)3311,HInstance,NULL);
  SendMessage(Win,WM_SETFONT,(UINT)Font,0);
  SendMessage(Win,EM_LIMITTEXT,100,0);
  int Len=SendMessage(Win,WM_GETTEXTLENGTH,0,0);
  SendMessage(Win,EM_SETSEL,Len,Len);
  SendMessage(Win,EM_SCROLLCARET,0,0);
  y+=30;
//ADVANCED_END

  Wid=GetCheckBoxSize(Font,T("Start in fullscreen mode")).Width;
  Win=CreateWindow("Button",T("Start in fullscreen mode"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | int(NoDD ? WS_DISABLED:0),
                          page_l,y,Wid,23,Handle,(HMENU)3302,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,GetCSFInt("Options","StartFullscreen",0,INIFile),0);
  y+=30;

ADVANCED_BEGIN
  Wid=GetCheckBoxSize(Font,T("Draw direct to video memory")).Width;
  Win=CreateWindow("Button",T("Draw direct to video memory"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | int(NoDD ? WS_DISABLED:0),
                          page_l,y,Wid,23,Handle,(HMENU)3304,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,GetCSFInt("Options","DrawToVidMem",Disp.DrawToVidMem,INIFile),0);
  ToolAddWindow(ToolTip,Win,T("Drawing direct to video memory is generally very fast but in some situations on some PCs it might cause Steem to slow down a lot.")+" "+
                    T("If you having problems with speed try turning this option off and restarting Steem."));
  y+=30;

  Wid=GetCheckBoxSize(Font,T("Hide mouse pointer when blit")).Width;
  Win=CreateWindow("Button",T("Hide mouse pointer when blit"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | int(NoDD ? WS_DISABLED:0),
                          page_l,y,Wid,23,Handle,(HMENU)3305,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,GetCSFInt("Options","BlitHideMouse",Disp.BlitHideMouse,INIFile),0);
  ToolAddWindow(ToolTip,Win,T("On some video cards, it makes a mess if the mouse pointer is over the area where the card is trying to draw.")+" "+
                    T("This option, when checked, makes Steem hide the mouse before it draws to the screen.")+" "+
                    T("Unfortunately this can make the mouse pointer flicker when Steem is running."));
  y+=30;
#if defined(SSE_VID_D3D)
  Wid=GetCheckBoxSize(Font,T("Never use Direct3D")).Width;
  Win=CreateWindow("Button",T("Never use Direct3D"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)3300,HInstance,NULL);
#else
  Wid=GetCheckBoxSize(Font,T("Never use DirectDraw")).Width;
  Win=CreateWindow("Button",T("Never use DirectDraw"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)3300,HInstance,NULL);
#endif
  SendMessage(Win,BM_SETCHECK,NoDD,0);
  y+=30;
#if !defined(SSE_SOUND_NO_NOSOUND_OPTION)
  Win=CreateWindow("Button",T("Never use DirectSound"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                          page_l,y,GetCheckBoxSize(Font,T("Never use DirectSound")).Width,20,
                          Handle,(HMENU)3301,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,CSF.GetInt("Options","NoDirectSound",0),0);
  y+=30;
#endif
#if defined(SSE_SOUND_OPTION_DISABLE_DSP)
  Wid=GetCheckBoxSize(Font,T("Disable DSP")).Width;
  Win=CreateWindow("Button",T("Disable DSP"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                          page_l,y,Wid,23,Handle,(HMENU)3306,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,DSP_DISABLED,0);
  ToolAddWindow(ToolTip,Win,T("If you have some odd crashes, checking this may help. DSP code uses the math coprocessor and exceptions are almost impossible to catch"));
  y+=30;
#endif
ADVANCED_END

#if !defined(SSE_SOUND_CAN_CHANGE_DRIVER) // moved to Sound
  Wid=get_text_width(T("Sound driver"));
  CreateWindow("Static",T("Sound driver"),WS_CHILD,
                          page_l,y+4,Wid,20,Handle,(HMENU)3000,HInstance,NULL);

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
                    page_l+5+Wid,y,page_w-(5+Wid),200,Handle,(HMENU)3001,HInstance,NULL);
  SendMessage(Win,CB_ADDSTRING,0,(LPARAM)CStrT("Default"));
  for (int i=0;i<DSDriverModuleList.NumStrings;i++){
    SendMessage(Win,CB_ADDSTRING,0,(LPARAM)DSDriverModuleList[i].String);
  }
  SendMessage(Win,CB_SETCURSEL,0,0);
  EasyStr DSDriverModName=CSF.GetStr("Options","DSDriverName","");
  if (DSDriverModName.NotEmpty()){
    for (int i=0;i<DSDriverModuleList.NumStrings;i++){
      if (IsSameStr_I(DSDriverModuleList[i].String,DSDriverModName)){
        SendMessage(Win,CB_SETCURSEL,1+i,0);
        break;
      }
    }
  }
#endif//#if !defined(SSE_SOUND_CAN_CHANGE_DRIVER)
  CSF.Close();

  if (Focus==NULL) Focus=GetDlgItem(Handle,3303);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
#if !(defined(SSE_VAR_NO_UPDATE))
void TOptionBox::CreateUpdatePage()
{
  int Wid;

  ConfigStoreFile CSF(INIFile);
  DWORD Disable=DWORD(Exists(RunDir+"\\SteemUpdate.exe") ? 0:WS_DISABLED);
  int Runs=CSF.GetInt("Update","Runs",0),
      Offline=CSF.GetInt("Update","Offline",0),
      WSError=CSF.GetInt("Update","WSError",0),
      y=10;

  EasyStr Info=EasyStr(" ");
  Info+=T("Update has checked for a new Steem")+" "+Runs+" "+time_or_times(Runs)+"\n ";
  Info+=T("It thought you were off-line")+" "+Offline+" "+time_or_times(Offline)+"\n ";
  Info+=T("It encountered an error")+" "+WSError+" "+time_or_times(WSError)+"\n ";
  CreateWindowEx(512,"Static",Info,WS_CHILD | Disable,
                  page_l,y,page_w,80,Handle,(HMENU)4100,HInstance,NULL);
  y+=90;


  Wid=GetCheckBoxSize(Font,T("Disable automatic update checking")).Width;
  HWND ChildWin=CreateWindow("Button",T("Disable automatic update checking"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | Disable,
                          page_l,y,Wid,20,Handle,(HMENU)4200,HInstance,NULL);
  SendMessage(ChildWin,BM_SETCHECK,!CSF.GetInt("Update","AutoUpdateEnabled",true),0);
  y+=30;

  Wid=GetCheckBoxSize(Font,T("This computer is never off-line")).Width;
  ChildWin=CreateWindow("Button",T("This computer is never off-line"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | Disable,
                          page_l,y,Wid,20,Handle,(HMENU)4201,HInstance,NULL);
  SendMessage(ChildWin,BM_SETCHECK,CSF.GetInt("Update","AlwaysOnline",0),0);
  y+=30;

  Wid=GetCheckBoxSize(Font,T("Download new patches")).Width;
  ChildWin=CreateWindow("Button",T("Download new patches"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | Disable,
                          page_l,y,Wid,20,Handle,(HMENU)4202,HInstance,NULL);
  SendMessage(ChildWin,BM_SETCHECK,CSF.GetInt("Update","PatchDownload",1),0);
  y+=30;

  Wid=GetCheckBoxSize(Font,T("Ask before installing new patches")).Width;
  ChildWin=CreateWindow("Button",T("Ask before installing new patches"),WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX | Disable,
                          page_l,y,Wid,20,Handle,(HMENU)4203,HInstance,NULL);
  SendMessage(ChildWin,BM_SETCHECK,CSF.GetInt("Update","AskPatchInstall",0),0);
  y+=30;

  HANDLE UpdateMutex=OpenMutex(MUTEX_ALL_ACCESS,0,"SteemUpdate_Running");
  if (UpdateMutex){
    CloseHandle(UpdateMutex);
    Disable=WS_DISABLED;
  }else if (UpdateWin || FullScreen){
    Disable=WS_DISABLED;
  }
  CreateWindow("Button",T("Check For Update Now"),WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | int(UpdateWin || FullScreen ? WS_DISABLED:0) | Disable,
                    page_l,y,page_w,23,Handle,(HMENU)4400,HInstance,NULL);

  CSF.Close();

  if (Focus==NULL) Focus=GetDlgItem(Handle,4200);
  SetPageControlsFont();
  ShowPageControls();
}
#endif
//---------------------------------------------------------------------------
void TOptionBox::AssAddToExtensionsLV(char *Ext,char *Desc,int Num)
{
  EasyStr Text=Str(Ext)+" ("+Desc+")";
  int y=5 + 30*Num;
  int ButWid=max(GetTextSize(Font,T("Associated")).Width,GetTextSize(Font,T("Associate")).Width)+16;
  int hoff=12-GetTextSize(Font,Text).Height/2;
  HWND But=CreateWindow("Button","",WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                        5,y,ButWid,23,Scroller.GetControlPage(),HMENU(5100+Num),HInstance,NULL);
  HWND Stat=CreateWindow("Steem HyperLink",Text,WS_CHILD | HL_STATIC | HL_WINDOWBK,ButWid+10,y+hoff,300,25,
                       Scroller.GetControlPage(),(HMENU)5000,HInstance,NULL);
  SendMessage(Stat,WM_SETFONT,WPARAM(Font),0);
  SendMessage(But,WM_SETFONT,WPARAM(Font),0);
  if (IsSteemAssociated(Ext)){
    SendMessage(But,WM_SETTEXT,0,LPARAM(T("Associated").Text));
#if !defined(SSE_GUI_MAY_REMOVE_ASSOCIATION)
    EnableWindow(But,0);
#endif
  }else{
    SendMessage(But,WM_SETTEXT,0,LPARAM(T("Associate").Text));
  }
  ShowWindow(Stat,SW_SHOW);
  ShowWindow(But,SW_SHOW);
}
//---------------------------------------------------------------------------
void TOptionBox::CreateAssocPage()
{
  HWND Win;
  Scroller.CreateEx(512,WS_CHILD | WS_VSCROLL | WS_HSCROLL,
    page_l,10,page_w,OPTIONS_HEIGHT-10-10-25-10,
                          Handle,5500,HInstance);
  Scroller.SetBkColour(GetSysColor(COLOR_WINDOW));
#ifdef SSE_DISK_EXT
  AssAddToExtensionsLV(dot_ext(EXT_ST),T("Disk Image"),0);
  AssAddToExtensionsLV(dot_ext(EXT_STT),T("Disk Image"),1);
  AssAddToExtensionsLV(dot_ext(EXT_MSA),T("Disk Image"),2);
#if USE_PASTI
  if (hPasti) AssAddToExtensionsLV(dot_ext(EXT_STX),T("Pasti Disk Image"),3);
#endif
  AssAddToExtensionsLV(dot_ext(EXT_DIM),T("Disk Image"),4);
  AssAddToExtensionsLV(".STZ",T("Zipped Disk Image"),5);
  AssAddToExtensionsLV(".STS",T("Memory Snapshot"),6);
#if !(defined(SSE_GUI_NO_ASSOCIATE_STC))
  AssAddToExtensionsLV(".STC",T("Cartridge Image"),7); //too rare
#else
#if defined(SSE_GUI_ASSOCIATE_HFE) 
  AssAddToExtensionsLV(dot_ext(EXT_HFE),T("ST/HxC Disk Image"),7); //height
#endif
#if defined(SSE_TOS_PRG_AUTORUN)
  AssAddToExtensionsLV(dot_ext(EXT_PRG),T("Atari PRG executable"),8);
  AssAddToExtensionsLV(dot_ext(EXT_TOS),T("Atari TOS executable"),9);
#endif
#endif


#else
  AssAddToExtensionsLV(".ST",T("Disk Image"),0);
  AssAddToExtensionsLV(".STT",T("Disk Image"),1);
  AssAddToExtensionsLV(".MSA",T("Disk Image"),2);
#if USE_PASTI
  if (hPasti) AssAddToExtensionsLV(".STX",T("Pasti Disk Image"),3);
#endif
  AssAddToExtensionsLV(".DIM",T("Disk Image"),4);
  AssAddToExtensionsLV(".STZ",T("Zipped Disk Image"),5);
  AssAddToExtensionsLV(".STS",T("Memory Snapshot"),6);
  AssAddToExtensionsLV(".STC",T("Cartridge Image"),7); //too rare
#endif//ext
  Scroller.AutoSize(5,5);

  int Wid=GetCheckBoxSize(Font,T("Always open files in new window")).Width;
  Win=CreateWindow("Button",T("Always open files in new window"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l,OPTIONS_HEIGHT-35,Wid,25,Handle,(HMENU)5502,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,GetCSFInt("Options","OpenFilesInNew",true,INIFile),0);

  if (Focus==NULL) Focus=GetDlgItem(Handle,5502);
  SetPageControlsFont();
  ShowPageControls();
}
//---------------------------------------------------------------------------
#if !defined(SSE_GUI_NO_MACROS)
void TOptionBox::CreateMacrosPage()
{
  int y=10,x,Wid;
  int ctrl_h=10+20+30+30+10;
  HWND Win;

  DTree.FileMasksESL.DeleteAll();
  DTree.FileMasksESL.Add("",0,RC_ICO_PCFOLDER);
  DTree.FileMasksESL.Add("stmac",0,RC_ICO_OPS_MACROS);
  UpdateDirectoryTreeIcons(&DTree);
  DTree.Create(Handle,page_l,y,page_w,OPTIONS_HEIGHT-ctrl_h-10-30,10000,WS_TABSTOP,
                DTreeNotifyProc,this,MacroDir,T("Macros"));
  y+=OPTIONS_HEIGHT-ctrl_h-30;

  CreateWindow("Button",T("New Macro"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l,y,page_w/2-5,23,Handle,(HMENU)10001,HInstance,NULL);

  CreateWindow("Button",T("Change Store Folder"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+page_w/2+5,y,page_w/2-5,23,Handle,(HMENU)10002,HInstance,NULL);


  y=10+OPTIONS_HEIGHT-ctrl_h;
  CreateWindow("Button",T("Controls"),WS_CHILD | BS_GROUPBOX,
                  page_l,y,page_w,ctrl_h-10-10,Handle,(HMENU)10010,HInstance,NULL);
  y+=20;

  x=page_l+10;
  CreateWindow("Steem Flat PicButton",Str(RC_ICO_RECORD),WS_CHILD | WS_TABSTOP,
                x,y,25,25,Handle,(HMENU)10011,HInstance,NULL);
  x+=30;

  CreateWindow("Steem Flat PicButton",Str(RC_ICO_PLAY_BIG),WS_CHILD | WS_TABSTOP,
                x,y,25,25,Handle,(HMENU)10012,HInstance,NULL);
  x+=30;
#if !defined(SSE_GUI_RECORDINPUT_C1)
ADVANCED_BEGIN
  Wid=get_text_width(T("Mouse speed"));
  CreateWindow("Static",T("Mouse speed"),WS_CHILD,x,y+4,Wid,23,Handle,(HMENU)10013,HInstance,NULL);
  x+=Wid+5;

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                          x,y,page_l+page_w-10-x,400,Handle,(HMENU)10014,HInstance,NULL);
  CBAddString(Win,T("Safe"),15);
  CBAddString(Win,T("Slow"),32);
  CBAddString(Win,T("Medium"),64);
  CBAddString(Win,T("Fast"),96);
  CBAddString(Win,T("V.Fast"),127);
  CBSelectItemWithData(Win,127);
  y+=30;

  x=page_l+10;
  Wid=get_text_width(T("Playback event delay"));
  CreateWindow("Static",T("Playback event delay"),WS_CHILD,x,y+4,Wid,23,Handle,(HMENU)10015,HInstance,NULL);
  x+=Wid+5;

  Win=CreateWindow("Combobox","",WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                          x,y,page_l+page_w-10-x,400,Handle,(HMENU)10016,HInstance,NULL);
  // Number of VBLs that input is allowed to be the same
  CBAddString(Win,T("As Recorded"),0);
  EasyStr Ms=Str(" ")+T("Milliseconds");
  for (int n=1;n<=25;n++) CBAddString(Win,Str(n*20)+Ms,n);
#if defined(SSE_IKBD_6301_MACRO)
  CBSelectItemWithData(Win,0); // 'As Recorded' works best with 'C1'
#else
  CBSelectItemWithData(Win,1);
#endif
  ADVANCED_END
#endif//#if !defined(SSE_GUI_RECORDINPUT_C1)
  DTree.SelectItemByPath(MacroSel);

  if (Focus==NULL) Focus=GetDlgItem(Handle,10000);
  SetPageControlsFont();
  ShowPageControls();
}
#endif
//---------------------------------------------------------------------------
#if !defined(SSE_GUI_NO_PROFILES)
void TOptionBox::CreateProfilesPage()
{
  int y=10;
  HWND Win;
  int ctrl_h=OPTIONS_HEIGHT/2-30;

  DTree.FileMasksESL.DeleteAll();
  DTree.FileMasksESL.Add("",0,RC_ICO_PCFOLDER);
  DTree.FileMasksESL.Add("ini",0,RC_ICO_OPS_PROFILES);
  UpdateDirectoryTreeIcons(&DTree);
  DTree.Create(Handle,page_l,y,page_w,OPTIONS_HEIGHT-ctrl_h-40,11000,WS_TABSTOP,
                DTreeNotifyProc,this,ProfileDir,T("Profiles"));
  y+=OPTIONS_HEIGHT-ctrl_h-30;

  CreateWindow("Button",T("Save New Profile"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l,y,page_w/2-5,23,Handle,(HMENU)11001,HInstance,NULL);

  CreateWindow("Button",T("Change Store Folder"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+page_w/2+5,y,page_w/2-5,23,Handle,(HMENU)11002,HInstance,NULL);


  y=10+OPTIONS_HEIGHT-ctrl_h;
  CreateWindow("Button",T("Controls"),WS_CHILD | BS_GROUPBOX,
                  page_l,y,page_w,ctrl_h-10-10,Handle,(HMENU)11010,HInstance,NULL);
  y+=20;

  CreateWindow("Button",T("Load Profile"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10,y,(page_w-20)/2-5,23,Handle,(HMENU)11011,HInstance,NULL);

  CreateWindow("Button",T("Save Over Profile"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+10+(page_w-20)/2+5,y,(page_w-20)/2-5,23,Handle,(HMENU)11012,HInstance,NULL);
  y+=30;

  Win=CreateWindowEx(512,WC_LISTVIEW,"",WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED |
                      LVS_SINGLESEL | LVS_REPORT | LVS_NOCOLUMNHEADER,
                      page_l+10,y,page_w-20,OPTIONS_HEIGHT-y-15,Handle,(HMENU)11013,HInstance,NULL);
  ListView_SetExtendedListViewStyle(Win,LVS_EX_CHECKBOXES);

  RECT rc;
  GetClientRect(Win,&rc);

  LV_COLUMN lvc;
  lvc.mask=LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
  lvc.fmt=LVCFMT_LEFT;
  lvc.cx=rc.right-GetSystemMetrics(SM_CXVSCROLL);
  lvc.pszText="";
  lvc.iSubItem=0;
  SendMessage(Win,LVM_INSERTCOLUMN,0,LPARAM(&lvc));

  LV_ITEM lvi;
  lvi.mask=LVIF_TEXT | LVIF_PARAM;
  int i=0;
  for(;;){
    if (ProfileSection[i].Name==NULL) break;
    lvi.iSubItem=0;
    lvi.pszText=StaticT(ProfileSection[i].Name);
    lvi.lParam=DWORD(ProfileSection[i].ID);
    lvi.iItem=i++;
    SendMessage(Win,LVM_INSERTITEM,0,(LPARAM)&lvi);
  }

  DTree.SelectItemByPath(ProfileSel);

  if (Focus==NULL) Focus=GetDlgItem(Handle,11000);
  SetPageControlsFont();
  ShowPageControls();
}
#endif
//---------------------------------------------------------------------------
void TOptionBox::IconsAddToScroller()
{
  for (int n=14100;n<14100+RC_NUM_ICONS;n++){
    if (GetDlgItem(Scroller.GetControlPage(),n)) DestroyWindow(GetDlgItem(Scroller.GetControlPage(),n));
  }

  int x=3,y=3;
  for (int want_size=16;want_size;want_size<<=1){
    for (int n=1;n<RC_NUM_ICONS;n++){
      int size=RCGetSizeOfIcon(n) & ~1;
      switch (n){
        case RC_ICO_HARDDRIVES:
        case RC_ICO_HARDDRIVES_FR:
          int want_ico=RC_ICO_HARDDRIVES;
          if (IsSameStr_I(T("File"),"Fichier")) want_ico=RC_ICO_HARDDRIVES_FR;
          if (n!=want_ico) size=0;
          break;
      }
      if (size==want_size){
        CreateWindow("Steem Flat PicButton",Str(n),WS_CHILD | PBS_RIGHTCLICK,
             x,y,size+4,size+4,Scroller.GetControlPage(),HMENU(14100+n),HInstance,NULL);
        x+=size+4+3;
      }
      if (x+want_size+4+3 >= page_w-GetSystemMetrics(SM_CXVSCROLL) || n==RC_NUM_ICONS-1){
        x=3;
        y+=want_size+4+3;
      }
    }
  }
  for (int n=14100;n<14100+RC_NUM_ICONS;n++){
    if (GetDlgItem(Scroller.GetControlPage(),n)) ShowWindow(GetDlgItem(Scroller.GetControlPage(),n),SW_SHOWNA);
  }
  Scroller.AutoSize(0,5);
}
//---------------------------------------------------------------------------
#if !defined(SSE_GUI_NO_ICONCHOICE)
void TOptionBox::CreateIconsPage()
{
  int th=GetTextSize(Font,T("Left click to change")).Height;
  int y=10,scroller_h=OPTIONS_HEIGHT-10-th-2-10-25-10;

  CreateWindow("Static",T("Left click to change, right to reset"),WS_CHILD,page_l,y,page_w,th,Handle,(HMENU)14002,HInstance,NULL);
  y+=th+2;

  Scroller.CreateEx(512,WS_CHILD | WS_VSCROLL | WS_HSCROLL,page_l,y,
                      page_w,scroller_h,Handle,14010,HInstance);
  Scroller.SetBkColour(GetSysColor(COLOR_BTNFACE));
  IconsAddToScroller();
  y+=scroller_h+10;

  CreateWindow("Button",T("Load Icon Scheme"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l,y,page_w/2-5,23,Handle,(HMENU)14020,HInstance,NULL);

  CreateWindow("Button",T("All Icons To Default"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                  page_l+page_w/2+5,y,page_w/2-5,23,Handle,(HMENU)14021,HInstance,NULL);


  if (Focus==NULL) Focus=GetDlgItem(Handle,11000);
  SetPageControlsFont();
  ShowPageControls();
}
#endif
//---------------------------------------------------------------------------

#if defined(SSE_GUI_OPTION_PAGE)

void TOptionBox::CreateSSEPage() {
  HWND Win;
  long Wid,Offset=0;
  int y=10; // top
  const int LineHeight=30;
  const int HorizontalSeparation=10;
  int mask;
  EasyStr tip_text;

  // Title
  Wid=get_text_width("Steem SSE Extra Options\n==============================")/2;
  CreateWindow("Static","Steem SSE Extra Options\n==============================",WS_CHILD,
    page_l,y,Wid,21,Handle,(HMENU)209,HInstance,NULL);

#if defined(SSE_GUI_OPTION_FOR_TESTS)
  Offset=Wid+HorizontalSeparation;
  Wid=GetCheckBoxSize(Font,T("Beta tests")).Width;
  Win=CreateWindow("Button",T("Beta tests"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l+Offset,y,Wid,25,Handle,(HMENU)7316,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,SSEOption.TestingNewFeatures,0);
#else
  ASSERT(!SSEOption.TestingNewFeatures);
#endif
  y+=LineHeight+10;
  Wid=0;

#if defined(SSE_GUI_ADVANCED)
  Wid=GetCheckBoxSize(Font,T("Advanced Settings")).Width;
  Win=CreateWindow("Button",T("Advanced Settings"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l,y,Wid,25,Handle,(HMENU)1038,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_ADVANCED,0);
  ToolAddWindow(ToolTip,Win,T("For those who know what they're doing and don't report fake bugs!"));

  Win=CreateWindow("Button",T("Reset Advanced Settings"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX | BS_PUSHLIKE,
                          page_l+Wid+20,y,130,23,Handle,(HMENU)1039,HInstance,NULL);
  ToolAddWindow(ToolTip,Win,T("Restore all advanced settings to the 'non-advanced' default")); 
  y+=LineHeight;
#endif


#if defined(SSE_GUI_STATUS_BAR) && !defined(SSE_GUI_STATUS_BAR_NOT_OPTIONAL)
  Wid=GetCheckBoxSize(Font,T("Status info")).Width;
  Win=CreateWindow("Button",T("Status info"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l,y,Wid,25,Handle,(HMENU)7307,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_STATUS_BAR,0);
  ToolAddWindow(ToolTip,Win,T("Displays some info in the tool bar."));
#if defined(SSE_GUI_STATUS_BAR_DISK_NAME_OPTION)
  Offset=Wid+HorizontalSeparation;
  Wid=GetCheckBoxSize(Font,T("Disk name")).Width;
  Win=CreateWindow("Button",T("Disk name"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l+Offset,y,Wid,25,Handle,(HMENU)7309,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_STATUS_BAR_GAME_NAME,0);
  ToolAddWindow(ToolTip,Win,T("Also the name of the current disk."));
#endif
  Offset+=Wid+HorizontalSeparation;
#endif


#if defined(SSE_GUI_STATUS_BAR_DISK_NAME_OPTION) && defined(SSE_GUI_STATUS_BAR_NOT_OPTIONAL)
  Wid=GetCheckBoxSize(Font,T("Disk name in status bar")).Width;
  Win=CreateWindow("Button",T("Disk name in status bar"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l+Offset,y,Wid,25,Handle,(HMENU)7309,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_STATUS_BAR_GAME_NAME,0);
  ToolAddWindow(ToolTip,Win,T("Toggles disk name of A: / disk image types of A: and B:"));
  Offset=Wid+HorizontalSeparation;
#endif


ADVANCED_BEGIN

#if defined(SSE_HACKS) && !defined(SSE_HACKS_NO_OPTION)
  Wid=GetCheckBoxSize(Font,T("Hacks")).Width;
  Win=CreateWindow("Button",T("Hacks"),WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l+Offset,y,Wid,23,Handle,(HMENU)1027,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_HACKS,0);
  ToolAddWindow(ToolTip,Win,T("For an edgier emulation, recommended!"));
  Offset+=Wid+HorizontalSeparation;
#endif

#if defined(SSE_VAR_EMU_DETECT) && !defined(SSE_VAR_NO_EMU_DETECT)
//  Offset+=Wid+HorizontalSeparation;
  Wid=GetCheckBoxSize(Font,T("Emu detect")).Width;
  Win=CreateWindow("Button",T("Emu detect"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l+Offset,y,Wid,25,Handle,(HMENU)1031,HInstance,NULL);
  //SendMessage(Win,BM_SETCHECK,!STEALTH_MODE,0);
  SendMessage(Win,BM_SETCHECK,OPTION_EMU_DETECT,0);
  ToolAddWindow(ToolTip,Win,T("Enable easy detection of Steem by ST programs."));
#endif  

ADVANCED_END

  Offset=0;
  y+=LineHeight;

#if defined(SSE_VID_VSYNC_WINDOW) || defined(SSE_VID_3BUFFER)
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
#endif

ADVANCED_BEGIN

#if defined(SSE_IKBD_6301) && !defined(SSE_IKBD_6301_NOT_OPTIONAL)
  Wid=GetCheckBoxSize(Font,T("C1: 6850/6301/E-Clock")).Width;
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
  if(!HD6301_OK)
  {
    OPTION_C1=0;
    mask|=WS_DISABLED;
  }
  Win=CreateWindow("Button",T("C1: 6850/6301/E-Clock"),mask,page_l,y,Wid,23,Handle,
    (HMENU)1029,HInstance,NULL);
  if(!HD6301_OK)
    SendMessage(Win,BN_DISABLE,0,0);
  else
    SendMessage(Win,BM_SETCHECK,OPTION_C1,0);
  ToolAddWindow(ToolTip,Win,
  T("Chipset 1 - This enables a low level emulation of the IKBD keyboard chip (using\
 the Sim6xxx code by Arne Riiber, thx dude!), precise E-Clock and ACIA timings.\
 Note: important for MIDI emulation too."));
  y+=LineHeight;
#endif

#if defined(SSE_INT_MFP_OPTION) && !defined(SSE_C2_NOT_OPTIONAL)
#if defined(SSE_IKBD_6301) 
  y-=LineHeight; // maybe it will be optimised away!
#endif
  Offset=Wid+HorizontalSeparation;
#if defined(SSE_YM2149_MAMELIKE) && !defined(SSE_YM2149_MAMELIKE_394)
  Wid=GetCheckBoxSize(Font,T("C2: 68901/YM2149")).Width;
  Win=CreateWindow("Button",T("C2: 68901/YM2149"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l+Offset,y,Wid,25,Handle,(HMENU)7323,HInstance,NULL);
#else
  Wid=GetCheckBoxSize(Font,T("C2: 68901")).Width;
  Win=CreateWindow("Button",T("C2: 68901"),WS_CHILD | WS_TABSTOP |
    BS_CHECKBOX,page_l+Offset,y,Wid,25,Handle,(HMENU)7323,HInstance,NULL);
#endif
  SendMessage(Win,BM_SETCHECK,OPTION_C2,0);
  ToolAddWindow(ToolTip,Win,
#if defined(SSE_YM2149_MAMELIKE) && !defined(SSE_YM2149_MAMELIKE_394)
    T("Chipset 2 - Check for a more precise emulation of the MFP and a lower level emulation of the PSG."));
#else
    T("Chipset 2 - Check for a more precise emulation of the MFP."));
#endif
  y+=LineHeight;
#endif

  Offset=0;

ADVANCED_END

#if defined(SSE_GUI_MOUSE_CAPTURE_OPTION)  
  Wid=GetCheckBoxSize(Font,T("Capture mouse")).Width;
  Win=CreateWindow("Button",T("Capture mouse"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l+Offset,y,Wid,25,Handle,(HMENU)1028,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_CAPTURE_MOUSE,0);
  ToolAddWindow(ToolTip,Win,T("If unchecked, Steem will leave mouse control to Windows until you click in the window."));
  y+=LineHeight;
#endif

#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
ADVANCED_BEGIN
  y-=LineHeight;
  Offset+=Wid+HorizontalSeparation;
  Wid=GetCheckBoxSize(Font,T("VM-friendly mouse")).Width;
  Win=CreateWindow("Button",T("VM-friendly mouse"),
                          WS_CHILD | WS_TABSTOP | BS_CHECKBOX,
                          page_l+Offset,y,Wid,25,Handle,(HMENU)1035,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,SSEOption.VMMouse,0);
  ToolAddWindow(ToolTip,Win,T("Alternative mouse handling - cursor not bound"));
  y+=LineHeight;
ADVANCED_END
#endif

  Offset=0;
  y+=LineHeight;

#if defined(SSE_VID_SDL)
  Wid=GetCheckBoxSize(Font,T("Use SDL")).Width;
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
  if(!SDL_OK)
    mask|=WS_DISABLED;
  Win=CreateWindow("Button",T("Use SDL"),mask,
    page_l,y,Wid,25,Handle,(HMENU)7304,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,USE_SDL,0);
  ToolAddWindow(ToolTip,Win,
    T("BETA TESTS - BUGGY"));
  y+=LineHeight;
#endif

ADVANCED_BEGIN

#if defined(SSE_CPU_MFP_RATIO_OPTION) // user can fine tune CPU clock
  Wid=GetCheckBoxSize(Font,T("Fine tune CPU clock")).Width;
  mask=WS_CHILD | WS_TABSTOP | BS_CHECKBOX;
  Win=CreateWindow("Button",T("Fine tune CPU clock"),mask,
    page_l+Offset,y,Wid,25,Handle,(HMENU)7322,HInstance,NULL);
  SendMessage(Win,BM_SETCHECK,OPTION_CPU_CLOCK,0);
  ToolAddWindow(ToolTip,Win,
    T("This parameter changes the CPU/MFP clock ratio"));

  CreateWindow("Static","",WS_CHILD | SS_CENTER,page_l+115,y+5,100,20,Handle,(HMENU)7321,HInstance,NULL);
  y+=20;
#define ST_CYCLES_TO_CONTROL(c) ((c-8000000)/10)
  Win=CreateWindow(TRACKBAR_CLASS,"",WS_CHILD | WS_TABSTOP | TBS_HORZ,
                    page_l,y,page_w,18,Handle,(HMENU)7320,HInstance,NULL);
  SendMessage(Win,TBM_SETRANGE,0,MAKELPARAM(ST_CYCLES_TO_CONTROL(8000000),ST_CYCLES_TO_CONTROL(8030000)));
  SendMessage(Win,TBM_SETLINESIZE,0,1);
#if defined(SSE_STF_MEGASTF_CLOCK)
  SendMessage(Win,TBM_SETTIC,0,ST_CYCLES_TO_CONTROL(CPU_CLOCK_STF_MEGA));
#endif
  SendMessage(Win,TBM_SETTIC,0,ST_CYCLES_TO_CONTROL(CPU_CLOCK_STE_PAL)); //we know it should be the same...
  SendMessage(Win,TBM_SETTIC,0,ST_CYCLES_TO_CONTROL(CPU_CLOCK_STF_PAL));
  SendMessage(Win,TBM_SETPOS,1,ST_CYCLES_TO_CONTROL(CpuCustomHz));
  SendMessage(Win,TBM_SETPAGESIZE,0,1);
  SendMessage(Handle,WM_HSCROLL,0,LPARAM(Win));
  y+=30;
#endif

ADVANCED_END

  if(y<300)//?
    y=300;

#if defined(SSE_GUI_OPTIONS_SSE_ICON_VERSION) // fancy
  y+=LineHeight+LineHeight;
  Offset=85;
  Win=CreateWindow("Static",NULL,WS_CHILD | WS_VISIBLE | SS_ICON ,
                          page_l+Offset,y,0,0,Handle,(HMENU)209,HInstance,NULL);
  SendMessage (Win,STM_SETICON,(WPARAM)hGUIIcon[RC_ICO_APP],0);
  Offset+=32;
  Wid=get_text_width(WINDOW_TITLE);
  Win=CreateWindow("Static",WINDOW_TITLE,WS_CHILD | WS_VISIBLE ,
                          page_l+Offset,y,Wid,21,Handle,(HMENU)209,HInstance,NULL);
#endif

  SetPageControlsFont();
  ShowPageControls();
}

#if defined(SSE_GUI_OPTIONS_REFRESH)
/* Update SSE options according to snapshot
*/

void TOptionBox::SSEUpdateIfVisible() {
  if (Handle==NULL) 
    return;
  HWND Win;
#if defined(SSE_IKBD_6301) && !defined(SSE_IKBD_6301_NOT_OPTIONAL)
  Win=GetDlgItem(Handle,1029); //HD6301 emu
  if(Win!=NULL) 
  {
    if(!HD6301_OK)
      SendMessage(Win,BN_DISABLE,0,0);
    else
      SendMessage(Win,BM_SETCHECK,OPTION_C1,0);
  }
#endif
  Win=GetDlgItem(Handle,7301); //kkb click
  if(Win!=NULL) 
    SendMessage(Win,BM_SETCHECK,PEEK(0x484)&1,0);
#if defined(SSE_VID_BORDERS_GUARD_EM)
  Win=GetDlgItem(Handle,1026);
  if((extended_monitor
#if defined(SSE_VID_BORDERS_GUARD_R2)
    || screen_res==2 || NewMonitorSel>0
#else
    || NewMonitorSel>1
#endif
    )&&Win)
    EnableWindow(Win,FALSE);
#endif

#if defined(SSE_GUI_MOUSE_VM_FRIENDLY)
  Win=GetDlgItem(Handle,1035); 
  if(Win!=NULL) 
    SendMessage(Win,BM_SETCHECK,SSEOption.VMMouse,0);
#endif

#if defined(SSE_GUI_STATUS_BAR)
  GUIRefreshStatusBar();//overkill
#endif

  InvalidateRect(Handle,NULL,true);

}
#endif //#if defined(SSE_GUI_OPTIONS_REFRESH)

#endif #if defined(SSE_GUI_OPTION_PAGE)

