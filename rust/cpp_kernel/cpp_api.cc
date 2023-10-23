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
}
