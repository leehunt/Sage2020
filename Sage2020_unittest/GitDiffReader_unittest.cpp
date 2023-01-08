#include "pch.h"

#include "../FileVersionInstance.h"
#include "../FileVersionInstanceEditor.h"
#include "../GitDiffReader.h"
#include "../GitFileReader.h"
#include "../Utility.h"

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
  // REVIEW: We could also use a revision of
  // |<commit>:<releative-git-file-path>|, which may be more clean/orthogonal as
  // we don't use blob hashes elsewhere.
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
      commit.sha_ + std::string(":") + git_releative_path;
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
  FileVersionInstance file_version_instance{};
  Sage2020ViewDocListener* listener_head = nullptr;
  FileVersionInstanceEditor editor(diffs, file_version_instance, listener_head);
  EXPECT_EQ(&editor.GetFileVersionInstance(), &file_version_instance);
  for (const auto& diff : diffs) {
    editor.AddDiff(diff);
    EXPECT_EQ(file_version_instance.GetCommit(), diff.commit_);
  }

  CompareFileInstanceToHead(file_path, diffs, file_version_instance);

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

static void LoadAllBranchesRecur(const std::filesystem::path& file_path,
                                 std::vector<FileVersionDiff>& diffs) {
  for (auto& diff : diffs) {
    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      std::string parent_revision_range =
#if 1  // REVIEW!!!!!
          (diff.parents_[0].commit_.IsValid() ? diff.parents_[0].commit_.sha_
                                              : "HEAD") +
#else
          (diff.file_parent_commit_.IsValid() ? diff.file_parent_commit_.sha_
                                              : "HEAD") +
#endif
          std::string("..") + diff.parents_[parent_index].commit_.sha_;
      GitDiffReader git_branch_diff_reader{file_path, parent_revision_range};
      diff.parents_[parent_index].file_version_diffs_ =
          std::make_unique<std::vector<FileVersionDiff>>(
              std::move(git_branch_diff_reader.MoveDiffs()));
      auto& parent_diffs = *diff.parents_[parent_index].file_version_diffs_;
      assert(!parent_diffs.empty());
      if (!parent_diffs.empty()) {
        assert(!parent_diffs.front().file_parent_commit_.IsValid());
        // Tricky: get the set of diffs from the *first* diff [oldest] in the
        // branch (right after it was branched) to the first ever commit. Then
        // the next diff entry will be the parent for this file (or empty if it
        // doesn't exist). That is:
        // | ... | parent (branch point) |
        constexpr COutputWnd* pwndOutput = nullptr;
        GitDiffReader git_branch_to_head_diff_reader{
            file_path, parent_diffs.front().commit_.sha_ + std::string("^"),
            pwndOutput,
            GitDiffReader::Opts(GitDiffReader::Opt::NO_FIRST_PARENT) |
                GitDiffReader::Opt::NO_FOLLOW};

        const auto& diff_to_head_diffs =
            git_branch_to_head_diff_reader.GetDiffs();
        // This can legitimately be empty() if this is the adding diff.
        assert(!diff_to_head_diffs.empty() ||
               parent_diffs.front().diff_tree_.action[0] == 'A');
        if (!diff_to_head_diffs.empty()) {
          parent_diffs.front().file_parent_commit_ =
              diff_to_head_diffs.back().commit_;
        }
      }
      LoadAllBranchesRecur(file_path, parent_diffs);
    }
  }
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
    editor.AddDiff(diff);
#if _DEBUG
    auto saved_diff_path = editor.GetFileVersionInstance().commit_path_;
#endif

    EXPECT_EQ(file_version_instance.GetCommit(), diff.commit_);
    CompareFileInstanceToCommit(file_version_instance, file_path, diff.commit_);

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

  // Load current starting parent, if any.
  const GitHash parent_commit =
      diffs_root.empty() || diffs_root.front().parents_.empty()
          ? GitHash{}
          : diffs_root.front().file_parent_commit_;
  std::deque<std::string> lines;
  if (parent_commit.IsValid()) {
    const std::string parent_revision =
        parent_commit.sha_ + std::string(":") + file_path.filename().string();
    GitFileReader git_file_reader_head{file_path.parent_path(),
                                       parent_revision};
    lines = std::move(git_file_reader_head.GetLines());
  }
  FileVersionInstance file_version_instance(std::move(lines), parent_commit);
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
#if 1
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
      L"C:\\Users\\leehu_000\\Source\\Repos\\libgit2\\libgit2\\.gitignore";
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
