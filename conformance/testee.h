#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__

#include <string>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "test_runner.h"
#include "google/protobuf/descriptor.h"

// This file defines the APIs used by conformance tests to interact with
// testees.  The structure of these APIs are intentionally decoupled from the
// runner/testee protocol (which are used to implement them), in order to
// maximize their flexibility in tests.
//
// Tests should not ever need to name any of these types directly, but will
// obtain a Test object pointing to the global testee and pass the final
// TestResult to one of our matchers.
//
// Example:
//
// EXPECT_THAT(RequiredTest()
//                .ParseBinary(Wire(LengthPrefixedField(1, "foo"))
//                .SerializeText({.print_unknown_fields = true}),
//             ParsedPayload(EqualsProto("pb(1: "foo")pb")));

// TODO Possible future APIs to expand conformance coverage:
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

// The strictness of a test.  Required tests will fail the test suite if they
// fail.  Recommended tests will not fail the test suite if they fail, but will
// be reported as a warning.
enum class TestStrictness {
  kRequired = 0,
  kRecommended = 1,
};

// The final result of a conformance test, to be processed by a matcher.
class TestResult {
 public:
  // The name of the test that was run, useful for failure matching and
  // reporting.
  absl::string_view name() const { return test_name_; }

  // The strictness of the test.
  TestStrictness strictness() const { return strictness_; }

  // The type of the message that was tested, needed for parsing.
  const Descriptor* type() const { return type_; }

  // The format of the output that was requested.
  ::conformance::WireFormat format() const { return format_; }

  // The conformance response that was returned from the testee.  This will
  // contain either the resulting payload or an error message.
  const ::conformance::ConformanceResponse& response() const {
    return response_;
  }

 private:
  TestResult(absl::string_view test_name, TestStrictness strictness,
             const Descriptor* type, ::conformance::WireFormat format,
             ::conformance::ConformanceResponse response)
      : test_name_(test_name),
        strictness_(strictness),
        type_(type),
        format_(format),
        response_(std::move(response)) {}
  friend class InMemoryMessage;

  std::string test_name_;
  TestStrictness strictness_;
  const Descriptor* type_;
  ::conformance::WireFormat format_;
  ::conformance::ConformanceResponse response_;
};

// Options for serializing text format.
struct TextSerializationOptions {
  bool print_unknown_fields = false;
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
  TestResult SerializeJson() &&;

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

  TestResult SerializeImpl(::conformance::WireFormat format);

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
