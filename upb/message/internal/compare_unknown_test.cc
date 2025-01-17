// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/compare_unknown.h"

#include <stdint.h>

#include <initializer_list>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/types/variant.h"
#include "google/protobuf/test_messages_proto2.upb.h"
#include "upb/base/internal/endian.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

struct UnknownField;

using UnknownFields = std::vector<UnknownField>;

struct Varint {
  explicit Varint(uint64_t _val) : val(_val) {}
  uint64_t val;
};
struct LongVarint {
  explicit LongVarint(uint64_t _val) : val(_val) {}
  uint64_t val;  // Over-encoded.
};
struct Delimited {
  explicit Delimited(std::string _val) : val(_val) {}
  std::string val;
};
struct Fixed64 {
  explicit Fixed64(uint64_t _val) : val(_val) {}
  uint64_t val;
};
struct Fixed32 {
  explicit Fixed32(uint32_t _val) : val(_val) {}
  uint32_t val;
};
struct Group {
  Group(std::initializer_list<UnknownField> _val);
  UnknownFields val;
};

struct UnknownField {
  uint32_t field_number;
  absl::variant<Varint, LongVarint, Delimited, Fixed64, Fixed32, Group> value;
};

Group::Group(std::initializer_list<UnknownField> _val) : val(_val) {}

void EncodeVarint(uint64_t val, std::string* str) {
  do {
    char byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    str->push_back(byte);
  } while (val);
}

std::string ToBinaryPayload(const UnknownFields& fields) {
  std::string ret;

  for (const auto& field : fields) {
    if (const auto* val = absl::get_if<Varint>(&field.value)) {
      EncodeVarint(field.field_number << 3 | kUpb_WireType_Varint, &ret);
      EncodeVarint(val->val, &ret);
    } else if (const auto* val = absl::get_if<LongVarint>(&field.value)) {
      EncodeVarint(field.field_number << 3 | kUpb_WireType_Varint, &ret);
      EncodeVarint(val->val, &ret);
      ret.back() |= 0x80;
      ret.push_back(0);
    } else if (const auto* val = absl::get_if<Delimited>(&field.value)) {
      EncodeVarint(field.field_number << 3 | kUpb_WireType_Delimited, &ret);
      EncodeVarint(val->val.size(), &ret);
      ret.append(val->val);
    } else if (const auto* val = absl::get_if<Fixed64>(&field.value)) {
      EncodeVarint(field.field_number << 3 | kUpb_WireType_64Bit, &ret);
      uint64_t swapped = upb_BigEndian64(val->val);
      ret.append(reinterpret_cast<const char*>(&swapped), sizeof(swapped));
    } else if (const auto* val = absl::get_if<Fixed32>(&field.value)) {
      EncodeVarint(field.field_number << 3 | kUpb_WireType_32Bit, &ret);
      uint32_t swapped = upb_BigEndian32(val->val);
      ret.append(reinterpret_cast<const char*>(&swapped), sizeof(swapped));
    } else if (const auto* val = absl::get_if<Group>(&field.value)) {
      EncodeVarint(field.field_number << 3 | kUpb_WireType_StartGroup, &ret);
      ret.append(ToBinaryPayload(val->val));
      EncodeVarint(field.field_number << 3 | kUpb_WireType_EndGroup, &ret);
    }
  }

  return ret;
}

upb_UnknownCompareResult CompareUnknownWithMaxDepth(UnknownFields uf1,
                                                    UnknownFields uf2,
                                                    int max_depth) {
  upb::Arena arena1;
  upb::Arena arena2;
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena1.ptr());
  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena2.ptr());
  // Add the unknown fields to the messages.
  std::string buf1 = ToBinaryPayload(uf1);
  std::string buf2 = ToBinaryPayload(uf2);
  UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg1), buf1.data(),
                                       buf1.size(), arena1.ptr(), false);
  UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg2), buf2.data(),
                                       buf2.size(), arena2.ptr(), false);
  return UPB_PRIVATE(_upb_Message_UnknownFieldsAreEqual)(
      UPB_UPCAST(msg1), UPB_UPCAST(msg2), max_depth);
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
