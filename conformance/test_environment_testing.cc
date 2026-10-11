// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/test_environment_testing.h"

#include "absl/log/absl_check.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

ScopedPartialRunOverride::ScopedPartialRunOverride(bool partial) {
  ConformanceEnvironment& environment = ConformanceEnvironment::Get();
  ABSL_CHECK(!environment.partial_run_override_.has_value())
      << "Another ScopedPartialRunOverride is already active.";
  environment.partial_run_override_ = partial;
}

ScopedPartialRunOverride::~ScopedPartialRunOverride() {
  ConformanceEnvironment::Get().partial_run_override_.reset();
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
