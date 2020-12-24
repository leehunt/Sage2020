#pragma once
#include <filesystem>
#include <fstream>

typedef std::unique_ptr<FILE, decltype(&fclose)> FILE_CACHE_STREAM;
// std::ifstream

class GitFileStreamCache {
 public:
  GitFileStreamCache();

  FILE_CACHE_STREAM GetStream(const std::filesystem::path& file_path,
                              const std::string& hash);

  FILE_CACHE_STREAM SaveStream(FILE* stream,
                               const std::filesystem::path& file_path,
                               const std::string& hash);

 private:
  std::filesystem::path GetGitRoot();
  std::filesystem::path GetItemCachePath(const std::filesystem::path& file_path,
                                         const std::string& hash);

 private:
  const std::string profile_name_;
};
