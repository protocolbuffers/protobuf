// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for `null`: it is accepted (and means "unset") as
// the value of a field of any type, but not as an element of a repeated
// field or as a map key or value.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNullTypes(), for
// every test message type; the requests sent to the testee are identical to the
// legacy ones.
//
// Like the legacy RunValidJsonTest(), the valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  The legacy ExpectParseFailureForJson() asked for
// JSON output under a name without an output suffix.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

class JsonNullTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// "null" is accepted for all field types, and leaves the field unset.
TEST_P(JsonNullTest, AllFieldAcceptNull) {
  constexpr absl::string_view kInput = R"({
        "optionalInt32": null,
        "optionalInt64": null,
        "optionalUint32": null,
        "optionalUint64": null,
        "optionalSint32": null,
        "optionalSint64": null,
        "optionalFixed32": null,
        "optionalFixed64": null,
        "optionalSfixed32": null,
        "optionalSfixed64": null,
        "optionalFloat": null,
        "optionalDouble": null,
        "optionalBool": null,
        "optionalString": null,
        "optionalBytes": null,
        "optionalNestedEnum": null,
        "optionalNestedMessage": null,
        "repeatedInt32": null,
        "repeatedInt64": null,
        "repeatedUint32": null,
        "repeatedUint64": null,
        "repeatedSint32": null,
        "repeatedSint64": null,
        "repeatedFixed32": null,
        "repeatedFixed64": null,
        "repeatedSfixed32": null,
        "repeatedSfixed64": null,
        "repeatedFloat": null,
        "repeatedDouble": null,
        "repeatedBool": null,
        "repeatedString": null,
        "repeatedBytes": null,
        "repeatedNestedEnum": null,
        "repeatedNestedMessage": null,
        "mapInt32Int32": null,
        "mapBoolBool": null,
        "mapStringNestedMessage": null
      })";
  EXPECT_THAT(Testee(kP0).ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
  EXPECT_THAT(Testee(kP0).ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

// Repeated field elements cannot be null.
TEST_P(JsonNullTest, RepeatedFieldPrimitiveElementIsNull) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"repeatedInt32": [1, null, 2]})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonNullTest, RepeatedFieldMessageElementIsNull) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(),
                     R"({"repeatedNestedMessage": [{"a":1}, null, {"a":2}]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Map field keys cannot be null.
TEST_P(JsonNullTest, MapFieldKeyIsNull) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"mapInt32Int32": {null: 1}})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Map field values cannot be null.
TEST_P(JsonNullTest, MapFieldValueIsNull) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"mapInt32Int32": {"0": null}})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonNullTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
