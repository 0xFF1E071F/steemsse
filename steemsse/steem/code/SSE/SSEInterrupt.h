#pragma once
#ifndef SSEINTERRUPT_H
#define SSEINTERRUPT_H

#include "SSESTF.h"
#include "SSEGlue.h"

#if defined(SSE_CPU_MFP_RATIO) 
extern double CpuMfpRatio;
extern DWORD CpuNormalHz;
#if defined(SSE_CPU_MFP_RATIO_OPTION)
extern DWORD CpuCustomHz;
#endif
#endif

#if defined(SSE_INTERRUPT)

#if defined(SSE_INT_VBL_STF)
extern int HblTiming;
#endif

#if defined(SSE_INT_HBL) 
extern void HBLInterrupt();
#define HBL_INTERRUPT HBLInterrupt(); // called in mpf.cpp
#endif

#if defined(SSE_INT_VBL) 
extern void VBLInterrupt();
#define VBL_INTERRUPT VBLInterrupt(); // called in mpf.cpp
#endif

#endif//#if defined(SSE_INTERRUPT)
#endif//#ifndef SSEINTERRUPT_H