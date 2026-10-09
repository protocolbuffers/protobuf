// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Declares the process-wide state that the Yields() matcher reads (see
// matchers.h).  The definition is deliberately not provided here.  It is the
// link-time seam between the matchers and the process that hosts them.  The
// conformance test binary provides it from its global test environment,
// populated from command-line flags and the failure list on disk.  Unit tests
// of the matchers provide a lightweight in-memory stand-in.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__

#include "conformance/failure_list.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// Returns the process-wide expected-failure list.  Yields() asks it for the
// verdict on every test, which applies the failure list and the enforcement
// level (see FailureList::set_enforcement_level()).  The test environment,
// which owns the list as part of its ResultLedger, is what tallies the
// outcomes (see ConformanceEnvironment in test_environment.h); the matchers
// never change anything.
const FailureList& GetGlobalFailureList();

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
