#pragma once
#include <filesystem>
#include <deque>
#include <string>

class GitFileReader {
 public:
  GitFileReader(const std::string& hash);
  virtual ~GitFileReader();

  void ProcessFileLines(FILE* stream);

  std::deque<std::string>& GetLines() { return lines_; }

  private:
    std::deque<std::string> lines_;
};
