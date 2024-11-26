// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/byte_size.h"

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/message.h"

namespace {
static const upb_MiniTable* kTestMiniTable =
    &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;

TEST(ByteSizeTest, UnpopulatedMsg) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  auto res = upb_ByteSize(UPB_UPCAST(msg), kTestMiniTable);
  EXPECT_EQ(res, 0);
  upb_Arena_Free(arena);
}

TEST(ByteSizeTest, PopulatedMsg) {
  upb_Arena* arena = upb_Arena_New();
  protobuf_test_messages_proto2_TestAllTypesProto2* msg =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_set_optional_int32(msg, 322);
  auto res = upb_ByteSize(UPB_UPCAST(msg), kTestMiniTable);
  EXPECT_EQ(res, 3);
  upb_Arena_Free(arena);
}

}  // namespace
