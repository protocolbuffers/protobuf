// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/matchers.h"

#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/field_comparator.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::WireFormat;

// Implements EqualsTextProto() and EqualsBinaryProto(): matches a message
// equivalent to the one obtained by decoding `expected` (in `format`) as the
// actual message's type.
class EquivalentMessageMatcher {
 public:
  using is_gtest_matcher = void;

  EquivalentMessageMatcher(WireFormat format, std::string expected)
      : format_(format), expected_(std::move(expected)) {
    ABSL_CHECK(format == ::conformance::PROTOBUF ||
               format == ::conformance::TEXT_FORMAT)
        << "Unsupported expected message format " << WireFormat_Name(format);
  }

  bool MatchAndExplain(const Message& actual,
                       testing::MatchResultListener* listener) const {
    std::unique_ptr<Message> expected = absl::WrapUnique(actual.New());
    if (format_ == ::conformance::PROTOBUF) {
      ABSL_CHECK(expected->ParseFromString(expected_))
          << "Failed to parse expected wire data for "
          << actual.GetDescriptor()->full_name() << ": "
          << absl::CEscape(expected_);
    } else {
      ABSL_CHECK(TextFormat::ParseFromString(expected_, expected.get()))
          << "Failed to parse expected text proto for "
          << actual.GetDescriptor()->full_name() << ": " << expected_;
    }

    util::MessageDifferencer differencer;
    util::DefaultFieldComparator field_comparator;
    field_comparator.set_treat_nan_as_equal(true);
    differencer.set_field_comparator(&field_comparator);
    std::string differences;
    differencer.ReportDifferencesToString(&differences);
    if (differencer.Compare(*expected, actual)) {
      return true;
    }
    *listener << "Output was not equivalent to reference message: "
              << differences;
    return false;
  }

  void DescribeTo(std::ostream* os) const {
    *os << "equals "
        << (format_ == ::conformance::PROTOBUF ? "binary proto" : "text proto")
        << " \"" << absl::CEscape(expected_) << "\"";
  }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "doesn't equal "
        << (format_ == ::conformance::PROTOBUF ? "binary proto" : "text proto")
        << " \"" << absl::CEscape(expected_) << "\"";
  }

 private:
  WireFormat format_;
  std::string expected_;
};

}  // namespace

testing::Matcher<const Message&> EqualsTextProto(absl::string_view text) {
  return EquivalentMessageMatcher(::conformance::TEXT_FORMAT,
                                  std::string(text));
}

testing::Matcher<const Message&> EqualsBinaryProto(Wire bytes) {
  return EquivalentMessageMatcher(::conformance::PROTOBUF,
                                  std::move(bytes).str());
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
