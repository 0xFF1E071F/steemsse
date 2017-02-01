/*---------------------------------------------------------------------------
FILE: display.cpp
MODULE: Steem
DESCRIPTION: A class to encapsulate the process of outputting to the display.
This contains the DirectDraw code used by Windows Steem for output.
SSE: Direct3D code is here too
---------------------------------------------------------------------------*/

#if defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: display.cpp")
#endif

#ifdef SSE_DEBUG_D3D
char *GetTextFromD3DError(HRESULT);
#endif

#if defined(SSE_STRUCTURE_DECLA)

#define EXT
#define INIT(s) =s

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
#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VAR_RESIZE)
BYTE HzIdxToHz[NUM_HZ]={0,50,60,MONO_HZ,100,120};
#else
int HzIdxToHz[NUM_HZ]={0,50,60,MONO_HZ,100,120};
#endif
#endif
EXT SteemDisplay Disp;

WIN_ONLY( bool TryDD=true; )
#ifdef NO_SHM
UNIX_ONLY( bool TrySHM=false; )
#else
UNIX_ONLY( bool TrySHM=true; )
#endif

#ifdef WIN32
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
// definition part of SteemFreeImage.h

#if defined(SSE_VID_FREEIMAGE4)

FI_FREEPROC FreeImage_Free;

#else

FI_INITPROC FreeImage_Initialise;
FI_DEINITPROC FreeImage_DeInitialise;
FI_CONVFROMRAWPROC FreeImage_ConvertFromRawBits;
FI_SAVEPROC FreeImage_Save;
FI_FREEPROC FreeImage_Free;
FI_SUPPORTBPPPROC FreeImage_FIFSupportsExportBPP;
#endif
#endif//#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
#endif

#undef EXT
#undef INIT

#endif

#if defined(SSE_VID_DD7) 
#if _MSC_VER == 1200 //VC6
// we can't link to a more recent dxguid.lib because of some M$ bug
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUID( IID_IDirectDraw7,0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );
#endif
#endif


//SS: the singleton object is Disp
#if 1//defined(SSE_BOILER_390_LOG2)
#define LOGSECTION LOGSECTION_VIDEO_RENDERING
#else
#define LOGSECTION LOGSECTION_INIT
#endif

//---------------------------------------------------------------------------
#ifdef SSE_DEBUG_D3D //stolen somewhere
char *GetTextFromD3DError(HRESULT hr){  
  char *text="Undefined error";     
  switch (hr)  {    
  case D3D_OK:      text="D3D_OK";break;    
  case D3DOK_NOAUTOGEN:      text="D3DOK_NOAUTOGEN";break;    
  case D3DERR_CONFLICTINGRENDERSTATE:      text="D3DERR_CONFLICTINGRENDERSTATE";break;    
  case D3DERR_CONFLICTINGTEXTUREFILTER:      text="D3DERR_CONFLICTINGTEXTUREFILTER";break;    
  case D3DERR_CONFLICTINGTEXTUREPALETTE:      text="D3DERR_CONFLICTINGTEXTUREPALETTE";break;    
  case D3DERR_DEVICELOST:      text="D3DERR_DEVICELOST";break;    
  case D3DERR_DEVICENOTRESET:      text="D3DERR_DEVICENOTRESET";break;    
  case D3DERR_DRIVERINTERNALERROR:      text="D3DERR_DRIVERINTERNALERROR";break;    
  case D3DERR_INVALIDCALL:      text="D3DERR_INVALIDCALL";break;    
  case D3DERR_INVALIDDEVICE:      text="D3DERR_INVALIDDEVICE";break;    
  case D3DERR_MOREDATA:      text="D3DERR_MOREDATA";break;    
  case D3DERR_NOTAVAILABLE:      text="D3DERR_NOTAVAILABLE";break;    
  case D3DERR_NOTFOUND:      text="D3DERR_NOTFOUND";break;    
  case D3DERR_OUTOFVIDEOMEMORY:      text="D3DERR_OUTOFVIDEOMEMORY";break;    
  case D3DERR_TOOMANYOPERATIONS:      text="D3DERR_TOOMANYOPERATIONS";break;    
  case D3DERR_UNSUPPORTEDALPHAARG:      text="D3DERR_UNSUPPORTEDALPHAARG";break;    
  case D3DERR_UNSUPPORTEDALPHAOPERATION:      text="D3DERR_UNSUPPORTEDALPHAOPERATION";break;    
  case D3DERR_UNSUPPORTEDCOLORARG:      text="D3DERR_UNSUPPORTEDCOLORARG";break;    
  case D3DERR_UNSUPPORTEDCOLOROPERATION:      text="D3DERR_UNSUPPORTEDCOLOROPERATION";break;    
  case D3DERR_UNSUPPORTEDFACTORVALUE:      text="D3DERR_UNSUPPORTEDFACTORVALUE";break;    
  case D3DERR_UNSUPPORTEDTEXTUREFILTER:      text="D3DERR_UNSUPPORTEDTEXTUREFILTER";break;    
  case D3DERR_WRONGTEXTUREFORMAT:      text="D3DERR_WRONGTEXTUREFORMAT";break;    
  case E_FAIL:      text="E_FAIL";break;    
  case E_INVALIDARG:      text="E_INVALIDARG";break;    
  case E_OUTOFMEMORY:      text="E_OUTOFMEMORY";break;  
  }   
  return text;
}
#define REPORT_D3D_ERR(function,d3derr) TRACE(function" %s\n",GetTextFromD3DError(d3derr))
#else
#define REPORT_D3D_ERR 
#endif
//---------------------------------------------------------------------------
SteemDisplay::SteemDisplay()
{
#ifdef WIN32
#if !defined(SSE_VID_D3D_ONLY)
  DDObj=NULL;
  DDPrimarySur=NULL;
  DDBackSur=NULL;

#if defined(SSE_VID_3BUFFER_WIN)
  DDBackSur2=NULL;
#endif

  DDClipper=NULL;
  DDBackSurIsAttached=0;
  DDExclusive=0;
#endif//#if !defined(SSE_VID_D3D_ONLY)
  GDIBmp=NULL;
  GDIBmpMem=NULL;
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  hFreeImage=NULL;
  ScreenShotFormatOpts=0;
#endif
  ScreenShotExt="bmp";

  DrawToVidMem=true;
  BlitHideMouse=true;
  DrawLetterboxWithGDI=0;

#if defined(SSE_VID_D3D)
  pD3D       = NULL;	// Used to create the D3DDevice
  pD3DDevice = NULL;	// Our rendering device
  pD3DTexture=NULL;
  pD3DSprite=NULL;
#endif

#elif defined(UNIX)
  X_Img=NULL;
  AlreadyWarnedOfBadMode=0;
  GoToFullscreenOnRun=0;

#ifndef NO_SHM
  XSHM_Attached=0;
  XSHM_Info.shmaddr=(char*)-1;
  XSHM_Info.shmid=-1;
  SHMCompletion=LASTEvent;
  asynchronous_blit_in_progress=false;
#endif

#ifndef NO_XVIDMODE
  XVM_Modes=NULL;
#endif

#endif

  ScreenShotFormat=0;
  ScreenShotUseFullName=0;ScreenShotAlwaysAddNum=0;
  ScreenShotMinSize=0;
#if defined(SSE_VID_SAVE_NEO)
  pNeoFile=NULL;
#endif
  RunOnChangeToWindow=0;
  DoAsyncBlit=0;

  Method=DISPMETHOD_NONE;
  UseMethods[0]=DISPMETHOD_NONE;
  nUseMethod=0;
}
//---------------------------------------------------------------------------
SteemDisplay::~SteemDisplay() { Release(); }
bool SteemDisplay::BorderPossible() { return (GetScreenWidth()>640); }
//---------------------------------------------------------------------------
void SteemDisplay::SetMethods(int Method1,...)
{
  int *mp=&Method1;
  for (int n=0;n<5;n++){
#if defined(SSE_X64_390B)
    UseMethods[n]=mp[n*2];
#else
    UseMethods[n]=mp[n];
#endif
    if (UseMethods[n]==0) break;
  }
  nUseMethod=0;
}
//---------------------------------------------------------------------------
HRESULT SteemDisplay::Init()
{
  Release();
  if (FullScreen==0){
    monitor_width=GetScreenWidth();
    monitor_height=GetScreenHeight();
  }

  //TRACE_LOG("Display init\n");

#if defined(SSE_VID_D3D)
  if(!D3D9_OK)
  {
    if(D3DInit()==D3D_OK)
    {
      D3D9_OK=true;
#if defined(SSE_VID_D3D_ONLY)
      return (Method=UseMethods[nUseMethod++]);
#endif
    }
  }
#endif

  while (nUseMethod<5){
#ifdef WIN32
    if (UseMethods[nUseMethod]==DISPMETHOD_DD){
#if !defined(SSE_VID_D3D_ONLY)
      if (InitDD()==DD_OK) return (Method=UseMethods[nUseMethod++]);
#endif
    }else if (UseMethods[nUseMethod]==DISPMETHOD_GDI){
      TRACE_LOG("Using GDI!!!\n");
      if (InitGDI()) return (Method=UseMethods[nUseMethod++]);
    }
#elif defined(UNIX)
    if (UseMethods[nUseMethod]==DISPMETHOD_X){
      if (InitX()) return (Method=UseMethods[nUseMethod++]);
    }else if (UseMethods[nUseMethod]==DISPMETHOD_XSHM){
      if (InitXSHM()) return (Method=UseMethods[nUseMethod++]);
    }
#endif

    if (UseMethods[nUseMethod]==0){
      break;
    }
    nUseMethod++;
  }
  return 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef WIN32
//#undef LOGSECTION //SS
//#define LOGSECTION LOGSECTION_INIT
#if !defined(SSE_VID_D3D_ONLY)
HRESULT SteemDisplay::InitDD()
{
  SetNotifyInitText("DirectDraw");
  //TRACE_LOG("DD Init\n");

  HRESULT Ret;
  try{
    IDirectDraw *DDObj1=NULL;

    dbg_log("STARTUP: Initialising DirectDraw, creating DirectDraw object");
    Ret=CoCreateInstance(CLSID_DirectDraw,NULL,CLSCTX_ALL,IID_IDirectDraw,(void**)&DDObj1);
    if (Ret!=S_OK || DDObj1==NULL){
      EasyStr Err="Unknown error";
      switch (Ret){
        case REGDB_E_CLASSNOTREG:
          Err="The specified class is not registered in the registration database.";
          break;
        case E_OUTOFMEMORY:
          Err="Out of memory.";
          break;
        case E_INVALIDARG:
          Err="One or more arguments are invalid.";
          break;
        case E_UNEXPECTED:
          Err="An unexpected error occurred.";
          break;
        case CLASS_E_NOAGGREGATION:
          Err="This class cannot be created as part of an aggregate.";
          break;
      }
      Err=EasyStr("CoCreateInstance error\n\n")+Err;
      log_write("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
      log_write(Err);
      TRACE_LOG("%s\n",Err.Text); //bug (no .Text) found by MinGW
      log_write("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
#ifndef ONEGAME
      MessageBox(NULL,Err,T("Steem Engine DirectDraw Error"),
                    MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_TOPMOST);
#endif
      return ~DD_OK;
    }

    dbg_log("STARTUP: Initialising DirectDraw object");
    if ((Ret=DDObj1->Initialize(NULL)) != DD_OK){
      DDObj1->Release();
      return DDError("Initialise FAILED",Ret);
    }

    dbg_log("STARTUP: Calling QueryInterface");
#if defined(SSE_VID_DD7)
/*  Apparently using DirectDraw 7 instead of 2 doesn't change anything.
    This was cheap to do anyway.
    It is defined in the D3D build, that needs a very up-to-date computer
    anyway, DirectX 9, so why not?
*/
    if ((Ret=DDObj1->QueryInterface(IID_IDirectDraw7,(LPVOID*)&DDObj)) != DD_OK)
#else
    if ((Ret=DDObj1->QueryInterface(IID_IDirectDraw2,(LPVOID*)&DDObj)) != DD_OK)
#endif
        return DDError("QueryInterface FAILED",Ret);

    dbg_log("STARTUP: Calling SetCooperativeLevel");
    if ((Ret=DDObj->SetCooperativeLevel(StemWin,DDSCL_NORMAL)) != DD_OK)
        return DDError("SetCooperativeLevel FAILED",Ret);

    dbg_log("STARTUP: Creating the clipper");
    if ((Ret=DDObj->CreateClipper(0,&DDClipper,NULL))!=DD_OK)
        return DDError("CreateClipper FAILED",Ret);

    dbg_log("STARTUP: Associating clipper with main window");
    if ((Ret=DDClipper->SetHWnd(0,StemWin)) != DD_OK)
        return DDError("SetHWnd FAILED",Ret);

    dbg_log("STARTUP: Creating surfaces");
    if ((Ret=DDCreateSurfaces())!=DD_OK) return Ret;

    dbg_log("STARTUP: Performing lock test");
    DDLockFlags=DDLOCK_NOSYSLOCK;
#if defined(SSE_VID_DD7)
    DDSURFACEDESC2 ddsd;
#else
    DDSURFACEDESC ddsd;
#endif
    ddsd.dwSize=sizeof(DDSURFACEDESC);
#if defined(SSE_VID_D3D) && !defined(SSE_VID_D3D_ONLY)
    if(!SSE_OPTION_D3D)
#endif
    if (DDBackSur->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLockFlags,NULL)!=DD_OK){
      DDLockFlags=0;
      if ((Ret=DDBackSur->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLockFlags,NULL))!=DD_OK){
        return DDError("Lock test FAILED",Ret);
      }
    }
#if defined(SSE_VID_D3D) && !defined(SSE_VID_D3D_ONLY)
    if(!SSE_OPTION_D3D)
#endif
    DDBackSur->Unlock(NULL);

    dbg_log("STARTUP: Enumerating display modes");
    ZeroMemory(DDDisplayModePossible,sizeof(DDDisplayModePossible));
    ZeroMemory(DDClosestHz,sizeof(DDClosestHz));
    DDObj->EnumDisplayModes(DDEDM_REFRESHRATES,NULL,this,DDEnumModesCallback);
    for (int idx=0;idx<3;idx++){
      for (int hicol=0;hicol<2;hicol++){
        for (int n=1;n<NUM_HZ;n++){
          if (DDClosestHz[idx][hicol][n]==0) DDClosestHz[idx][hicol][n]=HzIdxToHz[n]; 
        }
      }
    }
#if defined(SSE_VID_DD7)
    TRACE_LOG("DD7 Init OK\n");
#else
    TRACE_LOG("DD2 Init OK\n");
#endif
    return DD_OK;
  }catch(...){
    TRACE_LOG("DirectDraw caused DISASTER!\n");
    return DDError("DirectDraw caused DISASTER!",DDERR_EXCEPTION);
  }
}
//---------------------------------------------------------------------------
#if defined(SSE_VID_DD7)
HRESULT WINAPI SteemDisplay::DDEnumModesCallback(LPDDSURFACEDESC2 ddsd,LPVOID t)
#else
HRESULT WINAPI SteemDisplay::DDEnumModesCallback(LPDDSURFACEDESC ddsd,LPVOID t)
#endif
{  
#if defined(SSE_VID_DD_FS_32BIT)
/*  Finally understood why DirectDraw fullscreen wouldn't work on some
    systems. It's not the resolution, it's bpp. All video card drivers
    won't support 16bit display. 
*/
  if(ddsd->ddpfPixelFormat.dwRGBBitCount==8)
    SSEConfig.VideoCard8bit=true;
  else if(ddsd->ddpfPixelFormat.dwRGBBitCount==16)
    SSEConfig.VideoCard16bit=true;
#else
  if (ddsd->ddpfPixelFormat.dwRGBBitCount>16) 
    return DDENUMRET_OK;//SS: it's 8bit or 16bit, not 32bit
#endif
  //SS: this is a static function, hence the need for This 
  SteemDisplay *This=(SteemDisplay*)t;//SS as passed

  int hicol=(ddsd->ddpfPixelFormat.dwRGBBitCount>8),idx=-1; //hicol=16bit

  //SS: Steem looks for 3 modes only, 640x480, 800x600, 640x400
  if (ddsd->dwWidth==640 && ddsd->dwHeight==480) idx=0;
  if (ddsd->dwWidth==800 && ddsd->dwHeight==600) idx=1;
  if (ddsd->dwWidth==640 && ddsd->dwHeight==400) idx=2;

  if (idx>=0){
    This->DDDisplayModePossible[idx][hicol]=true;
    TRACE_LOG("Adding idx %d hicol %d w %d h%d %dHz\n",
      idx,hicol,ddsd->dwWidth,ddsd->dwHeight,ddsd->dwRefreshRate);
    for (int n=1;n<NUM_HZ;n++){
      int diff=abs(HzIdxToHz[n]-int(ddsd->dwRefreshRate));
      int curdiff=abs(HzIdxToHz[n]-int(This->DDClosestHz[idx][hicol][n]));
      if (diff<curdiff && diff<=DISP_MAX_FREQ_LEEWAY){
        This->DDClosestHz[idx][hicol][n]=ddsd->dwRefreshRate;
        TRACE_LOG("Adding close Hz\n");
      }
    }
  }
  return DDENUMRET_OK;
}
//---------------------------------------------------------------------------
HRESULT SteemDisplay::DDCreateSurfaces()
{
  //SS this is a private function

#if defined(SSE_VID_DD7)
  DDSURFACEDESC2 ddsd;
#else
  DDSURFACEDESC ddsd;
#endif
  HRESULT Ret;

  DDDestroySurfaces();

#if defined(SSE_VID_D3D)
#if defined(SSE_VID_D3D_WINDOW)
  if(D3D9_OK && SSE_OPTION_D3D)
#else
  if(D3D9_OK && SSE_OPTION_D3D && FullScreen)
#endif
    return D3DCreateSurfaces();
#endif

  int ExtraFlags=0;
  if (DrawToVidMem==0) ExtraFlags=DDSCAPS_SYSTEMMEMORY; // Like malloc

  //SS those 2 lines were commented out by Steem authors
//  ExtraFlags=DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM; // AGP?
//  ExtraFlags=DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM; // Video card?
  for (int n=0;n<2;n++){
#if defined(SSE_VID_DD7)
    ZeroMemory(&ddsd,sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
#else
    ZeroMemory(&ddsd,sizeof(DDSURFACEDESC));
    ddsd.dwSize=sizeof(DDSURFACEDESC);
#endif
    ddsd.dwFlags=DDSD_CAPS;
    ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE | ExtraFlags;
    if (FullScreen){
      ddsd.ddsCaps.dwCaps|=DDSCAPS_FLIP | DDSCAPS_COMPLEX;
      ddsd.dwFlags|=DDSD_BACKBUFFERCOUNT;
      ddsd.dwBackBufferCount=1;
    }
#if defined(SSE_VID_CHECK_POINTERS)
    if(!DDObj)
      return Ret;
#endif
    if ((Ret=DDObj->CreateSurface(&ddsd,&DDPrimarySur,NULL))!=DD_OK){
      if (n==0){
        ExtraFlags=0;
      }else{
        // Another DirectX app is fullscreen so fail silently
        if (Ret==DDERR_NOEXCLUSIVEMODE) return Ret;
        // Otherwise make a big song and dance!
        return DDError("CreateSurface for PrimarySur FAILED",Ret);
      }
    }else{
      break;
    }
  }

#if defined(SSE_DEBUG)
#if defined(SSE_VID_DD7)
  DDSURFACEDESC2 PrimaryFeedback;
  PrimaryFeedback.dwSize=sizeof(DDSURFACEDESC2);
#else
  DDSURFACEDESC PrimaryFeedback;
  PrimaryFeedback.dwSize=sizeof(DDSURFACEDESC);
#endif
  if (DDPrimarySur->GetSurfaceDesc(&PrimaryFeedback)==DD_OK)
  {
    TRACE_LOG("FS %d BackBufferCount %d Refresh Rate %d\n",
      FullScreen,PrimaryFeedback.dwBackBufferCount,PrimaryFeedback.dwRefreshRate);    
  }
#endif

  if (FullScreen) DDBackSurIsAttached=true;

  if ((Ret=DDPrimarySur->SetClipper(DDClipper))!=DD_OK){
    return DDError("SetClipper FAILED",Ret);
  }

#if defined(SSE_VID_3BUFFER_FS)
/*  When option Triple Buffer is set, we use the same system for fullscreen
    as for window. For the moment stretch mode only.
*/
  if(!FullScreen || SSE_3BUFFER
    || (BORDER_40 && draw_fs_blit_mode==DFSM_STRAIGHTBLIT)) 
#elif defined(SSE_VID_BORDERS_LB_DX)
  // we may need a bigger drawing zone in fullscreen mode too
  if (!FullScreen || (BORDER_40 && draw_fs_blit_mode==DFSM_STRAIGHTBLIT) )
#else
  if (FullScreen==0)
#endif
    {
    if (DrawToVidMem==0) ExtraFlags=DDSCAPS_SYSTEMMEMORY; // Like malloc
    for (int n=0;n<2;n++){
#if defined(SSE_VID_DD7)
      ZeroMemory(&DDBackSurDesc,sizeof(DDSURFACEDESC2));
      DDBackSurDesc.dwSize=sizeof(DDSURFACEDESC2);
#else
      ZeroMemory(&DDBackSurDesc,sizeof(DDSURFACEDESC));
      DDBackSurDesc.dwSize=sizeof(DDSURFACEDESC);
#endif
      DDBackSurDesc.dwFlags=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
      DDBackSurDesc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN | ExtraFlags;
#ifndef NO_CRAZY_MONITOR
      if(extended_monitor){
        DDBackSurDesc.dwWidth=GetScreenWidth();
        DDBackSurDesc.dwHeight=GetScreenHeight();
      }else
#endif
      if (Disp.BorderPossible()){ //SS: GetScreenWidth()>640
#if defined(SSE_VID_BORDERS)
#if defined(SSE_VID_BORDERS_LB_DX)
        DDBackSurDesc.dwWidth=640+4* (SideBorderSize); // we draw larger
#else
        DDBackSurDesc.dwWidth=640+4* (SideBorderSizeWin);
#endif
        DDBackSurDesc.dwHeight=400+2*(BORDER_TOP+BottomBorderSize
#if defined(SSE_VID_ADJUST_DRAWING_ZONE1)
        +1 // hack to fix crash on video emu panic, last 'border' scanline
#endif
        );
#else
        DDBackSurDesc.dwWidth=768;
        DDBackSurDesc.dwHeight=400+2*(BORDER_TOP+BORDER_BOTTOM);
#endif
      }else{
        DDBackSurDesc.dwWidth=640;
        DDBackSurDesc.dwHeight=480;
      }
#if defined(SSE_VID_ADJUST_DRAWING_ZONE2)
      // because we can increase or decrease display size ?
      draw_blit_source_rect.right=int(DDBackSurDesc.dwWidth)-1;
      draw_blit_source_rect.bottom=int(DDBackSurDesc.dwHeight)-1;
#else
      if (draw_blit_source_rect.right>=int(DDBackSurDesc.dwWidth)){
        draw_blit_source_rect.right=int(DDBackSurDesc.dwWidth)-1;
      }
      if (draw_blit_source_rect.bottom>=int(DDBackSurDesc.dwHeight)){
        draw_blit_source_rect.bottom=int(DDBackSurDesc.dwHeight)-1;
      }
#endif
      if ((Ret=DDObj->CreateSurface(&DDBackSurDesc,&DDBackSur,NULL))!=DD_OK){
        if (n==0){
          ExtraFlags=0;
        }else{
          return DDError("CreateSurface for BackSur FAILED",Ret);
        }
      }else{
#if defined(SSE_VID_3BUFFER_WIN)
        // Let's create a second back surface for our "triple buffer"
        if(SSE_3BUFFER)
        {
          Ret=DDObj->CreateSurface(&DDBackSurDesc,&DDBackSur2,NULL);
          if(Ret!=DD_OK)
          {
            TRACE_LOG("Failed create DDBackSur2 DDERR %d\n",Ret);
            DDBackSur2=NULL; // prevents system from being used
            SSE_3BUFFER=false; // reset option
#if defined(SSE_GUI_OPTIONS_REFRESH)
            OptionBox.SSEUpdateIfVisible(); 
#endif
          }
          VSyncTiming=0;
          SurfaceToggle=true; // will be toggled false at first lock
          if(FullScreen)
            draw_fs_blit_mode=DFSM_STRETCHBLIT; // only working one
        }
#endif
        break;
      }
    }
  }else{  // SS Fullscreen
#if defined(SSE_VID_DD7)
    DDSCAPS2 caps;
    ZeroMemory(&caps,sizeof(DDSCAPS2));
#else
    DDSCAPS caps;
#endif
    caps.dwCaps=DDSCAPS_BACKBUFFER;
    if ((Ret=DDPrimarySur->GetAttachedSurface(&caps,&DDBackSur))!=DD_OK){
      return DDError("CreateSurface for BackSur FAILED",Ret);
    }
  }
#if defined(SSE_VID_DD7)
  DDBackSurDesc.dwSize=sizeof(DDSURFACEDESC2);
#else
  DDBackSurDesc.dwSize=sizeof(DDSURFACEDESC);
#endif

  if ((Ret=DDBackSur->GetSurfaceDesc(&DDBackSurDesc))!=DD_OK){
    return DDError("GetSurfaceDesc for BackSur FAILED",Ret);
  }

  SurfaceWidth=DDBackSurDesc.dwWidth;
  SurfaceHeight=DDBackSurDesc.dwHeight;
  BytesPerPixel=DDBackSurDesc.ddpfPixelFormat.dwRGBBitCount/8;
  rgb555=(DDBackSurDesc.ddpfPixelFormat.dwGBitMask==0x3E0); //%0000001111100000  //1555
  rgb32_bluestart_bit=int((DDBackSurDesc.ddpfPixelFormat.dwBBitMask==0x0000ff00) ? 8:0);

  draw_init_resdependent();
  palette_prepare(true);
#if defined(SSE_DEBUG) && defined(SSE_VIDEO)
  TRACE_LOG("DD %s Primary %dx%d caps %X flags %X %dhz\n",(FullScreen?"fullscreen":"window"),PrimaryFeedback.dwWidth,PrimaryFeedback.dwHeight,PrimaryFeedback.ddsCaps,PrimaryFeedback.dwFlags,PrimaryFeedback.dwRefreshRate);
  TRACE_LOG("DD Back surface %dx%d %dbit flags %X caps %X triple %d %dhz\n",DDBackSurDesc.dwWidth,DDBackSurDesc.dwHeight,DDBackSurDesc.ddpfPixelFormat.dwRGBBitCount,DDBackSurDesc.dwFlags,DDBackSurDesc.ddsCaps,(bool)DDBackSur2,DDBackSurDesc.dwRefreshRate);
  TRACE_LOG("Source (%d,%d,%d,%d)\n",draw_blit_source_rect.left,draw_blit_source_rect.top,draw_blit_source_rect.right,draw_blit_source_rect.bottom);
#endif
  return DD_OK;
}
//---------------------------------------------------------------------------
void SteemDisplay::DDDestroySurfaces()
{
  //TRACE_LOG("DD Destroy Surfaces\n");
#if defined(SSE_VID_D3D)
  D3DDestroySurfaces();
#endif
  if (DDPrimarySur){
    DDPrimarySur->Release(); DDPrimarySur=NULL;
    if (DDBackSurIsAttached) DDBackSur=NULL;
  }
  if (DDBackSur){
    DDBackSur->Release(); DDBackSur=NULL;
  }
#if defined(SSE_VID_3BUFFER_WIN)
  if (DDBackSur2){
    DDBackSur2->Release(); DDBackSur2=NULL;
  }
#endif
  DDBackSurIsAttached=0;
}
//---------------------------------------------------------------------------
HRESULT SteemDisplay::DDError(char *ErrorText,HRESULT DErr)
{
  Release();

  char Text[1000];
  strcpy(Text,ErrorText);
  strcat(Text,"\n\n");
  DDGetErrorDescription(DErr,Text+strlen(Text),499-strlen(Text));
  log_write("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  log_write(Text);
  TRACE_LOG("!!!!!!!\n%s\n!!!!!!!!!!!!!!",Text);
  log_write("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  strcat(Text,EasyStr("\n\n")+T("Would you like to disable the use of DirectDraw?"));
#ifndef ONEGAME
  int Ret=MessageBox(NULL,Text,T("Steem Engine DirectDraw Error"),
                     MB_YESNO | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_TOPMOST);
  if (Ret==IDYES) WriteCSFStr("Options","NoDirectDraw","1",INIFile);
#endif

  return DErr;
}
#endif//#if !defined(SSE_VID_D3D_ONLY)
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool SteemDisplay::InitGDI()
{ // SS generally Direct X is used instead
  Release(); // note this will kill D3D
  int w=640,h=480;
#ifndef NO_CRAZY_MONITOR
  if (extended_monitor){
    w=GetScreenWidth();
    h=GetScreenHeight();
  }else
#endif

#if defined(SSE_VID_BORDERS)
  if(GetSystemMetrics(SM_CXSCREEN)>640+4*SideBorderSizeWin
    // testing also vertical pixels (bugfix 3.4.1) but... GDI? (useless)
    && GetSystemMetrics(SM_CYSCREEN)>400+2*(BORDER_TOP+BORDER_BOTTOM)
    )
  {
    w=640+4*SideBorderSizeWin;
    h=400+2*(BORDER_TOP+BORDER_BOTTOM);
  }
#else
  if (GetSystemMetrics(SM_CXSCREEN)>768){
    w=768;
    h=400+2*(BORDER_TOP+BORDER_BOTTOM);
  }
#endif

  dbg_log(Str("STARTUP: Creating bitmap w=")+w+" h="+h);
  HDC dc=GetDC(NULL);
  GDIBmp=CreateCompatibleBitmap(dc,w,h);
  ReleaseDC(NULL,dc);

  if (GDIBmp==NULL) return 0;

  BITMAP BmpInf;
  GetObject(GDIBmp,sizeof(BITMAP),&BmpInf);
  BytesPerPixel=(BmpInf.bmBitsPixel+7)/8;
  GDIBmpLineLength=BmpInf.bmWidthBytes;

  GDIBmpSize=GDIBmpLineLength*BmpInf.bmHeight;
  dbg_log(Str("STARTUP: BytesPerPixel=")+BytesPerPixel+" GDIBmpLineLength="+GDIBmpLineLength+" GDIBmpSize="+GDIBmpSize);

  GDIBmpDC=CreateCompatibleDC(NULL);
  SelectObject(GDIBmpDC,GDIBmp);
  SelectObject(GDIBmpDC,fnt);

  dbg_log("STARTUP: Creating bitmap memory");
  try{
    GDIBmpMem=new BYTE[GDIBmpSize+1];
  }catch (...){
    GDIBmpMem=NULL;
    Release();
    return 0;
  }

  if (BytesPerPixel>1){
    SetPixel(GDIBmpDC,0,0,RGB(255,0,0));
    GetBitmapBits(GDIBmp,GDIBmpSize,GDIBmpMem);
    DWORD RedBitMask=0;
    for (int i=BytesPerPixel-1;i>=0;i--){
      RedBitMask<<=8;
      RedBitMask|=GDIBmpMem[i];
    }
    rgb555=(RedBitMask==(hib01111100 | b00000000));
    rgb32_bluestart_bit=int((RedBitMask==0xff000000) ? 8:0);
  }
  SurfaceWidth=w;
  SurfaceHeight=h;
  dbg_log(Str("STARTUP: rgb555=")+rgb555+" rgb32_bluestart_bit="+rgb32_bluestart_bit+
        " SurfaceWidth="+SurfaceWidth+" SurfaceHeight="+SurfaceHeight);

  palette_prepare(true);
  draw_init_resdependent();

  return true;
}
//#undef LOGSECTION//#define LOGSECTION LOGSECTION_INIT
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//#define LOGSECTION LOGSECTION_VIDEO//SS

// for each frame, sequence is lock-write frame (emulate)-unlock-blit

HRESULT SteemDisplay::Lock()
{
#if defined(SSE_VID_SDL) && !defined(SSE_VID_SDL_DEACTIVATE)
  if(SDL.InUse)
  {
    SDL.Lock();
    return DD_OK;
  }
#endif

#if defined(SSE_VID_D3D)
#if defined(SSE_VID_D3D_ONLY)
  if(D3D9_OK && SSE_OPTION_D3D)
#elif defined(SSE_VID_D3D_WINDOW)
  if(D3D9_OK && SSE_OPTION_D3D && pD3DTexture)
#else
  if(D3D9_OK && SSE_OPTION_D3D && FullScreen && pD3DTexture)
#endif
  {
    return D3DLock();
  }
#endif

  switch (Method){
#if !defined(SSE_VID_D3D_ONLY)
    case DISPMETHOD_DD:
    {
      HRESULT DErr
#if defined(SSE_VID_CHECK_POINTERS)
        =1234;
#endif
        ;

      if (DDBackSur==NULL) return DDERR_SURFACELOST;

      if (DDBackSur->IsLost()==DDERR_SURFACELOST) return DDERR_SURFACELOST;
#if defined(SSE_VID_DD7)
      DDBackSurDesc.dwSize=sizeof(DDSURFACEDESC2);
#else
      DDBackSurDesc.dwSize=sizeof(DDSURFACEDESC);
#endif

#if defined(SSE_VID_3BUFFER_WIN)
      // trying to mind performance and footprint
#if defined(SSE_VID_DD7)
      IDirectDrawSurface7 *OurBackSur;
#else
      IDirectDrawSurface *OurBackSur;
#endif
      if(SSE_3BUFFER && DDBackSur2)
      {
        SurfaceToggle=!SurfaceToggle; // toggle at lock
        OurBackSur=(SurfaceToggle) ? DDBackSur2: DDBackSur;
      }
      else
        OurBackSur=DDBackSur;

#if defined(SSE_VID_CHECK_POINTERS)
      if(!OurBackSur)
        return DErr;
#endif

      if ((DErr=OurBackSur->Lock(NULL,&DDBackSurDesc,DDLOCK_WAIT | DDLockFlags,NULL))!=DD_OK){
        if (DErr!=DDERR_SURFACELOST && DErr!=DDERR_SURFACEBUSY){
          DDError(T("DirectDraw Lock Error"),DErr);
          Init();
        }
        return DErr;
      }
#else
      if ((DErr=DDBackSur->Lock(NULL,&DDBackSurDesc,DDLOCK_WAIT | DDLockFlags,NULL))!=DD_OK){
        if (DErr!=DDERR_SURFACELOST && DErr!=DDERR_SURFACEBUSY){
          DDError(T("DirectDraw Lock Error"),DErr);
          Init();
        }
        return DErr;
      }
#endif

      draw_line_length=DDBackSurDesc.lPitch;
      draw_mem=LPBYTE(DDBackSurDesc.lpSurface);


#if defined(SSE_VID_BORDERS_LB_DX)
      // trying to make it crash-free (v3.4.1)
      if(FullScreen && BORDER_40)
      {
        try { //try to erase memory (black screen) //note use fillrect ok?
          ZeroMemory(draw_mem,800*8*200);
        }
        catch(...) {
          // forget it
          TRACE_LOG("Exception during DD Lock CLS\n");
        }
      }
#endif
      return DD_OK;
    }
#endif//#if !defined(SSE_VID_D3D_ONLY)
    case DISPMETHOD_GDI:
      draw_line_length=GDIBmpLineLength;
      draw_mem=GDIBmpMem;
      return DD_OK;
  }
  return DDERR_GENERIC;
}
//---------------------------------------------------------------------------
void SteemDisplay::Unlock()
{

#if defined(SSE_VID_SDL) && !defined(SSE_VID_SDL_DEACTIVATE)
  if(SDL.InUse)
  {
    SDL.Unlock();
    return;
  }
#endif

#if defined(SSE_VID_D3D)
#if defined(SSE_VID_D3D_ONLY)
  if(D3D9_OK && SSE_OPTION_D3D)
#elif defined(SSE_VID_D3D_WINDOW)
  if(D3D9_OK && SSE_OPTION_D3D)
#else
  if(D3D9_OK && SSE_OPTION_D3D && FullScreen)
#endif
  {
    D3DUnlock();
    return;
  }
#endif
#if !defined(SSE_VID_D3D_ONLY)
  if (Method==DISPMETHOD_DD){
#if defined(SSE_VID_3BUFFER_WIN)
#if defined(SSE_VID_DD7)
    IDirectDrawSurface7 
#else
    IDirectDrawSurface 
#endif
      *OurBackSur=
      (SSE_3BUFFER && DDBackSur2 && SurfaceToggle) ? DDBackSur2:DDBackSur;

#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
    // moved here because now we draw garbage after lock (note: DD-only)
      if(COLOUR_MONITOR && shifter_last_draw_line==400)
      {
        HDC hdc;
        OurBackSur->GetDC(&hdc);
        RECT r={0,0,SurfaceWidth,SurfaceHeight};
        FillRect(hdc,&r,(HBRUSH)GetStockObject(BLACK_BRUSH));
        OurBackSur->ReleaseDC(hdc);
      }
#endif

      OurBackSur->Unlock(NULL);
#else
    DDBackSur->Unlock(NULL);
#endif

#if defined(SSE_VID_RECORD_AVI)

    //TODO reuse objects? possible?
    if(video_recording && runstate==RUNSTATE_RUNNING && !SSE_3BUFFER)
    {
      if(!pAviFile)
      {
        if(!frameskip||frameskip==8) // error||auto
          frameskip=1;
        TRACE_LOG("Start AVI recording, codec %s, frameskip %d\n",SSE_VID_RECORD_AVI_CODEC,frameskip);
        pAviFile=new CAviFile(SSE_VID_RECORD_AVI_FILENAME,
          mmioFOURCC(video_recording_codec[0],video_recording_codec[1],
          video_recording_codec[2],video_recording_codec[3]),
          shifter_freq_at_start_of_vbl/frameskip);
      }
      HDC SurfDC;
      DDBackSur->GetDC(&SurfDC);
      DDSURFACEDESC ddsd;
      ZeroMemory(&ddsd, sizeof(ddsd));
      ddsd.dwSize = sizeof(ddsd);
      DDBackSur->GetSurfaceDesc(&ddsd);
      HBITMAP OffscrBmp = CreateCompatibleBitmap(SurfDC,ddsd.dwWidth,ddsd.
        dwHeight);
      HDC OffscrDC = CreateCompatibleDC(SurfDC);
      HBITMAP OldBmp = (HBITMAP)SelectObject(OffscrDC, OffscrBmp);
      BitBlt(OffscrDC, 0, 0,ddsd.dwWidth,ddsd.dwHeight, SurfDC, 0, 0, SRCCOPY);
      if(pAviFile->AppendNewFrame(OffscrBmp))
      {
        delete pAviFile;
        video_recording=0;
      }
      DeleteDC(OffscrDC); // important, release Windows resources!
      DeleteObject(OldBmp);
      DeleteObject(OffscrBmp);
      DDBackSur->ReleaseDC(SurfDC);
    }
#endif
  }else if (Method==DISPMETHOD_GDI){
#else
  {
#endif//#if !defined(SSE_VID_D3D_ONLY)
    SetBitmapBits(GDIBmp,GDIBmpSize,GDIBmpMem);
  }
}
//---------------------------------------------------------------------------
void SteemDisplay::VSync()
{
#if !defined(SSE_VID_VSYNC_WINDOW)
  if (FullScreen==0) return;
#endif

#if defined(SSE_VID_D3D_ONLY)
// need to do something?
#else
#if defined(SSE_VID_VSYNC_WINDOW_CRASH_FIX1)
  if(!DDObj)
    return;
#endif
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: VSYNC - Starting wait for VBL at ")+(timeGetTime()-run_start_time));

  BOOL Blanking=FALSE;
  DDObj->GetVerticalBlankStatus(&Blanking);
  if (Blanking==FALSE){
#if defined(SSE_VID_VSYNC_WINDOW)
    DWORD botline; //=480-40;
    if(FullScreen
#if defined(SSE_VID_D3D)
      && !(D3D9_OK && SSE_OPTION_D3D)
#endif
      )
      if (border & 1)
        botline=600-40;
      else if (using_res_640_400)
        botline=400-40;
      else
        botline=480-40;
    else // window
      botline=GetScreenHeight();    
#else
    DWORD botline=480-40;
    if (border & 1){
      botline=600-40;
    }else if (using_res_640_400){
      botline=400-40;
    }
#endif
    DWORD line;
    HRESULT hRet;
    do{
      hRet=DDObj->GetScanLine(&line);
      if (line>=botline){
        break;
      }else{
//        Sleep(0);
      }
    }while (hRet==DD_OK);
    if (hRet!=DD_OK && hRet!=DDERR_VERTICALBLANKINPROGRESS){
      DDObj->GetVerticalBlankStatus(&Blanking);
      if (Blanking==FALSE) DDObj->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,NULL);
    }
  }
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: VSYNC - Finished waiting for VBL at ")+(timeGetTime()-run_start_time));
#endif//#if !defined(SSE_VID_D3D_ONLY)
}
//---------------------------------------------------------------------------

bool SteemDisplay::Blit()
{
#if defined(SSE_VID_SDL) && !defined(SSE_VID_SDL_DEACTIVATE)
  if(SDL.InUse)
  {
    SDL.Blit();
    return true;
  }
#endif

#if defined(SSE_VID_D3D)
#if defined(SSE_VID_D3D_ONLY)
  if(D3D9_OK && SSE_OPTION_D3D)
#elif defined(SSE_VID_D3D_WINDOW)
  if(D3D9_OK && SSE_OPTION_D3D)
#else
  if(D3D9_OK && SSE_OPTION_D3D && FullScreen)
#endif
  {
    return D3DBlit();
  }
#endif
#if !defined(SSE_VID_D3D_ONLY)
  if (Method==DISPMETHOD_DD){
    HRESULT hRet;
    if (FullScreen){
      if (runstate==RUNSTATE_RUNNING){
#if defined(SSE_VID_BLIT_TRY_BLOCK)
        try {
#endif
        switch (draw_fs_blit_mode){
          case DFSM_FLIP:
#if defined(SSE_VID_BORDERS_LB_DX)
            if(BORDER_40) // bad framing in flip mode
            {
              draw_fs_blit_mode=DFSM_STRAIGHTBLIT;
              TRACE_LOG("Fullscreen BorderSize %d changing to mode %d\n",DISPLAY_SIZE,draw_fs_blit_mode);
              break;
            }
#endif

            hRet=DDPrimarySur->Flip(NULL,0); //DDFLIP_WAIT);
            break;
          case DFSM_STRAIGHTBLIT:
          {
#if defined(SSE_VID_BORDERS_LB_DX)
            if(BORDER_40) // clip from larger to 800
            {
              RECT our_clipping={16,0,816,556-6};
              hRet=DDPrimarySur->Blt(&draw_blit_source_rect,DDBackSur,
                  &our_clipping,DDBLT_WAIT,NULL);
            }
            else // other sizes: blit fast
              hRet=DDPrimarySur->BltFast(draw_blit_source_rect.left,draw_blit_source_rect.top,
                DDBackSur,&draw_blit_source_rect,DDBLTFAST_WAIT);
#else
            hRet=DDPrimarySur->BltFast(draw_blit_source_rect.left,draw_blit_source_rect.top,
                                        DDBackSur,&draw_blit_source_rect,DDBLTFAST_WAIT);
#endif
            break;
          }
          case DFSM_STRETCHBLIT:case DFSM_LAPTOP:
          {
            RECT Dest;
            get_fullscreen_rect(&Dest);
#if defined(SSE_VID_SCANLINES_INTERPOLATED)
            if(SCANLINES_INTERPOLATED)
              if(screen_res==1)
                draw_blit_source_rect.right--; // and another hack...
              else
                draw_blit_source_rect.right/=2; 
#endif

#if defined(SSE_VID_3BUFFER_FS)
#if defined(SSE_VID_DD7)
            IDirectDrawSurface7 
#else
            IDirectDrawSurface 
#endif
              *OurBackSur;
            if(SSE_3BUFFER && DDBackSur2)
            {
              OurBackSur=(!SurfaceToggle) ? DDBackSur2:DDBackSur;
              
              if(OurBackSur->GetBltStatus(DDGBS_CANBLT)==DD_OK)
                hRet=DDPrimarySur->Blt(&Dest,OurBackSur,&draw_blit_source_rect,
                DDBLT_WAIT,NULL);
              else
                TRACE_LOG("Can't blt\n");
            }
            else
#endif
            hRet=DDPrimarySur->Blt(&Dest,DDBackSur,&draw_blit_source_rect,DDBLT_WAIT,NULL);

#if defined(SSE_VID_SCANLINES_INTERPOLATED)
            if(SCANLINES_INTERPOLATED) //restore!
              if(screen_res==1)
                draw_blit_source_rect.right++;
              else
                draw_blit_source_rect.right*=2; 
#endif
            break;
          }
        }
#if defined(SSE_VID_BLIT_TRY_BLOCK)
        }
        catch(...) { // VC6: catches system exceptions
          hRet=1234; // real ones are negative
        }
#endif
        if (hRet==DDERR_SURFACELOST){ 
          hRet=RestoreSurfaces();
          if (hRet!=DD_OK){ // SS can happen if idle for long
            DDError(T("Drawing memory permanently lost"),hRet);
            TRACE_LOG("Drawing memory permanently lost\n");
            Init();
          }
        }
#if defined(SSE_VID_BORDERS)
/*  It can fail for whatever reason. In that case we 
    brutally stop so the player sees it's wrong.
*/
        else if(hRet) 
        {
          TRACE_LOG("Fullscreen blit error %d\n",hRet);
#if defined(SSE_GUI_STATUS_BAR_ALERT)
          M68000.ProcessingState=TM68000::BLIT_ERROR;
#endif
          runstate=RUNSTATE_STOPPING;
        }
#endif
      }else{ //not running right now
        HCURSOR OldCur;
		if (BlitHideMouse) OldCur=SetCursor(NULL);
        RECT Dest;
        get_fullscreen_rect(&Dest);
        for (int i=0;i<2;i++){

#if defined(SSE_VID_3BUFFER_WIN)
#if defined(SSE_VID_DD7)
          IDirectDrawSurface7 
#else
          IDirectDrawSurface 
#endif
            *OurBackSur=
            (SSE_3BUFFER && !SurfaceToggle && DDBackSur2) 
            ? DDBackSur2: DDBackSur;

#if defined(SSE_VID_BORDERS_LB_DX)
          if(BORDER_40) // clip from larger to 800
          {
            RECT our_clipping={16,0,816,556-6};
            hRet=DDPrimarySur->Blt(&Dest,OurBackSur,
                &our_clipping,DDBLT_WAIT,NULL);
            }
          else
#endif
          hRet=DDPrimarySur->Blt(&Dest,OurBackSur,&draw_blit_source_rect,
            DDBLT_WAIT,NULL);
#else
          hRet=DDPrimarySur->Blt(&Dest,DDBackSur,&draw_blit_source_rect,DDBLT_WAIT,NULL);
#endif
          if (hRet==DDERR_SURFACELOST){
            if (i==0) hRet=RestoreSurfaces();
            if (hRet!=DD_OK){
              DDError(T("Drawing memory permanently lost"),hRet);
              TRACE_LOG("Drawing memory permanently lost\n");
              Init();
              break;
            }
          }else{
            break;
          }
        }
        if (BlitHideMouse) SetCursor(OldCur);
      }
    }else{  
      
      // SS not Fullscreen:

      HCURSOR OldCur;
      if (stem_mousemode==STEM_MOUSEMODE_DISABLED && BlitHideMouse) OldCur=SetCursor(NULL);
      RECT dest;GetClientRect(StemWin,&dest);
      dest.top+=MENUHEIGHT;dest.right-=4;dest.bottom-=4;
      POINT pt={2,2};
      ClientToScreen(StemWin,&pt);
      OffsetRect(&dest,pt.x,pt.y);
#if defined(SSE_VID_BORDERS_LB_DX) 
      if(BORDER_40 && (screen_res==2 || draw_win_mode[screen_res]))
      {// clip from larger to 800
        OffsetRect(&draw_blit_source_rect,
        (CanUse_400 && !SCANLINES_INTERPOLATED)?16:8,0);
      }
#endif

#if defined(SSE_VID_SCANLINES_INTERPOLATED)
      if(screen_res==1&&SCANLINES_INTERPOLATED)
        draw_blit_source_rect.right--; // and another hack...
#endif

      for (int i=0;i<2;i++){
#if defined(SSE_VID_3BUFFER)
#if defined(SSE_VID_DD7)
        IDirectDrawSurface7 
#else
        IDirectDrawSurface 
#endif
          *OurBackSur=
          (SSE_3BUFFER && !SurfaceToggle && DDBackSur2) ? DDBackSur2:DDBackSur;
        hRet=OurBackSur->GetBltStatus(DDGBS_CANBLT); // for surface lost
        if(hRet==DD_OK)
          hRet=DDPrimarySur->Blt(&dest,OurBackSur,&draw_blit_source_rect,
            DDBLT_WAIT,NULL);
#ifdef SSE_DEBUG
        else
          TRACE_VID("Can't blt\n");
#endif
#else
        hRet=DDPrimarySur->Blt(&dest,DDBackSur,&draw_blit_source_rect,DDBLT_WAIT,NULL);
#endif

        if (hRet==DDERR_SURFACELOST){
          if (i==0) hRet=RestoreSurfaces();
          if (hRet!=DD_OK){
#if !defined(SSE_VID_MEMORY_LOST) // no message box
            DDError(T("Drawing memory permanently lost"),hRet);
#endif
            Init();
            break;
          }
        }else{
          break;
        }
      }
      if (stem_mousemode==STEM_MOUSEMODE_DISABLED && BlitHideMouse) SetCursor(OldCur);
    }

    if (hRet==DD_OK) return true;
  }else if (Method==DISPMETHOD_GDI){
#else
  {
#endif//#if !defined(SSE_VID_D3D_ONLY)
    RECT dest;
    GetClientRect(StemWin,&dest);
    HDC dc=GetDC(StemWin);
    SetStretchBltMode(dc,COLORONCOLOR);
    StretchBlt(dc,2,MENUHEIGHT+2,dest.right-4,dest.bottom-(MENUHEIGHT+4),
               GDIBmpDC,draw_blit_source_rect.left,draw_blit_source_rect.top,
               draw_blit_source_rect.right-draw_blit_source_rect.left,
               draw_blit_source_rect.bottom-draw_blit_source_rect.top,SRCCOPY);
    ReleaseDC(StemWin,dc);
    return true;
  }
#if !(defined(SSE_VID_D3D_ONLY) && defined(SSE_VS2008_WARNING_382))
  return 0;
#endif
}
//---------------------------------------------------------------------------
void SteemDisplay::WaitForAsyncBlitToFinish()
{
}
//#undef LOGSECTION //video
//---------------------------------------------------------------------------

//#define LOGSECTION LOGSECTION_INIT //SS

void SteemDisplay::RunStart(bool Temp)
{
  if (FullScreen==0) return;
  if (Temp==0){
    bool ChangeSize=0;
#if defined(SSE_VID_D3D_ONLY) && defined(SSE_VS2008_WARNING_390)
    int w=640,h=400;
#else
    int w=640,h=400,hz=0;
#endif
#ifndef NO_CRAZY_MONITOR
    if (extended_monitor && (em_width<GetScreenWidth() || em_height<GetScreenHeight())){
      ChangeSize=true;
      w=em_width;
      h=em_height;
    }

    if (extended_monitor==0)
#endif
#if !defined(SSE_VID_D3D_ONLY)
    if (draw_fs_blit_mode!=DFSM_LAPTOP){
      if (prefer_res_640_400 && border==0 && DDDisplayModePossible[2][int(BytesPerPixel==1 ? 0:1)]){
        ChangeSize=true;
        hz=prefer_pc_hz[BytesPerPixel-1][0];
      }
    }
#endif
    if (ChangeSize){
#if defined(SSE_VID_D3D_ONLY) && defined(SSE_VS2008_WARNING_390)
      if (SetDisplayMode()==DD_OK){
#else
      int hz_ok=0;
      if (SetDisplayMode(w,h,BytesPerPixel*8,hz,&hz_ok)==DD_OK){
#endif
#if !defined(SSE_VID_D3D_ONLY)
        if (hz) tested_pc_hz[BytesPerPixel-1][0]=MAKEWORD(hz,hz_ok);
        using_res_640_400=true;
#endif
      }else{
        change_fullscreen_display_mode(0);
      }
    }
  }
#if !defined(SSE_VID_D3D_ONLY)
#ifdef SSE_VID_CHECK_POINTERS
  if(DDPrimarySur)
#endif
    DDPrimarySur->SetClipper(NULL);
#endif//#if !defined(SSE_VID_D3D_ONLY)
  ShowAllDialogs(0);

  SetStemMouseMode(STEM_MOUSEMODE_WINDOW);

#ifdef WIN32
#if !defined(SSE_VID_FS_382)
/*  This could interfere with user rights somehow, and fullscreen
    wouldn't display anything if Steem isn't run as administrator.
    Very strange: build in VS2008 when SSE_VID_FS_382 isn't defined,
    it doesn't work as simple user, rename the file, it does. Win 10 Pro.
*/
  if (DrawLetterboxWithGDI==0) LockWindowUpdate(StemWin);
#endif
  while (ShowCursor(0)>=0);
  SetCursor(NULL);
#endif
#if !defined(SSE_VID_D3D_ONLY)
  Disp.DrawFullScreenLetterbox();
#endif
}
//---------------------------------------------------------------------------
#if defined(SSE_VID_D3D_ONLY) && defined(SSE_VS2008_WARNING_390)
void SteemDisplay::RunEnd()
#else
void SteemDisplay::RunEnd(bool Temp)
#endif
{
  //TRACE_LOG("FS RunEnd(%d)\n",using_res_640_400);
#if defined(SSE_VID_FS_382)
 // ASSERT(FullScreen);
#else
  if (FullScreen==0) return;
#endif
#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_D3D)
  if(!(D3D9_OK && SSE_OPTION_D3D))
#endif
  if (using_res_640_400 && Temp==0 && bAppActive){
    // Save background
    RECT rcDest;
    get_fullscreen_rect(&rcDest);
    OffsetRect(&rcDest,-rcDest.left,-rcDest.top);
    int w=rcDest.right,h=rcDest.bottom;

    HRESULT hRet;
#if defined(SSE_VID_DD7)
    IDirectDrawSurface7 *SaveSur=NULL;
    DDSURFACEDESC2 SaveSurDesc;
#else
    IDirectDrawSurface *SaveSur=NULL;
    DDSURFACEDESC SaveSurDesc;
#endif
#if defined(SSE_VID_DD7) && defined(SSE_VID_FS_382)
    ZeroMemory(&SaveSurDesc,sizeof(DDSURFACEDESC2));
    SaveSurDesc.dwSize=sizeof(DDSURFACEDESC2);
#else
    ZeroMemory(&SaveSurDesc,sizeof(DDSURFACEDESC));
    SaveSurDesc.dwSize=sizeof(DDSURFACEDESC);
#endif
    SaveSurDesc.dwFlags=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    SaveSurDesc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    SaveSurDesc.dwWidth=w;
    SaveSurDesc.dwHeight=h;
    hRet=DDObj->CreateSurface(&SaveSurDesc,&SaveSur,NULL);
    if (hRet==DD_OK){
#if defined(SSE_VID_3BUFFER_FS)
#if defined(SSE_VID_DD7)
      IDirectDrawSurface7 
#else
      IDirectDrawSurface 
#endif
          *OurBackSur=
        (SSE_3BUFFER && DDBackSur2 && !SurfaceToggle) ? DDBackSur2: DDBackSur;
      hRet=SaveSur->Blt(&rcDest,OurBackSur,&draw_blit_source_rect,DDBLT_WAIT,NULL);
#else
      hRet=SaveSur->Blt(&rcDest,DDBackSur,&draw_blit_source_rect,DDBLT_WAIT,NULL);
#endif
      if (hRet!=DD_OK){
        SaveSur->Release(); SaveSur=NULL;
      }
    }else{
      SaveSur->Release(); SaveSur=NULL;
    }

    using_res_640_400=0;

    change_fullscreen_display_mode(true);

    if (SaveSur){
      DDBackSur->Blt(&draw_blit_source_rect,SaveSur,NULL,DDBLT_WAIT,NULL);
      SaveSur->Release();
    }

  }
#endif//#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_FS_GUI_OPTION)
/*  This is handy if it works. Blame Microsoft and video card drivers 
    if it doesn't.
*/
  if(OPTION_FULLSCREEN_GUI) 
#endif
#if defined(SSE_VID_D3D)
    if(D3D9_OK && SSE_OPTION_D3D && pD3DDevice)
#if defined(SSE_VID_FS_GUI_OPTION)
      pD3DDevice->SetDialogBoxMode(TRUE);
#if !defined(SSE_VID_D3D_ONLY)
    else
#endif
#endif
#endif
#if !defined(SSE_VID_D3D_ONLY)
      DDObj->FlipToGDISurface();
#endif
#if !defined(SSE_VID_FS_382)
  LockWindowUpdate(NULL);
#endif
#if !defined(SSE_VID_D3D_ONLY)
#ifdef SSE_VID_CHECK_POINTERS
  if(DDPrimarySur)
#endif
    DDPrimarySur->SetClipper(DDClipper);
#endif//#if !defined(SSE_VID_D3D_ONLY)
  while (ShowCursor(true)<0);

#if defined(SSE_VID_FS_GUI_OPTION) || defined(SSE_VID_D3D)
#if defined(SSE_VID_FS_GUI_OPTION)
  if(!OPTION_FULLSCREEN_GUI)
#elif defined(SSE_VID_D3D)
  if(D3D9_OK && SSE_OPTION_D3D)
#endif
  {
    ChangeToWindowedMode(); // this should work on all systems
    return;
  }
#endif

  ShowAllDialogs(true);

  InvalidateRect(StemWin,NULL,true);

}
//---------------------------------------------------------------------------
#if !defined(SSE_VID_D3D_ONLY)
void SteemDisplay::DrawFullScreenLetterbox()
{
  if (FullScreen==0 || using_res_640_400) return;
  if (draw_fs_blit_mode==DFSM_LAPTOP) return;

#if defined(SSE_VID_CHECK_POINTERS)
  if(!DDBackSur || !DDPrimarySur)
    return;
#endif

#ifndef NO_CRAZY_MONITOR
  if (extended_monitor) return;
#endif

  if (draw_fs_topgap || (border & 1)){
    if (Method==DISPMETHOD_DD){
      DDBLTFX bfx;
      ZeroMemory(&bfx,sizeof(DDBLTFX));
      bfx.dwSize=sizeof(DDBLTFX);
      bfx.dwFillColor=RGB(0,0,0);

      HDC dc=NULL;
      if (DrawLetterboxWithGDI) dc=GetDC(StemWin);

      RECT Dest={0,0,640,draw_fs_topgap};
      if (border & 1){
        Dest.right=800;
        Dest.bottom=(600-400-2*(BORDER_TOP+BORDER_BOTTOM))/2;
      }
      DDBackSur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
      if (dc){
        FillRect(dc,&Dest,(HBRUSH)GetStockObject(BLACK_BRUSH));
      }else{
        DDPrimarySur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
      }
      if (border & 1){
        Dest.top=600-Dest.bottom;
        Dest.bottom=600;
      }else{
        Dest.top=440;
        Dest.bottom=480;
      }
      DDBackSur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
      if (dc){
        FillRect(dc,&Dest,(HBRUSH)GetStockObject(BLACK_BRUSH));
      }else{
        DDPrimarySur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
      }

      if (border & 1){
        int SideGap=(800 - (BORDER_SIDE+320+BORDER_SIDE)*2) / 2;

        Dest.top=0;Dest.bottom=600;
        Dest.left=0;Dest.right=SideGap;
        DDBackSur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
        if (dc){
          FillRect(dc,&Dest,(HBRUSH)GetStockObject(BLACK_BRUSH)); // SS ape this instead...
        }else{
          DDPrimarySur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
        }

        Dest.left=800-SideGap;Dest.right=800;
        DDBackSur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
        if (dc){
          FillRect(dc,&Dest,(HBRUSH)GetStockObject(BLACK_BRUSH));
        }else{
          DDPrimarySur->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL | DDBLT_WAIT,&bfx);
        }
      }
      if (dc) ReleaseDC(StemWin,dc);
    }
  }
}
#endif//#if !defined(SSE_VID_D3D_ONLY)
//---------------------------------------------------------------------------
void SteemDisplay::ScreenChange()
{
  draw_end();
  TRACE_LOG("ScreenChange()\n");
#if defined(SSE_VID_SDL) && !defined(SSE_VID_SDL_DEACTIVATE)
  // temp, dubious
  if(USE_SDL && !SDL.InUse)
    SDL.EnterSDLVideoMode(); 
  else if(!USE_SDL && SDL.InUse)
    SDL.LeaveSDLVideoMode();
#endif

  if (Method==DISPMETHOD_DD){
// DX4         if (DDObj->TestCooperativeLevel()!=DDERR_EXCLUSIVEMODEALREADYSET)
#if defined(SSE_VID_D3D_ONLY)
    if (D3DCreateSurfaces()!=DD_OK) Init();
#else
    if (DDCreateSurfaces()!=DD_OK) Init();
#endif
  }else if (Method==DISPMETHOD_GDI){
    if (InitGDI()==0){
      Init();
    }else{
      Method=DISPMETHOD_GDI;
    }
  }
}
//---------------------------------------------------------------------------

/* SS player clicked on 'maximize'


  Disp.ChangeToFullScreen()
    |
    L-> change_fullscreen_display_mode(0),
          |
          L-> Disp.SetDisplayMode(w,h,bits,hz,&hz_ok)
*/

void SteemDisplay::ChangeToFullScreen()
{
  if (CanGoToFullScreen()==0 || FullScreen 
#if !defined(SSE_VID_D3D_ONLY)
    || DDExclusive
#endif
    ) return;

  TRACE_LOG("Going fullscreen...\n");

#if defined(SSE_VID_SCANLINES_INTERPOLATED) && !defined(SSE_VID_D3D_ONLY)
    if(SCANLINES_INTERPOLATED)
      draw_fs_blit_mode=DFSM_STRETCHBLIT; // only compatible FS mode
#endif

  draw_end();

  if (runstate==RUNSTATE_RUNNING){
    runstate=RUNSTATE_STOPPING;
    PostMessage(StemWin,WM_SYSCOMMAND,SC_MAXIMIZE,0);
  }else if (runstate!=RUNSTATE_STOPPED){ //Keep trying until succeed!
    PostMessage(StemWin,WM_SYSCOMMAND,SC_MAXIMIZE,0);
  }else if (bAppMinimized){
    ShowWindow(StemWin,SW_RESTORE);
    PostMessage(StemWin,WM_SYSCOMMAND,SC_MAXIMIZE,0);
  }else{
    bool MaximizeDiskMan=0;
#if defined(SSE_VID_D3D)
    if(!(D3D9_OK && SSE_OPTION_D3D))
#endif
    if (DiskMan.IsVisible()){
      if (IsIconic(DiskMan.Handle)) ShowWindow(DiskMan.Handle,SW_RESTORE);
      MaximizeDiskMan=DiskMan.FSMaximized;
      SetWindowLong(DiskMan.Handle,GWL_STYLE,(GetWindowLong(DiskMan.Handle,GWL_STYLE) & ~WS_MAXIMIZE) & ~WS_MINIMIZEBOX);
    }

    FullScreen=true;

    DirectoryTree::PopupParent=StemWin;
    
    GetWindowRect(StemWin,&rcPreFS); //SS "before fullscreen"
#if defined(SSE_VID_FS_GUI_OPTION)
    if(OPTION_FULLSCREEN_GUI)
#elif defined(SSE_VID_D3D)
    if(!(D3D9_OK && SSE_OPTION_D3D))
#endif    
    ShowWindow(GetDlgItem(StemWin,106),SW_SHOWNA); //SS icon "back to windowed"
    SetWindowLong(StemWin,GWL_STYLE,WS_VISIBLE);
#if defined(SSE_VID_D3D_ONLY)
    SetWindowPos(StemWin,HWND_TOPMOST,0,0,D3DFsW,D3DFsH,0);
#else
    int w=640,h=480;
    if (border & 1){
      w=800;h=600;
    }
#ifndef NO_CRAZY_MONITOR
    if (extended_monitor){
      w=em_width;h=em_height;
    }
#endif
    if (draw_fs_blit_mode==DFSM_LAPTOP){
      w=monitor_width;
      h=monitor_height;
    }
    SetWindowPos(StemWin,HWND_TOPMOST,0,0,w,h,0);
#endif//#if defined(SSE_VID_D3D_ONLY)

    CheckResetDisplay(true);
#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_D3D)
    if(!(D3D9_OK && SSE_OPTION_D3D))
#endif
    ClipWin=CreateWindow("Steem Fullscreen Clip Window","",WS_CHILD | WS_VISIBLE |
                          WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                          0,MENUHEIGHT,w,h-MENUHEIGHT,StemWin,(HMENU)1111,Inst,NULL);

#if defined(SSE_VID_D3D)
    if(!(D3D9_OK && SSE_OPTION_D3D))
#endif
    DDClipper->SetHWnd(0,ClipWin);
#endif//#if !defined(SSE_VID_D3D_ONLY)
#ifndef ONEGAME
#if defined(SSE_VID_FS_382)
    if(FSQuitBut)
      DestroyWindow(FSQuitBut);
#endif
#if defined(SSE_VID_D3D_ONLY)
    FSQuitBut=CreateWindow("Steem Fullscreen Quit Button","",WS_CHILD,
      0,D3DFsH-MENUHEIGHT-14,16,14,StemWin,HMENU(100),Inst,NULL);
    ShowWindow(FSQuitBut,TRUE);
#else
    FSQuitBut=CreateWindow("Steem Fullscreen Quit Button","",WS_CHILD,
                            0,h-MENUHEIGHT-14,16,14,ClipWin,HMENU(100),Inst,NULL);
#endif
    ToolAddWindow(ToolTip,FSQuitBut,T("Quit"));
#endif

    bool ShowInfoBox=InfoBox.IsVisible();
    for (int n=0;n<nStemDialogs;n++){
      if (DialogList[n]!=&InfoBox){
        DEBUG_ONLY( if (DialogList[n]!=&HistList) ) DialogList[n]->MakeParent(StemWin);
      }
    }
    InfoBox.Hide();

    SetParent(ToolTip,StemWin);
#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_D3D)
    if(!(D3D9_OK && SSE_OPTION_D3D))
#endif
    {//SS
    HRESULT Ret=DDObj->SetCooperativeLevel(StemWin,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN |
                                            DDSCL_ALLOWREBOOT);
    if (Ret!=DD_OK){
      DDError(T("Can't SetCooperativeLevel to exclusive"),Ret);
      Init();
      return;
    }
    }//SS
    DDExclusive=true;
#endif

    if (change_fullscreen_display_mode(0)==DD_OK){

      if (ShowInfoBox) InfoBox.Show();
      if (MaximizeDiskMan){
        SendMessage(DiskMan.Handle,WM_SETREDRAW,0,0);
        ShowWindow(DiskMan.Handle,SW_MAXIMIZE);
        PostMessage(DiskMan.Handle,WM_SETREDRAW,true,0);
      }

      OptionBox.EnableBorderOptions(true);

      SetForegroundWindow(StemWin);

      SetFocus(StemWin);

      palette_convert_all();

      ONEGAME_ONLY( DestroyNotifyInitWin(); )

#if defined(SSE_VID_FS_GUI_OPTION)
      if(!OPTION_FULLSCREEN_GUI) 
#endif
        PostRunMessage();

    }else{ //back to windowed mode
      TRACE_LOG("Can't go fullscreen 2\n");
      ChangeToWindowedMode(true);
    }
  }
}
//---------------------------------------------------------------------------
void SteemDisplay::ChangeToWindowedMode(bool Emergency)
{
  if (
#if !defined(SSE_VID_D3D_ONLY)
    DDExclusive==0 && 
#endif
    FullScreen==0) return;
  TRACE_LOG("Going windowed mode...\n");
  WIN_ONLY( if (FullScreen) TScreenSaver::killTimer(); )

  bool CanChangeNow=true;
  if (runstate==RUNSTATE_RUNNING){
    runstate=RUNSTATE_STOPPING;
    PostMessage(StemWin,WM_COMMAND,MAKEWPARAM(106,BN_CLICKED),(LPARAM)GetDlgItem(StemWin,106));
    CanChangeNow=0;
  }else if (runstate!=RUNSTATE_STOPPED){ //Keep trying until succeed!
    PostMessage(StemWin,WM_COMMAND,MAKEWPARAM(106,BN_CLICKED),(LPARAM)GetDlgItem(StemWin,106));
    CanChangeNow=0;
  }
  if (CanChangeNow || Emergency){
#if !defined(SSE_VID_D3D_ONLY)
    if (DDExclusive){
#endif
      draw_end();
#if defined(SSE_VID_D3D_ONLY)
      D3DDestroySurfaces();
#else
      DDDestroySurfaces();
      DDObj->RestoreDisplayMode();
      DDObj->SetCooperativeLevel(StemWin,DDSCL_NORMAL);
#endif
#if !defined(SSE_VID_D3D_ONLY)
      DDExclusive=0;
    }
#endif

    FullScreen=0;
#if defined(SSE_VID_D3D_ONLY)
    if(D3DCreateSurfaces()!=DD_OK)
      Init();
#else
    if (DDCreateSurfaces()!=DD_OK){
      Init();
    }else{
      DDClipper->SetHWnd(0,StemWin);
    }
#endif//#if defined(SSE_VID_D3D_ONLY)
    CheckResetDisplay(true); // Hide fullscreen reset display
#if defined(SSE_VID_D3D_ONLY)
    ToolsDeleteAllChildren(ToolTip,StemWin);
    DestroyWindow(FSQuitBut);
#else
    ToolsDeleteAllChildren(ToolTip,ClipWin);
    DestroyWindow(ClipWin);
#endif
    FSQuitBut=NULL;

    DirectoryTree::PopupParent=NULL;
#if !defined(SSE_VID_FS_382)
    LockWindowUpdate(NULL);
#endif
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
    if (border==3) overscan=OVERSCAN_MAX_COUNTDOWN; // Make sure auto border turns off
#endif
    // Sometimes things won't work if you do them immediately after switching to
    // windowed mode, so post a message and resize all the windows back when we can
    PostMessage(StemWin,WM_USER,12,0);
    ChangeToWinTimeOut=timeGetTime()+2000;
#if defined(SSE_VID_FS_382)
    InvalidateRect(StemWin,NULL,true);
#endif
  }
}
//---------------------------------------------------------------------------
bool SteemDisplay::CanGoToFullScreen()
{
  if (Method==DISPMETHOD_DD){
    return true;
  }
  TRACE_LOG("Can't go fullscreen 1, Method #%d\n",Method);
  return 0;
}
//---------------------------------------------------------------------------
void SteemDisplay::FlipToDialogsScreen() //SS called by Alert()
{
#if defined(SSE_VID_D3D_ONLY)
#else
  if (Method==DISPMETHOD_DD) DDObj->FlipToGDISurface();
#endif
}
//---------------------------------------------------------------------------
#if defined(SSE_VID_D3D_ONLY) && defined(SSE_VS2008_WARNING_390)
HRESULT SteemDisplay::SetDisplayMode()
#else
HRESULT SteemDisplay::SetDisplayMode(int w,int h,int bpp,int hz,int *hz_ok)
#endif
{
  ASSERT(FullScreen);

#if defined(SSE_VID_D3D)
/*  We ignore parameters, we use other values for size.
    DDCreateSurfaces() destroys surfaces and calls D3DCreateSurfaces().
*/
  TRACE_LOG("D3D9 ok%d option %d\n",D3D9_OK,SSE_OPTION_D3D);
  if(D3D9_OK && SSE_OPTION_D3D)
#if defined(SSE_VID_D3D_ONLY)
    return D3DCreateSurfaces();
#else
    return DDCreateSurfaces();
#endif
#endif
#if !defined(SSE_VID_D3D_ONLY)
  if (Method==DISPMETHOD_DD && DDExclusive){
    int idx=-1;
    if (w==640 && h==480) idx=0;
    if (w==800 && h==600) idx=1;
    if (w==640 && h==400) idx=2;
    if (idx>=0){
      for (int n=1;n<NUM_HZ;n++){
        if (hz==HzIdxToHz[n]){
          hz=DDClosestHz[idx][int(bpp>8)][n];
          break;
        }
      }
    }
    TRACE_LOG("SetDisplayMode %dx%d %dbit %dhz\n",w,h,bpp,hz);
    log_write(Str("PC DISPLAY: Changing mode to ")+w+"x"+h+"x"+bpp+" "+hz+"hz");

      HRESULT Ret=DDObj->SetDisplayMode(w,h,bpp,hz,0);
      if (Ret!=DD_OK){
        log_write("  It failed");
        if (hz_ok) *hz_ok=0;
        Ret=DDObj->SetDisplayMode(w,h,bpp,0,0);
        TRACE_LOG("SetDisplayMode %dx%d %dbit %dhz ERR %d\n",w,h,bpp,0,Ret);
      }else{
        log_write("  Success");
        if (hz_ok) *hz_ok=1;
      }
      if (Ret!=DD_OK){
        //      DDError(T("Can't SetDisplayMode"),Ret);
        //      Init();
        TRACE_LOG("DD error %d\n",Ret);
        return Ret;
      }

      if ((Ret=DDCreateSurfaces())!=DD_OK) Init();
      if (Ret!=DD_OK)
        TRACE_LOG("DD error %d\n",Ret);
      return Ret;

  }
#endif//#if !defined(SSE_VID_D3D_ONLY)
  return DDERR_GENERIC;
}
//---------------------------------------------------------------------------
#if !defined(SSE_VID_D3D_ONLY)
HRESULT SteemDisplay::RestoreSurfaces()
{
  if (Method==DISPMETHOD_DD){
    draw_end();
    HRESULT hRet=DDPrimarySur->Restore();
    if (hRet==DD_OK){
      hRet=DDBackSur->Restore();
#if defined(SSE_VID_3BUFFER_WIN)
      if(SSE_3BUFFER && hRet==DD_OK && DDBackSur2) 
        hRet=DDBackSur2->Restore();
      SurfaceToggle=true;
      VSyncTiming=0;
#endif
      if (hRet==DD_OK) return hRet;
    }
    return DDCreateSurfaces();
  }
  return DD_OK;
}
#endif//#if !defined(SSE_VID_D3D_ONLY)
//---------------------------------------------------------------------------
void SteemDisplay::Release()
{
  log_to_section(LOGSECTION_SHUTDOWN,"SHUTDOWN: Display::Release - calling draw_end()");
  draw_end();
  if (GDIBmp!=NULL){
    log_to_section(LOGSECTION_SHUTDOWN,"SHUTDOWN: Freeing GDI stuff");
    DeleteDC(GDIBmpDC);   GDIBmpDC=NULL;
    DeleteObject(GDIBmp); GDIBmp=NULL;
    delete[] GDIBmpMem;
  }
#if !defined(SSE_VID_D3D_ONLY)
  if (DDObj!=NULL){
    if (DDExclusive || FullScreen){
      log_to_section(LOGSECTION_SHUTDOWN,"SHUTDOWN: Calling ChangeToWindowedMode()");
      ChangeToWindowedMode(true);
    }

    log_to_section(LOGSECTION_SHUTDOWN,"SHUTDOWN: Destroying surfaces");
    DDDestroySurfaces();

    if (DDClipper!=NULL){
      log_to_section(LOGSECTION_SHUTDOWN,"SHUTDOWN: Destroying clipper");
      DDClipper->Release();
      DDClipper=NULL;
    }
    log_to_section(LOGSECTION_SHUTDOWN,"SHUTDOWN: Destroying DD object");
    DDObj->Release();
    DDObj=NULL;
  }
#endif//#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_D3D)
  D3DRelease();
#endif
  palette_remove();
  Method=DISPMETHOD_NONE;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef SHOW_WAVEFORM
#pragma message("you shouldn't read this")
void SteemDisplay::DrawWaveform()
{
  HDC dc;
  if (Method==DISPMETHOD_DD){
    if (DDBackSur->GetDC(&dc)!=DD_OK) return;
  }else if (Method==DISPMETHOD_GDI){
    dc=GDIBmpDC;
  }else{
    return;
  }

  int base=shifter_y-10;
  SelectObject(dc,GetStockObject((STpal[0]<0x777) ? WHITE_PEN:BLACK_PEN));
  MoveToEx(dc,0,base-129,0);
  LineTo(dc,shifter_x,base-129);
  MoveToEx(dc,0,base+1,0);
  LineTo(dc,shifter_x,base+1);
  MoveToEx(dc,0,base - temp_waveform_display[0]/2,0);
  for (int x=0;x<draw_blit_source_rect.right;x++){
    LineTo(dc,x,base - temp_waveform_display[x*SHOW_WAVEFORM]/2);
  }
  MoveToEx(dc,temp_waveform_play_counter/SHOW_WAVEFORM,0,0);
  LineTo(dc,temp_waveform_play_counter/SHOW_WAVEFORM,shifter_y);

  if (Method==DISPMETHOD_DD) DDBackSur->ReleaseDC(dc);
}
#endif
//---------------------------------------------------------------------------
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
void SteemDisplay::ScreenShotCheckFreeImageLoad()
{

  //SS the library is loaded only when needed

  if ((ScreenShotFormat>FIF_BMP && ScreenShotFormat<IF_TOCLIPBOARD) || ScreenShotFormatOpts){
    if (hFreeImage) return;

    hFreeImage=LoadLibrary(RunDir+"\\FreeImage\\FreeImage.dll");
    if (hFreeImage==NULL) hFreeImage=LoadLibrary(RunDir+"\\FreeImage\\FreeImage\\FreeImage.dll");
    if (hFreeImage==NULL) hFreeImage=LoadLibrary("FreeImage.dll");
#ifdef SSE_DEBUG
    if (hFreeImage==NULL)
      TRACE_LOG("FreeImage.dll not available\n");   
#endif
    if (hFreeImage==NULL) return;
#if defined(SSE_VID_FREEIMAGE4)
    FreeImage_Free=(FI_FREEPROC)GetProcAddress(hFreeImage,"_FreeImage_Free@4");
#else
    FreeImage_Initialise=(FI_INITPROC)GetProcAddress(hFreeImage,"_FreeImage_Initialise@4");
    FreeImage_DeInitialise=(FI_DEINITPROC)GetProcAddress(hFreeImage,"_FreeImage_DeInitialise@0");
    FreeImage_ConvertFromRawBits=
          (FI_CONVFROMRAWPROC)GetProcAddress(hFreeImage,"_FreeImage_ConvertFromRawBits@36");
    FreeImage_FIFSupportsExportBPP=
          (FI_SUPPORTBPPPROC)GetProcAddress(hFreeImage,"_FreeImage_FIFSupportsExportBPP@8");
    FreeImage_Save=(FI_SAVEPROC)GetProcAddress(hFreeImage,"_FreeImage_Save@16");
    FreeImage_Free=(FI_FREEPROC)GetProcAddress(hFreeImage,"_FreeImage_Free@4");
#endif

#if defined(SSE_VID_FREEIMAGE1) //3.6.3 init library
    if(!FreeImage_Free) // breaking change!
      FreeImage_Free=(FI_FREEPROC)GetProcAddress(hFreeImage,"_FreeImage_Unload@4");
#endif

    if (FreeImage_Initialise==NULL || FreeImage_DeInitialise==NULL ||
          FreeImage_ConvertFromRawBits==NULL || FreeImage_Save==NULL ||
          FreeImage_FIFSupportsExportBPP==NULL || FreeImage_Free==NULL){
      FreeLibrary(hFreeImage);hFreeImage=NULL;
      return;
    }
    FreeImage_Initialise(TRUE);
  }else{
    if (hFreeImage==NULL) return;

    FreeImage_DeInitialise();
    FreeLibrary(hFreeImage);hFreeImage=NULL;
  }
#ifdef SSE_DEBUG
  if(hFreeImage)
    TRACE_LOG("FreeImage.dll loaded\n");
#endif
}
#endif//#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
//---------------------------------------------------------------------------
#endif //WIN32
#pragma warning (disable: 4701) //SurLineLen
HRESULT SteemDisplay::SaveScreenShot()
{
  Str ShotFile=ScreenShotNextFile;
  ScreenShotNextFile="";
  bool ToClipboard=(int(ScreenShotFormat)==IF_TOCLIPBOARD);

  if (ShotFile.Empty() && ToClipboard==0){

    DWORD Attrib=GetFileAttributes(ScreenShotFol);
    if (Attrib==0xffffffff || (Attrib & FILE_ATTRIBUTE_DIRECTORY)==0) return DDERR_GENERIC;

#if defined(SSE_VID_D3D_SCREENSHOT_391C)
    Str Exts=ScreenShotExt;
#else
    Str Exts="bmp";
    ASSERT(ScreenShotExt.Text!=NULL);
    WIN_ONLY( if (hFreeImage) Exts=ScreenShotExt; )
#endif
#if defined(SSE_VID_SAVE_NEO)
    if(ScreenShotFormat==IF_NEO)
      Exts="NEO";
#endif
    EasyStr FirstWord="Steem_";
    if (FloppyDrive[0].DiskName.NotEmpty()){
      FirstWord=FloppyDrive[0].DiskName;
      if (ScreenShotUseFullName==0){
        char *spc=strchr(FirstWord,' ');
        if (spc) *spc=0;
      }
    }
    bool AddNumExt=true;
    if (ScreenShotUseFullName){
      ShotFile=ScreenShotFol+SLASH+FirstWord+"."+Exts;
      if (Exists(ShotFile)==0) AddNumExt=ScreenShotAlwaysAddNum;
    }
    if (AddNumExt){
      int Num=0;
      do{
#if defined(SSE_VID_SCREENSHOTS_NUMBER)
        if (++Num >= 100000) return DDERR_GENERIC;
        ShotFile=ScreenShotFol+SLASH+FirstWord+"_"+(EasyStr("00000")+Num).Rights(5)+"."+Exts;
#else
        if (++Num >= 1000) return DDERR_GENERIC;
        ShotFile=ScreenShotFol+SLASH+FirstWord+"_"+(EasyStr("000")+Num).Rights(3)+"."+Exts;
#endif
      }while (Exists(ShotFile));
    }
  }

  BYTE *SurMem=NULL;
  long SurLineLen;
  int w,h;

#ifdef WIN32

#if defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_D3D_SCREENSHOT_391B)
  IDirect3DSurface9 *BackBuff=NULL;
#endif
  IDirect3DSurface9 *SaveSur=NULL;
#else
#if defined(SSE_VID_DD7)
  IDirectDrawSurface7 *SaveSur=NULL;
#else
  IDirectDrawSurface *SaveSur=NULL;
#endif
#endif//#if !defined(SSE_VID_D3D_ONLY)
  HBITMAP SaveBmp=NULL;

  // Need to create new surfaces so we can blit in the same way we do to the
  // window, just in case image must be stretched. We can't do this ourselves
  // (even if we wanted to) because some video cards will blur.

#if defined(SSE_VID_D3D_ONLY) && defined(SSE_VID_SAVE_NEO)
  //v3.8.2: moved up again...
  if(ScreenShotFormat==IF_NEO && !ToClipboard)
  {
    if(pNeoFile)
    {
      pNeoFile->resolution=change_endian(screen_res);//bugfix v...
      // palette was already copied (sooner=better)
      for(int i=0;i<16000;i++)
        pNeoFile->data[i]=change_endian(DPEEK(xbios2+i*2));
      FILE *f=fopen(ShotFile,"wb");
      if(f)
      {
        fwrite(pNeoFile,sizeof(neochrome_file),1,f);
        fclose(f);
      }
      delete pNeoFile;
      pNeoFile=NULL;
      return 0;
    }
  }
#endif

  if (Method==DISPMETHOD_DD){
#if defined(SSE_VID_D3D_ONLY)
/*  If we want a D3D-only build, we must offer a screenshot facility as well.
    Code seems more compact.
*/
    if (!pD3D||!pD3DDevice) 
      return DDERR_GENERIC;
    HRESULT hRet;
    UINT Adapter=D3DADAPTER_DEFAULT;
    D3DDISPLAYMODE d3ddm;
    if((hRet=pD3D->GetAdapterDisplayMode(Adapter, &d3ddm))!=D3D_OK)
    {
      REPORT_D3D_ERR("GetAdapterDisplayMode",hRet);
      return hRet;
    }
#if defined(SSE_VID_D3D_SCREENSHOT_390) // bugfix stretch mode
    w=draw_blit_source_rect.right-draw_blit_source_rect.left;
    h=draw_blit_source_rect.bottom-draw_blit_source_rect.top;
#else
    w=SurfaceWidth;
    h=SurfaceHeight;
#endif
    ASSERT((w) && (h));

#if defined(SSE_VID_D3D_SCREENSHOT_391B)
/*  Source = BackBuff, Destination = SaveSur
    We just get a pointer to the back buffer.
*/
    if((hRet=pD3DDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&BackBuff))!=0)
    {
      REPORT_D3D_ERR("GetBackBuffer",hRet);
      return hRet;
    }
/*  If the target surface is a plain surface, we can't use StretchRect on it,
    so we use CreateRenderTarget() instead.
    TRUE for lockable, necessary for FreeImage.
*/
    if((hRet=pD3DDevice->CreateRenderTarget(w,h,d3ddm.Format,D3DMULTISAMPLE_NONE,
      0,TRUE,&SaveSur,NULL))!=D3D_OK)
    {
      REPORT_D3D_ERR("CreateRenderTarget",hRet);
      return hRet;
    }
#else
    if((hRet=pD3DDevice->CreateOffscreenPlainSurface(w,h,d3ddm.Format,
      D3DPOOL_SYSTEMMEM,&SaveSur,NULL))!=0)//390
    {
      REPORT_D3D_ERR("CreateOffscreenPlainSurface1",hRet);
      return hRet;
    }
    if((hRet=pD3DDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&SaveSur))!=0)//390
    {
      SaveSur->Release();
      return hRet;
    }
#endif
#if defined(SSE_VID_D3D_SCREENSHOT_391B)
    RECT rcDest={0,0,0,0};
    rcDest.right=w;
    rcDest.bottom=h;
    if(ScreenShotMinSize) // option
    {
      if (border & 1){
        rcDest.right=WinSizeBorder[screen_res][0].x;
        rcDest.bottom=WinSizeBorder[screen_res][0].y;
      }else{
        rcDest.right=WinSize[screen_res][0].x;
        rcDest.bottom=WinSize[screen_res][0].y;
      }
    }
    // copy source->destination
    if((hRet=pD3DDevice->StretchRect(BackBuff,&draw_blit_source_rect,SaveSur,&rcDest,D3DTEXF_NONE))!=DS_OK)
    {
      REPORT_D3D_ERR("StretchRect",hRet);
      // fall back on making SaveSur the back buffer
      if((hRet=pD3DDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&SaveSur))!=D3D_OK)
      {
        REPORT_D3D_ERR("StretchRect",hRet);
      }
    }
    if(BackBuff)
      BackBuff->Release();
    w=rcDest.right;h=rcDest.bottom;
#endif
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
    if(hFreeImage)
    {
      D3DLOCKED_RECT LockedRect;
      if((hRet=SaveSur->LockRect(&LockedRect,NULL,0))!=0)//390
      {
        REPORT_D3D_ERR("LockRect",hRet);
        SaveSur->Release();
        return hRet;
      }
      SurLineLen=LockedRect.Pitch;
      SurMem=(BYTE*)LockedRect.pBits;
      ASSERT(SurLineLen);
      ASSERT(SurMem);
    }
    else
#endif//#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
    {
#if defined(SSE_VID_D3D_SCREENSHOT_391C) // PNG, JPG available natively now...
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
      D3DXIMAGE_FILEFORMAT fileformat=D3DXIFF_BMP;
      switch(ScreenShotFormat)
      { //note the function can't save in tga or ppm format
      case FIF_JPEG:
        fileformat=D3DXIFF_JPG;
        break;
      case FIF_PNG:
        fileformat=D3DXIFF_PNG;
        break;
      }
      hRet=D3DXSaveSurfaceToFile(ShotFile,fileformat,SaveSur,NULL,&rcDest);
#else
      hRet=D3DXSaveSurfaceToFile(ShotFile,(D3DXIMAGE_FILEFORMAT)ScreenShotFormat,
        SaveSur,NULL,&rcDest);
#endif
      TRACE_LOG("Save screenshot %s %dx%d native ERR%d\n",ShotFile.Text,w,h,hRet);
#elif defined(SSE_VID_D3D_SCREENSHOT_391B) 
      hRet=D3DXSaveSurfaceToFile(ShotFile,D3DXIFF_BMP,SaveSur,NULL,&rcDest);
#elif defined(SSE_VID_D3D_SCREENSHOT_391) //fixes picture too big when no borders
      hRet=D3DXSaveSurfaceToFile(ShotFile,D3DXIFF_BMP,SaveSur,NULL,&draw_blit_source_rect);
#else
      hRet=D3DXSaveSurfaceToFile(ShotFile,D3DXIFF_BMP,SaveSur,NULL,NULL);
#endif
      SaveSur->Release();
      return hRet;
    }

#else
    if (DDBackSur==NULL) return DDERR_GENERIC;

    RECT rcDest={0,0,0,0};
    if (ScreenShotMinSize){
      if (border & 1){
        rcDest.right=WinSizeBorder[screen_res][0].x;
        rcDest.bottom=WinSizeBorder[screen_res][0].y;
      }else{
        rcDest.right=WinSize[screen_res][0].x;
        rcDest.bottom=WinSize[screen_res][0].y;
      }
    }else{
      if (FullScreen){
        get_fullscreen_rect(&rcDest);
        OffsetRect(&rcDest,-rcDest.left,-rcDest.top);
      }else{
        GetClientRect(StemWin,&rcDest);
        rcDest.right-=4;rcDest.bottom-=4+MENUHEIGHT;
      }
    }
    w=rcDest.right;h=rcDest.bottom;

    HRESULT hRet;
#if defined(SSE_VID_DD7)
    DDSURFACEDESC2 SaveSurDesc;
    ZeroMemory(&SaveSurDesc,sizeof(DDSURFACEDESC2));//and no betatester saw this
    SaveSurDesc.dwSize=sizeof(DDSURFACEDESC2);
#else
    DDSURFACEDESC SaveSurDesc;
    ZeroMemory(&SaveSurDesc,sizeof(DDSURFACEDESC));
    SaveSurDesc.dwSize=sizeof(DDSURFACEDESC);
#endif
    SaveSurDesc.dwFlags=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    SaveSurDesc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    SaveSurDesc.dwWidth=w;
    SaveSurDesc.dwHeight=h;
    hRet=DDObj->CreateSurface(&SaveSurDesc,&SaveSur,NULL);
    if (hRet!=DD_OK) return hRet;

    hRet=SaveSur->Blt(&rcDest,DDBackSur,&draw_blit_source_rect,DDBLT_WAIT,NULL);
    if (hRet!=DD_OK){
      SaveSur->Release();
      return hRet;
    }

    if (SaveSur->IsLost()==DDERR_SURFACELOST){
      SaveSur->Release();
      return hRet;
    }

    if (ToClipboard==0){
      SaveSurDesc.dwSize=sizeof(DDSURFACEDESC);
      hRet=SaveSur->Lock(NULL,&SaveSurDesc,DDLOCK_WAIT | DDLockFlags,NULL);
      if (hRet!=DD_OK){
        SaveSur->Release();
        return hRet;
      }

      SurMem=(BYTE*)SaveSurDesc.lpSurface;
      SurLineLen=SaveSurDesc.lPitch;
    }
#endif//#if defined(SSE_VID_D3D_ONLY)
  }else if (Method==DISPMETHOD_GDI){
    if (GDIBmp==NULL) return DDERR_GENERIC;

    BITMAP BmpInf;
    RECT rcDest;

    GetClientRect(StemWin,&rcDest);
    w=rcDest.right-4;h=rcDest.bottom-(4+MENUHEIGHT);

    HDC dc=GetDC(NULL);
    SaveBmp=CreateCompatibleBitmap(dc,w,h);
    ReleaseDC(NULL,dc);

    HDC SaveBmpDC=CreateCompatibleDC(NULL);
    SelectObject(SaveBmpDC,SaveBmp);
    SetStretchBltMode(SaveBmpDC,COLORONCOLOR);
    StretchBlt(SaveBmpDC,0,0,w,h,GDIBmpDC,draw_blit_source_rect.left,draw_blit_source_rect.top,
                 draw_blit_source_rect.right-draw_blit_source_rect.left,
                 draw_blit_source_rect.bottom-draw_blit_source_rect.top,SRCCOPY);
    DeleteDC(SaveBmpDC);

    if (ToClipboard==0){
      GetObject(SaveBmp,sizeof(BITMAP),&BmpInf);
      SurLineLen=BmpInf.bmWidthBytes;

      try{
        DWORD BmpBytes=SurLineLen*BmpInf.bmHeight;
        SurMem=new BYTE[BmpBytes];
        GetBitmapBits(SaveBmp,BmpBytes,SurMem);
      }catch(...){
        DeleteObject(SaveBmp);
        return DDERR_GENERIC;
      }
    }
  }else{
    return DDERR_GENERIC;
  }
#elif defined(UNIX)
  // No need to create a new surface here, X can't stretch
  if (Method==DISPMETHOD_X || Method==DISPMETHOD_XSHM){
    if (X_Img==NULL) return DDERR_GENERIC;

    w=draw_blit_source_rect.right;
    h=draw_blit_source_rect.bottom;
    SurMem=LPBYTE(X_Img->data);
    SurLineLen=X_Img->bytes_per_line;
  }else{
    return DDERR_GENERIC;
  }
#endif

	BYTE *Pixels=SurMem;
  bool ConvertPixels=true;
#if !defined(SSE_VID_FREEIMAGE2) //3.6.3 always convert pixels
#ifdef WIN32
  if (hFreeImage && ToClipboard==0){
    if (BytesPerPixel>1){
/*
Returns TRUE if the plugin belonging to the given FREE_IMAGE_FORMAT can save a
bitmap in the desired bit depth, FALSE otherwise.
-> Then you had a nice black screen on PNG
*/
      if (FreeImage_FIFSupportsExportBPP((FREE_IMAGE_FORMAT)ScreenShotFormat,BytesPerPixel*8)){
        ConvertPixels=0;
      }
    }
  }
#endif
#endif

  if (ToClipboard){
    ConvertPixels=0;
#ifdef WIN32
    if (Method==DISPMETHOD_DD){
      HDC DDSaveSurDC=NULL;
      HRESULT hRet=SaveSur->GetDC(&DDSaveSurDC);
      if (hRet!=DD_OK){
        SaveSur->Release();
        return hRet;
      }

      HDC dc=GetDC(NULL);
      SaveBmp=CreateCompatibleBitmap(dc,w,h);
      ReleaseDC(NULL,dc);

      HDC SaveBmpDC=CreateCompatibleDC(NULL);

      SelectObject(SaveBmpDC,SaveBmp);
      BitBlt(SaveBmpDC,0,0,w,h,DDSaveSurDC,0,0,SRCCOPY);

      DeleteDC(SaveBmpDC);
      SaveSur->ReleaseDC(DDSaveSurDC);
    }
    if (OpenClipboard(StemWin)){
      EmptyClipboard();

      SetClipboardData(CF_BITMAP,SaveBmp);

      CloseClipboard();
    }
#endif

  }else if (ConvertPixels){
    Pixels=new BYTE[w*h*3 + 16];
    BYTE *pPix=Pixels;
    switch (BytesPerPixel){
      case 1:
      {
        DWORD Col;
        BYTE *pSur=SurMem+((h-1)*SurLineLen),*pSurLineEnd;
        while (pSur>=SurMem){
          pSurLineEnd=pSur+w;
          for (;pSur<pSurLineEnd;pSur++){
            Col=(DWORD)logpal[(*pSur)-1];
            *LPDWORD(pPix)=((Col & 0xff) << 16) | (Col & 0x00ff00) | ((Col & 0xff0000) >> 16);
            pPix+=3;
          }
          pSur-=SurLineLen+w;
        }
        break;
      }
      case 2:
      {
        WORD Col;
        WORD *pSur=LPWORD(SurMem+((h-1)*SurLineLen)),*pSurLineEnd;
        if (rgb555){
          while (LPBYTE(pSur)>=SurMem){
            pSurLineEnd=pSur+w;
            for (;pSur<pSurLineEnd;pSur++){
              Col=*pSur;
              pPix[0]=BYTE((Col << 3) & b11111000);
              pPix[1]=BYTE((Col >> 2) & b11111000);
              pPix[2]=BYTE((Col >> 7) & b11111000);
              pPix+=3;
            }
            pSur=LPWORD(LPBYTE(pSur)-SurLineLen)-w;
          }
        }else{
          while (LPBYTE(pSur)>=SurMem){
            pSurLineEnd=pSur+w;
            for (;pSur<pSurLineEnd;pSur++){
              Col=*pSur;
              pPix[0]=BYTE((Col << 3) & b11111000);
              pPix[1]=BYTE((Col >> 3) & b11111100);
              pPix[2]=BYTE((Col >> 8) & b11111000);
              pPix+=3;
            }
            pSur=LPWORD(LPBYTE(pSur)-SurLineLen)-w;
          }
        }
        break;
      }
      case 3:
      {
        long WidBytes=(w*3+3) & -4;
        BYTE *pSur=SurMem+((h-1)*SurLineLen);
        while (pSur>=SurMem){
          memcpy(pPix,pSur,WidBytes);
          pSur-=SurLineLen;
          pPix+=WidBytes;
        }
        break;
      }
      case 4:
      {
        DWORD *pSur=LPDWORD(SurMem+((h-1)*SurLineLen)),*pSurLineEnd;
        if (rgb32_bluestart_bit){
          while (LPBYTE(pSur)>=SurMem){
            pSurLineEnd=pSur+w;
            for (;pSur<pSurLineEnd;pSur++){
              *LPDWORD(pPix)=(*pSur) >> rgb32_bluestart_bit;
              pPix+=3;
            }
            pSur=LPDWORD(LPBYTE(pSur)-SurLineLen)-w;
          }
        }else{
          while (LPBYTE(pSur)>=SurMem){
            pSurLineEnd=pSur+w;
            for (;pSur<pSurLineEnd;pSur++){
              *LPDWORD(pPix)=*pSur;
              pPix+=3;
            }
            pSur=LPDWORD(LPBYTE(pSur)-SurLineLen)-w;
          }
        }
        break;
      }
#if defined(SSE_VS2008_WARNING_390)
      default:
        NODEFAULT;
#endif
    }
  }

  if (ToClipboard==0){
#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_SAVE_NEO)
    //v3.6.3: moved up so that FreeImage doesn't get it
    if(ScreenShotFormat==IF_NEO)
    {
      if (pNeoFile)
      {
        pNeoFile->resolution=screen_res;
        // palette was already copied (sooner=better)
        for(int i=0;i<16000;i++)
          pNeoFile->data[i]=change_endian(DPEEK(xbios2+i*2));
        FILE *f=fopen(ShotFile,"wb");
        if(f)
        {
          fwrite(pNeoFile,sizeof(neochrome_file),1,f);
          fclose(f);
        }
        delete pNeoFile;
        pNeoFile=NULL;
      }
     }
    else
#endif
#endif//#if defined(SSE_VID_D3D_ONLY)

#ifdef WIN32
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
    if (hFreeImage){
      FIBITMAP *FIBmp;
      if (ConvertPixels==0){
        DWORD r_mask,g_mask,b_mask;
        switch (BytesPerPixel){
          case 2:
            if (rgb555){
              r_mask=MAKEBINW(b11111000,b00000000);
              g_mask=MAKEBINW(b00000111,b11100000);
              b_mask=MAKEBINW(b00000000,b00011111);
            }else{
              r_mask=MAKEBINW(b01111100,b00000000);
              g_mask=MAKEBINW(b00000011,b11100000);
              b_mask=MAKEBINW(b00000000,b00011111);
            }
            break;
          case 3:case 4:
            r_mask=0xff0000 << rgb32_bluestart_bit;
            g_mask=0x00ff00 << rgb32_bluestart_bit;
            b_mask=0x0000ff << rgb32_bluestart_bit;
            break;
#if defined(SSE_VS2008_WARNING_390)
          default:
            NODEFAULT;
#endif
        }

#if defined(SSE_VID_FREEIMAGE3) //3.6.3 flip pic
/*  I don't know if the library changed, but this was doing the 
    opposite of what it should (pics were upside down).
*/
        FIBmp=FreeImage_ConvertFromRawBits(SurMem,w,h,SurLineLen,BytesPerPixel*8,
          r_mask,g_mask,b_mask,true);
#else

        FIBmp=FreeImage_ConvertFromRawBits(SurMem,w,h,SurLineLen,BytesPerPixel*8,                                         
          r_mask,g_mask,b_mask,0);
#endif
      }else{
#if defined(SSE_VID_FREEIMAGE3) //3.6.3 flip pic

        FIBmp=FreeImage_ConvertFromRawBits(Pixels,w,h,w*3,24,
                                          0xff0000,0x00ff00,0x0000ff,false);
#else
        FIBmp=FreeImage_ConvertFromRawBits(Pixels,w,h,w*3,24,
                                          0xff0000,0x00ff00,0x0000ff,true);
#endif
      }
      //TRACE_LOG("ImageFree save %s format %d convert%d\n",ShotFile.Text,ScreenShotFormat,ConvertPixels);
      TRACE_LOG("Save screenshot %s %dx%d opts %d FreeImage\n",ShotFile.Text,w,h,ScreenShotFormatOpts);
      FreeImage_Save((FREE_IMAGE_FORMAT)ScreenShotFormat,FIBmp,ShotFile,ScreenShotFormatOpts);
      FreeImage_Free(FIBmp);
    }else
#endif//#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
#endif
    {
      BITMAPINFOHEADER bih;

      bih.biSize=sizeof(BITMAPINFOHEADER);
      bih.biWidth=w;
      bih.biHeight=h;
      WIN_ONLY(	bih.biPlanes=1; )
      WIN_ONLY(	bih.biBitCount=24; )
      UNIX_ONLY( bih.biPlanes_biBitCount=MAKELONG(1,24); )
      bih.biCompression=0 /*BI_RGB*/;
      bih.biSizeImage=0;
      bih.biXPelsPerMeter=0;
      bih.biYPelsPerMeter=0;
      bih.biClrUsed=0;
      bih.biClrImportant=0;

      FILE *f=fopen(ShotFile,"wb");
      if (f){
        // File header
        WORD bfType=19778; //'BM';
        DWORD bfSize=14 /*sizeof(BITMAPFILEHEADER)*/ + sizeof(BITMAPINFOHEADER)+(w*h*3);
        WORD bfReserved1=0;
        WORD bfReserved2=0;
        DWORD bfOffBits=14 /*sizeof(BITMAPFILEHEADER)*/ + sizeof(BITMAPINFOHEADER);

        fwrite(&bfType,sizeof(bfType),1,f);
        fwrite(&bfSize,sizeof(bfSize),1,f);
        fwrite(&bfReserved1,sizeof(bfReserved1),1,f);
        fwrite(&bfReserved2,sizeof(bfReserved2),1,f);
        fwrite(&bfOffBits,sizeof(bfOffBits),1,f);
        fflush(f);

        fwrite(&bih,sizeof(bih),1,f);
        fflush(f);
        fwrite(Pixels,w*h*3,1,f);
        fflush(f);
        fclose(f);
      }
    }
  }

#ifdef WIN32
  if (Method==DISPMETHOD_DD){
#if defined(SSE_VID_D3D_ONLY)
    if (ToClipboard==0) SaveSur->UnlockRect();
#else
    if (ToClipboard==0) SaveSur->Unlock(NULL);
#endif
    SaveSur->Release(); // SS DD or D3D
  }else if (Method==DISPMETHOD_GDI){
    delete[] SurMem;
  }
  if (SaveBmp) DeleteObject(SaveBmp);
#endif

  if (ConvertPixels) delete[] Pixels;

  return DD_OK;
}
#pragma warning (default: 4701)
//---------------------------------------------------------------------------
#ifdef IN_MAIN
#ifdef WIN32
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
bool SteemDisplay::ScreenShotIsFreeImageAvailable()
{
  if (hFreeImage) return true;

  Str Path;
  Path.SetLength(MAX_PATH);
  char *FilNam;
  if (SearchPath(NULL,"FreeImage.dll",NULL,MAX_PATH,Path.Text,&FilNam)>0) return true;
  if (Exists(RunDir+"\\FreeImage\\FreeImage.dll")) return true;
  if (Exists(RunDir+"\\FreeImage\\FreeImage\\FreeImage.dll")) return true;
  return 0;
}
#endif//#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
//---------------------------------------------------------------------------
void SteemDisplay::ScreenShotGetFormats(EasyStringList *pSL)
{
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  bool FIAvailable=ScreenShotIsFreeImageAvailable();
#endif
  pSL->Sort=eslNoSort;
  pSL->Add(T("To Clipboard"),IF_TOCLIPBOARD);
  pSL->Add("BMP",FIF_BMP);
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  if (FIAvailable){
    pSL->Add("JPEG (.jpg)",FIF_JPEG);
    pSL->Add("PNG",FIF_PNG);
    pSL->Add("TARGA (.tga)",FIF_TARGA);
    pSL->Add("TIFF",FIF_TIFF);
#if !defined(SSE_VID_FREEIMAGE3) //3.6.3 remove WBMP
    // it just crashes
    pSL->Add("WBMP",FIF_WBMP);
#endif
    pSL->Add("PBM",FIF_PBM);
    pSL->Add("PGM",FIF_PGM);
    pSL->Add("PPM",FIF_PPM);
  }
#endif//#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
#if defined(SSE_VID_D3D_SCREENSHOT_391C)
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
  else 
#endif
  {
    pSL->Add("JPEG (.jpg)",FIF_JPEG);
    pSL->Add("PNG",FIF_PNG);
  }
#endif
#if defined(SSE_VID_SAVE_NEO)
  pSL->Add("NEO",IF_NEO);
#endif
}
//---------------------------------------------------------------------------
#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
void SteemDisplay::ScreenShotGetFormatOpts(EasyStringList *pSL)
{
  bool FIAvailable=ScreenShotIsFreeImageAvailable();
  pSL->Sort=eslNoSort;
  switch (ScreenShotFormat){
    case FIF_BMP:
      if (FIAvailable){
        pSL->Add(T("Normal"),BMP_DEFAULT);
        pSL->Add("RLE",BMP_SAVE_RLE);
      }
      break;
    case FIF_JPEG:
      pSL->Add(T("Superb Quality"),JPEG_QUALITYSUPERB);
      pSL->Add(T("Good Quality"),JPEG_QUALITYGOOD);
      pSL->Add(T("Normal"),JPEG_QUALITYNORMAL);
      pSL->Add(T("Average Quality"),JPEG_QUALITYAVERAGE);
      pSL->Add(T("Bad Quality"),JPEG_QUALITYBAD);
      break;
    case FIF_PBM:case FIF_PGM:case FIF_PPM:
      pSL->Add(T("Binary"),PNM_SAVE_RAW);
      pSL->Add("ASCII",PNM_SAVE_ASCII);
      break;
  }
}
#endif//#if !defined(SSE_VID_D3D_NO_FREEIMAGE)
//---------------------------------------------------------------------------
#if defined(SSE_VID_3BUFFER_WIN) \
 && !defined(SSE_VID_3BUFFER_NO_VSYNC)
/*  When the option is on, this function is called a lot
    during emulation (each scanline) and during VBL idle
    times too, so the processor is always busy. TODO
    Scrolling is also sketchy, triple buffering only removes
    tearing.
    SSE_VID_3BUFFER_NO_VSYNC: this isn't defined, it was used
    to test what happens when we use Triple Buffering but don't
    sync on monitor VBlank: there's still some tearing.
    If we combine with "normal" VSync, it gets smooth, but
    at monitor speed, and triple buffering is useless anyway.
*/
BOOL SteemDisplay::BlitIfVBlank() {
  BOOL Blanking=FALSE;  
#if !defined(SSE_VID_D3D_ONLY)
  if(Disp.DDObj && ACT-Disp.VSyncTiming>80000-60000) // avoid bursts
  {
    Disp.DDObj->GetVerticalBlankStatus(&Blanking);
    if(Blanking)
    {
      Disp.VSyncTiming=ACT;
      draw_blit();
    }
  }
#endif
  return Blanking;
}

#endif
#endif
#endif
//---------------------------------------------------------------------------
void draw_init_resdependent()
{
  if (draw_grille_black<4) draw_grille_black=4;
  make_palette_table(brightness,contrast);
  palette_convert_all();
  if (BytesPerPixel==1) palette_copy();
  if (osd_plasma_pal){
    delete[] osd_plasma_pal; osd_plasma_pal=NULL;
    delete[] osd_plasma;     osd_plasma=NULL;
  }
}
//---------------------------------------------------------------------------

#if defined(SSE_VID_UTIL)
#ifdef WIN32
// miscellaneous functions

//TODO it's just lowres?
int SteemDisplay::STXPixels() {
  int st_x_pixels=320+(border&1)*SideBorderSizeWin*2; //displayed
#if defined(SSE_VID_D3D_391B)
  if(screen_res)
    st_x_pixels*=2;
#endif
  return st_x_pixels;
}


int SteemDisplay::STYPixels() {
  int st_y_pixels=200+(border&1)*(BORDER_TOP+BORDER_BOTTOM);
#if defined(SSE_VID_D3D_391B)
  if(screen_res)
    st_y_pixels*=2;
#endif
  return st_y_pixels;
}
#endif
#endif

//---------------------------------------------------------------------------
#if defined(SSE_VID_D3D)
/*  Direct3D support

    D3D support was introduced in v3.7.0, only for fullscreen.

    As of v3.8.2 there are two separate builds for DirectDraw support
    and for Direct3D support, both windowed and fullscreen modes.
    This makes options (a little) simpler.

    We use DirectX9 and the ID3DXSprite interface.

    Debug build: we avoid ASSERT because it can be trouble when failing
    in fullscreen mode.
   
    TODO: D3D Window could be useful for video capture
    TODO: filters (but looks complicated in WinUAE for so so results?)
 */




#pragma warning( disable : 4701) //OldCur, 390

bool SteemDisplay::D3DBlit() {
  //ASSERT(pD3D);
  HRESULT d3derr=E_FAIL;

  if(pD3DDevice && pD3DSprite)
  {
#ifdef SSE_VID_D3D_WINDOW
    RECT dest;
    HCURSOR OldCur;
    if(!FullScreen)
    {
      if (stem_mousemode==STEM_MOUSEMODE_DISABLED && BlitHideMouse) 
        OldCur=SetCursor(NULL);
      GetClientRect(StemWin,&dest);
      dest.top+=MENUHEIGHT;
      dest.right-=4;dest.bottom-=4;
      POINT pt={2,2};
      OffsetRect(&dest,pt.x,pt.y);
    }
#endif
    d3derr=pD3DDevice->BeginScene();
    d3derr=pD3DSprite->Begin(0); // the picture is one big sprite
#if defined(SSE_VID_D3D_CRISP)
#if defined(SSE_VID_D3D_CRISP_OPTION)
    if(OPTION_D3D_CRISP)
#endif
      pD3DDevice->SetSamplerState(0,D3DSAMP_MAGFILTER ,D3DTEXF_POINT); //v3.7.2
#endif
    
#if defined(SSE_VID_D3D_CRISP_391) 
   // TRACE_LOG("%d %d\n",screen_res,draw_win_mode[screen_res]);
    //TRACE_LOG("src %d %d %d %d dst %d %d %d %d\n",draw_blit_source_rect.left,draw_blit_source_rect.top,draw_blit_source_rect.right,draw_blit_source_rect.bottom,SurfaceWidth,SurfaceHeight,0,0);
    if(FullScreen && OPTION_D3D_CRISP && !SSE_INTERPOLATE
      && (screen_res<2 && draw_win_mode[screen_res]!=DWM_GRILLE))
      d3derr=pD3DSprite->Draw(pD3DTexture,&draw_blit_source_rect,NULL,NULL,0xFFFFFFFF);
    else
      d3derr=pD3DSprite->Draw(pD3DTexture,NULL,NULL,NULL,0xFFFFFFFF);
#elif defined(SSE_VID_D3D_CRISP_390) 
    // there could be trash in fullscreen crisp mode when switching shift modes
    d3derr=pD3DSprite->Draw(pD3DTexture,&draw_blit_source_rect,NULL,NULL,0xFFFFFFFF);
#else
    d3derr=pD3DSprite->Draw(pD3DTexture,NULL,NULL,NULL,0xFFFFFFFF);
#endif
    d3derr=pD3DSprite->End();
    d3derr=pD3DDevice->EndScene();
#ifdef SSE_VID_D3D_WINDOW
    if(!FullScreen)
    {
      d3derr=pD3DDevice->Present(&draw_blit_source_rect,&dest,NULL,NULL);
      if (stem_mousemode==STEM_MOUSEMODE_DISABLED && BlitHideMouse) 
        SetCursor(OldCur);
    }
    else
#endif
      d3derr=pD3DDevice->Present(NULL,NULL,NULL,NULL);
  }

#if defined(SSE_VID_D3D_382)
  if(d3derr) 
  {
    TRACE("BLIT ERROR\n");
    TRACE_LOG("Blit error %d\n",d3derr);
    REPORT_D3D_ERR("Blit",d3derr);
#if defined(SSE_GUI_STATUS_BAR_ALERT)
    M68000.ProcessingState=TM68000::BLIT_ERROR;
#if defined(SSE_VID_D3D_390B)
    GUIRefreshStatusBar(true); // make sure player sees it
#endif
#endif
    runstate=RUNSTATE_STOPPING; // player can save & quit
  }
#elif defined(SSE_DEBUG)
  if(d3derr)
    TRACE_LOG("D3D fail blit %d\n",d3derr);
#endif

  return !d3derr;
}

#pragma warning( default : 4701) //OldCur, 390

#if defined(SSE_VID_D3D_382) && !defined(SSE_VID_D3D_390)

void SteemDisplay::Cls() {
  if(pD3DDevice)
    pD3DDevice->Clear(0,0,D3DCLEAR_TARGET,0,0,0);
}

#endif

// local helper for D3DCreateSurfaces()

HRESULT check_device_type(D3DDEVTYPE DeviceType,D3DFORMAT DisplayFormat) {
  HRESULT d3derr;
  //ASSERT(Disp.pD3D);
  d3derr=Disp.pD3D->CheckDeviceType(D3DADAPTER_DEFAULT,DeviceType,DisplayFormat,DisplayFormat,false);
  TRACE_LOG("CheckDeviceType %d format %d err %d\n",DeviceType,DisplayFormat,d3derr);
  return d3derr;
}


HRESULT SteemDisplay::D3DCreateSurfaces() {
/*  The function is long but this is necessary, we can't take hardware
    capabilities for granted, many PC with motherboard video cards would
    fail, for those we use software rendering.
    Too bad all this burden is on our large programmers' shoulders.
*/
  HRESULT d3derr=E_FAIL;

#if defined(SSE_VID_D3D_390) && !defined(BCC_BUILD)
  if(!pD3D)
    goto D3DCreateSurfacesEnd;
#else
  if(!pD3D)
    return d3derr;
#endif
  D3DDestroySurfaces();

  UINT Adapter=D3DADAPTER_DEFAULT;

  // Get the current desktop display info
  D3DDISPLAYMODE d3ddm;
  d3derr=pD3D->GetAdapterDisplayMode(Adapter, &d3ddm);
  HDC hdc = GetDC(StemWin);
  WORD bitsperpixel= GetDeviceCaps(hdc, BITSPIXEL); // another D3D shortcoming
  ReleaseDC(StemWin, hdc);
  TRACE_LOG("Screen %dx%d %dhz format %d %dbit err %d\n",d3ddm.Width,d3ddm.Height,d3ddm.RefreshRate,d3ddm.Format,bitsperpixel,d3derr);

  // Probe capacities of video card, starting with desktop mode, HW
  // http://en.wikibooks.org/wiki/DirectX/9.0/Direct3D/Initialization
  D3DFORMAT DisplayFormat=d3ddm.Format;
  D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL;
  
  d3derr=check_device_type(DeviceType,DisplayFormat);
  if(d3derr) // could be "alpha" in desktop format?
  {
    REPORT_D3D_ERR("check_device_type1",d3derr);
    DisplayFormat=D3DFMT_X8R8G8B8; // try X8R8G8B8 format
    d3derr=check_device_type(DeviceType,DisplayFormat);
    if(d3derr) // no HW abilities?
    {
      REPORT_D3D_ERR("check_device_type2",d3derr);
      TRACE_LOG("D3D: poor hardware detected, software rendering\n");
      D3DDEVTYPE DeviceType=D3DDEVTYPE_REF; // try software processing (slow)
      d3derr=check_device_type(DeviceType,DisplayFormat);
    }
  }
#if defined(SSE_VID_D3D_390) && !defined(BCC_BUILD)
  if(d3derr)
    goto D3DCreateSurfacesEnd; //forget it
#else
  if(d3derr)
    return d3derr; //forget it
#endif
  D3DCAPS9 caps;
  d3derr=pD3D->GetDeviceCaps(Adapter,DeviceType,&caps);
  TRACE_LOG("DevCaps $%X HW quality %X err %d\n",caps.DevCaps,caps.DevCaps&(D3DDEVCAPS_HWTRANSFORMANDLIGHT|D3DDEVCAPS_PUREDEVICE),d3derr);

  DWORD vtx_proc;
  if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) {
    vtx_proc = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    if( caps.DevCaps & D3DDEVCAPS_PUREDEVICE ) {
      vtx_proc |= D3DCREATE_PUREDEVICE;
    }
  } else {
    vtx_proc = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
  }

#if defined(SSE_VID_D3D_ONLY) // not sure it makes much sense but...
  if (DrawToVidMem==0) 
    vtx_proc|=D3DDEVCAPS_EXECUTESYSTEMMEMORY|D3DDEVCAPS_TEXTURESYSTEMMEMORY;
#endif

  D3DPRESENT_PARAMETERS d3dpp; 
  ZeroMemory(&d3dpp, sizeof(d3dpp));
  d3dpp.Windowed=!FullScreen;
#if defined(SSE_VID_FS_GUI_OPTION)
  d3dpp.SwapEffect=((OPTION_FULLSCREEN_GUI||SSE_3BUFFER)&&FullScreen
    ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY);
#else
  d3dpp.SwapEffect=D3DSWAPEFFECT_COPY;
#endif
  d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER|D3DPRESENTFLAG_DEVICECLIP
#if defined(SSE_VID_D3D_382)
    |D3DPRESENT_INTERVAL_ONE//default?
#endif
    ;
  d3dpp.hDeviceWindow=StemWin;
  UINT Width=monitor_width; // default
  UINT Height=monitor_height;
#if defined(SSE_VID_D3D_382)
  d3dpp.BackBufferCount=((SSE_3BUFFER&&FullScreen)?2:1);
#else
  d3dpp.BackBufferCount=1;  
#endif
#if defined(SSE_VID_D3D_LIST_MODES)
#if defined(SSE_VID_D3D_WINDOW)
  if(FullScreen)
  {
#endif
    D3DDISPLAYMODE Mode; 
    pD3D->EnumAdapterModes(Adapter,DisplayFormat,D3DMode,&Mode);
    TRACE_LOG("D3D mode %d %dx%d %dhz format %d\n",D3DMode,Mode.Width,Mode.Height,Mode.RefreshRate,Mode.Format);
    d3dpp.BackBufferFormat=Mode.Format;
    d3dpp.FullScreen_RefreshRateInHz=Mode.RefreshRate;
    d3dpp.BackBufferWidth=Width=Mode.Width;
    d3dpp.BackBufferHeight=Height=Mode.Height;
#if defined(SSE_VID_D3D_WINDOW)
  }
  else
    d3dpp.BackBufferFormat=DisplayFormat;
#endif
#else//SSE_VID_D3D_LIST_MODES
  d3dpp.BackBufferFormat=DisplayFormat; 
  d3dpp.FullScreen_RefreshRateInHz=d3ddm.RefreshRate;
#endif
#ifndef SSE_VID_D3D_WINDOW
  d3dpp.BackBufferWidth=Width;
  d3dpp.BackBufferHeight=Height;
#endif
#ifdef SSE_VID_D3D_WINDOW
  // as copied from the DD part, maybe we could optimise
  if(!FullScreen)
  {
#ifndef NO_CRAZY_MONITOR
    if(extended_monitor){
      Width=GetScreenWidth();
      Height=GetScreenHeight();
    }else
#endif
    if (Disp.BorderPossible()){ //SS: GetScreenWidth()>640
#if defined(SSE_VID_BORDERS)
#if defined(SSE_VID_BORDERS_LB_DX)
      Width=640+4* (SideBorderSize); // we draw larger
#else
      Width=640+4* (SideBorderSizeWin);
#endif
      Height=400+2*(BORDER_TOP+BottomBorderSize
#if defined(SSE_VID_ADJUST_DRAWING_ZONE1) && !defined(SSE_VID_D3D_391)
        +1 // hack to fix crash on video emu panic, last 'border' scanline
#endif
        );
#else
      Width=768;
      Height=400+2*(BORDER_TOP+BORDER_BOTTOM);
#endif
    }else{
      Width=640;
      Height=480;
    }
#if defined(SSE_VID_ADJUST_DRAWING_ZONE2)
    draw_blit_source_rect.right=int(Width);
    draw_blit_source_rect.bottom=int(Height);
#else
    if (draw_blit_source_rect.right>=int(Width)){
      draw_blit_source_rect.right=int(Width)-1;
    }
    if (draw_blit_source_rect.bottom>=int(Height)){
      draw_blit_source_rect.bottom=int(Height)-1;
    }
#endif
    SurfaceWidth=d3dpp.BackBufferWidth=Width;
    SurfaceHeight=d3dpp.BackBufferHeight=Height;
  }
#endif
  d3derr=pD3D->CreateDevice(D3DADAPTER_DEFAULT,DeviceType,StemWin,vtx_proc,&d3dpp,&pD3DDevice);
  TRACE_LOG("D3D %s CreateDevice %dx%d %dhz vertex %X buffers %d flags %X err %d\n",
    (FullScreen?"fullscreen":"window"),Width,Height,d3dpp.FullScreen_RefreshRateInHz,vtx_proc,
    d3dpp.BackBufferCount,d3dpp.Flags,d3derr);

  //TRACE2("%s %dx%d %dbit %dhz %d buffers Vtx $%X Flags $%X\n",
  //  (FullScreen?"FS":"Win"),Width,Height,bitsperpixel,d3dpp.FullScreen_RefreshRateInHz,
  //  d3dpp.BackBufferCount,vtx_proc,d3dpp.Flags);


  if(!pD3DDevice)
  {
#if defined(SSE_VID_D3D_390)  && !defined(BCC_BUILD)
    goto D3DCreateSurfacesEnd;
#else
    REPORT_D3D_ERR("CreateSurfaces",d3derr);
    return d3derr; // lost cause
#endif
  }
#if !defined(SSE_VID_D3D_NO_GUI)
  d3derr=pD3DDevice->SetDialogBoxMode(TRUE); //changes nothing?
#endif
  // Create texture
#if defined(SSE_VID_D3D_382) // maybe it changes nothing
  d3derr=pD3DDevice->CreateTexture(Width,Height,1,D3DUSAGE_DYNAMIC,
    d3dpp.BackBufferFormat,D3DPOOL_DEFAULT,&pD3DTexture,NULL);
#else
  UINT Levels=1;
  D3DFORMAT Format=d3dpp.BackBufferFormat;
  DWORD Usage=D3DUSAGE_DYNAMIC;
  D3DPOOL Pool=D3DPOOL_DEFAULT;
  d3derr=pD3DDevice->Clear(0,0,D3DCLEAR_TARGET,0,0,0);//XP-intel
  HANDLE *pSharedHandle=NULL; //always
  d3derr=pD3DDevice->CreateTexture(Width,Height,Levels,Usage,Format,
    Pool,&pD3DTexture,pSharedHandle);
  TRACE_LOG("D3D CreateTexture %dx%d levels %d Usage %d Format %d Pool %d Err %d\n",
    Width,Height,Levels,Usage,Format,Pool,d3derr);
#endif

  if(!pD3DTexture)
  {
#if defined(SSE_VID_D3D_390)  && !defined(BCC_BUILD)
    goto D3DCreateSurfacesEnd;
#else
    REPORT_D3D_ERR("CreateSurfaces",d3derr);
    return d3derr;
#endif
  }

  BytesPerPixel= bitsperpixel/8; // Steem can do 8bit, 16bit, 32bit

#ifdef SSE_VID_D3D_WINDOW
  if(FullScreen)
#endif
  {
    SurfaceWidth=d3dpp.BackBufferWidth;
    SurfaceHeight=d3dpp.BackBufferHeight;
  }

  D3DSpriteInit();

  draw_init_resdependent();
  palette_prepare(true);
#if defined(SSE_VID_D3D_390) && !defined(BCC_BUILD)
D3DCreateSurfacesEnd:
  if(d3derr!=D3D_OK)
  {
    REPORT_D3D_ERR("CreateSurfaces",d3derr);
    TRACE2("D3D_ERR %d\n",d3derr);
  }
#endif
  return d3derr;
}


VOID SteemDisplay::D3DDestroySurfaces() {
  TRACE_LOG("D3D destroy surfaces S %x T %x D %x\n",pD3DSprite,pD3DTexture,pD3DDevice);
  if(pD3DSprite)
  {
    pD3DSprite->Release();
    pD3DSprite=NULL;
  }
  if (pD3DTexture)
  {
    pD3DTexture->Release();
    pD3DTexture=NULL;
  }

  if (pD3DDevice)
  {
    pD3DDevice->Release();
    pD3DDevice = NULL;
  }
}


HRESULT SteemDisplay::D3DInit()
{
  SetNotifyInitText("DirectD3D");
  //ASSERT(!pD3D);
  if(pD3D)
    pD3D->Release();
  // Create the D3D object - computer needs DirectX9
  if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) 
  {
    TRACE_LOG("D3D9 Init Fail!\n");
    return E_FAIL; 
  }
  TRACE_LOG("D3D9 Init OK\n");
  return S_OK;
}


HRESULT SteemDisplay::D3DLock() {
  HRESULT d3derr=E_FAIL;
  if(pD3DTexture)
  {
    D3DLOCKED_RECT LockedRect;
    d3derr=pD3DTexture->LockRect(0,&LockedRect,NULL,0);
#if defined(SSE_VID_D3D_382)
    if(d3derr)
      REPORT_D3D_ERR("LockRect",d3derr);
    else
    {
#endif
      draw_line_length=LockedRect.Pitch;
      draw_mem=(BYTE*)LockedRect.pBits;
#if !defined(SSE_VID_D3D_STRETCH_FORCE)
      // straight blit at the centre of the screen, for stretch it's arranged
      // in SpriteInit
      if(draw_fs_blit_mode==DFSM_STRAIGHTBLIT||draw_fs_blit_mode==DFSM_FLIP)
      {
        int st_x_pixels=2*STXPixels();//? med?
        int st_y_pixels=2*STYPixels();
        draw_mem+=draw_line_length*((SurfaceHeight-st_y_pixels)/2-1);
        draw_mem+=BytesPerPixel*((SurfaceWidth-st_x_pixels)/2-1);
      }
#endif
#if defined(SSE_VID_D3D_382)
    }
#endif

  }
  return d3derr;
}


VOID SteemDisplay::D3DRelease()
{
  // called by void SteemDisplay::Release()
  TRACE_LOG("D3DRelease()\n");

  D3DDestroySurfaces(); // destroys texture, sprite, device
  if (pD3D)
  {
    pD3D->Release();
    pD3D = NULL;
#if defined(SSE_VID_D3D_373)
    D3D9_OK=false;
#endif
  }
}


void SteemDisplay::D3DUnlock() {
  HRESULT d3derr=E_FAIL;
  if(pD3DTexture)
    d3derr=pD3DTexture->UnlockRect(0);
  return;
}


HRESULT SteemDisplay::D3DSpriteInit() {
  // called by D3DCreateSurfaces() and by ...
  // ScreenChange() calls DDCreateSurfaces()
  HRESULT hr=E_FAIL;
#if defined(SSE_VID_D3D_373)
  //ASSERT(pD3D);
  if(!pD3D)
    return hr;
#endif

  if(pD3DSprite)
    pD3DSprite->Release(); //so we can init sprite anytime
  hr = D3DXCreateSprite(pD3DDevice,&pD3DSprite); 
  if(!pD3DSprite)
    return hr;
#if defined(SSE_VID_D3D_STRETCH)
/*  Use SetTransform to stretch & translate if needs be, restore if not.
    We will draw small on the big backbuffer and this will be stretched and
    translated by the hardware before rendering.
*/
  float sw=1,sh=1;
  float tx=0,ty=0;
#if !defined(SSE_VID_D3D_STRETCH_FORCE) && !defined(SSE_VID_D3D_ONLY)
  if(draw_fs_blit_mode==DFSM_STRETCHBLIT || draw_fs_blit_mode==DFSM_LAPTOP)
#endif
  {
    int stx=STXPixels();
    int sty=STYPixels();

#ifndef NO_CRAZY_MONITOR
#ifdef SSE_VID_EXT_FS1
    if(extended_monitor
#if defined(SSE_TOS_GEMDOS_EM_390)
      && em_width && em_height
#endif
      )
    {
      stx=em_width; // that's it
      sty=em_height;
    }
#endif
#endif
#if defined(SSE_VID_D3D_STRETCH_ASPECT_RATIO) 
/*  Imitating a feature first seen in SainT, the screen is higher than it
    should, so that circles aren't perfect, squares are rectangles, etc.
    Note: you could use some settings on your monitor to cancel this effect,
    but many magazine screenshots show it.
    On an American display, 60hz, the picture is better, so Atari wasn't really
    aware of the problem.

    Some references:
    TV AR 4:3 = 1.333
    PAL 720:576 = 1.25
    NTSC 720:480 = 1.5
    ST AR LORES
      320:200 = 1.6 (useful picture)
      416:281 = 1.480 (plasma)
*/
#if defined(SSE_VID_D3D_STRETCH_AR_OPTION)
    if(OPTION_ST_ASPECT_RATIO)
#endif
      sty*=ST_ASPECT_RATIO_DISTORTION; // "reserve" more pixels
#endif

    TRACE_LOG("D3D Sprite STX %d STY %d Surface W %d H %d res %d mixed %d 16/9 %d\n",
      stx,sty,SurfaceWidth,SurfaceHeight,screen_res,mixed_output,
      (SurfaceWidth>(stx*SurfaceHeight)/sty));

#ifdef SSE_VID_D3D_WINDOW
    if(FullScreen)
#endif
    {
      if(SurfaceWidth>(stx*SurfaceHeight)/sty) // "16:9" - not tested
      {
        sw=sh=(float)SurfaceHeight/sty;
        tx=(SurfaceWidth-SurfaceHeight*stx/sty)/2;
      }
      else // "4:3"
      {
        sh=sw= (float)SurfaceWidth/stx;
        ty=(SurfaceHeight-SurfaceWidth*sty/stx)/2;
      }
    

#if defined(SSE_VID_D3D_CRISP)
/*  Trade-off, we sacrifice some screen space to have better proportions.
    v3.7.2
*/
#if defined(SSE_VID_D3D_CRISP_OPTION)
#if defined(SSE_VID_D3D_WINDOW) && !defined(SSE_VID_D3D_391B)
    if(OPTION_D3D_CRISP && FullScreen)
#else
    if(OPTION_D3D_CRISP)
#endif
#endif
    {
      sw=sh=(int)(sw); // no artefacts
      tx= (SurfaceWidth-(sw*stx))/2; // correct translation (on my 4:3 anyway)
      ty= (SurfaceHeight-(sw*sty))/2;
    }
#endif

#if defined(SSE_VID_D3D_391B)
    ASSERT(FullScreen);
    if(screen_res>=1||mixed_output) // everything but low
      sh*=2; // double # lines
#endif
  }
#if defined(SSE_VID_D3D_STRETCH_ASPECT_RATIO) 
#if defined(SSE_VID_D3D_STRETCH_AR_OPTION)
    if(OPTION_ST_ASPECT_RATIO)
#endif
#ifdef SSE_VID_D3D_WINDOW
      if(FullScreen)
#endif
        sh*=ST_ASPECT_RATIO_DISTORTION; // stretch more
#endif

    // resolution adjustments, a bit ad hoc for now...
#ifdef SSE_VID_D3D_WINDOW
    if(FullScreen)
#endif
    {
#if !defined(SSE_VID_D3D_391B)
      if(screen_res ||mixed_output)
        sw/=2;
#endif
      if(screen_res>=2 ||SCANLINES_INTERPOLATED
#if defined(SSE_VID_D3D_382)
        || FullScreen&&draw_win_mode[screen_res]==DWM_GRILLE
#endif
        )
        sh/=2;

#if !defined(SSE_VID_D3D_ONLY)
      if(BORDER_40)
        tx-=16*sw; // same old trick (BPOC 400 pixels)
#endif//#if !defined(SSE_VID_D3D_ONLY)
    }

#ifdef SSE_VID_EXT_FS1
    if(extended_monitor && stx==SurfaceWidth)
      sw=sh=1;
#endif

    TRACE_LOG("Sprite sw %f tx %f sh %f ty %f\n",sw,tx,sh,ty);
  }

  D3DMATRIX matrix= {
    sw,              0.0f,            0.0f,            0.0f,
    0.0f,            sh,              0.0f,            0.0f,
    0.0f,            0.0f,            1.0f,            0.0f,
    tx,              ty,              0.0f,            1.0f
  };
  pD3DSprite->SetTransform((D3DXMATRIX*)&matrix);
#endif
#if defined(SSE_VID_D3D_390)
  if(pD3DDevice)
    pD3DDevice->Clear(0,0,D3DCLEAR_TARGET,0,0,0);
#elif defined(SSE_VID_D3D_382)
  Cls();
#endif
  return hr;
}

#if defined(SSE_VID_D3D_382)
  
void SteemDisplay::D3DUpdateWH(UINT mode) {
  if(!pD3D)
    return;
  D3DDISPLAYMODE d3ddm;
  pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
  D3DDISPLAYMODE Mode; 
#if defined(SSE_VS2008_WARNING_390)//could do without param but it looks more logical this way
  pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT,d3ddm.Format,mode,&Mode);
#else
  pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT,d3ddm.Format,D3DMode,&Mode);
#endif
  D3DFsW=Mode.Width;
  D3DFsH=Mode.Height;
  TRACE_LOG("D3DUpdateWH mode %d w %d h %d\n",mode,D3DFsW,D3DFsH);
}

#endif

#endif//d3d


#ifdef UNIX
#include "x/x_display.cpp"
#endif

#undef LOGSECTION//SS
