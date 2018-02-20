#pragma once
#ifndef DRAW_DECLA_H
#define DRAW_DECLA_H

#include "SSE/SSEDecla.h"

#define EXT extern
#define EXTC extern "C"
#define INIT(s)

#if !defined(SSE_VID_BORDERS_BIGTOP)
#define BORDER_TOP 30
#endif

#if !defined(SSE_VID_BORDERS)
#define BORDER_SIDE 32 // redefined as variables! check SSEDecla.h
#define BORDER_BOTTOM 40
#endif

#define DFSM_FLIP 0
#define DFSM_STRAIGHTBLIT 1
#define DFSM_STRETCHBLIT 2
#define DFSM_LAPTOP 3

#define DFSFX_NONE 0
#define DFSFX_GRILLE 1
#define DFSFX_BLUR 2

EXT bool draw_routines_init();

EXT void init_screen();

EXT void draw_begin();
EXT void draw_end();
EXT bool draw_blit();
EXT void draw_set_jumps_and_source();

EXT void draw(bool);

EXT HRESULT change_fullscreen_display_mode(bool resizeclippingwindow);
EXT void change_window_size_for_border_change(int oldborder,int newborder);

EXT void res_change();


#if defined(SSE_VAR_RESIZE)
EXT BYTE bad_drawing INIT(0);
#if !defined(SSE_VID_D3D)
EXT BYTE draw_fs_blit_mode INIT( UNIX_ONLY(DFSM_STRAIGHTBLIT) WIN_ONLY(DFSM_STRETCHBLIT) );
#endif

#if defined(SSE_VID_D3D)
EXT BYTE draw_grille_black INIT(6);
#else
EXT BYTE draw_fs_fx INIT(DFSFX_NONE),draw_grille_black INIT(6);
#endif

#if defined(SSE_VID_DISABLE_AUTOBORDER) && !defined(SSE_VID_BORDERS_GUI_392)
EXT bool border INIT(1),border_last_chosen INIT(1);
#else
EXT BYTE border INIT(2),border_last_chosen INIT(2);
#endif
#if !defined(SSE_VID_D3D)
EXT BYTE draw_fs_topgap INIT(0);
extern BYTE prefer_pc_hz[2][3];
extern BYTE tested_pc_hz[2][3];
#endif
#else
EXT int bad_drawing INIT(0);
#if !defined(SSE_VID_D3D)
EXT int draw_fs_blit_mode INIT( UNIX_ONLY(DFSM_STRAIGHTBLIT) WIN_ONLY(DFSM_STRETCHBLIT) );
#endif
EXT int draw_fs_fx INIT(DFSFX_NONE),draw_grille_black INIT(6);
EXT int border INIT(2),border_last_chosen INIT(2);
EXT int draw_fs_topgap INIT(0);
extern int prefer_pc_hz[2][3];
extern WORD tested_pc_hz[2][3];
#endif

EXT RECT draw_blit_source_rect;

#define DWM_STRETCH 0
#define DWM_NOSTRETCH 1
#define DWM_GRILLE 2

WIN_ONLY( EXT int draw_win_mode[2]; ) // Inited by draw_fs_blit_mode

EXTC BYTE FullScreen INIT(0);

EXT bool draw_lock;

extern "C"
{
EXT BYTE *draw_mem;
EXT int draw_line_length;
EXT long *PCpal;
EXT WORD STpal[16];
EXT BYTE *draw_dest_ad,*draw_dest_next_scanline;
}

#define OVERSCAN_MAX_COUNTDOWN 25
#if defined(SSE_VID_BPP_NO_CHOICE)
EXT const BYTE display_option_8_bit_fs INIT(0);
#elif defined(SSE_VID_BPP_CHOICE)
EXT BYTE display_option_fs_bpp INIT(0);
#else
EXT bool display_option_8_bit_fs INIT(false);
#endif
#if !defined(SSE_VID_D3D)
EXT bool prefer_res_640_400 INIT(0),using_res_640_400 INIT(0);
#endif

#if !defined(SSE_VID_D3D)
EXT void get_fullscreen_rect(RECT *);
#endif
#if defined(SSE_VAR_RESIZE)
EXT char overscan INIT(0)
#else
EXT int overscan INIT(0)
#endif
#if !defined(SSE_VAR_RESIZE)
,stfm_borders INIT(0)
#endif
;

UNIX_ONLY( EXT int x_draw_surround_count INIT(4); )

WIN_ONLY( EXT HWND ClipWin; )



//#ifdef IN_EMU



#if defined(SSE_VAR_RESIZE)
EXT short cpu_cycles_from_hbl_to_timer_b;
#else
EXT int cpu_cycles_from_hbl_to_timer_b;
#endif


#define SCANLINES_ABOVE_SCREEN_50HZ 63 
#define SCANLINES_ABOVE_SCREEN_60HZ 34
#if defined(SSE_VID_HIRES_BORDER_FIX)
#define SCANLINES_ABOVE_SCREEN_70HZ 34
#else
#define SCANLINES_ABOVE_SCREEN_70HZ 61
#endif
#define SCANLINES_BELOW_SCREEN_50HZ 50
#define SCANLINES_BELOW_SCREEN_60HZ 29
#if defined(SSE_VID_HIRES_BORDER_FIX)
#define SCANLINES_BELOW_SCREEN_70HZ (40+(61-34)) // not used anyway
#else
#define SCANLINES_BELOW_SCREEN_70HZ 40
#endif
#define SCANLINE_TIME_IN_CPU_CYCLES_50HZ 512
#define SCANLINE_TIME_IN_CPU_CYCLES_60HZ 508
#define SCANLINE_TIME_IN_CPU_CYCLES_70HZ 224
#define CYCLES_FOR_VERTICAL_RETURN_IN_50HZ 444
#define CYCLES_FOR_VERTICAL_RETURN_IN_60HZ 444
#define CYCLES_FOR_VERTICAL_RETURN_IN_70HZ 200
#define CYCLES_FROM_START_VBL_TO_INTERRUPT 1544 //SS same for all freqs?
#define CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN 84 //SS 84 = 56+28
#define CYCLES_FROM_HBL_TO_RIGHT_BORDER_CLOSE (CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320)

#define CALC_CYCLES_FROM_HBL_TO_TIMER_B(freq) \
  switch (freq){ \
    case MONO_HZ: cpu_cycles_from_hbl_to_timer_b=192;break; \
    case 60: cpu_cycles_from_hbl_to_timer_b=(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320-4);break; \
    default: cpu_cycles_from_hbl_to_timer_b=(CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN+320); \
}

#define HBLS_PER_SECOND_AVE 15700 // Average between 50 and 60hz
#define HBLS_PER_SECOND_MONO (501.0*71.42857)


#if defined(SSE_VAR_RESIZE)
EXT const BYTE scanlines_above_screen[4];

EXT const WORD scanline_time_in_cpu_cycles_8mhz[4];
#else
EXT const int scanlines_above_screen[4];

EXT const int scanline_time_in_cpu_cycles_8mhz[4];
#endif
EXT int scanline_time_in_cpu_cycles[4];

EXT int draw_dest_increase_y;

EXT int res_vertical_scale;
EXT int draw_first_scanline_for_border,draw_last_scanline_for_border; //calculated from BORDER_TOP, BORDER_BOTTOM and res_vertical_scale

EXT int draw_first_possible_line,draw_last_possible_line;

void inline draw_scanline_to_end();

#if !defined(SSE_VIDEO_CHIPSET)
void inline draw_scanline_to(int);
#endif

EXT int scanline_drawn_so_far;
#ifndef SSE_VAR_DEAD_CODE
EXT int cpu_cycles_when_shifter_draw_pointer_updated;
#endif
EXT int left_border,right_border;
#if !defined(SSE_VIDEO_CHIPSET)
EXT bool right_border_changed;
#endif
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
EXT int overscan_add_extra;
#endif
//LPSCANPROC draw_scanline;

typedef void ASMCALL PIXELWISESCANPROC(int,int,int,int);
typedef PIXELWISESCANPROC* LPPIXELWISESCANPROC;
//LPRASTERWISESCANPROC jump_draw_scanline[2][4][3],draw_scanline;
EXT LPPIXELWISESCANPROC jump_draw_scanline[3][4][3],draw_scanline,draw_scanline_lowres,draw_scanline_medres;
void ASMCALL draw_scanline_dont(int,int,int,int);

//LPPALETTECONVERTPROC palette_convert_entry=NULL;
//void palette_convert_entry_dont(int){}
//void palette_convert_16_555(int);
//void palette_convert_16_565(int);
//void palette_convert_24(int);
//void palette_convert_32(int);


extern "C"{

long* ASMCALL Get_PCpal();
//void palette_convert_16_555(int), palette_convert_16_565(int);
//void palette_convert_24(int),     palette_convert_32(int);

//void draw_scanline_8_lowres(int,int,int), draw_scanline_16_lowres(int,int,int);
//void draw_scanline_24_lowres(int,int,int),draw_scanline_32_lowres(int,int,int);

//void draw_scanline_8_lowres_400(int,int,int), draw_scanline_16_lowres_400(int,int,int);
//void draw_scanline_24_lowres_400(int,int,int),draw_scanline_32_lowres_400(int,int,int);

void ASMCALL draw_scanline_8_lowres_pixelwise(int,int,int,int), draw_scanline_16_lowres_pixelwise(int,int,int,int);
void ASMCALL draw_scanline_24_lowres_pixelwise(int,int,int,int),draw_scanline_32_lowres_pixelwise(int,int,int,int);

void ASMCALL draw_scanline_8_lowres_pixelwise_dw(int,int,int,int), draw_scanline_16_lowres_pixelwise_dw(int,int,int,int);
void ASMCALL draw_scanline_24_lowres_pixelwise_dw(int,int,int,int),draw_scanline_32_lowres_pixelwise_dw(int,int,int,int);

void ASMCALL draw_scanline_8_lowres_pixelwise_400(int,int,int,int), draw_scanline_16_lowres_pixelwise_400(int,int,int,int);
void ASMCALL draw_scanline_24_lowres_pixelwise_400(int,int,int,int),draw_scanline_32_lowres_pixelwise_400(int,int,int,int);

//void draw_scanline_8_lowres_scrolled(int,int,int), draw_scanline_16_lowres_scrolled(int,int,int);
//void draw_scanline_24_lowres_scrolled(int,int,int),draw_scanline_32_lowres_scrolled(int,int,int);

//void draw_scanline_8_lowres_scrolled_400(int,int,int), draw_scanline_16_lowres_scrolled_400(int,int,int);
//void draw_scanline_24_lowres_scrolled_400(int,int,int),draw_scanline_32_lowres_scrolled_400(int,int,int);

//void draw_scanline_8_medres(int,int,int), draw_scanline_16_medres(int,int,int);
//void draw_scanline_24_medres(int,int,int),draw_scanline_32_medres(int,int,int);

void ASMCALL draw_scanline_8_medres_pixelwise(int,int,int,int), draw_scanline_16_medres_pixelwise(int,int,int,int);
void ASMCALL draw_scanline_24_medres_pixelwise(int,int,int,int),draw_scanline_32_medres_pixelwise(int,int,int,int);

//void draw_scanline_8_medres_400(int,int,int), draw_scanline_16_medres_400(int,int,int);
//void draw_scanline_24_medres_400(int,int,int),draw_scanline_32_medres_400(int,int,int);

void ASMCALL draw_scanline_8_medres_pixelwise_400(int,int,int,int), draw_scanline_16_medres_pixelwise_400(int,int,int,int);
void ASMCALL draw_scanline_24_medres_pixelwise_400(int,int,int,int),draw_scanline_32_medres_pixelwise_400(int,int,int,int);

void ASMCALL draw_scanline_8_hires(int,int,int,int), draw_scanline_16_hires(int,int,int,int);
void ASMCALL draw_scanline_24_hires(int,int,int,int),draw_scanline_32_hires(int,int,int,int);


}
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
#define OVERSCAN_ADD_EXTRA_FOR_LEFT_BORDER_REMOVAL 2
#define OVERSCAN_ADD_EXTRA_FOR_SMALL_LEFT_BORDER_REMOVAL 2
#define OVERSCAN_ADD_EXTRA_FOR_GREAT_BIG_RIGHT_BORDER -106
#define OVERSCAN_ADD_EXTRA_FOR_EARLY_RIGHT_BORDER -2
#define OVERSCAN_ADD_EXTRA_FOR_RIGHT_BORDER_REMOVAL 28

#define ADD_EXTRA_TO_SHIFTER_DRAW_POINTER_AT_END_OF_LINE(s) \
          s+=(shifter_fetch_extra_words)*2;     \
          if (shifter_skip_raster_for_hscroll){                \
            if (left_border){ \
              s+=8;                     \
            }else{     \
              s+=2;                     \
            } \
          }                           \
          s+=overscan_add_extra;
#endif
#if defined(SSE_TIMINGS_CPUTIMER64)
EXT COUNTER_VAR shifter_freq_change_time[32];
#else
EXT int shifter_freq_change_time[32];
#endif
#if defined(SSE_VAR_RESIZE)
EXT BYTE shifter_freq_change[32];
EXT BYTE shifter_freq_change_idx;
#else
EXT int shifter_freq_change[32];
EXT int shifter_freq_change_idx;
#endif

#if defined(SSE_SHIFTER_TRICKS)
// keeping a record for shift mode changes as well
EXT COUNTER_VAR shifter_shift_mode_change_time[32];
#if defined(SSE_VAR_RESIZE)
EXT BYTE shifter_shift_mode_change[32];
EXT BYTE shifter_shift_mode_change_idx;
#else
EXT int shifter_shift_mode_change[32];
EXT int shifter_shift_mode_change_idx;
#endif
#endif


#ifdef WIN32
// This is for the new scanline buffering (v2.6). If you write a lot direct
// to video memory it can be very slow due to recasching, so if the surface is
// in vid mem we set draw_buffer_complex_scanlines. This means that in
// draw_scanline_to we change draw_dest_ad to draw_temp_line_buf and
// set draw_scanline to draw_scanline_1_line. In draw_scanline_to_end
// we then copy from draw_temp_line_buf to the old draw_dest_ad and
// restore draw_scanline.

#if defined(SSE_VID_BORDERS)
//EXT BYTE draw_temp_line_buf[800*4+16+ 200 ]; // overkill but I can't count
EXT BYTE draw_temp_line_buf[800*4+16+ 400 ]; // overkill but I can't count
#else
EXT BYTE draw_temp_line_buf[800*4+16]; 
#endif

EXT BYTE *draw_store_dest_ad;
EXT LPPIXELWISESCANPROC draw_scanline_1_line[2],draw_store_draw_scanline;
EXT bool draw_buffer_complex_scanlines;
#endif//WIN32

EXT bool draw_med_low_double_height;

EXT bool draw_line_off;

#if !defined(SSE_SHIFTER_TRICKS)
#define ADD_SHIFTER_FREQ_CHANGE(f) \
  {shifter_freq_change_idx++;shifter_freq_change_idx&=31; \
  shifter_freq_change_time[shifter_freq_change_idx]=ABSOLUTE_CPU_TIME; \
  shifter_freq_change[shifter_freq_change_idx]=(f);                    \
  log_to_section(LOGSECTION_VIDEO,EasyStr("VIDEO: Change to freq ")+(f)+      \
            " at time "+ABSOLUTE_CPU_TIME);}
#endif

EXT bool freq_change_this_scanline;

#if !defined(SSE_VIDEO_CHIPSET)
void draw_check_border_removal();
#endif

//#endif//inemu

#undef EXT
#undef EXTC
#undef INIT

#endif//DRAW_DECLA_H