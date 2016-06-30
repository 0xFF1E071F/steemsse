#pragma once
#ifndef OSD_DECLA_H
#define OSD_DECLA_H
#if defined(SSE_ACSI_LED)
//#include <EasyStringList.h>
#include <easystringlist.h>//ux382
#endif
#define EXT extern
#define INIT(s)

#define OSD_ICON_SIZE 24
#define OSD_LOGO_W 124
#define OSD_LOGO_H 11

#define CHANCE_OF_SCROLLER 8
#define OSD_SHOW_ALWAYS 0xff

EXT bool osd_no_draw INIT(0),osd_disable INIT(0);
EXT int osd_show_plasma INIT(4),osd_show_speed INIT(6);
EXT int osd_show_icons INIT(6),osd_show_cpu INIT(6);

EXT bool osd_old_pos INIT(0);

EXT bool osd_show_disk_light INIT(true),osd_show_scrollers INIT(true);
EXT DWORD osd_start_time,osd_scroller_start_time,osd_scroller_finish_time;
EXT bool osd_shown_scroller INIT(0);
EXT long col_yellow[2],col_blue,col_red,col_green,col_white;

EXT DWORD FDCCantWriteDisplayTimer INIT(0);

#if defined(STEVEN_SEAGAL) && defined(SSE_OSD_DRIVE_LED)
EXT long col_fd_red[2],col_fd_green[2];
#if !defined(SSE_OSD_DRIVE_LED2)
EXT DWORD FDCWriting INIT(0);
EXT DWORD FDCWritingTimer INIT(0);
#endif
EXT DWORD HDDisplayTimer INIT(0);
#endif

EXT void osd_draw_begin();
EXT void osd_init_run(bool);
EXT void osd_draw();
EXT void osd_hide();
EXT void osd_start_scroller(char*);
EXT void osd_draw_full_stop();
EXT void osd_draw_end();
EXT void osd_routines_init();
EXT void osd_init_draw_static();
EXT bool osd_is_on(bool);

#ifdef WIN32
EXT void osd_draw_reset_info(HDC);
#else
EXT void osd_draw_reset_info(int,int,int,int);
#endif
WIN_ONLY( EXT LRESULT __stdcall ResetInfoWndProc(HWND,UINT,WPARAM,LPARAM); )
WIN_ONLY( EXT HWND ResetInfoWin INIT(NULL); )

UNIX_ONLY( extern "C" long* Get_charset_blk(); )

//#ifdef IN_MAIN

void osd_pick_scroller();
EXT EasyStr get_osd_scroller_text(int n);

EXT EasyStr osd_scroller;
EXT EasyStringList osd_scroller_array;

EXT DWORD *osd_plasma_pal;
EXT BYTE *osd_plasma;

void osd_draw_plasma(int,int,int);

void ASMCALL osd_blueize_line_8(int,int,int);
//void osd_blueize_line_16_555(int x1,int y,int w);
//void osd_blueize_line_16_565(int x1,int y,int w);
//void osd_blueize_line_24(int x1,int y,int w);
//void osd_blueize_line_32(int x1,int y,int w);
void ASMCALL osd_blueize_line_dont(int,int,int);

void ASMCALL osd_draw_char_dont(long*,BYTE*,long,long,int,long,long);
void ASMCALL osd_draw_char_clipped_dont(long*,BYTE*,long,long,int,long,long,RECT*);

EXT long *osd_font;

typedef void ASMCALL OSDDRAWCHARPROC(long*,BYTE*,long,long,int,long,long);
typedef void ASMCALL OSDDRAWCHARCLIPPEDPROC (long*,BYTE*,long,long,int,long,long,RECT*);
typedef void ASMCALL OSDBLUEIZELINEPROC (int,int,int);
typedef void ASMCALL OSDBLACKRECTPROC (void*,int,int,int,int,long);
typedef OSDDRAWCHARPROC* LPOSDDRAWCHARPROC;
typedef OSDDRAWCHARCLIPPEDPROC* LPOSDDRAWCHARCLIPPEDPROC;
typedef OSDBLUEIZELINEPROC* LPOSDBLUEIZELINEPROC;
typedef OSDBLACKRECTPROC* LPOSDBLACKRECTPROC;

EXT LPOSDBLUEIZELINEPROC osd_blueize_line;
EXT LPOSDDRAWCHARPROC jump_osd_draw_char[4],osd_draw_char,
  jump_osd_draw_char_transparent[4],osd_draw_char_transparent;
EXT LPOSDDRAWCHARCLIPPEDPROC jump_osd_draw_char_clipped[4],osd_draw_char_clipped,
  jump_osd_draw_char_clipped_transparent[4],osd_draw_char_clipped_transparent;
EXT LPOSDBLACKRECTPROC jump_osd_black_box[4],osd_black_box;

extern "C"{
void ASMCALL osd_draw_char_clipped_8(long*,BYTE*,long,long,int,long,long,RECT*);
void ASMCALL osd_draw_char_clipped_16(long*,BYTE*,long,long,int,long,long,RECT*);
void ASMCALL osd_draw_char_clipped_24(long*,BYTE*,long,long,int,long,long,RECT*);
void ASMCALL osd_draw_char_clipped_32(long*,BYTE*,long,long,int,long,long,RECT*);

void ASMCALL osd_draw_char_8(long*,BYTE*,long,long,int,long,long);
void ASMCALL osd_draw_char_16(long*,BYTE*,long,long,int,long,long);
void ASMCALL osd_draw_char_24(long*,BYTE*,long,long,int,long,long);
void ASMCALL osd_draw_char_32(long*,BYTE*,long,long,int,long,long);

void ASMCALL osd_draw_char_transparent_8(long*,BYTE*,long,long,int,long,long);
void ASMCALL osd_draw_char_transparent_16(long*,BYTE*,long,long,int,long,long);
void ASMCALL osd_draw_char_transparent_24(long*,BYTE*,long,long,int,long,long);
void ASMCALL osd_draw_char_transparent_32(long*,BYTE*,long,long,int,long,long);

void ASMCALL osd_draw_char_clipped_transparent_8(long*,BYTE*,long,long,int,long,long,RECT*);
void ASMCALL osd_draw_char_clipped_transparent_16(long*,BYTE*,long,long,int,long,long,RECT*);
void ASMCALL osd_draw_char_clipped_transparent_24(long*,BYTE*,long,long,int,long,long,RECT*);
void ASMCALL osd_draw_char_clipped_transparent_32(long*,BYTE*,long,long,int,long,long,RECT*);

void ASMCALL osd_blueize_line_16_555(int,int,int),osd_blueize_line_16_565(int,int,int);
void ASMCALL osd_blueize_line_24(int,int,int),osd_blueize_line_32(int,int,int);

void ASMCALL palette_convert_16_555(int),palette_convert_line_16_565(int);
void ASMCALL palette_convert_line_24(int),palette_convert_line_32(int);
#if defined(SSE_COMPILER_382__)
void ASMCALL osd_black_box_8(void*,int,int,int,int,long,long);
void ASMCALL osd_black_box_16(void*,int,int,int,int,long,long);
void ASMCALL osd_black_box_24(void*,int,int,int,int,long,long);
void ASMCALL osd_black_box_32(void*,int,int,int,int,long,long);
#else
void ASMCALL osd_black_box_8(void*,int,int,int,int,long);
void ASMCALL osd_black_box_16(void*,int,int,int,int,long);
void ASMCALL osd_black_box_24(void*,int,int,int,int,long);
void ASMCALL osd_black_box_32(void*,int,int,int,int,long);
#endif
}

//#endif//IN_MAIN

#undef EXT
#undef INIT

#endif//OSD_DECLA_H