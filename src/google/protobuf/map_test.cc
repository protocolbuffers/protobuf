// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/map.h"

#include <algorithm>
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
using ::testing::UnorderedElementsAreArray;


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

TEST(MapTest, CopyConstructionMaintainsProperLoadFactor) {
  Map<int, int> original;
  for (size_t size = 1; size < 50; ++size) {
    // Add one element.
    original[size];
    Map<int, int> copy = original;
    ASSERT_THAT(copy, UnorderedElementsAreArray(original));
    EXPECT_LE(copy.size(),
              MapTestPeer::CalculateHiCutoff(MapTestPeer::NumBuckets(copy)));
  }
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

template <typename F, typename... Key, typename... Value>
void TestAllKeyValueTypes(void (*)(Key...), void (*)(Value...), F f) {
  (
      [f]() {
        using K = Key;
        (f(K{}, Value{}), ...);
      }(),
      ...);
}

using KeyTypes = void (*)(bool, int32_t, uint32_t, int64_t, uint64_t,
                          std::string);
// Some arbitrary proto enum.
using SomeEnum = protobuf_unittest::TestAllTypes::NestedEnum;
using ValueTypes = void (*)(bool, int32_t, uint32_t, int64_t, uint64_t, float,
                            double, std::string, SomeEnum,
                            protobuf_unittest::TestEmptyMessage,
                            protobuf_unittest::TestAllTypes);

TEST(MapTest, StaticTypeInfoMatchesDynamicOne) {
  TestAllKeyValueTypes(KeyTypes(), ValueTypes(), [](auto key, auto value) {
    using Key = decltype(key);
    using Value = decltype(value);
    const MessageLite* value_prototype = nullptr;
    if constexpr (std::is_base_of_v<MessageLite, Value>) {
      value_prototype = &Value::default_instance();
    }
    const auto type_info = MapTestPeer::GetTypeInfo<Map<Key, Value>>();
    const auto dyn_type_info = internal::UntypedMapBase::GetTypeInfoDynamic(
        type_info.key_type, type_info.value_type, value_prototype);
    EXPECT_EQ(dyn_type_info.node_size, type_info.node_size);
    EXPECT_EQ(dyn_type_info.value_offset, type_info.value_offset);
    EXPECT_EQ(dyn_type_info.key_type, type_info.key_type);
    EXPECT_EQ(dyn_type_info.value_type, type_info.value_type);
  });
}

TEST(MapTest, StaticTypeKindWorks) {
  using UMB = UntypedMapBase;
  EXPECT_EQ(UMB::TypeKind::kBool, UMB::StaticTypeKind<bool>());
  EXPECT_EQ(UMB::TypeKind::kU32, UMB::StaticTypeKind<int32_t>());
  EXPECT_EQ(UMB::TypeKind::kU32, UMB::StaticTypeKind<uint32_t>());
  EXPECT_EQ(UMB::TypeKind::kU32, UMB::StaticTypeKind<SomeEnum>());
  EXPECT_EQ(UMB::TypeKind::kU64, UMB::StaticTypeKind<int64_t>());
  EXPECT_EQ(UMB::TypeKind::kU64, UMB::StaticTypeKind<uint64_t>());
  EXPECT_EQ(UMB::TypeKind::kString, UMB::StaticTypeKind<std::string>());
  EXPECT_EQ(UMB::TypeKind::kMessage,
            UMB::StaticTypeKind<protobuf_unittest::TestAllTypes>());
  EXPECT_EQ(UMB::TypeKind::kUnknown, UMB::StaticTypeKind<void**>());
}

template <typename LHS, typename RHS>
void TestEqPtr(LHS* lhs, RHS* rhs) {
  // To silence some false positive compiler errors in gcc 9.5
  (void)lhs;
  (void)rhs;
  if constexpr (std::is_integral_v<LHS> && std::is_signed_v<LHS>) {
    // Visitation uses unsigned types always, so fix that here.
    return TestEqPtr(reinterpret_cast<std::make_unsigned_t<LHS>*>(lhs), rhs);
  } else if constexpr (std::is_enum_v<LHS>) {
    // Enums are visited as uint32_t, so check that.
    EXPECT_TRUE((std::is_same_v<uint32_t, RHS>));
    EXPECT_EQ(static_cast<void*>(lhs), static_cast<void*>(rhs));
  } else if constexpr (std::is_same_v<std::decay_t<LHS>, std::decay_t<RHS>> ||
                       std::is_base_of_v<LHS, RHS> ||
                       std::is_base_of_v<RHS, LHS>) {
    EXPECT_EQ(lhs, rhs);
  } else {
    FAIL() << "Wrong types";
  }
}

TEST(MapTest, VistiKeyValueUsesTheRightTypes) {
  TestAllKeyValueTypes(KeyTypes(), ValueTypes(), [](auto key, auto value) {
    using Key = decltype(key);
    using Value = decltype(value);
    Map<Key, Value> map;
    map[Key{}];
    auto& base = reinterpret_cast<UntypedMapBase&>(map);
    NodeBase* node = base.begin().node_;
    // We use a runtime check because all the different overloads are
    // instantiated, but only the right one should be called at runtime.
    int key_result = base.VisitKey(node, [&](auto* k) {
      TestEqPtr(&map.begin()->first, k);
      return 17;
    });
    EXPECT_EQ(key_result, 17);
    int value_result = base.VisitValue(node, [&](auto* v) {
      TestEqPtr(&map.begin()->second, v);
      return 1979;
    });
    EXPECT_EQ(value_result, 1979);
  });
}

TEST(MapTest, VisitAllNodesUsesTheRightTypesOnAllNodes) {
  TestAllKeyValueTypes(KeyTypes(), ValueTypes(), [](auto key, auto value) {
    using Key = decltype(key);
    using Value = decltype(value);
    Map<Key, Value> map;
    for (int i = 0; i < 3; ++i, key += 1) {
      map[key];
    }

    // We should have 3 elements, unless key is bool.
    ASSERT_EQ(map.size(), (std::is_same_v<Key, bool> ? 2 : 3));

    auto it = map.begin();
    auto& base = reinterpret_cast<UntypedMapBase&>(map);
    base.VisitAllNodes([&](auto* key, auto* value) {
      // We use a runtime check because all the different overloads are
      // instantiated, but only the right one should be called at runtime.
      TestEqPtr(&it->first, key);
      TestEqPtr(&it->second, value);
      ++it;
    });
    EXPECT_TRUE(it == map.end());
  });
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
