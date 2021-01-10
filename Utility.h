#pragma once

#include <memory>
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
  void Join();

 private:
  PROCESS_INFORMATION pi_;
  FILE* file_;
};
