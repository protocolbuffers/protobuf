// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/backend/upb/interop.h"

#include <gtest/gtest.h>
#include "hpb_generator/tests/test_model.hpb.h"
#include "hpb_generator/tests/test_model.upb.h"
#include "hpb_generator/tests/test_model.upb_minitable.h"
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

TEST(CppGeneratedCode, CanCreateCProxyWithoutCasting) {
  upb_Arena* arena = upb_Arena_New();
  hpb_unittest_TestModel* msg = hpb_unittest_TestModel_new(arena);

  hpb_unittest::protos::TestModel::CProxy const_handle =
      hpb::interop::upb::MakeCHandle<TestModel>(msg, arena);
  EXPECT_EQ(const_handle.value(), 0);
  upb_Arena_Free(arena);
}

TEST(CppGeneratedCode, CanCreateProxyWithoutCasting) {
  upb_Arena* arena = upb_Arena_New();
  hpb_unittest_TestModel* msg = hpb_unittest_TestModel_new(arena);

  hpb_unittest::protos::TestModel::Proxy handle =
      hpb::interop::upb::MakeHandle<TestModel>(msg, arena);
  EXPECT_EQ(handle.value(), 0);
  upb_Arena_Free(arena);
}

TEST(CppGeneratedCode, CanCreateCProxyWithMiniTable) {
  upb_Arena* arena = upb_Arena_New();
  hpb_unittest_TestModel* msg = hpb_unittest_TestModel_new(arena);
  auto minitable = &hpb_0unittest__TestModel_msg_init;
  hpb_unittest::protos::TestModel::CProxy const_handle =
      hpb::interop::upb::MakeCHandle<TestModel>((upb_Message*)msg, minitable,
                                                arena);
  EXPECT_EQ(const_handle.value(), 0);
  upb_Arena_Free(arena);
}

TEST(CppGeneratedCode, CanCreateProxyWithMiniTable) {
  upb_Arena* arena = upb_Arena_New();
  hpb_unittest_TestModel* msg = hpb_unittest_TestModel_new(arena);
  auto minitable = &hpb_0unittest__TestModel_msg_init;
  hpb_unittest::protos::TestModel::Proxy handle =
      hpb::interop::upb::MakeHandle<TestModel>((upb_Message*)msg, minitable,
                                               arena);
  EXPECT_EQ(handle.value(), 0);
  upb_Arena_Free(arena);
}

TEST(CppGeneratedCode, NonmatchingMinitablesExplode) {
  upb_Arena* arena = upb_Arena_New();
  hpb_unittest_TestModel* msg = hpb_unittest_TestModel_new(arena);
  auto different_minitable = &hpb_0unittest__TestModel__NestedChild_msg_init;
  EXPECT_DEATH(
      {
        hpb::interop::upb::MakeHandle<TestModel>((upb_Message*)msg,
                                                 different_minitable, arena);
      },
      "Check failed: minitable == internal::AssociatedUpbTypes<T>::kMiniTable");
  EXPECT_DEATH(
      {
        hpb::interop::upb::MakeCHandle<TestModel>((upb_Message*)msg,
                                                  different_minitable, arena);
      },
      "Check failed: minitable == internal::AssociatedUpbTypes<T>::kMiniTable");
  upb_Arena_Free(arena);
}

}  // namespace
}  // namespace hpb::testing
