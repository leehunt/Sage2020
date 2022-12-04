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
  static bool IsTheSame(const FileVersionInstance& this_ref,
                        const FileVersionInstance& other_ref) {
    return this_ref == other_ref;
  }
};

static void CompareFileInstanceToHead(
    const std::filesystem::path& file_path,
    const std::vector<FileVersionDiff>& diffs,
    const FileVersionInstance& file_version_instance_from_diffs) {
  std::string blob_hash =
      diffs.size() > 0 ? diffs.back().diff_tree_.new_hash_string : "";
  // REVIEW: We could also use a revision of |<commit>:<file-name>|, which
  // may be more clean/orthogonal as we don't use blob hashes elsewhere.
  GitFileReader git_file_reader_latest{file_path.parent_path(), blob_hash};
  auto file_version_instance_loaded = std::make_unique<FileVersionInstance>(
      std::move(git_file_reader_latest.GetLines()),
      diffs.size() > 0 ? diffs.back().commit_ : GitHash());
  EXPECT_EQ(file_version_instance_loaded->GetLines().size(),
            file_version_instance_from_diffs.GetLines().size());
  for (size_t i = 0; i < file_version_instance_from_diffs.GetLines().size();
       i++) {
    const auto& file_version_line_loaded =
        file_version_instance_loaded->GetLines()[i];
    const auto& file_version_line_from_diffs =
        file_version_instance_from_diffs.GetLines()[i];
    EXPECT_STREQ(file_version_line_loaded.c_str(),
                 file_version_line_from_diffs.c_str());
  }
}

static void CompareFileInstanceToCommit(
    const FileVersionInstance& file_version_instance_from_diffs,
    const std::filesystem::path& file_path,
    const GitHash& commit) {
  std::string file_revision = commit.sha_ + ":" + file_path.filename().string();
  GitFileReader git_file_reader_commit{file_path.parent_path(), file_revision};
  EXPECT_EQ(file_version_instance_from_diffs.GetLines().size(),
            git_file_reader_commit.GetLines().size());
  for (size_t i = 0; i < git_file_reader_commit.GetLines().size(); i++) {
    const auto& file_version_line_commit = git_file_reader_commit.GetLines()[i];
    const auto& file_version_line_from_diffs =
        file_version_instance_from_diffs.GetLines()[i];
    EXPECT_STREQ(file_version_line_commit.c_str(),
                 file_version_line_from_diffs.c_str());
  }
}

TEST(GitDiffReaderTest, LoadAndCompareWithFile) {
  std::filesystem::path file_path = __FILE__;
  std::string tag;
  GitDiffReader git_diff_reader(file_path, tag);

  auto diffs = git_diff_reader.MoveDiffs();
  EXPECT_GT(diffs.size(), 0U);
  std::reverse(diffs.begin(), diffs.end());
  // Sythethesize FileVersionInstance from diffs, going from first diff (the
  // last recorded in the git log) forward.
  FileVersionInstance file_version_instance;
  Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(file_version_instance, listener_head);
  EXPECT_EQ(&editor.GetFileVersionInstance(), &file_version_instance);
  for (const auto& diff : diffs) {
    editor.AddDiff(diff);
    EXPECT_EQ(file_version_instance.GetCommit(), diff.commit_);
  }

  CompareFileInstanceToHead(file_path, diffs, file_version_instance);

  // Now go backwards in time (but forward in the diffs vector) down to nothing,
  // or at least the first known version.
  for (auto it = diffs.crbegin(); it != diffs.crend(); it++) {
    // auto parent_commit =
    //     it->parents_.empty() ? GitHash{} : it->parents_[0].commit_;
    const auto& new_commit =
        it != diffs.crbegin() ? (it - 1)->commit_ : GitHash{};
    // N.b.  The |parent_commit| is not often not |new_commit| because diffs
    // only record changes to the given file; not all changes to the branch.
    // EXPECT_EQ(new_commit, parent_commit);
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

static void LoadAllBranchesRecur(const std::filesystem::path& file_path,
                                 std::vector<FileVersionDiff>& diffs) {
  for (auto& diff : diffs) {
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
      auto& diffs_ref =
          *diff.parents_[secondary_parent_index].file_version_diffs_;
      std::reverse(diffs_ref.begin(), diffs_ref.end());
      LoadAllBranchesRecur(file_path, diffs_ref);
    }
  }
}

static void TraverseAndVerifyAllBranchesRecur(
    FileVersionInstanceEditor& editor,
    const std::filesystem::path& file_path,
    const std::vector<FileVersionDiff>& diffs) {
  const FileVersionInstance original_instance = editor.GetFileVersionInstance();

  // Reconstitute the orignal FileVersionInstance from diffs, going from
  // first diff forward.
  for (const auto& diff : diffs) {
    for (size_t secondary_parent_index = 1;
         secondary_parent_index < diff.parents_.size();
         secondary_parent_index++) {
      // We add new branches releative to the *parent*
      // of the diffs, so remove it here.
      // auto new_commit =
      //    ((it + 1) != diffs.cbegin()) ? (it + 1)->commit_ : GitHash{};
      // editor.RemoveDiff(diff, new_commit);
      const auto& child_diffs =
          *diff.parents_[secondary_parent_index].file_version_diffs_;
      const auto& last_child_parent_commit =
          child_diffs.empty() || child_diffs.front().parents_.empty()
              ? GitHash{}
              : child_diffs.front().parents_[0].commit_;
      const auto current_commit = editor.GetFileVersionInstance().GetCommit();
      editor.GoToCommit(last_child_parent_commit, diffs);

      TraverseAndVerifyAllBranchesRecur(editor, file_path, child_diffs);

      editor.GoToCommit(current_commit, diffs);
    }

    editor.AddDiff(diff);
    EXPECT_EQ(editor.GetFileVersionInstance().GetCommit(), diff.commit_);
  }

  const auto& last_commit = diffs.empty() ? GitHash{} : diffs.back().commit_;
  CompareFileInstanceToCommit(editor.GetFileVersionInstance(), file_path,
                              last_commit);

  // Now backwards in time, starting from the last recorded change in the git
  // log down to first recorded diff. Note that this may end up with nothing if
  // the last commit is the initial commit.
  for (auto it = diffs.crbegin(); it != diffs.crend(); it++) {
    const auto& diff = *it;

    // N.b. The |parent_commit| is not often not |previous_commit| because diffs
    // only record changes to the given file; not all changes to the branch.
    // That is, the follow is not always true:
    // auto parent_commit =
    //     it->parents_.empty() ? GitHash{} : it->parents_[0].commit_;
    const auto previous_commit =
        (it + 1) != diffs.crend()
            ? (it + 1)->commit_
            : (diff.parents_.empty() ? GitHash{} : diff.parents_[0].commit_);
    // EXPECT_EQ(new_commit, parent_commit);
    editor.RemoveDiff(diff, previous_commit);
    EXPECT_EQ(editor.GetFileVersionInstance().GetCommit(), previous_commit);
  }

  EXPECT_TRUE(GitDiffReaderTest::IsTheSame(editor.GetFileVersionInstance(),
                                           original_instance));
}

TEST(GitDiffReaderTest, LoadAndCompareWithFileAllBranches) {
  std::filesystem::path file_path = __FILE__;
  // This file has an interesting branch history.
  file_path = file_path.parent_path().parent_path() / "FileVersionInstance.cpp";
  std::string tag;
  GitDiffReader git_diff_reader(file_path, tag);
  EXPECT_GT(git_diff_reader.GetDiffs().size(), 0U);

  auto diffs = std::move(git_diff_reader.MoveDiffs());
  std::reverse(diffs.begin(), diffs.end());
  LoadAllBranchesRecur(file_path, diffs);

  // Load current starting parent, if any.
  GitHash parent_commit = diffs.empty() || diffs.front().parents_.empty()
                              ? GitHash{}
                              : diffs.front().parents_[0].commit_;
  std::string parent_revision =
      parent_commit.sha_ + ":" + file_path.filename().string();
  GitFileReader git_file_reader_head{file_path.parent_path(), parent_revision};

  FileVersionInstance file_version_instance(
      std::move(git_file_reader_head.GetLines()), parent_commit);
  Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(file_version_instance, listener_head);

  TraverseAndVerifyAllBranchesRecur(editor, file_path, diffs);

  EXPECT_EQ(file_version_instance.GetLines().size(), 0);
#if USE_SPARSE_INDEX_ARRAY
  EXPECT_TRUE(GitDiffReaderTest::GetLinesInfo(file_version_instance).IsEmpty());
#else
  EXPECT_EQ(GitDiffReaderTest::GetLinesInfo(file_version_instance).size(), 0);
#endif
}
