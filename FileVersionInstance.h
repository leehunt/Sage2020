#pragma once
#include <cassert>
#include <deque>
#include <filesystem>
#include <map>

#include "FileVersionDiff.h"

class GitDiffReaderTest;
class FileVersionInstanceTest;

class FileVersionLineInfo {
 public:
  FileVersionLineInfo(size_t commit_index)
      : commit_index_(commit_index) {
  }

  bool operator==(const FileVersionLineInfo& other) const {
    return commit_index_ == other.commit_index_;
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

  void PushDiff(const FileVersionDiff& diff);
  void PopDiff(const FileVersionDiff& diff);

  const std::deque<std::string>& GetLines() const { return file_lines_; }

  const FileVersionLineInfo& GetLineInfo(int line_num) const {
    return file_lines_info_[line_num - 1];
  }

  const size_t GetCommitCount() const { return commit_id_to_index_.size(); }

 private:
  // Updates/replaces lines for [line_num, line_num + line_count).
  void AddHunk(const FileVersionDiffHunk& hunk, const std::string& commit_id);
  void RemoveHunk(const FileVersionDiffHunk& hunk,
                  const std::string& commit_id);

  // Updates/replaces info for [line_num, line_num + line_count).
  void AddLineInfo(int line_num,
                   int line_count,
                   const FileVersionLineInfo& info) {
    assert(line_num > 0);
    assert(line_count > 0);
    if (line_count == 0)
      return;
    for (auto line_num_cur = line_num; line_num_cur < line_num + line_count;
         line_num_cur++) {
      file_lines_info_.insert(file_lines_info_.begin() + (line_num_cur - 1),
                              info);
    }

#if 0
    // There must always be at least a single 'end' info record (with an empty string commit_id).
    // 1. Preserve any info from [0, line_num).
    //    This automatically is done by the file_lines_info_.
    // 2. Add info from [line_num, line_num + line_count).
    //    a. add to file_lines_info_[line_num]
    //    b. ensure that file_lines_info_[line_num + line_count] = itLower.info
    // 3. Preserve any info from [line_num, end).
    // 4. Move out end record as necessary.
    auto itLower = file_lines_info_.lower_bound(line_num);
    file_lines_info_[line_num + line_count] = itLower->second;
    file_lines_info_[line_num] = info;
    for (auto it = file_lines_info_.rbegin(); it.base() != itLower; ) {
      ++it;
      auto node_handle = file_lines_info_.extract(it.base());
      node_handle.key() += line_count;
      file_lines_info_.insert(std::move(node_handle));
    }
#endif  // 0
  }

  // Updates/replaces info for [line_num, line_num + line_count).
  void RemoveLineInfo(int line_num, int line_count) {
    auto itBegin = file_lines_info_.begin() + (line_num - 1);
    file_lines_info_.erase(itBegin, itBegin + line_count);
  }

  const LineToFileVersionLineInfo& GetLinesInfo() const {
    return file_lines_info_;
  }

  size_t GetCommitIndex(const std::string& commit_id) const {
    auto it = commit_id_to_index_.find(commit_id);
    if (it == commit_id_to_index_.end()) {
      size_t new_commit_index = commit_index_to_id_.size();
      auto itInsert =
          commit_id_to_index_.insert(it, {commit_id, new_commit_index});
      assert(std::find(commit_index_to_id_.cbegin(), commit_index_to_id_.cend(),
                       &itInsert->first) == commit_index_to_id_.cend());
      commit_index_to_id_.push_back(&itInsert->first);
      assert(std::find(commit_index_to_id_.cbegin(), commit_index_to_id_.cend(),
                       &itInsert->first) != commit_index_to_id_.cend());
      assert(GetCommitIndex(commit_id) == new_commit_index);
      return new_commit_index;
    }
    return it->second;
  }

  const std::string& GetCommitFromIndex(size_t index) {
    return *commit_index_to_id_[index];
  }

  std::string PopCommit() {
    assert(commit_index_to_id_.size());
    auto newest_commit_ptr = const_cast<std::string *>(commit_index_to_id_.back());
    commit_index_to_id_.pop_back();
    auto newest_commit_index = commit_index_to_id_.size();

    auto newest_commit = std::move(*newest_commit_ptr);
    auto it = commit_id_to_index_.find(newest_commit);
    assert(it != commit_id_to_index_.end());
    assert(it->first == newest_commit);
    assert(it->second == newest_commit_index);
    commit_id_to_index_.erase(it);

    return newest_commit;
  }

 private:
  std::deque<std::string> file_lines_;
  LineToFileVersionLineInfo file_lines_info_;
  mutable std::map<std::string, size_t> commit_id_to_index_;
  mutable std::deque<const std::string *> commit_index_to_id_;

  friend GitDiffReaderTest;
  friend FileVersionInstanceTest;
};
