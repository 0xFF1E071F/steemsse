#if defined(SS_VIDEO)

#if defined(SS_STRUCTURE)
#pragma message("Included for compilation: SSEVideo.cpp")
#endif

#if defined(SS_VID_BORDERS)
int SideBorderSize=ORIGINAL_BORDER_SIDE; // 32
int SideBorderSizeWin=ORIGINAL_BORDER_SIDE;
int BottomBorderSize=ORIGINAL_BORDER_BOTTOM; // 40
#endif

#if defined(SS_SHIFTER_EVENTS)
#include "SSEShifterEvents.cpp" // debug module, the Steem way...
#endif

#endif//#if defined(SS_VIDEO)