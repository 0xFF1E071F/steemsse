#pragma once
#ifndef LOADSAVE_DECLA_H
#define LOADSAVE_DECLA_H

#if defined(SSE_VERSION)
// rather silly but so we leave the define here
#if SSE_VERSION>=382
#define SNAPSHOT_VERSION 53
#elif SSE_VERSION>=380
#define SNAPSHOT_VERSION 52
#elif SSE_VERSION>=372
#define SNAPSHOT_VERSION 51
#elif SSE_VERSION>=371
#define SNAPSHOT_VERSION 50
#elif SSE_VERSION>=370
#define SNAPSHOT_VERSION 49
#elif SSE_VERSION>=361
#define SNAPSHOT_VERSION 48
#elif SSE_VERSION>=360
#define SNAPSHOT_VERSION 47 //unused
#elif SSE_VERSION>=354//SSE_VERSION>=353
#define SNAPSHOT_VERSION 46
#elif SSE_VERSION>=352
#define SNAPSHOT_VERSION 45
#elif SSE_VERSION>=351
#define SNAPSHOT_VERSION 44
#elif SSE_VERSION>=350
#define SNAPSHOT_VERSION 43
#elif SSE_VERSION>=340
#define SNAPSHOT_VERSION 42
#elif SSE_VERSION==330
#define SNAPSHOT_VERSION 41
#endif

#else
#define SNAPSHOT_VERSION 40
#endif

#define LS_LOAD 0
#define LS_SAVE 1

extern int LoadSaveAllStuff(NOT_ONEGAME( FILE * ) ONEGAME_ONLY( BYTE* & ),
                              bool,int DEFVAL(-1),bool DEFVAL(true),int* DEFVAL(NULL));
extern void LoadSnapShotUpdateVars(int);

extern int LoadSnapShotChangeTOS(Str,int);
extern int LoadSnapShotChangeCart(Str);
extern int LoadSnapShotChangeDisks(Str[2],Str[2],Str[2]);
extern void LoadSaveChangeNumFloppies(int);
extern void LoadSavePastiActiveChange();

extern MEM_ADDRESS get_TOS_address(char *);

#if defined(SSE_ACSI_RELOAD_TOS)
extern bool load_TOS(char *);
#endif

#endif//#ifndef LOADSAVE_DECLA_H