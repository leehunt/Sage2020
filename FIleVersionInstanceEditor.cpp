#include "pch.h"

#include "FileVersionInstanceEditor.h"
#include "Sage2020ViewDocListener.h"

void FileVersionInstanceEditor::AddDiff(const FileVersionDiff& diff) {
  assert(file_version_instance_.commit_index_ >= -1);
  file_version_instance_.commit_index_++;
  assert(file_version_instance_.commit_index_ >= 0);

  for (auto& hunk : diff.hunks_) {
    AddHunk(hunk);
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfVersionChange(
        file_version_instance_.commit_index_);
  }
}

void FileVersionInstanceEditor::RemoveDiff(const FileVersionDiff& diff) {
  assert(file_version_instance_.commit_index_ >= 0);
  file_version_instance_.commit_index_--;
  assert(file_version_instance_.commit_index_ >= -1);

  for (auto it = diff.hunks_.crbegin(); it != diff.hunks_.crend(); it++) {
    RemoveHunk(*it);
  }

#if _DEBUG
#if USE_SPARSE_INDEX_ARRAY
  for (int line_index = 0;
       line_index < file_version_instance_.file_lines_info_.MaxLineIndex(); line_index++) {
    auto info = file_version_instance_.file_lines_info_.GetLineInfo(line_index);
    assert(static_cast<int>(info.commit_index()) <=
           file_version_instance_.commit_index_);
  }
#else
  for (size_t line_index = 0;
       line_index < file_version_instance_.file_lines_info_.size();
       line_index++) {
    auto& info = file_version_instance_.file_lines_info_[line_index];
    assert(static_cast<int>(info.commit_index()) <= file_version_instance_.commit_index_);
  }
#endif
#endif

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfVersionChange(
        file_version_instance_.commit_index_);
  }
}

bool FileVersionInstanceEditor::GoToCommit(
    const GitHash& commit,
    const std::vector<FileVersionDiff>& diffs) {
  bool edited = false;

  for (size_t commit_index = 0; commit_index < diffs.size(); commit_index++) {
    if (diffs[commit_index].commit_ == commit) {
      edited = GoToIndex(commit_index, diffs);
      break;
    }
  }

  return edited;
}

  bool FileVersionInstanceEditor::GoToIndex(
    size_t commit_index,
    const std::vector<FileVersionDiff>& diffs) {
  if (!diffs.size())
    return false;

  if (commit_index >= diffs.size())
    commit_index = diffs.size() - 1;

  bool edited = false;
  while (file_version_instance_.commit_index_ >
         static_cast<int>(commit_index)) {
    // N.b. Make a 'diff' copy since commit_index_ is modfied by RemoveDiff().
    const auto& diff = diffs[file_version_instance_.commit_index_];
    RemoveDiff(diff);
    edited = true;
  }
  while (file_version_instance_.commit_index_ <
         static_cast<int>(commit_index)) {
    // N.b. Make a 'diff' copy since commit_index_ is modfied by AddDiff().
    const auto& diff = diffs[file_version_instance_.commit_index_ + 1];
    AddDiff(diff);
    edited = true;
  }

  return edited;
}

void FileVersionInstanceEditor::AddHunk(
    const FileVersionDiffHunk& hunk) {

  // Remove lines.  N.b. Remove must be done first to get correct line
  // locations. Yes, using |add_location_| seems odd, but the add_location
  // tracks the new position of any adds/removed done by previous hunks in the
  // diff.
  if (hunk.remove_line_count_ > 0) {
    // This is tricky.  The 'add_location' is where to add in the 'to'
    // lines are found if diffs are added sequentially (which is how we use
    // AddHunk).  However if there is no add lines to replace the removed lines,
    // then the 'add_location' is shifted down by one because that line no
    // longer exists in the 'to' file.
    auto remove_location_index =
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_;
    assert(std::equal(
        file_version_instance_.file_lines_.begin() + remove_location_index,
        file_version_instance_.file_lines_.begin() + remove_location_index +
            hunk.remove_line_count_,
        hunk.remove_lines_.begin()));
    file_version_instance_.file_lines_.erase(
        file_version_instance_.file_lines_.begin() + remove_location_index,
        file_version_instance_.file_lines_.begin() + remove_location_index +
            hunk.remove_line_count_);

    if (!hunk.line_info_to_restore_) {
      hunk.line_info_to_restore_ = std::make_unique<LineToFileVersionLineInfo>();
      hunk.line_info_to_restore_->insert(
          hunk.line_info_to_restore_->begin(),
          file_version_instance_.file_lines_info_.begin() +
              remove_location_index,
          file_version_instance_.file_lines_info_.begin() +
              remove_location_index + hunk.remove_line_count_);
    }
    file_version_instance_.RemoveLineInfo(remove_location_index + 1,
                                          hunk.remove_line_count_);
  }

  // Add lines.
  if (hunk.add_line_count_ > 0) {
    auto it_insert =
        file_version_instance_.file_lines_.begin() + (hunk.add_location_ - 1);
    for (const auto& line : hunk.add_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstanceEditor.
      it_insert = file_version_instance_.file_lines_.insert(it_insert, line);
      ++it_insert;
    }
    // Add line info
    auto file_version_line_info = FileVersionLineInfo{
        static_cast<size_t>(file_version_instance_.commit_index_)};
    
    LineToFileVersionLineInfo single_infos;
    single_infos.push_front(file_version_line_info);
    file_version_instance_.AddLineInfo(hunk.add_location_, hunk.add_line_count_,
                                       single_infos);
#if _DEBUG
    for (auto line_num = hunk.add_location_;
         line_num < hunk.add_location_ + hunk.add_line_count_; line_num++) {
      //  assert(file_version_instance_.GetCommitFromIndex(
      //            file_version_instance_.GetLineInfo(line_num).commit_index())
      //            ==
      //         commit_id);
    }
#endif
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfEdit(
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_,
        hunk.add_line_count_ - hunk.remove_line_count_);
  }
}

void FileVersionInstanceEditor::RemoveHunk(const FileVersionDiffHunk& hunk) {
  // Remove lines by removing the "added" lines.
  // N.b. Remove must be done first to get correct line locations.
  // Yes, using |add_location_| seems odd, but the add_location tracks the
  // new position of any adds/removed done by previous hunks in the diff.
  if (hunk.add_line_count_ > 0) {
    assert(std::equal(
        file_version_instance_.file_lines_.begin() + (hunk.add_location_ - 1),
        file_version_instance_.file_lines_.begin() +
            (hunk.add_location_ - 1 + hunk.add_line_count_),
        hunk.add_lines_.begin()));
    file_version_instance_.file_lines_.erase(
        file_version_instance_.file_lines_.begin() + (hunk.add_location_ - 1),
        file_version_instance_.file_lines_.begin() +
            (hunk.add_location_ - 1 + hunk.add_line_count_));

    file_version_instance_.RemoveLineInfo(
        hunk.add_location_, hunk.add_line_count_);
  }

  // Add lines by adding the "removed" lines.
  if (hunk.remove_line_count_ > 0) {
    // This is tricky.  The 'add_location' is where to add in the 'to'
    // lines are found if diffs are removed sequentially in reverse (which is
    // how we use RemoveHunk).  However if there is no add lines to replace the
    // removed lines, then the 'add_location' is shifted down by one because
    // that line no longer exists in the 'to' file.
    auto add_location_index =
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_;
    auto it_insert =
        file_version_instance_.file_lines_.begin() + add_location_index;
    for (const auto& line : hunk.remove_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstanceEditor.
      it_insert = file_version_instance_.file_lines_.insert(it_insert, line);
      ++it_insert;
    }

    // Restore line info
    assert(hunk.line_info_to_restore_);
    file_version_instance_.AddLineInfo(add_location_index + 1,
                                       hunk.remove_line_count_,
                                       *hunk.line_info_to_restore_);
#if _DEBUG
    for (auto line_index = add_location_index;
         line_index < add_location_index + hunk.remove_line_count_;
         line_index++) {
      //  assert(file_version_instance_.GetCommitFromIndex(
      //          file_version_instance_.GetLineInfo(line_index).commit_index())
      //          ==
      //         commit_id);
    }
#endif
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfEdit(
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_,
        hunk.remove_line_count_ - hunk.add_line_count_);
  }
}
