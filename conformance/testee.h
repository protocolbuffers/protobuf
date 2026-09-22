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

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
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

// TODO: b/563659337 - Possible future APIs to expand conformance coverage:
// - Add ClearUnknownFields() to InMemoryMessage
// - Add MergeFrom() method to InMemoryMessage to merge raw binary
// - Remove && qualifiers on Parse* and add InMemoryMessage::Merge that merges
//   two parsed messages
// - Add ConstructEmpty methods on Test
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

// The final result of a conformance test.  It carries the full request that
// was sent to the testee and the testee's response, and is meant to be handed
// to exactly one EXPECT_THAT(..., Yields(...)) (see matchers.h), which records
// the outcome against the expected-failure list.
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

  // The request that was sent to the testee.  This carries the input payload
  // (and therefore its format), the requested output format and any options
  // such as `print_unknown_fields`.
  const ::conformance::ConformanceRequest& request() const { return request_; }

  // The format of the output that was requested.
  ::conformance::WireFormat format() const {
    return request_.requested_output_format();
  }

  // The conformance response that was returned from the testee.  This will
  // contain either the resulting payload or an error message.
  const ::conformance::ConformanceResponse& response() const {
    return response_;
  }

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
  // Check-fails if `type` is null.
  TestResult(ConformanceTestInfo info, const Descriptor* type,
             ::conformance::ConformanceRequest request,
             ::conformance::ConformanceResponse response);
  friend class InMemoryMessage;

  // The placeholder identity of ForTesting(test_name, ...).
  static ConformanceTestInfo UnmappedTestInfo(absl::string_view test_name,
                                              TestPriority priority);

  // Reports a gtest failure if this (non-moved-from) result was never checked.
  void ReportIfUnchecked() const;

  ConformanceTestInfo info_;
  const Descriptor* type_;
  ::conformance::ConformanceRequest request_;
  ::conformance::ConformanceResponse response_;
  mutable bool checked_ = false;
  mutable absl::optional<Verdict> verdict_;
  bool moved_from_ = false;
};

// Pretty-prints a TestResult in gtest failure output, including a short form
// of the request and the response.  Large payloads are truncated.
void PrintTo(const TestResult& result, std::ostream* os);

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
// where <Level> is the priority, <Input>Input the format of the request's
// payload and <Output>Output the requested output format (omitted by
// ParseOnly()).  `suite` and `test` are the gtest suite and test names without
// the INSTANTIATE_TEST_SUITE_P prefix and without the "/<params>" part;
// `params` is that part, verbatim, "" for tests that aren't parameterized.  So
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
//   - payloads: PayloadTag() of `request`.
//   - variant_number: <syntax digit><payload digit>; see taxonomy.h.
//   - coordinate: "%02d.%02d.%03d.%02d".
ConformanceTestInfo ResolveTestInfo(
    const TestName& name, TestPriority priority,
    const ::conformance::ConformanceRequest& request, NameStyle name_style,
    TestNumberer& numberer);

// This class represents a message held in memory by the testee that can be
// manipulated in various ways.
class InMemoryMessage {
 public:
  ~InMemoryMessage() = default;

  // Serialize the message back in any of our supported formats.  These all
  // consume the message.
  TestResult SerializeBinary() &&;
  TestResult SerializeText(TextSerializationOptions options = {}) &&;
  TestResult SerializeJson() &&;

  // Finishes a test whose outcome depends only on parsing, e.g. one that
  // expects a parse error.  The testee is still asked to serialize, by default
  // in the same format the input was in, exactly like the legacy runner; but
  // the test name carries no output-format suffix:
  // "Required.FooTest.Bar.ProtobufInput" (see TestName).
  //
  // `options.output_format` requests a different output format without
  // changing the name.  It exists for the JSON tests that ask for PROTOBUF
  // output from JSON input while keeping their parse-only name (the legacy
  // RunValidJsonTestOrParseFailure); see ParseOnlyOptions.
  //
  // Like the Serialize*() methods this consumes the message and must remain
  // the terminal call of a test: once further operations on an InMemoryMessage
  // exist (Merge, reflection accessors; see the TODO at the top of this file)
  // they have to come before it, not after.
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
  // Returns *this as an rvalue and must be chained in the same expression
  // (`.ParseBinary(...).OverrideTestCategory(...).SerializeText()`); binding
  // the result to a reference outlives the temporary it refers to.
  // TODO: b/563657722 - drop once the Ruby testee skips text-format tests by
  // payload and requested output format rather than by category.
  InMemoryMessage&& OverrideTestCategory(
      ::conformance::TestCategory category) &&;

 private:
  InMemoryMessage(class Testee* testee, TestName name, TestPriority priority,
                  const Descriptor* type,
                  ::conformance::ConformanceRequest request)
      : testee_(testee),
        name_(std::move(name)),
        priority_(priority),
        type_(type),
        request_(std::move(request)) {}
  friend class Test;

  // Finishes the test: requests `output_format` from the testee and runs it
  // under the name built according to `name_style`.  All public terminal
  // methods funnel into this.
  TestResult Finish(::conformance::WireFormat output_format,
                    NameStyle name_style);

  class Testee* testee_;
  TestName name_;
  TestPriority priority_;
  const Descriptor* type_;
  ::conformance::ConformanceRequest request_;
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
  explicit Testee(ConformanceTestRunner* runner) : runner_(runner) {}

  // Creates a test of the given priority named after the enclosing gtest
  // test; see TestName.  Check-fails if `name` has an empty suite or test, or
  // a piece with a character gtest wouldn't allow.
  Test CreateTest(TestName name, TestPriority priority);

 private:
  // Runs a single test against the testee.  Check-fails on a duplicate test
  // name, since each name may only be reported once per process.
  ::conformance::ConformanceResponse Run(
      absl::string_view test_name,
      const ::conformance::ConformanceRequest& request);
  friend class InMemoryMessage;

  ConformanceTestRunner* runner_;

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
