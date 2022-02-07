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

class StringAppender {
 public:
  StringAppender(upb_MtDataEncoder* e, std::string* str) : str_(str) {
    e->end = buf_ + sizeof(buf_);
  }

  template <class T>
  bool operator()(T&& func) {
    char* end = func(buf_);
    if (!end) return false;
    str_->append(buf_, end - buf_);
    return true;
  }

 private:
  char buf_[kUpb_MtDataEncoder_MinSize];
  std::string* str_;
};

// We can consider putting these in a standard upb .hpp header.
bool StartMessage(upb_MtDataEncoder* e, uint64_t msg_mod, std::string* out) {
  StringAppender appender(e, out);
  return appender([=](char* buf) {
    return upb_MtDataEncoder_StartMessage(e, buf, msg_mod);
  });
}

bool PutField(upb_MtDataEncoder* e, upb_FieldType type, uint32_t field_num,
              uint64_t field_mod, std::string* out) {
  StringAppender appender(e, out);
  return appender([=](char* buf) {
    return upb_MtDataEncoder_PutField(e, buf, type, field_num, field_mod);
  });
}

bool StartOneof(upb_MtDataEncoder* e, std::string* out) {
  StringAppender appender(e, out);
  return appender([=](char* buf) { return upb_MtDataEncoder_StartOneof(e, buf); });
}

bool PutOneofField(upb_MtDataEncoder* e, uint32_t field_num, std::string* out) {
  StringAppender appender(e, out);
  return appender([=](char* buf) {
    return upb_MtDataEncoder_PutOneofField(e, buf, field_num);
  });
}

class MiniTableTest : public testing::TestWithParam<upb_MiniTablePlatform> {};

TEST_P(MiniTableTest, Empty) {
  upb::Arena arena;
  upb::Status status;
  upb_MiniTable* table =
      upb_MiniTable_Build(NULL, 0, GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(0, table->field_count);
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, AllScalarTypes) {
  upb::Arena arena;
  std::string input;
  upb_MtDataEncoder e;
  ASSERT_TRUE(StartMessage(&e, 0, &input));
  int count = 0;
  for (int i = kUpb_FieldType_Double ; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(PutField(&e, static_cast<upb_FieldType>(i), i, 0, &input));
    count++;
  }
  fprintf(stderr, "YO: %s\n", input.c_str());
  upb::Status status;
  upb_MiniTable* table = upb_MiniTable_Build(
      input.data(), input.size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTable_Field* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
    EXPECT_EQ(i + kUpb_FieldType_Double, f->descriptortype);
    EXPECT_EQ(kUpb_FieldMode_Scalar, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, AllRepeatedTypes) {
  upb::Arena arena;
  std::string input;
  upb_MtDataEncoder e;
  ASSERT_TRUE(StartMessage(&e, 0, &input));
  int count = 0;
  for (int i = kUpb_FieldType_Double ; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(PutField(&e, static_cast<upb_FieldType>(i), i,
                         kUpb_FieldModifier_IsRepeated, &input));
    count++;
  }
  fprintf(stderr, "YO: %s\n", input.c_str());
  upb::Status status;
  upb_MiniTable* table = upb_MiniTable_Build(
      input.data(), input.size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTable_Field* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
    EXPECT_EQ(i + kUpb_FieldType_Double, f->descriptortype);
    EXPECT_EQ(kUpb_FieldMode_Array, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, Skips) {
  upb::Arena arena;
  std::string input;
  upb_MtDataEncoder e;
  ASSERT_TRUE(StartMessage(&e, 0, &input));
  int count = 0;
  std::vector<int> field_numbers;
  for (int i = 0; i < 25; i++) {
    int field_number = 1 << i;
    field_numbers.push_back(field_number);
    ASSERT_TRUE(PutField(&e, kUpb_FieldType_Float, field_number, 0, &input));
    count++;
  }
  fprintf(stderr, "YO: %s, %zu\n", input.c_str(), input.size());
  upb::Status status;
  upb_MiniTable* table = upb_MiniTable_Build(
      input.data(), input.size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (size_t i = 0; i < field_numbers.size(); i++) {
    const upb_MiniTable_Field* f = &table->fields[i];
    EXPECT_EQ(field_numbers[i], f->number);
    EXPECT_EQ(kUpb_FieldType_Float, f->descriptortype);
    EXPECT_EQ(kUpb_FieldMode_Scalar, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, AllScalarTypesOneof) {
  upb::Arena arena;
  std::string input;
  upb_MtDataEncoder e;
  ASSERT_TRUE(StartMessage(&e, 0, &input));
  int count = 0;
  for (int i = kUpb_FieldType_Double ; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(PutField(&e, static_cast<upb_FieldType>(i), i, 0, &input));
    count++;
  }
  ASSERT_TRUE(StartOneof(&e, &input));
  for (int i = kUpb_FieldType_Double ; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(PutOneofField(&e, i, &input));
  }
  fprintf(stderr, "YO: %s\n", input.c_str());
  upb::Status status;
  upb_MiniTable* table = upb_MiniTable_Build(
      input.data(), input.size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTable_Field* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
    EXPECT_EQ(i + kUpb_FieldType_Double, f->descriptortype);
    EXPECT_EQ(kUpb_FieldMode_Scalar, f->mode & kUpb_FieldMode_Mask);
    // For a oneof all fields have the same offset.
    EXPECT_EQ(table->fields[0].offset, f->offset);
    // All presence fields should point to the same oneof case offset.
    size_t case_ofs = _upb_oneofcase_ofs(f);
    EXPECT_EQ(table->fields[0].presence, f->presence);
    EXPECT_TRUE(f->offset < table->size);
    EXPECT_TRUE(case_ofs < table->size);
    EXPECT_TRUE(case_ofs != f->offset);
  }
  EXPECT_EQ(0, table->required_count);
}

INSTANTIATE_TEST_SUITE_P(Platforms, MiniTableTest,
                         testing::Values(kUpb_MiniTablePlatform_32Bit,
                                         kUpb_MiniTablePlatform_64Bit));
