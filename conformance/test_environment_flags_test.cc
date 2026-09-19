// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/test_environment_flags.h"

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/reflection.h"
#include "absl/strings/string_view.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::google::protobuf::conformance::internal::ConformanceEnvironmentOptions;
using ::google::protobuf::conformance::internal::OptionsFromFlags;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::IsNull;

// The flag-parsing step of a test_environment_main-style main, on the given
// command line (without argv[0]): parses the absl flags with the same parser
// the main relies on (InitGoogle() / absl::ParseCommandLine()) and returns the
// remaining positional arguments, which the main hands to OptionsFromFlags().
// Exits the process on a malformed command line, like the main would, so only
// well-formed command lines are tested this way.
//
// The returned pointers point into `args` (argv is made of its strings' data,
// hence the non-const reference), so `args` must outlive them: pass a local.
std::vector<char*> ParseCommandLine(std::vector<std::string>& args) {
  std::string argv0 = "conformance_test";
  std::vector<char*> argv = {argv0.data()};
  for (std::string& arg : args) argv.push_back(arg.data());
  std::vector<char*> positional_args;
  std::vector<absl::UnrecognizedFlag> unrecognized_flags;
  absl::ParseAbseilFlagsOnly(static_cast<int>(argv.size()), argv.data(),
                             positional_args, unrecognized_flags);
  positional_args.erase(positional_args.begin());  // argv[0]
  return positional_args;
}

// --- OptionsFromFlags() ------------------------------------------------------

TEST(OptionsFromFlagsTest, Defaults) {
  absl::FlagSaver saver;

  ConformanceEnvironmentOptions options = OptionsFromFlags();

  EXPECT_THAT(options.runner, IsNull());
  EXPECT_THAT(options.owned_runner, IsNull());
  EXPECT_THAT(options.testee_binary, IsEmpty());
  EXPECT_THAT(options.testee_args, IsEmpty());
  EXPECT_THAT(options.failure_list_files, IsEmpty());
  EXPECT_EQ(options.enforcement_level, kLowestPriority);
  EXPECT_EQ(options.maximum_edition, EDITION_PROTO3);
  EXPECT_FALSE(options.fix);
  EXPECT_THAT(options.fix_output_file, IsEmpty());
  EXPECT_TRUE(options.check_unseen_expected_failures);
}

TEST(OptionsFromFlagsTest, ForwardsEveryFlag) {
  absl::FlagSaver saver;
  absl::SetFlag(&FLAGS_testee_binary, "testee");
  absl::SetFlag(&FLAGS_testee_args, {"--a", "--b"});
  absl::SetFlag(&FLAGS_failure_list, {"x.txt", "y.txt"});
  absl::SetFlag(&FLAGS_enforcement_level, kP0);
  absl::SetFlag(&FLAGS_maximum_edition, EDITION_2023);
  absl::SetFlag(&FLAGS_fix, true);
  absl::SetFlag(&FLAGS_fix_output_file, "out.txt");

  ConformanceEnvironmentOptions options = OptionsFromFlags();

  EXPECT_EQ(options.testee_binary, "testee");
  EXPECT_THAT(options.testee_args, ElementsAre("--a", "--b"));
  EXPECT_THAT(options.failure_list_files, ElementsAre("x.txt", "y.txt"));
  EXPECT_EQ(options.enforcement_level, kP0);
  EXPECT_EQ(options.maximum_edition, EDITION_2023);
  EXPECT_TRUE(options.fix);
  EXPECT_EQ(options.fix_output_file, "out.txt");
  // Not flags: the main sets the runner, the environment the rest.
  EXPECT_THAT(options.runner, IsNull());
  EXPECT_THAT(options.owned_runner, IsNull());
  EXPECT_TRUE(options.check_unseen_expected_failures);
}

TEST(OptionsFromFlagsTest, AppendsPositionalArgsToTesteeArgs) {
  absl::FlagSaver saver;
  absl::SetFlag(&FLAGS_testee_args, {"--from_flag"});
  std::vector<std::string> positional = {"--from_command_line", "value"};
  std::vector<char*> positional_args;
  for (std::string& arg : positional) positional_args.push_back(arg.data());

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);

  EXPECT_THAT(options.testee_args,
              ElementsAre("--from_flag", "--from_command_line", "value"));
}

TEST(OptionsFromFlagsTest, PositionalArgsAloneBecomeTesteeArgs) {
  absl::FlagSaver saver;
  std::vector<std::string> positional = {"--only_positional"};
  std::vector<char*> positional_args;
  for (std::string& arg : positional) positional_args.push_back(arg.data());

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);

  EXPECT_THAT(options.testee_args, ElementsAre("--only_positional"));
}

TEST(OptionsFromFlagsTest, FixWithoutOutputFileLeavesTheOutputFileEmpty) {
  // Defaulting --fix_output_file to the single --failure_list (and resolving
  // it under $BUILD_WORKSPACE_DIRECTORY) is the environment's job, not the
  // flags'.
  absl::FlagSaver saver;
  absl::SetFlag(&FLAGS_failure_list, {"failure_lists/cpp_binary.txt"});
  absl::SetFlag(&FLAGS_fix, true);

  ConformanceEnvironmentOptions options = OptionsFromFlags();

  EXPECT_TRUE(options.fix);
  EXPECT_THAT(options.fix_output_file, IsEmpty());
  EXPECT_THAT(options.failure_list_files,
              ElementsAre("failure_lists/cpp_binary.txt"));
}

TEST(OptionsFromFlagsTest, FixOutputFileWithoutFixIsForwardedAsIs) {
  absl::FlagSaver saver;
  absl::SetFlag(&FLAGS_fix_output_file, "out.txt");

  ConformanceEnvironmentOptions options = OptionsFromFlags();

  EXPECT_FALSE(options.fix);
  EXPECT_EQ(options.fix_output_file, "out.txt");
}

// --- The flags as conformance_test() (conformance.bzl) passes them -----------

TEST(CommandLineTest, ParsesTheMacroCommandLine) {
  absl::FlagSaver saver;
  std::vector<std::string> args = {
      "--testee_binary=google/protobuf/conformance/conformance_cpp",
      "--failure_list=google/protobuf/conformance/failure_lists/"
      "cpp_binary.txt",
      "--maximum_edition=EDITION_2023"};
  std::vector<char*> positional_args = ParseCommandLine(args);

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);

  EXPECT_THAT(positional_args, IsEmpty());
  EXPECT_EQ(options.testee_binary,
            "google/protobuf/conformance/conformance_cpp");
  EXPECT_THAT(options.failure_list_files,
              ElementsAre("google/protobuf/conformance/failure_lists/"
                          "cpp_binary.txt"));
  EXPECT_EQ(options.maximum_edition, EDITION_2023);
  EXPECT_EQ(options.enforcement_level, kLowestPriority);
  EXPECT_FALSE(options.fix);
}

TEST(CommandLineTest, ParsesTheFixFlags) {
  absl::FlagSaver saver;
  std::vector<std::string> args = {"--fix", "--fix_output_file=new.txt",
                                   "--enforcement_level=0"};
  std::vector<char*> positional_args = ParseCommandLine(args);

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);

  EXPECT_TRUE(options.fix);
  EXPECT_EQ(options.fix_output_file, "new.txt");
  EXPECT_EQ(options.enforcement_level, kP0);
}

TEST(CommandLineTest, FailureListIsCommaSeparated) {
  absl::FlagSaver saver;
  std::vector<std::string> args = {"--failure_list=a.txt,b.txt"};
  std::vector<char*> positional_args = ParseCommandLine(args);

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);

  EXPECT_THAT(options.failure_list_files, ElementsAre("a.txt", "b.txt"));
}

TEST(CommandLineTest, RepeatedFailureListFlagReplacesThePreviousValue) {
  // absl vector flags don't accumulate: the last occurrence wins, so two
  // lists must be given as --failure_list=a.txt,b.txt (see the flag's help).
  absl::FlagSaver saver;
  std::vector<std::string> args = {"--failure_list=a.txt",
                                   "--failure_list=b.txt"};
  std::vector<char*> positional_args = ParseCommandLine(args);

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);

  EXPECT_THAT(options.failure_list_files, ElementsAre("b.txt"));
}

TEST(CommandLineTest, TesteeArgsAreCommaSeparatedAndPositionalArgsAppended) {
  absl::FlagSaver saver;
  std::vector<std::string> args = {"--testee_binary=testee",
                                   "--testee_args=--x,--y", "--", "--z", "w"};
  std::vector<char*> positional_args = ParseCommandLine(args);

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);

  EXPECT_THAT(positional_args,
              ElementsAre(absl::string_view("--z"), absl::string_view("w")));
  EXPECT_THAT(options.testee_args, ElementsAre("--x", "--y", "--z", "w"));
}

TEST(EnforcementLevelFlagTest, ParsesThePriorityNumber) {
  TestPriority priority = kLowestPriority;
  std::string error;
  EXPECT_TRUE(AbslParseFlag("0", &priority, &error));
  EXPECT_EQ(priority, kP0);
  EXPECT_TRUE(AbslParseFlag("1", &priority, &error));
  EXPECT_EQ(priority, kP1);
  EXPECT_EQ(AbslUnparseFlag(kP1), "1");

  for (absl::string_view bad : {"-1", "2", "P1", ""}) {
    EXPECT_FALSE(AbslParseFlag(bad, &priority, &error)) << bad;
    EXPECT_THAT(error, HasSubstr("from 0 to 1")) << bad;
  }
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
