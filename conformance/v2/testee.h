#ifndef GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_RUNNER_H__

#include <string>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_test.h"
#include "conformance/v2/binary_wireformat.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace internal {

enum class TestStrictness {
  kRequired = 0,
  kRecommended = 1,
};

class TestResult {
 public:
  TestResult(absl::string_view test_name, const Descriptor* type,
             conformance::ConformanceRequest request,
             conformance::ConformanceResponse response)
      : test_name_(test_name),
        type_(type),
        request_(std::move(request)),
        response_(std::move(response)) {}

  absl::string_view name() const { return test_name_; }
  const Descriptor* type() const { return type_; }
  const conformance::ConformanceRequest& request() const { return request_; }
  const conformance::ConformanceResponse& response() const { return response_; }

 private:
  std::string test_name_;
  const Descriptor* type_;
  conformance::ConformanceRequest request_;
  conformance::ConformanceResponse response_;
};

class InMemoryMessage {
 public:
  InMemoryMessage(const class Test* test, const Descriptor* type,
                  conformance::ConformanceRequest request)
      : test_(test), type_(type), request_(std::move(request)) {}
  ~InMemoryMessage() = default;

  TestResult SerializeBinary() &&;

  struct TextFormatOptions {
    TextFormatOptions() : print_unknown_fields(false) {}
    bool print_unknown_fields;
  };
  TestResult SerializeText(TextFormatOptions options = {}) &&;

  TestResult SerializeJson() &&;

 private:
  const class Test* test_;
  const Descriptor* type_;
  conformance::ConformanceRequest request_;
};

class Test {
 public:
  Test(class Testee* testee, absl::string_view name, TestStrictness strictness)
      : testee_(testee), name_(name), strictness_(strictness) {}
  ~Test() = default;

  InMemoryMessage ParseBinary(const Descriptor* type, Wire input);

  InMemoryMessage ParseText(const Descriptor* type, absl::string_view input);

  struct JsonParseOptions {
    JsonParseOptions() : ignore_unknown_fields(false) {}
    bool ignore_unknown_fields;
  };
  InMemoryMessage ParseJson(const Descriptor* type, absl::string_view input,
                            JsonParseOptions options = {});

 private:
  friend class InMemoryMessage;

  absl::string_view name() const { return name_; }
  TestStrictness strictness() const { return strictness_; }
  Testee& testee() const { return *testee_; }

  class Testee* testee_;
  std::string name_;
  TestStrictness strictness_;
};

class Testee {
 public:
  explicit Testee(absl::string_view testee_binary)
      : runner_(testee_binary, {}, false) {}

  conformance::ConformanceResponse Run(
      absl::string_view test_name,
      const conformance::ConformanceRequest& request);

 private:
  ForkPipeRunner runner_;

  absl::flat_hash_set<std::string> test_names_ran_;
};
}  // namespace internal

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_RUNNER_H__
