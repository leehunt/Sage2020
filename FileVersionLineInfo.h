#pragma once

#include <assert.h>
#include <map>

#include "GitHash.h"

class SparseIndexArray;
class FileVersionInstanceTest;
class FileVersionLineInfo {
 public:
  FileVersionLineInfo(const char commit_sha[41]) {
    strcpy_s(commit_sha_, commit_sha);
    // It important that every added commit is not is_eof(), as that is a
    // reserved value at the end of any sparse array.
    assert(!is_eof());
  }
  // Ending terminator (special case).
  FileVersionLineInfo() { commit_sha_[0] = 0; }

  bool operator==(const FileVersionLineInfo& other) const {
    return !strcmp(commit_sha_, other.commit_sha_);
  }

  // For testing/contaier use only.
  bool operator<(const FileVersionLineInfo& other) const {
    return strcmp(commit_sha_, other.commit_sha_) < 0;
  }

  const char* commit_sha() const { return commit_sha_; };

  bool is_eof() const { return !commit_sha_[0]; }

 private:
  char commit_sha_[41];

  friend class SparseIndexArray;
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

  size_t Size() const { return size(); }

  SparseIndexArray::iterator SubtractMapKey(SparseIndexArray::iterator it,
                                            size_t to_subtract);

  bool operator==(const SparseIndexArray& other) const {
    return *static_cast<const std::map<size_t, FileVersionLineInfo>*>(this) ==
           static_cast<const std::map<size_t, FileVersionLineInfo>>(other);
  }

 private:
  const_iterator LowerFloor(size_t index) const;

  friend class FileVersionInstanceTest;
};

// A one-based file line -> FileVersionLineInfo mapping.
// typedef std::map<int, FileVersionLineInfo> LineToFileVersionLineInfo;
#if USE_SPARSE_INDEX_ARRAY
typedef SparseIndexArray LineToFileVersionLineInfo;
#else
typedef std::deque<FileVersionLineInfo> LineToFileVersionLineInfo;
#endif
