/*---------------------------------------------------------------------------
FILE: psg.cpp
MODULE: emu
DESCRIPTION: Steem's Programmable Sound Generator (Yamaha 2149) and STE
DMA sound output emulation. Sound_VBL is the main function writing one
frame of sound to the output buffer. The I/O code isn't included here, see
ior.cpp and iow.cpp for the lowest level emulation.
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: psg.cpp")
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_PSG_H)

#ifdef IN_EMU
#define EXT
#define INIT(s) =s
#endif

#ifdef ENABLE_VARIABLE_SOUND_DAMPING
EXT int sound_variable_a INIT(32);
EXT int sound_variable_d INIT(208);
#endif

EXT bool sound_internal_speaker INIT(false);
EXT int sound_freq INIT(50066),sound_comline_freq INIT(0),sound_chosen_freq INIT(50066);
EXT int sound_mode INIT(SOUND_MODE_CHIP),sound_last_mode INIT(SOUND_MODE_CHIP);
EXT BYTE sound_num_channels INIT(1),sound_num_bits INIT(8);
EXT int sound_bytes_per_sample INIT(1);
#if defined(SSE_SOUND_VOL_LOGARITHMIC_2)
EXT int MaxVolume INIT(0xffff);
#else
EXT DWORD MaxVolume INIT(0xffff);
#endif
EXT bool sound_low_quality INIT(0);
EXT bool sound_write_primary INIT( NOT_ONEGAME(0) ONEGAME_ONLY(true) );
EXT bool sound_click_at_start INIT(0);
EXT int sound_time_method INIT(0);
EXT bool sound_record INIT(false);
EXT DWORD sound_record_start_time; //by timer variable = timeGetTime()
EXT int psg_write_n_screens_ahead INIT(3 UNIX_ONLY(+7) );

EXT int psg_voltage,psg_dv;

EXT int psg_reg_select;
EXT BYTE psg_reg[16],psg_reg_data;

EXT FILE *wav_file INIT(NULL);

#if defined(ENABLE_LOGFILE) || defined(SHOW_WAVEFORM)
EXT DWORD min_write_time;
EXT DWORD play_cursor,write_cursor;
#endif

EXT int sound_buffer_length INIT(DEFAULT_SOUND_BUFFER_LENGTH);
EXT DWORD SoundBufStartTime;

#if SCREENS_PER_SOUND_VBL == 1
#define MOD_PSG_BUF_LENGTH %PSG_BUF_LENGTH
EXT int cpu_time_of_last_sound_vbl INIT(0);
#endif

EXT DWORD psg_last_play_cursor;
EXT DWORD psg_last_write_time;
EXT DWORD psg_time_of_start_of_buffer;
EXT DWORD psg_time_of_last_vbl_for_writing,psg_time_of_next_vbl_for_writing;
EXT int psg_n_samples_this_vbl;

#ifdef SHOW_WAVEFORM

EXT int temp_waveform_display_counter;
EXT BYTE temp_waveform_display[DEFAULT_SOUND_BUFFER_LENGTH];
EXT DWORD temp_waveform_play_counter;

#endif

EXT FILE *psg_capture_file INIT(NULL);
EXT int psg_capture_cycle_base INIT(0);

EXT bool psg_always_capture_on_start INIT(0);

//#ifdef IN_EMU

//not all are used
#define ONE_MILLION 1048576
#define TWO_MILLION 2097152
//two to the power of 21
#define TWO_TO_SIXTEEN 65536
#define TWO_TO_SEVENTEEN 131072
#define TWO_TO_EIGHTEEN 262144

bool sound_first_vbl=0;

void sound_record_to_wav(int,DWORD,bool,int*);

EXT BYTE dma_sound_control,dma_sound_mode;
EXT MEM_ADDRESS dma_sound_start=0,next_dma_sound_start=0,
            dma_sound_end=0,next_dma_sound_end=0;

WORD MicroWire_Mask=0x07ff;
WORD MicroWire_Data=0;
int MicroWire_StartTime=0;

int dma_sound_mode_to_freq[4]={6258,12517,25033,50066};
int dma_sound_freq,dma_sound_output_countdown,dma_sound_samples_countdown;

WORD dma_sound_internal_buf[4],dma_sound_last_word;
int dma_sound_internal_buf_len=0;
MEM_ADDRESS dma_sound_fetch_address;

WORD dma_sound_channel_buf[DMA_SOUND_BUFFER_LENGTH+16];
DWORD dma_sound_channel_buf_last_write_t;
int dma_sound_on_this_screen=0;

int dma_sound_mixer=1,dma_sound_volume=40;
int dma_sound_l_volume=20,dma_sound_r_volume=20;
int dma_sound_l_top_val=128,dma_sound_r_top_val=128;

#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_MICROWIRE)
#include "../../3rdparty/dsp/dsp.h"
int dma_sound_bass=6; // 6 is neutral value
int dma_sound_treble=6;
TIirVolume MicrowireVolume[2];
TIirLowShelf MicrowireBass[2];
TIirHighShelf MicrowireTreble[2];
#if defined(SSE_SOUND_VOL)
TIirVolume PsgGain;
#endif
#endif//microwire

int psg_channels_buf[PSG_CHANNEL_BUF_LENGTH+16];
int psg_buf_pointer[3];
DWORD psg_tone_start_time[3];
char psg_noise[PSG_NOISE_ARRAY];

/*
  SS This table was in original psg.h. 
  Where does it come from?
  We can see that they took the values of the second column.
*/
/*
0 0     0      0
1 0.001 0.0045 0.0041
2 0.002 0.0081 0.0053
3 0.005 0.0117 0.008
4 0.02  0.0175 0.0124
5 0.05  0.0241 0.0158
6 0.065 0.0355 0.0211
7 0.08  0.048  0.0317
8 0.1   0.069  0.0596
9 0.125 0.095  0.0938
A 0.175 0.139  0.131
B 0.25  0.191  0.207
C 0.36  0.287  0.312
D 0.51  0.407  0.46
E 0.71  0.648  0.67
F 1     1      1
*/


#define VFP VOLTAGE_FIXED_POINT
#define VZL VOLTAGE_ZERO_LEVEL
#define VA VFP*(PSG_CHANNEL_AMPLITUDE) //SS 15360, 1/3 of 46k



const int psg_flat_volume_level[16]={0*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,8*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,
                                      17*VA/1000+VZL*VFP,24*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,48*VA/1000+VZL*VFP,
                                      69*VA/1000+VZL*VFP,95*VA/1000+VZL*VFP,139*VA/1000+VZL*VFP,191*VA/1000+VZL*VFP,
                                      287*VA/1000+VZL*VFP,407*VA/1000+VZL*VFP,648*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP};


#if defined(SSE_YM2149_FIXED_VOL_FIX1) //undef v3.7.0
/*  Values based on the graphic in Yamaha doc.
    It remains to be seen/heard if the sound is better with these values or
    Steem original values.
    For that reason, the mod is optional ("P.S.G.").
*/

const int psg_flat_volume_level2[16]=
{0*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,13*VA/1000+VZL*VFP,
22*VA/1000+VZL*VFP,31*VA/1000+VZL*VFP,41*VA/1000+VZL*VFP,63*VA/1000+VZL*VFP,
89*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,177*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,
354*VA/1000+VZL*VFP,500*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP};

#endif

#if defined(SSE_YM2149_FIXED_VOL_TABLE) 
/*  For this mod we use ljbk's table when we reckon we're playing samples.
    This is the case when tone and noise generators are disabled.
    Note that some games play samples on only some channel(s) or with another
    technique. eg Goldrunner, for those the table isn't used, it's a
    different sound.
    This doesn't fix all sample playing problems.
    ST-CNX scroller, ...
    v3.7.0: table used all the time, eg also for Goldrunner
*/


#if !defined(SSE_YM2149_DYNAMIC_TABLE) || defined(SSE_YM2149_DYNAMIC_TABLE0)
#if !defined(SSE_YM2149_DYNAMIC_TABLE0)
const //const must be removed for linker 
#endif
 WORD fixed_vol_3voices[16][16][16]= 
#include "../../3rdparty/various/ym2149_fixed_vol.h" //more bloat...
#endif


//TODO load only when check option

inline bool playing_samples() {
#if defined(SSE_SOUND_FILTER_STF3)
/*  This test is more complicated but will work with
    eg My socks are weapons.
*/
  ASSERT( PSG_FILTER_FIX );
  int yes=((psg_reg[PSGR_MIXER]&b00111111)==b00111111);
  if(!yes)
  {
    int freqs=0;
    for(int i=0;i<7;i++)
      freqs+=psg_reg[i]; // all zero?
    if(!freqs)
      yes=2;
    //yes=!psg_reg[PSGR_ENVELOPE_SHAPE];
  }
#if defined(SSE_BOILER_TRACE_CONTROL)
  if(yes&&(SOUND_CONTROL_MASK & SOUND_CONTROL_OSD))
    TRACE_OSD("DIGI%d",yes);
#endif
  return (bool) yes;
#else
  return (psg_reg[PSGR_MIXER] & b00111111)==b00111111; // 1 = disabled
#endif
}

#if !defined(SSE_YM2149_DELAY_RENDERING1)
inline WORD get_fixed_volume() {
  ASSERT( playing_samples() );
  return fixed_vol_3voices[psg_reg[10]&15][psg_reg[9]&15][psg_reg[8]&15];
}
#endif

#endif


const int psg_envelope_level[8][64]={
    {1000*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    1000*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP},
    {1000*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP},
    {1000*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP},
    {1000*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP},
    {0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP},
    {0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP},
    {0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    1000*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP},
    {0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,1*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,3*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,10*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,30*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,55*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,70*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,88*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,110*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,149*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,210*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,290*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,420*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,590*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,841*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP}};


#if defined(SSE_YM2149_ENV_FIX1)
/*  Values based on the graphic in Yamaha doc.
    It remains to be seen/heard if the sound is better with these values or
    Steem original values.
    For that reason, the mod is optional.
    v3.7.0: this mod was wrong, we must take the values mentioned in the doc 841, 707, etc.
    What was I thinking?
    SSE_YM2149_ENV_FIX1 undef
*/

const int psg_envelope_level2[8][64]={
    {1000*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    1000*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP},
    {1000*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP},
    {1000*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    0*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP},
    {1000*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,
    VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP},
    {0*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    0*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP},
    {0*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP,VA+VZL*VFP},
    {0*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    1000*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP},
    {0*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,4*VA/1000+VZL*VFP,7*VA/1000+VZL*VFP,9*VA/1000+VZL*VFP,12*VA/1000+VZL*VFP,15*VA/1000+VZL*VFP,16*VA/1000+VZL*VFP,22*VA/1000+VZL*VFP,29*VA/1000+VZL*VFP,35*VA/1000+VZL*VFP,40*VA/1000+VZL*VFP,46*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,54*VA/1000+VZL*VFP,66*VA/1000+VZL*VFP,81*VA/1000+VZL*VFP,91*VA/1000+VZL*VFP,107*VA/1000+VZL*VFP,130*VA/1000+VZL*VFP,152*VA/1000+VZL*VFP,179*VA/1000+VZL*VFP,212*VA/1000+VZL*VFP,256*VA/1000+VZL*VFP,300*VA/1000+VZL*VFP,355*VA/1000+VZL*VFP,425*VA/1000+VZL*VFP,507*VA/1000+VZL*VFP,598*VA/1000+VZL*VFP,704*VA/1000+VZL*VFP,834*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP,
    VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP,VZL*VFP}};
#endif

#if defined(SSE_YM2149_DELAY_RENDERING)
/*  Rendering is done later, we save the 5bit digital value instead of the 
    16bit "rendered" volume.
    TODO: something smarter than a dumb table?
*/
  
const BYTE psg_envelope_level3[8][64]={
    {31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
     31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},
    {31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
     0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
    {31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,0,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

#endif

#undef VFP
#undef VZL
#undef VA

DWORD psg_envelope_start_time=0xfffff000;

#undef EXT
#undef INIT


#endif//#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_PSG_H)

#define LOGSECTION LOGSECTION_SOUND

#if defined(STEVEN_SEAGAL) && defined(SSE_VID_RECORD_AVI) && defined(WIN32)
extern IDirectSoundBuffer *PrimaryBuf,*SoundBuf;
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_MICROWIRE)
#define LOW_SHELF_FREQ 80 // 50
#define HIGH_SHELF_FREQ (dma_sound_freq) // doesn't work very well
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
HRESULT Sound_Start() // SS called by
{
#ifdef UNIX
  if (sound_internal_speaker){
    console_device=open("/dev/console",O_RDONLY | O_NDELAY,0);
    if (console_device==-1){
      printf("Couldn't open console for internal speaker output\n");
      sound_internal_speaker=false;
      GUIUpdateInternalSpeakerBut();
    }
  }
#endif
  if (sound_mode==SOUND_MODE_MUTE) return DS_OK;
  if (UseSound==0) return DSERR_GENERIC;  // Not initialised
  if (SoundActive()) return DS_OK;        // Already started

  if (fast_forward || slow_motion || runstate!=RUNSTATE_RUNNING) return DSERR_GENERIC;

  sound_first_vbl=true;
  log("SOUND: Starting sound buffers and initialising PSG variables");

  // Work out startup voltage
  int envshape=psg_reg[13] & 15;
  int flatlevel=0;
#if defined(SSE_YM2149_FIXED_VOL_TABLE) && !defined(SSE_YM2149_NO_SAMPLES_OPTION)
  if(SSEOption.PSGFixedVolume&& playing_samples())
    flatlevel=get_fixed_volume();
  else
#endif
  for (int abc=0;abc<3;abc++){
    if ((psg_reg[8+abc] & BIT_4)==0){
#if defined(SSE_YM2149_FIXED_VOL_FIX1)
      flatlevel+=SSEOption.PSGMod?psg_flat_volume_level2[psg_reg[8+abc] & 15]:
        psg_flat_volume_level[psg_reg[8+abc] & 15];
#endif
    }else if (envshape==b1011 || envshape==b1101){
      flatlevel+=psg_flat_volume_level[15]; //SS 15 = 1 anyway
    }
  }
  psg_voltage=flatlevel;psg_dv=0;

  WORD dma_l,dma_r;
  dma_sound_get_last_sample(&dma_l,&dma_r);
  int current_l=HIBYTE(flatlevel)+HIBYTE(dma_l),current_r=HIBYTE(flatlevel)+HIBYTE(dma_r);
  if (sound_num_bits==16){
    current_l^=128;current_r^=128;
  }
  if (SoundStartBuffer((signed char)current_l,(signed char)current_r)!=DS_OK){
    return DDERR_GENERIC;
  }
  for (int n=PSG_NOISE_ARRAY-1;n>=0;n--) psg_noise[n]=(BYTE)random(2);//SS TODO

#ifdef ONEGAME
  // Make sure sound is still good(ish) if you are running below 80% speed
  OGExtraSamplesPerVBL=300;
  if (run_speed_ticks_per_second>1000){
    // Get the number of extra ms of sound per "second", change that to number
    // of samples, divide to get the number of samples per VBL and add extra.
    OGExtraSamplesPerVBL=((((run_speed_ticks_per_second-1000)*sound_freq)/1000)/shifter_freq)+300;
  }
#endif

  psg_time_of_start_of_buffer=0;
  psg_last_play_cursor=0;
//  psg_last_write_time=0; not used now?
  psg_time_of_last_vbl_for_writing=0;
  psg_time_of_next_vbl_for_writing=0;

/*
  psg_time_of_start_of_buffer=(0xffffffff-(sound_freq*4)) &(-PSG_BUF_LENGTH);
  psg_last_play_cursor=psg_time_of_start_of_buffer;
  psg_last_write_time=psg_time_of_start_of_buffer;
  psg_time_of_last_vbl_for_writing=psg_time_of_start_of_buffer;
  psg_time_of_next_vbl_for_writing=psg_time_of_start_of_buffer;
*/

  for (int abc=2;abc>=0;abc--){
    psg_buf_pointer[abc]=0;
    psg_tone_start_time[abc]=0;
  }
  for (int i=0;i<PSG_CHANNEL_BUF_LENGTH;i++) psg_channels_buf[i]=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);
  psg_envelope_start_time=0xff000000;

  if (sound_record){
    timer=timeGetTime();
    sound_record_start_time=timer+200; //start recording in 200ms time
    sound_record_open_file();
  }

  return DS_OK;
}
//---------------------------------------------------------------------------
void SoundStopInternalSpeaker()
{
  internal_speaker_sound_by_period(0);
}
//---------------------------------------------------------------------------
#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_INLINE)
/*  We transform some macros into inline functions to make conditional 
    compilation easier.
    We use tricks to make the macro calls work without change
    TODO: check for performance
*/

inline void CalcVChip(int &v,int &dv,int *source_p) {
  //CALC_V_CHIP

#define NBITS 5

#define proportion 10

#if defined(SSE_SOUND_FILTER_STF) //tests
  if(PSG_FILTER_FIX // Option PSG Filter
#if defined(SSE_SOUND_FILTER_STF2) // v3.7.0, too noisy on samples
    && !playing_samples() 
#endif
    ) 
  {
    v=SSE_SOUND_FILTER_STF_V;
    dv=SSE_SOUND_FILTER_STF_DV;
  }
  else 
#endif
  if (v!=*source_p || dv){                            
#if 1 //for tests
    v+=dv;             
    dv-=(v-(*source_p))>> 3;        
    dv*=13;           
    dv>>=4;   
#elif defined(ENABLE_VARIABLE_SOUND_DAMPING)   // defined() for mingw
 // Boiler control, useless now (undef)
    v+=dv;             
    dv-=(v-(*source_p))*sound_variable_a >> 8;        
    dv*=sound_variable_d;           
    dv>>=8;   
#else    
    v+=dv;                                            
    dv-=(v-(*source_p)) >> 3;                         
    dv*=13;                                           
    dv>>=4; 
#endif
  }
}


inline void CalcVChip25Khz(int &v,int &dv,int *source_p) {
  //CALC_V_CHIP_25KHZ = low quality
#if defined(SSE_SOUND_FILTER_STF) 
  if(PSG_FILTER_FIX) // Option PSG Filter
  {
    v=SSE_SOUND_FILTER_STF_V;
    dv=SSE_SOUND_FILTER_STF_DV;
  }
  else 
#endif
  if (v!=*source_p || dv){      
#ifdef ENABLE_VARIABLE_SOUND_DAMPING    
    v+=dv;             
    dv-=(v-(*source_p))*sound_variable_a >> 8;        
    dv*=sound_variable_d;           
    dv>>=8;   
#else   
    v+=dv;                                            
    dv-=((v-(*source_p)) *3) >>3;                         
    dv*=3;                                           
    dv>>=2;                                           
#endif
  }
}


inline void CalcVEmu(int &v,int *source_p) {
  //CALC_V_EMU  = direct
  v=*source_p;
}


#define CALC_V_CHIP             1
#define CALC_V_CHIP_25KHZ       2
#define CALC_V_EMU              3

#ifdef SHOW_WAVEFORM
  #define WAVEFORM_SET_VAL(v) (val=(v))
  #define WAVEFORM_ONLY(x) x
#else
  #define WAVEFORM_SET_VAL(v) v
  #define WAVEFORM_ONLY(x)
#endif

#ifdef WRITE_ONLY_SINE_WAVE //SS not defined
#define SINE_ONLY(s) s
//todo, one day...
#define WRITE_SOUND_LOOP(Alter_V)         \
	          while (c>0){                                                  \
                *(p++)=WAVEFORM_SET_VAL(BYTE(sin((double)t*(M_PI/64))*120+128)); \
                t++;                                                       \
                WAVEFORM_ONLY( temp_waveform_display[((int)(source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=(BYTE)val; ) \
    	          *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);                 \
                c--;    \
	          }

#else
#define SINE_ONLY(s)
#endif

inline void AlterV(int Alter_V,int &v,int &dv,int *source_p) {

#if defined(SSE_YM2149_DELAY_RENDERING)
/*
    each *source_p element is a 32bit integer made up like this:

    byte 0: channel A
    byte 1: channel B
    byte 2: channel C

    each channel byte is made up like this:
    bit 0-4: volume on 5bit
    bit 6: envelope (1) /fixed (0)

*/  

#if defined(SSE_YM2149_DELAY_RENDERING1)  
/*  we use the fixed volume table to render sound on 3 channels
    this table is 16x16x16, but envelope volume is coded on 5bit, so
    we need to interpolate somehow
    We can interpolate 15 values, so we have 15+16 but need 32
    31 -> 15
    30 -> 14.5
    29 -> 14
    ...
    2 -> 0.5
    1 -> 0 !
    0 -> 0
    We sacrifice the lowest value, of course.
    We interpolate when the last bit isn't set.
    Because of that, we need to know if it was enveloped:
    11110 is max for fixed, 11111 is max for enveloped
    bit 6 is used for that (1: enveloped)
    PSG tunes sound distorted like in Hatari but that's correct
    eg Ace 2 on BIG demo
*/

  if(SSE_OPTION_PSG
#if defined(SSE_YM2149_DYNAMIC_TABLE)
    && YM2149.p_fixed_vol_3voices
#endif
    )
  {

    BYTE index[3],interpolate[3];
    for(int abc=0;abc<3;abc++)
    {
      index[abc]=( ((*source_p)>>1))&0xF; // 4bit volume

      interpolate[abc]=
        ( ((*source_p)&BIT_6) && index[abc]>0 && !((*source_p)&1)) ? 1 : 0;

      *source_p>>=8;
      ASSERT( interpolate[abc]<=1 );
      ASSERT( index[abc]<=15 );
    }
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
    int vol=YM2149.p_fixed_vol_3voices[(16*16)*index[2]+16*index[1]+index[0]];
#else
    int vol=fixed_vol_3voices[index[2]] [index[1]] [index[0]];
#endif
    if(*(int*)(&interpolate[0]))
    {
      /*//not when -1
      ASSERT( index[0]+interpolate[0]<=15 );
      ASSERT( index[1]+interpolate[1]<=15 );
      ASSERT( index[2]+interpolate[2]<=15 );
      */
      
      ASSERT( !((index[0]-interpolate[0])&0x80) );
      ASSERT( !((index[1]-interpolate[1])&0x80) );
      ASSERT( !((index[2]-interpolate[2])&0x80) );
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
      int vol2=YM2149.p_fixed_vol_3voices[ (16*16)*(index[2]-interpolate[2])
        +16*(index[1]-interpolate[1])+(index[0]-interpolate[0])];
#else
      int vol2=fixed_vol_3voices[index[2]-interpolate[2]] 
        [index[1]-interpolate[1]] [index[0]-interpolate[0]];
#endif
      vol= (int) sqrt( (float) vol * (float) vol2); 
    }

    *source_p=vol;
  }//SSE_OPTION_PSG
#endif

#endif//mixing

  // Dispatches to the correct function  
  if(Alter_V==CALC_V_CHIP)                    
    CalcVChip(v,dv,source_p);                 
  else if(Alter_V==CALC_V_CHIP_25KHZ)         
    CalcVChip25Khz(v,dv,source_p);            
  else if(Alter_V==CALC_V_EMU)                
    CalcVEmu(v,source_p);                     
}

#if defined(SSE_SOUND_MICROWIRE)   // microwire this!

inline void Microwire(int channel,int &val) {
  if(
#if defined(SSE_SOUND_OPTION_DISABLE_DSP)
    DSP_ENABLED&&
#endif
    MICROWIRE_ON)
  {
    double d_dsp_v=val;
#if defined(SSE_STF)
    if(ST_TYPE==STE)
    {
#endif
      if(dma_sound_bass!=6)
        d_dsp_v=MicrowireBass[channel].FilterAudio(d_dsp_v,LOW_SHELF_FREQ,
          dma_sound_bass-6);
//      if(dma_sound_treble!=6)  //3.6.1? too buggy
  //      d_dsp_v=MicrowireTreble[channel].FilterAudio(d_dsp_v,HIGH_SHELF_FREQ
    //     ,dma_sound_treble-6);
      if(dma_sound_volume<0x28
        ||dma_sound_l_volume<0x14 &&!channel 
        ||dma_sound_r_volume<0x14 &&channel)//3.6.1: 2 channels
        d_dsp_v=MicrowireVolume[channel].FilterAudio(d_dsp_v,
          dma_sound_volume-0x28+dma_sound_l_volume-0x14);
#if defined(SSE_STF)
    }
#endif
    val=d_dsp_v;
  }
}

#endif

/*  The function is called at VBL. The sounds have already been computed.
    The function adds an optional low-pass filter to PSG sound and adds
    PSG and DMA sound together. (Ground for improvement).
    It also applies Microwire filters.
    It shouldn't be important that it be inline.
*/


inline void WriteSoundLoop(int Alter_V, int* Out_P,int Size,int& c,int &val,
  int &v,int &dv,int **source_p,WORD**lp_dma_sound_channel,
  WORD**lp_max_dma_sound_channel) {
  // the big loop, was harder to inline
  while(c>0)
  {       
    AlterV(Alter_V,v,dv,*source_p);


#if defined(SSE_SOUND_MICROWIRE_MIXMODE)//3.6.3
/*

"In STe, YM sound is only audible when Input1 is selected (this is default TOS 
setting). In case of Steem, whether you choose Input1/2/3/4 or Open YM is 
always audible."
                              0 0  = DMA + (YM2149 - 12 db)
                              0 1  = DMA + YM2149
                              1 0  = DMA only
                              1 1  = reserved

    Variable dma_sound_mixer was updated in iow.cpp, but not used.
    Must be =1 to mix YM and DMA, -12db doesn't work.
    SS: Pacemaker writes 0 then plays a PSG tune! -> 2 compensating bugs
*/
    if(MICROWIRE_ON 
#if defined(SSE_STF)
      && ST_TYPE==STE // only if option checked and we're on STE
#endif
      )
    {
      if(dma_sound_mixer!=1)
      {
#if defined(SSE_SOUND_VOL2)//no
        //TRACE("dma_sound_mixer %d\n",dma_sound_mixer);
        if(!dma_sound_mixer)
          v=PsgGain.FilterAudio(v,-12); // Pacemaker?
        else
#endif
          v=0; // dma-only
      }
#if defined(SSE_SOUND_VOL) && !defined(SSE_SOUND_VOL2)
      else if(dma_sound_on_this_screen)
        v=PsgGain.FilterAudio(v,-6); 
#endif
    }
#endif//SSE_SOUND_MICROWIRE_MIXMODE

    val=v; //inefficient?

    if(dma_sound_on_this_screen) //bugfix v3.6.0
    {//3.6.1
#if defined(SSE_OSD_CONTROL)
    if(OSD_MASK3 & OSD_CONTROL_DMASND) 
      TRACE_OSD("F%d %cV%d %d %d B%d T%d",dma_sound_freq,(dma_sound_mode & BIT_7)?'M':'S',dma_sound_volume,dma_sound_l_volume,dma_sound_r_volume,dma_sound_bass,dma_sound_treble);
#endif
      
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS)
      if(! (d2_dpeek(FAKE_IO_START+20)>>15) ) //dma
#endif
        val+= (**lp_dma_sound_channel);                           

#if defined(SSE_SOUND_MICROWIRE)
/*
? I successfully wrote to the LMC1992, but now YM2149 sound output is pure 
torture. What happened ?

! Well, the LMC1992 is not a chip that controls the DMA-sound in its digital 
form but manipulates the analogue sound that comes out of the DMA chip. If you 
now put the mixer to mix YM2149 and DMA sound, the LMC1992 will also manipulate 
the YM sound output. However, the YM2149 as a soundchip is not really meant to 
have Bass and Trebble enhanced. This might result in a very ugly sound.
*/
    Microwire(0,val);
#endif
    }

    if (val<VOLTAGE_FP(0))
      val=VOLTAGE_FP(0); 
    else if (val>VOLTAGE_FP(255))
      val=VOLTAGE_FP(255); 

    // "*(Out_P++)=Size(GetSize(&val))" : the trickiest part
    if(Size==sizeof(BYTE))
    {
      *(BYTE*)*(BYTE**)Out_P=(BYTE)((val&0x00FF00)>>8);
      (*(BYTE**)Out_P)++;
    }
    else // WORD
    {
      *(WORD*)*(WORD**)Out_P=((WORD)val) ^ MSB_W;
      (*(WORD**)Out_P)++;
    }  

    // stereo: do the same for right channel
    if(sound_num_channels==2){    
      
      val=v;
      if(dma_sound_on_this_screen) //bugfix v3.6
      {
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS)
        if(! (d2_dpeek(FAKE_IO_START+20)>>15) ) 
#endif
          val+= (*(*lp_dma_sound_channel+1)); 

#if defined(SSE_SOUND_MICROWIRE)
        Microwire(1,val);
#endif
      }

      if(val<VOLTAGE_FP(0))
        val=VOLTAGE_FP(0); 
      else if (val>VOLTAGE_FP(255))
        val=VOLTAGE_FP(255); 

      if(Size==sizeof(BYTE))
      {
        *(BYTE*)*(BYTE**)Out_P=(BYTE)((val&0x00FF00)>>8);
        (* (BYTE**)Out_P )++;
      }
      else
      {
        *(WORD*)*(WORD**)Out_P=((WORD)val) ^ MSB_W;
        (*(WORD**)Out_P)++;
      }    
    }//right channel 
    WAVEFORM_ONLY(temp_waveform_display[((int)(*source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=WORD_B_1(&val)); 
    *(*source_p)++=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);
    if(*lp_dma_sound_channel<*lp_max_dma_sound_channel) 
      *lp_dma_sound_channel+=2;
    c--;                                                          
  }//wend
}



#define WRITE_SOUND_LOOP(Alter_V,Out_P,Size,GetSize) WriteSoundLoop(Alter_V,(int*)&Out_P,sizeof(Size),c,val,v,dv,&source_p,&lp_dma_sound_channel,&lp_max_dma_sound_channel);

#define WRITE_TO_WAV_FILE_B 1 
#define WRITE_TO_WAV_FILE_W 2 


inline void SoundRecord(int Alter_V, int Write,int& c,int &val,
  int &v,int &dv,int **source_p,WORD**lp_dma_sound_channel,
  WORD**lp_max_dma_sound_channel,FILE* wav_file) {
  while(c>0)
  {       
    AlterV(Alter_V,v,dv,*source_p);

#if defined(SSE_SOUND_MICROWIRE_MIXMODE)//3.6.3
    if(MICROWIRE_ON 
#if defined(SSE_STF)
      && ST_TYPE==STE // only if option checked and we're on STE
#endif
      )
    {
      if(dma_sound_mixer!=1)
        v=0; // dma-only
#if defined(SSE_SOUND_VOL)
      else if(dma_sound_on_this_screen)
        v=PsgGain.FilterAudio(v,-6); 
#endif
    }
#endif//SSE_SOUND_MICROWIRE_MIXMODE

    val=v;//3.6.3, was it missing???

    if(dma_sound_on_this_screen) //bugfix v3.6
      val+= (**lp_dma_sound_channel);    

#if defined(SSE_SOUND_MICROWIRE)
    Microwire(0,val);
#endif

    if (val<VOLTAGE_FP(0))
      val=VOLTAGE_FP(0); 
    else if (val>VOLTAGE_FP(255))
      val=VOLTAGE_FP(255); 

    if(Write==WRITE_TO_WAV_FILE_B) 
      fputc(BYTE(WORD_B_1(&(val))),wav_file);
    else 
    {
      val^=MSB_W;
      fputc(LOBYTE(val),wav_file);
      fputc(HIBYTE(val),wav_file);
    }

    if(sound_num_channels==2){    
      
      if(dma_sound_on_this_screen) //bugfix v3.6
        val+= (*(*lp_dma_sound_channel+1)); 

#if defined(SSE_SOUND_MICROWIRE)
    Microwire(1,val);
#endif
      
      if(val<VOLTAGE_FP(0))
        val=VOLTAGE_FP(0); 
      else if (val>VOLTAGE_FP(255))
        val=VOLTAGE_FP(255); 
      
      if(Write==WRITE_TO_WAV_FILE_B) 
        fputc(BYTE(WORD_B_1(&(val))),wav_file);
      else 
      {
        val^=MSB_W;
        fputc(LOBYTE(val),wav_file);
        fputc(HIBYTE(val),wav_file);
      }

    }//right

    (*source_p)++;// don't zero! (or mute when recording)

    SINE_ONLY( t++ );
    if(*lp_dma_sound_channel<*lp_max_dma_sound_channel) 
      *lp_dma_sound_channel+=2;
    c--;   
  }//wend
}

#define SOUND_RECORD(Alter_V,WRITE) SoundRecord(Alter_V,WRITE,c,val,v,dv,&source_p,&lp_dma_sound_channel,&lp_max_dma_sound_channel,wav_file)

#elif defined(SSE_SOUND) // reintegrate macros for debugging



#ifdef ENABLE_VARIABLE_SOUND_DAMPING //SS for boiler...

#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_FILTER_STF)  
// a simplisctic but better (in my ears) low-pass filter, optional
#define CALC_V_CHIP if(PSG_FILTER_FIX && (v!=*source_p || dv)) v=SSE_SOUND_FILTER_STF_V,dv=SSE_SOUND_FILTER_STF_DV;\
                    else if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p))*sound_variable_a >> 8;        \
                  dv*=sound_variable_d;                                           \
                  dv>>=8;                                           \
                }

#else
#define CALC_V_CHIP  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p))*sound_variable_a >> 8;        \
                  dv*=sound_variable_d;                                           \
                  dv>>=8;                                           \
                }
#endif

#define CALC_V_CHIP_25KHZ  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p))*sound_variable_a >> 8;        \
                  dv*=sound_variable_d;                                           \
                  dv>>=8;                                           \
                }

#else

#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_FILTER_STF)  
// a simplisctic but better (in my ears) low-pass filter, optional
// not always better...
#define CALC_V_CHIP if(PSG_FILTER_FIX && (v!=*source_p || dv)) v=SSE_SOUND_FILTER_STF_V,dv=SSE_SOUND_FILTER_STF_DV;\
                    else if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p)) >> 3;                         \
                  dv*=13;                                           \
                  dv>>=4;                                           \
                }
#else
#define CALC_V_CHIP  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p)) >> 3;                         \
                  dv*=13;                                           \
                  dv>>=4;                                           \
                }
#endif

#define CALC_V_CHIP_25KHZ  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=((v-(*source_p)) *3) >>3;                         \
                  dv*=3;                                           \
                  dv>>=2;                                           \
                }

//60, C0

#endif

#define CALC_V_EMU  v=*source_p; // SS: no filter, contrary to chipmode

#ifdef SHOW_WAVEFORM
  #define WAVEFORM_SET_VAL(v) (val=(v))
  #define WAVEFORM_ONLY(x) x
#else
  #define WAVEFORM_SET_VAL(v) v
  #define WAVEFORM_ONLY(x)
#endif



#ifdef WRITE_ONLY_SINE_WAVE //SS not defined

#define SINE_ONLY(s) s

#define WRITE_SOUND_LOOP(Alter_V)         \
	          while (c>0){                                                  \
                *(p++)=WAVEFORM_SET_VAL(BYTE(sin((double)t*(M_PI/64))*120+128)); \
                t++;                                                       \
                WAVEFORM_ONLY( temp_waveform_display[((int)(source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=(BYTE)val; ) \
    	          *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);                 \
                c--;    \
	          }

#else

#define SINE_ONLY(s)

#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_MICROWIRE)
/*
The LMC1992 is not a chip that controls the DMA-sound in its digital 
form but manipulates the analogue sound that comes out of the DMA chip. 
If you now put the mixer to mix YM2149 and DMA sound, the LMC1992 will 
also manipulate the YM sound output. However, the YM2149 as a soundchip 
is not really meant to have Bass and Trebble enhanced. This might result 
in a very ugly sound.
->
We redefine this macro to send to our DSP filters when Microwire is used,
for: volume, balance, bass, treble.
We define twice because of the SSE_SOUND_VOL option (v -6db for YM chip effects)
Demos using bass/treble:
Beat; White Spirit
TODO simplify, make more efficient
*/
double d_dsp_v; // a bit silly, heavy, and maybe not optimal
#define LOW_SHELF_FREQ 80 // 50
#define HIGH_SHELF_FREQ (dma_sound_freq) // doesn't work very well


#if defined(SSE_SOUND_VOL) // -6db for PSG sound (using DSP), if Microwire on

#define WRITE_SOUND_LOOP(Alter_V,Out_P,Size,GetSize)         \
             while (c>0){                                                  \
              Alter_V                                                     \
              if(MICROWIRE_ON && (psg_reg[PSGR_MIXER] & b00111111)!=b00111111 ) v=PsgGain.FilterAudio(v,-6); \
              val=v + *lp_dma_sound_channel;                           \
              if( MICROWIRE_ON&&(\
                dma_sound_bass!=6||dma_sound_treble!=6\
              ||dma_sound_volume<0x28\
              ||dma_sound_l_volume<0x14)) d_dsp_v=val;\
              if(MICROWIRE_ON&&dma_sound_bass!=6) \
                d_dsp_v=MicrowireBass[0].FilterAudio(d_dsp_v,LOW_SHELF_FREQ,dma_sound_bass-6);\
              if(MICROWIRE_ON&&dma_sound_treble!=6)\
                d_dsp_v=MicrowireTreble[0].FilterAudio(d_dsp_v,HIGH_SHELF_FREQ,dma_sound_treble-6);\
              if(MICROWIRE_ON&&(dma_sound_volume<0x28||dma_sound_l_volume<0x14))\
                d_dsp_v=MicrowireVolume[0].FilterAudio(d_dsp_v,dma_sound_volume-0x28\
                  +dma_sound_l_volume-0x14);\
              if( MICROWIRE_ON &&(\
                dma_sound_bass!=6||dma_sound_treble!=6\
              ||dma_sound_volume<0x28\
              ||dma_sound_l_volume<0x14))  val=d_dsp_v;\
  	          if (val<VOLTAGE_FP(0)){                                     \
                val=VOLTAGE_FP(0); \
      	      }else if (val>VOLTAGE_FP(255)){                            \
        	      val=VOLTAGE_FP(255);                    \
  	          }                                                            \
              *(Out_P++)=Size(GetSize(&val)); \
              if (sound_num_channels==2){         \
                val=v + *(lp_dma_sound_channel+1);                                            \
                if(MICROWIRE_ON&&(dma_sound_bass!=6||dma_sound_treble!=6\
                  ||dma_sound_volume<0x28\
                  ||dma_sound_r_volume<0x14)) \
                  d_dsp_v=val;\
                if(MICROWIRE_ON&&dma_sound_bass!=6) \
                  d_dsp_v=MicrowireBass[1].FilterAudio(d_dsp_v,LOW_SHELF_FREQ,dma_sound_bass-6);\
                if(MICROWIRE_ON&&dma_sound_treble!=6)\
                  d_dsp_v=MicrowireTreble[1].FilterAudio(d_dsp_v,HIGH_SHELF_FREQ,dma_sound_treble-6);\
                if(MICROWIRE_ON&&(dma_sound_volume<0x28||dma_sound_r_volume<0x14))\
                d_dsp_v=MicrowireVolume[1].FilterAudio(d_dsp_v,dma_sound_volume-0x28\
                  +dma_sound_r_volume-0x14);\
                if(MICROWIRE_ON&&(dma_sound_bass!=6||dma_sound_treble!=6\
                  ||dma_sound_volume<0x28\
                  ||dma_sound_r_volume<0x14)) \
                  val=d_dsp_v;\
                if (val<VOLTAGE_FP(0)){                                     \
                  val=VOLTAGE_FP(0); \
                }else if (val>VOLTAGE_FP(255)){                            \
                  val=VOLTAGE_FP(255);                    \
                }                                                            \
                *(Out_P++)=Size(GetSize(&val)); \
              }     \
    	        WAVEFORM_ONLY(temp_waveform_display[((int)(source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=WORD_B_1(&val)); \
  	          *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);                 \
              if (lp_dma_sound_channel<lp_max_dma_sound_channel) lp_dma_sound_channel+=2; \
      	      c--;                                                          \
	          }

#else
#define WRITE_SOUND_LOOP(Alter_V,Out_P,Size,GetSize)         \
             while (c>0){                                                  \
              Alter_V                                                     \
              val=v + *lp_dma_sound_channel;                           \
              if( MICROWIRE_ON&&(\
                dma_sound_bass!=6||dma_sound_treble!=6\
              ||dma_sound_volume<0x28\
              ||dma_sound_l_volume<0x14)) tmp=val;\
              if(MICROWIRE_ON&&dma_sound_bass!=6) \
                tmp=MicrowireBass[0].FilterAudio(tmp,LOW_SHELF_FREQ,dma_sound_bass-6);\
              if(MICROWIRE_ON&&dma_sound_treble!=6)\
                tmp=MicrowireTreble[0].FilterAudio(tmp,HIGH_SHELF_FREQ,dma_sound_treble-6);\
              if(MICROWIRE_ON&&(dma_sound_volume<0x28||dma_sound_l_volume<0x14))\
                tmp=MicrowireVolume[0].FilterAudio(tmp,dma_sound_volume-0x28\
                  +dma_sound_l_volume-0x14);\
              if( MICROWIRE_ON &&(\
                dma_sound_bass!=6||dma_sound_treble!=6\
              ||dma_sound_volume<0x28\
              ||dma_sound_l_volume<0x14))  val=tmp;\
  	          if (val<VOLTAGE_FP(0)){                                     \
                val=VOLTAGE_FP(0); \
      	      }else if (val>VOLTAGE_FP(255)){                            \
        	      val=VOLTAGE_FP(255);                    \
  	          }                                                            \
              *(Out_P++)=Size(GetSize(&val)); \
              if (sound_num_channels==2){         \
                val=v + *(lp_dma_sound_channel+1);                                            \
                if(MICROWIRE_ON&&(dma_sound_bass!=6||dma_sound_treble!=6\
              ||dma_sound_volume<0x28\
              ||dma_sound_r_volume<0x14)) \
                  tmp=val;\
                if(MICROWIRE_ON&&dma_sound_bass!=6) \
                  tmp=MicrowireBass[1].FilterAudio(tmp,LOW_SHELF_FREQ,dma_sound_bass-6);\
                if(MICROWIRE_ON&&dma_sound_treble!=6)\
                  tmp=MicrowireTreble[1].FilterAudio(tmp,HIGH_SHELF_FREQ,dma_sound_treble-6);\
                if(MICROWIRE_ON&&(dma_sound_volume<0x28||dma_sound_r_volume<0x14))\
                tmp=MicrowireVolume[1].FilterAudio(tmp,dma_sound_volume-0x28\
                  +dma_sound_r_volume-0x14);\
                if(MICROWIRE_ON&&(dma_sound_bass!=6||dma_sound_treble!=6\
              ||dma_sound_volume<0x28\
              ||dma_sound_r_volume<0x14)) \
                 val=tmp;\
                if (val<VOLTAGE_FP(0)){                                     \
                  val=VOLTAGE_FP(0); \
                }else if (val>VOLTAGE_FP(255)){                            \
                  val=VOLTAGE_FP(255);                    \
                }                                                            \
                *(Out_P++)=Size(GetSize(&val)); \
              }     \
    	        WAVEFORM_ONLY(temp_waveform_display[((int)(source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=WORD_B_1(&val)); \
  	          *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);                 \
              if (lp_dma_sound_channel<lp_max_dma_sound_channel) lp_dma_sound_channel+=2; \
      	      c--;                                                          \
	          }

#undef LOW_SHELF_FREQ
#undef HIGH_SHELF_FREQ

#endif

#else

#define WRITE_SOUND_LOOP(Alter_V,Out_P,Size,GetSize)         \
	          while (c>0){                                                  \
              Alter_V                                                     \
              val=v + *lp_dma_sound_channel;                           \
  	          if (val<VOLTAGE_FP(0)){                                     \
                val=VOLTAGE_FP(0); \
      	      }else if (val>VOLTAGE_FP(255)){                            \
        	      val=VOLTAGE_FP(255);                    \
  	          }                                                            \
              *(Out_P++)=Size(GetSize(&val)); \
              if (sound_num_channels==2){         \
                val=v + *(lp_dma_sound_channel+1);                                            \
                if (val<VOLTAGE_FP(0)){                                     \
                  val=VOLTAGE_FP(0); \
                }else if (val>VOLTAGE_FP(255)){                            \
                  val=VOLTAGE_FP(255);                    \
                }                                                            \
                *(Out_P++)=Size(GetSize(&val)); \
              }     \
    	        WAVEFORM_ONLY(temp_waveform_display[((int)(source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=WORD_B_1(&val)); \
  	          *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);                 \
              if (lp_dma_sound_channel<lp_max_dma_sound_channel) lp_dma_sound_channel+=2; \
      	      c--;                                                          \
	          }

#endif//ss


#endif

#define WRITE_TO_WAV_FILE_B(val) fputc(BYTE(WORD_B_1(&(val))),wav_file);
#define WRITE_TO_WAV_FILE_W(val) val^=MSB_W;fputc(LOBYTE(val),wav_file);fputc(HIBYTE(val),wav_file);

#define SOUND_RECORD(Alter_V,WRITE)         \
            while (c>0){                        \
              Alter_V;                          \
              val=v + *lp_dma_sound_channel;                           \
              if (val<VOLTAGE_FP(0)){                                     \
                val=VOLTAGE_FP(0); \
              }else if (val>VOLTAGE_FP(255)){                            \
                val=VOLTAGE_FP(255); \
              }                                                            \
              WRITE(val); \
              if (sound_num_channels==2){         \
                val=v + *(lp_dma_sound_channel+1);                           \
                if (val<VOLTAGE_FP(0)){                                     \
                  val=VOLTAGE_FP(0); \
                }else if (val>VOLTAGE_FP(255)){                            \
                  val=VOLTAGE_FP(255); \
                }                                                            \
                WRITE(val); \
              }  \
              source_p++;                       \
              SINE_ONLY( t++ );                                   \
              if (lp_dma_sound_channel<lp_max_dma_sound_channel) lp_dma_sound_channel+=2; \
              c--;                                          \
            }




#else // original macros

#ifdef ENABLE_VARIABLE_SOUND_DAMPING

#define CALC_V_CHIP  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p))*sound_variable_a >> 8;        \
                  dv*=sound_variable_d;                                           \
                  dv>>=8;                                           \
                }

#define CALC_V_CHIP_25KHZ  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p))*sound_variable_a >> 8;        \
                  dv*=sound_variable_d;                                           \
                  dv>>=8;                                           \
                }

#else

#define CALC_V_CHIP  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=(v-(*source_p)) >> 3;                         \
                  dv*=13;                                           \
                  dv>>=4;                                           \
                }

#define CALC_V_CHIP_25KHZ  \
                if (v!=*source_p || dv){                            \
                  v+=dv;                                            \
                  dv-=((v-(*source_p)) *3) >>3;                         \
                  dv*=3;                                           \
                  dv>>=2;                                           \
                }

//60, C0

#endif

#define CALC_V_EMU  v=*source_p;

#ifdef SHOW_WAVEFORM
  #define WAVEFORM_SET_VAL(v) (val=(v))
  #define WAVEFORM_ONLY(x) x
#else
  #define WAVEFORM_SET_VAL(v) v
  #define WAVEFORM_ONLY(x)
#endif



#ifdef WRITE_ONLY_SINE_WAVE

#define SINE_ONLY(s) s

#define WRITE_SOUND_LOOP(Alter_V)         \
	          while (c>0){                                                  \
                *(p++)=WAVEFORM_SET_VAL(BYTE(sin((double)t*(M_PI/64))*120+128)); \
                t++;                                                       \
                WAVEFORM_ONLY( temp_waveform_display[((int)(source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=(BYTE)val; ) \
    	          *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);                 \
                c--;    \
	          }

#else

#define SINE_ONLY(s)

#define WRITE_SOUND_LOOP(Alter_V,Out_P,Size,GetSize)         \
	          while (c>0){                                                  \
              Alter_V                                                     \
              val=v + *lp_dma_sound_channel;                           \
  	          if (val<VOLTAGE_FP(0)){                                     \
                val=VOLTAGE_FP(0); \
      	      }else if (val>VOLTAGE_FP(255)){                            \
        	      val=VOLTAGE_FP(255);                    \
  	          }                                                            \
              *(Out_P++)=Size(GetSize(&val)); \
              if (sound_num_channels==2){         \
                val=v + *(lp_dma_sound_channel+1);                                            \
                if (val<VOLTAGE_FP(0)){                                     \
                  val=VOLTAGE_FP(0); \
                }else if (val>VOLTAGE_FP(255)){                            \
                  val=VOLTAGE_FP(255);                    \
                }                                                            \
                *(Out_P++)=Size(GetSize(&val)); \
              }     \
    	        WAVEFORM_ONLY(temp_waveform_display[((int)(source_p-psg_channels_buf)+psg_time_of_last_vbl_for_writing) % MAX_temp_waveform_display_counter]=WORD_B_1(&val)); \
  	          *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL);                 \
              if (lp_dma_sound_channel<lp_max_dma_sound_channel) lp_dma_sound_channel+=2; \
      	      c--;                                                          \
	          }

#endif

#define WRITE_TO_WAV_FILE_B(val) fputc(BYTE(WORD_B_1(&(val))),wav_file);
#define WRITE_TO_WAV_FILE_W(val) val^=MSB_W;fputc(LOBYTE(val),wav_file);fputc(HIBYTE(val),wav_file);

#define SOUND_RECORD(Alter_V,WRITE)         \
            while (c>0){                        \
              Alter_V;                          \
              val=v + *lp_dma_sound_channel;                           \
              if (val<VOLTAGE_FP(0)){                                     \
                val=VOLTAGE_FP(0); \
              }else if (val>VOLTAGE_FP(255)){                            \
                val=VOLTAGE_FP(255); \
              }                                                            \
              WRITE(val); \
              if (sound_num_channels==2){         \
                val=v + *(lp_dma_sound_channel+1);                           \
                if (val<VOLTAGE_FP(0)){                                     \
                  val=VOLTAGE_FP(0); \
                }else if (val>VOLTAGE_FP(255)){                            \
                  val=VOLTAGE_FP(255); \
                }                                                            \
                WRITE(val); \
              }  \
              source_p++;                       \
              SINE_ONLY( t++ );                                   \
              if (lp_dma_sound_channel<lp_max_dma_sound_channel) lp_dma_sound_channel+=2; \
              c--;                                          \
            }

#endif//inline/macros
//---------------------------------------------------------------------------
void sound_record_to_wav(int c,DWORD SINE_ONLY(t),bool chipmode,int *source_p)
{
  if (timer<sound_record_start_time) return;

  int v=psg_voltage,dv=psg_dv; //restore from last time
  WORD *lp_dma_sound_channel=dma_sound_channel_buf;
  WORD *lp_max_dma_sound_channel=dma_sound_channel_buf+dma_sound_channel_buf_last_write_t;
  int val;
  if (sound_num_bits==8){
    if (chipmode){
      if (sound_low_quality==0){
        SOUND_RECORD(CALC_V_CHIP,WRITE_TO_WAV_FILE_B);
      }else{
        SOUND_RECORD(CALC_V_CHIP_25KHZ,WRITE_TO_WAV_FILE_B);
      }
    }else{
      SOUND_RECORD(CALC_V_EMU,WRITE_TO_WAV_FILE_B);
    }
  }else{
    if (chipmode){
      if (sound_low_quality==0){
        SOUND_RECORD(CALC_V_CHIP,WRITE_TO_WAV_FILE_W);
      }else{
        SOUND_RECORD(CALC_V_CHIP_25KHZ,WRITE_TO_WAV_FILE_W);
      }
    }else{
      SOUND_RECORD(CALC_V_EMU,WRITE_TO_WAV_FILE_W);
    }
  }
}


HRESULT Sound_VBL()
{
#if SCREENS_PER_SOUND_VBL != 1 //SS it is 1
  static int screens_countdown=SCREENS_PER_SOUND_VBL;
  screens_countdown--;if (screens_countdown>0) return DD_OK;
  screens_countdown=SCREENS_PER_SOUND_VBL;
  cpu_time_of_last_sound_vbl=ABSOLUTE_CPU_TIME;
#endif
  if (sound_internal_speaker){
    static double op=0;
    int abc,chan=-1,max_vol=0,vol;
    // Find loudest channel
    for (abc=0;abc<3;abc++){
      if ((psg_reg[PSGR_MIXER] & (1 << abc))==0){ // Channel enabled in mixer
        vol=(psg_reg[PSGR_AMPLITUDE_A+abc] & 15);
        if (vol>max_vol){
          chan=abc;
          max_vol=vol;
        }
      }
    }
    if (chan==-1){ //no sound
      internal_speaker_sound_by_period(0);
      op=0;
    }else{
      double p=((((int)psg_reg[chan*2+1] & 0xf) << 8) + psg_reg[chan*2]);
      p*=(1193181.0/125000.0);
      if (op!=p){
        op=p;
        internal_speaker_sound_by_period((int)p);
      }
    }
  }

  if (psg_capture_file){
    psg_capture_check_boundary();
  }

  // This just clears up some clicks when Sound_VBL is called very soon after Sound_Start
  if (sound_first_vbl){
    sound_first_vbl=0;
    return DS_OK;
  }

  if (sound_mode==SOUND_MODE_MUTE) return DS_OK;
  if (UseSound==0) return DSERR_GENERIC;  // Not initialised
  if (SoundActive()==0) return DS_OK;        // Not started

  log("");
  log("SOUND: Start of Sound_VBL");

  void *DatAdr[2]={NULL,NULL};
  DWORD LockLength[2]={0,0};
  DWORD s_time,write_time_1,write_time_2;
  HRESULT Ret;

  int *source_p;

  DWORD n_samples_per_vbl=(sound_freq*SCREENS_PER_SOUND_VBL)/shifter_freq;
  log(EasyStr("SOUND: Calculating time; psg_time_of_start_of_buffer=")+psg_time_of_start_of_buffer);


  s_time=SoundGetTime();
  //we have data from time_of_last_vbl+PSG_WRITE_N_SCREENS_AHEAD*n_samples_per_vbl up to
  //wherever we want

  write_time_1=psg_time_of_last_vbl_for_writing; //3 screens ahead of where the cursor was
//  write_time_1=max(write_time_1,min_write_time); //minimum time for new write

  write_time_2=max(write_time_1+(n_samples_per_vbl+PSG_WRITE_EXTRA),
                   s_time+(n_samples_per_vbl+PSG_WRITE_EXTRA));
  if ((write_time_2-write_time_1)>PSG_CHANNEL_BUF_LENGTH){
    write_time_2=write_time_1+PSG_CHANNEL_BUF_LENGTH;
  }

//  psg_last_write_time=write_time_2;

  DWORD time_of_next_vbl_to_write=max(s_time+n_samples_per_vbl*psg_write_n_screens_ahead,psg_time_of_next_vbl_for_writing);
  if (time_of_next_vbl_to_write>s_time+n_samples_per_vbl*(psg_write_n_screens_ahead+2)){  //
    time_of_next_vbl_to_write=s_time+n_samples_per_vbl*(psg_write_n_screens_ahead+2);     // new bit added by Ant 9/1/2001 to stop the sound lagging behind
  }                                                                                    // get rid of it if it is causing problems

  log(EasyStr("   writing from ")+write_time_1+" to "+write_time_2+"; current play cursor at "+s_time+" ("+play_cursor+"); minimum write at "+min_write_time+" ("+write_cursor+")");

//  log_write(EasyStr("writing ")+(write_time_1-s_time)+" samples ahead of play cursor, "+(write_time_1-min_write_time)+" ahead of min write");

#ifdef SHOW_WAVEFORM
  temp_waveform_display_counter=write_time_1 MOD_PSG_BUF_LENGTH;
  temp_waveform_play_counter=play_cursor;
#endif
  log("SOUND: Working out data up to the end of this VBL plus a bit more for all channels");
  for (int abc=2;abc>=0;abc--){
    psg_write_buffer(abc,time_of_next_vbl_to_write+PSG_WRITE_EXTRA);
  }
  if (dma_sound_on_this_screen){
    WORD w[2]={dma_sound_channel_buf[dma_sound_channel_buf_last_write_t-2],dma_sound_channel_buf[dma_sound_channel_buf_last_write_t-1]};
    for (int i=0;i<PSG_WRITE_EXTRA;i++){
      if (dma_sound_channel_buf_last_write_t>=DMA_SOUND_BUFFER_LENGTH) break;
      dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w[0];
      dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w[1];
    }
  }else{
    WORD w1,w2;
    dma_sound_get_last_sample(&w1,&w2);
    dma_sound_channel_buf[0]=w1;
    dma_sound_channel_buf[1]=w2;
    dma_sound_channel_buf_last_write_t=0;
  }

  // write_time_1 and 2 are sample variables, convert to bytes
  DWORD StartByte=(write_time_1 MOD_PSG_BUF_LENGTH)*sound_bytes_per_sample;
  DWORD NumBytes=((write_time_2-write_time_1)+1)*sound_bytes_per_sample;
  log(EasyStr("SOUND: Trying to lock from ")+StartByte+", length "+NumBytes);
  Ret=SoundLockBuffer(StartByte,NumBytes,&DatAdr[0],&LockLength[0],&DatAdr[1],&LockLength[1]);
  if (Ret!=DSERR_BUFFERLOST){
    if (Ret!=DS_OK){
      log_write("SOUND: Lock totally failed, disaster!");
      return SoundError("Lock for PSG Buffer Failed",Ret);
    }
    log(EasyStr("SOUND: Locked lengths ")+LockLength[0]+", "+LockLength[1]);
    int i=min(max(int(write_time_1-psg_time_of_last_vbl_for_writing),0),PSG_CHANNEL_BUF_LENGTH-10);
    int v=psg_voltage,dv=psg_dv; //restore from last time
    log(EasyStr("SOUND: Zeroing channels buffer up to ")+i);
    for (int j=0;j<i;j++){
      psg_channels_buf[j]=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL); //zero the start of the buffer
    }
    source_p=psg_channels_buf+i;
    int samples_left_in_buffer=max(PSG_CHANNEL_BUF_LENGTH-i,0);
    int countdown_to_storing_values=max((int)(time_of_next_vbl_to_write-write_time_1),0);
    //this is set when we are counting down to the start time of the next write
    bool store_values=false,chipmode=bool((sound_mode==SOUND_MODE_EMULATED) ? false:true);
    if (sound_mode==SOUND_MODE_SHARPSAMPLES) chipmode=(psg_reg[PSGR_MIXER] & b00111111)!=b00111111;
    if (sound_mode==SOUND_MODE_SHARPCHIP)    chipmode=(psg_reg[PSGR_MIXER] & b00111111)==b00111111;
    if (sound_record){
      sound_record_to_wav(countdown_to_storing_values,write_time_1,chipmode,source_p);
    }

#ifdef WRITE_ONLY_SINE_WAVE
    DWORD t=write_time_1;
#endif

    int val;
    log("SOUND: Starting to write to buffers");
    WORD *lp_dma_sound_channel=dma_sound_channel_buf;
    WORD *lp_max_dma_sound_channel=dma_sound_channel_buf+dma_sound_channel_buf_last_write_t;
    BYTE *pb;
    WORD *pw;
    for (int n=0;n<2;n++){
      if (DatAdr[n]){
        pb=(BYTE*)(DatAdr[n]);
        pw=(WORD*)(DatAdr[n]);
        int c=min(int(LockLength[n]/sound_bytes_per_sample),samples_left_in_buffer),oc=c;
        if (c>countdown_to_storing_values){
          c=countdown_to_storing_values;
          oc-=countdown_to_storing_values;
          store_values=true;
        }
        for (;;){
          if (sound_num_bits==8){
            if (chipmode){
              if (sound_low_quality==0){
                WRITE_SOUND_LOOP(CALC_V_CHIP,pb,BYTE,DWORD_B_1);
              }else{
                WRITE_SOUND_LOOP(CALC_V_CHIP_25KHZ,pb,BYTE,DWORD_B_1);
              }
            }else{
              WRITE_SOUND_LOOP(CALC_V_EMU,pb,BYTE,DWORD_B_1);
            }
          }else{
            if (chipmode){
              if (sound_low_quality==0){
                WRITE_SOUND_LOOP(CALC_V_CHIP,pw,WORD,MSB_W ^ DWORD_W_0);
              }else{
                WRITE_SOUND_LOOP(CALC_V_CHIP_25KHZ,pw,WORD,MSB_W ^ DWORD_W_0);
              }
            }else{
              WRITE_SOUND_LOOP(CALC_V_EMU,pw,WORD,MSB_W ^ DWORD_W_0);
            }
          }

          if (store_values){
            c=oc;
            psg_voltage=v;
            psg_dv=dv;
            store_values=false;
            countdown_to_storing_values=0x7fffffff; //don't store the values again.
          }else{
            countdown_to_storing_values-=LockLength[n]/sound_bytes_per_sample;
            break;
          }
        }
        samples_left_in_buffer-=LockLength[n]/sound_bytes_per_sample;
      }
    }

#if defined(STEVEN_SEAGAL) && defined(SSE_VID_RECORD_AVI) 
    if(video_recording&&SoundBuf&&pAviFile&&pAviFile->Initialised)
    {
      VERIFY( pAviFile->AppendSound(DatAdr[0],LockLength[0])==0 );
    }
#endif
    SoundUnlock(DatAdr[0],LockLength[0],DatAdr[1],LockLength[1]);
    while (source_p < (psg_channels_buf+PSG_CHANNEL_BUF_LENGTH)){
      *(source_p++)=VOLTAGE_FP(VOLTAGE_ZERO_LEVEL); //zero the rest of the buffer
    }
  }
  psg_buf_pointer[0]=0;
  psg_buf_pointer[1]=0;
  psg_buf_pointer[2]=0;

  psg_time_of_last_vbl_for_writing=time_of_next_vbl_to_write;

  psg_time_of_next_vbl_for_writing=max(s_time+n_samples_per_vbl*(psg_write_n_screens_ahead+1),
                                       time_of_next_vbl_to_write+n_samples_per_vbl);
  psg_time_of_next_vbl_for_writing=min(psg_time_of_next_vbl_for_writing,
                                       s_time+(PSG_BUF_LENGTH/2));
  log(EasyStr("SOUND: psg_time_of_next_vbl_for_writing=")+psg_time_of_next_vbl_for_writing);

  psg_n_samples_this_vbl=psg_time_of_next_vbl_for_writing-psg_time_of_last_vbl_for_writing;

  log("SOUND: End of Sound_VBL");
  log("");

  //TRACE("PSG mixer %X\n",psg_reg[PSGR_MIXER]);

  return DS_OK;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*                                DMA SOUND                                  */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void dma_sound_set_control(BYTE io_src_b)
{
/*
  DMA-Sound Control Register:

    $FFFF8900  0 0 0 0 0 0 0 0 0 0 0 0 0 0 X X

Writing a "00" to the last 2 bits terminate DMA sound replay.

Bit 0 controls Replay off/on, Bit 1 controls Loop off/on (0=off, 1=on).
*/
  TRACE_LOG("DMA sound ");
  if ((dma_sound_control & BIT_0) && (io_src_b & BIT_0)==0){  //Stopping
    TRACE_LOG("stop");
    dma_sound_start=next_dma_sound_start;
    dma_sound_end=next_dma_sound_end;
    dma_sound_fetch_address=dma_sound_start; //SS IO registers
  }else if ((dma_sound_control & BIT_0)==0 && (io_src_b & BIT_0)){ //Start playing
    TRACE_LOG("start ");
    dma_sound_start=next_dma_sound_start;
    dma_sound_end=next_dma_sound_end;
    dma_sound_fetch_address=dma_sound_start;
    TRACE_LOG("frame %x-%x ",dma_sound_start,dma_sound_end);
    if (dma_sound_on_this_screen==0){
      // Pad buffer with last byte from VBL to current position
      bool Mono=bool(dma_sound_mode & BIT_7);
      TRACE_LOG((Mono) ? (char*)"Mono":(char*)"Stereo");
      int freq_idx=0;
      if (shifter_freq_at_start_of_vbl==60) freq_idx=1;
      if (shifter_freq_at_start_of_vbl==MONO_HZ) freq_idx=2;
      WORD w1,w2;
      dma_sound_get_last_sample(&w1,&w2);
      for (int y=-scanlines_above_screen[freq_idx];y<scan_y;y++){
        if (Mono){  //play half as many words
          dma_sound_samples_countdown+=dma_sound_freq*scanline_time_in_cpu_cycles_at_start_of_vbl/2;
        }else{ //stereo, 1 word per sample
          dma_sound_samples_countdown+=dma_sound_freq*scanline_time_in_cpu_cycles_at_start_of_vbl;
        }
        int loop=int(Mono ? 2:1);
        while (dma_sound_samples_countdown>=0){
          for (int i=0;i<loop;i++){
            dma_sound_output_countdown+=sound_freq;
            while (dma_sound_output_countdown>=0){
              if (dma_sound_channel_buf_last_write_t>=DMA_SOUND_BUFFER_LENGTH) break;
              dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w1;
              dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w2;
              dma_sound_output_countdown-=dma_sound_freq;
            }
          }
          dma_sound_samples_countdown-=n_cpu_cycles_per_second;
        }
      }
      dma_sound_on_this_screen=1;
    }
  }
  TRACE_LOG(" Freq %d\n",dma_sound_freq);
  log_to(LOGSECTION_SOUND,EasyStr("SOUND: ")+HEXSl(old_pc,6)+" - DMA sound control set to "+(io_src_b & 3)+" from "+(dma_sound_control & 3));
 
#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_VOL)
  ASSERT(!(io_src_b&~3)); // Sadeness
  io_src_b&=3;
#endif

  dma_sound_control=io_src_b;
  if (tos_version>=0x106) mfp_gpip_set_bit(MFP_GPIP_MONO_BIT,bool(COLOUR_MONITOR)^bool(dma_sound_control & BIT_0));
}
//---------------------------------------------------------------------------
void dma_sound_set_mode(BYTE new_mode)
{
/*
  DMA-Soundmode Register:

    $FFFF8920  0 0 0 0 0 0 0 0 X 0 0 0 X X X X

  Allows to toggle between several replay methods. Bit 7 switches Mono/Stereo 
  (1 = Mono, 0 = Stereo), Bit 0 and 1 encode the replay rate:
      0 0  -  6258 Hz
      0 1  - 12517 Hz
      1 0  - 25033 Hz
      1 1  - 50066 Hz
  01 Cool STE, Delirious 4 loaders
  80 RGBeast

  The ST-E's stereo sound allows playback at fixed sampling frequencies only
  (6.25 KHz, 12.5 KHz, 25 KHz and 50 KHz), 
  playing a sample back at different frequencies than it was recorded at 
  needs the CPU to stretch or shrink the sample data
  Amiga was more flexible
  Also, the ST-E's stereo output is purely a 2 channel system, so playing 
  more than 2 samples together (for example, playing a MOD file) requires 
  the CPU to mix the sample information together.
*/

#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND)
//  ASSERT(!(new_mode&~0x8F));
  new_mode&=0x8F;
  TRACE_LOG("DMA sound mode %X freq %d\n",new_mode,dma_sound_mode_to_freq[new_mode & 3]);
#endif

  dma_sound_mode=new_mode;
  dma_sound_freq=dma_sound_mode_to_freq[dma_sound_mode & 3];
#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_MICROWIRE)
  SampleRate=dma_sound_freq; // global of 3rd party dsp
#endif
  log_to(LOGSECTION_SOUND,EasyStr("SOUND: ")+HEXSl(old_pc,6)+" - DMA sound mode set to $"+HEXSl(dma_sound_mode,2)+" freq="+dma_sound_freq);
}
//---------------------------------------------------------------------------
void dma_sound_fetch()
{
/*
  During horizontal blanking (transparent to the processor) the DMA sound
  chip fetches samples from memory and provides them to a digital-to-analog
  converter (DAC) at one of several constant rates, programmable as 
  (approximately) 50KHz (kilohertz), 25KHz, 12.5KHz, and 6.25KHz. 
  This rate is called the sample frequency. 

   Each sample is stored as a signed eight-bit quantity, where -128 (80 hex) 
   means full negative displacement of the speaker, and 127 (7F hex) means full 
   positive displacement. In stereo mode, each word represents two samples: the 
   upper byte is the sample for the left channel, and the lower byte is the 
   sample for the right channel. In mono mode each byte is one sample. However, 
   the samples are always fetched a word at a time, so only an even number of 
   mono samples can be played. 

   This function is called by event_hbl() & event_scanline(), which matches the
   hblank timing.
   It sends some samples to the rendering buffer, then fill up the FIFO buffer.

   A possible problem in Steem (vs. Hatari) is that there's no independent clock
   for DMA sound, it's based on CPU speed.
*/
  bool Playing=bool(dma_sound_control & BIT_0);
  bool Mono=bool(dma_sound_mode & BIT_7);

  //we want to play a/b samples, where a is the DMA sound frequency
  //and b is the number of scanlines a second

  int left_vol_top_val=dma_sound_l_top_val,right_vol_top_val=dma_sound_r_top_val;
  //this a/b is the same as dma_sound_freq*scanline_time_in_cpu_cycles/8million

/*  SS
    About the a/b thing...
    It works this way with the 'hardware test' of ijbk, and if we change it it
    doesn't anymore but:

    At 50hz, scanline_time_in_cpu_cycles_at_start_of_vbl = 512
    #scanlines = 313x50 = 15650

    freq/15650 = freq*512/8000000 ?

    Then we would have: 8000000/15650 = 512
    Yet 8000000/15650 = 15625
*/

  if (Mono){  //play half as many words
    dma_sound_samples_countdown+=dma_sound_freq*scanline_time_in_cpu_cycles_at_start_of_vbl/2;
    left_vol_top_val=(dma_sound_l_top_val >> 1)+(dma_sound_r_top_val >> 1);
    right_vol_top_val=left_vol_top_val;
  }else{ //stereo, 1 word per sample
    dma_sound_samples_countdown+=dma_sound_freq*scanline_time_in_cpu_cycles_at_start_of_vbl;
  }
  bool vol_change_l=(left_vol_top_val<128),vol_change_r=(right_vol_top_val<128);

  while (dma_sound_samples_countdown>=0){
    //play word from buffer
    if (dma_sound_internal_buf_len>0){
      dma_sound_last_word=dma_sound_internal_buf[0];
      for (int i=0;i<3;i++) dma_sound_internal_buf[i]=dma_sound_internal_buf[i+1];
      dma_sound_internal_buf_len--;
      if (vol_change_l){
        int b1=(signed char)(HIBYTE(dma_sound_last_word));
        b1*=left_vol_top_val;
        b1/=128;
        dma_sound_last_word&=0x00ff;
        dma_sound_last_word|=WORD(BYTE(b1) << 8);
      }
      if (vol_change_r){
        int b2=(signed char)(LOBYTE(dma_sound_last_word));
        b2*=right_vol_top_val;
        b2/=128;
        dma_sound_last_word&=0xff00;
        dma_sound_last_word|=BYTE(b2);
      }
      dma_sound_last_word^=WORD((128 << 8) | 128); // unsign
    }
    dma_sound_output_countdown+=sound_freq;
    WORD w1;
    WORD w2;
    if (Mono){       //mono, play half as many words
      w1=WORD((dma_sound_last_word & 0xff00) >> 2);
      w2=WORD((dma_sound_last_word & 0x00ff) << 6);
      // dma_sound_channel_buf always stereo, so put each mono sample in twice
      while (dma_sound_output_countdown>=0){
        if (dma_sound_channel_buf_last_write_t>=DMA_SOUND_BUFFER_LENGTH) break;
        dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w1;
        dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w1;
        dma_sound_output_countdown-=dma_sound_freq;
      }
      dma_sound_output_countdown+=sound_freq;
      while (dma_sound_output_countdown>=0){
        if (dma_sound_channel_buf_last_write_t>=DMA_SOUND_BUFFER_LENGTH) break;
        dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w2;
        dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w2;
        dma_sound_output_countdown-=dma_sound_freq;
      }
    }else{//stereo , 1 word per sample
/*
 Stereo Samples have to be organized wordwise like 
 Lowbyte -> right channel
 Hibyte  -> left channel
*/
      if (sound_num_channels==1){
        //average the channels out
        w1=WORD(((dma_sound_last_word & 255)+(dma_sound_last_word >> 8)) << 5);
        w2=0; // skipped
      }else{
        w1=WORD((dma_sound_last_word & 0xff00) >> 2);
        w2=WORD((dma_sound_last_word & 0x00ff) << 6);
      }
      
      while (dma_sound_output_countdown>=0){
        if (dma_sound_channel_buf_last_write_t>=DMA_SOUND_BUFFER_LENGTH) break;

#if defined (STEVEN_SEAGAL) && defined(SSE_SOUND_FILTER_STE_________)
        // exactly the same low-pass filter as for STF sound
        // ->3.6.3 it filters but also pollutes the sound, MFD
        if(MICROWIRE_ON
          &&PSG_FILTER_FIX //for v3.6.3 tests
          &&dma_sound_channel_buf_last_write_t>3)
        {
          int tmp=(w1+
            + dma_sound_channel_buf[dma_sound_channel_buf_last_write_t-2])/2;
          dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=tmp;
          tmp=(w2+
            + dma_sound_channel_buf[dma_sound_channel_buf_last_write_t-2])/2;
          dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=tmp;
        }
        else
        {
          dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w1;
          dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w2;
        }
        dma_sound_output_countdown-=dma_sound_freq;
#else
        dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w1;
        dma_sound_channel_buf[dma_sound_channel_buf_last_write_t++]=w2;
        dma_sound_output_countdown-=dma_sound_freq;
#endif

      }
    }
#ifdef TEST01
    if(SSE_TEST_ON)
      dma_sound_samples_countdown-=STE_DMA_CLOCK;
    else
#endif
    dma_sound_samples_countdown-=n_cpu_cycles_per_second; 
                            //SS putting back 8000000 sounds worse

  }//while (dma_sound_samples_countdown>=0)

  if (Playing==0) return;
  if (dma_sound_internal_buf_len>=4) return;
  if (dma_sound_fetch_address>=himem) return;

  //SS fill FIFO with up to 8 bytes

  for (int i=0;i<4;i++){
    if (dma_sound_fetch_address>=dma_sound_end){
/*
       SS reset loop - immediate?
 A group of samples is called a "frame." A frame may be played once or can 
 automatically be repeated forever (until stopped). A frame is described by 
 its start and end addresses. The end address of a frame is actually the 
 address of the first byte in memory beyond the frame; a frame starting at 
 address 21100 which is 10 bytes long has an end address of 21110. 
 -> this seems OK in Steem
*/

      TRACE_LOG("DMA sound reset loop %X->%X\n",next_dma_sound_start,next_dma_sound_end);

      dma_sound_start=next_dma_sound_start;
      dma_sound_end=next_dma_sound_end;
      dma_sound_fetch_address=dma_sound_start;
      dma_sound_control&=~BIT_0;

      DMA_SOUND_CHECK_TIMER_A
      if (tos_version>=0x106) mfp_gpip_set_bit(MFP_GPIP_MONO_BIT,bool(COLOUR_MONITOR)^bool(dma_sound_control & BIT_0));

      if (dma_sound_control & BIT_1){
        dma_sound_control|=BIT_0; //Playing again immediately
        if (tos_version>=0x106) mfp_gpip_set_bit(MFP_GPIP_MONO_BIT,bool(COLOUR_MONITOR)^bool(dma_sound_control & BIT_0));
      }else{
        break;
      }
    }
    dma_sound_internal_buf[dma_sound_internal_buf_len++]=DPEEK(dma_sound_fetch_address);
    dma_sound_fetch_address+=2;
    if (dma_sound_internal_buf_len>=4) break;
  }//nxt
  TRACE_LOG("Y%d DMA frame counter %X\n",scan_y,dma_sound_fetch_address);
}
//---------------------------------------------------------------------------
void dma_sound_get_last_sample(WORD *pw1,WORD *pw2)
{
  if (dma_sound_mode & BIT_7){
    // ST plays HIBYTE, LOBYTE, so last sample is LOBYTE
    *pw1=WORD((dma_sound_last_word & 0x00ff) << 6);
    *pw2=*pw1; // play the same in both channels, or ignored in when sound_num_channels==1
  }else{
    if (sound_num_channels==1){
      //average the channels out
      *pw1=WORD(((dma_sound_last_word & 255)+(dma_sound_last_word >> 8)) << 5);
      *pw2=0; // skipped
    }else{
      *pw1=WORD((dma_sound_last_word & 0xff00) >> 2);
      *pw2=WORD((dma_sound_last_word & 0x00ff) << 6);
    }
  }
}
//---------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*                                PSG SOUND                                  */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#if !defined(STEVEN_SEAGAL) //not used
#define PSG_CALC_VOLTAGE_ENVELOPE                                     \
      {																																	\
        envstage=(t64-est64)/envmodulo2;           \
        if (envstage>=32 && envdeath!=-1){                           \
          *(p++)+=envdeath;                                             \
        }else{                                                       \
          *(p++)+=psg_envelope_level[envshape][envstage & 63];            \
        }																															\
      }
#endif

#define PSG_PULSE_NOISE(ntn) (psg_noise[(ntn) MOD_PSG_NOISE_ARRAY] )
#define PSG_PULSE_TONE  ((t*128 / psg_tonemodulo) & 1)
#define PSG_PULSE_TONE_t64  ((t*64 / psg_tonemodulo_2) & 1)


//#if defined(STEVEN_SEAGAL) && defined(SSE_SOUND_INLINE2)
/*  Second round of inlining
    Necessary for SSE_YM2149_DELAY_RENDERING
*/

#if defined(SSE_SOUND_INLINE2A)

void psg_prepare_envelope(double &af,double &bf,int &psg_envmodulo,DWORD t,
  int &psg_envstage,int &psg_envcountdown,int &envdeath,int &envshape,int &envvol) {
      //int envperiod=1|psg_reg[PSGR_ENVELOPE_PERIOD_LOW]+((psg_reg[PSGR_ENVELOPE_PERIOD_HIGH]) <<8);//buggy!
    int envperiod=max( (((int)psg_reg[PSGR_ENVELOPE_PERIOD_HIGH]) <<8) + psg_reg[PSGR_ENVELOPE_PERIOD_LOW],1);
      af=envperiod;
      af*=sound_freq;                  
      af*=((double)(1<<13))/15625;                               
      psg_envmodulo=(int)af; 
      bf=(((DWORD)t)-psg_envelope_start_time); 
      bf*=(double)(1<<17); 
      psg_envstage=(int)floor(bf/af); 
      bf=fmod(bf,af); /*remainder*/ 
      psg_envcountdown=psg_envmodulo-(int)bf; 
      envdeath=-1;                                                                  
      if ((psg_reg[PSGR_ENVELOPE_SHAPE] & PSG_ENV_SHAPE_CONT)==0 ||                  
           (psg_reg[PSGR_ENVELOPE_SHAPE] & PSG_ENV_SHAPE_HOLD)){                     
        if(psg_reg[PSGR_ENVELOPE_SHAPE]==11 || psg_reg[PSGR_ENVELOPE_SHAPE]==13){      
#if defined(SSE_YM2149_DELAY_RENDERING)  
          envdeath=(SSE_OPTION_PSG)?15:psg_flat_volume_level[15];                                           
#else
          envdeath=psg_flat_volume_level[15];                                           
#endif
        }else{       
#if defined(SSE_YM2149_DELAY_RENDERING)  
          envdeath=(SSE_OPTION_PSG)?0:psg_flat_volume_level[0];                                       
#else                                                                    
          envdeath=psg_flat_volume_level[0];     
#endif                                         
        }                                                                                   
      }                                                                                      
      envshape=psg_reg[PSGR_ENVELOPE_SHAPE] & 7;                    
      if (psg_envstage>=32 && envdeath!=-1){                           
        envvol=envdeath;                                             
      }else{        
#if defined(SSE_YM2149_DELAY_RENDERING)  
        envvol=(SSE_OPTION_PSG) 
          ? psg_envelope_level3[envshape][psg_envstage & 63]
          : psg_envelope_level[envshape][psg_envstage & 63] ;                   

#elif defined(SSE_YM2149_ENV_FIX1) //no                      
        envvol=SSEOption.PSGMod?psg_envelope_level2[envshape][psg_envstage & 63]      
          :psg_envelope_level[envshape][psg_envstage & 63];
#else
        envvol=psg_envelope_level[envshape][psg_envstage & 63];

#endif
      }																															\
}

#define PSG_PREPARE_ENVELOPE psg_prepare_envelope(af,bf,psg_envmodulo,t,psg_envstage,psg_envcountdown,envdeath,envshape,envvol);
// double &af,double &bf,int &psg_envmodulo,DWORD t,
//  int &psg_envstage,int &psg_envcountdown,int &envdeath,int &envshape,int &envvol


#else

#if defined(SSE_YM2149_ENV_FIX1)
#define PSG_PREPARE_ENVELOPE                                \
      int envperiod=max( (((int)psg_reg[PSGR_ENVELOPE_PERIOD_HIGH]) <<8) + psg_reg[PSGR_ENVELOPE_PERIOD_LOW],1);  \
      af=envperiod;                              \
      af*=sound_freq;                  \
      af*=((double)(1<<13))/15625;                               \
      psg_envmodulo=(int)af; \
      bf=(((DWORD)t)-psg_envelope_start_time); \
      bf*=(double)(1<<17); \
      psg_envstage=(int)floor(bf/af); \
      bf=fmod(bf,af); /*remainder*/ \
      psg_envcountdown=psg_envmodulo-(int)bf; \
      envdeath=-1;                                                                  \
      if ((psg_reg[PSGR_ENVELOPE_SHAPE] & PSG_ENV_SHAPE_CONT)==0 ||                  \
           (psg_reg[PSGR_ENVELOPE_SHAPE] & PSG_ENV_SHAPE_HOLD)){                      \
        if(psg_reg[PSGR_ENVELOPE_SHAPE]==11 || psg_reg[PSGR_ENVELOPE_SHAPE]==13){      \
          envdeath=psg_flat_volume_level[15];                                           \
        }else{                                                                           \
          envdeath=psg_flat_volume_level[0];                                              \
        }                                                                                   \
      }                                                                                      \
      envshape=psg_reg[PSGR_ENVELOPE_SHAPE] & 7;                    \
      if (psg_envstage>=32 && envdeath!=-1){                           \
        envvol=envdeath;                                             \
      }else{                                                       \
        envvol=SSEOption.PSGMod?psg_envelope_level2[envshape][psg_envstage & 63]            \
          :psg_envelope_level[envshape][psg_envstage & 63];\
      }																															\

#else
#define PSG_PREPARE_ENVELOPE                                \
      int envperiod=max( (((int)psg_reg[PSGR_ENVELOPE_PERIOD_HIGH]) <<8) + psg_reg[PSGR_ENVELOPE_PERIOD_LOW],1);  \
      af=envperiod;                              \
      af*=sound_freq;                  \
      af*=((double)(1<<13))/15625;                               \
      psg_envmodulo=(int)af; \
      bf=(((DWORD)t)-psg_envelope_start_time); \
      bf*=(double)(1<<17); \
      psg_envstage=(int)floor(bf/af); \
      bf=fmod(bf,af); /*remainder*/ \
      psg_envcountdown=psg_envmodulo-(int)bf; \
      envdeath=-1;                                                                  \
      if ((psg_reg[PSGR_ENVELOPE_SHAPE] & PSG_ENV_SHAPE_CONT)==0 ||                  \
           (psg_reg[PSGR_ENVELOPE_SHAPE] & PSG_ENV_SHAPE_HOLD)){                      \
        if(psg_reg[PSGR_ENVELOPE_SHAPE]==11 || psg_reg[PSGR_ENVELOPE_SHAPE]==13){      \
          envdeath=psg_flat_volume_level[15];                                           \
        }else{                                                                           \
          envdeath=psg_flat_volume_level[0];                                              \
        }                                                                                   \
      }                                                                                      \
      envshape=psg_reg[PSGR_ENVELOPE_SHAPE] & 7;                    \
      if (psg_envstage>=32 && envdeath!=-1){                           \
        envvol=envdeath;                                             \
      }else{                                                       \
        envvol=psg_envelope_level[envshape][psg_envstage & 63];            \
      }																															\

#endif

#endif//a


#if defined(SSE_SOUND_INLINE2B)

void psg_prepare_noise(double &af,double &bf,int &psg_noisemodulo,DWORD t,
    int &psg_noisecountdown, int &psg_noisecounter,bool &psg_noisetoggle) {

      int noiseperiod=(psg_reg[PSGR_NOISE_PERIOD]&0x1f)|1;      
      af=((int)noiseperiod*sound_freq);                              
      af*=((double)(1<<17))/15625; 
      psg_noisemodulo=(int)af; 
      bf=t; \
      bf*=(double)(1<<20); 
      psg_noisecounter=(int)floor(bf/af); 
      psg_noisecounter &= (PSG_NOISE_ARRAY-1); 
      bf=fmod(bf,af); 
      psg_noisecountdown=psg_noisemodulo-(int)bf; 
      psg_noisetoggle=psg_noise[psg_noisecounter];

}
#define PSG_PREPARE_NOISE psg_prepare_noise(af,bf,psg_noisemodulo,t,psg_noisecountdown,psg_noisecounter,psg_noisetoggle);

#else

#define PSG_PREPARE_NOISE                                \
      int noiseperiod=(1+(psg_reg[PSGR_NOISE_PERIOD]&0x1f));      \
      af=((int)noiseperiod*sound_freq);                              \
      af*=((double)(1<<17))/15625; \
      psg_noisemodulo=(int)af; \
      bf=t; \
      bf*=(double)(1<<20); \
      psg_noisecounter=(int)floor(bf/af); \
      psg_noisecounter &= (PSG_NOISE_ARRAY-1); \
      bf=fmod(bf,af); \
      psg_noisecountdown=psg_noisemodulo-(int)bf; \
      psg_noisetoggle=psg_noise[psg_noisecounter];

      /*
      if (abc==0) log_write(Str("toneperiod=")+toneperiod+" sound_freq="+sound_freq+" psg_tonemodulo_2="+psg_tonemodulo_2); \
      */

#endif//b

#if defined(SSE_SOUND_INLINE2C)

void psg_prepare_tone(int toneperiod,double &af,double &bf,
                      int &psg_tonemodulo_2,int abc,DWORD t,
                      int &psg_tonecountdown,bool &psg_tonetoggle) {

      af=((int)toneperiod*sound_freq);                              \
      af*=((double)(1<<17))/15625;                               \
      psg_tonemodulo_2=(int)af; \
      bf=(((DWORD)t)-psg_tone_start_time[abc]); \
      bf*=(double)(1<<21); \
      bf=fmod(bf,af*2); \
      af=bf-af;               \
      if(af>=0){                  \
        psg_tonetoggle=false;       \
        bf=af;                      \
      }                           \
      psg_tonecountdown=psg_tonemodulo_2-(int)bf; \
}

#define PSG_PREPARE_TONE psg_prepare_tone(toneperiod,af,bf,psg_tonemodulo_2,abc,t,psg_tonecountdown,psg_tonetoggle);

#else

#define PSG_PREPARE_TONE                                 \
      af=((int)toneperiod*sound_freq);                              \
      af*=((double)(1<<17))/15625;                               \
      psg_tonemodulo_2=(int)af; \
      bf=(((DWORD)t)-psg_tone_start_time[abc]); \
      bf*=(double)(1<<21); \
      bf=fmod(bf,af*2); \
      af=bf-af;               \
      if(af>=0){                  \
        psg_tonetoggle=false;       \
        bf=af;                      \
      }                           \
      psg_tonecountdown=psg_tonemodulo_2-(int)bf; \


#endif//c


#if defined(SSE_SOUND_INLINE2D)

void psg_envelope_advance(int &psg_envmodulo,int &psg_envstage,int &psg_envcountdown,int &envdeath,int &envshape,int &envvol) {

          psg_envcountdown-=TWO_TO_SEVENTEEN; //  131072
          while (psg_envcountdown<0){           \
            psg_envcountdown+=psg_envmodulo;             \
            psg_envstage++;                   \
            if (psg_envstage>=32 && envdeath!=-1){                           \
              envvol=envdeath;                                             \
            }else{   
#if defined(SSE_YM2149_DELAY_RENDERING)  
        envvol=(SSE_OPTION_PSG) 
          ? psg_envelope_level3[envshape][psg_envstage & 63]
          : psg_envelope_level[envshape][psg_envstage & 63] ;                   
                 
#elif defined(SSE_YM2149_ENV_FIX1)
              envvol=(SSEOption.PSGMod)?psg_envelope_level2[envshape][psg_envstage & 63]            \
                :psg_envelope_level[envshape][psg_envstage & 63];
#else                                                    
              envvol=psg_envelope_level[envshape][psg_envstage & 63];
#endif
            }																															\
          }
}

#define PSG_ENVELOPE_ADVANCE  psg_envelope_advance(psg_envmodulo,psg_envstage,psg_envcountdown,envdeath,envshape,envvol);


#else

#if defined(SSE_YM2149_ENV_FIX1)

#define PSG_ENVELOPE_ADVANCE                                   \
          psg_envcountdown-=TWO_TO_SEVENTEEN;  \
          while (psg_envcountdown<0){           \
            psg_envcountdown+=psg_envmodulo;             \
            psg_envstage++;                   \
            if (psg_envstage>=32 && envdeath!=-1){                           \
              envvol=envdeath;                                             \
            }else{                                                       \
              envvol=(SSEOption.PSGMod)?psg_envelope_level2[envshape][psg_envstage & 63]            \
                :psg_envelope_level[envshape][psg_envstage & 63];\
            }																															\
          }
#else

#define PSG_ENVELOPE_ADVANCE                                   \
          psg_envcountdown-=TWO_TO_SEVENTEEN;  \
          while (psg_envcountdown<0){           \
            psg_envcountdown+=psg_envmodulo;             \
            psg_envstage++;                   \
            if (psg_envstage>=32 && envdeath!=-1){                           \
              envvol=envdeath;                                             \
            }else{                                                       \
              envvol=psg_envelope_level[envshape][psg_envstage & 63];            \
            }																															\
          }
#endif

  //            envvol=(psg_envstage&255)*64;


#endif//d


#if defined(SSE_SOUND_INLINE2E)

void psg_tone_advance(int psg_tonemodulo_2,int &psg_tonecountdown,bool &psg_tonetoggle) {

          psg_tonecountdown-=TWO_MILLION;  \
          while (psg_tonecountdown<0){           \
            psg_tonecountdown+=psg_tonemodulo_2;             \
            psg_tonetoggle=!psg_tonetoggle;                   \
          }
}

#define PSG_TONE_ADVANCE psg_tone_advance(psg_tonemodulo_2,psg_tonecountdown,psg_tonetoggle);

#else

#define PSG_TONE_ADVANCE                                   \
          psg_tonecountdown-=TWO_MILLION;  \
          while (psg_tonecountdown<0){           \
            psg_tonecountdown+=psg_tonemodulo_2;             \
            psg_tonetoggle=!psg_tonetoggle;                   \
          }

#endif//e


#if defined(SSE_SOUND_INLINE2F)

void psg_noise_advance(int psg_noisemodulo,int &psg_noisecountdown,int &psg_noisecounter,bool &psg_noisetoggle) {
          psg_noisecountdown-=ONE_MILLION;   \
          while (psg_noisecountdown<0){   \
            psg_noisecountdown+=psg_noisemodulo;      \
            psg_noisecounter++;                        \
            if(psg_noisecounter>=PSG_NOISE_ARRAY){      \
              psg_noisecounter=0;                        \
            }                                             \
            psg_noisetoggle=psg_noise[psg_noisecounter];   \
          }
}

#define PSG_NOISE_ADVANCE  psg_noise_advance(psg_noisemodulo,psg_noisecountdown,psg_noisecounter,psg_noisetoggle);

#else

#define PSG_NOISE_ADVANCE                           \
          psg_noisecountdown-=ONE_MILLION;   \
          while (psg_noisecountdown<0){   \
            psg_noisecountdown+=psg_noisemodulo;      \
            psg_noisecounter++;                        \
            if(psg_noisecounter>=PSG_NOISE_ARRAY){      \
              psg_noisecounter=0;                        \
            }                                             \
            psg_noisetoggle=psg_noise[psg_noisecounter];   \
          }

#endif//f


//#else//!inline2













//#endif//inline2?


/*  SS:This function renders one PSG channel until timing to_t.
    v3.7.0, when option 'P.S.G.' is checked, rendering is delayed
    until VBL time so that the unique table for all channels may be
    used instead. Digital volume is written, as well as on/off info
    and fixed/envelope.
*/

void psg_write_buffer(int abc,DWORD to_t)
{

#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS)
#if SSE_VERSION>=370 // C<->A
  if( (4>>abc) & (d2_dpeek(FAKE_IO_START+20)>>12 ))
#else
  if( (1<<abc) & (d2_dpeek(FAKE_IO_START+20)>>12 ))
#endif
    return; // skip this channel
#endif

  //buffer starts at time time_of_last_vbl
  //we've written up to psg_buf_pointer[abc]
  //so start at pointer and write to to_t,
#ifdef TEST01
  int psg_tonemodulo_2=0,psg_noisemodulo=0;
  int psg_tonecountdown=0,psg_noisecountdown=0;
  int psg_noisecounter=0;
  double af=0,bf=0;

#else
  int psg_tonemodulo_2,psg_noisemodulo;
  int psg_tonecountdown,psg_noisecountdown;
  int psg_noisecounter;
  double af,bf;
#endif
  bool psg_tonetoggle=true,psg_noisetoggle
#ifdef TEST01
  //  =true
#endif
    ;
  int *p=psg_channels_buf+psg_buf_pointer[abc];
  DWORD t=(psg_time_of_last_vbl_for_writing+psg_buf_pointer[abc]);//SS where we are now
  to_t=max(to_t,t);//SS can't go backwards
  to_t=min(to_t,psg_time_of_last_vbl_for_writing+PSG_CHANNEL_BUF_LENGTH);//SS don't exceed buffer
  int count=max(min((int)(to_t-t),PSG_CHANNEL_BUF_LENGTH-psg_buf_pointer[abc]),0);//SS don't exceed buffer
  ASSERT( count>=0 );
#if defined(SSE_YM2149_OPT1)
  if(!count)
    return;
#endif
  int toneperiod=(((int)psg_reg[abc*2+1] & 0xf) << 8) + psg_reg[abc*2];

  if ((psg_reg[abc+8] & BIT_4)==0){ // Not Enveloped //SS bit 'M' in those registers

#if defined(SSE_YM2149_FIXED_VOL_TABLE) && !defined(SSE_YM2149_NO_SAMPLES_OPTION)
/*  One unique volume. It's possible because we sync rendering in case
    of sample playing (see below).
    v3.7.0 because the table is used all the time, this block isn't
    defined anymore.
*/
    int vol;
#if defined(SSE_YM2149_FIXED_VOL_FIX1)
      vol=SSEOption.PSGMod?psg_flat_volume_level2[psg_reg[8+abc] & 15]:
        psg_flat_volume_level[psg_reg[8+abc] & 15];
#else
      vol=psg_flat_volume_level[psg_reg[abc+8] & 15];
#endif
#else
#if defined(SSE_YM2149_FIXED_VOL_FIX1)
    int vol=SSEOption.PSGMod?psg_flat_volume_level2[psg_reg[8+abc] & 15]:
        psg_flat_volume_level[psg_reg[8+abc] & 15];
#else//Steem 3.2; vol will be used only if option 'P.S.G.' isn't checked
    int vol=psg_flat_volume_level[psg_reg[abc+8] & 15];
#endif
#endif

    if ((psg_reg[PSGR_MIXER] & (1 << abc))==0 && (toneperiod>9)){ //tone enabled
      PSG_PREPARE_TONE
      if (
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS_NOISE)
        !((1<<11)&d2_dpeek(FAKE_IO_START+20)) &&
#endif
        (psg_reg[PSGR_MIXER] & (8 << abc))==0){ //noise enabled

        PSG_PREPARE_NOISE
        for (;count>0;count--){
#if defined(SSE_YM2149_DELAY_RENDERING)
/*  We don't render (compute volume) here, we record digital volume
    instead if option 'P.S.G.' is checked
    Volume is coded on 5 bit (fixed volume is shifted)


      if( ! ((*source_p)&BIT_5) )
        interpolate[abc]=index[abc]=0;


*/
          if(SSE_OPTION_PSG)
          {
            int t=0;
            if(psg_tonetoggle || psg_noisetoggle)
              ; // mimic Steem's way
            else
              t|=(psg_reg[abc+8] & 15)<<1; // vol 4bit
            t<<=8*abc;
            *p|=t;
            p++;
          }else
#endif // Steem 3.2's immediate rendering
          if(psg_tonetoggle || psg_noisetoggle){
            p++;
          }else{
            *(p++)+=vol;
          }
          PSG_TONE_ADVANCE
          PSG_NOISE_ADVANCE
        }
      }else{ //tone only
        for (;count>0;count--){
#if defined(SSE_YM2149_DELAY_RENDERING)
          if(SSE_OPTION_PSG)
          {
            int t=0;
            if(psg_tonetoggle)
              ;
            else
              t|=(psg_reg[abc+8] & 15)<<1; // vol 4bit
            t<<=8*abc;
            *p|=t;
            p++;
          } else
#endif
          if(psg_tonetoggle){
            p++;
          }else{
            *(p++)+=vol;
          }
          PSG_TONE_ADVANCE
        }
      }
    }else if (
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS_NOISE)
      !((1<<11)&d2_dpeek(FAKE_IO_START+20)) &&
#endif      
      (psg_reg[PSGR_MIXER] & (8 << abc))==0){ //noise enabled
      PSG_PREPARE_NOISE
      for (;count>0;count--){
#if defined(SSE_YM2149_DELAY_RENDERING)
        if(SSE_OPTION_PSG || SSE_OPTION_PSG_FIXED)
        {
          int t=0;
          if(psg_noisetoggle)
            ;
          else
            t|=(psg_reg[abc+8] & 15)<<1; // vol 4bit
          t<<=8*abc;
          *p|=t;
          p++;
        } else
#endif

        if(psg_noisetoggle){
          p++;
        }else{
          *(p++)+=vol;
        }
        PSG_NOISE_ADVANCE
      }

    }else{ //nothing enabled //SS playing samples
      //TRACE("F%d y%d PSG %x: %d at %d\n",FRAME,scan_y,abc+8,count,vol);
      for (;count>0;count--){
#if defined(SSE_YM2149_DELAY_RENDERING)
        if(SSE_OPTION_PSG || SSE_OPTION_PSG_FIXED)
        {
          int t=0;
          t|=(psg_reg[abc+8] & 15)<<1; // vol 4bit
          t<<=8*abc;
          *p|=t;
          p++;
        } else
#endif
#if defined(SSE_YM2149_FIXED_VOL_TABLE) && !defined(SSE_YM2149_NO_SAMPLES_OPTION)
/*  We don't add, we set (so there are useless rewrites), so we can use the
    full table value (no >> shift).
    If DMA sound is added to this, too bad!
*/
        if(SSEOption.PSGFixedVolume && playing_samples())
          *(p++)=vol;
        else
#endif
        *(p++)+=vol;
      }
    }
    psg_buf_pointer[abc]=to_t-psg_time_of_last_vbl_for_writing;
    return;
  }else
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS_ENV)
    if(!((1<<10)&d2_dpeek(FAKE_IO_START+20))) //'mute env'
#endif
  {  
 
   // Enveloped

//    DWORD est64=psg_envelope_start_time*64;
#ifdef TEST01
    int envdeath=0,psg_envstage=0,envshape=0;
    int psg_envmodulo=0,envvol=0,psg_envcountdown=0;
#else
    int envdeath,psg_envstage,envshape;
    int psg_envmodulo,envvol,psg_envcountdown;
#endif
    PSG_PREPARE_ENVELOPE;
// double &af,double &bf,int &psg_envmodulo,DWORD t,
//  int &psg_envstage,int &psg_envcountdown,int &envdeath,int &envshape,int &envvol
    if ((psg_reg[PSGR_MIXER] & (1 << abc))==0 && (toneperiod>9)){ //tone enabled
      PSG_PREPARE_TONE
      if (
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS_NOISE)
        !((1<<11)&d2_dpeek(FAKE_IO_START+20)) &&
#endif
        (psg_reg[PSGR_MIXER] & (8 << abc))==0){ //noise enabled
        PSG_PREPARE_NOISE
        for (;count>0;count--){
#if defined(SSE_YM2149_DELAY_RENDERING)
          if(SSE_OPTION_PSG || SSE_OPTION_PSG_FIXED)
          {
            int t=BIT_6; // code for enveloped
            if(psg_tonetoggle || psg_noisetoggle)
              ;
            else
              t|=envvol; // vol 5bit - check 'prepare'
            t<<=8*abc;
            *p|=t;
            p++;
          } else
#endif
          if(psg_tonetoggle || psg_noisetoggle){
            p++;
          }else{
            *(p++)+=envvol;
          }
          PSG_TONE_ADVANCE
          PSG_NOISE_ADVANCE
          PSG_ENVELOPE_ADVANCE
        }
      }else{ //tone only
        for (;count>0;count--){
#if defined(SSE_YM2149_DELAY_RENDERING)
          if(SSE_OPTION_PSG || SSE_OPTION_PSG_FIXED)
          {
            int t=BIT_6;
            if(psg_tonetoggle)
              ;
            else
              t|=envvol; // vol 5bit
            t<<=8*abc;
            *p|=t;
            p++;
          } else
#endif
          if(psg_tonetoggle){
            p++;
          }else{
            *(p++)+=envvol;
          }
          PSG_TONE_ADVANCE
          PSG_ENVELOPE_ADVANCE
        }
      }
    }else if (
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS_NOISE)
        !((1<<11)&d2_dpeek(FAKE_IO_START+20)) &&
#endif
      (psg_reg[PSGR_MIXER] & (8 << abc))==0){ //noise enabled
      PSG_PREPARE_NOISE
      for (;count>0;count--){
#if defined(SSE_YM2149_DELAY_RENDERING)
        if(SSE_OPTION_PSG || SSE_OPTION_PSG_FIXED)
        {
          int t=BIT_6;
          if(psg_noisetoggle)
            ;
          else
            t|=envvol; // vol 5bit
          t<<=8*abc;
          *p|=t;
          p++;
        } else
#endif
        if(psg_noisetoggle){
          p++;
        }else{
          *(p++)+=envvol;
        }
        PSG_NOISE_ADVANCE
        PSG_ENVELOPE_ADVANCE
      }
    }else{ //nothing enabled
      for (;count>0;count--){

#if defined(SSE_YM2149_DELAY_RENDERING)
        if(SSE_OPTION_PSG || SSE_OPTION_PSG_FIXED)
        {
          int t=BIT_6; 
          t|=envvol; // vol 5bit
          t<<=8*abc;
          *p|=t;
          p++;
        } else
#endif
        *(p++)+=envvol;
        PSG_ENVELOPE_ADVANCE
      }
    }
    psg_buf_pointer[abc]=to_t-psg_time_of_last_vbl_for_writing;
  }
}
//---------------------------------------------------------------------------
DWORD psg_quantize_time(int abc,DWORD t)
{
  int toneperiod=(((int)psg_reg[abc*2+1] & 0xf) << 8) + psg_reg[abc*2];
 if (toneperiod<=1) return t;

  double a,b;
  a=toneperiod*sound_freq;
  a/=(15625*8); //125000
  b=(t-psg_tone_start_time[abc]);
//		b=a-fmod(b,a);
  a=fmod(b,a);
  b-=a;
  t=psg_tone_start_time[abc]+DWORD(b);
  return t;
}

DWORD psg_adjust_envelope_start_time(DWORD t,DWORD new_envperiod)
{
  double b,c;
  int envperiod=max( (((int)psg_reg[PSGR_ENVELOPE_PERIOD_HIGH]) <<8) + psg_reg[PSGR_ENVELOPE_PERIOD_LOW],1);
//  a=envperiod;
//  a*=sound_freq;

  b=(t-psg_envelope_start_time);

  c=b*(double)new_envperiod;
  c/=(double)envperiod;
  c=t-c;         //new env start time

//  a/=7812.5; //that's 2000000/256
//  b+=a;
//  a=fmod(b,a);
//  b-=a;
//  t=psg_envelope_start_time+DWORD(b);


  return DWORD(c);
}

/*  SS: this is the function called by iow.cpp when program changes the PSG 
    registers.
    It takes care of writing the appropriate part of the VBL sound buffer
    before the register change.
    If option PSG is checked, it will write the digital volume values, 
    rendering is delayed until VBL to take advantage of the 3 ways volume table.
    old_val has just been read in the PSG register by iow.cpp
    new_val will be put into the register after this function has executed
*/

void psg_set_reg(int reg,BYTE old_val,BYTE &new_val)
{
  ASSERT(reg<=15);
  // suggestions for global variables:  n_samples_per_vbl=sound_freq/shifter_freq,   shifter_y+(SCANLINES_ABOVE_SCREEN+SCANLINES_BELOW_SCREEN)
  if (reg==1 || reg==3 || reg==5 || reg==13){
    new_val&=15;
  }else if (reg==6 || (reg>=8 && reg<=10)){
    new_val&=31;
  }
  if (reg>=PSGR_PORT_A) return; //SS 14,15
  //ASSERT(!(old_val==new_val && reg!=PSGR_ENVELOPE_SHAPE));
#if !defined(SSE_YM2149_WRITE_SAME_VALUE) //test?
  if (old_val==new_val && reg!=PSGR_ENVELOPE_SHAPE) return;
#endif
  if (psg_capture_file){
    psg_capture_check_boundary();
    DWORD cycle=int(ABSOLUTE_CPU_TIME-psg_capture_cycle_base);
    if (n_millions_cycles_per_sec!=8){
      cycle*=8; // this is safe, max 128000000*8
      cycle/=n_millions_cycles_per_sec;
    }
    BYTE reg_byte=BYTE(reg);

//    log_write(Str("--- cycle=")+cycle+" - reg="+reg_byte+" - val="+new_val);
    fwrite(&cycle,1,sizeof(cycle),psg_capture_file);
    fwrite(&reg_byte,1,sizeof(reg_byte),psg_capture_file);
    fwrite(&new_val,1,sizeof(new_val),psg_capture_file);
  }

  if (SoundActive()==0){
    log(Str("SOUND: ")+HEXSl(old_pc,6)+" - PSG reg "+reg+" changed to "+new_val+" at "+scanline_cycle_log());
    return;
  }
  int cpu_cycles_per_vbl=n_cpu_cycles_per_second/shifter_freq; //160000 at 50hz

#if SCREENS_PER_SOUND_VBL != 1
  cpu_cycles_per_vbl*=SCREENS_PER_SOUND_VBL;
  DWORDLONG a64=(ABSOLUTE_CPU_TIME-cpu_time_of_last_sound_vbl);
#else
  DWORDLONG a64=(ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl);
#endif
#ifndef TEST01__
  a64*=psg_n_samples_this_vbl; 
  a64/=cpu_cycles_per_vbl;
#endif
  //////ASSERT(psg_time_of_last_vbl_for_writing==cpu_time_of_last_vbl);//different scales
  DWORD t=psg_time_of_last_vbl_for_writing+(DWORD)a64;
  log(EasyStr("SOUND: PSG reg ")+reg+" changed to "+new_val+" at "+scanline_cycle_log()+"; samples "+t+"; vbl was at "+psg_time_of_last_vbl_for_writing);
  switch (reg){
    case 0:case 1:
    case 2:case 3:
    case 4:case 5:
    {
      int abc=reg/2;
      // Freq is double bufferred, it cannot change until the PSG reaches the end of the current square wave.
      // psg_tone_start_time[abc] is set to the last end of wave, so if it is in future don't do anything.
      // Overflow will be a problem, however at 50Khz that will take a day of non-stop output.
      if (t>psg_tone_start_time[abc]){
        t=psg_quantize_time(abc,t);
        psg_write_buffer(abc,t);
        psg_tone_start_time[abc]=t;
      }
      break;
    }
    case 6:  //changed noise
      psg_write_buffer(0,t);
      psg_write_buffer(1,t);
      psg_write_buffer(2,t);
      break;
    case 7:  //mixer
//      new_val|=b00111110;

      psg_write_buffer(0,t);
      psg_write_buffer(1,t);
      psg_write_buffer(2,t);
      break;
    case 8:case 9:case 10:  //channel A,B,C volume
//      new_val&=0xf;

      // ST doesn't quantize, it changes the level straight away.
      //  t=psg_quantize_time(reg-8,t);
#if defined(SSE_YM2149_FIXED_VOL_TABLE)
/*  The fixed volume being chosen for all channels at once in case of sample
    playing, we render them before so that volume values are correct at
    each time.
    We could render just once instead, but then we should update pointers TODO
    When user tries to mute PSG channels, we give up this rendering system
    because it's all or nothing.
*/
      if(SSEOption.PSGFixedVolume && playing_samples()
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS)
        &&! ((d2_dpeek(FAKE_IO_START+20)>>12)&7)
#endif
        )
      {
        psg_write_buffer(0,t);
        psg_write_buffer(1,t);
        psg_write_buffer(2,t);
      }
      else
#endif
      psg_write_buffer(reg-8,t);

//        psg_tone_start_time[reg-8]=t;
      break;
    case 11: //changing envelope period low
    {
      psg_write_buffer(0,t);
      psg_write_buffer(1,t);
      psg_write_buffer(2,t);
      int new_envperiod=max( (((int)psg_reg[PSGR_ENVELOPE_PERIOD_HIGH]) <<8) + new_val,1);
      psg_envelope_start_time=psg_adjust_envelope_start_time(t,new_envperiod);
      break;
    }
    case 12: //changing envelope period high
    {
      psg_write_buffer(0,t);
      psg_write_buffer(1,t);
      psg_write_buffer(2,t);
      int new_envperiod=max( (((int)new_val) <<8) + psg_reg[PSGR_ENVELOPE_PERIOD_LOW],1);
      psg_envelope_start_time=psg_adjust_envelope_start_time(t,new_envperiod);
      break;
    }
    case 13: //envelope shape
    {
/*
      DWORD abc_t[3]={t,t,t};
      for (int abc=0;abc<3;abc++){
        if (psg_reg[8+abc] & 16) abc_t[abc]=psg_quantize_time(abc,t);
      }
*/
//        t=psg_quantize_envelope_time(t,0,&new_envelope_start_time);
      psg_write_buffer(0,t);
      psg_write_buffer(1,t);
      psg_write_buffer(2,t);
      psg_envelope_start_time=t;
/*
      for (int abc=0;abc<3;abc++){
        if (psg_reg[8+abc] & 16) psg_tone_start_time[abc]=abc_t[abc];
      }
*/
      break;
    }
  }
}

void psg_capture(bool start,Str file)
{
  if (psg_capture_file){
    fclose(psg_capture_file);
    psg_capture_file=NULL;
  }
  if (start){
    psg_capture_file=fopen(file,"wb");
    if (psg_capture_file){
      WORD magic=0x2149;
      DWORD data_start=sizeof(WORD)+sizeof(DWORD)+sizeof(WORD)+sizeof(BYTE)*14;
      WORD version=1;

      fwrite(&magic,1,sizeof(magic),psg_capture_file);
      fwrite(&data_start,1,sizeof(data_start),psg_capture_file);
      fwrite(&version,1,sizeof(version),psg_capture_file);
      fwrite(psg_reg,14,sizeof(BYTE),psg_capture_file);

      psg_capture_cycle_base=ABSOLUTE_CPU_TIME;
    }
  }
}

void psg_capture_check_boundary()
{
  if (int(ABSOLUTE_CPU_TIME-psg_capture_cycle_base)>=(int)n_cpu_cycles_per_second){
    psg_capture_cycle_base+=n_cpu_cycles_per_second;

//    log_write(Str("--- second boundary"));
    DWORD cycle=0;
    BYTE reg_byte=0xff;
    BYTE new_val=0xff;
    fwrite(&cycle,1,sizeof(cycle),psg_capture_file);
    fwrite(&reg_byte,1,sizeof(reg_byte),psg_capture_file);
    fwrite(&new_val,1,sizeof(new_val),psg_capture_file);
  }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#undef LOGSECTION

