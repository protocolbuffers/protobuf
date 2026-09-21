// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_result.pb.h"
#include "conformance/taxonomy.h"
#include "conformance/test_runner.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

// This file defines the APIs used by conformance tests to interact with
// testees.  The structure of these APIs are intentionally decoupled from the
// runner/testee protocol (which are used to implement them), in order to
// maximize their flexibility in tests.
//
// Tests should not ever need to name any of these types directly, but will
// obtain a Test object pointing to the global testee and pass the final
// TestResult to Yields() (see matchers.h).
//
// Example:
//
// EXPECT_THAT(Testee()
//                .ParseBinary(Wire(LengthPrefixedField(1, "foo"))
//                .SerializeText({/*print_unknown_fields=*/true}),
//             Yields(ParsedPayload(EqualsTextProto(R"pb(1: "foo")pb"))));

// A test is recorded as a list of protocol actions (see ConformanceAction in
// conformance.proto) and only turned into a request when it is finished:
// into the flat protocol version 1 request if the testee speaks version 1
// and the test fits that shape, into an action list otherwise.  Tests that
// only version 2 can express (Test::New(), InMemoryMessage::MergeFrom(),
// Also*()) are therefore skipped by the runner on a version 1 testee rather
// than sent; see TestResult::Outcome::kUnsupportedProtocol.  Which version
// the testee speaks is learned in the discovery handshake, see
// Testee::DetectProtocolVersion().
//
// TODO: b/563659337 - Possible future APIs to expand conformance coverage:
// - Add reflection methods to InMemoryMessage (e.g. Get/Set/Add, and a Has that
//   returns TestResult)
// - Add a SerializeIntoMemory method that allows further action on the results
//   of serialization instead of immediately returning it
namespace google {
namespace protobuf {
namespace conformance {

// How important it is that an implementation passes a test.  kP0 is the
// baseline every implementation must pass; kP1 and kP2 are deliberate
// downgrades; kP3 is only recommended: a kP3 failure is tolerated (counted,
// not failed) unless recommended tests are enforced (--enforce_recommended)
// or the test is in the failure list (see matchers.h).  Test names still
// spell kP0-kP2 as "Required" and kP3 as "Recommended" (see
// PriorityLevelName()).
//
// A suite declares its priority with ConformanceTest::DefaultPriority() and a
// single test overrides it with Testee(priority) (see test_environment.h).
// TODO: b/564550230 - rename the levels in test names to P0..P3 once every
// suite has been triaged.
enum class TestPriority { kP0 = 0, kP1 = 1, kP2 = 2, kP3 = 3 };

// The priorities, spelled the way suites write them: Testee(kP3).
inline constexpr TestPriority kP0 = TestPriority::kP0;
inline constexpr TestPriority kP1 = TestPriority::kP1;
inline constexpr TestPriority kP2 = TestPriority::kP2;
inline constexpr TestPriority kP3 = TestPriority::kP3;

// The name of a priority: "P0" .. "P3".
absl::string_view PriorityName(TestPriority priority);

// The level a priority is named with in test names, until the rename (see
// TestPriority): "Recommended" for kP3, "Required" otherwise.
absl::string_view PriorityLevelName(TestPriority priority);

// The four-part identity of a conformance test (see taxonomy.h), resolved
// once per TestResult (see TestResult::info()) and exported with its outcome
// as a ConformanceTestCaseResult (see ToCaseResult() and
// --output_result_file).  For the test
//
//   Required.PrematureEofTest.BeforeKnownNonRepeatedValue/EditionsProto2_DOUBLE.ProtobufInput
//
// the identity is
//
//   coordinate   01.01.001.31
//   test_name
//   wire.varint.eof_before_known_non_repeated_value_double_parsefails.ed_proto2_pb2pb
//
//   part            number                name
//   --------------  --------------------  ---------------------------
//   domain          01                    wire          } from the
//   section         01                    varint        } section table
//   test case       001                   eof_before_..._parsefails
//   variant         31                    ed_proto2_pb2pb
//
// The variant number is two digits: the first identifies the syntax, the
// second the payload transform.
struct ConformanceTestInfo {
  std::string test_name;         // "wire.varint.eof_..._double.ed_proto2_pb2pb"
  std::string legacy_test_name;  // the gtest-derived name (TestResult::name())
  std::string coordinate;        // "01.01.001.31"

  std::string section_coordinate;  // "01.01"
  std::string domain;              // "wire"
  std::string section;             // "varint"
  std::string subsection;          // "eof"  (a test case name prefix only)
  int test_number = 0;             // 1
  std::string test_case;           // "eof_..._double_parsefails"
  int variant_number = 0;          // 31
  std::string variant;             // "ed_proto2_pb2pb"
  std::string syntax;              // "ed_proto2"
  std::string payloads;            // "pb2pb"

  std::string description;
  bool is_informational = false;
  TestPriority priority = TestPriority::kP0;
};

// Copies `info` onto a ConformanceTestCaseResult: everything but the outcome
// (status, error_message, matched_behavior_id, execution_time_us).
::conformance::ConformanceTestCaseResult ToCaseResult(
    const ConformanceTestInfo& info);

namespace internal {

// The newest conformance protocol version this runner can speak; see
// ConformanceRequest.protocol_version in conformance.proto.  A Testee speaks
// at most this, whatever its testee answers in the discovery handshake.
inline constexpr int kLatestProtocolVersion = 2;

// The lowest protocol version that can carry `actions`, a finished test's
// action list: 1 iff it has exactly the shape of the flat version 1 request,
//
//   Parse(0) [Parse(1) Merge(1 -> 0)] [DiscardUnknownFields(0)] Serialize(0)
//
// (with the merge payload of the input's type, and a JSON merge payload
// lenient about unknown fields iff the input is lenient JSON), 2 otherwise.
// Exposed for tests; InMemoryMessage uses it to pick the request shape and to
// decide whether the testee can run the test.
int RequiredProtocolVersion(
    absl::Span<const ::conformance::ConformanceAction> actions);

// The final result of a conformance test.  It carries the full request that
// was sent to the testee and the testee's response, and is meant to be handed
// to exactly one EXPECT_THAT(..., Yields(...)) (see matchers.h), which records
// the outcome against the expected-failure list.
//
// The matchers don't read the raw request and response: they go through the
// protocol-neutral view below (outcome(), message(), output(), ...), which is
// computed once when the result is created.  That view is what keeps the
// matchers, and therefore the failure messages that failure lists match on,
// independent of the wire protocol the testee speaks.  request() and
// response() stay available for printing and for tests of the protocol
// itself.
//
// Results are move-only.  A result that is destroyed without ever having been
// checked by Yields() reports a gtest failure, since the outcome of its test
// would otherwise silently bypass the failure list; a moved-from result is
// inert.
class TestResult {
 public:
  // The outcome Yields() reached for a result: whether the test passed once
  // the failure list and the test's priority were taken into account, and
  // the explanation to show for it.
  struct Verdict {
    bool matched = false;
    std::string explanation;
  };

  // The kind of outcome a testee reported, independent of the protocol
  // version.  The five error kinds mirror the response's error strings, which
  // every protocol version shares so that the messages built on them don't
  // change.  kUnsupportedProtocol is the one outcome the runner produces
  // itself, without asking the testee.
  enum class Outcome {
    kNoResult,        // The response carries no result at all.
    kParseError,      // The testee failed to parse an input.
    kSerializeError,  // The testee failed to serialize an output.
    kRuntimeError,    // The testee hit an error that is neither of the above.
    kTimeoutError,    // The testee didn't answer in time.
    kSkipped,         // The testee declined to run the test.
    kOutput,          // The testee answered with serialized output(s).
    // The test needs a newer protocol version than the testee speaks, so the
    // runner never sent it (see required_protocol_version()).  Yields()
    // reports such a test to the TestManager as unsupported and marks the
    // gtest test skipped.
    kUnsupportedProtocol,
  };

  // One serialized output of the testee.  `format` is the format the testee
  // answered in, which is not necessarily the one the test asked for (see
  // format()); the matchers report that mismatch.
  struct Output {
    ::conformance::WireFormat format;
    std::string payload;
  };

  // Creates a result directly, for unit tests of the matchers and other
  // plumbing that don't want to go through a Testee.  Such a result is subject
  // to the same never-checked check as a real one; tests that only inspect it
  // should call MarkChecked().  `type` must not be null.  The result's info()
  // is a placeholder identity: both of its names are `test_name`, its
  // coordinate is all zeros and its domain is "unmapped".
  static TestResult ForTesting(absl::string_view test_name,
                               TestPriority priority, const Descriptor* type,
                               ::conformance::ConformanceRequest request,
                               ::conformance::ConformanceResponse response) {
    return TestResult(UnmappedTestInfo(test_name, priority), type,
                      std::move(request), std::move(response));
  }

  // Like above, with a full identity; the result is named
  // `info.legacy_test_name` and has `info.priority`.
  static TestResult ForTesting(ConformanceTestInfo info, const Descriptor* type,
                               ::conformance::ConformanceRequest request,
                               ::conformance::ConformanceResponse response) {
    return TestResult(std::move(info), type, std::move(request),
                      std::move(response));
  }

  // Like ForTesting(), but creates the result the runner produces for a test
  // it didn't send because `request` (whose `protocol_version` is the version
  // the test requires) needs a newer protocol than the testee's
  // `testee_protocol_version`: outcome() is kUnsupportedProtocol and
  // response() is empty.
  static TestResult ForTestingUnsupportedProtocol(
      absl::string_view test_name, TestPriority priority,
      const Descriptor* type, ::conformance::ConformanceRequest request,
      int testee_protocol_version) {
    return TestResult(UnmappedTestInfo(test_name, priority), type,
                      std::move(request),
                      UnsupportedProtocol{testee_protocol_version});
  }

  TestResult(TestResult&& other) noexcept;
  TestResult& operator=(TestResult&& other) noexcept;
  TestResult(const TestResult&) = delete;
  TestResult& operator=(const TestResult&) = delete;
  ~TestResult();

  // The name of the test that was run, useful for failure matching and
  // reporting.
  absl::string_view name() const { return info_.legacy_test_name; }

  // The priority of the test; see TestPriority.  Yields() tolerates a failing
  // kP3 test unless recommended tests are enforced or the test is listed.
  TestPriority priority() const { return info_.priority; }

  // The coordinate identity of the test; see ConformanceTestInfo.  Its
  // `legacy_test_name` is name() and its `priority` is priority().
  const ConformanceTestInfo& info() const { return info_; }

  // The type of the message that was tested, needed for parsing.  Never null.
  const Descriptor* type() const { return type_; }

  // The request that was sent to the testee: the flat protocol version 1
  // request, or a version 2 action list (see conformance.proto), depending on
  // the version the testee speaks and the test needs.  For a
  // kUnsupportedProtocol result this is the request that would have been
  // sent.  Prefer the protocol-neutral accessors below over reading it.
  const ::conformance::ConformanceRequest& request() const { return request_; }

  // The protocol version of request(): 1 for the flat request, otherwise its
  // `protocol_version`.  For a kUnsupportedProtocol result, the version the
  // test requires.
  int required_protocol_version() const { return required_protocol_version_; }

  // The format of the output the test's terminal Serialize*() or ParseOnly()
  // call asked for: the requested output format of a version 1 request, or
  // the format of the first SerializeAction of a version 2 request.
  ::conformance::WireFormat format() const { return format_; }

  // The conformance response that was returned from the testee.  This will
  // contain either the resulting payload or an error message.
  const ::conformance::ConformanceResponse& response() const {
    return response_;
  }

  // What kind of result the testee reported.
  Outcome outcome() const { return outcome_; }

  // The message the testee attached to an error or a skip; empty for every
  // other outcome.
  absl::string_view message() const { return message_; }

  // The first serialized output, or null unless outcome() is kOutput.  This is
  // the output the test's terminal Serialize*() call asked for.
  const Output* output() const {
    return outputs_.empty() ? nullptr : &outputs_.front();
  }

  // Every serialized output, in the order they were requested: the terminal
  // Serialize*() call's first, then the ones queued by Also*(); empty unless
  // outcome() is kOutput.  The v1 protocol carries at most one.
  absl::Span<const Output> outputs() const { return outputs_; }

  // The format of the first input the testee was asked to parse, i.e. the
  // one the test's name is derived from.  UNSPECIFIED if the request carries
  // no input at all: a test built from Test::New() alone, or a result built
  // with ForTesting() from an empty request.
  ::conformance::WireFormat input_format() const { return input_format_; }

  // For a version 2 response that reports an error: the index of the action
  // that failed (ConformanceResponse.failed_action), if the testee gave one.
  absl::optional<int> failed_action() const { return failed_action_; }

  // Whether the testee was asked to print unknown fields when serializing
  // text format (SerializeText({/*print_unknown_fields=*/true})).  The
  // matchers accept field numbers in text output only in that case.
  bool print_unknown_fields() const { return print_unknown_fields_; }

  // Whether this result has been checked by Yields() (or MarkChecked()).
  bool checked() const { return checked_; }

  // Marks this result as checked without recording a verdict.  Tests that
  // inspect a result directly instead of matching it must call this, or the
  // destructor reports the result as never checked.
  void MarkChecked() const { checked_ = true; }

  // Records the verdict Yields() reached for this result and marks it checked.
  // Must be called at most once, on an unchecked result.
  void SetVerdict(Verdict verdict) const;

  // The verdict recorded by SetVerdict(), if any.  A checked result without a
  // verdict was marked checked by MarkChecked().
  const absl::optional<Verdict>& verdict() const { return verdict_; }

 private:
  // Tag for the constructor of a kUnsupportedProtocol result.
  struct UnsupportedProtocol {
    int testee_protocol_version;
  };

  // Check-fails if `type` is null.
  TestResult(ConformanceTestInfo info, const Descriptor* type,
             ::conformance::ConformanceRequest request,
             ::conformance::ConformanceResponse response);
  // Creates a kUnsupportedProtocol result for a test that was never sent;
  // see ForTestingUnsupportedProtocol().  Check-fails if `type` is null.
  TestResult(ConformanceTestInfo info, const Descriptor* type,
             ::conformance::ConformanceRequest request,
             UnsupportedProtocol unsupported);
  friend class InMemoryMessage;

  // The placeholder identity of ForTesting(test_name, ...).
  static ConformanceTestInfo UnmappedTestInfo(absl::string_view test_name,
                                              TestPriority priority);

  // Computes the parts of the view that only depend on request_.
  void InitializeFromRequest();

  // Reports a gtest failure if this (non-moved-from) result was never checked.
  void ReportIfUnchecked() const;

  ConformanceTestInfo info_;
  const Descriptor* type_;
  ::conformance::ConformanceRequest request_;
  ::conformance::ConformanceResponse response_;
  // The protocol-neutral view of request_ and response_, computed by the
  // constructor.  message_ is a copy rather than a view into response_ so
  // that moving a result can't leave it dangling.
  Outcome outcome_ = Outcome::kNoResult;
  std::string message_;
  std::vector<Output> outputs_;
  int required_protocol_version_ = 1;
  ::conformance::WireFormat format_ = ::conformance::UNSPECIFIED;
  ::conformance::WireFormat input_format_ = ::conformance::UNSPECIFIED;
  bool print_unknown_fields_ = false;
  absl::optional<int> failed_action_;
  mutable bool checked_ = false;
  mutable absl::optional<Verdict> verdict_;
  bool moved_from_ = false;
};

// Pretty-prints a TestResult in gtest failure output, including a short form
// of the request and the response.  Large payloads are truncated.
void PrintTo(const TestResult& result, std::ostream* os);

// Prints an outcome by name (e.g. "kParseError") in gtest failure output.
void PrintTo(TestResult::Outcome outcome, std::ostream* os);

// Creates an empty message of the given type.  Generated types are served by
// the generated factory; everything else is backed by a dynamic message from a
// process-wide factory, so returned messages remain valid for the life of the
// process.
std::unique_ptr<Message> NewMessage(const Descriptor* type);

// Prints a message on a single line (expanding Any, short repeated
// primitives), for failure output.
std::string ToShortString(const Message& message);

// Options for serializing text format.
struct TextSerializationOptions {
  bool print_unknown_fields = false;
};

// Options for InMemoryMessage::ParseOnly().
struct ParseOnlyOptions {
  // The output format to request from the testee.  Defaults to the input's
  // format, like the legacy runner's parse-failure tests; the legacy
  // RunValidJsonTestOrParseFailure requested PROTOBUF for JSON input.
  absl::optional<::conformance::WireFormat> output_format;
};

// The pieces of a conformance test's name that come from the enclosing gtest
// test (see Testee() in test_environment.h).  The full name is
//
//   <Level>.<Suite>.<Test>[/<Params>][.<Suffix>].<Input>Input[.<Output>Output]
//
// where <Level> is the priority, <Input>Input the format of the first
// payload the test parses ("EmptyInput" for a test that parses nothing, i.e.
// one built from Test::New() alone) and <Output>Output the requested output
// format (omitted by ParseOnly()).  `suite` and `test` are the gtest suite and
// test names without the INSTANTIATE_TEST_SUITE_P prefix and without the
// "/<params>" part; `params` is that part, verbatim, "" for tests that aren't
// parameterized.  So
// "<Suite>.<Test>[/<Params>]" is exactly the gtest test's own name, e.g.
// "PrematureEofTest.BeforeKnownNonRepeatedValue/Proto3_INT32", as passed to
// --gtest_filter.  `suffix` tells apart several requests one test body sends
// with the same input and output formats; it is usually empty.
//
// Every non-empty piece must consist of [A-Za-z0-9_] only, which is what gtest
// allows in test and parameter names; "." and "/" are the name's separators.
struct TestName {
  std::string suite;
  std::string test;
  std::string params;
  std::string suffix;
};

// Whether a test's full name ends in the requested output format:
//   kWithOutputFormat     ....<Output>Output
//   kWithoutOutputFormat  ...                (ParseOnly())
enum class NameStyle {
  kWithOutputFormat,
  kWithoutOutputFormat,
};

// Resolves the identity of a test (see ConformanceTestInfo) from the pieces
// its gtest-derived name is built from: `legacy_test_name` is that name.
//
//   - domain, section, subsection: SectionOf(name.suite, name.test,
//     field_type) (taxonomy.h), where field_type is the first component of
//     `name.params` after the syntax component (see below), if any; an
//     unmapped suite is filed under domain 0, "unmapped", with the suite
//     (without its "Test" suffix) in snake_case as its section.
//   - test_case: the subsection, the suite (without its "Test" suffix), the
//     test, the parameter components after the syntax component and the
//     suffix, each in snake_case and joined with "_", followed by
//     "_parsefails" for a test whose name carries no output format
//     (kWithoutOutputFormat, i.e. ParseOnly()).  The suite is left out when
//     it ends in the subsection or the test starts with it.  Parameter
//     components go through ParamToSnakeCase(), so "inf" and "INF" stay
//     distinct.  Every variant of the same logical test shares this name and
//     therefore its test number, which `numberer` assigns per section.
//   - syntax: SyntaxTag() of the first "_"-separated component of
//     `name.params`, "" if that component isn't a syntax.
//   - payloads: PayloadTag(input_format, output_format), where the formats
//     are those the test asked for (its first Parse*() and terminal
//     Serialize*()), whichever protocol version carries them, so that the
//     identity doesn't depend on the version spoken to the testee.
//   - variant_number: <syntax digit><payload digit>; see taxonomy.h.
//   - coordinate: "%02d.%02d.%03d.%02d".
ConformanceTestInfo ResolveTestInfo(const TestName& name, TestPriority priority,
                                    ::conformance::WireFormat input_format,
                                    ::conformance::WireFormat output_format,
                                    NameStyle name_style,
                                    TestNumberer& numberer);

// This class represents a message held in memory by the testee that can be
// manipulated in various ways.  Every operation is recorded as a protocol
// action; nothing reaches the testee before a terminal call (Serialize*() or
// ParseOnly()).  See the file comment for how the recorded actions become a
// request.
class InMemoryMessage {
 public:
  ~InMemoryMessage() = default;

  // Serialize the message back in any of our supported formats.  These all
  // consume the message.
  TestResult SerializeBinary() &&;
  TestResult SerializeText(TextSerializationOptions options = {}) &&;
  TestResult SerializeJson() &&;

  // Asks the testee to merge a second payload into the message, as MergeFrom
  // would, after parsing succeeded and before serializing.  The payload's
  // format is independent of the parsed input's.  A JSON merge payload is
  // parsed with the same `ignore_unknown_fields` as the input: it is lenient
  // only after ParseJson(..., JsonParseOptions{/*ignore_unknown_fields=*/true})
  // and always strict after a binary or text input.  Whether an explicit
  // default value in the merge payload overwrites a field without presence is
  // unspecified (see conformance.proto), so don't write tests that depend on
  // it.  The test's name doesn't change; see DiscardUnknownFields().
  //
  // These are the shortcuts for the one merge shape protocol version 1 can
  // express (its `merge_payload`), and keep to it: a test merges at most one
  // payload this way (a second Merge*() is a DCHECK failure), and merging
  // happens before discarding unknown fields, so Merge*() must precede
  // DiscardUnknownFields() in the chain.  Anything else (several merges, a
  // merge after DiscardUnknownFields(), a merge source that is itself
  // manipulated first) is MergeFrom() below, which needs protocol version 2.
  // Testees that don't support merging answer with `skipped`.
  //
  // Returns *this as an rvalue and must be chained in the same expression,
  // like OverrideTestCategory().
  InMemoryMessage&& MergeBinary(Wire input) &&;
  InMemoryMessage&& MergeText(absl::string_view input) &&;
  InMemoryMessage&& MergeJson(absl::string_view input) &&;

  // Merges `other`, a second message of the same test, into this one, as
  // this->MergeFrom(other) would, at this point of the chain.  `other` is
  // built with the same Testee()/Testee(kP3) call (the same
  // TestName and priority, on the same testee; anything else is a DCHECK
  // failure) and consumed: its own actions become part of this test, ahead of
  // the merge, and it is never sent on its own.  So a test that merges a
  // parsed message with its unknown fields discarded into an empty one reads
  //
  //   Testee()
  //       .New(TestAllTypesProto2::descriptor())
  //       .MergeFrom(Testee()
  //                      .ParseBinary(TestAllTypesProto2::descriptor(), input)
  //                      .DiscardUnknownFields())
  //       .SerializeBinary()
  //
  // Unlike Merge*(), this may be called any number of times, before or after
  // DiscardUnknownFields(), and `other` may itself have merged messages;
  // `other` must not have queued extra outputs with Also*() nor relabeled its
  // category with OverrideTestCategory() (DCHECK failures: both apply to the
  // message a test serializes).  The test's name doesn't change: it is
  // derived from the first payload parsed, which is this message's unless it
  // was created by New().  `other` may be of another message type than this
  // one; what the testee does with such a merge is up to it (the C++ harness
  // answers a runtime_error).
  //
  // Needs protocol version 2 unless the whole test happens to have the shape
  // Merge*() produces (a merely parsed `other` of this message's type, merged
  // before DiscardUnknownFields()), which is sent as the same flat request;
  // otherwise a testee that speaks version 1 never sees the test (see
  // TestResult::Outcome::kUnsupportedProtocol).  Returns *this as an rvalue
  // and must be chained in the same expression.
  InMemoryMessage&& MergeFrom(InMemoryMessage&& other) &&;

  // Asks the testee to discard the message's unknown fields (recursively, as
  // C++'s Message::DiscardUnknownFields() does) at this point of the chain,
  // i.e. after parsing and any merge before it, and before serializing.  This
  // makes it possible to tell a value the testee parsed into a field from one
  // it kept as an unknown field, which a plain round trip re-emits either
  // way. The test's name doesn't change; a test that sends the same input with
  // and without this uses TestName::suffix to tell the two requests apart.
  //
  // Testees that don't support the option answer such a request with
  // `skipped`.
  //
  // Returns *this as an rvalue and must be chained in the same expression, like
  // OverrideTestCategory().
  InMemoryMessage&& DiscardUnknownFields() &&;

  // Ask the testee for additional outputs of this message, in the given
  // formats, on top of the one the terminal Serialize*() call asks for.  The
  // extra outputs follow the terminal one in TestResult::outputs(), in the
  // order these were called: outputs()[0] is the terminal Serialize*()'s and
  // outputs()[1], outputs()[2], ... the ones queued here.  The test's name
  // only carries the terminal call's format.
  //
  // Need protocol version 2 (see MergeFrom()).  Return *this as an rvalue
  // and must be chained in the same expression.
  InMemoryMessage&& AlsoBinary() &&;
  InMemoryMessage&& AlsoText(TextSerializationOptions options = {}) &&;
  InMemoryMessage&& AlsoJson() &&;

  // Finishes a test whose outcome depends only on parsing, e.g. one that
  // expects a parse error.  The testee is still asked to serialize, by default
  // in the same format the input was in, exactly like the legacy runner; but
  // the test name carries no output-format suffix:
  // "Required.FooTest.Bar.ProtobufInput" (see TestName).
  //
  // `options.output_format` requests a different output format without
  // changing the name.  It exists for the JSON tests that ask for PROTOBUF
  // output from JSON input while keeping their parse-only name (the legacy
  // RunValidJsonTestOrParseFailure); see ParseOnlyOptions.  It is required
  // for a test that parses nothing (Test::New() alone), which has no input
  // format to default to; forgetting it check-fails.
  //
  // Like the Serialize*() methods this consumes the message and is the
  // terminal call of a test: Merge*(), MergeFrom(), DiscardUnknownFields() and
  // Also*() (and any future operation, see the TODO at the top of this file)
  // come before it, not after.
  TestResult ParseOnly(ParseOnlyOptions options = {}) &&;

  // Relabels a PROTOBUF-input request as TEXT_FORMAT_TEST (the only category
  // accepted; anything else is a DCHECK failure).  ParseBinary() categorizes
  // its request as BINARY_TEST, but the text-format suite's binary-input tests
  // (text_unknown_field_test.cc) have to stay TEXT_FORMAT_TEST, as in the
  // legacy suite: no testee tells BINARY_TEST and JSON_TEST apart on a binary
  // payload, but the OSS Ruby testee skips every TEXT_FORMAT_TEST request and
  // cannot produce text-format output, so relabeling those tests would turn
  // its skips into failures that no failure list covers.  The JSON suite's
  // binary-input tests, which the legacy suite categorized as JSON_TEST, are
  // plain BINARY_TEST requests now.
  //
  // Test categories only exist in protocol version 1; a version 2 request has
  // none, so this has no effect on one.
  //
  // Returns *this as an rvalue and must be chained in the same expression
  // (`.ParseBinary(...).OverrideTestCategory(...).SerializeText()`); binding
  // the result to a reference outlives the temporary it refers to.
  // TODO: b/563657722 - drop once the Ruby testee skips text-format tests by
  // payload and requested output format rather than by category.
  InMemoryMessage&& OverrideTestCategory(
      ::conformance::TestCategory category) &&;

 private:
  // `actions` creates this message as handle 0 (a ParseAction or NewAction)
  // and is the start of the test's action list.
  InMemoryMessage(class Testee* testee, TestName name, TestPriority priority,
                  const Descriptor* type,
                  std::vector<::conformance::ConformanceAction> actions)
      : testee_(testee),
        name_(std::move(name)),
        priority_(priority),
        type_(type),
        actions_(std::move(actions)) {}
  friend class Test;

  // Finishes the test: appends the terminal SerializeAction `serialize` (for
  // handle 0) and the extra ones queued by Also*(), works out the protocol
  // version the action list needs, sends the request in the shape the testee
  // speaks (or doesn't send it at all, see the file comment) and returns the
  // result under the name built according to `name_style`.  All public
  // terminal methods funnel into this.
  TestResult Finish(::conformance::SerializeAction serialize,
                    NameStyle name_style);

  // Appends a ParseAction for a fresh handle and a MergeAction of it into
  // handle 0; the Merge*() shortcuts.  `parse` has its type and payload set.
  InMemoryMessage&& MergeParsed(::conformance::ParseAction parse);

  // Queues an extra SerializeAction of handle 0; the Also*() methods.
  InMemoryMessage&& QueueSerialize(::conformance::SerializeAction serialize);

  // DCHECKs that a Merge*() call is allowed: none was made yet, and
  // DiscardUnknownFields() hasn't been called.
  void CheckCanMerge() const;

  class Testee* testee_;
  TestName name_;
  TestPriority priority_;
  const Descriptor* type_;
  // The test so far, in order.  Handle 0 is this message; handles are
  // numbered in order of creation, the next one being next_handle_.  Never
  // holds a SerializeAction before Finish(): the terminal one is appended
  // there, followed by extra_serializes_.
  std::vector<::conformance::ConformanceAction> actions_;
  int next_handle_ = 1;
  std::vector<::conformance::SerializeAction> extra_serializes_;
  // Set by OverrideTestCategory(); only the version 1 request has a category.
  absl::optional<::conformance::TestCategory> test_category_override_;
};

// Options for parsing JSON.
struct JsonParseOptions {
  bool ignore_unknown_fields = false;
};

// This class represents a single test case representing some interaction with
// the testee.  The end result of a test should be a single TestResult.
class Test {
 public:
  ~Test() = default;

  // Parse the message from one of our supported formats into an in-memory
  // message for further processing.
  InMemoryMessage ParseBinary(const Descriptor* type, Wire input) &&;
  InMemoryMessage ParseText(const Descriptor* type, absl::string_view input) &&;
  InMemoryMessage ParseJson(const Descriptor* type, absl::string_view input,
                            JsonParseOptions options = {}) &&;

  // Creates an empty (default) message of `type` in the testee, to merge
  // parsed messages into (see InMemoryMessage::MergeFrom()) or to serialize
  // as is.  A test that parses nothing is named with "EmptyInput" in place of
  // the input format (see TestName) and has to say what ParseOnly() should ask
  // for (see ParseOnlyOptions).
  //
  // Needs protocol version 2, so a testee that speaks version 1 never sees
  // the test (see TestResult::Outcome::kUnsupportedProtocol).
  InMemoryMessage New(const Descriptor* type) &&;

 private:
  Test(class Testee* testee, TestName name, TestPriority priority)
      : testee_(testee), name_(std::move(name)), priority_(priority) {}
  friend class Testee;

  class Testee* testee_;
  TestName name_;
  TestPriority priority_;
};

// This class represents an abstraction of the testee.  It is used to
// create Test objects that can be used to interact further for testing.
class Testee {
 public:
  // `protocol_version` is the conformance protocol version to speak to the
  // testee until DetectProtocolVersion() says otherwise: 1 (the flat request)
  // unless told otherwise; see ConformanceRequest.protocol_version in
  // conformance.proto.  Tests that need a newer version are not sent to it
  // (see TestResult::Outcome::kUnsupportedProtocol).  Check-fails unless it is
  // between 1 and kLatestProtocolVersion.
  //
  // A Testee created this way does not probe the testee: unit tests that
  // drive a Testee against a mock runner see exactly the requests of their
  // tests.  ConformanceEnvironment::SetUp() runs the discovery handshake on
  // the testee it owns, so conformance test binaries always detect.
  explicit Testee(ConformanceTestRunner* runner, int protocol_version = 1);

  // The discovery handshake.  Sends the testee one probe request, which is at
  // the same time a valid version 1 and a valid version 2 request (its exact
  // shape is documented at ConformanceRequest.protocol_version and frozen
  // forever, since it is all a version 1 testee ever sees of the handshake),
  // under the name kProbeName (test_runner.h), and reads the version the
  // testee implements off the response's `protocol_version`: absent means 1,
  // which is why a testee implementing version 2 or later has to set it on
  // every response.  Whatever else the response says (an error, a skip, an
  // output) it is taken as an answer; a version newer than
  // kLatestProtocolVersion is clamped to it with a warning.  Only a testee
  // that doesn't answer at all (the runner reports a timeout, or something
  // that isn't a ConformanceResponse, or a filtering runner answered with
  // kTestNotSelectedSkipReason instead of forwarding the probe) is an error,
  // "could not probe the testee's protocol version: ...".
  //
  // `pinned_protocol_version` pins the version to speak: 0 (the default)
  // adopts the detected version; N adopts N provided the testee answered with
  // N or newer, and is otherwise the error "--protocol_version=N pins the
  // testee to protocol vN, but it answered the probe as vM".  Check-fails
  // unless it is 0 or between 1 and kLatestProtocolVersion.
  //
  // On success protocol_version() is the version adopted, logged once at
  // INFO; on error it is unchanged.  The probe is not a test: it has no test
  // name, is never reported to the TestManager and doesn't count as a test
  // run.  Must be called before the first test, if at all (a DCHECK failure
  // otherwise), and is normally only called by ConformanceEnvironment.
  absl::Status DetectProtocolVersion(int pinned_protocol_version = 0);

  // The protocol version spoken to the testee: the constructor's until
  // DetectProtocolVersion() succeeds, the version it adopted afterwards.
  int protocol_version() const { return protocol_version_; }

  // Creates a test of the given priority named after the enclosing gtest
  // test; see TestName.  Check-fails if `name` has an empty suite or test, or
  // a piece with a character gtest wouldn't allow.
  Test CreateTest(TestName name, TestPriority priority);

 private:
  // Records that a test named `test_name` was run (or, for a test the runner
  // skips, would have been).  Check-fails on a duplicate test name, since each
  // name may only be reported once per process.
  void RegisterTestName(absl::string_view test_name);

  // Runs a single test against the testee; see RegisterTestName() for the
  // duplicate-name check.
  ::conformance::ConformanceResponse Run(
      absl::string_view test_name,
      const ::conformance::ConformanceRequest& request);
  friend class InMemoryMessage;

  ConformanceTestRunner* runner_;
  int protocol_version_;

  absl::flat_hash_set<std::string> test_names_ran_;

  // Assigns the test numbers of the results' identities (see
  // ConformanceTestInfo).  Numbers are per process, like the testee itself:
  // the environment creates one Testee for the whole run.
  TestNumberer numberer_;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__
