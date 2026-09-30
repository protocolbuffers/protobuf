// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__

#include <gmock/gmock.h>
#include "absl/strings/str_cat.h"

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

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__
