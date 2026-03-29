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
#include "upb/mem/alloc.h"
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

// --- Out-of-memory injection for the merge-sort scratch buffer --------------
//
// upb_UnknownFields_Sort() in compare_unknown.c allocates its merge-sort
// scratch buffer with upb_grealloc(), which routes through the process-global
// allocator upb_alloc_global (unlike everything else in that file, which uses
// the comparison's arena). We temporarily swap that global allocator out so we
// can both count allocations and force a chosen one to return NULL, exercising
// the `if (!ctx->tmp) upb_UnknownFields_OutOfMemory(ctx);` check that follows
// the upb_grealloc() call.
upb_alloc_func* g_real_allocfunc = nullptr;
int g_alloc_calls = 0;    // count of size>0 (allocating) calls seen
int g_fail_on_call = -1;  // 1-based ordinal to fail; -1 disables failing

void* CountingFailingAllocFunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                               size_t size, size_t* actual_size) {
  if (size != 0) {
    ++g_alloc_calls;
    if (g_alloc_calls == g_fail_on_call) return nullptr;  // inject OOM
  }
  return g_real_allocfunc(alloc, ptr, oldsize, size, actual_size);
}

class ScopedGlobalAllocOverride {
 public:
  ScopedGlobalAllocOverride() {
    g_real_allocfunc = upb_alloc_global.func;
    g_alloc_calls = 0;
    g_fail_on_call = -1;
    upb_alloc_global.func = CountingFailingAllocFunc;
  }
  ~ScopedGlobalAllocOverride() { upb_alloc_global.func = g_real_allocfunc; }
};

// When upb_grealloc() fails to allocate the merge-sort scratch buffer, the
// comparison must report OutOfMemory rather than dereferencing a NULL pointer.
// `sorted` needs no merge sort, while `unsorted` (the same fields in reverse
// tag order) forces upb_UnknownFields_Sort() -> upb_grealloc(). Because
// `unsorted` is built after `sorted`, that grealloc() is the LAST allocation of
// the comparison, so we can target it by failing the last allocation without
// hardcoding any allocator-internal call counts.
TEST(CompareTest, TmpBufferAllocationFailureIsOutOfMemory) {
  const WireMessage sorted = {{1, Varint(100)}, {2, Varint(200)}};
  const WireMessage unsorted = {{2, Varint(200)}, {1, Varint(100)}};

  // Pass 1: count the global allocations a successful comparison performs.
  int total_allocs;
  {
    ScopedGlobalAllocOverride guard;
    EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
              CompareUnknown(sorted, unsorted));
    total_allocs = g_alloc_calls;
  }
  ASSERT_GT(total_allocs, 0);

  // Pass 2: fail the scratch-buffer allocation (the last one). Without the NULL
  // check after upb_grealloc() this dereferences NULL and crashes.
  {
    ScopedGlobalAllocOverride guard;
    g_fail_on_call = total_allocs;
    EXPECT_EQ(kUpb_UnknownCompareResult_OutOfMemory,
              CompareUnknown(sorted, unsorted));
  }
}

// Regression test for the integer overflow in upb_UnknownFields_Sort()'s
// scratch-buffer size computation. Reaching it requires more than
// INT_MAX / sizeof(upb_UnknownField) == 89,478,485 unknown fields, so that
// `tmp_size * sizeof(upb_UnknownField)` exceeds INT_MAX. With the old
// `const int` the product truncated to a negative value that sign-extended to a
// ~18 EB size_t when passed to upb_grealloc(), which then returned NULL (and,
// without the NULL check above, crashed). With `const size_t` the size is
// computed correctly (~3 GiB) and the allocation succeeds, so a comparison of
// two such messages reports Equal.
//
// This allocates several GiB and is therefore DISABLED by default; run with
// --gtest_also_run_disabled_tests on a machine with enough memory.
TEST(CompareTest, DISABLED_TmpBufferSizeDoesNotOverflowInt) {
  constexpr size_t kNumFields = 90000000;  // > INT_MAX / sizeof(upb_UnknownField)

  // An unsorted unknown-field payload (forcing the merge sort): one field with
  // tag 2 followed by kNumFields fields with tag 1. The first tag-1 field is
  // smaller than the preceding tag-2 field, which marks the set as unsorted.
  std::string payload;
  payload.reserve((kNumFields + 1) * 2);
  payload.push_back(0x10);  // tag 2, wire type 0 (varint)
  payload.push_back(0x00);  // value 0
  for (size_t i = 0; i < kNumFields; ++i) {
    payload.push_back(0x08);  // tag 1, wire type 0 (varint)
    payload.push_back(0x00);  // value 0
  }

  upb::Arena arena1;
  upb::Arena arena2;
  protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena1.ptr());
  protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
      protobuf_test_messages_proto2_TestAllTypesProto2_new(arena2.ptr());
  UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg1), payload.data(),
                                       payload.size(), arena1.ptr(),
                                       kUpb_AddUnknown_Copy);
  UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg2), payload.data(),
                                       payload.size(), arena2.ptr(),
                                       kUpb_AddUnknown_Copy);

  EXPECT_EQ(kUpb_UnknownCompareResult_Equal,
            UPB_PRIVATE(_upb_Message_UnknownFieldsAreEqual)(
                UPB_UPCAST(msg1), UPB_UPCAST(msg2), 64));
}

}  // namespace

}  // namespace test
}  // namespace upb
