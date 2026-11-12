// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Out-of-line cold-field storage for messages ("Split V2").
//
// Split fields live in an 8-way tree of 64-byte `Node`s (eight 8-byte slots
// each). Internal slots point to child `Node*`s; leaf slots store field
// payloads or 4-byte/1-byte fields packed by protoc. Unmodified subtrees point
// into read-only default nodes (`kZeroNode` or generated `kSplitNode[]`), and
// writes copy-on-write only the nodes along the accessed path onto the
// message's `Arena` (or heap if arena-less).
//
// Three address types reference a field at path `(hops..., offset)`:
//  - `BtreeSplitTypedAddress<T, offset, hops...>`: zero-size compile-time tag
//    used in generated `Impl_` classes for inlined
//    `Get`/`TryMutable`/`Mutable`.
//  - `BtreeSplitAddress`: compact 32-bit encoding stored in Reflection and
//    TcParser offset words (leaves top 3 bits 29..31 free for
//    `kAllOffsetTags`). Laid out LSB-first for cheap sequential shifting:
//      [ num_hops (3b) | hop_0 (3b) | ... | hop_{n-1} (3b) | offset (6b) ]
//  - `BtreeSplitExpandedAddress`: 64-bit byte-per-component encoding used for
//    out-of-line `MutableExpanded` calls from codegen:
//      [ num_hops (1B) | hop_0 (1B) | ... | hop_{n-1} (1B) | offset (1B) ]

#ifndef GOOGLE_PROTOBUF_BTREE_SPLIT_H__
#define GOOGLE_PROTOBUF_BTREE_SPLIT_H__

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/memory_align.h"
#include "google/protobuf/raw_ptr.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class MessageLite;

namespace internal {

template <typename T, int offset, int... hops>
class BtreeSplitTypedAddress;

// Compact 32-bit tree address fit for Reflection/TcParser offset tables.
// Immutable value type.
class BtreeSplitAddress {
 public:
  explicit constexpr BtreeSplitAddress(uint32_t bits) : bits_(bits) {}
  explicit constexpr BtreeSplitAddress(absl::Span<const size_t> hops,
                                       size_t offset);

  // Number of bits used to encode the hop count (depth 0..kMaxDepth <= 7).
  static constexpr size_t kHopsBits = 3;
  // The node is 64 bytes, so offsets within it are at most 2^6.
  static constexpr size_t kChunkOffsetBits = 6;
  // And it has 8 sub Node*, so 3 bits per level.
  static constexpr size_t kChunkPointersBits = 3;
  // Top 3 bits (29..31) are reserved for Reflection offset tags
  // (`kAllOffsetTags` in generated_message_reflection.h). Keep headroom below
  // those tag bits; max depth is 6 in both 28-bit and 29-bit budgets.
  static constexpr size_t kMaxDepth =
      (28 - kHopsBits - kChunkOffsetBits) / kChunkPointersBits;
  static constexpr size_t kMaxAddressBits =
      kHopsBits + kMaxDepth * kChunkPointersBits + kChunkOffsetBits;
  static constexpr uint32_t kAddressMask = (uint32_t{1} << kMaxAddressBits) - 1;

  static_assert(kMaxDepth < (size_t{1} << kHopsBits));

  constexpr uint32_t bits() const { return bits_; }

  class Generator {
   public:
    constexpr absl::optional<uint32_t> Next() {
      if (num_hops_ == 0) return absl::nullopt;
      uint32_t res = bits_ & ((1 << kChunkPointersBits) - 1);
      bits_ >>= kChunkPointersBits;
      --num_hops_;
      return res;
    }

    constexpr uint32_t Offset() const {
      ABSL_DCHECK_EQ(num_hops_, 0);
      ABSL_DCHECK_EQ(bits_ >> kChunkOffsetBits, 0);
      return bits_;
    }

   private:
    friend BtreeSplitAddress;
    constexpr explicit Generator(uint32_t bits)
        : num_hops_(bits & ((1 << kHopsBits) - 1)), bits_(bits >> kHopsBits) {
      ABSL_DCHECK_LE(num_hops_, kMaxDepth);
    }

    uint32_t num_hops_;
    uint32_t bits_;
  };

  constexpr Generator generator() const { return Generator(bits_); }

 private:
  static constexpr uint32_t Encode(absl::Span<const size_t> hops,
                                   size_t offset);

  uint32_t bits_;
};

// Byte-aligned 64-bit tree address for out-of-line codegen (`MutableExpanded`).
// Faster to decode than `BtreeSplitAddress`, but too wide for reflection.
// Immutable value type.
class BtreeSplitExpandedAddress {
 public:
  explicit constexpr BtreeSplitExpandedAddress(
      std::initializer_list<uint8_t> hops, uint32_t offset);

  constexpr uint64_t bytes() const { return bytes_; }

  class Generator {
   public:
    constexpr absl::optional<uint32_t> Next() {
      if (num_hops_ == 0) return absl::nullopt;
      uint32_t res = static_cast<uint8_t>(bytes_);
      bytes_ >>= 8;
      --num_hops_;
      return res;
    }

    constexpr uint32_t Offset() const {
      ABSL_DCHECK_EQ(num_hops_, uint32_t{0});
      ABSL_DCHECK_EQ(bytes_ >> BtreeSplitAddress::kChunkOffsetBits,
                     uint64_t{0});
      return static_cast<uint32_t>(bytes_);
    }

   private:
    friend BtreeSplitExpandedAddress;
    constexpr explicit Generator(uint64_t bytes)
        : num_hops_(static_cast<uint8_t>(bytes)), bytes_(bytes >> 8) {
      ABSL_DCHECK_LE(num_hops_, sizeof(uint64_t) - 2);
    }

    uint32_t num_hops_;
    uint64_t bytes_;
  };

  constexpr Generator generator() const { return Generator(bytes_); }

 private:
  // Stores 1 byte for num_hops, 1 byte per hop, and 1 byte for offset.
  static_assert(BtreeSplitAddress::kMaxDepth + 2 <= sizeof(uint64_t));

  static constexpr uint64_t Encode(std::initializer_list<uint8_t> hops,
                                   uint32_t offset);

  uint64_t bytes_;
};

// Non-owning handle to a message's split-field tree root.
//
// `BtreeSplit` itself has no destructor and does not track or free memory;
// generated `Message` destructors and clearing code traverse modified subtrees
// (via `NodeWithDefault` / `ConstNodeWithDefault`) to delete owned `Node`s and
// heap-allocated `RawPtr<V>` payloads.
class PROTOBUF_EXPORT BtreeSplit {
 public:
  struct Node;

 private:
  class NodePtr {
   public:
    constexpr NodePtr() = default;
    explicit constexpr NodePtr(Node* node) : ptr_(node) {}

    void* AllocAndMutable(size_t offset, Arena* arena);
    NodePtr* alloc_sub(size_t slot, Arena* arena);

    Node* node() const { return ptr_; }

   private:
    Node* ptr_ = nullptr;
  };

 public:
  // Nodes are 64-byte objects. Global default nodes (kZeroNode and generated
  // kSplitNode arrays) are explicitly 64-byte aligned to reduce cache misses
  // when reading defaults. Non-default nodes allocated on arenas/heap are
  // uncommon and intentionally left 8-byte aligned to avoid padding waste.
  struct Node {
    static constexpr size_t kNumSlots = 8;

    // NOLINTBEGIN(google-explicit-constructor)
    union Bytes4 {
      constexpr Bytes4() : u32(0) {}
      constexpr Bytes4(uint32_t u32) : u32(u32) {}
      constexpr Bytes4(int32_t i32) : i32(i32) {}
      constexpr Bytes4(float f) : f(f) {}
      constexpr Bytes4(bool b0, bool b1, bool b2, bool b3)
          : b{b0, b1, b2, b3} {}

      uint32_t u32;
      int32_t i32;
      float f;
      bool b[4];
    };
    union Bytes8 {
      constexpr Bytes8() : u64(0) {}
      constexpr Bytes8(const Node* subnode)
          : subnode(const_cast<Node*>(subnode)) {}
      constexpr Bytes8(std::nullptr_t) : ptr() {}
      constexpr Bytes8(const void* ptr) : ptr(const_cast<void*>(ptr)) {}
      constexpr Bytes8(uint64_t u64) : u64(u64) {}
      constexpr Bytes8(int64_t i64) : i64(i64) {}
      constexpr Bytes8(double d) : d(d) {}
      constexpr Bytes8(Bytes4 b4_0, Bytes4 b4_1) : b4{b4_0, b4_1} {}

      NodePtr subnode;
      void* ptr;
      uint64_t u64;
      int64_t i64;
      double d;
      Bytes4 b4[2];
    };
    // NOLINTEND(google-explicit-constructor)

    constexpr Node() = default;
    explicit constexpr Node(Bytes8 b0, Bytes8 b1 = {}, Bytes8 b2 = {},
                            Bytes8 b3 = {}, Bytes8 b4 = {}, Bytes8 b5 = {},
                            Bytes8 b6 = {}, Bytes8 b7 = {})
        : slots{b0, b1, b2, b3, b4, b5, b6, b7} {}

    // Reads a typed field in this node, unpacking `RawPtr<V>` to `const V&`.
    template <typename T, int offset, int... hops>
    PROTOBUF_ALWAYS_INLINE const auto& Get(
        BtreeSplitTypedAddress<T, offset, hops...>) const;
    const void* GetUntyped(size_t offset) const {
      ABSL_DCHECK_LT(offset, sizeof(Node));
      auto* res = reinterpret_cast<const char*>(this) + offset;
      PROTOBUF_ASSUME(res != nullptr);
      return res;
    }

    // Returns a mutable reference to the raw slot storage (does NOT unpack
    // `RawPtr<V>`, so generated clearing/destructors can manage it directly).
    template <typename T, int offset, int... hops>
    PROTOBUF_ALWAYS_INLINE T& Mutable(
        BtreeSplitTypedAddress<T, offset, hops...>);
    void* MutableUntyped(size_t offset) {
      ABSL_DCHECK_LT(offset, sizeof(Node));
      auto* res = reinterpret_cast<char*>(this) + offset;
      PROTOBUF_ASSUME(res != nullptr);
      return res;
    }

    const Node* sub(size_t slot) const {
      ABSL_DCHECK_LT(slot, kNumSlots);
      return slots[slot].subnode.node();
    }
    Node* sub(size_t slot) {
      ABSL_DCHECK_LT(slot, kNumSlots);
      return slots[slot].subnode.node();
    }

    Bytes8 slots[kNumSlots];

    static_assert(sizeof(Bytes4) == 4);
    static_assert(sizeof(Bytes8) == 8);
  };

  static_assert(sizeof(Node) == 64);
  static_assert(sizeof(Node) ==
                (size_t{1} << BtreeSplitAddress::kChunkOffsetBits));
  static_assert(Node::kNumSlots ==
                (size_t{1} << BtreeSplitAddress::kChunkPointersBits));

  alignas(64) static const Node kZeroNode;

  // Helpers for GenerateSplitFieldIfChain codegen to skip default subtrees.
  struct ConstNodeWithDefault {
    const Node* value;
    const Node* default_node;

    ConstNodeWithDefault sub(size_t slot) const {
      return {value->sub(slot), default_node->sub(slot)};
    }

    bool is_default() const { return value == default_node; }
  };

  struct NodeWithDefault {
    Node* value;
    const Node* default_node;

    NodeWithDefault sub(size_t slot) const {
      return {value->sub(slot), default_node->sub(slot)};
    }

    bool is_default() const { return value == default_node; }
  };

  // `head` must be non-null (typically a `.rodata` default node).
  explicit constexpr BtreeSplit(const Node* head)
      : head_(const_cast<Node*>(head)) {
    ABSL_DCHECK_NE(head, nullptr);
  }

  Node* head() { return head_.node(); }
  const Node* head() const { return head_.node(); }

  // Reads a typed field, transparently unwrapping `RawPtr<V>` to `const V&`.
  template <typename T, int offset, int... hops>
  PROTOBUF_ALWAYS_INLINE const auto& Get(
      BtreeSplitTypedAddress<T, offset, hops...>) const;

  // Reflection version. Returns a raw pointer to the slot bytes without
  // unwrapping `RawPtr<V>`.
  const void* GetUntyped(BtreeSplitAddress address) const;

  // Returns a mutable pointer to a typed field if its entire path is owned (and
  // for `RawPtr<V>`, not still pointing at `kZeroBuffer`); otherwise `nullptr`.
  template <typename T, int offset, int... hops>
  PROTOBUF_ALWAYS_INLINE auto* TryMutable(
      BtreeSplitTypedAddress<T, offset, hops...>, const Node* default_node);

  // Returns a mutable reference to a typed field (unwrapping `RawPtr<V>` to
  // `V&`), copy-on-write cloning any unowned nodes along the path onto
  // `parent_message`'s arena (or heap if arena-less). `parent_message` and
  // `default_node` must be non-null and `parent_message` must not be
  // `default_instance()`.
  template <typename T, int offset, int... hops>
  PROTOBUF_ALWAYS_INLINE auto& Mutable(
      BtreeSplitTypedAddress<T, offset, hops...> address,
      const MessageLite* parent_message, const Node* default_node);

  // Copy-on-write version for reflection/TcParser. Returns a pointer to the
  // raw slot bytes (without unwrapping `RawPtr<V>`).
  void* MutableUntyped(BtreeSplitAddress address,
                       const MessageLite* parent_message,
                       const Node* default_node);

 private:
  template <typename V>
  PROTOBUF_NOINLINE static void* CreateRaw(Arena* arena) {
    return Arena::Create<V>(arena);
  }

  template <bool expand_raw, typename T, int offset, int... hops>
  PROTOBUF_ALWAYS_INLINE auto* TryMutableImpl(
      BtreeSplitTypedAddress<T, offset, hops...>, const Node* default_node);

  void* MutableExpanded(BtreeSplitExpandedAddress address,
                        const MessageLite* parent_message,
                        const Node* default_node);
  void* MutableRawExpanded(BtreeSplitExpandedAddress address,
                           const MessageLite* parent_message,
                           const Node* default_node, void* (*create)(Arena*));

  NodePtr head_;
};

constexpr uint32_t BtreeSplitAddress::Encode(absl::Span<const size_t> hops,
                                             size_t offset) {
  ABSL_DCHECK_LE(hops.size(), kMaxDepth);
  ABSL_DCHECK_LT(offset, sizeof(BtreeSplit::Node));
  uint32_t bits = static_cast<uint32_t>(hops.size());
  size_t bit_offset = kHopsBits;
  for (size_t hop : hops) {
    ABSL_DCHECK_LT(hop, size_t{1} << kChunkPointersBits);
    bits |= static_cast<uint32_t>(hop) << bit_offset;
    bit_offset += kChunkPointersBits;
  }
  bits |= static_cast<uint32_t>(offset) << bit_offset;
  return bits;
}

constexpr BtreeSplitAddress::BtreeSplitAddress(absl::Span<const size_t> hops,
                                               size_t offset)
    : bits_(Encode(hops, offset)) {}

constexpr uint64_t BtreeSplitExpandedAddress::Encode(
    std::initializer_list<uint8_t> hops, uint32_t offset) {
  ABSL_DCHECK_LE(hops.size(), sizeof(uint64_t) - 2);
  ABSL_DCHECK_LT(offset, sizeof(BtreeSplit::Node));
  uint64_t bytes = hops.size();
  int bit_offset = 8;
  for (uint8_t hop : hops) {
    ABSL_DCHECK_LT(size_t{hop}, BtreeSplit::Node::kNumSlots);
    bytes |= uint64_t{hop} << bit_offset;
    bit_offset += 8;
  }
  bytes |= uint64_t{offset} << bit_offset;
  return bytes;
}

constexpr BtreeSplitExpandedAddress::BtreeSplitExpandedAddress(
    std::initializer_list<uint8_t> hops, uint32_t offset)
    : bytes_(Encode(hops, offset)) {}

template <typename T, int offset, int... hops>
class BtreeSplitTypedAddress {
  static_assert(offset >= 0, "Split offset must be non-negative.");
  static_assert(offset + sizeof(T) <= sizeof(BtreeSplit::Node),
                "Split field exceeds node size.");
  static_assert(alignof(T) <= alignof(BtreeSplit::Node),
                "Split field alignment exceeds node alignment.");
  static_assert(offset % alignof(T) == 0,
                "Split field offset must satisfy type alignment.");
  static_assert(sizeof...(hops) <= BtreeSplitAddress::kMaxDepth,
                "Split tree depth exceeds maximum depth.");
  static_assert(((hops >= 0 &&
                  hops < static_cast<int>(BtreeSplit::Node::kNumSlots)) &&
                 ...),
                "Split hop index out of range.");

 public:
  using value_type = T;

  constexpr BtreeSplitTypedAddress() = default;

  constexpr BtreeSplitExpandedAddress expanded() const {
    constexpr BtreeSplitExpandedAddress res({static_cast<uint8_t>(hops)...},
                                            static_cast<uint32_t>(offset));
    return res;
  }

  constexpr uint32_t ReflectionBits() const {
    constexpr BtreeSplitAddress res(
        std::array<size_t, sizeof...(hops)>{static_cast<size_t>(hops)...},
        static_cast<size_t>(offset));
    return res.bits();
  }
};

template <typename T, int offset, int... hops>
PROTOBUF_ALWAYS_INLINE const auto& BtreeSplit::Node::Get(
    BtreeSplitTypedAddress<T, offset, hops...>) const {
  const void* res = GetUntyped(offset);
  ABSL_DCHECK_LE(static_cast<size_t>(offset) + sizeof(T), sizeof(Node));
  ABSL_DCHECK(MemoryAlign::Of<T>().IsAligned(res));
  if constexpr (kIsRawPtr<T>) {
    return **reinterpret_cast<const T*>(res);
  } else {
    return *reinterpret_cast<const T*>(res);
  }
}

template <typename T, int offset, int... hops>
PROTOBUF_ALWAYS_INLINE T& BtreeSplit::Node::Mutable(
    BtreeSplitTypedAddress<T, offset, hops...>) {
  void* res = MutableUntyped(offset);
  ABSL_DCHECK_LE(static_cast<size_t>(offset) + sizeof(T), sizeof(Node));
  ABSL_DCHECK(MemoryAlign::Of<T>().IsAligned(res));
  return *reinterpret_cast<T*>(res);
}

template <bool expand_raw, typename T, int offset, int... hops>
PROTOBUF_ALWAYS_INLINE auto* BtreeSplit::TryMutableImpl(
    BtreeSplitTypedAddress<T, offset, hops...> address,
    const Node* default_node) {
  ABSL_DCHECK_NE(default_node, nullptr);
  Node* it = head_.node();
  if (ABSL_PREDICT_FALSE(it == default_node)) {
    if constexpr (kIsRawPtr<T> && expand_raw) {
      return static_cast<typename T::value_type*>(nullptr);
    } else {
      return static_cast<T*>(nullptr);
    }
  }

  T* value = ((it = it->sub(hops), default_node = default_node->sub(hops),
               ABSL_PREDICT_TRUE(it != default_node)) &&
              ...)
                 ? &it->Mutable(address)
                 : nullptr;
  if constexpr (kIsRawPtr<T> && expand_raw) {
    if (value == nullptr || value->IsDefault()) {
      return static_cast<typename T::value_type*>(nullptr);
    }
    auto* res = value->Get();
    PROTOBUF_ASSUME(res != nullptr);
    return res;
  } else {
    return value;
  }
}

template <typename T, int offset, int... hops>
PROTOBUF_ALWAYS_INLINE auto* BtreeSplit::TryMutable(
    BtreeSplitTypedAddress<T, offset, hops...> address,
    const Node* default_node) {
  return TryMutableImpl</*expand_raw=*/true>(address, default_node);
}

template <typename T, int offset, int... hops>
PROTOBUF_ALWAYS_INLINE const auto& BtreeSplit::Get(
    BtreeSplitTypedAddress<T, offset, hops...> address) const {
  const Node* it = head_.node();
  ((it = it->sub(hops)), ...);
  return it->Get(address);
}

template <typename T, int offset, int... hops>
PROTOBUF_ALWAYS_INLINE auto& BtreeSplit::Mutable(
    BtreeSplitTypedAddress<T, offset, hops...> address,
    const MessageLite* parent_message, const Node* default_node) {
  ABSL_DCHECK_NE(parent_message, nullptr);
  ABSL_DCHECK_NE(default_node, nullptr);

  // Fastpath for codegen when nodes (and any RawPtr payload) are already
  // owned, keeping all allocation/construction out-of-line.
  if constexpr (kIsRawPtr<T>) {
    static_assert(sizeof(T) == sizeof(void*));
    using V = typename T::value_type;
    V* p = TryMutableImpl</*expand_raw=*/true>(address, default_node);
    if (ABSL_PREDICT_FALSE(p == nullptr)) {
      p = static_cast<V*>(MutableRawExpanded(address.expanded(), parent_message,
                                             default_node, &CreateRaw<V>));
    }
    return *p;
  } else {
    T* p = TryMutableImpl</*expand_raw=*/false>(address, default_node);
    if (ABSL_PREDICT_FALSE(p == nullptr)) {
      p = static_cast<T*>(
          MutableExpanded(address.expanded(), parent_message, default_node));
    }
    return *p;
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_BTREE_SPLIT_H__
