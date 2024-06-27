#include "rust/cpp_kernel/map.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "google/protobuf/map.h"
#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/strings.h"

namespace {

template <typename T>
struct FromViewType {
  using type = T;
};

template <>
struct FromViewType<google::protobuf::rust::PtrAndLen> {
  using type = std::string;
};

// Insert
template <typename Key>
bool proto2_rust_map_insert(google::protobuf::internal::UntypedMapBase* m,
                            google::protobuf::internal::MapNodeSizeInfoT size_info,
                            Key key, google::protobuf::MessageLite* value,
                            void (*placement_new)(void*,
                                                  google::protobuf::MessageLite*)) {
  using MapBase = google::protobuf::internal::KeyMapBase<
      google::protobuf::internal::KeyForBase<typename FromViewType<Key>::type>>;
  google::protobuf::internal::NodeBase* node =
      google::protobuf::rust::MapVisibility::AllocNode(m, size_info);
  if constexpr (std::is_same<Key, google::protobuf::rust::PtrAndLen>::value) {
    new (node->GetVoidKey()) std::string(key.ptr, key.len);
  } else {
    *static_cast<Key*>(node->GetVoidKey()) = key;
  }
  void* value_ptr = node->GetVoidValue(size_info);
  placement_new(value_ptr, value);
  node = google::protobuf::rust::MapVisibility::InsertOrReplaceNode(
      static_cast<MapBase*>(m), node);
  if (node == nullptr) {
    return true;
  }
  if constexpr (std::is_same<Key, google::protobuf::rust::PtrAndLen>::value) {
    static_cast<std::string*>(node->GetVoidKey())->~basic_string();
  }
  google::protobuf::rust::MapVisibility::DestroyMessage(
      static_cast<google::protobuf::MessageLite*>(node->GetVoidValue(size_info)));
  google::protobuf::rust::MapVisibility::DeallocNode(m, node, size_info);
  return false;
}

template <typename Key>
bool proto2_rust_map_get(google::protobuf::internal::UntypedMapBase* m,
                         google::protobuf::internal::MapNodeSizeInfoT size_info, Key key,
                         google::protobuf::MessageLite** value) {
  using KeyForBase =
      google::protobuf::internal::KeyForBase<typename FromViewType<Key>::type>;
  using MapBase = google::protobuf::internal::KeyMapBase<KeyForBase>;
  google::protobuf::rust::MapVisibility::NodeAndBucket result;
  if constexpr (std::is_same<Key, google::protobuf::rust::PtrAndLen>::value) {
    result = google::protobuf::rust::MapVisibility::FindHelper(
        static_cast<MapBase*>(m), absl::string_view(key.ptr, key.len));
  } else {
    result = google::protobuf::rust::MapVisibility::FindHelper(
        static_cast<MapBase*>(m), static_cast<KeyForBase>(key));
  }
  if (result.node == nullptr) {
    return false;
  }
  *value =
      static_cast<google::protobuf::MessageLite*>(result.node->GetVoidValue(size_info));
  return true;
}

template <typename Key>
bool proto2_rust_map_remove(google::protobuf::internal::UntypedMapBase* m,
                            google::protobuf::internal::MapNodeSizeInfoT size_info,
                            Key key) {
  using KeyForBase =
      google::protobuf::internal::KeyForBase<typename FromViewType<Key>::type>;
  using MapBase = google::protobuf::internal::KeyMapBase<KeyForBase>;
  auto* map_base = static_cast<MapBase*>(m);
  google::protobuf::rust::MapVisibility::NodeAndBucket result;
  if constexpr (std::is_same<Key, google::protobuf::rust::PtrAndLen>::value) {
    result = google::protobuf::rust::MapVisibility::FindHelper(
        map_base, absl::string_view(key.ptr, key.len));
  } else {
    result = google::protobuf::rust::MapVisibility::FindHelper(
        map_base, static_cast<KeyForBase>(key));
  }
  if (result.node == nullptr) {
    return false;
  }
  google::protobuf::rust::MapVisibility::EraseNoDestroy(map_base, result.bucket,
                                              result.node);
  if constexpr (std::is_same<Key, google::protobuf::rust::PtrAndLen>::value) {
    static_cast<std::string*>(result.node->GetVoidKey())->~basic_string();
  }
  google::protobuf::rust::MapVisibility::DestroyMessage(
      static_cast<google::protobuf::MessageLite*>(result.node->GetVoidValue(size_info)));
  google::protobuf::rust::MapVisibility::DeallocNode(m, result.node, size_info);
  return true;
}

template <typename Key>
void map_iter_get(const google::protobuf::internal::UntypedMapIterator* iter,
                  google::protobuf::internal::MapNodeSizeInfoT size_info, Key* key,
                  google::protobuf::MessageLite** value) {
  google::protobuf::internal::NodeBase* node = iter->node_;
  if constexpr (std::is_same<Key, google::protobuf::rust::PtrAndLen>::value) {
    const std::string* s = static_cast<const std::string*>(node->GetVoidKey());
    *key = google::protobuf::rust::PtrAndLen(s->data(), s->size());
  } else {
    *key = *static_cast<const Key*>(node->GetVoidKey());
  }
  *value = static_cast<google::protobuf::MessageLite*>(node->GetVoidValue(size_info));
}

}  // namespace

extern "C" {

void proto2_rust_thunk_UntypedMapIterator_increment(
    google::protobuf::internal::UntypedMapIterator* iter) {
  iter->PlusPlus();
}

google::protobuf::internal::UntypedMapBase* proto2_rust_map_new() {
  return new google::protobuf::internal::UntypedMapBase(nullptr);
}

void proto2_rust_map_free(google::protobuf::internal::UntypedMapBase* m) {
  // FIXME
  // This probably leaks memory, because we're not doing anything to destroy
  // the contents of the map like we do in clear() below. This may be a gap in
  // the test coverage.
  delete m;
}

void proto2_rust_map_clear(google::protobuf::internal::UntypedMapBase* m,
                           bool key_is_string,
                           google::protobuf::internal::MapNodeSizeInfoT size_info) {
  if (google::protobuf::rust::MapVisibility::IsEmpty(m)) return;
  uint8_t bits = google::protobuf::rust::MapVisibility::kValueIsProto;
  if (key_is_string) {
    bits |= google::protobuf::rust::MapVisibility::kKeyIsString;
  }
  google::protobuf::rust::MapVisibility::ClearTable(
      m,
      google::protobuf::rust::MapVisibility::ClearInput{size_info, bits, true, nullptr});
}

size_t proto2_rust_map_size(google::protobuf::internal::UntypedMapBase* m) {
  return m->size();
}

google::protobuf::internal::UntypedMapIterator proto2_rust_map_iter(
    google::protobuf::internal::UntypedMapBase* m) {
  return m->begin();
}

// Insert
bool proto2_rust_map_insert_i32(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                int32_t key, google::protobuf::MessageLite* value,
                                void (*placement_new)(void*,
                                                      google::protobuf::MessageLite*)) {
  return proto2_rust_map_insert(m, size_info, key, value, placement_new);
}

bool proto2_rust_map_insert_u32(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                uint32_t key, google::protobuf::MessageLite* value,
                                void (*placement_new)(void*,
                                                      google::protobuf::MessageLite*)) {
  return proto2_rust_map_insert(m, size_info, key, value, placement_new);
}

bool proto2_rust_map_insert_i64(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                int64_t key, google::protobuf::MessageLite* value,
                                void (*placement_new)(void*,
                                                      google::protobuf::MessageLite*)) {
  return proto2_rust_map_insert(m, size_info, key, value, placement_new);
}

bool proto2_rust_map_insert_u64(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                uint64_t key, google::protobuf::MessageLite* value,
                                void (*placement_new)(void*,
                                                      google::protobuf::MessageLite*)) {
  return proto2_rust_map_insert(m, size_info, key, value, placement_new);
}

bool proto2_rust_map_insert_bool(google::protobuf::internal::UntypedMapBase* m,
                                 google::protobuf::internal::MapNodeSizeInfoT size_info,
                                 bool key, google::protobuf::MessageLite* value,
                                 void (*placement_new)(void*,
                                                       google::protobuf::MessageLite*)) {
  return proto2_rust_map_insert(m, size_info, key, value, placement_new);
}

bool proto2_rust_map_insert_ProtoString(
    google::protobuf::internal::UntypedMapBase* m,
    google::protobuf::internal::MapNodeSizeInfoT size_info, google::protobuf::rust::PtrAndLen key,
    google::protobuf::MessageLite* value,
    void (*placement_new)(void*, google::protobuf::MessageLite*)) {
  return proto2_rust_map_insert(m, size_info, key, value, placement_new);
}

// Get
bool proto2_rust_map_get_i32(google::protobuf::internal::UntypedMapBase* m,
                             google::protobuf::internal::MapNodeSizeInfoT size_info,
                             int32_t key, google::protobuf::MessageLite** value) {
  return proto2_rust_map_get(m, size_info, key, value);
}

bool proto2_rust_map_get_u32(google::protobuf::internal::UntypedMapBase* m,
                             google::protobuf::internal::MapNodeSizeInfoT size_info,
                             uint32_t key, google::protobuf::MessageLite** value) {
  return proto2_rust_map_get(m, size_info, key, value);
}

bool proto2_rust_map_get_i64(google::protobuf::internal::UntypedMapBase* m,
                             google::protobuf::internal::MapNodeSizeInfoT size_info,
                             int64_t key, google::protobuf::MessageLite** value) {
  return proto2_rust_map_get(m, size_info, key, value);
}

bool proto2_rust_map_get_u64(google::protobuf::internal::UntypedMapBase* m,
                             google::protobuf::internal::MapNodeSizeInfoT size_info,
                             uint64_t key, google::protobuf::MessageLite** value) {
  return proto2_rust_map_get(m, size_info, key, value);
}

bool proto2_rust_map_get_bool(google::protobuf::internal::UntypedMapBase* m,
                              google::protobuf::internal::MapNodeSizeInfoT size_info,
                              bool key, google::protobuf::MessageLite** value) {
  return proto2_rust_map_get(m, size_info, key, value);
}

bool proto2_rust_map_get_ProtoString(
    google::protobuf::internal::UntypedMapBase* m,
    google::protobuf::internal::MapNodeSizeInfoT size_info, google::protobuf::rust::PtrAndLen key,
    google::protobuf::MessageLite** value) {
  return proto2_rust_map_get(m, size_info, key, value);
}

// Remove
bool proto2_rust_map_remove_i32(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                int32_t key) {
  return proto2_rust_map_remove(m, size_info, key);
}

bool proto2_rust_map_remove_u32(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                uint32_t key) {
  return proto2_rust_map_remove(m, size_info, key);
}

bool proto2_rust_map_remove_i64(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                int64_t key) {
  return proto2_rust_map_remove(m, size_info, key);
}

bool proto2_rust_map_remove_u64(google::protobuf::internal::UntypedMapBase* m,
                                google::protobuf::internal::MapNodeSizeInfoT size_info,
                                uint64_t key) {
  return proto2_rust_map_remove(m, size_info, key);
}

bool proto2_rust_map_remove_bool(google::protobuf::internal::UntypedMapBase* m,
                                 google::protobuf::internal::MapNodeSizeInfoT size_info,
                                 bool key) {
  return proto2_rust_map_remove(m, size_info, key);
}

bool proto2_rust_map_remove_ProtoString(
    google::protobuf::internal::UntypedMapBase* m,
    google::protobuf::internal::MapNodeSizeInfoT size_info, google::protobuf::rust::PtrAndLen key) {
  return proto2_rust_map_remove(m, size_info, key);
}

// Iter Get
void proto2_rust_map_iter_get_i32(
    const google::protobuf::internal::UntypedMapIterator* iter,
    google::protobuf::internal::MapNodeSizeInfoT size_info, int32_t* key,
    google::protobuf::MessageLite** value) {
  return map_iter_get(iter, size_info, key, value);
}

void proto2_rust_map_iter_get_u32(
    const google::protobuf::internal::UntypedMapIterator* iter,
    google::protobuf::internal::MapNodeSizeInfoT size_info, uint32_t* key,
    google::protobuf::MessageLite** value) {
  return map_iter_get(iter, size_info, key, value);
}

void proto2_rust_map_iter_get_i64(
    const google::protobuf::internal::UntypedMapIterator* iter,
    google::protobuf::internal::MapNodeSizeInfoT size_info, int64_t* key,
    google::protobuf::MessageLite** value) {
  return map_iter_get(iter, size_info, key, value);
}

void proto2_rust_map_iter_get_u64(
    const google::protobuf::internal::UntypedMapIterator* iter,
    google::protobuf::internal::MapNodeSizeInfoT size_info, uint64_t* key,
    google::protobuf::MessageLite** value) {
  return map_iter_get(iter, size_info, key, value);
}

void proto2_rust_map_iter_get_bool(
    const google::protobuf::internal::UntypedMapIterator* iter,
    google::protobuf::internal::MapNodeSizeInfoT size_info, bool* key,
    google::protobuf::MessageLite** value) {
  return map_iter_get(iter, size_info, key, value);
}

void proto2_rust_map_iter_get_ProtoString(
    const google::protobuf::internal::UntypedMapIterator* iter,
    google::protobuf::internal::MapNodeSizeInfoT size_info, google::protobuf::rust::PtrAndLen* key,
    google::protobuf::MessageLite** value) {
  return map_iter_get(iter, size_info, key, value);
}

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
    google::protobuf::rust::PtrAndLen(cpp_value.data(), cpp_value.size()));
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoString, google::protobuf::rust::PtrAndLen, std::string*,
    std::move(*value),
    google::protobuf::rust::PtrAndLen(cpp_value.data(), cpp_value.size()));

}  // extern "C"
