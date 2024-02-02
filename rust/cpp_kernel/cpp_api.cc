#include "rust/cpp_kernel/cpp_api.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "google/protobuf/map.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"

extern "C" {

#define expose_repeated_field_methods(ty, rust_ty)                            \
  google::protobuf::RepeatedField<ty>* __pb_rust_RepeatedField_##rust_ty##_new() {      \
    return new google::protobuf::RepeatedField<ty>();                                   \
  }                                                                           \
  void __pb_rust_RepeatedField_##rust_ty##_free(                              \
      google::protobuf::RepeatedField<ty>* r) {                                         \
    delete r;                                                                 \
  }                                                                           \
  void __pb_rust_RepeatedField_##rust_ty##_add(google::protobuf::RepeatedField<ty>* r,  \
                                               ty val) {                      \
    r->Add(val);                                                              \
  }                                                                           \
  size_t __pb_rust_RepeatedField_##rust_ty##_size(                            \
      google::protobuf::RepeatedField<ty>* r) {                                         \
    return r->size();                                                         \
  }                                                                           \
  ty __pb_rust_RepeatedField_##rust_ty##_get(google::protobuf::RepeatedField<ty>* r,    \
                                             size_t index) {                  \
    return r->Get(index);                                                     \
  }                                                                           \
  void __pb_rust_RepeatedField_##rust_ty##_set(google::protobuf::RepeatedField<ty>* r,  \
                                               size_t index, ty val) {        \
    return r->Set(index, val);                                                \
  }                                                                           \
  void __pb_rust_RepeatedField_##rust_ty##_copy_from(                         \
      const google::protobuf::RepeatedField<ty>* src, google::protobuf::RepeatedField<ty>* dst) { \
    dst->CopyFrom(*src);                                                      \
  }                                                                           \
  void __pb_rust_RepeatedField_##rust_ty##_clear(                             \
      google::protobuf::RepeatedField<ty>* r) {                                         \
    r->Clear();                                                               \
  }

expose_repeated_field_methods(int32_t, i32);
expose_repeated_field_methods(uint32_t, u32);
expose_repeated_field_methods(float, f32);
expose_repeated_field_methods(double, f64);
expose_repeated_field_methods(bool, bool);
expose_repeated_field_methods(uint64_t, u64);
expose_repeated_field_methods(int64_t, i64);
#undef expose_repeated_field_methods

#define expose_repeated_ptr_field_methods(ty)                          \
  google::protobuf::RepeatedPtrField<std::string>*                               \
      __pb_rust_RepeatedField_##ty##_new() {                           \
    return new google::protobuf::RepeatedPtrField<std::string>();                \
  }                                                                    \
  void __pb_rust_RepeatedField_##ty##_free(                            \
      google::protobuf::RepeatedPtrField<std::string>* r) {                      \
    delete r;                                                          \
  }                                                                    \
  void __pb_rust_RepeatedField_##ty##_add(                             \
      google::protobuf::RepeatedPtrField<std::string>* r,                        \
      google::protobuf::rust_internal::PtrAndLen val) {                          \
    r->Add(std::string(val.ptr, val.len));                             \
  }                                                                    \
  size_t __pb_rust_RepeatedField_##ty##_size(                          \
      google::protobuf::RepeatedPtrField<std::string>* r) {                      \
    return r->size();                                                  \
  }                                                                    \
  google::protobuf::rust_internal::PtrAndLen __pb_rust_RepeatedField_##ty##_get( \
      google::protobuf::RepeatedPtrField<std::string>* r, size_t index) {        \
    const std::string& s = r->Get(index);                              \
    return google::protobuf::rust_internal::PtrAndLen(s.data(), s.size());       \
  }                                                                    \
  void __pb_rust_RepeatedField_##ty##_set(                             \
      google::protobuf::RepeatedPtrField<std::string>* r, size_t index,          \
      google::protobuf::rust_internal::PtrAndLen val) {                          \
    *r->Mutable(index) = std::string(val.ptr, val.len);                \
  }                                                                    \
  void __pb_rust_RepeatedField_##ty##_copy_from(                       \
      const google::protobuf::RepeatedPtrField<std::string>* src,                \
      google::protobuf::RepeatedPtrField<std::string>* dst) {                    \
    dst->CopyFrom(*src);                                               \
  }                                                                    \
  void __pb_rust_RepeatedField_##ty##_clear(                           \
      google::protobuf::RepeatedPtrField<std::string>* r) {                      \
    r->Clear();                                                        \
  }

expose_repeated_ptr_field_methods(ProtoStr);
expose_repeated_ptr_field_methods(Bytes);
#undef expose_repeated_field_methods

#undef expose_repeated_ptr_field_methods

#define expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key, \
                                  value_ty, rust_value_ty, ffi_value_ty,       \
                                  to_cpp_value, to_ffi_value)                  \
  google::protobuf::Map<key_ty, value_ty>*                                               \
      __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_new() {        \
    return new google::protobuf::Map<key_ty, value_ty>();                                \
  }                                                                            \
  void __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_free(         \
      google::protobuf::Map<key_ty, value_ty>* m) {                                      \
    delete m;                                                                  \
  }                                                                            \
  void __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_clear(        \
      google::protobuf::Map<key_ty, value_ty>* m) {                                      \
    m->clear();                                                                \
  }                                                                            \
  size_t __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_size(       \
      const google::protobuf::Map<key_ty, value_ty>* m) {                                \
    return m->size();                                                          \
  }                                                                            \
  bool __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_insert(       \
      google::protobuf::Map<key_ty, value_ty>* m, ffi_key_ty key, ffi_value_ty value) {  \
    auto iter_and_inserted = m->try_emplace(to_cpp_key, to_cpp_value);         \
    return iter_and_inserted.second;                                           \
  }                                                                            \
  bool __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_get(          \
      const google::protobuf::Map<key_ty, value_ty>* m, ffi_key_ty key,                  \
      ffi_value_ty* value) {                                                   \
    auto cpp_key = to_cpp_key;                                                 \
    auto it = m->find(cpp_key);                                                \
    if (it == m->end()) {                                                      \
      return false;                                                            \
    }                                                                          \
    auto& cpp_value = it->second;                                              \
    *value = to_ffi_value;                                                     \
    return true;                                                               \
  }                                                                            \
  bool __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_remove(       \
      google::protobuf::Map<key_ty, value_ty>* m, ffi_key_ty key, ffi_value_ty* value) { \
    auto cpp_key = to_cpp_key;                                                 \
    auto num_removed = m->erase(cpp_key);                                      \
    return num_removed > 0;                                                    \
  }

#define expose_scalar_map_methods_for_key_type(key_ty, rust_key_ty,            \
                                               ffi_key_ty, to_cpp_key)         \
  expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key,       \
                            int32_t, i32, int32_t, value, cpp_value);          \
  expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key,       \
                            uint32_t, u32, uint32_t, value, cpp_value);        \
  expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key,       \
                            float, f32, float, value, cpp_value);              \
  expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key,       \
                            double, f64, double, value, cpp_value);            \
  expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key, bool, \
                            bool, bool, value, cpp_value);                     \
  expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key,       \
                            uint64_t, u64, uint64_t, value, cpp_value);        \
  expose_scalar_map_methods(key_ty, rust_key_ty, ffi_key_ty, to_cpp_key,       \
                            int64_t, i64, int64_t, value, cpp_value);          \
  expose_scalar_map_methods(                                                   \
      key_ty, rust_key_ty, ffi_key_ty, to_cpp_key, std::string, Bytes,         \
      google::protobuf::rust_internal::PtrAndLen, std::string(value.ptr, value.len),     \
      google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));   \
  expose_scalar_map_methods(                                                   \
      key_ty, rust_key_ty, ffi_key_ty, to_cpp_key, std::string, ProtoStr,      \
      google::protobuf::rust_internal::PtrAndLen, std::string(value.ptr, value.len),     \
      google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));

expose_scalar_map_methods_for_key_type(int32_t, i32, int32_t, key);
expose_scalar_map_methods_for_key_type(uint32_t, u32, uint32_t, key);
expose_scalar_map_methods_for_key_type(bool, bool, bool, key);
expose_scalar_map_methods_for_key_type(uint64_t, u64, uint64_t, key);
expose_scalar_map_methods_for_key_type(int64_t, i64, int64_t, key);
expose_scalar_map_methods_for_key_type(std::string, ProtoStr,
                                       google::protobuf::rust_internal::PtrAndLen,
                                       std::string(key.ptr, key.len));

#undef expose_scalar_map_methods
#undef expose_map_methods
}
