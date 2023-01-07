
// Sage2020View.h : interface of the CSage2020View class
//

#pragma once
#include "Sage2020ViewDocListener.h"
#include "Utility.h"

class CSage2020Doc;
class FileVersionInstance;
class CSage2020View : public CScrollView, public Sage2020ViewDocListener {
 protected:  // create from serialization only
  CSage2020View() noexcept;
  DECLARE_DYNCREATE(CSage2020View)

  // Attributes
 public:
  CSage2020Doc* GetDocument() const;

  // Operations
 public:
  void FindAndSelectString(LPCTSTR szFind,
                           bool fScroll,
                           bool fBackward = false,
                           bool* pfWraparound = NULL);
  void FindAndSelectVersion(bool fCurVer,
                            bool fBackwards = false,
                            bool* pfWraparound = NULL);

  void SetCaseSensitive(bool fCaseSensitive) {
    m_fCaseSensitive = fCaseSensitive;
  }
  bool GetCaseSensitive() const { return m_fCaseSensitive; }

  void SetHighlightAll(bool fHighlightAll) { m_fHighlightAll = fHighlightAll; }
  bool GetHighlightAll() const { return m_fHighlightAll; }

  static COLORREF CrBackgroundForVersion(int nVer, int nVerMax);

  // Overrides
 protected:
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  virtual void OnInitialUpdate();
  virtual void OnDraw(CDC* pDC);  // overridden to draw this view
  virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
  virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll);

  // Implementation
  CFont* GetCustomFont(int* pFontGenerationIndex = nullptr);
  void UpdateScrollSizes(int dx);

  // REVIEW: MFC has 'CView::OnUpdate' that should be used to update /
  // invalidate the View (but NOT draw) in response to Doc updates.
  virtual void DocEditNotification(int iLine, int cLine);   // REVIEW: virtual?
  virtual void DocVersionChangedNotification(
      const DiffTreePath& commit_path);  // REVIEW: virtual?
  virtual void DocBranchChangedNotification(
      const std::vector<FileVersionDiff>& old_branch,
      const std::vector<FileVersionDiff>& new_branch);  // REVIEW: virtual?

  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
  virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
  virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

  // Implementation
 public:
  virtual ~CSage2020View();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

 protected:
  CSize m_sizChar;
  CSize m_sizPage;
  CSize m_sizAll;
  int m_sizCharFontGenerationIndex;

  int m_iSelStart;
  int m_iSelEnd;

  bool m_fLeftMouseDown;
  int m_iMouseDown;

  CString m_strSearchLast;
  CPoint m_ptSearchLast;
  bool m_fCaseSensitive;
  bool m_fHighlightAll;

  CSage2020Doc* m_pDocListened;

  CFont m_font;
  int m_fontGenerationIndex;
  CRegKey m_accesibilityScaleKey;
  SmartWindowsHandle m_accessibityRegKeyModificationEvent;

  const FileVersionInstance* m_currentFileVersionInstance;

  // Generated message map functions
 protected:
  afx_msg void OnFilePrintPreview();

  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

  afx_msg BOOL OnEraseBkgnd(CDC* pDC);

  afx_msg void OnSize(UINT nType, int cx, int cy);

  afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

  afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

  afx_msg void OnMouseMove(UINT nFlags, CPoint point);

  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

  afx_msg void OnEditCopy(CCmdUI* pCmdUI);
  afx_msg void OnEditCopy();

  afx_msg void OnEditSelectAll(CCmdUI* pCmdUI);
  afx_msg void OnEditSelectAll();

  afx_msg void OnGotoDlg();  // REVIEW: place this in MainFrm.h

  afx_msg void OnJumpToChangeVersion(CCmdUI* pCmdUI);
  afx_msg void OnJumpToChangeVersion();

  afx_msg void OnUpdatePropertiesPaneGrid(CCmdUI* pCmdUI);

  afx_msg void OnUpdateVersionsTree(CCmdUI* pCmdUI);

  afx_msg void OnUpdateStatusLineNumber(CCmdUI* pCmdUI);

  DECLARE_MESSAGE_MAP()
 public:
  afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};

#ifndef _DEBUG  // debug version in Sage2020View.cpp
inline CSage2020Doc* CSage2020View::GetDocument() const {
  return reinterpret_cast<CSage2020Doc*>(m_pDocument);
}
#endif
