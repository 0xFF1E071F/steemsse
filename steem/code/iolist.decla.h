#pragma once
#ifndef IOLIST_DECLA_H
#define IOLIST_DECLA_H

#define EXT extern
#define INIT(s)

#define IOLIST_PSEUDO_AD 0x53000000
#define IOLIST_PSEUDO_AD_PSG (IOLIST_PSEUDO_AD+0x1000)
#define IOLIST_PSEUDO_AD_FDC (IOLIST_PSEUDO_AD+0x2000)
#define IOLIST_PSEUDO_AD_IKBD (IOLIST_PSEUDO_AD+0x3000)
#if defined(SS_DEBUG_BROWSER_6301)
#define IOLIST_PSEUDO_AD_6301 (IOLIST_PSEUDO_AD+0x4000)
#endif
#define IS_IOLIST_PSEUDO_ADDRESS(x) ((x&0xff000000)==IOLIST_PSEUDO_AD)


EXT void iolist_add_entry(MEM_ADDRESS ad,char*name,int bytes,char*bitmask=NULL,BYTE*ptr=NULL);

typedef struct{
  MEM_ADDRESS ad;
  EasyStr name;
  int bytes;
  EasyStr bitmask;
  BYTE*ptr;
}iolist_entry;

void iolist_init();
iolist_entry*search_iolist(MEM_ADDRESS);
int iolist_box_draw(HDC,int,int,int,int,iolist_entry*,BYTE*);
void iolist_box_click(int,iolist_entry*,BYTE*); //bit number clicked, toggle bit
int iolist_box_width(iolist_entry*);

#undef EXT
#undef INIT

#endif//IOLIST_DECLA_H