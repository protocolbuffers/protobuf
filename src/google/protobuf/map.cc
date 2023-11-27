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

void UntypedMapBase::ClearTable(const ClearInput input) {
  ABSL_DCHECK_NE(num_buckets_, kGlobalEmptyTableSize);

  if (alloc_.arena() == nullptr) {
    const auto loop = [=](auto destroy_node) {
      const TableEntryPtr* table = table_;
      for (map_index_t b = 0, end = num_buckets_; b < end; ++b) {
        // XXX Fix prefetch
        if (TableEntryIsNonEmptyList(b)) {
          NodeBase* node = TableEntryToNode(table[b]);
          destroy_node(node);
          SizedDelete(node, SizeFromInfo(input.size_info));
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
    ResetGrowthLeft();
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
