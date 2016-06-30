#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_YM2149)
/*  In v3.5.1, object YM2149 was only used for drive management.
    In v3.7.0, sound functions were introduced.
*/

#include "../pch.h"
#include <cpu.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include "SSEYM2149.h"
#include "SSEOption.h"
#if defined(SSE_VAR_OPT_382)
#include <gui.decla.h>
#else
extern EasyStr GetEXEDir();//TODO
#endif

TYM2149::TYM2149() { //v3.7.0
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
  p_fixed_vol_3voices=NULL;
#endif
}


TYM2149::~TYM2149() { //v3.7.0
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
  FreeFixedVolTable();
#endif
}


//SOUND


#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
#define LOGSECTION LOGSECTION_SOUND

void TYM2149::FreeFixedVolTable() {
  if(p_fixed_vol_3voices)
  {
    TRACE_LOG("free memory of PSG table %p\n",p_fixed_vol_3voices);
    delete [] p_fixed_vol_3voices;
    p_fixed_vol_3voices=NULL;
  }
}


bool TYM2149::LoadFixedVolTable() {
  //TRACE("TYM2149::LoadFixedVolTable()\n");
  bool ok=false;
  FreeFixedVolTable(); //safety
  p_fixed_vol_3voices=new WORD[16*16*16];
  ASSERT(p_fixed_vol_3voices);
#if defined(SSE_VAR_OPT_382)
  EasyStr filename=RunDir+SLASH+YM2149_FIXED_VOL_FILENAME;
#else
  EasyStr filename=GetEXEDir();
  filename+=YM2149_FIXED_VOL_FILENAME;
#endif
  FILE *fp=fopen(filename.Text,"r+b");
  if(fp && p_fixed_vol_3voices)
  {
    int nwords=fread(p_fixed_vol_3voices,sizeof(WORD),16*16*16,fp);
    if(nwords==16*16*16)
      ok=true;
    fclose(fp);
    SSEConfig.ym2149_fixed_vol=true;
    TRACE_LOG("PSG %s loaded %d words in ram %p\n",filename.Text,nwords,p_fixed_vol_3voices);
  }
  else
  {
    TRACE_LOG("No file %s\n",filename.Text);
    FreeFixedVolTable();
    SSE_OPTION_PSG=0;
  }
  return ok;
}

#endif//SSE_YM2149_DYNAMIC_TABLE

//DRIVE

BYTE TYM2149::Drive(){
  BYTE drive=NO_VALID_DRIVE; // different from floppy_current_drive()
  if(!(PortA()&BIT_1))
    drive=0; //A:
  else if(!(PortA()&BIT_2))
    drive=1; //B:
  return drive;
}


BYTE TYM2149::PortA(){
  return psg_reg[PSGR_PORT_A];
}

#endif//SSE_YM2149

#endif//#if defined(STEVEN_SEAGAL)
