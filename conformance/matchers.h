// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file defines gMock matchers for conformance TestResult objects.  Every
// conformance test should end in exactly one EXPECT_THAT whose matcher is
// Yields(...) wrapped around one of the *leaf* matchers defined here
// (ParsedPayload, Payload, IsParseError, ...), e.g.:
//
//   EXPECT_THAT(Testee("foo")
//                   .ParseBinary(TestAllTypesProto2::descriptor(), input)
//                   .SerializeBinary(),
//               Yields(ParsedPayload(EqualsTextProto("optional_int32: 1"))));
//
// The leaf matchers are pure: they only inspect the TestResult (and their inner
// matcher) and write the legacy failure text to the listener, so they compose
// with gMock like any other matcher, e.g.
// `Yields(AnyOf(IsParseError(), ParsedPayload(EqualsBinaryProto(w))))` or
// `Yields(Not(ParsedPayload(...)))`.  ParsedPayload(), Payload() and
// JsonPayload() take care of decoding the testee's response and accept any
// gMock matcher for the decoded value.  EqualsTextProto(), EqualsBinaryProto()
// and HasUnknownFieldsInOrder() are ordinary matchers on `const Message&` that
// are intended to be used inside ParsedPayload(), but work anywhere.
//
// Yields() is the single matcher that talks to the global TestManager (see
// global_test_environment.h): it records the outcome of the test so that
// results can be checked against, and used to regenerate, the expected failure
// list, and it turns the failure list into the gtest verdict:
//
//   - A test whose failure is listed in the failure list *passes* (the failure
//     was expected), and a listed test that unexpectedly succeeds *fails*.
//   - A failure of a kP3 (recommended) test is tolerated (logged as a WARNING
//     and counted, but the test passes) unless the TestManager enforces
//     recommended tests or the test is listed in the failure list.  Failures
//     of kP0, kP1 and kP2 tests are enforced alike; see TestPriority in
//     testee.h.
//   - A response the testee marked as `skipped` passes and is counted as a
//     skip, unless the test is listed in the failure list, in which case it
//     fails once, naming the matched entry ("test X (matched to Y) is in the
//     failure list but was skipped by the testee: ...").  The entry still
//     counts as seen, so Finalize() doesn't report it again, and --fix keeps
//     it: removing it is up to the user.  The TestManager records such tests
//     in ListedSkips().  This is deliberately stricter than the legacy runner,
//     which passed a listed test the testee skipped.
//   - A test the runner filtered out (`skipped` is exactly
//     kTestNotSelectedSkipReason, see test_runner.h) passes silently and isn't
//     counted at all; its failure list entry, if any, only counts as matched.
//     That reason string is reserved for runners; a testee answering with it
//     would be treated as not run.
//   - A response holding a runtime or timeout error, or no result at all, is
//     always a failure, whatever the inner matcher says.
//
// The failure messages produced by these matchers are the ones that end up in
// failure lists, and are therefore kept byte-identical to the legacy
// conformance runner for every case the legacy runner covers.  Cases it
// doesn't cover produce new text: an inner matcher without an explanation of
// its own (e.g. a plain `Eq()`) yields "Expect: <description>, but got: ...",
// and composite inner matchers (AnyOf, Not, ...) yield gMock's own,
// version-dependent wording.  Failure list entries for those should be
// written as message prefixes (see TestManager::ReportFailure()).

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>
#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "json/value.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// Whether `T` is one of the types accepted as literal bytes by Payload() and
// EqualsBinaryProto().  Bare `const char*` (and string literals) are
// deliberately excluded: binary data routinely contains NUL bytes, which would
// silently truncate them.
template <typename T>
inline constexpr bool kIsBytesLike =
    std::is_same_v<T, Wire> || std::is_same_v<T, std::string> ||
    std::is_same_v<T, absl::string_view>;

// The bytes held by a bytes-like value.
inline std::string BytesOf(Wire wire) { return std::move(wire).str(); }
inline std::string BytesOf(std::string bytes) { return bytes; }
inline std::string BytesOf(absl::string_view bytes) {
  return std::string(bytes);
}

// Base class for the matchers that look at the payload of a successful
// response.  It handles all of the cases shared by every payload comparison
// (no payload, an error response, the wrong output format, a skipped test) and
// defers the actual comparison to MatchPayload().
class PayloadMatcher {
 public:
  using is_gtest_matcher = void;

  virtual ~PayloadMatcher() = default;

  bool MatchAndExplain(const TestResult& result,
                       testing::MatchResultListener* listener) const;
  void DescribeTo(std::ostream* os) const {
    DescribeInnerTo(os, /*negation=*/false);
  }
  void DescribeNegationTo(std::ostream* os) const {
    DescribeInnerTo(os, /*negation=*/true);
  }

 protected:
  PayloadMatcher() = default;
  PayloadMatcher(const PayloadMatcher&) = default;
  PayloadMatcher& operator=(const PayloadMatcher&) = default;

  // Compares the payload of `result`, which is guaranteed to hold a payload of
  // the format the test requested.  Returns true on a match; otherwise the
  // explanation written to `listener` becomes the failure message recorded in
  // the failure list.
  virtual bool MatchPayload(const TestResult& result,
                            testing::MatchResultListener* listener) const = 0;

  // Describes the matcher (or its negation).
  virtual void DescribeInnerTo(std::ostream* os, bool negation) const = 0;
};

// Implements ParsedPayload() and ParsedPayloadAs().
class ParsedPayloadMatcher : public PayloadMatcher {
 public:
  // The payload is decoded as `type_override` if it is non-null, and as the
  // test's message type otherwise.
  explicit ParsedPayloadMatcher(testing::Matcher<const Message&> matcher,
                                const Descriptor* type_override = nullptr)
      : matcher_(std::move(matcher)), type_override_(type_override) {}

 private:
  bool MatchPayload(const TestResult& result,
                    testing::MatchResultListener* listener) const override;
  void DescribeInnerTo(std::ostream* os, bool negation) const override;

  testing::Matcher<const Message&> matcher_;
  // Null means "the test's message type".
  const Descriptor* absl_nullable type_override_;
};

// Implements Payload().
class RawPayloadMatcher : public PayloadMatcher {
 public:
  explicit RawPayloadMatcher(testing::Matcher<absl::string_view> matcher)
      : matcher_(std::move(matcher)) {}

 private:
  bool MatchPayload(const TestResult& result,
                    testing::MatchResultListener* listener) const override;
  void DescribeInnerTo(std::ostream* os, bool negation) const override;

  testing::Matcher<absl::string_view> matcher_;
};

// Implements JsonPayload().
class JsonPayloadMatcher : public PayloadMatcher {
 public:
  explicit JsonPayloadMatcher(testing::Matcher<const Json::Value&> matcher)
      : matcher_(std::move(matcher)) {}

 private:
  bool MatchPayload(const TestResult& result,
                    testing::MatchResultListener* listener) const override;
  void DescribeInnerTo(std::ostream* os, bool negation) const override;

  testing::Matcher<const Json::Value&> matcher_;
};

// Byte-for-byte equality against `expected` whose explanation is the legacy
// "Output was not equivalent to reference message: Expect: <octal>, but got:
// <octal>" text.  An implementation detail of the Payload(bytes) overload
// below, declared here only because that overload is a template; not a public
// API.
testing::Matcher<absl::string_view> EqualsBytes(absl::string_view expected);

// Implements EqualsTextProto() and EqualsBinaryProto(): matches a message
// equivalent to the one obtained by decoding `expected` (in `format`) as the
// actual message's type.
class EquivalentMessageMatcher {
 public:
  using is_gtest_matcher = void;

  EquivalentMessageMatcher(::conformance::WireFormat format,
                           std::string expected);

  bool MatchAndExplain(const Message& actual,
                       testing::MatchResultListener* listener) const;
  void DescribeTo(std::ostream* os) const;
  void DescribeNegationTo(std::ostream* os) const;

 private:
  ::conformance::WireFormat format_;
  std::string expected_;
};

// Implements HasUnknownFieldsInOrder(): matches a message whose unknown field
// set holds exactly the fields of `expected`, in the same order.
//
// UnknownFieldSet serializes its fields in order with a canonical encoding for
// each, so two sets hold equal fields in the same order exactly when their
// serializations are equal.  Comparing those also sidesteps copying the
// expected set, which UnknownFieldSet doesn't support.
class UnknownFieldsInOrderMatcher {
 public:
  using is_gtest_matcher = void;

  explicit UnknownFieldsInOrderMatcher(const UnknownFieldSet& expected);

  bool MatchAndExplain(const Message& actual,
                       testing::MatchResultListener* listener) const;
  void DescribeTo(std::ostream* os) const;
  void DescribeNegationTo(std::ostream* os) const;

 private:
  std::string description_;  // `expected`, formatted for descriptions
  std::string expected_;     // `expected`'s serialization
};

// Matches a result whose response holds a specific kind of error.
class FailureMatcher {
 public:
  using is_gtest_matcher = void;

  // `name` is used for descriptions (e.g. "parse error").  `failure_message`
  // is the explanation when the response isn't the expected error.
  // `binary_input_runtime_error_failure_message`, if non-empty, is used
  // instead when the response is a runtime error *and* the test's input was
  // binary, mirroring the legacy binary runner (see IsParseError()).
  FailureMatcher(
      absl::string_view name,
      ::conformance::ConformanceResponse::ResultCase expected_result,
      absl::string_view failure_message,
      absl::string_view binary_input_runtime_error_failure_message = "")
      : name_(name),
        expected_result_(expected_result),
        failure_message_(failure_message),
        binary_input_runtime_error_failure_message_(
            binary_input_runtime_error_failure_message) {}

  bool MatchAndExplain(const TestResult& result,
                       testing::MatchResultListener* listener) const;
  void DescribeTo(std::ostream* os) const { *os << "is a " << name_; }
  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not a " << name_;
  }

 private:
  std::string name_;
  ::conformance::ConformanceResponse::ResultCase expected_result_;
  std::string failure_message_;
  std::string binary_input_runtime_error_failure_message_;
};

// Implements Yields(): see the function below for the semantics.
testing::Matcher<const TestResult&> MakeYieldsMatcher(
    testing::Matcher<const TestResult&> inner);

}  // namespace internal

// Matches a result whose payload, decoded as the test's message type according
// to the output format the test requested (binary, text or JSON), matches `m`.
// `m` may be any matcher on `const Message&`, typically EqualsTextProto() or
// EqualsBinaryProto().
//
// The failure message is the inner matcher's explanation (or, if it has none,
// its description and the actual message).  A payload that can't be decoded
// fails with the legacy "<format> output we received from test was
// unparseable." message.  JSON output is decoded with the default
// json::ParseOptions, exactly like the legacy runner did.
template <typename M>
internal::ParsedPayloadMatcher ParsedPayload(M m) {
  return internal::ParsedPayloadMatcher(
      testing::SafeMatcherCast<const Message&>(std::move(m)));
}

// Like ParsedPayload(), but decodes the payload as `type` instead of the
// message type the test was run against.  Needed when the testee is asked to
// serialize unknown fields that a richer "shadow" type (e.g.
// UnknownToTestAllTypes) can decode.  Failure messages are identical to
// ParsedPayload()'s.
template <typename M>
internal::ParsedPayloadMatcher ParsedPayloadAs(
    const Descriptor* absl_nonnull type, M m) {
  return internal::ParsedPayloadMatcher(
      testing::SafeMatcherCast<const Message&>(std::move(m)), type);
}

// Matches a result whose raw payload bytes, whatever the output format, match
// `m`, any matcher on `absl::string_view`.
//
// `m` may also be the expected bytes themselves (a Wire, std::string or
// absl::string_view), which means byte-for-byte equality and yields the legacy
// "Expect: <octal>, but got: <octal>" failure message.  Bare string literals
// and `const char*` are rejected at compile time because binary data may
// contain NUL bytes; wrap them in Wire() or std::string() (a string_view built
// from a literal still stops at the first NUL).  Any other matcher's own
// explanation is used instead (or, if it has none, its description and the
// actual bytes).
//
// Like the legacy runner's `require_same_wire_format` mode, a PROTOBUF payload
// is first checked to be parseable as the test's message type (failing with
// "Protobuf output we received from test was unparseable." otherwise),
// whatever `m` is.
template <typename M>
internal::RawPayloadMatcher Payload(M m) {
  if constexpr (internal::kIsBytesLike<M>) {
    return internal::RawPayloadMatcher(
        internal::EqualsBytes(internal::BytesOf(std::move(m))));
  } else {
    static_assert(!std::is_convertible_v<M, absl::string_view>,
                  "Payload() only accepts Wire, std::string or "
                  "absl::string_view as literal bytes: string literals and "
                  "const char* would be truncated at the first NUL byte.");
    return internal::RawPayloadMatcher(
        testing::SafeMatcherCast<absl::string_view>(std::move(m)));
  }
}

// Matches a result whose JSON payload, parsed with jsoncpp (default
// CharReaderBuilder settings, like the legacy runner's validators), matches
// `m`, any matcher on `const Json::Value&` (e.g. Truly() with a lambda, or a
// MATCHER()).  The test must have asked for JSON output (SerializeJson()).
//
// Unlike ParsedPayload(), this looks at the JSON text itself rather than at
// the message it decodes to, which is what the legacy "Validator" tests need:
// the JSON spec constrains serializers more than parsers (e.g. int64 fields
// must be serialized as strings), and a round trip through the message can't
// tell the two apart.
//
// A payload that isn't valid JSON fails with the legacy "JSON payload cannot
// be parsed as valid JSON: <error>" message.  Otherwise the failure message is
// the inner matcher's explanation (or, if it has none, its description and
// the actual JSON).  Responses without a JSON payload fail like they do for
// Payload().
template <typename M>
internal::JsonPayloadMatcher JsonPayload(M m) {
  return internal::JsonPayloadMatcher(
      testing::SafeMatcherCast<const Json::Value&>(std::move(m)));
}

// Matches a message equivalent to the message described by `text`, in text
// format, parsed as the actual message's type.  Equivalence is determined by
// MessageDifferencer with NaNs comparing equal, exactly like the legacy
// conformance runner; the failure message is the legacy "Output was not
// equivalent to reference message: " followed by the differences.  Intended for
// use inside ParsedPayload().
internal::EquivalentMessageMatcher EqualsTextProto(absl::string_view text);

// Like EqualsTextProto(), but the expected message is the binary serialization
// `bytes` (a Wire, std::string or absl::string_view; see Payload() for why bare
// string literals are rejected), parsed as the actual message's type.
template <typename T>
internal::EquivalentMessageMatcher EqualsBinaryProto(T bytes) {
  static_assert(internal::kIsBytesLike<T>,
                "EqualsBinaryProto() only accepts Wire, std::string or "
                "absl::string_view: string literals and const char* would be "
                "truncated at the first NUL byte.");
  return internal::EquivalentMessageMatcher(
      ::conformance::PROTOBUF, internal::BytesOf(std::move(bytes)));
}

// Matches a message whose unknown field set holds exactly the fields of
// `expected`, in the same order; known fields are ignored.  The failure message
// is the legacy "Unknown field mismatch", which failure lists refer to.
// Intended for use inside ParsedPayload() or ParsedPayloadAs().
internal::UnknownFieldsInOrderMatcher HasUnknownFieldsInOrder(
    const UnknownFieldSet& expected);

// Matches a response containing a parse error.
//
// The failure message when the testee instead raised a runtime error depends
// on the input format, to stay byte-identical with the legacy runner: for
// binary input it is "Should have failed to parse, but raised an error
// instead.", while for text format and JSON input it is the generic "Should
// have failed to parse, but didn't.".
internal::FailureMatcher IsParseError();

// Matches a response containing a serialize error.
internal::FailureMatcher IsSerializeError();

// The matcher every conformance test's EXPECT_THAT must use, wrapped around a
// leaf matcher (or any composition of them):
//
//   EXPECT_THAT(Testee("X").ParseBinary(d, w).SerializeBinary(),
//               Yields(ParsedPayload(EqualsBinaryProto(w))));
//
// Yields() evaluates `inner` against the TestResult and is the single place
// that reports the outcome to the global TestManager and applies the failure
// list, test priority and skip policy described at the top of this file.
// Its verdict is therefore *not* simply the inner matcher's: an expected
// failure passes and an unexpected success fails.  Do not wrap Yields() itself
// in Not() or other combinators; compose the inner matcher instead.
//
// Each TestResult may be checked by Yields() exactly once: the first
// evaluation reports to the TestManager and stores its verdict in the result
// (TestResult::verdict()).  A failed verdict is replayed by any later
// evaluation, which is what lets gtest re-evaluate a failing matcher to
// explain the failure; checking a result that already passed is a bug and
// fails with "already checked".  A result that is never checked reports a
// gtest failure when it is destroyed.
template <typename M>
testing::Matcher<const internal::TestResult&> Yields(M inner) {
  return internal::MakeYieldsMatcher(
      testing::SafeMatcherCast<const internal::TestResult&>(std::move(inner)));
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_H__
