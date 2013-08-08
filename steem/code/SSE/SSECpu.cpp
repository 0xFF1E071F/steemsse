#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_INFO)
#pragma message("Included for compilation: SSECpu.cpp")
#endif

#if defined(SS_CPU)

TM68000 M68000; // singleton

TM68000::TM68000() {
  Reset(true);
}


void TM68000::Reset(bool cold) {
#if defined(SS_DEBUG)    
  if(cold)
    nExceptions=nInstr=0;
  NextIrFetched=false; //at power on
#endif
#if defined(SS_CPU_PREFETCH_CALL) //yes
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

/*  SS_CPU_ROUNDING_SOURCE_100:
    We don't round because the instruction by itself takes 6/10 cycles.
    Fixes Cernit Trandafir demo scren #2 + the similar Summer Delight #5, #7
    Quite surprised that nothing seems to be broken by the change yet.
    TESTING
    INSTRUCTION_TIME_ROUND(4) instead of INSTRUCTION_TIME(6) would break
    RGBeast for example.

    PC: +2 as first step for .B,.W
*/

void m68k_get_source_100_b(){ // .B -(An)

  TRUE_PC+=2;

#if defined(SS_CPU_ROUNDING_SOURCE_100)
  INSTRUCTION_TIME(6);
#else
  INSTRUCTION_TIME_ROUND(6);
#endif

  areg[PARAM_M]--;
  if(PARAM_M==7)
    areg[7]--;

  m68k_READ_B(areg[PARAM_M]) 

}


void m68k_get_source_100_w(){ // .W -(An)

  TRUE_PC+=2;

#if defined(SS_CPU_ROUNDING_SOURCE_100)
  INSTRUCTION_TIME(6);
#else
  INSTRUCTION_TIME_ROUND(6);
#endif

  areg[PARAM_M]-=2;

  m68k_READ_W(areg[PARAM_M])  

}


void m68k_get_source_100_l(){ // .L -(An)

#if defined(SS_CPU_ROUNDING_SOURCE_100)
  INSTRUCTION_TIME(10);
#else
  INSTRUCTION_TIME_ROUND(10);
#endif

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
    TRUE_PC+=2;
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
    m68k_READ_B(fw)
    break;
  }
  case 1:{ // .B (xxx).L
    TRUE_PC+=4;
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
    TRUE_PC+=2;
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
    TRUE_PC+=2;
    INSTRUCTION_TIME_ROUND(8);
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
    m68k_READ_W(fw)
    break;
  }case 1:{ // .W (xxx).L
    TRUE_PC+=4;
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
    TRUE_PC+=2;
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
    TRUE_PC+=2;
    INSTRUCTION_TIME_ROUND(12);
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
    m68k_READ_L(fw) 
    break;
  }case 1:{ // .L (xxx).L
    TRUE_PC+=4;
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
    TRUE_PC+=4;
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
#if defined(SS_CPU_TRUE_PC)
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
#if defined(SS_CPU_TRUE_PC)
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
#if defined(SS_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_B(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();
    pc+=4;  
#if defined(SS_CPU_TRUE_PC)
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
#if defined(SS_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_W(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SS_CPU_TRUE_PC)
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
#if defined(SS_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_L(fw)
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SS_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=4;
#endif
    m68k_SET_DEST_L(fw)
    break;
  }default:
    ILLEGAL;
  }
}

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
#if defined(SS_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_W(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(12);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SS_CPU_TRUE_PC)
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
#if defined(SS_CPU_TRUE_PC)
  if(CHECK_READ)
    TRUE_PC+=2;
#endif
    m68k_SET_DEST_L(fw)
    EXTRA_PREFETCH
    break;
  }case 1:{
    INSTRUCTION_TIME_ROUND(16);
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
#if defined(SS_CPU_TRUE_PC)
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




////////////////
// Exceptions //
////////////////

#if defined(SS_CPU_EXCEPTION)

#define LOGSECTION LOGSECTION_CRASH

void m68k_exception::crash() { // copied from cpu.cpp and improved
  DWORD bytes_to_stack=int((bombs==BOMBS_BUS_ERROR || bombs==BOMBS_ADDRESS_ERROR)
    ? (4+2+2+4+2):(4+2));
  MEM_ADDRESS sp=(MEM_ADDRESS)(SUPERFLAG 
    ? (areg[7] & 0xffffff):(other_sp & 0xffffff));

#if defined(SS_DEBUG)   
  if(M68000.nExceptions!=-1)
  {
    M68000.nExceptions++;  
    TRACE_LOG("\nException #%d, %d bombs (",M68000.nExceptions,bombs);
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
#if defined(SS_CPU_TRUE_PC)
    TRACE_LOG(") during %s\n",(action==EA_READ||CHECK_READ)?"Read":"Write");
#else
    TRACE_LOG(") during %s\n",(action==EA_READ)?"Read":"Write (maybe!)");
#endif
#ifdef DEBUG_BUILD 
    EasyStr instr=disa_d2(old_pc); // take advantage of the disassembler
    TRACE_LOG("PC=%X-IR=%X-Ins: %s -SR=%X-Bus=%X",old_pc,ir,instr.Text,sr,abus);
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
    //SS:never quite understood this rounding for interrupts thing
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts 
    if(bombs==BOMBS_ILLEGAL_INSTRUCTION || bombs==BOMBS_PRIVILEGE_VIOLATION)
    {
      if(!SUPERFLAG) 
        change_to_supervisor_mode();
      TRACE_LOG("Push crash address %X on %X\n",(crash_address & 0x00ffffff) | pc_high_byte,r[15]-4);
      m68k_PUSH_L((crash_address & 0x00ffffff) | pc_high_byte); 
      INSTRUCTION_TIME_ROUND(8);
      TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
      m68k_PUSH_W(_sr); // Status register 
      INSTRUCTION_TIME_ROUND(4); //Round first for interrupts
      MEM_ADDRESS ad=LPEEK(bombs*4); // Get the vector
      if(ad & 1) // bad vector!
      {
        // Very rare, generally indicates emulation bug
        bombs=BOMBS_ADDRESS_ERROR;
#if defined(SS_DEBUG)
        BRK(odd exception vector);
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
      TRY_M68K_EXCEPTION
      {

#if defined(SS_HACKS) && !defined(SS_CPU_TRUE_PC_AND_NO_HACKS)
/*  Using opcode specific adjustments, disabled in v3.5.1
*/
        int offset=0;

        if(SSE_HACKS_ON)
        {
          switch(_ir) // opcode in IR at crash
          {
#if defined(SS_CPU_HACK_PHALEON)
          case 0x21F8: // move.l $0.W,$24.w
            TRACE_LOG("Protected demo (European, Phaleon, Transbeauce 2?\n");
            offset=-2; // used to fix those protected demos, but now no need?
            break;
#endif
#if defined(SS_CPU_HACK_WAR_HELI)
          case 0x2285: // move.l d5,(a1)
            TRACE_LOG("War Heli? (use ADAT)\n");
            offset=2; // fixes War Heli, already in Steem 3.2., but too generic
            break;
#endif
#if defined(SS_CPU_HACK_BLOOD_MONEY)
          case 0x48d6: // movem.l a0-5,(a6)
            TRACE_LOG("Blood Money?\n");
            offset=2;
            break;
#endif
          }//sw
        }//hacks
        if(offset)
           TRACE_LOG("Adjusting stacked PC %X (%d) %X\n",_pc,offset,_pc+offset);
        _pc+=offset;
#endif//hack opcode


#if defined(SS_CPU_DETECT_STACK_PC_USE)
        _pc=0x123456; // push garbage!
#elif defined(SS_CPU_TRUE_PC) 
        if(
#if !defined(SS_CPU_TRUE_PC_AND_NO_HACKS)
          !SSE_HACKS_ON && 
#endif
          _pc!=M68000.Pc)
        {
          TRACE_LOG("pc %X true PC %X\n",_pc,M68000.Pc);
          _pc=M68000.Pc;
        }
#endif
        TRACE_LOG("Push PC %X on %X\n",_pc,r[15]-4);
        m68k_PUSH_L(_pc);
        TRACE_LOG("Push SR %X on %X\n",_sr,r[15]-2);
        m68k_PUSH_W(_sr);
        TRACE_LOG("Push IR %X on %X\n",_ir,r[15]-2);
        m68k_PUSH_W(_ir);
        TRACE_LOG("Push address %X on %X\n",address,r[15]-4);
        m68k_PUSH_L(address);
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
        r[15]=0xf000; // R15=A7  // should be halt?
      }
      END_M68K_EXCEPTION

      TRACE_LOG("PC = %X\n\n",LPEEK(bombs*4));
      SET_PC(LPEEK(bombs*4)); // includes final prefetch

      SR_CLEAR(SR_TRACE);

#if defined(SS_CPU_FETCH_TIMING)
      INSTRUCTION_TIME_ROUND(50-4); // TODO micro timings?
      FETCH_TIMING;
#if defined(SS_CPU_PREFETCH_TIMING_SET_PC)
      INSTRUCTION_TIME_ROUND(4); // because FETCH_TIMING does nothing
#endif
#else
      INSTRUCTION_TIME_ROUND(50); //Round for fetch
#endif
#if defined(SS_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=2;
#endif
    }
    DEBUG_ONLY(log_history(bombs,crash_address));
  }
  PeekEvent(); // Stop exception freeze
}

#undef LOGSECTION


#endif//#if defined(SS_CPU_EXCEPTION)




//////////////
// Prefetch //
//////////////

#if defined(SS_CPU_PREFETCH_CALL)

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
// TODO: only parts not treated already, but it's dangerous
#if defined(SS_CPU_POKE)

void m68k_poke_abus2(BYTE x){
  abus&=0xffffff;
  if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_b(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SS_MMU_NO_CONFUSION)
    if (mmu_confused){
      mmu_confused_set_dest_to_addr(1,true);
      m68k_DEST_B=x;
#if defined(SS_MMU_TRACE2)
      TRACE_LOG("MMU %X: poke %X=%X\n",old_pc,m68k_DEST_B,x);
#endif
    }else 
#endif      
    if (abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_B(abus);
    if (SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      PEEK(abus)=x;
    else if (abus>=MEM_START_OF_USER_AREA)
      PEEK(abus)=x;
#if defined(SS_CPU_IGNORE_WRITE_0)
/*  Atari doc states:
    A 4 word portion  of ROM  is  shadowed  at  the  start of RAM for 
    the reset stack  pointer and program counter.  
    Writing to this  area  or  any ROM location will also result in 
    a bus error.

    Yet for Aladin (Macintosh emulator), we mustn't crash when the bus
    is set at 0, and we must ignore writes on 0 instead of crashing.

    It must crash at 4 for Death of the Clock Cycles (bugfix 3.5.1)

    No good explanation for that, we just have programs running.
*/
    else if(SUPERFLAG && abus==0 ) 
      ;
#endif
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}


void m68k_dpoke_abus2(WORD x){
  abus&=0xffffff;
  if(abus&1) exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_w(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SS_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(2,true);
      m68k_DEST_W=x;
#if defined(SS_MMU_TRACE2)
      TRACE_LOG("MMU %X: dpoke %X=%X\n",old_pc,m68k_DEST_W,x);
#endif
    }else 
#endif
    if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_W(abus);
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      DPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      DPEEK(abus)=x;
#if defined(SS_CPU_IGNORE_WRITE_0)
    else if(SUPERFLAG && !abus)
      ;
#endif
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}


void m68k_lpoke_abus2(LONG x){
  abus&=0xffffff;
  if(abus&1)exception(BOMBS_ADDRESS_ERROR,EA_WRITE,abus);
  else if(abus>=MEM_IO_BASE){
    if(SUPERFLAG)
      io_write_l(abus,x);
    else
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }else if(abus>=himem){
#if !defined(SS_MMU_NO_CONFUSION)
    if(mmu_confused){
      mmu_confused_set_dest_to_addr(4,true);
      m68k_DEST_L=x;
#if defined(SS_MMU_TRACE2)
      TRACE_LOG("MMU %X: lpoke %X=%X\n",old_pc,m68k_DEST_L,x);
#endif
    }else 
#endif
    if(abus>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
    } //otherwise throw away
  }else{
    DEBUG_CHECK_WRITE_L(abus);
    if(SUPERFLAG && abus>=MEM_FIRST_WRITEABLE)
      LPEEK(abus)=x;
    else if(abus>=MEM_START_OF_USER_AREA)
      LPEEK(abus)=x;
#if defined(SS_CPU_IGNORE_WRITE_0__)
/*  Not for long because it breaks Fuzion 77, 78, 84 (bugfix 3.5.2).
    There's no real explanation. Maybe the bus error detector doesn't 
    start until after the first word.
*/
  
    else if(SUPERFLAG && !abus)
      ;
#endif
    else exception(BOMBS_BUS_ERROR,EA_WRITE,abus);
  }
}

#endif


/////////////////////////
// Redone instructions //
/////////////////////////



#if defined(SS_CPU_DIV) 

// using ijor's timings (in 3rdparty\pasti), what they fix proves them correct
//#undef SS_CPU_LINE_8_TIMINGS
void                              m68k_divu(){
#if !defined(SS_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif
  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){ // div by 0
    // Clear V flag when dividing by zero. Fixes...?
    SR_CLEAR(SR_V);
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    PREFETCH_IRC;
#if defined(SS_CPU_LINE_8_TIMINGS)
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
    }else{
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
      r[PARAM_N]=((((unsigned long)r[PARAM_N])%((unsigned short)m68k_src_w))<<16)+q;
    }
  }
}


void                              m68k_divs(){
#if !defined(SS_CPU_LINE_8_TIMINGS)
  FETCH_TIMING;
#endif

  m68k_GET_SOURCE_W_NOT_A;
  if (m68k_src_w==0){
    /* Clear V flag when dividing by zero - Alcatraz Odyssey demo depends
     * on this (actually, it's doing a DIVS).  */
    SR_CLEAR(SR_V);
    m68k_interrupt(LPEEK(BOMBS_DIVISION_BY_ZERO*4));
    INSTRUCTION_TIME_ROUND(0); //Round first for interrupts
    INSTRUCTION_TIME_ROUND(38);
  }else{
    PREFETCH_IRC;
#if defined(SS_CPU_LINE_8_TIMINGS)
    FETCH_TIMING;
#endif
    signed long q;
    signed long dividend = (signed long) (r[PARAM_N]);
    signed short divisor = (signed short) m68k_src_w;
    int cycles_for_instr=getDivs68kCycles(dividend,divisor)-4; // -prefetch
    INSTRUCTION_TIME(cycles_for_instr);   // fixes Dragonnels loader
    q=(signed long)((signed long)dividend)/(signed long)((signed short)divisor);
    if(q<-32768 || q>32767){
      SR_SET(SR_V);
    }else{
      SR_CLEAR(SR_Z+SR_N+SR_C+SR_V);
      if(q&MSB_W)SR_SET(SR_N);
      if(q==0)SR_SET(SR_Z);
      r[PARAM_N]=((((signed long)r[PARAM_N])%((signed short)m68k_src_w))<<16)|((long)LOWORD(q));
    }
  }
}
//#define SS_CPU_LINE_8_TIMINGS
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

#if defined(SS_CPU_MOVE_B)

void m68k_0001() {  // move.b

#if (!defined(SS_CPU_LINE_3_TIMINGS) && !defined(SS_CPU_PREFETCH_TIMING)) \
  || !defined(SS_CPU_PREFETCH_MOVE_MEM)
  
#if defined(SS_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000)
    FETCH_TIMING_NO_ROUND; // it's the same, but recorded as fetch timing
  else
#endif
    INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SS_CPU_POST_INC) || defined(SS_CPU_PRE_DEC)
  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis
#endif

#if defined(SS_CPU_ASSERT_ILLEGAL)
  // We consider ILLEGAL before bus/address error
  // fixes Transbeauce 2 loader, Titan
  if( (ir&BITS_876)==BITS_876_001
    || (ir&BITS_876)==BITS_876_111
    && (ir&BITS_ba9)!=BITS_ba9_000
    && (ir&BITS_ba9)!=BITS_ba9_001 )
    m68k_unrecognised(); 
#endif

  // Source
  m68k_GET_SOURCE_B;

  // Destination

  TRUE_PC=pc+2;

  if((ir&BITS_876)==BITS_876_000)
  { // Dn
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_B=m68k_src_b; // move completed
    SR_CHECK_Z_AND_N_B; // update flags
#if defined(SS_CPU_LINE_1_TIMINGS) 
    FETCH_TIMING_NO_ROUND;
#endif
  }
#if !defined(SS_CPU_ASSERT_ILLEGAL)
  else if((ir&BITS_876)==BITS_876_001)
    m68k_unrecognised();
#endif
  else
  {   //to memory
    bool refetch=false;
    switch(ir&BITS_876)
    {
    case BITS_876_010: // (An)
#if (defined(SS_CPU_LINE_1_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011: // (An)+
#if (defined(SS_CPU_LINE_1_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
#if defined(SS_CPU_POST_INC)
      UpdateAn=(PARAM_N==7)?2:1;
#else
      areg[PARAM_N]++; 
      if(PARAM_N==7)
        areg[7]++;
#endif
      break;
    case BITS_876_100: // -(An)
#if defined(SS_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC;
      FETCH_TIMING;
#endif  
#if (defined(SS_CPU_LINE_1_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
#if defined(SS_CPU_PRE_DEC)
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
#if (defined(SS_CPU_LINE_1_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(14-4-4);
#if (defined(SS_CPU_LINE_1_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
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
#if (defined(SS_CPU_LINE_1_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
        INSTRUCTION_TIME(4);
#endif
        abus=0xffffff & (unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case BITS_ba9_001: // (xxx).L
#if defined(SS_CPU_PREFETCH_CLASS)
        if(refetch)
          M68000.PrefetchClass=2; // only .L
#endif
        INSTRUCTION_TIME(16-4-4);
#if (defined(SS_CPU_LINE_1_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
        INSTRUCTION_TIME(4);
#endif

        abus=m68k_fetchL() & 0xffffff;
        pc+=4;  
        break;
#if !defined(SS_CPU_ASSERT_ILLEGAL)
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
    m68k_poke_abus(m68k_src_b); // write; could crash

#if defined(SS_CPU_POST_INC) || defined(SS_CPU_PRE_DEC)
    areg[PARAM_N]+=UpdateAn; // can be 0,-1,-2,+1,+2
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
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
#if defined(SS_CPU_PREFETCH_CLASS)
  if(M68000.PrefetchClass==1) // classes 0 & 2 already handled
  {
    if((ir&BITS_876)==BITS_876_000)
      PREFETCH_IRC_NO_ROUND;
    else
      PREFETCH_IRC;
  }
#endif

}

#endif

#if defined(SS_CPU_MOVE_L)

void m68k_0010()  //move.l
{

#if (!defined(SS_CPU_LINE_3_TIMINGS) && !defined(SS_CPU_PREFETCH_TIMING)) \
  || !defined(SS_CPU_PREFETCH_MOVE_MEM)
#if defined(SS_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000
    ||(ir & BITS_876)==BITS_876_001)
    FETCH_TIMING_NO_ROUND;
  else
#endif
  INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SS_CPU_POST_INC) || defined(SS_CPU_PRE_DEC)
  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis
#endif

#if defined(SS_CPU_ASSERT_ILLEGAL)
  if( (ir&BITS_876)==BITS_876_111
    && (ir&BITS_ba9)!=BITS_ba9_000
    && (ir&BITS_ba9)!=BITS_ba9_001 )
      m68k_unrecognised();
#endif

  m68k_GET_SOURCE_L; // where tb2 etc crash

  // Destination

  TRUE_PC=pc+2; // eg. War Heli move.l d5,(a1)

  if ((ir & BITS_876)==BITS_876_000) // Dn
  { 
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_L=m68k_src_l;
    SR_CHECK_Z_AND_N_L;
#if defined(SS_CPU_LINE_2_TIMINGS)
    FETCH_TIMING_NO_ROUND;
#endif

  }
  else if ((ir & BITS_876)==BITS_876_001) // An
  {
    areg[PARAM_N]=m68k_src_l; // MOVEA
#if defined(SS_CPU_LINE_2_TIMINGS) 
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
#if (defined(SS_CPU_LINE_2_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011: // (An)+
      INSTRUCTION_TIME(12-4-4);
#if (defined(SS_CPU_LINE_2_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
#if defined(SS_CPU_POST_INC)
      UpdateAn=4;
#else
      areg[PARAM_N]+=4;
#endif
      break;
    case BITS_876_100: // -(An)
      INSTRUCTION_TIME(12-4-4);
#if (defined(SS_CPU_LINE_2_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC;
      FETCH_TIMING;
#endif  
#if defined(SS_CPU_PRE_DEC)
      UpdateAn=-4;
      abus=areg[PARAM_N]+UpdateAn;
#else
      areg[PARAM_N]-=4;
      abus=areg[PARAM_N];
#endif
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(16-4-4);
#if (defined(SS_CPU_LINE_2_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
      pc+=2; 
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(18-4-4);
#if (defined(SS_CPU_LINE_2_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
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
#if (defined(SS_CPU_LINE_2_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case BITS_ba9_001: // (xxx).L
        INSTRUCTION_TIME(20-4-4);
#if (defined(SS_CPU_LINE_2_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
        INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
        if(refetch) 
          M68000.PrefetchClass=2; 
#endif 
        abus=m68k_fetchL()&0xffffff;
        pc+=4;  
        break;
#if !defined(SS_CPU_ASSERT_ILLEGAL)
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
    m68k_lpoke_abus(m68k_src_l);

#if defined(SS_CPU_POST_INC) || defined(SS_CPU_PRE_DEC)
    areg[PARAM_N]+=UpdateAn; // can be 0,-4,+4
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
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
#if defined(SS_CPU_PREFETCH_CLASS)
  if(M68000.PrefetchClass==1)
  {
    if((ir&BITS_876)==BITS_876_000 || (ir & BITS_876)==BITS_876_001)
      PREFETCH_IRC_NO_ROUND;
    else
#ifdef TEST_01
      PREFETCH_IRC_NO_ROUND;
#else
      PREFETCH_IRC;
#endif

  }
#endif

}

#endif

#if defined(SS_CPU_MOVE_W)

void m68k_0011() //move.w
{
#if (!defined(SS_CPU_LINE_3_TIMINGS) && !defined(SS_CPU_PREFETCH_TIMING)) \
  || !defined(SS_CPU_PREFETCH_MOVE_MEM)
#if defined(SS_CPU_FETCH_TIMING)
  if((ir&BITS_876)==BITS_876_000
    ||(ir & BITS_876)==BITS_876_001)
    FETCH_TIMING_NO_ROUND;
  else
#endif
  INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
  M68000.PrefetchClass=1; // by default 
#endif

#if defined(SS_CPU_POST_INC) || defined(SS_CPU_PRE_DEC)
  int UpdateAn=0; // for (An)+ and -(An) both, based on microcodes analysis
#endif

#if defined(SS_CPU_ASSERT_ILLEGAL)
    if( (ir&BITS_876)==BITS_876_111
      && (ir&BITS_ba9)!=BITS_ba9_000
      && (ir&BITS_ba9)!=BITS_ba9_001 )
      m68k_unrecognised();
#endif

  m68k_GET_SOURCE_W;

  // Destination

  TRUE_PC=pc+2;

  if ((ir & BITS_876)==BITS_876_000) // Dn
  {
    SR_CLEAR(SR_V+SR_C+SR_N+SR_Z);
    m68k_dest=&(r[PARAM_N]);
    m68k_DEST_W=m68k_src_w;
    SR_CHECK_Z_AND_N_W;
#if defined(SS_CPU_LINE_3_TIMINGS) 
    FETCH_TIMING_NO_ROUND;
#endif
  }
  else if ((ir & BITS_876)==BITS_876_001) // An
  {
    areg[PARAM_N]=(signed long)((signed short)m68k_src_w); // movea
#if defined(SS_CPU_LINE_3_TIMINGS)
    FETCH_TIMING_NO_ROUND;
#endif
  }
  else
  {   //to memory
    bool refetch=0;
    switch (ir & BITS_876)
    {
    case BITS_876_010: // (An)
#if (defined(SS_CPU_LINE_3_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
      break;
    case BITS_876_011:  // (An)+
#if (defined(SS_CPU_LINE_3_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N];
#if defined(SS_CPU_POST_INC)
      UpdateAn=2; // fixes Beyond loader
#else
      areg[PARAM_N]+=2;
#endif
      break;
    case BITS_876_100: // -(An)
#if (defined(SS_CPU_LINE_3_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
#if defined(SS_CPU_PREFETCH_CLASS)
      M68000.PrefetchClass=0; 
      PREFETCH_IRC; // fixes 4pixel Rasters demo
      FETCH_TIMING;
#endif  
#if defined(SS_CPU_PRE_DEC)
      UpdateAn=-2;
      abus=areg[PARAM_N]+UpdateAn;
#else
      areg[PARAM_N]-=2;
      abus=areg[PARAM_N];
#endif
      break;
    case BITS_876_101: // (d16, An)
      INSTRUCTION_TIME(12-4-4);
#if (defined(SS_CPU_LINE_3_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif
      abus=areg[PARAM_N]+(signed short)m68k_fetchW();
#if defined(SS_CPU_3615GEN4_ULM) && defined(SS_SHIFTER)
      // Writing in video RAM just after it's been fetched into the shifter
      // fixes 3615Gen4 by ULM, but maybe not 100% (glitch in left border?)
      if(abus>=shifter_draw_pointer && abus<=shifter_draw_pointer+32 
        && Shifter.FetchingLine())
        Shifter.Render(LINECYCLES,DISPATCHER_CPU); 
#endif
      pc+=2; 
      break;
    case BITS_876_110: // (d8, An, Xn)
      INSTRUCTION_TIME(14-4-4);
#if (defined(SS_CPU_LINE_3_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
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
#if (defined(SS_CPU_LINE_3_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

        abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
        pc+=2; 
        break;
      case BITS_ba9_001: // (xxx).L
        INSTRUCTION_TIME(16-4-4);
#if (defined(SS_CPU_LINE_3_TIMINGS) || defined(SS_CPU_PREFETCH_TIMING)) \
  && defined(SS_CPU_PREFETCH_MOVE_MEM)
      INSTRUCTION_TIME(4);
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
        if(refetch) 
          M68000.PrefetchClass=2; 
#endif  
        abus=m68k_fetchL()&0xffffff;
        pc+=4;  
        break;
#if defined(SS_CPU_ASSERT_ILLEGAL)
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
    m68k_dpoke_abus(m68k_src_w);

#if defined(SS_CPU_POST_INC) || defined(SS_CPU_PRE_DEC)
    areg[PARAM_N]+=UpdateAn; // can be 0,-2,+2
#endif

#if defined(SS_CPU_PREFETCH_CLASS)
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
#if defined(SS_CPU_PREFETCH_CLASS)
  if(M68000.PrefetchClass==1)
  {
    if((ir&BITS_876)==BITS_876_000 || (ir & BITS_876)==BITS_876_001)
      PREFETCH_IRC_NO_ROUND;
    else
#ifdef TEST_01
      PREFETCH_IRC_NO_ROUND;
#else
      PREFETCH_IRC;
#endif

  }

#endif
}

#endif

#endif//#if defined(SS_CPU)
