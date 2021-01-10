#pragma once
#include <afxdockablepane.h>
#include <string>
#include <vector>

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

 protected:
  void AdjustLayout();

  void InitPropList();

 protected:
  CTreeCtrl m_wndTreeCtrl;
  CImageList m_imageList;
  CFont m_fntTreeCtrl;
  CToolTipCtrl* m_pToolTipControl;

 public:
  afx_msg void SetPropListFont();
  void OnSetFocus(CWnd* pOldWnd);
  void OnTreeNotifyExpanding(NMHDR* pNMHDR, LRESULT* plResult);
  static bool FEnsureTreeItemsAndSelection(
      CTreeCtrl& tree,
      HTREEITEM htreeitemRoot,
      const std::vector<FileVersionDiff>& file_diffs,
      const GitHash& selected_commit);
};
