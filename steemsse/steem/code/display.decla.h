#pragma once
#ifndef DISPLAY_DECLA_H
#define DISPLAY_DECLA_H

#define EXT extern
#define INIT(s)

#if defined(SSE_VID_D3D)

#ifdef BCC_BUILD
// yes sir, the old BCC5.5 will build the new DX9 option too
#pragma comment(lib, "../../3rdparty/d3d/bcc/d3d9.lib")
#pragma comment(lib, "../../3rdparty/d3d/bcc/d3dx9_43.lib")
#pragma message DD v DIRECTDRAW_VERSION D3D v DIRECT3D_VERSION
#define D3D_DISABLE_9EX
#define sqrtf (sqrt)  // so that it compiles, hopefully not used...
#pragma message("BCC including d3d9 files...")
#endif


#if _MSC_VER == 1200 // VC6 -  also for this dinosaur
#define D3D_DISABLE_9EX
#pragma comment(lib, "../../3rdparty/d3d/d3d9.lib")
#pragma comment(lib, "../../3rdparty/d3d/d3dx9.lib")
#endif

#if _MSC_VER == 1200 || defined(MINGW_BUILD) //mingw TODO?
#include "d3d\sal.h"
#include "d3d\d3d9.h"
#include "d3d\d3dx9core.h"
#include "d3d\D3d9types.h"
#include "d3d\D3dx9math.h"
#elif defined(SSE_VS2015)
#include "d3d\d3d9.h"
#include "d3d\d3dx9core.h"
#include "d3d\D3d9types.h"
#include "d3d\D3dx9math.h"
#else //bcc too?
#include "d3d9.h"
#include "d3dx9core.h"
#include "D3d9types.h"
#include "D3dx9math.h"
#endif

#endif//d3d

#if defined(SSE_VID_DD7)
#if _MSC_VER == 1200
//#pragma comment(lib, "../../3rdparty/d3d/ddraw.lib")

//DEFINE_GUID( IID_IDirectDraw7,0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );

//http://forums.codeguru.com/showthread.php?305064-LNK2001-Unresolved-External-Symbol
//Try linking your program with dxguid.lib 
//#pragma comment(lib, "../../3rdparty/d3d/dxguid.lib")
//"Fatal error LNK1103: debugging information corrupt"
#endif
#endif


#include <easystr.h>//unix case-sensitive
typedef EasyStr Str;

#if defined(SSE_VID_SAVE_NEO)
//http://wiki.multimedia.cx/index.php?title=Neochrome
#include "../../3rdparty/various/neochrome.h"
#endif

extern void draw_init_resdependent();

#define DISPMETHOD_NONE 0
#define DISPMETHOD_DD 1 //SS also D3D
#define DISPMETHOD_GDI 2
#define DISPMETHOD_X 3
#define DISPMETHOD_XSHM 4
#define DISPMETHOD_BE 5

extern "C"
{
#if defined(SSE_VAR_RESIZE)
EXT BYTE BytesPerPixel INIT(2),rgb32_bluestart_bit INIT(0);
EXT bool rgb555 INIT(0);
EXT WORD monitor_width,monitor_height; //true size of monitor, for LAPTOP mode.
#else
EXT int BytesPerPixel INIT(2),rgb32_bluestart_bit INIT(0);
EXT bool rgb555 INIT(0);
EXT int monitor_width,monitor_height; //true size of monitor, for LAPTOP mode.
#endif
}

#define MONO_HZ 72 /*71.47*/

#define NUM_HZ 6
#define DISP_MAX_FREQ_LEEWAY 5

#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VAR_RESIZE)
EXT BYTE HzIdxToHz[NUM_HZ];
#else
EXT int HzIdxToHz[NUM_HZ];
#endif
#endif
//---------------------------------------------------------------------------

#pragma pack(push, STRUCTURE_ALIGNMENT)//391

class SteemDisplay
{
private:
#ifdef WIN32
  // DD Only
#if !defined(SSE_VID_D3D_ONLY)
  HRESULT InitDD();
#if defined(SSE_VID_DD7)
  static HRESULT WINAPI DDEnumModesCallback(LPDDSURFACEDESC2,LPVOID);
#else
  static HRESULT WINAPI DDEnumModesCallback(LPDDSURFACEDESC,LPVOID);
#endif
  HRESULT DDCreateSurfaces();
  void DDDestroySurfaces();
  HRESULT DDError(char *,HRESULT);
#endif//#if !defined(SSE_VID_D3D_ONLY)

#if defined(SSE_VID_UTIL)
  int STXPixels();
  int STYPixels();
#endif

#if defined(SSE_VID_D3D)
public:
  HRESULT D3DInit(); 
  HRESULT D3DCreateSurfaces();
  HRESULT D3DSpriteInit();
  void D3DDestroySurfaces();
  VOID D3DRelease();
  HRESULT D3DLock();
  void D3DUnlock();
  bool D3DBlit();
  friend HRESULT check_device_type(D3DDEVTYPE DeviceType,D3DFORMAT DisplayFormat);
  LPDIRECT3D9 pD3D;
  LPDIRECT3DDEVICE9  pD3DDevice;
  IDirect3DTexture9* pD3DTexture;
  ID3DXSprite* pD3DSprite;
#if defined(SSE_VID_D3D_LIST_MODES)
  UINT D3DMode; // depends on video card and format
#endif
#if defined(SSE_VID_D3D_382)
  UINT D3DFsW,D3DFsH;
  void D3DUpdateWH(UINT mode);
#if !defined(SSE_VID_D3D_390)
  void Cls();
#endif
#endif
private:
#endif//d3d

  // GDI Only
  bool InitGDI();

#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_DD7)
  IDirectDraw7 *DDObj;
  IDirectDrawSurface7 *DDPrimarySur,*DDBackSur;
  DDSURFACEDESC2 DDBackSurDesc;
#else
  IDirectDraw2 *DDObj;
  IDirectDrawSurface *DDPrimarySur,*DDBackSur;
  DDSURFACEDESC DDBackSurDesc;
#endif

#if defined(SSE_VID_3BUFFER_WIN)
#if defined(SSE_VID_DD7)
  IDirectDrawSurface7 *DDBackSur2; // our second back buffer
#else
  IDirectDrawSurface *DDBackSur2; // our second back buffer
#endif
  bool SurfaceToggle;
#endif

  IDirectDrawClipper *DDClipper;
  
  DWORD DDLockFlags;
  bool DDBackSurIsAttached,DDExclusive;
  bool DDDisplayModePossible[3][2];
  int DDClosestHz[3][2][NUM_HZ];
#endif//#if !defined(SSE_VID_D3D_ONLY)
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
#if defined(SSE_VID_D3D_ONLY) && defined(SSE_VS2008_WARNING_390)
  void RunStart(bool=0),RunEnd();
#else
  void RunStart(bool=0),RunEnd(bool=0);
#endif
  void ScreenChange();
  void ChangeToFullScreen(),ChangeToWindowedMode(bool=0);
  void DrawFullScreenLetterbox(),FlipToDialogsScreen();
  bool CanGoToFullScreen();
#if defined(SSE_VID_D3D_ONLY) && defined(SSE_VS2008_WARNING_390)
  HRESULT SetDisplayMode();
#else
  HRESULT SetDisplayMode(int,int,int,int=0,int* = NULL);
#endif
  HRESULT RestoreSurfaces();
  void Release();
  HRESULT SaveScreenShot();
  bool BorderPossible();
  int Method,UseMethods[5],nUseMethod;
  bool RunOnChangeToWindow;
#if defined(SSE_VAR_RESIZE)
  WORD SurfaceWidth,SurfaceHeight;
#else
  int SurfaceWidth,SurfaceHeight;
#endif
  Str ScreenShotNextFile;
  int ScreenShotFormat;
  int ScreenShotMinSize;
  bool ScreenShotUseFullName,ScreenShotAlwaysAddNum;
  bool DoAsyncBlit;

#ifdef WIN32
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  void ScreenShotCheckFreeImageLoad();
#endif
#ifdef IN_MAIN
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  bool ScreenShotIsFreeImageAvailable();
#endif
  void ScreenShotGetFormats(EasyStringList*);
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  void ScreenShotGetFormatOpts(EasyStringList*);
  HINSTANCE hFreeImage;
#endif
#endif
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
#if defined(SSE_VAR_RESIZE) && !defined(SSE_VAR_RESIZE_391)
// argh! caused options ignored + crash on taking JPG screenshot before changing format
  BYTE ScreenShotFormatOpts; 
#else
  int ScreenShotFormatOpts;
#endif
#endif
  Str ScreenShotExt;

#if defined(SSE_VID_SAVE_NEO)
  neochrome_file *pNeoFile;
#endif
#if defined(SSE_VID_3BUFFER_WIN)
  long VSyncTiming; // must be public
  BOOL BlitIfVBlank(); // our polling function
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

#pragma pack(pop)//391

EXT SteemDisplay Disp;


WIN_ONLY( EXT bool TryDD; )
#ifdef NO_SHM
UNIX_ONLY( EXT bool TrySHM; )
#else
UNIX_ONLY( EXT bool TrySHM; )
#endif

#define IF_TOCLIPBOARD 0xfff0

#ifdef WIN32
#if defined(SSE_VID_D3D_NO_FREEIMAGE)
enum {FIF_BMP=D3DXIFF_BMP,FIF_JPEG=D3DXIFF_JPG,FIF_PNG=D3DXIFF_PNG,IF_NEO}; //0 1 3
#elif defined(SSE_VID_FREEIMAGE4) //3.6.4 use official header

#include <FreeImage/FreeImage.h>

#if defined(SSE_VID_SAVE_NEO)
enum {IF_NEO=FIF_LBM+1};
#endif

typedef void (__stdcall *FI_FREEPROC)(FIBITMAP*);

#else

//#include "SteemFreeImage.h"

// declaration part of SteemFreeImage.h
//SS this is a part of a .h we should include instead
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
#if defined(SSE_VID_SAVE_NEO)
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


#if !defined(MINGW_BUILD)
SET_GUID(CLSID_DirectDraw,			0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
SET_GUID(IID_IDirectDraw,			0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
SET_GUID(IID_IDirectDraw2,                  0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
#endif

#endif
#endif//#if defined(SSE_VID_FREEIMAGE4)


#undef EXT
#undef INIT

#endif//DISPLAY_DECLA_H
