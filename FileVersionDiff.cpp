#include "pch.h"

#include "FileVersionDiff.h"

#include <deque>

#ifdef _DEBUG
bool FileVersionDiffHunkRange::operator==(
    const FileVersionDiffHunkRange& other) const {
  if (this == &other)
    return true;
  if (location_ != other.location_)
    return false;
  if (count_ != other.count_)
    return false;

  return true;
}
bool FileVersionDiffHunkRange::operator!=(
    const FileVersionDiffHunkRange& other) const {
  return !(*this == other);
}
#endif  // _DEBUG

#ifdef _DEBUG
bool FileVersionCombinedDiffHunk::operator==(
    const FileVersionCombinedDiffHunk& other) const {
  if (this == &other)
    return true;
  if (ranges_ != other.ranges_)
    return false;
  if (start_context_ != other.start_context_)
    return false;
  if (lines_ != other.lines_)
    return false;
  if ((!line_info_to_restore_ != !other.line_info_to_restore_) ||
      (line_info_to_restore_ &&
       *line_info_to_restore_ != *other.line_info_to_restore_)) {
    return false;
  }

  return true;
}
bool FileVersionCombinedDiffHunk::operator!=(
    const FileVersionCombinedDiffHunk& other) const {
  return !(*this == other);
}
#endif  // _DEBUG

#ifdef _DEBUG
bool FileVersionDiffHunk::operator==(const FileVersionDiffHunk& other) const {
  if (this == &other)
    return true;
  if (to_location_ != other.to_location_)
    return false;
  if (to_line_count_ != other.to_line_count_)
    return false;
  if (to_lines_ != other.to_lines_)
    return false;
  if (from_location_ != other.from_location_)
    return false;
  if (from_line_count_ != other.from_line_count_)
    return false;
  if (from_lines_ != other.from_lines_)
    return false;
  if (start_context_ != other.start_context_)
    return false;
  if ((!line_info_to_restore_ != !other.line_info_to_restore_) ||
      (line_info_to_restore_ &&
       *line_info_to_restore_ != *other.line_info_to_restore_)) {
    return false;
  }

  return true;
}
bool FileVersionDiffHunk::operator!=(const FileVersionDiffHunk& other) const {
  return !(*this == other);
}
#endif  // _DEBUG

#ifdef _DEBUG
bool FileVersionDiffTree::operator==(const FileVersionDiffTree& other) const {
  if (this == &other)
    return true;
  if (old_mode != other.old_mode)
    return false;
  if (strcmp(old_hash_string, other.old_hash_string))
    return false;
  if (new_mode != other.new_mode)
    return false;
  if (strcmp(new_hash_string, other.new_hash_string))
    return false;
  if (action != other.action)
    return false;
  if (strcmp(file_path, other.file_path))
    return false;

  return true;
}
bool FileVersionDiffTree::operator!=(const FileVersionDiffTree& other) const {
  return !(*this == other);
}
#endif  // _DEBUG

#ifdef _DEBUG
bool FileVersionDiffParent::operator==(
    const FileVersionDiffParent& other) const {
  if (this == &other)
    return true;
  if (commit_ != other.commit_)
    return false;
#if OLD_DIFFS
  if (add_hunks_ != other.add_hunks_)
    return false;
#endif
  if (!file_version_diffs_ != !other.file_version_diffs_)
    return false;
  if (file_version_diffs_ &&
      (*file_version_diffs_ != *other.file_version_diffs_)) {
    return false;
  }

  return true;
}
bool FileVersionDiffParent::operator!=(
    const FileVersionDiffParent& other) const {
  return !(*this == other);
}
#endif  // _DEBUG

#ifdef _DEBUG
bool FileVersionDiffChild::operator==(const FileVersionDiffChild& other) const {
  if (this == &other)
    return true;

  if (commit_ != other.commit_)
    return false;

  return true;
}
bool FileVersionDiffChild::operator!=(const FileVersionDiffChild& other) const {
  return !(*this == other);
}
#endif  // _DEBUG

#ifdef _DEBUG
bool NameEmailTime::operator==(const NameEmailTime& other) const {
  if (this == &other)
    return true;
  if (name_ != other.name_)
    return false;
  if (email_ != other.email_)
    return false;
  if (memcmp(&time_, &other.time_, sizeof(time_)))
    return false;

  return true;
}
bool NameEmailTime::operator!=(const NameEmailTime& other) const {
  return !(*this == other);
}
#endif  // _DEBUG

#ifdef _DEBUG
bool FileVersionDiff::operator==(const FileVersionDiff& other) const {
  if (commit_ != other.commit_)
    return false;
  if (parents_ != other.parents_)
    return false;
#if OLD_DIFFS
  if (remove_hunks_ != other.remove_hunks_)
    return false;
#else
  if (hunks_ != other.hunks_)
    return false;
#endif
  if (author_ != other.author_)
    return false;
  if (diff_command_ != other.diff_command_)
    return false;
  if (committer_ != other.committer_)
    return false;
  if (comment_ != other.comment_)
    return false;
  if (tree_ != other.tree_)
    return false;
  if (diff_tree_ != other.diff_tree_)
    return false;
  if (index_ != other.index_)
    return false;
  if (commit_ != other.commit_)
    return false;
  if (path_ != other.path_)
    return false;

  return true;
}
bool FileVersionDiff::operator!=(const FileVersionDiff& other) const {
  return !(*this == other);
}
#endif  // _DEBUG
