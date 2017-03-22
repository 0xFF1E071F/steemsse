#include "SSE.h"

#include "../pch.h"
#include <conditions.h>
#include "SSEParameters.h"
#include "SSEVideo.h"

#if defined(SSE_VIDEO_CHIPSET)
// Video chipset made of GLUE, MMU, Shifter
TGlue Glue;

#if defined(SSE_MMU_MONSTER_ALT_RAM)
TMMU MMU={0,0,{0,2,2,1,1,2},{0,2,4,3,1,2},{0,2,0,0,-2,2},{0,2,2,0,0,2},0,0};
#elif defined(SSE_MMU_LOW_LEVEL)
TMMU MMU={0,0,{0,2,2,1,1,2},{0,2,4,3,1,2},{0,2,0,0,-2,2},{0,2,2,0,0,2},0,0};
#elif defined(SSE_MMU_WU)
TMMU MMU={0,{0,2,2,1,1,2},{0,2,4,3,1,2},{0,2,0,0,-2,2},{0,2,2,0,0,2},0,0};
#else
TMMU MMU;
#endif
TShifter Shifter;
#endif

#if defined(SSE_VIDEO)

#if defined(SSE_VID_BORDERS)
BYTE SideBorderSize=ORIGINAL_BORDER_SIDE; // 32
BYTE SideBorderSizeWin=ORIGINAL_BORDER_SIDE;
BYTE BottomBorderSize=ORIGINAL_BORDER_BOTTOM; // 40
#endif

#endif//#if defined(SSE_VIDEO)