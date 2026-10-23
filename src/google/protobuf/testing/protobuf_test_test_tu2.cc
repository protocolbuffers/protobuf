// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "google/protobuf/testing/protobuf_test.h"
#include "google/protobuf/testing/protobuf_test_test_fixture_b.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

// The second translation unit of protobuf_test_test; see the
// `ProtobufSharedFixtureB` test in protobuf_test_test.cc.
PB_TEST_F(ProtobufSharedFixtureB, SecondTu) {
  int body = PB_TEST_CHOOSE_VALUE((100, 200), "body");
  SharedFixtureRuns().push_back(absl::StrCat(
      "B2:", base_b_, ":", b_, ":", body, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
