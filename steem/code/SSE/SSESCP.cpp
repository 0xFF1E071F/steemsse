#include "SSE.h"

#if defined(STEVEN_SEAGAL)

#if defined(SSE_STRUCTURE_SSEFLOPPY_OBJ)
#include "../pch.h"
#include <cpu.decla.h>
#include <fdc.decla.h>
#include <floppy_drive.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include <run.decla.h>
#include "SSECpu.h"
#include "SSEInterrupt.h"
#include "SSEShifter.h"
#if defined(WIN32)
#include <pasti/pasti.h>
#endif
EasyStr GetEXEDir();//#include <mymisc.h>//missing...

#if defined(SSE_DRIVE_IPF1)
#include <stemdialogs.h>//temp...
#include <diskman.decla.h>
#endif

#if !defined(SSE_CPU)
#include <mfp.decla.h>
#endif

#endif//#if defined(SSE_STRUCTURE_SSEFLOPPY_OBJ)

#include "SSEDecla.h"
#include "SSEDebug.h"
#include "SSEFloppy.h"
#include "SSEOption.h"
#include "SSEScp.h"

#if defined(SSE_DISK_SCP)
/*  The SCP interface is based on the STW interface, so that integration in
    Steem (disk manager, FDC commands...) is straightforward.
    Remarkably few changes are necessary in the byte-level WD1772 emu to have
    some SCP images loading.
*/

#define LOGSECTION LOGSECTION_IMAGE_INFO

#if !defined(BIG_ENDIAN_PROCESSOR)
#include <acc.decla.h>
#define SWAP_WORD(val) *val=change_endian(*val);
#else
#define SWAP_WORD(val)
#endif


TImageSCP::TImageSCP() {
  Init();
}


TImageSCP::~TImageSCP() {
  Close();
}


void TImageSCP::Close() {
  if(fCurrentImage)
  {
    TRACE_LOG("SCP close image\n");
#if defined(SSE_DISK_SCP_WRITE)
    if(is_dirty)
      SaveTrack();
#endif    
    fclose(fCurrentImage);
    if(TimeFromIndexPulse)
      free(TimeFromIndexPulse);
  }
  Init();
}


void  TImageSCP::ComputePosition(WORD position) {
  if(!TimeFromIndexPulse)
    return; //safety

  // when we start reading/writing, where on the disk?
  position=position%nBytes; // 0-6256, safety
  Position=0; // safety

#if !defined(SSE_WD1772_DPLL) || defined(SSE_DISK_SCP_WRITE)
  ShiftsToNextOne=0; 
#endif
#if defined(SSE_WD1772_WEAK_BITS) && !defined(SSE_WD1772_DPLL)
  weak_bit_shift=0;
#endif

  // ignore "position", compute using IP timing and ACT //? TODO
  int cycles=time_of_next_event-SF314[DRIVE].time_of_last_ip;
  DWORD units=cycles*5;
  for(DWORD i=0;i<nBits;i++)
  {
    if( TimeFromIndexPulse[i]>=units)
    {
      Position=i;
      break;
    }
  }

#if defined(SSE_WD1772_DPLL)
  WD1772.Dpll.Reset(ACT); 
#endif


  Disk[DRIVE].current_byte=(ACT-SF314[DRIVE].time_of_last_ip)/SF314[DRIVE].CyclesPerByte();
//TRACE("%d %d %d\n",ACT,SF314[DRIVE].time_of_last_ip,Disk[DRIVE].TrackBytes);

  TRACE_LOG("Compute new position IP %d ACT %d cycles in %d units %d Position %d units %d byte %d\n",
    SF314[DRIVE].time_of_last_ip,ACT,ACT-SF314[DRIVE].time_of_last_ip,units,Position,TimeFromIndexPulse[Position],Disk[DRIVE].current_byte);
}

#if !defined(SSE_WD1772_PRECISE_SYNC) || defined(SSE_DISK_SCP_TO_MFM_PREVIEW)

BYTE TImageSCP::GetDelay(int position) {
  // we want delay in ms, typically 4, 6, 8
  WORD units_to_next_flux=UnitsToNextFlux(position);
  BYTE delay_in_us;
  delay_in_us=UsToNextFlux(units_to_next_flux);
  return delay_in_us;    
}

#endif

int TImageSCP::UnitsToNextFlux(int position) {
  // 1 unit = 25 nanoseconds = 1/40 ms
  ASSERT(position<nBits);
  ASSERT(position>=0);
  position=position%nBits; // safety
  DWORD time1=0,time2;
  if(position)
    time1=TimeFromIndexPulse[position-1];
  time2=TimeFromIndexPulse[position];
  ASSERT( time2>time1 );
  int units_to_next_flux=time2-time1; 
  return units_to_next_flux;    
}


int TImageSCP::UsToNextFlux(int units_to_next_flux) {
  BYTE us_to_next_flux;
  BYTE ref_us= ((units_to_next_flux/40)+1)&0xFE;  // eg 4
  WORD ref_units = ref_us * 40;
  if(units_to_next_flux<ref_units-SCP_DATA_WINDOW_TOLERANCY)
    us_to_next_flux=ref_us-1;
  else if (units_to_next_flux>ref_units+SCP_DATA_WINDOW_TOLERANCY)
    us_to_next_flux=ref_us+1;
  else
    us_to_next_flux=ref_us;
  return us_to_next_flux;    
}


WORD TImageSCP::GetMfmData(WORD position) {
/*  We use the same interface for SCP as for STW so that integration
    with the Disk manager, WD1772 emu etc. is straightforward.
    But precise emulation doesn't send MFM data word by word (16bit).
    Instead it sends bytes and AM signals according to bit sequences,
    as analysed in (3rd party-inspired) WD1772.ShiftBit().
*/

  WORD mfm_data=0;
#if !defined(SSE_WD1772_PRECISE_SYNC)
  WD1772.Amd.AMFound=false; // data can be shifted (not $A1)
#endif
  if(!TimeFromIndexPulse) //safety, SCP track in ram?
    return mfm_data;
  // must compute new starting point?
  if(position!=0xFFFF)
    ComputePosition(position);

#if defined(SSE_WD1772_DPLL)
  // we manage timing here, maybe we should do that in WD1772 instead
  int a1=WD1772.Dpll.ctime,a2,tm=0;
#else
  int starting_delay=TimeFromIndexPulse[Position];
#endif

#if defined(SSE_WD1772_PRECISE_SYNC)
  // clear dsr signals
  WD1772.Amd.aminfo&=~(CAPSFDC_AI_DSRREADY|CAPSFDC_AI_DSRAM|CAPSFDC_AI_DSRMA1);
  // loop until break
  for(int i=0; ;i++) 
#else
  for(int i=0;i<16;i++) // bits of our MFM word
#endif
  {
#if !defined(SSE_WD1772_PRECISE_SYNC)
    mfm_data<<=1; 
#endif

#if defined(SSE_WD1772_DPLL)
    int bit=WD1772.Dpll.GetNextBit(tm,DRIVE); //tm isn't used...
    ASSERT(bit==0 || bit==1); // 0 or 1, clock and data
    TRACE_MFM("%d",bit); // full flow of bits

#if defined(SSE_WD1772_PRECISE_SYNC)
    if(WD1772.ShiftBit(bit)) // true if byte ready to transfer
      break;
#else
    mfm_data|=bit;
    WD1772.Amd.AMWindow|=bit;
#endif

#else //!DPLL (code not compiled)
/*  This was the primitive system before we could use a correct
    DPLL algorithm. It already could load simple or not so simple
    disk images, but not dirty data (phase errors...).
*/
    for(int j=0;!ShiftsToNextOne&&j<255;j++) // j as safety
    {
      BYTE delay_in_us=GetDelay(Position);
#if defined(SSE_BOILER_TRACE_CONTROL)
      if(TRACE_MASK3&TRACE_CONTROL_FDCMFM)
        TRACE_FDC("%d (%d) ",delay_in_us,units_to_next_flux);
#endif
#if defined(SSE_WD1772_WEAK_BITS) && !defined(SSE_WD1772_DPLL)
/*  Fuzzy bits reckoning tailored for Dungeon Master sector 0-7, bytes 21-...
    Should be random, out of those 2 values only:
      $68 01101000 MFM 944A 1001010001001010 
      $E8 11101000 MFM 544A 0101010001001010 
    With other values, the 1st gate won't even open for your party.
    This was tuned with an image that could very well be incorrect!
*/
      if(delay_in_us&1) // 5 in DM
      {
        // By pairs, 5+5 = 4+6 or 6+4, not 4+4 or 6+6
        if(weak_bit_shift) 
        {
          delay_in_us-=weak_bit_shift; // 6+4 or 4+6
          weak_bit_shift=0;
        }
        else if( GetDelay(Position+1) & 1) // next is weak too?
        {
          weak_bit_shift=(rand()&2)-1; // -1 or +1
          delay_in_us+=weak_bit_shift;
        }
        else // if not assume +1 (hack, for DM) TODO
        {
          weak_bit_shift=1;
          delay_in_us+=weak_bit_shift;
        }
#if defined(SSE_BOILER_TRACE_CONTROL)
        if(TRACE_MASK3&TRACE_CONTROL_FDCMFM)
          TRACE_FDC("(%d) ",delay_in_us);
#endif
      }
      else 
        weak_bit_shift=0;
#endif
      ShiftsToNextOne=delay_in_us/2;
      IncPosition();
    }//nxt j
    ASSERT(ShiftsToNextOne);
    ShiftsToNextOne--;
    if(!ShiftsToNextOne) // set bit in MFM word and in address mark detector
    {
      mfm_data|=1;
      WD1772.Amd.AMWindow|=1;
    }
#endif

#if !defined(SSE_WD1772_PRECISE_SYNC) //broken sync reckoning
    if(WD1772.Amd.Enabled && 
      (WD1772.Amd.AMWindow==0x4489 || WD1772.Amd.AMWindow==0x5224))
    {
      TRACE_MFM(" AM %X bit %d ",WD1772.Amd.AMWindow,i);
      WD1772.Amd.AMFound=WD1772.Amd.AMWindow;
      if(WD1772.Amd.AMWindow==0x4489) // sync on $A1
      {
        //WD1772.Amd.AMWindow=0; // no overlap
        mfm_data<<=15-i;
        i=16; //sync
      }
      else if(WD1772.Amd.AMWindow==0x5224) 
      {
        // no sync... fails on some disks
        TRACE_MFM("(%d %d %d %d %d) ",GetDelay(Position-2),GetDelay(Position-1),GetDelay(Position),GetDelay(Position+1),GetDelay(Position+2));
      }
      TRACE_MFM(" bit %d ",i);
    }
    WD1772.Amd.AMWindow<<=1;
#endif
  }//nxt i

  //WD1772.Mfm.data_last_bit=(mfm_data&1); // no use

#if defined(SSE_WD1772_DPLL)
#ifdef SSE_DEBUG  // only report if there's adjustment
  if(WD1772.Dpll.increment!=128|| WD1772.Dpll.phase_add||WD1772.Dpll.phase_sub
    ||WD1772.Dpll.freq_add||WD1772.Dpll.freq_sub)
  {
    ASSERT( !(WD1772.Dpll.freq_add && WD1772.Dpll.freq_sub) ); 
    ASSERT( !(WD1772.Dpll.phase_add && WD1772.Dpll.phase_sub) );
    TRACE_MFM(" DPLL (%d,%d,%d) ",WD1772.Dpll.increment,WD1772.Dpll.phase_add-WD1772.Dpll.phase_sub,WD1772.Dpll.freq_add-WD1772.Dpll.freq_sub);
  }
#endif
  a2=WD1772.Dpll.ctime;
  int delay_in_cycles=(a2-a1);
  ASSERT(delay_in_cycles>0);
  TRACE_MFM(" %d cycles\n",delay_in_cycles);
#else // !DPLL
  // set up timing of DRQ
  // we don't count shifts between transitions, so it's not cycle-accurate
  int units_to_next_flux=
    TimeFromIndexPulse[ (Position) ? (Position-1) : (nBits-1) ] - starting_delay;
  int delay_in_cycles=units_to_next_flux/5;
#endif

  WD1772.update_time=time_of_next_event+delay_in_cycles; 
  if(WD1772.update_time-ACT<=0) // safety
  {
    TRACE_LOG("Argh! wrong disk timing %d ACT %d diff %d last IP %d pos %d/%d delay %d units %d\n",
      WD1772.update_time,ACT,ACT-WD1772.update_time,SF314[DRIVE].time_of_last_ip,Position,nBits-1,delay_in_cycles,TimeFromIndexPulse[Position-1]);
    WD1772.update_time=ACT+SF314[DRIVE].cycles_per_byte;
  }

#if defined(SSE_BOILER_TRACE_CONTROL) && !defined(SSE_WD1772_PRECISE_SYNC)
  if(TRACE_MASK3&TRACE_CONTROL_FDCMFM)
  {
    WD1772.Mfm.encoded=mfm_data;
    WD1772.Mfm.Decode();
    TRACE_MFM("MFM %X (C %X D %X) amd %X\n",mfm_data,WD1772.Mfm.clock,WD1772.Mfm.data,WD1772.Amd.AMWindow);
  }
#endif
#if defined(SSE_WD1772_PRECISE_SYNC)
  ASSERT(!mfm_data); // see note at top of function
#endif
  return mfm_data;
}


#if defined(SSE_WD1772_DPLL)
#ifdef SSE_WD1772_WEAK_BITS2 
int TImageSCP::GetNextTransition(BYTE& us_to_next_flux) {
  int t=UnitsToNextFlux(Position);
  us_to_next_flux=UsToNextFlux(t); // in parameter
  IncPosition();
  t/=5; // in cycles
  return t; 
}
#else

int TImageSCP::GetNextTransition() {

  int t=UnitsToNextFlux(Position);

  IncPosition();
  // so we have the delays in microseconds and the bits, eg (6)001:
  TRACE_MFM("(%d)",UsToNextFlux(t)); 
  t/=5; // in cycles
  return t; 
}
#endif

#endif


void TImageSCP::IncPosition() {
  ASSERT( Position>=0 );
  ASSERT( Position<nBits );
  Position++;
  if(Position==nBits)
  {
    Position=0;
    TRACE_FDC("\nSCP triggers IP side %d track %d rev %d/%d\n",
      CURRENT_SIDE,floppy_head_track[DRIVE],rev+1,file_header.IFF_NUMREVS);
    
#if defined(SSE_DRIVE_INDEX_PULSE3)
/*  If a sector is spread over IP, we make sure that our event
    system won't start a new byte before returning to current
    byte. 
*/
    SF314[DRIVE].IndexPulse(true);
#else
    SF314[DRIVE].IndexPulse();
#endif

#if defined(SSE_DRIVE_INDEX_PULSE3) && defined(SSE_DISK_SCP_START_REV1)
    // We step revs only if there's reading over the IP
    if(file_header.IFF_NUMREVS>1)
    {
      switch(WD1772.prg_phase)
      {
      case TWD1772::WD_TYPEI_READ_ID:
      case TWD1772::WD_TYPEII_READ_ID:
      case TWD1772::WD_TYPEIII_READ_ID:
      case TWD1772::WD_TYPEII_READ_DATA: 
      case TWD1772::WD_TYPEII_READ_CRC:
        LoadTrack(CURRENT_SIDE,SF314[DRIVE].Track(),true);
      }
    }      
#endif
  }
}


void TImageSCP::Init() {
  fCurrentImage=NULL;
  TimeFromIndexPulse=NULL;
  nSides=2;
  nTracks=83; //max
  nBytes=DRIVE_BYTES_ROTATION_STW; //not really pertinent (TODO?)
}


#if defined(SSE_BOILER) && defined(SSE_DISK_SCP_TO_MFM_PREVIEW)
/*  This function was used for development of a flux to MFM decoder.
    It transforms raw flux reversal delays into MFM, all at once, and
    "syncs" when it detects $A1 address marks.
    It is commanded by 'log image info' + 'mfm' in the control mask browser.
    It computes the # data bytes on the track, it's generally more than 6250,
    but that could be overrated.
    It still helped me probe Turrican.
*/

void TImageSCP::InterpretFlux() {
  WORD current_mfm_word=0,amd=0,ndatabytes=0;
  int bit_counter=0;
  for(int i=0;i<nBits;i++)
  {
    BYTE delay_in_us=GetDelay(i);
#if defined(SSE_BOILER_TRACE_CONTROL)
    if(TRACE_MASK3&TRACE_CONTROL_FDCMFM)
      TRACE_LOG("%d ",delay_in_us);
#endif
    int n_shifts=(delay_in_us/2); // 4->2; 6->3; 8->4
    for(int j=0;j<n_shifts;j++)
    {
      if(j==n_shifts-1) // eg 001 3rd iteration we set bit
      {
        current_mfm_word|=1;
        amd|=1;
      }
      if(amd==0x4489 || amd==0x5224) 
      {
#if defined(SSE_BOILER_TRACE_CONTROL)
        if(TRACE_MASK3&TRACE_CONTROL_FDCMFM)
          TRACE_LOG("AM? %x ctr %d\n",amd,bit_counter);
#endif
        if(amd==0x4489)
        {
          amd=0; // no overlap
          current_mfm_word<<=15-bit_counter;
          bit_counter=15; //sync
        }
      }
      amd<<=1;
      bit_counter++;
      if(bit_counter==16)
      {
        WD1772.Mfm.encoded=current_mfm_word;
        WD1772.Mfm.Decode();
#if defined(SSE_BOILER_TRACE_CONTROL)
        if(TRACE_MASK3&TRACE_CONTROL_FDCMFM)
          TRACE_LOG("MFM %X amd %X C %X D %X \n",current_mfm_word,amd,WD1772.Mfm.clock,WD1772.Mfm.data);
#endif
        current_mfm_word=0;
        bit_counter=0;
        ndatabytes++;
      }
      current_mfm_word<<=1;
    }//j
  }//i
  TRACE_LOG("%d bytes on SCP track\n",ndatabytes);
}

#endif


bool TImageSCP::LoadTrack(BYTE side,BYTE track
#if defined(SSE_DISK_SCP_START_REV1)
                          ,bool reload
#endif
                          ) {
  bool ok=false;

  ASSERT( side<2 && track<nTracks ); // unique side may be 1
  if(side>=2 || track>=nTracks)
    return ok; //no crash

#if defined(SSE_DISK_SCP_WRITE)
  if(is_dirty)
    SaveTrack();
#endif  

  BYTE trackn=track;
  if(nSides==2) // general case
    trackn=track*2+side; 

#if defined(SSE_DISK_SCP4) //dev switch
    if(! reload && track_header.TDH_TRACKNUM==trackn) //already loaded
      return true;
#endif

  if(TimeFromIndexPulse) 
    free(TimeFromIndexPulse);
  TimeFromIndexPulse=NULL;

  int offset= file_header.IFF_THDOFFSET[trackn]; // base = start of file

  if(fCurrentImage) // image exists
  {  
    fseek(fCurrentImage,offset,SEEK_SET);
    int size=sizeof(TSCP_track_header);
//      -( (5-file_header.IFF_NUMREVS)*sizeof(TSCP_TDH_TABLESTART));
    fread(&track_header,size,1,fCurrentImage);

#if defined(SSE_DISK_SCP_START_REV1) 
/*  Determine which track rev to load.
    Turrican SCP will fail if we don't start on rev1 so that it reads
    sector data of rev2 over IP (index pulse).
    So we make it a generality (the way it is coded, normally it will
    also do it with rev 3, 4... in the unlikely case that it's needed).
    This is because IP is by nature imprecise, and it won't fall on the same
    bitcell each time (unless you have mastering hardware, not mama's drive).
    One rev from IP to IP, unless "doctored", may have one or two bits too
    few or too many at the beginning or the end.
    Doing rev1 -> rev2 ensures that we use the exact same timing for IP,
    and no bit is lost or gained.
    It will work with 1rev versions if the copy is 100% (not our 
    responsibility) or if data at IP isn't important (most cases).
*/
    if(reload)
      rev++;
    else
      rev=0;
    rev%=file_header.IFF_NUMREVS;
#else
#if defined(SSE_DRIVE_INDEX_PULSE2) 
    rev=SF314[DRIVE].nRevs%file_header.IFF_NUMREVS;
#else
    rev=0;
#endif
#endif

    WORD* flux_to_flux_units_table_16bit=(WORD*)calloc(track_header.\
      TDH_TABLESTART[rev].TDH_LENGTH,sizeof(WORD));
    TimeFromIndexPulse=(DWORD*)calloc(track_header.TDH_TABLESTART[rev].\
      TDH_LENGTH,sizeof(DWORD));
    ASSERT(flux_to_flux_units_table_16bit && TimeFromIndexPulse);

    if(flux_to_flux_units_table_16bit && TimeFromIndexPulse)
    {
      fseek(fCurrentImage,offset+track_header.TDH_TABLESTART[rev].TDH_OFFSET,
        SEEK_SET);

      fread(flux_to_flux_units_table_16bit,sizeof(WORD),track_header.\
        TDH_TABLESTART[rev].TDH_LENGTH,fCurrentImage);

      int units_from_ip=0;
      nBits=0;
      // reverse endianess and convert to time after IP, one data per bit
      for(DWORD i=0;i<track_header.TDH_TABLESTART[rev].TDH_LENGTH;i++)
      {
        SWAP_WORD( &flux_to_flux_units_table_16bit[i] );
        ASSERT(units_from_ip + flux_to_flux_units_table_16bit[i] >= units_from_ip);
        units_from_ip+=(flux_to_flux_units_table_16bit[i])
          ? flux_to_flux_units_table_16bit[i] : 0xFFFF;
        ASSERT(units_from_ip<0x7FFFFFFF); // max +- 200,000,000, OK
        if(flux_to_flux_units_table_16bit[i])
          TimeFromIndexPulse[nBits++]=units_from_ip;
      }
      Position=0;
      free(flux_to_flux_units_table_16bit);
      ok=true;
#if defined(SSE_DISK_SCP_WRITE)
      is_dirty=false;
#endif
    }

    TRACE_LOG("SCP LoadTrack side %d track %d %c%c%c %d rev %d/%d  \
INDEX TIME %d (%f ms) TRACK LENGTH %d bits %d last bit unit %d DATA \
OFFSET %d  checksum %X\n",
side,track,
track_header.TDH_ID[0],track_header.TDH_ID[1],track_header.TDH_ID[2],
track_header.TDH_TRACKNUM,rev+1,file_header.IFF_NUMREVS,
track_header.TDH_TABLESTART[rev].TDH_DURATION,
(float)track_header.TDH_TABLESTART[rev].TDH_DURATION*25/1000000,
track_header.TDH_TABLESTART[rev].TDH_LENGTH, nBits,TimeFromIndexPulse[nBits-1],
track_header.TDH_TABLESTART[rev].TDH_OFFSET,track_header.track_data_checksum);

#if defined(SSE_BOILER) && defined(SSE_DISK_SCP_TO_MFM_PREVIEW)
    InterpretFlux();
#endif
  }

  return ok;
}


bool TImageSCP::Open(char *path) {

#ifdef SSE_WD1772_MFM_PRODUCE_TABLE // one-shot switch...
  // todo, also in bits
 for(int i=0;i<256;i++)
 {
   WD1772.Mfm.data=i;
   WD1772.Mfm.Encode();
   TRACE("D %02X -> C %02X MFM %04X\n",i,WD1772.Mfm.clock,WD1772.Mfm.encoded);
 }
#endif

  bool ok=false;
  Close(); // make sure previous image is correctly closed

  fCurrentImage=fopen(path,"rb+"); // try to open existing file

#if defined(SSE_DISK_SCP3)
  if(!fCurrentImage) // maybe it's read-only
    fCurrentImage=fopen(path,"rb");
#endif

  if(fCurrentImage) // image exists
  {
    // we read only the header
    if(fread(&file_header,sizeof(TSCP_file_header),1,fCurrentImage))
    {
      if(!strncmp(DISK_EXT_SCP,(char*)&file_header.IFF_ID,3)) // it's SCP
      {

        // compute nSides and nTracks
        if(file_header.IFF_HEADS)
        {
          nSides=1;
          nTracks=file_header.IFF_END-file_header.IFF_START+1;
        }
        else
          nTracks=(file_header.IFF_END-file_header.IFF_START+1)/2;

TRACE_LOG("SCP sides %d tracks %d IFF_VER %X IFF_DISKTYPE %X IFF_NUMREVS %d \
IFF_START %d IFF_END %d IFF_FLAGS %d IFF_ENCODING %d IFF_HEADS %d \
IFF_RSRVED %X IFF_CHECKSUM %X\n",nSides,nTracks,file_header.IFF_VER,
file_header.IFF_DISKTYPE,file_header.IFF_NUMREVS,file_header.IFF_START,
file_header.IFF_END,file_header.IFF_FLAGS,file_header.IFF_ENCODING,
file_header.IFF_HEADS,file_header.IFF_RSRVED,file_header.IFF_CHECKSUM);

#if defined(SSE_DISK_SCP4) //dev switch
        track_header.TDH_TRACKNUM=0xFF;
#endif

        ok=true; //TODO some checks?
      }//cmp
    }//read
  }

  if(!ok)
    Close();

  return ok;
}

#if defined(SSE_DISK_SCP_WRITE)

bool  TImageSCP::SaveTrack() {
/*  We need to convert back to big endian relative delays - not tested
*/
  bool ok=false;
  BYTE trackn=track_header.TDH_TRACKNUM;
  ASSERT(file_header.IFF_NUMREVS==1);
  ASSERT(is_dirty);
  if(TimeFromIndexPulse && fCurrentImage)
  {
    int offset= file_header.IFF_THDOFFSET[trackn]; // base = start of file

    WORD* flux_to_flux_units_table_16bit=(WORD*)calloc(track_header.\
      TDH_TABLESTART[rev].TDH_LENGTH,sizeof(WORD));

    if(flux_to_flux_units_table_16bit && TimeFromIndexPulse)
    {

      int total_units,previous_total_units=0;
      ASSERT( nBits>0 && nBits<=track_header.TDH_TABLESTART[rev].TDH_LENGTH);
      int k=0;
      for(DWORD i=0;i<nBits;i++)
      {
        total_units=TimeFromIndexPulse[i];
        int flux_to_flux_units_32bit=total_units-previous_total_units;        
        while( flux_to_flux_units_32bit>0xFFFF)
        {
          flux_to_flux_units_table_16bit[k++]=0;
          flux_to_flux_units_32bit-=0xFFFF;
        }
        flux_to_flux_units_table_16bit[k]=flux_to_flux_units_32bit;
        SWAP_WORD( &flux_to_flux_units_table_16bit[k] );
        k++;
        previous_total_units=total_units;      
      }//nxt i
      ASSERT( k == track_header.TDH_TABLESTART[rev].TDH_LENGTH-1 );

      fseek(fCurrentImage,offset+track_header.TDH_TABLESTART[rev].TDH_OFFSET,
        SEEK_SET);

      fwrite(flux_to_flux_units_table_16bit,sizeof(WORD),track_header.\
        TDH_TABLESTART[rev].TDH_LENGTH,fCurrentImage);

      ok=true;
     
      free(flux_to_flux_units_table_16bit);
    }
  }
  TRACE_LOG("SCP save track %d OK %d\n",trackn,ok);
  return ok;
}

#endif


void TImageSCP::SetMfmData(WORD position, WORD mfm_data) {

#if defined(SSE_DISK_SCP_WRITE) // not tested...
  if(file_header.IFF_NUMREVS==1 && TimeFromIndexPulse
    && !FloppyDrive[DRIVE].ReadOnly)
  {
    int starting_delay=TimeFromIndexPulse[Position];
    is_dirty=true;
    // must compute new starting point?
    if(position!=0xFFFF)
      ComputePosition(position);
    for(int i=0;i<16;i++)
    {
      ShiftsToNextOne++;
      bool bit=(mfm_data&1);
      if(bit)
      {
        TimeFromIndexPulse[Position++]= (ShiftsToNextOne*2)*40; // perfect sync
        ShiftsToNextOne=0;
      }
      mfm_data>>=1;
    }//nxt

    // set up timing of DRQ
    // we don't count shifts between transitions, so it's not cycle-accurate
    int units_to_next_flux= UnitsToNextFlux(Position);
    int cycles_to_next_flux=units_to_next_flux/5;
    WD1772.update_time=time_of_next_event+cycles_to_next_flux; 
    if(WD1772.update_time-ACT<0) // safety
    {
      TRACE_LOG("Argh! wrong disk timing %d ACT %d diff %d last IP %d pos %d shifts %d units %d cycles %d\n",
        WD1772.update_time,ACT,ACT-WD1772.update_time,SF314[DRIVE].time_of_last_ip,Position,ShiftsToNextOne,TimeFromIndexPulse[Position-1],cycles_to_next_flux);
      WD1772.update_time=ACT+SF314[DRIVE].cycles_per_byte;
    }
  }
#endif
}

#undef LOGSECTION

#endif//SCP

#endif//#if defined(STEVEN_SEAGAL)
