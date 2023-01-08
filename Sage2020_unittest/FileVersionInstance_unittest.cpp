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
  GitHash parent_commit;
  strcpy_s(parent_commit.sha_, "01234567890abcdef01234567890abcdef012345");
  FileVersionInstance file_version_instance(std::move(some_lines), parent_commit);
  int i = 0;
  for (const auto& line : some_lines) {
    EXPECT_EQ(line.size(), 0);
    EXPECT_STREQ(line.c_str(), "");
    EXPECT_STREQ(file_version_instance.GetLineInfo(i).commit_sha(),
                 parent_commit.sha_);
    i++;
  }
  EXPECT_TRUE(parent_commit.IsValid());
  EXPECT_EQ(file_version_instance.GetCommit(), parent_commit);

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
  sparse_index_array.Add(0, 1, "0");
  EXPECT_TRUE(!sparse_index_array.IsEmpty());
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, "0");
  sparse_index_array.Add(0, 1, "0");
  sparse_index_array.Remove(0, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, "0");
  sparse_index_array.Add(0, 1, "0");
  sparse_index_array.Remove(1, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, "0");
  sparse_index_array.Add(1, 1, "0");
  sparse_index_array.Remove(1, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
  sparse_index_array.Add(0, 1, "0");
  sparse_index_array.Add(1, 1, "0");
  sparse_index_array.Remove(0, 1);
  sparse_index_array.Remove(0, 1);
  EXPECT_TRUE(sparse_index_array.IsEmpty());
}

TEST(FileVersionInstanceTest, SparseIndexArrayUp) {
  SparseIndexArray sparse_index_array;

  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{});
  sparse_index_array.Add(0, 10, "99");
  // index:  0   1   2   3   4   5   6   7   8   9
  // value: |                  99                 |
  int end_pos = 10;
  EXPECT_EQ(sparse_index_array.Get(end_pos - 1), FileVersionLineInfo{"99"});
  EXPECT_EQ(sparse_index_array.Get(end_pos), FileVersionLineInfo{});

  for (int i = 0; i < 10; i++) {
    char sha[41];
    _itoa_s(i, sha, 16);
    sparse_index_array.Add(i, 1, sha);
    auto& line_info = sparse_index_array.Get(i);
    EXPECT_EQ(line_info, FileVersionLineInfo{sha});
    end_pos++;
    EXPECT_EQ(sparse_index_array.Get(end_pos), FileVersionLineInfo{});
  }

  EXPECT_EQ(sparse_index_array.Get(11), FileVersionLineInfo{"99"});
  EXPECT_EQ(sparse_index_array.Get(21), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Get(999), FileVersionLineInfo{});
}

TEST(FileVersionInstanceTest, SparseIndexArrayDown) {
  SparseIndexArray sparse_index_array;

  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{});

  sparse_index_array.Add(0, 10, "99");
  // index:  0   1   2   3   4   5   6   7   8   9
  // value: |                  99                 |
  int end_pos = 10;
  EXPECT_EQ(sparse_index_array.Get(end_pos - 1), FileVersionLineInfo{"99"});
  EXPECT_EQ(sparse_index_array.Get(end_pos), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Size(), 2);

  // Insert new single length items at indexes [9, 0] going backwards.
  // E.g.
  // First insert:
  // index:  0   1   2   3   4   5   6   7   8   9   10
  // value: |                99                | 9 | 99|
  // Second insert:
  // index:  0   1   2   3   4   5   6   7   8   9   10  11
  // value: |              99              | 8 | 99| 9 | 99|
  // ...
  int num_ranges = 2;
  for (int i = 10; i > 0;) {
    EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"99"});
    i--;
    char sha[40];
    _itoa_s(i, sha, 16);
    sparse_index_array.Add(i, 1, sha);
    auto& line_info = sparse_index_array.Get(i);
    EXPECT_EQ(line_info, FileVersionLineInfo{sha});
    end_pos++;
    EXPECT_EQ(sparse_index_array.Get(end_pos), FileVersionLineInfo{});
    num_ranges += i > 0 ? 2 : 1; 
    EXPECT_EQ(sparse_index_array.Size(), num_ranges);
  }

  const char* expected_values[] = {"0",  "99", "1",  "99", "2",  "99", "3",
                                   "99", "4",  "99", "5",  "99", "6",  "99",
                                   "7",  "99", "8",  "99", "9",  "99"};
  int i = 0;
  for (auto expected_value : expected_values) {
    EXPECT_EQ(sparse_index_array.Get(i), FileVersionLineInfo{expected_value});
    i++;
  }
  EXPECT_EQ(sparse_index_array.Get(i), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Get(21), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Get(999), FileVersionLineInfo{});
}
TEST(FileVersionInstanceTest, SparseIndexSplitTest) {
  SparseIndexArray sparse_index_array;
  sparse_index_array.Add(0, 1, "0");
  sparse_index_array.Add(1, 2, "1");
  sparse_index_array.Add(3, 1, "2");
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"0"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{"2"});
  EXPECT_EQ(sparse_index_array.Get(4), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Size(), 4);

  // No split --> move everthing down.
  sparse_index_array.Add(0, 1, "3");
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"3"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"0"});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(4), FileVersionLineInfo{"2"});
  EXPECT_EQ(sparse_index_array.Get(5), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Size(), 5);
  sparse_index_array.Remove(0, 1);  // Reset.
  EXPECT_EQ(sparse_index_array.Size(), 4);

  // No split --> inserted between two items.
  sparse_index_array.Add(1, 1, "3");
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"0"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"3"});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(4), FileVersionLineInfo{"2"});
  EXPECT_EQ(sparse_index_array.Get(5), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Size(), 5);
  sparse_index_array.Remove(1, 1);  // Reset.
  EXPECT_EQ(sparse_index_array.Size(), 4);

  // Split.
  sparse_index_array.Add(2, 1, "3");
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"0"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{"3"});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(4), FileVersionLineInfo{"2"});
  EXPECT_EQ(sparse_index_array.Get(5), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Size(), 6);
  sparse_index_array.Remove(2, 1);  // Reset.
  EXPECT_EQ(sparse_index_array.Size(), 4);

  // No split --> inserted between two items.
  sparse_index_array.Add(3, 1, "3");
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"0"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{"3"});
  EXPECT_EQ(sparse_index_array.Get(4), FileVersionLineInfo{"2"});
  EXPECT_EQ(sparse_index_array.Get(5), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Size(), 5);
  sparse_index_array.Remove(3, 1);  // Reset.
  EXPECT_EQ(sparse_index_array.Size(), 4);

  // No split --> Added at end.
  sparse_index_array.Add(4, 1, "3");
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"0"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{"2"});
  EXPECT_EQ(sparse_index_array.Get(4), FileVersionLineInfo{"3"});
  EXPECT_EQ(sparse_index_array.Get(5), FileVersionLineInfo{});
  EXPECT_EQ(sparse_index_array.Size(), 5);
  sparse_index_array.Remove(4, 1);  // Reset.
  EXPECT_EQ(sparse_index_array.Size(), 4);
}

TEST(FileVersionInstanceTest, SparseIndexRemove) {
  SparseIndexArray sparse_index_array;
  for (int i = 0; i < 10; i++) {
    char sha[40];
    _itoa_s(i, sha, 16);
    sparse_index_array.Add(i, 1, sha);
    auto& line_info = sparse_index_array.Get(i);
    EXPECT_EQ(line_info, FileVersionLineInfo{sha});
  }

  // No-op.
  sparse_index_array.Remove(0, 0);
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"1"});

  sparse_index_array.Remove(0, 1);
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"2"});

  sparse_index_array.Remove(0, 8);
  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"9"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{});
}

TEST(FileVersionInstanceTest, SparseIndexInsertVsAppend) {
  SparseIndexArray sparse_index_array1;
  sparse_index_array1.Add(0, 3, "3");
  sparse_index_array1.Add(0, 3, "2");
  sparse_index_array1.Add(0, 3, "1");

  SparseIndexArray sparse_index_array2;
  sparse_index_array2.Add(0, 3, "1");
  sparse_index_array2.Add(3, 3, "2");
  sparse_index_array2.Add(6, 3, "3");

  EXPECT_EQ(sparse_index_array1, sparse_index_array2);
}

TEST(FileVersionInstanceTest, SparseIndexInsertGappy) {
  SparseIndexArray sparse_index_array;
  sparse_index_array.Add(0, 3, "1");
  sparse_index_array.Add(3, 3, "2");
  sparse_index_array.Add(6, 3, "3");

  sparse_index_array.Remove(2, 6);

  EXPECT_EQ(sparse_index_array.Get(0), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(1), FileVersionLineInfo{"1"});
  EXPECT_EQ(sparse_index_array.Get(2), FileVersionLineInfo{"3"});
  EXPECT_EQ(sparse_index_array.Get(3), FileVersionLineInfo{});
}

TEST(FileVersionInstanceTest, SparseIndexBug) {
  SparseIndexArray sparse_index_array;
  sparse_index_array.Add(0, 12, "0");
  sparse_index_array.Add(1, 1, "1");

  sparse_index_array.Remove(2, 3);
}
