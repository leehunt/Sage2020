#pragma once
#include <filesystem>
#include <fstream>

typedef std::unique_ptr<FILE, decltype(&fclose)> AUTO_CLOSE_FILE_POINTER;
// std::ifstream

class GitFileStreamCache {
 public:
  GitFileStreamCache(const std::filesystem::path& file_path);

  AUTO_CLOSE_FILE_POINTER GetStream(const std::string& hash);
  AUTO_CLOSE_FILE_POINTER SaveStream(FILE* stream,
                                     const char* header_line,
                                     const std::string& hash);

 private:
  std::filesystem::path GetGitRoot();

  std::filesystem::path GetItemCachePath(const std::string& hash);

 private:
  const std::filesystem::path file_path_;
  const std::string profile_name_;
};
