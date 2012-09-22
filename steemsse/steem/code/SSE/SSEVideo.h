// Steem Steven Seagal Edition
// SSEVideo.h

/*  The ST was a barebone machine made up of cheap components hastily patched
    together, including the video shifter.
    A shifter was needed to translate the video RAM into the picture.
    Such a system was chosen because RAM space was precious. Chunk modes are
    only possible with byte or more sizes.
    All the strange things you can do with the shifter were found by hackers,
    they weren't devised by Atari. 
    This is both difficult and very interesting to emulate.
    I inserted some good documentation on all those tricks.
    Parts marked Flix are extracts of a text written by this demo maker
    Parts marked ST-CNX come from Alien of ST-Connexion,
    another demo maker, as translated by frost of Sector One
    (more to add)
*/

#pragma once
#ifndef SSEVIDEO_H
#define SSEVIDEO_H

#if defined(SS_VIDEO)
/*  The shifter trick mask is an imitation of Hatari's BorderMask.
    Each trick has a dedicated bit, to set it we '|' it, to check it
    we '&' it. Each value is the previous one x2.
    Note max 32 bit = $80 000 000
*/
#define TRICK_LINE_PLUS_26 0x001
#define TRICK_LINE_PLUS_2 0x02
#define TRICK_LINE_MINUS_106 0x04
#define TRICK_LINE_MINUS_2 0x08
#define TRICK_LINE_PLUS_44 0x10
#define TRICK_4BIT_SCROLL 0x20
#define TRICK_OVERSCAN_MED_RES 0x40 //?
#define TRICK_BLACK_LINE 0x80	
#define TRICK_VERTICAL_OVERSCAN 0x100 // for both top & bottom
#define TRICK_LINE_PLUS_20 0x200	
#define TRICK_0BYTE_LINE 0x400	
#define TRICK_STABILISER 0x800
#define TRICK_WRITE_SDP 0x1000
#define TRICK_WRITE_SDP_POST_DE 0x2000
#define TRICK_CONFUSED_SHIFTER 0x4000//tmp hack

/*  There's something wrong with the cycles when the line is 508 cycles,
    but fixing it will take some care. See the Omega hack in SSEVideo.cpp.
*/
#define LINECYCLE0 cpu_timer_at_start_of_hbl
#define LINECYCLES (ABSOLUTE_CPU_TIME-LINECYCLE0) 
#define FRAMECYCLES (ABSOLUTE_CPU_TIME-cpu_time_of_last_vbl)
#if defined(SS_DEBUG) || defined(SS_INT_VBI_START) // TODO check
#define FRAME (Shifter.nVbl) 
#elif defined(SS_VID_SHIFTER_EVENTS)
#define FRAME (VideoEvents.nVbl)
#else 
#define FRAME (-1)
#endif

// register names in Atari doc / Steem variable
#define HSCROLL shifter_hscroll
#define LINEWID shifter_fetch_extra_words

// ID which part of emulation required video rendering
enum {DISPATCHER_NONE, DISPATCHER_CPU, DISPATCHER_LINEWIDTH,
  DISPATCHER_WRITE_SDP, DISPATCHER_SET_SHIFT_MODE, DISPATCHER_SET_SYNC,
  DISPATCHER_SET_PAL, DISPATCHER_DSTE};

enum {BORDERS_NONE, BORDERS_ON, BORDERS_AUTO_OFF, BORDERS_AUTO_ON};

/*  The shifter latency could be the time needed by the shifter to treat
    data it's fed. 
    It is observed that palette changes occuring at some cycle apply at once,
    but on pixels that would be behind those that are currently being fetched
    (video counter).
    If we don't do that, emulation of Spectrum 512 pictures is not correct.
    There is a hack in Steem, delaying rendering by 28 cycles (16 fetching
    +12 treatment),and also a "magic value" (hack) of 7 (x4) used in Hatari 
    to shift lines with the same result.
    We try to rationalise this.
*/
#define SHIFTER_PREFETCH_LATENCY 12 
#define SHIFTER_RASTER_PREFETCH_TIMING (shifter_freq==72 ? 4 : 16) 
#define SHIFTER_RASTER (shifter_freq==72? 2 : (screen_res ? 4 : 8)) 


/* 
  Atari:

       Video display memory is configured as n logical  planes
  interwoven  by  16  bit words into contiguous memory to form
  one 32 Kbyte (actually 0x7d00) physical  plane  starting  at
  any 256 byte half page boundary (in RAM only).  The starting
  address of display  memory  is  placed  in  the  Video  Base
  Address  Register  (read/write,  reset:  all zeros) which is
  then loaded into the Video Address  Counter  Register  (read
  only, reset: all zeros) and incremented.  The following is a
  diagram of possible physical configurations of video display
  memory:


                 --------
  16 bit word   |        |
                 -------- -------- -------- -------- --------
  4 plane       |plane 0 |plane 1 |plane 2 |plane 3 |plane 0 |
                 -------- -------- -------- -------- --------
  2 plane       |plane 0 |plane 1 |plane 0 |plane 1 |plane 0 |
                 -------- -------- -------- -------- --------
  1 plane       |plane 0 |plane 0 |plane 0 |plane 0 |plane 0 |
                 -------- -------- -------- -------- --------


       The general flow of the video controller is as follows:
  BitMap  planes are taken a word at a time from video display
  memory and placed in the video shift register where one  bit
  from  each  plane is shifted out and collectively used as an
  index (plane 0 is the least  significant  bit)  to  a  color
  lookup  palette  entry which is supplied to 3 bit digital to
  analog converters to produce RGB output.  The following is a
  block diagram of the video controller:

  ----- Video Controller Block Diagram ---------------

         --------          --------        --------   lsb  --------
        | 3      |---- -->|0 1 2 3 |----->|      0 |----->| 16 x 9 |-
       --------  |    |   |        |      |      1 |----->| Lookup |-|->R
      | 2      |------    |        |      |      2 |----->|        |-
     --------  | |    |   |        |      |      3 |----->|        |-
    | 1      |--------    |        |      |        |--    |        |-|->G
   --------  | |-     |    --------        --------   |   |        |-
  | 0      |----------                                |   |        |-
  |        | |-                                       |   |        |-|->B
  |        | |                                        |   |        |-
  |        |-                                         |    --------
  |        |                                          |    --------
   --------                                            -->|Inverter|->MONO
                                                           --------

  Logical BitMap          Video Display   Video Shift     Color Palette
  Planes                  Memory          Register        and 3 Bit DACs


    ST-CNX:

Picture Display Format 

A picture is displayed 50 times a second in 50Hz mode (or 60 times in 60Hz). The
ST generates 313 lines per picture at  50Hz (263 at 60Hz). In theory, it  should
generate 312.5 lines, (obtained by  successively displaying 312 and 313  lines).
It therefore assumes  the monitor can  tolerate displaying a  picture at 49,92Hz
instead of 50Hz. This also explains why the ST cannot have a real interlace mode
such as that of  the Amiga. Each line  takes a constant time  to be transmitted:
otherwise the  picture would  be distorted.  This time  is calculated  using the
following formula:

               1
-------------------------------------------------------------
(Number of pictures per second * number of lines per picture)

For "Number of pictures per second"=50 and "Number of lines per  picture"=312.5,
we get 64 microseconds. But some will wonder how come there are 313 lines, if  I
only see 200  on the screen?  The other lines  are used by  the upper and  lower
borders, and  by the  vertical synchronization  signal which  tells the  monitor
where the top of  the screen starts. Overscan  is the process of  converting the
lines used to make  up the borders into  lines able to display  a picture loaded
from memory. I will call "useable  screen" that part of the screen  not occupied
by borders.

A monochrome picture  is displayed in  the same way,  where the refresh  rate is
approximately 70Hz and each line takes 28 microseconds to be transmitted. It  is
composed of approximately 500 lines, of which 400 form the useable screen.



Video Components 

Before getting into the subject, let us recall the role of each component of the
ST responsible of the display.

- The MMU: This component reads memory and sends it to the SHIFTER every 500 ns.

  Registers:
  * Address of the picture to display on next VBL:
$FFFF8201: upper byte
$FFFF8203: middle byte
$FFFF820D: lower byte (STE only)

  * Video counter, indicates the address being decoded at this instant:
$FFFF8205: upper byte
$FFFF8207: middle byte
$FFFF8209: lower byte
    Please note that you can only read from these on a plain ST, the STE however
    also allows to write to them,  which will immediately  change the  displayed
    screenarea.

- The GLUE: Produces the vertical and  horizontal synchronization signals.  It
   also produces signals telling the SHIFTER  when it should display the useable
   screen,  and  when it should show the background  colour 0  (corresponding to
   the border). It also tells the MMU when to send data to the SHIFTER.

  Registers:
  * $FFFF820A: Synchronization mode
    Bit 1 corresponds to the monitor refresh rate (0 for 60Hz, 1 for 50 Hz)

  * Resolution:
$FFFF8260  (Both the GLUE and the SHIFTER have a copy of this register)
    This byte can have  one of 3 values:  0 for low-,  1 for middle- and  2 for
    high-resolution.  The GLUE needs this register,  because in high-resolution
    the picture is displayed at ~70 Hz which requires different synchronization
    signals (see second part)

- The SHIFTER:  Decodes the data  sent by the MMU into  colour or  monochrome
   signals, corresponding to the picture in memory.

  Registers:
  * Resolution:
$FFFF8260  (Both the GLUE and the SHIFTER have a copy of this register)
    The SHIFTER needs this register because  it needs to know  how to decode the
    picture data (as 4, 2 or 1 bitplanes) and whether to send the picture out to
    the RGB pins or the monochrome pin. Indeed, if one of these pins are active,
    the other is low.  The "hardware"  proof is  that the SHIFTER  receives  the
    address bus signals A1 to A5 (no need for A0 because every address is even).
    However only A1 to A4 would be sufficient to access the palette.



Video Signals 

Now we'll discuss which signals the display components use to work together.
Before diving in, a word of warning: everything said here was deduced logically
but never verified with an oscilloscope. Thus, while I'm sure of my conclusions,
I don't take any responsibility if some hardware hacker damages his ST by
trusting this article.

The MMU reads picture data from memory  and passes it on the SHIFTER, using  the
Load/Dcyc signal to  tell the SHIFTER  the data on  its data bus  comes from the
screen buffer. When this happens, the  68000 can't access the data bus.  Because
the  data  bus  is also  used  to  program the  SHIFTER  registers,  you may  be
wondering,  how  can the  68000  change resolution  or  the palette?  After  all
programs such as Spectrum 512 change the palette while the picture is displayed!
Isn't there a conflict between the 68000 and the MMU?  To help us to understand,
we notice that the 68000 has access to the address and data buses every 4 cycles
(which explains why  all instruction cycle  times are multiples  of four: a  NOP
takes 4 cycles as specified in the 68000 manual, but EXG takes 8 cycles  instead
of the 6 specified in the manual). Thus, the MMU steals the other cycles to read
the video memory and to  transfer it to the SHIFTER,  and also to deal with  DMA
Sound on the STE. On the other  hand, the MMU interrupts the 68000 for  disk-DMA
(floppy and HD)  and Blitter transfers.  Thus, if these  devices are not  turned
off, every programs  that relies on  constant execution time  (such as Overscan)
will not work correctly.

 
The GLUE generates the DE, Vsync, Hsync and Blank signals:

- The DE (Display Enable) signal  tells the  SHIFTER when to display the  border
  and when it has to display the  usable screen.  It also tells the MMU  when to
  send data to the SHIFTER:  It is unnecessary  to send  data and  increment the
  video counter  while the  border is  displayed.  If you cut the track  of this
  signal you will obtain a  "hardware" overscan (thanks to Aragorn who confirmed
  this by doing it). However, due to stability issues, this is  not recommended.
  The Autoswitch Overscan board (see ST Magazine 50) works in a very similar way
  by controlling this signal.

- The Blank signal forces the  RGB colour outputs to zero  just before,  during,
  and just after the Hsync and Vsync synchronization signals.  Indeed,  monitors
  require the video signal  to be 0 volts  (black colour) before  every synchro-
  nization pulse (implemented as a negative voltage): Even the border colour has
  to be eliminated to prevent overlaying the border colour over the screen while
  the electron beam returns to the beginning of the next line (from the screen's
  right to its left)  and to the next picture  (from the screen's bottom  to its
  top).  Note that the Blank signal isn't used if the screen is displaying high-
  resolution.

The Vsync and Hsync sync signals are added to the video signal:

- Vsync is the  vertical synchronization signal which specifies where the top of
  the picture is located  in the video stream.  The VBL interrupt corresponds to
  an edge of this signal.

- Hsync is the horizontal synchronization signal. It generates the HBL interrupt
  (vector $68).  The HBL interrupt  should not  to be confused  with the Timer B
  interrupt (vector $120)  which is generated  when the DE signal goes low on an
  input of the MFP:  The Timer B interrupt is triggered  only,  when the useable
  screen is displayed, because DE is inactive during upper and lower border.

The MMU uses the  Vsync signal to  reset the video counter  with the contents of
$FFFF8201 and $FFFF8203 (you may also add $FFFF820D on STE).

Knowing that  the GLUE  handles this  signals,  it  seems logical  that it would
control the process of displaying the  screen. To do this, it would  require two
counters:

- a vertical counter:   the number of currenly displayed line

- a horizontal counter: the position within the line currently being output

        [Top and Bottom Overscan: see CheckVerticalOverscan()]

        Left and Right overscan

        The horizontal counter is incremented every 500ns (aka every
        0.5us). It is reset every HBL. Thus the MMU sends a word to the
        SHIFTER every 0.5us . Notice that at 50Hz a line will last 64us
        while at 60Hz it will last 63.5us. Also note that the SECAM
        specification requires that the horizontal synchronisation pulse
        should last 0.47us. However because the GLUE only has a
        resolution of 0.5us, the ST relies on the tolerance of the
        monitor and emits an Hsync signal of 0.5us.

        The GLUE goes through a similar process to display each line as
        it does to display the whole picture: first a border. This is
        achieved using a signal H (as in Horizontal) which is active
        during the portion of the line to be displayed. H and V are
        combined using an AND gate to give the signal DE (Display
        Enable). Thus only when H and V are active, is a useable screen
        displayed.

        H is activated once the horizontal counter reaches a first value
        at the end of the border. H is deactivated once the horizontal
        counter reaches a second value at the end of the useable screen,
        40us later. Finally once the horizontal counter reaches a third
        value, the Hsync signal is activated for 500ns and a new line is
        started. Notice that while these three values depend on the
        display frequency, the length of a standard useable line is
        always 40us (that is to say 160 bytes output every 0.5us as
        words).

 
        The following pseudo-code describes the GLUE's work. Note that I
        did not determine the exact moment the HBL interrupt takes
        place, and therefore a constant should be added to the following
        numbers. Similarly, there is little value in determining Y.

Position=0
REPEAT
 IF Position == 13  AND display_freq=60  THEN Activate    signal H
 IF Position == 14  AND display_freq=50  THEN Activate    signal H
 IF Position == 93  AND display_freq=60  THEN Disactivate signal H
 IF Position == 94  AND display_freq=50  THEN Disactivate signal H
 IF Position == Y   AND display_freq=60  THEN Activate    signal Hsync
 IF Position == Y+1 AND display_freq=60  THEN Disactivate signal Hsync
                                              and start a new line
 IF Position == Y+1 AND display_freq=50  THEN Activate    signal Hsync
 IF Position == Y+2 AND display_freq=50  THEN Disactivate signal Hsync
                                              and start a new line
 Position = Position+1
END_REPEAT

  [for specific border tricks, see SSEVideo.cpp]

        The internal structure of the SHIFTER:

        As the GLUE controls the DE signal, one would expect the
        beginning of the useable screen to always occur at the same
        location on the screen. However, if one removes the right
        border, one will see that the useable screen starts slightly
        further to the left than it does in the normal non-overscan
        mode. The SHIFTER is responsable for this effect. Unlike the
        GLUE, it is a complex circuit. Providing a full explanation of
        its function is beyond the scope of this article. Instead I
        present here a simplified explanation, which may describe
        certain aspects poorly, but suffices for this study.

 
        The SHIFTER contains 4 16-bit registers RR1 to RR4, where RR
        denotes Rotating Register. Each of these registers undergoes a
        continuous rotation, shifting out their most significant bit on
        each rotation. The most significant bits thus output are used to
        address colours in the palette which is directly linked to the
        RGB pins. For instance in low resolution, the four RR registers
        are shifted so 4 bits are output at a time, corresponding to the
        4 bitplanes. Thus, after 16 rotations, RR1 to RR4 are "empty".

        The four RR registers are updated simultaneously and at the same
        time, whatever the resolution. Consider medium resolution. It
        only uses 2 bitplanes, so only two registers RR1 and RR2 are
        being shifted. It doubles the horizontal resolution (640
        pixels), so both registers are rotating twice as fast as in low
        resolution. Each time the RR registers are shifted, their least
        significant bit is cleared. Once RR1 and RR2 have been
        "emptied", they are updated with the contents of RR3 and RR4,
        which are themselves emptied.

        High resolution is processed slightly differently: only RR1
        rotates, but at four times the speed it does in low resolution.
        The exiting bit addresses bit 0 of colour 0 in the palette to
        determine whether the picture should be shown in inversed video.
        It is also reinserted as the entering bit of RR1, so RR1 is
        rotated rather than shifted. When RR1 has been emptied, it is
        updated with the value of RR2; RR2 is updated with the value of
        RR3 and RR4 with that of RR3. RR4 is then empty, and is filled
        with $0000 or $FFFF depending on the value of bit 0 of colour 0.

        The SHIFTER also contains four temporary bitplane registers: IR1
        to IR4, where IR denotes Internal Register. Their behaviour is
        given by the following pseudo-code:

Number_of_read_bitplanes = 0
REPEAT
 IF Load is active THEN IR[Number_of_read_bitplanes] = Data sent from MMU
                        Number_of_read_bitplanes = Number_of_read_bitplanes + 1
 IF Number_of_read_bitplanes == 4 AND IF DE is active
    THEN RR1 = IR1; RR2 = IR2; RR3 = IR3; RR4 = IR4
 IF Number_of_read_bitplanes == 4 THEN Number_of_read_bitplanes = 0
END_REPEAT

        This process is continuous, and depends in no way on the
        synchronisation signals sent by the GLUE. Thus the RR registers
        are only reset when four bitplanes were received. As the RR
        registers are permanently rotated, they are emptied when the MMU
        sends no data or when DE (Display Enable) is off. In low and
        medium resolutions, each shift fills the least significant bit
        with a zero, which explains why the border colour is determined
        by colour zero in the palette. In high resolution they are
        filled by the value of bit 0 of colour 0 (signal BE)
        guaranteeing that the border is always black. This avoids the
        use of the Blank signal when the monitor's electron gun returns
        to the next line. This can be verified by causing the SHIFTER to
        display something during this time: it is displayed as a
        diagonal trace on the screen (it is extended and grey rather
        than white as the electron beam is moving quickly).

        Note that every change of resolution immediately changes the
        SHIFTER's behaviour: RR1 to RR4 will be treated differently, and
        any momentary switch to high resolution will appear as a black
        line on a colour monitor and a horizontal line on the monochrome
        monitor. This corresponds to a temporary disactivation of the
        RGB pins and the activation of the Mono pin. If transition
        occurs during the useable screen, the effect obtained differs on
        STF's and STE's, and differs according to the exact position on
        the screen in which it occured. For instance, there is a
        position from which a black line will extend all the way to the
        extreme right of the screen. This corresponds to the activation
        of the BLANK signal by the GLUE. These effects do not change the
        way in which the screen is decoded, or the synchronisation
        signals are generated.

        Shifts of the Useable Screen

        In the usual non-overscan scenario, DE is activated and
        disactivated 16 pixels before and after each line of the useable
        screen. The SHIFTER will only display the four IR values once it
        has received all of them. But if some of the IR registers were
        already filled, it won't need to wait to load all of them before
        loading the RR registers. Since the RR registers are loaded
        earlier, the useable screen will be shifted left. This shift
        will correspond directly to the number of words less that it did
        not need to wait for. As we saw, the MMU sends a new word to the
        SHIFTER every 500ns. Thus if only IR1 were filled, the image
        would be shifted 500ns left (corresponding to 4 pixels in low
        resolution). IR1 and IR2 correspond to 8 pixels, and so on.

        Note that this effect is perpetuated from VBL to VBL: if at the
        end of a VBL, the IR registers are not all empty, the next
        useable screen will be shifted left. If one changes resolution
        during the useable screen, depending on the location of the
        change, it is possible to affect the normal functioning of the
        IR registers. Such changes of resolution can result in the
        bitplanes being shifted. Finally this description is incomplete,
        because it does not account for screens where one observes
        alternating bands of useable screen and border.

 
        Consider now a line with the left border removed: it contains
        186 bytes, or 92+1 words. 92 is a multiple of 4, so there is a
        word too many per line. As we know, the beginning of the right
        border is due to DE being disactivated, so its position will not
        vary: if there is one word too many per line it will be loaded
        on the left of the screen. As the SHIFTER displays border unless
        all IR registers have been loaded, the first line displayed will
        stop 500ns before a normal line would, i.e. 4 pixels left in low
        resolution. At the end of the first line IR1 is already loaded,
        so the second line should start 0.5us earlier. Similarly at the
        end of the second line both IR1 and IR2 are already loaded, so
        the third line should start 1us earlier. Thus in low resolution
        one would expect the following sequence of left shifts: 4-8-12-0
        pixels. This actually occurs rarely (one time out of 20),
        depending on power up conditions causing the the GLUE, MMU and
        SHIFTER to be slightly out of synch. Instead one usually obtains
        the truncation of line four pixels early corresponding to the
        disactivation of DE. Is IR1 ignored? Indeed at the beginning of
        each left overscan line there is a transition to high resolution
        for a few cycles to activate DE. The change to high resolution
        occurs before DE is activated. Then there are 500ns before the
        Load/Dcyc signal of the MMU is activated (because there are 94
        positions between the switch to monochrome and the switch to
        60Hz to remove the right border: that is to say when DE is
        usually disactivated, for 93 (92+1) words read by the MMU). The
        fact that DE is activated, but Load/Dcyc isn't has as effect to
        clear the last IR register loaded, and to decrement the
        Number_of_read_bitplanes register. Thus if the MMU responds
        earlier, which can happen if it is out of synch with the
        SHIFTER, one will obtain the expected sequence of 4-8-12-0 pixel
        shifts in low resolution. To conclude, left overscan alone is
        potentially unstable and it is unadvisable to use it alone.

 
        So why is right overscan stable? Each line is two words too
        long! In fact, right overscan is only relatively stable in 50Hz
        low resolution: in medium resolution, two words too many
        correspond to 16 pixels. As all the IR registers must be loaded
        before their contents is transfered to the RR register, every
        second line is shifted left 16 pixels. Similarly, at 60Hz in low
        resolution the same thing occurs with an 8 pixel left shift
        every second line: only IR3 and IR4 need to be loaded. The fact
        this does not occur at 50Hz means that Number_of_read_bitplanes
        must be reset to zero. The same effect occurs for 158 byte
        lines: they should be shifted by 12-8-4-0 pixels every 4 lines
        in low resolution, since they have 3 words too many. These
        anomalies are due to rare combinations of the DE and Load/Dcyc
        signals as in the case of the left border: Load/Dcyc is active
        when DE is disactivated, or DE is active while Load/Dcyc have
        not been activated in the last 500ns.

* INTERESTING POSITIONS

; A list of positions follows: the first number
; corresponds to Mono_1, the second to Mono_2 and so on.
; The added fourth and fifth timings correspond to the time taken
; by the routines that change the resolution of the display frequency.
; Changing the display frequency takes 4*4 cycles, or 4 nops.
; Changing the display resolution takes 5*4 cycles, or 5 nops.
; Note that the 0 byte line obtained by changing display frequency
; does not occur on some power-ups; and that the 0 byte line obtained
; by switching to high resolution distorts the picture on some monitors:
; change the height of line 6 to see this effect.
; Complete Overscan: 1,0,0,1,1,89,13,9,230,2
; Left Overscan:     1,0,0,0,0,89+13+9+5+4,0,0,186,0
; Right Overscan:    0,0,0,1,0,89+5,13+9+5,204,0
; 80 byte line:      1,1,0,0,0,35,89+13+9+4-35,80,0
; 54 byte line:      0,1,0,0,0,35+5,89+13+9+4-35,54,0
; Right Overscan2:   0,1,0,0,0,89+5,13+9+4,0,204,0  (Switching to monochrome)
; 0 byte overscan:   0,0,0,1,0,14,75+5+13+9+5+5,0,0,0 (Changing frequency)
; 0 byte overscan:   0,1,0,0,0,6,89+13+9+5+4-6,0,0,0 (Switching to monochrome)

*/

#if defined(SS_VIDEO)

struct TScanline {
  int StartCycle; // eg 56
  int EndCycle; // eg 376
  int Bytes; // eg 160 - TODO make sure it's always correct
  int Cycles; // eg 512 
  int Tricks; // see mask description above
};


struct TShifter {
/*  As explained by ST-CNX and others, the video picture is produced by the
    MMU, the GLUE and the shifter. 
    In our emulation, we pretend that the shifter does most of it by itself:
    fetching memory, generating signals... It's a simplification. Otherwise
    we could imagine new objects: TMMU, TGlue...
*/
  TShifter(); 
  ~TShifter();
  inline void AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra);
  inline int CheckFreq(int t);
  void CheckSideOverscan(); // left & right border effects
  void CheckVerticalOverscan(); // top & bottom borders
  void CheckSyncTrick(); // at end of HBL, check if +2 -2 were correct
#if defined(WIN32)
  inline void DrawBufferedScanlineToVideo();
#endif
  void DrawScanlineToEnd();
  inline int FetchingLine();
  int IncScanline();
  void Render(int cycles_since_hbl, int dispatcher=DISPATCHER_NONE);
  inline RoundCycles(int &cycles_in);
  inline void SetPal(int n, WORD NewPal);
  void SetShiftMode(BYTE NewRes);
  void SetSyncMode(BYTE NewSync);
  void Vbl();

#if defined(SS_VID_STEEM_EXTENDED) // some functions aren't used
  inline void AddFreqChange(int f);
  inline void AddShiftModeChange(int r);
  inline int CheckShiftMode(int t);
  inline int FreqChange(int idx);
  inline int ShiftMode(int idx=-1);
  inline int FreqChangeCycle(int idx);
  inline int ShiftModeChangeCycle(int idx);
  inline int FreqChangeIdx(int cycle);
  inline int ShiftModeChangeIdx(int cycle);
  inline int FreqChangeAtCycle(int cycle);
  inline int ShiftModeChangeAtCycle(int cycle);
  // value before this cycle (and a possible change)
  inline int FreqAtCycle(int cycle);
  inline int ShiftModeAtCycle(int cycle);
  // cycle of next change to whatever value after this cycle
  inline int NextFreqChange(int cycle,int value=-1);
  inline int NextShiftModeChange(int cycle,int value=-1);
  // idx of next change to whatever value after this cycle
  inline int NextFreqChangeIdx(int cycle);
  inline int NextShiftModeChangeIdx(int cycle);
  // cycle of previous change to whatever value before this cycle
  inline int PreviousFreqChange(int cycle);
  inline int PreviousShiftModeChange(int cycle);
  // idx of previous change to whatever value before this cycle
  inline int PreviousFreqChangeIdx(int cycle);
  inline int PreviousShiftModeChangeIdx(int cycle);
  inline int CycleOfLastChangeToFreq(int value);
  inline int CycleOfLastChangeToShiftMode(int value);
#endif

#if defined(SS_VID_SDP)
#ifdef SS_VID_SDP_READ
  inline MEM_ADDRESS ReadSDP(int cycles_since_hbl,int dispatcher=DISPATCHER_NONE);
#endif
#ifdef SS_VID_SDP_WRITE
#if defined(_DEBUG) // VC IDE
  int WriteSDP(MEM_ADDRESS addr, BYTE io_src_b);
#else
  inline int WriteSDP(MEM_ADDRESS addr, BYTE io_src_b);
#endif
  int SDPMiddleByte; // glue it! 
#endif 
#endif

  // we need keep info for only 3 scanlines (pb: info for top/bottom?)
  TScanline PreviousScanline, CurrentScanline, NextScanline;

  int ExtraAdded;//rather silly
  int HblStartingHscroll; // saving true hscroll in MED RES (no use)
  int HblPixelShift; //
#if defined(WIN32)
  BYTE *ScanlineBuffer;
#endif
  int m_ShiftMode; // contrary to screen_res, it's updated at each change
  int TrickExecuted; //make sure that each trick will only be applied once
#if defined(SS_DEBUG) || defined(SS_INT_VBI_START)
  int nVbl;
#endif

};

extern TShifter Shifter; // singleton

#endif

////////////////////////////////
// TShifter: inline functions //
////////////////////////////////

inline void TShifter::AddExtraToShifterDrawPointerAtEndOfLine(unsigned long &extra) {
  // What a beautiful name!
  // Replacing macro ADD_EXTRA_TO_SHIFTER_DRAW_POINTER_AT_END_OF_LINE(s)

  if(ExtraAdded)
  {
    ASSERT(!overscan_add_extra);
    return;
  }
  extra+=(shifter_fetch_extra_words)*2;     
/*
Left border off + STE scrolling
For STE scrolling, the shifter must load the extra raster that will be used
to skip h pixels. But when the left border has been removed in the STF way (+
26 bytes), the shifter already has 6 bytes inside and will load only 2 more.
This doesn't happen with the STE-only left off (+20 bytes), hence we undo then
what we do here. Maybe.
Also, when must this be applied? Should be moved?
*/
  if(shifter_skip_raster_for_hscroll) 
#if defined(SS_VID_STE_MED_HSCROLL)
    extra+= (left_border) ? 
    (screen_res==1) ? 4 : 8 // handle MED RES, fixes Cool STE unreadable scrollers
    : 2; 
#else
    extra+= (left_border) ? 8 : 2;
#endif
  extra+=overscan_add_extra;
  overscan_add_extra=0;
  ExtraAdded++;
}


inline int TShifter::CheckFreq(int t) {
/* This is taken from Steem's draw_check_border_removal()
   v3.4: rewritten with a 'for' instead of a 'while' to avoid a 'break'
   (just C masturbation)
*/
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++);
  if(j==32)
    i=-1; // this ugly thing still necessary anyway
  return i;
}


#ifdef WIN32

inline void TShifter::DrawBufferedScanlineToVideo() {
  // replacing macro DRAW_BUFFERED_SCANLINE_TO_VIDEO
  if(draw_store_dest_ad)
  { 
    // Bytes that will be copied.
    int amount_drawn=(int)(draw_dest_ad-draw_temp_line_buf); 
    // From draw_temp_line_buf to draw_store_dest_ad
    DWORD *src=(DWORD*)draw_temp_line_buf; 
    DWORD *dest=(DWORD*)draw_store_dest_ad;  
    while(src<(DWORD*)draw_dest_ad) 
      *(dest++)=*(src++); 
    ASSERT(draw_med_low_double_height);
    if(draw_med_low_double_height) // ???
    {
      src=(DWORD*)draw_temp_line_buf;                        
      dest=(DWORD*)(draw_store_dest_ad+draw_line_length);      
      while(src<(DWORD*)draw_dest_ad) 
        *(dest++)=*(src++);       
    }                                                              
    draw_dest_ad=draw_store_dest_ad+amount_drawn;                    
    draw_store_dest_ad=NULL;                                           
    draw_scanline=draw_store_draw_scanline; 
  }
  ScanlineBuffer=NULL;
}

#endif


inline int TShifter::FetchingLine() {
  // does the current scan_y involve fetching by the shifter?
  // notice < shifter_last_draw_line, not <=
  return (scan_y>=shifter_first_draw_line && scan_y<shifter_last_draw_line);
}


inline TShifter::RoundCycles(int& cycles_in) {
  cycles_in-=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !shifter_hscroll) 
    cycles_in+=16;
  cycles_in+=SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in&=-SHIFTER_RASTER_PREFETCH_TIMING;
  cycles_in+=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN;
  if(shifter_hscroll_extra_fetch && !shifter_hscroll) 
    cycles_in-=16;
}


inline void TShifter::SetPal(int n, WORD NewPal) {
/*        STF
    A sixteen word color lookup  palette  is
    provided  with  nine  bits  of color per entry.  The sixteen
    Color Palette Registers (read/write,  reset:  not  affected)
    contain  three  bits  of red, green, and blue aligned on low
    nibble boundaries.  Eight intensity  levels  of  red,  eight
    intensity  levels  of  green,  and eight intensity levels of
    blue produce a total of 512 possible colors.
    In 320 x 200 4 plane mode all  sixteen  palette  colors
    can  be  indexed,  while  in 640 x 200 2 plane mode only the
    first four palette entries are applicable.   In  640  x  400
    monochrome mode the color palette is bypassed altogether and
    is instead provided with an inverter for inverse video  con-
    trolled  by  bit 0 of palette color 0 (normal video is black
    0, white 1).  Color palette memory is arranged the  same  as
    main  memory.   Palette  color  0  is  also used to assign a
    border color while in a  multi-plane  mode.   In  monochrome
    mode the border color is always black.

    ff 8240   R/W     |-----xxx-xxx-xxx|   Palette Color 0/0 (Border)
                            ||| ||| |||
                            ||| ||| || ----   Inverted/_Normal Monochrome
                            ||| ||| |||
                            ||| |||  ------   Blue
                            |||  ----------   Green
                             --------------   Red
    ff 8242   R/W     |-----xxx-xxx-xxx|   Palette Color 1/1
    ff 8244   R/W     |-----xxx-xxx-xxx|   Palette Color 2/2
    ff 8246   R/W     |-----xxx-xxx-xxx|   Palette Color 3/3
    ff 8248   R/W     |-----xxx-xxx-xxx|   Palette Color 4
    ff 824a   R/W     |-----xxx-xxx-xxx|   Palette Color 5
    ff 824c   R/W     |-----xxx-xxx-xxx|   Palette Color 6
    ff 824e   R/W     |-----xxx-xxx-xxx|   Palette Color 7
    ff 8250   R/W     |-----xxx-xxx-xxx|   Palette Color 8
    ff 8252   R/W     |-----xxx-xxx-xxx|   Palette Color 9
    ff 8254   R/W     |-----xxx-xxx-xxx|   Palette Color 10
    ff 8256   R/W     |-----xxx-xxx-xxx|   Palette Color 11
    ff 8258   R/W     |-----xxx-xxx-xxx|   Palette Color 12
    ff 825a   R/W     |-----xxx-xxx-xxx|   Palette Color 13
    ff 825c   R/W     |-----xxx-xxx-xxx|   Palette Color 14
    ff 825e   R/W     |-----xxx-xxx-xxx|   Palette Color 15

          STE 4096 Colours oh boy!
FF8240 ---- 0321 0321 0321
through     Red Green Blue
FF825E


  Palette Register:                                 ST     STE

    $FFFF8240  0 0 0 0 x X X X x X X X x X X X       X     x+X

    $FFFF8242  0 0 0 0 x X X X x X X X x X X X ...

    ...

    $FFFF825E  0 0 0 0 x X X X x X X X x X X X    (16 in total)

                       ---_--- ---_--- ---_---

                         red    green    blue

  These 16 registers serve exactly the same purpose as in the ST. 
  The only difference is that each Nibble for Red, Green or Blue 
  consists of 4 bits instead of 3 on the ST.

  For compatibility reasons, each nibble encoding the Red, Green or Blue
   values is not ordered "8 4 2 1", meaning the least significant 
   bit represents the value "1" while the most significant one represents 
   the value "8" - This would make the STE rather incompatible with the ST 
   and only display dark colours. The sequence of Bits is "0 3 2 1" encoding 
   the values "1 8 4 2". Therefore to fade from black to white, the sequence 
   of colours would be $000, $888, $111, $999, $222, $AAA, ...

*/
#if defined(SS_STF)
  if(ST_TYPE!=STE)
    NewPal &= 0x0777; // fixes Forest HW STF test
  else
#endif
    NewPal &= 0x0FFF;
#if defined(SS_VID_AUTOOFF_DECRUNCHING)
  if(!n && NewPal && NewPal!=0x777) // basic test, but who uses autoborder?
    overscan=OVERSCAN_MAX_COUNTDOWN;
#endif
    if(STpal[n]!=NewPal)
    {
      int CyclesIn=LINECYCLES;

#if defined(SHIFTER_PREFETCH_LATENCY)
      if(draw_lock && CyclesIn>SHIFTER_PREFETCH_LATENCY+SHIFTER_RASTER_PREFETCH_TIMING)
#else
      if(draw_lock)
#endif
        Shifter.Render(CyclesIn,DISPATCHER_SET_PAL);
      STpal[n]=NewPal;
      PAL_DPEEK(n*2)=STpal[n];
      if(!flashlight_flag && !draw_line_off DEBUG_ONLY(&&!debug_cycle_colours))
        palette_convert(n);
    }
}


#if defined(SS_VID_STEEM_EXTENDED)

/* V.3.3 used Hatari analysis. Now that we do without this hack, we need
   look-up functions to extend the existing Steem system.
   To make it easier we duplicate the 'freq_change' array for shift mode (res)
   changes (every function is duplicated even if not used).
   TODO: debug & optimise (seek only once)
*/

inline void TShifter::AddFreqChange(int f) {
  // Replacing macro ADD_SHIFTER_FREQ_CHANGE(shifter_freq)
  shifter_freq_change_idx++;
  shifter_freq_change_idx&=31;
  shifter_freq_change_time[shifter_freq_change_idx]=ABSOLUTE_CPU_TIME;
  shifter_freq_change[shifter_freq_change_idx]=f;                    
}

inline void TShifter::AddShiftModeChange(int mode) {
  // called only by SetShiftMode
  ASSERT(!mode||mode==1||mode==2);
  shifter_shift_mode_change_idx++;
  shifter_shift_mode_change_idx&=31;
  shifter_shift_mode_change_time[shifter_shift_mode_change_idx]=ABSOLUTE_CPU_TIME;
  shifter_shift_mode_change[shifter_shift_mode_change_idx]=mode;                    
}


inline int TShifter::CheckShiftMode(int t) {
  // this is inspired by the original Steem tests in draw_check_border_removal
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++);
  if(j==32)
    i=-1;
  return i;
}


inline int TShifter::FreqChange(int idx) {
  // return value of this change (50 or 60)
  idx&=31;
  ASSERT(shifter_freq_change[idx]==50||shifter_freq_change[idx]==60);
  return shifter_freq_change[idx];
}

inline int TShifter::ShiftMode(int idx) { // .. Change
  // return value of this change (0,1,2)
  if(idx==-1)
    return m_ShiftMode;
  idx&=31;
  ASSERT(!shifter_shift_mode_change[idx]||shifter_shift_mode_change[idx]==1||shifter_shift_mode_change[idx]==2);
  return shifter_shift_mode_change[idx];
}


inline int TShifter::FreqChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(shifter_freq_change[idx]==50||shifter_freq_change[idx]==60);
  int rv=shifter_freq_change_time[idx]-LINECYCLE0;
  return rv;
}

inline int TShifter::ShiftModeChangeCycle(int idx) {
  // just give the relative cycle of element idx
  idx&=31; // so you can send idx+1 or whatever here
  ASSERT(!shifter_shift_mode_change[idx]||shifter_shift_mode_change[idx]==1||shifter_shift_mode_change[idx]==2);
  int rv=shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return rv;
}


inline int TShifter::FreqChangeAtCycle(int cycle) {
  // if there was a change at this cycle, return it, otherwise -1
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  // loop while it's bigger than cycle, with safety
  for(i=shifter_freq_change_idx,j=0;
  j<32 && shifter_freq_change_time[i]-t>0;
  j++,i--,i&=31);
  // here, we're on the right cycle, or smaller
  int rv=(j<32 && !(shifter_freq_change_time[i]-t))
    ?shifter_freq_change[i]:-1;
  return rv;
}

inline int TShifter::ShiftModeChangeAtCycle(int cycle) {
  // if there was a change at this cycle, return it, otherwise -1
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  // loop while it's bigger than cycle, with safety
  for(i=shifter_shift_mode_change_idx,j=0;
  j<32 && shifter_shift_mode_change_time[i]-t>0;
  j++,i--,i&=31);
  // here, we're on the right cycle, or smaller
  int rv=(j<32 && !(shifter_shift_mode_change_time[i]-t))
    ?shifter_shift_mode_change[i]:-1;
  return rv;
}


inline int TShifter::FreqChangeIdx(int cycle) {
  // give the idx of freq change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckFreq(t); // use the <= check
  if(idx!=-1 &&shifter_freq_change_time[idx]==t)
    return idx;
  return -1;
}

inline int TShifter::ShiftModeChangeIdx(int cycle) {
  // give the idx of shift mode change at this cycle, if any
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx=CheckShiftMode(t); // use the <= check
  if(idx!=-1 &&shifter_shift_mode_change_time[idx]==t)
    return idx;
  return -1;
}


inline int TShifter::FreqAtCycle(int cycle) {
  // what was the frequency just before we reach this cycle?
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<0)
    return shifter_freq_change[i];
  return shifter_freq_at_start_of_vbl;
}

inline int TShifter::ShiftModeAtCycle(int cycle) {
  // what was the shift mode just before we reach this cycle?
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return shifter_shift_mode_change[i];
  return 0;
}

// REDO it stinks
inline int TShifter::NextFreqChange(int cycle,int value) {
  // return cycle of next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    if(value==-1 || shifter_shift_mode_change[i]==value)
      idx=i;
  if(idx!=-1 && shifter_freq_change_time[idx]-t>0)
    return shifter_freq_change_time[idx]-LINECYCLE0;
  return -1;
}

inline int TShifter::NextShiftModeChange(int cycle,int value) {
  // return cycle of next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=-1,i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    if(value==-1 || shifter_shift_mode_change[i]==value)
      idx=i;
  if(idx!=-1 && shifter_shift_mode_change_time[idx]-t>0)
    return shifter_shift_mode_change_time[idx]-LINECYCLE0;
  return -1;
}


inline int TShifter::NextFreqChangeIdx(int cycle) {
  // return idx next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    idx=i;
  if(shifter_freq_change_time[idx]-t>0)
    return idx;
  return -1;
}

inline int TShifter::NextShiftModeChangeIdx(int cycle) {
  // return idx next change after this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int idx,i,j;
  for(idx=i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>0 && j<32
    ; i--,i&=31,j++)
    idx=i;
  if(shifter_shift_mode_change_time[idx]-t>0)
    return idx;
  return -1;
}


inline int TShifter::PreviousFreqChange(int cycle) {
  // return cycle of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<0)
    return shifter_freq_change_time[i]-LINECYCLE0;
  return -1;
}

inline int TShifter::PreviousShiftModeChange(int cycle) {
  // return cycle of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}


inline int TShifter::PreviousFreqChangeIdx(int cycle) {
  // return idx of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change_time[i]-t<0)
    return i; 
  return -1;
}

inline int TShifter::PreviousShiftModeChangeIdx(int cycle) {
  // return idx of previous change before this cycle
  int t=cycle+LINECYCLE0; // convert to absolute
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change_time[i]-t>=0 && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change_time[i]-t<0)
    return i;
  return -1;
}


inline int TShifter::CycleOfLastChangeToFreq(int value) {
  int i,j;
  for(i=shifter_freq_change_idx,j=0
    ; shifter_freq_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_freq_change[i]==value)
    return shifter_freq_change_time[i]-LINECYCLE0;
  return -1;
}

inline int TShifter::CycleOfLastChangeToShiftMode(int value) {
  int i,j;
  for(i=shifter_shift_mode_change_idx,j=0
    ; shifter_shift_mode_change[i]!=value && j<32
    ; i--,i&=31,j++) ;
  if(shifter_shift_mode_change[i]==value)
    return shifter_shift_mode_change_time[i]-LINECYCLE0;
  return -1;
}

#endif



//////////////////////////
// Shifter draw pointer //
//////////////////////////

/* 
    Video Address Counter:

    STF - read only, writes ignored
    ff 8205   R               |xxxxxxxx|   Video Address Counter High
    ff 8207   R               |xxxxxxxx|   Video Address Counter Mid
    ff 8209   R               |xxxxxxxx|   Video Address Counter Low

    This counter is used by programs that do side overscan. This is handled
    in ReadSDP, based on Steem 3.2, which worked very well for most cases.
    Cases that didn't work: Forest, SNYD/TCB. Enchanted Land somehow.

    Flix:

    In  order to make some commands,  like the colour or  our 
    frequency  switchings,  occur at exactly the same  position,  you 
    have  to  become synchronized with  the  raster-electron-ray.  An 
    ingenious method to achieve this effect is the following one:

    WAIT:     MOVE.B    $FF8209.W,D0   ; Low-Byte
              BEQ.S     WAIT           ; mustn't be 0
              NOT.B     D0             ; negate D0
              LSL.B     D0,D0          ; Synchronisation

    If  you execute this routine every VBL,  all  following  commands 
    will be executed at the same position every VBL,  that means that 
    the colour- or frequency-switches are stable.  But what does this 
    little routine do?  At first,  the low-byte of the screen-address 
    is  loaded  into D0.  This byte exactly determines  the  position 
    within  the line.  It is negated and the LSL-command is  executed 
    (LSR,  ASL  or  ASR  work as well).  As you  can  read  in  every 
    processor-book  the LSL-command takes  8+2*n  clock-cycles.  That 
    means  that  the command needs more clock-cycles the  bigger  the 
    value in D0 is. That is exactly the shifting that we need! I hope 
    that you understood this part,  because all fullscreens and sync-
    scrolling-routines  are based upon this effect.  You should  know 
    that  one  VBL  (50  Hz) consists  of  160000  clock-cycles  (one 
    scanline  consists of 512 clock-cycles).

    STE - read & write

    FF8204 ---- ---- --xx xxxx (High)
    FF8206 ---- ---- xxxx xxxx
    FF8208 ---- ---- xxxx xxx- (Low)

    Video Address Counter.
    Now read/write. Allows update of the video refresh address during the 
    frame. 
    The effect is immediate, therefore it should be reloaded carefully 
    (or during blanking) to provide reliable results. 

    According to ST-CNX, those registers are in the MMU, not in the shifter.

*/

#if defined(SS_VID_SDP_READ)

inline MEM_ADDRESS TShifter::ReadSDP(int cycles_since_hbl,int dispatcher) {
  if (bad_drawing){
    // Fake SDP
#if defined(SS_VID_SDP_TRACE)
    TRACE("fake SDP\n");
#endif
    if (scan_y<0){
      return xbios2;
    }else if (scan_y<shifter_y){
      int line_len=(160/res_vertical_scale);
      return xbios2 + scan_y*line_len + min(cycles_since_hbl/2,line_len) & ~1;
    }else{
      return xbios2+32000;
    }
  }

  CheckSideOverscan();
  MEM_ADDRESS sdp; // return value
  if(FetchingLine())
  {
    int bytes_to_count=CurrentScanline.Bytes; // implicit fixes (Forest)
    if(shifter_skip_raster_for_hscroll)
      bytes_to_count+=SHIFTER_RASTER_PREFETCH_TIMING/2;
    int bytes_ahead=(shifter_hscroll_extra_fetch) 
      ?(SHIFTER_RASTER_PREFETCH_TIMING/2)*2:(SHIFTER_RASTER_PREFETCH_TIMING/2);
    int starts_counting=CYCLES_FROM_HBL_TO_LEFT_BORDER_OPEN/2 - bytes_ahead;
    if(!left_border)
      starts_counting-=26;

#if defined(SS_VID_TCB) && defined(SS_VID_STEEM_EXTENDED)
    else if(CurrentScanline.Tricks&TRICK_LINE_PLUS_2 
      // 512 cycles 60hz lines, we must compensate (don't understand it all
      && PreviousScanline.Cycles==512)  // but it works)
      starts_counting+=2; //fixes Swedish New Year Demo/TCB
#endif

    int c=cycles_since_hbl/2-starts_counting;
    sdp=shifter_draw_pointer_at_start_of_line;
    if (c>=bytes_to_count)
      sdp+=bytes_to_count+(shifter_fetch_extra_words*2);
    else if (c>=0){
      c&=-2;
      sdp+=c;
    }
    
#if defined(SS_VID_SDP_TRACE3) // compare with Steem (can't be 100%)
    if(sdp>shifter_draw_pointer_at_start_of_line)
    {
      MEM_ADDRESS sdpdbg=get_shifter_draw_pointer(cycles_since_hbl);
      if(sdpdbg!=sdp)
      {
        TRACE("SDP 0 %X %d %X (+%d) Steem 3.2 %X (+%d)\n",shifter_draw_pointer_at_start_of_line,cycles_since_hbl,sdp,sdp-shifter_draw_pointer_at_start_of_line,sdpdbg,sdpdbg-shifter_draw_pointer_at_start_of_line);
        MEM_ADDRESS sdpdbg=get_shifter_draw_pointer(cycles_since_hbl); // rego...
      }
    }
#endif
  }
  else // lines witout fetching (before or after frame)
    sdp=shifter_draw_pointer;

#if defined(SS_VID_SDP_TRACE2)
  TRACE("Read SDP F%d y%d c%d SDP %X (%d - %d) sdp %X\n",FRAME,scan_y,cycles_since_hbl,sdp,sdp-shifter_draw_pointer_at_start_of_line,CurrentScanline.Bytes,shifter_draw_pointer);
#endif

  return sdp;
}

#endif

#ifdef SS_VID_SDP_WRITE

int TShifter::WriteSDP(MEM_ADDRESS addr, BYTE io_src_b) {
/*
    This is a difficult side of STE emulation, made difficult too by
    Steem's rendering system, that uses the shifter draw pointer as 
    video pointer as well as ST IO registers. Other emulators may do 
    otherwise, but there are difficulties and imprecisions anyway.
    So this is mainly a collection of hacks for now, that handle pratical
    cases.
    TODO: examine interaction write SDP/left off in Steem
    Some cases (all should display fine):

    An Cool STE
    Line 000 - 008:r0900 028:r0900 048:r0900 068:r0908 452:w050D 480:w07F1 508:w0990 512:T1000
    Not in DE

    Asylum
    Line 001 - Events:  052:V0000 056:w050B 060:w0746 064:w09EE
    It's in fact not in DE, because SDP starts with delay?

    Beat Demo
    Line 199 - 016:F0096 456:S0000 476:w0502 500:w0711
    Line 200 - 012:w09A2

    Big Wobble
    Line -27 - 404:w0505 428:w079A 452:w090A 476:F0124 500:H0008 512:R0082 512:T1000
    Post DE, followed with... change Linewid, change HScroll, switch R2/R0
    Seems to require +4 in our system

    Braindamage 3D maze
    Line 117 - 016:r0900 036:r0900 056:r0900 076:r0904 456:r0776 456:w0775 476:P1002 484:P2002 492:P3002 500:P4002 508:P5002 512:T1000
    ...
    Issue can be: extra should have been added

    Cool STE
    Line -61 - 052:H0013 128:w09C6 162:w0505 166:w07DD 372:F0156
    Line 113 - 344:F0086 368:H0015 396:w0501 400:w0765 424:w093E 
    Line 149 - 156:H0015 384:w0501 388:w07D1 412:w092E
    Line 183 - 336:F0156 356:H0013 380:w0507 384:w0736 404:w09C6
    It's in fact "post-DE" (?), LINEWID mustn't apply or it flickers
    It must have been added before

    Coreflakes 3D titles
    Line -28 - 012:R0000 376:S0000 392:S0002 408:PA707 424:P3707 440:R0001 456:R0000 468:w070D 472:w0998 488:P2707 508:R0002 512:T0011
    Notice the switch at 440/456 is R1/R0
    All lines are like this, there should be no shift at all

    Dangerous Fantaisy credits
    Line -49 - 232:H000C 244:F0076 280:w050A 296:w07A8 312:w0918 472:P0000 476:P1077 496:P2067 500:P3056
    Line 084 - 224:H0000 248:F0000 388:w0900 422:w0766 440:w050B 512:T1000
    Line 179 - 336:H0000 360:F0000 392:w0900 426:w07C5 444:w050B 460:P0222 512:T1000
    The write to lower byte in line 179 may occur at 384 (bytes in: 158, drawn: 152)

    E605 planet
    Line -29 - 024:S0002 036:r0900 056:r0900 076:r0904 480:h0000 480:H000A 496:P00FF 508:P00F7
    Line -28 - ...504:R0002
    Line -27 - 008:R0000 376:S0000 392:S0002 416:V0000 420:w0507 424:w07E0 428:w09D6 440:R0002 456:R0000 504:R0002
    instruction used movep.l

    Molz Green pic
    Line -63 - 432:V0000 436:w0514 440:w0759 444:w0900 not DE
    Line -28 - 040:V0000 044:w051D 048:w0799 052:w0940  not DE
    Line 245 - 212:V0000 216:w0500 220:w0700 224:w0900  in DE but doesn't count

    Pacemaker credits
    ...
    Line 063 - 472:V0000 476:w0507 480:w070F 484:w0940 500:P0000 504:P102E
    ...
    Line 070 - 060:V0000 064:w0507 068:w0717 072:w0980 088:P02AB 092:P102E 472:V0000 476:w0507 480:w0718 484:w09E0 500:P0A34 504:P102E
    ...
    Line 079 - Events:  472:V0000 476:w0507 480:w0725 484:w09F0 500:P0000 504:P10A7
    Only 1 while DE, and there's another write post DE, which cancels the 
    'bytes to add' (which makes it work in Hatari?)

    RGBeast
    Line -29 - 072:r0902 332:w0502 336:w07E6 340:w0903 ... 376:S0192 384:S0130
    DE, right border still to be removed
    instruction used movep.l

    Schnusdie (Reality is a Lie)
    Line 199 - 020:R0000 376:S0000 396:S0002 424:w0503 452:w071D 472:S0000 500:w092C 512:T3111
    Line 200 - 020:S0002 376:S0000 396:S0002 428:P0000 432:P1019 436:P2092 440:P302A 444:P40A3 448:P503B 452:P60B4 456:P704C 460:P80C5 464:P905D 468:PA0D6 472:PB06E 476:PC0E7 480:PD07F 484:PE0FF 488:PF0FF 508:R0002 512:T0010
    Line 201 - 020:R0000 376:S0000 396:S0002 428:P0000 432:P1819 436:P2892 440:P382A 444:P48A3 448:P583B 452:P68B4 456:P784C 460:P88C5 464:P985D 468:PA8D6 472:PB86E 476:PC8E7 480:PD87F 484:PE8FF 488:PF8FF 508:R0002 512:T0011

    Sunny
    Line -17 - 400:w09E2 440:w077F 480:w0503 (eg)

    Tekila
    Line -29 - 016:S0130 136:r0922 240:V0006 256:V0000 272:w0900 508:R0002
    Other lines
    note that other lines must be aligned with first visible line
    012:R0000 376:S0000 388:S0130 408:w0506 424:w07D3 436:w0986 ... 468:H0008 508:R0002
    -29 & -28 are black,  write in -29 has no visible effect
    instruction used move.b

*/

  int cycles=LINECYCLES; // cycle in shifter reckoning

#if defined(SS_VID_SDP_TRACE2)
  TRACE("F%d y%d c%d Write %X to %X\n",FRAME,scan_y,cycles,io_src_b,addr);
#endif
#if defined(SS_VID_SHIFTER_EVENTS)
  VideoEvents.Add(scan_y,cycles,'w',((addr&0xF)<<8)|io_src_b);
#endif

#if defined(SS_STF)
  // some STF programs write to those addresses, it just must be ignored.
  if(ST_TYPE!=STE)
  {
#if defined(SS_VID_SDP_TRACE)
    TRACE("STF ignore write to SDP %x %x\n",addr,io_src_b);
#endif
    return FALSE; // fixes Nightdawn
  }
#endif

  // some bits will stay at 0 in the STE whatever you write
  if(mem_len<=FOUR_MEGS && addr==0xFF8205) // already in Steem 3.2 (not in 3.3!)
    io_src_b&=0x3F; // fixes Delirious IV bugged STE detection routine
#if defined(SS_VID_SDP_WRITE_LOWER_BYTE)
  else if(addr==0xFF8209)
    io_src_b&=0xFE; // fixes RGBeast mush (yoho!)
#endif

  BOOL fl=FetchingLine();
  BOOL de_started=fl && cycles>=CurrentScanline.StartCycle
    +SHIFTER_PREFETCH_LATENCY; // TESTING
  BOOL de_finished=de_started && cycles>CurrentScanline.EndCycle
    +SHIFTER_PREFETCH_LATENCY; // TESTING
  BOOL de=de_started && !de_finished;
  BOOL middle_byte_corrected=FALSE;

  Render(cycles,DISPATCHER_WRITE_SDP);

#if defined(SS_VID_SDP_READ)
  MEM_ADDRESS sdp_real=ReadSDP(cycles,DISPATCHER_WRITE_SDP); 
#else
  MEM_ADDRESS sdp_real=get_shifter_draw_pointer(cycles);
#endif
  ///int sdp_current_byte=( sdp_real>>8*((0xff8209-addr)/2) ) & 0xFF;
  int bytes_in=sdp_real-shifter_draw_pointer_at_start_of_line;
  // the 'draw' pointer can be <, = or > 'real'
  int bytes_drawn=shifter_draw_pointer-shifter_draw_pointer_at_start_of_line;

  if(addr==0xff8209 && SDPMiddleByte!=999) // it has been set?
  {
    int current_sdp_middle_byte=(shifter_draw_pointer&0xFF00)>>8;
    if(current_sdp_middle_byte != SDPMiddleByte) // need to restore?
    {
#if defined(SS_VID_SDP_TRACE)
      TRACE("F%d y%d c%d SDP %X reset middle byte from %X to %X\n",FRAME,scan_y,cycles,shifter_draw_pointer,current_sdp_middle_byte,SDPMiddleByte);
#endif
      DWORD_B(&shifter_draw_pointer,(0xff8209-0xff8207)/2)=SDPMiddleByte;
      middle_byte_corrected=TRUE;
    }
  }

  MEM_ADDRESS nsdp=shifter_draw_pointer; //sdp_real (should be!)
  DWORD_B(&nsdp,(0xff8209-addr)/2)=io_src_b;

  // Writing low byte while the shifter supposedly is fetching
  if(addr==0xFF8209 && de)
  {

#if defined(SS_VID_PACEMAKER)
    if(SSE_HACKS_ON&&(bytes_drawn==8||bytes_drawn==16)) 
    {
      nsdp+=bytes_drawn; // hack for Pacemaker credits line 70 flicker/shift
      overscan_add_extra-=bytes_drawn; 
    }
#endif

#if defined(SS_VID_DANGEROUS_FANTAISY)
    if(SSE_HACKS_ON 
      && bytes_drawn==CurrentScanline.Bytes-8 && bytes_in>bytes_drawn)
      nsdp+=-8; // hack for Dangerous Fantaisy credits lower overscan flicker
#endif

    // recompute right off bonus bytes - only necessary post-display
    // this part not considered a "hack", but is the computing correct?
    if(!right_border && !ExtraAdded && cycles>416)
    {
      int pxtodraw=320+BORDER_SIDE*2-scanline_drawn_so_far;
      int bytestofetch=pxtodraw/2;
      int bytes_to_run=(CurrentScanline.EndCycle-cycles+SHIFTER_PREFETCH_LATENCY)/2;
      ASSERT(CurrentScanline.EndCycle==460); bytes_to_run+=2; // is it 460 or 464?
      overscan_add_extra+=-44+bytes_to_run-bytestofetch;
    }

    // cancel the Steem 3.2 fix for left off with STE scrolling on
    if(!ExtraAdded && (CurrentScanline.Tricks&TRICK_LINE_PLUS_26)
      && shifter_hscroll>=12)
      overscan_add_extra+=8; // fixes bumpy scrolling in E605 Planet

#if defined(SS_VID_SDP_WRITE_DE_HSCROLL)
    if(SSE_HACKS_ON
      &&shifter_skip_raster_for_hscroll && !left_border && !ExtraAdded)
    {
      if(PreviousScanline.Tricks&TRICK_STABILISER)
        nsdp+=-2; // fixes D4/Tekila shift
      else
        nsdp+=4; // fixes E605 Planet shift
    }
#endif
  }

  if(de_finished && addr==0xff8209)
    CurrentScanline.Tricks|=TRICK_WRITE_SDP_POST_DE;

  // update shifter_draw_pointer_at_start_of_line or ReadSDP returns garbage
  shifter_draw_pointer_at_start_of_line-=shifter_draw_pointer; //sdp_real
  shifter_draw_pointer_at_start_of_line+=nsdp;
  shifter_draw_pointer=nsdp;

#if defined(SS_VID_SDP_WRITE_MIDDLE_BYTE)
  // hack, record middle byte, programmer couldn't intend it to change
  if(SSE_HACKS_ON && fl && addr==0xff8207)  
    SDPMiddleByte=io_src_b; // fixes Pacemaker credits, Tekila
#endif

  CurrentScanline.Tricks|=TRICK_WRITE_SDP; // could be handy

  return shifter_draw_pointer; // fancy
}

#endif


// just taking some unimportant code out of Render for clarity

#define   AUTO_BORDER_ADJUST  \
          if(!(border & 1)) { \
            if(scanline_drawn_so_far<BORDER_SIDE) { \
              border1-=(BORDER_SIDE-scanline_drawn_so_far); \
              if(border1<0){ \
                picture+=border1; \
                if(!screen_res) {  \
                  hscroll-=border1;  \
                  shifter_draw_pointer+=(hscroll/16)*8; \
                  hscroll&=15; \
                }else if(screen_res==1) { \
                  hscroll-=border1*2;  \
                  shifter_draw_pointer+=(hscroll/16)*4; \
                  hscroll&=15; \
                } \
                border1=0; \
                if(picture<0) picture=0; \
              } \
            } \
            int ta=(border1+picture+border2)-320; \
            if(ta>0) { \
              border2-=ta; \
              if(border2<0)  { \
                picture+=border2; \
                border2=0; \
                if (picture<0)  picture=0; \
              } \
            } \
            border1=border2=0; \
          }

#endif //defined(SS_VIDEO)

#endif//SSEVIDEO_H