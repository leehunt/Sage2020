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
    message.FormatMessage(_T("   Git command: '%1!s!'"), command.c_str());
    pwndOutput->AppendDebugTabMessage(message);
  }
  ProcessPipe process_pipe(to_wstring(command).c_str(), directory.c_str());

  ProcessFileLines(process_pipe.GetStandardOutput());
  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("   Done."));
    pwndOutput->AppendDebugTabMessage(message);
  }
}

GitFileReader::~GitFileReader() {}

void GitFileReader::ProcessFileLines(FILE* stream) {
  char stream_line[1024 * 2];
  bool is_utf16 = false;

  // Read first line to check for any BOM.
  if (fgets(stream_line, (int)std::size(stream_line), stream)) {
    if ((unsigned char)stream_line[0] == 0xFF &&
        (unsigned char)stream_line[1] == 0xFE)  // UTF-16 little-endian
      is_utf16 = true;
  }

  if (is_utf16) {
    wchar_t wide_stream_line[1024];
    wcscpy(wide_stream_line, (wchar_t*)stream_line + 1);  // Skip past BOM.
    do {
      ::WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, wide_stream_line, -1,
                            stream_line, (int)std::size(stream_line),
                            nullptr /*lpDefaultChar*/,
                            nullptr /*lpUsedDefaultChar*/);
      lines_.push_back(stream_line);
    } while (fgetws(wide_stream_line, (int)std::size(stream_line), stream));
  } else {
    do {
      lines_.push_back(stream_line);
    } while (fgets(stream_line, (int)std::size(stream_line), stream));
  }

  // REVIEW: Consider looking at the '\ No newline at end of file' message from
  // the 'git log' diff command to remove the '\n' instead (but it may be tricky
  // to figure out what diff to modify).
  if (!lines_.empty() && lines_.back().back() != '\n')
    lines_.back() += '\n';
}
