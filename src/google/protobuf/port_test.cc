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

#include <gtest/gtest.h>
#include "absl/base/config.h"

// Must be included last
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

int assume_var_for_test = 1;

TEST(PortTest, ProtobufAssume) {
  PROTOBUF_ASSUME(assume_var_for_test == 1);
#ifdef GTEST_HAS_DEATH_TEST
  EXPECT_DEBUG_DEATH(
      PROTOBUF_ASSUME(assume_var_for_test == 2),
      "port_test\\.cc:.*Assumption failed: 'assume_var_for_test == 2'");
#endif
}

TEST(PortTest, UnreachableTrapsOnDebugMode) {
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

}  // namespace internal
}  // namespace protobuf
}  // namespace google
