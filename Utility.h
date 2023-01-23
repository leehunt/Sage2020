#pragma once

#include <Windows.h>

#include <intsafe.h>
#include <filesystem>
#include <string>

// Helper function that safely converts from ANSI to Unicode.
std::wstring to_wstring(const std::string& s);

// Custom version of _popen(), without creating an annnoying DOS window.
class ProcessPipe {
 public:
  ProcessPipe(const TCHAR command_line[],
              const TCHAR starting_dir[],
              FILE* std_in = NULL);
  ~ProcessPipe();

  FILE* GetStandardOutput() const { return file_; }

  // Waits until the spawned process finishes.
  void Join(DWORD msWait = INFINITE);

 private:
  PROCESS_INFORMATION pi_;
  FILE* file_;
};

//typedef std::unique_ptr<FILE, decltype(&fclose)> AUTO_CLOSE_FILE_POINTER;
class AUTO_CLOSE_FILE_POINTER
    : public std::unique_ptr<FILE, decltype(&fclose)> {
 public:
  AUTO_CLOSE_FILE_POINTER(FILE* file = nullptr)
      : std::unique_ptr<FILE, decltype(&fclose)>(file, &fclose) {}
};

class SmartWindowsHandle
    : public std::unique_ptr<std::remove_pointer<HANDLE>::type,
                             void (*)(HANDLE)> {
 public:
  SmartWindowsHandle() : unique_ptr(nullptr, &SmartWindowsHandle::close) {}

  SmartWindowsHandle(HANDLE handle)
      : unique_ptr(handle, &SmartWindowsHandle::close) {}
  operator HANDLE() { return get(); }
  const bool valid() const { return (get() != INVALID_HANDLE_VALUE); }

 private:
  static void close(HANDLE handle) {
    if (handle != INVALID_HANDLE_VALUE)
      ::CloseHandle(handle);
  }
};

std::filesystem::path GetGitRoot(const std::filesystem::path& file_path);

std::wstring my_to_wstring(const std::string& stringToConvert);

std::string GetShortHashInRepo(std::string long_hash,
                               const std::filesystem::path& file_path);