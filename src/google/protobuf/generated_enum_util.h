// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_GENERATED_ENUM_UTIL_H__
#define GOOGLE_PROTOBUF_GENERATED_ENUM_UTIL_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/explicitly_constructed.h"
#include "google/protobuf/message_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

// This type trait can be used to cause templates to only match proto2 enum
// types.
template <typename T>
struct is_proto_enum : ::std::false_type {};

namespace internal {

// The table entry format for storing enum name-to-value mapping used with lite
// protos. This struct and the following related functions should only be used
// by protobuf generated code.
struct EnumEntry {
  absl::string_view name;
  int value;
};

// Looks up a numeric enum value given the string name.
PROTOBUF_FUTURE_ADD_EARLY_NODISCARD
PROTOBUF_EXPORT bool LookUpEnumValue(const EnumEntry* enums, size_t size,
                                     absl::string_view name, int* value);

// Looks up an enum name given the numeric value.
PROTOBUF_FUTURE_ADD_EARLY_NODISCARD
PROTOBUF_EXPORT int LookUpEnumName(const EnumEntry* enums,
                                   const int* sorted_indices, size_t size,
                                   int value);

// Initializes the list of enum names in std::string form.
PROTOBUF_FUTURE_ADD_EARLY_NODISCARD
PROTOBUF_EXPORT bool InitializeEnumStrings(
    const EnumEntry* enums, const int* sorted_indices, size_t size,
    internal::ExplicitlyConstructed<std::string>* enum_strings);

// The enum validation format is split in 3 parts:
//  - A dense sequence, with start+length
//  - A variable size presence bitmap (in increments of 32 bits)
//  - A variable size sorted int32_t set for everything else.
//
// The values are as follows:
//
// 0 - [ sequence start (int16_t) ] | [ sequence size (uint16_t) ] << 16
// 1 - [ bitmap size in bits (uint16_t) ] | [ ordered size (uint16_t) ] << 16
// x - [ variable length bitmap ]
// y - [ variable length of int32_t values ]
//
// where the bitmap starts right after the end of the sequence.
PROTOBUF_FUTURE_ADD_EARLY_NODISCARD
PROTOBUF_EXPORT bool ValidateEnum(int value, const uint32_t* data);
PROTOBUF_FUTURE_ADD_EARLY_NODISCARD
PROTOBUF_EXPORT std::vector<uint32_t> GenerateEnumData(
    absl::Span<const int32_t> values);

PROTOBUF_FUTURE_ADD_EARLY_NODISCARD
PROTOBUF_ALWAYS_INLINE bool ValidateEnumInlined(int value,
                                                const uint32_t* data) {
  const int16_t min_seq = static_cast<int16_t>(data[0] & 0xFFFF);
  const uint16_t length_seq = static_cast<uint16_t>(data[0] >> 16);
  uint64_t adjusted =
      static_cast<uint64_t>(static_cast<int64_t>(value)) - min_seq;
  // Check if the value is within the sequential part.
  if (ABSL_PREDICT_TRUE(adjusted < length_seq)) {
    return true;
  }

  const uint16_t length_bitmap = static_cast<uint16_t>(data[1] & 0xFFFF);
  adjusted -= length_seq;
  // Check if the value is within the bitmap.
  if (ABSL_PREDICT_TRUE(adjusted < length_bitmap)) {
    return ((data[2 + (adjusted / 32)] >> (adjusted % 32)) & 1) == 1;
  }

  // Check if the value is on the ordered part.
  const uint16_t num_ordered = static_cast<uint16_t>(data[1] >> 16);
  data += 2 + length_bitmap / 32;
  size_t pos = 0;
  while (pos < num_ordered) {
    const int32_t sample = static_cast<int32_t>(data[pos]);
    if (sample == value) return true;
    pos = 2 * pos + (sample > value ? 1 : 2);
  }
  return false;
}

// Abseil flag implementation for LITE enums.
template <typename E, bool is_lite>
using EnableIfProtoEnum = std::enable_if_t<
    is_proto_enum<E>::value && is_lite == LiteEnumFuncs<E>::kIsDefined, int>;

namespace generated_enum {
// We inject these functions in the user namespace to allow for ADL on the
// enums.
// These overloads handle LITE enums.
template <typename Enum, EnableIfProtoEnum<Enum, true> = 0>
bool AbslParseFlag(absl::string_view text, Enum* e, std::string* error) {
  if (LiteEnumFuncs<Enum>::kParseFunc(text, e)) return true;

  // Try as lower case
  if (absl::AsciiStrToLower(text) == text &&
      LiteEnumFuncs<Enum>::kParseFunc(absl::AsciiStrToUpper(text), e)) {
    return true;
  }

  // Try as a number
  int as_number;
  if (absl::SimpleAtoi(text, &as_number) &&
      ValidateEnum(as_number, EnumTraits<Enum>::validation_data())) {
    *e = static_cast<Enum>(as_number);
    return true;
  }

  return false;
}

template <typename Enum, EnableIfProtoEnum<Enum, true> = 0>
std::string AbslUnparseFlag(Enum e) {
  absl::string_view name = LiteEnumFuncs<Enum>::kNameFunc(e);
  return name.empty() ? absl::StrCat(static_cast<int>(e)) : std::string(name);
}

// Overloads for std::vector<E> for a list of enum values.
template <typename Enum, std::enable_if_t<is_proto_enum<Enum>::value, int> = 0>
bool AbslParseFlag(absl::string_view text, std::vector<Enum>* e,
                   std::string* error) {
  e->clear();
  if (text.empty()) return true;

  for (absl::string_view p : absl::StrSplit(text, ',')) {
    if (!AbslParseFlag(p, &e->emplace_back(), error)) {
      return false;
    }
  }
  return true;
}

template <typename Enum, std::enable_if_t<is_proto_enum<Enum>::value, int> = 0>
std::string AbslUnparseFlag(const std::vector<Enum>& e) {
  return absl::StrJoin(e, ",", [](std::string* out, Enum e) {
    return absl::StrAppend(out, AbslUnparseFlag(e));
  });
}

}  // namespace generated_enum

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_ENUM_UTIL_H__
