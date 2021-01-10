#include "pch.h"

#include "..\FileVersionInstance.h"

// Gtest 'friend' forwarders.
class FileVersionInstanceTest : public testing::Test {
 public:
  static const LineToFileVersionLineInfo& GetLinesInfo(
      FileVersionInstance& this_ref) {
    return this_ref.GetLinesInfo();
  }
};

TEST(FileVersionInstanceTest, Load) {
  std::deque<std::string> some_lines = {"Hello\n", "World!\n", "Thanks!\n"};
  std::deque<std::string> some_lines_copy = some_lines;
  GitHash commit;
  commit.sha_ = "01234567890abcdef01234567890abcdef01234567";
  FileVersionInstance file_version_instance(some_lines, commit);
  int i = 0;
  for (const auto& line : some_lines) {
    EXPECT_EQ(line.size(), 0);
    EXPECT_STREQ(line.c_str(), "");
    EXPECT_EQ(file_version_instance.GetLineInfo(i).commit_index(), 0);
    // TODO
#if 0
    EXPECT_STREQ(FileVersionInstanceTest::GetCommitFromIndex(
                     file_version_instance,
                     file_version_instance.GetLineInfo(i).commit_index())
                     .c_str(),
                 commit_id.c_str());
#endif  // 0
    i++;
  }
  i = 0;
  for (const auto& file_version_line : file_version_instance.GetLines()) {
    EXPECT_EQ(file_version_line.size(), some_lines_copy[i].size());
    EXPECT_STREQ(file_version_line.c_str(), some_lines_copy[i].c_str());
    i++;
  }
#if USE_SPARSE_INDEX_ARRAY
  EXPECT_FALSE(
      FileVersionInstanceTest::GetLinesInfo(file_version_instance).IsEmpty());
#else
  EXPECT_EQ(FileVersionInstanceTest::GetLinesInfo(file_version_instance).size(),
            3U);
#endif
}

TEST(FileVersionInstanceTest, SparseIndexBasic) {
  SparseIndexArray sparse_index_array;
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, 0);
  EXPECT_TRUE(!sparse_index_array.IsEmpty());
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, 0);
  sparse_index_array.Add(0, 1, 0);
  sparse_index_array.Remove(0, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, 0);
  sparse_index_array.Add(0, 1, 0);
  sparse_index_array.Remove(1, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, 0);
  sparse_index_array.Add(1, 1, 0);
  sparse_index_array.Remove(1, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, 0);
  sparse_index_array.Add(1, 1, 0);
  sparse_index_array.Remove(0, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
}

TEST(FileVersionInstanceTest, SparseIndexArrayUp) {
  SparseIndexArray sparse_index_array;
  for (size_t i = 0; i < 10; i++) {
    sparse_index_array.Add(i, 1, i);
    auto& line_info = sparse_index_array.Get(i);
    EXPECT_EQ(line_info, FileVersionLineInfo{i});
  }

  EXPECT_EQ(sparse_index_array.Get(11), FileVersionLineInfo{});
}

TEST(FileVersionInstanceTest, SparseIndexArrayDown) {
  SparseIndexArray sparse_index_array;

  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{});

  sparse_index_array.Add(0, 10, 99);
  for (size_t i = 10; i > 0;) {
    i--;
    sparse_index_array.Add(i, 1, i);
    auto& line_info = sparse_index_array.Get(i);
    EXPECT_EQ(line_info, FileVersionLineInfo{i});
  }

  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{0});
  EXPECT_EQ(sparse_index_array.Get(11), FileVersionLineInfo{5});
  EXPECT_EQ(sparse_index_array.Get(11), FileVersionLineInfo{5});
  EXPECT_EQ(sparse_index_array.Get(18), FileVersionLineInfo{9});
  EXPECT_EQ(sparse_index_array.Get(19), FileVersionLineInfo{9});
  EXPECT_EQ(sparse_index_array.Get(20), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Get(21), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Get(999), FileVersionLineInfo{});
}

TEST(FileVersionInstanceTest, SparseIndexRemove) {
  SparseIndexArray sparse_index_array;
  for (size_t i = 0; i < 10; i++) {
    sparse_index_array.Add(i, 1, i);
    auto& line_info = sparse_index_array.Get(i);
    EXPECT_EQ(line_info, FileVersionLineInfo{i});
  }

  // No-op.
  sparse_index_array.Remove(0, 0);
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{1});

  sparse_index_array.Remove(0, 1);
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{2});

  sparse_index_array.Remove(0, 8);
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{9});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{});
}

TEST(FileVersionInstanceTest, SparseIndexInsertVsAppend) {
  SparseIndexArray sparse_index_array1;
  sparse_index_array1.Add(0, 3, 3);
  sparse_index_array1.Add(0, 3, 2);
  sparse_index_array1.Add(0, 3, 1);

  SparseIndexArray sparse_index_array2;
  sparse_index_array2.Add(0, 3, 1);
  sparse_index_array2.Add(3, 3, 2);
  sparse_index_array2.Add(6, 3, 3);

  EXPECT_EQ(sparse_index_array1, sparse_index_array2);
}

TEST(FileVersionInstanceTest, SparseIndexInsertGappy) {
  SparseIndexArray sparse_index_array;
  sparse_index_array.Add(0, 3, 1);
  sparse_index_array.Add(3, 3, 2);
  sparse_index_array.Add(6, 3, 3);

  sparse_index_array.Remove(2, 6);

  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{1});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{1});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{3});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{});
}

TEST(FileVersionInstanceTest, SparseIndexBug) {
  SparseIndexArray sparse_index_array;
  sparse_index_array.Add(0, 12, 0);
  sparse_index_array.Add(1, 1, 1);

  sparse_index_array.Remove(2, 3);
}
