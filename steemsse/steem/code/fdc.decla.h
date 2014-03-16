#pragma once
#ifndef FDC_DECLA_H
#define FDC_DECLA_H

#define EXT extern
#define INIT(s)

#define FLOPPY_MAX_BYTESPERSECTOR 512
#define FLOPPY_MAX_TRACK_NUM      85
#define FLOPPY_MAX_SECTOR_NUM     26

#if defined(STEVEN_SEAGAL) && defined(SS_VAR_RESIZE)
EXT BYTE floppy_mediach[2]; // potentially breaks snapshot...
EXT BYTE num_connected_floppies INIT(2);
#else
EXT int floppy_mediach[2];
EXT int num_connected_floppies INIT(2);
#endif

EXT int floppy_current_drive();

EXT BYTE floppy_current_side();
EXT void fdc_add_to_crc(WORD &,BYTE);

#if !(defined(STEVEN_SEAGAL) && defined(SS_DMA))
EXT MEM_ADDRESS dma_address;
#endif

EXT bool floppy_instant_sector_access INIT(true);

EXT bool floppy_access_ff INIT(0);
#if !(defined(STEVEN_SEAGAL)&&defined(SS_OSD_DRIVE_LED3))
EXT DWORD disk_light_off_time INIT(0);
#endif
EXT bool floppy_access_started_ff INIT(0);

EXT void agenda_fdc_spun_up(int);
EXT void agenda_fdc_motor_flag_off(int),agenda_fdc_finished(int);
EXT void agenda_floppy_seek(int);
EXT void agenda_floppy_readwrite_sector(int);
EXT void agenda_floppy_read_address(int);
EXT void agenda_floppy_read_track(int);
EXT void agenda_floppy_write_track(int);

#if defined(STEVEN_SEAGAL) && defined(SS_FDC_RESTORE_AGENDA)
EXT void agenda_fdc_restore(int);
#endif
#if defined(STEVEN_SEAGAL) && defined(SS_FDC_VERIFY_AGENDA)
EXT void agenda_fdc_verify(int a);
#endif

#if USE_PASTI
EXT void pasti_handle_return(struct pastiIOINFO*);
EXT void pasti_motor_proc(BOOL);
EXT void pasti_log_proc(const char *);
EXT void pasti_warn_proc(const char *);
EXT HINSTANCE hPasti INIT(NULL);
EXT int pasti_update_time;
EXT const struct pastiFUNCS *pasti INIT(NULL);
//EXT bool pasti_use_all_possible_disks INIT(0);
EXT char pasti_file_exts[160];
EXT WORD pasti_store_byte_access;
EXT bool pasti_active INIT(0);
//EXT DynamicArray<pastiBREAKINFO> pasti_bks;
#endif



//#ifdef IN_EMU

#define FDC_STR_BUSY               BIT_0
#define FDC_STR_T1_INDEX_PULSE     BIT_1
#define FDC_STR_T23_DATA_REQUEST   BIT_1
#define FDC_STR_T1_TRACK_0         BIT_2
#define FDC_STR_T23_LOST_DATA      BIT_2
#define FDC_STR_CRC_ERROR          BIT_3
#define FDC_STR_SEEK_ERROR         BIT_4
#define FDC_STR_T1_SPINUP_COMPLETE BIT_5 //SS not in fdc.cpp
#define FDC_STR_T23_SECTOR_TYPE    BIT_5 // neither
#define FDC_STR_WRITE_PROTECT      BIT_6
#define FDC_STR_MOTOR_ON           BIT_7

#define FDC_CR_TYPE_1_VERIFY      BIT_2
#define FDC_VERIFY                (fdc_cr & FDC_CR_TYPE_1_VERIFY)

#if !(defined(STEVEN_SEAGAL) && defined(SS_DMA))
EXT WORD dma_mode;
EXT BYTE dma_status;
#endif

#if !(defined(STEVEN_SEAGAL) && defined(SS_FDC))
EXT BYTE fdc_cr,fdc_tr,fdc_sr,fdc_str,fdc_dr; // made struct
#endif
EXT bool fdc_last_step_inwards_flag;
EXT BYTE floppy_head_track[2];

void floppy_fdc_command(BYTE);
void fdc_execute();
bool floppy_track_index_pulse_active();

#if defined(STEVEN_SEAGAL) && defined(SS_VAR_RESIZE)
enum {FLOPPY_FF_VBL_COUNT=20};
EXT BYTE floppy_access_ff_counter;
enum {FLOPPY_IRQ_YES=9,FLOPPY_IRQ_ONESEC,FLOPPY_IRQ_NOW};
EXT BYTE floppy_irq_flag;
EXT BYTE fdc_step_time_to_hbls[4];
#if !(defined(STEVEN_SEAGAL) && defined(SS_DMA_FIFO_READ_ADDRESS))
EXT BYTE fdc_read_address_buffer_len;
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SS_DMA))
EXT WORD dma_sector_count; 
#endif
EXT WORD floppy_write_track_bytes_done;
EXT BYTE fdc_spinning_up;
EXT BYTE floppy_type1_command_active;  // Default to type 1 status
#if !(defined(STEVEN_SEAGAL) && defined(SS_DMA))
EXT WORD dma_bytes_written_for_sector_count;
#endif
#else
#define FLOPPY_FF_VBL_COUNT 20
int floppy_access_ff_counter;
#define FLOPPY_IRQ_YES 9
#define FLOPPY_IRQ_ONESEC 10
#define FLOPPY_IRQ_NOW 417
EXT int floppy_irq_flag;
EXT int fdc_step_time_to_hbls[4];
EXT int fdc_read_address_buffer_len;
#if !(defined(STEVEN_SEAGAL) && defined(SS_DMA))
EXT int dma_sector_count;
#endif
EXT int floppy_write_track_bytes_done;
EXT int fdc_spinning_up;
EXT int floppy_type1_command_active;  // Default to type 1 status
#if !(defined(STEVEN_SEAGAL) && defined(SS_DMA))
EXT int dma_bytes_written_for_sector_count;
#endif
#endif//defined(STEVEN_SEAGAL) && defined(SS_VAR_RESIZE)

#if defined(STEVEN_SEAGAL) && defined(SS_DMA_FIFO_READ_ADDRESS)
// just a little hack saving bytes TODO
#if !defined(SS_DMA_FIFO_READ_ADDRESS2)
#define fdc_read_address_buffer Dma.Fifo
#define fdc_read_address_buffer_len Dma.Fifo_idx
#endif
#else
EXT BYTE fdc_read_address_buffer[20];
#endif

//#endif

#define DMA_ADDRESS_IS_VALID_R (dma_address<himem)
#define DMA_ADDRESS_IS_VALID_W (dma_address<himem && dma_address>=MEM_FIRST_WRITEABLE)

#if defined(SS_DRIVE)
#define DRIVE floppy_current_drive() // 0 or 1 guaranteed
#endif

#undef EXT
#undef INIT

#endif//#define FDC_DECLA_H