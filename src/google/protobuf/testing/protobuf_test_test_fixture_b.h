// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_TEST_FIXTURE_B_H__
#define GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_TEST_FIXTURE_B_H__

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "google/protobuf/testing/protobuf_test.h"

namespace google {
namespace protobuf {
namespace internal {

// Fixtures defined in a header and used by both translation units of
// protobuf_test_test; see the `ProtobufSharedFixtureB` tests in
// protobuf_test_test.cc and protobuf_test_test_tu2.cc.

// What the tests on `ProtobufSharedFixtureA` and `ProtobufSharedFixtureB` ran
// with, checked by `VerificationEnvironment` in protobuf_test_test.cc.
inline std::vector<std::string>& SharedFixtureRuns() {
  static auto* const r = new std::vector<std::string>();
  return *r;
}

class ProtobufSharedFixtureBaseB : public ::testing::Test {
 protected:
  PB_TEST_FIXTURE_BASE();

  int base_b_ = PB_TEST_CHOOSE_VALUE((10, 20), "base_b");
};

class ProtobufSharedFixtureB : public ProtobufSharedFixtureBaseB {
 protected:
  int b_ = PB_TEST_CHOOSE_VALUE((3, 4), "b");
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TESTING_PROTOBUF_TEST_TEST_FIXTURE_B_H__
