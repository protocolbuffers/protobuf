#include "rust/cpp_kernel/cpp_api.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "google/protobuf/map.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
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
  }                                                                           \
  void __pb_rust_RepeatedField_##rust_ty##_reserve(                           \
      google::protobuf::RepeatedField<ty>* r, size_t additional) {                      \
    r->Reserve(r->size() + additional);                                       \
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
  }                                                                    \
  void __pb_rust_RepeatedField_##ty##_reserve(                         \
      google::protobuf::RepeatedPtrField<std::string>* r, size_t additional) {   \
    r->Reserve(r->size() + additional);                                \
  }

expose_repeated_ptr_field_methods(ProtoStr);
expose_repeated_ptr_field_methods(Bytes);
#undef expose_repeated_field_methods

#undef expose_repeated_ptr_field_methods

void __rust_proto_thunk__UntypedMapIterator_increment(
    google::protobuf::internal::UntypedMapIterator* iter) {
  iter->PlusPlus();
}

__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int32_t, i32, int32_t, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint32_t, u32, uint32_t,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(float, f32, float, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(double, f64, double, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(bool, bool, bool, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint64_t, u64, uint64_t,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int64_t, i64, int64_t, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, Bytes, google::protobuf::rust_internal::PtrAndLen,
    std::string(value.ptr, value.len),
    google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoStr, google::protobuf::rust_internal::PtrAndLen,
    std::string(value.ptr, value.len),
    google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));

#undef expose_scalar_map_methods
#undef expose_map_methods

google::protobuf::rust_internal::RustStringRawParts utf8_debug_string(
    const google::protobuf::Message* msg) {
  std::string text = google::protobuf::Utf8Format(*msg);
  return google::protobuf::rust_internal::RustStringRawParts(text);
}

google::protobuf::rust_internal::RustStringRawParts utf8_debug_string_lite(
    const google::protobuf::MessageLite* msg) {
  std::string text = google::protobuf::Utf8Format(*msg);
  return google::protobuf::rust_internal::RustStringRawParts(text);
}
}

namespace google {
namespace protobuf {
namespace rust_internal {

RustStringRawParts::RustStringRawParts(std::string src) {
  if (src.empty()) {
    data = nullptr;
    len = 0;
  } else {
    void* data_ = google::protobuf::rust_internal::__pb_rust_alloc(src.length(), 1);
    std::memcpy(data_, src.data(), src.length());
    data = static_cast<char*>(data_);
    len = src.length();
  }
}

}  // namespace rust_internal
}  // namespace protobuf
}  // namespace google
