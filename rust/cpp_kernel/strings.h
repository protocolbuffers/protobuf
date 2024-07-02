// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_STRINGS_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_STRINGS_H__

#include <cstddef>
#include <cstring>
#include <string>

namespace google {
namespace protobuf {
namespace rust_internal {

// Represents an ABI-stable version of &[u8]/string_view (borrowed slice of
// bytes) for FFI use only.
struct PtrAndLen {
  /// Borrows the memory.
  const char* ptr;
  size_t len;

  PtrAndLen(const char* ptr, size_t len) : ptr(ptr), len(len) {}
};

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

}  // namespace rust_internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_STRINGS_H__
