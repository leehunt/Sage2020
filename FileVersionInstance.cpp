#include "pch.h"

#include <cassert>
#include "FileVersionInstance.h"

FileVersionInstance::FileVersionInstance() : commit_index_(-1) {}

FileVersionInstance::FileVersionInstance(std::deque<std::string>& lines,
                                         const std::string& commit_id)
    : commit_index_(0), commit_id_(commit_id) {
  AddLineInfo(1, static_cast<int>(lines.size()),
              FileVersionLineInfo(commit_index_));
  for (auto& line : lines) {
    file_lines_.push_back(std::move(line));
  }
}

void FileVersionInstance::AddLineInfo(int line_num,
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

void FileVersionInstance::RemoveLineInfo(int line_num, int line_count) {
  auto itBegin = file_lines_info_.begin() + (line_num - 1);
  file_lines_info_.erase(itBegin, itBegin + line_count);
}

std::set<FileVersionLineInfo> FileVersionInstance::GetVersionsFromLines(
    int iStart,
    int iEnd) const {
  std::set<FileVersionLineInfo> line_info;
  for (int iLine = iStart; iLine < iEnd; iLine++) {
    line_info.insert(GetLineInfo(iLine));
  }

  return line_info;
}