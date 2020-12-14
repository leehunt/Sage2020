#include "pch.h"
#include "FileVersionInstance.h"

FileVersionInstance::FileVersionInstance() {
}

FileVersionInstance::FileVersionInstance(std::deque<std::string>& lines,
                                         int commit_sequence_num) {
  for (auto& line : lines) {
    FileVersionLine file_version_line = {std::move(line), commit_sequence_num};
    file_lines_.push_back(std::move(file_version_line));
  }
}

void FileVersionInstance::AddHunk(const FileVersionDiffHunk& hunk,
                                  int commit_sequence_num) {
  // Add lines
  auto it_insert = file_lines_.begin() + (hunk.add_location_ - 1);
  for (const auto& line : hunk.add_lines_) {
    it_insert = file_lines_.insert(it_insert,
                       {line, commit_sequence_num});
    it_insert++;
  }

  // Remove lines
  for (const auto& line : hunk.remove_lines_) {
    file_lines_.erase(
        file_lines_.begin() + hunk.remove_location_ - 1,
        file_lines_.begin() + hunk.remove_location_ - 1 + hunk.remove_line_count_);
  }
}
