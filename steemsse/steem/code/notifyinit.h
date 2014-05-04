#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_NOTIFYINIT_H)

#include "notifyinit.decla.h"

#else//!SSE_STRUCTURE_NOTIFYINIT_H

void CreateNotifyInitWin();
void DestroyNotifyInitWin();

void SetNotifyInitText(char*);

#ifdef WIN32
LRESULT __stdcall NotifyInitWndProc(HWND,UINT,WPARAM,LPARAM);
HWND NotifyWin=NULL;
ONEGAME_ONLY( HWND NotifyWinParent; )
#endif

#endif//SSE_STRUCTURE_NOTIFYINIT_H