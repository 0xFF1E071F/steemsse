#pragma once
#ifndef DATALOADSAVE_DECLA_H
#define DATALOADSAVE_DECLA_H

#define SEC(n) if (SecDisabled[int(n)]==0)

#ifndef ONEGAME

#define UPDATE \
    if (Handle) Hide();  \
    LoadPosition(pCSF); \
    if (pCSF->GetInt(Section,"Visible",0)) Show();


#else

#define UPDATE

#endif

void LoadAllDialogData(bool,Str,bool* = NULL,GoodConfigStoreFile* = NULL);
void SaveAllDialogData(bool,Str,ConfigStoreFile* = NULL);

#define PSEC_SNAP 0
#define PSEC_PASTE 1
#define PSEC_CUT 2
#define PSEC_PATCH 3
#define PSEC_MACHINETOS 4
#define PSEC_MACRO 5
#define PSEC_PORTS 6
#define PSEC_GENERAL 7
#define PSEC_SOUND 8
#define PSEC_DISPFULL 9
#define PSEC_STARTUP 10
#define PSEC_AUTOUP 11
#define PSEC_JOY 12
#define PSEC_HARDDRIVES 13
#define PSEC_DISKEMU 14
#define PSEC_POSSIZE 15
#define PSEC_DISKGUI 16
#define PSEC_PCJOY 17
#define PSEC_OSD 18

#ifdef SSE_BUGFIX_393D
#define PSEC_NSECT 19
#endif

#pragma pack(push, STRUCTURE_ALIGNMENT)//391

typedef struct{
  char *Name;
  int ID;
}ProfileSectionData;

#pragma pack(pop, STRUCTURE_ALIGNMENT)

extern ProfileSectionData ProfileSection[20];
Str ProfileSectionGetStrFromID(int ID);

#endif//DATALOADSAVE_DECLA_H