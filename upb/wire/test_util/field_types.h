// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_WIRE_TEST_WIRE_TYPES_H__
#define GOOGLE_UPB_UPB_WIRE_TEST_WIRE_TYPES_H__

#include <cstdint>
#include <limits>
#include <string>

#include <gtest/gtest.h>
#include "absl/base/casts.h"
#include "absl/strings/string_view.h"
#include "upb/base/descriptor_constants.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/test_util/wire_message.h"

namespace upb {
namespace test {
namespace field_types {

// This set of field types is similar to upb_FieldType, but it also includes
// some extra distinctions like closed vs. open enum and validated vs.
// unvalidated UTF-8.

struct Fixed32 {
  using Value = uint32_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Fixed32;
  static constexpr absl::string_view kName = "Fixed32";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Fixed32;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Fixed32(static_cast<Value>(value));
  }
};

struct Fixed64 {
  using Value = uint64_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Fixed64;
  static constexpr absl::string_view kName = "Fixed64";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Fixed64;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Fixed64(static_cast<Value>(value));
  }
};

struct SFixed32 {
  using Value = int32_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_SFixed32;
  static constexpr absl::string_view kName = "SFixed32";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Fixed32;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Fixed32(static_cast<uint32_t>(value));
  }
};

struct SFixed64 {
  using Value = int64_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_SFixed64;
  static constexpr absl::string_view kName = "SFixed64";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Fixed64;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Fixed64(static_cast<uint64_t>(value));
  }
};

struct Float {
  using Value = float;
  static constexpr Value kZero = 0.0f;
  static constexpr Value kMin = -1234.5f;
  static constexpr Value kMax = 1234.5f;
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Float;
  static constexpr absl::string_view kName = "Float";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Fixed32;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Fixed32(
        absl::bit_cast<uint32_t>(static_cast<Value>(value)));
  }
};

struct Double {
  using Value = double;
  static constexpr Value kZero = 0.0;
  static constexpr Value kMin = -1234567.89;
  static constexpr Value kMax = 1234567.89;
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Double;
  static constexpr absl::string_view kName = "Double";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Fixed64;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Fixed64(
        absl::bit_cast<uint64_t>(static_cast<Value>(value)));
  }
};

struct Int32 {
  using Value = int32_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Int32;
  static constexpr absl::string_view kName = "Int32";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Varint32;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    // Need to sign-extend to 64-bit varint.
    return wire_types::Varint(static_cast<int64_t>(static_cast<Value>(value)));
  }
};

struct Int64 {
  using Value = int64_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Int64;
  static constexpr absl::string_view kName = "Int64";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Varint64;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Varint(static_cast<int64_t>(value));
  }
};

struct UInt32 {
  using Value = uint32_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_UInt32;
  static constexpr absl::string_view kName = "UInt32";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Varint32;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Varint(static_cast<Value>(value));
  }
};

struct UInt64 {
  using Value = uint64_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_UInt64;
  static constexpr absl::string_view kName = "UInt64";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Varint64;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Varint(static_cast<Value>(value));
  }
};

struct SInt32 {
  using Value = int32_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_SInt32;
  static constexpr absl::string_view kName = "SInt32";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_ZigZag32;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    int32_t v = static_cast<Value>(value);
    uint32_t uvalue = ((uint32_t)v << 1) ^ (v >> 31);
    return wire_types::Varint(uvalue);
  }
};

struct SInt64 {
  using Value = int64_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = std::numeric_limits<Value>::min();
  static constexpr Value kMax = std::numeric_limits<Value>::max();
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_SInt64;
  static constexpr absl::string_view kName = "SInt64";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_ZigZag64;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    int64_t v = static_cast<Value>(value);
    uint64_t uvalue = ((uint64_t)v << 1) ^ (v >> 63);
    return wire_types::Varint(uvalue);
  }
};

struct Bool {
  using Value = bool;
  static constexpr Value kZero = false;
  static constexpr Value kMin = false;
  static constexpr Value kMax = true;
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Bool;
  static constexpr absl::string_view kName = "Bool";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Bool;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Varint(static_cast<Value>(value));
  }
};

struct String {
  using Value = std::string;
  static constexpr absl::string_view kZero = "";
  static constexpr absl::string_view kMin = "a very minimum valued string!";
  static constexpr absl::string_view kMax = "a very maximum valued string!";
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_String;
  static constexpr absl::string_view kName = "String";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_String;

  static wire_types::WireValue WireValue(std::string value) {
    return wire_types::Delimited(value);
  }
};

struct Bytes {
  using Value = std::string;
  static constexpr absl::string_view kZero = "";
  static constexpr absl::string_view kMin = "a very minimum valued bytes!";
  static constexpr absl::string_view kMax = "a very maximum valued bytes!";
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Bytes;
  static constexpr absl::string_view kName = "Bytes";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Bytes;

  static wire_types::WireValue WireValue(std::string value) {
    return wire_types::Delimited(value);
  }
};

struct Message {
  using Value = std::string;
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Message;
  static constexpr absl::string_view kName = "Message";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_Message;

  static wire_types::WireValue WireValue(std::string value) {
    return wire_types::Delimited(value);
  }
};

struct ClosedEnum {
  using Value = int32_t;
  static constexpr Value kZero = 0;
  static constexpr Value kMin = 1;
  static constexpr Value kMax = 2;
  static constexpr upb_FieldType kFieldType = kUpb_FieldType_Enum;
  static constexpr absl::string_view kName = "ClosedEnum";
  static constexpr upb_DecodeFast_Type kFastType = kUpb_DecodeFast_ClosedEnum;

  template <class T>
  static wire_types::WireValue WireValue(T value) {
    return wire_types::Varint(static_cast<int64_t>(static_cast<Value>(value)));
  }
};

// TODO: Group

}  // namespace field_types

using PackableFieldTypes =
    testing::Types<field_types::Fixed32, field_types::Fixed64,
                   field_types::SFixed32, field_types::SFixed64,
                   field_types::Float, field_types::Double, field_types::Int32,
                   field_types::Int64, field_types::UInt32, field_types::UInt64,
                   field_types::SInt32, field_types::SInt64, field_types::Bool,
                   field_types::ClosedEnum>;

using FieldTypes =
    testing::Types<field_types::Fixed32, field_types::Fixed64,
                   field_types::SFixed32, field_types::SFixed64,
                   field_types::Float, field_types::Double, field_types::Int32,
                   field_types::Int64, field_types::UInt32, field_types::UInt64,
                   field_types::SInt32, field_types::SInt64, field_types::Bool,
                   field_types::String, field_types::ClosedEnum>;

template <typename Func, typename... Ts>
void ForEachTypeImpl(Func&& func, testing::Types<Ts...>) {
  (std::forward<Func>(func)(Ts{}), ...);
}

template <typename Func>
void ForEachType(Func&& func) {
  ForEachTypeImpl(std::forward<Func>(func), FieldTypes{});
}

}  // namespace test
}  // namespace upb

#endif  // GOOGLE_UPB_UPB_WIRE_TEST_WIRE_TYPES_H__
