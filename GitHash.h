#pragma once

#include <string>

#define _DEBUG_MEM_TRACE 0

struct GitHash {
  GitHash() { sha_[0] = 0; }
  char sha_[41];  // 40 bytes hex plus terminator.
  std::string tag_;
  bool operator==(const GitHash& other) const {
    return !strcmp(sha_, other.sha_);
  }
  bool operator!=(const GitHash& other) const {
    return strcmp(sha_, other.sha_) != 0;
  }
  bool IsValid() const { return sha_[0] != 0; }
#if _DEBUG_MEM_TRACE
  GitHash() { printf("GitHash\t%p\n", this); }
  ~GitHash() { printf("~GitHash\t%p\n", this); }
#endif
};
