#pragma once
#ifndef ACC_DECLA_H
#define ACC_DECLA_H



#if defined(SSE_STRUCTURE_SSECPU_OBJ)///?
#include <easystr.h>
////#include <include.h> //no!
typedef EasyStr Str;//?!

#include <dynamicarray.h>

//#include <stdio.h>
//#include <SSE/SSEDebug.h>
#endif

#define EXT extern
#define INIT(s)



#if defined(STEVEN_SEAGAL) && defined(SSE_VID_SAVE_NEO)
WORD change_endian(WORD x); // double of something?
#endif

#ifdef ENABLE_LOGFILE

  #define log(s)  \
   {if(logsection_enabled[LOGSECTION]){ \
      if(!logging_suspended){            \
        log_write(s);                \
      }                               \
   }}

  #define logc(s)      \
    if (mode==STEM_MODE_CPU && logsection_enabled[LOGSECTION]) log_write(s);

  #define log_stack  \
   {if(logsection_enabled[LOGSECTION]){ \
      if(!logging_suspended){            \
        log_write_stack();                 \
      }                               \
   }}

  EXT void log_write(EasyStr);
  EXT void log_os_call(int trap);
  #define log_to_section(section,s) if (logsection_enabled[section] && logging_suspended==0) log_write(s);
  #define log_to(section,s)  if (logsection_enabled[section] && logging_suspended==0) log_write(s);
  EXT void log_write_stack();
#if defined(STEVEN_SEAGAL) && defined(SSE_VARIOUS)
  EXT bool logging_suspended INIT(TRUE);
#else
  EXT bool logging_suspended INIT(false);
#endif
  EXT bool logsection_enabled[100];
  EXT void log_io_write(MEM_ADDRESS,BYTE);

  #define CPU_INSTRUCTIONS_TO_LOG 10000
  EXT int log_cpu_count INIT(0);
  EXT void stop_cpu_log();

#ifdef DEBUG_BUILD
#if defined (SSE_DEBUG_CPU_TRACE_NO_STOP)
/*, The limit would uncheck the option at once. 
    Since we use the same option for the far more
    limited TRACE output, the limit is removed when logging is
    suspended (so that user still has choice).
*/
  #define LOG_CPU \
    if (log_cpu_count ||logging_suspended){ \
      log_to_section(LOGSECTION_CPU,HEXSl(pc,6)+": "+disa_d2(pc)); \
      if (!logging_suspended &&(--log_cpu_count)==0) stop_cpu_log(); \
    }

#elif defined(SSE_DEBUG_CPU_LOG_NO_STOP)
/*  A limited #instructions (10000) as defined would spare file IO,
    but sometimes you want more.
*/
  #define LOG_CPU log_to_section(LOGSECTION_CPU,HEXSl(pc,6)+": "+disa_d2(pc));
#else
  #define LOG_CPU \
    if (log_cpu_count){ \
      log_to_section(LOGSECTION_CPU,HEXSl(pc,6)+": "+disa_d2(pc)); \
      if ((--log_cpu_count)==0) stop_cpu_log(); \
    }
#endif
#else
  #define LOG_CPU
#endif

#if !(defined(STEVEN_SEAGAL) && defined(SSE_DEBUG_LOG_OPTIONS))
  #define LOGSECTION_ALWAYS 0
  #define LOGSECTION_FDC 1
  #define LOGSECTION_IO 2
  #define LOGSECTION_MFP_TIMERS 3
  #define LOGSECTION_INIT 4
  #define LOGSECTION_CRASH 5
  #define LOGSECTION_STEMDOS 6
  #define LOGSECTION_IKBD 7
  #define LOGSECTION_AGENDA 8
  #define LOGSECTION_INTERRUPTS 9
  #define LOGSECTION_TRAP 10
  #define LOGSECTION_SOUND 11
  #define LOGSECTION_VIDEO 12
  #define LOGSECTION_BLITTER 13
  #define LOGSECTION_MIDI 14
  #define LOGSECTION_TRACE 15
  #define LOGSECTION_SHUTDOWN 16
  #define LOGSECTION_SPEEDLIMIT 17
  #define LOGSECTION_CPU 18
  #define LOGSECTION_INIFILE 19
  #define LOGSECTION_GUI 20
  #define LOGSECTION_DIV 21
  #define LOGSECTION_PASTI 22
  #define NUM_LOGSECTIONS 23
#endif
  extern const char *name_of_mfp_interrupt[21];

//#ifdef IN_MAIN
  struct struct_logsection{
    char *Name;
    int Index;
  };
  
  EXT struct_logsection logsections[NUM_LOGSECTIONS+8];

  EXT const char *name_of_mfp_interrupt[21];

  //////////////////////////////// names of OS calls //////////////////////////////////
  EXT const char* gemdos_calls[0x58];

  EXT const char* bios_calls[12];

  EXT const char* xbios_calls[40];


  EXT FILE *logfile;
  EXT EasyStr LogFileName;

//#endif

#else
  #define log(s)
  #define logc(s)
  #define log_stack ;
#ifdef UNIX
  #define log_write(s) printf(s);printf("\n");
#else
  #define log_write(s)
#endif
  #define log_io_write(a,b)
  #define log_to_section(section,s)
  #define log_to(section,s)
  #define log_write_stack() ;
  #define LOG_CPU
#endif

#define log_DELETE_SOON(s) log_write(s);

EXT EasyStr HEXSl(long,int);
EXT int count_bits_set_in_word(unsigned short w);

EXT EasyStr read_string_from_memory(MEM_ADDRESS,int);
EXT MEM_ADDRESS write_string_to_memory(MEM_ADDRESS,char*);
EXT MEM_ADDRESS get_sp_before_trap(bool* DEFVAL(NULL));
EXT void acc_parse_search_string(Str,DynamicArray<BYTE>&,bool&);
EXT MEM_ADDRESS acc_find_bytes(DynamicArray<BYTE> &,bool,MEM_ADDRESS,int);

EXT bool STfile_read_error INIT(0);

EXT WORD STfile_read_word(FILE*f);
EXT LONG STfile_read_long(FILE*f);
EXT void STfile_read_to_ST_memory(FILE*f,MEM_ADDRESS ad,int n_bytes);
EXT void STfile_write_from_ST_memory(FILE*f,MEM_ADDRESS ad,int n_bytes);
EXT long colour_convert(int,int,int);

#ifdef ENABLE_LOGFILE
EXT Str scanline_cycle_log();
#endif

#ifdef IN_MAIN
void GetTOSKeyTableAddresses(MEM_ADDRESS *,MEM_ADDRESS *);
EasyStr time_or_times(int n);

char *reg_name(int n);
int get_text_width(char*t);

MEM_ADDRESS oi(MEM_ADDRESS,int);

EXT int how_big_is_0000;

EXT char reg_name_buf[8]; // SS removed _

int file_read_num(FILE*);

EasyStr DirID_to_text(int ID,bool st_key);

bool has_extension_list(char*,char*,...);
bool has_extension(char*,char*);
bool MatchesAnyString(char *,char *,...);
bool MatchesAnyString_I(char *,char *,...);

EasyStr GetEXEDir();
EasyStr GetCurrentDir();
EasyStr GetEXEFileName();

#endif

#undef EXT
#undef INIT


#endif//ACC_DECLA_H
