#pragma once

#include <Windows.h>

#define BUILD_TAB 0
#define DEBUG_TAB 1
#define FIND_TAB 0

#include <afxdockablepane.h>
#include <afxtabctrl.h>
#include <afxwin.h>
#include <atltypes.h>
#include <intsafe.h>

/////////////////////////////////////////////////////////////////////////////
// COutputList window

class COutputList : public CListBox {
  // Construction
 public:
  COutputList() noexcept;

  // Implementation
 public:
  virtual ~COutputList();

 protected:
  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

  afx_msg void OnEditCopy(CCmdUI* pCmdUI);
  afx_msg void OnEditCopy();

  afx_msg void OnEditClear(CCmdUI* pCmdUI);
  afx_msg void OnEditClear();

  afx_msg void OnViewOutput();

  DECLARE_MESSAGE_MAP()
};

class COutputWnd : public CDockablePane {
  // Construction
 public:
  COutputWnd() noexcept;

  void UpdateFonts();

  void AppendDebugTabMessage(LPCTSTR lpszItem);

  // Attributes
 protected:
  CMFCTabCtrl m_wndTabs;

#if BUILD_TAB
  COutputList m_wndOutputBuild;
#endif
#if DEBUG_TAB
  COutputList m_wndOutputDebug;
#endif
#if FIND_TAB
  COutputList m_wndOutputFind;
#endif

 protected:
#if OUTPUT_TAB
  void FillBuildWindow();
#endif
#if DEBUG_TAB
  void FillDebugWindow();
#endif
#if FIND_TAB
  void FillFindWindow();
#endif

  void AdjustHorzScroll(CListBox& wndListBox);

  // Implementation
 public:
  virtual ~COutputWnd();

 protected:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnSize(UINT nType, int cx, int cy);

  DECLARE_MESSAGE_MAP()
};
