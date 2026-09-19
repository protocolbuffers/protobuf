// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__

#include <string>

#include <gmock/gmock.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "json/value.h"

// Helpers shared by the gtest-based JSON conformance tests (json_*_test.cc).
//
// Like binary_test_util.h, this file only holds matchers and pure values.
// Every EXPECT_THAT lives in the tests themselves (go/totw/206), which use
// the framework directly:
//
//   EXPECT_THAT(
//       Testee("HelloWorld")
//           .ParseJson(message(), R"({"optionalString": "Hi"})")
//           .SerializeBinary(),
//       Yields(ParsedPayload(EqualsTextProto("optional_string: 'Hi'"))));
//
// The message type sets the tests are parameterized over are in
// binary_test_util.h: AllTestMessageTypes(), Proto3TestMessageTypes() and
// Proto2TestMessageTypes().

namespace google {
namespace protobuf {
namespace conformance {

// Returns `s` repeated `count` times, for building the deeply nested JSON
// inputs of the recursion-limit tests.
inline std::string Repeat(absl::string_view s, int count) {
  std::string result;
  for (int i = 0; i < count; ++i) {
    absl::StrAppend(&result, s);
  }
  return result;
}

// Matches a JSON object that has a member named `name`.  Meant for use inside
// JsonPayload():
//
//   Yields(JsonPayload(AllOf(HasJsonMember("x"), HasJsonMember("y"))))
//   Yields(JsonPayload(Not(HasJsonMember("x"))))
//
// A value that isn't a JSON object does not match.  The failure message is
// "Expect: JSON payload has member "x", but got: ...".
MATCHER_P(HasJsonMember, name,
          absl::StrCat(negation ? "doesn't have" : "has", " member \"", name,
                       "\"")) {
  if (!arg.isObject()) {
    *result_listener << "which is not a JSON object";
    return false;
  }
  return arg.isMember(name);
}

// Matches a JSON object that has a member named `name` whose value matches
// `member_matcher`, a matcher on a Json::Value.  For example, this checks
// that an int64 field is serialized as a string:
//
//   Yields(JsonPayload(HasJsonMemberThat("optionalInt64", IsJsonString("1"))))
//
// A value that isn't a JSON object does not match.
MATCHER_P2(HasJsonMemberThat, name, member_matcher,
           absl::StrCat(negation ? "doesn't have" : "has", " member \"", name,
                        "\" that ",
                        ::testing::DescribeMatcher<const Json::Value&>(
                            member_matcher))) {
  if (!arg.isObject()) {
    *result_listener << "which is not a JSON object";
    return false;
  }
  return arg.isMember(name) && ::testing::ExplainMatchResult(
                                   member_matcher, arg[name], result_listener);
}

// Matches a JSON string equal to `expected`.  A number or any other JSON kind
// doesn't match, whatever its textual form.
//
//   Yields(JsonPayload(HasJsonMemberThat("optionalInt64", IsJsonString("1"))))
MATCHER_P(IsJsonString, expected,
          absl::StrCat(negation ? "isn't" : "is", " the JSON string \"",
                       expected, "\"")) {
  return arg.isString() && arg.asString() == expected;
}

// Matches a JSON number that jsoncpp parsed as a signed integer
// (Json::intValue) equal to `expected`.  A string "123" or a real 123.0
// doesn't match.
//
//   Yields(JsonPayload(HasJsonMemberThat("optionalInt32", IsJsonInt(123))))
MATCHER_P(IsJsonInt, expected,
          absl::StrCat(negation ? "isn't" : "is", " the JSON integer ",
                       expected)) {
  return arg.type() == Json::intValue && arg.asInt() == expected;
}

// Matches the JSON value `null`.
//
//   Yields(JsonPayload(HasJsonMemberThat("oneofNullValue", IsJsonNull())))
MATCHER(IsJsonNull, absl::StrCat(negation ? "isn't" : "is", " JSON null")) {
  return arg.isNull();
}

// Matches a JSON object with no members, that is `{}`.  This is the
// serialization of a message none of whose fields is emitted:
//
//   Yields(JsonPayload(IsEmptyJsonObject()))
//
// Unlike Json::Value::empty(), neither `null` nor `[]` matches.
MATCHER(IsEmptyJsonObject,
        absl::StrCat(negation ? "isn't" : "is", " an empty JSON object")) {
  if (!arg.isObject()) {
    *result_listener << "which is not a JSON object";
    return false;
  }
  return arg.empty();
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__
