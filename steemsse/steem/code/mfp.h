#if defined(STEVEN_SEAGAL) && defined(SSE_STRUCTURE_MFP_H)

#include "mfp.decla.h"

#else//!SSE_STRUCTURE_MFP_H

#ifdef IN_EMU
#define EXT
#define INIT(s) =s
#else
#define EXT extern
#define INIT(s)
#endif

/*
    The MPU is directly  supported  by  an  MK68901  Multi
    Function Peripheral providing general purpose interrupt con-
    trol and timers, among other things.

-----------------------------------------------------

               MC68901 Multi-Function Peripheral Chip

                    Jeff Rigby/SOTA Computers

The 68901 is a very inexpensive very powerful multi-function chip. In the
ST it is responsible for the interrupt generation of the busy line on the
printer port, the interrupt signal from the hard drive port and the prin-
ter port, Midi port, keyboard, and RS-232 port as well as the entire RS -
232 receive, transmit and controll lines (16 levels of interrupt ). In ad-
dition, the MFP can be daisy chained ( a second 68901 can be added allo -
wing the interrupt levels to be chained ). All this in a chip with a re -
tail cost of about $12.00.

How it works; the 68000 CPU has 7 levels of interrupt, one of those levels
points to the 68901 MFP, when the CPU is told that the  MFP needs  servi -
ceing, the address of the service routine is given controll, it addresses
$fffffa00 which the glue chip decodes and sends a low on the chip select
pin 48 of the MFP, then the routine sends an address to the lower 5 lines
of the address bus to select a register in the MFP and the routine, depen-
ding on what it needs to do writes or reads the registers in the chip.

The operating system has previously set the interrupt registers in the MFP
with the priorities the Interrupt lines I0-I7 and the internal receive and
transmit buffers require. The 68901 then talks directly to the 68000 tel -
ling it that one of it's interrupt's needs servicing, giving the vector
adress of the service routine, and this goes on until all the 68901 inter-
rupts are serviced then control is passed back to the 68000 and the pro -
cess starts all over again. If no interrupts need servicing then control
is passed back to the 68000 immediately.     

I'll try to make this as non-technical as possible.

MFP tells the Glue chip that it needs servicing. Glue thinks and says OK,
I guess it is your turn. Glue tells CPU that a IRQ level is pending. CPU
to Glue " what level is it ". CPU to Glue tell the MFP it's his turn. 
Glue tells MFP to go ahead ---> CS on MFP goes low MFP gives the CPU the
vector for the needed routine The routine services the MFP and clears the
Interrupt register. Control is passed back to the CPU and the next level
down interrupt has a chance.

Maybe that was too simple, the 68901 does alot more than that and I could
spend all day and 30 screens to tell you about it.

Literature is available from Motorola on the MC68901, I believe the 28 pa-
ge technical booklet is #ADI-984, The great thing is that we can add a se-
cond RS-232 very cheaply by daisy chaining the IEI and IEO pins on the
68901 and changing the vector addresses for the service routines.

A few suggestions for standards in the ST upgrades.

1) The output pin in the video out port be used for Monitor switching (Mo-
   no to Color). 

2) Pin 25 I3 of the 68901 MFP be used for an interrupt to tell software
   that the power has been interrupted. In battery backups in the 1040 all
   power 12v and 5V can be provided for 10 min with C cell Ni-Cad. The
   software has to be informed that power is out and to save the data.

   Suggest a low for power interrupt on pin 25.  

3) Address $fffffb00 be used for a second 68901 MFP and the second MFP be
   daisy chained using the IEO and IEI pins.

4) An address for battery backup clocks be picked as standard. I don't ha-
   ve any thoughts on this one but an address should be suggested. Anyone
   have an address that is already being used for this purpose and the
   reason it was chosen please respond.

As a hardware hacker, I am interested in other people's input on this sub-
ject. The great thing about PD software is that people build upon the work
of others and eventually the PD software can be better than commercial
software. I would like to see something like that started on the technical
side, if someone finds a flaw or a better way to describe the 68901 please
add it to this file and upload to CIS.

Jeff Rigby/SOTA Computers
3949 Sawyer Road 
SARASOTA, FL. 33583 (USA)

------------------------------------------------------

The MFP in the Atari ST has 3 main functions:
- 16 interrupts controlled by several pairs of 8bit registers
- 4 timers (A-D), 2 with more options (A&B)
- RS232 (=USART) function

It is controlled via 24 8bit registers mapped from $fffa01 to $fffa2f


MK68901 MFP (Multi-Function Peripheral)
---------------------------------------

The MK68901 MFP (Multi-Function Peripheral) is a combination of many of the
necessary peripheral functions in a microprocessor system. Included are:

        Eight parallel I/O lines
        Interrupt controller for 16 sources
        Four timers
        Single channel full duplex USART

The use of the MFP in a system can significantly reduce chip count, thereby
reducing system cost. The MFP is completely 68000 bus compatible, and 24
directly addressable internal registers provide the necessary control and
status interface to the programmer.

The MFP is a derivative of the MK3801, a Z80 family peripheral.
*/

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_TIMER_B_NO_WOBBLE)
#define TB_TIME_WOBBLE (0) // no wobble for Timer B
#else
#define TB_TIME_WOBBLE (rand() & 4)
#endif

extern "C" void ASMCALL check_for_interrupts_pending();

EXT BYTE mfp_reg[24]; // 24 directly addressable internal registers, each 8bit

/*
Register MAP

Address   Abbreviation   Register Name
Port #
($)
0         GPIP           General purpose I/O
1         AER            Active edge register        Rising (1) of Falling (0)
2         DDR            Data direction register     Input (0) or Output (1)

Interrupt Control Registers
3         IERA           Interrupt enable register A
4         IERB           Interrupt enable register B
5         IPRA           Interrupt pending register A
6         IPRB           Interrupt pending register B
7         ISRA           Interrupt in-service register A
8         ISRB           Interrupt in-service register B
9         IMRA           Interrupt mask register A
A         IMRB           Interrupt mask register B
B         VR             Vector register

Timers
C         TACR           Timer A control register
D         TBCR           Timer B control register
E         TCDCR          Timers C and D control registers
F         TADR           Timer A data register
10        TBDR           Timer B data register
11        TCDR           Timer C data register
12        TDDR           Timer D data register

RS232?
13        SCR            Sync character register
14        UCR            USART control register
15        RSR            Receiver status register
16        TSR            Transmitter status register
17        UDR            USART data register

IO mapping

          ff fa01                   |xxxxxxxx|   MFP General Purpose I/O
          ff fa03                   |xxxxxxxx|   MFP Active Edge
          ff fa05                   |xxxxxxxx|   MFP Data Direction
          ff fa07                   |xxxxxxxx|   MFP Interrupt Enable A
          ff fa09                   |xxxxxxxx|   MFP Interrupt Enable B
          ff fa0b                   |xxxxxxxx|   MFP Interrupt Pending A
          ff fa0d                   |xxxxxxxx|   MFP Interrupt Pending B
          ff fa0f                   |xxxxxxxx|   MFP Interrupt In-Service A
          ff fa11                   |xxxxxxxx|   MFP Interrupt In-Service B
          ff fa13                   |xxxxxxxx|   MFP Interrupt Mask A
          ff fa15                   |xxxxxxxx|   MFP Interrupt Mask B
          ff fa17                   |xxxxxxxx|   MFP Vector
          ff fa19                   |xxxxxxxx|   MFP Timer A Control
          ff fa1b                   |xxxxxxxx|   MFP Timer B Control
          ff fa1d                   |xxxxxxxx|   MFP Timers C and D Control
          ff fa1f                   |xxxxxxxx|   MFP Timer A Data
          ff fa21                   |xxxxxxxx|   MFP Timer B Data
          ff fa23                   |xxxxxxxx|   MFP Timer C Data
          ff fa25                   |xxxxxxxx|   MFP Timer D Data
          ff fa27                   |xxxxxxxx|   MFP Sync Character
          ff fa29                   |xxxxxxxx|   MFP USART Control
          ff fa2b                   |xxxxxxxx|   MFP Receiver Status
          ff fa2d                   |xxxxxxxx|   MFP Transmitter Status
          ff fa2f                   |xxxxxxxx|   MFP USART Data
*/


#define MFPR_GPIP 0 // ff fa01 MFP General Purpose I/O
/*
The General Purpose I/O-Interrupt Port (GPIP) provides eight I/O lines that
may be operated either as inputs or outputs under software control. In
addition, each line may generate an interrupt on either a positive going edge
or a negative going edge of the input signal.
*/
#define MFPR_AER 1 // ff fa03 MFP Active Edge
/*
The Active Edge Register (AER) allows each of the General Purpose Interrupts
to produce an interrupt on either a 1-0 transition or a 0-1 transition.
Writing a zero to the appropriate bit of the AER causes the associated input
to produce an interrupt on the 1-0 transition, while a 1 causes the interrupt
on the 0-1 transition. The edge bit is simply one input to an exclusive-or
gate, with the other input coming from the input buffer and the output going
to a 1-0 transition detector. Thus, depending upon the state of the input,
writing the AER can cause an interrupt-producing transition, which will cause
an interrupt on the associated channel, if that channnel is enabled. One
would than normally configure the AER before enabling interrupts via IERA an
IERB. Note: changing the edge bit, with the interrupt enabled, may cause an
interrupt on that channel.
*/
#define MFPR_DDR 2 // ff fa05 MFP Data Direction
/*
The Data Direction Register (DDR) is used to define I0-I7 as input or as
outputs on a bit by bit basis. Writing a zero into a bit of the DDR causes
the corresponding Interrupt I/O pin to be a Hi-Z Input. Writing a one into a
bit of the DDR causes the corresponding pin to be configured as a push-pull
output. When data is written into the GPIP, those pins defined as inputs will
remain in the Hi-Z state while those pins defines as outputs will assume the
state (high or low) of their corresponding bit in the PIP. When the GPIP is
read, the data read will come directly from the corresponding bit of the GPIP
register for all pins defines as output, while the data read on all pins
defined as inputs will come from the input buffers.
*/

/*
Interrupts

A conceptual circuit of an interrupt:

       Edge       Enable                     Mask
    Register    Register                   Register              S-Bit
       |           |                          |                    |
       |           |                          ----|---\            --|---\ |------------|
       |           o-----|---\ |-----------|      |    |---o---------|    ||S Interrupt |
       ---\\--\    |     |    ||S Pending Q|------|---/    |       --|---/ |   Service  |
          ||   |---|-----|---/ |     R     |               |       |       |------------|
 I7-------//--/    |           |-----------|               |       |
                   |                 |                 Interrupt   |
                 -----             /---\                Request    |
                 \   /             |   |                           |
                  \ /              /---\                           |
                   o                | |                            |
                   |                | |                            |
                   ------------------ -----------------------------o------ Pass Vector




The Interrupt Control Registers provide control of interrupt processing for
all I/O facilities of the MK68901. These registers allow the programmer to
enable or disable any or all of the 16 interrupts, providing masking for any
interrupts, and provide access to the pending and in-service status of the
interrupts. Optional end-of-interrupt modes are availble under software
control.

To acknowledge an interrupt, IACK goes low, the IEI input must go low (or be
tied low) and the MK68901 MFP must have acknowledgeable interrupt pending.
The daisy chaining capability requires that all parts in a chain have a
command IACK. When the command IACK goes low all parts freeze and prioritize
interrupts in parallel. Then priority is passed down the chain, via IEI and
IEO, until a part which has a pending interrupt is reached. The part with
the pending interrupt, passes a vector, does not propagate IEO, and generates
DTACK.

Interrupt Control Register Definitions

Priority   Channel   Description
Highest     1111      General Purpose Interrupt 7(I7)
            1110      General Purpose Interrupt 6(I6)
            1101      Timer A
            1100      Receive Buffer Full
            1011      Receive Error
            1010      Transmit Buffer Empty
            1001      Transmit Error
            1000      Timer B
            0111      General Purpose Interrupt 5(I5)
            0110      General Purpose Interrupt 4(I4)
            0101      Timer C
            0100      Timer D
            0011      General Purpose Interrupt 3(I3)
            0010      General Purpose Interrupt 2(I2)
            0001      General Purpose Interrupt 1(I1)
Lowest      0000      General Purpose Interrupt 0(I0)
*/

#define MFPR_IERA 3 // ff fa07 MFP Interrupt Enable A
#define MFPR_IERB 4 // ff fa09 MFP Interrupt Enable B
/*
                    Interrupt Enable Registers
Port 3 (IERA) GPIP  GPIP TIMER  RCV   RCV  XMIT   XMIT TIMER
               7     6     A    Full  Err  Empty  Err    B

Port 4 (IERB) GPIP  GPIP TIMER TIMER  GPIP  GPIP  GPIP  GPIP
               5     4     C     D     3     2     1     0

                                       Each channel may be individually
enabled or disabled by writing a one or a zero in the appropriate bit of the
Interrupt Enable Registers (IERA,IERB). When disabled, an interrupt channel
is completely inactive. Any internal or external action which would normally
produce an interrupt on that channel is ignored and any pending interrupt on
that channel will be cleared by disabling that channel.
 IERA and IERB are also readable.
*/

#define MFPR_IPRA 5 // ff fa0b MFP Interrupt Pending A
#define MFPR_IPRB 6 // ff fa0d MFP Interrupt Pending B
/*
                    Interrupt Pending Registers
Port 5 (IPRA) GPIP  GPIP TIMER  RCV   RCV  XMIT   XMIT TIMER
               7     6     A    Full  Err  Empty  Err    B
Port 6 (IPRB) GPIP  GPIP TIMER TIMER  GPIP  GPIP  GPIP  GPIP
               5     4     C     D     3     2     1     0
                         Writing 0: Clear
                         Writing 1: Unchanged

Interrupts may be either polled or vectored.  
When an interrupt is received on an enabled channel, its corresponding bit in
the pending register will be set. When that channel is acknowledged it will
pass its vector, and the corresponding bit in the Interrupt Pending Register
(IPRA or IPRB) will be cleared. IPRA and IPRB are readable; thus by polling
IPRA and IPRB, it can be determind whether a channel has a pending interrupt.
IPRA and IPRB are also writeable and a pending interrupt can be cleared
without going through the acknowledge sequence by writing a zero to the
appropriate bit. This allows any one bit to be cleared, without altering any
other bits, simply by writing all ones except for the bit position to be
cleared on IPRA or IPRB. Thus a full polled interrupt scheme is possible.
Note: writing a one to IPRA, IPRB has no effect on the interrupt pending
register.
*/

#define MFPR_ISRA 7 // ff fa0f MFP Interrupt In-Service A
#define MFPR_ISRB 8 // ff fa11 MFP Interrupt In-Service B
/*
                    Interrupt In-Service Registers
Port 7 (ISRA) GPIP  GPIP TIMER  RCV   RCV  XMIT   XMIT TIMER
               7     6     A    Full  Err  Empty  Err    B
Port 8 (ISRB) GPIP  GPIP TIMER TIMER  GPIP  GPIP  GPIP  GPIP
               5     4     C     D     3     2     1     0

                                                    Disabling and
interrupt channel has no effect on the corresponding bit in Interrupt
in-Service Registers (ISRA,ISRB); thus, if the In-Service Registers are used
and an interrupt is in service on that channel when the channel is disabled,
it will remain in service until cleared in the normal manner.

*/

#define MFPR_IMRA 9 // ff fa13 MFP Interrupt Mask A
#define MFPR_IMRB 10 // ff fa15 MFP Interrupt Mask B
/*

                    Interrupt Mask Registers
Port 9 (IMRA) GPIP  GPIP TIMER  RCV   RCV  XMIT   XMIT TIMER
               7     6     A    Full  Err  Empty  Err    B
Port A (IMRB) GPIP  GPIP TIMER TIMER  GPIP  GPIP  GPIP  GPIP
               5     4     C     D     3     2     1     0
                    1: UnMasked    0: Masked


The interrupt mask registers (IMRA and IMRB) may be used to block a channel
from making an interrupt request. Writing a zero into the corresponding bit
of the mask register will still allow the channel to receive and interrupt
and latch it into its pending bit (if that channel is enabled), but will
prevent that channel from making an interrupt request. If that channel is
causing an interrupt request at the time the corresponding bit in the mask
register is cleared, the request will cease. If no other channel is making a
request, INTR will go inactive. If the mask bit is re-enabled, any pending
interrupt is now free to resume its request unless blocked by a higher
priority request for service. IMRA and iMRB are also readable.
*/

#define MFPR_VR 11 // ff fa17 MFP Vector
/*
Each individual functions in the MK68901 is provided with a unique interrupt
vector that is presented to the system during the interrupt acknowledge
cycle. The interrupt vector returned during the interrrupt acknowledge cycle
is shown below.

Interrupt Vector
         V7   V6   V5   V4   V3   V2   V1   V0
         \-----------------/\----------------/
                  |                  |
                  |                  ------------ Vector bits 3-0 supplied
                  |                               by the MFP based upon the interrupting
                  |                               channel.
                  |
                  ------------------------------- 4 most significant bits. Copied
                                                  from the vector register.
Vector Register
         V7   V6   V5   V4    S    *    *    *
         \-----------------/  |
                  |           |
                  |           ------------------- S In-Service Register Enable
                  |
                  ------------------------------- Upper 4 bits of the Vector Register
                                                  Written into by the user.

There are 16 vector addresses generated internally by the MK68901, one for
each of the 16 interrupt channels.

There are two end-of-interrupt modes: the automatic end-of-interrupt mode and
the software end-of-interrupt mode. The mode is selected by writing a one or
a zero to the S bit of the Vector Register (VR). If the S bit of the VR is a
one, all channels operate in the software end-of-interrupt mode. If the S bit
is a zero, all channels operate in the automatic end-of-interrupt mode, and a
reset is held on all in-service bits. In the automatic end-of-interrupt mode,
the pending bit is cleared when that channel passes its vector. At that
point, no further history of that interrupt remains in the MK68901 MFP. In
the software end-of-interrupt mode, the in-service bit is set and the pending
bit is cleared when the channel passes its vector. With the in-service bit
set, no lower priority channel is allowed to request an interrupt or to pass
its vector during an acknowledge sequence, however, a lower priority channel
may still receive and interrupt and latch it into the pending bit. A higher
priority channel may still request an interrupt and be acknowledged, The in-
service bit of a particular channel may be cleared by writing a zero to the 
corresponding bit in ISRA or ISRB. 
Typically, this will be done at the conclusion of the interrupt routine
just before the return. Thus no lower priority channel will be allowed to
request service until the higer priority channel is complete, while channels
of still higher priority will be allowed to request service. While the
in-service bit is set, a second interrupt on that channel may be received and
latched into the pending bit, though no service request will be made in
response to the second interrupt until the in-service bit is cleared. ISRA
and ISRB may be read at any time. Only a zero may be written into any bit of
ISRA and ISRB; thus the in-service may be cleared in software but cannot be
set in software. This allows any one bit to be cleared, without altering any
other bits, simply by writing all ones except for the bit position to be
cleared to ISRA or ISRB, as with IPRA and IPRB.

Each interrupt channel responds with a discrete 8-bit vector when
acknowledged. The upper four bits of the vector are set by writing the upper
four bits of the VR. The four low order bits (bit 3-bit 0) are generated by
the interrupt channel.
*/

/*

Timers

There are four timers on the MK68901 MFP. Two of the timers (Timer A and
Timer B) are full function timers which can perform the basic delay function
and can also perform event counting, pulse width measurement and waveform
generation. The other two timers (Timer C and Timer D) are delay timers only.
One or both of these timers can be used to supply the baud rate clocks for
the USART. All timers are prescaler/counter timers with a common independent
clock input (XTAL1, XTAL2). In addition, all timers have a time-out output
function that toggles each time the timer times out.



With the timer stopped, no counting can occur. The timer contents will remain
unaltered while the timer is stopped (unless reloaded by writing the Timer
Data Register), but any residual count in the prescaler will be lost.

In the delay mode, the prescaler is always active. A count pulse will be
applied to the main timer unit each time the prescribed number of timer clock
cycles has elapsed. Thus, if the prescaler is programmed to divide by ten, a
count pulse will be applied to the main counter every ten cycles of the timer
clock.

Each time a count pulse is applied to the main counter, it will decrement
its contents. The main counter is initially loaded by writing to the Timer
Data Register. Each count pulse will cause the current count to decrement.
When the timer has decremented down to '01', the next count pulse will not
cause it to decrement to '00'. Instead the next count pulse will cause the
timer to be reloaded from the Timer Data Register. Additionally, a 'Time Out'
pulse will be produced. This Time Out pulse is coupled to the timer interrupt
channel, and, if that channel is enabled an interrupt will be produced. The
Time Out pulse is also coupled to the timer output pin and will cause the pin
to change states. The output will remain in this new state until the next
Time Out pulse occurs. Thus the output will complete one full cycle each two
Time Out pulses.

If, for example, the prescaler were programmed to divide by ten, and the
Timer Data Register were loaded will 100(decimal), the main counter would
decrement once for every ten cycles of the timer clock. A Time Out pulse will
occur(hence an interrupt if that channel is enabled) every 1000 cycles of the
timer clock, and the timer output will complete one full cycle every 2000
cycles of the timer clock.

The main counter is an 8-bit binary down counter. If may be read at any time
by reading the Timer Data Register. The information read is the information
last clocked into the timer read register when the DS pin had last gone high
prior to the current read cycle. When written, data is loaded into the Timer
Data Register, and the main counter, if the timer is stopped. If the Timer
Data Register is written while the timer is running, the new word is not
loaded into the timer until it counts through H01. However, if the timer is
written while it is counting through H01, an indeterminate value will be
written into the time constant register. This may be circumvented by ensuring
that the daata register is not written when the count is H01.

If the main counter is loaded with 01, a Time Out Pulse will occur every
time the prescaler presents a count pulse to the main counter. If loaded
with 00, a Time Out pulse will occur after every 256 count pulses.

Changing the prescale value with the timer running can cause the first Time
Out pulse to occur at an indeterminate time, (no less than one nor more than
200 timer clock cycles times the number in the time constant register), but
subsequent Time Out pulses will then occur at the correct interval.

In addition to the delay mode described above, Timers A and B can also
function in Pulse Width Measurement mode or in the Event Count mode. In
either of these two modes, an auxilary count signal is required. The auxilary
control input for Timer A is TAI, and for Timer B, TBI is used. The interrupt
channels associated with I4 and I3 are used for TAI and TBI, respectively, in
Pulse Width mode.

The pulse width measurement mode functions much like the delay mode. However,
in this mode, the auxiliary control signal on TAI or TBI acts as an enable to
the timer. When the control signal on TAI or TBI is inactive, the timer will
be stopped. When it is active, the prescaler and main counter are allowed to
run. Thus the width of the active pulse on TAI or TBI is determined by the
number of timer counts which occur while the pulse allows the timer to run.
The active state of the signal on TAI or TBI is dependent upon the associated
Interrupt Channels edge bit (GPIP4 for TAI and GPIP3 for TBI, see Active Edge
Register). If the edge bit associated with the TAI or TBI input is a one, it
will be active high, thus the timer will be allowed to run when the input is
at a high level. If the edge bit is a zero, the TAI or TBI input will be
active low. As previously stated, the interrupt channel (I3 or I4) associated
with the input still functions when the timer is used in the pulse width
measurement mode. However, if the timer is programmed for the pulse width
measurement mode, the interrupt caused by transitions on the associated TAI
or TBI input will occur on the opposite transition.

A conceptual circuit of the MFP timer in the pulse width measurement mode

                            |-----|
                            | TAI |
                            |-----|
                               |
                               -----------|---\
  Timer A                                 |    |----
  Pulse Width Mode ------------o----------|---/    |
                               |                   |---\---\      |-----------|
                               |                        |   |-----| Interrupt |
           |----|              |   |\              |---/---/      |  Channel  |
           | I4 |-------|      ----| o----|---\    |              |-----------|
           |----|       |          |/     |    |----
                        ------------------|---/


                        ------------------|---\
           |----|       |          |\     |    |----
           | I3 |-------|      ----| o----|---/    |
           |----|              |   |/              |---\---\      |-----------|
                               |                        |   |-----| Interrupt |
  Timer B                      |                   |---/---/      |  Channel  |
  Pulse Width Mode ------------o----------|---\    |              |-----------|
                                          |    |----
                               -----------|---/    
                               |
                            |-----|
                            | TBI |
                            |-----|




For example, if the edge bit associated with the TAI input (AER-GPIP 4) is a
one, an interrupt would normally be generated on the 0-1 transition of the I4
input signal. If the timer associated with this input (Timer A) is placed in
the pulse width measurement mode, the interrupt will occur on the 1-0
transition of the TAI signal instead. Because the edge bit (AER-GPIP4) is a
one, Timer A will be allowed to count while the input is high. When the TAI
input makes the high to low transition, Timer A will stop, and it is at this
point that the interrupt will occur (assuming that the channel is enabled).
This allows the interrupt to signal the CPU that the pulse being measured has
terminated this Timer A may now be read to determine the pulse width. (Again
note that I3 and I4 may still be used for I/O when the timer is in the pulse
width measurement mode). If Timer A is re-programmed for another mode,
interrupts will again occur on the transition, as normally defined by the
edge bit. Note that, like changing the edge bit, placing the timer into or
taking it out of pulse width mode can produce a transition on the signal to
the interrupt channel and may cause an interrupt. If measuring consecutive
pulses, it is obvious that one must read the contents of the timer and
reinitalize the main counter by writing to the timer data register. If the
timer data register is written while the pulse is going to the active state,
the write operation may result in an indeterminate value being written into
the main counter. If the timer is written after the pulse goes active, the
timer counts from the previous contents, and when it counts through H01, the
correct value is written into the timer. The pulse width then includes counts
from before the timer was reloaded.

In the event count mode, the prescaler is disabled. Each time the control
input on TAI or TBI makes an active transition as defined by the associated
Interrupt Channel's edge bit, a count pulse will be generated, and the main
counter will decrement. In all other respects, the timer functions as
previously described. Altering the edge bit while the timer is in the event
count mode can produce a count pulse. The interrupt channel associated with
the input (I3 for TBI or I4 for TAI) is allowed to function normally. To
count transitions reliably, the input must remain in each state (1/0) for a
length of time equal to four perioids of the timer clock; thus signals of a
frequency up to one fourth of the timer clock can be counted.

The manner in which the timer output pins toggle states has previously been
described. All timer outputs will be forced low by a device RESET. The output
associated with Timers A and B will toggle on each Time Out pulse regardless
of the mode the timers are programmed to. In addition, the outputs from
Timers A and Timers B can be forced low at any time by writing a 1 to the
reset location in TACR and TBCR, respectively. The output will be forced to
the low state during the WRITE operation, and at the conclusion of the
operation, the output will again be free to toggle each time a Time Out pulse
occurs. This feature will allow waveform generation.

During reset, the Timer Data Registers and the main counters are not reset.
Also, if using the reset option on Timers A or B, one must make sure to keep
the other bits in the correct state so as not to affect the operation of
Timers A and B. 
*/


#define MFPR_TACR 12 // ff fa19 MFP Timer A Control
#define MFPR_TBCR 13 // ff fa1b MFP Timer B Control
/*
Timer A and B Control Register

     Port C (TACR)    *    *    *  Reset AC3  AC2  AC1  AC0

     Port D (TBCR)    *    *    *  Reset BC3  BC2  BC1  BC0

              C3  C2  C1  C0
               0   0   0   0   Timer Stopped
               0   0   0   1   Delay Mode, /4 Prescale
               0   0   1   0   Delay Mode, /10 Prescale
               0   0   1   1   Delay Mode, /16 Prescale
               0   1   0   0   Delay Mode, /50 Prescale
               0   1   0   1   Delay Mode, /64 Prescale
               0   1   1   0   Delay Mode, /100 Prescale
               0   1   1   1   Delay Mode, /200 Prescale
               1   0   0   0   8  Event Count Mode
               1   0   0   1   Pulse Width Mode, /4 Prescale
               1   0   1   0   Pulse Width Mode, /10 Prescale
               1   0   1   1   Pulse Width Mode, /16 Prescale
               1   1   0   0   Pulse Width Mode, /50 Prescale
               1   1   0   1   Pulse Width Mode, /64 Prescale
               1   1   1   0   Pulse Width Mode, /100 Prescale
               1   1   1   1   Pulse Width Mode, /200 Prescale
*/

#define MFPR_TCDCR 14 // ff fa1d MFP Timers C and D Control
/*
Timer C and D Control Register

     Port E (TCDCR)   *   CC2  CC1  CC0   *   DC2  DC1  DC0

                  C2  C1  C0
                   0   0   0   Timer Stopped
                   0   0   1   Delay Mode, /4 Prescale
                   0   1   0   Delay Mode, /10 Prescale
                   0   1   1   Delay Mode, /16 Prescale
                   1   0   0   Delay Mode, /50 Prescale
                   1   0   1   Delay Mode, /64 Prescale
                   1   1   0   Delay Mode, /100 Prescale
                   1   1   1   Delay Mode, /200 Prescale
*/

#define MFPR_TADR 15 // ff fa1f  MFP Timer A Data
#define MFPR_TBDR 16 // ff fa21  MFP Timer B Data
#define MFPR_TCDR 17 // ff fa23  MFP Timer C Data
#define MFPR_TDDR 18 // ff fa25  MFP Timer D Data
/*
Timer Data Registers (A,B,C and D)

     Port F (TADR)    D7   D6   D5   D4   D3   D2   D1   D0

     Port 10 (TBDR)   D7   D6   D5   D4   D3   D2   D1   D0

     Port 11 (TCDR)   D7   D6   D5   D4   D3   D2   D1   D0

     Port 12 (TDDR)   D7   D6   D5   D4   D3   D2   D1   D0

*/

// RS232
#define MFPR_SCR 19 // ff fa27 MFP Sync Character
#define MFPR_UCR 20 // ff fa29 MFP USART Control
#define MFPR_RSR 21 // ff fa2b MFP Receiver Status
#define MFPR_TSR 22 // ff fa2d MFP Transmitter Status
#define MFPR_UDR 23 // ff fa2f MFP USART Data

EXT BYTE mfp_gpip_no_interrupt INIT(0xf7);


/*
          ----- MK68901 Interrupt Control ---------------

                   --------------- --------------------------------
                  | Priority      | Definition                     |
                   --------------- --------------------------------
                  | 15 (HIGHEST)  | Monochrome Monitor Detect    I7|
                  | 14            | RS232 Ring Indicator         I6|
                  | 13            | System Clock / BUSY          TA|
                  | 12            | RS232 Receive Buffer Full      |
                  | 11            | RS232 Receive Error            |
                  | 10            | RS232 Transmit Buffer Empty    |
                  |  9            | RS232 Transmit Error           |
                  |  8            | Horizontal Blanking Counter  TB|
                  |  7            | Disk Drive Controller        I5|
                  |  6            | Keyboard and MIDI            I4|
                  |  5            | Timer C                      TC|
                  |  4            | RS232 Baud Rate Generator    TD|
                  |  3            | GPU Operation Done           I3|
                  |  2            | RS232 Clear To Send          I2|
                  |  1            | RS232 Data Carrier Detect    I1|
                  |  0 (LOWEST)   | Centronics BUSY              I0|
                   --------------- --------------------------------

          NOTE:  the MC6850 ACIA Interrupt Request status bit must be tested
                 to differentiate between keyboard and MIDI interrupts.
*/

#define MFP_INT_MONOCHROME_MONITOR_DETECT 15
#define MFP_INT_RS232_RING_INDICATOR 14
#define MFP_INT_TIMER_A 13
#define MFP_INT_RS232_RECEIVE_BUFFER_FULL 12
#define MFP_INT_RS232_RECEIVE_ERROR 11
#define MFP_INT_RS232_TRANSMIT_BUFFER_EMPTY 10
#define MFP_INT_RS232_TRANSMIT_ERROR 9
#define MFP_INT_TIMER_B 8
#define MFP_INT_FDC_AND_DMA 7
#define MFP_INT_ACIA 6  // Vector at $118
#define MFP_INT_TIMER_C 5
#define MFP_INT_TIMER_D 4
#define MFP_INT_BLITTER 3
#define MFP_INT_RS232_CTS 2
#define MFP_INT_RS232_DCD 1
#define MFP_INT_CENTRONICS_BUSY 0

#define MFP_GPIP_COLOUR BYTE(0x80)
#define MFP_GPIP_NOT_COLOUR BYTE(~0x80)
#define MFP_GPIP_CTS BYTE(BIT_2)
#define MFP_GPIP_DCD BYTE(BIT_1)
#define MFP_GPIP_RING BYTE(BIT_6)

#define MFP_GPIP_CENTRONICS_BIT 0
#define MFP_GPIP_DCD_BIT 1
#define MFP_GPIP_CTS_BIT 2
#define MFP_GPIP_BLITTER_BIT 3
#define MFP_GPIP_ACIA_BIT 4
#define MFP_GPIP_FDC_BIT 5
#define MFP_GPIP_RING_BIT 6
#define MFP_GPIP_MONO_BIT 7

#ifdef IN_EMU

BYTE mfp_gpip_input_buffer=0;

#define MFP_CLK 2451

#define MFP_CLK_EXACT 2451134 // Between 2451168 and 2451226 cycles

// Number of MFP clock ticks per 8000000 CPU cycles, very accurately tested!
// This is the most accurate number but we use the one above because Lethal Xcess
// won't work with this one.
//#define MFP_CLK_EXACT 2451182 // Between 2451168 and 2451226 cycles

//#define MFP_CLK_EXACT 2451034 // Between 2450992 and 2451050 (erring high) old version before 12 cycle delay

//#define MFP_CLK_EXACT 2450780  old version, checked inaccurately

#if defined(STEVEN_SEAGAL) && defined(SSE_INT_MFP_RATIO)
// it's a variable now! See SSE.h !!!!!!!!!!!!
#else
#define CPU_CYCLES_PER_MFP_CLK (8000000.0/double(MFP_CLK_EXACT))
#endif

#define CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED 20
//#define CYCLES_FROM_START_OF_MFP_IRQ_TO_WHEN_PEND_IS_CLEARED 20
#define MFP_S_BIT (mfp_reg[MFPR_VR] & BIT_3)

//const int mfp_io_port_bit[16]={BIT_0,BIT_1,BIT_2,BIT_3,-1,-1,BIT_4,BIT_5,-1,-1,-1,-1,-1,-1,BIT_6,BIT_7};




inline BYTE mfp_get_timer_control_register(int);
const int mfp_timer_8mhz_prescale[16]={65535,4,10,16,50,64,100,200,65535,4,10,16,50,64,100,200};

const int mfp_timer_irq[4]={13,8,5,4};
/*
                   --------------- --------------------------------
                  | 13            | System Clock / BUSY          TA|
                  |  8            | Horizontal Blanking Counter  TB|
                  |  5            | Timer C                      TC|
                  |  4            | RS232 Baud Rate Generator    TD|
                   --------------- --------------------------------
*/

void calc_time_of_next_timer_b();

int mfp_timer_prescale[16]={65535,4,10,16,50,64,100,200,65535,4,10,16,50,64,100,200};

int mfp_timer_counter[4];
int mfp_timer_timeout[4];
bool mfp_timer_enabled[4]={0,0,0,0};
int mfp_timer_period[4]={10000,10000,10000,10000};
#if defined(SSE_INT_MFP_RATIO_PRECISION)
int mfp_timer_period_fraction[4]={0,0,0,0};
int mfp_timer_period_current_fraction[4]={0,0,0,0};
#endif
bool mfp_timer_period_change[4]={0,0,0,0};
//int mfp_timer_prescale_counter[4]={0,0,0,0};

void mfp_set_timer_reg(int,BYTE,BYTE);
int mfp_calc_timer_counter(int);
void mfp_init_timers();
inline bool mfp_set_pending(int,int);

void mfp_gpip_set_bit(int,bool);

const int mfp_gpip_irq[8]={0,1,2,3,6,7,14,15};

//int mfp_gpip_timeout;

bool mfp_interrupt_enabled[16];
int mfp_time_of_start_of_last_interrupt[16];

int cpu_time_of_first_mfp_tick;


#define MFP_CALC_INTERRUPTS_ENABLED	\
{	\
  int mask=1;	\
  for (int n=0;n<8;n++){	\
    mfp_interrupt_enabled[n]=mfp_reg[MFPR_IERB] & mask; mask<<=1;	\
  }	\
  mask=1;	\
  for (int n=0;n<8;n++){	\
    mfp_interrupt_enabled[n+8]=mfp_reg[MFPR_IERA] & mask; mask<<=1;	\
  }	\
}

#define MFP_CALC_TIMERS_ENABLED	\
	for (int n=0;n<4;n++){	\
		mfp_timer_enabled[n]=mfp_interrupt_enabled[mfp_timer_irq[n]] && (mfp_get_timer_control_register(n) & 7);	\
	}

//int mfp_timer_tick_countdown[4];
void mfp_interrupt(int,int);
//bool mfp_interrupt_enabled(int irq);
#if !defined(STEVEN_SEAGAL) // this function isn't used
void mfp_gpip_transition(int,bool);
void mfp_check_for_timer_timeouts(); // SS not implemented
#endif
#define MFP_CALC_TIMER_PERIOD(t)  mfp_timer_period[t]=int(  \
          double(mfp_timer_prescale[mfp_get_timer_control_register(t)]* \
            int(BYTE_00_TO_256(mfp_reg[MFPR_TADR+t])))*CPU_CYCLES_PER_MFP_CLK);

#define mfp_interrupt_i_bit(irq) (BYTE(1 << (irq & 7)))
#define mfp_interrupt_i_ab(irq) (1-((irq & 8) >> 3))

//TODO check the condition
#define mfp_interrupt_pend(irq,when_fired)                                       \
  if (mfp_interrupt_enabled[irq]){                             \
    LOG_ONLY( bool done= ) mfp_set_pending(irq,when_fired);   \
    LOG_ONLY( if (done==0) log_to(LOGSECTION_MFP_TIMERS,EasyStr("INTERRUPT: MFP IRQ #")+irq+" ("+    \
                                (char*)name_of_mfp_interrupt[irq]+") - can't set pending as MFP cleared "  \
                                "pending after timeout"); )             \
  } 

#endif

#undef EXT
#undef INIT

#endif//SSE_STRUCTURE_MFP_H