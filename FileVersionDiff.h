#pragma once
#include <memory>
#include <string>
#include <vector>
#include <deque>

#if USE_SPARSE_INDEX_ARRAY
class SparseIndexArray;
typedef SparseIndexArray LineToFileVersionLineInfo;
#else
class FileVersionLineInfo;
typedef std::deque<FileVersionLineInfo> LineToFileVersionLineInfo;
#endif

struct FileVersionDiffHunk {
  int add_location_;
  int add_line_count_;
  std::vector<std::string> add_lines_;
  int remove_location_;
  int remove_line_count_;
  std::vector<std::string> remove_lines_;
  std::string start_context_;
  // Keep track of any deleted line info so that remove operations will restore
  // the current history of changes up to that point.
  // REVIEW: Find way to turn this into a std::unique_ptr<>.
  mutable std::shared_ptr<LineToFileVersionLineInfo> line_info_to_restore_;
};
struct FileVersionDiffTree {
  int old_mode;
  char old_hash_string[14];
  int new_mode;
  char new_hash_string[14];
  char action;
  char file_path[FILENAME_MAX];
};
struct FileVersionDiff {
  std::vector<FileVersionDiffHunk> hunks_;
  std::string commit_;
  std::string tree_;
  std::string parent_;
  std::string author_;
  std::string committer_;
  std::string comment_;
  FileVersionDiffTree diff_tree_;
  std::string diff_command_;
  std::string index_;
  int ver_;
};
