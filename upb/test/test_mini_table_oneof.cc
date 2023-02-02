/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto3.upb.h"
#include "upb/mini_table/common.h"
#include "upb/test/test.upb.h"
#include "upb/upb.hpp"

// Must be last.
#include "upb/port/def.inc"

TEST(MiniTableOneofTest, OneOfIteratorProto2) {
  constexpr int oneof_first_field_number = 111;
  constexpr int oneof_test_field_number = 116;

  const upb_MiniTable* google_protobuf_table =
      &protobuf_test_messages_proto2_TestAllTypesProto2_msg_init;
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(google_protobuf_table, oneof_test_field_number);
  ASSERT_TRUE(field != nullptr);
  const upb_MiniTableField* ptr = upb_MiniTable_GetOneof(google_protobuf_table, field);
  int field_num = oneof_first_field_number;
  do {
    EXPECT_EQ(ptr->number, field_num++);
  } while (upb_MiniTable_NextOneofField(google_protobuf_table, &ptr));
}

TEST(MiniTableOneofTest, InitialFieldNotOneOf) {
  constexpr int test_field_number = 1;  // optional int that is not a oneof
  const upb_MiniTable* google_protobuf_table =
      &protobuf_test_messages_proto2_TestAllTypesProto2_msg_init;
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(google_protobuf_table, test_field_number);
  ASSERT_TRUE(field != nullptr);
  const upb_MiniTableField* first_field =
      upb_MiniTable_GetOneof(google_protobuf_table, field);
  EXPECT_EQ(first_field, nullptr);
}
