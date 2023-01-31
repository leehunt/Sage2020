#include "pch.h"

#include "GitRevFilter.h"

#include <assert.h>
#include <string>
#include <vector>

#include "OutputWnd.h"
#include "Utility.h"

constexpr TCHAR kGitLogNameRevCommandFromStdIn[] =
    _T("git --no-pager name-rev --annotate-stdin");

GitRevFilter::GitRevFilter(const _TCHAR git_file_dir[],
                           FILE* stream,
                           COutputWnd* pwndOutput) {
  // Look for "commit "... lines that denote header blacks with hashes to be
  // decorated.

  std::filesystem::path filtered_file_path;
  auto filtered_file = CreateTmpFile(filtered_file_path);
  if (!filtered_file) {
    assert(false);
    return;
  }
  std::filesystem::path unfiltered_file_path;
  auto unfiltered_file = CreateTmpFile(unfiltered_file_path);
  if (!unfiltered_file) {
    assert(false);
    return;
  }
  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(
        _T("   Processing revs via temp 'to process' file '%1!s!' and ")
        _T("'%2!s!'."),
        filtered_file_path.c_str(), unfiltered_file_path.c_str());
    pwndOutput->AppendDebugTabMessage(message);
  }

  char stream_line[1024];
  bool in_header = true;
  std::vector<int> seam_offsets;
  int line_index = 0;
  while (fgets(stream_line, (int)std::size(stream_line), stream)) {
    if (!strncmp(stream_line, "commit ", 7)) {
      if (!in_header) {
        in_header = true;
        seam_offsets.push_back(line_index);
      }
    } else if (!strncmp(stream_line, "diff ", 5)) {
      assert(in_header);
      in_header = false;
      seam_offsets.push_back(line_index);
    }
    if (fputs(stream_line,
              in_header ? filtered_file.get() : unfiltered_file.get()) == EOF) {
      assert(false);
      break;
    }
    line_index++;
  }
  seam_offsets.push_back(line_index);  // Finish current range.
  fflush(filtered_file.get());
  fflush(unfiltered_file.get());

  // Filter the file.
  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(
        _T("   Processing revisions from last command: '%1!s!'"),
        kGitLogNameRevCommandFromStdIn);
    pwndOutput->AppendDebugTabMessage(message);
  }
  rewind(filtered_file.get());
  ProcessPipe process_pipe_git_rev(kGitLogNameRevCommandFromStdIn, git_file_dir,
                                   filtered_file.get());

  // Now stitch the file back together.
  std::filesystem::path dest_filtered_file_path;
  dest_filtered_file_ = CreateTmpFile(dest_filtered_file_path);
  if (!dest_filtered_file_) {
    assert(false);
    return;
  }
  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(_T("   Processing revs via temp output file'%1!s!'"),
                          dest_filtered_file_path.c_str());
    pwndOutput->AppendDebugTabMessage(message);
  }

  assert(!seam_offsets.empty());
  if (!seam_offsets.empty()) {
    rewind(unfiltered_file.get());
    char stream_filtered_line[1024];

    in_header = true;

    int seam_vector_index = 0;
    for (int i = 0; i < line_index; i++) {
      if (seam_vector_index == seam_offsets.size()) {
        // Corrupt indexes.
        assert(false);
        return;
      }
      if (i == seam_offsets[seam_vector_index]) {
        seam_vector_index++;
        in_header = !in_header;
      }
      // Swtich between filtered and unfiltered input.
      if (!fgets(stream_filtered_line, (int)std::size(stream_filtered_line),
                 in_header ? process_pipe_git_rev.GetStandardOutput()
                           : unfiltered_file.get())) {
        assert(false);
        return;
      }
      // Save the filtered or unfiltered results to |dest_filtered_file|.
      if (fputs(stream_filtered_line, dest_filtered_file_.get()) == EOF) {
        assert(false);
        return;
      }
    }
    //assert(feof(unfiltered_file.get()));
  }
  //assert(feof(stream));

  fflush(dest_filtered_file_.get());
  rewind(dest_filtered_file_.get());
  //assert(feof(process_pipe_git_rev.GetStandardOutput()));

  if (pwndOutput != nullptr) {
    CString message;
    message.FormatMessage(
        _T("   Processing revs completed. Found '%1!d!' ranges on '%2!d!' ")
        _T("lines."),
        (int)seam_offsets.size(), line_index);
    pwndOutput->AppendDebugTabMessage(message);
  }
}