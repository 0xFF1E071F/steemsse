// this file is included in cpu_sse.cpp, which is a big file already

void m68kReadBFromAddr();
void m68kReadWFromAddr();
void m68kReadLFromAddr();

// peek

BYTE m68k_peek(MEM_ADDRESS ad){
  ad&=0xffffff;
#if defined(SSE_MMU_RAM_TEST3)
  if (ad>=himem  || mmu_confused) {
#else
  if (ad>=himem) {
#endif
    if (ad>=MEM_IO_BASE){
      if(SUPERFLAG)return io_read_b(ad);
      else exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=0xfc0000){
#if defined(SSE_MMU_ROUNDING_BUS0A)
      if(tos_high && ad<(0xfc0000+192*1024))
      {
        if(MMU.Rounded)
        {
          INSTRUCTION_TIME(-2);
          MMU.Rounded=false;
        }
        return ROM_PEEK(ad-rom_addr);
      }
#else
      if(tos_high && ad<(0xfc0000+192*1024))return ROM_PEEK(ad-rom_addr);
#endif
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=MEM_EXPANSION_CARTRIDGE){
      if (cart) return CART_PEEK(ad-MEM_EXPANSION_CARTRIDGE);
      else return 0xff;
    }else if(ad>=rom_addr){
#if defined(SSE_MMU_ROUNDING_BUS0A)
      if (ad<(0xe00000+256*1024)) 
      {
        if(MMU.Rounded)
        {
          INSTRUCTION_TIME(-2);
          MMU.Rounded=false;
        }
        return ROM_PEEK(ad-rom_addr);
      }
#else
      if (ad<(0xe00000+256*1024)) return ROM_PEEK(ad-rom_addr);
#endif
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xff;
#if !defined(SSE_MMU_NO_CONFUSION)
    }else if (mmu_confused){
      return mmu_confused_peek(ad,true);
#endif
    }else if (ad>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else{
      return 0xff;
    }
  }else if(ad>=MEM_START_OF_USER_AREA){
    DEBUG_CHECK_READ_B(ad);
    return (BYTE)(PEEK(ad));
  }else if(SUPERFLAG){
    DEBUG_CHECK_READ_B(ad);
    return (BYTE)(PEEK(ad));
  }else exception(BOMBS_BUS_ERROR,EA_READ,ad);
  return 0;
}


WORD m68k_dpeek(MEM_ADDRESS ad){
  ad&=0xffffff;
  if(ad&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,ad);
#if defined(SSE_MMU_RAM_TEST3)
  if (ad>=himem  || mmu_confused) {
#else
  else if (ad>=himem) {
#endif
    if(ad>=MEM_IO_BASE){
      if(SUPERFLAG)return io_read_w(ad);
      else exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=0xfc0000){
#if defined(SSE_MMU_ROUNDING_BUS0A)
      if(tos_high && ad<(0xfc0000+192*1024))
      {
        if(MMU.Rounded)
        {
          INSTRUCTION_TIME(-2);
          MMU.Rounded=false;
        }
        return ROM_DPEEK(ad-rom_addr);
      }
#else
      if(tos_high && ad<(0xfc0000+192*1024))return ROM_DPEEK(ad-rom_addr);
#endif
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=MEM_EXPANSION_CARTRIDGE){
      if (cart) return CART_DPEEK(ad-MEM_EXPANSION_CARTRIDGE);
      else return 0xffff;
    }else if(ad>=rom_addr){
#if defined(SSE_MMU_ROUNDING_BUS0A)
      if (ad<(0xe00000+256*1024)) 
      {
        if(MMU.Rounded)
        {
          INSTRUCTION_TIME(-2);
          MMU.Rounded=false;
        }
        return ROM_DPEEK(ad-rom_addr);
      }
#else
      if (ad<(0xe00000+256*1024)) return ROM_DPEEK(ad-rom_addr);
#endif
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xffff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xffff;
#if !defined(SSE_MMU_NO_CONFUSION)
    }else if(mmu_confused){
      return mmu_confused_dpeek(ad,true);
#endif
    }else if(ad>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else{
      return 0xffff;
    }
  }else if(ad>=MEM_START_OF_USER_AREA){
    DEBUG_CHECK_READ_W(ad);
    return DPEEK(ad);
  }else if(SUPERFLAG){
    DEBUG_CHECK_READ_W(ad);
    return DPEEK(ad);
  }else exception(BOMBS_BUS_ERROR,EA_READ,ad);
  return 0;
}


LONG m68k_lpeek(MEM_ADDRESS ad){
  ad&=0xffffff;
  if(ad&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,ad);
#if defined(SSE_MMU_RAM_TEST3)
  if (ad>=himem  || mmu_confused) {
#else
  else if (ad>=himem) {
#endif
    if(ad>=MEM_IO_BASE){
      if(SUPERFLAG)return io_read_l(ad);
      else exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=0xfc0000){
#if defined(SSE_MMU_ROUNDING_BUS0A)
      if(tos_high && ad<(0xfc0000+192*1024))
      {
        if(MMU.Rounded)
        {
          INSTRUCTION_TIME(-2);
          MMU.Rounded=false;
        }
        return ROM_LPEEK(ad-rom_addr);
      }
#else
      if(tos_high && ad<(0xfc0000+192*1024-2))return ROM_LPEEK(ad-rom_addr);
#endif
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if (ad>=MEM_EXPANSION_CARTRIDGE){
      if (cart) return CART_LPEEK(ad-MEM_EXPANSION_CARTRIDGE);
      else return 0xffffffff;
    }else if (ad>=rom_addr){
#if defined(SSE_MMU_ROUNDING_BUS0A)
      if (ad<(0xe00000+256*1024)) 
      {
        if(MMU.Rounded)
        {
          INSTRUCTION_TIME(-2);
          MMU.Rounded=false;
        }
        return ROM_LPEEK(ad-rom_addr);
      }
#else
      if (ad<(0xe00000+256*1024-2)) return ROM_LPEEK(ad-rom_addr);
#endif
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xffffffff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xffffffff;
#if !defined(SSE_MMU_NO_CONFUSION)
    }else if (mmu_confused){
      return mmu_confused_lpeek(ad,true);
#endif
    }else if (ad>=FOUR_MEGS){
      exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else{
      return 0xffffffff;
    }
  }else if (ad>=MEM_START_OF_USER_AREA){
    DEBUG_CHECK_READ_L(ad);
    return LPEEK(ad);
  }else if (SUPERFLAG){
    DEBUG_CHECK_READ_L(ad);
    return LPEEK(ad);
  }else exception(BOMBS_BUS_ERROR,EA_READ,ad);
  return 0;
}

//read

BYTE m68k_read_dest_b(){
  BYTE x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return LOBYTE(r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    CPU_ABUS_ACCESS_READ; //nr
    return m68k_peek(areg[PARAM_M]);
  case BITS_543_011:
    CPU_ABUS_ACCESS_READ; //nr
    x=m68k_peek(areg[PARAM_M]);areg[PARAM_M]++;
    if(PARAM_M==7)areg[7]++;
    return x;
  case BITS_543_100:
    areg[PARAM_M]--;
    if (PARAM_M==7) areg[7]--;
    INSTRUCTION_TIME(2); //n
    CPU_ABUS_ACCESS_READ; //nr
    return m68k_peek(areg[PARAM_M]);
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register MEM_ADDRESS ad=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_peek(ad);
    return x;
  }case BITS_543_110:
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    if(m68k_iriwo&BIT_b){  //.l
      return m68k_peek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]);
    }else{         //.w
      return m68k_peek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]);
    }
  case BITS_543_111:
    switch(ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;//np
      register MEM_ADDRESS ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_peek(ad);
      return x;
    }case 1:{
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      register MEM_ADDRESS ad=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_peek(ad);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}


WORD m68k_read_dest_w(){
  WORD x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return LOWORD(r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    CPU_ABUS_ACCESS_READ;//nr
    return m68k_dpeek(areg[PARAM_M]);
  case BITS_543_011:
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(areg[PARAM_M]);areg[PARAM_M]+=2;
    return x;
  case BITS_543_100:
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ;//nr
    areg[PARAM_M]-=2;
    return m68k_dpeek(areg[PARAM_M]);
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register MEM_ADDRESS ad=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(ad);
    return x;
  }case BITS_543_110:
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    if(m68k_iriwo&BIT_b){  //.l
      return m68k_dpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]);
    }else{         //.w
      return m68k_dpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]);
    }
  case BITS_543_111:
    switch(ir&ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;//np
      register MEM_ADDRESS ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_dpeek(ad);
      return x;
    }case 1:{
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      register MEM_ADDRESS ad=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_dpeek(ad);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}


LONG m68k_read_dest_l(){
  LONG x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return (r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    CPU_ABUS_ACCESS_READ_L;//nR nr
    return m68k_lpeek(areg[PARAM_M]);
  case BITS_543_011:
    CPU_ABUS_ACCESS_READ_L;//nR nr
    x=m68k_lpeek(areg[PARAM_M]);areg[PARAM_M]+=4;
    return x;
  case BITS_543_100:
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_L;//nR nr
    areg[PARAM_M]-=4;
    return m68k_lpeek(areg[PARAM_M]);
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |  12(3/0)   |              np nR nr          
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register MEM_ADDRESS ad=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ_L;//nR nr
    x=m68k_lpeek(ad);
    return x;
  }case BITS_543_110:
//  (d8,An,Xn)      | 110 | reg |  14(3/0)   |         n    np nR nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ_L;////nR nr
    if(m68k_iriwo&BIT_b){  //.l
      return m68k_lpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12]);
    }else{         //.w
      return m68k_lpeek(areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12]);
    }
  case BITS_543_111:
    switch(ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |  12(3/0)   |              np nR nr      
      CPU_ABUS_ACCESS_READ_FETCH;//np
      register MEM_ADDRESS ad=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ_L;//nR nr
      x=m68k_lpeek(ad);
      return x;
    }case 1:{
//(xxx).L         | 111 | 001 |  16(4/0)   |           np np nR nr           
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      register MEM_ADDRESS ad=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ_L;//nR nr
      x=m68k_lpeek(ad);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}


/*

*******************************************************************************
                   OPERAND EFFECTIVE ADDRESS CALCULATION TIMES
*******************************************************************************
-------------------------------------------------------------------------------
       <ea>       |    Exec Time    |               Data Bus Usage
------------------+-----------------+------------------------------------------
.B or .W :        |                 |
  Dn              |          0(0/0) |
  An              |          0(0/0) |
  (An)            |          4(1/0) |                              nr           
  (An)+           |          4(1/0) |                              nr           
  -(An)           |          6(1/0) |                   n          nr           
  (d16,An)        |          8(2/0) |                        np    nr           
  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
  (xxx).W         |          8(2/0) |                        np    nr           
  (xxx).L         |         12(3/0) |                     np np    nr           
  #<data>         |          4(1/0) |                        np                 
.L :              |                 |
  Dn              |          0(0/0) |
  An              |          0(0/0) |
  (An)            |          8(2/0) |                           nR nr           
  (An)+           |          8(2/0) |                           nR nr           
  -(An)           |         10(2/0) |                   n       nR nr           
  (d16,An)        |         12(3/0) |                        np nR nr           
  (d8,An,Xn)      |         14(3/0) |                   n    np nR nr           
  (xxx).W         |         12(3/0) |                        np nR nr           
  (xxx).L         |         16(4/0) |                     np np nR nr           
  #<data>         |          8(2/0) |                     np np                 

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//////////////////////////    GET SOURCE     ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

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
Dn		          000	R	  0(0/0)	0(0/0)	Data Register Direct
An		          001	R	  0(0/0)	0(0/0)	Address Register Direct
(An)		        010	R	  4(1/0)	8(2/0)	Address Register Indirect
(An)+		        011	R	  4(1/0)	8(2/0)	Address Register Indirect with Postincrement
–(An)		        100	R	  6(1/0)	10(2/0)	Address Register Indirect with Predecrement
(d16, An)	      101	R	  8(2/0)	12(3/0)	Address Register Indirect with Displacement
(d8, An, Xn)*		110	R	  10(2/0)	14(3/0)	Address Register Indirect with Index
(xxx).W		      111	000	8(2/0)	12(3/0)	Absolute Short
(xxx).L		      111	001	12(3/0)	16(4/0)	Absolute Long
(d16, PC)		    111	010	8(2/0)	12(3/0)	Program Counter Indirect with Displacement
(d8, PC, Xn)*		111	011	10(2/0)	14(3/0)	Program Counter Indirect with Index - 8-Bit Displacement
#<data>		      111	100	4(1/0)	8(2/0)	Immediate
*/

// Dn

void m68k_get_source_000_b(){ // .B Dn
  //  Dn              |          0(0/0) |
  m68k_src_b=(BYTE)(r[PARAM_M]); 
}


void m68k_get_source_000_w(){ //.W Dn
  //  Dn              |          0(0/0) |
  m68k_src_w=(WORD)(r[PARAM_M]); 
}


void m68k_get_source_000_l(){ //.L Dn
  //  Dn              |          0(0/0) |
  m68k_src_l=(long)(r[PARAM_M]); 
}


// An

void m68k_get_source_001_b(){ // .B An
  //  An              |          0(0/0) |
  m68k_src_b=(BYTE)(areg[PARAM_M]); 
}


void m68k_get_source_001_w(){ // .W An
  //  An              |          0(0/0) |
  m68k_src_w=(WORD)(areg[PARAM_M]); 
}


void m68k_get_source_001_l(){ // .L An
  //  An              |          0(0/0) |
  m68k_src_l=(long)(areg[PARAM_M]); 
}

// (An)

void m68k_get_source_010_b(){ // .B (An)
  //  (An)            |          4(1/0) |                              nr       
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_B(abus) 
#else
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_B(areg[PARAM_M]) 
#endif
}


void m68k_get_source_010_w(){ // .W (An)
  //  (An)            |          4(1/0) |                              nr       
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(abus) 
#else
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(areg[PARAM_M]) 
#endif
}

  
void m68k_get_source_010_l(){ // .L (An)
  //  (An)            |          8(2/0) |                           nR nr         
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ_L; //nR nr
  m68k_READ_L(abus) 
#else
  CPU_ABUS_ACCESS_READ_L; //nR nr
  m68k_READ_L(areg[PARAM_M]) 
#endif
}

// (An)+

void m68k_get_source_011_b(){ // .B (An)+
  //  (An)+           |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ;//nr
  m68k_READ_B(abus) 
  areg[PARAM_M]++; 
  if(PARAM_M==7)
    areg[7]++;
#else
  CPU_ABUS_ACCESS_READ;//nr
  m68k_READ_B(areg[PARAM_M]) 
  areg[PARAM_M]++; 
  if(PARAM_M==7)
    areg[7]++;
#endif
}


void m68k_get_source_011_w(){ // .W (An)+
  //  (An)+           |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  m68k_READ_W(abus) 
#else
  m68k_READ_W(areg[PARAM_M]) 
#endif
  areg[PARAM_M]+=2; 
}


void m68k_get_source_011_l(){ // .L (An)+
  //  (An)+           |          8(2/0) |                           nR nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  m68k_READ_L(abus) 
#else
  m68k_READ_L(areg[PARAM_M]) 
#endif
  areg[PARAM_M]+=4; // for .L, we assume ++ post read
}

// -(An)

void m68k_get_source_100_b(){ // .B -(An)
  //  -(An)           |          6(1/0) |                   n          nr           
  TRUE_PC+=2;
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  areg[PARAM_M]--;
  if(PARAM_M==7)
    areg[7]--;
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  m68k_READ_B(abus) 
#else
  areg[PARAM_M]--;
  if(PARAM_M==7)
    areg[7]--;
  m68k_READ_B(areg[PARAM_M]) 
#endif
  
}


void m68k_get_source_100_w(){ // .W -(An)
  //  -(An)           |          6(1/0) |                   n          nr           
  TRUE_PC+=2;
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  areg[PARAM_M]-=2;
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  m68k_READ_W(abus)  
#else
  areg[PARAM_M]-=2;
  m68k_READ_W(areg[PARAM_M])  
#endif
  
}


void m68k_get_source_100_l(){ // .L -(An)
  //  -(An)           |         10(2/0) |                   n       nR nr           
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  areg[PARAM_M]-=4;
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  m68k_READ_L(abus) 
#else
  areg[PARAM_M]-=4;
  m68k_READ_L(areg[PARAM_M])  
#endif
  
}


// (d16, An)

void m68k_get_source_101_b(){ //.B (d16, An)
  //  (d16,An)        |          8(2/0) |                        np    nr           
  CPU_ABUS_ACCESS_READ_FETCH; //np
  register int fw=(signed short)m68k_fetchW(); 
  pc+=2;
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M]+fw;
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  m68k_READ_B(abus);
#else
  m68k_READ_B(areg[PARAM_M]+fw);
#endif
}


void m68k_get_source_101_w(){ // .W (d16, An)
  //  (d16,An)        |          8(2/0) |                        np    nr           
  CPU_ABUS_ACCESS_READ_FETCH;//np
  register int fw=(signed short)m68k_fetchW(); 
  pc+=2;
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ;//nr
  m68k_READ_W(abus);
#else
  CPU_ABUS_ACCESS_READ;//nr
  m68k_READ_W(areg[PARAM_M]+fw);
#endif
}


void m68k_get_source_101_l(){ // .L (d16, An)
  //  (d16,An)        |         12(3/0) |                        np nR nr           
  CPU_ABUS_ACCESS_READ_FETCH;//np
  register int fw=(signed short)m68k_fetchW();
  pc+=2;
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_READ_L(abus);
#else
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_READ_L(areg[PARAM_M]+fw);
#endif
}

// (d8,An,Xn)

void m68k_get_source_110_b(){ // .B (d8,An,Xn)
  //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH;//np
  WORD w=m68k_fetchW();
  pc+=2;
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  CPU_ABUS_ACCESS_READ;//nr
  m68k_READ_B_FROM_ADDR
}


void m68k_get_source_110_w(){ // .W (d8,An,Xn)
  //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH; //np
  WORD w=m68k_fetchW();
  pc+=2;
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W_FROM_ADDR
}


void m68k_get_source_110_l(){ // .L (d8,An,Xn)
  //  (d8,An,Xn)      |         14(3/0) |                   n    np nR nr           
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH; //np
  WORD w=m68k_fetchW();
  pc+=2; 
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  CPU_ABUS_ACCESS_READ_L; //nR nr
  m68k_READ_L_FROM_ADDR
}

// (xxx).W, (xxx).L, (d16, PC), (d8, PC, Xn), #<data>

void m68k_get_source_111_b(){

  switch(ir&0x7){
  case 0:{ // .B (xxx).W
    //  (xxx).W         |          8(2/0) |                        np    nr           
    TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=fw;
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_B(abus)
#else
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_B(fw)
#endif
    break;
  }
  case 1:{ // .B (xxx).L
    //  (xxx).L         |         12(3/0) |                     np np    nr    
    TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    register MEM_ADDRESS fl=m68k_fetchL();
    pc+=4;  
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=fl;
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_B(abus)
#else
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_B(fl)
#endif
    break;
   }

  case 2:{ // .B (d16, PC)
    //  (d16,An)        |          8(2/0) |                        np    nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=ad;
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_B(abus)
#else
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_B(ad)
#endif
    break;
  }
  case 3:{ // .B (d8, PC, Xn)
    //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    PC_RELATIVE_MONITOR(abus);
    pc+=2;
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_B_FROM_ADDR
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }
  case 4:{ // .B #<data>
    //  #<data>         |          4(1/0) |                        np                 
    TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_FETCH;//np
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
    //  (xxx).W         |          8(2/0) |                        np    nr           
    TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
#if defined(SSE_CPU_DATABUS)
///    TRACE("dbus %x -> %x\n", dbus,IRC);
    dbus=IRC;
#endif
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    m68k_READ_W(abus)
#else
    m68k_READ_W(fw)
#endif
    break;
  }case 1:{ // .W (xxx).L
    //  (xxx).L         |         12(3/0) |                     np np    nr    
    TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    register MEM_ADDRESS fl=m68k_fetchL();
    pc+=4;  
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=fl;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    m68k_READ_W(abus)
#else
    m68k_READ_W(fl)
#endif
    break;
  }case 2:{ // .W (d16, PC)
    //  (d16,An)        |          8(2/0) |                        np    nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=ad;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    m68k_READ_W(abus)
#else
    m68k_READ_W(ad)
#endif
    break;
  }case 3:{ // .W (d8, PC, Xn)
    //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    CPU_ABUS_ACCESS_READ;//nr
    m68k_READ_W_FROM_ADDR
    PC_RELATIVE_MONITOR(abus);
    pc+=2; 
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }case 4:{ // .W #<data>
    //  #<data>         |          4(1/0) |                        np                 
    TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_FETCH;//np
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
    //  (xxx).W         |         12(3/0) |                        np nR nr           
    TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    m68k_READ_L(abus) 
#else
    m68k_READ_L(fw) 
#endif
    break;
  }case 1:{ // .L (xxx).L
    //  (xxx).L         |         16(4/0) |                     np np nR nr           
    TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    register MEM_ADDRESS fl=m68k_fetchL();
    pc+=4;  
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=fl;
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    m68k_READ_L(abus)
#else
    m68k_READ_L(fl)
#endif
    break;
  }case 2:{ // .L (d16, PC)
    //  (d16,An)        |         12(3/0) |                        np nR nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    abus=ad;
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA)
    m68k_READ_L(abus)
#else
    m68k_READ_L(ad)
#endif
    break;
  }case 3:{ // .L (d8, PC, Xn)
    //  (d8,An,Xn)      |         14(3/0) |                   n    np nR nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();
    if(m68k_iriwo&BIT_b){  //.l
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=PC_RELATIVE_PC+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    PC_RELATIVE_MONITOR(abus);
    pc+=2;
    CPU_ABUS_ACCESS_READ_L;//nR nr
    m68k_READ_L_FROM_ADDR
    break;       //what do bits 8,9,a  of extra word do?  (not always 0)
  }case 4:{ // .B #<data>
    //  #<data>         |          8(2/0) |                     np np                 
    TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    m68k_src_l=m68k_fetchL();
    pc+=4;  
    break;
  }default:
    ILLEGAL;
  }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//////////////////////////    GET DEST       ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*  For some instructions, EA microcodes are used and the data is read even if
    it isn't necessary for the logic of the instruction.
    CLR etc. read before write.
    If we really are in a write cycle, PC should be at its definitive value.
    If it's a read cycle, we use the <EA> rules for PC.
*/


void m68k_get_dest_000_b(){ 
  //  Dn              |          0(0/0) |
  m68k_dest=r+PARAM_M; 
}


void m68k_get_dest_000_w(){ 
  //  Dn              |          0(0/0) |
  m68k_dest=r+PARAM_M; 
}


void m68k_get_dest_000_l(){ 
  //  Dn              |          0(0/0) |
  m68k_dest=r+PARAM_M; 
}


void m68k_get_dest_001_b(){ 
  //  An              |          0(0/0) |
  m68k_dest=areg+PARAM_M; 
}


void m68k_get_dest_001_w(){ 
  //  An              |          0(0/0) |
  m68k_dest=areg+PARAM_M; 
}


void m68k_get_dest_001_l(){ 
  //  An              |          0(0/0) |
  m68k_dest=areg+PARAM_M; 
}


void m68k_get_dest_010_b(){ 
  //  (An)            |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ; //nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  m68k_SET_DEST_B(abus); 
#else
  m68k_SET_DEST_B(areg[PARAM_M]); 
#endif
}


void m68k_get_dest_010_w(){
  //  (An)            |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  m68k_SET_DEST_W(abus); 
#else
  m68k_SET_DEST_W(areg[PARAM_M]); 
#endif
}


void m68k_get_dest_010_l(){ 
  //  (An)            |          8(2/0) |                           nR nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  m68k_SET_DEST_L(abus); 
#else
  m68k_SET_DEST_L(areg[PARAM_M]); 
#endif
}


void m68k_get_dest_011_b(){
  //  (An)+           |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  m68k_SET_DEST_B(abus);
#else
  m68k_SET_DEST_B(areg[PARAM_M]);
#endif
  areg[PARAM_M]+=1;
  if(PARAM_M==7)
    areg[PARAM_M]++;
}


void m68k_get_dest_011_w(){
  //  (An)+           |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  m68k_SET_DEST_W(abus);
#else
  m68k_SET_DEST_W(areg[PARAM_M]);
#endif
  areg[PARAM_M]+=2; 
}


void m68k_get_dest_011_l(){
  //  (An)+           |          8(2/0) |                           nR nr           
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  m68k_SET_DEST_L(abus);
#else
  m68k_SET_DEST_L(areg[PARAM_M]);
#endif
  areg[PARAM_M]+=4; 
}


void m68k_get_dest_100_b(){ 
  //  -(An)           |          6(1/0) |                   n          nr           
  if(CHECK_READ)
    TRUE_PC+=2;
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  areg[PARAM_M]--;
  if(PARAM_M==7)
    areg[PARAM_M]--; 
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ;//nr
  m68k_SET_DEST_B(abus);
#else
  CPU_ABUS_ACCESS_READ;//nr
  areg[PARAM_M]-=1;
  if(PARAM_M==7)
    areg[PARAM_M]--; 
  m68k_SET_DEST_B(areg[PARAM_M]);
#endif
}


void m68k_get_dest_100_w(){ 
  //  -(An)           |          6(1/0) |                   n          nr           
  if(CHECK_READ)
    TRUE_PC+=2;
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  areg[PARAM_M]-=2;
  abus=areg[PARAM_M];
  m68k_SET_DEST_W(abus);
#else
  CPU_ABUS_ACCESS_READ;//nr
  areg[PARAM_M]-=2;
  m68k_SET_DEST_W(areg[PARAM_M]); 
#endif
}


void m68k_get_dest_100_l(){
  //  -(An)           |         10(2/0) |                   n       nR nr           
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  areg[PARAM_M]-=4;
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_SET_DEST_L(abus);  
#else
  CPU_ABUS_ACCESS_READ_L;//nR nr
  areg[PARAM_M]-=4;
  m68k_SET_DEST_L(areg[PARAM_M]);  
#endif
}


void m68k_get_dest_101_b(){
  //  (d16,An)        |          8(2/0) |                        np    nr           
  CPU_ABUS_ACCESS_READ_FETCH; //np
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_B(abus);
#else
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_B(areg[PARAM_M]+fw);
#endif
#else
  m68k_SET_DEST_B(areg[PARAM_M]+fw);
  CPU_ABUS_ACCESS_READ; //nr
#endif
}


void m68k_get_dest_101_w(){
  //  (d16,An)        |          8(2/0) |                        np    nr           
  CPU_ABUS_ACCESS_READ_FETCH; //np
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_W(abus);
#else
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_W(areg[PARAM_M]+fw);
#endif
#else
  m68k_SET_DEST_W(areg[PARAM_M]+fw);
  CPU_ABUS_ACCESS_READ; //nr
#endif
}


void m68k_get_dest_101_l(){
  //  (d16,An)        |         12(3/0) |                        np nR nr           
  CPU_ABUS_ACCESS_READ_FETCH; //np
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_SET_DEST_L(abus);
#else
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_SET_DEST_L(areg[PARAM_M]+fw);
#endif
#else
  m68k_SET_DEST_L(areg[PARAM_M]+fw);
  CPU_ABUS_ACCESS_READ_L;//nR nr
#endif
}


void m68k_get_dest_110_b(){
  //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
  INSTRUCTION_TIME(2);//n 
  CPU_ABUS_ACCESS_READ_FETCH;//np
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
#if defined(SSE_CPU_EA_383)
  CPU_ABUS_ACCESS_READ;//nr
  m68k_SET_DEST_B_TO_ADDR
#else
  m68k_SET_DEST_B_TO_ADDR
  CPU_ABUS_ACCESS_READ;//nr
#endif
}


void m68k_get_dest_110_w(){
  //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH;//np
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
#if defined(SSE_CPU_EA_383)
  CPU_ABUS_ACCESS_READ;//nr
  m68k_SET_DEST_W_TO_ADDR
#else
  m68k_SET_DEST_W_TO_ADDR
  CPU_ABUS_ACCESS_READ;//nr
#endif
}


void m68k_get_dest_110_l(){
  //  (d8,An,Xn)      |         14(3/0) |                   n    np nR nr           
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH;//np
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
#if defined(SSE_CPU_EA_383)
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_SET_DEST_L_TO_ADDR
#else
  m68k_SET_DEST_L_TO_ADDR
  CPU_ABUS_ACCESS_READ_L;//nR nr
#endif
}


void m68k_get_dest_111_b(){
  switch(ir&0x7){
  case 0:{
    //  (xxx).W         |          8(2/0) |                        np    nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
    if(CHECK_READ)
      TRUE_PC+=2;
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    m68k_SET_DEST_B(abus)
#else
    m68k_SET_DEST_B(fw)
#endif
#else
    m68k_SET_DEST_B(fw)
    CPU_ABUS_ACCESS_READ;//nr
#endif
    break;
  }
  
  case 1:{
    //  (xxx).L         |         12(3/0) |                     np np    nr           
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    register MEM_ADDRESS fw=m68k_fetchL();
    pc+=4;  
    if(CHECK_READ)
      TRUE_PC+=4;
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    m68k_SET_DEST_B(abus)
#else
    m68k_SET_DEST_B(fw)
#endif
#else
    m68k_SET_DEST_B(fw)
    CPU_ABUS_ACCESS_READ;//nr
#endif
    break;
  }default:
    ILLEGAL;
  }
}


void m68k_get_dest_111_w(){
  switch(ir&0x7){
  case 0:{
    //  (xxx).W         |          8(2/0) |                        np    nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    if(CHECK_READ)
      TRUE_PC+=2;
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    m68k_SET_DEST_W(abus)
#else
    m68k_SET_DEST_W(fw)
#endif
#else
    m68k_SET_DEST_W(fw)
    CPU_ABUS_ACCESS_READ;//nr
#endif
    break;
  }
  
  case 1:{
    //  (xxx).L         |         12(3/0) |                     np np    nr           
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    if(CHECK_READ)
      TRUE_PC+=4;
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    m68k_SET_DEST_W(abus)
#else
    m68k_SET_DEST_W(fw)
#endif
#else
    m68k_SET_DEST_W(fw)
    CPU_ABUS_ACCESS_READ;//nr
#endif
    break;
  }default:
    ILLEGAL;
  }
}


void m68k_get_dest_111_l(){
  switch(ir&0x7){
  case 0:{
    //  (xxx).W         |         12(3/0) |                        np nR nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
    if(CHECK_READ)
      TRUE_PC+=2;
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    m68k_SET_DEST_L(abus)
#else
    m68k_SET_DEST_L(fw)
#endif
#else
    m68k_SET_DEST_L(fw)
    CPU_ABUS_ACCESS_READ_L;//nR nr
#endif
    break;
  }
  
  case 1:{
    //  (xxx).L         |         16(4/0) |                     np np nR nr           
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    register MEM_ADDRESS fw=m68k_fetchL();pc+=4;  
    if(CHECK_READ)
      TRUE_PC+=4;
#if defined(SSE_CPU_EA_383)
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    abus=fw;
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS2A_EA2)
    m68k_SET_DEST_L(abus)
#else
    m68k_SET_DEST_L(fw)
#endif
#else
    m68k_SET_DEST_L(fw)
    CPU_ABUS_ACCESS_READ_L;//nR nr
#endif
    break;
  }default:
    ILLEGAL;
  }
}


void m68kReadBFromAddr() {
  abus&=0xffffff;
#if defined(SSE_MMU_RAM_TEST3)
  if(abus>=himem  || mmu_confused)
#else
  if(abus>=himem)
#endif
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
  else if(abus>=MEM_START_OF_USER_AREA || SUPERFLAG)
  {                                              
    DEBUG_CHECK_READ_B(abus);  
    m68k_src_b=(BYTE)(PEEK(abus));                  
  }
  else
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}


void m68kReadWFromAddr() {
  abus&=0xffffff;                                   
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    
#if defined(SSE_MMU_RAM_TEST3)
  if(abus>=himem  || mmu_confused)
#else
  else if(abus>=himem)
#endif
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
  else if(abus>=MEM_START_OF_USER_AREA||SUPERFLAG)
  {                                              
    DEBUG_CHECK_READ_W(abus);  
    m68k_src_w=DPEEK(abus);                  
  }
  else 
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}


void m68kReadLFromAddr() {
  abus&=0xffffff;                                   
  if(abus&1)
    exception(BOMBS_ADDRESS_ERROR,EA_READ,abus);    
#if defined(SSE_MMU_RAM_TEST3)
  if(abus>=himem  || mmu_confused)
#else
  else if(abus>=himem)
#endif
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
  else if(abus>=MEM_START_OF_USER_AREA||SUPERFLAG)
  {                                              
    DEBUG_CHECK_READ_L(abus);  
    m68k_src_l=LPEEK(abus);   
  }
  else
    exception(BOMBS_BUS_ERROR,EA_READ,abus);
}
