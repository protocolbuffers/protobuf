// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/hpb/keywords.h"

#include <new>
#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace google::protobuf::hpb_generator {

static const absl::string_view kKeywordList[] = {
    //
    "NULL",
    "alignas",
    "alignof",
    "and",
    "and_eq",
    "asm",
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "class",
    "compl",
    "const",
    "constexpr",
    "const_cast",
    "continue",
    "decltype",
    "default",
    "delete",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "register",
    "reinterpret_cast",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_assert",
    "static_cast",
    "struct",
    "switch",
    "template",
    "this",
    "thread_local",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq",
    "char8_t",
    "char16_t",
    "char32_t",
    "concept",
    "consteval",
    "constinit",
    "co_await",
    "co_return",
    "co_yield",
    "requires",
};

static absl::flat_hash_set<std::string>* MakeKeywordsMap() {
  auto* result = new absl::flat_hash_set<std::string>();
  for (const auto keyword : kKeywordList) {
    result->emplace(keyword);
  }
  return result;
}

static absl::flat_hash_set<std::string>& kKeywords = *MakeKeywordsMap();

std::string ResolveKeywordConflict(absl::string_view name) {
  if (kKeywords.count(name) > 0) {
    return absl::StrCat(name, "_");
  }
  return std::string(name);
}

}  // namespace protobuf
}  // namespace google::hpb_generator
