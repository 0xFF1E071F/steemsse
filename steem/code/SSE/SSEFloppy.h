#pragma once
#ifndef SSEFLOPPY_H
#define SSEFLOPPY_H

/*
"The Atari ST uses a WD1772 FDC controller to interface to the Floppy Drive.
 In the Atari system architecture the FDC data and address busses are 
connected to a private bus of the Atari Custom DMA controller and therefore
 the DMA sits between the processor bus and the FDC/HDC.
 This enables the DMA controller to perform automatic DMA transfer between
 the FDC, or an external Hard Disk Controller, and the memory. 
But this also implies that all accesses to the FDC/HDC registers have to be
 done through the DMA controller. The floppy drives interface is also
 connected to three bits of the output Port A of the PSG (YM2149). 
These outputs allow controlling the selection of the drives and the side. 
The interrupt request of the FDC is connected to an input of the MFP (68901)
 general purpose I/O port (GPIO). This allows checking when an FDC/HDC 
command is terminated by either polling this input or by triggering an 
interrupt."
*/

#include <easystr.h>
#include <conditions.h>
#include "SSEParameters.h"

#if defined(SSE_DRIVE_SOUND) && defined(SSE_STRUCTURE_SSEDEBUG_OBJ)
#ifdef WIN32
#include <dsound.h>
#endif
#endif

#include "SSECapsImg.h"
#include "SSEDma.h"
#include "SSEDrive.h"
#if defined(SSE_DISK_GHOST) //3.7.0
#include "SSEGhostDisk.h"
#endif
#include "SSEYM2149.h"
#if defined(SSE_DISK_SCP)
#include "SSEScp.h" //3.7.1
#endif
#if defined(SSE_DISK_STW) //3.7.0
#include "SSESTW.h"
#endif
#if defined(SSE_DISK_HFE) //3.7.2
#include "SSEHfe.h"
#endif
#include "SSEWD1772.h"

#if defined(SSE_FLOPPY)

#ifdef SSE_DRIVE
#define ADAT (SF314[floppy_current_drive()].Adat())
#else
#define ADAT (!floppy_instant_sector_access)
#endif
#endif

#if defined(SSE_DISK_GHOST)
//struct TGhostDisk;//ouch! this works in VC, not BCC -> create apart files
extern TGhostDisk GhostDisk[2];
#endif

#if defined(SSE_DISK_STW)
extern TImageSTW ImageSTW[2];
#define IMAGE_STW (SF314[DRIVE].ImageType.Extension==EXT_STW)
#endif

#if defined(SSE_DISK_SCP)
extern TImageSCP ImageSCP[2];
#define IMAGE_SCP (SF314[DRIVE].ImageType.Extension==EXT_SCP)
#else
#define IMAGE_SCP 0
#endif

#if defined(SSE_DISK_HFE)
extern TImageHFE ImageHFE[2];
#define IMAGE_HFE (SF314[DRIVE].ImageType.Extension==EXT_HFE)
#else
#define IMAGE_HFE 0
#endif



#if defined(SSE_DMA)
extern TDma Dma;
#define dma_sector_count Dma.Counter
#define dma_address Dma.BaseAddress
#define dma_bytes_written_for_sector_count Dma.ByteCount
#define dma_mode Dma.MCR
#define dma_status Dma.SR
#endif//dma

#if defined(SSE_DRIVE)
extern TSF314 SF314[2]; // 2 double-sided drives, wow!
#endif

#if defined(SSE_DISK1)
extern TDisk Disk[2]; // 
#endif

#if defined(SSE_FDC)
extern TWD1772 WD1772;
#define fdc_cr WD1772.CR     // problem:
#define fdc_str WD1772.STR   // not identified in VC6 debugger
#define fdc_tr WD1772.TR
#define fdc_sr WD1772.SR
#define fdc_dr WD1772.DR
#endif //FDC

#if defined(SSE_DISK_STW)
#define fdc_last_step_inwards_flag WD1772.Lines.direction
#endif

#if defined(SSE_WD1772_REG2_B)
#define floppy_type1_command_active WD1772.StatusType
#endif

#if defined(SSE_YM2149)
extern TYM2149 YM2149;
#endif

#if defined(SSE_IPF)
extern TCaps Caps;
#endif

#endif//SSEFLOPPY_H
