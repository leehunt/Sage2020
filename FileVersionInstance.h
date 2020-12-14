#pragma once
#include <deque>
#include <filesystem>

#include "FileVersionDiff.h"

struct FileVersionLine {
  std::string line;
  int age;

  bool operator==(const FileVersionLine& other) const {
    return age == other.age && line == other.line;
  }
};
class FileVersionInstance {
 public:
  FileVersionInstance();
  FileVersionInstance(std::deque<std::string>& lines, int commit_sequence_num);
  virtual ~FileVersionInstance() {}

  void AddHunk(const FileVersionDiffHunk& hunk, int commit_sequence_num);

  const std::deque<FileVersionLine>& GetLines() const { return file_lines_; }

 private:
   std::deque<FileVersionLine> file_lines_;
};
