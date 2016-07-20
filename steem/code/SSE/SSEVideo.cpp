#include "SSE.h"

#include "../pch.h"
#include <conditions.h>
#include "SSEParameters.h"
#include "SSEVideo.h"


#if defined(SSE_VIDEO)

#if defined(SSE_VID_BORDERS)
BYTE SideBorderSize=ORIGINAL_BORDER_SIDE; // 32
BYTE SideBorderSizeWin=ORIGINAL_BORDER_SIDE;
BYTE BottomBorderSize=ORIGINAL_BORDER_BOTTOM; // 40
#endif

#endif//#if defined(SSE_VIDEO)