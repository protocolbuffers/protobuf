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

#include "upb/mini_table.h"

#include "absl/container/flat_hash_set.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "upb/msg_internal.h"
#include "upb/upb.hpp"

// We can consider putting these in a standard upb .hpp header.

static void EncodeField(upb_FieldType type, uint64_t modifiers,
                        std::string* str) {
  char buf[16];
  char* end =
      upb_MiniTable_EncodeField(type, modifiers, buf, buf + sizeof(buf));
  assert(end);
  str->append(buf, end - buf);
}

static void EncodeSkip(uint32_t skip, std::string* str) {
  char buf[16];
  char* end = upb_MiniTable_EncodeSkip(skip, buf, buf + sizeof(buf));
  assert(end);
  str->append(buf, end - buf);
}

static void StartOneofs(std::string* str) {
  char buf[16];
  char* end = upb_MiniTable_StartOneofs(buf, buf + sizeof(buf));
  assert(end);
  str->append(buf, end - buf);
}

static void EncodeOneofField(uint32_t field_num, std::string* str) {
  char buf[16];
  char* end =
      upb_MiniTable_EncodeOneofField(field_num, buf, buf + sizeof(buf));
  assert(end);
  str->append(buf, end - buf);
}

static void EncodeOneofFieldSeparator(std::string* str) {
  char buf[16];
  char* end = upb_MiniTable_EncodeOneofFieldSeparator(buf, buf + sizeof(buf));
  assert(end);
  str->append(buf, end - buf);
}

static void EncodeOneofSeparator(std::string* str) {
  char buf[16];
  char* end = upb_MiniTable_EncodeOneofSeparator(buf, buf + sizeof(buf));
  assert(end);
  str->append(buf, end - buf);
}

TEST(MiniTable, Empty) {
  upb::Arena arena;
  upb_MiniTable* table = upb_MiniTable_Build(NULL, 0, arena.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(0, table->field_count);
  EXPECT_EQ(0, table->required_count);
}

TEST(MiniTable, AllScalarTypes) {
  upb::Arena arena;
  std::string input;
  for (int i = kUpb_FieldType_Double ; i < kUpb_FieldType_SInt64; i++) {
    EncodeField(i, &input);
  }
  fprintf(stderr, "YO: %s\n", input.c_str());
  upb::Status status;
  upb_MiniTable* table = upb_MiniTable_Build(input.data(), input.size(),
                                             arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(16, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTable_Field* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
    EXPECT_EQ(kUpb_FieldMode_Scalar, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST(MiniTable, AllRepeatedTypes) {
  upb::Arena arena;
  std::string input;
  const size_t base = kUpb_EncodedType_RepeatedBase;
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Double));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Float));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Fixed32));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Fixed64));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_SFixed32));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_SFixed64));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Int32));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_UInt32));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_SInt32));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Int64));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_UInt64));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_SInt64));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Enum));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Bool));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_Bytes));
  input.push_back(upb_ToBase92(base + kUpb_EncodedType_String));
  upb_MiniTable* table = upb_MiniTable_Build(input.data(), input.size(), arena.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(16, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTable_Field* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
    EXPECT_EQ(kUpb_FieldMode_Array, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST(MiniTable, Skips) {
}
