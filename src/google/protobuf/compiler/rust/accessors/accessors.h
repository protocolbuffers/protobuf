// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSORS_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSORS_H__

#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void GenerateAccessorMsgImpl(Context<FieldDescriptor> field);
void GenerateAccessorExternC(Context<FieldDescriptor> field);
void GenerateAccessorThunkCc(Context<FieldDescriptor> field);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSORS_H__
