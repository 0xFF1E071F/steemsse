#include "SSE.h"

#if defined(SSE_YM2149_OBJECT)
/*  In v3.5.1, object YM2149 was only used for drive management (drive, side).
    In v3.7.0, sound functions were introduced (sampled soundchip, more realistic).
    In v3.9.2, it harbours an alternative PSG emu based on MAME.
*/

#include "../pch.h"
#include <cpu.decla.h>
#include <iorw.decla.h>
#include <psg.decla.h>
#include <gui.decla.h>
#include <stports.decla.h>
#include <display.decla.h>
#include "SSEYM2149.h"
#include "SSEOption.h"
#include "SSEInterrupt.h"


TYM2149::TYM2149() { //v3.7.0
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
  p_fixed_vol_3voices=NULL;
#endif
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  AntiAlias=NULL;
#endif
}


TYM2149::~TYM2149() { //v3.7.0
#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0
  FreeFixedVolTable();
#endif
}

#if defined(SSE_YM2149_DYNAMIC_TABLE)//v3.7.0

#define LOGSECTION LOGSECTION_SOUND

void TYM2149::FreeFixedVolTable() {
  if(p_fixed_vol_3voices)
  {
    TRACE_LOG("free memory of PSG table %p\n",p_fixed_vol_3voices);
    delete [] p_fixed_vol_3voices;
    p_fixed_vol_3voices=NULL;
  }
}


bool TYM2149::LoadFixedVolTable() {
  bool ok=false;
  FreeFixedVolTable(); //safety
  p_fixed_vol_3voices=new WORD[16*16*16];
  ASSERT(p_fixed_vol_3voices);
  EasyStr filename=RunDir+SLASH+YM2149_FIXED_VOL_FILENAME;
  FILE *fp=fopen(filename.Text,"r+b");
#if defined(SSE_YM2149_DYNAMIC_TABLE1)
  EasyStr filename2=RunDir+SLASH+"ym2149_fixed_vol2.bin";
  FILE *fp2=fopen(filename2.Text,"w+b");
#endif
  if(fp && p_fixed_vol_3voices)
  {
    int nwords=fread(p_fixed_vol_3voices,sizeof(WORD),16*16*16,fp);
    if(nwords==16*16*16)
      ok=true;
    fclose(fp);
#if defined(SSE_SOUND_16BIT_CENTRED)
/*  In previous versions, the zero (silence) was a very negative value,
    for sampled YM (here) as well as for 'Steem native' (using the tables
    in psg.cpp).
    If we want the zero to be set at the zero of a signed 16bit wave, we
    must reduce the range in both tables.
    So we simply lose the last bit to half the range.
    I doubt the ST has a 16bit dynamic range anyway :), we lose nothing.
    All values are positive, the signed sound wave will be totally lopsided.
*/
    for(int i=0;i<16*16*16;i++)
      p_fixed_vol_3voices[i]>>=1; 
#if defined(SSE_YM2149_DYNAMIC_TABLE1)
    nwords=fwrite(p_fixed_vol_3voices,sizeof(WORD),16*16*16,fp2);
    ASSERT(nwords==16*16*16);
    fclose(fp2);
#endif
#endif
#if defined(SSE_SOUND_MOVE_ZERO)
    // move the zero to make it match DMA's (tentative) //was bad idea
    for(int i=0;i<16*16*16;i++)
      p_fixed_vol_3voices[i]+=128;
#endif
    SSEConfig.ym2149_fixed_vol=true;
    TRACE_LOG("PSG %s loaded %d words in ram %p\n",filename.Text,nwords,p_fixed_vol_3voices);
  }
  else
  {
    TRACE_LOG("No file %s\n",filename.Text);
    FreeFixedVolTable();
#if defined(SSE_YM2149_TABLE_NOT_OPTIONAL) || defined(SSE_VAR_REQUIRE_FILES)
    throw filename.Text;
#else
    OPTION_SAMPLED_YM=0;
#endif
  }
  return ok;
}

#undef LOGSECTION

#endif//SSE_YM2149_DYNAMIC_TABLE


BYTE TYM2149::Drive(){
  BYTE drive=NO_VALID_DRIVE; // different from floppy_current_drive()
  if(!(PortA()&BIT_1))
    drive=0; //A:
  else if(!(PortA()&BIT_2))
    drive=1; //B:
  return drive;
}


BYTE TYM2149::PortA(){
  return psg_reg[PSGR_PORT_A];
}

#if defined(SSE_YM2149_MAMELIKE)

void TYM2149::Reset() {
  m_rng = 1; //it will take 2exp17=131072 values
  m_output[0] = 0;
  m_output[1] = 0;
  m_output[2] = 0;
  m_count[0] = 0;
  m_count[1] = 0;
  m_count[2] = 0;
  m_count_noise = 0;
  m_count_env = 0;
  m_prescale_noise = 0;
  m_cycles=0; 
  // 32 steps for envelope in YM - we do it at reset for old snapshots
  m_env_step_mask=ENVELOPE_MASK; 
  m_hold=1; // Captain Blood
#if defined(SSE_YM2149_MAMELIKE_AVG_SMP)
  m_oversampling_count=0;
#endif
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  time_at_vbl_start=0;
  time_of_last_sample=0;
#endif
}

#define NOISE_ENABLEQ(_chan)  ((psg_reg[PSGR_MIXER] >> (3 + _chan)) & 1)
#define TONE_ENABLEQ(_chan)   ((psg_reg[PSGR_MIXER] >> (_chan)) & 1)

#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS_NOISE)
#define NOISE_OUTPUT() (((1<<11)&d2_dpeek(FAKE_IO_START+20))?0:(m_rng & 1))
#else
#define NOISE_OUTPUT()          (m_rng & 1)
#endif

#define TONE_PERIOD(_chan)   \
  ( psg_reg[(_chan) << 1] | ((psg_reg[((_chan) << 1) | 1] & 0x0f) << 8) )
#define ENVELOPE_PERIOD()       ((psg_reg[PSGR_ENVELOPE_PERIOD_LOW] \
  | (psg_reg[PSGR_ENVELOPE_PERIOD_HIGH]<<8)))

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)

void TYM2149::psg_write_buffer(DWORD to_t, bool vbl) {
  ASSERT(AntiAlias||DSP_DISABLED);
#else

void TYM2149::psg_write_buffer(DWORD to_t) {

#endif
  ASSERT(OPTION_MAME_YM);

#if defined(SSE_CARTRIDGE_BAT2)
/*  B.A.T II plays the same samples on both the MV16 and the PSG, because
    the MV16 of B.A.T I wasn't included, it was just supported.
    If player inserted the cartridge, he doesn't need PSG sounds.
*/
  if(SSEConfig.mv16 && DONGLE_ID==TDongle::BAT2)
    return; 
#endif

#if defined(SSE_YM2149_MAMELIKE_393)
  if(!psg_n_samples_this_vbl)
    return;
#endif

  // compute #samples at our current sample rate
  DWORD t=(psg_time_of_last_vbl_for_writing+psg_buf_pointer[0]);
  to_t=max(to_t,t);
  to_t=min(to_t,psg_time_of_last_vbl_for_writing+PSG_CHANNEL_BUF_LENGTH);
  int count=max(min((int)(to_t-t),PSG_CHANNEL_BUF_LENGTH-psg_buf_pointer[0]),0);
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  if(!count && !AntiAlias)
#else
  if(!count)
#endif
    return;

  int *p=psg_channels_buf+psg_buf_pointer[0];
  ASSERT(p-psg_channels_buf<=PSG_CHANNEL_BUF_LENGTH);

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  if(!AntiAlias)
#endif
    *p=0;

  ASSERT(sound_freq);
  // YM2149 @2mhz = 1/4 * 8mhz clock  - On STF, same as CPU
  double ym2149_cycles_per_sample=((double)CpuNormalHz/4)/(double)sound_freq;

  // when is next sample due?
  COUNTER_VAR time_to_send_next_sample=(m_cycles+(int)ym2149_cycles_per_sample);

  COUNTER_VAR ym2149_cycles_at_start_of_loop=m_cycles;
  int samples_sent=0;

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  COUNTER_VAR cycles_to_run=0;
  if(AntiAlias)
  {
    COUNTER_VAR cpu_frame_cycles=FRAMECYCLES;
    COUNTER_VAR psg_frame_cycles=cpu_frame_cycles/4;
    COUNTER_VAR psg_already_run=m_cycles-time_at_vbl_start;
    if(psg_already_run<0)
      time_at_vbl_start=m_cycles,psg_already_run=0;
    cycles_to_run=psg_frame_cycles-psg_already_run;
    if(cycles_to_run<=0)
      return;
  }
#endif


/*  The following was inspired by MAME project, especially ay8910.cpp.
    thx Couriersud.
    Notice the emulation is both simple and short. It's possible that
    this system is more efficient than Steem's way (PREPARE, ADVANCE...).
*/

  /* The 8910 has three outputs, each output is the mix of one of the three */
  /* tone generators and of the (single) noise generator. The two are mixed */
  /* BEFORE going into the DAC. The formula to mix each channel is: */
  /* (ToneOn | ToneDisable) & (NoiseOn | NoiseDisable). */
  /* Note that this means that if both tone and noise are disabled, the output */
  /* is 1, not 0, and can be modulated changing the volume. */

  /* buffering loop */

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
  // override vbl: Steem may want to run longer
  for(COUNTER_VAR i=0; ((AntiAlias&&!vbl)?(i<cycles_to_run):count);i++)
#else
  while(count)
#endif
  {

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
    if(AntiAlias)
      m_cycles++;
    else
#endif
      m_cycles+=8;  //the driver is clocked with clock / 8  (250Khz)
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
    if(!AntiAlias||!(m_cycles%8)) {
#endif

    // We compute noise then envelope, then we compute each tone and
    // mix each channel

    m_count_noise++;

    if (m_count_noise >= (psg_reg[PSGR_NOISE_PERIOD] & 0x1f))
    {
      /* toggle the prescaler output. Noise is no different to
       * channels.
       */
      m_count_noise = 0;
      ASSERT(!(m_prescale_noise&0xfe));
      m_prescale_noise ^= 1;

      if (m_prescale_noise)
      {
        /* The Random Number Generator of the 8910 is a 17-bit shift */
        /* register. The input to the shift register is bit0 XOR bit3 */
        /* (bit0 is the output). This was verified on AY-3-8910 and YM2149 chips. */
        ASSERT(m_rng);
        m_rng ^= (((m_rng & 1) ^ ((m_rng >> 3) & 1)) << 17);
        m_rng >>= 1;
      }
    }

    /* update envelope */
    ASSERT(m_env_step_mask==ENVELOPE_MASK);
    ASSERT(!(m_holding&0xfe));

    if (m_holding == 0)
    {
      m_count_env++;
      if (m_count_env >= ENVELOPE_PERIOD()) // "m_step"=1 for YM2149
      {
        m_count_env = 0;
        m_env_step--;

        /* check envelope current position */
        if (m_env_step < 0)
        {
          if (m_hold)
          {
            if (m_alternate)
              m_attack ^=m_env_step_mask;
            m_holding = 1;
            m_env_step = 0;
          }
          else
          {
            /* if CountEnv has looped an odd number of times (usually 1), */
            /* invert the output. */
            if (m_alternate && (m_env_step & (m_env_step_mask + 1)))
              m_attack ^= m_env_step_mask;

            m_env_step &= m_env_step_mask;
          }
          ASSERT(m_env_step>=0);
        }

      }
    }
    m_env_volume = (m_env_step ^ m_attack);
    ASSERT(m_env_volume>=0 && m_env_volume<32);

    //as in psg's AlterV
    BYTE index[3],interpolate[4];
    *(int*)interpolate=0;
    int vol=0;
    //TRACE_OSD("%d %d %d",TONE_PERIOD(0),TONE_PERIOD(1),TONE_PERIOD(2));
    for(int abc=0;abc<3;abc++)
    {
      // Tone
      bool enveloped=((psg_reg[abc+8] & BIT_4)!=0);

      m_count[abc]++;
      if(m_count[abc]>=TONE_PERIOD(abc)) 
      {
        ASSERT(!(m_output[abc]&0xfe));
        m_output[abc] ^= 1;
        m_count[abc]=0;
      }

      // mixing
      m_vol_enabled[abc] = (m_output[abc] | TONE_ENABLEQ(abc)) 
        & (NOISE_OUTPUT() | NOISE_ENABLEQ(abc));

      // from here on, specific to Steem rendering (different options)
      if(OPTION_SAMPLED_YM) // one table for all [A,B,C] volume sets
      {
        int digit=(enveloped) ? BIT_6 :0;
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS)
        if( (4>>abc) & (d2_dpeek(FAKE_IO_START+20)>>12 ))
          ; // 0: skip this channel
        else
#endif
        // pick correct volume or 0
        if(!m_vol_enabled[abc])
          ; // 0
        else if (enveloped)
#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS_ENV)
        {
          if((1<<10)&d2_dpeek(FAKE_IO_START+20)) 
            ; //0: 'mute env'
          else
            digit |=m_env_volume; // vol 5bit
        }
#else
          digit |=m_env_volume; // vol 5bit
#endif
        else
          digit |=(psg_reg[abc+8] & 15)<<1; // vol 4bit shifted

        index[abc]=(digit >>1)&0xF; // 4bit volume
        interpolate[abc]=((digit&BIT_6) && index[abc]>0 && !(digit &1) ) ? 1 : 0;
      }
      else // Steem's orignal tables per channel
      {

#if defined(SSE_BOILER_MUTE_SOUNDCHANNELS)
        if( (4>>abc) & (d2_dpeek(FAKE_IO_START+20)>>12 ))
          ; // 0: skip this channel
        else
#endif
        if(!m_vol_enabled[abc])
          ; // 0
        else if (enveloped)
          vol+=psg_envelope_level[4][m_env_volume];
        else
          vol+=psg_flat_volume_level[(psg_reg[abc+8] & 15)];

      }
    }//nxt abc

    if(OPTION_SAMPLED_YM) // now we have 3 indexes (vol on 4bit)
    {
      // because of averaging, we "render" the 4/5bit samples now
      vol=p_fixed_vol_3voices[(16*16)*index[2]+16*index[1]+index[0]]; // assume SSE_YM2149_DYNAMIC_TABLE
      if(*(int*)(&interpolate[0]))
      {
        int vol2=p_fixed_vol_3voices[ (16*16)*(index[2]-interpolate[2])
          +16*(index[1]-interpolate[1])+(index[0]-interpolate[0])];
        vol= (int) sqrt( (float) vol * (float) vol2); 
      }
    }

#if defined(SSE_YM2149_MAMELIKE_AVG_SMP)
    ASSERT(m_oversampling_count<0xff);
    m_oversampling_count++;
    *p+=vol;
#else
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
/*  Thanks to this kick-ass filter, we avoid horrible aliasing in all
    sample rates.
    Notice that we must set the filter at about 10khz.
    The Union Demo is a good test case. 
    We have far better sound in Star Trek too.
*/
    if(AntiAlias)
      *p=AntiAlias->do_sample(vol);
    else
#endif
      *p=vol;
#endif

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
    }//if
#endif

    if(m_cycles-time_to_send_next_sample>=0
#if defined(SSE_BUGFIX_393) //anticrash
      && (p-psg_channels_buf)<=PSG_CHANNEL_BUF_LENGTH
#endif
      )
    {
      ASSERT(p-psg_channels_buf<=PSG_CHANNEL_BUF_LENGTH);
#if defined(SSE_YM2149_MAMELIKE_AVG_SMP)
      if(m_oversampling_count>1)
        *p/=m_oversampling_count;
      m_oversampling_count=0;
      //TRACE_OSD("%d",*p);
#endif
#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
      int copy=*p;
      *(++p)=copy; //same value, not zero
#else
      *(++p)=0; //overshoot by 1 is OK
#endif 
      count--;
      samples_sent++;

#if defined(SSE_YM2149_MAMELIKE_ANTIALIAS)
      frame_samples++;
      time_of_last_sample=m_cycles;
      if(AntiAlias)
      {
        time_to_send_next_sample=(double)time_at_vbl_start
          +((double)frame_samples+1.0)*ym2149_cycles_per_sample;
      }
      else
#endif
        time_to_send_next_sample=ym2149_cycles_at_start_of_loop
          +(int)(((double)samples_sent+1)*ym2149_cycles_per_sample);
    }
  }
  psg_buf_pointer[0]=to_t-psg_time_of_last_vbl_for_writing;
  psg_buf_pointer[2]=psg_buf_pointer[1]=psg_buf_pointer[0];
}

#endif//mame-like

#endif//SSE_YM2149_OBJECT

