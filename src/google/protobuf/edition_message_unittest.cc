// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This test is meant to verify the interaction of the most common and
// representative edition features. Each new edition feature must have its own
// unit test and we'll selectively accept new features when we believe doing so
// improves test coverage in a meaningful way.
//
// Note that new features that break the backward compatibility poses challenges
// to shared unit tests infrastructure this test uses. It may force us to split
// the shared tests. Keep the shared unit tests (message_unittest.inc)
// representative without sacrificing test coverage.

#include <functional>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/edition_unittest.pb.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/has_bits.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/unittest_import.pb.h"

#define MESSAGE_TEST_NAME EditionMessageTest
#define MESSAGE_FACTORY_TEST_NAME EditionMessageFactoryTest
#define UNITTEST_PACKAGE_NAME "edition_unittest"
#define UNITTEST ::edition_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import

// Must include after the above macros.
// clang-format off
#include "google/protobuf/test_util.inc"
#include "google/protobuf/message_unittest.inc"
// clang-format on

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {


TEST(EditionMessageTest, AllSetMethodsOnStringField) {
  UNITTEST::TestAllTypes msg;

  msg.set_optional_string(absl::string_view("Abcdef"));
  EXPECT_EQ(msg.optional_string(), "Abcdef");

  msg.set_optional_string("Asciiz");
  EXPECT_EQ(msg.optional_string(), "Asciiz");

  std::string value = "std::string value 1";
  msg.set_optional_string(value);
  EXPECT_EQ(msg.optional_string(), "std::string value 1");

  value = "std::string value 2";
  msg.set_optional_string(std::cref(value));
  EXPECT_EQ(msg.optional_string(), "std::string value 2");

  value = "std::string value 3";
  msg.set_optional_string(std::move(value));
  EXPECT_EQ(msg.optional_string(), "std::string value 3");
}

TEST(EditionMessageTest, AllAddMethodsOnRepeatedStringField) {
  UNITTEST::TestAllTypes msg;

  msg.add_repeated_string(absl::string_view("Abcdef"));
  EXPECT_EQ(msg.repeated_string(0), "Abcdef");
  msg.clear_repeated_string();

  msg.add_repeated_string("Asciiz");
  EXPECT_EQ(msg.repeated_string(0), "Asciiz");
  msg.clear_repeated_string();

  std::string value = "std::string value 1";
  msg.add_repeated_string(value);
  EXPECT_EQ(msg.repeated_string(0), "std::string value 1");
  msg.clear_repeated_string();

  value = "std::string value 2";
  msg.add_repeated_string(std::cref(value));
  EXPECT_EQ(msg.repeated_string(0), "std::string value 2");
  msg.clear_repeated_string();

  value = "std::string value 3";
  msg.add_repeated_string(std::move(value));
  EXPECT_EQ(msg.repeated_string(0), "std::string value 3");
  msg.clear_repeated_string();
}

template <typename T>
static const TcParseTableBase* GetTableIfAvailable(...) {
  return nullptr;
}

template <typename T>
static const TcParseTableBase* GetTableIfAvailable(
    decltype(TcParser::GetTable<T>())) {
  return TcParser::GetTable<T>();
}

TEST(EditionMessageTest,
     TestRegressionInlinedStringAuxIdxMismatchOnFastParser) {
  using Proto = UNITTEST::InlinedStringIdxRegressionProto;

  auto* table = GetTableIfAvailable<Proto>(nullptr);
  // Only test when TDP is on, and we have these fields inlined.
  if (table != nullptr &&
      table->fast_entry(1)->target() == TcParser::FastSiS1) {
    // optional string str1 = 1;
    // The aux_idx points to the inlined_string_idx and not the actual aux_idx.
    EXPECT_EQ(table->fast_entry(1)->bits.aux_idx(), 1);
    // optional InlinedStringIdxRegressionProto sub = 2;
    EXPECT_EQ(table->fast_entry(2)->bits.aux_idx(), 1);
    // optional string str2 = 3;
    // The aux_idx points to the inlined_string_idx and not the actual aux_idx.
    EXPECT_EQ(table->fast_entry(3)->bits.aux_idx(), 2);
    // optional string str3 = 4;
    // The aux_idx points to the inlined_string_idx and not the actual aux_idx.
    EXPECT_EQ(table->fast_entry(0)->bits.aux_idx(), 3);
  }

  std::string encoded;
  {
    Proto proto;
    // We use strings longer than SSO.
    proto.set_str1(std::string(100, 'a'));
    proto.set_str2(std::string(100, 'a'));
    proto.set_str3(std::string(100, 'a'));
    encoded = proto.SerializeAsString();
  }
  Arena arena;
  auto* proto = Arena::Create<Proto>(&arena);
  // We don't alter donation here, so it works even if the idx are bad.
  ASSERT_TRUE(proto->ParseFromString(encoded));
  // Now we alter donation bits. str2's bit (#2) will be off, but its aux_idx
  // (#3) will point to a donated string.
  proto = Arena::Create<Proto>(&arena);
  // String view fields don't allow mutable accessors which obviate the needs
  // for donation tracker. We will clean up the internal logic after migration
  // to string view fields matures.
  proto->set_str1("");
  proto->set_str2("");
  proto->set_str3("");
  // With the bug, this breaks the cleanup list, causing UB on arena
  // destruction.
  ASSERT_TRUE(proto->ParseFromString(encoded));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
