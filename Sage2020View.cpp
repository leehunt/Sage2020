// Sage2020View.cpp : implementation of the CSage2020View class
//
#include "pch.h"

#include <cwctype>

#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview,
// thumbnail and search filter handlers and allows sharing of document code with
// that project.
#ifndef SHARED_HANDLERS
#include "Sage2020.h"
#endif

#include "Sage2020Doc.h"
#include "Sage2020View.h"

#include "FIleVersionInstanceEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const int c_dxIndent = 2;

// CSage2020View

IMPLEMENT_DYNCREATE(CSage2020View, CScrollView)

BEGIN_MESSAGE_MAP(CSage2020View, CScrollView)
ON_WM_ERASEBKGND()
ON_WM_SIZE()
ON_WM_KEYDOWN()
ON_WM_MOUSEMOVE()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()

// Standard printing commands
ON_COMMAND(ID_FILE_PRINT, &CScrollView::OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_DIRECT, &CScrollView::OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CSage2020View::OnFilePrintPreview)
ON_WM_CONTEXTMENU()
ON_WM_RBUTTONUP()
ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

// CSage2020View construction/destruction

CSage2020View::CSage2020View() noexcept
    : m_iSelStart(-1),
      m_iSelEnd(-1),
      m_fLeftMouseDown(false),
      m_ptSearchLast(-1, 0),
      m_fCaseSensitive(false),
      m_fHighlightAll(true),
      m_pDocListened(nullptr),
      m_currentFileVersionInstance(nullptr) {
  m_sizChar.cx = 0;
}

CSage2020View::~CSage2020View() {
  if (m_pDocListened != NULL) {
    // m_pDocListened->RemoveDocListener(*this);  // TODO
    m_pDocListened = NULL;
  }
}

BOOL CSage2020View::PreCreateWindow(CREATESTRUCT& cs) {
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs

  return CView::PreCreateWindow(cs);
}

void CSage2020View::OnInitialUpdate()  // called first time after
                                       // construct
{
  __super::OnInitialUpdate();

  m_iSelStart = m_iSelEnd = -1;

  m_ptSearchLast.SetPoint(-1, 0);
  m_strSearchLast.Empty();

  SetCustomFont();

  RECT rcClient;
  GetClientRect(&rcClient);
  UpdateScrollSizes(rcClient.bottom);
}

void CSage2020View::SetCustomFont() {
  CDC* pdc = GetDC();
  assert(pdc != NULL);
  if (pdc != NULL) {
    CFont* pfnt = pdc->GetCurrentFont();
    assert(pfnt != NULL);
    if (pfnt != NULL) {
      LOGFONT lf = {};
      pfnt->GetLogFont(&lf);

      _tcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), _T("Courier New"));

      HFONT hfont = static_cast<HFONT>(m_font.Detach());
      if (hfont != NULL)
        ::DeleteObject(hfont);
      m_font.CreateFontIndirect(&lf);

      SetFont(&m_font);
    }
  }
}

void CSage2020View::UpdateScrollSizes(int cy) {
  CSage2020Doc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (pDoc == NULL)
    return;

  CDC* pDC = GetDC();
  if (pDC != NULL) {
    if (!m_sizChar.cx)
      m_sizChar = pDC->GetTextExtent(_T("W"), 1);

    m_sizAll = m_sizChar;
    m_sizAll.cx *= 256;
    m_sizAll.cy *= static_cast<LONG>(pDoc->GetFileVersionInstanceSize());

    m_sizPage.cx = m_sizChar.cx;
    m_sizPage.cy = cy / m_sizChar.cy * m_sizChar.cy;

    SetScrollSizes(MM_TEXT, m_sizAll, m_sizPage, m_sizChar);

    ReleaseDC(pDC);
  }
}

static int FindNoCase(const CString& string_to_search,
                      int offset,
                      const CString& string_to_find) {
  auto it =
      std::search(string_to_search.GetString() + offset,
                  string_to_search.GetString() + string_to_search.GetLength(),
                  string_to_find.GetString(),
                  string_to_find.GetString() + string_to_find.GetLength(),
                  [](TCHAR ch1, TCHAR ch2) {
                    return std::towupper(ch1) == std::towupper(ch2);
                  });
  if (it == string_to_search.GetString() + string_to_search.GetLength())
    return -1;
  return static_cast<int>(it - string_to_search.GetString());
}

const COLORREF c_rgclrrefAge[] = {
    RGB(0xFF, 0xFF, 0xFF),  // no change (ver 1)

    //	RGB(0xF7, 0xFF, 0xF7),  // not visible
    //	RGB(0xEF, 0xFF, 0xEF),
    //	RGB(0xE7, 0xFF, 0xE7),
    RGB(0xDF, 0xFF, 0xDF),
    RGB(0xD7, 0xFF, 0xD7),
    RGB(0xCF, 0xFF, 0xCF),
    RGB(0xC7, 0xFF, 0xC7),
    RGB(0xBF, 0xFF, 0xBF),
    RGB(0xB7, 0xFF, 0xB7),
    RGB(0xAF, 0xFF, 0xAF),
    RGB(0xA7, 0xFF, 0xA7),
    RGB(0x9F, 0xFF, 0x9F),
    RGB(0x97, 0xFF, 0x97),
    RGB(0x8F, 0xFF, 0x8F),
    RGB(0x87, 0xFF, 0x87),
    RGB(0x7F, 0xFF, 0x7F),
    RGB(0x77, 0xFF, 0x77),
    RGB(0x6F, 0xFF, 0x6F),
    RGB(0x67, 0xFF, 0x67),
    RGB(0x5F, 0xFF, 0x5F),
    RGB(0x57, 0xFF, 0x57),
    RGB(0x4F, 0xFF, 0x4F),
    RGB(0x47, 0xFF, 0x47),
    RGB(0x3F, 0xFF, 0x3F),
    RGB(0x37, 0xFF, 0x37),
    RGB(0x2F, 0xFF, 0x2F),
    RGB(0x27, 0xFF, 0x27),
    RGB(0x1F, 0xFF, 0x1F),
    RGB(0x17, 0xFF, 0x17),
    RGB(0x0F, 0xFF, 0x0F),
    RGB(0x07, 0xFF, 0x07),
    RGB(0x00, 0xFF, 0x00),
    RGB(0x00, 0xF0, 0x00),
    RGB(0x00, 0xE0, 0x00),
    RGB(0x00, 0xD0, 0x00),
    RGB(0x00, 0xC0, 0x00),
    RGB(0x00, 0xB0, 0x00),
    RGB(0x00, 0xA0, 0x00),
    RGB(0x00, 0x90, 0x00),
    RGB(0x00, 0x80, 0x00),
};

/*static*/ COLORREF CSage2020View::CrBackgroundForVersion(int nVer,
                                                          int nVerMax) {
  // set colors
  assert(nVerMax >= nVer);

  int iColor = 0;  // no change color
  if (nVer > 1) {
    iColor = _countof(c_rgclrrefAge) - (nVerMax - nVer) -
             1;  // -1 to ignore the zeroth no-change color

    // check that changes outside of the version color table always be iColor 1
    // (the oldest change color)
    if (iColor <= 0)
      iColor = 1;
  }
  assert(0 <= iColor && iColor < _countof(c_rgclrrefAge));
  return c_rgclrrefAge[iColor];
}
// CSage2020View drawing

BOOL CSage2020View::OnEraseBkgnd(CDC* pDC) {
  CSage2020Doc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (pDoc == NULL || pDoc->GetFileVersionInstanceSize() == 0)
    return __super::OnEraseBkgnd(pDC);

  return TRUE;  // we will erase when drawing
}

void CSage2020View::OnDraw(CDC* pDC) {
  CSage2020Doc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  if (m_pDocListened != pDoc) {
    // TODO
    // if (m_pDocListened != NULL)
    // m_pDocListened->RemoveDocListener(*this);
    // pDoc->AddDocListener(*this);

    m_pDocListened = pDoc;
  }

  RECT rcClient;
  GetClientRect(&rcClient);

  CFont* pfontOld = pDC->SelectObject(&m_font);

  const int iSearchOffset = GetHighlightAll() ? 0 : m_ptSearchLast.x;
  CPoint cptViewport = pDC->GetViewportOrg();
  CPoint cptOld = cptViewport;
  auto file_version_instance = pDoc->GetFileVersionInstance();
  if (m_currentFileVersionInstance != file_version_instance) {
    // TODO
    // if (m_currentFileVersionInstance != NULL)
    //  m_pfilerepLast->SetViewOrg(cptViewport);
    if (file_version_instance != nullptr) {
      // recalc for possible updated pDoc->FilerepRead().CLine()
      UpdateScrollSizes(rcClient.bottom);

      // ScrollToPosition(pfilerep->GetViewOrg());

      pDC->SetViewportOrg(-GetDeviceScrollPosition());
      cptViewport = pDC->GetViewportOrg();
    }

    m_currentFileVersionInstance = file_version_instance;
  }

  const int yScrollPos = -cptViewport.y;
  assert(yScrollPos == GetDeviceScrollPosition().y);
  const auto& diffs = pDoc->GetFileDiffs();
  const int nVerMax = static_cast<int>(diffs.size() - 1);
  const int cLine =
      file_version_instance != NULL
          ? static_cast<int>(file_version_instance->GetLines().size())
          : 0;
  const int cchFind = m_strSearchLast.GetLength();
  for (int y = yScrollPos / m_sizChar.cy * m_sizChar.cy;
       y < rcClient.bottom + yScrollPos; y += m_sizChar.cy) {
    int i = y / m_sizChar.cy;

    assert(i < cLine || cLine == 0);
    if (i >= cLine)
      break;

    // set colors
    int nVer =
        file_version_instance != NULL
            ? static_cast<int>(
                  file_version_instance->GetLineInfo(i + 1).commit_index())
            : 0;
    COLORREF crBack = CrBackgroundForVersion(nVer, nVerMax);
    COLORREF crFore = RGB(0x00, 0x00, 0x00);
    if (m_iSelStart <= i && i <= m_iSelEnd) {
      crBack = ~crBack & 0x00FFFFFF;
      crFore = ~crFore & 0x00FFFFFF;
    }
    pDC->SetBkColor(crBack);
    if (crFore == crBack)  // REVIEW: change this check to catch visually
                           // similar colors
      pDC->SetTextColor(~crFore & 0x00FFFFFF);
    else
      pDC->SetTextColor(crFore);

    RECT rcDrawT = {
        0,
        y,
        rcClient.right - (cptViewport.x + 0),
        m_sizChar.cy + y,
    };
    CString cstrTabbed(file_version_instance->GetLines()[i].c_str());
    int iFind;
    if (m_strSearchLast.IsEmpty() ||
        m_ptSearchLast.y != i && !GetHighlightAll() || iSearchOffset < 0 ||
        iSearchOffset >= cstrTabbed.GetLength() ||
        (iFind = GetCaseSensitive()
                     ? cstrTabbed.Find(m_strSearchLast, iSearchOffset) -
                           iSearchOffset
                     : FindNoCase(cstrTabbed.GetString(), iSearchOffset,
                                  m_strSearchLast)) < 0) {
      cstrTabbed.Replace(
          _T("\t"),
          _T("    "));  // REVIEW: this could be pre-processed once
      pDC->ExtTextOut(rcDrawT.left + c_dxIndent, rcDrawT.top, ETO_OPAQUE,
                      &rcDrawT, cstrTabbed, NULL);
    } else
    // draw any search highlight
    {
      iFind += iSearchOffset;
      // replace tabs with 4 spaces and update iFind
      int iTab = 0;
      while ((iTab = cstrTabbed.Find(_T("\t"), iTab)) >= 0) {
        cstrTabbed.Delete(iTab);
        cstrTabbed.Insert(iTab, _T("    "));
        if (iFind > iTab)
          iFind += 3;
        iTab += 4;
      }

      UINT uAlignPrev = pDC->SetTextAlign(TA_UPDATECP);
      pDC->MoveTo(c_dxIndent, y);
      do {
        CString cstrSubPrefix((LPCTSTR)cstrTabbed, iFind);
        CString cstrSubFound((LPCTSTR)cstrTabbed + iFind, cchFind);

        POINT ptPrev = pDC->GetCurrentPosition();
        pDC->TextOut(0, 0, cstrSubPrefix);
        POINT ptCur = pDC->GetCurrentPosition();

        crBack = ~crBack & 0x00FFFFFF;
        crFore = ~crFore & 0x00FFFFFF;
        pDC->SetBkColor(crBack);
        pDC->SetTextColor(crFore);

        pDC->TextOut(0, 0, cstrSubFound);

        crBack = ~crBack & 0x00FFFFFF;
        crFore = ~crFore & 0x00FFFFFF;
        pDC->SetBkColor(crBack);
        pDC->SetTextColor(crFore);

        cstrTabbed.Delete(0, iFind + cchFind);

        if (!GetHighlightAll())
          break;
        iFind = cstrTabbed.Find(m_strSearchLast);
      } while (iFind >= 0);

      POINT ptCur = pDC->GetCurrentPosition();
      rcDrawT.left = ptCur.x;
      pDC->ExtTextOut(0, 0,  // NOTE: drawing relative via TA_UPDATECP
                      ETO_OPAQUE, &rcDrawT, cstrTabbed, NULL);

      pDC->SetTextAlign(uAlignPrev);
    }
  }

  pDC->SelectObject(pfontOld);
}

BOOL CSage2020View::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll) {
  // This sucks.  nPos is derived from the upper half of WM_VSCROLL/WM_HSCROLL's
  // wParam which is a short.  To support more that 65536 scrollpos, fix the
  // value here
  WORD wScroll = LOWORD(nScrollCode);
  if (LOBYTE(wScroll) == 0xFF)
  /* fVert */
  {
    switch (HIBYTE(nScrollCode)) {
      case SB_THUMBPOSITION:
      case SB_THUMBTRACK: {
        SCROLLINFO si = {
            sizeof(si),
            SIF_TRACKPOS,
        };
        CScrollBar* pScrollBar = GetScrollBarCtrl(SB_VERT);
        if (pScrollBar != NULL) {
          if (pScrollBar->GetScrollInfo(&si, SIF_TRACKPOS))
            nPos = si.nTrackPos;
        } else {
          // Native Windows scrollbar
          if (::GetScrollInfo(GetSafeHwnd(), SB_VERT, &si))
            nPos = si.nTrackPos;
        }
        nPos = si.nTrackPos;
      }
      break;
    }
  } else if (HIBYTE(wScroll) == 0xFF)
  /* fHoriz */
  {
    switch (LOBYTE(wScroll)) {
      case SB_THUMBPOSITION:
      case SB_THUMBTRACK: {
        SCROLLINFO si = {
            sizeof(si),
            SIF_TRACKPOS,
        };
        CScrollBar* pScrollBar = GetScrollBarCtrl(SB_HORZ);
        if (pScrollBar != NULL) {
          if (pScrollBar->GetScrollInfo(&si, SIF_TRACKPOS))
            nPos = si.nTrackPos;
        } else {
          // Native Windows scrollbar
          if (::GetScrollInfo(GetSafeHwnd(), SB_HORZ, &si))
            nPos = si.nTrackPos;
        }
      } break;
    }
  }

  return __super::OnScroll(nScrollCode, nPos, bDoScroll);
}

BOOL CSage2020View::OnScrollBy(CSize sizeScroll, BOOL bDoScroll) {
  if (bDoScroll) {
    // make scroll actions >= a line height always start drawing at the top of a
    // line
    if (sizeScroll.cy >= m_sizChar.cy) {
      int y = GetScrollPos(SB_VERT);

      int dyOverage =
          sizeScroll.cy + y - (sizeScroll.cy + y) / m_sizChar.cy * m_sizChar.cy;
      sizeScroll.cy -= dyOverage;
    } else if (sizeScroll.cy <= -m_sizChar.cy) {
      int y = GetScrollPos(SB_VERT);

      int dyOverage =
          sizeScroll.cy + y - (sizeScroll.cy + y) / m_sizChar.cy * m_sizChar.cy;
      if (dyOverage)
        sizeScroll.cy += m_sizChar.cy - dyOverage;
    }
  }

  return __super::OnScrollBy(sizeScroll, bDoScroll);
}

void CSage2020View::DocEditNotification(int iLine, int cLine) {
  POINT ptCur = GetDeviceScrollPosition();

  SetRedraw(FALSE);

  // recalc for updated pDoc->FilerepRead().CLine()
  RECT rcClient;
  GetClientRect(&rcClient);
  UpdateScrollSizes(rcClient.bottom);

  int iLineTop = ptCur.y / m_sizChar.cy;
  int cLineOffset = m_iSelStart - iLineTop;
  if (m_iSelStart >= iLine) {
    if (cLine < 0 && m_iSelStart < iLine - cLine)
      m_iSelStart = iLine;
    else
      m_iSelStart += cLine;
    assert(m_iSelStart >= 0);
  }
  if (m_iSelEnd >= iLine) {
    if (cLine < 0 && m_iSelEnd < iLine - cLine)
      m_iSelEnd = iLine;
    else
      m_iSelEnd += cLine;
    assert(m_iSelEnd >= 0);
  }

  if (m_ptSearchLast.y >= iLine) {
    if (cLine < 0 && m_ptSearchLast.y < iLine - cLine)
      m_ptSearchLast.y = iLine;
    else
      m_ptSearchLast.y += cLine;
    assert(m_ptSearchLast.y >= 0);
  }

  if (iLineTop >= iLine) {
    if (cLine < 0 && iLineTop < iLine - cLine)
      iLineTop = iLine;
    else
      iLineTop += cLine;
    assert(iLineTop >= 0);
  }

  int iLineBottom = (ptCur.y + rcClient.bottom) / m_sizChar.cy;
  if (iLineBottom >= iLine) {
    if (cLine < 0 && iLineBottom < iLine - cLine)
      iLineBottom = iLine;
    else
      iLineBottom += cLine;
    assert(iLineBottom >= 0);
  }

  int iLineScrollAround = iLineTop <= m_iSelStart && m_iSelStart < iLineBottom
                              ? m_iSelStart
                              : iLineTop;

  if (iLineScrollAround >= iLine) {
    if (iLineScrollAround > iLineTop)
      ptCur.y = (iLineScrollAround - (cLineOffset)) * m_sizChar.cy;
    else
      ptCur.y += cLine * m_sizChar.cy;

    // momentarially turn off the visible bit to keep ScrollToPosition from
    // using windows scrolling
    LONG lWindowStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
    ::SetWindowLong(m_hWnd, GWL_STYLE, lWindowStyle & ~WS_VISIBLE);
    ScrollToPosition(ptCur);
    ::SetWindowLong(m_hWnd, GWL_STYLE, lWindowStyle);
  }

  SetRedraw(TRUE);
}

void CSage2020View::DocVersionChangedNotification(int nVer) {
  if (nVer == 0)
    m_currentFileVersionInstance = NULL;
}

// CSimpleAgeViewerView message handlers

void CSage2020View::OnMouseMove(UINT nFlags, CPoint point) {
  __super::OnMouseMove(nFlags, point);

  if (m_fLeftMouseDown) {
    CSage2020Doc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (pDoc == NULL)
      return;

    CPoint ptScroll = GetDeviceScrollPosition();
    int yMouse = ptScroll.y + point.y;
    int i = yMouse / m_sizChar.cy;
    int cLine = static_cast<int>(pDoc->GetFileVersionInstanceSize());
    if (i >= cLine)
      i = cLine - 1;
    if (i < 0)
      i = 0;

    if (i < m_iSelStart) {
      m_iSelStart = i;
      m_ptSearchLast.x = -1;
      m_ptSearchLast.y = i;
      Invalidate(FALSE /*bErase*/);
    } else if (i > m_iSelEnd) {
      m_iSelEnd = i;
      Invalidate(FALSE /*bErase*/);
    }
    // shrink the range if requested
    else if (m_iSelStart <= i && i <= m_iSelEnd && !(nFlags & MK_SHIFT)) {
      if (m_iSelStart == m_iMouseDown) {
        if (m_iSelEnd > i) {
          m_iSelEnd = i;
          Invalidate(FALSE /*bErase*/);
        }
      } else {
        // assert(m_iSelEnd == m_iMouseDown);
        if (m_iSelStart < i) {
          m_iSelStart = i;
          m_ptSearchLast.x = -1;
          m_ptSearchLast.y = i;
          Invalidate(FALSE /*bErase*/);
        }
      }
    }

    // scrolling
    RECT rcClient;
    GetClientRect(&rcClient);
    CPoint ptScrollNew = ptScroll;
    if (point.y > rcClient.bottom)
      ptScrollNew.y += point.y - rcClient.bottom;
    else if (point.y < rcClient.top)
      ptScrollNew.y += point.y + rcClient.top;

    if (point.x > rcClient.right)
      ptScrollNew.x += point.x - rcClient.right;
    else if (point.x < rcClient.left)
      ptScrollNew.x += point.x + rcClient.left;

    if (ptScroll != ptScrollNew)
      ScrollToPosition(ptScrollNew);
  } else
    ReleaseCapture();  // just in case
}

void CSage2020View::OnLButtonDown(UINT nFlags, CPoint point) {
  __super::OnLButtonDown(nFlags, point);
  if (!m_fLeftMouseDown) {
    CSage2020Doc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (pDoc == NULL)
      return;

    int yMouse = GetDeviceScrollPosition().y + point.y;
    int i = yMouse / m_sizChar.cy;
    if (i < 0)
      return;
    int cLine = static_cast<int>(pDoc->GetFileVersionInstanceSize());
    if (i >= cLine)
      return;  // clicked outside of content

    m_fLeftMouseDown = true;
    SetCapture();
    if (m_iSelStart != i || m_iSelEnd != i || m_iMouseDown != i) {
      if (m_iSelStart != -1 && (nFlags & MK_SHIFT)) {
        if (i > m_iSelStart)
          m_iSelEnd = i;
        else {
          m_iSelEnd = m_iSelStart;
          m_iSelStart = i;
        }
      } else {
        m_iSelStart = m_iSelEnd = i;
        m_ptSearchLast.x = -1;
        m_ptSearchLast.y = i;
      }
      m_iMouseDown = i;
      Invalidate(FALSE /*bErase*/);
    }
  }
}
void CSage2020View::OnLButtonUp(UINT nFlags, CPoint point) {
  __super::OnLButtonUp(nFlags, point);
  if (m_fLeftMouseDown) {
    m_fLeftMouseDown = false;
    ::ReleaseCapture();
  }
}

void CSage2020View::OnRButtonUp(UINT nFlags, CPoint point) {
  ClientToScreen(&point);
  OnContextMenu(this, point);
}

void CSage2020View::OnSize(UINT nType, int cx, int cy) {
  UpdateScrollSizes(cy);
}

void CSage2020View::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
  switch (nChar) {
      // dy
    case VK_HOME:
      OnScroll(MAKEWORD(SB_ENDSCROLL, SB_TOP), 0, TRUE /*bDoScroll*/);
      break;
    case VK_END:
      OnScroll(MAKEWORD(SB_ENDSCROLL, SB_BOTTOM), 0, TRUE /*bDoScroll*/);
      break;
    case VK_DOWN:
      OnScroll(MAKEWORD(SB_ENDSCROLL, SB_LINEDOWN), 0, TRUE /*bDoScroll*/);
      break;
    case VK_UP:
      OnScroll(MAKEWORD(SB_ENDSCROLL, SB_LINEUP), 0, TRUE /*bDoScroll*/);
      break;
    case VK_PRIOR:
      OnScroll(MAKEWORD(SB_ENDSCROLL, SB_PAGEUP), 0, TRUE /*bDoScroll*/);
      break;
    case VK_NEXT:
      OnScroll(MAKEWORD(SB_ENDSCROLL, SB_PAGEDOWN), 0, TRUE /*bDoScroll*/);
      break;

      // dx
    case VK_LEFT:
      if (::GetAsyncKeyState(VK_CONTROL) < 0)
        OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL),
                 GetScrollPos(SB_HORZ) - m_sizAll.cx, TRUE /*bDoScroll*/);
      else
        OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL),
                 GetScrollPos(SB_HORZ) - m_sizChar.cx, TRUE /*bDoScroll*/);
      break;
    case VK_RIGHT:
      if (::GetAsyncKeyState(VK_CONTROL) < 0)
        OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL),
                 GetScrollPos(SB_HORZ) + m_sizAll.cx, TRUE /*bDoScroll*/);
      else
        OnScroll(MAKEWORD(SB_THUMBTRACK, SB_ENDSCROLL),
                 GetScrollPos(SB_HORZ) + m_sizChar.cx, TRUE /*bDoScroll*/);
      break;
  }
}

// CSage2020View printing

void CSage2020View::OnFilePrintPreview() {
#ifndef SHARED_HANDLERS
  AFXPrintPreview(this);
#endif
}

BOOL CSage2020View::OnPreparePrinting(CPrintInfo* pInfo) {
  // default preparation
  return DoPreparePrinting(pInfo);
}

void CSage2020View::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/) {
  // TODO: add extra initialization before printing
}

void CSage2020View::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/) {
  // TODO: add cleanup after printing
}

void CSage2020View::OnContextMenu(CWnd* /* pWnd */, CPoint point) {
#ifndef SHARED_HANDLERS
  theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x,
                                                point.y, this, TRUE);
#endif
}

// CSage2020View diagnostics

#ifdef _DEBUG
void CSage2020View::AssertValid() const {
  CScrollView::AssertValid();
}

void CSage2020View::Dump(CDumpContext& dc) const {
  CScrollView::Dump(dc);
}

CSage2020Doc* CSage2020View::GetDocument() const  // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSage2020Doc)));
  return (CSage2020Doc*)m_pDocument;
}
#endif  //_DEBUG

// CSage2020View message handlers

BOOL CSage2020View::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
  // Turn Ctrl+scroll wheel into a version change
  if (nFlags & MK_CONTROL) {
    CSage2020Doc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (pDoc != NULL) {
      auto file_version_instance = pDoc->GetFileVersionInstance();

      if (file_version_instance != nullptr) {
        const size_t diffs_applied =
            file_version_instance->GetCommitIndex() + 1;
        const auto& diffs = pDoc->GetFileDiffs();
        const size_t diffs_total = diffs.size();
        FileVersionInstanceEditor editor(*file_version_instance);
        if (zDelta < 0) {
          // Add Diff
          if (diffs_applied < diffs_total) {
            editor.AddDiff(diffs[diffs_total - diffs_applied - 1]);
            pDoc->UpdateAllViews(NULL);  // NULL - also update this view
          }
        } else if (zDelta > 0) {
          // Remove diff
          if (diffs_applied > 0) {
            editor.RemoveDiff(diffs[diffs_total - diffs_applied]);
            pDoc->UpdateAllViews(NULL);  // NULL - also update this view
          }
        }
        return TRUE;
      }
    }
  }

  return __super::OnMouseWheel(nFlags, zDelta, pt);
}
