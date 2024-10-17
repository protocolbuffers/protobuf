#include "rust/cpp_kernel/map.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/log/absl_log.h"
#include "google/protobuf/map.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

namespace google {
namespace protobuf {
namespace rust {
namespace {

template <typename T>
struct FromViewType {
  using type = T;
};

template <>
struct FromViewType<PtrAndLen> {
  using type = std::string;
};

template <typename Key>
using KeyMap = internal::KeyMapBase<
    internal::KeyForBase<typename FromViewType<Key>::type>>;

internal::MapNodeSizeInfoT GetSizeInfo(size_t key_size,
                                       const google::protobuf::MessageLite* value) {
  // Each map node consists of a NodeBase followed by a std::pair<Key, Value>.
  // We need to compute the offset of the value and the total size of the node.
  size_t node_and_key_size = sizeof(internal::NodeBase) + key_size;
  uint16_t value_size;
  uint8_t value_alignment;
  internal::RustMapHelper::GetSizeAndAlignment(value, &value_size,
                                               &value_alignment);
  // Round node_and_key_size up to the nearest multiple of value_alignment.
  uint16_t offset =
      (((node_and_key_size - 1) / value_alignment) + 1) * value_alignment;
  return internal::RustMapHelper::MakeSizeInfo(offset + value_size, offset);
}

template <typename Key>
internal::MapNodeSizeInfoT GetSizeInfo(const google::protobuf::MessageLite* value) {
  return GetSizeInfo(sizeof(Key), value);
}

template <typename Key>
void DestroyMapNode(internal::UntypedMapBase* m, internal::NodeBase* node,
                    internal::MapNodeSizeInfoT size_info) {
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    static_cast<std::string*>(node->GetVoidKey())->~basic_string();
  }
  internal::RustMapHelper::DestroyMessage(
      static_cast<MessageLite*>(node->GetVoidValue(size_info)));
  internal::RustMapHelper::DeallocNode(m, node, size_info);
}

template <typename Key>
bool Insert(internal::UntypedMapBase* m, Key key, MessageLite* value) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo<typename FromViewType<Key>::type>(value);
  internal::NodeBase* node = internal::RustMapHelper::AllocNode(m, size_info);
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    new (node->GetVoidKey()) std::string(key.ptr, key.len);
  } else {
    *static_cast<Key*>(node->GetVoidKey()) = key;
  }

  MessageLite* new_msg = internal::RustMapHelper::PlacementNew(
      value, node->GetVoidValue(size_info));
  auto* full_msg = DynamicCastMessage<Message>(new_msg);

  // If we are working with a full (non-lite) proto, we reflectively swap the
  // value into place. Otherwise, we have to perform a copy.
  if (full_msg != nullptr) {
    full_msg->GetReflection()->Swap(full_msg,
                                    DynamicCastMessage<Message>(value));
  } else {
    new_msg->CheckTypeAndMergeFrom(*value);
  }

  node = internal::RustMapHelper::InsertOrReplaceNode(
      static_cast<KeyMap<Key>*>(m), node);
  if (node == nullptr) {
    return true;
  }
  DestroyMapNode<Key>(m, node, size_info);
  return false;
}

template <typename Map, typename Key,
          typename = typename std::enable_if<
              !std::is_same<Key, google::protobuf::rust::PtrAndLen>::value>::type>
internal::RustMapHelper::NodeAndBucket FindHelper(Map* m, Key key) {
  return internal::RustMapHelper::FindHelper(
      m, static_cast<internal::KeyForBase<Key>>(key));
}

template <typename Map>
internal::RustMapHelper::NodeAndBucket FindHelper(Map* m,
                                                  google::protobuf::rust::PtrAndLen key) {
  return internal::RustMapHelper::FindHelper(
      m, absl::string_view(key.ptr, key.len));
}

template <typename Key>
bool Get(internal::UntypedMapBase* m, const google::protobuf::MessageLite* prototype,
         Key key, MessageLite** value) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo<typename FromViewType<Key>::type>(prototype);
  auto* map_base = static_cast<KeyMap<Key>*>(m);
  internal::RustMapHelper::NodeAndBucket result = FindHelper(map_base, key);
  if (result.node == nullptr) {
    return false;
  }
  *value = static_cast<MessageLite*>(result.node->GetVoidValue(size_info));
  return true;
}

template <typename Key>
bool Remove(internal::UntypedMapBase* m, const google::protobuf::MessageLite* prototype,
            Key key) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo<typename FromViewType<Key>::type>(prototype);
  auto* map_base = static_cast<KeyMap<Key>*>(m);
  internal::RustMapHelper::NodeAndBucket result = FindHelper(map_base, key);
  if (result.node == nullptr) {
    return false;
  }
  internal::RustMapHelper::EraseNoDestroy(map_base, result.bucket, result.node);
  DestroyMapNode<Key>(m, result.node, size_info);
  return true;
}

template <typename Key>
void IterGet(const internal::UntypedMapIterator* iter,
             const google::protobuf::MessageLite* prototype, Key* key,
             MessageLite** value) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo<typename FromViewType<Key>::type>(prototype);
  internal::NodeBase* node = iter->node_;
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    const std::string* s = static_cast<const std::string*>(node->GetVoidKey());
    *key = PtrAndLen{s->data(), s->size()};
  } else {
    *key = *static_cast<const Key*>(node->GetVoidKey());
  }
  *value = static_cast<MessageLite*>(node->GetVoidValue(size_info));
}

// LINT.IfChange(map_key_category)
enum class MapKeyCategory : uint8_t {
  kOneByte = 0,
  kFourBytes = 1,
  kEightBytes = 2,
  kStdString = 3,
};
// LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp.rs:map_key_category)

size_t KeySize(MapKeyCategory category) {
  switch (category) {
    case MapKeyCategory::kOneByte:
      return 1;
    case MapKeyCategory::kFourBytes:
      return 4;
    case MapKeyCategory::kEightBytes:
      return 8;
    case MapKeyCategory::kStdString:
      return sizeof(std::string);
    default:
      ABSL_DLOG(FATAL) << "Unexpected value of MapKeyCategory enum";
  }
}

void ClearMap(internal::UntypedMapBase* m, MapKeyCategory category,
              bool reset_table, const google::protobuf::MessageLite* prototype) {
  internal::MapNodeSizeInfoT size_info =
      GetSizeInfo(KeySize(category), prototype);
  if (internal::RustMapHelper::IsGlobalEmptyTable(m)) return;
  uint8_t bits = internal::RustMapHelper::kValueIsProto;
  if (category == MapKeyCategory::kStdString) {
    bits |= internal::RustMapHelper::kKeyIsString;
  }
  internal::RustMapHelper::ClearTable(
      m, internal::RustMapHelper::ClearInput{size_info, bits, reset_table,
                                             /* destroy_node = */ nullptr});
}

}  // namespace
}  // namespace rust
}  // namespace protobuf
}  // namespace google

extern "C" {

void proto2_rust_thunk_UntypedMapIterator_increment(
    google::protobuf::internal::UntypedMapIterator* iter) {
  iter->PlusPlus();
}

google::protobuf::internal::UntypedMapBase* proto2_rust_map_new() {
  return new google::protobuf::internal::UntypedMapBase(/* arena = */ nullptr);
}

void proto2_rust_map_free(google::protobuf::internal::UntypedMapBase* m,
                          google::protobuf::rust::MapKeyCategory category,
                          const google::protobuf::MessageLite* prototype) {
  google::protobuf::rust::ClearMap(m, category, /* reset_table = */ false, prototype);
  delete m;
}

void proto2_rust_map_clear(google::protobuf::internal::UntypedMapBase* m,
                           google::protobuf::rust::MapKeyCategory category,
                           const google::protobuf::MessageLite* prototype) {
  google::protobuf::rust::ClearMap(m, category, /* reset_table = */ true, prototype);
}

size_t proto2_rust_map_size(google::protobuf::internal::UntypedMapBase* m) {
  return m->size();
}

google::protobuf::internal::UntypedMapIterator proto2_rust_map_iter(
    google::protobuf::internal::UntypedMapBase* m) {
  return m->begin();
}

#define DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(cpp_type, suffix)                 \
  bool proto2_rust_map_insert_##suffix(google::protobuf::internal::UntypedMapBase* m,  \
                                       cpp_type key,                         \
                                       google::protobuf::MessageLite* value) {         \
    return google::protobuf::rust::Insert(m, key, value);                              \
  }                                                                          \
                                                                             \
  bool proto2_rust_map_get_##suffix(google::protobuf::internal::UntypedMapBase* m,     \
                                    const google::protobuf::MessageLite* prototype,    \
                                    cpp_type key,                            \
                                    google::protobuf::MessageLite** value) {           \
    return google::protobuf::rust::Get(m, prototype, key, value);                      \
  }                                                                          \
                                                                             \
  bool proto2_rust_map_remove_##suffix(google::protobuf::internal::UntypedMapBase* m,  \
                                       const google::protobuf::MessageLite* prototype, \
                                       cpp_type key) {                       \
    return google::protobuf::rust::Remove(m, prototype, key);                          \
  }                                                                          \
                                                                             \
  void proto2_rust_map_iter_get_##suffix(                                    \
      const google::protobuf::internal::UntypedMapIterator* iter,                      \
      const google::protobuf::MessageLite* prototype, cpp_type* key,                   \
      google::protobuf::MessageLite** value) {                                         \
    return google::protobuf::rust::IterGet(iter, prototype, key, value);               \
  }

DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(int32_t, i32)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(uint32_t, u32)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(int64_t, i64)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(uint64_t, u64)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(bool, bool)
DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(google::protobuf::rust::PtrAndLen, ProtoString)

__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int32_t, i32, int32_t,
                                                   int32_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint32_t, u32, uint32_t,
                                                   uint32_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(float, f32, float, float,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(double, f64, double, double,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(bool, bool, bool, bool,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint64_t, u64, uint64_t,
                                                   uint64_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int64_t, i64, int64_t,
                                                   int64_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoBytes, google::protobuf::rust::PtrAndLen, std::string*,
    std::move(*value),
    (google::protobuf::rust::PtrAndLen{cpp_value.data(), cpp_value.size()}));
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoString, google::protobuf::rust::PtrAndLen, std::string*,
    std::move(*value),
    (google::protobuf::rust::PtrAndLen{cpp_value.data(), cpp_value.size()}));

}  // extern "C"
