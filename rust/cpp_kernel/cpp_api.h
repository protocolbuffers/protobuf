// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  /// Owns the memory.
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
