#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>
#include "upb/port/overflow.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

TEST(PortTest, UnreachableTrapsInDebugMode) {
#if !defined(GTEST_HAS_DEATH_TEST)
  GTEST_SKIP() << "Death test not supported.";
#endif
#if NDEBUG
  GTEST_SKIP() << "NDEBUG is set.";
#endif

  EXPECT_DEATH(
      { UPB_UNREACHABLE(); },
      "port_test.cc:.*: Reached unreachable statement in function "
      "`.*UnreachableTrapsInDebugMode");
}

TEST(PortTest, AddOverflow) {
  size_t res;
  // No overflow
  EXPECT_FALSE(upb_AddOverflow(10, 20, &res));
  EXPECT_EQ(res, 30);

  // Overflow
  EXPECT_TRUE(upb_AddOverflow(SIZE_MAX, 1, &res));
}

TEST(PortTest, MulOverflow) {
  size_t res;
  // No overflow
  EXPECT_FALSE(upb_MulOverflow(10, 20, &res));
  EXPECT_EQ(res, 200);

  // Overflow
  EXPECT_TRUE(upb_MulOverflow(SIZE_MAX / 2, 3, &res));

  // Multiply by 0 (should not divide by 0 and should not overflow)
  EXPECT_FALSE(upb_MulOverflow(10, 0, &res));
  EXPECT_EQ(res, 0);
  EXPECT_FALSE(upb_MulOverflow(0, 10, &res));
  EXPECT_EQ(res, 0);
  EXPECT_FALSE(upb_MulOverflow(0, 0, &res));
  EXPECT_EQ(res, 0);
}

}  // namespace
