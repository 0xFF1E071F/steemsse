/* <<<                                  */
/* Copyright (c) 1994-1996 Arne Riiber. */
/* All rights reserved.                 */
/* >>>                                  */
#include <stdio.h>

#include "defs.h"
#include "chip.h"
#include "memory.h" /* ireg_getb/putb */
#include "sci.h"
#include "timer.h"
#include "ireg.h"

#ifdef USE_PROTOTYPES
#endif


/*
 * Start/end of internal register block
 */
u_int ireg_start = 0;
u_char  iram[NIREGS];

#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif

/*  ST
    Internal registers are tied to ST hardware. We add functions that
    hopefully should give the correct values when reading DR1, 2 & 4.
    DR3 is never read (ASSERTed).
    Much of this is based on doc translated by Tobe (AF) from French ST Mag 
    articles by Stephane  Catala. 
    I also had a look at a scan of the original articles.
    Emulation still far from perfect.
*/

static dr1_getb P_((u_int offs));
static dr2_getb P_((u_int offs));
static dr4_getb P_((u_int offs));

/*  DR1
    This is the funniest part.
    To read the keyboard, one uses DR1, DR3 & DR4 and their associated direction
    registers DDR1, DDR3 & DDR4.
    All bits of DR1 that are cleared in DDR1 (meaning input) are set. 
    But if a key is pressed in the corresponding row (see table), and the bit in 
    DDR3 or DDR4 corresponding to the column is set, and the corresponding 
    bit in DR3 or DR4 is set [apparently], the bit in DR1 is cleared.
    It's rather complicated, but not hard to emulate. 
    To do that we use a look-up table based on doc. 
    Note that the first row had to be shifted on the right compared with the 
    existing doc.
    We also use a table with key states (already used in Steem), and a function 
    that builds DR1 bit by bit. 
    It's slow but performance isn't needed for the keyboard.
    TODO we could fetch the values in the ROM F319-> F370 + special, but then
    we depend on the ROM.
*/

int dr_table[8][15] = {
  //               DR3                |                  DR4                  |
  //  1 |  2 |  3 |  4 |  5 |  6 |  7 |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |
  //  FD   FB   F7   EF   DF   BF   7F   FE   FD   FB   F7   EF   DF   BF   7F
  // 002  004  008  010  020  040  080  001  002  004  008  010  020  040  080

  //  0    1    2    3    4    5    6    7    8    9   10   11   12   13   14
  ///////////////////////////////////////////////////////////////////////////// DR1
  {0x00,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,0x44,0x62,0x61,0x63,0x65},// 0
  {0x00,0x00,0x00,0x00,0x01,0x03,0x05,0x07,0x09,0x0B,0x0D,0x0E,0x48,0x64,0x66},// 1
  {0x00,0x00,0x00,0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x29,0x53,0x47,0x67,0x69},// 2
  {0x00,0x00,0x00,0x00,0x0F,0x11,0x13,0x15,0x16,0x18,0x1A,0x52,0x4B,0x68,0x4A},// 3
  {0x1D,0x00,0x00,0x00,0x10,0x12,0x14,0x22,0x17,0x19,0x1B,0x2B,0x50,0x6A,0x6C},// 4
  {0x00,0x2A,0x00,0x00,0x1E,0x1F,0x21,0x23,0x24,0x26,0x27,0x1C,0x4D,0x6B,0x4E},// 5
  {0x00,0x00,0x38,0x00,0x60,0x20,0x2E,0x30,0x25,0x33,0x34,0x28,0x6D,0x6E,0x6F},// 6
  {0x00,0x00,0x00,0x36,0x2C,0x2D,0x2F,0x31,0x32,0x39,0x3A,0x35,0x70,0x71,0x72} // 7
  };

/*

SCAN CODES (looks great in VC6 IDE with tab=2)

01	Esc	1B	]	            35	/		          4F	{ NOT USED }
02	1 	1C	RET	          36	(RIGHT) SHIFT	50	DOWN ARROW
03	2	  1D	CTRL	        37	{ NOT USED }	51	{ NOT USED }
04	3 	1E	A	            38	ALT		        52	INSERT
05	4 	1F	S	            39	SPACE BAR	    53	DEL
06	5 	20	D	            3A	CAPS LOCK	    54	{ NOT USED }
07	6 	21	F	            3B	F1		        5F	{ NOT USED }
08	7 	22	G	            3C	F2		        60	ISO KEY
09	8	  23	H	            3D	F3		        61	UNDO
0A	9	  24	J	            3E	F4		        62	HELP
0B	0	  25	K	            3F	F5		        63	KEYPAD (
0C	-	  26	L	            40	F6		        64	KEYPAD /
0D	==	27	;	            41	F7		        65	KEYPAD *
0E	BS	28	'	            42	F8		        66	KEYPAD *
0F	TAB	29	`	            43	F9		        67	KEYPAD 7
10	Q	  2A	(LEFT) SHIFT	44	F10	          68	KEYPAD 8
11	W	  2B	\	            45	{ NOT USED }	69	KEYPAD 9
12	E	  2C	Z	            46	{ NOT USED }	6A	KEYPAD 4
13	R	  2D	X	            47	HOME		      6B	KEYPAD 5
14	T	  2E	C	            48	UP ARROW	    6C	KEYPAD 6
15	Y	  2F	V	            49	{ NOT USED }	6D	KEYPAD 1
16	U	  30	B	            4A	KEYPAD -	    6E	KEYPAD 2
17	I	  31	N	            4B	LEFT ARROW	  6F	KEYPAD 3
18	O	  32	M	            4C	{ NOT USED }	70	KEYPAD 0
19	P	  33	,	            4D	RIGHT ARROW	  71	KEYPAD .
1A	[	  34	.	            4E	KEYPAD +	    72	KEYPAD ENTER
 
*/

static dr1_getb (offs)
u_int offs;
{
  u_char value=0xFF;
  u_char  ddr1=iram[DDR1];
  u_char  ddr3=iram[DDR3];
  u_char  ddr4=iram[DDR4];
  u_char  dr2=iram[P2];
  u_char  dr3=iram[P3];
  u_char  dr4=iram[P4];
  int dr1bit;
  int mask;
  ASSERT(offs==P1);
  ASSERT(!ddr1); // strong
//  ASSERT(!(dr2&1)); // strong, asserts at reset?

   // We make DR1 bit by bit
  for(dr1bit=0;dr1bit<8;dr1bit++)
  {
    mask=1<<dr1bit; 
    // we only consider bits with a corresponding 0 in DDR1
    if(!(mask&ddr1) && !(dr2&1))
    {
      int column,mask2,found=0; // 'found' avoids being too slow
      // test DR3
      for(column=0;column<7&&!found;column++)
      {
        mask2=1<<(column+1);
        if(ST_Key_Down[ dr_table[dr1bit][column] ] 
          &&  (dr3&mask2)   // must be set?, ST Mag doc said cleared
          &&  (ddr3&mask2)
          )
          found++;
      }
      // test DR4
      for(column=7;column<15&&!found;column++)
      {
        mask2=1<<(column-7);
        if(ST_Key_Down[ dr_table[dr1bit][column] ] 
          &&  (dr4&mask2)   // must be set?
          &&  (ddr4&mask2)
          )
          found++;
      }
      if(found)
      {
        value&=~mask; // clear bit
#if defined(SS_IKBD_6301_TRACE_KEYS)
        TRACE("Read DR1 %X\n",value);
#endif
      }
    }
  }
  ASSERT(value);
  return value;
}
 
/*  DR2 ($03 DDR2: $01)
    Bit 0 : Joystick 1 pin 5, output for selecting the 74LS244 (see DR4)
    Bit 1 : Left mouse button or joystick 0 Fire button.
    Bit 2 : Right mouse button or joystick 1 Fire button.
    Bit 3 : Acia TxDATA (6301 receive)
    Bit 4 : Acia RxDATA (6301 transmit)

    The ROM may write either 1 or 0 on DR2.
    When we read, DDR2 is always 1.
    Button pressed -> bit cleared (not set)
    mousek 1 ->   value 2
    mouesk 2 ->   value 4
    mousek 3 ->   value 0
*/

static dr2_getb (offs)
  u_int offs;
{
  u_char value;
#if !defined(NDEBUG)
  u_char ddr2=iram[DDR2];
  //ASSERT(ddr2==1); // strong
#endif
  //ASSERT(offs==P2);
  value=0xFF;
  if(mousek) // clear the correct bit (see above)
  {
    value=(mousek*2)%6;
//    TRACE("HD6301 handling mousek %x -> %x\n",mousek,value);
  }
  return value;
}

/*  DR4 ($07 DDR4: $05)

    This register is also used to read joysticks directions.
    For this you need to select the 74LS244 by setting the bit 0 of DR2 to 0,
    [in fact this bit should be set?]
    on output, then turn the DDR4 to all input (%00000000). 
    The four directions of each joystick are then readable on bits [0-3] and 
    [4-7].
    For keyboard handling, see DR1, DR3.

    Bit 0: XB & Up0
    Bit 1: XA & Down0
    Bit 2: YA & Left0
    Bit 3: YB & Right0
    Bit 4: Up1            NOT $10 = $EF
    Bit 5: Down1          NOT $20 = $DF
    Bit 6: Left1          NOT $40 = $BF
    Bit 7: Right1         NOT $80 = $7F
    
    XB, XA, YA, YB (mouse) are mapped on bits 4-7 if mouse is in port 1 
    (not emulated, no use)
    X# horizontal movement
    Y# vertical movement
*/

#define MOUSE_MASK 0x33333333 // series of 11001100... for rotation
static unsigned int mouse_x_counter=MOUSE_MASK;
static unsigned int mouse_y_counter=MOUSE_MASK;
static int mouse_click_x_time=0;
static int mouse_click_y_time=0; 

static dr4_getb (offs)
  u_int offs;
{
  u_char value;
  u_char  ddr2=iram[DDR2];
  u_char  ddr4=iram[DDR4];
  u_char  dr2=iram[P2];
  int joy0mvt=0,joy1mvt=0;
  ASSERT(offs==P4);
  ASSERT(!ddr4); // strong
  value=0xFF;
                
/*  Mouse movements
    As the mouse ball rotates, two axes spin and cause the logical rotation
    of a 0011 bit sequence in the hardware, two bits going to the 
    registry when read. To emulate this, we rotate a $3 (0011) sequence and
    send the last bits to registry bits 0-1 for horizontal movement, 2-3
    for vertical movement. 
    If we test for DR2 bit 0 set, ok for desktop but it breaks Froggies.
    The way we emulate this with 'n_chunk' speeds and 'cycles_per_chunk'
    cycles of delay between rotations is gross and will have you killed in
    Arkanoid (he he). We wanted to have a mouse slow enough for drawing 
    (NEOchrome) and fast enough that it feels OK in GEM.
    It seems to be the hardest part (also not perfect in SainT), but maybe we
    should make sure the send/receive timings are correct before we fiddle 
    again with mouse speed.
    TODO: speed, accuracy...
*/
  if(!(ddr4&0xF) && (ddr2&1) 
    && (HD6301.MouseVblDeltaX || HD6301.MouseVblDeltaY) )
  {
    int n_chunk=HD6301_MOUSE_SPEED_CHUNKS; // 20 // 15
    int cycles_per_chunk=HD6301_MOUSE_SPEED_CYCLES_PER_CHUNK;// 1250;// 500 
    int movement,cycles_for_a_click;
    if(HD6301.MouseVblDeltaX) // horizontal
    { 
      int amx=abs(HD6301.MouseVblDeltaX);
      movement=__min((int)amx,n_chunk-1);
      cycles_for_a_click=cycles_per_chunk*(n_chunk-movement);
      if(cpu.ncycles-mouse_click_x_time>cycles_for_a_click)
      {
        if(HD6301.MouseVblDeltaX<0) // left
          mouse_x_counter=_rotl(mouse_x_counter,1);
        else  // right
          mouse_x_counter=_rotr(mouse_x_counter,1);
        mouse_click_x_time=cpu.ncycles;
      }
    }
    
    if(HD6301.MouseVblDeltaY) // vertical
    {
      int amy=abs(HD6301.MouseVblDeltaY);
      movement=__min(amy,n_chunk-1);
      cycles_for_a_click=cycles_per_chunk*(n_chunk-movement);
      if(cpu.ncycles-mouse_click_y_time>cycles_for_a_click)
      {
        if(HD6301.MouseVblDeltaY<0) // up
          mouse_y_counter=_rotl(mouse_y_counter,1);
        else  // down
          mouse_y_counter=_rotr(mouse_y_counter,1);
        mouse_click_y_time=cpu.ncycles;
      }
    }   
  }

/*  Joystick movements
    Movement is signalled by cleared bits.
*/
  ASSERT(!ddr4);
  if(!ddr4 && (ddr2&1) && (dr2&1))
  {
    joy0mvt=stick[0]&0xF; // eliminate fire info
    joy1mvt=stick[1]&0xF;
    if(joy0mvt||joy1mvt)
    {
      value=0; // not always right (game mouse+joy?) but can't do better yet
      if(joy0mvt) // for games using joystick 0
        mouse_y_counter=mouse_x_counter=MOUSE_MASK; 
      value|= joy0mvt | ( joy1mvt<<4);
      value=~value;
      //TRACE("stick %X\n",value);
    }
  }  

  // We do it that way because mouse speed is better if we don't update that
  // value only when the mouse has moved.
  if(!joy0mvt)
    value = (value&(~0xF))  | (mouse_x_counter&3) | ((mouse_y_counter&3)<<2);

  iram[offs]=value;
  return value;
}


static port_putb P_((u_int offs, u_char value));

static
port_putb (offs, value)
  u_int  offs;
  u_char value;
{
  /* Only change output ports - xor with DDR */
  ireg_putb (offs, value ^ ireg_getb (offs - 2));
}


/*
 *  Pointers to functions to be called for reading internal registers
 *  ST: notice the addition of dr#_getb function pointers
 */
int (*ireg_getb_func[NIREGS]) P_((u_int offs)) = {
/* 0x00 */
  0,          0,  dr1_getb ,  dr2_getb ,
  0,          0,          0,  dr4_getb ,
  tcsr_getb,  0,          0,          0,
  0,          0,          0,          0,
/* 0x10 */
  0,          trcsr_getb, rdr_getb,   0,
  0
};


/*
 *  Pointers to functions to be called for writing internal registers
*/
int (*ireg_putb_func[NIREGS]) P_((u_int offs, u_char val)) = {
/* 0x00 */
  0,          0,          port_putb,   port_putb,
  0,          0,          port_putb,   port_putb,
  tcsr_putb,  0,          0,           ocr_putb,
  ocr_putb,   0,          0,           0,
/* 0x10 */
  0,          trcsr_putb, 0,           tdr_putb,
  0
};

#undef P_
