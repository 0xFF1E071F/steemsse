#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SS_STRUCTURE_SSEFLOPPY_OBJ)
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

#if defined(SS_DRIVE_IPF1)
#include <stemdialogs.h>//temp...
#include <diskman.decla.h>
#endif

#if !defined(SS_CPU)
#include <mfp.decla.h>
#endif

#endif//#if defined(SS_STRUCTURE_SSEFLOPPY_OBJ)

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"


#if defined(SS_DISK_GHOST)
  // Each drive has its own optional ghost image
  // Most will use A: but e.g. Lethal Xcess could save on B:
  TGhostDisk GhostDisk[2];
#endif

#if defined(SS_DISK_IMAGETYPE)
#endif

#if defined(SS_DISK_STW)
  TImageSTW ImageSTW[2];
#endif

#if defined(SS_DMA)
TDma Dma;
#endif

#if defined(SS_DRIVE)
TSF314 SF314[2]; // cool!
#endif

#if defined(SS_FDC)
TWD1772 WD1772;
#endif

#if defined(SS_PSG)
TYM2149 YM2149;
#endif

#if defined(SS_IPF) 
TCaps Caps;
#endif

#if defined(SS_SCP)
TScp Scp;
#endif

#endif//#if defined(STEVEN_SEAGAL)
