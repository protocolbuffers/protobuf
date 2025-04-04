#include <cstdlib>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/v2/global_test_environment.h"
#include "conformance/v2/test_manager.h"
#include "conformance/v2/testee.h"

#include "absl/flags/parse.h"
#include "absl/status/status_matchers.h"

ABSL_FLAG(std::string, expected_failures_list, "",
          "File containing the tests that are expected to fail.");
ABSL_FLAG(std::string, testee_binary, "", "The binary under test.");
ABSL_FLAG(bool, fix, false,
          "If set, we will attempt to update the failure list based on the "
          "results of the test.");

namespace google {
namespace protobuf {
namespace {

using ::absl_testing::IsOk;

class GlobalEnvironment : public ::testing::Environment {
 public:
  GlobalEnvironment() : testee_(absl::GetFlag(FLAGS_testee_binary)) {}
  ~GlobalEnvironment() override {}

  void SetUp() override {
    env_.LoadFailureList(absl::GetFlag(FLAGS_expected_failures_list));
  }

  void TearDown() override;

  static GlobalEnvironment* Get() {
    static auto* global_environment = new GlobalEnvironment();
    return global_environment;
  }

  TestManager& env() { return env_; }

  internal::Testee& testee() { return testee_; }

 private:
  TestManager env_;
  internal::Testee testee_;
};

constexpr absl::string_view kStatistics[] = {
    "skipped_tests",      "expected_failures",    "unexpected_failures",
    "expected_successes", "unexpected_successes",
};

int GetStatistic(absl::string_view name) {
  if (name == "skipped_tests") {
    return GlobalEnvironment::Get()->env().skipped();
  } else if (name == "expected_failures") {
    return GlobalEnvironment::Get()->env().expected_failures();
  } else if (name == "unexpected_failures") {
    return GlobalEnvironment::Get()->env().unexpected_failures();
  } else if (name == "expected_successes") {
    return GlobalEnvironment::Get()->env().expected_successes();
  } else if (name == "unexpected_successes") {
    return GlobalEnvironment::Get()->env().unexpected_successes();
  }
  ABSL_LOG(FATAL) << "Unknown conformance statistic: " << name;
}

void GlobalEnvironment::TearDown() {
  for (absl::string_view statistic : kStatistics) {
    testing::Test::RecordProperty(std::string(statistic),
                                  GetStatistic(statistic));
  }
  if (absl::GetFlag(FLAGS_fix)) {
    const char* dir = std::getenv("BUILD_WORKSPACE_DIRECTORY");
    // TODO: Non-Blazel support for finding the target file.
    ABSL_CHECK(dir != nullptr);
    env_.SaveFailureList(
        absl::StrCat(dir, "/", absl::GetFlag(FLAGS_expected_failures_list)));
  }
  ASSERT_THAT(env_.Finalize(), IsOk());
}
}  // namespace

namespace internal {

TestManager& GetGlobalTestManager() { return GlobalEnvironment::Get()->env(); }

internal::Testee& GetGlobalTestee() {
  return GlobalEnvironment::Get()->testee();
}

}  // namespace internal

auto* ConformanceTest::suite_counts_ =
    new absl::flat_hash_map<const ::testing::TestSuite*,
                            absl::flat_hash_map<std::string, int>>();

void ConformanceTest::SetUp() {
  for (absl::string_view statistic : kStatistics) {
    test_initial_counts_[statistic] = GetStatistic(statistic);
  }
}
void ConformanceTest::TearDown() {
  auto* suite = ::testing::UnitTest::GetInstance()->current_test_suite();
  for (absl::string_view statistic : kStatistics) {
    int delta = GetStatistic(statistic) - test_initial_counts_[statistic];
    testing::Test::RecordProperty(std::string(statistic), delta);
    (*suite_counts_)[suite][statistic] += delta;
  }
}
void ConformanceTest::TearDownTestSuite() {
  auto* suite = ::testing::UnitTest::GetInstance()->current_test_suite();
  for (absl::string_view statistic : kStatistics) {
    testing::Test::RecordProperty(std::string(statistic),
                                  (*suite_counts_)[suite][statistic]);
  }
}

internal::Test RequiredTest() {
  return RequiredTest(
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
}
internal::Test RequiredTest(absl::string_view test_name) {
  return internal::Test(&GlobalEnvironment::Get()->testee(), test_name,
                        internal::TestStrictness::kRequired);
}
internal::Test RecommendedTest() {
  return RecommendedTest(
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
}
internal::Test RecommendedTest(absl::string_view test_name) {
  return internal::Test(&GlobalEnvironment::Get()->testee(), test_name,
                        internal::TestStrictness::kRecommended);
}

}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);

  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(google::protobuf::GlobalEnvironment::Get());

  return RUN_ALL_TESTS();
}
