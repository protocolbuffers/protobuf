// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/status.h"

#include <type_traits>

#include <gtest/gtest.h>
#include "hpb_generator/tests/test_model.hpb.h"
#include "hpb/ptr.h"

namespace hpb::testing {

namespace {
using ::hpb_unittest::protos::TestModel;

TEST(StatusTest, RawPtrSize) {
  auto ptr = hpb::StatusOr<int*>(hpb::Error::kUnknown);
  EXPECT_LE(sizeof(ptr), 16);
}

TEST(StatusTest, HpbPtrSize) {
  auto ptr = hpb::StatusOr<hpb::Ptr<TestModel>>(hpb::Error::kUnknown);
  EXPECT_LE(sizeof(ptr), 24);
}

TEST(StatusTest, GuaranteedTraits) {
  using type = hpb::StatusOr<int>;
  EXPECT_TRUE(std::is_trivially_copy_constructible<type>::value);
  EXPECT_TRUE(std::is_trivially_copy_assignable<type>::value);
  EXPECT_TRUE(std::is_trivially_destructible<type>::value);
}

TEST(StatusTest, StatusOr) {
  auto basic = hpb::StatusOr<int>(100);
  EXPECT_TRUE(basic.ok());
  EXPECT_EQ(basic.value(), 100);
}

TEST(StatusTest, Error) {
  auto err = hpb::StatusOr<int>(hpb::Error::kUnknown);
  EXPECT_FALSE(err.ok());
  EXPECT_EQ(err.error(), "Unknown");
  ASSERT_DEATH(err.value(), "Cannot fetch hpb::value for errors.");
}

}  // namespace
}  // namespace hpb::testing
