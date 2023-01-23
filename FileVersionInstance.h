#pragma once

#include <deque>
#include <filesystem>
#include <set>
#include <vector>

#include "DiffTreePath.h"
#include "FileVersionLineInfo.h"
#include "GitHash.h"

class FileVersionInstanceEditor;
class GitDiffReaderTest;
class FileVersionInstanceTest;
struct FileVersionDiff;

class FileVersionInstance {
 public:
  FileVersionInstance(const std::vector<FileVersionDiff>& root);
  FileVersionInstance(const std::vector<FileVersionDiff>& root,
                      std::deque<std::string>&& lines,  // N.b. *Moves* |lines|.
                      const GitHash& commit);
  virtual ~FileVersionInstance() {}

  const GitHash& GetCommit() const;
  // Tricky: return the integer index (or -1 if none) for the current tip branch
  // index of the |commit_path_| path.
  int GetCommitIndex() const;
  const std::vector<FileVersionDiff>* GetBranchDiffs() const;

  const std::deque<std::string>& GetLines() const;
  std::set<FileVersionLineInfo> GetVersionsFromLines(int iStart,
                                                     int iEnd) const;

  const FileVersionLineInfo& GetLineInfo(int line_index) const;
  std::unique_ptr<LineToFileVersionLineInfo> SnapshotLineInfo(
      int line_num,
      int line_count) const;

 private:
  // Updates/replaces info for [line_num, line_num + line_count).
  void AddLineInfo(int line_num,
                   int line_count,
                   const LineToFileVersionLineInfo& line_infos);
  void AddLineInfo(int line_num,
                   int line_count,
                   LineToFileVersionLineInfo&& line_infos);

  // Updates/replaces info for [line_num, line_num + line_count).
  void RemoveLineInfo(int line_num, int line_count);

  const LineToFileVersionLineInfo& GetLinesInfo() const;

  // Note: can be expensive; use only for testing.
  // TODO: Make private.
  FileVersionInstance& operator=(const FileVersionInstance& source);

  // Note: can be expensive; use only for testing.
  // TODO: Make private.
  bool operator==(const FileVersionInstance& source) const;

 public:
  // TODO: Make Private!
  DiffTreePath commit_path_;

 private:
  std::deque<std::string> file_lines_;
  LineToFileVersionLineInfo file_lines_info_;
  GitHash commit_;  // TODO: This may be reududant; it could possibly be derived
                    // from the current |commit_path_|.

  // REVIEW: Is there a better way to express this relationship?
  friend FileVersionInstanceEditor;

  friend FileVersionInstanceTest;
  friend GitDiffReaderTest;
};
