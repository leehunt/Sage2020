#pragma once
#include <afxdockablepane.h>
class CChangeHistoryPane : public CDockablePane {
 public:
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

 public:
  afx_msg void SetPropListFont();
  void OnSetFocus(CWnd* pOldWnd);
  void OnTreeNotifyExpanding(NMHDR* pNMHDR, LRESULT* plResult);
};
