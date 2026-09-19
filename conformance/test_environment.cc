// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/test_environment.h"

#ifdef _WIN32
#include <direct.h>  // for _getcwd
#else
#include <unistd.h>  // for getcwd
#endif

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/fork_pipe_runner.h"
#include "conformance/global_test_environment.h"
#include "conformance/test_manager.h"
#include "conformance/test_runner.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_legacy.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::google::protobuf::conformance::internal::Statistics;

// Whether `path` is absolute on this platform.  Only paths that start with a
// slash are absolute on POSIX; Windows also has drive letters ("C:\\...",
// "C:/...") and root-relative backslash paths.
// The current working directory, or "" if it can't be determined.
std::string CurrentWorkingDirectory() {
  // Both glibc/macOS (as an extension) and the MSVC CRT malloc() a buffer of
  // the right size when given a null one.
#ifdef _WIN32
  char* cwd = _getcwd(nullptr, 0);
#else
  char* cwd = getcwd(nullptr, 0);
#endif
  if (cwd == nullptr) return "";
  std::string result = cwd;
  std::free(cwd);
  return result;
}

bool IsAbsolutePath(absl::string_view path) {
#ifdef _WIN32
  if (path.size() >= 2 && absl::ascii_isalpha(path[0]) && path[1] == ':') {
    return true;
  }
  return absl::StartsWith(path, "/") || absl::StartsWith(path, "\\");
#else
  return absl::StartsWith(path, "/");
#endif
}

// The process-global environment.  Set by ConformanceEnvironment::Install() or
// internal::ScopedGlobalConformanceEnvironment.
ConformanceEnvironment* global_environment = nullptr;

// Set by internal::ScopedPartialRunOverride.
absl::optional<bool> partial_run_override;

// The DefaultPriority() of the ConformanceTest fixture whose test is running,
// recorded by ConformanceTest::SetUp() and cleared by its TearDown(); unset
// outside a ConformanceTest fixture, where Testee() falls back to kP0.  A
// plain global, like the environment: the harness is single-threaded (see
// test_environment.h).
absl::optional<TestPriority> suite_default_priority;

// Statistics accumulated by ConformanceTest::TearDown() for each test suite,
// keyed by suite name, until TearDownTestSuite() reports them.
absl::flat_hash_map<std::string, Statistics>& SuiteStatistics() {
  static auto* suite_statistics =
      new absl::flat_hash_map<std::string, Statistics>();
  return *suite_statistics;
}

// Returns the runner the environment owns, if any: `owned_runner` itself, or a
// ForkPipeRunner for `testee_binary`.  Null when using the caller's `runner`.
std::unique_ptr<ConformanceTestRunner> TakeOwnedRunner(
    ConformanceEnvironmentOptions& options) {
  int sources = (options.runner != nullptr) +
                (options.owned_runner != nullptr) +
                !options.testee_binary.empty();
  ABSL_CHECK_EQ(sources, 1)
      << "Exactly one of `runner`, `owned_runner` and `testee_binary` must be "
         "set in ConformanceEnvironmentOptions.";
  if (options.owned_runner != nullptr) {
    return std::move(options.owned_runner);
  }
  if (!options.testee_binary.empty()) {
    return std::make_unique<ForkPipeRunner>(options.testee_binary,
                                            options.testee_args);
  }
  return nullptr;
}

std::string CurrentTestName() {
  const testing::TestInfo* test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  ABSL_CHECK(test_info != nullptr)
      << "Testee() can only infer the test name from inside a test body; "
         "pass a name explicitly otherwise.";
  return test_info->name();
}

std::string CurrentTestSuiteName() {
  const testing::TestSuite* test_suite =
      testing::UnitTest::GetInstance()->current_test_suite();
  ABSL_CHECK(test_suite != nullptr)
      << "ConformanceTest's TearDown hooks must run inside a test suite.";
  return test_suite->name();
}

// The environment handed to gtest by ConformanceEnvironment::Install().  gtest
// deletes it at the end of RUN_ALL_TESTS(); it forwards to the real
// environment, which must outlive that (see ConformanceEnvironment).
class ForwardingEnvironment : public testing::Environment {
 public:
  explicit ForwardingEnvironment(ConformanceEnvironment* absl_nonnull target)
      : target_(target) {}

  void SetUp() override { target_->SetUp(); }
  void TearDown() override { target_->TearDown(); }

 private:
  ConformanceEnvironment* const absl_nonnull target_;
};

}  // namespace

namespace internal {

Statistics Statistics::From(const TestManager& manager) {
  Statistics statistics;
  statistics.skipped_tests = manager.skipped();
  statistics.listed_skips = manager.listed_skips();
  statistics.tolerated_recommended_failures =
      manager.tolerated_recommended_failures();
  statistics.expected_failures = manager.expected_failures();
  statistics.unexpected_failures = manager.unexpected_failures();
  statistics.expected_successes = manager.expected_successes();
  statistics.unexpected_successes = manager.unexpected_successes();
  return statistics;
}

Statistics Statistics::operator-(const Statistics& other) const {
  Statistics delta;
  delta.skipped_tests = skipped_tests - other.skipped_tests;
  delta.listed_skips = listed_skips - other.listed_skips;
  delta.tolerated_recommended_failures =
      tolerated_recommended_failures - other.tolerated_recommended_failures;
  delta.expected_failures = expected_failures - other.expected_failures;
  delta.unexpected_failures = unexpected_failures - other.unexpected_failures;
  delta.expected_successes = expected_successes - other.expected_successes;
  delta.unexpected_successes =
      unexpected_successes - other.unexpected_successes;
  return delta;
}

Statistics& Statistics::operator+=(const Statistics& other) {
  skipped_tests += other.skipped_tests;
  listed_skips += other.listed_skips;
  tolerated_recommended_failures += other.tolerated_recommended_failures;
  expected_failures += other.expected_failures;
  unexpected_failures += other.unexpected_failures;
  expected_successes += other.expected_successes;
  unexpected_successes += other.unexpected_successes;
  return *this;
}

void Statistics::RecordProperties() const {
  testing::Test::RecordProperty("skipped_tests", skipped_tests);
  testing::Test::RecordProperty("listed_skips", listed_skips);
  testing::Test::RecordProperty("tolerated_recommended_failures",
                                tolerated_recommended_failures);
  testing::Test::RecordProperty("expected_failures", expected_failures);
  testing::Test::RecordProperty("unexpected_failures", unexpected_failures);
  testing::Test::RecordProperty("expected_successes", expected_successes);
  testing::Test::RecordProperty("unexpected_successes", unexpected_successes);
}

TestManager& GetGlobalTestManager() {
  return ConformanceEnvironment::Get().test_manager();
}

bool IsPartialRun() {
  if (partial_run_override.has_value()) {
    return *partial_run_override;
  }
  const testing::UnitTest& unit_test = *testing::UnitTest::GetInstance();
  // Under --gtest_fail_fast, the tests after the first failure are skipped but
  // still counted as "to run", so an early stop has to be detected separately.
  // This is conservative: a failure in the very last test also counts as
  // partial, although nothing was skipped.
  if (GTEST_FLAG_GET(fail_fast) && unit_test.Failed()) {
    return true;
  }
  // test_to_run_count() already reflects --gtest_filter and sharding.  Disabled
  // tests are excluded from the baseline, so a normal run isn't "partial" just
  // because it skips DISABLED_ tests, while --gtest_also_run_disabled_tests
  // still counts as a full run.
  return unit_test.test_to_run_count() <
         unit_test.total_test_count() - unit_test.disabled_test_count();
}

ScopedPartialRunOverride::ScopedPartialRunOverride(bool partial) {
  ABSL_CHECK(!partial_run_override.has_value())
      << "Another ScopedPartialRunOverride is already active.";
  partial_run_override = partial;
}

ScopedPartialRunOverride::~ScopedPartialRunOverride() {
  partial_run_override.reset();
}

ScopedGlobalConformanceEnvironment::ScopedGlobalConformanceEnvironment(
    ConformanceEnvironmentOptions options)
    : environment_(std::move(options)) {
  ABSL_CHECK(global_environment == nullptr)
      << "A global ConformanceEnvironment is already set.";
  global_environment = &environment_;
}

ScopedGlobalConformanceEnvironment::~ScopedGlobalConformanceEnvironment() {
  ABSL_CHECK_EQ(global_environment, &environment_);
  global_environment = nullptr;
}

}  // namespace internal

ConformanceEnvironment::ConformanceEnvironment(
    ConformanceEnvironmentOptions options)
    : options_(std::move(options)),
      owned_runner_(TakeOwnedRunner(options_)),
      testee_(std::make_unique<internal::Testee>(
          options_.runner != nullptr ? options_.runner : owned_runner_.get())) {
  // Anything below proto3 (including the legacy runner's EDITION_UNKNOWN
  // default) is meaningless, since proto2 and proto3 tests always run.  This is
  // the one place the maximum edition is clamped; see the option's comment.
  options_.maximum_edition = std::max(options_.maximum_edition, EDITION_PROTO3);
  test_manager_.set_enforce_recommended(options_.enforce_recommended);
}

ConformanceEnvironment::~ConformanceEnvironment() {
  // TestManager insists on being finalized before destruction.  An installed
  // environment is never destroyed (see Install()), so this only runs for
  // environments constructed by internal::ScopedGlobalConformanceEnvironment
  // in unit tests, and those don't necessarily call TearDown(); make sure we
  // don't crash on the way out.
  test_manager_.Finalize().IgnoreError();
}

ConformanceEnvironment& ConformanceEnvironment::Install(
    ConformanceEnvironmentOptions options) {
  ABSL_CHECK(global_environment == nullptr)
      << "ConformanceEnvironment::Install() must only be called once.";
  // gtest deletes the environments it owns at the end of RUN_ALL_TESTS(), but
  // callers (e.g. the merged conformance_test_runner) still need the test
  // manager afterwards.  So although this class is a testing::Environment, it
  // isn't registered with gtest itself: gtest only gets a forwarding proxy,
  // and the real environment intentionally lives until the process exits.
  // Only the environment object leaks, though; TearDown() releases the testee.
  // TODO: b/410122039 - register it directly once the legacy suites are gone.
  auto* environment = new ConformanceEnvironment(std::move(options));
  testing::AddGlobalTestEnvironment(new ForwardingEnvironment(environment));
  global_environment = environment;
  return *environment;
}

ConformanceEnvironment& ConformanceEnvironment::Get() {
  ABSL_CHECK(global_environment != nullptr)
      << "No ConformanceEnvironment has been installed.  Conformance test "
         "binaries must call ConformanceEnvironment::Install() before "
         "RUN_ALL_TESTS(); the usual way to do that is to depend on the "
         "test_environment_main library instead of a generic gtest main.";
  return *global_environment;
}

internal::Testee& ConformanceEnvironment::testee() {
  ABSL_CHECK(testee_ != nullptr)
      << "The testee has already been shut down by "
         "ConformanceEnvironment::TearDown(); conformance tests can't run "
         "after the environment is torn down.";
  return *testee_;
}

void ConformanceEnvironment::SetUp() {
  for (const std::string& failure_list : options_.failure_list_files) {
    absl::Status status = test_manager_.LoadFailureList(failure_list);
    ASSERT_TRUE(status.ok())
        << "Failed to load failure list " << failure_list << ": " << status;
  }
  set_up_succeeded_ = true;
}

void ConformanceEnvironment::TearDown() {
  Statistics::From(test_manager_).RecordProperties();
  const bool partial_run = internal::IsPartialRun();

  // The path the failure list was rewritten to, if --fix succeeded.
  std::string fixed_list;
  if (options_.fix && set_up_succeeded_) {
    if (partial_run) {
      ADD_FAILURE()
          << "--fix ignored: only a subset of the tests ran (e.g. because of "
             "--gtest_filter, sharding or --gtest_fail_fast), so rewriting the "
             "failure list would drop the entries of the tests that didn't "
             "run.  Rerun with --fix and without the filter.";
    } else {
      std::string output_file = options_.fix_output_file;
      const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY");
      if (output_file.empty()) {
        if (options_.failure_list_files.empty()) {
          ADD_FAILURE() << "--fix needs --fix_output_file when no "
                           "--failure_list is given.";
        } else if (options_.failure_list_files.size() > 1) {
          ADD_FAILURE() << "--fix needs --fix_output_file when more than one "
                           "--failure_list is given.";
        } else if (IsAbsolutePath(options_.failure_list_files[0])) {
          output_file = options_.failure_list_files[0];
        } else if (workspace != nullptr) {
          output_file =
              absl::StrCat(workspace, "/", options_.failure_list_files[0]);
        } else {
          ADD_FAILURE() << "--fix needs either --fix_output_file or "
                           "$BUILD_WORKSPACE_DIRECTORY (which `bazel run` "
                           "sets) to locate the failure list to rewrite.";
        }
      } else if (!IsAbsolutePath(output_file) && workspace != nullptr) {
        // Like a relative --failure_list, a relative --fix_output_file names a
        // source file: under `bazel run` the working directory is the
        // runfiles tree, so resolve it against the workspace instead.  This is
        // how conformance_test() (conformance.bzl) lets --fix create a failure
        // list that doesn't exist yet.
        output_file = absl::StrCat(workspace, "/", output_file);
      }
      if (!output_file.empty()) {
        const bool relative = !IsAbsolutePath(output_file);
        absl::Status status = test_manager_.SaveFailureList(output_file);
        if (status.ok()) {
          // Report where the file actually landed: a relative path outside
          // `bazel run` is relative to whatever the working directory is.
          std::string absolute_path = output_file;
          if (relative) {
            std::string cwd = CurrentWorkingDirectory();
            if (!cwd.empty()) {
              absolute_path = absl::StrCat(cwd, "/", output_file);
            }
          }
          ABSL_LOG(INFO) << "Wrote updated failure list to " << absolute_path;
          if (relative && workspace == nullptr &&
              (std::getenv("TEST_SRCDIR") != nullptr ||
               std::getenv("TEST_TMPDIR") != nullptr)) {
            ABSL_LOG(WARNING)
                << "The updated failure list was written inside the test "
                   "sandbox, not into the source tree, because "
                   "$BUILD_WORKSPACE_DIRECTORY is unset.  To update the "
                   "checked-in list use `bazel run <target> -- --fix` instead "
                   "of `bazel test`.";
          }
          fixed_list = absolute_path;
        } else {
          ADD_FAILURE() << "Failed to write updated failure list to "
                        << output_file << ": " << status
                        << " (does the directory exist?)";
        }
      }
    }
  }

  absl::Status status = test_manager_.Finalize();
  if (!status.ok() && set_up_succeeded_ &&
      options_.check_unseen_expected_failures) {
    if (partial_run) {
      // Only a subset of the tests ran, so most expected failures were never
      // exercised.  The legacy runner's --test flag behaves the same way.
      ABSL_LOG(INFO) << "Not checking for unseen expected failures because "
                        "only some of the tests ran (e.g. because of "
                        "--gtest_filter or sharding):\n"
                     << status.message();
    } else if (fixed_list.empty()) {
      ADD_FAILURE() << status.message()
                    << "\nRemove them from the failure list, or rerun with "
                       "--fix to do so automatically.";
    } else {
      ADD_FAILURE() << status.message()
                    << "\nThey have been dropped from the updated failure list "
                       "written to "
                    << fixed_list << ".";
    }
  }

  // Release the testee: for an owned runner this destroys it (stopping the
  // process itself is the runner's job, see ~ForkPipeRunner); for a
  // caller-owned runner it drops our reference so the caller may destroy the
  // runner right after RUN_ALL_TESTS() -- the merged runner relies on this.
  // Either way the leaked environment (see Install()) neither keeps a testee
  // alive until the process exits nor holds a dangling pointer to one.
  testee_.reset();
  owned_runner_.reset();
}

bool ConformanceTest::IsSupported(const Descriptor* absl_nonnull descriptor) {
  Edition edition = FileDescriptorLegacy(descriptor->file()).edition();
  Edition maximum_edition =
      ConformanceEnvironment::Get().options().maximum_edition;
  if (edition == EDITION_UNSTABLE) {
    // `maximum_edition` is the maximum *stable* edition.  Like the legacy
    // runner, run the unstable tests alongside any editions tests.
    // TODO: b/563659302 - this runs the (newer-than-anything) unstable tests
    // at --maximum_edition=2023 while a stable 2024 message would be skipped;
    // gate EDITION_UNSTABLE on its own flag once legacy parity no longer
    // matters.
    return maximum_edition >= EDITION_2023;
  }
  return edition <= maximum_edition;
}

void ConformanceTest::SetUp() {
  // First thing, before the skip below: TearDown() runs (and clears it) for a
  // skipped test too.
  suite_default_priority = DefaultPriority();
  ConformanceEnvironment& environment = ConformanceEnvironment::Get();
  initial_statistics_ = Statistics::From(environment.test_manager());
  if (IsPerformanceTest() && !environment.options().performance) {
    GTEST_SKIP() << "Performance tests only run with --performance.";
  }
  if (!IsPerformanceTest() && environment.options().performance) {
    GTEST_SKIP() << "Only performance tests run with --performance.";
  }
  if (const Descriptor* message = MessageUnderTest(); message != nullptr) {
    CONFORMANCE_SKIP_IF_UNSUPPORTED(message);
  }
}

void ConformanceTest::TearDown() {
  suite_default_priority.reset();
  Statistics delta =
      Statistics::From(ConformanceEnvironment::Get().test_manager()) -
      initial_statistics_;
  delta.RecordProperties();
  SuiteStatistics()[CurrentTestSuiteName()] += delta;
}

void ConformanceTest::TearDownTestSuite() {
  auto it = SuiteStatistics().find(CurrentTestSuiteName());
  if (it == SuiteStatistics().end()) {
    // gtest only runs TearDownTestSuite() for suites with at least one test to
    // run, and TearDown() runs even for tests skipped in SetUp(), so this only
    // happens when a fatal failure in a SetUpTestSuite() skipped every test
    // outright (which is already reported), or when a subclass's TearDown()
    // doesn't call ConformanceTest::TearDown().
    return;
  }
  it->second.RecordProperties();
  SuiteStatistics().erase(it);
}

internal::Test Testee() { return Testee(CurrentTestName()); }

internal::Test Testee(absl::string_view name) {
  return Testee(suite_default_priority.value_or(kP0), name);
}

internal::Test Testee(TestPriority priority) {
  return Testee(priority, CurrentTestName());
}

internal::Test Testee(TestPriority priority, absl::string_view name) {
  return ConformanceEnvironment::Get().testee().CreateTest(name, priority);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
