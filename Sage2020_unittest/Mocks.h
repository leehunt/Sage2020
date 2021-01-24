#pragma once

#include <afxwin.h>
#include <tchar.h>
#include <windows.h>


class CSage2020App : public CWinApp {
 public:
  CSage2020App();
};
extern CSage2020App theApp;

class COutputWnd {
 public:
  void AppendDebugTabMessage(LPCTSTR lpszItem);
};
