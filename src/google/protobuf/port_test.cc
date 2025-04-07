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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <gtest/gtest.h>
#include "absl/base/config.h"

// Must be included last
#include "absl/base/optimization.h"
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

TEST(PortTest, PrefetchWorksWithValidOffsets) {
  struct Base {
    char a[ABSL_CACHELINE_SIZE] = {1};
  };
  struct Derived : Base {
    char b[ABSL_CACHELINE_SIZE * 2] = {2};
  };

  Derived derived_array[3] = {};
  Base* base_ptr = derived_array;

  constexpr uintptr_t kOkOffset = sizeof(Derived) / 2;
  constexpr uintptr_t kJustBeyondOffset = sizeof(Derived);
  // A prefetch of a guaranteed valid address (using lines).
  {
    static constexpr PrefetchOpts kOpts = {
        {1, PrefetchOpts::kLines},
        {kOkOffset, PrefetchOpts::kBytes},
    };
    Prefetch<kOpts>(base_ptr);
  }
  // A prefetch of a guaranteed valid address (using bytes not wholly divisible
  // into lines).
  {
    static constexpr PrefetchOpts kOpts = {
        {ABSL_CACHELINE_SIZE + ABSL_CACHELINE_SIZE / 2, PrefetchOpts::kBytes},
        {kOkOffset, PrefetchOpts::kBytes},
    };
    Prefetch<kOpts>(base_ptr);
  }
  // Hit the tail for-loop constexpr path (barely).
  {
    static constexpr PrefetchOpts kOpts = {
        {20, PrefetchOpts::kLines},
        {kOkOffset, PrefetchOpts::kBytes},
    };
    Prefetch<kOpts>(base_ptr);
  }
  // Hit the tail for-loop constexpr path (a lot).
  {
    static constexpr PrefetchOpts kOpts = {
        {100, PrefetchOpts::kLines},
        {kOkOffset, PrefetchOpts::kBytes},
    };
    Prefetch<kOpts>(base_ptr);
  }
  // `base_ptr` actually points to an array of `Derived`s. Test that an explicit
  // non-void pointed-to template parameter compiles and doesn't trigger the
  // "type mismatch with actual pointer type" static assert.
  {
    static constexpr PrefetchOpts kOpts = {
        {2, PrefetchOpts::kObjects},
        {kOkOffset, PrefetchOpts::kBytes},
    };
    Prefetch<kOpts, Derived>(base_ptr);
  }
  // A prefetch of an invalid address (beyond the end of the buffer) is valid
  // and is just a no-op.
  {
    static constexpr PrefetchOpts kOpts = {
        {2, PrefetchOpts::kLines},
        {kJustBeyondOffset, PrefetchOpts::kBytes},
    };
    Prefetch<kOpts>(base_ptr);
  }
}

// Prefetching is a no-op for non-Clang compilers in OSS.
#if defined(__clang__) && defined(GTEST_HAS_DEATH_TEST)
TEST(PortDeathTest, PrefetchDiagnosesInvalidOffsets) {
  constexpr size_t kBufSize = ABSL_CACHELINE_SIZE * 100;
  std::byte buf[kBufSize];
  constexpr uintptr_t kWrapAroundOffset =
      std::numeric_limits<uintptr_t>::max() - 1;
  // A prefetch with a wrap-around offset should be diagnosed in debug builds.
  {
    static constexpr PrefetchOpts kOpts = {
        {2, PrefetchOpts::kLines},
        {kWrapAroundOffset, PrefetchOpts::kBytes},
    };
#ifndef NDEBUG
    // We want to catch overflows in address + offset computation in debug
    // builds, because that in itself is a UB...
    EXPECT_DEATH(Prefetch<kOpts>(buf), "assertion failed.*");
#else
    // ... but UB doesn't matter for prefetching, because the prefetch
    // instruction is just a no-op for invalid addresses. So we turn off the UB
    // check in opt builds and the following should succeed.
    Prefetch<kOpts>(buf);
#endif
  }
}
#endif  // __clang__ && GTEST_HAS_DEATH_TEST

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
