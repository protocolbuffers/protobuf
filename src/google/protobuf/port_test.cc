// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
#include "google/protobuf/port.h"

#include <stdio.h>
#include <stdlib.h>

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>
#include "absl/base/config.h"

// Must be included last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

int assume_var_for_test = 1;

TEST(PortDeathTest, ProtobufAssume) {
  PROTOBUF_ASSUME(assume_var_for_test == 1);
#ifdef GTEST_HAS_DEATH_TEST
#if defined(NDEBUG)
  // If NDEBUG is defined, then instead of reliably crashing, the code below
  // will assume a false statement. This is undefined behavior which will trip
  // up sanitizers.
  GTEST_SKIP() << "Can't test PROTOBUF_ASSUME()";
#else
  EXPECT_DEBUG_DEATH(
      PROTOBUF_ASSUME(assume_var_for_test == 2),
      "port_test\\.cc:.*Assumption failed: 'assume_var_for_test == 2'");
#endif
#endif
}

TEST(PortDeathTest, UnreachableTrapsOnDebugMode) {
#ifdef GTEST_HAS_DEATH_TEST
#if defined(NDEBUG)
  // In NDEBUG we crash with a UD instruction, so we don't get the "Assumption
  // failed" error.
  GTEST_SKIP() << "Can't test __builtin_unreachable()";
#elif ABSL_HAVE_BUILTIN(__builtin_FILE)
  EXPECT_DEATH(Unreachable(),
               "port_test\\.cc:.*Assumption failed: 'Unreachable'");
#else
  EXPECT_DEATH(Unreachable(), "Assumption failed: 'Unreachable'");
#endif
#endif
}

TEST(PortDeathTest, PrefetchWithHugeOffsetFails) {
#ifdef GTEST_HAS_DEATH_TEST
  char mem[1024];
  uintptr_t max_addr = std::numeric_limits<uintptr_t>::max();
  // Inside the buffer.
  uintptr_t good_offset = sizeof(mem) / 2;
  // Just before wrap-around. The resulting address is max uintptr.
  uintptr_t max_ok_offset = max_addr - reinterpret_cast<uintptr_t>(mem);
  // Just after wrap-around. The resulting address is 0.
  uintptr_t too_huge_offset_1 = max_ok_offset + 1;
  uintptr_t too_huge_offset_2 = max_ok_offset + 100;
  EXPECT_NO_FATAL_FAILURE(PROTOBUF_PREFETCH_WITH_OFFSET(mem, good_offset));
  // A prefetch of an invalid address is valid and is just a no-op.
  EXPECT_NO_FATAL_FAILURE(PROTOBUF_PREFETCH_WITH_OFFSET(mem, max_ok_offset));
  // But we don't want invalid (wrap-around) offsets: they should crash the
  // program.
  EXPECT_DEATH(PROTOBUF_PREFETCH_WITH_OFFSET(mem, too_huge_offset_1), "");
  EXPECT_DEATH(PROTOBUF_PREFETCH_WITH_OFFSET(mem, too_huge_offset_2), "");
#endif
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
