#include "pch.h"

#include "Mocks.h"

CSage2020App theApp;

CSage2020App::CSage2020App() {
  // Used for profile testing.
  m_pszProfileName = _tcsdup(_T("Sage2020Test"));
}

void COutputWnd::AppendDebugTabMessage(LPCTSTR lpszItem) {}
