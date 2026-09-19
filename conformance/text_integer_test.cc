// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for integer fields: the extreme values of
// each integer type in decimal, hex and octal, and rejection of values just
// past them.  This replaces the integer blocks of the legacy
// TextFormatConformanceTestSuiteImpl<M>::RunAllTests(); the test names and
// the requests sent to the testee are identical to the legacy ones.
//
// The legacy suite ran RunAllTests() only for TestAllTypesProto3, which its
// RunSuiteImpl() instantiated in its proto3 and editions flavours, so these
// run over Proto3TestMessageTypes() (like HelloWorld).

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using TextIntegerTest = MessageTypeConformanceTest;

// Extreme values in decimal.
TEST_P(TextIntegerTest, Int32FieldMaxValue) {
  EXPECT_THAT(
      Testee("Int32FieldMaxValue")
          .ParseText(message(), R"pb(optional_int32: 2147483647)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2147483647)pb"))));
  EXPECT_THAT(
      Testee("Int32FieldMaxValue")
          .ParseText(message(), R"pb(optional_int32: 2147483647)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2147483647)pb"))));
}

TEST_P(TextIntegerTest, Int32FieldMinValue) {
  EXPECT_THAT(Testee("Int32FieldMinValue")
                  .ParseText(message(), R"pb(optional_int32: -2147483648)pb")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_int32: -2147483648)pb"))));
  EXPECT_THAT(Testee("Int32FieldMinValue")
                  .ParseText(message(), R"pb(optional_int32: -2147483648)pb")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_int32: -2147483648)pb"))));
}

TEST_P(TextIntegerTest, Uint32FieldMaxValue) {
  EXPECT_THAT(Testee("Uint32FieldMaxValue")
                  .ParseText(message(), R"pb(optional_uint32: 4294967295)pb")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_uint32: 4294967295)pb"))));
  EXPECT_THAT(Testee("Uint32FieldMaxValue")
                  .ParseText(message(), R"pb(optional_uint32: 4294967295)pb")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_uint32: 4294967295)pb"))));
}

TEST_P(TextIntegerTest, Int64FieldMaxValue) {
  EXPECT_THAT(
      Testee("Int64FieldMaxValue")
          .ParseText(message(), R"pb(optional_int64: 9223372036854775807)pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(optional_int64: 9223372036854775807)pb"))));
  EXPECT_THAT(
      Testee("Int64FieldMaxValue")
          .ParseText(message(), R"pb(optional_int64: 9223372036854775807)pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(optional_int64: 9223372036854775807)pb"))));
}

// TODO: Write the plain-string inputs below as R"pb(...)pb" raw
// strings once the migration's request goldens are gone.  The goldens pin the
// exact request bytes, and clang-format would reformat the whitespace of these
// inputs inside a pb raw string.
TEST_P(TextIntegerTest, Int64FieldMinValue) {
  EXPECT_THAT(Testee("Int64FieldMinValue")
                  .ParseText(message(), "optional_int64: -9223372036854775808")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto("optional_int64: -9223372036854775808"))));
  EXPECT_THAT(Testee("Int64FieldMinValue")
                  .ParseText(message(), "optional_int64: -9223372036854775808")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto("optional_int64: -9223372036854775808"))));
}

TEST_P(TextIntegerTest, Uint64FieldMaxValue) {
  EXPECT_THAT(Testee("Uint64FieldMaxValue")
                  .ParseText(message(), "optional_uint64: 18446744073709551615")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto("optional_uint64: 18446744073709551615"))));
  EXPECT_THAT(Testee("Uint64FieldMaxValue")
                  .ParseText(message(), "optional_uint64: 18446744073709551615")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto("optional_uint64: 18446744073709551615"))));
}

// Extreme values in hex; the responses are compared to the decimal form.
TEST_P(TextIntegerTest, Int32FieldMaxValueHex) {
  EXPECT_THAT(
      Testee("Int32FieldMaxValueHex")
          .ParseText(message(), R"pb(optional_int32: 0x7FFFFFFF)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2147483647)pb"))));
  EXPECT_THAT(
      Testee("Int32FieldMaxValueHex")
          .ParseText(message(), R"pb(optional_int32: 0x7FFFFFFF)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2147483647)pb"))));
}

TEST_P(TextIntegerTest, Int32FieldMinValueHex) {
  EXPECT_THAT(Testee("Int32FieldMinValueHex")
                  .ParseText(message(), R"pb(optional_int32: -0x80000000)pb")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_int32: -2147483648)pb"))));
  EXPECT_THAT(Testee("Int32FieldMinValueHex")
                  .ParseText(message(), R"pb(optional_int32: -0x80000000)pb")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_int32: -2147483648)pb"))));
}

TEST_P(TextIntegerTest, Uint32FieldMaxValueHex) {
  EXPECT_THAT(Testee("Uint32FieldMaxValueHex")
                  .ParseText(message(), R"pb(optional_uint32: 0xFFFFFFFF)pb")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_uint32: 4294967295)pb"))));
  EXPECT_THAT(Testee("Uint32FieldMaxValueHex")
                  .ParseText(message(), R"pb(optional_uint32: 0xFFFFFFFF)pb")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_uint32: 4294967295)pb"))));
}

TEST_P(TextIntegerTest, Int64FieldMaxValueHex) {
  EXPECT_THAT(
      Testee("Int64FieldMaxValueHex")
          .ParseText(message(), R"pb(optional_int64: 0x7FFFFFFFFFFFFFFF)pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(optional_int64: 9223372036854775807)pb"))));
  EXPECT_THAT(
      Testee("Int64FieldMaxValueHex")
          .ParseText(message(), R"pb(optional_int64: 0x7FFFFFFFFFFFFFFF)pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(optional_int64: 9223372036854775807)pb"))));
}

TEST_P(TextIntegerTest, Int64FieldMinValueHex) {
  EXPECT_THAT(
      Testee("Int64FieldMinValueHex")
          .ParseText(message(), R"pb(optional_int64: -0x8000000000000000)pb")
          .SerializeBinary(),
      Yields(
          WhenParsed(EqualsTextProto("optional_int64: -9223372036854775808"))));
  EXPECT_THAT(
      Testee("Int64FieldMinValueHex")
          .ParseText(message(), R"pb(optional_int64: -0x8000000000000000)pb")
          .SerializeText(),
      Yields(
          WhenParsed(EqualsTextProto("optional_int64: -9223372036854775808"))));
}

TEST_P(TextIntegerTest, Uint64FieldMaxValueHex) {
  EXPECT_THAT(
      Testee("Uint64FieldMaxValueHex")
          .ParseText(message(), R"pb(optional_uint64: 0xFFFFFFFFFFFFFFFF)pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto("optional_uint64: 18446744073709551615"))));
  EXPECT_THAT(
      Testee("Uint64FieldMaxValueHex")
          .ParseText(message(), R"pb(optional_uint64: 0xFFFFFFFFFFFFFFFF)pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto("optional_uint64: 18446744073709551615"))));
}

// Extreme values in octal.
TEST_P(TextIntegerTest, Int32FieldMaxValueOctal) {
  EXPECT_THAT(
      Testee("Int32FieldMaxValueOctal")
          .ParseText(message(), R"pb(optional_int32: 017777777777)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2147483647)pb"))));
  EXPECT_THAT(
      Testee("Int32FieldMaxValueOctal")
          .ParseText(message(), R"pb(optional_int32: 017777777777)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2147483647)pb"))));
}

TEST_P(TextIntegerTest, Int32FieldMinValueOctal) {
  EXPECT_THAT(Testee("Int32FieldMinValueOctal")
                  .ParseText(message(), R"pb(optional_int32: -020000000000)pb")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_int32: -2147483648)pb"))));
  EXPECT_THAT(Testee("Int32FieldMinValueOctal")
                  .ParseText(message(), R"pb(optional_int32: -020000000000)pb")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_int32: -2147483648)pb"))));
}

TEST_P(TextIntegerTest, Uint32FieldMaxValueOctal) {
  EXPECT_THAT(Testee("Uint32FieldMaxValueOctal")
                  .ParseText(message(), R"pb(optional_uint32: 037777777777)pb")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_uint32: 4294967295)pb"))));
  EXPECT_THAT(Testee("Uint32FieldMaxValueOctal")
                  .ParseText(message(), R"pb(optional_uint32: 037777777777)pb")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto(R"pb(optional_uint32: 4294967295)pb"))));
}

TEST_P(TextIntegerTest, Int64FieldMaxValueOctal) {
  EXPECT_THAT(
      Testee("Int64FieldMaxValueOctal")
          .ParseText(message(), R"pb(optional_int64: 0777777777777777777777)pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(optional_int64: 9223372036854775807)pb"))));
  EXPECT_THAT(
      Testee("Int64FieldMaxValueOctal")
          .ParseText(message(), R"pb(optional_int64: 0777777777777777777777)pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(optional_int64: 9223372036854775807)pb"))));
}

TEST_P(TextIntegerTest, Int64FieldMinValueOctal) {
  EXPECT_THAT(
      Testee("Int64FieldMinValueOctal")
          .ParseText(message(), "optional_int64: -01000000000000000000000")
          .SerializeBinary(),
      Yields(
          WhenParsed(EqualsTextProto("optional_int64: -9223372036854775808"))));
  EXPECT_THAT(
      Testee("Int64FieldMinValueOctal")
          .ParseText(message(), "optional_int64: -01000000000000000000000")
          .SerializeText(),
      Yields(
          WhenParsed(EqualsTextProto("optional_int64: -9223372036854775808"))));
}

TEST_P(TextIntegerTest, Uint64FieldMaxValueOctal) {
  EXPECT_THAT(
      Testee("Uint64FieldMaxValueOctal")
          .ParseText(message(), "optional_uint64: 01777777777777777777777")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto("optional_uint64: 18446744073709551615"))));
  EXPECT_THAT(
      Testee("Uint64FieldMaxValueOctal")
          .ParseText(message(), "optional_uint64: 01777777777777777777777")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto("optional_uint64: 18446744073709551615"))));
}

// Parsers reject out-of-bound integer values.
TEST_P(TextIntegerTest, Int32FieldTooLarge) {
  EXPECT_THAT(Testee("Int32FieldTooLarge")
                  .ParseText(message(), R"pb(optional_int32: 2147483648)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int32FieldTooSmall) {
  EXPECT_THAT(Testee("Int32FieldTooSmall")
                  .ParseText(message(), R"pb(optional_int32: -2147483649)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Uint32FieldTooLarge) {
  EXPECT_THAT(Testee("Uint32FieldTooLarge")
                  .ParseText(message(), R"pb(optional_uint32: 4294967296)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int64FieldTooLarge) {
  EXPECT_THAT(
      Testee("Int64FieldTooLarge")
          .ParseText(message(), R"pb(optional_int64: 9223372036854775808)pb")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int64FieldTooSmall) {
  EXPECT_THAT(Testee("Int64FieldTooSmall")
                  .ParseText(message(), "optional_int64: -9223372036854775809")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Uint64FieldTooLarge) {
  EXPECT_THAT(Testee("Uint64FieldTooLarge")
                  .ParseText(message(), "optional_uint64: 18446744073709551616")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ... in hex
TEST_P(TextIntegerTest, Int32FieldTooLargeHex) {
  EXPECT_THAT(Testee("Int32FieldTooLargeHex")
                  .ParseText(message(), R"pb(optional_int32: 0x80000000)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int32FieldTooSmallHex) {
  EXPECT_THAT(Testee("Int32FieldTooSmallHex")
                  .ParseText(message(), R"pb(optional_int32: -0x80000001)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Uint32FieldTooLargeHex) {
  EXPECT_THAT(Testee("Uint32FieldTooLargeHex")
                  .ParseText(message(), R"pb(optional_uint32: 0x100000000)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int64FieldTooLargeHex) {
  EXPECT_THAT(
      Testee("Int64FieldTooLargeHex")
          .ParseText(message(), R"pb(optional_int64: 0x8000000000000000)pb")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int64FieldTooSmallHex) {
  EXPECT_THAT(
      Testee("Int64FieldTooSmallHex")
          .ParseText(message(), R"pb(optional_int64: -0x8000000000000001)pb")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Uint64FieldTooLargeHex) {
  EXPECT_THAT(Testee("Uint64FieldTooLargeHex")
                  .ParseText(message(), "optional_uint64: 0x10000000000000000")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ... and in octal.
TEST_P(TextIntegerTest, Int32FieldTooLargeOctal) {
  EXPECT_THAT(Testee("Int32FieldTooLargeOctal")
                  .ParseText(message(), R"pb(optional_int32: 020000000000)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int32FieldTooSmallOctal) {
  EXPECT_THAT(Testee("Int32FieldTooSmallOctal")
                  .ParseText(message(), R"pb(optional_int32: -020000000001)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Uint32FieldTooLargeOctal) {
  EXPECT_THAT(Testee("Uint32FieldTooLargeOctal")
                  .ParseText(message(), R"pb(optional_uint32: 040000000000)pb")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int64FieldTooLargeOctal) {
  EXPECT_THAT(
      Testee("Int64FieldTooLargeOctal")
          .ParseText(message(), "optional_int64: 01000000000000000000000")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Int64FieldTooSmallOctal) {
  EXPECT_THAT(
      Testee("Int64FieldTooSmallOctal")
          .ParseText(message(), "optional_int64: -01000000000000000000001")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextIntegerTest, Uint64FieldTooLargeOctal) {
  EXPECT_THAT(
      Testee("Uint64FieldTooLargeOctal")
          .ParseText(message(), "optional_uint64: 02000000000000000000000")
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, TextIntegerTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
