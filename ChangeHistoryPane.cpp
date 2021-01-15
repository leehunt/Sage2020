#include "pch.h"

#include <cassert>
#include <iomanip>
#include <sstream>
#include <vector>
#include "ChangeHistoryPane.h"
#include "FileVersionDiff.h"
#include "GitDiffReader.h"
#include "Sage2020.h"
#include "Utility.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CChangeHistoryPane, CDockablePane)
ON_WM_ACTIVATE()
ON_WM_CREATE()
ON_WM_SIZE()
ON_WM_SETFOCUS()
ON_NOTIFY(TVN_ITEMEXPANDING, IDR_HISTORY_TREE, OnTreeNotifyExpanding)
END_MESSAGE_MAP()

CChangeHistoryPane::CChangeHistoryPane() : m_pToolTipControl(NULL) {}

CChangeHistoryPane::~CChangeHistoryPane() {
  if (m_pToolTipControl)
    CTooltipManager::DeleteToolTip(m_pToolTipControl);
}

void CChangeHistoryPane::OnActivate(UINT nState,
                                    CWnd* pWndOther,
                                    BOOL bMinimized) {
  CDockablePane::OnActivate(nState, pWndOther, bMinimized);
  // TODO: Add your message handler code here
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

  NONCLIENTMETRICS info;
  info.cbSize = sizeof(info);

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

  HTREEITEM htreeitem = pTreeView->itemNew.hItem;

  auto file_version_diff = reinterpret_cast<const FileVersionDiff*>(
      m_wndTreeCtrl.GetItemData(htreeitem));
  if (file_version_diff == NULL) {
    assert(false);
    return;
  }
  for (auto it = file_version_diff->parents_.begin();
       it != file_version_diff->parents_.end(); ++it) {
#if 0
    if (!it->file_version_diffs) {
      GitDiffReader git_diff_reader{file_version_diff->path_,
                                    it->commit_.tag_};
      if (git_diff_reader.GetDiffs().size() > 0) {
        it->file_version_diffs = std::make_unique<std::vector<FileVersionDiff>>(
            git_diff_reader.MoveDiffs());
      }
    }
#endif  // 0
  }
}

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
    ptooltip->AddTool(
        &tree, wzTipLimited, &rcItem,
        reinterpret_cast<UINT_PTR>(&file_version_diff) /*unique id*/);
  }
}

static std::string CreateTreeItemLabel(size_t commit_index,
                                       const FileVersionDiff& file_diff) {
  char time_string[64];
  time_string[0] = '\0';
  if (asctime_s(time_string, &file_diff.author_.time_)) {
    // Error.
    time_string[0] = '\0';
  }
  std::string label =
      std::to_string(commit_index) + ' ' +
      file_diff.author_.name_ + " <" + file_diff.author_.email_ + "> " + std::string(time_string) + ' ' +
                      file_diff.commit_.tag_;
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

  switch (file_version_diff.diff_tree_.action) {
    case 'A':  // add
    {
      VERIFY(tree.SetItemImage(htreeitem, VERSION_IMAGELIST_ADD,
                               VERSION_IMAGELIST_ADD));

      HTREEITEM htreeitemChild = NULL;
      while ((htreeitemChild = tree.GetChildItem(htreeitem)) != NULL)
        tree.DeleteItem(htreeitemChild);
#if 0
      if (file_version_diff.parents_.size() > 0) {
        if (!tree.ItemHasChildren(htreeitem))
          tree.InsertItem(_T("Dummy"), htreeitem);
      }
#endif // 1
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
#if 0
      if (file_version_diff.parents_.size() > 0) {
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

  SetToolTip(tree, file_version_diff, htreeitem);
}

// returns true if the current version has changed due to a tree selection
/*static*/ bool CChangeHistoryPane::FEnsureTreeItemsAndSelection(
    CTreeCtrl& tree,
    HTREEITEM htreeitemRoot,
    const std::vector<FileVersionDiff>& file_diffs,
    const GitHash& selected_commit) {
  BOOL fSelected = FALSE;
  bool fTreeFocused = tree.GetFocus() == &tree;

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
        if (parent.file_version_diffs) {
          fSelected = !FEnsureTreeItemsAndSelection(
              tree, child_item, *parent.file_version_diffs,
              (*parent.file_version_diffs)[0].commit_);
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
#if _DEBUG
      TVITEM tvitem = {};
      tree.GetItem(&tvitem);
#endif  // _DEBUG
      VERIFY(tree.DeleteItem(htreeitem));

      htreeitem = tree.GetNextItem(htreeitem, TVGN_NEXT);
    } while (htreeitem != NULL);
  }
  return !fSelected;
}
