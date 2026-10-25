// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/stubs/common.h"
#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/port.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"
#include "google/protobuf/unittest_proto3.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

TEST(CopyMessageTest, CopyConstructor) {
  proto2_unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);
  proto2_unittest::TestAllTypes message2(message1);
  TestUtil::ExpectAllFieldsSet(message2);
}

TEST(CopyMessageTest, ArenaEnabledCopyConstructorNull) {
  proto2_unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);
  proto2_unittest::TestAllTypes* message2 =
      Arena::Create<proto2_unittest::TestAllTypes>(nullptr, message1);
  TestUtil::ExpectAllFieldsSet(*message2);
  delete message2;
}

TEST(CopyMessageTest, ArenaEnabledCopyConstructor) {
  proto2_unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);
  Arena arena;
  proto2_unittest::TestAllTypes* message2 =
      Arena::Create<proto2_unittest::TestAllTypes>(&arena, message1);
  TestUtil::ExpectAllFieldsSet(*message2);
}

TEST(CopyMessageTest, ArenaEnabledCopyConstructorArenaLeakTest) {
  // Set possible leaking field types for TestAllTypes with values
  // guaranteed to not be inlined string or Cord values.
  // TestAllTypes has unconditional ArenaDtor registration.
  proto2_unittest::TestAllTypes message1;

  message1.set_optional_string(std::string(1000, 'a'));
  message1.add_repeated_string(std::string(1000, 'd'));

  Arena arena;
  proto2_unittest::TestAllTypes* message2 =
      Arena::Create<proto2_unittest::TestAllTypes>(&arena, message1);

  EXPECT_EQ(message2->optional_string(), message1.optional_string());
  EXPECT_EQ(message2->repeated_string(0), message1.repeated_string(0));
}

TEST(CopyMessageTest, ClearedExplicitPresenceStringsDoNotAllocate) {
  if (internal::ForceInlineStringInProtoc()) {
    GTEST_SKIP() << "Applies to ArenaStringPtr fields.";
  }

  proto2_unittest::TestAllTypes untouched;
  Arena baseline_arena;
  (void)Arena::Create<proto2_unittest::TestAllTypes>(&baseline_arena,
                                                     untouched);

  proto2_unittest::TestAllTypes cleared;
  cleared.set_optional_string("hello");
  cleared.set_default_string("world");
  cleared.clear_optional_string();
  cleared.clear_default_string();

  proto2_unittest::TestAllTypes heap_copy(cleared);
  EXPECT_FALSE(heap_copy.has_optional_string());
  EXPECT_FALSE(heap_copy.has_default_string());
  EXPECT_EQ(heap_copy.optional_string(), "");
  EXPECT_EQ(heap_copy.default_string(), "hello");
  if (!internal::DebugHardenForceCopyDefaultString()) {
    EXPECT_EQ(
        &heap_copy.optional_string(),
        &proto2_unittest::TestAllTypes::default_instance().optional_string());
    EXPECT_EQ(
        &heap_copy.default_string(),
        &proto2_unittest::TestAllTypes::default_instance().default_string());
  }
  heap_copy.Clear();

  Arena arena;
  auto* arena_copy =
      Arena::Create<proto2_unittest::TestAllTypes>(&arena, cleared);
  EXPECT_FALSE(arena_copy->has_optional_string());
  EXPECT_FALSE(arena_copy->has_default_string());
  EXPECT_EQ(arena_copy->optional_string(), "");
  EXPECT_EQ(arena_copy->default_string(), "hello");
  EXPECT_EQ(arena.SpaceUsed(), baseline_arena.SpaceUsed());
  arena_copy->Clear();

  // Explicit-presence fields set to "" remain present after copying.
  proto2_unittest::TestAllTypes present_empty;
  present_empty.set_optional_string("");
  present_empty.set_default_string("");
  proto2_unittest::TestAllTypes present_empty_copy(present_empty);
  EXPECT_TRUE(present_empty_copy.has_optional_string());
  EXPECT_TRUE(present_empty_copy.has_default_string());
  EXPECT_EQ(present_empty_copy.optional_string(), "");
  EXPECT_EQ(present_empty_copy.default_string(), "");
}

TEST(CopyMessageTest, EmptyImplicitPresenceStringsDoNotAllocate) {
  if (internal::ForceInlineStringInProtoc()) {
    GTEST_SKIP() << "Applies to ArenaStringPtr fields.";
  }

  proto3_unittest::TestAllTypes untouched;
  Arena baseline_arena;
  (void)Arena::Create<proto3_unittest::TestAllTypes>(&baseline_arena,
                                                     untouched);

  // Exercise both clear_optional_string() (hint bit cleared) and
  // mutable_optional_string() / set_optional_string("") (hint bit set while
  // string is empty).
  for (int mode = 0; mode < 3; ++mode) {
    proto3_unittest::TestAllTypes empty_mutated;
    if (mode == 0) {
      empty_mutated.set_optional_string("hello");
      empty_mutated.clear_optional_string();
    } else if (mode == 1) {
      (void)empty_mutated.mutable_optional_string();
    } else {
      empty_mutated.set_optional_string("");
    }

    proto3_unittest::TestAllTypes heap_copy(empty_mutated);
    EXPECT_EQ(heap_copy.optional_string(), "");
    if (!internal::DebugHardenForceCopyDefaultString()) {
      EXPECT_EQ(
          &heap_copy.optional_string(),
          &proto3_unittest::TestAllTypes::default_instance().optional_string());
    }
    heap_copy.Clear();

    Arena arena;
    auto* arena_copy =
        Arena::Create<proto3_unittest::TestAllTypes>(&arena, empty_mutated);
    EXPECT_EQ(arena_copy->optional_string(), "");
    EXPECT_EQ(arena.SpaceUsed(), baseline_arena.SpaceUsed());
    arena_copy->Clear();
  }
}

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
