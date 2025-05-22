// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_no_field_presence.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::proto2_nofieldpresence_unittest::ExplicitForeignMessage;
using ::proto2_nofieldpresence_unittest::FOREIGN_BAZ;
using ::proto2_nofieldpresence_unittest::FOREIGN_FOO;
using ::proto2_nofieldpresence_unittest::ForeignMessage;
using ::proto2_nofieldpresence_unittest::TestAllMapTypes;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Not;
using ::testing::StrEq;
using ::testing::UnorderedPointwise;

// Custom gmock matchers to simplify testing for map entries.
//
// "HasKey" in this case means HasField() will return true in reflection.
MATCHER(MapEntryHasKey, "") {
  const Reflection* r = arg.GetReflection();
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* key = desc->map_key();

  return r->HasField(arg, key);
}

// "HasValue" in this case means HasField() will return true in reflection.
MATCHER(MapEntryHasValue, "") {
  const Reflection* r = arg.GetReflection();
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* key = desc->map_value();

  return r->HasField(arg, key);
}

// The following pattern is used to create a monomorphic matcher that matches an
// input type (to avoid implicit casts between sign and unsigned integers).
// Intentionally choose a verbose and specific namespace name so that there are
// no namespace conflicts. MSVC seems to not know how to prioritize
// ns::internal vs. ns::(anonymous namespace)::internal.
// Sample error:
//  D:\a\protobuf\protobuf\src\google/protobuf/map.h(138): error C2872:
//  'internal': ambiguous symbol
//  D:\a\protobuf\protobuf\google/protobuf/unittest_no_field_presence.pb.h(46):
//  note: could be 'google::protobuf::internal'
//  D:\a\protobuf\protobuf\src\google\protobuf\no_field_presence_map_test.cc(61):
//  note: or       'google::protobuf::`anonymous-namespace'::internal'
namespace no_presence_map_test_internal {
// `MATCHER_P` defines a polymorphic matcher; we monomorphize it for
// `uint64_t` below to avoid conflicting deduced template arguments.
MATCHER_P(MapEntryListFieldsSize, expected_size, "") {
  const Reflection* r = arg.GetReflection();

  std::vector<const FieldDescriptor*> list_fields_output;
  r->ListFields(arg, &list_fields_output);
  return list_fields_output.size() == expected_size;
}
}  // namespace no_presence_map_test_internal
// TODO: b/371232929 - can make this `inline constexpr` with C++17 as baseline.
constexpr auto& MapEntryListFieldsSize =
    no_presence_map_test_internal::MapEntryListFieldsSize<uint64_t>;

MATCHER(MapEntryKeyExplicitPresence, "") {
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* key = desc->map_key();

  return key->has_presence();
}

MATCHER(MapEntryValueExplicitPresence, "") {
  const Descriptor* desc = arg.GetDescriptor();
  const FieldDescriptor* value = desc->map_value();

  return value->has_presence();
}

// Given a message of type ForeignMessage or ExplicitForeignMessage that's also
// part of a map value, return whether its field |c| is present.
bool MapValueSubMessageHasFieldViaReflection(
    const google::protobuf::Message& map_submessage) {
  const Reflection* r = map_submessage.GetReflection();
  const Descriptor* desc = map_submessage.GetDescriptor();

  // "c" only exists in ForeignMessage or ExplicitForeignMessage, so an
  // assertion is necessary.
  ABSL_CHECK(absl::EndsWith(desc->name(), "ForeignMessage"));
  const FieldDescriptor* field = desc->FindFieldByName("c");

  return r->HasField(map_submessage, field);
}

TEST(NoFieldPresenceTest, GenCodeMapMissingKeyDeathTest) {
  TestAllMapTypes message;

  // Trying to find an unset key in a map would crash.
  EXPECT_DEATH(message.map_int32_bytes().at(9), "key not found");
}

#ifndef NDEBUG
// This test case tests a DCHECK assertion. If this scenario happens in
// optimized builds, it's technically UB, so having a test case for it in opt
// builds is meaningless.
TEST(NoFieldPresenceTest, GenCodeMapReflectionMissingKeyDeathTest) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Trying to get an unset map entry would crash with a DCHECK in debug mode.
  EXPECT_DEATH(r->GetRepeatedMessage(message, field_map_int32_bytes, 0),
               "index < current_size_");
}
#endif

TEST(NoFieldPresenceTest, ReflectionEmptyMapTest) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");
  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");
  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");
  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  ASSERT_NE(field_map_int32_bytes, nullptr);
  ASSERT_NE(field_map_int32_foreign_enum, nullptr);
  ASSERT_NE(field_map_int32_foreign_message, nullptr);
  ASSERT_NE(field_map_int32_explicit_foreign_message, nullptr);

  // Maps are treated as repeated fields -- so fieldsize should be zero.
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_bytes));
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_foreign_enum));
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_foreign_message));
  EXPECT_EQ(0, r->FieldSize(message, field_map_int32_explicit_foreign_message));
}

TEST(NoFieldPresenceTest, TestNonZeroMapEntriesStringValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_bytes())[9] = "hello";

  EXPECT_EQ(1, message.map_int32_bytes().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_bytes().contains(9));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_bytes().count(9));
  // Value can be retrieved.
  EXPECT_EQ("hello", message.map_int32_bytes().at(9));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestNonZeroMapEntriesIntValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  ASSERT_NE(0, static_cast<uint32_t>(FOREIGN_BAZ));

  EXPECT_EQ(1, message.map_int32_foreign_enum().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_foreign_enum().contains(99));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_foreign_enum().count(99));
  // Value can be retrieved.
  EXPECT_EQ(FOREIGN_BAZ, message.map_int32_foreign_enum().at(99));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestNonZeroMapEntriesMessageValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_foreign_message())[123].set_c(10101);

  EXPECT_EQ(1, message.map_int32_foreign_message().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_foreign_message().contains(123));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_foreign_message().count(123));
  // Value can be retrieved.
  EXPECT_EQ(10101, message.map_int32_foreign_message().at(123).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest,
     TestNonZeroMapEntriesExplicitMessageValuePopulatedInGenCode) {
  // Set nonzero values for key-value pairs and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().size());
  // Keys can be found.
  EXPECT_TRUE(message.map_int32_explicit_foreign_message().contains(456));
  // Values are counted properly.
  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().count(456));
  // Value can be retrieved.
  EXPECT_EQ(20202, message.map_int32_explicit_foreign_message().at(456).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestNonZeroStringMapEntriesHaveNoPresence) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_bytes())[9] = "hello";
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(bytes_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(bytes_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestNonZeroIntMapEntriesHaveNoPresence) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(enum_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(enum_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestNonZeroImplicitSubMessageMapEntriesHavePresence) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_foreign_message())[123].set_c(10101);

  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestNonZeroExplicitSubMessageMapEntriesHavePresence) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(explicit_msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestNonZeroStringMapEntriesPopulatedInReflection) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set nonzero values for key-value pairs and test that.
  (*message.mutable_map_int32_bytes())[9] = "hello";

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_bytes));
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(bytes_map_entry, MapEntryHasKey());
  EXPECT_THAT(bytes_map_entry, MapEntryHasValue());
  EXPECT_THAT(bytes_map_entry, MapEntryListFieldsSize(2));
}

TEST(NoFieldPresenceTest, TestNonZeroIntMapEntriesPopulatedInReflection) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set nonzero values for key-value pairs and test that.
  ASSERT_NE(0, static_cast<uint32_t>(FOREIGN_BAZ));
  (*message.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_enum));
  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(enum_map_entry, MapEntryHasKey());
  EXPECT_THAT(enum_map_entry, MapEntryHasValue());
  EXPECT_THAT(enum_map_entry, MapEntryListFieldsSize(2));
}

TEST(NoFieldPresenceTest,
     TestNonZeroSubMessageMapEntriesPopulatedInReflection) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  (*message.mutable_map_int32_foreign_message())[123].set_c(10101);

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_message));
  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(msg_map_entry, MapEntryHasValue());
  EXPECT_THAT(msg_map_entry, MapEntryListFieldsSize(2));

  // For value types that are messages, further test that the message fields
  // show up on reflection.
  EXPECT_TRUE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_foreign_message().at(123)));
}

TEST(NoFieldPresenceTest,
     TestNonZeroExplicitSubMessageMapEntriesPopulatedInReflection) {
  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  (*message.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  // Map entries show up on reflection.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_explicit_foreign_message));
  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // HasField for both key and value returns true.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasValue());
  EXPECT_THAT(explicit_msg_map_entry, MapEntryListFieldsSize(2));

  // For value types that are messages, further test that the message fields
  // show up on reflection.
  EXPECT_TRUE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_explicit_foreign_message().at(456)));
}

TEST(NoFieldPresenceTest, TestEmptyMapEntriesStringValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_bytes())[0];

  // Zero keys are valid entries in gencode.
  EXPECT_EQ(1, message.map_int32_bytes().size());
  EXPECT_TRUE(message.map_int32_bytes().contains(0));
  EXPECT_EQ(1, message.map_int32_bytes().count(0));
  EXPECT_EQ("", message.map_int32_bytes().at(0));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestEmptyMapEntriesIntValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_foreign_enum())[0];

  EXPECT_EQ(1, message.map_int32_foreign_enum().size());
  EXPECT_TRUE(message.map_int32_foreign_enum().contains(0));
  EXPECT_EQ(1, message.map_int32_foreign_enum().count(0));
  EXPECT_EQ(0, message.map_int32_foreign_enum().at(0));

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestEmptyMapEntriesMessageValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_foreign_message())[0];

  // ==== Gencode behaviour ====
  //
  // Zero keys are valid entries in gencode.
  EXPECT_EQ(1, message.map_int32_foreign_message().size());
  EXPECT_TRUE(message.map_int32_foreign_message().contains(0));
  EXPECT_EQ(1, message.map_int32_foreign_message().count(0));
  EXPECT_EQ(0, message.map_int32_foreign_message().at(0).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest,
     TestEmptyMapEntriesExplicitMessageValuePopulatedInGenCode) {
  // Set zero values for zero keys and test that.
  TestAllMapTypes message;
  (*message.mutable_map_int32_explicit_foreign_message())[0];

  // ==== Gencode behaviour ====
  //
  // Zero keys are valid entries in gencode.
  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().size());
  EXPECT_TRUE(message.map_int32_explicit_foreign_message().contains(0));
  EXPECT_EQ(1, message.map_int32_explicit_foreign_message().count(0));
  EXPECT_EQ(0, message.map_int32_explicit_foreign_message().at(0).c());

  // Note that `has_foo` APIs are not available for implicit presence fields.
  // So there is no way to check has_field behaviour in gencode.
}

TEST(NoFieldPresenceTest, TestEmptyStringMapEntriesHaveNoPresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_bytes())[0];
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(bytes_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(bytes_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestEmptyIntMapEntriesHaveNoPresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_enum())[0];
  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(enum_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Primitive types inherit presence semantics from the map itself.
  EXPECT_THAT(enum_map_entry, Not(MapEntryValueExplicitPresence()));
}

TEST(NoFieldPresenceTest, TestEmptySubMessageMapEntriesHavePresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_message));
  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestEmptyExplicitSubMessageMapEntriesHavePresence) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_explicit_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_explicit_foreign_message));
  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // Fields in map entries inherit field_presence from file defaults. If a map
  // is a "no presence" field, its key is also considered "no presence" from POV
  // of the descriptor. (Even though the key itself behaves like a normal index
  // with zeroes being valid indices). One day we will change this...
  EXPECT_THAT(explicit_msg_map_entry, Not(MapEntryKeyExplicitPresence()));

  // Message types always have presence in proto3.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryValueExplicitPresence());
}

TEST(NoFieldPresenceTest, TestEmptyStringMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_bytes =
      desc->FindFieldByName("map_int32_bytes");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_bytes())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_bytes));
  const google::protobuf::Message& bytes_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_bytes, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(bytes_map_entry, MapEntryHasKey());
  EXPECT_THAT(bytes_map_entry, MapEntryHasValue());
  EXPECT_THAT(bytes_map_entry, MapEntryListFieldsSize(2));
}

TEST(NoFieldPresenceTest, TestEmptyIntMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_enum =
      desc->FindFieldByName("map_int32_foreign_enum");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_enum())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_enum));
  const google::protobuf::Message& enum_map_entry =
      r->GetRepeatedMessage(message, field_map_int32_foreign_enum, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(enum_map_entry, MapEntryHasKey());
  EXPECT_THAT(enum_map_entry, MapEntryHasValue());
  EXPECT_THAT(enum_map_entry, MapEntryListFieldsSize(2));
}

TEST(NoFieldPresenceTest, TestEmptySubMessageMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_foreign_message =
      desc->FindFieldByName("map_int32_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_foreign_message));
  const google::protobuf::Message& msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_foreign_message, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(msg_map_entry, MapEntryHasValue());
  EXPECT_THAT(msg_map_entry, MapEntryListFieldsSize(2));

  // For value types that are messages, further test that the message fields
  // do not show up on reflection.
  EXPECT_FALSE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_foreign_message().at(0)));
}

TEST(NoFieldPresenceTest,
     TestEmptyExplicitSubMessageMapEntriesPopulatedInReflection) {
  // For map entries, test that you can set and read zero values.
  // Importantly this means that proto3 map fields behave like explicit
  // presence in reflection! i.e. they can be accessed even when zeroed.

  TestAllMapTypes message;
  const Reflection* r = message.GetReflection();
  const Descriptor* desc = message.GetDescriptor();

  const FieldDescriptor* field_map_int32_explicit_foreign_message =
      desc->FindFieldByName("map_int32_explicit_foreign_message");

  // Set zero values for zero keys and test that.
  (*message.mutable_map_int32_explicit_foreign_message())[0];

  // These map entries are considered valid in reflection APIs.
  EXPECT_EQ(1, r->FieldSize(message, field_map_int32_explicit_foreign_message));
  const google::protobuf::Message& explicit_msg_map_entry = r->GetRepeatedMessage(
      message, field_map_int32_explicit_foreign_message, /*index=*/0);

  // If map entries are truly "no presence", then they should not return true
  // for HasField!
  // However, the existing behavior is that map entries behave like
  // explicit-presence fields in reflection -- i.e. they must return true for
  // HasField even though they are zero.
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasKey());
  EXPECT_THAT(explicit_msg_map_entry, MapEntryHasValue());
  EXPECT_THAT(explicit_msg_map_entry, MapEntryListFieldsSize(2));

  // For value types that are messages, further test that the message fields
  // do not show up on reflection.
  EXPECT_FALSE(MapValueSubMessageHasFieldViaReflection(
      message.map_int32_explicit_foreign_message().at(0)));
}

// TODO: b/358616816 - `if constexpr` can be used here once C++17 is baseline.
template <typename T>
bool TestSerialize(const MessageLite& message, T* output);

template <>
bool TestSerialize<std::string>(const MessageLite& message,
                                std::string* output) {
  return message.SerializeToString(output);
}

template <>
bool TestSerialize<absl::Cord>(const MessageLite& message, absl::Cord* output) {
  return message.SerializeToString(output);
}

template <typename T>
class NoFieldPresenceMapSerializeTest : public testing::Test {
 public:
  T& GetOutputSinkRef() { return value_; }
  std::string GetOutput() { return std::string{value_}; }

 protected:
  // Cargo-culted from:
  // https://google.github.io/googletest/reference/testing.html#TYPED_TEST_SUITE
  T value_;
};

using SerializableOutputTypes = ::testing::Types<std::string, absl::Cord>;

// Providing the NameGenerator produces slightly more readable output in the
// test invocation summary (type names are displayed instead of numbers).
class NameGenerator {
 public:
  template <typename T>
  static std::string GetName(int) {
    if constexpr (std::is_same_v<T, std::string>) {
      return "string";
    } else if constexpr (std::is_same_v<T, absl::Cord>) {
      return "Cord";
    } else {
      static_assert(
          std::is_same_v<T, std::string> || std::is_same_v<T, absl::Cord>,
          "unsupported type");
    }
  }
};

TYPED_TEST_SUITE(NoFieldPresenceMapSerializeTest, SerializableOutputTypes,
                 NameGenerator);

TYPED_TEST(NoFieldPresenceMapSerializeTest,
           MapRoundTripNonZeroKeyNonZeroString) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_bytes())[9] = "hello";

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("hello", rt_msg.map_int32_bytes().at(9));
}

TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripNonZeroKeyNonZeroEnum) {
  TestAllMapTypes msg;
  ASSERT_NE(static_cast<uint32_t>(FOREIGN_BAZ), 0);
  (*msg.mutable_map_int32_foreign_enum())[99] = FOREIGN_BAZ;

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_foreign_enum(),
              UnorderedPointwise(Eq(), msg.map_int32_foreign_enum()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(FOREIGN_BAZ, rt_msg.map_int32_foreign_enum().at(99));
}

TYPED_TEST(NoFieldPresenceMapSerializeTest,
           MapRoundTripNonZeroKeyNonZeroMessage) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_foreign_message())[123].set_c(10101);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_foreign_message().at(123).c(),
            msg.map_int32_foreign_message().at(123).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(10101, rt_msg.map_int32_foreign_message().at(123).c());
}

TYPED_TEST(NoFieldPresenceMapSerializeTest,
           MapRoundTripNonZeroKeyNonZeroExplicitSubMessage) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_explicit_foreign_message())[456].set_c(20202);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_explicit_foreign_message().at(456).c(),
            msg.map_int32_explicit_foreign_message().at(456).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(20202, rt_msg.map_int32_explicit_foreign_message().at(456).c());

  // However, explicit presence messages expose a `has_foo` API.
  // Because map value is nonzero, they're expected to be present.
  EXPECT_TRUE(rt_msg.map_int32_explicit_foreign_message().at(456).has_c());
}

TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripZeroKeyNonZeroString) {
  // Because the map definitions all have int32 keys, testing one of them is
  // sufficient.
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_bytes())[0] = "hello";

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("hello", rt_msg.map_int32_bytes().at(0));
}

// Note: "zero value" in this case means that the value is zero, but still
// explicitly assigned.
TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripZeroKeyZeroString) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_bytes())[0] = "";

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("", rt_msg.map_int32_bytes().at(0));
}

TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripZeroKeyZeroEnum) {
  TestAllMapTypes msg;
  ASSERT_EQ(static_cast<uint32_t>(FOREIGN_FOO), 0);
  (*msg.mutable_map_int32_foreign_enum())[0] = FOREIGN_FOO;

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_foreign_enum(),
              UnorderedPointwise(Eq(), msg.map_int32_foreign_enum()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(FOREIGN_FOO, rt_msg.map_int32_foreign_enum().at(0));
}

TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripZeroKeyZeroMessage) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_foreign_message())[0].set_c(0);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_foreign_message().at(0).c(),
            msg.map_int32_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_foreign_message().at(0).c());
}

TYPED_TEST(NoFieldPresenceMapSerializeTest,
           MapRoundTripZeroKeyZeroExplicitMessage) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_explicit_foreign_message())[0].set_c(0);

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_explicit_foreign_message().at(0).c(),
            msg.map_int32_explicit_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_explicit_foreign_message().at(0).c());

  // However, explicit presence messages expose a `has_foo` API.
  // Because fields in an explicit message is explicitly set, they are expected
  // to be present.
  EXPECT_TRUE(rt_msg.map_int32_explicit_foreign_message().at(0).has_c());
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripZeroKeyDefaultString) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_bytes())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ("", rt_msg.map_int32_bytes().at(0));
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripZeroKeyDefaultEnum) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_foreign_enum())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  EXPECT_THAT(rt_msg.map_int32_bytes(),
              UnorderedPointwise(Eq(), msg.map_int32_bytes()));

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(FOREIGN_FOO, rt_msg.map_int32_foreign_enum().at(0));
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceMapSerializeTest, MapRoundTripZeroKeyDefaultMessage) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_foreign_message())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_foreign_message().at(0).c(),
            msg.map_int32_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_foreign_message().at(0).c());
}

// Note: "default value" in this case means that there is no explicit assignment
// to any value. Instead, map values are just created with operator[].
TYPED_TEST(NoFieldPresenceMapSerializeTest,
           MapRoundTripZeroKeyDefaultExplicitMessage) {
  TestAllMapTypes msg;
  (*msg.mutable_map_int32_explicit_foreign_message())[0];

  // Test that message can serialize.
  TypeParam& output_sink = this->GetOutputSinkRef();
  ASSERT_TRUE(TestSerialize(msg, &output_sink));
  // Maps with zero key or value fields are still serialized.
  ASSERT_FALSE(this->GetOutput().empty());

  // Test that message can roundtrip.
  TestAllMapTypes rt_msg;
  EXPECT_TRUE(rt_msg.ParseFromString(this->GetOutput()));
  // TODO: b/368089585 - write this better when we have access to EqualsProto.
  EXPECT_EQ(rt_msg.map_int32_explicit_foreign_message().at(0).c(),
            msg.map_int32_explicit_foreign_message().at(0).c());

  // The map behaviour is pretty much the same whether the key/value field is
  // zero or not.
  EXPECT_EQ(0, rt_msg.map_int32_explicit_foreign_message().at(0).c());

  // However, explicit presence messages expose a `has_foo` API.
  // Because fields in an explicit message is not set, they are not present.
  EXPECT_FALSE(rt_msg.map_int32_explicit_foreign_message().at(0).has_c());
}

}  // namespace
}  // namespace protobuf
}  // namespace google
