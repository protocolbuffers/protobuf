// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_test_util.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "conformance/binary_wireformat.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "google/protobuf/descriptor.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/testing/protobuf_test.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::edition_unstable::TestAllTypesEditionUnstable;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    ::protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    ::protobuf_test_messages::editions::proto3::TestAllTypesProto3;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::IsSubsetOf;
using ::testing::UnorderedElementsAreArray;

TEST(GetFieldForTypeTest, SingularInt32) {
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto3::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/false)
                ->number(),
            1);
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto2::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/false)
                ->number(),
            1);
}

TEST(GetFieldForTypeTest, RepeatedInt32) {
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto3::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/true)
                ->number(),
            31);
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto2::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/true)
                ->number(),
            31);
}

// repeated_int32 (31) is packed by default in proto3 but not in proto2, so the
// packed lookup only has to fall through to packed_int32 (75) for proto2.
TEST(GetFieldForTypeTest, PackedInt32) {
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto3::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/true,
                            Packedness::kPacked)
                ->number(),
            31);
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto2::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/true,
                            Packedness::kPacked)
                ->number(),
            75);
}

// ... and conversely the unpacked lookup only has to fall through to
// unpacked_int32 (89) for proto3.
TEST(GetFieldForTypeTest, UnpackedInt32) {
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto3::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/true,
                            Packedness::kUnpacked)
                ->number(),
            89);
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto2::descriptor(),
                            FieldDescriptor::TYPE_INT32, /*repeated=*/true,
                            Packedness::kUnpacked)
                ->number(),
            31);
}

TEST(GetFieldForTypeTest, Message) {
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto3::descriptor(),
                            FieldDescriptor::TYPE_MESSAGE, /*repeated=*/false)
                ->number(),
            18);
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto3::descriptor(),
                            FieldDescriptor::TYPE_MESSAGE, /*repeated=*/true)
                ->number(),
            48);
}

TEST(GetFieldForTypeTest, Enum) {
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto2::descriptor(),
                            FieldDescriptor::TYPE_ENUM, /*repeated=*/false)
                ->number(),
            21);
  EXPECT_EQ(GetFieldForType(*TestAllTypesProto2::descriptor(),
                            FieldDescriptor::TYPE_ENUM, /*repeated=*/true)
                ->number(),
            51);
}

PB_TEST(GetFieldForTypeTest,
        EditionsMessagesMatchTheirProto2AndProto3Originals) {
  const FieldDescriptor::Type type = PB_TEST_CHOOSE_VALUE(
      (FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_MESSAGE,
       FieldDescriptor::TYPE_ENUM, FieldDescriptor::TYPE_STRING),
      FieldTypeName);
  const bool repeated =
      PB_TEST_CHOOSE_VALUE((false, true), {"Singular", "Repeated"});
  EXPECT_EQ(
      GetFieldForType(*TestAllTypesProto3Editions::descriptor(), type, repeated)
          ->number(),
      GetFieldForType(*TestAllTypesProto3::descriptor(), type, repeated)
          ->number());
  EXPECT_EQ(
      GetFieldForType(*TestAllTypesProto2Editions::descriptor(), type, repeated)
          ->number(),
      GetFieldForType(*TestAllTypesProto2::descriptor(), type, repeated)
          ->number());
}

TEST(GetFieldForTypeDeathTest, MissingFieldIsFatal) {
  // TestAllTypesProto3 has no singular group field.
  EXPECT_DEATH(GetFieldForType(*TestAllTypesProto3::descriptor(),
                               FieldDescriptor::TYPE_GROUP, /*repeated=*/false),
               "Couldn't find field with type: Singular group for "
               "protobuf_test_messages.proto3.TestAllTypesProto3");
}

// Singular fields are never packed, so asking for a packed singular field can
// never succeed, even for a type the message does have a singular field of.
TEST(GetFieldForTypeDeathTest, PackedSingularIsFatal) {
  EXPECT_DEATH(GetFieldForType(*TestAllTypesProto3::descriptor(),
                               FieldDescriptor::TYPE_INT32, /*repeated=*/false,
                               Packedness::kPacked),
               "Couldn't find field with type: Singular Packed int32 for "
               "protobuf_test_messages.proto3.TestAllTypesProto3");
}

TEST(WireTypeForFieldTypeTest, AllTypes) {
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_DOUBLE),
            WireType::kFixed64);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_FLOAT),
            WireType::kFixed32);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_INT64),
            WireType::kVarint);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_UINT64),
            WireType::kVarint);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_INT32),
            WireType::kVarint);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_FIXED64),
            WireType::kFixed64);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_FIXED32),
            WireType::kFixed32);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_BOOL),
            WireType::kVarint);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_STRING),
            WireType::kLengthPrefixed);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_GROUP),
            WireType::kStartGroup);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_MESSAGE),
            WireType::kLengthPrefixed);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_BYTES),
            WireType::kLengthPrefixed);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_UINT32),
            WireType::kVarint);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_ENUM),
            WireType::kVarint);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_SFIXED32),
            WireType::kFixed32);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_SFIXED64),
            WireType::kFixed64);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_SINT32),
            WireType::kVarint);
  EXPECT_EQ(WireTypeForFieldType(FieldDescriptor::TYPE_SINT64),
            WireType::kVarint);
}

TEST(UpperCaseTypeNameTest, MatchesLegacyTestNames) {
  EXPECT_EQ(UpperCaseTypeName(FieldDescriptor::TYPE_INT32), "INT32");
  EXPECT_EQ(UpperCaseTypeName(FieldDescriptor::TYPE_SFIXED64), "SFIXED64");
  EXPECT_EQ(UpperCaseTypeName(FieldDescriptor::TYPE_MESSAGE), "MESSAGE");
  EXPECT_EQ(UpperCaseTypeName(FieldDescriptor::TYPE_BYTES), "BYTES");
}

// Enumerates FieldDescriptor::Type independently of the table.
TEST(AllFieldTypesExceptGroupTest, IsEveryTypeButGroup) {
  std::set<FieldDescriptor::Type> all_but_group;
  for (int i = 1; i <= FieldDescriptor::MAX_TYPE; ++i) {
    all_but_group.insert(static_cast<FieldDescriptor::Type>(i));
  }
  all_but_group.erase(FieldDescriptor::TYPE_GROUP);
  EXPECT_THAT(AllFieldTypesExceptGroup(),
              UnorderedElementsAreArray(all_but_group));
}

// Every test message has a singular and a repeated field of every entry's type
// (GetFieldForType() would check-fail otherwise).
PB_TEST(AllFieldTypesExceptGroupTest,
        EveryMessageTypeHasSingularAndRepeatedField) {
  const Descriptor& message =
      *PB_TEST_CHOOSE_VALUE(AllTestMessageTypes(), MessageTypeName);
  const FieldDescriptor::Type type =
      PB_TEST_CHOOSE_VALUE(AllFieldTypesExceptGroup(), FieldTypeName);
  EXPECT_EQ(GetFieldForType(message, type, /*repeated=*/false)->type(), type);
  EXPECT_EQ(GetFieldForType(message, type, /*repeated=*/true)->type(), type);
}

TEST(PackableFieldTypesTest, IsThePackableSubsetInTableOrder) {
  std::vector<FieldDescriptor::Type> packable;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (FieldDescriptor::IsTypePackable(type)) packable.push_back(type);
  }
  EXPECT_THAT(PackableFieldTypes(), ElementsAreArray(packable));
}

// Every test message has both a [packed = true] and a [packed = false]
// repeated field of every packable type, which is what the packed-field tests
// select with Packedness::kPacked / kUnpacked.
PB_TEST(PackableFieldTypesTest, EveryMessageTypeHasPackedAndUnpackedField) {
  const Descriptor& message =
      *PB_TEST_CHOOSE_VALUE(AllTestMessageTypes(), MessageTypeName);
  const FieldDescriptor::Type type =
      PB_TEST_CHOOSE_VALUE(PackableFieldTypes(), FieldTypeName);
  const FieldDescriptor* packed =
      GetFieldForType(message, type, /*repeated=*/true, Packedness::kPacked);
  const FieldDescriptor* unpacked =
      GetFieldForType(message, type, /*repeated=*/true, Packedness::kUnpacked);
  EXPECT_TRUE(packed->is_packed());
  EXPECT_FALSE(unpacked->is_packed());
}

TEST(LengthDelimitedFieldTypesTest, IsTheNonPackableSubsetInTableOrder) {
  std::vector<FieldDescriptor::Type> non_packable;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (!FieldDescriptor::IsTypePackable(type)) non_packable.push_back(type);
  }
  EXPECT_THAT(LengthDelimitedFieldTypes(), ElementsAreArray(non_packable));
}

PB_TEST(LengthDelimitedFieldTypesTest, AreEncodedLengthPrefixed) {
  const FieldDescriptor::Type type =
      PB_TEST_CHOOSE_VALUE(LengthDelimitedFieldTypes(), FieldTypeName);
  EXPECT_EQ(WireTypeForFieldType(type), WireType::kLengthPrefixed);
}

TEST(AllTestMessageTypesTest, LegacyOrder) {
  EXPECT_THAT(AllTestMessageTypes(),
              ElementsAre(TestAllTypesProto3::descriptor(),
                          TestAllTypesProto2::descriptor(),
                          TestAllTypesProto3Editions::descriptor(),
                          TestAllTypesProto2Editions::descriptor()));
}

TEST(Proto2TestMessageTypesTest, LegacyOrder) {
  EXPECT_THAT(Proto2TestMessageTypes(),
              ElementsAre(TestAllTypesProto2::descriptor(),
                          TestAllTypesProto2Editions::descriptor()));
}

TEST(Proto2TestMessageTypesTest, IsSubsetOfAllTestMessageTypes) {
  EXPECT_THAT(Proto2TestMessageTypes(), IsSubsetOf(AllTestMessageTypes()));
}

TEST(MessageTypeNameTest, IsTheEditionIdentifierWithoutUnderscore) {
  EXPECT_EQ(MessageTypeName(TestAllTypesProto3::descriptor(), 0), "Proto3");
  EXPECT_EQ(MessageTypeName(TestAllTypesProto2::descriptor(), 1), "Proto2");
  EXPECT_EQ(MessageTypeName(TestAllTypesProto3Editions::descriptor(), 2),
            "EditionsProto3");
  EXPECT_EQ(MessageTypeName(TestAllTypesProto2Editions::descriptor(), 3),
            "EditionsProto2");
  EXPECT_EQ(MessageTypeName(TestAllTypesEditionUnstable::descriptor(), 0),
            "EditionUnstable");
}

TEST(FieldTypeNameTest, IsTheUpperCaseTypeName) {
  EXPECT_EQ(FieldTypeName(FieldDescriptor::TYPE_INT32, 0), "INT32");
  EXPECT_EQ(FieldTypeName(FieldDescriptor::TYPE_SFIXED64, 1), "SFIXED64");
  EXPECT_EQ(FieldTypeName(FieldDescriptor::TYPE_MESSAGE, 2), "MESSAGE");
}

// Matches a non-empty string of ASCII letters and digits.  (Not
// MatchesRegex("[A-Za-z0-9]+"): gtest's own regex engine, which it falls back
// to on Windows, has no bracket expressions.)
MATCHER(IsAlphanumeric, negation ? "isn't alphanumeric" : "is alphanumeric") {
  return !arg.empty() && std::all_of(arg.begin(), arg.end(), [](char c) {
    return absl::ascii_isalnum(c);
  });
}

// Every printer result must be usable as (part of) a gtest name.
TEST(DimensionPrinterTest, ResultsAreValidGtestNameComponents) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    EXPECT_THAT(MessageTypeName(message, 0), IsAlphanumeric());
  }
  for (int i = 1; i <= FieldDescriptor::MAX_TYPE; ++i) {
    EXPECT_THAT(FieldTypeName(static_cast<FieldDescriptor::Type>(i), 0),
                IsAlphanumeric());
  }
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
