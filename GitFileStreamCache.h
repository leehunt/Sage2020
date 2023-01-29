#pragma once

#include <filesystem>

#include "Utility.h"

class GitFileStreamCache {
 public:
  GitFileStreamCache();

  AUTO_CLOSE_FILE_POINTER GetStream(const std::wstring& git_command);
  AUTO_CLOSE_FILE_POINTER SaveStream(FILE* stream,
                                     const std::wstring& git_command);

  std::filesystem::path GetItemCachePath(const std::wstring& git_command);

 private:
  const std::string profile_name_;
};
