#pragma once
#include <cassert>
#include <deque>
#include <filesystem>
#include <map>

#include "FileVersionDiff.h"

struct FileVersionLineInfo {
  std::string commit_id;

  bool operator==(const FileVersionLineInfo& other) const {
    return commit_id == other.commit_id;
  }
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

  void AddHunk(const FileVersionDiffHunk& hunk, const std::string& commit_id);
  void RemoveHunk(const FileVersionDiffHunk& hunk,
                  const std::string& commit_id);

  const std::deque<std::string>& GetLines() const { return file_lines_; }

  const LineToFileVersionLineInfo& GetLinesInfo() const {
    return file_lines_info_;
  }
  const FileVersionLineInfo& GetLineInfo(int line_num) const {
    return file_lines_info_[line_num - 1];
  }

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

 private:
  std::deque<std::string> file_lines_;
  LineToFileVersionLineInfo file_lines_info_;
};
