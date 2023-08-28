// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
      Arena::CreateMessage<protobuf_unittest::TestAllTypes>(nullptr, message1);
  TestUtil::ExpectAllFieldsSet(*message2);
  delete message2;
}

TEST(CopyMessageTest, ArenaEnabledCopyConstructor) {
  protobuf_unittest::TestAllTypes message1;
  TestUtil::SetAllFields(&message1);
  Arena arena;
  protobuf_unittest::TestAllTypes* message2 =
      Arena::CreateMessage<protobuf_unittest::TestAllTypes>(&arena, message1);
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
      Arena::CreateMessage<protobuf_unittest::TestAllTypes>(&arena, message1);

  EXPECT_EQ(message2->optional_string(), message1.optional_string());
  EXPECT_EQ(message2->repeated_string(0), message1.repeated_string(0));
}

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
