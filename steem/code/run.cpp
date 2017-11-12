/*---------------------------------------------------------------------------
FILE: run.cpp
MODULE: emu
DESCRIPTION: This file contains Steem's run() function, the routine that
actually makes Steem go. Also included here is the code for the event system
that allows time-critical Steem functions (such as VBLs, HBLs and MFP timers)
to be scheduled to the nearest cycle. Speed limiting and drawing is also
handled here, in event_scanline and event_vbl_interrupt.
---------------------------------------------------------------------------*/

#if defined(SSE_COMPILER_INCLUDED_CPP)
#pragma message("Included for compilation: run.cpp")
#endif

#if defined(SSE_BUILD)

#define EXT
#define INIT(s) =s
#if defined(SSE_VAR_RESIZE)
EXT BYTE runstate;
#else
EXT int runstate;
#endif
// fast_forward_max_speed=(1000 / (max %/100)); 0 for unlimited
EXT int fast_forward INIT(0),fast_forward_max_speed INIT(0);
EXT bool fast_forward_stuck_down INIT(0);
EXT int slow_motion INIT(0),slow_motion_speed INIT(100);
#if defined(SSE_NO_RUN_SPEED)
const EXT int run_speed_ticks_per_second INIT(1000);
#else
EXT int run_speed_ticks_per_second INIT(1000);
#endif
EXT bool disable_speed_limiting INIT(0);
LOG_ONLY( EXT int run_start_time; )
UNIX_ONLY( EXT bool RunWhenStop INIT(0); )

EXT DWORD avg_frame_time INIT(0),avg_frame_time_timer,frame_delay_timeout,timer;
EXT DWORD speed_limit_wait_till;
EXT int avg_frame_time_counter INIT(0);
EXT DWORD auto_frameskip_target_time;

#if defined(SSE_VID_ENFORCE_AUTOFRAMESKIP) && !defined(SSE_BUGFIX_393)
EXT const BYTE frameskip INIT(AUTO_FRAMESKIP);
EXT BYTE frameskip_count INIT(1);
#elif defined(SSE_VAR_RESIZE) && !defined(SSE_BUGFIX_393) // breaks Slow Motion
EXT BYTE frameskip INIT(AUTO_FRAMESKIP);
EXT BYTE frameskip_count INIT(1);
#else
EXT int frameskip INIT(AUTO_FRAMESKIP);
EXT int frameskip_count INIT(1);
#endif


EXT bool flashlight_flag INIT(false);

DEBUG_ONLY(EXT int mode);
#if defined(SSE_VAR_RESIZE)
EXT BYTE mixed_output INIT(0);
#else
EXT int mixed_output INIT(0);
#endif


#if defined(SSE_TIMINGS_CPUTIMER64)
EXT COUNTER_VAR cpu_time_of_last_vbl,shifter_cycle_base;
EXT COUNTER_VAR cpu_timer_at_start_of_hbl;
#else
EXT int cpu_time_of_last_vbl,shifter_cycle_base;
EXT int cpu_timer_at_start_of_hbl;
#endif

#if defined(SSE_GLUE)
// no more plans!
#else
screen_event_struct event_plan_50hz[313*2+2],event_plan_60hz[263*2+2],event_plan_70hz[600*2+2],
                    event_plan_boosted_50hz[313*2+2],event_plan_boosted_60hz[263*2+2],event_plan_boosted_70hz[600*2+2];
screen_event_struct*screen_event_pointer,*event_plan[4],*event_plan_boosted[4];
#endif
EVENTPROC event_mfp_timer_timeout[4]={event_timer_a_timeout,event_timer_b_timeout,
                          event_timer_c_timeout,event_timer_d_timeout};

#if defined(SSE_TIMINGS_CPUTIMER64)
COUNTER_VAR time_of_next_event;
COUNTER_VAR cpu_time_of_start_of_event_plan;
COUNTER_VAR time_of_next_timer_b=0;
COUNTER_VAR time_of_last_hbl_interrupt;
#if defined(SSE_INT_VBL_IACK)
COUNTER_VAR time_of_last_vbl_interrupt;
#endif
COUNTER_VAR cpu_timer_at_res_change;
#else
int time_of_next_event;
int cpu_time_of_start_of_event_plan;
int time_of_next_timer_b=0;
int time_of_last_hbl_interrupt;
#if defined(SSE_INT_VBL_IACK)
int time_of_last_vbl_interrupt;
#endif
int cpu_timer_at_res_change;
#endif

EVENTPROC screen_event_vector;


//int cpu_time_of_next_hbl_interrupt=0;



#if defined(SSE_VAR_RESIZE)
BYTE screen_res_at_start_of_vbl;
BYTE shifter_freq_at_start_of_vbl;
#else
int screen_res_at_start_of_vbl;
int shifter_freq_at_start_of_vbl;
#endif
int scanline_time_in_cpu_cycles_at_start_of_vbl;
bool hbl_pending;



#if USE_PASTI
void event_pasti_update();
#endif

#undef EXT
#undef INIT

#include "SSE/SSEDebug.h"
#include "SSE/SSEFloppy.h"
#include "SSE/SSEGlue.h"

#endif//decla


#ifdef SHOW_DRAW_SPEED
extern HWND  StemWin;
#endif

//---------------------------------------------------------------------------
void exception(int exn,exception_action ea,MEM_ADDRESS a)
{
  io_word_access=0;
  ioaccess=0;
  ExceptionObject.init(exn,ea,a);
#if defined(SSE_M68K_EXCEPTION_TRY_CATCH)
  throw &ExceptionObject;
#else
  if (pJmpBuf==NULL){
    log_write(Str("Unhandled exception! pc=")+HEXSl(old_pc,6)+" action="+int(ea)+" address involved="+HEXSl(a,6));
    BRK( Unhandled exception! ); //emulator crash on bad snapshot etc.
    return;
  }
  longjmp(*pJmpBuf,1);
#endif
}
//---------------------------------------------------------------------------
void run()
{
  //TODO don't restart if blit error, init DX should change state
#if defined(SSE_CPU_HALT)
  if(M68000.ProcessingState==TM68000::HALTED
#if defined(SSE_IKBD_6301) && !defined(SSE_BUGFIX_394)
    || HD6301.Crashed
#endif
    )
    return; // cancel "run" until reset
#if defined(SSE_GUI_STATUS_BAR_ALERT) && !defined(SSE_BUGFIX_394)
  else if(M68000.ProcessingState==TM68000::BLIT_ERROR
#ifdef SSE_BOILER
    ||M68000.ProcessingState==TM68000::BOILER_MESSAGE
#endif
    )
  {
    M68000.ProcessingState=TM68000::NORMAL;
    HWND status_bar_win=GetDlgItem(StemWin,120); // get handle
    InvalidateRect(status_bar_win,NULL,false);
  }
#endif
#endif

#if defined(SSE_IKBD_6301) && defined(SSE_BUGFIX_394)
  HD6301.Crashed=mousek=0;
  //after BLIT or HD6301 or whatever state/message
  M68000.ProcessingState=TM68000::NORMAL;
  HWND status_bar_win=GetDlgItem(StemWin,120); // get handle
  InvalidateRect(status_bar_win,NULL,false);
#endif

  bool ExcepHappened;

  Disp.RunStart();
#if !defined(SSE_YM2149_DISABLE_CAPTURE_FILE)
  if (psg_always_capture_on_start) psg_capture(true,"test.stym");
#endif
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

#if defined(SSE_GLUE) && defined(SSE_SHIFTER_TRICKS)
  Glue.AddFreqChange(shifter_freq);
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

#if defined(SSE_DEBUG_START_STOP_INFO)
  Debug.TraceGeneralInfos(TDebug::START);
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
#pragma warning(disable: 4611) //390
    TRY_M68K_EXCEPTION
#pragma warning(default: 4611)
      while (runstate==RUNSTATE_RUNNING){
        // cpu_cycles is the amount of cycles before next event.
        // SS It is *decremented* by instruction timings, not incremented.
        while (cpu_cycles>0 && runstate==RUNSTATE_RUNNING){
#if defined(SSE_BOILER_HISTORY_TIMING)
          pc_history_y[pc_history_idx]=scan_y;
          pc_history_c[pc_history_idx]=LINECYCLES;
#endif
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
#if defined(SSE_BOILER_TRACE_CONTROL)
          if(TRACE_MASK2&TRACE_CONTROL_EVENT)
            TRACE_EVENT(screen_event_vector);        
#endif
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
#pragma warning(disable: 4611) //390
        TRY_M68K_EXCEPTION
#pragma warning(default: 4611)
#ifndef SSE_BOILER_394
          WORD a=m68k_dpeek(LPEEK(e.bombs*4));
#endif
          if (e.bombs>8){
            alertflag=false;
#if defined(SSE_BOILER_EXCEPTION_NOT_TOS)
          }else if (crash_notification==CRASH_NOTIFICATION_NOT_TOS 
            && e._pc>=rom_addr && e._pc<rom_addr+tos_len){
            alertflag=false;
#endif
          }else if (crash_notification==CRASH_NOTIFICATION_BOMBS_DISPLAYED &&
#ifdef SSE_BOILER_394
            LPEEK(e.bombs*4)<rom_addr){ // 'bombs' handler is in rom
#else
                   a!=0x6102 && a!=0x4eb9 ){ //not bombs routine
#endif
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
#ifdef SSE_SOUND_16BIT_CENTRED
  Sound_Stop();
#else
  Sound_Stop(Quitting);
#endif

#if defined(SSE_VID_FS_382)
  if(FullScreen)
#endif
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

#if defined(SSE_DEBUG_START_STOP_INFO)
  Debug.TraceGeneralInfos(TDebug::STOP);
#endif
#if defined(SSE_BOILER_AUTO_FLUSH_TRACE)
  if(Debug.trace_file_pointer)
    fflush(Debug.trace_file_pointer);
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
#if !defined(SSE_YM2149_DISABLE_CAPTURE_FILE)
  psg_capture(0,"");
#endif
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
#if defined(SSE_GLUE)
  Glue.GetNextScreenEvent();
#else
  time_of_next_event=cpu_time_of_start_of_event_plan+(screen_event_pointer->time);
  screen_event_vector=(screen_event_pointer->event); // SS pointer to function
#endif
  //  end of new 3/7/2001

    //  PREPARE_EVENT_CHECK_FOR_DMA_SOUND_END
    //check timers for timeouts
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(0);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(1);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(2);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(3);
    
    PREPARE_EVENT_CHECK_FOR_TIMER_B;

#if defined(SSE_INT_MFP_LATCH_DELAY)
    PREPARE_EVENT_CHECK_FOR_MFP_WRITE;
#endif
    
    PREPARE_EVENT_CHECK_FOR_DEBUG;
    
    PREPARE_EVENT_CHECK_FOR_PASTI;

#if defined(SSE_WD1772_EMU)
    PREPARE_EVENT_CHECK_FOR_FLOPPY;
#endif

#if defined(SSE_ACIA)
    PREPARE_EVENT_CHECK_FOR_ACIA;
#endif

  // cpu_timer must always be set to the next 4 cycle boundary after time_of_next_event
  //SS: this is still true after rounding refactoring
  //guess (!) it's because it enforces CPU R/W cycle, which is still 4 cycles
  //(clocks) in our reckoning, but still don't see how exactly

  int oo=time_of_next_event-cpu_timer;
  oo=(oo+3) & -4;
//  log_write(EasyStr("prepare event again: offset=")+oo);
  cpu_cycles+=oo;cpu_timer+=oo;
}

#if defined(SSE_COMPILER_380)
void prepare_next_event() //VC6 would inline
#else
void inline prepare_next_event() //SS check this "inline" thing
#endif
{

#if defined(SSE_GLUE)
  Glue.GetNextScreenEvent();
#else
  time_of_next_event=cpu_time_of_start_of_event_plan + screen_event_pointer->time;
  screen_event_vector=(screen_event_pointer->event); // SS pointer to function
#endif

    //  PREPARE_EVENT_CHECK_FOR_DMA_SOUND_END

    // check timers for timeouts
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(0);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(1);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(2);
    PREPARE_EVENT_CHECK_FOR_TIMER_TIMEOUTS(3);

    PREPARE_EVENT_CHECK_FOR_TIMER_B;

#if defined(SSE_INT_MFP_LATCH_DELAY)
    PREPARE_EVENT_CHECK_FOR_MFP_WRITE;
#endif
      
    PREPARE_EVENT_CHECK_FOR_DEBUG;
      
    PREPARE_EVENT_CHECK_FOR_PASTI;

#if defined(SSE_WD1772_EMU)
    PREPARE_EVENT_CHECK_FOR_FLOPPY;
#endif

#if defined(SSE_ACIA)
    PREPARE_EVENT_CHECK_FOR_ACIA;
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

#if defined(SSE_INT_MFP_INLINE)

inline void handle_timeout(int tn) {

  dbg_log(Str("MFP: Timer ")+char('A'+tn)+" timeout at "+ABSOLUTE_CPU_TIME+" timeout was "+mfp_timer_timeout[tn]+
    " period was "+mfp_timer_period[tn]);

  if (mfp_timer_period_change[tn]){    
    // Audio Artistic, timer D would count through before the write
#if defined(SSE_INT_MFP_LATCH_DELAY)
    if(MC68901.WritePending)
    {
      TRACE_MFP("Handle time-out Flush MFP event ");
      event_mfp_write(); //flush
    }
#endif
    MFP_CALC_TIMER_PERIOD(tn);          
    mfp_timer_period_change[tn]=0;       
  }
  COUNTER_VAR stage=(mfp_timer_timeout[tn]-ABSOLUTE_CPU_TIME); 

#if defined(SSE_INT_MFP_TIMERS_WOBBLE)
  if(OPTION_C2)
    stage-=MC68901.Wobble[tn]; //get the correct timing (no drift)
#endif

  if (stage<=0){                                       
    stage+=((-stage/mfp_timer_period[tn])+1)*mfp_timer_period[tn]; 
  }else{ 
    stage%=mfp_timer_period[tn]; 
  }

  COUNTER_VAR new_timeout=ABSOLUTE_CPU_TIME+stage; 

  if(OPTION_C2)
  {
#if defined(SSE_INT_MFP_RATIO_PRECISION)
    mfp_timer_period_current_fraction[tn]+=mfp_timer_period_fraction[tn]; 
    // this guarantees that we're always at the right cycle, despite
    // the inconvenience of a ratio
    if(mfp_timer_period_current_fraction[tn]>=1000) {
      mfp_timer_period_current_fraction[tn]-=1000;
      new_timeout+=1; 
    }
#endif
#if defined(SSE_INT_MFP_TIMERS_WOBBLE_390)
    new_timeout+=MC68901.Wobble[tn]=(rand() % MFP_TIMERS_WOBBLE);
#elif defined(SSE_INT_MFP_TIMERS_WOBBLE)
    new_timeout+=MC68901.Wobble[tn]=rand()&MFP_TIMERS_WOBBLE;
#endif
#if defined(SSE_INT_MFP_PRESCALE)
    MC68901.Counter[tn]=mfp_reg[MFPR_TADR+tn]; // load counter
    BYTE prescale_index=(mfp_get_timer_control_register(tn)&7);
    MC68901.Prescale[tn]=mfp_timer_prescale[prescale_index]; // load prescale (bad if 0)
#endif
  }//C2
  mfp_interrupt_pend(mfp_timer_irq[tn],mfp_timer_timeout[tn]);
  mfp_timer_timeout[tn]=new_timeout;
}

#define HANDLE_TIMEOUT(tn) handle_timeout(tn)

#else 

#define HANDLE_TIMEOUT(tn) \
  dbg_log(Str("MFP: Timer ")+char('A'+tn)+" timeout at "+ABSOLUTE_CPU_TIME+" timeout was "+mfp_timer_timeout[tn]+ \
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

#endif//?inline

void event_timer_a_timeout()
{
  HANDLE_TIMEOUT(0);
#if !defined(SSE_INT_MFP_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_A,mfp_timer_timeout[0]);
  mfp_timer_timeout[0]=new_timeout;
#endif
}
void event_timer_b_timeout()
{
  HANDLE_TIMEOUT(1);
#if !defined(SSE_INT_MFP_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_B,mfp_timer_timeout[1]);
  mfp_timer_timeout[1]=new_timeout;
#endif
}
void event_timer_c_timeout()
{
  HANDLE_TIMEOUT(2);
#if !defined(SSE_INT_MFP_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_C,mfp_timer_timeout[2]);
  mfp_timer_timeout[2]=new_timeout;
#endif
}
void event_timer_d_timeout()
{
  HANDLE_TIMEOUT(3);
#if !defined(SSE_INT_MFP_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_D,mfp_timer_timeout[3]);
  mfp_timer_timeout[3]=new_timeout;
#endif
}
#undef LOGSECTION
//---------------------------------------------------------------------------
#define LOGSECTION LOGSECTION_INTERRUPTS
void event_timer_b() 
{
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_TIMER_B)
    FrameEvents.Add(scan_y,LINECYCLES,"TB",ACT-time_of_next_timer_b);
#endif
  if (scan_y<shifter_first_draw_line){
#if defined(SSE_INT_MFP_TIMER_B_392)
    if(!OPTION_C2)
#endif
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;
  }else if (scan_y<shifter_last_draw_line){
    if (mfp_reg[MFPR_TBCR]==8){
      // There is a problem that this draw_check_border_removal() can happen before
      // event_scanline but after a change to mono for left border removal, this
      // stops the border opening on the next line somehow.
      mfp_timer_counter[1]-=64;
#if defined(SSE_INT_MFP_PRESCALE)
      MC68901.Counter[1]--;
#endif
      log_to(LOGSECTION_MFP_TIMERS,EasyStr("MFP: Timer B counter decreased to ")+(mfp_timer_counter[1]/64)+" at "+scanline_cycle_log());
      if (mfp_timer_counter[1]<64){
        dbg_log(EasyStr("MFP: Timer B timeout at ")+scanline_cycle_log());
#ifdef SSE_DEBUG
        if (mfp_interrupt_enabled[8]) TRACE_LOG("F%d y%d c%d Timer B pending\n",TIMING_INFO); //?
#endif
        mfp_timer_counter[1]=BYTE_00_TO_256(mfp_reg[MFPR_TBDR])*64;
#if defined(SSE_INT_MFP_PRESCALE)
        MC68901.Counter[1]=mfp_reg[MFPR_TBDR];
#endif
        mfp_interrupt_pend(MFP_INT_TIMER_B,time_of_next_timer_b);
      }
    }
#if defined(SSE_INT_MFP_TIMER_B_392)
    if(!OPTION_C2)
#endif
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+cpu_cycles_from_hbl_to_timer_b+
        scanline_time_in_cpu_cycles_at_start_of_vbl + TB_TIME_WOBBLE;
  }else{
#if defined(SSE_INT_MFP_TIMER_B_392)
    if(!OPTION_C2)
#endif
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;
  }
#if defined(SSE_INT_MFP_TIMER_B_392)
  if(OPTION_C2)
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;  //put into future
#endif

}
#undef LOGSECTION
//---------------------------------------------------------------------------
#if !defined(SSE_GLUE)

void event_hbl()   //just HBL, don't draw yet
{
#define LOGSECTION LOGSECTION_AGENDA
  CHECK_AGENDA
#undef LOGSECTION

  log_to_section(LOGSECTION_VIDEO,EasyStr("VIDEO: Event HBL at end of line ")+scan_y+", cycle "+(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl));
  right_border_changed=0;
  scanline_drawn_so_far=0;
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  cpu_timer_at_start_of_hbl=time_of_next_event;
  scan_y++;

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
#if defined(SSE_IKBD_6301)
  // we run some 6301 cycles at the end of each scanline (x1)
  if(OPTION_C1 && !HD6301.Crashed)
  {
    ASSERT(HD6301_OK);
    hd6301_run_cycles(ACT);
    if(HD6301.Crashed)
    {
      TRACE("6301 CRASH\n");
#if defined(SSE_GUI_STATUS_BAR_392)
      GUIRefreshStatusBar();
#endif
      runstate=RUNSTATE_STOPPING;
    }
  }
#endif//6301

#if defined(SSE_DISK_CAPS)
  if(Caps.Active==1) 
    Caps.Hbl(); 
#endif
}
#endif
//---------------------------------------------------------------------------

#if defined(SSE_GLUE)
/*  We take some tasks out of event_scanline(), so we can execute them from
    event_vbl_interrupt().
*/

void event_scanline_sub() {
#define LOGSECTION LOGSECTION_AGENDA
  CHECK_AGENDA;
#undef LOGSECTION
#if defined(SSE_IKBD_6301)
  if(OPTION_C1)
  {
    ASSERT(HD6301_OK);

    hd6301_run_cycles(ACT);
    if(HD6301.Crashed)
    {
      TRACE("6301 CRASH\n");
#if defined(SSE_GUI_STATUS_BAR_392)
      GUIRefreshStatusBar();
#endif
      runstate=RUNSTATE_STOPPING;
    }
  }
#endif

#if defined(SSE_DISK_CAPS)
  if(Caps.Active==1)
    Caps.Hbl();
#endif

  if (dma_sound_on_this_screen) 
#if defined(SSE_CARTRIDGE_BAT)// don't interfere with our hack
    if(!SSEConfig.mv16 
#if defined(SSE_DONGLE_PROSOUND)
      && !(DONGLE_ID==TDongle::PROSOUND)
#endif
      ) 
#endif
      dma_sound_fetch(); 
}

#endif

void event_scanline()
{
#if defined(SSE_GLUE)

  event_scanline_sub();

#else

#define LOGSECTION LOGSECTION_AGENDA
  CHECK_AGENDA;
#undef LOGSECTION

#if defined(SSE_IKBD_6301)
  if(OPTION_C1)
  {
    ASSERT(HD6301_OK);

    hd6301_run_cycles(ACT);
    if(HD6301.Crashed)
    {
      TRACE("6301 CRASH\n");
#if defined(SSE_GUI_STATUS_BAR_392)
      GUIRefreshStatusBar();
#endif
      runstate=RUNSTATE_STOPPING;
    }
  }
#endif

#endif

#if defined(SSE_BUILD)
/*  Note: refactoring here is very dangerous!
    We must separate SSE_SHIFTER from SSE_INTERRUPT, which makes for many
    #if blocks.
*/
  if (scan_y<shifter_first_draw_line-1){
    if (scan_y>=draw_first_scanline_for_border){
      if (bad_drawing==0) 
#if defined(SSE_SHIFTER)
        Shifter.DrawScanlineToEnd();
#else
        draw_scanline_to_end();
#endif
#if defined(SSE_INT_MFP_TIMER_B_392)
      if(!OPTION_C2)
#endif
      time_of_next_timer_b=time_of_next_event+160000;  //put into future
    }
  }else if (scan_y<shifter_first_draw_line){ //next line is first visible
    if (bad_drawing==0) 
#if defined(SSE_SHIFTER)
      Shifter.DrawScanlineToEnd();
#else
      draw_scanline_to_end();
#endif
#if defined(SSE_INT_MFP_TIMER_B_392)
    if(!OPTION_C2)
#endif
    time_of_next_timer_b=time_of_next_event
      +cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
  }else if (scan_y<shifter_last_draw_line-1){
    if (bad_drawing==0) 
#if defined(SSE_SHIFTER)
      Shifter.DrawScanlineToEnd();
#else
      draw_scanline_to_end();
#endif
#if defined(SSE_INT_MFP_TIMER_B_392)
    if(!OPTION_C2)
#endif
    time_of_next_timer_b=time_of_next_event
      +cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
  }else if (scan_y<draw_last_scanline_for_border){
    if (bad_drawing==0) 
#if defined(SSE_SHIFTER)
      Shifter.DrawScanlineToEnd();
#else
      draw_scanline_to_end();
#endif
#if defined(SSE_INT_MFP_TIMER_B_392)
    if(!OPTION_C2)
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
  
#if defined(SSE_GLUE)
  if(Glue.FetchingLine())
    Glue.EndHBL(); // check for +2 -2 errors + unstable Shifter

  if((scan_y==-30||scan_y==shifter_last_draw_line-1&&scan_y<245)
    &&Glue.CurrentScanline.Cycles>224)

    Glue.CheckVerticalOverscan(); // check top & bottom borders
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
#if defined(SSE_SHIFTER_TRICKS)
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
#if !defined(SSE_VIDEO_CHIPSET) 
  right_border_changed=0;
#endif
  scanline_drawn_so_far=0;

#if defined(DEBUG_BUILD)
/*  Enforce register limitations, so that "report SDP" isn't messed up
    in the debug build.
*/
#if defined(SSE_MMU_MONSTER_ALT_RAM)
  if(mem_len<14*0x100000) 
#else
  if(mem_len<=FOUR_MEGS) 
#endif
    shifter_draw_pointer&=0x3FFFFE;
#endif

#if defined(SSE_GLUE)
/*  Refactoring of "add extra".
    We don't use variable overscan_add_extra to adjust the video counter 
    anymore.
    Too many interferences with rendering when writing to GLU and MMU registers,
    and too many hacks due to border size.
    Instead, the video counter is recomputed every scanline, based on
    the counter at the start of the line (adapted or not because of a write)
    and #bytes recorded in CurrentScanline.Bytes (must be accurate now!).
*/
  if(!emudetect_falcon_mode && Glue.FetchingLine())
  {
#if 0 && defined(SSE_MMU)
    //looks nice but takes more CPU power (another CheckSideOverscan round)
    MMU.UpdateVideoCounter(LINECYCLES);
    shifter_draw_pointer=shifter_draw_pointer_at_start_of_line=MMU.VideoCounter;
#else
    short added_bytes=Glue.CurrentScanline.Bytes;
    // extra words for HSCROLL are included in Bytes
    if(ST_TYPE==STE && added_bytes)
      added_bytes+=LINEWID*2; 
    shifter_draw_pointer_at_start_of_line+=added_bytes;
    shifter_draw_pointer=shifter_draw_pointer_at_start_of_line;
#endif
  }
  else 
#endif
    shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
#if defined(SSE_MMU)
  MMU.VideoCounter=shifter_draw_pointer_at_start_of_line;
#endif
  /////// SS as defined in draw.cpp,
  /////// and relative to cpu_time_of_last_vbl:
  cpu_timer_at_start_of_hbl=time_of_next_event; 

#if !defined(SSE_GLUE)
#if defined(SSE_DISK_CAPS)
  if(Caps.Active==1)
    Caps.Hbl();
#endif
#endif

#if defined(SSE_GLUE)
  Glue.IncScanline(); // will call Shifter.IncScanline()
#else
  scan_y++;
#endif

#if defined(SSE_BOILER_FRAME_REPORT_MASK) && defined(SSE_VIDEO_CHIPSET)
  if(Glue.FetchingLine() && (FRAME_REPORT_MASK1&FRAME_REPORT_MASK_SDP_LINES)) 
  {
#if defined(SSE_BOILER_FRAME_REPORT_392)
    FrameEvents.Add(scan_y,0,"VC",shifter_draw_pointer); 
#else //why did I split?
    FrameEvents.Add(scan_y,0,'@',(shifter_draw_pointer&0x00FF0000)>>16 ); 
    FrameEvents.Add(scan_y,0,'@',(shifter_draw_pointer&0xFFFF) ); 
#endif
  }
#endif

#ifdef DEBUG_BUILD
  if (debug_run_until==DRU_SCANLINE){
    if (debug_run_until_val==scan_y){
      if (runstate==RUNSTATE_RUNNING) runstate=RUNSTATE_STOPPING;
#if defined(SSE_BOILER_GO_AUTOSAVE_FRAME)
      FrameEvents.Report(); // sick of clicking 'Go' then 'Save frame report'
#endif
    }
  }
#endif

#if defined(SSE_INT_HBL_IACK2)
  BYTE iack_latency=(OPTION_C1)
    ? HBL_IACK_LATENCY + M68000.LastEClockCycles[TM68000::ECLOCK_HBL]
    : CYCLES_FROM_START_OF_HBL_IRQ_TO_WHEN_PEND_IS_CLEARED;
#endif

  if (abs_quick(cpu_timer_at_start_of_hbl-time_of_last_hbl_interrupt)
#if defined(SSE_INT_HBL_IACK2)
    >iack_latency
#else
    >CYCLES_FROM_START_OF_HBL_IRQ_TO_WHEN_PEND_IS_CLEARED
#endif
    ){
    hbl_pending=true;
  }
#ifdef SSE_DEBUG
  else
  {
    TRACE_INT("%d %d %d (%d) no HBI, %d cycles into HBI IACK\n",TIMING_INFO,ACT,abs_quick(cpu_timer_at_start_of_hbl-time_of_last_hbl_interrupt));
#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK1 & OSD_CONTROL_IACK)
      TRACE_OSD("HBI %d IACK %d",scan_y,cpu_timer_at_start_of_hbl-time_of_last_hbl_interrupt);
#endif
  }
#endif
#if !defined(SSE_GLUE)
  if (dma_sound_on_this_screen) dma_sound_fetch(); 
#endif
#if !defined(SSE_GLUE)
  screen_event_pointer++;
#endif

#if defined(SSE_VID_DD_3BUFFER_WIN)
  // DirectDraw Check for Window VSync at each ST scanline!
  if(OPTION_3BUFFER_WIN && !FullScreen)
    Disp.BlitIfVBlank();
#endif

#if defined(SSE_GLUE)
  Glue.Status.hbi_done=false;
  ASSERT(!Glue.Status.scanline_done);
  Glue.Status.scanline_done=true;
  Glue.Status.sdp_reload_done=false;
#endif
}
//---------------------------------------------------------------------------
//#undef LOGSECTION LOGSECTION_INTERRUPTS
#define LOGSECTION LOGSECTION_INTERRUPTS//SS

void event_start_vbl()
{
#if defined(SSE_GLUE)
  Glue.Status.sdp_reload_done=true; // checked this line
#endif
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
    FrameEvents.Add(scan_y,LINECYCLES,'r',shifter_freq_idx); // "reload"
#endif

  // This happens about 60 cycles into scanline 247 (50Hz) //SS but the timing in frame event plan was different
  //TRACE_LOG("F%d L%d reload SDP (%X) <- %X\n",FRAME,scan_y,shifter_draw_pointer,xbios2);

#if defined(SSE_GLUE_393D)
  ASSERT(!Glue.vsync);
  Glue.vsync=true;
#endif
  // As soon as VSYNC is asserted, the MMU keeps on copying VBASE to VCOUNT
  // We don't emulate the continuous copy but we copy at start and stop
  // See event_trigger_vbi()
#if defined(SSE_MMU)
  MMU.VideoCounter=
#endif
  shifter_draw_pointer=xbios2; // SS: reload SDP
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  shifter_pixel=shifter_hscroll;
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
  overscan_add_extra=0;
#endif
  left_border=BORDER_SIDE;right_border=BORDER_SIDE;
#if defined(SSE_GLUE)
  Glue.Status.vbl_done=false;
#else
  screen_event_pointer++;
#endif
}

#undef LOGSECTION//3.8.0

//---------------------------------------------------------------------------
void event_vbl_interrupt() //SS misleading name?
{ 

#if defined(SSE_CARTRIDGE_FREEZE) && defined(SSE_DONGLE)
/*  When pressing some button of his cartridge, the player triggered an MFP
    interrupt.
    By releasing the button, the concerned bit should change state. We use
    no counter but do it at first VBL for simplicity.
*/
  if(cart)
  {
    switch(DONGLE_ID) {
#if defined(SSE_DONGLE_URC) 
    case TDongle::URC: 
      if(!(mfp_reg[MFPR_GPIP]&0x40))
        mfp_gpip_set_bit(MFP_GPIP_RING_BIT,true); // Ultimate Ripper
      break;
#endif
#if defined(SSE_DONGLE_MULTIFACE) // cart + monochrome
    case TDongle::MULTIFACE:
      if(!(mfp_reg[MFPR_GPIP]&0x80))
        mfp_gpip_set_bit(MFP_GPIP_MONO_BIT,true);
      break;
#endif
    }
  }//if
#endif

#if defined(SSE_GLUE)
/*  With GLU/video event refactoring, we call event_scanline() one time fewer,
    if we did now it would mess up some timings, so we call the sub
    with some HBL-dependent tasks: DMA sound, HD6301 & CAPS emu.
    Important for Relapse DMA sound
*/
  event_scanline_sub(); 
  ASSERT(Glue.VCount<Glue.nLines);
  Glue.VCount=0;
#endif

#if defined(SSE_VID_VSYNC_WINDOW)
  bool VSyncing=( (OPTION_WIN_VSYNC&&bAppActive||FSDoVsync&&FullScreen) 
    && fast_forward==0 && slow_motion==0);
#else
  bool VSyncing=(FSDoVsync && FullScreen && fast_forward==0 && slow_motion==0);
#endif

  bool BlitFrame=0;
#ifndef NO_CRAZY_MONITOR
  if (extended_monitor==0)
#endif
  { // Make sure whole screen is drawn (in 60Hz and 70Hz there aren't enough lines)
    while (scan_y<draw_last_scanline_for_border){
#if defined(SSE_SHIFTER)
      if(!bad_drawing) 
        Shifter.DrawScanlineToEnd();
#else
      if (bad_drawing==0) draw_scanline_to_end();
#endif
      scanline_drawn_so_far=0;
      shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
      scan_y++;//SS it's just to fill up the window
    }
    scanline_drawn_so_far=0;
    shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  }
  //-------- display to screen -------
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished frame, blitting at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));
  if (draw_lock){
    draw_end();
#if defined(SSE_VID_DD_3BUFFER_WIN)
    if (VSyncing==0 && !OPTION_3BUFFER_WIN)
      draw_blit();
#else
    if (VSyncing==0) draw_blit();
#endif
    BlitFrame=true;
  }else if (bad_drawing & 2){
    // bad_drawing bits: & 1 - bad drawing option selected  & 2 - bad-draw next screen
    //                   & 4 - temporary bad drawing because of extended monitor.
    draw(0);
    bad_drawing&=(~2);
  }
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished blitting at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));

  //----------- VBL interrupt ---------
#if !defined(SSE_GLUE)
/*  
    VBI isn't set pending now but in 64 or so cycles.
    TODO: implement SSE_INT_VBL_IACK in the vbi event
*/
#if defined(SSE_INT_VBL_IACK)
/*  This is for the case when the VBI just started (second VBI pending during
    IACK is cleared), 
    Cases? Must be pretty rare, usually the problem is not too many VBI but
    missed VBI
*/
#if defined(SSE_INT_VBL_IACK2)//as for HBL
  if(time_of_last_vbl_interrupt+VBL_IACK_LATENCY
    +M68000.LastEClockCycles[TM68000::ECLOCK_VBL]-ACT>0)
#else
  if(OPTION_HACKS && time_of_last_vbl_interrupt+56+16-ACT>0)
#endif
  {
#if defined(SSE_DEBUG)
    TRACE_INT("no VBI %d\n",time_of_last_vbl_interrupt+28-ACT);
#endif
  }
  else
#endif
  vbl_pending=true;
#endif//!glue

/* //SS this was a commented out part:
  if ((sr & SR_IPL)<SR_IPL_4){ //level 4 interupt to m68k, is VBL interrupt enabled?
    VBL_INTERRUPT
  }else{
    vbl_pending=true;
  }
*/
#if !defined(SSE_GLUE)
  scan_y=-scanlines_above_screen[shifter_freq_idx];
#endif
  if (floppy_mediach[0]) floppy_mediach[0]--;  //counter for media change
  if (floppy_mediach[1]) floppy_mediach[1]--;  //counter for media change
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
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
#if defined(SSE_VAR_RESIZE)
          draw_grille_black=max((int)draw_grille_black,4);
#else
          draw_grille_black=max(draw_grille_black,4);
#endif
        }
      }
    }
  }
#endif  
#if !defined(SSE_VID_SCREEN_CHANGE_TIMING) // -> draw_blit()
  if (mixed_output>0){
    mixed_output--;
    if (mixed_output==2){
      init_screen();
      res_change();
    }else if (mixed_output==0){
      init_screen();
      if (screen_res==0
#if defined(SSE_VID_SCANLINES_INTERPOLATED_392)
        || SCANLINES_INTERPOLATED // eg The Pawn
#endif
        ) 
        res_change();
      screen_res_at_start_of_vbl=screen_res;
    }
  }else if (screen_res!=screen_res_at_start_of_vbl){
    init_screen();
    res_change();
    screen_res_at_start_of_vbl=screen_res;
  }
#endif
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

#if defined(SSE_VID_DD_3BUFFER_WIN)
    if(OPTION_3BUFFER_WIN && !FullScreen)
      Disp.BlitIfVBlank();
#endif

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

#if defined(SSE_VID_DD_3BUFFER_WIN)
/*  This is the part responsible for high CPU use.
    Sleep(1) is sure to make us miss VBLANK.
    Maybe check probability of VBLANK but it depends on HZ
*/
      if(OPTION_3BUFFER_WIN && !FullScreen)
      {
        int limit=(int)(frame_delay_timeout)-(int)(time_for_exact_limit);
        do {
          Disp.BlitIfVBlank();
        }while( (int)(timeGetTime())<limit);
      }
      else
#endif
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

#if defined(SSE_VID_DD_3BUFFER_WIN)
        if(OPTION_3BUFFER_WIN && !FullScreen)
          Disp.BlitIfVBlank();
#endif

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
#if defined(SSE_IKBD_6301)
  if(OPTION_C1)
    HD6301.Vbl();
#endif

  RS232_VBL();   // Update all flags, check for the phone ringing
  Sound_VBL();   // Write a VBLs worth + a bit of samples to the sound card

#if defined(SSE_DRIVE_SOUND)
/*  We don't check the option here because we may have to suddenly stop
    motor sound loop.
*/
  SF314[0].Sound_CheckMotor(); // it will just check STR -> 1 drive enough
#endif

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
#if defined(SSE_X64)
  while (abs(int(ABSOLUTE_CPU_TIME-cpu_time_of_first_mfp_tick))>160000){
    cpu_time_of_first_mfp_tick+=160000; 
  }
#else
  while (abs(ABSOLUTE_CPU_TIME-cpu_time_of_first_mfp_tick)>160000){
    cpu_time_of_first_mfp_tick+=160000; 
  }
#endif

#if defined(SSE_TIMINGS_CPUTIMER64)
  while (abs((int)(ABSOLUTE_CPU_TIME-shifter_cycle_base))>160000){
#else
  while (abs(ABSOLUTE_CPU_TIME-shifter_cycle_base)>160000){
#endif
    shifter_cycle_base+=60000; //SS 60000?
  }

  shifter_pixel=shifter_hscroll;
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
  overscan_add_extra=0;
#endif
  left_border=BORDER_SIDE;right_border=BORDER_SIDE;
#if !defined(SSE_VID_DISABLE_AUTOBORDER)
  if (shifter_hscroll_extra_fetch && shifter_hscroll==0) overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
  scanline_drawn_so_far=0;
  shifter_first_draw_line=0;
  shifter_last_draw_line=shifter_y;
  if (emudetect_falcon_mode && emudetect_falcon_extra_height){
    shifter_first_draw_line=-20;
    shifter_last_draw_line=320;
    overscan=OVERSCAN_MAX_COUNTDOWN;
  }
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
  if((Glue.m_ShiftMode&2)&&screen_res<2)
  {
    shifter_last_draw_line*=2; //400 fetching lines
    memset(PCpal,0,sizeof(long)*16); // all colours black
  }
#endif

#if !defined(SSE_MMU_RELOAD_SDP_380)
  event_start_vbl(); // Reset SDP again!
#endif

#if defined(SSE_GLUE)
  Glue.Vbl();
#endif

  shifter_freq_at_start_of_vbl=shifter_freq;
  scanline_time_in_cpu_cycles_at_start_of_vbl=scanline_time_in_cpu_cycles[shifter_freq_idx];

#if defined(SSE_INT_MFP_TIMER_B_392)
  if(!OPTION_C2)
#endif
  CALC_CYCLES_FROM_HBL_TO_TIMER_B(shifter_freq);
// SS: this was so in the source
  cpu_time_of_last_vbl=time_of_next_event; ///// ABSOLUTE_CPU_TIME;
//  cpu_time_of_last_vbl=ABSOLUTE_CPU_TIME;
// /////  cpu_time_of_next_hbl_interrupt=cpu_time_of_last_vbl+cycles_for_vertical_return[shifter_freq_idx]+
// /////                                 CPU_CYCLES_FROM_LINE_RETURN_TO_HBL_INTERRUPT;
//  cpu_time_of_next_hbl_interrupt=cpu_time_of_last_vbl; ///// HBL happens immediately after VBL

#if !defined(SSE_GLUE)
  screen_event_pointer++;
  if (screen_event_pointer->event==NULL){
    cpu_time_of_start_of_event_plan=cpu_time_of_last_vbl;
#if defined(SSE_CPU_MFP_RATIO)
    if (n_cpu_cycles_per_second>CpuNormalHz){
#else
    if (n_cpu_cycles_per_second>8000000){
#endif
      screen_event_pointer=event_plan_boosted[shifter_freq_idx];
    }else{
      screen_event_pointer=event_plan[shifter_freq_idx]; // SS 0 1 2
    }
  }
#else
  cpu_time_of_start_of_event_plan=cpu_time_of_last_vbl;
#endif

  log_to(LOGSECTION_SPEEDLIMIT,"--");
#if !defined(SSE_GUI_NO_PASTE)
  PasteVBL();
#endif
  ONEGAME_ONLY( OGVBL(); )

#ifdef DEBUG_BUILD
  if (debug_run_until==DRU_VBL || (debug_run_until==DRU_SCANLINE && debug_run_until_val==scan_y)){
    if (runstate==RUNSTATE_RUNNING) runstate=RUNSTATE_STOPPING;
  }
  debug_vbl();
#endif

#if (defined(SSE_DEBUG)||defined(SSE_OSD_SHOW_TIME))
  Debug.Vbl();
#endif

#if defined(SSE_SHIFTER)
  Shifter.Vbl(); 
#endif

#if defined(SSE_VIDEO_CHIPSET)
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SDP_LINES) 
  {
#if defined(SSE_BOILER_FRAME_REPORT_392)
    FrameEvents.Add(scan_y,0,"VB",xbios2); 
#else // split was useless... (int, not word)
    FrameEvents.Add(scan_y,0,'X',(xbios2&0x00FF0000)>>16 ); 
    FrameEvents.Add(scan_y,0,'X',(xbios2&0xFFFF) ); 
#endif
  }
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK) // report starting res & sync
#if defined(SSE_BOILER_FRAME_REPORT_392)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SHIFTMODE) 
    FrameEvents.Add(scan_y,0,"R=",Shifter.m_ShiftMode); 
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SYNCMODE)
    FrameEvents.Add(scan_y,0,"S=",Glue.m_SyncMode); 
#else
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SHIFTMODE) 
    FrameEvents.Add(scan_y,0,'R',Shifter.m_ShiftMode); 
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SYNCMODE)
    FrameEvents.Add(scan_y,0,'S',Glue.m_SyncMode); 
#endif
#endif
#endif

#if defined(SSE_CPU_E_CLOCK) && !defined(SSE_TIMINGS_CPUTIMER64_C)
  // call to refresh, overkill in 64bit build
  M68000.UpdateCyclesForEClock(); 
#endif

}
//---------------------------------------------------------------------------
void prepare_cpu_boosted_event_plans()
{
  n_millions_cycles_per_sec=n_cpu_cycles_per_second/1000000;
#if !defined(SSE_GLUE)
  screen_event_struct *source,*dest;
#endif
  int factor=n_millions_cycles_per_sec; //SS TODO optimise away

#if defined(SSE_TIMING_MULTIPLIER_392)
/*  Rather than multiplying prescales, which causes imprecision, we will
    directly apply the multiplier to timer periods.
    This change also affects screen_event.time (for the best hopefully).
*/
  cpu_cycles_multiplier=(double)factor/8;
  ASSERT(cpu_cycles_multiplier>0);
#endif

  for (int idx=0;idx<3;idx++){ //3 frequencies
#if !defined(SSE_GLUE)
    source=event_plan[idx];
    dest=event_plan_boosted[idx];
    for (;;){
      dest->time=(source->time * factor)/8;
      dest->event=source->event;
      if (source->event==NULL) break;
      source++;dest++;
    }
#endif
#if defined(SSE_TIMING_MULTIPLIER_392)
    scanline_time_in_cpu_cycles[idx]=
      scanline_time_in_cpu_cycles_8mhz[idx]*cpu_cycles_multiplier;
#else
    scanline_time_in_cpu_cycles[idx]=(scanline_time_in_cpu_cycles_8mhz[idx]*factor)/8;
#endif
  }

#if !defined(SSE_TIMING_MULTIPLIER_392)
  for (int n=0;n<16;n++){
    mfp_timer_prescale[n]=min((mfp_timer_8mhz_prescale[n]*factor)/8,1000);
  }
#endif

//  init_timings();
  mfp_init_timers();
  if (runstate==RUNSTATE_RUNNING) prepare_event_again();
  CheckResetDisplay();
}
//---------------------------------------------------------------------------
#if USE_PASTI
void event_pasti_update()
{
#if defined(SSE_DISK_PASTI_AUTO_SWITCH)
  if(!(hPasti && (pasti_active || SF314[DRIVE].ImageType.Extension==EXT_STX))){
#else
  if (hPasti==NULL || pasti_active==false){
#endif
#if defined(SSE_CPU_MFP_RATIO)
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
// SSE added events
//---------------------------------------------------------------------------
#if defined(SSE_GLUE)

void event_trigger_vbi() { //6X cycles into frame (reference end of HSYNC)
  ASSERT(!Glue.Status.vbi_done);
#if defined(SSE_MMU_RELOAD_SDP_380)
/*  The video counter is reloaded from VBASE a second time at the end
    of VSYNC, when VBI is set pending.
"When the MMU sees the VSync signal from the GLUE, it resets the video counter
with the contents of $FFFF8201 and $FFFF8203 (and $FFFF820D on STE)."
    The MMU reacts to signal change, that's why it resets the counter twice, when
    VSYNC is asserted, and when it is negated. (Our theory at least)

    Cases
    Beyond/Universal Coders

update ijor:
The video pointer is updated whenever VSYNC is asserted. 
But it is not exactly that it is reloaded again at the end of VSYNC,
it is constantly being reloaded during the whole period that the signal
is asserted.
*/

#if defined(SSE_GLUE_393D)
  //ASSERT(Glue.vsync); // problems on 1st vbl, don't bother...
  Glue.vsync=false;
#endif

#if defined(SSE_MMU)
  MMU.VideoCounter=
#endif
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer=xbios2;
#endif

#if defined(SSE_STF_HW_OVERSCAN)
  // hack to get correct display
  if(SSEConfig.OverscanOn && (ST_TYPE==STF_AUTOSWITCH||ST_TYPE==STF_LACESCAN))
  {
    int off;
    if(COLOUR_MONITOR)
    {
      if (shifter_freq_at_start_of_vbl==50)
        off=(ST_TYPE==STF_LACESCAN)?(27*236-8*3):(23*224+2*8); 
      // and 236*24-80+22+8+8+8+8+8 for other "generic" overscan circuit
      else //TODO
        off=(ST_TYPE==STF_LACESCAN)?(20*234-8*3):(16*224+2*8);
#if defined(SSE_VID_BORDERS) //visual comfort
      if(DISPLAY_SIZE>=2)
        off+=8;
#endif
    }
    else //monochrome: normal size only
      off=(ST_TYPE==STF_LACESCAN)?(100*18+4):(96*18+4);
    shifter_draw_pointer+=off;
    MMU.VideoCounter=shifter_draw_pointer;
  }
#endif

#if defined(SSE_INT_VBL_IACK2)
  BYTE iack_latency=(OPTION_C1)
    ? HBL_IACK_LATENCY + M68000.LastEClockCycles[TM68000::ECLOCK_VBL]
    : CYCLES_FROM_START_OF_HBL_IRQ_TO_WHEN_PEND_IS_CLEARED;
#endif
    if(cpu_timer_at_start_of_hbl-time_of_last_vbl_interrupt>iack_latency
    ||!cpu_timer_at_start_of_hbl&&!time_of_last_vbl_interrupt)
    {
      vbl_pending=true;
    }

  Glue.Status.vbi_done=true;

/*  Our hypothesis: the GLUE reloads a counter with a value depending on 
    frequency each time it runs out, then updates it each scanline.
    Not sure it's right, but it could be, and it makes emulation more simple
    and robust compared with v382 (though there was no problem).
    This would be similar to the HSYNC_COUNTER of the comment for Glue.Update(),
    and allow external sync.
    There would be another counter for DE (scan_y in Steem), that is reset at 
    each VBLANK.
    In our implementation, the counter is loaded with the # scanlines, then
    decremented until 0.
    This means that once a frame is started, it won't change # scanlines even
    if the program changes frequency.
    We do it when VBI is enabled, by convenience.
*/

  ASSERT(!Glue.VCount); // event_trigger_vbi() enabled only if VCount=0
  if(Glue.m_ShiftMode&2) // 72hz (monochrome)
  {
    Glue.nLines=501; // not 500
#if defined(SSE_GLUE_393C)
    Glue.de_start_line=34;
    Glue.de_end_line=434-1;
#endif
  }
  else if (Glue.m_SyncMode&2) // 50hz
  {
    Glue.nLines=313;
#if defined(SSE_GLUE_393C)
    Glue.de_start_line=63;
    Glue.de_end_line=263-1;
#endif
  }
  else // 60hz
  {
    Glue.nLines=263;
#if defined(SSE_GLUE_393C)
    Glue.de_start_line=34;
    Glue.de_end_line=234-1;
#endif
  }
  scan_y=-scanlines_above_screen[shifter_freq_idx];//coherent: Hackabonds

#if defined(SSE_GLUE_393C)
/*  shifter_freq_idx==2 only if screen is monochrome
    Removing that long standing hack is necessary for this mod else
    LEGACY/socks won't work anymore.
*/
  //if(nLines==501) //conservative, will miss the first vbl
  if(Glue.m_ShiftMode&2) // risky
    scan_y=-scanlines_above_screen[2];
#endif
}

#endif


#if defined(SSE_WD1772_EMU) 
/*  There's an event for floppy now because we want to handle DRQ for each
    byte, and the resolution of HBL is too gross for that:

    6256 bytes/ track , 5 revs /s = 31280 bytes
    1 second = 8021248 CPU cycles in our emu
    8021248/31280  = 256,433 cycles / byte
    8000000/31280  = 255,754 cycles / byte
    One HBL = 512 cycles at 50hz.

    Caps works with HBL because it hold its own cycle count.
    
    Here we should transfer control, or dispatch to handlers
*/


void event_wd1772() {
  WD1772.OnUpdate();
}


void event_driveA_ip() {
  SF314[0].IndexPulse();
}


void event_driveB_ip() {
  SF314[1].IndexPulse();
}

#endif//wd

#if defined(SSE_ACIA)
//  ACIA events to handle IO with both 6301 and MIDI

COUNTER_VAR time_of_event_acia;

void event_acia() {
  time_of_event_acia=time_of_next_event+n_cpu_cycles_per_second; // put into future
  if(OPTION_C1)
  {
    // find ACIA event to run
    // start transmission
    if(ACIA_IKBD.LineTxBusy==2 && time_of_next_event==ACIA_IKBD.time_of_event_outgoing)
      ACIA_IKBD.TransmitTDR();
    else if(ACIA_MIDI.LineTxBusy==2 && time_of_next_event==ACIA_MIDI.time_of_event_outgoing)
      ACIA_MIDI.TransmitTDR();
    // IKBD
    else if(ACIA_IKBD.LineRxBusy==1 && time_of_next_event==ACIA_IKBD.time_of_event_incoming)
      agenda_keyboard_replace(0); // from IKBD
    else if(ACIA_IKBD.LineTxBusy && time_of_next_event==ACIA_IKBD.time_of_event_outgoing)
      agenda_ikbd_process(ACIA_IKBD.TDRS); // to IKBD
    // MIDI
    else if(ACIA_MIDI.LineRxBusy && time_of_next_event==ACIA_MIDI.time_of_event_incoming)
      agenda_midi_replace(0); // from MIDI
    else if(ACIA_MIDI.LineTxBusy && time_of_next_event==ACIA_MIDI.time_of_event_outgoing)
    { // to MIDI, do the job here
      ACIA_MIDI.LineTxBusy=false; 
      MIDIPort.OutputByte(ACIA_MIDI.TDRS);
      // send next MIDI note if any
      if(!(ACIA_MIDI.SR&BIT_1))
        ACIA_MIDI.TransmitTDR();
    }
    // schedule next ACIA event if any (if not, it's still in the future)
    // it's not very smart by I see no better way for now
    if(ACIA_IKBD.LineRxBusy==1 && ACIA_IKBD.time_of_event_incoming-time_of_event_acia<0)
      time_of_event_acia=ACIA_IKBD.time_of_event_incoming;
    if(ACIA_IKBD.LineTxBusy && ACIA_IKBD.time_of_event_outgoing-time_of_event_acia<0)
      time_of_event_acia=ACIA_IKBD.time_of_event_outgoing;
    if(ACIA_MIDI.LineRxBusy && ACIA_MIDI.time_of_event_incoming-time_of_event_acia<0)
      time_of_event_acia=ACIA_MIDI.time_of_event_incoming;
    if(ACIA_MIDI.LineTxBusy && ACIA_MIDI.time_of_event_outgoing-time_of_event_acia<0)
      time_of_event_acia=ACIA_MIDI.time_of_event_outgoing;
  }
}


#endif//acia

#if defined(SSE_INT_MFP_LATCH_DELAY)

COUNTER_VAR time_of_event_mfp_write=0;

void event_mfp_write() {
  
  if(OPTION_C2 && MC68901.WritePending)
  {
    ASSERT(time_of_event_mfp_write!=MC68901.WriteTiming);
    TRACE_MFP("%d %d %d %d %s=%X\n",ACT,TIMING_INFO,mfp_reg_name[MC68901.LastRegisterWritten],MC68901.LastRegisterWrittenValue);
#if defined(SSE_DEBUG) //for trace
    MC68901.LastRegisterFormerValue=mfp_reg[MC68901.LastRegisterWritten];
#endif
    mfp_reg[MC68901.LastRegisterWritten]=MC68901.LastRegisterWrittenValue;
#if defined(SSE_INT_MFP_394C)
    MC68901.UpdateNextIrq(time_of_event_mfp_write);
#else
    MC68901.UpdateNextIrq(); // for example to clear IRQ
#endif
    MC68901.WritePending=false;
  }
  time_of_event_mfp_write=time_of_next_event+n_cpu_cycles_per_second;
}

#endif//mfp



