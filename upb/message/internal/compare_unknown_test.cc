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
  UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg1), buf1.data(),
                                       buf1.size(), arena1.ptr(),
                                       kUpb_AddUnknown_Copy);
  UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg2), buf2.data(),
                                       buf2.size(), arena2.ptr(),
                                       kUpb_AddUnknown_Copy);
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

// This test triggers the merge sort bug in compare_unknown.c line 105.
// The bug: in upb_UnknownFields_Merge(), the "else if (ptr2 < end2)" branch
// copies from ptr1 (already exhausted) instead of ptr2 (the actual remainder).
// This corrupts the sorted array, causing equal messages to compare as NotEqual.
//
// The key is to create field orderings where the RIGHT half of a merge has
// remaining elements after the left half is exhausted. For example:
//   Input [2, 1, 3] splits into [2] and [1, 3]
//   After recursive sort: [2] and [1, 3]
//   Merge: take 1 (from right), take 2 (from left) -> left exhausted
//   Remainder: ptr2 points to [3] but the bug copies from ptr1 (stale data)
TEST(CompareTest, MergeSortBug_RightHalfRemainder) {
  // 3 elements: [2, 1, 3] vs [1, 2, 3] - simplest trigger
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{2, Varint(200)}, {1, Varint(100)}, {3, Varint(300)}},
                           {{1, Varint(100)}, {2, Varint(200)}, {3, Varint(300)}}));

  // 5 elements: [2, 4, 1, 3, 5] vs [1, 2, 3, 4, 5]
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{2, Varint(200)},
                            {4, Varint(400)},
                            {1, Varint(100)},
                            {3, Varint(300)},
                            {5, Varint(500)}},
                           {{1, Varint(100)},
                            {2, Varint(200)},
                            {3, Varint(300)},
                            {4, Varint(400)},
                            {5, Varint(500)}}));

  // 7 elements: complex shuffle that triggers multiple buggy merges
  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            CompareUnknown({{4, Varint(400)},
                            {6, Varint(600)},
                            {2, Varint(200)},
                            {7, Varint(700)},
                            {1, Varint(100)},
                            {3, Varint(300)},
                            {5, Varint(500)}},
                           {{1, Varint(100)},
                            {2, Varint(200)},
                            {3, Varint(300)},
                            {4, Varint(400)},
                            {5, Varint(500)},
                            {6, Varint(600)},
                            {7, Varint(700)}}));
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

}  // namespace

}  // namespace test
}  // namespace upb
