#ifdef IN_EMU
#define EXT
#define INIT(s) =s
#else
#define EXT extern
#define INIT(s)
#endif

/*                           ATARI ST COMPUTER


                   ----------                    ----------
                  |Graphics  |                  |Music     |
                  |Subsystem |                  |Subsystem |
                   ----------                    ----------
                        |                            |
                        |         ----------         |
                         --------| Main     |--------
                                 | System   |
                                  ----------
                                       |
                                       |         ----------
                  The ST                --------|Device    |
                                                |Subsystems|
                                                 ----------



               The hardware architecture of the Atari  Corporation  ST
          (Sixteen/Thirty-two) computer system consists of a main sys-
          tem, a graphics subsystem, a music  subsystem,  and  several
          device  subsystems (most of the device subsystems require ST
          resident intelligence).  The ST is based on the  MC68000  16
          bit  data/24  bit  address  microprocessor  unit  capable of
          directly accessing up to 16 Mbytes of ROM  and  RAM  memory.
          Hardware features of the ST computer system include:


          Main System
                  o  16 bit data/24 bit address 8 MHz microprocessor unit
                  o  192 Kbyte ROM, cartridge expandable to 320 Kbyte
                  o  512 Kbyte RAM or 1 Mbyte RAM
                  o  direct memory access support

          Graphics Subsystem
                  o  32 Kbyte BitMap video display memory (from above)
                  o  320 x 200 pixel, 16 color palette from 512 selections
                  o  640 x 200 pixel, 4 color palette from 512 selections
                  o  640 x 400 pixel, monochrome

          Music Subsystem
                  o  programmable sound synthesizer
                  o  musical instrument network communication

          Device Subsystems
                  o  intelligent keyboard
                  o  two button mouse
                  o  RGB color and monochrome monitor interfaces
                  o  printer parallel interface
                  o  RS232 serial interface
                  o  MIDI musical instrument interface
                  o  on board floppy disk controller and DMA interface
                  o  hard disk drive DMA interface



               The following is a  simplified  hardware  system  block
          diagram of the Atari ST:

           ---------------
          | MC68000 MPU   |<--
          |               |   |
           ---------------    |
                              |                           ----------
                              |<------------------------>|192 Kbyte |<--->EXPAN
                     ---------|------------------------->| ROM      |
                    |         |                           ----------
                    |         |                           ----------
                    |         |                          |512K or 1M|
                    |         |                       -->| byte RAM |<--
            ----------        |        ----------    |    ----------    |
           | Control  |<----->|<----->| Memory   |<--                   |
           | Logic    |-------|------>|Controller|<--                   |
            ----------        |        ----------    |    ----------    |
             |||||            |        ----------     -->| Video    |<--  RF MOD
             |||||            |<----->| Buffers  |<----->| Shifter  |---->RGB
             |||||            |       |          |        ----------      MONO
             |||||            |        ----------
             |||||            |        ----------         ----------
             |||||            |<----->| MC6850   |<----->| Keyboard |<--->IKBD
             |||| ------------|------>| ACIA     |       | Port     |
             ||||             |        ----------         ----------
             ||||             |        ----------         ----------
             ||||             |<----->| MC6850   |<----->| MIDI     |---->OUT/THRU
             ||| -------------|------>| ACIA     |       | Ports    |<----IN
             |||              |        ----------         ----------
             |||              |        ----------         ----------
             |||              |<----->| MK68901  |<----->| RS232    |<--->MODEM
             || --------------|------>| MFP      |<--    | Port     |
             ||               |        ----------    |    ----------
             ||               |                      |    ----------
             ||               |                       ---| Parallel |<--->PRINTER
             ||               |                       -->| Port     |
             ||               |        ----------    |    ----------
             ||               |<----->| YM-2149  |<--     ----------
             | ---------------|------>| PSG      |------>| Sound    |---->AUDIO
             |                |       |          |---    | Channels |
             |                |        ----------    |    ----------
             |                |                      |    ----------
             |                |        ----------     -->| Floppy   |<--->FLOPPY
             |                |<------| WD1772   |<----->|Disk Port |     DRIVE
             |                |    -->| FDC      |        ----------
             |                |   |    ----------
             |                |   |    ----------         ----------
             |                |    -->| DMA      |<----->|Hard Disk |<--->HARD
             |                |<----->|Controller|       | Port     |     DRIVE
              ----------------------->|          |        ----------
                                       ----------



ST memory bandwidth is 8MB/s - 4 milions cycles /sec
2 milions/sec is granted to Shifter/SoundDMA, other 2milions/sec to CPU/Blitter/FDD HDD DMA.
There is no possibility to get Shifter's cycles by CPU/Blitter because MMU blocks access to them (even when those cycles are not in use).
Every CPU/Blitter memory access to ST RAM is always rounded to 4 cycles.
In case of 6cycle 68000 instruction, that instruction is rounded to 8, or paired with other instruction (more info on pasti site).
- There are two main data bus domains in the ST, the DRAM bus and the CPU bus.
- The two buses work together for CPU, DMA and Blitter DRAM accesses and independently at all other times. This is managed by a bus gateway controlled by the MMU.
- In each 0.5us period half is allocated to the MMU / Shifter ('MMU phase') and the other half to the CPU / DMA / Blitter ('CPU phase').
- The MMU phase contains either a DRAM refresh or a Shifter video access. There is no way to disable the refresh and force it to hand the phase to the CPU instead.
- When the CPU or another CPU-bus device starts a memory access, if the access is to DRAM and would not be aligned with the CPU phase, two wait states are inserted (by delay of DTACK) to align the CPU access with the CPU phase.
- The Blitter duplicates the CPU asynchronous bus protocol, so behaves (nominally) like the 68000.


*/


EXT int MILLISECONDS_TO_HBLS(int);
EXT void make_Mem(BYTE,BYTE);
EXT void GetCurrentMemConf(BYTE[2]);

EXT BYTE *Mem INIT(NULL),*Rom INIT(NULL);


//SS About memory:
/*
6.1	Memory

The ST’s RAM is stored in the variable Mem, which is declared in emulator.h and set 
up in the make_Mem() function in main.cpp.

The ST’s ROM is stored in the variable Rom, which is declared in emulator.h and set 
up in the Initialise() function in main.cpp.

On little-endian processors such as the 486-compatibles used in modern PCs, the issue 
arises of storing data for use by a big-endian processor such as an emulated Motorola 
68000 (the ST’s CPU).  For example, imagine that a computer has a byte $AB stored at 
address $100 and $CD stored at address $101.  If the computer is big-endian, reading 
a word (two bytes) from the address $100 will return $ABCD.  If the computer is
 little-endian, it will return $CDAB.  Thus if a little-endian computer is emulating 
a big-endian one, and the naïve storage method is used for the emulated computer’s
 memory, then each word and longword needs to be reversed before writing to and after
 reading from the emulated memory.  To avoid this overhead, Steem stores the ST memory 
in reverse order in the PC’s memory.  We store the address immediately after the last
 byte of the storage buffer (the variable Mem_End), and calculate PC addresses from 
ST addresses by subtracting the address and the data length from this address. 
 Hence the ST’s word at $100 is stored in the PC’s memory at (Mem_End-2-$100). 
 Macros are provided to hide this confusing calculation, for example

#define DPEEK(l)   *(WORD*)(Mem_End_minus_2-(l))

in steemh.h.


SS:

I find the big or little-endian definition confusing.
It's "big" if the RAM position is "bigger" (comes after).
In big-endian processors, we have AB CD; CD is on a higher, bigger RAM position
than AB.
In practice, little-endian processors (like Intel) reverse all the data they write
in memory (CDAB)

The way the Steem Authors dealt with the situation is a stroke of genius (or they 
took it from another genius...) 
By "naive" we must understand WinSTon and maybe Hatari.
(In fact it seems to be an option in Hatari?)
Mem_End is the end of the allocated PC RAM, but it's the beginning
of the ST RAM!

*/



EXT WORD tos_version;

#define COLOUR_MONITOR (mfp_gpip_no_interrupt & MFP_GPIP_COLOUR)
#define MONO (!(mfp_gpip_no_interrupt&MFP_GPIP_COLOUR))

#define ON_RTE_RTE 0
#define ON_RTE_STEMDOS 1
#define ON_RTE_LINE_A 2
#define ON_RTE_EMHACK 3
#define ON_RTE_DONE_MALLOC_FOR_EM 4
#define ON_RTE_STOP 400

#if defined(SS_CPU_LINE_F)
#define ON_RTE_LINE_F 5
#endif

EXT int interrupt_depth INIT(0);
EXT int em_width INIT(480);
EXT int em_height INIT(480);
EXT int em_planes INIT(4);
EXT int extended_monitor INIT(0);
EXT DWORD n_cpu_cycles_per_second INIT(8000000),new_n_cpu_cycles_per_second INIT(0),n_millions_cycles_per_sec INIT(8);
EXT int on_rte;
EXT int on_rte_interrupt_depth;

extern "C"
{
EXT MEM_ADDRESS shifter_draw_pointer;
EXT int shifter_hscroll,shifter_skip_raster_for_hscroll;
}

EXT MEM_ADDRESS xbios2,shifter_draw_pointer_at_start_of_line;
EXT int shifter_pixel;
EXT int shifter_freq INIT(60);
EXT int shifter_freq_idx INIT(1);
EXT int shifter_x,shifter_y;
EXT int shifter_first_draw_line;
EXT int shifter_last_draw_line;
EXT int shifter_scanline_width_in_bytes;
EXT int shifter_fetch_extra_words;
EXT bool shifter_hscroll_extra_fetch;
EXT int screen_res INIT(0);
EXT int scan_y;

#define SVmemvalid 0x420
#define SVmemctrl 0x424
#define SVphystop 0x42e
#define SV_membot 0x432
#define SV_memtop 0x436
#define SVmemval2 0x43a
#define SVscreenpt 0x45e

#define SVsshiftmd 0x44c
#define SV_v_bas_ad 0x44e

#define SV_drvbits 0x4c2

#define MEMCONF_128 0
#define MEMCONF_512 1
#define MEMCONF_2MB 2

// The MMU config never gets set to this, but we use it to distinguish between
// 640Kb (retained for legacy reasons) and 512Kb.
#define MEMCONF_0   3
#define MEMCONF_512K_BANK1_CONF MEMCONF_0
#define MEMCONF_2MB_BANK1_CONF MEMCONF_0

#define MEMCONF_7MB 4  

#define MB2 (2*1024*1024)
#define KB512 (512*1024)
#define KB128 (128*1024)

#if !defined(SS_MMU_NO_CONFUSION)
EXT MEM_ADDRESS mmu_confused_address(MEM_ADDRESS ad);
extern "C"{
BYTE ASMCALL mmu_confused_peek(MEM_ADDRESS ad,bool cause_exception);
WORD ASMCALL mmu_confused_dpeek(MEM_ADDRESS ad,bool cause_exception);
LONG ASMCALL mmu_confused_lpeek(MEM_ADDRESS ad,bool cause_exception);
void ASMCALL mmu_confused_set_dest_to_addr(int bytes,bool cause_exception);
}
#endif

EXT BYTE mmu_memory_configuration;

EXT MEM_ADDRESS mmu_bank_length[2];
EXT MEM_ADDRESS bank_length[2];

extern const MEM_ADDRESS mmu_bank_length_from_config[5];

extern void intercept_os();

extern void call_a000();

WIN_ONLY( EXT CRITICAL_SECTION agenda_cs; )

EXT void ASMCALL emudetect_falcon_draw_scanline(int,int,int,int);
EXT void emudetect_falcon_palette_convert(int);
EXT void emudetect_init();
//---------------------------------------------------------------------------
#ifdef IN_EMU

const MEM_ADDRESS mmu_bank_length_from_config[5]=
                  {128*1024,512*1024,2*1024*1024,0,7*1024*1024};
                  
void init_timings();

#define OS_CALL 0
#define OS_NO_CALL true

MEM_ADDRESS os_gemdos_vector=0,os_bios_vector=0,os_xbios_vector=0;
void intercept_gemdos(),intercept_bios(),intercept_xbios();

void emudetect_reset();

bool emudetect_called=0,emudetect_write_logs_to_printer=0,emudetect_overscans_fixed=false;

#define EMUD_FALC_MODE_OFF 0
#define EMUD_FALC_MODE_8BIT 1
#define EMUD_FALC_MODE_16BIT 2

BYTE emudetect_falcon_mode=EMUD_FALC_MODE_OFF;
BYTE emudetect_falcon_mode_size=0;
bool emudetect_falcon_extra_height=0;

DynamicArray<DWORD> emudetect_falcon_stpal;
DynamicArray<DWORD> emudetect_falcon_pcpal;

BYTE snapshot_loaded=0;

#ifndef NO_CRAZY_MONITOR
void extended_monitor_hack();
#endif

bool vbl_pending=false;

#define MAX_AGENDA_LENGTH 32

typedef void AGENDAPROC(int);
typedef AGENDAPROC* LPAGENDAPROC;

struct AGENDA_STRUCT{ // SS removed _
  LPAGENDAPROC perform;
  unsigned long time;
  int param;
}agenda[MAX_AGENDA_LENGTH];
int agenda_length=0;
unsigned long agenda_next_time=0x7fffffff;

void agenda_add(LPAGENDAPROC,int,int);
void agenda_delete(LPAGENDAPROC);

void agenda_keyboard_replace(int);

int agenda_get_queue_pos(LPAGENDAPROC);
//void inline agenda_process();
void agenda_acia_tx_delay_IKBD(int),agenda_acia_tx_delay_MIDI(int);


MEM_ADDRESS on_rte_return_address;


#if !(defined(STEVEN_SEAGAL) && defined(SS_CPU))
#define M68K_UNSTOP                         \
  if (cpu_stopped){ \
                   \
                  cpu_stopped=false;     \
                  SET_PC((pc+4) | pc_high_byte);          \
  }
#endif

// This list is used to reinit the agendas after loading a snapshot
// add any new agendas to the end of the list, replace old agendas
// with NULL.
LPAGENDAPROC agenda_list[]={agenda_fdc_spun_up,agenda_fdc_motor_flag_off,agenda_fdc_finished,
                          agenda_floppy_seek,agenda_floppy_readwrite_sector,agenda_floppy_read_address,
                          agenda_floppy_read_track,agenda_floppy_write_track,agenda_serial_sent_byte,
                          agenda_serial_break_boundary,agenda_serial_loopback_byte,agenda_midi_replace,
                          agenda_check_centronics_interrupt,agenda_ikbd_process,agenda_keyboard_reset,
                          agenda_acia_tx_delay_IKBD,agenda_acia_tx_delay_MIDI,ikbd_send_joystick_message,
                          ikbd_report_abs_mouse,agenda_keyboard_replace,
                          
#if defined(SS_FDC_RESTORE_AGENDA)
                          agenda_fdc_restore,
#endif
#if defined(SS_FDC_VERIFY_AGENDA)
                          agenda_fdc_verify,
#endif
                          (LPAGENDAPROC)1};
//--------------------------------------------------------------------------- SHIFTER

//#define SHIFTER_DRAWING_NOT 0
//#define SHIFTER_DRAWING_PICTURE 1
//#define SHIFTER_DRAWING_BORDER -1
//#define SHIFTER_DRAWING_PICTURE_EXCEPT_NOT 2
// >0 must indicate timer B running

//int shifter_drawing;

//int scanline_raster_position();

//int shifter_scanline_width_in_bytes_from_res[3]={160,160,80};
//--------------------------------------------------------------------------- ACIAs
int ACIAClockToHBLS(int,bool=0);
void ACIA_Reset(int,bool);
void ACIA_SetControl(int,BYTE);

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE)
#else
struct ACIA_STRUCT{ // SS removed _
  int clock_divide;

  int rx_delay__unused;
  bool rx_irq_enabled;
  bool rx_not_read;

  int overrun;

  int tx_flag;
  bool tx_irq_enabled;

  BYTE data;
  bool irq;

  int last_tx_write_time;
  int last_rx_read_time;

}acia[2];
#endif

#define ACIA_OVERRUN_NO 0
#define ACIA_OVERRUN_COMING 1
#define ACIA_OVERRUN_YES 416
#define NUM_ACIA_IKBD 0
#define NUM_ACIA_MIDI 1
#define ACIA_CYCLES_NEEDED_TO_START_TX 512

#define ACIA_IKBD acia[0]
#define ACIA_MIDI acia[1]


#ifndef NO_CRAZY_MONITOR

int aes_calls_since_reset=0;
long save_r[16];

MEM_ADDRESS line_a_base=0;
MEM_ADDRESS vdi_intout=0;

#endif

#endif

#undef EXT
#undef INIT

