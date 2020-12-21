#include "pch.h"

#include "FileVersionInstanceEditor.h"

void FileVersionInstanceEditor::AddDiff(const FileVersionDiff& diff) {
  for (auto& hunk : diff.hunks_) {
    AddHunk(hunk);
  }
  file_version_instance_.commit_index_++;
}

void FileVersionInstanceEditor::RemoveDiff(const FileVersionDiff& diff) {
  file_version_instance_.commit_index_--;
  for (auto it = diff.hunks_.crbegin(); it != diff.hunks_.crend(); it++) {
    RemoveHunk(*it);
  }
}

void FileVersionInstanceEditor::AddHunk(const FileVersionDiffHunk& hunk) {
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

    file_version_instance_.RemoveLineInfo(remove_location_index,
                                         hunk.remove_line_count_);
  }

  auto file_version_line_info = FileVersionLineInfo {static_cast<size_t>(file_version_instance_.commit_index_)};

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
    file_version_instance_.AddLineInfo(hunk.add_location_, hunk.add_line_count_,
                std::move(file_version_line_info));
#if _DEBUG
    for (auto line_num = hunk.add_location_;
         line_num < hunk.add_location_ + hunk.add_line_count_; line_num++) {
    //  assert(file_version_instance_.GetCommitFromIndex(
    //            file_version_instance_.GetLineInfo(line_num).commit_index()) ==
    //         commit_id);
    }
#endif
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

    file_version_instance_.RemoveLineInfo(hunk.add_location_,
                                         hunk.add_line_count_);
  }

  auto file_version_line_info =
      FileVersionLineInfo{static_cast<size_t>(file_version_instance_.commit_index_)};

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
    // Add line info
    file_version_instance_.AddLineInfo(add_location_index,
                                      hunk.remove_line_count_,
                std::move(file_version_line_info));
#if _DEBUG
    for (auto line_index = add_location_index;
         line_index < add_location_index + hunk.remove_line_count_;
         line_index++) {
    //  assert(file_version_instance_.GetCommitFromIndex(
    //          file_version_instance_.GetLineInfo(line_index).commit_index()) ==
    //         commit_id);
    }
#endif
  }
}
