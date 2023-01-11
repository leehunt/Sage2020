#pragma once

#include <filesystem>

#include "FileVersionLineInfo.h"
#include "GitHash.h"

struct FileVersionDiff;

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
  bool operator==(const FileVersionDiffHunk& other) const;
  bool operator!=(const FileVersionDiffHunk& other) const;
#endif

#if _DEBUG_MEM_TRACE
  FileVersionDiffHunk() { printf("FileVersionDiffHunk\t%p\n", this); }
  ~FileVersionDiffHunk() { printf("~FileVersionDiffHunk\t%p\n", this); }
#endif
};
struct FileVersionDiffTree {
  FileVersionDiffTree() : old_mode(0), new_mode(0) {
    old_hash_string[0] = '\0';
    new_hash_string[0] = '\0';
    file_path[0] = '\0';
    action[0] = '\0';
  }
  int old_mode;
  char old_hash_string[14];
  int new_mode;
  char new_hash_string[14];
  char action[5];  // The action (see FReadGitDiffTreeColon()) plus any
                   // similarity percentage.
  char file_path[FILENAME_MAX];

#ifdef _DEBUG
  bool operator==(const FileVersionDiffTree& other) const;
  bool operator!=(const FileVersionDiffTree& other) const;
#endif  // _DEBUG
};

struct FileVersionDiffParent {
  GitHash commit_;  // The *newest* commit (tail of the branch that was merged
                    // back in to parent).
  std::unique_ptr<std::vector<FileVersionDiff>> file_version_diffs_;

#ifdef _DEBUG
  bool operator==(const FileVersionDiffParent& other) const;
  bool operator!=(const FileVersionDiffParent& other) const;
#endif  // _DEBUG

#if _DEBUG_MEM_TRACE
  FileVersionDiffParent() { printf("FileVersionDiffParent\t%p\n", this); }
  ~FileVersionDiffParent() { printf("~FileVersionDiffParent\t%p\n", this); }
#endif
};
struct FileVersionDiffChild {
  GitHash commit_;  // The *oldest* commit (head of the branch that was merged
                    // back in to parent).
#ifdef _DEBUG
  bool operator==(const FileVersionDiffChild& other) const;
  bool operator!=(const FileVersionDiffChild& other) const;
#endif  // _DEBUG

#if _DEBUG_MEM_TRACE
  FileVersionDiffChild() { printf("FileVersionDiffChild\t%p\n", this); }
  ~FileVersionDiffChild() { printf("~FileVersionDiffChild\t%p\n", this); }
#endif
};
struct NameEmailTime {
  std::string name_;
  std::string email_;
  struct tm time_;

#ifdef _DEBUG
  bool operator==(const NameEmailTime& other) const;
  bool operator!=(const NameEmailTime& other) const;
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
                                // For a subbranch's 0th entry, this points to
                                // the parent branch's branch commit (i.e.down
                                // towards the first commit).
  mutable std::vector<FileVersionDiffChild>
      children_;  // NOTE: This is not fully populated with all branching points
                  // when getting the git log filtered to a file.
  GitHash file_child_commit_;  // Sometimes the same as children_[0].commit_,
                               // but will always point to a commit that has
                               // changes to the current *file* (whereas
                               // children_[0].commit_ may only have changes to
                               // other files of the diff_tree_).
                               // For a subbranch's nth entry, this points to
                               // the child branch's merge commit (i.e. up to
                               // the HEAD commit).
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
  bool operator==(const FileVersionDiff& other) const;
  bool operator!=(const FileVersionDiff& other) const;
#endif  // _DEBUG

#if _DEBUG_MEM_TRACE
//  FileVersionDiff() { printf("FileVersionDiff\t%p\n", this); }
// ~FileVersionDiff() { printf("~FileVersionDiff\t%p\n", this); }
#endif
};

// struct FileVersionDiffs : std::vector<FileVersionDiff> {};