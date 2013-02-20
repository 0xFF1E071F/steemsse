// 6301.h

#pragma once
#ifndef HD6301_H
#define HD6301_H

/*  We use Sim6xxx by Arne Riiber to emulate the HD6301V1 chip that was 
    in the ST, as described in 'doc_6301_ST'.
    This emulation was supposed to run as terminal on a DOS PC. A
    precise integration with another serial device (MC6850 ACIA in the
    ST) wasn't the focus.
    This also hadn't VC6 or BCC5 in mind, let alone VS2010 or Linux.
    The emu takes 64K just for the 8bit chip's ram (x2 if we keep the
    breaks). It must be possible to optimise this away (it only uses 256 bytes
    of RAM + 4K of ROM) but 64k is OK.
    The functions in original files have been modified when necessary, some
    have been added, in 6301.c, and in ireg.c (most interesting ones). 
    The module is built in 6301.c, which also contains specific functions.
    As of version 3.4, this emu runs at least:
    - GEM, Neochrome
    - Dragonnels, Froggies, Transbeauce 2, those are demos that reprogram the
    chip and this was the goal: mission completed
    - Barbarian, Manchester United, Jumping Jackson (WNW), Cobra compil 01
    Gauntlet (joy0 bug), Lethal Xcess, The Sentinel, Arkanoid, MGT
    All functions and variables beginning with hd6301 are declared in this
    6301 module and are meant to be visible to Steem (interface) by way of
    inclusion of this file.

    TODO: 
    - get real timings / Froggies must still be OK
    - mouse movement isn't satisfying yet, but it's complicated, SainT has
    problems too. It's probably related to timings.
    - check ram access range?
*/

#include "SSE/SSE.h"
#include "SSE/SSEParameters.h"
#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h> // printf...
#include <sys/types.h>

#if defined(SS_IKBD_6301)

// variables from Steem we declare here as 'C' linkage
extern int ST_Key_Down[128];
extern int mousek;
// variables that Steem must see
extern int hd6301_receiving_from_MC6850; // for sync
extern int hd6301_transmitting_to_MC6850; // for sync
extern int hd6301_completed_transmission_to_MC6850; // for sync
extern int hd6301_mouse_move_since_last_interrupt_x; // different lifetime
extern int hd6301_mouse_move_since_last_interrupt_y;
// functions used by Steem
int hd6301_init(); // just another name
int hd6301_destroy(); // like a C++ destructor
int hd6301_reset(); // just another name
int hd6301_run_cycles(u_int cycles); // emulate
int hd6301_load_save(int one_if_save, unsigned char *buffer); // for snaphot
int hd6301_transmit_byte(u_char byte_in); // just passing through

#define USE_PROTOTYPES 


// ensure compiling, TODO, find the BCC ID instead
#if !defined(_MSC_VER) 
#pragma warn- 8008
#pragma warn- 8019 // code has no effect
#pragma warn- 8045
#pragma warn- 8060
#pragma warn- 8061
#pragma warn- 8065
#pragma warn- 8066
#pragma warn- 8070
#pragma warn- 8071
#define __max(a,b) (((a) (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#else
#pragma warning( disable :  4716) // warning C4716: 'foo' : must return a value
#endif

#endif 

#endif // HD6301_H
