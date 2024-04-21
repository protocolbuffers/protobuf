// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/stubs/common.h"
#include <gtest/gtest.h>
#include "google/protobuf/arena.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

TEST(CopyMessageTest, CopyConstructor) {
  protobuf_unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);
  protobuf_unittest::TestAllTypes message2(message1);
  TestUtil::ExpectAllFieldsSet(message2);
}

TEST(CopyMessageTest, ArenaEnabledCopyConstructorNull) {
  protobuf_unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);
  protobuf_unittest::TestAllTypes* message2 =
      Arena::Create<protobuf_unittest::TestAllTypes>(nullptr, message1);
  TestUtil::ExpectAllFieldsSet(*message2);
  delete message2;
}

TEST(CopyMessageTest, ArenaEnabledCopyConstructor) {
  protobuf_unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);
  Arena arena;
  protobuf_unittest::TestAllTypes* message2 =
      Arena::Create<protobuf_unittest::TestAllTypes>(&arena, message1);
  TestUtil::ExpectAllFieldsSet(*message2);
}

TEST(CopyMessageTest, ArenaEnabledCopyConstructorArenaLeakTest) {
  // Set possible leaking field types for TestAllTypes with values
  // guaranteed to not be inlined string or Cord values.
  // TestAllTypes has unconditional ArenaDtor registration.
  protobuf_unittest::TestAllTypes message1;

  message1.set_optional_string(std::string(1000, 'a'));
  message1.add_repeated_string(std::string(1000, 'd'));

  Arena arena;
  protobuf_unittest::TestAllTypes* message2 =
      Arena::Create<protobuf_unittest::TestAllTypes>(&arena, message1);

  EXPECT_EQ(message2->optional_string(), message1.optional_string());
  EXPECT_EQ(message2->repeated_string(0), message1.repeated_string(0));
}

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
