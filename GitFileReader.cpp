#include "pch.h"

#include <memory>
#include "GitFileReader.h"
#include "OutputWnd.h"
#include "Utility.h"

constexpr char kGitShowCommand[] = "git --no-pager show ";

GitFileReader::GitFileReader(const std::filesystem::path& directory,
                             const std::string& revision,
                             COutputWnd* pwndOutput) {
  CWaitCursor wait;

  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("Reading commit '%1!S!' from git tree '%2!s!'"),
                          revision.c_str(), directory.c_str());
    pwndOutput->AppendDebugTabMessage(message);
  }
  std::string command = std::string(kGitShowCommand) + revision;

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
  // REVIEW: Consider looking at the '\ No newline at end of file' message from
  // the 'git log' diff command to remove the '\n' instead (but it may be tricky
  // to figure out what diff to modify).
  if (!lines_.empty() && lines_.back().back() != '\n')
    lines_.back() += '\n';
}
