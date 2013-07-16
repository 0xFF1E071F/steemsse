#pragma once
#ifndef DWINEDIT_DECLA_H
#define DWINEDIT_DECLA_H

/////////////////////////////// DWin_edit ////////////////////////////////////
extern void* DWin_edit_subject;
extern int DWin_edit_subject_type;
extern int DWin_edit_subject_index;
extern int DWin_edit_subject_col;
extern MEM_ADDRESS DWin_edit_subject_ad;
enum _DWin_edit_subject_content{DWESC_HEX=0,DWESC_TEXT,DWESC_BIN,DWESC_DISA};
extern _DWin_edit_subject_content DWin_edit_subject_content;

extern bool DWin_edit_is_being_temporarily_defocussed;

void set_DWin_edit(int type,void*subject,int n,int col);
void DWin_edit_finish(bool);

#endif//DWINEDIT_DECLA_H