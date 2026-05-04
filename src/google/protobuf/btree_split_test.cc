// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/btree_split.h"

#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

using testing::FieldsAre;

constexpr auto n1 = BtreeSplit::Node{
    {5, {true, 0, 0, 0}},
};
static_assert(n1.slots[0].b4[0].i32 == 5);
static_assert(n1.slots[0].b4[1].b[0] == true);

constexpr auto n2 = BtreeSplit::Node{
    uint64_t{111},
    {-1, {true, false, 0, 0}},
};

static_assert(n2.slots[0].u64 == 111);
static_assert(n2.slots[1].b4[0].i32 == -1);
static_assert(n2.slots[1].b4[1].b[0] == true);
static_assert(n2.slots[1].b4[1].b[1] == false);

constexpr auto n3 = BtreeSplit::Node{
    &n1,
    &n2,
    {13, -111},
    1.3,
};
static_assert(n3.slots[2].b4[0].i32 == 13);
static_assert(n3.slots[2].b4[1].i32 == -111);
static_assert(n3.slots[3].d == 1.3);

constexpr auto a00 = BtreeSplitTypedAddress<int>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 1>{0}, 0));
constexpr auto a01 = BtreeSplitTypedAddress<bool>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 1>{0}, 4));
constexpr auto a10 = BtreeSplitTypedAddress<uint64_t>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 1>{1}, 0));
constexpr auto a11 = BtreeSplitTypedAddress<int>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 1>{1}, 8));
constexpr auto a12 = BtreeSplitTypedAddress<bool>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 1>{1}, 12));
constexpr auto a13 = BtreeSplitTypedAddress<bool>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 1>{1}, 13));
constexpr auto a0 = BtreeSplitTypedAddress<int>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 0>{}, 16));
constexpr auto a1 = BtreeSplitTypedAddress<int>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 0>{}, 20));
constexpr auto a2 = BtreeSplitTypedAddress<double>(
    BtreeSplitAddress::CalculateBits(std::array<size_t, 0>{}, 24));

auto GatherValues(const BtreeSplit& split) {
  return std::tuple(std::tuple(split.Get(a00), split.Get(a01)),
                    std::tuple(split.Get(a10), split.Get(a11), split.Get(a12),
                               split.Get(a13)),
                    split.Get(a0), split.Get(a1), split.Get(a2));
}

TEST(BtreeSplitTest, ReadOnly) {
  BtreeSplit split(&n3);

  EXPECT_THAT(GatherValues(split),
              FieldsAre(FieldsAre(5, true),               //
                        FieldsAre(111, -1, true, false),  //
                        13, -111, 1.3));
}

TEST(BtreeSplitTest, MutableOneAtATime) {
  Arena arena;
  const auto* def = &n3;
  BtreeSplit split(def);
  ;

  EXPECT_THAT(GatherValues(split),
              FieldsAre(FieldsAre(5, true),               //
                        FieldsAre(111, -1, true, false),  //
                        13, -111, 1.3));

  size_t used = arena.SpaceUsed();
  split.Mutable(a1, &arena, def) = 14;
  // We only copied exactly one node
  EXPECT_EQ(arena.SpaceUsed() - used, 64);

  EXPECT_THAT(GatherValues(split),
              FieldsAre(FieldsAre(5, true),               //
                        FieldsAre(111, -1, true, false),  //
                        13, /*changed*/ 14, 1.3));

  used = arena.SpaceUsed();
  split.Mutable(a01, &arena, def) = false;
  // We only copied exactly one node
  EXPECT_EQ(arena.SpaceUsed() - used, 64);

  EXPECT_THAT(GatherValues(split),
              FieldsAre(FieldsAre(5, /*changed*/ false),  //
                        FieldsAre(111, -1, true, false),  //
                        13, 14, 1.3));
}

TEST(BtreeSplitTest, MutableMultiLevel) {
  Arena arena;
  const auto* def = &n3;
  BtreeSplit split(def);

  EXPECT_THAT(GatherValues(split),
              FieldsAre(FieldsAre(5, true),               //
                        FieldsAre(111, -1, true, false),  //
                        13, -111, 1.3));

  size_t used = arena.SpaceUsed();
  split.Mutable(a11, &arena, def) = -2;
  // We should have copied two nodes.
  EXPECT_EQ(arena.SpaceUsed() - used, 2 * 64);

  EXPECT_THAT(GatherValues(split),
              FieldsAre(FieldsAre(5, true),                           //
                        FieldsAre(111, /*changed*/ -2, true, false),  //
                        13, -111, 1.3));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
