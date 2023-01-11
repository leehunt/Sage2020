#include "pch.h"

#include "DiffTreePath.h"

#include <assert.h>
#include <ccomplex>

#include "FileVersionDiff.h"

DiffTreePathItem::DiffTreePathItem()
    : current_branch_index_(-1), parent_index_(0), branch_(nullptr) {}
DiffTreePathItem::DiffTreePathItem(const DiffTreePathItem& item)
    : current_branch_index_(item.current_branch_index_),
      parent_index_(item.parent_index_),
      branch_(item.branch_)
#if DIFF_TREE_PARENT_BRANCH_PATH
      ,
      parent_branch_path_(item.parent_branch_path_
                              ? new DiffTreePath(*item.parent_branch_path_)
                              : nullptr)
#endif
{
}
DiffTreePathItem::DiffTreePathItem(DiffTreePathItem&& item) noexcept
    : current_branch_index_(item.currentBranchIndex()),
      parent_index_(item.parentIndex()),
      branch_(item.branch_)
#if DIFF_TREE_PARENT_BRANCH_PATH
      ,
      parent_branch_path_(std::move(item.parent_branch_path_))
#endif
{
  item.current_branch_index_ = -1;
  item.parent_index_ = 0;
  item.branch_ = nullptr;
}

DiffTreePathItem& DiffTreePathItem::setCurrentBranchIndex(
    int current_branch_index) {
  current_branch_index_ = current_branch_index;
  return *this;
}
DiffTreePathItem& DiffTreePathItem::setParentIndex(size_t parent_index) {
  parent_index_ = parent_index;
  return *this;
}
DiffTreePathItem& DiffTreePathItem::setBranch(
    const std::vector<FileVersionDiff>* branch) {
  branch_ = branch;
  return *this;
}
#if DIFF_TREE_PARENT_BRANCH_PATH
DiffTreePathItem& DiffTreePathItem::setParentBranchPath(
    const DiffTreePath& parent_branch_path) {
  parent_branch_path_ = std::make_unique<DiffTreePath>(parent_branch_path);
  return *this;
}
#endif

bool DiffTreePathItem::operator==(const DiffTreePathItem& item) const {
  bool is_equal = currentBranchIndex() == item.currentBranchIndex() &&
                  HasSameParent(item)
#if DIFF_TREE_PARENT_BRANCH_PATH
                  && parentBranchPath() == item.parentBranchPath()
#endif
      ;
  return is_equal;
}
bool DiffTreePathItem::operator!=(const DiffTreePathItem& item) const {
  return !(*this == item);
}

bool DiffTreePathItem::HasSameParent(const DiffTreePathItem& item) const {
  bool has_same_parent = parentIndex() == item.parentIndex();
  return has_same_parent;
}

DiffTreePathItem& DiffTreePathItem::operator=(const DiffTreePathItem& item) {
  setCurrentBranchIndex(item.currentBranchIndex());
  setParentIndex(item.parentIndex());
#if DIFF_TREE_PARENT_BRANCH_PATH
  setParentBranchPath(item.parentBranchPath());
#endif
  return *this;
}

int DiffTreePathItem::currentBranchIndex() const {
  return current_branch_index_;
}
size_t DiffTreePathItem::parentIndex() const {
  return parent_index_;
}
const std::vector<FileVersionDiff>* DiffTreePathItem::branch() const {
  return branch_;
}
#if DIFF_TREE_PARENT_BRANCH_PATH
const DiffTreePath& DiffTreePathItem::parentBranchPath() const {
  static std::vector<FileVersionDiff> s_empty_root;
  static DiffTreePath s_empty_path{s_empty_root};
  return parent_branch_path_ ? *parent_branch_path_ : s_empty_path;
}
#endif

DiffTreePath& DiffTreePath::operator=(const DiffTreePath& other) {
  // Crazy hacky way to allow reseating the |root_| reference. :/
  if (this != &other) {
    this->~DiffTreePath();
    new (this) DiffTreePath(other);
  }
  return *this;
}

DiffTreePath::operator int() const {
  if (empty())
    return -1;
  return back().currentBranchIndex();
}

int DiffTreePath::operator++() {
  int ret = back().currentBranchIndex();
  ret++;
  assert(ret >= 0);  // This can be -1 if there is no parent branch (e.g.
  // the branch created the file).
  back().setCurrentBranchIndex(ret);
  return ret;
}

int DiffTreePath::operator--() {
  int ret = -1;
  if (!empty()) {
    ret = back().currentBranchIndex();
    if (!ret) {
      // Tricky: Pop off innermost branch after decrementing the 0th index.
      pop_back();
    } else {
      ret--;
      back().setCurrentBranchIndex(ret);
    }
  }
  return ret;
}

const FileVersionDiff* DiffTreePath::Diff() const {
  if (empty())
    return nullptr;

  auto current_item = &front();
  auto current_item_index = current_item->currentBranchIndex();
  if (current_item_index == -1)
    return nullptr;

  const FileVersionDiff* diff = &(GetRoot())[current_item_index];
  for (auto it = cbegin() + 1; it != cend(); it++) {
    const auto& new_item = *it;
    if (new_item.currentBranchIndex() == -1)
      break;
    assert(current_item->parentIndex() != 0);
    diff = &(*diff->parents_[current_item->parentIndex()].file_version_diffs_)
                .at(new_item.currentBranchIndex());
    current_item = &new_item;
  }

  return diff;
}

const std::vector<FileVersionDiff>* DiffTreePath::DiffsSubbranch() const {
  return back().branch();
}

bool DiffTreePath::HasSameParent(const DiffTreePath& other) const {
  return DiffsSubbranch() == other.DiffsSubbranch();
}

const std::vector<FileVersionDiff>& DiffTreePath::GetRoot() const {
  return root_;
}

DiffTreePath DiffTreePath::GetAncestors(size_t num_ancestors) const {
  assert(num_ancestors <= size());
  DiffTreePath new_path{GetRoot()};
  for (size_t i = 0; i < num_ancestors; i++)
    new_path.push_back((*this)[i]);
  return new_path;
}

std::string DiffTreePath::PathText() const {
  if (empty())
    return "{}";

  std::string s;
  s += "{";
  for (auto& item : *this) {
    s += " ";
    s += std::to_string(item.currentBranchIndex());
    s += ",";
    s += std::to_string(item.parentIndex());
  }
  s += " }";

  return s;
}
