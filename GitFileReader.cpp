#include "pch.h"
#include "GitFileReader.h"

constexpr char kGitShowCommand[] =
    "git --no-pager show ";

GitFileReader::GitFileReader(const std::string& hash) {
  std::string command = std::string(kGitShowCommand) + hash;
  std::unique_ptr<FILE, decltype(&_pclose)> git_stream(
      _popen(command.c_str(), "r"), &_pclose);

  ProcessFileLines(git_stream.get());
}

GitFileReader::~GitFileReader() {}

void GitFileReader::ProcessFileLines(FILE* stream) {
  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    lines_.push_back(stream_line);
  }
}
