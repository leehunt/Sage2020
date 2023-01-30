#pragma once

#include <stdio.h>
#include <tchar.h>

#include "Utility.h"

class COutputWnd;

class GitRevFilter {
 public:
  GitRevFilter(const _TCHAR git_file_path[],
               FILE* stream,
               COutputWnd* pwndOutput = nullptr);

  FILE* GetFilteredStream() const { return dest_filtered_file_.get(); }

 private:
  AUTO_CLOSE_FILE_POINTER dest_filtered_file_;
};
