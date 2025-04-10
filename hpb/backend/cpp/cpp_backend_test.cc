// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/hpb/tests/test_model.upb.proto.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/backend/cpp/cpp.h"
#include "google/protobuf/hpb/hpb.h"

namespace hpb::testing {
namespace {

using hpb_unittest::protos::TestModel;
using ::testing::status::IsOkAndHolds;

TEST(CppBackendTest, SerializeStub) {
  TestModel model;
  hpb::Arena arena;
  absl::StatusOr<absl::string_view> bytes = hpb::Serialize(&model, arena);
  EXPECT_THAT(bytes, IsOkAndHolds("stub"));
}
}  // namespace
}  // namespace hpb::testing
