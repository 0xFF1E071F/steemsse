#pragma once
#ifndef BIG_FORWARD_H
#define BIG_FORWARD_H

#include "SSE/SSE.h" // get switches

#if defined(SS_STRUCTURE_BIG_FORWARD)

// forward (due to shitty structure)
#if defined(SS_VAR_REWRITE)
extern "C" void ASMCALL m68k_trace();
#else
extern "C" ASMCALL void m68k_trace();
#endif
extern void (*m68k_high_nibble_jump_table[16])();
//void HandleIOAccess();
void ASMCALL perform_crash_and_burn();
#if defined(SS_CPU_DIV)
extern "C" unsigned getDivu68kCycles( unsigned long dividend, unsigned short divisor);
extern "C" unsigned getDivs68kCycles( signed long dividend, signed short divisor);
#endif
#if defined(SS_CPU_PREFETCH_TIMING_EXCEPT)
bool debug_prefetch_timing(WORD ir);
#endif
// forward

extern void m68k_get_source_000_b();
extern void m68k_get_source_000_w();
extern void m68k_get_source_000_l();
extern void m68k_get_source_001_b();
extern void m68k_get_source_001_w();
extern void m68k_get_source_001_l();
extern void m68k_get_source_010_b();
extern void m68k_get_source_010_w();
extern void m68k_get_source_010_l();
extern void m68k_get_source_011_b();
extern void m68k_get_source_011_w();
extern void m68k_get_source_011_l();
extern void m68k_get_source_100_b();
extern void m68k_get_source_100_w();
extern void m68k_get_source_100_l();
extern void m68k_get_source_101_b();
extern void m68k_get_source_101_w();
extern void m68k_get_source_101_l();
extern void m68k_get_source_110_b();
extern void m68k_get_source_110_w();
extern void m68k_get_source_110_l();
extern void m68k_get_source_111_b();
extern void m68k_get_source_111_w();
extern void m68k_get_source_111_l();

extern void m68k_get_dest_000_b();
extern void m68k_get_dest_000_w();
extern void m68k_get_dest_000_l();
extern void m68k_get_dest_001_b();
extern void m68k_get_dest_001_w();
extern void m68k_get_dest_001_l();
extern void m68k_get_dest_010_b();
extern void m68k_get_dest_010_w();
extern void m68k_get_dest_010_l();
extern void m68k_get_dest_011_b();
extern void m68k_get_dest_011_w();
extern void m68k_get_dest_011_l();
extern void m68k_get_dest_100_b();
extern void m68k_get_dest_100_w();
extern void m68k_get_dest_100_l();
extern void m68k_get_dest_101_b();
extern void m68k_get_dest_101_w();
extern void m68k_get_dest_101_l();
extern void m68k_get_dest_110_b();
extern void m68k_get_dest_110_w();
extern void m68k_get_dest_110_l();
extern void m68k_get_dest_111_b();
extern void m68k_get_dest_111_w();
extern void m68k_get_dest_111_l();
extern void m68k_get_dest_000_b_faster();
extern void m68k_get_dest_000_w_faster();
extern void m68k_get_dest_000_l_faster();
extern void m68k_get_dest_001_b_faster();
extern void m68k_get_dest_001_w_faster();
extern void m68k_get_dest_001_l_faster();
extern void m68k_get_dest_010_b_faster();
extern void m68k_get_dest_010_w_faster();
extern void m68k_get_dest_010_l_faster();
extern void m68k_get_dest_011_b_faster();
extern void m68k_get_dest_011_w_faster();
extern void m68k_get_dest_011_l_faster();
extern void m68k_get_dest_100_b_faster();
extern void m68k_get_dest_100_w_faster();
extern void m68k_get_dest_100_l_faster();
extern void m68k_get_dest_101_b_faster();
extern void m68k_get_dest_101_w_faster();
extern void m68k_get_dest_101_l_faster();
extern void m68k_get_dest_110_b_faster();
extern void m68k_get_dest_110_w_faster();
extern void m68k_get_dest_110_l_faster();
extern void m68k_get_dest_111_b_faster();
extern void m68k_get_dest_111_w_faster();
extern void m68k_get_dest_111_l_faster();

extern void (*m68k_jump_get_dest_b[8])();
extern void (*m68k_jump_get_dest_w[8])();
extern void (*m68k_jump_get_dest_l[8])();
extern void (*m68k_jump_get_dest_b_not_a[8])();
extern void (*m68k_jump_get_dest_w_not_a[8])();
extern void (*m68k_jump_get_dest_l_not_a[8])();
extern void (*m68k_jump_get_dest_b_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_w_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_l_not_a_or_d[8])();
extern void (*m68k_jump_get_dest_b_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_dest_w_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_dest_l_not_a_faster_for_d[8])();
extern void (*m68k_jump_get_source_b[8])();
extern void (*m68k_jump_get_source_w[8])();
extern void (*m68k_jump_get_source_l[8])();
extern void (*m68k_jump_get_source_b_not_a[8])();
extern void (*m68k_jump_get_source_w_not_a[8])();
extern void (*m68k_jump_get_source_l_not_a[8])();



extern int shifter_freq_at_start_of_vbl;

extern int draw_last_scanline_for_border,res_vertical_scale;




extern BYTE fdc_sr;
extern BYTE floppy_head_track[2];




extern int stemdos_current_drive;
#if defined(SS_SOUND_MICROWIRE)
extern int dma_sound_bass,dma_sound_treble;
#endif


void SetNotifyInitText(char*);//forward



int SwitchSTType(int new_type);//forward

#endif//defined(SS_STRUCTURE_BIG_FORWARD)
#endif//BIG_FORWARD_H