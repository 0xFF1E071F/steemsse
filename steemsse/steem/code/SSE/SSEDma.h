#pragma once
#ifndef SSEDMA_H
#define SSEDMA_H

#include "SSE.h"
#if defined(SSE_DMA_OBJECT)

//   Steem original variables are defined as the new ones.
#define dma_sector_count Dma.Counter
#define dma_address Dma.BaseAddress
#define dma_bytes_written_for_sector_count Dma.ByteCount
#define dma_mode Dma.MCR
#define dma_status Dma.SR

#pragma pack(push, STRUCTURE_ALIGNMENT)

struct TDma {

/*
    ff 8606   R       |-------------xxx|   DMA Status (Word Access)
                                    |||
                                    || ----   _Error Status (1=OK)
                                    | -----   _Sector Count Zero Status
                                     ------   _Data Request Inactive Status

    ff 8606   W       |-------xxxxxxxx-|   DMA Mode Control (Word Access)
                              ||||||||     0  Reserved (0)
                              ||||||| -----1  A0 lines of FDC/HDC
                              |||||| ------2  A1 ................ 
                              ||||| -------3  HDC (1) / FDC (0) Register Select
                              |||| --------4  Sector Count Register Select
                              |||0         5  Reserved (0)
                              || ----------6  Disable (1) / Enable (0) DMA
                              | -----------7  FDC DRQ (1) / HDC DRQ (0) 
                               ------------8  Write (1) / Read (0)
   See notes in SSEDma.cpp, not all bits are used.
*/
  // ENUM
  enum {SR_DRQ=BIT_2,SR_COUNT=BIT_1,SR_NO_ERROR=BIT_0};
  enum {CR_WRITE=BIT_8,CR_DRQ_FDC_OR_HDC=BIT_7,CR_DISABLE=BIT_6,CR_COUNT_OR_REGS=BIT_4,
  CR_HDC_OR_FDC=BIT_3,CR_A1=BIT_2,CR_A0=BIT_1,CR_RESERVED=BIT_5|BIT_0};

  // DATA
  MEM_ADDRESS BaseAddress; // Base Address Register
#if defined(SSE_DMA_DOUBLE_FIFO)
/*
"Internally the DMA has two 16 bytes FIFOs that are used alternatively. 
This feature allows the DMA to continue to receive bytes from the FDC/HDC
 controller while waiting for the processor to read the other FIFO. 
When a FIFO is full a bus request is made to the 68000 and when granted,
 the FIFO is transferred to the memory. This continues until all bytes 
have been transferred."
This feature is emulated in Steem if SSE_DMA_DOUBLE_FIFO is defined, but it
hasn't proved necessary yet.
TODO: maybe use the feature and remove #define to make code more readable?
*/
  BYTE Fifo[2][16
#if defined(SSE_DMA_FIFO_READ_ADDRESS) && !defined(SSE_DMA_FIFO_READ_ADDRESS2)
    +4 // see fdc.h, replaces fdc_read_address_buffer[20]//review this!
#endif
    ];
#else
  BYTE Fifo[16
#if defined(SSE_DMA_FIFO_READ_ADDRESS) && !defined(SSE_DMA_FIFO_READ_ADDRESS2)
    +4 // see fdc.h, replaces fdc_read_address_buffer[20]
#endif
    ];
#endif
#endif//FIFO
  WORD MCR; // mode control register
  WORD Counter; // Counter register
  WORD ByteCount; // 1-512 for sectors
#if defined(SSE_DMA_TRACK_TRANSFER)
  // to check # 16byte parts, it should be debug-only but isn't
  WORD Datachunk; 
#endif
  BYTE SR; // status register
#if defined(SSE_DMA_FIFO)
  unsigned int Request:1;//even so it works, no need for a nested struct
  unsigned int BufferInUse:1;
  unsigned int Fifo_idx:5; //5 bits, must reach 16!
#endif

  // FUNCTIONS
  TDma();
  BYTE IORead(MEM_ADDRESS addr);
  void IOWrite(MEM_ADDRESS addr,BYTE io_src_b);
/*  Because Steem runs CAPS and Pasti plugins, DMA/FDC regs are not
    always up-to-date. This function copies the values from CAPS or
    Pasti so that they are. Useful for debug, OSD.
*/
#if defined(SSE_VS2008_WARNING_383) && !defined(SSE_DEBUG)
  void UpdateRegs();
#else
  void UpdateRegs(bool trace_them=false);
#endif
#if defined(SSE_STRUCTURE_DMA_INC_ADDRESS)
  void IncAddress();
#define DMA_INC_ADDRESS Dma.IncAddress();
#endif
#if defined(SSE_DMA_FIFO)
  void AddToFifo(BYTE data);
#if defined(SSE_DMA_DRQ)
  bool Drq(); // void?
#endif
  BYTE GetFifoByte();
/*  To be able to emulate the DMA precisely if necessary, DMA transfers are
    emulated in two steps. First, they're requested, then they're executed.
    That way we can emulate a little delay before the transfer occurs & the
    toggling of two FIFO buffers at each transfer.
    It proved overkill however, so calling RequestTransfer() will result in
    the immediate call of TransferBytes() (as #defined in SSE.h).
    And only one buffer is used.
*/
  void RequestTransfer();
  void TransferBytes();
};

#pragma pack(pop)

#endif//dma object

#endif//SSEDMA_H
