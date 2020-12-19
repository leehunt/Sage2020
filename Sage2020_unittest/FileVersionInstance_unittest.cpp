#include "pch.h"

#include "..\FileVersionInstance.h"

// Gtest 'friend' forwarders.
class FileVersionInstanceTest : public testing::Test {
 public:
  static const std::string& GetCommitFromIndex(FileVersionInstance& this_ref,
                                               size_t commit_index) {
    return this_ref.GetCommitFromIndex(commit_index);
  }

  static const LineToFileVersionLineInfo& GetLinesInfo(
      FileVersionInstance& this_ref) {
    return this_ref.GetLinesInfo();
  }
};

TEST(FileVersionInstanceTest, Load) {
  std::deque<std::string> some_lines = {"Hello\n", "World!\n", "Thanks!\n"};
  std::deque<std::string> some_lines_copy = some_lines;
  std::string commit_id = "01234567890abcdef01234567890abcdef01234567";
  FileVersionInstance file_version_instance(some_lines, commit_id);
  int i = 1;
  for (const auto& line : some_lines) {
    EXPECT_EQ(line.size(), 0);
    EXPECT_STREQ(line.c_str(), "");
    EXPECT_EQ(file_version_instance.GetLineInfo(i).commit_index(), 0);
    EXPECT_STREQ(FileVersionInstanceTest::GetCommitFromIndex(
                     file_version_instance,
                     file_version_instance.GetLineInfo(i).commit_index())
                     .c_str(),
                 commit_id.c_str());
    i++;
  }
  i = 0;
  for (const auto& file_version_line : file_version_instance.GetLines()) {
    EXPECT_EQ(file_version_line.size(), some_lines_copy[i].size());
    EXPECT_STREQ(file_version_line.c_str(), some_lines_copy[i].c_str());
    i++;
  }
  EXPECT_EQ(FileVersionInstanceTest::GetLinesInfo(file_version_instance).size(),
            3U);
}