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
#include <iostream>
#include <string>

#include "absl/algorithm/container.h"

namespace google {
namespace protobuf {

// Represents a sequence of raw bytes.
//
// Example use-cases:
// * Return value of `<Message>.SerializeToString()`.
// * Parameter of setters for`bytes` fields.
//
// This struct is ABI compatible with the equivalend struct on the
// Rust side. It owns its data.
//
extern "C" struct Bytes {
  /// Owns the memory.
  const char* data;
  size_t size;
};

inline Bytes MakeBytesFromString(std::string byte_string) {
  size_t size = byte_string.size();
  char* data = (char*)malloc(sizeof(char) * (size));
  absl::c_copy(byte_string, data);
  Bytes result{.data = data, .size = size};
  return result;
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_CPP_H__
