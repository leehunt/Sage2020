#include "pch.h"
#include "../FileVersionInstance.h"
#include "../GitDiffReader.h"
#include "../GitFileReader.h"

TEST(GitDiffReaderTest, Load) {
  std::filesystem::path file_path = __FILE__;
  GitDiffReader git_diff_reader(file_path.filename());

  auto diffs = git_diff_reader.GetDiffs();
  FileVersionInstance file_version_instance;
  int commit_sequence_num = 0;
  for (auto& diff : diffs) {
    commit_sequence_num++;
    for (auto& hunk : diff.hunks_) {
      file_version_instance.AddHunk(hunk, commit_sequence_num);
    }
  }

  EXPECT_GT(diffs.size(), 0U);
  if (diffs.size() > 0) {
    GitFileReader git_file_reader{diffs[0].diff_tree_.new_hash_string};
    auto file_version_instance_loaded = std::make_unique<FileVersionInstance>(
        std::move(git_file_reader.GetLines()), commit_sequence_num);

    EXPECT_EQ(file_version_instance_loaded->GetLines().size(),
              file_version_instance.GetLines().size());
    for (size_t i = 0; i < file_version_instance.GetLines().size(); i++) {
      const auto& file_version_line_loaded = file_version_instance_loaded->GetLines()[i];
      const auto& file_version_line_from_diffs = file_version_instance.GetLines()[i];
      EXPECT_EQ(file_version_line_loaded, file_version_line_from_diffs);
    }
  }
}
