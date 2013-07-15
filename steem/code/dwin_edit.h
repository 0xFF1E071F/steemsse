#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_DWINEDIT_H)

#include "dwin_edit.decla.h"

#else//!defined(SS_STRUCTURE_DWINEDIT_H)


/////////////////////////////// DWin_edit ////////////////////////////////////
void* DWin_edit_subject;
int DWin_edit_subject_type;
int DWin_edit_subject_index;
int DWin_edit_subject_col;
MEM_ADDRESS DWin_edit_subject_ad;
enum _DWin_edit_subject_content{DWESC_HEX=0,DWESC_TEXT,DWESC_BIN,DWESC_DISA};
_DWin_edit_subject_content DWin_edit_subject_content;
bool DWin_edit_is_being_temporarily_defocussed=false;

void set_DWin_edit(int type,void*subject,int n,int col);
void DWin_edit_finish(bool);

#endif//SS_STRUCTURE_DWINEDIT_H