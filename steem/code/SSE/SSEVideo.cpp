#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SS_STRUCTURE_SSESTF_OBJ)
#include "../pch.h"
#include <conditions.h>
#include "SSEParameters.h"
#include "SSEVideo.h"
#endif

#if defined(SS_VIDEO)

#if defined(SS_STRUCTURE_INFO) && !defined(SS_STRUCTURE_SSESTF_VIDEO)
#pragma message("Included for compilation: SSEVideo.cpp")
#endif

#if defined(SS_VID_BORDERS)
BYTE SideBorderSize=ORIGINAL_BORDER_SIDE; // 32
BYTE SideBorderSizeWin=ORIGINAL_BORDER_SIDE;
BYTE BottomBorderSize=ORIGINAL_BORDER_BOTTOM; // 40
#endif

#endif//#if defined(SS_VIDEO)
#endif//#if defined(STEVEN_SEAGAL)