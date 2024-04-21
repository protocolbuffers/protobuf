// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/test/proto3_test.upb.h"

TEST(MiniTableOneofTest, OneOfIteratorProto2) {
  constexpr int oneof_first_field_number = 111;
  constexpr int oneof_test_field_number = 116;

  const upb_MiniTable* google_protobuf_table =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(google_protobuf_table, oneof_test_field_number);
  ASSERT_TRUE(field != nullptr);
  const upb_MiniTableField* ptr = upb_MiniTable_GetOneof(google_protobuf_table, field);
  int field_num = oneof_first_field_number;
  do {
    EXPECT_EQ(upb_MiniTableField_Number(ptr), field_num++);
  } while (upb_MiniTable_NextOneofField(google_protobuf_table, &ptr));
}

TEST(MiniTableOneofTest, InitialFieldOneOf) {
  const upb_MiniTable* table = &upb__test__TestOneOfInitialField_msg_init;
  const upb_MiniTableField* field = upb_MiniTable_FindFieldByNumber(table, 1);
  ASSERT_TRUE(field != nullptr);

  const upb_MiniTableField* ptr = upb_MiniTable_GetOneof(table, field);
  EXPECT_TRUE(ptr == field);
}

TEST(MiniTableOneofTest, InitialFieldNotOneOf) {
  constexpr int test_field_number = 1;  // optional int that is not a oneof
  const upb_MiniTable* google_protobuf_table =
      &protobuf_0test_0messages__proto2__TestAllTypesProto2_msg_init;
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(google_protobuf_table, test_field_number);
  ASSERT_TRUE(field != nullptr);
  const upb_MiniTableField* first_field =
      upb_MiniTable_GetOneof(google_protobuf_table, field);
  EXPECT_EQ(first_field, nullptr);
}
