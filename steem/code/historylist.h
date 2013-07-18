#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_HISTORYLIST_H)
// need no apart decla.h for this one
#ifndef HISTORYLIST_H
#define HISTORYLIST_H
#endif

//---------------------------------------------------------------------------
class THistoryList : public TStemDialog
{
private:
  static LRESULT __stdcall WndProc(HWND,UINT,WPARAM,LPARAM);
  void ManageWindowClasses(bool);
public:
  THistoryList();
  ~THistoryList();

  void Show(),Hide();

  void RefreshHistoryBox();

  int Width,Height;
  bool Maximized;
};

#if defined(STEVEN_SEAGAL) && defined(SS_STRUCTURE_HISTORYLIST_H)
#endif//HISTORYLIST_H
#endif