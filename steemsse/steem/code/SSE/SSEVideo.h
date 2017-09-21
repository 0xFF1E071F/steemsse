#pragma once
#ifndef SSEVIDEO_H
#define SSEVIDEO_H

#if defined(SSE_VIDEO_CHIPSET)

#include <conditions.h>

#include "SSEGlue.h"
#include "SSEMMU.h"
#include "SSEShifter.h"

/*  The overscan mask is an imitation of Hatari's BorderMask.
    Each trick has a dedicated bit, to set it we '|' it, to check it
    we '&' it. Each value is the previous one x2.
    Note max 32 bit = $80 000 000
*/
enum EOverscanMask {
 TRICK_LINE_PLUS_26=0x01, 
 TRICK_LINE_PLUS_2=0x02, 
 TRICK_LINE_MINUS_106=0x04,
 TRICK_LINE_MINUS_2=0x08,
 TRICK_LINE_PLUS_44=0x10,
 TRICK_4BIT_SCROLL=0x20,
 TRICK_OVERSCAN_MED_RES=0x40,
 TRICK_BLACK_LINE=0x80,
 TRICK_TOP_OVERSCAN=0x100,
 TRICK_BOTTOM_OVERSCAN=0x200,
 TRICK_BOTTOM_OVERSCAN_60HZ=0x400,
 TRICK_LINE_PLUS_20=0x800,
 TRICK_0BYTE_LINE=0x1000,
 TRICK_STABILISER=0x2000,
#if defined(SSE_SHIFTER_HIRES_COLOUR_DISPLAY)
 TRICK_80BYTE_LINE=0x4000, // don't assume a "no trick" colour line = 160byte
#endif
 TRICK_UNSTABLE=0x08000,
 TRICK_LINE_PLUS_24=0x10000,
 TRICK_LINE_PLUS_4=0x20000,
 TRICK_LINE_PLUS_6=0x40000,
 TRICK_8BIT_SHIFT=0x80000,
 TRICK_NEO=0x100000//tests
};

enum EBorders {BORDERS_NONE, BORDERS_ON, BORDERS_AUTO_OFF, BORDERS_AUTO_ON};

extern TGlue Glue;
extern TMMU MMU;
extern TShifter Shifter;

#endif//defined(SSE_VIDEO_CHIPSET)

#endif//SSEVIDEO_H