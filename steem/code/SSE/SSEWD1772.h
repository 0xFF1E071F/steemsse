#pragma once
#ifndef WD1772_H
#define WD1772_H

#include <conditions.h>

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

struct TWD1772IDField {
  BYTE track;
  BYTE side;
  BYTE num;
  BYTE len;
  BYTE CRC[2]; // not a WORD because ST was big-endian
  TWD1772IDField();
  WORD nBytes();
//#ifdef SSE_DEBUG
  void Trace(); // empty body if not debug
//#endif  
};


struct TWD1772MFM {
  enum {NORMAL_CLOCK,FORMAT_CLOCK};
  void Decode ();
  void Encode(int mode=NORMAL_CLOCK);
  BYTE data;
  BYTE clock; 
  WORD encoded;
  unsigned int data_last_bit:1;
};


struct TWD1772Crc {
  WORD crc;
  void Reset();
  void Add(BYTE data);
  bool Check(TWD1772IDField *IDField);
};



struct TWD1772 {

/*  Status bits.
    We define our own because Steem's are a bit long.
    We also define command flags of the docs, and some
    masks to better ID commands.
*/
  enum { STR_MO=BIT_7,STR_WP=BIT_6,STR_RT=BIT_5,STR_SU=BIT_5,
    STR_RNF=BIT_4,STR_SE=BIT_4,STR_CRC=BIT_3,STR_LD=BIT_2,
    STR_T0=BIT_2,STR_DRQ=BIT_1,STR_IP=BIT_1,STR_BUSY=BIT_0
  };

  enum { CR_U=BIT_4,CR_M=BIT_4,CR_H=BIT_3,CR_V=BIT_2,CR_E=BIT_2,
    CR_R1=BIT_1,CR_P=BIT_1,CR_R0=BIT_0,CR_A0=BIT_0,
    CR_I3=BIT_3,CR_I2=BIT_2,CR_I1=BIT_1,CR_I0=BIT_0,
    CR_STEP=(BIT_6|BIT_5),CR_STEP_IN=BIT_6,CR_STEP_OUT=CR_STEP,
    CR_SEEK=BIT_4,CR_RESTORE=0,
    CR_TYPEII_WRITE=BIT_5,CR_TYPEIII_WRITE=BIT_4,CR_TYPEIII_READ_ADDRESS=0xC0
  };


#if defined(SSE_WD1772_PHASE)

/*  We identify various phases in the Western Digital flow charts, so that we
    know where to "jump" next.
    We have two delay sources: current algorithm processing and index pulses.
    This could correspond to two different programs running in parallel. (?)
*/

  enum {
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
    WD_MOTOR_OFF
  };
  void NewCommand(BYTE command);
  int prg_phase;
#if defined(SSE_WD1772_F7_ESCAPE)
  bool F7_escaping;
#endif

#endif

  // IO registers
  BYTE CR;  // command
  BYTE STR; // status
  BYTE TR;  // track
  BYTE SR;  // sector
  BYTE DR;  // data

/*  Internal registers
    Some of them are mentioned in the doc, some are guesses or constructs.
    TODO: change order 
*/

#if defined(SSE_WD1772_REG2) //v3.7.0
/*  By adding registers here, we break snapshots, but we prefer to group
    variables coherently. 
    Since snapshots won't be OK for this, we may change sizes too.
*/
  BYTE DSR; // shift register - official
#if defined(SSE_DISK_STW)
  WORD ByteCount; // guessed
#endif
#ifdef SSE_WD1772_REG2_B
  BYTE StatusType; // guessed
#endif
  // definition is outside the class but objects belong to the class
  TWD1772IDField IDField; // to R/W those fields
  TWD1772Crc CrcChecker;
  TWD1772MFM Mfm;
#endif


#if defined(SSE_FDC_FORCE_INTERRUPT) || defined(SSE_WD1772)
  BYTE InterruptCondition; // guessed
#endif
#if defined(SSE_FDC_INDEX_PULSE_COUNTER) || defined(SSE_WD1772)
  BYTE IndexCounter; // guessed
#endif

/*  Lines (pins). Some are necessary (eg direction), others not
    really yet (eg write_gate).
    TODO put them all in then (fun)
*/
#if defined(SSE_DISK_STW)
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
#endif

#if defined(SSE_DISK_STW)
  BYTE n00,nFF,n_format_bytes; // to examine sequences before ID
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_FLOPPY_EVENT)
  int update_time; // when do we need to come back?
#endif


/*  functions
*/

  BYTE IORead(BYTE Line);
  void IOWrite(BYTE Line,BYTE io_src_b);
  BYTE CommandType(int command=-1); // I->IV

#if defined(SSE_FDC_RESET)
  void Reset(bool Cold);
#endif

#if defined(SSE_DISK_STW)
  bool Drq(bool state);//no default to make it clearer
  void Irq(bool state);
  void Motor(bool state);
  void Read(); // sends order to drive
  void StepPulse();
  int MsToCycles(int ms);
  void Write(); // sends order to drive
  void WriteCR(BYTE io_src_b); //horrible TODO

#if defined(STEVEN_SEAGAL) && defined(SSE_FLOPPY_EVENT)
  void OnUpdate();
#endif

#endif//#if defined(SSE_WD1772_REG2) //v3.7.0

#if defined(SSE_DEBUG) || defined(SSE_OSD_DRIVE_LED)
/*  This is useful for OSD: if we're writing then we need to display a red
    light (green when reading). This is used by pasti & IPF.
*/
  int WritingToDisk();
#endif

#if defined(SSE_DRIVE_INDEX_PULSE)
  void OnIndexPulse(int id); // called by drives (activates ip line)
#endif

#if defined(SSE_DISK_GHOST)
  bool CheckGhostDisk(BYTE drive,BYTE io_src_b);
#endif


#if defined(SSE_DEBUG)
  void TraceStatus();
#endif
};

#endif//#ifndef WD1772_H