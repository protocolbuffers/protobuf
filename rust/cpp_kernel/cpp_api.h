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

}  // namespace rust_internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_CPP_H__
