// Sage2020Doc.cpp : implementation of the CSage2020Doc class
//
#include "pch.h"

#include "Sage2020Doc.h"

#include <propkey.h>
#include <codecvt>

#include "GitDiffReader.h"
#include "GitFileReader.h"
#include "MainFrm.h"
#include "PropertiesWnd.h"  // REVIEW: this is icky
#include "Resource.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview,
// thumbnail and search filter handlers and allows sharing of document code with
// that project.
#ifndef SHARED_HANDLERS
#include "Sage2020.h"
#endif
#include "Sage2020ViewDocListener.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "ChangeHistoryPane.h"
#include "FIleVersionInstanceEditor.h"

// CSage2020Doc

IMPLEMENT_DYNCREATE(CSage2020Doc, CDocument)

// clang-format off
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wunused-local-typedef\"")
BEGIN_MESSAGE_MAP(CSage2020Doc, CDocument)
	ON_UPDATE_COMMAND_UI(IDR_PROPERTIES_GRID,
		&CSage2020Doc::OnUpdatePropertiesPaneGrid)
	ON_UPDATE_COMMAND_UI(IDR_HISTORY_TREE, &CSage2020Doc::OnUpdateHistoryTree)
END_MESSAGE_MAP()
_Pragma("GCC diagnostic pop")

// CSage2020Doc construction/destruction

CSage2020Doc::CSage2020Doc() noexcept
    // clang-format on
    : m_pDocListenerHead(nullptr), m_fNewDoc(false) {}

CSage2020Doc::~CSage2020Doc() {}

BOOL CSage2020Doc::OnNewDocument() {
  if (!CDocument::OnNewDocument())
    return FALSE;

  // N.b. add reinitialization code here
  // (SDI documents will reuse this document)

  return TRUE;
}

// CSage2020Doc serialization

void CSage2020Doc::Serialize(CArchive& ar) {
  POSITION pos = GetFirstViewPosition();
  CView* pView = GetNextView(pos /*in/out*/);
  CWnd* pwndStatus = NULL;
  COutputWnd* pwndOutput = NULL;
  if (pView != NULL) {
    CFrameWnd* pParentFrame = pView->GetParentFrame();
    assert(pParentFrame != NULL);
    if (pParentFrame != NULL &&
        pParentFrame->IsKindOf(RUNTIME_CLASS(CMainFrame))) {
      CMainFrame* pMainFrame = static_cast<CMainFrame*>(pParentFrame);
      pwndStatus = pMainFrame != NULL ? &pMainFrame->GetStatusWnd() : NULL;
      pwndOutput = pMainFrame != NULL ? &pMainFrame->GetOutputWnd() : NULL;
    }
  }

  if (ar.IsStoring()) {
    // N.b. add storing code here
  } else {
    auto native_path = ar.GetFile()->GetFilePath();
    std::filesystem::path file_path = (PCWSTR)native_path;

    GitDiffReader git_diff_reader{file_path, std::string(), pwndOutput};
    if (git_diff_reader.GetDiffs().empty()) {
      if (pwndStatus != nullptr) {
        CString strStatus;
        if (strStatus.LoadString(IDS_ERROR_LOADING_FILE)) {
          pwndStatus->SetWindowText(strStatus);
        }
      }
      AfxThrowArchiveException(CArchiveException::genericException);
    }

    // Sythethesize FileVersionInstance from diffs, going from first diff
    // (the last recorded in the git log) forward.
    file_diffs_ = git_diff_reader.MoveDiffs();

    // REVIEW: Is this now necessary since adds will sometimes come from
    // the branch diff?
    if (file_diffs_.front().diff_tree_.old_files[0].action[0] != 'A') {
      // if the first commit is not an add, then get the file at that point.
      std::filesystem::path parent_path = file_path;

      std::string initial_file_id =
          file_diffs_.front().diff_tree_.new_file.hash_string;
      GitFileReader git_file_reader{file_path.parent_path(), initial_file_id,
                                    pwndOutput};
      file_version_instance_ = std::make_unique<FileVersionInstance>(
          GetRootFileDiffs(), std::move(git_file_reader.GetLines()),
          file_diffs_.front().commit_);
    } else {
      file_version_instance_ =
          std::make_unique<FileVersionInstance>(GetRootFileDiffs());
    }

    FileVersionInstanceEditor editor(file_diffs_, *file_version_instance_.get(),
                                     m_pDocListenerHead);

    editor.GoToIndex(static_cast<int>(file_diffs_.size() - 1));
  }

  __super::Serialize(ar);
}

void CSage2020Doc::AddDocListener(Sage2020ViewDocListener& listener) {
  m_pDocListenerHead = listener.LinkListener(m_pDocListenerHead);

  assert(m_pDocListenerHead == &listener);
}

void CSage2020Doc::RemoveDocListener(Sage2020ViewDocListener& listener) {
  m_pDocListenerHead = listener.UnlinkListener(m_pDocListenerHead);

  assert(m_pDocListenerHead != &listener);
}

void CSage2020Doc::OnUpdatePropertiesPaneGrid(CCmdUI* pCmdUI) {
  // Update document-based properties (e.g. currently displayed version)
  CWnd* pWndT = CWnd::FromHandlePermanent(*pCmdUI->m_pOther);
  ASSERT_VALID(pWndT);

  CMFCPropertyGridCtrl* pGrid = static_cast<CMFCPropertyGridCtrl*>(pWndT);
  ASSERT_VALID(pGrid);
  if (pGrid == nullptr)
    return;

  CMFCPropertyGridProperty* pPropVersionHeader = pGrid->GetProperty(0);
  ASSERT_VALID(pPropVersionHeader);
  if (pPropVersionHeader == nullptr)
    return;

  CMFCPropertyGridProperty* pPropVersion = pPropVersionHeader->GetSubItem(0);
  ASSERT_VALID(pPropVersion);
  if (pPropVersion == nullptr)
    return;

  auto file_version_instance = GetFileVersionInstance();
  if (file_version_instance != nullptr &&
      file_version_instance->GetBranchDiffs() != nullptr) {
    const auto diffs = file_version_instance->GetBranchDiffs();
    int nVerMax = static_cast<int>(diffs->size());
    if (nVerMax != 0) {
      pPropVersion->EnableSpinControl(TRUE, 0, nVerMax - 1);
      pPropVersion->Enable(TRUE);
    } else {
      pPropVersion->EnableSpinControl(FALSE, 0 /*nMin*/, 0 /*nMax*/);
      pPropVersion->Enable(FALSE);
    }

    if (pPropVersion->GetValue().iVal !=
        file_version_instance->GetCommitIndex()) {
      if (pPropVersion->IsModified()) {
        CWaitCursor wait;
        FileVersionInstanceEditor editor(
            GetRootFileDiffs(), *file_version_instance, m_pDocListenerHead);
        if (editor.GoToIndex(pPropVersion->GetValue().iVal)) {
          pPropVersion->SetOriginalValue(COleVariant(
              (long)file_version_instance->GetCommitIndex(), VT_I4));
          pPropVersion->ResetOriginalValue();  // clears modified flag
          UpdateAllViews(NULL);
        }
      }

      pPropVersion->SetValue(
          COleVariant((long)file_version_instance->GetCommitIndex(), VT_I4));
      // Setting the orignal value allows IsModified() to work correctly.
      pPropVersion->SetOriginalValue(
          COleVariant((long)file_version_instance->GetCommitIndex(), VT_I4));

      CPropertiesWnd::UpdateGridBlock(
          file_version_instance->GetCommitIndex(),
          file_version_instance->GetCommitIndex() != -1
              ? &(*diffs)[file_version_instance->GetCommitIndex()]
              : nullptr,
          pPropVersionHeader, nVerMax, false /*fUpdateVersionConttrol*/);
    }
  } else {
    pPropVersion->EnableSpinControl(FALSE);
    pPropVersion->Enable(FALSE);
  }
}

void CSage2020Doc::OnUpdateHistoryTree(CCmdUI* pCmdUI) {
  // Update selection-based properties (e.g. currently displayed change)
  CWnd* pWndT = CWnd::FromHandlePermanent(*pCmdUI->m_pOther);
  ASSERT_VALID(pWndT);

  CTreeCtrl* pTree = static_cast<CTreeCtrl*>(pWndT);
  ASSERT_VALID(pTree);
  if (pTree == NULL)
    return;

  pCmdUI->m_bContinueRouting = TRUE;  // ensure that we route to doc

  if (m_fNewDoc) {
    pTree->DeleteAllItems();
    m_fNewDoc = false;
  }

  if (GetFileVersionInstance() == nullptr)
    return;

  const auto& commit = GetFileVersionInstance()->GetCommit();
  const auto& root_file_diffs = GetRootFileDiffs();
  if (CChangeHistoryPane::FEnsureTreeItemsAndSelection(
          *pTree, pTree->GetRootItem(), root_file_diffs, commit)) {
    HTREEITEM htreeitemSelected = pTree->GetSelectedItem();
    if (htreeitemSelected != NULL) {
      FileVersionDiff* selected_file_diff = reinterpret_cast<FileVersionDiff*>(
          pTree->GetItemData(htreeitemSelected));
      assert(selected_file_diff != NULL);
      if (selected_file_diff) {
        if (commit != selected_file_diff->commit_) {
          FileVersionInstanceEditor editor(GetRootFileDiffs(),
                                           *file_version_instance_.get(),
                                           m_pDocListenerHead);
          CWaitCursor wait;

          if (editor.GoToCommit(selected_file_diff->commit_))
            UpdateAllViews(NULL);
        }
      }
    }
  }
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CSage2020Doc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds) {
  // Modify this code to draw the document's data
  dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

  CString strText = _T("TODO: implement thumbnail drawing here");
  LOGFONT lf;

  CFont* pDefaultGUIFont =
      CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
  pDefaultGUIFont->GetLogFont(&lf);
  lf.lfHeight = 36;

  CFont fontDraw;
  fontDraw.CreateFontIndirect(&lf);

  CFont* pOldFont = dc.SelectObject(&fontDraw);
  dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
  dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CSage2020Doc::InitializeSearchContent() {
  CString strSearchContent;
  // Set search contents from document's data.
  // The content parts should be separated by ";"

  // For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
  SetSearchContent(strSearchContent);
}

void CSage2020Doc::SetSearchContent(const CString& value) {
  if (value.IsEmpty()) {
    RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
  } else {
    CMFCFilterChunkValueImpl* pChunk = nullptr;
    ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
    if (pChunk != nullptr) {
      pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
      SetChunkValue(pChunk);
    }
  }
}

#endif  // SHARED_HANDLERS

// CSage2020Doc diagnostics

#ifdef _DEBUG
void CSage2020Doc::AssertValid() const {
  CDocument::AssertValid();
}

void CSage2020Doc::Dump(CDumpContext& dc) const {
  CDocument::Dump(dc);
}
#endif  //_DEBUG

// CSage2020Doc commands

BOOL CSage2020Doc::OnOpenDocument(LPCTSTR lpszPathName) {
  // N.b. This must be done before |CDocument::OnOpenDocument| to reset any
  // current selection.
  if (m_pDocListenerHead) {
    m_pDocListenerHead->NotifyAllListenersOfVersionChange({GetRootFileDiffs()});
  }

  if (!CDocument::OnOpenDocument(lpszPathName))
    return FALSE;

  m_fNewDoc = true;  // HACK (Consider adding Sage2020ViewDocListener to
  // CChangeHistoryPane).

  viewport_origin() = CPoint();

  return TRUE;
}
