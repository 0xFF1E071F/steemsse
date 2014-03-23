/*---------------------------------------------------------------------------
FILE: ikbd.cpp
MODULE: emu
DESCRIPTION: The code to emulate the ST's Intellegent Keyboard Controller
(Motorola 6301) that encompasses mouse, keyboard and joystick input.
Note that this is functional rather than hardware-level emulation.
Reprogramming is not implemented. 
SS: 
Added doc, trace of commands
V3.3, fake emu borrowed from Hatari for reprogramming
V3.4, hardware-level emulation
V3.5.1: fake emu for reprogramming nuked (avoid bloat)
V3.6.0: all non 6301 true emu mods removed (ACIA + IKBD)
---------------------------------------------------------------------------*/

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_INFO)
#pragma message("Included for compilation: ikbd.cpp")
#endif

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_IKBD_H)

#define EXT
#define INIT(s) =s


#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
//extern "C" int ST_Key_Down[128];
#else
EXT bool ST_Key_Down[128];
#endif
EXT int disable_input_vbl_count INIT(0);
EXT int ikbd_joy_poll_line INIT(0),ikbd_mouse_poll_line INIT(0),ikbd_key_poll_line INIT(0);

EXT BYTE keyboard_buffer[MAX_KEYBOARD_BUFFER_SIZE];
EXT int keyboard_buffer_length;

EXT int mouse_speed INIT(10);

EXT int mouse_move_since_last_interrupt_x,mouse_move_since_last_interrupt_y;
EXT bool mouse_change_since_last_interrupt;
#if !(defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301))
EXT int mousek;
#endif
#ifndef CYGWIN
EXT int no_set_cursor_pos INIT(0);
#else
EXT int no_set_cursor_pos INIT(true);
#endif

//#ifdef IN_EMU

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

BYTE key_table[256]={
//         $.0  $.1  $.2  $.3  $.4  $.5  $.6  $.7  $.8  $.9  $.a  $.b  $.c  $.d  $.e  $.f
/* $0. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e,0x0f,0x00,0x00,0x00,0x1c,0x00,0x00,
/* $1. */ 0x2a,0x1d,0x38,0x00,0x3a,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
/* $2. */ 0x39,0x62,0x61,0x00,0x47,0x4b,0x48,0x4d,0x50,0x00,0x00,0x00,0x00,0x52,0x53,0x62,
//    PageUp=Help^    ^Page Down=Undo
/* $3. */ 0x0b,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,
/* $4. */ 0x00,0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,
/* $5. */ 0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c,0x00,0x00,0x00,0x00,0x00,
/* $6. */ 0x70,0x6d,0x6e,0x6f,0x6a,0x6b,0x6c,0x67,0x68,0x69,0x66,0x4e,0x00,0x4a,0x71,0x65,
/* $7. */ 0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x63,0x64,0x00,0x00,0x00,0x00,
//                                                           ^F11 ^F12 (numpad brackets)
/* $8. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $9. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $a. */ 0x2a,0x36,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//        ^ Left/Right distinguishing constants (doesn't work for most things that take VK codes)
/* $b. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $c. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $d. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $e. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* $f. */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
                    };

#ifdef WIN32
// When task switching is turned off we have to manually update these keys at the VBL
BYTE TaskSwitchVKList[4]={VK_ESCAPE,VK_TAB,VK_DELETE,0};
bool CutTaskSwitchVKDown[4]={0,0,0,0};
#endif


const int ikbd_clock_days_in_mon[13]={0,31,28,31,30,/*MAY!*/31,30,31,31,30,31,30,31};
const int ikbd_clock_max_val[6]={99,12,0,23,59,59};

IKBD_STRUCT ikbd;

#if (defined(STEVEN_SEAGAL) && defined(SS_ACIA)) 
ACIA_STRUCT acia[2];
#endif



//#endif

#undef EXT
#undef INIT


#endif

#define LOGSECTION LOGSECTION_IKBD
//---------------------------------------------------------------------------
void ikbd_run_start(bool Cold)
{
  if (Cold){
    keyboard_buffer_length=0;
    keyboard_buffer[0]=0;
//    ZeroMemory(ST_Key_Down,sizeof(ST_Key_Down));
  }else{
    UpdateSTKeys();
  }
  mouse_move_since_last_interrupt_x=0;
  mouse_move_since_last_interrupt_y=0;
  mouse_change_since_last_interrupt=false;
  JoyGetPoses();
}
//---------------------------------------------------------------------------
bool ikbd_keys_disabled()
{
  return ikbd.joy_mode>=100;
}
//---------------------------------------------------------------------------
void IKBD_VBL()
{
  if ((++ikbd.clock_vbl_count)>=shifter_freq_at_start_of_vbl){
    ikbd.clock_vbl_count=0;

    for (int n=5;n>=0;n--){
      int val=(ikbd.clock[n] >> 4)*10 + (ikbd.clock[n] & 0xf);
      int max_val=ikbd_clock_max_val[n];
      if (max_val==0){
        int mon=min((ikbd.clock[1] >> 4)*10 + (ikbd.clock[1] & 0xf),12);
        max_val=ikbd_clock_days_in_mon[mon];
      }
      bool increase_next=0;
      if ((++val)>max_val){
        val=0;if (n==1) val=1;
        increase_next=true;
      }
      if (n==0) val%=100;
      ikbd.clock[n]=BYTE((val % 10) | ((val/10) << 4));
      if (increase_next==0) break;
    }
  }
  if (macro_start_after_ikbd_read_count) return;

  // ikbd_poll_scanline determines where on the screen the IKBD has polled the keyboard,
  // joystick and mouse. Steem always polls at the VBL but that can cause problems for
  // some programs, so we delay sending the notifications. 
  // [SS now we poll once in the frame too for Corporation STE but it has its 
  // own problems (HighResMode!) -> hacks]

  int max_line=scanlines_above_screen[shifter_freq_idx]+(MONO ? 400:200);
  ikbd_joy_poll_line+=527;       // SS we keep these despite polling more
  ikbd_joy_poll_line%=max_line;  // or Arctic Moves is broken, certainly
  ikbd_key_poll_line+=793;       // many more
  ikbd_key_poll_line%=max_line;
  ikbd_mouse_poll_line+=379;
  ikbd_mouse_poll_line%=max_line;

  if (macro_play_has_keys) macro_play_keys();

  static BYTE old_stick[2];
  old_stick[0]=stick[0];
  old_stick[1]=stick[1];
  bool old_joypar1_bit4=(stick[N_JOY_PARALLEL_1] & BIT_4)!=0;

  if (macro_play_has_joys){
    macro_play_joy();
  }else{
    if (disable_input_vbl_count==0){
      joy_read_buttons();
      for (int Port=0;Port<8;Port++) stick[Port]=joy_get_pos(Port);
    }else{
      for (int Port=0;Port<8;Port++) stick[Port]=0;
    }
    if (IsJoyActive(N_JOY_PARALLEL_0)) stick[N_JOY_PARALLEL_0]|=BIT_4;
    if (IsJoyActive(N_JOY_PARALLEL_1)) stick[N_JOY_PARALLEL_1]|=BIT_4;
  }
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
  if(HD6301EMU_ON) ; else // we write no packets ourselves, the ROM will do it
#endif
  switch (ikbd.joy_mode){
    case IKBD_JOY_MODE_DURATION:
      keyboard_buffer_write( BYTE(int((stick[0] & MSB_B) ? BIT_1:0) | int((stick[1] & MSB_B) ? BIT_0:0)) );
      keyboard_buffer_write( BYTE(((stick[0] & 0xf) << 4) | (stick[1] & 0xf)) );
      break;
    case IKBD_JOY_MODE_AUTO_NOTIFY:
    {
      int j=int(ikbd.port_0_joy ? 0:1);
      for (;j<2;j++){
        BYTE os=old_stick[j],s=stick[j];
        // If mouse active then joystick button never down
        if (ikbd.port_0_joy==0) os&=0x0f, s&=0x0f;

        if (os!=s) agenda_add(ikbd_send_joystick_message,ikbd_joy_poll_line,j);
      }
      break;
    }
    case IKBD_JOY_MODE_FIRE_BUTTON_DURATION:
      if (stick[1] & MSB_B){
        keyboard_buffer_write_string(0xff,0xff,0xff,0xff,0xff,0xff,-1);
      }else{
        keyboard_buffer_write_string(0,0,0,0,0,0,-1);
      }
      break;
    case IKBD_JOY_MODE_CURSOR_KEYS:
    {
      if (stick[0] & (~old_stick[0]) & 0xc){ //new press left/right
        ikbd.cursor_key_joy_ticks[0]=timeGetTime(); //reset timer left/right
        ikbd.cursor_key_joy_ticks[2]=timeGetTime(); //last report
        if (stick[0] & 4){
          keyboard_buffer_write(0x4b);
          keyboard_buffer_write(0x4b | MSB_B);
        }else{
          keyboard_buffer_write(0x4d);
          keyboard_buffer_write(0x4d | MSB_B);
        }
      }else if (stick[0] & (~old_stick[0]) & 0x3){
        ikbd.cursor_key_joy_ticks[1]=timeGetTime(); //reset timer up/down
        ikbd.cursor_key_joy_ticks[3]=timeGetTime(); //last report
        if (stick[0] & 1){
          keyboard_buffer_write(0x48);
          keyboard_buffer_write(0x48 | MSB_B);
        }else{
          keyboard_buffer_write(0x50);
          keyboard_buffer_write(0x50 | MSB_B);
        }
      }else if (stick[0]){
        for (int xy=0;xy<2;xy++){
          BYTE s=stick[0] & BYTE(0xc >> (xy*2));
          if (s){ //one of these directions pressed
            DWORD interval=(timeGetTime()-ikbd.cursor_key_joy_ticks[2+xy])/100;
            DWORD elapsed=(timeGetTime()-ikbd.cursor_key_joy_ticks[xy])/100;
            bool report=false;
            BYTE key;
            if (elapsed>ikbd.cursor_key_joy_time[xy]){ //>Rx
              if (interval>ikbd.cursor_key_joy_time[2+xy]){ //Tx
                report=true;
              }
            }else if (interval>ikbd.cursor_key_joy_time[4+xy]){ //Vx
              report=true;
            }
            if (report){
              if (s & 8) key=0x4d;
              else if (s & 4) key=0x4b;
              else if (s & 2) key=0x50;
              else key=0x48;
              keyboard_buffer_write(key);
              keyboard_buffer_write(key | MSB_B);
              ikbd.cursor_key_joy_ticks[2+xy]=timeGetTime();
            }
          }
        }
      }
      break;
    }
  }

  if (macro_record){
    macro_jagpad[0]=GetJagPadDown(N_JOY_STE_A_0,0xffffffff);
    macro_jagpad[1]=GetJagPadDown(N_JOY_STE_B_0,0xffffffff);
    macro_record_joy();
  }

  // Handle io line for parallel port joystick 1 (busy bit cleared if fire is pressed)
  if (stick[N_JOY_PARALLEL_1] & BIT_4){
    mfp_gpip_set_bit(0,bool((stick[N_JOY_PARALLEL_1] & BIT_7))==0);
  }else if (old_joypar1_bit4){
    UpdateCentronicsBusyBit();
  }

  { //SS? there was an if?
    int old_mousek=mousek;
    mousek=0;
    if (stick[0] & 128) mousek|=BIT_LMB;
    if (stick[1] & 128) mousek|=BIT_RMB;

    if (stem_mousemode==STEM_MOUSEMODE_WINDOW){
      POINT pt;
      GetCursorPos(&pt);
      if (pt.x!=window_mouse_centre_x || pt.y!=window_mouse_centre_y){
        // disable_input_vbl_count is used when you reset to prevent TOS getting IKBD messages
        // before it is ready (causes annoying clicking). It is also used when you change disk
        // in order to prevent you from continuing before the new disk has been fully inserted.
        // In the latter case we do not need to disable mouse movement.
        if (disable_input_vbl_count<=30){
          mouse_move_since_last_interrupt_x+=(pt.x-window_mouse_centre_x);
          mouse_move_since_last_interrupt_y+=(pt.y-window_mouse_centre_y);
          if (mouse_speed!=10){
            int x_if_0=0;
            if (mouse_move_since_last_interrupt_x<0) x_if_0=-1;
            if (mouse_move_since_last_interrupt_x>0) x_if_0=1;

            int y_if_0=0;
            if (mouse_move_since_last_interrupt_y<0) y_if_0=-1;
            if (mouse_move_since_last_interrupt_y>0) y_if_0=1;

            mouse_move_since_last_interrupt_x*=mouse_speed;
            mouse_move_since_last_interrupt_y*=mouse_speed;
            mouse_move_since_last_interrupt_x/=10;
            mouse_move_since_last_interrupt_y/=10;
            if (mouse_move_since_last_interrupt_x==0) mouse_move_since_last_interrupt_x=x_if_0;
            if (mouse_move_since_last_interrupt_y==0) mouse_move_since_last_interrupt_y=y_if_0;
          }
          if (ikbd.mouse_upside_down){
            mouse_move_since_last_interrupt_y=-mouse_move_since_last_interrupt_y;
          }
          mouse_change_since_last_interrupt=true; 
        }
        
        if (no_set_cursor_pos){
          window_mouse_centre_x=pt.x;
          window_mouse_centre_y=pt.y;
        }else{
          SetCursorPos(window_mouse_centre_x,window_mouse_centre_y);
        }
      }
    }
    if (macro_record){
      macro_record_mouse(mouse_move_since_last_interrupt_x,mouse_move_since_last_interrupt_y);
    }
    if (macro_play_has_mouse){
      mouse_change_since_last_interrupt=0;
      macro_play_mouse(mouse_move_since_last_interrupt_x,mouse_move_since_last_interrupt_y);
      if (mouse_move_since_last_interrupt_x || mouse_move_since_last_interrupt_y){
        mouse_change_since_last_interrupt=true;
      }
    }

    int report_button_abs=0;
    if (mousek!=old_mousek){
      bool send_change_for_button=true;

      // Handle absolute mouse button flags
      if (RMB_DOWN(mousek) && RMB_DOWN(old_mousek)==0) report_button_abs|=BIT_0;
      if (RMB_DOWN(mousek)==0 && RMB_DOWN(old_mousek)) report_button_abs|=BIT_1;
      if (LMB_DOWN(mousek) && LMB_DOWN(old_mousek)==0) report_button_abs|=BIT_2;
      if (LMB_DOWN(mousek)==0 && LMB_DOWN(old_mousek)) report_button_abs|=BIT_3;
      ikbd.abs_mousek_flags|=report_button_abs;

      // Handle mouse buttons as keys
      if (ikbd.mouse_button_press_what_message & BIT_2){
        if (2 & (mousek^old_mousek)) keyboard_buffer_write(BYTE((mousek & 2) ? 0x74:0xf4)); //if mouse button 1
        if (1 & (mousek^old_mousek)) keyboard_buffer_write(BYTE((mousek & 1) ? 0x75:0xf5)); //if mouse button 2
        send_change_for_button=0; // Don't send mouse packet if you haven't moved mouse
        report_button_abs=0; // No ABS reporting
      }else if (ikbd.mouse_mode==IKBD_MOUSE_MODE_ABSOLUTE){
        if ((ikbd.mouse_button_press_what_message & BIT_0)==0){ // Don't report ABS on press
          report_button_abs&=~(BIT_0 | BIT_2);
        }
        if ((ikbd.mouse_button_press_what_message & BIT_1)==0){ // Don't report ABS on release
          report_button_abs&=~(BIT_1 | BIT_3);
        }
      }else{
        report_button_abs=0;
      }

      if (send_change_for_button) mouse_change_since_last_interrupt=true;
    }

    if (mouse_change_since_last_interrupt){
      int max_mouse_move=IKBD_DEFAULT_MOUSE_MOVE_MAX; //15
      if (macro_play_has_mouse) max_mouse_move=macro_play_max_mouse_speed;
      ikbd_mouse_move(mouse_move_since_last_interrupt_x,mouse_move_since_last_interrupt_y,mousek,max_mouse_move);
      mouse_change_since_last_interrupt=false;
      mouse_move_since_last_interrupt_x=0;
      mouse_move_since_last_interrupt_y=0;
    }
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
    else if(HD6301EMU_ON)
    {
      HD6301.MouseVblDeltaX=HD6301.MouseVblDeltaY=0;
    }
#endif

    if (report_button_abs){
      for (int bit=BIT_0;bit<=BIT_3;bit<<=1){
        if (report_button_abs & bit) ikbd_report_abs_mouse(report_button_abs & bit);
      }
    }
  }

  if (macro_play_has_keys==0){
    // Check modifier keys, it's simpler to check them like this rather than
    // respond to messages
    MODIFIERSTATESTRUCT mss=GetLRModifierStates();
    bool StemWinActive=GUICanGetKeys(); 

    if (joy_is_key_used(VK_SHIFT) || CutDisableKey[VK_SHIFT] || StemWinActive==0){
      mss.LShift=0;
      mss.RShift=0;
    }
    if (joy_is_key_used(VK_LSHIFT) || CutDisableKey[VK_LSHIFT]) mss.LShift=0;
    if (joy_is_key_used(VK_RSHIFT) || CutDisableKey[VK_RSHIFT]) mss.RShift=0;

    if (joy_is_key_used(VK_CONTROL) || CutDisableKey[VK_CONTROL] || StemWinActive==0){
      mss.LCtrl=0;
      mss.RCtrl=0;
    }
    if (joy_is_key_used(VK_LCONTROL) || CutDisableKey[VK_LCONTROL]) mss.LCtrl=0;
    if (joy_is_key_used(VK_RCONTROL) || CutDisableKey[VK_RCONTROL]) mss.RCtrl=0;

    if (joy_is_key_used(VK_MENU) || CutDisableKey[VK_MENU] || StemWinActive==0){
      mss.LAlt=0;
      mss.RAlt=0;
    }
    if (joy_is_key_used(VK_LMENU) || CutDisableKey[VK_LMENU]) mss.LAlt=0;
    if (joy_is_key_used(VK_RMENU) || CutDisableKey[VK_RMENU]) mss.RAlt=0;

    int ModDown=ExternalModDown | CutModDown;
    if (ModDown & b00000001) mss.LShift=true;
    if (ModDown & b00000010) mss.RShift=true;
    if (ModDown & b00001100) mss.LCtrl=true;
    if (ModDown & b00110000) mss.LAlt=true;

#if defined(STEVEN_SEAGAL) && defined(SS_VAR_REWRITE)
    if((bool)ST_Key_Down[key_table[VK_LSHIFT]]!=mss.LShift){
#else
    if (ST_Key_Down[key_table[VK_LSHIFT]]!=mss.LShift){
#endif
      HandleKeyPress(VK_LSHIFT,mss.LShift==0,IGNORE_EXTEND);
    }
#if defined(STEVEN_SEAGAL) && defined(SS_VAR_REWRITE)
    if((bool)ST_Key_Down[key_table[VK_RSHIFT]]!=mss.RShift){
#else
    if (ST_Key_Down[key_table[VK_RSHIFT]]!=mss.RShift){
#endif
      HandleKeyPress(VK_RSHIFT,mss.RShift==0,IGNORE_EXTEND);
    }
    if (ST_Key_Down[key_table[VK_CONTROL]]!=(mss.LCtrl || mss.RCtrl)){
      HandleKeyPress(VK_CONTROL,(mss.LCtrl || mss.RCtrl)==0,IGNORE_EXTEND);
    }
    if (ST_Key_Down[key_table[VK_MENU]]!=(mss.LAlt || mss.RAlt)){
      HandleKeyPress(VK_MENU,(mss.LAlt || mss.RAlt)==0,IGNORE_EXTEND);
    }

#if !defined(ONEGAME) && defined(WIN32)
    if (TaskSwitchDisabled){
      BYTE n=0,Key;
      while (TaskSwitchVKList[n]){
        Key=TaskSwitchVKList[n];
        if (joy_is_key_used(Key)==0 && CutDisableKey[Key]==0 && CutTaskSwitchVKDown[n]==0){
          if (ST_Key_Down[key_table[Key]] != (GetAsyncKeyState(Key)<0)){
            HandleKeyPress(Key,GetAsyncKeyState(Key)>=0,IGNORE_EXTEND);
          }
        }
        n++;
      }
    }
#endif
  }
  macro_advance();
  if (disable_input_vbl_count) disable_input_vbl_count--;
}
//---------------------------------------------------------------------------
/*
  SS (info from this very source)
  IKBD bugs exploited by some games
  It happens when some commands are issued while the IKBD is resetting
  (check ikbd.resetting) , that is between command reset & reset execution
  Psygnosys (Barbarian) : 
  08 Set Mouse Relative, 0B Set Mouse Threshold, 14 Joysticks Notify
  0814: 08 Set Mouse Relative 14 Joysticks Notify (prg?)
  1214: 12 Mouse OFF 14 Joysticks Notify (prg?)
  121A: 12 Mouse OFF 1A Joysticks OFF (prg?)
*/

void ikbd_inc_hack(int &hack_val,int inc_val) // SS using a reference
{
  if (ikbd.resetting==0) return; 
  if (hack_val==inc_val){
    hack_val=inc_val+1;
  }else{
    hack_val=-1;
  }
}
//---------------------------------------------------------------------------

/*  All bytes that are written by the program on address $fffc02 end
    up here, after a transmission delay. In Steem, this happens via
    the agenda (not very precise). TODO: event?
    This function handles fake reprogramming emulation and dispatching
    to the true 6301 emu as well.
*/

void agenda_ikbd_process(int src)    //intelligent keyboard handle byte
{
  log(EasyStr("IKBD: At ")+hbl_count+" receives $"+HEXSl(src,2));
  TRACE_LOG("IKBD RDRS->RDR $%X\n",src);

#if (defined(SS_DEBUG) || defined(SS_IKBD_MOUSE_OFF_JOYSTICK_EVENT)) && defined(SS_ACIA)
  // our powerful 6301 command interpreter, working for both emulations
  HD6301.InterpretCommand(src); 
#endif

#if defined(SS_IKBD_6301)
  if(HD6301EMU_ON  && !HD6301.Crashed)
  {
    ACIA_IKBD.SR|=BIT_1; // TDRE
    if((ACIA_IKBD.CR&BIT_5)&&!(ACIA_IKBD.CR&BIT_6))
    {
      ACIA_IKBD.SR|=BIT_7; 
      mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,0); //trigger IRQ
    }

#if defined(SS_ACIA_DOUBLE_BUFFER_TX)
    /*  If there's a byte in TDR waiting to be shifted, do it now.
    */
    ACIA_IKBD.LineTxBusy=false;
    if(ACIA_IKBD.ByteWaitingTx) 
      HD6301.ReceiveByte(ACIA_IKBD.TDR);
#endif

    //TRACE_LOG("6301 RDRS->RDR %X\n",src);
    hd6301_transmit_byte(src);// send byte to 6301 emu
    
#if defined(SS_IKBD_6301_RUN_CYCLES_AT_IO)//no...
    ASSERT(!HD6301.RunThisHbl); 
#if defined(SS_SHIFTER)
    int n6301cycles=Shifter.CurrentScanline.Cycles/HD6301_CYCLE_DIVISOR;
#else
    int n6301cycles=(screen_res==2) ? 20 : HD6301_CYCLES_PER_SCANLINE; //64
#endif
    if(hd6301_run_cycles(n6301cycles)==-1)
    {
      TRACE_LOG("6301 emu is hopelessly crashed!\n");
      HD6301.Crashed=1; 
    }
    HD6301.RunThisHbl=true; // stupid signal
#endif
    
    // That's it for Steem, the rest is handled by the program in ROM!
    return; 
   
  }
#endif

  ikbd.send_nothing=0;  // This should only happen if valid command is received!
  if(ikbd.command_read_count) // SS: entering command parameters
  { // that is, commands with parameters are handled first (here) 
    if (ikbd.command!=0x50){ //load memory rubbish // SS why 'rubbish'?
      ikbd.command_param[ikbd.command_parameter_counter++]=(BYTE)src;
    }else{
      // Save into IKBD RAM, this is in the strange range $0080 to $00FF (128 bytes)
      if (ikbd.load_memory_address>=0x80 && ikbd.load_memory_address<=0xff){
        ikbd.ram[ikbd.load_memory_address-0x80]=(BYTE)src;
      }
      ikbd.load_memory_address++;
    }
    ikbd.command_read_count--;
    if (ikbd.command_read_count<=0)
    {
      ASSERT(ikbd.command_read_count==0); // we've command & all parameters
      switch(ikbd.command) // SS commands with parameters
      {
/*
SET MOUSE BUTTON ACTION

    0x07
    %00000mss           ; mouse button action
                        ;       (m is presumed = 1 when in MOUSE KEYCODE mode)
                        ; mss=0xy, mouse button press or release causes mouse
                        ;  position report
                        ;  where y=1, mouse key press causes absolute report
                        ;  and x=1, mouse key release causes absolute report
                        ; mss=100, mouse buttons act like keys 

This command sets how the IKBD should treat the buttons on the mouse. 
The default mouse button action mode is %00000000, the buttons are 
treated as part of the mouse logically. When buttons act like keys,
 LEFT=0x74 & RIGHT=0x75.
*/
      case 0x7: // Set what package is returned when mouse buttons are pressed
        ikbd.mouse_button_press_what_message=ikbd.command_param[0];
        break;
/*
SET ABSOLUTE MOUSE POSITIONING
Code:

    0x09
    XMSB                ; X maximum (in scaled mouse clicks)
    XLSB
    YMSB                ; Y maximum (in scaled mouse clicks)
    YLSB
      
Set absolute mouse position maintenance. Resets the IKBD maintained X and Y
 coordinates.
In this mode, the value of the internally maintained coordinates does NOT 
wrap between 0 and large positive numbers. Excess motion below 0 is ignored.
 The command sets the maximum positive value that can be attained in the scaled
 coordinate system. Motion beyond that value is also ignored.
*/
      case 0x9: // Absolute mouse mode
        ikbd.mouse_mode=IKBD_MOUSE_MODE_ABSOLUTE;
        ikbd.abs_mouse_max_x=MAKEWORD(ikbd.command_param[1],ikbd.command_param[0]);
        ikbd.abs_mouse_max_y=MAKEWORD(ikbd.command_param[3],ikbd.command_param[2]);
        ikbd.port_0_joy=false;
        ikbd.abs_mouse_x=ikbd.abs_mouse_max_x/2;
        ikbd.abs_mouse_y=ikbd.abs_mouse_max_y/2;
        ikbd.abs_mousek_flags=0; 
        if (RMB_DOWN(mousek)) ikbd.abs_mousek_flags|=BIT_0;
        if (LMB_DOWN(mousek)) ikbd.abs_mousek_flags|=BIT_2;
        break;
/*
SET MOUSE KEYCODE MODE
Code:

    0x0A
    deltax              ; distance in X clicks to return (LEFT) or (RIGHT)
    deltay              ; distance in Y clicks to return (UP) or (DOWN)
 
Set mouse monitoring routines to return cursor motion keycodes instead of 
either RELATIVE or ABSOLUTE motion records. The IKBD returns the appropriate 
cursor keycode after mouse travel exceeding the user specified deltas in either
 axis. When the keyboard is in key scan code mode, mouse motion will cause the
 make code immediately followed by the break code. Note that this command is not 
affected by the mouse motion origin.
*/
      case 0xa: // Return mouse movements as cursor keys
        ikbd.mouse_mode=IKBD_MOUSE_MODE_CURSOR_KEYS;
        ikbd.cursor_key_mouse_pulse_count_x=max(int(ikbd.command_param[0]),1);
        ikbd.cursor_key_mouse_pulse_count_y=max(int(ikbd.command_param[1]),1);
        ikbd.port_0_joy=false;
        agenda_delete(ikbd_report_abs_mouse);
        break;
/*
SET MOUSE THRESHOLD
Code:

    0x0B
    X                   ; x threshold in mouse ticks (positive integers)
    Y                   ; y threshold in mouse ticks (positive integers)

This command sets the threshold before a mouse event is generated. Note that 
it does NOT affect the resolution of the data returned to the host. This command
 is valid only in RELATIVE MOUSE POSITIONING mode. The thresholds default to 1 at
 RESET (or power-up).
*/
      case 0xb: // Set relative mouse threshold
        ikbd.relative_mouse_threshold_x=ikbd.command_param[0];
        ikbd.relative_mouse_threshold_y=ikbd.command_param[1];
        ikbd_inc_hack(ikbd.psyg_hack_stage,1);
        break;
/*
SET MOUSE SCALE
Code:

    0x0C
    X                   ; horizontal mouse ticks per internel X
    Y                   ; vertical mouse ticks per internel Y

This command sets the scale factor for the ABSOLUTE MOUSE POSITIONING mode.
 In this mode, the specified number of mouse phase changes ('clicks') must
 occur before the internally maintained coordinate is changed by one 
 (independently scaled for each axis). Remember that the mouse position
information is available only by interrogating the IKBD in the
 ABSOLUTE MOUSE POSITIONING mode unless the IKBD has been commanded
 to report on button press or release (see SET MOSE BUTTON ACTION).
*/
      case 0xc://set absolute mouse threshold
        ikbd.abs_mouse_scale_x=ikbd.command_param[0];
        ikbd.abs_mouse_scale_y=ikbd.command_param[1];
        break;
/*
LOAD MOUSE POSITION
Code:

    0x0E
    0x00             	; filler
    XMSB                ; X coordinate
    XLSB                ; (in scaled coordinate system)
    YMSB                ; Y coordinate
    YLSB

This command allows the user to preset the internally maintained 
absolute mouse position.
*/
      case 0xe://set mouse position in IKBD
        ikbd.abs_mouse_x=MAKEWORD(ikbd.command_param[2],ikbd.command_param[1]);
        ikbd.abs_mouse_y=MAKEWORD(ikbd.command_param[4],ikbd.command_param[3]);
        break;
/*
SET JOYSTICK MONITORING
Code:

    0x17
    rate                ; time between samples in hundreths of a second
    Returns: (in packets of two as long as in mode)
            %000000xy   ; where y is JOYSTICK1 Fire button
                        ; and x is JOYSTICK0 Fire button
            %nnnnmmmm   ; where m is JOYSTICK1 state
                        ; and n is JOYSTICK0 state

Sets the IKBD to do nothing but monitor the serial command lne, maintain 
the time-of-day clock, and monitor the joystick. The rate sets the interval
 between joystick samples. N.B. The user should not set the rate higher than
 the serial communications channel will allow the 2 bytes packets to be 
transmitted.
*/
      case 0x17://joystick duration
        log("IKBD: Joysticks set to duration mode");
        ikbd.joy_mode=IKBD_JOY_MODE_DURATION;
        ikbd.duration=ikbd.command_param[0]*10; //in 1000ths of a second
        ikbd.mouse_mode=IKBD_MOUSE_MODE_OFF;  //disable mouse
        ikbd.port_0_joy=true;
        agenda_delete(ikbd_send_joystick_message); // just in case sending other type of packet
        agenda_delete(ikbd_report_abs_mouse);
        break;
/*
        SET JOYSTICK KEYCODE MODE
Code:

    0x19
    RX                  ; length of time (in tenths of seconds) until
                        ; horizontal velocity breakpoint is reached
    RY                  ; length of time (in tenths of seconds) until
                        ; vertical velocity breakpoint is reached
    TX                  ; length (in tenths of seconds) of joystick closure
                        ; until horizontal cursor key is generated before RX
                        ; has elapsed
    TY                  ; length (in tenths of seconds) of joystick closure
                        ; until vertical cursor key is generated before RY
                        ; has elapsed
    VX                  ; length (in tenths of seconds) of joystick closure
                        ; until horizontal cursor keystokes are generated
                        ; after RX has elapsed
    VY                  ; length (in tenths of seconds) of joystick closure
                        ; until vertical cursor keystokes are generated
                        ; after RY has elapsed
 
In this mode, joystick 0 is scanned in a way that simulates cursor keystrokes.
 On initial closure, a keystroke pair (make/break) is generated. Then up to Rn
 tenths of seconds later, keystroke pairs are generated every Tn tenths of 
 seconds. After the Rn breakpoint is reached, keystroke pairs are generated 
 every Vn tenths of seconds. This provides a velocity (auto-repeat) breakpoint
feature.
Note that by setting RX and/or Ry to zero, the velocity feature can be
 disabled. The values of TX and TY then become meaningless, and the generation
 of cursor 'keystrokes' is set by VX and VY.
*/
      case 0x19://cursor key simulation mode for joystick 0
        ikbd.joy_mode=IKBD_JOY_MODE_CURSOR_KEYS;
        for(int n=0;n<6;n++){
          ikbd.cursor_key_joy_time[n]=ikbd.command_param[n];
        }
        ikbd.cursor_key_joy_ticks[0]=timeGetTime();
        ikbd.cursor_key_joy_ticks[1]=ikbd.cursor_key_joy_ticks[0];
        ikbd.mouse_mode=IKBD_MOUSE_MODE_OFF;  //disable mouse
        ikbd.port_0_joy=true;
        agenda_delete(ikbd_send_joystick_message); // just in case sending other type of packet
        agenda_delete(ikbd_report_abs_mouse);
        break;
/*
TIME-OF-DAY CLOCK SET
Code:

    0x1B
    YY                  ; year (2 least significant digits)
    MM                  ; month
    DD                  ; day
    hh                  ; hour
    mm                  ; minute
    ss                  ; second

All time-of-day data should be sent to the IKBD in packed BCD format. 
Any digit that is not a valid BCD digit should be treated as a 'don't care' 
and not alter that particular field of the date or time. This permits 
setting only some subfields of the time-of-day clock.
*/
      case 0x1b://set clock time
        log("IKBD: Set clock to... ");
        for (int n=0;n<6;n++){
          int newval=ikbd.command_param[n];
          if ((newval & 0xf0)>=0xa0){ // Invalid high nibble
            newval&=0x0f;
            newval|=ikbd.clock[n] & 0xf0;
          }
          if ((newval & 0xf)>=0xa){ // Invalid low nibble
            newval&=0xf0;
            newval|=ikbd.clock[n] & 0x0f;
          }
          int val=(newval >> 4)*10 + (newval & 0xf);
          int max_val=ikbd_clock_max_val[n];
          if (max_val==0){
            int mon=min((ikbd.clock[1] >> 4)*10 + (ikbd.clock[1] & 0xf),12);
            max_val=ikbd_clock_days_in_mon[mon];
          }
          if (val>max_val){
            val=0;if (n==1) val=1;
          }
          ikbd.clock[n]=BYTE((val % 10) | ((val/10) << 4));

          log(HEXSl(ikbd.clock[n],2));
        }
        ikbd.clock_vbl_count=0;
        break;
/*
MEMORY LOAD
Code:

    0x20
    ADRMSB              ; address in controller
    ADRLSB              ; memory to be loaded
    NUM                 ; number of bytes (0-128)
    { data }

This command permits the host to load arbitrary values into the IKBD controller
 memory. The time between data bytes must be less than 20ms.
*/
      case 0x20:  //load memory
        ikbd.command=0x50; // Ant's command about loading memory
        ikbd.load_memory_address=MAKEWORD(ikbd.command_param[1],ikbd.command_param[0]);
        ikbd.command_read_count=ikbd.command_param[2]; //how many bytes to load
        log(Str("IKBD: Loading next ")+ikbd.command_read_count+" bytes into IKBD memory address "+
              HEXSl(ikbd.load_memory_address,4));
        break;
      case 0x50:
        log("IKBD: Finished loading memory");
        break;    //but instead just throw it away!
/*
MEMORY READ
Code:

    0x21
    ADRMSB              ; address in controller
    ADRLSB              ; memory to be read
    Returns:
            0xF6        ; status header
            0x20        ; memory access
            { data }    ; 6 data bytes starting at ADR

This command permits the host to read from the IKBD controller memory.
*/
      case 0x21:  //read memory
      {
        WORD adr=MAKEWORD(ikbd.command_param[1],ikbd.command_param[0]);
        log(Str("IKBD: Reading 6 bytes of IKBD memory, address ")+HEXSl(adr,4));
        keyboard_buffer_write_string(0xf6,0x20,(-1));
        for (int n=0;n<6;n++){
          BYTE b=0;
          if (adr>=0x80 && adr<=0xff) b=ikbd.ram[adr-0x80];
          keyboard_buffer_write(b);
        }
        break;
      }
/*
CONTROLLER EXECUTE
Code:

    0x22
    ADRMSB              ; address of subroutine in
    ADRLSB              ; controller memory to be called

This command allows the host to command the execution of a subroutine in the IKBD
 controller memory.
*/
/* Because RAM is limited to 128 bytes (!) and much is used by the ST, a
   2 steps process involving a loader is used by programs. Those loader are
   loaded using command $20 and executed here.
   They're supposed to take control, stop 6301 interrupts, free memory, etc.
   To emulate that, we will just accept and checksum incoming bytes (see top
   of this function).
*/
      case 0x22:  //execute routine

        log(Str("IKBD: Blimey! Executing IKBD routine at ")+
              HEXSl(MAKEWORD(ikbd.command_param[1],ikbd.command_param[0]),4));
        break;   
/*
RESET

    0x80
    0x01

N.B. The RESET command is the only two byte command understood by the IKBD.
 Any byte following an 0x80 command byte other than 0x01 is ignored 
(and causes the 0x80 to be ignored).
A reset may also be caused by sending a break lasting at least 200mS to the
 IKBD. 
Executing the RESET command returns the keyboard to its default (power-up)
 mode and parameter settings. It does not affect the time-of-day clock. 
The RESET command or function causes the IKBD to perform a simple self-test.
 If the test is successful, the IKBD will send the code of 0xF0 within 300mS
 of receipt of the RESET command (or the end of the break, or power-up).
 The IKBD will then scan the key matrix for any stuck (closed) keys. Any
 keys found closed will cause the break scan code to be generated (the break
 code arriving without being preceded by the make code is a flag for a key
 matrix error).
*/
      case 0x80:  
        if(src==0x01) 
          ikbd_reset(0);
        break;
      }//sw
    }
  }
  else
  { //new command SS: some commands are executed at once, others require parameters
/*
After any joystick command, the ikbd assumes that joysticks are connected to both Joystick0
and Joystick1. Any mouse command (except MOUSE DISABLE) then causes port 0 to again
be scanned as if it were a mouse, and both buttons are logically connected to it. If a mouse
disable command is received while port 0 is presumed to be a mouse, the button is logically
assigned to Joystick1 ( until the mouse is reenabled by another mouse command).
TODO!
*/
    if(ikbd.joy_mode==IKBD_JOY_MODE_FIRE_BUTTON_DURATION) 
    {
      ikbd.joy_mode=IKBD_JOY_MODE_OFF;
    }
    if (ikbd.resetting && src!=0x08 && src!=0x14) ikbd.reset_0814_hack=-1;
    if (ikbd.resetting && src!=0x12 && src!=0x14) ikbd.reset_1214_hack=-1;
    if (ikbd.resetting && src!=0x08 && src!=0x0B && src!=0x14) ikbd.psyg_hack_stage=-1;
    if (ikbd.resetting && src!=0x12 && src!=0x1A) ikbd.reset_121A_hack=-1;
    ikbd.command=(BYTE)src;
    switch (src){  //how many bytes of parameters do we want?
      case 0x7:case 0x17:case 0x80:ikbd.command_read_count=1;break;
/*
SET RELATIVE MOUSE POSITION REPORTING

    0x08

Set relative mouse position reporting. (DEFAULT) 
Mouse position packets are generated asynchronously by the IKBD whenever
 motion exceeds the setable threshold in either axis (see SET MOUSE THRESHOLD).
 Depending upon the mouse key mode, mouse position reports may also be generated
 when either mouse button is pressed or released. Otherwise the mouse buttons 
behave as if they were keyboard keys.
*/
      case 0x8: //return relative mouse position from now on
        ikbd.mouse_mode=IKBD_MOUSE_MODE_RELATIVE;
        ikbd.port_0_joy=false;
        ikbd_inc_hack(ikbd.psyg_hack_stage,0);
        ikbd_inc_hack(ikbd.reset_0814_hack,0);
        agenda_delete(ikbd_report_abs_mouse);
        break;
      case 0x9:ikbd.command_read_count=4;break;
      case 0xa:case 0xb:case 0xc:case 0x21:case 0x22:ikbd.command_read_count=2;break;
/*
INTERROGATE MOUSE POSITION
Code:

	0x0D
    	Returns:
          			  0xF7       	; absolute mouse position header
    	BUTTONS
          			  0000dcba 	  ; where a is right button down since last interrogation
                       			       	; b is right button up since last
                       				; c is left button down since last
                       				; d is left button up since last
          			  XMSB       	; X coordinate
          			  XLSB
          			  YMSB       	; Y coordinate
          			  YLSB

The INTERROGATE MOUSE POSITION command is valid when in the ABSOLUTE MOUSE
POSITIONING mode, regardless of the setting of the MOUSE BUTTON ACTION.
*/
      case 0xd: //read absolute mouse position
        // This should be ignored if you aren't in absolute mode!
        if (ikbd.mouse_mode!=IKBD_MOUSE_MODE_ABSOLUTE) break;
        ikbd.port_0_joy=false; // SS it's a valid mouse command
        // Ignore command if already calcing and sending packet
        if (agenda_get_queue_pos(ikbd_report_abs_mouse)>=0) break;
        agenda_add(ikbd_report_abs_mouse,IKBD_SCANLINES_FROM_ABS_MOUSE_POLL_TO_SEND,-1);
        break;
      case 0xe:ikbd.command_read_count=5;break;
/*
SET Y=0 AT BOTTOM

    0x0F

This command makes the origin of the Y axis to be at the bottom of the
 logical coordinate system internel to the IKBD for all relative or 
absolute mouse motion. This causes mouse motion toward the user to be 
negative in sign and away from the user to be positive.
*/
      case 0xf: //mouse goes upside down
        ikbd.mouse_upside_down=true;
        break;
/*
SET Y=0 AT TOP

    0x10

Makes the origin of the Y axis to be at the top of the logical
 coordinate system within the IKBD for all relative or absolute mouse
 motion. (DEFAULT) This causes mouse motion toward the user to be positive
 in sign and away from the user to be negative.
*/
      case 0x10: //mouse goes right way up
        ikbd.mouse_upside_down=false;
        break;
/*
RESUME

    0x11

Resume sending data to the host. Since any command received by the IKBD
 after its output has been paused also causes an implicit RESUME this 
command can be thought of as a NO OPERATION command. If this command is
 received by the IKBD and it is not PAUSED, it is simply ignored.
*/
      case 0x11: //okay to send!
        log("IKBD turned on");
        ikbd.send_nothing=false;
        break;
/*
DISABLE MOUSE

    0x12

All mouse event reporting is disabled (and scanning may be internally 
disabled). Any valid mouse mode command resumes mouse motion monitoring.
 (The valid mouse mode commands are SET RELATIVE MOUSE POSITION REPORTING,
 SET ABSOLUTE MOUSE POSITIONING, and SET MOUSE KEYCODE MODE. ) 
 N.B. If the mouse buttons have been commanded to act like keyboard keys,
 this command DOES affect their actions.
*/
      case 0x12: //turn mouse off
        log("IKBD: Mouse turned off");
        ikbd.mouse_mode=IKBD_MOUSE_MODE_OFF;
        ikbd.port_0_joy=true;
        ikbd_inc_hack(ikbd.reset_1214_hack,0);
        ikbd_inc_hack(ikbd.reset_121A_hack,0);
        agenda_delete(ikbd_report_abs_mouse);
        break;
/*
PAUSE OUTPUT

    0x13

Stop sending data to the host until another valid command is received.
 Key matrix activity is still monitored and scan codes or ASCII characters
 enqueued (up to the maximum supported by the microcontroller) to be sent
 when the host allows the output to be resumed. If in the JOYSTICK EVENT
 REPORTING mode, joystick events are also queued.
Mouse motion should be accumulated while the output is paused. If the IKBD
 is in RELATIVE MOUSE POSITIONING REPORTING mode, motion is accumulated
 beyond the normal threshold limits to produce the minimum number of packets
 necessary for transmission when output is resumed. Pressing or releasing 
either mouse button causes any accumulated motion to be immediately queued 
as packets, if the mouse is in RELATIVE MOUSE POSITION REPORTING mode.
 Because of the limitations of the microcontroller memory this command 
should be used sparingly, and the output should not be shut of for more
 than <tbd> milliseconds at a time.
The output is stopped only at the end of the current 'even'. 
If the PAUSE OUTPUT command is received in the middle of a multiple byte 
report, the packet will still be transmitted to conclusion and then the 
PAUSE will take effect. When the IKBD is in either the JOYSTICK MONITORING 
mode or the FIRE BUTTON MONITORING mode, the PAUSE OUTPUT command also 
temporarily stops the monitoring process (i.e. the samples are not enqueued
 for transmission).
*/
      case 0x13: //stop data transfer to main processor
        log("IKBD turned off");
        ikbd.send_nothing=true;
        break;
/*
SET JOYSTICK EVENT REPORTING

    0x14

Enter JOYSTICK EVENT REPORTING mode (DEFAULT). 
Each opening or closure of a joystick switch or trigger causes 
a joystick event record to be generated. 
*/
      case 0x14: //return joystick movements
        log("IKBD: Changed joystick mode to change notification");
        ikbd.port_0_joy=true; 
        ikbd.mouse_mode=IKBD_MOUSE_MODE_OFF;  //disable mouse
        agenda_delete(ikbd_report_abs_mouse);
        if (ikbd.joy_mode!=IKBD_JOY_MODE_AUTO_NOTIFY){
          ////agenda_delete(ikbd_send_joystick_message); // just in case sending other type of packet
          ikbd.joy_mode=IKBD_JOY_MODE_AUTO_NOTIFY;
        }
        // In the IKBD this resets old_stick to 0
        for (int j=0;j<2;j++){
          if (stick[j]) ikbd_send_joystick_message(j);
        }
        ikbd_inc_hack(ikbd.psyg_hack_stage,2);
        ikbd_inc_hack(ikbd.reset_0814_hack,1);
        ikbd_inc_hack(ikbd.reset_1214_hack,1);
        break;
/*
SET JOYSTICK INTERROGATION MODE

    0x15

Disables JOYSTICK EVENT REPORTING. Host must send individual JOYSTICK 
INTERROGATE commands to sense joystick state.
*/
      case 0x15: //don't return joystick movements
        log("IKBD: Joysticks set to only report when asked");
        ikbd.port_0_joy=true;
        ikbd.mouse_mode=IKBD_MOUSE_MODE_OFF;  //disable mouse
        agenda_delete(ikbd_report_abs_mouse);
        if (ikbd.joy_mode!=IKBD_JOY_MODE_ASK){
          ikbd.joy_mode=IKBD_JOY_MODE_ASK;
          agenda_delete(ikbd_send_joystick_message); // just in case sending other type of packet
        }
        break;
/*
JOYSTICK INTERROGATE

    0x16

Return a record indicating the current state of the joysticks. 
This command is valid in either the JOYSTICK EVENT REPORTING mode
 or the JOYSTICK INTERROGATION MODE.
*/
      case 0x16: //read joystick
        if (ikbd.joy_mode!=IKBD_JOY_MODE_OFF){
          // Ignore command if already calcing and sending packet
          if (agenda_get_queue_pos(ikbd_send_joystick_message)>=0) break;
          agenda_add(ikbd_send_joystick_message,IKBD_SCANLINES_FROM_JOY_POLL_TO_SEND,-1);
        }
        break;
/*
SET FIRE BUTTON MONITORING
Code:

    0x18
    Returns: (as long as in mode)
            %bbbbbbbb   ; state of the JOYSTICK1 fire button packed
                        ; 8 bits per byte, the first sample if the MSB

Set the IKBD to do nothing but monitor the serial command line, maintain 
the time-of-day clock, and monitor the fire button on Joystick 1. 
The fire button is scanned at a rate that causes 8 samples to be made
 in the time it takes for the previous byte to be sent to the host (i.e.
 scan rate = 8/10 * baud rate).
The sample interval should be as constant as possible.
*/
      case 0x18: //fire button duration, constant high speed joystick button test
        log("IKBD: Joysticks set to fire button duration mode!");
        ikbd.joy_mode=IKBD_JOY_MODE_FIRE_BUTTON_DURATION;
        ikbd.mouse_mode=IKBD_MOUSE_MODE_OFF;  //disable mouse
        agenda_delete(ikbd_report_abs_mouse);
        ikbd.port_0_joy=true;
        agenda_delete(ikbd_send_joystick_message); // just in case sending other type of packet
        break;
/*
DISABLE JOYSTICKS

    0x1A

Disable the generation of any joystick events (and scanning may be internally
disabled). Any valid joystick mode command resumes joystick monitoring. 
(The joystick mode commands are SET JOYSTICK EVENT REPORTING, SET JOYSTICK 
INTERROGATION MODE, SET JOYSTICK MONITORING, SET FIRE BUTTON MONITORING, 
and SET JOYSTICK KEYCODE MODE.)
*/
      case 0x1a: //turn off joysticks
        log("IKBD: Joysticks turned off");
      //  ikbd.mouse_mode=IKBD_MOUSE_MODE_OFF;  //disable mouse // already so in Steem 3.2
        ikbd.port_0_joy=0;
        ikbd.joy_mode=IKBD_JOY_MODE_OFF;
        stick[0]=0;stick[1]=0;
        ikbd_inc_hack(ikbd.reset_121A_hack,1);
        agenda_delete(ikbd_send_joystick_message); // just in case sending other type of packet
        break;
      case 0x1b:case 0x19:ikbd.command_read_count=6;break;
/*
INTERROGATE TIME-OF-DAT CLOCK
Code:

    0x1C
    Returns:
            0xFC        ; time-of-day event header
            YY          ; year (2 least significant digits)
            MM          ; month
            DD          ; day
            hh          ; hour
            mm          ; minute
            ss          ; second

    All time-of-day is sent in packed BCD format.

    This is used by Captain Blood. 
    F29 also reads the clock.
*/
      case 0x1c: //read clock time
        keyboard_buffer_write(0xfc);
        for (int n=0;n<6;n++){
          keyboard_buffer_write(ikbd.clock[n]);
        }
        break;
      case 0x20:ikbd.command_read_count=3;break;
/*
STATUS INQUIRIES
      
Status commands are formed by inclusively ORing 0x80 with the relevant SET command.
Code:

    Example:
    0x88 (or 0x89 or 0x8A)  ; request mouse mode
    Returns:
            0xF6        ; status response header
            mode        ; 0x08 is RELATIVE
                        ; 0x09 is ABSOLUTE
                        ; 0x0A is KEYCODE
            param1      ; 0 is RELATIVE
                        ; XMSB maximum if ABSOLUTE
                        ; DELTA X is KEYCODE
            param2      ; 0 is RELATIVE
                        ; YMSB maximum if ABSOLUTE
                        ; DELTA Y is KEYCODE
            param3      ; 0 if RELATIVE
                        ; or KEYCODE
                        ; YMSB is ABSOLUTE
            param4      ; 0 if RELATIVE
                        ; or KEYCODE
                        ; YLSB is ABSOLUTE
            0           ; pad
            0
 


The STATUS INQUIRY commands request the ikbd to return either the current mode or
 the parameters associated with a given command. All status reports are padded
 to form 8 byte long return packets. The responses to the status requests are
 designed so that the host may store them away (after stripping off the status
 report header byte) and later send them back as commands to IKBD to restore
 its state. The 0 pad bytes will be treated as NOPs by the IKBD.

Valid STATUS INQUIRY commands are:
Code:

            0x87    mouse button action
            0x88    mouse mode
            0x89
            0x8A
            0x8B    mnouse threshold
            0x8C    mouse scale
            0x8F    mouse vertical coordinates
            0x90    ( returns       0x0F Y=0 at bottom
                            0x10 Y=0 at top )
            0x92    mouse enable/disable
                    ( returns       0x00 enabled)
                            0x12 disabled )
            0x94    joystick mode
            0x95
            0x96
            0x9A    joystick enable/disable
                    ( returns       0x00 enabled
                            0x1A disabled )
 
It is the (host) programmer's responsibility to have only one unanswered inquiry
 in process at a time.
STATUS INQUIRY commands are not valid if the IKBD is in JOYSTICK MONITORING mode 
or FIRE BUTTON MONITORING mode.

*/
      case 0x87: //return what happens when mouse buttons are pressed
        keyboard_buffer_write_string(0xf6,0x7,ikbd.mouse_button_press_what_message,0,0,0,0,0,(-1));
        break;
      case 0x88:case 0x89:case 0x8a:
        keyboard_buffer_write(0xf6);
        keyboard_buffer_write(BYTE(ikbd.mouse_mode));
        if (ikbd.mouse_mode==0x9){
          keyboard_buffer_write_string(HIBYTE(ikbd.abs_mouse_max_x),LOBYTE(ikbd.abs_mouse_max_x),
                                        HIBYTE(ikbd.abs_mouse_max_y),LOBYTE(ikbd.abs_mouse_max_y),
                                        0,0,(-1));
        }else if (ikbd.mouse_mode==0xa){
          keyboard_buffer_write_string(ikbd.cursor_key_mouse_pulse_count_x,
                                        ikbd.cursor_key_mouse_pulse_count_y,
                                        0,0,0,0,(-1));
        }else{
          keyboard_buffer_write_string(0,0,0,0,0,0,(-1));
        }
        break;
      case 0x8b: //x, y threshhold for relative mouse movement messages
        keyboard_buffer_write_string(0xf6,0xb,ikbd.relative_mouse_threshold_x,
                                      ikbd.relative_mouse_threshold_y,
                                      0,0,0,0,(-1));
        break;
      case 0x8c: //x,y scaling of mouse for absolute mouse
        keyboard_buffer_write_string(0xf6,0xc,ikbd.abs_mouse_scale_x,
                                      ikbd.abs_mouse_scale_y,
                                      0,0,0,0,(-1));
        break;
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD)
 // we want it to hit default
#else
      case 0x8d: /*DEAD*/ break;
      case 0x8e: /*DEAD*/ break;
#endif
      case 0x8f:case 0x90: //return 0xf if mouse is upside down, 0x10 otherwise
        keyboard_buffer_write(0xf6);
        if (ikbd.mouse_upside_down){
          keyboard_buffer_write(0xf);
        }else{
          keyboard_buffer_write(0x10);
        }
        keyboard_buffer_write_string(0,0,0,0,0,0,(-1));
        break;
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD)
#else
      case 0x91: /*DEAD*/ break;
#endif
      case 0x92:  //is mouse off?
        keyboard_buffer_write(0xf6);
        if (ikbd.mouse_mode==IKBD_MOUSE_MODE_OFF){
          keyboard_buffer_write(0x12);
        }else{
          keyboard_buffer_write(0);
        }
        keyboard_buffer_write_string(0,0,0,0,0,0,(-1));
        break;
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD)
#else
      case 0x93: /*DEAD*/ break;
#endif
      case 0x94:case 0x95:case 0x99:
      {
        keyboard_buffer_write(0xf6);
        // if joysticks are disabled then return previous state. We don't store that.
        BYTE mode=BYTE(ikbd.joy_mode);
        if (mode==IKBD_JOY_MODE_OFF) mode=IKBD_JOY_MODE_AUTO_NOTIFY;
        keyboard_buffer_write(mode);
        if (ikbd.joy_mode==0x19){
          for (int n=0;n<6;n++) keyboard_buffer_write(BYTE(ikbd.cursor_key_joy_time[n]));
        }else{
          keyboard_buffer_write_string(0,0,0,0,0,0,(-1));
        }
        break;
      }
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD)
#else
      case 0x96: /*DEAD*/ break;
      case 0x97: /*DEAD*/ break;
      case 0x98: /*DEAD*/ break;
#endif
      case 0x9a:  //is joystick off?
        keyboard_buffer_write(0xf6);
        if (ikbd.joy_mode==IKBD_JOY_MODE_OFF){
          keyboard_buffer_write(0x1a);
        }else{
          keyboard_buffer_write(0);
        }
        keyboard_buffer_write_string(0,0,0,0,0,0,(-1));
        break;
        // > 0x9a all DEAD (tested up to 0xac)

      default: 
        if(src) TRACE_LOG("Byte ignored",src);  // (TODO)
    }//sw
    ikbd.command_parameter_counter=0;
  }
}
//---------------------------------------------------------------------------
void agenda_keyboard_replace(int) {

#if defined(SS_IKBD_6301)
  if(HD6301EMU_ON) 
  {
    if (keyboard_buffer_length){ //temp still use those var?
      if (!ikbd.send_nothing){
        keyboard_buffer_length--;
        ASSERT( keyboard_buffer_length>=0 );

#if defined(SS_ACIA_DOUBLE_BUFFER_RX)
        // here we should take care of next byte but we have another
        // problem anyway (txinterrupts)
        ASSERT( ACIA_IKBD.LineRxBusy );
        ACIA_IKBD.LineRxBusy=false;
        hd6301_completed_transmission_to_MC6850=true;
#endif

        /*  Check overrun.
        "The Overrun does not occur in the Status Register until the valid character
        prior to Overrun has been read."
        => 1. SR is updated at next read of RDR
        2. The old byte will be read, the new byte is lost
        */
        
        if(ACIA_IKBD.SR&BIT_0) // RDR full
        {
          TRACE_LOG("ACIA RDRS OVR: $%X->$%X\n",keyboard_buffer[keyboard_buffer_length],keyboard_buffer[keyboard_buffer_length+1]);
          log("IKBD: Overrun on keyboard ACIA");
          if(ACIA_IKBD.overrun!=ACIA_OVERRUN_YES) 
          {
            ACIA_IKBD.overrun=ACIA_OVERRUN_COMING; // keep original system
          }
        }
        else // not overrun, OK
        {
          TRACE_LOG("ACIA RDRS->RDR %X\n",ACIA_IKBD.RDRS);
          ACIA_IKBD.RDR=ACIA_IKBD.RDRS; // transfer shifted byte
          ACIA_IKBD.SR|=BIT_0; // set RDR full

#if defined(SS_ACIA_IRQ_DELAY2)
/*  This variable was defined but never used in Steem, we use it now for
    a little hack.
*/
          ACIA_IKBD.last_rx_read_time=ACT;
#endif
        }
        
        // Check if we must activate IRQ (overrun or normal)
        if(ACIA_IKBD.CR&BIT_7)
        {
          log(EasyStr("IKBD: Changing ACIA IRQ bit from ")+ACIA_IKBD.irq+" to 1");
          ACIA_IKBD.SR|=BIT_7;
          TRACE_LOG("ACIA IRQ (RDR)\n");
          ASSERT(  mfp_reg[MFPR_GPIP]&MFP_GPIP_ACIA_BIT ); // just curious
          mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,0);
        }
      }//if (!ikbd.send_nothing){
    }//if (keyboard_buffer_length)

    // More to process?
    if(keyboard_buffer_length) 
    {
      agenda_add(agenda_keyboard_replace,SS_6301_TO_ACIA_IN_HBL,0);
      ACIA_IKBD.RDRS=keyboard_buffer[keyboard_buffer_length-1];
#if defined(SS_ACIA_DOUBLE_BUFFER_RX)
      ACIA_IKBD.LineRxBusy=true;
#endif
    }
    if (macro_start_after_ikbd_read_count) 
      macro_start_after_ikbd_read_count--;
    return;
  }
#endif // Steem 3.2:

  log(EasyStr("IKBD: agenda_keyboard_replace at time=")+hbl_count+" with keyboard_buffer_length="+keyboard_buffer_length);

  if (keyboard_buffer_length){
    if (ikbd.send_nothing==0){
      keyboard_buffer_length--;
      if (ikbd.joy_packet_pos>=keyboard_buffer_length) ikbd.joy_packet_pos=-1;
      if (ikbd.mouse_packet_pos>=keyboard_buffer_length) ikbd.mouse_packet_pos=-1;

      if (ACIA_IKBD.rx_not_read){
        log("IKBD: Overrun on keyboard ACIA");
        // discard data and set overrun
        if (ACIA_IKBD.overrun!=ACIA_OVERRUN_YES) ACIA_IKBD.overrun=ACIA_OVERRUN_COMING;
      }else{
        ACIA_IKBD.data=keyboard_buffer[keyboard_buffer_length]; //---------------------------------------------------------------------------
        ACIA_IKBD.rx_not_read=true;
      }
      if (ACIA_IKBD.rx_irq_enabled){
        log(EasyStr("IKBD: Changing ACIA IRQ bit from ")+ACIA_IKBD.irq+" to 1");
        ACIA_IKBD.irq=true;
      }
      mfp_gpip_set_bit(MFP_GPIP_ACIA_BIT,!(ACIA_IKBD.irq || ACIA_MIDI.irq));
    }
    if (keyboard_buffer_length) agenda_add(agenda_keyboard_replace,ACIAClockToHBLS(ACIA_IKBD.clock_divide),0);
  }
  if (macro_start_after_ikbd_read_count) macro_start_after_ikbd_read_count--;
}


void keyboard_buffer_write_n_record(BYTE src)
{
  keyboard_buffer_write(src);
  if (macro_record) macro_record_key(src);
}



/* 
    Function keyboard_buffer_write() receive bytes from the fake or the true
    6301 emu, or from shortcuts or macros, and sets then in an agenda to take
    care of the 6301->ACIA delay.
*/

void keyboard_buffer_write(BYTE src) {

#if defined(SS_IKBD_6301)
  if(HD6301EMU_ON)
  {

#if defined(SS_ACIA_DOUBLE_BUFFER_RX)
    if(!ACIA_IKBD.LineRxBusy)
    {
#endif
      ACIA_IKBD.RDRS=src; // byte is being shifted
#if defined(SS_ACIA_DOUBLE_BUFFER_RX)
    }
    ACIA_IKBD.LineRxBusy=true;
#endif

/*  Normally the buffer should be 2 bytes, one being shifted, one in the 
    register. In fake mode, it's much more.
    If this is first byte, we prepare an agenda to emulate the ACIA transmission
    delay.
    If there's already info in the buffer, an agenda was already prepared,
    a new agenda will be prepared when this one is executed. We only need to
    shift the buffer and insert the new value.
    For the moment we keep this system of keyboard buffer even for 6301 true
    emu because it is flexible.
*/

    if(keyboard_buffer_length<MAX_KEYBOARD_BUFFER_SIZE)
    {
      if(keyboard_buffer_length)
        memmove(keyboard_buffer+1,keyboard_buffer,keyboard_buffer_length); // shift
      else
        agenda_add(agenda_keyboard_replace,SS_6301_TO_ACIA_IN_HBL,0);
      keyboard_buffer_length++;
      keyboard_buffer[0]=src;
      TRACE_LOG("IKBD +$%X(%d)\n",src,keyboard_buffer_length);
    }
    else
      TRACE_LOG("IKBD: Keyboard buffer overflow\n");
    return;
  }
#endif // Steem 3.2:

  if (keyboard_buffer_length<MAX_KEYBOARD_BUFFER_SIZE){
    if (keyboard_buffer_length){
      memmove(keyboard_buffer+1,keyboard_buffer,keyboard_buffer_length);
    }else{
      // new chars in keyboard so time them out, +1 for middle of scanline
      agenda_add(agenda_keyboard_replace,ACIAClockToHBLS(ACIA_IKBD.clock_divide)+1,0);
    }
    keyboard_buffer_length++;
    keyboard_buffer[0]=src;
    log(EasyStr("IKBD: Wrote $")+HEXSl(src,2)+" keyboard buffer length="+keyboard_buffer_length);
    if (ikbd.joy_packet_pos>=0) ikbd.joy_packet_pos++;
    if (ikbd.mouse_packet_pos>=0) ikbd.mouse_packet_pos++;
  }else{
    log("IKBD: Keyboard buffer overflow");
  }
}


void keyboard_buffer_write_string(int s1,...)
{
  int *ptr;
  for (ptr=&s1;*ptr!=-1;ptr++){
    keyboard_buffer_write(LOBYTE(*ptr));
  }
}


void ikbd_mouse_move(int x,int y,int mousek,int max_mouse_move)
{
  log(EasyStr("Mouse moves ")+x+","+y);
  
#if defined(STEVEN_SEAGAL) &&defined(SS_IKBD_6301)
  if(HD6301EMU_ON)
  {
#if defined(SS_IKBD_6301_MOUSE_ADJUST_SPEED)
    //TODO still our attempts to get a smoother mouse
    const int max_step=20+20*screen_res; //18
    const int multiplier=1; //2
    const int divisor=1; //3
    if(x>1||x<-1)
      x*=multiplier,x/=divisor;
    if(x>max_step)
      x=max_step;
    if(x<-max_step)
      x=-max_step;

    if(y>1||y<-1)
      y*=multiplier,y/=divisor;
    if(y>max_step)
      y=max_step;
    if(y<-max_step)
      y=-max_step;
#endif

    HD6301.MouseVblDeltaX=x,HD6301.MouseVblDeltaY=y;
    return;
  }
#endif

  if (ikbd.joy_mode<100 || ikbd.port_0_joy==0) {  //not in duration mode or joystick mode
    if (ikbd.mouse_mode==IKBD_MOUSE_MODE_ABSOLUTE){
      ikbd.abs_mouse_x+=x;
      if(ikbd.abs_mouse_x<0)ikbd.abs_mouse_x=0;
      else if(ikbd.abs_mouse_x>ikbd.abs_mouse_max_x)ikbd.abs_mouse_x=ikbd.abs_mouse_max_x;
      ikbd.abs_mouse_y+=y;
      if(ikbd.abs_mouse_y<0)ikbd.abs_mouse_y=0;
      else if(ikbd.abs_mouse_y>ikbd.abs_mouse_max_y)ikbd.abs_mouse_y=ikbd.abs_mouse_max_y;
    }else if (ikbd.mouse_mode==IKBD_MOUSE_MODE_RELATIVE){
      int x1=0;int y1=0;
      while (abs(x-x1)>max_mouse_move || abs(y-y1)>max_mouse_move){
        int x2=min(max_mouse_move,max(-max_mouse_move,x-x1));
        int y2=min(max_mouse_move,max(-max_mouse_move,y-y1));
/*
 0xF8-0xFB |relative mouse position records(lsbs determind by mouse button states)
The relative mouse position record is a three byte record of the form 
(regardless of keyboard mode):

do_relocation    %111110xy             ; mouse position record flag
                                   ; where y is the right button state
                                   ; and x is the left button state
    X                        ; delta x as twos complement integer
    Y                        ; delta y as twos complement integer [/code]

Note that the value of the button state bits should be valid even if the MOUSE
 BUTTON ACTION has set the buttons to act like part of the keyboard. If the
accumulated motion before the report packet is generated exceeds the +127...-128
 range, the motion is broken into multiple packets. Note that the sign of the 
delta y reported is a function of the Y origin selected.
*/
        keyboard_buffer_write(BYTE(0xf8+(mousek & 3)));
        keyboard_buffer_write(LOBYTE(x2));
        keyboard_buffer_write(LOBYTE(y2));
        x1+=x2;
        y1+=y2;
      }
      keyboard_buffer_write(BYTE(0xf8+(mousek & 3)));
      keyboard_buffer_write(LOBYTE(x-x1));
      keyboard_buffer_write(LOBYTE(y-y1));
    }else if (ikbd.mouse_mode==IKBD_MOUSE_MODE_CURSOR_KEYS){
/*
Mouse Cursor Key Mode

The IKBD can translate mouse motion into the equivalent cursor keystrokes.
 The number of mouse clicks per keystroke is independently programmable
 in each axis. The IKBD internally maintains mouse motion information to 
the highest resolution available, and merely generates a pair of cursor
 key events for each multiple of the scale factor. Mouse motion produces
 the cursor key make code immediately followed by the break code for the
 appropriate cursor key. The mouse buttons produce scan codes above those
 normally assigned for the largest envisioned keyboard 
(i.e. LEFT=0x74 & RIGHT=0x75).
*/
      while(abs(x)>ikbd.cursor_key_mouse_pulse_count_x || abs(y)>ikbd.cursor_key_mouse_pulse_count_y){
        if(x>ikbd.cursor_key_mouse_pulse_count_x){
          keyboard_buffer_write(0x4d);
          keyboard_buffer_write(0x4d|MSB_B);
          x-=ikbd.cursor_key_mouse_pulse_count_x;
        }else if(x<-ikbd.cursor_key_mouse_pulse_count_x){
          keyboard_buffer_write(0x4b);
          keyboard_buffer_write(0x4b|MSB_B);
          x+=ikbd.cursor_key_mouse_pulse_count_x;
        }
        if(y>ikbd.cursor_key_mouse_pulse_count_y){
          keyboard_buffer_write(0x50);
          keyboard_buffer_write(0x50|MSB_B);
          y-=ikbd.cursor_key_mouse_pulse_count_y;
        }else if(y<-ikbd.cursor_key_mouse_pulse_count_y){
          keyboard_buffer_write(0x48);
          keyboard_buffer_write(0x48|MSB_B);
          y+=ikbd.cursor_key_mouse_pulse_count_y;
        }
      }
      if(mousek&2)keyboard_buffer_write(0x74);else keyboard_buffer_write(0x74|MSB_B);
      if(mousek&1)keyboard_buffer_write(0x75);else keyboard_buffer_write(0x75|MSB_B);
    }
  }
}
//---------------------------------------------------------------------------
void ikbd_set_clock_to_correct_time()
{
  time_t timer=time(NULL);
  struct tm *lpTime=localtime(&timer);
  ikbd.clock[5]=BYTE((lpTime->tm_sec % 10) | ((lpTime->tm_sec/10) << 4));
  ikbd.clock[4]=BYTE((lpTime->tm_min % 10) | ((lpTime->tm_min/10) << 4));
  ikbd.clock[3]=BYTE((lpTime->tm_hour % 10) | ((lpTime->tm_hour/10) << 4));
  ikbd.clock[2]=BYTE((lpTime->tm_mday % 10) | ((lpTime->tm_mday/10) << 4));
  int m=    lpTime->tm_mon +1; //month is 0-based in C RTL
  ikbd.clock[1]=BYTE((m % 10) | ((m/10) << 4));
  int y= (lpTime->tm_year);
  y %= 100;
//  lpTime->tm_year %= 100;
//  ikbd.clock[0]=BYTE((lpTime->tm_year % 10) | ((lpTime->tm_year/10) << 4));
  ikbd.clock[0]=BYTE((y % 10) | ((y/10) << 4));
  ikbd.clock_vbl_count=0;
}

void ikbd_reset(bool Cold)
{
  ASSERT( Cold || !HD6301EMU_ON );// !!! always //check this
  agenda_delete(agenda_keyboard_reset);

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
  if(HD6301EMU_ON)
  {
    if(HD6301_OK) 
    {
      TRACE_LOG("6301 reset ikbd.cpp part\n");
      HD6301.Crashed=0;
      ikbd.mouse_upside_down=false;
      return;
    }
    else
      HD6301EMU_ON=0; // and no return
  }
#endif

  if (Cold){
    ikbd_set_clock_to_correct_time();
    ikbd.command_read_count=0;
    agenda_delete(agenda_keyboard_replace);
    keyboard_buffer_length=0;
    keyboard_buffer[0]=0;
    ikbd.joy_packet_pos=-1;
    ikbd.mouse_packet_pos=-1;
    agenda_keyboard_reset(0);
    ZeroMemory(ST_Key_Down,sizeof(ST_Key_Down));
  }else{ // SS we've received a 0x80 0x01 command (Cold=misnomer)
    agenda_keyboard_reset(0);
    ikbd.resetting=true;
    agenda_add(agenda_keyboard_reset,MILLISECONDS_TO_HBLS(50),true);
  }
}
//---------------------------------------------------------------------------
void agenda_keyboard_reset(int SendF0) // SS scheduled by ikbd_reset()
{ // 'F0' because that was in the doc for the first chip
  TRACE_LOG("IKBD: TM %d Execute reset\n",ABSOLUTE_CPU_TIME);

  if (SendF0==0){
    ikbd.mouse_button_press_what_message=0;
    ikbd.mouse_mode=IKBD_MOUSE_MODE_RELATIVE;
    ikbd.joy_mode=IKBD_JOY_MODE_AUTO_NOTIFY;
    ikbd.cursor_key_mouse_pulse_count_x=3;
    ikbd.cursor_key_mouse_pulse_count_y=3;
    ikbd.relative_mouse_threshold_x=1;
    ikbd.relative_mouse_threshold_y=1;
    ikbd.abs_mouse_scale_x=1;
    ikbd.abs_mouse_scale_y=1;
    ikbd.abs_mouse_x=shifter_x/2;
    ikbd.abs_mouse_y=shifter_y/2;
    ikbd.abs_mouse_max_x=shifter_x;
    ikbd.abs_mouse_max_y=shifter_y;
    ikbd.mouse_upside_down=false;
    ikbd.send_nothing=false;
    ikbd.port_0_joy=false;
    ikbd.abs_mousek_flags=0;

    ikbd.psyg_hack_stage=0;
    ikbd.reset_0814_hack=0;
    ikbd.reset_1214_hack=0;
    ikbd.reset_121A_hack=0;

    ZeroMemory(ikbd.ram,sizeof(ikbd.ram));

    agenda_delete(ikbd_send_joystick_message); // just in case sending other type of packet
    agenda_delete(ikbd_report_abs_mouse); // just in case sending other type of packet

    stick[0]=0;
    stick[1]=0;
  }else{
    log(EasyStr("IKBD: Finished reset at ")+hbl_count);

#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
    if(HD6301EMU_ON) 
      HD6301.ResetProgram();
    else
#endif
      keyboard_buffer_write(IKBD_RESET_MESSAGE); // 0xF1

    if (ikbd.psyg_hack_stage==3 || ikbd.reset_0814_hack==2 || ikbd.reset_1214_hack==2){
      log("IKBD: HACK ACTIVATED - turning mouse on.");
      TRACE_LOG("IKBD: HACK ACTIVATED - turning mouse on\n"); // Barbarian
      ikbd.mouse_mode=IKBD_MOUSE_MODE_RELATIVE;
      ikbd.port_0_joy=false;
    }
    if (ikbd.reset_121A_hack==2){ // Turned both mouse and joystick off, but they should be on.
      log("IKBD: HACK ACTIVATED - turning mouse and joystick on.");
      TRACE_LOG("IKBD: HACK ACTIVATED - turning mouse and joystick on.\n");
      ikbd.mouse_mode=IKBD_MOUSE_MODE_RELATIVE;
      ikbd.joy_mode=IKBD_JOY_MODE_AUTO_NOTIFY;
      ikbd.port_0_joy=false;
    }
    ikbd.mouse_button_press_what_message=0; // Hack to fix No Second Prize
    ikbd.send_nothing=0; // Fix Just Bugging (probably correct though)
#if defined(STEVEN_SEAGAL) && defined(SS_IKBD_6301)
    if(!HD6301EMU_ON) 
#endif
    for (int n=1;n<118;n++){
      // Send break codes for "stuck" keys
      // The break code for each key is obtained by ORing 0x80 with the make code.
      if (ST_Key_Down[n]) keyboard_buffer_write(BYTE(0x80 | n));
    }
  }
  ikbd.resetting=0;

#if defined(STEVEN_SEAGAL) && defined(SS_ACIA_IRQ_DELAY)
  ikbd.timer_when_keyboard_info=0;
#endif

}
//---------------------------------------------------------------------------
void ikbd_report_abs_mouse(int abs_mousek_flags)
{
  bool use_current_mousek=(abs_mousek_flags==-1);
  if (use_current_mousek) abs_mousek_flags=ikbd.abs_mousek_flags;

  if (ikbd.mouse_packet_pos>=0){
    keyboard_buffer[ikbd.mouse_packet_pos-1]|=LOBYTE(abs_mousek_flags); // Must |= this or could lose button presses
    keyboard_buffer[ikbd.mouse_packet_pos-2]=HIBYTE(ikbd.abs_mouse_x);
    keyboard_buffer[ikbd.mouse_packet_pos-3]=LOBYTE(ikbd.abs_mouse_x);
    keyboard_buffer[ikbd.mouse_packet_pos-4]=HIBYTE(ikbd.abs_mouse_y);
    keyboard_buffer[ikbd.mouse_packet_pos-5]=LOBYTE(ikbd.abs_mouse_y);
  }else{
// 0xF7 | absolute mouse position record
    keyboard_buffer_write_string(0xf7,LOBYTE(abs_mousek_flags),
                    HIBYTE(ikbd.abs_mouse_x),LOBYTE(ikbd.abs_mouse_x),
                    HIBYTE(ikbd.abs_mouse_y),LOBYTE(ikbd.abs_mouse_y),-1);
    ikbd.mouse_packet_pos=5;
  }
  if (use_current_mousek) ikbd.abs_mousek_flags=0;
}
//---------------------------------------------------------------------------
void ikbd_send_joystick_message(int jn)
{
  BYTE s[2]={stick[0],stick[1]};
  // If mouse active then joystick never sends button down
  if (ikbd.port_0_joy==0) s[0]&=0x0f, s[1]&=0x0f;
  if (jn==-1){ // requested packet
    if (ikbd.joy_packet_pos>=0){
      keyboard_buffer[ikbd.joy_packet_pos-1]=s[0];
      keyboard_buffer[ikbd.joy_packet_pos-2]=s[1];
    }else{
/*
Joystick Interrogation

The current state of the joystick ports may be interrogated at any time in this 
mode by sending an 'Interrogate Joystick' command to the IKBD.

The IKBD response to joystick interrogation is a three byte report of the form
Code:

    0xFD                	; joystick report header
    %x000yyyy           	; Joystick 0
    %x000yyyy           	; Joystick 1
                      		  ; where x is the trigger
                      		  ; and yyy is the stick position
*/
      keyboard_buffer_write_string(0xfd,s[0],s[1],-1);
      ikbd.joy_packet_pos=2; //0=stick 1, 1=stick 0, 2=header
    }
  }else{
/*

Joystick Event Reporting

In this mode, the IKBD generates a record whenever the joystick position is changed
 (i.e. for each opening or closing of a joystick switch or trigger).

The joystick event record is two bytes of the form:
Code:

    %1111111x           	; Joystick event marker
                      		  ; where x is Joystick 0 or 1
0xFE | joystick 0 event
0xFF | joystick 1 event
    %x000yyyy           	; where yyyy is the stick position
                      		  ; and x is the trigger
*/
    keyboard_buffer_write_string((BYTE)(0xfe + jn),s[jn],-1);
    log(EasyStr("IKBD: Notified joystick movement, stick[")+jn+"]="+s[jn]);
  }
}




//---------------------------------------------------------------------------
#undef LOGSECTION
