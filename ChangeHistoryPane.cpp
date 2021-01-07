#include "pch.h"

#include "ChangeHistoryPane.h"
#include "Sage2020.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CChangeHistoryPane, CDockablePane)
ON_WM_ACTIVATE()
ON_WM_CREATE()
ON_WM_SIZE()
ON_WM_SETFOCUS()
ON_NOTIFY(TVN_ITEMEXPANDING, IDR_HISTORY_TREE, OnTreeNotifyExpanding)
END_MESSAGE_MAP()

void CChangeHistoryPane::OnActivate(UINT nState,
                                    CWnd* pWndOther,
                                    BOOL bMinimized) {
  CDockablePane::OnActivate(nState, pWndOther, bMinimized);
  // TODO: Add your message handler code here
}

int CChangeHistoryPane::OnCreate(LPCREATESTRUCT lpCreateStruct) {
  if (CDockablePane::OnCreate(lpCreateStruct) == -1)
    return -1;

  CRect rectDummy;
  rectDummy.SetRectEmpty();

  if (!m_wndTreeCtrl.Create(WS_VISIBLE | WS_CHILD | TVS_SHOWSELALWAYS |
                                TVS_HASBUTTONS | TVS_LINESATROOT |
                                TVS_NOTOOLTIPS,
                            rectDummy, this, IDR_HISTORY_TREE)) {
    TRACE0("Failed to create Versions Tree \n");
    return -1;  // fail to create
  }

  // InitPropList();

  AdjustLayout();
  return 0;
}

void CChangeHistoryPane::OnSize(UINT nType, int cx, int cy) {
  CDockablePane::OnSize(nType, cx, cy);

  AdjustLayout();
}

void CChangeHistoryPane::AdjustLayout() {
  if (GetSafeHwnd() == NULL) {
    return;
  }

  CRect rectClient;
  GetClientRect(rectClient);

  m_wndTreeCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top,
                             rectClient.Width(), rectClient.Height(),
                             SWP_NOACTIVATE | SWP_NOZORDER);
}

void CChangeHistoryPane::InitPropList() {
  SetPropListFont();

  if (m_imageList == NULL) {
    VERIFY(m_imageList.Create(
        theApp.m_bHiColorIcons ? IDB_HISTORY_TREE_HC : IDB_HISTORY_TREE, 16,
        1 /*nGrow*/, RGB(255, 0, 255)));
    VERIFY(m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL) ==
           NULL /*no prev image list*/);
  }
}

void CChangeHistoryPane::SetPropListFont() {
  ::DeleteObject(m_fntTreeCtrl.Detach());

  LOGFONT lf;
  afxGlobalData.fontRegular.GetLogFont(&lf);

  NONCLIENTMETRICS info;
  info.cbSize = sizeof(info);

  afxGlobalData.GetNonClientMetrics(info);

  lf.lfHeight = info.lfMenuFont.lfHeight;
  lf.lfWeight = info.lfMenuFont.lfWeight;
  lf.lfItalic = info.lfMenuFont.lfItalic;

  m_fntTreeCtrl.CreateFontIndirect(&lf);

  m_wndTreeCtrl.SetFont(&m_fntTreeCtrl);
}

void CChangeHistoryPane::OnSetFocus(CWnd* pOldWnd) {
  CDockablePane::OnSetFocus(pOldWnd);

  m_wndTreeCtrl.SetFocus();
}

void CChangeHistoryPane::OnTreeNotifyExpanding(NMHDR* pNMHDR,
                                               LRESULT* plResult) {
  const NMTREEVIEW* pTreeView = reinterpret_cast<const NMTREEVIEW*>(pNMHDR);

  HTREEITEM htreeitem = pTreeView->itemNew.hItem;
}
