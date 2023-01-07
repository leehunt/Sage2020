#pragma once
#include <filesystem>
#include "FileVersionDiff.h"
#include "GitFileReader.h"

class GitFileStreamCache;
class COutputWnd;
struct TOK;

class GitDiffReader {
 public:
  enum class Opt : int {
    NONE = 0,
    NO_FILTER_TO_FILE = 1 << 0,
    NO_FIRST_PARENT = 1 << 1,
    NO_FOLLOW = 1 << 2,
  };
  class Opts {
   public:
    Opts(Opt opt) : opts_(static_cast<int>(opt)) {}
    Opts(const Opts& opts) : opts_(opts.opts_) {}

    Opts operator|(const Opt& opt) const {
      Opts new_opt(opt);
      return new_opt;
    }
    Opts operator|=(const Opt& opt) {
      opts_ |= static_cast<int>(opt);
      return *this;
    }

    Opts operator|(const Opts& opts) const {
      Opts new_opts(*this);
      new_opts |= opts;
      return new_opts;
    }
    Opts operator|=(const Opts& opts) {
      opts_ |= opts.opts_;
      return *this;
    }

    bool HasOpts(const Opts& opts) const {
      return (opts_ & opts.opts_) == opts.opts_;
    }

   private:
    int opts_;
  };
  GitDiffReader(const std::filesystem::path& file_path,
                const std::string& tag,
                COutputWnd* pwndOutput = nullptr,
                const Opts& opts = Opt::NONE);
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
  bool FReadParent(TOK* ptok);
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
