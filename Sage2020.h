
// Sage2020.h : main header file for the Sage2020 application
//
#pragma once

#ifndef __AFXWIN_H__
#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"  // main symbols

// CSage2020App:
// See Sage2020.cpp for the implementation of this class
//

class CSage2020App : public CWinAppEx {
 public:
  CSage2020App() noexcept;

  // Overrides
 public:
  virtual BOOL InitInstance();
  virtual int ExitInstance();

  // Implementation
  BOOL m_bHiColorIcons;

  virtual void PreLoadState();
  virtual void LoadCustomState();
  virtual void SaveCustomState();

  afx_msg void OnAppAbout();
  DECLARE_MESSAGE_MAP()
};

extern CSage2020App theApp;
