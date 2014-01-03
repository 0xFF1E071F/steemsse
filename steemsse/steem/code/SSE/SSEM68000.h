#pragma once
#ifndef SSEM68000_H
#define SSEM68000_H
//#pragma message("SSEM68000_H")

#if defined(SS_STRUCTURE_SSECPU_OBJ)
#include "SSEDebug.h"
#endif


/* Because of structure problems, we create this apart file that just contains
   the TM68000 struct definition and some inline functions.
   It's a mess again but at least we avoid double definition.
*/

/*  We now have a nice object representing the Motorola 68000, with some 
    functions, but the instructions stay global C
*/
struct TM68000 {
  TM68000();
  inline void FetchTiming();
  inline void FetchTimingNoRound();
  inline void FetchWord(WORD &dest_word); 
  inline void InstructionTime(int n);
  inline void InstructionTimeRound(int n);
  inline void Interrupt(MEM_ADDRESS ad);
  inline void PerformRte();
  inline void PrefetchSetPC();
  inline void Process();
  inline void m68kReadBFromAddr();
  inline void m68kReadWFromAddr();
  inline void m68kReadLFromAddr();
  void SetPC(MEM_ADDRESS ad);
  inline void Unstop();
  void Reset(bool cold);
#if defined(SS_DEBUG)
  int IrAddress; // pc at start
  int nExceptions;
  int nInstr;
  WORD PreviousIr;
  bool NextIrFetched; // fetched next instruction?
#endif//debug
#if defined(SS_CPU_EXCEPTION)
//  BYTE GetSize(WORD ir);
#if defined(SS_CPU_TRUE_PC)
  MEM_ADDRESS Pc;
  bool CheckRead; // most 'write' instructions read before, crash at read...
#endif
#endif
  inline void GetDestByte();
  inline void GetDestWord();
  inline void GetDestLong();
  inline void GetDestByteNotA();
  inline void GetDestWordNotA();
  inline void GetDestLongNotA();
  inline void GetDestByteNotAOrD();
  inline void GetDestWordNotAOrD();
  inline void GetDestLongNotAOrD();
  inline void GetDestByteNotAFasterForD();
  inline void GetDestWordNotAFasterForD();
  inline void GetDestLongNotAFasterForD();
  inline void GetSourceByte(); 
  inline void GetSourceWord();
  inline void GetSourceLong();
  inline void GetSourceByteNotA(); 
  inline void GetSourceWordNotA();
  inline void GetSourceLongNotA();


#if defined(SS_CPU_PREFETCH)

#if defined(SS_CPU_PREFETCH_CLASS)
  int PrefetchClass; // see ijor's article
#endif

#if defined(SS_CPU_PREFETCH_CALL)
  BOOL CallPrefetch; 
  WORD FetchForCall(MEM_ADDRESS ad);
  WORD *PrefetchAddress; 
  WORD PrefetchedOpcode;
#endif

  inline void PrefetchIrc();
  inline void PrefetchIrcNoRound();
  inline void RefetchIr();

#endif
};

extern TM68000 M68000;


void TM68000::PrefetchSetPC() { 
  // called by SetPC; we don't count timing here

#if defined(SS_CPU_FETCH_IO)
  // don't use lpfetch for fetching in IO zone, use io_read: fixes Warp original
  if(pc>=MEM_IO_BASE && !(pc>=0xff8240 && pc<0xff8260))
  {
    prefetch_buf[0]=io_read_w(pc);
    prefetch_buf[1]=io_read_w(pc+2);
    TRACE("Set PC in IO zone %X\n",pc);
    prefetched_2=true;
    return;
  }
#endif
  prefetch_buf[0]=*lpfetch;
  prefetch_buf[1]=*(lpfetch+MEM_DIR); 
  prefetched_2=true; // was false in Steem 3.2
#if defined(SS_CPU_PREFETCH_CALL)
  if(M68000.CallPrefetch) 
  {
    ASSERT(M68000.PrefetchedOpcode==prefetch_buf[0]);
    prefetch_buf[0]=M68000.PrefetchedOpcode;
    M68000.CallPrefetch=FALSE;
  }
#endif
  lpfetch+=MEM_DIR;

}
#define PREFETCH_SET_PC M68000.PrefetchSetPC();

/*
void TM68000::SetPC(MEM_ADDRESS ad) {
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
#if defined(SS_CPU_CHECK_PC)
    PC=ad;
#endif    
    PrefetchSetPC();//PREFETCH_SET_PC

}
*/
#define SET_PC(ad) M68000.SetPC(ad);




#endif//SSEM68000_H