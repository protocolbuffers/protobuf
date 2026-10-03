// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Tests for lazily parsed extensions: decoding stores the serialized payload,
// which is parsed on demand by upb_Message_PromoteLazyExtension().

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include "absl/base/thread_annotations.h"
#include "absl/random/distributions.h"
#include "absl/random/random.h"
#include "absl/synchronization/mutex.h"
#include "absl/synchronization/notification.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "google/protobuf/test_messages_proto2.upb.h"
#include "google/protobuf/test_messages_proto2.upb_minitable.h"
#include "upb/base/string_view.h"
#include "upb/base/upcast.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/compare.h"
#include "upb/message/copy.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/message/promote.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/test/test.upb.h"
#include "upb/test/test.upb_minitable.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

const upb_MiniTable* kModelMiniTable = &upb_0test__ModelWithExtensions_msg_init;

std::string ToString(upb_StringView sv) {
  return std::string(sv.data, sv.size);
}

// Serializes a ModelWithExtensions{random_int32: 42, [model_ext] {str: str}}
// using the (eager) generated code.
std::string SerializeModelWithExt1(const char* str) {
  upb_Arena* arena = upb_Arena_New();
  upb_test_ModelWithExtensions* msg = upb_test_ModelWithExtensions_new(arena);
  upb_test_ModelWithExtensions_set_random_int32(msg, 42);
  upb_test_ModelExtension1* ext = upb_test_ModelExtension1_new(arena);
  upb_test_ModelExtension1_set_str(ext, upb_StringView_FromString(str));
  upb_test_ModelExtension1_set_model_ext(msg, ext, arena);
  size_t size;
  char* buf = upb_test_ModelWithExtensions_serialize(msg, arena, &size);
  std::string ret(buf, size);
  upb_Arena_Free(arena);
  return ret;
}

class LazyExtensionTest : public ::testing::Test {
 protected:
  LazyExtensionTest() {
    // The registry (and the extension minitables it refers to) must outlive
    // any message parsed with it.
    registry_arena_ = upb_Arena_New();
    lazy_ext_ = *upb_test_ModelExtension1_model_ext_ext;
    EXPECT_TRUE(upb_MiniTableExtension_SetLazy(&lazy_ext_, true));
    EXPECT_TRUE(upb_MiniTableExtension_IsLazy(&lazy_ext_));
    registry_ = upb_ExtensionRegistry_New(registry_arena_);
    EXPECT_EQ(upb_ExtensionRegistry_Add(registry_, &lazy_ext_),
              kUpb_ExtensionRegistryStatus_Ok);
    arena_ = upb_Arena_New();
  }

  ~LazyExtensionTest() override {
    upb_Arena_Free(arena_);
    upb_Arena_Free(registry_arena_);
  }

  // Parses `wire` into a fresh message in `arena` using the lazy registry.
  upb_Message* Parse(const std::string& wire, int options = 0,
                     upb_Arena* arena = nullptr) {
    if (!arena) arena = arena_;
    upb_Message* msg = upb_Message_New(kModelMiniTable, arena);
    EXPECT_EQ(upb_Decode(wire.data(), wire.size(), msg, kModelMiniTable,
                         registry_, options, arena),
              kUpb_DecodeStatus_Ok);
    return msg;
  }

  // Returns the aux_data entry for the lazy extension, or a null tagged
  // pointer if there is none.
  upb_TaggedAuxPtr Entry(const upb_Message* msg) {
    upb_TaggedAuxPtr ptr = upb_TaggedAuxPtr_Null();
    UPB_PRIVATE(_upb_Message_FindExtensionEntry)(msg, &lazy_ext_, nullptr,
                                                 &ptr);
    return ptr;
  }

  std::string Encode(const upb_Message* msg, int options = 0) {
    upb_Arena* arena = upb_Arena_New();
    char* buf;
    size_t size;
    EXPECT_EQ(upb_Encode(msg, kModelMiniTable, options, arena, &buf, &size),
              kUpb_EncodeStatus_Ok);
    std::string ret(buf, size);
    upb_Arena_Free(arena);
    return ret;
  }

  upb_Arena* registry_arena_;
  upb_Arena* arena_;
  upb_MiniTableExtension lazy_ext_;
  upb_ExtensionRegistry* registry_;
};

TEST_F(LazyExtensionTest, SetLazyRejectsNonMessageExtensions) {
  upb_MiniTableExtension scalar =
      *protobuf_test_messages_proto2_extension_int32_ext;
  EXPECT_FALSE(upb_MiniTableExtension_SetLazy(&scalar, true));
  EXPECT_FALSE(upb_MiniTableExtension_IsLazy(&scalar));
  // SetLazy(false) is always allowed on a message extension.
  upb_MiniTableExtension ext = *upb_test_ModelExtension1_model_ext_ext;
  EXPECT_TRUE(upb_MiniTableExtension_SetLazy(&ext, true));
  EXPECT_TRUE(upb_MiniTableExtension_SetLazy(&ext, false));
  EXPECT_FALSE(upb_MiniTableExtension_IsLazy(&ext));
}

TEST_F(LazyExtensionTest, DecodeStoresPayloadWithoutParsing) {
  const std::string wire = SerializeModelWithExt1("hello");
  upb_Message* msg = Parse(wire);

  // The extension is present but has no parsed value yet.
  EXPECT_TRUE(upb_Message_HasExtension(msg, &lazy_ext_));
  EXPECT_EQ(upb_Message_GetExtensionMessage(msg, &lazy_ext_, nullptr), nullptr);
  EXPECT_EQ(upb_Message_ExtensionCount(msg), 1);
  EXPECT_FALSE(upb_Message_HasUnknown(msg));

  // Iteration skips unpromoted lazy extensions.
  const upb_MiniTableExtension* e;
  upb_MessageValue v;
  uintptr_t iter = kUpb_Message_ExtensionBegin;
  EXPECT_FALSE(upb_Message_NextExtension(msg, &e, &v, &iter));

  // The payload was copied (no aliasing requested).
  upb_TaggedAuxPtr entry = Entry(msg);
  ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
  EXPECT_FALSE(upb_TaggedAuxPtr_IsLazyExtensionAliased(entry));
  const upb_LazyExtensionData* lazy = upb_TaggedAuxPtr_LazyExtension(entry);
  EXPECT_EQ(lazy->ext, &lazy_ext_);
  EXPECT_EQ(lazy->registry, registry_);
  EXPECT_TRUE(lazy->data.data < wire.data() ||
              lazy->data.data >= wire.data() + wire.size());
  EXPECT_EQ(lazy->data.data, reinterpret_cast<const char*>(lazy + 1));
}

TEST_F(LazyExtensionTest, DecodeAliasesPayloadWhenRequested) {
  const std::string wire = SerializeModelWithExt1("hello");
  upb_Message* msg = Parse(wire, kUpb_DecodeOption_AliasString);

  upb_TaggedAuxPtr entry = Entry(msg);
  ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
  EXPECT_TRUE(upb_TaggedAuxPtr_IsLazyExtensionAliased(entry));
  const upb_LazyExtensionData* lazy = upb_TaggedAuxPtr_LazyExtension(entry);
  EXPECT_GE(lazy->data.data, wire.data());
  EXPECT_LE(lazy->data.data + lazy->data.size, wire.data() + wire.size());

  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  EXPECT_EQ(
      ToString(upb_test_ModelExtension1_str(
          reinterpret_cast<const upb_test_ModelExtension1*>(val.msg_val))),
      "hello");
}

TEST_F(LazyExtensionTest, PromoteParsesAndPublishes) {
  upb_Message* msg = Parse(SerializeModelWithExt1("hello"));

  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  ASSERT_NE(val.msg_val, nullptr);
  const auto* ext1 =
      reinterpret_cast<const upb_test_ModelExtension1*>(val.msg_val);
  EXPECT_EQ(ToString(upb_test_ModelExtension1_str(ext1)), "hello");

  // The entry now carries the promoted tag and behaves like any other
  // canonical extension.
  upb_TaggedAuxPtr entry = Entry(msg);
  EXPECT_TRUE(upb_TaggedAuxPtr_IsPromotedExtension(entry));
  EXPECT_TRUE(upb_TaggedAuxPtr_IsCanonicalExtension(entry));
  EXPECT_EQ(upb_Message_GetExtensionMessage(msg, &lazy_ext_, nullptr),
            val.msg_val);
  EXPECT_EQ(upb_Message_ExtensionCount(msg), 1);

  const upb_MiniTableExtension* e;
  upb_MessageValue v;
  uintptr_t iter = kUpb_Message_ExtensionBegin;
  ASSERT_TRUE(upb_Message_NextExtension(msg, &e, &v, &iter));
  EXPECT_EQ(e, &lazy_ext_);
  EXPECT_EQ(v.msg_val, val.msg_val);
  EXPECT_FALSE(upb_Message_NextExtension(msg, &e, &v, &iter));

  // Promoting again is idempotent.
  upb_MessageValue val2;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val2),
            kUpb_GetExtension_Ok);
  EXPECT_EQ(val2.msg_val, val.msg_val);

  // upb_Message_GetOrPromoteExtension() also understands lazy entries.
  upb_Message* msg2 = Parse(SerializeModelWithExt1("world"));
  upb_MessageValue val3;
  ASSERT_EQ(
      upb_Message_GetOrPromoteExtension(msg2, &lazy_ext_, 0, arena_, &val3),
      kUpb_GetExtension_Ok);
  EXPECT_EQ(
      ToString(upb_test_ModelExtension1_str(
          reinterpret_cast<const upb_test_ModelExtension1*>(val3.msg_val))),
      "world");
}

TEST_F(LazyExtensionTest, PromoteNotPresent) {
  upb_Message* msg = upb_Message_New(kModelMiniTable, arena_);
  upb_MessageValue val;
  EXPECT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_NotPresent);
}

TEST_F(LazyExtensionTest, PromoteEagerlyParsedExtension) {
  // An extension that was set directly (not lazily) is returned as-is.
  upb_Message* msg = upb_Message_New(kModelMiniTable, arena_);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(arena_);
  ASSERT_TRUE(upb_Message_SetExtensionMessage(msg, &lazy_ext_, UPB_UPCAST(ext1),
                                              arena_));
  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  EXPECT_EQ(val.msg_val, UPB_UPCAST(ext1));
  EXPECT_FALSE(upb_TaggedAuxPtr_IsPromotedExtension(Entry(msg)));
}

TEST_F(LazyExtensionTest, RepeatedOccurrencesAreCoalescedAndMerged) {
  // Two occurrences of the same singular message extension merge on parse;
  // "world" is set last and therefore wins.
  const std::string wire =
      SerializeModelWithExt1("hello") + SerializeModelWithExt1("world");
  for (int options : {0, static_cast<int>(kUpb_DecodeOption_AliasString)}) {
    upb_Message* msg = Parse(wire, options);
    EXPECT_EQ(upb_Message_ExtensionCount(msg), 1);

    upb_TaggedAuxPtr entry = Entry(msg);
    ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
    // Coalesced payloads are always copied into one contiguous block.
    EXPECT_FALSE(upb_TaggedAuxPtr_IsLazyExtensionAliased(entry));
    const upb_LazyExtensionData* lazy = upb_TaggedAuxPtr_LazyExtension(entry);
    EXPECT_EQ(lazy->data.data, reinterpret_cast<const char*>(lazy + 1));

    upb_MessageValue val;
    ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
              kUpb_GetExtension_Ok);
    EXPECT_EQ(
        ToString(upb_test_ModelExtension1_str(
            reinterpret_cast<const upb_test_ModelExtension1*>(val.msg_val))),
        "world");
  }
}

TEST_F(LazyExtensionTest, ThreeOccurrencesGrowTheBlock) {
  const std::string wire = SerializeModelWithExt1("a") +
                           SerializeModelWithExt1("bb") +
                           SerializeModelWithExt1("ccc");
  upb_Message* msg = Parse(wire);
  upb_TaggedAuxPtr entry = Entry(msg);
  ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
  const upb_LazyExtensionData* lazy = upb_TaggedAuxPtr_LazyExtension(entry);
  // Each occurrence contributes a `str` field of [2 byte tag][len][str].
  EXPECT_EQ(lazy->data.size, (3 + 1) + (3 + 2) + (3 + 3));

  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  EXPECT_EQ(
      ToString(upb_test_ModelExtension1_str(
          reinterpret_cast<const upb_test_ModelExtension1*>(val.msg_val))),
      "ccc");
}

TEST_F(LazyExtensionTest, LazyPayloadMergesIntoExistingParsedValue) {
  // If the message already holds a parsed value for the extension, the decoder
  // falls back to an eager parse so that the two can be merged.
  upb_Message* msg = upb_Message_New(kModelMiniTable, arena_);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(arena_);
  ASSERT_TRUE(upb_Message_SetExtensionMessage(msg, &lazy_ext_, UPB_UPCAST(ext1),
                                              arena_));
  const std::string wire = SerializeModelWithExt1("merged");
  ASSERT_EQ(upb_Decode(wire.data(), wire.size(), msg, kModelMiniTable,
                       registry_, 0, arena_),
            kUpb_DecodeStatus_Ok);
  EXPECT_FALSE(upb_TaggedAuxPtr_IsLazyExtension(Entry(msg)));
  EXPECT_EQ(upb_Message_GetExtensionMessage(msg, &lazy_ext_, nullptr),
            UPB_UPCAST(ext1));
  EXPECT_EQ(ToString(upb_test_ModelExtension1_str(ext1)), "merged");
  EXPECT_EQ(upb_Message_ExtensionCount(msg), 1);
}

TEST_F(LazyExtensionTest, EncodePreservesLazyPayload) {
  const std::string wire = SerializeModelWithExt1("hello");
  upb_Message* msg = Parse(wire);

  // Encoding a message with an unpromoted lazy extension writes the payload
  // back out verbatim, in both encoding modes.
  const std::string lazy_bytes = Encode(msg);
  EXPECT_EQ(lazy_bytes, wire);
  EXPECT_EQ(Encode(msg, kUpb_EncodeOption_Deterministic), wire);

  // Promotion does not change the serialized form.
  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  EXPECT_EQ(Encode(msg), wire);
  EXPECT_EQ(Encode(msg, kUpb_EncodeOption_Deterministic), wire);
}

TEST_F(LazyExtensionTest, EncodeCoalescedPayloadRoundTrips) {
  const std::string wire =
      SerializeModelWithExt1("hello") + SerializeModelWithExt1("world");
  upb_Message* msg = Parse(wire);
  const std::string lazy_bytes = Encode(msg, kUpb_EncodeOption_Deterministic);

  // The re-encoded message parses (eagerly, with the generated code and the
  // generated extension minitable) to the merged value.
  upb_ExtensionRegistry* eager_registry = upb_ExtensionRegistry_New(arena_);
  ASSERT_EQ(upb_ExtensionRegistry_Add(eager_registry,
                                      upb_test_ModelExtension1_model_ext_ext),
            kUpb_ExtensionRegistryStatus_Ok);
  upb_test_ModelWithExtensions* parsed = upb_test_ModelWithExtensions_parse_ex(
      lazy_bytes.data(), lazy_bytes.size(), eager_registry, 0, arena_);
  ASSERT_NE(parsed, nullptr);
  EXPECT_EQ(upb_test_ModelWithExtensions_random_int32(parsed), 42);
  const upb_test_ModelExtension1* ext1 =
      upb_test_ModelExtension1_model_ext(parsed);
  ASSERT_NE(ext1, nullptr);
  EXPECT_EQ(ToString(upb_test_ModelExtension1_str(ext1)), "world");
}

TEST_F(LazyExtensionTest, MalformedPayloadIsDetectedAtPromotion) {
  // Field 1547, length-delimited, payload "\x0a\x05ab" claims a 5 byte string
  // but only provides 2 bytes.
  std::string wire;
  const uint32_t tag = (1547 << 3) | 2;
  // Varint-encode the tag.
  wire.push_back(static_cast<char>((tag & 0x7f) | 0x80));
  wire.push_back(static_cast<char>(tag >> 7));
  wire.push_back(4);
  wire +=
      "\x0a\x05"
      "ab";

  // The outer parse succeeds because the payload is not inspected.
  upb_Message* msg = Parse(wire);
  EXPECT_TRUE(upb_Message_HasExtension(msg, &lazy_ext_));

  upb_MessageValue val;
  EXPECT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_ParseError);
  // The message is unchanged and the error is reproducible.
  EXPECT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(Entry(msg)));
  EXPECT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_ParseError);
  // And the bytes still round-trip.
  EXPECT_EQ(Encode(msg), wire);
}

TEST_F(LazyExtensionTest, SetExtensionReplacesLazyPayload) {
  upb_Message* msg = Parse(SerializeModelWithExt1("hello"));
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(arena_);
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString("set"));
  ASSERT_TRUE(upb_Message_SetExtensionMessage(msg, &lazy_ext_, UPB_UPCAST(ext1),
                                              arena_));
  EXPECT_EQ(upb_Message_ExtensionCount(msg), 1);
  EXPECT_FALSE(upb_TaggedAuxPtr_IsLazyExtension(Entry(msg)));
  EXPECT_EQ(upb_Message_GetExtensionMessage(msg, &lazy_ext_, nullptr),
            UPB_UPCAST(ext1));
  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  EXPECT_EQ(val.msg_val, UPB_UPCAST(ext1));
}

TEST_F(LazyExtensionTest, ClearExtensionRemovesLazyPayload) {
  upb_Message* msg = Parse(SerializeModelWithExt1("hello"));
  upb_Message_ClearExtension(msg, &lazy_ext_);
  EXPECT_FALSE(upb_Message_HasExtension(msg, &lazy_ext_));
  EXPECT_EQ(upb_Message_ExtensionCount(msg), 0);
  EXPECT_TRUE(upb_TaggedAuxPtr_IsNull(Entry(msg)));
  upb_MessageValue val;
  EXPECT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_NotPresent);
  // Only `random_int32: 42` remains.
  EXPECT_EQ(Encode(msg), "\x18\x2a");
}

TEST_F(LazyExtensionTest, DiscardUnknownKeepsLazyExtensions) {
  // Append an unknown field (number 2000, varint 1) to the wire format.
  std::string wire = SerializeModelWithExt1("hello");
  const uint32_t tag = (2000 << 3) | 0;
  wire.push_back(static_cast<char>((tag & 0x7f) | 0x80));
  wire.push_back(static_cast<char>(tag >> 7));
  wire.push_back(1);
  upb_Message* msg = Parse(wire);
  EXPECT_TRUE(upb_Message_HasUnknown(msg));
  _upb_Message_DiscardUnknown_shallow(msg);
  EXPECT_FALSE(upb_Message_HasUnknown(msg));
  EXPECT_TRUE(upb_Message_HasExtension(msg, &lazy_ext_));
  EXPECT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(Entry(msg)));
}

TEST_F(LazyExtensionTest, DeepCloneCopiesLazyPayload) {
  upb_Arena* src_arena = upb_Arena_New();
  upb_Message* src = Parse(SerializeModelWithExt1("hello"), 0, src_arena);
  const upb_LazyExtensionData* src_lazy =
      upb_TaggedAuxPtr_LazyExtension(Entry(src));

  upb_Message* clone = upb_Message_DeepClone(src, kModelMiniTable, arena_);
  ASSERT_NE(clone, nullptr);
  upb_TaggedAuxPtr entry = Entry(clone);
  ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
  EXPECT_FALSE(upb_TaggedAuxPtr_IsLazyExtensionAliased(entry));
  const upb_LazyExtensionData* lazy = upb_TaggedAuxPtr_LazyExtension(entry);
  EXPECT_NE(lazy, src_lazy);
  EXPECT_NE(lazy->data.data, src_lazy->data.data);
  EXPECT_EQ(lazy->ext, &lazy_ext_);
  EXPECT_EQ(lazy->registry, registry_);
  EXPECT_EQ(lazy->options, src_lazy->options);

  // The clone is fully independent of the source.
  upb_Arena_Free(src_arena);
  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(clone, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  EXPECT_EQ(
      ToString(upb_test_ModelExtension1_str(
          reinterpret_cast<const upb_test_ModelExtension1*>(val.msg_val))),
      "hello");
}

TEST_F(LazyExtensionTest, DeepClonePromotedExtensionBecomesCanonical) {
  upb_Message* src = Parse(SerializeModelWithExt1("hello"));
  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(src, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);

  upb_Message* clone = upb_Message_DeepClone(src, kModelMiniTable, arena_);
  ASSERT_NE(clone, nullptr);
  upb_TaggedAuxPtr entry = Entry(clone);
  EXPECT_TRUE(upb_TaggedAuxPtr_IsCanonicalExtension(entry));
  EXPECT_FALSE(upb_TaggedAuxPtr_IsPromotedExtension(entry));
  upb_Message* cloned_sub =
      upb_Message_GetExtensionMessage(clone, &lazy_ext_, nullptr);
  ASSERT_NE(cloned_sub, nullptr);
  EXPECT_NE(cloned_sub, val.msg_val);
  EXPECT_EQ(ToString(upb_test_ModelExtension1_str(
                reinterpret_cast<const upb_test_ModelExtension1*>(cloned_sub))),
            "hello");
}

TEST_F(LazyExtensionTest, ShallowCopyAliasesLazyPayload) {
  upb_Message* src = Parse(SerializeModelWithExt1("hello"));
  const upb_LazyExtensionData* src_lazy =
      upb_TaggedAuxPtr_LazyExtension(Entry(src));

  upb_Message* dst = upb_Message_New(kModelMiniTable, arena_);
  ASSERT_TRUE(upb_Message_ShallowCopy(dst, src, kModelMiniTable, arena_));
  upb_TaggedAuxPtr entry = Entry(dst);
  ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
  EXPECT_TRUE(upb_TaggedAuxPtr_IsLazyExtensionAliased(entry));
  const upb_LazyExtensionData* lazy = upb_TaggedAuxPtr_LazyExtension(entry);
  EXPECT_NE(lazy, src_lazy);
  EXPECT_EQ(lazy->data.data, src_lazy->data.data);
  EXPECT_EQ(lazy->data.size, src_lazy->data.size);

  // Promoting the copy does not affect the source.
  upb_MessageValue val;
  ASSERT_EQ(upb_Message_PromoteLazyExtension(dst, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
  EXPECT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(Entry(src)));
}

TEST_F(LazyExtensionTest, IsEqualHandlesLazyExtensions) {
  const std::string hello = SerializeModelWithExt1("hello");
  const std::string world = SerializeModelWithExt1("world");
  const upb_Message* lazy_hello = Parse(hello);
  const upb_Message* lazy_hello2 = Parse(hello);
  const upb_Message* lazy_world = Parse(world);

  // Eagerly parsed counterpart, using the same (lazy) extension minitable.
  upb_Message* eager_hello = upb_Message_New(kModelMiniTable, arena_);
  upb_test_ModelWithExtensions_set_random_int32(
      reinterpret_cast<upb_test_ModelWithExtensions*>(eager_hello), 42);
  upb_test_ModelExtension1* ext1 = upb_test_ModelExtension1_new(arena_);
  upb_test_ModelExtension1_set_str(ext1, upb_StringView_FromString("hello"));
  ASSERT_TRUE(upb_Message_SetExtensionMessage(eager_hello, &lazy_ext_,
                                              UPB_UPCAST(ext1), arena_));

  // Lazy vs lazy with identical bytes (fast path).
  EXPECT_TRUE(upb_Message_IsEqual(lazy_hello, lazy_hello2, kModelMiniTable, 0));
  // Lazy vs lazy with different bytes.
  EXPECT_FALSE(upb_Message_IsEqual(lazy_hello, lazy_world, kModelMiniTable, 0));
  // Lazy vs eager, both directions.
  EXPECT_TRUE(upb_Message_IsEqual(lazy_hello, eager_hello, kModelMiniTable, 0));
  EXPECT_TRUE(upb_Message_IsEqual(eager_hello, lazy_hello, kModelMiniTable, 0));
  EXPECT_FALSE(
      upb_Message_IsEqual(lazy_world, eager_hello, kModelMiniTable, 0));
  // Comparing never promotes.
  EXPECT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(Entry(lazy_hello)));
  EXPECT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(Entry(lazy_world)));

  // Missing on one side.
  upb_Message* empty = upb_Message_New(kModelMiniTable, arena_);
  upb_test_ModelWithExtensions_set_random_int32(
      reinterpret_cast<upb_test_ModelWithExtensions*>(empty), 42);
  EXPECT_FALSE(upb_Message_IsEqual(lazy_hello, empty, kModelMiniTable, 0));
  EXPECT_FALSE(upb_Message_IsEqual(empty, lazy_hello, kModelMiniTable, 0));

  // Still equal after promoting one side.
  upb_MessageValue val;
  ASSERT_EQ(
      upb_Message_PromoteLazyExtension(lazy_hello, &lazy_ext_, arena_, &val),
      kUpb_GetExtension_Ok);
  EXPECT_TRUE(upb_Message_IsEqual(lazy_hello, lazy_hello2, kModelMiniTable, 0));
  EXPECT_TRUE(upb_Message_IsEqual(lazy_hello2, lazy_hello, kModelMiniTable, 0));
}

TEST_F(LazyExtensionTest, FrozenMessagePromotesFrozenSubmessage) {
  upb_Message* msg = Parse(SerializeModelWithExt1("hello"));
  upb_Message_Freeze(msg, kModelMiniTable);
  ASSERT_TRUE(upb_Message_IsFrozen(msg));

  const upb_Message* const_msg = msg;
  upb_MessageValue val;
  ASSERT_EQ(
      upb_Message_PromoteLazyExtension(const_msg, &lazy_ext_, arena_, &val),
      kUpb_GetExtension_Ok);
  EXPECT_TRUE(upb_Message_IsFrozen(val.msg_val));
  EXPECT_EQ(
      ToString(upb_test_ModelExtension1_str(
          reinterpret_cast<const upb_test_ModelExtension1*>(val.msg_val))),
      "hello");
}

TEST_F(LazyExtensionTest, ConcurrentPromotionPublishesExactlyOneValue) {
  constexpr int kThreads = 16;
  constexpr int kRounds = 20;
  for (int round = 0; round < kRounds; round++) {
    upb_Message* msg = Parse(SerializeModelWithExt1("hello"));
    upb_Message_Freeze(msg, kModelMiniTable);
    const upb_Message* const_msg = msg;

    absl::Notification start;
    std::vector<const upb_Message*> results(kThreads, nullptr);
    std::vector<upb_GetExtension_Status> statuses(kThreads);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; i++) {
      threads.emplace_back([&, i] {
        start.WaitForNotification();
        upb_MessageValue val;
        statuses[i] = upb_Message_PromoteLazyExtension(const_msg, &lazy_ext_,
                                                       arena_, &val);
        results[i] = val.msg_val;
        // Concurrent readers must observe either no value or the published
        // one; never a torn or stale pointer.
        const upb_Message* seen =
            upb_Message_GetExtensionMessage(const_msg, &lazy_ext_, nullptr);
        EXPECT_TRUE(seen == nullptr || seen == results[i]);
      });
    }
    start.Notify();
    for (auto& t : threads) t.join();

    for (int i = 0; i < kThreads; i++) {
      EXPECT_EQ(statuses[i], kUpb_GetExtension_Ok);
      EXPECT_EQ(results[i], results[0]);
    }
    ASSERT_NE(results[0], nullptr);
    EXPECT_TRUE(upb_Message_IsFrozen(results[0]));
    EXPECT_EQ(
        ToString(upb_test_ModelExtension1_str(
            reinterpret_cast<const upb_test_ModelExtension1*>(results[0]))),
        "hello");
    EXPECT_EQ(upb_Message_GetExtensionMessage(const_msg, &lazy_ext_, nullptr),
              results[0]);
  }
}

TEST_F(LazyExtensionTest, NestedDepthLimitIsPreserved) {
  // With max depth 1 the extension submessage uses up the last level of
  // nesting; the remaining budget (zero) cannot be expressed as a lazy decode
  // option, so the decoder falls back to parsing eagerly, exactly like it would
  // without laziness.
  const std::string wire = SerializeModelWithExt1("hello");
  upb_Message* msg = upb_Message_New(kModelMiniTable, arena_);
  ASSERT_EQ(upb_Decode(wire.data(), wire.size(), msg, kModelMiniTable,
                       registry_, upb_DecodeOptions_MaxDepth(1), arena_),
            kUpb_DecodeStatus_Ok);
  EXPECT_TRUE(upb_TaggedAuxPtr_IsCanonicalExtension(Entry(msg)));
  EXPECT_FALSE(upb_TaggedAuxPtr_IsPromotedExtension(Entry(msg)));
  EXPECT_NE(upb_Message_GetExtensionMessage(msg, &lazy_ext_, nullptr), nullptr);

  // With depth 2 the payload is stored lazily and remembers that only one
  // more level of nesting is allowed.
  msg = upb_Message_New(kModelMiniTable, arena_);
  ASSERT_EQ(upb_Decode(wire.data(), wire.size(), msg, kModelMiniTable,
                       registry_, upb_DecodeOptions_MaxDepth(2), arena_),
            kUpb_DecodeStatus_Ok);
  upb_TaggedAuxPtr entry = Entry(msg);
  ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
  EXPECT_EQ(upb_DecodeOptions_GetEffectiveMaxDepth(
                upb_TaggedAuxPtr_LazyExtension(entry)->options),
            1);
  upb_MessageValue val;
  EXPECT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext_, arena_, &val),
            kUpb_GetExtension_Ok);
}

#ifndef UPB_SUPPRESS_MISSING_ATOMICS

// Concurrency fuzz tests.
//
// These mirror the arena fuzz tests in //third_party/upb/upb/mem:arena_test
// (FuzzFuseFuseRace, FuzzFuseSpaceAllocatedRace, ...): a pool of shared
// objects is hammered by several threads performing random operations for a
// fixed wall-clock budget, while a churn thread replaces pool entries. They are
// most useful under TSAN, where the upb_Xsan access annotations on
// upb_Message_Internal and upb_Arena turn any violation of the documented
// thread-safety contracts into a reported race, even if the underlying accesses
// happen to be atomic.
//
// The contract under test: upb_Message_PromoteLazyExtension() is a read-only
// operation and may race with any other read-only operation on the same
// message (readers, iteration, encoding, comparison, cloning, other promotions
// of the same or different extensions), as well as with arbitrary fuses into
// and SpaceAllocated() walks of the message arena.

constexpr int kNumFuzzExts = 6;

class LazyFuzzEnvironment {
 public:
  LazyFuzzEnvironment() : registry_arena_(upb_Arena_New()) {
    const upb_MiniTableExtension* eager[kNumFuzzExts] = {
        upb_test_ModelExtension1_model_ext_ext,
        upb_test_ModelExtension2_model_ext_ext,
        upb_test_ModelExtension2_model_ext_2_ext,
        upb_test_ModelExtension2_model_ext_3_ext,
        upb_test_ModelExtension2_model_ext_4_ext,
        upb_test_ModelExtension2_model_ext_5_ext,
    };
    registry_ = upb_ExtensionRegistry_New(registry_arena_);
    for (int i = 0; i < kNumFuzzExts; i++) {
      exts_[i] = *eager[i];
      EXPECT_TRUE(upb_MiniTableExtension_SetLazy(&exts_[i], true));
      EXPECT_EQ(upb_ExtensionRegistry_Add(registry_, &exts_[i]),
                kUpb_ExtensionRegistryStatus_Ok);
    }

    // Build the wire format with the generated (eager) code.
    upb_test_ModelWithExtensions* src =
        upb_test_ModelWithExtensions_new(registry_arena_);
    upb_test_ModelWithExtensions_set_random_int32(src, 42);
    upb_test_ModelExtension1* e1 =
        upb_test_ModelExtension1_new(registry_arena_);
    upb_test_ModelExtension1_set_str(e1, upb_StringView_FromString(kExt1Str));
    upb_test_ModelExtension1_set_model_ext(src, e1, registry_arena_);
    upb_test_ModelExtension2* e2[kNumFuzzExts];
    for (int i = 1; i < kNumFuzzExts; i++) {
      e2[i] = upb_test_ModelExtension2_new(registry_arena_);
      upb_test_ModelExtension2_set_i(e2[i], Ext2Value(i));
    }
    upb_test_ModelExtension2_set_model_ext(src, e2[1], registry_arena_);
    upb_test_ModelExtension2_set_model_ext_2(src, e2[2], registry_arena_);
    upb_test_ModelExtension2_set_model_ext_3(src, e2[3], registry_arena_);
    upb_test_ModelExtension2_set_model_ext_4(src, e2[4], registry_arena_);
    upb_test_ModelExtension2_set_model_ext_5(src, e2[5], registry_arena_);
    size_t size;
    char* buf =
        upb_test_ModelWithExtensions_serialize(src, registry_arena_, &size);
    wire_.assign(buf, size);

    // A fully promoted reference copy, for IsEqual() checks. (Extensions are
    // matched by upb_MiniTableExtension identity, so the reference must be
    // parsed with the same registry as the messages under test.)
    reference_ = upb_Message_New(kModelMiniTable, registry_arena_);
    EXPECT_EQ(upb_Decode(wire_.data(), wire_.size(), reference_,
                         kModelMiniTable, registry_, 0, registry_arena_),
              kUpb_DecodeStatus_Ok);
    for (int i = 0; i < kNumFuzzExts; i++) {
      upb_MessageValue val;
      EXPECT_EQ(upb_Message_PromoteLazyExtension(reference_, &exts_[i],
                                                 registry_arena_, &val),
                kUpb_GetExtension_Ok);
    }
    upb_Message_Freeze(reference_, kModelMiniTable);
  }

  ~LazyFuzzEnvironment() {
    VerifyAll();
    {
      absl::MutexLock lock(&mutex_);
      for (auto& slot : messages_) slot.reset();
    }
    upb_Arena_Free(registry_arena_);
  }

  // A lazily decoded message and the arena that owns it. Shared between
  // threads via shared_ptr so that a slot can be replaced while other threads
  // are still using the old message; the arena is freed with the last user.
  struct Message {
    upb_Arena* arena;
    const upb_Message* msg;
    bool frozen;
    ~Message() { upb_Arena_Free(arena); }
  };

  std::shared_ptr<const Message> NewMessage(absl::BitGen& gen) {
    auto m = std::make_shared<Message>();
    m->arena = upb_Arena_New();
    upb_Message* msg = upb_Message_New(kModelMiniTable, m->arena);
    // Alias half of the time so that both lazy tags are exercised; `wire_` is
    // immutable and outlives every message.
    int options = absl::Bernoulli(gen, 0.5) ? kUpb_DecodeOption_AliasString : 0;
    EXPECT_EQ(upb_Decode(wire_.data(), wire_.size(), msg, kModelMiniTable,
                         registry_, options, m->arena),
              kUpb_DecodeStatus_Ok);
    m->frozen = absl::Bernoulli(gen, 0.5);
    if (m->frozen) upb_Message_Freeze(msg, kModelMiniTable);
    m->msg = msg;
    return m;
  }

  // Replaces a random slot with a fresh message (cf. Environment::RandomNewFree
  // in arena_test.cc). The old message is destroyed once its last user is done.
  void RandomReplace(absl::BitGen& gen) {
    std::shared_ptr<const Message> fresh = NewMessage(gen);
    absl::MutexLock lock(&mutex_);
    messages_[RandomIndex(gen)].swap(fresh);
  }

  // Promotes a random extension of a random message. `arena` is either the
  // message's own arena or a caller-provided arena that is fused with it.
  void RandomPromote(absl::BitGen& gen, upb_Arena* arena = nullptr) {
    std::shared_ptr<const Message> m = RandomMessage(gen);
    int i = RandomExt(gen);
    if (arena) {
      ASSERT_TRUE(upb_Arena_Fuse(m->arena, arena));
    } else {
      arena = m->arena;
    }
    upb_MessageValue val;
    ASSERT_EQ(upb_Message_PromoteLazyExtension(m->msg, &exts_[i], arena, &val),
              kUpb_GetExtension_Ok);
    VerifyExt(i, val.msg_val, m->frozen);
    // Once promoted, every subsequent read must observe the same pointer.
    EXPECT_EQ(upb_Message_GetExtensionMessage(m->msg, &exts_[i], nullptr),
              val.msg_val);
  }

  // Reads a random extension; it is either not promoted yet or fully parsed.
  void RandomRead(absl::BitGen& gen) {
    std::shared_ptr<const Message> m = RandomMessage(gen);
    int i = RandomExt(gen);
    EXPECT_TRUE(upb_Message_HasExtension(m->msg, &exts_[i]));
    const upb_Message* sub =
        upb_Message_GetExtensionMessage(m->msg, &exts_[i], nullptr);
    if (sub) VerifyExt(i, sub, m->frozen);
  }

  // Iterates all (promoted) extensions and the extension count.
  void RandomIterate(absl::BitGen& gen) {
    std::shared_ptr<const Message> m = RandomMessage(gen);
    EXPECT_EQ(upb_Message_ExtensionCount(m->msg),
              static_cast<size_t>(kNumFuzzExts));
    uintptr_t iter = kUpb_Message_ExtensionBegin;
    const upb_MiniTableExtension* ext;
    upb_MessageValue val;
    int seen = 0;
    while (upb_Message_NextExtension(m->msg, &ext, &val, &iter)) {
      VerifyExt(ExtIndex(ext), val.msg_val, m->frozen);
      seen++;
    }
    EXPECT_LE(seen, kNumFuzzExts);
  }

  // Encodes the message; the output must not depend on which extensions have
  // been promoted so far.
  void RandomEncode(absl::BitGen& gen) {
    std::shared_ptr<const Message> m = RandomMessage(gen);
    upb_Arena* arena = upb_Arena_New();
    int options =
        absl::Bernoulli(gen, 0.5) ? kUpb_EncodeOption_Deterministic : 0;
    char* buf;
    size_t size;
    ASSERT_EQ(upb_Encode(m->msg, kModelMiniTable, options, arena, &buf, &size),
              kUpb_EncodeStatus_Ok);
    upb_Message* reparsed = upb_Message_New(kModelMiniTable, arena);
    ASSERT_EQ(
        upb_Decode(buf, size, reparsed, kModelMiniTable, registry_, 0, arena),
        kUpb_DecodeStatus_Ok);
    EXPECT_TRUE(upb_Message_IsEqual(reparsed, reference_, kModelMiniTable, 0));
    upb_Arena_Free(arena);
  }

  // Compares against the eager reference, which parses unpromoted extensions
  // into a scratch arena without mutating the message.
  void RandomIsEqual(absl::BitGen& gen) {
    std::shared_ptr<const Message> m = RandomMessage(gen);
    EXPECT_TRUE(upb_Message_IsEqual(m->msg, reference_, kModelMiniTable, 0));
  }

  // Deep clones into a private arena; lazy payloads are copied, promoted
  // extensions become ordinary canonical extensions.
  void RandomClone(absl::BitGen& gen) {
    std::shared_ptr<const Message> m = RandomMessage(gen);
    upb_Arena* arena = upb_Arena_New();
    upb_Message* clone = upb_Message_DeepClone(m->msg, kModelMiniTable, arena);
    ASSERT_NE(clone, nullptr);
    EXPECT_TRUE(upb_Message_IsEqual(clone, reference_, kModelMiniTable, 0));
    for (int i = 0; i < kNumFuzzExts; i++) {
      upb_MessageValue val;
      ASSERT_EQ(upb_Message_PromoteLazyExtension(clone, &exts_[i], arena, &val),
                kUpb_GetExtension_Ok);
      VerifyExt(i, val.msg_val, false);
    }
    upb_Arena_Free(arena);
  }

  // Races arena operations against the fuse that promotion performs into the
  // message arena (cf. FuzzFuseSpaceAllocatedRace / FuzzFuseFuseRace).
  void RandomArenaOp(absl::BitGen& gen) {
    std::shared_ptr<const Message> m = RandomMessage(gen);
    switch (absl::Uniform(gen, 0, 3)) {
      case 0: {
        upb_Arena* other = upb_Arena_New();
        EXPECT_TRUE(upb_Arena_Fuse(m->arena, other));
        upb_Arena_Free(other);
        break;
      }
      case 1: {
        size_t count;
        EXPECT_GT(upb_Arena_SpaceAllocated(m->arena, &count), 0u);
        EXPECT_GE(count, 1u);
        break;
      }
      case 2:
        EXPECT_TRUE(upb_Arena_IsFused(m->arena, m->arena));
        break;
    }
  }

  void RandomPoke(absl::BitGen& gen, upb_Arena* thread_arena = nullptr) {
    switch (absl::Uniform(gen, 0, 8)) {
      case 0:
        RandomPromote(gen);
        break;
      case 1:
        RandomPromote(gen, thread_arena);
        break;
      case 2:
        RandomRead(gen);
        break;
      case 3:
        RandomIterate(gen);
        break;
      case 4:
        RandomEncode(gen);
        break;
      case 5:
        RandomIsEqual(gen);
        break;
      case 6:
        RandomClone(gen);
        break;
      case 7:
        RandomArenaOp(gen);
        break;
    }
  }

  // Single-threaded final check of every live message: promoting everything
  // must succeed, agree with earlier reads, and leave exactly one entry per
  // extension in aux_data (i.e. no double publication).
  void VerifyAll() {
    absl::MutexLock lock(&mutex_);
    for (auto& slot : messages_) {
      if (!slot) continue;
      for (int i = 0; i < kNumFuzzExts; i++) {
        upb_MessageValue val;
        ASSERT_EQ(upb_Message_PromoteLazyExtension(slot->msg, &exts_[i],
                                                   slot->arena, &val),
                  kUpb_GetExtension_Ok);
        VerifyExt(i, val.msg_val, slot->frozen);
        EXPECT_EQ(
            upb_Message_GetExtensionMessage(slot->msg, &exts_[i], nullptr),
            val.msg_val);
      }
      EXPECT_EQ(upb_Message_ExtensionCount(slot->msg),
                static_cast<size_t>(kNumFuzzExts));
      int entries[kNumFuzzExts] = {};
      const upb_Message_Internal* in =
          UPB_PRIVATE(_upb_Message_GetInternal)(slot->msg);
      ASSERT_NE(in, nullptr);
      for (size_t j = 0; j < in->size; j++) {
        upb_TaggedAuxPtr ptr = UPB_PRIVATE(_upb_Message_Internal_GetAux)(in, j);
        ASSERT_TRUE(upb_TaggedAuxPtr_IsCanonicalExtension(ptr));
        entries[ExtIndex(upb_TaggedAuxPtr_CanonicalExtension(ptr)->ext)]++;
      }
      for (int i = 0; i < kNumFuzzExts; i++) EXPECT_EQ(entries[i], 1);
      EXPECT_TRUE(
          upb_Message_IsEqual(slot->msg, reference_, kModelMiniTable, 0));
    }
  }

 private:
  static constexpr const char* kExt1Str = "lazy extension payload";
  static int32_t Ext2Value(int i) { return 1000 + i; }

  static constexpr size_t kPoolSize = 32;

  size_t RandomIndex(absl::BitGen& gen) {
    return absl::Uniform<size_t>(gen, 0, kPoolSize);
  }
  int RandomExt(absl::BitGen& gen) {
    return absl::Uniform(gen, 0, kNumFuzzExts);
  }

  int ExtIndex(const upb_MiniTableExtension* ext) {
    for (int i = 0; i < kNumFuzzExts; i++) {
      if (ext == &exts_[i]) return i;
    }
    ADD_FAILURE() << "unknown extension";
    return 0;
  }

  void VerifyExt(int i, const upb_Message* sub, bool frozen) {
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(upb_Message_IsFrozen(sub), frozen);
    if (i == 0) {
      EXPECT_EQ(ToString(upb_test_ModelExtension1_str(
                    reinterpret_cast<const upb_test_ModelExtension1*>(sub))),
                kExt1Str);
    } else {
      EXPECT_EQ(upb_test_ModelExtension2_i(
                    reinterpret_cast<const upb_test_ModelExtension2*>(sub)),
                Ext2Value(i));
    }
  }

  // Returns a random pool entry, creating it if the slot is empty. The result
  // is shared with other threads.
  std::shared_ptr<const Message> RandomMessage(absl::BitGen& gen) {
    size_t index = RandomIndex(gen);
    absl::MutexLock lock(&mutex_);
    std::shared_ptr<const Message>& ret = messages_[index];
    if (!ret) ret = NewMessage(gen);
    return ret;
  }

  upb_Arena* registry_arena_;
  upb_MiniTableExtension exts_[kNumFuzzExts];
  upb_ExtensionRegistry* registry_;
  std::string wire_;
  upb_Message* reference_;

  absl::Mutex mutex_;
  std::array<std::shared_ptr<const Message>, kPoolSize> messages_
      ABSL_GUARDED_BY(mutex_);
};

// Runs `thread_count` worker threads calling `worker` until `budget` elapses,
// while the calling thread performs `main_op`.
template <typename Worker, typename MainOp>
void RunFuzz(int thread_count, absl::Duration budget, Worker worker,
             MainOp main_op) {
  absl::Notification done;
  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  for (int i = 0; i < thread_count; ++i) {
    threads.emplace_back([&] {
      absl::BitGen gen;
      while (!done.HasBeenNotified()) worker(gen);
    });
  }
  absl::BitGen gen;
  auto end = absl::Now() + budget;
  while (absl::Now() < end) main_op(gen);
  done.Notify();
  for (auto& t : threads) t.join();
}

TEST(LazyExtensionFuzzTest, SingleThreaded) {
  LazyFuzzEnvironment env;
  absl::BitGen gen;
  auto end = absl::Now() + absl::Seconds(0.5);
  while (absl::Now() < end) {
    env.RandomPoke(gen);
    if (absl::Bernoulli(gen, 0.1)) env.RandomReplace(gen);
  }
}

// Promotions of the same and different extensions racing with each other and
// with every read-only message operation, on a churning pool of messages.
TEST(LazyExtensionFuzzTest, PromoteReadRace) {
  LazyFuzzEnvironment env;
  RunFuzz(
      10, absl::Seconds(2), [&](absl::BitGen& gen) { env.RandomPoke(gen); },
      [&](absl::BitGen& gen) { env.RandomReplace(gen); });
}

// Many threads promoting only, so that first-promotion CAS races are frequent;
// the main thread keeps replacing messages so there is always something left
// to promote.
TEST(LazyExtensionFuzzTest, PromotePromoteRace) {
  LazyFuzzEnvironment env;
  RunFuzz(
      10, absl::Seconds(2), [&](absl::BitGen& gen) { env.RandomPromote(gen); },
      [&](absl::BitGen& gen) { env.RandomReplace(gen); });
}

// Promotion through per-thread arenas that are fused into the message arenas,
// racing with other fuses into, and SpaceAllocated() walks of, those arenas
// (cf. FuzzFuseSpaceAllocatedRace). Each thread periodically swaps in a fresh
// arena to keep the fuse groups bounded.
TEST(LazyExtensionFuzzTest, PromoteFuseRace) {
  LazyFuzzEnvironment env;
  RunFuzz(
      10, absl::Seconds(2),
      [&](absl::BitGen& gen) {
        upb_Arena* thread_arena = upb_Arena_New();
        for (int i = 0; i < 16; i++) {
          if (absl::Bernoulli(gen, 0.5)) {
            env.RandomPromote(gen, thread_arena);
          } else {
            env.RandomArenaOp(gen);
          }
        }
        upb_Arena_Free(thread_arena);
      },
      [&](absl::BitGen& gen) { env.RandomReplace(gen); });
}

#endif  // UPB_SUPPRESS_MISSING_ATOMICS

// MessageSet wire format.

const upb_MiniTable* kMsgSetMiniTable =
    &protobuf_0test_0messages__proto2__TestAllTypesProto2__MessageSetCorrect_msg_init;

TEST(LazyMessageSetTest, DecodePromoteAndEncode) {
  upb_Arena* arena = upb_Arena_New();
  upb_MiniTableExtension lazy_ext =
      *protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_message_set_extension_ext;
  ASSERT_TRUE(upb_MiniTableExtension_SetLazy(&lazy_ext, true));
  upb_ExtensionRegistry* registry = upb_ExtensionRegistry_New(arena);
  ASSERT_EQ(upb_ExtensionRegistry_Add(registry, &lazy_ext),
            kUpb_ExtensionRegistryStatus_Ok);

  // Build the wire format with the generated (eager) code.
  auto* src =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_new(
          arena);
  auto* src_ext =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_new(
          arena);
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_str(
      src_ext, upb_StringView_FromString("hello"));
  protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_set_message_set_extension(
      src, src_ext, arena);
  size_t size;
  char* buf =
      protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrect_serialize(
          src, arena, &size);
  const std::string wire(buf, size);

  for (int options : {0, static_cast<int>(kUpb_DecodeOption_AliasString)}) {
    upb_Message* msg = upb_Message_New(kMsgSetMiniTable, arena);
    ASSERT_EQ(upb_Decode(wire.data(), wire.size(), msg, kMsgSetMiniTable,
                         registry, options, arena),
              kUpb_DecodeStatus_Ok);
    upb_TaggedAuxPtr entry = upb_TaggedAuxPtr_Null();
    ASSERT_TRUE(UPB_PRIVATE(_upb_Message_FindExtensionEntry)(msg, &lazy_ext,
                                                             nullptr, &entry));
    ASSERT_TRUE(upb_TaggedAuxPtr_IsLazyExtension(entry));
    EXPECT_EQ(upb_TaggedAuxPtr_IsLazyExtensionAliased(entry),
              options == kUpb_DecodeOption_AliasString);

    // Lazy MessageSet items are re-emitted with MessageSet item framing.
    for (int enc_options :
         {0, static_cast<int>(kUpb_EncodeOption_Deterministic)}) {
      char* out;
      size_t out_size;
      ASSERT_EQ(upb_Encode(msg, kMsgSetMiniTable, enc_options, arena, &out,
                           &out_size),
                kUpb_EncodeStatus_Ok);
      EXPECT_EQ(std::string(out, out_size), wire);
    }

    upb_MessageValue val;
    ASSERT_EQ(upb_Message_PromoteLazyExtension(msg, &lazy_ext, arena, &val),
              kUpb_GetExtension_Ok);
    upb_StringView str =
        protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1_str(
            reinterpret_cast<
                const protobuf_test_messages_proto2_TestAllTypesProto2_MessageSetCorrectExtension1*>(
                val.msg_val));
    EXPECT_EQ(ToString(str), "hello");
  }
  upb_Arena_Free(arena);
}

}  // namespace
