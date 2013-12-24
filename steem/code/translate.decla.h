#pragma once
#ifndef TRANSLATE_DECLA_H
#define TRANSLATE_DECLA_H


#define EXT extern
#define INIT(s)

#define T(s)       Translation(s)
#define StrT(s)    Translation(s)
#define CStrT(s)   Translation(s).Text
#define StaticT(s) (TranslateString=Translation(s))

EXT EasyStr TranslateString,TranslateFileName;
EXT char *TranslateBuf INIT(NULL),*TranslateUpperBuf INIT(NULL);
EXT int TranslateBufLen INIT(0);

EXT EasyStr Translation(char *s);
extern EasyStr StripAndT(char*);


#undef EXT
#undef INIT

#endif//#ifndef TRANSLATE_DECLA_H
