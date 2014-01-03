
#include "SSE.h"
#if defined(SS_STRUCTURE_SSE6301_OBJ)

#include "../pch.h"

#include "SSE6301.h"
#include "SSEDebug.h"
#include <mymisc.h>
#include <acia.h>
#include <emulator.decla.h>
#include <ikbd.decla.h>
#endif

#if defined(SS_IKBD_6301)

#include "SSEOption.h"

// note most useful emulation code is in 3rdparty folder '6301'

#define LOGSECTION LOGSECTION_INIT

THD6301 HD6301; // singleton


THD6301::THD6301() {
}


THD6301::~THD6301() {
  hd6301_destroy(); // calling the 6301 function
}


#ifdef UNIX
extern EasyStr GetEXEDir();
#endif

void THD6301::Init() { // called in 'main'
  Initialised=Crashed=0;
  BYTE* ram=hd6301_init();
  EasyStr romfile;
  int checksum=0;
  FILE *fp;

  if(ram)
  {
    romfile=GetEXEDir(); // Steem's function WIN/Linux
    romfile+=HD6301_ROM_FILENAME;
    fp=fopen(romfile.Text,"r+b");
    if(fp)
    {
      int n=fread(ram+0xF000,1,4096,fp);
#if defined(SS_DEBUG)
      ASSERT(n==4096); // this detected the missing +b above
      for(int i=0;i<n;i++)
        checksum+=ram[0xF000+i];
      ASSERT( checksum==HD6301_ROM_CHECKSUM );
#endif
      fclose(fp);
      HD6301_OK=Initialised=1;
    }
    else 
    {
      printf("6301 rom error %s %d %d %d\n",romfile.Text,HD6301_OK,Initialised,ram);
      HD6301_OK=Initialised=0;
   //   if(ram)
       // free(ram);//linux: no direct access
      hd6301_destroy(); 
//      ram=NULL;
    } 
  }

#if defined(SS_DEBUG)
  TRACE_LOG("6301 emu %d RAM %d file %s open %d checksum %d\n",HD6301_OK,(bool)ram,romfile.Text,(bool)fp,checksum);
  if(!Initialised)
  {
    TRACE_OSD("NO HD6301");
    HD6301EMU_ON=0;
  }
#endif

}

#undef LOGSECTION//init
#define LOGSECTION LOGSECTION_IKBD

#if defined(SS_DEBUG)
/*  This should work for both 'fake' and 'true' 6301 emulation.
    We know command codes & parameters, we report this info through trace.
    when the command is complete.
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
    Parameter[CurrentParameter-1]--;   // bugfix 1
    if(!Parameter[CurrentParameter-1])  // bugfix 2
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
  // taking parameters of a command?
  else if(CurrentCommand!=-1)
  {
    ASSERT( nParameters );
    ASSERT( CurrentParameter>=0 && CurrentParameter<nParameters );
    Parameter[CurrentParameter++]=ByteIn;
  }
  else ; // could be junk?

  // report?
  if(CurrentCommand!=-1 && nParameters==CurrentParameter)
  {
    ReportCommand();
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
  // give command code
  TRACE_LOG("[IKBD interpreter $%02X ",CurrentCommand);
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
    case 0x20: 
      TRACE_LOG("MEMORY LOAD"); 
      TRACE_OSD("IKBD-PRG");
      break;
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
      TRACE_LOG("%d=$%X ",Parameter[i],Parameter[i]);
    TRACE_LOG(")");
  }
  TRACE_LOG("]\n");
}

#endif//debug

#if defined(SS_ACIA_DOUBLE_BUFFER_TX)

void THD6301::ReceiveByte(BYTE data) {
/*  Transfer byte to 6301 (call of this function initiates transfer).
    This function is used for true and fake 6301 emu.
*/
  TRACE_LOG("ACIA TDR->TDRS->IKBD RDRS $%X\n",data);
  ACIA_IKBD.ByteWaitingTx=false;
  agenda_add(agenda_ikbd_process,HD6301_CYCLES_TO_RECEIVE_BYTE_IN_HBL,data);
#if defined(SS_ACIA_REGISTERS)
  ACIA_IKBD.TDRS=ACIA_IKBD.TDR; // shift register
#endif
  ACIA_IKBD.LineTxBusy=true;
}

#endif

void THD6301::ResetChip(int Cold) {
  TRACE_LOG("6301 Reset chip %d\n",Cold);
  CustomProgram=CUSTOM_PROGRAM_NONE;
  ResetProgram();
#if defined(SS_IKBD_6301)
  if(HD6301_OK && HD6301EMU_ON)
  {
    HD6301.Crashed=0;
    hd6301_reset(Cold);
  }
#endif
#if defined(SS_IKBD_TRACE_CPU_READ)
/*  It's not clear at all what should be done, so 'Hacks' serves as an
    option here. It made a difference for Overdrive Demo: going back to
    menu at some point, then not anymore (? v3.5.2)
    It's also unclear whether we should do it for 'true' or 'fake' emu.
    Overdrive Demo: when you exit a demo screen, eg 'Oh No', by pressing the
    spacebar, the IKBD is reset. If the program receives the break code for
    spacebar ($B9) before the $F1, it will stop responding to IKBD input.
*/
  if(SSE_HACKS_ON)
    ZeroMemory(ST_Key_Down,sizeof(ST_Key_Down));
#endif  
}


void THD6301::ResetProgram() {
  TRACE_LOG("6301 Reset ST program\n");
  LastCommand=CurrentCommand=-1;
  CurrentParameter=0;
  nParameters=0;
}

#undef LOGSECTION

#endif
