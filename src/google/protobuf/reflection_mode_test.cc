// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#include "google/protobuf/reflection_mode.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {

#ifndef PROTOBUF_NO_THREADLOCAL

TEST(ReflectionModeTest, SimpleScopedReflection) {
  ASSERT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDefault);
  ScopedReflectionMode scope(ReflectionMode::kDiagnostics);
  EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDiagnostics);
}

TEST(ReflectionModeTest, CleanNestedScopedReflection) {
  ASSERT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDefault);
  {
    ScopedReflectionMode scope1(ReflectionMode::kDebugString);
    EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
              ReflectionMode::kDebugString);
    {
      ScopedReflectionMode scope2(ReflectionMode::kDiagnostics);
      EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
                ReflectionMode::kDiagnostics);
    }
    EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
              ReflectionMode::kDebugString);
  }
  EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDefault);
}

TEST(ReflectionModeTest, UglyNestedScopedReflection) {
  ASSERT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDefault);
  ScopedReflectionMode scope1(ReflectionMode::kDebugString);
  EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDebugString);
  ScopedReflectionMode scope2(ReflectionMode::kDiagnostics);
  EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDiagnostics);
}

TEST(ReflectionModeTest, DebugStringModeDoesNotReplaceDiagnosticsMode) {
  ASSERT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDefault);
  ScopedReflectionMode scope1(ReflectionMode::kDiagnostics);
  {
    ScopedReflectionMode scope2(ReflectionMode::kDebugString);
    EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
              ReflectionMode::kDiagnostics);
  }
}

#else

TEST(ReflectionModeTest, AlwaysReturnDefaultWhenNoThreadLocal) {
  ASSERT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDefault);
  {
    ScopedReflectionMode scope1(ReflectionMode::kDebugString);
    EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
              ReflectionMode::kDefault);
    {
      ScopedReflectionMode scope2(ReflectionMode::kDiagnostics);
      EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
                ReflectionMode::kDefault);
    }
    EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
              ReflectionMode::kDefault);
  }
  EXPECT_EQ(ScopedReflectionMode::current_reflection_mode(),
            ReflectionMode::kDefault);
}

#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google
