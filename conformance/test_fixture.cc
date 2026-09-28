// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/test_fixture.h"

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_legacy.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::google::protobuf::conformance::internal::ConformanceEnvironment;

// Whether tests for `descriptor`'s message type run under `maximum_edition`:
// the file's edition must not be newer than it.
bool IsSupported(const Descriptor& descriptor, Edition maximum_edition) {
  Edition edition = FileDescriptorLegacy(descriptor.file()).edition();
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

std::string CurrentTestName() {
  const testing::TestInfo* test_info =
      testing::UnitTest::GetInstance()->current_test_info();
  ABSL_CHECK(test_info != nullptr)
      << "Testee() can only infer the test name from inside a test body; "
         "pass a name explicitly otherwise.";
  return test_info->name();
}

}  // namespace

void ConformanceTest::SetUp() {
  ConformanceEnvironment& environment = ConformanceEnvironment::Get();
  // TODO: b/410126673 - Transitional.  Goes away with the in-binary
  // performance mode, see ConformanceEnvironmentOptions::performance.
  if (IsPerformanceTest() && !environment.options().performance) {
    GTEST_SKIP() << "Performance tests only run with --performance.";
  }
  if (!IsPerformanceTest() && environment.options().performance) {
    GTEST_SKIP() << "Only performance tests run with --performance.";
  }
  if (const Descriptor* message = MessageUnderTest();
      message != nullptr &&
      !IsSupported(*message, environment.options().maximum_edition)) {
    GTEST_SKIP() << "Skipping " << message->full_name()
                 << " because its edition is newer than --maximum_edition.";
  }
}

internal::Test ConformanceTest::Testee() { return Testee(CurrentTestName()); }

internal::Test ConformanceTest::Testee(absl::string_view name) {
  return Testee(DefaultPriority(), name);
}

internal::Test ConformanceTest::Testee(TestPriority priority) {
  return Testee(priority, CurrentTestName());
}

internal::Test ConformanceTest::Testee(TestPriority priority,
                                       absl::string_view name) {
  return ConformanceEnvironment::Get().testee().CreateTest(name, priority);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
