// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "binary_test_util.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "binary_wireformat.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "google/protobuf/descriptor.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"

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
using ::testing::IsSubsetOf;
using ::testing::MatchesRegex;

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

// Every ParamName() result must be usable as (part of) a gtest parameter name.
TEST(ParamNameTest, ResultsAreValidGtestNameComponents) {
  for (const Descriptor* message : AllTestMessageTypes()) {
    EXPECT_THAT(ParamName(message), MatchesRegex("[A-Za-z0-9]+"));
  }
  for (int i = 1; i <= FieldDescriptor::MAX_TYPE; ++i) {
    EXPECT_THAT(ParamName(static_cast<FieldDescriptor::Type>(i)),
                MatchesRegex("[A-Za-z0-9]+"));
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
