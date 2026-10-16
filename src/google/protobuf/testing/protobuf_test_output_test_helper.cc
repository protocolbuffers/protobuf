// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include "google/protobuf/testing/protobuf_test.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

class DisableTimeAndStackTraces : public ::testing::Environment {
 public:
  void SetUp() override {
    GTEST_FLAG_SET(print_time, false);
    GTEST_FLAG_SET(stack_trace_depth, 0);
    GTEST_FLAG_SET(color, "no");
    GTEST_FLAG_SET(brief, false);
  }
};

[[maybe_unused]] const auto* const kEnv =
    ::testing::AddGlobalTestEnvironment(new DisableTimeAndStackTraces());

PB_TEST(StandaloneSuite, NoParams) {
  ADD_FAILURE() << "NoParams failed as expected";
}

PB_TEST(StandaloneSuite, ValueCartesianProduct) {
  int x = PB_TEST_CHOOSE_VALUE((10, 20), "x");
  bool flag = PB_TEST_CHOOSE_VALUE(testing::Bool(), "flag");
  if (x == 10 || flag) {
    ADD_FAILURE() << "x=" << x << " flag=" << flag;
  }
}

PB_TEST(StandaloneSuite, TypeConstexprAndValue) {
  using T = PB_TEST_CHOOSE_TYPE((int32_t, int64_t));
  constexpr int kMult = PB_TEST_CHOOSE_CONSTEXPR((2, 3), "mult");
  int v = PB_TEST_CHOOSE_VALUE((1, 5), "v");
  T result = static_cast<T>(v) * kMult;
  if (result > 4) {
    ADD_FAILURE() << "sizeof(T)=" << sizeof(T) << " result=" << result;
  }
}

PB_TEST(StandaloneSuite, ConditionalCompileTime) {
  using T = PB_TEST_CHOOSE_TYPE((int32_t, double));
  if constexpr (std::is_same_v<T, int32_t>) {
    constexpr int kShift = PB_TEST_CHOOSE_CONSTEXPR((1, 2), "shift");
    ADD_FAILURE() << "int32 shift=" << kShift;
  } else {
    ADD_FAILURE() << "double branch";
  }
}

class OutputFixtureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    setup_val_ = PB_TEST_CHOOSE_VALUE((100, 200), "setup");
  }

  bool ctor_flag_ = PB_TEST_CHOOSE_VALUE(testing::Bool(), "Arena");
  int setup_val_ = 0;
};

PB_TEST_F(OutputFixtureTest, FixtureOnlyParams) {
  if (!ctor_flag_) {
    ADD_FAILURE() << "FixtureOnlyParams setup_val=" << setup_val_;
  }
}

PB_TEST_F(OutputFixtureTest, FixtureAndBodyParams) {
  int body_val = PB_TEST_CHOOSE_VALUE((1, 2), "body");
  if (ctor_flag_ != (body_val == 2)) {
    ADD_FAILURE() << "ctor_flag=" << ctor_flag_ << " setup_val=" << setup_val_
                  << " body_val=" << body_val;
  }
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
