// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/backend/upb/interop.h"

#include <cstddef>
#include <string>

#include <gtest/gtest.h>
#include "hpb_generator/tests/child_model.hpb.h"
#include "hpb_generator/tests/child_model.upb.h"
#include "hpb_generator/tests/test_model.hpb.h"
#include "hpb_generator/tests/test_model.upb.h"
#include "hpb_generator/tests/test_model.upb_minitable.h"
#include "upb/mem/alloc.h"
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
  EXPECT_EQ(hpb::interop::upb::GetMessage(&model), (upb_Message*)message);
  EXPECT_EQ(model.int_value_with_default(), 123);
}

TEST(CppGeneratedCode, InteropMoveMessageInitialBlockArena) {
  const std::string large_str(4096, 'x');
  upb_Message* original_message = nullptr;
  TestModel model;
  {
    char initial_block[1024];
    upb_Arena* source_arena =
        upb_Arena_Init(initial_block, sizeof(initial_block), &upb_alloc_global);
    ASSERT_NE(source_arena, nullptr);
    hpb_unittest_TestModel* message = hpb_unittest_TestModel_new(source_arena);
    ASSERT_NE(message, nullptr);
    hpb_unittest_TestModel_set_int_value_with_default(message, 123);
    hpb_unittest_TestModel_set_str1(
        message,
        hpb::interop::upb::CopyToUpbStringView(large_str, source_arena));
    original_message = reinterpret_cast<upb_Message*>(message);

    model = hpb::interop::upb::MoveMessage<TestModel>(original_message,
                                                      source_arena);
    upb_Arena_Free(source_arena);
  }

  EXPECT_NE(hpb::interop::upb::GetMessage(&model), original_message);
  EXPECT_EQ(model.int_value_with_default(), 123);
  EXPECT_EQ(model.str1(), large_str);
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

TEST(CppGeneratedCode, FrozenMessageConstRepeatedGetters) {
  upb_Arena* arena = upb_Arena_New();
  hpb_unittest_TestModel* msg = hpb_unittest_TestModel_new(arena);
  ASSERT_NE(msg, nullptr);
  ASSERT_TRUE(hpb_unittest_TestModel_add_value_array(msg, 42, arena));
  ASSERT_TRUE(hpb_unittest_TestModel_add_repeated_string(
      msg, hpb::interop::upb::CopyToUpbStringView("hello", arena), arena));
  hpb_unittest_ChildModel1* child =
      hpb_unittest_TestModel_add_child_models(msg, arena);
  ASSERT_NE(child, nullptr);
  hpb_unittest_ChildModel1_set_child_str1(
      child, hpb::interop::upb::CopyToUpbStringView("child", arena));

  upb_Message* upb_msg = reinterpret_cast<upb_Message*>(msg);
  upb_Message_Freeze(upb_msg, &hpb_0unittest__TestModel_msg_init);
  ASSERT_TRUE(upb_Message_IsFrozen(upb_msg));

  TestModel::CProxy const_handle =
      hpb::interop::upb::MakeCHandle<TestModel>(msg, arena);
  EXPECT_EQ(const_handle.value_array_size(), 1);
  EXPECT_EQ(const_handle.value_array(0), 42);
  EXPECT_EQ(const_handle.value_array()[0], 42);
  EXPECT_EQ(const_handle.repeated_string_size(), 1);
  EXPECT_EQ(const_handle.repeated_string(0), "hello");
  EXPECT_EQ(const_handle.repeated_string()[0], "hello");
  EXPECT_EQ(const_handle.child_models_size(), 1);
  EXPECT_EQ(const_handle.child_models(0)->child_str1(), "child");
  EXPECT_EQ(const_handle.child_models()[0].child_str1(), "child");

#if defined(GTEST_HAS_DEATH_TEST) && !defined(NDEBUG)
  size_t len = 0;
  EXPECT_DEATH((void)hpb_unittest_TestModel_mutable_value_array(msg, &len),
               "upb_Array_IsFrozen");
  EXPECT_DEATH((void)hpb_unittest_TestModel_mutable_repeated_string(msg, &len),
               "upb_Array_IsFrozen");
#endif

  upb_Arena_Free(arena);
}

}  // namespace
}  // namespace hpb::testing
