// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// A stand-in in-process testee for the analysis tests of
// conformance_test(in_process = True) (see conformance_bzl_test.bzl).  It is
// only ever analyzed, never linked or run.

#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/testee_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

std::unique_ptr<ConformanceTestRunner> MakeTesteeRunner(
    absl::string_view testee_binary,
    absl::Span<const std::string> testee_args) {
  return nullptr;
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
