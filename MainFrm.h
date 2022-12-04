
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#define FILE_VIEW_UI 0
#define CLASS_VIEW_UI 0
#define OUTPUT_PANE 1
#define OUTLOOK_BAR 0
#define SHELL_TREE_CONTROL 0
#define CALENDAR_BAR 0

#if FILE_VIEW_UI
#include "FileView.h"
#endif
#if CLASS_VIEW_UI
#include "ClassView.h"
#endif
#if OUTPUT_PANE
#include "OutputWnd.h"
#endif
#include "PropertiesWnd.h"
#if CALENDAR_BAR
#include "CalendarBar.h"
#endif
#include "ChangeHistoryPane.h"
#include "Resource.h"

class COutlookBar : public CMFCOutlookBar {
  virtual BOOL AllowShowOnPaneMenu() const { return TRUE; }
  virtual void GetPaneName(CString& strName) const {
    BOOL bNameValid = strName.LoadString(IDS_OUTLOOKBAR);
    ASSERT(bNameValid);
    if (!bNameValid)
      strName.Empty();
  }
};

class CMainFrame : public CFrameWndEx {
 protected:  // create from serialization only
  CMainFrame() noexcept;
  DECLARE_DYNCREATE(CMainFrame)

  // Attributes
 public:
  // Operations
 public:
  // Overrides
 public:
  virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  virtual BOOL LoadFrame(UINT nIDResource,
                         DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW |
                                                FWS_ADDTOTITLE,
                         CWnd* pParentWnd = nullptr,
                         CCreateContext* pContext = nullptr);

  // Implementation
 public:
  virtual ~CMainFrame();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  CWnd& GetStatusWnd() { return m_wndStatusBar; }

  COutputWnd& GetOutputWnd() { return m_wndOutput; }

 protected:
  CSplitterWnd m_wndSplitter;

  // control bar embedded members
  CMFCMenuBar m_wndMenuBar;
  CMFCToolBar m_wndToolBar;
  CMFCStatusBar m_wndStatusBar;
  CMFCToolBarImages m_UserImages;
#if FILE_VIEW_UI
  CFileView m_wndFileView;
#endif
#if CLASS_VIEW_UI
  CClassView m_wndClassView;
#endif
#if OUTPUT_PANE
  COutputWnd m_wndOutput;
#endif
  CPropertiesWnd m_wndProperties;
#if OUTLOOK_BAR
  COutlookBar m_wndNavigationBar;
#endif
#if SHELL_TREE_CONTROL
  CMFCShellTreeCtrl m_wndTree;
#endif
#if CALENDAR_BAR
  CCalendarBar m_wndCalendar;
#endif
  CChangeHistoryPane m_wndChangeHistoryPane;

  // Generated message map functions
 protected:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnViewCustomize();
  afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
  afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
  DECLARE_MESSAGE_MAP()

  BOOL CreateDockingWindows();
  void SetDockingWindowIcons(BOOL bHiColorIcons);
#if OUTLOOK_BAR
#if CALENDAR_BAR
  BOOL CreateOutlookBar(CMFCOutlookBar& bar,
                        UINT uiID,
                        CMFCShellTreeCtrl& tree,
                        CCalendarBar& calendar,
                        int nInitialWidth);
#endif

  int FindFocusedOutlookWnd(CMFCOutlookBarTabCtrl** ppOutlookWnd);

  CMFCOutlookBarTabCtrl* FindOutlookParent(CWnd* pWnd);
  CMFCOutlookBarTabCtrl* m_pCurrOutlookWnd;
  CMFCOutlookBarPane* m_pCurrOutlookPage;
#endif
 public:
  virtual BOOL OnCmdMsg(UINT nID,
                        int nCode,
                        void* pExtra,
                        AFX_CMDHANDLERINFO* pHandlerInfo);
};
