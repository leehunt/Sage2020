#pragma once
#include "DiffTreePath.h"
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

  // TODO: Make these private and expose only GoToCommit() and GoToIndex().
  void AddDiff(const FileVersionDiff& diff);
  void RemoveDiff(const FileVersionDiff& diff);

  bool GoToCommit(const GitHash& commit,
                  const std::vector<FileVersionDiff>& diffs_root);

  bool GoToIndex(size_t commit_index,
                 const std::vector<FileVersionDiff>& diffs);

  const FileVersionInstance& GetFileVersionInstance() const {
    return file_version_instance_;
  }

 private:
  void AddHunk(const FileVersionDiffHunk& hunk);
  void RemoveHunk(const FileVersionDiffHunk& hunk);

  DiffTreePath GetDiffTreePath(const GitHash& commit,
                               const std::vector<FileVersionDiff>& diffs);

 private:
  FileVersionInstance& file_version_instance_;
  Sage2020ViewDocListener* listener_head_;
  friend GitDiffReaderTest;
};
