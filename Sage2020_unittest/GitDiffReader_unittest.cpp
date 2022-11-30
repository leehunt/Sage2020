#include "pch.h"

#include "../FileVersionInstance.h"
#include "../FileVersionInstanceEditor.h"
#include "../GitDiffReader.h"
#include "../GitFileReader.h"

// Gtest 'friend' forwarders.
class GitDiffReaderTest : public testing::Test {
 public:
  static const LineToFileVersionLineInfo& GetLinesInfo(
      FileVersionInstance& this_ref) {
    return this_ref.GetLinesInfo();
  }
};

TEST(GitDiffReaderTest, LoadAndCompareWithFile) {
  std::filesystem::path file_path = __FILE__;
  std::string tag;
  GitDiffReader git_diff_reader(file_path, tag);

  auto& diffs = git_diff_reader.GetDiffs();
  EXPECT_GT(diffs.size(), 0U);
  // Sythethesize FileVersionInstance from diffs, going from first diff (the
  // last recorded in the git log) forward.
  FileVersionInstance file_version_instance;
  Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(file_version_instance, listener_head);
  EXPECT_EQ(&editor.GetFileVersionInstance(), &file_version_instance);
  for (auto it = diffs.crbegin(); it != diffs.crend(); it++) {
    editor.AddDiff(*it);
    EXPECT_EQ(file_version_instance.GetCommit(), it->commit_);
  }

  std::string latest_file_id =
      diffs.size() > 0 ? diffs.front().diff_tree_.new_hash_string : "";
  GitFileReader git_file_reader_latest{file_path.parent_path(), latest_file_id};
  auto file_version_instance_loaded = std::make_unique<FileVersionInstance>(
      std::move(git_file_reader_latest.GetLines()),
      diffs.size() > 0 ? diffs.front().commit_ : GitHash());
  EXPECT_EQ(file_version_instance_loaded->GetLines().size(),
            file_version_instance.GetLines().size());
  for (size_t i = 0; i < file_version_instance.GetLines().size(); i++) {
    const auto& file_version_line_loaded =
        file_version_instance_loaded->GetLines()[i];
    const auto& file_version_line_from_diffs =
        file_version_instance.GetLines()[i];
    EXPECT_STREQ(file_version_line_loaded.c_str(),
                 file_version_line_from_diffs.c_str());
  }

  // Now go backwards down to nothing.
  for (auto it = diffs.cbegin(); it != diffs.cend(); it++) {
    //auto parent_commit =
    //    it->parents_.empty() ? GitHash{} : it->parents_[0].commit_;
    auto new_commit = ((it + 1) != diffs.cend()) ? (it + 1)->commit_ : GitHash{};
    // N.b.  The |parent_commit| is not often not |new_commit| because diffs only
    // record changes to the given file; not all changes to the branch.
    //EXPECT_EQ(new_commit, parent_commit);
    editor.RemoveDiff(*it, new_commit);
    EXPECT_EQ(file_version_instance.GetCommit(), new_commit);
  }

  EXPECT_EQ(file_version_instance.GetLines().size(), 0);
#if USE_SPARSE_INDEX_ARRAY
  EXPECT_TRUE(GitDiffReaderTest::GetLinesInfo(file_version_instance).IsEmpty());
#else
  EXPECT_EQ(GitDiffReaderTest::GetLinesInfo(file_version_instance).size(), 0);
#endif
}

static void LoadAllBranchesRecur(
    const std::filesystem::path& file_path, std::vector<FileVersionDiff>& diffs) {

  // Sythethesize FileVersionInstance from diffs, going from first diff (the
  // last recorded in the git log) forward.
  for (auto it = diffs.crbegin(); it != diffs.crend(); it++) {
    auto& diff = *it;
    for (size_t secondary_parent_index = 1;
         secondary_parent_index < diff.parents_.size();
         secondary_parent_index++) {
      std::string secondary_parent_revision_range =
          diff.parents_[0].commit_.sha_ + ".." +
          diff.parents_[secondary_parent_index].commit_.sha_;
      GitDiffReader git_branch_diff_reader{file_path,
                                           secondary_parent_revision_range};
      diff.parents_[secondary_parent_index].file_version_diffs_ =
          std::make_unique<std::vector<FileVersionDiff>>(
              std::move(git_branch_diff_reader.MoveDiffs()));
      LoadAllBranchesRecur(
          file_path, *diff.parents_[secondary_parent_index].file_version_diffs_);
    }
  }
}

static void CompareAllBranchesRecur(
    FileVersionInstanceEditor& editor,
    const std::vector<FileVersionDiff>& diffs) {
  for (auto it = diffs.cbegin(); it != diffs.cend(); it++) {
    auto& diff = *it;
    for (size_t secondary_parent_index = 1;
         secondary_parent_index < diff.parents_.size();
         secondary_parent_index++) {
      // We add new branches releative to the *parent*
      // of the diffs, so remove it here.
      //auto new_commit =
      //    ((it + 1) != diffs.cbegin()) ? (it + 1)->commit_ : GitHash{};
      //editor.RemoveDiff(diff, new_commit);
      CompareAllBranchesRecur(
          editor, *diff.parents_[secondary_parent_index].file_version_diffs_);
    }
 
    auto new_commit =
        ((it + 1) != diffs.cend()) ? (it + 1)->commit_ : GitHash{};
    // N.b.  The |parent_commit| is not often not |new_commit| because diffs
    // only record changes to the given file; not all changes to the branch.
    // EXPECT_EQ(new_commit, parent_commit);
    editor.RemoveDiff(diff, new_commit);
    EXPECT_EQ(editor.GetFileVersionInstance().GetCommit(), diff.commit_);
  }

  // Now go backwards down to nothing.
  for (auto it = diffs.cbegin(); it != diffs.cend(); it++) {
    //auto parent_commit =
    //    it->parents_.empty() ? GitHash{} : it->parents_[0].commit_;
    auto new_commit =
        ((it + 1) != diffs.cend()) ? (it + 1)->commit_ : GitHash{};
    // N.b.  The |parent_commit| is not often not |new_commit| because diffs
    // only record changes to the given file; not all changes to the branch.
    // EXPECT_EQ(new_commit, parent_commit);
    editor.RemoveDiff(*it, new_commit);
    EXPECT_EQ(editor.GetFileVersionInstance().GetCommit(), new_commit);
  }
}

TEST(GitDiffReaderTest, LoadAndCompareWithFileAllBranches) {
  std::filesystem::path file_path = __FILE__;
  // This file has an interesting branch history.
  file_path = file_path.parent_path().parent_path() / "FileVersionInstance.cpp";
  std::string tag;
  GitDiffReader git_diff_reader(file_path, tag);
  EXPECT_GT(git_diff_reader.GetDiffs().size(), 0U);

  auto diffs = std::move(git_diff_reader.MoveDiffs());
  LoadAllBranchesRecur(file_path, diffs);

  FileVersionInstance file_version_instance;
  Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(file_version_instance, listener_head);
  CompareAllBranchesRecur(editor, diffs);

  EXPECT_EQ(file_version_instance.GetLines().size(), 0);
#if USE_SPARSE_INDEX_ARRAY
  EXPECT_TRUE(GitDiffReaderTest::GetLinesInfo(file_version_instance).IsEmpty());
#else
  EXPECT_EQ(GitDiffReaderTest::GetLinesInfo(file_version_instance).size(), 0);
#endif
}
