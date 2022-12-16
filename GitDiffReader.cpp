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
#include "OutputWnd.h"
#include "Utility.h"

constexpr TCHAR kGitMostRecentCommitForFileCommand[] =
    _T("git --no-pager log %S -n 1 -- %s");
constexpr TCHAR kGitDiffCommand[] =
    _T("git --no-pager log %S -p -U0 --raw --no-color --first-parent ")
    _T("--pretty=raw -- %s");  // FUTURE: Consider adding '--reverse' instead of
                               // calling 'std::reverse()' after processing the
                               // diffs (not currently doing so since
                               // std::reverse() is fast and it may take longer
                               // for git to reverse the diffs).
constexpr TCHAR kGitLogNameTagCommandFromStdIn[] =
    _T("git --no-pager name-rev --annotate-stdin");

static std::string GetTextToWhitespace(TOK* ptok) {
  std::string text;
  char* szStart = ptok->szVal;

  while (ptok->tk != TK::tkWSPC && ptok->tk != TK::tkNEWLINE &&
         ptok->tk != TK::tkNil) {
    if (!FGetTokDirect(ptok))
      break;
  }

  char chSav = ptok->szVal[0];
  ptok->szVal[0] = L'\0';

  text = szStart;

  ptok->szVal[0] = chSav;

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
  if (!FGetTok(ptok))
    return false;
  strcpy_s(hash.sha_, GetTextToWhitespace(ptok).c_str());

  if (!FGetTok(ptok))
    return false;
  hash.tag_ = GetTextToWhitespace(ptok);

  return true;
}

bool GitDiffReader::FReadCommit(TOK* ptok) {
  // Format: "commit <sha1> (<tag>)".
  if (ptok->tk != TK::tkWORD)
    return false;
  assert(!strcmp(ptok->szVal, "commit"));

  // Start new diff.  REVIEW: is there a better way to determine diff edges?
  diffs_.push_back({});
  current_diff_ = &diffs_.back();

  current_diff_->path_ = path_;

  return FReadGitHash(ptok, current_diff_->commit_);
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

  current_diff_->parents_.push_back({});
  return FReadGitHash(ptok, current_diff_->parents_.back().commit_);
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
  current_diff_->hunks_.push_back(std::move(hunk));

  return true;
}

bool GitDiffReader::FReadAddLine(TOK* ptok) {
  if (ptok->tk != TK::tkPLUS)
    return false;
  // N.b. ensure we get direct tokens here else we'll trim any leading
  // whitespace.
  if (!FGetTokDirect(ptok))
    return false;

  // If size() == 0, this is due to the '+++' line.
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

  // If size() == 0, this is due to the '---' line.
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

GitDiffReader::GitDiffReader(const std::filesystem::path& file_path,
                             const std::string& tag,
                             COutputWnd* pwndOutput)
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

  file_stream_cache_ = std::make_unique<GitFileStreamCache>(file_path);

  auto cache_stream = file_stream_cache_->GetStream(file_cache_revision);

  if (cache_stream) {
    if (pwndOutput != nullptr) {
      CString message;
      message.FormatMessage(
          _T("   Cache hit for revision '%1!S!', reading diff history from: ")
          _T("'%2!s!'"),
          file_cache_revision.c_str(),
          file_stream_cache_->GetItemCachePath(file_cache_revision)
              .c_str());
      pwndOutput->AppendDebugTabMessage(message);
    }
    ProcessDiffLines(cache_stream.get());
  } else {
    wsprintf(command, kGitDiffCommand, tag_no_parentheses.c_str(),
             file_path.filename().c_str());
    if (pwndOutput != nullptr) {
      CString message;
      message.FormatMessage(
          _T("   Cache miss for revision '%1!S!', reading full diff history: ")
          _T("'%2!s!'"),
          file_cache_revision.c_str(), command);
      pwndOutput->AppendDebugTabMessage(message);
    }
    ProcessPipe process_pipe_git_log(command, file_path.parent_path().c_str());

    if (pwndOutput != nullptr) {
      CString message;
      message.FormatMessage(_T("   Processing tags from last command: '%1!s!'"),
                            kGitLogNameTagCommandFromStdIn);
      pwndOutput->AppendDebugTabMessage(message);
    }
    ProcessPipe process_pipe_git_tag(kGitLogNameTagCommandFromStdIn,
                                     file_path.parent_path().c_str(),
                                     process_pipe_git_log.GetStandardOutput());

    auto file_stream = file_stream_cache_->SaveStream(
        process_pipe_git_tag.GetStandardOutput(), file_cache_revision);
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
