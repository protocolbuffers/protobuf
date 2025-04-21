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
#include <string>

#include "absl/base/casts.h"
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


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
