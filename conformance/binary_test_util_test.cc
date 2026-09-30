// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_test_util.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "conformance/binary_wireformat.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "google/protobuf/descriptor.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"

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

TEST(GetFieldForTypeTest, EditionsMessagesMatchTheirProto2AndProto3Originals) {
  for (FieldDescriptor::Type type :
       {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_MESSAGE,
        FieldDescriptor::TYPE_ENUM, FieldDescriptor::TYPE_STRING}) {
    for (bool repeated : {false, true}) {
      SCOPED_TRACE(absl::StrCat(repeated ? "repeated " : "singular ",
                                FieldDescriptor::TypeName(type)));
      EXPECT_EQ(
          GetFieldForType(*TestAllTypesProto3Editions::descriptor(), type,
                          repeated)
              ->number(),
          GetFieldForType(*TestAllTypesProto3::descriptor(), type, repeated)
              ->number());
      EXPECT_EQ(
          GetFieldForType(*TestAllTypesProto2Editions::descriptor(), type,
                          repeated)
              ->number(),
          GetFieldForType(*TestAllTypesProto2::descriptor(), type, repeated)
              ->number());
    }
  }
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

TEST(GetFieldForMapTypeTest, LegacyFieldNumbers) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    SCOPED_TRACE(message->full_name());
    EXPECT_EQ(GetFieldForMapType(*message, FieldDescriptor::TYPE_INT32,
                                 FieldDescriptor::TYPE_INT32)
                  ->number(),
              56);
    EXPECT_EQ(GetFieldForMapType(*message, FieldDescriptor::TYPE_STRING,
                                 FieldDescriptor::TYPE_STRING)
                  ->number(),
              69);
    EXPECT_EQ(GetFieldForMapType(*message, FieldDescriptor::TYPE_STRING,
                                 FieldDescriptor::TYPE_MESSAGE)
                  ->number(),
              71);
  }
}

TEST(GetFieldForMapTypeDeathTest, MissingFieldIsFatal) {
  // There is no map<bytes, ...> (bytes aren't valid map keys).
  EXPECT_DEATH(GetFieldForMapType(*TestAllTypesProto3::descriptor(),
                                  FieldDescriptor::TYPE_BYTES,
                                  FieldDescriptor::TYPE_BYTES),
               "Couldn't find map field with type: bytes and bytes for "
               "protobuf_test_messages.proto3.TestAllTypesProto3");
}

TEST(GetFieldForOneofTypeTest, LegacyFieldNumbers) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    SCOPED_TRACE(message->full_name());
    EXPECT_EQ(
        GetFieldForOneofType(*message, FieldDescriptor::TYPE_UINT32)->number(),
        111);
    EXPECT_EQ(
        GetFieldForOneofType(*message, FieldDescriptor::TYPE_MESSAGE)->number(),
        112);
    EXPECT_EQ(
        GetFieldForOneofType(*message, FieldDescriptor::TYPE_ENUM)->number(),
        119);
  }
}

// With `exclusive` the first oneof member of any *other* type is returned,
// which is oneof_uint32 (111) unless that is the type being excluded.
TEST(GetFieldForOneofTypeTest, Exclusive) {
  const Descriptor& message = *TestAllTypesProto3::descriptor();
  EXPECT_EQ(GetFieldForOneofType(message, FieldDescriptor::TYPE_MESSAGE,
                                 /*exclusive=*/true)
                ->number(),
            111);
  EXPECT_EQ(GetFieldForOneofType(message, FieldDescriptor::TYPE_UINT32,
                                 /*exclusive=*/true)
                ->number(),
            112);
}

TEST(GetFieldForOneofTypeDeathTest, MissingFieldIsFatal) {
  // No oneof member is a group.
  EXPECT_DEATH(GetFieldForOneofType(*TestAllTypesProto2::descriptor(),
                                    FieldDescriptor::TYPE_GROUP),
               "Couldn't find oneof field with type: group for "
               "protobuf_test_messages.proto2.TestAllTypesProto2");
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

TEST(GetDefaultValueTest, Varints) {
  for (FieldDescriptor::Type type :
       {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_INT64,
        FieldDescriptor::TYPE_UINT32, FieldDescriptor::TYPE_UINT64,
        FieldDescriptor::TYPE_ENUM, FieldDescriptor::TYPE_BOOL,
        FieldDescriptor::TYPE_SINT32, FieldDescriptor::TYPE_SINT64}) {
    SCOPED_TRACE(FieldDescriptor::TypeName(type));
    EXPECT_EQ(GetDefaultValue(type), Varint(0));
  }
}

TEST(GetDefaultValueTest, FixedWidth) {
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_FIXED32), Fixed32(0));
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_SFIXED32), Fixed32(0));
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_FLOAT), Float(0));
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_FIXED64), Fixed64(0));
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_SFIXED64), Fixed64(0));
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_DOUBLE), Double(0));
}

TEST(GetDefaultValueTest, LengthPrefixed) {
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_STRING), LengthPrefixed(""));
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_BYTES), LengthPrefixed(""));
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_MESSAGE), LengthPrefixed(""));
}

TEST(GetDefaultValueTest, GroupIsEmpty) {
  EXPECT_EQ(GetDefaultValue(FieldDescriptor::TYPE_GROUP), Wire());
}

TEST(GetNonDefaultValueTest, Varints) {
  for (FieldDescriptor::Type type :
       {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_INT64,
        FieldDescriptor::TYPE_UINT32, FieldDescriptor::TYPE_UINT64,
        FieldDescriptor::TYPE_ENUM, FieldDescriptor::TYPE_BOOL}) {
    SCOPED_TRACE(FieldDescriptor::TypeName(type));
    EXPECT_EQ(GetNonDefaultValue(type), Varint(1));
  }
  // Zig-zag encoded, so 1 becomes 2.
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_SINT32), Varint(2));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_SINT64), Varint(2));
}

TEST(GetNonDefaultValueTest, FixedWidth) {
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_FIXED32), Fixed32(1));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_SFIXED32), Fixed32(1));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_FLOAT), Float(1));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_FIXED64), Fixed64(1));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_SFIXED64), Fixed64(1));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_DOUBLE), Double(1));
}

TEST(GetNonDefaultValueTest, LengthPrefixed) {
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_STRING),
            LengthPrefixed("a"));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_BYTES),
            LengthPrefixed("a"));
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_MESSAGE),
            LengthPrefixed(VarintField(1, 1234)));
}

TEST(GetNonDefaultValueTest, GroupIsEmpty) {
  EXPECT_EQ(GetNonDefaultValue(FieldDescriptor::TYPE_GROUP), Wire());
}

TEST(GetNonDefaultValueTest, DiffersFromDefaultValue) {
  for (FieldDescriptor::Type type :
       {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_SINT64,
        FieldDescriptor::TYPE_FIXED32, FieldDescriptor::TYPE_DOUBLE,
        FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_MESSAGE}) {
    SCOPED_TRACE(FieldDescriptor::TypeName(type));
    EXPECT_NE(GetNonDefaultValue(type), GetDefaultValue(type));
  }
}

// Matches a Wire that the generated class of `message` parses into a message
// matching `matcher`.  The merge-test inputs are checked against the C++
// generated classes, whose merge semantics are the reference the conformance
// tests hold others to.
MATCHER_P2(ParsesAs, message, matcher, "") {
  std::unique_ptr<Message> parsed(
      MessageFactory::generated_factory()->GetPrototype(message)->New());
  if (!parsed->ParseFromString(arg.data())) {
    *result_listener << "which doesn't parse as " << message->full_name();
    return false;
  }
  return ExplainMatchResult(matcher, *parsed, result_listener);
}

TEST(RepeatedScalarMessageMergeInputTest, MergesTheTwoSubmessages) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    SCOPED_TRACE(message->full_name());
    EXPECT_THAT(
        RepeatedScalarMessageMergeInput(*message),
        ParsesAs(message, EqualsProto(R"pb(optional_nested_message {
                                             corecursive {
                                               optional_int32: 4321
                                               optional_int64: 1234
                                               optional_uint32: 4321
                                               repeated_int32: [ 1234, 4321 ]
                                             }
                                           })pb")));
  }
}

TEST(MapMessageValueMergeDataTest, LastEntryWins) {
  constexpr char kExpected[] =
      R"pb(map_string_nested_message {
             key: ""
             value { corecursive { optional_int64: 1 repeated_int32: 1 } }
           })pb";
  for (const Descriptor* message : AllTestMessageTypes()) {
    SCOPED_TRACE(message->full_name());
    const MergeTestData data = MapMessageValueMergeData(*message);
    EXPECT_THAT(data.expected, ParsesAs(message, EqualsProto(kExpected)));
    EXPECT_THAT(data.input, ParsesAs(message, EqualsProto(kExpected)));
  }
}

TEST(OneofMessageMergeDataTest, MergesTheTwoSubmessages) {
  constexpr char kExpected[] = R"pb(oneof_nested_message {
                                      corecursive {
                                        optional_int32: 1
                                        optional_int64: 1
                                        unpacked_int32: [ 1, 1 ]
                                      }
                                    })pb";
  for (const Descriptor* message : AllTestMessageTypes()) {
    SCOPED_TRACE(message->full_name());
    const MergeTestData data = OneofMessageMergeData(*message);
    EXPECT_THAT(data.expected, ParsesAs(message, EqualsProto(kExpected)));
    EXPECT_THAT(data.input, ParsesAs(message, EqualsProto(kExpected)));
  }
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
TEST(AllFieldTypesExceptGroupTest,
     EveryMessageTypeHasSingularAndRepeatedField) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
      SCOPED_TRACE(absl::StrCat(message->full_name(), " ",
                                FieldDescriptor::TypeName(type)));
      EXPECT_EQ(GetFieldForType(*message, type, /*repeated=*/false)->type(),
                type);
      EXPECT_EQ(GetFieldForType(*message, type, /*repeated=*/true)->type(),
                type);
    }
  }
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
TEST(PackableFieldTypesTest, EveryMessageTypeHasPackedAndUnpackedField) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    for (FieldDescriptor::Type type : PackableFieldTypes()) {
      SCOPED_TRACE(absl::StrCat(message->full_name(), " ",
                                FieldDescriptor::TypeName(type)));
      const FieldDescriptor* packed = GetFieldForType(
          *message, type, /*repeated=*/true, Packedness::kPacked);
      const FieldDescriptor* unpacked = GetFieldForType(
          *message, type, /*repeated=*/true, Packedness::kUnpacked);
      EXPECT_TRUE(packed->is_packed());
      EXPECT_FALSE(unpacked->is_packed());
    }
  }
}

TEST(LengthDelimitedFieldTypesTest, IsTheNonPackableSubsetInTableOrder) {
  std::vector<FieldDescriptor::Type> non_packable;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (!FieldDescriptor::IsTypePackable(type)) non_packable.push_back(type);
  }
  EXPECT_THAT(LengthDelimitedFieldTypes(), ElementsAreArray(non_packable));
}

TEST(LengthDelimitedFieldTypesTest, AreEncodedLengthPrefixed) {
  for (FieldDescriptor::Type type : LengthDelimitedFieldTypes()) {
    SCOPED_TRACE(FieldDescriptor::TypeName(type));
    EXPECT_EQ(WireTypeForFieldType(type), WireType::kLengthPrefixed);
  }
}

TEST(AllTestMessageTypesTest, LegacyOrder) {
  EXPECT_THAT(AllTestMessageTypes(),
              ElementsAre(TestAllTypesProto3::descriptor(),
                          TestAllTypesProto2::descriptor(),
                          TestAllTypesProto3Editions::descriptor(),
                          TestAllTypesProto2Editions::descriptor()));
}

TEST(Proto3TestMessageTypesTest, LegacyOrder) {
  EXPECT_THAT(Proto3TestMessageTypes(),
              ElementsAre(TestAllTypesProto3::descriptor(),
                          TestAllTypesProto3Editions::descriptor()));
}

TEST(Proto3TestMessageTypesTest, IsSubsetOfAllTestMessageTypes) {
  EXPECT_THAT(Proto3TestMessageTypes(), IsSubsetOf(AllTestMessageTypes()));
}

TEST(Proto2TestMessageTypesTest, LegacyOrder) {
  EXPECT_THAT(Proto2TestMessageTypes(),
              ElementsAre(TestAllTypesProto2::descriptor(),
                          TestAllTypesProto2Editions::descriptor()));
}

TEST(Proto2TestMessageTypesTest, IsSubsetOfAllTestMessageTypes) {
  EXPECT_THAT(Proto2TestMessageTypes(), IsSubsetOf(AllTestMessageTypes()));
}

TEST(Proto2TestMessageTypesTest, TogetherWithProto3CoversAllTestMessageTypes) {
  std::vector<const Descriptor*> both = Proto2TestMessageTypes();
  for (const Descriptor* message : Proto3TestMessageTypes()) {
    both.push_back(message);
  }
  EXPECT_THAT(both, UnorderedElementsAreArray(AllTestMessageTypes()));
}

TEST(ParamNameTest, StringKeepsOnlyLettersAndDigits) {
  EXPECT_EQ(ParamName("Proto3"), "Proto3");
  EXPECT_EQ(ParamName("Editions_Proto2"), "EditionsProto2");
  EXPECT_EQ(ParamName("a-b.c d/e"), "abcde");
  const std::string underscored = "x_1";
  EXPECT_EQ(ParamName(underscored), "x1");
}

TEST(ParamNameTest, MessageTypeIsTheEditionIdentifierWithoutUnderscore) {
  EXPECT_EQ(ParamName(TestAllTypesProto3::descriptor()), "Proto3");
  EXPECT_EQ(ParamName(TestAllTypesProto2::descriptor()), "Proto2");
  EXPECT_EQ(ParamName(TestAllTypesProto3Editions::descriptor()),
            "EditionsProto3");
  EXPECT_EQ(ParamName(TestAllTypesProto2Editions::descriptor()),
            "EditionsProto2");
  EXPECT_EQ(ParamName(TestAllTypesEditionUnstable::descriptor()),
            "EditionUnstable");
}

TEST(ParamNameTest, FieldTypeIsTheUpperCaseTypeName) {
  EXPECT_EQ(ParamName(FieldDescriptor::TYPE_INT32), "INT32");
  EXPECT_EQ(ParamName(FieldDescriptor::TYPE_SFIXED64), "SFIXED64");
  EXPECT_EQ(ParamName(FieldDescriptor::TYPE_MESSAGE), "MESSAGE");
}

TEST(ParamNameTest, IntIsDecimal) {
  EXPECT_EQ(ParamName(0), "0");
  EXPECT_EQ(ParamName(7), "7");
  EXPECT_EQ(ParamName(1234), "1234");
}

// Matches a non-empty string of ASCII letters and digits.  (Not
// MatchesRegex("[A-Za-z0-9]+"): gtest's own regex engine, which it falls back
// to on Windows, has no bracket expressions.)
MATCHER(IsAlphanumeric, negation ? "isn't alphanumeric" : "is alphanumeric") {
  return !arg.empty() && std::all_of(arg.begin(), arg.end(), [](char c) {
    return absl::ascii_isalnum(c);
  });
}

// Every ParamName() result must be usable as (part of) a gtest parameter name.
TEST(ParamNameTest, ResultsAreValidGtestNameComponents) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    EXPECT_THAT(ParamName(message), IsAlphanumeric());
  }
  for (int i = 1; i <= FieldDescriptor::MAX_TYPE; ++i) {
    EXPECT_THAT(ParamName(static_cast<FieldDescriptor::Type>(i)),
                IsAlphanumeric());
  }
}

TEST(ParamNameDeathTest, StringWithoutLettersOrDigitsIsFatal) {
  EXPECT_DEATH(ParamName("_-_"), "No letters or digits in parameter name");
}

TEST(ParamNameDeathTest, NegativeIntIsFatal) {
  EXPECT_DEATH(ParamName(-1), "Negative parameters have no valid gtest name");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
