// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for google.protobuf.Struct fields: a Struct holding
// every kind of Value, an empty ListValue member, and Structs nested 25 deep
// (must be accepted) and 200 deep (must be rejected).  This replaces the
// legacy BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForStruct(), which
// ran for the proto3-style message types only; the requests sent to the testee
// are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/json_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using JsonStructTest = MessageTypeConformanceTest;

TEST_P(JsonStructTest, Struct) {
  constexpr absl::string_view kInput = R"({
        "optionalStruct": {
          "nullValue": null,
          "intValue": 1234,
          "boolValue": true,
          "doubleValue": 1234.5678,
          "stringValue": "Hello world!",
          "listValue": [1234, "5678"],
          "objectValue": {
            "value": 0
    }
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_struct: {
          fields: {
            key: "nullValue"
            value: {null_value: NULL_VALUE}
    }
          fields: {
            key: "intValue"
            value: {number_value: 1234}
    }
          fields: {
            key: "boolValue"
            value: {bool_value: true}
    }
          fields: {
            key: "doubleValue"
            value: {number_value: 1234.5678}
    }
          fields: {
            key: "stringValue"
            value: {string_value: "Hello world!"}
    }
          fields: {
            key: "listValue"
            value: {
              list_value: {
                values: {
                  number_value: 1234
          }
                values: {
                  string_value: "5678"
          }
        }
      }
    }
          fields: {
            key: "objectValue"
            value: {
              struct_value: {
                fields: {
                  key: "value"
                  value: {
                    number_value: 0
            }
          }
        }
      }
    }
  }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStructTest, StructWithEmptyListValue) {
  constexpr absl::string_view kInput = R"({
        "optionalStruct": {
          "listValue": []
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_struct: {
          fields: {
            key: "listValue"
            value: {
              list_value: {
        }
      }
    }
  }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStructTest, StructDeepNesting25) {
  const std::string input =
      absl::StrCat(R"({"optionalStruct": {)", Repeat(R"("n": {)", 25),
                   R"("value": 1)", std::string(25, '}'), "}}");
  const std::string expected =
      absl::StrCat("optional_struct: {\n",
                   Repeat("  fields: {\n"
                          "    key: \"n\"\n"
                          "    value: {\n"
                          "      struct_value: {\n",
                          25),
                   "        fields: {\n"
                   "          key: \"value\"\n"
                   "          value: { number_value: 1 }\n"
                   "        }\n",
                   Repeat("      }\n"
                          "    }\n"
                          "  }\n",
                          25),
                   "}\n");
  EXPECT_THAT(Testee(kP3).ParseJson(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kP3).ParseJson(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(JsonStructTest, StructDeepNesting200) {
  const std::string input =
      absl::StrCat(R"({"optionalStruct": {)", Repeat(R"("n": {)", 200),
                   R"("value": 1)", std::string(200, '}'), "}}");
  EXPECT_THAT(Testee(kP3).ParseJson(message(), input).ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonStructTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
