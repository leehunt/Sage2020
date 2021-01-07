
#include "framework.h"
#include "pch.h"

#include <cassert>
#include "FileVersionDiff.h"
#include "MainFrm.h"
#include "PropertiesWnd.h"
#include "Resource.h"
#include "Sage2020.h"
#include "Utility.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// a subclass of CMFCPropertyGridProperty that fixes read-only copy behavior
class CMFCPropertyGridReadOnlySelectableProperty
    : public CMFCPropertyGridProperty {
  DECLARE_DYNAMIC(CMFCPropertyGridReadOnlySelectableProperty)

 public:
  // Simple property
  CMFCPropertyGridReadOnlySelectableProperty(const CString& strName,
                                             const COleVariant& varValue,
                                             LPCTSTR lpszDescr = NULL,
                                             DWORD_PTR dwData = 0,
                                             LPCTSTR lpszEditMask = NULL,
                                             LPCTSTR lpszEditTemplate = NULL,
                                             LPCTSTR lpszValidChars = NULL)
      : CMFCPropertyGridProperty(strName,
                                 varValue,
                                 lpszDescr,
                                 dwData,
                                 lpszEditMask,
                                 lpszEditTemplate,
                                 lpszValidChars) {}

 protected:
  virtual BOOL PushChar(UINT nChar)  // called on WM_KEYDOWN
  {
    // look for ctrl+c or clrl+ins
    if (!IsAllowEdit() && m_pWndList != NULL && m_pWndInPlace != NULL &&
        m_pWndInPlace->GetSafeHwnd() != NULL) {
      switch (nChar) {
        case VK_CONTROL:
          return TRUE;  // eat and don't destroy edit

        case _T('V'):
        case _T('X'): {
          if (::GetAsyncKeyState(VK_CONTROL) <
              0)          // Ctrl + 'V' --> Paste, Ctrl + 'X' --> Cut
            return TRUE;  // eat and don't destroy edit
        } break;

        case VK_DELETE: {
          if (::GetAsyncKeyState(VK_SHIFT) < 0)  // Shift + VK_INSERT --> Cut
            return TRUE;                         // eat and don't destroy edit
        } break;

        case VK_INSERT: {
          if (::GetAsyncKeyState(VK_SHIFT) < 0)  // Shift + VK_INSERT --> Paste
            return TRUE;                         // eat and don't destroy edit
          [[fallthrough]];
        }
        case _T('C'): {
          // sigh; this is copied from ProcessClipboardAccelerators() which is
          // protected
          BOOL bIsCtrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000);

          if (bIsCtrl)  // Ctrl + VK_INSERT, Ctrl + 'C' --> Copy
          {
            m_pWndInPlace->SendMessage(WM_COPY);
            return TRUE;  // don't destroy edit
          }
        } break;

        case _T('A'): {
          BOOL bIsCtrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000);

          if (bIsCtrl)  // Ctrl + 'A' --> Select All
          {
            m_pWndInPlace->SendMessage(EM_SETSEL, 0, -1);
            return TRUE;  // don't destroy edit
          }
        } break;
      }
    }

    return __super::PushChar(nChar);
  }
};

IMPLEMENT_DYNAMIC(CMFCPropertyGridReadOnlySelectableProperty,
                  CMFCPropertyGridProperty)

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CPropertiesWnd::CPropertiesWnd() noexcept {
  m_nComboHeight = 0;
}

CPropertiesWnd::~CPropertiesWnd() {}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
ON_WM_CREATE()
ON_WM_SIZE()
ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
ON_COMMAND(ID_PROPERTIES1, OnProperties1)
ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
ON_COMMAND(ID_PROPERTIES2, OnProperties2)
ON_UPDATE_COMMAND_UI(ID_PROPERTIES2, OnUpdateProperties2)
ON_WM_SETFOCUS()
ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CPropertiesWnd::AdjustLayout() {
  if (GetSafeHwnd() == nullptr ||
      (AfxGetMainWnd() != nullptr && AfxGetMainWnd()->IsIconic())) {
    return;
  }

  CRect rectClient;
  GetClientRect(rectClient);

  int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

  // m_wndObjectCombo.SetWindowPos(nullptr, rectClient.left, rectClient.top,
  // rectClient.Width(), m_nComboHeight, SWP_NOACTIVATE | SWP_NOZORDER);
  m_wndToolBar.SetWindowPos(nullptr, rectClient.left,
                            rectClient.top + m_nComboHeight, rectClient.Width(),
                            cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
  m_wndPropList.SetWindowPos(
      nullptr, rectClient.left, rectClient.top + m_nComboHeight + cyTlb,
      rectClient.Width(), rectClient.Height() - (m_nComboHeight + cyTlb),
      SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) {
  if (CDockablePane::OnCreate(lpCreateStruct) == -1)
    return -1;

  CRect rectDummy;
  rectDummy.SetRectEmpty();

#if 0
	// Create combo:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndObjectCombo.Create(dwViewStyle, rectDummy, this, 1))
	{
		TRACE0("Failed to create Properties Combo \n");
		return -1;      // fail to create
	}

	m_wndObjectCombo.AddString(_T("Application"));
	m_wndObjectCombo.AddString(_T("Properties Window"));
	m_wndObjectCombo.SetCurSel(0);

	CRect rectCombo;
	m_wndObjectCombo.GetClientRect (&rectCombo);

	m_nComboHeight = rectCombo.Height();
#endif  // 0

  if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this,
                            IDR_PROPERTIES_GRID)) {
    TRACE0("Failed to create Properties Grid \n");
    return -1;  // fail to create
  }

  InitPropList();

  m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
  m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* Is locked */);
  m_wndToolBar.CleanUpLockedImages();
  m_wndToolBar.LoadBitmap(
      theApp.m_bHiColorIcons ? IDB_PROPERTIES_HC : IDR_PROPERTIES, 0, 0,
      TRUE /* Locked */);

  m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS |
                            CBRS_FLYBY);
  m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() &
                            ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC |
                              CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM |
                              CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
  m_wndToolBar.SetOwner(this);

  // All commands will be routed via this control , not via the parent frame:
  m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

  AdjustLayout();
  return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy) {
  CDockablePane::OnSize(nType, cx, cy);
  AdjustLayout();
}

void CPropertiesWnd::OnExpandAllProperties() {
  m_wndPropList.ExpandAll();
}

void CPropertiesWnd::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */) {}

void CPropertiesWnd::OnSortProperties() {
  m_wndPropList.SetAlphabeticMode(!m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnUpdateSortProperties(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnProperties1() {
  // TODO: Add your command handler code here
}

void CPropertiesWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/) {
  // TODO: Add your command update UI handler code here
}

void CPropertiesWnd::OnProperties2() {
  // TODO: Add your command handler code here
}

void CPropertiesWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/) {
  // TODO: Add your command update UI handler code here
}

void CPropertiesWnd::InitPropList() {
  SetPropListFont();

  m_wndPropList.EnableHeaderCtrl(FALSE);
  m_wndPropList.EnableDescriptionArea();
  m_wndPropList.SetVSDotNetLook();
  m_wndPropList.MarkModifiedProperties();

#if 0
	CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Appearance"));

	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("3D Look"), (_variant_t) false, _T("Specifies the window's font will be non-bold and controls will have a 3D border")));

	CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty(_T("Border"), _T("Dialog Frame"), _T("One of: None, Thin, Resizable, or Dialog Frame"));
	pProp->AddOption(_T("None"));
	pProp->AddOption(_T("Thin"));
	pProp->AddOption(_T("Resizable"));
	pProp->AddOption(_T("Dialog Frame"));
	pProp->AllowEdit(FALSE);

	pGroup1->AddSubItem(pProp);
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("Caption"), (_variant_t) _T("About"), _T("Specifies the text that will be displayed in the window's title bar")));

	m_wndPropList.AddProperty(pGroup1);

	CMFCPropertyGridProperty* pSize = new CMFCPropertyGridProperty(_T("Window Size"), 0, TRUE);

	pProp = new CMFCPropertyGridProperty(_T("Height"), (_variant_t) 250l, _T("Specifies the window's height"));
	pProp->EnableSpinControl(TRUE, 50, 300);
	pSize->AddSubItem(pProp);

	pProp = new CMFCPropertyGridProperty( _T("Width"), (_variant_t) 150l, _T("Specifies the window's width"));
	pProp->EnableSpinControl(TRUE, 50, 200);
	pSize->AddSubItem(pProp);

	m_wndPropList.AddProperty(pSize);

	CMFCPropertyGridProperty* pGroup2 = new CMFCPropertyGridProperty(_T("Font"));

	LOGFONT lf;
	CFont* font = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	font->GetLogFont(&lf);

	_tcscpy_s(lf.lfFaceName, _T("Arial"));

	pGroup2->AddSubItem(new CMFCPropertyGridFontProperty(_T("Font"), lf, CF_EFFECTS | CF_SCREENFONTS, _T("Specifies the default font for the window")));
	pGroup2->AddSubItem(new CMFCPropertyGridProperty(_T("Use System Font"), (_variant_t) true, _T("Specifies that the window uses MS Shell Dlg font")));

	m_wndPropList.AddProperty(pGroup2);

	CMFCPropertyGridProperty* pGroup3 = new CMFCPropertyGridProperty(_T("Misc"));
	pProp = new CMFCPropertyGridProperty(_T("(Name)"), _T("Application"));
	pProp->Enable(FALSE);
	pGroup3->AddSubItem(pProp);

	CMFCPropertyGridColorProperty* pColorProp = new CMFCPropertyGridColorProperty(_T("Window Color"), RGB(210, 192, 254), nullptr, _T("Specifies the default window color"));
	pColorProp->EnableOtherButton(_T("Other..."));
	pColorProp->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
	pGroup3->AddSubItem(pColorProp);

	static const TCHAR szFilter[] = _T("Icon Files(*.ico)|*.ico|All Files(*.*)|*.*||");
	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Icon"), TRUE, _T(""), _T("ico"), 0, szFilter, _T("Specifies the window icon")));

	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Folder"), _T("c:\\")));

	m_wndPropList.AddProperty(pGroup3);

	CMFCPropertyGridProperty* pGroup4 = new CMFCPropertyGridProperty(_T("Hierarchy"));

	CMFCPropertyGridProperty* pGroup41 = new CMFCPropertyGridProperty(_T("First sub-level"));
	pGroup4->AddSubItem(pGroup41);

	CMFCPropertyGridProperty* pGroup411 = new CMFCPropertyGridProperty(_T("Second sub-level"));
	pGroup41->AddSubItem(pGroup411);

	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 1"), (_variant_t) _T("Value 1"), _T("This is a description")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 2"), (_variant_t) _T("Value 2"), _T("This is a description")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 3"), (_variant_t) _T("Value 3"), _T("This is a description")));

	pGroup4->Expand(FALSE);
	m_wndPropList.AddProperty(pGroup4);
#else
  CMFCPropertyGridProperty* pGroup0 = new CMFCPropertyGridProperty(
      _T("Current commit index (click to change)"));
  CMFCPropertyGridProperty* pPropertyGridPropertyT =
      new CMFCPropertyGridProperty(
          _T("Current commit"), COleVariant((long)0, VT_I4),
          _T("The currently displayed commit (Ctrl+Scroll wheel ")
          _T("to change)"));
  pPropertyGridPropertyT->EnableSpinControl(TRUE, 0, 0);
  pPropertyGridPropertyT->Enable(FALSE);
  pGroup0->AddSubItem(pPropertyGridPropertyT);

  pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
      _T("commit"), (_variant_t) _T(""), _T("The commit"));
  pPropertyGridPropertyT->AllowEdit(FALSE);
  pGroup0->AddSubItem(pPropertyGridPropertyT);

  pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
      _T("Date"), (_variant_t) _T(""),
      _T("The modification date and time of the commit"));
  pPropertyGridPropertyT->AllowEdit(FALSE);
  pGroup0->AddSubItem(pPropertyGridPropertyT);

  pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
      _T("Author"), (_variant_t) _T(""), _T("The author of the commit"));
  pPropertyGridPropertyT->AllowEdit(FALSE);
  pGroup0->AddSubItem(pPropertyGridPropertyT);

  pPropertyGridPropertyT =
      new CMFCPropertyGridColorProperty(_T("Color"), RGB(0xFF, 0xFF, 0xFF));
  pPropertyGridPropertyT->AllowEdit(FALSE);
  pPropertyGridPropertyT->Enable(FALSE);
  pGroup0->AddSubItem(pPropertyGridPropertyT);

  m_wndPropList.AddProperty(pGroup0);
#endif
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd) {
  CDockablePane::OnSetFocus(pOldWnd);
  m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection) {
  CDockablePane::OnSettingChange(uFlags, lpszSection);
  SetPropListFont();
}

void CPropertiesWnd::SetPropListFont() {
  ::DeleteObject(m_fntPropList.Detach());

  LOGFONT lf;
  afxGlobalData.fontRegular.GetLogFont(&lf);

  NONCLIENTMETRICS info;
  info.cbSize = sizeof(info);

  afxGlobalData.GetNonClientMetrics(info);

  lf.lfHeight = info.lfMenuFont.lfHeight;
  lf.lfWeight = info.lfMenuFont.lfWeight;
  lf.lfItalic = info.lfMenuFont.lfItalic;

  m_fntPropList.CreateFontIndirect(&lf);

  m_wndPropList.SetFont(&m_fntPropList);
  // m_wndObjectCombo.SetFont(&m_fntPropList);
}

// static
void CPropertiesWnd::EnsureItems(CMFCPropertyGridCtrl& wndPropList, int cItem) {
  CMFCPropertyGridProperty* pProp;
  while (wndPropList.GetPropertyCount() > cItem + 1 &&
         (pProp = wndPropList.GetProperty(cItem + 1)) != NULL) {
    // MFC has an annoying bug where if the selection is in a subitem
    // of an property that is deleted, it will crash.  The workaround
    // is to remove selection if the subitem is a child of the property
    CMFCPropertyGridProperty* pSel = wndPropList.GetCurSel();
    if (pSel != NULL) {
      while ((pSel = pSel->GetParent()) != NULL) {
        if (pSel == pProp) {
          wndPropList.SetCurSel(NULL);
          break;
        }
      }
    }
    VERIFY(wndPropList.DeleteProperty(pProp));
    assert(pProp == NULL);
  }

  for (int i = wndPropList.GetPropertyCount() - 1; i < cItem; ++i) {
    CMFCPropertyGridProperty* pGroup1 =
        new CMFCPropertyGridProperty(_T("Selected change description"));

    CMFCPropertyGridProperty* pPropertyGridPropertyT =
        new CMFCPropertyGridReadOnlySelectableProperty(
            _T("Version"), (_variant_t) _T(""),
            _T("The version that is currently selected"));
    pPropertyGridPropertyT->AllowEdit(FALSE);
    pGroup1->AddSubItem(pPropertyGridPropertyT);

    pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
        _T("Changelist"), (_variant_t) _T(""),
        _T("The changelist that is currently selected"));
    pPropertyGridPropertyT->AllowEdit(FALSE);
    pGroup1->AddSubItem(pPropertyGridPropertyT);

    pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
        _T("Date"), (_variant_t) _T(""),
        _T("The modification date and time of the change that is currently ")
        _T("selected"));
    pPropertyGridPropertyT->AllowEdit(FALSE);
    pGroup1->AddSubItem(pPropertyGridPropertyT);

    pPropertyGridPropertyT = new CMFCPropertyGridReadOnlySelectableProperty(
        _T("Author"), (_variant_t) _T(""),
        _T("The author of the change that is currently selected"));
    pPropertyGridPropertyT->AllowEdit(FALSE);
    pGroup1->AddSubItem(pPropertyGridPropertyT);

    pPropertyGridPropertyT =
        new CMFCPropertyGridColorProperty(_T("Color"), RGB(0xFF, 0xFF, 0xFF));
    pPropertyGridPropertyT->AllowEdit(FALSE);
    pPropertyGridPropertyT->Enable(FALSE);
    pGroup1->AddSubItem(pPropertyGridPropertyT);

    wndPropList.AddProperty(pGroup1);
  }
}

// static
void CPropertiesWnd::UpdateGridBlock(
    size_t commit_index,
    const FileVersionDiff* file_version_diff,
    CMFCPropertyGridProperty* pPropVersionHeader,
    int nVerMax,
    bool fUpdateVersionControl) {
  CMFCPropertyGridProperty* pPropVersionVersion =
      pPropVersionHeader->GetSubItem(0);
  ASSERT_VALID(pPropVersionVersion);
  if (fUpdateVersionControl && pPropVersionVersion != NULL) {
    WCHAR wzVer[32];
    _itow_s(static_cast<int>(commit_index), wzVer, _countof(wzVer), 10);
    _variant_t varWzNversT(wzVer);
    if (!(pPropVersionVersion->GetValue() == varWzNversT))
      pPropVersionVersion->SetValue(varWzNversT);
  }

  std::wstring change_list;
  if (file_version_diff != NULL)
    change_list = to_wstring(file_version_diff->commit_);
  CMFCPropertyGridProperty* pPropVersionCL = pPropVersionHeader->GetSubItem(1);
  ASSERT_VALID(pPropVersionCL);
  if (pPropVersionCL != NULL) {
    _variant_t varWzChangeListT(change_list.c_str());
    if (!(pPropVersionCL->GetValue() == varWzChangeListT))
      pPropVersionCL->SetValue(varWzChangeListT);
  }

#if 0
  std::wstring date_time;
  if (file_version_diff != NULL)
    date_time = to_wstring(
    _tasctime_s(wzDateTime, _countof(wzDateTime), &pdrVer->tm);
  else
    wzDateTime[0] = '\0';
  CMFCPropertyGridProperty* pPropVersionDateTime =
      pPropVersionHeader->GetSubItem(2);
  ASSERT_VALID(pPropVersionDateTime);
  if (pPropVersionDateTime != NULL) {
    _variant_t varWzDateTimeT(wzDateTime);
    if (!(pPropVersionDateTime->GetValue() == varWzDateTimeT))
      pPropVersionDateTime->SetValue(varWzDateTimeT);
  }

  CMFCPropertyGridProperty* pPropVersionAuthor =
      pPropVersionHeader->GetSubItem(3);
  ASSERT_VALID(pPropVersionAuthor);
  if (pPropVersionAuthor != NULL) {
    _variant_t varWzAuthorT(pdrVer != NULL ? pdrVer->szAuthor : _T(""));
    if (!(pPropVersionAuthor->GetValue() == varWzAuthorT))
      pPropVersionAuthor->SetValue(varWzAuthorT);
  }

  CMFCPropertyGridProperty* pPropVersionColorT =
      pPropVersionHeader->GetSubItem(4);
  CMFCPropertyGridColorProperty* pPropVersionColor =
      static_cast<CMFCPropertyGridColorProperty*>(pPropVersionColorT);
  ASSERT_VALID(pPropVersionColor);
  if (pPropVersionColor != NULL) {
    COLORREF crVersion(pdrVer != NULL
                           ? CSimpleAgeViewerView::CrBackgroundForVersion(
                                 pdrVer->nVer, nVerMax)
                           : RGB(0xFF, 0xFF, 0xFF));
    if (!(pPropVersionColor->GetColor() == crVersion))
      pPropVersionColor->SetColor(crVersion);
  }
#endif  // 0
}
