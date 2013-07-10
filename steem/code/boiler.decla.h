#pragma once
#ifndef BOILER_DECLA_H
#define BOILER_DECLA_H

#define EXT extern
#define INIT(s)

EXT MEM_ADDRESS dpc,old_dpc;
EXT HWND DWin,HiddenParent;
EXT HMENU menu,breakpoint_menu,monitor_menu,breakpoint_irq_menu;
EXT HMENU insp_menu;
EXT HMENU mem_browser_menu,history_menu,logsection_menu;
EXT HMENU menu1;
EXT HMENU boiler_op_menu,shift_screen_menu;
EXT HWND sr_display,DWin_edit;
EXT mr_static *lpms_other_sp;
EXT HWND DWin_trace_button,DWin_trace_over_button,DWin_run_button;

EXT Str LogViewProg;

void boiler_show_stack_display(int);
EXT ScrollControlWin DWin_timings_scroller;
EXT HWND DWin_right_display_combo;

EXT WNDPROC Old_sr_display_WndProc;

EXT mem_browser m_b_mem_disa,m_b_stack;

#define SIMULTRACE_CHOOSE ((HWND)(0xffffffff))
EXT HWND simultrace;

void debug_load_file_to_address(HWND,MEM_ADDRESS);

Str debug_parse_disa_for_display(Str);
EXT bool debug_monospace_disa,debug_uppercase_disa;

EXT bool d2_trace;

/////////////////////////////// insp menu ////////////////////////////////////
EXT int insp_menu_subject_type;
EXT void* insp_menu_subject;
EXT long insp_menu_long[3];
EXT char insp_menu_long_name[3][100];
EXT int insp_menu_long_bytes[3];
EXT void insp_menu_setup();
EXT int insp_menu_col,insp_menu_row;

/////////////////////////////// breakpoints ////////////////////////////////////
typedef struct{
  MEM_ADDRESS ad;
  int mode; // 0=off, 1=global, 2=break, 3=log
  int bwr; // & 1=break, & 2=write, & 4=read
  WORD mask[2]; // write mask, read mask
  char name[64];
}DEBUG_ADDRESS;

DEBUG_ADDRESS *debug_find_address(MEM_ADDRESS);
void debug_remove_address(MEM_ADDRESS);
void debug_update_bkmon();
void breakpoint_menu_setup();
void logfile_wipe();
void debug_set_bk(MEM_ADDRESS,bool);
void debug_set_mon(MEM_ADDRESS,bool,WORD);

EXT DynamicArray<DEBUG_ADDRESS> debug_ads;

/////////////////////////////// logfile  ////////////////////////////////////

//////////////////////////////// routines //////////////////////////////////
void update_register_display(bool);
void disa_to_file(FILE*f,MEM_ADDRESS dstart,int dlen,bool);
//---------------------------------------------------------------------------
EXT THistoryList HistList;

void debug_plugin_load(),debug_plugin_free();

#undef EXT
#undef INIT

#endif//BOILER_DECLA_H