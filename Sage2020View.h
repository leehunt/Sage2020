
// Sage2020View.h : interface of the CSage2020View class
//

#pragma once


class CSage2020View : public CView
{
protected: // create from serialization only
	CSage2020View() noexcept;
	DECLARE_DYNCREATE(CSage2020View)

// Attributes
public:
	CSage2020Doc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
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

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in Sage2020View.cpp
inline CSage2020Doc* CSage2020View::GetDocument() const
   { return reinterpret_cast<CSage2020Doc*>(m_pDocument); }
#endif

