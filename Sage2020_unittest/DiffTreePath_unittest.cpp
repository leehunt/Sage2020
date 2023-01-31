#include "pch.h"

#include "..\DiffTreePath.h"
#include "..\FileVersionDiff.h"

// Gtest 'friend' forwarders.
  class DiffTreePathTest : public testing::Test {
 public:
  ;
};

TEST(DiffTreePathTest, CreateSimple) {
  DiffTreePath path({});
  EXPECT_TRUE(path.empty());

  path.emplace_back(DiffTreePathItem{});
  EXPECT_FALSE(path.empty());
}