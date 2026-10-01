// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Declares the process-wide state that the Yields() matcher depends on (see
// matchers.h).  The definition is deliberately not provided here.  It is the
// link-time seam between the matchers and the process that hosts them.  The
// conformance test binary provides it from its global test environment,
// populated from command-line flags and the failure list on disk.  Unit tests
// of the matchers provide a lightweight in-memory stand-in.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__

#include "conformance/test_manager.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// Returns the process-wide TestManager that records test results against the
// expected-failure list.  Yields() reports the outcome of every test here.
// That lets the failure list be validated and regenerated, and the enforcement
// level be applied (see TestManager::set_enforcement_level()).
TestManager& GetGlobalTestManager();

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
