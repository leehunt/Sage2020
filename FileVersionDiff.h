#pragma once
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include "FileVersionInstance.h"
#include "GitHash.h"

struct FileVersionDiffHunk {
  int add_location_ = 0;
  int add_line_count_ = 0;
  std::vector<std::string> add_lines_;
  int remove_location_ = 0;
  int remove_line_count_ = 0;
  std::vector<std::string> remove_lines_;
  std::string start_context_;
  // Keep track of any deleted line info so that remove operations will restore
  // the current history of changes up to that point.
  mutable std::unique_ptr<LineToFileVersionLineInfo> line_info_to_restore_;

#ifdef _DEBUG
  bool operator==(const FileVersionDiffHunk& other) const {
    if (this == &other)
      return true;
    if (add_location_ != other.add_location_)
      return false;
    if (add_line_count_ != other.add_line_count_)
      return false;
    if (add_lines_ != other.add_lines_)
      return false;
    if (remove_location_ != other.remove_location_)
      return false;
    if (remove_line_count_ != other.remove_line_count_)
      return false;
    if (remove_lines_ != other.remove_lines_)
      return false;
    if (start_context_ != other.start_context_)
      return false;
    if (line_info_to_restore_ != other.line_info_to_restore_)
      return false;

    return true;
  }
  bool operator!=(const FileVersionDiffHunk& other) const {
    return !(*this == other);
  }
#endif  // _DEBUG

#if _DEBUG_MEM_TRACE
  FileVersionDiffHunk() { printf("FileVersionDiffHunk\t%p\n", this); }
  ~FileVersionDiffHunk() { printf("~FileVersionDiffHunk\t%p\n", this); }
#endif
};
struct FileVersionDiffTree {
  FileVersionDiffTree() : old_mode(0), new_mode(0), action('\0') {
    old_hash_string[0] = '\0';
    new_hash_string[0] = '\0';
    file_path[0] = '\0';
  }
  int old_mode;
  char old_hash_string[14];
  int new_mode;
  char new_hash_string[14];
  char action;
  char file_path[FILENAME_MAX];

#ifdef _DEBUG
  bool operator==(const FileVersionDiffTree& other) const {
    if (this == &other)
      return true;
    if (old_mode != other.old_mode)
      return false;
    if (strcmp(old_hash_string, other.old_hash_string))
      return false;
    if (new_mode != other.new_mode)
      return false;
    if (strcmp(new_hash_string, other.new_hash_string))
      return false;
    if (action != other.action)
      return false;
    if (strcmp(file_path, other.file_path))
      return false;

    return true;
  }
  bool operator!=(const FileVersionDiffTree& other) const {
    return !(*this == other);
  }
#endif  // _DEBUG
};
struct FileVersionDiff;
struct FileVersionDiffParent {
  GitHash commit_;  // The *newest* commit (head of the branch that was merged
                    // back in to parent).
  std::unique_ptr<std::vector<FileVersionDiff>> file_version_diffs_;

#ifdef _DEBUG
  bool operator==(const FileVersionDiffParent& other) const {
    if (this == &other)
      return true;
    if (commit_ != other.commit_)
      return false;
    if (!file_version_diffs_ != !other.file_version_diffs_)
      return false;
    if (file_version_diffs_ &&
        (*file_version_diffs_ != *other.file_version_diffs_)) {
      return false;
    }
    return true;
  }
  bool operator!=(const FileVersionDiffParent& other) const {
    return !(*this == other);
  }
#endif  // _DEBUG

#if _DEBUG_MEM_TRACE
  FileVersionDiffParent() { printf("FileVersionDiffParent\t%p\n", this); }
  ~FileVersionDiffParent() { printf("~FileVersionDiffParent\t%p\n", this); }
#endif
};
struct NameEmailTime {
  std::string name_;
  std::string email_;
  struct tm time_;

#ifdef _DEBUG
  bool operator==(const NameEmailTime& other) const {
    if (this == &other)
      return true;
    if (name_ != other.name_)
      return false;
    if (email_ != other.email_)
      return false;
    if (memcmp(&time_, &other.time_, sizeof(time_)))
      return false;

    return true;
  }
  bool operator!=(const NameEmailTime& other) const {
    return !(*this == other);
  }
#endif  // _DEBUG
};
struct FileVersionDiff {
  GitHash commit_;
  mutable std::vector<FileVersionDiffParent> parents_;
  GitHash file_parent_commit_;  // Sometimes the same as parents_[0].commit_,
                                // but will always point to a commit that has
                                // changes to the current *file* (whereas
                                // parents_[0].commit_ may only have changes to
                                // other files of the diff_tree_).
  std::vector<FileVersionDiffHunk> hunks_;
  NameEmailTime author_;
  std::string diff_command_;
  NameEmailTime committer_;
  std::string comment_;
  std::string tree_;
  // N.b. In the more genernal case; when getting info from a commit *not*
  // filtered to a file (i.e. |path_| here), there could be multiple instances
  // of |diff_tree_| per commit.
  FileVersionDiffTree diff_tree_;
  std::string index_;
  std::filesystem::path path_;

#ifdef _DEBUG
  bool operator==(const FileVersionDiff& other) const {
    if (commit_ != other.commit_)
      return false;
    if (parents_ != other.parents_)
      return false;
    if (hunks_ != other.hunks_)
      return false;
    if (author_ != other.author_)
      return false;
    if (diff_command_ != other.diff_command_)
      return false;
    if (committer_ != other.committer_)
      return false;
    if (comment_ != other.comment_)
      return false;
    if (tree_ != other.tree_)
      return false;
    if (diff_tree_ != other.diff_tree_)
      return false;
    if (index_ != other.index_)
      return false;
    if (commit_ != other.commit_)
      return false;
    if (path_ != other.path_)
      return false;

    return true;
  }
  bool operator!=(const FileVersionDiff& other) const {
    return !(*this == other);
  }
#endif  // _DEBUG

#if _DEBUG_MEM_TRACE
//  FileVersionDiff() { printf("FileVersionDiff\t%p\n", this); }
// ~FileVersionDiff() { printf("~FileVersionDiff\t%p\n", this); }
#endif
};

//struct FileVersionDiffs : std::vector<FileVersionDiff> {};