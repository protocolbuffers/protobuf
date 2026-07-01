// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/wire_format.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "absl/base/casts.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"
#include "google/protobuf/unittest_mset.pb.h"
#include "google/protobuf/unittest_mset_wire_format.pb.h"
#include "google/protobuf/unittest_proto3_arena.pb.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/wire_format_unittest.h"
#include <gtest/gtest.h>

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

PROTOBUF_INSTANTIATE_WIRE_FORMAT_UNITTEST(, );

TEST(RepeatedVarint, Int32) {
  RepeatedField<int32_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar Int32Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::Int32Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::Int32Size(v));
}

TEST(RepeatedVarint, Int64) {
  RepeatedField<int64_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar Int64Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::Int64Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::Int64Size(v));
}

TEST(RepeatedVarint, SInt32) {
  RepeatedField<int32_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar SInt32Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::SInt32Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::SInt32Size(v));
}

TEST(RepeatedVarint, SInt64) {
  RepeatedField<int64_t> v;

  // Insert -2^n, 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(-(1 << n));
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar SInt64Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::SInt64Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::SInt64Size(v));
}

TEST(RepeatedVarint, UInt32) {
  RepeatedField<uint32_t> v;

  // Insert 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar UInt32Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::UInt32Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::UInt32Size(v));
}

TEST(RepeatedVarint, UInt64) {
  RepeatedField<uint64_t> v;

  // Insert 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar UInt64Size.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::UInt64Size(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::UInt64Size(v));
}

TEST(RepeatedVarint, Enum) {
  RepeatedField<int> v;

  // Insert 2^n and 2^n-1.
  for (int n = 0; n < 10; n++) {
    v.Add(1 << n);
    v.Add((1 << n) - 1);
  }

  // Check consistency with the scalar EnumSize.
  size_t expected = 0;
  for (int i = 0; i < v.size(); i++) {
    expected += WireFormatLite::EnumSize(v[i]);
  }

  EXPECT_EQ(expected, WireFormatLite::EnumSize(v));
}

TEST(WireFormatTest, EnumsInSync) {
  // Verify that WireFormatLite::FieldType and WireFormatLite::CppType match
  // FieldDescriptor::Type and FieldDescriptor::CppType.

  EXPECT_EQ(absl::implicit_cast<int>(FieldDescriptor::MAX_TYPE),
            absl::implicit_cast<int>(WireFormatLite::MAX_FIELD_TYPE));
  EXPECT_EQ(absl::implicit_cast<int>(FieldDescriptor::MAX_CPPTYPE),
            absl::implicit_cast<int>(WireFormatLite::MAX_CPPTYPE));

  for (int i = 1; i <= WireFormatLite::MAX_FIELD_TYPE; i++) {
    EXPECT_EQ(absl::implicit_cast<int>(FieldDescriptor::TypeToCppType(
                  static_cast<FieldDescriptor::Type>(i))),
              absl::implicit_cast<int>(WireFormatLite::FieldTypeToCppType(
                  static_cast<WireFormatLite::FieldType>(i))));
  }
}

TEST(WireFormatTest, MaxFieldNumber) {
  // Make sure the max field number constant is accurate.
  EXPECT_EQ((1 << (32 - WireFormatLite::kTagTypeBits)) - 1,
            FieldDescriptor::kMaxNumber);
}

TEST(WireFormatTest, CppTypeForWorksForAllSupportedTypes) {
  using WFL = WireFormatLite;
  EXPECT_EQ(WFL::CppTypeFor<int32_t>(), WFL::CPPTYPE_INT32);
  EXPECT_EQ(WFL::CppTypeFor<int64_t>(), WFL::CPPTYPE_INT64);
  EXPECT_EQ(WFL::CppTypeFor<uint32_t>(), WFL::CPPTYPE_UINT32);
  EXPECT_EQ(WFL::CppTypeFor<uint64_t>(), WFL::CPPTYPE_UINT64);
  EXPECT_EQ(WFL::CppTypeFor<float>(), WFL::CPPTYPE_FLOAT);
  EXPECT_EQ(WFL::CppTypeFor<double>(), WFL::CPPTYPE_DOUBLE);
  EXPECT_EQ(WFL::CppTypeFor<bool>(), WFL::CPPTYPE_BOOL);
  EXPECT_EQ(WFL::CppTypeFor<proto2_unittest::TestAllTypes::NestedEnum>(),
            WFL::CPPTYPE_ENUM);
  EXPECT_EQ(WFL::CppTypeFor<std::string>(), WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<absl::Cord>(), WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<absl::string_view>(), WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<google::protobuf::MessageLite>(), WFL::CPPTYPE_MESSAGE);
  EXPECT_EQ(WFL::CppTypeFor<proto2_unittest::TestAllTypes>(),
            WFL::CPPTYPE_MESSAGE);

  // And repeated too
  EXPECT_EQ(WFL::CppTypeFor<RepeatedField<int32_t>>(), WFL::CPPTYPE_INT32);
  EXPECT_EQ(WFL::CppTypeFor<RepeatedField<double>>(), WFL::CPPTYPE_DOUBLE);
  EXPECT_EQ(WFL::CppTypeFor<RepeatedPtrField<std::string>>(),
            WFL::CPPTYPE_STRING);
  EXPECT_EQ(WFL::CppTypeFor<RepeatedPtrField<proto2_unittest::TestAllTypes>>(),
            WFL::CPPTYPE_MESSAGE);
}

TEST(WireFormatLiteTest, ReadPackedFixed32) {
  uint8_t buffer[64];
  uint8_t* ptr = buffer;
  *ptr++ = 12;  // varint: 3 * sizeof(uint32_t)
  ptr = WireFormatLite::WriteFixed32NoTagToArray(1, ptr);
  ptr = WireFormatLite::WriteFixed32NoTagToArray(2, ptr);
  ptr = WireFormatLite::WriteFixed32NoTagToArray(3, ptr);

  io::CodedInputStream input(buffer, static_cast<int>(ptr - buffer));
  RepeatedField<uint32_t> result;
  EXPECT_TRUE(
      (WireFormatLite::ReadPackedPrimitive<uint32_t,
                                           WireFormatLite::TYPE_FIXED32>(
          &input, &result)));
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result.Get(0), 1u);
  EXPECT_EQ(result.Get(1), 2u);
  EXPECT_EQ(result.Get(2), 3u);
}

TEST(WireFormatLiteTest, ReadPackedFixed64) {
  uint8_t buffer[64];
  uint8_t* ptr = buffer;
  *ptr++ = 16;  // varint: 2 * sizeof(uint64_t)
  ptr = WireFormatLite::WriteFixed64NoTagToArray(100, ptr);
  ptr = WireFormatLite::WriteFixed64NoTagToArray(200, ptr);

  io::CodedInputStream input(buffer, static_cast<int>(ptr - buffer));
  RepeatedField<uint64_t> result;
  EXPECT_TRUE(
      (WireFormatLite::ReadPackedPrimitive<uint64_t,
                                           WireFormatLite::TYPE_FIXED64>(
          &input, &result)));
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result.Get(0), 100u);
  EXPECT_EQ(result.Get(1), 200u);
}

TEST(WireFormatLiteTest, ReadPackedFixed32RejectsUnalignedLength) {
  uint8_t buffer[6];
  buffer[0] = 5;  // not a multiple of sizeof(uint32_t)
  std::memset(buffer + 1, 0, 5);

  io::CodedInputStream input(buffer, sizeof(buffer));
  RepeatedField<uint32_t> result;
  EXPECT_FALSE(
      (WireFormatLite::ReadPackedPrimitive<uint32_t,
                                           WireFormatLite::TYPE_FIXED32>(
          &input, &result)));
}

TEST(WireFormatLiteTest, ReadPackedFixed32Accumulates) {
  uint8_t buf1[64];
  uint8_t* p1 = buf1;
  *p1++ = 8;  // 2 fixed32s
  p1 = WireFormatLite::WriteFixed32NoTagToArray(10, p1);
  p1 = WireFormatLite::WriteFixed32NoTagToArray(20, p1);

  uint8_t buf2[64];
  uint8_t* p2 = buf2;
  *p2++ = 12;  // 3 fixed32s
  p2 = WireFormatLite::WriteFixed32NoTagToArray(30, p2);
  p2 = WireFormatLite::WriteFixed32NoTagToArray(40, p2);
  p2 = WireFormatLite::WriteFixed32NoTagToArray(50, p2);

  RepeatedField<uint32_t> result;

  io::CodedInputStream input1(buf1, static_cast<int>(p1 - buf1));
  EXPECT_TRUE(
      (WireFormatLite::ReadPackedPrimitive<uint32_t,
                                           WireFormatLite::TYPE_FIXED32>(
          &input1, &result)));

  io::CodedInputStream input2(buf2, static_cast<int>(p2 - buf2));
  EXPECT_TRUE(
      (WireFormatLite::ReadPackedPrimitive<uint32_t,
                                           WireFormatLite::TYPE_FIXED32>(
          &input2, &result)));

  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result.Get(0), 10u);
  EXPECT_EQ(result.Get(1), 20u);
  EXPECT_EQ(result.Get(2), 30u);
  EXPECT_EQ(result.Get(3), 40u);
  EXPECT_EQ(result.Get(4), 50u);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
