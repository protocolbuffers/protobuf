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
#include "google/protobuf/map_test_util.h"
#include "google/protobuf/map_unittest.pb.h"
#include "google/protobuf/reflection_tester.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"


#define UNITTEST ::proto2_unittest
#define UNITTEST_IMPORT ::proto2_unittest_import
#define UNITTEST_PACKAGE_NAME "proto2_unittest"

// Must include after defining UNITTEST, etc.
// clang-format off
#include "google/protobuf/map_test.inc"
// clang-format on

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
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

TEST(MapTest, CalculateCapacityForSizeTest) {
  for (size_t size = 1; size < 1000; ++size) {
    size_t capacity = MapTestPeer::CalculateCapacityForSize(size);
    // Verify is large enough for `size`.
    EXPECT_LE(size, MapTestPeer::CalculateHiCutoff(capacity));
    if (capacity > MapTestPeer::kMinTableSize) {
      // Verify it's the smallest capacity that's large enough.
      EXPECT_GT(size, MapTestPeer::CalculateHiCutoff(capacity / 2));
    }
  }

  // Verify very large size does not overflow bucket calculation.
  for (size_t size : {0x30000001u, 0x40000000u, 0x50000000u, 0x60000000u,
                      0x70000000u, 0x80000000u, 0x90000000u, 0xFFFFFFFFu}) {
    EXPECT_EQ(0x80000000u, MapTestPeer::CalculateCapacityForSize(size));
  }
}

TEST(MapTest, AlwaysSerializesBothEntries) {
  for (const Message* prototype :
       {static_cast<const Message*>(
            &proto2_unittest::TestI32StrMap::default_instance()),
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

TEST(MapTest, ErasingEnoughCausesDownwardRehashOnNextInsert) {
  for (size_t capacity = 1; capacity < 1000; capacity *= 2) {
    const size_t max_size = MapTestPeer::CalculateHiCutoff(capacity);
    for (size_t min_size = 1; min_size < max_size / 4; ++min_size) {
      Map<int, int> m;
      while (m.size() < max_size) m[m.size()];
      const size_t num_buckets = MapTestPeer::NumBuckets(m);
      while (m.size() > min_size) m.erase(m.size() - 1);
      // Erasing doesn't shrink the table.
      ASSERT_EQ(num_buckets, MapTestPeer::NumBuckets(m));
      // This insertion causes a shrinking rehash because the load factor is too
      // low.
      m[99999];
      size_t new_num_buckets = MapTestPeer::NumBuckets(m);
      EXPECT_LT(new_num_buckets, num_buckets);
      EXPECT_LE(m.size(),
                MapTestPeer::CalculateHiCutoff(MapTestPeer::NumBuckets(m)));
    }
  }
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

using KeyTypes = void (*)(bool, int32_t, uint32_t, int64_t, uint64_t,
                          std::string);
// Some arbitrary proto enum.
using SomeEnum = proto2_unittest::TestAllTypes::NestedEnum;
using ValueTypes = void (*)(bool, int32_t, uint32_t, int64_t, uint64_t, float,
                            double, std::string, SomeEnum,
                            proto2_unittest::TestEmptyMessage,
                            proto2_unittest::TestAllTypes);

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
            UMB::StaticTypeKind<proto2_unittest::TestAllTypes>());
}

#if !defined(__GNUC__) || defined(__clang__) || PROTOBUF_GNUC_MIN(9, 4)
// Parameter pack expansion bug before GCC 8.2:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85305

template <typename F, typename... Key, typename... Value>
void TestAllKeyValueTypes(void (*)(Key...), void (*)(Value...), F f) {
  (
      [f]() {
        using K = Key;
        (f(K{}, Value{}), ...);
      }(),
      ...);
}

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
        type_info.key_type_kind(), type_info.value_type_kind(),
        value_prototype);
    EXPECT_EQ(dyn_type_info.node_size, type_info.node_size);
    EXPECT_EQ(dyn_type_info.value_offset, type_info.value_offset);
    EXPECT_EQ(dyn_type_info.key_type, type_info.key_type);
    EXPECT_EQ(dyn_type_info.value_type, type_info.value_type);
  });
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

#endif

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



}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
