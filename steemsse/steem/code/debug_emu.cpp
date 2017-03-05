/*---------------------------------------------------------------------------
FILE: debug_emu.cpp
MODULE: emu
DESCRIPTION: General low-level debugging functions.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: debug_emu.cpp")
#endif

#if defined(SSE_BUILD)
#define EXT
#define INIT(s) =s

EXT bool debug_in_trace INIT(0),debug_wipe_log_on_reset;
EXT bool redraw_on_stop INIT(0),redraw_after_trace INIT(0);
EXT COLORREF debug_gun_pos_col INIT( RGB(255,0,0) );
EXT int crash_notification INIT(CRASH_NOTIFICATION_BOMBS_DISPLAYED);
EXT bool stop_on_blitter_flag INIT(false);
EXT bool stop_on_ipl_7 INIT(false);
EXT int stop_on_user_change INIT(0);
EXT int stop_on_next_program_run INIT(0);
EXT bool debug_first_instruction INIT(0);
EXT Str runstate_why_stop;
EXT DWORD debug_cycles_since_VBL,debug_cycles_since_HBL;
#if defined(SSE_BOILER_SHOW_ACT)
EXT DWORD debug_ACT;
#endif
#if defined(SSE_BOILER_MOVE_OTHER_SP2)
EXT DWORD debug_USP,debug_SSP;
#endif
EXT MEM_ADDRESS debug_VAP;
EXT int debug_time_to_timer_timeout[4];
#if defined(SSE_BOILER_DECRYPT_TIMERS)
EXT int debug_time_to_timer_prescale[4];
EXT int debug_time_to_timer_data[4];
EXT int debug_time_to_timer_count[4];
EXT int debug_time_to_timer_ticks[4];
#endif
#if defined(SSE_BOILER_FRAME_INTERRUPTS)
EXT int debug_frame_interrupts;
#endif
EXT int debug_cycle_colours INIT(0);
EXT int debug_screen_shift INIT(0);
EXT int debug_num_bk INIT(0),debug_num_mon_reads INIT(0),debug_num_mon_writes INIT(0);
EXT int debug_num_mon_reads_io INIT(0),debug_num_mon_writes_io INIT(0);

EXT MEM_ADDRESS debug_bk_ad[MAX_BREAKPOINTS],debug_mon_read_ad[MAX_BREAKPOINTS],debug_mon_write_ad[MAX_BREAKPOINTS];
EXT MEM_ADDRESS debug_mon_read_ad_io[MAX_BREAKPOINTS],debug_mon_write_ad_io[MAX_BREAKPOINTS];
EXT WORD debug_mon_read_mask[MAX_BREAKPOINTS],debug_mon_write_mask[MAX_BREAKPOINTS];
EXT WORD debug_mon_read_mask_io[MAX_BREAKPOINTS],debug_mon_write_mask_io[MAX_BREAKPOINTS];
//---------------------------------------------------------------------------
EXT MEM_ADDRESS trace_over_breakpoint INIT(0xffffffff);
EXT int debug_run_until INIT(DRU_OFF),debug_run_until_val;
#if defined(SSE_BOILER_MONITOR_372) //v3.7.2 to clarify code
EXT int monitor_mode INIT(MONITOR_MODE_STOP),breakpoint_mode INIT(2);
#else
EXT int monitor_mode INIT(2),breakpoint_mode INIT(2);
#endif
bool break_on_irq[NUM_BREAK_IRQS]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
EXT MEM_ADDRESS pc_history[HISTORY_SIZE];

#if defined(SSE_BOILER_HISTORY_TIMING)
EXT short pc_history_y[HISTORY_SIZE];
EXT short pc_history_c[HISTORY_SIZE];
#endif

EXT int pc_history_idx INIT(0);
EXT BYTE debug_send_alt_keys INIT(0),debug_send_alt_keys_vbl_countdown INIT(0);
void *debug_plugin_routines[]={(void*)2,(void*)debug_plugin_read_mem,(void*)debug_plugin_write_mem};
DynamicArray<DEBUGPLUGININFO> debug_plugins;


#undef EXT
#undef INIT

#include "SSE/SSEFloppy.h"

#endif//decla

#ifdef DEBUG_BUILD//SS

//---------------------------------------------------------------------------
// This is for if the emu is half way though the screen, it should be called
// immediately after draw_begin to fix draw_dest_ad
void debug_update_drawing_position(int *pHorz)
{
  int y=-scanlines_above_screen[0];
  if (shifter_freq_at_start_of_vbl==60) y=-scanlines_above_screen[1];
  for (;y<scan_y;y++){
    if (y>=draw_first_scanline_for_border || y>=shifter_first_draw_line){
      if (y<shifter_last_draw_line || y<draw_last_scanline_for_border){
        if (y>=draw_first_possible_line && y<draw_last_possible_line){
          draw_dest_ad=draw_dest_next_scanline;
          draw_dest_next_scanline+=draw_dest_increase_y;
        }
      }
    }
  }
  if (screen_res_at_start_of_vbl<2){
    if ((scan_y>=draw_first_scanline_for_border || scan_y>=shifter_first_draw_line) &&
          (scan_y<shifter_last_draw_line || scan_y<draw_last_scanline_for_border)){
      int horz_scale=1;
      if (screen_res_at_start_of_vbl==1 || mixed_output || draw_med_low_double_height) horz_scale=2;

      int x=scanline_drawn_so_far;
      if ((border & 1)==0) 
        x=max(x-BORDER_SIDE,0);
      x*=horz_scale;

      draw_dest_ad+=x*BytesPerPixel;
      if (pHorz) *pHorz=horz_scale;
    }
  }
}
//---------------------------------------------------------------------------
void update_display_after_trace()
{
  if (extended_monitor) return;

  // Draw can call this routine, and it calls draw, arrrggghh!
  static bool in=0;
  if (in) return;
  in=true;

  osd_no_draw=true;
  if (screen_res_at_start_of_vbl<2 && redraw_after_trace==0){
    RECT old_src=draw_blit_source_rect;
    draw_begin();
    if (draw_blit_source_rect.left!=old_src.left || draw_blit_source_rect.top!=old_src.top ||
          draw_blit_source_rect.right!=old_src.right || draw_blit_source_rect.bottom!=old_src.bottom){
      draw_end();
      draw(false);
      draw_begin();
    }
    draw_buffer_complex_scanlines=0;

    int horz_scale=0;
    debug_update_drawing_position(&horz_scale);

#if defined(SSE_SHIFTER)
    Shifter.Render(LINECYCLES);
#else
    draw_scanline_to(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
#endif
    if ((scanline_drawn_so_far < BORDER_SIDE+320+BORDER_SIDE) && horz_scale){

      int line_add=0;
      if (draw_med_low_double_height) line_add=draw_line_length;
      for (int i=0;i<horz_scale;i++){
        DWORD col=colour_convert(GetRValue(debug_gun_pos_col),GetGValue(debug_gun_pos_col),GetBValue(debug_gun_pos_col));
        switch (BytesPerPixel){
          case 1:
            *draw_dest_ad=BYTE(col);
            *(draw_dest_ad+line_add)=BYTE(col);
            break;
          case 2:
            *LPWORD(draw_dest_ad)=WORD(col);
            *LPWORD(draw_dest_ad+line_add)=WORD(col);
            break;
          case 3:
          case 4:
            *LPDWORD(draw_dest_ad)=DWORD(col);
            *LPDWORD(draw_dest_ad+line_add)=DWORD(col);
            break;
        }
        draw_dest_ad+=BytesPerPixel;
      }
    }
    draw_end();
    draw_blit();

  }
#if !defined(SSE_BOILER_HIRES_DONT_REDRAW) // not for overscan in hires
  else
    draw(false);
#endif
  osd_no_draw=0;
  in=0;
}
//---------------------------------------------------------------------------
void breakpoint_log()
{
  if (logging_suspended) return;

  Str logline=Str("\r\n!!!! PASSED BREAKPOINT at address $")+HEXSl(pc,6)+", sr="+HEXSl(sr,4);
  logline+="\r\n";
  for (int n=0;n<8;n++) logline+=Str("d")+n+"="+HEXSl(r[n],6)+"  ";
  logline+="\r\n";
  for (int n=0;n<8;n++) logline+=Str("a")+n+"="+HEXSl(areg[n],6)+"  ";
  logline+="\r\n";
  logline+=Str("time=")+ABSOLUTE_CPU_TIME+" scanline="+scan_y+" cycle="+(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
  logline+="\r\n";
  log_write(logline);
}

//---------------------------------------------------------------------------
void breakpoint_check()
{
  if (runstate!=RUNSTATE_RUNNING) return;
  for (int n=0;n<debug_num_bk;n++){
    if (debug_bk_ad[n]==pc){
      if (debug_get_ad_mode(pc)==3){
        breakpoint_log();
      }else{
        runstate=RUNSTATE_STOPPING;
        SET_WHY_STOP(Str("Hit breakpoint at address $")+HEXSl(pc,6));
      }
      return;
    }
  }
}
//---------------------------------------------------------------------------
void debug_update_cycle_counts()
{
  debug_cycles_since_VBL=ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl;
  debug_cycles_since_HBL=ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl;
#if defined(SSE_VIDEO_CHIPSET)
  debug_VAP=MMU.ReadVideoCounter(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
#else
  debug_VAP=get_shifter_draw_pointer(ABSOLUTE_CPU_TIME-cpu_timer_at_start_of_hbl);
#endif
#if defined(SSE_BOILER_SHOW_ACT)
  debug_ACT=ACT;
#endif

#if defined(SSE_BOILER_BROWSER_6301) && defined(SSE_ACIA)
  hd6301_copy_ram(Debug.HD6301RamBuffer); // in 6301.c
#endif

  for (int t=0;t<4;t++){
    if (mfp_timer_enabled[t]){
      debug_time_to_timer_timeout[t]=mfp_timer_timeout[t]-ABSOLUTE_CPU_TIME;

#if defined(SSE_BOILER_DECRYPT_TIMERS)
        ASSERT(mfp_get_timer_control_register(t)>=0);
        ASSERT(mfp_get_timer_control_register(t)<16);
        debug_time_to_timer_prescale[t]=mfp_timer_8mhz_prescale[mfp_get_timer_control_register(t)];
        debug_time_to_timer_data[t]=mfp_reg[MFPR_TADR+t]; //could directly point to it?
        debug_time_to_timer_ticks[t]=mfp_calc_timer_counter(t);
        debug_time_to_timer_count[t]=mfp_timer_counter[t]/64;
#endif

    }else{
#if defined(SSE_BOILER_TIMER_B)
      if(t==1) 
        debug_time_to_timer_timeout[t]=mfp_timer_counter[t]/64;
      else
#endif
        debug_time_to_timer_timeout[t]=0;

#if defined(SSE_BOILER_DECRYPT_TIMERS)
        debug_time_to_timer_prescale[t]=0;
        debug_time_to_timer_data[t]=0;
        debug_time_to_timer_count[t]=0;
        debug_time_to_timer_ticks[t]=0;
#endif
#if defined(SSE_BOILER_FRAME_INTERRUPTS2)
        debug_frame_interrupts=(Debug.FrameInterrupts<<16)+Debug.FrameMfpIrqs;
#endif

    }
  }

#if defined(SSE_BOILER_MOVE_OTHER_SP2)
  debug_USP=USP;
  debug_SSP=SSP;
#endif

}
//---------------------------------------------------------------------------
extern BYTE d2_peek(MEM_ADDRESS);
extern WORD d2_dpeek(MEM_ADDRESS);

void debug_hit_mon(MEM_ADDRESS ad,int read)
{
  if (mode!=STEM_MODE_CPU) return;
  int bytes=2;
#if defined(SSE_BOILER_MONITOR_VALUE)
/*  When the option is checked, we will stop Steem only if the condition
    is met when R/W on the address.
    Problem: Steem didn't foresee it and more changes are needed for
    write (what value?)
*/
  int val=0;
#if defined(SSE_BOILER_390)
  if(ad&1) 
    ad--;  
#endif
  if(Debug.MonitorValueSpecified && Debug.MonitorComparison)
  {
#if !defined(SSE_BOILER_390)
    if(ad&1) 
      ad--;  // We only check words
#endif
    val=(int)d2_dpeek(ad); // must read RAM or IO
    if(
      (Debug.MonitorComparison=='=' && val!=Debug.MonitorValue)
      || (Debug.MonitorComparison=='!' && val==Debug.MonitorValue)
      || (Debug.MonitorComparison=='<' && val>=Debug.MonitorValue)
      || (Debug.MonitorComparison=='>' && val<=Debug.MonitorValue))
    {
      return;
    }
    else 
      TRACE("addr %X value %X %c %X\n",ad,val,Debug.MonitorComparison,Debug.MonitorValue);
  }
  else
  {
    WORD mask=debug_get_ad_mask(ad,read);
    if (mask==0xff00) bytes=1;
    if (mask==0x00ff) bytes=1, ad++;
    ASSERT(!(ad&1));
    val=int((bytes==1) ? int(d2_peek(ad)):int(d2_dpeek(ad)));
  }
#else 
    WORD mask=debug_get_ad_mask(ad,read);
    if (mask==0xff00) bytes=1;
    if (mask==0x00ff) bytes=1, ad++;
    int val=int((bytes==1) ? int(d2_peek(ad)):int(d2_dpeek(ad)));
#endif//390

  Str mess;
  if (read){
    mess=HEXSl(old_pc,6)+": Read "+val+" ($"+HEXSl(val,bytes*2)+") from address $"+HEXSl(ad,6);
  }else{
    mess=HEXSl(old_pc,6)+": Write to address $"+HEXSl(ad,6);
  }
  int mode=debug_get_ad_mode(ad & ~1);
#if defined(SSE_BOILER_MONITOR_RANGE) // mode is likely 0 (ad not found)
  if(Debug.MonitorRange)
#if defined(SSE_BOILER_MONITOR_372)
    mode=monitor_mode;
#else
    mode=2;
#endif
#endif
#if defined(SSE_BOILER_MONITOR_372) //v3.7.2 to clarify code
  if (mode==MONITOR_MODE_STOP){
#else
  if (mode==2){
#endif
    if (runstate==RUNSTATE_RUNNING){
      runstate=RUNSTATE_STOPPING;
      SET_WHY_STOP(mess);
    }else if (runstate==RUNSTATE_STOPPED){
      Alert(mess,"Monitor Activated",0);
    }
  }else{
    //ASSERT(!read); //Boiler bug!
    debug_mem_write_log_address=ad;
    debug_mem_write_log_bytes=bytes;
#if defined(SSE_BOILER_MONITOR_TRACE)//3.7.2
    if(read)
      ioaccess|=IOACCESS_DEBUG_MEM_READ_LOG;
    else
#endif
    ioaccess|=IOACCESS_DEBUG_MEM_WRITE_LOG;
  }
}
//---------------------------------------------------------------------------
void debug_hit_io_mon_write(MEM_ADDRESS ad,int val)
{
  if (mode!=STEM_MODE_CPU) return;

#if defined(SSE_BOILER_390)
  WORD mask=debug_get_ad_mask(ad,FALSE); // "return pda->mask[int(read ? 1:0)];"
#elif defined(SSE_BOILER) // 330 
  WORD mask=debug_get_ad_mask(ad,TRUE);
#else
  WORD mask=debug_get_ad_mask(ad,read); //SS read is a function
#endif

#if defined(SSE_BOILER_MONITOR_VALUE)
/*  When the option is checked, we will stop Steem only if the condition
    is met when R/W on the address.
*/
  if(Debug.MonitorValueSpecified && Debug.MonitorComparison)
  {
    if(
      (Debug.MonitorComparison=='=' && val!=Debug.MonitorValue)
      || (Debug.MonitorComparison=='!' && val==Debug.MonitorValue)
      || (Debug.MonitorComparison=='<' && val>=Debug.MonitorValue)
      || (Debug.MonitorComparison=='>' && val<=Debug.MonitorValue))
    {
      return;
    }
    else 
      TRACE("addr %X value %X %c %X\n",ad,val,Debug.MonitorComparison,Debug.MonitorValue);
  }
#endif

  int bytes=2;
  if (mask==0xff00) bytes=1;
  if (mask==0x00ff) bytes=1, ad++;
  Str mess=HEXSl(old_pc,6)+": Wrote to address $"+HEXSl(ad,6)+", new value is "+val+" ($"+HEXSl(val,bytes*2)+")";
  int mode=debug_get_ad_mode(ad & ~1);
#if defined(SSE_BOILER_MONITOR_RANGE)
  if(Debug.MonitorRange)
    mode=2;
#endif
  if (mode==2){
    if (runstate==RUNSTATE_RUNNING){
      runstate=RUNSTATE_STOPPING;
      SET_WHY_STOP(mess);
    }else if (runstate==RUNSTATE_STOPPED){
      Alert(mess,"Monitor Activated",0);
    }
  }else{
    log_write(mess);
  }
}
//---------------------------------------------------------------------------
void debug_check_for_events()
{
  while (cpu_cycles<=0){
    screen_event_vector();
    prepare_next_event();
    if (cpu_cycles>0) check_for_interrupts_pending();
  }
}
//---------------------------------------------------------------------------
#if !defined(SSE_GLUE)
void debug_trace_event_plan_init()
{
  if (screen_event_pointer==NULL) screen_event_pointer=event_plan[shifter_freq_idx];
}
#endif
//---------------------------------------------------------------------------
void iolist_debug_add_pseudo_addresses()
{
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x000,"PSG0 Ch.A Freq L",1,NULL,psg_reg);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x002,"PSG1 Ch.A Freq H",1,NULL,psg_reg+1);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x004,"PSG2 Ch.B Freq L",1,NULL,psg_reg+2);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x006,"PSG3 Ch.B Freq H",1,NULL,psg_reg+3);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x008,"PSG4 Ch.C Freq L",1,NULL,psg_reg+4);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x00a,"PSG5 Ch.C Freq H",1,NULL,psg_reg+5);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x00c,"PSG6 Noise Freq",1,NULL,psg_reg+6);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x00e,"PSG7 Mixer",1,"PortB Out|PortA Out|Ch.C Noise off|B Noise Off|A Noise Off|C Tone Off|B Tone Off|A Tone Off",psg_reg+7);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x010,"PSG8 Ch.A Ampl",1,".|.|.|env|A3|A2|A1|A0",psg_reg+8);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x012,"PSG9 Ch.B Ampl",1,".|.|.|env|A3|A2|A1|A0",psg_reg+9);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x014,"PSG10 Ch.C Ampl",1,".|.|.|env|A3|A2|A1|A0",psg_reg+10);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x016,"PSG11 Env Period H",1,NULL,psg_reg+11);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x018,"PSG12 Env Period L",1,NULL,psg_reg+12);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x01a,"PSG13 Env shape",1,".|.|.|.|Continue|Attack|Alternate|Hold",psg_reg+13);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x01c,"PSG14 Port A",1,"IDE Drv on|SCC A|Mon Jack GPO|Int. Spkr|Cent strobe|RS232 DTR|RS232 RTS|Drv 1|Drv 0|Drv side",psg_reg+14);
  iolist_add_entry(IOLIST_PSEUDO_AD_PSG+0x01e,"PSG15 Port B (Parallel port)",1,NULL,psg_reg+15);

  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x000,"FDC Command Register",1,NULL,&fdc_cr);
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x002,"FDC Status Register",1,"Motor|Write Protect|Spin/Rec|Seek Fail|CRC Err|Track 0|Index|Busy",&fdc_str);
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x004,"FDC Sector Register",1,NULL,&fdc_sr);
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x006,"FDC Track Register",1,NULL,&fdc_tr);
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x008,"FDC Data Register",1,NULL,&fdc_dr);
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x00a,"FDC Drive/Side (PSG 14)",1,"B|A|Side 0",&(psg_reg[14]));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x00c,"FDC Current Track Drive A",1,NULL,&(floppy_head_track[0]));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x00e,"FDC Current Track Drive B",1,NULL,&(floppy_head_track[1]));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x010,"FDC Spinning Up",1,NULL,lpDWORD_B_0(&fdc_spinning_up));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x012,"FDC Type 1 Command Active",1,NULL,lpDWORD_B_0(&floppy_type1_command_active));
#if !defined(SSE_WD1772_REGS_FOR_FDC)
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x014,"FDC Stepping In Flag",1,NULL,lpDWORD_B_0(&fdc_last_step_inwards_flag));
#endif
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x018,"DMA Address High",1,NULL,lpDWORD_B_2(&dma_address));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x01a,"DMA Address Mid",1,NULL,lpDWORD_B_1(&dma_address));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x01c,"DMA Address Low",1,NULL,lpDWORD_B_0(&dma_address));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x01e,"DMA Mode",1,"FDC Transfer|Disable DMA|.|Sec Count Select|HDC|A1|A0|.",lpWORD_B_0(&dma_mode));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x020,"DMA Write From RAM (Bit 8 of Mode)",1,NULL,lpWORD_B_1(&dma_mode));
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x022,"DMA Status",1,"DRQ|Sec Count Not 0|No Error",&dma_status);
  iolist_add_entry(IOLIST_PSEUDO_AD_FDC+0x024,"DMA Sector Count",1,NULL,lpDWORD_B_0(&dma_sector_count));

  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x000,"IKBD Mouse Mode",1,NULL,lpDWORD_B_0(&ikbd.mouse_mode));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x002,"IKBD Joy Mode",1,NULL,lpDWORD_B_0(&ikbd.joy_mode));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x004,"IKBD Send Nothing Flag",1,NULL,(BYTE*)&ikbd.send_nothing);
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x006,"IKBD Resetting Flag",1,NULL,(BYTE*)&ikbd.resetting);
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x008,"IKBD Mouse Button Action",1,"Keys|Release ABS|Press ABS",&ikbd.mouse_button_press_what_message);
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x00a,"IKBD Port 0 Joystick Flag",1,NULL,(BYTE*)&ikbd.port_0_joy);
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x00c,"IKBD Mouse Y Reverse Flag",1,NULL,(BYTE*)&ikbd.mouse_upside_down);
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x00e,"IKBD Relative Mouse Threshold X",1,NULL,lpDWORD_B_0(&ikbd.relative_mouse_threshold_x));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x010,"IKBD Relative Mouse Threshold Y",1,NULL,lpDWORD_B_0(&ikbd.relative_mouse_threshold_y));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x012,"IKBD Abs Mouse Pos X High",1,NULL,lpDWORD_B_1(&ikbd.abs_mouse_x));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x014,"IKBD Abs Mouse Pos X Low",1,NULL,lpDWORD_B_0(&ikbd.abs_mouse_x));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x016,"IKBD Abs Mouse Pos Y High",1,NULL,lpDWORD_B_1(&ikbd.abs_mouse_y));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x018,"IKBD Abs Mouse Pos Y Low",1,NULL,lpDWORD_B_0(&ikbd.abs_mouse_y));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x01a,"IKBD Abs Mouse Max X High",1,NULL,lpDWORD_B_1(&ikbd.abs_mouse_max_x));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x01c,"IKBD Abs Mouse Max X Low",1,NULL,lpDWORD_B_0(&ikbd.abs_mouse_max_x));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x01e,"IKBD Abs Mouse Max Y High",1,NULL,lpDWORD_B_1(&ikbd.abs_mouse_max_y));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x020,"IKBD Abs Mouse Max Y Low",1,NULL,lpDWORD_B_0(&ikbd.abs_mouse_max_y));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x022,"IKBD Abs Mouse Scale X",1,NULL,lpDWORD_B_0(&ikbd.abs_mouse_scale_x));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x024,"IKBD Abs Mouse Scale Y",1,NULL,lpDWORD_B_0(&ikbd.abs_mouse_scale_y));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x026,"IKBD Absolute Mouse Buttons",1,
            "RMB Down|RMB Was Down|LMB Down|LMB Was Down",lpDWORD_B_0(&ikbd.abs_mousek_flags));
  iolist_add_entry(IOLIST_PSEUDO_AD_IKBD+0x028,"IKBD Joy Button Duration",1,NULL,lpDWORD_B_0(&ikbd.duration));

#if defined(SSE_BOILER_BROWSER_6301)
  char buffer[80],mask[80]; //overkill for a time
  // internal registers $0-$15
  for(int i=0;i<256;i++)
  {
    mask[0]=NULL;
    sprintf(buffer,(i<0x16)?"IREG %02X":"RAM %02X",i);
    if(i==0x00) 
      strcat(buffer," DDR1");
    else if(i==0x01) 
      strcat(buffer," DDR2");
    else if(i==0x02) 
      strcat(buffer," DR1");
    else if(i==0x03) 
      strcat(buffer," DR2");
    else if(i==0x04) 
      strcat(buffer," DDR3");
    else if(i==0x05) 
      strcat(buffer," DDR4");
    else if(i==0x06) 
      strcat(buffer," DR3");
    else if(i==0x07) 
      strcat(buffer," DR4");
    else if(i==0x08) 
      strcat(buffer," TCSR");
    else if(i==0x09 || i==0x0A)
      strcat(buffer," FRC");
    else if(i==0x0B || i==0x0C)
      strcat(buffer," OCR");
    else if(i==0x0D || i==0x0E)
      strcat(buffer," ICR");
    else if(i==0x0F) 
      strcat(buffer," CSR");
    else if(i==0x10) 
      strcat(buffer," RMCR");
    else if(i==0x11) 
    {
      strcat(buffer," TRCSR");
      strcpy(mask,"RDRF|OVR|TDRE|RIE|RE|TIE|TE|WU"); //yeah!
    }
    else if(i==0x12) 
      strcat(buffer," RDR");
    else if(i==0x13) 
      strcat(buffer," TDR");
    else if(i==0x14) 
      strcat(buffer," RCR");
    else if(i<0x80)
      strcat(buffer," (NULL)");
    else if(i==0x80)
      strcat(buffer," [speculation ahead]");
    else if(i>=0x82 && i<=0x87) 
      strcat(buffer," Date+time");
    else if(i==0x88)
      strcat(buffer," Init"); // $AA
    else if(i==0x9B) 
      strcat(buffer," Buttons");
    else if(i==0xA4) 
      strcat(buffer," Joystick 0");
    else if(i==0xA5)
      strcat(buffer," Joystick 1");
    else if(i==0xAA || i==0xAB) 
      strcat(buffer," AbsX"); 
    else if(i==0xAC || i==0xAD)
      strcat(buffer," AbsY");
    else if(i==0xAE || i==0xAE)
      strcat(buffer," Mouse keycode");
    else if(i==0xB0 || i==0xB1)
      strcat(buffer," Mouse threshold ");
    else if(i==0xB2 || i==0xB3)
      strcat(buffer," Mouse scale ");
    else if(i==0xB4)
      strcat(buffer," Mouse button action");
    else if(i>=0xB5 && i<=0xB9)
      strcat(buffer," Abs Mouse report");
    else if(i==0xBC)
      strcat(buffer," Mouse X");
    else if(i==0xBD)
      strcat(buffer," Mouse Y");
    else if(i==0xBE)
      strcat(buffer," Mouse move X");
    else if(i==0xBF)
      strcat(buffer," Mouse move Y");
    else if(i>=0xC0 && i<=0xC2)
      strcat(buffer," Mouse buttons");
    else if(i==0xC9)
    {
      strcat(buffer," Mouse mode");
      strcpy(mask,"on|key|abs|evt|.|.|mon|rev"); 
    }
    else if(i==0xCA)
    {
      strcat(buffer," Joystick mode");
      strcpy(mask,".|.|on|int|evt|key|mon|but"); 
    }
    else if(i==0xCB)
    {
      strcat(buffer," Command status");
      strcpy(mask,"new|.|input full|complete|complete|par|par|par"); 
    }
    else if(i>=0xCD && i<=0xD4) 
      strcat(buffer," Input buffer");
    else if(i==0xD6)
      strcat(buffer," Output buffer index");
    else if(i==0xD7)
      strcat(buffer," Output buffer counter");
    else if(i>=0xD9 && i<=0xED) 
      strcat(buffer," Output buffer");
    else if(i>=0xFF-6) // size?
      strcat(buffer," Stack");
    iolist_add_entry(IOLIST_PSEUDO_AD_6301+i*2,buffer,1,mask[0]?mask:NULL,&Debug.HD6301RamBuffer[i]);
  }

#endif

#if defined(SSE_BOILER_PSEUDO_STACK)
  for(int i=0;i<PSEUDO_STACK_ELEMENTS;i++)
    iolist_add_entry(IOLIST_PSEUDO_AD_STACK+i*8,"stack",8,NULL,(BYTE*)(&Debug.PseudoStack[i]) );
#endif


}
//---------------------------------------------------------------------------
int __stdcall debug_plugin_read_mem(DWORD ad,BYTE *buf,int len)
{
  if (ad>=himem) return 0;
  if (ad+len>=himem) len=himem-ad;

  int n_bytes=len;
  BYTE *p=lpPEEK(ad);
  while (len--){
    *(buf++)=*p;
    p+=MEM_DIR;
  }
  return n_bytes;
}
//---------------------------------------------------------------------------
int __stdcall debug_plugin_write_mem(DWORD ad,BYTE *buf,int len)
{
  if (ad>=himem) return 0;
  if (ad+len>=himem) len=himem-ad;

  int n_bytes=len;
  BYTE *p=lpPEEK(ad);
  while (len--){
    *p=*(buf++);
    p+=MEM_DIR;
  }
  return n_bytes;
}
//---------------------------------------------------------------------------

#if defined(SSE_BOILER_MONITOR_RANGE) 
/*  Adding range check: is ad between ad1 and ad2
    We use the first 2 watches
*/
  //bool debug_check_wr_check_range(MEM_ADDRESS ad,int num,MEM_ADDRESS *adarr,bool wr) {
bool debug_check_wr_check_range(MEM_ADDRESS ad,int num,MEM_ADDRESS *adarr) {//390
  MEM_ADDRESS ad1=0,ad2=0;
  for(int i=0;i<num;i++)
  {
    if(!ad1)
      ad1=adarr[i];
    else if(!ad2)
    {
      ad2=adarr[i];
      break;
    }
  }
  if(ad1&&ad2&& (ad1<ad2 && ad1<=ad && ad<=ad2
    || ad1>ad2 && ad2<=ad && ad<=ad1))
  {
    return true;
  }
  return false;
}
#endif

#endif//#ifdef DEBUG_BUILD//SS