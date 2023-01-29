#include "pch.h"

#include "GitFileStreamCache.h"

#include <Windows.h>

#include <corecrt_wstdio.h>
#include <intsafe.h>
#include <shlobj.h>
#include <cassert>

#include "Sage2020.h"
#include "Utility.h"

extern CSage2020App theApp;

constexpr char kFileCacheVersionLine[] = "Cache file version: 10\n";

// TODO!: Add in current opts line to the branch cache dir entries.
GitFileStreamCache::GitFileStreamCache() {}

AUTO_CLOSE_FILE_POINTER GitFileStreamCache::GetStream(
    const std::wstring& git_command) {
  const auto cache_file_path = GetItemCachePath(git_command);
  const auto& native_file_path = cache_file_path.native();
  AUTO_CLOSE_FILE_POINTER file_cache_stream(
      _wfopen(native_file_path.c_str(), L"r"));

  if (!file_cache_stream) {
    return file_cache_stream;
  }

  char stream_line[1024];
  if (!fgets(stream_line, (int)std::size(stream_line),
             file_cache_stream.get())) {
    // Error.
    file_cache_stream.reset();
    std::filesystem::remove(cache_file_path);
    return file_cache_stream;
  }

  if (strcmp(stream_line, kFileCacheVersionLine)) {
    // Old version; reject.
    file_cache_stream.reset();
    std::filesystem::remove(cache_file_path);
    return file_cache_stream;
  }

  return file_cache_stream;
}

AUTO_CLOSE_FILE_POINTER GitFileStreamCache::SaveStream(
    FILE* stream,
    const std::wstring& git_command) {
  const auto cache_file_path = GetItemCachePath(git_command);
  std::filesystem::create_directories(cache_file_path.parent_path());
  // Create output file, clearing it if it already exists.
  AUTO_CLOSE_FILE_POINTER file_cache_stream(
      _wfopen(cache_file_path.native().c_str(), L"w+"));
  if (!file_cache_stream) {
    // Error.
    return file_cache_stream;
  }

  fpos_t pos;
  if (fgetpos(file_cache_stream.get(), &pos)) {
    // Error.
    file_cache_stream.reset();
    std::filesystem::remove(cache_file_path);
    return file_cache_stream;
  }

  if (fputs(kFileCacheVersionLine, file_cache_stream.get()) == EOF) {
    // Error.
    file_cache_stream.reset();
    std::filesystem::remove(cache_file_path);
    return file_cache_stream;
  }

  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    if (fputs(stream_line, file_cache_stream.get()) == EOF) {
      // Error.
      file_cache_stream.reset();
      std::filesystem::remove(cache_file_path);
      return file_cache_stream;
    }
  }

  if (!feof(stream)) {
    // Read (fputs) error.
    file_cache_stream.reset();
    std::filesystem::remove(cache_file_path);
    return file_cache_stream;
  }

  // Reset stream to right after header line.
  if (fsetpos(file_cache_stream.get(), &pos)) {
    // Error.
    file_cache_stream.reset();
    std::filesystem::remove(cache_file_path);
    return file_cache_stream;
  }

  return file_cache_stream;
}

std::filesystem::path GitFileStreamCache::GetItemCachePath(
    const std::wstring& git_command) {
  // Look for file in
  // "CSIDL_LOCAL_APPDATA/Sage2020/type_name/git_command/object(s)_relative_path.

  TCHAR local_app_data[MAX_PATH];
#if _DEBUG
  HRESULT result =
#endif
      ::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                        local_app_data);
  assert(SUCCEEDED(result));

  std::filesystem::path item_cache_path{local_app_data};

  item_cache_path /=
      theApp.m_pszProfileName / std::filesystem::path(git_command);
  item_cache_path += ".log";

  return item_cache_path;
}
