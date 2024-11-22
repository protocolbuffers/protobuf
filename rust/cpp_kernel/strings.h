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
namespace rust {

// Represents an ABI-stable version of &[u8]/string_view (borrowed slice of
// bytes) for FFI use only.
struct PtrAndLen {
  /// Borrows the memory.
  const char* ptr;
  size_t len;
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

}  // namespace rust
}  // namespace protobuf
}  // namespace google

extern "C" {

// Allocates a new std::string on the C++ heap and returns a pointer to it.
std::string* proto2_rust_cpp_new_string(google::protobuf::rust::PtrAndLen src);

// Deletes a std::string object from the C++ heap.
void proto2_rust_cpp_delete_string(std::string* str);

// Obtain a PtrAndLen, the FFI-safe view type, from a std::string.
google::protobuf::rust::PtrAndLen proto2_rust_cpp_string_to_view(std::string* str);
}

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_STRINGS_H__
