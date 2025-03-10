#ifndef GOOGLE_PROTOBUF_CONFORMANCE_V2_MATCHERS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_V2_MATCHERS_H__

#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "v2/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace internal {

std::unique_ptr<Message> ParseBinary(const TestResult& result);
std::unique_ptr<Message> ParseText(const TestResult& result);

bool ReportSuccess(absl::string_view test_name,
                   testing::MatchResultListener* result_listener);
bool ReportFailure(absl::string_view test_name,
                   absl::string_view failure_message,
                   testing::MatchResultListener* result_listener);
bool ReportSkip(absl::string_view test_name,
                testing::MatchResultListener* result_listener);

void PrintTo(const TestResult& result, std::ostream* os);

template <typename M, typename P>
class PayloadMatcher {
 public:
  using is_gtest_matcher = void;

  explicit PayloadMatcher(absl::string_view name,
                          conformance::ConformanceResponse::ResultCase result,
                          M matcher)
      : name_(name), result_(result), matcher_(std::move(matcher)) {}
  virtual ~PayloadMatcher() = default;

  bool MatchAndExplain(const TestResult& arg,
                       ::testing::MatchResultListener* result_listener) const {
    if (arg.response().result_case() != result_) {
      if (arg.response().has_skipped()) {
        return internal::ReportSkip(arg.name(), result_listener);
      }
      return internal::ReportFailure(
          arg.name(), absl::StrCat(name_, " is missing"), result_listener);
    }

    auto payload = ExtractPayload(arg);
    if (payload == nullptr) {
      return internal::ReportFailure(arg.name(),
                                     absl::StrCat("failed to extract ", name_),
                                     result_listener);
    }

    testing::StringMatchResultListener inner_listener;
    bool result =
        testing::ExplainMatchResult(matcher_, *payload, &inner_listener);
    if (!result) {
      return internal::ReportFailure(arg.name(), inner_listener.str(),
                                     result_listener);
    }
    return internal::ReportSuccess(arg.name(), result_listener);
  }

  void DescribeTo(::std::ostream* gmock_os) const {
    *gmock_os << name_ << " " << testing::DescribeMatcher<P>(matcher_, false);
  }
  void DescribeNegationTo(::std::ostream* gmock_os) const {
    *gmock_os << name_ << " " << testing::DescribeMatcher<P>(matcher_, true);
  }

 protected:
  virtual std::unique_ptr<P> ExtractPayload(const TestResult& arg) const = 0;

 private:
  std::string name_;
  conformance::ConformanceResponse::ResultCase result_;
  M matcher_;
};

template <typename M>
class ParsedBinaryPayloadMatcher : public PayloadMatcher<M, Message> {
 public:
  explicit ParsedBinaryPayloadMatcher(M matcher)
      : PayloadMatcher<M, Message>(
            "parsed binary payload",
            conformance::ConformanceResponse::kProtobufPayload,
            std::move(matcher)) {}
  virtual ~ParsedBinaryPayloadMatcher() = default;

 private:
  std::unique_ptr<Message> ExtractPayload(
      const TestResult& arg) const override {
    return internal::ParseBinary(arg);
  }
};

template <typename M>
class ParsedTextPayloadMatcher : public PayloadMatcher<M, Message> {
 public:
  explicit ParsedTextPayloadMatcher(M matcher)
      : PayloadMatcher<M, Message>(
            "parsed text payload",
            conformance::ConformanceResponse::kProtobufPayload,
            std::move(matcher)) {}
  virtual ~ParsedTextPayloadMatcher() = default;

 private:
  std::unique_ptr<Message> ExtractPayload(
      const TestResult& arg) const override {
    return internal::ParseText(arg);
  }
};

template <typename M>
class BinaryPayloadMatcher : public PayloadMatcher<M, absl::string_view> {
 public:
  explicit BinaryPayloadMatcher(M matcher)
      : PayloadMatcher<M, absl::string_view>(
            "binary payload",
            conformance::ConformanceResponse::kProtobufPayload,
            std::move(matcher)) {}
  virtual ~BinaryPayloadMatcher() = default;

 private:
  std::unique_ptr<absl::string_view> ExtractPayload(
      const TestResult& arg) const override {
    return std::make_unique<absl::string_view>(
        arg.response().protobuf_payload());
  }
};

class FailureMatcher {
 public:
  using is_gtest_matcher = void;

  explicit FailureMatcher(absl::string_view name,
                          conformance::ConformanceResponse::ResultCase result)
      : name_(name), result_(result) {}
  virtual ~FailureMatcher() = default;

  bool MatchAndExplain(const TestResult& arg,
                       ::testing::MatchResultListener* result_listener) const {
    if (arg.response().result_case() != result_) {
      if (arg.response().has_skipped()) {
        return internal::ReportSkip(arg.name(), result_listener);
      }
      return internal::ReportFailure(
          arg.name(), absl::StrCat("is not a ", name_), result_listener);
    }

    return internal::ReportSuccess(arg.name(), result_listener);
  }

  void DescribeTo(std::ostream* os) const { *os << name_; }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not a " << name_;
  }

 private:
  std::string name_;
  conformance::ConformanceResponse::ResultCase result_;
};

}  // namespace internal

template <typename M>
internal::ParsedBinaryPayloadMatcher<M> ParsedBinaryPayload(M matcher) {
  return internal::ParsedBinaryPayloadMatcher<M>(std::move(matcher));
}

template <typename M>
internal::ParsedTextPayloadMatcher<M> ParsedTextPayload(M matcher) {
  return internal::ParsedTextPayloadMatcher<M>(std::move(matcher));
}

template <typename M>
internal::BinaryPayloadMatcher<M> BinaryPayload(M matcher) {
  return internal::BinaryPayloadMatcher<M>(std::move(matcher));
}

inline internal::FailureMatcher IsParseError() {
  return internal::FailureMatcher(
      "parse error", conformance::ConformanceResponse::kParseError);
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_V2_MATCHERS_H__
