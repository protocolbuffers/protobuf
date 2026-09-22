// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for google.protobuf.Any fields: the "@type" member
// (in any position, or nested in another Any), the well-known types whose
// JSON form is a "value" member, the type URLs that must be rejected, an Any
// without a type, `null`, and an empty packed message.  This replaces the
// legacy BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForAny(), which ran
// for the proto3-style message types only; the requests sent to the testee are
// identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.  The inputs are the legacy raw
// string literals verbatim; the ones packing the test message itself get its
// type URL substituted per message type, as in the legacy suite.

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using JsonAnyTest = MessageTypeConformanceTest;

TEST_P(JsonAnyTest, Any) {
  const std::string type_url =
      absl::StrCat("type.googleapis.com/", message()->full_name());
  const std::string input = absl::Substitute(R"({
        "optionalAny": {
          "@type": "$0",
          "optionalInt32": 12345
  }
      })",
                                             type_url);
  const std::string expected = absl::Substitute(R"(
        optional_any: {
          [$0] {
            optional_int32: 12345
          }
        }
      )",
                                                type_url);
  EXPECT_THAT(Testee().ParseJson(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee().ParseJson(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(JsonAnyTest, AnyNested) {
  const std::string type_url =
      absl::StrCat("type.googleapis.com/", message()->full_name());
  const std::string input = absl::Substitute(R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Any",
          "value": {
            "@type": "$0",
            "optionalInt32": 12345
    }
  }
      })",
                                             type_url);
  const std::string expected = absl::Substitute(R"(
        optional_any: {
          [type.googleapis.com/google.protobuf.Any] {
            [$0] {
              optional_int32: 12345
            }
          }
        }
      )",
                                                type_url);
  EXPECT_THAT(Testee().ParseJson(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee().ParseJson(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

// The special "@type" tag is not required to appear first.
TEST_P(JsonAnyTest, AnyUnorderedTypeTag) {
  const std::string type_url =
      absl::StrCat("type.googleapis.com/", message()->full_name());
  const std::string input = absl::Substitute(R"({
        "optionalAny": {
          "optionalInt32": 12345,
          "@type": "$0"
        }
      })",
                                             type_url);
  const std::string expected = absl::Substitute(R"(
        optional_any: {
          [$0] {
            optional_int32: 12345
          }
        }
      )",
                                                type_url);
  EXPECT_THAT(Testee().ParseJson(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee().ParseJson(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

// Well-known types in Any.
TEST_P(JsonAnyTest, AnyWithInt32ValueWrapper) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Int32Value",
          "value": 12345
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Int32Value] {
            value: 12345
    }
  }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonAnyTest, AnyWithDuration) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Duration",
          "value": "1.5s"
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Duration] {
            seconds: 1
            nanos: 500000000
    }
  }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonAnyTest, AnyWithTimestamp) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Timestamp",
          "value": "1970-01-01T00:00:00Z"
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Timestamp] {
            seconds: 0
            nanos: 0
    }
  }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonAnyTest, AnyWithFieldMask) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.FieldMask",
          "value": "foo,barBaz"
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.FieldMask] {
            paths: ["foo", "bar_baz"]
    }
  }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonAnyTest, AnyWithStruct) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Struct",
          "value": {
            "foo": 1
    }
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Struct] {
            fields: {
              key: "foo"
              value: {
                number_value: 1
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

TEST_P(JsonAnyTest, AnyWithValueForJsonObject) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Value",
          "value": {
            "foo": 1
    }
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Value] {
            struct_value: {
              fields: {
                key: "foo"
                value: {
                  number_value: 1
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

TEST_P(JsonAnyTest, AnyWithValueForInteger) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Value",
          "value": 1
  }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Value] {
            number_value: 1
    }
  }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// When the Any is in WKT form (with "@type"), the type_url must be present
// and URL shaped, otherwise it should be a parse error (because it can't be
// parsed into the Any schema).
TEST_P(JsonAnyTest, AnyWktRepresentationWithEmptyTypeAndValue) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({
        "optionalAny": {
          "@type": "",
          "value": ""
        }
      })")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonAnyTest, AnyWktRepresentationWithBadType) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({
        "optionalAny": {
          "@type": "not_a_url",
          "value": ""
        }
      })")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// When the Any can be parsed as non-WKT form, the type_url could be missing
// or invalid, since that can still be parsed into the Any schema.
TEST_P(JsonAnyTest, AnyWithNoType) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {}
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {}
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// `null` where an Any exists should just result in the field being unset.
TEST_P(JsonAnyTest, AnyNull) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": null
      })";
  constexpr absl::string_view kExpected = R"(
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// google.protobuf.Empty packed into an Any, implementations must accept it
// without the "value" field set. This also confirms that what they round trip
// does not have `"value":{}` set on it, since the test harness uses the C++
// JSON parser which will reject it.
TEST_P(JsonAnyTest, AnyEmpty) {
  constexpr absl::string_view kInput = R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Empty"
        }
      })";
  constexpr absl::string_view kExpected = R"(
        optional_any: {
          [type.googleapis.com/google.protobuf.Empty] {}
        }
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonAnyTest, ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
