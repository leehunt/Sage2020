#include "pch.h"

#include <memory>
#include "GitFileReader.h"
#include "OutputWnd.h"
#include "Utility.h"

constexpr char kGitShowCommand[] = "git --no-pager show ";

GitFileReader::GitFileReader(const std::filesystem::path& directory,
                             const std::string& hash,
                             COutputWnd* pwndOutput) {
  CWaitCursor wait;

  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("Reading commit '%1!S!' from git tree '%2!s!'"), hash.c_str(), directory.c_str());
    pwndOutput->AppendDebugTabMessage(message);
  }
  std::string command = std::string(kGitShowCommand) + hash;

  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("   Git command: '%1!s!'"), command);
    pwndOutput->AppendDebugTabMessage(message);
  }
  ProcessPipe process_pipe(to_wstring(command).c_str(), directory.c_str());

  ProcessFileLines(process_pipe.GetStandardOutput());
  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("   Done."), command);
    pwndOutput->AppendDebugTabMessage(message);
  }
}

GitFileReader::~GitFileReader() {}

void GitFileReader::ProcessFileLines(FILE* stream) {
  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    lines_.push_back(stream_line);
  }
}
