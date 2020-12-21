#include "pch.h"

#include "../FileVersionInstance.h"
#include "../FileVersionInstanceEditor.h"
#include "../GitDiffReader.h"
#include "../GitFileReader.h"

// Gtest 'friend' forwarders.
class GitDiffReaderTest : public testing::Test {
 public:
  static void AddHunk(FileVersionInstanceEditor& this_ref,
                      const FileVersionDiffHunk& hunk) {
    this_ref.FileVersionInstanceEditor::AddHunk(hunk);
  }
  static void RemoveHunk(FileVersionInstanceEditor& this_ref,
                         const FileVersionDiffHunk& hunk) {
    this_ref.FileVersionInstanceEditor::RemoveHunk(hunk);
  }

  static const LineToFileVersionLineInfo& GetLinesInfo(
      FileVersionInstance& this_ref) {
    return this_ref.GetLinesInfo();
  }
};

TEST(GitDiffReaderTest, LoadAndCompareWithFile) {
  std::filesystem::path file_path = __FILE__;
  GitDiffReader git_diff_reader(file_path.filename());

  auto diffs = git_diff_reader.GetDiffs();
  EXPECT_GT(diffs.size(), 0U);
  // Sythethesize FileVersionInstance from diffs, going from first diff (the
  // last recorded in the git log) forward.
  FileVersionInstance file_version_instance;
  FileVersionInstanceEditor editor(file_version_instance);
  for (auto it = diffs.crbegin(); it != diffs.crend(); it++) {
    editor.AddDiff(*it);
  }

  std::string latest_file_id = diffs.front().diff_tree_.new_hash_string;
  GitFileReader git_file_reader_latest{latest_file_id};
  auto file_version_instance_loaded = std::make_unique<FileVersionInstance>(
      std::move(git_file_reader_latest.GetLines()), diffs.front().commit_);

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
    for (auto itHunk = it->hunks_.crbegin(); itHunk != it->hunks_.crend();
         ++itHunk) {
      GitDiffReaderTest::RemoveHunk(editor, *itHunk);
    }
  }

  EXPECT_EQ(file_version_instance.GetLines().size(), 0);
  EXPECT_EQ(GitDiffReaderTest::GetLinesInfo(file_version_instance).size(), 0);
}
