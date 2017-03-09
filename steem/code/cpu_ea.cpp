// this file is included in cpu_sse.cpp, which is a big file already
// ea stands for Effective Address

void m68kReadBFromAddr();
void m68kReadWFromAddr();
#if !defined(SSE_CPU_SPLIT_RL)
void m68kReadLFromAddr();
#endif

#define LOGSECTION LOGSECTION_CARTRIDGE

// peek

//can be used generally, not only for pure emulation

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
      if(tos_high && ad<(0xfc0000+192*1024))return ROM_PEEK(ad-rom_addr);
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=MEM_EXPANSION_CARTRIDGE){
      TRACE_LOG("PC %X read.b %X = %X\n",old_pc,ad,cart?CART_PEEK(ad-MEM_EXPANSION_CARTRIDGE):0xFF);
#if defined(SSE_CARTRIDGE_BAT)
/*  See m68k_dpeek().
    B.A.T I and II and Music Master use MOVE.W.
    Drumbeat for the Replay 16 cartridge uses MOVE.B.
 */
      if (cart)
      {
        DWORD cart_addr=ad-MEM_EXPANSION_CARTRIDGE; // DWORD, not WORD (128KB), EmuTOS cartridge
        if(SSEConfig.mv16 && cart_addr>4) //not when checking for presence
          dma_mv16_fetch((WORD)cart_addr);
        return CART_PEEK(cart_addr);
      }
#else
      if (cart) return CART_PEEK(ad-MEM_EXPANSION_CARTRIDGE);
#endif
      else return 0xff;
    }else if(ad>=rom_addr){
      if (ad<(0xe00000+256*1024)) return ROM_PEEK(ad-rom_addr);
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xff;
    }else if (mmu_confused){
      return mmu_confused_peek(ad,true);
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
      if(tos_high && ad<(0xfc0000+192*1024))return ROM_DPEEK(ad-rom_addr);
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if(ad>=MEM_EXPANSION_CARTRIDGE){
      TRACE_LOG("PC %X read.w %X = %X\n",old_pc,ad,cart?CART_DPEEK(ad-MEM_EXPANSION_CARTRIDGE):0xFFFF);
#if defined(SSE_CARTRIDGE_BAT)
/*  The MV16 cartridge was designed for the game B.A.T.
    It plays samples sent through reading an address on the 
    cartridge (address=data).
*/
      if (cart)
      {
        DWORD cart_addr=ad-MEM_EXPANSION_CARTRIDGE;
        if(SSEConfig.mv16)
          dma_mv16_fetch((WORD)cart_addr);
        return CART_DPEEK(cart_addr);
      }
#else
      if (cart) return CART_DPEEK(ad-MEM_EXPANSION_CARTRIDGE);
#endif
      else return 0xffff;
    }else if(ad>=rom_addr){
      if (ad<(0xe00000+256*1024)) return ROM_DPEEK(ad-rom_addr);
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xffff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xffff;
    }else if(mmu_confused){
      return mmu_confused_dpeek(ad,true);
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
      if(tos_high && ad<(0xfc0000+192*1024-2))return ROM_LPEEK(ad-rom_addr);
      else if (ad<0xfe0000 || ad>=0xfe2000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
    }else if (ad>=MEM_EXPANSION_CARTRIDGE){
      TRACE_LOG("PC %X read.l %X = %X\n",old_pc,ad,cart?CART_LPEEK(ad-MEM_EXPANSION_CARTRIDGE):0xFFFFFF);
      if (cart) return CART_LPEEK(ad-MEM_EXPANSION_CARTRIDGE);
      else return 0xffffffff;
    }else if (ad>=rom_addr){
      if (ad<(0xe00000+256*1024-2)) return ROM_LPEEK(ad-rom_addr);
      if (ad>=0xec0000) exception(BOMBS_BUS_ERROR,EA_READ,ad);
      return 0xffffffff;
    }else if (ad>=0xd00000 && ad<0xd80000){
      return 0xffffffff;
    }else if (mmu_confused){
      return mmu_confused_lpeek(ad,true);
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

#if defined(SSE_CPU_392) // refactoring: one return 

BYTE m68k_read_dest_b(){ //only used by tst.b, cmpi.b
  BYTE x=0;
  switch(ir&BITS_543){
  case BITS_543_000:
    x=LOBYTE(r[PARAM_M]);
    break;
  case BITS_543_001:
    m68k_unrecognised();
    break;
  case BITS_543_010:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    x=m68k_peek(abus);
    break;
  case BITS_543_011:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    x=m68k_peek(abus);
    areg[PARAM_M]++;
    if(PARAM_M==7)
      areg[7]++;
    break;
  case BITS_543_100:
    areg[PARAM_M]--;
    if (PARAM_M==7) 
      areg[7]--;
    INSTRUCTION_TIME(2); //n
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    x=m68k_peek(abus);
    break;
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_peek(abus);
    break;
  }case BITS_543_110:
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_peek(abus);
    break;
  case BITS_543_111:
    switch(ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_peek(abus);
      break;
    }case 1:{
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      abus=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_peek(abus);
      break;
    }default:
      m68k_unrecognised();
    }
  }
#if defined(SSE_BLT_392A)
  if (ioaccess & IOACCESS_FLAG_DO_BLIT)
    Blitter_Start_Now(); 
#endif
  return x;
}


WORD m68k_read_dest_w(){ // //only used by tst.w, cmpi.w
  WORD x=0;
  switch(ir&BITS_543){
  case BITS_543_000:
    x=LOWORD(r[PARAM_M]);
    break;
  case BITS_543_001:
    m68k_unrecognised();
    break;
  case BITS_543_010:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(abus);
    break;
  case BITS_543_011:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(abus);
    areg[PARAM_M]+=2;
    break;
  case BITS_543_100:
    INSTRUCTION_TIME(2);//n
    areg[PARAM_M]-=2;
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(abus);
    break;
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(abus);
    break;
  }case BITS_543_110:
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(abus);
    break;
  case BITS_543_111:
    switch(ir&ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_dpeek(abus);
      break;
    }case 1:{
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      abus=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_dpeek(abus);
      break;
    }default:
      m68k_unrecognised();
    }
  }
#if defined(SSE_BLT_392A)
  if (ioaccess & IOACCESS_FLAG_DO_BLIT)
    Blitter_Start_Now(); 
#endif
  return x;
}


#else

BYTE m68k_read_dest_b(){ //only used by tst.b, cmpi.b
  BYTE x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return LOBYTE(r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    return m68k_peek(abus);
  case BITS_543_011:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    x=m68k_peek(abus);
    areg[PARAM_M]++;
    if(PARAM_M==7)
      areg[7]++;
    return x;
  case BITS_543_100:
    areg[PARAM_M]--;
    if (PARAM_M==7) 
      areg[7]--;
    INSTRUCTION_TIME(2); //n
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nr
    return m68k_peek(abus);
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_peek(abus);
    return x;
  }case BITS_543_110:
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    CPU_ABUS_ACCESS_READ;//nr
    return m68k_peek(abus);
  case BITS_543_111:
    switch(ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_peek(abus);
      return x;
    }case 1:{
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      abus=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_peek(abus);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}


WORD m68k_read_dest_w(){ // //only used by tst.w, cmpi.w
  WORD x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return LOWORD(r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ;//nr
    return m68k_dpeek(abus);
  case BITS_543_011:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(abus);
    areg[PARAM_M]+=2;
    return x;
  case BITS_543_100:
    INSTRUCTION_TIME(2);//n
    areg[PARAM_M]-=2;
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ;//nr
    return m68k_dpeek(abus);
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |   8(2/0)   |              np    nr     
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ;//nr
    x=m68k_dpeek(abus);
    return x;
  }case BITS_543_110:
//  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    CPU_ABUS_ACCESS_READ;//nr
    return m68k_dpeek(abus);
  case BITS_543_111:
    switch(ir&ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |   8(2/0)   |              np    nr       
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_dpeek(abus);
      return x;
    }case 1:{
//  (xxx).L         | 111 | 001 |  12(3/0)   |           np np    nr    
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      abus=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ;//nr
      x=m68k_dpeek(abus);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}

#endif

#if defined(SSE_CPU_SPLIT_RL) //we read word by word, a bit fancy...

LONG m68k_read_dest_l(){ //only used by tst.l, cmpi.l
  LONG x=0;
  switch(ir&BITS_543){
  case BITS_543_000:
    x=r[PARAM_M];
    break;
  case BITS_543_001:
    m68k_unrecognised();
    break;
  case BITS_543_010:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nR
    dbus=m68k_dpeek(abus);
    x=dbus<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    dbus=m68k_dpeek(abus);
    x|=dbus;
    break;
  case BITS_543_011:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nR
    dbus=m68k_dpeek(abus);
    x=dbus<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    dbus=m68k_dpeek(abus);
    x|=dbus;
    areg[PARAM_M]+=4;
    break;
  case BITS_543_100:
    INSTRUCTION_TIME(2);//n
    areg[PARAM_M]-=4;
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ; //nR
    dbus=m68k_dpeek(abus);
    x=dbus<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    dbus=m68k_dpeek(abus);
    x|=dbus;
    break;
  case BITS_543_101:
    //  (d16,An)        | 101 | reg |  12(3/0)   |              np nR nr          
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ; //nR
    dbus=m68k_dpeek(abus);
    x=dbus<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    dbus=m68k_dpeek(abus);
    x|=dbus;
    break;
  case BITS_543_110:
//  (d8,An,Xn)      | 110 | reg |  14(3/0)   |         n    np nR nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    CPU_ABUS_ACCESS_READ; //nR
    dbus=m68k_dpeek(abus);
    x=dbus<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    dbus=m68k_dpeek(abus);
    x|=dbus;
    break;
  case BITS_543_111:
    switch(ir&0x7){
    case 0:
//  (xxx).W         | 111 | 000 |  12(3/0)   |              np nR nr      
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ; //nR
      dbus=m68k_dpeek(abus);
      x=dbus<<16;
      abus+=2;
      CPU_ABUS_ACCESS_READ; //nr
      dbus=m68k_dpeek(abus);
      x|=dbus;
      break;
    case 1:
//(xxx).L         | 111 | 001 |  16(4/0)   |           np np nR nr           
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      abus=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ; //nR
      dbus=m68k_dpeek(abus);
      x=dbus<<16;
      abus+=2;
      CPU_ABUS_ACCESS_READ; //nr
      dbus=m68k_dpeek(abus);
      x|=dbus;
      break;
    default:
      m68k_unrecognised();
      break;
    }
    break;
  }
#if defined(SSE_BLT_392A)
  if (ioaccess & IOACCESS_FLAG_DO_BLIT)
    Blitter_Start_Now(); 
#endif
  return x;
}

#else

LONG m68k_read_dest_l(){
  LONG x;
  switch(ir&BITS_543){
  case BITS_543_000:
    return (r[PARAM_M]);
  case BITS_543_001:
    m68k_unrecognised();break;
  case BITS_543_010:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ_L;//nR nr
    return m68k_lpeek(abus);
  case BITS_543_011:
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ_L;//nR nr
    x=m68k_lpeek(abus);
    areg[PARAM_M]+=4;
    return x;
  case BITS_543_100:
    INSTRUCTION_TIME(2);//n
    areg[PARAM_M]-=4;
    abus=areg[PARAM_M];
    CPU_ABUS_ACCESS_READ_L;//nR nr
    return m68k_lpeek(abus);
  case BITS_543_101:{
    //  (d16,An)        | 101 | reg |  12(3/0)   |              np nR nr          
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=areg[PARAM_M]+(signed short)m68k_fetchW();pc+=2; 
    CPU_ABUS_ACCESS_READ_L;//nR nr
    x=m68k_lpeek(abus);
    return x;
  }case BITS_543_110:
//  (d8,An,Xn)      | 110 | reg |  14(3/0)   |         n    np nR nr           
    INSTRUCTION_TIME(2);//n
    CPU_ABUS_ACCESS_READ_FETCH;//np
    m68k_iriwo=m68k_fetchW();pc+=2; 
    if(m68k_iriwo&BIT_b){  //.l
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
    }else{         //.w
      abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
    }
    CPU_ABUS_ACCESS_READ_L;////nR nr
    return m68k_lpeek(abus);
  case BITS_543_111:
    switch(ir&0x7){
    case 0:{
//  (xxx).W         | 111 | 000 |  12(3/0)   |              np nR nr      
      CPU_ABUS_ACCESS_READ_FETCH;//np
      abus=0xffffff&(unsigned long)((signed long)((signed short)m68k_fetchW()));
      pc+=2; 
      CPU_ABUS_ACCESS_READ_L;//nR nr
      x=m68k_lpeek(abus);
      return x;
    }case 1:{
//(xxx).L         | 111 | 001 |  16(4/0)   |           np np nR nr           
      CPU_ABUS_ACCESS_READ_FETCH_L;//np np
      abus=m68k_fetchL()&0xffffff;
      pc+=4;  
      CPU_ABUS_ACCESS_READ_L;//nR nr
      x=m68k_lpeek(abus);
      return x;
    }default:
      m68k_unrecognised();
    }
  }
  return 0;
}

#endif



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
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=areg[PARAM_M];
#if defined(SSE_CPU_SPLIT_RL) 
  CPU_ABUS_ACCESS_READ; //nR
  m68k_READ_W(abus)
  m68k_src_l=m68k_src_w<<16;
  //LONG y=m68k_src_l;
  abus+=2;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(abus)
  m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?

  //LONG x=m68k_lpeek(abus-2) ;
  //ASSERT(m68k_src_l==x);
 
#else
  CPU_ABUS_ACCESS_READ_L; //nR nr
  m68k_READ_L(abus) 
#endif
#else
  CPU_ABUS_ACCESS_READ_L; //nR nr
  m68k_READ_L(areg[PARAM_M]) 
#endif
}

// (An)+

void m68k_get_source_011_b(){ // .B (An)+
  //  (An)+           |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_READ_W(abus) 
#else
  m68k_READ_W(areg[PARAM_M]) 
#endif
  areg[PARAM_M]+=2; 
}


void m68k_get_source_011_l(){ // .L (An)+
  //  (An)+           |          8(2/0) |                           nR nr           
#if defined(SSE_CPU_SPLIT_RL) 
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ; //nR
  m68k_READ_W(abus)
  m68k_src_l=m68k_src_w<<16;
  abus+=2;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(abus)
  m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
  areg[PARAM_M]+=4; // for .L, we assume ++ post read
#else
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_READ_L(abus) 
#else
  m68k_READ_L(areg[PARAM_M]) 
#endif
  areg[PARAM_M]+=4; // for .L, we assume ++ post read
#endif//#if defined(SSE_CPU_SPLIT_RL) 
}

// -(An)

void m68k_get_source_100_b(){ // .B -(An)
  //  -(An)           |          6(1/0) |                   n          nr           
  TRUE_PC+=2;
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS)
  areg[PARAM_M]--;
  if(PARAM_M==7)
    areg[7]--;
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
  areg[PARAM_M]-=2;
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_READ_W(abus)  
#else
  areg[PARAM_M]-=2;
  m68k_READ_W(areg[PARAM_M])  
#endif
  
}


void m68k_get_source_100_l(){ // .L -(An)
  //  -(An)           |         10(2/0) |                   n       nR nr           
#if defined(SSE_CPU_SPLIT_RL) 
  INSTRUCTION_TIME(2);//n
  areg[PARAM_M]-=4;
  abus=areg[PARAM_M];
  CPU_ABUS_ACCESS_READ; //nR
  m68k_READ_W(abus)
  m68k_src_l=m68k_src_w<<16;
  abus+=2;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(abus)
  m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
#else
  INSTRUCTION_TIME(2);//n
#if defined(SSE_MMU_ROUNDING_BUS)
  areg[PARAM_M]-=4;
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_READ_L(abus) 
#else
  areg[PARAM_M]-=4;
  m68k_READ_L(areg[PARAM_M])  
#endif
#endif//#if defined(SSE_CPU_SPLIT_RL)   
}


// (d16, An)

void m68k_get_source_101_b(){ //.B (d16, An)
  //  (d16,An)        |          8(2/0) |                        np    nr           
  CPU_ABUS_ACCESS_READ_FETCH; //np
  register int fw=(signed short)m68k_fetchW(); 
  pc+=2;
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=areg[PARAM_M]+fw;
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_CPU_SPLIT_RL)
  CPU_ABUS_ACCESS_READ_FETCH;//np
  register int fw=(signed short)m68k_fetchW();
  pc+=2;
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ; //nR
  m68k_READ_W(abus)
  m68k_src_l=m68k_src_w<<16;
  abus+=2;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(abus)
  m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
#else
  CPU_ABUS_ACCESS_READ_FETCH;//np
  register int fw=(signed short)m68k_fetchW();
  pc+=2;
#if defined(SSE_MMU_ROUNDING_BUS)
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_READ_L(abus);
#else
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_READ_L(areg[PARAM_M]+fw);
#endif
#endif//#if defined(SSE_CPU_SPLIT_RL)
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
#if defined(SSE_CPU_SPLIT_RL)
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH; //np
  WORD w=m68k_fetchW();
  pc+=2; 
  if(w&BIT_b){  //.l
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(int)r[w>>12];
  }else{         //.w
    abus=areg[PARAM_M]+(signed char)LOBYTE(w)+(signed short)r[w>>12];
  }
  CPU_ABUS_ACCESS_READ; //nR
  m68k_READ_W(abus)
  m68k_src_l=m68k_src_w<<16;
  abus+=2;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_READ_W(abus)
  m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
#else
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
#endif//#if defined(SSE_CPU_SPLIT_RL)
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
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=PC_RELATIVE_PC+(signed short)m68k_fetchW();
#else
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
#endif
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
#if defined(SSE_MMU_ROUNDING_BUS)
#if !defined(SSE_MMU_ROUNDING_BUS)
    abus=ad;
#endif
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
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=(signed short)m68k_fetchW(); // the cast is important for sign extension!
#else
    register signed int fw=(signed short)m68k_fetchW();
#endif
    pc+=2; 
#if defined(SSE_CPU_DATABUS)
    dbus=IRC;
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_W(abus)
#else
    m68k_READ_W(fw)
#endif
    break;
  }case 1:{ // .W (xxx).L
    //  (xxx).L         |         12(3/0) |                     np np    nr    
    TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=m68k_fetchL();
#else
    register MEM_ADDRESS fl=m68k_fetchL();
#endif
    pc+=4;  
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_W(abus)
#else
    m68k_READ_W(fl)
#endif
    break;
  }case 2:{ // .W (d16, PC)
    //  (d16,An)        |          8(2/0) |                        np    nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(abus);
#else
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=ad;
#endif
#endif
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
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

#if defined(SSE_CPU_SPLIT_RL)

void m68k_get_source_111_l(){

  switch(ir&0x7){
  case 0:{ // .L (xxx).W
    //  (xxx).W         |         12(3/0) |                        np nR nr           
    TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=(signed short)m68k_fetchW();
    pc+=2; 
    CPU_ABUS_ACCESS_READ; //nR
    m68k_READ_W(abus)
    m68k_src_l=m68k_src_w<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    m68k_READ_W(abus)
    m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
    break;
  }case 1:{ // .L (xxx).L
    //  (xxx).L         |         16(4/0) |                     np np nR nr           
    TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
    abus=m68k_fetchL();
    pc+=4;  
    CPU_ABUS_ACCESS_READ; //nR
    m68k_READ_W(abus)
    m68k_src_l=m68k_src_w<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    m68k_READ_W(abus)
    m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
    break;
  }case 2:{ // .L (d16, PC)
    //  (d16,An)        |         12(3/0) |                        np nR nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
    abus=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(abus);
    CPU_ABUS_ACCESS_READ; //nR
    m68k_READ_W(abus)
    m68k_src_l=m68k_src_w<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    m68k_READ_W(abus)
    m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
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
    CPU_ABUS_ACCESS_READ; //nR
    m68k_READ_W(abus)
    m68k_src_l=m68k_src_w<<16;
    abus+=2;
    CPU_ABUS_ACCESS_READ; //nr
    m68k_READ_W(abus)
    m68k_src_l|=(WORD)m68k_src_w; //nice crash if no WORD cast... sign extension?
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

#else

void m68k_get_source_111_l(){

  switch(ir&0x7){
  case 0:{ // .L (xxx).W
    //  (xxx).W         |         12(3/0) |                        np nR nr           
    TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_FETCH;//np
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=(signed short)m68k_fetchW();
    pc+=2; 
#else
    register signed int fw=(signed short)m68k_fetchW();
    pc+=2; 
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=fw;
#endif
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_L(abus) 
#else
    m68k_READ_L(fw) 
#endif
    break;
  }case 1:{ // .L (xxx).L
    //  (xxx).L         |         16(4/0) |                     np np nR nr           
    TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=m68k_fetchL();
    pc+=4;  
#else
    register MEM_ADDRESS fl=m68k_fetchL();
    pc+=4;  
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=fl;
#endif
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_READ_L(abus)
#else
    m68k_READ_L(fl)
#endif
    break;
  }case 2:{ // .L (d16, PC)
    //  (d16,An)        |         12(3/0) |                        np nR nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(abus);
#else
    register MEM_ADDRESS ad=PC_RELATIVE_PC+(signed short)m68k_fetchW();
    pc+=2;
    PC_RELATIVE_MONITOR(ad);
#if defined(SSE_MMU_ROUNDING_BUS)
    abus=ad;
#endif
#endif
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
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

#endif//#if defined(SSE_CPU_SPLIT_RL)

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

/*  note for TRUE_PC
    For some instructions, EA microcodes are used and the data is read even if
    it isn't necessary for the logic of the instruction.
    CLR etc. read before write.
    If we really are in a write cycle, PC should be at its definitive value.
    If it's a read cycle, we use the <EA> rules for PC.
*/

/*  GET DEST implies reading the destination, that's why timing is counted.
    Steem sets the void pointer m68k_dest, if reading is useful it happens in
    the instruction... 
    TODO: systematically use read_dest instead?
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
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS) //for when PREFETCH_IRC modifies abus (not the case now)
  dest_addr=  
#endif
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ; //nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_SET_DEST_B(abus); 
#else
  m68k_SET_DEST_B(areg[PARAM_M]); 
#endif
}


void m68k_get_dest_010_w(){
  //  (An)            |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_SET_DEST_W(abus); 
#else
  m68k_SET_DEST_W(areg[PARAM_M]); 
#endif
}


void m68k_get_dest_010_l(){ 
  //  (An)            |          8(2/0) |                           nR nr           
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_SET_DEST_L(abus); 
#else
  m68k_SET_DEST_L(areg[PARAM_M]); 
#endif
}


void m68k_get_dest_011_b(){
  //  (An)+           |          4(1/0) |                              nr           
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
  m68k_SET_DEST_W(abus);
#else
  m68k_SET_DEST_W(areg[PARAM_M]);
#endif
  areg[PARAM_M]+=2; 
}


void m68k_get_dest_011_l(){
  //  (An)+           |          8(2/0) |                           nR nr           
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M];
#endif
  CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
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
#if defined(SSE_MMU_ROUNDING_BUS)
  areg[PARAM_M]--;
  if(PARAM_M==7)
    areg[PARAM_M]--; 
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
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
#if defined(SSE_MMU_ROUNDING_BUS)
  areg[PARAM_M]-=2;
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
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
#if defined(SSE_MMU_ROUNDING_BUS)
  areg[PARAM_M]-=4;
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
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
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_B(abus);
#else
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_B(areg[PARAM_M]+fw);
#endif
}


void m68k_get_dest_101_w(){
  //  (d16,An)        |          8(2/0) |                        np    nr           
  CPU_ABUS_ACCESS_READ_FETCH; //np
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_W(abus);
#else
  CPU_ABUS_ACCESS_READ; //nr
  m68k_SET_DEST_W(areg[PARAM_M]+fw);
#endif
}


void m68k_get_dest_101_l(){
  //  (d16,An)        |         12(3/0) |                        np nR nr           
  CPU_ABUS_ACCESS_READ_FETCH; //np
  register signed int fw=(signed short)m68k_fetchW();
  pc+=2; 
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=  
#endif
  abus=areg[PARAM_M]+fw;
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_SET_DEST_L(abus);
#else
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_SET_DEST_L(areg[PARAM_M]+fw);
#endif
}


void m68k_get_dest_110_b(){
  //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
  INSTRUCTION_TIME(2);//n 
  CPU_ABUS_ACCESS_READ_FETCH;//np
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  CPU_ABUS_ACCESS_READ;//nr
  m68k_SET_DEST_B_TO_ADDR
}


void m68k_get_dest_110_w(){
  //  (d8,An,Xn)      |         10(2/0) |                   n    np    nr           
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH;//np
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  CPU_ABUS_ACCESS_READ;//nr
  m68k_SET_DEST_W_TO_ADDR
}


void m68k_get_dest_110_l(){
  //  (d8,An,Xn)      |         14(3/0) |                   n    np nR nr           
  INSTRUCTION_TIME(2);//n
  CPU_ABUS_ACCESS_READ_FETCH;//np
  m68k_iriwo=m68k_fetchW();
  pc+=2; 
  if(m68k_iriwo&BIT_b){  //.l
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(int)r[m68k_iriwo>>12];
  }else{         //.w
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=areg[PARAM_M]+(signed char)LOBYTE(m68k_iriwo)+(signed short)r[m68k_iriwo>>12];
  }
  CPU_ABUS_ACCESS_READ_L;//nR nr
  m68k_SET_DEST_L_TO_ADDR
}

//#undef SSE_MMU_ROUNDING_BUS
void m68k_get_dest_111_b(){
  switch(ir&0x7){
  case 0:{
    //  (xxx).W         |          8(2/0) |                        np    nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=(signed short)m68k_fetchW(); //cast important for sign extension!
#else
    register signed int fw=(signed short)m68k_fetchW();
#endif
    pc+=2; 
    if(CHECK_READ)
      TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_SET_DEST_B(abus)
#else
    m68k_SET_DEST_B(fw)
#endif
    break;
  }
  
  case 1:{
    //  (xxx).L         |         12(3/0) |                     np np    nr           
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=m68k_fetchL();
#else
    register MEM_ADDRESS fw=m68k_fetchL();
#endif
    pc+=4;  
    if(CHECK_READ)
      TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_SET_DEST_B(abus)
#else
    m68k_SET_DEST_B(fw)
#endif
    break;
  }default:
    ILLEGAL;
  }
}

//#define SSE_MMU_ROUNDING_BUS
void m68k_get_dest_111_w(){
  switch(ir&0x7){
  case 0:{
    //  (xxx).W         |          8(2/0) |                        np    nr           
    CPU_ABUS_ACCESS_READ_FETCH;//np
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=(signed short)m68k_fetchW();pc+=2; 
#else
    register signed int fw=(signed short)m68k_fetchW();pc+=2; 
#endif
    if(CHECK_READ)
      TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_SET_DEST_W(abus)
#else
    m68k_SET_DEST_W(fw)
#endif
    break;
  }
  
  case 1:{
    //  (xxx).L         |         12(3/0) |                     np np    nr           
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=m68k_fetchL();
#else
    register MEM_ADDRESS fw=m68k_fetchL();
#endif
    pc+=4;  
    if(CHECK_READ)
      TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ;//nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_SET_DEST_W(abus)
#else
    m68k_SET_DEST_W(fw)
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
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=(signed short)m68k_fetchW();
#else
    register signed int fw=(signed short)m68k_fetchW();
#endif
    pc+=2; 
    if(CHECK_READ)
      TRUE_PC+=2;
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_SET_DEST_L(abus)
#else
    m68k_SET_DEST_L(fw)
#endif
    break;
  }
  
  case 1:{
    //  (xxx).L         |         16(4/0) |                     np np nR nr           
    CPU_ABUS_ACCESS_READ_FETCH_L;//np np
#if defined(SSE_MMU_ROUNDING_BUS)
#if defined(SSE_CPU_RESTORE_ABUS)
    dest_addr=  
#endif
    abus=m68k_fetchL();
#else
    register MEM_ADDRESS fw=m68k_fetchL();
#endif
    pc+=4;  
    if(CHECK_READ)
      TRUE_PC+=4;
    CPU_ABUS_ACCESS_READ_L;//nR nr
#if defined(SSE_MMU_ROUNDING_BUS)
    m68k_SET_DEST_L(abus)
#else
    m68k_SET_DEST_L(fw)
#endif
    break;
  }default:
    ILLEGAL;
  }
}


void m68kReadBFromAddr() {
  abus&=0xffffff;
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=abus;
#endif
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
    else if(mmu_confused)
      m68k_src_b=mmu_confused_peek(abus,true);                                         
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
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=abus;
#endif
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
    else if(mmu_confused)
      m68k_src_w=mmu_confused_dpeek(abus,true);                                         
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

#if !defined(SSE_CPU_SPLIT_RL)
void m68kReadLFromAddr() {
  abus&=0xffffff;                                   
#if defined(SSE_CPU_RESTORE_ABUS)
  dest_addr=abus;
#endif
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
    else if (mmu_confused)
      m68k_src_l=mmu_confused_lpeek(abus,true);                                         
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
#endif

#undef LOGSECTION
