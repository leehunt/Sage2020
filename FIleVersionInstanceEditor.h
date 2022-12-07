#pragma once
#include "FileVersionDiff.h"
#include "FileVersionInstance.h"

class GitDiffReaderTest;
class Sage2020ViewDocListener;

class FileVersionInstanceEditor {
 public:
  FileVersionInstanceEditor(FileVersionInstance& file_version_instance,
                            Sage2020ViewDocListener* listener_head)
      : file_version_instance_(file_version_instance),
        listener_head_(listener_head) {}

  void AddDiff(const FileVersionDiff& diff);
  void RemoveDiff(const FileVersionDiff& diff);

  bool GoToCommit(const GitHash& commit,
                  const std::vector<FileVersionDiff>& diffs_root);

  bool GoToIndex(size_t commit_index,
                 const std::vector<FileVersionDiff>& diffs);

  const FileVersionInstance& GetFileVersionInstance() const {
    return file_version_instance_;
  }

 private:
  void AddHunk(const FileVersionDiffHunk& hunk);
  void RemoveHunk(const FileVersionDiffHunk& hunk);

  // {Index of current item, parent index}.
  class CommitTreePathItem {
   public:
    CommitTreePathItem() : current_branch_index_(0), parent_index_(0) {}
    CommitTreePathItem(const CommitTreePathItem& item)
        : current_branch_index_(item.current_branch_index_),
          parent_index_(item.parent_index_),
          sub_branch_root_(nullptr) {}

    CommitTreePathItem& setCurrentBranchIndex(size_t current_branch_index) {
      current_branch_index_ = current_branch_index;
      return *this;
    }
    CommitTreePathItem& setParentIndex(size_t parent_index) {
      parent_index_ = parent_index;
      return *this;
    }
    CommitTreePathItem& setSubBranchRoot(
        const std::vector<FileVersionDiff>* sub_branch_root) {
      sub_branch_root_ = sub_branch_root;
      return *this;
    }

    bool operator==(const CommitTreePathItem& item) const {
      return current_branch_index_ == item.current_branch_index_ &&
             parent_index_ == item.parent_index_;
    }
    bool operator!=(const CommitTreePathItem& item) const {
      return !(*this == item);
    }

    size_t currentBranchIndex() const { return current_branch_index_; }
    size_t parentIndex() const { return parent_index_; }
    const std::vector<FileVersionDiff>* subBranchRoot() const {
      return sub_branch_root_;
    }

   private:
    size_t current_branch_index_;
    size_t parent_index_;
    const std::vector<FileVersionDiff>* sub_branch_root_;
  };

  // TODO: Move this struct into its own header or maybe into
  // FileVersionInstance.h.
  struct CommitTreePath : std::vector<CommitTreePathItem> {
    bool HasSameParent(const CommitTreePath& other) const {
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
  };
  CommitTreePath GetCommitTreePath(const GitHash& commit,
                                   const std::vector<FileVersionDiff>& diffs);

 private:
  FileVersionInstance& file_version_instance_;
  Sage2020ViewDocListener* listener_head_;
  friend GitDiffReaderTest;
};
