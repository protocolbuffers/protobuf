// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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

void UntypedMapBase::EraseFromTree(map_index_t b,
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

map_index_t UntypedMapBase::VariantBucketNumber(VariantKey key) const {
  return BucketNumberFromHash(key.Hash());
}

void UntypedMapBase::InsertUniqueInTree(map_index_t b, GetKey get_key,
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

    map_index_t b = VariantBucketNumber(get_key(node));
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
      for (map_index_t b = index_of_first_non_null_, end = num_buckets_;
           b < end; ++b) {
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

auto UntypedMapBase::FindFromTree(map_index_t b, VariantKey key,
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
  for (map_index_t b = 0; b < num_buckets_; ++b) {
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
