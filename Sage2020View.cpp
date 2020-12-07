
// Sage2020View.cpp : implementation of the CSage2020View class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Sage2020.h"
#endif

#include "Sage2020Doc.h"
#include "Sage2020View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSage2020View

IMPLEMENT_DYNCREATE(CSage2020View, CView)

BEGIN_MESSAGE_MAP(CSage2020View, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CSage2020View::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CSage2020View construction/destruction

CSage2020View::CSage2020View() noexcept
{
	// TODO: add construction code here

}

CSage2020View::~CSage2020View()
{
}

BOOL CSage2020View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CSage2020View drawing

void CSage2020View::OnDraw(CDC* /*pDC*/)
{
	CSage2020Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CSage2020View printing


void CSage2020View::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CSage2020View::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSage2020View::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CSage2020View::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CSage2020View::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CSage2020View::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CSage2020View diagnostics

#ifdef _DEBUG
void CSage2020View::AssertValid() const
{
	CView::AssertValid();
}

void CSage2020View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSage2020Doc* CSage2020View::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSage2020Doc)));
	return (CSage2020Doc*)m_pDocument;
}
#endif //_DEBUG


// CSage2020View message handlers
