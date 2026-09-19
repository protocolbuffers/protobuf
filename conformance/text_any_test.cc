// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for google.protobuf.Any fields: the
// `[type_url] { ... }` expansion, raw type_url/value pairs, and the type URL
// prefixes and separators a parser must accept or reject.  This replaces the
// legacy TextFormatConformanceTestSuiteImpl<M>::RunAnyTests(), which ran for
// the proto3-style message types only; the test names and the requests sent to
// the testee are identical to the legacy ones.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// The inputs below are the legacy raw string literals verbatim: their
// surrounding whitespace and line breaks are part of the request bytes.  The
// packed message is always protobuf_test_messages.proto3.TestAllTypesProto3,
// whichever message type the test runs for, as in the legacy suite.

using TextAnyTest = MessageTypeConformanceTest;

TEST_P(TextAnyTest, AnyField) {
  constexpr absl::string_view kInput = R"(
        optional_any: {
          [type.googleapis.com/protobuf_test_messages.proto3.TestAllTypesProto3]
  { optional_int32: 12345
          }
        }
        )";
  EXPECT_THAT(
      RequiredTest("AnyField").ParseText(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(
      RequiredTest("AnyField").ParseText(message(), kInput).SerializeText(),
      Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextAnyTest, WithRawBytes) {
  constexpr absl::string_view kInput = R"(
        optional_any: {
          type_url:
  "type.googleapis.com/protobuf_test_messages.proto3.TestAllTypesProto3" value:
  "\b\271`"
        }
        )";
  EXPECT_THAT(RequiredTest("AnyFieldWithRawBytes")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(RequiredTest("AnyFieldWithRawBytes")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextAnyTest, WithInvalidType) {
  constexpr absl::string_view kInput = R"(
        optional_any: {
          [type.googleapis.com/unknown] {
            optional_int32: 12345
          }
        }
        )";
  EXPECT_THAT(RequiredTest("AnyFieldWithInvalidType")
                  .ParseText(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextAnyTest, WithCustomTypeUrlPrefix) {
  constexpr absl::string_view kInput = R"(
        optional_any: {
          [non.default.domain/protobuf_test_messages.proto3.TestAllTypesProto3]
          {
            optional_int32: 12345
          }
        }
        )";
  EXPECT_THAT(RequiredTest("AnyFieldWithCustomTypeUrlPrefix")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(RequiredTest("AnyFieldWithCustomTypeUrlPrefix")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextAnyTest, WithSpecialCharactersInTypeUrlPrefix) {
  constexpr absl::string_view kInput = R"(
        optional_any: {
          [non.default.domain/sub-path_0/~!$&()*+,;=/protobuf_test_messages.proto3.TestAllTypesProto3]
          {
            optional_int32: 12345
          }
        }
        )";
  EXPECT_THAT(RequiredTest("AnyFieldWithSpecialCharactersInTypeUrlPrefix")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(RequiredTest("AnyFieldWithSpecialCharactersInTypeUrlPrefix")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextAnyTest, WithValidUrlPercentEscapeInTypeUrlPrefix) {
  constexpr absl::string_view kInput = R"(
        optional_any: {
          [non.default.domain/%2F/protobuf_test_messages.proto3.TestAllTypesProto3]
          {
            optional_int32: 12345
          }
        }
        )";
  EXPECT_THAT(RequiredTest("AnyFieldWithValidUrlPercentEscapeInTypeUrlPrefix")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(RequiredTest("AnyFieldWithValidUrlPercentEscapeInTypeUrlPrefix")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextAnyTest, WithInvalidUrlPercentEscapeInTypeUrlPrefix) {
  constexpr absl::string_view kInput = R"(
        optional_any: {
          [non.default.domain/%ZZ/protobuf_test_messages.proto3.TestAllTypesProto3]
          {
            optional_int32: 12345
          }
        }
        )";
  EXPECT_THAT(RequiredTest("AnyFieldWithInvalidUrlPercentEscapeInTypeUrlPrefix")
                  .ParseText(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextAnyTest, WithWhitespaceInTypeUrl) {
  constexpr absl::string_view kInput =
      "optional_any: {\n"
      "  [ ty pe.go\nogleap\tis.com/\n"
      "    proto buf_te\nst_messages.proto3.Test\tAllTypesProto3 ]\n"
      "  {\n"
      "    optional_int32: 12345\n"
      "  }\n"
      "}";
  EXPECT_THAT(RequiredTest("AnyFieldWithWhitespaceInTypeUrl")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(RequiredTest("AnyFieldWithWhitespaceInTypeUrl")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextAnyTest, WithCommentsInTypeUrl) {
  constexpr absl::string_view kInput =
      "optional_any: {\n"
      "  [type.google # comment \napis.com/"
      "protobuf_test_messages.proto3.Test # comment \nAllTypesProto3]"
      "  {\n"
      "    optional_int32: 12345\n"
      "  }\n"
      "}";
  EXPECT_THAT(RequiredTest("AnyFieldWithCommentsInTypeUrl")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(RequiredTest("AnyFieldWithCommentsInTypeUrl")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

INSTANTIATE_TEST_SUITE_P(All, TextAnyTest, ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
