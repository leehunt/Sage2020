#include "pch.h"

#include "FileVersionInstanceEditor.h"
#include "Sage2020ViewDocListener.h"

void FileVersionInstanceEditor::AddDiff(const FileVersionDiff& diff) {
  ++file_version_instance_.commit_index_;

  // N.b. This must be set here because AddHunk uses it.
  assert(diff.commit_.IsValid());
#ifdef _DEBUG
  const auto old_commit = file_version_instance_.commit_;
#endif
  file_version_instance_.commit_ = diff.commit_;

  for (auto& hunk : diff.hunks_) {
    AddHunk(hunk);
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfVersionChange(
        file_version_instance_.commit_index_);
  }
}

void FileVersionInstanceEditor::RemoveDiff(const FileVersionDiff& diff) {
  --file_version_instance_.commit_index_;

  for (auto it = diff.hunks_.crbegin(); it != diff.hunks_.crend(); it++) {
    RemoveHunk(*it);
  }

  assert(diff.file_parent_commit_.IsValid() ||
         file_version_instance_.commit_index_.empty());
  file_version_instance_.commit_ = diff.file_parent_commit_;

#ifdef _DEBUG
#if USE_SPARSE_INDEX_ARRAY
  for (int line_index = 0;
       line_index < file_version_instance_.file_lines_info_.MaxLineIndex();
       line_index++) {
    auto info = file_version_instance_.file_lines_info_.GetLineInfo(line_index);
    assert(static_cast<int>(info.commit_index()) <=
           file_version_instance_.commit_index_);
  }
#else
  for (size_t line_index = 0;
       line_index < file_version_instance_.file_lines_info_.size();
       line_index++) {
    auto& info = file_version_instance_.file_lines_info_[line_index];
    // TODO: Reimplement.
    // assert(static_cast<int>(info.commit_index()) <=
    //       file_version_instance_.commit_index_);
  }
#endif
#endif

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfVersionChange(
        file_version_instance_.commit_index_);
  }
}

bool FileVersionInstanceEditor::GoToCommit(
    const GitHash& commit,
    const std::vector<FileVersionDiff>& diffs_root) {
  bool edited = false;

  // Check if requested commit is on same branch.
  auto new_commit_path = GetDiffTreePath(commit, diffs_root);
  if (!new_commit_path.size()) {
    // Commit not found (!).
    return false;
  }

  auto current_commit_path =
      GetDiffTreePath(file_version_instance_.commit_, diffs_root);
  if (!current_commit_path.size()) {
    // Current commit not found (!!).
    VERIFY(FALSE);
    return false;
  }

  if (current_commit_path.HasSameParent(new_commit_path)) {
    const auto& diffs = *current_commit_path.back().subBranchRoot();
    for (size_t diff_index = 0; diff_index < diffs.size(); diff_index++) {
      if (diffs[diff_index].commit_ == commit) {
        edited = GoToIndex(diff_index, diffs);
        return edited;
      }
    }
    // Current commit not found (!!).
    VERIFY(FALSE);
    return false;
  }

  // 1. Find common parent.
  size_t common_path_index_max = 0;
  while (common_path_index_max < current_commit_path.size() &&
         common_path_index_max < new_commit_path.size() &&
         current_commit_path[common_path_index_max] ==
             new_commit_path[common_path_index_max]) {
    common_path_index_max++;
  }

  // 2. Traverse to common parent.
  size_t i = current_commit_path.size() - 1;
  for (; i > common_path_index_max; i--) {
    if (i > 0 && current_commit_path[i - 1].subBranchRoot() !=
                     current_commit_path[i].subBranchRoot()) {
      // Get ready to leave branch.
      edited |= GoToIndex(0, *current_commit_path[i].subBranchRoot());
      const auto& old_diff = current_commit_path[i].subBranchRoot()->front();
      RemoveDiff(old_diff);
      file_version_instance_.commit_index_ =
          current_commit_path.GetAncestors(i);
      continue;
    }
    edited |= GoToIndex(current_commit_path[i].currentBranchIndex(),
                        *current_commit_path[i].subBranchRoot());
  }

  // 3. Traverse up to new commit.
  for (; i < new_commit_path.size(); i++) {
    if (i > 0 && new_commit_path[i - 1].subBranchRoot() !=
                     new_commit_path[i].subBranchRoot()) {
      const auto& old_path = new_commit_path[i - 1];
      // First go to branch root when changing braches.
      const auto& new_diff = new_commit_path[i].subBranchRoot()->front();
      AddDiff(new_diff);
      file_version_instance_.commit_index_ = new_commit_path.GetAncestors(i);
      // Denote that we're at the root of the sub-branch.
      file_version_instance_.commit_index_.push_back(DiffTreePathItem());
      edited = true;
    }
    edited |= GoToIndex(new_commit_path[i].currentBranchIndex(),
                        *new_commit_path[i].subBranchRoot());
  }

  return edited;
}

bool FileVersionInstanceEditor::GoToIndex(
    size_t commit_index,
    const std::vector<FileVersionDiff>& diffs) {
  if (!diffs.size())
    return false;

  if (commit_index >= diffs.size())
    commit_index = diffs.size() - 1;

  bool edited = false;
  while (file_version_instance_.commit_index_ >
         static_cast<int>(commit_index)) {
    // N.b. Make a 'diff' copy reference since commit_index_ is modfied by
    // RemoveDiff().
    const auto& diff = diffs[file_version_instance_.commit_index_];
    RemoveDiff(diff);
    edited = true;
  }
  while (file_version_instance_.commit_index_ <
         static_cast<int>(commit_index)) {
    // N.b. Make a 'diff' copy reference since commit_index_ is modfied by
    // AddDiff().
    const auto& diff = diffs[(size_t)file_version_instance_.commit_index_ + 1];
    AddDiff(diff);
    edited = true;
  }

  file_version_instance_.commit_index_.back().setSubBranchRoot(&diffs);

  return edited;
}

void FileVersionInstanceEditor::AddHunk(const FileVersionDiffHunk& hunk) {
  // Remove lines.
  // N.b. Remove must be done first to get correct line locations.
  // Yes, using |add_location_| seems odd, but the add_location tracks the new
  // position of any adds or removes done by previous hunks in the diff.
  if (hunk.remove_line_count_ > 0) {
    // This is tricky. The |add_location_| is where to add in the 'to' lines
    // found when the diffs are added sequentially from first to last hunk
    // (that is, subsequent hunk's |add_location_|s are offset to account for
    // the previously added hunks). However if there are no add lines to replace
    // the removed lines, then the 'add_location' is shifted down by one because
    // that line no longer exists in the 'to' file.
    auto remove_location_index =
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_;
    assert(std::equal(
        file_version_instance_.file_lines_.begin() + remove_location_index,
        file_version_instance_.file_lines_.begin() + remove_location_index +
            hunk.remove_line_count_,
        hunk.remove_lines_.begin()));
    file_version_instance_.file_lines_.erase(
        file_version_instance_.file_lines_.begin() + remove_location_index,
        file_version_instance_.file_lines_.begin() + remove_location_index +
            hunk.remove_line_count_);

    if (!hunk.line_info_to_restore_) {
      hunk.line_info_to_restore_ =
          std::make_unique<LineToFileVersionLineInfo>();
      hunk.line_info_to_restore_->insert(
          hunk.line_info_to_restore_->begin(),
          file_version_instance_.file_lines_info_.begin() +
              remove_location_index,
          file_version_instance_.file_lines_info_.begin() +
              remove_location_index + hunk.remove_line_count_);
    }
    file_version_instance_.RemoveLineInfo(remove_location_index + 1,
                                          hunk.remove_line_count_);
  }

  // Add lines.
  if (hunk.add_line_count_ > 0) {
    auto it_insert =
        file_version_instance_.file_lines_.begin() + (hunk.add_location_ - 1);
    for (const auto& line : hunk.add_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstanceEditor.
      it_insert = file_version_instance_.file_lines_.insert(it_insert, line);
      ++it_insert;
    }
    // Add line info
    auto file_version_line_info =
        FileVersionLineInfo{file_version_instance_.commit_.sha_};

    LineToFileVersionLineInfo single_infos;
    single_infos.push_front(file_version_line_info);
    file_version_instance_.AddLineInfo(hunk.add_location_, hunk.add_line_count_,
                                       single_infos);
#ifdef _DEBUG
    for (auto line_num = hunk.add_location_;
         line_num < hunk.add_location_ + hunk.add_line_count_; line_num++) {
      //  assert(file_version_instance_.GetCommitFromIndex(
      //            file_version_instance_.GetLineInfo(line_num).commit_index())
      //            ==
      //         commit_id);
    }
#endif
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfEdit(
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_,
        hunk.add_line_count_ - hunk.remove_line_count_);
  }
}

void FileVersionInstanceEditor::RemoveHunk(const FileVersionDiffHunk& hunk) {
  // Remove lines by removing the "added" lines.
  // N.b. Remove must be done first to get correct line locations.
  // Yes, using |add_location_| seems odd, but the add_location tracks the
  // new position of any adds/removed done by previous hunks in the diff.
  if (hunk.add_line_count_ > 0) {
    assert(std::equal(
        file_version_instance_.file_lines_.begin() + (hunk.add_location_ - 1),
        file_version_instance_.file_lines_.begin() +
            (hunk.add_location_ - 1 + hunk.add_line_count_),
        hunk.add_lines_.begin()));
    file_version_instance_.file_lines_.erase(
        file_version_instance_.file_lines_.begin() + (hunk.add_location_ - 1),
        file_version_instance_.file_lines_.begin() +
            (hunk.add_location_ - 1 + hunk.add_line_count_));

    file_version_instance_.RemoveLineInfo(hunk.add_location_,
                                          hunk.add_line_count_);
  }

  // Add lines by adding the "removed" lines.
  if (hunk.remove_line_count_ > 0) {
    // This is tricky.  The 'add_location' is where to add in the 'to'
    // lines are found if diffs are removed sequentially in reverse (which is
    // how we use RemoveHunk).  However if there is no add lines to replace the
    // removed lines, then the 'add_location' is shifted down by one because
    // that line no longer exists in the 'to' file.
    auto add_location_index =
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_;
    auto it_insert =
        file_version_instance_.file_lines_.begin() + add_location_index;
    for (const auto& line : hunk.remove_lines_) {
      // REVIEW: Consider moving line data to/from hunk array and
      // FileVersionInstanceEditor.
      it_insert = file_version_instance_.file_lines_.insert(it_insert, line);
      ++it_insert;
    }

    // Restore line info
    assert(hunk.line_info_to_restore_);
    file_version_instance_.AddLineInfo(add_location_index + 1,
                                       hunk.remove_line_count_,
                                       *hunk.line_info_to_restore_);
#ifdef _DEBUG
    for (auto line_index = add_location_index;
         line_index < add_location_index + hunk.remove_line_count_;
         line_index++) {
      //  assert(file_version_instance_.GetCommitFromIndex(
      //          file_version_instance_.GetLineInfo(line_index).commit_index())
      //          ==
      //         commit_id);
    }
#endif
  }

  if (listener_head_ != nullptr) {
    listener_head_->NotifyAllListenersOfEdit(
        hunk.add_line_count_ ? hunk.add_location_ - 1 : hunk.add_location_,
        hunk.remove_line_count_ - hunk.add_line_count_);
  }
}

DiffTreePath FileVersionInstanceEditor::GetDiffTreePath(
    const GitHash& commit,
    const std::vector<FileVersionDiff>& diffs) {
  int i = 0;
  for (const auto& diff : diffs) {
    if (diff.commit_ == commit) {
      auto& item =
          DiffTreePathItem().setCurrentBranchIndex(i).setSubBranchRoot(&diffs);
      DiffTreePath path;
      path.push_back(std::move(item));

      return path;
    }

    for (size_t parent_index = 1; parent_index < diff.parents_.size();
         parent_index++) {
      if (diff.parents_[parent_index].file_version_diffs_) {
        const auto& parent_diffs =
            *diff.parents_[parent_index].file_version_diffs_;
        auto subpath = GetDiffTreePath(commit, parent_diffs);
        if (subpath.size() > 0) {
          assert(!parent_diffs.empty());  // This should be ensured by the
                                          // "subpath.size() > 0" check.
          // Look for index of |file_parent_commit_| in |diffs| (i.e. the commit
          // from which the sub-branch was created).
          auto child_index_it =
              std::find_if(diffs.cbegin(), diffs.cend(),
                           [&parent_diffs](const FileVersionDiff& cmp_diff) {
                             return cmp_diff.commit_ ==
                                    parent_diffs.front().file_parent_commit_;
                           });
          assert(child_index_it != diffs.cend());
          auto& parent_item = DiffTreePathItem()
                                  .setCurrentBranchIndex(
                                      child_index_it == diffs.cend()
                                          ? (size_t)-1
                                          : (child_index_it - diffs.cbegin()))
                                  .setParentIndex(parent_index)
                                  .setSubBranchRoot(&diffs);
          // Prepend the parent item to returned |subpath|.
          subpath.insert(subpath.cbegin(), std::move(parent_item));
          return subpath;
        }
      }
    }

    i++;
  }

  return {};
}
