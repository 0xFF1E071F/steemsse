/*---------------------------------------------------------------------------
FILE: hdimg.cpp
MODULE: Steem
NOTE: STUB
DESCRIPTION: This file was going to house Steem's medium-level hard disk image
emulation, but it hasn't been written yet (and probably won't be).
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_INFO)
#pragma message("Included for compilation: hdimg.cpp")
#endif

#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_HDIMG_H)
#define EXT
#define INIT(s) =s


EXT bool hdimg_active INIT(0);

EXT MEM_ADDRESS os_hdimg_init_vector INIT(0),os_hdimg_bpb_vector INIT(0),os_hdimg_rw_vector INIT(0),
              os_hdimg_boot_vector INIT(0),os_hdimg_mediach_vector INIT(0);

#undef EXT
#undef INIT

#endif

//---------------------------------------------------------------------------
void hdimg_init_vectors()
{
  os_hdimg_init_vector=LPEEK(0x46a);
  os_hdimg_bpb_vector=LPEEK(0x472);
  os_hdimg_rw_vector=LPEEK(0x476);
  os_hdimg_boot_vector=LPEEK(0x47a);
  os_hdimg_mediach_vector=LPEEK(0x47e);
}
//---------------------------------------------------------------------------
void hdimg_reset()
{
  os_hdimg_init_vector=0;
  os_hdimg_bpb_vector=0;
  os_hdimg_rw_vector=0;
  os_hdimg_boot_vector=0;
  os_hdimg_mediach_vector=0;
}
//---------------------------------------------------------------------------
void hdimg_intercept(MEM_ADDRESS pc)
{
  if (pc==os_hdimg_init_vector){
    hdimg_intercept_init();
  }else if (pc==os_hdimg_bpb_vector){
  }else if (pc==os_hdimg_rw_vector){
  }else if (pc==os_hdimg_boot_vector){
  }else if (pc==os_hdimg_mediach_vector){
  }
}
//---------------------------------------------------------------------------
void hdimg_intercept_init()
{
}
//---------------------------------------------------------------------------

