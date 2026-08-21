// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/test_util/wire_message.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

#include "absl/log/absl_check.h"
#include "upb/base/internal/endian.h"
#include "upb/wire/reader.h"
#include "upb/wire/types.h"

namespace upb {
namespace test {

using wire_types::Delimited;
using wire_types::Fixed32;
using wire_types::Fixed64;
using wire_types::Group;
using wire_types::Varint;
using wire_types::WireField;
using wire_types::WireMessage;
using wire_types::WireValue;

static void EncodeVarint(uint64_t val, int varint_length, std::string* str) {
  uint64_t v = val;
  std::string tmp;
  for (int i = 0; v > 0 || i < varint_length; i++) {
    if (i > 0) tmp.back() |= 0x80;
    tmp.push_back(v & 0x7fU);
    v >>= 7;
  }

  ABSL_CHECK_GE(tmp.size(), static_cast<size_t>(varint_length)) << val;
  uint64_t parsed_val;
  ABSL_CHECK_EQ(upb_WireReader_ReadVarint(tmp.data(), &parsed_val, nullptr),
                tmp.data() + tmp.size())
      << "val=" << val << ", size=" << tmp.size()
      << ", varint_length=" << varint_length;
  ABSL_CHECK_EQ(parsed_val, val);

  str->append(tmp);
}

static void EncodeTag(const WireField& field, upb_WireType wire_type,
                      int min_tag_length, std::string* str) {
  // Tags are 5 bytes max.
  EncodeVarint(field.field_number << 3 | wire_type, min_tag_length, str);
}

static upb_WireType WireType(const WireValue& value) {
  if (std::holds_alternative<Varint>(value)) {
    return kUpb_WireType_Varint;
  } else if (std::holds_alternative<Delimited>(value)) {
    return kUpb_WireType_Delimited;
  } else if (std::holds_alternative<Fixed64>(value)) {
    return kUpb_WireType_64Bit;
  } else if (std::holds_alternative<Fixed32>(value)) {
    return kUpb_WireType_32Bit;
  } else if (std::holds_alternative<Group>(value)) {
    return kUpb_WireType_StartGroup;
  }
  ABSL_CHECK(false);
  return kUpb_WireType_Varint;
}

std::string ToBinaryPayloadWithLongVarints(const WireValue& value,
                                           int min_tag_length,
                                           int min_val_varint_length) {
  std::string ret;
  if (const auto* val = std::get_if<Varint>(&value)) {
    EncodeVarint(val->val, min_val_varint_length, &ret);
  } else if (const auto* val = std::get_if<Delimited>(&value)) {
    EncodeVarint(val->val.size(), min_val_varint_length, &ret);
    ret.append(val->val);
  } else if (const auto* val = std::get_if<Fixed64>(&value)) {
    uint64_t swapped = upb_BigEndian64(val->val);
    ret.append(reinterpret_cast<const char*>(&swapped), sizeof(swapped));
  } else if (const auto* val = std::get_if<Fixed32>(&value)) {
    uint32_t swapped = upb_BigEndian32(val->val);
    ret.append(reinterpret_cast<const char*>(&swapped), sizeof(swapped));
  } else if (const auto* val = std::get_if<Group>(&value)) {
    ret.append(ToBinaryPayloadWithLongVarints(val->val, min_tag_length,
                                              min_val_varint_length));
  }
  return ret;
}

std::string ToBinaryPayload(const WireValue& value) {
  return ToBinaryPayloadWithLongVarints(value, 1, 1);
}

std::string ToBinaryPayloadWithLongVarints(const WireMessage& msg,
                                           int min_tag_length,
                                           int min_val_varint_length) {
  std::string ret;

  for (const auto& field : msg) {
    EncodeTag(field, WireType(field.value), min_tag_length, &ret);
    ret.append(ToBinaryPayloadWithLongVarints(field.value, min_tag_length,
                                              min_val_varint_length));
    if (std::holds_alternative<Group>(field.value)) {
      EncodeTag(field, kUpb_WireType_EndGroup, min_tag_length, &ret);
    }
  }

  return ret;
}

std::string ToBinaryPayload(const WireMessage& msg) {
  return ToBinaryPayloadWithLongVarints(msg, 1, 1);
}

}  // namespace test
}  // namespace upb
