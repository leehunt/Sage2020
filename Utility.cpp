#include "pch.h"

#include <fcntl.h>  // _O_NOINHERIT, _O_TEXT
#include <io.h>     // pipe(), _close()
#include <algorithm>
#include <cassert>
#include "Utility.h"

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

  int my_pipe[_PIPE_MAX] = {};
  if (_pipe(my_pipe, 1024 * 16, _O_TEXT))
    return;

  STARTUPINFO si = {
      sizeof(si),
  };
  // si.wShowWindow = SW_MINIMIZE;
  si.dwFlags = STARTF_USESTDHANDLES;
  if (std_in != NULL)
    si.hStdInput = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(std_in)));
  // reinterpret_cast<HANDLE>(_get_osfhandle(rgfdPipe[PIPE_READ]));
  si.hStdOutput = reinterpret_cast<HANDLE>(_get_osfhandle(my_pipe[PIPE_WRITE]));
  // si.hStdError =
  // reinterpret_cast<HANDLE>(_get_osfhandle(rgfdPipe[PIPE_WRITE]));

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
    _close(my_pipe[PIPE_READ]);
    _close(my_pipe[PIPE_WRITE]);
  }
  _close(my_pipe[PIPE_WRITE]);  // This has been inherited by the child process.

  file_ = _fdopen(my_pipe[PIPE_READ], "r");
  if (!file_)
    _close(my_pipe[PIPE_READ]);
}

ProcessPipe::~ProcessPipe() {
  // REVIEW: Consider calling ::TerminateProcess() such that any long-running
  // processes don't keep going (e.g. once we've gotten the output we need).
  if (file_) {
    fclose(file_);
    file_ = NULL;
  }
  if (pi_.hThread) {
    VERIFY(::CloseHandle(pi_.hThread));
    pi_.hThread = NULL;
  }
  if (pi_.hProcess) {
    VERIFY(::CloseHandle(pi_.hProcess));
    pi_.hProcess = NULL;
  }
}

void ProcessPipe::Join() {
  if (pi_.hProcess)
    ::WaitForSingleObject(pi_.hProcess, 120 * 1000 /*ms*/);
}