#include "upb/base/internal/log2.h"

#include <climits>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

namespace {

TEST(Log2Test, Log2Ceiling) {
  EXPECT_EQ(upb_Log2Ceiling(0), 0);
  EXPECT_EQ(upb_Log2Ceiling(1), 0);
  EXPECT_EQ(upb_Log2Ceiling(2), 1);
  EXPECT_EQ(upb_Log2Ceiling(3), 2);
  EXPECT_EQ(upb_Log2Ceiling(4), 2);
  EXPECT_EQ(upb_Log2Ceiling(5), 3);
  EXPECT_EQ(upb_Log2Ceiling(6), 3);
  EXPECT_EQ(upb_Log2Ceiling(7), 3);
  EXPECT_EQ(upb_Log2Ceiling(8), 3);

  EXPECT_EQ(upb_Log2Ceiling(SIZE_MAX), sizeof(size_t) * CHAR_BIT);
}

TEST(Log2Test, RoundUpToPowerOfTwo) {
  EXPECT_EQ(upb_RoundUpToPowerOfTwo(0), 1);
  EXPECT_EQ(upb_RoundUpToPowerOfTwo(1), 1);
  EXPECT_EQ(upb_RoundUpToPowerOfTwo(2), 2);
  EXPECT_EQ(upb_RoundUpToPowerOfTwo(3), 4);
  EXPECT_EQ(upb_RoundUpToPowerOfTwo(4), 4);
  EXPECT_EQ(upb_RoundUpToPowerOfTwo(5), 8);
}

TEST(Log2Test, ShlOverflow) {
  size_t a = 1;
  EXPECT_FALSE(upb_ShlOverflow(&a, 0));
  EXPECT_EQ(a, 1);
  EXPECT_FALSE(upb_ShlOverflow(&a, 1));
  EXPECT_EQ(a, 2);
  EXPECT_FALSE(upb_ShlOverflow(&a, 2));
  EXPECT_EQ(a, 8);

  a = SIZE_MAX / 2;
  EXPECT_FALSE(upb_ShlOverflow(&a, 1));
  EXPECT_EQ(a, SIZE_MAX - 1);

  a = (SIZE_MAX / 2) + 1;
  EXPECT_TRUE(upb_ShlOverflow(&a, 1));
}

}  // namespace
