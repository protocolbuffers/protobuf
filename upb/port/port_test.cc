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
  EXPECT_FALSE(upb_AddOverflow(10u, 20u, &res));
  EXPECT_EQ(res, 30);

  // Overflow
  EXPECT_TRUE(upb_AddOverflow(SIZE_MAX, 1u, &res));
}

TEST(PortTest, MulOverflow) {
  size_t res;
  // No overflow
  EXPECT_FALSE(upb_MulOverflow(10u, 20u, &res));
  EXPECT_EQ(res, 200);

  // Overflow
  EXPECT_TRUE(upb_MulOverflow(SIZE_MAX / 2, 3u, &res));

  // Multiply by 0 (should not divide by 0 and should not overflow)
  EXPECT_FALSE(upb_MulOverflow(10u, 0u, &res));
  EXPECT_EQ(res, 0);
  EXPECT_FALSE(upb_MulOverflow(0u, 10u, &res));
  EXPECT_EQ(res, 0);
  EXPECT_FALSE(upb_MulOverflow(0u, 0u, &res));
  EXPECT_EQ(res, 0);
}

TEST(PortTest, ShlOverflow) {
  size_t res;
  // No overflow
  EXPECT_FALSE(upb_ShlOverflow(1u, 0u, &res));
  EXPECT_EQ(res, 1);
  EXPECT_FALSE(upb_ShlOverflow(1u, 1u, &res));
  EXPECT_EQ(res, 2);
  EXPECT_FALSE(upb_ShlOverflow(1u, 2u, &res));
  EXPECT_EQ(res, 4);
  EXPECT_FALSE(upb_ShlOverflow(2u, 2u, &res));
  EXPECT_EQ(res, 8);

  EXPECT_FALSE(upb_ShlOverflow(SIZE_MAX / 2, 1u, &res));
  EXPECT_EQ(res, SIZE_MAX - 1);

  // Overflow
  EXPECT_TRUE(upb_ShlOverflow((SIZE_MAX / 2) + 1, 1u, &res));
  EXPECT_TRUE(upb_ShlOverflow(1u, sizeof(size_t) * 8, &res));
  EXPECT_TRUE(upb_ShlOverflow(1u, sizeof(size_t) * 8 + 10, &res));

  // Shift 0
  EXPECT_FALSE(upb_ShlOverflow(0u, 5u, &res));
  EXPECT_EQ(res, 0);

  // uint32_t tests
  uint32_t res32;
  EXPECT_FALSE(upb_ShlOverflow_uint32(1u, 0u, &res32));
  EXPECT_EQ(res32, 1);
  EXPECT_FALSE(upb_ShlOverflow_uint32(1u, 31u, &res32));
  EXPECT_EQ(res32, 0x80000000U);
  EXPECT_TRUE(upb_ShlOverflow_uint32(0x80000000U, 1u, &res32));
  EXPECT_TRUE(upb_ShlOverflow_uint32(1u, 32u, &res32));

  EXPECT_TRUE(upb_AddOverflow_uint32(UINT32_MAX, 1u, &res32));
  EXPECT_TRUE(upb_MulOverflow_uint32(UINT32_MAX / 2 + 1, 2u, &res32));
}

}  // namespace
