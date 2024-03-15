// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/rust_keywords.h"

#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

bool IsLegalRawIdentifierName(absl::string_view str_without_r_prefix) {
  // These keywords cannot be used even with an r# prefix.
  // https://doc.rust-lang.org/reference/identifiers.html
  static const auto* illegal_raw_identifiers =
      new absl::flat_hash_set<std::string>{"crate", "self", "super", "Self"};
  return !illegal_raw_identifiers->contains(str_without_r_prefix);
}

bool IsRustKeyword(absl::string_view str) {
  // https://doc.rust-lang.org/reference/keywords.html#reserved-keywords
  static const auto* rust_keywords = new absl::flat_hash_set<std::string>{
      "as",     "async",   "await",   "break",    "const",  "continue",
      "crate",  "dyn",     "else",    "enum",     "extern", "false",
      "fn",     "for",     "if",      "impl",     "in",     "let",
      "loop",   "match",   "mod",     "move",     "mut",    "pub",
      "ref",    "return",  "Self",    "self",     "static", "struct",
      "super",  "trait",   "true",    "type",     "union",  "unsafe",
      "use",    "where",   "while",   "abstract", "become", "box",
      "do",     "final",   "macro",   "override", "priv",   "try",
      "typeof", "unsized", "virtual", "yield"};
  return rust_keywords->contains(str);
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
