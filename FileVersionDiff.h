#pragma once
#include <deque>
#include <memory>
#include <string>
#include <vector>

#if USE_SPARSE_INDEX_ARRAY
class SparseIndexArray;
typedef SparseIndexArray LineToFileVersionLineInfo;
#else
class FileVersionLineInfo;
typedef std::deque<FileVersionLineInfo> LineToFileVersionLineInfo;
#endif

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
  // REVIEW: Find way to turn this into a std::unique_ptr<> (doing so requires
  // exposing all of FileVersionLineInfo, which is currently not in unqiue
  // header file).
  mutable std::shared_ptr<LineToFileVersionLineInfo> line_info_to_restore_;
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
  int ver_ = 0;

  ~FileVersionDiff() {
#if _DEBUG
    ver_ = 0;
#endif
  }
};
