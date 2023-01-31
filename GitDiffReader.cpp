#include "pch.h"

#include "GitDiffReader.h"

#include <Windows.h>

#include <afxstr.h>
#include <afxwin.h>
#include <tchar.h>
#include <cassert>

#include "GitFileStreamCache.h"
#include "GitHash.h"
#include "GitRevFilter.h"
#include "LineTokenizer.h"
#include "OutputWnd.h"
#include "Utility.h"

#define ADD_TAGS 1

constexpr TCHAR kGitMostRecentCommitForFileCommand[] =
    _T("git --no-pager log %S -n 1 -- %s");
// N.b. See GitDiffReader::GetGitCommand() for what was |kGitDiffCommand|.

static std::string GetTextToWhitespace(TOK* ptok) {
  std::string text;
  char* szStart = ptok->szVal;

  while (ptok->tk != TK::tkWSPC && ptok->tk != TK::tkTAB &&
         ptok->tk != TK::tkNEWLINE && ptok->tk != TK::tkNil) {
    if (!FGetTokDirect(ptok))
      break;
  }

  char chSav = ptok->szVal[0];
  ptok->szVal[0] = L'\0';

  text = szStart;

  ptok->szVal[0] = chSav;

  if (ptok->tk == TK::tkWSPC || ptok->tk == TK::tkTAB) {
    // Get next whilespace token.
#if _DEBUG
    bool ret =
#endif
        FGetTok(ptok);
    assert(ret);
  }

  return text;
}

static std::string GetTextToToken(TOK* ptok, TK tk, int cch_tok_to_include) {
  std::string text;
  char* szStart = ptok->szVal;

  while (ptok->tk != tk && ptok->tk != TK::tkNil) {
    if (!FGetTok(ptok))
      break;
  }

  char chSav = ptok->szVal[cch_tok_to_include];
  ptok->szVal[cch_tok_to_include] = L'\0';

  text = szStart;

  ptok->szVal[cch_tok_to_include] = chSav;

  return text;
}

static std::string GetTextToEndOfLine(TOK* ptok) {
  // Keep any ending '\n' by setting |cch_tok_to_include| to 1 (if there
  // is no |tk| then we still simply just add another '\0').
  return GetTextToToken(ptok, TK::tkNEWLINE, 1 /*cch_tok_to_include*/);
}

bool GitDiffReader::FReadGitHash(TOK* ptok, GitHash& hash) {
  if (ptok->tk == TK::tkNil || ptok->tk == TK::tkNEWLINE)
    return false;
  strcpy_s(hash.sha_, GetTextToWhitespace(ptok).c_str());

  // Read optional tag.
  // REVIEW: Consider doing this more cleanly, looking
  // for a tag with balenced parentheses, i.e. "(<tag>)".
  if (ptok->tk == TK::tkLPAREN) {
    hash.tag_ = GetTextToWhitespace(ptok);
  }

  return true;
}

bool GitDiffReader::FReadCommit(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "commit"));

  FileVersionDiff diff;

  if (!FGetTok(ptok)) {
    assert(false);
    return false;
  }

  // Format: "commit <sha1> (<tag>)".
  if (!FReadGitHash(ptok, diff.commit_)) {
    assert(false);
    return false;
  }

  if (ptok->tk != TK::tkNEWLINE) {
    // Read any children.
    // Format: "[commit <sha1> (<tag>)]*".
    GitHash child_hash;
    while (FReadGitHash(ptok, child_hash)) {
      FileVersionDiffChild child;
      child.commit_ = std::move(child_hash);
      diff.children_.push_back(std::move(child));
    }
  }

  if (ptok->tk != TK::tkNEWLINE) {
    assert(false);
    return false;
  }

  // Start new diff.  REVIEW: is there a better way to determine diff edges?
  diff.path_ = path_;
  diffs_.push_back(std::move(diff));

  current_diff_ = &diffs_.back();

  return true;
}

bool GitDiffReader::FReadTree(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "tree"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->tree_ = GetTextToWhitespace(ptok);

  if (ptok->tk != TK::tkNEWLINE) {
    assert(false);
    return false;
  }

  return true;
}

bool GitDiffReader::FReadParent(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "parent"));

  if (!FGetTok(ptok)) {
    assert(false);
    return false;
  }

  FileVersionDiffParent parent;
  if (!FReadGitHash(ptok, parent.commit_)) {
    assert(false);
    return false;
  }

  if (ptok->tk != TK::tkNEWLINE) {
    assert(false);
    return false;
  }

  current_diff_->parents_.push_back(std::move(parent));

  return true;
}

bool GitDiffReader::FReadNameEmailAndTime(TOK* ptok,
                                          NameEmailTime& name_email_time) {
  // name <email> time [+|-]dddd
  name_email_time.name_ = GetTextToToken(ptok, TK::tkLESSTHAN, 0);
  if (name_email_time.name_.size() > 0 &&
      name_email_time.name_[name_email_time.name_.size() - 1] == ' ') {
    // Trim any trailing space.
    name_email_time.name_ =
        name_email_time.name_.substr(0, name_email_time.name_.size() - 1);
  }

  // skip TK::tkLESSTHAN
  if (!FGetTok(ptok))
    return false;
  name_email_time.email_ = GetTextToToken(ptok, TK::tkGREATERTHAN, 0);

  // skip TK::tkGREATERTHAN
  if (!FGetTok(ptok))
    return false;
  auto tm_string = GetTextToEndOfLine(ptok);
  auto time_val = static_cast<time_t>(_atoi64(tm_string.c_str()));
  gmtime_s(&name_email_time.time_, &time_val);

  return ptok->tk != TK::tkNil;
}

bool GitDiffReader::FReadAuthor(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "author"));

  if (!FGetTok(ptok))
    return false;

  return FReadNameEmailAndTime(ptok, current_diff_->author_);
}

bool GitDiffReader::FReadCommitter(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "committer"));

  if (!FGetTok(ptok))
    return false;

  return FReadNameEmailAndTime(ptok, current_diff_->committer_);
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


For merges we get https://git-scm.com/docs/diff-format#_diff_format_for_merges

- there is a colon for each parent
- there are more "src" modes and "src" sha1
- status is concatenated status characters for each parent
- no optional "score" number
- tab-separated pathname(s) of the file [if '--combined-all-paths' is set, there
can be more that one file]

E.g.:
   ::100644 100644 100644 bbf2455 6617bf6 c0a81df MM FileVersionInstance.cpp
  srcmode1 srcmode2 dstmode srchash1 srchash2 dsthash S1S2 dest path
*/

bool GitDiffReader::FReadGitDiffTreeColon(TOK* ptok) {
  if (ptok->tk != TK::tkCOLON)
    return false;

  if (!FGetTok(ptok)) {
    assert(false);
    return false;
  }

  int num_parents = 1;
  while (ptok->tk == TK::tkCOLON) {
    num_parents++;
    if (!FGetTok(ptok)) {
      assert(false);
      return false;
    }
  }

  // Read src mod(es).
  for (int i = 0; i < num_parents; i++) {
    FileVersionDiffTreeFile tree_file;
    if (!FReadGitDiffTreeColonFileMode(ptok, tree_file.mode)) {
      assert(false);
      return false;
    }

    current_diff_->diff_tree_.old_files.push_back(tree_file);
  }

  // Read dst mode.
  if (!FReadGitDiffTreeColonFileMode(ptok,
                                     current_diff_->diff_tree_.new_file.mode)) {
    assert(false);
    return false;
  }

  // Read src hash(es).
  for (int i = 0; i < num_parents; i++) {
    const auto src_hash = GetTextToWhitespace(ptok);
    if (src_hash.empty()) {
      assert(false);
      return false;
    }
    strcpy(current_diff_->diff_tree_.old_files[i].hash_string,
           src_hash.c_str());
  }

  // Read dst hash.
  const auto dst_hash = GetTextToWhitespace(ptok);
  if (dst_hash.empty()) {
    assert(false);
    return false;
  }
  strcpy(current_diff_->diff_tree_.new_file.hash_string, dst_hash.c_str());

  // Read actions.
  if (ptok->tk != TK::tkWORD) {
    assert(false);
    return false;
  }

  for (int i = 0; i < num_parents; i++) {
    const auto action = ptok->szVal[i];
    if (!action) {
      assert(false);
      return false;
    }
    assert(action == 'A' || action == 'C' || action == 'D' || action == 'M' ||
           action == 'R' || action == 'T' ||
           action == 'U');  // N.B. Disallow 'X' since this is a bug.

    const char action_string[2] = {action};
    strcpy(current_diff_->diff_tree_.old_files[i].action, action_string);
  }

  if (!FGetTok(ptok)) {
    assert(false);
    return false;
  }

  // Read src file name(s).
  for (int i = 0; i < num_parents; i++) {
    const auto src_file_name = GetTextToWhitespace(ptok);
    assert(!src_file_name.empty());  // REVIEW: will this be legitimately empty
                                     // on
                                     // file delete?
    strcpy(current_diff_->diff_tree_.old_files[i].file_path,
           src_file_name.c_str());
  }

  if (ptok->tk != TK::tkNEWLINE) {
    // Read optional dest file name (e.g. if a rename).
    const auto dst_file_name = GetTextToWhitespace(ptok);
    assert(!dst_file_name.empty());
    strcpy(current_diff_->diff_tree_.new_file.file_path, dst_file_name.c_str());
  }

  if (ptok->tk != TK::tkNEWLINE) {
    assert(false);
    return false;
  }

  // Handle rename.
  for (int i = 0; i < num_parents; i++) {
    if (current_diff_->diff_tree_.old_files[i].action[0] == 'R') {
      std::string new_path_string = path_.generic_string();
      auto path_base_pos = new_path_string.rfind(
          current_diff_->diff_tree_.old_files[i].file_path);
      // This can not match if we're reading unfiltered (i.e. not per-file) diff
      // entires.
      if (path_base_pos != new_path_string.npos) {
        new_path_string.resize(path_base_pos);
        new_path_string.append(current_diff_->diff_tree_.new_file.file_path);
        path_ = new_path_string;
        current_diff_->path_ = new_path_string;
        break;
      }
    }
  }

  return true;
}
// Reads octal file mode.
bool GitDiffReader::FReadGitDiffTreeColonFileMode(TOK* ptok, int& file_mode) {
  if (ptok->tk != TK::tkINTEGER)
    return false;

  char* end_ptr = nullptr;
  constexpr int octal_radix = 8;
  file_mode = strtoul(ptok->szVal, &end_ptr, octal_radix);

  if (!FGetTok(ptok))
    return false;

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

bool GitDiffReader::FReadBinary(TOK* ptok) {
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "Binary"));

  if (!FGetTok(ptok))
    return false;
  current_diff_->is_binary_ = true;

  return true;
}

// https://en.wikipedia.org/wiki/Diff (Unified Format)
//@@ <from-file-range> <to-file-range> @@
//
// Add line:
// @@ -l,s +l,s @@ optional section heading
// N.b. If the line count (",s" above) is missing, it is assumed to be 1.
//
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

// @@@ <from-file-range>+ <to-file-range> @@@

#if !OLD_DIFFS

bool GitDiffReader::FReadHunkDiffLine(TOK* ptok) {
  auto line = GetTextToEndOfLine(ptok);
  current_diff_->hunks_.back().lines_.push_back(std::move(line));

  return true;
}

bool GitDiffReader::ProcessLogDiffLine(char* line) {
  bool is_done = false;
  TOK tok;
  AttachTokToLine(&tok, line);
  if (!FGetTokDirect(&tok)) {
    is_done = true;
    goto LDone;
  }

  switch (tok.tk) {
    case TK::tkATSIGN: {
      is_done = !FReadCombinedHunkHeader(&tok);
      break;
    }
    case TK::tkBSLASH: {
      auto line = GetTextToEndOfLine(&tok);
      if (line == "\\ No newline at end of file\n") {
        auto& last_line = current_diff_->hunks_.back().lines_.back();
        if (last_line.back() == '\n') {
          last_line.pop_back();
          assert(last_line.back() != '\n');
        }
      }
      break;
    }

    default: {
      is_done = !FReadHunkDiffLine(&tok);
      break;
    }
  }

LDone:
  UnattachTok(&tok);
  return !is_done;
}

bool GitDiffReader::FReadHunkLines(TOK* ptok) {
  // Diff block in ended by EOF or empty line

  if (!FReadCombinedHunkHeader(ptok))
    return false;

  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream_)) {
    if (!stream_line[0] || stream_line[0] == '\n')
      return true;
    if (!ProcessLogDiffLine(stream_line))
      break;
  }

  return false;  // EOF
}

bool GitDiffReader::FReadCombinedHunkHeaderRange(
    TOK* ptok,
    FileVersionDiffHunkRange& range) {
  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkINTEGER)
    return false;
  range.location_ = atol(ptok->szVal);

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk == TK::tkCOMMA) {
    if (!FGetTok(ptok))
      return false;
    if (ptok->tk != TK::tkINTEGER)
      return false;
    range.count_ = atol(ptok->szVal);

    if (!FGetTok(ptok))
      return false;
  } else {
    range.count_ = 1;
  }

  return true;
}

// @ [@ <from-file-range>]+ <to-file-range> @[@]+
bool GitDiffReader::FReadCombinedHunkHeader(TOK* ptok) {
  if (ptok->tk != TK::tkATSIGN)
    return false;

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkATSIGN)
    return false;

  FileVersionCombinedDiffHunk hunk;

  int num_parents = 1;
  while (FGetTok(ptok)) {
    if (ptok->tk != TK::tkATSIGN)
      break;
    num_parents++;
  }

  // Read "from" range(s).
  for (int i = 0; i < num_parents; i++) {
    if (ptok->tk != TK::tkMINUS)
      return false;
    FileVersionDiffHunkRange from_range;
    if (!FReadCombinedHunkHeaderRange(ptok, from_range))
      return false;

    hunk.ranges_.push_back(from_range);
  }

  // Read "to" range.
  if (ptok->tk != TK::tkPLUS)
    return false;
  FileVersionDiffHunkRange to_range;
  if (!FReadCombinedHunkHeaderRange(ptok, to_range))
    return false;

  hunk.ranges_.push_back(to_range);

  // Ensure ending '@@...' match the starting number.
  for (int i = 0; i < num_parents + 1; i++) {
    if (ptok->tk != TK::tkATSIGN)
      return false;
    if (!FGetTok(ptok))
      return false;
  }

  hunk.start_context_ = GetTextToEndOfLine(ptok);

  current_diff_->hunks_.push_back(std::move(hunk));

  return true;
}

#else
bool GitDiffReader::FReadHunkHeader(TOK* ptok) {
  if (ptok->tk != TK::tkATSIGN)
    return false;

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkATSIGN)
    return false;

  if (!FGetTok(ptok))
    return false;

  FileVersionDiffHunk remove_hunk;
  FileVersionDiffHunk add_hunk;

  // Get removal lines.
  if (ptok->tk != TK::tkMINUS)
    return false;

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkINTEGER)
    return false;
  remove_hunk.from_location_ = atol(ptok->szVal);

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk == TK::tkCOMMA) {
    if (!FGetTok(ptok))
      return false;
    if (ptok->tk != TK::tkINTEGER)
      return false;
    remove_hunk.from_line_count_ = atol(ptok->szVal);

    if (!FGetTok(ptok))
      return false;
  } else {
    remove_hunk.from_line_count_ = 1;
  }

  // Get addition lines.
  if (ptok->tk != TK::tkPLUS)
    return false;

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk != TK::tkINTEGER)
    return false;
  add_hunk.to_location_ = atol(ptok->szVal);

  if (!FGetTok(ptok))
    return false;
  if (ptok->tk == TK::tkCOMMA) {
    if (!FGetTok(ptok))
      return false;
    if (ptok->tk != TK::tkINTEGER)
      return false;
    add_hunk.to_line_count_ = atol(ptok->szVal);

    if (!FGetTok(ptok))
      return false;
  } else {
    add_hunk.to_line_count_ = 1;
  }

  if (ptok->tk != TK::tkATSIGN)
    return false;
  if (!FGetTok(ptok))
    return false;

  if (ptok->tk != TK::tkATSIGN)
    return false;
  if (!FGetTok(ptok))
    return false;

  remove_hunk.start_context_ = GetTextToEndOfLine(ptok);

  // Create new Hunk series.
  current_diff_->remove_hunks_.push_back(std::move(remove_hunk));
  current_diff_->parents_[0].add_hunks_.push_back(std::move(add_hunk));

  return true;
}
#endif

#if OLD_DIFFS
bool GitDiffReader::FReadAddLine(TOK* ptok) {
  if (ptok->tk != TK::tkPLUS)
    return false;
  // N.b. ensure we get direct tokens here else we'll trim any leading
  // whitespace.
  if (!FGetTokDirect(ptok))
    return false;

  // If size() == 0, this is due to the '+++' line.
  if (current_diff_->parents_[0].add_hunks_.size() > 0) {
    current_diff_->parents_[0].add_hunks_.back().to_lines_.push_back(
        GetTextToEndOfLine(ptok));
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
#endif  // OLD_DIFFS

#if OLD_DIFFS
bool GitDiffReader::FReadRemoveLine(TOK* ptok) {
  if (ptok->tk != TK::tkMINUS)
    return false;
  if (!FGetTokDirect(ptok))
    return false;

  // If size() == 0, this is due to the '---' line.
  if (current_diff_->remove_hunks_.size() > 0) {
    current_diff_->remove_hunks_.back().from_lines_.push_back(
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
#endif  // OLD_DIFFS

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
    // TODO- change this into "FReadDiffSection()" and then sub-parse "new file
    // mode", "index", "Binary files" etc.
    return FReadDiff(ptok);
  } else if (!strcmp(ptok->szVal, "index")) {
    return FReadIndex(ptok);
  } else if (!strcmp(ptok->szVal, "Binary")) {
    return FReadBinary(ptok);
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
#if OLD_DIFFS
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
#else
    case TK::tkATSIGN: {
      is_done = !FReadHunkLines(&tok);
      break;
    }
#endif  // OLD_DIFFS
#if NEEDTHIS
    case TK::tkBSLASH: {
      // "\ No newline at end of file"
      // According to https://github.com/git/git/blob/master/diff.c this is the
      // only use of a leading backspace.
      is_done = !FReadControlLine(&tok);
      break;
    }
#endif
  }

LDone:
  UnattachTok(&tok);
  return !is_done;
}

// static
std::wstring GitDiffReader::GetGitCommand(
    const std::filesystem::path& file_path,
    const std::string& revision,
    const Opts& opts) {
  std::wstring opt_str;
  assert(file_path.is_absolute());
  const auto git_root = std::filesystem::canonical(GetGitRoot(file_path));
  const auto git_relative_path =
      std::filesystem::canonical(file_path).lexically_relative(git_root);
  std::wstring git_relative_path_forward_slash = git_relative_path;
  // Turn the relative path into forward slashes; those are still accepted by
  // Git and then can be made part of the cache file name.
  for (auto& ch : git_relative_path_forward_slash) {
    if (ch == '\\')
      ch = '/';
  }
  if (!opts.HasOpts(Opt::NO_FIRST_PARENT))
    // <-- TODO: Check the effects of `--diff-merges=combined` (it removes
    // extra diff on root of add merges -- I think it then means that a
    // commits parent's must be diff merged first to correctly express the
    // state of the merged branch). Also we might need to use
    // `--diff-merges=dense-combined` which supposedly only show *merge
    // conflicts* in the resulting merge commit.
    //
    // N.b. We're using short args to save on cache file path length. Here are
    // the longer equivalents.
    //      '-c'  --> '--diff-merges=combined'
    //      '-U1' --> '--unified=1' (implies '--patch' aka '-p')
    opt_str += L"--first-parent -c --combined-all-paths ";
  // This must go last.
  if (!opts.HasOpts(Opt::NO_FILTER_TO_FILE)) {
    if (opts.HasOpts(Opt::FOLLOW))
      opt_str += L"--follow ";
    opt_str += std::wstring(L"-- ") + git_relative_path_forward_slash;
  }

  std::wstring command = std::wstring(L"git --no-pager log ") +
                         to_wstring(revision.empty() ? "" : revision + " ") +
                         L"-U1 --raw --format=raw --no-color --children " +
                         opt_str;

  return command;
}

GitDiffReader::GitDiffReader(const std::filesystem::path& file_path,
                             const std::string& tag,
                             COutputWnd* pwndOutput,
                             const Opts& opts)
    : current_diff_(nullptr), path_(file_path) {
  TCHAR command[1024];

  CWaitCursor wait;

  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("Reading git diff history for: '%1!s!' %2!S!:"),
                          file_path.c_str(), tag.c_str());
    pwndOutput->AppendDebugTabMessage(message);
  }

  std::string tag_no_parentheses = tag;
  if (!tag_no_parentheses.empty()) {
    if (tag_no_parentheses.front() == '(')
      tag_no_parentheses = tag_no_parentheses.substr(1);
    if (!tag_no_parentheses.empty() && tag_no_parentheses.back() == ')')
      tag_no_parentheses =
          tag_no_parentheses.substr(0, tag_no_parentheses.size() - 1);
  }

  std::string file_cache_revision;
  if (tag_no_parentheses.empty()) {
    wsprintf(command, kGitMostRecentCommitForFileCommand,
             tag_no_parentheses.c_str(), file_path.filename().c_str());
    if (pwndOutput != nullptr) {
      CString message;
      message.FormatMessage(_T("   Checking current hash: '%1!s!'"), command);
      pwndOutput->AppendDebugTabMessage(message);
    }
    ProcessPipe process_pipe_git_commit(command,
                                        file_path.parent_path().c_str());

    FILE* stream = process_pipe_git_commit.GetStandardOutput();
    if (stream == nullptr) {
      if (pwndOutput != nullptr) {
        CString message;
        message.FormatMessage(_T("   Error getting current git hash."));
        pwndOutput->AppendDebugTabMessage(message);
      }
      return;
    }

    char header_line[1024];
    if (!fgets(header_line, (int)std::size(header_line), stream)) {
      if (pwndOutput != nullptr) {
        CString message;
        message.FormatMessage(_T("   Error reading hash command output."));
        pwndOutput->AppendDebugTabMessage(message);
      }
      return;
    }

    // Sniff header of stream to see if we have commit cached.
    ProcessLogLine(header_line);
    if (!current_diff_ || !current_diff_->commit_.IsValid()) {
      if (pwndOutput != nullptr) {
        CString message;
        message.FormatMessage(
            _T("   Cache header line is missing or invalid."));
        pwndOutput->AppendDebugTabMessage(message);
      }
    } else {
      file_cache_revision = current_diff_->commit_.sha_;
    }
  } else {
    file_cache_revision = tag_no_parentheses;
  }

  file_stream_cache_ = std::make_unique<GitFileStreamCache>();

  const auto wcommand = GetGitCommand(file_path, tag_no_parentheses, opts);
  auto cache_stream = file_stream_cache_->GetStream(wcommand);

  if (cache_stream) {
    if (pwndOutput != nullptr) {
      CString message;
      message.FormatMessage(
          _T("   Cache hit for revision '%1!S!', reading diff history from: ")
          _T("'%2!s!'"),
          file_cache_revision.c_str(),
          file_stream_cache_->GetItemCachePath(wcommand).c_str());
      pwndOutput->AppendDebugTabMessage(message);
    }
    ProcessDiffLines(cache_stream.get());
  } else {
    const auto wcommand = GetGitCommand(file_path, tag_no_parentheses, opts);
    wcscpy_s(command, wcommand.c_str());

    if (pwndOutput != nullptr) {
      CString message;
      message.FormatMessage(
          _T("   Cache miss for revision '%1!S!', reading full diff ")
          _T("history: ")
          _T("'%2!s!'"),
          file_cache_revision.c_str(), command);
      pwndOutput->AppendDebugTabMessage(message);
    }
    ProcessPipe process_pipe_git_log(command, file_path.parent_path().c_str());

#if ADD_TAGS
#if 0 
    ProcessPipe process_pipe_git_tag(kGitLogNameTagCommandFromStdIn,
                                     file_path.parent_path().c_str(),
                                     process_pipe_git_log.GetStandardOutput());

    auto file_stream = file_stream_cache_->SaveStream(
        process_pipe_git_tag.GetStandardOutput(), wcommand);
#else
    GitRevFilter git_rev_filter(file_path.parent_path().wstring().c_str(),
                                process_pipe_git_log.GetStandardOutput(),
                                pwndOutput);
    auto file_stream = file_stream_cache_->SaveStream(
        git_rev_filter.GetFilteredStream(), wcommand);
#endif
#else
    auto file_stream = file_stream_cache_->SaveStream(
        process_pipe_git_log.GetStandardOutput(), wcommand);
#endif
    if (file_stream) {
      ProcessDiffLines(file_stream.get());
    }
  }

  // Fix up diffs_:
  // 1. Reverse such that oldest diffs come first.
  std::reverse(diffs_.begin(), diffs_.end());
  // 2. Generate file_parent_commit_.
  GitHash file_parent_commit;
  for (auto& diff : diffs_) {
    diff.file_parent_commit_ = file_parent_commit;
    file_parent_commit = diff.commit_;
    assert(file_parent_commit.IsValid());
  }

  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("   Done."));
    pwndOutput->AppendDebugTabMessage(message);
  }
}

GitDiffReader::~GitDiffReader() {}

void GitDiffReader::ProcessDiffLines(FILE* stream) {
  assert(!current_diff_ ^ diffs_.size());
  diffs_.clear();
  current_diff_ = nullptr;
  stream_ = stream;

  char stream_line[1024];
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    if (!ProcessLogLine(stream_line))
      break;
  }
}

const std::vector<FileVersionDiff>& GitDiffReader::GetDiffs() const {
  return diffs_;
}

std::vector<FileVersionDiff> GitDiffReader::MoveDiffs() {
  return std::move(diffs_);
}
