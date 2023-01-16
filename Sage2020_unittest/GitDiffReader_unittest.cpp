#include "pch.h"

#include "../FileVersionInstance.h"
#include "../FileVersionInstanceEditor.h"
#include "../GitDiffReader.h"
#include "../GitFileReader.h"
#include "../Utility.h"

// #define USE_PRE_DIFF // Apply diff before parents.

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
    const FileVersionDiff& diff,
    const FileVersionInstance& file_version_instance_from_diffs) {
  std::string blob_hash = diff.diff_tree_.new_hash_string;
  // REVIEW: We could also use a revision of
  // |<commit>:<releative-git-file-path>|, which may be more clean/orthogonal as
  // we don't use blob hashes elsewhere.
  GitFileReader git_file_reader_latest{file_path.parent_path(), blob_hash};
  auto file_version_instance_loaded = std::make_unique<FileVersionInstance>(
      file_version_instance_from_diffs.commit_path_.GetRoot(),
      std::move(git_file_reader_latest.GetLines()), diff.commit_);
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
  // Skip empty files; assume they are binary.
  if (file_version_instance_from_diffs.GetLines().empty())
    return;

  // REVIEW:  Using the relative file name is more cumbersome that anticipated;
  // consider passing in the diff file tree hash instead.

  // Read file (aside: git oddly doesn't have a clean way to find binary files).
  const auto git_directory_root = GetGitRoot(file_path);
  const auto relative_path_native =
      file_path.lexically_relative(git_directory_root);
  // HACK: Convert to Git-friendly forward slashes.
  std::string git_releative_path = relative_path_native.string();
  for (auto& ch : git_releative_path) {
    if (ch == '\\')
      ch = '/';
  }
  std::string file_revision =
      commit.sha_ + std::string(":./") + git_releative_path;
  GitFileReader git_file_reader_commit{file_path.parent_path(), file_revision};

  // Check for same number of lines.
  EXPECT_EQ(file_version_instance_from_diffs.GetLines().size(),
            git_file_reader_commit.GetLines().size());

  // Check for identical contents.
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
  // Sythethesize FileVersionInstance from diffs, going from first diff (the
  // last recorded in the git log) forward.
  FileVersionInstance file_version_instance{diffs};
  Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(diffs, file_version_instance, listener_head);
  EXPECT_EQ(&editor.GetFileVersionInstance(), &file_version_instance);
  for (const auto& diff : diffs) {
    editor.AddDiff(diff);
    EXPECT_EQ(file_version_instance.GetCommit(), diff.commit_);
  }

  CompareFileInstanceToHead(file_path, diffs.back(), file_version_instance);

  // Now go backwards in time (but forward in the diffs vector) down to nothing,
  // or at least the first known version.
  for (auto it = diffs.crbegin(); it != diffs.crend(); it++) {
    const auto& diff = *it;
    editor.RemoveDiff(diff);
    EXPECT_EQ(file_version_instance.GetCommit(), diff.file_parent_commit_);
  }

  EXPECT_EQ(file_version_instance.GetLines().size(), 0);
#if USE_SPARSE_INDEX_ARRAY
  EXPECT_TRUE(GitDiffReaderTest::GetLinesInfo(file_version_instance).IsEmpty());
#else
  EXPECT_EQ(GitDiffReaderTest::GetLinesInfo(file_version_instance).size(), 0);
#endif
}

static std::string GetShortHash(const GitHash& hash) {
  std::string s = hash.sha_;
  return s.substr(0, 8);  // Limit to eight characters (short hash).
}

static std::string GetParentRevisionRange(const FileVersionDiff& diff,
                                          size_t parent_index) {
  // If there is no |file_parent_commit| to get the revision range for vs.
  // the current parent commit, then just look at the commit history of the
  // parent commit itself. This addresses the the "add file from subbranch"
  // case.
  std::string parent_revision_range =
#if 0
      diff.file_parent_commit_.IsValid()
          ? diff.file_parent_commit_.sha_ + std::string("..") +
                diff.parents_[parent_index].commit_.sha_
          : diff.parents_[parent_index].commit_.sha_;
#else
      GetShortHash(diff.parents_[0].commit_) + std::string("..") +
      GetShortHash(diff.parents_[parent_index].commit_);
#endif
      return parent_revision_range;
}

static GitHash GetBranchsBaseCommitFromGit(
    const FileVersionDiff& base_diff,
    const std::filesystem::path& file_path) {
  // Tricky: get the set of diffs from the *first* diff [oldest] in the
  // branch (right after it was branched) to the first ever commit. Then
  // the next diff entry will be the parent for this file (or empty if it
  // doesn't exist). That is:
  // | ... | parent (branch point) |
  // If the file is being added then the base is not in the first parent,
  // but instead in a subbranch.
  const auto parent_base_ref =
      GetShortHash(base_diff.commit_) + std::string("^");
  constexpr COutputWnd* pwndOutput = nullptr;
  GitDiffReader git_branch_to_head_diff_reader{
      file_path, parent_base_ref, pwndOutput,
      GitDiffReader::Opt::NO_FIRST_PARENT};

  const auto& diff_to_head_diffs = git_branch_to_head_diff_reader.GetDiffs();
  // This can legitimately be empty() if this is the adding diff.
  assert(!diff_to_head_diffs.empty() || base_diff.diff_tree_.action[0] == 'A' ||
         base_diff.diff_tree_.action[0] == 0);

  GitHash branches_base_commit;
  if (!diff_to_head_diffs.empty()) {
    branches_base_commit = diff_to_head_diffs.back().commit_;
  }

  return branches_base_commit;
}

static void LoadAllBranchesRecur(const std::filesystem::path& file_path,
                                 std::vector<FileVersionDiff>& diffs) {
  // Examine current branch's diffs.
  for (auto& diff : diffs) {
    // Handle any parents.
    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      // Read parent branch diffs.
      auto const parent_revision_range =
          GetParentRevisionRange(diff, parent_index);
      constexpr COutputWnd* pwndOutput = nullptr;
      GitDiffReader git_branch_diff_reader{file_path, parent_revision_range,
                                           pwndOutput};

      // Add new diffs to diffs tree.
      diff.parents_[parent_index].file_version_diffs_ =
          std::make_unique<std::vector<FileVersionDiff>>(
              std::move(git_branch_diff_reader.MoveDiffs()));

      // Find any base commit for branch.
      auto& parent_diffs = *diff.parents_[parent_index].file_version_diffs_;
      assert(!parent_diffs.empty());
      if (!parent_diffs.empty()) {
        auto& first_diff = parent_diffs.front();
        assert(!first_diff.file_parent_commit_.IsValid());
        first_diff.file_parent_commit_ =
            GetBranchsBaseCommitFromGit(first_diff, file_path);
      }

      // Recurse.
      LoadAllBranchesRecur(file_path, parent_diffs);
    }
  }
}

static FileVersionInstance LoadFileInstanceFromDiffs(
    const std::filesystem::path& file_path,
    const std::vector<FileVersionDiff>& diffs) {
  // Load current starting parent, if any.
  const GitHash parent_commit = diffs.empty() || diffs.front().parents_.empty()
                                    ? GitHash{}
                                    : diffs.front().file_parent_commit_;
  std::deque<std::string> lines;
  if (parent_commit.IsValid()) {
    const std::string parent_revision =
        parent_commit.sha_ + std::string(":./") + file_path.filename().string();
    GitFileReader git_file_reader_head{file_path.parent_path(),
                                       parent_revision};
    lines = std::move(git_file_reader_head.GetLines());
  }
  FileVersionInstance file_version_instance(diffs, std::move(lines),
                                            parent_commit);

  return file_version_instance;
}

static void TraverseAndVerifyAllBranchesRecur(
    FileVersionInstanceEditor& editor,
    const std::filesystem::path& file_path,
    const std::vector<FileVersionDiff>& diffs) {
  auto& file_version_instance =
      const_cast<FileVersionInstance&>(editor.GetFileVersionInstance());
  const FileVersionInstance original_instance =
      file_version_instance;  // Save off.

  int i = 0;
  // Reconstitute the orignal FileVersionInstance from diffs, going from
  // first diff forward.
  for (const auto& diff : diffs) {
    assert(editor.GetFileVersionInstance().commit_path_.DiffsSubbranch() ==
           &diffs);
#if USE_PRE_DIFF
    editor.AddDiff(diff);

    EXPECT_EQ(file_version_instance.GetCommit(), diff.commit_);
    CompareFileInstanceToCommit(file_version_instance, file_path, diff.commit_);
#endif

#if _DEBUG
    auto saved_diff_path = editor.GetFileVersionInstance().commit_path_;
#endif

    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      assert(diff.parents_[parent_index].file_version_diffs_);
      const auto& parent_diffs =
          *diff.parents_[parent_index].file_version_diffs_;

      const GitHash current_commit = file_version_instance.GetCommit();
      assert(editor.GetFileVersionInstance().commit_path_.DiffsSubbranch() ==
             &diffs);
#if _DEBUG
      const auto path_save = editor.GetFileVersionInstance().commit_path_;
#endif

      assert(!parent_diffs.empty());

      // Enter into any parent commit subbranch.
      if (parent_diffs.front().file_parent_commit_.IsValid()) {
        editor.EnterBranch(parent_diffs);
        assert(!parent_diffs
                    .empty());  // Ensure that we will add at least one diff
                                // such that we clear the "-1" subbranch index.

        TraverseAndVerifyAllBranchesRecur(editor, file_path, parent_diffs);

        editor.GoToCommit(current_commit);
        assert(editor.GetFileVersionInstance().commit_path_.DiffsSubbranch() ==
               &diffs);
        assert(editor.GetFileVersionInstance().commit_path_ == saved_diff_path);

        assert(editor.GetFileVersionInstance().GetCommit() == current_commit);
      }
    }

#if !USE_PRE_DIFF
    editor.AddDiff(diff);

    EXPECT_EQ(file_version_instance.GetCommit(), diff.commit_);
    CompareFileInstanceToCommit(file_version_instance, file_path, diff.commit_);
#endif

    i++;
  }

  const auto& last_commit = diffs.empty() ? GitHash{} : diffs.back().commit_;
  EXPECT_EQ(editor.GetFileVersionInstance().GetCommit(), last_commit);
  CompareFileInstanceToCommit(editor.GetFileVersionInstance(), file_path,
                              last_commit);

  // Now backwards in time, starting from the last recorded change in the git
  // log down to first recorded diff. Note that this may end up with nothing if
  // the last commit is the initial commit.
  for (auto it = diffs.crbegin(); it != diffs.crend(); it++) {
    const auto& diff = *it;
    editor.RemoveDiff(diff);
    EXPECT_EQ(editor.GetFileVersionInstance().GetCommit(),
              diff.file_parent_commit_);
  }

  EXPECT_TRUE(GitDiffReaderTest::IsTheSame(editor.GetFileVersionInstance(),
                                           original_instance));
}

static void LoadFileAndCompareAllBranches(
    const std::filesystem::path& file_path,
    std::vector<FileVersionDiff>& diffs_root) {
  EXPECT_GT(diffs_root.size(), 0U);

  LoadAllBranchesRecur(file_path, diffs_root);

  // Load current starting parent, if any.  TODO: use
  // LoadFileInstanceFromDiffs().
  const GitHash parent_commit =
      diffs_root.empty() || diffs_root.front().parents_.empty()
          ? GitHash{}
          : diffs_root.front().file_parent_commit_;
  std::deque<std::string> lines;
  if (parent_commit.IsValid()) {
    const std::string parent_revision =
        parent_commit.sha_ + std::string(":./") + file_path.filename().string();
    GitFileReader git_file_reader_head{file_path.parent_path(),
                                       parent_revision};
    lines = std::move(git_file_reader_head.GetLines());
  }
  FileVersionInstance file_version_instance(diffs_root, std::move(lines),
                                            parent_commit);
  constexpr Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(diffs_root, file_version_instance,
                                   listener_head);

  TraverseAndVerifyAllBranchesRecur(editor, file_path, diffs_root);

  EXPECT_TRUE(editor.GetFileVersionInstance().commit_path_.empty());
  EXPECT_EQ(file_version_instance.GetLines().size(), 0);
#if USE_SPARSE_INDEX_ARRAY
  EXPECT_TRUE(GitDiffReaderTest::GetLinesInfo(file_version_instance).IsEmpty());
#else
  EXPECT_EQ(GitDiffReaderTest::GetLinesInfo(file_version_instance).size(), 0);
#endif
}

TEST(GitDiffReaderTest, LoadAndCompareWithFileAllBranches) {
  _set_error_mode(_OUT_TO_MSGBOX);
  const std::filesystem::path anchor_file_path =
      L"C:\\Users\\leehu_000\\Source\\Repos\\libgit2\\libgit2";  //__FILE__;
#if 0
  for (auto i = std::filesystem::recursive_directory_iterator{anchor_file_path};
       i != std::filesystem::recursive_directory_iterator(); i++) {
    auto const& directory_entry = *i;
    if (directory_entry.is_directory()) {
      if (*directory_entry.path().filename().string().c_str() == '.')
        i.disable_recursion_pending();  // Skip past all '.' starting dirs like
                                        // '.git'.
      continue;
    }
    std::string empty_tag;
    GitDiffReader git_diff_reader(directory_entry, empty_tag);
    if (!git_diff_reader.GetDiffs().empty()) {
      printf("Processing %s...\n", directory_entry.path().string().c_str());
      LoadFileAndCompareAllBranches(directory_entry,
                                    std::move(git_diff_reader.MoveDiffs()));
    }
  }
#else
  const std::filesystem::path file_path =
      L"C:\\Users\\leehu_000\\Source\\Repos\\libgit2\\libgit2\\.mailmap";
  //"Sage2020_unittest/FileVersionInstance_unittest.cpp";
  //"FileVersionDiff.h";  // "ChangeHistoryPane.cpp";
  std::string empty_tag;
  GitDiffReader git_diff_reader(file_path, empty_tag);
  if (!git_diff_reader.GetDiffs().empty()) {
    printf("Processing %s...\n", file_path.string().c_str());
    LoadFileAndCompareAllBranches(file_path,
                                  std::move(git_diff_reader.MoveDiffs()));
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

// ** git diff <top-of-Master-pre-merge-commit>..<Master-merge-commit> -->
// merge's Branch changes **. This should be the command to add the proper diff
// to the top of the brnach to sync it with the master's commit (and then the
// master commit should be skipped).

static void PrintAllBranchesRecur(const std::vector<FileVersionDiff>& diffs,
                                  const int depth) {
  int index = 0;
  for (const auto& diff : diffs) {
    for (int i = 0; i < depth; i++)
      printf("  ");
    printf("%d %s %s %s hunks:%d\n", index, GetShortHash(diff.commit_).c_str(),
           diff.commit_.tag_.c_str(), diff.diff_tree_.action,
           diff.hunks_.size());

    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      const auto rev_range =
          parent_index ? GetParentRevisionRange(diff, parent_index) : "";
      for (int i = 0; i < depth; i++)
        printf("  ");
      printf("[%s]\n", rev_range.c_str());

      PrintAllBranchesRecur(*diff.parents_[parent_index].file_version_diffs_,
                            depth + 1);
    }

    index++;
  }
}

static void LoadFileAndPrintAllBranches(
    const std::filesystem::path& file_path,
    std::vector<FileVersionDiff>& diffs_root) {
  EXPECT_GT(diffs_root.size(), 0U);

  LoadAllBranchesRecur(file_path, diffs_root);

  constexpr int depth = 0;
  PrintAllBranchesRecur(diffs_root, depth);
}

TEST(GitDiffReaderTest, LoadFileAndPrintAllBranches) {
  _set_error_mode(_OUT_TO_MSGBOX);
  const std::filesystem::path file_path =
      L"C:\\Users\\leehu_000\\Source\\Repos\\libgit2\\libgit2\\.mailmap";
  // L"C:\\Users\\leehu_000\\Source\\Repos\\Sage2020\\Sage2020_unittest\\test_"
  // L"git_files\\add_with_modify_branch_no_ff.txt";
  std::string empty_tag;
  GitDiffReader git_diff_reader(file_path, empty_tag, nullptr);
  if (!git_diff_reader.GetDiffs().empty()) {
    printf("Processing %s...\n", file_path.string().c_str());
    LoadFileAndPrintAllBranches(file_path,
                                std::move(git_diff_reader.MoveDiffs()));
  }
}

TEST(GitDiffReaderTest, BranchMergeBlame) {
  _set_error_mode(_OUT_TO_MSGBOX);
  const std::filesystem::path file_path =
      // L"C:\\Users\\leehu_000\\Source\\Repos\\libgit2\\libgit2\\.mailmap";
      L"C:\\Users\\leehu_000\\Source\\Repos\\Sage2020\\Sage2020_unittest\\test_"
      L"git_files\\base.txt";
  const std::string empty_tag;
  GitDiffReader git_diff_reader(file_path, empty_tag);
  printf("Processing %s...\n", file_path.string().c_str());
  auto diffs_root = git_diff_reader.MoveDiffs();
  EXPECT_GT(diffs_root.size(), 0U);
  LoadAllBranchesRecur(file_path, diffs_root);

  auto file_instance = LoadFileInstanceFromDiffs(file_path, diffs_root);
  assert(file_instance.GetLines().size() ==
         0);  // No lines should have been loaded yet.
  constexpr Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(diffs_root, file_instance, listener_head);

  for (const auto& diff : diffs_root) {
    if (diff.parents_.size() > 1) {
      // Handle merge.
      // TODO: Recurse.
      assert(diff.parents_[1].file_version_diffs_);
      const auto& branch_diffs = *diff.parents_[1].file_version_diffs_;
      auto branch_file_instance =
          LoadFileInstanceFromDiffs(file_path, branch_diffs);
      FileVersionInstanceEditor branch_editor(
          branch_diffs, branch_file_instance, listener_head);

      // Load all diffs up to merge point.
      for (const auto& branch_diff : branch_diffs) {
        branch_editor.AddDiff(branch_diff);
      }

      // Get diff from tip of branch to merge (`git diff
      // <top-of-Master-pre-merge-commit>..<Master-merge-commit>`) to merge's
      // Branch changes. This should be the command to add the proper diff to
      // the top of the brnach to sync it with the master's commit (and then the
      // master commit should be skipped).
      constexpr COutputWnd* pwndOutput = nullptr;
      const std::string branch_to_main_merge_revision =
          diff.parents_[0].commit_.sha_ + std::string("..") +
          diff.parents_[1].commit_.sha_;
      GitDiffReader branch_to_main_merge_diff_reader{
          file_path, branch_to_main_merge_revision, pwndOutput,
          GitDiffReader::Opt::NO_FIRST_PARENT};

      // Apply the diff from branch_editor to editor...
      // Test 1: Ensure that applying diff to top of branch_file_instance makes
      // for an identical instance to if we added the merge commit.
      auto merge_diffs = branch_to_main_merge_diff_reader.MoveDiffs();
      merge_diffs.back().file_parent_commit_ = branch_diffs.back().commit_;
      EXPECT_EQ(merge_diffs.back().hunks_, diff.hunks_);
      // branch_editor.AddDiff(merge_diffs.back());
      // editor.AddDiff(diff);
      // EXPECT_TRUE(GitDiffReaderTest::IsTheSame(branch_file_instance,
      // file_instance));
    }
    editor.AddDiff(diff);
  }
}

static void TraverseAndVerifyAllBranchesTheNewWayRecur(
    FileVersionInstanceEditor& editor,
    const std::filesystem::path& file_path,
    const std::vector<FileVersionDiff>& diffs) {
  auto& file_version_instance =
      const_cast<FileVersionInstance&>(editor.GetFileVersionInstance());
  const FileVersionInstance original_instance =
      file_version_instance;  // Save a snapshot.

  int i = 0;
  // Reconstitute the orignal FileVersionInstance from diffs, going from
  // first diff forward.
  for (const auto& diff : diffs) {
#if _DEBUG
    auto saved_diff_path = editor.GetFileVersionInstance().commit_path_;
#endif

    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      assert(diff.parents_[parent_index].file_version_diffs_);
      const auto& parent_diffs =
          *diff.parents_[parent_index].file_version_diffs_;

      const GitHash current_commit = file_version_instance.GetCommit();
#if _DEBUG
      const auto path_save = editor.GetFileVersionInstance().commit_path_;
#endif

      assert(!parent_diffs.empty());

      TraverseAndVerifyAllBranchesTheNewWayRecur(editor, file_path,
                                                 parent_diffs);
    }

    editor.AddDiff(diff);

    EXPECT_EQ(file_version_instance.GetCommit(), diff.commit_);
    CompareFileInstanceToCommit(file_version_instance, file_path, diff.commit_);

    i++;
  }

  const auto& last_commit = diffs.empty() ? GitHash{} : diffs.back().commit_;
  EXPECT_EQ(editor.GetFileVersionInstance().GetCommit(), last_commit);
  CompareFileInstanceToCommit(editor.GetFileVersionInstance(), file_path,
                              last_commit);
}

TEST(GitDiffReaderTest, LoadAndValidateTheNewWay) {
  _set_error_mode(_OUT_TO_MSGBOX);
  const std::filesystem::path file_path =
      L"C:\\Users\\leehu_000\\Source\\Repos\\libgit2\\libgit2\\.mailmap";
      //L"C:\\Users\\leehu_000\\Source\\Repos\\Sage2020\\Sage2020_unittest\\test_"
      //L"git_files\\base.txt";
  const std::string empty_tag;
  GitDiffReader git_diff_reader(file_path, empty_tag);
  printf("Processing %s...\n", file_path.string().c_str());
  auto diffs_root = git_diff_reader.MoveDiffs();
  EXPECT_GT(diffs_root.size(), 0U);
  LoadAllBranchesRecur(file_path, diffs_root);

  auto file_instance = LoadFileInstanceFromDiffs(file_path, diffs_root);
  assert(file_instance.GetLines().size() ==
         0);  // No lines should have been loaded yet.
  constexpr Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(diffs_root, file_instance, listener_head);

  TraverseAndVerifyAllBranchesTheNewWayRecur(editor, file_path, diffs_root);
}
