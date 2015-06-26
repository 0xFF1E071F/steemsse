/*---------------------------------------------------------------------------
FILE: reset.cpp
MODULE: emu
DESCRIPTION: Functions to reset the emulator to a startup state.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: reset.cpp")
#endif

/*
Atari ST Boot Up Operation
The Motorola 68000 on boot up requires initial
values to load into its supervisor stack pointer
and reset vector address. These come in the
form of two long words at address 0x000000 to
0x000007.

Boot up sequence
Figure 9 - Atari ST boot up sequence
(1)
. Load SSP with long word value from 0xFC0000.
. Load PC with long word value from 0xFC0004 (Garbage value, memory not yet
sized).
. CPU Supervisor Mode Interrupts disabled (IPL=7).
. RESET instruction to reset all peripheral chips.
. Check for magic number 0xFA52235F on cartridge port, if present jump to
diagnostic cartridge.
(2).
. Test for warm start, if memvalid (0x000420) and memval2 (0x00043A) contain
the Magic numbers 0x7520191F3 and 0x237698AA respectively, then load the
memconf (0xFF8001) contents with data from memctrl (0x000424).
(3)
. If the resvalid (0x000426) contains the Magic number 0x31415926, jump to reset
vector taken from Resvector (0x00042A).
(4)
. YM2149 sound chip initialized (Floppy deselected).
. The vertical synchronization frequency in syncmode (0xFF820A) is adjusted to
50Hz or 60Hz depending on region.
. Shifter palette initialized.
. Shifter Base register (0xFF8201 and 0xFF8203) are initialized to 0x010000.
. The following steps 5 to 8 are only done on a coldstart to initialize memory.
(5)
. Write 0x000a (2 Mbyte & 2 Mbyte) to the MMU Memory Configuration Register
0xff8001).
(6)
. Write Pattern to 0x000000 - 0x000lff.
. Read Pattern from 0x000200 - 0x0003ff.
. If Match then Bank0 contains 128 Kbyte; goto step 7.
. Read Pattern from 0x000400 - 0x0005ff.
. If Match then Bank0 contains 512 Kbyte; goto step 7.
. Read Pattern from 0x000000 - 0x0001ff.
. If Match then Bank0 contains 2 Mbyte; goto step 7.
. panic: RAM error in Bank0.
(7)
. Write Pattern to 0x200000 - 0x200lff.
. Read Pattern from 0x200200 - 0x2003ff.
. If Match then Bank1 contains 128 Kbyte; goto step 8.
. Read Pattern from 0x200400 - 0x2005ff.
. If Match then Bank1 contains 512 Kbyte; goto step 8.
. Read Pattern from 0x200000 - 0x2001ff.
. If Match then Bank1 contains 2 Mbyte; goto step 8.
. note: Bank1 not fitted.
(8)
. Write Configuration to MMU Memory Configuration Register (0xff8001).
. Note Total Memory Size (Top of RAM) for future reference in phystop
(0x00042E).
. Set magic values in memvalid (0x000420) and memval2 (0x00043A).
(9)
. Clear the first 64 Kbytes of RAM from top of operating system variables
(0x00093A) to Shifter base address (0x010000).
. Initialize operating system variables.
. Change and locate Shifter Base register to 32768 bytes from top of physical ram.
. Initialize interrupt CPU vector table.
. Initialize BIOS.
. Initialize MFP.
(10)
. Cartridge port checked, if software with bit 2 set in CA_INIT then start.
(11)
. Identify type of monitor attached for mode of operation for the Shifter video chip
and initialize.
(12)
. Cartridge port checked, if software with CA_INIT clear (execute prior to display
memory and interrupt vector initialization) then start.
(13)
. CPU Interrupt level (IPL) lowered to 3 (HBlank interrupts remain masked).
(14)
. Cartridge port checked, if software with bit 1 set in CA_INIT (Execute prior to
GEMDOS initialization) then start.
(15)
. The GEMDOS Initialization routines are completed.
(16)
. Attempt boot from floppy disk if operating system variable _bootdev (0x000446)
smaller than 2 (for floppy disks) is. Before a boot attempt is made bit 3 in
CA_INIT (Execute prior to boot disk) checked, if set, start cartridge.
. The ACSI Bus is examined for devices, if successful search and load boot sector.
. If system variable _cmdload (0x000482) is 0x0000, skip step 17.
(17)
. Turn screen cursor on
. Start any program in AUTO folder of boot device
. Start COMMAND.PRG for a shell
(18)
. Start any program in AUTO folder of boot device
. AES (in the ROM) starts.

It is important to have these two different reset signals, as some parts of the design only
need to be reset on power up to known states. One of these components was the clock
signal component. It was important for the CPU that the clock was running while a reset
is issued, and that the reset was active for at least 132 clock cycles [27].
*/

#if defined(SSE_ACSI)
#include "harddiskman.decla.h"
#include "SSE/SSEAcsi.h"
#endif

//---------------------------------------------------------------------------
void power_on()
{

#if defined(STEVEN_SEAGAL) && defined(SSE_GUI_STATUS_STRING)
  GUIRefreshStatusBar();//overkill
#endif

  ZeroMemory(Mem+MEM_EXTRA_BYTES,mem_len);
  on_rte=ON_RTE_RTE;
  SET_PC(rom_addr);
  for (int n=0;n<16;n++) r[n]=0;
  other_sp=0;
  ioaccess=0;

  xbios2=mem_len-0x8000;
  if (extended_monitor) xbios2=max(int(mem_len-(em_width*em_height*em_planes)/8),0);

  ZeroMemory(STpal,sizeof(STpal));
  if (tos_version>0x104){
    STpal[0]=0xfff;
  }else if (tos_version>0){
    STpal[0]=0x777;
  }
  for (int n=0;n<16;n++) PAL_DPEEK(n*2)=STpal[n];
  palette_convert_all();

  DEBUG_ONLY( if (debug_wipe_log_on_reset) logfile_wipe(); )

  log_write("************************* Power On ************************");

  osd_init_run(0);

  mixed_output=0;

  floppy_mediach[0]=0;
  floppy_mediach[1]=0;

  os_gemdos_vector=0;
  os_bios_vector=0;
  os_xbios_vector=0;
  
#ifndef DISABLE_STEMDOS
  stemdos_reset();
#endif

  interrupt_depth=0;
  hbl_count=0;
  agenda_length=0;

  LPEEK(0)=ROM_LPEEK(0);
  LPEEK(4)=ROM_LPEEK(4);

  sr=0x2700;

  emudetect_reset();
  snapshot_loaded=0;

  fdc_str=BIT_2;
  for (int floppyno=0;floppyno<2;floppyno++){
    floppy_head_track[floppyno]=0;
#if defined(STEVEN_SEAGAL) && defined(SSE_DRIVE)
    SF314[floppyno].Id=floppyno;
#if defined(SSE_DRIVE_INIT)
    SF314[floppyno].Init();
#else
#if defined(SSE_DRIVE_MOTOR_ON)
#if defined(SSE_DRIVE_STATE)
    SF314[floppyno].State.motor=false;
#else
    SF314[floppyno].motor_on=false;
#endif
#ifdef SSE_DISK_STW
    SF314[floppyno].rpm=300; // temp
    WD1772.Lines.motor=false;
#endif
#endif
#endif
#endif

    
/*  it's too late here MFD
#if defined(SSE_DISK_STW2)
    ImageSTW[floppyno].Id=floppyno;
#endif
#if defined(SSE_DISK_HFE)
    ImageHFE[floppyno].Id=floppyno;
#endif
#if defined(SSE_DISK_SCP2A)
    ImageSCP[floppyno].Id=floppyno;
#endif
    */
#if defined(SSE_DISK1) && !defined(SSE_DISK2)
    Disk[floppyno].Id=floppyno;//same idea
#endif
  }
  fdc_tr=0;fdc_sr=0;fdc_dr=0;

  psg_reg_select=0;
  psg_reg_data=0;

  floppy_irq_flag=0;
  fdc_spinning_up=0;
  floppy_type1_command_active=2;

#if defined(STEVEN_SEAGAL) 

#if defined(SSE_DMA_WRITE_CONTROL)
  dma_mode=0; // see reset_peripherals()
#endif

#if defined(SSE_STF)
  SwitchSTType(ST_TYPE);
#endif

#if defined(SSE_INT_MFP_TxDR_RESET)
  ZeroMemory(&mfp_reg[MFPR_TADR],4);
#endif

#endif//ss
#if !defined(SSE_ACSI_DISABLE_HDIMG)
  hdimg_reset();
#endif
  reset_peripherals(true);

  init_screen();
  init_timings();

  hbl_pending=false;

  disable_input_vbl_count=50*3; // 3 seconds

#if defined(SSE_VID_3BUFFER_WIN)
  Disp.VSyncTiming=0;
#endif

#if defined(SSE_CPU_HALT)
  M68000.ProcessingState=TM68000::NORMAL; //care later for exception
#endif


}
//---------------------------------------------------------------------------
#define LOGSECTION LOGSECTION_ALWAYS
void reset_peripherals(bool Cold)
{
  log("***** reset peripherals ****");
  if(Cold)
    TRACE_LOG("Reset peripherals (cold)\n");
  else
    TRACE_LOG("Reset peripherals (warm)\n");

#if defined(SSE_OSD_CONTROL)
  if( !Cold && (OSD_MASK_CPU&OSD_CONTROL_CPURESET)) 
    TRACE_OSD("RESET");
#endif

#ifndef NO_CRAZY_MONITOR
  if (extended_monitor){
    if (em_planes==1){
      screen_res=2;
    }else{
      screen_res=0;
    }
    shifter_freq=50;
    shifter_freq_idx=0;
  }else
#endif

  if (COLOUR_MONITOR){
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY4)
    BYTE old_screen_res=screen_res;
#endif
    screen_res=0;
    shifter_freq=60; // SS notice reset freq=60hz
    shifter_freq_idx=1;
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY4)
    if(old_screen_res==2)
      init_screen(); // or some bad crashes
#endif
  }else{
    screen_res=2;
    shifter_freq=MONO_HZ;
    shifter_freq_idx=2;
  }
  
#if defined(STEVEN_SEAGAL)

#if defined(SSE_CPU)
  M68000.Reset(Cold);
#endif

#if defined(SSE_SHIFTER)
  Shifter.Reset(Cold);
#endif

#if defined(SSE_WD1772_RESET)
  WD1772.Reset(Cold);
#endif

#if defined(SSE_INT_JITTER_RESET) 
  if(Cold)
  {
    HblJitterIndex=0;
    VblJitterIndex=0;
  }
#endif

#endif
  
  shifter_hscroll=0;
  shifter_hscroll_extra_fetch=0;
  shifter_fetch_extra_words=0; //unspecified
  shifter_first_draw_line=0;
  shifter_last_draw_line=shifter_y;
  CALC_CYCLES_FROM_HBL_TO_TIMER_B(shifter_freq);
  vbl_pending=false;

  dma_status=1;  //no error, apparently
#if defined(STEVEN_SEAGAL) && defined(SSE_DMA_WRITE_CONTROL)
/*
          The actual DMA operation is performed through a 32 byte
          FIFO  programmed  via  the  DMA  Mode Control Register (word
          access write only, reset: not affected) and DMA Sector Count
          Register  (word  access  write only, reset: all zeros).
*/
  dma_sector_count=0;//SS test
#else
  dma_mode=0;
  dma_sector_count=0xffff;
#endif
#if !(defined(STEVEN_SEAGAL) && defined(SSE_DMA_FIFO_READ_ADDRESS2))
  fdc_read_address_buffer_len=0;
#endif
  dma_bytes_written_for_sector_count=0;

#if USE_PASTI
  if (hPasti){
//    log_to(LOGSECTION_PASTI,"PASTI: Reset, calling HwReset()");
    pasti->HwReset(Cold);
  }
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_IPF)
  if(CAPSIMG_OK)
    Caps.Reset();
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_ACSI) 
  if(ACSI_EMU_ON)
#if defined(SSE_ACSI_MULTIPLE)
    for(int i=0;i<TAcsiHdc::MAX_ACSI_DEVICES;i++)
      AcsiHdc[i].Reset(Cold);
#else
    AcsiHdc.Reset(Cold);
#endif
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_TxDR_RESET)
  DWORD tmp;
  memcpy(&tmp,&mfp_reg[MFPR_TADR],4);
#endif
  ZeroMemory(mfp_reg,sizeof(mfp_reg));
#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_TxDR_RESET)
  if(!Cold)
    memcpy(&mfp_reg[MFPR_TADR],&tmp,4);
#endif
#if defined(SSE_INT_MFP_IACK_LATENCY4)
  for(int i=0;i<4;i++)
    MC68901.SkipTimer[i]=0;
#endif
  mfp_reg[MFPR_GPIP]=mfp_gpip_no_interrupt;
  mfp_reg[MFPR_AER]=0x4;   // CTS goes the other way
  mfp_reg[MFPR_TSR]=BIT_7 | BIT_4;  //buffer empty | END
  for (int timer=0;timer<4;timer++) mfp_timer_counter[timer]=256*64;
  MFP_CALC_INTERRUPTS_ENABLED;
  MFP_CALC_TIMERS_ENABLED;

  dma_sound_control=0;
  dma_sound_start=0,next_dma_sound_start=0;
  dma_sound_end=0,next_dma_sound_end=0;
  dma_sound_fetch_address=dma_sound_start;
  dma_sound_mode=BIT_7;
  dma_sound_freq=dma_sound_mode_to_freq[0];
  dma_sound_output_countdown=0;
  dma_sound_samples_countdown=0;
  dma_sound_last_word=MAKEWORD(128,128);

  MicroWire_Mask=0x07ff,MicroWire_Data=0;
  dma_sound_volume=40;
  dma_sound_l_volume=20;
  dma_sound_r_volume=20;
  dma_sound_l_top_val=128;
  dma_sound_r_top_val=128;
  dma_sound_mixer=1;

#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_MICROWIRE)
  dma_sound_bass=6; // 6 is neutral value
  dma_sound_treble=6;
#endif
#if defined(SSE_ACIA_NO_RESET_PIN) 
  if(Cold) // don't reset ACIA on warm reset, fixes Dragonnels/Plazma
#endif
  ACIA_Reset(NUM_ACIA_IKBD,true);
  ikbd_reset(true); // Always cold reset, soft reset is different

#if defined(SSE_IKBD_6301)
#if SSE_VERSION>=351
  HD6301.ResetChip(Cold);
#endif
  if(Cold) // only cold
    keyboard_buffer_length=0; // cold reset & run, Transbeauce 2
#endif

#if defined(SSE_ACIA_NO_RESET_PIN) 
  if(Cold) 
#endif
  ACIA_Reset(NUM_ACIA_MIDI,true);
  MIDIPort.Reset();
  ParallelPort.Reset();
  SerialPort.Reset();

  for (int n=0;n<16;n++) psg_reg[n]=0;
  psg_reg[PSGR_PORT_A]=0xff; // Set DTR RTS, no drive, side 0, strobe and GPO

  RS232_CalculateBaud(bool(mfp_reg[MFPR_UCR] & BIT_7),mfp_get_timer_control_register(3),true);
  RS232_VBL();

  ZeroMemory(&Blit,sizeof(Blit));

  cpu_stopped=false;
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU) && defined(SSE_DEBUG)
  M68000.NextIrFetched=false;
#endif

  if (runstate==RUNSTATE_RUNNING) 
    prepare_event_again();

#if defined(SSE_DRIVE_SOUND)
  if(Cold)
    SF314[0].Sound_StopBuffers();
#endif
}
#undef LOGSECTION
//---------------------------------------------------------------------------
void reset_st(DWORD flags)
{
#if defined(STEVEN_SEAGAL) && defined(SSE_CPU_PREFETCH)
  prefetched_2=FALSE;
#endif
  bool Stop=bool(flags & RESET_NOSTOP)==0;
  bool Warm=bool(flags & RESET_WARM);
  bool ChangeSettings=bool(flags & RESET_NOCHANGESETTINGS)==0;
  bool Backup=bool(flags & RESET_NOBACKUP)==0;
  
  if (runstate==RUNSTATE_RUNNING && Stop) runstate=RUNSTATE_STOPPING;
  if (Backup) GUISaveResetBackup();

  if (Warm){
    reset_peripherals(0);
#ifndef DISABLE_STEMDOS
    stemdos_set_drive_reset();
#endif
    SET_PC(LPEEK(4));

    if (runstate==RUNSTATE_STOPPED){
      // Hack alert! Show user reset has happened
      MEM_ADDRESS old_x2=xbios2;
      xbios2=0;
      draw_end();
      draw(0);
      xbios2=old_x2;
    }
  }else{
    if (ChangeSettings) GUIColdResetChangeSettings();
//    TRACE("reset_st->power on\n");
    power_on();
    palette_convert_all();

    if (ResChangeResize) StemWinResize();

    draw_end();
    draw(true);
  }
  DEBUG_ONLY( debug_reset(); )
  shifter_freq_at_start_of_vbl=shifter_freq;
  PasteIntoSTAction(STPASTE_STOP);
  CheckResetIcon();
  CheckResetDisplay();
  
#ifndef NO_CRAZY_MONITOR
  /////
  line_a_base=0;
  vdi_intout=0;
  aes_calls_since_reset=0;
  if (extended_monitor) extended_monitor=1; //first stage of extmon init
#endif

#if defined(SSE_BOILER_PSEUDO_STACK)
  for(int i=0;i<PSEUDO_STACK_ELEMENTS;Debug.PseudoStack[i++]=0);
#endif

}



