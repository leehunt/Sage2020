#pragma once
#include <deque>
#include <filesystem>

struct FileVersionLine {
  std::string line;
  int age;
};
class FileVersionInstance {
 public:
   FileVersionInstance(std::deque<std::string>& lines) {
    for (auto& line : lines) {
      FileVersionLine file_version_line = {std::move(line), 0};
      file_lines_.push_back(std::move(file_version_line));
    }
  }
  virtual ~FileVersionInstance() {}

 private:
   std::deque<FileVersionLine> file_lines_;
};
