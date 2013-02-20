#pragma once
#ifndef SSESHIFTEREVENTS_H
#define SSESHIFTEREVENTS_H


#if defined(SS_SHIFTER_EVENTS)
/*  Function: record all "shifter events" of a frame. Save as file what I
    find a fantastic report (helped me fix many cases).
    At some point, to replace the Hatari hack  I tried using it for emulation,
    but it's too slow, probably because the table is too big (cache misses?)
*/

struct SVideoEvent {
  int Scanline; 
  int Cycle;
  char Type;
  int Value; 
  inline void Add(int scanline,int cycle, char type, int value); 
};


class TVideoEvents {
public:
  enum {MAX_EVENTS=210*32*2*2}; // too high?
  int TriggerReport; // set 2 to ask a full report, then it's set FALSE again
  TVideoEvents();
  inline void Add(int scanline, int cycle, char type, int value);
//  inline void Add(int pc,int scanline, int cycle, char type, int value);
  void Init();
  int Report();
  int Vbl(); 
  int nVbl;
  struct SVideoEvent m_VideoEvent[MAX_EVENTS]; // it's public
private:
  int m_nEvents; // how many video events occurred this vbl?
  int m_nReports;
};


extern TVideoEvents VideoEvents; // singleton


// inline functions


inline void SVideoEvent::Add(int scanline,int cycle, char type, int value) {
  Scanline=scanline;
  Cycle=cycle;
  Type=type;
  Value=value;
}


extern int shifter_freq_at_start_of_vbl; // forward
inline void TVideoEvents::Add(int scanline, int cycle, char type, int value) {
  //if(type=='P' || type=='Q' || type=='p') return; // raster effects slow down emu
  m_nEvents++;  // starting from 0 each VBL, event 0 is dummy 
  if(m_nEvents<=0||m_nEvents>MAX_EVENTS) {BRK(bad m_nEvents); return;}
  int total_cycles= (shifter_freq_at_start_of_vbl==50) ?512:508;// Shifter.CurrentScanline.Cycles;//512;
  if(
    
    0&&
    cycle>total_cycles) // if hbl was delayed, adjust here
  {
    cycle-=total_cycles;
    scanline++;
  }
  m_VideoEvent[m_nEvents].Add(scanline, cycle, type, value);
}

#endif

#endif//#ifndef SSESHIFTEREVENTS_H