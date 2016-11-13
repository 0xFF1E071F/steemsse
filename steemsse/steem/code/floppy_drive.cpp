/*---------------------------------------------------------------------------
FILE: floppydrive.cpp
MODULE: Steem
DESCRIPTION: These functions define the TFloppyImage class, a core object
that handles all disk image-related functions. The emulator uses SeekSector
to move to the correct place in the disk image file and then reads/writes
data.
---------------------------------------------------------------------------*/


#include "SSE/SSEDebug.h"

#if defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: floppydrive.cpp")
#endif

//TODO refactor, much duplicate code here

#if defined(SSE_STRUCTURE_DECLA)
TFloppyImage FloppyDrive[2]; //SS fascinating names of class and object:
  // it's hard to tell what is 'drive' from what is 'disk'.
  // there's the same problem in the SSE files
bool FloppyArchiveIsReadWrite=0;
#include "SSE/SSEOption.h"
#include "SSE/SSEFloppy.h"
#define LOGSECTION LOGSECTION_IMAGE_INFO//SS
#else
#define LOGSECTION LOGSECTION_FDC//SS //as dummy
#endif


#pragma warning (disable: 4701) //some MSA vars
int TFloppyImage::SetDisk(EasyStr File,EasyStr CompressedDiskName,BPBINFO *pDetectBPB,BPBINFO *pFileBPB)
{

#if defined(SSE_DRIVE_OBJECT)
  int drive=-1;
  if (this==&FloppyDrive[0]) drive=0; // and not according to PSG
  if (this==&FloppyDrive[1]) drive=1; // it's different here!
  TRACE_LOG("%c: SetDisk %s\n",'A'+drive,File.c_str());
#endif

#if defined(SSE_DISK1)
//  Disk[drive].Init(); //seriously bugged
#endif

  // SS: note that the drive may still be spinning when a floppy is removed
  // or inserted -> that state mustn't be changed (Braindamage)

#if defined(SSE_DISK_GHOST)
  // SetDisk() may be called without call to RemoveDisk() first so we
  // must do this here too
  if(SF314[drive].State.ghost) 
    GhostDisk[drive].Close();
  SF314[drive].State.ghost=false;
#endif

#ifndef SSE_VAR_RESET_SAME_DISK //383
  if (IsSameStr_I(File,ImageFile) && IsSameStr_I(CompressedDiskName,DiskInZip)) 
  {//SS
    TRACE_LOG("Same file %s, exit SetDisk\n",File.Text);
//#ifndef SSE_VAR_RESET_SAME_DISK
    return 0;
//#endif
  }//SS
#endif

  if (Exists(File)==0) 
    return FIMAGE_FILEDOESNTEXIST;

  EasyStr OriginalFile=File,NewZipTemp;

  bool FileIsReadOnly=bool(GetFileAttributes(File) & FILE_ATTRIBUTE_READONLY);

  Str Ext;
  bool MSA,STT,DIM,f_PastiDisk=0;
  char *dot=strrchr(File,'.');
  if (dot) Ext=dot+1;

#if defined(SSE_DISK_EXT)
  MSA=IsSameStr_I(Ext,DISK_EXT_MSA);
  STT=IsSameStr_I(Ext,DISK_EXT_STT);
  DIM=IsSameStr_I(Ext,DISK_EXT_DIM);
#else
  MSA=IsSameStr_I(Ext,"MSA");
  STT=IsSameStr_I(Ext,"STT");
  DIM=IsSameStr_I(Ext,"DIM");
#endif

#if defined(SSE_DISK_CAPS)
  bool IPF=IsSameStr_I(Ext,DISK_EXT_IPF);
#ifdef SSE_DISK_CAPS_CTRAW
  bool CTR=IsSameStr_I(Ext,DISK_EXT_CTR);
#endif
#endif
#if defined(SSE_DISK_SCP)
  bool SCP=IsSameStr_I(Ext,DISK_EXT_SCP);
#endif
#if defined(SSE_DISK_STW)
  bool STW=IsSameStr_I(Ext,DISK_EXT_STW);
#endif
#if defined(SSE_TOS_PRG_AUTORUN)
  bool PRG=OPTION_PRG_SUPPORT && IsSameStr_I(Ext,DISK_EXT_PRG);
  bool TOS=OPTION_PRG_SUPPORT && IsSameStr_I(Ext,DISK_EXT_TOS);
#endif
#if defined(SSE_DISK_HFE)
  bool HFE=IsSameStr_I(Ext,DISK_EXT_HFE);
#endif  
#if defined(SSE_DISK_ST)
  bool ST=IsSameStr_I(Ext,DISK_EXT_ST);
#endif

  // NewDiskInZip will be blank for default disk, RealDiskInZip will be the
  // actual name of the file in the zip that is a disk image
  EasyStr NewDiskInZip,RealDiskInZip;

  int Type=ExtensionIsDisk(dot);


  if (Type==DISK_COMPRESSED){
    if (!enable_zip) return FIMAGE_WRONGFORMAT;

    int HOffset=-1;
    bool CorruptZip=true;
    if (zippy.first(File)==0){
      CorruptZip=0;
      do{
        EasyStr fn=zippy.filename_in_zip();
//        TRACE_LOG("File in zip %s\n",fn.c_str());
        Type=FileIsDisk(fn); // SS this changes Type
        if (Type==DISK_UNCOMPRESSED || Type==DISK_PASTI){
          if (CompressedDiskName.Empty() || IsSameStr_I(CompressedDiskName,fn.Text)){
            // Blank DiskInZip name means default disk (first in zip)
#ifdef SSE_DISK_MSA
            MSA=has_extension(fn,DISK_EXT_MSA);
#endif
#ifdef SSE_DISK_ST
            ST=has_extension(fn,DISK_EXT_ST);
#endif
            if (Type==DISK_PASTI){
#if defined(SSE_DISK_PASTI_ONLY_STX)
              ASSERT(drive!=-1);
              if(drive!=-1)
              {
                TRACE_LOG("Disk in %c (%s) is managed by Pasti.dll\n",'A'+drive,fn.Text);

                SF314[drive].ImageType.Manager=MNGR_PASTI;
#ifdef SSE_DISK_ST
                if(ST)
                  SF314[drive].ImageType.Extension=EXT_ST;
#ifdef SSE_DISK_MSA
                else if(MSA)
                  SF314[drive].ImageType.Extension=EXT_MSA;
#endif
                else
#endif
                  SF314[drive].ImageType.Extension=EXT_STX;
              }
#endif
              f_PastiDisk=true;
#if defined(SSE_DISK_PASTI_AUTO_SWITCH)
              pasti_active=true;
              //TRACE_LOG("pasti_active %d\n",pasti_active);
#endif
            }else{
#ifndef SSE_DISK_MSA
              MSA=has_extension(fn,"MSA");
#endif
#ifdef SSE_DISK_EXT
              STT=has_extension(fn,DISK_EXT_STT);
              DIM=has_extension(fn,DISK_EXT_DIM);
#else
              STT=has_extension(fn,"STT");
              DIM=has_extension(fn,"DIM");
#endif
#if defined(SSE_DISK_CAPS)
              IPF=has_extension(fn,DISK_EXT_IPF);
#ifdef SSE_DISK_CAPS_CTRAW
              CTR=has_extension(fn,DISK_EXT_CTR);
#endif
#endif
#if defined(SSE_DISK_SCP)
              SCP=has_extension(fn,DISK_EXT_SCP);
#endif
#if defined(SSE_DISK_STW)
              STW=has_extension(fn,DISK_EXT_STW); // in archive, makes less sense
#endif
#if defined(SSE_DISK_HFE)
              HFE=has_extension(fn,DISK_EXT_HFE);
#endif
#if defined(SSE_TOS_PRG_AUTORUN)
              PRG=OPTION_PRG_SUPPORT && has_extension(fn,DISK_EXT_PRG);
              TOS=OPTION_PRG_SUPPORT && has_extension(fn,DISK_EXT_TOS);
#endif
#if defined(SSE_FLOPPY) && defined(WIN32)
              if(PASTI_JUST_STX&& drive!=-1 && SF314[1-drive].ImageType.Extension!=EXT_STX)
                pasti_active=false;
#endif
            }
            HOffset=zippy.current_file_offset;
            NewDiskInZip=CompressedDiskName;
            RealDiskInZip=fn.Text;
            break;
          }
        }
      }while (zippy.next()==0);
    }
    zippy.close(); // SS archive always opened twice

    if (HOffset!=-1){
      NewZipTemp.SetLength(MAX_PATH);
      GetTempFileName(WriteDir,"ZIP",0,NewZipTemp);
      if (zippy.extract_file(File,HOffset,NewZipTemp,true /*bool hide*/)){
        TRACE_LOG("Failed to extract%s\n",NewZipTemp.c_str());
        DeleteFile(NewZipTemp);
        return FIMAGE_WRONGFORMAT;
      }
      File=NewZipTemp;
      if (FloppyArchiveIsReadWrite){
        FileIsReadOnly=0;
      }else{
        FileIsReadOnly=true;
      }
    }else{
      if (CorruptZip) return FIMAGE_CORRUPTZIP;
      return FIMAGE_NODISKSINZIP;
    }
  }else if (Type==DISK_PASTI
#if defined(SSE_DISK_PASTI_AUTO_SWITCH)
    && (IsSameStr_I(Ext,DISK_EXT_STX)||pasti_active)
#endif
    ){
    f_PastiDisk=true;
#if defined(SSE_DISK_PASTI_AUTO_SWITCH)
    pasti_active=true;
#endif
  }else if (Type==0){
    TRACE_LOG("Disk type 0\n");
    return FIMAGE_WRONGFORMAT;
  }

#if !(defined(SSE_DRIVE_OBJECT))
  int drive=-1;
  if (this==&FloppyDrive[0]) drive=0;
  if (this==&FloppyDrive[1]) drive=1;
#endif
  if (drive==-1) f_PastiDisk=0; // Never use pasti for extra drives

#if defined(SSE_DISK_CAPS)
  if(CAPSIMG_OK && drive!=-1 && SF314[drive].ImageType.Manager==MNGR_CAPS)
    RemoveDisk();
#endif

  if (f_PastiDisk){

#if USE_PASTI
    int Ret=0;
    if (drive>=0 && hPasti){
      RemoveDisk();
#if defined(SSE_DISK_PASTI_ONLY_STX)
//      TRACE_LOG("Activate pasti 2\n");
      pasti_active=true; //3.6.1, because RemoveDisk clears it
      //TRACE_LOG("pasti_active %d\n",pasti_active);
#endif
      
#if defined(SSE_DISK_PASTI_ONLY_STX)
      ASSERT(drive!=-1);
      if(drive!=-1)
      {
        //TRACE_LOG("Disk in %c is STX\n",'A'+drive);

        SF314[drive].ImageType.Manager=MNGR_PASTI;
#ifdef SSE_DISK_ST
        if(ST)
          SF314[drive].ImageType.Extension=EXT_ST;
        else if(MSA)
          SF314[drive].ImageType.Extension=EXT_MSA;
        else
#endif                
          SF314[drive].ImageType.Extension=EXT_STX;
      }
#endif


      FILE *nf=fopen(File,"rb");
      if (nf){
        PastiBufLen=GetFileLength(nf);
        PastiBuf=new BYTE[PastiBufLen+16];
        fread(PastiBuf,1,PastiBufLen,nf);
        fclose(nf);

        Str RealFile=File;
        if (NewZipTemp.NotEmpty()) RealFile=RealDiskInZip;
        struct pastiDISKIMGINFO pdi;
        pdi.imgType=PASTI_ITPROT;
        if (has_extension(RealFile,"ST")) pdi.imgType=PASTI_ITST;
        if (has_extension(RealFile,"MSA")) pdi.imgType=PASTI_ITMSA;
        if (pdi.imgType==PASTI_ITPROT && NewZipTemp.Empty()){
          pdi.mode=PASTI_LDFNAME;
        }else{
          pdi.mode=PASTI_LDMEM;
        }
        pdi.fileName=RealFile;
        pdi.fileBuf=PastiBuf;
        pdi.fileLength=PastiBufLen;
        pdi.bufSize=PastiBufLen+16;
        pdi.bDirty=0;

        BOOL bMediachDelay=TRUE;
        if (pc==rom_addr) bMediachDelay=FALSE;
        if (pasti->ImgLoad(drive,FileIsReadOnly,bMediachDelay,ABSOLUTE_CPU_TIME,&pdi)==FALSE) Ret=FIMAGE_CANTOPEN;
      }else{
        Ret=FIMAGE_CANTOPEN;
      }
    }else{
      Ret=FIMAGE_WRONGFORMAT;
    }
    if (Ret){
      delete[] PastiBuf;
      PastiBuf=NULL;
      if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);
      return Ret;
    }
#endif
#if defined(SSE_DISK_CAPS)
  }else if(CAPSIMG_OK &&
#if defined(SSE_DISK_CAPS_CTRAW)
    (CTR||IPF))
#else
    IPF) 
#endif
  { 

#if defined(SSE_DISK_REMOVE_DISK_ON_SET_DISK)
    RemoveDisk();
#endif

#if defined(SSE_DISK_CAPS_CTRAW)
    SF314[drive].ImageType.Manager=MNGR_CAPS;
    SF314[drive].ImageType.Extension=IPF?EXT_IPF:EXT_CTR;
#endif
    CapsImageInfo img_info;
    int Ret=Caps.InsertDisk(drive,File,&img_info);
    if(Ret==FIMAGE_WRONGFORMAT)
    {
#if defined(SSE_DISK_CAPS_CTRAW) //wrong switch TODO
      ZeroMemory(&SF314[drive].ImageType,sizeof(SF314[drive].ImageType));
#endif
      return Ret;
    }
    // for properties box
    BytesPerSector=SectorsPerTrack=0; 
    Sides=img_info.maxhead+1;
    TracksPerSide=img_info.maxcylinder;
#endif
//SS this horror needs to be refactored of course
#if defined(SSE_DISK_SCP)
  }else if(SCP) { 
    ASSERT(!(this==&FloppyDrive[0] && drive!=0));
    ASSERT(!(this==&FloppyDrive[1] && drive!=1));
#if defined(SSE_DISK_REMOVE_DISK_ON_SET_DISK)
    RemoveDisk();
#endif
    ASSERT(!(this==&FloppyDrive[0] && drive!=0));
    ASSERT(!(this==&FloppyDrive[1] && drive!=1));
    if(drive==-1  || !ImageSCP[drive].Open(File))
      return FIMAGE_WRONGFORMAT;

#if USE_PASTI
    if(SF314[1-drive].ImageType.Extension!=EXT_STX)
      pasti_active=false;
#endif
    SF314[drive].ImageType.Manager=MNGR_WD1772;
    SF314[drive].ImageType.Extension=EXT_SCP;

#if defined(SSE_DISK1)
    Disk[drive].TrackBytes=ImageSCP[drive].nBytes;
#endif
    SF314[drive].State.reading=SF314[drive].State.writing=0;

#endif

#if defined(SSE_DISK_STW)
  }else if(STW) { 

#if defined(SSE_DISK_REMOVE_DISK_ON_SET_DISK)
    RemoveDisk();
#endif

    if(drive==-1  || !ImageSTW[drive].Open(File))
      return FIMAGE_WRONGFORMAT;

#if USE_PASTI
    if(SF314[1-drive].ImageType.Extension!=EXT_STX)
      pasti_active=false;
#endif
    SF314[drive].ImageType.Manager=MNGR_WD1772;
    SF314[drive].ImageType.Extension=EXT_STW;

#if defined(SSE_DISK1)
#if !defined(SSE_VAR_RESIZE_372)
    Disk[drive].TrackBytes=ImageSTW[drive].nBytes;
#if defined(SSE_DISK_STW2) // HxC converter track is too big...
    //hack works but not when we save space (Disk[drive].TrackBytes==ImageSTW[drive].nBytes)
    //the problem is in hxc software, if we don't like that we need to 
    //create our own STW images... see below; this code isn't compiled
    if(OPTION_HACKS && Disk[drive].TrackBytes-DRIVE_BYTES_ROTATION_STW>50
      && ImageSTW[drive].Version==0x100)
      Disk[drive].TrackBytes=DRIVE_BYTES_ROTATION_STW; 
    ASSERT(Disk[drive].TrackBytes==DRIVE_BYTES_ROTATION_STW);
#endif
#endif

#endif
    SF314[drive].State.reading=SF314[drive].State.writing=0;

#if defined(SSE_DISK_HFE_TO_STW) 
    // using Steem to convert, because HxC software counts too many bytes
    // see just above
    // this code is only compiled on demand, it's no Steem feature
    if(drive==1 && SF314[0].ImageType.Extension==EXT_HFE)
    {
      TRACE("Converting HFE to STW...\n");
      for(int si=0;si<2;si++)
        for(int tr=0;tr<ImageHFE[0].file_header.number_of_track;tr++)
        {
          ImageHFE[0].LoadTrack(si,tr);
          ImageSTW[1].LoadTrack(si,tr);
          for(int by=0;by<Disk[1].TrackBytes;by++)
          {
            WORD mfm=ImageHFE[0].GetMfmData(by);
            ImageSTW[1].SetMfmData(by,mfm);
          }
        }
        WrittenTo=true;
        ImageSTW[1].Close();
    }
#endif

#endif//stw


#if defined(SSE_DISK_HFE)
  }else if(HFE) { 

#if defined(SSE_DISK_REMOVE_DISK_ON_SET_DISK)
    RemoveDisk();
#endif

    if(drive==-1  || !ImageHFE[drive].Open(File))
      return FIMAGE_WRONGFORMAT;
#if defined(SSE_DISK1)
    Disk[drive].TrackBytes=DRIVE_BYTES_ROTATION_STW; //default
#endif

#if USE_PASTI
    if(SF314[1-drive].ImageType.Extension!=EXT_STX)
      pasti_active=false;
#endif
    SF314[drive].ImageType.Manager=MNGR_WD1772;
    SF314[drive].ImageType.Extension=EXT_HFE;

    SF314[drive].State.reading=SF314[drive].State.writing=0;
#endif//hfe
  }

#if defined(SSE_TOS_PRG_AUTORUN)
/*  v3.7
    Support for PRG and TOS files
    Not disk images but single PRG or TOS files may be selected.
    In that case we copy to harddisk Z: and boot from it.
    This approach minimises RAM/code use.
*/
  else if(PRG||TOS)
  {
    bool old_DisableHardDrives=HardDiskMan.DisableHardDrives;
    HardDiskMan.DisableHardDrives=false; // or mount path is wrong
    HardDiskMan.update_mount();
    Str PrgPath=mount_path[AUTORUN_HD]; // Z: (2+'Z'-'C')
    //TRACE_LOG("PRG/TOS, path of disk %c: is %s\n",AUTORUN_HD+'A',PrgPath.Text);
    if(!strncmp(PrgPath.Rights(3),"PRG",3)) //our dedicated folder
    {
      RemoveDisk();
      SF314[drive].ImageType.Manager=MNGR_STEEM;
      SF314[drive].ImageType.Extension=(TOS)?EXT_TOS:EXT_PRG;
      Str NewPath,AutoPath;
      AutoPath=PrgPath+SLASH+"AUTO"+SLASH+"AUTORUN.PRG";
      DeleteFile(AutoPath.Text); // anyway
      if(TOS || tos_version<=0x102 || tos_version>=0x200)
      {
        // we'll use AUTO, provided in PRG folder
        NewPath=AutoPath;
      }
      else
      {
        // we'll use DESKTOP.INF, provided in PRG folder
        NewPath=PrgPath+SLASH+"AUTORUN.PRG";
        DeleteFile(NewPath.Text);
      }
      //TRACE_LOG("copy %s to %s\n",File.Text,NewPath.Text);
      CopyFile(File.Text,NewPath.Text,FALSE); // there's no CopyFile in Unix
      
    }//if
    else
      HardDiskMan.DisableHardDrives=old_DisableHardDrives;
  }
#endif  

  else
  {

#if defined(SSE_DISK_IMAGETYPE)
#if defined(SSE_TOS_PRG_AUTORUN)
    if(SF314[drive].ImageType.Extension==EXT_PRG
      || SF314[drive].ImageType.Extension==EXT_TOS)
    {
      HardDiskMan.DisableHardDrives=true; // we guess
    }
#endif
    SF314[drive].ImageType.Manager=MNGR_STEEM;
#if defined(SSE_FDC_383A)
    Disk[drive].TrackBytes=TDisk::TRACK_BYTES; 
#endif
#ifdef SSE_OSD_DRIVE_INFO_EXT //gotta be a smarter way...
    if(MSA)
      SF314[drive].ImageType.Extension=EXT_MSA;
    else if (DIM)
      SF314[drive].ImageType.Extension=EXT_DIM;
    else
      SF314[drive].ImageType.Extension=EXT_ST;
#else
    SF314[drive].ImageType.Extension=MSA?EXT_MSA:EXT_ST; //DIM?
#endif
#endif

    // Open for read for an MSA (going to convert to ST and write to that)
    // and if the file is read-only, otherwise open for update
    FILE *nf=fopen(File,LPSTR((MSA || FileIsReadOnly) ? "rb":"r+b"));
    if (nf==NULL){
      if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);
      return FIMAGE_CANTOPEN;
    }

    if (GetFileLength(nf)<512){
      TRACE_LOG("File length of %s = %d\n",File.c_str(),GetFileLength(nf));
      fclose(nf);
      if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);
      return FIMAGE_WRONGFORMAT;
    }

    fseek(nf,0,SEEK_SET);

    EasyStr NewMSATemp="";
    short MSA_SecsPerTrack,MSA_EndTrack,MSA_Sides;

    if (MSA){
      NewMSATemp.SetLength(MAX_PATH);
      GetTempFileName(WriteDir,"MSA",0,NewMSATemp);

      FILE *tf=fopen(NewMSATemp,"wb");
      if (tf){
        bool Err=0;
        short ID,StartTrack;

        fseek(nf,0,SEEK_SET);

        // Read header
        fread(&ID,2,1,nf);               SWAPBYTES(ID);
        fread(&MSA_SecsPerTrack,2,1,nf); SWAPBYTES(MSA_SecsPerTrack);
        fread(&MSA_Sides,2,1,nf);        SWAPBYTES(MSA_Sides);
        fread(&StartTrack,2,1,nf);       SWAPBYTES(StartTrack);
        fread(&MSA_EndTrack,2,1,nf);     SWAPBYTES(MSA_EndTrack);
/*
Header:
	Word	ID marker, should be $0E0F
	Word	Sectors per track
	Word	Sides (0 or 1; add 1 to this to get correct number of sides)
	Word	Starting track (0-based)
	Word	Ending track (0-based)
*/
        TRACE_LOG("MSA ID %X sides %d tracks %d (%d-%d) sectors %d\n",
          ID,MSA_Sides+1,MSA_EndTrack-StartTrack+1,StartTrack,MSA_EndTrack,MSA_SecsPerTrack);

        if (MSA_SecsPerTrack<1 || MSA_SecsPerTrack>FLOPPY_MAX_SECTOR_NUM ||
            MSA_Sides<0 || MSA_Sides>1 ||
            StartTrack<0 || StartTrack>FLOPPY_MAX_TRACK_NUM || StartTrack>=MSA_EndTrack ||
            MSA_EndTrack<1 || MSA_EndTrack>FLOPPY_MAX_TRACK_NUM){
          Err=true;
        }

        if (Err==0){
          // Read data
          WORD Len,NumRepeats;
          BYTE *TrackData=new BYTE[(MSA_SecsPerTrack*512)+16];
          BYTE *pDat,*pEndDat,dat;
          BYTE *STBuf=new BYTE[(MSA_SecsPerTrack*512)+16];
          BYTE *pSTBuf,*pSTBufEnd=STBuf+(MSA_SecsPerTrack*512)+8;
          for (int n=0;n<=MSA_EndTrack;n++){
            for (int s=0;s<=MSA_Sides;s++){
              if (n>=StartTrack){
                Len=0;
                fread(&Len,1,2,nf); SWAPBYTES(Len);
                if (Len>MSA_SecsPerTrack*512 || Len==0){
                  Err=true;break;
                }
                if (WORD(fread(TrackData,1,Len,nf))<Len){
                  Err=true;break;
                }
                if (Len==(MSA_SecsPerTrack*512)){
                  fwrite(TrackData,Len,1,tf);
                }else{
                  // Convert compressed MSA format track in TrackData to ST format in STBuf
                  pSTBuf=STBuf;
                  pDat=TrackData;
                  pEndDat=TrackData+Len;
                  while (pDat<pEndDat && pSTBuf<pSTBufEnd){
                    dat=*(pDat++);
                    if (dat==0xE5){
                      dat=*(pDat++);
                      NumRepeats=*LPWORD(pDat);pDat+=2;
                      SWAPBYTES(NumRepeats);
                      for (int s=0;s<NumRepeats && pSTBuf<pSTBufEnd;s++) *(pSTBuf++)=dat;
                    }else{
                      *(pSTBuf++)=dat;
                    }
                  }
                  if (pSTBuf>=pSTBufEnd){
                    Err=true;break;
                  }
                  fwrite(STBuf,MSA_SecsPerTrack*512,1,tf);
                }
              }else{
                ZeroMemory(TrackData,MSA_SecsPerTrack*512);
                if (n==0 && s==0){   // Write BPB
                  *LPWORD(TrackData+11)=512;
                  TrackData[13]=2;           // SectorsPerCluster

                  *LPWORD(TrackData+17)=112; // nDirEntries
                  *LPWORD(TrackData+19)=WORD(MSA_EndTrack * MSA_SecsPerTrack);

                  *LPWORD(TrackData+22)=3;   // SectorsPerFAT
                  *LPWORD(TrackData+24)=MSA_SecsPerTrack;
                  *LPWORD(TrackData+26)=MSA_Sides;
                  *LPWORD(TrackData+28)=0;
                }
                fwrite(TrackData,MSA_SecsPerTrack*512,1,tf);
              }
            }
            if (Err) break;
          }
          delete[] TrackData;
          delete[] STBuf;
        }

        fclose(tf);
        fclose(nf);

        if (Err==0){
          SetFileAttributes(NewMSATemp,FILE_ATTRIBUTE_HIDDEN);
          nf=fopen(NewMSATemp,"r+b");
          Err=(nf==NULL);
        }

        if (Err){
          TRACE_LOG("Error opening %s\n",NewMSATemp.c_str());
          DeleteFile(NewMSATemp);
          if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);
          return FIMAGE_WRONGFORMAT;
        }
      }else{
        // Couldn't open NewMSATemp
        fclose(nf);
        if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);
        return FIMAGE_CANTOPEN;
      }
    }

    bool f_ValidBPB=true;
    DWORD f_DiskFileLen=GetFileLength(nf);
    if (STT){
      bool Err=0;
      DWORD Magic;
      WORD Version,Flags,AllTrackFlags,NumTracks,NumSides;

      fread(&Magic,4,1,nf);
      fread(&Version,2,1,nf);
      fread(&Flags,2,1,nf);
      fread(&AllTrackFlags,2,1,nf);
      fread(&NumTracks,2,1,nf);
      fread(&NumSides,2,1,nf);
      Err=(Magic!=MAKECHARCONST('S','T','E','M') || Version!=1 || (AllTrackFlags & BIT_0)==0);
      if (Err==0){
        ZeroMemory(STT_TrackStart,sizeof(STT_TrackStart));
        ZeroMemory(STT_TrackLen,sizeof(STT_TrackLen));
        for (int s=0;s<NumSides;s++){
          for (int t=0;t<NumTracks;t++){
            fread(&STT_TrackStart[s][t],4,1,nf);
            fread(&STT_TrackLen[s][t],2,1,nf);
          }
        }
      }else{
        fclose(nf);

        if (NewMSATemp.NotEmpty()) DeleteFile(NewMSATemp);
        if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);

        return FIMAGE_WRONGFORMAT;
      }

      fseek(nf,0,SEEK_SET);

      RemoveDisk();
      f=nf;
      STT_File=true;
      TracksPerSide=NumTracks;
      Sides=NumSides;

      BytesPerSector=512;
      SectorsPerTrack=0xff; // Variable
    }else{
      BPBINFO bpbi={0,0,0,0};
      int HeaderLen=int(DIM ? 32:0); //SS 0 for .ST
      f_DiskFileLen-=HeaderLen;

      if (DIM){
        int Err=0;

        fseek(nf,0,SEEK_SET);
        WORD Magic;
        fread(&Magic,1,2,nf);
        if (Magic!=0x4242){
          Err=FIMAGE_DIMNOMAGIC;
        }else{
          BYTE UsedSectors;
          fseek(nf,3,SEEK_SET);
          fread(&UsedSectors,1,1,nf);
          if (UsedSectors!=0) Err=FIMAGE_DIMTYPENOTSUPPORTED;
        }

        if (Err){
          fclose(nf);

          if (NewMSATemp.NotEmpty()) DeleteFile(NewMSATemp);
          if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);

          return Err;
        }
      }

      // Always append the name of the real disk file in the zip to the name
      // of the steembpb file, even if we are using default disk
      // This is so we don't need 2 .steembpb files for the default disk
      EasyStr BPBFile=OriginalFile+RealDiskInZip+".steembpb";
      bool HasBPBFile=(GetCSFInt("BPB","Sides",0,BPBFile)!=0);

      if (MSA){
        bpbi.BytesPerSector=512; //SS MSA no choice
      }else{
        fseek(nf,HeaderLen+11,SEEK_SET);
        fread(&bpbi.BytesPerSector,2,1,nf); //SS .ST, .DIM, we have choice?
      }
      fseek(nf,HeaderLen+19,SEEK_SET);
      fread(&bpbi.Sectors,2,1,nf);
      fseek(nf,HeaderLen+24,SEEK_SET);
      fread(&bpbi.SectorsPerTrack,2,1,nf);
      fread(&bpbi.Sides,2,1,nf);
      if (pFileBPB) *pFileBPB=bpbi; // Store BPB exactly as it is in the file (for DiskMan)

      // A BPB is corrupt when one of its fields is totally wrong
      bool BPBCorrupt=0;
      if (bpbi.BytesPerSector!=128 && bpbi.BytesPerSector!=256 &&
            bpbi.BytesPerSector!=512 && bpbi.BytesPerSector!=1024) BPBCorrupt=true;//SS 1024 possible
      if (bpbi.SectorsPerTrack<1 || bpbi.SectorsPerTrack>FLOPPY_MAX_SECTOR_NUM) BPBCorrupt=true;
      if (bpbi.Sides<1 || bpbi.Sides>2) BPBCorrupt=true;
      // Has to be exact length for Steem to accept it
      if (DWORD(bpbi.Sectors*bpbi.BytesPerSector)!=f_DiskFileLen || BPBCorrupt){
        f_ValidBPB=0;
        // If the BPB is only a few sectors out then we don't want to destroy
        // the value in BytesPerSector.
        if (BPBCorrupt) bpbi.BytesPerSector=512; // 99.9% of ST disks used sectors this size
      }

      if (f_ValidBPB==0){
        if (MSA){
          // Probably got a better chance of being right than guessing
          bpbi.SectorsPerTrack=MSA_SecsPerTrack;
          bpbi.Sides=short(MSA_Sides+1);
          bpbi.Sectors=short((MSA_EndTrack+1)*bpbi.SectorsPerTrack*bpbi.Sides);
        }else{
          // BPB's wrong, time to guess the format
          bpbi.SectorsPerTrack=0;

          bpbi.Sectors=short(f_DiskFileLen/bpbi.BytesPerSector);
          bpbi.Sides=WORD((bpbi.Sectors<1100) ? 1:2); // Total guess

          // Work out bpbi.SectorsPerTrack from bpbi.Sides and bpbi.Sectors
          bool Found=0;
          for (;;){
            for (int t=75;t<=FLOPPY_MAX_TRACK_NUM;t++){
              for (int s=8;s<=13;s++){
                if (bpbi.Sectors==(t+1)*s*bpbi.Sides){
                  bpbi.SectorsPerTrack=WORD(s);
                  Found=true;
                  break;
                }
              }
              if (Found) break;
            }
            if (Found) break;

            if (bpbi.Sectors<10) break;

            bpbi.Sectors--;
          }
          if (bpbi.SectorsPerTrack==0 && HasBPBFile==0){
            fclose(nf);

            if (NewMSATemp.NotEmpty()) DeleteFile(NewMSATemp);
            if (NewZipTemp.NotEmpty()) DeleteFile(NewZipTemp);

            return FIMAGE_WRONGFORMAT;
          }
        }
      }
      if (pDetectBPB) *pDetectBPB=bpbi; // Steem's best guess (or the BPB if it is valid)

      if (HasBPBFile){
        // User specified disk parameters
        ConfigStoreFile CSF(BPBFile);
        bpbi.Sides=CSF.GetInt("BPB","Sides",2);
        bpbi.SectorsPerTrack=CSF.GetInt("BPB","SectorsPerTrack",9);
        bpbi.BytesPerSector=CSF.GetInt("BPB","BytesPerSector",512);
        bpbi.Sectors=CSF.GetInt("BPB","Sectors",1440);
        CSF.Close();
      }

      fseek(nf,HeaderLen,SEEK_SET);
#if defined(SSE_OSD_DRIVE_INFO_EXT)
      //this is pretty annoying, this RemoveDisk
      TImageType save_type=SF314[drive].ImageType;
#endif
      RemoveDisk();
#if defined(SSE_OSD_DRIVE_INFO_EXT)
      //this is pretty annoying, this RemoveDisk
      SF314[drive].ImageType=save_type;
#endif
      f=nf;
      BytesPerSector=short(bpbi.BytesPerSector);
      SectorsPerTrack=short(bpbi.SectorsPerTrack);
      Sides=short(bpbi.Sides);

      TracksPerSide=short(short(bpbi.Sectors/SectorsPerTrack)/Sides);

      DIM_File=DIM;
    }
    MSATempFile=NewMSATemp;
    ValidBPB=f_ValidBPB;
    DiskFileLen=f_DiskFileLen;
  }

  ReadOnly=FileIsReadOnly;
  ZipTempFile=NewZipTemp;
  DiskInZip=NewDiskInZip;
  ImageFile=OriginalFile;
  PastiDisk=f_PastiDisk;

#if USE_PASTI && defined(SSE_DISK_PASTI_AUTO_SWITCH2) //3.8.0
/*  We deactivate pasti even without option 'Pasti only for STX' if we're
    running CAPS or WD1772 manager.
*/
  if(pasti_active && !PASTI_JUST_STX && SF314[drive].ImageType.Manager!=MNGR_PASTI)
    pasti_active=false;
#endif

#if defined(SSE_DISK_CAPS)
  IPFDisk=IPF;
#ifdef SSE_DISK_CAPS_CTRAW
  CTRDisk=CTR;
#endif
#endif
#if defined(SSE_DISK_SCP)
  SCPDisk=SCP;
#endif
#if defined(SSE_DISK_STW)
  STWDisk=STW;
#endif
#if defined(SSE_DISK_HFE)
  HFEDisk=HFE;
#endif

  WrittenTo=0;

  //SS note that options haven't been retrieved yet when starting
  //steem and auto.sts is loaded with its disks -> we can't open a
  //ghost image here, we don't know if option is set

  // Media change, write protect for 10 VBLs, unprotect for 10 VBLs, wp for 10
  if (this==&FloppyDrive[0]) floppy_mediach[0]=30;
  if (this==&FloppyDrive[1]) floppy_mediach[1]=30;
  // disable input for pasti
  disable_input_vbl_count=max(disable_input_vbl_count,30);
#if defined(SSE_DISK_IMAGETYPE__)
  ASSERT(SF314[drive].ImageType.Extension);
#endif
  dbg_log("");
  dbg_log(EasyStr("FDC: Inserted disk ")+OriginalFile);
  dbg_log(EasyStr("     Into drive ")+LPSTR(floppy_current_drive() ? "B":"A")+" its BPB was "+LPSTR(ValidBPB ? "valid.":"invalid."));
  dbg_log(EasyStr("     BytesPerSector=")+BytesPerSector+", SectorsPerTrack="+SectorsPerTrack+
      ", Sides="+Sides);
  dbg_log(Str("     TracksPerSide=")+TracksPerSide+", ReadOnly="+ReadOnly);
  dbg_log("");

#if defined(SSE_FLOPPY_ADAT_UPDATE)
  SF314[drive].UpdateAdat();
#endif


  TRACE_LOG("Ext %s Manager %d Sides %d Tracks %d Sectors %d adat %d\n",
//dot_ext(SF314[drive].ImageType.Extension),SF314[drive].ImageType.Manager,Sides,TracksPerSide,SectorsPerTrack);
extension_list[SF314[drive].ImageType.Extension],SF314[drive].ImageType.Manager,Sides,TracksPerSide,SectorsPerTrack,SF314[drive].State.adat);

  return 0;
}
#pragma warning (default: 4701)
//---------------------------------------------------------------------------
bool TFloppyImage::ReinsertDisk()
{
  if (Empty() || PastiDisk
#if defined(SSE_DISK_CAPS)
    || IPFDisk
#ifdef SSE_DISK_CAPS_CTRAW
    || CTRDisk
#endif

#endif
#if defined(SSE_DISK_SCP)
    || SCPDisk //not right, but we're starting...
#endif
#if defined(SSE_DISK_HFE)
    || HFEDisk //?
#endif
    ) return 0;

  fclose(f);
  if (ZipTempFile.NotEmpty()) ReadOnly=(FloppyArchiveIsReadWrite==0);
  if (MSATempFile.NotEmpty()){
    f=fopen(MSATempFile,"r+b");
  }else if (ZipTempFile.NotEmpty()){
    f=fopen(ZipTempFile,LPSTR(ReadOnly ? "rb":"r+b"));
  }else{
    f=fopen(ImageFile,LPSTR(ReadOnly ? "rb":"r+b"));
  }
  if (f==NULL){
    DiskMan.EjectDisk(this==&FloppyDrive[0]?0:1);
    return 0;
  }
  return true;
}
//---------------------------------------------------------------------------
bool TFloppyImage::OpenFormatFile()
{
  if (f==NULL || ReadOnly || Format_f || STT_File || PastiDisk
#if defined(SSE_DISK_CAPS)
    || IPFDisk
#ifdef SSE_DISK_CAPS_CTRAW
    || CTRDisk
#endif
#endif 
#if defined(SSE_DISK_SCP)
    || SCPDisk
#endif   
#if defined(SSE_DISK_STW)
    || STWDisk
#endif
#if defined(SSE_DISK_HFE)
    || HFEDisk
#endif
    ) return 0;

  // The format file is just a max size ST file, any formatted tracks
  // go in here and then are merged with unformatted tracks when
  // the disk is removed from the drive
  FormatTempFile.SetLength(MAX_PATH);
  GetTempFileName(WriteDir,"FMT",0,FormatTempFile);

  // Create it
  Format_f=fopen(FormatTempFile,"wb");
  if (Format_f==NULL) return 0;
  fclose(Format_f);

  SetFileAttributes(FormatTempFile,FILE_ATTRIBUTE_HIDDEN);
  Format_f=fopen(FormatTempFile,"r+b");
  if (Format_f==NULL) return 0;

  char zeros[FLOPPY_MAX_BYTESPERSECTOR];
  ZeroMemory(zeros,sizeof(zeros));
  for (int Side=0;Side<2;Side++){
    for (int Track=0;Track<=FLOPPY_MAX_TRACK_NUM;Track++){
      for (int Sector=1;Sector<=FLOPPY_MAX_SECTOR_NUM;Sector++){
        fwrite(zeros,FLOPPY_MAX_BYTESPERSECTOR,1,Format_f);
      }
    }
  }
  fflush(Format_f);

  return true;
}
//---------------------------------------------------------------------------
bool TFloppyImage::ReopenFormatFile()
{
  if (Format_f==NULL || f==NULL || ReadOnly || PastiDisk
#if defined(SSE_DISK_CAPS)
    || IPFDisk
#ifdef SSE_DISK_CAPS_CTRAW
    || CTRDisk
#endif
#endif    
#if defined(SSE_DISK_SCP)
    || SCPDisk
#endif
#if defined(SSE_DISK_STW)
    || STWDisk
#endif
#if defined(SSE_DISK_HFE)
    || HFEDisk
#endif
    ) return 0;

  fclose(Format_f);

  Format_f=fopen(FormatTempFile,"r+b");
  if (Format_f) return true;

  return 0;
}
//---------------------------------------------------------------------------
// Seek in the disk image to the start of the required sector
//---------------------------------------------------------------------------
bool TFloppyImage::SeekSector(int Side,int Track,int Sector,bool Format)
{
  if (Format_f==NULL) Format=0;
//  TRACE("SEEK %d/%d\n",Track,TracksPerSide-1);
  if (Empty()){
    return true;
  }else if (Side<0 || Track<0 || Side>1){
    dbg_log(EasyStr("FDC: Seek Failed - Side ")+Side+" track "+Track+" sector "+Sector+" - negative values!");
    return true;
  }else if (Side>=int(Format ? 2:Sides)){
    dbg_log(EasyStr("FDC: Seek Failed - Can't seek to side ")+Side);
    return true;
  }else if (Track>=int(Format ? FLOPPY_MAX_TRACK_NUM+1:TracksPerSide)){
    dbg_log(EasyStr("FDC: Seek Failed - Can't seek to track ")+Track+" on side "+Side);
    return true;
  }

#if defined(SSE_DRIVE_SINGLE_SIDE_RND)//3.7.0
  if(SSEOption.SingleSideDriveMap&(DRIVE+1) && Side==1)
    return true;// -> RNF
#endif

  if (STT_File){
    DWORD TrackStart=STT_TrackStart[Side][Track],Magic=0;
    WORD DataFlags;

    if (TrackStart==0) return true; // Track doesn't exist
    fseek(f,TrackStart,SEEK_SET);
    if (fread(&Magic,4,1,f)==0){
      if (ReinsertDisk()==0) return true;
      if ((TrackStart=STT_TrackStart[Side][Track])==0) return true;
      fseek(f,TrackStart,SEEK_SET);
      fread(&Magic,4,1,f);
    }
    if (Magic!=MAKECHARCONST('T','R','C','K')) return true;
    fread(&DataFlags,2,1,f);

    bool Failed=true;
    if (DataFlags & BIT_0){       //Sectors
      WORD Offset,Flags,NumSectors;
      fread(&Offset,2,1,f);
      fread(&Flags,2,1,f);
      fread(&NumSectors,2,1,f);

      BYTE TrackNum,SideNum,SectorNum,LenIdx,CRC1,CRC2;
      WORD SectorOffset,SectorLen;
      for (int n=0;n<NumSectors;n++){
        fread(&TrackNum,1,1,f);
        fread(&SideNum,1,1,f);
        fread(&SectorNum,1,1,f);
        fread(&LenIdx,1,1,f);
        fread(&CRC1,1,1,f);
        fread(&CRC2,1,1,f);
        fread(&SectorOffset,2,1,f);
        fread(&SectorLen,2,1,f);

        // I'm not sure but it is very possible changing sides during a disk operation
        // would cause it to immediately start reading the other side
        //SS: we don't do that for SCP etc. it would return garbage
        if (TrackNum==Track && SideNum==floppy_current_side() && SectorNum==Sector && SectorLen!=0){
          fseek(f,TrackStart+SectorOffset,SEEK_SET);
          BytesPerSector=SectorLen;
          Failed=0;
          break;
        }
      }
    }else if (DataFlags & BIT_1){ // Raw track data (with bad syncs)
    }

    LOG_ONLY( if (Failed) dbg_log(EasyStr("FDC: Seek Failed - Can't find sector ")+Sector+" in track "+Track+" on side "+Side); )

    return Failed;
  }else{
    if (Sector==0 || Sector>int(Format ? FLOPPY_MAX_SECTOR_NUM:SectorsPerTrack)){
      dbg_log(EasyStr("FDC: Seek Failed - Can't seek to sector ")+Sector+" of track "+Track+" on side "+Side);
      return true;
    }

    if (Format==0){
      int HeaderLen=int(DIM_File ? 32:0);
      fseek(f,HeaderLen+(GetLogicalSector(Side,Track,Sector)*BytesPerSector),SEEK_SET);
    }else{
      fseek(Format_f,GetLogicalSector(Side,Track,Sector,true)*FLOPPY_MAX_BYTESPERSECTOR,SEEK_SET);
    }
    return false;  //no error!
  }
}
//---------------------------------------------------------------------------
long TFloppyImage::GetLogicalSector(int Side,int Track,int Sector,bool FormatFile)
{
  if (Empty()) return 0;

  if (FormatFile==0 || Format_f==NULL){
    return ((Track*Sides*SectorsPerTrack)+(Side*SectorsPerTrack)+(Sector-1));
  }

  return (Track*2*FLOPPY_MAX_SECTOR_NUM)+(Side*FLOPPY_MAX_SECTOR_NUM)+(Sector-1);
}
//---------------------------------------------------------------------------
int TFloppyImage::GetIDFields(int Side,int Track,FDC_IDField IDList[30])
{
  if (Empty()) return 0;

  if (STT_File){
    DWORD TrackStart=STT_TrackStart[Side][Track],Magic=0;
    WORD DataFlags;

    if (TrackStart==0) return 0;
    fseek(f,TrackStart,SEEK_SET);
    if (fread(&Magic,4,1,f)==0){
      if (ReinsertDisk()==0) return 0;
      if ((TrackStart=STT_TrackStart[Side][Track])==0) return 0;
      fseek(f,TrackStart,SEEK_SET);
      fread(&Magic,4,1,f);
    }
    if (Magic!=MAKECHARCONST('T','R','C','K')) return 0;
    fread(&DataFlags,2,1,f);

    if (DataFlags & BIT_0){       //Sectors
      DWORD Dummy;
      WORD Offset,Flags,NumSectors;
      fread(&Offset,2,1,f);
      fread(&Flags,2,1,f);
      fread(&NumSectors,2,1,f);
      for (int n=0;n<NumSectors;n++){
        fread(&IDList[n].Track,1,1,f);
        fread(&IDList[n].Side,1,1,f);
        fread(&IDList[n].SectorNum,1,1,f);
        fread(&IDList[n].SectorLen,1,1,f);
        fread(&IDList[n].CRC1,1,1,f);
        fread(&IDList[n].CRC2,1,1,f);
        fread(&Dummy,4,1,f); // SectorOffset, SectorLen
      }
      return NumSectors;
    }else if (DataFlags & BIT_1){ //Raw track
    }
    return 0;
  }else{
    bool Format=0;
    if (Track<=FLOPPY_MAX_TRACK_NUM) Format=TrackIsFormatted[Side][Track];
    if (Side>=int(Format ? 2:Sides)){
      return 0;
    }else if (Track>=int(Format ? FLOPPY_MAX_TRACK_NUM+1:TracksPerSide)){
      return 0;
    }
    for (int n=0;n<int(Format ? FormatMostSectors:SectorsPerTrack);n++){
      IDList[n].Track=BYTE(Track);
      IDList[n].Side=BYTE(Side);
#if defined(SSE_DRIVE_11_SECTORS)
/*  11 sectors interleave '6': 1 7 2 8 3 9 4 10 5 11 6
    -Load faster in ADAT mode (Overscan Demos)
    -ACopy 1.30 & Flofor report interleave = DRIVE_11SEC_INTERLEAVE (6), yeah!
    -ProCopy Analyse 1.50 'unformated', 2.02 finds IDs.
    -Write (format) 11 sectors tracks on ST/MSA: forget it
*/
//      if(SectorsPerTrack>=11)
      if(SectorsPerTrack==11) // bugfix v3.6.1, HD floppies in ADAT mode
        IDList[n].SectorNum=1+(n*DRIVE_11SEC_INTERLEAVE)%SectorsPerTrack;
      else
#endif
        IDList[n].SectorNum=BYTE(1+n);
      switch (BytesPerSector){
        case 128:  IDList[n].SectorLen=0; break;
        case 256:  IDList[n].SectorLen=1; break;
        case 1024: IDList[n].SectorLen=3; break;
        default:   IDList[n].SectorLen=2; break;
      }

      WORD CRC=0xffff;
      fdc_add_to_crc(CRC,0xa1);
      fdc_add_to_crc(CRC,0xa1);
      fdc_add_to_crc(CRC,0xa1);
      fdc_add_to_crc(CRC,0xfe);
      fdc_add_to_crc(CRC,IDList[n].Track);
      fdc_add_to_crc(CRC,IDList[n].Side);
      fdc_add_to_crc(CRC,IDList[n].SectorNum);
      fdc_add_to_crc(CRC,IDList[n].SectorLen);
      IDList[n].CRC1=HIBYTE(CRC);
      IDList[n].CRC2=LOBYTE(CRC);
    }
    return int(Format ? FormatMostSectors:SectorsPerTrack);
  }
}
//---------------------------------------------------------------------------
int TFloppyImage::GetRawTrackData(int Side,int Track)
{
  if (STT_File){
    DWORD TrackStart=STT_TrackStart[Side][Track],Magic;
    WORD DataFlags;

    if (TrackStart==0) return 0;
    fseek(f,TrackStart,SEEK_SET);
    if (fread(&Magic,4,1,f)==0){
      if (ReinsertDisk()==0) return 0;
      if ((TrackStart=STT_TrackStart[Side][Track])==0) return 0;
      fseek(f,TrackStart,SEEK_SET);
      fread(&Magic,4,1,f);
    }
    if (Magic!=MAKECHARCONST('T','R','C','K')) return 0;
    fread(&DataFlags,2,1,f);

    if (DataFlags & BIT_0){ // Skip this section if it exists
      WORD Offset;
      fread(&Offset,2,1,f);
      fseek(f,TrackStart+Offset,SEEK_SET);
    }
    if (DataFlags & BIT_1){ //Raw
      WORD Offset,Flags,TrackDataOffset,TrackDataLen;
      fread(&Offset,2,1,f);
      fread(&Flags,2,1,f);
      fread(&TrackDataOffset,2,1,f);
      fread(&TrackDataLen,2,1,f);

      fseek(f,TrackStart+TrackDataOffset,SEEK_SET);
      return TrackDataLen;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
#pragma warning (disable: 4701) //MSA vars
void TFloppyImage::RemoveDisk(bool LoseChanges)
{

#if  defined(SSE_VAR_DONT_REMOVE_NON_EXISTENT_IMAGES)
  if(Empty()) // nothing to do
    return;
#endif

#if defined(SSE_GUI_DISK_MANAGER_INSERT_DISKB_REMOVE)
  if((this==&FloppyDrive[0]) && DiskMan.AutoInsert2&2)
    DiskMan.EjectDisk(1); // v3.7.2 don't keep former disk B
  DiskMan.AutoInsert2&=~2; //TODO def
#endif

  static bool Removing=0;
  ASSERT(!Removing);//there must be a reason
  if (Removing) return;
  Removing=true;

#if defined(SSE_DISK_CAPS) \
|| defined(SSE_UNIX)
  int drive=-1;
  if (this==&FloppyDrive[0]) drive=0;
  if (this==&FloppyDrive[1]) drive=1;
#ifdef SSE_DEBUG
  if((drive==0 || drive==1) && !Empty())
    TRACE_LOG("Remove disk %s from drive %c\n",DiskName.Text,'A'+drive );
#endif
#endif

  if (f && ReadOnly==0 && LoseChanges==0 && WrittenTo && ZipTempFile.Empty()){
    short MSASecsPerTrack,MSAStartTrack=0,MSAEndTrack,MSASides;
    bool MSAResize=0;
    if (Format_f){
      if (FormatLargestSector>0){ // Formatted any track properly?
        // Try to merge the formatted data on Format_f with the old data on f
        int MaxTrack=0;
        for (int Side=0;Side<2;Side++){
          for (int Track=FLOPPY_MAX_TRACK_NUM;Track>0;Track--){
            if (TrackIsFormatted[Side][Track]){
              if (Track>MaxTrack) MaxTrack=Track;
              break;
            }
          }
        }
        if (MaxTrack>0){
          bool CanShrink=true,WipeOld=true;
          // Only shrink if all tracks were written
          for (int Track=MaxTrack;Track>=1;Track--){
            if (TrackIsFormatted[0][Track]==0){
              CanShrink=0;
              WipeOld=0;
              break;
            }
          }
          if (MaxTrack<70){ // Might want some old data left on the end
            CanShrink=0;
            WipeOld=0;
          }

          // Should we make it single sided?
          int NewSides=1;
          for (int Track=MaxTrack;Track>=0;Track--){
            if (TrackIsFormatted[1][Track]){
              NewSides=2;
            }else{
              // Don't wipe if haven't formatted over all sectors
              WipeOld=0;
            }
          }
          if (CanShrink==0) NewSides=max(NewSides,(int)(Sides));

          int NewTracksPerSide=(int)(CanShrink ? MaxTrack+1:max((int)TracksPerSide,MaxTrack+1));
          int NewSectorsPerTrack=(int)(CanShrink ? FormatMostSectors:max((int)SectorsPerTrack,FormatMostSectors));
          int NewBytesPerSector=(int)(WipeOld ? FormatLargestSector:max((int)BytesPerSector,FormatLargestSector));
          int NewBytesPerTrack=NewBytesPerSector*NewSectorsPerTrack;

          dbg_log("FDC: Formatted disk removed, copying data to disk image");
          dbg_log(EasyStr("  New format: Sides=")+NewSides+"  Tracks per side="+NewTracksPerSide+
                      "     SectorsPerTrack="+NewSectorsPerTrack);

          int HeaderLen=(int)(DIM_File ? 32:0);
          BYTE *NewDiskBuf=new BYTE[HeaderLen + NewBytesPerSector*NewSectorsPerTrack*NewTracksPerSide*NewSides];
          BYTE *lpNewDisk=NewDiskBuf;
          ZeroMemory(NewDiskBuf,HeaderLen + NewBytesPerSector*NewSectorsPerTrack*NewTracksPerSide*NewSides);
          if (HeaderLen){
            // Keep the header if there is one
            fseek(f,0,SEEK_SET);
            fread(lpNewDisk,HeaderLen,1,f);
            lpNewDisk+=HeaderLen;
          }

          for (int t=0;t<NewTracksPerSide;t++){
            for (int Side=0;Side<NewSides;Side++){
              int Countdown=3;
              if (TrackIsFormatted[Side][t]){
                // Read a track from the format file
                for (int s=1;s<=NewSectorsPerTrack;s++){
                  bool NextSector=true;
                  SeekSector(Side,t,s,true);
                  if (fread(lpNewDisk,1,NewBytesPerSector,Format_f)<size_t(NewBytesPerSector)){
                    if ((Countdown--)>0){
                      if (ReopenFormatFile()){
                        s--; // Try to redo a sector 3 times
                        NextSector=0;
                      }
                    }
                  }
                  if (NextSector) lpNewDisk+=NewBytesPerSector;
                }
              }else if (t<TracksPerSide && Side<Sides){
                // Copy information from the old disk onto the new disk
                for (int s=1;s<=min(int(SectorsPerTrack),NewSectorsPerTrack);s++){
                  bool NextSector=true;
                  SeekSector(Side,t,s,0);
                  if (fread(lpNewDisk,1,min(int(BytesPerSector),NewBytesPerSector),f)<size_t(min(int(BytesPerSector),NewBytesPerSector))){
                    if ((Countdown--)>0){
                      if (ReinsertDisk()){
                        s--; // Try to redo a sector 3 times
                        NextSector=0;
                      }
                    }
                  }
                  if (NextSector) lpNewDisk+=NewBytesPerSector;
                }
                // If getting bigger then skip
                for (int s=SectorsPerTrack;s<NewSectorsPerTrack;s++) lpNewDisk+=NewBytesPerSector;
              }else{
                lpNewDisk+=NewBytesPerTrack;
              }
            }
          }

          // Write it back to the original file (finally)
          int Countdown=3;
          LOOP{
            fclose(f);

            SectorsPerTrack=short(NewSectorsPerTrack);
            Sides=short(NewSides);
            TracksPerSide=short(NewTracksPerSide);
            BytesPerSector=short(NewBytesPerSector);

            if (MSATempFile.NotEmpty()){
              MSASecsPerTrack=short(NewSectorsPerTrack);
              MSAEndTrack=short(NewTracksPerSide+1);
              MSASides=short(NewSides-1);
              MSAResize=true;
              f=fopen(MSATempFile,"wb");
            }else{
              f=fopen(ImageFile,"wb");
            }
            if (f){
              fseek(f,0,SEEK_SET);
              int NewDiskSize=HeaderLen + NewBytesPerSector*NewSectorsPerTrack*NewTracksPerSide*NewSides;
              if (fwrite(NewDiskBuf,1,NewDiskSize,f)==size_t(NewDiskSize)){
                ConfigStoreFile CSF(ImageFile+".steembpb");
                CSF.SetStr("BPB","Sides",Str(Sides));
                CSF.SetStr("BPB","SectorsPerTrack",Str(SectorsPerTrack));
                CSF.SetStr("BPB","BytesPerSector",Str(BytesPerSector));
                CSF.SetStr("BPB","Sectors",Str(SectorsPerTrack*TracksPerSide*Sides));
                CSF.Close();
                break;
              }else{
                if ((--Countdown)<0){
                  log_write("Error writing to disk image after format! All data lost!");
                  break;
                }
              }
            }else{
              log_write("Error opening disk image after format! All data lost!");
              break;
            }
          }

          delete[] NewDiskBuf;
        }
      }
    }
    if (MSATempFile.NotEmpty() && f){
      // Write ST format MSATempFile to MSA format ImageFile
      WIN_ONLY( if (stem_mousemode!=STEM_MOUSEMODE_WINDOW) SetCursor(LoadCursor(NULL,IDC_WAIT)); )

      FILE *MSA=fopen(ImageFile,"r+b");
      if (MSA){
        BYTE Temp;
        fseek(MSA,2,SEEK_SET); //Seek past ID
        if (MSAResize==0){
          fread(&MSASecsPerTrack,2,1,MSA); SWAPBYTES(MSASecsPerTrack);
          fread(&MSASides,2,1,MSA);        SWAPBYTES(MSASides);
          fseek(MSA,2,SEEK_CUR);        // Skip StartTrack
          fread(&MSAEndTrack,2,1,MSA);     SWAPBYTES(MSAEndTrack);

          fseek(MSA,6,SEEK_SET);
          Temp=HIBYTE(MSAStartTrack);   fwrite(&Temp,1,1,MSA);
          Temp=LOBYTE(MSAStartTrack);   fwrite(&Temp,1,1,MSA);
        }else{
          // Write out MSA file info (in big endian)
          Temp=HIBYTE(MSASecsPerTrack); fwrite(&Temp,1,1,MSA);
          Temp=LOBYTE(MSASecsPerTrack); fwrite(&Temp,1,1,MSA);
          Temp=HIBYTE(MSASides);        fwrite(&Temp,1,1,MSA);
          Temp=LOBYTE(MSASides);        fwrite(&Temp,1,1,MSA);
          Temp=HIBYTE(MSAStartTrack);   fwrite(&Temp,1,1,MSA);
          Temp=LOBYTE(MSAStartTrack);   fwrite(&Temp,1,1,MSA);
          Temp=HIBYTE(MSAEndTrack);     fwrite(&Temp,1,1,MSA);
          Temp=LOBYTE(MSAEndTrack);     fwrite(&Temp,1,1,MSA);
        }

        fseek(MSA,10,SEEK_SET); // Past header

        fseek(f,0,SEEK_SET);

        int Len=WORD(MSASecsPerTrack*512);

        // Convert ST format f to MSA format MSA (uncompressed)
        int ReinsertAttempts=0;
        BYTE *MSADataBuf=new BYTE[(MSAEndTrack+1)*(MSASides+1)*MSASecsPerTrack*(512+2)];
        BYTE *pD=MSADataBuf;
        for (int t=0;t<=MSAEndTrack;t++){
          for (int s=0;s<=MSASides;s++){
            *(pD++)=HIBYTE(Len);
            *(pD++)=LOBYTE(Len);
            for (int sec=1;sec<=MSASecsPerTrack;sec++){
              SeekSector(s,t,sec,0);
              if (fread(pD,1,512,f)==512){ // Read sector from ST file
                pD+=512;
              }else if (ReinsertAttempts<5){
                ReinsertDisk();
                sec--;
                ReinsertAttempts++;
              }else{ // All else has failed, write an empty sector
                ZeroMemory(pD,512);
                pD+=512;
              }
            }
          }
        }
        fwrite(MSADataBuf,1,long(pD)-long(MSADataBuf),MSA);
        fclose(MSA);

        delete[] MSADataBuf;
      }
      WIN_ONLY( if (stem_mousemode!=STEM_MOUSEMODE_WINDOW) SetCursor(PCArrow); )
    }
  }

#if USE_PASTI
  if (PastiDisk){
#if !(defined(SSE_DISK_CAPS)) // put above
    int drive=-1;
    if (this==&FloppyDrive[0]) drive=0;
    if (this==&FloppyDrive[1]) drive=1;
#endif
    if (ZipTempFile.Empty() && ReadOnly==0 && drive>=0 && hPasti && LoseChanges==0){
      struct pastiDISKIMGINFO pdi;
      pdi.mode=PASTI_LDFNAME;
      pdi.fileName=ImageFile;
      pdi.fileBuf=PastiBuf;
      pdi.bufSize=PastiBufLen;
      if (pasti->SaveImg(drive,0,&pdi)==FALSE){
#if !(defined(SSE_DISK_PASTI_NO_RESET))
        int err=pasti->GetLastError();
        if (err!=pastiErrUnimpl){
          Alert(T("Unable to save to disk image, changes have been lost!"),T("Disk Image Error"),MB_OK);
        }
#endif
      }
    }
    pasti->Eject(drive,ABSOLUTE_CPU_TIME);
#if defined(SSE_DISK_PASTI_ONLY_STX)
/*  3.6.1 Disable Pasti at once when ejecting STX disk if the other disk
    isn't STX and option 'Pasti only for STX' is checked.
*/
    if(PASTI_JUST_STX && 
      SF314[1-floppy_current_drive()].ImageType.Extension!=EXT_STX)
    {
      pasti_active=false;
    }
#endif
  }
#endif
  if (PastiBuf) delete[] PastiBuf;
  PastiBuf=NULL;
  PastiBufLen=0;
  PastiDisk=0;

#if defined(SSE_DISK_CAPS)
  if(CAPSIMG_OK && drive!=-1 && SF314[drive].ImageType.Manager==MNGR_CAPS)
    Caps.RemoveDisk(drive);
  IPFDisk=
#ifdef SSE_DISK_CAPS_CTRAW
    CTRDisk=
#endif
    0;
#endif

#if defined(SSE_DISK_STW)
  if(SF314[drive].ImageType.Extension==EXT_STW)
    ImageSTW[drive].Close();
  STWDisk=0;
  SF314[drive].State.reading=SF314[drive].State.writing=0;
#endif

#if defined(SSE_DISK_SCP)
  if(SF314[drive].ImageType.Extension==EXT_SCP)
    ImageSCP[drive].Close();
  SCPDisk=0;
#if !defined(SSE_DISK_STW)
  SF314[drive].State.reading=SF314[drive].State.writing=0;
#endif
#endif

#if defined(SSE_DISK_HFE)
  if(SF314[drive].ImageType.Extension==EXT_HFE)
    ImageHFE[drive].Close();
  HFEDisk=0;
#if !defined(SSE_DISK_STW) && !defined(SSE_DISK_SCP)
  SF314[drive].State.reading=SF314[drive].State.writing=0;
#endif
#endif

  if (f) fclose(f);
  f=NULL;
  if (Format_f) fclose(Format_f);
  Format_f=NULL;

#if defined(SSE_DISK_IMAGETYPE) 
  SF314[drive].ImageType.Manager=MNGR_STEEM; //default
  SF314[drive].ImageType.Extension=0;
#endif
#if defined(SSE_FLOPPY_ADAT_UPDATE)
  SF314[drive].UpdateAdat();
#endif
#if defined(SSE_DISK_GHOST)
  // This makes sure to update the image before leaving, though
  // there's a destructor. Really needed?
  if(SSE_GHOST_DISK && SF314[drive].State.ghost)
  {
    GhostDisk[drive].Close();
    SF314[drive].State.ghost=0; 
  }
#endif

  if (ZipTempFile.NotEmpty())    DeleteFile(ZipTempFile);
  if (MSATempFile.NotEmpty())    DeleteFile(MSATempFile);
  if (FormatTempFile.NotEmpty()) DeleteFile(FormatTempFile);

  ImageFile="";MSATempFile="";ZipTempFile="";FormatTempFile="";
  DiskName="";
  ReadOnly=true;
  BytesPerSector=0;Sides=0;SectorsPerTrack=0;TracksPerSide=0;
  ZeroMemory(TrackIsFormatted,sizeof(TrackIsFormatted));

  FormatMostSectors=0;FormatLargestSector=0;
  STT_File=0;
  DIM_File=0;

  Removing=0;
}
#pragma warning (default: 4701)
//---------------------------------------------------------------------------
#undef LOGSECTION

