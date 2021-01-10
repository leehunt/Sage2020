#include "pch.h"

#include <memory>
#include "GitFileReader.h"
#include "Utility.h"
constexpr char kGitShowCommand[] = "git --no-pager show ";

GitFileReader::GitFileReader(const std::filesystem::path& directory,
                             const std::string& hash) {
  std::string command = std::string(kGitShowCommand) + hash;

  ProcessPipe process_pipe(to_wstring(command).c_str(), directory.c_str());

  ProcessFileLines(process_pipe.GetStandardOutput());
}

GitFileReader::~GitFileReader() {}

void GitFileReader::ProcessFileLines(FILE* stream) {
  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    lines_.push_back(stream_line);
  }
}
