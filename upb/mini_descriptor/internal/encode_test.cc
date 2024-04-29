// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_descriptor/internal/encode.hpp"

#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "google/protobuf/descriptor.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/message/internal/accessors.h"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_descriptor/internal/base92.h"
#include "upb/mini_descriptor/internal/modifiers.h"
#include "upb/mini_table/enum.h"
#include "upb/wire/decode.h"

// begin:google_only
// #include "testing/fuzzing/fuzztest.h"
// end:google_only

namespace protobuf = ::google::protobuf;

class MiniTableTest : public testing::TestWithParam<upb_MiniTablePlatform> {};

TEST_P(MiniTableTest, Empty) {
  upb::Arena arena;
  upb::Status status;
  upb_MiniTable* table =
      _upb_MiniTable_Build(nullptr, 0, GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(0, table->field_count);
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, AllScalarTypes) {
  upb::Arena arena;
  upb::MtDataEncoder e;
  ASSERT_TRUE(e.StartMessage(0));
  int count = 0;
  for (int i = kUpb_FieldType_Double; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(e.PutField(static_cast<upb_FieldType>(i), i, 0));
    count++;
  }
  upb::Status status;
  upb_MiniTable* table = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTableField* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
    EXPECT_EQ(kUpb_FieldMode_Scalar, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, AllRepeatedTypes) {
  upb::Arena arena;
  upb::MtDataEncoder e;
  ASSERT_TRUE(e.StartMessage(0));
  int count = 0;
  for (int i = kUpb_FieldType_Double; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(e.PutField(static_cast<upb_FieldType>(i), i,
                           kUpb_FieldModifier_IsRepeated));
    count++;
  }
  upb::Status status;
  upb_MiniTable* table = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTableField* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
    EXPECT_EQ(kUpb_FieldMode_Array, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, Skips) {
  upb::Arena arena;
  upb::MtDataEncoder e;
  ASSERT_TRUE(e.StartMessage(0));
  int count = 0;
  std::vector<int> field_numbers;
  for (int i = 0; i < 25; i++) {
    int field_number = 1 << i;
    field_numbers.push_back(field_number);
    ASSERT_TRUE(e.PutField(kUpb_FieldType_Float, field_number, 0));
    count++;
  }
  upb::Status status;
  upb_MiniTable* table = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (size_t i = 0; i < field_numbers.size(); i++) {
    const upb_MiniTableField* f = &table->fields[i];
    EXPECT_EQ(field_numbers[i], f->number);
    EXPECT_EQ(kUpb_FieldType_Float, upb_MiniTableField_Type(f));
    EXPECT_EQ(kUpb_FieldMode_Scalar, f->mode & kUpb_FieldMode_Mask);
    EXPECT_TRUE(offsets.insert(f->offset).second);
    EXPECT_TRUE(f->offset < table->size);
  }
  EXPECT_EQ(0, table->required_count);
}

TEST_P(MiniTableTest, AllScalarTypesOneof) {
  upb::Arena arena;
  upb::MtDataEncoder e;
  ASSERT_TRUE(e.StartMessage(0));
  int count = 0;
  for (int i = kUpb_FieldType_Double; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(e.PutField(static_cast<upb_FieldType>(i), i, 0));
    count++;
  }
  ASSERT_TRUE(e.StartOneof());
  for (int i = kUpb_FieldType_Double; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(e.PutOneofField(i));
  }
  upb::Status status;
  upb_MiniTable* table = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table) << status.error_message();
  EXPECT_EQ(count, table->field_count);
  absl::flat_hash_set<size_t> offsets;
  for (int i = 0; i < 16; i++) {
    const upb_MiniTableField* f = &table->fields[i];
    EXPECT_EQ(i + 1, f->number);
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

TEST_P(MiniTableTest, SizeOverflow) {
  upb::Arena arena;
  upb::MtDataEncoder e;
  // upb can only handle messages up to UINT16_MAX.
  size_t max_double_fields = UINT16_MAX / (sizeof(double) + 1);

  // A bit under max_double_fields is ok.
  ASSERT_TRUE(e.StartMessage(0));
  for (size_t i = 1; i < max_double_fields; i++) {
    ASSERT_TRUE(e.PutField(kUpb_FieldType_Double, i, 0));
  }
  upb::Status status;
  upb_MiniTable* table = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table) << status.error_message();

  // A bit over max_double_fields fails.
  ASSERT_TRUE(e.StartMessage(0));
  for (size_t i = 1; i < max_double_fields + 2; i++) {
    ASSERT_TRUE(e.PutField(kUpb_FieldType_Double, i, 0));
  }
  upb_MiniTable* table2 = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_EQ(nullptr, table2) << status.error_message();
}

INSTANTIATE_TEST_SUITE_P(Platforms, MiniTableTest,
                         testing::Values(kUpb_MiniTablePlatform_32Bit,
                                         kUpb_MiniTablePlatform_64Bit));

TEST(MiniTablePlatformIndependentTest, Base92Roundtrip) {
  for (char i = 0; i < 92; i++) {
    EXPECT_EQ(i, _upb_FromBase92(_upb_ToBase92(i)));
  }
}

TEST(MiniTablePlatformIndependentTest, IsTypePackable) {
  for (int i = 1; i <= protobuf::FieldDescriptor::MAX_TYPE; i++) {
    EXPECT_EQ(upb_FieldType_IsPackable(static_cast<upb_FieldType>(i)),
              protobuf::FieldDescriptor::IsTypePackable(
                  static_cast<protobuf::FieldDescriptor::Type>(i)));
  }
}

TEST(MiniTableEnumTest, Enum) {
  upb::Arena arena;
  upb::MtDataEncoder e;

  ASSERT_TRUE(e.StartEnum());
  absl::flat_hash_set<int32_t> values;
  for (int i = 0; i < 256; i++) {
    values.insert(i * 2);
    e.PutEnumValue(i * 2);
  }
  e.EndEnum();

  upb::Status status;
  upb_MiniTableEnum* table = upb_MiniTableEnum_Build(
      e.data().data(), e.data().size(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table) << status.error_message();

  for (int i = 0; i < UINT16_MAX; i++) {
    EXPECT_EQ(values.contains(i), upb_MiniTableEnum_CheckValue(table, i)) << i;
  }
}

TEST_P(MiniTableTest, SubsInitializedToEmpty) {
  upb::Arena arena;
  upb::MtDataEncoder e;
  // Create mini table with 2 message fields.
  ASSERT_TRUE(e.StartMessage(0));
  ASSERT_TRUE(e.PutField(kUpb_FieldType_Message, 15, 0));
  ASSERT_TRUE(e.PutField(kUpb_FieldType_Message, 16, 0));
  upb::Status status;
  upb_MiniTable* table = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(table->field_count, 2);
  EXPECT_EQ(table->subs[0].submsg, &_kUpb_MiniTable_Empty);
  EXPECT_EQ(table->subs[1].submsg, &_kUpb_MiniTable_Empty);
}

TEST(MiniTableEnumTest, PositiveAndNegative) {
  upb::Arena arena;
  upb::MtDataEncoder e;

  ASSERT_TRUE(e.StartEnum());
  absl::flat_hash_set<int32_t> values;
  for (int i = 0; i < 100; i++) {
    values.insert(i);
    e.PutEnumValue(i);
  }
  for (int i = 100; i > 0; i--) {
    values.insert(-i);
    e.PutEnumValue(-i);
  }
  e.EndEnum();

  upb::Status status;
  upb_MiniTableEnum* table = upb_MiniTableEnum_Build(
      e.data().data(), e.data().size(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table) << status.error_message();

  for (int i = -UINT16_MAX; i < UINT16_MAX; i++) {
    EXPECT_EQ(values.contains(i), upb_MiniTableEnum_CheckValue(table, i)) << i;
  }
}

TEST_P(MiniTableTest, Extendible) {
  upb::Arena arena;
  upb::MtDataEncoder e;
  ASSERT_TRUE(e.StartMessage(kUpb_MessageModifier_IsExtendable));
  for (int i = kUpb_FieldType_Double; i < kUpb_FieldType_SInt64; i++) {
    ASSERT_TRUE(e.PutField(static_cast<upb_FieldType>(i), i, 0));
  }
  upb::Status status;
  upb_MiniTable* table = _upb_MiniTable_Build(
      e.data().data(), e.data().size(), GetParam(), arena.ptr(), status.ptr());
  ASSERT_NE(nullptr, table);
  EXPECT_EQ(kUpb_ExtMode_Extendable, table->ext & kUpb_ExtMode_Extendable);
}

// begin:google_only
//
// static void BuildMiniTable(std::string_view s, bool is_32bit) {
//   upb::Arena arena;
//   upb::Status status;
//   _upb_MiniTable_Build(
//       s.data(), s.size(),
//       is_32bit ? kUpb_MiniTablePlatform_32Bit : kUpb_MiniTablePlatform_64Bit,
//       arena.ptr(), status.ptr());
// }
// FUZZ_TEST(FuzzTest, BuildMiniTable);
//
// TEST(FuzzTest, BuildMiniTableRegression) {
//   BuildMiniTable("g}{v~fq{\271", false);
// }
//
// end:google_only
