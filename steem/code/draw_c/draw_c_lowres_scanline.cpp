/*---------------------------------------------------------------------------
FILE: draw_c_lowres_scanline.cpp
MODULE: draw_c
DESCRIPTION: Low res C++ drawing routine
---------------------------------------------------------------------------*/

//void draw_scanline_lowres_pixelwise_8(int border1,int picture,int border2,int hscroll){
  int n;
#if defined(SSE_DRAW_C_390C) && defined(SSE_DRAW_C_390_INTRINSICS)
  WORD w[4], mask;
#else
  WORD w0,w1,w2,w3,mask;
#endif
  MEM_ADDRESS source;
  GET_START(0,160)
#if defined(SSE_DRAW_C_390B)
  DRAW_BORDER_PIXELS(border1)
#else
  n=border1/16;DRAW_BORDER(n);
  for(border1&=15;border1>0;border1--){
    DRAWPIXEL(PCpal);
  }
#endif
  if(picture){
    n=16-hscroll;
    if(picture<n)n=picture;
    if(n<16){ //draw a bit of a raster
      picture-=n;
      GET_SCREEN_DATA_INTO_REGS_AND_INC_SA
#if defined(SSE_DRAW_C_390_INTRINSICSB)
      mask = WORD(15-hscroll);
#else
      mask=WORD(0x8000 >> hscroll);
#endif
      for(;n>0;n--){
        CALC_COL_LOWRES_AND_DRAWPIXEL(mask);
#if defined(SSE_DRAW_C_390_INTRINSICSB)
        mask--;
#else
        mask>>=1;
#endif
      }
    }
    for(n=picture/16;n>0;n--){
      GET_SCREEN_DATA_INTO_REGS_AND_INC_SA
#if defined(SSE_DRAW_C_390A) //anyway, to reduce bloat
#if defined(SSE_DRAW_C_390_INTRINSICSB)
      for(int mask = 15;(signed)mask>=0;mask--)
#else
      for(int mask=BIT_15;mask;mask>>=1)
#endif
      {
        CALC_COL_LOWRES_AND_DRAWPIXEL(mask);
      }
#else
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_15);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_14);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_13);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_12);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_11);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_10);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_9);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_8);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_7);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_6);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_5);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_4);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_3);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_2);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_1);
      CALC_COL_LOWRES_AND_DRAWPIXEL(BIT_0);
#endif
    }
    picture&=15;
    if(picture){
      GET_SCREEN_DATA_INTO_REGS_AND_INC_SA
#if defined(SSE_DRAW_C_390_INTRINSICSB)
      mask = 15;
#else
      mask=0x8000;
#endif
      for(;picture>0;picture--){
        CALC_COL_LOWRES_AND_DRAWPIXEL(mask);
#if defined(SSE_DRAW_C_390_INTRINSICSB)
        mask--;
#else
        mask>>=1;
#endif
      }
    }
  }
#if defined(SSE_DRAW_C_390B)
  DRAW_BORDER_PIXELS(border2)
#else
  n=border2/16;DRAW_BORDER(n);
  for(border2&=15;border2>0;border2--){
    DRAWPIXEL(PCpal);
  }
#endif
