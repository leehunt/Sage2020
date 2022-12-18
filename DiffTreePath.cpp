#include "pch.h"

#include "DiffTreePath.h"

#include "FileVersionDiff.h"

DiffTreePathItem::DiffTreePathItem()
    : current_branch_index_(0), parent_index_(0), sub_branch_root_(nullptr) {}
DiffTreePathItem::DiffTreePathItem(const DiffTreePathItem& item)
    : current_branch_index_(item.current_branch_index_),
      parent_index_(item.parent_index_),
      sub_branch_root_(item.sub_branch_root_) {}
DiffTreePathItem::DiffTreePathItem(DiffTreePathItem&& item)
    : current_branch_index_(item.current_branch_index_),
      parent_index_(item.parent_index_),
      sub_branch_root_(item.sub_branch_root_) {
  item.current_branch_index_ = 0;
  item.parent_index_ = 0;
  item.sub_branch_root_ = nullptr;
}

DiffTreePathItem& DiffTreePathItem::setCurrentBranchIndex(
    size_t current_branch_index) {
  current_branch_index_ = current_branch_index;
  return *this;
}
DiffTreePathItem& DiffTreePathItem::setParentIndex(size_t parent_index) {
  parent_index_ = parent_index;
  return *this;
}
DiffTreePathItem& DiffTreePathItem::setSubBranchRoot(
    const std::vector<FileVersionDiff>* sub_branch_root) {
  sub_branch_root_ = sub_branch_root;
  return *this;
}

bool DiffTreePathItem::operator==(const DiffTreePathItem& item) const {
  bool is_equal = current_branch_index_ == item.current_branch_index_ &&
                  parent_index_ == item.parent_index_;
  assert(!sub_branch_root_ == !item.sub_branch_root_);
  assert(!sub_branch_root_ || *sub_branch_root_ == *item.sub_branch_root_);
  return is_equal;
}
bool DiffTreePathItem::operator!=(const DiffTreePathItem& item) const {
  return !(*this == item);
}

DiffTreePathItem& DiffTreePathItem::operator=(const DiffTreePathItem& item) {
  current_branch_index_ = item.current_branch_index_;
  parent_index_ = item.parent_index_;
  sub_branch_root_ = item.sub_branch_root_;
  return *this;
}

size_t DiffTreePathItem::currentBranchIndex() const {
  return current_branch_index_;
}
size_t DiffTreePathItem::parentIndex() const {
  return parent_index_;
}
const std::vector<FileVersionDiff>* DiffTreePathItem::subBranchRoot() const {
  return sub_branch_root_;
}

DiffTreePath::operator int() const {
  if (empty())
    return -1;
  return (int)(back().currentBranchIndex());
}

size_t DiffTreePath::operator++() {
  size_t ret = 0;
  if (empty())
    push_back(DiffTreePathItem());
  else {
    ret = back().currentBranchIndex();
    ret++;
    assert(ret > 0);
    back().setCurrentBranchIndex(ret);
  }
  return ret;
}

size_t DiffTreePath::operator--() {
  size_t ret = 0;
  if (!empty()) {
    ret = back().currentBranchIndex();
    if (!ret)
      pop_back();
    else {
      ret--;
      back().setCurrentBranchIndex(ret);
    }
  }
  return ret;
}

bool DiffTreePath::HasSameParent(const DiffTreePath& other) const {
  if (size() != other.size())
    return false;
  if (empty())
    return true;

  // Check if the parents are the same.
  for (size_t i = 0; i < size() - 1; i++) {
    if ((*this)[i] != other[i])
      return false;
  }
  return true;
}

DiffTreePath DiffTreePath::GetAncestors(size_t num_ancestors) const {
  assert(num_ancestors <= size());
  DiffTreePath new_path;
  for (auto it = cbegin(); it < cbegin() + num_ancestors; it++)
    new_path.push_back(*it);
  return new_path;
}
