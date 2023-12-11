// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "protos/protos_internal.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "protos_generator/tests/test_model.upb.h"
#include "protos_generator/tests/test_model.upb.proto.h"
#include "upb/mem/arena.h"

namespace protos::testing {
namespace {
using ::protos_generator::test::protos::TestModel;

TEST(CppGeneratedCode, InternalMoveMessage) {
  // Generate message (simulating message created in another VM/language)
  upb_Arena* source_arena = upb_Arena_New();
  protos_generator_test_TestModel* message =
      protos_generator_test_TestModel_new(source_arena);
  ASSERT_NE(message, nullptr);
  protos_generator_test_TestModel_set_int_value_with_default(message, 123);

  // Move ownership.
  TestModel model =
      protos::internal::MoveMessage<TestModel>(message, source_arena);
  // Now that we have moved ownership, free original arena.
  upb_Arena_Free(source_arena);
  EXPECT_EQ(model.int_value_with_default(), 123);
}

}  // namespace
}  // namespace protos::testing
