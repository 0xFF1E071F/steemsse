#if defined(STEVEN_SEAGAL) // other name?

int rt_sound_buf_pointer=0; // sample count
bool rt_started=0; // true when playing a sample

void Rt_FreeBuffer(bool);
//---------------------------------------------------------------------------
HRESULT Rt_Init()
{
//  TRACE("Create new RtAudio object...\n");
  try{
    rt_audio=new RtAudio(); // declared in init_sound.h
  }catch (RtError &error){
//    TRACE("Fail!\n");
    
    error.printMessage();
    return DSERR_GENERIC;
  }
  UseSound=XS_RT;
  if(sound_device_name.IsEmpty()) // EasyStr declared in init_sound.h, Unix-only
  {
    RtAudio::DeviceInfo info;
    unsigned int devices=rt_audio->getDeviceCount();
    fprintf(stderr,"There are %u devices\n",devices);
    for(unsigned int i=0;i<devices;i++) // !!! 0->n-1: has the system changed?
    {
      fprintf(stderr,"getting info for device %u\n",i);
      info=rt_audio->getDeviceInfo(i);
      if(info.probed && info.outputChannels>0) // add probed
      {
//        TRACE("RtAudio device %u #output channels: %u Formats:%X\n",i,info.outputChannels,info.nativeFormats);
        if(info.isDefaultOutput)
          sound_device_name=info.name.c_str(); // std::string!
      }
    }//nxt
//    if(!sound_device_name.IsEmpty())
    {
//      TRACE("RtAudio device %s\n",sound_device_name.c_str()); // easystr! segmentation fault...
    }
  }
  return DS_OK;
}
//---------------------------------------------------------------------------
DWORD Rt_GetTime()
{
  return rt_sound_buf_pointer;
}
//---------------------------------------------------------------------------
void Rt_Release()
{
  Rt_FreeBuffer(true);
  delete rt_audio;
  rt_audio=NULL;
}
//---------------------------------------------------------------------------
int Rt_Callback(void *bufferP, void*, unsigned int bufferSize, double, RtAudioStreamStatus, void*)
{
/* 
  parameters ( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
  some parameters are ignored
  The callback function must feed the input stream.
  Here it seems to be a copy from x_sound_buf[] to parameter bufferP
*/
  char *buffer = (char*) bufferP;
  int pointer_byte=rt_sound_buf_pointer;
  pointer_byte%=sound_buffer_length; // Get sample count within buffer
  pointer_byte*=sound_bytes_per_sample; // Convert to bytes

  if (sound_num_bits==8){
    for (int i=0;i<bufferSize;i++){
      if(rt_unsigned_8bit){
        for(int a=0;a<sound_bytes_per_sample;a++)*(buffer++)=char(x_sound_buf[pointer_byte++]);
      }else{
        for(int a=0;a<sound_bytes_per_sample;a++)*(buffer++)=char(x_sound_buf[pointer_byte++] ^ 128);
      }
      if (pointer_byte>=X_SOUND_BUF_LEN_BYTES) pointer_byte-=X_SOUND_BUF_LEN_BYTES;
      rt_sound_buf_pointer++;
    }
  }else{
    for (int i=0;i<bufferSize;i++){
      for (int b=0;b<sound_bytes_per_sample;b++){
        *(buffer++)=x_sound_buf[pointer_byte++];
        if (pointer_byte>=X_SOUND_BUF_LEN_BYTES) pointer_byte-=X_SOUND_BUF_LEN_BYTES;
      }
      rt_sound_buf_pointer++;
    }

  }
  return 0; // value for OK
}
//---------------------------------------------------------------------------
HRESULT Rt_StartBuffer(int flatlevel1,int flatlevel2)
{
  if(rt_audio==NULL)
    return DSERR_GENERIC;


  Rt_FreeBuffer(true); //Steem's, defined in this file
//note EXT int rt_buffer_size INIT(256),rt_buffer_num INIT(4);
  unsigned int bufferSize=rt_buffer_size;  // 256 sample frames
  unsigned int device=0;        // 0 indicates the default or first available device

  RtAudioFormat format=RTAUDIO_SINT8; // Signed 8-bit integer
  if(sound_num_bits==16)
    format=RTAUDIO_SINT16; // Signed 16-bit integer

  // now we find the device index through the name...
  RtAudio::DeviceInfo info;
  unsigned int devices=rt_audio->getDeviceCount();
///////  ASSERT(c); //what was it?
  for(unsigned int i=0;i<devices;i++) // 1->0
  {
    info=rt_audio->getDeviceInfo(i);
    if(IsSameStr_I(info.name.c_str(),sound_device_name)) // optimal?
    {
      device=i;
      break;
    }
  }//nxt

  info=rt_audio->getDeviceInfo(device); // and again...
  int closest_freq=0,f;
  for(int n=0;n<(int)(info.sampleRates.size());n++) // why not ++n (;))
  {
    f=info.sampleRates[n]; // it's a vector
    if (abs(sound_chosen_freq-f)<abs(sound_chosen_freq-closest_freq))
      closest_freq=f;
  }
  if (!closest_freq) 
    closest_freq=44100;
  sound_freq=closest_freq;

  // Let's open a stream
  RtAudio::StreamParameters outParams;
  ASSERT(device==rt_audio->getDefaultOutputDevice());
  outParams.deviceId = device;
  outParams.nChannels = sound_num_channels; // psg.h, =1
  try {
    rt_audio->openStream(&outParams,NULL,format,sound_freq,(unsigned int*)&rt_buffer_size, Rt_Callback);
    // 2 last parameters not specified, null by default
    // bufferSize -> global rt_buffer_size (bug?)
  }
  catch(RtError &error)
  {
//    TRACE("RtAudio failed to open stream\n");
    try{
      rt_audio->closeStream();
    }
    catch(...)
    {
//      TRACE("Error while closing stream\n");
    }
    error.printMessage();
    return DSERR_GENERIC;
  }
  sound_buffer_length=X_SOUND_BUF_LEN_BYTES/sound_bytes_per_sample;
/* note x_sound.cpp
// Enough for 500ms at 100Khz in 16-bit stereo
#define X_SOUND_BUF_LEN_BYTES (50000*4)
BYTE x_sound_buf[X_SOUND_BUF_LEN_BYTES+16];
*/
  XSoundInitBuffer(flatlevel1,flatlevel2); // this is a Steem function, in x_sound.cpp
  rt_sound_buf_pointer=0;// sample count
  try{
    rt_audio->startStream();
  }catch (RtError &error){
//    TRACE("RtAudio failed to start stream\n");
    try{
      rt_audio->stopStream();
    }catch(...){}
    try{
      rt_audio->closeStream();
    }catch(...){}
    error.printMessage();
    return DSERR_GENERIC;
  }
  rt_started=true;
  SoundBufStartTime=timeGetTime();
  sound_low_quality=(sound_freq<35000);
  WIN_ONLY( DSOpen=true; ) // notice WIN_ONLY

  return DS_OK;
}
//---------------------------------------------------------------------------
bool Rt_IsPlaying(){ return rt_started; }
//---------------------------------------------------------------------------
void Rt_ChangeVolume()
{
}
//---------------------------------------------------------------------------
void Rt_FreeBuffer(bool)
{
  if(!rt_started)
    return;
  try
  {
    rt_audio->stopStream();
  }
  catch (RtError& e) 
  {
//    TRACE("RtAudio failed to stop stream\n");
    e.printMessage();
  }
  if(rt_audio->isStreamOpen())
    rt_audio->closeStream(); 
  rt_started=0;
}
//---------------------------------------------------------------------------
HRESULT Rt_Stop(bool Immediate)
{
  Rt_FreeBuffer(Immediate);
  return DSERR_GENERIC;
}
//---------------------------------------------------------------------------

















#else // Steem 3.2


int rt_sound_buf_pointer=0; // sample count
bool rt_started=0;

void Rt_FreeBuffer(bool);
//---------------------------------------------------------------------------
HRESULT Rt_Init()
{
  try{
    rt_audio=new RtAudio();
  }catch (RtError &error){
    error.printMessage();
    return DSERR_GENERIC;
  }
  UseSound=XS_RT;

  {
    if(sound_device_name.IsEmpty()){
      RtAudio::DeviceInfo radi;
      int c=rt_audio->getDeviceCount();
      for (int i=1;i<=c;i++){
        radi=rt_audio->getDeviceInfo(i);
        if (radi.outputChannels>0){
          if (radi.isDefaultOutput){
            sound_device_name=radi.name.c_str();
          }
        }
      }
    }
  }
  return DS_OK;
}
//---------------------------------------------------------------------------
DWORD Rt_GetTime()
{
  return rt_sound_buf_pointer;
}
//---------------------------------------------------------------------------
void Rt_Release()
{
  Rt_FreeBuffer(true);
  delete rt_audio;
  rt_audio=NULL;
}
//---------------------------------------------------------------------------
int Rt_Callback(void *bufferP, void*, unsigned int bufferSize, double, RtAudioStreamStatus, void*)
{
  char *buffer = (char*) bufferP;
  int pointer_byte=rt_sound_buf_pointer;
  pointer_byte%=sound_buffer_length; // Get sample count within buffer
  pointer_byte*=sound_bytes_per_sample; // Convert to bytes

  if (sound_num_bits==8){
    for (int i=0;i<bufferSize;i++){
      if(rt_unsigned_8bit){
        for(int a=0;a<sound_bytes_per_sample;a++)*(buffer++)=char(x_sound_buf[pointer_byte++]);
      }else{
        for(int a=0;a<sound_bytes_per_sample;a++)*(buffer++)=char(x_sound_buf[pointer_byte++] ^ 128);
      }
      if (pointer_byte>=X_SOUND_BUF_LEN_BYTES) pointer_byte-=X_SOUND_BUF_LEN_BYTES;
      rt_sound_buf_pointer++;
    }
  }else{
    for (int i=0;i<bufferSize;i++){
      for (int b=0;b<sound_bytes_per_sample;b++){
        *(buffer++)=x_sound_buf[pointer_byte++];
        if (pointer_byte>=X_SOUND_BUF_LEN_BYTES) pointer_byte-=X_SOUND_BUF_LEN_BYTES;
      }
      rt_sound_buf_pointer++;
    }

  }
  return 0;
}
//---------------------------------------------------------------------------
HRESULT Rt_StartBuffer(int flatlevel1,int flatlevel2)
{
  RtAudio::DeviceInfo radi;
  if (rt_audio==NULL) return DSERR_GENERIC;

  Rt_FreeBuffer(true);

  unsigned int bufferSize=rt_buffer_size;  // 256 sample frames
  int device=0;        // 0 indicates the default or first available device

  RtAudioFormat format=RTAUDIO_SINT8;
  if (sound_num_bits==16) format=RTAUDIO_SINT16;

  {
    int c=rt_audio->getDeviceCount();
    for (int i=1;i<=c;i++){
      radi=rt_audio->getDeviceInfo(i);
      if (IsSameStr_I(radi.name.c_str(),sound_device_name)){
        device=i;
        break;
      }
    }
  }

  radi=rt_audio->getDeviceInfo(device);
  {
    int closest_freq=0,f;
    for (int n=0;n<int(radi.sampleRates.size());n++){
      f=radi.sampleRates[n];
      if (abs(sound_chosen_freq-f)<abs(sound_chosen_freq-closest_freq)){
        closest_freq=f;
      }
    }
    if (closest_freq==0) closest_freq=44100;
    sound_freq=closest_freq;
  }
  
  try{
    RtAudio::StreamParameters outParams;
    outParams.deviceId = device;
    outParams.nChannels = sound_num_channels;
    rt_audio->openStream(&outParams,NULL,format,sound_freq,&bufferSize, Rt_Callback);
  }catch (RtError &error){
    try{
      rt_audio->closeStream();
    }catch(...){}
    error.printMessage();
    return DSERR_GENERIC;
  }

  sound_buffer_length=X_SOUND_BUF_LEN_BYTES/sound_bytes_per_sample;
  XSoundInitBuffer(flatlevel1,flatlevel2);
  rt_sound_buf_pointer=0;
  try{
    rt_audio->startStream();
  }catch (RtError &error){
    try{
      rt_audio->stopStream();
    }catch(...){}
    try{
      rt_audio->closeStream();
    }catch(...){}
    error.printMessage();
    return DSERR_GENERIC;
  }

  rt_started=true;
  SoundBufStartTime=timeGetTime();
  sound_low_quality=(sound_freq<35000);
  WIN_ONLY( DSOpen=true; )

  return DS_OK;
}
//---------------------------------------------------------------------------
bool Rt_IsPlaying(){ return rt_started; }
//---------------------------------------------------------------------------
void Rt_ChangeVolume()
{
}
//---------------------------------------------------------------------------
void Rt_FreeBuffer(bool)
{
  if (rt_started==0) return;

  try{
    rt_audio->stopStream();
  }catch (...){};
  try{
    rt_audio->closeStream();
  }catch (...){};
  rt_started=0;
}
//---------------------------------------------------------------------------
HRESULT Rt_Stop(bool Immediate)
{
  Rt_FreeBuffer(Immediate);
  return DSERR_GENERIC;
}
//---------------------------------------------------------------------------

#endif