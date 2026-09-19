// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file declares the process-wide state that the Yields() matcher (see
// matchers.h) depends on.  The definition is intentionally *not* provided
// here: it is the link-time seam between the matchers and the process that
// hosts them.  The conformance test binary provides it from its global test
// environment (populated from command-line flags and the failure list on
// disk), while unit tests of the matchers provide a lightweight in-memory
// stand-in.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__

#include "test_manager.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// Returns the process-wide TestManager used to record test results against the
// expected-failure list.  Yields() reports the outcome of every test here so
// that the failure list can be validated and regenerated, and reads the
// policy for recommended tests (TestManager::enforce_recommended()) from it.
TestManager& GetGlobalTestManager();

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
