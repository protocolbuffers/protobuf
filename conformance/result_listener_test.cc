// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/result_listener.h"

#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/result_ledger.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::testing::AllOf;
using ::testing::Contains;
using ::testing::IsEmpty;
using ::testing::Pair;

constexpr absl::string_view kFooTestName =
    "Required.Proto3.ProtobufInput.Foo.ProtobufOutput";
constexpr absl::string_view kSetUpTestName =
    "Required.Proto3.ProtobufInput.SetUpTestSuite.ProtobufOutput";
constexpr absl::string_view kTearDownTestName =
    "Required.Proto3.ProtobufInput.TearDownTestSuite.ProtobufOutput";

// Drives a listener by hand, with the gtest test and suite it hears about
// being this one.  Whatever it records lands on the running test, including
// what it records when told that the suite ended.
class ResultListenerTest : public testing::Test {
 protected:
  ~ResultListenerTest() override {
    // ResultLedger insists on being finalized before destruction.
    ledger_.Finalize().IgnoreError();
  }

  // What Yields() does when it checks a result: reports its outcome to the
  // ledger.
  void Pass(absl::string_view test_name) {
    ledger_.ReportSuccess(test_name).IgnoreError();
  }
  void Fail(absl::string_view test_name) {
    ledger_.ReportFailure(test_name, kP0, "boom").IgnoreError();
  }

  static const testing::TestSuite& CurrentSuite() {
    return *testing::UnitTest::GetInstance()->current_test_suite();
  }
  static const testing::TestInfo& CurrentTest() {
    return *testing::UnitTest::GetInstance()->current_test_info();
  }

  // The properties of the running test so far, as (key, value) pairs.
  static std::vector<std::pair<std::string, std::string>> Properties() {
    const testing::TestResult& result = *CurrentTest().result();
    std::vector<std::pair<std::string, std::string>> properties;
    for (int i = 0; i < result.test_property_count(); ++i) {
      const testing::TestProperty& property = result.GetTestProperty(i);
      properties.emplace_back(property.key(), property.value());
    }
    return properties;
  }

  ResultLedger ledger_;
  ResultListener listener_{&ledger_};
};

TEST_F(ResultListenerTest, ReportsWhenTheTestEnds) {
  listener_.OnTestSuiteStart(CurrentSuite());
  listener_.OnTestStart(CurrentTest());

  Pass(kFooTestName);
  EXPECT_THAT(Properties(), IsEmpty());

  listener_.OnTestEnd(CurrentTest());
  EXPECT_THAT(Properties(), AllOf(Contains(Pair("expected_successes", "1")),
                                  Contains(Pair("unexpected_failures", "0"))));
}

TEST_F(ResultListenerTest, ReportsWhatTheTestAndTheSuiteAdded) {
  listener_.OnTestSuiteStart(CurrentSuite());
  Pass(kSetUpTestName);

  listener_.OnTestStart(CurrentTest());
  Fail(kFooTestName);
  listener_.OnTestEnd(CurrentTest());
  // The test reports its own statistics only.
  EXPECT_THAT(Properties(), AllOf(Contains(Pair("expected_successes", "0")),
                                  Contains(Pair("unexpected_failures", "1"))));

  Pass(kTearDownTestName);
  listener_.OnTestSuiteEnd(CurrentSuite());
  // The suite reports the statistics of the whole suite, including what
  // SetUpTestSuite() and TearDownTestSuite() checked.
  EXPECT_THAT(Properties(), AllOf(Contains(Pair("expected_successes", "2")),
                                  Contains(Pair("unexpected_failures", "1"))));
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
