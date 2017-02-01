#include "SSE.h"

#if defined(SSE_YM2149_OBJECT)
/*  In v3.5.1, object YM2149 was only used for drive management (drive, side).
    In v3.7.0, sound functions were introduced (sampled soundchip, more realistic).
*/

#include "../pch.h"
#include <cpu.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include "SSEYM2149.h"
#include "SSEOption.h"
#include <gui.decla.h>

//SOUND

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
  bool ok=false;
  FreeFixedVolTable(); //safety
  p_fixed_vol_3voices=new WORD[16*16*16];
  ASSERT(p_fixed_vol_3voices);
  EasyStr filename=RunDir+SLASH+YM2149_FIXED_VOL_FILENAME;
  FILE *fp=fopen(filename.Text,"r+b");
  if(fp && p_fixed_vol_3voices)
  {
    int nwords=fread(p_fixed_vol_3voices,sizeof(WORD),16*16*16,fp);
    if(nwords==16*16*16)
      ok=true;
    fclose(fp);
#if defined(SSE_SOUND_391)
    // move the zero to make it match DMA's (tentative)
    for(int i=0;i<16*16*16;i++)
      p_fixed_vol_3voices[i]+=128;
#endif
    SSEConfig.ym2149_fixed_vol=true;
    TRACE_LOG("PSG %s loaded %d words in ram %p\n",filename.Text,nwords,p_fixed_vol_3voices);
  }
  else
  {
    TRACE_LOG("No file %s\n",filename.Text);
    FreeFixedVolTable();
    OPTION_SAMPLED_YM=0;
  }
  return ok;
}

#undef LOGSECTION

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

#endif//SSE_YM2149_OBJECT

