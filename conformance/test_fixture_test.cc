// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/test_fixture.h"

#include <string>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/matchers.h"
#include "conformance/mock_test_runner.h"
#include "conformance/test_environment.h"
#include "conformance/test_environment_testing.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::google::protobuf::conformance::internal::ConformanceEnvironmentOptions;
using ::protobuf_test_messages::edition_unstable::TestAllTypesEditionUnstable;
using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StartsWith;

std::string SerializedResponse(absl::string_view textproto) {
  ::conformance::ConformanceResponse response;
  ABSL_CHECK(TextFormat::ParseFromString(textproto, &response));
  return response.SerializeAsString();
}

// A testee that successfully round-trips `optional_int32: 99` for every test
// (by default: being a mock, a test can still set expectations on it).
class FakeTestRunner : public NiceMock<MockTestRunner> {
 public:
  FakeTestRunner() {
    ON_CALL(*this, RunTest)
        .WillByDefault(
            Return(SerializedResponse(R"pb(protobuf_payload: "\010c")pb")));
  }
};

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

// --- Testee() ----------------------------------------------------------------

TEST_F(FixtureTest, InfersTestNameFromCurrentTest) {
  EXPECT_CALL(runner_, RunTest("Required.Proto3.ProtobufInput."
                               "InfersTestNameFromCurrentTest.ProtobufOutput",
                               _));
  EXPECT_THAT(
      Testee()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));

  EXPECT_CALL(runner_, RunTest("Recommended.Proto3.ProtobufInput."
                               "InfersTestNameFromCurrentTest.ProtobufOutput",
                               _));
  EXPECT_THAT(
      Testee(kP1)
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

// A ConformanceTest whose SetUp() leaves the global environment alone, so
// that Testee() can be called without one.
class TesteeDeathTest : public ConformanceTest {
 protected:
  void SetUp() override {}
};

TEST_F(TesteeDeathTest, WithoutInstall) { EXPECT_DEATH(Testee(), "Install"); }

TEST_F(TesteeDeathTest, AfterTearDown) {
  FakeTestRunner runner;
  internal::ScopedGlobalConformanceEnvironment env({/*runner=*/&runner});
  env->SetUp();
  env->TearDown();
  EXPECT_DEATH(Testee("Foo"), "already been shut down");
}

// --- Edition gating ----------------------------------------------------------

// SetUp() skips a test whose MessageUnderTest() is from an edition newer than
// --maximum_edition.  EDITION_UNSTABLE counts as supported from EDITION_2023
// on.
struct EditionGatingCase {
  const Descriptor* message;
  Edition maximum_edition;
  bool supported;
};

class EditionGatingTest
    : public FixtureTest,
      public testing::WithParamInterface<EditionGatingCase> {
 protected:
  EditionGatingTest()
      : FixtureTest(WithMaximumEdition(GetParam().maximum_edition)) {}

  void SetUp() override {
    ConformanceTest::SetUp();
    EXPECT_EQ(IsSkipped(), !GetParam().supported);
  }

 private:
  static ConformanceEnvironmentOptions WithMaximumEdition(Edition edition) {
    ConformanceEnvironmentOptions options;
    options.maximum_edition = edition;
    return options;
  }

  const Descriptor* MessageUnderTest() const override {
    return GetParam().message;
  }
};

TEST_P(EditionGatingTest, SetUpSkipsUnsupportedMessages) {
  EXPECT_TRUE(GetParam().supported) << "SetUp() should have skipped this test.";
}

INSTANTIATE_TEST_SUITE_P(
    All, EditionGatingTest,
    testing::Values(
        EditionGatingCase{TestAllTypesProto2::descriptor(), EDITION_PROTO3,
                          /*supported=*/true},
        EditionGatingCase{TestAllTypesProto3::descriptor(), EDITION_PROTO3,
                          /*supported=*/true},
        EditionGatingCase{TestAllTypesEdition2023::descriptor(), EDITION_PROTO3,
                          /*supported=*/false},
        EditionGatingCase{TestAllTypesEdition2023::descriptor(), EDITION_2023,
                          /*supported=*/true},
        EditionGatingCase{TestAllTypesEditionUnstable::descriptor(),
                          EDITION_PROTO3, /*supported=*/false},
        EditionGatingCase{TestAllTypesEditionUnstable::descriptor(),
                          EDITION_2023, /*supported=*/true},
        EditionGatingCase{TestAllTypesEditionUnstable::descriptor(),
                          EDITION_2024, /*supported=*/true}),
    [](const testing::TestParamInfo<EditionGatingCase>& info) {
      return absl::StrCat(
          absl::StripPrefix(info.param.message->name(), "TestAllTypes"), "_",
          Edition_Name(info.param.maximum_edition));
    });

// --- ConformanceTest fixture -------------------------------------------------

TEST_F(FixtureTest, RunsWithoutMessageUnderTest) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

class Edition2023MessageTest : public FixtureTest {
 protected:
  using FixtureTest::FixtureTest;

 private:
  const Descriptor* MessageUnderTest() const override {
    return TestAllTypesEdition2023::descriptor();
  }
};

TEST_F(Edition2023MessageTest, IsSkippedInSetUpUnderDefaultMaximumEdition) {
  FAIL() << "SetUp() should have skipped this test.";
}

// TODO: b/410126673 - Transitional.  Goes away with the in-binary
// performance mode, see ConformanceEnvironmentOptions::performance.
class PerformanceFixtureTest : public FixtureTest {
 protected:
  bool IsPerformanceTest() const override { return true; }
};

TEST_F(PerformanceFixtureTest, IsSkippedWithoutPerformanceMode) {
  FAIL() << "SetUp() should have skipped this test.";
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

// --- Priorities --------------------------------------------------------------

// ConformanceTest's DefaultPriority() is kP0, which is named "Required".
TEST_F(FixtureTest, TesteeDefaultsToTheSuitePriority) {
  internal::TestResult result =
      Testee()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary();
  EXPECT_EQ(result.priority(), kP0);
  EXPECT_THAT(result.name(), StartsWith("Required."));
  EXPECT_THAT(result,
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

TEST_F(FixtureTest, TesteeTakesAnExplicitPriority) {
  internal::TestResult result =
      Testee(kP1)
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary();
  EXPECT_EQ(result.priority(), kP1);
  EXPECT_THAT(result.name(), StartsWith("Recommended."));
  EXPECT_THAT(result,
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

class P1FixtureTest : public FixtureTest {
 private:
  TestPriority DefaultPriority() const override { return kP1; }
};

TEST_F(P1FixtureTest, TesteeUsesTheOverriddenDefault) {
  internal::TestResult result =
      Testee()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary();
  EXPECT_EQ(result.priority(), kP1);
  EXPECT_THAT(result.name(), StartsWith("Recommended."));
  EXPECT_THAT(result,
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

TEST_F(P1FixtureTest, ExplicitPriorityBeatsTheDefault) {
  internal::TestResult result =
      Testee(kP0)
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary();
  EXPECT_EQ(result.priority(), kP0);
  EXPECT_THAT(result.name(), StartsWith("Required."));
  EXPECT_THAT(result,
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
