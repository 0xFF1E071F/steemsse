#include "SSE.h"

/*  SSEFloppy just instantiates as global variables the floppy drive-related
    objects we define in other files, such as Dma, WD1772, SF314, etc.
*/

#if defined(STEVEN_SEAGAL)

#if defined(SSE_STRUCTURE_SSEFLOPPY_OBJ)
#include "../pch.h"
#include <cpu.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include <run.decla.h>
#include "SSECpu.h"
#include "SSEInterrupt.h"
#include "SSEShifter.h"
#if defined(WIN32)
#include <pasti/pasti.h>
#endif
EasyStr GetEXEDir();//#include <mymisc.h>//missing...

#if defined(SSE_DRIVE_IPF1)
#include <stemdialogs.h>//temp...
#include <diskman.decla.h>
#endif

#if !defined(SSE_CPU)
#include <mfp.decla.h>
#endif

#endif//#if defined(SSE_STRUCTURE_SSEFLOPPY_OBJ)

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"


#if defined(SSE_DISK_GHOST)
  // Each drive has its own optional ghost image
  // Most will use A: but e.g. Lethal Xcess could save on B:
  TGhostDisk GhostDisk[2];
#endif

#if defined(SSE_DISK_IMAGETYPE)
#endif

#if defined(SSE_DISK_STW)
TImageSTW ImageSTW[2];
#endif

#if defined(SSE_DISK_SCP)
TImageSCP ImageSCP[2];
#endif

#if defined(SSE_DISK_HFE)
TImageHFE ImageHFE[2];
#endif


#if defined(SSE_DMA)
TDma Dma;
#endif

#if defined(SSE_DRIVE)
TSF314 SF314[2]; // cool!
#endif

#if defined(SSE_DISK1)
TDisk Disk[2]; // 
#endif

#if defined(SSE_FDC)
TWD1772 WD1772;
#endif

#if defined(SSE_YM2149)
TYM2149 YM2149;
#endif

#if defined(SSE_IPF) 
TCaps Caps;
#endif



#endif//#if defined(STEVEN_SEAGAL)
