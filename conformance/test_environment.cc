// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
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
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"  // IWYU pragma: keep
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "conformance/failure_list.h"
#include "conformance/fork_pipe_runner.h"
#include "conformance/global_test_environment.h"
#include "conformance/result_ledger.h"
#include "conformance/result_listener.h"
#include "conformance/test_runner.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

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

// Whether `path` is absolute on this platform.  Only paths that start with a
// slash are absolute on POSIX; Windows also has drive letters ("C:\\...",
// "C:/...") and root-relative backslash paths.
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

// The process-global environment.  Set and cleared by ConformanceEnvironment's
// constructor and destructor.
ConformanceEnvironment* global_environment = nullptr;

// Whether the current gtest run only executes a subset of the binary's
// non-disabled tests.  That happens with --gtest_filter, with test sharding,
// or when --gtest_fail_fast stopped the run early after a failure.  Only
// meaningful once RUN_ALL_TESTS() has selected the tests to run.
bool IsPartialRun() {
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

// Hooks ReportUncheckedResults() into the end of every gtest test.  gtest
// still attributes failures to the test at that point (it forgets the current
// test only after the listeners ran).
class ConformanceEnvironment::UncheckedResultReporter
    : public testing::EmptyTestEventListener {
 public:
  explicit UncheckedResultReporter(
      ConformanceEnvironment* absl_nonnull environment)
      : environment_(environment) {}

  void OnTestEnd(const testing::TestInfo& /*test_info*/) override {
    environment_->ReportUncheckedResults();
  }

 private:
  ConformanceEnvironment* const absl_nonnull environment_;
};

const FailureList& GetGlobalFailureList() {
  return ConformanceEnvironment::Get().ledger().failure_list();
}

ConformanceEnvironment::ConformanceEnvironment(
    ConformanceEnvironmentOptions options)
    : options_(std::move(options)),
      owned_runner_(TakeOwnedRunner(options_)),
      testee_(std::make_unique<Testee>(
          options_.runner != nullptr ? options_.runner : owned_runner_.get())),
      result_listener_(new ResultListener(&ledger_)),
      unchecked_result_reporter_(new UncheckedResultReporter(this)) {
  ABSL_CHECK(global_environment == nullptr)
      << "A global ConformanceEnvironment is already set.";
  global_environment = this;
  // Anything below proto3 (including conformance_test_runner's EDITION_UNKNOWN
  // default) is meaningless, since proto2 and proto3 tests always run.  This is
  // the one place the maximum edition is clamped; see the option's comment.
  options_.maximum_edition = std::max(options_.maximum_edition, EDITION_PROTO3);
  ledger_.set_enforcement_level(options_.enforcement_level);
  testing::TestEventListeners& listeners =
      testing::UnitTest::GetInstance()->listeners();
  listeners.Append(result_listener_);
  listeners.Append(unchecked_result_reporter_);
}

ConformanceEnvironment::~ConformanceEnvironment() {
  testing::TestEventListeners& listeners =
      testing::UnitTest::GetInstance()->listeners();
  delete listeners.Release(unchecked_result_reporter_);
  delete listeners.Release(result_listener_);
  // ResultLedger insists on being finalized before destruction.  An installed
  // environment is never destroyed (see Install()), so this only runs for
  // environments constructed by ScopedGlobalConformanceEnvironment in unit
  // tests, and those don't necessarily call TearDown(); make sure we don't
  // crash on the way out.
  ledger_.Finalize().IgnoreError();
  ABSL_CHECK_EQ(global_environment, this);
  global_environment = nullptr;
}

ConformanceEnvironment& ConformanceEnvironment::Install(
    ConformanceEnvironmentOptions options) {
  ABSL_CHECK(global_environment == nullptr)
      << "ConformanceEnvironment::Install() must only be called once.";
  // gtest deletes the environments it owns at the end of RUN_ALL_TESTS(), but
  // the merged conformance_test_runner still needs the ledger
  // afterwards.  So although this class is a testing::Environment, it isn't
  // registered with gtest itself: gtest only gets a forwarding proxy, and the
  // real environment intentionally lives until the process exits.  Only the
  // environment object leaks, though; TearDown() releases the testee.
  // TODO: register it directly once conformance_test_main.cc no
  // longer needs it after RUN_ALL_TESTS() (see the class comment).
  auto* environment = new ConformanceEnvironment(std::move(options));
  testing::AddGlobalTestEnvironment(new ForwardingEnvironment(environment));
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

void ConformanceEnvironment::ReportUncheckedResults() {
  if (testee_ == nullptr) {
    return;  // TearDown() has already run.
  }
  absl::Span<const std::string> tests_run = testee_->tests_run();
  absl::Span<const std::string> new_tests =
      tests_run.subspan(results_reconciled_);
  results_reconciled_ = tests_run.size();
  // Don't pile a second failure onto a test that bailed out before it could
  // check its results.
  if (testing::Test::HasFatalFailure()) {
    return;
  }
  for (const std::string& test_name : new_tests) {
    if (!ledger_.WasReported(test_name)) {
      ADD_FAILURE() << "TestResult for " << test_name
                    << " was never checked; wrap the matcher in Yields()";
    }
  }
}

Testee& ConformanceEnvironment::testee() {
  ABSL_CHECK(testee_ != nullptr)
      << "The testee has already been shut down by "
         "ConformanceEnvironment::TearDown(); conformance tests can't run "
         "after the environment is torn down.";
  return *testee_;
}

void ConformanceEnvironment::SetUp() {
  for (const std::string& failure_list : options_.failure_list_files) {
    absl::Status status = ledger_.LoadFailureList(failure_list);
    ASSERT_TRUE(status.ok())
        << "Failed to load failure list " << failure_list << ": " << status;
  }
  set_up_succeeded_ = true;
}

void ConformanceEnvironment::TearDown() {
  // What's left after the last test: results checked outside any test suite
  // are only reported here, and results run after the last test (e.g. in a
  // TearDownTestSuite()) are only checked here.
  result_listener_->ReportRecordedResults();
  ReportUncheckedResults();
  Statistics::From(ledger_).RecordProperties();
  for (const std::string& entry : ledger_.OverexpandedWildcards()) {
    ADD_FAILURE() << "The failure list entry " << entry << " matched more than "
                  << kMaximumWildcardExpansions
                  << " tests, which hides too much.  List the failing tests "
                     "individually, or narrow the wildcard.";
  }
  const bool partial_run = partial_run_override_.value_or(IsPartialRun());

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
        absl::Status status = ledger_.SaveFailureList(output_file);
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

  absl::Status status = ledger_.Finalize();
  if (!status.ok() && set_up_succeeded_ &&
      options_.check_unseen_expected_failures) {
    if (partial_run) {
      // Only a subset of the tests ran, so most expected failures were never
      // exercised.  The conformance_test_runner --test flag behaves the same
      // way.
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
  // runner right after RUN_ALL_TESTS(); the merged runner relies on this.
  // Either way the leaked environment (see Install()) neither keeps a testee
  // alive until the process exits nor holds a dangling pointer to one.
  testee_.reset();
  owned_runner_.reset();
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
