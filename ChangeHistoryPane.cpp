#include "pch.h"

#include "ChangeHistoryPane.h"

#include <cassert>
#include <iomanip>
#include <sstream>
#include <vector>

#include <afxtooltipmanager.h>

#include "FileVersionDiff.h"
#include "FileVersionInstance.h"
#include "GitDiffReader.h"
#include "MainFrm.h"
#include "Resource.h"
#include "Sage2020.h"
#include "Utility.h"

// clang-format off
BEGIN_MESSAGE_MAP(CChangeHistoryPane, CDockablePane)
	ON_WM_ACTIVATE()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_NOTIFY(TVN_ITEMEXPANDING, IDR_HISTORY_TREE, &CChangeHistoryPane::OnTreeNotifyExpanding)
	// ON_NOTIFY(TVN_DELETEITEM, IDR_HISTORY_TREE, OnTreeDeleteItem)
END_MESSAGE_MAP()
// clang-format on

CChangeHistoryPane::CChangeHistoryPane() : m_pToolTipControl(NULL) {}

CChangeHistoryPane::~CChangeHistoryPane() {
  if (m_pToolTipControl)
    CTooltipManager::DeleteToolTip(m_pToolTipControl);
}

void CChangeHistoryPane::OnActivate(UINT nState,
                                    CWnd* pWndOther,
                                    BOOL bMinimized) {
  CDockablePane::OnActivate(nState, pWndOther, bMinimized);
  // N.B. Add any message handler code here
}

int CChangeHistoryPane::OnCreate(LPCREATESTRUCT lpCreateStruct) {
  if (CDockablePane::OnCreate(lpCreateStruct) == -1)
    return -1;

  CRect rectDummy;
  rectDummy.SetRectEmpty();

  if (!m_wndTreeCtrl.Create(WS_VISIBLE | WS_CHILD | TVS_SHOWSELALWAYS |
                                TVS_FULLROWSELECT | TVS_HASBUTTONS |
                                TVS_LINESATROOT | TVS_NOTOOLTIPS,
                            rectDummy, this, IDR_HISTORY_TREE)) {
    TRACE0("Failed to create Versions Tree \n");
    return -1;  // fail to create
  }

  CTooltipManager::CreateToolTip(m_pToolTipControl /*ref*/, this,
                                 AFX_TOOLTIP_TYPE_DEFAULT);
  if (m_pToolTipControl != NULL) {
    m_pToolTipControl->SetMaxTipWidth(1024);  // make multi-line
    m_wndTreeCtrl.SetToolTips(m_pToolTipControl);
  }

  InitPropList();

  AdjustLayout();
  return 0;
}

void CChangeHistoryPane::OnSize(UINT nType, int cx, int cy) {
  CDockablePane::OnSize(nType, cx, cy);

  AdjustLayout();
}

void CChangeHistoryPane::AdjustLayout() {
  if (GetSafeHwnd() == NULL) {
    return;
  }

  CRect rectClient;
  GetClientRect(rectClient);

  m_wndTreeCtrl.SetWindowPos(NULL, rectClient.left, rectClient.top,
                             rectClient.Width(), rectClient.Height(),
                             SWP_NOACTIVATE | SWP_NOZORDER);
}

void CChangeHistoryPane::InitPropList() {
  SetPropListFont();

  if (m_imageList == NULL) {
    VERIFY(m_imageList.Create(
        theApp.m_bHiColorIcons ? IDB_HISTORY_TREE_HC : IDB_HISTORY_TREE, 16,
        1 /*nGrow*/, RGB(255, 0, 255)));
    VERIFY(m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL) ==
           NULL /*no prev image list*/);
  }
}

void CChangeHistoryPane::SetPropListFont() {
  ::DeleteObject(m_fntTreeCtrl.Detach());

  LOGFONT lf;
  afxGlobalData.fontRegular.GetLogFont(&lf);

  NONCLIENTMETRICS info{sizeof(info)};

  afxGlobalData.GetNonClientMetrics(info);

  lf.lfHeight = info.lfMenuFont.lfHeight;
  lf.lfWeight = info.lfMenuFont.lfWeight;
  lf.lfItalic = info.lfMenuFont.lfItalic;

  m_fntTreeCtrl.CreateFontIndirect(&lf);

  m_wndTreeCtrl.SetFont(&m_fntTreeCtrl);
}

void CChangeHistoryPane::OnSetFocus(CWnd* pOldWnd) {
  CDockablePane::OnSetFocus(pOldWnd);

  m_wndTreeCtrl.SetFocus();
}

void CChangeHistoryPane::OnTreeNotifyExpanding(NMHDR* pNMHDR,
                                               LRESULT* plResult) {
  const NMTREEVIEW* pTreeView = reinterpret_cast<const NMTREEVIEW*>(pNMHDR);

  if (pTreeView->action != TVE_EXPAND)
    return;

  HTREEITEM htreeitem = pTreeView->itemNew.hItem;

  auto file_version_diff = reinterpret_cast<const FileVersionDiff*>(
      m_wndTreeCtrl.GetItemData(htreeitem));
  if (file_version_diff == NULL) {
    assert(false);
    return;
  }
  if (file_version_diff->parents_.size() < 2) {
    assert(false);
    return;
  }
  COutputWnd* pwndOutput = NULL;
  CFrameWnd* pParentFrame = GetParentFrame();
  assert(pParentFrame != NULL);
  if (pParentFrame != NULL &&
      pParentFrame->IsKindOf(RUNTIME_CLASS(CMainFrame))) {
    CMainFrame* pMainFrame = static_cast<CMainFrame*>(pParentFrame);
    pwndOutput = pMainFrame != NULL ? &pMainFrame->GetOutputWnd() : NULL;
  }

  // TODO: This needs to be upgraded to handle > 2 parents.
  auto& file_version_diff_parent = file_version_diff->parents_[1];

  std::string parent_revision_range =
      file_version_diff->parents_[0].commit_.sha_ + std::string("..") +
      file_version_diff_parent.commit_.sha_;
  GitDiffReader git_diff_reader{file_version_diff->path_, parent_revision_range,
                                pwndOutput};
  assert(!git_diff_reader.GetDiffs().empty());
  if (!git_diff_reader.GetDiffs().empty()) {
    file_version_diff_parent.file_version_diffs_ =
        std::make_unique<std::vector<FileVersionDiff>>(
            git_diff_reader.MoveDiffs());

    // Get *all* diffs for this branch such that then we add "^" in the next
    // command, something will be found.
    GitDiffReader git_diff_reader_all_commits{
        file_version_diff->path_, parent_revision_range + "^", pwndOutput,
        GitDiffReader::Opt::NO_FILTER_TO_FILE};

    if (!git_diff_reader_all_commits.GetDiffs().empty()) {
      // Find the common ancestor branch commit of this sub-branch.
      std::string common_parent_rev =
          git_diff_reader_all_commits.GetDiffs().front().commit_.sha_ +
          std::string("^");
      // N.b. The file might be renamed, so we need to re-get path_ here.
      GitDiffReader git_diff_reader_parent{
          git_diff_reader_all_commits.GetDiffs().front().path_,
          common_parent_rev, pwndOutput};
      if (!git_diff_reader_parent.GetDiffs().empty()) {
        assert(!file_version_diff_parent.file_version_diffs_->front()
                    .file_parent_commit_.IsValid());
        file_version_diff_parent.file_version_diffs_->front()
            .file_parent_commit_ =
            git_diff_reader_parent.GetDiffs().back().commit_;
        assert(file_version_diff_parent.file_version_diffs_->front()
                   .file_parent_commit_.IsValid());
      } else {
        // Branch has no upsteam commit (e.g. its creation has been merged
        // through from another branch). Do nothing, leaving
        // |file_parent_commit_| empty.
        assert(false);
      }
    }
  }
}

#if 0
void CChangeHistoryPane::OnTreeDeleteItem(NMHDR* pNMHDR, LRESULT* plResult) {
	const NMTREEVIEW* pTreeView = reinterpret_cast<const NMTREEVIEW*>(pNMHDR);
	CToolTipCtrl* ptooltip = m_wndTreeCtrl.GetToolTips();
	if (ptooltip != NULL) {
		HTREEITEM htreeitem = pTreeView->itemNew.hItem;

		auto file_version_diff = reinterpret_cast<const FileVersionDiff*>(
			m_wndTreeCtrl.GetItemData(htreeitem));

		CToolInfo tool_info;
		if (ptooltip->GetToolInfo(tool_info, &m_wndTreeCtrl,
			reinterpret_cast<UINT_PTR>(&file_version_diff))) {
			ptooltip->DelTool(&m_wndTreeCtrl,
				reinterpret_cast<UINT_PTR>(&file_version_diff));
		}
	}
}
#endif  // 0

#if 0
static void SetToolTip(CTreeCtrl& tree,
                       const FileVersionDiff& file_version_diff,
                       HTREEITEM htreeitem) {
  CToolTipCtrl* ptooltip = tree.GetToolTips();
  if (ptooltip != NULL) {
    TCHAR wzTipLimited[1024];  // must limit tips to this size
    wzTipLimited[0] = '\0';
    _tcsncpy_s(wzTipLimited, to_wstring(file_version_diff.comment_).c_str(),
               _countof(wzTipLimited) - 1);
    RECT rcItem;
    VERIFY(tree.GetItemRect(htreeitem, &rcItem, FALSE /*bTextOnly*/));

    CToolInfo tool_info;
    if (ptooltip->GetToolInfo(tool_info, &tree,
                              reinterpret_cast<UINT_PTR>(&file_version_diff))) {
      // Tool exists for this item; update its rectangle.
      tool_info.rect = rcItem;
      ptooltip->SetToolInfo(&tool_info);
    } else {
      ptooltip->AddTool(
          &tree, wzTipLimited, &rcItem,
          reinterpret_cast<UINT_PTR>(&file_version_diff) /*unique id*/);
    }
  }
}
#endif  // 0

static std::string CreateTreeItemLabel(size_t commit_index,
                                       const FileVersionDiff& file_diff) {
  char time_string[64];
  time_string[0] = '\0';
  if (asctime_s(time_string, &file_diff.author_.time_)) {
    // Error.
    time_string[0] = '\0';
  }
  std::string label = std::to_string(commit_index) + ' ' +
                      file_diff.author_.name_ + " <" +
                      file_diff.author_.email_ + "> " +
                      std::string(time_string) + ' ' + file_diff.commit_.tag_;
  return label;
}

enum VERSION_IMAGES {
  VERSION_IMAGELIST_PLAIN,
  VERSION_IMAGELIST_DELETE,
  VERSION_IMAGELIST_ADD,
  VERSION_IMAGELIST_INTEGRATE,
  VERSION_IMAGELIST_BRANCH,
  VERSION_IMAGELIST_COPY,
  VERSION_IMAGELIST_IGNORE,

  VERSION_IMAGELIST_MERGE_OVERLAY,
  VERSION_IMAGELIST_BLANK_OVERLAY,
  VERSION_IMAGELIST_EXCLAIM_OVERLAY,
};

static void SetTreeItemData(CTreeCtrl& tree,
                            HTREEITEM htreeitem,
                            const FileVersionDiff& file_version_diff,
                            const std::wstring& item_label) {
  VERIFY(tree.SetItemText(htreeitem, item_label.c_str()));
  VERIFY(tree.SetItemData(htreeitem,
                          reinterpret_cast<DWORD_PTR>(&file_version_diff)));

  switch (file_version_diff.diff_tree_.action[0]) {
    case 'A':  // add
    {
      VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_ADD,
                               VERSION_IMAGELIST_ADD));

      HTREEITEM htreeitemChild = NULL;
      while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
        tree.DeleteItem(htreeitemChild);

      if (file_version_diff.parents_.size() > 1) {
        if (!tree.ItemHasChildren(htreeitem))
          tree.InsertItem(_T("Dummy"), htreeitem);
      }
      break;
    }
#if 0
	case 'b':  // branch
	{
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_BRANCH,
			VERSION_IMAGELIST_BRANCH));

		if (!tree.ItemHasChildren(htreeitem))
			tree.InsertItem(_T("Dummy"), htreeitem);
		break;
	}
#endif
    case 'C':  // copy
      VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_COPY,
                               VERSION_IMAGELIST_INTEGRATE));
      break;
    case 'D':  // delete
    {
      VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_DELETE,
                               VERSION_IMAGELIST_DELETE));

      HTREEITEM htreeitemChild = NULL;
      while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
        tree.DeleteItem(htreeitemChild);
      break;
    }
    case 'M':  // modificion
    {
      VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_PLAIN,
                               VERSION_IMAGELIST_PLAIN));

      HTREEITEM htreeitemChild = NULL;
      while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
        tree.DeleteItem(htreeitemChild);
#if 1
      if (file_version_diff.parents_.size() > 1) {
        if (!tree.ItemHasChildren(htreeitem))
          tree.InsertItem(_T("Dummy"), htreeitem);
      }
#endif  // 0
      break;
    }
#if 0
	case 'i':
		if (dr.szAction[1] == 'n')  // integrate
			VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_INTEGRATE,
				VERSION_IMAGELIST_INTEGRATE));
		else {
			assert(dr.szAction[1] == 'g');  // ignore
			VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_INTEGRATE,
				VERSION_IMAGELIST_INTEGRATE));
		}
		break;
	case 'm':  // merge
		VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_INTEGRATE,
			VERSION_IMAGELIST_INTEGRATE));
		break;
#endif
    case 'R':  // Rename.
      VERIFY(tree.SetItemImage(
          htreeitem, VERSION_IMAGELIST_INTEGRATE,  // REVIEW: Better image?
          VERSION_IMAGELIST_INTEGRATE));
      break;

    default: {
      assert(false);
      VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_PLAIN,
                               VERSION_IMAGELIST_PLAIN));

      HTREEITEM htreeitemChild = NULL;
      while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
        tree.DeleteItem(htreeitemChild);
      break;
    }
  }

  // TODO: This doesn't work reliably since it requires a |rect| to display that
  // can move/scroll as the containing Pane is updated. Consder using
  // TVS_INFOTIP instead which will ask for an item. SetToolTip(tree,
  // file_version_diff, htreeitem);
}

// returns true if the current version has changed due to a tree selection
/*static*/ bool CChangeHistoryPane::FEnsureTreeItemsAndSelection(
    CTreeCtrl& tree,
    HTREEITEM htreeitemRoot,
    const std::vector<FileVersionDiff>& file_diffs,
    const GitHash& selected_commit) {
  BOOL fSelected = FALSE;
  const bool fTreeFocused = tree.GetFocus() == &tree;

  // Sync current items.
  size_t commit_index = 0;
  HTREEITEM htreeitem = htreeitemRoot;
  while (commit_index < file_diffs.size() && htreeitem != NULL) {
    const auto& file_diff = file_diffs[commit_index];
    const FileVersionDiff* item_file_diff =
        reinterpret_cast<const FileVersionDiff*>(tree.GetItemData(htreeitem));
    if (item_file_diff == NULL ||
        item_file_diff->commit_ != file_diff.commit_) {
      auto item_label =
          to_wstring(CreateTreeItemLabel(commit_index, file_diff));
      SetTreeItemData(tree, htreeitem, file_diff, item_label);

      if (file_diff.commit_ == selected_commit) {
        VERIFY(fSelected = tree.SelectItem(htreeitem));
      }
    } else {
      if (!fTreeFocused && file_diff.commit_ == selected_commit)
        VERIFY(fSelected = tree.SelectItem(htreeitem));
    }

    auto child_item = tree.GetChildItem(htreeitem);
    if (child_item != NULL) {
      for (auto& parent : file_diff.parents_) {
        if (parent.file_version_diffs_) {
          fSelected = !FEnsureTreeItemsAndSelection(
              tree, child_item, *parent.file_version_diffs_, selected_commit);
        }
      }
    }

    commit_index++;
    htreeitem = tree.GetNextItem(htreeitem, TVGN_NEXT);
  }

  // Add any new items.
  if (commit_index < file_diffs.size()) {
    HTREEITEM htreeitemParent = tree.GetParentItem(htreeitemRoot);
    do {
      const auto& file_diff = file_diffs[commit_index];
      auto item_label =
          to_wstring(CreateTreeItemLabel(commit_index, file_diff));
      HTREEITEM htreeitemNew = tree.InsertItem(
          item_label.c_str(), htreeitemParent);  // inserts at end
      if (htreeitemNew != NULL) {
        SetTreeItemData(tree, htreeitemNew, file_diff, item_label);

        if (file_diff.commit_ == selected_commit)
          VERIFY(fSelected = tree.SelectItem(htreeitemNew));
      } else {
        if (!fTreeFocused && file_diff.commit_ == selected_commit)
          VERIFY(fSelected = tree.SelectItem(htreeitem));
      }
      commit_index++;
    } while (commit_index < file_diffs.size());
  } else if (htreeitem != NULL) {
    do {
#ifdef _DEBUG
      TVITEM tvitem = {};
      tree.GetItem(&tvitem);
#endif  // _DEBUG
      VERIFY(tree.DeleteItem(htreeitem));

      htreeitem = tree.GetNextItem(htreeitem, TVGN_NEXT);
    } while (htreeitem != NULL);
  }
  return !fSelected;
}
