#include "pch.h"

#include <cassert>
#include "FileVersionInstance.h"

FileVersionInstance::FileVersionInstance() {}

FileVersionInstance::FileVersionInstance(std::deque<std::string>& lines,
                                         const std::string& commit_id) {
  AddLineInfo(1, static_cast<int>(lines.size()),
              FileVersionLineInfo{commit_id});
  for (auto& line : lines) {
    file_lines_.push_back(std::move(line));
  }
}

void FileVersionInstance::AddHunk(const FileVersionDiffHunk& hunk,
                                  const std::string& commit_id) {
  // Remove lines.  N.b. Remove must be done first to get correct line
  // locations. Yes, using |add_location_| seems odd, but the add_location
  // tracks the new position of any adds/removed done by previous hunks in the
  // diff.
  if (hunk.remove_line_count_ > 0) {
    assert(std::equal(file_lines_.begin() + (hunk.add_location_ - 1),
                      file_lines_.begin() +
                          (hunk.add_location_ - 1 + hunk.remove_line_count_),
                      hunk.remove_lines_.begin()));
    file_lines_.erase(file_lines_.begin() + (hunk.add_location_ - 1),
                      file_lines_.begin() +
                          (hunk.add_location_ - 1 + hunk.remove_line_count_));

    RemoveLineInfo(hunk.add_location_, hunk.remove_line_count_);
  }

  // Add lines.
  if (hunk.add_line_count_ > 0) {
    auto it_insert = file_lines_.begin() + (hunk.add_location_ - 1);
    for (const auto& line : hunk.add_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstance.
      it_insert = file_lines_.insert(it_insert, line);
      ++it_insert;
    }
    // Add line info
    AddLineInfo(hunk.add_location_, hunk.add_line_count_,
                FileVersionLineInfo{commit_id});
#if _DEBUG
    for (auto line_num = hunk.add_location_;
         line_num < hunk.add_location_ + hunk.add_line_count_; line_num++) {
      assert(GetLineInfo(line_num).commit_id == commit_id);
    }
#endif
  }
}

void FileVersionInstance::RemoveHunk(const FileVersionDiffHunk& hunk,
                                     const std::string& commit_id) {
  // Remove lines by removing the "added" lines.
  // N.b. Remove must be done first to get correct line locations.
  // Yes, using |add_location_| seems odd, but the add_location tracks the
  // new position of any adds/removed done by previous hunks in the diff.
  if (hunk.add_line_count_ > 0) {
    assert(std::equal(
        file_lines_.begin() + (hunk.add_location_ - 1),
        file_lines_.begin() + (hunk.add_location_ - 1 + hunk.add_line_count_),
        hunk.add_lines_.begin()));
    file_lines_.erase(
        file_lines_.begin() + (hunk.add_location_ - 1),
        file_lines_.begin() + (hunk.add_location_ - 1 + hunk.add_line_count_));

    RemoveLineInfo(hunk.add_location_, hunk.add_line_count_);
  }

  // Add lines by adding the "removed" lines.
  if (hunk.remove_line_count_ > 0) {
    auto it_insert = file_lines_.begin() + (hunk.add_location_ - 1);
    for (const auto& line : hunk.remove_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstance.
      it_insert = file_lines_.insert(it_insert, line);
      ++it_insert;
    }
    // Add line info
    AddLineInfo(hunk.add_location_, hunk.remove_line_count_,
                FileVersionLineInfo{commit_id});
#if _DEBUG
    for (auto line_num = hunk.add_location_;
         line_num < hunk.add_location_ + hunk.remove_line_count_; line_num++) {
      assert(GetLineInfo(line_num).commit_id == commit_id);
    }
#endif
  }
}
