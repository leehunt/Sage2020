#include "pch.h"

#include "FIleVersionInstanceEditor.h"

#include <afx.h>
#include <assert.h>
#include <intsafe.h>
#include <execution>

#include "FileVersionDiff.h"
#include "FileVersionInstance.h"
#include "FileVersionLineInfo.h"
#include "GitHash.h"
#include "Sage2020ViewDocListener.h"

FileVersionInstanceEditor::FileVersionInstanceEditor(
    const std::vector<FileVersionDiff>& diffs_root,
    FileVersionInstance& file_version_instance,
    Sage2020ViewDocListener* listener_head)
    : file_version_instance_(file_version_instance),
      listener_head_(listener_head) {
  if (file_version_instance_.commit_path_.empty()) {
    DiffTreePathItem item;
    item.setCurrentBranchIndex(-1).setBranch(&diffs_root);
    file_version_instance_.commit_path_.push_back(item);
  }
}

void FileVersionInstanceEditor::AddDiff(const FileVersionDiff& diff) {
  // N.b. This must be incremented here because AddHunk uses it.
#if _DEBUG
  int new_int_index =
#endif
      ++file_version_instance_.commit_path_;
  assert(new_int_index >= 0);
  std::string commit_to_add = diff.commit_.sha_;
  commit_to_add.resize(8);
  std::string current_commit = file_version_instance_.GetCommit().sha_;
  current_commit.resize(8);
  printf("AddDiff:    %s (current %s) at path: %s\n", commit_to_add.c_str(),
         current_commit.c_str(),
         file_version_instance_.commit_path_.PathText().c_str());
  assert(diff.commit_.IsValid());
  // Check that we're applying the diff to the expected base commit.
#if 0   // TODO: FIX.
  assert(file_version_instance_.commit_ == diff.file_parent_commit_ ||
         !diff.file_parent_commit_.IsValid() &&
             (diff.diff_tree_.old_files.empty() ||
              diff.diff_tree_.old_files[0].action[0] == 'A'));
#endif  // 0
#ifdef _DEBUG
  const char* sha = diff.commit_.sha_;
  assert(strlen(sha));
  const GitHash old_commit = file_version_instance_.commit_;
  assert(old_commit.IsValid() || new_int_index == 0);
#endif
  file_version_instance_.commit_ = diff.commit_;

  /*
  for (auto& hunk : diff.hunks_) {
    AddHunk(hunk);
  }*/
#if OLD_DIFFS
  for (size_t hunk_index = 0; hunk_index < diff.remove_hunks_.size();
       hunk_index++) {
    FileVersionDiffHunk combined_hunk;
    const auto& remove_hunk = diff.remove_hunks_[hunk_index];

    combined_hunk.from_lines_ = remove_hunk.from_lines_;
    combined_hunk.from_location_ = remove_hunk.from_location_;
    combined_hunk.from_line_count_ = remove_hunk.from_line_count_;

    // Add the remove hunks from diff.
    for (size_t parent_index = 0; parent_index < diff.parents_.size();
         parent_index++) {
      assert(diff.remove_hunks_.size() ==
             diff.parents_[parent_index].add_hunks_.size());

      const auto& add_hunk = diff.parents_[parent_index].add_hunks_[hunk_index];

      combined_hunk.line_info_to_restore_ =
          std::move(remove_hunk.line_info_to_restore_);
      combined_hunk.to_lines_ = add_hunk.to_lines_;
      combined_hunk.to_location_ = add_hunk.to_location_;
      combined_hunk.to_line_count_ = add_hunk.to_line_count_;

      // Add the add hunks from diff.
      AddHunk(combined_hunk);

      remove_hunk.line_info_to_restore_ =
          std::move(combined_hunk.line_info_to_restore_);
    }
  }
#else
  for (auto& hunk : diff.hunks_) {
    // Apply change to all parents (parent_bits all set to 1).
    // TODO!!! Figure out correct way to account for parent.
    // int parent_bits = (1 << num_parents) - 1;
    int parent_bits = 1;
    constexpr bool reverse_action = false;
    ApplyCombinedHunk(hunk, parent_bits, reverse_action);
  }
#endif

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfVersionChange(
        file_version_instance_.commit_path_);
  }
}

void FileVersionInstanceEditor::RemoveDiff(const FileVersionDiff& diff) {
#ifdef _DEBUG
  const char* sha = diff.commit_.sha_;
  assert(strlen(sha));
  std::string commit_to_remove = diff.file_parent_commit_.sha_;
  commit_to_remove.resize(8);
  std::string current_commit = diff.commit_.sha_;
  current_commit.resize(8);
  // REVIEW: Why does this fail sometimes (e.g. with ci\test.sh)?
  // assert(file_version_instance_.GetCommit() == diff.commit_);
  printf("RemoveDiff: %s (going back to %s) at path: %s\n",
         current_commit.c_str(), commit_to_remove.c_str(),
         file_version_instance_.commit_path_.PathText().c_str());
#endif
  /*
  for (auto it = diff.hunks_.crbegin(); it != diff.hunks_.crend(); it++) {
    RemoveHunk(*it);
  }
  */
#if OLD_DIFFS
  for (size_t hunk_index = diff.remove_hunks_.size(); hunk_index-- > 0;) {
    FileVersionDiffHunk combined_hunk;
    const auto& remove_hunk = diff.remove_hunks_[hunk_index];

    combined_hunk.from_lines_ = remove_hunk.from_lines_;
    combined_hunk.from_location_ = remove_hunk.from_location_;
    combined_hunk.from_line_count_ = remove_hunk.from_line_count_;

    for (size_t parent_index = diff.parents_.size(); parent_index-- > 0;) {
      assert(diff.remove_hunks_.size() ==
             diff.parents_[parent_index].add_hunks_.size());

      const auto& add_hunk = diff.parents_[parent_index].add_hunks_[hunk_index];

      combined_hunk.line_info_to_restore_ =
          std::move(remove_hunk.line_info_to_restore_);
      combined_hunk.to_lines_ = add_hunk.to_lines_;
      combined_hunk.to_location_ = add_hunk.to_location_;
      combined_hunk.to_line_count_ = add_hunk.to_line_count_;

      // Remove the add hunks from diff.
      RemoveHunk(combined_hunk);

      remove_hunk.line_info_to_restore_ =
          std::move(combined_hunk.line_info_to_restore_);
    }
  }
#else
  for (auto it = diff.hunks_.crbegin(); it != diff.hunks_.crend(); it++) {
    const auto& hunk = *it;
    // Apply change to all parents (parent_bits all set to 1).
    // TODO: Fix for other parents!
    // const int num_parents = hunk.ranges_.size() - 1;
    // int parent_bits = (1 << num_parents) - 1;
    int parent_bits = 1;
    constexpr bool reverse_action = true;
    ApplyCombinedHunk(hunk, parent_bits, reverse_action);
  }
#endif

  // Special: If we're deleting the last diff, the decrement operator will pop
  // off the tip of the |commit_path_|.
#if _DEBUG
  int new_int_index =
#endif
      --file_version_instance_.commit_path_;
  assert(new_int_index >= -1);
  // Check that we're removing the diff to the expected base commit.
  assert(diff.file_parent_commit_.IsValid() ||
         (!diff.diff_tree_.old_files.empty() &&
          diff.diff_tree_.old_files[0].action[0] == 'A') ||
         diff.hunks_.empty() ||
         // file_version_instance_.commit_path_.empty() ||
         file_version_instance_.commit_path_ == -1);

  file_version_instance_.commit_ = diff.file_parent_commit_;

#ifdef _DEBUG
#if USE_SPARSE_INDEX_ARRAY
  for (int line_index = 0;
       line_index < file_version_instance_.file_lines_info_.MaxLineIndex();
       line_index++) {
    auto info = file_version_instance_.file_lines_info_.GetLineInfo(line_index);
    assert(static_cast<int>(info.commit_index()) <=
           file_version_instance_.commit_path_);
  }
#else
#if 0
  for (size_t line_index = 0;
       line_index < file_version_instance_.file_lines_info_.size();
       line_index++) {
    auto& info = file_version_instance_.file_lines_info_[line_index];
    // TODO: Reimplement. But how? |info.commit_sha()| doesn't respect
    // inequalities.
    // assert(static_cast<int>(info.commit_index()) <=
    //       file_version_instance_.commit_path_);
  }
#endif  // 0
#endif
#endif  // _DEBUG

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfVersionChange(
        file_version_instance_.commit_path_);
  }
}

bool FileVersionInstanceEditor::EnterBranch(
    const std::vector<FileVersionDiff>& new_branch) {
  const auto old_branch = GetFileVersionInstance().GetBranchDiffs();
  const auto old_path = file_version_instance_.commit_path_;
  const auto& parent_branch_base_commit =
      new_branch.front().file_parent_commit_;

  // Go to the current branch's base commit.
  assert(parent_branch_base_commit.IsValid());
  bool edited = GoToCommit(parent_branch_base_commit);

  // Add new '-1' index entry for subbranch.
  auto& new_item =
      DiffTreePathItem().setCurrentBranchIndex(-1).setBranch(&new_branch);
  file_version_instance_.commit_path_.push_back(std::move(new_item));

  printf("EnterBranch: old path: %s; new path: %s\n",
         old_path.PathText().c_str(),
         file_version_instance_.commit_path_.PathText().c_str());

  std::vector<FileVersionDiff> empty_diff{};
  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfBranchChange(
        old_branch ? *old_branch : empty_diff, new_branch);
  }

  return edited;
}

bool FileVersionInstanceEditor::ExitBranch() {
  const auto old_branch = GetFileVersionInstance().GetBranchDiffs();
  const auto old_path = file_version_instance_.commit_path_;
  assert(file_version_instance_.commit_path_.size() > 1);

#if _DEBUG
  const auto& parent_branch_base_commit =
      old_branch->front().file_parent_commit_;
  assert(parent_branch_base_commit.IsValid());
#endif

  // N.b. This will pop |commit_path_| by one.
  bool edited = GoToIndex(-1);

  const auto new_branch = GetFileVersionInstance().GetBranchDiffs();
  printf("ExitBranch: old path: %s; new path: %s\n",
         old_path.PathText().c_str(),
         file_version_instance_.commit_path_.PathText().c_str());

  std::vector<FileVersionDiff> empty_diff{};
  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfBranchChange(
        old_branch ? *old_branch : empty_diff,
        new_branch ? *new_branch : empty_diff);
  }

  return edited;
}

bool FileVersionInstanceEditor::GoToDiff(const FileVersionDiff& diff) {
  return GoToCommit(diff.commit_);
}

bool FileVersionInstanceEditor::GoToCommit(const GitHash& commit) {
  bool edited = false;

  // Not on same branch:
  // 1. Find common root.
  // 2. Move down to common root.
  // 3. Move up to destination commit branch.
  // 4. Move to index on destination branch.
  DiffTreePath current_commit_path = GetFileVersionInstance().commit_path_;
  if (!current_commit_path.size()) {
    // Current commit not found (!!).
    VERIFY(FALSE);
    return false;
  }
  auto new_commit_path = GetDiffTreeBranchPath(commit);
  if (!new_commit_path.size()) {
    // Commit not found (!).
    VERIFY(FALSE);
    return false;
  }
  // Not on current branch.
  // 1. Find common parent.
  size_t common_path_index_max = 0;
  while (common_path_index_max < current_commit_path.size() &&
         common_path_index_max < new_commit_path.size()) {
    if ((current_commit_path[common_path_index_max].branch() !=
         new_commit_path[common_path_index_max].branch()) ||
        (current_commit_path[common_path_index_max].parentIndex() !=
         new_commit_path[common_path_index_max].parentIndex())) {
      break;
    }
    common_path_index_max++;
  }
  assert(common_path_index_max > 0);  // The root should always match.

  // 2. Move down to common root.
  while (current_commit_path.size() > common_path_index_max) {
    assert(current_commit_path.size() > 1);
    edited = ExitBranch() || edited;

    current_commit_path = GetFileVersionInstance().commit_path_;
  }

  // 3. Move up to destination commit branch.
  while (current_commit_path.size() < new_commit_path.size()) {
    assert(common_path_index_max < new_commit_path.size());

    const auto branch = new_commit_path[common_path_index_max].branch();
    assert(branch);

    edited = EnterBranch(*branch) || edited;

    common_path_index_max++;
    current_commit_path = GetFileVersionInstance().commit_path_;
  }

  // 4. Move to index on destination branch.
  const int diff_index = FindCommitIndexOnCurrentBranch(commit);
  assert(diff_index != -1);
  if (diff_index != -1) {
    edited = GoToIndex(diff_index) || edited;
  }

  return edited;
}

bool FileVersionInstanceEditor::GoToIndex(int commit_index) {
  const auto diffs_ptr = GetFileVersionInstance().GetBranchDiffs();
  if (diffs_ptr == nullptr)
    return false;

  const auto& diffs = *diffs_ptr;
  if (diffs.empty())
    return commit_index == -1;  // Return true if going to no index (-1).

  assert(diffs.size() <= INT_MAX);
  assert(commit_index < static_cast<int>(diffs.size()));
  if (commit_index >= static_cast<int>(diffs.size()))
    commit_index = static_cast<int>(diffs.size() - 1);

  assert(commit_index >= -1);
  bool edited = false;
  while (file_version_instance_.commit_path_ > commit_index) {
    // N.b. Make a 'diff' copy reference since commit_path_ is modfied by
    // RemoveDiff().
    int old_index = file_version_instance_.commit_path_;
    assert(old_index >= 0);
#if _DEBUG
    const auto old_path = file_version_instance_.commit_path_;
#endif
    const auto& diff = diffs[old_index];
    RemoveDiff(diff);
    assert(old_index ? file_version_instance_.commit_path_ == old_index - 1
                     : file_version_instance_.commit_path_.size() ==
                           old_path.size() - 1);
    edited = true;
    if (old_index == 0)
      break;  // N.b. This is necessary since RemoveDiff() will pop
              // |commit_path_| when removing last diff in branch.
  }
  while (file_version_instance_.commit_path_ < commit_index) {
    // N.b. Make a 'diff' copy reference since commit_path_ is modfied by
    // AddDiff().
#if _DEBUG
    int old_index = file_version_instance_.commit_path_;
#endif
    const auto& diff = diffs[(size_t)file_version_instance_.commit_path_ + 1];
    AddDiff(diff);
    assert(file_version_instance_.commit_path_ == old_index + 1);
    edited = true;
  }

  return edited;
}

#if OLD_DIFFS
void FileVersionInstanceEditor::AddHunk(const FileVersionDiffHunk& hunk) {
  // Remove lines.
  // N.b. Remove must be done first to get correct line locations.
  // Yes, using |to_location_| seems odd, but the add_location tracks the new
  // position of any adds or removes done by previous hunks in the diff.
  if (hunk.from_line_count_ > 0) {
    // This is tricky. The |to_location_| is where to add in the 'to' lines
    // found when the diffs are added sequentially from first to last hunk
    // (that is, subsequent hunk's |to_location_|s are offset to account for
    // the previously added hunks). However if there are no add lines to replace
    // the removed lines, then the 'add_location' is shifted down by one because
    // that line no longer exists in the 'to' file.
    auto from_location_index =
        hunk.to_line_count_ ? hunk.to_location_ - 1 : hunk.to_location_;
    assert(std::equal(
        file_version_instance_.file_lines_.begin() + from_location_index,
        file_version_instance_.file_lines_.begin() + from_location_index +
            hunk.from_line_count_,
        hunk.from_lines_.begin()));
    file_version_instance_.file_lines_.erase(
        file_version_instance_.file_lines_.begin() + from_location_index,
        file_version_instance_.file_lines_.begin() + from_location_index +
            hunk.from_line_count_);

    // TODO!!!! : This needs to be hoisted up to use both remove and add hunks.
    // Stash away line info.
    if (!hunk.line_info_to_restore_) {
      hunk.line_info_to_restore_ = file_version_instance_.SnapshotLineInfo(
          from_location_index + 1, hunk.from_line_count_);
    }
    file_version_instance_.RemoveLineInfo(from_location_index + 1,
                                          hunk.from_line_count_);
  }

  // Add lines.
  if (hunk.to_line_count_ > 0) {
    auto it_insert =
        file_version_instance_.file_lines_.begin() + (hunk.to_location_ - 1);
    for (const auto& line : hunk.to_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstanceEditor.
      it_insert = file_version_instance_.file_lines_.insert(it_insert, line);
      ++it_insert;
    }
    // Add line info
    auto file_version_line_info =
        FileVersionLineInfo{file_version_instance_.commit_.sha_};

    LineToFileVersionLineInfo single_infos;
    single_infos.push_front(file_version_line_info);  // REVIEW: std::move().
    file_version_instance_.AddLineInfo(hunk.to_location_, hunk.to_line_count_,
                                       single_infos);  // REVIEW: std::move().
#ifdef _DEBUG
    for (auto line_num = hunk.to_location_;
         line_num < hunk.to_location_ + hunk.to_line_count_; line_num++) {
      //  assert(file_version_instance_.GetCommitFromIndex(
      //            file_version_instance_.GetLineInfo(line_num).commit_index())
      //            ==
      //         commit_id);
    }
#endif
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfEdit(
        hunk.to_line_count_ ? hunk.to_location_ - 1 : hunk.to_location_,
        hunk.to_line_count_ - hunk.from_line_count_);
  }
}
#endif  // OLD_DIFFS

#if OLD_DIFFS
void FileVersionInstanceEditor::RemoveHunk(const FileVersionDiffHunk& hunk) {
  // Remove lines by removing the "added" lines.
  // N.b. Remove must be done first to get correct line locations.
  // Yes, using |to_location_| seems odd, but the add_location tracks the
  // new position of any adds/removed done by previous hunks in the diff.
  if (hunk.to_line_count_ > 0) {
    assert(std::equal(
        file_version_instance_.file_lines_.begin() + (hunk.to_location_ - 1),
        file_version_instance_.file_lines_.begin() +
            (hunk.to_location_ - 1 + hunk.to_line_count_),
        hunk.to_lines_.begin()));
    // TODO!!!! : This needs to be hoisted up to use both remove and add hunks.
    file_version_instance_.file_lines_.erase(
        file_version_instance_.file_lines_.begin() + (hunk.to_location_ - 1),
        file_version_instance_.file_lines_.begin() +
            (hunk.to_location_ - 1 + hunk.to_line_count_));

    file_version_instance_.RemoveLineInfo(hunk.to_location_,
                                          hunk.to_line_count_);
  }

  // Add lines by adding the "removed" lines.
  if (hunk.from_line_count_ > 0) {
    // This is tricky.  The 'to_location_' is where to add in the 'to'
    // lines are found if diffs are removed sequentially in reverse (which is
    // how we use RemoveHunk).  However if there is no add lines to replace the
    // removed lines, then the 'add_location' is shifted down by one because
    // that line no longer exists in the 'to' file.
    auto to_location_index =
        hunk.to_line_count_ ? hunk.to_location_ - 1 : hunk.to_location_;
    auto it_insert =
        file_version_instance_.file_lines_.begin() + to_location_index;
    for (const auto& line : hunk.from_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstanceEditor.
      it_insert = file_version_instance_.file_lines_.insert(it_insert, line);
      ++it_insert;
    }

    // Restore line info
    assert(hunk.line_info_to_restore_);
    file_version_instance_.AddLineInfo(to_location_index + 1,
                                       hunk.from_line_count_,
                                       *hunk.line_info_to_restore_);
#ifdef _DEBUG
    for (auto line_index = to_location_index;
         line_index < to_location_index + hunk.from_line_count_; line_index++) {
      //  assert(file_version_instance_.GetCommitFromIndex(
      //          file_version_instance_.GetLineInfo(line_index).commit_index())
      //          ==
      //         commit_id);
    }
#endif
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfEdit(
        hunk.to_line_count_ ? hunk.to_location_ - 1 : hunk.to_location_,
        hunk.from_line_count_ - hunk.to_line_count_);
  }
}
#endif  // OLD_DIFFS

// Applies any diff line starting with action '-', '+' or ' ' * |num_parents|
// (|num_parents| > 1 is the combined diff case). Only diffs actions of index n
// |parent_bits| will be applied. To remove the the (e.g. when removing diffs,
// set |reverse_action| to true.
void FileVersionInstanceEditor::ApplyDiffLine(const std::string& line,
                                              int& location_index,
                                              int num_parents,
                                              int parent_bits,
                                              bool reverse_action) {
  char action = ' ';
  int i = 0;
  for (; i < num_parents; i++) {
    if (line[i] != ' ' && (parent_bits & (1 << i))) {
      action = line[i];
      break;
    }
  }
#if _DEBUG
  // Ensure that any remaining branch edits are the same action
  // (e.g. no '+-' or '-+').
  for (i++; i < num_parents; i++) {
    assert(line[i] == action || line[i] == ' ');
  }
#endif
  if (reverse_action) {
    if (action == '+')
      action = '-';
    else if (action == '-')
      action = '+';
  }

  switch (action) {
    case '+': {
      assert(location_index >= 0);
      file_version_instance_.file_lines_.insert(
          file_version_instance_.file_lines_.begin() + location_index,
          line.substr(num_parents));

      // Add new line info.
      auto file_version_line_info =
          FileVersionLineInfo{file_version_instance_.commit_.sha_};
      LineToFileVersionLineInfo single_infos;
      single_infos.push_front(std::move(file_version_line_info));
      file_version_instance_.AddLineInfo(location_index + 1, 1,
                                         std::move(single_infos));

      location_index++;
      break;
    }
    case '-': {
      assert(location_index >= 0);
      assert(file_version_instance_.file_lines_[location_index] ==
             line.substr(num_parents));
      file_version_instance_.file_lines_.erase(
          file_version_instance_.file_lines_.begin() + location_index);

      // Remove old line info.
      file_version_instance_.RemoveLineInfo(location_index + 1, 1);
      break;
    }
    case ' ': {
      // Context line or other branch edit in a combined diff.
      assert(location_index >= 0);
      // Look for combined diff case of an edit from another parent, the
      // latter's subtraction lines -- like this branch's ones -- are ignored.
#if _DEBUG
      bool found_edit = false;
#endif
      for (i = 0; i < num_parents; i++) {
        if (line[i] == '-') {
          break;  // Ignore any other branch's subtract.
        }
#if _DEBUG
        if (line[i] != ' ') {
          found_edit = true;
          assert(line[i] == '+');
        }
#endif
      }
      const bool other_branch_subtract = i != num_parents;
      if (!other_branch_subtract) {
        // |!other_branch_subtract| && |!found_edit| --> A context line that
        // should match the current file state.
        assert(found_edit ||
               file_version_instance_.file_lines_[location_index] ==
                   line.substr(num_parents));
        location_index++;
      }
      break;
    }
    default:
      assert(false);
  }
}

void FileVersionInstanceEditor::ApplyCombinedHunk(
    const FileVersionCombinedDiffHunk& hunk,
    int parent_bits,
    bool reverse_action) {
  const auto& to_range = hunk.ranges_.back();
  const int to_location_index =
      to_range.count_ ? to_range.location_ - 1 : to_range.location_;
  // `to_range.location_ if !to_range.count_`: This is necessary because of
  // diffs like this**. Note how the to start index is one less after deleting
  // the line, even though conceptually we're still on the same line (and when
  // the converse happens here## when adding a line, the before/after indexes
  // more rationally don't change. Sigh.
  /*
  index bca98bd..d4ec793 100644
  --- a/FileVersionInstance.cpp
  +++ b/FileVersionInstance.cpp
  @@ -6 +6 @@
  -FileVersionInstance::FileVersionInstance() : commit_index_(-1) {}
  +FileVersionInstance::FileVersionInstance() {}
  @@ -10,2 +10,2 @@
  FileVersionInstance::FileVersionInstance(std::deque<std::string>& lines,
  -    : commit_index_(-1),
  -      commit_(parent_commit) {  // N.b. parent_commit has a -1 index.
  +    : commit_(parent_commit) {  // N.b. When set to the parent_commit
  +                                // |commit_index_| is empty.
  @@ -47 +46,0 @@ void FileVersionInstance::AddLineInfo( **
  -    }
  @@ -50,0 +50 @@ void FileVersionInstance::AddLineInfo( ##
  +}
  @@ -52 +52 @@ void FileVersionInstance::AddLineInfo(
  -  void FileVersionInstance::RemoveLineInfo(int line_num, int line_count) {
  +void FileVersionInstance::RemoveLineInfo(int line_num, int line_count) {
  */
  assert(to_location_index >= 0);

  // Apply diff edits.
  int running_to_location_index = to_location_index;
  const int num_parents = hunk.ranges_.size() - 1;
  for (const auto& line : hunk.lines_) {
    ApplyDiffLine(line, running_to_location_index, num_parents, parent_bits,
                  reverse_action);
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfEdit(to_location_index,
                                             to_range.count_);
  }
}

// Gets the path from Root to |commit| via the branching commits.
DiffTreePath FileVersionInstanceEditor::GetDiffTreeBranchPath(
    const GitHash& commit) const {
  const auto& diffs_root = GetRoot();
  auto path = GetDiffTreeBranchPathRecur(commit, diffs_root);
  assert(path.empty() || path.front().branch() == &diffs_root);
  return path;
}
DiffTreePath FileVersionInstanceEditor::GetDiffTreeBranchPathRecur(
    const GitHash& commit,
    const std::vector<FileVersionDiff>& subroot) const {
  int item_index = 0;
  for (const auto& diff : subroot) {
    if (diff.commit_ == commit) {
      auto item = DiffTreePathItem()
                      .setCurrentBranchIndex(item_index)
                      .setBranch(&subroot);

      auto& parent_commit = subroot.front().file_parent_commit_;
      if (parent_commit.IsValid()) {
        // Add ancestors.
        auto path = GetDiffTreeBranchPath(parent_commit);
        path.push_back(std::move(item));
        return path;
      } else {
        // REVIEW this...
        // If there is no |file_parent_commit_| then we're at the root.
        assert(subroot == GetRoot());
        // If there is no |file_parent_commit_| then we're adding the file.
        // assert(subroot.front().diff_tree_.old_files[0].action[0] == 'A');
        DiffTreePath path{GetRoot()};
        path.push_back(std::move(item));
        return path;
      }
    }

    // Keep looking deeper in the tree for the commit.
    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      auto& parent = diff.parents_[parent_index];
      assert(parent.file_version_diffs_);
      if (parent.file_version_diffs_) {
        const auto& path =
            GetDiffTreeBranchPathRecur(commit, *parent.file_version_diffs_);
        if (!path.empty())
          return path;
      }
    }

    item_index++;
  }

  return DiffTreePath{GetRoot()};
}

// Gets the path from Root to |commit| via the integration commits.
// REVIEW: Do we still need this?
DiffTreePath FileVersionInstanceEditor::GetDiffTreeMergePath(
    const GitHash& commit) const {
  const auto& diffs_root = GetRoot();
  return GetDiffTreeMergePathRecur(commit, diffs_root);
}

DiffTreePath FileVersionInstanceEditor::GetDiffTreeMergePathRecur(
    const GitHash& commit,
    const std::vector<FileVersionDiff>& subroot) const {
  int i = -1;  // Merge indexes merge before the current commit at the parent,
               // so start with -1 (which is the index of a Diffs collection
               // before the 0th index).
  for (const auto& diff : subroot) {
    if (diff.commit_ == commit) {
      auto& item =
          DiffTreePathItem().setCurrentBranchIndex(i).setBranch(&subroot);
      DiffTreePath path{GetRoot()};
      path.push_back(std::move(item));

      return path;
    }

    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      if (diff.parents_[parent_index].file_version_diffs_) {
        const auto& parent_diffs =
            *diff.parents_[parent_index].file_version_diffs_;
        auto subpath = GetDiffTreeMergePathRecur(commit, parent_diffs);
        if (subpath.size() > 0) {
          assert(!parent_diffs.empty());  // This should be ensured by the
                                          // "subpath.size() > 0" check.
          // Record current index and parent index.
          auto& parent_item = DiffTreePathItem()
                                  .setCurrentBranchIndex(i)
                                  .setParentIndex(parent_index)
                                  .setBranch(&subroot);
          // Insert new |parent_item| at the beginning.
          subpath.insert(subpath.cbegin(), std::move(parent_item));
          return subpath;
        }
      }
    }

    i++;
  }

  return DiffTreePath{GetRoot()};
}

const std::vector<FileVersionDiff>& FileVersionInstanceEditor::GetRoot() const {
  return file_version_instance_.commit_path_.GetRoot();
}

int FileVersionInstanceEditor::FindCommitIndexOnCurrentBranch(
    const GitHash& commit) const {
  const auto diffs = GetFileVersionInstance().GetBranchDiffs();
  if (diffs == nullptr)
    return -1;

  auto child_index_it =
      std::find_if(diffs->cbegin(), diffs->cend(),
                   [&commit](const FileVersionDiff& cmp_diff) {
                     return cmp_diff.commit_ == commit;
                   });
  return child_index_it != diffs->cend()
             ? static_cast<int>(child_index_it - diffs->cbegin())
             : -1;
}

const FileVersionDiff* FileVersionInstanceEditor::FindDiff(
    const GitHash& commit,
    const std::vector<FileVersionDiff>& subroot) const {
  for (const auto& diff : subroot) {
    if (diff.commit_ == commit)
      return &diff;

    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      auto& parent_diffs = diff.parents_[parent_index].file_version_diffs_;
      assert(parent_diffs);
      if (parent_diffs) {
        const auto* diff = FindDiff(commit, *parent_diffs);
        if (diff)
          return diff;
        // Continue searching in other subbranches.
      }
    }
  }

  return nullptr;
}

const std::vector<FileVersionDiff>* FileVersionInstanceEditor::FindDiffs(
    const GitHash& commit,
    const std::vector<FileVersionDiff>& subroot) const {
  for (const auto& diff : subroot) {
    if (diff.commit_ == commit)
      return &subroot;

    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      auto& parent_diffs = diff.parents_[parent_index].file_version_diffs_;
      assert(parent_diffs);
      if (parent_diffs) {
        const auto* diffs = FindDiffs(commit, *parent_diffs);
        if (diffs)
          return diffs;
        // Continue searching in other subbranches.
      }
    }
  }

  return nullptr;
}