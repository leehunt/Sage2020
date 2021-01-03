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

SparseIndexArray::const_iterator SparseIndexArray::LowerFloor(
    size_t index) const {
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
  assert(cbegin()->first == 0);
  assert(std::prev(end())->second == FileVersionLineInfo());

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
    auto itInsert = std::next(it);
    emplace_hint(itInsert, std::make_pair(std::size_t{line_index}, line_info));
  } else if (it->first > line_index) {
    auto itInsert = std::next(it);
    // Extend range to line_index.
    auto node_handle = extract(it);
    node_handle.key() = line_index;
    insert(itInsert, std::move(node_handle));
  } else {
    // Nothing to do; the range is already that commit index.
    assert(it->first < line_index);
  }

  assert(cbegin()->first == 0);
  assert(std::prev(end())->second == FileVersionLineInfo());
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
    auto to_erase = min(it->first - line_index, line_count);
    auto itNext = std::next(it);
    auto node_handle = extract(it);
    node_handle.key() -= to_erase;
    it = insert(itNext, std::move(node_handle));
    itPrev = it;
    ++it;
  }

  // Check for a range to delete.
  if (it != end()) {
    if (line_index == itPrev->first && it->first <= line_index + line_count) {
      auto itLim = std::prev(upper_bound(line_index + line_count));
      assert(itLim != end());
      assert(itPrev->first <= itLim->first);
      it = erase(itPrev, itLim);
      assert(it == itLim);
      itPrev = it;
    }

    // Offset range(s).
    if (it != end()) {
      // Check for partial-range case.
      if (line_index + line_count > it->first) {
        auto to_offset = it->first - line_index;
        assert(to_offset < line_count);
        auto itNext = std::next(it);
        auto node_handle = extract(it);
        node_handle.key() -= to_offset;
        it = insert(itNext, std::move(node_handle));
        itPrev = it;
        ++it;
      }

      // Offset any full remaining ranges.
      while (it != end()) {
        auto to_offset = line_count;
        auto itNext = std::next(it);
        auto node_handle = extract(it);
        node_handle.key() -= to_offset;
        it = insert(itNext, std::move(node_handle));
        itPrev = it;
        ++it;
      }
    }
  }

  assert(cbegin()->first == 0);
  assert(std::prev(end())->second == FileVersionLineInfo());
}
