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

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
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
using ::testing::FieldsAre;
using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

// The names of `count` conformance tests:
// Required.Proto3.ProtobufInput.<stem>1.ProtobufOutput and so on.
std::vector<std::string> TestNames(absl::string_view stem, int count) {
  std::vector<std::string> names;
  for (int i = 1; i <= count; ++i) {
    names.push_back(absl::StrCat("Required.Proto3.ProtobufInput.", stem, i,
                                 ".ProtobufOutput"));
  }
  return names;
}

// Drives a listener by hand, with the gtest test and suite it hears about
// being this one.  Whatever it records lands on the running test, including
// what it records when told that the suite ended.
class ResultListenerTest : public testing::Test {
 protected:
  ~ResultListenerTest() override {
    // ResultLedger insists on being finalized before destruction.
    ledger_.Finalize().IgnoreError();
  }

  // Loads `content` as the ledger's failure list.
  void LoadFailureList(absl::string_view content) {
    const std::string path =
        absl::StrCat(TestTempDir(), "/", CurrentTest().name(), ".txt");
    ABSL_CHECK_OK(File::SetContents(path, content, true));
    ABSL_CHECK_OK(ledger_.LoadFailureList(path));
  }

  // What Yields() does when it checks a result: reports its outcome to the
  // ledger, here for each of the `count` tests TestNames(stem, count) names.
  void Pass(absl::string_view stem, int count = 1) {
    for (const std::string& name : TestNames(stem, count)) {
      ledger_.ReportSuccess(name).IgnoreError();
    }
  }
  void Fail(absl::string_view stem, int count = 1,
            TestPriority priority = kP0) {
    for (const std::string& name : TestNames(stem, count)) {
      ledger_.ReportFailure(name, priority, "boom").IgnoreError();
    }
  }
  void Skip(absl::string_view stem, int count = 1) {
    for (const std::string& name : TestNames(stem, count)) {
      ledger_.ReportSkip(name, "reason").IgnoreError();
    }
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

  Pass("Foo");
  EXPECT_THAT(Properties(), IsEmpty());

  listener_.OnTestEnd(CurrentTest());
  EXPECT_THAT(Properties(), AllOf(Contains(Pair("expected_successes", "1")),
                                  Contains(Pair("unexpected_failures", "0"))));
}

TEST_F(ResultListenerTest, ReportsWhatTheTestAndTheSuiteAdded) {
  listener_.OnTestSuiteStart(CurrentSuite());
  Pass("SetUpTestSuite");

  listener_.OnTestStart(CurrentTest());
  Fail("Foo");
  listener_.OnTestEnd(CurrentTest());
  // The test reports its own statistics only.
  EXPECT_THAT(Properties(), AllOf(Contains(Pair("expected_successes", "0")),
                                  Contains(Pair("unexpected_failures", "1"))));

  Pass("TearDownTestSuite");
  listener_.OnTestSuiteEnd(CurrentSuite());
  // The suite reports the statistics of the whole suite, including what
  // SetUpTestSuite() and TearDownTestSuite() checked.
  EXPECT_THAT(Properties(), AllOf(Contains(Pair("expected_successes", "2")),
                                  Contains(Pair("unexpected_failures", "1"))));
}

// Every outcome the ledger counts, each a different number of times, lands in
// the property of its name.
TEST_F(ResultListenerTest, ReportsEveryStatistic) {
  LoadFailureList(
      "Required.Proto3.ProtobufInput.ExpectedFailure1.ProtobufOutput # boom\n"
      "Required.Proto3.ProtobufInput.ExpectedFailure2.ProtobufOutput # boom\n"
      "Required.Proto3.ProtobufInput.ListedSkip1.ProtobufOutput\n"
      "Required.Proto3.ProtobufInput.UnexpectedSuccess1.ProtobufOutput\n"
      "Required.Proto3.ProtobufInput.UnexpectedSuccess2.ProtobufOutput\n"
      "Required.Proto3.ProtobufInput.UnexpectedSuccess3.ProtobufOutput\n");
  ledger_.set_enforcement_level(kP0);
  listener_.OnTestStart(CurrentTest());

  Skip("ListedSkip");
  Skip("Skip", 3);
  Fail("ExpectedFailure", 2);
  Pass("UnexpectedSuccess", 3);
  Fail("UnexpectedFailure", 5);
  Fail("Tolerated", 6, kP1);
  Pass("Pass", 7);

  listener_.OnTestEnd(CurrentTest());
  EXPECT_THAT(
      Properties(),
      UnorderedElementsAre(
          Pair("skipped_tests", "4"), Pair("listed_skips", "1"),
          Pair("expected_failures", "2"), Pair("unexpected_successes", "3"),
          Pair("unexpected_failures", "5"), Pair("tolerated_failures", "6"),
          Pair("expected_successes", "7")));
}

TEST(StatisticsTest, AddsAndSubtractsEveryCounter) {
  const Statistics one{1, 2, 3, 4, 5, 6, 7};
  Statistics sum = one;
  sum += one;
  EXPECT_THAT(sum, FieldsAre(2, 4, 6, 8, 10, 12, 14));
  EXPECT_THAT(sum - one, FieldsAre(1, 2, 3, 4, 5, 6, 7));
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
