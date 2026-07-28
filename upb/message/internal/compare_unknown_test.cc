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

#include <gtest/gtest.h>
#include "google/protobuf/test_messages_proto2.upb.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.hpp"
#include "upb/message/internal/message.h"
#include "upb/wire/test_util/wire_message.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace test {

namespace {

using ::upb::test::wire_types::Delimited;
using ::upb::test::wire_types::Fixed32;
using ::upb::test::wire_types::Fixed64;
using ::upb::test::wire_types::Group;
using ::upb::test::wire_types::Varint;
using ::upb::test::wire_types::WireMessage;

upb_UnknownCompareResult CompareUnknownWithMaxDepth(
    WireMessage uf1, WireMessage uf2, int max_depth, int min_tag_length = 1,
    int min_val_varint_length = 1) {
  upb::Arena arena1;
  upb::Arena arena2;
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena1.ptr());
  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena2.ptr());
  // Add the unknown fields to the messages.
  std::string buf1 = ToBinaryPayloadWithLongVarints(uf1, min_tag_length,
                                                    min_val_varint_length);
  std::string buf2 = ToBinaryPayloadWithLongVarints(uf2, min_tag_length,
                                                    min_val_varint_length);
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg1), buf1.data(), buf1.size(), arena1.ptr(),
      kUpb_AddUnknown_Copy));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg2), buf2.data(), buf2.size(), arena2.ptr(),
      kUpb_AddUnknown_Copy));
  return UPB_PRIVATE(_upb_Message_UnknownFieldsAreEqual)(
      UPB_UPCAST(msg1), UPB_UPCAST(msg2), max_depth);
}

upb_UnknownCompareResult CompareUnknown(WireMessage uf1, WireMessage uf2) {
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
            CompareUnknownWithMaxDepth({{1, Varint(123)}, {2, Varint(456)}},
                                       {{1, Varint(123)}, {2, Varint(456)}}, 64,
                                       5, 10));
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknownWithMaxDepth({{2, Varint(456)}, {1, Varint(123)}},
                                       {{1, Varint(123)}, {2, Varint(456)}}, 64,
                                       5, 10));
}

TEST(CompareTest, MaxDepth) {
  EXPECT_EQ(
      kUpb_UnknownCompareResult_MaxDepthExceeded,
      CompareUnknownWithMaxDepth(
          {{1, Group({{2, Group({{3, Fixed32(456)}, {4, Fixed64(123)}})}})}},
          {{1, Group({{2, Group({{4, Fixed64(123)}, {3, Fixed32(456)}})}})}},
          1));
}

// Test with enough fields to trigger reallocation in the merge sort tmp buffer
// (initial capacity is 8). Also exercises the right-half remainder path in
// upb_UnknownFields_Merge where ptr2 < end2.
TEST(CompareTest, SortReallocation) {
  // 20 fields in descending order forces merge sort to reallocate beyond the
  // initial tmp buffer of 8, and the reversed ordering ensures the right-half
  // remainder branch is exercised during merges.
  WireMessage descending = {
      {20, Varint(20)}, {19, Varint(19)}, {18, Varint(18)},
      {17, Varint(17)}, {16, Varint(16)}, {15, Varint(15)},
      {14, Varint(14)}, {13, Varint(13)}, {12, Varint(12)},
      {11, Varint(11)}, {10, Varint(10)}, {9, Varint(9)},
      {8, Varint(8)},   {7, Varint(7)},   {6, Varint(6)},
      {5, Varint(5)},   {4, Varint(4)},   {3, Varint(3)},
      {2, Varint(2)},   {1, Varint(1)},
  };
  WireMessage ascending = {
      {1, Varint(1)},   {2, Varint(2)},   {3, Varint(3)},
      {4, Varint(4)},   {5, Varint(5)},   {6, Varint(6)},
      {7, Varint(7)},   {8, Varint(8)},   {9, Varint(9)},
      {10, Varint(10)}, {11, Varint(11)}, {12, Varint(12)},
      {13, Varint(13)}, {14, Varint(14)}, {15, Varint(15)},
      {16, Varint(16)}, {17, Varint(17)}, {18, Varint(18)},
      {19, Varint(19)}, {20, Varint(20)},
  };
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown(descending, ascending));

  // Verify inequality still detected after sort.
  WireMessage ascending_modified = ascending;
  ascending_modified.back() = {20, Varint(999)};
  EXPECT_EQ(kUpb_UnknownCompareResult_NotEqual,
            CompareUnknown(descending, ascending_modified));
}

}  // namespace

}  // namespace test
}  // namespace upb
