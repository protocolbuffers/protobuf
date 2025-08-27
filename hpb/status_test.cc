// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/status.h"

#include <memory>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include "absl/status/statusor.h"
#include "hpb_generator/tests/test_model.hpb.h"
#include "hpb/ptr.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace hpb::testing {

namespace {
using ::hpb_unittest::protos::TestModel;

TEST(StatusTest, RawPtrSize) {
  using type = hpb::StatusOr<int*>;
  EXPECT_LE(sizeof(type), 16);
}

TEST(StatusTest, HpbPtrSize) {
  using type = hpb::StatusOr<hpb::Ptr<TestModel>>;
  EXPECT_LE(sizeof(type), 24);
}

TEST(StatusTest, GuaranteedTraits) {
  using type = hpb::StatusOr<int>;
  EXPECT_TRUE(std::is_trivially_copy_constructible<type>::value);
  EXPECT_TRUE(std::is_trivially_copy_assignable<type>::value);
  EXPECT_TRUE(std::is_trivially_destructible<type>::value);
}

TEST(StatusTest, Moves) {
  std::unique_ptr<int> i = std::make_unique<int>(100);
  hpb::StatusOr<std::unique_ptr<int>> status(std::move(i));
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(*status.value(), 100);

  hpb::StatusOr<std::unique_ptr<int>> move(std::move(status));
  EXPECT_TRUE(move.ok());
  EXPECT_EQ(*move.value(), 100);
}

TEST(StatusTest, StatusOr) {
  auto basic = hpb::StatusOr<int>(100);
  EXPECT_TRUE(basic.ok());
  EXPECT_EQ(basic.value(), 100);
}

TEST(StatusTest, DecodeError) {
  auto statusOr =
      hpb::StatusOr<int>(internal::backend::Error(kUpb_DecodeStatus_BadUtf8));
  EXPECT_FALSE(statusOr.ok());
  EXPECT_EQ(statusOr.error(), "String field had bad UTF-8");
  ASSERT_DEATH(statusOr.value(), "Cannot fetch hpb::value for errors.");
}

TEST(StatusTest, EncodeError) {
  auto statusOr = hpb::StatusOr<int>(
      internal::backend::Error(kUpb_EncodeStatus_MaxDepthExceeded));
  EXPECT_FALSE(statusOr.ok());
  EXPECT_EQ(statusOr.error(), "Max depth exceeded");
  ASSERT_DEATH(statusOr.value(), "Cannot fetch hpb::value for errors.");
}

TEST(StatusTest, AbslStatusOrOK) {
  absl::StatusOr<int> absl_convert = hpb::StatusOr<int>(100).ToAbslStatusOr();
  EXPECT_TRUE(absl_convert.ok());
  EXPECT_EQ(absl_convert.value(), 100);
}

TEST(StatusTest, AblStatusOrError) {
  absl::StatusOr<int> absl_convert =
      hpb::StatusOr<int>(
          internal::backend::Error(kUpb_EncodeStatus_MaxDepthExceeded))
          .ToAbslStatusOr();
  EXPECT_FALSE(absl_convert.ok());
  EXPECT_EQ(absl_convert.status().message(), "Max depth exceeded");
}

}  // namespace
}  // namespace hpb::testing
