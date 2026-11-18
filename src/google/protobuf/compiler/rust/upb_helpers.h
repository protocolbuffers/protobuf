// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_UPB_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_UPB_HELPERS_H__

#include <cstdint>
#include <string>

#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

// The symbol name for the message's MiniTable.
std::string UpbMiniTableName(const Descriptor& msg);

// Returns the symbol name of the MiniTable, qualified relative to the current
// context. This is necessary for referring to a MiniTable in a different
// module.
std::string QualifiedUpbMiniTableName(Context& ctx, const Descriptor& msg);

// The field index that the provided field will be in a upb_MiniTable.
uint32_t UpbMiniTableFieldIndex(const FieldDescriptor& field);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_UPB_HELPERS_H__
