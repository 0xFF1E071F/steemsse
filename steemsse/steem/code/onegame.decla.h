#pragma once
#ifndef ONEGAME_DECLA_H
#define ONEGAME_DECLA_H
// new structure not tested
#define EXT extern
#define INIT(s)

EXT void OGVBL(),OGIntercept(),OGWriteSector(int,int,int,int),OGHandleQuit(),OGSetRestart();

#define OG_TEXT_ADDRESS 0xffa000
#define OG_TEXT_LEN 0x1000
EXT char OG_TextMem[OG_TEXT_LEN];
EXT int OGExtraSamplesPerVBL INIT(300);

//#ifdef IN_MAIN
bool OGInit();
void OGRestoreSectors();
void OGLoadData(ConfigStoreFile*);
void OGCleanUp();

EXT Str OGDiskPath;
EXT bool OGInfinite,OGInvincible,OGUnlimitedTime,OGUnlimitedAmmo,OGEasyMode;
EXT int OGNumLives;
EXT WORD *pOGTitle,*pOGExtraScreen[2],*pOGSprites;
EXT WORD OGStorePal[16];

#define OG_QUIT 1
#define OG_RESTART 2
EXT int OGStopAction INIT(0);

//#endif

#undef EXT
#undef INIT

#endif//ONEGAME_DECLA_H