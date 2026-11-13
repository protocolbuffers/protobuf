// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/expected.h"

#include "absl/log/absl_check.h"

namespace hpb::internal {

[[noreturn]] void ExpectedAbortNoValue() {
  ABSL_CHECK(false)
      << "Attempting to access value of an Expected in error state.";
}

[[noreturn]] void ExpectedAbortHasValue() {
  ABSL_CHECK(false)
      << "Attempting to access error of an Expected in value state.";
}

}  // namespace hpb::internal
