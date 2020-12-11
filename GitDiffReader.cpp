#include "pch.h"
#include "GitDiffReader.h"

#include "LineTokenizer.h"
#include <map>
#include <stdio.h>
#include <string>
#include <functional>

constexpr char kGitDiffCommand[] = "git --no-pager log -p -U0 --raw --no-color --pretty=raw -- ";

enum class Directive{ tkCommit, tkTree, tkParent, tkAuthor, tkCommitter, tkColonEndCommitter, tkDiff, tkIndex, tk3Minus, tk3Plus, tk2At, tkPlus,  tkEnd };

static bool FGetTextToEndOfLine(TOK* ptok,
                                _Out_ char* szTextRet) {
  bool fRet = TRUE;
  char* szStart = ptok->pchEnd;

  while (ptok->tk != TK::tkNEWLINE && ptok->tk != TK::tkNil) {
    fRet = FGetTok(ptok);
    if (!fRet)
      goto LDone;
  }

  if (szTextRet) {
    char* szT = NULL;
    char chSav = ptok->szVal[0];
    ptok->szVal[0] = L'\0';

    if (szStart) {
      strcpy_s(szTextRet, 512, szStart);
    }

    ptok->szVal[0] = chSav;
  }

LDone:
  return fRet;
}

static bool FReadCommit(TOK* ptok) {
  return true;
}

static bool FReadTree(TOK* ptok) {
  return true;
}

static bool FReadParent(TOK* ptok) {
  return true;
}

static bool FReadAuthor(TOK* ptok) {
  return true;
}

static bool FReadCommitter(TOK* ptok) {
  return true;
}

static bool FReadDiff(TOK* ptok) {
  return true;
}

static bool FReadIndex(TOK* ptok) {
  return true;
}

static bool FReadComment(TOK* ptok) {
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
static bool FReadGitDiffTreeColon(TOK* ptok) {
  char szTemp[512];
  szTemp[0] = 0;
  if (!FGetTextToEndOfLine(ptok, szTemp))
    return false;

  return true;
}

// Example:
//@@ <from-file-range> <to-file-range> @@
//
// Add line:
//@@ -65,0 +66 @@ IDR_MAINFRAME           ICON                    "theme\google_chrome\chrome.ico"
//+IDR_MAINFRAME_2         ICON                    "theme\google_chrome\chrome2.ico"
//
// That means "removed 0 lines starting at line 65" + "added one line 66" <and
// trailing context line>
//
// Revert same change:
//@@ -66 +65,0 @@ IDR_MAINFRAME           ICON "theme\google_chrome\chrome.ico"
//-IDR_MAINFRAME_2 ICON "theme\google_chrome\chrome2.ico"
//
// That means "removed 1 lines starting at line 66" + "added zero lines at 65" <and
// trailing context line>
//
// N.b. There are (number of parents + 1) @ characters in the chunk header for
// combined diff format. E.g.:
//@@@ <from-file-range> <from-file-range> <to-file-range> @@@
static bool FReadDiffHeader(TOK* ptok) {
  char szTemp[512];
  szTemp[0] = 0;
  if (!FGetTextToEndOfLine(ptok, szTemp))
    return false;

  return true;
}

static bool FReadAddLine(TOK* ptok) {
  char szTemp[512];
  szTemp[0] = 0;
  if (!FGetTextToEndOfLine(ptok, szTemp))
    return false;

  return true;
}

static bool FReadRemoveLine(TOK* ptok) {
  char szTemp[512];
  szTemp[0] = 0;
  if (!FGetTextToEndOfLine(ptok, szTemp))
    return false;

  return true;
}

bool CGitDiffReader::ProcessLogLine(char* line) {
  bool is_done = false;
  TOK tok;
  AttachTokToLine(&tok, line);
  if (!FGetTokDirect(&tok)) {
    is_done = true;
    goto LDone;
  }

  switch (tok.tk) {
    case TK::tkWORD: {
      static std::map<std::string, std::function<bool(TOK*)>> verb_to_function =
          {
              {"commit", FReadCommit},       {"tree", FReadTree},
              {"parent", FReadParent},       {"author", FReadAuthor},
              {"committer", FReadCommitter}, {"diff", FReadDiff},
              {"index", FReadIndex},
          };
      auto it = verb_to_function.find(tok.szVal);
      if (it != verb_to_function.end()) {
        is_done = !it->second(&tok);
      }
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
    case TK::tkPLUS: {
      // +++ b/chrome/app/chrome_exe.rc
      is_done = !FReadAddLine(&tok);
      break;
    }
    case TK::tkMINUS: {
      // +++ b/chrome/app/chrome_exe.rc
      is_done = !FReadRemoveLine(&tok);
      break;
    }
    case TK::tkINTEGER: {
      // --- a/chrome/app/chrome_exe.rc
      // REVIEW: why isn't this tkMINUS?
      // is_done = !FReadRemoveLine(&tok);
      break;
    }
    case TK::tkATSIGN: {
      is_done = !FReadDiffHeader(&tok);
      break;
    }

  }

LDone:
  UnattachTok(&tok);
  return !is_done;
}

void CGitDiffReader::ProcessDiffLines(FILE* stream) {
  char stream_line[1024];
  while (fgets(stream_line, std::size(stream_line), stream)) {
    if (!ProcessLogLine(stream_line))
      break;
  }
}

CGitDiffReader::CGitDiffReader(const std::filesystem::path& file_path) {
  std::string command =
      std::string(kGitDiffCommand) + std::string(file_path.u8string());
  std::unique_ptr<FILE, decltype(&_pclose)> git_stream(
      _popen(command.c_str(), "r"), &_pclose);

  ProcessDiffLines(git_stream.get());
}

CGitDiffReader::~CGitDiffReader() {

}

std::vector<FileVersionDiff> CGitDiffReader::GetDiffs() {
  return diffs_;
}
