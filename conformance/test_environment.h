// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file defines the process-wide plumbing shared by every gtest-based
// conformance suite:
//
//   - ConformanceEnvironment: the process-global state of a conformance test
//     binary.  It owns the testee connection and the TestManager
//     (expected-failure list), records statistics, and validates/regenerates
//     the failure list at the end of the run.  Exactly one is installed per
//     test binary, normally by test_environment_main.cc from command-line
//     flags (see test_environment_flags.h); Install() hooks its
//     SetUp()/TearDown() into gtest.
//   - ConformanceTest: the fixture every conformance test should use.  It
//     handles performance-test filtering, edition gating, per-test statistics
//     and the suite's default priority (DefaultPriority(); see TestPriority in
//     testee.h).
//   - Testee(): the entry point for building a test against the global
//     testee, at the suite's priority or at the one it is given.
//
// Example:
//
//   using DelimitedFieldTest = ConformanceTest;
//
//   TEST_F(DelimitedFieldTest, ValidNonMessage) {
//     CONFORMANCE_SKIP_IF_UNSUPPORTED(TestAllTypesEdition2023::descriptor());
//     EXPECT_THAT(Testee()
//                     .ParseBinary(TestAllTypesEdition2023::descriptor(),
//                                  VarintField(1, 99))
//                     .SerializeBinary(),
//                 Yields(ParsedPayload(EqualsTextProto("optional_int32:
//                 99"))));
//   }
//
//   // A suite that is only recommended, with one test that isn't.
//   class OneofZeroTest : public ConformanceTest {
//    public:
//     TestPriority DefaultPriority() const override { return kP3; }
//   };
//
//   TEST_F(OneofZeroTest, Baseline) {
//     EXPECT_THAT(Testee(kP0).ParseBinary(...).SerializeBinary(), Yields(...));
//   }
//
// Everything here is single-threaded: it is only meant to be used from gtest's
// main thread (test bodies, fixtures and the environment hooks).  The global
// environment is a plain pointer and TestManager isn't thread-safe, so tests
// must not create or run conformance Tests from other threads.
//
// Limitations: --gtest_repeat is not supported.  Each conformance test result
// is recorded in the TestManager exactly once, so a repeated run would see
// every test as already recorded and the statistics and failure-list checks
// would be wrong.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_H__

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "conformance/test_manager.h"
#include "conformance/test_runner.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {

// Options controlling a ConformanceEnvironment.  Normally populated from
// command-line flags by test_environment_main.cc (see
// OptionsFromFlags() in test_environment_flags.h).
struct ConformanceEnvironmentOptions {
  // Exactly one of `runner`, `owned_runner` or `testee_binary` must be set.
  //
  // A caller-owned runner, which must outlive the environment.  Used for
  // mocks and by the transitional merged runner.
  ConformanceTestRunner* absl_nullable runner = nullptr;
  // A runner the environment takes ownership of and destroys in TearDown(),
  // after the failure-list checks, e.g. an in-process testee that needs
  // shutting down.  (The environment moves it out of the options, so
  // ConformanceEnvironment::options().owned_runner is always null.)
  std::unique_ptr<ConformanceTestRunner> owned_runner;
  // A testee executable to spawn (via ForkPipeRunner) with `testee_args`.
  // Like `owned_runner`, it is shut down in TearDown().
  std::string testee_binary;
  std::vector<std::string> testee_args;

  // Failure list files to load.  All of them are loaded into a single
  // TestManager, so entries must not overlap between files; SetUp() fails
  // fatally if they do.
  std::vector<std::string> failure_list_files;

  // Whether failures in recommended tests count as failures.  When false, they
  // are tolerated (recorded as such) unless the test is listed in the failure
  // list, in which case it counts as an expected failure just like a required
  // test would.  The default matches the legacy conformance_test_runner.
  bool enforce_recommended = false;

  // The maximum *stable* edition to test.  Tests for messages whose edition is
  // newer than this are skipped, except that EDITION_UNSTABLE tests run
  // whenever this is EDITION_2023 or newer (as in the legacy runner).  Values
  // below EDITION_PROTO3 (e.g. EDITION_UNKNOWN, the legacy runner's default)
  // are meaningless, since proto2 and proto3 tests always run; the environment
  // clamps them to EDITION_PROTO3 in its constructor, and that is the one place
  // the clamp lives.  The default only runs proto2 and proto3 tests, like the
  // legacy runner's.
  Edition maximum_edition = EDITION_PROTO3;

  // When true, only performance tests run; when false, only regular tests run.
  // Because the two kinds of tests are mutually exclusive within one run, a
  // performance run needs its own failure list (as with the legacy runner's
  // *_performance.txt lists): with a shared list, every regular test's entry
  // would be "unseen" (and dropped by `fix`).
  // TODO: b/410126673 - the design doc plans performance tests as a separate
  // suite/binary; this in-binary mode exists for parity with the legacy
  // runner's --performance flag during the migration.
  bool performance = false;

  // Rewrite the failure list at the end of the run, dropping entries that no
  // longer fail and adding new failures with their messages.  Refused (with a
  // test failure, and without writing anything) when only a subset of the
  // binary's tests ran (see internal::IsPartialRun()), since the entries of
  // the tests that didn't run would otherwise be dropped as "unseen".
  bool fix = false;
  // Where to write the fixed failure list.  Required when `fix` is set and
  // there isn't exactly one entry in `failure_list_files`; otherwise defaults
  // to that single file under $BUILD_WORKSPACE_DIRECTORY (i.e. the source file
  // when run via `bazel run`).
  std::string fix_output_file;

  // Whether TearDown() fails the run if some expected failures were never
  // seen.  The check is also skipped automatically when only a subset of the
  // binary's tests ran (because of --gtest_filter, test sharding, etc.), since
  // a partial run can't see every expected failure; the legacy runner's --test
  // flag behaves the same way.  The merged runner sets this to false and
  // reconciles unseen entries against the legacy suites itself (see
  // TestManager::UnseenExpectedFailures).
  bool check_unseen_expected_failures = true;
};

namespace internal {

// A snapshot of the TestManager's counters.  Used to report per-test and
// per-suite deltas as test properties.
struct Statistics {
  int skipped_tests = 0;
  // The skipped tests that are in the failure list; see
  // TestManager::ListedSkips().
  int listed_skips = 0;
  int tolerated_recommended_failures = 0;
  int expected_failures = 0;
  int unexpected_failures = 0;
  int expected_successes = 0;
  int unexpected_successes = 0;

  static Statistics From(const TestManager& manager);
  Statistics operator-(const Statistics& other) const;
  Statistics& operator+=(const Statistics& other);

  // Records each statistic as a gtest property of the current test (or suite,
  // or run) under the names skipped_tests, listed_skips,
  // tolerated_recommended_failures, expected_failures, unexpected_failures,
  // expected_successes and unexpected_successes.
  void RecordProperties() const;
};

// Returns whether the current gtest run only executes a subset of the binary's
// (non-disabled) tests, e.g. because of --gtest_filter or test sharding, or
// because --gtest_fail_fast stopped it early after a failure.  Only meaningful
// once RUN_ALL_TESTS() has selected the tests to run.
bool IsPartialRun();

// Makes IsPartialRun() return `partial` for the lifetime of this object.  This
// is for unit tests of the environment itself, so they can exercise both the
// full-run and the partial-run behavior no matter how the test binary itself
// was invoked (--gtest_filter, sharding, ...).  Check-fails if another
// override is active.
class ScopedPartialRunOverride {
 public:
  explicit ScopedPartialRunOverride(bool partial);
  ~ScopedPartialRunOverride();

  ScopedPartialRunOverride(const ScopedPartialRunOverride&) = delete;
  ScopedPartialRunOverride& operator=(const ScopedPartialRunOverride&) = delete;
};

class ScopedGlobalConformanceEnvironment;

}  // namespace internal

// The process-global state of a conformance test binary.  See the file comment
// for an overview.
//
// This is a testing::Environment (so SetUp()/TearDown() are genuine overrides
// that gtest runs around the tests), but it can't be handed to
// testing::AddGlobalTestEnvironment() directly, and its constructor is private
// to make sure of that: gtest deletes the environments registered with it at
// the end of RUN_ALL_TESTS() (see RunAllTests in gtest.cc), whereas the
// installed instance must outlive that, because the transitional merged
// runner (conformance_test_main.cc) reads test_manager() afterwards to report
// the results of the gtest suites alongside the legacy ones.  Install() instead
// registers a private proxy with gtest that forwards to SetUp() and TearDown().
// The environment object itself is intentionally leaked; the testee is not
// (TearDown() releases it).
// TODO: b/410122039 - once the legacy suites are gone, nothing needs the
// environment after RUN_ALL_TESTS(); register it directly with
// AddGlobalTestEnvironment() and delete the proxy.
//
// Single-threaded; see the file comment.
class ConformanceEnvironment : public testing::Environment {
 public:
  ~ConformanceEnvironment() override;

  ConformanceEnvironment(const ConformanceEnvironment&) = delete;
  ConformanceEnvironment& operator=(const ConformanceEnvironment&) = delete;

  // Creates an environment, hooks its SetUp()/TearDown() into gtest and makes
  // it the process-global instance.  Must be called exactly once, before
  // RUN_ALL_TESTS().  The environment itself is never destroyed (see the class
  // comment), but TearDown() releases its testee.
  static ConformanceEnvironment& Install(ConformanceEnvironmentOptions options);

  // Returns the process-global instance.  Check-fails if Install() (or, in
  // unit tests, internal::ScopedGlobalConformanceEnvironment) hasn't been
  // used, which typically means test_environment_main wasn't linked in.
  static ConformanceEnvironment& Get();

  // Loads the failure lists.  Any failure here is fatal, so no tests run.  Run
  // by gtest before the first test (see Install()).
  void SetUp() override;

  // Records the run's statistics as test properties, rewrites the failure list
  // if `fix` was requested (and this isn't a partial run), fails if expected
  // failures were never seen (unless `check_unseen_expected_failures` is false
  // or this is a partial run; see internal::IsPartialRun()), and finally
  // releases the testee: for an owned runner this destroys it (and, with
  // ~ForkPipeRunner's shutdown, stops the process); for a caller-owned runner
  // it drops our reference so the caller may destroy the runner right after
  // RUN_ALL_TESTS() -- the merged runner relies on this.  Run by gtest after
  // the last test (see Install()).
  //
  // Also safe to call if SetUp() never ran (gtest selected no test): it
  // records empty statistics, skips --fix and the unseen check, and releases
  // the testee; conformance_test_main relies on this.
  void TearDown() override;

  internal::TestManager& test_manager() { return test_manager_; }
  // Check-fails once TearDown() has released the testee.
  internal::Testee& testee();
  const ConformanceEnvironmentOptions& options() const { return options_; }

 private:
  friend class internal::ScopedGlobalConformanceEnvironment;

  // Creates an environment.  The testee connection is created eagerly (a
  // ForkPipeRunner only spawns the testee on first use); failure lists are
  // loaded in SetUp().
  explicit ConformanceEnvironment(ConformanceEnvironmentOptions options);

  ConformanceEnvironmentOptions options_;
  // The runner we own (from `owned_runner` or spawned for `testee_binary`);
  // null when using the caller's `runner`.  Released in TearDown().
  std::unique_ptr<ConformanceTestRunner> owned_runner_;
  internal::TestManager test_manager_;
  // Null once TearDown() has released it.
  std::unique_ptr<internal::Testee> testee_;
  bool set_up_succeeded_ = false;
};

// The fixture for all conformance tests.  Tests that aren't performance tests
// should derive from this directly; performance tests should derive from
// PerformanceConformanceTest.
//
// Subclasses that override SetUp(), TearDown() or TearDownTestSuite() must
// call the base implementation: first thing in SetUp() and last thing in
// TearDown() and TearDownTestSuite(), so that the statistics recorded there
// cover the whole test.  The base SetUp() may skip the test, and gtest only
// skips the test *body* on GTEST_SKIP(), so an overriding SetUp() must bail out
// itself afterwards:
//
//   void SetUp() override {
//     ConformanceTest::SetUp();
//     if (IsSkipped()) return;
//     ...
//   }
class ConformanceTest : public testing::Test {
 public:
  // The priority of this suite's tests unless a test says otherwise
  // (Testee(priority)); see TestPriority.  kP0 unless a subclass overrides
  // it.  Read once per test, in SetUp().
  virtual TestPriority DefaultPriority() const { return kP0; }

  // Returns whether tests for `descriptor`'s message type should run under the
  // current --maximum_edition: its file's edition must not be newer than the
  // maximum, except that EDITION_UNSTABLE is supported whenever the maximum is
  // EDITION_2023 or newer.  Prefer CONFORMANCE_SKIP_IF_UNSUPPORTED below (or
  // MessageUnderTest()), which also skips the test.
  //
  // This reads the global environment, so it must not be called during static
  // initialization or from an INSTANTIATE_TEST_SUITE_P parameter generator
  // that runs before ConformanceEnvironment::Install(); it check-fails in that
  // case.  Test bodies and SetUp() are always fine.
  static bool IsSupported(const Descriptor* absl_nonnull descriptor);

 protected:
  // Skips the test if it isn't the kind (performance or regular) selected by
  // the environment or if MessageUnderTest() isn't supported under
  // --maximum_edition, and snapshots statistics for TearDown().
  void SetUp() override;

  // Records this test's statistics as test properties and accumulates them for
  // the suite.
  void TearDown() override;

  // Records the suite's accumulated statistics as suite properties.
  static void TearDownTestSuite();

  // Whether this is a performance test.  Performance tests only run when the
  // environment was configured with `performance = true`, and regular tests
  // only run when it wasn't.
  virtual bool IsPerformanceTest() const { return false; }

  // The message type this test exercises, if the fixture knows it (e.g. a
  // fixture parameterized over the message type).  When non-null, SetUp()
  // skips the test unless IsSupported() holds for it, exactly like
  // CONFORMANCE_SKIP_IF_UNSUPPORTED in the test body, so tests can use the
  // message unconditionally.  The default, null, disables the check.
  virtual const Descriptor* absl_nullable MessageUnderTest() const {
    return nullptr;
  }

 private:
  internal::Statistics initial_statistics_;
};

// The fixture for performance conformance tests.  See
// ConformanceEnvironmentOptions::performance.
class PerformanceConformanceTest : public ConformanceTest {
 protected:
  bool IsPerformanceTest() const override { return true; }
};

// Skips the current test if `descriptor` (a `const Descriptor*`, typically
// `SomeMessage::descriptor()`) isn't supported under --maximum_edition; see
// ConformanceTest::IsSupported().  This must be used in the test body before
// any Test is created so that no request is sent for skipped editions.
#define CONFORMANCE_SKIP_IF_UNSUPPORTED(descriptor)                            \
  do {                                                                         \
    const ::google::protobuf::Descriptor* conformance_skip_descriptor = (descriptor);    \
    if (!::google::protobuf::conformance::ConformanceTest::IsSupported(                  \
            conformance_skip_descriptor)) {                                    \
      GTEST_SKIP() << "Skipping " << conformance_skip_descriptor->full_name()  \
                   << " because its edition is newer than --maximum_edition."; \
    }                                                                          \
  } while (false)

// Creates a test against the global testee.  The name defaults to the current
// gtest test's name, which must be unique across the binary once combined with
// the input/output formats and message edition.
//
// The test's priority (see TestPriority), which <Level> is derived from, is
// the enclosing fixture's DefaultPriority(), recorded by
// ConformanceTest::SetUp(); outside a ConformanceTest fixture (a test body
// whose fixture doesn't derive from it, e.g. in unit tests of the harness) it
// is kP0.  The overloads taking a priority use it instead, for the tests of a
// suite that are more or less important than the rest of it: Testee(kP3).
internal::Test Testee();
internal::Test Testee(absl::string_view name);
internal::Test Testee(TestPriority priority);
internal::Test Testee(TestPriority priority, absl::string_view name);

namespace internal {

// Makes a freshly constructed ConformanceEnvironment the process-global one for
// the lifetime of this object, without registering it with gtest.  This is
// intended for unit tests of the environment itself, which need to drive
// SetUp()/TearDown() directly on several environments in one process; test
// binaries should use ConformanceEnvironment::Install() instead.  Check-fails
// if a global environment is already set.
class ScopedGlobalConformanceEnvironment {
 public:
  explicit ScopedGlobalConformanceEnvironment(
      ConformanceEnvironmentOptions options);
  ~ScopedGlobalConformanceEnvironment();

  ScopedGlobalConformanceEnvironment(
      const ScopedGlobalConformanceEnvironment&) = delete;
  ScopedGlobalConformanceEnvironment& operator=(
      const ScopedGlobalConformanceEnvironment&) = delete;

  ConformanceEnvironment& environment() { return environment_; }
  ConformanceEnvironment* operator->() { return &environment_; }

 private:
  ConformanceEnvironment environment_;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_H__
