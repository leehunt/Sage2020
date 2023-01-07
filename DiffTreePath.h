#pragma once
#include <vector>

#include "FileVersionDiff.h"

// #define DIFF_TREE_PARENT_BRANCH_PATH 1

// struct FileVersionDiff;
//  {Index of current item, parent index}.
class DiffTreePath;
class DiffTreePathItem {
 public:
  DiffTreePathItem();
  DiffTreePathItem(const DiffTreePathItem& item);
  DiffTreePathItem(DiffTreePathItem&& item) noexcept;

  DiffTreePathItem& setCurrentBranchIndex(int current_branch_index);
  DiffTreePathItem& setParentIndex(size_t parent_index);
  DiffTreePathItem& setBranch(const std::vector<FileVersionDiff>* branch);
#if DIFF_TREE_PARENT_BRANCH_PATH
  DiffTreePathItem& setParentBranchPath(const DiffTreePath& parent_branch_path);
#endif

  bool operator==(const DiffTreePathItem& item) const;
  bool operator!=(const DiffTreePathItem& item) const;

  bool HasSameParent(const DiffTreePathItem& item) const;

  DiffTreePathItem& operator=(const DiffTreePathItem& item);

  int currentBranchIndex() const;
  size_t parentIndex() const;
  const std::vector<FileVersionDiff>* branch() const;
#if DIFF_TREE_PARENT_BRANCH_PATH
  const DiffTreePath& parentBranchPath() const;
#endif

 private:
  int current_branch_index_;  // Index into the FileVersionDiff vector. -1 means
                              // no current index.
  size_t parent_index_;       // For children items, this is the index of their
                              // parent's FileVersionDiff.parent_ array.
  const std::vector<FileVersionDiff>*
      branch_;  // The subbranch for the current item.
#if DIFF_TREE_PARENT_BRANCH_PATH
  std::unique_ptr<DiffTreePath>
      parent_branch_path_;  // For children items, this is the index from which
                            // the subranch has been branched.
#endif
};

class DiffTreePath : public std::vector<DiffTreePathItem> {
 public:
  DiffTreePath() {}
#if 1
  DiffTreePath(const DiffTreePath& other) {
    auto& inner_class = static_cast<std::vector<DiffTreePathItem>&>(*this);
    inner_class = static_cast<const std::vector<DiffTreePathItem>&>(other);
  }
#endif
  // Clever/sleezy index adpators that operate on the innermost
  // |DiffTreePathItem|'s currentBranchIndex().
  int operator++();
  int operator--();
  operator int() const;

  DiffTreePath& operator=(const DiffTreePath& other);

  const std::vector<FileVersionDiff>* GetRoot() const;
  const std::vector<FileVersionDiff>* DiffsSubbranch() const;

  const FileVersionDiff* Diff() const;

  bool HasSameParent(const DiffTreePath& other) const;
  DiffTreePath GetAncestors(size_t num_ancestors) const;

  std::string PathText() const;

 private:
  ;
};