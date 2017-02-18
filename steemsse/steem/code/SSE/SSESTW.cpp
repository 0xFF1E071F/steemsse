#include "SSE.h"

#if defined(SSE_DISK_STW)
/*  We really go for it here as this simplistic interface knows nothing but
    the side, track and words on the track. MFM encoding/decoding, timing and
    all the rest is for the distinct drive and WD1772 emulators.
*/

#include "../pch.h"

#if !defined(BIG_ENDIAN_PROCESSOR)
#include <acc.decla.h>
#define SWAP_WORD(val) *val=change_endian(*val);
#else
#define SWAP_WORD(val)
#endif

#include <fdc.decla.h>
#include <floppy_drive.decla.h>

#include "SSESTW.h"
#include "SSEDebug.h"
#include "SSEParameters.h"
#include "SSEFloppy.h"
 
#define N_SIDES FloppyDrive[Id].Sides
#define N_TRACKS FloppyDrive[Id].TracksPerSide
#define TRACKBYTES Disk[Id].TrackBytes
#define HEADER_SIZE (4+2+1+1+2) //STW Version N_SIDES N_TRACKS TRACKBYTES
#define TRACK_HEADER_SIZE (3+2) //"TRK" side track
#define IMAGE_SIZE (HEADER_SIZE+N_SIDES*N_TRACKS*\
(TRACK_HEADER_SIZE+TRACKBYTES*sizeof(WORD)))

#define LOGSECTION LOGSECTION_IMAGE_INFO


TImageSTW::TImageSTW() {
  Init();
}


TImageSTW::~TImageSTW() {
  Close();
}


void TImageSTW::Close() {
  if(fCurrentImage)
  {
#if defined(SSE_DEBUG) && defined(SSE_DISK_STW2)
    TRACE_LOG("STW %s image\n",FloppyDrive[Id].WrittenTo?"save and close":"close");
#endif
    fseek(fCurrentImage,0,SEEK_SET); // rewind
    if(ImageData && FloppyDrive[Id].WrittenTo)
      fwrite(ImageData,1,IMAGE_SIZE,fCurrentImage);
    fclose(fCurrentImage);
    free(ImageData);
  }
  Init(); // zeroes variables
}


#if defined(SSE_GUI_DM_STW)

bool TImageSTW::Create(char *path) {
  // utility called by Disk manager
  ASSERT(Id==0);
  bool ok=false;
  Close();
  fCurrentImage=fopen(path,"wb+"); // create new image
  if(fCurrentImage)
  {
    // write header
    fwrite(DISK_EXT_STW,1,4,fCurrentImage); // "STW"
    SWAP_WORD(&Version);
    fwrite(&Version,sizeof(WORD),1,fCurrentImage);
    SWAP_WORD(&Version);
    fwrite(&N_SIDES,sizeof(BYTE),1,fCurrentImage);
    fwrite(&N_TRACKS,sizeof(BYTE),1,fCurrentImage);
    SWAP_WORD(&TRACKBYTES);
    fwrite(&TRACKBYTES,sizeof(WORD),1,fCurrentImage);
    SWAP_WORD(&TRACKBYTES);

    // init all tracks with random bytes (unformatted disk) 
    for(BYTE track=0;track<N_TRACKS;track++)
    {
      for(BYTE side=0;side<N_SIDES;side++)
      {
        // this can be seen as "metaformat"
        fwrite("TRK",1,3,fCurrentImage);
        fwrite(&side,1,1,fCurrentImage);
        fwrite(&track,1,1,fCurrentImage);

        WORD data; // MFM encoding = clock byte and data byte mixed
        for(int byte=0;byte<TRACKBYTES;byte++)
        {
          data=(WORD)rand();
          fwrite(&data,sizeof(data),1,fCurrentImage); 
        }
      }
    }
    ok=true;
    Close(); 
  }
  TRACE_LOG("STW create %s %s\n",path,ok?"OK":"failed");  
  return ok;
}

#endif


WORD TImageSTW::GetMfmData(WORD position) {
  WORD mfm_data=0;
  if(TrackData && position<TRACKBYTES)
  {
    mfm_data=TrackData[position];
    SWAP_WORD(&mfm_data);
  }
#ifdef SSE_DEBUG
  else 
  {
    TRACE_LOG("GetMfmData(%c:%d) error\n",'A'+DRIVE,position);
    TRACE_OSD("STW ERR");
  }
#endif
  return mfm_data;
}


void TImageSTW::Init() {
  Version=0x0100; // 1.0
  fCurrentImage=NULL;
  ImageData=NULL;
  TrackData=NULL;
  N_SIDES=2;
  N_TRACKS=84;
  TRACKBYTES=DRIVE_BYTES_ROTATION_STW;
}


bool  TImageSTW::LoadTrack(BYTE side,BYTE track) {
  ASSERT(Id==0||Id==1);
  bool ok=false;
  if(side<N_SIDES && track<N_TRACKS && ImageData)  
  {
    int position=HEADER_SIZE
      +track*N_SIDES*(TRACK_HEADER_SIZE+TRACKBYTES*sizeof(WORD))
      +side*(TRACK_HEADER_SIZE+TRACKBYTES*sizeof(WORD));
    //runtime format check
    if( !strncmp("TRK",(char*)ImageData+position,3) 
      && *(ImageData+position+3)==side && *(ImageData+position+4)==track)
    {
#ifdef SSE_DEBUG
      if(TrackData!=(WORD*)(ImageData+position+TRACK_HEADER_SIZE)) //only once
        TRACE_LOG("STW LoadTrack %c: side %d track %d\n",'A'+DRIVE,side,track);  
#endif
      Disk[Id].current_side=side;
      Disk[Id].current_track=track;
      TrackData=(WORD*)(ImageData+position+TRACK_HEADER_SIZE);
      ok=true;
    }
  }
#ifdef SSE_DEBUG
  else
    TRACE_LOG("STW can't load side %d track %d from %p\n",side,track,ImageData);
#endif
  return ok;
}


bool TImageSTW::Open(char *path) {
  bool ok=false;
  Close(); // make sure previous image is correctly closed
  fCurrentImage=fopen(path,"rb+"); // try to open existing file
#if defined(SSE_DISK_STW_READONLY) //3.7.1 last minute
  if(!fCurrentImage) // maybe it's read-only
    fCurrentImage=fopen(path,"rb");
#endif
  if(fCurrentImage) // image exists
  {
    ImageData=(BYTE*)malloc(IMAGE_SIZE); //max size
    if(ImageData)
    {
      fread(ImageData,1,IMAGE_SIZE,fCurrentImage); 
      if(!strncmp(DISK_EXT_STW,(char*)ImageData,3)) // it's STW
      {
        Version=* (WORD*)(ImageData+4);
        SWAP_WORD(&Version);
        if(Version>=0x100 && Version <0x200)
          ok=true;

        N_SIDES=*(BYTE*)(ImageData+6);
        N_TRACKS=*(BYTE*)(ImageData+7);
        TRACKBYTES=*(WORD*)(ImageData+8);
        SWAP_WORD(&TRACKBYTES);
        if(N_SIDES>2 || N_TRACKS>88 || TRACKBYTES>6800)
          ok=false;
#ifdef SSE_DEBUG
        // check meta-format
        else for(BYTE track=0;track<N_TRACKS;track++)
        {
          for(BYTE side=0;side<N_SIDES;side++)
          {
            int position=HEADER_SIZE
              +track*N_SIDES*(TRACK_HEADER_SIZE+TRACKBYTES*sizeof(WORD))
              +side*(TRACK_HEADER_SIZE+TRACKBYTES*sizeof(WORD));
            if(strncmp("TRK",(char*)ImageData+position,3)
              ||  *(ImageData+position+3)!=side 
              ||  *(ImageData+position+3)!=side )
              ok=false;
          }//nxt side
        }//nxt track
        ASSERT(ok);
        TRACE_LOG("Open STW %s, V%X S%d T%d B%d OK%d\n",path,Version,N_SIDES,N_TRACKS,TRACKBYTES,ok); 
#endif
      }
    }
  }//if(fCurrentImage)
  if(!ok)
    Close();
  return ok;
}


void TImageSTW::SetMfmData(WORD position,WORD mfm_data) {
  if(TrackData && position<TRACKBYTES)
  {
    TrackData[position]=mfm_data;
    SWAP_WORD(&TrackData[position]);
#ifdef SSE_DISK_STW2
    if(!FloppyDrive[Id].ReadOnly)
      FloppyDrive[Id].WrittenTo=true;
#endif
  }
}

#endif//defined(SSE_DISK_STW)
