#pragma once
#ifndef SSESCP_H
#define SSESCP_H

#include <easystr.h>
#include <conditions.h>
#include "SSEParameters.h"

#if defined(SSE_DRIVE_SOUND) && defined(SSE_STRUCTURE_SSEDEBUG_OBJ)
#ifdef WIN32
#include <dsound.h>
#endif
#endif

#include "SSEDma.h"
#include "SSEDrive.h"
#include "SSEYM2149.h"
#include "SSEWD1772.h"


#if defined(SSE_DISK_SCP)

/*
; TRACK DATA = 16 BIT VALUE, TIME IN NANOSECONDS/25ns FOR ONE BIT CELL TIME
;
; i.e. 0x00DA = 218, 218*25 = 5450ns = 5.450us
*/

#define SCP_DATA_WINDOW_TOLERANCY 20 

struct TSCP_file_header {
  char IFF_ID[3]; //"SCP" (ASCII CHARS)
  BYTE IFF_VER; //version (nibbles major/minor)
  BYTE IFF_DISKTYPE; //disk type (0=CBM, 1=AMIGA, 2=APPLE II, 3=ATARI ST, 4=ATARI 800, 5=MAC 800, 6=360K/720K, 7=1.44MB)
  BYTE IFF_NUMREVS; //number of revolutions (2=default)
  BYTE IFF_START; //start track (0-165)
  BYTE IFF_END; //end track (0-165)
  BYTE IFF_FLAGS; //FLAGS bits (0=INDEX, 1=TPI, 2=RPM, 3=TYPE - see defines below)
  BYTE IFF_ENCODING; //BIT CELL ENCODING (0=16 BITS, >0=NUMBER OF BITS USED)
  BYTE IFF_HEADS; //0=both heads are in image, 1=side 0 only, 2=side 1 only
  BYTE IFF_RSRVED; //reserved space
  DWORD IFF_CHECKSUM; //32 bit checksum of data added together starting at 0x0010 through EOF
  DWORD IFF_THDOFFSET[166]; // track data header offset
};


struct TSCP_TDH_TABLESTART {
  DWORD TDH_DURATION; //duration of track, from index pulse to index pulse (1st revolution)
  DWORD TDH_LENGTH; //length of track (1st revolution)
  DWORD TDH_OFFSET;
};


struct TSCP_track_header {
  char TDH_ID[3]; //"TRK" (ASCII CHARS)
  BYTE TDH_TRACKNUM; //track number
  TSCP_TDH_TABLESTART TDH_TABLESTART[5]; //table of entries (3 longwords per revolution stored)
  DWORD track_data_checksum; //? see hxc project
};


struct  TImageSCP {
  // interface (the same as for STW disk images)
  bool Open(char *path);
  void Close();
#if defined(SSE_DISK_SCP_START_REV1)
  bool LoadTrack(BYTE side,BYTE track,bool reload=false);
#else
  bool LoadTrack(BYTE side,BYTE track);
#endif
  WORD GetMfmData(WORD position); 
  void SetMfmData(WORD position, WORD mfm_data);
#if defined(SSE_WD1772_DPLL)
#ifdef SSE_WD1772_WEAK_BITS2
  int GetNextTransition(BYTE& us_to_next_flux);
#else
  int GetNextTransition();
#endif
#endif
  // other functions
  TImageSCP();
  ~TImageSCP();
  void ComputePosition(WORD position);
  int UnitsToNextFlux(int position);
  int UsToNextFlux(int units_to_next_flux);
#if !defined(SSE_WD1772_PRECISE_SYNC)  || defined(SSE_DISK_SCP_TO_MFM_PREVIEW)
  BYTE GetDelay(int position);
#endif
  void IncPosition();
  void Init();
#if defined(SSE_DISK_SCP_WRITE)
  bool SaveTrack();
#endif
  // variables
  BYTE nSides;
  BYTE nTracks;
  WORD nBytes;
  DWORD nBits;
  TSCP_file_header file_header;
  TSCP_track_header track_header;
  DWORD Position;
  BYTE rev;
private: 
  FILE *fCurrentImage;
  DWORD *TimeFromIndexPulse; // from IP
#if !defined(SSE_WD1772_DPLL) || defined(SSE_DISK_SCP_WRITE)
  BYTE ShiftsToNextOne;
#endif
#if defined(SSE_DISK_SCP_WRITE)
  bool is_dirty;
#endif
#if defined(SSE_WD1772_WEAK_BITS) && !defined(SSE_WD1772_DPLL)
  char weak_bit_shift;
#endif
#if defined(SSE_BOILER) && defined(SSE_DISK_SCP_TO_MFM_PREVIEW)
  void InterpretFlux(); // was a dev step
#endif

};


#endif//scp

#endif//SSESCP_H