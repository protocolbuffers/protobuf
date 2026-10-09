// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Helpers for unit tests of the conformance environment and of the fixtures
// built on it.  Conformance test binaries don't need anything here; they get
// their environment from ConformanceEnvironment::Install().

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_TESTING_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_TESTING_H__

#include <utility>

#include "absl/base/nullability.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// A ConformanceEnvironment that isn't registered with gtest, for tests that
// drive SetUp() and TearDown() themselves.  Constructing it makes it the
// process-global environment (see ConformanceEnvironment's constructor), so
// ConformanceEnvironment::Get() returns it; destroying it clears that.
// Check-fails if a global environment is already set.
class ScopedGlobalConformanceEnvironment {
 public:
  explicit ScopedGlobalConformanceEnvironment(
      ConformanceEnvironmentOptions options)
      : environment_(std::move(options)) {}

  ConformanceEnvironment& environment() { return environment_; }
  ConformanceEnvironment* absl_nonnull operator->() { return &environment_; }

 private:
  ConformanceEnvironment environment_;
};

// Makes the global environment's TearDown() treat the gtest run as partial
// (`partial` true) or complete (false) for the lifetime of this object,
// whatever --gtest_filter, sharding or --gtest_fail_fast say.  So a test can
// exercise both behaviors however the test binary was invoked.  Check-fails if
// there is no global environment or another override is active.
class ScopedPartialRunOverride {
 public:
  explicit ScopedPartialRunOverride(bool partial);
  ~ScopedPartialRunOverride();

  ScopedPartialRunOverride(const ScopedPartialRunOverride&) = delete;
  ScopedPartialRunOverride& operator=(const ScopedPartialRunOverride&) = delete;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_TESTING_H__
