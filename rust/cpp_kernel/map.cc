#include "rust/cpp_kernel/map.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/map.h"
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
bool Insert(internal::UntypedMapBase* m, internal::MapNodeSizeInfoT size_info,
            Key key, MessageLite* value,
            void (*placement_new)(void*, MessageLite*)) {
  internal::NodeBase* node = internal::RustMapHelper::AllocNode(m, size_info);
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    new (node->GetVoidKey()) std::string(key.ptr, key.len);
  } else {
    *static_cast<Key*>(node->GetVoidKey()) = key;
  }
  void* value_ptr = node->GetVoidValue(size_info);
  placement_new(value_ptr, value);
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
bool Get(internal::UntypedMapBase* m, internal::MapNodeSizeInfoT size_info,
         Key key, MessageLite** value) {
  auto* map_base = static_cast<KeyMap<Key>*>(m);
  internal::RustMapHelper::NodeAndBucket result = FindHelper(map_base, key);
  if (result.node == nullptr) {
    return false;
  }
  *value = static_cast<MessageLite*>(result.node->GetVoidValue(size_info));
  return true;
}

template <typename Key>
bool Remove(internal::UntypedMapBase* m, internal::MapNodeSizeInfoT size_info,
            Key key) {
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
             internal::MapNodeSizeInfoT size_info, Key* key,
             MessageLite** value) {
  internal::NodeBase* node = iter->node_;
  if constexpr (std::is_same<Key, PtrAndLen>::value) {
    const std::string* s = static_cast<const std::string*>(node->GetVoidKey());
    *key = PtrAndLen{s->data(), s->size()};
  } else {
    *key = *static_cast<const Key*>(node->GetVoidKey());
  }
  *value = static_cast<MessageLite*>(node->GetVoidValue(size_info));
}

void ClearMap(internal::UntypedMapBase* m, internal::MapNodeSizeInfoT size_info,
              bool key_is_string, bool reset_table) {
  if (internal::RustMapHelper::IsGlobalEmptyTable(m)) return;
  uint8_t bits = internal::RustMapHelper::kValueIsProto;
  if (key_is_string) {
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
                          bool key_is_string,
                          google::protobuf::internal::MapNodeSizeInfoT size_info) {
  google::protobuf::rust::ClearMap(m, size_info, key_is_string,
                         /* reset_table = */ false);
  delete m;
}

void proto2_rust_map_clear(google::protobuf::internal::UntypedMapBase* m,
                           bool key_is_string,
                           google::protobuf::internal::MapNodeSizeInfoT size_info) {
  google::protobuf::rust::ClearMap(m, size_info, key_is_string, /* reset_table = */ true);
}

size_t proto2_rust_map_size(google::protobuf::internal::UntypedMapBase* m) {
  return m->size();
}

google::protobuf::internal::UntypedMapIterator proto2_rust_map_iter(
    google::protobuf::internal::UntypedMapBase* m) {
  return m->begin();
}

#define DEFINE_KEY_SPECIFIC_MAP_OPERATIONS(cpp_type, suffix)              \
  bool proto2_rust_map_insert_##suffix(                                   \
      google::protobuf::internal::UntypedMapBase* m,                                \
      google::protobuf::internal::MapNodeSizeInfoT size_info, cpp_type key,         \
      google::protobuf::MessageLite* value,                                         \
      void (*placement_new)(void*, google::protobuf::MessageLite*)) {               \
    return google::protobuf::rust::Insert(m, size_info, key, value, placement_new); \
  }                                                                       \
                                                                          \
  bool proto2_rust_map_get_##suffix(                                      \
      google::protobuf::internal::UntypedMapBase* m,                                \
      google::protobuf::internal::MapNodeSizeInfoT size_info, cpp_type key,         \
      google::protobuf::MessageLite** value) {                                      \
    return google::protobuf::rust::Get(m, size_info, key, value);                   \
  }                                                                       \
                                                                          \
  bool proto2_rust_map_remove_##suffix(                                   \
      google::protobuf::internal::UntypedMapBase* m,                                \
      google::protobuf::internal::MapNodeSizeInfoT size_info, cpp_type key) {       \
    return google::protobuf::rust::Remove(m, size_info, key);                       \
  }                                                                       \
                                                                          \
  void proto2_rust_map_iter_get_##suffix(                                 \
      const google::protobuf::internal::UntypedMapIterator* iter,                   \
      google::protobuf::internal::MapNodeSizeInfoT size_info, cpp_type* key,        \
      google::protobuf::MessageLite** value) {                                      \
    return google::protobuf::rust::IterGet(iter, size_info, key, value);            \
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
