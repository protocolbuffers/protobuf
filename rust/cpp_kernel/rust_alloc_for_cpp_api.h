// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_RUST_CPP_KERNEL_RUST_ALLOC_FOR_CPP_API_H__
#define GOOGLE_PROTOBUF_RUST_CPP_KERNEL_RUST_ALLOC_FOR_CPP_API_H__

#include <cstddef>

// Allocates memory using the current Rust global allocator.
//
// This function is defined in `rust_alloc_for_cpp_api.rs`.
extern "C" void* proto2_rust_alloc(size_t size, size_t align);

#endif  // GOOGLE_PROTOBUF_RUST_CPP_KERNEL_RUST_ALLOC_FOR_CPP_API_H__
