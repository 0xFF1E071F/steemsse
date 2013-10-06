#pragma once
#ifndef NOTIFYINIT_DECLA_H
#define NOTIFYINIT_DECLA_H

#define EXT extern
#define INIT(s)

void CreateNotifyInitWin();
void DestroyNotifyInitWin();
void SetNotifyInitText(char*);

#ifdef WIN32
EXT LRESULT __stdcall NotifyInitWndProc(HWND,UINT,WPARAM,LPARAM);
EXT HWND NotifyWin INIT(NULL);
ONEGAME_ONLY( EXT HWND NotifyWinParent; )
#endif

#undef EXT
#undef INIT

#endif//NOTIFYINIT_DECLA_H