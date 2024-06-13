// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void UnsupportedField::InMsgImpl(Context& ctx, const FieldDescriptor& field,
                                 AccessorCase accessor_case) const {
  ctx.Emit({{"reason", reason_}}, R"rs(
    // Unsupported! :( Reason: $reason$
    )rs");
  ctx.printer().PrintRaw("\n");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
