#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_H__

#include <limits>
#include <string>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"
#include "google/protobuf/descriptor.h"

// The APIs conformance tests use to interact with a testee.  They are
// deliberately decoupled from the runner/testee protocol that implements
// them.  That keeps them flexible for tests.
//
// Tests should never need to name any of these types directly.  A test
// obtains a Test object for the global testee from Testee() (see
// test_environment.h), chains operations on it and passes the final
// TestResult to Yields() (see matchers.h):
//
//   EXPECT_THAT(Testee()
//                   .ParseBinary(TestAllTypesProto2::descriptor(), input)
//                   .SerializeBinary(),
//               Yields(ParsedPayload(EqualsBinaryProto(input))));

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
// baseline every implementation must pass.  Every priority is enforced
// unless the enforcement level (see below) leaves some out.  A failing kP1
// test is then counted but not failed, unless it is in the failure list (see
// matchers.h).  Test names still spell kP0 as "Required" and kP1 as
// "Recommended" (see PriorityLevelName()).
//
// A suite declares its priority with ConformanceTest::DefaultPriority().  A
// single test overrides it with Testee(priority); see test_environment.h.
// TODO: b/564550230 - rename the levels in test names to P0/P1 once every
// suite has been triaged.
enum class TestPriority { kP0 = 0, kP1 = 1 };

// The priorities, spelled the way suites write them: Testee(kP1).
inline constexpr TestPriority kP0 = TestPriority::kP0;
inline constexpr TestPriority kP1 = TestPriority::kP1;

// An enforcement level is the highest priority, as an int (0 for kP0, 1 for
// kP1), whose unlisted failures fail the run.  See TestManager in
// test_manager.h.  kEnforceAllPriorities, the default, enforces every
// priority, however many there are.
inline constexpr int kEnforceAllPriorities = std::numeric_limits<int>::max();

// The name of a priority: "P0" or "P1".
absl::string_view PriorityName(TestPriority priority);

// The level a priority is named with in test names, until the rename (see
// TestPriority): "Required" for kP0, "Recommended" for kP1.
absl::string_view PriorityLevelName(TestPriority priority);

namespace internal {

// The final result of a conformance test: the testee's response and what the
// test asked of it.  Hand it to exactly one EXPECT_THAT(..., Yields(...)),
// which records the outcome against the failure list (see matchers.h).
//
// Results are move-only.  A result that is destroyed without having been
// checked by Yields() reports a gtest failure.  Otherwise the outcome of its
// test would silently bypass the failure list.  A moved-from result is inert.
class TestResult {
 public:
  // The outcome Yields() reached for a result.  `matched` says whether the
  // test passed once the failure list and the test's priority were taken
  // into account.  `explanation` is the text to show for it.
  struct Verdict {
    bool matched = false;
    std::string explanation;
  };

  TestResult(TestResult&& other) noexcept;
  TestResult& operator=(TestResult&& other) noexcept;
  TestResult(const TestResult&) = delete;
  TestResult& operator=(const TestResult&) = delete;
  ~TestResult();

  // The name of the test that was run, useful for failure matching and
  // reporting.
  absl::string_view name() const { return test_name_; }

  // The priority of the test (see TestPriority).  Yields() tolerates a
  // failing kP1 test only if the enforcement level leaves kP1 out and the
  // test isn't listed.
  TestPriority priority() const { return priority_; }

  // The type of the message that was tested, needed for parsing.
  const Descriptor* type() const { return type_; }

  // The format of the output that was requested.
  ::conformance::WireFormat format() const { return format_; }

  // Whether the testee was asked to print unknown fields when serializing
  // text format, as SerializeText({/*print_unknown_fields=*/true}) does.  It
  // then prints them by field number.
  bool print_unknown_fields() const { return print_unknown_fields_; }

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
  // Records what `request` asked of the testee, without keeping the request
  // itself.
  TestResult(absl::string_view test_name, TestPriority priority,
             const Descriptor* type,
             const ::conformance::ConformanceRequest& request,
             ::conformance::ConformanceResponse response)
      : test_name_(test_name),
        priority_(priority),
        type_(type),
        format_(request.requested_output_format()),
        print_unknown_fields_(request.print_unknown_fields()),
        response_(std::move(response)) {}
  friend class InMemoryMessage;

  // Reports a gtest failure if this (non-moved-from) result was never checked.
  void ReportIfUnchecked() const;

  std::string test_name_;
  TestPriority priority_;
  const Descriptor* type_;
  ::conformance::WireFormat format_;
  bool print_unknown_fields_;
  ::conformance::ConformanceResponse response_;
  mutable bool checked_ = false;
  mutable absl::optional<Verdict> verdict_;
  bool moved_from_ = false;
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
