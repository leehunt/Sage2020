#pragma once
#include <deque>
#include <filesystem>
#include <string>

class GitFileReader {
 public:
  GitFileReader(const std::filesystem::path& directory,
                const std::string& hash);
  virtual ~GitFileReader();

  void ProcessFileLines(FILE* stream);

  std::deque<std::string>& GetLines() { return lines_; }

 private:
  std::deque<std::string> lines_;
};
