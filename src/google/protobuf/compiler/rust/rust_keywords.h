// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_RUST_KEYWORDS_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_RUST_KEYWORDS_H__

#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

// Returns true if the provided str is a 'Raw Identifier', which is a symbol
// which cannot be used even with r# prefix.
bool IsNotLegalEvenWithRPoundPrefix(absl::string_view str);

// Returns true if the provided str is a Rust 2021 Edition keyword and cannot be
// used as a symbol. These symbols can can be used with an r# prefix unless
// IsNotLegalEvenWithRPoundPrefix is true. This function should always match the
// behavior for the corresponding Edition that our emitted crates use.
bool IsRustKeyword(absl::string_view str);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_RUST_KEYWORDS_H__
