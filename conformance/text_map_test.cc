// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for map fields: entries given in key order
// round-trip, and a duplicated key keeps the last value.  This replaces the
// legacy map block of TextFormatConformanceTestSuiteImpl<M>::RunAllTests(),
// which ran for the proto3-style message types only; the test names and the
// requests sent to the testee are identical to the legacy ones.
//
// The legacy tests passed a prototype pre-filled with the map entries in
// reverse order to RunValidTextFormatTestWithMessage(); that had no effect
// (the legacy ConformanceRequestSetting only took the prototype's descriptor
// and compared against prototype.New()), so nothing replaces it here.

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

// The inputs below are the legacy raw string literals verbatim: their
// surrounding whitespace and line breaks are part of the request bytes.

using TextMapTest = MessageTypeConformanceTest;

TEST_P(TextMapTest, AlphabeticallySortedMapStringKeys) {
  constexpr absl::string_view kInput = R"(
        map_string_string {
          key: "a"
          value: "value"
        }
        map_string_string {
          key: "b"
          value: "value"
        }
        map_string_string {
          key: "c"
          value: "value"
        }
        )";
  EXPECT_THAT(Testee("AlphabeticallySortedMapStringKeys")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(Testee("AlphabeticallySortedMapStringKeys")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextMapTest, AlphabeticallySortedMapIntKeys) {
  constexpr absl::string_view kInput = R"(
        map_int32_int32 {
          key: 1
          value: 0
        }
        map_int32_int32 {
          key: 2
          value: 0
        }
        map_int32_int32 {
          key: 3
          value: 0
        }
        )";
  EXPECT_THAT(Testee("AlphabeticallySortedMapIntKeys")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(Testee("AlphabeticallySortedMapIntKeys")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

TEST_P(TextMapTest, AlphabeticallySortedMapBoolKeys) {
  constexpr absl::string_view kInput = R"(
        map_bool_bool {
          key: false
          value: false
        }
        map_bool_bool {
          key: true
          value: false
        }
        )";
  EXPECT_THAT(Testee("AlphabeticallySortedMapBoolKeys")
                  .ParseText(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
  EXPECT_THAT(Testee("AlphabeticallySortedMapBoolKeys")
                  .ParseText(message(), kInput)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(kInput))));
}

// The last-specified value will be retained in a parsed map.  Like the legacy
// test, this only checks the binary output.
TEST_P(TextMapTest, DuplicateMapKey) {
  constexpr absl::string_view kInput = R"(
        map_string_nested_message {
          key: "duplicate"
          value: { a: 123 }
        }
        map_string_nested_message {
          key: "duplicate"
          value: { corecursive: {} }
        }
        )";
  constexpr absl::string_view kExpected = R"(
        map_string_nested_message {
          key: "duplicate"
          value: { corecursive: {} }
        }
        )";
  EXPECT_THAT(
      Testee("DuplicateMapKey").ParseText(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, TextMapTest, ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
