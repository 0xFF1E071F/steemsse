#include "SSE.h"

#if defined(STEVEN_SEAGAL) && defined(SSE_DISK_STW)
/*  We really go for it here as this simplistic interface knows nothing but
    the side, track and words on the track. Eventual MFM encoding/decoding,
    timing and all the rest is for the distinct WD1772 emulator.
*/

#include "../pch.h"

#include "SSESTW.h"
#include "SSEDebug.h"
#include "SSEParameters.h"
#include "SSEFloppy.h"
#if !defined(BIG_ENDIAN_PROCESSOR)
#include <acc.decla.h>
#define SWAP_WORD(val) *val=change_endian(*val);
#else
#define SWAP_WORD(val)
#endif

#include <fdc.decla.h> //DRIVE
#if defined(SSE_DISK_STW_READONLY) //3.7.1
#include <floppy_drive.decla.h> //FloppyDrive[]
#endif
 

#define HEADER_SIZE (4+2+1+1+2) //STW Version nSides nTracks nBytes

#define NUM_SIDES nSides//2
#define NUM_TRACKS nTracks//84
#define NUM_BYTES nBytes//6256 //DRIVE_BYTES_ROTATION_STW 

#define TRACK_HEADER_SIZE (3+2) //"TRK" side track
#define IMAGE_SIZE (HEADER_SIZE+NUM_SIDES*NUM_TRACKS*\
(TRACK_HEADER_SIZE+NUM_BYTES*sizeof(WORD)))

#if defined(SSE_VAR_RESIZE_372) // saves space, complicates debugging...
#if !defined(SSE_VAR_RESIZE_382)
#define fCurrentImage FloppyDrive[Id].f
#endif
#define nSides FloppyDrive[Id].Sides
#define nTracks FloppyDrive[Id].TracksPerSide
#if defined(SSE_DISK1)
#define nBytes Disk[Id].TrackBytes
#endif
#endif

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
#if defined(SSE_DEBUG) && SSE_VERSION>=371
    TRACE_LOG("STW %s image\n",FloppyDrive[Id].WrittenTo?"save and close":"close");
#endif
    fseek(fCurrentImage,0,SEEK_SET); // rewind
    ASSERT(ImageData);
    if(ImageData)
#ifdef SSE_DISK_STW2
      if(FloppyDrive[Id].WrittenTo)
#endif
        fwrite(ImageData,1,IMAGE_SIZE,fCurrentImage);
    fclose(fCurrentImage);
    free(ImageData);
  }
  Init();
}


bool TImageSTW::Create(char *path) {
  // utility called by Disk manager
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
    fwrite(&nSides,sizeof(BYTE),1,fCurrentImage);
    fwrite(&nTracks,sizeof(BYTE),1,fCurrentImage);
    SWAP_WORD(&nBytes);
    fwrite(&nBytes,sizeof(WORD),1,fCurrentImage);
    SWAP_WORD(&nBytes);

    // init all tracks with random bytes (unformatted disk) 
    for(BYTE track=0;track<NUM_TRACKS;track++)
    {
      for(BYTE side=0;side<NUM_SIDES;side++)
      {
        // this can be seen as "metaformat"
        fwrite("TRK",1,3,fCurrentImage);
        fwrite(&side,1,1,fCurrentImage);
        fwrite(&track,1,1,fCurrentImage);

        WORD data; // MFM encoding = clock byte and data byte mixed
        for(int byte=0;byte<NUM_BYTES;byte++)
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


WORD TImageSTW::GetMfmData(WORD position) {
  WORD mfm_data=0;
  if(TrackData && position<NUM_BYTES)
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
  nSides=2;
  nTracks=84;
  nBytes=DRIVE_BYTES_ROTATION_STW;//6256;
}


bool  TImageSTW::LoadTrack(BYTE side,BYTE track) {
  bool ok=false;
  if(side<NUM_SIDES && track<NUM_TRACKS && ImageData)  
  {
    int position=HEADER_SIZE
      +track*NUM_SIDES*(TRACK_HEADER_SIZE+NUM_BYTES*sizeof(WORD))
      +side*(TRACK_HEADER_SIZE+NUM_BYTES*sizeof(WORD));
#if defined(SSE_DISK_STW2) //runtime format check
    if( !strncmp("TRK",(char*)ImageData+position,3) 
      && *(ImageData+position+3)==side && *(ImageData+position+4)==track)
    {
#ifdef SSE_DEBUG
      if(TrackData!=(WORD*)(ImageData+position+TRACK_HEADER_SIZE)) //only once
        TRACE_LOG("STW LoadTrack %c: side %d track %d\n",'A'+DRIVE,side,track);  
#endif
#if defined(SSE_DISK2)
      Disk[Id].current_side=side;
      Disk[Id].current_track=track;
#endif      
      TrackData=(WORD*)(ImageData+position+TRACK_HEADER_SIZE);
      ok=true;
    }
#else
    ASSERT( !strncmp("TRK",(char*)ImageData+position,3) );
    ASSERT( *(ImageData+position+3)==side );
    ASSERT( *(ImageData+position+4)==track );

    TrackData=(WORD*)(ImageData+position+TRACK_HEADER_SIZE);
    TRACE_LOG("STW LoadTrack %c: side %d track %d\n",'A'+DRIVE,side,track);  
    ok=true;
#if defined(SSE_DISK2)
      Disk[Id].current_side=side;
      Disk[Id].current_track=track;
#endif
#endif//stw2?
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

        nSides=*(BYTE*)(ImageData+6);
        nTracks=*(BYTE*)(ImageData+7);
        nBytes=*(WORD*)(ImageData+8);
        SWAP_WORD(&nBytes);
        if(nSides>2 || nTracks>88 || nBytes>6800)
          ok=false;
#ifdef SSE_DEBUG
        // check meta-format
        else for(BYTE track=0;track<NUM_TRACKS;track++)
        {
          for(BYTE side=0;side<NUM_SIDES;side++)
          {
            int position=HEADER_SIZE
              +track*NUM_SIDES*(TRACK_HEADER_SIZE+NUM_BYTES*sizeof(WORD))
              +side*(TRACK_HEADER_SIZE+NUM_BYTES*sizeof(WORD));
            if(strncmp("TRK",(char*)ImageData+position,3)
              ||  *(ImageData+position+3)!=side 
              ||  *(ImageData+position+3)!=side )
              ok=false;
          }//nxt side
        }//nxt track
        ASSERT(ok);
        TRACE_LOG("Open STW %s, V%X S%d T%d B%d OK%d\n",path,Version,nSides,nTracks,nBytes,ok); 
#endif
      }
    }
  }//if(fCurrentImage)
  if(!ok)
    Close();
  return ok;
}


void TImageSTW::SetMfmData(WORD position,WORD mfm_data) {
  if(TrackData && position<NUM_BYTES)
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
