#pragma once
#ifndef TRACE_DECLA_H
#define TRACE_DECLA_H

#define TDE_BEFORE 1
#define TDE_AFTER 2
#define TDE_TEXT_ONLY 0x80

typedef struct{
  short when;
  bool regflag;
  MEM_ADDRESS ad;
  char name[100];
  int bytes;
  unsigned long val[2];
  unsigned long*ptr;
}trace_display_entry;

#define MAX_TRACE_DISPLAY_ENTRIES 50
extern trace_display_entry t_d_e[MAX_TRACE_DISPLAY_ENTRIES];
extern unsigned short trace_sr_before,trace_sr_after;
extern MEM_ADDRESS trace_pc;
extern int trace_entries;
extern mem_browser m_b_trace;
extern HWND trace_window_handle;
extern HWND trace_repeat_trace_button;
extern HWND trace_hLABEL[MAX_TRACE_DISPLAY_ENTRIES];
extern HWND trace_sr_before_display,trace_sr_after_display;
extern ScrollControlWin trace_scroller;
extern bool trace_show_window;

LRESULT __stdcall trace_window_WndProc(HWND,UINT,WPARAM,LPARAM);
void trace_window_init();
void trace_init();
void trace_add_entry(char*name1,char*name2,short when,bool regflag,
    int bytes,MEM_ADDRESS ad);
void trace_add_movem_block(char*name,int,short when,int bytes,MEM_ADDRESS ad,int count);
void trace_get_after();
void trace_display();
void trace();
void trace_exception_display(m68k_exception*exc);
void trace_add_text(char*tt);


extern const char*bombs_name[12];

extern const char*exception_action_name[4];


#endif//#ifndef TRACE_DECLA_H