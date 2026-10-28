// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_STRINGS_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_STRINGS_H__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace rust {

// Represents an ABI-stable version of &[u8]/string_view (borrowed slice of
// bytes) for FFI use only.
//
// Note that the intent is for either Rust or C++ to construct one of these with
// the pointer that they would have from a Rust slice or C++ string_view.
// Notably this means that if len==0, ptr may be any value, including nullptr or
// an invalid pointer, which may be a value incompatible for use with either a
// Rust slice or C++ string_view.
//
// It may be constructed trivially, but use the provided conversion methods
// when converting from this type into any other type to avoid obscure undefined
// behavior.
struct PtrAndLen {
  /// Borrows the memory.
  const char* ptr;
  size_t len;

  std::string CopyToString() const;
  absl::string_view AsStringView() const;
  void PlacementNewString(void* location);
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

// Tagged FFI payload describing any Rust `IntoProtoCord` value passed into C++.
//
enum class RawCordInputKind : uint8_t {
  kBorrowedSlice = 0,
  kStaticSlice = 1,
  kBorrowedCord = 2,
  kOwnedCord = 3,
  kRustVec = 4,
  kCppStdString = 5,
};

struct RawCordInput {
  RawCordInputKind kind;
  const void* ptr;
  size_t len;
  size_t cap;
};

// Consumes `input` and materializes an `absl::Cord`, transferring ownership or
// sharing reference counts without copying whenever supported by `input.kind`.
absl::Cord TakeCordInput(RawCordInput input);

struct CordChunkIter {
  absl::Cord::ChunkIterator current;
  absl::Cord::ChunkIterator end;
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

size_t proto2_rust_cord_len(const absl::Cord* cord);
bool proto2_rust_cord_try_flat(const absl::Cord* cord,
                               google::protobuf::rust::PtrAndLen* out);
void proto2_rust_cord_copy_to_bytes(const absl::Cord* cord, uint8_t* dst);
bool proto2_rust_cord_eq(const absl::Cord* lhs, const absl::Cord* rhs);
bool proto2_rust_cord_eq_bytes(const absl::Cord* lhs,
                               google::protobuf::rust::PtrAndLen rhs);

google::protobuf::rust::CordChunkIter* proto2_rust_cord_iter_new(const absl::Cord* cord);
bool proto2_rust_cord_iter_next(google::protobuf::rust::CordChunkIter* iter,
                                google::protobuf::rust::PtrAndLen* out);
google::protobuf::rust::CordChunkIter* proto2_rust_cord_iter_clone(
    const google::protobuf::rust::CordChunkIter* iter);
void proto2_rust_cord_iter_delete(google::protobuf::rust::CordChunkIter* iter);

absl::Cord* proto2_rust_cord_new_empty();
absl::Cord* proto2_rust_cord_new_from_input(google::protobuf::rust::RawCordInput input);
absl::Cord* proto2_rust_cord_clone(const absl::Cord* cord);
void proto2_rust_cord_swap(absl::Cord* lhs, absl::Cord* rhs);
void proto2_rust_cord_delete(absl::Cord* cord);
}

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_STRINGS_H__
