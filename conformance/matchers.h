// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// gMock matchers for conformance tests.
//
// The matchers are pure, so they compose with gMock like any other matcher.
// The failure messages they write end up in failure lists, so each matcher
// documents its message and keeps it stable.
//
// This file also defines PrintTo() for TestResult, which gtest uses to print a
// result when a matcher on it fails.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__

#include <ostream>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/testee.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// Prints a TestResult in gtest failure output.  The output has the test's
// priority and name and a short form of the response.  Long payloads are
// truncated.  Binary payloads are also decoded as the test's message type.
// Declared in TestResult's namespace so that gtest finds it through ADL.  It
// lives with the matchers because their failures are where results get
// printed.
void PrintTo(const TestResult& result, std::ostream* os);

// Implements WhenParsed() below.
testing::Matcher<const TestResult&> MakeWhenParsedMatcher(
    testing::Matcher<const Message&> m);

}  // namespace internal

// Matches a result whose payload, decoded as the test's message type, matches
// `m`.  The payload is decoded from binary or text output.  `m` is any matcher
// on `const Message&`, usually EqualsTextProto() or EqualsBinaryProto().
//
//   EXPECT_THAT(result,
//               WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")));
//
// A payload that can't be decoded fails with "<format> output we received
// from test was unparseable."  Otherwise the failure message comes from `m`.
// JSON output can't be decoded yet (b/410122158) and fails with a message
// saying so.  Use RawPayload() for it.
template <typename M>
testing::Matcher<const internal::TestResult&> WhenParsed(M m) {
  return internal::MakeWhenParsedMatcher(
      testing::SafeMatcherCast<const Message&>(std::move(m)));
}

// Matches a result whose raw payload is exactly `bytes`, whatever the output
// format.
//
//   EXPECT_THAT(result, RawPayload(input));
//
// A mismatch fails with "Output was not equivalent to reference message:
// Expect: <octal>, but got: <octal>".  A response that isn't a payload of the
// requested format fails with a fixed message for that case.  That covers no
// result, an error, a skipped test and the wrong output format.  A PROTOBUF
// payload must also be parseable as the test's message type.  Otherwise it
// fails with "Protobuf output we received from test was unparseable."
testing::Matcher<const internal::TestResult&> RawPayload(Wire bytes);

// Matches a message equivalent to `text`, parsed as the actual message's type.
// Messages are compared with MessageDifferencer, with NaN equal to NaN.
//
//   EXPECT_THAT(result,
//               WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")));
testing::Matcher<const Message&> EqualsTextProto(absl::string_view text);

// Like EqualsTextProto(), but the expected message is the binary serialization
// `bytes`.  It takes a Wire rather than a string so that NUL bytes survive.
// Messages are compared with MessageDifferencer, with NaN equal to NaN.
//
//   EXPECT_THAT(message, EqualsBinaryProto(VarintField(1, 1)));
testing::Matcher<const Message&> EqualsBinaryProto(Wire bytes);

// TODO: b/410122158 - Add JSON matchers once JSON support is migrated.

// Matches a response that reports a parse error.
//
//   EXPECT_THAT(result, IsParseError());
//
// Any other response fails with "Should have failed to parse, but didn't."  A
// runtime error fails with "Should have failed to parse, but raised an error
// instead."
testing::Matcher<const internal::TestResult&> IsParseError();

// Matches a response that reports a serialize error.
//
//   EXPECT_THAT(result, IsSerializeError());
//
// Any other response fails like it does for IsParseError(), with the
// corresponding "Should have failed to serialize, ..." messages.
testing::Matcher<const internal::TestResult&> IsSerializeError();

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
