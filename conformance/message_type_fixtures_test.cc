// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/message_type_fixtures.h"

#include <cstddef>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/binary_test_util.h"
#include "conformance/mock_test_runner.h"
#include "conformance/test_environment.h"
#include "conformance/test_environment_testing.h"
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
using ::testing::StrictMock;
using ::testing::TestParamInfo;
using ::testing::ValuesIn;

// The fixtures under test consult the process-global ConformanceEnvironment in
// SetUp(), so this binary installs one for the duration of the run.  It keeps
// the default maximum_edition (EDITION_PROTO3), under which every editions
// message is unsupported and must be skipped.  No conformance test is ever run
// against the testee, so any call on the strict mock fails.
class GlobalEnvironment : public testing::Environment {
 public:
  void SetUp() override {
    environment_.emplace(
        internal::ConformanceEnvironmentOptions{/*runner=*/&runner_});
  }
  void TearDown() override { environment_.reset(); }

 private:
  StrictMock<MockTestRunner> runner_;
  absl::optional<internal::ScopedGlobalConformanceEnvironment> environment_;
};

testing::Environment* const kGlobalEnvironment =
    testing::AddGlobalTestEnvironment(new GlobalEnvironment);

TEST(MessageTypeParamNameTest, IsMessageTypeNameOfTheDescriptor) {
  size_t index = 0;
  for (const Descriptor* message : AllTestMessageTypes()) {
    EXPECT_EQ(
        MessageTypeParamName(TestParamInfo<const Descriptor*>(message, index)),
        MessageTypeName(message, index));
    ++index;
  }
  EXPECT_EQ(MessageTypeParamName(TestParamInfo<const Descriptor*>(
                TestAllTypesProto2Editions::descriptor(), 0)),
            "EditionsProto2");
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
  }
};

TEST_P(MessageTypeSkipTest, BodyOnlyRunsForSupportedMessages) {
  EXPECT_THAT(MessageTypeName(message(), 0), Not(HasSubstr("Editions")));
}

INSTANTIATE_TEST_SUITE_P(All, MessageTypeSkipTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// TestAllTypesEdition2023 is never supported under EDITION_PROTO3, so SetUp()
// must skip every test of an Edition2023ConformanceTest suite.
class Edition2023SkipTest : public Edition2023ConformanceTest {
 protected:
  static void TearDownTestSuite() {
    ExpectSkippedIff([](absl::string_view) { return true; });
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
