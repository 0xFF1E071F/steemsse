#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_YM2149)

#include "../pch.h"
#include <cpu.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include "SSEYM2149.h"

/*  In v3.5.1, object YM2149 is only used for drive management.
*/

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

#if !defined(SSE_YM2149A)//MFD
BYTE TYM2149::Side(){
  return (PortA()&BIT_0)==0;  //0: side 1;  1: side 0 !
}
#endif

#endif//SSE_YM2149

#endif//#if defined(STEVEN_SEAGAL)
