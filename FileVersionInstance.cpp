#include "pch.h"

#include <cassert>
#include "FileVersionInstance.h"

FileVersionInstance::FileVersionInstance() {}

FileVersionInstance::FileVersionInstance(std::deque<std::string>& lines,
                                         const GitHash& commit)
    : commit_(commit) {
  if (commit_.IsValid()) {
    LineToFileVersionLineInfo infos;
    infos.emplace_back(FileVersionLineInfo(commit_.sha_));
    AddLineInfo(1, static_cast<int>(lines.size()), infos);
    for (auto& line : lines) {
      file_lines_.push_back(std::move(line));
    }
  } else {
    assert(lines.empty());
  }
}

const GitHash& FileVersionInstance::GetCommit() const {
#if _DEBUG
  // Special case: a completely empty path can still have a valid commit_.
  if (!commit_path_.empty()) {
    auto path_copy = commit_path_;
    do {
      const auto& current_item = path_copy.back();
      assert(current_item.branch());
      if (current_item.currentBranchIndex() != -1)
        break;
      path_copy.pop_back();
    } while (!path_copy.empty());
    if (!path_copy.empty()) {
      const auto& current_item = path_copy.back();
      const auto& diffs = *current_item.branch();
      const auto& index_diff = diffs[current_item.currentBranchIndex()];
      assert(commit_ == index_diff.commit_);
    } else {
      assert(!commit_.IsValid());
    }
  }
#endif  // _DEBUG
  return commit_;
}

int FileVersionInstance::GetCommitIndex() const {
  return commit_path_;
}
const std::vector<FileVersionDiff>* FileVersionInstance::GetBranchDiffs()
    const {
  if (commit_path_.empty())
    return nullptr;
  return commit_path_.DiffsSubbranch();
}

const std::deque<std::string>& FileVersionInstance::GetLines() const {
  return file_lines_;
}

const FileVersionLineInfo& FileVersionInstance::GetLineInfo(
    int line_index) const {
#if USE_SPARSE_INDEX_ARRAY
  return file_lines_info_.Get(line_index);
#else
  return file_lines_info_[line_index];
#endif
}

void FileVersionInstance::AddLineInfo(
    int line_num,
    int line_count,
    const LineToFileVersionLineInfo& line_infos) {
  assert(line_num > 0);
  assert(line_count >= 0);  // N.b. When adding a null parent commit, the number
                            // of lines can be zero.
  if (line_count == 0)
    return;

  if (line_infos.size() == 1) {
#if USE_SPARSE_INDEX_ARRAY
    file_lines_info_.Add(line_num - 1, line_count, line_infos[0]);
#else
    for (auto line_num_cur = line_num; line_num_cur < line_num + line_count;
         line_num_cur++) {
      file_lines_info_.insert(file_lines_info_.begin() + (line_num_cur - 1),
                              line_infos[0]);
    }
#endif
  } else {
    for (auto i = 0; i < line_count; i++) {
#if USE_SPARSE_INDEX_ARRAY
      file_lines_info_.Add(line_num - 1 + i, 1, line_infos[i]);
#else
      file_lines_info_.insert(file_lines_info_.begin() + (line_num - 1) + i,
                              line_infos[i]);
#endif
    }
  }
}

void FileVersionInstance::RemoveLineInfo(int line_num, int line_count) {
#if USE_SPARSE_INDEX_ARRAY
  file_lines_info_.Remove(line_num - 1, line_count);
#else
  auto itBegin = file_lines_info_.begin() + (line_num - 1);
  file_lines_info_.erase(itBegin, itBegin + line_count);
#endif
}

const LineToFileVersionLineInfo& FileVersionInstance::GetLinesInfo() const {
  return file_lines_info_;
}

// Note: can be expensive; use only for testing.
FileVersionInstance& FileVersionInstance::operator=(
    const FileVersionInstance& source) {
  file_lines_ = source.file_lines_;
  file_lines_info_ = source.file_lines_info_;
  commit_ = source.commit_;

  return *this;
}

// Note: can be expensive; use only for testing.
bool FileVersionInstance::operator==(const FileVersionInstance& source) const {
  if (commit_ != source.commit_)
    return false;
  if (file_lines_info_ != source.file_lines_info_)
    return false;
  if (file_lines_ != source.file_lines_)
    return false;

  return true;
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

SparseIndexArray::const_iterator SparseIndexArray::LowerFloor(
    size_t index) const {
  auto it = upper_bound(index);
  if (it == begin()) {
    // Ensure that the ending marker is present.
    assert(it->second.is_eof());
  } else {
    --it;
  }
  return it;
}

const FileVersionLineInfo& SparseIndexArray::Get(size_t index) const {
  return LowerFloor(index)->second;
}

SparseIndexArray::SparseIndexArray() {
  // Add the ending marker.
  emplace(std::make_pair(std::size_t{0}, FileVersionLineInfo()));
}

void SparseIndexArray::Add(size_t line_index,
                           size_t line_count,
                           const FileVersionLineInfo& line_info) {
  assert(cbegin()->first == 0);
  assert(std::prev(end())->second == FileVersionLineInfo());
  assert(!line_info.is_eof());

  if (line_count == 0)
    return;

  // Pull all higher items up first so that any inserts done later will
  // iterated correctly. Use forward iterators to skip extra deference (and
  // they're move intuitative when using .key()).
  auto it = end();
  assert(it != begin());  // There should always be at least the terminator.
  for (; it != begin();) {
    --it;
    if (it->first < line_index)
      break;
    // Update the node's key to the new shifted down position.
    auto node_handle = extract(it);
    node_handle.key() += line_count;
    it = insert(begin(), std::move(node_handle));
  }

  assert(it != end());

  // Add the item.
  if (strcmp(it->second.commit_sha(), line_info.commit_sha())) {
    // Different sha: insert new item.
    const auto itInsertHint = std::next(it);
    bool need_to_split = itInsertHint != end() &&
                         it->first < line_index + line_count &&
                         itInsertHint->first > line_index + line_count;
    auto itNewElement = emplace_hint(
        itInsertHint, line_index,
        line_info);  // REVIEW: Should |itInsertHint| instead be |it|?
    // Split current element as necessary.
    if (need_to_split) {
      assert(!it->second.is_eof());
      emplace_hint(itNewElement, line_index + line_count, it->second);
    }
  } else if (it->first > line_index) {
    assert(!it->second.is_eof());
    const auto itInsertHint = std::next(it);
    // Extend range to line_index.
    auto node_handle = extract(it);
    node_handle.key() = line_index;
    insert(itInsertHint,
           std::move(
               node_handle));  // REVIEW: Should |itInsertHint| instead be |it|?
  } else {
    // Nothing to do; the range is already that commit index.
    assert(it->first < line_index);
  }

  assert(cbegin()->first == 0);
  assert(std::prev(end())->second == FileVersionLineInfo());
}

SparseIndexArray::iterator SparseIndexArray::SubtractMapKey(
    SparseIndexArray::iterator it,
    size_t to_subtract) {
  const auto itInsertHint = std::next(it);
  auto node_handle = extract(it);
  node_handle.key() -= to_subtract;
  return insert(itInsertHint, std::move(node_handle));
}

void SparseIndexArray::Remove(size_t line_index, size_t line_count) {
  assert(cbegin()->first == 0);
  assert(std::prev(end())->second == FileVersionLineInfo());

  if (line_count == 0)
    return;

  // Ensure ending marker is present.
  assert(cend() != cbegin());
  if (IsEmpty()) {
    // Attempt to delete empty collection.
    assert(false);
    return;
  }
  if (MaxLineIndex() < line_index + line_count) {
    // Attempt to delete outside of collection.
    assert(false);
    return;
  }

  auto it = upper_bound(line_index);
  assert(it != end());
  auto itPrev = std::prev(it);
  assert(itPrev != end());

  // Trim any leading range.
  // N.b. if "itPrev->first == line_index" then it will be deleted (if
  // line_count is long enough) or offset (if not) below.
  if (itPrev->first < line_index && line_index < it->first) {
    auto to_trim = min(it->first - line_index, line_count);
    it = SubtractMapKey(it, to_trim);
    itPrev = it;
    ++it;
  }

  if (it != end()) {
    // Check for any contiguous range(s) to delete.
    if (line_index == itPrev->first && it->first <= line_index + line_count) {
      auto itLim = std::prev(upper_bound(line_index + line_count));
      assert(itLim != end());
      assert(itPrev->first <= itLim->first);
      it = erase(itPrev, itLim);
      assert(it == itLim);

      // Merge any previously split item.
      if (it != cbegin()) {
        itPrev = std::prev(it);
        if (itPrev->second == it->second) {
          itLim = std::next(it);
          assert(itLim != end());  // There should always be at least a
                                   // terminator at the end, or terminators
                                   // will never merge.
          it = erase(it, itLim);
        }
      }
    }

    // Offset any remaining range(s).
    if (it != end()) {
      // Trim any trailing range.
      if (line_index + line_count > it->first) {
        auto to_subtract = it->first - line_index;
        assert(to_subtract < line_count);
        it = SubtractMapKey(it, to_subtract);
        ++it;
      }

      // Offset any full remaining ranges.
      while (it != end()) {
        auto to_offset = line_count;
        it = SubtractMapKey(it, to_offset);
        ++it;
      }
    }
  }

  assert(cbegin()->first == 0);
  assert(std::prev(end())->second == FileVersionLineInfo());
}
