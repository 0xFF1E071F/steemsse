#pragma once
#ifndef STEMDOS_DECLA_H
#define STEMDOS_DECLA_H

#include <dirsearch.h>

#define EXT extern
#define INIT(s)

EXT bool mount_flag[26];
EXT EasyStr mount_path[26];
EXT bool stemdos_comline_read_is_rb INIT(0);

#ifndef DISABLE_STEMDOS

EXT void stemdos_intercept_trap_1();
EXT void stemdos_rte();
EXT void stemdos_set_drive_reset();
EXT void stemdos_update_drvbits();
EXT bool stemdos_check_mount(int);
EXT void stemdos_reset();
EXT int stemdos_get_boot_drive();
EXT void stemdos_check_paths();
EXT bool stemdos_any_files_open();
EXT void stemdos_control_c(); //control-c pressed
EXT void stemdos_close_all_files();

EXT void stemdos_init();

EXT int stemdos_boot_drive INIT(2);
EXT bool stemdos_intercept_datetime INIT(0);

//#ifdef IN_EMU

EXT EasyStr mount_gemdos_path[26];

#define STEMDOS_TRAP_1     \
  {MEM_ADDRESS original_return_address=m68k_lpeek(areg[7]+2);   \
    SET_PC(original_return_address);                            \
    m68k_interrupt(os_gemdos_vector);                              \
  }


#define GEMDOS_VECTOR LPEEK(0x84)

#define STEMDOS_RTE_GETDRIVE 0x02
#define STEMDOS_RTE_GETDIR 0x01
#define STEMDOS_RTE_DUP 0x03
#define STEMDOS_RTE_FCREATE 0x10
#define STEMDOS_RTE_FOPEN 0x20
#define STEMDOS_RTE_GET_DTA_FOR_FSFIRST 0x30
#define STEMDOS_RTE_FCLOSE 0x40
#define STEMDOS_RTE_DFREE 0x50
#define STEMDOS_RTE_MKDIR 0x60
#define STEMDOS_RTE_RMDIR 0x70
#define STEMDOS_RTE_FDELETE 0x80
#define STEMDOS_RTE_FATTRIB 0x90
#define STEMDOS_RTE_RENAME 0xa0
#define STEMDOS_RTE_PEXEC 0xb0
#define STEMDOS_RTE_MFREE 0xc0
#define STEMDOS_RTE_MFREE2 0xd0

#define STEMDOS_RTE_SUBACTION 0xf
#define STEMDOS_RTE_MAINACTION 0xf0

#define STEMDOS_FILE_IS_STEMDOS 0
#define STEMDOS_FILE_IS_GEMDOS 1
#define STEMDOS_FILE_ASKING 2

EXT int stemdos_rte_action;

void stemdos_get_PC_path();

typedef struct{
  bool open;
  FILE *f;
  int owner_program;
  Str filename;
  WORD date,time;
  DWORD attrib;
}stemdos_file_struct;
EXT stemdos_file_struct stemdos_file[46];
EXT stemdos_file_struct stemdos_new_file;
#if defined(SSE_VAR_RESIZE_382)
EXT BYTE stemdos_std_handle_forced_to[6];//h>=6 && h<46
#else
EXT int stemdos_std_handle_forced_to[6];
#endif

#define MAX_STEMDOS_FSNEXT_STRUCTS 100
typedef struct{
  MEM_ADDRESS dta;
  EasyStr path;
  EasyStr NextFile;
  int attr;
  DWORD start_hbl;
}stemdos_fsnext_struct_type;
EXT stemdos_fsnext_struct_type stemdos_fsnext_struct[MAX_STEMDOS_FSNEXT_STRUCTS];
//---------------------------------------------------------------------------
#if defined(SSE_VAR_RESIZE_382)
EXT BYTE stemdos_command;
EXT BYTE stemdos_attr;
#else
EXT int stemdos_command;
EXT int stemdos_attr;
#endif

//int stemdos_Fattrib_mode;
EXT EasyStr stemdos_filename;
EXT EasyStr stemdos_rename_to_filename;
EXT EasyStr PC_filename;

EXT FILE *stemdos_Pexec_file;
EXT MEM_ADDRESS stemdos_Pexec_com,stemdos_Pexec_env;
#if defined(SSE_VAR_RESIZE_382)
EXT BYTE stemdos_Pexec_mode;
#else
EXT int stemdos_Pexec_mode;
#endif
#define MAX_STEMDOS_PEXEC_LIST 76 //Change loadsave_emu.cpp if change this! 
void stemdos_add_to_Pexec_list(MEM_ADDRESS);
bool stemdos_mfree_from_Pexec_list();

EXT int stemdos_Pexec_list_ptr;

EXT MEM_ADDRESS stemdos_Pexec_list[MAX_STEMDOS_PEXEC_LIST];
EXT bool stemdos_ignore_next_pexec4;

//const char* PC_file_mode[3]={"rb","r+b","r+b"};

EXT MEM_ADDRESS stemdos_dfree_buffer;
#if defined(SSE_VAR_RESIZE_382)
EXT WORD stemdos_Fattrib_flag;
#else
EXT int stemdos_Fattrib_flag;
#endif
void stemdos_open_file(int);
void stemdos_close_file(stemdos_file_struct*);

void stemdos_read(int h,MEM_ADDRESS);
void stemdos_seek(int h,MEM_ADDRESS);
void stemdos_Fdatime(int h,MEM_ADDRESS);
void stemdos_Dfree(int dr,MEM_ADDRESS);
void stemdos_mkdir();
void stemdos_rmdir();
void stemdos_Fdelete();
void stemdos_rename();
void stemdos_Fattrib();
void stemdos_Pexec();
//---------------------------------------------------------------------------
void stemdos_fsfirst(MEM_ADDRESS),stemdos_fsnext();

int stemdos_get_file_path();
void stemdos_parse_path(); //remove \..\ etc.
//---------------------------------------------------------------------------
EXT MEM_ADDRESS stemdos_dta;

EXT short stemdos_save_sr;
#if defined(SSE_VAR_RESIZE_382)
EXT BYTE stemdos_current_drive;
#else
EXT int stemdos_current_drive;
#endif
#if defined(SSE_TOS_GEMDOS_NOINLINE)

void stemdos_trap_1_Fdup();
void stemdos_trap_1_Mfree(MEM_ADDRESS ad);
void stemdos_trap_1_Fgetdta();
void stemdos_trap_1_Fclose(int);
void stemdos_trap_1_Pexec_basepage();

#else

NOT_DEBUG(inline) void stemdos_trap_1_Fdup();
NOT_DEBUG(inline) void stemdos_trap_1_Mfree(MEM_ADDRESS ad);

/*
void inline stemdos_trap_1_Dgetdrv();
void inline stemdos_trap_1_Dgetpath();
*/
NOT_DEBUG(inline) void stemdos_trap_1_Fgetdta();
NOT_DEBUG(inline) void stemdos_trap_1_Fclose(int);
NOT_DEBUG(inline) void stemdos_trap_1_Pexec_basepage();

#endif

void stemdos_finished();
void stemdos_final_rte(); //clear stack from original GEMDOS call

/*
char stemdos_old_buffer[64];

MEM_ADDRESS stemdos_path_buffer;
void stemdos_save_path_buffer(MEM_ADDRESS ad);
void stemdos_restore_path_buffer();
*/
char* StrUpperNoSpecial(char*);

#if !defined(SSE_TOS_GEMDOS_VAR1) //function does nothing
void STStringToPC(char*),PCStringToST(char*);
#endif
//#endif//#ifdef IN_EMU

#endif//#ifndef DISABLE_STEMDOS

#ifdef SSE_TOS_STRUCT

struct TTos {
  BYTE LastPTermedProcess; //to go around (Steem) bug...
#if defined(SSE_STF_MATCH_TOS3)
  BYTE DefaultCountry;
#endif
#if defined(SSE_TOS_GEMDOS_RESTRICT_TOS3)
  BYTE VersionWarning;
#endif
#if defined(SSE_TOS_SNAPSHOT_AUTOSELECT2) //version with refactoring
  EasyStr GetNextTos(DirSearch &ds); // to enumerate TOS files
  void GetTosProperties(EasyStr Path,WORD &Ver,BYTE &Country,WORD &Date);
#endif
#if defined(SSE_VAR_KEYBOARD_CLICK2)
  void CheckKeyboardClick();
#endif
#if defined(SSE_TOS_GEMDOS_EM_381B)
  void HackMemoryForExtendedMonitor();
#endif
};

extern TTos Tos;

#endif

//#endif//#ifndef DISABLE_STEMDOS

#undef EXT
#undef INIT

#endif//#ifndef STEMDOS_DECLA_H
