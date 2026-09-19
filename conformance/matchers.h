// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// gMock matchers for conformance TestResults.
//
// Every conformance test ends in one EXPECT_THAT.  Its matcher is Yields()
// wrapped around one of the leaf matchers defined here:
//
//   EXPECT_THAT(Testee("foo")
//                   .ParseBinary(TestAllTypesProto2::descriptor(), input)
//                   .SerializeBinary(),
//               Yields(ParsedPayload(EqualsTextProto("optional_int32: 1"))));
//
// The leaf matchers (ParsedPayload, Payload, IsParseError, ...) only look at
// the TestResult, so they compose with other gMock matchers:
// Yields(AnyOf(IsParseError(), ParsedPayload(m))) works as expected.
// EqualsTextProto() and EqualsBinaryProto() match a `const Message&` and are
// meant to be used inside ParsedPayload().
//
// Yields() is the one matcher that talks to the global TestManager.  It
// records the outcome of the test and turns the failure list into the gtest
// verdict:
//
//   - A failure that is in the failure list passes.  A listed test that
//     succeeds fails.
//   - A failure of a test above the enforcement level is tolerated unless
//     the test is listed.  kP0 failures always count (see TestPriority).
//   - A test the testee skipped passes unless it is listed.  The TestManager
//     counts it in ListedSkips().
//   - A test the runner filtered out (see kTestNotSelectedSkipReason in
//     test_runner.h) is not counted at all.
//   - A runtime error, a timeout or a missing result is always a failure.
//
// The failure messages of the leaf matchers end up in failure lists, so each
// matcher documents its message and keeps it stable.  Messages that come from
// an inner matcher use gMock's wording, which can change between versions.
// Failure list entries for those should only use a prefix of the message (see
// TestManager::ReportFailure()).
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

// Implements ParsedPayload() below.
testing::Matcher<const TestResult&> MakeParsedPayloadMatcher(
    testing::Matcher<const Message&> m);

// Implements Yields(): see the function below for the semantics.
testing::Matcher<const TestResult&> MakeYieldsMatcher(
    testing::Matcher<const TestResult&> inner);

}  // namespace internal

// Matches a result whose payload, decoded as the test's message type, matches
// `m`.  The payload is decoded from binary or text output.  `m` is any matcher
// on `const Message&`, usually EqualsTextProto() or EqualsBinaryProto().
//
//   EXPECT_THAT(Testee("Foo").ParseBinary(type, input).SerializeBinary(),
//               Yields(ParsedPayload(EqualsTextProto("optional_int32: 1"))));
//
// A payload that can't be decoded fails with "<format> output we received
// from test was unparseable."  Otherwise the failure message comes from `m`.
// JSON output can't be decoded yet (b/410122158) and fails with a message
// saying so.  Use Payload() for it.
template <typename M>
testing::Matcher<const internal::TestResult&> ParsedPayload(M m) {
  return internal::MakeParsedPayloadMatcher(
      testing::SafeMatcherCast<const Message&>(std::move(m)));
}

// Matches a result whose raw payload is exactly `bytes`, whatever the output
// format.
//
//   EXPECT_THAT(Testee("Foo").ParseBinary(type, input).SerializeBinary(),
//               Yields(Payload(input)));
//
// A mismatch fails with "Output was not equivalent to reference message:
// Expect: <octal>, but got: <octal>".  A response that isn't a payload of the
// requested format fails with a fixed message for that case.  That covers no
// result, an error, a skipped test and the wrong output format.  A PROTOBUF
// payload must also be parseable as the test's message type.  Otherwise it
// fails with "Protobuf output we received from test was unparseable."
testing::Matcher<const internal::TestResult&> Payload(Wire bytes);

// Matches a message equivalent to `text`, parsed as the actual message's type.
// Messages are compared with MessageDifferencer, with NaN equal to NaN.
//
//   Yields(ParsedPayload(EqualsTextProto("optional_int32: 1")))
testing::Matcher<const Message&> EqualsTextProto(absl::string_view text);

// Like EqualsTextProto(), but the expected message is the binary serialization
// `bytes`.  It takes a Wire rather than a string so that NUL bytes survive.
//
//   Yields(ParsedPayload(EqualsBinaryProto(VarintField(1, 1))))
testing::Matcher<const Message&> EqualsBinaryProto(Wire bytes);

// TODO: b/410122158 - Add JSON matchers once JSON support is migrated.

// Matches a response that reports a parse error.
//
//   EXPECT_THAT(Testee("Foo").ParseBinary(type, "\x08").SerializeBinary(),
//               Yields(IsParseError()));
//
// Any other response fails with "Should have failed to parse, but didn't."  A
// runtime error fails with "Should have failed to parse, but raised an error
// instead."
testing::Matcher<const internal::TestResult&> IsParseError();

// Matches a response that reports a serialize error.
//
//   EXPECT_THAT(Testee("Foo").ParseBinary(type, input).SerializeJson(),
//               Yields(IsSerializeError()));
//
// Any other response fails like it does for IsParseError(), with the
// corresponding "Should have failed to serialize, ..." messages.
testing::Matcher<const internal::TestResult&> IsSerializeError();

// Wraps the matcher of every conformance test's EXPECT_THAT.
//
//   EXPECT_THAT(Testee("X").ParseBinary(type, input).SerializeBinary(),
//               Yields(ParsedPayload(EqualsBinaryProto(input))));
//
// Yields() evaluates `inner` against the TestResult, reports the outcome to
// the global TestManager and applies the failure list and priority rules
// described at the top of this file.  Its verdict is not simply `inner`'s: an
// expected failure passes and an unexpected success fails.  Do not wrap
// Yields() in Not() or other combinators.  Compose `inner` instead.
//
// Each TestResult may be checked exactly once.  The first evaluation reports
// to the TestManager and stores the verdict in the result.  Later evaluations
// replay a failed verdict, which lets gtest explain the failure.  Checking a
// result that already passed fails with "already checked".  A result that is
// never checked reports a gtest failure when it is destroyed.
template <typename M>
testing::Matcher<const internal::TestResult&> Yields(M inner) {
  return internal::MakeYieldsMatcher(
      testing::SafeMatcherCast<const internal::TestResult&>(std::move(inner)));
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
