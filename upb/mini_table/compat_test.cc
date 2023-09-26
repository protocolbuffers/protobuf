// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/* Test of mini table accessors.
 *
 * Messages are created and mutated using generated code, and then
 * accessed through reflective APIs exposed through mini table accessors.
 */

#include "upb/mini_table/compat.h"

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto3.upb.h"

namespace {

TEST(GeneratedCode, EqualsTestProto2) {
  EXPECT_TRUE(upb_MiniTable_Equals(
      &protobuf_test_messages_proto2_ProtoWithKeywords_msg_init,
      &protobuf_test_messages_proto2_ProtoWithKeywords_msg_init));
}

TEST(GeneratedCode, EqualsTestProto3) {
  EXPECT_TRUE(upb_MiniTable_Equals(
      &protobuf_test_messages_proto3_TestAllTypesProto3_msg_init,
      &protobuf_test_messages_proto3_TestAllTypesProto3_msg_init));
}

}  // namespace
