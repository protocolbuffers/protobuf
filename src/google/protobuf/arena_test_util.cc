// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arena_test_util.h"

#include "absl/log/absl_check.h"


#define EXPECT_EQ ABSL_CHECK_EQ

namespace google {
namespace protobuf {
namespace internal {

NoHeapChecker::~NoHeapChecker() {
  capture_alloc.Unhook();
  EXPECT_EQ(0, capture_alloc.alloc_count());
  EXPECT_EQ(0, capture_alloc.free_count());
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
