// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/map.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>

#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

NodeBase* const kGlobalEmptyTable[kGlobalEmptyTableSize] = {};

void UntypedMapBase::ClearTable(const ClearInput input) {
  ABSL_DCHECK_NE(num_buckets_, kGlobalEmptyTableSize);

  if (alloc_.arena() == nullptr) {
    const auto loop = [&, this](auto destroy_node) {
      NodeBase** table = table_;
      for (map_index_t b = index_of_first_non_null_, end = num_buckets_;
           b < end; ++b) {
        for (NodeBase* node = table[b]; node != nullptr;) {
          NodeBase* next = node->next;
          absl::PrefetchToLocalCacheNta(next);
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
              ->DestroyInstance();
        });
        break;
      case kKeyIsString | kValueIsProto:
        loop([size_info = input.size_info](NodeBase* node) {
          static_cast<std::string*>(node->GetVoidKey())->~basic_string();
          static_cast<MessageLite*>(node->GetVoidValue(size_info))
              ->DestroyInstance();
        });
        break;
      case kUseDestructFunc:
        loop(input.destroy_node);
        break;
    }
  }

  if (input.reset_table) {
    std::fill(table_, table_ + num_buckets_, nullptr);
    num_elements_ = 0;
    index_of_first_non_null_ = num_buckets_;
  } else {
    DeleteTable(table_, num_buckets_);
  }
}

size_t UntypedMapBase::SpaceUsedInTable(size_t sizeof_node) const {
  size_t size = 0;
  // The size of the table.
  size += sizeof(void*) * num_buckets_;
  // All the nodes.
  size += sizeof_node * num_elements_;
  return size;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
