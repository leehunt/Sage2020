#pragma once
#include <filesystem>
#include "FileVersionDiff.h"
#include "GitFileReader.h"

class GitFileStreamCache;
class COutputWnd;
struct TOK;

class GitDiffReader {
 public:
  GitDiffReader(const std::filesystem::path& file_path,
                const std::string& tag,
                COutputWnd* pwndOutput = nullptr);
  virtual ~GitDiffReader();

  const std::vector<FileVersionDiff>& GetDiffs() const;

  std::vector<FileVersionDiff> MoveDiffs();

 private:
  void ProcessDiffLines(FILE* stream);
  bool ProcessLogLine(char* line);

  bool FReadHunkHeader(TOK* ptok);
  bool FReadAddLine(TOK* ptok);
  bool FReadRemoveLine(TOK* ptok);
  bool FParseNamedLine(TOK* ptok);
  bool FReadGitHash(TOK* ptok, GitHash& hash);
  bool FReadCommit(TOK* ptok);
  bool FReadTree(TOK* ptok);
  bool FReadNameEmailAndTime(TOK* ptok, NameEmailTime& name_email_time);
  bool FReadParent(TOK * ptok);
  bool FReadAuthor(TOK* ptok);
  bool FReadCommitter(TOK* ptok);
  bool FReadDiff(TOK* ptok);
  bool FReadIndex(TOK* ptok);
  bool FReadComment(TOK* ptok);
  bool FReadGitDiffTreeColon(TOK* ptok);

  FileVersionDiff* current_diff_;
  std::vector<FileVersionDiff> diffs_;

  std::unique_ptr<GitFileStreamCache> file_stream_cache_;
  std::filesystem::path path_;
};
