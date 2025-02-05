#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "rust/cpp_kernel/strings.h"

extern "C" {
#define expose_repeated_field_methods(ty, rust_ty)                             \
  google::protobuf::RepeatedField<ty>* proto2_rust_RepeatedField_##rust_ty##_new() {     \
    return new google::protobuf::RepeatedField<ty>();                                    \
  }                                                                            \
  void proto2_rust_RepeatedField_##rust_ty##_free(                             \
      google::protobuf::RepeatedField<ty>* r) {                                          \
    delete r;                                                                  \
  }                                                                            \
  void proto2_rust_RepeatedField_##rust_ty##_add(google::protobuf::RepeatedField<ty>* r, \
                                                 ty val) {                     \
    r->Add(val);                                                               \
  }                                                                            \
  size_t proto2_rust_RepeatedField_##rust_ty##_size(                           \
      google::protobuf::RepeatedField<ty>* r) {                                          \
    return r->size();                                                          \
  }                                                                            \
  ty proto2_rust_RepeatedField_##rust_ty##_get(google::protobuf::RepeatedField<ty>* r,   \
                                               size_t index) {                 \
    return r->Get(index);                                                      \
  }                                                                            \
  void proto2_rust_RepeatedField_##rust_ty##_set(google::protobuf::RepeatedField<ty>* r, \
                                                 size_t index, ty val) {       \
    return r->Set(index, val);                                                 \
  }                                                                            \
  void proto2_rust_RepeatedField_##rust_ty##_copy_from(                        \
      const google::protobuf::RepeatedField<ty>* src, google::protobuf::RepeatedField<ty>* dst) {  \
    dst->CopyFrom(*src);                                                       \
  }                                                                            \
  void proto2_rust_RepeatedField_##rust_ty##_clear(                            \
      google::protobuf::RepeatedField<ty>* r) {                                          \
    r->Clear();                                                                \
  }                                                                            \
  void proto2_rust_RepeatedField_##rust_ty##_reserve(                          \
      google::protobuf::RepeatedField<ty>* r, size_t additional) {                       \
    r->Reserve(r->size() + additional);                                        \
  }

expose_repeated_field_methods(int32_t, i32);
expose_repeated_field_methods(uint32_t, u32);
expose_repeated_field_methods(float, f32);
expose_repeated_field_methods(double, f64);
expose_repeated_field_methods(bool, bool);
expose_repeated_field_methods(uint64_t, u64);
expose_repeated_field_methods(int64_t, i64);
#undef expose_repeated_field_methods

#define expose_repeated_ptr_field_methods(ty)                        \
  google::protobuf::RepeatedPtrField<std::string>*                             \
      proto2_rust_RepeatedField_##ty##_new() {                       \
    return new google::protobuf::RepeatedPtrField<std::string>();              \
  }                                                                  \
  void proto2_rust_RepeatedField_##ty##_free(                        \
      google::protobuf::RepeatedPtrField<std::string>* r) {                    \
    delete r;                                                        \
  }                                                                  \
  void proto2_rust_RepeatedField_##ty##_add(                         \
      google::protobuf::RepeatedPtrField<std::string>* r, std::string* val) {  \
    r->AddAllocated(val);                                            \
  }                                                                  \
  size_t proto2_rust_RepeatedField_##ty##_size(                      \
      google::protobuf::RepeatedPtrField<std::string>* r) {                    \
    return r->size();                                                \
  }                                                                  \
  google::protobuf::rust::PtrAndLen proto2_rust_RepeatedField_##ty##_get(      \
      google::protobuf::RepeatedPtrField<std::string>* r, size_t index) {      \
    const std::string& s = r->Get(index);                            \
    return google::protobuf::rust::PtrAndLen{s.data(), s.size()};              \
  }                                                                  \
  void proto2_rust_RepeatedField_##ty##_set(                         \
      google::protobuf::RepeatedPtrField<std::string>* r, size_t index,        \
      std::string* val) {                                            \
    *r->Mutable(index) = std::move(*val);                            \
    delete val;                                                      \
  }                                                                  \
  void proto2_rust_RepeatedField_##ty##_copy_from(                   \
      const google::protobuf::RepeatedPtrField<std::string>* src,              \
      google::protobuf::RepeatedPtrField<std::string>* dst) {                  \
    dst->CopyFrom(*src);                                             \
  }                                                                  \
  void proto2_rust_RepeatedField_##ty##_clear(                       \
      google::protobuf::RepeatedPtrField<std::string>* r) {                    \
    r->Clear();                                                      \
  }                                                                  \
  void proto2_rust_RepeatedField_##ty##_reserve(                     \
      google::protobuf::RepeatedPtrField<std::string>* r, size_t additional) { \
    r->Reserve(r->size() + additional);                              \
  }

expose_repeated_ptr_field_methods(ProtoString);
expose_repeated_ptr_field_methods(ProtoBytes);
#undef expose_repeated_field_methods

#undef expose_repeated_ptr_field_methods

using google::protobuf::internal::GenericTypeHandler;
using google::protobuf::internal::RepeatedPtrFieldBase;
using google::protobuf::internal::RustRepeatedMessageHelper;

void* proto2_rust_RepeatedField_Message_new() {
  return RustRepeatedMessageHelper::New();
}

void proto2_rust_RepeatedField_Message_free(RepeatedPtrFieldBase* field) {
  RustRepeatedMessageHelper::Delete(field);
}

size_t proto2_rust_RepeatedField_Message_size(
    const RepeatedPtrFieldBase* field) {
  return RustRepeatedMessageHelper::Size(*field);
}

const google::protobuf::MessageLite* proto2_rust_RepeatedField_Message_get(
    const RepeatedPtrFieldBase* field, size_t index) {
  return &RustRepeatedMessageHelper::At(*field, index);
}

google::protobuf::MessageLite* proto2_rust_RepeatedField_Message_get_mut(
    RepeatedPtrFieldBase* field, size_t index) {
  return &RustRepeatedMessageHelper::At(*field, index);
}

google::protobuf::MessageLite* proto2_rust_RepeatedField_Message_add(
    RepeatedPtrFieldBase* field, const google::protobuf::MessageLite* prototype) {
  return field->AddMessage(prototype);
}

void proto2_rust_RepeatedField_Message_clear(RepeatedPtrFieldBase* field) {
  field->Clear<GenericTypeHandler<google::protobuf::MessageLite>>();
}

void proto2_rust_RepeatedField_Message_copy_from(
    RepeatedPtrFieldBase* dst, const RepeatedPtrFieldBase* src) {
  dst->Clear<GenericTypeHandler<google::protobuf::MessageLite>>();
  dst->MergeFrom<google::protobuf::MessageLite>(*src);
}

void proto2_rust_RepeatedField_Message_reserve(RepeatedPtrFieldBase* field,
                                               size_t additional) {
  RustRepeatedMessageHelper::Reserve(*field, additional);
}

}  // extern "C"
