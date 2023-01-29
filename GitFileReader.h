#pragma once

#include <deque>
#include <filesystem>

class COutputWnd;

class GitFileReader {
 public:
  GitFileReader(const std::filesystem::path& directory,
                const std::string& revision,
                COutputWnd* pwndOutput = nullptr);
  virtual ~GitFileReader();

  int ProcessFileLines(FILE* stream);

  std::deque<std::string>& GetLines() { return lines_; }

 private:
  std::deque<std::string> lines_;
};
