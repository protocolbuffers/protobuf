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
//   EXPECT_THAT(
//       Testee()
//           .ParseBinary(TestAllTypesProto2::descriptor(), input)
//           .SerializeBinary(),
//       Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb"))));
//
// The leaf matchers (WhenParsed, RawPayload, IsParseError, ...) only look at
// the result, so they compose with other gMock matchers:
// Yields(AnyOf(IsParseError(), WhenParsed(m))) works as expected.  They match
// the YieldedResult that Yields() hands them rather than the TestResult, so a
// leaf matcher applied to a TestResult without Yields() does not compile.
// EqualsTextProto() and EqualsBinaryProto() match a `const Message&` and are
// meant to be used inside WhenParsed().
//
// Yields() is the one matcher that reads the global failure list (see
// GetGlobalFailureList() in global_test_environment.h).  It turns the failure
// list into the gtest verdict:
//
//   - A failure that is in the failure list passes.  A listed test that
//     succeeds fails.
//   - A failure of a test above the enforcement level is tolerated unless
//     the test is listed.  kP0 failures always count (see TestPriority).
//   - A test the testee skipped passes unless it is listed.  The test
//     environment counts it as a listed skip.
//
// Yields() changes nothing in the process.  It records the outcome of the
// test as a gtest success in the running test, whose message carries an
// internal::ResultRecord (see ResultRecordMessage() in result_record.h).  The
// test environment picks the records up as gtest reports them, tallies them in
// its ResultLedger and reports each test's outcome as a property of the gtest
// test (see test_environment.h).
//
// The failure messages of the leaf matchers end up in failure lists, so each
// matcher documents its message and keeps it stable.  Messages that come from
// an inner matcher use gMock's wording, which can change between versions.
// Failure list entries for those should only use a prefix of the message (see
// ResultLedger::VerdictOnFailure()).
//
// This file also defines PrintTo() for TestResult, which gtest uses to print a
// result when a matcher on it fails.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__

#include <ostream>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// The view of a TestResult that Yields() hands to its inner matcher.  The
// leaf matchers match this rather than the TestResult itself, which is what
// makes a forgotten Yields() a compile error:
//
//   EXPECT_THAT(Testee().ParseBinary(type, input).SerializeBinary(),
//               IsParseError());  // error: TestResult isn't a YieldedResult
//
// It is a view: the result must outlive it.  Only Yields() and the tests of
// the leaf matchers construct one.
class YieldedResult {
 public:
  explicit YieldedResult(const TestResult& result) : result_(&result) {}
  YieldedResult(TestResult&&) = delete;

  const TestResult& result() const { return *result_; }

 private:
  const TestResult* absl_nonnull result_;
};

// Prints a TestResult in gtest failure output.  The output has the test's
// priority and name and a short form of the response.  Long payloads are
// truncated.  Binary payloads are also decoded as the test's message type.
// Declared in TestResult's namespace so that gtest finds it through ADL.  It
// lives with the matchers because their failures are where results get
// printed.
void PrintTo(const TestResult& result, std::ostream* absl_nonnull os);

// Implements WhenParsed() and WhenParsedAs() below.  The payload is
// decoded as `type_override` if it is non-null, and as the test's message type
// otherwise.
testing::Matcher<const YieldedResult&> MakeWhenParsedMatcher(
    testing::Matcher<const Message&> m,
    const Descriptor* absl_nullable type_override = nullptr);

// Implements Yields(): see the function below for the semantics.
testing::Matcher<const TestResult&> MakeYieldsMatcher(
    testing::Matcher<const YieldedResult&> inner);

// The gtest result that a test part result reported now goes to: the running
// test's, else (in SetUpTestSuite() or TearDownTestSuite()) the current test
// suite's ad hoc result, else the whole run's.  Yields() records what it
// learns about each conformance test as a success there, and finds there what
// an earlier check of the same test recorded.
const testing::TestResult& CurrentGtestResult();

}  // namespace internal

// Matches a result whose payload, decoded as the test's message type, matches
// `m`.  The payload is decoded from binary or text output.  `m` is any matcher
// on `const Message&`, usually EqualsTextProto() or EqualsBinaryProto().
//
//   EXPECT_THAT(
//       Testee().ParseBinary(type, input).SerializeBinary(),
//       Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb"))));
//
// A payload that can't be decoded fails with "<format> output we received
// from test was unparseable."  Otherwise the failure message comes from `m`.
// JSON output can't be decoded yet (b/410122158) and fails with a message
// saying so.  Use RawPayload() for it.
template <typename M>
testing::Matcher<const internal::YieldedResult&> WhenParsed(M m) {
  return internal::MakeWhenParsedMatcher(
      testing::SafeMatcherCast<const Message&>(std::move(m)));
}

// Like WhenParsed(), but decodes the payload as the generated type `T`
// instead of the message type the test was run against.  Use it when the
// testee serializes unknown fields that a richer "shadow" type such as
// UnknownToTestAllTypes can decode:
//
//   EXPECT_THAT(Testee("Foo").ParseBinary(type, input).SerializeBinary(),
//               Yields(WhenParsedAs<UnknownToTestAllTypes>(
//                   EqualsBinaryProto(input))));
//
// Failure messages are the same as WhenParsed()'s.
template <typename T, typename M>
testing::Matcher<const internal::YieldedResult&> WhenParsedAs(M m) {
  return internal::MakeWhenParsedMatcher(
      testing::SafeMatcherCast<const Message&>(std::move(m)), T::descriptor());
}

// Matches a result whose raw payload is exactly `bytes`, whatever the output
// format.
//
//   EXPECT_THAT(Testee().ParseBinary(type, input).SerializeBinary(),
//               Yields(RawPayload(input)));
//
// A mismatch fails with "Output was not equivalent to reference message:
// Expect: <octal>, but got: <octal>".  A response that isn't a payload of the
// requested format fails with a fixed message for that case.  That covers no
// result, an error, a skipped test and the wrong output format.  A PROTOBUF
// payload must also be parseable as the test's message type.  Otherwise it
// fails with "Protobuf output we received from test was unparseable."
testing::Matcher<const internal::YieldedResult&> RawPayload(Wire bytes);

// Matches a message equivalent to `text`, parsed as the actual message's type.
// Messages are compared with MessageDifferencer, with NaN equal to NaN.
//
//   EXPECT_THAT(
//       Testee().ParseBinary(type, input).SerializeBinary(),
//       Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb"))));
testing::Matcher<const Message&> EqualsTextProto(absl::string_view text);

// Like EqualsTextProto(), but the expected message is the binary serialization
// `bytes`.  It takes a Wire rather than a string so that NUL bytes survive.
// Messages are compared with MessageDifferencer, with NaN equal to NaN.
//
//   EXPECT_THAT(Testee().ParseBinary(type, input).SerializeBinary(),
//               Yields(WhenParsed(EqualsBinaryProto(VarintField(1, 1)))));
testing::Matcher<const Message&> EqualsBinaryProto(Wire bytes);

// TODO: b/410122158 - Add JSON matchers once JSON support is migrated.

// Matches a response that reports a parse error.
//
//   EXPECT_THAT(Testee().ParseBinary(type, Wire("\x08")).SerializeBinary(),
//               Yields(IsParseError()));
//
// Any other response fails with "Should have failed to parse, but didn't."  A
// runtime error fails with "Should have failed to parse, but raised an error
// instead."
testing::Matcher<const internal::YieldedResult&> IsParseError();

// Matches a response that reports a serialize error.
//
//   EXPECT_THAT(Testee().ParseBinary(type, input).SerializeJson(),
//               Yields(IsSerializeError()));
//
// Any other response fails like it does for IsParseError(), with the
// corresponding "Should have failed to serialize, ..." messages.
testing::Matcher<const internal::YieldedResult&> IsSerializeError();

// Wraps the matcher of every conformance test's EXPECT_THAT.
//
//   EXPECT_THAT(Testee().ParseBinary(type, input).SerializeBinary(),
//               Yields(WhenParsed(EqualsBinaryProto(input))));
//
// Yields() evaluates `inner` against the result, wrapped in an
// internal::YieldedResult (which is what the leaf matchers match), records
// the outcome in the running gtest test and applies the failure list and
// priority rules described at the top of this file.  Its verdict is not
// simply `inner`'s: an expected failure passes and an unexpected success
// fails.  Do not wrap Yields() in Not() or other combinators.  Compose `inner`
// instead.
//
// Each result is meant to be checked once.  gtest evaluates a failing matcher
// a second time to explain the failure, so a second check is allowed as long
// as it reaches the same outcome, which it then only repeats.  A check that
// reaches a different outcome than the one recorded is a test bug and fails
// with "already checked".  A gtest test that runs a conformance test but
// never checks its result fails too (see ConformanceEnvironment in
// test_environment.h).
template <typename M>
testing::Matcher<const internal::TestResult&> Yields(M inner) {
  return internal::MakeYieldsMatcher(
      testing::SafeMatcherCast<const internal::YieldedResult&>(
          std::move(inner)));
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
