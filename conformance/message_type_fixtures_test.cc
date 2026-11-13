// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/message_type_fixtures.h"

#include <cstddef>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/binary_test_util.h"
#include "conformance/mock_test_runner.h"
#include "conformance/test_environment.h"
#include "conformance/test_environment_testing.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "google/protobuf/descriptor.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
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
// SetUp().  Each suite here installs its own, at the maximum edition it
// exercises, before the fixture's SetUp() runs.  No conformance test is ever
// run against the testee, so any call on the strict mock fails.
template <typename Fixture, Edition kMaximumEdition>
class WithEnvironment : public Fixture {
 protected:
  void SetUp() override {
    internal::ConformanceEnvironmentOptions options;
    options.runner = &runner_;
    options.maximum_edition = kMaximumEdition;
    environment_.emplace(std::move(options));
    Fixture::SetUp();
  }

 private:
  StrictMock<MockTestRunner> runner_;
  absl::optional<internal::ScopedGlobalConformanceEnvironment> environment_;
};

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

// Instantiated over all four message types under EDITION_PROTO3: SetUp() must
// skip the two editions instances (via MessageUnderTest()) before their bodies
// run, and let the proto2/proto3 ones through.
class MessageTypeSkipTest
    : public WithEnvironment<MessageTypeConformanceTest, EDITION_PROTO3> {
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

// Under EDITION_2023 every message type is supported, so none is skipped.
class MessageTypeRunTest
    : public WithEnvironment<MessageTypeConformanceTest, EDITION_2023> {
 protected:
  static void TearDownTestSuite() {
    ExpectSkippedIff([](absl::string_view) { return false; });
  }
};

TEST_P(MessageTypeRunTest, BodyRunsForEveryMessage) {
  EXPECT_EQ(message(), GetParam());
}

INSTANTIATE_TEST_SUITE_P(All, MessageTypeRunTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// TestAllTypesEdition2023 is never supported under EDITION_PROTO3, so SetUp()
// must skip every test of an Edition2023ConformanceTest suite ...
class Edition2023SkipTest
    : public WithEnvironment<Edition2023ConformanceTest, EDITION_PROTO3> {
 protected:
  static void TearDownTestSuite() {
    ExpectSkippedIff([](absl::string_view) { return true; });
  }
};

TEST_F(Edition2023SkipTest, BodyNeverRuns) {
  FAIL() << "SetUp() must skip this test: " << message()->full_name()
         << " isn't supported under --maximum_edition=PROTO3.";
}

// ... and run every one of them under EDITION_2023.
class Edition2023RunTest
    : public WithEnvironment<Edition2023ConformanceTest, EDITION_2023> {
 protected:
  static void TearDownTestSuite() {
    ExpectSkippedIff([](absl::string_view) { return false; });
  }
};

TEST_F(Edition2023RunTest, BodyRunsWithTheEdition2023Message) {
  EXPECT_EQ(message(), TestAllTypesEdition2023::descriptor());
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
