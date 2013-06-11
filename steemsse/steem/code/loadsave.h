#if defined(STEVEN_SEAGAL)
// rather silly but so we leave the define here

#if SSE_VERSION>=351
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

#ifdef IN_MAIN
extern void AddSnapShotToHistory(char *);
extern bool LoadSnapShot(char *,bool,bool,bool);
extern bool load_TOS(char *);
extern MEM_ADDRESS get_TOS_address(char *);
extern void SaveSnapShot(char*,int,bool);

#ifdef ENABLE_LOGFILE
extern void load_logsections();
#endif

extern bool load_TOS(char *);
extern bool load_cart(char *); // return true on failure
extern void LoadState(GoodConfigStoreFile *);
extern void SaveState(ConfigStoreFile *);
#endif

