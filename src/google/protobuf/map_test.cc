// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <array>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "google/protobuf/arena_test_util.h"
#include "google/protobuf/internal_visibility_for_testing.h"
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

using ::testing::FieldsAre;
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
