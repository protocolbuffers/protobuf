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
    // `maximum_edition` is the maximum *stable* edition; EDITION_UNSTABLE is
    // newer than any of them, so it is gated on the editions tier instead: the
    // unstable tests run whenever the 2023 tests do, as in the original suites
    // and the open-source runner.  Every test message the suites use is 2023
    // or older, so nothing is skipped that the unstable tests would leapfrog.
    // TODO: once a test message of a newer stable edition
    // exists, give EDITION_UNSTABLE its own opt-in (an `unstable_edition`
    // option / flag / conformance_test() attribute) so that
    // --maximum_edition=2023 keeps skipping the newer stable message while
    // still running these.
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
  if (const Descriptor* message = MessageUnderTest();
      message != nullptr &&
      !IsSupported(*message,
                   ConformanceEnvironment::Get().options().maximum_edition)) {
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
