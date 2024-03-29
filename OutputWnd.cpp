
#include "pch.h"

#include "OutputWnd.h"

#include <cassert>

#include <afx.h>
#include <afxMDIFrameWndEx.h>
#include <afxglobals.h>
#include <afxpopupmenu.h>
#include <afxres.h>
#include <afxstr.h>
#include <corecrt_wstring.h>
#include <errhandlingapi.h>
#include <libloaderapi.h>
#include <stddef.h>
#include <tchar.h>

#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputBar

COutputWnd::COutputWnd() noexcept {}

COutputWnd::~COutputWnd() {}

// clang-format off
BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()
// clang-format on

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
  if (CDockablePane::OnCreate(lpCreateStruct) == -1)
    return -1;

  CRect rectDummy;
  rectDummy.SetRectEmpty();

  // Create tabs window:
  if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 1)) {
    TRACE0("Failed to create output tab window\n");
    return -1;  // fail to create
  }

  // Create output panes:
  const DWORD dwStyle =
      LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

#if BUILD_TAB
  if (!m_wndOutputBuild.Create(dwStyle, rectDummy, &m_wndTabs, 2)) {
    TRACE0("Failed to create output windows\n");
    return -1;  // fail to create
  }
#endif
#if DEBUG_TAB
  if (!m_wndOutputDebug.Create(dwStyle, rectDummy, &m_wndTabs, 3)) {
    TRACE0("Failed to create output windows\n");
    return -1;  // fail to create
  }
#endif
#if FIND_TAB
  if (!m_wndOutputFind.Create(dwStyle, rectDummy, &m_wndTabs, 4)) {
    TRACE0("Failed to create output windows\n");
    return -1;  // fail to create
  }
#endif

  UpdateFonts();

  CString strTabName;
#if _DEBUG
  BOOL bNameValid;
#endif

  // Attach list windows to tab:
#if BUILD_TAB
#if _DEBUG
  bNameValid =
#endif
      strTabName.LoadString(IDS_BUILD_TAB);
  ASSERT(bNameValid);
  m_wndTabs.AddTab(&m_wndOutputBuild, strTabName, (UINT)0);
#endif
#if DEBUG_TAB
#if _DEBUG
  bNameValid =
#endif
      strTabName.LoadString(IDS_DEBUG_TAB);
  ASSERT(bNameValid);
  m_wndTabs.AddTab(&m_wndOutputDebug, strTabName, (UINT)1);
#endif
#if FIND_TAB
#if _DEBUG
  bNameValid =
#endif
      strTabName.LoadString(IDS_FIND_TAB);
  ASSERT(bNameValid);
  m_wndTabs.AddTab(&m_wndOutputFind, strTabName, (UINT)2);
#endif

  // Fill output tabs with some dummy text (nothing magic here)
#if BUILD_TAB
  FillBuildWindow();
#endif
#if DEBUG_TAB
  // FillDebugWindow();
#endif
#if FIND_TAB
  FillFindWindow();
#endif

  return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy) {
  CDockablePane::OnSize(nType, cx, cy);

  // Tab control should cover the whole client area:
  m_wndTabs.SetWindowPos(nullptr, -1, -1, cx, cy,
                         SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void COutputWnd::AdjustHorzScroll(CListBox& wndListBox) {
  CClientDC dc(this);
  CFont* pOldFont = dc.SelectObject(&afxGlobalData.fontRegular);

  int cxExtentMax = 0;

  for (int i = 0; i < wndListBox.GetCount(); i++) {
    CString strItem;
    wndListBox.GetText(i, strItem);

    cxExtentMax = max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
  }

  wndListBox.SetHorizontalExtent(cxExtentMax);
  dc.SelectObject(pOldFont);
}

#if BUILD_TAB
void COutputWnd::FillBuildWindow() {
  m_wndOutputBuild.AddString(_T("Build output is being displayed here."));
  m_wndOutputBuild.AddString(
      _T("The output is being displayed in rows of a list view"));
  m_wndOutputBuild.AddString(
      _T("but you can change the way it is displayed as you wish..."));
}
#endif

#if DEBUG_TAB
void COutputWnd::FillDebugWindow() {
  m_wndOutputDebug.AddString(_T("Debug output is being displayed here."));
  m_wndOutputDebug.AddString(
      _T("The output is being displayed in rows of a list view"));
  m_wndOutputDebug.AddString(
      _T("but you can change the way it is displayed as you wish..."));
}
#endif

#if FIND_TAB
void COutputWnd::FillFindWindow() {
  m_wndOutputFind.AddString(_T("Find output is being displayed here."));
  m_wndOutputFind.AddString(
      _T("The output is being displayed in rows of a list view"));
  m_wndOutputFind.AddString(
      _T("but you can change the way it is displayed as you wish..."));
}
#endif

void COutputWnd::UpdateFonts() {
#if BUILD_TAB
  m_wndOutputBuild.SetFont(&afxGlobalData.fontRegular);
#endif
#if DEBUG_TAB
  m_wndOutputDebug.SetFont(&afxGlobalData.fontRegular);
#endif
#if FIND_TAB
  m_wndOutputFind.SetFont(&afxGlobalData.fontRegular);
#endif
}

void COutputWnd::AppendDebugTabMessage(LPCTSTR lpszItem) {
  m_wndOutputDebug.AddString(lpszItem);
}

/////////////////////////////////////////////////////////////////////////////
// COutputList1

COutputList::COutputList() noexcept {}

COutputList::~COutputList() {}

// clang-format off
BEGIN_MESSAGE_MAP(COutputList, CListBox)
  ON_WM_CONTEXTMENU()
  ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &COutputList::OnEditCopy)
  ON_COMMAND(ID_EDIT_COPY, &COutputList::OnEditCopy)
  ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, &COutputList::OnEditClear)
  ON_COMMAND(ID_EDIT_CLEAR, &COutputList::OnEditClear)
  ON_COMMAND(ID_VIEW_OUTPUTWND, &COutputList::OnViewOutput)
  ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()
// clang-format on

/////////////////////////////////////////////////////////////////////////////
// COutputList message handlers

void COutputList::OnContextMenu(CWnd* /*pWnd*/, CPoint point) {
  CMenu menu;
  menu.LoadMenu(IDR_OUTPUT_POPUP);

  CMenu* pSumMenu = menu.GetSubMenu(0);

  if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx))) {
    CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

    if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu,
                            FALSE, TRUE))
      return;

    ((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
    UpdateDialogControls(this, FALSE);
  }

  SetFocus();
}

void COutputList::OnEditCopy(CCmdUI* pCmdUI) {
  pCmdUI->Enable(GetCurSel() != LB_ERR);
}

void COutputList::OnEditCopy() {
  bool fClipboardOpen = false;
  HGLOBAL hGlob = NULL;

  auto current_selection = GetCurSel();
  if (current_selection != LB_ERR) {
    if (!::OpenClipboard(NULL /*hWndNewOwner; NULL - use current task */)) {
      CString msg;
      msg.Format(_T("Cannot open the Clipboard, error: %d"), ::GetLastError());
      AfxMessageBox(msg);
      goto LCleanup;
    }
    fClipboardOpen = true;

    // Remove the current Clipboard contents
    if (!::EmptyClipboard()) {
      CString msg;
      msg.Format(_T("Cannot empty the Clipboard, error: %d"), ::GetLastError());
      AfxMessageBox(msg);
      goto LCleanup;
    }
    // Get the currently selected data
    static_assert(sizeof(TCHAR) == 2);

    CString line;
    GetText(current_selection, line /*ref*/);

    size_t cbClip = (line.GetLength() + 1 /*lf -> cr/lf*/ + 1 /*ending NUL*/) *
                    sizeof(TCHAR);
    hGlob = ::GlobalAlloc(GMEM_MOVEABLE, cbClip);
    if (hGlob == NULL) {
      CString msg;
      msg.Format(_T("Cannot alloc clipboard memory, error: %d"),
                 ::GetLastError());
      AfxMessageBox(msg);
      goto LCleanup;
    }

    TCHAR* szClip = static_cast<TCHAR*>(::GlobalLock(hGlob));
    if (szClip == NULL) {
      CString msg;
      msg.Format(_T("Cannot alloc clipboard memory, error: %d"),
                 ::GetLastError());
      AfxMessageBox(msg);
      goto LCleanup;
    }

    wcscpy_s(szClip, cbClip / sizeof(TCHAR), line);
    int cchLine = line.GetLength();
    if (cchLine > 0 && szClip[cchLine - 1] == _T('\n')) {
      szClip[cchLine - 1] = _T('\r');
      szClip[cchLine++] = _T('\n');
    }
    // NUL terminate
    szClip[cchLine++] = _T('\0');

    VERIFY(::GlobalUnlock(hGlob) == 0 /*no locks*/);

    hGlob = ::GlobalReAlloc(hGlob, cchLine * sizeof(TCHAR),
                            GMEM_MOVEABLE);  // shrink down to used size
    assert(hGlob != NULL);

    if (hGlob == NULL || ::SetClipboardData(CF_UNICODETEXT, hGlob) == NULL) {
      CString msg;
      msg.Format(_T("Unable to set Clipboard data, error: %d"),
                 ::GetLastError());
      AfxMessageBox(msg);
      goto LCleanup;
    }
  }

LCleanup:
  if (fClipboardOpen)
    ::CloseClipboard();
  if (hGlob != NULL)
    ::GlobalFree(hGlob);
}

void COutputList::OnEditClear(CCmdUI* pCmdUI) {
  pCmdUI->Enable(GetCount() > 0);
}

void COutputList::OnEditClear() {
  MessageBox(_T("Clear output"));
}

void COutputList::OnViewOutput() {
  CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
  CMDIFrameWndEx* pMainFrame =
      DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

  if (pMainFrame != nullptr && pParentBar != nullptr) {
    pMainFrame->SetFocus();
    pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
    pMainFrame->RecalcLayout();
  }
}
