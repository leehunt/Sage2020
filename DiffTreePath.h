#pragma once
#include <vector>

struct FileVersionDiff;
// {Index of current item, parent index}.
class DiffTreePathItem {
 public:
  DiffTreePathItem();
  DiffTreePathItem(const DiffTreePathItem& item);
  DiffTreePathItem(DiffTreePathItem&& item);

  DiffTreePathItem& setCurrentBranchIndex(size_t current_branch_index);
  DiffTreePathItem& setParentIndex(size_t parent_index);
  DiffTreePathItem& setSubBranchRoot(
      const std::vector<FileVersionDiff>* sub_branch_root);

  bool operator==(const DiffTreePathItem& item) const;
  bool operator!=(const DiffTreePathItem& item) const;

  DiffTreePathItem& operator=(const DiffTreePathItem& item);

  size_t currentBranchIndex() const;
  size_t parentIndex() const;
  const std::vector<FileVersionDiff>* subBranchRoot() const;

 private:
  size_t current_branch_index_;
  size_t parent_index_;
  const std::vector<FileVersionDiff>* sub_branch_root_;
};

class DiffTreePath : public std::vector<DiffTreePathItem> {
 public:
  // Clever/sleezy index adpators that operate on the innermost
  // |DiffTreePathItem|'s currentBranchIndex().
  size_t operator++();
  size_t operator--();
  operator int() const;

  bool HasSameParent(const DiffTreePath& other) const;
  DiffTreePath GetAncestors(size_t num_ancestors) const;
};