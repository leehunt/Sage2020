#pragma once
#include <filesystem>
#include "FileVersionDiff.h"

class CGitDiffReader {
 public:
  CGitDiffReader(const std::filesystem::path& file_path);
  virtual ~CGitDiffReader();

  size_t Size() const { return diffs_.size(); }
  std::vector<FileVersionDiff> GetDiffs();

 private:
  void ProcessDiffLines(FILE* stream);
  bool ProcessLogLine(char* line);

  std::vector<FileVersionDiff> diffs_;
};
