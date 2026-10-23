// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_TEST_FIXTURE_A_H__
#define GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_TEST_FIXTURE_A_H__

#include <gtest/gtest.h>
#include "google/protobuf/testing/protobuf_test.h"

namespace google {
namespace protobuf {
namespace internal {

// A `PB_TEST_F` fixture defined in a header; see the `ProtobufSharedFixtureA`
// test in protobuf_test_test.cc.
class ProtobufSharedFixtureA : public ::testing::Test {
 protected:
  int a_ = PB_TEST_CHOOSE_VALUE((1, 2), "a");
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_TEST_FIXTURE_A_H__
