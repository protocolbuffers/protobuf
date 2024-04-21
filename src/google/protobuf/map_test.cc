// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/map.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/arena_test_util.h"
#include "google/protobuf/internal_visibility_for_testing.h"
#include "google/protobuf/map_field.h"
#include "google/protobuf/map_proto2_unittest.pb.h"
#include "google/protobuf/map_unittest.pb.h"
#include "google/protobuf/reflection_tester.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"


#define BRIDGE_UNITTEST ::google::protobuf::bridge_unittest
#define UNITTEST ::protobuf_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import
#define UNITTEST_PACKAGE_NAME "protobuf_unittest"

// Must include after defining UNITTEST, etc.
// clang-format off
#include "google/protobuf/test_util.inc"
#include "google/protobuf/map_test_util.inc"
#include "google/protobuf/map_test.inc"
// clang-format on

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

struct AlignedAsDefault {
  int x;
};
struct alignas(8) AlignedAs8 {
  int x;
};

template <>
struct is_internal_map_value_type<AlignedAsDefault> : std::true_type {};
template <>
struct is_internal_map_value_type<AlignedAs8> : std::true_type {};

namespace {

using ::testing::AllOf;
using ::testing::FieldsAre;
using ::testing::Ge;
using ::testing::Le;
using ::testing::UnorderedElementsAre;


TEST(MapTest, CopyConstructIntegers) {
  auto token = internal::InternalVisibilityForTesting{};
  using MapType = Map<int32_t, int32_t>;
  MapType original;
  original[1] = 2;
  original[2] = 3;

  MapType map1(original);
  ASSERT_EQ(map1.size(), 2);
  EXPECT_EQ(map1[1], 2);
  EXPECT_EQ(map1[2], 3);

  MapType map2(token, nullptr, original);
  ASSERT_EQ(map2.size(), 2);
  EXPECT_EQ(map2[1], 2);
  EXPECT_EQ(map2[2], 3);
}

TEST(MapTest, CopyConstructStrings) {
  auto token = internal::InternalVisibilityForTesting{};
  using MapType = Map<std::string, std::string>;
  MapType original;
  original["1"] = "2";
  original["2"] = "3";

  MapType map1(original);
  ASSERT_EQ(map1.size(), 2);
  EXPECT_EQ(map1["1"], "2");
  EXPECT_EQ(map1["2"], "3");

  MapType map2(token, nullptr, original);
  ASSERT_EQ(map2.size(), 2);
  EXPECT_EQ(map2["1"], "2");
  EXPECT_EQ(map2["2"], "3");
}

TEST(MapTest, CopyConstructMessages) {
  auto token = internal::InternalVisibilityForTesting{};
  using MapType = Map<std::string, TestAllTypes>;
  MapType original;
  original["1"].set_optional_int32(1);
  original["2"].set_optional_int32(2);

  MapType map1(original);
  ASSERT_EQ(map1.size(), 2);
  EXPECT_EQ(map1["1"].optional_int32(), 1);
  EXPECT_EQ(map1["2"].optional_int32(), 2);

  MapType map2(token, nullptr, original);
  ASSERT_EQ(map2.size(), 2);
  EXPECT_EQ(map2["1"].optional_int32(), 1);
  EXPECT_EQ(map2["2"].optional_int32(), 2);
}

TEST(MapTest, CopyConstructIntegersWithArena) {
  auto token = internal::InternalVisibilityForTesting{};
  using MapType = Map<int32_t, int32_t>;
  MapType original;
  original[1] = 2;
  original[2] = 3;

  Arena arena;
  alignas(MapType) char mem1[sizeof(MapType)];
  MapType& map1 = *new (mem1) MapType(token, &arena, original);
  ASSERT_EQ(map1.size(), 2);
  EXPECT_EQ(map1[1], 2);
  EXPECT_EQ(map1[2], 3);
  EXPECT_EQ(map1[2], 3);
}

TEST(MapTest, CopyConstructStringsWithArena) {
  auto token = internal::InternalVisibilityForTesting{};
  using MapType = Map<std::string, std::string>;
  MapType original;
  original["1"] = "2";
  original["2"] = "3";

  Arena arena;
  alignas(MapType) char mem1[sizeof(MapType)];
  MapType& map1 = *new (mem1) MapType(token, &arena, original);
  ASSERT_EQ(map1.size(), 2);
  EXPECT_EQ(map1["1"], "2");
  EXPECT_EQ(map1["2"], "3");
}

TEST(MapTest, CopyConstructMessagesWithArena) {
  auto token = internal::InternalVisibilityForTesting{};
  using MapType = Map<std::string, TestAllTypes>;
  MapType original;
  original["1"].set_optional_int32(1);
  original["2"].set_optional_int32(2);

  Arena arena;
  alignas(MapType) char mem1[sizeof(MapType)];
  MapType& map1 = *new (mem1) MapType(token, &arena, original);
  ASSERT_EQ(map1.size(), 2);
  EXPECT_EQ(map1["1"].optional_int32(), 1);
  EXPECT_EQ(map1["1"].GetArena(), &arena);
  EXPECT_EQ(map1["2"].optional_int32(), 2);
  EXPECT_EQ(map1["2"].GetArena(), &arena);
}

TEST(MapTest, AlwaysSerializesBothEntries) {
  for (const Message* prototype :
       {static_cast<const Message*>(
            &protobuf_unittest::TestI32StrMap::default_instance()),
        static_cast<const Message*>(
            &proto3_unittest::TestI32StrMap::default_instance())}) {
    const FieldDescriptor* map_field =
        prototype->GetDescriptor()->FindFieldByName("m_32_str");
    const FieldDescriptor* map_key = map_field->message_type()->map_key();
    const FieldDescriptor* map_value = map_field->message_type()->map_value();
    for (bool add_key : {true, false}) {
      for (bool add_value : {true, false}) {
        std::unique_ptr<Message> message(prototype->New());
        Message* entry_message =
            message->GetReflection()->AddMessage(message.get(), map_field);
        // Add the fields, but leave them as the default to make it easier to
        // match.
        if (add_key) {
          entry_message->GetReflection()->SetInt32(entry_message, map_key, 0);
        }
        if (add_value) {
          entry_message->GetReflection()->SetString(entry_message, map_value,
                                                    "");
        }
        ASSERT_EQ(4, entry_message->ByteSizeLong());
        EXPECT_EQ(entry_message->SerializeAsString(),
                  std::string({
                      '\010', '\0',  // key, VARINT, value=0
                      '\022', '\0',  // value, LEN, size=0
                  }));
        ASSERT_EQ(6, message->ByteSizeLong());
        EXPECT_EQ(message->SerializeAsString(),
                  std::string({
                      '\012', '\04',  // field=1, LEN, size=4
                      '\010', '\0',   // key, VARINT, value=0
                      '\022', '\0',   // value, LEN, size=0
                  }));
      }
    }
  }
}

TEST(MapTest, LoadFactorCalculationWorks) {
  // Three stages:
  //  - empty
  //  - small
  //  - large

  const auto calculate = MapTestPeer::CalculateHiCutoff;
  // empty
  EXPECT_EQ(calculate(kGlobalEmptyTableSize), 0);

  // small
  EXPECT_EQ(calculate(2), 2);
  EXPECT_EQ(calculate(4), 4);
  EXPECT_EQ(calculate(8), 8);

  // large
  for (int i = 16; i < 10000; i *= 2) {
    EXPECT_EQ(calculate(i), .75 * i) << "i=" << i;
  }
}

TEST(MapTest, NaturalGrowthOnArenasReuseBlocks) {
  Arena arena;
  std::vector<Map<int, int>*> values;

  static constexpr int kNumFields = 100;
  static constexpr int kNumElems = 1000;
  for (int i = 0; i < kNumFields; ++i) {
    values.push_back(Arena::Create<Map<int, int>>(&arena));
    auto& field = *values.back();
    for (int j = 0; j < kNumElems; ++j) {
      field[j] = j;
    }
  }

  struct MockNode : internal::NodeBase {
    std::pair<int, int> v;
  };
  size_t expected =
      values.size() * (MapTestPeer::NumBuckets(*values[0]) * sizeof(void*) +
                       values[0]->size() * sizeof(MockNode));
  // Use a 2% slack for other overhead. If we were not reusing the blocks, the
  // actual value would be ~2x the cost of the bucket array.
  EXPECT_THAT(arena.SpaceUsed(), AllOf(Ge(expected), Le(1.02 * expected)));
}

// We changed the internal implementation to use a smaller size type, but the
// public API will still use size_t to avoid changing the API. Test that.
TEST(MapTest, SizeTypeIsSizeT) {
  using M = Map<int, int>;
  EXPECT_TRUE((std::is_same<M::size_type, size_t>::value));
  EXPECT_TRUE((std::is_same<decltype(M().size()), size_t>::value));
  size_t x = 0;
  x = std::max(M().size(), x);
  (void)x;
}

TEST(MapTest, IteratorNodeFieldIsNullPtrAtEnd) {
  Map<int, int> map;
  EXPECT_EQ(internal::UntypedMapIterator::FromTyped(map.cbegin()).node_,
            nullptr);
  map.insert({1, 1});
  // This behavior is depended on by Rust FFI.
  EXPECT_NE(internal::UntypedMapIterator::FromTyped(map.cbegin()).node_,
            nullptr);
  EXPECT_EQ(internal::UntypedMapIterator::FromTyped(map.cend()).node_, nullptr);
}

template <typename Aligned, bool on_arena = false>
void MapTest_Aligned() {
  Arena arena;
  constexpr size_t align_mask = alignof(Aligned) - 1;
  Map<int, Aligned> map(on_arena ? &arena : nullptr);
  map.insert({1, Aligned{}});
  auto it = map.find(1);
  ASSERT_NE(it, map.end());
  ASSERT_EQ(reinterpret_cast<intptr_t>(&it->second) & align_mask, 0);
  map.clear();
}

TEST(MapTest, Aligned) { MapTest_Aligned<AlignedAsDefault>(); }
TEST(MapTest, AlignedOnArena) { MapTest_Aligned<AlignedAsDefault, true>(); }
TEST(MapTest, Aligned8) { MapTest_Aligned<AlignedAs8>(); }
TEST(MapTest, Aligned8OnArena) { MapTest_Aligned<AlignedAs8, true>(); }



}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
