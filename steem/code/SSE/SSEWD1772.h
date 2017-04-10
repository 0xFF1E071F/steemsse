#pragma once
#ifndef WD1772_H
#define WD1772_H
#include "SSE.H"
#include <conditions.h>
#include <fdc.decla.h>
#include "SSEParameters.h"


/*
This is from the WD1772 doc:
Byte #     Meaning                |     Sector length code     Sector length
------     ------------------     |     ------------------     -------------
1          Track                  |     0                      128
2          Side                   |     1                      256
3          Sector                 |     2                      512
4          Sector length code     |     3                      1024
5          CRC byte 1             |
6          CRC byte 2             |

The structure is used by SSEWD1772.cpp, but also by SSEGhostDisk.cpp.

*/

#pragma pack(push, STRUCTURE_ALIGNMENT)//TODO

#if defined(SSE_WD1772_IDFIELD)

struct TWD1772IDField {
  // DATA
  BYTE track;
  BYTE side;
  BYTE num;
  BYTE len;
  BYTE CRC[2]; // not a WORD because ST was big-endian
  // FUCNTIONS
  inline TWD1772IDField() {
#ifdef SSE_UNIX
    memset(this,0,sizeof(TWD1772IDField));
#else
    ZeroMemory(this,sizeof(TWD1772IDField));
#endif
  }
  inline WORD nBytes(){
    return (1<<( (len&3) +7)); // other way: harder
  }
//#ifdef SSE_DEBUG
  void Trace(); // empty body if not debug
//#endif  
};

#endif

#if defined(SSE_WD1772_MFM)

struct TWD1772MFM {
  enum {NORMAL_CLOCK,FORMAT_CLOCK};
  WORD encoded;
  BYTE clock,data; 
  unsigned int data_last_bit:1;
  void Decode ();
  void Encode(int mode=NORMAL_CLOCK);
};

#endif

#if defined(SSE_WD1772_CRC)

struct TWD1772Crc {
  DWORD crccnt;
  WORD crc;
/*  Contrary to what the doc states, the CRC Register isn't preset to ones 
    ($FFFF) prior to data being shifted through the circuit, but to $CDB4.
    This happens for each $A1 address mark (read or written), so the register
    value after $A1 is the same no matter how many address marks.
    When formatting the backup disk, Dragonflight writes a single $F5 (->$A1)
    in its custom track headers and expects to read value $CDB4.
*/
  inline void Reset() {
    crc=0xCDB4;
  }
  inline void Add(BYTE data) {
    fdc_add_to_crc(crc,data); // we just call Steem's original function for now
  }
  bool Check(TWD1772IDField *IDField);
};

#endif

#if defined(SSE_WD1772_BIT_LEVEL)

struct TWD1772Dpll {

  int GetNextBit(int &tm, BYTE drive);
  void Reset(int when);
  void SetClock(const int &period);
  int latest_transition;
  int ctime;
  int delays[42];
  int write_start_time;
  int write_buffer[32];
  int write_position;
  WORD counter;
  WORD increment;
  WORD transition_time;
  BYTE history;
  BYTE slot;
  BYTE phase_add, phase_sub, freq_add, freq_sub;

};

#endif//bit-level

struct TWD1772AmDetector {
#if defined(SSE_WD1772_BIT_LEVEL)
  DWORD amdecode,aminfo,amisigmask;
  // we keep those here because it's 32bit and integrated in the logic:
  int dsr,dsrcnt; 
  BYTE amdatadelay,ammarkdist,ammarktype,amdataskip;
#endif
  BYTE nA1; // counter
  void Enable();
  bool Enabled;
  void Reset();
};


struct TWD1772 {

  // ENUM
#if defined(SSE_WD1772_EMU)
/*  Status bits.
    We define our own because Steem's are a bit long.
    We also define command flags of the docs, and some
    masks to better ID commands.
*/
  enum EWd1772SatusBits { STR_MO=BIT_7,STR_WP=BIT_6,STR_RT=BIT_5,STR_SU=BIT_5,
    STR_RNF=BIT_4,STR_SE=BIT_4,STR_CRC=BIT_3,STR_LD=BIT_2,
    STR_T00=BIT_2,STR_DRQ=BIT_1,STR_IP=BIT_1,STR_BUSY=BIT_0
  };

  enum { CR_U=BIT_4,CR_M=BIT_4,CR_H=BIT_3,CR_V=BIT_2,CR_E=BIT_2,
    CR_R1=BIT_1,CR_P=BIT_1,CR_R0=BIT_0,CR_A0=BIT_0,
    CR_I3=BIT_3,CR_I2=BIT_2,CR_I1=BIT_1,CR_I0=BIT_0,
    CR_STEP=(BIT_6|BIT_5),CR_STEP_IN=BIT_6,CR_STEP_OUT=CR_STEP,
    CR_SEEK=BIT_4,CR_RESTORE=0,
    CR_TYPEII_WRITE=BIT_5,CR_TYPEIII_WRITE=BIT_4,CR_TYPEIII_READ_ADDRESS=0xC0
  };

/*  We identify various phases in the Western Digital flow charts, so that we
    know where to "jump" next.
    We have two delay sources: current algorithm processing and index pulses.
    This could correspond to two different programs running in parallel. (?)
*/

  enum EWd1772Phase {
    WD_READY,
    WD_TYPEI_SPINUP,
    WD_TYPEI_SPUNUP, // spunup must be right after spinup
    WD_TYPEI_SEEK,
    WD_TYPEI_STEP_UPDATE,
    WD_TYPEI_STEP,
    WD_TYPEI_STEP_PULSE,
    WD_TYPEI_CHECK_VERIFY,
    WD_TYPEI_HEAD_SETTLE,
    WD_TYPEI_FIND_ID,
    WD_TYPEI_READ_ID, // read ID must be right after find ID
    WD_TYPEI_TEST_ID, // test ID must be right after read ID
    WD_TYPEII_SPINUP,
    WD_TYPEII_SPUNUP, // spunup must be right after spinup
    WD_TYPEII_HEAD_SETTLE, //head settle must be right after spunup
    WD_TYPEII_FIND_ID,
    WD_TYPEII_READ_ID, // read ID must be right after find ID
    WD_TYPEII_TEST_ID, // test ID must be right after read ID
    WD_TYPEII_FIND_DAM,
    WD_TYPEII_READ_DATA,
    WD_TYPEII_READ_CRC,
    WD_TYPEII_CHECK_MULTIPLE,
    WD_TYPEII_WRITE_DAM,
    WD_TYPEII_WRITE_DATA,
    WD_TYPEII_WRITE_CRC,
    WD_TYPEIII_SPINUP,
    WD_TYPEIII_SPUNUP, // spunup must be right after spinup
    WD_TYPEIII_HEAD_SETTLE, //head settle must be right after spunup
    WD_TYPEIII_IP_START, // start read/write
    WD_TYPEIII_FIND_ID,
    WD_TYPEIII_READ_ID, // read ID must be right after find ID
    WD_TYPEIII_TEST_ID,
    WD_TYPEIII_READ_DATA,
    WD_TYPEIII_WRITE_DATA,
    WD_TYPEIII_WRITE_DATA2, // CRC is 1 byte in RAM -> 2 bytes on disk
    WD_TYPEIV_4, // $D4
    WD_TYPEIV_8, // $D8
    WD_MOTOR_OFF,
  };
#endif
  // DATA
#if defined(SSE_WD1772_EMU)
  int prg_phase;
  int update_time; // when do we need to come back?
  WORD ByteCount; // guessed
  // definition is outside the class but objects belong to the class
  TWD1772IDField IDField; // to R/W those fields
  TWD1772Crc CrcLogic;
#if defined(SSE_WD1772_BIT_LEVEL)
  TWD1772Dpll Dpll;
#endif
  TWD1772AmDetector Amd; // not the processor
  TWD1772MFM Mfm;
#endif
  BYTE CR;  // command
  BYTE STR; // status
  BYTE TR;  // track
  BYTE SR;  // sector
  BYTE DR;  // data
  BYTE DSR; // shift register - official
#if defined(SSE_WD1772_EMU)
#if defined(SSE_WD1772_F7_ESCAPE) //keep a switch because I'm not sure of this
  bool F7_escaping;
#endif
  BYTE n_format_bytes; // to examine sequences before ID
#endif
  BYTE StatusType; // guessed
  BYTE InterruptCondition; // guessed
  BYTE IndexCounter; // guessed

/*  Lines (pins). Some are necessary (eg direction), others not
    really yet (eg write_gate).
    TODO put them all in then (fun)
*/
  struct {
    unsigned int drq:1;
    unsigned int irq:1;
    unsigned int motor:1;
    unsigned int direction:1;  // can't take the address of this for Boiler
    unsigned int track0:1;  
    unsigned int step:1; 
    unsigned int read:1;
    unsigned int write:1;
    unsigned int write_protect:1;
    unsigned int write_gate:1;
    unsigned int ip:1;
#if defined(SSE_DISK_GHOST)
    unsigned int CommandWasIntercepted:1; // pseudo
#endif
  }Lines;

  //FUNCTIONS

#if defined(SSE_DISK_GHOST)
  bool CheckGhostDisk(BYTE drive,BYTE io_src_b);
#endif
  BYTE CommandType(int command=-1); // I->IV
  BYTE IORead(BYTE Line);
  void IOWrite(BYTE Line,BYTE io_src_b);
  bool WritingToDisk();
  void Reset();
#if defined(SSE_DEBUG)
  void TraceStatus();
#endif

#if defined(SSE_WD1772_EMU)
  void NewCommand(BYTE command);
  bool Drq(bool state);//no default to make it clearer
  void Irq(bool state);
  void Motor(bool state);
  void Read(); // sends order to drive
  void StepPulse();
  int MsToCycles(int ms);
  void Write(); // sends order to drive
  void WriteCR(BYTE io_src_b); //horrible TODO
  void OnUpdate();
  // called by drive or by image

#if defined(SSE_DEBUG)
  void OnIndexPulse(int Id, bool image_triggered); 
#else
  void OnIndexPulse(bool image_triggered); 
#endif
#if defined(SSE_WD1772_BIT_LEVEL)
  bool ShiftBit(int bit);
#endif
#endif//emu
};

//#endif

#pragma pack(pop)

#endif//#ifndef WD1772_H