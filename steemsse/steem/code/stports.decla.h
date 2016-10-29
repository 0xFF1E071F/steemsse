#pragma once
#ifndef STPORTS_DECLA_H
#define STPORTS_DECLA_H

#include <portio.h>

extern void agenda_midi_replace(int);
extern void MidiInBufNotEmpty();

extern void ParallelInputNotify();
extern void ParallelOutputNotify();
extern void UpdateCentronicsBusyBit();
extern void agenda_check_centronics_interrupt(int);

extern void SerialInBufNotEmpty();

extern void PortsRunStart(),PortsRunEnd(),PortsOpenAll();

class TMIDIIn;//?
class TMIDIOut;

#define PORTTYPE_NONE 0
#define PORTTYPE_MIDI 1
#define PORTTYPE_PARALLEL 2
#define PORTTYPE_COM 3
#define PORTTYPE_FILE 4
#define PORTTYPE_LOOP 5
#define PORTTYPE_LAN 6
#define PORTTYPE_UNIX_SEQUENCER 100
#define PORTTYPE_UNIX_OTHER 101


#if defined(SSE_DONGLE)
struct TDongle { 
  enum {NONE,BAT2,MUSIC_MASTER,URC,LEADERBOARD,TENTHFRAME,MULTIFACE,PROSOUND,
    CRICKET,RUGBY,SOCCER};
  //BYTE ID;
  int Timing;
  WORD Value;
};
extern TDongle Dongle;

#if defined(SSE_DONGLE_PORT3) 
#define DONGLE_ID STPort[3].Type // in case we choose another way later
#endif

#endif

#ifdef UNIX
// Convert Porttype to the type of IO device or -1 if not an IO device
inline int GetPortIOType(int PortType)
{
	switch (PortType){
		case PORTTYPE_MIDI:       return TPORTIO_TYPE_MIDI;
		case PORTTYPE_PARALLEL:   return TPORTIO_TYPE_PARALLEL;
		case PORTTYPE_COM:        return TPORTIO_TYPE_SERIAL;
		case PORTTYPE_UNIX_OTHER: return TPORTIO_TYPE_UNKNOWN;
    case PORTTYPE_LAN:        return TPORTIO_TYPE_PIPE;
	}
	return -1;
}
#endif


#define PORT_LOOPBUFSIZE 8192

#define MIDIPort STPort[0]
#define ParallelPort STPort[1]
#define SerialPort STPort[2]

class TSTPort
{
private:
public:
  TSTPort();
  ~TSTPort() {Close();}

  bool Create(Str &,Str &,bool);
  bool IsOpen() { return MIDI_Out || MIDI_In || PCPort || f || LoopBuf || PCPortIn; }
  bool IsPCPort() { return PCPort!=NULL; }
  void SetupCOM(int,bool,int,int,bool,BYTE,BYTE,BYTE);
  DWORD GetModemFlags();
  bool SetDTR(bool),SetRTS(bool);

  bool OutputByte(BYTE);
  bool StartBreak(),EndBreak();
  bool AreBytesToOutput();
  void StartOutput(),StopOutput();
  WIN_ONLY( int GetMIDIOutDeviceID(); )

  void StartInput(),StopInput();
  bool AreBytesToRead();
  bool AreBytesToCome() { return AreBytesToRead(); }
  void NextByte();
  BYTE ReadByte();
  WIN_ONLY( int GetMIDIInDeviceID(); )

  void Reset();
  void Close();

  int Type;
#ifdef WIN32
  int MIDIOutDevice,MIDIInDevice;
  int COMNum,LPTNum;
#elif defined(UNIX)
  Str PortDev[TPORTIO_NUM_TYPES];
  Str LANPipeIn; // Need seperate device for input for PORTTYPE_LAN
  bool AllowIO[TPORTIO_NUM_TYPES][2];
#endif
  EasyStr File;

  TMIDIOut *MIDI_Out;
  TMIDIIn *MIDI_In;
  TPortIO *PCPort,*PCPortIn;
  FILE *f;
  CircularBuffer *LoopBuf;
};

#if defined(SSE_DONGLE_PORT3)
extern TSTPort STPort[4];
#else
extern TSTPort STPort[3];
#endif
#endif//#ifndef STPORTS_DECLA_H