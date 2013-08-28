#pragma once
#ifndef KEYTABLE_DECLA_H
#define KEYTABLE_DECLA_H

#define EXT extern
#define INIT(s)

#ifndef STEEMKEYTEST
EXT DWORD AltKeys_French[8];
EXT DWORD AltKeys_German[7];
EXT DWORD AltKeys_Spanish[8];
EXT DWORD AltKeys_Italian[8];
EXT DWORD AltKeys_Swedish[9];
EXT DWORD AltKeys_Swiss[10];
                         
extern LANGID KeyboardLangID;
//---------------------------------------------------------------------------
void GetTOSKeyTableAddresses(MEM_ADDRESS *lpUnshiftTable,MEM_ADDRESS *lpShiftTable);
//---------------------------------------------------------------------------
void GetAvailablePressChars(DynamicArray<DWORD> *lpChars);
#endif//#ifndef STEEMKEYTEST

EXT WORD *shift_key_table[4];

EXT bool EnableShiftSwitching,ShiftSwitchingAvailable;
//---------------------------------------------------------------------------
#ifdef WIN32
void SetSTKeys(char *Letters,int Val1,...);
#elif defined(UNIX)
EXT KeyCode Key_Pause,Key_Quit;
void SetSTKeys(char *Letters,int Val1,...);
void SetSTKey(KeySym Sym,BYTE STCode,bool CanOverwrite=0);
#endif
//---------------------------------------------------------------------------
#define PC_SHIFT 1
#define NO_PC_SHIFT 0
#define PC_ALT 2
#define NO_PC_ALT 0

#define ST_SHIFT 1
#define NO_ST_SHIFT 0
#define ST_ALT 2
#define NO_ST_ALT 0

void AddToShiftSwitchTable(int PCModifiers,int PCAscii,BYTE STModifier,BYTE STCode);
void DestroyKeyTable();
void InitKeyTable();

#ifdef UNIX
void UNIX_get_fake_VKs();
#endif

#undef EXT
#undef INIT

#endif//#ifndef KEYTABLE_DECLA_H