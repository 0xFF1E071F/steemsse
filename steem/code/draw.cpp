/*---------------------------------------------------------------------------
FILE: draw.cpp
MODULE: emu
DESCRIPTION: Routines to handle Steem's video output. draw_routines_init
initialises the system, draw_begin locks output, draw_scanline is used to
draw one line, draw_end unlocks output and draw_blit blits the drawn output
to the PC display. 
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: draw.cpp")
#endif

#if defined(SSE_BUILD)

#define EXT
#define EXTC
#define INIT(s) =s

#if defined(SSE_VAR_RESIZE)
EXT BYTE bad_drawing INIT(0);
#if !defined(SSE_VID_D3D_ONLY)
EXT BYTE draw_fs_blit_mode INIT( UNIX_ONLY(DFSM_STRAIGHTBLIT) WIN_ONLY(DFSM_STRETCHBLIT) );
#endif
#if defined(SSE_VID_D3D_ONLY)
EXT BYTE draw_grille_black INIT(6);
#else
EXT BYTE draw_fs_fx INIT(DFSFX_NONE),draw_grille_black INIT(6);
#endif
#if defined(SSE_VID_BORDERS_GUI_392) 
EXT BYTE border INIT(1),border_last_chosen INIT(1);
#elif defined(SSE_VID_DISABLE_AUTOBORDER)
EXT bool border INIT(1),border_last_chosen INIT(1);
#else
EXT BYTE border INIT(2),border_last_chosen INIT(2);
#endif
#if !defined(SSE_VID_D3D_ONLY)
EXT BYTE draw_fs_topgap INIT(0);
BYTE prefer_pc_hz[2][3]={{0,0,0},{0,0,0}};
BYTE tested_pc_hz[2][3]={{0,0,0},{0,0,0}};
#endif//#if !defined(SSE_VID_D3D_ONLY)
#else
EXT int bad_drawing INIT(0);
#if !defined(SSE_VID_D3D_ONLY)
EXT int draw_fs_blit_mode INIT( UNIX_ONLY(DFSM_STRAIGHTBLIT) WIN_ONLY(DFSM_STRETCHBLIT) );
#endif
EXT int draw_fs_fx INIT(DFSFX_NONE),draw_grille_black INIT(6);
EXT int border INIT(2),border_last_chosen INIT(2);
EXT int draw_fs_topgap INIT(0);
int prefer_pc_hz[2][3]={{0,0,0},{0,0,0}};
WORD tested_pc_hz[2][3]={{0,0,0},{0,0,0}};
#endif

EXT RECT draw_blit_source_rect;


WIN_ONLY( EXT int draw_win_mode[2]; ) // Inited by draw_fs_blit_mode

EXTC BYTE FullScreen INIT(0);

EXT bool draw_lock;

EXT BYTE *draw_mem;
EXT int draw_line_length;
EXT long *PCpal;
EXT WORD STpal[16];
EXT BYTE *draw_dest_ad,*draw_dest_next_scanline;
#if defined(SSE_VID_BPP_NO_CHOICE)
EXT const BYTE display_option_8_bit_fs INIT(0);
#elif defined(SSE_VID_BPP_CHOICE)
EXT BYTE display_option_fs_bpp INIT(0); 
#else
EXT bool display_option_8_bit_fs INIT(false);
#endif
#if !defined(SSE_VID_D3D_ONLY)
EXT bool prefer_res_640_400 INIT(0),using_res_640_400 INIT(0);
#endif
#if defined(SSE_VAR_RESIZE)
EXT char overscan INIT(0)
#else
EXT int overscan INIT(0)
#endif
#if !defined(SSE_SHIFTER_REMOVE_USELESS_VAR)
,stfm_borders INIT(0)
#endif
;
UNIX_ONLY( EXT int x_draw_surround_count INIT(4); )

WIN_ONLY( EXT HWND ClipWin; )



#if defined(SSE_VAR_RESIZE)

short cpu_cycles_from_hbl_to_timer_b;

const BYTE scanlines_above_screen[4]={SCANLINES_ABOVE_SCREEN_50HZ,
                                    SCANLINES_ABOVE_SCREEN_60HZ,
                                    SCANLINES_ABOVE_SCREEN_70HZ,
                                    16};

const WORD scanline_time_in_cpu_cycles_8mhz[4]={SCANLINE_TIME_IN_CPU_CYCLES_50HZ,
                                                SCANLINE_TIME_IN_CPU_CYCLES_60HZ,
                                                SCANLINE_TIME_IN_CPU_CYCLES_70HZ,
                                                128};


#else

int cpu_cycles_from_hbl_to_timer_b;

const int scanlines_above_screen[4]={SCANLINES_ABOVE_SCREEN_50HZ,
                                    SCANLINES_ABOVE_SCREEN_60HZ,
                                    SCANLINES_ABOVE_SCREEN_70HZ,
                                    16};

const int scanline_time_in_cpu_cycles_8mhz[4]={SCANLINE_TIME_IN_CPU_CYCLES_50HZ,
                                                SCANLINE_TIME_IN_CPU_CYCLES_60HZ,
                                                SCANLINE_TIME_IN_CPU_CYCLES_70HZ,
                                                128};

#endif

int scanline_time_in_cpu_cycles[4]={SCANLINE_TIME_IN_CPU_CYCLES_50HZ,
                                    SCANLINE_TIME_IN_CPU_CYCLES_60HZ,
                                    SCANLINE_TIME_IN_CPU_CYCLES_70HZ,
                                    128};

int draw_dest_increase_y;

int res_vertical_scale=1;
int draw_first_scanline_for_border,draw_last_scanline_for_border; //calculated from BORDER_TOP, BORDER_BOTTOM and res_vertical_scale

int draw_first_possible_line=0,draw_last_possible_line=200;

int scanline_drawn_so_far;
#ifndef SSE_VAR_DEAD_CODE
int cpu_cycles_when_shifter_draw_pointer_updated;
#endif
int left_border=BORDER_SIDE,right_border=BORDER_SIDE;
#if !defined(SSE_VIDEO_CHIPSET)
bool right_border_changed=0;//we use the border mask instead
#endif
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
// We finally do without this confusing variable (especially annoying
// for large display mode differences :)
int overscan_add_extra;
#endif
LPPIXELWISESCANPROC jump_draw_scanline[3][4][3],draw_scanline,draw_scanline_lowres,draw_scanline_medres;
#if defined(SSE_TIMINGS_CPUTIMER64)
COUNTER_VAR shifter_freq_change_time[32];
#else
int shifter_freq_change_time[32];
#endif
#if defined(SSE_VAR_RESIZE)
BYTE shifter_freq_change[32];
BYTE shifter_freq_change_idx=0;
#else
int shifter_freq_change[32];
int shifter_freq_change_idx=0;
#endif

#if defined(SSE_SHIFTER_TRICKS)
// keeping a record for shift mode changes as well
COUNTER_VAR shifter_shift_mode_change_time[32];
#if defined(SSE_VAR_RESIZE)
BYTE shifter_shift_mode_change[32];
BYTE shifter_shift_mode_change_idx=0;
#else
int shifter_shift_mode_change[32];
int shifter_shift_mode_change_idx=0;
#endif
#endif


#ifdef WIN32
#if defined(SSE_VID_BORDERS)
//BYTE draw_temp_line_buf[800*4+16+ 200 ]; // overkill but I can't count
BYTE draw_temp_line_buf[800*4+16+ 400 ]; // overkill but I can't count
#else
BYTE draw_temp_line_buf[800*4+16]; 
#endif

BYTE *draw_store_dest_ad=NULL;
LPPIXELWISESCANPROC draw_scanline_1_line[2],draw_store_draw_scanline;
bool draw_buffer_complex_scanlines;
#endif

bool draw_med_low_double_height;

bool draw_line_off=0;

bool freq_change_this_scanline=false;

#undef EXT
#undef EXTC
#undef INIT

#define LOGSECTION LOGSECTION_VIDEO_RENDERING

#endif//decla

//---------------------------------------------------------------------------
void ASMCALL draw_scanline_dont(int,int,int,int) {}

//---------------------------------------------------------------------------
#ifdef WIN32 // SS inlined
#define DRAW_BUFFERED_SCANLINE_TO_VIDEO \
  if (draw_store_dest_ad){ \
    int amount_drawn=int(draw_dest_ad-draw_temp_line_buf); \
    DWORD *src=(DWORD*)draw_temp_line_buf; \
    DWORD *dest=(DWORD*)draw_store_dest_ad;  \
    while (src<(DWORD*)draw_dest_ad) *(dest++)=*(src++); \
    if (draw_med_low_double_height){                       \
      src=(DWORD*)draw_temp_line_buf;                        \
      dest=(DWORD*)(draw_store_dest_ad+draw_line_length);      \
      while (src<(DWORD*)draw_dest_ad) *(dest++)=*(src++);       \
    }                                                              \
    draw_dest_ad=draw_store_dest_ad+amount_drawn;                    \
    draw_store_dest_ad=NULL;                                           \
    draw_scanline=draw_store_draw_scanline; \
  }

#else
#define DRAW_BUFFERED_SCANLINE_TO_VIDEO
#endif
//---------------------------------------------------------------------------

void draw_begin()
{
  //TRACE_OSD("frame %d",FRAME);//called at each frame, it calls Lock()
  if (draw_lock) return;
  /*
  #ifndef NO_CRAZY_MONITOR
  if(extended_monitor){
    if(!FullScreen && em_needs_fullscreen){
      return;
    }
  }
#endif
*/
  // this initialises shifter_x, shifter_y,res_vertical_scale,
  // draw_first_scanline_for_border, draw_last_scanline_for_border
//  init_screen();
#if defined(SSE_VID_BORDERS_GUI_392)
  if (border){
#else
  if (border & 1){
#endif
    draw_first_possible_line=draw_first_scanline_for_border;
    draw_last_possible_line=draw_last_scanline_for_border;
  }else{
    draw_first_possible_line=0;
    draw_last_possible_line=shifter_y;
#if defined(SSE_VID_DISABLE_AUTOBORDER)
#if !defined(SSE_VID_D3D_ONLY)
    draw_fs_topgap=40;
#endif
#else
    if (FullScreen && overscan && (border==2)){ //hack overscan
      draw_last_possible_line+=40;
      draw_fs_topgap=0;
    }else{
      draw_fs_topgap=40;
    }
#endif
  }
  UNIX_ONLY( draw_fs_topgap=0; )
#if !defined(SSE_VID_D3D_ONLY)
  if (draw_grille_black>0) Disp.DrawFullScreenLetterbox(); //TODO test FS here
#endif

  if (Disp.Lock()!=DD_OK) return;

  if (BytesPerPixel==1) palette_copy();
  osd_draw_begin();

  draw_lock=true;

  draw_set_jumps_and_source();
//  scanline_drawn_so_far=0;
//  shifter_draw_pointer_at_start_of_line=xbios2;

  draw_mem+=(draw_blit_source_rect.top*draw_line_length)+
            (draw_blit_source_rect.left*BytesPerPixel);
  draw_dest_ad=draw_mem;
  draw_dest_next_scanline=draw_dest_ad+draw_dest_increase_y;
  WIN_ONLY( draw_store_dest_ad=NULL; )

#if defined(SSE_VID_SCANLINES_INTERPOLATED)
    if(SCANLINES_INTERPOLATED
#if defined(SSE_VID_D3D_382)
    || FullScreen&&draw_win_mode[screen_res]==DWM_GRILLE 
#endif          
      ) 
      draw_grille_black=4;
#endif
  if (draw_grille_black>0){
    bool using_grille=0;
    if (draw_dest_increase_y>draw_line_length){
#ifdef WIN32
#if !defined(SSE_VID_D3D_ONLY)
      if (FullScreen){
        if (draw_fs_fx==DFSFX_GRILLE) using_grille=true;
      }else 
#endif
        if (screen_res<2){
        if (draw_win_mode[screen_res]==DWM_GRILLE) using_grille=true;
      }
#else
      if (draw_fs_fx==DFSFX_GRILLE) using_grille=true;
#endif
    }
#if defined(SSE_VID_SCANLINES_INTERPOLATED)
    if(SCANLINES_INTERPOLATED
#if defined(SSE_VID_D3D_382)
    || FullScreen&&draw_win_mode[screen_res]==DWM_GRILLE 
#endif          
      ) 
      using_grille=true;
#endif
    if (using_grille){
      int l=640;
      if (Disp.BorderPossible()) l+=BORDER_SIDE*2 * 2;
      l*=BytesPerPixel;
      BYTE *d=draw_dest_ad+draw_line_length;
      int y=200;
#if  defined(SSE_VID_BORDERS_BIGTOP)
     // if (Disp.BorderPossible()) y=200+BORDER_BOTTOM+ 30;
      if (Disp.BorderPossible()) y=200+BottomBorderSize+ BORDER_TOP;
#else
      if (Disp.BorderPossible()) y=200+BORDER_BOTTOM+BORDER_TOP;
#endif
      for(;y>0;y--){
        memset(d,0,l);
        d+=draw_dest_increase_y;
      }
    }
    draw_grille_black--;
  }

#ifdef UNIX
  if (x_draw_surround_count>0){
    Disp.Surround();
    x_draw_surround_count--;
  }
#endif

#ifdef DEBUG_BUILD
  if (debug_cycle_colours){
    debug_cycle_colours--;
    if (debug_cycle_colours==0){
      for (int i=0;i<16;i++){
        switch (BytesPerPixel){
        case 1:
          PCpal[i]=rand() % 256;
          PCpal[i]|=(PCpal[i] << 8);
          break;
        case 2:
          PCpal[i]=0x8888+(rand() % 0x7777);
          PCpal[i]|=(PCpal[i] << 16);
          break;
        case 3:case 4:
          PCpal[i]=0x88888888+(rand() % 0x77777777);
          break;
        }
      }
      debug_cycle_colours=CYCLE_COL_SPEED;
    }
  }
#endif
}
//---------------------------------------------------------------------------
void draw_set_jumps_and_source()
{
  // SS called at each frame by draw_begin()
  if (!draw_lock){
    draw_scanline=draw_scanline_dont;
    return;
  }
  bool big_draw=CanUse_400;
  UNIX_ONLY( if (FullScreen) big_draw=true;  )
#ifdef WIN32
#if defined(SSE_VID_D3D_ONLY)
  if(FullScreen)
    big_draw=false;
#else
  if (FullScreen
#if defined(SSE_VID_SCANLINES_INTERPOLATED) && defined(SSE_VID_3BUFFER) && !defined(SSE_VID_3BUFFER_392)
/*  About this complication, it is necessary to have interpolated
    scanlines working in fullscreen mode with triple buffer.
    There are many more places where we test that.
*/
    && !(SCANLINES_INTERPOLATED&&OPTION_3BUFFER)
#endif
    ){
    if (draw_fs_blit_mode==DFSM_STRETCHBLIT || draw_fs_blit_mode==DFSM_LAPTOP
#if defined(SSE_VID_D3D_STRETCH_FORCE)      
      || D3D9_OK && OPTION_D3D
#endif
      ) big_draw=0;
  }else 
#endif//#if defined(SSE_VID_D3D_ONLY)    
    if (big_draw){
    if (ResChangeResize==0){
      big_draw=0; // always stretch
    }else if (screen_res<2){
      if (draw_win_mode[screen_res]==DWM_STRETCH)
        big_draw=0;
    }
  }
#endif

  if (emudetect_falcon_mode!=EMUD_FALC_MODE_OFF){
    draw_scanline=emudetect_falcon_draw_scanline;
    draw_dest_increase_y=draw_line_length;

    int ox=0,oy=0,ow,oh;
    if (big_draw){
      ow=640;oh=400;
#if defined(SSE_VID_BORDERS_GUI_392)
      if (border){
#else
      if (border & 1){
#endif
        ow=640 + BORDER_SIDE*2 + BORDER_SIDE*2;
        oh=400 + BORDER_TOP*2 + BORDER_BOTTOM*2;
#ifdef WIN32
#if !defined(SSE_VID_D3D_ONLY)
        if (FullScreen) ox=(800-ow)/2, oy=(600-oh)/2;
#endif
#if !defined(SSE_VID_D3D_ONLY)
      }else if (FullScreen && using_res_640_400==0){
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
        if (overscan && border==2 && emudetect_falcon_extra_height) oh=480;
#endif
        oy=(480-oh)/2;
#endif
#endif
      }
      if (emudetect_falcon_mode_size==1) draw_dest_increase_y*=2; // Have to draw double height
    }else{
      ow=320*emudetect_falcon_mode_size,oh=200*emudetect_falcon_mode_size;
#if defined(SSE_VID_BORDERS_GUI_392)
      if (border){
#else
      if (border & 1){
#endif
        // We double the size of borders too to keep the aspect ratio of the screen the same
        ow+=BORDER_SIDE*2*emudetect_falcon_mode_size;
        oh+=(BORDER_TOP+BORDER_BOTTOM)*emudetect_falcon_mode_size;
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
      }else if (overscan && border==2 && emudetect_falcon_extra_height){
        oh+=40*emudetect_falcon_mode_size;
#endif
      }
    }
    draw_blit_source_rect.left=ox;
    draw_blit_source_rect.right=ox+ow;
    draw_blit_source_rect.top=oy;
    draw_blit_source_rect.bottom=oy+oh;
    return;
  }

#ifndef NO_CRAZY_MONITOR
  if (extended_monitor){
    int r=screen_res; // Low/Med
    if (em_planes==1) r=2;
    draw_scanline=jump_draw_scanline[0][BytesPerPixel-1][r];
    draw_blit_source_rect.left=0;
    draw_blit_source_rect.right=min(em_width,Disp.SurfaceWidth);
    draw_blit_source_rect.top=0;
    draw_blit_source_rect.bottom=min(em_height,Disp.SurfaceHeight);
    draw_dest_increase_y=draw_line_length;
    return;
  }
#endif
  
  if (big_draw
#if defined(SSE_VID_SCANLINES_INTERPOLATED)
    || (SCANLINES_INTERPOLATED
#if defined(SSE_VID_D3D_382)
      ||FullScreen&&draw_win_mode[screen_res]==DWM_GRILLE
#endif

#if !defined(SSE_VID_D3D_ONLY) && !defined(SSE_VID_3BUFFER_392)
#if defined(SSE_VID_SCANLINES_INTERPOLATED) && defined(SSE_VID_3BUFFER)
        && !(SCANLINES_INTERPOLATED&&OPTION_3BUFFER)
#endif
#endif
    )
#endif
    ){
    int p=1; // 640 width 400 height
#ifdef WIN32
    if (FullScreen
#if !defined(SSE_VID_D3D_ONLY) && !defined(SSE_VID_3BUFFER_392)
#if defined(SSE_VID_SCANLINES_INTERPOLATED) && defined(SSE_VID_3BUFFER)
        && !(SCANLINES_INTERPOLATED&&OPTION_3BUFFER)
#endif
#endif//#if !defined(SSE_VID_D3D_ONLY)
      ){
#if !defined(SSE_VID_D3D_ONLY)
      if (draw_fs_fx==DFSFX_GRILLE) p=2; // 640x200 low/med
#endif
    }else if (screen_res<2){
      if (draw_win_mode[screen_res]==DWM_GRILLE) p=2; // 640x200 low/med
    }
#else
    if (draw_fs_fx==DFSFX_GRILLE) p=2; // 640x200 low/med
#endif

#if defined(SSE_VID_SCANLINES_INTERPOLATED)
    if(SCANLINES_INTERPOLATED
#if defined(SSE_VID_D3D_382)
      ||FullScreen&&draw_win_mode[screen_res]==DWM_GRILLE
#endif
      )
      p=0;
#endif

    draw_scanline=jump_draw_scanline[p][BytesPerPixel-1][screen_res];
    draw_scanline_lowres=jump_draw_scanline[p][BytesPerPixel-1][0];
    draw_scanline_medres=jump_draw_scanline[p][BytesPerPixel-1][1];
    draw_med_low_double_height=(p==1);

#ifdef WIN32
    draw_scanline_1_line[0]=jump_draw_scanline[2][BytesPerPixel-1][0];
    draw_scanline_1_line[1]=jump_draw_scanline[2][BytesPerPixel-1][1];
#endif

    int ox=0,oy=0,ow=640,oh=400;
#if defined(SSE_VID_BORDERS_GUI_392)
    if (border){
#else
    if (border & 1){
#endif
      oh=BORDER_TOP*2 + 400 + BORDER_BOTTOM*2;
#if defined(SSE_VID_BORDERS)
      // this may not be larger than the window
      ow=SideBorderSizeWin*2 + 640 + SideBorderSizeWin*2;

#if defined(SSE_VID_BORDERS_413) // here we go again...
      if(SideBorderSizeWin==VERY_LARGE_BORDER_SIDE_WIN)
        ow+=2;
#endif

#else
      ow=BORDER_SIDE*2 + 640 + BORDER_SIDE*2;
#endif
#ifdef WIN32

#if !defined(SSE_VID_D3D_FS_392A) || !defined(SSE_VID_D3D_ONLY)
      if (FullScreen
#if defined(SSE_VID_SCANLINES_INTERPOLATED) && defined(SSE_VID_3BUFFER) && !defined(SSE_VID_3BUFFER_392)
        && !(SCANLINES_INTERPOLATED&&OPTION_3BUFFER)
#endif
#if defined(SSE_VID_D3D_STRETCH)
        && !(D3D9_OK && OPTION_D3D)//TODO specify the switches
#endif
        ){
        ox=(800-ow)/2;oy=(600-oh)/2;
      }
#endif

#endif
    }else if (FullScreen
#if defined(SSE_VID_SCANLINES_INTERPOLATED) && !defined(SSE_VID_3BUFFER_392)
      && !(SCANLINES_INTERPOLATED&&(OPTION_3BUFFER||OPTION_D3D))
#endif
      ){
#if !defined(SSE_VID_D3D_ONLY)
      WIN_ONLY( oy=int(using_res_640_400 ? 0:40); )
#endif
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
      if (overscan && (border==2)){ //hack overscan
        oy=0;oh=480;
      }
#endif
    }

#if defined(SSE_VID_SCANLINES_INTERPOLATED)
#if defined(SSE_VID_3BUFFER_392) // option_3buffer or not -> false if fullscreen anyway
    if(SCANLINES_INTERPOLATED && !screen_res && !FullScreen) 
      ow/=2;
#else
    if(!screen_res && SCANLINES_INTERPOLATED && 
#if defined(SSE_VID_3BUFFER)
      (!FullScreen ||OPTION_3BUFFER
#if defined(SSE_VID_SCANLINES_INTERPOLATED) && defined(SSE_VID_3BUFFER)
        && !(SCANLINES_INTERPOLATED&&OPTION_3BUFFER)
#endif
      )
#else
      !FullScreen
#endif
      ) 
      ow/=2;
#endif
#endif

    draw_blit_source_rect.left=ox;
    draw_blit_source_rect.right=ox+ow;
    draw_blit_source_rect.top=oy;
    draw_blit_source_rect.bottom=oy+oh;

    if (MONO){
      draw_dest_increase_y=draw_line_length;
    }else{
      draw_dest_increase_y=2*draw_line_length;
#if defined(SSE_VID_D3D_INTERPOLATED_SCANLINES)
      if(SCANLINES_INTERPOLATED && OPTION_D3D)
        draw_blit_source_rect.right-=2;
#endif
    }
  }else{
    draw_scanline=jump_draw_scanline[0][BytesPerPixel-1][screen_res];
    draw_scanline_lowres=jump_draw_scanline[0][BytesPerPixel-1][0];
    draw_scanline_medres=jump_draw_scanline[0][BytesPerPixel-1][1];
    draw_med_low_double_height=0;
    int ox=0,oy=0,ow=shifter_x,oh=shifter_y;
    if (mixed_output){
      draw_scanline_lowres=jump_draw_scanline[2][BytesPerPixel-1][0];
      if (screen_res==0) draw_scanline=draw_scanline_lowres;
      ow=640;
    }

#ifdef WIN32
    draw_scanline_1_line[0]=draw_scanline_lowres;
    draw_scanline_1_line[1]=draw_scanline_medres;
#endif

//    draw_scanline=jump_draw_scanline[x][0][BytesPerPixel-1][screen_res];
#if defined(SSE_VID_BORDERS_GUI_392)
    if (border){
#else
    if (border & 1){
#endif
      if (screen_res==0 && mixed_output==0){
        ow+=(BORDER_SIDE+BORDER_SIDE);
      }else{
        ow+=(BORDER_SIDE+BORDER_SIDE) * 2;
      }
      oh=shifter_y+res_vertical_scale*(BORDER_TOP+BORDER_BOTTOM);
    }else if (FullScreen
#if defined(SSE_VID_SCANLINES_INTERPOLATED) && defined(SSE_VID_3BUFFER) && !defined(SSE_VID_3BUFFER_392)
        && !(SCANLINES_INTERPOLATED&&OPTION_3BUFFER)
#endif
      ){
#if !defined(SSE_VID_D3D_ONLY)
#if defined(SSE_VID_D3D_STRETCH)
      if(!(D3D9_OK && OPTION_D3D))
#endif
      WIN_ONLY( oy=int(using_res_640_400 ? 0:40); )
#endif
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
      if (overscan && (border==2)){ //hack overscan
        oy=0;
        oh=240;
      }
#endif
    }
//    ox=0;oy=0;ow=736;oh=460;  //horrible big
    draw_blit_source_rect.left=ox;
    draw_blit_source_rect.top=oy;
    draw_blit_source_rect.right=ox+ow;
    draw_blit_source_rect.bottom=oy+oh;
//    if(overscan)draw_blit_source_rect_bottom+=OVERSCAN_HEIGHT; should this be put in???
    draw_dest_increase_y=draw_line_length;
  }
  WIN_ONLY( draw_buffer_complex_scanlines=(Disp.Method==DISPMETHOD_DD &&
                  Disp.DrawToVidMem && draw_med_low_double_height); )
#if defined(SSE_BOILER_VIDEO_CONTROL__)
  if(VIDEO_CONTROL_MASK & VIDEO_CONTROL_BLIT)
  {
    TRACE_LOG("src %d %d %d %d\n",draw_blit_source_rect.left,draw_blit_source_rect.top,draw_blit_source_rect.right,draw_blit_source_rect.bottom);
  }
#endif
  //TRACE("src %d %d %d %d len %d\n",draw_blit_source_rect.left,draw_blit_source_rect.top,draw_blit_source_rect.right,draw_blit_source_rect.bottom,draw_line_length);
}
//---------------------------------------------------------------------------
void draw_end()
{
  if (draw_lock==0) return;
#ifndef ONEGAME
  bool draw_osd=true;
  if (DoSaveScreenShot || slow_motion) draw_osd=0;
  DEBUG_ONLY( if (runstate!=RUNSTATE_RUNNING) draw_osd=0; )
  if (draw_osd) osd_draw();
#endif

#ifdef DEBUG_BUILD //SS? TODO

#if defined(SSE_SHIFTER)
  Shifter.DrawBufferedScanlineToVideo(); 
#else
  DRAW_BUFFERED_SCANLINE_TO_VIDEO
#endif

#endif

  Disp.Unlock();

#ifdef SHOW_WAVEFORM
  Disp.DrawWaveform();
#endif
  osd_draw_end(); // resets osd_draw function pointers

  draw_scanline=draw_scanline_dont;
  WIN_ONLY( draw_store_dest_ad=NULL; )
  draw_lock=false;

  if (DoSaveScreenShot){
    Disp.SaveScreenShot();
    DoSaveScreenShot&=~1;
  }
}


#if !defined(SSE_VIDEO_CHIPSET)
#define LOGSECTION LOGSECTION_VIDEO
//---------------------------------------------------------------------------
/* SS this was the core of Shifter trick analysis in Steem 3.2.
   There's not so much code but it ran many cases eg Darkside of the Spoon,
   it was missing the 0byte line, 4bit scrolling.
   It has been much expanded and commented in SSEGlue.cpp.
   This function isn't compiled.
*/
void draw_check_border_removal()
{
//  if (shifter_freq_at_start_of_vbl!=50) return;
  if (screen_res>=2) return;
  if (scan_y<shifter_first_draw_line || scan_y>=shifter_last_draw_line) return;

  int act=ABSOLUTE_CPU_TIME,t,i;
/* SS notice how those cycles relate to a supposed HBL at 484 but the
   actual tests use other values.
*/
  // Border    | Cycle
  // Mono off  | 21-23
  // 60Hz off  | 80
  // 50Hz off  | 84
  // Mono on   | 192
  // 60Hz on   | 400
  // 50Hz on   | 404
  if (left_border){
    // We haven't removed the left border, so check if we ought to.
    t=cpu_timer_at_start_of_hbl+2+stfm_borders; //trigger point
    if (act>t){
      i=shifter_freq_change_idx;
      while (shifter_freq_change_time[i]>t){
        i--;i&=31;
        if (i==shifter_freq_change_idx){ i=-1;break; }
      }
      if (i>=0){
        if (shifter_freq_change[i]==MONO_HZ){
          //remove left border
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: LEFT BORDER REMOVED - there was a change to 70 at cycle ")+
            (shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl));

          shifter_draw_pointer+=8;
          if (shifter_freq_at_start_of_vbl==50) overscan_add_extra+=OVERSCAN_ADD_EXTRA_FOR_LEFT_BORDER_REMOVAL;
          left_border=0;
/*
          int fetches_in_mono=((shifter_freq_change_time[(i+1) & 31]-cpu_timer_at_start_of_hbl-8)/4) % 4;
          shifter_draw_pointer+=fetches_in_mono*2;
          overscan_add_extra-=fetches_in_mono*2;
*/


          if (shifter_freq_at_start_of_vbl==50) shifter_pixel+=4;  /////  new - added 1/1/2002 - to make Oh Crikey What a Scorcher Hidden Screen (First Part) work.

          if (shifter_hscroll>=12){
            shifter_draw_pointer+=8;  ///// New, shifter bug causes this, just a hack
            overscan_add_extra-=8;
          }

          overscan=OVERSCAN_MAX_COUNTDOWN;
        }
      }
    }
  }

/*
  // This should do something, but I don't know what! See Nostalgia Lemmings screen.
  if (overscan_add_extra>-300){
    t=cpu_timer_at_start_of_hbl+48; //trigger point
    if (act>t){
      i=shifter_freq_change_idx;
      while (shifter_freq_change_time[i]>t){
        i--; i&=31;
        if (i==shifter_freq_change_idx){ i=-1;break; }
      }
      if (i>=0){
        if (shifter_freq_change[i]==MONO_HZ){
          overscan_add_extra=-320;
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: Skipping this whole line, there was a change to mono at ")+
                    (shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl));
        }
      }
    }
  }
*/
  //  This must disable the display of a line. No change to sdp or border removal
  //  just disable output, i.e draw a black line.
  if (shifter_freq_at_start_of_vbl==50){
    if (draw_line_off==0){
      t=cpu_timer_at_start_of_hbl+28; //trigger point
      if (act>t){
        i=shifter_freq_change_idx;
        while (shifter_freq_change_time[i]>t){
          i--; i&=31;
          if (i==shifter_freq_change_idx){ i=-1;break; }
        }
        if (i>=0){
          if (shifter_freq_change[i]==60 && shifter_freq_change_time[i]==t){
            log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: Disabling this whole line, there was a change to 60Hz at ")+
                      (shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl));
            draw_line_off=true;
            memset(PCpal,0,sizeof(long)*16);
          }
        }
      }
    }

    // This should move the line left, and increase its length by 2 bytes, but
    // not change fetching position
    if (left_border==BORDER_SIDE){
      t=cpu_timer_at_start_of_hbl+58; // Check 60Hz border off (50Hz border off-4)
      if (act>t){
        i=shifter_freq_change_idx;
        while (shifter_freq_change_time[i]>t){
          i--;i&=31;
          if (i==shifter_freq_change_idx){ i=-1;break; }
        }
        if (i>=0 && shifter_freq_change[i]==60){
          log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: LEFT BORDER SHRUNK by 4 pixels - there was a change to 60 at absolute cycle ")+shifter_freq_change_time[i]);
          left_border-=4;
          overscan_add_extra=OVERSCAN_ADD_EXTRA_FOR_SMALL_LEFT_BORDER_REMOVAL;
          overscan=OVERSCAN_MAX_COUNTDOWN;
  /*
        }else if (overscan_add_extra==0){
          t=cpu_timer_at_start_of_hbl+38;
          i=shifter_freq_change_idx;
          while (shifter_freq_change_time[i]>t){
            i--;i&=31;
            if (i==shifter_freq_change_idx){ i=-1;break; }
          }
          if (i>=0) if (shifter_freq_change[i]==60){
            log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: Shifting screen by a word - there was a change to 60 at absolute cycle ")+shifter_freq_change_time[i]);
            shifter_draw_pointer_at_start_of_line+=2;
            shifter_draw_pointer+=2;
            overscan_add_extra=2;
          }
  */
        }
      }
    }
  }

  if (right_border_changed) return;

  // Check for right border stuff
  t=cpu_timer_at_start_of_hbl+172; //trigger point for big right border
  if (act>=t){
    i=shifter_freq_change_idx;
    while (shifter_freq_change_time[i]>t){
      i--; i&=31;
      if (i==shifter_freq_change_idx){i=-1;break;}
    }
    if (i>=0){
      if (shifter_freq_change[i]==MONO_HZ){ //big right border
        log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: HUGE RIGHT BORDER ACTIVE, change to 70 at ")+
                        (shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl));
        overscan_add_extra+=OVERSCAN_ADD_EXTRA_FOR_GREAT_BIG_RIGHT_BORDER;
        right_border_changed=true;
      }
    }
  }

  if (shifter_freq_at_start_of_vbl!=50) return;

  // Check for right border removal/cut
  t=cpu_timer_at_start_of_hbl+372; //trigger point for right border cut
   if (act>=t){
    i=shifter_freq_change_idx;
    while (shifter_freq_change_time[i]>t){
      i--; i&=31;
      if (i==shifter_freq_change_idx) return;
    }
    if (shifter_freq_change[i]==60){ //early right  border
      log_to(LOGSECTION_VIDEO,Str("VIDEO: RIGHT BORDER SLIGHTLY ENLARGED, change to 60 at ")+
                      (shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl));
      overscan_add_extra+=OVERSCAN_ADD_EXTRA_FOR_EARLY_RIGHT_BORDER;
      right_border_changed=true;
    }else{
      t=cpu_timer_at_start_of_hbl+378; //trigger point for right border cut
      if (act>=t){
        if (i!=shifter_freq_change_idx){
          //check if we need to advance one more
          int i1=(i+1) & 31;
          if (shifter_freq_change_time[i1]<=t) i=i1;
        }
        if (shifter_freq_change[i]!=50){ //remove right border
          log_to(LOGSECTION_VIDEO,Str("VIDEO: RIGHT BORDER REMOVED, change to 60 at ")+
                          (shifter_freq_change_time[i]-cpu_timer_at_start_of_hbl));
          right_border=0;overscan_add_extra+=OVERSCAN_ADD_EXTRA_FOR_RIGHT_BORDER_REMOVAL;
          overscan=OVERSCAN_MAX_COUNTDOWN;
          right_border_changed=true;
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void inline draw_scanline_to(int cycles_since_hbl)
{
  if (screen_res>=2) return;
#ifndef NO_CRAZY_MONITOR
  if (extended_monitor || emudetect_falcon_mode!=EMUD_FALC_MODE_OFF) return;
#endif

//  log_write(EasyStr("draw_scanline_to, left_border=")+left_border+", right_border="+right_border);
//  if(!scanline_drawn_so_far){
//    shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
//  }

  if (freq_change_this_scanline) draw_check_border_removal();

//  int cycles_since_hbl=ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl;
  int pixels_in=cycles_since_hbl-(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN-BORDER_SIDE);
  if (pixels_in > BORDER_SIDE+320+BORDER_SIDE) pixels_in=BORDER_SIDE+320+BORDER_SIDE;
  if (pixels_in>=0){
#ifdef WIN32
    if (draw_buffer_complex_scanlines && draw_lock){
      if (scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border){
        if (draw_store_dest_ad==NULL && pixels_in<BORDER_SIDE+320+BORDER_SIDE){
          draw_store_dest_ad=draw_dest_ad;
          draw_dest_ad=draw_temp_line_buf;
          draw_store_draw_scanline=draw_scanline;
        }
        if (draw_store_dest_ad) draw_scanline=draw_scanline_1_line[screen_res];
      }
    }
#endif

    if (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line){
      //draw from scanline_drawn_so_far to pixels_in
      int border1=0,border2=0,picture=0,hscroll=0;
      int picture_left_edge=left_border; //might be negative if big overscan
      //last pixel from extreme left to draw of picture
      int picture_right_edge=BORDER_SIDE+320+BORDER_SIDE-right_border;

      if (pixels_in>picture_left_edge){ //might be some picture to draw
        if (scanline_drawn_so_far>picture_left_edge){
          picture=pixels_in-scanline_drawn_so_far;
          if (picture>picture_right_edge-scanline_drawn_so_far){
            picture=picture_right_edge-scanline_drawn_so_far;
          }
        }else{
          picture=pixels_in-picture_left_edge;
          if (picture>picture_right_edge-picture_left_edge){
            picture=picture_right_edge-picture_left_edge;
          }
        }
        if (picture<0) picture=0;
      }
      if (scanline_drawn_so_far<left_border){
        if (pixels_in>left_border){
          border1=left_border-scanline_drawn_so_far;
        }else{
          border1=pixels_in-scanline_drawn_so_far;
        }
        if (border1<0) border1=0;
      }
      border2=pixels_in-scanline_drawn_so_far-border1-picture;
      if (border2<0) border2=0;

      int old_shifter_pixel=shifter_pixel;
      shifter_pixel+=picture; //number of pixels drawn this time
      MEM_ADDRESS nsdp=shifter_draw_pointer;
      if (screen_res==0){
        hscroll=old_shifter_pixel & 15;
        nsdp-=(old_shifter_pixel/16)*8;
        nsdp+=(shifter_pixel/16)*8;
      }else if (screen_res==1){
        hscroll=(old_shifter_pixel*2) & 15;
        nsdp-=(old_shifter_pixel/8)*4;
        nsdp+=(shifter_pixel/8)*4;
      }
//      if(shifter_pixel<320 && shifter_pixel>0){
//        dbg_log(EasyStr("old_shifter_pixel=")+old_shifter_pixel+", shifter_pixel="+shifter_pixel
//          +" shifter_draw_pointer increase by "+(-(old_shifter_pixel/8)+(shifter_pixel/8))+" rasters hscroll="+hscroll);
//      }
      if (draw_lock){
        if (scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line){
          // actually draw it.
          if (picture_left_edge<0) picture+=picture_left_edge;

          if ((border & 1)==0){
            if (scanline_drawn_so_far<BORDER_SIDE){
              //take away BORDER_SIDE-scanline_drawn_so_far from border1+picture
              border1-=(BORDER_SIDE-scanline_drawn_so_far);
              if (border1<0){
                picture+=border1;

                if(screen_res==0){ //new bit 1/1/2002 to fix Punish Your Machine the Best Part of Creation
                  hscroll-=border1; //pixels not to draw
                  shifter_draw_pointer+=(hscroll/16)*8;
                  hscroll&=15;
                }else if(screen_res==1){
                  hscroll-=border1*2; //pixels not to draw
                  shifter_draw_pointer+=(hscroll/16)*4;
                  hscroll&=15;
                }

                border1=0;
                if (picture<0) picture=0;
              }
            }
            int ta=(border1+picture+border2)-320;
            if (ta>0){
              border2-=ta;
              if (border2<0){
                picture+=border2;
                border2=0;
                if (picture<0) picture=0;
              }
            }
            border1=0;
            border2=0;
          }

          DEBUG_ONLY( shifter_draw_pointer+=debug_screen_shift; )
          draw_scanline(border1,picture,border2,hscroll);
        }
      }

      shifter_draw_pointer=nsdp;
      if (pixels_in>=picture_right_edge && scanline_drawn_so_far<picture_right_edge){
        // add extra onto shifter_draw_pointer for overscans
        ADD_EXTRA_TO_SHIFTER_DRAW_POINTER_AT_END_OF_LINE(shifter_draw_pointer);
      }
    }else if (scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border){
      int border1;
      int left_visible_edge,right_visible_edge;
      if (border & 1){
        left_visible_edge=0;right_visible_edge=BORDER_SIDE + 320 + BORDER_SIDE;
      }else{
        left_visible_edge=BORDER_SIDE;right_visible_edge=320+BORDER_SIDE;
      }
      if (scanline_drawn_so_far<=left_visible_edge){
        border1=pixels_in-left_visible_edge;
      }else{
        border1=pixels_in-scanline_drawn_so_far;
      }
      if (border1<0){
        border1=0;
      }else if (border1> (right_visible_edge - left_visible_edge)){
        border1=(right_visible_edge - left_visible_edge);
      }
      if (scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line){
        draw_scanline(border1,0,0,0);
      }
    }
    scanline_drawn_so_far=pixels_in;
  }
}
//---------------------------------------------------------------------------
void inline draw_scanline_to_end()
{
  MEM_ADDRESS nsdp;

#ifndef NO_CRAZY_MONITOR
  if (emudetect_falcon_mode!=EMUD_FALC_MODE_OFF){
    int pic=320*emudetect_falcon_mode_size,bord=0;
    // We double the size of borders too to keep the aspect ratio of the screen the same
    if (border & 1) bord=BORDER_SIDE*emudetect_falcon_mode_size;
    if (scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line){
      bool in_border=(scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border);
      bool in_pic=(scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line);
      if (in_pic || in_border){
        // We only have 200 scanlines, but if emudetect_falcon_mode_size==2
        // then the picture must be 400 pixels high. So to make it work we
        // draw two different lines at the end of each scanline.
        // To make it more confusing in some drawing modes if
        // emudetect_falcon_mode_size==1 we have to draw two identical
        // lines. That is handled by the draw_scanline routine. 
        for (int n=0;n<emudetect_falcon_mode_size;n++){
          if (in_pic){
            nsdp=shifter_draw_pointer + pic*emudetect_falcon_mode;
            draw_scanline(bord,pic,bord,shifter_hscroll);
            ADD_EXTRA_TO_SHIFTER_DRAW_POINTER_AT_END_OF_LINE(nsdp);
            shifter_draw_pointer=nsdp;
          }else{
            draw_scanline(bord+pic+bord,0,0,0);
          }
          draw_dest_ad=draw_dest_next_scanline;
          draw_dest_next_scanline+=draw_dest_increase_y;
        }
      }
    }
    shifter_pixel=shifter_hscroll; //start by drawing this pixel
    scanline_drawn_so_far=BORDER_SIDE+320+BORDER_SIDE;

  }else if (extended_monitor){
    int h=min(em_height,Disp.SurfaceHeight);
    int w=min(em_width,Disp.SurfaceWidth);
    if (extended_monitor==1){	// Borders needed, before hack
      if (em_planes==1){ //mono
        int y=h/2-200, x=(w/2-320)&-16;
        if (scan_y<h){
          if (scan_y<y || scan_y>=y+400){
            draw_scanline(w/16,0,0,0);
          }else{
            draw_scanline(x/16,640/16,w/16-x/16-640/16,0);
            shifter_draw_pointer+=80;
          }
        }
      }else{
        int y=h/2-100, x=(w/2-160)&-16;
        if (scan_y<h){
          if (scan_y<y || scan_y>=y+200){
            draw_scanline(w,0,0,0);
          }else{
            draw_scanline(x,320,w-x-320,0);
            shifter_draw_pointer+=160;
          }
        }
      }
      draw_dest_ad=draw_dest_next_scanline;
      draw_dest_next_scanline+=draw_dest_increase_y;
    }else{
      if (scan_y<h){
        if (em_planes==1) w/=16;
        if (screen_res==1) w/=2; // medium res routine draws two pixels for every one w
        draw_scanline(0,w,0,0);
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }
      int real_planes=em_planes;
      if (screen_res==1) real_planes=2;
      shifter_draw_pointer+=em_width*real_planes/8;
    }
  }else
#endif
  if (screen_res<2){
    draw_scanline_to(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320+BORDER_SIDE);

    DRAW_BUFFERED_SCANLINE_TO_VIDEO

    if (scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line){
      draw_dest_ad=draw_dest_next_scanline;
      draw_dest_next_scanline+=draw_dest_increase_y;
    }
    shifter_pixel=shifter_hscroll; //start by drawing this pixel
  }else if (screen_res==2){ //hi-res scanlinewise
    if (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line){
      nsdp=shifter_draw_pointer+80;
      shifter_pixel=shifter_hscroll; //start by drawing this pixel

      if (scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line){
        if (border & 1){
          draw_scanline((BORDER_SIDE*2)/16, 640/16, (BORDER_SIDE*2)/16,0);
        }else{
          draw_scanline(0,640/16,0,0);
        }
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }

      shifter_draw_pointer=nsdp;
    }else if (scan_y>=draw_first_scanline_for_border && scan_y<draw_last_scanline_for_border){
      if (scan_y>=draw_first_possible_line && scan_y<draw_last_possible_line){
        if (border & 1){
          draw_scanline((BORDER_SIDE*2+640+BORDER_SIDE*2)/16,0,0,0); // rasters!
        }else{
          draw_scanline(640/16,0,0,0);
        }
        draw_dest_ad=draw_dest_next_scanline;
        draw_dest_next_scanline+=draw_dest_increase_y;
      }
    }
    scanline_drawn_so_far=BORDER_SIDE+320+BORDER_SIDE; //border1+picture+border2;
  }

  left_border=BORDER_SIDE;

  if (shifter_hscroll) left_border+=16;

  if (shifter_hscroll_extra_fetch) left_border-=16;
  right_border=BORDER_SIDE;overscan_add_extra=0;

  // In the STE if you make hscroll non-zero in the normal way then the shifter
  // buffers 2 rasters ahead. We don't do this so to make sdp correct at the
  // end of the line we must add a raster.  
  shifter_skip_raster_for_hscroll = shifter_hscroll!=0;
}
#undef LOGSECTION
#endif//#if !defined(SSE_VIDEO_CHIPSET)

//---------------------------------------------------------------------------
bool draw_blit()
{
#if defined(SSE_VID_3BUFFER_WIN)
  // we blit the unlocked backsurface
  if (!draw_lock 
#if !defined(SSE_VID_D3D_3BUFFER) 
#if defined(SSE_VID_3BUFFER_392)
    || OPTION_3BUFFER_WIN && !FullScreen 
#else
    || OPTION_3BUFFER 
#endif
#endif
#if !defined(SSE_VID_3BUFFER_FS)
    && !FullScreen
#endif
    ){ 
#else
  if (!draw_lock){
#endif
    if (bAppMinimized==0){
      if (BytesPerPixel==1) palette_flip();
      return Disp.Blit();
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
void draw(bool osd)
{
  // SS: this is called by init, load... not for actual emulation
  // It draws the screen in one time

  int save_scan_y=scan_y;
  MEM_ADDRESS save_sdp=shifter_draw_pointer;
  MEM_ADDRESS save_sdp_at_start_of_line=shifter_draw_pointer_at_start_of_line;
// fix: int
  MEM_ADDRESS save_drawn_so_far=scanline_drawn_so_far;
  MEM_ADDRESS save_pixel=shifter_pixel;
#if defined(SSE_TOS_GEMDOS_EM_382) && defined(SSE_DEBUG)
  if (extended_monitor)
  {
    ASSERT(xbios2!=mem_len-0x8000);
  }
#endif
  shifter_draw_pointer=xbios2;
#if defined(SSE_BOILER_XBIOS2)
  int save_shifter_first_draw_line=shifter_first_draw_line;
  int save_shifter_last_draw_line=shifter_last_draw_line;
#endif
#if defined(SSE_VID_3BUFFER_WIN)
  // we blit the unlocked backsurface
  if (!draw_lock 
#if !defined(SSE_VID_D3D_3BUFFER)
#if defined(SSE_VID_3BUFFER_392)
    || OPTION_3BUFFER_WIN && !FullScreen
#else
    || OPTION_3BUFFER 
#endif
#endif
#if !defined(SSE_VID_3BUFFER_FS)
    && !FullScreen
#endif
    ){ 
#else
  if (!draw_lock){
#endif
    draw_begin();
    int yy,yy2;
#ifndef NO_CRAZY_MONITOR
    if(extended_monitor){
      ASSERT(shifter_draw_pointer==xbios2);
      yy=0;yy2=min(em_height,Disp.SurfaceHeight);
    }else
#endif
#if defined(SSE_VID_BORDERS_GUI_392)
    if (border){
#else
    if (border & 1){
#endif
      yy=draw_first_scanline_for_border;yy2=draw_last_scanline_for_border;
    }else{
      yy=0;yy2=shifter_y;
    }
    for (;yy<yy2;yy++){
      scan_y=yy;
      scanline_drawn_so_far=0;
      shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
#if defined(SSE_SHIFTER)
      Shifter.DrawScanlineToEnd();
#else      
      draw_scanline_to_end();
#endif
#if defined(SSE_BOILER_XBIOS2) && defined(SSE_SHIFTER_TRICKS)
      DWORD Tricks=FrameEvents.GetShifterTricks(scan_y);
      if(Tricks&TRICK_TOP_OVERSCAN)
        shifter_first_draw_line=-29;
      else if(Tricks&(TRICK_BOTTOM_OVERSCAN|TRICK_BOTTOM_OVERSCAN_60HZ))
        shifter_last_draw_line=247;  
#endif
      //      shifter_draw_pointer+=shifter_scanline_width_in_bytes;
    }
    if (osd) osd_init_draw_static();
    draw_end();

    draw_blit();
  }
  scan_y=save_scan_y;
  shifter_draw_pointer=save_sdp;
  shifter_draw_pointer_at_start_of_line=save_sdp_at_start_of_line;
  scanline_drawn_so_far=save_drawn_so_far;
  shifter_pixel=save_pixel;
#if defined(SSE_BOILER_XBIOS2)
  shifter_first_draw_line=save_shifter_first_draw_line;
  shifter_last_draw_line=save_shifter_last_draw_line;
#endif

  DEBUG_ONLY( update_display_after_trace(); )
}
//---------------------------------------------------------------------------
#if !defined(SSE_VID_D3D_ONLY)
void get_fullscreen_rect(RECT *rc)
{
  if (draw_fs_blit_mode==DFSM_LAPTOP){
    rc->left=0;rc->top=0;
    rc->right=monitor_width;
    rc->bottom=monitor_height;
  }else
#ifndef NO_CRAZY_MONITOR
  if (extended_monitor){
    rc->left=0;
    rc->top=0;
    rc->right=min(em_width,Disp.SurfaceWidth);
    rc->bottom=min(em_height,Disp.SurfaceHeight);
    if (FullScreen && runstate!=RUNSTATE_RUNNING){
      int x_gap=(Disp.SurfaceWidth-em_width)/2;
      int y_gap=(Disp.SurfaceHeight-em_height)/2;
      rc->left+=x_gap;
      rc->right+=x_gap;
      rc->top+=y_gap;
      rc->bottom+=y_gap;
    }
  }else
#endif
  if (border & 1){
#if defined(SSE_VID_BORDERS_LB_DX)
    // not sure about this...
    rc->left=(800 - (SideBorderSizeWin+320+SideBorderSizeWin)*2)/2;
    rc->top=(600 - (BORDER_TOP+200+BORDER_BOTTOM)*2)/2;
    rc->right=rc->left + (SideBorderSizeWin+320+SideBorderSizeWin)*2;
    rc->bottom=rc->top + (BORDER_TOP+200+BORDER_BOTTOM)*2;
#else
    rc->left=(800 - (BORDER_SIDE+320+BORDER_SIDE)*2)/2;
    rc->top=(600 - (BORDER_TOP+200+BORDER_BOTTOM)*2)/2;
    rc->right=rc->left + (BORDER_SIDE+320+BORDER_SIDE)*2;
    rc->bottom=rc->top + (BORDER_TOP+200+BORDER_BOTTOM)*2;
#endif
  }else{
    rc->left=0;
    rc->top=int(using_res_640_400 ? 0:40);
    rc->right=640;
    rc->bottom=int(using_res_640_400 ? 400:440);
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
    if (overscan && (border==2)){ //hack overscan
      rc->top=0;
      rc->bottom=480;
    }
#endif
  }
}
#endif//#if !defined(SSE_VID_D3D_ONLY)
//---------------------------------------------------------------------------
void init_screen()
{
  
  draw_end();

#ifndef NO_CRAZY_MONITOR//SS as is:
/*  if(extended_monitor){  //crazy monitor
    shifter_x=em_width;shifter_y=em_height;
    res_vertical_scale=1;
  }else */
#endif
  // shifter_x/shifter_y/res_vertical_scale are used to get the source rect.
  switch (screen_res){
    case 0:
      shifter_x=320;shifter_y=200;
      res_vertical_scale=1;
      break;
    case 1:
      shifter_x=640;shifter_y=200;
      res_vertical_scale=1;
      break;
    case 2:
      shifter_x=640;shifter_y=400;
      res_vertical_scale=2;
      break;
  }

  // These are used to determine where to draw
  draw_first_scanline_for_border=res_vertical_scale*(-BORDER_TOP);
#if defined(SSE_VID_BORDERS)//should have redefined
  draw_last_scanline_for_border=shifter_y+res_vertical_scale*(BottomBorderSize);
#else
  draw_last_scanline_for_border=shifter_y+res_vertical_scale*(BORDER_BOTTOM);
#endif
  // This is used to know where to cause the timer B event
#if defined(SSE_INT_MFP_TIMER_B_392A)
  if(!OPTION_C2)
#endif
  CALC_CYCLES_FROM_HBL_TO_TIMER_B(shifter_freq);
//  res_change(); //all this does is resize the window - do we want that to happen?

  TRACE_LOG("init_screen() %dx%d,%d-%d\n",shifter_x,shifter_y,draw_first_scanline_for_border,draw_last_scanline_for_border);
}
//---------------------------------------------------------------------------
//SS as is:
/*void palette_convert_8(int n)
{
//  long col=((STpal[n] & 0x888) >> 3)+((STpal[n] & 0x777) << 1);  //correct STE colour
  long col=palette_table[(STpal[n] & 0xfff)];
  if (draw_lock) PCpal[n]=palette_add_entry(col);
}*/

/*
void old_palette_convert_16_555(int n){
  long col=((STpal[n] & 0x888) >> 3)+((STpal[n] & 0x777) << 1);  //correct STE colour
  PCpal[n]=((col & 0x00f) << 1) | ((col & 0x0f0) << 2) | ((col & 0xf00) << 3);
  *((WORD*)(&PCpal[n])+1)=LOWORD(PCpal[n]); //copy to long
}

void old_palette_convert_16_565(int n){
  long col=((STpal[n] & 0x888) >> 3)+((STpal[n] & 0x777) << 1);  //correct STE colour
  PCpal[n]=((col & 0x00f) << 1) | ((col & 0x0f0) << 3) | ((col & 0xf00) << 4);
  *((WORD*)(&PCpal[n])+1)=LOWORD(PCpal[n]); //copy to long
}

void old_palette_convert_24(int n){
  long col=((STpal[n] & 0x888) >> 3)+((STpal[n] & 0x777) << 1);  //correct STE colour
  PCpal[n]=((col & 0x00f) << 4) | ((col & 0x0f0) << 8)
         |((col & 0xf00) << 12) | ((col & 0x00f) << 28);
}

void old_palette_convert_32(int n){
  long col=((STpal[n] & 0x888) >> 3)+((STpal[n] & 0x777) << 1);  //correct STE colour
  PCpal[n]=((col & 0x00f) << 4) | ((col & 0x0f0) << 8) | ((col & 0xf00) << 12);
}
*/
//---------------------------------------------------------------------------
void res_change()
{
  TRACE_LOG("res_change()\n");
  if (ResChangeResize) StemWinResize();

  draw_set_jumps_and_source();
}
//---------------------------------------------------------------------------
bool draw_routines_init()
{
#if !defined(SSE_GLUE)
/*  Timings for each scanline, hbl and vbl interrupts were set once and for all
    for the three ST syncs: 50hz, 60hz, 72hz.
    This system was simple, fast and robust.
    It had 2 disadvantages:
    - Some memory use
    - Lack of flexibility. Programs on the ST can and do change sync during
    a frame, so that there's no guarantee of fixed frame timings. Typically,
    a 50hz frame would contain 60hz scanlines (508 cycles instead of 512).
    Steem could still display the frames fine in some cases, but timings during
    and at the end of the frame were wrong.
    Now, we compute the next screen event at each event check.
    See TGlue::GetNextScreenEvent().
    Advantages and disadvantages are the opposite of the previous system.
*/
  {
    event_plan[0]=event_plan_50hz;
    event_plan[1]=event_plan_60hz;
    event_plan[2]=event_plan_70hz;
    event_plan_boosted[0]=event_plan_boosted_50hz;
    event_plan_boosted[1]=event_plan_boosted_60hz;
    event_plan_boosted[2]=event_plan_boosted_70hz;

    /* 50Hz:
      VBLs spaced by 160256 cycles (49.92Hz)

      444 cycles:  HBL interrupt
      444+n*512, n=1..312:  draw scanline to end, HBL interrupt
      160256:  VBL
    */
/*
    At 50Hz, the ST image is made of 313 equivalent scan lines each of them
    equivalent to 512 CPU clock cycles. 
    This makes: 313*512 = 160256 CPU clock cycles. 
    But in fact the display will only show 274 of the 313 lines: 
    - 29 top border lines 
    - 200 normal lines 
    - 45 bottom border lines 
    The remaining 313-274 = 39 lines which correspond to 39*512 = 19968 CPU 
    clock cycles are used for the Vertical Blank. 
    [This is what the guy believed, that's why there's some trash at the very
    bottom of some Overscan Demos. There are 3 more bottom lines (48 total), 
    but monitors displaying those were rare.]
*/
    screen_event_struct *evp=event_plan_50hz;

    // SS for line 0, event is not scanline but hbl, but for all the others,
    // it is scanline, placed at the hbl timing..
    evp->time=CYCLES_FOR_VERTICAL_RETURN_IN_50HZ; // 444 -> this is the hack
    evp->event=event_hbl;

    evp++;
    for (int y=1;y<313;y++){
      evp->time=CYCLES_FOR_VERTICAL_RETURN_IN_50HZ + y*512;
      evp->event=event_scanline;
      evp++;
      if ((CYCLES_FOR_VERTICAL_RETURN_IN_50HZ+y*512) <= (160256-CYCLES_FROM_START_VBL_TO_INTERRUPT) &&
          (CYCLES_FOR_VERTICAL_RETURN_IN_50HZ+y*512+512) > (160256-CYCLES_FROM_START_VBL_TO_INTERRUPT)){
        evp->time=160256-CYCLES_FROM_START_VBL_TO_INTERRUPT; //SS -1544
        evp->event=event_start_vbl;
        evp++;
      }
    }
    evp->time=160256;
    evp->event=event_vbl_interrupt;
    evp++;
    evp->event=NULL;

    /* 60Hz:
      VBLs spaced by 133604 cycles (59.87Hz)

      444 cycles:  HBL interrupt
      444+n*508, n=1..262:  draw scanline to end, HBL interrupt
      133604:  VBL
    */
    evp=event_plan_60hz;

    evp->time=CYCLES_FOR_VERTICAL_RETURN_IN_60HZ;
    evp->event=event_hbl;
    evp++;
    for(int y=1;y<263;y++){
      evp->time=CYCLES_FOR_VERTICAL_RETURN_IN_60HZ + y*508;
      evp->event=event_scanline;
      evp++;
      if ((CYCLES_FOR_VERTICAL_RETURN_IN_60HZ+y*508) <= (133604-CYCLES_FROM_START_VBL_TO_INTERRUPT) &&
          (CYCLES_FOR_VERTICAL_RETURN_IN_60HZ+y*508+508) > (133604-CYCLES_FROM_START_VBL_TO_INTERRUPT)){
        evp->time=133604-CYCLES_FROM_START_VBL_TO_INTERRUPT;
        evp->event=event_start_vbl;
        evp++;
      }
    }
    evp->time=133604;
    evp->event=event_vbl_interrupt;
    evp++;
    evp->event=NULL;

    /* 70Hz:
      VBLs spaced by 112000 cycles (71.36Hz)

      200 cycles:  HBL interrupt
      200+n*224, n=1..452:  draw scanline to end, HBL interrupt
      112000:  VBL
    */
    evp=event_plan_70hz;

    for (int y=0;y < (SCANLINES_ABOVE_SCREEN_70HZ+400+SCANLINES_BELOW_SCREEN_70HZ);y++){
      evp->time=CYCLES_FOR_VERTICAL_RETURN_IN_70HZ + y*SCANLINE_TIME_IN_CPU_CYCLES_70HZ;
      evp->event=event_scanline;
      evp++;
      if ((CYCLES_FOR_VERTICAL_RETURN_IN_70HZ + y*SCANLINE_TIME_IN_CPU_CYCLES_70HZ) <= (112224-CYCLES_FROM_START_VBL_TO_INTERRUPT) &&
          (CYCLES_FOR_VERTICAL_RETURN_IN_70HZ + y*SCANLINE_TIME_IN_CPU_CYCLES_70HZ+SCANLINE_TIME_IN_CPU_CYCLES_70HZ)
             > (112224-CYCLES_FROM_START_VBL_TO_INTERRUPT)){
        evp->time=112224-CYCLES_FROM_START_VBL_TO_INTERRUPT;
        evp->event=event_start_vbl;
        evp++;
      }
    }
    evp->time=112224;
    evp->event=event_vbl_interrupt;
    evp++;
    evp->event=NULL;
  }
#endif//no plans

  PCpal=Get_PCpal(); // SS defined in asm_draw.asm

  // SS init to draw_scanline_dont (will draw nothing)
  for(int a=0;a<3;a++)
    for(int b=0;b<4;b++)
      for(int c=0;c<3;c++)
        jump_draw_scanline[a][b][c]=draw_scanline_dont;

  // [0=Smallest size possible, 1=640x400 (all reses), 2=640x200 (med/low res)]
  //  [BytesPerPixel-1]
  //    [screen_res]
//TODO: should init just one time?
  // SS Smallest size possible
  // 1 byte per pixel
  jump_draw_scanline[0][0][0]=draw_scanline_8_lowres_pixelwise; //LO
  jump_draw_scanline[0][0][1]=draw_scanline_8_medres_pixelwise; //ME
  jump_draw_scanline[0][0][2]=draw_scanline_8_hires; //HI
  // 2 bytes per pixel
  jump_draw_scanline[0][1][0]=draw_scanline_16_lowres_pixelwise; //LO
  jump_draw_scanline[0][1][1]=draw_scanline_16_medres_pixelwise; //ME
  jump_draw_scanline[0][1][2]=draw_scanline_16_hires; //HI
  // 3 bytes per pixel
  jump_draw_scanline[0][2][0]=draw_scanline_24_lowres_pixelwise; //LO
  jump_draw_scanline[0][2][1]=draw_scanline_24_medres_pixelwise; //ME
  jump_draw_scanline[0][2][2]=draw_scanline_24_hires; //HI
  // 4 bytes per pixel
  jump_draw_scanline[0][3][0]=draw_scanline_32_lowres_pixelwise; //LO
  jump_draw_scanline[0][3][1]=draw_scanline_32_medres_pixelwise; //ME
  jump_draw_scanline[0][3][2]=draw_scanline_32_hires; //HI

  // SS 640x400 (all reses)
  // 1 byte per pixel
  jump_draw_scanline[1][0][0]=draw_scanline_8_lowres_pixelwise_400; //LO
  jump_draw_scanline[1][0][1]=draw_scanline_8_medres_pixelwise_400; //ME
  jump_draw_scanline[1][0][2]=draw_scanline_8_hires; //HI
  // 2 bytes per pixel
  jump_draw_scanline[1][1][0]=draw_scanline_16_lowres_pixelwise_400; //LO
  jump_draw_scanline[1][1][1]=draw_scanline_16_medres_pixelwise_400; //ME
  jump_draw_scanline[1][1][2]=draw_scanline_16_hires; //HI
  // 3 bytes per pixel
  jump_draw_scanline[1][2][0]=draw_scanline_24_lowres_pixelwise_400; //LO
  jump_draw_scanline[1][2][1]=draw_scanline_24_medres_pixelwise_400; //ME
  jump_draw_scanline[1][2][2]=draw_scanline_24_hires; //HI
  // 4 bytes per pixel
  jump_draw_scanline[1][3][0]=draw_scanline_32_lowres_pixelwise_400; //LO
  jump_draw_scanline[1][3][1]=draw_scanline_32_medres_pixelwise_400; //ME
  jump_draw_scanline[1][3][2]=draw_scanline_32_hires; //HI

  // SS 640x200 (med/low res) dw=1
  // 1 byte per pixel
  jump_draw_scanline[2][0][0]=draw_scanline_8_lowres_pixelwise_dw; //LO
  jump_draw_scanline[2][0][1]=draw_scanline_8_medres_pixelwise; //ME
  jump_draw_scanline[2][0][2]=draw_scanline_8_hires; //HI
  // 2 bytes per pixel
  jump_draw_scanline[2][1][0]=draw_scanline_16_lowres_pixelwise_dw; //LO
  jump_draw_scanline[2][1][1]=draw_scanline_16_medres_pixelwise; //ME
  jump_draw_scanline[2][1][2]=draw_scanline_16_hires; //HI
  // 3 bytes per pixel
  jump_draw_scanline[2][2][0]=draw_scanline_24_lowres_pixelwise_dw; //LO
  jump_draw_scanline[2][2][1]=draw_scanline_24_medres_pixelwise; //ME
  jump_draw_scanline[2][2][2]=draw_scanline_24_hires; //HI
  // 4 bytes per pixel
  jump_draw_scanline[2][3][0]=draw_scanline_32_lowres_pixelwise_dw; //LO
  jump_draw_scanline[2][3][1]=draw_scanline_32_medres_pixelwise; //ME
  jump_draw_scanline[2][3][2]=draw_scanline_32_hires; //HI

  draw_scanline=draw_scanline_dont; // SS init the general pointer
//  palette_convert_entry=palette_convert_16_565;

  osd_routines_init();

  return true;
}
//---------------------------------------------------------------------------
#undef LOGSECTION

