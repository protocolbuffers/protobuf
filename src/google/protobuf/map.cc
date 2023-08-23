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

#include "google/protobuf/map.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>

#include "absl/hash/hash.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/message_lite.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

const TableEntryPtr kGlobalEmptyTable[kGlobalEmptyTableSize] = {};

NodeBase* UntypedMapBase::DestroyTree(Tree* tree) {
  NodeBase* head = tree->empty() ? nullptr : tree->begin()->second;
  if (alloc_.arena() == nullptr) {
    delete tree;
  }
  return head;
}

void UntypedMapBase::EraseFromTree(size_type b,
                                   typename Tree::iterator tree_it) {
  ABSL_DCHECK(TableEntryIsTree(b));
  Tree* tree = TableEntryToTree(table_[b]);
  if (tree_it != tree->begin()) {
    NodeBase* prev = std::prev(tree_it)->second;
    prev->next = prev->next->next;
  }
  tree->erase(tree_it);
  if (tree->empty()) {
    DestroyTree(tree);
    table_[b] = TableEntryPtr{};
  }
}

size_t UntypedMapBase::VariantBucketNumber(VariantKey key) const {
  return BucketNumberFromHash(key.Hash());
}

void UntypedMapBase::InsertUniqueInTree(size_type b, GetKey get_key,
                                        NodeBase* node) {
  if (TableEntryIsNonEmptyList(b)) {
    // To save in binary size, we delegate to an out-of-line function to do
    // the conversion.
    table_[b] = ConvertToTree(TableEntryToNode(table_[b]), get_key);
  }
  ABSL_DCHECK(TableEntryIsTree(b))
      << (void*)table_[b] << " " << (uintptr_t)table_[b];

  Tree* tree = TableEntryToTree(table_[b]);
  auto it = tree->try_emplace(get_key(node), node).first;
  // Maintain the linked list of the nodes in the tree.
  // For simplicity, they are in the same order as the tree iteration.
  if (it != tree->begin()) {
    NodeBase* prev = std::prev(it)->second;
    prev->next = node;
  }
  auto next = std::next(it);
  node->next = next != tree->end() ? next->second : nullptr;
}

void UntypedMapBase::TransferTree(Tree* tree, GetKey get_key) {
  NodeBase* node = DestroyTree(tree);
  do {
    NodeBase* next = node->next;

    size_type b = VariantBucketNumber(get_key(node));
    // This is similar to InsertUnique, but with erasure.
    if (TableEntryIsEmpty(b)) {
      InsertUniqueInList(b, node);
      index_of_first_non_null_ = (std::min)(index_of_first_non_null_, b);
    } else if (TableEntryIsNonEmptyList(b) && !TableEntryIsTooLong(b)) {
      InsertUniqueInList(b, node);
    } else {
      InsertUniqueInTree(b, get_key, node);
    }

    node = next;
  } while (node != nullptr);
}

TableEntryPtr UntypedMapBase::ConvertToTree(NodeBase* node, GetKey get_key) {
  auto* tree = Arena::Create<Tree>(alloc_.arena(), typename Tree::key_compare(),
                                   typename Tree::allocator_type(alloc_));
  for (; node != nullptr; node = node->next) {
    tree->try_emplace(get_key(node), node);
  }
  ABSL_DCHECK_EQ(MapTreeLengthThreshold(), tree->size());

  // Relink the nodes.
  NodeBase* next = nullptr;
  auto it = tree->end();
  do {
    node = (--it)->second;
    node->next = next;
    next = node;
  } while (it != tree->begin());

  return TreeToTableEntry(tree);
}

void UntypedMapBase::ClearTable(const ClearInput input) {
  ABSL_DCHECK_NE(num_buckets_, kGlobalEmptyTableSize);

  if (alloc_.arena() == nullptr) {
    const auto loop = [=](auto destroy_node) {
      const TableEntryPtr* table = table_;
      for (size_type b = index_of_first_non_null_, end = num_buckets_; b < end;
           ++b) {
        NodeBase* node =
            PROTOBUF_PREDICT_FALSE(internal::TableEntryIsTree(table[b]))
                ? DestroyTree(TableEntryToTree(table[b]))
                : TableEntryToNode(table[b]);

        while (node != nullptr) {
          NodeBase* next = node->next;
          destroy_node(node);
          SizedDelete(node, SizeFromInfo(input.size_info));
          node = next;
        }
      }
    };
    switch (input.destroy_bits) {
      case 0:
        loop([](NodeBase*) {});
        break;
      case kKeyIsString:
        loop([](NodeBase* node) {
          static_cast<std::string*>(node->GetVoidKey())->~basic_string();
        });
        break;
      case kValueIsString:
        loop([size_info = input.size_info](NodeBase* node) {
          static_cast<std::string*>(node->GetVoidValue(size_info))
              ->~basic_string();
        });
        break;
      case kKeyIsString | kValueIsString:
        loop([size_info = input.size_info](NodeBase* node) {
          static_cast<std::string*>(node->GetVoidKey())->~basic_string();
          static_cast<std::string*>(node->GetVoidValue(size_info))
              ->~basic_string();
        });
        break;
      case kValueIsProto:
        loop([size_info = input.size_info](NodeBase* node) {
          static_cast<MessageLite*>(node->GetVoidValue(size_info))
              ->~MessageLite();
        });
        break;
      case kKeyIsString | kValueIsProto:
        loop([size_info = input.size_info](NodeBase* node) {
          static_cast<std::string*>(node->GetVoidKey())->~basic_string();
          static_cast<MessageLite*>(node->GetVoidValue(size_info))
              ->~MessageLite();
        });
        break;
      case kUseDestructFunc:
        loop(input.destroy_node);
        break;
    }
  }

  if (input.reset_table) {
    std::fill(table_, table_ + num_buckets_, TableEntryPtr{});
    num_elements_ = 0;
    index_of_first_non_null_ = num_buckets_;
  } else {
    DeleteTable(table_, num_buckets_);
  }
}

auto UntypedMapBase::FindFromTree(size_type b, VariantKey key,
                                  Tree::iterator* it) const -> NodeAndBucket {
  Tree* tree = TableEntryToTree(table_[b]);
  auto tree_it = tree->find(key);
  if (it != nullptr) *it = tree_it;
  if (tree_it != tree->end()) {
    return {tree_it->second, b};
  }
  return {nullptr, b};
}

size_t UntypedMapBase::SpaceUsedInTable(size_t sizeof_node) const {
  size_t size = 0;
  // The size of the table.
  size += sizeof(void*) * num_buckets_;
  // All the nodes.
  size += sizeof_node * num_elements_;
  // For each tree, count the overhead of those nodes.
  // Two buckets at a time because we only care about trees.
  for (size_t b = 0; b < num_buckets_; ++b) {
    if (TableEntryIsTree(b)) {
      size += sizeof(Tree);
      size += sizeof(Tree::value_type) * TableEntryToTree(table_[b])->size();
    }
  }
  return size;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
