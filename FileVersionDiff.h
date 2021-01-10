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
};
struct FileVersionDiff;
struct FileVersionDiffParent {
  GitHash commit_;
  mutable std::unique_ptr<std::vector<FileVersionDiff>> file_version_diffs;
#if _DEBUG_MEM_TRACE
  FileVersionDiffParent() { printf("FileVersionDiffParent\t%p\n", this); }
  ~FileVersionDiffParent() { printf("~FileVersionDiffParent\t%p\n", this); }
#endif
};
struct FileVersionDiff {
  std::filesystem::path path_;
  std::vector<FileVersionDiffHunk> hunks_;
  GitHash commit_;
  std::string tree_;
  std::vector<FileVersionDiffParent> parents_;
  std::string author_;
  std::string committer_;
  std::string comment_;
  FileVersionDiffTree diff_tree_;
  std::string diff_command_;
  std::string index_;
  int ver_ = 0;
#if _DEBUG_MEM_TRACE
  FileVersionDiff() { printf("FileVersionDiff\t%p\n", this); }
  ~FileVersionDiff() { printf("~FileVersionDiff\t%p\n", this); }
#endif
};
