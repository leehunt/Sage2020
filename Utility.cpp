#include "pch.h"

#include <fcntl.h>  // _O_NOINHERIT, _O_TEXT
#include <io.h>     // pipe(), _close()
#include <algorithm>
#include <cassert>
#include <map>
#include "Utility.h"

constexpr char kGitGetRootCommand[] = "git rev-parse --show-toplevel";

std::wstring to_wstring(const std::string& s) {
  // Check for all ANSI (speedy).
  if (std::find_if(s.cbegin(), s.cend(),
                   [](unsigned char byte) { return byte > 127; }) == s.end()) {
    return std::wstring(s.cbegin(), s.cend());
  }
  size_t desired_size = static_cast<size_t>(::MultiByteToWideChar(
      CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0));
  std::wstring wstr(desired_size, '\0');
  ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                        wstr.data(), static_cast<int>(wstr.size()));
  return wstr;
}

enum PIPES {
  PIPE_READ,
  PIPE_WRITE,
  _PIPE_MAX
}; /* Constants 0 and 1 for READ and WRITE */

ProcessPipe::ProcessPipe(const TCHAR command_line[],
                         const TCHAR starting_dir[],
                         FILE* std_in)
    : file_(NULL) {
  pi_ = {};

  int my_pipes[_PIPE_MAX] = {};
  if (_pipe(my_pipes, 1024 * 16, _O_TEXT))
    return;

  STARTUPINFO si = {
      sizeof(si),
  };
  // si.wShowWindow = SW_MINIMIZE;
  si.dwFlags = STARTF_USESTDHANDLES;
  if (std_in != NULL)
    si.hStdInput = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(std_in)));
  // reinterpret_cast<HANDLE>(_get_osfhandle(my_pipes[PIPE_READ]));
  si.hStdOutput =
      reinterpret_cast<HANDLE>(_get_osfhandle(my_pipes[PIPE_WRITE]));
  // ::SetHandleInformation(si.hStdOutput, HANDLE_FLAG_INHERIT, 0);
  // si.hStdError =
  // reinterpret_cast<HANDLE>(_get_osfhandle(my_pipes[PIPE_WRITE]));

  // Copy command line so that it may be changed (::CreateProcess() may modify
  // it).
  std::wstring command_line_copy(command_line);
  // Start the child process.
  if (!::CreateProcess(
          NULL,  // No application name (use command line)
          const_cast<wchar_t*>(command_line_copy.c_str()),  // Command line
          NULL,              // Process handle not inheritable
          NULL,              // Thread handle not inheritable
          TRUE,              // Set handle inheritance to *TRUE*
          CREATE_NO_WINDOW,  // Creation flags
          NULL,              // Use parent's environment block
          starting_dir,      // Use parent's starting directory
          &si,               // Pointer to STARTUPINFO structure
          &pi_)              // Pointer to PROCESS_INFORMATION structure
  ) {
    DWORD err = ::GetLastError();
    assert(false);
    _close(my_pipes[PIPE_READ]);
    _close(my_pipes[PIPE_WRITE]);
    return;
  }
  _close(my_pipes[PIPE_WRITE]);  // This has been inherited by the child process
                                 // but is still shared with our process; close
                                 // it here to remove a deadlock.

  file_ = _fdopen(my_pipes[PIPE_READ], "r");
  if (!file_)
    _close(my_pipes[PIPE_READ]);
}

ProcessPipe::~ProcessPipe() {
  // REVIEW: Consider calling ::TerminateProcess() such that any long-running
  // processes don't keep going (e.g. once we've gotten the output we need).
  if (file_) {
    fclose(file_);
    file_ = NULL;
  }

  Join();

  if (pi_.hThread) {
    VERIFY(::CloseHandle(pi_.hThread));
    pi_.hThread = NULL;
  }
  if (pi_.hProcess) {
    VERIFY(::CloseHandle(pi_.hProcess));
    pi_.hProcess = NULL;
  }
}

void ProcessPipe::Join(DWORD msWait) {
  if (pi_.hProcess)
    ::WaitForSingleObject(pi_.hProcess, msWait);
}

std::filesystem::path GetGitRoot(const std::filesystem::path& file_path) {
  static std::map< std::filesystem::path, std::filesystem::path> root_map_;
  const auto it = root_map_.find(file_path);
  if (it != root_map_.cend())
    return it->second;
  ProcessPipe process_pipe(to_wstring(kGitGetRootCommand).c_str(),
                           file_path.parent_path().c_str());

  char stream_line[1024];
  if (!fgets(stream_line, (int)std::size(stream_line),
             process_pipe.GetStandardOutput())) {
    return {};
  }
  auto len = strlen(stream_line);
  if (len > 0 && stream_line[len - 1] == '\n')
    stream_line[len - 1] = '\0';
  root_map_[file_path] = std::filesystem::path(stream_line);
  return root_map_[file_path];
}
