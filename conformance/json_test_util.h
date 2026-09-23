// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_JSON_TEST_UTIL_H__

#include <gmock/gmock.h>
#include "absl/strings/str_cat.h"

// Helpers shared by the gtest-based JSON conformance tests (json_*_test.cc).
// Like binary_test_util.h, this only holds matchers and pure values: every
// EXPECT_THAT lives in the tests themselves (go/totw/206), which use the
// framework directly, e.g.
//
//   EXPECT_THAT(
//       Testee("HelloWorld")
//           .ParseJson(message(), R"({"optionalString": "Hi"})")
//           .SerializeBinary(),
//       Yields(ParsedPayload(EqualsTextProto("optional_string: 'Hi'"))));
//
// The message type sets the tests are parameterized over
// (AllTestMessageTypes(), Proto3TestMessageTypes(), Proto2TestMessageTypes())
// are in binary_test_util.h.

namespace google {
namespace protobuf {
namespace conformance {

// Matches a JSON object (a Json::Value) that has a member named `name`, for
// use inside JsonPayload().  The legacy suite's JSON "validators" were almost
// all of the form `value.isMember("x") && value.isMember("y")`, which becomes
// `JsonPayload(AllOf(HasJsonMember("x"), HasJsonMember("y")))`; a serializer
// that must omit a field uses `JsonPayload(Not(HasJsonMember("x")))`.  Being a
// named matcher, its description makes JsonPayload()'s failure message
// meaningful: "Expect: JSON payload has member "x", but got: ...".  A JSON
// value that isn't an object (an array, a string, null, ...) doesn't match,
// and says so, since Json::Value::isMember() itself only accepts objects.
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
