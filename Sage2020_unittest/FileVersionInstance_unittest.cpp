#include "pch.h"

#include "..\FileVersionInstance.h"

TEST(FileVersionInstanceTest, Load) {
  std::deque<std::string> some_lines = {"Hello\n", "World!\n"};
  std::deque<std::string> some_lines_copy = some_lines;
  std::string commit_id = "01234567890abcdef01234567890abcdef01234567";
  FileVersionInstance file_version_instance(some_lines, commit_id);
  int i = 1;
  for (const auto& line : some_lines) {
    EXPECT_EQ(line.size(), 0);
    EXPECT_STREQ(line.c_str(), "");
    EXPECT_EQ(file_version_instance.GetLineInfo(i).commit_id, commit_id);
    i++;
  }
  i = 0;
  for (const auto& file_version_line : file_version_instance.GetLines()) {
    EXPECT_EQ(file_version_line.size(), some_lines_copy[i].size());
    EXPECT_STREQ(file_version_line.c_str(), some_lines_copy[i].c_str());
    i++;
  }
  EXPECT_EQ(file_version_instance.GetLinesInfo().size(), 2U);
}