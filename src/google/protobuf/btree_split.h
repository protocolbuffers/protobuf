// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_BTREE_SPLIT_H__
#define GOOGLE_PROTOBUF_BTREE_SPLIT_H__

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "google/protobuf/arena.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// DO NOT SUBMIT: Document addressing scheme.

class BtreeSplitAddress {
 public:
  explicit constexpr BtreeSplitAddress(uint32_t v) : bits_(v) {}

  constexpr uint32_t bits() const { return bits_; }
  constexpr uint32_t ConsumeBits(size_t n_bits) {
    uint32_t res = bits_ & ((uint32_t{1} << n_bits) - 1);
    bits_ >>= n_bits;
    return res;
  }

  static constexpr uint32_t CalculateBits(absl::Span<const size_t> steps,
                                          size_t chunk_offset);

  // private:
  uint32_t bits_;
};

template <typename T>
class BtreeSplitTypedAddress : public BtreeSplitAddress {
 public:
  using value_type = T;

  constexpr uint32_t node_offset() const {
    const uint32_t b = bits();
    // DO NOT SUBMIT: Use constants
    const uint32_t num_hops = b & 3;
    return b >> ((num_hops + 1) * 3);
  }

 private:
  friend BtreeSplitAddress;
  using BtreeSplitAddress::BtreeSplitAddress;
};

class BtreeSplit {
 public:
  static constexpr size_t kMaxHopsBits = 3;
  static constexpr size_t kChunkPointersBits = 3;
  static constexpr size_t kChunkOffsetBits = 6;
  static constexpr size_t kMaxDepth =
      (28 - kMaxHopsBits - kChunkOffsetBits) / kChunkPointersBits;

  struct Node;

  class NodePtr {
   public:
    constexpr NodePtr() = default;
    explicit constexpr NodePtr(void* node) : ptr_(node) {}

    void* Mutable(size_t offset, Arena* arena, const Node* default_node) {
      EnsureAllocated(arena, default_node);
      return node()->Mutable(offset);
    }

    const void* Get(size_t offset) const {
      ABSL_DCHECK_NE(node(), nullptr);
      ABSL_DCHECK_LT(offset, sizeof(Node));
      return node()->Get(offset);
    }

    NodePtr& sub_mutable(size_t n, Arena* arena, const Node* default_node) {
      EnsureAllocated(arena, default_node);
      return node()->slots[n].subnode;
    }

    const Node* sub(size_t n) const { return node()->sub(n); }

    Node* node() const { return reinterpret_cast<Node*>(ptr_); }

   private:
    void EnsureAllocated(Arena* arena, const Node* default_node) & {
      if (ptr_ == default_node) {
        ptr_ = Arena::Create<Node>(arena, *node());
      }
    }

    void* ptr_;
  };

  struct Node {
    union Bytes4 {
      constexpr Bytes4() = default;
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
      constexpr Bytes8() = default;
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

    constexpr Node() = default;
    constexpr Node(Bytes8 b0, Bytes8 b1 = {}, Bytes8 b2 = {}, Bytes8 b3 = {},
                   Bytes8 b4 = {}, Bytes8 b5 = {}, Bytes8 b6 = {},
                   Bytes8 b7 = {})
        : slots{b0, b1, b2, b3, b4, b5, b6, b7} {}

    template <typename T>
    const T& Get(BtreeSplitTypedAddress<T> address) const {
      return *reinterpret_cast<const T*>(Get(address.node_offset()));
    }
    const void* Get(size_t offset) const {
      return reinterpret_cast<const char*>(this) + offset;
    }
    template <typename T>
    T& Mutable(BtreeSplitTypedAddress<T> address) {
      return *reinterpret_cast<T*>(Mutable(address.node_offset()));
    }
    void* Mutable(size_t offset) {
      return reinterpret_cast<char*>(this) + offset;
    }

    const Node* sub(size_t n) const { return slots[n].subnode.node(); }
    Node* sub(size_t n) { return slots[n].subnode.node(); }

    Bytes8 slots[8];
  };

  struct ConstNodeWithDefault {
    const Node* value;
    const Node* default_node;

    ConstNodeWithDefault sub(size_t n) {
      return {value->sub(n), default_node->sub(n)};
    }

    bool is_default() const { return value == default_node; }
  };

  struct NodeWithDefault {
    Node* value;
    const Node* default_node;

    NodeWithDefault sub(size_t n) {
      return {value->sub(n), default_node->sub(n)};
    }

    bool is_default() const { return value == default_node; }
  };

  explicit constexpr BtreeSplit(const void* head)
      : head_(const_cast<void*>(head)) {}

  Node* head() { return head_.node(); }
  const Node* head() const { return head_.node(); }

  template <typename T>
  T& AssumeMutable(BtreeSplitTypedAddress<T> address) {
    return AssumeMutable<T>(BtreeSplitAddress(address));
  }

  template <typename T>
  T& AssumeMutable(BtreeSplitAddress address) {
    return *static_cast<T*>(AssumeMutableImpl(address));
  }

  template <typename T>
  T* TryMutable(BtreeSplitTypedAddress<T> address, const Node* default_node) {
    return TryMutable<T>(BtreeSplitAddress(address), default_node);
  }

  template <typename T>
  T* TryMutable(BtreeSplitAddress address, const Node* default_node) {
    return static_cast<T*>(TryMutableImpl(address, default_node));
  }

  template <typename T>
  const T& Get(BtreeSplitTypedAddress<T> address) const {
    return Get<T>(BtreeSplitAddress(address));
  }

  template <uint32_t num_hops, uint32_t encoded, size_t... I>
  static constexpr auto GetOffsetSequence(std::index_sequence<I...> seq) {
    if constexpr (num_hops == 0) {
      return seq;
    } else {
      constexpr uint32_t mask = ((uint32_t{1} << kChunkPointersBits) - 1);
      return GetOffsetSequence<num_hops - 1, (encoded >> kChunkPointersBits)>(
          std::index_sequence<I..., encoded & mask>{});
    }
  }

  // DO NOT SUBMIT: Document
  template <auto address>
  static constexpr auto IntoOffsets() {
    return GetOffsetSequence<
        BtreeSplitAddress(address).ConsumeBits(kMaxHopsBits),
        (address.bits() >> kMaxHopsBits), address.node_offset()>({});
  }

  template <size_t offset, size_t... chunks>
  PROTOBUF_ALWAYS_INLINE const void* GetImpl(
      std::index_sequence<offset, chunks...>) const {
    const Node* it = head_.node();
    ((it = it->sub(chunks)), ...);
    return it->Get(offset);
  }

  // Template version of the function to move the address decoding to constant
  // evaluation. This makes it easier for the optimizer.
  template <auto address>
  PROTOBUF_ALWAYS_INLINE const decltype(address)::value_type& Get() const {
    constexpr auto bits = IntoOffsets<address>();
    return *static_cast<const decltype(address)::value_type*>(GetImpl(bits));
  }

  template <typename T>
  const T& Get(BtreeSplitAddress address) const {
    return *static_cast<const T*>(GetImpl(address));
  }

  const void* Get(BtreeSplitAddress address) const { return GetImpl(address); }

  template <typename T, typename Msg>
  T& Mutable(BtreeSplitTypedAddress<T> address, Msg* msg,
             const Node* default_node) {
    // This one is for codegen, so we have a fastpath for when it does not need
    // to allocate where we can inline the accesses.
    if (T* p = TryMutable(address, default_node)) {
      return *p;
    }
    return *static_cast<T*>(
        Mutable(BtreeSplitAddress(address), msg, default_node));
  }

  template <typename Msg>
  void* Mutable(BtreeSplitAddress address, Msg* msg, const Node* default_node) {
    return MutableImplNoInline(address, msg, default_node);
  }

  // DO NOT SUBMIT: fix comment.
  // Helper functions to mark as NOINLINE to reduce code bloat in generated
  // code.
  //
  template <typename T, typename Arena>
  void SetPrimitive(BtreeSplitTypedAddress<T> address, Arena* arena,
                    std::enable_if_t<true, T> value, const Node* default_node) {
    Mutable(address, arena, default_node) = value;
  }

  // Assign function that traverses both splits at the same time.
  template <typename T>
  PROTOBUF_NOINLINE void AssignFrom(BtreeSplitTypedAddress<T> address,
                                    Arena* arena, const BtreeSplit& from,
                                    const Node* default_node) {
    NodePtr* self_it = &head_;
    const Node* from_it = from.head();

    uint32_t num_hops = address.ConsumeBits(kMaxHopsBits);
    while (num_hops-- > 0) {
      const uint32_t slot = address.ConsumeBits(kChunkPointersBits);
      self_it = &self_it->sub_mutable(slot, arena, default_node);
      from_it = from_it->sub(slot);
      default_node = default_node->sub(slot);
    }

    *static_cast<T*>(self_it->Mutable(address.bits(), arena, default_node)) =
        *static_cast<const T*>(from_it->Get(address.bits()));
  }

  template <typename T>
  PROTOBUF_ALWAYS_INLINE void ClearIfNotDefault(
      BtreeSplitTypedAddress<T> address, const Node* default_node) {
    if (auto* p = TryMutable(address, default_node)) p->ClearIfNotDefault();
  }
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void ClearToEmpty(BtreeSplitTypedAddress<T> address,
                                           const Node* default_node) {
    if (auto* p = TryMutable(address, default_node)) p->ClearToEmpty();
  }
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void ClearNonDefaultToEmpty(
      BtreeSplitTypedAddress<T> address) {
    if (auto* p = TryMutable(address)) p->ClearNonDefaultToEmpty();
  }
  template <typename MessageLite>
  PROTOBUF_NOINLINE void MessageCopyConstruct(
      BtreeSplitTypedAddress<MessageLite*> address, const BtreeSplit& from,
      Arena* arena) {
    *static_cast<MessageLite**>(MutableImpl(address, arena)) =
        MessageLite::CopyConstruct(arena, *from.Get(address));
  }
  template <typename T>
  PROTOBUF_NOINLINE T& RawPtrConstructIfNeeded(
      BtreeSplitTypedAddress<T> address,
      std::conditional_t<false, T, const MessageLite*> msg,
      const Node* default_node) {
    auto& value = Mutable(address, msg, default_node);
    if (value.IsDefault()) {
      value.Set(Arena::Create<typename T::value_type>(msg->GetArena()));
    }
    return value;
  }
  // This one does not allocate memory, so we don't mark it NOINLINE. It is
  // pretty short when the address is a compile time constant.
  template <typename T>
  void SetPrimitiveAssumeMutable(BtreeSplitTypedAddress<T> address,
                                 std::enable_if_t<true, T> value) {
    AssumeMutable(address) = value;
  }

  template <typename T>
  void SetPrimitiveIfMutable(BtreeSplitTypedAddress<T> address,
                             std::enable_if_t<true, T> value,
                             const Node* default_node) {
    if (auto* p = TryMutable(address, default_node)) *p = value;
  }

 private:
  void* AssumeMutableImpl(BtreeSplitAddress address) {
    const Node* it = head_.node();

    uint32_t num_hops = address.ConsumeBits(kMaxHopsBits);
    while (num_hops-- > 0) {
      it = it->sub(address.ConsumeBits(kChunkPointersBits));
    }

    void* res = const_cast<void*>(it->Get(address.bits()));
    PROTOBUF_ASSUME(res != nullptr);
    return res;
  }

  void* TryMutableImpl(BtreeSplitAddress address, const Node* default_node) {
    const Node* it = head_.node();
    if (it == default_node) return nullptr;

    uint32_t num_hops = address.ConsumeBits(kMaxHopsBits);
    while (num_hops-- > 0) {
      const uint32_t slot = address.ConsumeBits(kChunkPointersBits);
      it = it->sub(slot);
      default_node = default_node->sub(slot);
      if (it == default_node) return nullptr;
    }

    void* res = const_cast<void*>(it->Get(address.bits()));
    PROTOBUF_ASSUME(res != nullptr);
    return res;
  }

  template <typename Arena>
  auto ResolveArena(Arena* arena) {
    if constexpr (std::is_same_v<Arena, google::protobuf::Arena>) {
      return arena;
    } else {
      return arena->GetArena();
    }
  }

  void* MutableImpl(BtreeSplitAddress address, Arena* arena,
                    const Node* default_node) {
    NodePtr* it = &head_;

    uint32_t num_hops = address.ConsumeBits(kMaxHopsBits);
    while (num_hops-- > 0) {
      const uint32_t slot = address.ConsumeBits(kChunkPointersBits);
      it = &it->sub_mutable(slot, arena, default_node);
      default_node = default_node->sub(slot);
    }
    return it->Mutable(address.bits(), arena, default_node);
  }

  template <typename Msg>
  PROTOBUF_NOINLINE void* MutableImplNoInline(BtreeSplitAddress address,
                                              const Msg* msg,
                                              const Node* default_node) {
    return MutableImpl(address, msg->GetArena(), default_node);
  }

  PROTOBUF_NOINLINE void* MutableImplNoInline(BtreeSplitAddress address,
                                              Arena* arena,
                                              const Node* default_node) {
    return MutableImpl(address, arena, default_node);
  }

  const void* GetImpl(BtreeSplitAddress address) const {
    const Node* it = head_.node();
    uint32_t num_hops = address.ConsumeBits(kMaxHopsBits);
    while (num_hops-- > 0) {
      it = it->sub(address.ConsumeBits(kChunkPointersBits));
    }
    return it->Get(address.bits());
  }

  NodePtr head_;
};

constexpr uint32_t BtreeSplitAddress::CalculateBits(
    absl::Span<const size_t> steps, size_t chunk_offset) {
  uint32_t result = steps.size();
  int offset = BtreeSplit::kMaxHopsBits;
  for (size_t step : steps) {
    result |= step << offset;
    offset += BtreeSplit::kChunkPointersBits;
  }
  result |= chunk_offset << offset;

  BtreeSplitAddress address(result);
  ABSL_CHECK_EQ(steps.size(), address.ConsumeBits(BtreeSplit::kMaxHopsBits));
  for (size_t step : steps) {
    ABSL_CHECK_EQ(step, address.ConsumeBits(BtreeSplit::kChunkPointersBits));
  }
  ABSL_CHECK_EQ(chunk_offset, address.bits());

  return result;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_BTREE_SPLIT_H__
