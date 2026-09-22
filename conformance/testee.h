#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__

#include <string>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"
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
// EXPECT_THAT(Testee()
//                .ParseBinary(Wire(LengthPrefixedField(1, "foo"))
//                .SerializeText({/*print_unknown_fields=*/true}),
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

namespace internal {

// The final result of a conformance test, to be processed by a matcher.
class TestResult {
 public:
  // The name of the test that was run, useful for failure matching and
  // reporting.
  absl::string_view name() const { return test_name_; }

  // The priority of the test; see TestPriority.  Yields() tolerates a failing
  // kP3 test unless recommended tests are enforced or the test is listed.
  TestPriority priority() const { return priority_; }

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
  TestResult(absl::string_view test_name, TestPriority priority,
             const Descriptor* type, ::conformance::WireFormat format,
             ::conformance::ConformanceResponse response)
      : test_name_(test_name),
        priority_(priority),
        type_(type),
        format_(format),
        response_(std::move(response)) {}
  friend class InMemoryMessage;

  std::string test_name_;
  TestPriority priority_;
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
                  TestPriority priority, const Descriptor* type,
                  ::conformance::ConformanceRequest request)
      : testee_(testee),
        name_(name),
        priority_(priority),
        type_(type),
        request_(std::move(request)) {}
  friend class Test;

  TestResult SerializeImpl(::conformance::WireFormat format);

  class Testee* testee_;
  std::string name_;
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
  Test(class Testee* testee, absl::string_view name, TestPriority priority)
      : testee_(testee), name_(name), priority_(priority) {}
  friend class Testee;

  class Testee* testee_;
  std::string name_;
  TestPriority priority_;
};

// This class represents an abstraction of the testee.  It is used to
// create Test objects that can be used to interact further for testing.
class Testee {
 public:
  explicit Testee(ConformanceTestRunner* runner) : runner_(runner) {}

  Test CreateTest(absl::string_view name, TestPriority priority) {
    return Test(this, name, priority);
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
