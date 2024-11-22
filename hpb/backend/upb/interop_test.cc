// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/hpb/backend/upb/interop.h"

#include <gtest/gtest.h>
#include "google/protobuf/compiler/hpb/tests/test_model.upb.h"
#include "google/protobuf/compiler/hpb/tests/test_model.upb.proto.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"

namespace hpb::testing {
namespace {
using ::hpb_unittest::protos::TestModel;

TEST(CppGeneratedCode, InteropMoveMessage) {
  // Generate message (simulating message created in another VM/language)
  upb_Arena* source_arena = upb_Arena_New();
  hpb_unittest_TestModel* message = hpb_unittest_TestModel_new(source_arena);
  ASSERT_NE(message, nullptr);
  hpb_unittest_TestModel_set_int_value_with_default(message, 123);

  // Move ownership.
  TestModel model = hpb::interop::upb::MoveMessage<TestModel>(
      (upb_Message*)message, source_arena);
  // Now that we have moved ownership, free original arena.
  upb_Arena_Free(source_arena);
  EXPECT_EQ(model.int_value_with_default(), 123);
}

}  // namespace
}  // namespace hpb::testing
