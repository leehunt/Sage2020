#pragma once
#include <string>

#define _DEBUG_MEM_TRACE 0

struct GitHash {
  std::string sha_;
  std::string tag_;
  bool operator==(const GitHash& other) const { return sha_ == other.sha_; }
  bool operator!=(const GitHash& other) const { return sha_ != other.sha_; }
  bool IsValid() const { return !sha_.empty(); }
#if _DEBUG_MEM_TRACE
  GitHash() { printf("GitHash\t%p\n", this); }
  ~GitHash() { printf("~GitHash\t%p\n", this); }
#endif
};
