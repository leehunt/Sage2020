#pragma once
#include "FileVersionDiff.h"
#include "FileVersionInstance.h"

class GitDiffReaderTest;

class FileVersionInstanceEditor {
 public:
  FileVersionInstanceEditor(FileVersionInstance& file_version_instance)
      : file_version_instance_(file_version_instance) {}

  void AddDiff(const FileVersionDiff& diff);
  void RemoveDiff(const FileVersionDiff& diff);

 private:
  void AddHunk(const FileVersionDiffHunk& hunk);
  void RemoveHunk(const FileVersionDiffHunk& hunk);

 private:
  FileVersionInstance& file_version_instance_;
  friend GitDiffReaderTest;
};
