#pragma once
#include <cassert>
#include <deque>
#include <filesystem>
#include <map>
#include <set>
#include <vector>

#define USE_SPARSE_INDEX_ARRAY 0

class FileVersionInstanceEditor;
class GitDiffReaderTest;
class FileVersionInstanceTest;
class SparseIndexArray;

class FileVersionLineInfo {
 public:
  FileVersionLineInfo(size_t commit_index) : commit_index_(commit_index) {
    assert(static_cast<int>(commit_index) != -1);
  }
  // Ending terminator (special case)
  FileVersionLineInfo() : commit_index_(static_cast<size_t>(-1)) {}

  bool operator==(const FileVersionLineInfo& other) const {
    return commit_index_ == other.commit_index_;
  }

  bool operator<(const FileVersionLineInfo& other) const {
    return commit_index_ < other.commit_index_;
  }

  const size_t commit_index() const { return commit_index_; };

 private:
  size_t commit_index_;

  friend SparseIndexArray;
};

class SparseIndexArray : std::map<size_t, FileVersionLineInfo> {
 public:
  SparseIndexArray();

  const FileVersionLineInfo& Get(size_t index) const;

  void Add(size_t line_index,
           size_t line_count,
           const FileVersionLineInfo& line_info);
  void Remove(size_t line_index, size_t line_count);

  bool IsEmpty() const {
    assert(std::prev(end())->second == FileVersionLineInfo());

    bool is_empty = std::prev(end()) == cbegin();
    assert(cbegin()->first == 0);
    return is_empty;
  }

  size_t MaxLineIndex() const { return std::prev(end())->first; }

  SparseIndexArray::iterator SubtractMapKey(SparseIndexArray::iterator it,
                                            size_t to_subtract);

  bool operator==(const SparseIndexArray& other) const {
    return *static_cast<const std::map<size_t, FileVersionLineInfo>*>(this) ==
           static_cast<const std::map<size_t, FileVersionLineInfo>>(other);
  }

 private:
  const_iterator LowerFloor(size_t index) const;

  friend FileVersionInstanceTest;
};

// A one-based file line -> FileVersionLineInfo mapping.
// typedef std::map<int, FileVersionLineInfo> LineToFileVersionLineInfo;
#if USE_SPARSE_INDEX_ARRAY
typedef SparseIndexArray LineToFileVersionLineInfo;
#else
typedef std::deque<FileVersionLineInfo> LineToFileVersionLineInfo;
#endif

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
#if USE_SPARSE_INDEX_ARRAY
    return file_lines_info_.Get(line_index);
#else
    return file_lines_info_[line_index];
#endif
  }

 private:
  // Updates/replaces info for [line_num, line_num + line_count).
  void AddLineInfo(int line_num,
                   int line_count,
      const LineToFileVersionLineInfo& line_infos);

  // Updates/replaces info for [line_num, line_num + line_count).
  void RemoveLineInfo(
      int line_num,
      int line_count);

  const LineToFileVersionLineInfo& GetLinesInfo() const {
    return file_lines_info_;
  }

 private:
  std::deque<std::string> file_lines_;
  LineToFileVersionLineInfo file_lines_info_;
  std::string commit_id_;
  int commit_index_;

  // REVIEW: Is there a better way to express this relationship?
  friend FileVersionInstanceEditor;

  friend GitDiffReaderTest;
  friend FileVersionInstanceTest;
};
