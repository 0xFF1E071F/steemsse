/*---------------------------------------------------------------------------
FILE: run.cpp
MODULE: emu
DESCRIPTION: This file contains Steem's run() function, the routine that
actually makes Steem go. Also included here is the code for the event system
that allows time-critical Steem functions (such as VBLs, HBLs and MFP timers)
to be scheduled to the nearest cycle. Speed limiting and drawing is also
handled here, in event_scanline and event_vbl_interrupt.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_INFO)
#pragma message("Included for compilation: run.cpp")
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_RUN_H)

#define EXT
#define INIT(s) =s

EXT int runstate;
// fast_forward_max_speed=(1000 / (max %/100)); 0 for unlimited
EXT int fast_forward INIT(0),fast_forward_max_speed INIT(0);
EXT bool fast_forward_stuck_down INIT(0);
EXT int slow_motion INIT(0),slow_motion_speed INIT(100);
EXT int run_speed_ticks_per_second INIT(1000);
EXT bool disable_speed_limiting INIT(0);
LOG_ONLY( EXT int run_start_time; )
UNIX_ONLY( EXT bool RunWhenStop INIT(0); )

EXT DWORD avg_frame_time INIT(0),avg_frame_time_timer,frame_delay_timeout,timer;
EXT DWORD speed_limit_wait_till;
EXT int avg_frame_time_counter INIT(0);
EXT DWORD auto_frameskip_target_time;

#if defined(STEVEN_SEAGAL) && defined(SS_VARIOUS___)
EXT int frameskip INIT(1);
#else
EXT int frameskip INIT(AUTO_FRAMESKIP);
#endif
EXT int frameskip_count INIT(1);

EXT bool flashlight_flag INIT(false);

DEBUG_ONLY(EXT int mode);

EXT int mixed_output INIT(0);

EXT int cpu_time_of_last_vbl,shifter_cycle_base;

EXT int cpu_timer_at_start_of_hbl;

//#ifdef IN_EMU

#if defined(STEVEN_SEAGAL) && defined(SS_INT_VBI_START)
screen_event_struct event_plan_50hz[313*2+2+1],event_plan_60hz[263*2+2+1],event_plan_70hz[600*2+2+1],
                    event_plan_boosted_50hz[313*2+2+1],event_plan_boosted_60hz[263*2+2+1],event_plan_boosted_70hz[600*2+2+1];

void event_trigger_vbi();

#else

screen_event_struct event_plan_50hz[313*2+2],event_plan_60hz[263*2+2],event_plan_70hz[600*2+2],
                    event_plan_boosted_50hz[313*2+2],event_plan_boosted_60hz[263*2+2],event_plan_boosted_70hz[600*2+2];
#endif

screen_event_struct*screen_event_pointer,*event_plan[4],*event_plan_boosted[4];

EVENTPROC event_mfp_timer_timeout[4]={event_timer_a_timeout,event_timer_b_timeout,
                          event_timer_c_timeout,event_timer_d_timeout};
int time_of_next_event;
EVENTPROC screen_event_vector;
int cpu_time_of_start_of_event_plan;

//int cpu_time_of_next_hbl_interrupt=0;
int time_of_next_timer_b=0;
int time_of_last_hbl_interrupt;
int screen_res_at_start_of_vbl;
int shifter_freq_at_start_of_vbl;
int scanline_time_in_cpu_cycles_at_start_of_vbl;
bool hbl_pending;

int cpu_timer_at_res_change;


//#endif

#if USE_PASTI
void event_pasti_update();
#endif

#undef EXT
#undef INIT


#endif

#include "SSE/SSEDebug.h"

#ifdef SHOW_DRAW_SPEED
extern HWND  StemWin;
#endif

//---------------------------------------------------------------------------
void exception(int exn,exception_action ea,MEM_ADDRESS a)
{
  io_word_access=0;
  ioaccess=0;
  ExceptionObject.init(exn,ea,a);
  if (pJmpBuf==NULL){
    log_write(Str("Unhandled exception! pc=")+HEXSl(old_pc,6)+" action="+int(ea)+" address involved="+HEXSl(a,6));
    BRK( Unhandled exception! ); //never yet
    return;
  }
  longjmp(*pJmpBuf,1);
}
//---------------------------------------------------------------------------
void run()
{
  bool ExcepHappened;

  Disp.RunStart();

  if (psg_always_capture_on_start) psg_capture(true,"test.stym");

  GUIRunStart();

  DEBUG_ONLY( debug_run_start(); )

#ifndef DISABLE_STEMDOS
  if (pc==rom_addr) stemdos_set_drive_reset();
#endif
  ikbd_run_start(pc==rom_addr);
  runstate=RUNSTATE_RUNNING;

#ifdef WIN32
  // Make timer accurate to 1ms
  TIMECAPS tc;
  tc.wPeriodMin=1;
  timeGetDevCaps(&tc,sizeof(TIMECAPS));
  timeBeginPeriod(tc.wPeriodMin);
#endif
  timer=timeGetTime();

  Sound_Start();

#if defined (STEVEN_SEAGAL) && defined(SS_SHIFTER_TRICKS)
  Shifter.AddFreqChange(shifter_freq);
#else
  ADD_SHIFTER_FREQ_CHANGE(shifter_freq);
#endif	

  init_screen();

  if (bad_drawing==0){
    draw_begin();
    DEBUG_ONLY( debug_update_drawing_position(); )
  }

  PortsRunStart();

  DEBUG_ONLY(mode=STEM_MODE_CPU;)

  log_write(">>> Start Emulation <<<");

#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG_START_STOP_INFO)
  Debug.ReportGeneralInfos(TDebug::START);
#endif

  DEBUG_ONLY( debug_first_instruction=true; ) // Don't break if running from breakpoint

  timer=timeGetTime();
  LOG_ONLY( run_start_time=timer; ) // For log speed limiting
  osd_init_run(true);

  frameskip_count=1;
  speed_limit_wait_till=timer+((run_speed_ticks_per_second+(shifter_freq/2))/shifter_freq);
  avg_frame_time_counter=0;
  avg_frame_time_timer=timer;

  // I don't think this can do any damage now, it just checks its
  // list and updates cpu_timer and cpu_cycles
  DEBUG_ONLY( prepare_next_event(); )

  ioaccess=0;
  if (Blit.Busy) Blitter_Draw();
  do{
    ExcepHappened=0;

    TRY_M68K_EXCEPTION

      while (runstate==RUNSTATE_RUNNING){
        // cpu_cycles is the amount of cycles before next event.
        // SS It is *decremented* by instruction timings, not incremented.
        while (cpu_cycles>0 && runstate==RUNSTATE_RUNNING){

          DEBUG_ONLY( pc_history[pc_history_idx++]=pc; )
          DEBUG_ONLY( if (pc_history_idx>=HISTORY_SIZE) pc_history_idx=0; )
          
#define LOGSECTION LOGSECTION_CPU
          m68k_PROCESS 
#undef LOGSECTION

          DEBUG_ONLY( debug_first_instruction=0; )
          CHECK_BREAKPOINT
        }//while (cpu_cycles>0 && runstate==RUNSTATE_RUNNING)
        DEBUG_ONLY( if (runstate!=RUNSTATE_RUNNING) break; )
        DEBUG_ONLY( mode=STEM_MODE_INSPECT; )
       
        while (cpu_cycles<=0){
          screen_event_vector(); 
          prepare_next_event();
          // This has to be in while loop as it can cause an interrupt,
          // thus making another event happen.
          if (cpu_cycles>0) check_for_interrupts_pending();

        }//while (cpu_cycles<=0)

        CHECK_BREAKPOINT

        DEBUG_ONLY( mode=STEM_MODE_CPU; )
//---------------------------------------------------------------------------
      }//while (runstate==RUNSTATE_RUNNING)
    CATCH_M68K_EXCEPTION
      m68k_exception e=ExceptionObject;
      ExcepHappened=true;
#ifndef DEBUG_BUILD
      e.crash();
#else
      mode=STEM_MODE_INSPECT;
      bool alertflag=false;
      if (crash_notification!=CRASH_NOTIFICATION_NEVER){
        alertflag=true;
//        try{
        TRY_M68K_EXCEPTION
          WORD a=m68k_dpeek(LPEEK(e.bombs*4));
          if (e.bombs>8){
            alertflag=false;
          }else if (crash_notification==CRASH_NOTIFICATION_BOMBS_DISPLAYED &&
                   a!=0x6102 && a!=0x4eb9 ){ //not bombs routine
            alertflag=false;
          }
//        }catch (m68k_exception &m68k_e){
        CATCH_M68K_EXCEPTION
          alertflag=true;
        END_M68K_EXCEPTION
      }
      if (alertflag==0){
        e.crash();
      }else{
        bool was_locked=draw_lock;
        draw_end();
        draw(false);
        if (IDOK==Alert("Exception - do you want to crash (OK)\nor trace? (CANCEL)",EasyStr("Exception ")+e.bombs,
                          MB_OKCANCEL | MB_ICONEXCLAMATION)){
          e.crash();
          if (was_locked) draw_begin();
        }else{
          runstate=RUNSTATE_STOPPING;
          e.crash(); //crash
          debug_trace_crash(e);
          ExcepHappened=0;
        }
      }
      if (debug_num_bk) breakpoint_check();
      if (runstate!=RUNSTATE_RUNNING) ExcepHappened=0;
#endif
    END_M68K_EXCEPTION
  }while (ExcepHappened);

  PortsRunEnd();

  Sound_Stop(Quitting);

  Disp.RunEnd();

  runstate=RUNSTATE_STOPPED;

  GUIRunEnd();

  draw_end();

  CheckResetDisplay();

#ifdef DEBUG_BUILD
  if (redraw_on_stop){
    draw(0);
  }else{
    update_display_after_trace();
  }
  debug_run_until=DRU_OFF;
#else
  osd_draw_full_stop();
#endif

  DEBUG_ONLY( debug_run_end(); )

  log_write(">>> Stop Emulation <<<");

#if defined(STEVEN_SEAGAL) && defined(SS_DEBUG_START_STOP_INFO)
  Debug.ReportGeneralInfos(TDebug::STOP);
#endif

#ifdef WIN32
  timeEndPeriod(tc.wPeriodMin); // Finished with accurate timing
#endif

  ONEGAME_ONLY( OGHandleQuit(); )

#ifdef UNIX
  if (RunWhenStop){
    PostRunMessage();
    RunWhenStop=0;
  }
#endif
  psg_capture(0,"");
}
//---------------------------------------------------------------------------
#ifdef DEBUG_BUILD
void event_debug_stop()
{
  if (runstate==RUNSTATE_RUNNING) runstate=RUNSTATE_STOPPING;
  debug_run_until=DRU_OFF; // Must be here to prevent freeze up as this event never goes into the future!
}
#endif
//---------------------------------------------------------------------------
void inline prepare_event_again() //might be an earlier one
{
  //  new 3/7/2001 - if, say, a timer period is extended so that the next event
  //  in the plan is to be postponed, we need to compare the next screen event
  //  as well as all the timer timeouts to work out which one is due next.  That's
  //  why the time_of_next_event is reset here.
  time_of_next_event=cpu_time_of_start_of_event_plan+(screen_event_pointer->time);
  screen_event_vector=(screen_event_pointer->event); // SS pointer to function
  //  end of new 3/7/2001


    //  PREPARE_EVENT_CHECK_FOR_DMA_SOUND_END
    //check timers for timeouts
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(0);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(1);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(2);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(3);
    
    PREPARE_EVENT_CHECK_FOR_TIMER_B;
    
    PREPARE_EVENT_CHECK_FOR_DEBUG;
    
    PREPARE_EVENT_CHECK_FOR_PASTI;

#if defined(STEVEN_SEAGAL) && defined(SS_DMA_DELAY)
    PREPARE_EVENT_CHECK_FOR_DMA;
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_ACIA_IRQ_DELAY)
    PREPARE_EVENT_CHECK_FOR_ACIA_IKBD_IN;
#endif

  // cpu_timer must always be set to the next 4 cycle boundary after time_of_next_event
  int oo=time_of_next_event-cpu_timer;
  oo=(oo+3) & -4;

//  log_write(EasyStr("prepare event again: offset=")+oo);
  cpu_cycles+=oo;cpu_timer+=oo;
}

void inline prepare_next_event()
{
  time_of_next_event=cpu_time_of_start_of_event_plan + screen_event_pointer->time;
  screen_event_vector=(screen_event_pointer->event); // SS pointer to function

    //  PREPARE_EVENT_CHECK_FOR_DMA_SOUND_END
    
    // check timers for timeouts
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(0);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(1);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(2);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(3);
    
    PREPARE_EVENT_CHECK_FOR_TIMER_B;
      
    PREPARE_EVENT_CHECK_FOR_DEBUG;
      
    PREPARE_EVENT_CHECK_FOR_PASTI;

#if defined(STEVEN_SEAGAL) && defined(SS_DMA_DELAY)
    PREPARE_EVENT_CHECK_FOR_DMA;
#endif
#if defined(STEVEN_SEAGAL) && defined(SS_ACIA_IRQ_DELAY)
    PREPARE_EVENT_CHECK_FOR_ACIA_IKBD_IN;
#endif

  // It is safe for events to be in past, whatever happens events
  // cannot get into a constant loop.
  // If a timer is set to shorter than the time for an MFP interrupt then it will
  // happen a few times, but eventually will go into the future (as the interrupt can
  // only fire once, when it raises the IPL).

  int oo=time_of_next_event-cpu_timer;
  oo=(oo+3) & -4;
  cpu_cycles+=oo;cpu_timer+=oo;
}
//---------------------------------------------------------------------------
#define LOGSECTION LOGSECTION_MFP_TIMERS


#if defined(SS_MFP_RATIO_PRECISION)

#define HANDLE_TIMEOUT(tn) \
  log(Str("MFP: Timer ")+char('A'+tn)+" timeout at "+ABSOLUTE_CPU_TIME+" timeout was "+mfp_timer_timeout[tn]+ \
    " period was "+mfp_timer_period[tn]); \
  if (mfp_timer_period_change[tn]){    \
    MFP_CALC_TIMER_PERIOD(tn);          \
    mfp_timer_period_change[tn]=0;       \
  }                                       \
  int stage=(mfp_timer_timeout[tn]-ABSOLUTE_CPU_TIME); \
  if (stage<=0){                                       \
    stage+=((-stage/mfp_timer_period[tn])+1)*mfp_timer_period[tn]; \
  }else{ \
    stage%=mfp_timer_period[tn]; \
  }   \
  int new_timeout=ABSOLUTE_CPU_TIME+stage; \
  mfp_timer_period_current_fraction[tn]+=mfp_timer_period_fraction[tn]; \
  if(mfp_timer_period_current_fraction[tn]>=4000) {\
    mfp_timer_period_current_fraction[tn]-=4000;\
    new_timeout+=4; \
  }


#else
#define HANDLE_TIMEOUT(tn) \
  log(Str("MFP: Timer ")+char('A'+tn)+" timeout at "+ABSOLUTE_CPU_TIME+" timeout was "+mfp_timer_timeout[tn]+ \
    " period was "+mfp_timer_period[tn]); \
  if (mfp_timer_period_change[tn]){    \
    MFP_CALC_TIMER_PERIOD(tn);          \
    mfp_timer_period_change[tn]=0;       \
  }                                       \
  int stage=(mfp_timer_timeout[tn]-ABSOLUTE_CPU_TIME); \
  if (stage<=0){                                       \
    stage+=((-stage/mfp_timer_period[tn])+1)*mfp_timer_period[tn]; \
  }else{ \
    stage%=mfp_timer_period[tn]; \
  }   \
  int new_timeout=ABSOLUTE_CPU_TIME+stage;
#endif


void event_timer_a_timeout()
{
  HANDLE_TIMEOUT(0);
  mfp_interrupt_pend(MFP_INT_TIMER_A,mfp_timer_timeout[0]);
  mfp_timer_timeout[0]=new_timeout;
}
void event_timer_b_timeout()
{
  HANDLE_TIMEOUT(1);
  mfp_interrupt_pend(MFP_INT_TIMER_B,mfp_timer_timeout[1]);
  mfp_timer_timeout[1]=new_timeout;
}
void event_timer_c_timeout()
{
  HANDLE_TIMEOUT(2);
  mfp_interrupt_pend(MFP_INT_TIMER_C,mfp_timer_timeout[2]);
  mfp_timer_timeout[2]=new_timeout;
}
void event_timer_d_timeout()
{
  HANDLE_TIMEOUT(3);
  mfp_interrupt_pend(MFP_INT_TIMER_D,mfp_timer_timeout[3]);
  mfp_timer_timeout[3]=new_timeout;
}
#undef LOGSECTION
//---------------------------------------------------------------------------
#define LOGSECTION LOGSECTION_INTERRUPTS
void event_timer_b()
{
  if (scan_y<shifter_first_draw_line){
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;
  }else if (scan_y<shifter_last_draw_line){
    if (mfp_reg[MFPR_TBCR]==8){
      // There is a problem that this draw_check_border_removal() can happen before
      // event_scanline but after a change to mono for left border removal, this
      // stops the border opening on the next line somehow.
      mfp_timer_counter[1]-=64;
      log_to(LOGSECTION_MFP_TIMERS,EasyStr("MFP: Timer B counter decreased to ")+(mfp_timer_counter[1]/64)+" at "+scanline_cycle_log());
      if (mfp_timer_counter[1]<64){
        log(EasyStr("MFP: Timer B timeout at ")+scanline_cycle_log());
        TRACE_LOG("F%d y%d Timer B%d\n",FRAME,scan_y,1);
        mfp_timer_counter[1]=BYTE_00_TO_256(mfp_reg[MFPR_TBDR])*64;
        mfp_interrupt_pend(MFP_INT_TIMER_B,time_of_next_timer_b);
      }
    }
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+cpu_cycles_from_hbl_to_timer_b+
        scanline_time_in_cpu_cycles_at_start_of_vbl + TB_TIME_WOBBLE;
  }else{
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;
  }
}
#undef LOGSECTION
//---------------------------------------------------------------------------
void event_hbl()   //just HBL, don't draw yet
{
/* 
  SS: It seems this function is called only once per frame before first line
  is drawn, event_scanline is called for each line and takes over this hbl's
  tasks, including hbl_pending (the interrupts are triggered in mfp.cpp).
  IPL 2 - note usual level=3, so HBL isn't executed; if it is, it is triggered
  in check_for_interrupts_pending() (mfp.cpp).
  For 50hz, 313 lines, 1 call to event_hbl, 312 calls to event_scanline
*/

#define LOGSECTION LOGSECTION_AGENDA
  CHECK_AGENDA
#undef LOGSECTION

  log_to_section(LOGSECTION_VIDEO,EasyStr("VIDEO: Event HBL at end of line ")+scan_y+", cycle "+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl));
  right_border_changed=0;//SS useful in SSE?
  scanline_drawn_so_far=0;
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  cpu_timer_at_start_of_hbl=time_of_next_event; // SS as defined in draw.cpp
#if defined(STEVEN_SEAGAL) && defined(SS_SHIFTER)
  Shifter.IncScanline();
  ASSERT(scan_y==-32 || scan_y==-62 || scan_y==-33);
#else
  scan_y++;
#endif
#ifdef DEBUG_BUILD
  if (debug_run_until==DRU_SCANLINE){
    if (debug_run_until_val==scan_y){
      if (runstate==RUNSTATE_RUNNING) runstate=RUNSTATE_STOPPING;
    }
  }
#endif
  if (abs_quick(cpu_timer_at_start_of_hbl-time_of_last_hbl_interrupt)>CYCLES_FROM_START_OF_HBL_IRQ_TO_WHEN_PEND_IS_CLEARED){
    hbl_pending=true;
  }
  if (dma_sound_on_this_screen) dma_sound_fetch();//,dma_sound_fetch();
  screen_event_pointer++;  

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
  // we run some 6301 cycles at the end of each scanline (x1)
  if(HD6301EMU_ON && !HD6301.Crashed)
  {
#if defined(SS_IKBD_6301_RUN_CYCLES_AT_IO)
    if(!HD6301.RunThisHbl) 
#endif
    {
      ASSERT(HD6301_OK);
      int n6301cycles;
#if defined(SS_SHIFTER)
      n6301cycles=Shifter.CurrentScanline.Cycles/HD6301_CYCLE_DIVISOR;
#else
      n6301cycles=(screen_res==2) ? 20 : HD6301_CYCLES_PER_SCANLINE; //64
#endif
      ASSERT(n6301cycles);
      if(!hd6301_run_cycles(n6301cycles))
      {
#define LOGSECTION LOGSECTION_IKBD
        TRACE_LOG("6301 emu is hopelessly crashed!\n");
#undef LOGSECTION
        HD6301.Crashed=1; 
      }
    }
#if defined(SS_IKBD_6301_RUN_CYCLES_AT_IO)
    HD6301.RunThisHbl=0; // reset for next hbl
#endif
  }
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_IPF) && !defined(SS_IPF_CPU)
  if(Caps.Active) 
    Caps.Hbl(); 
#endif
}
//---------------------------------------------------------------------------

//SS: The pointer to this function is used, so don't mess with it C++ like!
void event_scanline()
{
#define LOGSECTION LOGSECTION_AGENDA
  CHECK_AGENDA;
#undef LOGSECTION

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
  // we run some 6301 cycles at the end of each scanline (x312)
  if(HD6301EMU_ON && !HD6301.Crashed)
  {
#if defined(SS_IKBD_6301_RUN_CYCLES_AT_IO)
    if(!HD6301.RunThisHbl) 
#endif
    {
      ASSERT(HD6301_OK);
      int n6301cycles;
#if defined(SS_SHIFTER)
      n6301cycles=Shifter.CurrentScanline.Cycles/HD6301_CYCLE_DIVISOR;
#else
      n6301cycles=(screen_res==2) ? 20 : HD6301_CYCLES_PER_SCANLINE; //64
#endif
      ASSERT(n6301cycles);
      if(hd6301_run_cycles(n6301cycles)==-1)
      {
#define LOGSECTION LOGSECTION_IKBD
        TRACE_LOG("6301 emu is hopelessly crashed!\n");
#undef LOGSECTION
        HD6301.Crashed=1; 
      }
    }
#if defined(SS_IKBD_6301_RUN_CYCLES_AT_IO)
    HD6301.RunThisHbl=0; // reset for next hbl
#endif
  }
#endif

#if defined(STEVEN_SEAGAL)
/*  Note: refactoring here is very dangerous!
    We must separate SS_SHIFTER from SS_INTERRUPT, which makes for many
    #if blocks.
*/
  if (scan_y<shifter_first_draw_line-1){
    if (scan_y>=draw_first_scanline_for_border){
      if (bad_drawing==0) 
#if defined(SS_SHIFTER) &&!defined(SS_SHIFTER_DRAW_DBG)
        Shifter.DrawScanlineToEnd();
#else
        draw_scanline_to_end();
#endif
      time_of_next_timer_b=time_of_next_event+160000;  //put into future
    }
  }else if (scan_y<shifter_first_draw_line){ //next line is first visible
    if (bad_drawing==0) 
#if defined(SS_SHIFTER) &&!defined(SS_SHIFTER_DRAW_DBG)
      Shifter.DrawScanlineToEnd();
#else
      draw_scanline_to_end();
#endif
    time_of_next_timer_b=time_of_next_event
      +cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
  }else if (scan_y<shifter_last_draw_line-1){
    if (bad_drawing==0) 
#if defined(SS_SHIFTER) &&!defined(SS_SHIFTER_DRAW_DBG)
      Shifter.DrawScanlineToEnd();
#else
      draw_scanline_to_end();
#endif
    time_of_next_timer_b=time_of_next_event
      +cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
#if defined(SS_MFP_TIMER_B_AER)
//  from Hatari, fixes Seven Gates of Jambala; Trex Warrior
    if(mfp_reg[1]&8)
      time_of_next_timer_b-=320; //gross?
#endif
  }else if (scan_y<draw_last_scanline_for_border){
    if (bad_drawing==0) 
#if defined(SS_SHIFTER) &&!defined(SS_SHIFTER_DRAW_DBG)
      Shifter.DrawScanlineToEnd();
#else
      draw_scanline_to_end();
#endif
    time_of_next_timer_b=time_of_next_event+160000;  //put into future
  }

#else // Steem 3.2

  if (scan_y<shifter_first_draw_line-1){
    if (scan_y>=draw_first_scanline_for_border){
      if (bad_drawing==0) draw_scanline_to_end();
      time_of_next_timer_b=time_of_next_event+160000;  //put into future
    }
  }else if (scan_y<shifter_first_draw_line){ //next line is first visible
    if (bad_drawing==0) draw_scanline_to_end();
    time_of_next_timer_b=time_of_next_event+cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
  }else if (scan_y<shifter_last_draw_line-1){
    if (bad_drawing==0) draw_scanline_to_end();
    time_of_next_timer_b=time_of_next_event+cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
  }else if (scan_y<draw_last_scanline_for_border){
    if (bad_drawing==0) draw_scanline_to_end();
    time_of_next_timer_b=time_of_next_event+160000;  //put into future
  }

#endif

  log_to(LOGSECTION_VIDEO,EasyStr("VIDEO: Event Scanline at end of line ")+scan_y+" sdp is $"+HEXSl(shifter_draw_pointer,6));
  
#if defined(STEVEN_SEAGAL) && defined(SS_SHIFTER) &&!defined(SS_SHIFTER_DRAW_DBG)
  Shifter.CheckSyncTrick(); // check for +2 -2 errors
  Shifter.CheckVerticalOverscan(); // top & bottom borders
#else // Steem 3.2's vertical overscan check
  if (shifter_freq_at_start_of_vbl==50){
    if (scan_y==-30 || scan_y==199 || scan_y==225){
      // Check top/bottom overscan
      int freq_at_trigger=shifter_freq;
      if (screen_res==2) freq_at_trigger=MONO_HZ;
      if (emudetect_overscans_fixed){
        freq_at_trigger=(scan_y==-30 || scan_y==199) ? 60:0;
      }else if (freq_change_this_scanline){
        // Accurate version!
        int t,i=shifter_freq_change_idx;
        if (scan_y==225){
          // close in different place, this is a guess but only about 2 programs use it!
          t=cpu_timer_at_start_of_hbl+CYCLES_FROM_HBL_TO_RIGHT_BORDER_CLOSE+48;
        }else{
          t=cpu_timer_at_start_of_hbl+CYCLES_FROM_HBL_TO_RIGHT_BORDER_CLOSE+98;
        }
        while (shifter_freq_change_time[i]>t){
          i--; i&=31;
        }
        freq_at_trigger=shifter_freq_change[i];
      }
      if (freq_at_trigger==60){
        if (scan_y==-30){
          log_to_section(LOGSECTION_VIDEO,EasyStr("VIDEO: TOP BORDER REMOVED"));
          
          shifter_first_draw_line=-29;
          overscan=OVERSCAN_MAX_COUNTDOWN;
          time_of_next_timer_b=time_of_next_event+cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
          if (FullScreen && border==2){    //hack overscans
            int off=shifter_first_draw_line-draw_first_possible_line;
            draw_first_possible_line+=off;
            draw_last_possible_line+=off;
          }
          
        }else if (scan_y==199){
          // Turn on bottom overscan
          log_to_section(LOGSECTION_VIDEO,EasyStr("VIDEO: BOTTOM BORDER REMOVED"));
          overscan=OVERSCAN_MAX_COUNTDOWN;
          
          // Timer B will fire for the last time when scan_y is 246
          shifter_last_draw_line=247;
          
          // Must be time of the next scanline or we don't get a Timer B on scanline 200!
          time_of_next_timer_b=time_of_next_event+cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
        }else if (scan_y==225){
          if (shifter_last_draw_line>200){
            log_to_section(LOGSECTION_VIDEO,EasyStr("VIDEO: BOTTOM BORDER TURNED BACK ON"));
            shifter_last_draw_line=225;
            time_of_next_timer_b=time_of_next_event+160000;  //put into future
          }
        }
      }
    }
  }
#endif

  if (freq_change_this_scanline){
#if defined(SS_SHIFTER_TRICKS)
    if(shifter_freq_change_time[shifter_freq_change_idx]<time_of_next_event-16
      && shifter_shift_mode_change_time[shifter_shift_mode_change_idx]
      <time_of_next_event-16)
      {
#else
    if (shifter_freq_change_time[shifter_freq_change_idx]<time_of_next_event-16){
#endif
      freq_change_this_scanline=0;
    }

    if (draw_line_off){ // SS: we had a blank line = all colours black
      palette_convert_all();
      draw_line_off=0;
    }
  }
  right_border_changed=0;
  scanline_drawn_so_far=0;
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  /////// SS as defined in draw.cpp,
  /////// and relative to cpu_time_of_last_vbl:
  cpu_timer_at_start_of_hbl=time_of_next_event; 

#if defined(STEVEN_SEAGAL) && defined(SS_IPF) && !defined(SS_IPF_CPU)
  if(Caps.Active)
    Caps.Hbl();
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_SHIFTER)
  Shifter.IncScanline();
#else
  scan_y++;
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_POLL_IN_FRAME)
  // We peek Windows message once during the frame and not just at VBL
  if(scan_y==ikbd.scanline_to_poll
    && shifter_freq_at_start_of_vbl!=60 // HighRes Mode (hack) TODO: why?
#if defined(SS_HACKS)
    && SSE_HACKS_ON // in case there's more trouble
#endif
    ) // scanline_to_poll is different each VBL
    PeekEvent();  // fixes Corporation STE, but messes HighResMode
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_INT_JITTER)
    HblJitterIndex++; // exactly like in Hatari
    if(HblJitterIndex==5)
      HblJitterIndex=0;
#endif

#ifdef DEBUG_BUILD
  if (debug_run_until==DRU_SCANLINE){
    if (debug_run_until_val==scan_y){
      if (runstate==RUNSTATE_RUNNING) runstate=RUNSTATE_STOPPING;
    }
  }
#endif
  if (abs_quick(cpu_timer_at_start_of_hbl-time_of_last_hbl_interrupt)>CYCLES_FROM_START_OF_HBL_IRQ_TO_WHEN_PEND_IS_CLEARED){
    hbl_pending=true;
  }
  if (dma_sound_on_this_screen) dma_sound_fetch(); 
  screen_event_pointer++;
}
//---------------------------------------------------------------------------
//#undef LOGSECTION LOGSECTION_INTERRUPTS
#define LOGSECTION LOGSECTION_INTERRUPTS//SS

void event_start_vbl()
{
  // This happens about 60 cycles into scanline 247 (50Hz)

  TRACE_LOG("F%d L%d reload SDP (%X) <- %X\n",FRAME,scan_y,shifter_draw_pointer,xbios2);
  shifter_draw_pointer=xbios2; // SS: reload SDP
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  shifter_pixel=shifter_hscroll;
  overscan_add_extra=0; //SS?
  left_border=BORDER_SIDE;right_border=BORDER_SIDE;
  screen_event_pointer++;
}

#undef LOGSECTION

//---------------------------------------------------------------------------
void event_vbl_interrupt() //SS misleading name?
{ 
  bool VSyncing=(FSDoVsync && FullScreen && fast_forward==0 && slow_motion==0);
  bool BlitFrame=0;
#ifndef NO_CRAZY_MONITOR
  if (extended_monitor==0)
#endif
  { // Make sure whole screen is drawn (in 60Hz and 70Hz there aren't enough lines)
    while (scan_y<draw_last_scanline_for_border){
#if defined(STEVEN_SEAGAL) && defined(SS_SHIFTER)
      if(!bad_drawing) 
        Shifter.DrawScanlineToEnd();
      scanline_drawn_so_far=0;
      shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
      Shifter.IncScanline();
#else
      if (bad_drawing==0) draw_scanline_to_end();
      scanline_drawn_so_far=0;
      shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
      scan_y++;
#endif
    }
    scanline_drawn_so_far=0;
    shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  }
  //-------- display to screen -------
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished frame, blitting at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));
  if (draw_lock){
    draw_end();
    if (VSyncing==0) draw_blit();
    BlitFrame=true;
  }else if (bad_drawing & 2){
    // bad_drawing bits: & 1 - bad drawing option selected  & 2 - bad-draw next screen
    //                   & 4 - temporary bad drawing because of extended monitor.
    draw(0);
    bad_drawing&=(~2);
  }
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished blitting at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));

  //----------- VBL interrupt ---------
#if !(defined(STEVEN_SEAGAL) && defined(SS_INT_VBI_START))
  vbl_pending=true;
#endif

/* //SS this was a commented out part:
  if ((sr & SR_IPL)<SR_IPL_4){ //level 4 interupt to m68k, is VBL interrupt enabled?
    VBL_INTERRUPT
  }else{
    vbl_pending=true;
  }
*/

  scan_y=-scanlines_above_screen[shifter_freq_idx];

  if (floppy_mediach[0]) floppy_mediach[0]--;  //counter for media change
  if (floppy_mediach[1]) floppy_mediach[1]--;  //counter for media change

  if (border & 2){ //auto-border
    if (overscan){	// SS: overscan started at 25, so it's in VBL
      overscan--;
      if ((border & 1)==0){
        //change to bordered mode
        if (FullScreen==0 || draw_fs_blit_mode==DFSM_LAPTOP){ //otherwise fudge overscan, and don't change border&1 !
          border|=1;
          if (FullScreen==0) change_window_size_for_border_change(0,1);
        }
      }
      if (overscan<=0){
        overscan=0;
        if (border & 1){ //overscan's finished
          if (FullScreen==0 || draw_fs_blit_mode==DFSM_LAPTOP){
            border&=-2;
            change_window_size_for_border_change(1,0);
          }
        }else if (FullScreen){ //finish fudging
          WIN_ONLY( draw_fs_topgap=40; )
          draw_grille_black=max(draw_grille_black,4);
        }
      }
    }
  }
  if (mixed_output>0){
    mixed_output--;
    if (mixed_output==2){
      init_screen();
      res_change();
    }else if (mixed_output==0){
      init_screen();
      if (screen_res==0) res_change();
      screen_res_at_start_of_vbl=screen_res;
    }
  }else if (screen_res!=screen_res_at_start_of_vbl){
    init_screen();
    res_change();
    screen_res_at_start_of_vbl=screen_res;
  }
  log_to_section(LOGSECTION_VIDEO,EasyStr("VIDEO: VBL interrupt - next screen is in freq ")+shifter_freq);

#ifdef SHOW_DRAW_SPEED
  {
    HDC dc=GetDC(StemWin);
    if (dc!=NULL){
      char buf[16];
      ultoa(avg_frame_time*10/12,buf,10);
      TextOut(dc,2,MENUHEIGHT+2,buf,strlen(buf));
      ReleaseDC(StemWin,dc);
    }
  }
#endif

  //------------ Shortcuts -------------
  if ( (--shortcut_vbl_count)<0 ){
    ShortcutsCheck();
    shortcut_vbl_count=SHORTCUT_VBLS_BETWEEN_CHECKS;
  }

  //------------- Auto Frameskip Calculation -----------
  if (frameskip==AUTO_FRAMESKIP){ //decide if we are ahead of schedule
    if (fast_forward==0 && slow_motion==0 && VSyncing==0){
      timer=timeGetTime();
      if (timer<auto_frameskip_target_time){
        frameskip_count=1;   //we are ahead of target so draw the next frame
        speed_limit_wait_till=auto_frameskip_target_time;
      }else{
#define LOGSECTION LOGSECTION_VIDEO//SS
        TRACE_LOG("Overload - Skip VBL %d\n",FRAME);
#undef LOGSECTION
        auto_frameskip_target_time+=(run_speed_ticks_per_second+(shifter_freq/2))/shifter_freq;
      }
    }else if (VSyncing){
      frameskip_count=1;   //disable auto frameskip
      auto_frameskip_target_time=timer;
    }
  }

  int time_for_exact_limit=1;

  // Work out how long to wait until we start next screen
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Getting ready to wait at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));
  if (slow_motion){
    int i=int((cut_slow_motion_speed) ? cut_slow_motion_speed:slow_motion_speed);
    frame_delay_timeout=timer+(1000000/i)/shifter_freq;
    auto_frameskip_target_time=timer;
    frameskip_count=1;
  }else if ((frameskip_count<=1 || fast_forward) && disable_speed_limiting==0){
    frame_delay_timeout=speed_limit_wait_till;
    if (VSyncing){
      // Allow up to a 25% increase in run speed
      time_for_exact_limit=((run_speed_ticks_per_second+(shifter_freq/2))/shifter_freq)/4;
    }
  }else{
    frame_delay_timeout=timer;
  }
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Going to wait until ")+(frame_delay_timeout-run_start_time)+" timer="+(timer-run_start_time));

  //--------- Look for Windows messages ------------
  int old_slow_motion=slow_motion;
  int m=0;
  LOOP{
    timer=timeGetTime();

    // Break if used up enough time and processed at least 3 messages
    if (int(frame_delay_timeout-timer)<=time_for_exact_limit && m>=3) break;

    // Get next message from the queue, if none left then go to the Sleep
    // routine in Windows to give other processes more time. Also do that
    // if more than 15 messages have been retrieved.
    // Don't go to Sleep if slow motion is on, that way the message to turn
    // it off can be dealt with instantly.
    log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Getting message at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));
    if (PeekEvent()==PEEKED_NOTHING || m>15){
      // Get more than 15 messages if slow motion is on, otherwise GUI will lock up
      // Should really do something to stop high CPU load when slow_motion is on
      if (slow_motion==0){
        if (old_slow_motion){
          // Don't sleep if you just turned slow motion off (stops annoying GUI delay)
          frame_delay_timeout=timer;
        }
        break;
      }
    }
    m++;
  }
  if (new_n_cpu_cycles_per_second){
    if (new_n_cpu_cycles_per_second!=n_cpu_cycles_per_second){
      n_cpu_cycles_per_second=new_n_cpu_cycles_per_second;
      prepare_cpu_boosted_event_plans();
    }
    new_n_cpu_cycles_per_second=0;
  }
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished getting messages at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));

  {
    // Eat up remaining time-time_for_exact_limit with Sleep
    int time_to_sleep=(int(frame_delay_timeout)-int(timeGetTime()))-time_for_exact_limit;
    if (time_to_sleep>0){
      log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Sleeping for ")+time_to_sleep);
      Sleep(DWORD(time_to_sleep));
    }

    if (VSyncing && BlitFrame){
      Disp.VSync();
      timer=timeGetTime();
      draw_blit();
#ifdef ENABLE_LOGFILE
      if (timer>speed_limit_wait_till+5){
        log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: !!!!!!!!! SLOW FRAME !!!!!!!!!"));
      }
      log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished Vsynced frame at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));
#endif
    }else{

      log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Doing exact timing at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));
      // Wait until desired time (to nearest 1000th of a second).
      do{
        timer=timeGetTime();
      }while (int(frame_delay_timeout-timer)>0);
      log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished speed limiting at ")+(timer-run_start_time));

      // For some reason when we get here timer can be > frame_delay_timeout, even if
      // we are running very fast. This line makes it so we don't lose a millisecond
      // here and there.
      if (time_to_sleep>0) timer=frame_delay_timeout;
    }
  }

  //-------------- Pause Steem -------------
  if (GUIPauseWhenInactive()){
    timer=timeGetTime();
    avg_frame_time_timer=timer;
    avg_frame_time_counter=0;
    auto_frameskip_target_time=timer;
  }

  if (floppy_access_ff_counter>0){
    floppy_access_ff_counter--;
    if (fast_forward==0 && floppy_access_ff_counter>0){
      fast_forward_change(true,0);
      floppy_access_started_ff=true;
    }else if (fast_forward && floppy_access_ff_counter==0 && floppy_access_started_ff){
      fast_forward_change(0,0);
    }
  }

  //--------- Work out avg_frame_time (for OSD) ----------
  if (avg_frame_time==0){
    avg_frame_time=(timer-avg_frame_time)*12;
  }else if (++avg_frame_time_counter>=12){
    avg_frame_time=timer-avg_frame_time_timer; //take average of frame time over 12 frames, ignoring the time we've skipped
    avg_frame_time_timer=timer;
    avg_frame_time_counter=0;
  }

  JoyGetPoses(); // Get the positions of all the PC joysticks
  if (slow_motion){
    // Extra screenshot check (so you actually take a picture of what you see)
    frameskip_count=0;
    ShortcutsCheck();
    if (DoSaveScreenShot & 1){
      Disp.SaveScreenShot();
      DoSaveScreenShot&=~1;
    }
  }
  IKBD_VBL();    // Handle ST joysticks and mouse
  RS232_VBL();   // Update all flags, check for the phone ringing
  Sound_VBL();   // Write a VBLs worth + a bit of samples to the sound card
  dma_sound_channel_buf_last_write_t=0;  //need to maintain this even if sound off
  dma_sound_on_this_screen=(dma_sound_control & BIT_0) || dma_sound_internal_buf_len;

  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished event_vbl tasks at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));

  //---------- Frameskip -----------
  if ((--frameskip_count)<=0){
    if (runstate==RUNSTATE_RUNNING && bAppMinimized==0){
#ifndef NO_CRAZY_MONITOR
      if (extended_monitor){
//bad_drawing bits: &1 - bad drawing option selected  &2 - bad-draw next screen
//                  &4 - temporary bad drawing because of extended monitor.
        bad_drawing|=6;
//        if(!FullScreen && em_needs_fullscreen)bad_drawing=4;
      }else
#endif
      {
        bad_drawing&=3;
        if (bad_drawing & 1){
          bad_drawing|=3;
        }else{
          draw_begin();
        }
      }
    }
    if (fast_forward){
      frameskip_count=20;
    }else{
      int fs=frameskip;
      if (fs==AUTO_FRAMESKIP && VSyncing) fs=1;
      frameskip_count=fs;
      if (fs==AUTO_FRAMESKIP){
        auto_frameskip_target_time=timer+((run_speed_ticks_per_second+(shifter_freq/2))/shifter_freq);
        speed_limit_wait_till=auto_frameskip_target_time;
      }else{
        speed_limit_wait_till=timer+((fs*(run_speed_ticks_per_second+(shifter_freq/2)))/shifter_freq);
        log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Calculating speed_limit_wait_till at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));
        log_to(LOGSECTION_SPEEDLIMIT,Str("      frameskip=")+frameskip+" shifter_freq="+shifter_freq);
      }
    }
  }
  if (fast_forward){
    speed_limit_wait_till=timer+(fast_forward_max_speed/shifter_freq);
  }
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: speed_limit_wait_till is ")+(speed_limit_wait_till-run_start_time));

  // The MFP clock aligns with the CPU clock every 8000 CPU cycles
  while (abs(ABSOLUTE_CPU_TIME-cpu_time_of_first_mfp_tick)>160000){
    cpu_time_of_first_mfp_tick+=160000;
  }
  while (abs(ABSOLUTE_CPU_TIME-shifter_cycle_base)>160000){
    shifter_cycle_base+=60000;
  }

  shifter_pixel=shifter_hscroll;
  overscan_add_extra=0;
  left_border=BORDER_SIDE;right_border=BORDER_SIDE;
  if (shifter_hscroll_extra_fetch && shifter_hscroll==0) overscan=OVERSCAN_MAX_COUNTDOWN;

  scanline_drawn_so_far=0;
  shifter_first_draw_line=0;
  shifter_last_draw_line=shifter_y;
  if (emudetect_falcon_mode && emudetect_falcon_extra_height){
    shifter_first_draw_line=-20;
    shifter_last_draw_line=320;
    overscan=OVERSCAN_MAX_COUNTDOWN;
  }

#if !(defined(STEVEN_SEAGAL) && defined(SS_INT_RELOAD_SDP_TIMING))
  event_start_vbl(); // Reset SDP again! //SS check
#endif

#if defined(SS_SHIFTER_DRAGON)//temp
  if(SS_signal==SS_SIGNAL_SHIFTER_CONFUSED_1)
    SS_signal=SS_SIGNAL_SHIFTER_CONFUSED_2; // stage 2 of our hack
#endif  
  shifter_freq_at_start_of_vbl=shifter_freq;
  scanline_time_in_cpu_cycles_at_start_of_vbl=scanline_time_in_cpu_cycles[shifter_freq_idx];
  CALC_CYCLES_FROM_HBL_TO_TIMER_B(shifter_freq);
// SS: this was so in the source
  cpu_time_of_last_vbl=time_of_next_event; ///// ABSOLUTE_CPU_TIME;
//  cpu_time_of_last_vbl=ABSOLUTE_CPU_TIME;
// /////  cpu_time_of_next_hbl_interrupt=cpu_time_of_last_vbl+cycles_for_vertical_return[shifter_freq_idx]+
// /////                                 CPU_CYCLES_FROM_LINE_RETURN_TO_HBL_INTERRUPT;
//  cpu_time_of_next_hbl_interrupt=cpu_time_of_last_vbl; ///// HBL happens immediately after VBL

  screen_event_pointer++;
  if (screen_event_pointer->event==NULL){
    cpu_time_of_start_of_event_plan=cpu_time_of_last_vbl;
#if defined(STEVEN_SEAGAL) && defined(SS_MFP_RATIO)
    if (n_cpu_cycles_per_second>CpuNormalHz){ // in fact should leave as is?
#else
    if (n_cpu_cycles_per_second>8000000){
#endif
      screen_event_pointer=event_plan_boosted[shifter_freq_idx];
    }else{
      screen_event_pointer=event_plan[shifter_freq_idx]; // SS 0 1 2
    }
  }

#if defined(STEVEN_SEAGAL) && defined(SS_INT_JITTER)
    VblJitterIndex++; // like Hatari
    if(VblJitterIndex==5)
      VblJitterIndex=0;
#endif

  log_to(LOGSECTION_SPEEDLIMIT,"--");

  PasteVBL();
  ONEGAME_ONLY( OGVBL(); )

#ifdef DEBUG_BUILD
  if (debug_run_until==DRU_VBL || (debug_run_until==DRU_SCANLINE && debug_run_until_val==scan_y)){
    if (runstate==RUNSTATE_RUNNING) runstate=RUNSTATE_STOPPING;
  }
  debug_vbl();
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_SHIFTER)
  Shifter.Vbl();
#endif
}
//---------------------------------------------------------------------------
void prepare_cpu_boosted_event_plans()
{
  n_millions_cycles_per_sec=n_cpu_cycles_per_second/1000000;

  screen_event_struct *source,*dest;
  int factor=n_millions_cycles_per_sec;
  for (int idx=0;idx<3;idx++){ //3 frequencies
    source=event_plan[idx];
    dest=event_plan_boosted[idx];
    for (;;){
      dest->time=(source->time * factor)/8;
      dest->event=source->event;
      if (source->event==NULL) break;
      source++;dest++;
    }
    scanline_time_in_cpu_cycles[idx]=(scanline_time_in_cpu_cycles_8mhz[idx]*factor)/8;
  }
  for (int n=0;n<16;n++){
    mfp_timer_prescale[n]=min((mfp_timer_8mhz_prescale[n]*factor)/8,1000);
  }
//  init_timings();
  mfp_init_timers();
  if (runstate==RUNSTATE_RUNNING) prepare_event_again();
  CheckResetDisplay();
}
//---------------------------------------------------------------------------
#if USE_PASTI
void event_pasti_update()
{
  if (hPasti==NULL || pasti_active==false
#if defined(STEVEN_SEAGAL) && defined(SS_PASTI_ONLY_STX)
    || PASTI_JUST_STX && SF314[floppy_current_drive()].ImageType!=3
#endif
    ){
#if defined(STEVEN_SEAGAL) && defined(SS_MFP_RATIO)
    pasti_update_time=ABSOLUTE_CPU_TIME+CpuNormalHz;
#else
    pasti_update_time=ABSOLUTE_CPU_TIME+8000000;
#endif
    return;
  }

//SS pasti.dll tells us when we must call it again through the variable
// pastiIOINFO->updateCycles as read in pasti_handle_return. The sync is
// ensured by the plugin

  struct pastiIOINFO pioi;
  pioi.stPC=pc;
  pioi.cycles=ABSOLUTE_CPU_TIME;
//  log_to(LOGSECTION_PASTI,Str("PASTI: Update pc=$")+HEXSl(pc,6)+" cycles="+pioi.cycles);
  pasti->Io(PASTI_IOUPD,&pioi);
  pasti_handle_return(&pioi);
}
#endif
//---------------------------------------------------------------------------
#if defined(STEVEN_SEAGAL) // added events

#if defined(SS_ACIA_IRQ_DELAY) 
/*  Imitating a Hatari feature, we implement a short delay between the time
    a byte has been transferred from the ACIA's shift register to its RDR,
    and the time IRQ is set in the SR.
    We add on the "event" system of Steem, which gives cycle accuracy.
    This feature doesn't depend on HD6301 true emu.
    It fixes V8 Music System.
*/

#define LOGSECTION LOGSECTION_IKBD

void event_acia_rx_irq() { 
  int lcycles=ABSOLUTE_CPU_TIME-ikbd.timer_when_keyboard_info;
  ASSERT( ACIA_IKBD.rx_stage );

  // RDRS->RDR
  if(ACIA_IKBD.rx_stage==1) 
  { 
    ASSERT(lcycles>=SS_6301_TO_ACIA_IN_CYCLES);

#if defined(STEVEN_SEAGAL) && defined(SS_ACIA_DOUBLE_BUFFER_RX)//TX?!
    // here we should take care of next byte but we have another
    // problem anyway (txinterrupts)
    ASSERT(ACIA_IKBD.LineRxBusy)
    if(ACIA_IKBD.LineRxBusy)
    {
      ACIA_IKBD.LineRxBusy=false;
#if defined(SS_IKBD_6301) 
      if(HD6301EMU_ON)
        hd6301_completed_transmission_to_MC6850=true;
#endif
    }
#endif

    ASSERT( !ikbd.send_nothing );
    ASSERT( keyboard_buffer_length );

    // run-time safety
    if(ikbd.send_nothing || !keyboard_buffer_length) 
    {
      ACIA_IKBD.rx_stage=0; // avoid infinite loop (hanging)
      return; 
    }
    else
    {
      keyboard_buffer_length--;
      ASSERT( keyboard_buffer_length>=0 );
      if(!HD6301EMU_ON)
      {
        if(ikbd.joy_packet_pos>=keyboard_buffer_length) 
          ikbd.joy_packet_pos=-1;
        if(ikbd.mouse_packet_pos>=keyboard_buffer_length) 
          ikbd.mouse_packet_pos=-1;
      }

#if defined(SS_ACIA_TEST_REGISTERS)
      ASSERT( !(!(ACIA_IKBD.SR&BIT_0)^!(ACIA_IKBD.rx_not_read)) );
#endif

#if defined(SS_ACIA_REGISTERS)
      ACIA_IKBD.RDR=ACIA_IKBD.RDRS;
#endif
#if !defined(SS_ACIA_USE_REGISTERS) || defined(SS_ACIA_TEST_REGISTERS)
      ACIA_IKBD.data=keyboard_buffer[keyboard_buffer_length];
#endif
#if defined(SS_ACIA_TEST_REGISTERS)
      ASSERT( ACIA_IKBD.RDR==ACIA_IKBD.data );
#endif
      TRACE_LOG("ACIA RDRS->RDR %X (%d cycles)\n",keyboard_buffer[keyboard_buffer_length],lcycles);

/*  Check overrun.
    The overrun isn't set before the data is read, Steem was right on this.
    But it seems previous byte is lost, not new one (TESTING), see above,
    it was already copied.
*/
#if defined(SS_ACIA_USE_REGISTERS)
      if(ACIA_IKBD.SR&BIT_0) // RDR full
#else
      if(ACIA_IKBD.rx_not_read)
#endif
      {
        if(ACIA_IKBD.overrun!=ACIA_OVERRUN_YES) 
        {
          TRACE_LOG("ACIA IKBD OVR\n");
          ACIA_IKBD.overrun=ACIA_OVERRUN_COMING;
#if defined(SS_ACIA_REGISTERS)
          ACIA_IKBD.SR|=BIT_5;
#endif
        }
      }
      else // not overrun, OK
      {
#if !defined(SS_ACIA_USE_REGISTERS) || defined(SS_ACIA_TEST_REGISTERS)
        ACIA_IKBD.rx_not_read=true;
#endif
#if defined(SS_ACIA_REGISTERS)
        ACIA_IKBD.SR|=BIT_0; // RDR full
        ACIA_IKBD.SR&=~BIT_5; // no overrun
#endif
      }


/*  Check IRQ
*/
#if defined(SS_ACIA_TEST_REGISTERS)
//      ASSERT( !(!(ACIA_IKBD.CR&BIT_7)^!(ACIA_IKBD.rx_irq_enabled)) );
      if(!(ACIA_IKBD.CR&BIT_7)^!(ACIA_IKBD.rx_irq_enabled))
        TRACE_LOG("ACIA IKBD IRQ RX enabled difference: CR %X rx_irq_enabled %d\n",ACIA_IKBD.CR,ACIA_IKBD.rx_irq_enabled);
#endif
#if defined(SS_ACIA_USE_REGISTERS)
      if(ACIA_IKBD.CR&BIT_7)
#else
      if(ACIA_IKBD.rx_irq_enabled)
#endif
        ACIA_IKBD.rx_stage++; // 2: irq can trigger
      else
        ACIA_IKBD.rx_stage=0; // we're done
      
      // More to process?
      if(keyboard_buffer_length) 
      {
        agenda_add(agenda_keyboard_replace,SS_6301_TO_ACIA_IN_HBL,0);
#if defined(SS_ACIA_USE_REGISTERS)
        ACIA_IKBD.RDRS=keyboard_buffer[keyboard_buffer_length-1];
#endif
      }
      if(macro_start_after_ikbd_read_count) 
        macro_start_after_ikbd_read_count--;
    }
  }
  // set IRQ
  if(ACIA_IKBD.rx_stage==2 && lcycles
    >=SS_6301_TO_ACIA_IN_CYCLES+SS_ACIA_IRQ_DELAY_CYCLES) 
  {
#if defined(SS_ACIA_TEST_REGISTERS)
    ASSERT( ACIA_IKBD.CR&BIT_7 );
    ASSERT( ACIA_IKBD.rx_irq_enabled );
#endif
#if defined(SS_ACIA_REGISTERS)
    if(ACIA_IKBD.CR&BIT_7)
      ACIA_IKBD.SR|=BIT_7;
#endif
#if !defined(SS_ACIA_USE_REGISTERS) || defined(SS_ACIA_TEST_REGISTERS)
    ACIA_IKBD.irq=true;
#endif
    TRACE_LOG("ACIA IRQ (RDR) (%d cycles)\n",lcycles);
    mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,0);
    ACIA_IKBD.rx_stage=0;
  }
}
#undef LOGSECTION
#endif

#if defined(SS_INT_VBI_START) // normally not used

void event_trigger_vbi() { 
  if ((sr & SR_IPL)<SR_IPL_4)// && vbl_pending)
  {
    ASSERT( !Blit.HasBus );
    ASSERT( !vbl_pending );
    vbl_pending=true;
    VBL_INTERRUPT
  }
  screen_event_pointer++;
}
#endif
#endif