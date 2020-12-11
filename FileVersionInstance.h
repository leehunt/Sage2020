#pragma once
#include <deque>
#include <filesystem>

struct FileVersionLine {
  std::string line;
  int age;
};
class CFileVersionInstance {
 public:
   CFileVersionInstance(std::filesystem::path path, int version) {

  }
  virtual ~CFileVersionInstance() {}

 private:
   std::deque<FileVersionLine> file_lines_;
};
