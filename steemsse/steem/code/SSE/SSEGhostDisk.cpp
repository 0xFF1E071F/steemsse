#include "SSE.h"

#if defined(STEVEN_SEAGAL) && defined(SSE_DISK_GHOST)

#include "../pch.h"

#include "SSEGhostDisk.h"
#include "SSEDebug.h"

#define LOGSECTION LOGSECTION_IMAGE_INFO

#define HEADER_SIZE (4+2+2)       // "STG"VVNN
#define NRECORDS_POSITION (4+2)
#define RECORD_HEADER_SIZE 5 // OK for future (?) TRK## 
#define SECTOR_HEADER "SEC"  // SEC## where ## is record number


TGhostDisk::TGhostDisk() {
  Init();
}


TGhostDisk::~TGhostDisk() {
  Close();
}


void TGhostDisk::Close() {
  if(fCurrentImage)
  {
    TRACE_LOG("STG close image with %d records\n",nRecords);
    fseek(fCurrentImage,NRECORDS_POSITION,SEEK_SET);
    fwrite(&nRecords,sizeof(WORD),1,fCurrentImage); 
    fclose(fCurrentImage);
    free(SectorData);
    Init();
  }
}


bool TGhostDisk::FindIDField(TWD1772IDField *IDField) {
  bool found=false;
  if(fCurrentImage)
  {
    // set on 1st header, skipping header
    fseek(fCurrentImage,HEADER_SIZE,SEEK_SET);

    // this loop is inefficient, we expect few records
    for(int record=0;record<nRecords&&!found;record++)
    {
      // to test header, we recycle IDField (optimisation)
      fread(&CurrentIDField,1,RECORD_HEADER_SIZE,fCurrentImage);
      ASSERT( !strncmp((char*)&CurrentIDField,SECTOR_HEADER,3) );

      // load current ID field
      fread(&CurrentIDField,sizeof(TWD1772IDField),1,fCurrentImage);

      // compare with assumed ID field
      if(CurrentIDField.side==IDField->side 
        && CurrentIDField.track==IDField->track 
        && CurrentIDField.num==IDField->num)
      {
        found=true;
      }
      else // skip data, go to next ID field (#bytes depends on len)
      {
        WORD nbytes_to_skip=CurrentIDField.nBytes();
        ASSERT(nbytes_to_skip==512); // up to now
        fseek(fCurrentImage,nbytes_to_skip,SEEK_CUR);
      }
    }
  }
  return found;
}


void TGhostDisk::Init() {
  Version=0x0100; // 1.0
  nRecords=0;
  fCurrentImage=NULL;
  SectorData=NULL;
  SectorBytes=1024;//max
}


bool TGhostDisk::Open(char *path) {
  bool ok=false;
  Close(); // make sure previous image is correctly closed
  const char header_stg[4]="STG";
  fCurrentImage=fopen(path,"rb+"); // try to open existing file
  if(fCurrentImage) // image exists
  {
    char buffer[4];
    fread(buffer,1,4,fCurrentImage);
    if(!strncmp(header_stg,buffer,3)) // it's STG
    {
      fread(&Version,sizeof(WORD),1,fCurrentImage);
      if(Version>=0x100 && Version <0x200)
        ok=true;
      fread(&nRecords,sizeof(WORD),1,fCurrentImage);
      TRACE_LOG("STG open existing %s, v%04x, %d records\n",path,Version,nRecords);
    }
    else
    {
      TRACE_LOG("File %s isn't a STG file\n",path);
      Close();
    }
  }
  else // image doesn't exist
  {
    Init();
    TRACE_LOG("STG create %s\n",path);
    fCurrentImage=fopen(path,"wb+"); // create new image
    fwrite(header_stg,1,4,fCurrentImage); // "STG"
    fwrite(&Version,sizeof(WORD),1,fCurrentImage);
    fwrite(&nRecords,sizeof(WORD),1,fCurrentImage);
    ok=true;
  }
  if(ok)
    SectorData=(BYTE*)malloc(SectorBytes); 
  return ok;
}


WORD TGhostDisk::ReadSector(TWD1772IDField *IDField) {
  WORD nbytes=0; 
  if(fCurrentImage && SectorData && FindIDField(IDField))
  {
    nbytes=CurrentIDField.nBytes();
    fread(SectorData,1,nbytes,fCurrentImage);
    TRACE_LOG("STG read %d-%d-%d (%d)\n",IDField->side,IDField->track,IDField->num,nbytes);
  } 
  return nbytes;
}


void TGhostDisk::WriteSector(TWD1772IDField *IDField) {

  if(fCurrentImage && SectorData)
  {
    bool IDField_existed=false;
    // the following relies on knowing file pointer after FindIDField()
    if(FindIDField(IDField))
    {
      IDField_existed=true;
      fseek(fCurrentImage,-sizeof(TWD1772IDField),SEEK_CUR);
    }
    else
    {
      nRecords++;
      // write record header, record # is in big-endian (easier to read)
      char buf[6];
      sprintf(buf,"%s%c%c",SECTOR_HEADER,HIBYTE(nRecords),LOBYTE(nRecords));
      fwrite(buf,RECORD_HEADER_SIZE,1,fCurrentImage);    
    }
    // (re)write IDField
    fwrite(IDField,sizeof(TWD1772IDField),1,fCurrentImage);  
    // write data
    WORD bytes_to_write=IDField->nBytes();
    fwrite(SectorData,sizeof(BYTE),bytes_to_write,fCurrentImage); 
    TRACE_LOG("STG %s %d-%d-%d (%d)\n", (IDField_existed?"update":"write"),
      IDField->side,IDField->track,IDField->num,bytes_to_write);
  }

}

#endif//#if defined(STEVEN_SEAGAL)

