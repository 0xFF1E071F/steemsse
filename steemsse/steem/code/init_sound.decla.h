#pragma once
#ifndef INITSOUND_DECLA_H
#define INITSOUND_DECLA_H

#define EXT extern
#define INIT(s)


EXT HRESULT Sound_Start(),Sound_Stop(bool=0);
EXT HRESULT SoundStartBuffer(int,int);
EXT void SoundGetPosition(DWORD*,DWORD*);
EXT HRESULT SoundLockBuffer(DWORD,DWORD,LPVOID*,DWORD*,LPVOID*,DWORD*);
EXT void SoundUnlock(LPVOID,DWORD,LPVOID,DWORD);
WIN_ONLY( EXT Str SoundLogError(Str,HRESULT); )
EXT HRESULT SoundError(char*,HRESULT);
EXT void sound_record_open_file();
EXT void sound_record_close_file();
EXT DWORD SoundGetTime();
WIN_ONLY( EXT void SoundChangeVolume(); )
EXT bool SoundActive();

EXT bool DSOpen INIT(0);
EXT int UseSound INIT(0);

#ifdef UNIX

#define XS_PA 1
#define XS_RT 2

EXT int x_sound_lib;
#ifndef NO_RTAUDIO
#include <RtAudio.h>
EXT RtAudio *rt_audio;
EXT int rt_unsigned_8bit;
#endif

HRESULT InitSound();
void SoundRelease();



EXT void internal_speaker_sound_by_period(int);

EXT EasyStr sound_device_name; // INIT("/dev/dsp");
EXT int console_device INIT(-1);


#if defined(SS_RTAUDIO_LARGER_BUFFER)
EXT int rt_buffer_size INIT(256*2),rt_buffer_num INIT(4);
#else
EXT int rt_buffer_size INIT(256),rt_buffer_num INIT(4);
#endif

#endif

#if defined(UNIX) || (defined(USE_PORTAUDIO_ON_WIN32) && defined(WIN32))
EXT int pa_output_buffer_size INIT(128);
#endif


EXT bool TrySound;

#undef EXT
#undef INIT

#endif//#ifndef INITSOUND_DECLA_H