// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_EXTENSION_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_EXTENSION_H__

#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"
#include "upb/reflection/def.hpp"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

// Generates code for a particular extension in `.pb.rs`.
void GenerateRs(Context& ctx, const FieldDescriptor& extension,
                const upb::DefPool& pool);

// Generates code for a particular extension in `.pb.thunk.cc`.
void GenerateThunksCc(Context& ctx, const FieldDescriptor& extension);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_EXTENSION_H__
