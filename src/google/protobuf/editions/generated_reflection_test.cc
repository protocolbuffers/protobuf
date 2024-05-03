// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "google/protobuf/editions/golden/test_messages_proto2.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::protobuf_test_messages::proto2::TestAllTypesProto2;

// It's important that no calls that would initialize the generated pool occur
// in any tests in this file. This test guarantees that there's no mutex
// deadlock from lazily loading cpp_features.proto during reflection.
TEST(Generated, Reflection) {
  TestAllTypesProto2 message;

  message.GetReflection()->SetInt32(
      &message, message.GetDescriptor()->FindFieldByName("optional_int32"), 1);
  EXPECT_EQ(message.optional_int32(), 1);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
