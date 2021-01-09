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
                  const std::vector<FileVersionDiff>& diffs);

  bool GoToIndex(size_t commit_index, const std::vector<FileVersionDiff>& diffs);

 private:
  void AddHunk(const FileVersionDiffHunk& hunk);
  void RemoveHunk(const FileVersionDiffHunk& hunk);

 private:
  FileVersionInstance& file_version_instance_;
  Sage2020ViewDocListener* listener_head_;
  friend GitDiffReaderTest;
};
