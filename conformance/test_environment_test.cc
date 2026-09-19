// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "test_environment.h"

#include <limits.h>  // for PATH_MAX
#include <unistd.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "global_test_environment.h"
#include "matchers.h"
#include "test_manager.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "test_runner.h"
#include "testee.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::edition_unstable::TestAllTypesEditionUnstable;
using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Return;

// The name the testee sees for RequiredTest("Foo") parsing TestAllTypesProto3
// from binary and serializing to binary.
constexpr absl::string_view kFooTestName =
    "Required.Proto3.ProtobufInput.Foo.ProtobufOutput";
constexpr absl::string_view kBarTestName =
    "Required.Proto3.ProtobufInput.Bar.ProtobufOutput";

std::string SerializedResponse(absl::string_view textproto) {
  ::conformance::ConformanceResponse response;
  ABSL_CHECK(TextFormat::ParseFromString(textproto, &response));
  return response.SerializeAsString();
}

// A testee that successfully round-trips `optional_int32: 99` for every test.
class FakeTestRunner : public ConformanceTestRunner {
 public:
  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    return SerializedResponse(R"pb(protobuf_payload: "\010c")pb");
  }
};

// A FakeTestRunner that reports its destruction.
class FlaggingTestRunner : public FakeTestRunner {
 public:
  explicit FlaggingTestRunner(bool* destroyed) : destroyed_(destroyed) {}
  ~FlaggingTestRunner() override { *destroyed_ = true; }

 private:
  bool* destroyed_;
};

class MockTestRunner : public ConformanceTestRunner {
 public:
  MOCK_METHOD(std::string, RunTest,
              (absl::string_view test_name, absl::string_view input),
              (override));
};

// Sets an environment variable for the lifetime of this object.
class ScopedEnvironmentVariable {
 public:
  ScopedEnvironmentVariable(absl::string_view name,
                            absl::optional<std::string> value)
      : name_(name) {
    if (const char* old = std::getenv(name_.c_str()); old != nullptr) {
      old_value_ = old;
    }
    Set(value);
  }
  ~ScopedEnvironmentVariable() { Set(old_value_); }

 private:
  void Set(const absl::optional<std::string>& value) {
    if (value.has_value()) {
      ABSL_CHECK_EQ(setenv(name_.c_str(), value->c_str(), /*overwrite=*/1), 0);
    } else {
      ABSL_CHECK_EQ(unsetenv(name_.c_str()), 0);
    }
  }

  std::string name_;
  absl::optional<std::string> old_value_;
};

// Changes the working directory for the lifetime of this object.
class ScopedWorkingDirectory {
 public:
  explicit ScopedWorkingDirectory(const std::string& directory) {
    char buffer[PATH_MAX];
    ABSL_CHECK(getcwd(buffer, sizeof(buffer)) != nullptr);
    old_directory_ = buffer;
    ABSL_CHECK_EQ(chdir(directory.c_str()), 0) << directory;
  }
  ~ScopedWorkingDirectory() { ABSL_CHECK_EQ(chdir(old_directory_.c_str()), 0); }

 private:
  std::string old_directory_;
};

// Tears down `env` in a full run, expecting it to report unseen expected
// failures with a message containing `expected_message`.
void TearDownExpectingUnseenFailures(ConformanceEnvironment& env,
                                     absl::string_view expected_message) {
  internal::ScopedPartialRunOverride full_run(false);
  EXPECT_NONFATAL_FAILURE(env.TearDown(), std::string(expected_message));
}

class ConformanceEnvironmentTest : public ::testing::Test {
 protected:
  ConformanceEnvironmentTest()
      : tmp_dir_(absl::StrCat(
            TestTempDir(), "/test_environment_test/",
            testing::UnitTest::GetInstance()->current_test_info()->name())) {
    ABSL_CHECK_OK(
        File::RecursivelyCreateDir(tmp_dir_, 0777));
  }

  ~ConformanceEnvironmentTest() override {
    File::DeleteRecursively(tmp_dir_, NULL, NULL);
  }

  // Writes `content` to a file named `name` in this test's temp directory and
  // returns its path.
  std::string WriteFile(absl::string_view name, absl::string_view content) {
    std::string path = TmpPath(name);
    ABSL_CHECK_OK(File::SetContents(path, content, true));
    return path;
  }

  // `path` is a std::string (not a string_view) for the open source
  // File::GetContents() shim.
  std::string ReadFile(const std::string& path) {
    std::string content;
    ABSL_CHECK_OK(File::GetContents(path, &content, true));
    return content;
  }

  std::string TmpPath(absl::string_view name) {
    return absl::StrCat(tmp_dir_, "/", name);
  }

  std::string CreateTmpDir(absl::string_view name) {
    std::string path = TmpPath(name);
    ABSL_CHECK_OK(File::RecursivelyCreateDir(path, 0777));
    return path;
  }

  // Runs a required test named `name` against the global testee.
  internal::TestResult RunRequired(absl::string_view name) {
    return RequiredTest(name)
        .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
        .SerializeBinary();
  }

  internal::TestResult RunRecommended(absl::string_view name) {
    return RecommendedTest(name)
        .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
        .SerializeBinary();
  }

  FakeTestRunner fake_runner_;

 private:
  std::string tmp_dir_;
};

TEST_F(ConformanceEnvironmentTest, GetReturnsScopedEnvironment) {
  internal::ScopedGlobalConformanceEnvironment env({.runner = &fake_runner_});
  EXPECT_EQ(&ConformanceEnvironment::Get(), &env.environment());
  EXPECT_EQ(&internal::GetGlobalTestManager(), &env->test_manager());
}

TEST_F(ConformanceEnvironmentTest, EnforceRecommendedIsPassedToTestManager) {
  {
    // The default matches the legacy runner's.
    internal::ScopedGlobalConformanceEnvironment env({.runner = &fake_runner_});
    EXPECT_FALSE(env->test_manager().enforce_recommended());
  }
  {
    internal::ScopedGlobalConformanceEnvironment env(
        {.runner = &fake_runner_, .enforce_recommended = true});
    EXPECT_TRUE(env->test_manager().enforce_recommended());
  }
}

TEST_F(ConformanceEnvironmentTest, RequiredTestRoutesToRunner) {
  MockTestRunner mock;
  internal::ScopedGlobalConformanceEnvironment env({.runner = &mock});
  env->SetUp();

  EXPECT_CALL(mock, RunTest(kFooTestName, _))
      .WillOnce(Return(SerializedResponse(R"pb(protobuf_payload: "\010c")pb")));
  EXPECT_THAT(RunRequired("Foo"),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));

  EXPECT_EQ(env->test_manager().expected_successes(), 1);
  EXPECT_EQ(env->test_manager().unexpected_failures(), 0);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, InfersTestNameFromCurrentTest) {
  MockTestRunner mock;
  internal::ScopedGlobalConformanceEnvironment env({.runner = &mock});
  env->SetUp();

  EXPECT_CALL(mock, RunTest("Required.Proto3.ProtobufInput."
                            "InfersTestNameFromCurrentTest.ProtobufOutput",
                            _))
      .WillOnce(Return(SerializedResponse(R"pb(protobuf_payload: "\010c")pb")));
  EXPECT_THAT(
      RequiredTest()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));

  EXPECT_CALL(mock, RunTest("Recommended.Proto3.ProtobufInput."
                            "InfersTestNameFromCurrentTest.ProtobufOutput",
                            _))
      .WillOnce(Return(SerializedResponse(R"pb(protobuf_payload: "\010c")pb")));
  EXPECT_THAT(
      RecommendedTest()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, UnexpectedFailureFailsTheTest) {
  internal::ScopedGlobalConformanceEnvironment env({.runner = &fake_runner_});
  env->SetUp();

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Foo"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");

  EXPECT_EQ(env->test_manager().unexpected_failures(), 1);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, ExpectedFailureFromFailureListPasses) {
  std::string failure_list =
      WriteFile("failures.txt",
                absl::StrCat(kFooTestName, " # Should have failed to parse\n"));
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_, .failure_list_files = {failure_list}});
  env->SetUp();

  EXPECT_THAT(RunRequired("Foo"), Yields(IsParseError()));

  EXPECT_EQ(env->test_manager().expected_failures(), 1);
  EXPECT_EQ(env->test_manager().unexpected_failures(), 0);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, LoadsMultipleFailureLists) {
  std::string first = WriteFile("first.txt", absl::StrCat(kFooTestName, "\n"));
  std::string second =
      WriteFile("second.txt", absl::StrCat(kBarTestName, "\n"));
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_, .failure_list_files = {first, second}});
  env->SetUp();

  EXPECT_THAT(RunRequired("Foo"), Yields(IsParseError()));
  EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError()));

  EXPECT_EQ(env->test_manager().expected_failures(), 2);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, OverlappingFailureListsAreFatalInSetUp) {
  std::string first = WriteFile("first.txt", absl::StrCat(kFooTestName, "\n"));
  std::string second =
      WriteFile("second.txt", absl::StrCat(kFooTestName, "\n"));
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_, .failure_list_files = {first, second}});

  testing::TestPartResultArray failures;
  {
    testing::ScopedFakeTestPartResultReporter reporter(
        testing::ScopedFakeTestPartResultReporter::
            INTERCEPT_ONLY_CURRENT_THREAD,
        &failures);
    env->SetUp();
  }
  ASSERT_EQ(failures.size(), 1);
  EXPECT_TRUE(failures.GetTestPartResult(0).fatally_failed());
  EXPECT_THAT(failures.GetTestPartResult(0).message(),
              HasSubstr(absl::StrCat("Failed to load failure list ", second)));
  EXPECT_THAT(failures.GetTestPartResult(0).message(),
              HasSubstr("already exists"));
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest,
       RecommendedFailureIsToleratedWhenNotEnforced) {
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_, .enforce_recommended = false});
  env->SetUp();

  EXPECT_THAT(RunRecommended("Foo"), Yields(IsParseError()));

  EXPECT_EQ(env->test_manager().tolerated_recommended_failures(), 1);
  EXPECT_EQ(env->test_manager().skipped(), 0);
  EXPECT_EQ(env->test_manager().unexpected_failures(), 0);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest,
       ListedRecommendedFailureIsExpectedEvenWhenNotEnforced) {
  std::string failure_list = WriteFile(
      "failures.txt",
      "Recommended.Proto3.ProtobufInput.Foo.ProtobufOutput # Should have "
      "failed to parse\n");
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {failure_list},
       .enforce_recommended = false});
  env->SetUp();

  EXPECT_THAT(RunRecommended("Foo"), Yields(IsParseError()));

  EXPECT_EQ(env->test_manager().expected_failures(), 1);
  EXPECT_EQ(env->test_manager().tolerated_recommended_failures(), 0);
  // ...and so it isn't unseen either.
  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, MissingFailureListIsFatalInSetUp) {
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {TmpPath("does_not_exist.txt")}});

  testing::TestPartResultArray failures;
  {
    testing::ScopedFakeTestPartResultReporter reporter(
        testing::ScopedFakeTestPartResultReporter::
            INTERCEPT_ONLY_CURRENT_THREAD,
        &failures);
    env->SetUp();
  }
  ASSERT_EQ(failures.size(), 1);
  EXPECT_TRUE(failures.GetTestPartResult(0).fatally_failed());
  EXPECT_THAT(failures.GetTestPartResult(0).message(),
              HasSubstr("Failed to load failure list"));

  // TearDown must not pile on more failures after a failed SetUp.
  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, TearDownFailsOnUnseenExpectedFailures) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, "\n"));
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_, .failure_list_files = {failure_list}});
  env->SetUp();

  TearDownExpectingUnseenFailures(
      env.environment(),
      absl::StrCat("expected failures were not seen: ", kFooTestName,
                   "\nRemove them from the failure list, or rerun with --fix"));
}

TEST_F(ConformanceEnvironmentTest, UnseenExpectedFailuresAreFineInPartialRun) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, "\n"));
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_, .failure_list_files = {failure_list}});
  env->SetUp();

  internal::ScopedPartialRunOverride partial_run(true);
  env->TearDown();
  EXPECT_THAT(env->test_manager().UnseenExpectedFailures(),
              ElementsAre(kFooTestName));
}

TEST_F(ConformanceEnvironmentTest, UnseenCheckCanBeDisabled) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, "\n"));
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {failure_list},
       .check_unseen_expected_failures = false});
  env->SetUp();

  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();
  EXPECT_THAT(env->test_manager().UnseenExpectedFailures(),
              ElementsAre(kFooTestName));
}

TEST_F(ConformanceEnvironmentTest, FixWritesToTheOutputFile) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, " # stale\n"));
  std::string fixed = TmpPath("fixed.txt");
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {failure_list},
       .fix = true,
       .fix_output_file = fixed});
  env->SetUp();

  // Foo (listed) is never run; Bar (unlisted) fails.
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  // The unseen entry is still reported, but the message points at the
  // rewritten list instead of suggesting --fix.
  TearDownExpectingUnseenFailures(
      env.environment(),
      absl::StrCat("expected failures were not seen: ", kFooTestName,
                   "\nThey have been dropped from the updated failure list "
                   "written to ",
                   fixed));

  std::string content = ReadFile(fixed);
  EXPECT_THAT(content, HasSubstr(absl::StrCat(
                           kBarTestName,
                           " # Should have failed to parse, but didn't.")));
  EXPECT_THAT(content, Not(HasSubstr(kFooTestName)));
  // The input is left alone when an explicit output file is given.
  EXPECT_THAT(ReadFile(failure_list), HasSubstr("stale"));
}

TEST_F(ConformanceEnvironmentTest, FixDefaultsToTheSingleAbsoluteFailureList) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, " # stale\n"));
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {failure_list},
       .fix = true});
  env->SetUp();

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  TearDownExpectingUnseenFailures(
      env.environment(),
      absl::StrCat("updated failure list written to ", failure_list));

  std::string content = ReadFile(failure_list);
  EXPECT_THAT(content, HasSubstr(kBarTestName));
  EXPECT_THAT(content, Not(HasSubstr(kFooTestName)));
}

TEST_F(ConformanceEnvironmentTest,
       FixResolvesRelativeFailureListUnderBuildWorkspaceDirectory) {
  // The relative path is loaded from the working directory (the runfiles, under
  // bazel) but rewritten under $BUILD_WORKSPACE_DIRECTORY (the source tree).
  std::string runfiles = CreateTmpDir("runfiles");
  std::string workspace = CreateTmpDir("workspace");
  std::string stale = absl::StrCat(kFooTestName, " # stale\n");
  WriteFile("runfiles/failures.txt", stale);
  WriteFile("workspace/failures.txt", stale);
  ScopedWorkingDirectory cwd(runfiles);
  ScopedEnvironmentVariable build_workspace_directory(
      "BUILD_WORKSPACE_DIRECTORY", workspace);

  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {"failures.txt"},
       .fix = true});
  env->SetUp();

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  TearDownExpectingUnseenFailures(
      env.environment(), absl::StrCat("updated failure list written to ",
                                      workspace, "/failures.txt"));

  EXPECT_THAT(ReadFile(TmpPath("workspace/failures.txt")),
              HasSubstr(kBarTestName));
  EXPECT_EQ(ReadFile(TmpPath("runfiles/failures.txt")), stale);
}

TEST_F(ConformanceEnvironmentTest,
       FixWithRelativeFailureListNeedsBuildWorkspaceDirectory) {
  std::string runfiles = CreateTmpDir("runfiles");
  WriteFile("runfiles/failures.txt", "");
  ScopedWorkingDirectory cwd(runfiles);
  ScopedEnvironmentVariable build_workspace_directory(
      "BUILD_WORKSPACE_DIRECTORY", absl::nullopt);

  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {"failures.txt"},
       .fix = true});
  env->SetUp();

  internal::ScopedPartialRunOverride full_run(false);
  EXPECT_NONFATAL_FAILURE(
      env->TearDown(),
      "--fix needs either --fix_output_file or $BUILD_WORKSPACE_DIRECTORY");
  EXPECT_EQ(ReadFile(TmpPath("runfiles/failures.txt")), "");
}

TEST_F(ConformanceEnvironmentTest,
       FixResolvesRelativeOutputFileUnderBuildWorkspaceDirectory) {
  // conformance_test() passes a relative --fix_output_file (and no
  // --failure_list) when the failure list doesn't exist yet; --fix must then
  // create it in the source tree, not in the runfiles.
  std::string runfiles = CreateTmpDir("runfiles");
  std::string workspace = CreateTmpDir("workspace");
  ScopedWorkingDirectory cwd(runfiles);
  ScopedEnvironmentVariable build_workspace_directory(
      "BUILD_WORKSPACE_DIRECTORY", workspace);

  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .fix = true,
       .fix_output_file = "failures.txt"});
  env->SetUp();

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();

  EXPECT_THAT(ReadFile(TmpPath("workspace/failures.txt")),
              HasSubstr(kBarTestName));
  EXPECT_FALSE(
      File::Exists(TmpPath("runfiles/failures.txt")));
}

TEST_F(ConformanceEnvironmentTest,
       FixWithRelativeOutputFileWritesToTheWorkingDirectoryOutsideBlazeRun) {
  std::string runfiles = CreateTmpDir("runfiles");
  ScopedWorkingDirectory cwd(runfiles);
  ScopedEnvironmentVariable build_workspace_directory(
      "BUILD_WORKSPACE_DIRECTORY", absl::nullopt);

  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .fix = true,
       .fix_output_file = "failures.txt"});
  env->SetUp();

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();

  EXPECT_THAT(ReadFile(TmpPath("runfiles/failures.txt")),
              HasSubstr(kBarTestName));
}

TEST_F(ConformanceEnvironmentTest, FixWithMultipleListsNeedsOutputFile) {
  std::string first = WriteFile("first.txt", "");
  std::string second = WriteFile("second.txt", "");
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {first, second},
       .fix = true});
  env->SetUp();

  internal::ScopedPartialRunOverride full_run(false);
  EXPECT_NONFATAL_FAILURE(
      env->TearDown(),
      "--fix needs --fix_output_file when more than one --failure_list");
}

TEST_F(ConformanceEnvironmentTest, FixWithNoListsNeedsOutputFile) {
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_, .fix = true});
  env->SetUp();

  internal::ScopedPartialRunOverride full_run(false);
  EXPECT_NONFATAL_FAILURE(
      env->TearDown(), "--fix needs --fix_output_file when no --failure_list");
}

TEST_F(ConformanceEnvironmentTest, FixIsRefusedInPartialRun) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, " # stale\n"));
  std::string fixed = TmpPath("fixed.txt");
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {failure_list},
       .fix = true,
       .fix_output_file = fixed});
  env->SetUp();

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");

  // Rewriting would drop Foo, whose test simply wasn't selected.  The unseen
  // check itself stays quiet in a partial run, so this is the only failure.
  internal::ScopedPartialRunOverride partial_run(true);
  EXPECT_NONFATAL_FAILURE(env->TearDown(), "--fix ignored");
  EXPECT_FALSE(File::Exists(fixed));
  EXPECT_THAT(ReadFile(failure_list), HasSubstr("stale"));
}

TEST_F(ConformanceEnvironmentTest, FixReportsWriteFailures) {
  std::string failure_list = WriteFile("failures.txt", "");
  std::string not_a_file = CreateTmpDir("directory");
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &fake_runner_,
       .failure_list_files = {failure_list},
       .fix = true,
       .fix_output_file = not_a_file});
  env->SetUp();

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");

  internal::ScopedPartialRunOverride full_run(false);
  EXPECT_NONFATAL_FAILURE(
      env->TearDown(),
      absl::StrCat("Failed to write updated failure list to ", not_a_file));
}

TEST_F(ConformanceEnvironmentTest, FixDropsListedSkips) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, " # listed\n"));
  std::string fixed = TmpPath("fixed.txt");
  MockTestRunner mock;
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &mock,
       .failure_list_files = {failure_list},
       .fix = true,
       .fix_output_file = fixed});
  env->SetUp();

  // Foo (listed) is skipped by the testee, which fails it (see matchers.h);
  // the rewritten list no longer has its entry, so the fixed run passes.
  EXPECT_CALL(mock, RunTest(kFooTestName, _))
      .WillOnce(Return(SerializedResponse(R"pb(skipped: "not supported")pb")));
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Foo"), Yields(IsParseError())),
      "is in the failure list but was skipped by the testee");
  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();

  EXPECT_THAT(ReadFile(fixed), Not(HasSubstr(kFooTestName)));
}

TEST_F(ConformanceEnvironmentTest, StatisticsSnapshotAndDelta) {
  internal::ScopedGlobalConformanceEnvironment env({.runner = &fake_runner_});
  env->SetUp();
  internal::Statistics before = internal::Statistics::From(env->test_manager());

  EXPECT_THAT(RunRequired("Foo"),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Bar"), Yields(IsParseError())),
      "Should have failed to parse");

  internal::Statistics delta =
      internal::Statistics::From(env->test_manager()) - before;
  EXPECT_EQ(delta.expected_successes, 1);
  EXPECT_EQ(delta.unexpected_failures, 1);
  EXPECT_EQ(delta.expected_failures, 0);
  EXPECT_EQ(delta.unexpected_successes, 0);
  EXPECT_EQ(delta.skipped_tests, 0);
  EXPECT_EQ(delta.listed_skips, 0);
  EXPECT_EQ(delta.tolerated_recommended_failures, 0);

  internal::Statistics total;
  total += delta;
  total += delta;
  EXPECT_EQ(total.expected_successes, 2);
  EXPECT_EQ(total.unexpected_failures, 2);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, StatisticsCountListedSkips) {
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, " # listed\n"));
  MockTestRunner mock;
  internal::ScopedGlobalConformanceEnvironment env(
      {.runner = &mock, .failure_list_files = {failure_list}});
  env->SetUp();
  internal::Statistics before = internal::Statistics::From(env->test_manager());

  // A listed test the testee skips fails (see matchers.h) and is both a skip
  // and a listed skip, but neither an expected nor an unexpected anything.
  EXPECT_CALL(mock, RunTest(kFooTestName, _))
      .WillOnce(Return(SerializedResponse(R"pb(skipped: "not supported")pb")));
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunRequired("Foo"), Yields(IsParseError())),
      "is in the failure list but was skipped by the testee");

  internal::Statistics delta =
      internal::Statistics::From(env->test_manager()) - before;
  EXPECT_EQ(delta.skipped_tests, 1);
  EXPECT_EQ(delta.listed_skips, 1);
  EXPECT_EQ(delta.expected_failures, 0);
  EXPECT_EQ(delta.unexpected_failures, 0);
  EXPECT_EQ(delta.expected_successes, 0);
  EXPECT_EQ(delta.unexpected_successes, 0);

  internal::Statistics total;
  total += delta;
  total += delta;
  EXPECT_EQ(total.skipped_tests, 2);
  EXPECT_EQ(total.listed_skips, 2);
  // The skip counts as having seen the entry, so this doesn't report it.
  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, IsSupportedHonorsMaximumEdition) {
  {
    internal::ScopedGlobalConformanceEnvironment env({.runner = &fake_runner_});
    EXPECT_TRUE(ConformanceTest::IsSupported(TestAllTypesProto2::descriptor()));
    EXPECT_TRUE(ConformanceTest::IsSupported(TestAllTypesProto3::descriptor()));
    EXPECT_FALSE(
        ConformanceTest::IsSupported(TestAllTypesEdition2023::descriptor()));
  }
  {
    internal::ScopedGlobalConformanceEnvironment env(
        {.runner = &fake_runner_, .maximum_edition = EDITION_2023});
    EXPECT_TRUE(ConformanceTest::IsSupported(TestAllTypesProto2::descriptor()));
    EXPECT_TRUE(
        ConformanceTest::IsSupported(TestAllTypesEdition2023::descriptor()));
  }
}

TEST_F(ConformanceEnvironmentTest, MaximumEditionIsClampedToProto3) {
  // proto2 and proto3 tests always run, so anything older than proto3
  // (including the legacy runner's EDITION_UNKNOWN default) behaves like
  // proto3 rather than disabling everything.
  for (Edition edition : {EDITION_UNKNOWN, EDITION_LEGACY, EDITION_PROTO2}) {
    internal::ScopedGlobalConformanceEnvironment env(
        {.runner = &fake_runner_, .maximum_edition = edition});
    EXPECT_EQ(env->options().maximum_edition, EDITION_PROTO3);
    EXPECT_TRUE(ConformanceTest::IsSupported(TestAllTypesProto2::descriptor()));
    EXPECT_TRUE(ConformanceTest::IsSupported(TestAllTypesProto3::descriptor()));
    EXPECT_FALSE(
        ConformanceTest::IsSupported(TestAllTypesEdition2023::descriptor()));
  }
}

TEST_F(ConformanceEnvironmentTest, UnstableEditionIsSupportedFrom2023On) {
  {
    internal::ScopedGlobalConformanceEnvironment env({.runner = &fake_runner_});
    EXPECT_FALSE(ConformanceTest::IsSupported(
        TestAllTypesEditionUnstable::descriptor()));
  }
  {
    internal::ScopedGlobalConformanceEnvironment env(
        {.runner = &fake_runner_, .maximum_edition = EDITION_2023});
    EXPECT_TRUE(ConformanceTest::IsSupported(
        TestAllTypesEditionUnstable::descriptor()));
  }
  {
    internal::ScopedGlobalConformanceEnvironment env(
        {.runner = &fake_runner_, .maximum_edition = EDITION_2024});
    EXPECT_TRUE(ConformanceTest::IsSupported(
        TestAllTypesEditionUnstable::descriptor()));
  }
}

TEST_F(ConformanceEnvironmentTest, SpawnsForkPipeRunnerLazily) {
  // The testee binary doesn't exist, but ForkPipeRunner only spawns it on
  // first use, so constructing the environment must succeed.
  internal::ScopedGlobalConformanceEnvironment env(
      {.testee_binary = TmpPath("no_such_testee")});
  env->SetUp();
  env->TearDown();
}

TEST_F(ConformanceEnvironmentTest, TearDownShutsDownTheOwnedRunner) {
  bool destroyed = false;
  internal::ScopedGlobalConformanceEnvironment env(
      {.owned_runner = std::make_unique<FlaggingTestRunner>(&destroyed)});
  env->SetUp();
  EXPECT_THAT(RunRequired("Foo"),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));
  EXPECT_FALSE(destroyed);

  env->TearDown();
  EXPECT_TRUE(destroyed);
  // The statistics survive the testee.
  EXPECT_EQ(env->test_manager().expected_successes(), 1);
}

TEST_F(ConformanceEnvironmentTest, TearDownWithoutSetUpOnlyReleasesTheTestee) {
  // gtest skips an environment's SetUp() and TearDown() when it selects no
  // test to run; the merged runner (conformance_test_main.cc) then calls
  // TearDown() itself so that the environment lets go of its runner.  Nothing
  // else may happen: no --fix rewrite and no unseen-entry failure, as the
  // failure list was never loaded.
  std::string failure_list =
      WriteFile("failures.txt", absl::StrCat(kFooTestName, " # never seen\n"));
  std::string fixed = TmpPath("fixed.txt");
  bool destroyed = false;
  internal::ScopedGlobalConformanceEnvironment env(
      {.owned_runner = std::make_unique<FlaggingTestRunner>(&destroyed),
       .failure_list_files = {failure_list},
       .fix = true,
       .fix_output_file = fixed});
  internal::ScopedPartialRunOverride full_run(false);
  env->TearDown();
  EXPECT_TRUE(destroyed);
  EXPECT_FALSE(File::Exists(fixed));
  EXPECT_THAT(ReadFile(failure_list), HasSubstr("never seen"));
  EXPECT_EQ(env->test_manager().expected_failures(), 0);
}

TEST_F(ConformanceEnvironmentTest, OwnedRunnerDiesWithTheEnvironment) {
  bool destroyed = false;
  {
    internal::ScopedGlobalConformanceEnvironment env(
        {.owned_runner = std::make_unique<FlaggingTestRunner>(&destroyed)});
    EXPECT_FALSE(destroyed);
  }
  EXPECT_TRUE(destroyed);
}

TEST_F(ConformanceEnvironmentTest, PartialRunOverride) {
  {
    internal::ScopedPartialRunOverride partial_run(true);
    EXPECT_TRUE(internal::IsPartialRun());
  }
  {
    internal::ScopedPartialRunOverride full_run(false);
    EXPECT_FALSE(internal::IsPartialRun());
  }
}

TEST(ConformanceEnvironmentDeathTest, GetWithoutInstall) {
  EXPECT_DEATH(ConformanceEnvironment::Get(), "Install");
}

TEST(ConformanceEnvironmentDeathTest, RequiresExactlyOneTestee) {
  FakeTestRunner runner;
  EXPECT_DEATH(internal::ScopedGlobalConformanceEnvironment env({}),
               "Exactly one");
  EXPECT_DEATH(internal::ScopedGlobalConformanceEnvironment env(
                   {.runner = &runner, .testee_binary = "/bin/true"}),
               "Exactly one");
  EXPECT_DEATH(internal::ScopedGlobalConformanceEnvironment env(
                   {.runner = &runner,
                    .owned_runner = std::make_unique<FakeTestRunner>()}),
               "Exactly one");
}

TEST(ConformanceEnvironmentDeathTest, TesteeIsGoneAfterTearDown) {
  FakeTestRunner runner;
  internal::ScopedGlobalConformanceEnvironment env({.runner = &runner});
  env->SetUp();
  env->TearDown();
  EXPECT_DEATH(env->testee(), "already been shut down");
  EXPECT_DEATH(RequiredTest("Foo"), "already been shut down");
}

// --- ConformanceTest fixture -------------------------------------------------

// A ConformanceTest whose global environment is set up by the fixture itself,
// before SetUp() runs, so it can be exercised inside this test binary.
class FixtureTest : public ConformanceTest {
 protected:
  FixtureTest() : FixtureTest(ConformanceEnvironmentOptions()) {}
  explicit FixtureTest(ConformanceEnvironmentOptions options)
      : env_(WithRunner(std::move(options))) {}

  FakeTestRunner runner_;

 private:
  ConformanceEnvironmentOptions WithRunner(
      ConformanceEnvironmentOptions options) {
    options.runner = &runner_;
    return options;
  }

  internal::ScopedGlobalConformanceEnvironment env_;
};

TEST_F(FixtureTest, RunsWithoutMessageUnderTest) {
  EXPECT_THAT(
      RequiredTest()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));
}

class Edition2023MessageTest : public FixtureTest {
 protected:
  using FixtureTest::FixtureTest;

  const Descriptor* MessageUnderTest() const override {
    return TestAllTypesEdition2023::descriptor();
  }
};

TEST_F(Edition2023MessageTest, IsSkippedInSetUpUnderDefaultMaximumEdition) {
  FAIL() << "SetUp() should have skipped this test.";
}

class Edition2023MessageWith2023Test : public Edition2023MessageTest {
 protected:
  Edition2023MessageWith2023Test()
      : Edition2023MessageTest({.maximum_edition = EDITION_2023}) {}
};

TEST_F(Edition2023MessageWith2023Test, RunsWhenMaximumEditionCoversIt) {
  EXPECT_THAT(RequiredTest()
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               VarintField(1, 99))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));
}

// Checks, from an overriding SetUp(), that the documented `if (IsSkipped())
// return;` pattern sees the base class's skip.
class SkipAwareSetUpTest : public Edition2023MessageTest {
 protected:
  void SetUp() override {
    ConformanceTest::SetUp();
    EXPECT_TRUE(IsSkipped());
    if (IsSkipped()) return;
    FAIL() << "Unreachable.";
  }
};

TEST_F(SkipAwareSetUpTest, SubclassSetUpSeesTheSkip) {
  FAIL() << "SetUp() should have skipped this test.";
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
