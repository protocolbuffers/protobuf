// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/map.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
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

std::atomic<MapFieldBaseForParse::SyncFunc>
    MapFieldBaseForParse::sync_map_with_repeated{};

NodeBase* const kGlobalEmptyTable[kGlobalEmptyTableSize] = {};

void UntypedMapBase::UntypedMergeFrom(const UntypedMapBase& other) {
  if (other.empty()) return;

  // Do the merging in steps to avoid Key*Value number of instantiations and
  // reduce code duplication per instantation.
  NodeBase* nodes = nullptr;

  // First, allocate all the nodes without types.
  for (size_t i = 0; i < other.num_elements_; ++i) {
    NodeBase* new_node = AllocNode();
    new_node->next = nodes;
    nodes = new_node;
  }

  // Then, copy the values.
  VisitValueType([&](auto value_type) {
    using Value = typename decltype(value_type)::type;
    NodeBase* out_node = nodes;

    // Get the ClassData once to avoid redundant virtual function calls.
    const internal::ClassData* class_data =
        std::is_same_v<MessageLite, Value>
            ? GetClassData(*other.GetValue<MessageLite>(other.begin().node_))
            : nullptr;

    for (auto it = other.begin(); !it.Equals(EndIterator()); it.PlusPlus()) {
      Value* out = GetValue<Value>(out_node);
      out_node = out_node->next;
      auto& in = *other.GetValue<Value>(it.node_);
      if constexpr (std::is_same_v<MessageLite, Value>) {
        class_data->PlacementNew(out, arena())->CheckTypeAndMergeFrom(in);
      } else {
        Arena::CreateInArenaStorage(out, this->arena_, in);
      }
    }
  });

  // Finally, copy the keys and insert the nodes.
  VisitKeyType([&](auto key_type) {
    using Key = typename decltype(key_type)::type;
    for (auto it = other.begin(); !it.Equals(EndIterator()); it.PlusPlus()) {
      NodeBase* node = nodes;
      nodes = nodes->next;
      const Key& in = *other.GetKey<Key>(it.node_);
      Key* out = GetKey<Key>(node);
      if (!internal::InitializeMapKey(out, in, this->arena_)) {
        Arena::CreateInArenaStorage(out, this->arena_, in);
      }

      static_cast<KeyMapBase<Key>*>(this)->InsertOrReplaceNode(
          static_cast<typename KeyMapBase<Key>::KeyNode*>(node));
    }
  });
}

void UntypedMapBase::UntypedSwap(UntypedMapBase& other) {
  if (arena() == other.arena()) {
    InternalSwap(&other);
  } else {
    UntypedMapBase tmp(arena_, type_info_);
    InternalSwap(&tmp);

    ABSL_DCHECK(empty());
    UntypedMergeFrom(other);

    other.ClearTable(true);
    other.UntypedMergeFrom(tmp);

    if (arena_ == nullptr) tmp.ClearTable(false);
  }
}

void UntypedMapBase::DeleteNode(NodeBase* node) {
  const auto destroy = absl::Overload{
      [](std::string* str) { str->~basic_string(); },
      [](MessageLite* msg) { msg->DestroyInstance(); }, [](void*) {}};
  VisitKey(node, destroy);
  VisitValue(node, destroy);
  DeallocNode(node);
}

void UntypedMapBase::ClearTableImpl(bool reset) {
  ABSL_DCHECK_NE(num_buckets_, kGlobalEmptyTableSize);

  if (arena_ == nullptr) {
    const auto loop = [this](auto destroy_node) {
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
      if (type_info_.key_type_kind() < TypeKind::kString) {
        loop(value_handler);
      } else if (type_info_.key_type_kind() == TypeKind::kString) {
        loop([=](NodeBase* node) {
          static_cast<std::string*>(node->GetVoidKey())->~basic_string();
          value_handler(node);
        });
      } else {
        Unreachable();
      }
    };

    if (type_info_.value_type_kind() < TypeKind::kString) {
      dispatch_key([](NodeBase*) {});
    } else if (type_info_.value_type_kind() == TypeKind::kString) {
      dispatch_key([&](NodeBase* node) {
        GetValue<std::string>(node)->~basic_string();
      });
    } else if (type_info_.value_type_kind() == TypeKind::kMessage) {
      dispatch_key([&](NodeBase* node) {
        GetValue<MessageLite>(node)->DestroyInstance();
      });
    } else {
      Unreachable();
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
      Narrow<uint8_t>(value_offsets.start), static_cast<uint8_t>(key_type),
      static_cast<uint8_t>(value_type)};
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
