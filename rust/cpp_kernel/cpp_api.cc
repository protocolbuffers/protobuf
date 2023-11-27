#include "google/protobuf/map.h"
#include "google/protobuf/repeated_field.h"

extern "C" {

#define expose_repeated_field_methods(ty, rust_ty)                            \
  google::protobuf::RepeatedField<ty>* __pb_rust_RepeatedField_##rust_ty##_new() {      \
    return new google::protobuf::RepeatedField<ty>();                                   \
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
      google::protobuf::RepeatedField<ty> const& src, google::protobuf::RepeatedField<ty>& dst) { \
    dst.CopyFrom(src);                                                        \
  }

expose_repeated_field_methods(int32_t, i32);
expose_repeated_field_methods(uint32_t, u32);
expose_repeated_field_methods(float, f32);
expose_repeated_field_methods(double, f64);
expose_repeated_field_methods(bool, bool);
expose_repeated_field_methods(uint64_t, u64);
expose_repeated_field_methods(int64_t, i64);

#undef expose_repeated_field_methods

#define expose_scalar_map_methods(key_ty, rust_key_ty, value_ty,       \
                                  rust_value_ty)                       \
  google::protobuf::Map<key_ty, value_ty>*                                       \
      __pb_rust_Map_##rust_key_ty##_##rust_value_ty##_new() {          \
    return new google::protobuf::Map<key_ty, value_ty>();                        \
  }                                                                    \
  void __pb_rust_Map_##rust_key_ty##_##rust_value_ty##_clear(          \
      google::protobuf::Map<key_ty, value_ty>* m) {                              \
    m->clear();                                                        \
  }                                                                    \
  size_t __pb_rust_Map_##rust_key_ty##_##rust_value_ty##_size(         \
      google::protobuf::Map<key_ty, value_ty>* m) {                              \
    return m->size();                                                  \
  }                                                                    \
  void __pb_rust_Map_##rust_key_ty##_##rust_value_ty##_insert(         \
      google::protobuf::Map<key_ty, value_ty>* m, key_ty key, value_ty val) {    \
    (*m)[key] = val;                                                   \
  }                                                                    \
  bool __pb_rust_Map_##rust_key_ty##_##rust_value_ty##_get(            \
      google::protobuf::Map<key_ty, value_ty>* m, key_ty key, value_ty* value) { \
    auto it = m->find(key);                                            \
    if (it == m->end()) {                                              \
      return false;                                                    \
    }                                                                  \
    *value = it->second;                                               \
    return true;                                                       \
  }                                                                    \
  bool __pb_rust_Map_##rust_key_ty##_##rust_value_ty##_remove(         \
      google::protobuf::Map<key_ty, value_ty>* m, key_ty key, value_ty* value) { \
    auto it = m->find(key);                                            \
    if (it == m->end()) {                                              \
      return false;                                                    \
    } else {                                                           \
      *value = it->second;                                             \
      m->erase(it);                                                    \
      return true;                                                     \
    }                                                                  \
  }

#define expose_scalar_map_methods_for_key_type(key_ty, rust_key_ty) \
  expose_scalar_map_methods(key_ty, rust_key_ty, int32_t, i32);     \
  expose_scalar_map_methods(key_ty, rust_key_ty, uint32_t, u32);    \
  expose_scalar_map_methods(key_ty, rust_key_ty, float, f32);       \
  expose_scalar_map_methods(key_ty, rust_key_ty, double, f64);      \
  expose_scalar_map_methods(key_ty, rust_key_ty, bool, bool);       \
  expose_scalar_map_methods(key_ty, rust_key_ty, uint64_t, u64);    \
  expose_scalar_map_methods(key_ty, rust_key_ty, int64_t, i64);

expose_scalar_map_methods_for_key_type(int32_t, i32);
expose_scalar_map_methods_for_key_type(uint32_t, u32);
expose_scalar_map_methods_for_key_type(bool, bool);
expose_scalar_map_methods_for_key_type(uint64_t, u64);
expose_scalar_map_methods_for_key_type(int64_t, i64);

#undef expose_scalar_map_methods
#undef expose_map_methods
}
