#include "pch.h"
#include "../GitDiffReader.h"

#define MACRO_EXPANDER(s) s
#define QUOTIZER(s) #s

TEST(GitDiffReaderTest, Load) {
  std::filesystem::path file_path = __FILE__;
  GitDiffReader git_diff_reader(file_path.filename());

  auto diffs = git_diff_reader.GetDiffs();
}
