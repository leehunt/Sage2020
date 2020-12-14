#include "pch.h"
#include "..\FileVersionInstance.h"

TEST(FileVersionInstanceTest, Load) {
  std::deque<std::string> some_lines = {"Hello\n", "World!\n"};
  std::deque<std::string> some_lines_copy = some_lines;
  int commit_sequence_num = 1;
  FileVersionInstance file_version_instance(some_lines, commit_sequence_num);
  for (const auto& line : some_lines) {
    EXPECT_EQ(line.size(), 0);
    EXPECT_STREQ(line.c_str(), "");
  }
  int i = 0;
  for (const auto& file_version_line : file_version_instance.GetLines()) {
    EXPECT_EQ(file_version_line.age, commit_sequence_num);
    EXPECT_EQ(file_version_line.line.size(), some_lines_copy[i].size());
    EXPECT_STREQ(file_version_line.line.c_str(), some_lines_copy[i].c_str());
    i++;
  }
}