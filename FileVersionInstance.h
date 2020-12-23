#pragma once
#include <cassert>
#include <deque>
#include <filesystem>
#include <map>
#include <set>

class FileVersionInstanceEditor;
class GitDiffReaderTest;
class FileVersionInstanceTest;

class FileVersionLineInfo {
 public:
  FileVersionLineInfo(size_t commit_index) : commit_index_(commit_index) {
    assert(static_cast<int>(commit_index) != -1);
  }

  bool operator==(const FileVersionLineInfo& other) const {
    return commit_index_ == other.commit_index_;
  }

  bool operator<(const FileVersionLineInfo& other) const {
    return commit_index_ < other.commit_index_;
  }

  const size_t commit_index() const { return commit_index_; };

 private:
  size_t commit_index_;
};
// A one-based file line -> FileVersionLineInfo mapping.
// typedef std::map<int, FileVersionLineInfo> LineToFileVersionLineInfo;
typedef std::deque<FileVersionLineInfo> LineToFileVersionLineInfo;

class FileVersionInstance {
 public:
  FileVersionInstance();
  FileVersionInstance(std::deque<std::string>& lines,
                      const std::string& commit_id);
  virtual ~FileVersionInstance() {}

  std::string GetCommit() const { return commit_id_; }
  size_t GetCommitIndex() const { return commit_index_; }

  const std::deque<std::string>& GetLines() const { return file_lines_; }

  std::set<FileVersionLineInfo> GetVersionsFromLines(int iStart,
                                                     int iEnd) const;

  const FileVersionLineInfo& GetLineInfo(int line_index) const {
    return file_lines_info_[line_index];
  }

 private:
  // Updates/replaces info for [line_num, line_num + line_count).
  void AddLineInfo(int line_num,
                   int line_count,
                   const FileVersionLineInfo& info);

  // Updates/replaces info for [line_num, line_num + line_count).
  void RemoveLineInfo(int line_num, int line_count);

  const LineToFileVersionLineInfo& GetLinesInfo() const {
    return file_lines_info_;
  }

 private:
  std::deque<std::string> file_lines_;
  LineToFileVersionLineInfo file_lines_info_;
  std::string commit_id_;
  int commit_index_;

  // TODO: remove
  friend FileVersionInstanceEditor;

  friend GitDiffReaderTest;
  friend FileVersionInstanceTest;
};
