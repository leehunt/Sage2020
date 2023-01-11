#pragma once

#include <Windows.h>

#include <afxdockablepane.h>
#include <afxwin.h>
#include <commctrl.h>
#include <intsafe.h>
#include <string>
#include <vector>
#include "framework.h"

struct FileVersionDiff;
struct GitHash;
class CChangeHistoryPane : public CDockablePane {
 public:
  CChangeHistoryPane();
  ~CChangeHistoryPane();

  DECLARE_MESSAGE_MAP()
  afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnSetFocus(CWnd* pOldWnd);
  afx_msg void OnTreeNotifyExpanding(NMHDR* pNMHDR, LRESULT* plResult);
  // void OnTreeDeleteItem(NMHDR* pNMHDR, LRESULT* plResult);
  afx_msg void SetPropListFont();

 protected:
  void AdjustLayout();

  void InitPropList();

 protected:
  CTreeCtrl m_wndTreeCtrl;
  CImageList m_imageList;
  CFont m_fntTreeCtrl;
  CToolTipCtrl* m_pToolTipControl;

 public:
  static bool FEnsureTreeItemsAndSelection(
      CTreeCtrl& tree,
      HTREEITEM htreeitemRoot,
      const std::vector<FileVersionDiff>& file_diffs,
      const GitHash& selected_commit);
};
