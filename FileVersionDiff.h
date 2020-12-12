#pragma once
#include <string>
#include <vector>
struct FileVersionDiffHunk {
  int add_location_;
  int add_line_count_;
  std::vector<std::string> add_lines_;
  int remove_location_;
  int remove_line_count_;
  std::vector<std::string> remove_lines_;
  std::string start_context_;
};
struct FileVersionDiffTree {
  int old_mode;
  char old_hash_string[14];
  int new_mode;
  char new_hash_string[14];
  char action;
  char file_path[MAX_PATH];
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
