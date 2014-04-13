#pragma once
#ifndef DISPLAY_DECLA_H
#define DISPLAY_DECLA_H

#define EXT extern
#define INIT(s)

#if defined(SS_STRUCTURE_SSESHIFTER_OBJ)
#include <easystr.h>//unix case-sensitive
typedef EasyStr Str;
#endif


#if defined(STEVEN_SEAGAL) && defined(SS_VID_SAVE_NEO)
//http://wiki.multimedia.cx/index.php?title=Neochrome
#include "../../3rdparty/various/neochrome.h"
#endif

extern void draw_init_resdependent();

#define DISPMETHOD_NONE 0
#define DISPMETHOD_DD 1
#define DISPMETHOD_GDI 2
#define DISPMETHOD_X 3
#define DISPMETHOD_XSHM 4
#define DISPMETHOD_BE 5

extern "C"
{
EXT int BytesPerPixel INIT(2),rgb32_bluestart_bit INIT(0);
EXT bool rgb555 INIT(0);
EXT int monitor_width,monitor_height; //true size of monitor, for LAPTOP mode.
}

#define MONO_HZ 72 /*71.47*/

#define NUM_HZ 6
#define DISP_MAX_FREQ_LEEWAY 5

EXT int HzIdxToHz[NUM_HZ];

//---------------------------------------------------------------------------
class SteemDisplay
{
private:
#ifdef WIN32
  // DD Only
  HRESULT InitDD();
  static HRESULT WINAPI DDEnumModesCallback(LPDDSURFACEDESC,LPVOID);
  HRESULT DDCreateSurfaces();
  void DDDestroySurfaces();
  HRESULT DDError(char *,HRESULT);

  // GDI Only
  bool InitGDI();

  IDirectDraw2 *DDObj;
  IDirectDrawSurface *DDPrimarySur,*DDBackSur;

#if defined(STEVEN_SEAGAL) && defined(SS_VID_3BUFFER_WIN)
  IDirectDrawSurface *DDBackSur2; // our second back buffer
  bool SurfaceToggle;
#endif

  IDirectDrawClipper *DDClipper;
  DDSURFACEDESC DDBackSurDesc;
  DWORD DDLockFlags;
  bool DDBackSurIsAttached,DDExclusive;
  bool DDDisplayModePossible[3][2];
  int DDClosestHz[3][2][NUM_HZ];

  HBITMAP GDIBmp;
  HDC GDIBmpDC;
  BYTE *GDIBmpMem;
  int GDIBmpLineLength;
  DWORD GDIBmpSize;
#elif defined(UNIX)
	bool CheckDisplayMode(DWORD,DWORD,DWORD);

  bool AlreadyWarnedOfBadMode;

  bool InitX();

  XImage *X_Img;

  bool InitXSHM();

#ifndef NO_SHM
  XShmSegmentInfo XSHM_Info;
  bool XSHM_Attached;
#endif

#ifndef NO_XVIDMODE
  int XVM_nModes,XVM_ViewX,XVM_ViewY;
  XF86VidModeModeInfo **XVM_Modes;
  static int XVM_WinProc(void*,Window,XEvent*);
#endif

#endif

public:
  SteemDisplay();
  ~SteemDisplay();

  void SetMethods(int,...);
  HRESULT Init();
  HRESULT Lock();
  void VSync();
  bool Blit();
  void WaitForAsyncBlitToFinish();
  void Unlock();
#ifdef SHOW_WAVEFORM
  void DrawWaveform();
#endif
  void RunStart(bool=0),RunEnd(bool=0);
  void ScreenChange();
  void ChangeToFullScreen(),ChangeToWindowedMode(bool=0);
  void DrawFullScreenLetterbox(),FlipToDialogsScreen();
  bool CanGoToFullScreen();
  HRESULT SetDisplayMode(int,int,int,int=0,int* = NULL);
  HRESULT RestoreSurfaces();
  void Release();
  HRESULT SaveScreenShot();
  bool BorderPossible();

  int Method,UseMethods[5],nUseMethod;
  bool RunOnChangeToWindow;
  int SurfaceWidth,SurfaceHeight;
  Str ScreenShotNextFile;
  int ScreenShotFormat;
  int ScreenShotMinSize;
  bool ScreenShotUseFullName,ScreenShotAlwaysAddNum;
  bool DoAsyncBlit;

#ifdef WIN32
  void ScreenShotCheckFreeImageLoad();
#ifdef IN_MAIN
  bool ScreenShotIsFreeImageAvailable();
  void ScreenShotGetFormats(EasyStringList*);
  void ScreenShotGetFormatOpts(EasyStringList*);
#endif

  HINSTANCE hFreeImage;
  int ScreenShotFormatOpts;
  Str ScreenShotExt;
#if defined(STEVEN_SEAGAL)
#if defined(SS_VID_SAVE_NEO)
  neochrome_file *pNeoFile;
#endif
#if defined(SS_VID_3BUFFER_WIN)
  long VSyncTiming; // must be public
  BOOL BlitIfVBlank(); // our polling function
#endif
#endif

  bool DrawToVidMem,BlitHideMouse;
  
  DWORD ChangeToWinTimeOut;
  bool DrawLetterboxWithGDI;
#endif

#if defined(UNIX)
	void Surround();

#ifndef NO_SHM
	int SHMCompletion;
	bool asynchronous_blit_in_progress;
#endif

  Window XVM_FullWin;
  bool GoToFullscreenOnRun;
  int XVM_FullW,XVM_FullH;

#endif

};


EXT SteemDisplay Disp;


WIN_ONLY( EXT bool TryDD; )
#ifdef NO_SHM
UNIX_ONLY( EXT bool TrySHM; )
#else
UNIX_ONLY( EXT bool TrySHM; )
#endif

#define IF_TOCLIPBOARD 0xfff0

#ifdef WIN32
//#include "SteemFreeImage.h"

// declaration part of SteemFreeImage.h

#define FI_ENUM(x)      enum x
#define FI_STRUCT(x)	struct x

FI_STRUCT (FIBITMAP) { void *data; };

FI_ENUM(FREE_IMAGE_FORMAT) {
	FIF_UNKNOWN = -1,
	FIF_BMP = 0,
	FIF_ICO,
	FIF_JPEG,
	FIF_JNG,
	FIF_KOALA,
	FIF_LBM,
	FIF_MNG,
	FIF_PBM,
	FIF_PBMRAW,
	FIF_PCD,
	FIF_PCX,
	FIF_PGM,
	FIF_PGMRAW,
	FIF_PNG,
	FIF_PPM,
	FIF_PPMRAW,
	FIF_RAS,
	FIF_TARGA,
	FIF_TIFF,
	FIF_WBMP,
	FIF_PSD,
	FIF_CUT,
	FIF_IFF = FIF_LBM,
#if defined(STEVEN_SEAGAL) && defined(SS_VID_SAVE_NEO)
  IF_NEO,
#endif
};

#define BMP_DEFAULT         0
#define BMP_SAVE_RLE        1
#define CUT_DEFAULT         0
#define ICO_DEFAULT         0
#define ICO_FIRST           0
#define ICO_SECOND          0
#define ICO_THIRD           0
#define IFF_DEFAULT         0
#define JPEG_DEFAULT        0
#define JPEG_FAST           1
#define JPEG_ACCURATE       2
#define JPEG_QUALITYSUPERB  0x80
#define JPEG_QUALITYGOOD    0x100
#define JPEG_QUALITYNORMAL  0x200
#define JPEG_QUALITYAVERAGE 0x400
#define JPEG_QUALITYBAD     0x800
#define KOALA_DEFAULT       0
#define LBM_DEFAULT         0
#define MNG_DEFAULT         0
#define PCD_DEFAULT         0
#define PCD_BASE            1
#define PCD_BASEDIV4        2
#define PCD_BASEDIV16       3
#define PCX_DEFAULT         0
#define PNG_DEFAULT         0
#define PNG_IGNOREGAMMA		  1		// avoid gamma correction
#define PNM_DEFAULT         0
#define PNM_SAVE_RAW        0       // If set the writer saves in RAW format (i.e. P4, P5 or P6)
#define PNM_SAVE_ASCII      1       // If set the writer saves in ASCII format (i.e. P1, P2 or P3)
#define RAS_DEFAULT         0
#define TARGA_DEFAULT       0
#define TARGA_LOAD_RGB888   1       // If set the loader converts RGB555 and ARGB8888 -> RGB888.
#define TARGA_LOAD_RGB555   2       // This flag is obsolete
#define TIFF_DEFAULT        0
#define WBMP_DEFAULT        0
#define PSD_DEFAULT         0

typedef void (__stdcall *FI_INITPROC)(BOOL);
typedef void (__stdcall *FI_DEINITPROC)();
typedef FIBITMAP* (__stdcall *FI_CONVFROMRAWPROC)(BYTE*,int,int,int,UINT,UINT,UINT,UINT,BOOL);
typedef BOOL (__stdcall *FI_SAVEPROC)(FREE_IMAGE_FORMAT,FIBITMAP*,const char *,int);
typedef void (__stdcall *FI_FREEPROC)(FIBITMAP*);
typedef BOOL (__stdcall *FI_SUPPORTBPPPROC)(FREE_IMAGE_FORMAT,int);



SET_GUID(CLSID_DirectDraw,			0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
SET_GUID(IID_IDirectDraw,			0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
SET_GUID(IID_IDirectDraw2,                  0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );

#endif



#undef EXT
#undef INIT

#endif//DISPLAY_DECLA_H
