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
#if USE_SPARSE_INDEX_ARRAY
  file_lines_info_.Add(line_num - 1, line_count, info);
#else
  for (auto line_num_cur = line_num; line_num_cur < line_num + line_count;
       line_num_cur++) {
    file_lines_info_.insert(file_lines_info_.begin() + (line_num_cur - 1),
                            info);
  }
#endif
}

void FileVersionInstance::RemoveLineInfo(int line_num, int line_count) {
#if USE_SPARSE_INDEX_ARRAY
  file_lines_info_.Remove(line_num - 1, line_count);
#else
  auto itBegin = file_lines_info_.begin() + (line_num - 1);
  file_lines_info_.erase(itBegin, itBegin + line_count);
#endif
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

SparseIndexArray::const_iterator SparseIndexArray::LowerFloor(size_t index) const {
  auto it = upper_bound(index);
  if (it == end()) {
    // Out of range, return ending marker.
    it = --end();
  } else if (it != begin()) {
    assert(it != begin());
    --it;
  } else {
    // Where's the ending marker?
    assert(it->second.commit_index() == static_cast<size_t>(-1));
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
  assert(std::prev(end())->second == FileVersionLineInfo());
  assert(cbegin()->first == 0);

  if (line_count == 0)
    return;

  // Pull all higher items up first so that any inserts done later will iterated
  // correctly.
  // Use forward iterators to skip extra deference (and they're move
  // intuitative when using .base()).
  auto it = end();
  for (; it != begin();) {
    --it;
    if (it->first < line_index)
      break;
    auto node_handle = extract(it);
    node_handle.key() += line_count;
    it = insert(begin(), std::move(node_handle));
  }
  assert(it != end());

  // Add the item.
  if (it->second.commit_index() != line_info.commit_index()) {
    emplace(std::make_pair(std::size_t{line_index}, line_info));
  } else if (it->first > line_index) {
    // Extend range to line_index.
    auto node_handle = extract(it);
    node_handle.key() = line_index;
    insert(begin(), std::move(node_handle));
  } else {
    // Nothing to do; the range is already that commit index.
    assert(it->first < line_index);
  }

  assert(std::prev(end())->second == FileVersionLineInfo());
  assert(cbegin()->first == 0);
}

void SparseIndexArray::Remove(size_t line_index, size_t line_count) {
  assert(std::prev(end())->second == FileVersionLineInfo());
  assert(cbegin()->first == 0);

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

  auto itLower = lower_bound(line_index);
  auto itUpper = std::prev(upper_bound(line_index + line_count));
  assert(itUpper != end());
  
  if (itLower->first < itUpper->first) {
    erase(itLower, itUpper);
    // N.B. itLower may now be invalid.
  }

  // Trim any trailing range.
  assert(line_index + line_count >= itUpper->first);
  size_t incursion = line_index + line_count - itUpper->first;
  if (incursion > 0) {
    if (incursion != line_count) {
      auto node_handle = extract(itUpper);
      node_handle.key() -= line_count - incursion;
      itUpper = insert(begin(), std::move(node_handle));
    }
    ++itUpper;
  }

  // Move all higher items down.
  for (; itUpper != end(); ++itUpper) {
    auto node_handle = extract(itUpper);
    node_handle.key() -= line_count;
    itUpper = insert(begin(), std::move(node_handle));
  }

  assert(std::prev(end())->second == FileVersionLineInfo());
  assert(cbegin()->first == 0);
}
