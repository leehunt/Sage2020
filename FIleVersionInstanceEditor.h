#pragma once

#include <vector>

#include "DiffTreePath.h"

class GitDiffReaderTest;
struct FileVersionDiff;
struct FileVersionDiffHunk;
class Sage2020ViewDocListener;
class FileVersionInstance;
struct GitHash;

class FileVersionInstanceEditor {
 public:
  FileVersionInstanceEditor(const std::vector<FileVersionDiff>& diffs_root,
                            FileVersionInstance& file_version_instance,
                            Sage2020ViewDocListener* listener_head);

  // TODO: Make these private and expose only GoToCommit() and GoToIndex().
  void AddDiff(const FileVersionDiff& diff);
  void RemoveDiff(const FileVersionDiff& diff);

  // TODO: Make these private and expose only GoToCommit() and GoToIndex().
  //
  // Moves to |sub_branch|'s branch commit and sets the current branch_index_ to
  // |sub_branch|, but does not apply any diffs (i.e. the branch_index_ is -1).
  // Returns true if edit changes FileVersionInstance contents.
  bool EnterBranch(const std::vector<FileVersionDiff>& sub_branch);
  // Moves to |sub_branch|'s parent branch commit and sets the current
  // branch_index_ to that commit. Returns true if edit changes
  // FileVersionInstance contents.
  bool ExitBranch();

  // Returns true if edit changes FileVersionInstance contents.
  bool GoToCommit(const GitHash& commit);
  bool GoToDiff(const FileVersionDiff& diff);

  // Uses current |file_version_instance_| branch.
  // Returns true if edit changes FileVersionInstance contents.
  bool GoToIndex(int commit_index);

  int FindCommitIndexOnCurrentBranch(const GitHash&) const;

  const FileVersionInstance& GetFileVersionInstance() const {
    return file_version_instance_;
  }

 private:
  void AddHunk(const FileVersionDiffHunk& hunk);
  void RemoveHunk(const FileVersionDiffHunk& hunk);

  const std::vector<FileVersionDiff>& GetRoot() const;

  // Gets the path from Root to |commit| via merge (integation) commits.
  // This is typically used for walking the graph via its |Diff.parent_| nodes.
  // It also is used in showing the Merge Tree following intergration is more
  // intuitive than following branch commit.
  DiffTreePath GetDiffTreeMergePath(const GitHash& commit) const;

  // Gets the path from Root to |commit| via branching (base) commits.
  // This is typically used for walking the graph from one commit to another.
  DiffTreePath GetDiffTreeBranchPath(const GitHash& commit) const;

  const FileVersionDiff* FindDiff(
      const GitHash& commit,
      const std::vector<FileVersionDiff>& subroot) const;
  const std::vector<FileVersionDiff>* FindDiffs(
      const GitHash& commit,
      const std::vector<FileVersionDiff>& subroot) const;

 private:
  DiffTreePath GetDiffTreeMergePathRecur(
      const GitHash& commit,
      const std::vector<FileVersionDiff>& subroot) const;

  DiffTreePath GetDiffTreeBranchPathRecur(
      const GitHash& commit,
      const std::vector<FileVersionDiff>& subroot) const;

  FileVersionInstance& file_version_instance_;
  Sage2020ViewDocListener* listener_head_;
  friend GitDiffReaderTest;
};
