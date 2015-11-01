/*
*/

#if defined(SSE_STRUCTURE_INFO) && !defined(SSE_STRUCTURE_SSECPU_OBJ)
#pragma message("Included for compilation: SSECpu.cpp")
#endif

#include "SSE.h"

#if defined(SSE_STRUCTURE_SSECPU_OBJ) // defined or not in SSE.h
#include "../pch.h" // Each object should include this precompiled header
#pragma hdrstop  // Signals the end of precompilation
#include <gui.decla.h> //PeekEvent()
#include "SSECpu.h"
#include "SSEFrameReport.h"
#if defined(DEBUG_BUILD)
////////#include <trace.decla.h>
extern const char*exception_action_name[4];//={"read from","write to","fetch from","instruction execution"};
#endif
#endif//#if defined(SSE_STRUCTURE_SSECPU_OBJ)

#include "SSESTF.h"
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
#include "SSEGlue.h"
#endif

#if defined(SSE_CPU)

TM68000 M68000; // singleton


TM68000::TM68000() {
  Reset(true);
}


void TM68000::Reset(bool cold) {
#if defined(SSE_DEBUG)    
  if(cold)
    nExceptions=nInstr=0;
  NextIrFetched=false; //at power on
#endif
#if defined(SSE_CPU_PREFETCH_CALL) //yes
  CallPrefetch=FALSE;
#endif
}


//////////////////////////////
// Effective Address (<EA>) //
//////////////////////////////

/*

This is copied from cpu.cpp (GET SOURCE block) and extended.

Explanation for TRUE_PC, it's based on microcodes and confirmed by tests, 
so there's no contest!
It's useful in case of crash during the read.

b543	b210         Mode           .B, .W             .L

000	R            Dn              PC                PC
001	R            An              PC                PC
010	R            (An)            PC                PC
011	R            (An)+           PC                PC
100	R            -(An)           PC+2              PC
101	R            (d16, An)       PC                PC
110	R            (d8, An, Xn)    PC                PC
111	000          (xxx).W         PC+2              PC+2
111	001          (xxx).L         PC+4              PC+4
111	010          (d16, PC)       PC                PC
111	011          (d8, PC, Xn)    PC                PC
111	100          #<data>         PC+2              PC+4

*/
/*
Effective Address 						
						
		Mode Field	Reg Field	Cycles (n(r/w))		
Addressing Mode 	source	b543	b210	Byte, Word 	Long	
	dest	b876	bBA9			
Dn		000	R	0(0/0)	0(0/0)	Data Register Direct
An		001	R	0(0/0)	0(0/0)	Address Register Direct
(An)		010	R	4(1/0)	8(2/0)	Address Register Indirect
(An)+		011	R	4(1/0)	8(2/0)	Address Register Indirect with Postincrement
–(An)		100	R	6(1/0)	10(2/0)	Address Register Indirect with Predecrement
(d16, An)		101	R	8(2/0)	12(3/0)	Address Register Indirect with Displacement
(d8, An, Xn)*		110	R	10(2/0)	14(3/0)	Address Register Indirect with Index
(xxx).W		111	000	8(2/0)	12(3/0)	Absolute Short
(xxx).L		111	001	12(3/0)	16(4/0)	Absolute Long
(d16, PC)		111	010	8(2/0)	12(3/0)	Program Counter Indirect with Displacement
(d8, PC, Xn)*		111	011	10(2/0)	14(3/0)	Program Counter Indirect with Index - 8-Bit Displacement
#<data>		111	100	4(1/0)	8(2/0)	Immediate

*/

// Dn

void m68k_get_source_000_b(){ // .B Dn
  m68k_src_b=(BYTE)(r[PARAM_M]); 
}


void m68k_get_source_000_w(){ //.W Dn
  m68k_src_w=(WORD)(r[PARAM_M]); 
}


void m68k_get_source_000_l(){ //.L Dn
  m68k_src_l=(long)(r[PARAM_M]); 
}


// An

void m68k_get_source_001_b(){ // .B An
  m68k_src_b=(BYTE)(areg[PARAM_M]); 
}


void m68k_get_source_001_w(){ // .W An
  m68k_src_w=(WORD)(areg[PARAM_M]); 
}


void m68k_get_source_001_l(){ // .L An
  m68k_src_l=(long)(areg[PARAM_M]); 
}

// (An)

void m68k_get_source_010_b(){ // .B (An)

  INSTRUCTION_TIME_ROUND(4);

  m68k_READ_B(areg[PARAM_M]) 

}


void m68k_get_source_010_w(){ // .W (An)

  INSTRUCTION_TIME_ROUND(4);

  m68k_READ_W(areg[PARAM_M]) 

}


void m68k_get_source_010_l(){ // .L (An)

  INSTRUCTION_TIME_ROUND(8);

  m68k_READ_L(areg[PARAM_M]) 

}

// (An)+

void m68k_get_source_011_b(){ // .B (An)+

  INSTRUCTION_TIME_ROUND(4);

  m68k_READ_B(areg[PARAM_M]) 
  areg[PARAM_M]++; 
  if(PARAM_M==7)
    areg[7]++;

}


void m68k_get_source_011_w(){ // .W (An)+

  INSTRUCTION_TIME_ROUND(4);

  m68k_READ_W(areg[PARAM_M]) 

  areg[PARAM_M]+=2; 
}


void m68k_get_source_011_l(){ // .L (An)+

  INSTRUCTION_TIME_ROUND(8);

  m68k_READ_L(areg[PARAM_M]) 

  areg[PARAM_M]+=4; // for .L, we assume ++ post read

}

// -(An)

#if !defined(SSE_CPU_EXCEPTION)//temp...
//long silly_dummy_for_true_pc2;
//#define TRUE_PC silly_dummy_for_true_pc2
#endif

void m68k_get_source_100_b(){ // .B -(An)
#if defined(SSE_CPU_TRUE_PC)
  TRUE_PC+=2;
#endif
  INSTRUCTION_TIME_ROUND(6);

  areg[PARAM_M]--;
  if(PARAM_M==7)
    areg[7]--;

  m68k_READ_B(areg[PARAM_M]) 

}


void m68k_get_source_100_w(){ // .W -(An)
#if defined(SSE_CPU_TRUE_PC)
  TRUE_PC+=2;
#endif
  INSTRUCTION_TIME_ROUND(6);

  areg[PARAM_M]-=2;

  m68k_READ_W(areg[PARAM_M])  

}


void m68k_get_source_100_l(){ // .L -(An)

  INSTRUCTION_TIME_ROUND(10);

  areg[PARAM_M]-=4;

  m68k_READ_L(areg[PARAM_M])  

}


// (d16, An)

void m68k_get_source_101_b(){ //.B (d16, An)

  INSTRUCTION_TIME_ROUND(8);

  register int fw=(signed short)m68k_fetchW(); 
  pc+=2;

  m68k_READ_B(areg[PARAM_M]+fw);

}


void m68k_get_source_101_w(){ // .W (d16, An)

  INSTRUCTION_TIME_ROUND(8);

  register int fw=(signed short)m68k_fetchW(); 
  pc+=2;

  m68k_READ_W(areg[PARAM_M]+fw);

}


void m68k_get_source_101_l(){ // .L (d16, An)

  INSTRUCTION_TIME_ROUND(12);

  register int fw=(signed short)m68k_fetchW();
  pc+=2;

  m68k_READ_L(areg[PARAM_M]+fw);

}

// (d8,An,Xn)

/*  (d8,An,Xn) +(d8,PC,Xn)
    Hatari: 
    "On ST, d8(An,Xn) takes 2 cycles more (which can generate pairing)."
    It was already correctly handled in Steem by using :
    INSTRUCTION_TIME_ROUND(10);
    It will take 12 cycles except if pairing.
    Cases: DSoS, Anomaly
    Strange though, still to examine.
*/

void m68k_get_source_110_b(){ // .B (d8,An,Xn)

  INSTRUCTION_TIME_ROUND(10);

  WORD w=m68k_fetchW();
  pc+=2;

  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  m68k_READ_B_FROM_ADDR

}


void m68k_get_source_110_w(){ // .W (d8,An,Xn)

  INSTRUCTION_TIME_ROUND(10);

  WORD w=m68k_fetchW();

  pc+=2;
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }

  m68k_READ_W_FROM_ADDR

}


void m68k_get_source_110_l(){ // .L (d8,An,Xn)

  INSTRUCTION_TIME_ROUND(14);

  WORD w=m68k_fetchW();
  pc+=2; 

  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }

  m68k_READ_L_FROM_ADDR

}

// (xxx).W, (xxx).L, (d16, PC), (d8, PC, Xn), #<data>

void m68k_get_source_111_b(){

  switch(ir&0x7){
  case 0:{ // .B (xxx).W
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=2;
#endif
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
    m68k_READ_B(fw)
    break;
  }
  case 1:{ // .B (xxx).L
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=4;
#endif
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fl=m68k_fetchL();
    pc+=4;  
    m68k_READ_B(fl)
    break;
   }
  case 2:{ // .B (d16, PC)
    INSTRUCTION_TIME_ROUND(8);
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
    m68k_READ_B(ad)
    break;
  }
  case 3:{ // .B (d8, PC, Xn)

    INSTRUCTION_TIME_ROUND(10);

    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    PC_RELATIVE_MONITOR(abus);
    pc+=2;
    m68k_READ_B_FROM_ADDR
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }
  case 4:{ // .B #<data>
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=2;
#endif
    INSTRUCTION_TIME_ROUND(4);
    pc+=2;
    m68k_src_b=m68k_fetchB();
    break;
  }
  default:
    ILLEGAL;
  }
}


void m68k_get_source_111_w(){

  switch(ir&0x7){
  case 0:{ // .W (xxx).W
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=2;
#endif
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 

#if defined(SSE_CPU_DATABUS)
///    TRACE("dbus %x -> %x\n", M68000.dbus,IRC);
    M68000.dbus=IRC;
#endif

    m68k_READ_W(fw)
    break;
  }case 1:{ // .W (xxx).L
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=4;
#endif
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fl=m68k_fetchL();
    pc+=4;  
    m68k_READ_W(fl)
    break;
  }case 2:{ // .W (d16, PC)
    INSTRUCTION_TIME_ROUND(8);
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
    m68k_READ_W(ad)
    break;
  }case 3:{ // .W (d8, PC, Xn)
    INSTRUCTION_TIME_ROUND(10);
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    m68k_READ_W_FROM_ADDR
    PC_RELATIVE_MONITOR(abus);
    pc+=2; 
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }case 4:{ // .B #<data>
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=2;
#endif
    INSTRUCTION_TIME_ROUND(4);
//      ap=m68k_fetchL();pc+=4;
    pc+=2;
    m68k_src_w=m68k_fetchW();
    break;
  }default:
    ILLEGAL;
  }
}


void m68k_get_source_111_l(){

  switch(ir&0x7){
  case 0:{ // .L (xxx).W
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=2;
#endif
    INSTRUCTION_TIME_ROUND(12);
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
    m68k_READ_L(fw) 
    break;
  }case 1:{ // .L (xxx).L
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=4;
#endif
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fl=m68k_fetchL();
    pc+=4;  
    m68k_READ_L(fl)
    break;
  }case 2:{ // .L (d16, PC)
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
    m68k_READ_L(ad)
    break;
  }case 3:{ // .L (d8, PC, Xn)
    INSTRUCTION_TIME_ROUND(14);
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    PC_RELATIVE_MONITOR(abus);
    pc+=2;
    m68k_READ_L_FROM_ADDR
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }case 4:{ // .B #<data>
#if defined(SSE_CPU_TRUE_PC)
    TRUE_PC+=4;
#endif
    INSTRUCTION_TIME_ROUND(8);
    m68k_src_l=m68k_fetchL();
    pc+=4;  
    break;
  }default:
    ILLEGAL;
  }
}


/////////////////
// Destination //
/////////////////


/*  For some instructions, EA microcodes are used and the data is read even if
    it isn't necessary for the logic of the instruction.
    CLR etc. read before write.
    If we really are in a write cycle, PC should be at its definitive value.
    If it's a read cycle, we use the <EA> rules for PC.
*/


void m68k_get_dest_000_b(){ 
  m68k_dest=r+PARAM_M; 
}

void m68k_get_dest_000_w(){ 
  m68k_dest=r+PARAM_M; 
}

void m68k_get_dest_000_l(){ 
  m68k_dest=r+PARAM_M; 
}

void m68k_get_dest_001_b(){ 
  m68k_dest=areg+PARAM_M; 
}

void m68k_get_dest_001_w(){ 
  m68k_dest=areg+PARAM_M; 
}

void m68k_get_dest_001_l(){ 
  m68k_dest=areg+PARAM_M; 
}

void m68k_get_dest_010_b(){ 
  INSTRUCTION_TIME_ROUND(4); 
  m68k_SET_DEST_B(areg[PARAM_M]); 
}

void m68k_get_dest_010_w(){
  INSTRUCTION_TIME_ROUND(4);
  m68k_SET_DEST_W(areg[PARAM_M]); 
}

void m68k_get_dest_010_l(){ 
  INSTRUCTION_TIME_ROUND(8);
  m68k_SET_DEST_L(areg[PARAM_M]); 
}

void m68k_get_dest_011_b(){
  INSTRUCTION_TIME_ROUND(4);
  m68k_SET_DEST_B(areg[PARAM_M]);
  areg[PARAM_M]+=1;
  if(PARAM_M==7)
    areg[PARAM_M]++;
}

void m68k_get_dest_011_w(){
  INSTRUCTION_TIME_ROUND(4);
  m68k_SET_DEST_W(areg[PARAM_M]);
  areg[PARAM_M]+=2; 
}

void m68k_get_dest_011_l(){
  INSTRUCTION_TIME_ROUND(8);
  m68k_SET_DEST_L(areg[PARAM_M]);
  areg[PARAM_M]+=4; 
}

void m68k_get_dest_100_b(){ 
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
  INSTRUCTION_TIME_ROUND(6);
  areg[PARAM_M]-=1;
  if(PARAM_M==7)
    areg[PARAM_M]--; 
  m68k_SET_DEST_B(areg[PARAM_M]);
}

void m68k_get_dest_100_w(){ 
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
  INSTRUCTION_TIME_ROUND(6);
  areg[PARAM_M]-=2;
  m68k_SET_DEST_W(areg[PARAM_M]); 
}

void m68k_get_dest_100_l(){
  INSTRUCTION_TIME_ROUND(10);
  areg[PARAM_M]-=4;
  m68k_SET_DEST_L(areg[PARAM_M]);  
}

void m68k_get_dest_101_b(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
  m68k_SET_DEST_B(areg[PARAM_M]+fw);
}

void m68k_get_dest_101_w(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
  m68k_SET_DEST_W(areg[PARAM_M]+fw);
}

void m68k_get_dest_101_l(){
  INSTRUCTION_TIME_ROUND(12);
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
  m68k_SET_DEST_L(areg[PARAM_M]+fw);
}

void m68k_get_dest_110_b(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_B_TO_ADDR
}

void m68k_get_dest_110_w(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_W_TO_ADDR
}

void m68k_get_dest_110_l(){
  INSTRUCTION_TIME_ROUND(14);
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_L_TO_ADDR
}

void m68k_get_dest_111_b(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_B(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();
    pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=4;
#endif
    m68k_SET_DEST_B(fw)
    break;
  }default:
    ILLEGAL;
  }
}

void m68k_get_dest_111_w(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_W(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=4;
#endif
    m68k_SET_DEST_W(fw)
    break;
  }default:
    ILLEGAL;
  }
}

void m68k_get_dest_111_l(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(12);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_L(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=4;
#endif
    m68k_SET_DEST_L(fw)
    break;
  }default:
    ILLEGAL;
  }
}
#if !defined(SSE_CPU_ROUNDING_NO_FASTER_FOR_D)
/*  EXTRA_PREFETCH has been deactivated, it's being taken care of for
    all instructions now, by PREFETCH_IRC
    The negative timings correct extra timing added in instructions like ORI,
    it seems it was to spare some code?
*/

void m68k_get_dest_000_b_faster(){
  INSTRUCTION_TIME(-4);
  m68k_dest=r+PARAM_M; 
}

void m68k_get_dest_000_w_faster(){
  INSTRUCTION_TIME(-4);
  m68k_dest=r+PARAM_M; 
}

void m68k_get_dest_000_l_faster(){
  INSTRUCTION_TIME(-4);
  m68k_dest=r+PARAM_M; 
}

void m68k_get_dest_001_b_faster(){ m68k_dest=areg+PARAM_M; EXTRA_PREFETCH}
void m68k_get_dest_001_w_faster(){ m68k_dest=areg+PARAM_M; EXTRA_PREFETCH}
void m68k_get_dest_001_l_faster(){ m68k_dest=areg+PARAM_M; EXTRA_PREFETCH}
void m68k_get_dest_010_b_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_B(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_010_w_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_W(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_010_l_faster(){ INSTRUCTION_TIME_ROUND(8); m68k_SET_DEST_L(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_011_b_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_B(areg[PARAM_M]); areg[PARAM_M]+=1; if(PARAM_M==7)areg[7]++; EXTRA_PREFETCH}
void m68k_get_dest_011_w_faster(){ INSTRUCTION_TIME_ROUND(4); m68k_SET_DEST_W(areg[PARAM_M]); areg[PARAM_M]+=2; EXTRA_PREFETCH}
void m68k_get_dest_011_l_faster(){ INSTRUCTION_TIME_ROUND(8); m68k_SET_DEST_L(areg[PARAM_M]); areg[PARAM_M]+=4; EXTRA_PREFETCH}
void m68k_get_dest_100_b_faster(){ INSTRUCTION_TIME_ROUND(6); areg[PARAM_M]-=1; if(PARAM_M==7)areg[7]--; m68k_SET_DEST_B(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_100_w_faster(){ INSTRUCTION_TIME_ROUND(6); areg[PARAM_M]-=2; m68k_SET_DEST_W(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_100_l_faster(){ INSTRUCTION_TIME_ROUND(10); areg[PARAM_M]-=4; m68k_SET_DEST_L(areg[PARAM_M]); EXTRA_PREFETCH}
void m68k_get_dest_101_b_faster(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_B(areg[PARAM_M]+fw);
  EXTRA_PREFETCH
}
void m68k_get_dest_101_w_faster(){
  INSTRUCTION_TIME_ROUND(8);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_W(areg[PARAM_M]+fw);
  EXTRA_PREFETCH
}
void m68k_get_dest_101_l_faster(){
  INSTRUCTION_TIME_ROUND(12);
  register signed int fw=(signed short)m68k_fetchW();pc+=2; 
  m68k_SET_DEST_L(areg[PARAM_M]+fw);
  EXTRA_PREFETCH
}
void m68k_get_dest_110_b_faster(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_B_TO_ADDR
  EXTRA_PREFETCH
}
void m68k_get_dest_110_w_faster(){
  INSTRUCTION_TIME_ROUND(10);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_W_TO_ADDR
  EXTRA_PREFETCH
}
void m68k_get_dest_110_l_faster(){
  INSTRUCTION_TIME_ROUND(14);
  m68k_iriwo=m68k_fetchW();pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  m68k_SET_DEST_L_TO_ADDR
  EXTRA_PREFETCH
}
void m68k_get_dest_111_b_faster(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    m68k_SET_DEST_B(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    m68k_SET_DEST_B(fw)
    EXTRA_PREFETCH
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_dest_111_w_faster(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_W(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=4;
#endif
    m68k_SET_DEST_W(fw)
    EXTRA_PREFETCH
    break;
  }default:
    ILLEGAL;
  }
}
void m68k_get_dest_111_l_faster(){
  switch(ir&0x7){
  case 0:{
    INSTRUCTION_TIME_ROUND(12);
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_L(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SSE_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=4;
#endif
    m68k_SET_DEST_L(fw)
    EXTRA_PREFETCH
    break;
  }default:
    ILLEGAL;
  }
}
#endif

// Read from address (v3.5.3)

#if defined(SSE_CPU_INLINE_READ_FROM_ADDR)
// They're not much called, VC6 wouldn't inline them

void TM68000::m68kReadBFromAddr() {
  // Replacing macro m68k_READ_B_FROM_ADDR.
  abus&=0xffffff;
  if(abus>=himem)
  {                                  
    if(abus>=MEM_IO_BASE)
    {            
      if(SUPERFLAG)
        m68k_src_b=io_read_b(abus);           
      else 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);         
    }
    else if(abus>=0xfc0000)
    {                             
      if(tos_high && abus<(0xfc0000+192*1024))
        m68k_src_b=ROM_PEEK(abus-rom_addr);   
      else if (abus<0xfe0000 || abus>=0xfe2000) 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);  
    }
    else if(abus>=MEM_EXPANSION_CARTRIDGE)
    {           
      if(cart)
        m68k_src_b=CART_PEEK(abus-MEM_EXPANSION_CARTRIDGE);  
      else
        m68k_src_b=(BYTE)0xff;
    }
    else if (abus>=rom_addr)
    {                         
      if(abus<(0xe00000+256*1024))
        m68k_src_b=ROM_PEEK(abus-rom_addr);                           
      else if (abus>=0xec0000)
        exception(BOMBS_BUS_ERROR,EA_READ,abus);          
      else
        m68k_src_b=(BYTE)0xff;                                          
    }
    else if (abus>=0xd00000 && abus<0xd80000)
      m68k_src_b=(BYTE)0xff;               
#if !defined(SSE_MMU_NO_CONFUSION)      
    else if(mmu_confused)
      m68k_src_b=mmu_confused_peek(abus,true);                                         
#endif
    else if(abus>=FOUR_MEGS)
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          
    else
      m68k_src_b=(BYTE)0xff;                                          
  }
  else if(abus>=MEM_START_OF_USER_AREA)
  {                                              
    DEBUG_CHECK_READ_B(abus);  
    m68k_src_b=(BYTE)(PEEK(abus));                  
  }
  else if(SUPERFLAG)
  {     
    DEBUG_CHECK_READ_B(abus);  
    m68k_src_b=(BYTE)(PEEK(abus));                  
  }
  else
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}


void TM68000::m68kReadWFromAddr() {
  // Replacing macro m68k_READ_W_FROM_ADDR.
  abus&=0xffffff;                                   
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    
  else if(abus>=himem)
  {                                  
    if(abus>=MEM_IO_BASE)
    {            
      if(SUPERFLAG)
        m68k_src_w=io_read_w(abus);           
      else 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);         
    }
    else if(abus>=0xfc0000)
    {                             
      if(tos_high && abus<(0xfc0000+192*1024))
        m68k_src_w=ROM_DPEEK(abus-rom_addr);   
      else if(abus<0xfe0000 || abus>=0xfe2000) 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);  
    }
    else if(abus>=MEM_EXPANSION_CARTRIDGE)
    {           
      if(cart)
        m68k_src_w=CART_DPEEK(abus-MEM_EXPANSION_CARTRIDGE);  
      else
        m68k_src_w=(WORD)0xffff;                                    
    }
    else if(abus>=rom_addr)
    {                         
      if(abus<(0xe00000+256*1024)) 
        m68k_src_w=ROM_DPEEK(abus-rom_addr);                           
      else if (abus>=0xec0000) 
        exception(BOMBS_BUS_ERROR,EA_READ,abus);          
      else 
        m68k_src_w=(WORD)0xffff;                                          
    }
    else if (abus>=0xd00000 && abus<0xd80000)
      m68k_src_w=(WORD)0xffff;                                          
#if !defined(SSE_MMU_NO_CONFUSION)
    else if(mmu_confused)
      m68k_src_w=mmu_confused_dpeek(abus,true);                                         
#endif
    else if(abus>=FOUR_MEGS)
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          
    else
      m68k_src_w=(WORD)0xffff;                                          
  }
  else if(abus>=MEM_START_OF_USER_AREA)
  {                                              
    DEBUG_CHECK_READ_W(abus);  
    m68k_src_w=DPEEK(abus);                  
  }
  else if(SUPERFLAG)
  {     
    DEBUG_CHECK_READ_W(abus);  
    m68k_src_w=DPEEK(abus);                  
  }
  else 
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}


void TM68000::m68kReadLFromAddr() {
  // Replacing macro m68k_READ_L_FROM_ADDR.
  abus&=0xffffff;                                   
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    
  else if(abus>=himem)
  {                                  
    if(abus>=MEM_IO_BASE)
    {           
      if(SUPERFLAG)
        m68k_src_l=io_read_l(abus);          
      else
        exception(BOMBS_BUS_ERROR,EA_READ,abus);         
    }
    else if(abus>=0xfc0000)
    {                             
      if(tos_high && abus<(0xfc0000+192*1024-2)) 
        m68k_src_l=ROM_LPEEK(abus-rom_addr);   
      else if(abus<0xfe0000 || abus>=0xfe2000)
        exception(BOMBS_BUS_ERROR,EA_READ,abus);  
    }
    else if(abus>=MEM_EXPANSION_CARTRIDGE)
    {           
      if(cart)
        m68k_src_l=CART_LPEEK(abus-MEM_EXPANSION_CARTRIDGE);  
      else
        m68k_src_l=0xffffffff;                                    
    }
    else if(abus>=rom_addr)
    {                         
      if(abus<(0xe00000+256*1024-2)) 
        m68k_src_l=ROM_LPEEK(abus-rom_addr);   
      else if(abus>=0xec0000)
        exception(BOMBS_BUS_ERROR,EA_READ,abus);          
      else
        m68k_src_l=0xffffffff;                                          
    }
    else if (abus>=0xd00000 && abus<0xd80000-2)
      m68k_src_l=0xffffffff;                                          
#if !defined(SSE_MMU_NO_CONFUSION)
    else if (mmu_confused)
      m68k_src_l=mmu_confused_lpeek(abus,true);                                         
#endif
    else if(abus>=FOUR_MEGS)
      exception(BOMBS_BUS_ERROR,EA_READ,abus);                          
    else
      m68k_src_l=0xffffffff;                                          
  }
  else if(abus>=MEM_START_OF_USER_AREA)
  {                                              
    DEBUG_CHECK_READ_L(abus);  
    m68k_src_l=LPEEK(abus);                  
  }
  else if(SUPERFLAG)
  {     
    DEBUG_CHECK_READ_L(abus);  
    m68k_src_l=LPEEK(abus);                  
  }
  else
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}

#endif


// VC6 won't inline this function

void TM68000::SetPC(MEM_ADDRESS ad) {

#if defined(SSE_BOILER_SHOW_INTERRUPT)
  if(ad==rom_addr)
    Debug.InterruptIdx=0;
#endif
//ASSERT(pc!=0x80000);
    pc=ad;                               
    pc_high_byte=pc & 0xff000000;     
    pc&=0xffffff;                    
    lpfetch=lpDPEEK(0); //Default to instant bus error when fetch
    lpfetch_bound=lpDPEEK(0);         
    if (pc>=himem){                                                       
      if (pc<MEM_IO_BASE){           
        if (pc>=MEM_EXPANSION_CARTRIDGE){                                
          if (pc>=0xfc0000){                                                   
            if (tos_high && pc<(0xfc0000+192*1024)){         
              lpfetch=lpROM_DPEEK(pc-0xfc0000); 
              lpfetch_bound=lpROM_DPEEK(192*1024);         
            }                                                
          }else if (cart){                    
            lpfetch=lpCART_DPEEK(pc-MEM_EXPANSION_CARTRIDGE);
            lpfetch_bound=lpCART_DPEEK(128*1024);       
          }                   
        }else if(pc>=rom_addr){            
          if (pc<(0xe00000 + 256*1024)){   
            lpfetch=lpROM_DPEEK(pc-0xe00000);
            lpfetch_bound=lpROM_DPEEK(256*1024);   
          }              
        }  
#if defined(SSE_CPU_FETCH_80000A)
        if(pc>=0x80000 && pc<0x3FFFFF)
        {
          TRACE("fake fetching address for %x\n",pc);
          lpfetch=lpDPEEK(xbios2); 
          lpfetch_bound=lpDPEEK(mem_len+(MEM_EXTRA_BYTES/2)); 
        }
#endif            
      }else{   
        if (pc>=0xff8240 && pc<0xff8260){      
          lpfetch=lpPAL_DPEEK(pc-0xff8240); 
          lpfetch_bound=lpPAL_DPEEK(64+PAL_EXTRA_BYTES);     
        }                                          
      }                                   
    }else{             
      lpfetch=lpDPEEK(pc); 
      lpfetch_bound=lpDPEEK(mem_len+(MEM_EXTRA_BYTES/2));  
    }                        
#if defined(SSE_CPU_CHECK_PC)
    PC=ad;
#endif    
#if defined(SSE_BOILER_PSEUDO_STACK)
    Debug.PseudoStackCheck(ad) ;
#endif
    PrefetchSetPC();
}

#if defined(SSE_INLINE_370)

void TM68000::PrefetchSetPC() { 
  // called by SetPC; we don't count timing here

#if defined(SSE_CPU_FETCH_IO)
  // don't use lpfetch for fetching in IO zone, use io_read: fixes Warp 
  // original STX + Lethal Xcess Beta STX
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    prefetch_buf[0]=io_read_w(pc);
    prefetch_buf[1]=io_read_w(pc+2);
    TRACE("Set PC in IO zone %X\n",pc);
    prefetched_2=true;
    return;
  }
#endif
#if defined(SSE_CPU_FETCH_80000A)
  // don't crash, but the correct value would be 'dbus'
  if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
  {
    prefetch_buf[0]=prefetch_buf[1]=0xFFFF; // default, incorrect?
    prefetched_2=true;
    TRACE("Set PC in empty zone %X\n",pc);
    return;
  }
#endif
  prefetch_buf[0]=*lpfetch;
  prefetch_buf[1]=*(lpfetch+MEM_DIR); 
  prefetched_2=true; // was false in Steem 3.2
#if defined(SSE_CPU_PREFETCH_CALL)
  if(CallPrefetch) 
  {
    ASSERT(PrefetchedOpcode==prefetch_buf[0]);
    prefetch_buf[0]=PrefetchedOpcode;
    CallPrefetch=FALSE;
  }
#endif
  lpfetch+=MEM_DIR;

}

#endif

#if defined(SSE_INLINE_370)
#define LOGSECTION LOGSECTION_CRASH
void TM68000::PrefetchIrc() {

#if defined(SSE_DEBUG) && defined(SSE_CPU_PREFETCH_ASSERT)
  ASSERT(!prefetched_2); // strong, only once per instruction 
#endif

#if !defined(SSE_CPU_FETCH_IO2)
  ASSERT( !(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260)) );
#else 
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    IRC=io_read_w(pc); 
    TRACE_LOG("IRC %X in IO zone %X\n",IRC,pc);
  }
  else
#endif
#if defined(SSE_CPU_FETCH_80000C)
  if(pc>himem && pc>=0x80000 && pc<0x3FFFFF)
  {
    IRC=0xFFFF;
    TRACE("Fetch IRC %X in empty zone %X\n",IRC,pc);
  }
  else
#endif
    IRC=*lpfetch;
  prefetched_2=true;
#if defined(SSE_CPU_PREFETCH_TIMING) // we count fetch timing here
#if defined(SSE_DEBUG) && !(defined(_DEBUG) && defined(DEBUG_BUILD))
  //ASSERT(!NextIrFetched); //strong
  if(NextIrFetched)
  {
    TRACE_LOG("PC %X IR %X double prefetch?\n",pc,ir);
#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK_CPU & OSD_CONTROL_CPUPREFETCH) 
#endif
      TRACE_OSD("IRC 2X FETCH"); // generally false alert at start-up
  }
//#endif
  NextIrFetched=true;
#endif
#if defined(SSE_CPU_PREFETCH_TIMING_EXCEPT)
  if(debug_prefetch_timing(IRD))
    ; else
#endif
    InstructionTimeRound(4);
#endif//SSE_CPU_PREFETCH_TIMING

}

#endif

// VC6 won't inline this function

void TM68000::Interrupt(MEM_ADDRESS ad) {

#if defined(SSE_CPU_UNSTOP)
  M68K_UNSTOP; // fixes trash in Hackabonds Demo intro (STOP in trace)
#endif

  WORD _sr=sr;
  if (!SUPERFLAG) 
    change_to_supervisor_mode();
#if defined(SSE_CPU_PREFETCH_CLASS)
  PrefetchClass=2;
#endif

#if defined(SSE_BOILER_STACK_68030_FRAME)
  if(Debug.M68030StackFrame)
  {//macro must be scoped!
    m68k_PUSH_W(0); // format + offset, both may be 0
  }
#endif

  m68k_PUSH_L(PC32);
#if defined(SSE_BOILER_PSEUDO_STACK)
  Debug.PseudoStackPush(PC32);
#endif
  m68k_PUSH_W(_sr);
  SetPC(ad);
  SR_CLEAR(SR_TRACE);
  interrupt_depth++;
//  TRACE_LOG("%X %d\n",ad,interrupt_depth);
}



////////////////
// Exceptions //
////////////////

#if defined(SSE_CPU_EXCEPTION)

#define LOGSECTION LOGSECTION_CRASH

void m68k_exception::crash() { // copied from cpu.cpp and improved
  DWORD bytes_to_stack=int((bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR)
    ? (4+2+2+4+2):(4+2));
  MEM_ADDRESS sp=(MEM_ADDRESS)(SUPERFLAG 
    ? (areg[7] & 0xffffff):(other_sp & 0xffffff));

#if defined(SSE_CPU_EXCEPTION_FCO)
/*
Table 6-1. Reference Classification
Function Code Output
FC2 FC1 FC0 Address Space
0 0 0 (Undefined, Reserved)*
0 0 1 User Data
0 1 0 User Program
0 1 1 (Undefined, Reserved)*
1 0 0 (Undefined, Reserved)*
1 0 1 Supervisor Data
1 1 0 Supervisor Program
1 1 1 CPU Space
*Address space 3 is reserved for user definition, while 0 and
4 are reserved for future use by Motorola.

  For those cases, we generally get "data" at this point, but "program"
  may be more appropriate.
  

*/
  if(bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR)
  {
    switch(_ir)
    {
    case 0x4e73://rte
    case 0x4e75://rts - Fixes Blood Money -SUP 1MB
      action=EA_FETCH;
    }
    if((_ir&0xF000)==0x6000 //bra etc
      || (_ir&0xFFC0)==0x4EC0) // jmp (?)
    {
      action=EA_FETCH;
    }
  }
#endif


#if defined(SSE_DEBUG)   
  if(M68000.nExceptions!=-1)
  {
    M68000.nExceptions++;  
    TRACE_LOG("\nException #%d, %d bombs (",M68000.nExceptions,bombs);

#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK_CPU & OSD_CONTROL_CPUBOMBS) 
#endif
    TRACE_OSD("%d-%d BOMBS",M68000.nExceptions,bombs);
    switch(bombs)
    {  
    case 2:
      TRACE_LOG("BOMBS_BUS_ERROR"); 
      break;
    case 3:
      TRACE_LOG("BOMBS_ADDRESS_ERROR"); 
      break;
    case 4:
      TRACE_LOG("BOMBS_ILLEGAL_INSTRUCTION"); 
      break;
    case 5:
      TRACE_LOG("BOMBS_DIVISION_BY_ZERO"); 
      break;
    case 6:
      TRACE_LOG("BOMBS_CHK"); 
      break;
    case 7:
      TRACE_LOG("BOMBS_TRAPV"); 
      break;
    case 8:
      TRACE_LOG("BOMBS_PRIVILEGE_VIOLATION"); 
      break;
    case 9:
      TRACE_LOG("BOMBS_TRACE_EXCEPTION"); 
      break;
    case 10:
      TRACE_LOG("BOMBS_LINE_A"); 
      break;
    case 11:
      TRACE_LOG("BOMBS_LINE_F"); 
      break;
    }//sw
#if defined(SSE_CPU_TRUE_PC)
#if defined(DEBUG_BUILD)
    TRACE_LOG(") during \"%s\"\n",exception_action_name[action]);
#else
    TRACE_LOG(") during %s\n",(action==EA_READ||CHECK_READ)?"Read":"Write");
#endif
#else
    TRACE_LOG(") during %s\n",(action==EA_READ)?"Read":"Write (maybe!)");
#endif
#ifdef DEBUG_BUILD 
    EasyStr instr=disa_d2(old_pc); // take advantage of the disassembler
    //TRACE_LOG("PC=%X-IR=%X-Ins: %s -SR=%X-Bus=%X",old_pc,ir,instr.Text,sr,abus);
    TRACE_LOG("PC=%X-IR=%X-Ins: %s -SR=%X-Bus=%X",old_pc,_ir,instr.Text,sr,abus);
#else
    TRACE_LOG("PC=%X-IR=%X-SR=%X-Bus=%X",old_pc,ir,sr,abus);
#endif
    TRACE_LOG("-Vector $%X=%X\n",bombs*4,LPEEK(bombs*4));
    // dump registers
    for(int i=0;i<8;i++) // D0-D7
      TRACE_LOG("D%d=%X ",i,r[i]);
    TRACE_LOG("\n");
    for(int i=0;i<8;i++) // A0-A7 (A7 when the exception occurred)
      TRACE_LOG("A%d=%X ",i,areg[i]);
    TRACE_LOG("\n");
  }

#endif

  if (sp<bytes_to_stack || sp>FOUR_MEGS)
  {
    // Double bus error, CPU halt (we crash and burn)
    // This only has to be done here, m68k_PUSH_ will cause bus error if invalid
    DEBUG_ONLY( log_history(bombs,crash_address) );
    TRACE_LOG("Double bus error SP:%d < bytes to stack %d\n",sp,bytes_to_stack);
    perform_crash_and_burn();
  }
  else
  {
#if defined(SSE_CPU_HALT) && defined(SSE_GUI_STATUS_STRING_HALT)
    M68000.ProcessingState=TM68000::EXCEPTION;
#endif

    //SS:never quite understood this rounding for interrupts thing
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts 
    if(bombs==BOMBS_ILLEGAL_INSTRUCTION || bombs==BOMBS_PRIVILEGE_VIOLATION)
    {
      if(!SUPERFLAG) 
        change_to_supervisor_mode();
      TRACE_LOG("Push crash address %X->%X on %X\n",crash_address,(crash_address & 0x00ffffff) | pc_high_byte,r[15]-4);
      m68k_PUSH_L(( (crash_address) & 0x00ffffff) | pc_high_byte);  // crash address = old_pc
      INSTRUCTION_TIME_ROUND(8);
      TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
      m68k_PUSH_W(_sr); // Status register 
      INSTRUCTION_TIME_ROUND(4); //Round first for interrupts
      MEM_ADDRESS ad=LPEEK(bombs*4); // Get the vector
      if(ad & 1) // bad vector!
      {
        // Very rare, generally indicates emulation/snapshot bug, but there are cases
        bombs=BOMBS_ADDRESS_ERROR;
#if defined(SSE_DEBUG)
//        BRK(odd exception vector); // GEN4-OVR
        M68000.nExceptions++;
        TRACE_LOG("->%d bombs\n",bombs);
#endif
        address=ad;
        action=EA_FETCH;
      }
      else
      {
        TRACE_LOG("PC = %X\n\n",ad);
        M68000.SetPC(ad);
        SR_CLEAR(SR_TRACE);
        INSTRUCTION_TIME_ROUND(22); 
        interrupt_depth++; // Is this necessary?
      }
    }
    if(bombs==BOMBS_BUS_ERROR||bombs==BOMBS_ADDRESS_ERROR)
    {
      if(!SUPERFLAG) 
        change_to_supervisor_mode();
      TRY_M68K_EXCEPTION //TODO: warning C4611
      {

#if defined(SSE_CPU_DETECT_STACK_PC_USE)
        _pc=0x123456; // push garbage!
#elif defined(SSE_CPU_TRUE_PC) 


#if defined(SSE_CPU_TRUE_PC3)
/*  v3.7.1
    Legit fix for Aladin, replacing the 'write on 0' hack.
    Hack confirmed invalid on real STE. Now that I think about it,
    it was a nonsense and dangerous hack!
    CLR.W crashes on 'read' but not in that zone where only
    writing crashes, so it's a logic 'exception' in our 'true pc'
    system for bus error stack frame.
    Suspect other CPU fixes helped remove the hack.
    We test here, not before, for performance.
*/
        if(M68000.CheckRead && abus<MEM_FIRST_WRITEABLE) 
          M68000.Pc=pc+2;
#endif
        if(_pc!=M68000.Pc)
        {

          TRACE_LOG("pc %X true PC %X\n",_pc,M68000.Pc);
          _pc=M68000.Pc; // guaranteed exact...
        }
#endif
#if defined(SSE_CPU_BUS_ERROR_ADDRESS2)
        TRACE_LOG("Push PC %X on %X\n",_pc | pc_high_byte,r[15]-4);
        m68k_PUSH_L(_pc| pc_high_byte);
#else
        TRACE_LOG("Push PC %X on %X\n",_pc,r[15]-4);
        m68k_PUSH_L(_pc);
#endif
        TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
        m68k_PUSH_W(_sr);
        TRACE_LOG("Push IR %X on %X\n",_ir,r[15]-2);
        m68k_PUSH_W(_ir);
#if defined(SSE_CPU_BUS_ERROR_ADDRESS) //no reason to do it only for illegal
        TRACE_LOG("Push crash address %X->%X on %X\n",address,(address & 0x00ffffff) | pc_high_byte,r[15]-4);
        m68k_PUSH_L((address & 0x00ffffff) | pc_high_byte); 
#else
        TRACE_LOG("Push address %X on %X\n",address,r[15]-4);
        m68k_PUSH_L(address);
#endif
        // status
        WORD x=WORD(_ir & 0xffe0); 
        if(action!=EA_WRITE) x|=B6_010000;
        if(action==EA_FETCH)
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_PROGRAM:FC_USER_PROGRAM);
        else
          x|=WORD((_sr & SR_SUPER) ? FC_SUPERVISOR_DATA:FC_USER_DATA);
        TRACE_LOG("Push status %X on %X\n",x,r[15]-2);
        m68k_PUSH_W(x);
      }
      CATCH_M68K_EXCEPTION
      {
        TRACE_LOG("Exception during exception...\n");
#if defined(SSE_CPU_HALT)
        perform_crash_and_burn(); //wait for cases...
#endif
        r[15]=0xf000; // R15=A7  // should be halt?
      }
      END_M68K_EXCEPTION

      TRACE_LOG("PC = %X\n\n",LPEEK(bombs*4));
#if defined(SSE_BOILER_SHOW_INTERRUPT)
      Debug.RecordInterrupt("BOMBS",bombs);
#endif

      SET_PC(LPEEK(bombs*4)); // includes final prefetch

      SR_CLEAR(SR_TRACE);

#if defined(SSE_CPU_FETCH_TIMING)
      INSTRUCTION_TIME_ROUND(50-4); // TODO micro timings?
      FETCH_TIMING;
#if defined(SSE_CPU_PREFETCH_TIMING_SET_PC)
      INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#else
      INSTRUCTION_TIME_ROUND(50); //Round for fetch
#endif
#if defined(SSE_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=2;
#endif
    }
    DEBUG_ONLY(log_history(bombs,crash_address));
  }
#if defined(SSE_CPU_HALT) || defined(SSE_CPU_TRACE_REFACTOR)
  // formalising that there's no trace after a crash
  M68000.ProcessingState=TM68000::NORMAL;
#endif

  PeekEvent(); // Stop exception freeze
}

#undef LOGSECTION


#endif//#if defined(SSE_CPU_EXCEPTION)




//////////////
// Prefetch //
//////////////

#if defined(SSE_CPU_PREFETCH_CALL)

WORD TM68000::FetchForCall(MEM_ADDRESS ad) {
  // fetch PC before pushing return address in JSR, fixes nothing
  ad&=0xffffff; 
  PrefetchAddress=lpDPEEK(0); // default
  if(ad>=himem)
  {
    if(ad<MEM_IO_BASE)
    {           
      if(ad>=MEM_EXPANSION_CARTRIDGE)
      {                                
        if(ad>=0xfc0000)
        {                                                   
          if(tos_high && ad<(0xfc0000+192*1024)) 
            PrefetchAddress=lpROM_DPEEK(ad-0xfc0000); 
        }
        else if(cart)
          PrefetchAddress=lpCART_DPEEK(ad-MEM_EXPANSION_CARTRIDGE); 
      }
      else if(ad>=rom_addr)
      {                                                      
        if(ad<(0xe00000 + 256*1024))
          PrefetchAddress=lpROM_DPEEK(ad-0xe00000); 
      }                            
    }
    else
    {   
      if(ad>=0xff8240 && ad<0xff8260)      
      {
        BRK(set PC to palette!);// impossible?
        PrefetchAddress=lpPAL_DPEEK(ad-0xff8240);
      }
    }                                                                           
  }
  else //  in normal ram
    PrefetchAddress=lpDPEEK(ad);
  prefetched_2=false; // will have prefetched 1 word / cancels eventual prefetch
  PrefetchedOpcode=*(PrefetchAddress); // fetch instruction!
  CallPrefetch=TRUE;
  return PrefetchedOpcode;
}

#endif


/////////////////////////////////////////////
// Poke, non inlined part (full functions) //
/////////////////////////////////////////////


#if defined(SSE_CPU_POKE)

#if defined(SSE_INLINE_370) 

#elif defined(DEBUG_BUILD)

NOT_DEBUG(inline) void m68k_poke_abus(BYTE x){
  abus&=0xffffff; // annoying that we must do this
  BOOL super=SUPERFLAG;
  if(abus>=MEM_IO_BASE && super)
    io_write_b(abus,x);
#if defined(SSE_CPU_CHECK_VIDEO_RAM_B)
/*  To save some performance, we do just one basic Shifter test in the inline
    part. More precise test is in m68k_poke_abus2().
*/
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
  {
#if defined(SSE_BOILER_MONITOR_VALUE2)
    PEEK(abus)=x;
    DEBUG_CHECK_WRITE_B(abus);
#else
    DEBUG_CHECK_WRITE_B(abus);
    PEEK(abus)=x;
#endif
  }
  else 
    m68k_poke_abus2(x);
}
 

NOT_DEBUG(inline) void m68k_dpoke_abus(WORD x){
  abus&=0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1) 
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE && super)
      io_write_w(abus,x);
#if defined(SSE_CPU_CHECK_VIDEO_RAM_W) // 3615 GEN4 100
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
  {
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DPEEK(abus)=x;
    DEBUG_CHECK_WRITE_W(abus);
#else
    DEBUG_CHECK_WRITE_W(abus);
    DPEEK(abus)=x;
#endif
  }
  else
    m68k_dpoke_abus2(x);
}


NOT_DEBUG(inline) void m68k_lpoke_abus(LONG x){
  abus&=0xffffff;
  BOOL super=SUPERFLAG;
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else 
  if(abus>=MEM_IO_BASE && super)
    io_write_l(abus,x);
#if defined(SSE_CPU_CHECK_VIDEO_RAM_L)
  else if(abus<shifter_draw_pointer_at_start_of_line && abus<himem
    && (abus>=MEM_START_OF_USER_AREA ||super && abus>=MEM_FIRST_WRITEABLE))
#else
  else if(abus<himem && (abus>=MEM_START_OF_USER_AREA
    ||super && abus>=MEM_FIRST_WRITEABLE))
#endif
  {
#if defined(SSE_BOILER_MONITOR_VALUE4)//3.6.1, also for long of course (argh!)
    LPEEK(abus)=x;
    DEBUG_CHECK_WRITE_L(abus);
#else
    DEBUG_CHECK_WRITE_L(abus);
    LPEEK(abus)=x;
#endif
  }
  else
    m68k_lpoke_abus2(x);
}

#endif


#if defined(SSE_INLINE_370) 
void m68k_poke_abus(BYTE x){
#else
void m68k_poke_abus2(BYTE x){
#endif
  abus&=0xffffff;
  if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_b(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SSE_MMU_NO_CONFUSION)
    if (mmu_confused){
      mmu_confused_set_dest_to_addr(1,true);
      m68k_DEST_B=x;
#if defined(SSE_MMU_TRACE2)
      TRACE_LOG("MMU %X: poke %X=%X\n",old_pc,m68k_DEST_B,x);
#endif
    }else 
#endif      
    if (abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
#if !defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_B(abus);
#endif
#if defined(SSE_CPU_CHECK_VIDEO_RAM_B)
/*  If we're going to write in video RAM of the current scanline,
    we check whether we need to render before. Some programs write
    just after the memory has been fetched, but Steem renders at
    Shifter events, and if nothing happens, at the end of the line.
    So if we do nothing it will render wrong memory.
    The test isn't perfect and will cause some "false alerts" but
    we have performance in mind: CPU poke is used a lot, it is rare
    when the address bus is around the current scanline.
*/
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
    if(Glue.FetchingLine()
#else
    if(Shifter.FetchingLine() 
#endif
      && abus>=shifter_draw_pointer
      && abus<shifter_draw_pointer_at_start_of_line+LINECYCLES/2)
      Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
    if (SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      PEEK(abus)=x;
    else if (abus>=MEM_START_OF_USER_AREA)
      PEEK(abus)=x;
#if defined(SSE_CPU_IGNORE_WRITE_B_0) //undef v3.7.1
    else if(SUPERFLAG && abus==0 ) 
      ;
#endif
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_B(abus);
#endif
  }
}

#if defined(SSE_INLINE_370) 
void m68k_dpoke_abus(WORD x){
#else
void m68k_dpoke_abus2(WORD x){
#endif
  abus&=0xffffff;
  if(abus&1) exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_w(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SSE_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(2,true);
      m68k_DEST_W=x;
#if defined(SSE_MMU_TRACE2)
      TRACE_LOG("MMU %X: dpoke %X=%X\n",old_pc,m68k_DEST_W,x);
#endif
    }else 
#endif
#if defined(SSE_CPU_IGNORE_RW_4MB)//undef 3.7
    if(abus>FOUR_MEGS || abus==FOUR_MEGS&&mem_len<FOUR_MEGS){
#else
    if(abus>=FOUR_MEGS){
#endif
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
#if !defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_W(abus);
#endif
#if defined(SSE_CPU_CHECK_VIDEO_RAM_W) // 3615 GEN4
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
    if(Glue.FetchingLine()
#else
    if(Shifter.FetchingLine() 
#endif
      && abus>=shifter_draw_pointer
      && abus<shifter_draw_pointer_at_start_of_line+LINECYCLES/2
      //&& DPEEK(abus)!=x
      )
      Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      DPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      DPEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
#if defined(SSE_BOILER_MONITOR_VALUE2)
    DEBUG_CHECK_WRITE_W(abus);
#endif

  }
}

#if defined(SSE_INLINE_370) 
void m68k_lpoke_abus(LONG x){
#else
void m68k_lpoke_abus2(LONG x){
#endif
  abus&=0xffffff;
  if(abus&1)exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_l(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SSE_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(4,true);
      m68k_DEST_L=x;
#if defined(SSE_MMU_TRACE2)
      TRACE_LOG("MMU %X: lpoke %X=%X\n",old_pc,m68k_DEST_L,x);
#endif
    }else 
#endif
    if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
#if !defined(SSE_BOILER_MONITOR_VALUE4) //??
/////#if defined(SSE_BOILER_MONITOR_VALUE2) //3.7.2 bugfix ?!
    DEBUG_CHECK_WRITE_L(abus);
#endif
#if defined(SSE_CPU_CHECK_VIDEO_RAM_L)
#if defined(SSE_MOVE_SHIFTER_CONCEPTS_TO_GLUE1)
    if(Glue.FetchingLine()
#else
    if(Shifter.FetchingLine() 
#endif
      && abus>=shifter_draw_pointer
      && abus<shifter_draw_pointer_at_start_of_line+LINECYCLES/2)
      Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      LPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      LPEEK(abus)=x;
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
//#if !defined(SSE_BOILER_MONITOR_VALUE2)
#if defined(SSE_BOILER_MONITOR_VALUE2) //3.7.2 bugfix?
    DEBUG_CHECK_WRITE_L(abus);
#endif

  }
}

#endif


// E-clock

#if defined(SSE_CPU_E_CLOCK)
/*  
"Enable (E)
This signal is the standard enable signal common to all M6800 Family peripheral
devices. A single period of clock E consists of 10 MC68000 clock periods (six clocks
low, four clocks high). This signal is generated by an internal ring counter that may
come up in any state. (At power-on, it is impossible to guarantee phase relationship of E
to CLK.) The E signal is a free-running clock that runs regardless of the state of the
MPU bus."

  The ambition of this routine is to get correct timings
  for ACIA, HBL and VBL interrupts..
  It is based on this table by Nyh:
http://www.atari-forum.com/viewtopic.php?f=68&p=254097&sid=3f96c5838329b68260a38e4b388b4561#p254097
CPU-Clock  E-clock Keyboard read
  000000   00000.0     28
  160256   16025.6     20
  320512   32051.2     24
  480768   48076.8     20
  641024   64102.4     24
  801280   80128.0     28
  961536   96153.6     20
 1121792  112179.2     24
 1282048  128204.8     20
 1442304  144230.4     24


  Compare with jitter tables in Hatari:
 
  int HblJitter[] = {8,4,4,0,0}; 
  int VblJitter[] = {8,0,4,0,4}; 

  Sync with E-clock is more precise emulation.

  Cases:
  3615GEN4 HMD #1
  Anomaly
  Demoniak
  Krig
  NOJITTER.PRG
  Reality is a Lie/Schnusdie
  TCB

  It depends on option 6301/Acia, for ACIA as well as HBI & VBI.
  If the option isn't checked, we fall back on Steem's wobble.
*/


#define LOGSECTION LOGSECTION_INTERRUPTS

#if defined(SSE_CPU_E_CLOCK2)
/*  Now the function returns # E-clock cycles, without changing CPU
    cycles itself.
*/
int 
#else
void
#endif
TM68000::SyncEClock(
#if defined(SSE_CPU_E_CLOCK_DISPATCHER)
                    int dispatcher
#endif
                    ) {
                      
#if defined(SSE_CPU_E_CLOCK2)
  BYTE wait_states=0;
#endif

  // sync with E max once per instruction
  if(EClock_synced) 
#if defined(SSE_CPU_E_CLOCK2)
    return wait_states;
#else
    return;
#endif
  EClock_synced=true; 

  int act=ACT;

#if defined(SSE_SHIFTER) && defined(SSE_TIMINGS_FRAME_ADJUSTMENT)//no
  act-=4*Shifter.n508lines; //legit hack: NOJITTER.PRG
#endif
#if defined(SSE_CPU_E_CLOCK3)
/*
  int a=2147483644;
  a+=8; //4 2
  a+=2; //2 4
  a+=2; //0 6
  a+=2; //8 8
  a+=2; //6 0
  a+=2; //4 2
*/
  BYTE cycles=abs(act%10);
  if(act<0 && cycles!=8)
    cycles=6-cycles;
#endif

#if !defined(SSE_CPU_E_CLOCK2)
  BYTE wait_states;
#endif

#if defined(SSE_CPU_E_CLOCK3)
  switch(cycles) {
#else
  switch(abs(act%10)) {
#endif

  case 0:
    wait_states=8;
#if defined(SSE_INT_HBL_E_CLOCK_HACK)
    // pathetic hack for 3615GEN4 HMD #1 and 3615GEN4-OVR, make it 4
    // cycles instead on HBI of STF
    // TEST16, HBITMG: jitter is 0, 4, 8, timing is 56 
    // -> we know it's not correct...
    if(dispatcher==ECLOCK_HBL&&ST_TYPE==STF&&SSE_HACKS_ON)
      TRACE_LOG("ECLK 8->4\n");
    else
#endif
    break;
  case 2:
  case 4: 
    wait_states=4;
    break;
  default: //6,8
    wait_states=0;
  }//sw

#if !defined(SSE_CPU_E_CLOCK2)
  InstructionTime(wait_states); 
#endif
#if defined(SSE_DEBUG) 
  char* sdispatcher[]={"ACIA","HBL","VBL"};
#if defined(SSE_DEBUG_FRAME_REPORT_ACIA)
  FrameEvents.Add(scan_y,LINECYCLES,'E',wait_states);
#endif
#if defined(SSE_DEBUG_FRAME_REPORT) && defined(SSE_BOILER_FRAME_REPORT_MASK)
#if defined(SSE_BOILER_FRAME_REPORT_MASK2)
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_INT)
#else
  if(FRAME_REPORT_MASK2 & FRAME_REPORT_MASK_ACIA)
#endif
    FrameEvents.Add(scan_y,LINECYCLES,'E',wait_states);
#endif
#if defined(SSE_BOILER_TRACE_CONTROL)
  if(wait_states && (TRACE_MASK2&TRACE_CONTROL_ECLOCK)) 
#if defined(SSE_CPU_E_CLOCK_DISPATCHER)
    TRACE_LOG("F%d y%d c%d %s E-Clock +%d\n",TIMING_INFO,sdispatcher[dispatcher],wait_states);
#else
    TRACE_LOG("F%d y%d c%d E-Clock +%d\n",TIMING_INFO,wait_states);
#endif
#endif
#endif//dbg
#if defined(SSE_CPU_E_CLOCK2)
#if defined(SSE_CPU_E_CLOCK_DISPATCHER)
  LastEClockCycles[dispatcher]=wait_states;
#endif
  return wait_states;
#endif
}

#undef LOGSECTION

#endif//clockE




/////////////////////////
// Redone instructions //
/////////////////////////



#if defined(SSE_CPU_DIV) 

// using ijor's timings (in 3rdparty\pasti), what they fix proves them correct
//#undef SSE_CPU_LINE_8_TIMINGS
void                              m68k_divu(){
#if !defined(SSE_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){ // div by 0
    // Clear V flag when dividing by zero. 
    SR_CLEAR(SR_V);
#if defined(SSE_CPU_DIV_CC)
    SR_CLEAR(SR_C);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("DIV");
#endif
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    PREFETCH_IRC; // TODO: at the end?
#if defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    unsigned long q;
    unsigned long dividend = (unsigned long) (r[PARAM_N]);
    unsigned short divisor = (unsigned short) m68k_src_w;
    int cycles_for_instr=getDivu68kCycles(dividend,divisor) -4; // -prefetch
    INSTRUCTION_TIME(cycles_for_instr); // fixes Pandemonium loader
    q=(unsigned long)((unsigned long)dividend)/(unsigned long)((unsigned short)divisor);
    if(q&0xffff0000){
      SR_SET(SR_V);
#if defined(SSE_CPU_DIVU_OVERFLOW)
/*  
"N : Set if the quotient is negative; cleared otherwise; undefined if overflow 
  or divide by zero occurs."
    Setting it if overflow fixes Sadeness and Spectrum Analyzer by Axxept.
    (hex) 55667788/55=349B+101 strange, neither the dividend nor the quotient
    are negative.
    The value of SR is used by the trace protection decoder of those demos.
    But Z must be cleared.
    v3.7: 
    C — Always cleared, official doc, this was forgotten in case of overflow
*/
      SR_SET(SR_N);
#endif
#if defined(SSE_CPU_DIV_CC)
      SR_CLEAR(SR_C);// fixes Spacker II
#endif
    }else{
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
      r[PARAM_N]=((((unsigned long)r[PARAM_N])%((unsigned short)m68k_src_w))<<16)+q;
    }
  }
}


void                              m68k_divs(){
#if !defined(SSE_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif

  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){
    /* Clear V flag when dividing by zero - Alcatraz Odyssey demo depends
     * on this (actually, it's doing a DIVS).  
    SS: it's an Amiga demo. 
    */
    SR_CLEAR(SR_V);
#if defined(SSE_CPU_DIV_CC)
    SR_CLEAR(SR_C);
#endif
#if defined(SSE_BOILER_SHOW_INTERRUPT)
    Debug.RecordInterrupt("DIV");
#endif
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    PREFETCH_IRC;
#if defined(SSE_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    signed long q;
    signed long dividend = (signed long) (r[PARAM_N]);
    signed short divisor = (signed short) m68k_src_w;
    int cycles_for_instr=getDivs68kCycles(dividend,divisor)-4; // -prefetch
    INSTRUCTION_TIME(cycles_for_instr);   // fixes Dragonnels loader
#if defined(SSE_CPU_DIVS_OVERFLOW_PC)
    ASSERT(divisor);
#if defined(BCC_BUILD) || (defined(VC_BUILD) && _MSC_VER < 1500) 
    ASSERT(dividend!=(-2147483647 - 1)); // X86 crashes on div overflow
    if(dividend==(-2147483647 - 1))
#else
    ASSERT(dividend!=INT_MIN); // X86 crashes on div overflow
    if(dividend==INT_MIN)
#endif
      q=dividend;//-32768-1; 
    else
#endif
    q=(signed long)((signed long)dividend)/(signed long)((signed short)divisor);
    if(q<-32768 || q>32767){
      SR_SET(SR_V);
#if defined(SSE_CPU_DIVS_OVERFLOW)
      SR_SET(SR_N); // maybe, see SSE_CPU_DIVU_OVERFLOW
#endif
#if defined(SSE_CPU_DIV_CC)
      SR_CLEAR(SR_C);
#endif  
    }else{
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
      r[PARAM_N]=((((signed long)r[PARAM_N])%((signed short)m68k_src_w))<<16)|((long)LOWORD(q));
    }
  }
}
//#define SSE_CPU_LINE_8_TIMINGS
#endif

/*
MOVE instructions. Most variants, except the ones noted below

 

1)      Perform as many prefetch cycles as extension words are in the source 
operand (optional).

2)      Read source operand (optional if source is register).

3)      Perform as many prefetch cycles as extension words are in the 
destination operand (optional).

4)      Writes memory operand.

5)      Perform last prefetch cycle.

 

Net effect is that MOVE instructions are of class 1.

 

 

MOVE <ea>,-(An)

 

When the destination addressing mode is pre-decrement, steps 4 and 5 above are 
inverted. So it behaves like a read modify instruction and it is a class 0 
instruction.

 

Note: The behavior is the same disregarding transfer size (byte, word or long), 
and disregarding the source addressing mode.

 

 

MOVE memory,(xxx).L

 

When the destination addressing mode is long absolute and the source operand is 
any memory addr.mode, step 4 is interleaved in the middle of step 3. Step 3 
only performs a single prefetch in this case. The other prefetch cycle that is 
normally performed at that step is deferred after the write cycles.

 

So, two prefetch cycles are performed after the write ones. It is a class 2 
instruction.

 

Note: The behavior is the same disregarding transfer size (byte, word or long). 
But if the source operand is a data or address register, or immediate, then the 
behavior is the same as other MOVE variants (class 1 instruction).

*/

#if defined(SSE_CPU_MOVE_B)

void m68k_0001() {  // move.b

//ASSERT(old_pc!=0x4532e);

#if (!defined(SSE_CPU_LINE_3_TIMINGS) && !defined(SSE_CPU_PREFETCH_TIMING)) \
  || !defined(SSE_CPU_PREFETCH_MOVE_MEM)
  
#if defined(SSE_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000)
    FETCH_TIMING_NO_ROUND; // it's the same, but recorded as fetch timing
  else
#endif
    INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SSE_CPU_POST_INC) || defined(SSE_CPU_PRE_DEC)
  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis
#endif

#if defined(SSE_CPU_ASSERT_ILLEGAL)
/* In this case we consider ILLEGAL before bus/address error.
   Fixes Transbeauce 2 loader, Titan, Crazy Cars 2
   The CPU logic is to always assert illegal before execution, emulating
   this could be interesting but would take a hit on performance.
*/
  if( (ir&BITS_876)==BITS_876_001
    || (ir&BITS_876)==BITS_876_111
    && (ir&BITS_ba9)!=BITS_ba9_000
    && (ir&BITS_ba9)!=BITS_ba9_001 
#if defined(SSE_CPU_ASSERT_ILLEGAL2)
/* v3.7
     "For byte size operation, address register direct is not allowed."
     ->  move.b EA=an is illegal
	 Note: this EA is used by Blood Money -SUP, by coincidence crashing
	 would make the game fail, but this was because of how subsequent
	 exceptions should be handled, see SSE_CPU_EXCEPTION_FCO.
*/
    || (ir&BITS_543)==BITS_543_001 // source = An
#endif
    )
    m68k_unrecognised();
#endif

  // Source
  m68k_GET_SOURCE_B;

  // Destination
#if defined(SSE_CPU_TRUE_PC)
  TRUE_PC=pc+2;
#endif
  if((ir&BITS_876)==BITS_876_000)
  { // Dn
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_B=m68k_src_b; // move completed
    SR_CHECK_Z_AND_N_B; // update flags
#if defined(SSE_CPU_LINE_1_TIMINGS) 
    FETCH_TIMING_NO_ROUND;
#endif
  }
#if !defined(SSE_CPU_ASSERT_ILLEGAL)
  else if((ir&BITS_876)==BITS_876_001)
    m68k_unrecognised();
#endif
  else
  {   //to memory
    bool refetch=false;
    switch(ir&BITS_876)
    {
    case BITS_876_010: // (An)
#if (defined(SSE_CPU_LINE_1_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011: // (An)+
#if (defined(SSE_CPU_LINE_1_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
#if defined(SSE_CPU_POST_INC)
      UpdateAn=(PARAM_N==7)?2:1;
#else
      areg[PARAM_N]++; 
      if(PARAM_N==7)
        areg[7]++;
#endif
      break;
    case BITS_876_100: // -(An)
#if defined(SSE_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC;
      FETCH_TIMING;
#endif  
#if (defined(SSE_CPU_LINE_1_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_PRE_DEC)
      UpdateAn=(PARAM_N==7)?-2:-1;
      abus=areg[PARAM_N]+UpdateAn;
#else
      areg[PARAM_N]--;
      if(PARAM_N==7)
        areg[PARAM_N]--;
      abus=areg[PARAM_N];
#endif
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(12-4-4);
#if (defined(SSE_CPU_LINE_1_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(14-4-4);
#if (defined(SSE_CPU_LINE_1_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

      m68k_iriwo=m68k_fetchW();pc+=2; 
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_876_111:
      if (SOURCE_IS_REGISTER_OR_IMMEDIATE==0) refetch=true;
      switch (ir & BITS_ba9){
      case BITS_ba9_000: // (xxx).W
        INSTRUCTION_TIME(12-4-4);
#if (defined(SSE_CPU_LINE_1_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
        INSTRUCTION_TIME(4);
#endif
        abus=0xffffff & (unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case BITS_ba9_001: // (xxx).L
#if defined(SSE_CPU_PREFETCH_CLASS)
        if(refetch)
          M68000.PrefetchClass=2; // only .L
#endif
        INSTRUCTION_TIME(16-4-4);
#if (defined(SSE_CPU_LINE_1_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
        INSTRUCTION_TIME(4);
#endif

        abus=m68k_fetchL() & 0xffffff;
        pc+=4;  
        break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL)
      default:
        m68k_unrecognised();
#endif
      }
    }
    // Set flags
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if(!m68k_src_b){
      SR_SET(SR_Z);
    }
    if(m68k_src_b&MSB_B){
      SR_SET(SR_N);
    }

#if defined(SSE_CPU_DATABUS)
    M68000.dbus|=m68k_src_b;
#endif

    m68k_poke_abus(m68k_src_b); // write; could crash

#if defined(SSE_CPU_POST_INC) || defined(SSE_CPU_PRE_DEC)
    areg[PARAM_N]+=UpdateAn; // can be 0,-1,-2,+1,+2
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
    if(M68000.PrefetchClass==2)
    {
      REFETCH_IR;
      PREFETCH_IRC;
    }
    if(M68000.PrefetchClass) // -(An), already fetched
      FETCH_TIMING;
#else
    if (refetch) 
      prefetch_buf[0]=*(lpfetch-MEM_DIR);
    FETCH_TIMING;  
#endif  
  }// to memory
#if defined(SSE_CPU_PREFETCH_CLASS)
  if(M68000.PrefetchClass==1) // classes 0 & 2 already handled
  {
#if !defined(SSE_CPU_ROUNDING_MOVE)
    if((ir&BITS_876)==BITS_876_000)
      PREFETCH_IRC_NO_ROUND;
    else
#endif
      PREFETCH_IRC;
  }
#endif

}

#endif

#if defined(SSE_CPU_MOVE_L)

void m68k_0010()  //move.l
{

#if (!defined(SSE_CPU_LINE_3_TIMINGS) && !defined(SSE_CPU_PREFETCH_TIMING)) \
  || !defined(SSE_CPU_PREFETCH_MOVE_MEM)
#if defined(SSE_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000
    ||(ir & BITS_876)==BITS_876_001)
    FETCH_TIMING_NO_ROUND;
  else
#endif
  INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SSE_CPU_POST_INC) || defined(SSE_CPU_PRE_DEC)
  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis
#endif

#if defined(SSE_CPU_ASSERT_ILLEGAL)
  if( (ir&BITS_876)==BITS_876_111
    && (ir&BITS_ba9)!=BITS_ba9_000
    && (ir&BITS_ba9)!=BITS_ba9_001 )
      m68k_unrecognised();
#endif

  m68k_GET_SOURCE_L; // where tb2 etc (used to) crash

  // Destination
#if defined(SSE_CPU_TRUE_PC)
  TRUE_PC=pc+2; // eg. War Heli move.l d5,(a1)
#endif
  if ((ir & BITS_876)==BITS_876_000) // Dn
  { 
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_L=m68k_src_l;
    SR_CHECK_Z_AND_N_L;
#if defined(SSE_CPU_LINE_2_TIMINGS)
    FETCH_TIMING_NO_ROUND;
#endif

  }
  else if ((ir & BITS_876)==BITS_876_001) // An
  {
    areg[PARAM_N]=m68k_src_l; // MOVEA
#if defined(SSE_CPU_LINE_2_TIMINGS) 
    FETCH_TIMING_NO_ROUND;
#endif
  }
  else
  {   //to memory
    bool refetch=0;
    switch(ir&BITS_876)
    {
    case BITS_876_010: // (An)
      INSTRUCTION_TIME(12-4-4);
#if (defined(SSE_CPU_LINE_2_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011: // (An)+
      INSTRUCTION_TIME(12-4-4);
#if (defined(SSE_CPU_LINE_2_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
#if defined(SSE_CPU_POST_INC)
      UpdateAn=4;
#else
      areg[PARAM_N]+=4;
#endif
      break;
    case BITS_876_100: // -(An)
      INSTRUCTION_TIME(12-4-4);
#if (defined(SSE_CPU_LINE_2_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC;
      FETCH_TIMING;
#endif  
#if defined(SSE_CPU_PRE_DEC)
      UpdateAn=-4;
      abus=areg[PARAM_N]+UpdateAn;
#else
      areg[PARAM_N]-=4;
      abus=areg[PARAM_N];
#endif
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(16-4-4);
#if (defined(SSE_CPU_LINE_2_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(18-4-4);
#if (defined(SSE_CPU_LINE_2_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

      m68k_iriwo=m68k_fetchW();pc+=2; 
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_876_111:
      if (SOURCE_IS_REGISTER_OR_IMMEDIATE==0) refetch=true;
      switch(ir&BITS_ba9){
      case BITS_ba9_000: // (xxx).W
        INSTRUCTION_TIME(16-4-4);
#if (defined(SSE_CPU_LINE_2_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case BITS_ba9_001: // (xxx).L
        INSTRUCTION_TIME(20-4-4);
#if (defined(SSE_CPU_LINE_2_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
        INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
        if(refetch) 
          M68000.PrefetchClass=2; 
#endif 
        abus=m68k_fetchL()&0xffffff;
        pc+=4;  
        break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL)
      default:
        m68k_unrecognised();
#endif
      }
    }
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if(!m68k_src_l){
      SR_SET(SR_Z);
    }
    if(m68k_src_l&MSB_L){
      SR_SET(SR_N);
    }

#if defined(SSE_CPU_DATABUS)
    M68000.dbus=m68k_src_l&0xFFFF; // 2nd part?
#endif

    m68k_lpoke_abus(m68k_src_l);

#if defined(SSE_CPU_POST_INC) || defined(SSE_CPU_PRE_DEC)
    areg[PARAM_N]+=UpdateAn; // can be 0,-4,+4
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
    if(M68000.PrefetchClass==2)
    {
      REFETCH_IR;
      PREFETCH_IRC;
    }
    if(M68000.PrefetchClass) // -(An), already fetched
      FETCH_TIMING;
#else
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
    FETCH_TIMING;  // move fetches after instruction
#endif  

  }// to memory
#if defined(SSE_CPU_PREFETCH_CLASS)
  if(M68000.PrefetchClass==1)
  {
#if !defined(SSE_CPU_ROUNDING_MOVE)
    if((ir&BITS_876)==BITS_876_000 || (ir & BITS_876)==BITS_876_001)
      PREFETCH_IRC_NO_ROUND;
    else
#endif
      PREFETCH_IRC;
  }
#endif

}

#endif

#if defined(SSE_CPU_MOVE_W)

void m68k_0011() //move.w
{
#if (!defined(SSE_CPU_LINE_3_TIMINGS) && !defined(SSE_CPU_PREFETCH_TIMING)) \
  || !defined(SSE_CPU_PREFETCH_MOVE_MEM)
#if defined(SSE_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000
    ||(ir & BITS_876)==BITS_876_001)
    FETCH_TIMING_NO_ROUND;
  else
#endif
  INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SSE_CPU_POST_INC) || defined(SSE_CPU_PRE_DEC)
  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis
#endif

#if defined(SSE_CPU_ASSERT_ILLEGAL)
    if( (ir&BITS_876)==BITS_876_111
      && (ir&BITS_ba9)!=BITS_ba9_000
      && (ir&BITS_ba9)!=BITS_ba9_001 )
      m68k_unrecognised();
#endif

  m68k_GET_SOURCE_W;

  // Destination
#if defined(SSE_CPU_TRUE_PC)
  TRUE_PC=pc+2;
#endif
  if ((ir & BITS_876)==BITS_876_000) // Dn
  {
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_W=m68k_src_w;
    SR_CHECK_Z_AND_N_W;
#if defined(SSE_CPU_LINE_3_TIMINGS) 
    FETCH_TIMING_NO_ROUND;
#endif
  }
  else if ((ir & BITS_876)==BITS_876_001) // An
  {
    areg[PARAM_N]=(signed long)((signed short)m68k_src_w); // movea
#if defined(SSE_CPU_LINE_3_TIMINGS)
    FETCH_TIMING_NO_ROUND;
#endif
  }
  else
  {   //to memory
    bool refetch=0;
    switch (ir & BITS_876)
    {
    case BITS_876_010: // (An)
#if (defined(SSE_CPU_LINE_3_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011:  // (An)+
#if (defined(SSE_CPU_LINE_3_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
#if defined(SSE_CPU_POST_INC)
      UpdateAn=2; // fixes Beyond loader
#else
      areg[PARAM_N]+=2;
#endif
      break;
    case BITS_876_100: // -(An)
#if (defined(SSE_CPU_LINE_3_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
#if defined(SSE_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=0; 
#if defined(SSE_CPU_PREFETCH_4PIXEL_RASTERS)
/*  This mod was already present in v3.3, when switch SSE_CPU_PREFETCH_TIMING
    wasn't defined yet, but this is a timing fix.
    Fixes 4pixel Rasters demo
*/
      PREFETCH_IRC; 
      FETCH_TIMING;
#endif
#endif  
#if defined(SSE_CPU_PRE_DEC)
      UpdateAn=-2;
      abus=areg[PARAM_N]+UpdateAn;
#else
      areg[PARAM_N]-=2;
      abus=areg[PARAM_N];
#endif
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(12-4-4);
#if (defined(SSE_CPU_LINE_3_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(14-4-4);
#if (defined(SSE_CPU_LINE_3_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

      m68k_iriwo=m68k_fetchW();pc+=2; 
      if(m68k_iriwo&BIT_b){  //.l
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
      }else{         //.w
        abus=areg[PARAM_N]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
      }
      break;
    case BITS_876_111:
      if (SOURCE_IS_REGISTER_OR_IMMEDIATE==0) refetch=true;
      switch (ir & BITS_ba9){
      case BITS_ba9_000: // (xxx).W
        INSTRUCTION_TIME(12-4-4);
#if (defined(SSE_CPU_LINE_3_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case BITS_ba9_001: // (xxx).L
        INSTRUCTION_TIME(16-4-4);
#if (defined(SSE_CPU_LINE_3_TIMINGS) || defined(SSE_CPU_PREFETCH_TIMING)) \
  && defined(SSE_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
        if(refetch) 
          M68000.PrefetchClass=2; 
#endif  
        abus=m68k_fetchL()&0xffffff;
        pc+=4;  
        break;
#if !defined(SSE_CPU_ASSERT_ILLEGAL)
      default:
        m68k_unrecognised();
#endif
      }
    }
    SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
    if (!m68k_src_w){
      SR_SET(SR_Z);
    }
    if (m68k_src_w & MSB_W){
      SR_SET(SR_N);
    }

#if defined(SSE_CPU_DATABUS)
//    M68000.dbus=m68k_src_w;
#endif

    m68k_dpoke_abus(m68k_src_w);

#if defined(SSE_CPU_POST_INC) || defined(SSE_CPU_PRE_DEC)
    areg[PARAM_N]+=UpdateAn; // can be 0,-2,+2
#endif

#if defined(SSE_CPU_PREFETCH_CLASS)
    if(M68000.PrefetchClass==2)
    {
      REFETCH_IR;
      PREFETCH_IRC;
    }
    if(M68000.PrefetchClass) // -(An), already fetched
      FETCH_TIMING;
#else
    if (refetch) prefetch_buf[0]=*(lpfetch-MEM_DIR);
    FETCH_TIMING;  // move fetches after instruction
#endif  
  }// to memory
#if defined(SSE_CPU_PREFETCH_CLASS)
  if(M68000.PrefetchClass==1)
  {
#if !defined(SSE_CPU_ROUNDING_MOVE)
    if((ir&BITS_876)==BITS_876_000 || (ir & BITS_876)==BITS_876_001)
      PREFETCH_IRC_NO_ROUND;
    else
#endif
      PREFETCH_IRC;
  }

#endif
}

#endif

#endif//#if defined(SSE_CPU)
