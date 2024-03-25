// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains support code for generated C++ thunks.

#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_CPP_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_CPP_H__

#include <cstddef>
#include <cstring>
#include <string>

#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace rust_internal {

// Represents serialized Protobuf wire format data.
//
// Only to be used to transfer serialized data from C++ to Rust under these
// assumptions:
// * Rust and C++ versions of this struct are ABI compatible.
// * Rust version owns and frees its data.
// * The data were allocated using the Rust allocator.
//
extern "C" struct SerializedData {
  // Owns the memory.
  const char* data;
  size_t len;

  SerializedData(const char* data, size_t len) : data(data), len(len) {}
};

// Allocates memory using the current Rust global allocator.
//
// This function is defined in `rust_alloc_for_cpp_api.rs`.
extern "C" void* __pb_rust_alloc(size_t size, size_t align);

inline SerializedData SerializeMsg(const google::protobuf::Message* msg) {
  size_t len = msg->ByteSizeLong();
  void* bytes = __pb_rust_alloc(len, alignof(char));
  if (bytes == nullptr) {
    ABSL_LOG(FATAL) << "Rust allocator failed to allocate memory.";
  }
  if (!msg->SerializeToArray(bytes, static_cast<int>(len))) {
    ABSL_LOG(FATAL) << "Couldn't serialize the message.";
  }
  return SerializedData(static_cast<char*>(bytes), len);
}

// Represents an ABI-stable version of &[u8]/string_view (borrowed slice of
// bytes) for FFI use only.
struct PtrAndLen {
  /// Borrows the memory.
  const char* ptr;
  size_t len;

  PtrAndLen(const char* ptr, size_t len) : ptr(ptr), len(len) {}
};

// Defines concrete thunks to access typed map methods from Rust.
#define __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                   \
    key_ty, rust_key_ty, ffi_key_ty, to_cpp_key, to_ffi_key, value_ty,         \
    rust_value_ty, ffi_value_ty, to_cpp_value, to_ffi_value)                   \
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
    if (!iter_and_inserted.second) {                                           \
      iter_and_inserted.first->second = to_cpp_value;                          \
    }                                                                          \
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
  google::protobuf::internal::UntypedMapIterator                                         \
      __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_iter(          \
          const google::protobuf::Map<key_ty, value_ty>* m) {                            \
    return google::protobuf::internal::UntypedMapIterator::FromTyped(m->cbegin());       \
  }                                                                            \
  void __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_iter_get(     \
      const google::protobuf::internal::UntypedMapIterator* iter, ffi_key_ty* key,       \
      ffi_value_ty* value) {                                                   \
    auto typed_iter =                                                          \
        iter->ToTyped<google::protobuf::Map<key_ty, value_ty>::const_iterator>();        \
    const auto& cpp_key = typed_iter->first;                                   \
    const auto& cpp_value = typed_iter->second;                                \
    *key = to_ffi_key;                                                         \
    *value = to_ffi_value;                                                     \
  }                                                                            \
  bool __rust_proto_thunk__Map_##rust_key_ty##_##rust_value_ty##_remove(       \
      google::protobuf::Map<key_ty, value_ty>* m, ffi_key_ty key, ffi_value_ty* value) { \
    auto cpp_key = to_cpp_key;                                                 \
    auto num_removed = m->erase(cpp_key);                                      \
    return num_removed > 0;                                                    \
  }

// Defines the map thunks for all supported key types for a given value type.
#define __PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(                  \
    value_ty, rust_value_ty, ffi_value_ty, to_cpp_value, to_ffi_value)       \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(int32_t, i32, int32_t, key, cpp_key,   \
                                      value_ty, rust_value_ty, ffi_value_ty, \
                                      to_cpp_value, to_ffi_value);           \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(uint32_t, u32, uint32_t, key, cpp_key, \
                                      value_ty, rust_value_ty, ffi_value_ty, \
                                      to_cpp_value, to_ffi_value);           \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(bool, bool, bool, key, cpp_key,        \
                                      value_ty, rust_value_ty, ffi_value_ty, \
                                      to_cpp_value, to_ffi_value);           \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(uint64_t, u64, uint64_t, key, cpp_key, \
                                      value_ty, rust_value_ty, ffi_value_ty, \
                                      to_cpp_value, to_ffi_value);           \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(int64_t, i64, int64_t, key, cpp_key,   \
                                      value_ty, rust_value_ty, ffi_value_ty, \
                                      to_cpp_value, to_ffi_value);           \
  __PB_RUST_EXPOSE_SCALAR_MAP_METHODS(                                       \
      std::string, ProtoStr, google::protobuf::rust_internal::PtrAndLen,               \
      std::string(key.ptr, key.len),                                         \
      google::protobuf::rust_internal::PtrAndLen(cpp_key.data(), cpp_key.size()),      \
      value_ty, rust_value_ty, ffi_value_ty, to_cpp_value, to_ffi_value);

// Represents an owned string for FFI purposes.
//
// This must only be used to transfer a string from C++ to Rust. The
// below invariants must hold:
//   * Rust and C++ versions of this struct are ABI compatible.
//   * The data were allocated using the Rust allocator and are 1 byte aligned.
//   * The data is valid UTF-8.
struct RustStringRawParts {
  // Owns the memory.
  const char* data;
  size_t len;

  RustStringRawParts() = delete;
  // Copies src.
  explicit RustStringRawParts(std::string src);
};

extern "C" RustStringRawParts utf8_debug_string(const google::protobuf::Message* msg);
}  // namespace rust_internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_CPP_H__
