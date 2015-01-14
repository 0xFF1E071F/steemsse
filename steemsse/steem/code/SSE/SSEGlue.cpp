#include "SSE.h"

#include "../pch.h"
#include <conditions.h>

#include "SSEGlue.h"
#include "SSEMMU.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_GLUE)

TGlue::TGlue() {
  DE_cycles[0]=DE_cycles[1]=320;
  DE_cycles[2]=160;
}



/*  There's already a MMU object where wake-up state is managed.
    We use Glue to compute some "Shifter tricks" thresholds only
    when some options are changed, and not at every check for each
    scanline.
    This should improve performance and make code clearer.
*/

void TGlue::Update() {


}

TGlue Glue;

#endif//#if defined(SSE_SHIFTER)

#endif//#if defined(STEVEN_SEAGAL)