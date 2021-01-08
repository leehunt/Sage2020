#include "pch.h"

#include <direct.h>
#include <stdio.h>
#include <cassert>
#include <functional>
#include <map>
#include <string>
#include "GitDiffReader.h"
#include "GitFileStreamCache.h"
#include "LineTokenizer.h"
#include "Utility.h"

constexpr char kGitDiffCommand[] =
    "git --no-pager log -p -U0 --raw --no-color --pretty=raw -- ";

static std::string GetTextToEndOfLine(TOK* ptok) {
  std::string text;
  char* szStart = ptok->szVal;

  while (ptok->tk != TK::tkNEWLINE && ptok->tk != TK::tkNil) {
    if (!FGetTok(ptok))
      break;
  }

  // Keep any '\n', but ensure the string is terminated.
  char chSav = ptok->szVal[1];
  ptok->szVal[1] = L'\0';

  text = szStart;

  ptok->szVal[1] = chSav;

  return text;
}

bool GitDiffReader::FReadCommit(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "commit"));

  // Start new diff.
  diffs_.emplace_back(std::move(FileVersionDiff{}));
  current_diff_ = &diffs_.back();

  if (!FGetTok(ptok))
    return false;
  current_diff_->commit_ = GetTextToEndOfLine(ptok);

  return true;
}

bool GitDiffReader::FReadTree(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "tree"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->tree_ = GetTextToEndOfLine(ptok);

  return true;
}

bool GitDiffReader::FReadParent(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "parent"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->parent_ = GetTextToEndOfLine(ptok);

  return true;
}

bool GitDiffReader::FReadAuthor(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "author"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->author_ = GetTextToEndOfLine(ptok);

  return true;
}

bool GitDiffReader::FReadCommitter(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "committer"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->committer_ = GetTextToEndOfLine(ptok);

  return true;
}

bool GitDiffReader::FReadDiff(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "diff"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->diff_command_ = GetTextToEndOfLine(ptok);

  return true;
}

bool GitDiffReader::FReadIndex(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "index"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->index_ = GetTextToEndOfLine(ptok);

  return true;
}

bool GitDiffReader::FReadComment(TOK* ptok) {
  if (ptok->tk != TK::tkWSPC)
    return false;

  if (!FGetTok(ptok))
    return false;
  current_diff_->comment_.append(GetTextToEndOfLine(ptok));

  return true;
}

// See https://git-scm.com/docs/diff-format
/*
An output line is formatted this way:
a colon.
mode for "src"; 000000 if creation or unmerged.
a space.
mode for "dst"; 000000 if deletion or unmerged.
a space.
sha1 for "src"; 0{40} if creation or unmerged.
a space.
sha1 for "dst"; 0{40} if creation, unmerged or "look at work tree".
a space.
status, followed by optional "score" number.
a tab or a NUL when -z option is used.
path for "src"
a tab or a NUL when -z option is used; only exists for C or R.
path for "dst"; only exists for C or R.
an LF or a NUL when -z option is used, to terminate the record.
Possible status letters are:
A: addition of a file
C: copy of a file into a new one
D: deletion of a file
M: modification of the contents or mode of a file
R: renaming of a file
T: change in the type of the file
U: file is unmerged (you must complete the merge before it can be committed)
X: "unknown" change type (most probably a bug, please report it)

Status letters C and R are always followed by a score (denoting the percentage
of similarity between the source and target of the move or copy). Status letter
M may be followed by a score (denoting the percentage of dissimilarity) for file
rewrites.

<sha1> is shown as all 0’s if a file is new on the filesystem and it is out of
sync with the index.
*/
bool GitDiffReader::FReadGitDiffTreeColon(TOK* ptok) {
  if (ptok->tk != TK::tkCOLON)
    return false;

  if (!FGetTok(ptok))
    return false;

  // e.g. "100644 100644 285ecec5de92e c90011667b640 M chrome/app/chrome_exe.rc"
  auto diff_tree_line = GetTextToEndOfLine(ptok);
  int num_args = sscanf_s(
      diff_tree_line.c_str(), "%o %o %s %s %c %s",
      &current_diff_->diff_tree_.old_mode,                             // %o
      &current_diff_->diff_tree_.new_mode,                             // %o
      current_diff_->diff_tree_.old_hash_string,                       // %s
      (unsigned)std::size(current_diff_->diff_tree_.old_hash_string),  // %s len
      current_diff_->diff_tree_.new_hash_string,                       // %s
      (unsigned)std::size(current_diff_->diff_tree_.new_hash_string),  // %s len
      &current_diff_->diff_tree_.action,                               // %c
      (unsigned)sizeof(current_diff_->diff_tree_.action),              // %c len
      current_diff_->diff_tree_.file_path,                             // %s
      (unsigned)std::size(current_diff_->diff_tree_.file_path)         // %s len
  );
  if (num_args != 6)
    return false;

  return true;
}

// https://en.wikipedia.org/wiki/Diff (Unified Format)
//@@ <from-file-range> <to-file-range> @@
//
// Add line:
// @@ -l,s +l,s @@ optional section heading
// E.g.:
//@@ -65,0 +66 @@ IDR_MAINFRAME           ICON "theme\google_chrome\chrome.ico"
//+IDR_MAINFRAME_2         ICON "theme\google_chrome\chrome2.ico"
//
// That means "removed 0 lines starting at line 65" + "added one line 66" <and
// trailing context line>
//
// Revert same change:
//@@ -66 +65,0 @@ IDR_MAINFRAME           ICON "theme\google_chrome\chrome.ico"
//-IDR_MAINFRAME_2 ICON "theme\google_chrome\chrome2.ico"
//
// That means "removed 1 lines starting at line 66" + "added zero lines at 65"
// <and trailing context line>
//
// N.b. There are (number of parents + 1) @ characters in the chunk header for
// combined diff format. E.g.:
//@@@ <from-file-range> <from-file-range> <to-file-range> @@@
bool GitDiffReader::FReadHunkHeader(TOK* ptok) {
  if (ptok->tk != TK::tkATSIGN)
    return false;

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkATSIGN)
    return false;

  if (!FGetTok(ptok))
    return false;

  FileVersionDiffHunk hunk;

  // Get removed lines.
  if (ptok->tk != TK::tkMINUS)
    return false;

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkINTEGER)
    return false;
  hunk.remove_location_ = atol(ptok->szVal);

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk == TK::tkCOMMA) {
    if (!FGetTok(ptok))
      return false;
    if (ptok->tk != TK::tkINTEGER)
      return false;
    hunk.remove_line_count_ = atol(ptok->szVal);

    if (!FGetTok(ptok))
      return false;
  } else {
    hunk.remove_line_count_ = 1;
  }

  // Get added lines.
  if (ptok->tk != TK::tkPLUS)
    return false;

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkINTEGER)
    return false;
  hunk.add_location_ = atol(ptok->szVal);

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk == TK::tkCOMMA) {
    if (!FGetTok(ptok))
      return false;
    if (ptok->tk != TK::tkINTEGER)
      return false;
    hunk.add_line_count_ = atol(ptok->szVal);

    if (!FGetTok(ptok))
      return false;
  } else {
    hunk.add_line_count_ = 1;
  }

  if (ptok->tk != TK::tkATSIGN)
    return false;
  if (!FGetTok(ptok))
    return false;

  if (ptok->tk != TK::tkATSIGN)
    return false;
  if (!FGetTok(ptok))
    return false;

  hunk.start_context_ = GetTextToEndOfLine(ptok);

  // Create new Hunk series.
  current_diff_->hunks_.push_back(hunk);

  return true;
}

bool GitDiffReader::FReadAddLine(TOK* ptok) {
  if (ptok->tk != TK::tkPLUS)
    return false;
  // N.b. ensure we get direct tokens here else we'll trim any leading
  // whitespace.
  if (!FGetTokDirect(ptok))
    return false;

  // If size() == 0, this is due to the '+++' or '---' line
  if (current_diff_->hunks_.size() > 0) {
    current_diff_->hunks_.back().add_lines_.push_back(GetTextToEndOfLine(ptok));
  } else {
    if (ptok->tk != TK::tkPLUS)
      return false;

    if (!FGetTok(ptok))
      return false;
    if (ptok->tk != TK::tkPLUS)
      return false;
  }

  return true;
}

bool GitDiffReader::FReadRemoveLine(TOK* ptok) {
  if (ptok->tk != TK::tkMINUS)
    return false;
  if (!FGetTokDirect(ptok))
    return false;

  // If size() == 0, this is due to the '+++' or '---' line
  if (current_diff_->hunks_.size() > 0) {
    current_diff_->hunks_.back().remove_lines_.push_back(
        GetTextToEndOfLine(ptok));
  } else {
    if (ptok->tk != TK::tkMINUS)
      return false;

    if (!FGetTok(ptok))
      return false;
    if (ptok->tk != TK::tkMINUS)
      return false;
  }

  return true;
}

bool GitDiffReader::FParseNamedLine(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;

  if (!strcmp(ptok->szVal, "commit")) {
    return FReadCommit(ptok);
  } else if (!strcmp(ptok->szVal, "tree")) {
    return FReadTree(ptok);
  } else if (!strcmp(ptok->szVal, "parent")) {
    return FReadParent(ptok);
  } else if (!strcmp(ptok->szVal, "author")) {
    return FReadAuthor(ptok);
  } else if (!strcmp(ptok->szVal, "committer")) {
    return FReadCommitter(ptok);
  } else if (!strcmp(ptok->szVal, "diff")) {
    return FReadDiff(ptok);
  } else if (!strcmp(ptok->szVal, "index")) {
    return FReadIndex(ptok);
  }

  return true;
}
bool GitDiffReader::ProcessLogLine(char* line) {
  bool is_done = false;
  TOK tok;
  AttachTokToLine(&tok, line);
  if (!FGetTokDirect(&tok)) {
    is_done = true;
    goto LDone;
  }

  switch (tok.tk) {
    case TK::tkWORD: {
      is_done = !FParseNamedLine(&tok);
      break;
    }
    case TK::tkCOLON: {
      is_done = !FReadGitDiffTreeColon(&tok);
      break;
    }
    case TK::tkWSPC: {
      is_done = !FReadComment(&tok);
      break;
    }
    case TK::tkATSIGN: {
      is_done = !FReadHunkHeader(&tok);
      break;
    }
    case TK::tkPLUS: {
      // One of:
      // "+++ b/chrome/app/chrome_exe.rc"
      // or
      // "+Added lines"
      is_done = !FReadAddLine(&tok);
      break;
    }
    case TK::tkMINUS: {
      // One of:
      // "--- a/chrome/app/chrome_exe.rc"
      // or
      // "-Removed lines"
      is_done = !FReadRemoveLine(&tok);
      break;
    }
  }

LDone:
  UnattachTok(&tok);
  return !is_done;
}

GitDiffReader::GitDiffReader(const std::filesystem::path& file_path)
    : current_diff_(nullptr) {
  std::wstring command =
      to_wstring(kGitDiffCommand) + std::wstring(file_path.filename());
  ProcessPipe process_pipe(command.c_str(), file_path.parent_path().c_str());

  FILE* stream = process_pipe.GetStandardOutput();

  // Check if it is in the cache.
  fpos_t pos;
  if (fgetpos(stream, &pos))
    return;
  char header_line[1024];
  if (!fgets(header_line, (int)std::size(header_line), stream))
    return;
  if (fsetpos(stream, &pos))
    return;

  file_stream_cache_ = std::make_unique<GitFileStreamCache>(file_path);

  // Format: "commit <sha1>".
  auto sha1 = strrchr(header_line, ' ');
  if (!sha1)
    return;
  // Skip space.
  sha1++;
  auto len = strlen(sha1);
  if (sha1[len - 1] == '\n')
    sha1[len - 1] = '\0';
  auto cache_stream = file_stream_cache_->GetStream(sha1);

  if (cache_stream.get()) {
    if (cache_stream)
      ProcessDiffLines(cache_stream.get());
  } else {
    auto file_stream = file_stream_cache_->SaveStream(stream, sha1);
    if (file_stream)
      ProcessDiffLines(file_stream.get());
  }
}

GitDiffReader::~GitDiffReader() {}

void GitDiffReader::ProcessDiffLines(FILE* stream) {
  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    if (!ProcessLogLine(stream_line))
      break;
  }
}

const std::vector<FileVersionDiff>& GitDiffReader::GetDiffs() {
  return diffs_;
}

std::vector<FileVersionDiff> GitDiffReader::MoveDiffs() {
  return std::move(diffs_);
}
