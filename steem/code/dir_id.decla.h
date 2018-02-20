#pragma once
#ifndef DIRID_DECLA_H
#define DIRID_DECLA_H


//---------------------------------------------------------------------------
bool IsDirIDPressed(int,int,bool,bool=0);
void init_DirID_to_text();
EasyStr DirID_to_text(int,bool);
#ifdef WIN32
void RegisterButtonPicker(),UnregisterButtonPicker();
LRESULT __stdcall ButtonPickerWndProc(HWND,UINT,WPARAM,LPARAM);
#endif

#define DIRID_SHORTCUT 0
#define DIRID_JOY_KEY  1
#define DIRID_JOY_1    2
#define DIRID_JOY_2    3

int ConvertDirID(int,int);
//---------------------------------------------------------------------------
extern char *KeyboardButtonName[256];

#ifdef WIN32
#define BLANK_DIRID(i) (i==0 || HIBYTE(i)==0xff)
#define NOT_BLANK_DIRID(i) (i!=0 && HIBYTE(i)!=0xff)
#else
#define BLANK_DIRID(i) (HIBYTE(i)==0xff)
#define NOT_BLANK_DIRID(i) (HIBYTE(i)!=0xff)
#endif


#endif//DIRID_DECLA_H