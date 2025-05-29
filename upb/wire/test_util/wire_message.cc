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
  ABSL_CHECK_EQ(upb_WireReader_ReadVarint(tmp.data(), &parsed_val),
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

std::string ToBinaryPayloadWithLongVarints(const WireMessage& msg,
                                           int min_tag_length,
                                           int min_val_varint_length) {
  std::string ret;

  for (const auto& field : msg) {
    if (const auto* val = std::get_if<Varint>(&field.value)) {
      EncodeTag(field, kUpb_WireType_Varint, min_tag_length, &ret);
      EncodeVarint(val->val, min_val_varint_length, &ret);
    } else if (const auto* val = std::get_if<Delimited>(&field.value)) {
      EncodeTag(field, kUpb_WireType_Delimited, min_tag_length, &ret);
      EncodeVarint(val->val.size(), min_val_varint_length, &ret);
      ret.append(val->val);
    } else if (const auto* val = std::get_if<Fixed64>(&field.value)) {
      EncodeTag(field, kUpb_WireType_64Bit, min_tag_length, &ret);
      uint64_t swapped = upb_BigEndian64(val->val);
      ret.append(reinterpret_cast<const char*>(&swapped), sizeof(swapped));
    } else if (const auto* val = std::get_if<Fixed32>(&field.value)) {
      EncodeTag(field, kUpb_WireType_32Bit, min_tag_length, &ret);
      uint32_t swapped = upb_BigEndian32(val->val);
      ret.append(reinterpret_cast<const char*>(&swapped), sizeof(swapped));
    } else if (const auto* val = std::get_if<Group>(&field.value)) {
      EncodeTag(field, kUpb_WireType_StartGroup, min_tag_length, &ret);
      ret.append(ToBinaryPayloadWithLongVarints(val->val, min_tag_length,
                                                min_val_varint_length));
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
