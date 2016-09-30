#pragma once
#ifndef PSG_DECLA_H
#define PSG_DECLA_H

#define EXT extern
#define INIT(s)


/*  tone frequency
     fT = fMaster
          -------
           16TP


ie. period = 16TP / 2 000 000
= TP/125000

 = TP / (8*15625)


 the envelope repetition frequency fE is obtained as follows from the
envelope setting period value EP (decimal):

       fE = fMaster        (fMaster if the frequency of the master clock)
            -------
             256EP

The period of the actual frequency fEA used for the envelope generated is
1/32 of the envelope repetition period (1/fE).

ie. period of whole envelope = 256EP/2 000 000 = 2*EP / 15625 (think this is one go through, eg. /)
Period of envelope change frequency is 1/32 of this, so EP/ 16*15625
Scale by 64 and multiply by frequency of buffer to get final period, 4*EP/15625

*/

/*      New scalings - 6/7/01

__TONE__  Period = TP/(8*15625) Hz = (TP*sound_freq)/(8*15625) samples
          Period in samples goes up to 1640.6.  Want this to correspond to 2000M, so
          scale up by 2^20 = 1M.  So period to initialise counter is
          (TP*sound_freq*2^17)/15625, which ranges up to 1.7 thousand million.  The
          countdown each time is 2^20, but we double this to 2^21 so that we can
          spot each half of the period.  To initialise the countdown, bit 0 of
          (time*2^21)/tonemodulo gives high or low.  The counter is initialised
          to tonemodulo-(time*2^21 mod tonemodulo).

__NOISE__ fudged similarly to tone.

__ENV__   Step period = 2EP/(15625*32) Hz.  Scale by 2^17 to scale 13124.5 to 1.7 thousand
          million.  Step period is (EP*sound_freq*2^13)/15625.

*/




#define SOUND_DESIRED_LQ_FREQ (50066/2)

#ifdef ENABLE_VARIABLE_SOUND_DAMPING
EXT int sound_variable_a INIT(32);
EXT int sound_variable_d INIT(208);
#endif

#if defined(SSE_SOUND_FILTER_STF5)
#define SOUND_MODE_MUTE         0
#define SOUND_MODE_EMULATED     1
#define SOUND_MODE_CHIP         2
#define SOUND_MODE_MONITOR      3
#define SOUND_MODE_SHARPSAMPLES 4
#define SOUND_MODE_SHARPCHIP    5
#if defined(SSE_SOUND_FILTER_HATARI)
#define SOUND_MODE_HATARI    6
#endif

#else
#define SOUND_MODE_MUTE         0
#define SOUND_MODE_CHIP         1
#define SOUND_MODE_EMULATED     2
#define SOUND_MODE_SHARPSAMPLES 3
#define SOUND_MODE_SHARPCHIP    4
#if defined(SSE_SOUND_FILTER_STF5)
//#define SOUND_MODE_MONITOR      5
#endif
#endif
#if !defined(SOUND_DISABLE_INTERNAL_SPEAKER)
EXT bool sound_internal_speaker INIT(false);
#endif
EXT int sound_freq INIT(50066),sound_comline_freq INIT(0),sound_chosen_freq INIT(50066);
EXT BYTE sound_num_channels INIT(1),sound_num_bits INIT(8);
#if defined(SSE_VAR_RESIZE_383)
EXT BYTE sound_bytes_per_sample INIT(1);
#else
EXT int sound_bytes_per_sample INIT(1);
#endif
#if defined(SSE_SOUND_VOL_LOGARITHMIC_2)
EXT int MaxVolume;// INIT(0xffff);
#else
EXT DWORD MaxVolume INIT(0xffff);
#endif
EXT bool sound_low_quality INIT(0);
EXT bool sound_write_primary INIT( NOT_ONEGAME(0) ONEGAME_ONLY(true) );
EXT bool sound_click_at_start INIT(0);
EXT bool sound_record INIT(false);
EXT DWORD sound_record_start_time; //by timer variable = timeGetTime()
EXT int psg_write_n_screens_ahead INIT(3 UNIX_ONLY(+7) );

EXT void SoundStopInternalSpeaker();

EXT int psg_voltage,psg_dv;

#define PSGR_PORT_A 14
#define PSGR_PORT_B 15

EXT BYTE psg_reg[16],psg_reg_data;

EXT FILE *wav_file INIT(NULL);

#if defined(ENABLE_LOGFILE) || defined(SHOW_WAVEFORM)
EXT DWORD min_write_time;
EXT DWORD play_cursor,write_cursor;
#endif

#define DEFAULT_SOUND_BUFFER_LENGTH (32768*SCREENS_PER_SOUND_VBL)
EXT int sound_buffer_length INIT(DEFAULT_SOUND_BUFFER_LENGTH);
EXT DWORD SoundBufStartTime;

#define PSG_BUF_LENGTH sound_buffer_length

#if SCREENS_PER_SOUND_VBL == 1
//#define MOD_PSG_BUF_LENGTH &(PSG_BUF_LENGTH-1)
#define MOD_PSG_BUF_LENGTH %PSG_BUF_LENGTH
#else
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
#define MAX_temp_waveform_display_counter PSG_BUF_LENGTH
EXT BYTE temp_waveform_display[DEFAULT_SOUND_BUFFER_LENGTH];
EXT void draw_waveform();
EXT DWORD temp_waveform_play_counter;
//#define TEMP_WAVEFORM_INTERVAL 31

#endif

EXT void dma_sound_get_last_sample(WORD*,WORD*);

EXT void psg_capture(bool,Str),psg_capture_check_boundary();
EXT FILE *psg_capture_file INIT(NULL);
EXT int psg_capture_cycle_base INIT(0);

EXT bool psg_always_capture_on_start INIT(0);


//#ifdef IN_EMU
//---------------------------------------------------------------------------
HRESULT Sound_VBL();
//--------------------------------------------------------------------------- DMA Sound
void dma_sound_fetch();
#if defined(SSE_CARTRIDGE_BAT)
void dma_mv16_fetch(WORD data);
#endif
void dma_sound_set_control(BYTE);
void dma_sound_set_mode(BYTE);

EXT BYTE dma_sound_control,dma_sound_mode;
EXT MEM_ADDRESS dma_sound_start,next_dma_sound_start,
            dma_sound_end,next_dma_sound_end;

EXT WORD MicroWire_Mask;
EXT WORD MicroWire_Data;
EXT int MicroWire_StartTime;

#define CPU_CYCLES_PER_MW_SHIFT 8

EXT WORD dma_sound_internal_buf[4],dma_sound_last_word;
EXT MEM_ADDRESS dma_sound_fetch_address;

// Max frequency/lowest refresh *2 for stereo
#define DMA_SOUND_BUFFER_LENGTH 2600 * SCREENS_PER_SOUND_VBL * 2
EXT WORD dma_sound_channel_buf[DMA_SOUND_BUFFER_LENGTH+16];
EXT DWORD dma_sound_channel_buf_last_write_t;

#define DMA_SOUND_CHECK_TIMER_A \
    if (mfp_reg[MFPR_TACR]==8){ \
      mfp_timer_counter[0]-=64;  \
      if (mfp_timer_counter[0]<64){ \
        mfp_timer_counter[0]=BYTE_00_TO_256(mfp_reg[MFPR_TADR])*64; \
        mfp_interrupt_pend(MFP_INT_TIMER_A,ABSOLUTE_CPU_TIME); \
      }                                 \
    }

#if defined(SSE_VAR_RESIZE_383)
EXT BYTE psg_reg_select;
EXT BYTE sound_time_method INIT(0);
EXT BYTE sound_mode INIT(SOUND_MODE_CHIP),sound_last_mode INIT(SOUND_MODE_CHIP);
EXT WORD dma_sound_mode_to_freq[4],dma_sound_freq;
EXT int dma_sound_output_countdown,dma_sound_samples_countdown;
EXT BYTE dma_sound_internal_buf_len;
EXT bool dma_sound_on_this_screen;
EXT BYTE dma_sound_mixer,dma_sound_volume;
EXT BYTE dma_sound_l_volume,dma_sound_r_volume;
EXT BYTE dma_sound_l_top_val,dma_sound_r_top_val;
#else
EXT int psg_reg_select;
EXT int sound_time_method INIT(0);
EXT int sound_mode INIT(SOUND_MODE_CHIP),sound_last_mode INIT(SOUND_MODE_CHIP);
EXT int dma_sound_mode_to_freq[4];
EXT int dma_sound_freq,dma_sound_output_countdown,dma_sound_samples_countdown;
EXT int dma_sound_internal_buf_len;
EXT int dma_sound_on_this_screen;
EXT int dma_sound_mixer,dma_sound_volume;
EXT int dma_sound_l_volume,dma_sound_r_volume;
EXT int dma_sound_l_top_val,dma_sound_r_top_val;
#endif
#if defined(SSE_SOUND_MICROWIRE)
#include "../../3rdparty/dsp/dsp.h"
#if defined(SSE_VAR_RESIZE_383)
EXT BYTE dma_sound_bass;
EXT BYTE dma_sound_treble;
#else
EXT int dma_sound_bass;
EXT int dma_sound_treble;
#endif
#endif//microwire

//---------------------------------- PSG ------------------------------------

void psg_write_buffer(int,DWORD);

//make constant for dimming waveform display

//#define PSG_BUF_LENGTH (32768*SCREENS_PER_SOUND_VBL)

#define PSG_NOISE_ARRAY 8192
#define MOD_PSG_NOISE_ARRAY & 8191

#ifndef ONEGAME
#if defined(SSE_SOUND_NO_EXTRA_PER_VBL)
#define PSG_WRITE_EXTRA 0
#else
#define PSG_WRITE_EXTRA 300
#endif
#else
#define PSG_WRITE_EXTRA OGExtraSamplesPerVBL
#endif

//#define PSG_WRITE_EXTRA 10

//#define PSG_CHANNEL_BUF_LENGTH (2048*SCREENS_PER_SOUND_VBL)
#define PSG_CHANNEL_BUF_LENGTH (8192*SCREENS_PER_SOUND_VBL)
#define VOLTAGE_ZERO_LEVEL 0
#define VOLTAGE_FIXED_POINT 256
//must now be fixed at 256!
#define VOLTAGE_FP(x) ((x) << 8)

//BYTE psg_channel_buf[3][PSG_CHANNEL_BUF_LENGTH];
EXT int psg_channels_buf[PSG_CHANNEL_BUF_LENGTH+16]; //SS Steem will add channels into one integer
EXT int psg_buf_pointer[3];
EXT DWORD psg_tone_start_time[3];


//DWORD psg_tonemodulo_2,psg_noisemodulo;
//const short volscale[16]=  {0,1,1,2,3,4,5,7,12,20,28,44,70,110,165,255};

EXT char psg_noise[PSG_NOISE_ARRAY];



#define PSGR_NOISE_PERIOD 6
#define PSGR_MIXER 7
#define PSGR_AMPLITUDE_A 8
#define PSGR_AMPLITUDE_B 9
#define PSGR_AMPLITUDE_C 10
#define PSGR_ENVELOPE_PERIOD 11
#define PSGR_ENVELOPE_PERIOD_LOW 11
#define PSGR_ENVELOPE_PERIOD_HIGH 12
#define PSGR_ENVELOPE_SHAPE 13

/*
       |     |13 Envelope Shape                         BIT 3 2 1 0|
       |     |  Continue -----------------------------------' | | ||
       |     |  Attack ---------------------------------------' | ||
       |     |  Alternate --------------------------------------' ||
       |     |  Hold ---------------------------------------------'|
       |     |   00xx - \____________________________________      |
       |     |   01xx - /|___________________________________      |
       |     |   1000 - \|\|\|\|\|\|\|\|\|\|\|\|\|\|\|\|\|\|\      |
       |     |   1001 - \____________________________________      |
       |     |   1010 - \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\      |
       |     |   1011 - \|-----------------------------------      |
       |     |   1100 - /|/|/|/|/|/|/|/|/|/|/|/|/|/|/|/|/|/|/      |
       |     |   1101 - /------------------------------------      |
       |     |   1110 - /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/      |
       |     |   1111 - /|___________________________________      |

*/

#define PSG_ENV_SHAPE_HOLD BIT_0
#define PSG_ENV_SHAPE_ALT BIT_1
#define PSG_ENV_SHAPE_ATTACK BIT_2
#define PSG_ENV_SHAPE_CONT BIT_3

#define PSG_CHANNEL_AMPLITUDE 60 //SS what is this?

//#define PSG_VOLSCALE(vl) (volscale[vl]/4+VOLTAGE_ZERO_LEVEL)
/*
#define PSG_ENV_DOWN 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
#define PSG_ENV_UP   00,01,02,03,04,05,6,7,8,9,10,11,12,13,14,15
#define PSG_ENV_0    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#define PSG_ENV_LOUD 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15

const BYTE psg_envelopes[8][32]={
    {PSG_ENV_DOWN,PSG_ENV_DOWN},
    {PSG_ENV_DOWN,PSG_ENV_0},
    {PSG_ENV_DOWN,PSG_ENV_UP},
    {PSG_ENV_DOWN,PSG_ENV_LOUD},
    {PSG_ENV_UP,PSG_ENV_UP},
    {PSG_ENV_UP,PSG_ENV_LOUD},
    {PSG_ENV_UP,PSG_ENV_DOWN},
    {PSG_ENV_UP,PSG_ENV_0}};

*/
//const int psg_flat_volume_level[16]={0*VA/1000+VZL*VFP,0*VA/1000+VZL*VFP,2*VA/1000+VZL*VFP,5*VA/1000+VZL*VFP,20*VA/1000+VZL*VFP,50*VA/1000+VZL*VFP,65*VA/1000+VZL*VFP,80*VA/1000+VZL*VFP,100*VA/1000+VZL*VFP,125*VA/1000+VZL*VFP,178*VA/1000+VZL*VFP,250*VA/1000+VZL*VFP,354*VA/1000+VZL*VFP,510*VA/1000+VZL*VFP,707*VA/1000+VZL*VFP,1000*VA/1000+VZL*VFP};
EXT const int psg_flat_volume_level[16];

EXT const int psg_envelope_level[8][64];

DWORD psg_quantize_time(int,DWORD);
void psg_set_reg(int,BYTE,BYTE&);

EXT DWORD psg_envelope_start_time;
//---------------------------------------------------------------------------

//#endif//inemu

#undef EXT
#undef INIT

#endif//#ifndef PSG_DECLA_H