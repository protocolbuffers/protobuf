// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/compare_unknown.h"

#include <stdint.h>

#include <cstddef>
#include <initializer_list>
#include <string>

#include <gtest/gtest.h>
#include "absl/cleanup/cleanup.h"
#include "google/protobuf/test_messages_proto2.upb.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/message/compare.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/test.upb.h"
#include "upb/message/test.upb_minitable.h"
#include "upb/message/unknown_fields.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/encode.h"
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
  return _upb_Message_UnknownFieldsAreEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                                            max_depth);
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

TEST(CompareTest, MessageIsEqualWithIdenticalNonCanonicalExtensions) {
  upb::Arena arena;

  // 1. Create base msg1 and msg2 which start empty
  upb_test_ModelWithExtensions* msg1 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithExtensions* msg2 =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 2. Create sub-message value ("World")
  upb_test_ModelExtension1* extension1 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  upb_test_ModelExtension1* extension2 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(extension2,
                                   upb_StringView_FromString("World"));

  // 3. Attach identical non-canonical extensions to both messages
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg1), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena.ptr()));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg2), upb_test_ModelExtension1_model_ext_ext, &extension2,
      arena.ptr()));

  // 4. Verify upb_Message_IsEqual yields true
  bool is_equal = upb_Message_IsEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                                      &upb_0test__ModelWithExtensions_msg_init,
                                      kUpb_CompareOption_IncludeUnknownFields);
  EXPECT_TRUE(is_equal);
}

TEST(CompareTest, MessageIsEqualWithDifferentNonCanonicalExtensions) {
  upb::Arena arena;

  // 1. Create base msg1 and msg2 which start empty
  upb_test_ModelWithExtensions* msg1 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithExtensions* msg2 =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 2. Create custom sub-message 1 ("World")
  upb_test_ModelExtension1* extension1 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  // 3. Create custom sub-message 2 ("Hello")
  upb_test_ModelExtension1* extension2 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(extension2,
                                   upb_StringView_FromString("Hello"));

  // 4. Attach different non-canonical extensions to the two messages
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg1), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena.ptr()));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg2), upb_test_ModelExtension1_model_ext_ext, &extension2,
      arena.ptr()));

  // Verify that NextUnknown2 successfully yields the non-canonical extension
  upb_MessageUnknown udata;
  uintptr_t uiter = kUpb_Message_UnknownBegin;
  ASSERT_TRUE(upb_Message_NextUnknown2(UPB_UPCAST(msg1), &udata, &uiter));
  EXPECT_EQ(udata.type, kUpb_MessageUnknownType_NonCanonicalExtension);

  // 5. Verify upb_Message_IsEqual yields false because they are different!
  bool is_equal = upb_Message_IsEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                                      &upb_0test__ModelWithExtensions_msg_init,
                                      kUpb_CompareOption_IncludeUnknownFields);
  EXPECT_FALSE(is_equal);
}

TEST(CompareTest, MessageIsEqualWithOnlyOneHavingNonCanonicalExtension) {
  upb::Arena arena;

  // 1. Create base msg1 and msg2
  upb_test_ModelWithExtensions* msg1 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelWithExtensions* msg2 =
      upb_test_ModelWithExtensions_new(arena.ptr());

  // 2. Create parsed submessage ("World")
  upb_test_ModelExtension1* extension1 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  // 3. Attach only to msg1
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg1), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena.ptr()));

  // 4. Verify upb_Message_IsEqual yields false with IncludeUnknownFields
  bool is_equal_with_unknowns =
      upb_Message_IsEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                          &upb_0test__ModelWithExtensions_msg_init,
                          kUpb_CompareOption_IncludeUnknownFields);
  EXPECT_FALSE(is_equal_with_unknowns);

  // 5. Without IncludeUnknownFields, they compare equal
  bool is_equal_without_unknowns =
      upb_Message_IsEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                          &upb_0test__ModelWithExtensions_msg_init, 0);
  EXPECT_TRUE(is_equal_without_unknowns);
}

TEST(CompareTest, MessageIsEqualWithNonCanonicalExtensionMatchingRawUnknown) {
  upb::Arena arena;

  // 1. Create custom sub-message ("World")
  upb_test_ModelExtension1* extension1 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(extension1,
                                   upb_StringView_FromString("World"));

  // 2. Obtain encoded raw wire bytes of Extension A by serializing tmp_msg
  upb_test_ModelWithExtensions* tmp_msg =
      upb_test_ModelWithExtensions_new(arena.ptr());
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(tmp_msg), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena.ptr()));

  char* buf;
  size_t size;
  upb_EncodeStatus enc_status =
      upb_Encode(UPB_UPCAST(tmp_msg), &upb_0test__ModelWithExtensions_msg_init,
                 0, arena.ptr(), &buf, &size);
  ASSERT_EQ(enc_status, kUpb_EncodeStatus_Ok);
  ASSERT_GT(size, 0u);

  // 3. Create msg1 with non-canonical extension A
  upb_test_ModelWithExtensions* msg1 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg1), upb_test_ModelExtension1_model_ext_ext, &extension1,
      arena.ptr()));

  // 4. Create msg2 with raw unknown bytes representing A
  upb_test_ModelWithExtensions* msg2 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg2), buf, size, arena.ptr(), kUpb_AddUnknown_Copy));

  // 5. Verify they compare equal under IncludeUnknownFields
  bool is_equal = upb_Message_IsEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                                      &upb_0test__ModelWithExtensions_msg_init,
                                      kUpb_CompareOption_IncludeUnknownFields);
  EXPECT_TRUE(is_equal);
}

TEST(CompareTest, MessageSetNonCanonicalExtMatchesUnknown) {
  upb::Arena arena;

  // 1. Create custom sub-message (MessageSetMember)
  upb_test_MessageSetMember* extension1 =
      upb_test_MessageSetMember_new(arena.ptr());
  upb_test_MessageSetMember_set_optional_int32(extension1, 123);

  // 2. Obtain encoded raw wire bytes of Extension A by serializing tmp_msg
  upb_test_TestMessageSet* tmp_msg = upb_test_TestMessageSet_new(arena.ptr());
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(tmp_msg), upb_test_MessageSetMember_message_set_extension_ext,
      &extension1, arena.ptr()));

  char* buf;
  size_t size;
  upb_EncodeStatus enc_status =
      upb_Encode(UPB_UPCAST(tmp_msg), &upb_0test__TestMessageSet_msg_init, 0,
                 arena.ptr(), &buf, &size);
  ASSERT_EQ(enc_status, kUpb_EncodeStatus_Ok);
  ASSERT_GT(size, 0u);

  // 3. Create msg1 with non-canonical extension A
  upb_test_TestMessageSet* msg1 = upb_test_TestMessageSet_new(arena.ptr());
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg1), upb_test_MessageSetMember_message_set_extension_ext,
      &extension1, arena.ptr()));

  // 4. Create msg2 with raw unknown bytes representing A
  upb_test_TestMessageSet* msg2 = upb_test_TestMessageSet_new(arena.ptr());
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_AddUnknown)(
      UPB_UPCAST(msg2), buf, size, arena.ptr(), kUpb_AddUnknown_Copy));

  // 5. Verify they compare equal under IncludeUnknownFields
  bool is_equal = upb_Message_IsEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                                      &upb_0test__TestMessageSet_msg_init,
                                      kUpb_CompareOption_IncludeUnknownFields);
  EXPECT_TRUE(is_equal);
}

TEST(CompareTest, MessageIsEqualWithCanonicalAndNonCanonicalExtensions) {
  upb::Arena arena;

  // 1. Create msg1 and set canonical extension
  upb_test_ModelWithExtensions* msg1 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelExtension1* ext_msg1 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(ext_msg1,
                                   upb_StringView_FromString("World"));
  upb_test_ModelExtension1_set_model_ext(msg1, ext_msg1, arena.ptr());

  // 2. Create msg2 and set identical non-canonical extension
  upb_test_ModelWithExtensions* msg2 =
      upb_test_ModelWithExtensions_new(arena.ptr());
  upb_test_ModelExtension1* ext_msg2 =
      upb_test_ModelExtension1_new(arena.ptr());
  upb_test_ModelExtension1_set_str(ext_msg2,
                                   upb_StringView_FromString("World"));
  EXPECT_TRUE(UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(
      UPB_UPCAST(msg2), upb_test_ModelExtension1_model_ext_ext, &ext_msg2,
      arena.ptr()));

  // 3. Verify upb_Message_IsEqual registers them as NOT equal
  bool is_equal = upb_Message_IsEqual(UPB_UPCAST(msg1), UPB_UPCAST(msg2),
                                      &upb_0test__ModelWithExtensions_msg_init,
                                      kUpb_CompareOption_IncludeUnknownFields);
  EXPECT_FALSE(is_equal);
}

}  // namespace

TEST(CompareTest, AllocationFailure) {
  if (!upb_AllocationCount_IsAvailable()) return;

  auto RunScenario = [&]() -> bool {
    upb_Arena* arena1 = upb_Arena_New();
    if (!arena1) return false;
    auto cleanup1 = absl::MakeCleanup([arena1] { upb_Arena_Free(arena1); });

    upb_Arena* arena2 = upb_Arena_New();
    if (!arena2) return false;
    auto cleanup2 = absl::MakeCleanup([arena2] { upb_Arena_Free(arena2); });

    protobuf_test_messages_proto2_TestAllTypesProto2* msg1 =
        protobuf_test_messages_proto2_TestAllTypesProto2_new(arena1);
    if (!msg1) return false;
    protobuf_test_messages_proto2_TestAllTypesProto2* msg2 =
        protobuf_test_messages_proto2_TestAllTypesProto2_new(arena2);
    if (!msg2) return false;

    WireMessage uf1 = {{2, Fixed32(456)}, {1, Fixed64(123)}};
    WireMessage uf2 = {{2, Fixed32(456)}, {1, Fixed64(123)}};
    std::string buf1 = ToBinaryPayloadWithLongVarints(uf1, 1, 1);
    std::string buf2 = ToBinaryPayloadWithLongVarints(uf2, 1, 1);

    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg1), buf1.data(),
                                              buf1.size(), arena1,
                                              kUpb_AddUnknown_Copy)) {
      return false;
    }
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(UPB_UPCAST(msg2), buf2.data(),
                                              buf2.size(), arena2,
                                              kUpb_AddUnknown_Copy)) {
      return false;
    }
    upb_UnknownCompareResult res = _upb_Message_UnknownFieldsAreEqual(
        UPB_UPCAST(msg1), UPB_UPCAST(msg2), 64);
    return res == kUpb_UnknownCompareResult_Equal;
  };

  upb_AllocationCount_Reset();
  if (RunScenario()) {
    size_t total_allocations = upb_AllocationCount_Get();
    for (size_t i = 0; i < total_allocations; ++i) {
      upb_AllocationCount_Reset();
      upb_AllocationCount_FailOn(i);
      bool success_with_fail = RunScenario();
      EXPECT_FALSE(success_with_fail)
          << "Unknown comparison unexpectedly succeeded when allocation "
          << "number " << i << " was failed.";
    }
  }
  upb_AllocationCount_Reset();
}

}  // namespace test
}  // namespace upb
