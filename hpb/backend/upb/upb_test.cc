// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "google/protobuf/compiler/hpb/tests/test_model.hpb.h"
#include "google/protobuf/hpb/arena.h"
#include "google/protobuf/hpb/hpb.h"

namespace hpb::testing {
namespace {
using ::hpb_unittest::protos::TestModel;

TEST(UpbBackendTest, DebugString) {
  hpb::Arena arena;
  auto testModel = hpb::CreateMessage<TestModel>(arena);
  testModel.set_str1("avotai");
  auto dbgstr = hpb::DebugString(&testModel);
  EXPECT_EQ(dbgstr, "115: \"avotai\"\n");
}

}  // namespace
}  // namespace hpb::testing
