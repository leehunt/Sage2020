#include "pch.h"

#include <shlobj.h>
#include <stdio.h>
#include "GitFileStreamCache.h"
//#include "Sage2020.h"
//#include <memory>

// extern CSage2020App theApp;

#pragma comment(lib, "shell32.lib")

constexpr char kGitGetRootCommand[] = "git rev-parse --show-toplevel";

GitFileStreamCache::GitFileStreamCache() {}

FILE_CACHE_STREAM GitFileStreamCache::GetStream(
    const std::filesystem::path& file_path,
    const std::string& hash) {
  const auto cache_file_path = GetItemCachePath(file_path, hash);
  auto temp_string = cache_file_path.u8string();
  std::unique_ptr<FILE, decltype(&fclose)> file_cache_stream(
      fopen(temp_string.c_str(), "r"), &fclose);

  return file_cache_stream;
}

FILE_CACHE_STREAM GitFileStreamCache::SaveStream(
    FILE* stream,
    const std::filesystem::path& file_path,
    const std::string& hash) {
  const auto cache_file_path = GetItemCachePath(file_path, hash);
  std::filesystem::create_directories(cache_file_path.parent_path());
  auto native_file_path = cache_file_path.u8string();
  // Create output file, failing if it already exists.
  std::unique_ptr<FILE, decltype(&fclose)> file_cache_stream(
      fopen(native_file_path.c_str(), "w+"), &fclose);

  fpos_t pos;
  if (fgetpos(file_cache_stream.get(), &pos)) {
    file_cache_stream.reset();
    remove(native_file_path.c_str());
    return file_cache_stream;
  }

  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    if (fputs(stream_line, file_cache_stream.get()) == EOF) {
      file_cache_stream.reset();
      remove(native_file_path.c_str());
      return file_cache_stream;
    }
  }

  if (fsetpos(file_cache_stream.get(), &pos)) {
    file_cache_stream.reset();
    remove(native_file_path.c_str());
    return file_cache_stream;
  }

  return file_cache_stream;
}

std::filesystem::path GitFileStreamCache::GetGitRoot() {
  // REVIEW: this assumes the cwd is |file_path_|.
  std::unique_ptr<FILE, decltype(&_pclose)> git_stream(
      _popen(kGitGetRootCommand, "r"), &_pclose);

  char stream_line[1024];
  if (!fgets(stream_line, (int)std::size(stream_line), git_stream.get()))
    return {};
  auto len = strlen(stream_line);
  if (len > 0 && stream_line[len - 1] == '\n')
    stream_line[len - 1] = '\0';
  return std::filesystem::path(stream_line);
}

std::filesystem::path GitFileStreamCache::GetItemCachePath(
    const std::filesystem::path& file_path,
    const std::string& hash) {
  // Look for file in
  // "CSIDL_LOCAL_APPDATA/Sage2020/type_name/hash/object(s)_relative_path.

  // Get git root to remove from file path, if any.
  std::filesystem::path relative_path;
  if (file_path.is_absolute()) {
    auto git_root = GetGitRoot();
    relative_path = file_path.lexically_relative(git_root);
  } else {
    relative_path = file_path;
  }

  TCHAR local_app_data[MAX_PATH];
  HRESULT result = ::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                     SHGFP_TYPE_CURRENT, local_app_data);

  std::filesystem::path item_cache_path(local_app_data);

  item_cache_path /=
      /* theApp.m_pszProfileName*/ "Sage2020" / std::filesystem::path(hash) /
      relative_path;

  return item_cache_path;
}
