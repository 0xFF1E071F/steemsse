#include "SSE.h"

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
#include "SSEScp.h"


#if defined(SSE_DISK_GHOST)
#include "SSEGhostDisk.h"
#endif


#if defined(SSE_SCP) // Future implementation of SCP support in Steem
//TODO

int TScp::InsertDisk(int drive,char* File) {
  //TRACE("file = %s\n",File); // may be temp zip file eg path\ZIP3A52.tmp

//  return FIMAGE_WRONGFORMAT;

//else
  ImageInfo[drive].DiskIn=true;
  ImageInfo[drive].File=File;

//return what?
  return 0;
}

#endif


#endif//#if defined(STEVEN_SEAGAL)
