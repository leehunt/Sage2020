
// Sage2020.cpp : Defines the class behaviors for the application.
//

#include "pch.h"

#include "MainFrm.h"
#include "Sage2020.h"
#include "afxdialogex.h"
#include "afxwinappex.h"
#include "framework.h"

#include "Sage2020Doc.h"
#include "Sage2020View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSage2020App

BEGIN_MESSAGE_MAP(CSage2020App, CWinAppEx)
ON_COMMAND(ID_APP_ABOUT, &CSage2020App::OnAppAbout)
// Standard file based document commands
ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
// Standard print setup command
ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()

// CSage2020App construction

CSage2020App::CSage2020App() noexcept {
  m_bHiColorIcons = TRUE;

  // support Restart Manager
  m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
  // If the application is built using Common Language Runtime support (/clr):
  //     1) This additional setting is needed for Restart Manager support to
  //     work properly. 2) In your project, you must add a reference to
  //     System.Windows.Forms in order to build.
  System::Windows::Forms::Application::SetUnhandledExceptionMode(
      System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

  // TODO: replace application ID string below with unique ID string;
  // recommended format for string is
  // CompanyName.ProductName.SubProduct.VersionInformation
  SetAppID(_T("Sage2020.AppID.NoVersion"));

  // TODO: add construction code here,
  // Place all significant initialization in InitInstance
}

// The one and only CSage2020App object

CSage2020App theApp;

// CSage2020App initialization

BOOL CSage2020App::InitInstance() {
  // InitCommonControlsEx() is required on Windows XP if an application
  // manifest specifies use of ComCtl32.dll version 6 or later to enable
  // visual styles.  Otherwise, any window creation will fail.
  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof(InitCtrls);
  // Set this to include all the common control classes you want to use
  // in your application.
  InitCtrls.dwICC = ICC_WIN95_CLASSES | ICC_TREEVIEW_CLASSES | ICC_LINK_CLASS;
  InitCommonControlsEx(&InitCtrls);

  CWinAppEx::InitInstance();

  // Initialize OLE libraries
  if (!AfxOleInit()) {
    AfxMessageBox(IDP_OLE_INIT_FAILED);
    return FALSE;
  }

  AfxEnableControlContainer();

  EnableTaskbarInteraction(FALSE);

  // AfxInitRichEdit2() is required to use RichEdit control
  // AfxInitRichEdit2();

  // Standard initialization
  // If you are not using these features and wish to reduce the size
  // of your final executable, you should remove from the following
  // the specific initialization routines you do not need
  // Change the registry key under which our settings are stored
  SetRegistryKey(_T("LeeSoft"));
  LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

  InitContextMenuManager();
  InitShellManager();

  InitKeyboardManager();

  InitTooltipManager();
  CMFCToolTipInfo ttParams;
  ttParams.m_bVislManagerTheme = TRUE;
  theApp.GetTooltipManager()->SetTooltipParams(
      AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

  // Register the application's document templates.  Document templates
  //  serve as the connection between documents, frame windows and views
  CSingleDocTemplate* pDocTemplate;
  pDocTemplate = new CSingleDocTemplate(
      IDR_MAINFRAME, RUNTIME_CLASS(CSage2020Doc),
      RUNTIME_CLASS(CMainFrame),  // main SDI frame window
      RUNTIME_CLASS(CSage2020View));
  if (!pDocTemplate)
    return FALSE;
  AddDocTemplate(pDocTemplate);

  // Parse command line for standard shell commands, DDE, file open
  CCommandLineInfo cmdInfo;
  ParseCommandLine(cmdInfo);

  // Dispatch commands specified on the command line.  Will return FALSE if
  // app was launched with /RegServer, /Register, /Unregserver or /Unregister.
  if (!ProcessShellCommand(cmdInfo))
    return FALSE;

  // The one and only window has been initialized, so show and update it
  m_pMainWnd->ShowWindow(SW_SHOW);
  m_pMainWnd->UpdateWindow();
  return TRUE;
}

int CSage2020App::ExitInstance() {
  // TODO: handle additional resources you may have added
  AfxOleTerm(FALSE);

  return CWinAppEx::ExitInstance();
}

// CSage2020App message handlers

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx {
 public:
  CAboutDlg() noexcept;

// Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_ABOUTBOX };
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support

  // Implementation
 protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CSage2020App::OnAppAbout() {
  CAboutDlg aboutDlg;
  aboutDlg.DoModal();
}

// CSage2020App customization load/save methods

void CSage2020App::PreLoadState() {
  BOOL bNameValid;
  CString strName;
  bNameValid = strName.LoadString(IDS_EDIT_MENU);
  ASSERT(bNameValid);
  GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
  bNameValid = strName.LoadString(IDS_EXPLORER);
  ASSERT(bNameValid);
  GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CSage2020App::LoadCustomState() {}

void CSage2020App::SaveCustomState() {}

// CSage2020App message handlers
