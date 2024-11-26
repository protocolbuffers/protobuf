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
#include "absl/functional/overload.h"
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

void UntypedMapBase::ClearTableImpl(bool reset, void (*destroy)(NodeBase*)) {
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
          SizedDelete(node, type_info_.node_size);
          node = next;
        }
      }
    };

    const auto dispatch_key = [&](auto value_handler) {
      if (type_info_.key_type < TypeKind::kString) {
        return loop(value_handler);
      } else if (type_info_.key_type == TypeKind::kString) {
        return loop([=](NodeBase* node) {
          static_cast<std::string*>(node->GetVoidKey())->~basic_string();
          value_handler(node);
        });
      } else {
        ABSL_CHECK(destroy != nullptr);
        return loop(destroy);
      }
    };

    if (type_info_.value_type < TypeKind::kString) {
      dispatch_key([](NodeBase*) {});
    } else if (type_info_.value_type == TypeKind::kString) {
      dispatch_key([&](NodeBase* node) {
        GetValue<std::string>(node)->~basic_string();
      });
    } else if (type_info_.value_type == TypeKind::kMessage) {
      dispatch_key([&](NodeBase* node) {
        GetValue<MessageLite>(node)->DestroyInstance();
      });
    } else {
      ABSL_CHECK(destroy != nullptr);
      loop(destroy);
    }
  }

  if (reset) {
    std::fill(table_, table_ + num_buckets_, nullptr);
    num_elements_ = 0;
    index_of_first_non_null_ = num_buckets_;
  } else {
    DeleteTable(table_, num_buckets_);
  }
}

size_t UntypedMapBase::SpaceUsedExcludingSelfLong() const {
  size_t size = 0;
  // The size of the table.
  size += sizeof(void*) * num_buckets_;
  // All the nodes.
  size += type_info_.node_size * num_elements_;
  VisitAllNodes([&](auto* key, auto* value) {
    const auto space_used = absl::Overload{
        [](const std::string* str) -> size_t {
          return StringSpaceUsedExcludingSelfLong(*str);
        },
        [&](const MessageLite* msg) -> size_t {
          const auto* class_data = GetClassData(*msg);
          if (class_data->is_lite) return 0;
          return class_data->full().descriptor_methods->space_used_long(*msg) -
                 class_data->allocation_size();
        },
        [](const void*) -> size_t { return 0; }};
    size += space_used(key);
    size += space_used(value);
  });
  return size;
}

static size_t AlignTo(size_t v, size_t alignment, size_t& max_align) {
  max_align = std::max<size_t>(max_align, alignment);
  return (v + alignment - 1) / alignment * alignment;
}

struct Offsets {
  size_t start;
  size_t end;
};

template <typename T>
static Offsets AlignAndAddSize(size_t v, size_t& max_align) {
  v = AlignTo(v, alignof(T), max_align);
  return {v, v + sizeof(T)};
}

static Offsets AlignAndAddSizeDynamic(
    size_t v, UntypedMapBase::TypeKind kind,
    const MessageLite* value_prototype_if_message, size_t& max_align) {
  switch (kind) {
    case UntypedMapBase::TypeKind::kBool:
      return AlignAndAddSize<bool>(v, max_align);
    case UntypedMapBase::TypeKind::kU32:
      return AlignAndAddSize<int32_t>(v, max_align);
    case UntypedMapBase::TypeKind::kU64:
      return AlignAndAddSize<int64_t>(v, max_align);
    case UntypedMapBase::TypeKind::kFloat:
      return AlignAndAddSize<float>(v, max_align);
    case UntypedMapBase::TypeKind::kDouble:
      return AlignAndAddSize<double>(v, max_align);
    case UntypedMapBase::TypeKind::kString:
      return AlignAndAddSize<std::string>(v, max_align);
    case UntypedMapBase::TypeKind::kMessage: {
      auto* class_data = GetClassData(*value_prototype_if_message);
      v = AlignTo(v, class_data->alignment(), max_align);
      return {v, v + class_data->allocation_size()};
    }
    default:
      Unreachable();
  }
}

template <typename T, typename U>
T Narrow(U value) {
  ABSL_CHECK_EQ(value, static_cast<T>(value));
  return static_cast<T>(value);
}

UntypedMapBase::TypeInfo UntypedMapBase::GetTypeInfoDynamic(
    TypeKind key_type, TypeKind value_type,
    const MessageLite* value_prototype_if_message) {
  size_t max_align = alignof(NodeBase);
  const auto key_offsets =
      AlignAndAddSizeDynamic(sizeof(NodeBase), key_type, nullptr, max_align);
  const auto value_offsets = AlignAndAddSizeDynamic(
      key_offsets.end, value_type, value_prototype_if_message, max_align);
  return TypeInfo{
      Narrow<uint16_t>(AlignTo(value_offsets.end, max_align, max_align)),
      Narrow<uint8_t>(value_offsets.start), key_type, value_type};
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
