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

#include "upb/util/compare.h"

#include <stdint.h>

#include <string_view>
#include <vector>

#include "absl/strings/string_view.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

struct UnknownField;

using UnknownFields = std::vector<UnknownField>;

enum class UnknownFieldType {
  kVarint,
  kLongVarint,  // Over-encoded to have distinct wire format.
  kDelimited,
  kFixed64,
  kFixed32,
  kGroup,
};

union UnknownFieldValue {
  uint64_t varint;
  uint64_t fixed64;
  uint32_t fixed32;
  // NULL-terminated (strings must not have embedded NULL).
  const char* delimited;
  UnknownFields* group;
};

struct TypeAndValue {
  UnknownFieldType type;
  UnknownFieldValue value;
};

struct UnknownField {
  uint32_t field_number;
  TypeAndValue value;
};

TypeAndValue Varint(uint64_t val) {
  TypeAndValue ret{UnknownFieldType::kVarint};
  ret.value.varint = val;
  return ret;
}

TypeAndValue LongVarint(uint64_t val) {
  TypeAndValue ret{UnknownFieldType::kLongVarint};
  ret.value.varint = val;
  return ret;
}

TypeAndValue Fixed64(uint64_t val) {
  TypeAndValue ret{UnknownFieldType::kFixed64};
  ret.value.fixed64 = val;
  return ret;
}

TypeAndValue Fixed32(uint32_t val) {
  TypeAndValue ret{UnknownFieldType::kFixed32};
  ret.value.fixed32 = val;
  return ret;
}

TypeAndValue Delimited(const char* val) {
  TypeAndValue ret{UnknownFieldType::kDelimited};
  ret.value.delimited = val;
  return ret;
}

TypeAndValue Group(UnknownFields nested) {
  TypeAndValue ret{UnknownFieldType::kGroup};
  ret.value.group = &nested;
  return ret;
}

void EncodeVarint(uint64_t val, std::string* str) {
  do {
    char byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    str->push_back(byte);
  } while (val);
}

std::string ToBinaryPayload(const UnknownFields& fields) {
  static const upb_WireType wire_types[] = {
      kUpb_WireType_Varint, kUpb_WireType_Varint, kUpb_WireType_Delimited,
      kUpb_WireType_64Bit,  kUpb_WireType_32Bit,  kUpb_WireType_StartGroup,
  };
  std::string ret;

  for (const auto& field : fields) {
    uint32_t tag = field.field_number << 3 |
                   (wire_types[static_cast<int>(field.value.type)]);
    EncodeVarint(tag, &ret);
    switch (field.value.type) {
      case UnknownFieldType::kVarint:
        EncodeVarint(field.value.value.varint, &ret);
        break;
      case UnknownFieldType::kLongVarint:
        EncodeVarint(field.value.value.varint, &ret);
        ret.back() |= 0x80;
        ret.push_back(0);
        break;
      case UnknownFieldType::kDelimited:
        EncodeVarint(strlen(field.value.value.delimited), &ret);
        ret.append(field.value.value.delimited);
        break;
      case UnknownFieldType::kFixed64: {
        uint64_t val = _upb_BigEndian_Swap64(field.value.value.fixed64);
        ret.append(reinterpret_cast<const char*>(&val), sizeof(val));
        break;
      }
      case UnknownFieldType::kFixed32: {
        uint32_t val = _upb_BigEndian_Swap32(field.value.value.fixed32);
        ret.append(reinterpret_cast<const char*>(&val), sizeof(val));
        break;
      }
      case UnknownFieldType::kGroup: {
        uint32_t end_tag = field.field_number << 3 | kUpb_WireType_EndGroup;
        ret.append(ToBinaryPayload(*field.value.value.group));
        EncodeVarint(end_tag, &ret);
        break;
      }
    }
  }

  return ret;
}

upb_UnknownCompareResult CompareUnknownWithMaxDepth(UnknownFields uf1,
                                                    UnknownFields uf2,
                                                    int max_depth) {
  std::string buf1 = ToBinaryPayload(uf1);
  std::string buf2 = ToBinaryPayload(uf2);
  return upb_Message_UnknownFieldsAreEqual(buf1.data(), buf1.size(),
                                           buf2.data(), buf2.size(), max_depth);
}

upb_UnknownCompareResult CompareUnknown(UnknownFields uf1, UnknownFields uf2) {
  return CompareUnknownWithMaxDepth(uf1, uf2, 64);
}

TEST(CompareTest, UnknownFieldsReflexive) {
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal, CompareUnknown({}, {}));
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{1, Varint(123)}, {2, Fixed32(456)}},
                           {{1, Varint(123)}, {2, Fixed32(456)}}));
  EXPECT_EQ(
      kUpb_UnknownCompareResult_Equal,
      CompareUnknown(
          {{1, Group({{2, Group({{3, Fixed32(456)}, {4, Fixed64(123)}})}})}},
          {{1, Group({{2, Group({{3, Fixed32(456)}, {4, Fixed64(123)}})}})}}));
}

TEST(CompareTest, UnknownFieldsOrdering) {
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{1, Varint(111)},
                            {2, Delimited("ABC")},
                            {3, Fixed32(456)},
                            {4, Fixed64(123)},
                            {5, Group({})}},
                           {{5, Group({})},
                            {4, Fixed64(123)},
                            {3, Fixed32(456)},
                            {2, Delimited("ABC")},
                            {1, Varint(111)}}));
  EXPECT_EQ(kUpb_UnknownCompareResult_NotEqual,
            CompareUnknown({{1, Varint(111)},
                            {2, Delimited("ABC")},
                            {3, Fixed32(456)},
                            {4, Fixed64(123)},
                            {5, Group({})}},
                           {{5, Group({})},
                            {4, Fixed64(123)},
                            {3, Fixed32(455)},  // Small difference.
                            {2, Delimited("ABC")},
                            {1, Varint(111)}}));
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{3, Fixed32(456)}, {4, Fixed64(123)}},
                           {{4, Fixed64(123)}, {3, Fixed32(456)}}));
  EXPECT_EQ(
      kUpb_UnknownCompareResult_Equal,
      CompareUnknown(
          {{1, Group({{2, Group({{3, Fixed32(456)}, {4, Fixed64(123)}})}})}},
          {{1, Group({{2, Group({{4, Fixed64(123)}, {3, Fixed32(456)}})}})}}));
}

TEST(CompareTest, LongVarint) {
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{1, LongVarint(123)}, {2, LongVarint(456)}},
                           {{1, Varint(123)}, {2, Varint(456)}}));
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{2, LongVarint(456)}, {1, LongVarint(123)}},
                           {{1, Varint(123)}, {2, Varint(456)}}));
}

TEST(CompareTest, MaxDepth) {
  EXPECT_EQ(
      kUpb_UnknownCompareResult_MaxDepthExceeded,
      CompareUnknownWithMaxDepth(
          {{1, Group({{2, Group({{3, Fixed32(456)}, {4, Fixed64(123)}})}})}},
          {{1, Group({{2, Group({{4, Fixed64(123)}, {3, Fixed32(456)}})}})}},
          2));
}
