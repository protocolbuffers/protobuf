// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/message_type_fixtures.h"

#include <cstddef>
#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/binary_test_util.h"
#include "conformance/test_environment.h"
#include "conformance/test_runner.h"
#include "google/protobuf/descriptor.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    ::protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::TestParamInfo;
using ::testing::ValuesIn;

// The fixtures under test consult the process-global ConformanceEnvironment in
// SetUp(), so this binary installs one for the duration of the run.  It keeps
// the default maximum_edition (EDITION_PROTO3), under which every editions
// message is unsupported and must be skipped.  No conformance test is ever run
// against the testee, so the runner never gets called.
class NeverCalledTestRunner : public ConformanceTestRunner {
 public:
  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    ADD_FAILURE() << "Unexpected conformance test " << test_name;
    return "";
  }
};

class GlobalEnvironment : public testing::Environment {
 public:
  void SetUp() override {
    environment_.emplace(ConformanceEnvironmentOptions{/*runner=*/&runner_});
  }
  void TearDown() override { environment_.reset(); }

 private:
  NeverCalledTestRunner runner_;
  absl::optional<internal::ScopedGlobalConformanceEnvironment> environment_;
};

testing::Environment* const kGlobalEnvironment =
    testing::AddGlobalTestEnvironment(new GlobalEnvironment);

TEST(MessageTypeParamNameTest, IsParamNameOfTheDescriptor) {
  size_t index = 0;
  for (const Descriptor* message : AllTestMessageTypes()) {
    EXPECT_EQ(MessageTypeParamName(
                  TestParamInfo<const Descriptor*>(message, index++)),
              ParamName(message));
  }
  EXPECT_EQ(MessageTypeParamName(TestParamInfo<const Descriptor*>(
                TestAllTypesProto2Editions::descriptor(), 0)),
            "EditionsProto2");
}

TEST(TupleParamNameTest, JoinsElementNamesWithUnderscore) {
  using Param = std::tuple<const Descriptor*, FieldDescriptor::Type>;
  EXPECT_EQ(
      TupleParamName(TestParamInfo<Param>(
          {TestAllTypesProto3::descriptor(), FieldDescriptor::TYPE_INT32}, 0)),
      "Proto3_INT32");
  EXPECT_EQ(TupleParamName(
                TestParamInfo<Param>({TestAllTypesProto2Editions::descriptor(),
                                      FieldDescriptor::TYPE_MESSAGE},
                                     1)),
            "EditionsProto2_MESSAGE");
}

TEST(TupleParamNameTest, HandlesIntsAndSingleElementTuples) {
  using Param = std::tuple<const Descriptor*, int, int, int>;
  EXPECT_EQ(TupleParamName(TestParamInfo<Param>(
                {TestAllTypesProto2::descriptor(), 6, 0, 3}, 0)),
            "Proto2_6_0_3");
  EXPECT_EQ(TupleParamName(TestParamInfo<std::tuple<const Descriptor*>>(
                {TestAllTypesProto3::descriptor()}, 0)),
            "Proto3");
}

// Expects that every test of the current suite that ran was skipped iff
// `skipped_if(test name)` holds.  For use from TearDownTestSuite().
template <typename Predicate>
void ExpectSkippedIff(Predicate skipped_if) {
  const testing::TestSuite& suite =
      *testing::UnitTest::GetInstance()->current_test_suite();
  for (int i = 0; i < suite.total_test_count(); ++i) {
    const testing::TestInfo& test = *suite.GetTestInfo(i);
    if (!test.should_run()) continue;  // Filtered out or sharded away.
    EXPECT_EQ(test.result()->Skipped(), skipped_if(test.name())) << test.name();
  }
}

// Instantiated over all four message types: SetUp() must skip the two editions
// instances (via MessageUnderTest()) before their bodies run, and let the
// proto2/proto3 ones through.
class MessageTypeSkipTest : public MessageTypeConformanceTest {
 protected:
  static void TearDownTestSuite() {
    ExpectSkippedIff([](absl::string_view name) {
      return absl::StrContains(name, "Editions");
    });
    MessageTypeConformanceTest::TearDownTestSuite();
  }
};

TEST_P(MessageTypeSkipTest, BodyOnlyRunsForSupportedMessages) {
  EXPECT_TRUE(IsSupported(message()));
  EXPECT_THAT(ParamName(message()), Not(HasSubstr("Editions")));
}

INSTANTIATE_TEST_SUITE_P(All, MessageTypeSkipTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// TestAllTypesEdition2023 is never supported under EDITION_PROTO3, so SetUp()
// must skip every test of an Edition2023ConformanceTest suite.
class Edition2023SkipTest : public Edition2023ConformanceTest {
 protected:
  static void TearDownTestSuite() {
    ExpectSkippedIff([](absl::string_view) { return true; });
    Edition2023ConformanceTest::TearDownTestSuite();
  }
};

TEST_F(Edition2023SkipTest, BodyNeverRuns) {
  FAIL() << "SetUp() must skip this test: " << message()->full_name()
         << " isn't supported under --maximum_edition=PROTO3.";
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
