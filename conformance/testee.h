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
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "test_runner.h"
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
// EXPECT_THAT(RequiredTest()
//                .ParseBinary(Wire(LengthPrefixedField(1, "foo"))
//                .SerializeText({.print_unknown_fields = true}),
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
namespace internal {

// The strictness of a test.  Required tests fail the test suite if they fail.
// Recommended tests are only reported as a warning when they fail, unless
// recommended tests are enforced or the test is in the failure list (see
// matchers.h).
enum class TestStrictness {
  kRequired = 0,
  kRecommended = 1,
};

// The name of a strictness as used in test names: "Required" or "Recommended".
absl::string_view StrictnessName(TestStrictness strictness);

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
  // the failure list and the test's strictness were taken into account, and
  // the explanation to show for it.
  struct Verdict {
    bool matched = false;
    std::string explanation;
  };

  // Creates a result directly, for unit tests of the matchers and other
  // plumbing that don't want to go through a Testee.  Such a result is subject
  // to the same never-checked check as a real one; tests that only inspect it
  // should call MarkChecked().  `type` must not be null.
  static TestResult ForTesting(absl::string_view test_name,
                               TestStrictness strictness,
                               const Descriptor* type,
                               ::conformance::ConformanceRequest request,
                               ::conformance::ConformanceResponse response) {
    return TestResult(test_name, strictness, type, std::move(request),
                      std::move(response));
  }

  TestResult(TestResult&& other) noexcept;
  TestResult& operator=(TestResult&& other) noexcept;
  TestResult(const TestResult&) = delete;
  TestResult& operator=(const TestResult&) = delete;
  ~TestResult();

  // The name of the test that was run, useful for failure matching and
  // reporting.
  absl::string_view name() const { return test_name_; }

  // The strictness of the test.
  TestStrictness strictness() const { return strictness_; }

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
  TestResult(absl::string_view test_name, TestStrictness strictness,
             const Descriptor* type, ::conformance::ConformanceRequest request,
             ::conformance::ConformanceResponse response);
  friend class InMemoryMessage;

  // Reports a gtest failure if this (non-moved-from) result was never checked.
  void ReportIfUnchecked() const;

  std::string test_name_;
  TestStrictness strictness_;
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

// Options for serializing JSON.
struct JsonSerializationOptions {
  // The layout of the test's full name.  The legacy runner named a few kinds
  // of JSON tests differently from the usual
  // "<Strictness>.<Edition>.<Input>Input.<name>.<Output>Output"; those
  // layouts are kept so that existing failure lists keep matching.
  // TODO: b/563658359 - Rename the tests and delete this.
  enum class LegacyName {
    kDefault,             // <S>.<E>.JsonInput.<name>.JsonOutput
    kValidatorSuffix,     // <S>.<E>.JsonInput.<name>.Validator
                          // (legacy RunValidJsonTestWithValidator)
    kWithoutInputFormat,  // <S>.<E>.<name>.JsonOutput
                          // (legacy ExpectSerializeFailureForJson)
  };
  LegacyName legacy_name = LegacyName::kDefault;
};

// Options for InMemoryMessage::ParseOnly().
struct ParseOnlyOptions {
  // The output format to request from the testee.  Defaults to the input's
  // format, like the legacy runner's parse-failure tests; the legacy
  // RunValidJsonTestOrParseFailure requested PROTOBUF for JSON input.
  absl::optional<::conformance::WireFormat> output_format;
};

// How a test's full name is derived from the name it was created with:
//   kWithOutputFormat     <S>.<E>.<Input>Input.<name>.<Output>Output
//   kWithoutOutputFormat  <S>.<E>.<Input>Input.<name>   (ParseOnly())
//   kValidatorSuffix      <S>.<E>.<Input>Input.<name>.Validator
//   kWithoutInputFormat   <S>.<E>.<name>.<Output>Output
// The last two are legacy JSON layouts (JsonSerializationOptions::LegacyName).
enum class NameStyle {
  kWithOutputFormat,
  kWithoutOutputFormat,
  kValidatorSuffix,
  kWithoutInputFormat,
};

// This class represents a message held in memory by the testee that can be
// manipulated in various ways.
class InMemoryMessage {
 public:
  ~InMemoryMessage() = default;

  // Serialize the message back in any of our supported formats.  These all
  // consume the message.
  TestResult SerializeBinary() &&;
  TestResult SerializeText(TextSerializationOptions options = {}) &&;
  TestResult SerializeJson(JsonSerializationOptions options = {}) &&;

  // Finishes a test whose outcome depends only on parsing, e.g. one that
  // expects a parse error.  The testee is still asked to serialize, by default
  // in the same format the input was in, exactly like the legacy runner; but
  // the test name carries no output-format suffix:
  // "Required.Proto3.ProtobufInput.<name>".
  //
  // `options.output_format` requests a different output format without
  // changing the name.  It exists for the legacy JSON tests that asked for
  // PROTOBUF output from JSON input while still being named
  // "<S>.<E>.JsonInput.<name>" (RunValidJsonTestOrParseFailure); see
  // ParseOnlyOptions.
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
  InMemoryMessage(class Testee* testee, absl::string_view name,
                  TestStrictness strictness, const Descriptor* type,
                  ::conformance::ConformanceRequest request)
      : testee_(testee),
        name_(name),
        strictness_(strictness),
        type_(type),
        request_(std::move(request)) {}
  friend class Test;

  // Finishes the test: requests `output_format` from the testee and runs it
  // under the name built according to `name_style`.  All public terminal
  // methods funnel into this.
  TestResult Finish(::conformance::WireFormat output_format,
                    NameStyle name_style);

  class Testee* testee_;
  std::string name_;
  TestStrictness strictness_;
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
  Test(class Testee* testee, absl::string_view name, TestStrictness strictness)
      : testee_(testee), name_(name), strictness_(strictness) {}
  friend class Testee;

  class Testee* testee_;
  std::string name_;
  TestStrictness strictness_;
};

// This class represents an abstraction of the testee.  It is used to
// create Test objects that can be used to interact further for testing.
class Testee {
 public:
  explicit Testee(ConformanceTestRunner* runner) : runner_(runner) {}

  Test CreateTest(absl::string_view name, TestStrictness strictness) {
    return Test(this, name, strictness);
  }

 private:
  // Runs a single test against the testee.  Check-fails on a duplicate test
  // name, since each name may only be reported once per process.
  ::conformance::ConformanceResponse Run(
      absl::string_view test_name,
      const ::conformance::ConformanceRequest& request);
  friend class InMemoryMessage;

  ConformanceTestRunner* runner_;

  absl::flat_hash_set<std::string> test_names_ran_;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__
