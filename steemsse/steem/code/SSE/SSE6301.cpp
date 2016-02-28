#include "SSE.h"

#if defined(SSE_IKBD_6301)
/*  Note most useful 6301 emulation code is in 3rdparty folder '6301', in C.
    Object HD6301 is more for some support, of both true and fake IKBD
    emulation.
    v3.7.3: done some refactoring without compile switches.
*/

#if defined(SSE_STRUCTURE_SSE6301_OBJ)
#include "../pch.h"
#include <easystr.h>
#include <mymisc.h>
#include <acia.decla.h>
#include <emulator.decla.h>
#include <ikbd.decla.h>
#endif//SSE_STRUCTURE_SSE6301_OBJ

#include "SSEDebug.h"
#include "SSE6301.h"
#include "SSEOption.h"
#include "SSEShifter.h" //frame //TODO
#if !defined(SSE_SHIFTER)
#include "SSEFrameReport.h" //for some trace
#endif


#define LOGSECTION LOGSECTION_INIT

THD6301 HD6301; // singleton


THD6301::THD6301() {
}


THD6301::~THD6301() {
  hd6301_destroy(); // calling the C 6301 function
}


#undef LOGSECTION//init
#define LOGSECTION LOGSECTION_IKBD

#if defined(SSE_DEBUG) && defined(SSE_IKBD_6301_IKBDI)

/*  This should work for both 'fake' and 'true' 6301 emulation.
    We know command codes & parameters, we report this info through trace.
    when the command is complete. It has no effect. Debug-only.
    3.5.3: bugfix IKBD reprogramming mess
*/
void THD6301::InterpretCommand(BYTE ByteIn) {

  // custom program running?
  if(CustomProgram==CUSTOM_PROGRAM_RUNNING) 
  {
  }
  // custom program (boot) loading?
  else if(CustomProgram==CUSTOM_PROGRAM_LOADING)
  {
    Parameter[CurrentParameter-1]--;
    if(!Parameter[CurrentParameter-1])
      CustomProgram=CUSTOM_PROGRAM_LOADED; // we don't try and ID the program
  }
  // new command?
  else if(CurrentCommand==-1 && ByteIn) 
  {
    switch(ByteIn)
    {
    case 0x80: // RESET
    case 0x07: // SET MOUSE BUTTON ACTION
    case 0x17: // SET JOYSTICK MONITORING
      nParameters=1;
      break;
    case 0x09: // SET ABSOLUTE MOUSE POSITIONING
      nParameters=4;
      break;
    case 0x0A: // SET MOUSE KEYCODE MOUSE
    case 0x0B: // SET MOUSE THRESHOLD
    case 0x0C: // SET MOUSE SCALE
    case 0x21: // MEMORY READ
    case 0x22: // CONTROLLER EXECUTE
      nParameters=2;
      break;
    case 0x0E: // LOAD MOUSE POSITION
      nParameters=5;
      break;
    case 0x19: // SET JOYSTICK KEYCODE MODE
    case 0x1B: // TIME-OF-DAY CLOCK SET
      nParameters=6;
      break;
    case 0x20: // MEMORY LOAD
      nParameters=3;
      break;
    default:
      // 8 SET RELATIVE MOUSE POSITION REPORTING
      // D INTERROGATE MOUSE POSITION
      // F SET Y=0 AT BOTTOM
      // 10 SET Y=0 AT TOP
      // 11 RESUME
      // 12 DISABLE MOUSE
      // 13 PAUSE OUTPUT
      // 14 SET JOYSTICK EVENT REPORTING
      // 15 SET JOYSTICK INTERROGATION MODE
      // 16 JOYSTICK INTERROGATE
      // 18 SET FIRE BUTTON MONITORING
      // 1A DISABLE JOYSTICKS
      // 1C INTERROGATE TIME-OF-DAT CLOCK
      // 87... STATUS INQUIRIES
      nParameters=0;
    }
    CurrentCommand=ByteIn;
    CurrentParameter=0;
  }

  else if(nParameters>12) //snapshot trouble
    CurrentCommand=-1; 

  // taking parameters of a command?
  else if(CurrentCommand!=-1)
  {
#if 0// SSE_VERSION>=373 //annoying asserts, player clicks 'ignore', crash
    ASSERT( nParameters );
    ASSERT( CurrentParameter>=0 && CurrentParameter<nParameters );
#else
    if(CurrentParameter>=0 && CurrentParameter<nParameters)
#endif
      Parameter[CurrentParameter++]=ByteIn;
  }
  else ; // could be junk?

  // report?
  if(CurrentCommand!=-1 && nParameters==CurrentParameter)
  {
#if defined(SSE_IKBD_6301_IKBDI)
#if defined(SSE_DEBUG)
    ReportCommand();
#endif
#endif
    // how to treat further bytes?
    switch(CurrentCommand) {
    case 0x20:
      TRACE_LOG("Loading %d bytes\n",Parameter[CurrentParameter-1]);
      CustomProgram=CUSTOM_PROGRAM_LOADING;
      break;
    case 0x22:
      ASSERT( CustomProgram==CUSTOM_PROGRAM_LOADED );
      CustomProgram=CUSTOM_PROGRAM_RUNNING;
      break;
    }
    LastCommand=CurrentCommand;
    CurrentCommand=-1;
  }
}

#define LOGSECTION LOGSECTION_IKBD

void THD6301::ReportCommand() {
  ASSERT( CurrentCommand!=-1 );

#if defined(SSE_OSD_CONTROL)
  if(OSD_MASK1 & OSD_CONTROL_IKBD)
    TRACE_OSD("IKBD $%02X ",CurrentCommand); 
#endif

  // give command code
  //TRACE_LOG("[IKBDi $%02X ",CurrentCommand);//i for interpreter
  TRACE_LOG("IKBDi $%02X ",CurrentCommand);//i for interpreter
  // spell out command (as in Atari manual)
  switch(CurrentCommand) {
    case 0x80: TRACE_LOG("RESET"); break;
    case 0x07: TRACE_LOG("SET MOUSE BUTTON ACTION"); break;
    case 0x17: TRACE_LOG("SET JOYSTICK MONITORING"); break;
    case 0x09: TRACE_LOG("SET ABSOLUTE MOUSE POSITIONING"); break;
    case 0x0A: TRACE_LOG("SET MOUSE KEYCODE MOUSE"); break;
    case 0x0B: TRACE_LOG("SET MOUSE THRESHOLD"); break;
    case 0x0C: TRACE_LOG("SET MOUSE SCALE"); break;
    case 0x21: TRACE_LOG("MEMORY READ"); break;
    case 0x22: TRACE_LOG("CONTROLLER EXECUTE"); break;
    case 0x0E: TRACE_LOG("LOAD MOUSE POSITION"); break;
    case 0x19: TRACE_LOG("SET JOYSTICK KEYCODE MODE"); break;
    case 0x1B: TRACE_LOG("TIME-OF-DAY CLOCK SET"); break;
    case 0x20: TRACE_LOG("MEMORY LOAD"); break;
    case 0x08: TRACE_LOG("SET RELATIVE MOUSE POSITION REPORTING"); break;
    case 0x0D: TRACE_LOG("INTERROGATE MOUSE POSITION"); break;
    case 0x0F: TRACE_LOG("SET Y=0 AT BOTTOM"); break;
    case 0x10: TRACE_LOG("SET Y=0 AT TOP"); break;
    case 0x11: TRACE_LOG("RESUME"); break;
    case 0x12: TRACE_LOG("DISABLE MOUSE"); break;
    case 0x13: TRACE_LOG("PAUSE OUTPUT"); break;
    case 0x14: TRACE_LOG("SET JOYSTICK EVENT REPORTING"); break;
    case 0x15: TRACE_LOG("SET JOYSTICK INTERROGATION MODE"); break;
    case 0x16: TRACE_LOG("JOYSTICK INTERROGATE"); break;
    case 0x18: TRACE_LOG("SET FIRE BUTTON MONITORING"); break;
    case 0x1A: TRACE_LOG("DISABLE JOYSTICKS"); break;
    case 0x1C: TRACE_LOG("INTERROGATE TIME-OF-DAT CLOCK"); break;
    case 0x87: TRACE_LOG("STATUS INQUIRY mouse button action"); break;
    case 0x88: TRACE_LOG("STATUS INQUIRY mouse mode"); break;
    case 0x8B: TRACE_LOG("STATUS INQUIRY mnouse threshold"); break;
    case 0x8C: TRACE_LOG("STATUS INQUIRY mouse scale"); break;
    case 0x8F: TRACE_LOG("STATUS INQUIRY mouse vertical coordinates"); break;
    case 0x90: TRACE_LOG("STATUS INQUIRY Y=0 at top"); break;
    case 0x92: TRACE_LOG("STATUS INQUIRY mouse enable/disable"); break;
    case 0x94: TRACE_LOG("STATUS INQUIRY joystick mode"); break;
    case 0x9A: TRACE_LOG("STATUS INQUIRY joystick enable/disable"); break;
    default:   TRACE_LOG("Unknown command %X",CurrentCommand);
  }
  // list parameters if any
  if(nParameters)
  {
    TRACE_LOG(" (");
    for(int i=0;i<nParameters;i++)
      //TRACE_LOG("%d=$%X ",Parameter[i],Parameter[i]);
      TRACE_LOG("%d=$%X ",i,Parameter[i]); // v3.8
    TRACE_LOG(")");
  }
  //TRACE_LOG("]\n");
  TRACE_LOG("\n");
}

#endif//#if defined(SSE_IKBD_6301_IKBDI)



#if SSE_VERSION<=350

void THD6301::Init() { // called in 'main'
  Initialised=Crashed=0;
  if(hd6301_init()) // calling the 6301 function
  {
    HD6301_OK=Initialised=1;
    TRACE_LOG("HD6301 emu initialised\n");
  }
  else
  {
    TRACE_LOG("HD6301 emu NOT initialised\n");
    HD6301EMU_ON=0;
  }
}

#else//!ver

void THD6301::Init() { // called in 'main'
  Initialised=Crashed=0;
  BYTE* ram=hd6301_init();
  EasyStr romfile=GetEXEDir() + HD6301_ROM_FILENAME; 
  FILE *fp;
#if defined(SSE_DEBUG)
  int checksum=0;
#endif
  if(ram)
  {
    fp=fopen(romfile.Text,"r+b");
    ASSERT(fp);
    if(fp) // put ROM (4KB) at the end of 6301 RAM
    {
#if defined(SSE_IKBD_6301_MINIRAM)
      int n=fread(ram+256,1,4096,fp);
#else
      int n=fread(ram+0xF000,1,4096,fp);
#endif
#if defined(SSE_DEBUG)
      ASSERT(n==4096); // this detected the missing +b above
      for(int i=0;i<n;i++)
#if defined(SSE_IKBD_6301_MINIRAM)
        checksum+=ram[256+i];
#else
        checksum+=ram[0xF000+i];
#endif
      ASSERT( checksum==HD6301_ROM_CHECKSUM );
#endif
      fclose(fp);
      HD6301_OK=Initialised=true;
    }
    else 
    {
      HD6301_OK=false;
      hd6301_destroy(); 
    } 
  }

#if defined(SSE_DEBUG)
  TRACE_LOG("6301 emu %d RAM %d file %s open %d checksum %d\n",HD6301_OK,(bool)ram,romfile.Text,(bool)fp,checksum);
  if(!Initialised)
  {
    TRACE_OSD("NO HD6301");
    HD6301EMU_ON=0;
  }
#endif

}


#if defined(SSE_ACIA_DOUBLE_BUFFER_TX)

void THD6301::ReceiveByte(BYTE data) {
/*  Transfer byte to 6301.
    Call of this function initiates transfer. The function sets up an
    agenda or an event about 20 scanlines later.
    This function is used for true and fake 6301 emu.
*/
  //TRACE_LOG("ACIA TDR->TDRS->IKBD RDRS $%X\n",data);
  TRACE_LOG("%d %d %d ACIA TDRS %X\n",TIMING_INFO,data);
  ACIA_IKBD.ByteWaitingTx=false;

#if defined(SSE_IKBD_6301_EVENT)
  ASSERT(ACIA_IKBD.TDR==data);
  ACIA_IKBD.TDRS=ACIA_IKBD.TDR=data;
  ASSERT(!ACIA_IKBD.LineTxBusy||ACT-ACIA_IKBD.last_tx_write_time<ACIA_TDR_COPY_DELAY);
  ACIA_IKBD.LineTxBusy=true;
  if(HD6301EMU_ON)
  {
    time_of_event_ikbd2=ACT+ACIA_TO_HD6301_IN_CYCLES;
    EventStatus|=2;
  }
  else
#endif
    agenda_add(agenda_ikbd_process,HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL,data);

#if defined(SSE_ACIA_REGISTERS) && !defined(SSE_IKBD_6301_EVENT)
  ACIA_IKBD.TDRS=ACIA_IKBD.TDR; // shift register
  ACIA_IKBD.LineTxBusy=true;
#endif
}

#endif

void THD6301::ResetChip(int Cold) {
  TRACE_LOG("6301 Reset chip %d\n",Cold);

#if defined(SSE_IKBD_6301_IKBDI)
  CustomProgram=CUSTOM_PROGRAM_NONE;
#if SSE_VERSION>=351
  ResetProgram();
#endif
#endif
#if defined(SSE_IKBD_6301)
  if(HD6301_OK && HD6301EMU_ON)
  {
    HD6301.Crashed=false;
    hd6301_reset(Cold);
  }
#endif
#if defined(SSE_IKBD_6301_STUCK_KEYS)
  if(Cold)  // real cold
    ZeroMemory(ST_Key_Down,sizeof(ST_Key_Down));
#endif
}

#if defined(SSE_IKBD_6301_IKBDI) 
void THD6301::ResetProgram() { // debug only
  TRACE_LOG("6301 Reset ST program\n");
  LastCommand=CurrentCommand=-1;
  CurrentParameter=0;
  nParameters=0;
}
#endif

#if defined(SSE_IKBD_6301_VBL)

void THD6301::Vbl() {
  //TRACE_LOG("6301 VBL cycles %d\n",hd6301_vbl_cycles);
  hd6301_vbl_cycles=0;

#if defined(SSE_IKBD_6301_MOUSE_ADJUST_SPEED2)
  click_x=click_y=0;
  // the following avoids the mouse going backward at high speed
  const int max_pix_h=(screen_res)?30:40;
  const int max_pix_v=(screen_res==2)?30:40;
  if(MouseVblDeltaX>max_pix_h)
    MouseVblDeltaX=max_pix_h;
  else if(MouseVblDeltaX<-max_pix_h)
    MouseVblDeltaX=-max_pix_h;
  if(MouseVblDeltaY>max_pix_v)
    MouseVblDeltaY=max_pix_v;
  else if(MouseVblDeltaY<-max_pix_v)
    MouseVblDeltaY=-max_pix_v;
#ifdef SSE_DEBUG
  if(MouseVblDeltaX||MouseVblDeltaX)
      TRACE_LOG("F%d 6301 mouse move %d,%d\n",FRAME,MouseVblDeltaX,MouseVblDeltaY);
#endif
#endif//SSE_IKBD_6301_MOUSE_ADJUST_SPEED2
}

#endif//SSE_IKBD_6301_VBL

#undef LOGSECTION

#endif//ver?

#endif//SSE_IKBD_6301
