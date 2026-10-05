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

TEST(OptionsFromFlagsTest, Defaults) {
  absl::FlagSaver saver;
  ConformanceEnvironmentOptions options = OptionsFromFlags();
  EXPECT_THAT(options.testee_binary, IsEmpty());
  EXPECT_THAT(options.testee_args, IsEmpty());
  EXPECT_THAT(options.failure_list_files, IsEmpty());
  EXPECT_EQ(options.enforcement_level, kLowestPriority);
  EXPECT_EQ(options.maximum_edition, EDITION_PROTO3);
  EXPECT_FALSE(options.performance);
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
  absl::SetFlag(&FLAGS_performance, true);
  absl::SetFlag(&FLAGS_fix, true);
  absl::SetFlag(&FLAGS_fix_output_file, "out.txt");

  std::vector<std::string> positional = {"--c", "d"};
  std::vector<char*> positional_args;
  for (std::string& arg : positional) positional_args.push_back(arg.data());

  ConformanceEnvironmentOptions options = OptionsFromFlags(positional_args);
  EXPECT_EQ(options.testee_binary, "testee");
  EXPECT_THAT(options.testee_args, ElementsAre("--a", "--b", "--c", "d"));
  EXPECT_THAT(options.failure_list_files, ElementsAre("x.txt", "y.txt"));
  EXPECT_EQ(options.enforcement_level, kP0);
  EXPECT_EQ(options.maximum_edition, EDITION_2023);
  EXPECT_TRUE(options.performance);
  EXPECT_TRUE(options.fix);
  EXPECT_EQ(options.fix_output_file, "out.txt");
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
