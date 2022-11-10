
// MainFrm.cpp : implementation of the CMainFrame class
//
#include "pch.h"

#include "Sage2020.h"
#include "framework.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

const int iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

static UINT indicators[] = {
		ID_SEPARATOR,  // status line indicator
		ID_INDICATOR_CAPS,
		ID_INDICATOR_NUM,
		ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame() noexcept {
}

CMainFrame::~CMainFrame() {}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;

	if (!m_wndMenuBar.Create(this)) {
		TRACE0("Failed to create menubar\n");
		return -1;  // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC |
		CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT,
		WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER |
		CBRS_TOOLTIPS | CBRS_FLYBY |
		CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256
			: IDR_MAINFRAME)) {
		TRACE0("Failed to create toolbar\n");
		return -1;  // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(nullptr, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_wndStatusBar.Create(this)) {
		TRACE0("Failed to create status bar\n");
		return -1;  // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

	// N.b. Delete these five lines if you don't want the toolbar and menubar to
	// be dockable
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Navigation pane will be created at left, so temporary disable docking at
	// the left side:
	EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM | CBRS_ALIGN_RIGHT);

#if OUTLOOK_NAV_BAR
	// Create and setup "Outlook" navigation bar:
	if (!CreateOutlookBar(m_wndNavigationBar, ID_VIEW_NAVIGATION, m_wndTree,
		m_wndCalendar, 250)) {
		TRACE0("Failed to create navigation pane\n");
		return -1;  // fail to create
	}
#endif

	// Outlook bar is created and docking on the left side should be allowed.
	EnableDocking(CBRS_ALIGN_LEFT);
	EnableAutoHidePanes(CBRS_ALIGN_RIGHT);

	// Load menu item image (not placed on any standard toolbars):
	CMFCToolBar::AddToolBarForImageCollection(
		IDR_MENU_IMAGES,
		theApp.m_bHiColorIcons ? IDB_MENU_IMAGES_24 : IDB_MENU_IMAGES);

	// create docking windows
	if (!CreateDockingWindows()) {
		TRACE0("Failed to create docking windows\n");
		return -1;
	}

#if FILE_VIEW_UI
	m_wndFileView.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndFileView);
#endif
#if CLASS_VIEW_UI
	m_wndClassView.EnableDocking(CBRS_ALIGN_ANY);
#endif
#if FILE_VIEW_UI && CLASS_VIEW_UI
	CDockablePane* pTabbedBar = nullptr;
	m_wndClassView.AttachToTabWnd(&m_wndFileView, DM_SHOW, TRUE, &pTabbedBar);
#endif
#if OUTPUT_PANE
	m_wndOutput.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndOutput);
#endif
	m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndProperties);

	m_wndChangeHistoryPane.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndChangeHistoryPane);

	// this must happen *after* docking the pane
	m_wndProperties.DockToWindow(&m_wndChangeHistoryPane, CBRS_ALIGN_BOTTOM);

	// set the visual manager used to draw all user interface elements
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == nullptr) {
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp"))) {
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}

	// enable menu personalization (most-recently used commands)
	// FUTURE: define your own basic commands, ensuring that each pulldown menu has
	// at least one basic command.
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_PRINT);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_SORTING_SORTALPHABETIC);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYTYPE);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYACCESS);
	lstBasicCommands.AddTail(ID_SORTING_GROUPBYTYPE);

	CMFCToolBar::SetBasicCommands(lstBasicCommands);

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) {
	return m_wndSplitter.Create(this, 2 /*cRow*/, 1 /*cCol*/,
		CSize(10, 10) /*sizMinPane*/, pContext);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
	if (!CFrameWndEx::PreCreateWindow(cs))
		return FALSE;
	// N.b. Modify the Window class or styles here by modifying
	// the CREATESTRUCT cs

	return TRUE;
}

BOOL CMainFrame::CreateDockingWindows() {
	BOOL bNameValid;

#if CLASS_VIEW_UI
	// Create class view
	CString strClassView;
	bNameValid = strClassView.LoadString(IDS_CLASS_VIEW);
	ASSERT(bNameValid);
	if (!m_wndClassView.Create(
		strClassView, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_CLASSVIEW,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		CBRS_LEFT | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Class View window\n");
		return FALSE;  // failed to create
	}
#endif

#if FILE_VIEW_UI
	// Create file view
	CString strFileView;
	bNameValid = strFileView.LoadString(IDS_FILE_VIEW);
	ASSERT(bNameValid);
	if (!m_wndFileView.Create(
		strFileView, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_FILEVIEW,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		CBRS_LEFT | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create File View window\n");
		return FALSE;  // failed to create
	}
#endif

#if OUTPUT_PANE
	// Create output window
	CString strOutputWnd;
	bNameValid = strOutputWnd.LoadString(IDS_OUTPUT_WND);
	ASSERT(bNameValid);
	if (!m_wndOutput.Create(
		strOutputWnd, this, CRect(0, 0, 100, 100), TRUE, ID_VIEW_OUTPUTWND,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		CBRS_BOTTOM | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Output window\n");
		return FALSE;  // failed to create
	}
#endif

	// Create properties window
	CString strPropertiesWnd;
	bNameValid = strPropertiesWnd.LoadString(IDS_PROPERTIES_WND);
	ASSERT(bNameValid);
	if (!m_wndProperties.Create(strPropertiesWnd, this, CRect(0, 0, 200, 200),
		TRUE, ID_VIEW_PROPERTIESWND,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		WS_CLIPCHILDREN | CBRS_RIGHT |
		CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Properties window\n");
		return FALSE;  // failed to create
	}

	// Create the Change History window.
	CString strChangeHistoryWnd;
	bNameValid = strChangeHistoryWnd.LoadString(IDS_CHANGE_HISTORY_WND);
	ASSERT(bNameValid);
	if (!m_wndChangeHistoryPane.Create(
		strChangeHistoryWnd, this, CRect(0, 0, 200, 200), TRUE,
		ID_VIEW_CHANGE_HISTORY,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		CBRS_LEFT | CBRS_ALIGN_LEFT)) {
		TRACE0("Failed to create Change History window\n");
		return FALSE;  // failed to create
	}

	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons) {
#if FILE_VIEW_UI
	HICON hFileViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_FILE_VIEW_HC : IDI_FILE_VIEW),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndFileView.SetIcon(hFileViewIcon, FALSE);
#endif

#if CLASS_VIEW_UI
	HICON hClassViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_CLASS_VIEW_HC : IDI_CLASS_VIEW),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndClassView.SetIcon(hClassViewIcon, FALSE);
#endif

#if OUTPUT_PANE
	HICON hOutputBarIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_OUTPUT_WND_HC : IDI_OUTPUT_WND),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndOutput.SetIcon(hOutputBarIcon, FALSE);
#endif

	HICON hPropertiesBarIcon =
		(HICON)::LoadImage(::AfxGetResourceHandle(),
			MAKEINTRESOURCE(bHiColorIcons ? IDI_PROPERTIES_WND_HC
				: IDI_PROPERTIES_WND),
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON),
			::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndProperties.SetIcon(hPropertiesBarIcon, FALSE);

	HICON hChangeHistoryPaneIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_PROPERTIES_WND_HC
			: IDI_PROPERTIES_WND),  // Apparently not shown by panes.
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndChangeHistoryPane.SetIcon(hChangeHistoryPaneIcon, FALSE);
}

#if OUTLOOK_BAR
BOOL CMainFrame::CreateOutlookBar(CMFCOutlookBar& bar,
	UINT uiID,
	CMFCShellTreeCtrl& tree,
	CCalendarBar& calendar,
	int nInitialWidth) {
	bar.SetMode2003();

	BOOL bNameValid;
	CString strTemp;
	bNameValid = strTemp.LoadString(IDS_SHORTCUTS);
	ASSERT(bNameValid);
	if (!bar.Create(strTemp, this, CRect(0, 0, nInitialWidth, 32000), uiID,
		WS_CHILD | WS_VISIBLE | CBRS_LEFT)) {
		return FALSE;  // fail to create
	}

	CMFCOutlookBarTabCtrl* pOutlookBar =
		(CMFCOutlookBarTabCtrl*)bar.GetUnderlyingWindow();

	if (pOutlookBar == nullptr) {
		ASSERT(FALSE);
		return FALSE;
	}

	pOutlookBar->EnableInPlaceEdit(TRUE);

	static UINT uiPageID = 1;

	// can float, can autohide, can resize, CAN NOT CLOSE
	DWORD dwStyle = AFX_CBRS_FLOAT | AFX_CBRS_AUTOHIDE | AFX_CBRS_RESIZE;

	CRect rectDummy(0, 0, 0, 0);
	const DWORD dwTreeStyle =
		WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;

	tree.Create(dwTreeStyle, rectDummy, &bar, 1200);
	bNameValid = strTemp.LoadString(IDS_FOLDERS);
	ASSERT(bNameValid);
	pOutlookBar->AddControl(&tree, strTemp, 2, TRUE, dwStyle);

	calendar.Create(rectDummy, &bar, 1201);
	bNameValid = strTemp.LoadString(IDS_CALENDAR);
	ASSERT(bNameValid);
	pOutlookBar->AddControl(&calendar, strTemp, 3, TRUE, dwStyle);

	bar.SetPaneStyle(bar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY |
		CBRS_SIZE_DYNAMIC);

	pOutlookBar->SetImageList(theApp.m_bHiColorIcons ? IDB_PAGES_HC : IDB_PAGES,
		24);
	pOutlookBar->SetToolbarImageList(
		theApp.m_bHiColorIcons ? IDB_PAGES_SMALL_HC : IDB_PAGES_SMALL, 16);
	pOutlookBar->RecalcLayout();

	BOOL bAnimation = theApp.GetInt(_T("OutlookAnimation"), TRUE);
	CMFCOutlookBarTabCtrl::EnableAnimation(bAnimation);

	bar.SetButtonsFont(&afxGlobalData.fontBold);

	return TRUE;
}
#endif

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const {
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const {
	CFrameWndEx::Dump(dc);
}
#endif  //_DEBUG

// CMainFrame message handlers

void CMainFrame::OnViewCustomize() {
	CMFCToolBarsCustomizeDialog* pDlgCust =
		new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM lp) {
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp, lp);
	if (lres == 0) {
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

BOOL CMainFrame::LoadFrame(UINT nIDResource,
	DWORD dwDefaultStyle,
	CWnd* pParentWnd,
	CCreateContext* pContext) {
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd,
		pContext)) {
		return FALSE;
	}

	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i++) {
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != nullptr) {
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE,
				strCustomize);
		}
	}

	return TRUE;
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection) {
	CFrameWndEx::OnSettingChange(uFlags, lpszSection);
#if OUTPUT_PANE
	m_wndOutput.UpdateFonts();
#endif
}

BOOL CMainFrame::OnCmdMsg(UINT nID,
	int nCode,
	void* pExtra,
	AFX_CMDHANDLERINFO* pHandlerInfo) {
	// MFC is foobared when it comes to command routing for Panes.  They are not
	// considered in the routing sequence, instead they do:
	// 1. pump through current view FIRST
	// 2. then pump through frame
	// 3. last but not least, pump through app

	// Note the sad lack of Pane routing (which can have focus).

	CWnd* pFocus = GetFocus();
	if (pFocus && pFocus != this) {
		if (pFocus->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}

	return CFrameWndEx::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
