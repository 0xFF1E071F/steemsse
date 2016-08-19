/*---------------------------------------------------------------------------
FILE: loadsave_emu.cpp
MODULE: emu
DESCRIPTION: Functions to load and save emulation variables. This is mainly
for Steem's memory snapshots system.
---------------------------------------------------------------------------*/

#if defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: loadsave_emu.cpp")
#endif

#if defined(SSE_STRUCTURE_DECLA)
extern EasyStr RunDir,WriteDir,INIFile,ScreenShotFol;
extern EasyStr LastSnapShot,BootStateFile,StateHist[10],AutoSnapShotName;
#undef LOGSECTION
#define LOGSECTION LOGSECTION_INIT

#if defined(SSE_VS2008)
#pragma warning(disable : 4258)
#endif

#endif



//---------------------------------------------------------------------------
void ReadWriteVar(void *lpVar,DWORD szVar,NOT_ONEGAME( FILE *f ) ONEGAME_ONLY( BYTE* &pMem ),
                        int LoadOrSave,int Type,int Version)
{
  bool SaveSize;   // SS Steem saves size, so we may extend structs with no worry
  if (Type==0){ // Variable
    SaveSize=(Version==17); //SS wasn't stupid either
  }else if (Type==1){ // Array
    SaveSize=(Version>=3);
  }else{  // Struct
    SaveSize=(Version>=5);
  }

#ifndef ONEGAME
  if (SaveSize==0){
    if (LoadOrSave==LS_SAVE){
      fwrite(lpVar,1,szVar,f);
    }else{
      fread(lpVar,1,szVar,f);
    }
//    log_write(Str(szVar));
  }else{
    if (LoadOrSave==LS_SAVE){
      fwrite(&szVar,1,sizeof(szVar),f);
      fwrite(lpVar,1,szVar,f);
//      log_write(Str("Block, l=")+szVar);
    }else{
      DWORD l=0;
      fread(&l,1,sizeof(l),f);
      if (szVar<l){
        fread(lpVar,1,szVar,f);
        fseek(f,l-szVar,SEEK_CUR);
      }else{
        fread(lpVar,1,l,f);
      }
//      log_write(Str("Block, l=")+l);
    }
  }
#else
  if (LoadOrSave==LS_LOAD){
    BYTE *pVar=(BYTE*)lpVar;
    if (SaveSize==0){
      for (DWORD n=0;n<szVar;n++) *(pVar++)=*(pMem++);
    }else{
      DWORD l=*LPDWORD(pMem);pMem+=4;
      for (DWORD n=0;n<l;n++){
        BYTE b=*(pMem++);
        if (n<szVar) *(pVar++)=b;
      }
    }
  }
#endif
}
//---------------------------------------------------------------------------
int ReadWriteEasyStr(EasyStr &s,NOT_ONEGAME( FILE *f ) ONEGAME_ONLY( BYTE* &pMem ),int LoadOrSave,int)
{
#ifndef ONEGAME
  int l;
  if (LoadOrSave==LS_SAVE){
    l=s.Length();
    fwrite(&l,1,sizeof(l),f);
    fwrite(s.Text,1,l,f);
//    log_write(Str("String, l=")+l);
  }else{
    l=-1;
    fread(&l,1,sizeof(l),f);
//    log_write(Str("String, l=")+l);
    if (l<0 || l>260) return 2; // Corrupt snapshot
    s.SetLength(l);
    if (l) fread(s.Text,1,l,f);
  }
#else
  if (LoadOrSave==LS_LOAD){
    int l=*(int*)(pMem);pMem+=4;
    if (l<0 || l>260) return 2; // Corrupt snapshot
    s.SetLength(l);
    char *pT=s.Text;
    for (int n=0;n<l;n++) *(pT++)=(char)*(pMem++);
  }
#endif

  return 0;
}

#define ReadWrite(var) ReadWriteVar(&(var),sizeof(var),f,LoadOrSave,0,Version)
#define ReadWriteArray(var) ReadWriteVar(var,sizeof(var),f,LoadOrSave,1,Version)
#define ReadWriteStruct(var) ReadWriteVar(&(var),sizeof(var),f,LoadOrSave,2,Version)

#define ReadWriteStr(s) {int i=ReadWriteEasyStr(s,f,LoadOrSave,Version);if (i) return i; }
//---------------------------------------------------------------------------
int LoadSaveAllStuff(NOT_ONEGAME( FILE *f ) ONEGAME_ONLY( BYTE* &f ),
                      bool LoadOrSave,int Version,bool NOT_ONEGAME( ChangeDisksAndCart ),int *pVerRet)
{
  ONEGAME_ONLY( BYTE *pStartByte=f; )
  if (Version==-1) Version=SNAPSHOT_VERSION;

  //TRACE_INIT("%s memory snaphot V%d\n",(LoadOrSave==LS_LOAD?"Load":"Save"),Version);

  ReadWrite(Version);        //4
  if (pVerRet) *pVerRet=Version;

  ReadWrite(pc);             //4
  ReadWrite(pc_high_byte);   //4

  ReadWriteArray(r);//SS 64
  ReadWrite(sr);             //2
  ReadWrite(other_sp);       //4


  ReadWrite(xbios2);         //4
  ReadWriteArray(STpal);//SS 32

  ReadWrite(interrupt_depth); //4
  ReadWrite(on_rte);          //4
  ReadWrite(on_rte_interrupt_depth); //4
  ReadWrite(shifter_draw_pointer); //4
  ReadWrite(shifter_freq);         //4
  if (shifter_freq>65) shifter_freq=MONO_HZ;
  ReadWrite(shifter_x);            //4
  ReadWrite(shifter_y);            //4
  ReadWrite(shifter_scanline_width_in_bytes); //4 //SS: unused
#if defined(SSE_VAR_RESIZE)
  {
    int tmp=shifter_fetch_extra_words;
    ReadWrite(tmp);       //4
    shifter_fetch_extra_words=(BYTE)tmp;
    tmp=shifter_hscroll;
    ReadWrite(tmp);       //4
    shifter_hscroll=(BYTE)tmp;
  }
#else
  ReadWrite(shifter_fetch_extra_words);       //4
  ReadWrite(shifter_hscroll);                 //4
#endif
  if (Version==17 || Version==18){ // The unreleased freak versions!
    ReadWrite(screen_res);
  }else{
    char temp=char(screen_res);
    ReadWrite(temp);                //1
    screen_res=temp;
  }

  ReadWrite(mmu_memory_configuration);  //1

  ReadWriteArray(mfp_reg);//SS 24

  int dummy[4]; //was mfp_timer_precounter[4];
  ReadWriteArray(dummy);//SS 16

  {
    // Make sure saving can't affect current emulation
    int save_mfp_timer_counter[4],save_mfp_timer_period[4];
    if (LoadOrSave==LS_SAVE){
      memcpy(save_mfp_timer_counter,mfp_timer_counter,sizeof(mfp_timer_counter));
      memcpy(save_mfp_timer_period,mfp_timer_period,sizeof(mfp_timer_period));
      for (int n=0;n<4;n++) mfp_calc_timer_counter(n);
    }
    ReadWriteArray(mfp_timer_counter);//SS 16
    if (LoadOrSave==LS_SAVE){
      memcpy(mfp_timer_counter,save_mfp_timer_counter,sizeof(mfp_timer_counter));
      memcpy(mfp_timer_period,save_mfp_timer_period,sizeof(mfp_timer_period));
    }
  }

  ReadWrite(mfp_gpip_no_interrupt); //1
#if defined(SSE_VAR_RESIZE_383)
  int i_psg_reg_select=psg_reg_select;
  ReadWrite(i_psg_reg_select);      //4
  psg_reg_select=i_psg_reg_select;
#else
  ReadWrite(psg_reg_select);        //4
#endif
  ReadWriteArray(psg_reg);//SS16

  ReadWrite(dma_sound_control);     //1
  ReadWrite(dma_sound_start);       //4
  ReadWrite(dma_sound_end);         //4
  ReadWrite(dma_sound_mode);        //1

  ReadWriteStruct(ikbd);
  ReadWriteArray(keyboard_buffer);//SS 1024
  ReadWrite(keyboard_buffer_length); //4
  if (Version<8){
    keyboard_buffer[0]=0;
    keyboard_buffer_length=0;
  }

  ReadWrite(dma_mode);          //2
  ReadWrite(dma_status);        //1
  ReadWrite(dma_address);       //4
#if defined(SSE_DMA_OBJECT)
  {
    int dma_sector_count_tmp=dma_sector_count;
    ReadWrite(dma_sector_count_tmp);
    dma_sector_count=dma_sector_count_tmp;
  }
#else
  ReadWrite(dma_sector_count);  //4
#endif
  ReadWrite(fdc_cr);            //1
  ReadWrite(fdc_tr);            //1
  ReadWrite(fdc_sr);            //1
  ReadWrite(fdc_str);           //1
  ReadWrite(fdc_dr);                     //1
#if (defined(SSE_DISK_STW))
  BYTE dummy_byte;
  ReadWrite(dummy_byte);
#else
  ReadWrite(fdc_last_step_inwards_flag); //1
#endif
  ReadWriteArray(floppy_head_track);
#if defined(SSE_VAR_RESIZE)
  {
    int floppy_mediach_tmp[2];
    floppy_mediach_tmp[0]=floppy_mediach[0];
    floppy_mediach_tmp[1]=floppy_mediach[1];
    ReadWriteArray(floppy_mediach_tmp);//2
    floppy_mediach[0]=floppy_mediach_tmp[0];
    floppy_mediach[1]=floppy_mediach_tmp[1];
  }
#else
  ReadWriteArray(floppy_mediach);//SS 8
#endif
#ifdef DISABLE_STEMDOS
  int stemdos_Pexec_list_ptr=0;
  MEM_ADDRESS stemdos_Pexec_list[76];//SS must be MAX_STEMDOS_PEXEC_LIST
  ZeroMemory(stemdos_Pexec_list,sizeof(stemdos_Pexec_list));
  int stemdos_current_drive=0;
#endif
  ReadWrite(stemdos_Pexec_list_ptr);  //4
  ReadWriteArray(stemdos_Pexec_list); //SS 304
#if defined(SSE_VAR_RESIZE_382)
  int stemdos_current_drive_int=stemdos_current_drive;
  ReadWrite(stemdos_current_drive_int);
  stemdos_current_drive=stemdos_current_drive_int;
#else
  ReadWrite(stemdos_current_drive);   //4
#endif
  EasyStr NewROM=ROMFile;
  ReadWriteStr(NewROM);
  WORD NewROMVer=tos_version;
  if (Version>=7){
    ReadWrite(NewROMVer); // 2
  }else{
    NewROMVer=0x701;
  }

  ReadWrite(bank_length[0]);       // 4
  ReadWrite(bank_length[1]);       // 4 
  if (LoadOrSave==LS_LOAD){
    BYTE MemConf[2]={MEMCONF_512,MEMCONF_512};
    GetCurrentMemConf(MemConf);
    delete[] Mem;Mem=NULL;
    make_Mem(MemConf[0],MemConf[1]);
  }

  EasyStr NewDiskName[2],NewDisk[2];
  if (Version>=1){
    for (int disk=0;disk<2;disk++){
      NewDiskName[disk]=FloppyDrive[disk].DiskName;
      ReadWriteStr(NewDiskName[disk]);
      NewDisk[disk]=FloppyDrive[disk].GetDisk();
      ReadWriteStr(NewDisk[disk]);
    }
  }
  if (Version>=2){
#ifdef DISABLE_STEMDOS
    Str mount_gemdos_path[26];
#endif
    for (int n=0;n<26;n++) ReadWriteStr(mount_gemdos_path[n]);
#ifndef DISABLE_STEMDOS
    if (LoadOrSave==LS_LOAD) stemdos_check_paths();
#endif
  }
  if (Version>=4) ReadWriteStruct(Blit);

  if (Version>=5) ReadWriteArray(ST_Key_Down);

#if defined(SSE_VAR_RESIZE_383A) // problem is memory snapshots, structure: more complicated
  struct {
    int clock_divide;
    int rx_delay__unused;
    BYTE rx_irq_enabled;
    BYTE rx_not_read;
    int overrun;
    int tx_flag;
    BYTE tx_irq_enabled;
    BYTE data;
    BYTE irq;
    int last_tx_write_time;
    int last_rx_read_time;
    BYTE LineRxBusy;
    BYTE ByteWaitingRx;
    BYTE ByteWaitingTx;
    BYTE LineTxBusy;
    BYTE CR,SR,RDR,TDR,RDRS,TDRS;
  } old_acia[2];

  old_acia[NUM_ACIA_IKBD].clock_divide=ACIA_IKBD.clock_divide;
  old_acia[NUM_ACIA_IKBD].rx_irq_enabled=ACIA_IKBD.rx_irq_enabled;
  old_acia[NUM_ACIA_IKBD].rx_not_read=ACIA_IKBD.rx_not_read;
  old_acia[NUM_ACIA_IKBD].overrun=ACIA_IKBD.overrun;
  old_acia[NUM_ACIA_IKBD].tx_flag=ACIA_IKBD.tx_flag;
  old_acia[NUM_ACIA_IKBD].tx_irq_enabled=ACIA_IKBD.tx_irq_enabled;
  old_acia[NUM_ACIA_IKBD].data=ACIA_IKBD.data;
  old_acia[NUM_ACIA_IKBD].irq=ACIA_IKBD.irq;
  old_acia[NUM_ACIA_IKBD].last_tx_write_time=ACIA_IKBD.last_tx_write_time;
  old_acia[NUM_ACIA_IKBD].last_rx_read_time=ACIA_IKBD.last_rx_read_time;
#if defined(SSE_ACIA_DOUBLE_BUFFER_RX)
  old_acia[NUM_ACIA_IKBD].LineRxBusy=ACIA_IKBD.LineRxBusy;
  old_acia[NUM_ACIA_IKBD].ByteWaitingRx=ACIA_IKBD.ByteWaitingRx;
#endif
#if defined(SSE_ACIA_DOUBLE_BUFFER_TX)
  old_acia[NUM_ACIA_IKBD].ByteWaitingTx=ACIA_IKBD.ByteWaitingTx;
  old_acia[NUM_ACIA_IKBD].LineTxBusy=ACIA_IKBD.LineTxBusy;
#endif
#if defined(SSE_ACIA_REGISTERS)
  old_acia[NUM_ACIA_IKBD].CR=ACIA_IKBD.CR;
  old_acia[NUM_ACIA_IKBD].SR=ACIA_IKBD.SR;
  old_acia[NUM_ACIA_IKBD].RDR=ACIA_IKBD.RDR;
  old_acia[NUM_ACIA_IKBD].TDR=ACIA_IKBD.TDR;
  old_acia[NUM_ACIA_IKBD].RDRS=ACIA_IKBD.RDRS;
  old_acia[NUM_ACIA_IKBD].TDRS=ACIA_IKBD.TDRS;
#endif

  if (Version>=8) ReadWriteStruct(old_acia[NUM_ACIA_IKBD]);

  ACIA_IKBD.clock_divide=old_acia[NUM_ACIA_IKBD].clock_divide;
  ACIA_IKBD.rx_irq_enabled=old_acia[NUM_ACIA_IKBD].rx_irq_enabled;
  ACIA_IKBD.rx_not_read=old_acia[NUM_ACIA_IKBD].rx_not_read;
  ACIA_IKBD.overrun=old_acia[NUM_ACIA_IKBD].overrun;
  ACIA_IKBD.tx_flag=old_acia[NUM_ACIA_IKBD].tx_flag;
  ACIA_IKBD.tx_irq_enabled=old_acia[NUM_ACIA_IKBD].tx_irq_enabled;
  ACIA_IKBD.data=old_acia[NUM_ACIA_IKBD].data;
  ACIA_IKBD.irq=old_acia[NUM_ACIA_IKBD].irq;
  ACIA_IKBD.last_tx_write_time=old_acia[NUM_ACIA_IKBD].last_tx_write_time;
  ACIA_IKBD.last_rx_read_time=old_acia[NUM_ACIA_IKBD].last_rx_read_time;
#if defined(SSE_ACIA_DOUBLE_BUFFER_RX)
  ACIA_IKBD.LineRxBusy=old_acia[NUM_ACIA_IKBD].LineRxBusy;
  ACIA_IKBD.ByteWaitingRx=old_acia[NUM_ACIA_IKBD].ByteWaitingRx;
#endif
#if defined(SSE_ACIA_DOUBLE_BUFFER_TX)
  ACIA_IKBD.ByteWaitingTx=old_acia[NUM_ACIA_IKBD].ByteWaitingTx;
  ACIA_IKBD.LineTxBusy=old_acia[NUM_ACIA_IKBD].LineTxBusy;
#endif
#if defined(SSE_ACIA_REGISTERS)
  ACIA_IKBD.CR=old_acia[NUM_ACIA_IKBD].CR;
  ACIA_IKBD.SR=old_acia[NUM_ACIA_IKBD].SR;
  ACIA_IKBD.RDR=old_acia[NUM_ACIA_IKBD].RDR;
  ACIA_IKBD.TDR=old_acia[NUM_ACIA_IKBD].TDR;
  ACIA_IKBD.RDRS=old_acia[NUM_ACIA_IKBD].RDRS;
  ACIA_IKBD.TDRS=old_acia[NUM_ACIA_IKBD].TDRS;
#endif
#else
  if (Version>=8) ReadWriteStruct(ACIA_IKBD);
#endif
 

#if defined(SSE_ACIA_REGISTERS)
  if(Version<44 && LoadOrSave==LS_LOAD) //v3.5.1
  {
    ACIA_IKBD.CR=0x96; // usually
    ACIA_IKBD.SR=2; // usually
    ACIA_MIDI.CR=0x95; // usually
    ACIA_MIDI.SR=2; // usually
  //}//3.8.0
#if defined(SSE_ACIA_DOUBLE_BUFFER_RX) && !defined(SSE_ACIA_380)
    ACIA_IKBD.LineRxBusy=0;
#endif
#if defined(SSE_ACIA_DOUBLE_BUFFER_TX) && !defined(SSE_ACIA_380)
    ACIA_IKBD.LineTxBusy=0;
#endif
    }//3.8.0
#endif

  if (Version>=9){
#ifdef DISABLE_STEMDOS
    MEM_ADDRESS stemdos_dta;
#endif
    ReadWrite(stemdos_dta); //4
  }

  if (Version>=10){
    ReadWrite(dma_sound_fetch_address);    //4
    ReadWrite(next_dma_sound_end);   //4
    ReadWrite(next_dma_sound_start); //4
  }else if (LoadOrSave==LS_LOAD){
    next_dma_sound_end=dma_sound_end;
    next_dma_sound_start=dma_sound_start;
    dma_sound_fetch_address=dma_sound_start;
  }
//  dma_sound_subdivide_minus_one=dma_mode_to_subdivide[sound_low_quality][dma_sound_mode & 3];
  dma_sound_freq=dma_sound_mode_to_freq[dma_sound_mode & 3];
  dma_sound_output_countdown=0;

  DWORD StartOfData=0;
  NOT_ONEGAME( DWORD StartOfDataPos=ftell(f); )
  if (Version>=11) ReadWrite(StartOfData);

  if (Version>=12){
    ReadWrite(os_gemdos_vector);
    ReadWrite(os_bios_vector);
    ReadWrite(os_xbios_vector);
  }

  if (Version>=13) ReadWrite(paddles_ReadMask);

  EasyStr NewCart=CartFile;
  if (Version>=14) ReadWriteStr(NewCart);

  if (Version>=15){
    ReadWrite(rs232_recv_byte);
    ReadWrite(rs232_recv_overrun);
    ReadWrite(rs232_bits_per_word);
    ReadWrite(rs232_hbls_per_word);
  }

  EasyStr NewDiskInZip[2];
  if (Version>=20){
    for (int disk=0;disk<2;disk++){
      NewDiskInZip[disk]=FloppyDrive[disk].DiskInZip;
      ReadWriteStr(NewDiskInZip[disk]);
    }
  }

#ifndef ONEGAME
  bool ChangeTOS=true,ChangeCart=ChangeDisksAndCart,ChangeDisks=ChangeDisksAndCart;
#endif
  DWORD ExtraFlags=0;

  if (Version>=21) ReadWrite(ExtraFlags);

#ifndef ONEGAME
  if (ExtraFlags & BIT_0) ChangeDisks=0;
  // Flag here for saving disks in this file? (huge!)
  if (ExtraFlags & BIT_1) ChangeTOS=0;
  // Flag here for only asking user to locate version and country code TOS?
  // Flag here for saving TOS in this file?
  if (ExtraFlags & BIT_2) ChangeCart=0;
  // Flag here for saving the cart in this file?
#endif

  if (Version>=22){
#ifdef DISABLE_STEMDOS
    #define MAX_STEMDOS_FSNEXT_STRUCTS 100
    struct _STEMDOS_FSNEXT_STRUCT{
      MEM_ADDRESS dta;
      EasyStr path;
      EasyStr NextFile;
      int attr;
      DWORD start_hbl;
    }stemdos_fsnext_struct[MAX_STEMDOS_FSNEXT_STRUCTS];
#endif
    int max_fsnexts=MAX_STEMDOS_FSNEXT_STRUCTS;
    ReadWrite(max_fsnexts);
    for (int n=0;n<max_fsnexts;n++){
      ReadWrite(stemdos_fsnext_struct[n].dta);
      // If this is invalid then it will just return "no more files"
      ReadWriteStr(stemdos_fsnext_struct[n].path);
      ReadWriteStr(stemdos_fsnext_struct[n].NextFile);
      ReadWrite(stemdos_fsnext_struct[n].attr);
      ReadWrite(stemdos_fsnext_struct[n].start_hbl);
    }
  }

  if (Version>=23) ReadWrite(shifter_hscroll_extra_fetch);

#ifdef NO_CRAZY_MONITOR
  int em_width=480,em_height=480,em_planes=4,extended_monitor=0,aes_calls_since_reset=0;
  long save_r[16];
  MEM_ADDRESS line_a_base=0,vdi_intout=0;
#endif
  bool old_em=bool(extended_monitor);
  if (Version>=24){

#if defined(SSE_VAR_RESIZE_383)
    // what we win in pure size, we lose in code, but this function could be paged
    int em_ints[4];
    em_ints[0]=em_width;
    em_ints[1]=em_height;
    em_ints[2]=em_planes;
    em_ints[3]=extended_monitor;
    for(int i=0;i<4;i++)
      ReadWrite(em_ints[i]);
    em_width=em_ints[0];
    em_height=em_ints[1];
    em_planes=em_ints[2];
    extended_monitor=em_ints[3];
#else
    ReadWrite(em_width);
    ReadWrite(em_height);
    ReadWrite(em_planes);
    ReadWrite(extended_monitor);
#endif
    ReadWrite(aes_calls_since_reset);
    ReadWriteArray(save_r);
    ReadWrite(line_a_base);
    ReadWrite(vdi_intout);

#ifdef SSE_TOS_GEMDOS_EM_381B
    if(LoadOrSave==LS_LOAD)
      vdi_intout=line_a_base=0;//?
#endif


  }else if (LoadOrSave==LS_LOAD){
    extended_monitor=0;
  }

  if (Version>=25){
    int save_mfp_timer_counter[4],save_mfp_timer_period[4];
    if (LoadOrSave==LS_SAVE){
      memcpy(save_mfp_timer_counter,mfp_timer_counter,sizeof(mfp_timer_counter));
      memcpy(save_mfp_timer_period,mfp_timer_period,sizeof(mfp_timer_period));
    }
    for (int n=0;n<4;n++){
      BYTE prescale_ticks=0;
      if (LoadOrSave==LS_SAVE) prescale_ticks=(BYTE)mfp_calc_timer_counter(n);
      ReadWrite(prescale_ticks);
      if (LoadOrSave==LS_LOAD){
        int ticks_per_count=mfp_timer_prescale[mfp_get_timer_control_register(n) & 7];
        mfp_timer_counter[n]-=int(double(64.0/ticks_per_count)*double(prescale_ticks));
      }
    }
    if (LoadOrSave==LS_SAVE){
      memcpy(mfp_timer_counter,save_mfp_timer_counter,sizeof(mfp_timer_counter));
      memcpy(mfp_timer_period,save_mfp_timer_period,sizeof(mfp_timer_period));
    }
  }
  if (Version>=26){
    ReadWriteArray(mfp_timer_period);
  }else if (LoadOrSave==LS_LOAD){
    for (int timer=0;timer<4;timer++) MFP_CALC_TIMER_PERIOD(timer);
  }
  if (Version>=27){
    ReadWriteArray(mfp_timer_period_change);
  }else if (LoadOrSave==LS_LOAD){
    for (int timer=0;timer<4;timer++) mfp_timer_period_change[timer]=0;
  }
  if (Version>=28){
    int rel_time=0;
    ReadWrite(rel_time);
  }

  if (Version>=29){
    ReadWrite(emudetect_called);
    if (LoadOrSave==LS_LOAD) emudetect_init();
  }

  if (Version>=30){
    ReadWrite(MicroWire_Mask);
    ReadWrite(MicroWire_Data);
#if defined(SSE_VAR_RESIZE_383)
    int i_dma_sound_volume=dma_sound_volume;
    int i_dma_sound_l_volume=dma_sound_l_volume;
    int i_dma_sound_r_volume=dma_sound_r_volume;
    int i_dma_sound_l_top_val=dma_sound_l_top_val;
    int i_dma_sound_r_top_val=dma_sound_r_top_val;
    int i_dma_sound_mixer=dma_sound_mixer;
    ReadWrite(i_dma_sound_volume);
    ReadWrite(i_dma_sound_l_volume);
    ReadWrite(i_dma_sound_r_volume);
    ReadWrite(i_dma_sound_l_top_val);
    ReadWrite(i_dma_sound_r_top_val);
    ReadWrite(i_dma_sound_mixer);
    dma_sound_volume=i_dma_sound_volume;
    dma_sound_l_volume=i_dma_sound_l_volume;
    dma_sound_r_volume=i_dma_sound_r_volume;
    dma_sound_l_top_val=i_dma_sound_l_top_val;
    dma_sound_r_top_val=i_dma_sound_r_top_val;
    dma_sound_mixer=i_dma_sound_mixer;
#else
    ReadWrite(dma_sound_volume);
    ReadWrite(dma_sound_l_volume);
    ReadWrite(dma_sound_r_volume);
    ReadWrite(dma_sound_l_top_val);
    ReadWrite(dma_sound_r_top_val);
    ReadWrite(dma_sound_mixer);
#endif
  }

  int NumFloppyDrives=num_connected_floppies;
  if (Version>=31){
    ReadWrite(NumFloppyDrives);
  }else{
    NumFloppyDrives=2;
  }

  bool spin_up=bool(fdc_spinning_up);
  if (Version>=32) ReadWrite(spin_up);
  if (Version>=33){
#if defined(SSE_VAR_RESIZE)
    {
      int fdc_spinning_up_tmp=fdc_spinning_up;
      ReadWrite(fdc_spinning_up_tmp);
      fdc_spinning_up=fdc_spinning_up_tmp;
    }
#else
    ReadWrite(fdc_spinning_up);
#endif
  }else if (LoadOrSave==LS_LOAD){
    fdc_spinning_up=spin_up;
  }

  if (Version>=34) ReadWrite(emudetect_write_logs_to_printer);

  if (Version>=35){
    ReadWrite(psg_reg_data);

#if defined(SSE_DMA_FIFO_READ_ADDRESS2)
    BYTE fdc_read_address_buffer_len=0;
#endif

#if defined(SSE_VAR_RESIZE)
    {
      int floppy_type1_command_active_tmp=floppy_type1_command_active;
      ReadWrite(floppy_type1_command_active_tmp);
      floppy_type1_command_active=floppy_type1_command_active_tmp;
      int fdc_read_address_buffer_len_tmp=fdc_read_address_buffer_len;
      ReadWrite(fdc_read_address_buffer_len_tmp);
      fdc_read_address_buffer_len=fdc_read_address_buffer_len_tmp;
    }
#else
    ReadWrite(floppy_type1_command_active);
    ReadWrite(fdc_read_address_buffer_len);
#endif


#if defined(SSE_DMA_FIFO_READ_ADDRESS2)
    {
      BYTE fdc_read_address_buffer_fake[20];
      ReadWriteArray(fdc_read_address_buffer_fake);
    }
#else
    ReadWriteArray(fdc_read_address_buffer);
#endif


#if defined(SSE_VAR_RESIZE)
    {
      int dma_bytes_written_for_sector_count_tmp=dma_bytes_written_for_sector_count;
      ReadWrite(dma_bytes_written_for_sector_count_tmp);
      dma_bytes_written_for_sector_count=dma_bytes_written_for_sector_count_tmp;
    }
#else
    ReadWrite(dma_bytes_written_for_sector_count);
#endif
  }

  if (Version>=36){
    struct AGENDA_STRUCT temp_agenda[MAX_AGENDA_LENGTH]; // SS removed _
    int temp_agenda_length=agenda_length;
    for (int i=0;i<agenda_length;i++) temp_agenda[i]=agenda[i];

    if (LoadOrSave==LS_SAVE){
      // Convert vectors to indexes and hbl_counts to relative
      for (int i=0;i<temp_agenda_length;i++){
        int l=0;
        while (DWORD(agenda_list[l])!=1){
          if (temp_agenda[i].perform==agenda_list[l]){
            temp_agenda[i].perform=(LPAGENDAPROC)l;
            break;
          }
          l++;
        }
        if (DWORD(agenda_list[l])==1) temp_agenda[i].perform=(LPAGENDAPROC)-1;
        temp_agenda[i].time-=hbl_count;
      }
    }
    ReadWrite(temp_agenda_length);
    for (int i=0;i<temp_agenda_length;i++){
      ReadWriteStruct(temp_agenda[i]);
    }
    if (LoadOrSave==LS_LOAD){
      int list_len=0;
      while (DWORD(agenda_list[++list_len])!=1);

      for (int i=0;i<temp_agenda_length;i++){
        int idx=int(temp_agenda[i].perform);
        if (idx>=list_len || idx<0){
          temp_agenda[i].perform=NULL;
        }else{
          temp_agenda[i].perform=agenda_list[idx];
        }
      }
      agenda_length=temp_agenda_length;
      for (int i=0;i<agenda_length;i++) agenda[i]=temp_agenda[i];
      agenda_next_time=0xffffffff;
      if (agenda_length) agenda_next_time=agenda[agenda_length-1].time;
    }
  }

  if (Version>=37){
#if defined(DISABLE_STEMDOS)
    int stemdos_intercept_datetime=0;
#endif
    ReadWrite(stemdos_intercept_datetime);
  }

  if (Version>=38){
    ReadWrite(emudetect_falcon_mode);
    ReadWrite(emudetect_falcon_mode_size);

    DWORD l=256;
    if (emudetect_called==0) l=0;
    ReadWrite(l);
#if defined(SSE_LOAD_SAVE_001)
    for (DWORD n=0;n<(BYTE)l;n++){
#else
    for (DWORD n=0;n<l;n++){
#endif
      ReadWrite(emudetect_falcon_stpal[n]);
    }
  }

  if (Version>=39){
    ReadWriteArray(dma_sound_internal_buf);
#if defined(SSE_VAR_RESIZE_383)
    int i_dma_sound_internal_buf_len=dma_sound_internal_buf_len;
    ReadWrite(i_dma_sound_internal_buf_len);
    dma_sound_internal_buf_len=i_dma_sound_internal_buf_len;
#else
    ReadWrite(dma_sound_internal_buf_len);
#endif
  }

  BYTE *pasti_block=NULL;
  DWORD pasti_block_len=0;

#if defined(SSE_VAR_REWRITE)
  bool pasti_old_active;
#else
  int pasti_old_active;
#endif
#if USE_PASTI
  pasti_old_active=pasti_active;
#else
  pasti_old_active=0;
#endif

  if (Version>=40){
#if USE_PASTI==0
    int pasti_active=0;
#endif
    ReadWrite(pasti_active);
#if USE_PASTI
    if (hPasti==0) pasti_active=0;
#endif
    if (LoadOrSave==LS_SAVE){
      //ask Pasti for variable block, save length as a long, followed by block
#if USE_PASTI
      if (hPasti && pasti_active){
        DWORD l=0;
        pastiSTATEINFO psi;
        psi.bufSize=0;
        psi.buffer=NULL;
        psi.cycles=ABSOLUTE_CPU_TIME;
        pasti->SaveState(&psi);
        l=psi.bufSize;
        BYTE*buf=new BYTE[l];
        psi.buffer=(void*)buf;
        if (pasti->SaveState(&psi)){
          //TRACE("Saving pasti state\n");
          ReadWriteVar(buf,l,f,LS_SAVE,1,Version);
        }else{
          l=0;
          ReadWrite(l);
        }
        delete []buf;
      }else
#endif
      {
        DWORD l=0;
        ReadWrite(l);
      }

    }else{ //load
      //read in length, read in block, pass it to pasti.
      ReadWrite(pasti_block_len);
      if (pasti_block_len){ //something to load in
#ifdef COMPILER_VC6// avoid bad crash
        if(pasti_block_len>0 && pasti_block_len<1024*1024)
        {
          pasti_block=new BYTE[pasti_block_len];
          ASSERT(pasti_block);
          fread(pasti_block,1,pasti_block_len,f);
#if USE_PASTI
          if (hPasti==NULL)
#endif
          {
            delete[] pasti_block;
            pasti_block=NULL;
          }
        }
#else
        fread(pasti_block,1,pasti_block_len,f);
#if USE_PASTI
        if (hPasti==NULL)
#endif
        {
          delete[] pasti_block;
          pasti_block=NULL;
        }
#endif//SS
      }
    }
  }else{
#if USE_PASTI
    pasti_active=0;
#endif
  }
#if defined(SSE_BUILD)
#if SSE_VERSION>=330
  if(Version>=41) // Steem 3.3
  {
#ifdef SSE_STF
    ReadWrite(ST_TYPE);
#if SSE_VERSION>=382
    if(LoadOrSave==LS_LOAD)
#endif
    SwitchSTType(ST_TYPE);
#endif
    int dummy=0; // dummy for former Program ID
    ReadWrite(dummy);
  }
#endif
#if SSE_VERSION>=340
  if(Version>=42) // Steem 3.4
  {
#if defined(SSE_SOUND_MICROWIRE)
    ReadWrite(SampleRate); // global of 3rd party
    if(SampleRate<6258 || SampleRate>50066)
      SampleRate=12517;
#if defined(SSE_VAR_RESIZE_383)
    int i_dma_sound_bass=dma_sound_bass;
    ReadWrite(i_dma_sound_bass);
    dma_sound_bass=dma_sound_bass;
#else
    ReadWrite(dma_sound_bass);
#endif
    if(dma_sound_bass<0 || dma_sound_bass>=0xC)
      dma_sound_bass=6;
#if defined(SSE_VAR_RESIZE_383)
    int i_dma_sound_treble=dma_sound_treble;
    ReadWrite(i_dma_sound_treble);
    dma_sound_treble=dma_sound_treble;
#else
    ReadWrite(dma_sound_treble);
#endif
    if(dma_sound_treble<0 || dma_sound_treble>=0xC)
      dma_sound_treble=6;
#endif

#if defined(SSE_IKBD_6301)
/*  If it must work with ReadWrite, we must use a variable that
    can be used with sizeof, so we take on the stack.
*/
#if !defined(SSE_IKBD_6301_DISABLE_CALLSTACK)
    BYTE buffer_for_hd6301[3260];
#else
    BYTE buffer_for_hd6301[200]; // spare the stack!
#endif
    if (LoadOrSave==LS_SAVE) // 1=save
    {
      if(HD6301_OK)
        hd6301_load_save(LoadOrSave,buffer_for_hd6301);
      ReadWrite(buffer_for_hd6301);  
    }
    else // 0=load
    {
      ReadWrite(buffer_for_hd6301);  
      if(HD6301_OK)
        hd6301_load_save(LoadOrSave,buffer_for_hd6301);
    }
#endif
  }
  else // default to safe values
  {
#if defined(SSE_IKBD_6301)
    OPTION_C1=0;
#endif
#if defined(SSE_STF)
    ST_TYPE=0;
#endif
  }
#endif
  //3.5.0: nothing special
#if SSE_VERSION>=351
  if(Version>=44) // Steem 3.5.1
  {
#if defined(SSE_SHIFTER)
    ReadWriteStruct(Shifter); // for res & sync
#endif

#if defined(SSE_IKBD_6301) // too bad it was forgotten before
    WORD HD6301EMU_ON_tmp=OPTION_C1;
    ReadWrite(HD6301EMU_ON_tmp); // but is it better?
    OPTION_C1=HD6301EMU_ON_tmp!=0;
    if(!HD6301_OK)
      OPTION_C1=0;
#endif
#if defined(SSE_ACIA_REGISTERS)
#if defined(SSE_VAR_RESIZE_383A)
  old_acia[NUM_ACIA_MIDI].clock_divide=ACIA_MIDI.clock_divide;
  old_acia[NUM_ACIA_MIDI].rx_irq_enabled=ACIA_MIDI.rx_irq_enabled;
  old_acia[NUM_ACIA_MIDI].rx_not_read=ACIA_MIDI.rx_not_read;
  old_acia[NUM_ACIA_MIDI].overrun=ACIA_MIDI.overrun;
  old_acia[NUM_ACIA_MIDI].tx_flag=ACIA_MIDI.tx_flag;
  old_acia[NUM_ACIA_MIDI].tx_irq_enabled=ACIA_MIDI.tx_irq_enabled;
  old_acia[NUM_ACIA_MIDI].data=ACIA_MIDI.data;
  old_acia[NUM_ACIA_MIDI].irq=ACIA_MIDI.irq;
  old_acia[NUM_ACIA_MIDI].last_tx_write_time=ACIA_MIDI.last_tx_write_time;
  old_acia[NUM_ACIA_MIDI].last_rx_read_time=ACIA_MIDI.last_rx_read_time;
#if defined(SSE_ACIA_DOUBLE_BUFFER_RX)
  old_acia[NUM_ACIA_MIDI].LineRxBusy=ACIA_MIDI.LineRxBusy;
  old_acia[NUM_ACIA_MIDI].ByteWaitingRx=ACIA_MIDI.ByteWaitingRx;
#endif
#if defined(SSE_ACIA_DOUBLE_BUFFER_TX)
  old_acia[NUM_ACIA_MIDI].ByteWaitingTx=ACIA_MIDI.ByteWaitingTx;
  old_acia[NUM_ACIA_MIDI].LineTxBusy=ACIA_MIDI.LineTxBusy;
#endif
#if defined(SSE_ACIA_REGISTERS)
  old_acia[NUM_ACIA_MIDI].CR=ACIA_MIDI.CR;
  old_acia[NUM_ACIA_MIDI].SR=ACIA_MIDI.SR;
  old_acia[NUM_ACIA_MIDI].RDR=ACIA_MIDI.RDR;
  old_acia[NUM_ACIA_MIDI].TDR=ACIA_MIDI.TDR;
  old_acia[NUM_ACIA_MIDI].RDRS=ACIA_MIDI.RDRS;
  old_acia[NUM_ACIA_MIDI].TDRS=ACIA_MIDI.TDRS;
#endif

  ReadWriteStruct(old_acia[NUM_ACIA_MIDI]);

  ACIA_MIDI.clock_divide=old_acia[NUM_ACIA_MIDI].clock_divide;
  ACIA_MIDI.rx_irq_enabled=old_acia[NUM_ACIA_MIDI].rx_irq_enabled;
  ACIA_MIDI.rx_not_read=old_acia[NUM_ACIA_MIDI].rx_not_read;
  ACIA_MIDI.overrun=old_acia[NUM_ACIA_MIDI].overrun;
  ACIA_MIDI.tx_flag=old_acia[NUM_ACIA_MIDI].tx_flag;
  ACIA_MIDI.tx_irq_enabled=old_acia[NUM_ACIA_MIDI].tx_irq_enabled;
  ACIA_MIDI.data=old_acia[NUM_ACIA_MIDI].data;
  ACIA_MIDI.irq=old_acia[NUM_ACIA_MIDI].irq;
  ACIA_MIDI.last_tx_write_time=old_acia[NUM_ACIA_MIDI].last_tx_write_time;
  ACIA_MIDI.last_rx_read_time=old_acia[NUM_ACIA_MIDI].last_rx_read_time;
#if defined(SSE_ACIA_DOUBLE_BUFFER_RX)
  ACIA_MIDI.LineRxBusy=old_acia[NUM_ACIA_MIDI].LineRxBusy;
  ACIA_MIDI.ByteWaitingRx=old_acia[NUM_ACIA_MIDI].ByteWaitingRx;
#endif
#if defined(SSE_ACIA_DOUBLE_BUFFER_TX)
  ACIA_MIDI.ByteWaitingTx=old_acia[NUM_ACIA_MIDI].ByteWaitingTx;
  ACIA_MIDI.LineTxBusy=old_acia[NUM_ACIA_MIDI].LineTxBusy;
#endif
#if defined(SSE_ACIA_REGISTERS)
  ACIA_MIDI.CR=old_acia[NUM_ACIA_MIDI].CR;
  ACIA_MIDI.SR=old_acia[NUM_ACIA_MIDI].SR;
  ACIA_MIDI.RDR=old_acia[NUM_ACIA_MIDI].RDR;
  ACIA_MIDI.TDR=old_acia[NUM_ACIA_MIDI].TDR;
  ACIA_MIDI.RDRS=old_acia[NUM_ACIA_MIDI].RDRS;
  ACIA_MIDI.TDRS=old_acia[NUM_ACIA_MIDI].TDRS;
#endif

#else
    ReadWriteStruct(ACIA_MIDI);
#endif

#endif
#if defined(SSE_DMA_OBJECT)
    ReadWriteStruct(Dma);
#endif
  }
#if defined(SSE_SHIFTER)//don't trust content of snapshot
#if defined(SSE_GLUE_383)
  Shifter.m_ShiftMode=(BYTE)screen_res;
  Glue.HiRes=(screen_res&2)!=0;
#else
  Glue.m_ShiftMode=Shifter.m_ShiftMode=(BYTE)screen_res;
#endif
  Glue.m_SyncMode= (BYTE)((shifter_freq==50)?2:0);
#endif
#endif

#if defined(SSE_VAR_CHECK_SNAPSHOT) //stupid!
  if(Version>=44)
  {
    int magic=123456;
    ReadWrite(magic);
#if !defined(SSE_VAR_CHECK_SNAPSHOT2)
    //ASSERT(magic==123456);
    if(magic!=123456)
    {
      Alert("Bad snapshot file has corrupted emulation \n\
Steem SSE will reset auto.sts and quit\nSorry!",
        T("Load Memory Snapshot Failed"),MB_ICONEXCLAMATION);      
      fclose(f);
      DeleteFile(WriteDir+SLASH+AutoSnapShotName+".sts");
      PostQuitMessage(-1);
    }
#endif
  }
#endif
#if SSE_VERSION>=352
  if(Version>=45) //3.5.2
  {
#if defined(SSE_DRIVE_OBJECT)

#if defined(SSE_DRIVE_SOUND)
    TSF314 SF314Copy=SF314[0];
#endif

#if SSE_VERSION>=370
/*  The line for drives was:
    ReadWriteStruct(SF314);
    But this wasn't correct as there are 2 drives.
    Now (v3.7.0) we R/W 2 drives. 
    v3.7.0 adds variables anyway, so snapshots wouldn't be correct. 
    TODO!!!
*/
    for(int drive=0;drive<2;drive++)
    {
      // same problem as for Pasti, we should find a shorter way to do that
      TImageType image_type=SF314[drive].ImageType;
      ReadWriteStruct(SF314[drive]);
      if(SF314[drive].Id!=drive) // and not ==... 
      {
        SF314[drive].Init();
        SF314[drive].Id=drive;
      }
      SF314[drive].ImageType=image_type;
    }
#endif

#if defined(SSE_DRIVE_SOUND) // avoid crash, restore volume
#if defined(SSE_DRIVE_SOUND_VOLUME)
    SF314[0].Sound_Volume=SF314Copy.Sound_Volume;
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
    SF314[1].Sound_Volume=SF314Copy.Sound_Volume;
#endif
#endif
    for(int i=0;i<TSF314::NSOUNDS;i++)
#if defined(SSE_DRIVE_SOUND_SINGLE_SET) // drive B uses sounds of A
      SF314[1].Sound_Buffer[i]=
#endif
      SF314[0].Sound_Buffer[i]=SF314Copy.Sound_Buffer[i];
#endif

#endif
  }
#endif
#if SSE_VERSION>=354
  if(Version>=46) // 3.5.4
  {
#if defined(SSE_MMU_WAKE_UP)
    ReadWrite(OPTION_WS); // and not struct MMU
#if defined(SSE_GUI_STATUS_STRING)
    GUIRefreshStatusBar();//overkill
#endif
#if defined(SSE_GLUE_THRESHOLDS)    
    Glue.Update();
#endif
#endif
  }
#endif
#if SSE_VERSION>=361
  if(Version>=48) // 3.6.1
  {
#if defined(SSE_DISK_CAPS_RESUME_____)//3.6.1
/*  This just restore registers, not internal state.
    Funny to see how the "drive" then finds back its track,
    in some cases it will work, in other fail.
    v3.6.2: do it only for load ...
    v3.6.3: check that Caps is loaded before ...
*/
    //TRACE("CAPSIMG_OK%d\n",CAPSIMG_OK);
    if(LoadOrSave==LS_LOAD //3.6.2
      && CAPSIMG_OK //3.6.3
      && Caps.Active
      ) 
    {
      Caps.WritePsgA(psg_reg[PSGR_PORT_A]);
      Caps.WriteWD1772(0,fdc_cr);
      Caps.WriteWD1772(1,fdc_tr);
      Caps.WriteWD1772(2,fdc_sr);
      Caps.WriteWD1772(3,fdc_dr);
    }
#endif
  }
  else
  {
  }
#endif
#if SSE_VERSION>=370
  if(Version>=49) // 3.7.0
  {

#if defined(SSE_DISK_CAPS_RESUME)
/*  This just restore registers, not internal state.
    Funny to see how the "drive" then finds back its track,
    in some cases it will work, in other fail.
    v3.6.2: do it only for load...
    v3.6.3: check that Caps is loaded...
    v3.7.0: check that Caps was active...
*/
    if(LoadOrSave==LS_LOAD //3.6.2
      && CAPSIMG_OK //3.6.3
//////      && Caps.Active //3.7.0
      ) 
    {
      Caps.WritePsgA(psg_reg[PSGR_PORT_A]);
      Caps.WriteWD1772(0,fdc_cr);
      Caps.WriteWD1772(1,fdc_tr);
      Caps.WriteWD1772(2,fdc_sr);
      Caps.WriteWD1772(3,fdc_dr);
    }
#endif

#if defined(SSE_YM2149A)
    {
#if defined(SSE_YM2149_DYNAMIC_TABLE)
    WORD *tmp=YM2149.p_fixed_vol_3voices; //always the same problem
#endif
    ReadWriteStruct(YM2149);
#if defined(SSE_YM2149_DYNAMIC_TABLE)
    YM2149.p_fixed_vol_3voices=tmp;
#endif
    }
#endif

#if defined(SSE_INT_MFP_OBJECT)
    //TRACE("L/S MFP\n");
    ReadWriteStruct(MC68901);
#if defined(SSE_INT_MFP)
    if(LoadOrSave==LS_LOAD)
#endif
      MC68901.Init(); // in case of bad snapshot
#endif

  }//3.7.0
#endif
#if SSE_VERSION>=371
  if(Version>=50) // 3.7.1, last minute
  {
#ifdef SSE_WD1772
    ReadWriteStruct(WD1772); //yep, was missing before... explains a lot!
    //TRACE("phase %d\n",WD1772.prg_phase);
    if(LoadOrSave==LS_LOAD && WD1772.prg_phase>TWD1772::WD_READY)
    {
      WD1772.update_time=ACT+256; // yeah, but it will work with some prgs
    }
#endif
#ifdef SSE_DRIVE_OBJECT
    SF314[0].time_of_next_ip=SF314[1].time_of_next_ip=ACT+n_cpu_cycles_per_second;
#endif
  }
#endif

#if SSE_VERSION>=372
  if(Version>=51) 
  {
  }
#endif

#if SSE_VERSION>=380
  if(Version>=52) 
  {
#if defined(SSE_GLUE_FRAME_TIMINGS_INIT)
    Glue.scanline=0; // Steem is always stopped at the start of a frame
#endif
#if defined(SSE_IKBD_6301_380) 
    BYTE HD6301_Initialised=HD6301.Initialised;
    ReadWriteStruct(HD6301); // registers... 
    HD6301.Initialised=HD6301_Initialised;
#endif
  }
#endif

  if(Version>=53) //382
  {
#if defined(SSE_TIMINGS_SNAPSHOT_CORRECTION)
/*  Steem starts/stops at the start of a frame. But the last instruction of
    the previous frame will be completed exactly at cycle 0 of the new frame
    only irregularly.
    When loading a snapshot, timing of next event (VBI trigger) should be
    adapted, see LoadSnapShotUpdateVars()
    We use ioaccess as temporary variable, it's a hack that spares a variable.
*/
    ioaccess=(Glue.screen_event.time-cpu_cycles);
    ReadWrite(ioaccess);
#if defined(SSE_CPU_E_CLOCK)
    M68000.cycles_for_eclock-=ioaccess;
#endif
#endif
#if defined(SSE_CPU_E_CLOCK) // keep E-Clock synced on loading a snapshot
    ReadWrite(M68000.cycles_for_eclock); 
#if defined(SSE_CPU_E_CLOCK_383)
    if(LoadOrSave==LS_SAVE)
      M68000.cycles_for_eclock+=ioaccess; // restore
#endif
#endif

  }
#if SSE_VERSION>=383
#if defined(SSE_TOS_SNAPSHOT_AUTOSELECT_383)
  int NewROMCountry=Tos.DefaultCountry;
#endif
  if(Version>=54) //383
  {
#if defined(SSE_TOS_SNAPSHOT_AUTOSELECT_383)
    if(LoadOrSave==LS_SAVE)
      NewROMCountry=ROM_PEEK(0x1D);
    ReadWrite(NewROMCountry);
#endif
  }
#endif

#endif//sse_build



  //

  // SAVE THIS -> emudetect_overscans_fixed

  //

  // End of data, seek to compressed memory
  if (Version>=11){
#ifndef ONEGAME
    if (LoadOrSave==LS_SAVE){
      StartOfData=ftell(f);
      fseek(f,StartOfDataPos,SEEK_SET);
      ReadWrite(StartOfData);
    }
    // Seek to start of compressed data (this was loaded earlier if LS_LOAD)
    fseek(f,StartOfData,SEEK_SET);
#else
    f=pStartByte+StartOfData;
#endif
  }

  if (LoadOrSave==LS_SAVE) return 0;

  init_screen();

#ifndef NO_CRAZY_MONITOR
  if (bool(extended_monitor)!=old_em || extended_monitor){
    if (FullScreen){
      change_fullscreen_display_mode(true);
    }else{
      Disp.ScreenChange(); // For extended monitor
    }
  }
#endif

#ifndef ONEGAME
  if (ChangeTOS){
#if defined(SSE_TOS_SNAPSHOT_AUTOSELECT_383)
    int ret=LoadSnapShotChangeTOS(NewROM,NewROMVer,NewROMCountry);
#else
    int ret=LoadSnapShotChangeTOS(NewROM,NewROMVer);
#endif
    //TRACE_INIT("LoadSnapShotChangeTOS %d\n",ret);
    if (ret>0) return ret;
  }

  if (ChangeDisks){
    int ret=LoadSnapShotChangeDisks(NewDisk,NewDiskInZip,NewDiskName);
    //TRACE_INIT("ChangeDisks %d\n",ret);
    if (ret>0) return ret;
  }

  if (ChangeCart){
    int ret=LoadSnapShotChangeCart(NewCart);
    //TRACE_INIT("ChangeCart %d\n",ret);
    if (ret>0) return ret;
  }

  LoadSaveChangeNumFloppies(NumFloppyDrives);
#endif

#if USE_PASTI
  if (hPasti && pasti_block){
    pastiSTATEINFO psi;
    psi.bufSize=pasti_block_len;
    psi.buffer=pasti_block;
    psi.cycles=ABSOLUTE_CPU_TIME;
    pasti->LoadState(&psi);
  }
  if (pasti_active!=pasti_old_active) LoadSavePastiActiveChange();
#endif

  return 0;
}
#undef ReadWrite
#undef ReadWritePtr
#undef ReadWriteStruct
//---------------------------------------------------------------------------
void LoadSnapShotUpdateVars(int Version)
{

  SET_PC(PC32);
#if !defined(SSE_TIMINGS_SNAPSHOT_CORRECTION)
  ioaccess=0;
#endif
  init_timings();

#if defined(SSE_TIMINGS_SNAPSHOT_CORRECTION)
  INSTRUCTION_TIME(ioaccess);
  ioaccess=0;
#endif

  UpdateSTKeys();

  if (Version<36){
    // No agendas saved
    if (ikbd.resetting) ikbd_reset(0);
    if (ikbd.mouse_mode==IKBD_MOUSE_MODE_OFF) ikbd.port_0_joy=true;

    if (keyboard_buffer_length) agenda_add(agenda_keyboard_replace,ACIAClockToHBLS(ACIA_IKBD.clock_divide)+1,0);
    if (MIDIPort.AreBytesToCome()) agenda_add(agenda_midi_replace,ACIAClockToHBLS(ACIA_MIDI.clock_divide,true)+1,0);
    if (floppy_irq_flag==FLOPPY_IRQ_YES || floppy_irq_flag==FLOPPY_IRQ_ONESEC){
      agenda_add(agenda_fdc_finished,MILLISECONDS_TO_HBLS(2),0);
    }
    if (fdc_spinning_up) agenda_add(agenda_fdc_spun_up,MILLISECONDS_TO_HBLS(40),fdc_spinning_up==2);
    if (ACIA_MIDI.tx_flag) agenda_add(agenda_acia_tx_delay_MIDI,2,0);
    if (ACIA_IKBD.tx_flag) agenda_add(agenda_acia_tx_delay_IKBD,2,0);
  }

#if USE_PASTI
  if (hPasti){
    pastiPEEKINFO ppi;
    pasti->Peek(&ppi);
    if (ppi.intrqState){
      mfp_reg[MFPR_GPIP]&=~(1 << MFP_GPIP_FDC_BIT);
    }else{
      mfp_reg[MFPR_GPIP]|=(1 << MFP_GPIP_FDC_BIT);
    }
    pasti_motor_proc(ppi.motorOn);
  }
#endif

  dma_sound_on_this_screen=1; // SS strange?
  dma_sound_output_countdown=0;
  dma_sound_samples_countdown=0;
  dma_sound_channel_buf_last_write_t=0;

  prepare_event_again();

  disable_input_vbl_count=0;

  snapshot_loaded=true;

  res_change();

  palette_convert_all();

  draw(false);

  for (int n=0;n<16;n++) PAL_DPEEK(n*2)=STpal[n];
}
//---------------------------------------------------------------------------

#undef LOGSECTION //SS
