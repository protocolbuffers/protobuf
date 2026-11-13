// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arenastring.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/wire_format_lite.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

using internal::ArenaStringPtr;

const internal::LazyString nonempty_default{{{"default", 7}}, {nullptr}};
const std::string* empty_default = &internal::GetEmptyString();

class SingleArena : public testing::TestWithParam<bool> {
 public:
  SingleArena()
      : arena_(GetParam() ? nullptr : std::make_unique<Arena>()),
        message_(Arena::MakeUnique<proto2_unittest::TestEmptyMessage>(
            arena_.get())) {}

  Arena* arena() { return arena_.get(); }

  const MessageLite& parent_message() { return *message_; }

 private:
  std::unique_ptr<Arena> arena_;
  Arena::UniquePtr<proto2_unittest::TestEmptyMessage> message_;
};

INSTANTIATE_TEST_SUITE_P(ArenaString, SingleArena, testing::Bool());

TEST_P(SingleArena, GetSet) {
  ArenaStringPtr field;
  field.InitDefault();
  EXPECT_EQ("", field.Get());
  field.Set("Test short", parent_message());
  EXPECT_EQ("Test short", field.Get());
  field.Set("Test long long long long value", parent_message());
  EXPECT_EQ("Test long long long long value", field.Get());
  field.Set("", parent_message());
  field.Destroy();
}

TEST_P(SingleArena, MutableAccessor) {
  ArenaStringPtr field;
  field.InitDefault();

  std::string* mut = field.Mutable(parent_message());
  EXPECT_EQ(mut, field.Mutable(parent_message()));
  EXPECT_EQ(mut, &field.Get());
  EXPECT_NE(empty_default, mut);
  EXPECT_EQ("", *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ("Test long long long long value", field.Get());
  field.Destroy();
}

TEST_P(SingleArena, NullDefault) {
  ArenaStringPtr field;
  field.InitDefault();
  std::string* mut = field.Mutable(nonempty_default, parent_message());
  EXPECT_EQ(mut, field.Mutable(nonempty_default, parent_message()));
  EXPECT_EQ(mut, &field.Get());
  EXPECT_NE(nullptr, mut);
  EXPECT_EQ("default", *mut);
  *mut = "Test long long long long value";  // ensure string allocates storage
  EXPECT_EQ("Test long long long long value", field.Get());
  field.Destroy();
}

TEST(ArenaStringPtrTest, ConstInit) {
  // Verify that we can constinit construct an ArenaStringPtr from the global
  // string.
  PROTOBUF_CONSTINIT static const ArenaStringPtr ptr(
      &internal::fixed_address_empty_string, internal::ConstantInitialized{});
  EXPECT_EQ(&ptr.Get(), &internal::GetEmptyStringAlreadyInited());
}

TEST_P(SingleArena, ConstructEmpty) {
  ArenaStringPtr field(arena());

  EXPECT_EQ(field.Get(), "");
  if (internal::DebugHardenForceCopyDefaultString()) {
    EXPECT_FALSE(field.IsDefault());
  } else {
    EXPECT_TRUE(field.IsDefault());
  }
  if (arena() == nullptr) field.Destroy();
}

TEST_P(SingleArena, ConstructEmptyWithDefault) {
  internal::LazyString default_value{{{"Hello default", 13}}, {nullptr}};
  ArenaStringPtr field(arena(), default_value);

  if (internal::DebugHardenForceCopyDefaultString()) {
    EXPECT_EQ(field.Get(), "Hello default");
    EXPECT_FALSE(field.IsDefault());
  } else {
    EXPECT_EQ(field.Get(), "");
    EXPECT_TRUE(field.IsDefault());
  }
  if (arena() == nullptr) field.Destroy();
}

TEST_P(SingleArena, CopyConstructEmpty) {
  std::string empty;
  ArenaStringPtr field;
  field.InitExternal(&empty);

  ArenaStringPtr dst(arena(), field);
  EXPECT_EQ(dst.Get(), "");
  if (internal::DebugHardenForceCopyDefaultString()) {
    EXPECT_FALSE(dst.IsDefault());
  } else {
    EXPECT_TRUE(dst.IsDefault());
  }
  if (arena() == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructEmptyWithDefault) {
  std::string empty;
  ArenaStringPtr field;
  field.InitExternal(&empty);

  internal::LazyString default_value{{{"Hello default", 13}}, {nullptr}};
  ArenaStringPtr dst(arena(), field, default_value);
  if (internal::DebugHardenForceCopyDefaultString()) {
    EXPECT_EQ(dst.Get(), "Hello default");
    EXPECT_FALSE(dst.IsDefault());
  } else {
    EXPECT_EQ(dst.Get(), "");
    EXPECT_TRUE(dst.IsDefault());
  }
  if (arena() == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructValueWithDefault) {
  std::string empty;
  ArenaStringPtr field;
  field.InitExternal(&empty);
  field.Set("Hello world", nullptr);

  internal::LazyString default_value{{{"Hello default", 13}}, {nullptr}};
  ArenaStringPtr dst(arena(), field, default_value);
  EXPECT_EQ(dst.Get(), "Hello world");
  if (arena() == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructSSO) {
  std::string empty;
  ArenaStringPtr field;
  field.InitExternal(&empty);
  field.Set("Hello world", nullptr);

  ArenaStringPtr dst(arena(), field);
  EXPECT_EQ(dst.Get(), "Hello world");
  if (arena() == nullptr) dst.Destroy();
  field.Destroy();
}

TEST_P(SingleArena, CopyConstructLong) {
  std::string empty;
  ArenaStringPtr field;
  field.InitExternal(&empty);
  field.Set("A string long enough to not be inlined", nullptr);

  ArenaStringPtr dst(arena(), field);
  EXPECT_EQ(dst.Get(), "A string long enough to not be inlined");
  if (arena() == nullptr) dst.Destroy();
  field.Destroy();
}


TEST(BitMaskTest, Basic) {
  using internal::arenastring_internal::BitMask;
  std::vector<int> empty;
  for (int b : BitMask(0u)) empty.push_back(b);
  EXPECT_TRUE(empty.empty());

  std::vector<int> bits;
  for (int b : BitMask((1u << 0) | (1u << 4) | (1u << 31))) {
    bits.push_back(b);
  }
  EXPECT_THAT(bits, testing::ElementsAre(0, 4, 31));
}

TEST(BitfieldMaskTest, Basic) {
  constexpr uint32_t kOutside = 0xffff001fu;
  EXPECT_FALSE((internal::BitfieldMask<3, 5>(kOutside).any()));

  internal::BitfieldMask<3, 5> m(kOutside | (0b101u << 5));
  EXPECT_TRUE(m.any());
  EXPECT_EQ(m.bits(), 0b101u);
  EXPECT_TRUE(m.test(0));
  EXPECT_FALSE(m.test(1));
  EXPECT_TRUE(m.test(2));
  EXPECT_EQ((internal::BitfieldMask<32, 0>(0x80000001u).bits()), 0x80000001u);
}

template <size_t kCount>
void TestMergeAndClearStringFields(Arena* arena) {
  internal::ArenaStringPtr src[kCount], dst[kCount];
  for (size_t i = 0; i < kCount; ++i) {
    src[i].InitDefault();
    dst[i].InitDefault();
    src[i].Set(std::string((i % 2 == 0 ? 40 : 5) + i, 'a' + i), arena);
  }
  auto dst_msg = Arena::MakeUnique<proto2_unittest::TestEmptyMessage>(arena);

  constexpr int kShift = 3;
  constexpr uint32_t kAll = (uint32_t{1} << kCount) - 1;
  constexpr uint32_t kOutside = ~(kAll << kShift);

  for (uint32_t raw : {0u, 1u, 0b1110u, 0x55555555u, 0xaaaaaaaau,
                       (1u << (kCount - 1)) | 1u, kAll}) {
    const uint32_t bits = raw & kAll;
    SCOPED_TRACE(bits);
    const internal::BitfieldMask<kCount, kShift> mask((bits << kShift) |
                                                      kOutside);
    auto expect_dst = [&](bool cleared) {
      for (size_t i = 0; i < kCount; ++i) {
        absl::string_view expected = "keep";
        if ((bits >> i) & 1u) {
          expected = cleared ? absl::string_view("") : src[i].Get();
        }
        EXPECT_EQ(dst[i].Get(), expected) << i;
      }
    };

    size_t expected_size_1 = 0;
    size_t expected_size_2 = 0;
    for (size_t i = 0; i < kCount; ++i) {
      dst[i].Set("keep", *dst_msg);
      if ((bits >> i) & 1u) {
        const size_t n = internal::WireFormatLite::StringSize(src[i].Get());
        expected_size_1 += 1 + n;
        expected_size_2 += 2 + n;
      }
    }

    EXPECT_EQ(internal::ByteSizeStringFields(mask, src), expected_size_1);
    internal::MergeStringFields(mask, dst, src, *dst_msg);
    expect_dst(/*cleared=*/false);
    internal::ClearStringFields(mask, dst);
    expect_dst(/*cleared=*/true);

    if (bits != 0) {
      EXPECT_EQ(
          (internal::ByteSizeStringFields</*kMinTagSize=*/2,
                                          /*kBatchCheck=*/false>(mask, src)),
          expected_size_2);
      internal::MergeStringFields</*kBatchCheck=*/false>(mask, dst, src,
                                                         *dst_msg);
      expect_dst(/*cleared=*/false);
      internal::ClearStringFields</*kBatchCheck=*/false>(mask, dst);
      expect_dst(/*cleared=*/true);
    }
  }

  if (arena == nullptr) {
    internal::DestroyStringFields<kCount>(dst);
    internal::DestroyStringFields<kCount>(src);
  }
}

TEST(BatchedStringFieldsTest, MergeClearAndDestroy) {
  Arena arena;
  for (Arena* a : {static_cast<Arena*>(nullptr), &arena}) {
    TestMergeAndClearStringFields<1>(a);
    TestMergeAndClearStringFields<2>(a);
    TestMergeAndClearStringFields<4>(a);
    TestMergeAndClearStringFields<10>(a);
  }

  // Exercise out-of-line DestroyStringFields fast path (all default/arena) and
  // slow-path transition across the 8-wide unrolled loop + tail.
  for (size_t count : {size_t{0}, size_t{3}, size_t{10}}) {
    std::vector<internal::ArenaStringPtr> fields(count);
    for (size_t i = 0; i < count; ++i) fields[i].InitDefault();
    internal::arenastring_internal::DestroyStringFields(fields.data(), count);
    if (count >= 2) {
      for (size_t i = 0; i < count; ++i) fields[i].InitDefault();
      fields[0].Set(std::string(48, 'a'), &arena);
      fields[count - 1].Set(std::string(48, 'h'), nullptr);
      internal::arenastring_internal::DestroyStringFields(fields.data(), count);
    }
  }
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
