// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/btree_split.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"
#include "google/protobuf/raw_ptr.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/testing/protobuf_test.h"
#include "google/protobuf/unittest.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::proto2_unittest::TestEmptyMessage;
using Node = BtreeSplit::Node;
using Bytes8 = Node::Bytes8;
using Bytes4 = Node::Bytes4;

struct ArenaAwarePayload {
  using InternalArenaConstructable_ = void;

  ArenaAwarePayload() = default;
  explicit ArenaAwarePayload(Arena* arena) : arena(arena) {}

  Arena* arena = nullptr;
  uint64_t value = 0;
};

struct RawPayload {
  uint64_t value = 0;
};

enum class AccessMode { kTyped, kUntyped };

template <AccessMode mode, typename T, int offset, int... hops>
void CheckAddressRoundtrip(BtreeSplit& split,
                           BtreeSplitTypedAddress<T, offset, hops...> typed,
                           TestEmptyMessage* msg, const Node* def_root, T val) {
  constexpr std::array<size_t, sizeof...(hops)> kHops = {
      static_cast<size_t>(hops)...};
  constexpr BtreeSplitAddress kAddr(absl::Span<const size_t>(kHops), offset);
  ASSERT_EQ(kAddr.bits(), typed.ReflectionBits());
  ASSERT_EQ(kAddr.bits() & ~BtreeSplitAddress::kAddressMask, 0u);
  auto compact = kAddr.generator();
  auto expanded = typed.expanded().generator();
  for (size_t hop : kHops) {
    ASSERT_EQ(compact.Next(), hop);
    ASSERT_EQ(expanded.Next(), hop);
  }
  ASSERT_EQ(compact.Next(), absl::nullopt);
  ASSERT_EQ(expanded.Next(), absl::nullopt);
  ASSERT_EQ(compact.Offset(), static_cast<uint32_t>(offset));
  ASSERT_EQ(expanded.Offset(), static_cast<uint32_t>(offset));

  if constexpr (mode == AccessMode::kTyped) {
    split.Mutable(typed, msg, def_root) = val;
  } else {
    *static_cast<T*>(split.MutableUntyped(
        BtreeSplitAddress(typed.ReflectionBits()), msg, def_root)) = val;
  }
  EXPECT_EQ(split.Get(typed), val);
  EXPECT_EQ(*static_cast<const T*>(split.GetUntyped(kAddr)), val);
}

PB_TEST(BtreeSplitTest, Traits) {
  EXPECT_TRUE(std::is_trivially_copyable_v<Node>);
  EXPECT_TRUE(std::is_trivially_destructible_v<Node>);
}

PB_TEST(BtreeSplitTest, NodeZeroAndConstexprConstruction) {
  std::array<char, sizeof(Node)> zeros{};
  EXPECT_EQ(std::memcmp(&BtreeSplit::kZeroNode, zeros.data(), zeros.size()), 0);
  for (size_t i = 0; i < Node::kNumSlots; ++i) {
    EXPECT_EQ(BtreeSplit::kZeroNode.sub(i), nullptr);
  }

  static constexpr int kTarget = 42;
  static constexpr Node kLeaf{
      Bytes8{
          Bytes4{uint32_t{0x12345678}},
          Bytes4{int32_t{-7}},
      },  // 0
      Bytes8{
          Bytes4{1.5f},
          Bytes4{true, false, true, false},
      },                                     // 1
      Bytes8{uint64_t{0xfeedfacecafebeef}},  // 2
      Bytes8{int64_t{-9876543210}},          // 3
      Bytes8{3.25},                          // 4
      Bytes8{&kTarget},                      // 5
      Bytes8{nullptr},                       // 6
  };
  static constexpr Node kRoot(&kLeaf);

  EXPECT_EQ(kRoot.sub(0), &kLeaf);
  for (size_t i = 1; i < Node::kNumSlots; ++i) {
    EXPECT_EQ(kRoot.sub(i), nullptr);
  }

  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<uint32_t, 0>{}), 0x12345678u);
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<int32_t, 4>{}), -7);
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<float, 8>{}), 1.5f);
  EXPECT_TRUE((kLeaf.Get(BtreeSplitTypedAddress<bool, 12>{})));
  EXPECT_FALSE((kLeaf.Get(BtreeSplitTypedAddress<bool, 13>{})));
  EXPECT_TRUE((kLeaf.Get(BtreeSplitTypedAddress<bool, 14>{})));
  EXPECT_FALSE((kLeaf.Get(BtreeSplitTypedAddress<bool, 15>{})));
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<uint64_t, 16>{}),
            0xfeedfacecafebeefu);
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<int64_t, 24>{}), -9876543210);
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<double, 32>{}), 3.25);
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<const void*, 40>{}), &kTarget);
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<const void*, 48>{}), nullptr);
  EXPECT_EQ(kLeaf.Get(BtreeSplitTypedAddress<uint64_t, 56>{}), 0u);
}

PB_TEST(BtreeSplitTest, NodeMutableAndRawPtrAccess) {
  Node children[Node::kNumSlots]{};
  Node node(&children[0], &children[1], &children[2], &children[3],
            &children[4], &children[5], &children[6], &children[7]);
  for (size_t i = 0; i < Node::kNumSlots; ++i) {
    EXPECT_EQ(node.sub(i), &children[i]);
    EXPECT_EQ(std::as_const(node).sub(i), &children[i]);
  }

  // Packed bool mutations must not clobber neighboring bytes, including the
  // last byte of a 64-byte node; 4B/8B boundary offsets must round-trip
  // cleanly.
  Node leaf{};
  constexpr BtreeSplitTypedAddress<bool, 0> kB0;
  constexpr BtreeSplitTypedAddress<bool, 1> kB1;
  constexpr BtreeSplitTypedAddress<bool, 63> kBLast;
  leaf.Mutable(kB0) = true;
  leaf.Mutable(kBLast) = true;
  EXPECT_TRUE(std::as_const(leaf).Get(kB0));
  EXPECT_FALSE(std::as_const(leaf).Get(kB1));
  EXPECT_TRUE(std::as_const(leaf).Get(kBLast));

  constexpr BtreeSplitTypedAddress<uint32_t, 60> kU32End;
  leaf.Mutable(kU32End) = 0xabcdef01;
  EXPECT_EQ(std::as_const(leaf).Get(kU32End), 0xabcdef01u);

  constexpr BtreeSplitTypedAddress<uint64_t, 56> kU64End;
  leaf.Mutable(kU64End) = 0x0123456789abcdef;
  EXPECT_EQ(std::as_const(leaf).Get(kU64End), 0x0123456789abcdefu);

  // Node::Mutable exposes RawPtr<T>& while Node::Get unpacks to const T&.
  constexpr BtreeSplitTypedAddress<RawPtr<RawPayload>, 8> kRawAddr;
  static_assert(
      std::is_same_v<decltype(leaf.Mutable(kRawAddr)), RawPtr<RawPayload>&>);
  static_assert(std::is_same_v<decltype(std::as_const(leaf).Get(kRawAddr)),
                               const RawPayload&>);

  Node raw_node(Bytes8{}, Bytes8(DefaultRawPtr()));
  EXPECT_EQ(std::as_const(raw_node).Get(kRawAddr).value, 0u);
  RawPayload payload{42};
  raw_node.Mutable(kRawAddr).Set(&payload);
  EXPECT_EQ(&std::as_const(raw_node).Get(kRawAddr), &payload);
  EXPECT_EQ(std::as_const(raw_node).Get(kRawAddr).value, 42u);
}

PB_TEST(BtreeSplitTest, AddressRoundtripAcrossDepths) {
  constexpr BtreeSplitTypedAddress<uint32_t, 4, 2, 5> kTyped;
  constexpr uint64_t kExpandedBytes = kTyped.expanded().bytes();
  static_assert(kExpandedBytes != 0);

  Arena arena;
  auto* msg = Arena::Create<TestEmptyMessage>(&arena);

  Node def_nodes[8]{};
  def_nodes[0].slots[7] = Bytes8(&def_nodes[1]);
  def_nodes[1].slots[0] = Bytes8(&def_nodes[2]);
  def_nodes[2].slots[5] = Bytes8(&def_nodes[3]);
  def_nodes[3].slots[2] = Bytes8(&def_nodes[4]);
  def_nodes[4].slots[7] = Bytes8(&def_nodes[5]);
  def_nodes[5].slots[1] = Bytes8(&def_nodes[6]);
  def_nodes[5].slots[6] = Bytes8(&def_nodes[7]);
  BtreeSplit split(&def_nodes[0]);

  constexpr BtreeSplitTypedAddress<uint64_t, 48> kDepth0;
  constexpr BtreeSplitTypedAddress<uint32_t, 60, 7> kDepth1;
  constexpr BtreeSplitTypedAddress<uint8_t, 0, 7, 0, 5, 2, 7, 1> kDepth6Start;
  constexpr BtreeSplitTypedAddress<uint8_t, 63, 7, 0, 5, 2, 7, 1> kDepth6End;
  constexpr BtreeSplitTypedAddress<uint8_t, 63, 7, 0, 5, 2, 7, 6> kDepth6Alt;

  CheckAddressRoundtrip<AccessMode::kTyped>(split, kDepth0, msg, &def_nodes[0],
                                            uint64_t{101});
  CheckAddressRoundtrip<AccessMode::kUntyped>(split, kDepth0, msg,
                                              &def_nodes[0], uint64_t{202});
  CheckAddressRoundtrip<AccessMode::kTyped>(split, kDepth1, msg, &def_nodes[0],
                                            uint32_t{303});
  CheckAddressRoundtrip<AccessMode::kUntyped>(split, kDepth1, msg,
                                              &def_nodes[0], uint32_t{404});
  CheckAddressRoundtrip<AccessMode::kTyped>(split, kDepth6Start, msg,
                                            &def_nodes[0], uint8_t{0x11});
  CheckAddressRoundtrip<AccessMode::kUntyped>(split, kDepth6Start, msg,
                                              &def_nodes[0], uint8_t{0x22});
  CheckAddressRoundtrip<AccessMode::kTyped>(split, kDepth6End, msg,
                                            &def_nodes[0], uint8_t{0x33});
  CheckAddressRoundtrip<AccessMode::kUntyped>(split, kDepth6End, msg,
                                              &def_nodes[0], uint8_t{0x44});
  CheckAddressRoundtrip<AccessMode::kTyped>(split, kDepth6Alt, msg,
                                            &def_nodes[0], uint8_t{0x55});
  CheckAddressRoundtrip<AccessMode::kUntyped>(split, kDepth6Alt, msg,
                                              &def_nodes[0], uint8_t{0x66});

  EXPECT_EQ(split.Get(kDepth0), 202u);
  EXPECT_EQ(split.Get(kDepth1), 404u);
  EXPECT_EQ(split.Get(kDepth6Start), 0x22);
  EXPECT_EQ(split.Get(kDepth6End), 0x44);
  EXPECT_EQ(split.Get(kDepth6Alt), 0x66);
}

PB_TEST(BtreeSplitTest, ZeroNodeDefaultRoot) {
  PROTOBUF_CONSTINIT static const BtreeSplit kDefaultZero(
      &BtreeSplit::kZeroNode);
  constexpr BtreeSplitTypedAddress<uint32_t, 0> kField;
  constexpr BtreeSplitTypedAddress<bool, 4> kFlag;

  Arena arena;
  auto* msg = Arena::Create<TestEmptyMessage>(&arena);
  BtreeSplit split = kDefaultZero;

  EXPECT_EQ(split.TryMutable(kField, &BtreeSplit::kZeroNode), nullptr);
  EXPECT_EQ(split.Get(kField), 0u);
  EXPECT_FALSE(split.Get(kFlag));

  split.Mutable(kField, msg, &BtreeSplit::kZeroNode) = 99;
  EXPECT_NE(split.head(), &BtreeSplit::kZeroNode);
  EXPECT_EQ(split.Get(kField), 99u);
  EXPECT_FALSE(split.Get(kFlag));
  EXPECT_NE(split.TryMutable(kField, &BtreeSplit::kZeroNode), nullptr);
  EXPECT_EQ(kDefaultZero.Get(kField), 0u);
}

PB_TEST(BtreeSplitTest, CopyOnWriteStructuralSharing) {
  const bool use_untyped =
      PB_TEST_CHOOSE_VALUE((false, true), {"typed", "untyped"});

  PROTOBUF_CONSTINIT static const Node def_leaf0{
      Bytes8{
          Bytes4{uint32_t{10}},
          Bytes4{uint32_t{20}},
      },
      Bytes8{int64_t{-30}},
  };
  PROTOBUF_CONSTINIT static const Node def_leaf1{
      Bytes8{
          Bytes4{uint32_t{40}},
          Bytes4{},
      },
  };
  PROTOBUF_CONSTINIT static const Node def_leaf2{Bytes8{uint64_t{50}}};
  PROTOBUF_CONSTINIT static const Node def_mid0{
      &def_leaf0,
      &def_leaf1,
      &BtreeSplit::kZeroNode,
  };
  PROTOBUF_CONSTINIT static const Node def_mid1{&def_leaf2};
  PROTOBUF_CONSTINIT static const Node def_root{
      &def_mid0,
      &def_mid1,
      &BtreeSplit::kZeroNode,
      Bytes8{},
      Bytes8{},
      Bytes8{},
      Bytes8{},
      Bytes8{
          Bytes4{true, false, false, false},
          Bytes4{},
      },
  };

  constexpr BtreeSplitTypedAddress<uint32_t, 0, 0, 0> kLeaf0A;
  constexpr BtreeSplitTypedAddress<uint32_t, 4, 0, 0> kLeaf0B;
  constexpr BtreeSplitTypedAddress<int64_t, 8, 0, 0> kLeaf0C;
  constexpr BtreeSplitTypedAddress<uint32_t, 0, 0, 1> kLeaf1;
  constexpr BtreeSplitTypedAddress<uint32_t, 0, 0, 2> kMid0Zero;
  constexpr BtreeSplitTypedAddress<uint64_t, 0, 1, 0> kLeaf2;
  constexpr BtreeSplitTypedAddress<bool, 56> kRootFlag;

  Arena arena;
  auto* msg = Arena::Create<TestEmptyMessage>(&arena);
  BtreeSplit split(&def_root);

  const auto read_both = [&](auto addr) {
    using T = typename decltype(addr)::value_type;
    const T& typed = split.Get(addr);
    const T& untyped = *static_cast<const T*>(
        split.GetUntyped(BtreeSplitAddress(addr.ReflectionBits())));
    EXPECT_EQ(typed, untyped);
    return typed;
  };
  const auto write_field = [&](auto addr, auto value) {
    using T = typename decltype(addr)::value_type;
    if (use_untyped) {
      *static_cast<T*>(split.MutableUntyped(
          BtreeSplitAddress(addr.ReflectionBits()), msg, &def_root)) = value;
    } else {
      split.Mutable(addr, msg, &def_root) = value;
    }
  };

  // All reads start at defaults with no mutable paths.
  EXPECT_EQ(read_both(kLeaf0A), 10u);
  EXPECT_EQ(read_both(kLeaf1), 40u);
  EXPECT_EQ(read_both(kMid0Zero), 0u);
  EXPECT_EQ(read_both(kLeaf2), 50u);
  EXPECT_TRUE(read_both(kRootFlag));
  EXPECT_EQ(split.TryMutable(kLeaf0A, &def_root), nullptr);
  EXPECT_EQ(split.TryMutable(kRootFlag, &def_root), nullptr);

  // Mutate leaf0: clones root -> mid0 -> leaf0, sharing sibling subtrees.
  write_field(kLeaf0A, uint32_t{111});
  Node* owned_root = split.head();
  Node* owned_mid0 = owned_root->sub(0);
  Node* owned_leaf0 = owned_mid0->sub(0);
  EXPECT_NE(owned_root, &def_root);
  EXPECT_NE(owned_mid0, &def_mid0);
  EXPECT_NE(owned_leaf0, &def_leaf0);
  EXPECT_EQ(owned_root->sub(1), &def_mid1);
  EXPECT_EQ(owned_root->sub(2), &BtreeSplit::kZeroNode);
  EXPECT_EQ(owned_mid0->sub(1), &def_leaf1);
  EXPECT_EQ(owned_mid0->sub(2), &BtreeSplit::kZeroNode);
  EXPECT_NE(split.TryMutable(kLeaf0B, &def_root), nullptr);
  EXPECT_NE(split.TryMutable(kRootFlag, &def_root), nullptr);
  EXPECT_EQ(split.TryMutable(kLeaf1, &def_root), nullptr);
  EXPECT_EQ(split.TryMutable(kMid0Zero, &def_root), nullptr);
  EXPECT_EQ(split.TryMutable(kLeaf2, &def_root), nullptr);
  EXPECT_EQ(read_both(kLeaf0B), 20u);
  EXPECT_EQ(read_both(kLeaf0C), -30);
  EXPECT_TRUE(read_both(kRootFlag));

  // Mutating a sibling in leaf0 reuses existing owned nodes.
  write_field(kLeaf0B, uint32_t{222});
  EXPECT_EQ(split.head(), owned_root);
  EXPECT_EQ(owned_root->sub(0), owned_mid0);
  EXPECT_EQ(owned_mid0->sub(0), owned_leaf0);

  // Mutate leaf1 and mid0's kZeroNode slot: reuses root and mid0.
  write_field(kLeaf1, uint32_t{444});
  EXPECT_EQ(split.head(), owned_root);
  EXPECT_EQ(owned_root->sub(0), owned_mid0);
  EXPECT_NE(owned_mid0->sub(1), &def_leaf1);
  EXPECT_EQ(owned_root->sub(1), &def_mid1);

  write_field(kMid0Zero, uint32_t{333});
  EXPECT_EQ(split.head(), owned_root);
  EXPECT_EQ(owned_root->sub(0), owned_mid0);
  EXPECT_NE(owned_mid0->sub(2), &BtreeSplit::kZeroNode);

  // Mutate leaf2: reuses root, clones mid1 -> leaf2.
  write_field(kLeaf2, uint64_t{555});
  EXPECT_EQ(split.head(), owned_root);
  EXPECT_NE(owned_root->sub(1), &def_mid1);
  EXPECT_NE(owned_root->sub(1)->sub(0), &def_leaf2);

  EXPECT_EQ(read_both(kLeaf0A), 111u);
  EXPECT_EQ(read_both(kLeaf0B), 222u);
  EXPECT_EQ(read_both(kLeaf0C), -30);
  EXPECT_EQ(read_both(kLeaf1), 444u);
  EXPECT_EQ(read_both(kMid0Zero), 333u);
  EXPECT_EQ(read_both(kLeaf2), 555u);
  EXPECT_EQ(def_leaf0.Get(BtreeSplitTypedAddress<uint32_t, 0>{}), 10u);
}

PB_TEST(BtreeSplitTest, RawPtrArenaAndHeapForwarding) {
  Arena arena;
  Arena* target_arena =
      PB_TEST_CHOOSE_VALUE((nullptr, &arena), {"heap", "arena"});

  auto* msg = Arena::Create<TestEmptyMessage>(target_arena);
  constexpr BtreeSplitTypedAddress<uint32_t, 0> kScalar;
  constexpr BtreeSplitTypedAddress<RawPtr<ArenaAwarePayload>, 8> kPayload;
  PROTOBUF_CONSTINIT static const Node def_root{
      Bytes8{
          Bytes4{uint32_t{7}},
          Bytes4{uint32_t{0}},
      },
      Bytes8{DefaultRawPtr()},
  };
  BtreeSplit split(&def_root);

  EXPECT_EQ(split.TryMutable(kPayload, &def_root), nullptr);
  EXPECT_EQ(split.Get(kPayload).arena, nullptr);
  EXPECT_EQ(split.Get(kPayload).value, 0u);

  // Cloning the node via a sibling scalar leaves RawPtr default (TryMutable
  // remains null, and MutableUntyped does not allocate the payload).
  *static_cast<uint32_t*>(split.MutableUntyped(
      BtreeSplitAddress(kScalar.ReflectionBits()), msg, &def_root)) = 19;
  EXPECT_NE(split.head(), &def_root);
  EXPECT_EQ(split.Get(kScalar), 19u);
  EXPECT_EQ(split.TryMutable(kPayload, &def_root), nullptr);
  EXPECT_TRUE(
      static_cast<RawPtr<ArenaAwarePayload>*>(
          split.MutableUntyped(BtreeSplitAddress(kPayload.ReflectionBits()),
                               msg, &def_root))
          ->IsDefault());

  // Typed Mutable allocates ArenaAwarePayload forwarding GetArena(msg), and
  // subsequent Mutable/TryMutable calls reuse the same allocation.
  Node* owned_root = split.head();
  ArenaAwarePayload& payload = split.Mutable(kPayload, msg, &def_root);
  EXPECT_EQ(split.head(), owned_root);
  EXPECT_EQ(payload.arena, target_arena);
  payload.value = 42;
  EXPECT_EQ(&split.Get(kPayload), &payload);
  EXPECT_EQ(split.TryMutable(kPayload, &def_root), &payload);
  EXPECT_EQ(&split.Mutable(kPayload, msg, &def_root), &payload);
  EXPECT_EQ(split.Get(kPayload).value, 42u);

  // Typed Mutable also handles unowned intermediate/leaf nodes directly from
  // the default tree, cloning nodes and allocating the RawPtr payload together.
  PROTOBUF_CONSTINIT static const Node def_parent(&def_root);
  constexpr BtreeSplitTypedAddress<RawPtr<ArenaAwarePayload>, 8, 0>
      kChildPayload;
  BtreeSplit split_from_default(&def_parent);
  ArenaAwarePayload& child_payload =
      split_from_default.Mutable(kChildPayload, msg, &def_parent);
  EXPECT_NE(split_from_default.head(), &def_parent);
  EXPECT_NE(split_from_default.head()->sub(0), &def_root);
  EXPECT_EQ(child_payload.arena, target_arena);
  child_payload.value = 99;
  EXPECT_EQ(&split_from_default.Get(kChildPayload), &child_payload);
  EXPECT_EQ(split_from_default.TryMutable(kChildPayload, &def_parent),
            &child_payload);
  EXPECT_EQ(split_from_default.Get(kChildPayload).value, 99u);

  if (target_arena == nullptr) {
    if (ArenaAwarePayload* allocated = split.TryMutable(kPayload, &def_root)) {
      delete allocated;
    }
    if (split.head() != &def_root) {
      delete split.head();
    }
    if (ArenaAwarePayload* allocated =
            split_from_default.TryMutable(kChildPayload, &def_parent)) {
      delete allocated;
    }
    if (split_from_default.head() != &def_parent) {
      if (split_from_default.head()->sub(0) != &def_root) {
        delete split_from_default.head()->sub(0);
      }
      delete split_from_default.head();
    }
    delete msg;
  }
}

PB_TEST(BtreeSplitTest, NodeWithDefaultAndConstNodeWithDefault) {
  PROTOBUF_CONSTINIT static const Node def_leaf(Bytes8(uint64_t{1}));
  PROTOBUF_CONSTINIT static const Node def_root(&def_leaf,
                                                &BtreeSplit::kZeroNode);

  Node owned_leaf(Bytes8(uint64_t{2}));
  Node owned_root(&owned_leaf, &BtreeSplit::kZeroNode);

  const auto check_pair = [&](Node* root_ptr, bool expect_root_owned,
                              bool expect_slot0_owned,
                              bool expect_slot1_owned) {
    const auto check_one = [&](auto root) {
      EXPECT_EQ(root.is_default(), !expect_root_owned);
      EXPECT_EQ(root.sub(0).is_default(), !expect_slot0_owned);
      EXPECT_EQ(root.sub(1).is_default(), !expect_slot1_owned);
    };
    check_one(BtreeSplit::ConstNodeWithDefault{root_ptr, &def_root});
    check_one(BtreeSplit::NodeWithDefault{root_ptr, &def_root});
  };

  check_pair(const_cast<Node*>(&def_root), false, false, false);

  Node partial_root(&def_leaf, &BtreeSplit::kZeroNode);
  check_pair(&partial_root, true, false, false);

  check_pair(&owned_root, true, true, false);

  Node owned_zero_child(Bytes8(uint64_t{3}));
  owned_root.slots[1] = Bytes8(&owned_zero_child);
  check_pair(&owned_root, true, true, true);
}

}  // namespace

// Some codegen thunks that we can use to dump the asm via lldb.
uint32_t CodegenBtreeSplitGetScalar(const BtreeSplit& split) {
  return split.Get(BtreeSplitTypedAddress<uint32_t, 8, 1, 2>{});
}

const RepeatedField<int32_t>& CodegenBtreeSplitGetRepeated(
    const BtreeSplit& split) {
  return split.Get(
      BtreeSplitTypedAddress<RawPtr<RepeatedField<int32_t>>, 8, 1, 2>{});
}

uint32_t* CodegenBtreeSplitTryMutableScalar(
    BtreeSplit& split, const BtreeSplit::Node* default_node) {
  return split.TryMutable(BtreeSplitTypedAddress<uint32_t, 8, 1, 2>{},
                          default_node);
}

RepeatedField<int32_t>* CodegenBtreeSplitTryMutableRepeated(
    BtreeSplit& split, const BtreeSplit::Node* default_node) {
  return split.TryMutable(
      BtreeSplitTypedAddress<RawPtr<RepeatedField<int32_t>>, 8, 1, 2>{},
      default_node);
}

uint32_t& CodegenBtreeSplitMutableDepth0(BtreeSplit& split,
                                         const MessageLite* msg,
                                         const BtreeSplit::Node* default_node) {
  return split.Mutable(BtreeSplitTypedAddress<uint32_t, 8>{}, msg,
                       default_node);
}

uint32_t& CodegenBtreeSplitMutableScalar(BtreeSplit& split,
                                         const MessageLite* msg,
                                         const BtreeSplit::Node* default_node) {
  return split.Mutable(BtreeSplitTypedAddress<uint32_t, 8, 1, 2>{}, msg,
                       default_node);
}

RepeatedField<int32_t>& CodegenBtreeSplitMutableRepeated(
    BtreeSplit& split, const MessageLite* msg,
    const BtreeSplit::Node* default_node) {
  return split.Mutable(
      BtreeSplitTypedAddress<RawPtr<RepeatedField<int32_t>>, 8, 1, 2>{}, msg,
      default_node);
}

static int odr [[maybe_unused]] =
    (StrongPointer(&CodegenBtreeSplitGetScalar),
     StrongPointer(&CodegenBtreeSplitGetRepeated),
     StrongPointer(&CodegenBtreeSplitTryMutableScalar),
     StrongPointer(&CodegenBtreeSplitTryMutableRepeated),
     StrongPointer(&CodegenBtreeSplitMutableDepth0),
     StrongPointer(&CodegenBtreeSplitMutableScalar),
     StrongPointer(&CodegenBtreeSplitMutableRepeated), 0);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
