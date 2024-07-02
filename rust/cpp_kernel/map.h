#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_MAP_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_MAP_H__

// Defines concrete thunks to access typed map methods from Rust.
#define __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                  \
    key_ty, rust_key_ty, ffi_key_ty, to_cpp_key, to_ffi_key, value_ty,        \
    rust_value_ty, ffi_view_ty, ffi_value_ty, to_cpp_value, to_ffi_value)     \
  google::protobuf::Map<key_ty, value_ty>*                                              \
      __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_new() {       \
    return new google::protobuf::Map<key_ty, value_ty>();                               \
  }                                                                           \
  void __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_free(        \
      google::protobuf::Map<key_ty, value_ty>* m) {                                     \
    delete m;                                                                 \
  }                                                                           \
  void __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_clear(       \
      google::protobuf::Map<key_ty, value_ty>* m) {                                     \
    m->clear();                                                               \
  }                                                                           \
  size_t __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_size(      \
      const google::protobuf::Map<key_ty, value_ty>* m) {                               \
    return m->size();                                                         \
  }                                                                           \
  bool __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_insert(      \
      google::protobuf::Map<key_ty, value_ty>* m, ffi_key_ty key, ffi_value_ty value) { \
    auto iter_and_inserted = m->try_emplace(to_cpp_key, to_cpp_value);        \
    if (!iter_and_inserted.second) {                                          \
      iter_and_inserted.first->second = to_cpp_value;                         \
    }                                                                         \
    return iter_and_inserted.second;                                          \
  }                                                                           \
  bool __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_get(         \
      const google::protobuf::Map<key_ty, value_ty>* m, ffi_key_ty key,                 \
      ffi_view_ty* value) {                                                   \
    auto cpp_key = to_cpp_key;                                                \
    auto it = m->find(cpp_key);                                               \
    if (it == m->end()) {                                                     \
      return false;                                                           \
    }                                                                         \
    auto& cpp_value = it->second;                                             \
    *value = to_ffi_value;                                                    \
    return true;                                                              \
  }                                                                           \
  google::protobuf::internal::UntypedMapIterator                                        \
      __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_iter(         \
          const google::protobuf::Map<key_ty, value_ty>* m) {                           \
    return google::protobuf::internal::UntypedMapIterator::FromTyped(m->cbegin());      \
  }                                                                           \
  void __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_iter_get(    \
      const google::protobuf::internal::UntypedMapIterator* iter, ffi_key_ty* key,      \
      ffi_view_ty* value) {                                                   \
    auto typed_iter =                                                         \
        iter->ToTyped<google::protobuf::Map<key_ty, value_ty>::const_iterator>();       \
    const auto& cpp_key = typed_iter->first;                                  \
    const auto& cpp_value = typed_iter->second;                               \
    *key = to_ffi_key;                                                        \
    *value = to_ffi_value;                                                    \
  }                                                                           \
  bool __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_remove(      \
      google::protobuf::Map<key_ty, value_ty>* m, ffi_key_ty key, ffi_view_ty* value) { \
    auto cpp_key = to_cpp_key;                                                \
    auto num_removed = m->erase(cpp_key);                                     \
    return num_removed > 0;                                                   \
  }

// Defines the map thunks for all supported key types for a given value type.
#define __PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(                 \
    value_ty, rust_value_ty, ffi_view_ty, ffi_value_ty, to_cpp_value,       \
    to_ffi_value)                                                           \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                      \
      int32_t, i32, int32_t, key, cpp_key, value_ty, rust_value_ty,         \
      ffi_view_ty, ffi_value_ty, to_cpp_value, to_ffi_value);               \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                      \
      uint32_t, u32, uint32_t, key, cpp_key, value_ty, rust_value_ty,       \
      ffi_view_ty, ffi_value_ty, to_cpp_value, to_ffi_value);               \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                      \
      bool, bool, bool, key, cpp_key, value_ty, rust_value_ty, ffi_view_ty, \
      ffi_value_ty, to_cpp_value, to_ffi_value);                            \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                      \
      uint64_t, u64, uint64_t, key, cpp_key, value_ty, rust_value_ty,       \
      ffi_view_ty, ffi_value_ty, to_cpp_value, to_ffi_value);               \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                      \
      int64_t, i64, int64_t, key, cpp_key, value_ty, rust_value_ty,         \
      ffi_view_ty, ffi_value_ty, to_cpp_value, to_ffi_value);               \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                      \
      std::string, ProtoString, google::protobuf::rust_internal::PtrAndLen,           \
      std::string(key.ptr, key.len),                                        \
      google::protobuf::rust_internal::PtrAndLen(cpp_key.data(), cpp_key.size()),     \
      value_ty, rust_value_ty, ffi_view_ty, ffi_value_ty, to_cpp_value,     \
      to_ffi_value);

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_MAP_H__
