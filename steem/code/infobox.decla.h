#define INFOPAGE_LINK_ID_BASE 200

#if defined(SSE_GUI_INFOBOX)

enum {
 INFOPAGE_ABOUT,
 INFOPAGE_DRAWSPEED,
 INFOPAGE_LINKS,
 INFOPAGE_README,
 INFOPAGE_UNIXREADME,
 INFOPAGE_HOWTO_DISK,
 INFOPAGE_HOWTO_CART,
 INFOPAGE_FAQ,
 INFOPAGE_README_SSE,
 INFOPAGE_HINTS,
 NUM_INFOPAGE
};

#else 

#define INFOPAGE_ABOUT 0
#define INFOPAGE_DRAWSPEED 1
#define INFOPAGE_LINKS 2
#define INFOPAGE_README 3
#define INFOPAGE_UNIXREADME 4
#define INFOPAGE_HOWTO_DISK 5
#define INFOPAGE_HOWTO_CART 6
#define INFOPAGE_FAQ 7
#define NUM_INFOPAGE 8

#endif



#define HL_STATIC 1
#define HL_UNDERLINE 2
#define HL_WINDOWBK 4
//---------------------------------------------------------------------------

#if defined(SSE_COMPILER_STRUCT_391)

#pragma pack(push, STRUCTURE_ALIGNMENT)

class TGeneralInfo : public TStemDialog
{
public: //TODO
  void GetHyperlinkLists(EasyStringList &,EasyStringList &);
#ifdef WIN32
  static WINDOWPROC OldEditWndProc;
  HBRUSH BkBrush;
  HIMAGELIST il;
#if defined(SSE_GUI_INFOBOX_80COL)
  HFONT hFontCourier;
#endif
  EasyStr SearchText;
  int page_l,page_w;
  int MaxLinkID;
  int Page;
  ScrollControlWin Scroller;
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall HyperLinkWndProc(HWND,UINT,WPARAM,LPARAM);
  void DestroyCurrentPage();
  void ManageWindowClasses(bool);
#elif defined(UNIX)
  int page_l,page_w;
  int Page;
  static int WinProc(TGeneralInfo*,Window,XEvent*);
  static int button_notifyproc(hxc_button*,int,int*);
  static int listview_notify_proc(hxc_listview*,int,int);
  static int edit_notify_proc(hxc_edit*,int,int);
	void ShowTheReadme(char*,bool=false);
	hxc_button gb,thanks_label;
	hxc_textdisplay about,thanks;
	hxc_textdisplay readme;
	hxc_button steem_link,email_link;
	hxc_scrollarea sa;
	hxc_listview page_lv;
  int last_find_idx;
#endif
public:
  TGeneralInfo();
  ~TGeneralInfo() { Hide();WIN_ONLY( DeleteObject(BkBrush); ) }
  void Show(),Hide();
  bool ToggleVisible(){ IsVisible() ? Hide():Show();return IsVisible(); }
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);
  void CreatePage(int);
  void CreateSpeedPage(),CreateAboutPage(),CreateLinksPage(),CreateReadmePage(int);
#ifdef WIN32
  void HidePage(int),HideStatics(int,int);
  int DrawColumn(int,int,int,char*,...);
  EasyStr dp4_disp(int);
  void SetFonts(int,int);
  void LoadIcons();
#elif defined(UNIX)
#endif
};

#pragma pack(pop, STRUCTURE_ALIGNMENT)

#else

class TGeneralInfo : public TStemDialog
{
private:
  void GetHyperlinkLists(EasyStringList &,EasyStringList &);

  int page_l,page_w;

#ifdef WIN32
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  static LRESULT __stdcall HyperLinkWndProc(HWND,UINT,WPARAM,LPARAM);
  void DestroyCurrentPage();
  void ManageWindowClasses(bool);

  static WINDOWPROC OldEditWndProc;
  HBRUSH BkBrush;
  HIMAGELIST il;
  int MaxLinkID;

#if defined(SSE_GUI_INFOBOX_80COL)
  HFONT hFontCourier;
#endif

#elif defined(UNIX)
  static int WinProc(TGeneralInfo*,Window,XEvent*);
  static int button_notifyproc(hxc_button*,int,int*);
  static int listview_notify_proc(hxc_listview*,int,int);
  static int edit_notify_proc(hxc_edit*,int,int);

	void ShowTheReadme(char*,bool=false);

	hxc_button gb,thanks_label;
	hxc_textdisplay about,thanks;
	hxc_textdisplay readme;
	hxc_button steem_link,email_link;
	hxc_scrollarea sa;
	hxc_listview page_lv;

  int last_find_idx;
#endif
public:
  TGeneralInfo();
  ~TGeneralInfo() { Hide();WIN_ONLY( DeleteObject(BkBrush); ) }
  void Show(),Hide();
  bool ToggleVisible(){ IsVisible() ? Hide():Show();return IsVisible(); }
  bool LoadData(bool,GoodConfigStoreFile*,bool* = NULL),SaveData(bool,ConfigStoreFile*);

  int Page;
  void CreatePage(int);
  void CreateSpeedPage(),CreateAboutPage(),CreateLinksPage(),CreateReadmePage(int);
#ifdef WIN32
  ScrollControlWin Scroller;

  void HidePage(int),HideStatics(int,int);
  int DrawColumn(int,int,int,char*,...);
  EasyStr dp4_disp(int);
  void SetFonts(int,int);
  void LoadIcons();

  EasyStr SearchText;
#elif defined(UNIX)
#endif
};

#endif//#if defined(SSE_COMPILER_STRUCT_391)

WIN_ONLY( WINDOWPROC TGeneralInfo::OldEditWndProc; );

