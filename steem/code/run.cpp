/*---------------------------------------------------------------------------
FILE: run.cpp
MODULE: emu
DESCRIPTION: This file contains Steem's run() function, the routine that
actually makes Steem go. Also included here is the code for the event system
that allows time-critical Steem functions (such as VBLs, HBLs and MFP timers)
to be scheduled to the nearest cycle. Speed limiting and drawing is also
handled here, in event_scanline and event_vbl_interrupt.
---------------------------------------------------------------------------*/

#if defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: run.cpp")
#endif

#if defined(SSE_STRUCTURE_DECLA)

#define EXT
#define INIT(s) =s
#if defined(SSE_VAR_RESIZE_382)
EXT BYTE runstate;
#else
EXT int runstate;
#endif
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

#if defined(SSE_VAR_RESIZE_382)
EXT BYTE frameskip INIT(AUTO_FRAMESKIP);
EXT BYTE frameskip_count INIT(1);
#else
EXT int frameskip INIT(AUTO_FRAMESKIP);
EXT int frameskip_count INIT(1);
#endif


EXT bool flashlight_flag INIT(false);

DEBUG_ONLY(EXT int mode);
#if defined(SSE_VAR_RESIZE_382)
EXT BYTE mixed_output INIT(0);
#else
EXT int mixed_output INIT(0);
#endif
EXT int cpu_time_of_last_vbl,shifter_cycle_base;

EXT int cpu_timer_at_start_of_hbl;

#if defined(SSE_GLUE_FRAME_TIMINGS)
// no more plans!
#else
screen_event_struct event_plan_50hz[313*2+2],event_plan_60hz[263*2+2],event_plan_70hz[600*2+2],
                    event_plan_boosted_50hz[313*2+2],event_plan_boosted_60hz[263*2+2],event_plan_boosted_70hz[600*2+2];
screen_event_struct*screen_event_pointer,*event_plan[4],*event_plan_boosted[4];
#endif
EVENTPROC event_mfp_timer_timeout[4]={event_timer_a_timeout,event_timer_b_timeout,
                          event_timer_c_timeout,event_timer_d_timeout};
int time_of_next_event;
EVENTPROC screen_event_vector;
int cpu_time_of_start_of_event_plan;

//int cpu_time_of_next_hbl_interrupt=0;
int time_of_next_timer_b=0;
int time_of_last_hbl_interrupt;
#if defined(SSE_INT_VBL_IACK)
int time_of_last_vbl_interrupt;
#endif
#if defined(SSE_VAR_RESIZE_382)
BYTE screen_res_at_start_of_vbl;
BYTE shifter_freq_at_start_of_vbl;
#else
int screen_res_at_start_of_vbl;
int shifter_freq_at_start_of_vbl;
#endif
int scanline_time_in_cpu_cycles_at_start_of_vbl;
bool hbl_pending;

int cpu_timer_at_res_change;

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
  //ASSERT(!Blit.HasBus); // boiler update
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
#if defined(SSE_CPU_HALT)
  if(M68000.ProcessingState==TM68000::HALTED)
    return; // cancel "run" until reset
#if defined(SSE_GUI_STATUS_STRING_ICONS)
  else if(M68000.ProcessingState==TM68000::BOILER_MESSAGE)
  {
    M68000.ProcessingState=TM68000::NORMAL;
    HWND status_bar_win=GetDlgItem(StemWin,120); // get handle
    InvalidateRect(status_bar_win,NULL,false);
  }
#endif
#endif

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

#if defined(SSE_SHIFTER_TRICKS)
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
#if defined(SSE_VAR_MAIN_LOOP1)//undef 383, see main.cpp
    try {
#endif
#pragma warning(disable: 4611) //383
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
#pragma warning(disable: 4611) //383
        TRY_M68K_EXCEPTION
#pragma warning(default: 4611)
          WORD a=m68k_dpeek(LPEEK(e.bombs*4));
          if (e.bombs>8){
            alertflag=false;
#if defined(SSE_BOILER_EXCEPTION_NOT_TOS)
          }else if (crash_notification==CRASH_NOTIFICATION_NOT_TOS 
            && e._pc>=rom_addr && e._pc<rom_addr+tos_len){
            alertflag=false;
#endif
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
#if defined(SSE_VAR_MAIN_LOOP1)//undef 383
/*  This will catch exceptions during emulation.
    Tested with DIVMAX.TOS when SSE_CPU_DIVS_OVERFLOW_PC isn't defined.
    Also triggered sometimes when refactoring GLUE/Frame timings
*/
    }
    catch(...)
    {
      TRACE("System exception\n");
      runstate=RUNSTATE_STOPPING;
#if defined(SSE_GUI_STATUS_STRING) && defined(SSE_CPU_HALT)
      M68000.ProcessingState=TM68000::INTEL_CRASH; // Intel M68000 crashed
      GUIRefreshStatusBar();
#endif
    }
#endif

  }while (ExcepHappened);

  PortsRunEnd();

  Sound_Stop(Quitting);
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
    //TRACE("redraw_on_stop\n");
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
#if defined(SSE_GLUE_FRAME_TIMINGS)
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

#if defined(SSE_INT_MFP)
    PREPARE_EVENT_CHECK_FOR_MFP_WRITE;
#endif
    
    PREPARE_EVENT_CHECK_FOR_DEBUG;
    
    PREPARE_EVENT_CHECK_FOR_PASTI;

#if defined(SSE_FLOPPY_EVENT)
    PREPARE_EVENT_CHECK_FOR_FLOPPY;
#endif

#if defined(SSE_DMA_DELAY)
    PREPARE_EVENT_CHECK_FOR_DMA;
#endif

#if defined(SSE_ACIA_383)
    PREPARE_EVENT_CHECK_FOR_ACIA;
#elif defined(SSE_IKBD_6301_EVENT)
    PREPARE_EVENT_CHECK_FOR_IKBD;
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

#if defined(SSE_GLUE_FRAME_TIMINGS)
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

#if defined(SSE_INT_MFP)
    PREPARE_EVENT_CHECK_FOR_MFP_WRITE;
#endif
      
    PREPARE_EVENT_CHECK_FOR_DEBUG;
      
    PREPARE_EVENT_CHECK_FOR_PASTI;

#if defined(SSE_FLOPPY_EVENT)
    PREPARE_EVENT_CHECK_FOR_FLOPPY;
#endif

#if defined(SSE_DMA_DELAY)
    PREPARE_EVENT_CHECK_FOR_DMA;
#endif

#if defined(SSE_ACIA_383)
    PREPARE_EVENT_CHECK_FOR_ACIA;
#elif defined(SSE_IKBD_6301_EVENT)
    PREPARE_EVENT_CHECK_FOR_IKBD; // two events: both directions
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

#if defined(SSE_INT_MFP_TIMERS_INLINE)
/*  About time we did this. Easier for mods and debugging.
    It doesn't matter if it's really inlined or not.
*/
inline void handle_timeout(int tn) {

  dbg_log(Str("MFP: Timer ")+char('A'+tn)+" timeout at "+ABSOLUTE_CPU_TIME+" timeout was "+mfp_timer_timeout[tn]+
    " period was "+mfp_timer_period[tn]);

  if (mfp_timer_period_change[tn]){    
#if defined(SSE_INT_MFP)
    // Audio Artistic, timer D would count through before the write
    if(MC68901.WritePending)
    {
      TRACE_MFP("Handle time-out Flush MFP event ");
      event_mfp_write(); //flush
    }
#endif
    MFP_CALC_TIMER_PERIOD(tn);          
    mfp_timer_period_change[tn]=0;       
  }
  int stage=(mfp_timer_timeout[tn]-ABSOLUTE_CPU_TIME); 

#if defined(SSE_INT_MFP_TIMERS_WOBBLE)
  if(OPTION_C2)
    stage-=MC68901.Wobble[tn]; //get the correct timing (no drift)
#endif

  if (stage<=0){                                       
    stage+=((-stage/mfp_timer_period[tn])+1)*mfp_timer_period[tn]; 
  }else{ 
    stage%=mfp_timer_period[tn]; 
  }

  int new_timeout=ABSOLUTE_CPU_TIME+stage; 

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
#if defined(SSE_INT_MFP_TIMERS_WOBBLE)
   new_timeout+=MC68901.Wobble[tn]=rand()&MFP_TIMERS_WOBBLE;
#endif
  }
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
#if !defined(SSE_INT_MFP_TIMERS_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_A,mfp_timer_timeout[0]);
  mfp_timer_timeout[0]=new_timeout;
#endif
}
void event_timer_b_timeout()
{
  HANDLE_TIMEOUT(1);
#if !defined(SSE_INT_MFP_TIMERS_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_B,mfp_timer_timeout[1]);
  mfp_timer_timeout[1]=new_timeout;
#endif
}
void event_timer_c_timeout()
{
  HANDLE_TIMEOUT(2);
#if !defined(SSE_INT_MFP_TIMERS_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_C,mfp_timer_timeout[2]);
  mfp_timer_timeout[2]=new_timeout;
#endif
}
void event_timer_d_timeout()
{
  HANDLE_TIMEOUT(3);
#if !defined(SSE_INT_MFP_TIMERS_INLINE)
  mfp_interrupt_pend(MFP_INT_TIMER_D,mfp_timer_timeout[3]);
  mfp_timer_timeout[3]=new_timeout;
#endif
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
        dbg_log(EasyStr("MFP: Timer B timeout at ")+scanline_cycle_log());
#ifdef SSE_DEBUG
        if (mfp_interrupt_enabled[8]) TRACE_LOG("F%d y%d c%d Timer B pending\n",TIMING_INFO); //?
#endif
        mfp_timer_counter[1]=BYTE_00_TO_256(mfp_reg[MFPR_TBDR])*64;
        mfp_interrupt_pend(MFP_INT_TIMER_B,time_of_next_timer_b);
      }
    }
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+cpu_cycles_from_hbl_to_timer_b+
        scanline_time_in_cpu_cycles_at_start_of_vbl + TB_TIME_WOBBLE;
#if defined(SSE_GLUE) && defined(SSE_INT_MFP_TIMER_B_AER) // refactored
    if(mfp_reg[1]&8)
      time_of_next_timer_b-=Glue.DE_cycles[shifter_freq_idx];
#endif
  }else{
    time_of_next_timer_b=cpu_timer_at_start_of_hbl+160000;
  }
}
#undef LOGSECTION
//---------------------------------------------------------------------------
#if !defined(SSE_GLUE_FRAME_TIMINGS)

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
#if !defined(SSE_VAR_RESIZE_370) 
  right_border_changed=0;
#endif
  scanline_drawn_so_far=0;
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  cpu_timer_at_start_of_hbl=time_of_next_event; // SS as defined in draw.cpp

#if defined(SSE_SHIFTER)
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
  if (abs_quick(cpu_timer_at_start_of_hbl-time_of_last_hbl_interrupt)>CYCLES_FROM_START_OF_HBL_IRQ_TO_WHEN_PEND_IS_CLEARED
#if defined(SSE_INT_HBL_IACK_FIX)
    -12 // 28-12 = 16 = IACK cycles
#endif
    ){
    hbl_pending=true;
  }
  if (dma_sound_on_this_screen) dma_sound_fetch();//,dma_sound_fetch();
#if !defined(SSE_GLUE_FRAME_TIMINGS)
  screen_event_pointer++;  
#endif
#if defined(SSE_IKBD_6301)
  // we run some 6301 cycles at the end of each scanline (x1)
  if(OPTION_C1 && !HD6301.Crashed)
  {
    ASSERT(HD6301_OK);
    int n6301cycles;
#if defined(SSE_SHIFTER)
    n6301cycles=Glue.CurrentScanline.Cycles/HD6301_CYCLE_DIVISOR;
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
#endif//6301

#if defined(SSE_DISK_CAPS)
  if(Caps.Active==1) 
    Caps.Hbl(); 
#endif
}
#endif
//---------------------------------------------------------------------------

#if defined(SSE_GLUE_FRAME_TIMINGS_HBL)
/*  We take some tasks out of event_scanline(), so we can execute them from
    event_vbl_interrupt().
*/

void event_scanline_sub() {
#define LOGSECTION LOGSECTION_AGENDA
  CHECK_AGENDA;
#undef LOGSECTION
#if defined(SSE_IKBD_6301)
  // we run some 6301 cycles at the end of each scanline (x312)
  if(OPTION_C1 && !HD6301.Crashed)
  {
    ASSERT(HD6301_OK);
    int n6301cycles;
#if defined(SSE_SHIFTER)
    ASSERT(Glue.CurrentScanline.Cycles>=224);
    n6301cycles=Glue.CurrentScanline.Cycles/HD6301_CYCLE_DIVISOR;
#else
    n6301cycles=(screen_res==2) ? 20 : HD6301_CYCLES_PER_SCANLINE; //64
#endif
    ASSERT(n6301cycles);
    if(hd6301_run_cycles(n6301cycles)==-1)
    {
#define LOGSECTION LOGSECTION_IKBD
      TRACE_LOG("6301 emu is hopelessly crashed!\n");
#undef LOGSECTION
      TRACE2("6301 emu crash\n");
      HD6301.Crashed=1; 
    }
  }
#endif

#if defined(SSE_DISK_CAPS)
  if(Caps.Active==1)
    Caps.Hbl();
#endif

  if (dma_sound_on_this_screen) 
#if defined(SSE_CARTRIDGE_BAT)
    if(!SSEConfig.mv16 
#if defined(SSE_DONGLE_PROSOUND)
      && !(STPort[3].Type==TDongle::PROSOUND)
#endif
      ) // don't interfere with our hack
#endif
      dma_sound_fetch(); 
}

#endif

void event_scanline()
{
#if defined(SSE_GLUE_FRAME_TIMINGS_HBL)

  event_scanline_sub();

#else

#define LOGSECTION LOGSECTION_AGENDA
  CHECK_AGENDA;
#undef LOGSECTION
#if defined(SSE_IKBD_6301)
  // we run some 6301 cycles at the end of each scanline (x312)
  if(OPTION_C1 && !HD6301.Crashed)
  {
    ASSERT(HD6301_OK);
    int n6301cycles;
#if defined(SSE_SHIFTER)
    ASSERT(Glue.CurrentScanline.Cycles>=224);
    n6301cycles=Glue.CurrentScanline.Cycles/HD6301_CYCLE_DIVISOR;
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
#endif

#endif

#if defined(SSE_BUILD)
/*  Note: refactoring here is very dangerous!
    We must separate SSE_SHIFTER from SSE_INTERRUPT, which makes for many
    #if blocks.
*/

#if defined(SSE_GLUE) && defined(SSE_INT_MFP_TIMER_B_AER2)
  if(OPTION_C2 && Glue.FetchingLine())
    CALC_CYCLES_FROM_HBL_TO_TIMER_B(shifter_freq); // update each scanline
#endif

  if (scan_y<shifter_first_draw_line-1){
    if (scan_y>=draw_first_scanline_for_border){
      if (bad_drawing==0) 
#if defined(SSE_SHIFTER)
        Shifter.DrawScanlineToEnd();
#else
        draw_scanline_to_end();
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
    time_of_next_timer_b=time_of_next_event
      +cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
#if defined(SSE_GLUE) && defined(SSE_INT_MFP_TIMER_B_AER) // refactored
    if(mfp_reg[1]&8) //v3.7 also 1st line
      time_of_next_timer_b-=Glue.DE_cycles[shifter_freq_idx];
#endif
  }else if (scan_y<shifter_last_draw_line-1){
    if (bad_drawing==0) 
#if defined(SSE_SHIFTER)
      Shifter.DrawScanlineToEnd();
#else
      draw_scanline_to_end();
#endif
    time_of_next_timer_b=time_of_next_event
      +cpu_cycles_from_hbl_to_timer_b+TB_TIME_WOBBLE;
#if defined(SSE_INT_MFP_TIMER_B_AER) // refactored 
    if(mfp_reg[1]&8)
#if defined(SSE_GLUE)
      time_of_next_timer_b-=Glue.DE_cycles[shifter_freq_idx];
#else
      time_of_next_timer_b-=320;
#endif
#endif
  }else if (scan_y<draw_last_scanline_for_border){
    if (bad_drawing==0) 
#if defined(SSE_SHIFTER)
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
  
#if defined(SSE_SHIFTER) || defined(SSE_GLUE)
#if defined(SSE_VAR_OPT_383)
  if(Glue.FetchingLine())
#endif
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
#if !defined(SSE_VAR_RESIZE_370) 
  right_border_changed=0;
#endif
  scanline_drawn_so_far=0;

#if defined(DEBUG_BUILD)
/*  Enforce register limitations, so that "report SDP" isn't messed up
    in the debug build.
*/
  if(mem_len<=FOUR_MEGS) 
    shifter_draw_pointer&=0x3FFFFE;
#endif

#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
/*  Refactoring of "add extra".
    We don't use variable overscan_add_extra to adjust SDP anymore.
    Too many interferences with rendering when writing to GLU and MMU registers,
    and too many hacks due to border size.
    Instead, the video counter is recomputed every scanline, based on
    the counter at the start of the line (adapted or not because of a write)
    and #bytes recorded in CurrentScanline.Bytes (must be accurate now!).
*/
  if(!emudetect_falcon_mode && Glue.FetchingLine())
  {
#if 0 
    //looks nice but guess it takes more CPU power? (another CheckSideOverscan round)
    MMU.UpdateVideoCounter(LINECYCLES);
    shifter_draw_pointer=shifter_draw_pointer_at_start_of_line=MMU.VideoCounter;
#else
    short added_bytes=Glue.CurrentScanline.Bytes;
    if(ST_TYPE==STE && added_bytes)
      added_bytes+=(LINEWID+MMU.WordsToSkip)*2; 
#if defined(SSE_BOILER_FRAME_REPORT_MASK) && defined(SSE_GLUE_017B)
    if((FRAME_REPORT_MASK1&FRAME_REPORT_MASK_SDP_LINES))
      FrameEvents.Add(scan_y,LINECYCLES,'a',added_bytes);
#endif
    shifter_draw_pointer_at_start_of_line+=added_bytes;
    shifter_draw_pointer=shifter_draw_pointer_at_start_of_line;
#endif
  }
  else 
#endif
    shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
  MMU.VideoCounter=shifter_draw_pointer_at_start_of_line;
#endif
  /////// SS as defined in draw.cpp,
  /////// and relative to cpu_time_of_last_vbl:
  cpu_timer_at_start_of_hbl=time_of_next_event; 

#if !defined(SSE_GLUE_FRAME_TIMINGS_HBL)
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
    FrameEvents.Add(scan_y,0,'@',(shifter_draw_pointer&0x00FF0000)>>16 ); 
    FrameEvents.Add(scan_y,0,'@',(shifter_draw_pointer&0xFFFF) ); 
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
#if defined(SSE_INT_HBL_IACK_FIX)
    -12 //this was for BBC52, useless now
#endif
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
#if !defined(SSE_GLUE_FRAME_TIMINGS_HBL)
  if (dma_sound_on_this_screen) dma_sound_fetch(); 
#endif
#if !defined(SSE_GLUE_FRAME_TIMINGS)
  screen_event_pointer++;
#endif
#if !defined(SSE_VID_D3D_3BUFFER)
#if defined(SSE_VID_3BUFFER_WIN) && !defined(SSE_VID_3BUFFER_NO_VSYNC)
  if(SSE_3BUFFER && !(scan_y%2)
#if !defined(SSE_VID_3BUFFER_FS)
    && !FullScreen
#endif
    )
    Disp.BlitIfVBlank();
#endif
#endif//#if !defined(SSE_VID_D3D_3BUFFER)
#if defined(SSE_GLUE_FRAME_TIMINGS)
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
#if defined(SSE_GLUE_FRAME_TIMINGS)
/*  This emulates several "reloads" just in case, but we use # line cycles to
    avoid spurious "reloads", because we don't even know the exact timing,
    nor how the GLU decides (Closure multi-scroll triggered reload if checking
    shifter_freq)
*/
  Glue.Status.sdp_reload_done=true; // checked this line
  if(Glue.scanline==310&&Glue.CurrentScanline.Cycles!=512
    ||Glue.scanline==260&&Glue.CurrentScanline.Cycles!=508
    ||Glue.scanline==494&&Glue.CurrentScanline.Cycles!=224)
    return;
#endif
#if defined(SSE_BOILER_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VIDEOBASE)
    FrameEvents.Add(scan_y,LINECYCLES,'r',shifter_freq_idx); // "reload"
#endif

  // This happens about 60 cycles into scanline 247 (50Hz) //SS but the timing in frame event plan was different
  //TRACE_LOG("F%d L%d reload SDP (%X) <- %X\n",FRAME,scan_y,shifter_draw_pointer,xbios2);
  //TRACE("RELOAD SDP %X -> %X h %d y %d f %d c %d\n",shifter_draw_pointer,xbios2,Glue.scanline,scan_y,shifter_freq,LINECYCLES);

#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
  MMU.VideoCounter=
#endif
  shifter_draw_pointer=xbios2; // SS: reload SDP
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
  shifter_pixel=shifter_hscroll;
#if !defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA2)
  overscan_add_extra=0;
#endif
  left_border=BORDER_SIDE;right_border=BORDER_SIDE;
#if !defined(SSE_GLUE_FRAME_TIMINGS)
  screen_event_pointer++;
#endif
#if defined(SSE_GLUE_FRAME_TIMINGS)
  Glue.Status.vbl_done=false;
#endif
}

#undef LOGSECTION//3.8.0

//---------------------------------------------------------------------------
void event_vbl_interrupt() //SS misleading name?
{ 

#if defined(SSE_CARTRIDGE_FREEZE)
/*  When pressing some button of his cartridge, the player triggered an MFP
    interrupt.
    By releasing the button, the concerned bit should change state. We use
    no counter but do it at first VBL for simplicity.
*/
  if(cart)
  {
#if defined(SSE_DONGLE_URC) 
#if defined(SSE_DONGLE_MENU)
    if(STPort[3].Type==TDongle::URC && !(mfp_reg[MFPR_GPIP]&0x40))
#else
    if(STPort[2].Type==PORTTYPE_DONGLE_URC && !(mfp_reg[MFPR_GPIP]&0x40))
#endif
      mfp_gpip_set_bit(MFP_GPIP_RING_BIT,true); // Ultimate Ripper
    else 
#endif
#if defined(SSE_DONGLE_MULTIFACE) // cart + monochrome
    if(STPort[3].Type==TDongle::MULTIFACE && !(mfp_reg[MFPR_GPIP]&0x80))
#else
    if(!(mfp_reg[MFPR_GPIP]&0x80)) // Multiface
#endif
      mfp_gpip_set_bit(MFP_GPIP_MONO_BIT,true);
  }
#endif

#if defined(SSE_GLUE_FRAME_TIMINGS_HBL)
/*  With GLU/video event refactoring, we call event_scanline() one time fewer,
    if we did now it would mess up some timings, so we call the sub
    with some HBL-dependent tasks: DMA sound, HD6301 & CAPS emu.
    Important for Relapse DMA sound
*/
  event_scanline_sub(); 
#endif
#if defined(SSE_VID_VSYNC_WINDOW)
  bool VSyncing=( (SSE_WIN_VSYNC&&bAppActive||FSDoVsync&&FullScreen) 
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
      scanline_drawn_so_far=0;
      shifter_draw_pointer_at_start_of_line=shifter_draw_pointer;
      Glue.IncScanline();
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
#if defined(SSE_VID_3BUFFER_WIN) && !defined(SSE_VID_3BUFFER_NO_VSYNC)
    if (VSyncing==0 
#if !defined(SSE_VID_D3D_3BUFFER)
      &&( !SSE_3BUFFER
#if !defined(SSE_VID_3BUFFER_FS)
      || FullScreen
#endif
      )
#endif//#if !defined(SSE_VID_D3D_3BUFFER)
      )
      draw_blit();
#else
    if (VSyncing==0) draw_blit();
#endif
    BlitFrame=true;
  }else if (bad_drawing & 2){
    // bad_drawing bits: & 1 - bad drawing option selected  & 2 - bad-draw next screen
    //                   & 4 - temporary bad drawing because of extended monitor.
    //TRACE("bad drawing\n");
    draw(0);
    bad_drawing&=(~2);
  }
  log_to(LOGSECTION_SPEEDLIMIT,Str("SPEED: Finished blitting at ")+(timeGetTime()-run_start_time)+" timer="+(timer-run_start_time));

  //----------- VBL interrupt ---------
#if !defined(SSE_GLUE_FRAME_TIMINGS)
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
#if !defined(SSE_GLUE_FRAME_TIMINGS)
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
#if defined(SSE_VAR_RESIZE_370)
          draw_grille_black=max((int)draw_grille_black,4);
#else
          draw_grille_black=max(draw_grille_black,4);
#endif
        }
      }
    }
  }
#endif  
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
//#if !defined(SSE_VID_D3D_3BUFFER)
#if defined(SSE_VID_3BUFFER_WIN) && !defined(SSE_VID_3BUFFER_NO_VSYNC)
  if(SSE_3BUFFER
#if !defined(SSE_VID_3BUFFER_FS)
    && !FullScreen
#endif
    )
    Disp.BlitIfVBlank();
#endif
//#endif//#if !defined(SSE_VID_D3D_3BUFFER)

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
#if !defined(SSE_VID_D3D_3BUFFER)
#if defined(SSE_VID_3BUFFER_WIN) && !defined(SSE_VID_3BUFFER_NO_VSYNC)
/*  This is the part responsible for high CPU use.
    Sleep(1) is sure to make us miss VBLANK.
    Maybe check probability of VBLANK but it depends on HZ
*/
      if(SSE_3BUFFER
#if !defined(SSE_VID_3BUFFER_FS)
        && !FullScreen
#endif
        )
      {
        int limit=(int)(frame_delay_timeout)-(int)(time_for_exact_limit);
        do {
          Disp.BlitIfVBlank();
        }while( (int)(timeGetTime())<limit);
      }
      else
#endif
#endif//#if !defined(SSE_VID_D3D_3BUFFER)
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
#if !defined(SSE_VID_D3D_3BUFFER)
#if defined(SSE_VID_3BUFFER_WIN) && !defined(SSE_VID_3BUFFER_NO_VSYNC)
        if(SSE_3BUFFER //&& SSE_WIN_VSYNC
#if !defined(SSE_VID_3BUFFER_FS)
          && !FullScreen 
#endif
          )
          Disp.BlitIfVBlank();
#endif
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
#if defined(SSE_IKBD_6301_VBL)
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
#if !defined(SSE_INT_MFP_TIMERS_BASETIME) //let's eliminate that junk...
  // The MFP clock aligns with the CPU clock every 8000 CPU cycles
  while (abs(ABSOLUTE_CPU_TIME-cpu_time_of_first_mfp_tick)>160000){
    cpu_time_of_first_mfp_tick+=160000; 
  }
#endif
  while (abs(ABSOLUTE_CPU_TIME-shifter_cycle_base)>160000){
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
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY_382)
#if defined(SSE_GLUE_383)
  if((Shifter.m_ShiftMode&2)&&screen_res<2) //TODO?
#else
  if((Glue.m_ShiftMode&2)&&screen_res<2)
#endif
    shifter_last_draw_line*=2; //400 fetching lines
#endif

#if !defined(SSE_MMU_RELOAD_SDP_380)
  event_start_vbl(); // Reset SDP again!
#endif

#if defined(SSE_GLUE_FRAME_TIMINGS)
  Glue.Vbl();
#endif

#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY_370)//no
/*  v3.7
    The ST could be set on high resolution for a long time
    on a colour screen.
    Without damaging the screen?
    In that case we must adapt Steem timings as if we had a
    monochrome monitor. Display will be off.
    The test is so so, curious to see what it will break :)
    Fixes My Socks are Weapons.
    The protection of this demo is rather vicious. It sets the
    ST in highres, then uses the video counter in the middle of
    its trace decoding routine.
    Update v3.8.2: other way (SSE_SHIFTER_HIRES_COLOUR_DISPLAY_382)
*/

  if( screen_res<2 && (Shifter.m_ShiftMode&2) && COLOUR_MONITOR
    && Glue.CurrentScanline.Cycles==224) 
  {
    TRACE_OSD("RES2");
    screen_res=2;
    shifter_freq=MONO_HZ;
    shifter_freq_idx=2;
    init_screen(); //radical but spares much code
  }
#endif//SSE_SHIFTER_HIRES_COLOUR_DISPLAY_370

  shifter_freq_at_start_of_vbl=shifter_freq;
  scanline_time_in_cpu_cycles_at_start_of_vbl=scanline_time_in_cpu_cycles[shifter_freq_idx];
  CALC_CYCLES_FROM_HBL_TO_TIMER_B(shifter_freq);
// SS: this was so in the source
  cpu_time_of_last_vbl=time_of_next_event; ///// ABSOLUTE_CPU_TIME;
//  cpu_time_of_last_vbl=ABSOLUTE_CPU_TIME;
// /////  cpu_time_of_next_hbl_interrupt=cpu_time_of_last_vbl+cycles_for_vertical_return[shifter_freq_idx]+
// /////                                 CPU_CYCLES_FROM_LINE_RETURN_TO_HBL_INTERRUPT;
//  cpu_time_of_next_hbl_interrupt=cpu_time_of_last_vbl; ///// HBL happens immediately after VBL

#if !defined(SSE_GLUE_FRAME_TIMINGS)
  screen_event_pointer++;
  if (screen_event_pointer->event==NULL){
    cpu_time_of_start_of_event_plan=cpu_time_of_last_vbl;
#if defined(SSE_INT_MFP_RATIO)
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

  PasteVBL();
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
    FrameEvents.Add(scan_y,0,'X',(xbios2&0x00FF0000)>>16 ); 
    FrameEvents.Add(scan_y,0,'X',(xbios2&0xFFFF) ); 
  }
#endif
#if defined(SSE_BOILER_FRAME_REPORT_MASK) // report starting res & sync
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SHIFTMODE) 
    FrameEvents.Add(scan_y,0,'R',Shifter.m_ShiftMode); 
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SYNCMODE)
    FrameEvents.Add(scan_y,0,'S',Glue.m_SyncMode); 
#endif
#endif

#if defined(SSE_CPU_E_CLOCK)
  M68000.UpdateCyclesForEClock(); // this function is called at least each VBL
#endif

}
//---------------------------------------------------------------------------
void prepare_cpu_boosted_event_plans()
{
  n_millions_cycles_per_sec=n_cpu_cycles_per_second/1000000;
#if !defined(SSE_GLUE_FRAME_TIMINGS)
  screen_event_struct *source,*dest;
#endif
  int factor=n_millions_cycles_per_sec; //SS TODO optimise away
#if defined(SSE_TIMING_MULTIPLIER)
  cpu_cycles_multiplier=factor/8;
  ASSERT(cpu_cycles_multiplier>0);
#elif defined(SSE_CPU_4GHZ) || defined(SSE_CPU_3GHZ) || defined(SSE_CPU_2GHZ) || defined(SSE_CPU_1GHZ) || defined(SSE_CPU_512MHZ)
  if(factor>=512)
    SSEOption.Chipset2=SSEOption.Chipset1=false; 
#endif
  for (int idx=0;idx<3;idx++){ //3 frequencies
#if !defined(SSE_GLUE_FRAME_TIMINGS)
    source=event_plan[idx];
    dest=event_plan_boosted[idx];
    for (;;){
      dest->time=(source->time * factor)/8;
      dest->event=source->event;
      if (source->event==NULL) break;
      source++;dest++;
    }
#endif
    scanline_time_in_cpu_cycles[idx]=(scanline_time_in_cpu_cycles_8mhz[idx]*factor)/8;
  }
  //TRACE("factor %d\n",factor);
  for (int n=0;n<16;n++){
#if defined(SSE_INT_MFP_TIMERS_NO_BOOST_LIMIT)
/*  We leave the prescale unlimited, it won't mess timings if the value is small 
    enough... but what if it's big?
*/
    mfp_timer_prescale[n]=(mfp_timer_8mhz_prescale[n]*factor)/8;
#else
    mfp_timer_prescale[n]=min((mfp_timer_8mhz_prescale[n]*factor)/8,1000);
#endif
    //TRACE("mfp_timer_prescale[%d]=%d (%d) (%d)\n",n,mfp_timer_prescale[n],(mfp_timer_8mhz_prescale[n]*factor)/8,mfp_timer_8mhz_prescale[n]);
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
#if defined(SSE_DISK_PASTI_ONLY_STX)
    ||PASTI_JUST_STX && SF314[YM2149.SelectedDrive].ImageType.Extension!=EXT_STX
#if defined(SSE_DISK_PASTI_ONLY_STX_HD) && defined(SSE_DMA_OBJECT)
    && ! ( pasti_active && (Dma.MCR&TDma::CR_HDC_OR_FDC)) // hard disk handling by pasti
#endif
#endif
    ){
#if defined(SSE_INT_MFP_RATIO)
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

#if defined(SSE_GLUE_FRAME_TIMINGS)

void event_trigger_vbi() { //6X cycles into frame (reference end of HSYNC)
#if defined(SSE_GLUE_FRAME_TIMINGS)
  ASSERT(!Glue.Status.vbi_done);
#endif
#if defined(SSE_MMU_RELOAD_SDP_380)
/*  The video counter is reloaded from VBASE a second time at the end
    of VSYNC, when VBI is set pending.
"When the MMU sees the VSync signal from the GLUE, it resets the video counter
with the contents of $FFFF8201 and $FFFF8203 (and $FFFF820D on STE)."
    The MMU reacts to signal change, that's why it resets the counter twice, when
    VSYNC is asserted, and when it is negated. (Our theory at least)

    Cases
    Beyond/Universal Coders
*/

#if defined(SSE_GLUE_REFACTOR_OVERSCAN_EXTRA)
  MMU.VideoCounter=
#endif
  shifter_draw_pointer_at_start_of_line=shifter_draw_pointer=xbios2;
#endif

#if defined(SSE_INT_VBL_IACK2)
  BYTE iack_latency=(OPTION_C1)
    ? HBL_IACK_LATENCY + M68000.LastEClockCycles[TM68000::ECLOCK_VBL]
    : CYCLES_FROM_START_OF_HBL_IRQ_TO_WHEN_PEND_IS_CLEARED;
  if(cpu_timer_at_start_of_hbl-time_of_last_vbl_interrupt>iack_latency
    ||!cpu_timer_at_start_of_hbl&&!time_of_last_vbl_interrupt)
#endif
      vbl_pending=true;

#if !defined(SSE_GLUE_FRAME_TIMINGS)
  screen_event_pointer++;
#endif
#if defined(SSE_GLUE_FRAME_TIMINGS)
  Glue.Status.vbi_done=true;
#endif
}
#endif


#if defined(SSE_FLOPPY_EVENT) 
#if defined(SSE_DISK_STW) || defined(SSE_DISK_SCP) || defined(SSE_DISK_HFE)
/*  There's an event for floppy now because we want to handle DRQ for each
    byte, and the resolution of HBL is too gross for that:

    6256 bytes/ track , 5 revs /s = 31280 bytes
    1 second = 8021248 CPU cycles in our emu
    8021248/31280  = 256,433 cycles / byte
    8000000/31280  = 255,754 cycles / byte
    One HBL = 512 cycles at 50hz.

    Caps works with HBL because it hold its own cycle count.
    (which is maybe the way we should handle STW too. TODO)
    
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
#endif
#endif//flp

#if defined(SSE_ACIA_383)
//  ACIA events to handle IO with both 6301 and MIDI

int time_of_event_acia=0;

void event_acia() {
  time_of_event_acia=time_of_next_event+n_cpu_cycles_per_second; 
  if(OPTION_C1)
  {
    // IKBD
    if(ACIA_IKBD.LineRxBusy && time_of_next_event==ACIA_IKBD.time_of_event_incoming)
      agenda_keyboard_replace(0); // from IKBD
    else if(ACIA_IKBD.LineTxBusy && time_of_next_event==ACIA_IKBD.time_of_event_outgoing)
      agenda_ikbd_process(ACIA_IKBD.TDRS); // to IKBD
    // MIDI
    else if(ACIA_MIDI.LineRxBusy && time_of_next_event==ACIA_MIDI.time_of_event_incoming)
      agenda_midi_replace(0); // from MIDI
    else if(ACIA_MIDI.LineTxBusy && time_of_next_event==ACIA_MIDI.time_of_event_outgoing)
    { // to MIDI, do the job here
      MIDIPort.OutputByte(ACIA_MIDI.TDRS);
      ACIA_MIDI.SR|=BIT_1; // TDRE (register free)
      if((ACIA_MIDI.CR&BIT_5)&&!(ACIA_MIDI.CR&BIT_6))
      {
        ACIA_MIDI.SR|=BIT_7; 
        mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,0); //trigger IRQ (rare!)
      }
      ACIA_MIDI.LineTxBusy=false;
      // send next MIDI note if any
      if(ACIA_MIDI.ByteWaitingTx)
      {
        ACIA_MIDI.TDRS=ACIA_MIDI.TDR;
        ACIA_MIDI.LineTxBusy=true;
        time_of_event_acia=time_of_next_event+ACIA_MIDI_OUT_CYCLES;
        ACIA_MIDI.ByteWaitingTx=false;
      }
    }
  }
}

#elif defined(SSE_IKBD_6301_EVENT)

int time_of_event_ikbd,time_of_event_ikbd2;

void event_ikbd() {
  time_of_event_ikbd=time_of_next_event+n_cpu_cycles_per_second; 
  if(OPTION_C1 && (HD6301.EventStatus&1))
  {
    HD6301.EventStatus&=0xFE;
    agenda_keyboard_replace(0);
  }
}


void event_ikbd2() {
  time_of_event_ikbd2=time_of_next_event+n_cpu_cycles_per_second; 
  if(OPTION_C1 && (HD6301.EventStatus&2))
  {
    HD6301.EventStatus&=0xFD;
    agenda_ikbd_process(ACIA_IKBD.TDRS);
  }
}

#endif//ikbd

#if defined(SSE_INT_MFP)

int time_of_event_mfp_write=0;

void event_mfp_write() {
  
  if(OPTION_C2 && MC68901.WritePending)
  {
    ASSERT(time_of_event_mfp_write!=MC68901.WriteTiming);
//    TRACE_MFP("%d execute event_mfp_write(): mfp_reg[%d]=%X\n",ACT,MC68901.LastRegisterWritten,MC68901.LastRegisterWrittenValue);
    TRACE_MFP("%d %s=%X\n",ACT,mfp_reg_name[MC68901.LastRegisterWritten],MC68901.LastRegisterWrittenValue);
#if defined(SSE_DEBUG) //for trace
    MC68901.LastRegisterFormerValue=mfp_reg[MC68901.LastRegisterWritten];
#endif
    mfp_reg[MC68901.LastRegisterWritten]=MC68901.LastRegisterWrittenValue;
    MC68901.UpdateNextIrq(); // for example to clear IRQ
    MC68901.WritePending=false;
  }
  time_of_event_mfp_write=time_of_next_event+n_cpu_cycles_per_second;
}


#endif//mfp



