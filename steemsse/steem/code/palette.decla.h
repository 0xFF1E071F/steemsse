#pragma once
#ifndef PALETTE_DECLA_H
#define PALETTE_DECLA_H

#define EXT extern

EXT void palette_copy();
EXT void palette_flip();
EXT WORD palette_add_entry(DWORD col);
EXT void palette_convert_all();
EXT void palette_convert(int);

//#ifdef IN_MAIN
EXT void palette_prepare(bool);
EXT void palette_remove();
EXT bool palette_changed;

EXT void make_palette_table(int brightness,int contrast);

#if defined(SSE_VAR_RESIZE_390)
EXT short brightness,contrast;
#else
EXT int brightness,contrast;
#endif
#if defined(SSE_VID_GAMMA)
EXT char *rgb_txt[3];
EXT short gamma[3]; // RGB
#endif

EXT int palhalf,palnum;

#ifdef WIN32
EXT HPALETTE winpal,oldwinpal;
EXT HDC PalDC;
#elif defined(UNIX)
EXT const long standard_palette[18][2];
EXT Colormap colormap;
EXT XColor new_pal[257];

#endif

EXT long logpal[257];
//#endif

#undef EXT

#endif//PALETTE_DECLA_H