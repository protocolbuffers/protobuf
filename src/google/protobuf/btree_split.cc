// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/btree_split.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "absl/log/absl_check.h"
#include "absl/types/optional.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/raw_ptr.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

alignas(64) PROTOBUF_CONSTINIT const BtreeSplit::Node BtreeSplit::kZeroNode{};

inline void* BtreeSplit::NodePtr::AllocAndMutable(size_t offset, Arena* arena) {
  ABSL_DCHECK_NE(ptr_, nullptr);
  ptr_ = Arena::Create<Node>(arena, *ptr_);
  return ptr_->MutableUntyped(offset);
}

inline BtreeSplit::NodePtr* BtreeSplit::NodePtr::alloc_sub(size_t slot,
                                                           Arena* arena) {
  ABSL_DCHECK_NE(ptr_, nullptr);
  ABSL_DCHECK_LT(slot, Node::kNumSlots);
  ptr_ = Arena::Create<Node>(arena, *ptr_);
  return &ptr_->slots[slot].subnode;
}

const void* BtreeSplit::GetUntyped(const BtreeSplitAddress address) const {
  const Node* it = head_.node();
  auto gen = address.generator();
  while (auto next = gen.Next()) {
    it = it->sub(*next);
  }
  return it->GetUntyped(gen.Offset());
}

void* BtreeSplit::MutableUntyped(const BtreeSplitAddress address,
                                 const MessageLite* parent_message,
                                 const Node* default_node) {
  ABSL_DCHECK_NE(parent_message, nullptr);
  ABSL_DCHECK_NE(default_node, nullptr);
  Arena* arena = parent_message->GetArena();

  NodePtr* it = &head_;

  auto gen = address.generator();

  // Traverse nodes until we find the owned leaf or the first unowned node.
  while (true) {
    if (it->node() == default_node) {
      // We found a node we don't own. We will start copying.
      while (auto next = gen.Next()) {
        it = it->alloc_sub(*next, arena);
      }
      return it->AllocAndMutable(gen.Offset(), arena);
    }

    auto next = gen.Next();

    if (!next.has_value()) {
      // We found the leaf and we own them all.
      return it->node()->MutableUntyped(gen.Offset());
    }

    // Still not a leaf. Go to the next one.
    const uint32_t slot = *next;
    ABSL_DCHECK_LT(slot, Node::kNumSlots);
    it = &it->node()->slots[slot].subnode;
    default_node = default_node->sub(slot);
  }
}

void* BtreeSplit::MutableExpanded(const BtreeSplitExpandedAddress address,
                                  const MessageLite* parent_message,
                                  const Node* default_node) {
  ABSL_DCHECK_NE(parent_message, nullptr);
  ABSL_DCHECK_NE(default_node, nullptr);
  Arena* arena = parent_message->GetArena();

  NodePtr* it = &head_;
  auto gen = address.generator();

  // We were called because we have to allocate.
  // This means we _must_ find a node where we are the default.

  while (it->node() != default_node) {
    auto next = gen.Next();
    ABSL_DCHECK(next.has_value());
    const uint32_t slot = *next;
    ABSL_DCHECK_LT(slot, Node::kNumSlots);
    it = &it->node()->slots[slot].subnode;
    default_node = default_node->sub(slot);
  }

  // We found it. Now copy the nodes transitively until we reach the leaf.
  while (auto next = gen.Next()) {
    it = it->alloc_sub(*next, arena);
  }

  return it->AllocAndMutable(gen.Offset(), arena);
}

void* BtreeSplit::MutableRawExpanded(const BtreeSplitExpandedAddress address,
                                     const MessageLite* parent_message,
                                     const Node* default_node,
                                     void* (*create)(Arena*)) {
  ABSL_DCHECK_NE(parent_message, nullptr);
  ABSL_DCHECK_NE(default_node, nullptr);
  ABSL_DCHECK_NE(create, nullptr);
  Arena* arena = parent_message->GetArena();

  NodePtr* it = &head_;
  auto gen = address.generator();
  void* slot;
  while (true) {
    if (it->node() == default_node) {
      while (auto next = gen.Next()) {
        it = it->alloc_sub(*next, arena);
      }
      slot = it->AllocAndMutable(gen.Offset(), arena);
      break;
    }

    auto next = gen.Next();
    if (!next.has_value()) {
      slot = it->node()->MutableUntyped(gen.Offset());
      break;
    }

    const uint32_t hop = *next;
    ABSL_DCHECK_LT(hop, Node::kNumSlots);
    it = &it->node()->slots[hop].subnode;
    default_node = default_node->sub(hop);
  }

  void* p;
  std::memcpy(&p, slot, sizeof(p));
  ABSL_DCHECK_EQ(p, DefaultRawPtr());
  p = create(arena);
  std::memcpy(slot, &p, sizeof(p));
  return p;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
